/*
 * stat.h
 *
 * Symbolic constants for opening and creating files, also stat, fstat and
 * chmod functions.
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
 * $Revision: 1.7 $
 * $Author: gvg $
 * $Date: 2003/04/27 23:14:04 $
 *
 */

#ifndef __STRICT_ANSI__

#ifndef _STAT_H_
#define _STAT_H_

#include <msvcrt/sys/types.h>


#ifndef _WCHAR_T_
#define _WCHAR_T_
#define _WCHAR_T
#define _WCHAR_T_DEFINED
#ifndef __WCHAR_TYPE__
#define __WCHAR_TYPE__      short unsigned int
#endif
#ifndef __cplusplus
typedef __WCHAR_TYPE__      wchar_t;
#endif  /* C++ */
#endif  /* wchar_t not already defined */


/*
 * Constants for the stat st_mode member.
 */
#define S_IFIFO     0x1000  /* FIFO */
#define S_IFCHR     0x2000  /* Character */
#define S_IFBLK     0x3000  /* Block */
#define S_IFDIR     0x4000  /* Directory */
#define S_IFREG     0x8000  /* Regular */

#define S_IFMT      0xF000  /* File type mask */

#define S_IEXEC     0x0040
#define S_IWRITE    0x0080
#define S_IREAD     0x0100

#define S_ISDIR(m)  ((m) & S_IFDIR)
#define S_ISFIFO(m) ((m) & S_IFIFO)
#define S_ISCHR(m)  ((m) & S_IFCHR)
#define S_ISBLK(m)  ((m) & S_IFBLK)
#define S_ISREG(m)  ((m) & S_IFREG)

#define S_IRWXU     (S_IREAD | S_IWRITE | S_IEXEC)
#define S_IXUSR     S_IEXEC
#define S_IWUSR     S_IWRITE
#define S_IRUSR     S_IREAD

#define _S_IEXEC    S_IEXEC
#define _S_IREAD    S_IREAD
#define _S_IWRITE   S_IWRITE

/*
 * The structure manipulated and returned by stat and fstat.
 *
 * NOTE: If called on a directory the values in the time fields are not only
 * invalid, they will cause localtime et. al. to return NULL. And calling
 * asctime with a NULL pointer causes an Invalid Page Fault. So watch it!
 */
struct stat
{
    unsigned st_dev;     /* Equivalent to drive number 0=A 1=B ... */
    short    st_ino;     /* Always zero ? */
    short    st_mode;    /* See above constants */
    short    st_nlink;   /* Number of links. */
    short    st_uid;     /* User: Maybe significant on NT ? */
    short    st_gid;     /* Group: Ditto */
    unsigned st_rdev;    /* Seems useless (not even filled in) */
    long     st_size;    /* File size in bytes */
    time_t   st_atime;   /* Accessed date (always 00:00 hrs local on FAT) */
    time_t   st_mtime;   /* Modified time */
    time_t   st_ctime;   /* Creation time */
};


struct _stati64
{
    unsigned st_dev;     /* Equivalent to drive number 0=A 1=B ... */
    short    st_ino;     /* Always zero ? */
    short    st_mode;    /* See above constants */
    short    st_nlink;   /* Number of links. */
    short    st_uid;     /* User: Maybe significant on NT ? */
    short    st_gid;     /* Group: Ditto */
    unsigned st_rdev;    /* Seems useless (not even filled in) */
    __int64 st_size;    /* File size in bytes */
    time_t  st_atime;   /* Accessed date (always 00:00 hrs local on FAT) */
    time_t  st_mtime;   /* Modified time */
    time_t  st_ctime;   /* Creation time */
};

#ifdef  __cplusplus
extern "C" {
#endif

int _fstat(int, struct stat*);
int _stat(const char*, struct stat*);

__int64 _fstati64(int nFileNo, struct _stati64* pstat);
__int64 _stati64(const char* szPath, struct _stati64* pstat);
int _wstat(const wchar_t* szPath, struct stat* pstat);
__int64 _wstati64(const wchar_t* szPath, struct _stati64* pstat);


#ifndef _NO_OLDNAMES

#define fstat(nFileNo, pstat)   _fstat(nFileNo, pstat)
#define stat(szPath,pstat)  _stat(szPath,pstat)

#endif  /* Not _NO_OLDNAMES */


#ifdef  __cplusplus
}
#endif

#endif  /* Not _STAT_H_ */

#endif  /* Not __STRICT_ANSI__ */
