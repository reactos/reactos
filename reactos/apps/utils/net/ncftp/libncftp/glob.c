/* glob.c
 *
 * Copyright (c) 1996-2001 Mike Gleason, NCEMRSoft.
 * All rights reserved.
 *
 */

#include "syshdrs.h"

static const char *rwx[9] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx", NULL };



/* We need to use this because using NLST gives us more stuff than
 * we want back sometimes.  For example, say we have:
 *
 * /a		(directory)
 * /a/b		(directory)
 * /a/b/b1
 * /a/b/b2
 * /a/b/b3
 * /a/c		(directory)
 * /a/c/c1
 * /a/c/c2
 * /a/c/c3
 * /a/file
 *
 * If you did an "echo /a/<star>" in a normal unix shell, you would expect
 * to get back /a/b /a/c /a/file.  But NLST gives back:
 *
 * /a/b/b1
 * /a/b/b2
 * /a/b/b3
 * /a/c/c1
 * /a/c/c2
 * /a/c/c3
 * /a/file
 *
 * So we use the following routine to convert into the format I expect.
 */

static void
RemoteGlobCollapse(const char *pattern, LineListPtr fileList)
{
	LinePtr lp, nextLine;
	string patPrefix;
	string cur, prev;
	char *endp, *cp, *dp;
	const char *pp;
	int wasGlobChar;
	size_t plen;

	/* Copy all characters before the first glob-char. */
	dp = patPrefix;
	endp = dp + sizeof(patPrefix) - 1;
	wasGlobChar = 0;
	for (cp = (char *) pattern; dp < endp; ) {
		for (pp=kGlobChars; *pp != '\0'; pp++) {
			if (*pp == *cp) {
				wasGlobChar = 1;
				break;
			}
		}
		if (wasGlobChar)
			break;
		*dp++ = *cp++;
	}
	*dp = '\0';
	plen = (size_t) (dp - patPrefix);

	*prev = '\0';
	for (lp=fileList->first; lp != NULL; lp = nextLine) {
		nextLine = lp->next;
		if (strncmp(lp->line, patPrefix, plen) == 0) {
			(void) STRNCPY(cur, lp->line + plen);
			cp = strchr(cur, '/');
			if (cp == NULL)
				cp = strchr(cur, '\\');
			if (cp != NULL)
				*cp = '\0';
			if ((*prev != '\0') && (STREQ(cur, prev))) {
				nextLine = RemoveLine(fileList, lp);
			} else {
				(void) STRNCPY(prev, cur);
				/* We are playing with a dynamically
				 * allocated string, but since the
				 * following expression is guaranteed
				 * to be the same or shorter, we won't
				 * overwrite the bounds.
				 */
				(void) sprintf(lp->line, "%s%s", patPrefix, cur);
			}
		}
	}
}	/* RemoteGlobCollapse */




#if 0
/* May need this later. */
static void
CheckForLS_d(FTPCIPtr cip)
{
	LineList lines;
	char *cp;

	if (cip->hasNLST_d == kCommandAvailabilityUnknown) {
		if (FTPListToMemory2(cip, ".", &lines, "-d ", 0, (int *) 0) == kNoErr) {
			if ((lines.first != NULL) && (lines.first == lines.last)) {
				/* If we have only one item in the list, see if it really was
				 * an error message we would recognize.
				 */
				cp = strchr(lines.first->line, ':');
				if ((cp != NULL) && STREQ(cp, ": No such file or directory")) {
					cip->hasNLST_d = kCommandNotAvailable;
				} else {
					cip->hasNLST_d = kCommandAvailable;
				}
			} else {
				cip->hasNLST_d = kCommandNotAvailable;
			}
		} else {
			cip->hasNLST_d = kCommandNotAvailable;
		}
		DisposeLineListContents(&lines);
	}
}	/* CheckForLS_d */
#endif




static int
LsMonthNameToNum(char *cp)
{
	int mon;	/* 0..11 */

	switch (*cp++) {
		case 'A':
			mon = (*cp == 'u') ? 7 : 3;
			break;
		case 'D':
			mon = 11;
			break;
		case 'F':
			mon = 1;
			break;
		default:
		case 'J':
			if (*cp++ == 'u')
				mon = (*cp == 'l') ? 6 : 5;
			else
				mon = 0;
			break;
		case 'M':
			mon = (*++cp == 'r') ? 2 : 4;
			break;
		case 'N':
			mon = 10;
			break;
		case 'O':
			mon = 9;
			break;
		case 'S':
			mon = 8;
	}
	return (mon);
}	/* LsMonthNameToNum */




