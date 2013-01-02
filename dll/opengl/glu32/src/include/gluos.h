/*
** gluos.h - operating system dependencies for GLU
**
*/
#ifdef __VMS
#ifdef __cplusplus
#pragma message disable nocordel
#pragma message disable codeunreachable
#pragma message disable codcauunr
#endif
#endif

#ifdef __WATCOMC__
/* Disable *lots* of warnings to get a clean build. I can't be bothered fixing the
 * code at the moment, as it is pretty ugly.
 */
#pragma warning 7   10
#pragma warning 13  10
#pragma warning 14  10
#pragma warning 367 10
#pragma warning 379 10
#pragma warning 726 10
#pragma warning 836 10
#endif

#ifdef BUILD_FOR_SNAP

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#elif defined(_WIN32)

#include <stdlib.h>	    /* For _MAX_PATH definition */
#include <stdio.h>
#include <malloc.h>

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOIME
#define NOMINMAX

#ifdef __MINGW64_VERSION_MAJOR
  #undef _WIN32_WINNT
#endif

#ifndef _WIN32_WINNT
  /* XXX: Workaround a bug in mingw-w64's headers when NOGDI is set and
   * _WIN32_WINNT >= 0x0600 */
  #define _WIN32_WINNT 0x0400
#endif
#ifndef STRICT
  #define STRICT 1
#endif

#include <windows.h>

/* Disable warnings */
#if defined(_MSC_VER)
#pragma warning(disable : 4101)
#pragma warning(disable : 4244)
#pragma warning(disable : 4761)
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1200 && _MSC_VER < 1300
#pragma comment(linker, "/OPT:NOWIN98")
#endif

#ifndef WINGDIAPI
#define WINGDIAPI
#endif

#elif defined(__OS2__)

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#define WINGDIAPI

#else

/* Disable Microsoft-specific keywords */
#define GLAPIENTRY
#define WINGDIAPI

#endif
