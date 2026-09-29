#include <gb/gb.h>
#include "Camera.h"
#include "Level.h"
#include "Hud.h"

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144
#define CAMERA_MAX_X (LEVEL_WIDTH - SCREEN_WIDTH)
#define CAMERA_MAX_Y (LEVEL_HEIGHT - SCREEN_HEIGHT)
// Horizontal dead zone: the camera scrolls when the target's screen x leaves this range
#define CAMERA_LEFT_X 56
#define CAMERA_RIGHT_X 72
// Vertical dead zone: keep the target's top at least this far below the screen
// top (clear of the HUD) and its feet at least this far above the screen bottom
#define CAMERA_MARGIN_TOP (HUD_PIXEL_HEIGHT + 16)
#define CAMERA_MARGIN_BOTTOM 32

int16_t cameraX, cameraY;

void cameraReset(void) {
    cameraX = 0;
    cameraY = CAMERA_MAX_Y;
    move_bkg(0, (uint8_t)cameraY);
}

void cameraFollow(const struct GameCharacter *target) {
    if (target->x - cameraX > CAMERA_RIGHT_X) {
        cameraX = target->x - CAMERA_RIGHT_X;
    } else if (target->x - cameraX < CAMERA_LEFT_X) {
        cameraX = target->x - CAMERA_LEFT_X;
    }
    if (cameraX < 0) cameraX = 0;
    if (cameraX > CAMERA_MAX_X) cameraX = CAMERA_MAX_X;

    if (target->y - cameraY < CAMERA_MARGIN_TOP) {
        cameraY = target->y - CAMERA_MARGIN_TOP;
    } else if (target->y + target->height - cameraY > SCREEN_HEIGHT - CAMERA_MARGIN_BOTTOM) {
        cameraY = target->y + target->height - (SCREEN_HEIGHT - CAMERA_MARGIN_BOTTOM);
    }
    if (cameraY < 0) cameraY = 0;
    if (cameraY > CAMERA_MAX_Y) cameraY = CAMERA_MAX_Y;
}

void cameraJumpTo(const struct GameCharacter *target) {
    cameraX = target->x - CAMERA_LEFT_X;
    cameraY = CAMERA_MAX_Y;
    cameraFollow(target);
}

void cameraApply(void) {
    move_bkg((uint8_t)cameraX, (uint8_t)cameraY);
    levelStream(cameraX);
}
