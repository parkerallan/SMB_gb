#include <gb/gb.h>
#include "Sprites.h"
#include "Camera.h"
#include "Hud.h"

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144

static uint8_t next;
static uint8_t usedLastFrame = MAX_HARDWARE_SPRITES;

// Level position minus these gives a number that is small only when the thing
// is visible, so each axis is checked with one unsigned compare:
//   x - originX = screen x + 15, visible (at least partly) when below VISIBLE_X
//   y - originY = screen y - HUD, visible (below the HUD) when below VISIBLE_Y
#define VISIBLE_X (SCREEN_WIDTH + 15)
#define VISIBLE_Y (SCREEN_HEIGHT - HUD_PIXEL_HEIGHT)
static int16_t originX, originY;

void spritesBegin(void) {
    next = 0;
    originX = cameraX - 15;
    originY = cameraY + HUD_PIXEL_HEIGHT;
}

void spritesEnd(void) {
    // only hide what was showing last frame and isn't now
    if (next < usedLastFrame) hide_sprites_range(next, usedLastFrame);
    usedLastFrame = next;
}

uint8_t spritesNext(void) {
    return next;
}

void spritesAdvance(uint8_t count) {
    next += count;
}

// These write straight into GBDK's sprite table with one position calculation
// and 8-bit math. (GBDK's metasprite routines, or drawing tile by tile, were
// several times slower and made the game lag with a screen full of enemies.)
// A thing partly off the left edge gets x values that wrap to 240+, which the
// hardware doesn't show.
void spriteDraw16(uint8_t first, uint8_t props, int16_t x, int16_t y) {
    uint16_t u = x - originX, v = y - originY;
    uint8_t left, right, top, bottom;
    uint8_t *o;
    if (u >= VISIBLE_X || v >= VISIBLE_Y || next > MAX_HARDWARE_SPRITES - 4) return;
    left = (uint8_t)u + (SPRITE_OFFSET_X - 15);
    right = left + 8;
    top = (uint8_t)v + (HUD_PIXEL_HEIGHT + SPRITE_OFFSET_Y);
    bottom = top + 8;
    // tiles are TL, BL, TR, BR; mirroring swaps the columns or rows
    if (props & S_FLIPX) { right = left; left += 8; }
    if (props & S_FLIPY) { bottom = top; top += 8; }
    o = (uint8_t *)&shadow_OAM[next];
    next += 4;
    *o++ = top;    *o++ = left;  *o++ = first++; *o++ = props;
    *o++ = bottom; *o++ = left;  *o++ = first++; *o++ = props;
    *o++ = top;    *o++ = right; *o++ = first++; *o++ = props;
    *o++ = bottom; *o++ = right; *o++ = first;   *o = props;
}

void spriteDrawKoopa(uint8_t first, uint8_t props, int16_t x, int16_t y) {
    uint16_t u = x - originX, v = y - originY;
    uint8_t left, right, top;
    uint8_t *o;
    if (u >= VISIBLE_X || v >= VISIBLE_Y || next > MAX_HARDWARE_SPRITES - 5) return;
    left = (uint8_t)u + (SPRITE_OFFSET_X - 15);
    right = left + 8;
    top = (uint8_t)v + (HUD_PIXEL_HEIGHT + SPRITE_OFFSET_Y);
    if (props & S_FLIPX) { right = left; left += 8; }
    o = (uint8_t *)&shadow_OAM[next];
    next += 5;
    // tiles row by row: TL (blank, skipped), TR, then two full rows
    first++;
    *o++ = top; *o++ = right; *o++ = first++; *o++ = props;
    top += 8;
    *o++ = top; *o++ = left;  *o++ = first++; *o++ = props;
    *o++ = top; *o++ = right; *o++ = first++; *o++ = props;
    top += 8;
    *o++ = top; *o++ = left;  *o++ = first++; *o++ = props;
    *o++ = top; *o++ = right; *o++ = first;   *o = props;
}

void spriteDraw(uint8_t tile, uint8_t props, int16_t x, int16_t y) {
    int16_t sx = x - cameraX + SPRITE_OFFSET_X;
    int16_t sy = y - cameraY + SPRITE_OFFSET_Y;
    if (next >= MAX_HARDWARE_SPRITES) return;
    if (sx <= 0 || sx >= SCREEN_WIDTH + SPRITE_OFFSET_X) return;
    if (sy < HUD_PIXEL_HEIGHT + SPRITE_OFFSET_Y || sy >= SCREEN_HEIGHT + SPRITE_OFFSET_Y) return;
    set_sprite_tile(next, tile);
    set_sprite_prop(next, props);
    move_sprite(next, (uint8_t)sx, (uint8_t)sy);
    next++;
}
