/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for FindExecutable
 * COPYRIGHT:   Copyright 2021-2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <stdio.h>
#include <shlwapi.h>

typedef struct TEST_ENTRY
{
    INT lineno;
    UINT ret; // SE_ERR_NOASSOC, SE_ERR_FNF, or 0xBEEFCAFE (success).
    LPCSTR file;
    LPCSTR dir;
    LPCSTR result;
} TEST_ENTRY;

static char s_win_dir[MAX_PATH];
static char s_win_notepad[MAX_PATH];
static char s_sys_notepad[MAX_PATH];
static char s_win_test_exe[MAX_PATH];
static char s_sys_test_exe[MAX_PATH];
static char s_win_bat_file[MAX_PATH];
static char s_sys_bat_file[MAX_PATH];
static char s_win_txt_file[MAX_PATH];
static char s_sys_txt_file[MAX_PATH];

#define DIR_0 NULL
#define DIR_1 "."
#define DIR_2 ".."
#define DIR_3 s_win_dir
#define DIR_4 "(invalid)"

static const TEST_ENTRY s_entries[] =
{
    { __LINE__, 0xBEEFCAFE, "notepad", DIR_0, s_sys_notepad },
    { __LINE__, 0xBEEFCAFE, "notepad", DIR_1, s_sys_notepad },
    { __LINE__, 0xBEEFCAFE, "notepad", DIR_2, s_sys_notepad },
    { __LINE__, 0xBEEFCAFE, "notepad", DIR_3, s_win_notepad },
    { __LINE__, 0xBEEFCAFE, "notepad", DIR_4, s_sys_notepad },
    { __LINE__, SE_ERR_FNF, "  notepad", DIR_0, "" },
    { __LINE__, SE_ERR_FNF, "  notepad", DIR_1, "" },
    { __LINE__, SE_ERR_FNF, "  notepad", DIR_2, "" },
    { __LINE__, SE_ERR_FNF, "  notepad", DIR_3, "" },
    { __LINE__, SE_ERR_FNF, "  notepad", DIR_4, "" },
    { __LINE__, SE_ERR_FNF, "notepad  ", DIR_0, "" },
    { __LINE__, SE_ERR_FNF, "notepad  ", DIR_1, "" },
    { __LINE__, SE_ERR_FNF, "notepad  ", DIR_2, "" },
    { __LINE__, SE_ERR_FNF, "notepad  ", DIR_3, "" },
    { __LINE__, SE_ERR_FNF, "notepad  ", DIR_4, "" },
    { __LINE__, 0xBEEFCAFE, "\"notepad\"", DIR_0, s_sys_notepad },
    { __LINE__, 0xBEEFCAFE, "\"notepad\"", DIR_1, s_sys_notepad },
    { __LINE__, 0xBEEFCAFE, "\"notepad\"", DIR_2, s_sys_notepad },
    { __LINE__, 0xBEEFCAFE, "\"notepad\"", DIR_3, s_win_notepad },
    { __LINE__, 0xBEEFCAFE, "\"notepad\"", DIR_4, s_sys_notepad },
    { __LINE__, 0xBEEFCAFE, "notepad.exe", DIR_0, s_sys_notepad },
    { __LINE__, 0xBEEFCAFE, "notepad.exe", DIR_1, s_sys_notepad },
    { __LINE__, 0xBEEFCAFE, "notepad.exe", DIR_2, s_sys_notepad },
    { __LINE__, 0xBEEFCAFE, "notepad.exe", DIR_3, s_win_notepad },
    { __LINE__, 0xBEEFCAFE, "notepad.exe", DIR_4, s_sys_notepad },
    { __LINE__, SE_ERR_FNF, "notepad.bat", DIR_0, "" },
    { __LINE__, SE_ERR_FNF, "notepad.bat", DIR_1, "" },
    { __LINE__, SE_ERR_FNF, "notepad.bat", DIR_2, "" },
    { __LINE__, SE_ERR_FNF, "notepad.bat", DIR_3, "" },
    { __LINE__, SE_ERR_FNF, "notepad.bat", DIR_4, "" },
    { __LINE__, SE_ERR_FNF, "C:\\notepad.exe", DIR_0, "" },
    { __LINE__, SE_ERR_FNF, "C:\\notepad.exe", DIR_1, "" },
    { __LINE__, SE_ERR_FNF, "C:\\notepad.exe", DIR_2, "" },
    { __LINE__, SE_ERR_FNF, "C:\\notepad.exe", DIR_3, "" },
    { __LINE__, SE_ERR_FNF, "C:\\notepad.exe", DIR_4, "" },
    { __LINE__, SE_ERR_FNF, ".\\notepad.exe", DIR_0, "" },
    { __LINE__, SE_ERR_FNF, ".\\notepad.exe", DIR_1, "" },
    { __LINE__, SE_ERR_FNF, ".\\notepad.exe", DIR_2, "" },
    { __LINE__, 0xBEEFCAFE, ".\\notepad.exe", DIR_3, s_win_notepad },
    { __LINE__, SE_ERR_FNF, ".\\notepad.exe", DIR_4, "" },
    { __LINE__, SE_ERR_FNF, "system32\\notepad.exe", DIR_0, "" },
    { __LINE__, SE_ERR_FNF, "system32\\notepad.exe", DIR_1, "" },
    { __LINE__, SE_ERR_FNF, "system32\\notepad.exe", DIR_2, "" },
    { __LINE__, 0xBEEFCAFE, "system32\\notepad.exe", DIR_3, s_sys_notepad },
    { __LINE__, SE_ERR_FNF, "system32\\notepad.exe", DIR_4, "" },
    { __LINE__, 0xBEEFCAFE, s_win_notepad, DIR_0, s_win_notepad },
    { __LINE__, 0xBEEFCAFE, s_win_notepad, DIR_1, s_win_notepad },
    { __LINE__, 0xBEEFCAFE, s_win_notepad, DIR_2, s_win_notepad },
    { __LINE__, 0xBEEFCAFE, s_win_notepad, DIR_3, s_win_notepad },
    { __LINE__, 0xBEEFCAFE, s_win_notepad, DIR_4, s_win_notepad },
    { __LINE__, 0xBEEFCAFE, "test program", DIR_0, s_sys_test_exe },
    { __LINE__, 0xBEEFCAFE, "test program", DIR_1, s_sys_test_exe },
    { __LINE__, 0xBEEFCAFE, "test program", DIR_2, s_sys_test_exe },
    { __LINE__, 0xBEEFCAFE, "test program", DIR_3, s_win_test_exe },
    { __LINE__, 0xBEEFCAFE, "test program", DIR_4, s_sys_test_exe },
    { __LINE__, SE_ERR_FNF, "  test program", DIR_0, "" },
    { __LINE__, SE_ERR_FNF, "  test program", DIR_1, "" },
    { __LINE__, SE_ERR_FNF, "  test program", DIR_2, "" },
    { __LINE__, SE_ERR_FNF, "  test program", DIR_3, "" },
    { __LINE__, SE_ERR_FNF, "  test program", DIR_4, "" },
    { __LINE__, SE_ERR_FNF, "test program  ", DIR_0, "" },
    { __LINE__, SE_ERR_FNF, "test program  ", DIR_1, "" },
    { __LINE__, SE_ERR_FNF, "test program  ", DIR_2, "" },
    { __LINE__, SE_ERR_FNF, "test program  ", DIR_3, "" },
    { __LINE__, SE_ERR_FNF, "test program  ", DIR_4, "" },
    { __LINE__, 0xBEEFCAFE, "\"test program\"", DIR_0, s_sys_test_exe },
    { __LINE__, 0xBEEFCAFE, "\"test program\"", DIR_1, s_sys_test_exe },
    { __LINE__, 0xBEEFCAFE, "\"test program\"", DIR_2, s_sys_test_exe },
    { __LINE__, 0xBEEFCAFE, "\"test program\"", DIR_3, s_win_test_exe },
    { __LINE__, 0xBEEFCAFE, "\"test program\"", DIR_4, s_sys_test_exe },
    { __LINE__, 0xBEEFCAFE, "test program.exe", DIR_0, s_sys_test_exe },
    { __LINE__, 0xBEEFCAFE, "test program.exe", DIR_1, s_sys_test_exe },
    { __LINE__, 0xBEEFCAFE, "test program.exe", DIR_2, s_sys_test_exe },
    { __LINE__, 0xBEEFCAFE, "test program.exe", DIR_3, s_win_test_exe },
    { __LINE__, 0xBEEFCAFE, "test program.exe", DIR_4, s_sys_test_exe },
    { __LINE__, 0xBEEFCAFE, "\"test program.exe\"", DIR_0, s_sys_test_exe },
    { __LINE__, 0xBEEFCAFE, "\"test program.exe\"", DIR_1, s_sys_test_exe },
    { __LINE__, 0xBEEFCAFE, "\"test program.exe\"", DIR_2, s_sys_test_exe },
    { __LINE__, 0xBEEFCAFE, "\"test program.exe\"", DIR_3, s_win_test_exe },
    { __LINE__, 0xBEEFCAFE, "\"test program.exe\"", DIR_4, s_sys_test_exe },
    { __LINE__, SE_ERR_NOASSOC, "\"test program.exe \"", DIR_0, "" },
    { __LINE__, SE_ERR_NOASSOC, "\"test program.exe \"", DIR_1, "" },
    { __LINE__, SE_ERR_NOASSOC, "\"test program.exe \"", DIR_2, "" },
    { __LINE__, SE_ERR_NOASSOC, "\"test program.exe \"", DIR_3, "" },
    { __LINE__, SE_ERR_NOASSOC, "\"test program.exe \"", DIR_4, "" },
    { __LINE__, SE_ERR_FNF, "\" test program.exe\"", DIR_0, "" },
    { __LINE__, SE_ERR_FNF, "\" test program.exe\"", DIR_1, "" },
    { __LINE__, SE_ERR_FNF, "\" test program.exe\"", DIR_2, "" },
    { __LINE__, SE_ERR_FNF, "\" test program.exe\"", DIR_3, "" },
    { __LINE__, SE_ERR_FNF, "\" test program.exe\"", DIR_4, "" },
    { __LINE__, 0xBEEFCAFE, "test program.bat", DIR_0, s_sys_bat_file },
    { __LINE__, 0xBEEFCAFE, "test program.bat", DIR_1, s_sys_bat_file },
    { __LINE__, 0xBEEFCAFE, "test program.bat", DIR_2, s_sys_bat_file },
    { __LINE__, 0xBEEFCAFE, "test program.bat", DIR_3, s_win_bat_file },
    { __LINE__, 0xBEEFCAFE, "test program.bat", DIR_4, s_sys_bat_file },
    { __LINE__, SE_ERR_FNF, "  test program.bat  ", DIR_0, "" },
    { __LINE__, SE_ERR_FNF, "  test program.bat  ", DIR_1, "" },
    { __LINE__, SE_ERR_FNF, "  test program.bat  ", DIR_2, "" },
    { __LINE__, SE_ERR_FNF, "  test program.bat  ", DIR_3, "" },
    { __LINE__, SE_ERR_FNF, "  test program.bat  ", DIR_4, "" },
    { __LINE__, 0xBEEFCAFE, "\"test program.bat\"", DIR_0, s_sys_bat_file },
    { __LINE__, 0xBEEFCAFE, "\"test program.bat\"", DIR_1, s_sys_bat_file },
    { __LINE__, 0xBEEFCAFE, "\"test program.bat\"", DIR_2, s_sys_bat_file },
    { __LINE__, 0xBEEFCAFE, "\"test program.bat\"", DIR_3, s_win_bat_file },
    { __LINE__, 0xBEEFCAFE, "\"test program.bat\"", DIR_4, s_sys_bat_file },
    { __LINE__, 0xBEEFCAFE, "test file.txt", DIR_0, s_sys_notepad },
    { __LINE__, 0xBEEFCAFE, "test file.txt", DIR_1, s_sys_notepad },
    { __LINE__, 0xBEEFCAFE, "test file.txt", DIR_2, s_sys_notepad },
    { __LINE__, 0xBEEFCAFE, "test file.txt", DIR_3, s_sys_notepad },
    { __LINE__, 0xBEEFCAFE, "test file.txt", DIR_4, s_sys_notepad },
    { __LINE__, SE_ERR_FNF, "invalid file.txt", DIR_0, "" },
    { __LINE__, SE_ERR_FNF, "invalid file.txt", DIR_1, "" },
    { __LINE__, SE_ERR_FNF, "invalid file.txt", DIR_2, "" },
    { __LINE__, SE_ERR_FNF, "invalid file.txt", DIR_3, "" },
    { __LINE__, SE_ERR_FNF, "invalid file.txt", DIR_4, "" },
    { __LINE__, SE_ERR_FNF, "invalid file.InvalidExtension", DIR_0, "" },
    { __LINE__, SE_ERR_FNF, "invalid file.InvalidExtension", DIR_1, "" },
    { __LINE__, SE_ERR_FNF, "invalid file.InvalidExtension", DIR_2, "" },
    { __LINE__, SE_ERR_FNF, "invalid file.InvalidExtension", DIR_3, "" },
    { __LINE__, SE_ERR_FNF, "invalid file.InvalidExtension", DIR_4, "" },
};

