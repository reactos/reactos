/* bookmark.c 
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#include "syshdrs.h"

#include "bookmark.h"
#include "util.h"

/*
 * The ~/.ncftp/bookmarks file contains a list of sites
 * the user wants to remember.
 *
 * Unlike previous versions of the program, we now open/close
 * the file every time we need it;  That way we can have
 * multiple ncftp processes changing the file.  There is still
 * a possibility that two different processes could be modifying
 * the file at the same time.
 */

Bookmark gBm;
int gLoadedBm = 0;
int gBookmarkMatchMode = 0;
int gNumBookmarks = 0;
BookmarkPtr gBookmarkTable = NULL;

extern char gOurDirectoryPath[];

/* Converts a pre-loaded Bookmark structure into a RFC 1738
 * Uniform Resource Locator.
 */
void
BookmarkToURL(BookmarkPtr bmp, char *url, size_t urlsize)
{
	char pbuf[32];

	/* //<user>:<password>@<host>:<port>/<url-path> */
	/* Note that if an absolute path is given,
	 * you need to escape the first entry, i.e. /pub -> %2Fpub
	 */
	(void) Strncpy(url, "ftp://", urlsize);
	if (bmp->user[0] != '\0') {
		(void) Strncat(url, bmp->user, urlsize);
		if (bmp->pass[0] != '\0') {
			(void) Strncat(url, ":", urlsize);
			(void) Strncat(url, "PASSWORD", urlsize);
		}
		(void) Strncat(url, "@", urlsize);
	}
	(void) Strncat(url, bmp->name, urlsize);
	if (bmp->port != 21) {
		(void) sprintf(pbuf, ":%u", (unsigned int) bmp->port);
		(void) Strncat(url, pbuf, urlsize);
	}
	if (bmp->dir[0] == '/') {
		/* Absolute URL path, must escape first slash. */
		(void) Strncat(url, "/%2F", urlsize);
		(void) Strncat(url, bmp->dir + 1, urlsize);
		(void) Strncat(url, "/", urlsize);
	} else if (bmp->dir[0] != '\0') {
		(void) Strncat(url, "/", urlsize);
		(void) Strncat(url, bmp->dir, urlsize);
		(void) Strncat(url, "/", urlsize);
	}
}	/* BookmarkToURL */




void
SetBookmarkDefaults(BookmarkPtr bmp)
{
	(void) memset(bmp, 0, sizeof(Bookmark));

	bmp->xferType = 'I';
	bmp->xferMode = 'S';	/* Use FTP protocol default as ours too. */
	bmp->hasSIZE = kCommandAvailabilityUnknown;
	bmp->hasMDTM = kCommandAvailabilityUnknown;
	bmp->hasUTIME = kCommandAvailabilityUnknown;
	bmp->hasPASV = kCommandAvailabilityUnknown;
	bmp->isUnix = 1;
	bmp->lastCall = (time_t) 0;
	bmp->deleted = 0;
}	/* SetBookmarkDefaults */




/* Used when converting hex strings to integral types. */
static int
HexCharToNibble(int c)
{
	switch (c) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			return (c - '0');
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			return (c - 'a' + 10);
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			return (c - 'A' + 10);

	}
	return (-1);	/* Error. */
}	/* HexCharToNibble */





/* Fills in a Bookmark structure based off of a line from the NcFTP
 * "bookmarks" file.
 */
