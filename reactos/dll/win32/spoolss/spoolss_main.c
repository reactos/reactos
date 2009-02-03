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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(spoolss);

/* ################################ */

static CRITICAL_SECTION backend_cs;
static CRITICAL_SECTION_DEBUG backend_cs_debug =
{
    0, 0, &backend_cs,
    { &backend_cs_debug.ProcessLocksList, &backend_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": backend_cs") }
};
static CRITICAL_SECTION backend_cs = { &backend_cs_debug, -1, 0, 0, 0, 0 };

/* ################################ */

static HMODULE hwinspool;
static HMODULE hlocalspl;
static BOOL (WINAPI *pInitializePrintProvidor)(LPPRINTPROVIDOR, DWORD, LPWSTR);

static PRINTPROVIDOR * backend;

/* ################################ */

static const WCHAR localspldllW[] = {'l','o','c','a','l','s','p','l','.','d','l','l',0};
static const WCHAR winspooldrvW[] = {'w','i','n','s','p','o','o','l','.','d','r','v',0};

/******************************************************************************
 * backend_load [internal]
 *
 * load and init our backend (the local printprovider: "localspl.dll")
 *
 * PARAMS
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE and RPC_S_SERVER_UNAVAILABLE
 *
 * NOTES
 *  In windows, the spooler router (spoolss.dll) support multiple
 *  printprovider (localspl.dll for the local system)
 *
 */
static BOOL backend_load(void)
{
    static PRINTPROVIDOR mybackend;
    DWORD res;

    if (backend) return TRUE;

    EnterCriticalSection(&backend_cs);
    hlocalspl = LoadLibraryW(localspldllW);
    if (hlocalspl) {
        pInitializePrintProvidor = (void *) GetProcAddress(hlocalspl, "InitializePrintProvidor");
        if (pInitializePrintProvidor) {

            /* native localspl does not clear unused entries */
            memset(&mybackend, 0, sizeof(mybackend));
            res = pInitializePrintProvidor(&mybackend, sizeof(mybackend), NULL);
            if (res) {
                backend = &mybackend;
                LeaveCriticalSection(&backend_cs);
                TRACE("backend: %p (%p)\n", backend, hlocalspl);
                return TRUE;
            }
        }
        FreeLibrary(hlocalspl);
    }

    LeaveCriticalSection(&backend_cs);

    WARN("failed to load the backend: %u\n", GetLastError());
    SetLastError(RPC_S_SERVER_UNAVAILABLE);
    return FALSE;
}

/******************************************************************
 * unload_backend [internal]
 *
 */
static void backend_unload(void)
{
    EnterCriticalSection(&backend_cs);
    if (backend) {
        backend = NULL;
        FreeLibrary(hlocalspl);
    }
    LeaveCriticalSection(&backend_cs);
}

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
            backend_unload();
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
    return backend_load();
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
