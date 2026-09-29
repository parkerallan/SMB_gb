#ifndef ENEMIES_H
#define ENEMIES_H

#include <stdint.h>
#include <gb/gb.h>

// Goombas and Koopa Troopas, placed from the level's enemy list (Level1_1Enemies)
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

#endif
