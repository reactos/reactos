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
#include <versionhelpers.h>
#include "SHChangeNotify.h"

//#define DO_TRIVIAL
//#define NO_INTERRUPT_LEVEL
//#define NO_SHELL_LEVEL
//#define SHELL_LEVEL_ONLY
//#define INTERRUPT_LEVEL_ONLY
#define NEW_DELIVERY_ONLY
//#define ENTRY_TICK
//#define GROUP_TICK
#define TOTAL_TICK

static HWND s_hwnd = NULL;
static WCHAR s_szSubProgram[MAX_PATH];
static HANDLE s_hThread = NULL;
static HANDLE s_hEvent = NULL;

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
#define MOVE_FLAGS          (MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)
static void
DoAction1(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, TEST_FILE);
    ok(DoCreateEmptyFile(pszPath), "Line %d: DoCreateEmptyFile failed\n", pEntry->line);
}

static void
DoAction2(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath1 = GetWatchDir(pEntry->iWriteDir);
    LPWSTR pszPath2 = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath1, TEST_FILE);
    PathAppendW(pszPath2, TEST_FILE_RENAMED);
    ok(MoveFileExW(pszPath1, pszPath2, MOVE_FLAGS), "Line %d: MoveFileExW(%ls, %ls) failed (%ld)\n",
       pEntry->line, pszPath1, pszPath2, GetLastError());
}

static void
DoAction3(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath1 = GetWatchDir(pEntry->iWriteDir);
    LPWSTR pszPath2 = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath1, TEST_FILE_RENAMED);
    PathAppendW(pszPath2, TEST_FILE);
    ok(MoveFileExW(pszPath1, pszPath2, MOVE_FLAGS), "Line %d: MoveFileExW(%ls, %ls) failed (%ld)\n",
       pEntry->line, pszPath1, pszPath2, GetLastError());
}

static void
DoAction4(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, TEST_FILE);
    ok(DeleteFileW(pszPath), "Line %d: DeleteFileW(%ls) failed (%ld)\n",
       pEntry->line, pszPath, GetLastError());
}

static void
DoAction5(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, TEST_DIR);
    ok(CreateDirectoryW(pszPath, NULL), "Line %d: CreateDirectoryW(%ls) failed (%ld)\n",
       pEntry->line, pszPath, GetLastError());
}

static void
DoAction6(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath1 = GetWatchDir(pEntry->iWriteDir);
    LPWSTR pszPath2 = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath1, TEST_DIR);
    PathAppendW(pszPath2, TEST_DIR_RENAMED);
    ok(MoveFileExW(pszPath1, pszPath2, MOVE_FLAGS), "Line %d: MoveFileExW(%ls, %ls) failed (%ld)\n",
       pEntry->line, pszPath1, pszPath2, GetLastError());
}

static void
DoAction7(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath1 = GetWatchDir(pEntry->iWriteDir);
    LPWSTR pszPath2 = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath1, TEST_DIR_RENAMED);
    PathAppendW(pszPath2, TEST_DIR);
    ok(MoveFileExW(pszPath1, pszPath2, MOVE_FLAGS), "Line %d: MoveFileExW(%ls, %ls) failed (%ld)\n",
       pEntry->line, pszPath1, pszPath2, GetLastError());
}

