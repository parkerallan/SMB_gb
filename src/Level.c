#include <gb/gb.h>
#include <string.h>
#include "Level.h"
#include "Blocks.h"

// The VRAM background map is 32 tiles wide and wraps around
#define VRAM_COLUMNS 32

// Bank rule: while a level is being played, the map's ROM bank stays switched
// in, because collision and scrolling read the map every frame. Anything that
// switches to another bank must switch back afterwards.

// Blocks Mario can't pass through; everything else (sky, hills, bushes, clouds,
// castle, flagpole, coins, water, hidden blocks until hit) is scenery
static const uint8_t solidBlocks[] = {
    SCENERY_PIPE_TOP_LEFT, SCENERY_PIPE_TOP_RIGHT, SCENERY_DECOR_PIPE_TOP_LEFT,
    SCENERY_DECOR_PIPE_TOP_RIGHT, SCENERY_PIPE_LEFT, SCENERY_PIPE_RIGHT,
    SCENERY_SIDE_PIPE_END_TOP, SCENERY_SIDE_PIPE_TOP, SCENERY_SIDE_PIPE_JOINT_TOP,
    SCENERY_SIDE_PIPE_END_BOTTOM, SCENERY_SIDE_PIPE_BOTTOM, SCENERY_SIDE_PIPE_JOINT_BOTTOM,
    SCENERY_TREE_LEDGE_LEFT, SCENERY_TREE_LEDGE_MIDDLE, SCENERY_TREE_LEDGE_RIGHT,
    SCENERY_MUSHROOM_LEFT, SCENERY_MUSHROOM_MIDDLE, SCENERY_MUSHROOM_RIGHT, SCENERY_SEAPLANT,
    SCENERY_ROCK, SCENERY_STAIR_BLOCK, SCENERY_WHITE_WALL, SCENERY_WATER_ROCK,
    SCENERY_BRICK_LINE, SCENERY_BRICK, SCENERY_BRICK_UNUSED, SCENERY_BRICK_LINE_POWERUP,
    SCENERY_BRICK_LINE_VINE, SCENERY_BRICK_LINE_STAR, SCENERY_BRICK_LINE_COINS,
    SCENERY_BRICK_LINE_1UP, SCENERY_BRICK_POWERUP, SCENERY_BRICK_VINE, SCENERY_BRICK_STAR,
    SCENERY_BRICK_COINS, SCENERY_BRICK_1UP, SCENERY_HALF_BRICK, SCENERY_HALF_BRICK_SPRING,
    SCENERY_BRIDGE, SCENERY_BOWSER_BRIDGE, SCENERY_CLOUD_GROUND,
    SCENERY_CANNON_BARREL, SCENERY_CANNON_TOP, SCENERY_CANNON_BOTTOM,
    SCENERY_WATER_PIPE_TOP, SCENERY_WATER_PIPE_BOTTOM,
    SCENERY_QUESTION_COIN, SCENERY_QUESTION_POWERUP, SCENERY_USED_BLOCK,
};

static uint8_t blockSolid[SceneryMetatileCount];
static uint8_t blockTiles[SceneryMetatileCount * 4]; // copy of SceneryMetatiles, read every column
static int16_t firstColumn, lastColumn;               // range of level columns currently in VRAM
static uint8_t columnBuffer[LEVEL_ROWS];

void levelInit(void) {
    uint8_t i, saved = CURRENT_BANK;
    SWITCH_ROM(BANK(SceneryTiles));
    memcpy(blockTiles, SceneryMetatiles, sizeof(blockTiles));
    SWITCH_ROM(saved);
    for (i = 0; i < sizeof(solidBlocks); i++) blockSolid[solidBlocks[i]] = 1;
}

const uint8_t *levelBlockTiles(uint8_t block) {
    return blockTiles + block * 4;
}

// Copy one full-height level column (half a block column) into the wrapping VRAM map
static void loadColumn(int16_t column) {
    const uint8_t *src = Level1_1 + (column >> 1);
    uint8_t half = column & 1; // 0 = left tiles of each block, 1 = right tiles
    uint8_t row;
    const uint8_t *tiles;
    for (row = 0; row < LEVEL_ROWS; row += 2) {
        tiles = blockTiles + *src * 4 + half;
        columnBuffer[row] = tiles[0];
        columnBuffer[row + 1] = tiles[2];
        src += Level1_1Width;
    }
    blocksPatchColumn(column, columnBuffer);
    set_bkg_tiles(column & (VRAM_COLUMNS - 1), 0, 1, LEVEL_ROWS, columnBuffer);
}

void levelLoad(int16_t cameraX) {
    SWITCH_ROM(BANK(SceneryTiles));
    set_bkg_data(0, SceneryTilesCount, SceneryTiles);
    SWITCH_ROM(BANK(Level1_1)); // stays switched in for the rest of the level
    firstColumn = cameraX >> 3;
    lastColumn = firstColumn;
    loadColumn(firstColumn);
    levelStream(cameraX);
}

// Keep VRAM filled one column past both edges of the screen. Loading a column
// on one side overwrites the far column on the other.
void levelStream(int16_t cameraX) {
    int16_t firstNeeded = (cameraX >> 3) - 1;
    int16_t lastNeeded = (cameraX >> 3) + 21;
    if (firstNeeded < 0) firstNeeded = 0;
    if (lastNeeded >= LEVEL_COLUMNS) lastNeeded = LEVEL_COLUMNS - 1;

    while (lastColumn < lastNeeded) {
        loadColumn(++lastColumn);
        if (lastColumn - firstColumn >= VRAM_COLUMNS) firstColumn = lastColumn - (VRAM_COLUMNS - 1);
    }
    while (firstColumn > firstNeeded) {
        loadColumn(--firstColumn);
        if (lastColumn - firstColumn >= VRAM_COLUMNS) lastColumn = firstColumn + (VRAM_COLUMNS - 1);
    }
}

uint8_t levelBlockAt(int16_t bx, int16_t by) {
    if (bx < 0 || bx >= Level1_1Width || by < 0 || by >= Level1_1Height) return SCENERY_BLANK;
    return Level1_1[by * Level1_1Width + bx];
}

void levelSetTile(int16_t tx, int16_t ty, uint8_t tile) {
    if (tx >= firstColumn && tx <= lastColumn) {
        set_bkg_tile_xy(tx & (VRAM_COLUMNS - 1), ty, tile);
    }
}

uint8_t levelTileSolid(int16_t tx, int16_t ty) {
    if (tx < 0 || tx >= LEVEL_COLUMNS) return 1; // level edges are walls
    if (ty < 0 || ty >= LEVEL_ROWS) return 0;    // open sky above, pits below
    return blockSolid[Level1_1[(ty >> 1) * Level1_1Width + (tx >> 1)]];
}

uint8_t levelColumnSolid(int16_t tx, int16_t top, int16_t bottom) {
    int16_t ty;
    for (ty = top >> 3; ty <= (bottom >> 3); ty++) {
        if (levelTileSolid(tx, ty)) return 1;
    }
    return 0;
}

uint8_t levelRowSolid(int16_t ty, int16_t left, int16_t right) {
    int16_t tx;
    for (tx = left >> 3; tx <= (right >> 3); tx++) {
        if (levelTileSolid(tx, ty)) return 1;
    }
    return 0;
}