int
ParseHostLine(char *line, BookmarkPtr bmp)
{
	char token[128];
	char pass[128];
	char *s, *d;
	char *tokenend;
	long L;
	int i;
	int result;
	int n, n1, n2;

	SetBookmarkDefaults(bmp);
	s = line;
	tokenend = token + sizeof(token) - 1;
	result = -1;
	for (i=1; ; i++) {
		if (*s == '\0')
			break;
		/* Some tokens may need to have a comma in them.  Since this is a
		 * field delimiter, these fields use \, to represent a comma, and
		 * \\ for a backslash.  This chunk gets the next token, paying
		 * attention to the escaped stuff.
		 */
		for (d = token; *s != '\0'; ) {
			if ((*s == '\\') && (s[1] != '\0')) {
				if (d < tokenend)
					*d++ = s[1];
				s += 2;
			} else if (*s == ',') {
				++s;
				break;
			} else if ((*s == '$') && (s[1] != '\0') && (s[2] != '\0')) {
				n1 = HexCharToNibble(s[1]);
				n2 = HexCharToNibble(s[2]);
				if ((n1 >= 0) && (n2 >= 0)) {
					n = (n1 << 4) | n2;
					if (d < tokenend)
						*(unsigned char *)d++ = (unsigned int) n;
				}
				s += 3;
			} else {
				if (d < tokenend)
					*d++ = *s;
				++s;
			}
		}
		*d = '\0';
		switch(i) {
			case 1: (void) STRNCPY(bmp->bookmarkName, token); break;
			case 2: (void) STRNCPY(bmp->name, token); break;
			case 3: (void) STRNCPY(bmp->user, token); break;
			case 4: (void) STRNCPY(bmp->pass, token); break;
			case 5: (void) STRNCPY(bmp->acct, token); break;
			case 6: (void) STRNCPY(bmp->dir, token);
					result = 0;		/* Good enough to have these fields. */
					break;
			case 7:
				if (token[0] != '\0')
					bmp->xferType = (int) token[0];
				break;
			case 8:
				/* Most of the time, we won't have a port. */
				if (token[0] == '\0')
					bmp->port = (unsigned int) kDefaultFTPPort;
				else
					bmp->port = (unsigned int) atoi(token);
				break;
			case 9:
				(void) sscanf(token, "%lx", &L);
				bmp->lastCall = (time_t) L;
				break;
			case 10: bmp->hasSIZE = atoi(token); break;
			case 11: bmp->hasMDTM = atoi(token); break;
			case 12: bmp->hasPASV = atoi(token); break;
			case 13: bmp->isUnix = atoi(token);
					result = 3;		/* Version 3 had all fields to here. */
					break;
			case 14: (void) STRNCPY(bmp->lastIP, token); break;
			case 15: (void) STRNCPY(bmp->comment, token); break;
			case 16:
			case 17:
			case 18:
			case 19:
				break;
			case 20: bmp->xferMode = token[0];
					result = 7;		/* Version 7 has all fields to here. */
					break;
			case 21: bmp->hasUTIME = atoi(token);
					break;
			case 22: (void) STRNCPY(bmp->ldir, token);
					result = 8;		/* Version 8 has all fields to here. */
					break;
			default:
					result = 99;	/* Version >8 ? */
					goto done;
		}
	}
done:

	/* Decode password, if it was base-64 encoded. */
	if (strncmp(bmp->pass, kPasswordMagic, kPasswordMagicLen) == 0) {
		FromBase64(pass, bmp->pass + kPasswordMagicLen, strlen(bmp->pass + kPasswordMagicLen), 1);
		(void) STRNCPY(bmp->pass, pass);
	}
	return (result);
}	/* ParseHostLine */




void
CloseBookmarkFile(FILE *fp)
{
	if (fp != NULL)
		(void) fclose(fp);
}	/* CloseBookmarkFile */





int
GetNextBookmark(FILE *fp, Bookmark *bmp)
{
	char line[512];

	while (FGets(line, sizeof(line), fp) != NULL) {
		if (ParseHostLine(line, bmp) >= 0)
			return (0);
	}
	return (-1);
}	/* GetNextBookmark */




/* Opens a NcFTP 2.x or 3.x style bookmarks file, and sets the file pointer
 * so that it is ready to read the first data line.
 */
