/* ls.c
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#include "syshdrs.h"
#include "util.h"
#include "ls.h"
#include "trace.h"

/* The program keeps a timestamp of 6 months ago and an hour from now, because
 * the standard /bin/ls command will print the time (i.e. "Nov  8 09:20")
 * instead of the year (i.e. "Oct 27  1996") if a file's timestamp is within
 * this period.
 */
time_t gNowMinus6Mon, gNowPlus1Hr;

/* An array of month name abbreviations.  This may not be in English. */
char gLsMon[13][4];

/* The program keeps its own cache of directory listings, so it doesn't
 * need to re-request them from the server.
 */
LsCacheItem gLsCache[kLsCacheSize];
int gOldestLsCacheItem;
int gLsCacheItemLifetime = kLsCacheItemLifetime;

extern FTPConnectionInfo gConn;
extern char gRemoteCWD[512];
extern int gScreenColumns, gDebug;


void
InitLsCache(void)
{
	(void) memset(gLsCache, 0, sizeof(gLsCache));
	gOldestLsCacheItem = 0;
}	/* InitLsCache */



/* Creates the ls monthname abbreviation array, so we don't have to
 * re-calculate them each time.
 */
void InitLsMonths(void)
{
	time_t now;
	struct tm *ltp;
	int i;

	(void) time(&now);
	ltp = localtime(&now);	/* Fill up the structure. */
	ltp->tm_mday = 15;
	ltp->tm_hour = 12;
	for (i=0; i<12; i++) {
		ltp->tm_mon = i;
		(void) strftime(gLsMon[i], sizeof(gLsMon[i]), "%b", ltp);
		gLsMon[i][sizeof(gLsMon[i]) - 1] = '\0';
	}
	(void) strcpy(gLsMon[i], "BUG");
}	/* InitLsMonths */




void InitLs(void)
{
	InitLsCache();
	InitLsMonths();
}	/* InitLs */




/* Deletes an item from the ls cache. */
static void
FlushLsCacheItem(int i)
{
	Trace(1, "flush ls cache item: %s\n", gLsCache[i].itempath);
	if (gLsCache[i].itempath != NULL)
		free(gLsCache[i].itempath);
	gLsCache[i].itempath = NULL;
	gLsCache[i].expiration = (time_t) 0;
	DisposeFileInfoListContents(&gLsCache[i].fil);
}	/* FlushLsCacheItem */




/* Clears all items from the ls cache. */
void
FlushLsCache(void)
{
	int i;

	for (i=0; i<kLsCacheSize; i++) {
		if (gLsCache[i].expiration != (time_t) 0) {
			FlushLsCacheItem(i);
		}
	}
}	/* FlushLsCache */




/* Checks the cache for a directory listing for the given path. */
int
LsCacheLookup(const char *const itempath)
{
	int i, j;
	time_t now;

	(void) time(&now);
	for (i=0, j=gOldestLsCacheItem; i<kLsCacheSize; i++) {
		if (--j < 0)
			j = kLsCacheSize - 1;
		if ((gLsCache[j].expiration != (time_t) 0) && (gLsCache[j].itempath != NULL)) {
			if (strcmp(itempath, gLsCache[j].itempath) == 0) {
				if (now > gLsCache[j].expiration) {
					/* Found it, but it was expired. */
					FlushLsCacheItem(j);
					return (-1);
				}
				gLsCache[j].hits++;
				return (j);
			}
		}
	}
	return (-1);
}	/* LsCacheLookup */




/* Saves a directory listing from the given path into the cache. */
static void
LsCacheAdd(const char *const itempath, FileInfoListPtr files)
{
	char *cp;
	int j;

	/* Never cache empty listings in case of errors */
	if (files->nFileInfos == 0)
		return;

	j = LsCacheLookup(itempath);
	if (j >= 0) {
		/* Directory was already in there;
		 * Replace it with the new
		 * contents.
		 */
		FlushLsCacheItem(j);
	}

	cp = StrDup(itempath);
	if (cp == NULL)
		return;

	j = gOldestLsCacheItem;
	(void) memcpy(&gLsCache[j].fil, files, sizeof(FileInfoList));
	(void) time(&gLsCache[j].expiration);
	gLsCache[j].expiration += gLsCacheItemLifetime;
	gLsCache[j].hits = 0;
	gLsCache[j].itempath = cp;
	Trace(1, "ls cache add: %s\n", itempath);

	/* Increment the pointer.  This is a circular array, so if it
	 * hits the end it wraps over to the other side.
	 */
	gOldestLsCacheItem++;
	if (gOldestLsCacheItem >= kLsCacheSize)
		gOldestLsCacheItem = 0;
}	/* LsCacheAdd */




