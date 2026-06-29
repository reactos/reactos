/*
 * _stat() definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_SYS_STAT_H
#define __WINE_SYS_STAT_H

#include <corecrt.h>
#include <sys/types.h>

#include <pshpack8.h>

#ifndef _DEV_T_DEFINED
# ifdef _CRTDLL
typedef unsigned short _dev_t;
# else
typedef unsigned int _dev_t;
# endif
#define _DEV_T_DEFINED
#endif

#ifndef _INO_T_DEFINED
typedef unsigned short _ino_t;
#define _INO_T_DEFINED
#endif

#ifndef _OFF_T_DEFINED
typedef int _off_t;
#define _OFF_T_DEFINED
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

#ifndef _STAT_DEFINED
#define _STAT_DEFINED

struct _stat {
  _dev_t st_dev;
  _ino_t st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  _dev_t st_rdev;
  _off_t st_size;
  time_t st_atime;
  time_t st_mtime;
  time_t st_ctime;
};

struct stat {
  _dev_t st_dev;
  _ino_t st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  _dev_t st_rdev;
  _off_t st_size;
  time_t st_atime;
  time_t st_mtime;
  time_t st_ctime;
};

struct _stat32 {
  _dev_t st_dev;
  _ino_t st_ino;
  unsigned short st_mode;
  short st_nlink;
  short st_uid;
  short st_gid;
  _dev_t st_rdev;
  _off_t st_size;
  __time32_t st_atime;
  __time32_t st_mtime;
  __time32_t st_ctime;
};

struct _stat32i64 {
  _dev_t st_dev;
  _ino_t st_ino;
  unsigned short st_mode;
  short st_nlink;
  short st_uid;
  short st_gid;
  _dev_t st_rdev;
  __int64 DECLSPEC_ALIGN(8) st_size;
  __time32_t st_atime;
  __time32_t st_mtime;
  __time32_t st_ctime;
};

struct _stat64i32 {
  _dev_t st_dev;
  _ino_t st_ino;
  unsigned short st_mode;
  short st_nlink;
  short st_uid;
  short st_gid;
  _dev_t st_rdev;
  _off_t st_size;
  __time64_t st_atime;
  __time64_t st_mtime;
  __time64_t st_ctime;
};

struct _stati64 {
  _dev_t st_dev;
  _ino_t st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  _dev_t st_rdev;
  __int64 DECLSPEC_ALIGN(8) st_size;
  time_t st_atime;
  time_t st_mtime;
  time_t st_ctime;
};

struct _stat64 {
  _dev_t st_dev;
  _ino_t st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  _dev_t st_rdev;
  __int64 DECLSPEC_ALIGN(8) st_size;
  __time64_t     st_atime;
  __time64_t     st_mtime;
  __time64_t     st_ctime;
};
#endif /* _STAT_DEFINED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _UCRT
# ifdef _USE_32BIT_TIME_T
#  define _fstat      _fstat32
#  define _fstati64   _fstat32i64
#  define _stat       _stat32
#  define _stati64    _stat32i64
#  define _wstat      _wstat32
#  define _wstati64   _wstat32i64
# else
#  define _fstat      _fstat64i32
#  define _fstati64   _fstat64
#  define _stat       _stat64i32
#  define _stati64    _stat64
#  define _wstat      _wstat64i32
#  define _wstati64   _wstat64
# endif
#else /* _UCRT */
# ifdef _USE_32BIT_TIME_T
#  define _fstat32    _fstat
#  define _fstat32i64 _fstati64
#  define _stat32i64  _stati64
#  define _stat32     _stat
#  define _wstat32    _wstat
#  define _wstat32i64 _wstati64
# else
#  define _fstat64i32 _fstat
#  define _fstat64    _fstati64
#  define _stat64     _stati64
#  define _stat64i32  _stat
#  define _wstat64i32 _wstat
#  define _wstat64    _wstati64
# endif
#endif

#define __stat64 _stat64

_ACRTIMP int __cdecl _fstat32(int, struct _stat32*);
_ACRTIMP int __cdecl _fstat32i64(int, struct _stat32i64*);
_ACRTIMP int __cdecl _fstat64(int,struct _stat64*);
_ACRTIMP int __cdecl _fstat64i32(int,struct _stat64i32*);
_ACRTIMP int __cdecl _stat32(const char*, struct _stat32*);
_ACRTIMP int __cdecl _stat32i64(const char*, struct _stat32i64*);
_ACRTIMP int __cdecl _stat64(const char*,struct _stat64*);
_ACRTIMP int __cdecl _stat64i32(const char*,struct _stat64i32*);
_ACRTIMP int __cdecl _umask(int);
_ACRTIMP int __cdecl _wstat32(const wchar_t*,struct _stat32*);
_ACRTIMP int __cdecl _wstat32i64(const wchar_t*, struct _stat32i64*);
_ACRTIMP int __cdecl _wstat64(const wchar_t*,struct _stat64*);
_ACRTIMP int __cdecl _wstat64i32(const wchar_t*,struct _stat64i32*);

#ifdef __cplusplus
}
#endif


#define S_IFMT   _S_IFMT
#define S_IFDIR  _S_IFDIR
#define S_IFCHR  _S_IFCHR
#define S_IFREG  _S_IFREG
#define S_IREAD  _S_IREAD
#define S_IWRITE _S_IWRITE
#define S_IEXEC  _S_IEXEC

static inline int fstat(int fd, struct stat* ptr) { return _fstat(fd, (struct _stat*)ptr); }
static inline int stat(const char* path, struct stat* ptr) { return _stat(path, (struct _stat*)ptr); }
#ifndef _UMASK_DEFINED
static inline int umask(int fd) { return _umask(fd); }
#define _UMASK_DEFINED
#endif

#include <poppack.h>

#endif /* __WINE_SYS_STAT_H */
