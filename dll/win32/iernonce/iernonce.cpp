/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ReactOS Extended RunOnce processing with UI.
 * COPYRIGHT:   Copyright 2013-2016 Robert Naumann
 *              Copyright 2021 He Yang <1160386205@qq.com>
 */

#include "iernonce.h"

RUNONCEEX_CALLBACK g_Callback = NULL;
BOOL g_bSilence = FALSE;

BOOL
WINAPI
DllMain(_In_ HINSTANCE hinstDLL,
        _In_ DWORD dwReason,
        _In_ LPVOID reserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

   return TRUE;
}

extern "C" VOID WINAPI
RunOnceExProcess(_In_ HWND hwnd,
                 _In_ HINSTANCE hInst,
                 _In_ LPCSTR pszCmdLine,
                 _In_ int nCmdShow)
{
    // iernonce may use shell32 API.
    HRESULT Result = CoInitialize(NULL);
    if (Result != S_OK && Result != S_FALSE)
    {
        return;
    }

    HKEY RootKeys[] = { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER };
    for (UINT i = 0; i < _countof(RootKeys); ++i)
    {
        RunOnceExInstance Instance(RootKeys[i]);
        Instance.Run(g_bSilence);
    }

    CoUninitialize();
}

extern "C" VOID WINAPI
InitCallback(_In_ RUNONCEEX_CALLBACK Callback,
             _In_ BOOL bSilence)
{
    g_Callback = Callback;
    g_bSilence = bSilence;
}