static void
DoAction8(const TEST_ENTRY *pEntry)
{
    LPWSTR pszPath = GetWatchDir(pEntry->iWriteDir);
    PathAppendW(pszPath, TEST_DIR);
    ok(RemoveDirectoryW(pszPath), "Line %d: RemoveDirectoryW(%ls) failed (%ld)\n",
       pEntry->line, pszPath, GetLastError());
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
static WCHAR s_szDesktop[MAX_PATH];
static WCHAR s_szDesktopTestFile[MAX_PATH];
static WCHAR s_szDesktopTestFileRenamed[MAX_PATH];
static WCHAR s_szDesktopTestDir[MAX_PATH];
static WCHAR s_szDesktopTestDirRenamed[MAX_PATH];

#define WRITEDIR_1 WATCHDIR_MYDOCUMENTS
static WCHAR s_szDocuments[MAX_PATH];
static WCHAR s_szDocumentTestFile[MAX_PATH];
static WCHAR s_szDocumentTestFileRenamed[MAX_PATH];
static WCHAR s_szDocumentTestDir[MAX_PATH];
static WCHAR s_szDocumentTestDirRenamed[MAX_PATH];

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
    { __LINE__, WRITEDIR_0, "0100000", s_szDesktopTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0010000", s_szDesktopTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0001000", s_szDesktopTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000100", s_szDesktopTestDir, L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "0100000", s_szDocumentTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0010000", s_szDocumentTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0001000", s_szDocumentTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000100", s_szDocumentTestDir, L"", DoAction12 },
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
    { __LINE__, WRITEDIR_0, "0100000", s_szDesktopTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0010000", s_szDesktopTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0001000", s_szDesktopTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000100", s_szDesktopTestDir, L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "0100000", s_szDocumentTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0010000", s_szDocumentTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0001000", s_szDocumentTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000100", s_szDocumentTestDir, L"", DoAction12 },
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
    { __LINE__, WRITEDIR_1, "0100000", s_szDocumentTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0010000", s_szDocumentTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0001000", s_szDocumentTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000100", s_szDocumentTestDir, L"", DoAction12 },
};

static const TEST_ENTRY s_group_04[] =
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
    { __LINE__, WRITEDIR_1, "0100000", s_szDocumentTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0010000", s_szDocumentTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0001000", s_szDocumentTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000100", s_szDocumentTestDir, L"", DoAction12 },
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
    { __LINE__, WRITEDIR_0, "0100000", s_szDesktopTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0010000", s_szDesktopTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0001000", s_szDesktopTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000100", s_szDesktopTestDir, L"", DoAction12 },

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

static const TEST_ENTRY s_group_06[] =
{
    { __LINE__, WRITEDIR_0, "0100000", s_szDesktopTestFile, L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "1000000", s_szDesktopTestFile, s_szDesktopTestFileRenamed, DoAction2 },
    { __LINE__, WRITEDIR_0, "1000000", s_szDesktopTestFileRenamed, s_szDesktopTestFile, DoAction3 },
    { __LINE__, WRITEDIR_0, "0010000", s_szDesktopTestFile, L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0001000", s_szDesktopTestDir, L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000010", s_szDesktopTestDir, s_szDesktopTestDirRenamed, DoAction6 },
    { __LINE__, WRITEDIR_0, "0000010", s_szDesktopTestDirRenamed, s_szDesktopTestDir, DoAction7 },
    { __LINE__, WRITEDIR_0, "0000100", s_szDesktopTestDir, L"", DoAction8 },
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

static const TEST_ENTRY s_group_07[] =
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

    { __LINE__, WRITEDIR_1, NULL, NULL, L"", DoAction1 }, // Why?
    { __LINE__, WRITEDIR_1, "1000000", s_szDocumentTestFile, s_szDocumentTestFileRenamed, DoAction2 },
    { __LINE__, WRITEDIR_1, "1000000", s_szDocumentTestFileRenamed, s_szDocumentTestFile, DoAction3 },
    { __LINE__, WRITEDIR_1, "0010000", s_szDocumentTestFile, L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "0001000", s_szDocumentTestDir, L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "0000010", s_szDocumentTestDir, s_szDocumentTestDirRenamed, DoAction6 },
    { __LINE__, WRITEDIR_1, "0000010", s_szDocumentTestDirRenamed, s_szDocumentTestDir, DoAction7 },
    { __LINE__, WRITEDIR_1, "0000100", s_szDocumentTestDir, L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction12 },
};

static const TEST_ENTRY s_group_08[] =
{
    { __LINE__, WRITEDIR_0, "0100000", s_szDesktopTestFile, L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "1000000", s_szDesktopTestFile, s_szDesktopTestFileRenamed, DoAction2 },
    { __LINE__, WRITEDIR_0, "1000000", s_szDesktopTestFileRenamed, s_szDesktopTestFile, DoAction3 },
    { __LINE__, WRITEDIR_0, "0010000", s_szDesktopTestFile, L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0001000", s_szDesktopTestDir, L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000010", s_szDesktopTestDir, s_szDesktopTestDirRenamed, DoAction6 },
    { __LINE__, WRITEDIR_0, "0000010", s_szDesktopTestDirRenamed, s_szDesktopTestDir, DoAction7 },
    { __LINE__, WRITEDIR_0, "0000100", s_szDesktopTestDir, L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "0100000", s_szDesktopTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0010000", s_szDesktopTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0001000", s_szDesktopTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000100", s_szDesktopTestDir, L"", DoAction12 },

    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction2 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction3 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction6 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction7 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "0100000", s_szDocumentTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0010000", s_szDocumentTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0001000", s_szDocumentTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000100", s_szDocumentTestDir, L"", DoAction12 },
};

static const TEST_ENTRY s_group_09[] =
{
    { __LINE__, WRITEDIR_0, "0100000", s_szDesktopTestFile, L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "1000000", s_szDesktopTestFile, s_szDesktopTestFileRenamed, DoAction2 },
    { __LINE__, WRITEDIR_0, "1000000", s_szDesktopTestFileRenamed, s_szDesktopTestFile, DoAction3 },
    { __LINE__, WRITEDIR_0, "0010000", s_szDesktopTestFile, L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0001000", s_szDesktopTestDir, L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000010", s_szDesktopTestDir, s_szDesktopTestDirRenamed, DoAction6 },
    { __LINE__, WRITEDIR_0, "0000010", s_szDesktopTestDirRenamed, s_szDesktopTestDir, DoAction7 },
    { __LINE__, WRITEDIR_0, "0000100", s_szDesktopTestDir, L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "0100000", s_szDesktopTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0010000", s_szDesktopTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0001000", s_szDesktopTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000100", s_szDesktopTestDir, L"", DoAction12 },

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

static const TEST_ENTRY s_group_10[] =
{
    { __LINE__, WRITEDIR_0, "0100000", s_szDesktopTestFile, L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "1000000", s_szDesktopTestFile, s_szDesktopTestFileRenamed, DoAction2 },
    { __LINE__, WRITEDIR_0, "1000000", s_szDesktopTestFileRenamed, s_szDesktopTestFile, DoAction3 },
    { __LINE__, WRITEDIR_0, "0010000", s_szDesktopTestFile, L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0001000", s_szDesktopTestDir, L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000010", s_szDesktopTestDir, s_szDesktopTestDirRenamed, DoAction6 },
    { __LINE__, WRITEDIR_0, "0000010", s_szDesktopTestDirRenamed, s_szDesktopTestDir, DoAction7 },
    { __LINE__, WRITEDIR_0, "0000100", s_szDesktopTestDir, L"", DoAction8 },
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

static const TEST_ENTRY s_group_11[] =
{
    { __LINE__, WRITEDIR_0, "0100000", s_szDesktopTestFile, L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "1000000", s_szDesktopTestFile, s_szDesktopTestFileRenamed, DoAction2 },
    { __LINE__, WRITEDIR_0, "1000000", s_szDesktopTestFileRenamed, s_szDesktopTestFile, DoAction3 },
    { __LINE__, WRITEDIR_0, "0010000", s_szDesktopTestFile, L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0001000", s_szDesktopTestDir, L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000010", s_szDesktopTestDir, s_szDesktopTestDirRenamed, DoAction6 },
    { __LINE__, WRITEDIR_0, "0000010", s_szDesktopTestDirRenamed, s_szDesktopTestDir, DoAction7 },
    { __LINE__, WRITEDIR_0, "0000100", s_szDesktopTestDir, L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "0100000", s_szDesktopTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0010000", s_szDesktopTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0001000", s_szDesktopTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000100", s_szDesktopTestDir, L"", DoAction12 },

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

static const TEST_ENTRY s_group_12[] =
{
    { __LINE__, WRITEDIR_0, "0100000", s_szDesktopTestFile, L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "1000000", s_szDesktopTestFile, s_szDesktopTestFileRenamed, DoAction2 },
    { __LINE__, WRITEDIR_0, "1000000", s_szDesktopTestFileRenamed, s_szDesktopTestFile, DoAction3 },
    { __LINE__, WRITEDIR_0, "0010000", s_szDesktopTestFile, L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0001000", s_szDesktopTestDir, L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000010", s_szDesktopTestDir, s_szDesktopTestDirRenamed, DoAction6 },
    { __LINE__, WRITEDIR_0, "0000010", s_szDesktopTestDirRenamed, s_szDesktopTestDir, DoAction7 },
    { __LINE__, WRITEDIR_0, "0000100", s_szDesktopTestDir, L"", DoAction8 },
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

static const TEST_ENTRY s_group_13[] =
{
    { __LINE__, WRITEDIR_0, "0100000", s_szDesktopTestFile, L"", DoAction1 },
    { __LINE__, WRITEDIR_0, "1000000", s_szDesktopTestFile, s_szDesktopTestFileRenamed, DoAction2 },
    { __LINE__, WRITEDIR_0, "1000000", s_szDesktopTestFileRenamed, s_szDesktopTestFile, DoAction3 },
    { __LINE__, WRITEDIR_0, "0010000", s_szDesktopTestFile, L"", DoAction4 },
    { __LINE__, WRITEDIR_0, "0001000", s_szDesktopTestDir, L"", DoAction5 },
    { __LINE__, WRITEDIR_0, "0000010", s_szDesktopTestDir, s_szDesktopTestDirRenamed, DoAction6 },
    { __LINE__, WRITEDIR_0, "0000010", s_szDesktopTestDirRenamed, s_szDesktopTestDir, DoAction7 },
    { __LINE__, WRITEDIR_0, "0000100", s_szDesktopTestDir, L"", DoAction8 },
    { __LINE__, WRITEDIR_0, "0100000", s_szDesktopTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_0, "0010000", s_szDesktopTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_0, "0001000", s_szDesktopTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_0, "0000100", s_szDesktopTestDir, L"", DoAction12 },

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

static const TEST_ENTRY s_group_14[] =
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

    { __LINE__, WRITEDIR_1, "0100000", s_szDocumentTestFile, L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "1000000", s_szDocumentTestFile, s_szDocumentTestFileRenamed, DoAction2 },
    { __LINE__, WRITEDIR_1, "1000000", s_szDocumentTestFileRenamed, s_szDocumentTestFile, DoAction3 },
    { __LINE__, WRITEDIR_1, "0010000", s_szDocumentTestFile, L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "0001000", s_szDocumentTestDir, L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "0000010", s_szDocumentTestDir, s_szDocumentTestDirRenamed, DoAction6 },
    { __LINE__, WRITEDIR_1, "0000010", s_szDocumentTestDirRenamed, s_szDocumentTestDir, DoAction7 },
    { __LINE__, WRITEDIR_1, "0000100", s_szDocumentTestDir, L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000000", L"", L"", DoAction12 },
};

static const TEST_ENTRY s_group_15[] =
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

    { __LINE__, WRITEDIR_1, "0100000", s_szDocumentTestFile, L"", DoAction1 },
    { __LINE__, WRITEDIR_1, "1000000", s_szDocumentTestFile, s_szDocumentTestFileRenamed, DoAction2 },
    { __LINE__, WRITEDIR_1, "1000000", s_szDocumentTestFileRenamed, s_szDocumentTestFile, DoAction3 },
    { __LINE__, WRITEDIR_1, "0010000", s_szDocumentTestFile, L"", DoAction4 },
    { __LINE__, WRITEDIR_1, "0001000", s_szDocumentTestDir, L"", DoAction5 },
    { __LINE__, WRITEDIR_1, "0000010", s_szDocumentTestDir, s_szDocumentTestDirRenamed, DoAction6 },
    { __LINE__, WRITEDIR_1, "0000010", s_szDocumentTestDirRenamed, s_szDocumentTestDir, DoAction7 },
    { __LINE__, WRITEDIR_1, "0000100", s_szDocumentTestDir, L"", DoAction8 },
    { __LINE__, WRITEDIR_1, "0100000", s_szDocumentTestFile, L"", DoAction9 },
    { __LINE__, WRITEDIR_1, "0010000", s_szDocumentTestFile, L"", DoAction10 },
    { __LINE__, WRITEDIR_1, "0001000", s_szDocumentTestDir, L"", DoAction11 },
    { __LINE__, WRITEDIR_1, "0000100", s_szDocumentTestDir, L"", DoAction12 },
};

LPCSTR PatternFromFlags(DWORD flags)
{
    static char s_buf[TYPE_MAX + 1];
    DWORD i;
    for (i = 0; i <= TYPE_MAX; ++i)
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
DoTestEntry(INT iEntry, const TEST_ENTRY *entry, INT nSources)
{
    DWORD flags;
    LPCSTR pattern;
#ifdef ENTRY_TICK
    DWORD dwOldTick = GetTickCount();
#endif

    if (entry->action)
    {
        entry->action(entry);
    }

    if (nSources & SHCNRF_InterruptLevel)
    {
        // The event won't work at here. Manually waiting...
        UINT cTry = ((iEntry == 0) ? 100 : 50);
        for (UINT iTry = 0; iTry < cTry; ++iTry)
        {
            flags = SendMessageW(s_hwnd, WM_GET_NOTIFY_FLAGS, 0, 0);
            pattern = PatternFromFlags(flags);
            if (strcmp(pattern, "0000000") != 0)
                break;

            Sleep(50);
        }
    }
    else
    {
        if (WaitForSingleObject(s_hEvent, 100) == WAIT_OBJECT_0)
        {
            Sleep(50);
        }

        flags = SendMessageW(s_hwnd, WM_GET_NOTIFY_FLAGS, 0, 0);
        pattern = PatternFromFlags(flags);
    }

    SendMessageW(s_hwnd, WM_SET_PATHS, 0, 0);

    WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH];
    szPath1[0] = szPath2[0] = 0;
    BOOL bOK = DoGetPaths(szPath1, szPath2);

    if (wcsstr(szPath1, L"Recent") != NULL)
    {
        skip("Recent written\n");
    }
    else if (pattern[TYPE_UPDATEDIR] == '1')
    {
        trace("Line %d: SHCNE_UPDATEDIR: Calm down...\n", entry->line);

        // Calm down.
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

static void DoEnd(void)
{
    SendMessageW(s_hwnd, WM_COMMAND, IDOK, 0);
    DeleteFileA(TEMP_FILE);
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
        {
            DoAbortThread();
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
    lstrcpynW(s_szDesktop, psz, _countof(s_szDesktop));

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
    lstrcpynW(s_szDocuments, psz, _countof(s_szDocuments));

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

#define SOURCES_00  0
#define SOURCES_01  SHCNRF_ShellLevel
#define SOURCES_02  (SHCNRF_NewDelivery)
#define SOURCES_03  (SHCNRF_NewDelivery | SHCNRF_ShellLevel)
#define SOURCES_04  SHCNRF_InterruptLevel
#define SOURCES_05  (SHCNRF_InterruptLevel | SHCNRF_ShellLevel)
#define SOURCES_06  (SHCNRF_InterruptLevel | SHCNRF_NewDelivery)
#define SOURCES_07  (SHCNRF_InterruptLevel | SHCNRF_NewDelivery | SHCNRF_ShellLevel)
#define SOURCES_08  (SHCNRF_RecursiveInterrupt | SHCNRF_InterruptLevel)
#define SOURCES_09  (SHCNRF_RecursiveInterrupt | SHCNRF_InterruptLevel | SHCNRF_ShellLevel)
#define SOURCES_10  (SHCNRF_RecursiveInterrupt | SHCNRF_InterruptLevel | SHCNRF_NewDelivery)
#define SOURCES_11  (SHCNRF_RecursiveInterrupt | SHCNRF_InterruptLevel | SHCNRF_NewDelivery | SHCNRF_ShellLevel)

#define WATCHDIR_0 WATCHDIR_NULL
#define WATCHDIR_1 WATCHDIR_DESKTOP
#define WATCHDIR_2 WATCHDIR_MYCOMPUTER
#define WATCHDIR_3 WATCHDIR_MYDOCUMENTS

static void DoWaitForWindow(BOOL bClosing)
{
    for (INT i = 0; i < 15; ++i)
    {
        s_hwnd = FindWindowW(CLASSNAME, CLASSNAME);
        if (bClosing)
        {
            if (!s_hwnd)
                break;
        }
        else
        {
            if (s_hwnd)
                break;
        }

        Sleep(50);
    }
}

static void
DoTestGroup(INT line, UINT cEntries, const TEST_ENTRY *pEntries, BOOL fRecursive,
            INT nSources, WATCHDIR iWatchDir)
{
#ifdef NO_INTERRUPT_LEVEL
    if (nSources & SHCNRF_InterruptLevel)
        return;
#endif
#ifdef NO_SHELL_LEVEL
    if (nSources & SHCNRF_ShellLevel)
        return;
#endif
#ifdef INTERRUPT_LEVEL_ONLY
    if (!(nSources & SHCNRF_InterruptLevel))
        return;
#endif
#ifdef SHELL_LEVEL_ONLY
    if (!(nSources & SHCNRF_ShellLevel))
        return;
#endif
#ifdef NEW_DELIVERY_ONLY
    if (!(nSources & SHCNRF_NewDelivery))
        return;
#endif
#ifdef GROUP_TICK
    DWORD dwOldTick = GetTickCount();
#endif
    trace("DoTestGroup: Line %d: fRecursive:%u, iWatchDir:%u, nSources:0x%X\n",
          line, fRecursive, iWatchDir, nSources);

    if (s_hEvent)
    {
        CloseHandle(s_hEvent);
        s_hEvent = NULL;
    }
    s_hEvent = CreateEventA(NULL, TRUE, FALSE, EVENT_NAME);

    WCHAR szParams[64];
    wsprintfW(szParams, L"%u,%u,%u", fRecursive, iWatchDir, nSources);

    HINSTANCE hinst = ShellExecuteW(NULL, NULL, s_szSubProgram, szParams, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)hinst <= 32)
    {
        skip("Unable to run shell32_apitest_sub.exe.\n");
        return;
    }

    DoWaitForWindow(FALSE);

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
            DoEnd();
            break;
        }

        DoTestEntry(i, &pEntries[i], nSources);
    }

    DoEnd();

    DoWaitForWindow(TRUE);

    if (s_hEvent)
    {
        CloseHandle(s_hEvent);
        s_hEvent = NULL;
    }

    DeleteFileW(s_szDesktopTestFile);
    DeleteFileW(s_szDesktopTestFileRenamed);
    DeleteFileW(s_szDocumentTestFile);
    DeleteFileW(s_szDocumentTestFileRenamed);
    RemoveDirectoryW(s_szDesktopTestDir);
    RemoveDirectoryW(s_szDesktopTestDirRenamed);
    RemoveDirectoryW(s_szDocumentTestDir);
    RemoveDirectoryW(s_szDocumentTestDirRenamed);

#ifdef GROUP_TICK
    DWORD dwNewTick = GetTickCount();
    DWORD dwTick = dwNewTick - dwOldTick;
    trace("DoTestGroup: Line %d: %lu.%lu sec\n", line, (dwTick / 1000), (dwTick / 100 % 10));
#endif
}

static unsigned __stdcall TestThreadProc(void *)
{
    // fRecursive == FALSE.
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_00, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_01), s_group_01, FALSE, SOURCES_01, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_02, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_01), s_group_01, FALSE, SOURCES_03, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, FALSE, SOURCES_04, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, FALSE, SOURCES_05, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, FALSE, SOURCES_06, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, FALSE, SOURCES_07, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, FALSE, SOURCES_08, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, FALSE, SOURCES_09, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, FALSE, SOURCES_10, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, FALSE, SOURCES_11, WATCHDIR_0);

    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_00, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_05), s_group_05, FALSE, SOURCES_01, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_02, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_05), s_group_05, FALSE, SOURCES_03, WATCHDIR_1);
    if (IsWindowsXPOrGreater() && !IsWindowsVistaOrGreater())
        DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_04, WATCHDIR_1);
    else
        DoTestGroup(__LINE__, _countof(s_group_10), s_group_10, FALSE, SOURCES_04, WATCHDIR_1); // NG
    DoTestGroup(__LINE__, _countof(s_group_09), s_group_09, FALSE, SOURCES_05, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_10), s_group_10, FALSE, SOURCES_06, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_11), s_group_11, FALSE, SOURCES_07, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_12), s_group_12, FALSE, SOURCES_08, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_13), s_group_13, FALSE, SOURCES_09, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_12), s_group_12, FALSE, SOURCES_10, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_13), s_group_13, FALSE, SOURCES_11, WATCHDIR_1);

