#ifndef FLAGPOLE_H
#define FLAGPOLE_H

// The flag on the level's flagpole (a sprite, like on the NES)

// Load the flag's sprite tiles
void flagpoleInit(void);

// Find the flagpole in the level (call after levelLoad)
void flagpoleReset(void);

// Draw the flag relative to the camera
void flagpoleDraw(void);

#endif
