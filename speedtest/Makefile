CC=nios2-elf-gcc
LD=nios2-elf-ld
CFLAGS=-c -nostdinc -W -Wall -G0 -mhw-div -O -I ..

OBJECTS    = start.o speedtest.o
MEMORY_MAP = speedtest.x
ELF_OUT    = speedtest.elf
BINARY     = speedtest.bin
LD_SCRIPT  = speedtest.ld

.PHONY: clean

#
# Binäärin teko
#
$(BINARY): $(ELF_OUT)
	nios2-elf-objcopy -O binary $(ELF_OUT) $(BINARY)

#
# Linkkeri
#
$(ELF_OUT): $(OBJECTS) $(LD_SCRIPT)
	$(LD) -o $(ELF_OUT) $(OBJECTS) ../fonts.o ../graphic.o ../string.o \
		../timer.o ../utils.o -T $(LD_SCRIPT)

#
# Käsitellään muistikartta cppllä
#
$(LD_SCRIPT): $(MEMORY_MAP)
	cpp $(MEMORY_MAP) >$(LD_SCRIPT)
	sed -i -e 's/^#.*$$//g' $(LD_SCRIPT)

start.o: start.S
	$(CC) $(CFLAGS) start.S

clean:
	rm -f $(ELF_OUT) $(BINARY) $(LD_SCRIPT) $(OBJECTS)
