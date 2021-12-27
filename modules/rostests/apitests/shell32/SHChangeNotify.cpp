/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for SHChangeNotify
 * COPYRIGHT:   Copyright 2020-2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

// NOTE: This test program closes the Explorer windows before tests.

#include "shelltest.h"
#include "SHChangeNotify.h"
#include <time.h>
#include <process.h>
#include <versionhelpers.h>

// --- The selection of tests ---
#define DISABLE_THIS_TESTCASE
#define NO_TRIVIAL
//#define NO_INTERRUPT_LEVEL
//#define NO_SHELL_LEVEL
#define NEW_DELIVERY_ONLY
//#define RANDOM_HALF
#define RANDOM_QUARTER

// --- Show the elapsed time by GetTickCount() ---
//#define ENTRY_TICK
#define GROUP_TICK
#define TOTAL_TICK

static HWND s_hwnd = NULL;
static WCHAR s_szSubProgram[MAX_PATH];
static HANDLE s_hThread = NULL;
static HANDLE s_hEvent = NULL;

static HWND DoWaitForWindow(LPCWSTR clsname, LPCWSTR text, BOOL bClosing, BOOL bForce)
{
    HWND hwnd = NULL;
    for (INT i = 0; i < 50; ++i)
    {
        hwnd = FindWindowW(clsname, text);
        if (bClosing)
        {
            if (!hwnd)
                break;

            if (bForce)
                PostMessage(hwnd, WM_CLOSE, 0, 0);
        }
        else
        {
            if (hwnd)
                break;
        }

        Sleep(1);
    }
    return hwnd;
}

static BOOL DoCreateEmptyFile(LPCWSTR pszFileName)
{
    FILE *fp = _wfopen(pszFileName, L"wb");
    if (fp)
        fclose(fp);
    return fp != NULL;
}

struct TEST_ENTRY;

typedef BOOL (*ACTION)(const struct TEST_ENTRY *entry);

typedef struct TEST_ENTRY
{
    INT line;
    DIRTYPE iWriteDir;
    LPCSTR pattern;
    LPCWSTR path1;
    LPCWSTR path2;
    ACTION action;
} TEST_ENTRY;

#define TEST_FILE      L"_TEST_.txt"
#define TEST_FILE_KAI  L"_TEST_KAI_.txt"
#define TEST_DIR       L"_TESTDIR_"
#define TEST_DIR_KAI   L"_TESTDIR_KAI_"
#define MOVE_FILE(from, to) MoveFileW((from), (to))

static BOOL DoAction1(const TEST_ENTRY *entry)
{
    LPWSTR pszPath = DoGetDir(entry->iWriteDir);
    PathAppendW(pszPath, TEST_FILE);
    ok(DoCreateEmptyFile(pszPath), "Line %d: DoCreateEmptyFile failed\n", entry->line);
    return TRUE;
}

static BOOL DoAction2(const TEST_ENTRY *entry)
{
    LPWSTR pszPath1 = DoGetDir(entry->iWriteDir), pszPath2 = DoGetDir(entry->iWriteDir);
    PathAppendW(pszPath1, TEST_FILE);
    PathAppendW(pszPath2, TEST_FILE_KAI);
    ok(MOVE_FILE(pszPath1, pszPath2), "Line %d: MOVE_FILE(%ls, %ls) failed (%ld)\n",
       entry->line, pszPath1, pszPath2, GetLastError());
    return TRUE;
}

static BOOL DoAction3(const TEST_ENTRY *entry)
{
    LPWSTR pszPath1 = DoGetDir(entry->iWriteDir), pszPath2 = DoGetDir(entry->iWriteDir);
    PathAppendW(pszPath1, TEST_FILE_KAI);
    PathAppendW(pszPath2, TEST_FILE);
    ok(MOVE_FILE(pszPath1, pszPath2), "Line %d: MOVE_FILE(%ls, %ls) failed (%ld)\n",
       entry->line, pszPath1, pszPath2, GetLastError());
    return TRUE;
}