static void DoTestEntry(const TEST_ENTRY *pEntry)
{
    char result[MAX_PATH];
    result[0] = 0;

    UINT ret = (UINT)(UINT_PTR)FindExecutableA(pEntry->file, pEntry->dir, result);
    if (ret > 32)
        ret = 0xBEEFCAFE;

    ok(ret == pEntry->ret, "Line %u: ret expected %d, got %d\n", pEntry->lineno, pEntry->ret, ret);

    GetLongPathNameA(result, result, _countof(result));

    ok(lstrcmpiA(result, pEntry->result) == 0,
       "Line %u: result expected '%s', got '%s'\n",
       pEntry->lineno, pEntry->result, result);
}

START_TEST(FindExecutable)
{
    char cur_dir[MAX_PATH];
    GetCurrentDirectoryA(_countof(cur_dir), cur_dir);
    if (PathIsRootA(cur_dir))
    {
        skip("Don't use this program at root directory\n");
        return;
    }

    GetWindowsDirectoryA(s_win_dir, _countof(s_win_dir));

    GetWindowsDirectoryA(s_win_notepad, _countof(s_win_notepad));
    PathAppendA(s_win_notepad, "notepad.exe");

    GetSystemDirectoryA(s_sys_notepad, _countof(s_sys_notepad));
    PathAppendA(s_sys_notepad, "notepad.exe");

    GetWindowsDirectoryA(s_win_test_exe, _countof(s_win_test_exe));
    PathAppendA(s_win_test_exe, "test program.exe");
    BOOL ret = CopyFileA(s_win_notepad, s_win_test_exe, FALSE);
    if (!ret)
    {
        skip("Please retry with admin rights\n");
        return;
    }

    GetSystemDirectoryA(s_sys_test_exe, _countof(s_sys_test_exe));
    PathAppendA(s_sys_test_exe, "test program.exe");
    ret = CopyFileA(s_win_notepad, s_sys_test_exe, FALSE);
    if (!ret)
    {
        skip("Please retry with admin rights\n");
        return;
    }

    GetWindowsDirectoryA(s_win_bat_file, _countof(s_win_bat_file));
    PathAppendA(s_win_bat_file, "test program.bat");
    fclose(fopen(s_win_bat_file, "wb"));
    ok_int(PathFileExistsA(s_win_bat_file), TRUE);

    GetSystemDirectoryA(s_sys_bat_file, _countof(s_sys_bat_file));
    PathAppendA(s_sys_bat_file, "test program.bat");
    fclose(fopen(s_sys_bat_file, "wb"));
    ok_int(PathFileExistsA(s_sys_bat_file), TRUE);

    GetWindowsDirectoryA(s_win_txt_file, _countof(s_win_txt_file));
    PathAppendA(s_win_txt_file, "test file.txt");
    fclose(fopen(s_win_txt_file, "wb"));
    ok_int(PathFileExistsA(s_win_txt_file), TRUE);

    GetSystemDirectoryA(s_sys_txt_file, _countof(s_sys_txt_file));
    PathAppendA(s_sys_txt_file, "test file.txt");
    fclose(fopen(s_sys_txt_file, "wb"));
    ok_int(PathFileExistsA(s_sys_txt_file), TRUE);

    for (UINT iTest = 0; iTest < _countof(s_entries); ++iTest)
    {
        DoTestEntry(&s_entries[iTest]);
    }

    DeleteFileA(s_win_test_exe);
    DeleteFileA(s_sys_test_exe);
    DeleteFileA(s_win_bat_file);
    DeleteFileA(s_sys_bat_file);
    DeleteFileA(s_win_txt_file);
    DeleteFileA(s_sys_txt_file);
}
