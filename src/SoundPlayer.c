// The sound player lives with the music: ROM bank 4 (see make.bat)
#pragma bank 4

#include <gb/gb.h>
#include <gb/hardware.h>
#include "SoundPlayer.h"
#include "Sound.h"
#include "Music.h"

// Channels, in MusicPart order: SMB's square 1 and 2, triangle and noise
#define PULSE1 0
#define PULSE2 1   // plays the lead: when it runs out, the part is over
#define WAVE   2
#define NOISE  3
#define CHANNELS 4

#define NO_SONG 0xFF
#define DUTY_50 0x80
#define DUTY_25 0x40
#define WAVE_FULL_VOLUME 0x20

// ---- Sound effects (Super Mario Bros.' are made by code, not data, so these
// are written to sound like them). Each is: channel, duty (NRx1), envelope
// (NRx2: start volume, fade speed), then steps of (frames, frequency low, high)
// and a 0. STEP restarts the sound at a frequency, SLIDE just changes the
// frequency (a smooth slide), REST goes quiet. Noise steps give NR43 (the
// noise's pitch and kind) instead of a frequency.
#define HZ(f) (2048 - 131072 / (f))
#define STEP(frames, f)  (frames), (uint8_t)(HZ(f) & 0xFF), (uint8_t)((HZ(f) >> 8) | 0x80)
#define SLIDE(frames, f) (frames), (uint8_t)(HZ(f) & 0xFF), (uint8_t)(HZ(f) >> 8)
#define REST(frames)     (frames), 0, 0
#define HIT(frames, poly) (frames), (poly), 0x80

static const uint8_t sfxJump[] = {PULSE1, DUTY_50, 0xC3,
    STEP(2, 330), SLIDE(2, 370), SLIDE(2, 415), SLIDE(2, 466), SLIDE(2, 523), SLIDE(2, 587),
    SLIDE(2, 659), SLIDE(2, 740), SLIDE(2, 831), SLIDE(3, 932), SLIDE(4, 988), 0};
static const uint8_t sfxJumpBig[] = {PULSE1, DUTY_50, 0xC3,
    STEP(2, 220), SLIDE(2, 247), SLIDE(2, 277), SLIDE(2, 311), SLIDE(2, 349), SLIDE(2, 392),
    SLIDE(2, 440), SLIDE(2, 494), SLIDE(2, 554), SLIDE(3, 622), SLIDE(4, 659), 0};
static const uint8_t sfxBump[] = {PULSE1, DUTY_25, 0xF1,
    STEP(3, 196), STEP(3, 165), STEP(5, 131), 0};
static const uint8_t sfxStomp[] = {PULSE1, DUTY_50, 0xF1,
    STEP(1, 1047), SLIDE(1, 880), SLIDE(1, 740), SLIDE(1, 622), SLIDE(1, 523), SLIDE(1, 440), SLIDE(3, 370), 0};
static const uint8_t sfxKick[] = {PULSE1, DUTY_50, 0xE1,
    STEP(2, 1568), SLIDE(2, 1175), SLIDE(3, 988), 0};
static const uint8_t sfxFireball[] = {PULSE1, DUTY_25, 0xB1,
    STEP(1, 2093), SLIDE(1, 1760), SLIDE(1, 1480), SLIDE(2, 1245), 0};
static const uint8_t sfxPipe[] = {PULSE1, DUTY_50, 0xF0,
    STEP(4, 147), REST(4), STEP(4, 139), REST(4), STEP(4, 131), REST(4), STEP(4, 123), REST(4),
    STEP(4, 147), REST(4), STEP(4, 139), REST(4), 0};
static const uint8_t sfxFlagpole[] = {PULSE1, DUTY_50, 0xD0,
    STEP(4, 1047), SLIDE(4, 988), SLIDE(4, 932), SLIDE(4, 880), SLIDE(4, 831), SLIDE(4, 784),
    SLIDE(4, 740), SLIDE(4, 698), SLIDE(4, 659), SLIDE(4, 622), SLIDE(4, 587), SLIDE(4, 554),
    SLIDE(4, 523), SLIDE(4, 494), SLIDE(4, 466), SLIDE(4, 440), SLIDE(4, 415), SLIDE(4, 392), 0};
static const uint8_t sfxPowerupAppear[] = {PULSE2, DUTY_50, 0xB2,
    STEP(2, 392), SLIDE(2, 587), SLIDE(2, 784), STEP(2, 440), SLIDE(2, 659), SLIDE(2, 880),
    STEP(2, 494), SLIDE(2, 740), SLIDE(2, 988), STEP(2, 523), SLIDE(2, 784), SLIDE(4, 1047), 0};
static const uint8_t sfxPowerup[] = {PULSE2, DUTY_50, 0xC1,
    STEP(2, 262), STEP(2, 392), STEP(2, 523), STEP(2, 294), STEP(2, 440), STEP(2, 587),
    STEP(2, 330), STEP(2, 494), STEP(2, 659), STEP(2, 370), STEP(2, 554), STEP(2, 740),
    STEP(2, 415), STEP(2, 622), STEP(2, 831), STEP(2, 466), STEP(2, 698), STEP(4, 932), 0};