static BOOL DoAction4(const TEST_ENTRY *entry)
{
    LPWSTR pszPath = DoGetDir(entry->iWriteDir);
    PathAppendW(pszPath, TEST_FILE);
    ok(DeleteFileW(pszPath), "Line %d: DeleteFileW(%ls) failed (%ld)\n",
       entry->line, pszPath, GetLastError());
    return TRUE;
}

static BOOL DoAction5(const TEST_ENTRY *entry)
{
    LPWSTR pszPath = DoGetDir(entry->iWriteDir);
    PathAppendW(pszPath, TEST_DIR);
    ok(CreateDirectoryW(pszPath, NULL), "Line %d: CreateDirectoryW(%ls) failed (%ld)\n",
       entry->line, pszPath, GetLastError());
    return TRUE;
}

static BOOL DoAction6(const TEST_ENTRY *entry)
{
    LPWSTR pszPath1 = DoGetDir(entry->iWriteDir), pszPath2 = DoGetDir(entry->iWriteDir);
    PathAppendW(pszPath1, TEST_DIR);
    PathAppendW(pszPath2, TEST_DIR_KAI);
    ok(MOVE_FILE(pszPath1, pszPath2), "Line %d: MOVE_FILE(%ls, %ls) failed (%ld)\n",
       entry->line, pszPath1, pszPath2, GetLastError());
    return TRUE;
}

static BOOL DoAction7(const TEST_ENTRY *entry)
{
    LPWSTR pszPath1 = DoGetDir(entry->iWriteDir), pszPath2 = DoGetDir(entry->iWriteDir);
    PathAppendW(pszPath1, TEST_DIR_KAI);
    PathAppendW(pszPath2, TEST_DIR);
    ok(MOVE_FILE(pszPath1, pszPath2), "Line %d: MOVE_FILE(%ls, %ls) failed (%ld)\n",
       entry->line, pszPath1, pszPath2, GetLastError());
    return TRUE;
}

static BOOL DoAction8(const TEST_ENTRY *entry)
{
    LPWSTR pszPath = DoGetDir(entry->iWriteDir);
    PathAppendW(pszPath, TEST_DIR);
    ok(RemoveDirectoryW(pszPath), "Line %d: RemoveDirectoryW(%ls) failed (%ld)\n",
       entry->line, pszPath, GetLastError());
    return TRUE;
}

static BOOL DoAction9(const TEST_ENTRY *entry)
{
    LPWSTR pszPath = DoGetDir(entry->iWriteDir);
    PathAppendW(pszPath, TEST_FILE);
    SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW | SHCNF_FLUSH, pszPath, NULL);
    return FALSE;
}

static BOOL DoAction10(const TEST_ENTRY *entry)
{
    LPWSTR pszPath = DoGetDir(entry->iWriteDir);
    PathAppendW(pszPath, TEST_FILE);
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW | SHCNF_FLUSH, pszPath, NULL);
    return FALSE;
}

static BOOL DoAction11(const TEST_ENTRY *entry)
{
    LPWSTR pszPath = DoGetDir(entry->iWriteDir);
    PathAppendW(pszPath, TEST_DIR);
    SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW | SHCNF_FLUSH, pszPath, NULL);
    return FALSE;
}

static BOOL DoAction12(const TEST_ENTRY *entry)
{
    LPWSTR pszPath = DoGetDir(entry->iWriteDir);
    PathAppendW(pszPath, TEST_DIR);
    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHW | SHCNF_FLUSH, pszPath, NULL);
    return FALSE;
}

#define WRITEDIR_0 DIRTYPE_DESKTOP
static WCHAR s_szDesktop[MAX_PATH];
static WCHAR s_szTestFile0[MAX_PATH];
static WCHAR s_szTestFile0Kai[MAX_PATH];
static WCHAR s_szTestDir0[MAX_PATH];
static WCHAR s_szTestDir0Kai[MAX_PATH];

