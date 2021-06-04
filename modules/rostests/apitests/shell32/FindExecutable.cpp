/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for FindExecutable
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <stdio.h>
#include <shlwapi.h>

static char s_szSubProgram[MAX_PATH];

static BOOL
GetSubProgramPath(void)
{
    GetModuleFileNameA(NULL, s_szSubProgram, _countof(s_szSubProgram));
    PathRemoveFileSpecA(s_szSubProgram);
    PathAppendA(s_szSubProgram, "shell32_apitest_sub.exe");

    if (!PathFileExistsA(s_szSubProgram))
    {
        PathRemoveFileSpecA(s_szSubProgram);
        PathAppendA(s_szSubProgram, "testdata\\shell32_apitest_sub.exe");

        if (!PathFileExistsA(s_szSubProgram))
        {
            return FALSE;
        }
    }

    return TRUE;
}

typedef struct TEST_ENTRY
{
    INT lineno;
    BOOL ret;
    LPCSTR file;
    LPCSTR dir;
    LPCSTR result;
} TEST_ENTRY;

static char s_szWinDir[MAX_PATH];
static char s_szWinNotepad[MAX_PATH];
static char s_szSysNotepad[MAX_PATH];
static char s_szWinTestExe[MAX_PATH];
static char s_szSysTestExe[MAX_PATH];
static char s_szWinBatFile[MAX_PATH];
static char s_szSysBatFile[MAX_PATH];

#define DIR_0 NULL
#define DIR_1 "."
#define DIR_2 ".."
#define DIR_3 s_szWinDir
#define DIR_4 "(invalid)"

static const TEST_ENTRY s_entries[] =
{
    { __LINE__, TRUE, "notepad", DIR_0, s_szSysNotepad },
    { __LINE__, TRUE, "notepad", DIR_1, s_szSysNotepad },
    { __LINE__, TRUE, "notepad", DIR_2, s_szSysNotepad },
    { __LINE__, TRUE, "notepad", DIR_3, s_szWinNotepad },
    { __LINE__, TRUE, "notepad", DIR_4, s_szSysNotepad },
    { __LINE__, TRUE, "notepad.exe", DIR_0, s_szSysNotepad },
    { __LINE__, TRUE, "notepad.exe", DIR_1, s_szSysNotepad },
    { __LINE__, TRUE, "notepad.exe", DIR_2, s_szSysNotepad },
    { __LINE__, TRUE, "notepad.exe", DIR_3, s_szWinNotepad },
    { __LINE__, TRUE, "notepad.exe", DIR_4, s_szSysNotepad },
    { __LINE__, FALSE, "notepad.bat", DIR_0, "" },
    { __LINE__, FALSE, "notepad.bat", DIR_1, "" },
    { __LINE__, FALSE, "notepad.bat", DIR_2, "" },
    { __LINE__, FALSE, "notepad.bat", DIR_3, "" },
    { __LINE__, FALSE, "notepad.bat", DIR_4, "" },
    { __LINE__, FALSE, "C:\\notepad.exe", DIR_0, "" },
    { __LINE__, FALSE, "C:\\notepad.exe", DIR_1, "" },
    { __LINE__, FALSE, "C:\\notepad.exe", DIR_2, "" },
    { __LINE__, FALSE, "C:\\notepad.exe", DIR_3, "" },
    { __LINE__, FALSE, "C:\\notepad.exe", DIR_4, "" },
    { __LINE__, FALSE, "..\\notepad.exe", DIR_0, "" },
    { __LINE__, FALSE, "..\\notepad.exe", DIR_1, "" },
    { __LINE__, FALSE, "..\\notepad.exe", DIR_2, "" },
    { __LINE__, FALSE, "..\\notepad.exe", DIR_3, "" },
    { __LINE__, FALSE, "..\\notepad.exe", DIR_4, "" },
    { __LINE__, TRUE, "test program", DIR_0, s_szSysTestExe },
    { __LINE__, TRUE, "test program", DIR_1, s_szSysTestExe },
    { __LINE__, TRUE, "test program", DIR_2, s_szSysTestExe },
    { __LINE__, TRUE, "test program", DIR_3, s_szWinTestExe },
    { __LINE__, TRUE, "test program", DIR_4, s_szSysTestExe },
    { __LINE__, TRUE, "test program.exe", DIR_0, s_szSysTestExe },
    { __LINE__, TRUE, "test program.exe", DIR_1, s_szSysTestExe },
    { __LINE__, TRUE, "test program.exe", DIR_2, s_szSysTestExe },
    { __LINE__, TRUE, "test program.exe", DIR_3, s_szWinTestExe },
    { __LINE__, TRUE, "test program.exe", DIR_4, s_szSysTestExe },
    { __LINE__, TRUE, "test program.bat", DIR_0, s_szSysBatFile },
    { __LINE__, TRUE, "test program.bat", DIR_1, s_szSysBatFile },
    { __LINE__, TRUE, "test program.bat", DIR_2, s_szSysBatFile },
    { __LINE__, TRUE, "test program.bat", DIR_3, s_szWinBatFile },
    { __LINE__, TRUE, "test program.bat", DIR_4, s_szSysBatFile },
    { __LINE__, TRUE, "shell32_apitest_sub.exe", DIR_0, s_szSubProgram },
    { __LINE__, TRUE, "shell32_apitest_sub.exe", DIR_1, "shell32_apitest_sub.exe" },
    { __LINE__, FALSE, "shell32_apitest_sub.exe", DIR_2, "" },
    { __LINE__, FALSE, "shell32_apitest_sub.exe", DIR_3, "" },
    { __LINE__, FALSE, "shell32_apitest_sub.exe", DIR_4, "" },
};

