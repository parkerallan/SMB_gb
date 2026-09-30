#include <gb/gb.h>
#include "Util.h"
#include <string.h>
#include "Level.h"
#include "Font.h"

// The VRAM background map is 32 tiles wide and wraps around
#define VRAM_COLUMNS 32

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

#if Level1_1BonusHeight != Level1_1Height || Level1_2IntroHeight != Level1_1Height || \
    Level1_2Height != Level1_1Height || Level1_2BonusHeight != Level1_1Height
#error "every area is as tall as 1-1"
#endif
#if Level1_1BonusWidth > Level1_1Width || Level1_2IntroWidth > Level1_1Width || \
    Level1_2Width > Level1_1Width || Level1_2BonusWidth > Level1_1Width
#error "no area is wider than 1-1 (see map[] below)"
#endif

#define MAX_TEXT 32 // characters of levelAddText

uint8_t levelArea;
uint8_t levelWidth;
int16_t levelColumns;
int16_t levelPixelWidth;
uint8_t levelUnderground;
uint8_t levelCoins;

static uint8_t blockSolid[SceneryMetatileCount];
static uint8_t blockTiles[SceneryMetatileCount * 4]; // copy of SceneryMetatiles, read every column
// Working copy of the area's map: hit blocks change it, and it's refilled from
// ROM every time the area is entered (sized for the biggest area, 1-1)
static uint8_t map[Level1_1Width * Level1_1Height];
// Start of each block row in `map`: the Game Boy has no multiply instruction,
// and these lookups run many times a frame for Mario and every enemy
static uint16_t rowStart[LEVEL_BLOCK_ROWS];
// Each area only uses some of the scenery blocks, and their tiles fit in the
// background-only half of video memory (tiles 0-127), which leaves tiles
// 128-155, shared with sprites, free for sprites. These are for picking them.
static uint8_t blockUsed[SceneryMetatileCount];
static uint8_t tileSlot[SceneryTilesCount];
static uint8_t freeTile; // the next free background tile after the area's
// levelAddText's characters: tile column, row, tile; and the columns they span
static int16_t textColumn[MAX_TEXT];
static uint8_t textRow[MAX_TEXT], textTile[MAX_TEXT];
static uint8_t textCount;
static int16_t textFirst, textLast;
static int16_t firstColumn, lastColumn;               // range of level columns currently in VRAM
static uint8_t columnBuffer[LEVEL_ROWS];

void levelInit(void) {
    uint8_t i;
    bankedMemcpy(blockTiles, SceneryMetatiles, sizeof(blockTiles), BANK(SceneryTiles));
    for (i = 0; i < sizeof(solidBlocks); i++) blockSolid[solidBlocks[i]] = 1;
}

const uint8_t *levelBlockTiles(uint8_t block) {
    return blockTiles + block * 4;
}

// Copy one full-height level column (half a block column) into the wrapping VRAM map
static void loadColumn(int16_t column) {
    const uint8_t *src = map + (column >> 1);
    uint8_t half = column & 1; // 0 = left tiles of each block, 1 = right tiles
    uint8_t row;
    const uint8_t *tiles;
    for (row = 0; row < LEVEL_ROWS; row += 2) {
        tiles = blockTiles + *src * 4 + half;
        columnBuffer[row] = tiles[0];
        columnBuffer[row + 1] = tiles[2];
        src += levelWidth;
    }
    if (textCount && column >= textFirst && column <= textLast) {
        for (row = 0; row < textCount; row++) {
            if (textColumn[row] == column) columnBuffer[textRow[row]] = textTile[row];
        }
    }
    set_bkg_tiles(column & (VRAM_COLUMNS - 1), 0, 1, LEVEL_ROWS, columnBuffer);
}

// Load the tiles of the blocks in blockUsed into video memory, one copy of each,
// from tile 0 up, and point blockTiles at them
static void loadSceneryTiles(void) {
    uint8_t b, k, t;
    uint8_t *tiles = blockTiles;
    memset(tileSlot, 0xFF, sizeof(tileSlot));
    bankedMemcpy(blockTiles, SceneryMetatiles, sizeof(blockTiles), BANK(SceneryTiles));
    freeTile = 0;
    for (b = 0; b < SceneryMetatileCount; b++, tiles += 4) {
        if (!blockUsed[b]) continue;
        for (k = 0; k < 4; k++) {
            t = tiles[k];
            if (tileSlot[t] == 0xFF) {
                tileSlot[t] = freeTile;
                bankedSetBkgData(freeTile, 1, SceneryTiles + t * 16, BANK(SceneryTiles));
                freeTile++;
            }
            tiles[k] = tileSlot[t];
        }
    }
}

