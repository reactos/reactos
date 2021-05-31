/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ReactOS Extended RunOnce processing with UI.
 * COPYRIGHT:   Copyright 2013-2016 Robert Naumann
 *              Copyright 2021 He Yang <1160386205@qq.com>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <windows.h>

#define NDEBUG
#include <debug.h>

#include "registry.h"


BOOL
WINAPI
DllMain(_In_ HINSTANCE hinstDLL,
        _In_ DWORD dwReason,
        _In_ LPVOID reserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

   return TRUE;
}

extern "C" VOID WINAPI
RunOnceExProcess(_In_ HWND hwnd,
                 _In_ HINSTANCE hInst,
                 _In_ LPCSTR path,
                 _In_ int nShow)
{
    RunOnceExInstance RunonceExInst_LM(HKEY_LOCAL_MACHINE);
    RunOnceExInstance RunonceExInst_CU(HKEY_CURRENT_USER);

    if (RunonceExInst_LM.m_bSuccess)
    {
        // TODO: continue coding here
    }

    if (RunonceExInst_CU.m_bSuccess)
    {
        // TODO: continue coding here
    }
}
