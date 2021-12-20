/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for SHChangeNotify
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

// NOTE: This test program closes the Explorer cabinets before tests.

#include "shelltest.h"
#include <shlwapi.h>
#include <stdio.h>
#include <process.h> // for _beginthreadex
#include "SHChangeNotify.h"

static HWND s_hwnd = NULL;
static WCHAR s_szSubProgram[MAX_PATH];
static HANDLE s_hThread = NULL;

struct TEST_ENTRY;

typedef void (*ACTION)(const struct TEST_ENTRY *pEntry);

typedef struct TEST_ENTRY
{
    INT line;
    WATCHDIR iWriteDir;
    LPCSTR pattern;
    LPCWSTR path1;
    LPCWSTR path2;
    ACTION action;
} TEST_ENTRY;

static BOOL
DoCreateEmptyFile(LPCWSTR pszFileName)
{
    FILE *fp = _wfopen(pszFileName, L"wb");
    if (fp)
        fclose(fp);
    return fp != NULL;
}

#define TEST_FILE           L"_TEST_.txt"
#define TEST_FILE_RENAMED   L"_TEST_RENAMED_.txt"
#define TEST_DIR            L"_TESTDIR_"
#define TEST_DIR_RENAMED    L"_TESTDIR_RENAMED_"

static void
DoAction1(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, TEST_FILE);
    DoCreateEmptyFile(pszPath);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

static void
DoAction2(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath1 = GetWatchDir(pEntry->iWriteDir);
    LPWSTR pszPath2 = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath1, TEST_FILE);
    PathAppendW(pszPath2, TEST_FILE_RENAMED);
    MoveFileW(pszPath1, pszPath2);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

static void
DoAction3(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath1 = GetWatchDir(pEntry->iWriteDir);
    LPWSTR pszPath2 = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath1, TEST_FILE_RENAMED);
    PathAppendW(pszPath2, TEST_FILE);
    MoveFileW(pszPath1, pszPath2);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

static void
DoAction4(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, TEST_FILE);
    DeleteFileW(pszPath);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

static void
DoAction5(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, TEST_DIR);
    CreateDirectoryW(pszPath, NULL);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

static void
DoAction6(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath1 = GetWatchDir(pEntry->iWriteDir);
    LPWSTR pszPath2 = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath1, TEST_DIR);
    PathAppendW(pszPath2, TEST_DIR_RENAMED);
    MoveFileW(pszPath1, pszPath2);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

static void
DoAction7(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath1 = GetWatchDir(pEntry->iWriteDir);
    LPWSTR pszPath2 = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath1, TEST_DIR_RENAMED);
    PathAppendW(pszPath2, TEST_DIR);
    MoveFileW(pszPath1, pszPath2);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

static void
DoAction8(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, TEST_DIR);
    RemoveDirectoryW(pszPath);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

static void
DoAction9(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, TEST_FILE);
    SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW | SHCNF_FLUSH, pszPath, NULL);
}

static void
DoAction10(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, TEST_FILE);
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW | SHCNF_FLUSH, pszPath, NULL);
}

static void
DoAction11(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, TEST_DIR);
    SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW | SHCNF_FLUSH, pszPath, NULL);
}

static void
DoAction12(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, TEST_DIR);
    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHW | SHCNF_FLUSH, pszPath, NULL);
}

#define WRITEDIR_0 WATCHDIR_DESKTOP
static WCHAR s_szDesktopTestFile[MAX_PATH];
static WCHAR s_szDesktopTestFileRenamed[MAX_PATH];
static WCHAR s_szDesktopTestDir[MAX_PATH];
static WCHAR s_szDesktopTestDirRenamed[MAX_PATH];

#define WRITEDIR_1 WATCHDIR_MYDOCUMENTS
static WCHAR s_szDocumentTestFile[MAX_PATH];
static WCHAR s_szDocumentTestFileRenamed[MAX_PATH];
static WCHAR s_szDocumentTestDir[MAX_PATH];
static WCHAR s_szDocumentTestDirRenamed[MAX_PATH];

