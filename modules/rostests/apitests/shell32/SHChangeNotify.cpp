/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for SHChangeNotify
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <stdio.h>
#include <shlwapi.h>

#define WM_SHELL_NOTIFY (WM_USER + 100)

#define ID_STAGE1 1001
#define ID_STAGE2 1002
#define ID_STAGE3 1003
#define ID_STAGE4 1004
#define ID_STAGE5 1005
#define ID_STAGE6 1006
#define ID_STAGE7 1007
#define ID_STAGE8 1008

static WCHAR s_szDir1[MAX_PATH];    // "%TEMP%\\WatchDir1"
static WCHAR s_szDir2[MAX_PATH];    // "%TEMP%\\WatchDir1\\Dir2"
static WCHAR s_szDir3[MAX_PATH];    // "%TEMP%\\WatchDir1\\Dir3"
static WCHAR s_szFile1[MAX_PATH];   // "%TEMP%\\WatchDir1\\File1.txt"
static WCHAR s_szFile2[MAX_PATH];   // "%TEMP%\\WatchDir1\\File2.txt"

static HWND s_hwnd = NULL;
static WCHAR s_szName[] = L"SHChangeNotify testcase";
static LPITEMIDLIST s_pidl = NULL;
static UINT s_uRegID = 0;
static SHChangeNotifyEntry s_entry;

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

#define DoClearCounters() ZeroMemory(&s_counters, sizeof(s_counters));
#define DoCompareCounters(pattern) \
    memcmp(s_counters, (pattern), sizeof(s_counters)) == 0

static LPCSTR
GetCounters(void)
{
    size_t i;
    static char buf[TYPE_FREESPACE + 1 + 1];
    for (i = 0; i < sizeof(buf); ++i)
    {
        buf[i] = (char)('0' + s_counters[i]);
    }
    buf[i] = 0;
    return buf;
}

static BOOL
DoInit(HWND hwnd)
{
    WCHAR szTemp[MAX_PATH], szPath[MAX_PATH];

    GetTempPathW(_countof(szTemp), szTemp);
    GetLongPathNameW(szTemp, szPath, _countof(szPath));

    lstrcpyW(s_szDir1, szPath);
    PathAddBackslashW(s_szDir1);
    lstrcatW(s_szDir1, L"WatchDir1");
    CreateDirectoryW(s_szDir1, NULL);
    trace("s_szDir1: %S\n", s_szDir1);

    lstrcpyW(s_szDir2, s_szDir1);
    PathAddBackslashW(s_szDir2);
    lstrcatW(s_szDir2, L"Dir2");
    trace("s_szDir2: %S\n", s_szDir2);

    lstrcpyW(s_szDir3, s_szDir1);
    PathAddBackslashW(s_szDir3);
    lstrcatW(s_szDir3, L"Dir3");
    trace("s_szDir3: %S\n", s_szDir3);

    lstrcpyW(s_szFile1, s_szDir1);
    PathAddBackslashW(s_szFile1);
    lstrcatW(s_szFile1, L"File1.txt");
    trace("s_szFile1: %S\n", s_szFile1);

    lstrcpyW(s_szFile2, s_szDir1);
    PathAddBackslashW(s_szFile2);
    lstrcatW(s_szFile2, L"File2.txt");
    trace("s_szFile2: %S\n", s_szFile2);

    s_pidl = ILCreateFromPathW(s_szDir1);

    s_entry.pidl = s_pidl;
    s_entry.fRecursive = TRUE;
    LONG fEvents = SHCNE_ALLEVENTS;
    s_uRegID = SHChangeNotifyRegister(hwnd, SHCNRF_ShellLevel/* | SHCNRF_NewDelivery*/,
                                      fEvents, WM_SHELL_NOTIFY, 1, &s_entry);
    return s_uRegID != 0;
}

static BOOL
DoCreateEmptyFile(LPCWSTR pszFileName)
{
    FILE *fp = _wfopen(pszFileName, L"wb");
    fclose(fp);
    return fp != NULL;
}

