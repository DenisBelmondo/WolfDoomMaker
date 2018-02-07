#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

// all wolfenstein 3d maps have three planes and it's highly unlikely
// to encounter any wolf3d maps that have more than that but it's wise
// to externalize it for readability purposes
#define NUM_PLANES 3
#define NUM_MAPCHARS 16 // 16th is null char

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
int wReadMapHead(char* const argv[], WolfSet* const);
int wReadGameMaps(char* const argv[], WolfSet* const);
int wDeCarmacize(char* const argv[], WolfSet* const);

#endif // MAIN_H_INCLUDED
