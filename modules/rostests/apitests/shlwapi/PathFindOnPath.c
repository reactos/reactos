/*
 * Copyright 2017 Katayama Hirofumi MZ
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <apitest.h>
#include <shlwapi.h>
#include <assert.h>

#define EF_FULLPATH     1
#define EF_TESTDATA     2
#define EF_WIN_DIR      4
#define EF_SYS_DIR      8
#define EF_TYPE_MASK    0xF

#define EF_NAME_ONLY    16

typedef struct ENTRY
{
    INT         EntryNumber;    /* # */
    INT         Ret;
    DWORD       Error;
    UINT        EF_;
    LPCWSTR     NameBefore;
    LPCWSTR     NameExpected;
    LPCWSTR     PathExpected;
    LPWSTR *    Dirs;
} ENTRY;

#define BEEF        0xBEEF      /* Error Code 48879 */
#define DEAD        0xDEAD      /* Error Code 57005 */
#define IGNORE_ERR  0x7F7F7F7F  /* Ignore Error Code */
#define RAISED      9999        /* exception raised */

static WCHAR    s_TestDataPath[MAX_PATH];
static LPWSTR   s_Dirs[2];
static WCHAR    s_WinDir[MAX_PATH];
static WCHAR    s_SysDir[MAX_PATH];
static WCHAR    s_FontsDir[MAX_PATH];
static WCHAR    s_NotepadPath[MAX_PATH];

