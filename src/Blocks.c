// Level objects are banked code: ROM bank 3 (see make.bat)
#pragma bank 3

#include <gb/gb.h>
#include "Util.h"
#include "Blocks.h"
#include "Level.h"
#include "Game.h"
#include "Player.h"
#include "Powerup.h"
#include "Enemies.h"
#include "Popup.h"
#include "Physics.h"
#include "ItemTiles.h"
#include "Sprites.h"

#define MAX_COINS 2
#define BRICK_POINTS 50
#define COIN_POINTS 200
// A multi-coin brick gives a coin per hit until this long after its first
// hit; the next hit gives its last coin and uses it up
#define MULTI_COIN_FRAMES 240

// Coin physics in 1/16 pixels, like Player.c. Lower than the NES coin so it
// stays on the Game Boy's shorter screen: rises ~14px over ~10 frames.
#define COIN_JUMP -42
#define COIN_GRAVITY 4
// Block rise in pixels on each frame of a bump
static const int8_t bumpOffsets[] = {-2, -4, -5, -6, -6, -5, -4, -2, -1};

// Broken brick: 4 pieces fly out from its quarters (1/16 pixel speeds)
#define FRAGMENT_GRAVITY 5
static const int8_t fragmentSpeedX[4] = {-16, 16, -16, 16};   // TL, TR, BL, BR
static const int8_t fragmentSpeedY[4] = {-80, -80, -48, -48}; // top pieces fly higher

static struct {
    uint8_t active, frame;
    int16_t tx;
    uint8_t ty;
    uint8_t sprite;       // first of 4 sprite tiles shown while bumping: TL, TR, BL, BR
    uint8_t finalTile[4]; // background tiles restored afterwards
} bump;

static struct {
    uint8_t active, timer, sub;
    int16_t x, y, endY, vy;
} coinEffects[MAX_COINS];

static struct {
    uint8_t active, timer, subX, subY;
    int16_t x, y, vy;
} fragments[4];

// Is anything showing? Most frames nothing is, and then update and draw
// return straight away (looping over the empty slots cost several scanlines)
static uint8_t busy;

// The multi-coin brick being emptied, if any (block coordinates) and its time left
static int16_t multiX;
static uint8_t multiY;
static uint8_t multiTimer;

static uint8_t isQuestion(uint8_t block) {
    return block == SCENERY_QUESTION_COIN || block == SCENERY_QUESTION_POWERUP;
}

// Bricks with a line along the top, like the ones in 1-1
static uint8_t isLineBrick(uint8_t block) {
    return block == SCENERY_BRICK_LINE ||
           (block >= SCENERY_BRICK_LINE_POWERUP && block <= SCENERY_BRICK_LINE_1UP);
}

static uint8_t isBrick(uint8_t block) {
    return isLineBrick(block) || block == SCENERY_BRICK || block == SCENERY_BRICK_UNUSED ||
           (block >= SCENERY_BRICK_POWERUP && block <= SCENERY_BRICK_1UP);
}

static uint8_t isItemBrick(uint8_t block) {
    return block == SCENERY_BRICK_LINE_STAR || block == SCENERY_BRICK_STAR ||
           block == SCENERY_BRICK_LINE_POWERUP || block == SCENERY_BRICK_POWERUP;
}

// Sprite copy of a block's 4 background tiles, for bouncing it
static void loadBlockSprite(uint8_t firstSpriteTile, uint8_t block) {
    const uint8_t *tiles = levelBlockTiles(block);
    uint8_t i;
    for (i = 0; i < 4; i++) {
        bankedSetSpriteData(firstSpriteTile + i, 1, SceneryTiles + tiles[i] * 16, BANK(SceneryTiles));
    }
}

void blocksInit(void) BANKED {
    loadBlockSprite(SPR_TILE_BRICK_LINE, SCENERY_BRICK_LINE);
    loadBlockSprite(SPR_TILE_BRICK, SCENERY_BRICK);
    loadBlockSprite(SPR_TILE_USED, SCENERY_USED_BLOCK);
    bankedSetSpriteData(SPR_TILE_COIN, 8, ItemTiles + ITEMTILES_COIN * 16, BANK(ItemTiles));
    bankedSetSpriteData(SPR_TILE_BRICK_PIECE, 1, ItemTiles + ITEMTILES_BRICK_PIECE * 16, BANK(ItemTiles));
}

void blocksReset(void) BANKED {
    uint8_t i;
    bump.active = 0;
    for (i = 0; i < MAX_COINS; i++) coinEffects[i].active = 0;
    for (i = 0; i < 4; i++) fragments[i].active = 0;
    busy = 0;
    multiX = -1;
    multiTimer = 0;
}

