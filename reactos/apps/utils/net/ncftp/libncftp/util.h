/* Util.h
 *
 * Copyright (c) 1996-2001 Mike Gleason, NCEMRSoft.
 * All rights reserved.
 *
 */

#ifndef _util_h_
#define _util_h_ 1

typedef char string[160], str16[16], str32[32], str64[64];
typedef char longstring[512];
typedef char pathname[512];

#ifndef PTRZERO
#	define PTRZERO(p,siz)  (void) memset(p, 0, (size_t) (siz))
#endif

#define ZERO(a)	PTRZERO(&(a), sizeof(a))
#define STREQ(a,b) (strcmp(a,b) == 0)
#define STRNEQ(a,b,s) (strncmp(a,b,(size_t)(s)) == 0)

#ifndef ISTRCMP
#	ifdef HAVE_STRCASECMP
#		define ISTRCMP strcasecmp
#		define ISTRNCMP strncasecmp
#	else
#		define ISTRCMP strcmp
#		define ISTRNCMP strncmp
#	endif
#endif

#define ISTREQ(a,b) (ISTRCMP(a,b) == 0)
#define ISTRNEQ(a,b,s) (ISTRNCMP(a,b,(size_t)(s)) == 0)

typedef int (*cmp_t)(const void *, const void *);
#define QSORT(base,n,s,cmp) \
	qsort(base, (size_t)(n), (size_t)(s), (cmp_t)(cmp))

#define BSEARCH(key,base,n,s,cmp) \
	bsearch(key, base, (size_t)(n), (size_t)(s), (cmp_t)(cmp))

/* For Error(): */
#define kDoPerror		1
#define kDontPerror		0

#define kClosedFileDescriptor (-1)

#define SZ(a) ((size_t) (a))

#ifndef F_OK
#	define F_OK 0
#endif

#ifdef HAVE_REMOVE
#	define UNLINK remove
#else
#	define UNLINK unlink
#endif

#ifndef SEEK_SET
#	define SEEK_SET    0
#	define SEEK_CUR    1
#	define SEEK_END    2
#endif  /* SEEK_SET */

#ifdef SETVBUF_REVERSED
#	define SETVBUF(a,b,c,d) setvbuf(a,c,b,d)
#else
#	define SETVBUF setvbuf
#endif


/* Util.c */
char *FGets(char *, size_t, FILE *);
struct passwd *GetPwByName(void);
void GetHomeDir(char *, size_t);
void GetUsrName(char *, size_t);
void CloseFile(FILE **);
void PrintF(const FTPCIPtr cip, const char *const fmt, ...)
#if (defined(__GNUC__)) && (__GNUC__ >= 2)
__attribute__ ((format (printf, 2, 3)))
#endif
;
void Error(const FTPCIPtr cip, const int pError, const char *const fmt, ...)
#if (defined(__GNUC__)) && (__GNUC__ >= 2)
__attribute__ ((format (printf, 3, 4)))
#endif
;
int GetSockBufSize(int sockfd, size_t *rsize, size_t *ssize);
int SetSockBufSize(int sockfd, size_t rsize, size_t ssize);
time_t UnMDTMDate(char *);
void Scramble(unsigned char *dst, size_t dsize, unsigned char *src, char *key);
#if defined(WIN32) || defined(_WINDOWS)
char *StrFindLocalPathDelim(const char *src);
char *StrRFindLocalPathDelim(const char *src);
void TVFSPathToLocalPath(char *dst);
void LocalPathToTVFSPath(char *dst);
int gettimeofday(struct timeval *const tp, void *junk);
#endif

#ifdef HAVE_SIGACTION
void (*NcSignal(int signum, void (*handler)(int)))(int);
#elif !defined(NcSignal)
#	define NcSignal signal
#endif

#endif	/* _util_h_ */
