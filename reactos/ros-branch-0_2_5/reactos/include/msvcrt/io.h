/* 
 * io.h
 *
 * System level I/O functions and types.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.5 $
 * $Author: robd $
 * $Date: 2002/11/24 18:06:00 $
 *
 */
/* Appropriated for Reactos Crtdll by Ariadne */
/* added D_OK */
/* changed get_osfhandle and open_osfhandle */
/* added fileno as macro */

#ifndef __STRICT_ANSI__

#ifndef _IO_H_
#define _IO_H_

#include <msvcrt/sys/types.h>

#include <msvcrt/sys/stat.h>


/* We need the definition of FILE anyway... */
#include <msvcrt/stdio.h>

/* MSVC's io.h contains the stuff from dir.h, so I will too.
 * NOTE: This also defines off_t, the file offset type, through
 * and inclusion of sys/types.h */
#include <msvcrt/dir.h>

/* TODO: Maximum number of open handles has not been tested, I just set
 * it the same as FOPEN_MAX. */
#define HANDLE_MAX      FOPEN_MAX


/* Some defines for _access nAccessMode (MS doesn't define them, but
 * it doesn't seem to hurt to add them). */
#define F_OK    0       /* Check for file existence */
#define W_OK    2       /* Check for write permission */
#define R_OK    4       /* Check for read permission */
/* TODO: Is this safe? X_OK not supported directly... */
#define X_OK    R_OK    /* Check for execute permission */
#define D_OK    0x10



#ifdef  __cplusplus
extern "C" {
#endif

int             _access (const char*, int);
int             _chsize (int, long);
int             _close (int);
int             _commit(int);

/* NOTE: The only significant bit in unPermissions appears to be bit 7 (0x80),
 *       the "owner write permission" bit (on FAT). */
int             _creat (const char*, int);
int             _dup (int);
int             _dup2 (int, int);
long            _filelength (int);
int             _fileno (FILE*);
void*           _get_osfhandle (int);
int             _isatty (int);

int             _chmod (const char* szPath, int nMode);
__int64         _filelengthi64(int nHandle);

/* In a very odd turn of events this function is excluded from those
 * files which define _STREAM_COMPAT. This is required in order to
 * build GNU libio because of a conflict with _eof in streambuf.h
 * line 107. Actually I might just be able to change the name of
 * the enum member in streambuf.h... we'll see. TODO */
#ifndef _STREAM_COMPAT
int             _eof (int);
#endif

/* LK_... locking commands defined in sys/locking.h. */
int             _locking (int, int, long);

off_t           _lseek(int, off_t, int);

/* Optional third argument is unsigned unPermissions. */
int             _open (const char*, int, ...);

int             _open_osfhandle (void*, int);
int             _pipe (int*, unsigned int, int);
size_t          _read(int, void*, size_t);

/* SH_... flags for nShFlags defined in share.h
 * Optional fourth argument is unsigned unPermissions */
int             _sopen (char*, int, int, int);

long            _tell(int);
/* Should umask be in sys/stat.h and/or sys/types.h instead? */
unsigned        _umask(unsigned);
int             _unlink(const char*);
size_t          _write(int, const void*, size_t);

__int64         _lseeki64(int _fildes, __int64 _offset, int _whence);
__int64         _telli64(int nHandle);

/* Wide character versions. Also declared in wchar.h. */
/* Not in crtdll.dll */
int             _waccess(const wchar_t*, int);
int             _wchmod(const wchar_t*, int);
int             _wcreat(const wchar_t*, int);

int             _wunlink(const wchar_t*);
int             _wopen(const wchar_t*, int, ...);
int             _wsopen(wchar_t*, int, int, int);


#ifndef _NO_OLDNAMES
/*
 * Non-underscored versions of non-ANSI functions to improve portability.
 * These functions live in libmoldname.a.
 */

#define access          _access
#define chmod           _chmod
#define chsize          _chsize
#define close           _close
#define creat           _creat
#define dup             _dup
#define dup2            _dup2
#define eof             _eof
#define filelength      _filelength
#define fileno(f)       ((f)->_file)
#define isatty          _isatty
#define lseek           _lseek
#define open            _open
#define read            _read
#define sopen(path,access,shflag,mode)  _open((path), (access)|(shflag), (mode))
#define tell(file)                      _lseek(_file, 0, SEEK_CUR)
#define umask           _umask
#define unlink          _unlink
#define write           _write


#endif  /* Not _NO_OLDNAMES */

#ifdef  __cplusplus
}
#endif

#endif  /* _IO_H_ not defined */

#endif  /* Not strict ANSI */