static void setBlockTiles(int16_t tx, uint8_t ty, const uint8_t *tiles) {
    levelSetTile(tx, ty, tiles[0]);
    levelSetTile(tx + 1, ty, tiles[1]);
    levelSetTile(tx, ty + 1, tiles[2]);
    levelSetTile(tx + 1, ty + 1, tiles[3]);
}

static void finishBump(void) {
    if (!bump.active) return;
    setBlockTiles(bump.tx, bump.ty, bump.finalTile);
    bump.active = 0;
}

// Swap the block's background tiles for sky and bounce a sprite copy instead.
// `becomes` is the block shown afterwards (a hit ? block becomes a used block).
static void startBump(int16_t tx, uint8_t ty, uint8_t becomes) {
    const uint8_t *finalTiles = levelBlockTiles(becomes);
    uint8_t i;

    finishBump();
    if (becomes == SCENERY_USED_BLOCK) bump.sprite = SPR_TILE_USED;
    else if (isLineBrick(becomes))     bump.sprite = SPR_TILE_BRICK_LINE;
    else                               bump.sprite = SPR_TILE_BRICK;
    bump.active = 1;
    busy = 1;
    bump.frame = 0;
    bump.tx = tx;
    bump.ty = ty;
    for (i = 0; i < 4; i++) bump.finalTile[i] = finalTiles[i];
    setBlockTiles(tx, ty, levelBlockTiles(SCENERY_BLANK));
}

static void spawnCoin(int16_t tx, uint8_t ty) {
    uint8_t i;
    for (i = 0; i < MAX_COINS; i++) {
        if (!coinEffects[i].active) break;
    }
    if (i == MAX_COINS) i = 0; // reuse the oldest slot
    coinEffects[i].active = 1;
    busy = 1;
    coinEffects[i].timer = 0;
    coinEffects[i].sub = 0;
    coinEffects[i].x = (tx << 3) + 4;           // centered over the 16px block
    coinEffects[i].y = (ty << 3) - 16;          // just above it
    coinEffects[i].endY = coinEffects[i].y - 4; // vanishes on the way back down
    coinEffects[i].vy = COIN_JUMP;
}

// Big Mario smashes a plain brick: it's gone, and 4 pieces fly out
static void breakBrick(int16_t tx, uint8_t ty) {
    uint8_t i;
    levelSetBlock(tx >> 1, ty >> 1, SCENERY_BLANK);
    for (i = 0; i < 4; i++) {
        fragments[i].active = 1;
        busy = 1;
        fragments[i].timer = 0;
        fragments[i].subX = fragments[i].subY = 0;
        fragments[i].x = (tx << 3) + ((i & 1) << 3);
        fragments[i].y = (ty << 3) + ((i >> 1) << 3);
        fragments[i].vy = fragmentSpeedY[i];
    }
    gameAddScore(BRICK_POINTS);
}

void blocksCollectCoins(void) BANKED {
    int16_t bx, by;
    int16_t left = mario.x >> 4, right = (mario.x + mario.width - 1) >> 4;
    int16_t bottom = (mario.y + mario.height - 1) >> 4;
    for (by = mario.y >> 4; by <= bottom; by++) {
        for (bx = left; bx <= right; bx++) {
            if (levelBlockAt(bx, by) == SCENERY_COIN) {
                levelSetBlock(bx, by, SCENERY_BLANK);
                levelCoins--;
                gameCollectCoin();
            }
        }
    }
}

