"""Xsens MTi-630 on UART4 (2 Mbaud, 100 Hz): FA FF 36 LEN [id16 len data]... CS, big-endian float32 triplets
(Euler 0x2030, Acceleration 0x4020, FreeAcceleration 0x4030, RateOfTurn 0x8020). Attitude follows auvprofile."""
import math
import struct
import sys, os
sys.path.insert(0, os.getcwd())
import auvprofile as P


def rec(did, vals):
    return struct.pack(">HB3f", did, 12, *vals)


class Peer:
    def __init__(self, ctx):
        self.ctx = ctx
        ctx.schedule_periodic_ns(10_000_000)

    def on_timer(self):
        t = self.ctx.now_ns / 1e9
        pitch = -8.0 * (P.vdown(t) > 0) + 8.0 * (P.vdown(t) < 0)
        yaw = P.yaw(t)
        wz = (P.yaw(t + 0.01) - yaw) / 0.01
        body = (rec(0x2030, (0.0, pitch, yaw if yaw <= 180 else yaw - 360)) + rec(0x4020, (0.0, 0.0, -9.81))
                + rec(0x4030, (0.0, 0.0, 0.0)) + rec(0x8020, (0.0, 0.0, math.radians(wz))))
        pkt = bytes([0xFA, 0xFF, 0x36, len(body)]) + body
        self.ctx.send(pkt + bytes([(-sum(pkt[1:])) & 0xFF]))

    def on_byte(self, b):
        pass