static const TEST_ENTRY s_group_00[] =
{
    { __LINE__, WRITEDIR_0, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction12 },
};

static const TEST_ENTRY s_group_01[] =
{
    { __LINE__, WRITEDIR_0, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "010000", s_szDesktopTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "001000", s_szDesktopTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "000100", s_szDesktopTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "000010", s_szDesktopTestDir, L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "010000", s_szDocumentTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "001000", s_szDocumentTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "000100", s_szDocumentTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "000010", s_szDocumentTestDir, L"", DoAction12 },
};

static const TEST_ENTRY s_group_02[] =
{
    { __LINE__, WRITEDIR_0, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "010000", s_szDesktopTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "001000", s_szDesktopTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "000100", s_szDesktopTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "000010", s_szDesktopTestDir, L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "010000", s_szDocumentTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "001000", s_szDocumentTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "000100", s_szDocumentTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "000010", s_szDocumentTestDir, L"", DoAction12 },
};

static const TEST_ENTRY s_group_03[] =
{
    { __LINE__, WRITEDIR_0, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "010000", s_szDocumentTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "001000", s_szDocumentTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "000100", s_szDocumentTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "000010", s_szDocumentTestDir, L"", DoAction12 },
};

static const TEST_ENTRY s_group_04[] =
{
    { __LINE__, WRITEDIR_0, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "010000", s_szDocumentTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "001000", s_szDocumentTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "000100", s_szDocumentTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "000010", s_szDocumentTestDir, L"", DoAction12 },
};

static const TEST_ENTRY s_group_05[] =
{
    { __LINE__, WRITEDIR_0, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "010000", s_szDesktopTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "001000", s_szDesktopTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "000100", s_szDesktopTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "000010", s_szDesktopTestDir, L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "010000", s_szDocumentTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "001000", s_szDocumentTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "000100", s_szDocumentTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "000010", s_szDocumentTestDir, L"", DoAction12 },
};

static const TEST_ENTRY s_group_06[] =
{
    { __LINE__, WRITEDIR_0, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "010000", s_szDesktopTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "001000", s_szDesktopTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "000100", s_szDesktopTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "000010", s_szDesktopTestDir, L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "010000", s_szDocumentTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "001000", s_szDocumentTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "000100", s_szDocumentTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "000010", s_szDocumentTestDir, L"", DoAction12 },
};

static const TEST_ENTRY s_group_07[] =
{
    { __LINE__, WRITEDIR_0, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "010000", s_szDesktopTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "001000", s_szDesktopTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "000100", s_szDesktopTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "000010", s_szDesktopTestDir, L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction12 },
};

static const TEST_ENTRY s_group_08[] =
{
    { __LINE__, WRITEDIR_0, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "010000", s_szDesktopTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "001000", s_szDesktopTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "000100", s_szDesktopTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "000010", s_szDesktopTestDir, L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction12 },
};

static const TEST_ENTRY s_group_09[] =
{
    { __LINE__, WRITEDIR_0, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "010000", s_szDocumentTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "001000", s_szDocumentTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "000100", s_szDocumentTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "000010", s_szDocumentTestDir, L"", DoAction12 },
};

static const TEST_ENTRY s_group_10[] =
{
    { __LINE__, WRITEDIR_0, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "010000", s_szDocumentTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "001000", s_szDocumentTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "000100", s_szDocumentTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "000010", s_szDocumentTestDir, L"", DoAction12 },
};

LPCSTR PatternFromFlags(DWORD flags)
{
    static char s_buf[TYPE_RENAMEFOLDER + 1 + 1];
    DWORD i;
    for (i = 0; i <= TYPE_RENAMEFOLDER; ++i)
    {
        s_buf[i] = (char)('0' + !!(flags & (1 << i)));
    }
    s_buf[i] = 0;
    return s_buf;
}