static int
UnDosLine(	char *const line,
		const char *const curdir,
		size_t curdirlen,
		char *fname,
		size_t fnamesize,
		int *ftype,
		longest_int *fsize,
		time_t *ftime)
{
	char *cp;
	int hour, year;
	char *filestart;
	char *sizestart;
	struct tm ftm;

	/*
	 *
0123456789012345678901234567890123456789012345678901234567890123456789
04-27-99  10:32PM               270158 Game booklet.pdf
03-11-99  10:03PM       <DIR>          Get A3d Banner

We also try to parse the format from CMD.EXE, which is similar:

03/22/2001  06:23p              62,325 cls.pdf

	 *
	 */
	cp = line;
	if (
		isdigit((int) cp[0])
		&& isdigit((int) cp[1])
		&& ispunct((int) cp[2])
		&& isdigit((int) cp[3])
		&& isdigit((int) cp[4])
		&& ispunct((int) cp[5])
		&& isdigit((int) cp[6])
		&& isdigit((int) cp[7])
	) {
		(void) memset(&ftm, 0, sizeof(struct tm));
		ftm.tm_isdst = -1;
		cp[2] = '\0';
		ftm.tm_mon = atoi(cp + 0);
		if (ftm.tm_mon > 0)
			ftm.tm_mon -= 1;
		cp[5] = '\0';
		ftm.tm_mday = atoi(cp + 3);
		if ((isdigit((int) cp[8])) && (isdigit((int) cp[9]))) {
			/* Four-digit year */
			cp[10] = '\0';
			year = atoi(cp + 6);
			if (year > 1900)
				year -= 1900;
			ftm.tm_year = year;	/* years since 1900 */
			cp += 11;
		} else {
			/* Two-digit year */
			cp[8] = '\0';
			year = atoi(cp + 6);
			if (year < 98)
				year += 100;
			ftm.tm_year = year;	/* years since 1900 */
			cp += 9;
		}

		for (;;) {
			if (*cp == '\0')
				return (-1);
			if (isdigit(*cp))
				break;
			cp++;
		}

		cp[2] = '\0';
		hour = atoi(cp);
		if (((cp[5] == 'P') || (cp[5] == 'p')) && (hour < 12))
			hour += 12;
		else if (((cp[5] == 'A') || (cp[5] == 'a')) && (hour == 12))
			hour -= 12;
		ftm.tm_hour = hour;
		cp[5] = '\0';
		ftm.tm_min = atoi(cp + 3);
		*ftime = mktime(&ftm);
		if (*ftype == (time_t) -1)
			return (-1);

		cp += 6;
		*ftype = '-';
		for (;;) {
			if (*cp == '\0')
				return (-1);
			if ((*cp == '<') && (cp[1] == 'D')) {
				/* found <DIR> */
				*ftype = 'd';
				cp += 5;
				break;	/* size field will end up being empty string */
			} else if ((*cp == '<') && (cp[1] == 'J')) {
				/* found <JUNCTION>
				 *
				 * Will we ever really see this?
				 * IIS from Win2000sp1 sends <DIR>
				 * for FTP, but CMD.EXE prints
				 * <JUNCTION>.
				 */
				*ftype = 'd';
				cp += 10;
				break;
			} else if (isdigit(*cp)) {
				break;
			} else {
				cp++;
			}
		}

		sizestart = cp;
		for (;;) {
			if (*cp == '\0')
				return (-1);
#ifdef HAVE_MEMMOVE
			if (*cp == ',') {
				/* Yuck -- US Locale dependency */
				memmove(cp, cp + 1, strlen(cp + 1) + 1);
			}
#endif
			if (!isdigit(*cp)) {
				*cp++ = '\0';
				break;
			}
			cp++;
		}

		if (fsize != NULL) {
#if defined(HAVE_LONG_LONG) && defined(SCANF_LONG_LONG)
			if (*ftype == 'd')
				*fsize = 0;
			else
				(void) sscanf(sizestart, SCANF_LONG_LONG, fsize);
#elif defined(HAVE_LONG_LONG) && defined(HAVE_STRTOQ)
			if (*ftype == 'd')
				*fsize = 0;
			else
				*fsize = (longest_int) strtoq(sizestart, NULL, 0);
#else
			*fsize = (longest_int) 0;
			if (*ftype != 'd') {
				long fsize2 = 0L;

				(void) sscanf(sizestart, "%ld", &fsize2);
				*fsize = (longest_int) fsize2;
			}
#endif
		}

		for (;;) {
			if (*cp == '\0')
				return (-1);
			if (!isspace(*cp)) {
				break;
			}
			cp++;
		}

		filestart = cp;
		if (curdirlen == 0) {
			(void) Strncpy(fname, filestart, fnamesize);
		} else {
			(void) Strncpy(fname, curdir, fnamesize);
			(void) Strncat(fname, filestart, fnamesize);
		}

		return (0);
	}
	return (-1);
}	/* UnDosLine */




static int
UnLslRLine(	char *const line,
		const char *const curdir,
		size_t curdirlen,
		char *fname,
		size_t fnamesize,
		char *linkto,
		size_t linktosize,
		int *ftype,
		longest_int *fsize,
		time_t *ftime,
		time_t now,
		int thisyear,
		int *plugend)
{
	char *cp;
	int mon = 0, dd = 0, hr = 0, min = 0, year = 0;
	char *monstart, *ddstart, *hrstart, *minstart, *yearstart;
	char *linktostart, *filestart = NULL;
	char *sizestart;
	char *pe;
	struct tm ftm;

	/*
	 * Look for the digit just before the space
	 * before the month name.
	 *
-rw-rw----   1 gleason  sysdev      33404 Mar 24 01:29 RCmd.o
-rw-rw-r--   1 gleason  sysdevzz     1829 Jul  7  1996 README
-rw-rw-r--   1 gleason  sysdevzz     1829 Jul 7   1996 README
-rw-rw-r--   1 gleason  sysdevzz     1829 Jul  7 1996  README
-rw-rw-r--   1 gleason  sysdevzz     1829 Jul 7  1996  README
         *
	 *------------------------------^
	 *                              0123456789012345
	 *------plugend--------^
	 *                     9876543210
	 *
	 */
	for (cp = line; *cp != '\0'; cp++) {
		if (	(isdigit((int) *cp))
			&& (isspace((int) cp[1]))
			&& (isupper((int) cp[2]))
			&& (islower((int) cp[3]))
		/*	&& (islower((int) cp[4])) */
			&& (isspace((int) cp[5]))
			&& (
((isdigit((int) cp[6])) && (isdigit((int) cp[7])))
|| ((isdigit((int) cp[6])) && (isspace((int) cp[7])))
|| ((isspace((int) cp[6])) && (isdigit((int) cp[7])))
			)
			&& (isspace((int) cp[8]))
		) {
			monstart = cp + 2;
			ddstart = cp + 6;
			if (	((isspace((int) cp[9])) || (isdigit((int) cp[9])))
				&& (isdigit((int) cp[10]))
				&& (isdigit((int) cp[11]))
				&& (isdigit((int) cp[12]))
				&& ((isdigit((int) cp[13])) || (isspace((int) cp[13])))
			) {
				/* "Mon DD  YYYY" form */
				yearstart = cp + 9;
				if (isspace((int) *yearstart))
					yearstart++;
				hrstart = NULL;
				minstart = NULL;
				filestart = cp + 15;
				cp[1] = '\0';	/* end size */
				cp[5] = '\0';	/* end mon */
				cp[8] = '\0';	/* end dd */
				cp[14] = '\0';	/* end year */
				mon = LsMonthNameToNum(monstart);
				dd = atoi(ddstart);
				hr = 23;
				min = 59;
				year = atoi(yearstart);

				pe = cp;
				while (isdigit((int) *pe))
					pe--;
				while (isspace((int) *pe))
					pe--;
				*plugend = (int) (pe - line) + 1;
				break;
			} else if (	/*
					 * Windows NT does not 0 pad.
					(isdigit((int) cp[9])) &&
					 */
				(isdigit((int) cp[10]))
				&& (cp[11] == ':')
				&& (isdigit((int) cp[12]))
				&& (isdigit((int) cp[13]))
			) {
				/* "Mon DD HH:MM" form */
				yearstart = NULL;
				hrstart = cp + 9;
				minstart = cp + 12;
				filestart = cp + 15;
				cp[1] = '\0';	/* end size */
				cp[5] = '\0';	/* end mon */
				cp[8] = '\0';	/* end dd */
				cp[11] = '\0';	/* end hr */
				cp[14] = '\0';	/* end min */
				mon = LsMonthNameToNum(monstart);
				dd = atoi(ddstart);
				hr = atoi(hrstart);
				min = atoi(minstart);
				year = 0;

				pe = cp;
				while (isdigit((int) *pe))
					pe--;
				while (isspace((int) *pe))
					pe--;
				*plugend = (int) (pe - line) + 1;
				break;
			}
		}
	}

	if (*cp == '\0')
		return (-1);

	linktostart = strstr(filestart, " -> ");
	if (linktostart != NULL) {
		*linktostart = '\0';
		linktostart += 4;
		(void) Strncpy(linkto, linktostart, linktosize);
	} else {
		*linkto = '\0';
	}

	if (curdirlen == 0) {
		(void) Strncpy(fname, filestart, fnamesize);
	} else {
		(void) Strncpy(fname, curdir, fnamesize);
		(void) Strncat(fname, filestart, fnamesize);
	}

	if (ftime != NULL) {
		(void) memset(&ftm, 0, sizeof(struct tm));
		ftm.tm_mon = mon;
		ftm.tm_mday = dd;
		ftm.tm_hour = hr;
		ftm.tm_min = min;
		ftm.tm_isdst = -1;
		if (year == 0) {
			/* We guess the year, based on what the
			 * current year is.  We know the file
			 * on the remote server is either less
			 * than six months old or less than
			 * one hour into the future.
			 */
			ftm.tm_year = thisyear - 1900;
			*ftime = mktime(&ftm);
			if (*ftime == (time_t) -1) {
				/* panic */
			} else if (*ftime > (now + (15552000L + 86400L))) {
				--ftm.tm_year;
				*ftime = mktime(&ftm);
			} else if (*ftime < (now - (15552000L + 86400L))) {
				++ftm.tm_year;
				*ftime = mktime(&ftm);
			}
		} else {
			ftm.tm_year = year - 1900;
			*ftime = mktime(&ftm);
		}
	}

	if (fsize != NULL) {
		while ((cp > line) && (isdigit((int) *cp)))
			--cp;
		sizestart = cp + 1;
#if defined(HAVE_LONG_LONG) && defined(SCANF_LONG_LONG)
		(void) sscanf(sizestart, SCANF_LONG_LONG, fsize);
#elif defined(HAVE_LONG_LONG) && defined(HAVE_STRTOQ)
		*fsize = (longest_int) strtoq(sizestart, NULL, 0);
#else
		{
			long fsize2 = 0L;
			
			(void) sscanf(sizestart, "%ld", &fsize2);
			*fsize = (longest_int) fsize2;
		}
#endif
	}

	switch (line[0]) {
		case 'd':
		case 'l':
			*ftype = (int) line[0];
			break;
		case 'b':
		case 'c':
		case 's':
			*ftype = (int) line[0];
			return (-1);
		default:
			*ftype = '-';
	}

	return (0);
}	/* UnLslRLine */



