#ifndef SOUND_PLAYER_H
#define SOUND_PLAYER_H

#include <stdint.h>
#include <gb/gb.h>

// The music and sound effect player (ROM bank 4, with the music); see Sound.h
// for how the game uses it.

#define SOUND_NO_REQUEST 0xFF
#define SOUND_STOP       0xFE
#define SOUND_SFX_QUEUE  4 // a power of 2

// Requests, written by the game (Sound.c) and taken by the player
extern volatile uint8_t soundSongRequest, soundSongQueued, soundSongPlaying;
extern volatile uint8_t soundSfxQueue[SOUND_SFX_QUEUE];
extern volatile uint8_t soundSfxHead, soundSfxTail;

void soundPlayerInit(void) BANKED;
// One frame: take requests, then play music and sound effects (from vblank)
void soundPlayerTick(void) BANKED;

#endif
