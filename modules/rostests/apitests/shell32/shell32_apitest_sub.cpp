/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for SHChangeNotify
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include "SHChangeNotify.h"

static HWND s_hwnd = NULL;
static UINT s_uRegID = 0;
static BOOL s_fRecursive = FALSE;
static DIRTYPE s_iWatchDir = DIRTYPE_NULL;
static INT s_nSources = 0;
static LPITEMIDLIST s_pidl = NULL;
static WCHAR s_path1[MAX_PATH], s_path2[MAX_PATH];
static BYTE s_counters[TYPE_MAX + 1];
static HANDLE s_hEvent = NULL;

static BOOL
OnCreate(HWND hwnd)
{
    s_hwnd = hwnd;
    s_pidl = DoGetPidl(s_iWatchDir);

    SHChangeNotifyEntry entry;
    entry.pidl = s_pidl;
    entry.fRecursive = s_fRecursive;
    s_uRegID = SHChangeNotifyRegister(hwnd, s_nSources, SHCNE_ALLEVENTS, WM_SHELL_NOTIFY, 1, &entry);
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

static BOOL DoPathes(PIDLIST_ABSOLUTE pidl1, PIDLIST_ABSOLUTE pidl2)
{
    WCHAR path[MAX_PATH];
    if (!SHGetPathFromIDListW(pidl1, path))
    {
        s_path1[0] = s_path2[0] = 0;
        return FALSE;
    }

    if (wcsstr(path, L"Recent") != NULL)
        return FALSE;

    StringCchCopyW(s_path1, _countof(s_path1), path);

    if (!SHGetPathFromIDListW(pidl2, s_path2))
        s_path2[0] = 0;

    return TRUE;
}

static VOID DoPathesAndFlags(UINT type, PIDLIST_ABSOLUTE pidl1, PIDLIST_ABSOLUTE pidl2)
{
    if (DoPathes(pidl1, pidl2))
    {
        s_counters[type] = 1;
        SetEvent(s_hEvent);
    }
}

static void
DoShellNotify(HWND hwnd, PIDLIST_ABSOLUTE pidl1, PIDLIST_ABSOLUTE pidl2, LONG lEvent)
{
    switch (lEvent)
    {
        case SHCNE_RENAMEITEM:
            DoPathesAndFlags(TYPE_RENAMEITEM, pidl1, pidl2);
            break;
        case SHCNE_CREATE:
            DoPathesAndFlags(TYPE_CREATE, pidl1, pidl2);
            break;
        case SHCNE_DELETE:
            DoPathesAndFlags(TYPE_DELETE, pidl1, pidl2);
            break;
        case SHCNE_MKDIR:
            DoPathesAndFlags(TYPE_MKDIR, pidl1, pidl2);
            break;
        case SHCNE_RMDIR:
            DoPathesAndFlags(TYPE_RMDIR, pidl1, pidl2);
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
            DoPathesAndFlags(TYPE_UPDATEDIR, pidl1, pidl2);
            break;
        case SHCNE_UPDATEITEM:
            break;
        case SHCNE_SERVERDISCONNECT:
            break;
        case SHCNE_UPDATEIMAGE:
            break;
        case SHCNE_DRIVEADDGUI:
            break;
        case SHCNE_RENAMEFOLDER:
            DoPathesAndFlags(TYPE_RENAMEFOLDER, pidl1, pidl2);
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
    StringCchCopyW(szText, _countof(szText), s_path1);
    StringCchCatW(szText, _countof(szText), L"|");
    StringCchCatW(szText, _countof(szText), s_path2);

    FILE *fp = _wfopen(TEMP_FILE, L"wb");
    if (fp)
    {
        fwrite(szText, (wcslen(szText) + 1) * sizeof(WCHAR), 1, fp);
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
            s_path1[0] = s_path2[0] = 0;
            break;

        case WM_SET_PATHS:
            DoSetPaths(hwnd);
            break;

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

static BOOL ParseCommandLine(LPWSTR lpCmdLine)
{
    LPWSTR pch = lpCmdLine; // fRecursive,iWatchDir,nSources
    s_fRecursive = !!wcstoul(pch, NULL, 0);
    pch = wcschr(pch, L',');
    if (!pch)
        return FALSE;
    ++pch;

    s_iWatchDir = (DIRTYPE)wcstoul(pch, NULL, 0);
    pch = wcschr(pch, L',');
    if (!pch)
        return FALSE;
    ++pch;

    s_nSources = wcstoul(pch, NULL, 0);
    return TRUE;
}

INT APIENTRY
wWinMain(HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPWSTR    lpCmdLine,
         INT       nCmdShow)
{
    if (lstrcmpiW(lpCmdLine, L"") == 0 || lstrcmpiW(lpCmdLine, L"TEST") == 0)
        return 0;

    if (!ParseCommandLine(lpCmdLine))
        return -1;

    s_hEvent = OpenEventW(EVENT_ALL_ACCESS, TRUE, EVENT_NAME);

    WNDCLASSW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = CLASSNAME;
    if (!RegisterClassW(&wc))
        return -1;

    HWND hwnd = CreateWindowW(CLASSNAME, CLASSNAME, WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 400, 100,
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

    CloseHandle(s_hEvent);

    return 0;
}
