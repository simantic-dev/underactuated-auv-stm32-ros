"""Jetson on USART2 (57600): ATKP frames AA FF id len data cksum addsum, both ways. Sends the dive as 0x51 control
frames (data[0]=rudder 0..255, data[3]=elevator 0..255, data[4]-data[5]=thrust) and 0x02/0x26 for stop; logs every frame
the STM32 sends (0x20 PWM, 0xF2 DVL velocity, 0x08 position, 0x01 acc/gyro)."""
import struct
import sys, os
sys.path.insert(0, os.getcwd())
import auvprofile as P

CMDS = [  # (t s, rudder, elevator, thrust fwd, thrust rev)
    (P.T_GO, 127, 150, 120, 0), (P.T_LEVEL, 127, 127, 120, 0), (P.T_TURN, 190, 127, 120, 0),
    (P.T_UP, 127, 100, 120, 0), (P.T_STOP - 3, 127, 127, 40, 0),
]
STOP_T = P.T_STOP


def frame(mid, data):
    b = bytes([0xAA, 0xFF, mid, len(data)]) + bytes(data)
    ck = ad = 0
    for x in b:
        ck = (ck + x) & 0xFF
        ad = (ad + ck) & 0xFF
    return b + bytes([ck, ad])


class Peer:
    def __init__(self, ctx):
        self.ctx = ctx
        self.todo = [(t, frame(0x51, [r, 0, 0, e, f, rv, 0, 0])) for t, r, e, f, rv in CMDS]
        self.todo.append((STOP_T, frame(0x02, [0x26, 0, 0, 0])))
        self.rx = bytearray()
        self.arm()

    def arm(self):
        if self.todo:
            self.ctx.schedule_oneshot_ns(max(1, int(self.todo[0][0] * 1e9) - self.ctx.now_ns))

    def on_timer(self):
        while self.todo and int(self.todo[0][0] * 1e9) <= self.ctx.now_ns:
            t, f = self.todo.pop(0)
            self.ctx.info(f"CMD {self.ctx.now_ns / 1e9:.4f} id=0x{f[2]:02X} data={f[4:-2].hex()}")
            self.ctx.send(f)
        self.arm()

    def on_byte(self, b):
        self.rx.append(b if isinstance(b, int) else b[0])
        while len(self.rx) >= 6:
            if self.rx[0] != 0xAA or self.rx[1] != 0xFF:
                del self.rx[0]
                continue
            n = self.rx[3]
            if len(self.rx) < n + 6:
                return
            f = bytes(self.rx[:n + 6])
            del self.rx[:n + 6]
            ck = ad = 0
            for x in f[:-2]:
                ck = (ck + x) & 0xFF
                ad = (ad + ck) & 0xFF
            ok = (ck, ad) == (f[-2], f[-1])
            self.decode(f[2], f[4:-2], ok)

    def decode(self, mid, d, ok):
        t = self.ctx.now_ns / 1e9
        if mid == 0x20 and len(d) >= 6:
            h, e, th = struct.unpack("<HHH", d[:6])
            self.ctx.info(f"PWM {t:.4f} rudder_us={h} elev_us={e} thrust_us={th} ok={ok}")
        elif mid == 0xF2 and len(d) >= 12:
            vx, vy, vz = struct.unpack("<iii", d[:12])
            self.ctx.info(f"DVLV {t:.4f} {vx} {vy} {vz} ok={ok}")
        elif mid == 0x08:
            x, y, ts, z = struct.unpack("<iiIi", d[:16])
            self.ctx.info(f"POS {t:.4f} x_cm={x} y_cm={y} z_cm={z} ok={ok}")
        elif mid == 0x01:
            ay, ax, az, gy, gx, gz = struct.unpack("<6h", d[:12])
            self.ctx.info(f"IMU {t:.4f} acc_cm_s2={ax},{ay},{az} gyro_crad_s={gx},{gy},{gz} ok={ok}")
        else:
            self.ctx.info(f"FRAME {t:.4f} id=0x{mid:02X} {d.hex()} ok={ok}")
