/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#ifndef __MSVCRT_INTERNAL_H
#define __MSVCRT_INTERNAL_H

#define ____MINGW_IMPORT
#define __MINGW_IMPORT extern

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utime.h>
#include <malloc.h>
#include <io.h>
#include <errno.h>
#include <share.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <tchar.h>
#include <mbctype.h>
#include <mbstring.h>
#include <conio.h>
#include <float.h>
#include <math.h>
#include <direct.h>
#include <fcntl.h>
#include <locale.h>
#include <process.h>
#include <time.h>
#include <excpt.h>
#include <ntdef.h>
#include <windows.h>


#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE

struct __atexit {
  struct __atexit *__next;
  void (*__function)(void);
};

extern struct __atexit *__atexit_ptr;

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */


#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE

#ifndef _IORMONCL
#define _IORMONCL 004000  /* remove on close, for temp files */
#endif
/* if _flag & _IORMONCL, ._name_to_remove needs freeing */

#ifndef _IOUNGETC
#define _IOUNGETC 010000  /* there is an ungetc'ed character in the buffer */
#endif

#define _IOAHEAD  0x000008
#define _IODIRTY  0x000080

#define stdaux	(&_iob[3])
#define stdprn	(&_iob[4])

// might need check for IO_APPEND aswell
#define OPEN4WRITING(f) ((((f)->_flag & _IOWRT) == _IOWRT ) )

#define OPEN4READING(f) ((((f)->_flag & _IOREAD) == _IOREAD ) )

// might need check for IO_APPEND aswell
#define WRITE_STREAM(f) ((((f)->_flag & _IOWRT) == _IOWRT ) )

#define READ_STREAM(f) ((((f)->_flag & _IOREAD) == _IOREAD ) )


char __validfp (FILE *f);

int __set_errno(int err);
int __set_doserrno (int error);

void *filehnd(int fn);

char __is_text_file(FILE *p);

int __fileno_alloc(void *hFile, int mode);

int _doprnt(const char *fmt, va_list args, FILE *f);
int _doscan(FILE *iop, const char *fmt, va_list argp);


int __fileno_dup2( int handle1, int handle2 );
int __fileno_getmode(int _fd);
int __fileno_setmode(int _fd, int _newmode);
int __fileno_close(int _fd);

void sigabort_handler(int sig);

#include <windows.h>

void UnixTimeToFileTime( time_t unix_time, FILETIME *filetime, DWORD remainder );
time_t FileTimeToUnixTime( const FILETIME *filetime, DWORD *remainder );


#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */


#define __FILE_REC_MAX 20
typedef struct __file_rec 
{
   struct __file_rec *next;
   int count;
   FILE *files[__FILE_REC_MAX];
} __file_rec;

extern __file_rec *__file_rec_list;


typedef struct {
  unsigned int mantissa:23;
  unsigned int exponent:8;
  unsigned int sign:1;
} float_t;

typedef struct {
  unsigned int mantissal:32;
  unsigned int mantissah:20;
  unsigned int exponent:11;
  unsigned int sign:1;
} double_t;

typedef struct {
  unsigned int mantissal:32;
  unsigned int mantissah:32;
  unsigned int exponent:15;
  unsigned int sign:1;
  unsigned int empty:16;
} long_double_t;


#define _RT_STACK	0	/* stack overflow */
#define _RT_NULLPTR	1	/* null pointer assignment */
#define _RT_FLOAT	2	/* floating point not loaded */
#define _RT_INTDIV	3	/* integer divide by 0 */
#define _RT_SPACEARG	4	/* not enough space for arguments */
#define _RT_SPACEENV	5	/* not enough space for environment */
#define _RT_ABORT	6	/* abnormal program termination */
#define _RT_THREAD	7	/* not enough space for thread data */
#define _RT_LOCK	8	/* unexpected multi-thread lock error */
#define _RT_HEAP	9	/* unexpected heap error */
#define _RT_OPENCON	10	/* unable to open console device */
#define _RT_NONCONT	11	/* non-continuable exception */
#define _RT_INVALDISP	12	/* invalid disposition of exception */
#define _RT_ONEXIT	13	/* insufficient heap to allocate
				 * initial table of function pointers
				 * used by _onexit()/atexit(). */
