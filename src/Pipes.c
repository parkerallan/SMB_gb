// Level objects are banked code: ROM bank 3 (see make.bat)
#pragma bank 3

#include <gb/gb.h>
#include "Pipes.h"
#include "Level.h"
#include "Player.h"
#include "Game.h"
#include "Sound.h"

#define SINK_FRAMES 32        // down into a pipe: a pixel a frame until even big Mario is in
#define WALK_IN_FRAMES 20     // into a sideways pipe
#define ENTRY_SLACK 4         // how far Mario's middle may be from the pipe's middle to go down it

#define BLOCK(n) ((int16_t)(n) << 4)

// How Mario goes in, and how he comes out at the other end
enum { DOWN, RIGHT };  // down a pipe's top, or right into a sideways pipe's mouth
enum { DROP, RISE };   // drops into the area from above, or rises out of a pipe

typedef struct {
    uint8_t area, way;
    int16_t x, y;              // DOWN: the pipe's left and top; RIGHT: the mouth's left and bottom (where he stands)
    uint8_t toArea, arrive;
    int16_t toX, toY;          // DROP: his left and his feet; RISE: the pipe's left and top
} Pipe;

// Every pipe that leads somewhere (all from SMB's own level data: see tools/nes2gb.py)
static const Pipe pipes[] = {
    // 1-1 down to its coin room, and back out near its end
    {AREA_1_1, DOWN, BLOCK(LEVEL1_1_PIPE_COLUMN), BLOCK(LEVEL1_1_PIPE_ROW),
     AREA_1_1_BONUS, DROP, LEVEL1_1_BONUS_START_X, LEVEL1_1_BONUS_START_FEET},
    {AREA_1_1_BONUS, RIGHT, BLOCK(LEVEL1_1_BONUS_EXIT_COLUMN), BLOCK(LEVEL1_1_BONUS_EXIT_ROW + 2),
     AREA_1_1, RISE, BLOCK(LEVEL1_1_RETURN_PIPE_COLUMN), BLOCK(LEVEL1_1_RETURN_PIPE_ROW)},
    // 1-2's intro pipe, into the level underground
    {AREA_1_2_INTRO, RIGHT, BLOCK(LEVEL1_2_INTRO_PIPE_COLUMN), BLOCK(LEVEL1_2_INTRO_PIPE_ROW + 2),
     AREA_1_2, DROP, LEVEL1_2_START_X, LEVEL1_2_START_FEET},
    // 1-2 down to its coin room, and back out further on
    {AREA_1_2, DOWN, BLOCK(LEVEL1_2_PIPE_COLUMN), BLOCK(LEVEL1_2_PIPE_ROW),
     AREA_1_2_BONUS, DROP, LEVEL1_2_BONUS_START_X, LEVEL1_2_BONUS_START_FEET},
    {AREA_1_2_BONUS, RIGHT, BLOCK(LEVEL1_2_BONUS_EXIT_COLUMN), BLOCK(LEVEL1_2_BONUS_EXIT_ROW + 2),
     AREA_1_2, RISE, BLOCK(LEVEL1_2_RETURN_PIPE_COLUMN), BLOCK(LEVEL1_2_RETURN_PIPE_ROW)},
    // 1-2's way out: SMB reuses the end of 1-1 for it (the flagpole and castle)
    {AREA_1_2, RIGHT, BLOCK(LEVEL1_2_EXIT_COLUMN), BLOCK(LEVEL1_2_EXIT_ROW + 2),
     AREA_1_1, RISE, BLOCK(LEVEL1_2_EXIT_PIPE_COLUMN), BLOCK(LEVEL1_2_EXIT_PIPE_ROW)},
};
#define PIPE_COUNT (sizeof(pipes) / sizeof(pipes[0]))

enum { NONE, GOING_DOWN, GOING_RIGHT, COMING_UP };

static uint8_t state, timer;
static const Pipe *pipe; // the one Mario's going through

void pipesReset(void) BANKED {
    state = NONE;
}

uint8_t pipesActive(void) BANKED {
    return state != NONE;
}

static void start(const Pipe *p) {
    pipe = p;
    state = (p->way == DOWN) ? GOING_DOWN : GOING_RIGHT;
    timer = 0;
    if (p->way == RIGHT) playerFaceLeft(0);
    playerSetPose(POSE_PIPE);
    musicStop();
    sfxPlay(SFX_PIPE);
}

void pipesCheck(uint8_t input) BANKED {
    const Pipe *p;
    int16_t feet = mario.y + mario.height;
    if (!playerOnGround()) return;
    for (p = pipes; p != pipes + PIPE_COUNT; p++) {
        if (p->area != levelArea) continue;
        if (p->way == DOWN) {
            // Down while standing on the pipe's top, near its middle (it's 32 pixels wide)
            if ((input & J_DOWN) && feet == p->y &&
                (uint16_t)(mario.x - (p->x + 8 - ENTRY_SLACK)) <= 2 * ENTRY_SLACK) {
                start(p);
                return;
            }
        } else if ((input & J_RIGHT) && mario.x + mario.width >= p->x && mario.x < p->x && feet == p->y) {
            // Right while standing against the sideways pipe's mouth
            start(p);
            return;
        }
    }
}

// At the other end: drop in from above, or rise out of a pipe
static void arrive(void) {
    if (pipe->arrive == DROP) {
        gameChangeArea(pipe->toArea, pipe->toX, pipe->toY - mario.height);
        playerSetPose(POSE_NORMAL);
        state = NONE;
    } else {
        gameChangeArea(pipe->toArea, pipe->toX + 8, pipe->toY);
        playerFaceLeft(0);
        playerSetPose(POSE_PIPE);
        state = COMING_UP;
    }
}

void pipesUpdate(void) BANKED {
    switch (state) {
    case GOING_DOWN:
        mario.y++;
        if (++timer >= SINK_FRAMES) arrive();
        break;
    case GOING_RIGHT:
        mario.x++;
        playerAnimateWalk();
        if (++timer >= WALK_IN_FRAMES) arrive();
        break;
    case COMING_UP:
        if (mario.y + mario.height > pipe->toY) {
            mario.y--;
        } else {
            playerSetPose(POSE_NORMAL);
            state = NONE;
        }
        break;
    }
}
