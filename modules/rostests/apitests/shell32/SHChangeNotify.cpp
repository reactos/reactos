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
#include "SHChangeNotify.h"

static HWND s_hwnd = NULL;
static WCHAR s_szSubProgram[MAX_PATH];

static BOOL DoCreateEmptyFile(LPCWSTR pszFileName)
{
    FILE *fp = _wfopen(pszFileName, L"wb");
    fclose(fp);
    return fp != NULL;
}

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
} TEST_ENTRY, *LPTEST_ENTRY;

void DoAction1(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, L"_TEST_.txt");
    DoCreateEmptyFile(pszPath);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

void DoAction2(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath1 = GetWatchDir(pEntry->iWriteDir);
    LPWSTR pszPath2 = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath1, L"_TEST_.txt");
    PathAppendW(pszPath2, L"_TEST_RENAMED_.txt");
    MoveFileW(pszPath1, pszPath2);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

void DoAction3(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath1 = GetWatchDir(pEntry->iWriteDir);
    LPWSTR pszPath2 = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath1, L"_TEST_RENAMED_.txt");
    PathAppendW(pszPath2, L"_TEST_.txt");
    MoveFileW(pszPath1, pszPath2);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

void DoAction4(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, L"_TEST_.txt");
    DeleteFileW(pszPath);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

void DoAction5(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, L"_TESTDIR_");
    CreateDirectoryW(pszPath, NULL);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

void DoAction6(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath1 = GetWatchDir(pEntry->iWriteDir);
    LPWSTR pszPath2 = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath1, L"_TESTDIR_");
    PathAppendW(pszPath2, L"_TESTDIR_RENAMED_");
    MoveFileW(pszPath1, pszPath2);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

void DoAction7(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath1 = GetWatchDir(pEntry->iWriteDir);
    LPWSTR pszPath2 = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath1, L"_TESTDIR_RENAMED_");
    PathAppendW(pszPath2, L"_TESTDIR_");
    MoveFileW(pszPath1, pszPath2);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

void DoAction8(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, L"_TESTDIR_");
    RemoveDirectoryW(pszPath);
    SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);
}

void DoAction9(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, L"_TEST_.txt");
    SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW | SHCNF_FLUSH, pszPath, NULL);
}

void DoAction10(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, L"_TEST_.txt");
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW | SHCNF_FLUSH, pszPath, NULL);
}

void DoAction11(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, L"_TESTDIR_");
    SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW | SHCNF_FLUSH, pszPath, NULL);
}

void DoAction12(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, L"_TESTDIR_");
    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHW | SHCNF_FLUSH, pszPath, NULL);
}

#define WRITEDIR_0 WATCHDIR_DESKTOP
#define WRITEDIR_1 WATCHDIR_MYDOCUMENTS

static WCHAR s_szDesktopTestFile[MAX_PATH];
static WCHAR s_szDesktopTestFileRenamed[MAX_PATH];
static WCHAR s_szDesktopTestDir[MAX_PATH];
static WCHAR s_szDesktopTestDirRenamed[MAX_PATH];
static WCHAR s_szDocumentTestFile[MAX_PATH];
static WCHAR s_szDocumentTestFileRenamed[MAX_PATH];
static WCHAR s_szDocumentTestDir[MAX_PATH];
static WCHAR s_szDocumentTestDirRenamed[MAX_PATH];

static const TEST_ENTRY s_entries_0[] =
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

static const TEST_ENTRY s_entries_1[] =
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

