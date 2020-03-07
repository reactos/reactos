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

static WCHAR s_dir1[MAX_PATH];  // "%TEMP%\\WatchDir1"
static WCHAR s_dir2[MAX_PATH];  // "%TEMP%\\WatchDir1\\Dir2"
static WCHAR s_dir3[MAX_PATH];  // "%TEMP%\\WatchDir1\\Dir3"
static WCHAR s_file1[MAX_PATH]; // "%TEMP%\\WatchDir1\\File1.txt"
static WCHAR s_file2[MAX_PATH]; // "%TEMP%\\WatchDir1\\File2.txt"

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

typedef void (*ACTION)(void);

typedef struct TEST_ENTRY
{
    INT line;
    LONG event;
    LPCVOID item1;
    LPCVOID item2;
    LPCSTR pattern;
    ACTION action;
    LPCWSTR path1;
    LPCWSTR path2;
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

static const TEST_ENTRY s_TestEntries[] = {
    {__LINE__, SHCNE_MKDIR, s_dir1, NULL, "000100000", NULL, s_dir1, L""},
    {__LINE__, SHCNE_MKDIR, s_dir2, NULL, "000100000", NULL, s_dir2, L""},
    {__LINE__, SHCNE_RMDIR, s_dir2, NULL, "000010000", NULL, s_dir2, L""},
    {__LINE__, SHCNE_MKDIR, s_dir2, NULL, "000100000", DoAction1, s_dir2, L""},
    {__LINE__, SHCNE_RMDIR, s_dir2, NULL, "000010000", NULL, s_dir2, L""},
    {__LINE__, SHCNE_RMDIR, s_dir2, NULL, "000010000", DoAction2, s_dir2, L""},
    {__LINE__, SHCNE_MKDIR, s_dir2, NULL, "000100000", DoAction1, s_dir2, L""},
    {__LINE__, SHCNE_RENAMEFOLDER, s_dir2, s_dir3, "000000010", NULL, s_dir2, s_dir3},
    {__LINE__, SHCNE_RENAMEFOLDER, s_dir2, s_dir3, "000000010", DoAction3, s_dir2, s_dir3},
    {__LINE__, SHCNE_CREATE, s_file1, NULL, "010000000", NULL, s_file1, L""},
    {__LINE__, SHCNE_CREATE, s_file1, s_file2, "010000000", NULL, s_file1, s_file2},
    {__LINE__, SHCNE_CREATE, s_file1, NULL, "010000000", DoAction4, s_file1, L""},
    {__LINE__, SHCNE_RENAMEITEM, s_file1, s_file2, "100000000", NULL, s_file1, s_file2},
    {__LINE__, SHCNE_RENAMEITEM, s_file1, s_file2, "100000000", DoAction5, s_file1, s_file2},
    {__LINE__, SHCNE_RENAMEITEM, s_file1, s_file2, "100000000", NULL, s_file1, s_file2},
    {__LINE__, SHCNE_UPDATEITEM, s_file1, NULL, "000000100", NULL, s_file1, L""},
    {__LINE__, SHCNE_UPDATEITEM, s_file2, NULL, "000000100", NULL, s_file2, L""},
    {__LINE__, SHCNE_UPDATEITEM, s_file1, s_file2, "000000100", NULL, s_file1, s_file2},
    {__LINE__, SHCNE_UPDATEITEM, s_file2, s_file1, "000000100", NULL, s_file2, s_file1},
    {__LINE__, SHCNE_DELETE, s_file1, NULL, "001000000", NULL, s_file1, L""},
    {__LINE__, SHCNE_DELETE, s_file2, NULL, "001000000", NULL, s_file2, L""},
    {__LINE__, SHCNE_DELETE, s_file2, s_file1, "001000000", NULL, s_file2, s_file1},
    {__LINE__, SHCNE_DELETE, s_file1, s_file2, "001000000", NULL, s_file1, s_file2},
    {__LINE__, SHCNE_DELETE, s_file2, NULL, "001000000", DoAction6, s_file2, L""},
    {__LINE__, SHCNE_DELETE, s_file2, NULL, "001000000", NULL, s_file2, L""},
    {__LINE__, SHCNE_DELETE, s_file1, NULL, "001000000", NULL, s_file1, L""},
    {__LINE__, SHCNE_UPDATEDIR, s_file1, NULL, "000001000", NULL, s_file1, L""},
    {__LINE__, SHCNE_UPDATEDIR, s_file2, NULL, "000001000", NULL, s_file2, L""},
    {__LINE__, SHCNE_UPDATEDIR, s_file1, s_file2, "000001000", NULL, s_file1, s_file2},
    {__LINE__, SHCNE_UPDATEDIR, s_file2, s_file1, "000001000", NULL, s_file2, s_file1},
    {__LINE__, SHCNE_UPDATEDIR, s_dir1, NULL, "000001000", NULL, s_dir1, L""},
    {__LINE__, SHCNE_UPDATEDIR, s_dir2, NULL, "000001000", NULL, s_dir2, L""},
    {__LINE__, SHCNE_UPDATEDIR, s_dir1, s_dir2, "000001000", NULL, s_dir1, s_dir2},
    {__LINE__, SHCNE_UPDATEDIR, s_dir2, s_dir1, "000001000", NULL, s_dir2, s_dir1},
    {__LINE__, SHCNE_RMDIR, s_dir1, NULL, "000010000", NULL, s_dir1, L""},
    {__LINE__, SHCNE_RMDIR, s_dir2, NULL, "000010000", NULL, s_dir2, L""},
    {__LINE__, SHCNE_RMDIR, s_dir3, NULL, "000010000", NULL, s_dir3, L""},
    {__LINE__, SHCNE_RMDIR, s_dir1, s_dir2, "000010000", NULL, s_dir1, s_dir2},
    {__LINE__, SHCNE_RMDIR, s_dir1, s_dir3, "000010000", NULL, s_dir1, s_dir3},
    {__LINE__, SHCNE_RMDIR, s_dir2, s_dir1, "000010000", NULL, s_dir2, s_dir1},
    {__LINE__, SHCNE_RMDIR, s_dir2, s_dir3, "000010000", NULL, s_dir2, s_dir3},
    {__LINE__, SHCNE_RMDIR, s_dir3, NULL, "000010000", NULL, s_dir3, L""},
    {__LINE__, SHCNE_RMDIR, s_dir3, NULL, "000010000", DoAction7, s_dir3, L""},
    {__LINE__, SHCNE_RMDIR, s_dir1, NULL, "000010000", NULL, s_dir1, L""},
    {__LINE__, SHCNE_RMDIR, s_dir1, NULL, "000010000", DoAction8, s_dir1, L""},
};

LPCSTR PatternFromFlags(DWORD flags)
{
    static char s_buf[TYPE_FREESPACE + 1 + 1];
    DWORD i;
    for (i = 0; i <= TYPE_FREESPACE; ++i)
    {
        s_buf[i] = (char)('0' + !!(flags & (1 << i)));
    }
    s_buf[i] = 0;
    return s_buf;
}

static BOOL
DoGetClipText(LPWSTR pszPath1, LPWSTR pszPath2)
{
    pszPath1[0] = pszPath2[0] = 0;

    if (!OpenClipboard(NULL) || !IsClipboardFormatAvailable(CF_UNICODETEXT))
        return FALSE;

    WCHAR szText[MAX_PATH * 2];
    HGLOBAL hGlobal = GetClipboardData(CF_UNICODETEXT);
    LPWSTR psz = (LPWSTR)GlobalLock(hGlobal);
    lstrcpynW(szText, psz, _countof(szText));
    GlobalUnlock(hGlobal);
    CloseClipboard();

    LPWSTR pch = wcschr(szText, L'|');
    if (pch == NULL)
        return FALSE;

    *pch = 0;
    lstrcpynW(pszPath1, szText, MAX_PATH);
    lstrcpynW(pszPath2, pch + 1, MAX_PATH);
    return TRUE;
}

static void
DoTestEntry(const TEST_ENTRY *entry)
{
    if (entry->action)
    {
        (*entry->action)();
    }

    SHChangeNotify(entry->event, SHCNF_PATHW | SHCNF_FLUSH, entry->item1, entry->item2);

    DWORD flags = SendMessageW(s_hwnd, WM_GET_NOTIFY_FLAGS, 0, 0);
    LPCSTR pattern = PatternFromFlags(flags);

    ok(lstrcmpA(pattern, entry->pattern) == 0, "Line %d: pattern mismatch '%s'\n", entry->line, pattern);

    SendMessageW(s_hwnd, WM_SET_PATHS, 0, 0);

    WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH];
    BOOL bOK = DoGetClipText(szPath1, szPath2);
    if (entry->path1)
        ok(bOK && lstrcmpiW(entry->path1, szPath1) == 0,
           "Line %d: path1 mismatch '%S' (%d)\n", entry->line, szPath1, bOK);
    if (entry->path2)
        ok(bOK && lstrcmpiW(entry->path2, szPath2) == 0,
           "Line %d: path2 mismatch '%S' (%d)\n", entry->line, szPath2, bOK);

    SendMessageW(s_hwnd, WM_CLEAR_FLAGS, 0, 0);
}