static BOOL
DoGetPaths(LPWSTR pszPath1, LPWSTR pszPath2)
{
    pszPath1[0] = pszPath2[0] = 0;

    WCHAR szText[MAX_PATH * 2];
    szText[0] = 0;
    if (FILE *fp = fopen(TEMP_FILE, "rb"))
    {
        fread(szText, 1, sizeof(szText), fp);
        fclose(fp);
    }

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
        (*entry->action)(entry);
    }

    Sleep(30);

    DWORD flags = SendMessageW(s_hwnd, WM_GET_NOTIFY_FLAGS, 0, 0);
    LPCSTR pattern = PatternFromFlags(flags);

    SendMessageW(s_hwnd, WM_SET_PATHS, 0, 0);
    Sleep(100);

    WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH];
    szPath1[0] = szPath2[0] = 0;
    BOOL bOK = DoGetPaths(szPath1, szPath2);

    if (wcsstr(szPath1, L"Recent") != NULL)
    {
        skip("Recent written\n");
    }
    else
    {
        if (entry->pattern)
        {
            ok(lstrcmpA(pattern, entry->pattern) == 0,
               "Line %d: pattern mismatch '%s'\n", entry->line, pattern);
        }
        if (entry->path1)
            ok(bOK && lstrcmpiW(entry->path1, szPath1) == 0,
               "Line %d: path1 mismatch '%S' (%d)\n", entry->line, szPath1, bOK);
        if (entry->path2)
            ok(bOK && lstrcmpiW(entry->path2, szPath2) == 0,
               "Line %d: path2 mismatch '%S' (%d)\n", entry->line, szPath2, bOK);
    }

    SendMessageW(s_hwnd, WM_CLEAR_FLAGS, 0, 0);
}

static void DoEnd()
{
    DeleteFileA(TEMP_FILE);
    SendMessageW(s_hwnd, WM_COMMAND, IDOK, 0);
}

