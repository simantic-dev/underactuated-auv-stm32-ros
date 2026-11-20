"""Water Linked style DVL on USART1 (115200): 'wrz,...*CRC8' velocity and 'wrp,...*CRC8' dead-reckoning lines at 5 Hz.
The firmware skips 9 characters of the time_of_validity/time_of_transmission fields, so they are 16-digit microseconds."""
import math
import sys, os
sys.path.insert(0, os.getcwd())
import auvprofile as P


def crc8(s):
    c = 0
    for b in s.encode():
        c ^= b
        for _ in range(8):
            c = ((c << 1) ^ 0x07) & 0xFF if c & 0x80 else (c << 1) & 0xFF
    return c


def line(body):
    return f"{body}*{crc8(body):02x}\r\n".encode()


class Peer:
    def __init__(self, ctx):
        self.ctx = ctx
        ctx.schedule_periodic_ns(200_000_000)

    def on_timer(self):
        t = self.ctx.now_ns / 1e9
        us = 1_700_000_000_000_000 + self.ctx.now_ns // 1000
        x, y = P.pos(t)
        h = math.radians(P.yaw(t))
        v = P.speed(t)
        wrz = (f"wrz,{v:.3f},0.000,{P.vdown(t):.3f},y,{max(5.0 - P.depth(t), 0.1):.2f},0.001,"
               f"1e-5;0;0;0;1e-5;0;0;0;1e-5,{us},{us + 3000},{t * 1000:.1f},0")
        wrp = f"wrp,{t:.3f},{x:.3f},{y:.3f},{P.depth(t):.3f},0.01,0.0,0.0,{math.degrees(h):.2f},0"
        self.ctx.send(line(wrz) + line(wrp))

    def on_byte(self, b):
        pass
