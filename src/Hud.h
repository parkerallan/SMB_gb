#ifndef HUD_H
#define HUD_H

#include <stdint.h>

// Height of the HUD strip at the top of the screen, in pixels
#define HUD_PIXEL_HEIGHT 16

void hudInit(void);
void hudUpdate(uint32_t score, uint8_t coins, int8_t lives, int8_t world, int8_t level, uint16_t time);
void hudUpdateScore(uint32_t score, uint8_t coins);
void hudUpdateTime(uint16_t time);

// Background palette for everything below the HUD (the HUD keeps its own)
void hudSetLevelPalette(uint8_t palette);

#endif