static BOOL CALLBACK HandlerRoutine(DWORD dwCtrlType)
{
    switch (dwCtrlType)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        {
            if (s_hThread)
            {
                TerminateThread(s_hThread, -1);
                s_hThread = NULL;
            }
            DoEnd();
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL
DoInit(void)
{
    LPWSTR psz;

    psz = GetWatchDir(WATCHDIR_DESKTOP);
    PathAppendW(psz, TEST_FILE);
    lstrcpynW(s_szDesktopTestFile, psz, _countof(s_szDesktopTestFile));

    psz = GetWatchDir(WATCHDIR_DESKTOP);
    PathAppendW(psz, TEST_FILE_RENAMED);
    lstrcpynW(s_szDesktopTestFileRenamed, psz, _countof(s_szDesktopTestFileRenamed));

    psz = GetWatchDir(WATCHDIR_DESKTOP);
    PathAppendW(psz, TEST_DIR);
    lstrcpynW(s_szDesktopTestDir, psz, _countof(s_szDesktopTestDir));

    psz = GetWatchDir(WATCHDIR_DESKTOP);
    PathAppendW(psz, TEST_DIR_RENAMED);
    lstrcpynW(s_szDesktopTestDirRenamed, psz, _countof(s_szDesktopTestDirRenamed));

    psz = GetWatchDir(WATCHDIR_MYDOCUMENTS);
    PathAppendW(psz, TEST_FILE);
    lstrcpynW(s_szDocumentTestFile, psz, _countof(s_szDocumentTestFile));

    psz = GetWatchDir(WATCHDIR_MYDOCUMENTS);
    PathAppendW(psz, TEST_FILE_RENAMED);
    lstrcpynW(s_szDocumentTestFileRenamed, psz, _countof(s_szDocumentTestFileRenamed));

    psz = GetWatchDir(WATCHDIR_MYDOCUMENTS);
    PathAppendW(psz, TEST_DIR);
    lstrcpynW(s_szDocumentTestDir, psz, _countof(s_szDocumentTestDir));

    psz = GetWatchDir(WATCHDIR_MYDOCUMENTS);
    PathAppendW(psz, TEST_DIR_RENAMED);
    lstrcpynW(s_szDocumentTestDirRenamed, psz, _countof(s_szDocumentTestDirRenamed));

    FILE *fp = fopen(TEMP_FILE, "wb");
    if (fp)
        fclose(fp);

    SetConsoleCtrlHandler(HandlerRoutine, TRUE);

    // close Explorer windows
    INT i, nCount = 50;
    trace("Closing Explorer windows...\n");
    for (i = 0; i < nCount; ++i)
    {
        HWND hwnd = FindWindowW(L"CabinetWClass", NULL);
        if (hwnd == NULL)
            break;

        PostMessage(hwnd, WM_CLOSE, 0, 0);
        Sleep(50);
    }

    if (i >= nCount)
        return FALSE;

    // close the CLASSNAME windows
    for (i = 0; i < nCount; ++i)
    {
        HWND hwnd = FindWindowW(CLASSNAME, CLASSNAME);
        if (hwnd == NULL)
            break;

        PostMessage(hwnd, WM_CLOSE, 0, 0);
        Sleep(50);
    }

    return (i < nCount);
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
        {
            return FALSE;
        }
    }

    return TRUE;
}

#define SOURCES_00   0
#define SOURCES_01   SHCNRF_ShellLevel
#define SOURCES_02   (SHCNRF_NewDelivery)
#define SOURCES_03   (SHCNRF_NewDelivery | SHCNRF_ShellLevel)
// TODO: SHCNRF_InterruptLevel, SHCNRF_RecursiveInterrupt

#define WATCHDIR_0 WATCHDIR_NULL
#define WATCHDIR_1 WATCHDIR_DESKTOP
#define WATCHDIR_2 WATCHDIR_MYCOMPUTER
#define WATCHDIR_3 WATCHDIR_MYDOCUMENTS

typedef struct TEST_GROUP
{
    INT line;
    UINT cEntries;
    const TEST_ENTRY *pEntries;
    BOOL fRecursive;
    INT nSources;
    WATCHDIR iWatchDir;
} TEST_GROUP;

static const TEST_GROUP s_groups[] =
{
    // fRecursive == FALSE.
    { __LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_00, WATCHDIR_0 },
    { __LINE__, _countof(s_group_05), s_group_05, FALSE, SOURCES_01, WATCHDIR_0 },
    { __LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_02, WATCHDIR_0 },
    { __LINE__, _countof(s_group_06), s_group_06, FALSE, SOURCES_03, WATCHDIR_0 },
    { __LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_00, WATCHDIR_1 },
    { __LINE__, _countof(s_group_07), s_group_07, FALSE, SOURCES_01, WATCHDIR_1 },
    { __LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_02, WATCHDIR_1 },
    { __LINE__, _countof(s_group_08), s_group_08, FALSE, SOURCES_03, WATCHDIR_1 },
    { __LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_00, WATCHDIR_2 },
    { __LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_01, WATCHDIR_2 },
    { __LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_02, WATCHDIR_2 },
    { __LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_03, WATCHDIR_2 },
    { __LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_00, WATCHDIR_3 },
    { __LINE__, _countof(s_group_09), s_group_09, FALSE, SOURCES_01, WATCHDIR_3 },
    { __LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_02, WATCHDIR_3 },
    { __LINE__, _countof(s_group_10), s_group_10, FALSE, SOURCES_03, WATCHDIR_3 },
    // fRecursive == TRUE.
    { __LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_00, WATCHDIR_0 },
    { __LINE__, _countof(s_group_01), s_group_01, TRUE, SOURCES_01, WATCHDIR_0 },
    { __LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_02, WATCHDIR_0 },
    { __LINE__, _countof(s_group_02), s_group_02, TRUE, SOURCES_03, WATCHDIR_0 },
    { __LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_00, WATCHDIR_1 },
    { __LINE__, _countof(s_group_01), s_group_01, TRUE, SOURCES_01, WATCHDIR_1 },
    { __LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_02, WATCHDIR_1 },
    { __LINE__, _countof(s_group_02), s_group_02, TRUE, SOURCES_03, WATCHDIR_1 },
    { __LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_00, WATCHDIR_2 },
    { __LINE__, _countof(s_group_01), s_group_01, TRUE, SOURCES_01, WATCHDIR_2 },
    { __LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_02, WATCHDIR_2 },
    { __LINE__, _countof(s_group_02), s_group_02, TRUE, SOURCES_03, WATCHDIR_2 },
    { __LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_00, WATCHDIR_3 },
    { __LINE__, _countof(s_group_03), s_group_03, TRUE, SOURCES_01, WATCHDIR_3 },
    { __LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_02, WATCHDIR_3 },
    { __LINE__, _countof(s_group_04), s_group_04, TRUE, SOURCES_03, WATCHDIR_3 },
};

static void DoTestGroup(const TEST_GROUP *pGroup)
{
    INT line = pGroup->line;
    UINT cEntries = pGroup->cEntries;
    const TEST_ENTRY *pEntries = pGroup->pEntries;
    BOOL fRecursive = pGroup->fRecursive;
    INT nSources = pGroup->nSources;
    WATCHDIR iWatchDir = pGroup->iWatchDir;
    trace("DoTestGroup: Line %d, fRecursive:%u, iWatchDir:%u, nSources:0x%08X\n",
          line, fRecursive, iWatchDir, nSources);

    WCHAR szParams[64];
    wsprintfW(szParams, L"%u,%u,%u", fRecursive, iWatchDir, nSources);

    HINSTANCE hinst = ShellExecuteW(NULL, NULL, s_szSubProgram, szParams, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)hinst <= 32)
    {
        skip("Unable to run shell32_apitest_sub.exe.\n");
        return;
    }

    for (int i = 0; i < 15; ++i)
    {
        s_hwnd = FindWindowW(CLASSNAME, CLASSNAME);
        if (s_hwnd)
            break;

        Sleep(50);
    }

    if (!s_hwnd)
    {
        skip("Unable to find window.\n");
        return;
    }

    for (UINT i = 0; i < cEntries; ++i)
        DoTestEntry(&pEntries[i]);

    DoEnd();

    for (int i = 0; i < 15; ++i)
    {
        s_hwnd = FindWindowW(CLASSNAME, CLASSNAME);
        if (!s_hwnd)
            break;

        Sleep(50);
    }
}

