/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll\win32\iernonce\iernonce.c
 * PURPOSE:     ReactOS Extended RunOnce processing with UI
 * PROGRAMMERS: Copyright 2013-2016 Robert Naumann
 */


#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>

#define NDEBUG
#include <debug.h>

HINSTANCE hInstance;

BOOL
WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID reserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_PROCESS_DETACH:
            hInstance = hinstDLL;
            break;
    }

   return TRUE;
}

VOID WINAPI RunOnceExProcess(HWND hwnd, HINSTANCE hInst, LPCSTR path, int nShow)
{
    DPRINT1("RunOnceExProcess() not implemented\n");
}