FILE *
OpenBookmarkFile(int *numBookmarks0)
{
	char pathName[256], path2[256];
	char line[256];
	FILE *fp;
	int version;
	int numBookmarks;
	Bookmark junkbm;

	if (gOurDirectoryPath[0] == '\0')
		return NULL;		/* Don't create in root directory. */
	(void) OurDirectoryPath(pathName, sizeof(pathName), kBookmarkFileName);
	fp = fopen(pathName, FOPEN_READ_TEXT);
	if (fp == NULL) {
		/* See if it exists under the old name. */
		(void) OurDirectoryPath(path2, sizeof(path2), kOldBookmarkFileName);
		if (rename(path2, pathName) == 0) {
			/* Rename succeeded, now open it. */
			fp = fopen(pathName, FOPEN_READ_TEXT);
			if (fp == NULL)
				return NULL;
		}
		return NULL;		/* Okay to not have one yet. */
	}
	
	(void) chmod(pathName, 00600);
	if (FGets(line, sizeof(line), fp) == NULL) {
		(void) fprintf(stderr, "%s: invalid format.\n", pathName);
		(void) fclose(fp);
		return NULL;
	}
	
	/* Sample line we're looking for:
	 * "NcFTP bookmark-file version: 8"
	 */
	version = -1;
	(void) sscanf(line, "%*s %*s %*s %d", &version);
	if (version < kBookmarkMinVersion) {
		if (version < 0) {
			(void) fprintf(stderr, "%s: invalid format, or bad version.\n", pathName);
			(void) fclose(fp);
			return NULL;
		}
		(void) STRNCPY(path2, pathName);
		(void) sprintf(line, ".v%d", version);
		(void) STRNCAT(path2, line);
		(void) rename(pathName, path2);
		(void) fprintf(stderr, "%s: old version.\n", pathName);
		(void) fclose(fp);
		return NULL;
	}

	/* Sample line we're looking for:
	 * "Number of entries: 28" or "# # # 1"
	 */
	numBookmarks = -1;
	
	/* At the moment, we can't trust the number stored in the
	 * file.  It's there for future use.
	 */
	if (FGets(line, sizeof(line), fp) == NULL) {
		(void) fprintf(stderr, "%s: invalid format.\n", pathName);
		(void) fclose(fp);
		return NULL;
	}

	if (numBookmarks0 == (int *) 0) {
		/* If the caller doesn't care how many bookmarks are *really*
		 * in the file, then we can return now.
		 */
		return(fp);
	}

	/* Otherwise, we have to read through the whole file because
	 * unfortunately the header line can't be trusted.
	 */
	for (numBookmarks = 0; ; numBookmarks++) {
		if (GetNextBookmark(fp, &junkbm) < 0)
			break;
	}

	/* Now we have to re-open and re-position the file.
	 * We don't use rewind() because it doesn't always work.
	 * This introduces a race condition, but the bookmark
	 * functionality wasn't designed to be air-tight.
	 */
	CloseBookmarkFile(fp);
	fp = fopen(pathName, FOPEN_READ_TEXT);
	if (fp == NULL)
		return (NULL);
	if (FGets(line, sizeof(line), fp) == NULL) {
		(void) fprintf(stderr, "%s: invalid format.\n", pathName);
		(void) fclose(fp);
		return NULL;
	}

	if (FGets(line, sizeof(line), fp) == NULL) {
		(void) fprintf(stderr, "%s: invalid format.\n", pathName);
		(void) fclose(fp);
		return NULL;
	}

	/* NOW we're done. */
	*numBookmarks0 = numBookmarks;
	return (fp);
}	/* OpenBookmarkFile */




