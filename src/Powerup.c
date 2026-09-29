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

#define MUSHROOM_SPEED 16       // 1 pixel per frame (1/16 pixel units)
#define EMERGE_FRAMES 32        // rises 16 pixels, half a pixel per frame
#define MUSHROOM_POINTS 1000
#define SCREEN_WIDTH 160

enum { NONE, EMERGING, MOVING };

static struct GameCharacter mushroom;
static uint8_t state;
static uint8_t timer;
static uint8_t subX, subY;
static int16_t velocityY;
static int16_t speed; // +/- MUSHROOM_SPEED

void powerupInit(void) BANKED {
    bankedSetSpriteData(SPR_TILE_MUSHROOM, 4, ItemTiles + ITEMTILES_MUSHROOM * 16, BANK(ItemTiles));
}

void powerupReset(void) BANKED {
    state = NONE;
}

void powerupSpawn(int16_t tx, int16_t ty) BANKED {
    mushroom.x = tx << 3;
    mushroom.y = ty << 3;
    mushroom.width = 16;
    mushroom.height = 16;
    state = EMERGING;
    timer = 0;
    subX = subY = 0;
    velocityY = 0;
    speed = MUSHROOM_SPEED;
}

static uint8_t touchingMario(void) {
    return mushroom.x < mario.x + mario.width && mushroom.x + mushroom.width > mario.x &&
           mushroom.y < mario.y + mario.height && mushroom.y + mushroom.height > mario.y;
}

static void move(void) {
    int8_t dy;

    if (physicsMoveX(&mushroom, physicsSubPixelStep(&subX, speed))) {
        speed = -speed; // bounce off walls
        subX = 0;
    }
    velocityY += PHYSICS_GRAVITY;
    if (velocityY > PHYSICS_MAX_FALL_SPEED) velocityY = PHYSICS_MAX_FALL_SPEED;
    dy = physicsSubPixelStep(&subY, velocityY);
    if (physicsMoveY(&mushroom, dy)) {
        velocityY = 0;
        subY = 0;
    }
}

void powerupUpdate(void) BANKED {
    if (state == EMERGING) {
        if (++timer & 1) mushroom.y--;
        if (timer >= EMERGE_FRAMES) state = MOVING;
    } else if (state == MOVING) {
        move();
        if (mushroom.y > LEVEL_HEIGHT || mushroom.x + mushroom.width < cameraX ||
            mushroom.x > cameraX + SCREEN_WIDTH) {
            state = NONE; // fell in a pit or left the screen
        } else if (touchingMario()) {
            state = NONE;
            playerGrow();
            gameAddScore(MUSHROOM_POINTS);
            popupShow(mushroom.x, mushroom.y, MUSHROOM_POINTS);
        }
    }
}

void powerupDraw(void) BANKED {
    // drawn behind the block while it rises out of it
    uint8_t props = (state == EMERGING) ? S_PRIORITY : 0;
    if (state == NONE) return;
    // ItemTiles' mushroom is TL, BL, TR, BR
    spriteDraw16(SPR_TILE_MUSHROOM, props, mushroom.x, mushroom.y);
}
