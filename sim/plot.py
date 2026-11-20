"""Decode a run.log of the AUV scenario and draw the dive. Usage: plot.py run.log dive.png measured.txt
Sources in run.log: ATKP frames on the USB CDC console `(usb_otg_fs)`, `[servo] SERVO` lines (USART3), `(MEM)` writes to the
Depth variable and to TIM8 CCR3, and `[jetson] CMD` lines."""
import re, struct, sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

log, png, out = sys.argv[1:4]
usb, servo, ccr, depth_w, cmds, cur = [], [], [], {}, [], {}
for ln in open(log, errors="replace"):
    m = re.match(r"\[([\d.]+)s\] \(usb_otg_fs\) ((?:[0-9A-F]{2} ?)+)", ln)
    if m:
        usb += [(float(m[1]), int(h, 16)) for h in m[2].split()]
        continue
    m = re.match(r"\[servo\] SERVO ([\d.]+) PWM1:(\d+),PWM2:(\d+),PWM3:(\d+),PWM4:(\d+)", ln)
    if m:
        servo.append((float(m[1]),) + tuple(int(x) for x in m.groups()[1:]))
        continue
    m = re.match(r"\[([\d.]+)s\] \(MEM\) 0x([0-9A-Fa-f]+):4 write addr=0x([0-9a-f]+) width=(\d) value=0x([0-9a-f]+)", ln)
    if m:
        t, base, addr, w, v = float(m[1]), int(m[2], 16), int(m[3], 16), int(m[4]), int(m[5], 16)
        if base == 0x4001043C:
            ccr.append((t, v))
        else:
            cur.setdefault("b", bytearray(4))[addr - base:addr - base + w] = v.to_bytes(w, "little")
            if addr - base + w >= 4:       # last byte of the float written: one Depth update
                depth_w[t] = cur.pop("b")
        continue
    m = re.match(r"\[jetson\] CMD ([\d.]+) id=(0x\w+) data=(\w+)", ln)
    if m:
        cmds.append((float(m[1]), m[2], m[3]))

# ATKP frames AA FF id len data ck add
frames, bad, i, b = [], 0, 0, [x for _, x in usb]
ts = [t for t, _ in usb]
while i + 6 <= len(b):
    if b[i] != 0xAA or b[i + 1] != 0xFF:
        i += 1
        continue
    n = b[i + 3]
    if i + n + 6 > len(b):
        break
    f = b[i:i + n + 6]
    ck = ad = 0
    for x in f[:-2]:
        ck = (ck + x) & 255
        ad = (ad + ck) & 255
    if (ck, ad) == (f[-2], f[-1]):
        frames.append((ts[i], f[2], bytes(f[4:-2])))
    else:
        bad += 1
    i += n + 6
by = {}
for t, mid, d in frames:
    by.setdefault(mid, []).append((t, d))

dv = [(t, *struct.unpack("<3i", d[:12])) for t, d in by.get(0xF2, [])]
pos = [(t, *struct.unpack("<iiIi", d[:16])) for t, d in by.get(0x08, [])]
imu = [(t, *struct.unpack("<6h", d[:12])) for t, d in by.get(0x01, [])]
pwmf = [(t, *struct.unpack("<HHH", d[:6])) for t, d in by.get(0x20, [])]
dep = sorted((t, struct.unpack("<f", bytes(v))[0]) for t, v in depth_w.items() if len(v) == 4)
dep_ok = [(t, d) for t, d in dep if d == 0 or 1e-3 <= d < 100]
dep_bad = [(t, d) for t, d in dep if not (d == 0 or 1e-3 <= d < 100)]

fig, ax = plt.subplots(4, 1, figsize=(9, 10), sharex=True)
for t, _, _ in cmds:
    for a in ax:
        a.axvline(t, color="0.8", lw=0.8)
ax[0].plot(*zip(*dep_ok), label="Depth variable (serial depth sensor, m)")
ax[0].plot([p[0] for p in pos], [p[4] / 100 for p in pos], "--", label="DVL dead-reckoning z, via USB telemetry (m)")
ax[0].set_ylabel("depth, m"); ax[0].invert_yaxis(); ax[0].legend(loc="upper left", fontsize=8)
ax[1].plot([p[0] for p in dv], [p[1] / 1000 for p in dv], label="DVL vx (m/s)")
ax[1].plot([p[0] for p in dv], [p[3] / 1000 for p in dv], label="DVL vz (m/s)")
ax[1].plot([p[0] for p in pos], [p[1] / 100 for p in pos], label="DVL x (m)")
ax[1].plot([p[0] for p in pos], [p[2] / 100 for p in pos], label="DVL y (m)")
ax[1].legend(loc="upper left", fontsize=8); ax[1].set_ylabel("DVL")
ax[2].plot([p[0] for p in imu], [p[6] / 100 for p in imu])
ax[2].set_ylabel("IMU gyro z, rad/s")
if ccr:
    ax[3].step([c[0] for c in ccr], [c[1] for c in ccr], where="post", label="TIM8 CCR3 thruster (us)")
