#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

int main(int argc, char* argv[])
{
	int returnVal = 0; // tentative
	
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
				wReadMapHead(argv, &ws) ||
				wReadGameMaps(argv, &ws) /*||
				wDeCarmacize(argv, &ws)*/
			) { returnVal = 1; }
			break;
	}
	
	if (ws.maps)
		{ free(ws.maps); }
	
	return returnVal;
}

#define RLEW 0xABCD

int wReadMapHead(char* const argv[], WolfSet* const ws)
{
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
			return 1;
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
		fprintf(stderr, "MAPHEAD file corrupted or not found!\n");
		return 1;
	}
	
	fclose(fp);
	return 0;
}

int wReadGameMaps(char* const argv[], WolfSet* const ws)
{
	FILE* fp;
	
	puts("Opening and verifying GAMEMAPS file...");
	
	if ( (fp = fopen(argv[2], "rb")) )
	{
		// this is at the top of all GAMEMAPS files
		
		const char TEDHEAD[] = "TED5v1.0";
		
		// TEDHEAD is implicitly null terminated, so taking that into
		// account, allocate sizeof(TEDHEAD) - 1
		
		char *tedHead = (char *)calloc(sizeof(TEDHEAD) - 1, 1);
		fread(tedHead, 1, sizeof(TEDHEAD) - 1, fp);
		
		// now you can make a null-terminator-less comparison!
		
		if (memcmp(TEDHEAD, tedHead, sizeof(TEDHEAD) - 1)) {
			fputs("TED5v1.0 string not found! Be sure that ", stdout);
			fputs("this is a valid GAMEMAPS file!\n", stdout);
			return 1;
		}
		
		free(tedHead);
	}
	else
	{
		fprintf(stderr, "GAMEMAPS file corrupted or not found!\n");
		return 1;
	}
	
	puts("GAMEMAPS read successfully!");
	
	fclose(fp);
	return 0;
}

#define NEARTAG 0xA7
#define FARTAG 0xA8
#define EXCEPTAG 0x0

int wDeCarmacize(char* const argv[], WolfSet* const ws)
{	
	FILE *fp;

	/*	numWords = number of words to copy
		relOff = 	relative offset of the first word to copy
		absOff =	absolute offset of the first word to copy */
	u_int8_t numWords, relOff, absOff;
	
	puts("De-Carmacizing...");
	
	if ( (fp = fopen(argv[2], "rb")) )
	{
		
	}
	else
	{
		fprintf(stderr, "wDeCarmacize(): GAMEMAPS file corrupted or not found!\n");
		return 1;
	}
	
	fclose(fp);
	return 0;
}
