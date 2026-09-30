// Level objects are banked code: ROM bank 3 (see make.bat)
#pragma bank 3

#include <gb/gb.h>
#include "Util.h"
#include "Powerup.h"
#include "Physics.h"
#include "Player.h"
#include "Camera.h"
#include "Level.h"
#include "Game.h"
#include "Popup.h"
#include "ItemTiles.h"
#include "Sprites.h"

#define ITEM_SPEED 16           // mushroom and star: 1 pixel per frame (1/16 pixel units)
#define STAR_BOUNCE -64         // the star hops along
#define EMERGE_FRAMES 32        // rises 16 pixels, half a pixel per frame
#define ITEM_POINTS 1000
#define SCREEN_WIDTH 160

enum { NONE, EMERGING, MOVING };

static struct GameCharacter item;
static uint8_t kind;  // POWERUP_*
static uint8_t state;
static uint8_t timer;
static uint8_t subX, subY;
static int16_t velocityY;
static int16_t speed; // +/- ITEM_SPEED

static const uint8_t itemTiles[] = {SPR_TILE_MUSHROOM, SPR_TILE_FLOWER, SPR_TILE_STAR, SPR_TILE_ONE_UP};

void powerupInit(void) BANKED {
    // mushroom, flower and star are consecutive in both places
    bankedSetSpriteData(SPR_TILE_MUSHROOM, 12, ItemTiles + ITEMTILES_MUSHROOM * 16, BANK(ItemTiles));
    bankedSetSpriteData(SPR_TILE_ONE_UP, 4, ItemTiles + ITEMTILES_ONE_UP * 16, BANK(ItemTiles));
}

void powerupReset(void) BANKED {
    state = NONE;
}

void powerupSpawn(int16_t tx, int16_t ty, uint8_t what) BANKED {
    item.x = tx << 3;
    item.y = ty << 3;
    item.width = 16;
    item.height = 16;
    kind = what;
    state = EMERGING;
    timer = 0;
    subX = subY = 0;
    velocityY = 0;
    speed = ITEM_SPEED;
}

static uint8_t touchingMario(void) {
    return item.x < mario.x + mario.width && item.x + item.width > mario.x &&
           item.y < mario.y + mario.height && item.y + item.height > mario.y;
}

// The mushroom slides along and falls off ledges; the star does the same but
// keeps hopping. The flower stays put.
static void move(void) {
    int8_t dy;

    if (physicsMoveX(&item, physicsSubPixelStep(&subX, speed))) {
        speed = -speed; // bounce off walls
        subX = 0;
    }
    velocityY += PHYSICS_GRAVITY;
    if (velocityY > PHYSICS_MAX_FALL_SPEED) velocityY = PHYSICS_MAX_FALL_SPEED;
    dy = physicsSubPixelStep(&subY, velocityY);
    if (physicsMoveY(&item, dy)) {
        velocityY = (kind == POWERUP_STAR && dy > 0) ? STAR_BOUNCE : 0;
        subY = 0;
    }
}

static void collect(void) {
    state = NONE;
    if (kind == POWERUP_ONE_UP) {
        gameAddLife();
        popupShow(item.x, item.y, POPUP_1UP);
        return;
    }
    switch (kind) {
    case POWERUP_MUSHROOM: playerGrow(); break;
    case POWERUP_FLOWER:   playerFire(); break;
    default:               playerStar(); break;
    }
    gameAddScore(ITEM_POINTS);
    popupShow(item.x, item.y, ITEM_POINTS);
}

void powerupUpdate(void) BANKED {
    if (state == EMERGING) {
        if (++timer & 1) item.y--;
        if (timer >= EMERGE_FRAMES) {
            state = MOVING;
            if (kind == POWERUP_STAR) velocityY = STAR_BOUNCE;
        }
    } else if (state == MOVING) {
        if (kind != POWERUP_FLOWER) move();
        if (item.y > LEVEL_HEIGHT || item.x + item.width < cameraX ||
            item.x > cameraX + SCREEN_WIDTH) {
            state = NONE; // fell in a pit or left the screen
        } else if (touchingMario()) {
            collect();
        }
    }
}

void powerupDraw(void) BANKED {
    // drawn behind the block while it rises out of it
    uint8_t props = (state == EMERGING) ? S_PRIORITY : 0;
    if (state == NONE) return;
    // ItemTiles' items are TL, BL, TR, BR
    spriteDraw16(itemTiles[kind], props, item.x, item.y);
}
