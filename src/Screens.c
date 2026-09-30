#include <gb/gb.h>
#include <gb/metasprites.h>
#include "Screens.h"
#include "Font.h"
#include "Metasprites.h"
#include "Sound.h"
#include "TitleMap.h"
#include "SceneryTiles.h"
#include "MiscBackgroundTiles.h"
#include "Sprites.h"
#include "Util.h"

#define WORLD_SCREEN_FRAMES 150
#define GAME_OVER_FRAMES 180

// Blank the visible part of the background (the HUD window stays on top)
static void clearScreen(void) {
    move_bkg(0, 0);
    fill_bkg_rect(0, 0, 20, 18, FONT_BLANK_TILE);
}

// The title is drawn with the game's sheets: scenery from tile 0, the logo
// pieces right after it, and the font at the top (see TitleMap.h)
void titleScreen(void) {
    uint8_t logo[TitleLogoTileCount];
    uint8_t i;

    bankedMemcpy(logo, TitleLogoTiles, sizeof(logo), BANK(TitleMap)); // which logo pieces
    bankedSetBkgData(0, SceneryTilesCount, SceneryTiles, BANK(SceneryTiles));
    for (i = 0; i < TitleLogoTileCount; i++) {
        bankedSetBkgData(SceneryTilesCount + i, 1, MiscBackgroundTiles + logo[i] * 16, BANK(MiscBackgroundTiles));
    }
    fontLoad();
    // tiles are in place: now the layout
    bankedSetBkgTiles(0, 0, TitleMapWidth, TitleMapHeight, TitleMap, BANK(TitleMap));

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
    // Mario icon in the first 4 hardware sprites; hide leftovers from play
    move_metasprite_ex(mario_metasprite, SPR_TILE_MARIO, 0, 0, 56 + SPRITE_OFFSET_X, 76 + SPRITE_OFFSET_Y);
    hide_sprites_range(4, MAX_HARDWARE_SPRITES);
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

    musicPlay(MUSIC_GAME_OVER);
    waitFrames(GAME_OVER_FRAMES);
    while (musicPlaying()) wait_vbl_done();
}