#ifdef DO_TRIVIAL
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_00, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_01, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_02, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_03, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_04, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_05, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_06, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_07, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_08, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_09, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_10, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_11, WATCHDIR_2);
#endif

    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_00, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_03), s_group_03, FALSE, SOURCES_01, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, FALSE, SOURCES_02, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_03), s_group_03, FALSE, SOURCES_03, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_14), s_group_14, FALSE, SOURCES_04, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_15), s_group_15, FALSE, SOURCES_05, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_14), s_group_14, FALSE, SOURCES_06, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_15), s_group_15, FALSE, SOURCES_07, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_14), s_group_14, FALSE, SOURCES_08, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_15), s_group_15, FALSE, SOURCES_09, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_14), s_group_14, FALSE, SOURCES_10, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_15), s_group_15, FALSE, SOURCES_11, WATCHDIR_3);

    // fRecursive == TRUE.
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_00, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_01), s_group_01, TRUE, SOURCES_01, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_02, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_02), s_group_02, TRUE, SOURCES_03, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SOURCES_04, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SOURCES_05, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_12), s_group_12, TRUE, SOURCES_06, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SOURCES_07, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SOURCES_08, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SOURCES_09, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_10), s_group_10, TRUE, SOURCES_10, WATCHDIR_0);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SOURCES_11, WATCHDIR_0);

    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_00, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_01), s_group_01, TRUE, SOURCES_01, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_02, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_02), s_group_02, TRUE, SOURCES_03, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SOURCES_04, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SOURCES_05, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_12), s_group_12, TRUE, SOURCES_06, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SOURCES_07, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SOURCES_08, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SOURCES_09, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_10), s_group_10, TRUE, SOURCES_10, WATCHDIR_1);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SOURCES_11, WATCHDIR_1);

    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_00, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_01), s_group_01, TRUE, SOURCES_01, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_02, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_02), s_group_02, TRUE, SOURCES_03, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SOURCES_04, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SOURCES_05, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_12), s_group_12, TRUE, SOURCES_06, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SOURCES_07, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_06), s_group_06, TRUE, SOURCES_08, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SOURCES_09, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_10), s_group_10, TRUE, SOURCES_10, WATCHDIR_2);
    DoTestGroup(__LINE__, _countof(s_group_08), s_group_08, TRUE, SOURCES_11, WATCHDIR_2);

    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_00, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_03), s_group_03, TRUE, SOURCES_01, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_00), s_group_00, TRUE, SOURCES_02, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_04), s_group_04, TRUE, SOURCES_03, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_07), s_group_07, TRUE, SOURCES_04, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_15), s_group_15, TRUE, SOURCES_05, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_14), s_group_14, TRUE, SOURCES_06, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_15), s_group_15, TRUE, SOURCES_07, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_14), s_group_14, TRUE, SOURCES_08, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_15), s_group_15, TRUE, SOURCES_09, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_14), s_group_14, TRUE, SOURCES_10, WATCHDIR_3);
    DoTestGroup(__LINE__, _countof(s_group_15), s_group_15, TRUE, SOURCES_11, WATCHDIR_3);

    return 0;
}

START_TEST(SHChangeNotify)
{
#ifdef TOTAL_TICK
    DWORD dwOldTick = GetTickCount();
#endif

    if (!GetSubProgramPath())
    {
        skip("shell32_apitest_sub.exe not found\n");
    }

    if (!DoInit())
    {
        skip("Unable to initialize.\n");
        return;
    }

    trace("Please don't operate your PC while testing...\n");

    s_hThread = (HANDLE)_beginthreadex(NULL, 0, TestThreadProc, NULL, 0, NULL);
    WaitForSingleObject(s_hThread, INFINITE);
    CloseHandle(s_hThread);

#ifdef TOTAL_TICK
    DWORD dwNewTick = GetTickCount();
    DWORD dwTick = dwNewTick - dwOldTick;
    trace("SHChangeNotify: Total %lu.%lu sec\n", (dwTick / 1000), (dwTick / 100 % 10));
#endif
}