#define WRITEDIR_1 DIRTYPE_MYDOCUMENTS
static WCHAR s_szDocuments[MAX_PATH];
static WCHAR s_szTestFile1[MAX_PATH];
static WCHAR s_szTestFile1Kai[MAX_PATH];
static WCHAR s_szTestDir1[MAX_PATH];
static WCHAR s_szTestDir1Kai[MAX_PATH];

static void DoDeleteFilesAndDirs(void)
{
    DeleteFileW(TEMP_FILE);
    DeleteFileW(s_szTestFile0);
    DeleteFileW(s_szTestFile0Kai);
    DeleteFileW(s_szTestFile1);
    DeleteFileW(s_szTestFile1Kai);
    RemoveDirectoryW(s_szTestDir0);
    RemoveDirectoryW(s_szTestDir0Kai);
    RemoveDirectoryW(s_szTestDir1);
    RemoveDirectoryW(s_szTestDir1Kai);
}

static const TEST_ENTRY s_group_00[] =
{
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction12 },
};

static const TEST_ENTRY s_group_01[] =
{
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "0100000", s_szTestFile0, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0010000", s_szTestFile0, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0001000", s_szTestDir0, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000100", s_szTestDir0, L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "0100000", s_szTestFile1, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0010000", s_szTestFile1, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0001000", s_szTestDir1, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000100", s_szTestDir1, L"", DoAction12 },
};

static const TEST_ENTRY s_group_02[] =
{
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "0100000", s_szTestFile1, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0010000", s_szTestFile1, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0001000", s_szTestDir1, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000100", s_szTestDir1, L"", DoAction12 },
};

static const TEST_ENTRY s_group_03[] =
{
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "0100000", s_szTestFile0, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0010000", s_szTestFile0, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0001000", s_szTestDir0, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000100", s_szTestDir0, L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction12 },
};

static const TEST_ENTRY s_group_04[] =
{
    { __LINE__, WRITEDIR_0, "0100000", s_szTestFile0, L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "1000000", s_szTestFile0, s_szTestFile0Kai, DoAction2 },
    { __LINE__, WRITEDIR_0, "1000000", s_szTestFile0Kai, s_szTestFile0, DoAction3 },
    { __LINE__, WRITEDIR_0, "0010000", s_szTestFile0, L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0001000", s_szTestDir0, L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000010", s_szTestDir0, s_szTestDir0Kai, DoAction6 },
    { __LINE__, WRITEDIR_0, "0000010", s_szTestDir0Kai, s_szTestDir0, DoAction7 },
    { __LINE__, WRITEDIR_0, "0000100", s_szTestDir0, L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction12 },
};

static const TEST_ENTRY s_group_05[] =
{
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "0100000", s_szTestFile1, L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "1000000", s_szTestFile1, s_szTestFile1Kai, DoAction2 },
    { __LINE__, WRITEDIR_1, "1000000", s_szTestFile1Kai, s_szTestFile1, DoAction3 },
    { __LINE__, WRITEDIR_1, "0010000", s_szTestFile1, L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "0001000", s_szTestDir1, L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "0000010", s_szTestDir1, s_szTestDir1Kai, DoAction6 },
    { __LINE__, WRITEDIR_1, "0000010", s_szTestDir1Kai, s_szTestDir1, DoAction7 },
    { __LINE__, WRITEDIR_1, "0000100", s_szTestDir1, L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction12 },
};

static const TEST_ENTRY s_group_06[] =
{
    { __LINE__, WRITEDIR_0, "0100000", s_szTestFile0, L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "1000000", s_szTestFile0, s_szTestFile0Kai, DoAction2 },
    { __LINE__, WRITEDIR_0, "1000000", s_szTestFile0Kai, s_szTestFile0, DoAction3 },
    { __LINE__, WRITEDIR_0, "0010000", s_szTestFile0, L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0001000", s_szTestDir0, L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000010", s_szTestDir0, s_szTestDir0Kai, DoAction6 },
    { __LINE__, WRITEDIR_0, "0000010", s_szTestDir0Kai, s_szTestDir0, DoAction7 },
    { __LINE__, WRITEDIR_0, "0000100", s_szTestDir0, L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "0100000", s_szTestFile0, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0010000", s_szTestFile0, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0001000", s_szTestDir0, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000100", s_szTestDir0, L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "0100000", s_szTestFile1, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0010000", s_szTestFile1, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0001000", s_szTestDir1, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000100", s_szTestDir1, L"", DoAction12 },
};

