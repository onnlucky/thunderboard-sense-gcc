DEVICE:=EFR32MG1P
PART_NUMBER:=$(DEVICE)132F256GM48

DEV:="/Applications/Simplicity Studio.app/Contents/Eclipse/developer"

COMMANDER:=$(DEV)/adapter_packs/commander/Commander.app/Contents/MacOS/commander

BLE:=$(DEV)/stacks/ble/v2.1.1.0
PROTO:=$(BLE)/protocol/bluetooth_2.1
TOOLCHAIN:=$(DEV)/toolchains/gnu_arm/4.9_2015q3

CC:=$(TOOLCHAIN)/bin/arm-none-eabi-gcc
LD=$(TOOLCHAIN)/bin/arm-none-eabi-gcc
OBJCOPY=$(TOOLCHAIN)/bin/arm-none-eabi-objcopy

DEFINES=-D__NO_SYSTEM_INIT -D$(PART_NUMBER)
CPUFLAGS=-mcpu=cortex-m4 -march=armv7e-m -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=softfp -std=c99
LDFLAGS=$(CPUFLAGS) --specs=nosys.specs $(DEFINES) -Os -gdwarf-2 -Wl,-no-wchar-size-warning,--no-enum-size-warning -lm
CFLAGS=-Wall -Werror -Wno-strict-aliasing -c $(CPUFLAGS) -fno-short-enums $(DEFINES) -Os -lm

STARTUP=startup_efr32mg1p.S
LINKERFILE=-Tefr32mg1p.ld

LIBS=$(PROTO)/lib/binbootloader.o $(PROTO)/lib/binstack.o $(PROTO)/lib/stack.a

INCLUDE=-I$(PROTO)/ble_stack/inc/ -I$(PROTO)/ble_stack/inc/common/ -I$(PROTO)/ble_stack/inc/soc/ -I$(BLE)/platform/emlib/inc/ -I$(BLE)/platform/emlib/src/ -I$(BLE)/platform/Device/SiliconLabs/$(DEVICE)/Include/ -I$(BLE)/platform/CMSIS/Include/ -I$(BLE)/platform/radio/rail_lib/chip/efr32/rf/common/cortex/

SOURCES:=$(shell echo *.c)
OBJECTS:=$(SOURCES:.c=.o)

all: BLE_device.bin

# GATT definition
gatt_db.h gatt_db.c: gatt.bgproj gatt.xml
	$(PROTO)/bin/bgbuild -gn -c $(TOOLCHAIN) $<

# ELF file of application & stack
BLE_device.elf: $(OBJECTS) Makefile
	$(LD) $(LDFLAGS) $(OBJECTS) $(LIBS) $(LINKERFILE) $(STARTUP) -o $@

%.o:%.c *.h Makefile
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

gatt_db.c: gatt_db.h
main.c: gatt_db.h

# binary file for eacommander
BLE_device.bin: BLE_device.elf
	$(OBJCOPY) $< $@ --gap-fill 255 -O binary

# flash
load: BLE_device.bin
	$(COMMANDER) flash $<

run: load

clean:
	rm -rf $(OBJECTS) gatt_db.* constants BLE_device.*

.PHONY: all run load clean
