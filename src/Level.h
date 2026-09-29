#ifndef LEVEL_H
#define LEVEL_H

#include <stdint.h>
#include "Level1_1.h"
#include "SceneryTiles.h"

// The map is a grid of 16x16 blocks (SCENERY_* numbers); positions below are
// in 8x8 tiles or pixels unless they say "block"
#define LEVEL_COLUMNS (Level1_1Width * 2)
#define LEVEL_ROWS (Level1_1Height * 2)
#define LEVEL_WIDTH (LEVEL_COLUMNS * 8)
#define LEVEL_HEIGHT (LEVEL_ROWS * 8)

// Build the block tables (once at power on)
void levelInit(void);

// Load the level's map and tiles, and fill VRAM with the columns visible from cameraX
void levelLoad(int16_t cameraX);

// Copy any columns newly scrolled into view into VRAM
void levelStream(int16_t cameraX);

// Solidity queries in level tile coordinates
uint8_t levelTileSolid(int16_t tx, int16_t ty);
// Any solid tile in column tx between pixel rows top..bottom?
uint8_t levelColumnSolid(int16_t tx, int16_t top, int16_t bottom);
// Any solid tile in row ty between pixel columns left..right?
uint8_t levelRowSolid(int16_t ty, int16_t left, int16_t right);

// Block (SCENERY_*) at block coordinates, SCENERY_BLANK outside the level
uint8_t levelBlockAt(int16_t bx, int16_t by);

// Change a block (e.g. a hit ? block becomes USED_BLOCK) and redraw it if on screen
void levelSetBlock(int16_t bx, int16_t by, uint8_t block);

// The 4 background tiles that draw a block: TL, TR, BL, BR
const uint8_t *levelBlockTiles(uint8_t block);

// Change the tile shown at (tx, ty) if that column is currently in VRAM
void levelSetTile(int16_t tx, int16_t ty, uint8_t tile);

#endif
