#ifndef FONT_H
#define FONT_H

#include <stdint.h>

// Super Mario Bros style font shared by the HUD and the text screens.
// Tiles 128+ live at 0x8800, clear of the level tiles (0-122) and Mario's
// sprite tiles (0x8000-0x813F).
#define FONT_FIRST_TILE 128
#define FONT_BLANK_TILE FONT_FIRST_TILE

// Copy the font into background tile memory
void fontLoad(void);

// Background tile for a character (unknown characters are blank)
uint8_t fontTile(char c);

// Write text to the background map at tile (x, y)
void fontPrint(uint8_t x, uint8_t y, const char *text);

#endif
