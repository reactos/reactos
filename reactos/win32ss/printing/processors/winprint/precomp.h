/*
 * PROJECT:     ReactOS Standard Print Processor
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Precompiled Header for all source files
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#ifndef _PRECOMP_H
#define _PRECOMP_H

#define WIN32_NO_STATUS
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <winspool.h>
#include <winsplp.h>

#include <spoolss.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(winprint);

// Structures
typedef struct _WINPRINT_HANDLE
{
    enum { RAW } Datatype;
    DWORD dwJobID;
    PWSTR pwszDatatype;
    PWSTR pwszDocumentName;
    PWSTR pwszOutputFile;
    PWSTR pwszPrinterPort;
}
WINPRINT_HANDLE, *PWINPRINT_HANDLE;

// raw.c
DWORD PrintRawJob(PWINPRINT_HANDLE pHandle, PWSTR pwszPrinterAndJob);

#endif