static const TEST_ENTRY s_group_07[] =
{
    { __LINE__, WRITEDIR_0, "0100000", s_szTestFile0, L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "1000000", s_szTestFile0, s_szTestFile0Kai, DoAction2 },
    { __LINE__, WRITEDIR_0, "1000000", s_szTestFile0Kai, s_szTestFile0, DoAction3 },
    { __LINE__, WRITEDIR_0, "0010000", s_szTestFile0, L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0001000", s_szTestDir0, L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000010", s_szTestDir0, s_szTestDir0Kai, DoAction6 },
    { __LINE__, WRITEDIR_0, "0000010", s_szTestDir0Kai, s_szTestDir0, DoAction7 },
    { __LINE__, WRITEDIR_0, "0000100", s_szTestDir0, L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "0100000", s_szTestFile0, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0010000", s_szTestFile0, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0001000", s_szTestDir0, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000100", s_szTestDir0, L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction12 },
};

static const TEST_ENTRY s_group_08[] =
{
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000000", L"", L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "0100000", s_szTestFile1, L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "1000000", s_szTestFile1, s_szTestFile1Kai, DoAction2 },
    { __LINE__, WRITEDIR_1, "1000000", s_szTestFile1Kai, s_szTestFile1, DoAction3 },
    { __LINE__, WRITEDIR_1, "0010000", s_szTestFile1, L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "0001000", s_szTestDir1, L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "0000010", s_szTestDir1, s_szTestDir1Kai, DoAction6 },
    { __LINE__, WRITEDIR_1, "0000010", s_szTestDir1Kai, s_szTestDir1, DoAction7 },
    { __LINE__, WRITEDIR_1, "0000100", s_szTestDir1, L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "0100000", s_szTestFile1, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0010000", s_szTestFile1, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0001000", s_szTestDir1, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000100", s_szTestDir1, L"", DoAction12 },
};

static LPCSTR PatternFromFlags(DWORD flags)
{
    static CHAR s_buf[(TYPE_MAX + 1) + 1];
    for (INT i = 0; i < (TYPE_MAX + 1); ++i)
        s_buf[i] = (char)('0' + !!(flags & (1 << i)));

    s_buf[TYPE_MAX + 1] = 0;
    return s_buf;
}

static BOOL DoGetPaths(LPWSTR pszPath1, LPWSTR pszPath2)
{
    pszPath1[0] = pszPath2[0] = 0;

    WCHAR szText[MAX_PATH * 2];
    szText[0] = 0;
    FILE *fp = _wfopen(TEMP_FILE, L"rb");
    if (fp)
    {
        fread(szText, 1, sizeof(szText), fp);
        fclose(fp);
    }

    LPWSTR pch = wcschr(szText, L'|');
    if (pch == NULL)
        return FALSE;

    *pch = 0;
    StringCchCopyW(pszPath1, MAX_PATH, szText);
    StringCchCopyW(pszPath2, MAX_PATH, pch + 1);
    return TRUE;
}

static void DoTestEntry(INT iEntry, const TEST_ENTRY *entry, INT nSources)
{
#ifdef ENTRY_TICK
    DWORD dwOldTick = GetTickCount();
#endif

    BOOL bInterrupting = FALSE;
    if (entry->action)
        bInterrupting = entry->action(entry);

    DWORD flags;
    LPCSTR pattern;
    if ((nSources & SHCNRF_InterruptLevel) && bInterrupting)
    {
        // The event won't work at here. Manually waiting...
        UINT cTry = ((iEntry == 0) ? 100 : 60);
        for (UINT iTry = 0; iTry < cTry; ++iTry)
        {
            flags = SendMessageW(s_hwnd, WM_GET_NOTIFY_FLAGS, 0, 0);
            pattern = PatternFromFlags(flags);
            if (strcmp(pattern, "0000000") != 0)
                break;

            Sleep(1);
        }
    }
    else
    {
        if (WaitForSingleObject(s_hEvent, 100) == WAIT_OBJECT_0)
            Sleep(1);

        flags = SendMessageW(s_hwnd, WM_GET_NOTIFY_FLAGS, 0, 0);
        pattern = PatternFromFlags(flags);
    }

    SendMessageW(s_hwnd, WM_SET_PATHS, 0, 0);

    WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH];
    szPath1[0] = szPath2[0] = 0;
    BOOL bOK = DoGetPaths(szPath1, szPath2);

    static UINT s_cCalmDown = 0;

    if (pattern[TYPE_UPDATEDIR] == '1')
    {
        trace("Line %d: SHCNE_UPDATEDIR: Calm down (%u)...\n", entry->line, s_cCalmDown);

        if (++s_cCalmDown < 3)
            Sleep(3000);

        if (entry->pattern)
            ok(TRUE, "Line %d:\n", entry->line);
        if (entry->path1)
            ok(TRUE, "Line %d:\n", entry->line);
        if (entry->path2)
            ok(TRUE, "Line %d:\n", entry->line);
    }
    else
    {
        s_cCalmDown = 0;
        if (entry->pattern)
        {
            ok(strcmp(pattern, entry->pattern) == 0,
               "Line %d: pattern mismatch '%s', tick=0x%08lX\n",
               entry->line, pattern, GetTickCount());
        }
        if (entry->path1)
            ok(bOK && lstrcmpiW(entry->path1, szPath1) == 0,
               "Line %d: path1 mismatch '%S' (%d)\n", entry->line, szPath1, bOK);
        if (entry->path2)
            ok(bOK && lstrcmpiW(entry->path2, szPath2) == 0,
               "Line %d: path2 mismatch '%S' (%d)\n", entry->line, szPath2, bOK);
    }

    SendMessageW(s_hwnd, WM_CLEAR_FLAGS, 0, 0);
    ResetEvent(s_hEvent);

#ifdef ENTRY_TICK
    DWORD dwNewTick = GetTickCount();
    DWORD dwTick = dwNewTick - dwOldTick;
    trace("DoTestEntry: Line %d: tick=%lu.%lu sec\n", entry->line,
          (dwTick / 1000), (dwTick / 100 % 10));
#endif
}

static void DoQuitTest(BOOL bForce)
{
    PostMessageW(s_hwnd, WM_COMMAND, IDOK, 0);

    DoWaitForWindow(CLASSNAME, CLASSNAME, TRUE, bForce);
    s_hwnd = NULL;

    if (s_hEvent)
    {
        CloseHandle(s_hEvent);
        s_hEvent = NULL;
    }

    DoDeleteFilesAndDirs();
}

static void DoAbortThread(void)
{
    skip("Aborting the thread...\n");
    if (s_hThread)
    {
        TerminateThread(s_hThread, -1);
        s_hThread = NULL;
    }
}

static BOOL CALLBACK HandlerRoutine(DWORD dwCtrlType)
{
    switch (dwCtrlType)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            DoAbortThread();
            DoQuitTest(TRUE);
            return TRUE;
    }
    return FALSE;
}

