GBDK_PATH = D:/Emulation/gbdk
EMULATOR = D:/Emulation/GBA/mGBA/mGBA.exe

CC = $(GBDK_PATH)/bin/lcc
# MBC1 cartridge, 8 ROM banks (128KB): core code in bank 0, maps in bank 1, tiles in bank 2,
# level-object code in bank 3, music and the sound player in bank 4 (5-7 are free)
CFLAGS = -Wa-l -Wl-m -Wl-j -DUSE_SFR_FOR_REG -Wl-yt1 -Wl-yo8
INCLUDES = -Isrc -Iassets/sprites -Iassets/tiles -Iassets/maps -Iassets/sound

SRC = $(wildcard src/*.c assets/*/*.c)
HEADERS = $(wildcard src/*.h assets/*/*.h)

OUTPUT = build/main.gb

all: $(OUTPUT)

$(OUTPUT): $(SRC) $(HEADERS)
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) -o $(OUTPUT) $(SRC)
	$(GBDK_PATH)/bin/romusage build/main.map -q -R
	rm -f *.lst *.o *.ihx *.cdb *.adb *.asm *.sym

run: $(OUTPUT)
	"$(EMULATOR)" $(OUTPUT) &

clean:
	rm -rf build
	rm -f *.lst *.o *.ihx *.cdb *.adb *.asm *.sym

.PHONY: all run clean
