/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_UTIME
#define _INC_UTIME

#ifndef _WIN32
#error Only Win32 target is supported!
#endif

#include <crtdefs.h>

#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _UTIMBUF_DEFINED
#define _UTIMBUF_DEFINED

  struct _utimbuf {
    time_t actime;
    time_t modtime;
  };

  struct __utimbuf32 {
    __time32_t actime;
    __time32_t modtime;
  };

#if _INTEGRAL_MAX_BITS >= 64
  struct __utimbuf64 {
    __time64_t actime;
    __time64_t modtime;
  };
#endif

#ifndef	NO_OLDNAMES
  struct utimbuf {
    time_t actime;
    time_t modtime;
  };

  struct utimbuf32 {
    __time32_t actime;
    __time32_t modtime;
  };
#endif
#endif /* !_UTIMBUF_DEFINED */

  _CRTIMP int __cdecl _utime(const char *_Filename,struct _utimbuf *_Time);
  _CRTIMP int __cdecl _utime32(const char *_Filename,struct __utimbuf32 *_Time);
  _CRTIMP int __cdecl _futime(int _FileDes,struct _utimbuf *_Time);
  _CRTIMP int __cdecl _futime32(int _FileDes,struct __utimbuf32 *_Time);
  _CRTIMP int __cdecl _wutime(const wchar_t *_Filename,struct _utimbuf *_Time);
  _CRTIMP int __cdecl _wutime32(const wchar_t *_Filename,struct __utimbuf32 *_Time);
#if _INTEGRAL_MAX_BITS >= 64
  _CRTIMP int __cdecl _utime64(const char *_Filename,struct __utimbuf64 *_Time);
  _CRTIMP int __cdecl _futime64(int _FileDes,struct __utimbuf64 *_Time);
  _CRTIMP int __cdecl _wutime64(const wchar_t *_Filename,struct __utimbuf64 *_Time);
#endif

#ifndef RC_INVOKED
#ifdef _USE_32BIT_TIME_T
__CRT_INLINE int __cdecl _utime32(const char *_Filename,struct __utimbuf32 *_Utimbuf) {
  return _utime(_Filename,(struct _utimbuf *)_Utimbuf);
}
__CRT_INLINE int __cdecl _futime32(int _Desc,struct __utimbuf32 *_Utimbuf) {
  return _futime(_Desc,(struct _utimbuf *)_Utimbuf);
}
__CRT_INLINE int __cdecl _wutime32(const wchar_t *_Filename,struct __utimbuf32 *_Utimbuf) {
  return _wutime(_Filename,(struct _utimbuf *)_Utimbuf);
}
#endif

#ifndef	NO_OLDNAMES
__CRT_INLINE int __cdecl utime(const char *_Filename,struct utimbuf *_Utimbuf) {
  return _utime(_Filename,(struct _utimbuf *)_Utimbuf);
}
#endif
#endif

#ifdef __cplusplus
}
#endif

#pragma pack(pop)
#endif
