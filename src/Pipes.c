// Level objects are banked code: ROM bank 3 (see make.bat)
#pragma bank 3

#include <gb/gb.h>
#include "Pipes.h"
#include "Level.h"
#include "Player.h"
#include "Game.h"

#define SINK_FRAMES 32        // down into a pipe: a pixel a frame until even big Mario is in
#define WALK_IN_FRAMES 20     // into the sideways pipe
#define ENTRY_SLACK 4         // how far Mario's middle may be from the pipe's middle to go down it

// 1-1's pipe down to the coin room, the room's way out, and the pipe it leads back to
#define PIPE_X        (LEVEL1_1_PIPE_COLUMN << 4)
#define PIPE_TOP      (LEVEL1_1_PIPE_ROW << 4)
#define EXIT_X        (LEVEL1_1_BONUS_EXIT_COLUMN << 4)
#define EXIT_BOTTOM   ((LEVEL1_1_BONUS_EXIT_ROW + 2) << 4)
#define RETURN_X      (LEVEL1_1_RETURN_PIPE_COLUMN << 4)
#define RETURN_TOP    (LEVEL1_1_RETURN_PIPE_ROW << 4)

enum { NONE, GOING_DOWN, GOING_RIGHT, COMING_UP };

static uint8_t state, timer;

void pipesReset(void) BANKED {
    state = NONE;
}

uint8_t pipesActive(void) BANKED {
    return state != NONE;
}

static void start(uint8_t how) {
    state = how;
    timer = 0;
    playerSetPose(POSE_PIPE);
}

void pipesCheck(uint8_t input) BANKED {
    if (!playerOnGround()) return;
    if (levelArea == AREA_1_1) {
        // Down while standing on the pipe's top, near its middle (it's 32 pixels wide)
        if ((input & J_DOWN) && mario.y + mario.height == PIPE_TOP &&
            (uint16_t)(mario.x - (PIPE_X + 8 - ENTRY_SLACK)) <= 2 * ENTRY_SLACK) {
            start(GOING_DOWN);
        }
    } else if (levelArea == AREA_1_1_BONUS) {
        // Right while standing against the sideways pipe's mouth
        if ((input & J_RIGHT) && mario.x + mario.width >= EXIT_X && mario.y + mario.height == EXIT_BOTTOM) {
            playerFaceLeft(0);
            start(GOING_RIGHT);
        }
    }
}

void pipesUpdate(void) BANKED {
    switch (state) {
    case GOING_DOWN:
        mario.y++;
        if (++timer >= SINK_FRAMES) {
            // drop into the coin room from the top
            gameChangeArea(AREA_1_1_BONUS, LEVEL1_1_BONUS_START_X, LEVEL1_1_BONUS_START_Y);
            playerSetPose(POSE_NORMAL);
            state = NONE;
        }
        break;
    case GOING_RIGHT:
        mario.x++;
        playerAnimateWalk();
        if (++timer >= WALK_IN_FRAMES) {
            // back in 1-1, inside the pipe near the end, and rise out of it
            gameChangeArea(AREA_1_1, RETURN_X + 8, RETURN_TOP);
            playerSetPose(POSE_PIPE);
            state = COMING_UP;
        }
        break;
    case COMING_UP:
        if (mario.y + mario.height > RETURN_TOP) {
            mario.y--;
        } else {
            playerSetPose(POSE_NORMAL);
            state = NONE;
        }
        break;
    }
}
