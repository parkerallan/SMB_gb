#include <gb/gb.h>
#include <string.h>
#include "Util.h"

void waitFrames(uint8_t frames) {
    while (frames--) {
        wait_vbl_done();
    }
}

void bankedMemcpy(void *dst, const void *src, uint16_t length, uint8_t bank) {
    uint8_t saved = CURRENT_BANK;
    SWITCH_ROM(bank);
    memcpy(dst, src, length);
    SWITCH_ROM(saved);
}

void bankedSetSpriteData(uint8_t first, uint8_t count, const uint8_t *tiles, uint8_t bank) {
    uint8_t saved = CURRENT_BANK;
    SWITCH_ROM(bank);
    set_sprite_data(first, count, tiles);
    SWITCH_ROM(saved);
}

void bankedSetBkgData(uint8_t first, uint8_t count, const uint8_t *tiles, uint8_t bank) {
    uint8_t saved = CURRENT_BANK;
    SWITCH_ROM(bank);
    set_bkg_data(first, count, tiles);
    SWITCH_ROM(saved);
}

void bankedSetBkgTiles(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *map, uint8_t bank) {
    uint8_t saved = CURRENT_BANK;
    SWITCH_ROM(bank);
    set_bkg_tiles(x, y, w, h, map);
    SWITCH_ROM(saved);
}
