/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _IO_DEFINED
#define _IO_DEFINED

#include <corecrt_wio.h>

#include <pshpack8.h>

#if defined(_USE_32BIT_TIME_T)
# define _finddata_t     _finddata32_t
# define _finddatai64_t  _finddata32i64_t
#else
# define _finddata_t     _finddata64i32_t
# define _finddatai64_t  _finddata64_t
#endif

struct _finddata32_t {
  unsigned   attrib;
  __time32_t time_create;
  __time32_t time_access;
  __time32_t time_write;
  _fsize_t   size;
  char       name[260];
};

struct _finddata32i64_t {
  unsigned   attrib;
  __time32_t time_create;
  __time32_t time_access;
  __time32_t time_write;
  __int64    DECLSPEC_ALIGN(8) size;
  char       name[260];
};

struct _finddata64i32_t {
  unsigned   attrib;
  __time64_t time_create;
  __time64_t time_access;
  __time64_t time_write;
  _fsize_t   size;
  char       name[260];
};

struct _finddata64_t {
  unsigned   attrib;
  __time64_t time_create;
  __time64_t time_access;
  __time64_t time_write;
  __int64    DECLSPEC_ALIGN(8) size;
  char       name[260];
};

/* The following are also defined in dos.h */
#define _A_NORMAL 0x00000000
#define _A_RDONLY 0x00000001
#define _A_HIDDEN 0x00000002
#define _A_SYSTEM 0x00000004
#define _A_VOLID  0x00000008
#define _A_SUBDIR 0x00000010
#define _A_ARCH   0x00000020

#ifdef _UCRT
# ifdef _USE_32BIT_TIME_T
#  define _findfirst      _findfirst32
#  define _findfirsti64   _findfirst32i64
#  define _findnext       _findnext32
#  define _findnexti64    _findnext32i64
# else
#  define _findfirst      _findfirst64i32
#  define _findfirsti64   _findfirst64
#  define _findnext       _findnext64i32
#  define _findnexti64    _findnext64
# endif
#else /* _UCRT */
# ifdef _USE_32BIT_TIME_T
#  define _findfirst32    _findfirst
#  define _findfirst32i64 _findfirsti64
#  define _findnext32     _findnext
#  define _findnext32i64  _findnexti64
# else
#  define _findfirst64i32 _findfirst
#  define _findfirst64    _findfirsti64
#  define _findnext64i32  _findnext
#  define _findnext64     _findnexti64
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP int      __cdecl _access(const char*,int);
_ACRTIMP int      __cdecl _chmod(const char*,int);
_ACRTIMP int      __cdecl _chsize(int,__msvcrt_long);
_ACRTIMP int      __cdecl _chsize_s(int,__int64);
_ACRTIMP int      __cdecl _close(int);
_ACRTIMP int      __cdecl _creat(const char*,int);
_ACRTIMP int      __cdecl _dup(int);
_ACRTIMP int      __cdecl _dup2(int,int);
_ACRTIMP int      __cdecl _eof(int);
_ACRTIMP __int64  __cdecl _filelengthi64(int);
_ACRTIMP __msvcrt_long __cdecl _filelength(int);
_ACRTIMP int      __cdecl _findclose(intptr_t);
#ifdef _UCRT
_ACRTIMP intptr_t __cdecl _findfirst32(const char*,struct _finddata32_t*);
_ACRTIMP intptr_t __cdecl _findfirst32i64(const char*, struct _finddata32i64_t*);
_ACRTIMP intptr_t __cdecl _findfirst64(const char*,struct _finddata64_t*);
_ACRTIMP intptr_t __cdecl _findfirst64i32(const char*, struct _finddata64i32_t*);
_ACRTIMP int      __cdecl _findnext32(intptr_t,struct _finddata32_t*);
_ACRTIMP int      __cdecl _findnext32i64(intptr_t,struct _finddata32i64_t*);
_ACRTIMP int      __cdecl _findnext64(intptr_t,struct _finddata64_t*);
_ACRTIMP int      __cdecl _findnext64i32(intptr_t,struct _finddata64i32_t*);
#else
_ACRTIMP intptr_t __cdecl _findfirst(const char*,struct _finddata_t*);
_ACRTIMP intptr_t __cdecl _findfirsti64(const char*, struct _finddatai64_t*);
_ACRTIMP intptr_t __cdecl _findfirst64(const char*, struct _finddata64_t*);
_ACRTIMP int      __cdecl _findnext(intptr_t,struct _finddata_t*);
_ACRTIMP int      __cdecl _findnexti64(intptr_t, struct _finddatai64_t*);
_ACRTIMP int      __cdecl _findnext64(intptr_t, struct _finddata64_t*);
#endif
_ACRTIMP intptr_t __cdecl _get_osfhandle(int);
_ACRTIMP int      __cdecl _isatty(int);
_ACRTIMP int      __cdecl _locking(int,int,__msvcrt_long);
_ACRTIMP __msvcrt_long __cdecl _lseek(int,__msvcrt_long,int);
_ACRTIMP __int64  __cdecl _lseeki64(int,__int64,int);
_ACRTIMP char*    __cdecl _mktemp(char*);
_ACRTIMP int      __cdecl _mktemp_s(char*,size_t);
_ACRTIMP int      __cdecl _open(const char*,int,...);
_ACRTIMP int      __cdecl _open_osfhandle(intptr_t,int);
_ACRTIMP int      __cdecl _pipe(int*,unsigned int,int);
_ACRTIMP int      __cdecl _read(int,void*,unsigned int);
_ACRTIMP int      __cdecl _setmode(int,int);
_ACRTIMP int      __cdecl _sopen(const char*,int,int,...);
_ACRTIMP errno_t  __cdecl _sopen_dispatch(const char*,int,int,int,int*,int);
_ACRTIMP errno_t  __cdecl _sopen_s(int*,const char*,int,int,int);
_ACRTIMP __msvcrt_long __cdecl _tell(int);
_ACRTIMP __int64  __cdecl _telli64(int);
_ACRTIMP int      __cdecl _umask(int);
_ACRTIMP int      __cdecl _unlink(const char*);
_ACRTIMP int      __cdecl _write(int,const void*,unsigned int);
_ACRTIMP int      __cdecl remove(const char*);
_ACRTIMP int      __cdecl rename(const char*,const char*);

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* _IO_DEFINED */
