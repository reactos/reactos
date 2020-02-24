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

#define ID_TEST 1000

static WCHAR s_dir1[MAX_PATH];    // "%TEMP%\\WatchDir1"
static WCHAR s_dir2[MAX_PATH];    // "%TEMP%\\WatchDir1\\Dir2"
static WCHAR s_dir3[MAX_PATH];    // "%TEMP%\\WatchDir1\\Dir3"
static WCHAR s_file1[MAX_PATH];   // "%TEMP%\\WatchDir1\\File1.txt"
static WCHAR s_file2[MAX_PATH];   // "%TEMP%\\WatchDir1\\File2.txt"

static HWND s_hwnd = NULL;
static WCHAR s_szName[] = L"SHChangeNotify testcase";
static LPITEMIDLIST s_pidl = NULL;
static UINT s_uRegID = 0;
static SHChangeNotifyEntry s_entry;

static CHAR s_path1[MAX_PATH], s_path2[MAX_PATH];

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

static LPCSTR
DoGetPattern(void)
{
    size_t i;
    static char buf[TYPE_FREESPACE + 1 + 1];
    for (i = 0; i < TYPE_FREESPACE + 1; ++i)
    {
        buf[i] = (char)('0' + s_counters[i]);
    }
    buf[i] = 0;
    return buf;
}

typedef void (*ACTION)(void);

typedef struct TEST_ENTRY
{
    INT line;
    LONG event;
    LPCVOID item1;
    LPCVOID item2;
    LPCSTR pattern;
    ACTION action;
} TEST_ENTRY;

static BOOL
DoCreateEmptyFile(LPCWSTR pszFileName)
{
    FILE *fp = _wfopen(pszFileName, L"wb");
    fclose(fp);
    return fp != NULL;
}

static void
DoAction1(void)
{
    ok_int(CreateDirectoryW(s_dir2, NULL), TRUE);
}

static void
DoAction2(void)
{
    ok_int(RemoveDirectoryW(s_dir2), TRUE);
}

static void
DoAction3(void)
{
    ok_int(MoveFileExW(s_dir2, s_dir3, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING), TRUE);
}

static void
DoAction4(void)
{
    ok_int(DoCreateEmptyFile(s_file1), TRUE);
}

static void
DoAction5(void)
{
    ok_int(MoveFileExW(s_file1, s_file2, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING), TRUE);
}

static void
DoAction6(void)
{
    ok_int(DeleteFileW(s_file2), TRUE);
}

static void
DoAction7(void)
{
    DeleteFileW(s_file1);
    DeleteFileW(s_file2);
    ok_int(RemoveDirectoryW(s_dir3), TRUE);
}

static void
DoAction8(void)
{
    ok_int(RemoveDirectoryW(s_dir1), TRUE);
}

