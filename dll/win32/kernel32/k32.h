/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            dll/win32/kernel32/k32.h
 * PURPOSE:         Win32 Kernel Library Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#ifndef __K32_H
#define __K32_H

/* INCLUDES ******************************************************************/

#include <stdio.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winbase_undoc.h>
#include <wingdi.h>
#include <winreg.h>
#include <wincon.h>
#include <wincon_undoc.h>
#include <winuser.h>

#undef TEXT
#define TEXT(s) L##s
#include <regstr.h>

#include <tlhelp32.h>

/* Redefine NTDDI_VERSION to 2K3 SP1 to get correct NDK definitions */
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WS03SP1

#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/iotypes.h>
#include <ndk/kdtypes.h>
#include <ndk/kefuncs.h>
#include <ndk/ldrfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>
#include <ndk/umfuncs.h>

#include <ntstrsafe.h>

/* CSRSS Headers */
#include <csr/csr.h>
#include <win/base.h>
#include <win/basemsg.h>
#include <win/console.h>
#include <win/conmsg.h>
#include <win/vdm.h>

/* DDK Driver Headers */
#include <mountmgr.h>

/* Internal Kernel32 Header */
#include "include/kernel32.h"

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

/* Base Macros */
#include "include/base_x.h"

/* Console API Client Definitions */
#include "include/console.h"

/* Virtual DOS Machines (VDM) Support Definitions */
#include "include/vdm.h"

/* Undo hacks in wine/unicode.h */
#undef tolowerW
static inline WCHAR tolowerW( WCHAR ch )
{
    extern WINE_UNICODE_API const WCHAR wine_casemap_lower[];
    return ch + wine_casemap_lower[wine_casemap_lower[ch >> 8] + (ch & 0xff)];
}

#undef toupperW
static inline WCHAR toupperW( WCHAR ch )
{
    extern WINE_UNICODE_API const WCHAR wine_casemap_upper[];
    return ch + wine_casemap_upper[wine_casemap_upper[ch >> 8] + (ch & 0xff)];
}

#endif /* __K32_H */