/* Looks for a saved bookmark by the abbreviation given. */
int
GetBookmark(const char *const bmabbr, Bookmark *bmp)
{
	FILE *fp;
	char line[512];
	Bookmark byHostName;
	Bookmark byHostAbbr;
	Bookmark byBmAbbr;
	size_t byBmNameFlag = 0;
	size_t byBmAbbrFlag = 0;
	size_t byHostNameFlag = 0;
	size_t byHostAbbrFlag = 0;
	int result = -1;
	int exactMatch = 0;
	size_t bmabbrLen;
	char *cp;

	fp = OpenBookmarkFile(NULL);
	if (fp == NULL)
		return (-1);

	bmabbrLen = strlen(bmabbr);
	while (FGets(line, sizeof(line), fp) != NULL) {
		if (ParseHostLine(line, bmp) < 0)
			continue;
		if (ISTREQ(bmp->bookmarkName, bmabbr)) {
			/* Exact match, done. */
			byBmNameFlag = bmabbrLen;
			exactMatch = 1;
			break;
		} else if (ISTRNEQ(bmp->bookmarkName, bmabbr, bmabbrLen)) {
			/* Remember this one, it matched an abbreviated
			 * bookmark name.
			 */
			byBmAbbr = *bmp;
			byBmAbbrFlag = bmabbrLen;
		} else if (ISTREQ(bmp->name, bmabbr)) {
			/* Remember this one, it matched a full
			 * host name.
			 */
			byHostName = *bmp;
			byHostNameFlag = bmabbrLen;
		} else if ((cp = strchr(bmp->name, '.')) != NULL) {
			/* See if it matched part of the hostname. */
			if (ISTRNEQ(bmp->name, "ftp", 3)) {
				cp = cp + 1;
			} else if (ISTRNEQ(bmp->name, "www", 3)) {
				cp = cp + 1;
			} else {
				cp = bmp->name;
			}
			if (ISTRNEQ(cp, bmabbr, bmabbrLen)) {
				/* Remember this one, it matched a full
				 * host name.
				 */
				byHostAbbr = *bmp;
				byHostAbbrFlag = bmabbrLen;
			}
		}
	}

	if (gBookmarkMatchMode == 0) {
		/* Only use a bookmark when the exact
		 * bookmark name was used.
		 */
		if (exactMatch != 0) {
			result = 0;
		}
	} else {
		/* Pick the best match, if any. */
		if (byBmNameFlag != 0) {
			/* *bmp is already set. */
			result = 0;
		} else if (byBmAbbrFlag != 0) {
			result = 0;
			*bmp = byBmAbbr;
		} else if (byHostNameFlag != 0) {
			result = 0;
			*bmp = byHostName;
		} else if (byHostAbbrFlag != 0) {
			result = 0;
			*bmp = byHostAbbr;
		}
	}

	if (result != 0)
		memset(bmp, 0, sizeof(Bookmark));

	CloseBookmarkFile(fp);
	return (result);
}	/* GetBookmark */




static int
BookmarkSortProc(const void *a, const void *b)
{
	return (ISTRCMP((*(Bookmark *)a).bookmarkName, (*(Bookmark *)b).bookmarkName));	
}	/* BookmarkSortProc */



static int
BookmarkSearchProc(const void *key, const void *b)
{
	return (ISTRCMP((char *) key, (*(Bookmark *)b).bookmarkName));	
}	/* BookmarkSearchProc */



BookmarkPtr
SearchBookmarkTable(const char *key)
{
	return ((BookmarkPtr) bsearch(key, gBookmarkTable, (size_t) gNumBookmarks, sizeof(Bookmark), BookmarkSearchProc));
}	/* SearchBookmarkTable */




void
SortBookmarks(void)
{
	if ((gBookmarkTable == NULL) || (gNumBookmarks < 2))
		return;

	/* Sorting involves swapping entire Bookmark structures.
	 * Normally the proper thing to do is to use an array
	 * of pointers to Bookmarks and sort them, but even
	 * these days a large bookmark list can be sorted in
	 * the blink of an eye.
	 */
	qsort(gBookmarkTable, (size_t) gNumBookmarks, sizeof(Bookmark), BookmarkSortProc);
}	/* SortBookmarks */



