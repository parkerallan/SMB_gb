GBDK_PATH = d:/gbdk

# Flags and path to compiler
CC = $(GBDK_PATH)/bin/lcc
CFLAGS = -Wa-l -Wl-m -Wl-j -DUSE_SFR_FOR_REG

SRC = main.c Mario.c
OBJ = $(SRC:.c=.o)

OUTPUT = main.gb

all: $(OUTPUT)

# Compiling and Linking
$(OUTPUT): $(OBJ)
	$(CC) $(CFLAGS) -o main.gb main.c Mario.c Metasprites.c WorldTiles.c SmallMap.c TitleMap.c

# Clean target
clean:
	rm -f $(OBJ) $(OUTPUT)
	rm -f *.lst *.map *.sym *.ihx *.lnk *.asm *.noi

# Phony targets
.PHONY: all clean