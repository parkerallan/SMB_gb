#ifndef SPRITES_H
#define SPRITES_H

#include <stdint.h>
#include <gb/metasprites.h>
#include "MarioTiles.h"

// Where each part of the game keeps its sprite tiles in video memory. Tiles
// 128-155 are shared with the level's scenery tiles and 215-255 with the font,
// so sprites use 0-127 and 156-214.
#define SPR_SMALL_MARIO_TILES MARIOTILES_SKID   // small Mario's stand, walk and jump frames
#define SPR_TILE_MARIO       0                                          // small Mario
#define SPR_TILE_MARIO_DEAD  (SPR_TILE_MARIO + SPR_SMALL_MARIO_TILES)    // 4: TL BL TR BR
#define SPR_TILE_BIG_MARIO   (SPR_TILE_MARIO_DEAD + 4)                   // 56: 7 big frames x 8
#define SPR_TILE_BRICK_LINE  (SPR_TILE_BIG_MARIO + 56)                   // 4: bumped brick, TL TR BL BR
#define SPR_TILE_BRICK       (SPR_TILE_BRICK_LINE + 4)                   // 4: bumped plain brick
#define SPR_TILE_USED        (SPR_TILE_BRICK + 4)                        // 4: bumped used block
#define SPR_TILE_COIN        (SPR_TILE_USED + 4)                         // 8: 4 spin frames x (top, bottom)
#define SPR_TILE_FLAG        (SPR_TILE_COIN + 8)                         // 4: flagpole flag, TL BL TR BR
#define SPR_TILE_MUSHROOM    (SPR_TILE_FLAG + 4)                         // 4: TL BL TR BR
#define SPR_TILE_FLOWER      (SPR_TILE_MUSHROOM + 4)                     // 4: TL BL TR BR
#define SPR_TILE_STAR        (SPR_TILE_FLOWER + 4)                       // 4: TL BL TR BR
#define SPR_TILE_BRICK_PIECE (SPR_TILE_STAR + 4)                         // 1
#define SPR_TILE_FIREBALL    (SPR_TILE_BRICK_PIECE + 1)                  // 1
#define SPR_TILE_BIG_FIRE    (SPR_TILE_FIREBALL + 1)                     // 8: fire Mario throwing
#define SPR_TILE_SCORE       ((uint8_t)156)                             // 9: see Popup.c
#define SPR_TILE_ENEMIES     ((uint8_t)(SPR_TILE_SCORE + 9))             // 28: see Enemies.c
#define SPR_TILE_EXPLOSION   ((uint8_t)(SPR_TILE_ENEMIES + 28))          // 12: small, medium, large (see Fireball.c)

// Hardware sprite coordinates are offset from the screen by (8, 16)
#define SPRITE_OFFSET_X 8
#define SPRITE_OFFSET_Y 16

// Hardware sprites are handed out afresh every frame, in drawing order, to
// whatever is on screen: call spritesBegin(), draw everything, then spritesEnd().
void spritesBegin(void);
void spritesEnd(void);   // hides the hardware sprites nobody used this frame

// One 8x8 sprite at a level position; skipped when off screen, under the HUD,
// or when all 40 hardware sprites are taken
void spriteDraw(uint8_t tile, uint8_t props, int16_t x, int16_t y);

// A 16x16 frame of 4 tiles stored TL, BL, TR, BR, and a Koopa's 16x24 frame of
// 6 tiles row by row (TL blank), at a level position. `props` can mirror them
// (S_FLIPX, S_FLIPY) or put them behind the background (S_PRIORITY). Skipped
// when off screen, under the HUD, or out of hardware sprites.
void spriteDraw16(uint8_t first, uint8_t props, int16_t x, int16_t y);
void spriteDrawKoopa(uint8_t first, uint8_t props, int16_t x, int16_t y);

// For metasprites: draw at hardware sprite spritesNext(), then report how many were used
uint8_t spritesNext(void);
void spritesAdvance(uint8_t count);

#endif
