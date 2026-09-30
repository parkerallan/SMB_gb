#ifndef ENEMIES_H
#define ENEMIES_H

#include <stdint.h>
#include <gb/gb.h>

// Goombas, Koopa Troopas and Piranha Plants, placed from the level's enemy list
// (LevelEnemies), which also has its lifts (see Lifts.h)
// and brought to life as they scroll into view

// Load their sprite tiles
void enemiesInit(void) BANKED;

// Level (re)start: everyone back at their starting spots, waiting to scroll into view
void enemiesReset(void) BANKED;

// Spawn, move, and sort out collisions with Mario and each other
void enemiesUpdate(void) BANKED;
void enemiesDraw(void) BANKED;

// Mario bumped the block at block (bx, by) from below: knock out whatever stands on it
void enemiesBumpBlock(int16_t bx, int16_t by) BANKED;

// A fireball at (x, y) (8x8, its top-left): knocks out the first enemy it
// touches; returns 1 if it hit one
uint8_t enemiesFireballHit(int16_t x, int16_t y) BANKED;

#endif
