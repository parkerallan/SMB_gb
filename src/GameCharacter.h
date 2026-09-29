#ifndef GAME_CHARACTER_H
#define GAME_CHARACTER_H

#include <stdint.h>

struct GameCharacter
{
    int16_t x;       // X position (world pixels)
    int16_t y;       // Y position (world pixels)
    uint8_t width;  // Width of the character
    uint8_t height; // Height of the character
};

#endif
