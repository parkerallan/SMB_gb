#ifndef GAME_H
#define GAME_H

#include <stdint.h>

extern int8_t lives;
extern uint32_t score; // SMB's six digits don't fit in 16 bits
extern uint8_t coins;
extern int8_t world, level;

// Load graphics and set up the HUD (once at power on)
void gameInit(void);

// Reset lives, score and world for a new game
void gameNew(void);

// Show the world screen, then (re)start the current level from the beginning
void gameEnterLevel(void);

// Add points and show them in the HUD
void gameAddScore(uint16_t points);

// Extra life (shown in the HUD)
void gameAddLife(void);

// The clock, and the level end's time bonus: turns one unit of time left into
// points; returns 0 once the clock is empty
uint16_t gameTimeLeft(void);
uint8_t gameTimeBonus(void);

// Mario got a coin: +200 points, and every 100 coins is an extra life
void gameCollectCoin(void);

// Run one frame of gameplay
void gameUpdate(void);

#endif
