#include <gb/gb.h>
#include "Util.h"
#include <gb/metasprites.h>
#include "Player.h"
#include "Physics.h"
#include "Level.h"
#include "Camera.h"
#include "Blocks.h"
#include "Fireball.h"
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
// Fire Mario: throws a fireball per press of B, showing the throw frame briefly
#define THROW_FRAMES 8
#define FIREBALL_HAND_X 12 // where the fireball leaves his hand, facing right
#define FIREBALL_HAND_Y 8
// Star: invincible for 10 seconds, flashing colors, slower for the last 2
#define STAR_FRAMES 600
#define STAR_SLOW_FRAMES 120

// Mario's colors. Normal Mario uses sprite palette 0 like everything else;
// fire and star Mario use palette 1, which only Mario uses. In MarioTiles the
// shades stand for SMB's red (2), skin (1) and brown (3); fire Mario's red
// parts turn white and his brown parts red, like SMB.
#define PALETTE_FIRE DMG_PALETTE(DMG_WHITE, DMG_LITE_GRAY, DMG_WHITE, DMG_DARK_GRAY)
static const uint8_t starPalettes[] = {
    DMG_PALETTE(DMG_WHITE, DMG_LITE_GRAY, DMG_DARK_GRAY, DMG_BLACK), // normal
    PALETTE_FIRE,
    DMG_PALETTE(DMG_WHITE, DMG_DARK_GRAY, DMG_BLACK, DMG_WHITE),
    DMG_PALETTE(DMG_WHITE, DMG_BLACK, DMG_LITE_GRAY, DMG_DARK_GRAY),
};

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
static uint8_t fire;
static uint8_t becomingFire;    // sizeTimer is running for the flower, not a size change
static uint8_t throwTimer;
static uint8_t lastInput;
static uint16_t starTimer;
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
    bankedSetSpriteData(SPR_TILE_BIG_FIRE, 8, BigMarioTiles + BIGMARIOTILES_FIRE * 16, BANK(BigMarioTiles));
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
    fire = 0;
    becomingFire = 0;
    throwTimer = 0;
    lastInput = 0;
    starTimer = 0;
    dying = 0;
}

void playerGrow(void) {
    if (big) return;
    big = 1;
    mario.y -= BIG_HEIGHT - SMALL_HEIGHT; // grow upward, feet stay put
    mario.height = BIG_HEIGHT;
    sizeTimer = CHANGE_SIZE_FRAMES;
}

void playerFire(void) {
    if (!big) {
        playerGrow(); // a flower makes small Mario big, like a mushroom
        return;
    }
    if (fire) return;
    fire = 1;
    becomingFire = 1;
    sizeTimer = CHANGE_SIZE_FRAMES; // the game pauses while he flashes
}

void playerStar(void) {
    starTimer = STAR_FRAMES;
}

uint8_t playerHasStar(void) {
    return starTimer != 0;
}

uint8_t playerIsBig(void) {
    return big;
}

uint8_t playerIsChangingSize(void) {
    return sizeTimer != 0;
}

void playerHurt(void) {
    if (invincibleTimer || starTimer || sizeTimer || dying) return;
    if (!big) {
        playerDie();
        return;
    }
    big = 0;
    fire = 0; // fire Mario goes straight back to small, like SMB
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
    fire = 0;
    starTimer = 0;
    sizeTimer = 0;
    becomingFire = 0;
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
        if (!--sizeTimer) becomingFire = 0;
        return;
    }
    if (invincibleTimer) invincibleTimer--;
    if (starTimer) starTimer--;
    if (throwTimer) throwTimer--;

    // Fire Mario throws a fireball each time B is pressed
    if (fire && (input & J_B) && !(lastInput & J_B) && !crouching &&
        fireballThrow(facingLeft ? mario.x + (16 - FIREBALL_HAND_X - 8) : mario.x + FIREBALL_HAND_X,
                      mario.y + FIREBALL_HAND_Y, facingLeft)) {
        throwTimer = THROW_FRAMES;
    }
    lastInput = input;

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