/* Does "ls -C", or the nice columnized /bin/ls-style format. */
static void
LsC(FileInfoListPtr dirp, int endChars, FILE *stream)
{
	char buf[400];
	char buf2[400];
	int ncol, nrow;
	int i, j, k, l;
	int colw;
	int n;
	FileInfoVec itemv;
	FileInfoPtr itemp;
	char *cp1, *cp2, *lim;
	int screenColumns;

	screenColumns = gScreenColumns;
	if (screenColumns > 400)
		screenColumns = 400;
	ncol = (screenColumns - 1) / ((int) dirp->maxFileLen + 2 + /*1or0*/ endChars);
	if (ncol < 1)
		ncol = 1;
	colw = (screenColumns - 1) / ncol; 
	n = dirp->nFileInfos;
	nrow = n / ncol;
	if ((n % ncol) != 0)
		nrow++;

	for (i=0; i<(int) sizeof(buf2); i++)
		buf2[i] = ' ';

	itemv = dirp->vec;

	for (j=0; j<nrow; j++) {
		(void) memcpy(buf, buf2, sizeof(buf));
		for (i=0, k=j, l=0; i<ncol; i++, k += nrow, l += colw) {
			if (k >= n)
				continue;
			itemp = itemv[k];
			cp1 = buf + l;
			lim = cp1 + (int) (itemp->relnameLen);
			cp2 = itemp->relname;
			while (cp1 < lim)
				*cp1++ = *cp2++;
			if (endChars != 0) {
				if (itemp->type == 'l') {
					/* Regular ls always uses @
					 * for a symlink tail, even if
					 * the linked item is a directory.
					 */
					*cp1++ = '@';
				} else if (itemp->type == 'd') {
					*cp1++ = '/';
				}
			}
		}
		for (cp1 = buf + sizeof(buf); *--cp1 == ' '; ) {}
		++cp1;
		*cp1++ = '\n';
		*cp1 = '\0';
		(void) fprintf(stream, "%s", buf);
		Trace(0, "%s", buf);
	}
}	/* LsC */



/* Converts a timestamp into a recent date string ("May 27 06:33"), or an
 * old (or future) date string (i.e. "Oct 27  1996").
 */
void
LsDate(char *dstr, time_t ts)
{
	struct tm *gtp;

	if (ts == kModTimeUnknown) {
		(void) strcpy(dstr, "            ");
		return;
	}
	gtp = localtime(&ts);
	if (gtp == NULL) {
		(void) strcpy(dstr, "Jan  0  1900");
		return;
	}
	if ((ts > gNowPlus1Hr) || (ts < gNowMinus6Mon)) {
		(void) sprintf(dstr, "%s %2d  %4d",
			gLsMon[gtp->tm_mon],
			gtp->tm_mday,
			gtp->tm_year + 1900
		);
	} else {
		(void) sprintf(dstr, "%s %2d %02d:%02d",
			gLsMon[gtp->tm_mon],
			gtp->tm_mday,
			gtp->tm_hour,
			gtp->tm_min
		);
	}
}	/* LsDate */