static BOOL
DoInit(HWND hwnd)
{
    WCHAR szTemp[MAX_PATH], szPath[MAX_PATH];

    GetTempPathW(_countof(szTemp), szTemp);
    GetLongPathNameW(szTemp, szPath, _countof(szPath));

    lstrcpyW(s_dir1, szPath);
    PathAppendW(s_dir1, L"WatchDir1");
    CreateDirectoryW(s_dir1, NULL);

    lstrcpyW(s_dir2, s_dir1);
    PathAppendW(s_dir2, L"Dir2");

    lstrcpyW(s_dir3, s_dir1);
    PathAppendW(s_dir3, L"Dir3");

    lstrcpyW(s_file1, s_dir1);
    PathAppendW(s_file1, L"File1.txt");

    lstrcpyW(s_file2, s_dir1);
    PathAppendW(s_file2, L"File2.txt");

    return TRUE;
}

static void
DoEnd(HWND hwnd)
{
    DeleteFileW(s_file1);
    DeleteFileW(s_file2);
    RemoveDirectoryW(s_dir3);
    RemoveDirectoryW(s_dir2);
    RemoveDirectoryW(s_dir1);

    SendMessageW(s_hwnd, WM_COMMAND, IDOK, 0);
}

static BOOL CALLBACK
PropEnumProcEx(HWND hwnd, LPWSTR lpszString, HANDLE hData, ULONG_PTR dwData)
{
    if (HIWORD(lpszString))
        trace("Prop: '%S' --> %p\n", lpszString, hData);
    else
        trace("Prop: '%u' --> %p\n", LOWORD(lpszString), hData);
    return TRUE;
}

