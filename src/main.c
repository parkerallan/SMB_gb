#include <gb/gb.h>
#include "Game.h"
#include "Screens.h"
#include "Camera.h"

void main(void) {
    DISPLAY_ON;
    SHOW_BKG;

    titleScreen();
    gameInit();
    gameNew();
    gameEnterLevel();

    while (1) {
        gameUpdate();
        wait_vbl_done();
        cameraApply();
    }
}