int
LoadBookmarkTable(void)
{
	int i, nb;
	FILE *infp;

	infp = OpenBookmarkFile(&nb);
	if (infp == NULL) {
		nb = 0;
	}
	if ((nb != gNumBookmarks) && (gBookmarkTable != NULL)) {
		/* Re-loading the table from disk. */
		gBookmarkTable = (Bookmark *) realloc(gBookmarkTable, (size_t) (nb + 1) * sizeof(Bookmark));
		memset(gBookmarkTable, 0, (nb + 1) * sizeof(Bookmark));
	} else {
		gBookmarkTable = calloc((size_t) (nb + 1), (size_t) sizeof(Bookmark));
	}

	if (gBookmarkTable == NULL) {
		CloseBookmarkFile(infp);
		return (-1);
	}

	for (i=0; i<nb; i++) {
		if (GetNextBookmark(infp, gBookmarkTable + i) < 0) {
			break;
		}
	}
	gNumBookmarks = i;

	CloseBookmarkFile(infp);
	SortBookmarks();
	return (0);
}	/* LoadBookmarkTable */




/* Some characters need to be escaped so the file is editable and can
 * be parsed correctly the next time it is read.
 */
static char *
BmEscapeTok(char *dst, size_t dsize, char *src)
{
	char *dlim = dst + dsize - 1;
	char *dst0 = dst;
	int c;

	while ((c = *src) != '\0') {
		src++;
		if ((c == '\\') || (c == ',') || (c == '$')) {
			/* These need to be escaped. */
			if ((dst + 1) < dlim) {
				*dst++ = '\\';
				*dst++ = c;
			}
		} else if (!isprint(c)) {
			/* Escape non-printing characters. */
			if ((dst + 2) < dlim) {
				(void) sprintf(dst, "$%02x", c);
				dst += 3;
			}
		} else {
			if (dst < dlim)
				*dst++ = c;
		}
	}
	*dst = '\0';
	return (dst0);
}	/* BmEscapeTok */




/* Converts a Bookmark into a text string, and writes it to the saved
 * bookmarks file.
 */
static int
WriteBmLine(Bookmark *bmp, FILE *outfp, int savePassword)
{
	char tok[256];
	char pass[160];

	if (fprintf(outfp, "%s", bmp->bookmarkName) < 0) return (-1) ;/*1*/
	if (fprintf(outfp, ",%s", BmEscapeTok(tok, sizeof(tok), bmp->name)) < 0) return (-1) ;/*2*/
	if (fprintf(outfp, ",%s", BmEscapeTok(tok, sizeof(tok), bmp->user)) < 0) return (-1) ;/*3*/
	if ((bmp->pass[0] != '\0') && (savePassword == 1)) {
		(void) memcpy(pass, kPasswordMagic, kPasswordMagicLen);
		ToBase64(pass + kPasswordMagicLen, bmp->pass, strlen(bmp->pass), 1);
		if (fprintf(outfp, ",%s", pass) < 0) return (-1) ;/*4*/
	} else {
		if (fprintf(outfp, ",%s", "") < 0) return (-1) ;/*4*/
	}
	if (fprintf(outfp, ",%s", BmEscapeTok(tok, sizeof(tok), bmp->acct)) < 0) return (-1) ;/*5*/
	if (fprintf(outfp, ",%s", BmEscapeTok(tok, sizeof(tok), bmp->dir)) < 0) return (-1) ;/*6*/
	if (fprintf(outfp, ",%c", bmp->xferType) < 0) return (-1) ;/*7*/
	if (fprintf(outfp, ",%u", (unsigned int) bmp->port) < 0) return (-1) ;/*8*/
	if (fprintf(outfp, ",%lu", (unsigned long) bmp->lastCall) < 0) return (-1) ;/*9*/
	if (fprintf(outfp, ",%d", bmp->hasSIZE) < 0) return (-1) ;/*10*/
	if (fprintf(outfp, ",%d", bmp->hasMDTM) < 0) return (-1) ;/*11*/
	if (fprintf(outfp, ",%d", bmp->hasPASV) < 0) return (-1) ;/*12*/
	if (fprintf(outfp, ",%d", bmp->isUnix) < 0) return (-1) ;/*13*/
	if (fprintf(outfp, ",%s", bmp->lastIP) < 0) return (-1) ;/*14*/
	if (fprintf(outfp, ",%s", BmEscapeTok(tok, sizeof(tok), bmp->comment)) < 0) return (-1) ;/*15*/
	if (fprintf(outfp, ",%s", "") < 0) return (-1) ;/*16*/
	if (fprintf(outfp, ",%s", "") < 0) return (-1) ;/*17*/
	if (fprintf(outfp, ",%s", "") < 0) return (-1) ;/*18*/
	if (fprintf(outfp, ",%s", "") < 0) return (-1) ;/*19*/
	if (fprintf(outfp, ",%c", bmp->xferMode) < 0) return (-1) ;/*20*/
	if (fprintf(outfp, ",%d", bmp->hasUTIME) < 0) return (-1) ;/*21*/
	if (fprintf(outfp, ",%s", BmEscapeTok(tok, sizeof(tok), bmp->ldir)) < 0) return (-1) ;/*22*/
	if (fprintf(outfp, "\n") < 0) return (-1) ;
	if (fflush(outfp) < 0) return (-1);
	return (0);
}	/* WriteBmLine */



