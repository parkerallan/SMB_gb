// Level objects are banked code: ROM bank 3 (see make.bat)
#pragma bank 3

#include <gb/gb.h>
#include "Util.h"
#include "Enemies.h"
#include "Physics.h"
#include "Player.h"
#include "Camera.h"
#include "Level.h"
#include "Game.h"
#include "Popup.h"
#include "Level1_1Enemies.h"
#include "EnemyTiles.h"
#include "Sprites.h"
#include "Sound.h"

#define MAX_ENEMIES 6
#define SCREEN_WIDTH 160
#define SPAWN_MARGIN 16      // come alive just before scrolling into view
#define DESPAWN_MARGIN 64    // gone once this far off screen
// Speeds in 1/16 pixels per frame, like Physics
#define WALK_SPEED 8         // half a pixel per frame
#define SHELL_SPEED 48       // kicked shell: 3 pixels per frame
#define KNOCK_JUMP -48       // knocked out: pop up, then fall off the screen
#define SQUASH_FRAMES 30     // a stomped Goomba stays flat this long
#define SHELL_REVIVE_FRAMES 420 // a shell left alone turns back into a Koopa...
#define SHELL_LEGS_FRAMES 360   // ...wiggling its legs out first
#define KICK_GRACE_FRAMES 8  // a shell just kicked ignores Mario briefly
#define WALK_ANIM_SHIFT 3    // walk animation changes every 8 frames
#define STOMP_DEPTH 8        // stomp if Mario's feet are no deeper than this into the enemy
#define BUMP_POINTS 100
#define FIRE_POINTS_GOOMBA 100 // fireball kills, like SMB
#define FIRE_POINTS_KOOPA 200
#define SHELL_CHAIN_START 3  // shell kills start at 500

// Sprite tiles, loaded from EnemyTiles in this order
#define TILE_GOOMBA      ((uint8_t)(SPR_TILE_ENEMIES + 0))  // 16x16: TL BL TR BR
#define TILE_GOOMBA_FLAT ((uint8_t)(SPR_TILE_ENEMIES + 4))  // 16x16
#define TILE_KOOPA1      ((uint8_t)(SPR_TILE_ENEMIES + 8))  // 16x24: 6 tiles row by row, facing right
#define TILE_KOOPA2      ((uint8_t)(SPR_TILE_ENEMIES + 14))
#define TILE_SHELL       ((uint8_t)(SPR_TILE_ENEMIES + 20)) // 16x16, stored upside down
#define TILE_SHELL_LEGS  ((uint8_t)(SPR_TILE_ENEMIES + 24)) // 16x16, stored upside down
#define ENEMY_TILE_COUNT 28

enum { NONE, WALKING, SQUASHED, KNOCKED, SHELL, SHELL_MOVING };

typedef struct {
    struct GameCharacter body; // 16x16; a Koopa's head sticks up 8 pixels above it
    uint8_t kind;              // ENEMY_GOOMBA or ENEMY_KOOPA
    uint8_t state;
    int16_t speed;             // sign is the direction
    int16_t velocityY;
    uint8_t subX, subY;
    uint16_t timer;            // frames in the current state
    uint8_t anim;              // walk animation counter
    uint8_t kickGrace;
    uint8_t shellChain;        // how many enemies this shell has knocked out
    uint8_t grounded;          // standing on something: no need to fall until it moves
} Enemy;

static Enemy enemies[MAX_ENEMIES];
#define ENEMIES_END (enemies + MAX_ENEMIES)
static Enemy * const enemyAt[MAX_ENEMIES] = {
    enemies, enemies + 1, enemies + 2, enemies + 3, enemies + 4, enemies + 5
};
// For the collision checks, per enemy: whether it can touch things this frame,
// and its 16-pixel column (x >> 4) and the low bytes of its position, which
// only change when it moves (see remember())
static uint8_t collides[MAX_ENEMIES];
static uint8_t column[MAX_ENEMIES];
static uint8_t lowX[MAX_ENEMIES], lowY[MAX_ENEMIES];
static uint8_t frameParity; // alternates each frame: see enemiesUpdate
// Loops walk the array with a pointer: indexing an 18-byte struct array costs a
// multiply, which the Game Boy has to do in software

