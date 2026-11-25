"""Dashboard GIF of the AUV run: usage make_gif.py run.log dive.gif "<sim version>". Pillow + ffmpeg only (gifkit.py).
Same decoding as plot.py: ATKP frames on the USB CDC (AA FF id len data ck add), the Depth variable and TIM8 CCR3 writes
(--trace-memory), USART3 servo text lines, the Jetson peer's command log."""
import os, re, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gifkit import *

VER = sys.argv[3]
usb, servo, ccr, depw, cmds, cur = [], [], [], [], [], bytearray(4)
for ln in open(sys.argv[1], errors="replace"):
    m = re.match(r"\[([\d.]+)s\] \(usb_otg_fs\) ((?:[0-9A-F]{2} ?)+)", ln)
    if m: usb += [(float(m[1]), int(h, 16)) for h in m[2].split()]; continue
    m = re.match(r"\[servo\] SERVO ([\d.]+) PWM1:(\d+),PWM2:(\d+),PWM3:(\d+),PWM4:(\d+)", ln)
    if m: servo.append((float(m[1]),) + tuple(int(x) for x in m.groups()[1:])); continue
    m = re.match(r"\[([\d.]+)s\] \(MEM\) 0x([0-9A-Fa-f]+):4 write addr=0x([0-9a-f]+) width=(\d) value=0x([0-9a-f]+)", ln)
    if m:
        t, base, addr, w, v = float(m[1]), int(m[2], 16), int(m[3], 16), int(m[4]), int(m[5], 16)
        if base == 0x4001043C: ccr.append((t, v))
        else:
            cur[addr - base:addr - base + w] = v.to_bytes(w, "little")
            if addr - base + w >= 4:
                d = struct.unpack("<f", bytes(cur))[0]
                if d == 0 or 1e-3 <= d < 100: depw.append((t, d))
        continue
    m = re.match(r"\[jetson\] CMD ([\d.]+) id=(0x\w+) data=(\w+)", ln)
    if m: cmds.append((float(m[1]), m[2], bytes.fromhex(m[3])))
b = [x for _, x in usb]; ts = [t for t, _ in usb]; frames = []; i = 0
while i + 6 <= len(b):
    if b[i] != 0xAA or b[i + 1] != 0xFF: i += 1; continue
    n = b[i + 3]
    if i + n + 6 > len(b): break
    f = b[i:i + n + 6]; ck = ad = 0
    for x in f[:-2]: ck = (ck + x) & 255; ad = (ad + ck) & 255
    if (ck, ad) == (f[-2], f[-1]): frames.append((ts[i], f[2], bytes(f[4:-2])))
    i += n + 6
by = {}
for t, mid, d in frames: by.setdefault(mid, []).append((t, d))
dv = [(t, *struct.unpack("<3i", d[:12])) for t, d in by[0xF2]]
pos = [(t, *struct.unpack("<iiIi", d[:16])) for t, d in by[0x08]]
imu = [(t, *struct.unpack("<6h", d[:12])) for t, d in by[0x01]]
pwm = [(t, *struct.unpack("<HHH", d[:6])) for t, d in by[0x20]]
END = 40.0
ccr = [x for x in ccr if x[1] >= 1000]   # drop the pre-init zeros of the timer register
sv = [s_ for i, s_ in enumerate(servo) if i == 0 or s_[1:] != servo[i - 1][1:]]   # only changes of the servo lines
# the Jetson peer's command table (jetson_peer.py CMDS): what each 0x51 control frame asks for
PHASES = ["DIVE", "LEVEL", "TURN", "ASCEND", "THRUST DOWN", "STOP"]
ptime = [c[0] for c in cmds]
log = sorted([(c[0], "WARN", f"jetson -> auv ATKP id={c[1]} data={c[2].hex()}") for c in cmds] +
             [(s[0], "INFO", f"servo USART3: PWM1:{s[1]} PWM2:{s[2]} PWM3:{s[3]} PWM4:{s[4]} us") for s in sv])

def cur_(series, t, k=1):
    r = [s for s in series if s[0] <= t]
    return r[-1] if r else None

def draw(c, t):
    c.header("Underactuated AUV firmware (STM32H7) on a scripted dive", "team firmware, unmodified. Jetson, depth sensor, DVL, IMU and servo board are scripted peers; the dive profile is the stimulus, not a vehicle model.", t)
    d = cur_(depw, t); v = cur_(dv, t); p = cur_(pos, t); g = cur_(imu, t); cc = cur_(ccr, t)
    tw = 196; X = [24 + i * (tw + 12) for i in range(6)]
    c.tile(X[0], 90, tw, 92, "DEPTH VARIABLE", f"{d[1]:.2f}" if d else "--", "m", CYAN)
    c.tile(X[1], 90, tw, 92, "DVL FORWARD", f"{v[1] / 1000:.2f}" if v else "--", "m/s", CYAN)
    c.tile(X[2], 90, tw, 92, "DVL DESCENT RATE", f"{v[3] / 1000:.2f}" if v else "--", "m/s", CYAN)
    c.tile(X[3], 90, tw, 92, "DEAD RECKONING X,Y", f"{p[1] / 100:.1f},{p[2] / 100:.1f}" if p else "--", "m", AMBER)
    c.tile(X[4], 90, tw, 92, "GYRO Z (IMU)", f"{g[6] / 100:.2f}" if g else "--", "rad/s", AMBER)
    c.tile(X[5], 90, tw, 92, "THRUSTER PWM (TIM8)", f"{cc[1]}" if cc else "--", "us", GREEN if cc and cc[1] != 1500 else FG)
    c.panel(24, 196, 1232, 112, "Jetson command phase (control frames 0x51 sent by the scripted Jetson; time of each frame)")
    nct = len([x for x in ptime if x <= t])
    times = [x if x <= t else None for x in ptime[:5]] + [None]
    c.chain(40, 226, 1200, 70, PHASES, (nct - 1 if nct else None), times, [BLUE, BLUE, AMBER, GREEN, AMBER, RED], sz=16)
    c.strip(24, 322, 620, 180, [(a, x) for a, x in depw], t, END, 0, 1.6, "depth variable the firmware holds (m)", CYAN, "m", step=True, ticks=4)
    c.strip(24, 512, 620, 170, [(a, x) for a, x in ccr[::40]], t, END, 1400, 1700, "thruster PWM, TIM8 CCR3 (us)", GREEN, "us", step=True, ticks=3)
    c.log(656, 322, 600, 360, log, t, "traffic: Jetson commands in (USART2), servo lines out (USART3), on change", sz=13)
    c.footer(VER, SPEED)

SPEED = END / 15
print("frames", render(draw, END, SPEED, sys.argv[2]))
