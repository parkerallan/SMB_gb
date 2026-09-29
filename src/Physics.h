#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdint.h>
#include "GameCharacter.h"

// Movement and level collision shared by Mario and items. Speeds are in 1/16
// pixels per frame (4-bit sub-pixels).
#define PHYSICS_GRAVITY 3
#define PHYSICS_MAX_FALL_SPEED 64

// Add a speed to a sub-pixel accumulator, returning whole pixels to move
int8_t physicsSubPixelStep(uint8_t *sub, int16_t speed);

// Move by dx pixels (|dx| < 8). Returns 1 if a wall stopped it (snapped flush).
uint8_t physicsMoveX(struct GameCharacter *body, int8_t dx);

// Move by dy pixels (|dy| < 8). Returns 1 if the ground or a ceiling stopped it (snapped flush).
uint8_t physicsMoveY(struct GameCharacter *body, int8_t dy);

// Is the body standing on something solid?
uint8_t physicsOnGround(const struct GameCharacter *body);

#endif
