# Added for the GCC build (the upstream project is Keil MDK-ARM). Usage: make -C sim
R := ..
CC := arm-none-eabi-gcc
FW := $(R)/Middlewares/Third_Party/FreeRTOS/Source
HAL := $(R)/Drivers/STM32H7xx_HAL_Driver
HALSRC := cortex dma_ex dma exti flash_ex flash gpio hsem i2c_ex i2c mdma pcd_ex pcd pwr_ex pwr rcc_ex rcc spi_ex spi tim_ex tim uart_ex uart
SRC := $(wildcard $(R)/Core/Src/*.c) $(addprefix $(HAL)/Src/stm32h7xx_hal_,$(addsuffix .c,$(HALSRC))) $(HAL)/Src/stm32h7xx_hal.c $(HAL)/Src/stm32h7xx_ll_usb.c \
  $(wildcard $(R)/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_*.c) $(R)/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c \
  $(wildcard $(R)/USB_DEVICE/App/*.c) $(R)/USB_DEVICE/Target/usbd_conf.c \
  $(FW)/list.c $(FW)/queue.c $(FW)/tasks.c $(FW)/timers.c $(FW)/event_groups.c $(FW)/stream_buffer.c $(FW)/croutine.c $(FW)/CMSIS_RTOS_V2/cmsis_os2.c $(FW)/portable/MemMang/heap_4.c port/port.c \
  $(R)/COMMUNICATE/atpk.c $(R)/COMMUNICATE/Uart2link.c $(R)/COMMUNICATE/usblink.c $(R)/Debug/debug.c $(R)/DVL/DVL.c $(R)/EKF/EKF.c $(R)/EKF/Math_Ekf.c $(R)/GPS/GPS.c \
  $(R)/HARDWARE/deph.c $(R)/HARDWARE/imu.c $(R)/moter/moter.c $(R)/PID/pid.c $(R)/stabilizer/stabilizer.c $(R)/WS2812/WS2812.c syscalls.c
INC := Core/Inc Drivers/STM32H7xx_HAL_Driver/Inc Drivers/STM32H7xx_HAL_Driver/Inc/Legacy Middlewares/Third_Party/FreeRTOS/Source/include Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 \
  Drivers/CMSIS/Device/ST/STM32H7xx/Include Drivers/CMSIS/Include HARDWARE USB_DEVICE/App USB_DEVICE/Target Middlewares/ST/STM32_USB_Device_Library/Core/Inc \
  Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc Debug COMMUNICATE stabilizer moter WS2812 PID DVL GPS EKF
CFLAGS := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard -Os -g -ffunction-sections -fdata-sections -w -DUSE_HAL_DRIVER -DSTM32H750xx -DUSE_PWR_LDO_SUPPLY \
  $(addprefix -I$(R)/,$(INC)) -Iport
LDFLAGS := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard -TSTM32H750VBTx_FLASH.ld -specs=nano.specs -specs=nosys.specs -u _printf_float -u _scanf_float -Wl,--gc-sections -Wl,-Map=auv.map
OBJ := $(patsubst %.c,build/%.o,$(subst $(R)/,,$(SRC))) build/startup.o
auv.elf: $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -lc -lm -lnosys -o $@
	arm-none-eabi-size $@
build/startup.o: startup_stm32h750xx.s
	@mkdir -p $(dir $@); $(CC) -mcpu=cortex-m7 -mthumb -c $< -o $@
build/%.o: $(R)/%.c
	@mkdir -p $(dir $@); $(CC) $(CFLAGS) -c $< -o $@
build/%.o: %.c
	@mkdir -p $(dir $@); $(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -rf build auv.elf auv.map
