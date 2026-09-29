#ifndef SPRITES_H
#define SPRITES_H

#include "MarioTiles.h"

// Where each part of the game keeps its sprite tiles in video memory
#define SPR_TILE_MARIO      0                                        // MarioTilesCount tiles
#define SPR_TILE_BRICK_LINE (SPR_TILE_MARIO + MarioTilesCount)       // 4: bumped brick, TL TR BL BR
#define SPR_TILE_BRICK      (SPR_TILE_BRICK_LINE + 4)                // 4: bumped plain brick
#define SPR_TILE_USED       (SPR_TILE_BRICK + 4)                     // 4: bumped used block
#define SPR_TILE_COIN       (SPR_TILE_USED + 4)                      // 8: 4 spin frames x (top, bottom)
#define SPR_TILE_SCORE20    (SPR_TILE_COIN + 8)                      // "20"
#define SPR_TILE_SCORE0     (SPR_TILE_SCORE20 + 1)                   // "0"
#define SPR_TILE_FLAG       (SPR_TILE_SCORE0 + 1)                    // 4: flagpole flag, TL BL TR BR

// Hardware sprite coordinates are offset from the screen by (8, 16)
#define SPRITE_OFFSET_X 8
#define SPRITE_OFFSET_Y 16

// Which hardware sprites (OAM entries) each part of the game uses
#define OAM_MARIO 0  // 4
#define OAM_BUMP  4  // 4: block bouncing after a hit
#define OAM_COINS 8  // 3 per coin effect, 2 effects
#define OAM_FLAG  14 // 4

#endif