static uint8_t palette; // S_PALETTE while fire or star Mario, else 0

// 16x16 frames (mario_metasprite's layout) at `firstTile`
static void drawSmall(const metasprite_t *frame, uint8_t firstTile, uint8_t sx, uint8_t sy) {
    uint8_t used;
    if (facingLeft) used = move_metasprite_flipx(frame, firstTile, palette, spritesNext(), sx + mario.width, sy);
    else            used = move_metasprite_ex(frame, firstTile, palette, spritesNext(), sx, sy);
    spritesAdvance(used);
}

// 16x32 frames (big_mario_metasprite's layout) starting at sprite tile `tile`
static void drawBig(uint8_t tile, uint8_t sx, uint8_t sy) {
    uint8_t used;
    if (facingLeft) used = move_metasprite_flipx(big_mario_metasprite, tile, palette, spritesNext(), sx + mario.width, sy);
    else            used = move_metasprite_ex(big_mario_metasprite, tile, palette, spritesNext(), sx, sy);
    spritesAdvance(used);
}

// Fire colors, or the star's flashing colors
static void choosePalette(void) {
    palette = 0;
    if (starTimer) {
        OBP1_REG = starPalettes[(starTimer >> (starTimer < STAR_SLOW_FRAMES ? 3 : 1)) & 3];
        palette = S_PALETTE;
    } else if (fire) {
        // flashes while turning into fire Mario
        if (!becomingFire || ((sizeTimer >> 2) & 1)) {
            OBP1_REG = PALETTE_FIRE;
            palette = S_PALETTE;
        }
    }
}

void playerDraw(void) {
    uint8_t sx = mario.x - cameraX + SPRITE_OFFSET_X;
    uint8_t sy = mario.y - cameraY + SPRITE_OFFSET_Y;

    choosePalette();
    if (dying) {
        if (mario.y < cameraY + 144) drawSmall(mario_metasprite, SPR_TILE_MARIO_DEAD, sx, sy);
        return;
    }
    if (sizeTimer) {
        // Flash between small (standing at the same feet position) and big.
        // `big` already says which size he's becoming.
        uint8_t showBig = becomingFire || (((sizeTimer / CHANGE_SIZE_FLASH_FRAMES) & 1) ? !big : big);
        if (showBig) drawBig(SPR_TILE_BIG_MARIO + BIG_STAND, sx, big ? sy : sy - (BIG_HEIGHT - SMALL_HEIGHT));
        else         drawSmall(mario_metasprite, SPR_TILE_MARIO, sx, big ? sy + (BIG_HEIGHT - SMALL_HEIGHT) : sy);
        return;
    }
    // Flicker while invincible
    if (invincibleTimer & 2) return;

    if (big) {
        if (throwTimer)     drawBig(SPR_TILE_BIG_FIRE, sx, sy);
        else if (!onGround) drawBig(SPR_TILE_BIG_MARIO + BIG_JUMP, sx, sy);
        else if (crouching) drawBig(SPR_TILE_BIG_MARIO + BIG_CROUCH, sx, sy);
        else if (moving)    drawBig(SPR_TILE_BIG_MARIO + bigWalkFrames[walkFrame], sx, sy);
        else                drawBig(SPR_TILE_BIG_MARIO + BIG_STAND, sx, sy);
    } else {
        if (!onGround)      drawSmall(mario_jump_metasprite, SPR_TILE_MARIO, sx, sy);
        else if (moving)    drawSmall(walkFrames[walkFrame], SPR_TILE_MARIO, sx, sy);
        else                drawSmall(mario_metasprite, SPR_TILE_MARIO, sx, sy);
    }
}

uint8_t playerFellOut(void) {
    return mario.y > LEVEL_HEIGHT;
}
