#include <gb/gb.h>
#include "Hud.h"
#include "Font.h"

#define HUD_ROWS 2
#define HUD_COLS 20

// Layout, modeled on SMB's top bar (labels on the first row, values below):
//   " MARIO x4 WORLD TIME"
//   " 000000    1-1   400"
#define HUD_NAME_X   1
#define HUD_LIVES_X  7
#define HUD_WORLD_X  10
#define HUD_TIME_X   16
#define HUD_SCORE_X  1
#define HUD_LEVEL_X  11
#define HUD_CLOCK_X  17

static uint8_t hudMap[HUD_COLS * HUD_ROWS];

static void hudText(uint8_t x, uint8_t y, const char *text) {
    while (*text) {
        hudMap[y * HUD_COLS + x++] = fontTile(*text++);
    }
}

// Write value as exactly `digits` digits, zero padded
static void hudNumber(uint8_t x, uint8_t y, uint16_t value, uint8_t digits) {
    uint8_t *dst = hudMap + y * HUD_COLS + x + digits;
    while (digits--) {
        *--dst = fontTile('0' + (value % 10));
        value /= 10;
    }
}

static void hudClearRow(uint8_t y) {
    uint8_t i;
    for (i = 0; i < HUD_COLS; i++) hudMap[y * HUD_COLS + i] = FONT_BLANK_TILE;
}

// The window covers the whole screen below WY, so it is switched off after
// the HUD's last line and back on at the start of every frame.
static void hudHideWindow(void) {
    HIDE_WIN;
}

static void hudShowWindow(void) {
    SHOW_WIN;
}

void hudInit(void) {
    fontLoad();
    hudClearRow(0);
    hudClearRow(1);
    hudText(HUD_NAME_X, 0, "MARIO");
    hudText(HUD_WORLD_X, 0, "WORLD");
    hudText(HUD_TIME_X, 0, "TIME");
    move_win(7, 0);

    CRITICAL {
        LYC_REG = HUD_PIXEL_HEIGHT - 1;
        STAT_REG = STATF_LYC;
        add_LCD(hudHideWindow);
        add_VBL(hudShowWindow);
    }
    set_interrupts(IE_REG | LCD_IFLAG);
}

void hudUpdate(uint16_t score, int8_t lives, int8_t world, int8_t level, uint16_t time) {
    hudText(HUD_LIVES_X, 0, "x");
    hudNumber(HUD_LIVES_X + 1, 0, lives, 1);
    hudNumber(HUD_SCORE_X, 1, score, 6);
    hudNumber(HUD_LEVEL_X, 1, world, 1);
    hudText(HUD_LEVEL_X + 1, 1, "-");
    hudNumber(HUD_LEVEL_X + 2, 1, level, 1);
    hudNumber(HUD_CLOCK_X, 1, time, 3);
    set_win_tiles(0, 0, HUD_COLS, HUD_ROWS, hudMap);
}

// Redraw only the clock digits; cheaper than a full hudUpdate() so the
// per-tick update fits in the frame
void hudUpdateTime(uint16_t time) {
    hudNumber(HUD_CLOCK_X, 1, time, 3);
    set_win_tiles(HUD_CLOCK_X, 1, 3, 1, hudMap + HUD_COLS + HUD_CLOCK_X);
}
