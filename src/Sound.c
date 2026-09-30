#include <gb/gb.h>
#include "Sound.h"
#include "SoundPlayer.h"

// Requests for the player. The game only writes these (single bytes) and the
// interrupt only takes them, so the two never modify the player's state at once.
volatile uint8_t soundSongRequest = SOUND_NO_REQUEST;
volatile uint8_t soundSongQueued = SOUND_NO_REQUEST;
volatile uint8_t soundSongPlaying; // set by the player
volatile uint8_t soundSfxQueue[SOUND_SFX_QUEUE];
volatile uint8_t soundSfxHead, soundSfxTail; // the game adds at head, the player takes from tail

static void soundInterrupt(void) {
    soundPlayerTick();
}

void soundInit(void) {
    NR52_REG = 0x80; // sound on
    NR50_REG = 0x77; // full volume, both speakers
    NR51_REG = 0xFF; // every channel to both speakers
    soundPlayerInit();
    CRITICAL {
        add_VBL(soundInterrupt);
    }
}

void musicPlay(uint8_t song) {
    soundSongQueued = SOUND_NO_REQUEST;
    soundSongRequest = song;
    soundSongPlaying = 1; // counts as playing from now (the player starts it next vblank)
}

void musicQueue(uint8_t song) {
    soundSongQueued = song;
}

void musicStop(void) {
    soundSongQueued = SOUND_NO_REQUEST;
    soundSongRequest = SOUND_STOP;
    soundSongPlaying = 0;
}

uint8_t musicPlaying(void) {
    return soundSongPlaying;
}

void sfxPlay(uint8_t sfx) {
    uint8_t next = (soundSfxHead + 1) & (SOUND_SFX_QUEUE - 1);
    if (next == soundSfxTail) return; // full: drop it
    soundSfxQueue[soundSfxHead] = sfx;
    soundSfxHead = next;
}
