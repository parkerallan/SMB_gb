#include <gb/gb.h>
#include <gb/metasprites.h>
#include "Screens.h"
#include "Font.h"
#include "Metasprites.h"
#include "WorldTiles.h"
#include "TitleMap.h"
#include "Util.h"

#define WORLD_TILE_COUNT 123
#define WORLD_SCREEN_FRAMES 150
#define GAME_OVER_FRAMES 180
// Hardware sprite coordinates are offset from the screen by (8, 16)
#define SPRITE_OFFSET_X 8
#define SPRITE_OFFSET_Y 16

// Blank the visible part of the background (the HUD window stays on top)
static void clearScreen(void) {
    move_bkg(0, 0);
    fill_bkg_rect(0, 0, 20, 18, FONT_BLANK_TILE);
}

void titleScreen(void) {
    set_bkg_data(0, WORLD_TILE_COUNT, WorldTiles);
    set_bkg_tiles(0, 0, 20, 18, TitleMap);
    waitpad(J_START | J_A);
}

// Layout, like SMB:
//        WORLD 1-1
//
//      [M]  x  3
void worldScreen(int8_t world, int8_t level, int8_t lives) {
    char text[] = "WORLD 0-0";
    text[6] = '0' + world;
    text[8] = '0' + level;

    DISPLAY_OFF;
    clearScreen();
    fontPrint(5, 7, text);
    fontPrint(10, 10, "x");
    if (lives >= 10) {
        text[0] = '0' + lives / 10;
        text[1] = '0' + lives % 10;
        text[2] = 0;
    } else {
        text[0] = '0' + lives;
        text[1] = 0;
    }
    fontPrint(12, 10, text);
    move_metasprite_ex(mario_metasprite, 0, 0, 0, 56 + SPRITE_OFFSET_X, 76 + SPRITE_OFFSET_Y);
    SHOW_SPRITES;
    DISPLAY_ON;

    waitFrames(WORLD_SCREEN_FRAMES);
}

void gameOverScreen(void) {
    DISPLAY_OFF;
    HIDE_SPRITES;
    clearScreen();
    fontPrint(5, 8, "GAME OVER");
    DISPLAY_ON;

    waitFrames(GAME_OVER_FRAMES);
}
