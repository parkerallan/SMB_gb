#ifndef FLAGPOLE_H
#define FLAGPOLE_H

#include <gb/gb.h>

// The end of the level, like SMB: grabbing the flagpole (points by height),
// sliding down with the flag, walking into the castle, the time bonus, the
// star flag rising over the castle and sometimes fireworks

// Load the flag's sprite tiles
void flagpoleInit(void) BANKED;

// Find the flagpole in the level (call after levelLoad)
void flagpoleReset(void) BANKED;

// Normal play: did Mario just grab the pole? That starts the ending.
void flagpoleCheck(void) BANKED;

// Is the ending running? Then it controls Mario, and flagpoleUpdate() runs it
// one frame at a time, returning 1 when it's over.
uint8_t flagpoleEnding(void) BANKED;
uint8_t flagpoleUpdate(void) BANKED;

// Draw the flags (and fireworks) relative to the camera
void flagpoleDraw(void) BANKED;

#endif