int
UnLslR(FileInfoListPtr filp, LineListPtr llp, int serverType)
{
	char curdir[256];
	char line[256];
	int hadblankline = 0;
	int len;
	size_t curdirlen = 0;
	char fname[256];
	char linkto[256];
	char *cp;
	longest_int fsize;
	int ftype;
	time_t ftime, now;
	int thisyear;
	struct tm *nowtm;
	int rc;
	LinePtr lp;
	FileInfo fi;
	int linesread = 0;
	int linesconverted = 0;
	size_t maxFileLen = 0;
	size_t maxPlugLen = 0;
	size_t fileLen;
	int plugend;

	(void) time(&now);
	nowtm = localtime(&now);
	if (nowtm == NULL)
		thisyear = 1970;	/* should never happen */
	else
		thisyear = nowtm->tm_year + 1900;

	curdir[0] = '\0';

	InitFileInfoList(filp);
	for (lp = llp->first; lp != NULL; lp = lp->next) {
		len = (int) strlen(STRNCPY(line, lp->line));
		if ((line[0] == 't') && (strncmp(line, "total", 5) == 0)) {
			/* total XX line? */
			if (line[len - 1] != ':') {
				hadblankline = 0;
				continue;
			}
			/* else it was a subdir named total */
		} else {
			for (cp = line; ; cp++) {
				if ((*cp == '\0') || (!isspace((int) *cp)))
					break;
			}
			if (*cp == '\0') {
				/* Entire line was blank. */
				/* separator line between dirs */
				hadblankline = 1;
				continue;
			}
		}

		if ((hadblankline != 0) && (line[len - 1] == ':')) {
			/* newdir */
			hadblankline = 0;
			if ((line[0] == '.') && (line[1] == '/')) {
				line[len - 1] = '/';
				(void) memcpy(curdir, line + 2, (size_t) len + 1 - 2);
				curdirlen = (size_t) (len - 2);
			} else if ((line[0] == '.') && (line[1] == '\\')) {
				line[len - 1] = '\\';
				(void) memcpy(curdir, line + 2, (size_t) len + 1 - 2);
				curdirlen = (size_t) (len - 2);
			} else {
				line[len - 1] = '/';
				(void) memcpy(curdir, line, (size_t) len + 1);
				curdirlen = (size_t) len;
			}
			continue;
		}

		linesread++;
		rc = UnLslRLine(line, curdir, curdirlen, fname, sizeof(fname), linkto, sizeof(linkto), &ftype, &fsize, &ftime, now, thisyear, &plugend);
		if ((rc < 0) && (serverType == kServerTypeMicrosoftFTP)) {
			rc = UnDosLine(line, curdir, curdirlen, fname, sizeof(fname), &ftype, &fsize, &ftime);
			if (rc == 0) {
				*linkto = '\0';
				plugend = 0;
			}
		}
		if (rc == 0) {
			linesconverted++;
			fileLen = strlen(fname);
			if (fileLen > maxFileLen)
				maxFileLen = fileLen;
			fi.relnameLen = fileLen;
			fi.relname = StrDup(fname);
			fi.rname = NULL;
			fi.lname = NULL;
			fi.rlinkto = (linkto[0] == '\0') ? NULL : StrDup(linkto);
			fi.mdtm = ftime;
			fi.size = (longest_int) fsize;
			fi.type = ftype;
			if (plugend > 0) {
				fi.plug = (char *) malloc((size_t) plugend + 1);
				if (fi.plug != NULL) {
					(void) memcpy(fi.plug, line, (size_t) plugend);
					fi.plug[plugend] = '\0';
					if ((size_t) plugend > maxPlugLen)
						maxPlugLen = (size_t) plugend;
				}
			} else {
				fi.plug = (char *) malloc(32);
				if (fi.plug != NULL) {
					strcpy(fi.plug, "----------   1 ftpuser  ftpusers");
					fi.plug[0] = (char) ftype;
					if (30 > maxPlugLen)
						maxPlugLen = (size_t) 30;
				}
			}
			(void) AddFileInfo(filp, &fi);
		}

		hadblankline = 0;
	}

	filp->maxFileLen = maxFileLen;
	filp->maxPlugLen = maxPlugLen;
	if (linesread == 0)
		return (0);
	return ((linesconverted > 0) ? linesconverted : (-1));
}	/* UnLslR */




