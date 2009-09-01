CC=nios2-elf-gcc
LD=nios2-elf-ld
CFLAGS=-c -nostdinc -W -Wall -G0 -O2 -mhw-div -I../ -DRANGECHECK -DFPM_DEFAULT -DNDEBUG

OBJECTS    = start.o madtimer.o malloc.o bit.o decoder.o fixed.o frame.o huffman.o layer3.o layer12.o mp3.o stream.o synth.o version.o

MEMORY_MAP = mp3.x
ELF_OUT    = mp3.elf
BINARY     = mp3.bin
LD_SCRIPT  = mp3.ld

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
	$(LD) -o $(ELF_OUT) $(OBJECTS) -T $(LD_SCRIPT) ../string.o \
	../graphic.o ../fat32.o ../timer.o ../menu.o \
	../utils.o ../jtag.o ../fonts.o \
	../sdcard.o -Map memorymap

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
