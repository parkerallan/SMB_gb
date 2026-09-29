#ifndef BLOCKS_H
#define BLOCKS_H

#include <stdint.h>
#include <gb/gb.h>

// ? blocks and bricks: bumping, emptying ? blocks (coin or mushroom), and big
// Mario breaking bricks

// Load block and coin graphics
void blocksInit(void) BANKED;

// Clear effects (level restart; the level itself refills the blocks)
void blocksReset(void) BANKED;

// Mario's head hit the solid tile at (tx, ty)
void blocksHit(int16_t tx, int16_t ty) BANKED;

// Animate bumps, coins and brick pieces
void blocksUpdate(void) BANKED;
void blocksDraw(void) BANKED;

#endif
