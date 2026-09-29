#ifndef GAME_H
#define GAME_H

#include <stdint.h>

extern int8_t lives;
extern uint16_t score;
extern uint8_t coins;
extern int8_t world, level;

// Load graphics and set up the HUD (once at power on)
void gameInit(void);

// Reset lives, score and world for a new game
void gameNew(void);

// Show the world screen, then (re)start the current level from the beginning
void gameEnterLevel(void);

// Mario got a coin: +200 points, and every 100 coins is an extra life
void gameCollectCoin(void);

// Run one frame of gameplay
void gameUpdate(void);

#endif
