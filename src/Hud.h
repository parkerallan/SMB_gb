#ifndef HUD_H
#define HUD_H

#include <stdint.h>

// Height of the HUD strip at the top of the screen, in pixels
#define HUD_PIXEL_HEIGHT 16

void hudInit(void);
void hudUpdate(uint16_t score, int8_t lives, int8_t world, int8_t level, uint16_t time);
void hudUpdateTime(uint16_t time);

#endif
