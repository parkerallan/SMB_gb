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
#include "Pipes.h"
#include "Sprites.h"
#include "Util.h"
#include "Sound.h"

#define START_LIVES 4
// Clock starts at 400 and ticks down once per real second (60 frames).
// The NES original ticks every 24 frames (0.4 s).
#define TIME_LIMIT 400
#define TIME_TICK_FRAMES 60
#define DEATH_PAUSE_FRAMES 30
#define COIN_POINTS 200
#define COINS_PER_LIFE 100
// Background palettes: normal, and underground (black sky; the bricks' dark
// outlines blend into it, like SMB's)
#define PALETTE_OVERWORLD   DMG_PALETTE(DMG_WHITE, DMG_LITE_GRAY, DMG_DARK_GRAY, DMG_BLACK)
#define PALETTE_UNDERGROUND DMG_PALETTE(DMG_BLACK, DMG_LITE_GRAY, DMG_DARK_GRAY, DMG_BLACK)
#define TIME_BONUS_POINTS 50
#define TIME_BONUS_TICK 4      // the counting beeps every this many units
#define HURRY_TIME 100         // the clock gets here: the hurry jingle, then faster music
#define DEATH_MUSIC_MAX_FRAMES 300 // per unit of time left at the end of a level, like SMB

int8_t lives;
uint32_t score;
uint8_t coins;
int8_t world, level;

static uint16_t timeLeft;
static uint8_t timeFrames;
static uint8_t areaSong;   // the music for where Mario is (or the star's)
static uint8_t deathMusic; // started for this death

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

// Everything that belongs to the area Mario's in: its blocks, items, enemies
static void resetAreaObjects(void) {
    blocksReset(); // the area refills its blocks; coins and score carry over
    powerupReset();
    popupsReset();
    fireballsReset();
    enemiesReset();
    flagpoleReset();
    pipesReset();
    hudSetLevelPalette(levelUnderground ? PALETTE_UNDERGROUND : PALETTE_OVERWORLD);
}

// The music for where Mario is: ground or underground, the star's while he
// has one, all faster once the clock is under HURRY_TIME
static uint8_t areaMusic(void) {
    uint8_t song = playerHasStar() ? MUSIC_STAR : levelUnderground ? MUSIC_UNDERGROUND : MUSIC_GROUND;
    // each has its faster version right after it
    return timeLeft < HURRY_TIME ? song + 1 : song;
}

static void startAreaMusic(void) {
    areaSong = areaMusic();
    musicPlay(areaSong);
}

// Load the level with the screen off so it appears all at once
static void startLevel(void) {
    DISPLAY_OFF;
    playerReset();
    cameraReset();
    levelLoad(AREA_1_1, cameraX);
    resetAreaObjects();
    timeFrames = 0;
    drawSprites();
    SHOW_SPRITES;
    DISPLAY_ON;
    deathMusic = 0;
    startAreaMusic();
}

void gameChangeArea(uint8_t area, int16_t x, int16_t y) {
    DISPLAY_OFF;
    levelLoad(area, 0);
    mario.x = x;
    mario.y = y;
    cameraJumpTo(&mario);
    cameraApply(); // scroll there and fill in the columns now on screen
    resetAreaObjects();
    drawSprites();
    DISPLAY_ON;
    startAreaMusic();
}

void gameEnterLevel(void) {
    hudSetLevelPalette(PALETTE_OVERWORLD); // for the world screen
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
    sfxPlay(SFX_ONE_UP);
    lives++;
    refreshHud();
}

void gameCollectCoin(void) {
    sfxPlay(SFX_COIN);
    score += COIN_POINTS;
    if (++coins >= COINS_PER_LIFE) {
        coins = 0;
        gameAddLife();
    } else {
        hudUpdateScore(score, coins);
    }
}

static void startDeathMusic(void) {
    if (deathMusic) return;
    deathMusic = 1;
    musicPlay(MUSIC_DEATH);
}

static void loseLife(void) {
    uint16_t wait;
    startDeathMusic(); // (falling in a pit goes straight here)
    waitFrames(DEATH_PAUSE_FRAMES);
    // like SMB, carry on once the death music has finished
    for (wait = 0; musicPlaying() && wait < DEATH_MUSIC_MAX_FRAMES; wait++) wait_vbl_done();
    hudSetLevelPalette(PALETTE_OVERWORLD); // for the screens that follow
    lives--;
    if (lives <= 0) {
        refreshHud();
        gameOverScreen();
        reset(); // back to the title screen
    }
    gameEnterLevel();
}

uint16_t gameTimeLeft(void) {
    return timeLeft;
}

uint8_t gameTimeBonus(void) {
    if (!timeLeft) return 0;
    timeLeft--;
    score += TIME_BONUS_POINTS;
    if (!(timeLeft % TIME_BONUS_TICK)) sfxPlay(SFX_TICK);
    hudUpdateTime(timeLeft);
    hudUpdateScore(score, coins);
    return 1;
}

// Only 1-1 exists so far: play it again, keeping score, lives, coins and
// Mario's power-ups
static void levelComplete(void) {
    gameEnterLevel();
}

// Count the clock down; returns 1 when time runs out
static uint8_t tickClock(void) {
    if (++timeFrames >= TIME_TICK_FRAMES) {
        timeFrames = 0;
        timeLeft--;
        hudUpdateTime(timeLeft);
        if (timeLeft == HURRY_TIME - 1) {
            // hurry up! then the music carries on faster
            musicPlay(MUSIC_HURRY);
            areaSong = areaMusic();
            musicQueue(areaSong);
        }
    }
    return timeLeft == 0;
}

void gameUpdate(void) {
    uint8_t input;
    if (playerIsDying() || playerIsChangingSize()) {
        // Everything else stops while Mario changes size or dies, like SMB
        playerUpdate(0);
        if (playerIsDying()) startDeathMusic();
        if (playerDeathFinished()) {
            loseLife();
            return;
        }
    } else if (flagpoleEnding()) {
        // The level end runs Mario, the time bonus and the castle; everything
        // else stays put (the clock has stopped)
        if (flagpoleUpdate()) {
            levelComplete();
            return;
        }
        cameraFollow(&mario);
        blocksUpdate();
        fireballsUpdate();
        popupsUpdate();
    } else if (pipesActive()) {
        // Going through a pipe: everything else waits
        pipesUpdate();
        cameraFollow(&mario);
    } else {
        input = joypad();
        playerUpdate(input);
        if (playerFellOut()) {
            playerDie(); // takes his power-ups away, like any death
            loseLife();
            return;
        }
        if (tickClock()) playerDie(); // out of time
        flagpoleCheck(); // grabbed the flagpole?
        if (input & (J_DOWN | J_RIGHT)) pipesCheck(input); // going down (or into) a pipe?
        if (levelCoins) blocksCollectCoins();
        if (areaMusic() != areaSong) startAreaMusic(); // the star started or wore off

        cameraFollow(&mario);
        enemiesUpdate();
        blocksUpdate();
        powerupUpdate();
        fireballsUpdate();
        popupsUpdate();
    }
    drawSprites();
}