static BOOL DoInitTest(void)
{
    // DIRTYPE_DESKTOP
    LPWSTR psz = DoGetDir(DIRTYPE_DESKTOP);
    StringCchCopyW(s_szDesktop, _countof(s_szDesktop), psz);

    PathAppendW(psz, TEST_FILE);
    StringCchCopyW(s_szTestFile0, _countof(s_szTestFile0), psz);

    PathRemoveFileSpecW(psz);
    PathAppendW(psz, TEST_FILE_KAI);
    StringCchCopyW(s_szTestFile0Kai, _countof(s_szTestFile0Kai), psz);

    PathRemoveFileSpecW(psz);
    PathAppendW(psz, TEST_DIR);
    StringCchCopyW(s_szTestDir0, _countof(s_szTestDir0), psz);

    PathRemoveFileSpecW(psz);
    PathAppendW(psz, TEST_DIR_KAI);
    StringCchCopyW(s_szTestDir0Kai, _countof(s_szTestDir0Kai), psz);

    // DIRTYPE_MYDOCUMENTS
    psz = DoGetDir(DIRTYPE_MYDOCUMENTS);
    StringCchCopyW(s_szDocuments, _countof(s_szDocuments), psz);

    PathAppendW(psz, TEST_FILE);
    StringCchCopyW(s_szTestFile1, _countof(s_szTestFile1), psz);

    PathRemoveFileSpecW(psz);
    PathAppendW(psz, TEST_FILE_KAI);
    StringCchCopyW(s_szTestFile1Kai, _countof(s_szTestFile1Kai), psz);

    PathRemoveFileSpecW(psz);
    PathAppendW(psz, TEST_DIR);
    StringCchCopyW(s_szTestDir1, _countof(s_szTestDir1), psz);

    PathRemoveFileSpecW(psz);
    PathAppendW(psz, TEST_DIR_KAI);
    StringCchCopyW(s_szTestDir1Kai, _countof(s_szTestDir1Kai), psz);

    // prepare for files and dirs
    DoDeleteFilesAndDirs();
    DoCreateEmptyFile(TEMP_FILE);

    // Ctrl+C
    SetConsoleCtrlHandler(HandlerRoutine, TRUE);

    // close Explorer windows
    trace("Closing Explorer windows...\n");
    DoWaitForWindow(L"CabinetWClass", NULL, TRUE, TRUE);

    // close the CLASSNAME windows
    return DoWaitForWindow(CLASSNAME, CLASSNAME, TRUE, TRUE) == NULL;
}

