/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
/*
 * Some stuff taken from active perl: perl\win32.c (ioinfo stuff)
 *
 * (c) 1995 Microsoft Corporation. All rights reserved.
 *       Developed by hip communications inc., http://info.hip.com/info/
 * Portions (c) 1993 Intergraph Corporation. All rights reserved.
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 */

#ifndef __CRT_INTERNAL_FILE_H
#define __CRT_INTERNAL_FILE_H

#ifndef _CRT_PRECOMP_H
#error DO NOT INCLUDE THIS HEADER DIRECTLY
#endif

#include <stdarg.h>
#include <time.h>

#ifndef _IORMONCL
#define _IORMONCL 004000  /* remove on close, for temp files */
#endif
/* if _flag & _IORMONCL, ._name_to_remove needs freeing */

#ifndef _IOUNGETC
#define _IOUNGETC 010000  /* there is an ungetc'ed character in the buffer */
#endif

/* might need check for IO_APPEND aswell */
#define OPEN4WRITING(f) ((((f)->_flag & _IOWRT)  == _IOWRT))
#define OPEN4READING(f) ((((f)->_flag & _IOREAD) == _IOREAD))

/* might need check for IO_APPEND aswell */
#define WRITE_STREAM(f) ((((f)->_flag & _IOWRT) == _IOWRT))
#define READ_STREAM(f) ((((f)->_flag & _IOREAD) == _IOREAD))

int __set_errno(int err);
int __set_doserrno(int error);
void* filehnd(int fn);
char __is_text_file(FILE*);
int _doprnt(const char* fmt, va_list args, FILE *);
int _doscan(FILE* iop, const char* fmt, va_list argp);
int __fileno_dup2(int handle1, int handle2);
char __fileno_getmode(int _fd);
int __fileno_setmode(int _fd, int _newmode);
void sigabort_handler(int sig);

//void UnixTimeToFileTime(time_t unix_time, FILETIME* filetime, DWORD remainder);
//time_t FileTimeToUnixTime(const FILETIME* filetime, DWORD *remainder);


#define __FILE_REC_MAX 20
typedef struct __file_rec
{
    struct __file_rec* next;
    int count;
    FILE* files[__FILE_REC_MAX];
} __file_rec;

extern __file_rec* __file_rec_list;


typedef struct _FDINFO
{
    HANDLE hFile;
    char fdflags;
    char pipechar;    /* one char buffer for handles opened on pipes */
    int lockinitflag;
    CRITICAL_SECTION lock;
} FDINFO;

#define FDINFO_ENTRIES_PER_BUCKET_SHIFT 5  /* log2(32) = 5 */
#define FDINFO_BUCKETS 64
#define FDINFO_ENTRIES_PER_BUCKET 32
#define FDINFO_ENTRIES (FDINFO_BUCKETS * FDINFO_ENTRIES_PER_BUCKET)

/* pipech */
#define LF        10 /* line feed */
#define CR        13 /* carriage return */
#define CTRLZ     26      /* ctrl-z means eof for text */

/* mode */
#define FOPEN        0x01  /* file handle open */
#define FEOFLAG      0x02  /* end of file has been encountered */
#define FCRLF        0x04  /* CR-LF across read buffer (in text mode) */
#define FPIPE        0x08  /* file refers to a pipe */
#define FNOINHERIT   0x10  /* file handle opened _O_NOINHERIT */
#define FAPPEND      0x20  /* file opened O_APPEND */
#define FDEV         0x40  /* file refers to device */
#define FTEXT        0x80  /* file is in text mode (absence = binary) */

/* get bucket index (0-63) from an fd */
#define fdinfo_bucket_idx(i) ((i) >> FDINFO_ENTRIES_PER_BUCKET_SHIFT)
/* get position inside a bucket (0-31) from an fd */
#define fdinfo_bucket_entry_idx(i) ((i) & (FDINFO_ENTRIES_PER_BUCKET - 1))
/* get bucket ptr. (ptr. to first fdinfo inside a bucket) from an fd */
#define fdinfo_bucket(i) ( __pioinfo[fdinfo_bucket_idx(i)])
/* get fdinfo ptr. from an fd */
#define fdinfo(i) (fdinfo_bucket(i) + fdinfo_bucket_entry_idx(i))

//extern FDINFO* __pioinfo[];

void _dosmaperr(unsigned long oserrcode);






int access_dirA(const char *_path);
int access_dirW(const wchar_t *_path);

#ifdef _UNICODE
   #define access_dirT access_dirW
#else
   #define access_dirT access_dirA
#endif





#undef MB_CUR_MAX
#define MB_CUR_MAX __mb_cur_max


//int _isnanl(long double x);
//int _isinfl(long double x);
//int _isnan(double x);
//int _isinf(double x);

/* Flags for the iobuf structure (for reference) */
#if 0
#define  _IOREAD  1 /* currently reading */
#define  _IOWRT   2 /* currently writing */
#define  _IORW 0x0080 /* opened as "r+w" */
#endif

#ifndef F_OK
#define	F_OK	0	/* Check for file existence */
#endif
#ifndef W_OK
#define	W_OK	2	/* Check for write permission */
#endif

/* internal FILE->_flag flags */

#define _IOMYBUF  0x0008  /* stdio malloc()'d buffer */
#define _IOEOF    0x0010  /* EOF reached on read */
#define _IOERR    0x0020  /* I/O error from system */
#define _IOSTRG   0x0040  /* Strange or no file descriptor */
#define _IOBINARY 0x040000
#define _IOTEXT   0x000000
#define _IOCOMMIT 0x100000
#define _IODIRTY  0x010000
#define _IOAHEAD  0x020000

/*
 * The three possible buffering mode (nMode) values for setvbuf.
 * NOTE: _IOFBF works, but _IOLBF seems to work like unbuffered...
 * maybe I'm testing it wrong?
 */
#define _IOFBF    0x0000     /* full buffered */
#define _IOLBF    0x0040     /* line buffered */
#define _IONBF    0x0004     /* not buffered */
#define _IO_LBF   0x80000    /* this value is used insteat of _IOLBF within the
                                structure FILE as value for _flags,
                                because _IOLBF has the same value as _IOSTRG */

wint_t _filwbuf(FILE *f);

//#if __MINGW32_MAJOR_VERSION < 3 || __MINGW32_MINOR_VERSION < 2
//   int __cdecl _filbuf (FILE*);
//   int __cdecl _flsbuf (int, FILE*);
//#endif

#endif /* __dj_include_libc_file_h__ */