void blocksHit(int16_t tx, int16_t ty) BANKED {
    uint8_t block = levelBlockAt(tx >> 1, ty >> 1);
    int16_t bx = tx & ~1; // block's top-left tile
    uint8_t by = ty & ~1;

    if (isQuestion(block)) {
        levelSetBlock(bx >> 1, by >> 1, SCENERY_USED_BLOCK);
        startBump(bx, by, SCENERY_USED_BLOCK);
        if (block == SCENERY_QUESTION_POWERUP) {
            powerupSpawn(bx, by, playerIsBig() ? POWERUP_FLOWER : POWERUP_MUSHROOM);
        } else {
            spawnCoin(bx, by);
            gameCollectCoin();
        }
    } else if (block == SCENERY_HIDDEN_1UP || block == SCENERY_HIDDEN_COIN) {
        // appears (see playerUpdate) and gives its 1-up or coin
        levelSetBlock(bx >> 1, by >> 1, SCENERY_USED_BLOCK);
        startBump(bx, by, SCENERY_USED_BLOCK);
        if (block == SCENERY_HIDDEN_1UP) {
            powerupSpawn(bx, by, POWERUP_ONE_UP);
        } else {
            spawnCoin(bx, by);
            gameCollectCoin();
        }
    } else if (block == SCENERY_BRICK_LINE_COINS || block == SCENERY_BRICK_COINS) {
        if (multiX != (bx >> 1) || multiY != (by >> 1)) {
            // first hit starts its timer
            multiX = bx >> 1;
            multiY = by >> 1;
            multiTimer = MULTI_COIN_FRAMES;
            busy = 1;
        }
        spawnCoin(bx, by);
        gameCollectCoin();
        if (multiTimer) {
            startBump(bx, by, block);
        } else {
            // time's up: that was its last coin
            levelSetBlock(bx >> 1, by >> 1, SCENERY_USED_BLOCK);
            startBump(bx, by, SCENERY_USED_BLOCK);
            multiX = -1;
        }
    } else if (isItemBrick(block)) {
        // bricks holding an item give it once, then are used blocks
        levelSetBlock(bx >> 1, by >> 1, SCENERY_USED_BLOCK);
        startBump(bx, by, SCENERY_USED_BLOCK);
        if (block == SCENERY_BRICK_LINE_STAR || block == SCENERY_BRICK_STAR) {
            powerupSpawn(bx, by, POWERUP_STAR);
        } else {
            powerupSpawn(bx, by, playerIsBig() ? POWERUP_FLOWER : POWERUP_MUSHROOM);
        }
    } else if (block == SCENERY_BRICK_LINE || block == SCENERY_BRICK) {
        if (playerIsBig()) breakBrick(bx, by);
        else               startBump(bx, by, block);
    } else if (isBrick(block)) {
        startBump(bx, by, block); // bricks holding items don't break
    } else {
        return;
    }
    enemiesBumpBlock(bx >> 1, by >> 1); // knock out anything standing on it
}

static void updateCoin(uint8_t i) {
    int16_t total;
    if (!coinEffects[i].active) return;
    total = (int16_t)coinEffects[i].sub + coinEffects[i].vy;
    coinEffects[i].sub = total & 15;
    coinEffects[i].y += total >> 4;
    coinEffects[i].vy += COIN_GRAVITY;
    coinEffects[i].timer++;
    if (coinEffects[i].vy > 0 && coinEffects[i].y >= coinEffects[i].endY) {
        coinEffects[i].active = 0;
        // "200" centered on the block, just above it
        popupShow(coinEffects[i].x - 4, coinEffects[i].endY + 12, COIN_POINTS);
    }
}

static void updateFragment(uint8_t i) {
    if (!fragments[i].active) return;
    fragments[i].x += physicsSubPixelStep(&fragments[i].subX, fragmentSpeedX[i]);
    fragments[i].y += physicsSubPixelStep(&fragments[i].subY, fragments[i].vy);
    fragments[i].vy += FRAGMENT_GRAVITY;
    fragments[i].timer++;
    if (fragments[i].y > LEVEL_HEIGHT) fragments[i].active = 0;
}

void blocksUpdate(void) BANKED {
    uint8_t i;
    if (!busy) return;
    if (bump.active && ++bump.frame >= sizeof(bumpOffsets)) finishBump();
    if (multiTimer) multiTimer--;
    busy = bump.active | (multiTimer != 0);
    for (i = 0; i < MAX_COINS; i++) {
        updateCoin(i);
        busy |= coinEffects[i].active;
    }
    for (i = 0; i < 4; i++) {
        updateFragment(i);
        busy |= fragments[i].active;
    }
}

void blocksDraw(void) BANKED {
    uint8_t i, frame;
    int16_t x, y;

    if (!busy) return;
    if (bump.active) {
        x = bump.tx << 3;
        y = (bump.ty << 3) + bumpOffsets[bump.frame];
        spriteDraw(bump.sprite, 0, x, y);
        spriteDraw(bump.sprite + 1, 0, x + 8, y);
        spriteDraw(bump.sprite + 2, 0, x, y + 8);
        spriteDraw(bump.sprite + 3, 0, x + 8, y + 8);
    }
    for (i = 0; i < MAX_COINS; i++) {
        if (!coinEffects[i].active) continue;
        frame = (coinEffects[i].timer >> 2) & 3;
        spriteDraw(SPR_TILE_COIN + frame * 2, 0, coinEffects[i].x, coinEffects[i].y);
        spriteDraw(SPR_TILE_COIN + frame * 2 + 1, 0, coinEffects[i].x, coinEffects[i].y + 8);
    }
    for (i = 0; i < 4; i++) {
        if (!fragments[i].active) continue;
        // tumble by flipping every few frames
        spriteDraw(SPR_TILE_BRICK_PIECE, (fragments[i].timer & 4) ? S_FLIPX : 0, fragments[i].x, fragments[i].y);
    }
}
