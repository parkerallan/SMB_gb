#ifndef BLOCKS_H
#define BLOCKS_H

#include <stdint.h>

// ? blocks and bricks: bumping, emptying ? blocks, and the coins they give

// Load block and coin graphics
void blocksInit(void);

// Forget emptied blocks and clear effects (level restart)
void blocksReset(void);

// Mario's head hit the solid tile at (tx, ty)
void blocksHit(int16_t tx, int16_t ty);

// Animate bumps and coins, relative to the camera
void blocksUpdate(void);

// Swap emptied ? blocks into a level column as it streams into VRAM
void blocksPatchColumn(int16_t column, uint8_t *tiles);

#endif
