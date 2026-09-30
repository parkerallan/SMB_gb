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

// Mario moves like SMB, with its numbers. Speeds are in 1/256 pixels per
// frame (SMB's units: its 1/16-pixel speeds times 16).
// Walking and running (B held): top speed, and how fast he speeds up (and
// slows down again when nothing is pressed); twice that when skidding
#define WALK_MAX 384            // 1.5 pixels a frame
#define RUN_MAX 640             // 2.5
#define WALK_ACCEL 10
#define RUN_ACCEL 14
#define RUN_KEEP_FRAMES 10      // still running this long after B is let go
// Jumping: how hard, and gravity while A is held on the way up (so holding it
// jumps higher) or otherwise, by how fast he was going when he jumped
static const int16_t jumpSpeedAt[] = {-1024, -1024, -1024, -1280, -1280};
static const uint8_t jumpGravityAt[] = {32, 32, 30, 40, 40};
static const uint8_t fallGravityAt[] = {112, 112, 96, 144, 144};
static const int16_t jumpSpeedLimits[] = {9 * 16, 16 * 16, 25 * 16, 28 * 16}; // the steps between those
#define MAX_FALL_SPEED 1024     // 4 pixels a frame
#define STOMP_BOUNCE -1024
// Walking animation: frames per step, quicker the faster he goes
#define ANIM_FAST_SPEED (28 * 16)
#define ANIM_MEDIUM_SPEED (14 * 16)
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
#define DEATH_JUMP -1024
#define DEATH_GRAVITY 48
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

static int16_t speedX, velocityY;  // 1/256 pixels a frame
static uint8_t subX, subY;         // and the pixel fractions
static uint8_t gravityIndex;       // which of the *At[] tables' columns: set by the last jump
static uint8_t jumping;            // off the ground by jumping (A's hold matters)
static uint8_t airMax;             // top speed in the air: 1 if he jumped running
static uint8_t runTimer;
static uint8_t onGround;
static uint8_t moving;
static uint8_t skidding;
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
static uint8_t pose;
static uint8_t dying;
static uint8_t deathTimer;

static const metasprite_t* const walkFrames[] = {mario_walk_frame1, mario_walk_frame2, mario_walk_frame3};
static const uint8_t bigWalkFrames[] = {BIG_WALK1, BIG_WALK2, BIG_WALK3};

// Fire Mario's throwing frame
static void loadThrowFrame(void) {
    bankedSetSpriteData(SPR_TILE_BIG_FIRE, 8, BigMarioTiles + BIGMARIOTILES_FIRE * 16, BANK(BigMarioTiles));
}

void playerInit(void) {
    bankedSetSpriteData(SPR_TILE_MARIO, SPR_SMALL_MARIO_TILES, MarioTiles, BANK(MarioTiles));
    bankedSetSpriteData(SPR_TILE_MARIO_DEAD, 4, MarioTiles + MARIOTILES_DEAD * 16, BANK(MarioTiles));
    // stand, walk x3, skid and jump are consecutive in the sheet; crouch goes after them
    bankedSetSpriteData(SPR_TILE_BIG_MARIO + BIG_STAND, BIGMARIOTILES_SWIM1, BigMarioTiles, BANK(BigMarioTiles));
    bankedSetSpriteData(SPR_TILE_BIG_MARIO + BIG_CROUCH, 8, BigMarioTiles + BIGMARIOTILES_CROUCH * 16,
                        BANK(BigMarioTiles));
    bankedSetSpriteData(SPR_TILE_SMALL_SKID, 4, MarioTiles + MARIOTILES_SKID * 16, BANK(MarioTiles));
    loadThrowFrame();
}

// Mario starts each level with the power-ups he finished the last one with
// (dying already took them away)
void playerReset(void) {
    loadThrowFrame(); // the level end borrows its tiles for the climbing frame
    mario.x = MARIO_START_X;
    mario.width = 16;
    mario.height = big ? BIG_HEIGHT : SMALL_HEIGHT;
    mario.y = MARIO_START_Y + SMALL_HEIGHT - mario.height;
    speedX = 0;
    velocityY = 0;
    subX = 0;
    subY = 0;
    gravityIndex = 0;
    jumping = 0;
    runTimer = 0;
    onGround = 0;
    skidding = 0;
    crouching = 0;
    facingLeft = 0;
    sizeTimer = 0;
    invincibleTimer = 0;
    becomingFire = 0;
    throwTimer = 0;
    lastInput = 0;
    starTimer = 0;
    dying = 0;
    pose = POSE_NORMAL;
}

