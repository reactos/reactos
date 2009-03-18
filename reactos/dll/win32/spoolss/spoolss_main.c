/*
 * Implementation of the Spooler-Service helper DLL
 *
 * Copyright 2006 Detlef Riekenberg
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "wingdi.h"
#include "winspool.h"
#include "ddk/winsplp.h"
#include "spoolss.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(spoolss);

/* ################################ */

static HMODULE hwinspool;

static const WCHAR winspooldrvW[] = {'w','i','n','s','p','o','o','l','.','d','r','v',0};

/******************************************************************************
 *
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
        case DLL_WINE_PREATTACH:
            return FALSE;  /* prefer native version */
        case DLL_PROCESS_ATTACH: {
            DisableThreadLibraryCalls(hinstDLL);
            break;

        case DLL_PROCESS_DETACH:
            backend_unload_all();
            break;
        }
    }
    return TRUE;
}

/******************************************************************
 *   AllocSplStr   [SPOOLSS.@]
 *
 * Create a copy from the String on the Spooler-Heap
 *
 * PARAMS
 *  pwstr [I] PTR to the String to copy
 *
 * RETURNS
 *  Failure: NULL
 *  Success: PTR to the copied String
 *
 */
LPWSTR WINAPI AllocSplStr(LPCWSTR pwstr)
{
    LPWSTR  res = NULL;
    DWORD   len;

    TRACE("(%s)\n", debugstr_w(pwstr));
    if (!pwstr) return NULL;

    len = (lstrlenW(pwstr) + 1) * sizeof(WCHAR);
    res = HeapAlloc(GetProcessHeap(), 0, len);
    if (res) lstrcpyW(res, pwstr);
        
    TRACE("returning %p\n", res);
    return res;
}

/******************************************************************
 *   BuildOtherNamesFromMachineName   [SPOOLSS.@]
 */
BOOL WINAPI BuildOtherNamesFromMachineName(LPVOID * ptr1, LPVOID * ptr2)
{
    FIXME("(%p, %p) stub\n", ptr1, ptr2);

    *ptr1 = NULL;
    *ptr2 = NULL;
    return FALSE;
}

/******************************************************************
 *   DllAllocSplMem   [SPOOLSS.@]
 *
 * Allocate cleared memory from the spooler heap
 *
 * PARAMS
 *  size [I] Number of bytes to allocate
 *
 * RETURNS
 *  Failure: NULL
 *  Success: PTR to the allocated memory
 *
 * NOTES
 *  We use the process heap (Windows use a separate spooler heap)
 *
 */
LPVOID WINAPI DllAllocSplMem(DWORD size)
{
    LPVOID  res;

    res = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    TRACE("(%d) => %p\n", size, res);
    return res;
}

/******************************************************************
 *   DllFreeSplMem   [SPOOLSS.@]
 *
 * Free the allocated spooler memory
 *
 * PARAMS
 *  memory [I] PTR to the memory allocated by DllAllocSplMem
 *
 * RETURNS
 *  Failure: FALSE
 *  Success: TRUE
 *
 * NOTES
 *  We use the process heap (Windows use a separate spooler heap)
 *
 */

BOOL WINAPI DllFreeSplMem(LPBYTE memory)
{
    TRACE("(%p)\n", memory);
    return HeapFree(GetProcessHeap(), 0, memory);
}

/******************************************************************
 *   DllFreeSplStr   [SPOOLSS.@]
 *
 * Free the allocated Spooler-String
 *
 * PARAMS
 *  pwstr [I] PTR to the WSTR, allocated by AllocSplStr
 *
 * RETURNS
 *  Failure: FALSE
 *  Success: TRUE
 *
 */

BOOL WINAPI DllFreeSplStr(LPWSTR pwstr)
{
    TRACE("(%s) PTR: %p\n", debugstr_w(pwstr), pwstr);
    return HeapFree(GetProcessHeap(), 0, pwstr);
}


/******************************************************************
 *   ImpersonatePrinterClient   [SPOOLSS.@]
 */
BOOL WINAPI ImpersonatePrinterClient(HANDLE hToken)
{
    FIXME("(%p) stub\n", hToken);
    return TRUE;
}

/******************************************************************
 *   InitializeRouter   [SPOOLSS.@]
 */
BOOL WINAPI InitializeRouter(void)
{
    TRACE("()\n");
    return backend_load_all();
}

/******************************************************************
 *   IsLocalCall    [SPOOLSS.@]
 */
BOOL WINAPI IsLocalCall(void)
{
    FIXME("() stub\n");
    return TRUE;
}

/******************************************************************
 *   RevertToPrinterSelf   [SPOOLSS.@]
 */
HANDLE WINAPI RevertToPrinterSelf(void)
{
    FIXME("() stub\n");
    return (HANDLE) 0xdead0947;
}

/******************************************************************
 *   SplInitializeWinSpoolDrv   [SPOOLSS.@]
 *
 * Dynamic load "winspool.drv" and fill an array with some function-pointer
 *
 * PARAMS
 *  table  [I] array of function-pointer to fill
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 * NOTES
 *  Native "spoolss.dll" from w2k fill the table with 11 Function-Pointer.
 *  We implement the XP-Version (The table has only 9 Pointer)
 *
 */
BOOL WINAPI SplInitializeWinSpoolDrv(LPVOID * table)
{
    DWORD res;

    TRACE("(%p)\n", table);

    hwinspool = LoadLibraryW(winspooldrvW);
    if (!hwinspool) return FALSE;

    table[0] = (void *) GetProcAddress(hwinspool, "OpenPrinterW");
    table[1] = (void *) GetProcAddress(hwinspool, "ClosePrinter");
    table[2] = (void *) GetProcAddress(hwinspool, "SpoolerDevQueryPrintW");
    table[3] = (void *) GetProcAddress(hwinspool, "SpoolerPrinterEvent");
    table[4] = (void *) GetProcAddress(hwinspool, "DocumentPropertiesW");
    table[5] = (void *) GetProcAddress(hwinspool, (LPSTR) 212);  /* LoadPrinterDriver */
    table[6] = (void *) GetProcAddress(hwinspool, (LPSTR) 213);  /* RefCntLoadDriver */
    table[7] = (void *) GetProcAddress(hwinspool, (LPSTR) 214);  /* RefCntUnloadDriver */
    table[8] = (void *) GetProcAddress(hwinspool, (LPSTR) 215);  /* ForceUnloadDriver */

    for (res = 0; res < 9; res++) {
        if (table[res] == NULL) return FALSE;
    }

    return TRUE;

}

/******************************************************************
 *   SplIsUpgrade   [SPOOLSS.@]
 */
BOOL WINAPI SplIsUpgrade(void)
{
    FIXME("() stub\n");
    return FALSE;
}

/******************************************************************
 *   SpoolerHasInitialized  [SPOOLSS.@]
 */
BOOL WINAPI SpoolerHasInitialized(void)
{
    FIXME("() stub\n");
    return TRUE;
}

/******************************************************************
 *   SpoolerInit   [SPOOLSS.@]
 */
BOOL WINAPI SpoolerInit(void)
{
    FIXME("() stub\n");
    return TRUE;
}

/******************************************************************
 *   WaitForSpoolerInitialization   [SPOOLSS.@]
 */
BOOL WINAPI WaitForSpoolerInitialization(void)
{
    FIXME("() stub\n");
    return TRUE;
}
