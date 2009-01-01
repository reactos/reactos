/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_TIMEB
#define _INC_TIMEB

#include <crtdefs.h>

#ifndef _WIN32
#error Only Win32 target is supported!
#endif

#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _TIMEB_DEFINED
#define _TIMEB_DEFINED

  struct _timeb {
    time_t time;
    unsigned short millitm;
    short timezone;
    short dstflag;
  };

  struct __timeb32 {
    __time32_t time;
    unsigned short millitm;
    short timezone;
    short dstflag;
  };

#ifndef	NO_OLDNAMES
  struct timeb {
    time_t time;
    unsigned short millitm;
    short timezone;
    short dstflag;
  };
#endif

#if _INTEGRAL_MAX_BITS >= 64
  struct __timeb64 {
    __time64_t time;
    unsigned short millitm;
    short timezone;
    short dstflag;
  };
#endif

#endif /* !_TIMEB_DEFINED */

  _CRTIMP void __cdecl _ftime(struct _timeb *_Time);
  _CRT_INSECURE_DEPRECATE(_ftime32_s) _CRTIMP void __cdecl _ftime32(struct __timeb32 *_Time);
  _CRTIMP errno_t __cdecl _ftime32_s(struct __timeb32 *_Time);
#if _INTEGRAL_MAX_BITS >= 64
  _CRT_INSECURE_DEPRECATE(_ftime64_s) _CRTIMP void __cdecl _ftime64(struct __timeb64 *_Time);
  _CRTIMP errno_t __cdecl _ftime64_s(struct __timeb64 *_Time);
#endif

#ifndef NO_OLDNAMES
#if !defined (RC_INVOKED)
__CRT_INLINE void __cdecl ftime(struct timeb *_Tmb) {
  _ftime((struct _timeb *)_Tmb);
}
#endif
#endif

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#include <sec_api/sys/timeb_s.h>

#endif /* !_INC_TIMEB */
