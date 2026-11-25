"""Tiny PIL dashboard kit -> GIF through ffmpeg. Pillow + ffmpeg only.
usage: render(draw, vt_end, speed, out, fps=12, size=(1280, 720))   draw(c, t) paints virtual time t."""
import subprocess
from PIL import Image, ImageDraw, ImageFont

BG, PANEL, EDGE = (16, 20, 28), (26, 32, 44), (52, 62, 80)
FG, DIM = (232, 237, 245), (150, 160, 178)
GREEN, AMBER, RED, BLUE, CYAN, VIOLET = (80, 210, 120), (255, 190, 60), (255, 90, 90), (90, 160, 255), (80, 220, 230), (190, 140, 255)
LVL = {"INFO": FG, "DEBUG": DIM, "WARN": AMBER, "ERROR": RED, "FAULT": RED}
_f = {}


def font(sz, bold=False, mono=False):
    k = (sz, bold, mono)
    if k not in _f:
        _f[k] = (ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", sz, index=1 if bold else 0) if mono
                 else ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", sz, index=1 if bold else 0))
    return _f[k]


class C:
    def __init__(self, size):
        self.im = Image.new("RGB", size, BG)
        self.d = ImageDraw.Draw(self.im)
        self.w, self.h = size

    def text(self, x, y, s, sz=16, fill=FG, bold=False, mono=False, anchor="la"):
        self.d.text((x, y), s, font=font(sz, bold, mono), fill=fill, anchor=anchor)

    def panel(self, x, y, w, h, title=None):
        self.d.rounded_rectangle((x, y, x + w, y + h), 8, fill=PANEL, outline=EDGE)
        if title:
            self.text(x + 14, y + 8, title, 15, DIM, True)

    def header(self, title, sub, t):
        self.text(24, 12, title, 28, FG, True)
        self.text(24, 48, sub, 16, DIM)
        self.text(self.w - 24, 14, f"{t:7.3f} s", 30, CYAN, True, True, "ra")
        self.text(self.w - 24, 50, "virtual time", 14, DIM, anchor="ra")

    def footer(self, version, speed, extra=""):
        self.text(24, self.h - 26, f"simulated on Simantic sim {version}, virtual time, playback {speed:.2f}x{extra}", 14, DIM)

    def tile(self, x, y, w, h, label, value, unit="", col=FG):
        self.panel(x, y, w, h)
        self.text(x + 14, y + 8, label, 15, DIM, True)
        self.text(x + 14, y + h - 14, value, 34, col, True, False, "ld")
        if unit:
            self.d.text((x + 14 + self.d.textlength(value, font=font(34, True)) + 6, y + h - 16), unit, font=font(16), fill=DIM, anchor="ld")

    def chain(self, x, y, w, h, names, active, times, colors=None, sz=18):
        """Flight/mode style state chain. times[i] = virtual time state i was entered (or None)."""
        n = len(names); gap = 10; bw = (w - gap * (n - 1)) / n
        for i, nm in enumerate(names):
            bx = x + i * (bw + gap)
            reached = times[i] is not None
            on = i == active
            col = (colors[i] if colors else GREEN) if reached else EDGE
            self.d.rounded_rectangle((bx, y, bx + bw, y + h), 8, fill=col if on else PANEL, outline=col, width=2)
            self.text(bx + bw / 2, y + h * 0.38, nm, sz, BG if on else (FG if reached else DIM), True, False, "mm")
            self.text(bx + bw / 2, y + h * 0.75, f"{times[i]:.3f} s" if reached else "-", 14, BG if on else DIM, False, True, "mm")

    def strip(self, x, y, w, h, series, t, tmax, lo, hi, label, col=CYAN, unit="", step=False, ticks=4):
        self.panel(x, y, w, h)
        self.text(x + 14, y + 8, label, 15, DIM, True)
        px, py, pw, ph = x + 60, y + 34, w - 80, h - 56
        for i in range(ticks + 1):
            v = lo + (hi - lo) * i / ticks
            yy = py + ph - ph * i / ticks
            self.d.line((px, yy, px + pw, yy), fill=EDGE)
            self.text(px - 6, yy, f"{v:g}", 13, DIM, anchor="rm")
        pts = [(px + pw * min(a, t) / tmax, py + ph - ph * (min(max(b, lo), hi) - lo) / (hi - lo)) for a, b in series if a <= t]
        if pts:
            if step:
                sp = []
                for i, p in enumerate(pts):
                    if i: sp.append((p[0], pts[i - 1][1]))
                    sp.append(p)
                sp.append((px + pw * t / tmax, pts[-1][1])); pts = sp
            if len(pts) > 1: self.d.line(pts, fill=col, width=3)
            self.d.ellipse((pts[-1][0] - 4, pts[-1][1] - 4, pts[-1][0] + 4, pts[-1][1] + 4), fill=col)
            cur = [b for a, b in series if a <= t][-1]
            self.text(x + w - 14, y + 8, f"{cur:g} {unit}", 16, col, True, False, "ra")
        cx = px + pw * t / tmax
        self.d.line((cx, py, cx, py + ph), fill=DIM)

    def log(self, x, y, w, h, lines, t, title="firmware console (UART, virtual time)", sz=14, width=None):
        """lines: (t, level, text). Shows the newest that fit."""
        self.panel(x, y, w, h, title)
        lh = sz + 5
        n = int((h - 36) // lh)
        vis = [l for l in lines if l[0] <= t][-n:]
        mw = width or int((w - 28) / (sz * 0.602))
        for i, (lt, lv, tx) in enumerate(vis):
            s = f"[{lt:7.3f}] {tx}"
            self.text(x + 14, y + 32 + i * lh, s if len(s) <= mw else s[:mw - 3] + '...', sz, LVL.get(lv, FG), False, True)


def render(draw, vt_end, speed, out, fps=12, size=(1280, 720), tail=1.5):
    nf = int(vt_end / speed * fps)
    p = subprocess.Popen(["ffmpeg", "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{size[0]}x{size[1]}", "-r", str(fps), "-i", "-",
                          "-vf", "split[a][b];[a]palettegen=max_colors=64:stats_mode=full[p];[b][p]paletteuse=dither=none", "-loop", "0", out], stdin=subprocess.PIPE)
    for k in range(nf + int(tail * fps)):
        c = C(size); draw(c, min(vt_end, k * speed / fps)); p.stdin.write(c.im.tobytes())
    p.stdin.close(); p.wait()
    return nf + int(tail * fps)
