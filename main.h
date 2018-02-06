#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#define NUM_PLANES 3
#define NUM_MAPCHARS 16 // 16th is null char

typedef struct WolfPlane
{
	u_int32_t offset;
	u_int16_t size, deSize;
	char* data;
} WolfPlane;

typedef struct WolfMap
{
	WolfPlane planes[NUM_PLANES];
	
	u_int16_t sizeX, sizeY;
	u_int32_t* offset;
	char name[NUM_MAPCHARS];
} WolfMap;

typedef struct WolfSet
{
	WolfMap* maps;
	
	int numLvls;
} WolfSet;

// main methods
int wReadMapHead(char* const argv[], WolfSet* const);
int wReadGameMaps(char* const argv[], WolfSet* const);
int wDeCarmacize(char* const argv[], WolfSet* const);

#endif