int
UnMlsT(const char *const line0, const MLstItemPtr mlip)
{
	char *cp, *val, *fact;
	int ec;
	size_t len;
	char line[1024];
	
	memset(mlip, 0, sizeof(MLstItem));
	mlip->mode = -1;
	mlip->fsize = kSizeUnknown;
	mlip->ftype = '-';
	mlip->ftime = kModTimeUnknown;

	len = strlen(line0);
	if (len > (sizeof(line) - 1))
		return (-1);	/* Line too long, sorry. */
	/* This should be re-coded so does not need to make a
	 * copy of the buffer; it could be done in place.
	 */
	memcpy(line, line0, len + 1);

	/* Skip leading whitespace. */
	for (cp = line; *cp != '\0'; cp++) {
		if (! isspace(*cp))
			break;
	}

	while (*cp != '\0') {
		for (fact = cp; ; cp++) {
			if ((*cp == '\0') || (*cp == ' ')) {
				/* protocol violation */
				return (-1);
			}
			if (*cp == '=') {
				/* End of fact name. */
				*cp++ = '\0';
				break;
			}
		}
		for (val = cp; ; cp++) {
			if (*cp == '\0') {
				/* protocol violation */
				return (-1);
			}
			if (*cp == ' ') {
				ec = ' ';
				*cp++ = '\0';
				break;
			} else if (*cp == ';') {
				if (cp[1] == ' ') {
					ec = ' ';
					*cp++ = '\0';
					*cp++ = '\0';
				} else {
					ec = ';';
					*cp++ = '\0';
				}
				break;
			}
		}
		if (ISTRNEQ(fact, "OS.", 3))
			fact += 3;
		if (ISTREQ(fact, "type")) {
			if (ISTREQ(val, "file")) {
				mlip->ftype = '-';
			} else if (ISTREQ(val, "dir")) {
				mlip->ftype = 'd';
			} else if (ISTREQ(val, "cdir")) {
				/* not supported: current directory */
				return (-2);
			} else if (ISTREQ(val, "pdir")) {
				/* not supported: parent directory */
				return (-2);
			} else {
				/* ? */
				return (-1);
			}
		} else if (ISTREQ(fact, "UNIX.mode")) {
			if (val[0] == '0')
				sscanf(val, "%o", &mlip->mode);
			else	
				sscanf(val, "%i", &mlip->mode);
			if (mlip->mode != (-1))
				mlip->mode &= 00777;
		} else if (ISTREQ(fact, "perm")) {
			STRNCPY(mlip->perm, val);
		} else if (ISTREQ(fact, "size")) {
#if defined(HAVE_LONG_LONG) && defined(SCANF_LONG_LONG)
			(void) sscanf(val, SCANF_LONG_LONG, &mlip->fsize);
#elif defined(HAVE_LONG_LONG) && defined(HAVE_STRTOQ)
			mlip->fsize = (longest_int) strtoq(val, NULL, 0);
#else
			{
				long fsize2 = 0L;

				(void) sscanf(val, "%ld", &fsize2);
				mlip->fsize = (longest_int) fsize2;
			}
#endif
		} else if (ISTREQ(fact, "modify")) {
			mlip->ftime = UnMDTMDate(val);
		} else if (ISTREQ(fact, "UNIX.owner")) {
			STRNCPY(mlip->owner, val);
		} else if (ISTREQ(fact, "UNIX.group")) {
			STRNCPY(mlip->group, val);
		} else if (ISTREQ(fact, "UNIX.uid")) {
			mlip->uid = atoi(val);
		} else if (ISTREQ(fact, "UNIX.gid")) {
			mlip->gid = atoi(val);
		} else if (ISTREQ(fact, "perm")) {
			STRNCPY(mlip->perm, val);
		}

		/* End of facts? */
		if (ec == ' ')
			break;
	}

	len = strlen(cp);
	if (len > (sizeof(mlip->fname) - 1)) {
		/* Filename too long */
		return (-1);
	}
	memcpy(mlip->fname, cp, len);

	/* also set linkto here if used */

	return (0);
}	/* UnMlsT */