static int
SwapBookmarkFiles(void)
{
	char pidStr[32];
	char pathName[256], path2[256];

	(void) OurDirectoryPath(path2, sizeof(path2), kBookmarkFileName);
	(void) OurDirectoryPath(pathName, sizeof(pathName), kTmpBookmarkFileName);
	(void) sprintf(pidStr, "-%u.txt", (unsigned int) getpid());
	(void) STRNCAT(pathName, pidStr);

	(void) remove(path2);
	if (rename(pathName, path2) < 0) {
		return (-1);
	}
	return (0);
}	/* SwapBookmarkFiles */






/* Saves a Bookmark structure into the bookmarks file. */
FILE *
OpenTmpBookmarkFile(int nb)
{
	FILE *outfp;
	char pidStr[32];
	char pathName[256], path2[256];

	if (gOurDirectoryPath[0] == '\0')
		return (NULL);		/* Don't create in root directory. */

	(void) OurDirectoryPath(path2, sizeof(path2), kBookmarkFileName);
	(void) OurDirectoryPath(pathName, sizeof(pathName), kTmpBookmarkFileName);
	(void) sprintf(pidStr, "-%u.txt", (unsigned int) getpid());
	(void) STRNCAT(pathName, pidStr);

	outfp = fopen(pathName, FOPEN_WRITE_TEXT);
	if (outfp == NULL) {
		(void) fprintf(stderr, "Could not save bookmark.\n");
		perror(pathName);
		return (NULL);
	}
	(void) chmod(pathName, 00600);
	if (nb > 0) {
		if (fprintf(outfp, "NcFTP bookmark-file version: %d\nNumber of bookmarks: %d\n", kBookmarkVersion, nb) < 0) {
			(void) fprintf(stderr, "Could not save bookmark.\n");
			perror(pathName);
			(void) fclose(outfp);
			return (NULL);
		}
	} else {
		if (fprintf(outfp, "NcFTP bookmark-file version: %d\nNumber of bookmarks: ??\n", kBookmarkVersion) < 0) {
			(void) fprintf(stderr, "Could not save bookmark.\n");
			perror(pathName);
			(void) fclose(outfp);
			return (NULL);
		}
	}

	return (outfp);
}	/* OpenTmpBookmarkFile */




