/* ls.h
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#define kLsCacheItemLifetime 900	/* seconds */

typedef struct LsCacheItem {
	char *itempath;
	FileInfoList fil;
	time_t expiration;
	int hits;
} LsCacheItem;

#define kLsCacheSize 32

/* ls.c */
void InitLsCache(void);
void InitLsMonths(void);
void InitLs(void);
void FlushLsCache(void);
int LsCacheLookup(const char *const);
void LsDate(char *, time_t);
void LsL(FileInfoListPtr, int, int, FILE *);
void Ls1(FileInfoListPtr, int, FILE *);
void Ls(const char *const, int, const char *const, FILE *);
void LLs(const char *const, int, const char *const, FILE *);