static BOOL
GetSubProgramPath(void)
{
    GetModuleFileNameW(NULL, s_szSubProgram, _countof(s_szSubProgram));
    PathRemoveFileSpecW(s_szSubProgram);
    PathAppendW(s_szSubProgram, L"shell32_apitest_sub.exe");

    if (!PathFileExistsW(s_szSubProgram))
    {
        PathRemoveFileSpecW(s_szSubProgram);
        PathAppendW(s_szSubProgram, L"testdata\\shell32_apitest_sub.exe");

        if (!PathFileExistsW(s_szSubProgram))
            return FALSE;
    }

    return TRUE;
}

#define SRC_00 0
#define SRC_01 SHCNRF_ShellLevel
#define SRC_02 (SHCNRF_NewDelivery)
#define SRC_03 (SHCNRF_NewDelivery | SHCNRF_ShellLevel)
#define SRC_04 SHCNRF_InterruptLevel
#define SRC_05 (SHCNRF_InterruptLevel | SHCNRF_ShellLevel)
#define SRC_06 (SHCNRF_InterruptLevel | SHCNRF_NewDelivery)
#define SRC_07 (SHCNRF_InterruptLevel | SHCNRF_NewDelivery | SHCNRF_ShellLevel)
#define SRC_08 (SHCNRF_RecursiveInterrupt | SHCNRF_InterruptLevel)
#define SRC_09 (SHCNRF_RecursiveInterrupt | SHCNRF_InterruptLevel | SHCNRF_ShellLevel)
#define SRC_10 (SHCNRF_RecursiveInterrupt | SHCNRF_InterruptLevel | SHCNRF_NewDelivery)
#define SRC_11 (SHCNRF_RecursiveInterrupt | SHCNRF_InterruptLevel | SHCNRF_NewDelivery | SHCNRF_ShellLevel)

#define WATCHDIR_0 DIRTYPE_NULL
#define WATCHDIR_1 DIRTYPE_DESKTOP
#define WATCHDIR_2 DIRTYPE_MYCOMPUTER
#define WATCHDIR_3 DIRTYPE_MYDOCUMENTS

