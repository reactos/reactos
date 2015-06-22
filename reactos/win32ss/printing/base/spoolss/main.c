/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Main functions
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

HANDLE hProcessHeap;
PRINTPROVIDOR LocalSplFuncs;


BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            hProcessHeap = GetProcessHeap();
            break;
    }

    return TRUE;
}

BOOL WINAPI
InitializeRouter(HANDLE SpoolerStatusHandle)
{
    HINSTANCE hinstLocalSpl;
    PInitializePrintProvidor pfnInitializePrintProvidor;

    // Only initialize localspl.dll for now.
    // This function should later look for all available print providers in the registry and initialize all of them.
    hinstLocalSpl = LoadLibraryW(L"localspl");
    if (!hinstLocalSpl)
    {
        ERR("LoadLibraryW for localspl failed with error %lu!\n", GetLastError());
        return FALSE;
    }

    pfnInitializePrintProvidor = (PInitializePrintProvidor)GetProcAddress(hinstLocalSpl, "InitializePrintProvidor");
    if (!pfnInitializePrintProvidor)
    {
        ERR("GetProcAddress failed with error %lu!\n", GetLastError());
        return FALSE;
    }

    if (!pfnInitializePrintProvidor(&LocalSplFuncs, sizeof(PRINTPROVIDOR), NULL))
    {
        ERR("InitializePrintProvidor failed for localspl with error %lu!\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

BOOL WINAPI
SplInitializeWinSpoolDrv(PVOID* pTable)
{
    HINSTANCE hWinspool;
    int i;

    hWinspool = LoadLibraryW(L"winspool.drv");
    if (!hWinspool)
    {
        ERR("Could not load winspool.drv, last error is %lu!\n", GetLastError());
        return FALSE;
    }

    // Get the function pointers which are meant to be returned by this function.
    pTable[0] = GetProcAddress(hWinspool, "OpenPrinterW");
    pTable[1] = GetProcAddress(hWinspool, "ClosePrinter");
    pTable[2] = GetProcAddress(hWinspool, "SpoolerDevQueryPrintW");
    pTable[3] = GetProcAddress(hWinspool, "SpoolerPrinterEvent");
    pTable[4] = GetProcAddress(hWinspool, "DocumentPropertiesW");
    pTable[5] = GetProcAddress(hWinspool, (LPSTR)212);
    pTable[6] = GetProcAddress(hWinspool, (LPSTR)213);
    pTable[7] = GetProcAddress(hWinspool, (LPSTR)214);
    pTable[8] = GetProcAddress(hWinspool, (LPSTR)215);

    // Verify that all calls succeeded.
    for (i = 0; i < 9; i++)
        if (!pTable[i])
            return FALSE;

    return TRUE;
}

BOOL WINAPI
SplIsUpgrade()
{
	return FALSE;
}

DWORD WINAPI
SpoolerInit()
{
    // Nothing to do here yet
    return ERROR_SUCCESS;
}
