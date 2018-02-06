#define NUM_PLANES 3
#define NUM_MAPCHARS 16 // 16th is null char

typedef struct WolfPlane
{
	u_int32_t offset, size;
	u_int16_t deSize;
	char* data;
} WolfPlane;

typedef struct WolfMap
{
	WolfPlane planes[NUM_PLANES];
	
	u_int16_t sizeX, sizeY;
	char name[NUM_MAPCHARS];
} WolfMap;

typedef struct WolfSet
{
	WolfMap* maps;
	
	int numLvls;
	u_int32_t* lvlOffs;
} WolfSet;

int wReadMapHead(char* const argv[], WolfSet* const wm);
int wReadGameMaps(char* const argv[], WolfSet* const wm);
int wDeCarmacize(char* const argv[], WolfSet* const wm);
