#include <gb/gb.h>
#include "Game.h"
#include "Player.h"
#include "Camera.h"
#include "Level.h"
#include "Hud.h"
#include "Screens.h"
#include "Util.h"

#define START_LIVES 4
// Clock starts at 400 and ticks down once per real second (60 frames).
// The NES original ticks every 24 frames (0.4 s).
#define TIME_LIMIT 400
#define TIME_TICK_FRAMES 60
#define DEATH_PAUSE_FRAMES 30

int8_t lives;
uint16_t score;
int8_t world, level;

static uint16_t timeLeft;
static uint8_t timeFrames;

static void refreshHud(void) {
    hudUpdate(score, lives, world, level, timeLeft);
}

void gameNew(void) {
    lives = START_LIVES;
    score = 0;
    world = 1;
    level = 1;
}

// Load the level with the screen off so it appears all at once
static void startLevel(void) {
    DISPLAY_OFF;
    playerReset();
    cameraReset();
    levelLoad(cameraX);
    timeFrames = 0;
    playerDraw();
    SHOW_SPRITES;
    DISPLAY_ON;
}

void gameEnterLevel(void) {
    timeLeft = TIME_LIMIT;
    refreshHud();
    worldScreen(world, level, lives);
    startLevel();
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
    playerUpdate(joypad());

    if (tickClock() || playerFellOut()) {
        loseLife();
        return;
    }

    cameraFollow(&mario);
    playerDraw();
}
