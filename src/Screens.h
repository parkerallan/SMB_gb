#ifndef SCREENS_H
#define SCREENS_H

#include <stdint.h>

// Show the title and wait for Start or A
void titleScreen(void);

// SMB's intermission screen: "WORLD w-l" and Mario x lives
void worldScreen(int8_t world, int8_t level, int8_t lives);

// "GAME OVER" on a blank screen
void gameOverScreen(void);

#endif