static void
DoPropTest(HWND hwnd)
{
    EnumPropsExW(hwnd, PropEnumProcEx, 0);
}

START_TEST(SHChangeNotify)
{
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, _countof(szPath));
    PathRemoveFileSpecW(szPath);
    PathAppendW(szPath, L"shell-notify.exe");

    if (!PathFileExistsW(szPath))
    {
        trace("szPath: %S\n", szPath);
        PathRemoveFileSpecW(szPath);
        PathAppendW(szPath, L"testdata\\shell-notify.exe");

        if (!PathFileExistsW(szPath))
        {
            trace("szPath: %S\n", szPath);
            skip("shell-notify.exe not found\n");
            return;
        }
    }

    HINSTANCE hinst = ShellExecuteW(NULL, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)hinst <= 32)
    {
        skip("Unable to run shell-notify.exe.\n");
        return;
    }

    for (int i = 0; i < 15; ++i)
    {
        s_hwnd = FindWindowW(s_szName, s_szName);
        if (s_hwnd)
            break;

        Sleep(50);
    }

    if (!s_hwnd)
    {
        skip("Unable to find window.\n");
        return;
    }

    if (!DoInit(s_hwnd))
    {
        skip("Unable to initialize.\n");
        return;
    }

    DoPropTest(s_hwnd);

    for (size_t i = 0; i < _countof(s_TestEntries); ++i)
    {
        DoTestEntry(&s_TestEntries[i]);
    }

    DoEnd(s_hwnd);
}