static unsigned __stdcall TestThreadProc(void *)
{
    for (UINT iGroup0 = 0; iGroup0 < _countof(s_groups); ++iGroup0)
    {
        for (UINT iGroup1 = iGroup0 + 1; iGroup1 < _countof(s_groups); ++iGroup1)
        {
            const TEST_GROUP *pGroup0 = &s_groups[iGroup0];
            const TEST_GROUP *pGroup1 = &s_groups[iGroup1];
            if (pGroup0->cEntries != pGroup1->cEntries)
                continue;
            if (memcmp(pGroup0, pGroup1, pGroup0->cEntries * sizeof(TEST_ENTRY)) == 0)
            {
                trace("Group %u and Group %u are same.\n", iGroup0, iGroup1);
            }
        }
    }
    for (UINT iGroup = 0; iGroup < _countof(s_groups); ++iGroup)
    {
        DoTestGroup(&s_groups[iGroup]);
    }
    return 0;
}

START_TEST(SHChangeNotify)
{
    if (!GetSubProgramPath())
    {
        skip("shell32_apitest_sub.exe not found\n");
    }

    if (!DoInit())
    {
        skip("Unable to initialize.\n");
        return;
    }

    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
    SendMessageW(s_hwnd, WM_CLEAR_FLAGS, 0, 0);

    trace("Please don't operate your PC while testing...\n");

    s_hThread = (HANDLE)_beginthreadex(NULL, 0, TestThreadProc, NULL, 0, NULL);
    WaitForSingleObject(s_hThread, INFINITE);
    CloseHandle(s_hThread);
}
