"""Serial depth sensor on UART8 (115200): ASCII 'T=<temp>D=<depth m>' at 10 Hz, parsed by sscanf("T=%fD=%f")."""
import sys, os
sys.path.insert(0, os.getcwd())
import auvprofile as P


class Peer:
    def __init__(self, ctx):
        self.ctx = ctx
        ctx.schedule_periodic_ns(100_000_000)

    def on_timer(self):
        t = self.ctx.now_ns / 1e9
        self.ctx.send(f"T=18.5D={P.depth(t):.3f}\r\n".encode())

    def on_byte(self, b):
        pass
