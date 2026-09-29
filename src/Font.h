#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include "FontTiles.h"

// The Super Mario Bros. font (assets/tiles/FontTiles), shared by the HUD and
// the text screens. Letters A-Z, digits, '-', '!', 'x' (times sign), '@' (coin).
// It sits at the very end of the background tiles (up to 255), above the
// scenery tiles the level loads from 0 up.
#define FONT_FIRST_TILE ((uint8_t)(256 - FontTilesCount))
#define FONT_BLANK_TILE FONT_FIRST_TILE

// Copy the font into background tile memory
void fontLoad(void);

// Background tile for a character (unknown characters are blank)
uint8_t fontTile(char c);

// Write text to the background map at tile (x, y)
void fontPrint(uint8_t x, uint8_t y, const char *text);

#endif
