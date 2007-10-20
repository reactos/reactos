/*
 * PROJECT:     	ReactOS Telephone API UI DLL
 * LICENSE:     	GPL - See COPYING in the top level directory
 * FILE:        	dll/win32/tapiui/tapiui.c
 * PURPOSE:     	Telephone API UI DLL
 * COPYRIGHT:   	Dmitry Chapyshev <lentind@yandex.ru>
 *
 */

#include <windows.h>
#include "resource.h"

static HINSTANCE hDllInstance;

BOOL WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hDllInstance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}
