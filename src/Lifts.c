// Level objects are banked code: ROM bank 3 (see make.bat)
#pragma bank 3

#include <gb/gb.h>
#include "Util.h"
#include "Lifts.h"
#include "Level.h"
#include "Player.h"
#include "ItemTiles.h"
#include "Sprites.h"

#define MAX_LIFTS 4
#define LIFT_SEGMENTS 6            // 8-pixel pieces: 48 pixels wide
#define LIFT_WIDTH (LIFT_SEGMENTS * 8)
#define LIFT_TOP_WRAP (-16)        // off the top of the level: back in at the bottom...
#define LIFT_BOTTOM_WRAP LEVEL_HEIGHT // ...and the other way round

// One array per field, walked with pointers: an array of structs costs a
// multiply per access (the Game Boy has to do those in software)
static int16_t liftX[MAX_LIFTS], liftY[MAX_LIFTS];
static int8_t liftDy[MAX_LIFTS];   // -1 going up, 1 going down (a pixel a frame)
uint8_t liftCount;
static uint8_t riding;             // Mario's lift + 1, or 0

void liftsInit(void) BANKED {
    bankedSetSpriteData(SPR_TILE_LIFT, 1, ItemTiles + ITEMTILES_PLATFORM * 16, BANK(ItemTiles));
}

void liftsReset(void) BANKED {
    liftCount = 0;
    riding = 0;
}

void liftsAdd(int16_t x, int16_t y, uint8_t goingUp) BANKED {
    if (liftCount == MAX_LIFTS) return;
    liftX[liftCount] = x;
    liftY[liftCount] = y;
    liftDy[liftCount] = goingUp ? -1 : 1;
    liftCount++;
}

uint8_t liftsLand(int16_t feetBefore) BANKED {
    uint8_t i;
    int16_t feet, left, right;
    const int16_t *x = liftX, *y = liftY;
    riding = 0;
    if (!liftCount || feetBefore == LIFTS_NOT_LANDING) return 0;
    feet = mario.y + mario.height;
    left = mario.x - LIFT_WIDTH;
    right = mario.x + mario.width;
    for (i = 0; i < liftCount; i++, x++, y++) {
        // his feet reached its top this frame, and he's over it
        if (feetBefore <= *y && feet >= *y && *x > left && *x < right) {
            mario.y = *y - mario.height;
            riding = i + 1;
            return 1;
        }
    }
    return 0;
}

void liftsUpdate(void) BANKED {
    uint8_t i;
    int16_t *y = liftY;
    const int8_t *dy = liftDy;
    for (i = 1; i <= liftCount; i++, y++, dy++) {
        *y += *dy;
        if (*y < LIFT_TOP_WRAP) {
            *y = LIFT_BOTTOM_WRAP;
            if (riding == i) riding = 0; // (he stays behind)
        } else if (*y > LIFT_BOTTOM_WRAP) {
            *y = LIFT_TOP_WRAP;
            if (riding == i) riding = 0;
        } else if (riding == i) {
            mario.y += *dy; // carry him along
        }
    }
}

void liftsDraw(void) BANKED {
    uint8_t i;
    const int16_t *x = liftX, *y = liftY;
    for (i = 0; i < liftCount; i++, x++, y++) {
        // behind the background: SMB's 1-2 has two lifts going round inside the
        // wall by its exit pipe, where they don't show
        spriteDrawRow(SPR_TILE_LIFT, LIFT_SEGMENTS, S_PRIORITY, *x, *y);
    }
}
