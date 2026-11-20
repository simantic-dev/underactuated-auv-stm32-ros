#!/usr/bin/env bash
# Run the AUV firmware (built by `make`) in the simulator and write run.log, measured.txt and dive.png.
# Needs `sim` (https://simantic.com) on PATH; arm-none-eabi-gcc only to rebuild auv.elf; python3 + matplotlib for the plot.
set -euo pipefail
cd "$(dirname "$0")"
[ -f auv.elf ] || make
# Depth variable (0x24016820 here: `arm-none-eabi-nm auv.elf | grep ' Depth$'`) and TIM8 CCR3 (thruster PWM, 0x4001043C)
DEPTH=0x$(arm-none-eabi-nm auv.elf | awk '$3=="Depth"{print $1}')
sim run --scenario scenario.yaml --ascii lpuart1 --timeout 40 --trace-memory "$DEPTH:4" --trace-memory 0x4001043C:4 2>&1 \
  | grep -vE '\((uart|usart|lpuart)[0-9]* [<>←→]' > run.log
${PYTHON:-python3} plot.py run.log dive.png measured.txt
cat measured.txt