static const TEST_ENTRY s_TestEntries[] =
{
    {__LINE__, SHCNE_MKDIR, s_dir1, NULL, "000100000", NULL},
    {__LINE__, SHCNE_MKDIR, s_dir2, NULL, "000100000", NULL},
    {__LINE__, SHCNE_RMDIR, s_dir2, NULL, "000010000", NULL},
    {__LINE__, SHCNE_MKDIR, s_dir2, NULL, "000100000", DoAction1},
    {__LINE__, SHCNE_RMDIR, s_dir2, NULL, "000010000", NULL},
    {__LINE__, SHCNE_RMDIR, s_dir2, NULL, "000010000", DoAction2},
    {__LINE__, SHCNE_MKDIR, s_dir2, NULL, "000100000", DoAction1},
    {__LINE__, SHCNE_RENAMEFOLDER, s_dir2, s_dir3, "000000010", NULL},
    {__LINE__, SHCNE_RENAMEFOLDER, s_dir2, NULL, "000000010", NULL},
    {__LINE__, SHCNE_RENAMEFOLDER, s_dir2, s_dir3, "000000010", DoAction3},
    {__LINE__, SHCNE_RENAMEFOLDER, s_dir2, NULL, "000000010", NULL},
    {__LINE__, SHCNE_CREATE, s_file1, NULL, "010000000", NULL},
    {__LINE__, SHCNE_CREATE, s_file1, s_file2, "010000000", NULL},
    {__LINE__, SHCNE_CREATE, s_file1, NULL, "010000000", DoAction4},
    {__LINE__, SHCNE_RENAMEITEM, s_file1, NULL, "100000000", NULL},
    {__LINE__, SHCNE_RENAMEITEM, s_file1, s_file2, "100000000", NULL},
    {__LINE__, SHCNE_RENAMEITEM, s_file1, s_file2, "100000000", DoAction5},
    {__LINE__, SHCNE_RENAMEITEM, s_file1, s_file2, "100000000", NULL},
    {__LINE__, SHCNE_UPDATEITEM, s_file1, NULL, "000000100", NULL},
    {__LINE__, SHCNE_UPDATEITEM, s_file2, NULL, "000000100", NULL},
    {__LINE__, SHCNE_UPDATEITEM, s_file1, s_file2, "000000100", NULL},
    {__LINE__, SHCNE_UPDATEITEM, s_file2, s_file1, "000000100", NULL},
    {__LINE__, SHCNE_DELETE, s_file1, NULL, "001000000", NULL},
    {__LINE__, SHCNE_DELETE, s_file2, NULL, "001000000", NULL},
    {__LINE__, SHCNE_DELETE, s_file2, s_file1, "001000000", NULL},
    {__LINE__, SHCNE_DELETE, s_file1, s_file2, "001000000", NULL},
    {__LINE__, SHCNE_DELETE, s_file2, NULL, "001000000", DoAction6},
    {__LINE__, SHCNE_DELETE, s_file2, NULL, "001000000", NULL},
    {__LINE__, SHCNE_DELETE, s_file1, NULL, "001000000", NULL},
    {__LINE__, SHCNE_UPDATEDIR, s_file1, NULL, "000001000", NULL},
    {__LINE__, SHCNE_UPDATEDIR, s_file2, NULL, "000001000", NULL},
    {__LINE__, SHCNE_UPDATEDIR, s_file1, s_file2, "000001000", NULL},
    {__LINE__, SHCNE_UPDATEDIR, s_file2, s_file1, "000001000", NULL},
    {__LINE__, SHCNE_UPDATEDIR, s_dir1, NULL, "000001000", NULL},
    {__LINE__, SHCNE_UPDATEDIR, s_dir2, NULL, "000001000", NULL},
    {__LINE__, SHCNE_UPDATEDIR, s_dir1, s_dir2, "000001000", NULL},
    {__LINE__, SHCNE_UPDATEDIR, s_dir2, s_dir1, "000001000", NULL},
    {__LINE__, SHCNE_RMDIR, s_dir1, NULL, "000010000", NULL},
    {__LINE__, SHCNE_RMDIR, s_dir2, NULL, "000010000", NULL},
    {__LINE__, SHCNE_RMDIR, s_dir3, NULL, "000010000", NULL},
    {__LINE__, SHCNE_RMDIR, s_dir1, s_dir2, "000010000", NULL},
    {__LINE__, SHCNE_RMDIR, s_dir1, s_dir3, "000010000", NULL},
    {__LINE__, SHCNE_RMDIR, s_dir2, s_dir1, "000010000", NULL},
    {__LINE__, SHCNE_RMDIR, s_dir2, s_dir3, "000010000", NULL},
    {__LINE__, SHCNE_RMDIR, s_dir3, NULL, "000010000", NULL},
    {__LINE__, SHCNE_RMDIR, s_dir3, NULL, "000010000", DoAction7},
    {__LINE__, SHCNE_RMDIR, s_dir1, NULL, "000010000", NULL},
    {__LINE__, SHCNE_RMDIR, s_dir1, NULL, "000010000", DoAction8},
};
static const size_t s_nTestEntries = _countof(s_TestEntries);
static size_t s_iTest = 0;

