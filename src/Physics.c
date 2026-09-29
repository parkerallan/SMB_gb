#include "Physics.h"
#include "Level.h"

int8_t physicsSubPixelStep(uint8_t *sub, int16_t speed) {
    int16_t total = (int16_t)*sub + speed;
    *sub = total & 15;
    return (int8_t)(total >> 4);
}

// Only the leading edge is checked; on a hit the body snaps flush against the wall
uint8_t physicsMoveX(struct GameCharacter *body, int8_t dx) {
    int16_t top = body->y;
    int16_t bottom = top + body->height - 1;
    int16_t edge;

    if (dx > 0) {
        edge = body->x + body->width - 1 + dx;
        if (levelColumnSolid(edge >> 3, top, bottom)) {
            body->x = ((edge >> 3) << 3) - body->width;
            return 1;
        }
    } else if (dx < 0) {
        edge = body->x + dx;
        if (levelColumnSolid(edge >> 3, top, bottom)) {
            body->x = ((edge >> 3) + 1) << 3;
            return 1;
        }
    }
    body->x += dx;
    return 0;
}

uint8_t physicsMoveY(struct GameCharacter *body, int8_t dy) {
    int16_t left = body->x;
    int16_t right = left + body->width - 1;
    int16_t edge;

    if (dy > 0) {
        edge = body->y + body->height - 1 + dy;
        if (levelRowSolid(edge >> 3, left, right)) {
            body->y = ((edge >> 3) << 3) - body->height; // landed
            return 1;
        }
    } else if (dy < 0) {
        edge = body->y + dy;
        if (levelRowSolid(edge >> 3, left, right)) {
            body->y = ((edge >> 3) + 1) << 3;            // bumped a ceiling
            return 1;
        }
    }
    body->y += dy;
    return 0;
}

uint8_t physicsOnGround(const struct GameCharacter *body) {
    int16_t feet = body->y + body->height;
    return ((feet & 7) == 0) && levelRowSolid(feet >> 3, body->x, body->x + body->width - 1);
}
