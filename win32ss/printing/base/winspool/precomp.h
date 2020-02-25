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
#include <wingdi.h>
#include <winreg.h>
#include <winspool.h>
#include <winspool_c.h>
#include <winddiui.h>
#include <ndk/rtlfuncs.h>
#include <strsafe.h>

#include <spoolss.h>
#include <marshalling/marshalling.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(winspool);

// Structures
/*
 * Describes a handle returned by OpenPrinterW.
 */
typedef struct _SPOOLER_HANDLE
{
    BOOL bStartedDoc : 1;
    DWORD dwJobID;
    HANDLE hPrinter;
    HANDLE hSPLFile;
}
SPOOLER_HANDLE, *PSPOOLER_HANDLE;

// main.c
extern HANDLE hProcessHeap;

// utils.c
extern BOOL UnicodeToAnsiInPlace(PWSTR pwszField);

// devmode.c
extern void RosConvertAnsiDevModeToUnicodeDevmode(PDEVMODEA pDevModeInput, PDEVMODEW pDevModeOutput);

extern void RosConvertUnicodeDevModeToAnsiDevmode(PDEVMODEW pDevModeInput, PDEVMODEA pDevModeOutput);

#endif