// Starting spots, copied from ROM at level start
static int16_t spawnX[Level1_1EnemyCount];
static uint8_t spawnY[Level1_1EnemyCount];
static uint8_t spawnKind[Level1_1EnemyCount];
static uint8_t spawned[Level1_1EnemyCount];
static uint8_t spawnCount; // entries in the current area's list (the coin room has none)

static uint8_t stompChain; // stomps since Mario last touched the ground
static uint8_t starChain;  // enemies knocked out by the current star
static uint8_t windowStart; // first spawn entry that isn't left of the spawn window (the list is sorted by x)
static int16_t despawnLeft, despawnRight;

// SMB's points for a chain of stomps (or shell hits); past the end it's a 1-UP
static const uint16_t chainPoints[] = {100, 200, 400, 500, 800, 1000, 2000, 4000, 5000, 8000};

void enemiesInit(void) BANKED {
    uint8_t bank = BANK(EnemyTiles);
    bankedSetSpriteData(TILE_GOOMBA, 4, EnemyTiles + ENEMYTILES_GOOMBA * 16, bank);
    bankedSetSpriteData(TILE_GOOMBA_FLAT, 4, EnemyTiles + ENEMYTILES_GOOMBA_FLAT * 16, bank);
    bankedSetSpriteData(TILE_KOOPA1, 6, EnemyTiles + ENEMYTILES_KOOPA1 * 16, bank);
    bankedSetSpriteData(TILE_KOOPA2, 6, EnemyTiles + ENEMYTILES_KOOPA2 * 16, bank);
    bankedSetSpriteData(TILE_SHELL, 4, EnemyTiles + ENEMYTILES_SHELL1 * 16, bank);
    bankedSetSpriteData(TILE_SHELL_LEGS, 4, EnemyTiles + ENEMYTILES_SHELL2 * 16, bank);
}

void enemiesReset(void) BANKED {
    uint8_t list[Level1_1EnemyCount * 4];
    const uint8_t *src = list;
    uint8_t i;
    Enemy *e;

    spawnCount = (levelArea == AREA_1_1) ? Level1_1EnemyCount : 0;
    bankedMemcpy(list, Level1_1Enemies, sizeof(list), BANK(Level1_1Enemies));
    for (i = 0; i < spawnCount; i++, src += 4) {
        spawnX[i] = src[0] | (src[1] << 8);
        spawnY[i] = src[2];
        spawnKind[i] = src[3];
        spawned[i] = 0;
    }

    for (e = enemies; e != ENEMIES_END; e++) e->state = NONE;
    stompChain = 0;
    windowStart = 0;
}

static void remember(uint8_t slot, const Enemy *e) {
    column[slot] = (uint8_t)(e->body.x >> 4);
    lowX[slot] = (uint8_t)e->body.x;
    lowY[slot] = (uint8_t)e->body.y;
}

static void spawn(uint8_t i) {
    Enemy *e = enemies;
    uint8_t slot;
    for (slot = 0; slot < MAX_ENEMIES; slot++, e++) {
        if (e->state == NONE) break;
    }
    if (slot == MAX_ENEMIES) return; // full: try again next frame
    spawned[i] = 1;
    e->body.x = spawnX[i];
    e->body.y = spawnY[i];
    e->body.width = 16;
    e->body.height = 16;
    e->kind = spawnKind[i];
    e->state = WALKING;
    e->speed = -WALK_SPEED; // SMB's enemies start off walking left, toward Mario
    e->velocityY = 0;
    e->subX = e->subY = 0;
    e->timer = 0;
    e->kickGrace = 0;
    e->grounded = 0;
    remember(slot, e);
}

// 16-wide things can only overlap if their 16-pixel columns are next to each
// other: a one-byte test that rules out most pairs before the full check
#define NEAR(colA, colB) ((uint8_t)((colA) - (colB) + 1) <= 2)
// Once they're NEAR, the low bytes of the positions are enough to tell whether
// two things overlap (differences wrap around the same way the full values do).
// Enemies are 16x16 and Mario is 16 wide.
#define OVERLAP_X(ax, bx) ((uint8_t)((ax) - (bx) + 15) < 31)
#define OVERLAP_Y(ay, ah, by, bh) ((uint8_t)((ay) - (by) + (ah) - 1) < (uint8_t)((ah) + (bh) - 1))
#define CAN_COLLIDE(state) ((state) == WALKING || (state) >= SHELL) // SHELL, SHELL_MOVING

