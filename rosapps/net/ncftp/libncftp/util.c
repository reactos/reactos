/* util.c
 *
 * Copyright (c) 1996-2001 Mike Gleason, NCEMRSoft.
 * All rights reserved.
 *
 */

#include "syshdrs.h"

#if defined(WIN32) || defined(_WINDOWS)
#include <conio.h>
extern void GetSpecialDir(char *dst, size_t size, int whichDir);
#endif


static void *
Realloc(void *ptr, size_t siz)
{
	if (ptr == NULL)
		return (void *) malloc(siz);
	return ((void *) realloc(ptr, siz));
}	/* Realloc */


/* Use getcwd/getwd to get the full path of the current local
 * working directory.
 */
char *
FTPGetLocalCWD(char *buf, size_t size)
{
#ifdef HAVE_GETCWD
	static char *cwdBuf = NULL;
	static size_t cwdBufSize = 0;

	if (cwdBufSize == 0) {
		cwdBufSize = (size_t) 128;
		cwdBuf = (char *) malloc(cwdBufSize);
	}

	for (errno = 0; ; ) {
		if (cwdBuf == NULL) {
			return NULL;
		}

		if (getcwd(cwdBuf, cwdBufSize) != NULL)
			break;
		if (errno != ERANGE) {
			(void) Strncpy(buf, ".", size);
			return NULL;
		}
		cwdBufSize *= 2;
		cwdBuf = (char *) Realloc(cwdBuf, cwdBufSize);
	}
	
	return (Strncpy(buf, cwdBuf, size));
#else
#ifdef HAVE_GETWD
	static char *cwdBuf = NULL;
	char *dp;
	
	/* Due to the way getwd is usually implemented, it's
	 * important to have a buffer large enough to hold the
	 * whole thing.  getwd usually starts at the end of the
	 * buffer, and works backwards, returning you a pointer
	 * to the beginning of it when it finishes.
	 */
	if (size < MAXPATHLEN) {
		/* Buffer not big enough, so use a temporary one,
		 * and then copy the first 'size' bytes of the
		 * temporary buffer to your 'buf.'
		 */
		if (cwdBuf == NULL) {
			cwdBuf = (char *) malloc((size_t) MAXPATHLEN);
			if (cwdBuf == NULL) {
				return NULL;
			}
		}
		dp = cwdBuf;
	} else {
		/* Buffer is big enough already. */
		dp = buf;
	}
	*dp = '\0';
	if (getwd(dp) == NULL) {
		/* getwd() should write the reason why in the buffer then,
		 * according to the man pages.
		 */
		(void) Strncpy(buf, ".", size);
		return (NULL);
	}
	return (Strncpy(buf, dp, size));

#elif defined(WIN32) || defined(_WINDOWS)
	if (GetCurrentDirectory((DWORD) size - 1, buf) < 1)
		return NULL;
	buf[size - 1] = '\0';
	return buf;
#else
	/* Not a solution, but does anybody not have either of
	 * getcwd or getwd?
	 */
	--Error--;
#endif
#endif
}   /* GetCWD */



/* Read a line, and axe the end-of-line. */
char *
FGets(char *str, size_t size, FILE *fp)
{
	char *cp, *nlptr;
	
	cp = fgets(str, ((int) size) - 1, fp);
	if (cp != NULL) {
		cp[((int) size) - 1] = '\0';	/* ensure terminator */
		nlptr = cp + strlen(cp) - 1;
		if (*nlptr == '\n')
			*nlptr = '\0';
	} else {
		memset(str, 0, size);
	}
	return cp;
}	/* FGets */




#if defined(WIN32) || defined(_WINDOWS)

int gettimeofday(struct timeval *const tp, void *junk)
{
	SYSTEMTIME systemTime;

	GetSystemTime(&systemTime);

	/* Create an arbitrary second counter;
	 * Note that this particular one creates
	 * a problem at the end of the month.
	 */
	tp->tv_sec =
		systemTime.wSecond +
		systemTime.wMinute * 60 +
		systemTime.wHour * 3600 +
		systemTime.wDay * 86400;

	tp->tv_usec = systemTime.wMilliseconds * 1000;

	return 0;
}	/* gettimeofday */