ax[3].step([s[0] for s in servo], [s[1] for s in servo], where="post", label="rudder PWM1 (us, USART3)")
ax[3].step([s[0] for s in servo], [s[3] for s in servo], where="post", label="elevator PWM3 (us, USART3)")
ax[3].legend(loc="upper left", fontsize=8); ax[3].set_ylabel("PWM, us"); ax[3].set_xlabel("virtual time, s")
ax[3].set_xlim(0, 40)
fig.suptitle("AUV firmware: scripted dive, Jetson commands (grey lines) -> thruster / servo outputs")
fig.tight_layout(); fig.savefig(png, dpi=110)

def at(series, t, k=1):
    r = [s for s in series if s[0] <= t]
    return r[-1][k] if r else None

lines = []
w = lines.append
w(f"ATKP frames decoded from USB CDC: {len(frames)} good, {bad} bad checksum")
for mid, name in ((0x01, "0x01 acc/gyro"), (0x08, "0x08 position"), (0xF2, "0xF2 DVL velocity"), (0x20, "0x20 PWM")):
    ts_ = [t for t, _ in by.get(mid, [])]
    w(f"  {name}: {len(ts_)} frames, first at {ts_[0]:.3f} s, last at {ts_[-1]:.3f} s" if ts_ else f"  {name}: none")
w(f"Jetson commands sent on USART2: {len(cmds)}: " + "; ".join(f"{t:.1f}s {i}" for t, i, _ in cmds))
if ccr:
    first_cmd = cmds[0][0] if cmds else 0
    w(f"TIM8 CCR3 (thruster) writes: {len(ccr)}; first write after boot {ccr[1][1]} us at {ccr[1][0]:.3f} s; first non-neutral value "
      f"{next((v for t, v in ccr if t > 1 and v != 1500), None)} us at {next((t for t, v in ccr if t > 1 and v != 1500), float('nan')):.3f} s; after the 12 s command: {at(ccr, 13.0)} us; "
      f"after the 33 s command: {at(ccr, 34.0)}; after stop (36 s): {at(ccr, 37.0)}")
    burst = [t for t, _ in ccr if 7.1 <= t < 7.3]
    w(f"TIM8 CCR3 stabilizer loop catch-up: {len(burst)} writes between 7.1 and 7.3 s (loop period is 2 ms: nominal 100); "
      f"median gap {sorted(b - a for a, b in zip(burst, burst[1:]))[len(burst) // 2] * 1e6:.0f} us" if len(burst) > 2 else "no burst")
    steady = [b - a for (a, _), (b, _) in zip(ccr, ccr[1:]) if 8 < a < 35]
    w(f"TIM8 CCR3 steady period 8..35 s: median {sorted(steady)[len(steady) // 2] * 1e3:.3f} ms ({len(steady)} writes)")
if servo:
    w(f"USART3 servo lines: {len(servo)}; at 10 s {at(servo,10.0,1)},{at(servo,10.0,2)},{at(servo,10.0,3)},{at(servo,10.0,4)} us (PWM1..4); at 14 s {at(servo,14.0,1)},{at(servo,14.0,3)} (rudder, elevator); "
      f"at 24 s {at(servo,24.0,1)},{at(servo,24.0,3)}; at 31 s {at(servo,31.0,1)},{at(servo,31.0,3)}; at 37 s {at(servo,37.0,1)},{at(servo,37.0,3)}")
if dep_ok:
    w(f"Depth variable: {len(dep)} writes, {len(dep_ok)} plausible; max {max(d for _, d in dep_ok):.3f} m at "
      f"{max(dep_ok, key=lambda x: x[1])[0]:.1f} s; at 20 s {at(dep_ok, 20.0):.3f} m (profile 1.500), at 36 s {at(dep_ok, 36.0):.3f} m")
if dep_bad:
    w("Depth variable implausible values: " + ", ".join(f"{d!r} at {t:.3f} s" for t, d in dep_bad[:5]))
if dv:
    w(f"DVL vx at 18 s {at(dv,18.0,1)/1000:.3f} m/s (profile 0.600); vz at 16 s {at(dv,16.0,3)/1000:.3f} m/s (profile 0.1875)")
if pos:
    w(f"DVL dead-reckoning at 36 s: x {at(pos,36.0,1)/100:.2f} m, y {at(pos,36.0,2)/100:.2f} m, z {at(pos,36.0,4)/100:.2f} m")
if imu:
    w(f"IMU gyro z reported at 26 s: {at(imu,26.0,6)/100:.2f} rad/s (profile 0.196, the firmware truncates to 0.01 rad/s)")
open(out, "w").write("\n".join(lines) + "\n")