int
SaveBookmarkTable(void)
{
	int i;
	FILE *outfp;
	int nb;

	if ((gNumBookmarks < 1) || (gBookmarkTable == NULL))
		return (0);	/* Nothing to save. */

	/* Get a count of live bookmarks. */
	for (i=0, nb=0; i<gNumBookmarks; i++) {
		if (gBookmarkTable[i].deleted == 0)
			nb++;
	}
	outfp = OpenTmpBookmarkFile(nb);
	if (outfp == NULL) {
		return (-1);
	}

	for (i=0; i<gNumBookmarks; i++) {
		if (gBookmarkTable[i].deleted == 0) {
			if (WriteBmLine(gBookmarkTable + i, outfp, 1) < 0) {
				CloseBookmarkFile(outfp);
				return (-1);
			}
		}
	}
	CloseBookmarkFile(outfp);
	if (SwapBookmarkFiles() < 0) {
		return (-1);
	}
	return (0);
}	/* SaveBookmarkTable */



/* Saves a Bookmark structure into the bookmarks file. */
int
PutBookmark(Bookmark *bmp, int savePassword)
{
	FILE *infp, *outfp;
	char line[256];
	char bmAbbr[64];
	int replaced = 0;
	size_t len;

	outfp = OpenTmpBookmarkFile(0);
	if (outfp == NULL)
		return (-1);

	(void) STRNCPY(bmAbbr, bmp->bookmarkName);
	(void) STRNCAT(bmAbbr, ",");
	len = strlen(bmAbbr);

	/* This may fail the first time we ever save a bookmark. */
	infp = OpenBookmarkFile(NULL);
	if (infp != NULL) {
		while (FGets(line, sizeof(line), infp) != NULL) {
			if (strncmp(line, bmAbbr, len) == 0) {
				/* Replace previous entry. */
				if (WriteBmLine(bmp, outfp, savePassword) < 0) {
					(void) fprintf(stderr, "Could not save bookmark.\n");
					perror("reason");
					(void) fclose(outfp);
				}
				replaced = 1;
			} else {
				if (fprintf(outfp, "%s\n", line) < 0) {
					(void) fprintf(stderr, "Could not save bookmark.\n");
					perror("reason");
					(void) fclose(outfp);
					return (-1);
				}
			}
		}
		CloseBookmarkFile(infp);
	}

	if (replaced == 0) {
		/* Add it as a new bookmark. */
		if (WriteBmLine(bmp, outfp, savePassword) < 0) {
			(void) fprintf(stderr, "Could not save bookmark.\n");
			perror("reason");
			(void) fclose(outfp);
			return (-1);
		}
	}

	if (fclose(outfp) < 0) {
		(void) fprintf(stderr, "Could not save bookmark.\n");
		perror("reason");
		return (-1);
	}

	if (SwapBookmarkFiles() < 0) {
		(void) fprintf(stderr, "Could not rename bookmark file.\n");
		perror("reason");
		return (-1);
	}
	return (0);
}	/* PutBookmark */




/* Tries to generate a bookmark abbreviation based off of the hostname. */
void
DefaultBookmarkName(char *dst, size_t siz, char *src)
{
	char str[128];
	const char *token;
	const char *cp;

	(void) STRNCPY(str, src);
	
	/* Pick the first "significant" part of the hostname.  Usually
	 * this is the first word in the name, but if it's something like
	 * ftp.unl.edu, we would want to choose "unl" and not "ftp."
	 */
	token = str;
	if ((token = strtok(str, ".")) == NULL)
		token = str;
	else if ((ISTRNEQ(token, "ftp", 3)) || (ISTRNEQ(token, "www", 3))) {
		if ((token = strtok(NULL, ".")) == NULL)
			token = "";
	}
	for (cp = token; ; cp++) {
		if (*cp == '\0') {
			/* Token was all digits, like an IP address perhaps. */
			token = "";
		}
		if (!isdigit((int) *cp))
			break;
	}
	(void) Strncpy(dst, token, siz);
}	/* DefaultBookmarkName */
