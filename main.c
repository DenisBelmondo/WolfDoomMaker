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
		GameMaps->MapHead.lvlOffs =(u_int32_t *)calloc(1, sizeof(u_int32_t));
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
		plane[i] offset values appear consecutively in the file.
		they are then followed by the size values which are also
		contiguous. so it ends up looking something like:
		plane0off, plane1off, plane2off, plane0size, plane1size, plane2size
	*/
	
	printf("\nGAMEMAPS INFO:\n\n");
	
	// shorthands for readability
	MapHeadWL6* const MapHead = &GameMaps->MapHead;
	MapWL6* const Maps = GameMaps->Maps;
	
	int plane; int level = 0;
	while (!feof(fpin) && level < MapHead->numLvls)
	{
		printf("<MAP %d:>\n", level);
		
		if (MapHead->lvlOffs[level] != 0x0)
		{
			fseek(fpin, MapHead->lvlOffs[level], SEEK_SET);
			
			// read offsets and size
			for(plane = 0; plane < (NUM_PLANES * 2); ++plane)
			{
				if (plane < NUM_PLANES) { // read offsets
				
					fread(
						&Maps[level].Planes[plane].offset, 1, 
							sizeof(u_int32_t), fpin);
							
						printf("Plane %d offset: 0x%X\n", plane,
							Maps[level].Planes[plane].offset);
							
				} else if (plane >= NUM_PLANES) { // read compressed size
				
					fread(
						&Maps[level].Planes[plane - NUM_PLANES].size,
							1, sizeof(u_int16_t), fpin);
							
						printf("Plane %d size: %u (compressed)\n",
							plane - NUM_PLANES,
							Maps[level].Planes[plane - NUM_PLANES].size);
				}
			}
			
			// read rest of properties
			for(plane = 0; plane <= 3; ++plane)
			{
				switch(plane) {
					case 0:
						fread(&Maps[level].sizeX, 1,
							sizeof(u_int16_t), fpin);
						printf("SizeX: %d\n",
							Maps[level].sizeX);
						break;
					case 1:
						fread(&Maps[level].sizeY, 1,
							sizeof(u_int16_t), fpin);
						printf("SizeY: %d\n",
							Maps[level].sizeY);
						break;
					case 2:
						fread(Maps[level].name, MAPNAME_SIZE,
							sizeof(char), fpin);
						printf("Level Name: %s\n\n",
							Maps[level].name);
						break;
				}
			}
			
			// read the actual data into the gamemaps struct
			int i; for(i = 0; i < NUM_PLANES; ++i) {
				Maps[level].Planes[i].data =
					(char *)calloc(Maps[level].Planes[i].size, 1);
				fread(Maps[level].Planes[i].data, 1, Maps[level].Planes[i].size, fpin);
			}
		}
		else
		{
			printf("Null data...\n\n");
		}
		
		++level;
	}
	
	return 0;
}

/*
	decompression
*/

#define TEMPFILE "TEMP.tmp"

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
	
	// another shorthand for readability
	MapWL6* const Maps = GameMaps->Maps;
	
	// get the planes' decompressed size
	int map, plane;
	for(map = 0; map < GameMaps->MapHead.numLvls; ++map)
	{
		printf("<MAP %d:>\n", map);
		for(plane = 0; plane < NUM_PLANES; ++plane)
		{
			Maps[map].Planes[plane].deSize =
				(Maps[map].Planes[plane].size +
				(Maps[map].Planes[plane].size << 8)) * 2;
			
			printf("Plane %d deSize: %u\n", plane, 
				Maps[map].Planes[plane].deSize);
		}
		printf("\n");
	}
	
	/* 	create the file to which we're going to write the
		decarmacized data	*/
	FILE* fLvl;
	
	// go to where the map data starts 
	fseek(fpin, Maps[0].Planes[0].offset, SEEK_SET);
	
	/*	UNSIGNED CHAR IS IMPORTANT!!!
		https://
		stackoverflow.com/
		questions/
		31090616/
		printf-adds-extra-ffffff-to-hex-print-from-a-char-array	*/
	unsigned char ch;
	
	while(!feof(fpin)) //tentative
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
	
	/*
	if (!remove(TEMPFILE) && fTemp != NULL) {
		printf("Temp file deleted successfully.\n");
	} else {
		printf("Temp file could not be deleted.\n");
	}*/
	
	return 0;
}

int wDeRLEW(FILE* fpin, GameMapsWL6* GameMaps)
{
	FILE* fpout = fopen(TEMPFILE, "w");
	
	fclose(fpout);
	return 0;
}
