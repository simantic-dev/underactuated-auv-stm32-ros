"""Animates run.log (sim's own capture) into dive.gif: depth the firmware holds, thruster PWM (TIM8 CCR3), Jetson commands.
usage: make_gif.py run.log dive.gif "tag"   (matplotlib, pillow)"""
import re, struct, sys, io
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from PIL import Image
ccr, dep, cmds, cur = [], [], [], bytearray(4)
for ln in open(sys.argv[1], errors="replace"):
    m = re.match(r"\[([\d.]+)s\] \(MEM\) 0x([0-9A-Fa-f]+):4 write addr=0x([0-9a-f]+) width=(\d) value=0x([0-9a-f]+)", ln)
    if m:
        t, base, addr, w, v = float(m[1]), int(m[2], 16), int(m[3], 16), int(m[4]), int(m[5], 16)
        if base == 0x4001043C: ccr.append((t, v))
        else:
            cur[addr - base:addr - base + w] = v.to_bytes(w, "little")
            if addr - base + w >= 4:
                d = struct.unpack("<f", bytes(cur))[0]
                if 0 <= d < 10: dep.append((t, d))
        continue
    m = re.match(r"\[jetson\] CMD ([\d.]+) id=(0x\w+)", ln)
    if m: cmds.append((float(m[1]), m[2]))
end = 40.0; frames = []
for k in range(1, 41):
    tc = end * k / 40
    fig, ax = plt.subplots(2, 1, figsize=(7, 4.4), dpi=70, sharex=True)
    d = [x for x in dep if x[0] <= tc]; c = [x for x in ccr if x[0] <= tc]
    ax[0].plot([t for t, _ in d], [v for _, v in d], ".", ms=2); ax[0].invert_yaxis(); ax[0].set_ylim(1.7, -0.1); ax[0].set_ylabel("Depth variable, m", fontsize=7)
    ax[1].plot([t for t, _ in c], [v for _, v in c], "-", c="C3", lw=.8); ax[1].set_ylim(1400, 1750); ax[1].set_ylabel("TIM8 CCR3 thruster, us", fontsize=7)
    for t, i in cmds:
        if t <= tc:
            for a in ax: a.axvline(t, c="k", lw=.5, ls=":")
            ax[1].text(t, 1735, i, rotation=90, fontsize=6, va="top")
    ax[1].set_xlim(0, end); ax[1].set_xlabel("virtual time, s")
    ax[0].set_title(f"AUV firmware on simulated H743, t={tc:.0f} s virtual, {sys.argv[3]}", fontsize=8)
    fig.tight_layout(); bf = io.BytesIO(); fig.savefig(bf, format="png"); plt.close(fig)
    frames.append(Image.open(bf).convert("RGB").quantize(colors=64, dither=Image.Dither.NONE))
frames[0].save(sys.argv[2], save_all=True, append_images=frames[1:], duration=[120] * 39 + [1500], loop=0, optimize=True)