int
UnMlsD(FileInfoListPtr filp, LineListPtr llp)
{
	MLstItem mli;
	char plug[64];
	char og[32];
	int rc;
	LinePtr lp;
	FileInfo fi;
	int linesread = 0;
	int linesconverted = 0;
	int linesignored = 0;
	size_t maxFileLen = 0;
	size_t maxPlugLen = 0;
	size_t fileLen, plugLen;
	int m1, m2, m3;
	const char *cm1, *cm2, *cm3;

	InitFileInfoList(filp);
	for (lp = llp->first; lp != NULL; lp = lp->next) {
		linesread++;
		rc = UnMlsT(lp->line, &mli);
		if (rc == 0) {
			linesconverted++;
			fileLen = strlen(mli.fname);
			if (fileLen > maxFileLen)
				maxFileLen = fileLen;
			fi.relnameLen = fileLen;
			fi.relname = StrDup(mli.fname);
			fi.rname = NULL;
			fi.lname = NULL;
			fi.rlinkto = (mli.linkto[0] == '\0') ? NULL : StrDup(mli.linkto);
			fi.mdtm = mli.ftime;
			fi.size = (longest_int) mli.fsize;
			fi.type = mli.ftype;
			plug[0] = (char) mli.ftype;
			plug[1] = '\0';
			m1 = 0;
			m2 = 0;
			m3 = -1;
			if (mli.mode != (-1)) {
				m1 = (mli.mode & 00700) >> 6;
				m2 = (mli.mode & 00070) >> 3;
				m3 = (mli.mode & 00007);
			}
			if (mli.perm[0] != '\0') {
				m3 = 0;
				if (fi.type == 'd') {
					if (strchr(mli.perm, 'e') != NULL) {
						/* execute -> execute */
						m3 |= 00001;
					}
					if (strchr(mli.perm, 'c') != NULL) {
						/* create -> write */
						m3 |= 00002;
					}
					if (strchr(mli.perm, 'l') != NULL) {
						/* list -> read */
						m3 |= 00004;
					}
				} else {
					if (strchr(mli.perm, 'w') != NULL) {
						/* write -> write */
						m3 |= 00002;
					}
					if (strchr(mli.perm, 'r') != NULL) {
						/* read -> read */
						m3 |= 00004;
					}
				}
			}
			if (m3 != (-1)) {
				cm1 = rwx[m1];
				cm2 = rwx[m2];
				cm3 = rwx[m3];
				sprintf(plug + 1, "%s%s%s", cm1, cm2, cm3);
			}
			if (mli.owner[0] != '\0') {
				if (mli.group[0] != '\0') {
#ifdef HAVE_SNPRINTF
					snprintf(og, sizeof(og) - 1,
#else
					sprintf(og,
#endif	/* HAVE_SNPRINTF */
						"   %-8.8s %s",
						mli.owner, mli.group
					);
					STRNCAT(plug, og);
				} else {
					STRNCAT(plug, "   ");
					STRNCAT(plug, mli.owner);
				}
			}
			fi.plug = StrDup(plug);
			if (fi.plug != NULL) {
				plugLen = strlen(plug);
				if (plugLen > maxPlugLen)
					maxPlugLen = plugLen;
			}
			(void) AddFileInfo(filp, &fi);
		} else if (rc == (-2)) {
			linesignored++;
		}
	}

	filp->maxFileLen = maxFileLen;
	filp->maxPlugLen = maxPlugLen;
	if (linesread == 0)
		return (0);
	linesconverted += linesignored;
	return ((linesconverted > 0) ? linesconverted : (-1));
}	/* UnMlsD */



#if 0
static void
print1(FileInfoListPtr list)
{
	FileInfoPtr fip;
	int i;

	for (i = 1, fip = list->first; fip != NULL; fip = fip->next, i++)
		printf("%d: %s\n", i, fip->relname == NULL ? "NULL" : fip->relname);
}



static void
print2(FileInfoListPtr list)
{
	FileInfoPtr fip;
	int i, n;

	n = list->nFileInfos;
	for (i=0; i<n; i++) {
		fip = list->vec[i];
		printf("%d: %s\n", i + 1, fip->relname == NULL ? "NULL" : fip->relname);
	}
}




static void
SortRecursiveFileList(FileInfoListPtr files)
{
	VectorizeFileInfoList(files);
	SortFileInfoList(files, 'b', '?');
	UnvectorizeFileInfoList(files);
}	/* SortRecursiveFileList */
#endif




int
FTPRemoteRecursiveFileList1(FTPCIPtr cip, char *const rdir, FileInfoListPtr files)
{
	LineList dirContents;
	FileInfoList fil;
	char cwd[512];
	int result;

	if ((result = FTPGetCWD(cip, cwd, sizeof(cwd))) < 0)
		return (result);

	InitFileInfoList(files);

	if (rdir == NULL)
		return (-1);

	if (FTPChdir(cip, rdir) < 0) {
		/* Probably not a directory.
		 * Just add it as a plain file
		 * to the list.
		 */
		(void) ConcatFileToFileInfoList(files, rdir);
		return (kNoErr);
	}

	/* Paths collected must be relative. */
	if ((result = FTPListToMemory2(cip, "", &dirContents, "-lRa", 1, (int *) 0)) < 0) {
		if ((result = FTPChdir(cip, cwd)) < 0) {
			return (result);
		}
	}

	(void) UnLslR(&fil, &dirContents, cip->serverType);
	DisposeLineListContents(&dirContents);
	/* Could sort it to breadth-first here. */
	/* (void) SortRecursiveFileList(&fil); */
	(void) ComputeRNames(&fil, rdir, 1, 1);
	(void) ConcatFileInfoList(files, &fil);
	DisposeFileInfoListContents(&fil);

	if ((result = FTPChdir(cip, cwd)) < 0) {
		return (result);
	}
	return (kNoErr);
}	/* FTPRemoteRecursiveFileList1 */




int
FTPRemoteRecursiveFileList(FTPCIPtr cip, LineListPtr fileList, FileInfoListPtr files)
{
	LinePtr filePtr, nextFilePtr;
	LineList dirContents;
	FileInfoList fil;
	char cwd[512];
	int result;
	char *rdir;

	if ((result = FTPGetCWD(cip, cwd, sizeof(cwd))) < 0)
		return (result);

	InitFileInfoList(files);

	for (filePtr = fileList->first;
		filePtr != NULL;
		filePtr = nextFilePtr)
	{
		nextFilePtr = filePtr->next;

		rdir = filePtr->line;
		if (rdir == NULL)
			continue;

		if (FTPChdir(cip, rdir) < 0) {
			/* Probably not a directory.
			 * Just add it as a plain file
			 * to the list.
			 */
			(void) ConcatFileToFileInfoList(files, rdir);
			continue;
		}

		/* Paths collected must be relative. */
		if ((result = FTPListToMemory2(cip, "", &dirContents, "-lRa", 1, (int *) 0)) < 0) {
			goto goback;
		}

		(void) UnLslR(&fil, &dirContents, cip->serverType);
		DisposeLineListContents(&dirContents);
		(void) ComputeRNames(&fil, rdir, 1, 1);
		(void) ConcatFileInfoList(files, &fil);
		DisposeFileInfoListContents(&fil);

goback:
		if ((result = FTPChdir(cip, cwd)) < 0) {
			return (result);
		}
	}	
	return (kNoErr);
}	/* FTPRemoteRecursiveFileList */



#if defined(WIN32) || defined(_WINDOWS)

static void
Traverse(FTPCIPtr cip, char *fullpath, struct Stat *st, char *relpath, FileInfoListPtr filp)
{
	WIN32_FIND_DATA ffd;
	HANDLE searchHandle;
	DWORD dwErr;
	char *cp, *c2;
	const char *file;
	FileInfo fi;

	/* Handle directory entry first. */
	if (relpath[0] != '\0') {
		fi.relname = StrDup(relpath);
		fi.rname = NULL;
		fi.lname = StrDup(fullpath);
		fi.rlinkto = NULL;
		fi.plug = NULL;
		fi.mdtm = st->st_mtime;
		fi.size = (longest_int) st->st_size;
		fi.type = 'd';
		(void) AddFileInfo(filp, &fi);
	}

	cp = fullpath + strlen(fullpath);
	*cp++ = LOCAL_PATH_DELIM;
	strcpy(cp, "*.*");

	c2 = relpath + strlen(relpath);
	*c2++ = LOCAL_PATH_DELIM;
	*c2 = '\0';

	memset(&ffd, 0, sizeof(ffd));

	/* "Open" the directory. */
	searchHandle = FindFirstFile(fullpath, &ffd);
	if (searchHandle == INVALID_HANDLE_VALUE) {
		return;
	}

	for (;;) {

		file = ffd.cFileName;
		if ((*file == '.') && ((file[1] == '\0') || ((file[1] == '.') && (file[2] == '\0')))) {
			/* It was "." or "..", so skip it. */
			goto next;
		}

		(void) strcpy(cp, file);	/* append name after slash */
		(void) strcpy(c2, file);	

		if (Lstat(fullpath, st) < 0) {
			Error(cip, kDoPerror, "could not stat %s.\n", fullpath);
			goto next;
		}

		fi.relname = StrDup(relpath + (((relpath[0] == '/') || (relpath[0] == '\\')) ? 1 : 0));
		fi.rname = NULL;
		fi.lname = StrDup(fullpath);
		fi.mdtm = st->st_mtime;
		fi.size = (longest_int) st->st_size;
		fi.rlinkto = NULL;
		fi.plug = NULL;

		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			Traverse(cip, fullpath, st, relpath, filp);
		} else {
			/* file */
			fi.type = '-';
			(void) AddFileInfo(filp, &fi);
		}

next:
#if _DEBUG
		memset(&ffd, 0, sizeof(ffd));
#endif
		if (!FindNextFile(searchHandle, &ffd)) {
			dwErr = GetLastError();
			if (dwErr != ERROR_NO_MORE_FILES) {
				FindClose(searchHandle);
				return;
			}
			break;
		}
	}
	FindClose(searchHandle);
}	// Traverse

