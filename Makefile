CC=nios2-elf-gcc
LD=nios2-elf-ld
CFLAGS=-c -nostdinc -W -Wall -G0 -O2 -mhw-div -DDEBUG
ONBOARD_FLASH=0x01400000

OBJECTS    = start.o utils.o my_boot.o jtag.o fonts.o graphic.o string.o timer.o fat32.o sdcard.o menu.o
MEMORY_MAP = my_boot.x
OUT_ELF    = my_boot.elf
FLASH_FILE = my_boot.flash
LD_SCRIPT  = my_boot.ld

.PHONY: clean flash

#
# Flash-tiedoston luonti
#
$(FLASH_FILE): $(OUT_ELF)
	elf2flash --input=$(OUT_ELF) --output=$(FLASH_FILE) \
	--base $(ONBOARD_FLASH) \
	--end=0xffffffff --reset=$(ONBOARD_FLASH)

#
# Linkkeri
#
$(OUT_ELF): $(OBJECTS) $(LD_SCRIPT)
	$(LD) -o $(OUT_ELF) $(OBJECTS) -T $(LD_SCRIPT) -Map memorymap

#
# Käsitellään muistikartta cppllä
#
$(LD_SCRIPT): $(MEMORY_MAP)
	cpp $(MEMORY_MAP) >$(LD_SCRIPT)
	sed -i -e 's/^#.*$$//g' $(LD_SCRIPT)

start.o: start.S
	$(CC) $(CFLAGS) start.S

utils.o: utils.S
	$(CC) $(CFLAGS) utils.S

clean:
	rm -f $(OUT_ELF) $(FLASH_FILE) $(LD_SCRIPT) $(OBJECTS)

#
# Flashin kirjoitus laudalle
#
flash: $(FLASH_FILE)
	nios2-flash-programmer --base $(ONBOARD_FLASH) $(FLASH_FILE)
