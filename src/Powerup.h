#ifndef POWERUP_H
#define POWERUP_H

#include <stdint.h>
#include <gb/gb.h>

// The mushroom from a power-up ? block: it rises out of the block, slides
// along, and makes small Mario big when he touches it

// Load its sprite tiles
void powerupInit(void) BANKED;

// Remove it (level restart)
void powerupReset(void) BANKED;

// A power-up block at tile (tx, ty) (its top-left) was hit
void powerupSpawn(int16_t tx, int16_t ty) BANKED;

// Move and collect for one frame, and draw
void powerupUpdate(void) BANKED;
void powerupDraw(void) BANKED;

#endif
