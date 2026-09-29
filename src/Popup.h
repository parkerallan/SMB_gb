#ifndef POPUP_H
#define POPUP_H

#include <stdint.h>
#include <gb/gb.h>

// SMB's small floating score numbers ("100", "1000", "1UP"...)

#define POPUP_1UP 0 // pass as `points` for a floating "1UP"

// Load the number tiles
void popupsInit(void) BANKED;

// Clear them (level restart)
void popupsReset(void) BANKED;

// Float `points` up from a level position. Handles 100, 200, 400, 500, 800,
// 1000, 2000, 4000, 5000, 8000 and POPUP_1UP.
void popupShow(int16_t x, int16_t y, uint16_t points) BANKED;

void popupsUpdate(void) BANKED;
void popupsDraw(void) BANKED;

#endif