#else

static void
Traverse(FTPCIPtr cip, char *fullpath, struct Stat *st, char *relpath, FileInfoListPtr filp)
{
	char *dname;
	struct dirent *dirp;
	mode_t m;
	DIR *dp;
	char *cp;
	char *c2;
	FileInfo fi;

	if (relpath[0] != '\0') {
		fi.relname = StrDup(relpath);
		fi.rname = NULL;
		fi.lname = StrDup(fullpath);
		fi.rlinkto = NULL;
		fi.plug = NULL;
		fi.mdtm = st->st_mtime;
		fi.size = (longest_int) st->st_size;
		fi.type = 'd';
		(void) AddFileInfo(filp, &fi);
	}

	/* Handle directory entry first. */
	cp = fullpath + strlen(fullpath);
	*cp++ = '/';
	*cp = '\0';

	c2 = relpath + strlen(relpath);
	*c2++ = '/';
	*c2 = '\0';

	if ((dp = opendir(fullpath)) == NULL) {
		cp[-1] = '\0';
		c2[-1] = '\0';
		Error(cip, kDoPerror, "could not opendir %s.\n", fullpath);
		return;
	}

	while ((dirp = readdir(dp)) != NULL) {
		dname = dirp->d_name;
		if ((dname[0] == '.') && ((dname[1] == '\0') || ((dname[1] == '.') && (dname[2] == '\0'))))
			continue;	/* skip "." and ".." directories. */

		(void) strcpy(cp, dirp->d_name);	/* append name after slash */
		(void) strcpy(c2, dirp->d_name);	
		if (Lstat(fullpath, st) < 0) {
			Error(cip, kDoPerror, "could not stat %s.\n", fullpath);
			continue;
		}

		fi.relname = StrDup(relpath + (((relpath[0] == '/') || (relpath[0] == '\\')) ? 1 : 0));
		fi.rname = NULL;
		fi.lname = StrDup(fullpath);
		fi.mdtm = st->st_mtime;
		fi.size = (longest_int) st->st_size;
		fi.rlinkto = NULL;
		fi.plug = NULL;

		m = st->st_mode;
		if (S_ISREG(m) != 0) {
			/* file */
			fi.type = '-';
			(void) AddFileInfo(filp, &fi);
		} else if (S_ISDIR(m)) {
			Traverse(cip, fullpath, st, relpath, filp);
#ifdef S_ISLNK
		} else if (S_ISLNK(m)) {
			fi.type = 'l';
			fi.rlinkto = calloc(128, 1);
			if (fi.rlinkto != NULL) {
				if (readlink(fullpath, fi.rlinkto, 127) < 0) {
					free(fi.rlinkto);
				} else {
					(void) AddFileInfo(filp, &fi);
				}
			}
#endif	/* S_ISLNK */
		}
	}
	cp[-1] = '\0';
	c2[-1] = '\0';

	(void) closedir(dp);
}	/* Traverse */

#endif





int
FTPLocalRecursiveFileList2(FTPCIPtr cip, LineListPtr fileList, FileInfoListPtr files, int erelative)
{
	LinePtr filePtr, nextFilePtr;
#if defined(WIN32) || defined(_WINDOWS)
	char fullpath[_MAX_PATH + 1];	
	char relpath[_MAX_PATH + 1];
#else
	char fullpath[512];	
	char relpath[512];
#endif
	struct Stat st;
	FileInfo fi;
	char *cp;

	InitFileInfoList(files);

	for (filePtr = fileList->first;
		filePtr != NULL;
		filePtr = nextFilePtr)
	{
		nextFilePtr = filePtr->next;

		(void) STRNCPY(fullpath, filePtr->line);	/* initialize fullpath */
		if ((erelative != 0) || (strcmp(filePtr->line, ".") == 0) || (filePtr->line[0] == '\0'))
			(void) STRNCPY(relpath, "");
		else if ((cp = StrRFindLocalPathDelim(filePtr->line)) == NULL)
			(void) STRNCPY(relpath, filePtr->line);
		else
			(void) STRNCPY(relpath, cp + 1);
		if (Lstat(fullpath, &st) < 0) {
			Error(cip, kDoPerror, "could not stat %s.\n", fullpath);
			continue;
		}

		if (S_ISDIR(st.st_mode) == 0) {
			fi.relname = StrDup(relpath);
			fi.rname = NULL;
			fi.lname = StrDup(fullpath);
			fi.mdtm = st.st_mtime;
			fi.size = (longest_int) st.st_size;
			fi.rlinkto = NULL;
			fi.plug = NULL;
			fi.type = '-';
			(void) AddFileInfo(files, &fi);
			continue;			/* wasn't a directory */
		}

		/* Paths collected must be relative. */
		Traverse(cip, fullpath, &st, relpath, files);
	}
	return (kNoErr);
}	/* FTPLocalRecursiveFileList */




int
FTPLocalRecursiveFileList(FTPCIPtr cip, LineListPtr fileList, FileInfoListPtr files)
{
	return (FTPLocalRecursiveFileList2(cip, fileList, files, 0));
}	/* FTPLocalRecursiveFileList */



