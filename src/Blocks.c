#include <gb/gb.h>
#include "Blocks.h"
#include "Level.h"
#include "Camera.h"
#include "Game.h"
#include "ItemTiles.h"
#include "Sprites.h"

#define MAX_COINS 2
#define MAX_USED_BLOCKS 16

// Coin physics in 1/16 pixels, like Player.c. Lower than the NES coin so it
// stays on the Game Boy's shorter screen: rises ~14px over ~10 frames.
#define COIN_JUMP -42
#define COIN_GRAVITY 4
#define COIN_SCORE_FRAMES 30
// Block rise in pixels on each frame of a bump
static const int8_t bumpOffsets[] = {-2, -4, -5, -6, -6, -5, -4, -2, -1};

// ? blocks that have been emptied, by top-left tile
static struct { int16_t tx; uint8_t ty; } usedBlocks[MAX_USED_BLOCKS];
static uint8_t usedCount;

static struct {
    uint8_t active, frame;
    int16_t tx;
    uint8_t ty;
    uint8_t sprite[4];    // sprite tiles shown while bumping: TL, TR, BL, BR
    uint8_t finalTile[4]; // background tiles restored afterwards
} bump;

static struct {
    uint8_t active, showScore, timer, sub;
    int16_t x, y, endY, vy;
} coinEffects[MAX_COINS];

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

// Sprite copy of a block's 4 background tiles, for bouncing it
static void loadBlockSprite(uint8_t firstSpriteTile, uint8_t block) {
    const uint8_t *tiles = levelBlockTiles(block);
    uint8_t i;
    for (i = 0; i < 4; i++) {
        set_sprite_data(firstSpriteTile + i, 1, SceneryTiles + tiles[i] * 16);
    }
}

void blocksInit(void) {
    uint8_t saved = CURRENT_BANK;
    SWITCH_ROM(BANK(SceneryTiles)); // graphics bank: SceneryTiles and ItemTiles
    loadBlockSprite(SPR_TILE_BRICK_LINE, SCENERY_BRICK_LINE);
    loadBlockSprite(SPR_TILE_BRICK, SCENERY_BRICK);
    loadBlockSprite(SPR_TILE_USED, SCENERY_USED_BLOCK);
    set_sprite_data(SPR_TILE_COIN, 8, ItemTiles + ITEMTILES_COIN * 16);
    set_sprite_data(SPR_TILE_SCORE20, 1, ItemTiles + (ITEMTILES_SCORE + 1) * 16);
    set_sprite_data(SPR_TILE_SCORE0, 1, ItemTiles + (ITEMTILES_SCORE + 5) * 16);
    SWITCH_ROM(saved);
}

static void hideSprites(uint8_t first, uint8_t count) {
    while (count--) move_sprite(first++, 0, 0);
}

void blocksReset(void) {
    uint8_t i;
    usedCount = 0;
    bump.active = 0;
    for (i = 0; i < MAX_COINS; i++) coinEffects[i].active = 0;
    hideSprites(OAM_BUMP, 4 + MAX_COINS * 3);
}

static uint8_t isUsed(int16_t tx, uint8_t ty) {
    uint8_t i;
    for (i = 0; i < usedCount; i++) {
        if (usedBlocks[i].tx == tx && usedBlocks[i].ty == ty) return 1;
    }
    return 0;
}

void blocksPatchColumn(int16_t column, uint8_t *tiles) {
    const uint8_t *used = levelBlockTiles(SCENERY_USED_BLOCK);
    uint8_t i;
    int16_t right;
    for (i = 0; i < usedCount; i++) {
        right = column - usedBlocks[i].tx; // 0 = left half, 1 = right half
        if (right == 0 || right == 1) {
            tiles[usedBlocks[i].ty] = used[right];
            tiles[usedBlocks[i].ty + 1] = used[2 + right];
        }
    }
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
    hideSprites(OAM_BUMP, 4);
    bump.active = 0;
}

// Swap the block's background tiles for sky and bounce a sprite copy instead.
// `becomes` is the block shown afterwards (a hit ? block becomes a used block).
static void startBump(int16_t tx, uint8_t ty, uint8_t becomes) {
    uint8_t firstSprite, i;
    const uint8_t *finalTiles = levelBlockTiles(becomes);

    finishBump();
    if (becomes == SCENERY_USED_BLOCK) firstSprite = SPR_TILE_USED;
    else if (isLineBrick(becomes))     firstSprite = SPR_TILE_BRICK_LINE;
    else                               firstSprite = SPR_TILE_BRICK;
    bump.active = 1;
    bump.frame = 0;
    bump.tx = tx;
    bump.ty = ty;
    for (i = 0; i < 4; i++) {
        bump.sprite[i] = firstSprite + i;
        bump.finalTile[i] = finalTiles[i];
        set_sprite_tile(OAM_BUMP + i, bump.sprite[i]);
    }
    setBlockTiles(tx, ty, levelBlockTiles(SCENERY_BLANK));
}

