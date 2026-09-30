#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>
#include "Music.h"

// Music (Super Mario Bros.' own, see assets/sound/Music.h) and sound effects.
// The player runs from the vblank interrupt, so music keeps time even when the
// game is slow; these calls just leave it a request for its next run.

// Sound effects. Like SMB, each one borrows a music channel while it plays.
#define SFX_JUMP           0
#define SFX_JUMP_BIG       1
#define SFX_BUMP           2
#define SFX_STOMP          3
#define SFX_KICK           4
#define SFX_FIREBALL       5
#define SFX_PIPE           6   // going through a pipe, and getting hurt
#define SFX_FLAGPOLE       7
#define SFX_POWERUP_APPEAR 8
#define SFX_POWERUP        9
#define SFX_COIN           10
#define SFX_ONE_UP         11
#define SFX_TICK           12  // the time bonus counting
#define SFX_BRICK          13
#define SFX_FIREWORK       14
#define SFX_COUNT          15

// Turn the sound on and hook the player to vblank (once, at power on)
void soundInit(void);

// Start a song (MUSIC_*) from its beginning; it replaces whatever's playing
void musicPlay(uint8_t song);
// Start this song when the current one (one that doesn't loop) finishes
void musicQueue(uint8_t song);
void musicStop(void);
// Is a song still playing? (to wait for a jingle to finish)
uint8_t musicPlaying(void);

void sfxPlay(uint8_t sfx);

#endif
