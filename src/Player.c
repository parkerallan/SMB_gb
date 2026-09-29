#include <gb/gb.h>
#include "Util.h"
#include <gb/metasprites.h>
#include "Player.h"
#include "Physics.h"
#include "Level.h"
#include "Camera.h"
#include "Blocks.h"
#include "MarioTiles.h"
#include "BigMarioTiles.h"
#include "Metasprites.h"
#include "Sprites.h"

#define JUMP_STRENGTH -80
#define STOMP_BOUNCE -64
#define WALK_SPEED 24
#define WALK_ANIM_FRAMES 6
#define MARIO_START_X 40
#define MARIO_START_Y 192
#define SMALL_HEIGHT 16
#define BIG_HEIGHT 32
// Growing and shrinking: the game pauses while Mario flashes between sizes
#define CHANGE_SIZE_FRAMES 60
#define CHANGE_SIZE_FLASH_FRAMES 6
// After shrinking, enemies can't hurt Mario for a while (he flickers)
#define INVINCIBLE_FRAMES 120
// Death: freeze, then hop up and fall off the bottom of the screen
#define DEATH_FREEZE_FRAMES 30
#define DEATH_JUMP -64
#define DEATH_FALL_BELOW 24 // pixels below the screen before the death is over

// Big Mario's frames in video memory, 8 tiles each (see playerInit)
#define BIG_STAND  0
#define BIG_WALK1  8
#define BIG_WALK2  16
#define BIG_WALK3  24
#define BIG_SKID   32
#define BIG_JUMP   40
#define BIG_CROUCH 48

struct GameCharacter mario;

static int16_t velocityY;
static uint8_t subX, subY;
static uint8_t onGround;
static uint8_t moving;
static uint8_t crouching;
static uint8_t facingLeft;
static uint8_t walkTimer, walkFrame;
static uint8_t big;
static uint8_t sizeTimer;       // growing or shrinking
static uint8_t invincibleTimer;
static uint8_t dying;
static uint8_t deathTimer;

static const metasprite_t* const walkFrames[] = {mario_walk_frame1, mario_walk_frame2, mario_walk_frame3};
static const uint8_t bigWalkFrames[] = {BIG_WALK1, BIG_WALK2, BIG_WALK3};

void playerInit(void) {
    bankedSetSpriteData(SPR_TILE_MARIO, SPR_SMALL_MARIO_TILES, MarioTiles, BANK(MarioTiles));
    bankedSetSpriteData(SPR_TILE_MARIO_DEAD, 4, MarioTiles + MARIOTILES_DEAD * 16, BANK(MarioTiles));
    // stand, walk x3, skid and jump are consecutive in the sheet; crouch goes after them
    bankedSetSpriteData(SPR_TILE_BIG_MARIO + BIG_STAND, BIGMARIOTILES_SWIM1, BigMarioTiles, BANK(BigMarioTiles));
    bankedSetSpriteData(SPR_TILE_BIG_MARIO + BIG_CROUCH, 8, BigMarioTiles + BIGMARIOTILES_CROUCH * 16,
                        BANK(BigMarioTiles));
}

void playerReset(void) {
    mario.x = MARIO_START_X;
    mario.y = MARIO_START_Y;
    mario.width = 16;
    mario.height = SMALL_HEIGHT;
    velocityY = 0;
    subX = 0;
    subY = 0;
    onGround = 0;
    crouching = 0;
    facingLeft = 0;
    big = 0;
    sizeTimer = 0;
    invincibleTimer = 0;
    dying = 0;
}

void playerGrow(void) {
    if (big) return;
    big = 1;
    mario.y -= BIG_HEIGHT - SMALL_HEIGHT; // grow upward, feet stay put
    mario.height = BIG_HEIGHT;
    sizeTimer = CHANGE_SIZE_FRAMES;
}

uint8_t playerIsBig(void) {
    return big;
}

uint8_t playerIsChangingSize(void) {
    return sizeTimer != 0;
}

void playerHurt(void) {
    if (invincibleTimer || sizeTimer || dying) return;
    if (!big) {
        playerDie();
        return;
    }
    big = 0;
    crouching = 0;
    mario.y += BIG_HEIGHT - SMALL_HEIGHT; // shrink toward the feet
    mario.height = SMALL_HEIGHT;
    sizeTimer = CHANGE_SIZE_FRAMES;
    invincibleTimer = INVINCIBLE_FRAMES;
}

uint8_t playerIsInvincible(void) {
    return invincibleTimer != 0;
}

void playerDie(void) {
    if (dying) return;
    if (big) {
        big = 0;
        mario.y += BIG_HEIGHT - SMALL_HEIGHT;
        mario.height = SMALL_HEIGHT;
    }
    dying = 1;
    deathTimer = 0;
    velocityY = 0;
    subY = 0;
}

uint8_t playerIsDying(void) {
    return dying;
}

uint8_t playerDeathFinished(void) {
    return dying && mario.y > cameraY + 144 + DEATH_FALL_BELOW;
}

void playerBounce(void) {
    velocityY = STOMP_BOUNCE;
    subY = 0;
    onGround = 0;
}

uint8_t playerIsFalling(void) {
    return velocityY > 0;
}

uint8_t playerOnGround(void) {
    return onGround;
}

