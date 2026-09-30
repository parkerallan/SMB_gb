#ifndef PIPES_H
#define PIPES_H

#include <stdint.h>
#include <gb/gb.h>

// Pipes Mario can go through, like SMB's: Down on a pipe that leads somewhere
// (the coin rooms), Right into a sideways pipe (out of a coin room, into 1-2
// from its intro, and 1-2's way out), rising out of another pipe or dropping
// in from above at the other end

// Nobody in a pipe (level start)
void pipesReset(void) BANKED;

// Normal play: is Mario (given the joypad) going into a pipe? That starts it.
void pipesCheck(uint8_t input) BANKED;

// Going through a pipe: pipesUpdate() moves Mario and changes the area; the
// rest of the game waits meanwhile
uint8_t pipesActive(void) BANKED;
void pipesUpdate(void) BANKED;

#endif
