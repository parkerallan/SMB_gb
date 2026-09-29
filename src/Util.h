#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

// Wait for this many frames (vblanks)
void waitFrames(uint8_t frames);

// Reading data that lives in a switchable ROM bank: these switch to `bank`,
// copy, and switch back to whatever was there. They live in bank 0, so code in
// any bank (including banked code) can call them.
void bankedMemcpy(void *dst, const void *src, uint16_t length, uint8_t bank);
void bankedSetSpriteData(uint8_t first, uint8_t count, const uint8_t *tiles, uint8_t bank);
void bankedSetBkgData(uint8_t first, uint8_t count, const uint8_t *tiles, uint8_t bank);
void bankedSetBkgTiles(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *map, uint8_t bank);

#endif