static const ENTRY s_Entries[] =
{
    /* NULL or empty path */
    { 0, 0, BEEF, EF_FULLPATH, NULL, NULL },
    { 1, 1, BEEF, EF_FULLPATH, L"", s_SysDir },
    /* path without dirs in Windows */
    { 2, 0, BEEF, EF_WIN_DIR, L"", L"" },
    { 3, 0, BEEF, EF_WIN_DIR, L"Fonts", L"Fonts" },
    { 4, 0, BEEF, EF_WIN_DIR, L"notepad", L"notepad" },
    { 5, 0, BEEF, EF_WIN_DIR, L"notepad.exe", L"notepad.exe" },
    { 6, 0, BEEF, EF_WIN_DIR, L"system32", L"system32" },
    { 7, 0, BEEF, EF_WIN_DIR, L"invalid name", L"invalid name" },
    /* path with dirs in Windows */
    { 8, 0, BEEF, EF_WIN_DIR, L"", L"", NULL, s_Dirs },
    { 9, 0, BEEF, EF_WIN_DIR, L"Fonts", L"Fonts", NULL, s_Dirs },
    { 10, 0, BEEF, EF_WIN_DIR, L"notepad", L"notepad", NULL, s_Dirs },
    { 11, 0, BEEF, EF_WIN_DIR, L"notepad.exe", L"notepad.exe", NULL, s_Dirs },
    { 12, 0, BEEF, EF_WIN_DIR, L"system32", L"system32", NULL, s_Dirs },
    { 13, 0, BEEF, EF_WIN_DIR, L"invalid name", L"invalid name", NULL, s_Dirs },
    /* name only without dirs in Windows */
    { 14, 1, BEEF, EF_WIN_DIR | EF_NAME_ONLY, L"", L"system32" },
    { 15, 1, ERROR_FILE_NOT_FOUND, EF_WIN_DIR | EF_NAME_ONLY, L"Fonts", L"Fonts" },
    { 16, 0, ERROR_FILE_NOT_FOUND, EF_WIN_DIR | EF_NAME_ONLY, L"notepad" },
    { 17, 1, BEEF, EF_WIN_DIR | EF_NAME_ONLY, L"notepad.exe", NULL, s_NotepadPath },
    { 18, 1, ERROR_FILE_NOT_FOUND, EF_WIN_DIR | EF_NAME_ONLY, L"system32" },
    { 19, 0, ERROR_FILE_NOT_FOUND, EF_WIN_DIR | EF_NAME_ONLY, L"invalid name", NULL, L"invalid name" },
    /* name only with dirs in Windows */
    { 20, 1, BEEF, EF_WIN_DIR | EF_NAME_ONLY, L"", NULL, s_TestDataPath, s_Dirs },
    { 21, 1, ERROR_FILE_NOT_FOUND, EF_WIN_DIR | EF_NAME_ONLY, L"Fonts", L"Fonts", NULL, s_Dirs },
    { 22, 0, ERROR_FILE_NOT_FOUND, EF_WIN_DIR | EF_NAME_ONLY, L"notepad", NULL, L"notepad", s_Dirs },
    { 23, 1, ERROR_FILE_NOT_FOUND, EF_WIN_DIR | EF_NAME_ONLY, L"notepad.exe", NULL, s_NotepadPath, s_Dirs },
    { 24, 1, ERROR_FILE_NOT_FOUND, EF_WIN_DIR | EF_NAME_ONLY, L"system32", L"system32", NULL, s_Dirs },
    { 25, 0, ERROR_FILE_NOT_FOUND, EF_WIN_DIR | EF_NAME_ONLY, L"invalid name", NULL, L"invalid name", s_Dirs },
    /* path without dirs in system32 */
    { 26, 0, BEEF, EF_SYS_DIR, L"", L"" },
    { 27, 0, BEEF, EF_SYS_DIR, L"Fonts", L"Fonts" },
    { 28, 0, BEEF, EF_SYS_DIR, L"notepad", L"notepad" },
    { 29, 0, BEEF, EF_SYS_DIR, L"notepad.exe", L"notepad.exe" },
    { 30, 0, BEEF, EF_SYS_DIR, L"system32", L"system32" },
    { 31, 0, BEEF, EF_SYS_DIR, L"invalid name", L"invalid name" },
    /* path with dirs in system32 */
    { 32, 0, BEEF, EF_SYS_DIR, L"", L"", NULL, s_Dirs },
    { 33, 0, BEEF, EF_SYS_DIR, L"Fonts", L"Fonts", NULL, s_Dirs },
    { 34, 0, BEEF, EF_SYS_DIR, L"notepad", L"notepad", NULL, s_Dirs },
    { 35, 0, BEEF, EF_SYS_DIR, L"notepad.exe", L"notepad.exe", NULL, s_Dirs },
    { 36, 0, BEEF, EF_SYS_DIR, L"system32", L"system32", NULL, s_Dirs },
    { 37, 0, BEEF, EF_SYS_DIR, L"invalid name", L"invalid name", NULL, s_Dirs },
    /* name only without dirs in system32 */
    { 38, 1, BEEF, EF_SYS_DIR | EF_NAME_ONLY, L"", NULL, s_SysDir },
    { 39, 1, ERROR_FILE_NOT_FOUND, EF_SYS_DIR | EF_NAME_ONLY, L"Fonts", NULL, s_FontsDir },
    { 40, 0, ERROR_FILE_NOT_FOUND, EF_SYS_DIR | EF_NAME_ONLY, L"notepad", NULL, L"notepad" },
    { 41, 1, BEEF, EF_SYS_DIR | EF_NAME_ONLY, L"notepad.exe", L"notepad.exe" },
    { 42, 1, ERROR_FILE_NOT_FOUND, EF_SYS_DIR | EF_NAME_ONLY, L"system32", NULL, s_SysDir },
    { 43, 0, ERROR_FILE_NOT_FOUND, EF_SYS_DIR | EF_NAME_ONLY, L"invalid name", NULL, L"invalid name" },
    /* name only with dirs in system32 */
    { 44, 1, BEEF, EF_SYS_DIR | EF_NAME_ONLY, L"", NULL, s_TestDataPath, s_Dirs },
    { 45, 1, ERROR_FILE_NOT_FOUND, EF_SYS_DIR | EF_NAME_ONLY, L"Fonts", NULL, s_FontsDir, s_Dirs },
    { 46, 0, ERROR_FILE_NOT_FOUND, EF_SYS_DIR | EF_NAME_ONLY, L"notepad", NULL, L"notepad", s_Dirs },
    { 47, 1, ERROR_FILE_NOT_FOUND, EF_SYS_DIR | EF_NAME_ONLY, L"notepad.exe", L"notepad.exe", NULL, s_Dirs },
    { 48, 1, ERROR_FILE_NOT_FOUND, EF_SYS_DIR | EF_NAME_ONLY, L"system32", NULL, s_SysDir, s_Dirs },
    { 49, 0, ERROR_FILE_NOT_FOUND, EF_SYS_DIR | EF_NAME_ONLY, L"invalid name", NULL, L"invalid name", s_Dirs },
    /* path without dirs in testdata dir */
    { 50, 0, BEEF, EF_TESTDATA, L"", L"" },
    { 51, 0, BEEF, EF_TESTDATA, L"Fonts", L"Fonts" },
    { 52, 0, BEEF, EF_TESTDATA, L"notepad", L"notepad" },
    { 53, 0, BEEF, EF_TESTDATA, L"notepad.exe", L"notepad.exe" },
    { 54, 0, BEEF, EF_TESTDATA, L"system32", L"system32" },
    { 55, 0, BEEF, EF_TESTDATA, L"invalid name", L"invalid name" },
    { 56, 0, BEEF, EF_TESTDATA, L"README.txt", L"README.txt" },
    { 57, 0, BEEF, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils" },
    { 58, 0, BEEF, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe" },
    /* path with dirs in testdata dir */
    { 59, 0, BEEF, EF_TESTDATA, L"", L"", NULL, s_Dirs },
    { 60, 0, BEEF, EF_TESTDATA, L"Fonts", L"Fonts", NULL, s_Dirs },
    { 61, 0, BEEF, EF_TESTDATA, L"notepad", L"notepad", NULL, s_Dirs },
    { 62, 0, BEEF, EF_TESTDATA, L"notepad.exe", L"notepad.exe", NULL, s_Dirs },
    { 63, 0, BEEF, EF_TESTDATA, L"system32", L"system32", NULL, s_Dirs },
    { 64, 0, BEEF, EF_TESTDATA, L"invalid name", L"invalid name", NULL, s_Dirs },
    { 65, 0, BEEF, EF_TESTDATA, L"README.txt", L"README.txt", NULL, s_Dirs },
    { 66, 0, BEEF, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", NULL, s_Dirs },
    { 67, 0, BEEF, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe", NULL, s_Dirs },
    /* name only without dirs in testdata dir */
    { 68, 1, BEEF, EF_TESTDATA | EF_NAME_ONLY, L"", NULL, s_SysDir },
    { 69, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"Fonts", NULL, s_FontsDir },
    { 70, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"notepad", NULL, L"notepad" },
    { 71, 1, BEEF, EF_TESTDATA | EF_NAME_ONLY, L"notepad.exe", NULL, s_NotepadPath },
    { 72, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"system32", NULL, s_SysDir },
    { 73, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"invalid name", NULL, L"invalid name" },
    { 74, 1, BEEF, EF_TESTDATA | EF_NAME_ONLY, L"README.txt", L"README.txt", NULL, s_Dirs },
    { 75, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, L"CmdLineUtils", s_Dirs },
    { 76, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, L"CmdLineUtils.exe", s_Dirs },
    /* name only with dirs in testdata dir */
    { 77, 1, BEEF, EF_TESTDATA | EF_NAME_ONLY, L"", NULL, s_TestDataPath, s_Dirs },
    { 78, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"Fonts", NULL, s_FontsDir, s_Dirs },
    { 79, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"notepad", NULL, L"notepad", s_Dirs },
    { 80, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"notepad.exe", NULL, s_NotepadPath, s_Dirs },
    { 81, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"system32", NULL, s_SysDir, s_Dirs },
    { 82, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"invalid name", NULL, L"invalid name", s_Dirs },
    { 83, 1, BEEF, EF_TESTDATA | EF_NAME_ONLY, L"README.txt", L"README.txt", NULL, s_Dirs },
    { 84, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, L"CmdLineUtils", s_Dirs },
    { 85, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, L"CmdLineUtils.exe", s_Dirs },
};

static void DoEntry(INT EntryNumber, const ENTRY *pEntry, const WCHAR *PathVar)
{
    WCHAR Path[MAX_PATH], PathExpected[MAX_PATH], OldPathVar[256];
    INT Ret;
    DWORD Error;

    if (pEntry->NameBefore == NULL)
    {
        assert(pEntry->NameExpected == NULL);
        assert(pEntry->PathExpected == NULL);
    }

    switch (pEntry->EF_ & EF_TYPE_MASK)
    {
    case EF_FULLPATH:
        if (pEntry->NameBefore)
        {
            lstrcpyW(Path, pEntry->NameBefore);
        }
        if (pEntry->NameExpected)
        {
            lstrcpyW(PathExpected, pEntry->NameExpected);
        }
        break;

    case EF_TESTDATA:
        if (pEntry->EF_ & EF_NAME_ONLY)
        {
            lstrcpyW(Path, pEntry->NameBefore);
        }
        else
        {
            lstrcpyW(Path, s_TestDataPath);
            lstrcatW(Path, L"\\");
            lstrcatW(Path, pEntry->NameBefore);
        }

        if (pEntry->NameExpected)
        {
            lstrcpyW(PathExpected, s_TestDataPath);
            lstrcatW(PathExpected, L"\\");
            lstrcatW(PathExpected, pEntry->NameExpected);
        }
        break;

    case EF_WIN_DIR:
        if (pEntry->EF_ & EF_NAME_ONLY)
        {
            lstrcpyW(Path, pEntry->NameBefore);
        }
        else
        {
            GetWindowsDirectoryW(Path, _countof(Path));
            lstrcatW(Path, L"\\");
            lstrcatW(Path, pEntry->NameBefore);
        }

        if (pEntry->NameExpected)
        {
            GetWindowsDirectoryW(PathExpected, _countof(PathExpected));
            lstrcatW(PathExpected, L"\\");
            lstrcatW(PathExpected, pEntry->NameExpected);
        }
        break;

    case EF_SYS_DIR:
        if (pEntry->EF_ & EF_NAME_ONLY)
        {
            lstrcpyW(Path, pEntry->NameBefore);
        }
        else
        {
            GetSystemDirectoryW(Path, _countof(Path));
            lstrcatW(Path, L"\\");
            lstrcatW(Path, pEntry->NameBefore);
        }

        if (pEntry->NameExpected)
        {
            GetSystemDirectoryW(PathExpected, _countof(PathExpected));
            lstrcatW(PathExpected, L"\\");
            lstrcatW(PathExpected, pEntry->NameExpected);
        }
        break;
    }

    if (PathVar)
    {
        if (!GetEnvironmentVariableW(L"PATH", OldPathVar, _countof(OldPathVar)))
        {
            skip("#%d: GetEnvironmentVariableW failed\n", EntryNumber);
            return;
        }
        if (!SetEnvironmentVariableW(L"PATH", PathVar))
        {
            skip("#%d: SetEnvironmentVariableW failed\n", EntryNumber);
            return;
        }
    }

    _SEH2_TRY
    {
        SetLastError(BEEF);
        if (pEntry->NameBefore)
        {
            Ret = PathFindOnPathW(Path, (LPCWSTR *)pEntry->Dirs);
        }
        else
        {
            Ret = PathFindOnPathW(NULL, (LPCWSTR *)pEntry->Dirs);
        }
        Error = GetLastError();
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Ret = RAISED;
        Error = DEAD;
    }
    _SEH2_END;

    if (PathVar)
    {
        ok(SetEnvironmentVariableW(L"PATH", OldPathVar),
           "#%d: SetEnvironmentVariableW failed\n", EntryNumber);
    }

    ok(Ret == pEntry->Ret, "#%d: Ret expected %d, was %d.\n",
       EntryNumber, pEntry->Ret, Ret);

    if (pEntry->Error != IGNORE_ERR)
    {
        ok(Error == pEntry->Error, "#%d: last error expected %ld, was %ld.\n",
           EntryNumber, pEntry->Error, Error);
    }

    if (pEntry->PathExpected)
    {
        ok(lstrcmpiW(Path, pEntry->PathExpected) == 0, "#%d: Path expected %s, was %s.\n",
           EntryNumber, wine_dbgstr_w(pEntry->PathExpected), wine_dbgstr_w(Path));
    }
    else if (pEntry->NameExpected)
    {
        ok(lstrcmpiW(Path, PathExpected) == 0, "#%d: Path expected %s, was %s.\n",
           EntryNumber, wine_dbgstr_w(PathExpected), wine_dbgstr_w(Path));
    }
}

static void Test_PathFindOnPathW(void)
{
    UINT i;

    for (i = 0; i < _countof(s_Entries); ++i)
    {
        DoEntry(s_Entries[i].EntryNumber, &s_Entries[i], NULL);
    }
}

START_TEST(PathFindOnPath)
{
    LPWSTR pch;

    GetWindowsDirectoryW(s_WinDir, _countof(s_WinDir));
    GetSystemDirectoryW(s_SysDir, _countof(s_SysDir));

    lstrcpyW(s_FontsDir, s_WinDir);
    lstrcatW(s_FontsDir, L"\\Fonts");

    lstrcpyW(s_NotepadPath, s_SysDir);
    lstrcatW(s_NotepadPath, L"\\notepad.exe");

    GetModuleFileNameW(NULL, s_TestDataPath, _countof(s_TestDataPath));
    pch = wcsrchr(s_TestDataPath, L'\\');
    if (pch == NULL)
        pch = wcsrchr(s_TestDataPath, L'/');
    if (pch == NULL)
    {
        skip("GetModuleFileName and/or wcsrchr are insane.\n");
        return;
    }
    lstrcpyW(pch, L"\\testdata");
    if (GetFileAttributesW(s_TestDataPath) == INVALID_FILE_ATTRIBUTES)
    {
        skip("testdata is not found.\n");
        return;
    }

    s_Dirs[0] = s_TestDataPath;
    s_Dirs[1] = NULL;

    Test_PathFindOnPathW();
}