#endif




#if defined(WIN32) || defined(_WINDOWS)
#else
/* This looks up the user's password entry, trying to look by the username.
 * We have a couple of extra hacks in place to increase the probability
 * that we can get the username.
 */
struct passwd *
GetPwByName(void)
{
	char *cp;
	struct passwd *pw;
	
	cp = getlogin();
	if (cp == NULL) {
		cp = (char *) getenv("LOGNAME");
		if (cp == NULL)
			cp = (char *) getenv("USER");
	}
	pw = NULL;
	if (cp != NULL)
		pw = getpwnam(cp);
	return (pw);
}	/* GetPwByName */
#endif



char *
GetPass(const char *const prompt)
{
#ifdef HAVE_GETPASS
	return getpass(prompt);
#elif defined(_CONSOLE) && (defined(WIN32) || defined(_WINDOWS))
	static char pwbuf[128];
	char *dst, *dlim;
	int c;

	(void) memset(pwbuf, 0, sizeof(pwbuf));
	if (! _isatty(_fileno(stdout)))
		return (pwbuf);
	(void) fputs(prompt, stdout);
	(void) fflush(stdout);

	for (dst = pwbuf, dlim = dst + sizeof(pwbuf) - 1;;) {
		c = _getch();
		if ((c == 0) || (c == 0xe0)) {
			/* The key is a function or arrow key; read and discard. */
			(void) _getch();
		}
		if ((c == '\r') || (c == '\n'))
			break;
		if (dst < dlim)
			*dst++ = c;
	}
	*dst = '\0';

	(void) fflush(stdout);
	(void) fflush(stdin);
	return (pwbuf);
#else
	static char pwbuf[128];

	(void) memset(pwbuf, 0, sizeof(pwbuf));
#if defined(WIN32) || defined(_WINDOWS)
	if (! _isatty(_fileno(stdout)))
#else
	if (! isatty(1))
#endif
		return (pwbuf);
	(void) fputs(prompt, stdout);
	(void) fflush(stdout);
	(void) FGets(pwbuf, sizeof(pwbuf), stdin);
	(void) fflush(stdout);
	(void) fflush(stdin);
	return (pwbuf);
#endif
}	/* GetPass */




void
GetHomeDir(char *dst, size_t size)
{
#if defined(WIN32) || defined(_WINDOWS)
	const char *homedrive, *homepath;

	homedrive = getenv("HOMEDRIVE");
	homepath = getenv("HOMEPATH");
	if ((homedrive != NULL) && (homepath != NULL)) {
		(void) Strncpy(dst, homedrive, size);
		(void) Strncat(dst, homepath, size);
		return;
	}

//	GetSpecialDir(dst, size, CSIDL_PERSONAL	/* "My Documents" */);
//	if (dst[0] != '\0')
//		return;
//
//	dst[0] = '\0';
//	if (GetWindowsDirectory(dst, size - 1) < 1)
//		(void) Strncpy(dst, ".", size);
//	else if (dst[1] == ':') {
//		dst[2] = '\\';
//		dst[3] = '\0';
//	}
#else
	const char *cp;
	struct passwd *pw;

	pw = NULL;
#if defined(USE_GETPWUID)
	/* Try to use getpwuid(), but if we have to, fall back to getpwnam(). */
	if ((pw = getpwuid(getuid())) == NULL)
		pw = GetPwByName();	/* Oh well, try getpwnam() then. */
#else
	/* Try to use getpwnam(), but if we have to, fall back to getpwuid(). */
	if ((pw = GetPwByName()) == NULL)
		pw = getpwuid(getuid());	/* Try getpwnam() then. */
#endif
	if (pw != NULL)
		cp = pw->pw_dir;
	else if ((cp = (const char *) getenv("LOGNAME")) == NULL)
			cp = ".";
	(void) Strncpy(dst, cp, size);
#endif
}	/* GetHomeDir */




