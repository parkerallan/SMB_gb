#ifndef GAME_CHARACTER_H
#define GAME_CHARACTER_H

#include <stdint.h>

struct GameCharacter
{
    uint16_t x;      // X position
    uint16_t y;      // Y position
    uint8_t width;  // Width of the character
    uint8_t height; // Height of the character
};

#endif