static const uint8_t sfxCoin[] = {PULSE2, DUTY_50, 0xD3,
    STEP(4, 988), STEP(24, 1319), 0};
static const uint8_t sfxOneUp[] = {PULSE2, DUTY_50, 0xC2,
    STEP(6, 659), STEP(6, 784), STEP(6, 1319), STEP(6, 1047), STEP(6, 1175), STEP(10, 1568), 0};
static const uint8_t sfxTimeTick[] = {PULSE2, DUTY_50, 0x81,
    STEP(3, 1760), 0};
static const uint8_t sfxBrick[] = {NOISE, 0, 0xF2,
    HIT(4, 0x55), HIT(4, 0x56), HIT(6, 0x57), HIT(10, 0x67), 0};
static const uint8_t sfxFirework[] = {NOISE, 0, 0xF4,
    HIT(4, 0x44), HIT(24, 0x66), 0};

static const uint8_t * const sfxData[SFX_COUNT] = {
    sfxJump, sfxJumpBig, sfxBump, sfxStomp, sfxKick, sfxFireball, sfxPipe, sfxFlagpole,
    sfxPowerupAppear, sfxPowerup, sfxCoin, sfxOneUp, sfxTimeTick, sfxBrick, sfxFirework,
};

// A triangle wave for the wave channel (SMB's triangle), 32 4-bit steps
static const uint8_t triangleWave[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10,
};

// SMB's drum kinds 1-3 (short, strong, long): NR41 (length), NR42, NR43
static const uint8_t drums[] = {
    60, 0xA0, 0x21,
    58, 0xC0, 0x55,
    40, 0xA0, 0x21,
};

// ---- Music state
static uint8_t song = NO_SONG;
static const uint8_t *partList;
static uint8_t partCount, partNext, loopTo, voice;
static const uint8_t *stream[CHANNELS];
static uint8_t counter[CHANNELS];
// ---- Sound effect state (a channel with an effect playing skips the music's notes)
static const uint8_t *sfxStep[CHANNELS];
static uint8_t sfxCounter[CHANNELS];
static uint8_t sfxPlaying; // how many channels have one
static uint8_t sfxDuty[CHANNELS], sfxEnvelope[CHANNELS];

static void silence(uint8_t c) {
    // turning a channel's DAC off (volume and envelope 0) silences it at once
    if (c == PULSE1)      NR12_REG = 0;
    else if (c == PULSE2) NR22_REG = 0;
    else if (c == WAVE)   NR30_REG = 0;
    else                  NR42_REG = 0;
}

// Starting a note, one function per channel. The player runs in the vblank
// interrupt, which must be done well before the HUD's split at line 15, so
// these avoid looking anything up by channel number (slow in SDCC's code).
static void playPulse1(uint8_t note) {
    const uint8_t *f;
    if (!note) {
        NR12_REG = 0;
        return;
    }
    f = (const uint8_t *)(MusicFreqs + note);
    NR10_REG = 0;
    NR11_REG = DUTY_50;
    NR12_REG = voice;
    NR13_REG = f[0];
    NR14_REG = 0x80 | f[1];
}

static void playPulse2(uint8_t note) {
    const uint8_t *f;
    if (!note) {
        NR22_REG = 0;
        return;
    }
    f = (const uint8_t *)(MusicFreqs + note);
    NR21_REG = DUTY_50;
    NR22_REG = voice;
    NR23_REG = f[0];
    NR24_REG = 0x80 | f[1];
}

static void playWave(uint8_t note) {
    const uint8_t *f;
    // off first, then on again before restarting: restarting a playing wave
    // channel can garble its wave on the original Game Boy
    NR30_REG = 0;
    if (!note) return;
    f = (const uint8_t *)(MusicFreqs + note);
    NR30_REG = 0x80;
    NR32_REG = WAVE_FULL_VOLUME;
    NR33_REG = f[0];
    NR34_REG = 0x80 | f[1];
}

static void playNoise(uint8_t note) {
    const uint8_t *d;
    if (!note) {
        NR42_REG = 0;
        return;
    }
    d = drums + (note - 1) * 3;
    NR41_REG = d[0];
    NR42_REG = d[1];
    NR43_REG = d[2];
    NR44_REG = 0xC0; // start, and stop after the length
}

// A channel's note is over: the next one (or the end of its stream). Most
// frames none are, so each channel's check is written out with its fixed index.
#define CHANNEL_TICK(c, play)                    \
    if (stream[c] && !--counter[c]) {            \
        const uint8_t *s = stream[c];            \
        if (!s[0]) {                             \
            stream[c] = 0;                       \
        } else {                                 \
            counter[c] = s[0];                   \
            stream[c] = s + 2;                   \
            if (!sfxStep[c]) play(s[1]);         \
        }                                        \
    }

