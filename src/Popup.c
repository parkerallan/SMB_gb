// Level objects are banked code: ROM bank 3 (see make.bat)
#pragma bank 3

#include <gb/gb.h>
#include "Util.h"
#include "Popup.h"
#include "ItemTiles.h"
#include "Sprites.h"

#define MAX_POPUPS 4
#define POPUP_FRAMES 30

// ItemTiles' score tiles, in order: "10" "20" "40" "50" "80" "0" "1U" "P" "00"
#define TILE_10 ((uint8_t)(SPR_TILE_SCORE + 0))
#define TILE_20 ((uint8_t)(SPR_TILE_SCORE + 1))
#define TILE_40 ((uint8_t)(SPR_TILE_SCORE + 2))
#define TILE_50 ((uint8_t)(SPR_TILE_SCORE + 3))
#define TILE_80 ((uint8_t)(SPR_TILE_SCORE + 4))
#define TILE_0  ((uint8_t)(SPR_TILE_SCORE + 5))
#define TILE_1U ((uint8_t)(SPR_TILE_SCORE + 6))
#define TILE_P  ((uint8_t)(SPR_TILE_SCORE + 7))
#define TILE_00 ((uint8_t)(SPR_TILE_SCORE + 8))

static struct {
    uint8_t active, timer, left, right;
    int16_t x, y;
} popups[MAX_POPUPS];

// Is anything showing? Most frames nothing is, and then update and draw
// return straight away (looping over the empty slots cost several scanlines)
static uint8_t busy;

void popupsInit(void) BANKED {
    bankedSetSpriteData(SPR_TILE_SCORE, 9, ItemTiles + ITEMTILES_SCORE * 16, BANK(ItemTiles));
}

void popupsReset(void) BANKED {
    uint8_t i;
    for (i = 0; i < MAX_POPUPS; i++) popups[i].active = 0;
    busy = 0;
}

void popupShow(int16_t x, int16_t y, uint16_t points) BANKED {
    uint8_t i, left, right;
    uint16_t lead = points;

    if (points == POPUP_1UP) {
        left = TILE_1U;
        right = TILE_P;
    } else {
        // "10" .. "80" followed by "0" (hundreds) or "00" (thousands)
        right = TILE_0;
        if (points >= 1000) {
            lead = points / 10;
            right = TILE_00;
        }
        switch (lead) {
        case 100: left = TILE_10; break;
        case 200: left = TILE_20; break;
        case 400: left = TILE_40; break;
        case 500: left = TILE_50; break;
        default:  left = TILE_80; break;
        }
    }

    for (i = 0; i < MAX_POPUPS; i++) {
        if (!popups[i].active) break;
    }
    if (i == MAX_POPUPS) i = 0; // reuse the oldest
    popups[i].active = 1;
    busy = 1;
    popups[i].timer = 0;
    popups[i].x = x;
    popups[i].y = y;
    popups[i].left = left;
    popups[i].right = right;
}

void popupsUpdate(void) BANKED {
    uint8_t i;
    if (!busy) return;
    busy = 0;
    for (i = 0; i < MAX_POPUPS; i++) {
        if (!popups[i].active) continue;
        if (++popups[i].timer > POPUP_FRAMES) popups[i].active = 0;
        else if (popups[i].timer & 1) popups[i].y--;
        busy |= popups[i].active;
    }
}

void popupsDraw(void) BANKED {
    uint8_t i;
    if (!busy) return;
    for (i = 0; i < MAX_POPUPS; i++) {
        if (!popups[i].active) continue;
        spriteDraw(popups[i].left, 0, popups[i].x, popups[i].y);
        spriteDraw(popups[i].right, 0, popups[i].x + 8, popups[i].y);
    }
}