void playerGrow(void) {
    if (big) return;
    big = 1;
    mario.y -= BIG_HEIGHT - SMALL_HEIGHT; // grow upward, feet stay put
    mario.height = BIG_HEIGHT;
    sizeTimer = CHANGE_SIZE_FRAMES;
}

void playerSetPose(uint8_t newPose) {
    pose = newPose;
    if (pose == POSE_POLE) {
        // the climbing frame goes where the throwing frame was (it's not needed
        // at the flagpole); playerReset puts the throwing frame back
        if (big) bankedSetSpriteData(SPR_TILE_BIG_FIRE, 8, BigMarioTiles + BIGMARIOTILES_CLIMB1 * 16, BANK(BigMarioTiles));
        else     bankedSetSpriteData(SPR_TILE_BIG_FIRE, 4, MarioTiles + MARIOTILES_CLIMB1 * 16, BANK(MarioTiles));
        speedX = 0;
        velocityY = 0;
        subX = subY = 0;
        starTimer = 0;
        invincibleTimer = 0;
        throwTimer = 0;
    } else if (pose == POSE_PIPE) {
        speedX = 0;
        velocityY = 0;
        subY = 0;
        skidding = 0;
        onGround = 1;
        moving = 0;
        crouching = 0;
        throwTimer = 0;
    }
}

void playerFaceLeft(uint8_t left) {
    facingLeft = left;
}

static void animateWalk(void) {
    uint16_t speed = speedX < 0 ? -speedX : speedX;
    uint8_t frames = speed >= ANIM_FAST_SPEED ? 2 : speed >= ANIM_MEDIUM_SPEED ? 4 : 7;
    if (++walkTimer >= frames) {
        walkTimer = 0;
        if (++walkFrame >= 3) walkFrame = 0;
    }
}

