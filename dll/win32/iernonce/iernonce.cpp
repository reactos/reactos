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
#include "dialog.h"

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
        if (!Instance.m_bSuccess)
            continue;

        if ((Instance.m_dwFlags & FLAGS_NO_STAT_DIALOG) || !Instance.m_bShowDialog)
        {
            Instance.Exec(NULL);
        }
        else
        {
            // The dialog is responsible to create a thread and execute.
            ProgressDlg dlg(Instance);
            dlg.RunDialogBox();
        }
    }

    CoUninitialize();
}

extern "C" VOID WINAPI
InitCallback(
    _In_ PVOID Callback,
    _In_ BOOL bSilence)
{
    // FIXME: unimplemented
}