/* Does "ls -l", or the detailed /bin/ls-style, one file per line . */
void
LsL(FileInfoListPtr dirp, int endChars, int linkedTo, FILE *stream)
{
	FileInfoPtr diritemp;
	FileInfoVec diritemv;
	int i;
	char fTail[2];
	int fType;
	const char *l1, *l2;
	char datestr[16];
	char sizestr[32];
	char plugspec[16];
	char plugstr[64];
	const char *expad;

	fTail[0] = '\0';
	fTail[1] = '\0';

	(void) time(&gNowPlus1Hr);
	gNowMinus6Mon = gNowPlus1Hr - 15552000;
	gNowPlus1Hr += 3600;

	diritemv = dirp->vec;
#ifdef HAVE_SNPRINTF
	(void) snprintf(
		plugspec,
		sizeof(plugspec) - 1,
#else
	(void) sprintf(
		plugspec,
#endif
		"%%-%ds",
		(int) dirp->maxPlugLen
	);

	if (dirp->maxPlugLen < 29) {
		/* We have some extra space to work with,
		 * so we can space out the columns a little.
		 */
		expad = "  ";
	} else {
		expad = "";
	}

	for (i=0; ; i++) {
		diritemp = diritemv[i];
		if (diritemp == NULL)
			break;

		fType = (int) diritemp->type;
		if (endChars != 0) {
			if (fType == 'd')
				fTail[0] = '/';
			else
				fTail[0] = '\0';
		}

		if (diritemp->rlinkto != NULL) {
			if (linkedTo != 0) {
				l1 = "";
				l2 = "";
			} else {
				l1 = " -> ";
				l2 = diritemp->rlinkto;
			}
		} else {
			l1 = "";
			l2 = "";
		}

		LsDate(datestr, diritemp->mdtm);

		if (diritemp->size == kSizeUnknown) {
			*sizestr = '\0';
		} else {
#ifdef HAVE_SNPRINTF
			(void) snprintf(
				sizestr,
				sizeof(sizestr) - 1,
#else
			(void) sprintf(
				sizestr,
#endif
#if defined(HAVE_LONG_LONG) && defined(PRINTF_LONG_LONG)
				PRINTF_LONG_LONG,
#else
				"%ld",
#endif
				(longest_int) diritemp->size
			);
		}

#ifdef HAVE_SNPRINTF
		(void) snprintf(
			plugstr,
			sizeof(plugstr) - 1,
#else
		(void) sprintf(
			plugstr,
#endif
			plugspec,
			diritemp->plug
		);

		(void) fprintf(stream, "%s %12s %s%s %s%s%s%s%s\n",
			plugstr,
			sizestr,
			expad,
			datestr,
			expad,
			diritemp->relname,
			l1,
			l2,
			fTail
		);
		Trace(0, "%s %12s %s%s %s%s%s%s%s\n",
			plugstr,
			sizestr,
			expad,
			datestr,
			expad,
			diritemp->relname,
			l1,
			l2,
			fTail
		);
	}
}	/* LsL */




/* Does "ls -1", or the simple single-column /bin/ls-style format, with
 * one file per line.
 */
void
Ls1(FileInfoListPtr dirp, int endChars, FILE *stream)
{
	char fTail[2];
	int i;
	int fType;
	FileInfoVec diritemv;
	FileInfoPtr diritemp;

	fTail[0] = '\0';
	fTail[1] = '\0';
	diritemv = dirp->vec;

	for (i=0; ; i++) {
		diritemp = diritemv[i];
		if (diritemp == NULL)
			break;

		fType = (int) diritemp->type;
		if (endChars != 0) {
			if (fType == 'd')
				fTail[0] = '/';
			else
				fTail[0] = '\0';
		}

		(void) fprintf(stream, "%s%s\n",
			diritemp->relname,
			fTail
		);

		Trace(0, "%s%s\n",
			diritemp->relname,
			fTail
		);
	}
}	/* Ls1 */





/* Prints a directory listing in the specified format on the specified
 * output stream.  It may or may not need to request it from the remote
 * server, depending on whether it was cached.
 */
void
Ls(const char *const item, int listmode, const char *const options, FILE *stream)
{
	char itempath[512];
	FileInfoList fil;
	FileInfoListPtr filp;
	LinePtr linePtr, nextLinePtr;
	LineList dirContents;
	int parsed;
	int linkedTo;
	int endChars;
	int rlisted;
	int opt;
	const char *cp;
	int sortBy;
	int sortOrder;
	int unknownOpts;
	char optstr[32];
	char unoptstr[32];
	int doNotUseCache;
	int wasInCache;
	int mlsd;
	int ci;

	InitLineList(&dirContents);
	InitFileInfoList(&fil);

	sortBy = 'n';		/* Sort by filename. */
	sortOrder = 'a';	/* Sort in ascending order. */
	linkedTo = 0;
	endChars = (listmode == 'C') ? 1 : 0;
	unknownOpts = 0;
	memset(unoptstr, 0, sizeof(unoptstr));
	unoptstr[0] = '-';
	doNotUseCache = 0;
	rlisted = 0;

	for (cp = options; *cp != '\0'; cp++) {
		opt = *cp;
		switch (opt) {
			case 't':
				sortBy = 't';		/* Sort by modification time. */
				break;
			case 'S':
				sortBy = 's';		/* Sort by size. */
				break;
			case 'r':
				sortOrder = 'd';	/* descending order */
				break;
			case 'L':
				linkedTo = 1;
				break;
			case 'f':
				doNotUseCache = 1;
				break;
			case 'F':
			case 'p':
				endChars = 1;
				break;
			case '1':
			case 'C':
			case 'l':
				listmode = opt;
				break;
			case '-':
				break;
			default:
				if (unknownOpts < ((int) sizeof(unoptstr) - 2))
					unoptstr[unknownOpts + 1] = opt;
				unknownOpts++;
				break;
		}
	}

	/* Create a possibly relative path into an absolute path. */
	PathCat(itempath, sizeof(itempath), gRemoteCWD,
		(item == NULL) ? "." : item);

	if (unknownOpts > 0) {
		/* Can't handle these -- pass them through
		 * to the server.
		 */

		Trace(0, "ls caching not used because of ls flags: %s\n", unoptstr);
		optstr[0] = '-';
		optstr[1] = listmode;
		optstr[2] = '\0';
		(void) STRNCAT(optstr, options);
		if ((FTPListToMemory2(&gConn, (item == NULL) ? "" : item, &dirContents, optstr, 1, 0)) < 0) {
			if (stream != NULL)
				(void) fprintf(stderr, "List failed.\n");
			return;
		}

		rlisted = 1;
		parsed = -1;
		wasInCache = 0;
		filp = NULL;
	} else if ((doNotUseCache != 0) || ((ci = LsCacheLookup(itempath)) < 0)) {
		/* Not in cache. */
		wasInCache = 0;

		mlsd = 1;
		if ((FTPListToMemory2(&gConn, (item == NULL) ? "" : item, &dirContents, "-l", 1, &mlsd)) < 0) {
			if (stream != NULL)
				(void) fprintf(stderr, "List failed.\n");
			return;
		}

		rlisted = 1;
		filp = &fil;
		if (mlsd != 0) {
			parsed = UnMlsD(filp, &dirContents);
			if (parsed < 0) {
				Trace(0, "UnMlsD: %d\n", parsed);
			}
		} else {
			parsed = UnLslR(filp, &dirContents, gConn.serverType);
			if (parsed < 0) {
				Trace(0, "UnLslR: %d\n", parsed);
			}
		}
		if (parsed >= 0) {
			VectorizeFileInfoList(filp);
			if (filp->vec == NULL) {
				if (stream != NULL)
					(void) fprintf(stderr, "List processing failed.\n");
				return;
			}
		}
	} else {
		filp = &gLsCache[ci].fil;
		wasInCache = 1;
		parsed = 1;
		Trace(0, "ls cache hit: %s\n", itempath);
	}

	if (rlisted != 0) {
		Trace(0, "Remote listing contents {\n");	
		for (linePtr = dirContents.first;
			linePtr != NULL;
			linePtr = nextLinePtr)
		{
			nextLinePtr = linePtr->next;
			Trace(0, "    %s\n", linePtr->line);	
		}
		Trace(0, "}\n");	
	}

	if (parsed >= 0) {
		SortFileInfoList(filp, sortBy, sortOrder);
		if (stream != NULL) {
			if (listmode == 'l')
				LsL(filp, endChars, linkedTo, stream);
			else if (listmode == '1')
				Ls1(filp, endChars, stream);
			else
				LsC(filp, endChars, stream);
		}
		if (wasInCache == 0) {
			LsCacheAdd(itempath, filp);
		}
	} else if (stream != NULL) {
		for (linePtr = dirContents.first;
			linePtr != NULL;
			linePtr = nextLinePtr)
		{
			nextLinePtr = linePtr->next;
			(void) fprintf(stream, "%s\n", linePtr->line);	
			Trace(0, "    %s\n", linePtr->line);	
		}
	}

	DisposeLineListContents(&dirContents);
}	/* Ls */

	
	
#if defined(WIN32) || defined(_WINDOWS)
/* Prints a local directory listing in the specified format on the specified
 * output stream.
 */
void
LLs(const char *const item, int listmode, const char *const options, FILE *stream)
{
	char itempath[512];
	int linkedTo;
	int endChars;
	int opt;
	const char *cp;
	int sortBy;
	int sortOrder;
	int unknownOpts;
	char unoptstr[32];
	LineList ll;
	FileInfoPtr fip, fip2;
	FileInfoList fil;
	struct Stat st;
	int result;
	size_t len;

	InitLineList(&ll);
	InitFileInfoList(&fil);

	sortBy = 'n';		/* Sort by filename. */
	sortOrder = 'a';	/* Sort in ascending order. */
	linkedTo = 0;
	endChars = (listmode == 'C') ? 1 : 0;
	unknownOpts = 0;
	memset(unoptstr, 0, sizeof(unoptstr));
	unoptstr[0] = '-';

	for (cp = options; *cp != '\0'; cp++) {
		opt = *cp;
		switch (opt) {
			case 't':
				sortBy = 't';		/* Sort by modification time. */
				break;
			case 'S':
				sortBy = 's';		/* Sort by size. */
				break;
			case 'r':
				sortOrder = 'd';	/* descending order */
				break;
			case 'L':
				linkedTo = 1;
				break;
			case 'f':
				break;
			case 'F':
			case 'p':
				endChars = 1;
				break;
			case '1':
			case 'C':
			case 'l':
				listmode = opt;
				break;
			case '-':
				break;
			default:
				if (unknownOpts < ((int) sizeof(unoptstr) - 2))
					unoptstr[unknownOpts + 1] = opt;
				unknownOpts++;
				break;
		}
	}

	if ((item == NULL) || (strcmp(item, ".") == 0))
		STRNCPY(itempath, "*.*");
	else {
		STRNCPY(itempath, item);
		if (strpbrk(itempath, "*?") == NULL)
			STRNCAT(itempath, "\\*.*");
	}
	
	InitLineList(&ll);
	result = FTPLocalGlob(&gConn, &ll, itempath, kGlobYes);
	if (result < 0) {
		FTPPerror(&gConn, result, kErrGlobFailed, "local glob", itempath);
		DisposeLineListContents(&ll);
		return;
	}
	if (LineListToFileInfoList(&ll, &fil) < 0)
		return;
	DisposeLineListContents(&ll);
	
	for (fip = fil.first; fip != NULL; fip = fip2) {
		fip2 = fip->next;
		if (Stat(fip->relname, &st) < 0) {
			fip2 = RemoveFileInfo(&fil, fip);
			continue;
		}
		cp = StrRFindLocalPathDelim(fip->relname);
		if (cp != NULL) {
			/* FTPLocalGlob will tack on the pathnames too,
			 * which we don't want for this hack.
			 */
			cp++;
			len = strlen(cp);
			memmove(fip->relname, cp, len + 1);
		} else {
			len = strlen(fip->relname);
		}
		if (len > fil.maxFileLen)
			fil.maxFileLen = len;
		fip->relnameLen = len;
		fip->rname = StrDup(fip->relname);
		fip->lname = StrDup(fip->relname);
		fip->plug = StrDup("----------   1 user  group");
		if (S_ISDIR(st.st_mode)) {
			fip->type = 'd';
			fip->plug[0] = 'd';
		} else {
			fip->type = '-';
			fip->size = st.st_size;
		}
		fip->mdtm = st.st_mtime;
	}
	fil.maxPlugLen = strlen("----------   1 user  group");

	VectorizeFileInfoList(&fil);
	SortFileInfoList(&fil, sortBy, sortOrder);
	if (stream != NULL) {
		if (listmode == 'l')
			LsL(&fil, endChars, linkedTo, stream);
		else if (listmode == '1')
			Ls1(&fil, endChars, stream);
		else
			LsC(&fil, endChars, stream);
	}
	
	DisposeFileInfoListContents(&fil);
}	/* LLs */
#endif

