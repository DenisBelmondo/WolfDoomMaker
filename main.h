#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

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
	unsigned char* data;
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

int wReadMapHead(char* const argv[], WolfSet* const);
int wReadGameMaps(char* const argv[], WolfSet* const);
int wParseCarmack(char* const argv[], WolfSet* const);
int wDeCarmacize(char* const argv[], WolfSet* const);

#endif // MAIN_H_INCLUDED
