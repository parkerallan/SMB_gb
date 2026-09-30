#ifndef POWERUP_H
#define POWERUP_H

#include <stdint.h>
#include <gb/gb.h>

// The item from a power-up block: it rises out of the block, then the mushroom
// slides along, the flower stays put and the star hops along. Touching it
// makes small Mario big, big Mario fire Mario, or Mario invincible; the 1-up
// mushroom slides along like the mushroom and gives an extra life.
#define POWERUP_MUSHROOM 0
#define POWERUP_FLOWER   1
#define POWERUP_STAR     2
#define POWERUP_ONE_UP   3

// Load its sprite tiles
void powerupInit(void) BANKED;

// Remove it (level restart)
void powerupReset(void) BANKED;

// A block at tile (tx, ty) (its top-left) was hit and gives a POWERUP_* item
void powerupSpawn(int16_t tx, int16_t ty, uint8_t what) BANKED;

// Move and collect for one frame, and draw
void powerupUpdate(void) BANKED;
void powerupDraw(void) BANKED;

#endif
