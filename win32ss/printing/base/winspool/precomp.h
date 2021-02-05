/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Precompiled Header for all source files
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#ifndef _PRECOMP_H
#define _PRECOMP_H

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winreg.h>
#include <winspool.h>
#include <winspool_c.h>
#include <winsplp.h>
#include <winddiui.h>
#include <ndk/rtlfuncs.h>
#include <strsafe.h>

#include <spoolss.h>
#include <marshalling/marshalling.h>

#include "wspool.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(winspool);

#define SPOOLER_HANDLE_SIG 'gg'

// Structures
/*
 * Describes a handle returned by AddPrinterW or OpenPrinterW.
 */
typedef struct _SPOOLER_HANDLE
{
    DWORD_PTR Sig;
    BOOL bStartedDoc : 1;
    BOOL bJob : 1;
    BOOL bAnsi : 1;
    BOOL bDocEvent : 1;
    BOOL bTrayIcon : 1;
    BOOL bNoColorProfile : 1;
    BOOL bShared : 1;
    BOOL bClosed : 1;
    DWORD dwJobID;
    HANDLE hPrinter;
    HANDLE hSPLFile;
    DWORD cCount;
    HANDLE hSpoolFileHandle;
    DWORD dwOptions;
}
SPOOLER_HANDLE, *PSPOOLER_HANDLE;

// main.c
extern HANDLE hProcessHeap;
extern CRITICAL_SECTION rtlCritSec;

// utils.c
DWORD UnicodeToAnsiInPlace(PWSTR pwszField);
DWORD UnicodeToAnsiZZInPlace(PWSTR pwszzField);
SECURITY_DESCRIPTOR * get_sd(SECURITY_DESCRIPTOR *sd, DWORD *size);
LONG WINAPI IntProtectHandle(HANDLE,BOOL);
BOOL WINAPI IntUnprotectHandle(HANDLE);
VOID UpdateTrayIcon(HANDLE hPrinter, DWORD JobId);

// devmode.c
extern void RosConvertAnsiDevModeToUnicodeDevmode(PDEVMODEA pDevModeInput, PDEVMODEW *pDevModeOutput);

extern void RosConvertUnicodeDevModeToAnsiDevmode(PDEVMODEW pDevModeInput, PDEVMODEA pDevModeOutput);

//
// [MS-EMF] 2.2.27 UniversalFontId Object
//
typedef struct _UNIVERSAL_FONT_ID
{
    ULONG CheckSum;
    ULONG Index;
} UNIVERSAL_FONT_ID, *PUNIVERSAL_FONT_ID;

BOOL WINAPI IsValidDevmodeNoSizeW(PDEVMODEW pDevmode);

/* RtlCreateUnicodeStringFromAsciiz will return an empty string in the buffer
   if passed a NULL string. This returns NULLs to the result.
*/
static inline PWSTR AsciiToUnicode( UNICODE_STRING * usBufferPtr, LPCSTR src )
{
    if ( (src) )
    {
        RtlCreateUnicodeStringFromAsciiz(usBufferPtr, src);
        return usBufferPtr->Buffer;
    }
    usBufferPtr->Buffer = NULL; /* so that RtlFreeUnicodeString won't barf */
    return NULL;
}

#endif
