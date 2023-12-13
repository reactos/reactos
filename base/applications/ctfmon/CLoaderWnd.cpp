/*
 * PROJECT:     ReactOS CTF Monitor
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero TIP Bar loader window
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include "CLoaderWnd.h"
#include "CRegWatcher.h"

BOOL CLoaderWnd::s_bUninitedSystem = FALSE;
BOOL CLoaderWnd::s_bWndClassRegistered = FALSE;

BOOL CLoaderWnd::Init()
{
    if (s_bWndClassRegistered)
        return TRUE; // Already registered

    // Register a window class
    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize           = sizeof(wc);
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.hInstance        = g_hInst;
    wc.hCursor          = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.lpfnWndProc      = WindowProc;
    wc.lpszClassName    = L"CiCTipBarClass";
    if (!::RegisterClassExW(&wc))
        return FALSE;

    s_bWndClassRegistered = TRUE; // Remember
    return TRUE;
}

HWND CLoaderWnd::CreateWnd()
{
    m_hWnd = ::CreateWindowExW(0, L"CiCTipBarClass", NULL, WS_DISABLED,
                               0, 0, 0, 0, NULL, NULL, g_hInst, NULL);
    return m_hWnd;
}

LRESULT CALLBACK
CLoaderWnd::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            break;

        case WM_DESTROY:
            ::PostQuitMessage(0);
            break;

        case WM_QUERYENDSESSION:
            // NOTE: We don't support Win95/98/Me
#ifdef SUPPORT_WIN9X
            if (!(g_dwOsInfo & OSINFO_NT) && (!g_fWinLogon || (lParam & ENDSESSION_LOGOFF)))
            {
                ClosePopupTipbar();
                TF_UninitSystem();
                CLoaderWnd::s_bUninitedSystem = TRUE;
            }
#endif
            return TRUE;

        case WM_ENDSESSION:
            if (wParam) // The session is being ended?
            {
                if (!s_bUninitedSystem)
                {
                    // Un-initialize now
                    UninitApp();
                    TF_UninitSystem();
                    s_bUninitedSystem = TRUE;
                }
            }
            else if (s_bUninitedSystem) // Once un-initialized?
            {
                // Re-initialize
                TF_InitSystem();
                if (!g_bOnWow64)
                    GetPopupTipbar(hwnd, !!g_fWinLogon);

                s_bUninitedSystem = FALSE;
            }
            break;

        case WM_SYSCOLORCHANGE:
        case WM_DISPLAYCHANGE:
            if (!g_bOnWow64) // Is the system x86/x64 native?
                CRegWatcher::StartSysColorChangeTimer();
            break;

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}