static void
DoTestEntry1(const TEST_ENTRY *entry)
{
    if (entry->action)
    {
        (*entry->action)();
    }

    SHChangeNotify(entry->event, SHCNF_PATHW | SHCNF_FLUSH, entry->item1, entry->item2);
    SendMessageW(s_hwnd, WM_COMMAND, ID_TEST + s_iTest, 0);

    ZeroMemory(&s_counters, sizeof(s_counters));
}

static void
DoTestEntry2(const TEST_ENTRY *entry)
{
    LPCSTR pattern = DoGetPattern();
    ok(lstrcmpA(pattern, entry->pattern) == 0,
       "Line %d: pattern mismatch '%s'\n", entry->line, pattern);
}

static BOOL
DoInit(HWND hwnd)
{
    WCHAR szTemp[MAX_PATH], szPath[MAX_PATH];

    GetTempPathW(_countof(szTemp), szTemp);
    GetLongPathNameW(szTemp, szPath, _countof(szPath));

    lstrcpyW(s_dir1, szPath);
    PathAddBackslashW(s_dir1);
    lstrcatW(s_dir1, L"WatchDir1");
    CreateDirectoryW(s_dir1, NULL);
    //trace("s_dir1: %S\n", s_dir1);

    lstrcpyW(s_dir2, s_dir1);
    PathAddBackslashW(s_dir2);
    lstrcatW(s_dir2, L"Dir2");
    //trace("s_dir2: %S\n", s_dir2);

    lstrcpyW(s_dir3, s_dir1);
    PathAddBackslashW(s_dir3);
    lstrcatW(s_dir3, L"Dir3");
    //trace("s_dir3: %S\n", s_dir3);

    lstrcpyW(s_file1, s_dir1);
    PathAddBackslashW(s_file1);
    lstrcatW(s_file1, L"File1.txt");
    //trace("s_file1: %S\n", s_file1);

    lstrcpyW(s_file2, s_dir1);
    PathAddBackslashW(s_file2);
    lstrcatW(s_file2, L"File2.txt");
    //trace("s_file2: %S\n", s_file2);

    s_pidl = ILCreateFromPathW(s_dir1);

    s_entry.pidl = s_pidl;
    s_entry.fRecursive = TRUE;
    LONG fEvents = SHCNE_ALLEVENTS;
    s_uRegID = SHChangeNotifyRegister(hwnd, SHCNRF_ShellLevel/* | SHCNRF_NewDelivery*/,
                                      fEvents, WM_SHELL_NOTIFY, 1, &s_entry);
    return s_uRegID != 0;
}

static DWORD WINAPI
ThreadFunc(LPVOID)
{
    for (size_t i = 0; i < s_nTestEntries; ++i)
    {
        s_iTest = i;
        DoTestEntry1(&s_TestEntries[i]);
    }

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
        default:
            if (ID_TEST <= id && id < ID_TEST + 1000)
            {
                DoTestEntry2(&s_TestEntries[s_iTest]);
            }
            break;
    }
}

static void
OnDestroy(HWND hwnd)
{
    SHChangeNotifyDeregister(s_uRegID);
    CoTaskMemFree(s_pidl);
    DeleteFileW(s_file1);
    DeleteFileW(s_file2);
    RemoveDirectoryW(s_dir3);
    RemoveDirectoryW(s_dir2);
    RemoveDirectoryW(s_dir1);
    PostQuitMessage(0);
    s_hwnd = NULL;
}

