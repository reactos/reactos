/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests whether a dialog is getting visible even if WM_INITDIALOG ends it.
 * COPYRIGHT:   Copyright 2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winuser.h>

#define NTOS_MODE_USER
#include <ndk/iotypes.h> // Needed for kdtypes.h
#include <ndk/ketypes.h> //   "     "     "
#include <ndk/kdtypes.h> // For SharedUserData

#include "resource.h"

INT_PTR CALLBACK
AboutDlgProc(
    _In_ HWND hDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Intentional breakpoint for you to step-by-step debug into a debugger! */
            if (IsDebuggerPresent() || SharedUserData->KdDebuggerEnabled)
                __debugbreak();

            SendMessageW(hDlg, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), 0);
            EndDialog(hDlg, -1);
            return (INT_PTR)TRUE;
        }

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
        }
    }
    return (INT_PTR)FALSE;
}

int APIENTRY
wWinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    return DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_ABOUTBOX), NULL, AboutDlgProc);
}
