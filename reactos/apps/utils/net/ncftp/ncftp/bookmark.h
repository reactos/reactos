/* bookmark.h
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

typedef struct Bookmark *BookmarkPtr;
typedef struct Bookmark {
	char				bookmarkName[16];
	char				name[64];
	char				user[64];
	char				pass[64];
	char				acct[64];
	char				dir[160];
	char				ldir[160];
	int				xferType;
	unsigned int			port;
	time_t				lastCall;
	int				hasSIZE;
	int				hasMDTM;
	int				hasPASV;
	int				isUnix;
	char				lastIP[32];
	char				comment[128];
	int				xferMode;
	int				hasUTIME;

	int				deleted;
} Bookmark;

#define kBookmarkVersion		8
#define kBookmarkMinVersion		3
#if defined(WIN32) || defined(_WINDOWS)
#	define kBookmarkFileName		"bookmarks.txt"
#else
#	define kBookmarkFileName		"bookmarks"
#endif
#define kTmpBookmarkFileName		"bookmarks-tmp"
#define kOldBookmarkFileName		"hosts"
#define kBookmarkBupFileName		"bookmarks.old"

#define BMTINDEX(p) ((int) ((char *) p - (char *) gBookmarkTable) / (int) sizeof(Bookmark))

/* bookmark.c */
void BookmarkToURL(BookmarkPtr, char *, size_t);
void SetBookmarkDefaults(BookmarkPtr);
int ParseHostLine(char *, BookmarkPtr);
void CloseBookmarkFile(FILE *);
FILE *OpenBookmarkFile(int *);
FILE *OpenTmpBookmarkFile(int);
int SaveBookmarkTable(void);
int GetNextBookmark(FILE *, Bookmark *);
int GetBookmark(const char *const, Bookmark *);
int PutBookmark(Bookmark *, int);
int LoadBookmarkTable(void);
BookmarkPtr SearchBookmarkTable(const char *);
void SortBookmarks(void);
void DefaultBookmarkName(char *, size_t, char *);
