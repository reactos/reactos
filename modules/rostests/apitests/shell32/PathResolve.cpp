/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for PathResolve
 * PROGRAMMER:      Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shelltest.h"

#include <stdio.h>
#include <assert.h>

/*
 * NOTE: "App Paths" registry key and PATHEXT environment variable
 *       have no effect for PathResolve.
 */

/* PathResolve */
typedef int (WINAPI *PATHRESOLVE)(LPWSTR, LPWSTR*, UINT);

/* IsLFNDriveW */
typedef BOOL (WINAPI *ISLFNDRIVEW)(LPCWSTR);

static HINSTANCE    s_hShell32 = NULL;
static PATHRESOLVE  s_pPathResolve = NULL;
static ISLFNDRIVEW  s_pIsLFNDriveW = NULL;
static WCHAR        s_TestDir[MAX_PATH];
static WCHAR        s_TestDirWithBackslash[MAX_PATH];
static WCHAR        s_ShortcutLongName[MAX_PATH];
static WCHAR        s_LinkTarget[MAX_PATH];
static WCHAR        s_LinkTargetWithBackslash[MAX_PATH];
static WCHAR        s_LinkTargetDoubleBackslash[MAX_PATH];
static LPWSTR       s_Dirs[2] = { s_TestDir, NULL };
static LPWSTR       s_DirsWithBackslash[2] = { s_TestDirWithBackslash, NULL };

/* PathResolve flags */
#ifndef PRF_VERIFYEXISTS
    #define PRF_VERIFYEXISTS         0x01
    #define PRF_EXECUTABLE           0x02
    #define PRF_TRYPROGRAMEXTENSIONS (PRF_EXECUTABLE | PRF_VERIFYEXISTS)
    #define PRF_FIRSTDIRDEF          0x04
    #define PRF_DONTFINDLNK          0x08
#endif
#ifndef PRF_REQUIREABSOLUTE
    #define PRF_REQUIREABSOLUTE      0x10
#endif

/* Abstraction of PathResolve flags to manage the test entries */
#define FLAGS0      0
#define FLAGS1      PRF_VERIFYEXISTS
#define FLAGS2      PRF_EXECUTABLE
#define FLAGS3      PRF_TRYPROGRAMEXTENSIONS
#define FLAGS4      (PRF_FIRSTDIRDEF | PRF_VERIFYEXISTS)
#define FLAGS5      (PRF_FIRSTDIRDEF | PRF_EXECUTABLE)
#define FLAGS6      (PRF_FIRSTDIRDEF | PRF_TRYPROGRAMEXTENSIONS)
#define FLAGS7      (PRF_REQUIREABSOLUTE | PRF_VERIFYEXISTS)
#define FLAGS8      (PRF_REQUIREABSOLUTE | PRF_EXECUTABLE)
#define FLAGS9      (PRF_REQUIREABSOLUTE | PRF_TRYPROGRAMEXTENSIONS)
#define FLAGS10     (PRF_REQUIREABSOLUTE | PRF_FIRSTDIRDEF | PRF_VERIFYEXISTS)
#define FLAGS11     (PRF_REQUIREABSOLUTE | PRF_FIRSTDIRDEF | PRF_EXECUTABLE)
#define FLAGS12     (PRF_REQUIREABSOLUTE | PRF_FIRSTDIRDEF | PRF_TRYPROGRAMEXTENSIONS)
#define FLAGS13     0xFFFFFFFF

/* The test entry structure */
typedef struct tagTEST_ENTRY
{
    INT         LineNumber;    /* # */
    INT         Ret;
    DWORD       Error;
    UINT        EF_;
    LPCWSTR     NameBefore;
    LPCWSTR     NameExpected;
    UINT        Flags;
    LPWSTR     *Dirs;
} TEST_ENTRY, *PTEST_ENTRY;

/* Flags for TEST_ENTRY */
#define EF_FULLPATH     0x01
#define EF_TESTDATA     0x02
#define EF_TYPE_MASK    0x0F
#define EF_NAME_ONLY    0x10
#define EF_APP_PATH     0x20

#define RET_IGNORE 0x00BEF00D

/* Special error codes */
#define ERR_NO_CHANGE 0xBEEF      /* Error Code 48879 */
#define ERR_DEAD      0xDEAD      /* Error Code 57005 */
#define ERR_IGNORE    0x7F7F7F7F  /* Ignore Error Code */
#define RAISED        9999        /* exception raised */