// Points for the next hit in a chain, shown where it happened
static void award(uint8_t *chain, int16_t x, int16_t y) {
    if (*chain < sizeof(chainPoints) / sizeof(chainPoints[0])) {
        gameAddScore(chainPoints[*chain]);
        popupShow(x, y - 8, chainPoints[*chain]);
    } else {
        gameAddLife();
        popupShow(x, y - 8, POPUP_1UP);
    }
    (*chain)++;
}

static void knockOut(Enemy *e) {
    sfxPlay(SFX_KICK);
    e->state = KNOCKED;
    e->velocityY = KNOCK_JUMP;
    e->subY = 0;
    e->timer = 0;
}

// Every call here costs a lot of CPU time on the Game Boy, so enemies skip
// physics whenever it can't change anything: most frames they don't move a whole
// pixel, and standing enemies only need to look for ground after they've moved.
static void fall(Enemy *e) {
    e->velocityY += PHYSICS_GRAVITY;
    if (e->velocityY > PHYSICS_MAX_FALL_SPEED) e->velocityY = PHYSICS_MAX_FALL_SPEED;
    if (physicsMoveY(&e->body, physicsSubPixelStep(&e->subY, e->velocityY))) {
        e->velocityY = 0;
        e->subY = 0;
        e->grounded = 1;
    }
}

static void walk(Enemy *e, uint8_t frames) {
    // Sub-pixel step in 8 bits: enemy speeds are small (at most 3 pixels a frame)
    int8_t total = (int8_t)e->subX + (int8_t)(frames == 2 ? e->speed << 1 : e->speed);
    int8_t dx = total >> 4;
    uint8_t left, newLeft;
    e->subX = total & 15;
    if (!dx) return;
    // Walls and ground only change at 16-pixel block boundaries: if neither edge
    // of the 16-wide body crosses one, there's nothing new to bump into or fall off
    left = (uint8_t)e->body.x;
    newLeft = left + dx;
    if (!((left ^ newLeft) & 0xF0) && !(((uint8_t)(left + 15) ^ (uint8_t)(newLeft + 15)) & 0xF0)) {
        e->body.x += dx;
        return;
    }
    if (physicsMoveX(&e->body, dx)) {
        e->speed = -e->speed; // turn around at walls
        e->subX = 0;
    }
    if (e->grounded && !physicsOnGround(&e->body)) e->grounded = 0; // walked off a ledge
}

// Advance an enemy by `frames` (1, or 2 for the half-rate updates below)
static void move(Enemy *e, uint8_t frames) {
    e->timer += frames;
    e->anim += frames;
    switch (e->state) {
    case WALKING:
    case SHELL_MOVING:
        walk(e, frames);
        e->kickGrace = (e->kickGrace > frames) ? e->kickGrace - frames : 0;
        if (!e->grounded) {
            fall(e);
            if (frames == 2 && !e->grounded) fall(e);
        }
        break;
    case SHELL:
        if (!e->grounded) fall(e);
        if (e->timer >= SHELL_REVIVE_FRAMES) {
            e->state = WALKING; // the Koopa climbs back out, heading for Mario
            e->speed = (mario.x < e->body.x) ? -WALK_SPEED : WALK_SPEED;
            e->timer = 0;
        }
        break;
    case SQUASHED:
        if (e->timer >= SQUASH_FRAMES) e->state = NONE;
        break;
    case KNOCKED:
        // falls straight through everything
        do {
            e->velocityY += PHYSICS_GRAVITY;
            e->body.y += physicsSubPixelStep(&e->subY, e->velocityY);
        } while (--frames);
        break;
    }
    if (e->body.y > LEVEL_HEIGHT || e->body.x < despawnLeft || e->body.x > despawnRight) {
        e->state = NONE;
    }
}

