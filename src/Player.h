#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include "GameCharacter.h"

extern struct GameCharacter mario;

// Load Mario's sprite tiles (small, big and dead)
void playerInit(void);

// Put Mario back at the level start (keeping his size and fire power)
void playerReset(void);

// Move, jump and fall for one frame based on the joypad state. While Mario is
// changing size or dying, this only runs that animation.
void playerUpdate(uint8_t input);

// Draw Mario relative to the camera
void playerDraw(void);

// Has Mario fallen out of the bottom of the level?
uint8_t playerFellOut(void);

// Mushroom: small Mario grows (the game pauses while he flashes)
void playerGrow(void);
uint8_t playerIsBig(void);

// Fire flower: big Mario becomes fire Mario (the game pauses while he flashes)
// and throws fireballs with B; small Mario just grows
void playerFire(void);

// Star: invincible for a while, knocking out any enemy he touches
void playerStar(void);
uint8_t playerHasStar(void);

// Growing or shrinking: the rest of the game pauses meanwhile
uint8_t playerIsChangingSize(void);

// An enemy got Mario: big Mario shrinks and is briefly invincible, small Mario dies
void playerHurt(void);
uint8_t playerIsInvincible(void);

// Death animation (hurt while small, or time ran out); the game pauses meanwhile
void playerDie(void);
uint8_t playerIsDying(void);
uint8_t playerDeathFinished(void);

// The level end takes over Mario: holding the flagpole (the climbing frame,
// not moving), hidden inside the castle, or back to normal
#define POSE_NORMAL 0
#define POSE_POLE   1
#define POSE_HIDDEN 2
void playerSetPose(uint8_t pose);
void playerFaceLeft(uint8_t left);

// Stomping an enemy: little hop off it
void playerBounce(void);
uint8_t playerIsFalling(void);
uint8_t playerOnGround(void);

#endif
