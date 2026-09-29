#include <gb/gb.h>
#include "Util.h"

void waitFrames(uint8_t frames) {
    while (frames--) {
        wait_vbl_done();
    }
}