/* The test entries for long file name (LFN) */
static const TEST_ENTRY s_LFNEntries[] =
{
    /* null path */
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, NULL, NULL, FLAGS0 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, NULL, NULL, FLAGS1 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, NULL, NULL, FLAGS2 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, NULL, NULL, FLAGS3 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, NULL, NULL, FLAGS4 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, NULL, NULL, FLAGS5 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, NULL, NULL, FLAGS6 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, NULL, NULL, FLAGS7 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, NULL, NULL, FLAGS8 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, NULL, NULL, FLAGS9 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, NULL, NULL, FLAGS10 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, NULL, NULL, FLAGS11 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, NULL, NULL, FLAGS12 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, NULL, NULL, FLAGS13 },
    /* empty path */
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L"", NULL, FLAGS0 },
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"", NULL, FLAGS1 },
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"", NULL, FLAGS2 },
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"", NULL, FLAGS3 },
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"", NULL, FLAGS4 },
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"", NULL, FLAGS5 },
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"", NULL, FLAGS6 },
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"", NULL, FLAGS7 },
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"", NULL, FLAGS8 },
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"", NULL, FLAGS9 },
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"", NULL, FLAGS10 },
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"", NULL, FLAGS11 },
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"", NULL, FLAGS12 },
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"", NULL, FLAGS13 },
    /* invalid name */
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"invalid name", L"invalid name", FLAGS0 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"invalid name", L"invalid name", FLAGS1 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"invalid name", L"invalid name", FLAGS2 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"invalid name", L"invalid name", FLAGS3 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"invalid name", L"invalid name", FLAGS4 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"invalid name", L"invalid name", FLAGS5 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"invalid name", L"invalid name", FLAGS6 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"invalid name", L"invalid name", FLAGS7 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"invalid name", L"invalid name", FLAGS8 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"invalid name", L"invalid name", FLAGS9 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"invalid name", L"invalid name", FLAGS10 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"invalid name", L"invalid name", FLAGS11 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"invalid name", L"invalid name", FLAGS12 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"invalid name", L"invalid name", FLAGS13 },
    /* testdir/2PRONG (path) */
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"2PRONG", L"2PRONG", FLAGS0 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG", L"2PRONG", FLAGS1 },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"2PRONG", L"2PRONG", FLAGS2 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG", L"2PRONG", FLAGS3 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"2PRONG", L"2PRONG", FLAGS4 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"2PRONG", L"2PRONG", FLAGS5 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"2PRONG", L"2PRONG", FLAGS6 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG", L"2PRONG", FLAGS7 },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"2PRONG", L"2PRONG", FLAGS8 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG", L"2PRONG", FLAGS9 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"2PRONG", L"2PRONG", FLAGS10 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"2PRONG", L"2PRONG", FLAGS11 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"2PRONG", L"2PRONG", FLAGS12 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"2PRONG", L"2PRONG", FLAGS13 },
    /* testdir/2PRONG (name only) */
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS0 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS1 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS2 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS3 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS4 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS5 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS6 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS7 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS8 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS9 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS10 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS11 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS12 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS13 },
    /* testdir/2PRONG with dirs (name only) */
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS0, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS1, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS2, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS3, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS4, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS5, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS6, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS7, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS8, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS9, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS10, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS11, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS12, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG", NULL, FLAGS13, s_Dirs },
    /* testdir/2PRONG (name only, app path) */
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"2PRONG", L"2PRONG", FLAGS0 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"2PRONG", L"2PRONG", FLAGS1 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"2PRONG", L"2PRONG", FLAGS2 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"2PRONG", L"2PRONG", FLAGS3 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"2PRONG", L"2PRONG", FLAGS4 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"2PRONG", L"2PRONG", FLAGS5 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"2PRONG", L"2PRONG", FLAGS6 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"2PRONG", L"2PRONG", FLAGS7 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"2PRONG", L"2PRONG", FLAGS8 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"2PRONG", L"2PRONG", FLAGS9 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"2PRONG", L"2PRONG", FLAGS10 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"2PRONG", L"2PRONG", FLAGS11 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"2PRONG", L"2PRONG", FLAGS12 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"2PRONG", L"2PRONG", FLAGS13 },
    /* 2PRONG.txt */
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS0 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS1 },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS2 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS3 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS4 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS5 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS6 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS7 },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS8 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS9 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS10 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS11 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS12 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS13 },
    /* 2PRONG.txt with dirs */
    { __LINE__, 1, ERR_IGNORE, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS0, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS1, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS2, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS3, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS4, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS5, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS6, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS7, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS8, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS9, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS10, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS11, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS12, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"2PRONG.txt", L"2PRONG.txt", FLAGS13, s_Dirs },
    /* 2PRONG.txt with dirs (path) */
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS0, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS1, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS2, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS3, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS4, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS5, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS6, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS7, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS8, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS9, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS10, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS11, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS12, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS13, s_Dirs },
    /* 2PRONG.txt (with a trailing backslash) */
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS0 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS1 },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS2 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS3 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_LinkTargetWithBackslash, NULL, FLAGS4 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_LinkTargetWithBackslash, NULL, FLAGS5 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_LinkTargetWithBackslash, NULL, FLAGS6 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS7 },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS8 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS9 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_LinkTargetWithBackslash, NULL, FLAGS10 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_LinkTargetWithBackslash, NULL, FLAGS11 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_LinkTargetWithBackslash, NULL, FLAGS12 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_LinkTargetWithBackslash, NULL, FLAGS13 },
    /* 2PRONG.txt with dirs (with a trailing backslash) */
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS0, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS1, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS2, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS3, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS4, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS5, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS6, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS7, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS8, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS9, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS10, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS11, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS12, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS13, s_Dirs },
    /* 2PRONG.txt with dirs (with a trailing backslash) */
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS0, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS1, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS2, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS3, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS4, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS5, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS6, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS7, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS8, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS9, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS10, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS11, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS12, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"2PRONG.txt", s_LinkTarget, FLAGS13, s_Dirs },
    /* 2PRONG.txt with dirs (with a trailing backslash) (s_DirsWithBackslash) */
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS0, s_DirsWithBackslash },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS1, s_DirsWithBackslash },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS2, s_DirsWithBackslash },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS3, s_DirsWithBackslash },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS4, s_DirsWithBackslash },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS5, s_DirsWithBackslash },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS6, s_DirsWithBackslash },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS7, s_DirsWithBackslash },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS8, s_DirsWithBackslash },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS9, s_DirsWithBackslash },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS10, s_DirsWithBackslash },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS11, s_DirsWithBackslash },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS12, s_DirsWithBackslash },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetWithBackslash, s_LinkTarget, FLAGS13, s_DirsWithBackslash },
    /* 2PRONG.txt (double backslash) */
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS0 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS1 },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS2 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS3 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_LinkTargetDoubleBackslash, NULL, FLAGS4 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_LinkTargetDoubleBackslash, NULL, FLAGS5 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_LinkTargetDoubleBackslash, NULL, FLAGS6 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS7 },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS8 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS9 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_LinkTargetDoubleBackslash, NULL, FLAGS10 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_LinkTargetDoubleBackslash, NULL, FLAGS11 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_LinkTargetDoubleBackslash, NULL, FLAGS12 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_LinkTargetDoubleBackslash, NULL, FLAGS13 },
    /* 2PRONG.txt with dirs (double backslash) */
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS0, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS1, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS2, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS3, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS4, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS5, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS6, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS7, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS8, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS9, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS10, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS11, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS12, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_LinkTargetDoubleBackslash, s_LinkTarget, FLAGS13, s_Dirs },
    /* 2PRONG.txt (name only) */
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", NULL, FLAGS0 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", NULL, FLAGS1 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", NULL, FLAGS2 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", NULL, FLAGS3 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", NULL, FLAGS4 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", NULL, FLAGS5 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", NULL, FLAGS6 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", NULL, FLAGS7 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", NULL, FLAGS8 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", NULL, FLAGS9 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", NULL, FLAGS10 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", NULL, FLAGS11 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", NULL, FLAGS12 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", NULL, FLAGS13 },
    /* .\2PRONG.txt with dirs (path) */
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L".\\2PRONG.txt", NULL, FLAGS0, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\2PRONG.txt", NULL, FLAGS1, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L".\\2PRONG.txt", NULL, FLAGS2, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\2PRONG.txt", NULL, FLAGS3, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\2PRONG.txt", s_LinkTarget, FLAGS4, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L".\\2PRONG.txt", NULL, FLAGS5, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\2PRONG.txt", NULL, FLAGS6, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\2PRONG.txt", NULL, FLAGS7, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L".\\2PRONG.txt", NULL, FLAGS8, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\2PRONG.txt", NULL, FLAGS9, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\2PRONG.txt", s_LinkTarget, FLAGS10, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L".\\2PRONG.txt", NULL, FLAGS11, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\2PRONG.txt", s_LinkTarget, FLAGS12, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\2PRONG.txt", s_LinkTarget, FLAGS13, s_Dirs },
    /* .\\2PRONG.txt with dirs (path) */
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L".\\\\2PRONG.txt", NULL, FLAGS0, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\\\2PRONG.txt", NULL, FLAGS1, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L".\\\\2PRONG.txt", NULL, FLAGS2, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\\\2PRONG.txt", NULL, FLAGS3, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\\\2PRONG.txt", s_LinkTarget, FLAGS4, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L".\\\\2PRONG.txt", NULL, FLAGS5, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\\\2PRONG.txt", NULL, FLAGS6, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\\\2PRONG.txt", NULL, FLAGS7, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L".\\\\2PRONG.txt", NULL, FLAGS8, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\\\2PRONG.txt", NULL, FLAGS9, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\\\2PRONG.txt", s_LinkTarget, FLAGS10, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L".\\\\2PRONG.txt", NULL, FLAGS11, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\\\2PRONG.txt", s_LinkTarget, FLAGS12, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L".\\\\2PRONG.txt", s_LinkTarget, FLAGS13, s_Dirs },
    /* .\..\.\testdir\..\testdir\2PRONG.txt with dirs */
    { __LINE__, 1, ERR_IGNORE, EF_TESTDATA, L".\\..\\.\\testdir\\..\\testdir\\2PRONG.txt", L"2PRONG.txt", FLAGS0, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L".\\..\\.\\testdir\\..\\testdir\\2PRONG.txt", L"2PRONG.txt", FLAGS1, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L".\\..\\.\\testdir\\..\\testdir\\2PRONG.txt", L"2PRONG.txt", FLAGS2, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L".\\..\\.\\testdir\\..\\testdir\\2PRONG.txt", L"2PRONG.txt", FLAGS3, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L".\\..\\.\\testdir\\..\\testdir\\2PRONG.txt", L"2PRONG.txt", FLAGS4, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L".\\..\\.\\testdir\\..\\testdir\\2PRONG.txt", L"2PRONG.txt", FLAGS5, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L".\\..\\.\\testdir\\..\\testdir\\2PRONG.txt", L"2PRONG.txt", FLAGS6, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L".\\..\\.\\testdir\\..\\testdir\\2PRONG.txt", L"2PRONG.txt", FLAGS7, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L".\\..\\.\\testdir\\..\\testdir\\2PRONG.txt", L"2PRONG.txt", FLAGS8, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L".\\..\\.\\testdir\\..\\testdir\\2PRONG.txt", L"2PRONG.txt", FLAGS9, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L".\\..\\.\\testdir\\..\\testdir\\2PRONG.txt", L"2PRONG.txt", FLAGS10, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L".\\..\\.\\testdir\\..\\testdir\\2PRONG.txt", L"2PRONG.txt", FLAGS11, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L".\\..\\.\\testdir\\..\\testdir\\2PRONG.txt", L"2PRONG.txt", FLAGS12, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L".\\..\\.\\testdir\\..\\testdir\\2PRONG.txt", L"2PRONG.txt", FLAGS13, s_Dirs },
    /* ..\testdir\.\..\testdir\.\.\2PRONG.txt with dirs (path) */
    { __LINE__, 1, ERR_IGNORE, EF_FULLPATH, L"..\\testdir\\.\\..\\testdir\\.\\.\\2PRONG.txt", NULL, FLAGS0, s_Dirs },
    { __LINE__, RET_IGNORE, ERR_IGNORE, EF_FULLPATH, L"..\\testdir\\.\\..\\testdir\\.\\.\\2PRONG.txt", NULL, FLAGS1, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L"..\\testdir\\.\\..\\testdir\\.\\.\\2PRONG.txt", NULL, FLAGS2, s_Dirs },
    { __LINE__, RET_IGNORE, ERR_IGNORE, EF_FULLPATH, L"..\\testdir\\.\\..\\testdir\\.\\.\\2PRONG.txt", NULL, FLAGS3, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"..\\testdir\\.\\..\\testdir\\.\\.\\2PRONG.txt", s_LinkTarget, FLAGS4, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L"..\\testdir\\.\\..\\testdir\\.\\.\\2PRONG.txt", s_LinkTarget, FLAGS5, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"..\\testdir\\.\\..\\testdir\\.\\.\\2PRONG.txt", s_LinkTarget, FLAGS6, s_Dirs },
    { __LINE__, RET_IGNORE, ERR_IGNORE, EF_FULLPATH, L"..\\testdir\\.\\..\\testdir\\.\\.\\2PRONG.txt", NULL, FLAGS7, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L"..\\testdir\\.\\..\\testdir\\.\\.\\2PRONG.txt", NULL, FLAGS8, s_Dirs },
    { __LINE__, RET_IGNORE, ERR_IGNORE, EF_FULLPATH, L"..\\testdir\\.\\..\\testdir\\.\\.\\2PRONG.txt", NULL, FLAGS9, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"..\\testdir\\.\\..\\testdir\\.\\.\\2PRONG.txt", s_LinkTarget, FLAGS10, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, L"..\\testdir\\.\\..\\testdir\\.\\.\\2PRONG.txt", s_LinkTarget, FLAGS11, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"..\\testdir\\.\\..\\testdir\\.\\.\\2PRONG.txt", s_LinkTarget, FLAGS12, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, L"..\\testdir\\.\\..\\testdir\\.\\.\\2PRONG.txt", s_LinkTarget, FLAGS13, s_Dirs },
    /* 2PRONG.txt with dirs (name only) */
    { __LINE__, 1, ERR_IGNORE, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", L"2PRONG.txt", FLAGS0, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", L"2PRONG.txt", FLAGS1, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", L"2PRONG.txt", FLAGS2, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", L"2PRONG.txt", FLAGS3, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", L"2PRONG.txt", FLAGS4, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", L"2PRONG.txt", FLAGS5, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", L"2PRONG.txt", FLAGS6, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", L"2PRONG.txt", FLAGS7, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", L"2PRONG.txt", FLAGS8, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", L"2PRONG.txt", FLAGS9, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", L"2PRONG.txt", FLAGS10, s_Dirs },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", L"2PRONG.txt", FLAGS11, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", L"2PRONG.txt", FLAGS12, s_Dirs },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"2PRONG.txt", L"2PRONG.txt", FLAGS13, s_Dirs },
    /* testdir/CmdLineUtils (path) */
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS0 },
    { __LINE__, 1, ERR_IGNORE, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils.lnk", FLAGS1 },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS2 },
    { __LINE__, 1, ERR_IGNORE, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils.lnk", FLAGS3 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS4 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS5 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS6 },
    { __LINE__, 1, ERR_IGNORE, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils.lnk", FLAGS7 },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS8 },
    { __LINE__, 1, ERR_IGNORE, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils.lnk", FLAGS9 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS10 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS11 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS12 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS13 },
    /* testdir/CmdLineUtils with PRF_DONTFINDLNK (path) */
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS0 | PRF_DONTFINDLNK },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS1 | PRF_DONTFINDLNK },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS2 | PRF_DONTFINDLNK },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS3 | PRF_DONTFINDLNK },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS4 | PRF_DONTFINDLNK },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS5 | PRF_DONTFINDLNK },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS6 | PRF_DONTFINDLNK },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS7 | PRF_DONTFINDLNK },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS8 | PRF_DONTFINDLNK },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS9 | PRF_DONTFINDLNK },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS10 | PRF_DONTFINDLNK },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS11 | PRF_DONTFINDLNK },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS12 | PRF_DONTFINDLNK },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils", L"CmdLineUtils", FLAGS13 | PRF_DONTFINDLNK },
    /* testdir/CmdLineUtils (name only) */
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, FLAGS0 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, FLAGS1 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, FLAGS2 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, FLAGS3 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, FLAGS4 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, FLAGS5 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, FLAGS6 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, FLAGS7 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, FLAGS8 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, FLAGS9 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, FLAGS10 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, FLAGS11 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, FLAGS12 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils", NULL, FLAGS13 },
    /* testdir/CmdLineUtils.exe (path) */
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe", FLAGS0 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe", FLAGS1 },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe", FLAGS2 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe", FLAGS3 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe", FLAGS4 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe", FLAGS5 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe", FLAGS6 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe", FLAGS7 },
    { __LINE__, 1, ERR_NO_CHANGE, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe", FLAGS8 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe", FLAGS9 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe", FLAGS10 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe", FLAGS11 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe", FLAGS12 },
    { __LINE__, RAISED, ERR_DEAD, EF_TESTDATA, L"CmdLineUtils.exe", L"CmdLineUtils.exe", FLAGS13 },
    /* testdir/CmdLineUtils.exe (name only) */
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS0 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS1 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS2 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS3 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS4 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS5 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS6 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS7 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS8 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS9 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS10 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS11 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS12 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS13 },
    /* testdir/CmdLineUtils.exe with dirs (name only) */
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS0, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS1, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS2, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS3, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS4, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS5, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS6, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS7, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS8, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS9, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS10, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS11, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS12, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS13, s_Dirs },
    /* testdir/CmdLineUtils.exe with dirs (name only) */
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS0, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS1, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS2, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS3, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS4, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS5, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS6, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS7, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS8, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS9, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS10, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS11, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS12, s_Dirs },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY, L"CmdLineUtils.exe", NULL, FLAGS13, s_Dirs },
    /* GhostProgram.exe -> testdir/CmdLineUtils.exe (name only, app path) */
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"GhostProgram.exe", L"CmdLineUtils.exe", FLAGS0 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"GhostProgram.exe", L"CmdLineUtils.exe", FLAGS1 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"GhostProgram.exe", L"CmdLineUtils.exe", FLAGS2 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"GhostProgram.exe", L"CmdLineUtils.exe", FLAGS3 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"GhostProgram.exe", L"CmdLineUtils.exe", FLAGS4 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"GhostProgram.exe", L"CmdLineUtils.exe", FLAGS5 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"GhostProgram.exe", L"CmdLineUtils.exe", FLAGS6 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"GhostProgram.exe", L"CmdLineUtils.exe", FLAGS7 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"GhostProgram.exe", L"CmdLineUtils.exe", FLAGS8 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"GhostProgram.exe", L"CmdLineUtils.exe", FLAGS9 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"GhostProgram.exe", L"CmdLineUtils.exe", FLAGS10 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"GhostProgram.exe", L"CmdLineUtils.exe", FLAGS11 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"GhostProgram.exe", L"CmdLineUtils.exe", FLAGS12 },
    { __LINE__, 0, ERROR_FILE_NOT_FOUND, EF_TESTDATA | EF_NAME_ONLY | EF_APP_PATH, L"GhostProgram.exe", L"CmdLineUtils.exe", FLAGS13 },
    /* CmdLineUtils.lnk */
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS0 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS1 },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_ShortcutLongName, NULL, FLAGS2 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS3 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS4 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS5 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS6 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS7 },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS8 },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS9 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS10 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS11 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS12 },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS13 },
    /* CmdLineUtils.lnk (with PRF_DONTFINDLNK) */
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS0 | PRF_DONTFINDLNK },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS1 | PRF_DONTFINDLNK },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_ShortcutLongName, NULL, FLAGS2 | PRF_DONTFINDLNK },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS3 | PRF_DONTFINDLNK },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS4 | PRF_DONTFINDLNK },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS5 | PRF_DONTFINDLNK },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS6 | PRF_DONTFINDLNK },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS7 | PRF_DONTFINDLNK },
    { __LINE__, 1, ERR_NO_CHANGE, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS8 | PRF_DONTFINDLNK },
    { __LINE__, 1, ERROR_FILE_NOT_FOUND, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS9 | PRF_DONTFINDLNK },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS10 | PRF_DONTFINDLNK },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS11 | PRF_DONTFINDLNK },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS12 | PRF_DONTFINDLNK },
    { __LINE__, RAISED, ERR_DEAD, EF_FULLPATH, s_ShortcutLongName, s_ShortcutLongName, FLAGS13 | PRF_DONTFINDLNK },
};