static void
DoTestGroup(INT line, UINT cEntries, const TEST_ENTRY *pEntries, BOOL fRecursive,
            INT nSources, DIRTYPE iWatchDir)
{
#ifdef NO_INTERRUPT_LEVEL
    if (nSources & SHCNRF_InterruptLevel)
        return;
#endif
#ifdef NO_SHELL_LEVEL
    if (nSources & SHCNRF_ShellLevel)
        return;
#endif
#ifdef NEW_DELIVERY_ONLY
    if (!(nSources & SHCNRF_NewDelivery))
        return;
#endif
#ifdef GROUP_TICK
    DWORD dwOldTick = GetTickCount();
#endif
#ifdef RANDOM_QUARTER
    if ((rand() & 3) == 0)
        return;
#elif defined(RANDOM_HALF)
    if (rand() & 1)
        return;
#endif

    trace("DoTestGroup: Line %d: fRecursive:%u, iWatchDir:%u, nSources:0x%X\n",
          line, fRecursive, iWatchDir, nSources);

    if (s_hEvent)
    {
        CloseHandle(s_hEvent);
        s_hEvent = NULL;
    }
    s_hEvent = CreateEventW(NULL, TRUE, FALSE, EVENT_NAME);

    WCHAR szParams[64];
    StringCchPrintfW(szParams, _countof(szParams), L"%u,%u,%u", fRecursive, iWatchDir, nSources);

    HINSTANCE hinst = ShellExecuteW(NULL, NULL, s_szSubProgram, szParams, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)hinst <= 32)
    {
        skip("Unable to run shell32_apitest_sub.exe.\n");
        return;
    }

    s_hwnd = DoWaitForWindow(CLASSNAME, CLASSNAME, FALSE, FALSE);
    if (!s_hwnd)
    {
        skip("Unable to find window.\n");
        return;
    }

    for (UINT i = 0; i < cEntries; ++i)
    {
        if (!IsWindow(s_hwnd))
        {
            DoAbortThread();
            DoQuitTest(TRUE);
            break;
        }

        DoTestEntry(i, &pEntries[i], nSources);
    }

    DoQuitTest(FALSE);

#ifdef GROUP_TICK
    DWORD dwNewTick = GetTickCount();
    DWORD dwTick = dwNewTick - dwOldTick;
    trace("DoTestGroup: Line %d: %lu.%lu sec\n", line, (dwTick / 1000), (dwTick / 100 % 10));
#endif
}

static unsigned __stdcall TestThreadProc(void *)
{
    srand(time(NULL));
#ifdef RANDOM_QUARTER
    skip("RANDOM_QUARTER\n");
#elif defined(RANDOM_HALF)
    skip("RANDOM_HALF\n");
#endif

    // fRecursive == FALSE.
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_00, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_01), s_group_01, FALSE, SRC_01, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_02, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_01), s_group_01, FALSE, SRC_03, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, FALSE, SRC_04, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, FALSE, SRC_05, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, FALSE, SRC_06, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, FALSE, SRC_07, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, FALSE, SRC_08, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, FALSE, SRC_09, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, FALSE, SRC_10, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, FALSE, SRC_11, WATCHDIR_0);

    BOOL bTarget = IsWindowsXPOrGreater() && !IsWindowsVistaOrGreater();

#define SWITCH(x, y) (bTarget ? (x) : (y))
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_00, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_03), s_group_03, FALSE, SRC_01, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_02, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_03), s_group_03, FALSE, SRC_03, WATCHDIR_1);
    DoTestGroup(__LINE__, SWITCH(_countof(s_group_00), _countof(s_group_04)), SWITCH(s_group_00, s_group_04), FALSE, SRC_04, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_07), s_group_07, FALSE, SRC_05, WATCHDIR_1);
    DoTestGroup(__LINE__, SWITCH(_countof(s_group_00), _countof(s_group_04)), SWITCH(s_group_00, s_group_04), FALSE, SRC_06, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_07), s_group_07, FALSE, SRC_07, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, FALSE, SRC_08, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_07), s_group_07, FALSE, SRC_09, WATCHDIR_1);
    DoTestGroup(__LINE__, SWITCH(_countof(s_group_00), _countof(s_group_04)), SWITCH(s_group_00, s_group_04), FALSE, SRC_06, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_07), s_group_07, FALSE, SRC_11, WATCHDIR_1);
