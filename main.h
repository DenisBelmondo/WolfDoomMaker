#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

typedef struct Args {
	char* MAPHEAD_PATH;
	char* GAMEMAPS_PATH;
} Args;

// all wolfenstein 3d maps have three planes and it's highly unlikely
// to encounter any wolf3d/wolf3d mod maps that have more than that but
// it's wise to externalize it for readability and future-proofing
// purposes. i tried to make my implementation plane-amount-agnostic.

#define NUM_PLANES 3

// 16th char (map.name[15]) is null terminator

#define NUM_MAPCHARS 16

typedef struct WolfPlane
{
	u_int32_t offset;
	u_int16_t size, deSize;
	
	u_int8_t* data, *deData;
} WolfPlane;

typedef struct WolfMap
{
	WolfPlane planes[NUM_PLANES];
	
	u_int16_t sizeX, sizeY;
	u_int32_t offset;
	
	char name[NUM_MAPCHARS];
} WolfMap;

typedef struct WolfSet
{
	WolfMap* maps;
	unsigned int numLvls;
} WolfSet;

// main methods
// COOL THING TO TRY? would it be worth it to unionize the args so that
// a lot of this repetition can be avoided?

int wReadMapHead (char* const[], WolfSet* const);
int wReadGameMaps(char* const[], WolfSet* const);
int wCarmackExpand(WolfSet* const, unsigned int lvl, unsigned int pl);
int wDeCarmacize (char* const[], WolfSet* const);

#endif // MAIN_H_INCLUDED
