/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#ifndef __dj_include_libc_file_h__
#define __dj_include_libc_file_h__

#include <msvcrt/stdio.h>
#include <msvcrt/fcntl.h>


#ifdef __cplusplus
extern "C" {
#endif

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


#ifdef __cplusplus
}
#endif


#endif /* __dj_include_libc_file_h__ */