void
GetUsrName(char *dst, size_t size)
{
#if defined(WIN32) || defined(_WINDOWS)
	DWORD size1;

	size1 = size - 1;
	if (! GetUserName(dst, &size1))
		(void) strncpy(dst, "unknown", size);
	dst[size - 1] = '\0';
#else
	const char *cp;
	struct passwd *pw;

	pw = NULL;
#ifdef USE_GETPWUID
	/* Try to use getpwuid(), but if we have to, fall back to getpwnam(). */
	if ((pw = getpwuid(getuid())) == NULL)
		pw = GetPwByName();	/* Oh well, try getpwnam() then. */
#else
	/* Try to use getpwnam(), but if we have to, fall back to getpwuid(). */
	if ((pw = GetPwByName()) == NULL)
		pw = getpwuid(getuid());	/* Try getpwnam() then. */
#endif
	if (pw != NULL)
		cp = pw->pw_name;
	else if ((cp = (const char *) getenv("LOGNAME")) == NULL)
			cp = "UNKNOWN";
	(void) Strncpy(dst, cp, size);
#endif
}	/* GetUserName */





/* Closes the file supplied, if it isn't a std stream. */
void
CloseFile(FILE **f)
{
	if (*f != NULL) {
		if ((*f != stdout) && (*f != stdin) && (*f != stderr))
			(void) fclose(*f);
		*f = NULL;
	}
}	/* CloseFile */



/*VARARGS*/
void
PrintF(const FTPCIPtr cip, const char *const fmt, ...)
{
	va_list ap;
	char buf[256];

	va_start(ap, fmt);
	if (cip->debugLog != NULL) {
		(void) vfprintf(cip->debugLog, fmt, ap);
		(void) fflush(cip->debugLog);
	}
	if (cip->debugLogProc != NULL) {
#ifdef HAVE_VSNPRINTF
		(void) vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
		buf[sizeof(buf) - 1] = '\0';
#else
		(void) vsprintf(buf, fmt, ap);
#endif
		(*cip->debugLogProc)(cip, buf);
	}
	va_end(ap);
}	/* PrintF */





/*VARARGS*/
void
Error(const FTPCIPtr cip, const int pError, const char *const fmt, ...)
{
	va_list ap;
	int errnum;
	size_t len;
	char buf[256];
	int endsinperiod;
	int endsinnewline;
#ifndef HAVE_STRERROR
	char errnostr[16];
#endif

	errnum = errno;
	va_start(ap, fmt);
#ifdef HAVE_VSNPRINTF
	vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
	buf[sizeof(buf) - 1] = '\0';
#else
	(void) vsprintf(buf, fmt, ap);
#endif
	va_end(ap);

	if (pError != 0) {
		len = strlen(buf);
		endsinperiod = 0;
		endsinnewline = 0;
		if (len > 2) {
			if (buf[len - 1] == '\n') {
				endsinnewline = 1;
				buf[len - 1] = '\0';
				if (buf[len - 2] == '.') {
					endsinperiod = 1;
					buf[len - 2] = '\0';
				}
			} else if (buf[len - 1] == '.') {
				endsinperiod = 1;
				buf[len - 1] = '\0';
			}
		}
#ifdef HAVE_STRERROR
		(void) STRNCAT(buf, ": ");
		(void) STRNCAT(buf, strerror(errnum));
#else
#	ifdef HAVE_SNPRINTF
		sprintf(errnostr, sizeof(errnostr) - 1, " (errno = %d)", errnum);
		errnostr[sizeof(errnostr) - 1] = '\0';
#	else
		sprintf(errnostr, " (errno = %d)", errnum);
#	endif
		STRNCAT(buf, errnostr);
#endif
		if (endsinperiod != 0)
			(void) STRNCAT(buf, ".");
		if (endsinnewline != 0)
			(void) STRNCAT(buf, "\n");
	}

	if (cip->errLog != NULL) {
		(void) fprintf(cip->errLog, "%s", buf);
		(void) fflush(cip->errLog);
	}
	if ((cip->debugLog != NULL) && (cip->debugLog != cip->errLog)) {
		if ((cip->errLog != stderr) || (cip->debugLog != stdout)) {
			(void) fprintf(cip->debugLog, "%s", buf);
			(void) fflush(cip->debugLog);
		}
	}
	if (cip->errLogProc != NULL) {
		(*cip->errLogProc)(cip, buf);
	}
	if ((cip->debugLogProc != NULL) && (cip->debugLogProc != cip->errLogProc)) {
		(*cip->debugLogProc)(cip, buf);
	}
}	/* Error */




