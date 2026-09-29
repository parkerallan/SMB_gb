/*
 TITLEMAP.H

 The title screen you designed, drawn with the game's tile sheets. Tile numbers
 assume this video memory layout (see titleScreen):
   0 ..                 SceneryTiles (sky, hill, pipe, ground)
   SceneryTilesCount .. the MiscBackgroundTiles listed in TitleLogoTiles (logo)
   FONT_FIRST_TILE ..   FontTiles ("PRESS START")
*/
#ifndef __TitleMap_h_INCLUDE
#define __TitleMap_h_INCLUDE

#include <gbdk/platform.h>

#define TitleMapWidth 20
#define TitleMapHeight 18
#define TitleLogoTileCount 34

// In switchable ROM bank 1 (maps): switch to BANK(TitleMap) before reading either array
BANKREF_EXTERN(TitleMap)
extern const unsigned char TitleMap[];
extern const unsigned char TitleLogoTiles[]; // MiscBackgroundTiles indexes, loaded in order

#endif
