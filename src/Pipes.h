#ifndef PIPES_H
#define PIPES_H

#include <stdint.h>
#include <gb/gb.h>

// Pipes Mario can go through, like SMB's 1-1: Down on the pipe that leads to
// the underground coin room, and Right into the room's sideways pipe, which
// brings him back up out of a pipe near the end of 1-1

// Nobody in a pipe (level start)
void pipesReset(void) BANKED;

// Normal play: is Mario (given the joypad) going into a pipe? That starts it.
void pipesCheck(uint8_t input) BANKED;

// Going through a pipe: pipesUpdate() moves Mario and changes the area; the
// rest of the game waits meanwhile
uint8_t pipesActive(void) BANKED;
void pipesUpdate(void) BANKED;

#endif