static void startPart(uint8_t i) {
    const MusicPart *part = &MusicParts[partList[i]];
    uint8_t c;
    for (c = 0; c < CHANNELS; c++) {
        stream[c] = part->channel[c];
        counter[c] = 1;
        if (!stream[c] && !sfxStep[c]) silence(c);
    }
    partNext = i + 1;
}

static void startSong(uint8_t s) {
    const MusicSong *sg = &MusicSongs[s];
    song = s;
    partList = sg->parts;
    partCount = sg->count;
    loopTo = sg->loop;
    voice = sg->voice;
    startPart(0);
    soundSongPlaying = 1;
}

static void stopSong(void) {
    uint8_t c;
    song = NO_SONG;
    for (c = 0; c < CHANNELS; c++) {
        stream[c] = 0;
        if (!sfxStep[c]) silence(c);
    }
    soundSongPlaying = 0;
}

static void musicTick(void) {
    uint8_t n;
    if (song == NO_SONG) return;
    for (;;) {
        CHANNEL_TICK(PULSE1, playPulse1)
        CHANNEL_TICK(PULSE2, playPulse2)
        CHANNEL_TICK(WAVE, playWave)
        CHANNEL_TICK(NOISE, playNoise)
        if (stream[PULSE2]) return; // the lead's still going
        // the part's over: the next one, back to the loop point, or the end
        if (partNext < partCount) {
            startPart(partNext);
        } else if (loopTo != MUSIC_NO_LOOP) {
            startPart(loopTo);
        } else if (soundSongQueued != SOUND_NO_REQUEST) {
            n = soundSongQueued;
            soundSongQueued = SOUND_NO_REQUEST;
            startSong(n);
        } else {
            stopSong();
            return;
        }
    }
}

static void sfxStart(uint8_t id) {
    const uint8_t *d = sfxData[id];
    uint8_t c = d[0];
    if (!sfxStep[c]) sfxPlaying++;
    sfxDuty[c] = d[1];
    sfxEnvelope[c] = d[2];
    sfxStep[c] = d + 3;
    sfxCounter[c] = 1;
}

static void sfxApply(uint8_t c, uint8_t lo, uint8_t hi) {
    if (!lo && !hi) {
        silence(c);
        return;
    }
    if (c == PULSE1) {
        if (hi & 0x80) {
            NR10_REG = 0;
            NR11_REG = sfxDuty[c];
            NR12_REG = sfxEnvelope[c];
        }
        NR13_REG = lo;
        NR14_REG = hi;
    } else if (c == PULSE2) {
        if (hi & 0x80) {
            NR21_REG = sfxDuty[c];
            NR22_REG = sfxEnvelope[c];
        }
        NR23_REG = lo;
        NR24_REG = hi;
    } else if (c == NOISE) {
        NR41_REG = 0;
        NR42_REG = sfxEnvelope[c];
        NR43_REG = lo;
        NR44_REG = hi;
    }
}

// An effect's step is over: the next one, or the end
static void sfxNext(uint8_t c) {
    const uint8_t *s = sfxStep[c];
    if (!s[0]) {
        // done: quiet until the music's next note on this channel
        sfxStep[c] = 0;
        sfxPlaying--;
        silence(c);
        return;
    }
    sfxCounter[c] = s[0];
    sfxStep[c] = s + 3;
    sfxApply(c, s[1], s[2]);
}

#define EFFECT_TICK(c) if (sfxStep[c] && !--sfxCounter[c]) sfxNext(c)

static void sfxTick(void) {
    if (!sfxPlaying) return;
    EFFECT_TICK(PULSE1);
    EFFECT_TICK(PULSE2);
    EFFECT_TICK(NOISE);
}

void soundPlayerInit(void) BANKED {
    uint8_t i;
    volatile uint8_t *waveRam = (volatile uint8_t *)0xFF30;
    NR10_REG = 0;
    NR30_REG = 0; // the wave channel must be off to load its wave
    for (i = 0; i < sizeof(triangleWave); i++) waveRam[i] = triangleWave[i];
    for (i = 0; i < CHANNELS; i++) {
        stream[i] = 0;
        sfxStep[i] = 0;
        silence(i);
    }
    sfxPlaying = 0;
    song = NO_SONG;
}

void soundPlayerTick(void) BANKED {
    uint8_t r = soundSongRequest;
    if (r != SOUND_NO_REQUEST) {
        soundSongRequest = SOUND_NO_REQUEST;
        if (r == SOUND_STOP) stopSong();
        else                 startSong(r);
    }
    while (soundSfxTail != soundSfxHead) {
        sfxStart(soundSfxQueue[soundSfxTail]);
        soundSfxTail = (soundSfxTail + 1) & (SOUND_SFX_QUEUE - 1);
    }
    musicTick();
    sfxTick();
}
