GBDK_PATH = D:/Emulation/gbdk
EMULATOR = D:/Emulation/GBA/mGBA/mGBA.exe

CC = $(GBDK_PATH)/bin/lcc
CFLAGS = -Wa-l -Wl-m -Wl-j -DUSE_SFR_FOR_REG
INCLUDES = -Isrc -Iassets/sprites -Iassets/tiles -Iassets/maps

SRC = $(wildcard src/*.c assets/*/*.c)
HEADERS = $(wildcard src/*.h assets/*/*.h)

OUTPUT = build/main.gb

all: $(OUTPUT)

$(OUTPUT): $(SRC) $(HEADERS)
	mkdir -p build
	$(CC) $(CFLAGS) $(INCLUDES) -o $(OUTPUT) $(SRC)
	rm -f *.lst *.o *.ihx *.cdb *.adb *.asm *.sym

run: $(OUTPUT)
	"$(EMULATOR)" $(OUTPUT) &

clean:
	rm -rf build
	rm -f *.lst *.o *.ihx *.cdb *.adb *.asm *.sym

.PHONY: all run clean
