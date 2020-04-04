/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for SHChangeNotify
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <shlwapi.h>
#include <stdio.h>

#define WM_SHELL_NOTIFY (WM_USER + 100)
#define WM_GET_NOTIFY_FLAGS (WM_USER + 101)
#define WM_CLEAR_FLAGS (WM_USER + 102)
#define WM_SET_PATHS (WM_USER + 103)

static HWND s_hwnd = NULL;
static const WCHAR s_szName[] = L"SHChangeNotify testcase";

typedef enum TYPE
{
    TYPE_RENAMEITEM,
    TYPE_CREATE,
    TYPE_DELETE,
    TYPE_MKDIR,
    TYPE_RMDIR,
    TYPE_UPDATEDIR,
    TYPE_UPDATEITEM,
    TYPE_RENAMEFOLDER,
    TYPE_FREESPACE
} TYPE;

static BYTE s_counters[TYPE_FREESPACE + 1];
static UINT s_uRegID = 0;

static WCHAR s_dir1[MAX_PATH];  // "%TEMP%\\WatchDir1"
static WCHAR s_dir2[MAX_PATH];  // "%TEMP%\\WatchDir1\\Dir2"
static WCHAR s_dir3[MAX_PATH];  // "%TEMP%\\WatchDir1\\Dir3"
static WCHAR s_file1[MAX_PATH]; // "%TEMP%\\WatchDir1\\File1.txt"
static WCHAR s_file2[MAX_PATH]; // "%TEMP%\\WatchDir1\\File2.txt"

static WCHAR s_path1[MAX_PATH], s_path2[MAX_PATH];

static LPITEMIDLIST s_pidl = NULL;
static SHChangeNotifyEntry s_entry;

static BOOL
OnCreate(HWND hwnd)
{
    s_hwnd = hwnd;

    WCHAR szTemp[MAX_PATH], szPath[MAX_PATH];

    GetTempPathW(_countof(szTemp), szTemp);
    GetLongPathNameW(szTemp, szPath, _countof(szPath));

    lstrcpyW(s_dir1, szPath);
    PathAppendW(s_dir1, L"WatchDir1");

    lstrcpyW(s_dir2, s_dir1);
    PathAppendW(s_dir2, L"Dir2");

    lstrcpyW(s_dir3, s_dir1);
    PathAppendW(s_dir3, L"Dir3");

    lstrcpyW(s_file1, s_dir1);
    PathAppendW(s_file1, L"File1.txt");

    lstrcpyW(s_file2, s_dir1);
    PathAppendW(s_file2, L"File2.txt");

    s_pidl = ILCreateFromPathW(s_dir1);

    s_entry.pidl = s_pidl;
    s_entry.fRecursive = TRUE;
    LONG fEvents = SHCNE_ALLEVENTS;
    s_uRegID = SHChangeNotifyRegister(hwnd, SHCNRF_ShellLevel, fEvents, WM_SHELL_NOTIFY,
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
            s_counters[TYPE_FREESPACE] = 1;
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
    DWORD dwFlags = 0;
    for (size_t i = 0; i < _countof(s_counters); ++i)
    {
        if (s_counters[i])
            dwFlags |= (1 << i);
    }
    return dwFlags;
}

static void
DoSetClipText(HWND hwnd)
{
    if (!OpenClipboard(hwnd))
        return;

    EmptyClipboard();

    WCHAR szText[MAX_PATH * 2];
    lstrcpyW(szText, s_path1);
    lstrcatW(szText, L"|");
    lstrcatW(szText, s_path2);

    DWORD cbText = (lstrlenW(szText) + 1) * sizeof(WCHAR);
    HGLOBAL hGlobal = GlobalAlloc(GHND | GMEM_SHARE, cbText);
    if (hGlobal)
    {
        LPWSTR psz = (LPWSTR)GlobalLock(hGlobal);
        if (psz)
        {
            CopyMemory(psz, szText, cbText);
            GlobalUnlock(hGlobal);

            SetClipboardData(CF_UNICODETEXT, hGlobal);
        }
    }

    CloseClipboard();
    Sleep(60);
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
            DoSetClipText(hwnd);
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
