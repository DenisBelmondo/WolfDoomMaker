#include "main.h"

/*
	entry point
*/

int main(int argc, char* argv[])
{
	FILE* fpin;
	GameMapsWL6 GameMaps;
	
	if ((fpin = fopen(argv[1], "r")))
	{
		if (!wReadMapHead(fpin, &GameMaps)) {
			fclose(fpin);
			if ((fpin = fopen(argv[2], "r"))) {
				wReadGameMaps(fpin, &GameMaps);
				wDeCarmacize(fpin, &GameMaps);
				fclose(fpin);
			} else {
				puts("GAMEMAPS file not found.");
			}
			
			free(GameMaps.Maps);
		}
	}
	else
	{
		puts("Please specify a MAPHEAD file.");
	}
	
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
		return 1; // return error if the RLEW tag is not found
	}
	else
	{	
		// initial allocation for maps struct
		GameMaps->MapHead.lvlOffs = (u_int32_t *)calloc(1, sizeof(u_int32_t));
		GameMaps->Maps = (MapWL6 *)calloc(1, sizeof(MapWL6));
		
		printf("MAPHEAD INFO:\n\n");
		
		// numLvls is simply a shorthand
		GameMaps->MapHead.numLvls = 0;
		
		/*
			const unsigned int*: the value being pointed to is a constant
			unsigned integer.
			
			unsigned int* const: the value being pointed to is a non-const 
			unsigned int but the pointer cannot point to anything else
			because it itself is a const.
		*/
		
		// just a shorthand for readability
		unsigned int* const numLvls = &GameMaps->MapHead.numLvls;
		
		// read file
		while(!feof(fpin))
		{
			// realloc numlvls + 1 because we don't wanna alloc 0 elements when numlvls = 0.
			GameMaps->Maps = (MapWL6 *)realloc(GameMaps->Maps,
				sizeof(MapWL6) * (*numLvls + 1)
			);
			
			GameMaps->MapHead.lvlOffs = (u_int32_t *)realloc(
				GameMaps->MapHead.lvlOffs, sizeof(u_int32_t) * (*numLvls + 1));
			
			fread(&GameMaps->MapHead.lvlOffs[*numLvls], 1,
					sizeof(u_int32_t), fpin);
			
			printf("Map entry %u: 0x%X\n",
					*numLvls, GameMaps->MapHead.lvlOffs[*numLvls]);
			
			++(*numLvls);
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
							
						printf("Plane %d offset: 0x%X\n", curPlane,
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
			for(curPlane = 0; curPlane < 4; ++curPlane)
			{
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

#define TEMPFILE "TEMP"

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
	
	// UNSIGNED CHAR IS IMPORTANT!!!
	// https://stackoverflow.com/questions/31090616/printf-adds-extra-ffffff-to-hex-print-from-a-char-array
	unsigned char ch;
	
	// go to where the map data starts 
	fseek(fpin, GameMaps->Maps[0].Planes[0].offset, SEEK_SET);
	
	// instruction buffer
	char instr[4];
	
	while(!(feof(fpin)))
	{
		fread(&ch, 1, 1, fpin); // read a byte
		/*if (ch == NEARTAG) {
			printf("NEARTAG 0x%X detected at 0x%lX\n", NEARTAG, ftell(fpin));
		} else if (ch == FARTAG) {
			printf("FARTAG 0x%X detected at 0x%lX\n", FARTAG, ftell(fpin));
		}*/
		
		switch(ch) {
			case NEARTAG:
				
				break;
			case FARTAG:
				break;
			default:
				break;
		}
	}
	
	return 0;
}

int wDeRLEW(FILE* fpin, GameMapsWL6* GameMaps)
{
	FILE* fpout = fopen(TEMPFILE, "w");
	
	
	
	fclose(fpout);
	
	return 0;
}
