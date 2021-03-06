#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "carmackexpand.h"
#include "wolftypes.h"
#include "main.h"

int main(int argc, char* argv[])
{
	int exitStatus = EXIT_SUCCESS;
	
	WolfSet ws = { NULL, 0 };
	
	if (( ws.maps = (WolfMap *)malloc(sizeof(WolfMap)) ))
	{
		switch(argc)
		{
			default:
				printf(
					"Usage: %s [MAPHEAD] [GAMEMAPS] "
					"[MAP<-MAP>],...\n", argv[0]
				);
				break;
			case 3:
				if (
					(exitStatus = mReadMapHead(argv, &ws)) ||
					(exitStatus = mReadGameMaps(argv, &ws)) ||
					(exitStatus = mDeCarmacize(argv, &ws))
				)
				break;
		}
		
		// don't wanna free decompressed data until program end
		
		if (ws.maps)
		{
			unsigned int lvl;
			for(lvl = 0; lvl < ws.numLvls; ++lvl) {
				unsigned int pl;
				for(pl = 0; pl < NUM_PLANES; ++pl) {
					if (ws.maps[lvl].planes[pl].deData)
						free(ws.maps[lvl].planes[pl].deData);
				}
			}
			free(ws.maps);
		}
	}
	else
	{
		fputs("Could not do initial alloc.", stderr);
		exitStatus = EXIT_FAILURE;
	}
	
	return exitStatus;
}

#define RLEW 0xABCD

int mReadMapHead(char* const argv[], WolfSet* const wsPtr)
{
	int exitStatus = EXIT_SUCCESS;
	
	FILE* fp;
	u_int16_t magic;
	
	puts("Opening and verifying MAPHEAD file... ");
	
	if ( (fp = fopen(argv[1], "rb")) )
	{
		// verify that this is indeed the maphead file by the magic #
		
		fread(&magic, sizeof(u_int16_t), 1, fp);
		if (magic != RLEW)
		{
			fputs (
				"RLEW tag not found. Be sure that argument 1 is a "
				"MAPHEAD file.\n",
				stderr
			);
			
			exitStatus = EPERM;
		}
		else
		{
			puts("RLEW Tag found! Counting level offsets...");
			
			int i;
			for(i = 0; !feof(fp); ++i)
			{
				// don't realloc to size of 0
				// remember i is the index not the number of elements
				
				if (i > 0) {
					if (!( wsPtr->maps = (WolfMap *)realloc(wsPtr->maps,
						(i + 1) * sizeof(WolfMap)) )) {
						fputs("Could not resize maps array", stderr);
						return EPERM;
					}
				}
				
				// initialize offset to make valgrind stop complaining
				
				wsPtr->maps[i].offset = 0x0;
				fread(&wsPtr->maps[i].offset, sizeof(u_int32_t), 1, fp);
				
				// increment numLvls if the current offset is 0x0
				// which means (there's no level in this slot)
				
				if (wsPtr->maps[i].offset != 0x0) {
					++wsPtr->numLvls;
				}
			}
			
			printf("%u levels found\n", wsPtr->numLvls);
		}
	}
	else
	{
		fputs("MAPHEAD file corrupted or not found!\n", stderr);
		exitStatus = ENOENT;
	}
	
	fclose(fp);
	return exitStatus;
}