static void spawnCoin(int16_t tx, uint8_t ty) {
    uint8_t i;
    for (i = 0; i < MAX_COINS; i++) {
        if (!coinEffects[i].active) break;
    }
    if (i == MAX_COINS) i = 0; // reuse the oldest slot
    coinEffects[i].active = 1;
    coinEffects[i].showScore = 0;
    coinEffects[i].timer = 0;
    coinEffects[i].sub = 0;
    coinEffects[i].x = (tx << 3) + 4;         // centered over the 16px block
    coinEffects[i].y = (ty << 3) - 16;        // just above it
    coinEffects[i].endY = coinEffects[i].y - 4;     // vanishes on the way back down
    coinEffects[i].vy = COIN_JUMP;
}

void blocksHit(int16_t tx, int16_t ty) {
    uint8_t block = levelBlockAt(tx >> 1, ty >> 1);
    int16_t bx = tx & ~1; // block's top-left tile
    uint8_t by = ty & ~1;

    if (isQuestion(block)) {
        if (isUsed(bx, by) || usedCount == MAX_USED_BLOCKS) return; // empty: no bump
        usedBlocks[usedCount].tx = bx;
        usedBlocks[usedCount].ty = by;
        usedCount++;
        startBump(bx, by, SCENERY_USED_BLOCK);
        spawnCoin(bx, by);
        gameCollectCoin();
    } else if (isBrick(block)) {
        startBump(bx, by, block); // small Mario can't break bricks
    }
}

static void updateBump(void) {
    int16_t x, y;
    if (!bump.active) return;
    if (bump.frame >= sizeof(bumpOffsets)) {
        finishBump();
        return;
    }
    x = bump.tx << 3;
    y = (bump.ty << 3) + bumpOffsets[bump.frame++];
    cameraPlaceSprite(OAM_BUMP, x, y);
    cameraPlaceSprite(OAM_BUMP + 1, x + 8, y);
    cameraPlaceSprite(OAM_BUMP + 2, x, y + 8);
    cameraPlaceSprite(OAM_BUMP + 3, x + 8, y + 8);
}

static void updateCoin(uint8_t i) {
    uint8_t oam = OAM_COINS + i * 3;
    int16_t total;
    uint8_t frame;

    if (!coinEffects[i].active) return;

    if (!coinEffects[i].showScore) {
        total = (int16_t)coinEffects[i].sub + coinEffects[i].vy;
        coinEffects[i].sub = total & 15;
        coinEffects[i].y += total >> 4;
        coinEffects[i].vy += COIN_GRAVITY;
        if (coinEffects[i].vy > 0 && coinEffects[i].y >= coinEffects[i].endY) {
            // Coin done: turn it into a floating "200"
            coinEffects[i].showScore = 1;
            coinEffects[i].timer = 0;
            coinEffects[i].x -= 4;                    // "200" centered on the block
            coinEffects[i].y = coinEffects[i].endY + 12; // just above the block
            set_sprite_tile(oam, SPR_TILE_SCORE20);
            set_sprite_tile(oam + 1, SPR_TILE_SCORE0);
            move_sprite(oam + 2, 0, 0);
        } else {
            frame = (coinEffects[i].timer++ >> 2) & 3;
            set_sprite_tile(oam, SPR_TILE_COIN + frame * 2);
            set_sprite_tile(oam + 1, SPR_TILE_COIN + frame * 2 + 1);
            cameraPlaceSprite(oam, coinEffects[i].x, coinEffects[i].y);
            cameraPlaceSprite(oam + 1, coinEffects[i].x, coinEffects[i].y + 8);
            move_sprite(oam + 2, 0, 0);
            return;
        }
    }

    if (++coinEffects[i].timer > COIN_SCORE_FRAMES) {
        coinEffects[i].active = 0;
        hideSprites(oam, 3);
        return;
    }
    if (coinEffects[i].timer & 1) coinEffects[i].y--;
    cameraPlaceSprite(oam, coinEffects[i].x, coinEffects[i].y);
    cameraPlaceSprite(oam + 1, coinEffects[i].x + 8, coinEffects[i].y);
}

void blocksUpdate(void) {
    uint8_t i;
    updateBump();
    for (i = 0; i < MAX_COINS; i++) updateCoin(i);
}