static const TEST_ENTRY s_entries_2[] =
{
    { __LINE__, WRITEDIR_0, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_0, "010000", s_szDesktopTestFile, L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "100000", s_szDesktopTestFile, s_szDesktopTestFileRenamed, DoAction2 },
    { __LINE__, WRITEDIR_0, "100000", s_szDesktopTestFileRenamed, s_szDesktopTestFile, DoAction3 },
    { __LINE__, WRITEDIR_0, "001000", s_szDesktopTestFile, L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "000100", s_szDesktopTestDir, L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "000001", s_szDesktopTestDir, s_szDesktopTestDirRenamed, DoAction6 },
    { __LINE__, WRITEDIR_0, "000001", s_szDesktopTestDirRenamed, s_szDesktopTestDir, DoAction7 },
    { __LINE__, WRITEDIR_0, "000010", s_szDesktopTestDir, L"", DoAction8 },
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

static const TEST_ENTRY s_entries_3[] =
{
    { __LINE__, WRITEDIR_0, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", NULL },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_0, "000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "000100", s_szDesktopTestDir, L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "000001", s_szDesktopTestDir, s_szDesktopTestDirRenamed, DoAction6 },
    { __LINE__, WRITEDIR_0, "000001", s_szDesktopTestDirRenamed, s_szDesktopTestDir, DoAction7 },
    { __LINE__, WRITEDIR_0, "000010", s_szDesktopTestDir, L"", DoAction8 },
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

static const TEST_ENTRY s_entries_5[] =
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

static const TEST_ENTRY s_entries_7[] =
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

static const TEST_ENTRY s_entries_9[] =
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

static const TEST_ENTRY s_entries_11[] =
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

static const TEST_ENTRY s_entries_15[] =
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

static const TEST_ENTRY s_entries_21[] =
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

static const TEST_ENTRY s_entries_25[] =
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

static const TEST_ENTRY s_entries_31[] =
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

static const TEST_ENTRY s_entries_35[] =
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

static BOOL DoGetPaths(LPWSTR pszPath1, LPWSTR pszPath2)
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


static void DoTestEntry(const TEST_ENTRY *entry)
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

static BOOL DoInit(void)
{
    LPWSTR psz;

    psz = GetWatchDir(WATCHDIR_DESKTOP);
    PathAppendW(psz, L"_TEST_.txt");
    lstrcpynW(s_szDesktopTestFile, psz, _countof(s_szDesktopTestFile));

    psz = GetWatchDir(WATCHDIR_DESKTOP);
    PathAppendW(psz, L"_TEST_RENAMED_.txt");
    lstrcpynW(s_szDesktopTestFileRenamed, psz, _countof(s_szDesktopTestFileRenamed));

    psz = GetWatchDir(WATCHDIR_DESKTOP);
    PathAppendW(psz, L"_TESTDIR_");
    lstrcpynW(s_szDesktopTestDir, psz, _countof(s_szDesktopTestDir));

    psz = GetWatchDir(WATCHDIR_DESKTOP);
    PathAppendW(psz, L"_TESTDIR_RENAMED_");
    lstrcpynW(s_szDesktopTestDirRenamed, psz, _countof(s_szDesktopTestDirRenamed));

    psz = GetWatchDir(WATCHDIR_MYDOCUMENTS);
    PathAppendW(psz, L"_TEST_.txt");
    lstrcpynW(s_szDocumentTestFile, psz, _countof(s_szDocumentTestFile));

    psz = GetWatchDir(WATCHDIR_MYDOCUMENTS);
    PathAppendW(psz, L"_TEST_RENAMED_.txt");
    lstrcpynW(s_szDocumentTestFileRenamed, psz, _countof(s_szDocumentTestFileRenamed));

    psz = GetWatchDir(WATCHDIR_MYDOCUMENTS);
    PathAppendW(psz, L"_TESTDIR_");
    lstrcpynW(s_szDocumentTestDir, psz, _countof(s_szDocumentTestDir));

    psz = GetWatchDir(WATCHDIR_MYDOCUMENTS);
    PathAppendW(psz, L"_TESTDIR_RENAMED_");
    lstrcpynW(s_szDocumentTestDirRenamed, psz, _countof(s_szDocumentTestDirRenamed));

    if (FILE *fp = fopen(TEMP_FILE, "wb"))
    {
        fclose(fp);
    }

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

    if (i >= nCount)
        return FALSE;

    return TRUE;
}

static void DoEnd(HWND hwnd)
{
    DeleteFileA(TEMP_FILE);
    SendMessageW(s_hwnd, WM_COMMAND, IDOK, 0);
}

static void
JustDoIt(INT line, UINT cEntries, const TEST_ENTRY *pEntries, INT nSources,
         BOOL fRecursive, WATCHDIR iWatchDir)
{
    trace("JustDoIt: Line %d, fRecursive:%u, iWatchDir:%u, nSources:0x%08X\n",
          line, fRecursive, iWatchDir, nSources);

    WCHAR szParams[128];
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

    DoEnd(s_hwnd);

    for (int i = 0; i < 15; ++i)
    {
        s_hwnd = FindWindowW(CLASSNAME, CLASSNAME);
        if (!s_hwnd)
            break;

        Sleep(50);
    }
}

#define SOURCES_00   0
#define SOURCES_01   SHCNRF_ShellLevel
#define SOURCES_02   SHCNRF_InterruptLevel
#define SOURCES_03   (SHCNRF_InterruptLevel | SHCNRF_ShellLevel)
#define SOURCES_04   (SHCNRF_NewDelivery)
#define SOURCES_05   (SHCNRF_NewDelivery | SHCNRF_ShellLevel)
#define SOURCES_06   (SHCNRF_NewDelivery | SHCNRF_InterruptLevel | SHCNRF_ShellLevel)
// TODO: SHCNRF_RecursiveInterrupt

static BOOL GetSubProgramPath(void)
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

#define WATCHDIR_0 WATCHDIR_NULL
#define WATCHDIR_1 WATCHDIR_DESKTOP
#define WATCHDIR_2 WATCHDIR_MYCOMPUTER
#define WATCHDIR_3 WATCHDIR_MYDOCUMENTS

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

    // fRecursive == TRUE.
    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_00, TRUE, WATCHDIR_0);
    JustDoIt(__LINE__, _countof(s_entries_1), s_entries_1, SOURCES_01, TRUE, WATCHDIR_0);
    //JustDoIt(__LINE__, _countof(s_entries_2), s_entries_2, SOURCES_02, TRUE, WATCHDIR_0);
    //JustDoIt(__LINE__, _countof(s_entries_3), s_entries_3, SOURCES_03, TRUE, WATCHDIR_0);
    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_04, TRUE, WATCHDIR_0);
    JustDoIt(__LINE__, _countof(s_entries_5), s_entries_5, SOURCES_05, TRUE, WATCHDIR_0);
    //JustDoIt(__LINE__, _countof(s_entries_6), s_entries_6, SOURCES_06, TRUE, WATCHDIR_0);

    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_00, TRUE, WATCHDIR_1);
    JustDoIt(__LINE__, _countof(s_entries_1), s_entries_1, SOURCES_01, TRUE, WATCHDIR_1);
    //JustDoIt(__LINE__, _countof(s_entries_2), s_entries_2, SOURCES_02, TRUE, WATCHDIR_1);
    //JustDoIt(__LINE__, _countof(s_entries_3), s_entries_3, SOURCES_03, TRUE, WATCHDIR_1);
    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_04, TRUE, WATCHDIR_1);
    JustDoIt(__LINE__, _countof(s_entries_5), s_entries_5, SOURCES_05, TRUE, WATCHDIR_1);
    //JustDoIt(__LINE__, _countof(s_entries_6), s_entries_6, SOURCES_06, TRUE, WATCHDIR_1);

    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_00, TRUE, WATCHDIR_2);
    JustDoIt(__LINE__, _countof(s_entries_1), s_entries_1, SOURCES_01, TRUE, WATCHDIR_2);
    //JustDoIt(__LINE__, _countof(s_entries_2), s_entries_2, SOURCES_02, TRUE, WATCHDIR_2);
    //JustDoIt(__LINE__, _countof(s_entries_3), s_entries_3, SOURCES_03, TRUE, WATCHDIR_2);
    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_04, TRUE, WATCHDIR_2);
    JustDoIt(__LINE__, _countof(s_entries_5), s_entries_5, SOURCES_05, TRUE, WATCHDIR_2);
    //JustDoIt(__LINE__, _countof(s_entries_6), s_entries_6, SOURCES_06, TRUE, WATCHDIR_2);

    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_00, TRUE, WATCHDIR_3);
    JustDoIt(__LINE__, _countof(s_entries_7), s_entries_7, SOURCES_01, TRUE, WATCHDIR_3);
    //JustDoIt(__LINE__, _countof(s_entries_8), s_entries_8, SOURCES_02, TRUE, WATCHDIR_3);
    //JustDoIt(__LINE__, _countof(s_entries_10), s_entries_10, SOURCES_03, TRUE, WATCHDIR_3);
    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_04, TRUE, WATCHDIR_3);
    JustDoIt(__LINE__, _countof(s_entries_9), s_entries_9, SOURCES_05, TRUE, WATCHDIR_3);
    //JustDoIt(__LINE__, _countof(s_entries_10), s_entries_10, SOURCES_06, TRUE, WATCHDIR_3);

    // fRecursive == FALSE.
    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_00, FALSE, WATCHDIR_0);
    JustDoIt(__LINE__, _countof(s_entries_11), s_entries_11, SOURCES_01, FALSE, WATCHDIR_0);
    //JustDoIt(__LINE__, _countof(s_entries_12), s_entries_12, SOURCES_02, FALSE, WATCHDIR_0);
    //JustDoIt(__LINE__, _countof(s_entries_13), s_entries_13, SOURCES_03, FALSE, WATCHDIR_0);
    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_04, FALSE, WATCHDIR_0);
    JustDoIt(__LINE__, _countof(s_entries_15), s_entries_15, SOURCES_05, FALSE, WATCHDIR_0);
    //JustDoIt(__LINE__, _countof(s_entries_16), s_entries_16, SOURCES_06, FALSE, WATCHDIR_0);

    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_00, FALSE, WATCHDIR_1);
    JustDoIt(__LINE__, _countof(s_entries_21), s_entries_21, SOURCES_01, FALSE, WATCHDIR_1);
    //JustDoIt(__LINE__, _countof(s_entries_22), s_entries_22, SOURCES_02, FALSE, WATCHDIR_1);
    //JustDoIt(__LINE__, _countof(s_entries_23), s_entries_23, SOURCES_03, FALSE, WATCHDIR_1);
    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_04, FALSE, WATCHDIR_1);
    JustDoIt(__LINE__, _countof(s_entries_25), s_entries_25, SOURCES_05, FALSE, WATCHDIR_1);
    //JustDoIt(__LINE__, _countof(s_entries_26), s_entries_26, SOURCES_06, FALSE, WATCHDIR_1);

    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_00, FALSE, WATCHDIR_2);
    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_01, FALSE, WATCHDIR_2);
    //JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_02, FALSE, WATCHDIR_2);
    //JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_03, FALSE, WATCHDIR_2);
    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_04, FALSE, WATCHDIR_2);
    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_05, FALSE, WATCHDIR_2);
    //JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_06, FALSE, WATCHDIR_2);

    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_00, FALSE, WATCHDIR_3);
    JustDoIt(__LINE__, _countof(s_entries_31), s_entries_31, SOURCES_01, FALSE, WATCHDIR_3);
    //JustDoIt(__LINE__, _countof(s_entries_32), s_entries_32, SOURCES_02, FALSE, WATCHDIR_3);
    //JustDoIt(__LINE__, _countof(s_entries_33), s_entries_33, SOURCES_03, FALSE, WATCHDIR_3);
    JustDoIt(__LINE__, _countof(s_entries_0), s_entries_0, SOURCES_04, FALSE, WATCHDIR_3);
    JustDoIt(__LINE__, _countof(s_entries_35), s_entries_35, SOURCES_05, FALSE, WATCHDIR_3);
    //JustDoIt(__LINE__, _countof(s_entries_36), s_entries_36, SOURCES_06, FALSE, WATCHDIR_3);
}
