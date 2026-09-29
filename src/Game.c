#include <gb/gb.h>
#include "Game.h"
#include "Player.h"
#include "Camera.h"
#include "Level.h"
#include "Hud.h"
#include "Screens.h"
#include "Blocks.h"
#include "Flagpole.h"
#include "Powerup.h"
#include "Enemies.h"
#include "Popup.h"
#include "Fireball.h"
#include "Sprites.h"
#include "Util.h"

#define START_LIVES 4
// Clock starts at 400 and ticks down once per real second (60 frames).
// The NES original ticks every 24 frames (0.4 s).
#define TIME_LIMIT 400
#define TIME_TICK_FRAMES 60
#define DEATH_PAUSE_FRAMES 30
#define COIN_POINTS 200
#define COINS_PER_LIFE 100

int8_t lives;
uint16_t score;
uint8_t coins;
int8_t world, level;

static uint16_t timeLeft;
static uint8_t timeFrames;

static void refreshHud(void) {
    hudUpdate(score, coins, lives, world, level, timeLeft);
}

void gameInit(void) {
    hudInit();
    levelInit();
    playerInit();
    blocksInit();
    powerupInit();
    enemiesInit();
    popupsInit();
    fireballsInit();
    flagpoleInit();
}

void gameNew(void) {
    lives = START_LIVES;
    score = 0;
    coins = 0;
    world = 1;
    level = 1;
}

// Every sprite, drawn fresh each frame (also while the game is paused)
static void drawSprites(void) {
    spritesBegin();
    playerDraw();
    enemiesDraw();
    powerupDraw();
    fireballsDraw();
    blocksDraw();
    popupsDraw();
    flagpoleDraw();
    spritesEnd();
}

// Load the level with the screen off so it appears all at once
static void startLevel(void) {
    DISPLAY_OFF;
    playerReset();
    cameraReset();
    blocksReset(); // the level refills its blocks; coins and score carry over
    powerupReset();
    popupsReset();
    fireballsReset();
    levelLoad(cameraX);
    enemiesReset();
    flagpoleReset();
    timeFrames = 0;
    drawSprites();
    SHOW_SPRITES;
    DISPLAY_ON;
}

void gameEnterLevel(void) {
    timeLeft = TIME_LIMIT;
    refreshHud();
    worldScreen(world, level, lives);
    startLevel();
}

void gameAddScore(uint16_t points) {
    score += points;
    hudUpdateScore(score, coins);
}

void gameAddLife(void) {
    lives++;
    refreshHud();
}

void gameCollectCoin(void) {
    score += COIN_POINTS;
    if (++coins >= COINS_PER_LIFE) {
        coins = 0;
        gameAddLife();
    } else {
        hudUpdateScore(score, coins);
    }
}

static void loseLife(void) {
    waitFrames(DEATH_PAUSE_FRAMES);
    lives--;
    if (lives <= 0) {
        refreshHud();
        gameOverScreen();
        reset(); // back to the title screen
    }
    gameEnterLevel();
}

// Count the clock down; returns 1 when time runs out
static uint8_t tickClock(void) {
    if (++timeFrames >= TIME_TICK_FRAMES) {
        timeFrames = 0;
        timeLeft--;
        hudUpdateTime(timeLeft);
    }
    return timeLeft == 0;
}

void gameUpdate(void) {
    if (playerIsDying() || playerIsChangingSize()) {
        // Everything else stops while Mario changes size or dies, like SMB
        playerUpdate(0);
        if (playerDeathFinished()) {
            loseLife();
            return;
        }
    } else {
        playerUpdate(joypad());
        if (playerFellOut()) {
            loseLife();
            return;
        }
        if (tickClock()) playerDie(); // out of time

        cameraFollow(&mario);
        enemiesUpdate();
        blocksUpdate();
        powerupUpdate();
        fireballsUpdate();
        popupsUpdate();
    }
    drawSprites();
}