static void
DoShellNotify(HWND hwnd, PIDLIST_ABSOLUTE pidl1, PIDLIST_ABSOLUTE pidl2, LONG lEvent)
{
    if (pidl1)
        SHGetPathFromIDListA(pidl1, s_path1);
    else
        s_path1[0] = 0;

    if (pidl2)
        SHGetPathFromIDListA(pidl2, s_path2);
    else
        s_path2[0] = 0;

    switch (lEvent)
    {
        case SHCNE_RENAMEITEM:
            trace("SHCNE_RENAMEITEM('%s', '%s')\n", s_path1, s_path2);
            s_counters[TYPE_RENAMEITEM] = 1;
            break;
        case SHCNE_CREATE:
            trace("SHCNE_CREATE('%s', '%s')\n", s_path1, s_path2);
            s_counters[TYPE_CREATE] = 1;
            break;
        case SHCNE_DELETE:
            trace("SHCNE_DELETE('%s', '%s')\n", s_path1, s_path2);
            s_counters[TYPE_DELETE] = 1;
            break;
        case SHCNE_MKDIR:
            trace("SHCNE_MKDIR('%s', '%s')\n", s_path1, s_path2);
            s_counters[TYPE_MKDIR] = 1;
            break;
        case SHCNE_RMDIR:
            trace("SHCNE_RMDIR('%s', '%s')\n", s_path1, s_path2);
            s_counters[TYPE_RMDIR] = 1;
            break;
        case SHCNE_MEDIAINSERTED:
            trace("SHCNE_MEDIAINSERTED('%s', '%s')\n", s_path1, s_path2);
            break;
        case SHCNE_MEDIAREMOVED:
            trace("SHCNE_MEDIAREMOVED('%s', '%s')\n", s_path1, s_path2);
            break;
        case SHCNE_DRIVEREMOVED:
            trace("SHCNE_DRIVEREMOVED('%s', '%s')\n", s_path1, s_path2);
            break;
        case SHCNE_DRIVEADD:
            trace("SHCNE_DRIVEADD('%s', '%s')\n", s_path1, s_path2);
            break;
        case SHCNE_NETSHARE:
            trace("SHCNE_NETSHARE('%s', '%s')\n", s_path1, s_path2);
            break;
        case SHCNE_NETUNSHARE:
            trace("SHCNE_NETUNSHARE('%s', '%s')\n", s_path1, s_path2);
            break;
        case SHCNE_ATTRIBUTES:
            trace("SHCNE_ATTRIBUTES('%s', '%s')\n", s_path1, s_path2);
            break;
        case SHCNE_UPDATEDIR:
            trace("SHCNE_UPDATEDIR('%s', '%s')\n", s_path1, s_path2);
            s_counters[TYPE_UPDATEDIR] = 1;
            break;
        case SHCNE_UPDATEITEM:
            trace("SHCNE_UPDATEITEM('%s', '%s')\n", s_path1, s_path2);
            s_counters[TYPE_UPDATEITEM] = 1;
            break;
        case SHCNE_SERVERDISCONNECT:
            trace("SHCNE_SERVERDISCONNECT('%s', '%s')\n", s_path1, s_path2);
            break;
        case SHCNE_UPDATEIMAGE:
            trace("SHCNE_UPDATEIMAGE('%s', '%s')\n", s_path1, s_path2);
            break;
        case SHCNE_DRIVEADDGUI:
            trace("SHCNE_DRIVEADDGUI('%s', '%s')\n", s_path1, s_path2);
            break;
        case SHCNE_RENAMEFOLDER:
            trace("SHCNE_RENAMEFOLDER('%s', '%s')\n", s_path1, s_path2);
            s_counters[TYPE_RENAMEFOLDER] = 1;
            break;
        case SHCNE_FREESPACE:
            trace("SHCNE_FREESPACE('%s', '%s')\n", s_path1, s_path2);
            s_counters[TYPE_FREESPACE] = 1;
            break;
        case SHCNE_EXTENDED_EVENT:
            trace("SHCNE_EXTENDED_EVENT('%p', '%p')\n", pidl1, pidl2);
            break;
        case SHCNE_ASSOCCHANGED:
            trace("SHCNE_ASSOCCHANGED('%s', '%s')\n", s_path1, s_path2);
            break;
        default:
            trace("(lEvent:%08lX)('%s', '%s')\n", lEvent, s_path1, s_path2);
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