/* Cheezy, but somewhat portable way to get GMT offset. */
#ifdef HAVE_MKTIME
static
time_t GetUTCOffset(int mon, int mday)
{
	struct tm local_tm, utc_tm, *utc_tmptr;
	time_t local_t, utc_t, utcOffset;

	ZERO(local_tm);
	ZERO(utc_tm);
	utcOffset = 0;
	
	local_tm.tm_year = 94;	/* Doesn't really matter. */
	local_tm.tm_mon = mon;
	local_tm.tm_mday = mday;
	local_tm.tm_hour = 12;
	local_tm.tm_isdst = -1;
	local_t = mktime(&local_tm);
	
	if (local_t != (time_t) -1) {
		utc_tmptr = gmtime(&local_t);
		utc_tm.tm_year = utc_tmptr->tm_year;
		utc_tm.tm_mon = utc_tmptr->tm_mon;
		utc_tm.tm_mday = utc_tmptr->tm_mday;
		utc_tm.tm_hour = utc_tmptr->tm_hour;
		utc_tm.tm_isdst = -1;
		utc_t = mktime(&utc_tm);

		if (utc_t != (time_t) -1)
			utcOffset = (local_t - utc_t);
	}
	return (utcOffset);
}	/* GetUTCOffset */
#endif	/* HAVE_MKTIME */



/* Converts a MDTM date, like "19930602204445"
 * format to a time_t.
 */
time_t UnMDTMDate(char *dstr)
{
#ifndef HAVE_MKTIME
	return (kModTimeUnknown);
#else
	struct tm ut, *t;
	time_t mt, now;
	time_t result = kModTimeUnknown;

	if (strncmp(dstr, "19100", 5) == 0) {
		/* Server Y2K bug! */
		return (result);
	}

	(void) time(&now);
	t = localtime(&now);

	/* Copy the whole structure of the 'tm' pointed to by t, so it will
	 * also set all fields we don't specify explicitly to be the same as
	 * they were in t.  That way we copy non-standard fields such as
	 * tm_gmtoff, if it exists or not.
	 */
	ut = *t;

	/* The time we get back from the server is (should be) in UTC. */
	if (sscanf(dstr, "%04d%02d%02d%02d%02d%02d",
		&ut.tm_year,
		&ut.tm_mon,
		&ut.tm_mday,
		&ut.tm_hour,
		&ut.tm_min,
		&ut.tm_sec) == 6)
	{	
		--ut.tm_mon;
		ut.tm_year -= 1900;
		mt = mktime(&ut);
		if (mt != (time_t) -1) {
			mt += GetUTCOffset(ut.tm_mon, ut.tm_mday);
			result = (time_t) mt;
		}
	}
	return result;
#endif	/* HAVE_MKTIME */
}	/* UnMDTMDate */



int
GetSockBufSize(int sockfd, size_t *rsize, size_t *ssize)
{
#ifdef SO_SNDBUF
	int rc = -1;
	int opt;
	int optsize;

	if (ssize != NULL) {
		opt = 0;
		optsize = sizeof(opt);
		rc = getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *) &opt, &optsize);
		if (rc == 0)
			*ssize = (size_t) opt;
		else
			*ssize = 0;
	}
	if (rsize != NULL) {
		opt = 0;
		optsize = sizeof(opt);
		rc = getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *) &opt, &optsize);
		if (rc == 0)
			*rsize = (size_t) opt;
		else
			*rsize = 0;
	}
	return (rc);