void playerAnimateWalk(void) {
    moving = 1;
    animateWalk();
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
    // holding A bounces higher, as when jumping
    velocityY = STOMP_BOUNCE;
    subY = 0;
    jumping = 1;
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

// Hidden blocks aren't solid, but one appears when Mario's head goes up into
// it from below (the block over the middle of his head, like SMB)
static void hitHiddenBlock(int8_t dy) {
    int16_t by = mario.y >> 4;
    int16_t bx = (mario.x + 8) >> 4;
    uint8_t block;
    if (((mario.y - dy) >> 4) == by) return; // his head was already in this row
    block = levelBlockAt(bx, by);
    if (block != SCENERY_HIDDEN_1UP && block != SCENERY_HIDDEN_COIN) return;
    mario.y = (by + 1) << 4; // stop under it, like any ceiling
    velocityY = 0;
    subY = 0;
    blocksHit(bx << 1, by << 1);
}

// Whole pixels to move this frame for a speed in 1/256 pixels, keeping the fraction
static int8_t step(uint8_t *sub, int16_t speed) {
    int16_t total = *sub + speed;
    *sub = (uint8_t)total;
    return (int8_t)(total >> 8);
}

static void applyGravity(uint8_t input) {
    int8_t dy;

    // Move first, then gravity changes the speed, like SMB (the other way
    // round, a full jump falls short of 1-1's 4-block stairs)
    dy = step(&subY, velocityY);
    // light gravity while he's still going up with A held
    if (jumping && velocityY < 0 && (input & J_A)) velocityY += jumpGravityAt[gravityIndex];
    else                                            velocityY += fallGravityAt[gravityIndex];
    if (velocityY > MAX_FALL_SPEED) velocityY = MAX_FALL_SPEED;
    if (physicsMoveY(&mario, dy)) {
        if (dy < 0) hitBlockAbove();
        velocityY = 0;
        subY = 0;
    } else if (dy < 0) {
        hitHiddenBlock(dy);
    }

    onGround = velocityY >= 0 && physicsOnGround(&mario);
    if (onGround) {
        velocityY = 0;
        subY = 0;
        jumping = 0;
    }
}

// Speed up toward the direction held (up to the walking or running top speed),
// slow down when nothing's held, skid when turning around on the ground
static void updateSpeed(uint8_t input) {
    int8_t dir = (input & J_LEFT) ? -1 : (input & J_RIGHT) ? 1 : 0;
    uint8_t running = runTimer != 0;
    int16_t max = (onGround ? running : airMax) ? RUN_MAX : WALK_MAX;
    uint8_t accel = running ? RUN_ACCEL : WALK_ACCEL;

    if (crouching) dir = 0; // big Mario ducking just slides to a stop
    skidding = 0;
    if (dir && onGround) facingLeft = dir < 0; // he only turns round on the ground
    if (dir > 0) {
        if (speedX < 0) {
            if (onGround) skidding = 1;
            speedX += accel << 1;
        } else if (speedX < max) {
            speedX += accel;
            if (speedX > max) speedX = max;
        } else if (onGround) {
            speedX -= accel; // was running and let go of B
        }
    } else if (dir < 0) {
        if (speedX > 0) {
            if (onGround) skidding = 1;
            speedX -= accel << 1;
        } else if (speedX > -max) {
            speedX -= accel;
            if (speedX < -max) speedX = -max;
        } else if (onGround) {
            speedX += accel;
        }
    } else if (onGround) {
        // friction
        if (speedX > WALK_ACCEL) speedX -= WALK_ACCEL;
        else if (speedX < -WALK_ACCEL) speedX += WALK_ACCEL;
        else speedX = 0;
    }
}

static void moveX(void) {
    if (physicsMoveX(&mario, step(&subX, speedX))) {
        speedX = 0; // walked into a wall
        subX = 0;
    }
}

// Speed index for jumping: faster runs jump higher (see jumpSpeedAt)
static uint8_t jumpIndex(void) {
    uint16_t speed = speedX < 0 ? -speedX : speedX;
    uint8_t i = 0;
    while (i < sizeof(jumpSpeedLimits) / sizeof(jumpSpeedLimits[0]) && speed >= (uint16_t)jumpSpeedLimits[i]) i++;
    return i;
}

// Pop up, then fall straight through everything
static void updateDeath(void) {
    if (deathTimer < DEATH_FREEZE_FRAMES) {
        if (++deathTimer == DEATH_FREEZE_FRAMES) velocityY = DEATH_JUMP;
        return;
    }
    velocityY += DEATH_GRAVITY;
    if (velocityY > MAX_FALL_SPEED) velocityY = MAX_FALL_SPEED;
    mario.y += step(&subY, velocityY);
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

    // Running: B held on the ground (and a moment after letting go)
    if ((input & J_B) && onGround) runTimer = RUN_KEEP_FRAMES;
    else if (runTimer) runTimer--;

    // Big Mario ducks while Down is held on the ground
    crouching = big && onGround && (input & J_DOWN);

    // A new press of A jumps: how high depends on his speed and how long A is held
    if ((input & J_A) && !(lastInput & J_A) && onGround) {
        gravityIndex = jumpIndex();
        velocityY = jumpSpeedAt[gravityIndex];
        subY = 0;
        jumping = 1;
        airMax = runTimer != 0 || (speedX > WALK_MAX || speedX < -WALK_MAX);
        onGround = 0;
        crouching = 0;
    }
    lastInput = input;

    updateSpeed(input);
    moveX();
    applyGravity(input);

    moving = speedX != 0;
    if (moving && onGround) animateWalk();
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
    if (pose == POSE_HIDDEN) return;
    if (pose == POSE_PIPE) palette |= S_PRIORITY; // behind the pipe
    if (pose == POSE_POLE) {
        if (big) drawBig(SPR_TILE_BIG_FIRE, sx, sy);
        else     drawSmall(mario_metasprite, SPR_TILE_BIG_FIRE, sx, sy);
        return;
    }
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
        else if (skidding)  drawBig(SPR_TILE_BIG_MARIO + BIG_SKID, sx, sy);
        else if (moving)    drawBig(SPR_TILE_BIG_MARIO + bigWalkFrames[walkFrame], sx, sy);
        else                drawBig(SPR_TILE_BIG_MARIO + BIG_STAND, sx, sy);
    } else {
        if (!onGround)      drawSmall(mario_jump_metasprite, SPR_TILE_MARIO, sx, sy);
        else if (skidding)  drawSmall(mario_metasprite, SPR_TILE_SMALL_SKID, sx, sy);
        else if (moving)    drawSmall(walkFrames[walkFrame], SPR_TILE_MARIO, sx, sy);
        else                drawSmall(mario_metasprite, SPR_TILE_MARIO, sx, sy);
    }
}

uint8_t playerFellOut(void) {
    return mario.y > LEVEL_HEIGHT;
}