static DWORD WINAPI
ThreadFunc(LPVOID)
{
    ok_int(CreateDirectoryW(s_szDir2, NULL), TRUE);
    SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW | SHCNF_FLUSH, s_szDir2, NULL);
    SendMessageW(s_hwnd, WM_COMMAND, ID_STAGE1, 0);

    ok_int(MoveFileW(s_szDir2, s_szDir3), TRUE);
    SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_PATHW | SHCNF_FLUSH, s_szDir2, s_szDir3);
    SendMessageW(s_hwnd, WM_COMMAND, ID_STAGE2, 0);

    ok_int(DoCreateEmptyFile(s_szFile1), TRUE);
    SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW | SHCNF_FLUSH, s_szFile1, NULL);
    SendMessageW(s_hwnd, WM_COMMAND, ID_STAGE3, 0);

    ok_int(MoveFileExW(s_szFile1, s_szFile2, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED), TRUE);
    SHChangeNotify(SHCNE_RENAMEITEM, SHCNF_PATHW | SHCNF_FLUSH, s_szFile1, s_szFile2);
    SendMessageW(s_hwnd, WM_COMMAND, ID_STAGE4, 0);

    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW | SHCNF_FLUSH, s_szFile1, NULL);
    SendMessageW(s_hwnd, WM_COMMAND, ID_STAGE5, 0);

    ok_int(DeleteFileW(s_szFile2), TRUE);
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW | SHCNF_FLUSH, s_szFile1, NULL);
    SendMessageW(s_hwnd, WM_COMMAND, ID_STAGE6, 0);

    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATHW | SHCNF_FLUSH, s_szFile1, NULL);
    SendMessageW(s_hwnd, WM_COMMAND, ID_STAGE7, 0);

    ok_int(RemoveDirectoryW(s_szDir3), TRUE);
    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHW | SHCNF_FLUSH, s_szDir1, s_szDir2);
    SendMessageW(s_hwnd, WM_COMMAND, ID_STAGE8, 0);

    SendMessageW(s_hwnd, WM_COMMAND, IDOK, 0);
    return 0;
}

static BOOL
OnCreate(HWND hwnd)
{
    s_hwnd = hwnd;

    BOOL bOK = DoInit(hwnd);
    if (!bOK)
    {
        skip("SHChangeNotifyRegister failed\n");
        return FALSE;
    }

    DWORD tid;
    HANDLE hThread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, &tid);
    if (hThread == NULL)
    {
        skip("CreateThread failed\n");
        return FALSE;
    }
    CloseHandle(hThread);

    return TRUE;
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
        case ID_STAGE1:
            trace("ID_STAGE1\n");
            ok(DoCompareCounters("\0\0\0\1\0\0\0\0\0"), "s_counters was %s\n", GetCounters());
            break;
        case ID_STAGE2:
            trace("ID_STAGE2\n");
            ok(DoCompareCounters("\0\0\0\0\0\0\0\1\0"), "s_counters was %s\n", GetCounters());
            break;
        case ID_STAGE3:
            trace("ID_STAGE3\n");
            ok(DoCompareCounters("\0\1\0\0\0\0\0\0\0"), "s_counters was %s\n", GetCounters());
            break;
        case ID_STAGE4:
            trace("ID_STAGE4\n");
            ok(DoCompareCounters("\1\0\0\0\0\0\0\0\0"), "s_counters was %s\n", GetCounters());
            break;
        case ID_STAGE5:
            trace("ID_STAGE5\n");
            ok(DoCompareCounters("\0\0\0\0\0\0\1\0\0"), "s_counters was %s\n", GetCounters());
            break;
        case ID_STAGE6:
            trace("ID_STAGE6\n");
            ok(DoCompareCounters("\0\0\1\0\0\0\0\0\0"), "s_counters was %s\n", GetCounters());
            break;
        case ID_STAGE7:
            trace("ID_STAGE7\n");
            ok(DoCompareCounters("\0\0\0\0\0\1\0\0\0"), "s_counters was %s\n", GetCounters());
            break;
        case ID_STAGE8:
            trace("ID_STAGE8\n");
            ok(DoCompareCounters("\0\0\0\0\1\0\0\0\0"), "s_counters was %s\n", GetCounters());
            break;
    }

    DoClearCounters();
}

static void
OnDestroy(HWND hwnd)
{
    SHChangeNotifyDeregister(s_uRegID);
    CoTaskMemFree(s_pidl);
    DeleteFileW(s_szFile1);
    DeleteFileW(s_szFile2);
    RemoveDirectoryW(s_szDir3);
    RemoveDirectoryW(s_szDir2);
    RemoveDirectoryW(s_szDir1);
    PostQuitMessage(0);
    s_hwnd = NULL;
}

