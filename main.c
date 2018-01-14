#include "main.h"

/*
	entry point
*/

int main(int argc, char* argv[])
{
	FILE* fpin;
	GameMapsWL6 GameMaps;
	
	if (argv[1] == NULL)
	{
		printf("File not found.\n");
	}
	else
	{
		fpin = fopen(argv[1], "r");
		
		if (!(wReadMapHead(fpin, &GameMaps)) && argv[2] == NULL) {
			printf("You must specify a GAMEMAPS file.\n");
		} else {
			fclose(fpin);
			fpin = fopen(argv[2], "r");
			wReadGameMaps(fpin, &GameMaps);
		}
		
		fclose(fpin);
	}
	
	// free(GameMaps.Maps);
	return 0;
}

/*
	file reading
*/

#define RLEW_TAG 0xABCD // RLEW magic number LE

int wReadMapHead(FILE* fpin, GameMapsWL6* GameMaps)
{
	fread(&GameMaps->MapHead.magic, 1, sizeof(u_int16_t), fpin);
	
	if (GameMaps->MapHead.magic != RLEW_TAG)
	{
		printf("MAPHEAD file invalid.\n");
		return 1; // return error
	}
	else
	{	
		// allocate memory for maps struct
		GameMaps->MapHead.lvlOffs = (u_int32_t *)calloc(1, sizeof(u_int32_t));
		GameMaps->Maps = (MapWL6 *)calloc(1, sizeof(MapWL6));
		
		printf("MAPHEAD INFO:\n\n");
		
		// numLvls is simply a shorthand
		GameMaps->MapHead.numLvls = 0;
		unsigned int* numLvls = &GameMaps->MapHead.numLvls;
		
		// read file
		while(!feof(fpin))
		{
			GameMaps->Maps = (MapWL6 *)realloc(GameMaps->Maps,
				sizeof(MapWL6) * (*numLvls + 1)
			);
			
			GameMaps->MapHead.lvlOffs = (u_int32_t *)realloc(
				GameMaps->MapHead.lvlOffs, sizeof(u_int32_t) * (*numLvls + 1));
			
			fread(&GameMaps->MapHead.lvlOffs[*numLvls], 1,
					sizeof(u_int32_t), fpin);
			
			printf("Map entry %u: 0x%x\n",
					*numLvls, GameMaps->MapHead.lvlOffs[*numLvls]);
			
			++*numLvls;
			
		}
	}
	
	return 0; // success!
}

int wReadGameMaps(FILE* fpin, GameMapsWL6* GameMaps)
{
	/*
		plane[curPlane] offset values appear consecutively in the file.
		they are then followed by the size values which are also
		contiguous.
	*/
	
	printf("\nGAMEMAPS INFO:\n\n");
	
	int curPlane; int curLvl = 0;
	
	while (!feof(fpin) && curLvl < GameMaps->MapHead.numLvls)
	{
		printf("<MAP %d:>\n", curLvl);
		
		if (GameMaps->MapHead.lvlOffs[curLvl] != 0x0)
		{
			fseek(fpin, GameMaps->MapHead.lvlOffs[curLvl], SEEK_SET);
			
			// read offsets and size
			for(curPlane = 0; curPlane < (NUM_PLANES * 2); ++curPlane)
			{
				if (curPlane < NUM_PLANES) { // read offsets
				
					fread(
						&GameMaps->Maps[curLvl].Planes[curPlane].offset, 1, 
							sizeof(u_int32_t), fpin);
							
						printf("Plane %d offset: 0x%x\n", curPlane,
							GameMaps->Maps[curLvl].Planes[curPlane].offset);
							
				} else if (curPlane >= NUM_PLANES) { // read compressed size
				
					fread(
						&GameMaps->Maps[curLvl].Planes[curPlane - NUM_PLANES].size,
							1, sizeof(u_int16_t), fpin);
							
						printf("Plane %d size: %u (compressed)\n",
							curPlane - NUM_PLANES,
							GameMaps->Maps[curLvl].Planes[curPlane - NUM_PLANES].size);
				}
			}
			
			// read rest of properties
			for(curPlane = 0; curPlane < 4; ++curPlane) {
				switch(curPlane) {
					case 0:
						fread(&GameMaps->Maps[curLvl].sizeX, 1, sizeof(u_int16_t), fpin);
						printf("SizeX: %d\n", GameMaps->Maps[curLvl].sizeX);
						break;
					case 1:
						fread(&GameMaps->Maps[curLvl].sizeY, 1, sizeof(u_int16_t), fpin);
						printf("SizeY: %d\n", GameMaps->Maps[curLvl].sizeY);
						break;
					case 2:
						fread(GameMaps->Maps[curLvl].name, MAPNAME_SIZE,
							sizeof(char), fpin);
						printf("Level Name: %s\n\n", GameMaps->Maps[curLvl].name);
						break;
					case 3:
						break;
				}
			}
		}
		else
		{
			printf("Null data...\n\n");
		}
		
		++curLvl;
	}
	
	return 0;
}

/*
	decompression
*/

#define TEMPFILE "temp.tmp"

#define NEARTAG 0xA7
#define FARTAG 	0xA8

int wDeCarmacize(FILE* fpin, GameMapsWL6* GameMaps)
{
	/*
		http://gaarabis.free.fr/_sites/specs/wlspec_index.html
		
		The near pointer "0x05 0xA7 0x0A", means "repeat the 5 words
		starting 10 words ago".
		
		The far pointer "0x10 0xA8 0x01 0x20", means "repeat the 16
		words starting at word number 513".
		
		PERSONAL ADDENDUM: 0x00 is the exception and marks the end
	*/
	
	// neartag/fartag vars
	u_int16_t	cpyCnt,		// number of words to copy
				cpyType, 	// pointer type (could be 0xA7 or 0xA8)
				cpyOff;		// relative offset of the first word to copy
				
	// size of the decompressed buffer
	u_int16_t size;
	
	FILE* fpout = fopen(TEMPFILE, "w");
	
	// assess the size of the of the planes
	int i; int j = 0; do
	{
		for(i = 0; i < NUM_PLANES; ++i) {
			if (GameMaps->Maps[j].Planes[i].size > 1) { // null plane 3's have size of 1
				size += GameMaps->Maps[j].Planes[i].size;
			}
		} printf("Size of buffer: %u (compressed)\n", size);
		
		++j;
	} while(!feof(fpin));
	
	fclose(fpout);
	if (remove(TEMPFILE)) {
		printf("\nCould not delete %s, continuing...\n", TEMPFILE); }
	else {
		printf("\n%s deleted successfully.\n", TEMPFILE); }
	
	return 0;
}

int wDeRLEW(FILE* fpin, GameMapsWL6* GameMaps)
{
	FILE* fpout = fopen(TEMPFILE, "w");
	
	
	
	fclose(fpout);
	
	return 0;
}
