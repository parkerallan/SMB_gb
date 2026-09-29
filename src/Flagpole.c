// Level objects are banked code: ROM bank 3 (see make.bat)
#pragma bank 3

#include <gb/gb.h>
#include "Util.h"
#include "Flagpole.h"
#include "Level.h"
#include "ItemTiles.h"
#include "Sprites.h"

static uint8_t hasFlag;
static int16_t flagX, flagY; // top-left of the 16x16 flag, in level pixels

void flagpoleInit(void) BANKED {
    bankedSetSpriteData(SPR_TILE_FLAG, 4, ItemTiles + ITEMTILES_FLAG * 16, BANK(ItemTiles));
}

void flagpoleReset(void) BANKED {
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

void flagpoleDraw(void) BANKED {
    if (!hasFlag) return;
    // ItemTiles' flag is TL, BL, TR, BR
    spriteDraw16(SPR_TILE_FLAG, 0, flagX, flagY);
}
