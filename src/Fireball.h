#ifndef FIREBALL_H
#define FIREBALL_H

#include <stdint.h>
#include <gb/gb.h>

// Fire Mario's fireballs: they bounce along the ground, knock out the enemies
// they hit, and burst on walls

// Load their sprite tiles
void fireballsInit(void) BANKED;

// Remove them all (level restart)
void fireballsReset(void) BANKED;

// Throw one from (x, y), its top-left; returns 0 if two are already out
uint8_t fireballThrow(int16_t x, int16_t y, uint8_t left) BANKED;

// Move and hit things for one frame, and draw
void fireballsUpdate(void) BANKED;
void fireballsDraw(void) BANKED;

#endif
