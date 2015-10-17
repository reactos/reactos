/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            win32ss/gdi/gdi32/include/precomp.h
 * PURPOSE:         User-Mode Win32 GDI Library Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#ifndef _GDI32_PCH_
#define _GDI32_PCH_

/* INCLUDES ******************************************************************/

/* Definitions */
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

#include <stdarg.h>

/* SDK/DDK/NDK Headers. */
#include <windef.h>

/* Avoid type casting, by defining RECT to RECTL */
#define RECT RECTL
#define PRECT PRECTL
#define LPRECT LPRECTL
#define LPCRECT LPCRECTL
#define POINT POINTL
#define LPPOINT PPOINTL
#define PPOINT PPOINTL

#include <winbase.h>
#include <winnls.h>
#include <objbase.h>
#include <ndk/rtlfuncs.h>
#include <wingdi.h>
#define _ENGINE_EXPORT_
#include <winddi.h>
#include <prntfont.h>
#include <winddiui.h>
#include <winspool.h>

#include <ddrawi.h>
#include <ddrawgdi.h>

/* Public Win32K Headers */
#include <ntgdityp.h>
#include <ntgdi.h>
#include <ntgdihdl.h>

/* Private GDI32 Header */
#include "gdi32p.h"

/* Deprecated NTGDI calls which shouldn't exist */
#include <ntgdibad.h>

#endif /* _GDI32_PCH_ */