void levelLoad(uint8_t area, int16_t cameraX) {
    uint8_t i;
    uint16_t start = 0;
    const uint8_t *cell, *end;
    const uint8_t *source;
    uint8_t bank;
    levelArea = area;
    levelUnderground = 1;
    switch (area) {
    case AREA_1_1_BONUS:
        source = Level1_1Bonus; bank = BANK(Level1_1Bonus); levelWidth = Level1_1BonusWidth;
        break;
    case AREA_1_2_INTRO:
        source = Level1_2Intro; bank = BANK(Level1_2Intro); levelWidth = Level1_2IntroWidth;
        levelUnderground = 0;
        break;
    case AREA_1_2:
        source = Level1_2; bank = BANK(Level1_2); levelWidth = Level1_2Width;
        break;
    case AREA_1_2_BONUS:
        source = Level1_2Bonus; bank = BANK(Level1_2Bonus); levelWidth = Level1_2BonusWidth;
        break;
    default:
        source = Level1_1; bank = BANK(Level1_1); levelWidth = Level1_1Width;
        levelUnderground = 0;
        break;
    }
    // (widened first, then shifted: SDCC 4.4 drops the carry out of the low
    // byte for `(int16_t)levelWidth << 1`, which broke widths over 127 blocks)
    levelColumns = levelWidth;
    levelColumns <<= 1;
    levelPixelWidth = levelColumns << 3;
    for (i = 0; i < LEVEL_BLOCK_ROWS; i++, start += levelWidth) rowStart[i] = start;
    bankedMemcpy(map, source, start, bank);

    // Which blocks it uses (plus the ones hit blocks turn into), and its loose
    // coins. (A pointer loop: indexing made level starts noticeably slower.)
    memset(blockUsed, 0, sizeof(blockUsed));
    blockUsed[SCENERY_BLANK] = 1;
    blockUsed[SCENERY_USED_BLOCK] = 1;
    levelCoins = 0;
    for (cell = map, end = map + start; cell != end; cell++) {
        blockUsed[*cell] = 1;
        if (*cell == SCENERY_COIN) levelCoins++;
    }
    loadSceneryTiles();
    textCount = 0;
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

    // The camera jumped (e.g. a warp): skip straight to the columns now on screen
    // instead of drawing every column in between
    if (lastNeeded - lastColumn >= VRAM_COLUMNS || firstColumn - firstNeeded >= VRAM_COLUMNS) {
        firstColumn = firstNeeded;
        lastColumn = firstNeeded;
        loadColumn(firstColumn);
    }

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
    if ((uint16_t)bx >= levelWidth || (uint16_t)by >= LEVEL_BLOCK_ROWS) return SCENERY_BLANK;
    return map[rowStart[by] + bx];
}

void levelSetBlock(int16_t bx, int16_t by, uint8_t block) {
    const uint8_t *tiles = blockTiles + block * 4;
    int16_t tx = bx << 1, ty = by << 1;
    map[rowStart[by] + bx] = block;
    levelSetTile(tx, ty, tiles[0]);
    levelSetTile(tx + 1, ty, tiles[1]);
    levelSetTile(tx, ty + 1, tiles[2]);
    levelSetTile(tx + 1, ty + 1, tiles[3]);
}

void levelAddText(int16_t tx, uint8_t ty, const char *text) {
    uint8_t glyph[16];
    uint8_t i, t;
    for (; *text; text++, tx++) {
        if (*text == ' ' || textCount == MAX_TEXT) continue;
        // the font's ink is color 3, black on the HUD's white; underground the
        // background is black too, so this copy uses color 1 (light) instead
        t = fontTile(*text) - FONT_FIRST_TILE;
        bankedMemcpy(glyph, FontTiles + t * 16, 16, BANK(FontTiles));
        for (i = 1; i < 16; i += 2) glyph[i] = 0;
        set_bkg_data(freeTile, 1, glyph);
        if (!textCount || tx < textFirst) textFirst = tx;
        if (!textCount || tx > textLast) textLast = tx;
        textColumn[textCount] = tx;
        textRow[textCount] = ty;
        textTile[textCount] = freeTile;
        textCount++;
        levelSetTile(tx, ty, freeTile++); // if it's already on screen
    }
}

void levelSetTile(int16_t tx, int16_t ty, uint8_t tile) {
    if (tx >= firstColumn && tx <= lastColumn) {
        set_bkg_tile_xy(tx & (VRAM_COLUMNS - 1), ty, tile);
    }
}

uint8_t levelTileSolid(int16_t tx, int16_t ty) {
    if ((uint16_t)tx >= LEVEL_COLUMNS) return 1; // level edges are walls (negatives wrap huge)
    if ((uint16_t)ty >= LEVEL_ROWS) return 0;    // open sky above, pits below
    return blockSolid[map[rowStart[ty >> 1] + (tx >> 1)]];
}

// Solidity is per 16x16 block, so these check whole blocks (a 16-pixel body
// spans at most 2) and look the block row up once. They run several times a
// frame for Mario and every enemy, so they avoid per-tile function calls.
uint8_t levelColumnSolid(int16_t tx, int16_t top, int16_t bottom) {
    int16_t bx = tx >> 1;
    int16_t by, byEnd;
    const uint8_t *cell;
    if ((uint16_t)bx >= levelWidth) return 1;   // level edges are walls
    if (bottom < 0) return 0;                    // entirely above the level
    by = (top < 0) ? 0 : (top >> 4);
    byEnd = bottom >> 4;
    if (byEnd >= LEVEL_BLOCK_ROWS) byEnd = LEVEL_BLOCK_ROWS - 1; // pits below
    cell = map + rowStart[by] + bx;
    for (; by <= byEnd; by++, cell += levelWidth) {
        if (blockSolid[*cell]) return 1;
    }
    return 0;
}

uint8_t levelRowSolid(int16_t ty, int16_t left, int16_t right) {
    int16_t by = ty >> 1;
    int16_t bx, bxEnd;
    const uint8_t *row;
    if ((uint16_t)by >= LEVEL_BLOCK_ROWS) return 0; // open sky above, pits below
    bx = left >> 4;
    bxEnd = right >> 4;
    if (bx < 0 || bxEnd >= levelWidth) return 1; // level edges are walls
    row = map + rowStart[by];
    for (; bx <= bxEnd; bx++) {
        if (blockSolid[row[bx]]) return 1;
    }
    return 0;
}