#undef SWITCH

#ifndef NO_TRIVIAL
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_00, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_01, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_02, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_03, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_04, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_05, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_06, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_07, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_08, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_09, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_10, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_11, WATCHDIR_2);
#endif

    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_00, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_02), s_group_02, FALSE, SRC_01, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SRC_02, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_02), s_group_02, FALSE, SRC_03, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_05), s_group_05, FALSE, SRC_04, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, FALSE, SRC_05, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_05), s_group_05, FALSE, SRC_06, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, FALSE, SRC_07, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_05), s_group_05, FALSE, SRC_08, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, FALSE, SRC_09, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_05), s_group_05, FALSE, SRC_10, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, FALSE, SRC_11, WATCHDIR_3);

    // fRecursive == TRUE.
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SRC_00, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_01), s_group_01, TRUE, SRC_01, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SRC_02, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_01), s_group_01, TRUE, SRC_03, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, TRUE, SRC_04, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SRC_05, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, TRUE, SRC_06, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SRC_07, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, TRUE, SRC_08, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SRC_09, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, TRUE, SRC_10, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SRC_11, WATCHDIR_0);

    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SRC_00, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_01), s_group_01, TRUE, SRC_01, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SRC_02, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_01), s_group_01, TRUE, SRC_03, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, TRUE, SRC_04, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SRC_05, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, TRUE, SRC_06, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SRC_07, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, TRUE, SRC_08, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SRC_09, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, TRUE, SRC_10, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SRC_11, WATCHDIR_1);

    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SRC_00, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_01), s_group_01, TRUE, SRC_01, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SRC_02, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_01), s_group_01, TRUE, SRC_03, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, TRUE, SRC_04, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SRC_05, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, TRUE, SRC_06, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SRC_07, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, TRUE, SRC_08, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SRC_09, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, TRUE, SRC_10, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SRC_11, WATCHDIR_2);

    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SRC_00, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_02), s_group_02, TRUE, SRC_01, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SRC_02, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_02), s_group_02, TRUE, SRC_03, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_05), s_group_05, TRUE, SRC_04, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SRC_05, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_05), s_group_05, TRUE, SRC_06, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SRC_07, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_05), s_group_05, TRUE, SRC_08, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SRC_09, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_05), s_group_05, TRUE, SRC_10, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SRC_11, WATCHDIR_3);

    return 0;
}

START_TEST(SHChangeNotify)
{
#ifdef DISABLE_THIS_TESTCASE
    skip("This testcase is disabled by DISABLE_THIS_TESTCASE macro.\n");
#endif
#ifdef TOTAL_TICK
    DWORD dwOldTick = GetTickCount();
#endif

    trace("Please don't operate your PC while testing...\n");

    if (!GetSubProgramPath())
    {
        skip("shell32_apitest_sub.exe not found\n");
        return;
    }

    if (!DoInitTest())
    {
        skip("Unable to initialize.\n");
        DoQuitTest(TRUE);
        return;
    }

    s_hThread = (HANDLE)_beginthreadex(NULL, 0, TestThreadProc, NULL, 0, NULL);
    WaitForSingleObject(s_hThread, INFINITE);
    CloseHandle(s_hThread);

#ifdef TOTAL_TICK
    DWORD dwNewTick = GetTickCount();
    DWORD dwTick = dwNewTick - dwOldTick;
    trace("SHChangeNotify: Total %lu.%lu sec\n", (dwTick / 1000), (dwTick / 100 % 10));
#endif
}
