/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/shellext/stobject/hotplug.cpp
 * PURPOSE:     Hotplug notification icon handler
 * PROGRAMMERS: Eric Kohl <eric.kohl@reactos.org>
 *              David Quintana <gigaherz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(stobject);

static HICON g_hIconHotplug = NULL;
static BOOL g_IsRunning = FALSE;


HRESULT STDMETHODCALLTYPE Hotplug_Init(_In_ CSysTray * pSysTray)
{
    WCHAR strTooltip[128];

    TRACE("Hotplug_Init\n");

    g_hIconHotplug = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_EXTRACT));

    LoadStringW(g_hInstance, IDS_HOTPLUG_REMOVE_1, strTooltip, _countof(strTooltip));

    g_IsRunning = TRUE;

    return pSysTray->NotifyIcon(NIM_ADD, ID_ICON_HOTPLUG, g_hIconHotplug, strTooltip);
}

HRESULT STDMETHODCALLTYPE Hotplug_Update(_In_ CSysTray * pSysTray)
{
    TRACE("Hotplug_Update\n");
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Hotplug_Shutdown(_In_ CSysTray * pSysTray)
{
    TRACE("Hotplug_Shutdown\n");

    g_IsRunning = FALSE;

    return pSysTray->NotifyIcon(NIM_DELETE, ID_ICON_HOTPLUG, NULL, NULL);
}

static void RunHotplug()
{
    ShellExecuteW(NULL, NULL, L"rundll32.exe", L"shell32.dll,Control_RunDLL hotplug.dll", NULL, SW_SHOWNORMAL);
}

static void ShowContextMenu(CSysTray *pSysTray)
{
    WCHAR szBuffer[128];
    DWORD id, msgPos;
    HMENU hPopup;

    LoadStringW(g_hInstance, IDS_HOTPLUG_REMOVE_2, szBuffer, _countof(szBuffer));

    hPopup = CreatePopupMenu();
    AppendMenuW(hPopup, MF_STRING, 1, szBuffer);

    msgPos = GetMessagePos();

    SetForegroundWindow(pSysTray->GetHWnd());
    id = TrackPopupMenuEx(hPopup,
                          TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                          GET_X_LPARAM(msgPos),
                          GET_Y_LPARAM(msgPos),
                          pSysTray->GetHWnd(),
                          NULL);

    DestroyMenu(hPopup);

    if (id == 1)
        RunHotplug();
}

static
VOID
ShowHotplugPopupMenu(
    HWND hWnd)
{
#if 0
    DWORD id, msgPos;

    HMENU hPopup = CreatePopupMenu();

    // FIXME
    AppendMenuW(hPopup, MF_STRING, IDS_VOL_OPEN, strOpen);

    msgPos = GetMessagePos();

    SetForegroundWindow(hWnd);
    id = TrackPopupMenuEx(hPopup,
                          TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                          GET_X_LPARAM(msgPos),
                          GET_Y_LPARAM(msgPos),
                          hWnd,
                          NULL);

    DestroyMenu(hPopup);

    if (id != 0)
    {
        // FIXME
    }
#endif
}

HRESULT STDMETHODCALLTYPE Hotplug_Message(_In_ CSysTray *pSysTray, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT &lResult)
{
    TRACE("Hotplug_Message uMsg=%d, wParam=%x, lParam=%x\n", uMsg, wParam, lParam);

    switch (uMsg)
    {
        case WM_USER + 220:
            TRACE("Hotplug_Message: WM_USER+220\n");
            if (wParam == 2)
            {
                if (lParam == FALSE)
                    return Hotplug_Init(pSysTray);
                else
                    return Hotplug_Shutdown(pSysTray);
            }
            return S_FALSE;

        case WM_USER + 221:
            TRACE("Hotplug_Message: WM_USER+221\n");
            if (wParam == 2)
            {
                lResult = (LRESULT)g_IsRunning;
                return S_OK;
            }
            return S_FALSE;

        case ID_ICON_HOTPLUG:
            Hotplug_Update(pSysTray);

            switch (lParam)
            {
                case WM_LBUTTONDOWN:
                    SetTimer(pSysTray->GetHWnd(), HOTPLUG_TIMER_ID, 500, NULL);
                    break;

                case WM_LBUTTONUP:
                    break;

                case WM_LBUTTONDBLCLK:
                    KillTimer(pSysTray->GetHWnd(), HOTPLUG_TIMER_ID);
                    RunHotplug();
                    break;

                case WM_RBUTTONDOWN:
                    break;

                case WM_RBUTTONUP:
                    ShowContextMenu(pSysTray);
                    break;

                case WM_RBUTTONDBLCLK:
                    break;

                case WM_MOUSEMOVE:
                    break;
            }
            return S_OK;

        default:
            TRACE("Hotplug_Message received for unknown ID %d, ignoring.\n");
            return S_FALSE;
    }

    return S_FALSE;
}

VOID
Hotplug_OnTimer(HWND hWnd)
{
    TRACE("Hotplug_OnTimer\n!");
    KillTimer(hWnd, HOTPLUG_TIMER_ID);
    ShowHotplugPopupMenu(hWnd);
}
