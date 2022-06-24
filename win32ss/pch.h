#ifndef __W32K_H
#define __W32K_H
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32k subsystem
 * FILE:            win32ss/pch.h
 * PURPOSE:         Main Win32K Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#define _NO_COM
#define STRICT

/* DDK/NDK/SDK headers */
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WS03SP1
#include <ntifs.h>
#include <ntddmou.h>
#include <ndk/exfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/kdfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/sefuncs.h>
#include <ndk/rtlfuncs.h>
#include <ntstrsafe.h>
#include <ntintsafe.h>
#include <ntddkbd.h>

/* Win32 headers */
/* FIXME: Defines in winbase.h that we need... */
typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
#define MAKEINTATOM(i) (LPWSTR)((ULONG_PTR)((WORD)(i)))
#define WINBASEAPI
#define STARTF_USESHOWWINDOW 1
#define STARTF_USESIZE 2
#define STARTF_USEPOSITION 4
#include <stdarg.h>
#include <windef.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <intrin.h>

// Needed because windef.h messes up CDECL for whatever
#undef CDECL
#define CDECL __cdecl

/* Avoid type casting, by defining RECT to RECTL */
#define RECT RECTL
#define PRECT PRECTL
#define LPRECT LPRECTL
#define LPCRECT LPCRECTL
#define POINT POINTL
#define LPPOINT PPOINTL
#define PPOINT PPOINTL

#include <winerror.h>
#include <wingdi.h>
#define NT_BUILD_ENVIRONMENT
#define _ENGINE_EXPORT_
#include <winddi.h>
#include <winuser.h>
#include <prntfont.h>
#define _NOCSECT_TYPE
#include <ddrawi.h>
#include <imm.h>
#include <dbt.h>
#include <ntddvdeo.h>

/* SEH support with PSEH */
#include <pseh/pseh2.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Public Win32K headers */
#include <include/ntgdityp.h>
#include <ntgdi.h>
#include <include/ntgdihdl.h>
#include <include/ntgdibad.h>

#ifndef __cplusplus
#include <include/ntusrtyp.h>
#include <include/ntuser.h>
#include <include/callback.h>
#endif // __cplusplus

/* Undocumented user definitions */
#include <undocuser.h>

/* Freetype headers */
#include <ft2build.h>
#include FT_FREETYPE_H

#define InterlockedIncrementUL(Value) InterlockedIncrement((PLONG)Value)
#define InterlockedDecrementUL(Value) InterlockedDecrement((PLONG)Value)

/* Internal Win32K header */
#include "win32kp.h"

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __W32K_H */
