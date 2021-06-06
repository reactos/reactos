/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for SHChangeNotify
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <shlwapi.h>
#include <stdio.h>
#include "SHChangeNotify.h"

static HWND s_hwnd = NULL;
static const WCHAR s_szName[] = L"SHChangeNotify testcase";
static INT s_nMode;

static BYTE s_counters[TYPE_RENAMEFOLDER + 1];
static UINT s_uRegID = 0;

static WCHAR s_path1[MAX_PATH], s_path2[MAX_PATH];

static LPITEMIDLIST s_pidl = NULL;
static SHChangeNotifyEntry s_entry;

static BOOL
OnCreate(HWND hwnd)
{
    s_hwnd = hwnd;

    DoInitPaths();

    s_pidl = ILCreateFromPathW(s_dir1);
    s_entry.pidl = s_pidl;

    INT nSources;
    switch (s_nMode)
    {
        case 0:
            s_entry.fRecursive = TRUE;
            nSources = SHCNRF_ShellLevel;
            break;

        case 1:
            s_entry.fRecursive = TRUE;
            nSources = SHCNRF_ShellLevel | SHCNRF_InterruptLevel;
            break;

        case 2:
            s_entry.fRecursive = FALSE;
            nSources = SHCNRF_ShellLevel | SHCNRF_NewDelivery;
            break;

        case 3:
            s_entry.fRecursive = TRUE;
            nSources = SHCNRF_InterruptLevel | SHCNRF_RecursiveInterrupt | SHCNRF_NewDelivery;
            break;

        case 4:
            s_entry.fRecursive = FALSE;
            nSources = SHCNRF_InterruptLevel | SHCNRF_NewDelivery;
            break;

        case 5:
            s_entry.fRecursive = TRUE;
            nSources = SHCNRF_InterruptLevel | SHCNRF_RecursiveInterrupt | SHCNRF_NewDelivery;
            s_entry.pidl = NULL;
            break;

        default:
            return FALSE;
    }
    LONG fEvents = SHCNE_ALLEVENTS;
    s_uRegID = SHChangeNotifyRegister(hwnd, nSources, fEvents, WM_SHELL_NOTIFY,
                                      1, &s_entry);
    return s_uRegID != 0;
}

static void
OnCommand(HWND hwnd, UINT id)
{
    switch (id)
    {
        case IDOK:
        case IDCANCEL:
            DestroyWindow(hwnd);
            break;
    }
}

static void
OnDestroy(HWND hwnd)
{
    SHChangeNotifyDeregister(s_uRegID);
    s_uRegID = 0;

    CoTaskMemFree(s_pidl);
    s_pidl = NULL;

    PostQuitMessage(0);
    s_hwnd = NULL;
}

static void
DoShellNotify(HWND hwnd, PIDLIST_ABSOLUTE pidl1, PIDLIST_ABSOLUTE pidl2, LONG lEvent)
{
    if (pidl1)
        SHGetPathFromIDListW(pidl1, s_path1);
    else
        s_path1[0] = 0;

    if (pidl2)
        SHGetPathFromIDListW(pidl2, s_path2);
    else
        s_path2[0] = 0;

    switch (lEvent)
    {
        case SHCNE_RENAMEITEM:
            s_counters[TYPE_RENAMEITEM] = 1;
            break;
        case SHCNE_CREATE:
            s_counters[TYPE_CREATE] = 1;
            break;
        case SHCNE_DELETE:
            s_counters[TYPE_DELETE] = 1;
            break;
        case SHCNE_MKDIR:
            s_counters[TYPE_MKDIR] = 1;
            break;
        case SHCNE_RMDIR:
            s_counters[TYPE_RMDIR] = 1;
            break;
        case SHCNE_MEDIAINSERTED:
            break;
        case SHCNE_MEDIAREMOVED:
            break;
        case SHCNE_DRIVEREMOVED:
            break;
        case SHCNE_DRIVEADD:
            break;
        case SHCNE_NETSHARE:
            break;
        case SHCNE_NETUNSHARE:
            break;
        case SHCNE_ATTRIBUTES:
            break;
        case SHCNE_UPDATEDIR:
            s_counters[TYPE_UPDATEDIR] = 1;
            break;
        case SHCNE_UPDATEITEM:
            s_counters[TYPE_UPDATEITEM] = 1;
            break;
        case SHCNE_SERVERDISCONNECT:
            break;
        case SHCNE_UPDATEIMAGE:
            break;
        case SHCNE_DRIVEADDGUI:
            break;
        case SHCNE_RENAMEFOLDER:
            s_counters[TYPE_RENAMEFOLDER] = 1;
            break;
        case SHCNE_FREESPACE:
            break;
        case SHCNE_EXTENDED_EVENT:
            break;
        case SHCNE_ASSOCCHANGED:
            break;
        default:
            break;
    }
}

static INT_PTR
OnShellNotify(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LONG lEvent;
    PIDLIST_ABSOLUTE *pidlAbsolute;
    HANDLE hLock = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &pidlAbsolute, &lEvent);
    if (hLock)
    {
        DoShellNotify(hwnd, pidlAbsolute[0], pidlAbsolute[1], lEvent);
        SHChangeNotification_Unlock(hLock);
    }
    else
    {
        pidlAbsolute = (PIDLIST_ABSOLUTE *)wParam;
        DoShellNotify(hwnd, pidlAbsolute[0], pidlAbsolute[1], lParam);
    }
    return TRUE;
}

static LRESULT
OnGetNotifyFlags(HWND hwnd)
{
    if (s_uRegID == 0)
        return 0xFFFFFFFF;

    DWORD dwFlags = 0;
    for (size_t i = 0; i < _countof(s_counters); ++i)
    {
        if (s_counters[i])
            dwFlags |= (1 << i);
    }
    return dwFlags;
}

static void
DoSetPaths(HWND hwnd)
{
    WCHAR szText[MAX_PATH * 2];
    lstrcpyW(szText, s_path1);
    lstrcatW(szText, L"|");
    lstrcatW(szText, s_path2);

    if (FILE *fp = fopen(TEMP_FILE, "wb"))
    {
        fwrite(szText, (lstrlenW(szText) + 1) * sizeof(WCHAR), 1, fp);
        fflush(fp);
        fclose(fp);
    }
}

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            return (OnCreate(hwnd) ? 0 : -1);

        case WM_COMMAND:
            OnCommand(hwnd, LOWORD(wParam));
            break;

        case WM_SHELL_NOTIFY:
            return OnShellNotify(hwnd, wParam, lParam);

        case WM_DESTROY:
            OnDestroy(hwnd);
            break;

        case WM_GET_NOTIFY_FLAGS:
            return OnGetNotifyFlags(hwnd);

        case WM_CLEAR_FLAGS:
            ZeroMemory(&s_counters, sizeof(s_counters));
            break;

        case WM_SET_PATHS:
            DoSetPaths(hwnd);
            break;

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

INT APIENTRY
wWinMain(HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPWSTR    lpCmdLine,
         INT       nCmdShow)
{
    if (lstrcmpiW(lpCmdLine, L"") == 0 || lstrcmpiW(lpCmdLine, L"TEST") == 0)
        return 0;

    s_nMode = _wtoi(lpCmdLine);

    WNDCLASSW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = s_szName;
    if (!RegisterClassW(&wc))
        return -1;

    HWND hwnd = CreateWindowW(s_szName, s_szName, WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
                              NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!hwnd)
        return -1;

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