#else
	if (ssize != NULL)
		*ssize = 0;
	if (rsize != NULL)
		*rsize = 0;
	return (-1);
#endif
}	/* GetSockBufSize */



int
SetSockBufSize(int sockfd, size_t rsize, size_t ssize)
{
#ifdef SO_SNDBUF
	int rc = -1;
	int opt;
	int optsize;

#ifdef TCP_RFC1323
	/* This is an AIX-specific socket option to do RFC1323 large windows */
	if (ssize > 0 || rsize > 0) {
		opt = 1;
		optsize = sizeof(opt);
		rc = setsockopt(sockfd, IPPROTO_TCP, TCP_RFC1323, &opt, optsize);
	}
#endif

	if (ssize > 0) {
		opt = (int) ssize;
		optsize = sizeof(opt);
		rc = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *) &opt, optsize);
	}
	if (rsize > 0) {
		opt = (int) rsize;
		optsize = sizeof(opt);
		rc = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *) &opt, optsize);
	}
	return (rc);
#else
	return (-1);
#endif
}	/* SetSockBufSize */



void
Scramble(unsigned char *dst, size_t dsize, unsigned char *src, char *key)
{
	int i;
	unsigned int ch;
	unsigned char *k2;
	size_t keyLen;

	keyLen = strlen(key);
	k2 = (unsigned char *) key;
	for (i=0; i < (int) dsize - 1; i++) {
		ch = src[i];
		if (ch == 0)
			break;
		dst[i] = (unsigned char) (ch ^ (int) (k2[i % (int) keyLen]));
	}
	dst[i] = '\0';
}	/* Scramble */





#if defined(WIN32) || defined(_WINDOWS)
void WinSleep(unsigned int seconds)
{
	DWORD now, deadline;
	DWORD milliseconds = seconds * 1000;

	if (milliseconds > 0) {
		now = GetTickCount();
		deadline = now + milliseconds;
		if (now < deadline) {
			/* Typical case */
			do {
				milliseconds = deadline - now;
				Sleep(milliseconds);
				now = GetTickCount();
			} while (now < deadline);
		} else {
			/* Overflow case */
			deadline = now - 1;
			milliseconds -= (0xFFFFFFFF - now);
			do {
				Sleep(0xFFFFFFFF - now);
				now = GetTickCount();
			} while (now > deadline);
			/* Counter has now wrapped around */
			deadline = now + milliseconds;
			do {
				milliseconds = deadline - now;
				Sleep(milliseconds);
				now = GetTickCount();
			} while (now < deadline);
		}
	}
}	/* WinSleep */




char *
StrFindLocalPathDelim(const char *src) /* TODO: optimize */
{
	const char *first;
	int c;

	first = NULL;
	for (;;) {
		c = *src++;
		if (c == '\0')
			break;
		if (IsLocalPathDelim(c)) {
			first = src - 1;
			break;
		}
	}

	return ((char *) first);
}	/* StrFindLocalPathDelim */



char *
StrRFindLocalPathDelim(const char *src)	/* TODO: optimize */
{
	const char *last;
	int c;

	last = NULL;
	for (;;) {
		c = *src++;
		if (c == '\0')
			break;
		if (IsLocalPathDelim(c))
			last = src - 1;
	}

	return ((char *) last);
}	/* StrRFindLocalPathDelim */




void
StrRemoveTrailingLocalPathDelim(char *dst)
{
	char *cp;

	cp = StrRFindLocalPathDelim(dst);
	if ((cp == NULL) || (cp[1] != '\0'))
		return;

	/* Note: Do not destroy a path of "/" */
	while ((cp > dst) && (IsLocalPathDelim(*cp)))
		*cp-- = '\0';
}	/* StrRemoveTrailingLocalPathDelim */



