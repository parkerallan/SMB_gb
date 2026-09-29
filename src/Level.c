#include <gb/gb.h>
#include "Level.h"
#include "WorldTiles.h"

#define WORLD_TILE_COUNT 123
// Everything from this column on is the castle, which is scenery
#define CASTLE_START_COL 402
// The VRAM background map is 32 tiles wide and wraps around
#define VRAM_COLUMNS 32

// Scenery: sky, hills, bushes, flagpole/flag, clouds and castle pieces
static const uint8_t nonCollidableTiles[] = {
    0x00, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C,
    0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34,
    0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x4F
};

static uint8_t solidTile[256];
static uint8_t solidTileReady;
static int16_t firstColumn, lastColumn; // range of level columns currently in VRAM
static uint8_t columnBuffer[LEVEL_ROWS];

static void initSolidTiles(void) {
    uint8_t i = 0, j;
    do {
        solidTile[i] = 1;
        for (j = 0; j < sizeof(nonCollidableTiles); j++) {
            if (nonCollidableTiles[j] == i) solidTile[i] = 0;
        }
    } while (++i);
    solidTileReady = 1;
}

// Copy one full-height level column into the wrapping VRAM map
static void loadColumn(int16_t column) {
    const uint8_t *src = Level1_1 + column;
    uint8_t row;
    for (row = 0; row < LEVEL_ROWS; row++) {
        columnBuffer[row] = *src;
        src += LEVEL_COLUMNS;
    }
    set_bkg_tiles(column & (VRAM_COLUMNS - 1), 0, 1, LEVEL_ROWS, columnBuffer);
}

void levelLoad(int16_t cameraX) {
    if (!solidTileReady) initSolidTiles();
    set_bkg_data(0, WORLD_TILE_COUNT, WorldTiles);
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

uint8_t levelTileSolid(int16_t tx, int16_t ty) {
    uint8_t tile;
    if (tx < 0 || tx >= LEVEL_COLUMNS) return 1; // level edges are walls
    if (ty < 0 || ty >= LEVEL_ROWS) return 0;    // open sky above, pits below
    tile = Level1_1[ty * LEVEL_COLUMNS + tx];
    if (tx >= CASTLE_START_COL) return tile >= 0x01 && tile <= 0x04; // only ground by the castle
    return solidTile[tile];
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
