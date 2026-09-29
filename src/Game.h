#ifndef GAME_H
#define GAME_H

#include <stdint.h>

extern int8_t lives;
extern uint16_t score;
extern int8_t world, level;

// Reset lives, score and world for a new game
void gameNew(void);

// Show the world screen, then (re)start the current level from the beginning
void gameEnterLevel(void);

// Run one frame of gameplay
void gameUpdate(void);

#endif
