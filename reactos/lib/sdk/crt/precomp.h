#ifndef _CRT_PRECOMP_H
#define _CRT_PRECOMP_H

/* We don't want to use the Microsoft CRT inline functions
   so we hack around them in msvc build */
#define _INC_WTIME_INL
#define _INC_UTIME_INL
#define _INC_TIME_INL

/* needed to expose _s api definitions */
#define MINGW_HAS_SECURE_API 1

/* Headers to be compiled */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <conio.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <malloc.h>
#include <math.h>
#include <limits.h>
#include <io.h>

#include <sys/stat.h>
#include <sys/locking.h>
#include <share.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <winuser.h>
#include <ndk/rtlfuncs.h>

// Our headers are pretty messed up
#undef CDECL
#if defined(_M_IX86)
#define CDECL __cdecl
#else
#define CDECL
#endif

#if !defined(_MSC_VER)
  #include <stdint.h>
#endif

/* kernelmode libcnt should not include Wine stuff */
#ifndef _LIBCNT_
#include "wine/unicode.h"
#include "wine/config.h"
#define WINE_NO_TRACE_MSGS
#include "wine/debug.h"
#ifndef __WINE_DEBUG_CHANNEL__
WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);
#endif
#else
#define NDEBUG
#include <debug.h>
#define TRACE DPRINT
#define WARN DPRINT1
#endif

/* CRT Internal data */
#include <internal/atexit.h>
#include <internal/console.h>
#include <internal/ieee.h>
#include <internal/locale.h>
#include <internal/math.h>
#include <internal/mbstring.h>
#include <internal/misc.h>
#include <internal/mtdll.h>
#include <internal/rterror.h>
#include <internal/safecrt.h>
#include <internal/time.h>
#if !defined(_LIBCNT_) && !defined(_MSVCRTEX_)
#include <internal/tls.h>
#endif

#endif /* _CRT_PRECOMP_H */
