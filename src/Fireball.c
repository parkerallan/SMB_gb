// Level objects are banked code: ROM bank 3 (see make.bat)
#pragma bank 3

#include <gb/gb.h>
#include "Util.h"
#include "Fireball.h"
#include "Physics.h"
#include "Camera.h"
#include "Level.h"
#include "Enemies.h"
#include "ItemTiles.h"
#include "Sprites.h"

#define MAX_FIREBALLS 2       // like SMB: two on screen at once
#define FIREBALL_SPEED 64     // 4 pixels per frame (1/16 pixel units)
#define FIREBALL_DROP 32      // thrown slightly downward
#define FIREBALL_GRAVITY 5
#define FIREBALL_BOUNCE -40   // bounces along the ground
#define FIREBALL_MAX_FALL 64
#define EXPLOSION_FRAME_TIME 3 // frames per explosion size (small, medium, large)
#define SCREEN_WIDTH 160

enum { NONE, FLYING, EXPLODING };

static struct {
    struct GameCharacter body; // 8x8
    uint8_t state;
    uint8_t timer;
    uint8_t subX, subY;
    int8_t speed;
    int16_t velocityY;
} fireballs[MAX_FIREBALLS];

// Is anything showing? Most frames nothing is, and then update and draw
// return straight away (looping over the empty slots cost several scanlines)
static uint8_t busy;

void fireballsInit(void) BANKED {
    bankedSetSpriteData(SPR_TILE_FIREBALL, 1, ItemTiles + ITEMTILES_FIREBALL * 16, BANK(ItemTiles));
    // small, medium and large are consecutive
    bankedSetSpriteData(SPR_TILE_EXPLOSION, 12, ItemTiles + ITEMTILES_EXPLOSION_SMALL * 16, BANK(ItemTiles));
}

void fireballsReset(void) BANKED {
    uint8_t i;
    for (i = 0; i < MAX_FIREBALLS; i++) fireballs[i].state = NONE;
    busy = 0;
}

uint8_t fireballThrow(int16_t x, int16_t y, uint8_t left) BANKED {
    uint8_t i;
    for (i = 0; i < MAX_FIREBALLS; i++) {
        if (fireballs[i].state == NONE) break;
    }
    if (i == MAX_FIREBALLS) return 0;
    fireballs[i].body.x = x;
    fireballs[i].body.y = y;
    fireballs[i].body.width = 8;
    fireballs[i].body.height = 8;
    fireballs[i].state = FLYING;
    busy = 1;
    fireballs[i].timer = 0;
    fireballs[i].subX = fireballs[i].subY = 0;
    fireballs[i].speed = left ? -FIREBALL_SPEED : FIREBALL_SPEED;
    fireballs[i].velocityY = FIREBALL_DROP;
    return 1;
}

static void explode(uint8_t i) {
    fireballs[i].state = EXPLODING;
    fireballs[i].timer = 0;
    // the explosion is 16x16, centered where the fireball was
    fireballs[i].body.x -= 4;
    fireballs[i].body.y -= 4;
}

static void fly(uint8_t i) {
    struct GameCharacter *body = &fireballs[i].body;
    int8_t dy;

    fireballs[i].timer++;
    if (physicsMoveX(body, physicsSubPixelStep(&fireballs[i].subX, fireballs[i].speed))) {
        explode(i); // hit a wall
        return;
    }
    fireballs[i].velocityY += FIREBALL_GRAVITY;
    if (fireballs[i].velocityY > FIREBALL_MAX_FALL) fireballs[i].velocityY = FIREBALL_MAX_FALL;
    dy = physicsSubPixelStep(&fireballs[i].subY, fireballs[i].velocityY);
    if (physicsMoveY(body, dy)) {
        fireballs[i].subY = 0;
        fireballs[i].velocityY = (dy > 0) ? FIREBALL_BOUNCE : 0;
    }
    if (body->y > LEVEL_HEIGHT || body->x + 8 < cameraX || body->x > cameraX + SCREEN_WIDTH) {
        fireballs[i].state = NONE; // gone off screen
    } else if (enemiesFireballHit(body->x, body->y)) {
        explode(i);
    }
}

void fireballsUpdate(void) BANKED {
    uint8_t i;
    if (!busy) return;
    busy = 0;
    for (i = 0; i < MAX_FIREBALLS; i++) {
        if (fireballs[i].state == FLYING) {
            fly(i);
        } else if (fireballs[i].state == EXPLODING) {
            if (++fireballs[i].timer >= 3 * EXPLOSION_FRAME_TIME) fireballs[i].state = NONE;
        }
        busy |= fireballs[i].state;
    }
}

void fireballsDraw(void) BANKED {
    uint8_t i, size;
    if (!busy) return;
    for (i = 0; i < MAX_FIREBALLS; i++) {
        if (fireballs[i].state == FLYING) {
            // spins by flipping
            spriteDraw(SPR_TILE_FIREBALL, (fireballs[i].timer << 3) & (S_FLIPX | S_FLIPY),
                       fireballs[i].body.x, fireballs[i].body.y);
        } else if (fireballs[i].state == EXPLODING) {
            size = fireballs[i].timer / EXPLOSION_FRAME_TIME;
            spriteDraw16(SPR_TILE_EXPLOSION + (size << 2), 0, fireballs[i].body.x, fireballs[i].body.y);
        }
    }
}
