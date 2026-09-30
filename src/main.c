#include <gb/gb.h>
#include "Game.h"
#include "Screens.h"
#include "Camera.h"
#include "Sound.h"

void main(void) {
    DISPLAY_ON;
    SHOW_BKG;

    titleScreen();
    gameInit();
    // after gameInit: the HUD's vblank handler (it switches the window on) has
    // to run before the sound player's, or the HUD comes on late and flickers
    soundInit();
    gameNew();
    gameEnterLevel();

    while (1) {
        gameUpdate();
        wait_vbl_done();
        cameraApply();
    }
}