// Bumped a solid row from below: hit the block over Mario's center, like SMB,
// or the one under whichever edge is touching
static void hitBlockAbove(void) {
    int16_t ty = (mario.y >> 3) - 1;
    int16_t left = mario.x;
    int16_t right = left + mario.width - 1;
    int16_t tx = (left + (mario.width >> 1)) >> 3;
    if (!levelTileSolid(tx, ty)) {
        tx = levelTileSolid(left >> 3, ty) ? (left >> 3) : (right >> 3);
    }
    blocksHit(tx, ty);
}

static void applyGravity(void) {
    int8_t dy;

    velocityY += PHYSICS_GRAVITY;
    if (velocityY > PHYSICS_MAX_FALL_SPEED) velocityY = PHYSICS_MAX_FALL_SPEED;
    dy = physicsSubPixelStep(&subY, velocityY);
    if (physicsMoveY(&mario, dy)) {
        if (dy < 0) hitBlockAbove();
        velocityY = 0;
        subY = 0;
    }

    onGround = velocityY >= 0 && physicsOnGround(&mario);
    if (onGround) {
        velocityY = 0;
        subY = 0;
    }
}

static void walk(int16_t speed) {
    moving = 1;
    if (physicsMoveX(&mario, physicsSubPixelStep(&subX, speed))) subX = 0;
}

// Pop up, then fall straight through everything
static void updateDeath(void) {
    if (deathTimer < DEATH_FREEZE_FRAMES) {
        if (++deathTimer == DEATH_FREEZE_FRAMES) velocityY = DEATH_JUMP;
        return;
    }
    velocityY += PHYSICS_GRAVITY;
    if (velocityY > PHYSICS_MAX_FALL_SPEED) velocityY = PHYSICS_MAX_FALL_SPEED;
    mario.y += physicsSubPixelStep(&subY, velocityY);
}

void playerUpdate(uint8_t input) {
    if (dying) {
        updateDeath();
        return;
    }
    if (sizeTimer) {
        sizeTimer--;
        return;
    }
    if (invincibleTimer) invincibleTimer--;

    if ((input & J_A) && onGround) {
        velocityY = JUMP_STRENGTH;
        onGround = 0;
    }

    // Big Mario ducks while Down is held on the ground, and can't walk while ducking
    crouching = big && onGround && (input & J_DOWN);

    moving = 0;
    if (!crouching) {
        if (input & J_LEFT) {
            facingLeft = 1;
            walk(-WALK_SPEED);
        } else if (input & J_RIGHT) {
            facingLeft = 0;
            walk(WALK_SPEED);
        }
    }

    applyGravity();

    if (moving && onGround) {
        if (++walkTimer >= WALK_ANIM_FRAMES) {
            walkTimer = 0;
            if (++walkFrame >= 3) walkFrame = 0;
        }
    }
}

// 16x16 frames (mario_metasprite's layout) at `firstTile`
static void drawSmall(const metasprite_t *frame, uint8_t firstTile, uint8_t sx, uint8_t sy) {
    uint8_t used;
    if (facingLeft) used = move_metasprite_flipx(frame, firstTile, 0, spritesNext(), sx + mario.width, sy);
    else            used = move_metasprite_ex(frame, firstTile, 0, spritesNext(), sx, sy);
    spritesAdvance(used);
}

static void drawBig(uint8_t bigFrame, uint8_t sx, uint8_t sy) {
    uint8_t tile = SPR_TILE_BIG_MARIO + bigFrame;
    uint8_t used;
    if (facingLeft) used = move_metasprite_flipx(big_mario_metasprite, tile, 0, spritesNext(), sx + mario.width, sy);
    else            used = move_metasprite_ex(big_mario_metasprite, tile, 0, spritesNext(), sx, sy);
    spritesAdvance(used);
}

void playerDraw(void) {
    uint8_t sx = mario.x - cameraX + SPRITE_OFFSET_X;
    uint8_t sy = mario.y - cameraY + SPRITE_OFFSET_Y;

    if (dying) {
        if (mario.y < cameraY + 144) drawSmall(mario_metasprite, SPR_TILE_MARIO_DEAD, sx, sy);
        return;
    }
    if (sizeTimer) {
        // Flash between small (standing at the same feet position) and big.
        // `big` already says which size he's becoming.
        uint8_t showBig = ((sizeTimer / CHANGE_SIZE_FLASH_FRAMES) & 1) ? !big : big;
        if (showBig) drawBig(BIG_STAND, sx, big ? sy : sy - (BIG_HEIGHT - SMALL_HEIGHT));
        else         drawSmall(mario_metasprite, SPR_TILE_MARIO, sx, big ? sy + (BIG_HEIGHT - SMALL_HEIGHT) : sy);
        return;
    }
    // Flicker while invincible
    if (invincibleTimer & 2) return;

    if (big) {
        if (!onGround)      drawBig(BIG_JUMP, sx, sy);
        else if (crouching) drawBig(BIG_CROUCH, sx, sy);
        else if (moving)    drawBig(bigWalkFrames[walkFrame], sx, sy);
        else                drawBig(BIG_STAND, sx, sy);
    } else {
        if (!onGround)      drawSmall(mario_jump_metasprite, SPR_TILE_MARIO, sx, sy);
        else if (moving)    drawSmall(walkFrames[walkFrame], SPR_TILE_MARIO, sx, sy);
        else                drawSmall(mario_metasprite, SPR_TILE_MARIO, sx, sy);
    }
}

uint8_t playerFellOut(void) {
    return mario.y > LEVEL_HEIGHT;
}