static void DoTestEntry(const TEST_ENTRY *pEntry)
{
    char result[MAX_PATH];
    result[0] = 0;
    BOOL ret = ((INT_PTR)FindExecutableA(pEntry->file, pEntry->dir, result) > 32);
    ok(ret == pEntry->ret, "Line %u: ret expected %d, got %d\n", pEntry->lineno, pEntry->ret, ret);

    GetLongPathNameA(result, result, _countof(result));

    ok(lstrcmpiA(result, pEntry->result) == 0,
       "Line %u: result expected '%s', got '%s'\n",
       pEntry->lineno, pEntry->result, result);
}

START_TEST(FindExecutable)
{
    if (!GetSubProgramPath())
    {
        skip("shell32_apitest_sub.exe not found\n");
    }

    GetWindowsDirectoryA(s_szWinDir, _countof(s_szWinDir));

    GetWindowsDirectoryA(s_szWinNotepad, _countof(s_szWinNotepad));
    PathAppendA(s_szWinNotepad, "notepad.exe");

    GetSystemDirectoryA(s_szSysNotepad, _countof(s_szSysNotepad));
    PathAppendA(s_szSysNotepad, "notepad.exe");

    GetWindowsDirectoryA(s_szWinTestExe, _countof(s_szWinTestExe));
    PathAppendA(s_szWinTestExe, "test program.exe");
    BOOL ret = CopyFileA(s_szSubProgram, s_szWinTestExe, FALSE);
    if (!ret)
    {
        skip("Please retry with admin rights\n");
        return;
    }

    GetSystemDirectoryA(s_szSysTestExe, _countof(s_szSysTestExe));
    PathAppendA(s_szSysTestExe, "test program.exe");
    ok_int(CopyFileA(s_szSubProgram, s_szSysTestExe, FALSE), TRUE);

    GetWindowsDirectoryA(s_szWinBatFile, _countof(s_szWinBatFile));
    PathAppendA(s_szWinBatFile, "test program.bat");
    fclose(fopen(s_szWinBatFile, "wb"));
    ok_int(PathFileExistsA(s_szWinBatFile), TRUE);

    GetSystemDirectoryA(s_szSysBatFile, _countof(s_szSysBatFile));
    PathAppendA(s_szSysBatFile, "test program.bat");
    fclose(fopen(s_szSysBatFile, "wb"));
    ok_int(PathFileExistsA(s_szSysBatFile), TRUE);

    for (UINT iTest = 0; iTest < _countof(s_entries); ++iTest)
    {
        DoTestEntry(&s_entries[iTest]);
    }

    DeleteFileA(s_szWinTestExe);
    DeleteFileA(s_szSysTestExe);
    DeleteFileA(s_szWinBatFile);
    DeleteFileA(s_szSysBatFile);
}
