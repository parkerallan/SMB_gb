#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include "GameCharacter.h"

extern struct GameCharacter mario;

// Load Mario's sprite tiles
void playerInit(void);

// Put Mario back at the level start
void playerReset(void);

// Move, jump and fall for one frame based on the joypad state
void playerUpdate(uint8_t input);

// Draw Mario relative to the camera
void playerDraw(void);

// Has Mario fallen out of the bottom of the level?
uint8_t playerFellOut(void);

#endif