static void touchMario(Enemy *e) {
    // Feet coming down on the top part of the enemy is a stomp; anything else hurts
    uint8_t stomp = playerIsFalling() && mario.y + mario.height <= e->body.y + STOMP_DEPTH;

    if (playerHasStar()) {
        // star power knocks out anything Mario touches
        knockOut(e);
        award(&starChain, e->body.x, e->body.y);
        return;
    }

    switch (e->state) {
    case WALKING:
        if (stomp) {
            if (e->kind == ENEMY_GOOMBA) {
                e->state = SQUASHED;
            } else {
                e->state = SHELL;
                e->speed = 0;
            }
            e->timer = 0;
            playerBounce();
            sfxPlay(SFX_STOMP);
            award(&stompChain, e->body.x, e->body.y);
        } else {
            playerHurt();
        }
        break;
    case SHELL:
        // kick it away from Mario
        e->state = SHELL_MOVING;
        sfxPlay(SFX_KICK);
        e->speed = (mario.x + 8 < e->body.x + 8) ? SHELL_SPEED : -SHELL_SPEED;
        e->kickGrace = KICK_GRACE_FRAMES;
        e->shellChain = SHELL_CHAIN_START;
        e->timer = 0;
        if (stomp) playerBounce();
        break;
    case SHELL_MOVING:
        if (stomp) {
            e->state = SHELL;
            e->speed = 0;
            e->timer = 0;
            playerBounce();
            sfxPlay(SFX_STOMP);
            award(&stompChain, e->body.x, e->body.y);
        } else {
            playerHurt();
        }
        break;
    }
}

// Walking into each other turns both around; a moving shell knocks out whatever it hits
static void touchEachOther(Enemy *a, Enemy *b) {
    uint8_t aShell = a->state == SHELL_MOVING, bShell = b->state == SHELL_MOVING;
    if (aShell || bShell) {
        if (aShell) award(&a->shellChain, b->body.x, b->body.y);
        else        award(&b->shellChain, a->body.x, a->body.y);
        if (!aShell || bShell) knockOut(a);
        if (!bShell || aShell) knockOut(b);
        return;
    }
    if ((a->speed > 0 && a->body.x < b->body.x) || (a->speed < 0 && a->body.x > b->body.x)) a->speed = -a->speed;
    if ((b->speed > 0 && b->body.x < a->body.x) || (b->speed < 0 && b->body.x > a->body.x)) b->speed = -b->speed;
}

void enemiesUpdate(void) BANKED {
    uint8_t i, j, state, everyFrame, marioColumn, marioX, marioY, marioH;
    Enemy *e;
    int16_t spawnRight = cameraX + SCREEN_WIDTH + SPAWN_MARGIN;
    int16_t spawnLeft = cameraX - SPAWN_MARGIN - 16;

    despawnLeft = cameraX - DESPAWN_MARGIN - 16;
    despawnRight = cameraX + SCREEN_WIDTH + DESPAWN_MARGIN;

    // The spawn list is sorted by x: slide the window's start with the camera,
    // then only look at the few entries inside it
    while (windowStart < spawnCount && spawnX[windowStart] <= spawnLeft) windowStart++;
    while (windowStart > 0 && spawnX[windowStart - 1] > spawnLeft) windowStart--;
    for (i = windowStart; i < spawnCount && spawnX[i] < spawnRight; i++) {
        if (!spawned[i]) spawn(i);
    }

    if (playerOnGround()) stompChain = 0;
    if (!playerHasStar()) starChain = 0;
    marioColumn = (uint8_t)(mario.x >> 4);
    marioX = (uint8_t)mario.x;
    marioY = (uint8_t)mario.y;
    marioH = mario.height;
    if (playerIsDying()) marioColumn += 128; // nothing is NEAR a dying Mario

    // The Game Boy can't move six enemies every frame and keep up, so each one
    // moves every other frame by two frames' worth (half on even frames, half on
    // odd). Walking enemies only move a pixel every other frame anyway, so this
    // looks the same; a kicked shell is fast and still moves every frame, and
    // so does a knocked-out enemy falling off the screen.
    // Collisions with Mario are still checked every frame.
    frameParity ^= 1;
    e = enemies;
    for (i = 0; i < MAX_ENEMIES; i++, e++) {
        state = e->state;
        if (state == NONE) {
            collides[i] = 0;
            continue;
        }
        everyFrame = (state == SHELL_MOVING || state == KNOCKED);
        if (everyFrame || (i & 1) == frameParity) {
            move(e, everyFrame ? 1 : 2);
            state = e->state;
            remember(i, e);
        }
        state = CAN_COLLIDE(state);
        collides[i] = state;
        // a shell that was just kicked ignores Mario for a moment
        if (state && NEAR(column[i], marioColumn) && !e->kickGrace &&
            OVERLAP_X(marioX, lowX[i]) && OVERLAP_Y(marioY, marioH, lowY[i], 16)) {
            touchMario(e);
            collides[i] = CAN_COLLIDE(e->state);
        }
    }

    // Enemies against each other: every other frame is plenty (a shell just
    // knocks an enemy out a frame later)
    if (frameParity) return;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!collides[i]) continue;
        for (j = i + 1; j < MAX_ENEMIES; j++) {
            if (!collides[j] || !NEAR(column[i], column[j])) continue;
            if (OVERLAP_X(lowX[i], lowX[j]) && OVERLAP_Y(lowY[i], 16, lowY[j], 16)) {
                touchEachOther(enemyAt[i], enemyAt[j]);
                collides[i] = CAN_COLLIDE(enemyAt[i]->state);
                collides[j] = CAN_COLLIDE(enemyAt[j]->state);
                if (!collides[i]) break;
            }
        }
    }
}

