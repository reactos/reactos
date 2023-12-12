/*
 * PROJECT:     ReactOS CTF Monitor
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero TIP Bar front-end
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "ctfmon.h"
#include "CTipBarWnd.h"
#include "CRegWatcher.h"

BOOL CTipBarWnd::s_bUninitedSystem = FALSE;
BOOL CTipBarWnd::s_bWndClassRegistered = FALSE;

BOOL CTipBarWnd::Init()
{
    if (s_bWndClassRegistered)
        return TRUE; // Already registered

    // Register a window class
    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hInstance = g_hInst;
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = L"CiCTipBarClass";
    if (!::RegisterClassExW(&wc))
        return FALSE;

    s_bWndClassRegistered = TRUE; // Remember
    return TRUE;
}

HWND CTipBarWnd::CreateWnd()
{
    m_hWnd = ::CreateWindowExW(0, L"CiCTipBarClass", NULL, WS_DISABLED,
                               0, 0, 0, 0, NULL, NULL, g_hInst, NULL);
    return m_hWnd;
}

LRESULT CALLBACK
CTipBarWnd::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            break;

        case WM_DESTROY:
            ::PostQuitMessage(0);
            break;

        case WM_QUERYENDSESSION:
            // Is the system non-NT and (non-log-on or non-log-off)?
            if (!(g_dwOsInfo & OSINFO_NT) && (!g_fWinLogon || !(lParam & ENDSESSION_LOGOFF)))
            {
                // Un-initialize now
                ClosePopupTipbar();
                TF_UninitSystem();
                s_bUninitedSystem = TRUE;
            }
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