int
FTPRemoteGlob(FTPCIPtr cip, LineListPtr fileList, const char *pattern, int doGlob)
{
	char *cp;
	const char *lsflags;
	LinePtr lp;
	int result;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if (fileList == NULL)
		return (kErrBadParameter);
	InitLineList(fileList);

	if ((pattern == NULL) || (pattern[0] == '\0'))
		return (kErrBadParameter);

	/* Note that we do attempt to use glob characters even if the remote
	 * host isn't UNIX.  Most non-UNIX remote FTP servers look for UNIX
	 * style wildcards.
	 */
	if ((doGlob == 1) && (GLOBCHARSINSTR(pattern))) {
		/* Use NLST, which lists files one per line. */
		lsflags = "";
		
		/* Optimize for "NLST *" case which is same as "NLST". */
		if (strcmp(pattern, "*") == 0) {
			pattern = "";
		} else if (strcmp(pattern, "**") == 0) {
			/* Hack; Lets you try "NLST -a" if you're daring. */
			pattern = "";
			lsflags = "-a";
		}

		if ((result = FTPListToMemory2(cip, pattern, fileList, lsflags, 0, (int *) 0)) < 0) {
			if (*lsflags == '\0')
				return (result);
			/* Try again, without "-a" */
			lsflags = "";
			if ((result = FTPListToMemory2(cip, pattern, fileList, lsflags, 0, (int *) 0)) < 0) {
				return (result);
			}
		}
		if (fileList->first == NULL) {
			cip->errNo = kErrGlobNoMatch;
			return (kErrGlobNoMatch);
		}
		if (fileList->first == fileList->last) {
#define glberr(a) (ISTRNEQ(cp, a, strlen(a)))
			/* If we have only one item in the list, see if it really was
			 * an error message we would recognize.
			 */
			cp = strchr(fileList->first->line, ':');
			if (cp != NULL) {
				if (glberr(": No such file or directory")) {
					(void) RemoveLine(fileList, fileList->first);
					cip->errNo = kErrGlobFailed;
					return (kErrGlobFailed);
				} else if (glberr(": No match")) {
					cip->errNo = kErrGlobNoMatch;
					return (kErrGlobNoMatch);
				}
			}
		}
		RemoteGlobCollapse(pattern, fileList);
		for (lp=fileList->first; lp != NULL; lp = lp->next)
			PrintF(cip, "  Rglob [%s]\n", lp->line);
	} else {
		/* Or, if there were no globbing characters in 'pattern', then the
		 * pattern is really just a filename.  So for this case the
		 * file list is really just a single file.
		 */
		fileList->first = fileList->last = NULL;
		(void) AddLine(fileList, pattern);
	}
	return (kNoErr);
}	/* FTPRemoteGlob */




/* This does "tilde-expansion."  Examples:
 * ~/pub         -->  /usr/gleason/pub
 * ~pdietz/junk  -->  /usr/pdietz/junk
 */
static void
ExpandTilde(char *pattern, size_t siz)
{
	string pat;
	char *cp, *rest, *firstent;
#if defined(WIN32) || defined(_WINDOWS)
#else
	struct passwd *pw;
#endif
	string hdir;

	if ((pattern[0] == '~') &&
	(isalnum((int) pattern[1]) || IsLocalPathDelim(pattern[1]) || (pattern[1] == '\0'))) {
		(void) STRNCPY(pat, pattern);
		if ((cp = StrFindLocalPathDelim(pat)) != NULL) {
			*cp = 0;
			rest = cp + 1;	/* Remember stuff after the ~/ part. */
		} else {
			rest = NULL;	/* Was just a ~ or ~username.  */
		}
		if (pat[1] == '\0') {
			/* Was just a ~ or ~/rest type.  */
			GetHomeDir(hdir, sizeof(hdir));
			firstent = hdir;
		} else {
#if defined(WIN32) || defined(_WINDOWS)
			return;
#else
			/* Was just a ~username or ~username/rest type.  */
			pw = getpwnam(pat + 1);
			if (pw != NULL)
				firstent = pw->pw_dir;
			else
				return;		/* Bad user -- leave it alone. */
#endif
		}
		
		(void) Strncpy(pattern, firstent, siz);
		if (rest != NULL) {
			(void) Strncat(pattern, LOCAL_PATH_DELIM_STR, siz);
			(void) Strncat(pattern, rest, siz);
		}
	}
}	/* ExpandTilde */





#if defined(WIN32) || defined(_WINDOWS)

static int
WinLocalGlob(FTPCIPtr cip, LineListPtr fileList, const char *const srcpat)
{
	char pattern[_MAX_PATH];
	WIN32_FIND_DATA ffd;
	HANDLE searchHandle;
	DWORD dwErr;
	char *cp;
	const char *file;
	int result;

	STRNCPY(pattern, srcpat);

	/* Get rid of trailing slashes. */
	cp = pattern + strlen(pattern) - 1;
	while ((cp >= pattern) && IsLocalPathDelim(*cp))
		*cp-- = '\0';

	memset(&ffd, 0, sizeof(ffd));

	/* "Open" the directory. */
	searchHandle = FindFirstFile(pattern, &ffd);
	if (searchHandle == INVALID_HANDLE_VALUE) {
		dwErr = GetLastError();
		return ((dwErr == 0) ? 0 : -1);
	}

	/* Get rid of basename. */
	cp = StrRFindLocalPathDelim(pattern);
	if (cp == NULL)
		cp = pattern;
	else
		cp++;
	*cp = '\0';

	for (result = 0;;) {
		file = ffd.cFileName;
		if ((file[0] == '.') && ((file[1] == '\0') || ((file[1] == '.') && (file[2] == '\0')))) {
			/* skip */
		} else {
			Strncpy(cp, ffd.cFileName, sizeof(pattern) - (cp - pattern));
			PrintF(cip, "  Lglob [%s]\n", pattern);
			(void) AddLine(fileList, pattern);
		}

		if (!FindNextFile(searchHandle, &ffd)) {
			dwErr = GetLastError();
			if (dwErr != ERROR_NO_MORE_FILES) {
				result = ((dwErr == 0) ? 0 : -1);
			}
			break;
		}
	}

	return (result);
}	// WinLocalGlob

#else

