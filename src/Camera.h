#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>
#include "GameCharacter.h"

// Top-left of the visible screen, in level pixels
extern int16_t cameraX, cameraY;

// Put the camera at the bottom-left of the level
void cameraReset(void);

// Jump to show the target (arriving in another area), then scroll as usual
void cameraJumpTo(const struct GameCharacter *target);

// Scroll to keep the target inside the dead zone
void cameraFollow(const struct GameCharacter *target);

// Push the camera to the scroll registers and stream in new level columns.
// Call right after wait_vbl_done() so the screen doesn't tear.
void cameraApply(void);

#endif
