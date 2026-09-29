#include <gb/gb.h>
#include "Game.h"
#include "Screens.h"
#include "Player.h"
#include "Camera.h"
#include "Hud.h"

void main(void) {
    DISPLAY_ON;
    SHOW_BKG;

    titleScreen();
    hudInit();
    playerInit();
    gameNew();
    gameEnterLevel();

    while (1) {
        gameUpdate();
        wait_vbl_done();
        cameraApply();
    }
}
