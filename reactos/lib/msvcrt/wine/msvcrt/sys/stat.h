/*
 * _stat() definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_SYS_STAT_H
#define __WINE_SYS_STAT_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#include <sys/types.h>

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif

#ifndef MSVCRT_WCHAR_T_DEFINED
#define MSVCRT_WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short MSVCRT(wchar_t);
#endif
#endif

#ifndef _MSC_VER
# ifndef __int64
#  define __int64 long long
# endif
#endif

#ifndef MSVCRT_DEV_T_DEFINED
typedef unsigned int   _dev_t;
#define MSVCRT_DEV_T_DEFINED
#endif

#ifndef MSVCRT_INO_T_DEFINED
typedef unsigned short _ino_t;
#define MSVCRT_INO_T_DEFINED
#endif

#ifndef MSVCRT_TIME_T_DEFINED
typedef long MSVCRT(time_t);
#define MSVCRT_TIME_T_DEFINED
#endif

#ifndef MSVCRT_OFF_T_DEFINED
typedef int MSVCRT(_off_t);
#define MSVCRT_OFF_T_DEFINED
#endif

#define _S_IEXEC  0x0040
#define _S_IWRITE 0x0080
#define _S_IREAD  0x0100
#define _S_IFIFO  0x1000
#define _S_IFCHR  0x2000
#define _S_IFDIR  0x4000
#define _S_IFREG  0x8000
#define _S_IFMT   0xF000

/* for FreeBSD */
#undef st_atime
#undef st_ctime
#undef st_mtime

#ifndef MSVCRT_STAT_DEFINED
#define MSVCRT_STAT_DEFINED

struct _stat {
  _dev_t         st_dev;
  _ino_t         st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  _dev_t         st_rdev;
  MSVCRT(_off_t) st_size;
  MSVCRT(time_t) st_atime;
  MSVCRT(time_t) st_mtime;
  MSVCRT(time_t) st_ctime;
};

struct MSVCRT(stat) {
  _dev_t         st_dev;
  _ino_t         st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  _dev_t         st_rdev;
  MSVCRT(_off_t) st_size;
  MSVCRT(time_t) st_atime;
  MSVCRT(time_t) st_mtime;
  MSVCRT(time_t) st_ctime;
};

struct _stati64 {
  _dev_t         st_dev;
  _ino_t         st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  _dev_t         st_rdev;
  __int64        st_size;
  MSVCRT(time_t) st_atime;
  MSVCRT(time_t) st_mtime;
  MSVCRT(time_t) st_ctime;
};
#endif /* MSVCRT_STAT_DEFINED */

#ifdef __cplusplus
extern "C" {
#endif

int MSVCRT(_fstat)(int,struct _stat*);
int MSVCRT(_stat)(const char*,struct _stat*);
int _fstati64(int,struct _stati64*);
int _stati64(const char*,struct _stati64*);
int _umask(int);

#ifndef MSVCRT_WSTAT_DEFINED
#define MSVCRT_WSTAT_DEFINED
int _wstat(const MSVCRT(wchar_t)*,struct _stat*);
int _wstati64(const MSVCRT(wchar_t)*,struct _stati64*);
#endif /* MSVCRT_WSTAT_DEFINED */

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define S_IFMT   _S_IFMT
#define S_IFDIR  _S_IFDIR
#define S_IFCHR  _S_IFCHR
#define S_IFREG  _S_IFREG
#define S_IREAD  _S_IREAD
#define S_IWRITE _S_IWRITE
#define S_IEXEC  _S_IEXEC

#define	S_ISCHR(m)	(((m)&_S_IFMT) == _S_IFCHR)
#define	S_ISDIR(m)	(((m)&_S_IFMT) == _S_IFDIR)
#define	S_ISFIFO(m)	(((m)&_S_IFMT) == _S_IFIFO)
#define	S_ISREG(m)	(((m)&_S_IFMT) == _S_IFREG)

static inline int fstat(int fd, struct stat* ptr) { return _fstat(fd, (struct _stat*)ptr); }
static inline int stat(const char* path, struct stat* ptr) { return _stat(path, (struct _stat*)ptr); }
#ifndef MSVCRT_UMASK_DEFINED
static inline int umask(int fd) { return _umask(fd); }
#define MSVCRT_UMASK_DEFINED
#endif
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_SYS_STAT_H */
