#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_PLANES 3

typedef struct WolfPlane
{
	u_int32_t offset, size;
} WolfPlane;

typedef struct WolfMap
{
	WolfPlane planes[NUM_PLANES];
} WolfMap;

typedef struct WolfSet
{
	int numLvls;
	u_int32_t* lvlOffs;
	
	WolfMap* maps;
} WolfSet;

int wReadMapHead(char* const argv[], WolfSet* const wm);
int wReadGameMaps(char* const argv[], WolfSet* const wm);
