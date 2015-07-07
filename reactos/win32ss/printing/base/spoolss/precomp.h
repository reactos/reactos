/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Precompiled Header for all source files
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#ifndef _PRECOMP_H
#define _PRECOMP_H

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <winspool.h>
#include <winsplp.h>
#include <ndk/rtlfuncs.h>

#include <spoolss.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(spoolss);

// Function pointers
typedef BOOL (WINAPI *PInitializePrintProvidor)(LPPRINTPROVIDOR, DWORD, LPWSTR);

// Structures
/**
 * Describes a Print Provider.
 */
typedef struct _SPOOLSS_PRINT_PROVIDER
{
    LIST_ENTRY Entry;
    PRINTPROVIDOR PrintProvider;
}
SPOOLSS_PRINT_PROVIDER, *PSPOOLSS_PRINT_PROVIDER;

/*
 * Describes a handle returned by OpenPrinterW.
 * We can't just pass the handle returned by the Print Provider, because spoolss has to remember which Print Provider opened this handle.
 */
typedef struct _SPOOLSS_PRINTER_HANDLE
{
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;         /** Pointer to the Print Provider that opened this printer. */
    HANDLE hPrinter;                                /** The handle returned by fpOpenPrinter of the Print Provider and passed to subsequent Print Provider functions. */
}
SPOOLSS_PRINTER_HANDLE, *PSPOOLSS_PRINTER_HANDLE;

// main.c
extern HANDLE hProcessHeap;
extern LIST_ENTRY PrintProviderList;

#endif
