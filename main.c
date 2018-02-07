#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include "main.h"

int main(int argc, char* argv[])
{
	int exit_status = EXIT_SUCCESS;
	
	WolfSet ws = { NULL, 0 };
	ws.maps = (WolfMap *)malloc(sizeof(WolfMap));
	
	switch(argc)
	{
		default:
			// puts("Usage: wdmak [OPTION]... [MAPHEAD] [GAMEMAPS]");
			puts("Usage: wdmak [MAPHEAD] [GAMEMAPS]");
			break;
		case 3:
			if (
				(exit_status = wReadMapHead(argv, &ws)) ||
				(exit_status = wReadGameMaps(argv, &ws)) ||
				(exit_status = wDeCarmacize(argv, &ws))
			)
			break;
	}
	
	if (ws.maps)
		{ free(ws.maps); }
	
	return exit_status;
}

#define RLEW 0xABCD

int wReadMapHead(char* const argv[], WolfSet* const ws)
{
	int exit_status = EXIT_SUCCESS;
	
	FILE* fp;
	u_int16_t magic;
	
	puts("Opening and verifying MAPHEAD file... ");
	
	if ( (fp = fopen(argv[1], "rb")) )
	{
		// verify that this is indeed the maphead file by the magic #
		
		fread(&magic, sizeof(u_int16_t), 1, fp);
		if (magic != RLEW) {
			fprintf(stderr, "%s%s",
				"RLEW tag not found. Be sure that argument 1 is a ",
				"MAPHEAD file.\n");
			exit_status = EPERM;
		}
		
		puts("RLEW Tag found! Counting level offsets...");
		
		int i;
		for(i = 0; !feof(fp); ++i)
		{
			// don't realloc to size of 0
			// remember i is the index not the number of elements
			
			if (i > 0) {
				ws->maps = (WolfMap *)realloc(ws->maps,
					(i + 1) * sizeof(WolfMap));
			}
			
			// initialize offset to make valgrind stop complaining
			
			ws->maps[i].offset = 0x0;
			fread(&ws->maps[i].offset, sizeof(u_int32_t), 1, fp);
			
			// increment numLvls if the current offset is 0x0
			// which means (there's no level in this slot)
			
			if (ws->maps[i].offset != 0x0) {
				++ws->numLvls;
			}
		}
		
		printf("%u levels found\n", ws->numLvls);
	}
	else
	{
		fputs("MAPHEAD file corrupted or not found!\n", stderr);
		exit_status = ENOENT;
	}
	
	fclose(fp);
	return exit_status;
}

int wReadGameMaps(char* const argv[], WolfSet* const ws)
{
	int exit_status = EXIT_SUCCESS;
	
	FILE* fp;
	char* tedHead;
	
	puts("Opening and verifying GAMEMAPS file...");
	
	if ( (fp = fopen(argv[2], "rb")) )
	{
		// this is at the top of all GAMEMAPS files
		
		const char TEDHEAD[] = "TED5v1.0";
		
		// TEDHEAD is implicitly null terminated, so taking that into
		// account, allocate sizeof(TEDHEAD) - 1
		
		char* tedHead = (char *)calloc(sizeof(TEDHEAD) - 1, 1);
		fread(tedHead, 1, sizeof(TEDHEAD) - 1, fp);
		
		// now you can make a null-terminator-less comparison!
		
		if (memcmp(TEDHEAD, tedHead, sizeof(TEDHEAD) - 1))
		{
			fputs("TED5v1.0 string not found! Be sure that ", stderr);
			fputs("this is a valid GAMEMAPS file!\n", stderr);
			exit_status = EPERM;
		}
		else
		{
			unsigned int lvl;
			for(lvl = 0; lvl < ws->numLvls; ++lvl)
			{
				fseek(fp, ws->maps[lvl].offset, SEEK_SET);
				
				unsigned int pl;
				
				// read plane offsets
				
				for(pl = 0; pl < NUM_PLANES; ++pl) {
					fread(&ws->maps[lvl].planes[pl].offset,
						sizeof(u_int32_t), 1, fp);
				}
				
				// read plane sizes
				
				for(pl = 0; pl < NUM_PLANES; ++pl) {
					fread(&ws->maps[lvl].planes[pl].size,
						sizeof(u_int16_t), 1, fp);
				}
				
				// read width and height
				
				fread(&ws->maps[lvl].sizeX, sizeof(u_int16_t), 1, fp);
				fread(&ws->maps[lvl].sizeY, sizeof(u_int16_t), 1, fp);
				
				// read name
				
				fread(ws->maps[lvl].name, 1, 16, fp);
			}
		}
		
		if (tedHead)
			{ free(tedHead); }
	}
	else
	{
		fputs("GAMEMAPS file corrupted or not found!\n", stderr);
		exit_status = EPERM;
	}
	
	puts("GAMEMAPS read successfully!");
	
	fclose(fp);
	return exit_status;
}

#define NEARTAG 0xA7
#define FARTAG 0xA8
#define EXCEPTAG 0x0

int wDeCarmacize(char* const argv[], WolfSet* const ws)
{
	int exit_status = EXIT_SUCCESS;
	
	FILE* fp;

	/*	numWords = number of words to copy
		relOff = 	relative offset of the first word to copy
		absOff =	absolute offset of the first word to copy */
	u_int8_t numWords, relOff, absOff;
	
	puts("De-Carmacizing...");
	
	if ( (fp = fopen(argv[2], "rb")) )
	{
		// read decompressed size of each plane
		
		unsigned int lvl;
		for(lvl = 0; lvl < ws->numLvls; ++lvl)
		{
			unsigned int pl;
			for(pl = 0; pl < NUM_PLANES; ++pl)
			{
				// read decompressed size of the plane
				
				fseek(fp, ws->maps[lvl].planes[pl].offset, SEEK_SET);
				fread(&ws->maps[lvl].planes[pl].deSize,
					sizeof(u_int16_t), 1, fp);
				
				// allocate for carmacized data
				
				ws->maps[lvl].planes[pl].data = (
					(unsigned char *)calloc (
						ws->maps[lvl].planes[pl].size, 1
					)
				);
				
				// read carmacized data into allocated space
				
				fread(ws->maps[lvl].planes[pl].data, 1,
					ws->maps[lvl].planes[pl].size, fp);
				
				printf("%s, Plane %d: \n", ws->maps[lvl].name, pl);
				
				int i;
				for(i = 0; i < ws->maps[lvl].planes[pl].size; ++i) {
					printf("0x%X, ", ws->maps[lvl].planes[pl].data[i]);
				} printf("\n");
				
				free(ws->maps[lvl].planes[pl].data);
			}
		}
	}
	else
	{
		fputs("wDeCarmacize(): GAMEMAPS file corrupted or ", stderr);
		fputs("not found!\n", stderr);
		exit_status = EPERM;
	}
	
	fclose(fp);
	return exit_status;
}
