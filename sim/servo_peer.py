"""Rudder/elevator sub-controller on USART3 (115200). The firmware only prints the servo commands there
('PWM1:..,PWM2:..,PWM3:..,PWM4:..'); this peer logs them with virtual time."""


class Peer:
    def __init__(self, ctx):
        self.ctx = ctx
        self.buf = b""

    def on_byte(self, b):
        self.buf += bytes([b]) if isinstance(b, int) else b
        while b"\n" in self.buf:
            ln, self.buf = self.buf.split(b"\n", 1)
            ln = ln.strip(b"\r\x00").decode(errors="replace")
            if ln.startswith("PWM1"):
                self.ctx.info(f"SERVO {self.ctx.now_ns / 1e9:.4f} {ln}")
