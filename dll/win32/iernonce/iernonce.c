/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll\win32\iernonce\iernonce.c
 * PURPOSE:     DLL for RunOnceEx Keys
 * PROGRAMMERS: Copyright 2013 Robert Naumann
 */


#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <setupapi.h>

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
