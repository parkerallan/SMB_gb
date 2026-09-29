#ifndef FLAGPOLE_H
#define FLAGPOLE_H

#include <gb/gb.h>

// The flag on the level's flagpole (a sprite, like on the NES)

// Load the flag's sprite tiles
void flagpoleInit(void) BANKED;

// Find the flagpole in the level (call after levelLoad)
void flagpoleReset(void) BANKED;

// Draw the flag relative to the camera
void flagpoleDraw(void) BANKED;

#endif
