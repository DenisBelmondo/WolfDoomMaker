#include "main.h"

int main(int argc, char* argv[])
{
	int returnVal = 0; // tentative
	
	WolfSet ws;
	
	ws.numLvls = 0;
	ws.lvlOffs = (u_int32_t *)malloc(sizeof(u_int32_t));
	
	ws.maps = (WolfMap *)malloc(sizeof(WolfMap));
	
	switch(argc)
	{
		default:
			puts("Usage: wdmak [OPTION]... [MAPHEAD] [GAMEMAPS]");
			break;
		case 3:
			if (
				wReadMapHead(argv, &ws) ||
				wReadGameMaps(argv, &ws) ||
				wDeCarmacize(argv, &ws)
			) { returnVal = 1; }
			break;
	}
	
	if (ws.lvlOffs)
		{ free(ws.lvlOffs); }
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
		
		// loop through the maphead file and record all the offsets
		
		int i; for(i = 0; !feof(fp); ++i)
		{
			// i + 1 because we don't wanna realloc to block of size 0
			ws->lvlOffs = (u_int32_t *)realloc(ws->lvlOffs,
				(i + 1) * sizeof(u_int32_t));
			
			fread(&ws->lvlOffs[i], sizeof(u_int32_t), 1, fp);
			
			// an offset value of 0x0 means there is no level, so don't
			// increment numLvls in that case
			if (ws->lvlOffs[i] != 0x0) { ++ws->numLvls; }
			
			// printf("Offset %d: 0x%X\n", i, ws->lvlOffs[i]);
		}
		
		printf("%d offsets, %d levels\n", i, ws->numLvls);
	}
	else
	{
		fprintf(stderr, "MAPHEAD file corrupted or not found!\n");
		return 1;
	}
	
	// the ternary statement checks numLvls if it's 0 for extra safety
	ws->maps = (WolfMap *)calloc (
		ws->numLvls ? 1 : ws->numLvls, sizeof(WolfMap) );
	
	fclose(fp);
	return 0;
}

int wReadGameMaps(char* const argv[], WolfSet* const ws)
{
	FILE* fp;
	
	puts("Opening and verifying GAMEMAPS file...");
	
	if ( (fp = fopen(argv[2], "rb")) )
	{
		puts("GAMEMAPS file found, reading...");
		
		const char TEDHEAD[] = {'T','E','D','5','v','1','.','0'};
		char tedHead[sizeof(TEDHEAD)];
		
		fread(tedHead, 1, sizeof(TEDHEAD), fp);
		int c; for(c = 0; c < sizeof(TEDHEAD); ++c) {
			if (tedHead[c] != TEDHEAD[c]) {
				fprintf(stderr, "%s%s",
					"TED5v1.0 string not found. Make sure that you ",
					"supplied an actual GAMEMAPS file.\n");
				return 1;
			}
		}
		
		int lvl; for(lvl = 0; lvl < ws->numLvls; ++lvl)
		{
			if (!fseek(fp, ws->lvlOffs[lvl], SEEK_SET))
			{
				int pl;
				
				//printf("<MAP %d at 0x%X:>\n", lvl, ws->lvlOffs[lvl]);
				
				// get the plane offsets...
				for(pl = 0; pl < NUM_PLANES; ++pl) {
					fread(&ws->maps[lvl].planes[pl].offset,
						sizeof(u_int32_t), 1, fp);
					// printf("Plane %d offset: 0x%X\n", pl, ws->maps[lvl].planes[pl].offset);
				}
				
				// then, get the plane sizes
				for(pl = 0; pl < NUM_PLANES; ++pl) {
					fread(&ws->maps[lvl].planes[pl].size,
						sizeof(u_int16_t), 1, fp);
					// printf("Plane %d size: %u\n", pl, ws->maps[lvl].planes[pl].size);
				}
				
				// get the length and width
				fread(&ws->maps[lvl].sizeX, sizeof(u_int16_t), 1, fp);
				fread(&ws->maps[lvl].sizeY, sizeof(u_int16_t), 1, fp);
				
				// a plane's decompressed size is sizeX * sizeY * 2
				for(pl = 0; pl < NUM_PLANES; ++pl) {
					ws->maps[lvl].planes[pl].deSize =
						ws->maps[lvl].sizeX * ws->maps[lvl].sizeY * 2;
						//printf("Plane %d deSize: %u\n", pl,
							//ws->maps[lvl].planes[pl].deSize);
				}
				
				// get the map name
				fread(&ws->maps[lvl].name, 1, NUM_MAPCHARS, fp);
			}
			else
			{
				fprintf(stderr, "%s%d%s%s%s",
					"Level offset discrepancy found at ", lvl, ". ",
					"Be sure that the MAPHEAD file you provided ",
					"correlates to the GAMEMAPS file.\n");
				return 1;
			}
		}
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
	FILE* fp, *fpTemp;

	/*	numWords = number of words to copy
		relOff = 	relative offset of the first word to copy
		absOff =	absolute offset of the first word to copy */
	u_int8_t numWords, relOff, absOff;
	
	unsigned char* carBuf, rlewBuf;
	
	if ( (fp = fopen(argv[2], "rb")) )
	{
		carBuf = (unsigned char *)malloc(ws->maps[0].planes[0].size);
		fseek(fp, ws->maps[0].planes[0].offset, SEEK_SET);
		
		// write the carmacized data into carBuf
		unsigned int i;
		for(i = 0; i < ws->maps[0].planes[0].size; ++i)
		{
			fread(&carBuf[i], 1, 1, fp);
			//printf("0x%X, ", carBuf[i]);
		} //fputc('\n', stdout);
		
		// create the tmpfile
		if ((fpTemp = tmpfile()) == NULL) {
			fprintf(stderr, "Could not create temporary file.");
			return 1;
		}
		
		// process carmacized data
		puts("De-Carmacizing...");
		int j; for(i = 0; i < ws->maps[0].planes[0].size; ++i)
		{
			switch(carBuf[i])
			{
				case NEARTAG:
					printf("repeat the %u words starting %u words ago\n",
						(unsigned int) carBuf[i - 1], (unsigned int) carBuf[i + 1]);
					break;
				case FARTAG:
					break;
				default:
					break;
			}
		}
		
		if (fpTemp) { fclose(fpTemp); }
		if (carBuf) { free(carBuf); }
	}
	else
	{
		fprintf(stderr, "wDeCarmacize(): GAMEMAPS file corrupted or not found!\n");
		return 1;
	}
	
	fclose(fp);
	return 0;
}
