#ifndef __W32K_H
#define __W32K_H
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Graphics Subsystem
 * FILE:            subsys/win32k/pch.h
 * PURPOSE:         Main Win32K Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#define _NO_COM

/* DDK/NDK/SDK Headers */
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WS03SP1
#include <ntddk.h>
#include <ntddmou.h>
#include <ntddvdeo.h>
#include <ntifs.h>
#include <tvout.h>
#include <ndk/exfuncs.h>
#include <ndk/kbd.h>
#include <ndk/kdfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/lpcfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ntstrsafe.h>
#include <ntddkbd.h>
#include <bugcodes.h>

/* Win32 Headers */
/* FIXME: Defines in winbase.h that we need... */
typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
#define WINBASEAPI
#define STARTF_USESIZE 2
#define STARTF_USEPOSITION 4
#include <stdarg.h>
#include <windef.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <intrin.h>

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
#include <winddi.h>
#include <winuser.h>
#include <prntfont.h>
#include <dde.h>
#include <wincon.h>
#define _NOCSECT_TYPE
#include <ddrawi.h>

/* SEH Support with PSEH */
#include <pseh/pseh2.h>

/* CSRSS Header */
#include <csrss/csrss.h>

/* Public Win32K Headers */
#include <win32k/callback.h>
#include <win32k/ntusrtyp.h>
#include <win32k/ntuser.h>
#include <win32k/ntgdityp.h>
#include <win32k/ntgdibad.h>
#include <win32k/ntgdihdl.h>
#include <ntgdi.h>

/* Undocumented user definitions */
#include <undocuser.h>

/* DirectX headers */
#include <ddrawint.h>
#include <ddkernel.h>
#include <reactos/drivers/directx/directxint.h>
#include <reactos/drivers/directx/dxg.h>
#include <reactos/drivers/directx/dxeng.h>

/* Freetype headers*/
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_TYPE1_TABLES_H
#include <freetype/tttables.h>
#include <freetype/fttrigon.h>
#include <freetype/ftglyph.h>
#include <freetype/ftbitmap.h>
#include <freetype/ftoutln.h>
#include <freetype/ftwinfnt.h>
#include <freetype/freetype.h>

/* Internal Win32K Header */
#include "include/win32kp.h"

#endif /* __W32K_H */