static int
LazyUnixLocalGlob(FTPCIPtr cip, LineListPtr fileList, const char *const pattern)
{
	string cmd;
	longstring gfile;
	FILE *fp;
	FTPSigProc sp;
	
	/* Do it the easy way and have the shell do the dirty
	 * work for us.
	 */
#ifdef HAVE_SNPRINTF
	(void) snprintf(cmd, sizeof(cmd) - 1, "%s -c \"%s %s %s\"", "/bin/sh", "/bin/ls",
		"-d", pattern);
	cmd[sizeof(cmd) - 1] = '\0';
#else
	(void) sprintf(cmd, "%s -c \"%s %s %s\"", "/bin/sh", "/bin/ls",
		"-d", pattern);
#endif
	
	fp = (FILE *) popen(cmd, "r");
	if (fp == NULL) {
		Error(cip, kDoPerror, "Could not Lglob: [%s]\n", cmd);
		cip->errNo = kErrGlobFailed;
		return (kErrGlobFailed);
	}
	sp = NcSignal(SIGPIPE, (FTPSigProc) SIG_IGN);
	while (FGets(gfile, sizeof(gfile), (FILE *) fp) != NULL) {
		PrintF(cip, "  Lglob [%s]\n", gfile);
		(void) AddLine(fileList, gfile);
	}
	(void) pclose(fp);
	(void) NcSignal(SIGPIPE, sp);
	return (kNoErr);
}	/* LazyUnixLocalGlob */

#endif




int
FTPLocalGlob(FTPCIPtr cip, LineListPtr fileList, const char *pattern, int doGlob)
{
	string pattern2;
	int result;

	if (cip == NULL)
		return (kErrBadParameter);
	if (strcmp(cip->magic, kLibraryMagic))
		return (kErrBadMagic);

	if (fileList == NULL)
		return (kErrBadParameter);
	InitLineList(fileList);

	if ((pattern == NULL) || (pattern[0] == '\0'))
		return (kErrBadParameter);

	(void) STRNCPY(pattern2, pattern);	/* Don't nuke the original. */
	
	/* Pre-process for ~'s. */ 
	ExpandTilde(pattern2, sizeof(pattern2));
	InitLineList(fileList);
	result = kNoErr;

	if ((doGlob == 1) && (GLOBCHARSINSTR(pattern2))) {
#if defined(WIN32) || defined(_WINDOWS)
		result = WinLocalGlob(cip, fileList, pattern2);
#else
		result = LazyUnixLocalGlob(cip, fileList, pattern2);
#endif
	} else {
		/* Or, if there were no globbing characters in 'pattern', then
		 * the pattern is really just a single pathname.
		 */
		(void) AddLine(fileList, pattern2);
	}

	return (result);
}	/* FTPLocalGlob */




static int
FTPFtwL2(const FTPCIPtr cip, char *dir, char *end, size_t dirsize, FTPFtwProc proc, int maxdepth)
{
	LineList fileList;
	LinePtr filePtr;
	char *file, *cp;
	int result;

	if (maxdepth <= 0) {
		result = cip->errNo = kErrRecursionLimitReached;
		return (result);
	}

	result = FTPRemoteGlob(cip, &fileList, "**", kGlobYes);
	if (result != kNoErr) {
		if (result == kErrGlobNoMatch)
			result = kNoErr;	/* empty directory is okay. */
		return (result);
	}

	for (filePtr = fileList.first;
		filePtr != NULL;
		filePtr = filePtr->next)
	{
		file = filePtr->line;
		if (file == NULL) {
			cip->errNo = kErrBadLineList;
			break;
		}

		if ((file[0] == '.') && ((file[1] == '\0') || ((file[1] == '.') && (file[2] == '\0'))))
			continue;	/* Skip . and .. */

		result = FTPIsDir(cip, file);
		if (result < 0) {
			/* error */
			/* could be just a stat error, so continue */
			continue;
		} else if (result == 1) {
			/* directory */
			cp = Strnpcat(dir, file, dirsize);
			result = (*proc)(cip, dir, kFtwDir);
			if (result != kNoErr)
				break;

			if ((strchr(dir, '/') == NULL) && (strrchr(dir, '\\') != NULL))
				*cp++ = '\\';
			else
				*cp++ = '/';
			*cp = '\0';

			result = FTPChdir(cip, file);
			if (result == kNoErr) {
				result = FTPFtwL2(cip, dir, cp, dirsize, proc, maxdepth - 1);
				if (result != kNoErr)
					break;
				if (FTPChdir(cip, "..") < 0) {
					result = kErrCannotGoToPrevDir;
					cip->errNo = kErrCannotGoToPrevDir;
					break;
				}
			}

			*end = '\0';
			if (result != 0)
				break;
		} else {
			/* file */
			cp = Strnpcat(dir, file, dirsize);
			result = (*proc)(cip, dir, kFtwFile);
			*end = '\0';
			if (result != 0)
				break;
		}
	}
	DisposeLineListContents(&fileList);

	return (result);
} 	/* FTPFtwL2 */



int
FTPFtw(const FTPCIPtr cip, const char *const dir, FTPFtwProc proc, int maxdepth)
{
	int result, result2;
	char *cp;
	char savedcwd[1024];
	char curcwd[2048];

	result = FTPIsDir(cip, dir);
	if (result < 0) {
		/* error */
		return result;
	} else if (result == 0) {
		result = cip->errNo = kErrNotADirectory;
		return (result);
	}

	/* Preserve old working directory. */
	(void) FTPGetCWD(cip, savedcwd, sizeof(savedcwd));

	result = FTPChdir(cip, dir);
	if (result != kNoErr) {
		return (result);
	}

	/* Get full working directory we just changed to. */
	result = FTPGetCWD(cip, curcwd, sizeof(curcwd) - 3);
	if (result != kNoErr) {
		if (FTPChdir(cip, savedcwd) != kNoErr) {
			result = kErrCannotGoToPrevDir;
			cip->errNo = kErrCannotGoToPrevDir;
		}
		return (result);
	}

	result2 = (*proc)(cip, curcwd, kFtwDir);
	if (result2 == kNoErr) {
		cp = curcwd + strlen(curcwd);

		if ((strchr(curcwd, '/') == NULL) && (strrchr(curcwd, '\\') != NULL))
			*cp++ = '\\';
		else
			*cp++ = '/';
		*cp = '\0';
		result = FTPFtwL2(cip, curcwd, cp, sizeof(curcwd), proc, maxdepth - 1);
	}


	if (FTPChdir(cip, savedcwd) != kNoErr) {
		/* Could not cd back to the original user directory -- bad. */
		result = kErrCannotGoToPrevDir;
		cip->errNo = kErrCannotGoToPrevDir;
		return (result);
	}

	if ((result2 != kNoErr) && (result == kNoErr))
		result = result2;

	return (result);
}	/* FTPFtw */

/* eof */
