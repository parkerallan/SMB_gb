#include <gb/gb.h>
#include "Util.h"
#include "Font.h"
#include "FontTiles.h"

static const char fontChars[] = FONT_CHARS;

void fontLoad(void) {
    bankedSetBkgData(FONT_FIRST_TILE, FontTilesCount, FontTiles, BANK(FontTiles));
}

uint8_t fontTile(char c) {
    uint8_t i;
    for (i = 0; fontChars[i]; i++) {
        if (fontChars[i] == c) return FONT_FIRST_TILE + i;
    }
    return FONT_BLANK_TILE;
}

void fontPrint(uint8_t x, uint8_t y, const char *text) {
    uint8_t tile;
    while (*text) {
        tile = fontTile(*text++);
        set_bkg_tile_xy(x++, y, tile);
    }
}