int mReadGameMaps(char* const argv[], WolfSet* const wsPtr)
{
	int exitStatus = EXIT_SUCCESS;
	
	FILE* fp;
	
	puts("Opening and verifying GAMEMAPS file...");
	
	if ( (fp = fopen(argv[2], "rb")) )
	{
		// this is at the top of all GAMEMAPS files
		
		const char TEDHEAD[] = "TED5v1.0";
		
		// TEDHEAD is implicitly null terminated, so taking that into
		// account, allocate sizeof(TEDHEAD) - 1
		
		char* tedHead = (char *)calloc(sizeof(TEDHEAD) - 1, 1);
		fread(tedHead, 1, sizeof(TEDHEAD) - 1, fp);
		
		// ditto except for !ID! byte signature
		
		const char IDSIG[] = "!ID!";
		char* idSig = (char *)calloc(sizeof(IDSIG) - 1, 1);
		
		// now you can make a null-terminator-less comparison!
		
		if (memcmp(TEDHEAD, tedHead, sizeof(TEDHEAD) - 1))
		{
			fputs (
				"TED5v1.0 string not found! Be sure that this is a "
				"valid GAMEMAPS file!\n",
				stderr
			);
			
			exitStatus = EPERM;
		}
		else
		{				
			unsigned int lvl;
			for(lvl = 0; lvl < wsPtr->numLvls; ++lvl)
			{
				fseek(fp, wsPtr->maps[lvl].offset, SEEK_SET);
				
				// read plane offsets
				
				unsigned int pl = 0;
				for(pl = 0; pl < NUM_PLANES; ++pl) {
					fread(&wsPtr->maps[lvl].planes[pl].offset,
						sizeof(u_int32_t), 1, fp);
				}
				
				// read plane sizes
				
				// see pl in for loop above
				for(pl = 0; pl < NUM_PLANES; ++pl) {
					fread(&wsPtr->maps[lvl].planes[pl].size,
						sizeof(u_int16_t), 1, fp);
				}
				
				// read width and height
				
				fread(&wsPtr->maps[lvl].sizeX, sizeof(u_int16_t),
					1, fp);
				fread(&wsPtr->maps[lvl].sizeY, sizeof(u_int16_t),
					1, fp);
				
				// read name
				
				fread(wsPtr->maps[lvl].name, 1, NUM_MAPCHARS, fp);
				
				// read !ID! byte signature
				
				fread(idSig, 1, sizeof(IDSIG) - 1, fp);
				
				if (memcmp(IDSIG, idSig, sizeof(IDSIG) - 1))
				{					
					fputs (
						"!ID! signature not found at corresponding "
						"offset. Make sure your map files belong to a "
						"Wolfenstein 3d distribution whose version is "
						"greater than v1.0 OR that your MAPHEAD file "
						"corresponds to your GAMEMAPS file.\n",
						stderr
					);
					
					fprintf (
						stderr,
						"OFFENDING OFFSET: 0x%X\n",
						wsPtr->maps[lvl].planes[pl].offset
					);
					
					fprintf (stderr, "OFFENDING MAP: %u\n", lvl);
					
					exitStatus = EPERM;
					break; // sorry not sorry
				}
			}
		}
		
		if(idSig)
			{ free(idSig); }
		if (tedHead)
			{ free(tedHead); }
	}
	else
	{
		fputs("GAMEMAPS file corrupted or not found!\n", stderr);
		exitStatus = EPERM;
	}
	
	if (fp)
		{ fclose(fp); }
	return exitStatus;
}

int mDeCarmacize(char* const argv[], WolfSet* const wsPtr)
{
	int exitStatus = EXIT_SUCCESS;
	
	FILE* fp;
	
	puts("De-Carmacizing...");
	
	if ( (fp = fopen(argv[2], "rb")) )
	{
		// read decompressed size of each plane
		
		unsigned int lvl;
		for(lvl = 0; lvl < wsPtr->numLvls; ++lvl)
		{
			unsigned int pl;
			for(pl = 0; pl < NUM_PLANES; ++pl)
			{
				// read decompressed size of the plane
				
				fseek(fp, wsPtr->maps[lvl].planes[pl].offset, SEEK_SET);
				fread(&wsPtr->maps[lvl].planes[pl].deSize,
					sizeof(u_int16_t), 1, fp);
				
				// allocate for carmacized data
				
				wsPtr->maps[lvl].planes[pl].data = (
					(u_int8_t *)calloc (
						wsPtr->maps[lvl].planes[pl].size, 1
					)
				);
				
				// read carmacized data into allocated space
				
				fread(wsPtr->maps[lvl].planes[pl].data, 1,
					wsPtr->maps[lvl].planes[pl].size, fp);
				
				wsPtr->maps[lvl].planes[pl].deData
					= carmackExpand (
						wsPtr->maps[lvl].planes[pl].data,
						// compressed size
						wsPtr->maps[lvl].planes[pl].size,
						// decompressed size
						wsPtr->maps[lvl].planes[pl].deSize
					);
				
				free(wsPtr->maps[lvl].planes[pl].data);
			}
		}
	}
	else
	{
		fputs("wDeCarmacize(): GAMEMAPS file corrupted or not found!\n",
			stderr);
		
		exitStatus = EPERM;
	}
	
	fclose(fp);
	return exitStatus;
}
