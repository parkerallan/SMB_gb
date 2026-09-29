#ifndef LEVEL_H
#define LEVEL_H

#include <stdint.h>
#include "Level1_1.h"

// Level size in tiles and pixels
#define LEVEL_COLUMNS Level1_1Width
#define LEVEL_ROWS Level1_1Height
#define LEVEL_WIDTH (LEVEL_COLUMNS * 8)
#define LEVEL_HEIGHT (LEVEL_ROWS * 8)

// Load the level's tiles and fill VRAM with the columns visible from cameraX
void levelLoad(int16_t cameraX);

// Copy any columns newly scrolled into view into VRAM
void levelStream(int16_t cameraX);

// Solidity queries in level coordinates
uint8_t levelTileSolid(int16_t tx, int16_t ty);
// Any solid tile in column tx between pixel rows top..bottom?
uint8_t levelColumnSolid(int16_t tx, int16_t top, int16_t bottom);
// Any solid tile in row ty between pixel columns left..right?
uint8_t levelRowSolid(int16_t ty, int16_t left, int16_t right);

#endif
