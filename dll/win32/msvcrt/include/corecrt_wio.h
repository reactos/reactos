/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _WIO_DEFINED
#define _WIO_DEFINED

#include <corecrt.h>

#include <pshpack8.h>

typedef __msvcrt_ulong _fsize_t;

#if defined(_USE_32BIT_TIME_T)
# define _wfinddata_t     _wfinddata32_t
# define _wfinddatai64_t  _wfinddata32i64_t
#else
# define _wfinddata_t     _wfinddata64i32_t
# define _wfinddatai64_t  _wfinddata64_t
#endif

struct _wfinddata32_t {
  unsigned   attrib;
  __time32_t time_create;
  __time32_t time_access;
  __time32_t time_write;
  _fsize_t   size;
  wchar_t    name[260];
};

struct _wfinddata32i64_t {
  unsigned   attrib;
  __time32_t time_create;
  __time32_t time_access;
  __time32_t time_write;
  __int64    DECLSPEC_ALIGN(8) size;
  wchar_t    name[260];
};

struct _wfinddata64i32_t {
  unsigned   attrib;
  __time64_t time_create;
  __time64_t time_access;
  __time64_t time_write;
  _fsize_t   size;
  wchar_t    name[260];
};

struct _wfinddata64_t {
  unsigned   attrib;
  __time64_t time_create;
  __time64_t time_access;
  __time64_t time_write;
  __int64    DECLSPEC_ALIGN(8) size;
  wchar_t    name[260];
};

#ifdef _UCRT
# ifdef _USE_32BIT_TIME_T
#  define _wfindfirst      _wfindfirst32
#  define _wfindfirsti64   _wfindfirst32i64
#  define _wfindnext       _wfindnext32
#  define _wfindnexti64    _wfindnext32i64
# else
#  define _wfindfirst      _wfindfirst64i32
#  define _wfindfirsti64   _wfindfirst64
#  define _wfindnext       _wfindnext64i32
#  define _wfindnexti64    _wfindnext64
# endif
#else /* _UCRT */
# ifdef _USE_32BIT_TIME_T
#  define _wfindfirst32    _wfindfirst
#  define _wfindfirst32i64 _wfindfirsti64
#  define _wfindnext32     _wfindnext
#  define _wfindnext32i64  _wfindnexti64
# else
#  define _wfindfirst64i32 _wfindfirst
#  define _wfindfirst64    _wfindfirsti64
#  define _wfindnext64i32  _wfindnext
#  define _wfindnext64     _wfindnexti64
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP int      __cdecl _waccess(const wchar_t*,int);
_ACRTIMP int      __cdecl _wchmod(const wchar_t*,int);
_ACRTIMP int      __cdecl _wcreat(const wchar_t*,int);
_ACRTIMP intptr_t __cdecl _wfindfirst32(const wchar_t*,struct _wfinddata32_t*);
_ACRTIMP intptr_t __cdecl _wfindfirst32i64(const wchar_t*, struct _wfinddata32i64_t*);
_ACRTIMP intptr_t __cdecl _wfindfirst64(const wchar_t*,struct _wfinddata64_t*);
_ACRTIMP intptr_t __cdecl _wfindfirst64i32(const wchar_t*, struct _wfinddata64i32_t*);
_ACRTIMP int      __cdecl _wfindnext32(intptr_t,struct _wfinddata32_t*);
_ACRTIMP int      __cdecl _wfindnext32i64(intptr_t,struct _wfinddata32i64_t*);
_ACRTIMP int      __cdecl _wfindnext64(intptr_t,struct _wfinddata64_t*);
_ACRTIMP int      __cdecl _wfindnext64i32(intptr_t,struct _wfinddata64i32_t*);
_ACRTIMP wchar_t* __cdecl _wmktemp(wchar_t*);
_ACRTIMP int      __cdecl _wopen(const wchar_t*,int,...);
_ACRTIMP int      __cdecl _wrename(const wchar_t*,const wchar_t*);
_ACRTIMP int      __cdecl _wsopen(const wchar_t*,int,int,...);
_ACRTIMP int      __cdecl _wunlink(const wchar_t*);

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* _WIO_DEFINED */
