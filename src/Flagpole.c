#include <gb/gb.h>
#include "Flagpole.h"
#include "Level.h"
#include "Camera.h"
#include "ItemTiles.h"
#include "Sprites.h"

static uint8_t hasFlag;
static int16_t flagX, flagY; // top-left of the 16x16 flag, in level pixels

void flagpoleInit(void) {
    uint8_t i, saved = CURRENT_BANK;
    SWITCH_ROM(BANK(ItemTiles));
    set_sprite_data(SPR_TILE_FLAG, 4, ItemTiles + ITEMTILES_FLAG * 16);
    SWITCH_ROM(saved);
    for (i = 0; i < 4; i++) set_sprite_tile(OAM_FLAG + i, SPR_TILE_FLAG + i);
}

void flagpoleReset(void) {
    int16_t bx, by;
    hasFlag = 0;
    for (by = 0; by < Level1_1Height && !hasFlag; by++) {
        for (bx = 0; bx < Level1_1Width; bx++) {
            if (levelBlockAt(bx, by) == SCENERY_FLAGPOLE_BALL) {
                // Hangs on the left of the pole, just under the ball
                flagX = (bx << 4) - 8;
                flagY = (by + 1) << 4;
                hasFlag = 1;
                break;
            }
        }
    }
}

void flagpoleDraw(void) {
    if (!hasFlag) return;
    // ItemTiles' flag is TL, BL, TR, BR
    cameraPlaceSprite(OAM_FLAG, flagX, flagY);
    cameraPlaceSprite(OAM_FLAG + 1, flagX, flagY + 8);
    cameraPlaceSprite(OAM_FLAG + 2, flagX + 8, flagY);
    cameraPlaceSprite(OAM_FLAG + 3, flagX + 8, flagY + 8);
}
