/*
 * Copyright (C) 2004, 2006-2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: config.h.win32,v 1.19 2008/09/25 07:44:12 marka Exp $ */

/*
 * win32 configuration file
 * All definitions, declarations, macros and includes are
 * specific to the requirements of the Windows NT and Windows 2000
 * platforms
 */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define as __inline if that's what the C compiler calls it.  */
#define inline __inline

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/*
 * ANSI C compliance enabled
 */
#ifndef __REACTOS__
#define __STDC__ 1
#endif

/*
 * Silence compiler warnings about using strcpy and friends.
 */
#ifndef __REACTOS__
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

/*
 * Use 32 bit time.
 */
#define _USE_32BIT_TIME_T 1

/*
 * Windows NT and 2K only
 */
#ifndef __REACTOS__
#define _WIN32_WINNT 0x0400
#endif
/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* define on DEC OSF to enable 4.4BSD style sa_len support */
/* #undef _SOCKADDR_LEN */

/* define if your system needs pthread_init() before using pthreads */
/* #undef NEED_PTHREAD_INIT */

/* define if your system has sigwait() */
/* #undef HAVE_SIGWAIT */

/* define on Solaris to get sigwait() to work using pthreads semantics */
/* #undef _POSIX_PTHREAD_SEMANTICS */

/* define if LinuxThreads is in use */
/* #undef HAVE_LINUXTHREADS */

/* define if catgets() is available */
/* #undef HAVE_CATGETS */

/* define if you have the NET_RT_IFLIST sysctl variable. */
#define HAVE_IFLIST_SYSCTL 1

/* define if you need to #define _XPG4_2 before including sys/socket.h */
/* #undef NEED_XPG4_2_BEFORE_SOCKET_H */

/* define if you need to #define _XOPEN_SOURCE_ENTENDED before including
 * sys/socket.h
 */
/* #undef NEED_XSE_BEFORE_SOCKET_H */

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <sys/sockio.h> header file.  */
#define HAVE_SYS_SOCKIO_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the c_r library (-lc_r).  */
/* #undef HAVE_LIBC_R */

/* Define if you have the nsl library (-lnsl).  */
/* #undef HAVE_LIBNSL */

/* Define if you have the pthread library (-lpthread).  */
/* #undef HAVE_LIBPTHREAD */

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

/* Define if you have h_errno */
#define HAVE_H_ERRNO

/* Define if you have RSA_generate_key(). */
#define HAVE_RSA_GENERATE_KEY

/* Define if you have DSA_generate_parameters(). */
#define HAVE_DSA_GENERATE_PARAMETERS

/* Define if you have DH_generate_parameters(). */
#define HAVE_DH_GENERATE_PARAMETERS

#define WANT_IPV6

#define S_IFMT   _S_IFMT         /* file type mask */
#define S_IFDIR  _S_IFDIR        /* directory */
#define S_IFCHR  _S_IFCHR        /* character special */
#define S_IFIFO  _S_IFIFO        /* pipe */
#define S_IFREG  _S_IFREG        /* regular */
#define S_IREAD  _S_IREAD        /* read permission, owner */
#define S_IWRITE _S_IWRITE       /* write permission, owner */
#define S_IEXEC  _S_IEXEC        /* execute/search permission, owner */

#define O_RDONLY        _O_RDONLY
#define O_WRONLY        _O_WRONLY
#define O_RDWR          _O_RDWR
#define O_APPEND        _O_APPEND
#define O_CREAT         _O_CREAT
#define O_TRUNC         _O_TRUNC
#define O_EXCL          _O_EXCL

/* open() under unix allows setting of read/write permissions
 * at the owner, group and other levels.  These don't exist in NT
 * We'll just map them all to the NT equivalent
 */

#ifndef __REACTOS__
#define S_IRUSR _S_IREAD	/* Owner read permission */
#define S_IWUSR _S_IWRITE	/* Owner write permission */
#endif

#define S_IRGRP _S_IREAD	/* Group read permission */
#define S_IWGRP _S_IWRITE	/* Group write permission */
#define S_IROTH _S_IREAD	/* Other read permission */
#define S_IWOTH _S_IWRITE	/* Other write permission */


/*
 * WIN32 specials until some other way of dealing with these is decided.
 */

#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
#define strdup _strdup

#ifndef __REACTOS__
#define sopen _sopen
#endif

#define isascii __isascii

#ifndef __REACTOS__
#define stat _stat
#define fstat _fstat
#define fileno _fileno
#endif

#define unlink _unlink
#define chdir _chdir
#define mkdir _mkdir

#ifndef __REACTOS__
#define getcwd _getcwd
#endif

#define utime _utime
#define utimbuf _utimbuf

/* #define EAFNOSUPPORT EINVAL */
#ifndef __REACTOS__
#define chmod _chmod
#endif

#define getpid _getpid
#define getppid _getpid	/* WARNING!!! For now this gets the same pid */
#define random rand	/* Random number generator */
#define srandom srand	/* Random number generator seeding */
/* for the config file */
typedef unsigned int    uid_t;          /* user id */
typedef unsigned int    gid_t;          /* group id */
typedef long pid_t;			/* PID */
typedef int ssize_t;

#ifndef __REACTOS__
typedef long off_t;
#endif

/*
 * Set up the Version Information
 */
#include <versions.h>

/* We actually are using the CryptAPI and not a device */
#define PATH_RANDOMDEV		"CryptAPI"

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

/*
 * Applications may need to get the configuration path
 */
#ifndef _USRDLL
#include <isc/ntpaths.h>
#endif

#ifndef __REACTOS__
#define fdopen	_fdopen
#define read	_read
#define open	_open
#define close	_close
#define write	_write
#endif
#include <io.h>
#define isatty	_isatty

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#endif

/*
 * Make the number of available sockets large
 * The number of sockets needed can get large and memory's cheap
 * This must be defined before winsock2.h gets included as the
 * macro is used there.
 */

#define FD_SETSIZE 16384
#include <windows.h>

/*
 * Windows doesn't use configure so just set "default" here.
 */
#define CONFIGARGS "default"
