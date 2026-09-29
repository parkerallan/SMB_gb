#include <gb/gb.h>
#include <gb/metasprites.h>
#include "Player.h"
#include "Level.h"
#include "Camera.h"
#include "Mario.h"
#include "Metasprites.h"

// Physics runs every frame in 1/16 pixel units (4-bit sub-pixels)
#define GRAVITY 3
#define JUMP_STRENGTH -80
#define MAX_FALL_SPEED 64
#define WALK_SPEED 24
#define WALK_ANIM_FRAMES 6
#define MARIO_TILE_COUNT 20
#define MARIO_START_X 40
#define MARIO_START_Y 192
// Hardware sprite coordinates are offset from the screen by (8, 16)
#define SPRITE_OFFSET_X 8
#define SPRITE_OFFSET_Y 16

struct GameCharacter mario;

static int16_t velocityY;
static uint8_t subX, subY;
static uint8_t onGround;
static uint8_t moving;
static uint8_t facingLeft;
static uint8_t walkTimer, walkFrame;

static const metasprite_t* const walkFrames[] = {mario_walk_frame1, mario_walk_frame2, mario_walk_frame3};

void playerInit(void) {
    set_sprite_data(0, MARIO_TILE_COUNT, MarioTiles);
}

void playerReset(void) {
    mario.x = MARIO_START_X;
    mario.y = MARIO_START_Y;
    mario.width = 16;
    mario.height = 16;
    velocityY = 0;
    subX = 0;
    subY = 0;
    onGround = 0;
    facingLeft = 0;
}

// Add a speed in 1/16 pixels to a sub-pixel accumulator, returning whole pixels to move
static int8_t subPixelStep(uint8_t *sub, int16_t speed) {
    int16_t total = (int16_t)*sub + speed;
    *sub = total & 15;
    return (int8_t)(total >> 4);
}

// Move by dx pixels (|dx| < 8), checking only the leading edge and snapping
// flush against a wall if one is hit
static void moveX(int8_t dx) {
    int16_t top = mario.y;
    int16_t bottom = top + mario.height - 1;
    int16_t edge;

    if (dx > 0) {
        edge = mario.x + mario.width - 1 + dx;
        if (levelColumnSolid(edge >> 3, top, bottom)) {
            mario.x = ((edge >> 3) << 3) - mario.width;
            subX = 0;
        } else {
            mario.x += dx;
        }
    } else if (dx < 0) {
        edge = mario.x + dx;
        if (levelColumnSolid(edge >> 3, top, bottom)) {
            mario.x = ((edge >> 3) + 1) << 3;
            subX = 0;
        } else {
            mario.x += dx;
        }
    }
}

static void moveY(int8_t dy) {
    int16_t left = mario.x;
    int16_t right = left + mario.width - 1;
    int16_t edge;

    if (dy > 0) {
        edge = mario.y + mario.height - 1 + dy;
        if (levelRowSolid(edge >> 3, left, right)) {
            mario.y = ((edge >> 3) << 3) - mario.height; // landed
            velocityY = 0;
            subY = 0;
        } else {
            mario.y += dy;
        }
    } else if (dy < 0) {
        edge = mario.y + dy;
        if (levelRowSolid(edge >> 3, left, right)) {
            mario.y = ((edge >> 3) + 1) << 3;            // bumped head
            velocityY = 0;
            subY = 0;
        } else {
            mario.y += dy;
        }
    }
}

static void applyGravity(void) {
    int16_t feet;

    velocityY += GRAVITY;
    if (velocityY > MAX_FALL_SPEED) velocityY = MAX_FALL_SPEED;
    moveY(subPixelStep(&subY, velocityY));

    // Standing if the row directly under the feet is solid
    feet = mario.y + mario.height;
    onGround = ((feet & 7) == 0) && velocityY >= 0 &&
               levelRowSolid(feet >> 3, mario.x, mario.x + mario.width - 1);
    if (onGround) {
        velocityY = 0;
        subY = 0;
    }
}

void playerUpdate(uint8_t input) {
    if ((input & J_A) && onGround) {
        velocityY = JUMP_STRENGTH;
        onGround = 0;
    }

    moving = 0;
    if (input & J_LEFT) {
        facingLeft = 1;
        moving = 1;
        moveX(subPixelStep(&subX, -WALK_SPEED));
    } else if (input & J_RIGHT) {
        facingLeft = 0;
        moving = 1;
        moveX(subPixelStep(&subX, WALK_SPEED));
    }

    applyGravity();

    if (moving && onGround) {
        if (++walkTimer >= WALK_ANIM_FRAMES) {
            walkTimer = 0;
            if (++walkFrame >= 3) walkFrame = 0;
        }
    }
}

void playerDraw(void) {
    const metasprite_t *frame;
    uint8_t sx = mario.x - cameraX + SPRITE_OFFSET_X;
    uint8_t sy = mario.y - cameraY + SPRITE_OFFSET_Y;

    if (!onGround)   frame = mario_jump_metasprite;
    else if (moving) frame = walkFrames[walkFrame];
    else             frame = mario_metasprite;

    if (facingLeft) move_metasprite_flipx(frame, 0, 0, 0, sx + mario.width, sy);
    else            move_metasprite_ex(frame, 0, 0, 0, sx, sy);
}

uint8_t playerFellOut(void) {
    return mario.y > LEVEL_HEIGHT;
}
