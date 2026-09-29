// Level objects are banked code: ROM bank 3 (see make.bat)
#pragma bank 3

#include <gb/gb.h>
#include "Util.h"
#include "Flagpole.h"
#include "Level.h"
#include "Player.h"
#include "Game.h"
#include "Popup.h"
#include "ItemTiles.h"
#include "Sprites.h"

#define SLIDE_SPEED 2            // pixels per frame, Mario and the flag
#define TURN_FRAMES 30           // Mario hangs on the far side of the pole
#define BONUS_PAUSE_FRAMES 30    // before and after the time bonus
#define CASTLE_FLAG_RISE 16      // pixels, half a pixel per frame
#define FIREWORK_FRAMES 18       // small, medium, large explosion, 6 frames each
#define FIREWORK_GAP 16          // then a pause before the next one
#define FIREWORK_POINTS 500
#define FINISH_FRAMES 90         // before the next level
#define POLE_GRAB_X 14           // Mario's x relative to the pole: hands on it from the left...
#define POLE_OTHER_SIDE_X 2      // ...then from the right

enum { WAITING, SLIDING, TURNING, WALKING, BONUS, CASTLE_FLAG, FIREWORKS, FINISHED };

static uint8_t hasPole, state;
static uint8_t timer;
static int16_t poleX;                 // the pole's center
static int16_t poleTop, poleBottom;   // top of the pole under the ball; top of the block under it
static int16_t flagX, flagY;          // top-left of the 16x16 flag, in level pixels
static int16_t doorX;                 // Mario disappears here (the castle door)
static int16_t castleFlagX, castleFlagY, castleFlagTop;
static uint8_t fireworks, fireworksLeft;

// Flag points by how high Mario grabbed the pole (his feet above its bottom), like SMB
static const uint8_t grabHeights[] = {128, 96, 56, 24, 0};
static const uint16_t grabPoints[] = {5000, 2000, 800, 400, 100};

// Where fireworks go off, relative to the castle flag: in the sky around the
// castle (the Game Boy's screen shows much less sky above it than the NES's)
static const int8_t fireworkX[] = {-48, 56, -20, 72, -40, 40};
static const int8_t fireworkY[] = {8, 0, 4, 24, 24, 32};

void flagpoleInit(void) BANKED {
    bankedSetSpriteData(SPR_TILE_FLAG, 4, ItemTiles + ITEMTILES_FLAG * 16, BANK(ItemTiles));
    bankedSetSpriteData(SPR_TILE_CASTLE_FLAG, 4, ItemTiles + ITEMTILES_CASTLE_FLAG * 16, BANK(ItemTiles));
}

void flagpoleReset(void) BANKED {
    int16_t bx, by;
    uint8_t block;
    hasPole = 0;
    state = WAITING;
    doorX = LEVEL_WIDTH;
    // The pole and the castle are at the end of the level: look through it
    // column by column from the right, and stop after the pole's column
    // (checking every block of the level took about a second)
    for (bx = levelWidth - 1; bx >= 0 && !hasPole; bx--) {
        for (by = 0; by < LEVEL_BLOCK_ROWS; by++) {
            block = levelBlockAt(bx, by);
            if (block == SCENERY_FLAGPOLE_BALL) {
                poleX = (bx << 4) + 8;
                poleTop = (by + 1) << 4;
                // Hangs on the left of the pole, just under the ball
                flagX = poleX - 16;
                flagY = poleTop;
                hasPole = 1;
            } else if (block == SCENERY_FLAGPOLE) {
                poleBottom = (by + 1) << 4;
            } else if (block == SCENERY_CASTLE_DOOR) {
                doorX = bx << 4;
            } else if (block == SCENERY_CASTLE_DOOR_TOP) {
                // the star flag rises out of the castle's top (3 blocks above
                // the door top), over the door
                castleFlagX = (bx << 4) + 6; // its pole is its left edge: center it
                castleFlagTop = ((by - 3) << 4) - CASTLE_FLAG_RISE;
            }
        }
    }
}

uint8_t flagpoleEnding(void) BANKED {
    return state != WAITING;
}

