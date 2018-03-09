#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

typedef struct Args {
	char* MAPHEAD_PATH;
	char* GAMEMAPS_PATH;
} Args;

// main methods
// COOL THING TO TRY? would it be worth it to unionize the args so that
// a lot of this repetition can be avoided?

int mReadMapHead (char* const[], WolfSet* const);
int mReadGameMaps(char* const[], WolfSet* const);
int mDeCarmacize (char* const[], WolfSet* const);

#endif // MAIN_H_INCLUDED