static void
DoShellNotify(HWND hwnd, PIDLIST_ABSOLUTE pidl1, PIDLIST_ABSOLUTE pidl2, LONG lEvent)
{
    CHAR szPath1[MAX_PATH], szPath2[MAX_PATH];

    if (pidl1)
        SHGetPathFromIDListA(pidl1, szPath1);
    else
        szPath1[0] = 0;

    if (pidl2)
        SHGetPathFromIDListA(pidl2, szPath2);
    else
        szPath2[0] = 0;

    switch (lEvent)
    {
        case SHCNE_RENAMEITEM:
            trace("SHCNE_RENAMEITEM('%s', '%s')\n", szPath1, szPath2);
            s_counters[TYPE_RENAMEITEM] = 1;
            break;
        case SHCNE_CREATE:
            trace("SHCNE_CREATE('%s', '%s')\n", szPath1, szPath2);
            s_counters[TYPE_CREATE] = 1;
            break;
        case SHCNE_DELETE:
            trace("SHCNE_DELETE('%s', '%s')\n", szPath1, szPath2);
            s_counters[TYPE_DELETE] = 1;
            break;
        case SHCNE_MKDIR:
            trace("SHCNE_MKDIR('%s', '%s')\n", szPath1, szPath2);
            s_counters[TYPE_MKDIR] = 1;
            break;
        case SHCNE_RMDIR:
            trace("SHCNE_RMDIR('%s', '%s')\n", szPath1, szPath2);
            s_counters[TYPE_RMDIR] = 1;
            break;
        case SHCNE_MEDIAINSERTED:
            trace("SHCNE_MEDIAINSERTED('%s', '%s')\n", szPath1, szPath2);
            break;
        case SHCNE_MEDIAREMOVED:
            trace("SHCNE_MEDIAREMOVED('%s', '%s')\n", szPath1, szPath2);
            break;
        case SHCNE_DRIVEREMOVED:
            trace("SHCNE_DRIVEREMOVED('%s', '%s')\n", szPath1, szPath2);
            break;
        case SHCNE_DRIVEADD:
            trace("SHCNE_DRIVEADD('%s', '%s')\n", szPath1, szPath2);
            break;
        case SHCNE_NETSHARE:
            trace("SHCNE_NETSHARE('%s', '%s')\n", szPath1, szPath2);
            break;
        case SHCNE_NETUNSHARE:
            trace("SHCNE_NETUNSHARE('%s', '%s')\n", szPath1, szPath2);
            break;
        case SHCNE_ATTRIBUTES:
            trace("SHCNE_ATTRIBUTES('%s', '%s')\n", szPath1, szPath2);
            break;
        case SHCNE_UPDATEDIR:
            trace("SHCNE_UPDATEDIR('%s', '%s')\n", szPath1, szPath2);
            s_counters[TYPE_UPDATEDIR] = 1;
            break;
        case SHCNE_UPDATEITEM:
            trace("SHCNE_UPDATEITEM('%s', '%s')\n", szPath1, szPath2);
            s_counters[TYPE_UPDATEITEM] = 1;
            break;
        case SHCNE_SERVERDISCONNECT:
            trace("SHCNE_SERVERDISCONNECT('%s', '%s')\n", szPath1, szPath2);
            break;
        case SHCNE_UPDATEIMAGE:
            trace("SHCNE_UPDATEIMAGE('%s', '%s')\n", szPath1, szPath2);
            break;
        case SHCNE_DRIVEADDGUI:
            trace("SHCNE_DRIVEADDGUI('%s', '%s')\n", szPath1, szPath2);
            break;
        case SHCNE_RENAMEFOLDER:
            trace("SHCNE_RENAMEFOLDER('%s', '%s')\n", szPath1, szPath2);
            s_counters[TYPE_RENAMEFOLDER] = 1;
            break;
        case SHCNE_FREESPACE:
            trace("SHCNE_FREESPACE('%s', '%s')\n", szPath1, szPath2);
            s_counters[TYPE_FREESPACE] = 1;
            break;
        case SHCNE_EXTENDED_EVENT:
            trace("SHCNE_EXTENDED_EVENT('%p', '%p')\n", pidl1, pidl2);
            break;
        case SHCNE_ASSOCCHANGED:
            trace("SHCNE_ASSOCCHANGED('%s', '%s')\n", szPath1, szPath2);
            break;
        default:
            trace("(lEvent:%08lX)('%s', '%s')\n", lEvent, szPath1, szPath2);
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

        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

START_TEST(SHChangeNotify)
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
    {
        skip("RegisterClassW failed\n");
        return;
    }

    HWND hwnd = CreateWindowW(s_szName, s_szName, WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
                              NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!hwnd)
    {
        skip("CreateWindowW failed\n");
        return;
    }
    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}
