/****************************************************************************
 CONFIG.H -   Midnight Commander Configuration for Win32 and OS/2


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  

 ----------------------------------------------------------------------------
 Changes:
        - Created 951204/jfg
	- Changed from Alexander Dong (ado) for OS/2
	- Changed 980329 by Pavel Roskin for both OS/2 and NT

 ----------------------------------------------------------------------------
 Contents:
        - Headers flags
        - Library flags
        - Typedefs
        - etc.
 ****************************************************************************/
#ifndef __CONFIG_H
#define __CONFIG_H

#define OS2_NT

#ifdef MC_NT
#  ifndef WIN32
#    define WIN32
#  endif
#  ifndef __WIN32__
#    define __WIN32__
#  endif
#  ifndef MSWINDOWS
#    define MSWINDOWS
#  endif
#  ifndef _OS_NT
#    define _OS_NT
#  endif
#endif /* MC_NT */

#ifdef MC_OS2
#  ifndef OS2
#    define OS2
#  endif
#  ifndef __os2__
#    define __os2__
#  endif
#endif /* MC_OS2 */

#include "../VERSION"

#ifndef pc_system
#   define pc_system
#endif

#ifndef HAVE_SLANG
#   define HAVE_SLANG
#endif

#ifndef _CONSOLE
#   define _CONSOLE
#endif

#define FLOAT_TYPE
#define MIDNIGHT
#define USE_INTERNAL_EDIT

#define STDC_HEADERS
#define HAVE_STDLIB_H
#define HAVE_STRING_H
#define HAVE_DIRENT_H
#define HAVE_LIMITS_H
#define HAVE_FCNTL_H
#define HAVE_UTIME_H

#define HAVE_MEMSET
#define HAVE_MEMCHR
#define HAVE_MEMCPY
#define HAVE_MEMCMP
#define HAVE_MEMMOVE
#define HAVE_STRDUP
#define HAVE_STRERROR
#define HAVE_TRUNCATE

#define REGEX_MALLOC
#define NO_INFOMOUNT

typedef unsigned int umode_t;
#define S_IFLNK 0
#define S_ISLNK(x) 0

#ifdef __EMX__

#define S_IFBLK 0
#define S_ISBLK(x) 0

#endif /* __EMX__ */

#ifdef __MINGW32__

#define S_IRGRP         0000040
#define S_IWGRP         0000020
#define S_IXGRP         0000010
#define S_IROTH         0000004
#define S_IWOTH         0000002
#define S_IXOTH         0000001

#define ENABLE_NLS

#define pipe(p)  _pipe(p, 4096, 0x8000 /* O_BINARY */)

/*typedef int mode_t;*/
typedef unsigned int nlink_t;
typedef int gid_t;
typedef int uid_t;
/*typedef int pid_t;*/

#endif /* __MINGW32__ */

#ifdef _MSC_VER

#pragma include_alias(<utime.h>, <sys/utime.h>)

#define INLINE
#define inline

#define S_ISCHR(m)    (((m) & S_IFMT) == S_IFCHR)
#define S_ISDIR(m)    (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m)    (((m) & S_IFMT) == S_IFREG)

#define S_ISFIFO(m)   0
#define S_ISBLK(x)    0

#define S_IRWXU         0000700
#define S_IRUSR         0000400
#define S_IWUSR         0000200
#define S_IXUSR         0000100

#define S_IRWXG         0000070
#define S_IRGRP         0000040
#define S_IWGRP         0000020
#define S_IXGRP         0000010
#define S_IRWXO         0000007
#define S_IROTH         0000004
#define S_IWOTH         0000002
#define S_IXOTH         0000001

/* FIXME: is this definition correct? */
#define R_OK    4

#define pipe(p)  _pipe(p, 4096, 0x8000 /* O_BINARY */)
#define popen   _popen
#define pclose  _pclose

typedef int mode_t;
typedef unsigned int nlink_t;
typedef int gid_t;
typedef int uid_t;
typedef int pid_t;

#endif /* _MSC_VER */

#ifdef __BORLANDC__

#define INLINE
#define inline

#define S_IRWXG         0000070
#define S_IRGRP         0000040
#define S_IWGRP         0000020
#define S_IXGRP         0000010
#define S_IRWXO         0000007
#define S_IROTH         0000004
#define S_IWOTH         0000002
#define S_IXOTH         0000001

/* FIXME: is this definition correct? */
#define R_OK    4

#define pipe(p)  _pipe(p, 4096, 0x8000 /* O_BINARY */)
#define popen   _popen
#define pclose  _pclose
#define sleep   _sleep

typedef int pid_t;

#endif /* __BORLANDC__ */

#ifdef __IBMC__

#define INLINE
#define inline

#define S_ISFIFO(m)   0
#define S_ISBLK(x)    0

#define S_ISCHR(m)    (((m) & S_IFCHR) != 0)
#define S_ISDIR(m)    (((m) & S_IFDIR) != 0)
#define S_ISREG(m)    (((m) & S_IFREG) != 0)

#define S_IRWXU         0000700
#define S_IRUSR         0000400
#define S_IWUSR         0000200
#define S_IXUSR         0000100

#define S_IRWXG         0000070
#define S_IRGRP         0000040
#define S_IWGRP         0000020
#define S_IXGRP         0000010
#define S_IRWXO         0000007
#define S_IROTH         0000004
#define S_IWOTH         0000002
#define S_IXOTH         0000001

#define ENOTDIR		ENOENT

/* FIXME: is this definition correct? */
#define R_OK    4

#pragma map( chdir , "_chdir"  )
#pragma map( getcwd, "_getcwd" )
#pragma map( mkdir , "_mkdir"  )
#pragma map( rmdir , "_rmdir"  )

#define popen   DosCreatePipe
#define pclose  DosClose
#define sleep   DosSleep

typedef unsigned int nlink_t;
typedef int mode_t;
typedef int gid_t;
typedef int uid_t;
typedef int pid_t;

#endif /* __IBMC__ */

#endif /* __CONFIG_H */
