#include <gb/metasprites.h>

const metasprite_t mario_metasprite[] = {
    METASPR_ITEM(0, 0, 0, 0),  // First sprite (top-left)
    METASPR_ITEM(8, 0, 1, 0),  // Second sprite (top-right)
    METASPR_ITEM(-8, 8, 2, 0),  // Third sprite (bottom-left)
    METASPR_ITEM(8, 0, 3, 0),  // Fourth sprite (bottom-right)
    {metasprite_end}
};

const metasprite_t mario_jump_metasprite[] = {
    METASPR_ITEM(0, 0, 16, 0),  // First sprite (top-left)
    METASPR_ITEM(8, 0, 17, 0),  // Second sprite (top-right)
    METASPR_ITEM(-8, 8, 18, 0),  // Third sprite (bottom-left)
    METASPR_ITEM(8, 0, 19, 0),  // Fourth sprite (bottom-right)
    {metasprite_end}               
};

const metasprite_t mario_walk_frame1[] = {
    METASPR_ITEM(0, 0, 4, 0),  // First sprite (top-left)
    METASPR_ITEM(8, 0, 5, 0),  // Second sprite (top-right)
    METASPR_ITEM(-8, 8, 6, 0),  // Third sprite (bottom-left)
    METASPR_ITEM(8, 0, 7, 0),  // Fourth sprite (bottom-right)
    {metasprite_end}
};

const metasprite_t mario_walk_frame2[] = {
    METASPR_ITEM(0, 0, 8, 0),  // First sprite (top-left)
    METASPR_ITEM(8, 0, 9, 0),  // Second sprite (top-right)
    METASPR_ITEM(-8, 8, 10, 0),  // Third sprite (bottom-left)
    METASPR_ITEM(8, 0, 11, 0),  // Fourth sprite (bottom-right)
    {metasprite_end}
};

const metasprite_t mario_walk_frame3[] = {
    METASPR_ITEM(0, 0, 12, 0),  // First sprite (top-left)
    METASPR_ITEM(8, 0, 13, 0),  // Second sprite (top-right)
    METASPR_ITEM(-8, 8, 14, 0),  // Third sprite (bottom-left)
    METASPR_ITEM(8, 0, 15, 0),  // Fourth sprite (bottom-right)
    {metasprite_end}
};

// Big Mario, 16x32: 8 tiles row by row (TL, TR, then down). Every big frame
// uses this layout; the frame is picked by the base tile passed when drawing.
const metasprite_t big_mario_metasprite[] = {
    METASPR_ITEM(0, 0, 0, 0),
    METASPR_ITEM(0, 8, 1, 0),
    METASPR_ITEM(8, -8, 2, 0),
    METASPR_ITEM(0, 8, 3, 0),
    METASPR_ITEM(8, -8, 4, 0),
    METASPR_ITEM(0, 8, 5, 0),
    METASPR_ITEM(8, -8, 6, 0),
    METASPR_ITEM(0, 8, 7, 0),
    {metasprite_end}
};
