#ifndef LIFTS_H
#define LIFTS_H

#include <stdint.h>
#include <gb/gb.h>

// 1-2's lifts: platforms that keep going up (or down), coming back round from
// the other end of the level, and that Mario can ride

// How many the area has. Most have none: then the rest needn't be called (each
// call into this bank costs a little, every frame).
extern uint8_t liftCount;

// Load their sprite tile
void liftsInit(void) BANKED;

// None (area start); then the area's enemy list adds its lifts (see enemiesReset)
void liftsReset(void) BANKED;
void liftsAdd(int16_t x, int16_t y, uint8_t goingUp) BANKED;

// Every frame after Mario moves: did he land on a lift? `feetBefore` is where
// his feet were before the move, or LIFTS_NOT_LANDING (he's jumping up, or on
// the ground). Puts him on it, and he rides it until the next call says otherwise.
#define LIFTS_NOT_LANDING 0x7FFF
uint8_t liftsLand(int16_t feetBefore) BANKED;

// Move the lifts (and Mario with his), and draw them
void liftsUpdate(void) BANKED;
void liftsDraw(void) BANKED;

#endif
