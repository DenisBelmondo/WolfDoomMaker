#include <stdio.h>
#include <stdlib.h>

#include <string.h>

/*
	if by any chance there's an engine or a map that supports n planes,
	it's probably best if i externalize the number of planes and it just
	makes it more readable overall so
*/
#define NUM_PLANES 3
#define MAPNAME_SIZE 16

/*
	wl6 level structs
*/

typedef struct {
	// used during initial GAMEMAPS info gathering
	u_int32_t offset;
	u_int16_t size;
	
	// this is for carmacization
	u_int16_t deSize; // deSize = decompressed size
	u_int16_t** data; // dynamic 2d array. should it be char?
} PlaneWL6;

typedef struct MapWL6 {
	u_int16_t sizeX, sizeY;
	char name[MAPNAME_SIZE];
	
	PlaneWL6 Planes[NUM_PLANES];
} MapWL6;

/*
	wl6 file structs
*/

typedef struct {
	u_int16_t magic;
	
	u_int32_t* lvlOffs;
	unsigned int numLvls; // track the number of levels
} MapHeadWL6;

typedef struct GameMapsWL6
{	
	MapHeadWL6 MapHead;
	MapWL6* Maps;
	
} GameMapsWL6;

/*
	wolf types
*/

	// placeholder LMAO

/*
	doom types
*/

typedef struct {
	double x, y;
} VertexDMap;

typedef struct {
	char* texture;
	VertexDMap* V1, V2;
} LineDefDMap;

typedef struct {
	LineDefDMap* Lines;
	unsigned int tag;
} SectorDMap;

/* doom types */

typedef struct MapDMap
{
	SectorDMap* Sectors;
	
} MapDMap;

/*
	prototypes come after typedefs or it'll throw an error
*/

int wReadMapHead(FILE* fpin, GameMapsWL6* GameMaps);
int wReadGameMaps(FILE* fpin, GameMapsWL6* GameMaps);

int wDeCarmacize(FILE* fpin, GameMapsWL6* GameMaps);
int wDeRLEW(FILE* fpin, GameMapsWL6* GameMaps);