#define _RT_PUREVIRT	14	/* pure virtual function call attempted
				 * (C++ error) */
#define _RT_STDIOINIT	15	/* not enough space for stdio initialization */
#define _RT_LOWIOINIT	16	/* not enough space for lowio initialization */

void _amsg_exit (int errnum);

#ifdef __cplusplus
}
#endif


int __vfscanf (FILE *s, const char *format, va_list argptr);
int __vscanf (const char *format, va_list arg);
int __vsscanf (const char *s,const char *format,va_list arg);


extern int char_avail;
extern int ungot_char;


typedef struct _ThreadData
{
  int terrno;					/* *nix error code */
  unsigned long tdoserrno;			/* Win32 error code (for I/O only) */
  unsigned long long tnext;			/* used by rand/srand */

  char *lasttoken;				/* used by strtok */
  wchar_t *wlasttoken;				/* used by wcstok */


  int fpecode;					/* fp exception code */

  /* qsort variables */
  int (*qcmp)(const void *, const void *);	/* the comparison routine */
  int qsz;					/* size of each record */
  int thresh;					/* THRESHold in chars */
  int mthresh;					/* MTHRESHold in chars */

} THREADDATA, *PTHREADDATA;


int CreateThreadData(void);
void DestroyThreadData(void);

void FreeThreadData(PTHREADDATA ptd);
PTHREADDATA GetThreadData(void);

#define _KNJ_M  ((char)0x01)    /* Non-punctuation of Kana-set */
#define _KNJ_P  ((char)0x02)    /* Punctuation of Kana-set */
#define _KNJ_1  ((char)0x04)    /* Legal 1st byte of double byte stream */
#define _KNJ_2  ((char)0x08)    /* Legal 2nd btye of double byte stream */


#define ___     0
#define _1_     _KNJ_1 /* Legal 1st byte of double byte code */
#define __2     _KNJ_2 /* Legal 2nd byte of double byte code */
#define _M_     _KNJ_M /* Non-puntuation in Kana-set */
#define _P_     _KNJ_P /* Punctuation of Kana-set */
#define _12     (_1_|__2)
#define _M2     (_M_|__2)
#define _P2     (_P_|__2)

#define _jctype _mbctype


#define	SIGINT		2	/* Interactive attention */
#define	SIGILL		4	/* Illegal instruction */
#define	SIGFPE		8	/* Floating point error */
#define	SIGSEGV		11	/* Segmentation violation */
#define	SIGTERM		15	/* Termination request */
#define SIGBREAK	21	/* Control-break */
#define	SIGABRT		22	/* Abnormal termination (abort) */

#define SIGALRM	293
#define SIGHUP	294
/* SIGINT is ansi */
#define SIGKILL	296
#define SIGPIPE	297
#define SIGQUIT	298
#define SIGUSR1	299
#define SIGUSR2	300

#define SIGNOFP 301
#define SIGTRAP 302
#define SIGTIMR 303	/* Internal for setitimer (SIGALRM, SIGPROF) */
#define SIGPROF 304
#define SIGMAX  320


/* FIXME: Should be in MinGW runtime */
void _fwalk(void (*func)(FILE *));

size_t _mbstrlen( const char *string );

#if 1
// Put in MinGW runtime?

#define _O_NOINHERIT 0x0080
#define O_NOINHERIT _O_NOINHERIT

#endif

//typedef int (* _onexit_t)(void);
_onexit_t onexit(_onexit_t);

#endif /* __MSVCRT_INTERNAL_H */