void enemiesBumpBlock(int16_t bx, int16_t by) BANKED {
    int16_t left = bx << 4, top = by << 4;
    Enemy *e;
    for (e = enemies; e != ENEMIES_END; e++) {
        if ((e->state == WALKING || e->state == SHELL) && e->body.y + e->body.height == top &&
            e->body.x + e->body.width > left && e->body.x < left + 16) {
            knockOut(e);
            gameAddScore(BUMP_POINTS);
            popupShow(e->body.x, e->body.y - 8, BUMP_POINTS);
        }
    }
}

uint8_t enemiesFireballHit(int16_t x, int16_t y) BANKED {
    // an 8x8 fireball against 16x16 enemies, with the same 8-bit checks as
    // enemiesUpdate (NEAR first, then the low bytes)
    uint8_t i, col = (uint8_t)(x >> 4), fx = (uint8_t)x, fy = (uint8_t)y;
    uint16_t points;
    Enemy *e = enemies;
    for (i = 0; i < MAX_ENEMIES; i++, e++) {
        if (!CAN_COLLIDE(e->state) || !NEAR(column[i], col)) continue;
        if ((uint8_t)(fx - lowX[i] + 7) < 23 && (uint8_t)(fy - lowY[i] + 7) < 23) {
            knockOut(e);
            collides[i] = 0;
            points = (e->kind == ENEMY_GOOMBA) ? FIRE_POINTS_GOOMBA : FIRE_POINTS_KOOPA;
            gameAddScore(points);
            popupShow(e->body.x, e->body.y - 8, points);
            return 1;
        }
    }
    return 0;
}

void enemiesDraw(void) BANKED {
    uint8_t walkFlip;
    Enemy *e;
    for (e = enemies; e != ENEMIES_END; e++) {
        walkFlip = (e->anim & (1 << WALK_ANIM_SHIFT)) ? S_FLIPX : 0;
        switch (e->state) {
        case WALKING:
            if (e->kind == ENEMY_GOOMBA) {
                // walks by flipping
                spriteDraw16(TILE_GOOMBA, walkFlip, e->body.x, e->body.y);
            } else {
                // Koopa tiles face right; its head sticks up 8 pixels above the body
                spriteDrawKoopa(walkFlip ? TILE_KOOPA2 : TILE_KOOPA1, e->speed < 0 ? S_FLIPX : 0,
                                e->body.x, e->body.y - 8);
            }
            break;
        case SQUASHED:
            spriteDraw16(TILE_GOOMBA_FLAT, 0, e->body.x, e->body.y);
            break;
        case SHELL:
        case SHELL_MOVING:
            // shell tiles are stored upside down: flip them the right way up
            spriteDraw16((e->state == SHELL && e->timer >= SHELL_LEGS_FRAMES) ? TILE_SHELL_LEGS : TILE_SHELL,
                         S_FLIPY, e->body.x, e->body.y);
            break;
        case KNOCKED:
            if (e->kind == ENEMY_GOOMBA) spriteDraw16(TILE_GOOMBA, S_FLIPY, e->body.x, e->body.y);
            else                         spriteDraw16(TILE_SHELL, 0, e->body.x, e->body.y);
            break;
        }
    }
}