static BOOL
CreateShortcut(LPCWSTR pszLnkFileName,
               LPCWSTR pszTargetPathName)
{
    IPersistFile *ppf;
    IShellLinkW* psl;
    HRESULT hres;

    hres = CoInitialize(NULL);
    if (SUCCEEDED(hres))
    {
        hres = CoCreateInstance(CLSID_ShellLink, NULL,
            CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&psl);
        if (SUCCEEDED(hres))
        {
            psl->SetPath(pszTargetPathName);
            hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
            if (SUCCEEDED(hres))
            {
                hres = ppf->Save(pszLnkFileName, TRUE);
                ppf->Release();
            }
            psl->Release();
        }
        CoUninitialize();
    }
    SetLastError(hres);

    return SUCCEEDED(hres);
}

static BOOL
CreateRegAppPath(INT SectionNumber, INT LineNumber, const WCHAR* Name, const WCHAR* Value)
{
    HKEY RegistryKey;
    LONG Result;
    WCHAR Buffer[1024];
    DWORD Disposition;

    wcscpy(Buffer, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\");
    wcscat(Buffer, Name);
    Result = RegCreateKeyExW(HKEY_LOCAL_MACHINE, Buffer, 0, NULL,
        0, KEY_WRITE, NULL, &RegistryKey, &Disposition);
    if (Result != ERROR_SUCCESS)
    {
        trace("Section %d, Line %d: Could not create test key. Status: %lu\n",
              SectionNumber, LineNumber, Result);
        return FALSE;
    }
    Result = RegSetValueW(RegistryKey, NULL, REG_SZ, Value, 0);
    if (Result != ERROR_SUCCESS)
    {
        trace("Section %d, Line %d: Could not set value of the test key. Status: %lu\n",
              SectionNumber, LineNumber, Result);
        RegCloseKey(RegistryKey);
        return FALSE;
    }
    RegCloseKey(RegistryKey);
    return TRUE;
}

static BOOL
DeleteRegAppPath(INT SectionNumber, INT LineNumber, const WCHAR* Name)
{
    LONG Result;
    WCHAR Buffer[1024];
    wcscpy(Buffer, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\");
    wcscat(Buffer, Name);
    Result = RegDeleteKeyW(HKEY_LOCAL_MACHINE, Buffer);
    if (Result != ERROR_SUCCESS)
    {
        trace("Section %d, Line %d: Could not remove the test key. Status: %lu\n",
              SectionNumber, LineNumber, Result);
        return FALSE;
    }
    return TRUE;
}

static void DoEntry(INT SectionNumber, INT LineNumber, const TEST_ENTRY *pEntry)
{
    WCHAR Path[MAX_PATH], PathExpected[MAX_PATH];
    INT Ret;
    DWORD Error;

    ZeroMemory(Path, sizeof(Path));
    ZeroMemory(PathExpected, sizeof(PathExpected));

    if (pEntry->NameBefore == NULL)
    {
        assert(pEntry->NameExpected == NULL);
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
                lstrcpyW(Path, s_TestDir);
                lstrcatW(Path, L"\\");
                lstrcatW(Path, pEntry->NameBefore);
            }

            if (pEntry->NameExpected)
            {
                lstrcpyW(PathExpected, s_TestDir);
                lstrcatW(PathExpected, L"\\");
                lstrcatW(PathExpected, pEntry->NameExpected);
            }
            break;

        default:
            assert(0);
            break;
    }

    if (pEntry->EF_ & EF_APP_PATH)
    {
        if (!CreateRegAppPath(SectionNumber, LineNumber, pEntry->NameBefore, PathExpected))
        {
            skip("Section %d, Line %d: CreateRegAppPath failure\n", SectionNumber, LineNumber);
            return;
        }
    }

    _SEH2_TRY
    {
        SetLastError(ERR_NO_CHANGE);
        if (pEntry->NameBefore)
        {
            Ret = (*s_pPathResolve)(Path, pEntry->Dirs, pEntry->Flags);
        }
        else
        {
            Ret = (*s_pPathResolve)(NULL, pEntry->Dirs, pEntry->Flags);
        }
        Error = GetLastError();
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Ret = RAISED;
        Error = ERR_DEAD;
    }
    _SEH2_END;

    if (pEntry->EF_ & EF_APP_PATH)
    {
        ok(DeleteRegAppPath(SectionNumber, LineNumber, pEntry->NameBefore),
           "Section %d, Line %d: DeleteRegAppPath failed\n", SectionNumber, LineNumber);
    }

    if (pEntry->Ret != RET_IGNORE)
    {
        ok(Ret == pEntry->Ret, "Section %d, Line %d: Ret expected %d, was %d.\n",
           SectionNumber, LineNumber, pEntry->Ret, Ret);
    }

    if (pEntry->Error != ERR_IGNORE)
    {
        ok(Error == pEntry->Error, "Section %d, Line %d: last error expected %ld, was %ld.\n",
           SectionNumber, LineNumber, pEntry->Error, Error);
    }

    if (pEntry->NameExpected && !(pEntry->EF_ & EF_APP_PATH))
    {
        char expected[MAX_PATH];
        char path[MAX_PATH];

        lstrcpynA(expected, wine_dbgstr_w(PathExpected), _countof(expected));
        lstrcpynA(path, wine_dbgstr_w(Path), _countof(path));

        ok(lstrcmpW(Path, PathExpected) == 0, "Section %d, Line %d: Path expected %s, was %s.\n",
           SectionNumber, LineNumber, expected, path);
    }
}

static void TestMain_PathResolve(void)
{
    UINT i, cEntries;
    const TEST_ENTRY *pEntries;
    WCHAR Saved[128], *pPathExtSaved;

    pEntries = s_LFNEntries;
    cEntries = _countof(s_LFNEntries);

    /* save PATHEXT */
    if (GetEnvironmentVariableW(L"PATHEXT", Saved, _countof(Saved)))
        pPathExtSaved = Saved;
    else
        pPathExtSaved = NULL;

    /* Section 1 */
    for (i = 0; i < cEntries; ++i)
    {
        DoEntry(1, pEntries[i].LineNumber, &pEntries[i]);
    }

    /* Section 2: reset PATHEXT */
    if (SetEnvironmentVariableW(L"PATHEXT", NULL))
    {
        for (i = 0; i < cEntries; ++i)
        {
            DoEntry(2, pEntries[i].LineNumber, &pEntries[i]);
        }
    }
    else
    {
        skip("SetEnvironmentVariableW failed\n");
    }

    /* Section 3: set PATHEXT to ".COM;.EXE;.BAT" */
    if (SetEnvironmentVariableW(L"PATHEXT", L".COM;.EXE;.BAT"))
    {
        for (i = 0; i < cEntries; ++i)
        {
            DoEntry(3, pEntries[i].LineNumber, &pEntries[i]);
        }
    }
    else
    {
        skip("SetEnvironmentVariableW failed\n");
    }

    /* Section 4: set PATHEXT to ".TXT" */
    if (SetEnvironmentVariableW(L"PATHEXT", L".TXT"))
    {
        for (i = 0; i < cEntries; ++i)
        {
            DoEntry(4, pEntries[i].LineNumber, &pEntries[i]);
        }
    }
    else
    {
        skip("SetEnvironmentVariableW failed\n");
    }

    /* restore PATHEXT */
    SetEnvironmentVariableW(L"PATHEXT", pPathExtSaved);
}

START_TEST(PathResolve)
{
    LPWSTR pch;
    WCHAR szRoot[MAX_PATH];

    /* Get this program's path */
    GetModuleFileNameW(NULL, s_TestDir, _countof(s_TestDir));

    /* Add '\testdir' to the path */
    pch = wcsrchr(s_TestDir, L'\\');
    if (pch == NULL)
    {
        skip("GetModuleFileName and/or wcsrchr are insane.\n");
        return;
    }
    lstrcpyW(pch, L"\\testdir");

    /* Create the testdir directory */
    CreateDirectoryW(s_TestDir, NULL);
    if (GetFileAttributesW(s_TestDir) == INVALID_FILE_ATTRIBUTES)
    {
        skip("testdir is not found.\n");
        return;
    }

    /* Build s_TestDirWithBackslash path */
    lstrcpyW(s_TestDirWithBackslash, s_TestDir);
    lstrcatW(s_TestDirWithBackslash, L"\\");

    /* Build s_LinkTarget path */
    lstrcpyW(s_LinkTarget, s_TestDir);
    lstrcatW(s_LinkTarget, L"\\");
    lstrcatW(s_LinkTarget, L"2PRONG.txt");

    /* Create the file */
    fclose(_wfopen(s_LinkTarget, L"wb"));
    ok(GetFileAttributesW(s_LinkTarget) != INVALID_FILE_ATTRIBUTES, "s_LinkTarget not found\n");

    /* Build s_LinkTargetWithBackslash path */
    lstrcpyW(s_LinkTargetWithBackslash, s_TestDir);
    lstrcatW(s_LinkTargetWithBackslash, L"\\");
    lstrcatW(s_LinkTargetWithBackslash, L"2PRONG.txt");
    lstrcatW(s_LinkTargetWithBackslash, L"\\");

    /* Build s_LinkTargetDoubleBackslash path */
    lstrcpyW(s_LinkTargetDoubleBackslash, s_TestDir);
    lstrcatW(s_LinkTargetDoubleBackslash, L"\\\\");
    lstrcatW(s_LinkTargetDoubleBackslash, L"2PRONG.txt");

    /* Build s_ShortcutLongName path */
    lstrcpyW(s_ShortcutLongName, s_TestDir);
    lstrcatW(s_ShortcutLongName, L"\\");
    lstrcatW(s_ShortcutLongName, L"CmdLineUtils.lnk"); /* in Long File Name */

    /* Create s_ShortcutLongName shortcut file */
    ok(CreateShortcut(s_ShortcutLongName, s_LinkTarget),
       "CreateShortcut(%s, %s) failed.\n",
       wine_dbgstr_w(s_ShortcutLongName), wine_dbgstr_w(s_LinkTarget));

    /* Load shell32.dll */
    s_hShell32 = LoadLibraryA("shell32");
    if (s_hShell32 == NULL)
    {
        skip("Unable to load shell32.\n");
        goto Cleanup;
    }

    /* Get PathResolve procedure */
    s_pPathResolve = (PATHRESOLVE)GetProcAddress(s_hShell32, "PathResolve");
    if (s_pPathResolve == NULL)
    {
        skip("Unable to get PathResolve address.\n");
        goto Cleanup;
    }

    /* Get IsLFNDriveW procedure */
    s_pIsLFNDriveW = (ISLFNDRIVEW)GetProcAddress(s_hShell32, (LPCSTR)(INT_PTR)42);
    if (s_pIsLFNDriveW == NULL)
    {
        skip("Unable to get IsLFNDriveW address.\n");
        goto Cleanup;
    }

    /* Is LFN supported? */
    lstrcpyW(szRoot, s_TestDir);
    PathStripToRootW(szRoot);
    if (!s_pIsLFNDriveW(szRoot))
    {
        skip("LFN is not supported in this drive %s.\n", wine_dbgstr_w(szRoot));
        goto Cleanup;
    }

    /* Do tests */
    TestMain_PathResolve();

    /* Clean up */
Cleanup:
    DeleteFileW(s_LinkTarget);
    DeleteFileW(s_ShortcutLongName);
    RemoveDirectoryW(s_TestDir);
    FreeLibrary(s_hShell32);
}
