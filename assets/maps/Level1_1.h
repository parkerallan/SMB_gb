#ifndef __Level1_1_h_INCLUDE
#define __Level1_1_h_INCLUDE

#include <gbdk/platform.h>

// Size in 16x16 blocks
#define Level1_1Width 211
#define Level1_1Height 15

// In switchable ROM bank 1 (maps): switch to BANK(Level1_1) before reading it
BANKREF_EXTERN(Level1_1)
extern const unsigned char Level1_1[];

#endif