void flagpoleCheck(void) BANKED {
    int16_t height;
    uint8_t i;
    uint16_t points;
    if (!hasPole || state != WAITING || playerIsDying()) return;
    // hands reach the pole
    if (mario.x + POLE_GRAB_X < poleX || mario.x > poleX || mario.y >= poleBottom) return;

    if (mario.y < poleTop) mario.y = poleTop;
    height = poleBottom - (mario.y + mario.height);
    if (height < 0) height = 0;
    for (i = 0; (uint8_t)height < grabHeights[i]; i++) {}
    points = grabPoints[i];
    gameAddScore(points);
    popupShow(poleX + 4, mario.y, points);

    // SMB's fireworks: one per the clock's last digit when it's 1, 3 or 6
    i = (uint8_t)(gameTimeLeft() % 10);
    fireworks = (i == 1 || i == 3 || i == 6) ? i : 0;

    mario.x = poleX - POLE_GRAB_X;
    playerFaceLeft(0);
    playerSetPose(POSE_POLE);
    state = SLIDING;
}

// Returns 1 once the whole ending is over
uint8_t flagpoleUpdate(void) BANKED {
    uint8_t done;
    switch (state) {
    case SLIDING:
        // Mario and the flag slide down together; the flag goes all the way
        done = 1;
        if (mario.y + mario.height < poleBottom) {
            mario.y += SLIDE_SPEED;
            if (mario.y + mario.height > poleBottom) mario.y = poleBottom - mario.height;
            done = 0;
        }
        if (flagY < poleBottom - 16) {
            flagY += SLIDE_SPEED;
            done = 0;
        }
        if (done) {
            // swing round to the other side of the pole
            mario.x = poleX - POLE_OTHER_SIDE_X;
            playerFaceLeft(1);
            state = TURNING;
            timer = 0;
        }
        break;
    case TURNING:
        if (++timer >= TURN_FRAMES) {
            playerFaceLeft(0);
            playerSetPose(POSE_NORMAL);
            state = WALKING;
        }
        break;
    case WALKING:
        // hop down and walk into the castle
        playerUpdate(J_RIGHT);
        if (mario.x >= doorX) {
            playerSetPose(POSE_HIDDEN);
            state = BONUS;
            timer = 0;
        }
        break;
    case BONUS:
        // the time left turns into points; pause before and after
        if (timer < BONUS_PAUSE_FRAMES) {
            timer++;
        } else if (!gameTimeBonus() && ++timer >= 2 * BONUS_PAUSE_FRAMES) {
            state = CASTLE_FLAG;
            castleFlagY = castleFlagTop + CASTLE_FLAG_RISE;
            timer = 0;
        }
        break;
    case CASTLE_FLAG:
        if (castleFlagY > castleFlagTop) {
            if (++timer & 1) castleFlagY--;
        } else {
            state = FIREWORKS;
            fireworksLeft = fireworks;
            timer = 0;
        }
        break;
    case FIREWORKS:
        if (!fireworksLeft) {
            if (++timer >= FINISH_FRAMES) state = FINISHED;
        } else {
            if (timer == 0) gameAddScore(FIREWORK_POINTS);
            if (++timer >= FIREWORK_FRAMES + FIREWORK_GAP) {
                fireworksLeft--;
                timer = 0;
            }
        }
        break;
    case FINISHED:
        return 1;
    }
    return 0;
}

void flagpoleDraw(void) BANKED {
    uint8_t n;
    if (!hasPole) return;
    // ItemTiles' flags are TL, BL, TR, BR
    spriteDraw16(SPR_TILE_FLAG, 0, flagX, flagY);
    if (state >= CASTLE_FLAG) {
        // behind the castle's bricks while it rises out of them
        spriteDraw16(SPR_TILE_CASTLE_FLAG, S_PRIORITY, castleFlagX, castleFlagY);
    }
    if (state == FIREWORKS && fireworksLeft && timer < FIREWORK_FRAMES) {
        n = fireworks - fireworksLeft;
        spriteDraw16(SPR_TILE_EXPLOSION + (timer / (FIREWORK_FRAMES / 3)) * 4, 0,
                     castleFlagX + fireworkX[n], castleFlagTop + fireworkY[n]);
    }
}