void
TVFSPathToLocalPath(char *dst)
{
	int c;

	/* Note: Technically we don't need to do this,
	 * since Win32 accepts a / as equivalent to a \
	 * in a pathname.
	 */
	if (dst != NULL) {
		for (;;) {
			c = *dst++;
			if (c == '\0')
				break;
			if (c == '/')
				dst[-1] = LOCAL_PATH_DELIM;
		}
	}
}	/* TVFSPathToLocalPath */


void
LocalPathToTVFSPath(char *dst)
{
	int c;

	if (dst != NULL) {
		for (;;) {
			c = *dst++;
			if (c == '\0')
				break;
			if (c == LOCAL_PATH_DELIM)
				dst[-1] = '/';
		}
	}
}	/* LocalPathToTVFSPath */
#endif		/* WINDOWS */




void
StrRemoveTrailingSlashes(char *dst)
{
	char *cp;

	cp = strrchr(dst, '/');
	if ((cp == NULL) || (cp[1] != '\0'))
		return;

	/* Note: Do not destroy a path of "/" */
	while ((cp > dst) && (*cp == '/'))
		*cp-- = '\0';
}	/* StrRemoveTrailingSlashes */




int
MkDirs(const char *const newdir, int mode1)
{
	char s[512];
	int rc;
	char *cp, *sl;
#if defined(WIN32) || defined(_WINDOWS)
	struct _stat st;
	char *share;
#else
	struct Stat st;
	mode_t mode = (mode_t) mode1;
#endif

#if defined(WIN32) || defined(_WINDOWS)
	if ((isalpha(newdir[0])) && (newdir[1] == ':')) {
		if (! IsLocalPathDelim(newdir[2])) {
			/* Special case "c:blah", and errout.
			 * "c:\blah" must be used or _access GPFs.
			 */
			errno = EINVAL;
			return (-1);
		} else if (newdir[3] == '\0') {
			/* Special case root directory, which cannot be made. */
			return (0);
		}
	} else if (IsUNCPrefixed(newdir)) {
		share = StrFindLocalPathDelim(newdir + 2);
		if ((share == NULL) || (StrFindLocalPathDelim(share + 1) == NULL))
			return (-1);
	}

	if (_access(newdir, 00) == 0) {
		if (_stat(newdir, &st) < 0)
			return (-1);
		if (! S_ISDIR(st.st_mode)) {
			errno = ENOTDIR;
			return (-1);
		}
		return 0;
	}
#else
	if (access(newdir, F_OK) == 0) {
		if (Stat(newdir, &st) < 0)
			return (-1);
		if (! S_ISDIR(st.st_mode)) {
			errno = ENOTDIR;
			return (-1);
		}
		return 0;
	}
#endif

	(void) strncpy(s, newdir, sizeof(s));
	if (s[sizeof(s) - 1] != '\0') {
#ifdef ENAMETOOLONG
		errno = ENAMETOOLONG;
#else
		errno = EINVAL;
		return (-1);
#endif
	}

	cp = StrRFindLocalPathDelim(s);
	if (cp == NULL) {
#if defined(WIN32) || defined(_WINDOWS)
		if (! CreateDirectory(newdir, (LPSECURITY_ATTRIBUTES) 0))
			return (-1);
		return (0);
#else
		rc = mkdir(newdir, mode);
		return (rc);
#endif
	} else if (cp[1] == '\0') {
		/* Remove trailing slashes from path. */
		--cp;
		while (cp > s) {
			if (! IsLocalPathDelim(*cp))
				break;
			--cp;
		}
		cp[1] = '\0';
		cp = StrRFindLocalPathDelim(s);
		if (cp == NULL) {
#if defined(WIN32) || defined(_WINDOWS)
			if (! CreateDirectory(s, (LPSECURITY_ATTRIBUTES) 0))
				return (-1);
#else
			rc = mkdir(s, mode);
			return (rc);
#endif
		}
	}

	/* Find the deepest directory in this
	 * path that already exists.  When
	 * we do, we want to have the 's'
	 * string as it was originally, but
	 * with 'cp' pointing to the first
	 * slash in the path that starts the
	 * part that does not exist.
	 */
	sl = NULL;
	for (;;) {
		*cp = '\0';
#if defined(WIN32) || defined(_WINDOWS)
		rc = _access(s, 00);
#else
		rc = access(s, F_OK);
#endif
		if (sl != NULL)
			*sl = LOCAL_PATH_DELIM;
		if (rc == 0) {
			*cp = LOCAL_PATH_DELIM;
			break;
		}
		sl = cp;
		cp = StrRFindLocalPathDelim(s);
		if (cp == NULL) {
			/* We do not have any more
			 * slashes, so none of the
			 * new directory's components
			 * existed before, so we will
			 * have to make everything
			 * starting at the first node.
			 */
			if (sl != NULL)
				*sl = LOCAL_PATH_DELIM;

			/* We refer to cp + 1 below,
			 * so this is why we can
			 * set "cp" to point to the
			 * byte before the array starts.
			 */
			cp = s - 1;
			break;
		}
	}

	for (;;) {
		/* Extend the path we have to
		 * include the next component
		 * to make.
		 */
		sl = StrFindLocalPathDelim(cp + 1);
		if (sl == s) {
			/* If the next slash is pointing
			 * to the start of the string, then
			 * the path is an absolute path and
			 * we don't need to make the root node,
			 * and besides the next mkdir would
			 * try an empty string.
			 */
			++cp;
			sl = StrFindLocalPathDelim(cp + 1);
		}
		if (sl != NULL) {
			*sl = '\0';
		}
#if defined(WIN32) || defined(_WINDOWS)
		if (! CreateDirectory(s, (LPSECURITY_ATTRIBUTES) 0))
			return (-1);
#else
		rc = mkdir(s, mode);
		if (rc < 0)
			return rc;
#endif
		if (sl == NULL)
			break;
		*sl = LOCAL_PATH_DELIM;
		cp = sl;
	}
	return (0);
}	/* MkDirs */




int
FilenameExtensionIndicatesASCII(const char *const pathName, const char *const extnList)
{
	const char *extn;
	char *cp;
	int c;
	char extnPattern[16];

	extn = pathName + strlen(pathName) - 1;
	forever {
		if (extn <= pathName)
			return (0);	/* End of pathname, no extension. */
		c = (int) *--extn;
		if (IsLocalPathDelim(c))
			return (0);	/* End of filename, no extension. */
		if (c == '.') {
			extn += 1;
			break;
		}
	}
	if (strlen(extn) > (sizeof(extnPattern) - 2 - 1 - 1)) {
		return (0);
	}
#ifdef HAVE_SNPRINTF
	snprintf(extnPattern, sizeof(extnPattern),
#else
	sprintf(extnPattern,
#endif
		"|.%s|",
		extn
	);

	cp = extnPattern;
	forever {
		c = *cp;
		if (c == '\0')
			break;
		if (isupper(c)) {
			c = tolower(c);
			*cp++ = (char) c;
		} else {
			cp++;
		}
	}

	/* Extension list is specially formatted, like this:
	 *
	 * 	|ext1|ext2|ext3|...|extN|
	 *
	 * I.e, each filename extension is delimited with 
	 * a pipe, and we always begin and end the string
	 * with a pipe.
	 */
	if (strstr(extnList, extnPattern) != NULL) {
		return (1);
	}
	return (0);
}	/* FilenameExtensionIndicatesASCII */





#ifdef HAVE_SIGACTION
void (*NcSignal(int signum, void (*handler)(int)))(int)
{
	struct sigaction sa, osa;

	(void) sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = handler;
	if (signum == SIGALRM) {
#ifdef SA_INTERRUPT
		sa.sa_flags |= SA_INTERRUPT;
#endif
	} else {
#ifdef SA_RESTART
		sa.sa_flags |= SA_RESTART;
#endif
	}
	if (sigaction(signum, &sa, &osa) < 0)
		return ((FTPSigProc) SIG_ERR);
	return (osa.sa_handler);
}
#endif	/* HAVE_SIGACTION */

/* eof */
