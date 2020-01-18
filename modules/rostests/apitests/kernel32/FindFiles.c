/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for the Find*File*A/W APIs.
 * PROGRAMMER:      Hermès BÉLUSCA - MAÏTO
 */

#include "precomp.h"
#include <versionhelpers.h>

/*
 * NOTE: This test supposes the following requirements:
 * - There is a disk attached to the letter "C:"
 * - There is a Windows-like system installed in "C:\<installation_directory>"
 *   which contains a sub-directory "system32"
 * - There is no sub-directory called "foobar" in "C:\".
 *
 * If one of these requirements is not fulfilled, one or more tests may fail.
 */

static CHAR     OSDirA[MAX_PATH];       // OS directory
static WCHAR    OSDirW[MAX_PATH];
static CHAR     baseA[MAX_PATH];        // Current directory
static WCHAR    baseW[MAX_PATH];
static CHAR     selfnameA[MAX_PATH];    // Path to this executable
static WCHAR    selfnameW[MAX_PATH];
static LPSTR    exenameA;               // Executable's name
static LPWSTR   exenameW;
static INT      myARGC;
static LPSTR*   myARGV;


/*
 * Fixes definition of Wine's ok_err
 */
#ifdef ok_err
#undef ok_err
#endif

#define ok_err(error) \
    ok_int(GetLastError(), error)


/*
 * Types of tests. Define them as macros so that calling them
 * into the code reports the actual line where they were called.
 */
#define testType1_A(lpFileName, dwInitialError, hExpectedHandleValue, dwExpectedError, bExpectedNullFilename)       \
do {    \
    ZeroMemory(&fd, sizeof(fd));            \
    SetLastError((dwInitialError));         \
    h = FindFirstFileA((lpFileName), &fd);  \
    ok(h == (hExpectedHandleValue), "FindFirstFileA returned 0x%p, expected 0x%p\n", h, (hExpectedHandleValue));    \
    ok_err(dwExpectedError);                \
    if (bExpectedNullFilename)              \
        ok(fd.cFileName[0] == 0, "fd.cFileName != \"\"\n"); \
    else                                    \
        ok(fd.cFileName[0] != 0, "fd.cFileName == \"\"\n"); \
    FindClose(h);                           \
} while (0)

#define testType1_W(lpFileName, dwInitialError, hExpectedHandleValue, dwExpectedError, bExpectedNullFilename)       \
do {    \
    ZeroMemory(&fd, sizeof(fd));            \
    SetLastError((dwInitialError));         \
    h = FindFirstFileW((lpFileName), &fd);  \
    ok(h == (hExpectedHandleValue), "FindFirstFileW returned 0x%p, expected 0x%p\n", h, (hExpectedHandleValue));    \
    ok_err(dwExpectedError);                \
    if (bExpectedNullFilename)              \
        ok(fd.cFileName[0] == 0, "fd.cFileName != \"\"\n"); \
    else                                    \
        ok(fd.cFileName[0] != 0, "fd.cFileName == \"\"\n"); \
    FindClose(h);                           \
} while (0)

#define testType2_A(lpFileName, dwInitialError, hUnexpectedHandleValue, dwExpectedError)    \
do {    \
    ZeroMemory(&fd, sizeof(fd));            \
    SetLastError((dwInitialError));         \
    h = FindFirstFileA((lpFileName), &fd);  \
    ok(h != (hUnexpectedHandleValue), "FindFirstFileA returned 0x%p\n", h); \
    ok_err(dwExpectedError);                \
    ok(fd.cFileName[0] != 0, "fd.cFileName == \"\"\n"); \
    FindClose(h);                           \
} while (0)

#define testType2_W(lpFileName, dwInitialError, hUnexpectedHandleValue, dwExpectedError)    \
do {    \
    ZeroMemory(&fd, sizeof(fd));            \
    SetLastError((dwInitialError));         \
    h = FindFirstFileW((lpFileName), &fd);  \
    ok(h != (hUnexpectedHandleValue), "FindFirstFileW returned 0x%p\n", h); \
    ok_err(dwExpectedError);                \
    ok(fd.cFileName[0] != 0, "fd.cFileName == \"\"\n"); \
    FindClose(h);                           \
} while (0)


static void Test_FindFirstFileA(void)
{
    CHAR CurrentDirectory[MAX_PATH];
    CHAR Buffer[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE h;

    /* Save the current directory */
    GetCurrentDirectoryA(sizeof(CurrentDirectory) / sizeof(CHAR), CurrentDirectory);

/*** Tests for the root directory - root directory ***/
    /* Modify the current directory */
    SetCurrentDirectoryA("C:\\");

    testType1_A("C:", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);

    testType1_A("C:\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_A("C:\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType2_A("C:\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    testType1_A("\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_A("\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, IsWindows7OrGreater() ? ERROR_INVALID_NAME : ERROR_BAD_NETPATH, TRUE);
    testType2_A("\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    /* Restore the old current directory */
    SetCurrentDirectoryA(CurrentDirectory);
/*****************************************************/

/*** Tests for the root directory - long directory ***/
    /* Modify the current directory */
    SetCurrentDirectoryA(OSDirA); /* We expect here that OSDir is of the form: C:\OSDir */

    testType2_A("C:", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    testType1_A("C:\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_A("C:\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType2_A("C:\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    testType1_A("\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_A("\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, IsWindows7OrGreater() ? ERROR_INVALID_NAME : ERROR_BAD_NETPATH, TRUE);
    testType2_A("\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    /* Restore the old current directory */
    SetCurrentDirectoryA(CurrentDirectory);
/*****************************************************/

/*** Relative paths ***/
    /*
     * NOTE: This test does not give the same results if you launch the app
     * from a root drive or from a long-form directory (of the form C:\dir).
     */
    // testType2_A("..", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);
    // testType1_A("..", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
/**********************/

/*** Relative paths - root directory ***/
    /* Modify the current directory */
    SetCurrentDirectoryA("C:\\");

    testType1_A(".", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_A(".\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_A(".\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType2_A(".\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    testType1_A("..", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_A("..\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_A("..\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType2_A("..\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    /* Restore the old current directory */
    SetCurrentDirectoryA(CurrentDirectory);
/***************************************/

/*** Relative paths - long directory ***/
    /* Modify the current directory */
    SetCurrentDirectoryA(OSDirA); /* We expect here that OSDir is of the form: C:\OSDir */

    testType2_A(".", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    testType1_A(".\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_A(".\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType2_A(".\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    testType1_A("..", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);

    testType1_A("..\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_A("..\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType2_A("..\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    /* Restore the old current directory */
    SetCurrentDirectoryA(CurrentDirectory);
/****************************************/

/*** Unexisting path ***/
    testType1_A("C:\\foobar", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_A("C:\\foobar\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_A("C:\\foobar\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_A("C:\\foobar\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_A("C:\\foobar\\system32\\..\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    /* Possibly a DOS device */
    testType1_A("C:\\foobar\\nul", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_A("C:\\foobar\\nul\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_A("C:\\foobar\\nul\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_A("C:\\foobar\\nul\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    testType1_A("C:\\foobar\\toto", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_A("C:\\foobar\\toto\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_A("C:\\foobar\\toto\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_A("C:\\foobar\\toto\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    strcpy(Buffer, "C:\\foobar\\");
    strcat(Buffer, exenameA);
    testType1_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    strcpy(Buffer, "C:\\foobar\\.\\");
    strcat(Buffer, exenameA);
    testType1_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
/***********************/

/*** Existing path ***/
    strcpy(Buffer, OSDirA);
    testType2_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    strcpy(Buffer, OSDirA);
    strcat(Buffer, "\\");
    testType1_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);

    strcpy(Buffer, OSDirA);
    strcat(Buffer, "\\\\");
    testType1_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);

    strcpy(Buffer, OSDirA);
    strcat(Buffer, "\\*");
    testType2_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    strcpy(Buffer, OSDirA);
    strcat(Buffer, "\\.\\*");
    testType2_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    strcpy(Buffer, OSDirA);
    strcat(Buffer, "\\system32\\..\\*");
    testType2_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    /* Possibly a DOS device */
    strcpy(Buffer, OSDirA);
    strcat(Buffer, "\\nul");
    testType1_A(Buffer, 0xdeadbeef, (HANDLE)0x00000001, 0xdeadbeef, FALSE);

    strcpy(Buffer, OSDirA);
    strcat(Buffer, "\\nul\\");
    testType1_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    strcpy(Buffer, OSDirA);
    strcat(Buffer, "\\nul\\\\");
    testType1_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    strcpy(Buffer, OSDirA);
    strcat(Buffer, "\\nul\\*");
    testType1_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    strcpy(Buffer, OSDirA);
    strcat(Buffer, "\\toto");
    testType1_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);

    strcpy(Buffer, OSDirA);
    strcat(Buffer, "\\toto\\");
    testType1_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    strcpy(Buffer, OSDirA);
    strcat(Buffer, "\\toto\\\\");
    testType1_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    strcpy(Buffer, OSDirA);
    strcat(Buffer, "\\toto\\*");
    testType1_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    // strcpy(Buffer, baseA);
    // strcat(Buffer, "\\");
    // strcat(Buffer, exenameA);
    // testType2_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    // strcpy(Buffer, baseA);
    // strcat(Buffer, "\\.\\");
    // strcat(Buffer, exenameA);
    // testType2_A(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);
/*********************/

    return;
}

static void Test_FindFirstFileW(void)
{
    WCHAR CurrentDirectory[MAX_PATH];
    WCHAR Buffer[MAX_PATH];
    WIN32_FIND_DATAW fd;
    HANDLE h;

    /* Save the current directory */
    GetCurrentDirectoryW(sizeof(CurrentDirectory) / sizeof(WCHAR), CurrentDirectory);

/*** Tests for the root directory - root directory ***/
    /* Modify the current directory */
    SetCurrentDirectoryW(L"C:\\");

    testType1_W(L"C:", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);

    testType1_W(L"C:\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_W(L"C:\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType2_W(L"C:\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    testType1_W(L"\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_W(L"\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, IsWindows7OrGreater() ? ERROR_INVALID_NAME : ERROR_BAD_NETPATH, TRUE);
    testType2_W(L"\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    /* Restore the old current directory */
    SetCurrentDirectoryW(CurrentDirectory);
/*****************************************************/

/*** Tests for the root directory - long directory ***/
    /* Modify the current directory */
    SetCurrentDirectoryW(OSDirW); /* We expect here that OSDir is of the form: C:\OSDir */

    testType2_W(L"C:", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    testType1_W(L"C:\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_W(L"C:\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType2_W(L"C:\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    testType1_W(L"\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_W(L"\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, IsWindows7OrGreater() ? ERROR_INVALID_NAME : ERROR_BAD_NETPATH, TRUE);
    testType2_W(L"\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    /* Restore the old current directory */
    SetCurrentDirectoryW(CurrentDirectory);
/*****************************************************/

/*** Relative paths ***/
    /*
     * NOTE: This test does not give the same results if you launch the app
     * from a root drive or from a long-form directory (of the form C:\dir).
     */
    // testType2_W(L"..", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);
    // testType1_W(L"..", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
/**********************/

/*** Relative paths - root directory ***/
    /* Modify the current directory */
    SetCurrentDirectoryW(L"C:\\");

    testType1_W(L".", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_W(L".\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_W(L".\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType2_W(L".\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    testType1_W(L"..", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_W(L"..\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_W(L"..\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType2_W(L"..\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    /* Restore the old current directory */
    SetCurrentDirectoryW(CurrentDirectory);
/***************************************/

/*** Relative paths - long directory ***/
    /* Modify the current directory */
    SetCurrentDirectoryW(OSDirW); /* We expect here that OSDir is of the form: C:\OSDir */

    testType2_W(L".", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    testType1_W(L".\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_W(L".\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType2_W(L".\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    testType1_W(L"..", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);

    testType1_W(L"..\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_W(L"..\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType2_W(L"..\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    /* Restore the old current directory */
    SetCurrentDirectoryW(CurrentDirectory);
/****************************************/

/*** Unexisting path ***/
    testType1_W(L"C:\\foobar", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);
    testType1_W(L"C:\\foobar\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_W(L"C:\\foobar\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_W(L"C:\\foobar\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_W(L"C:\\foobar\\system32\\..\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    /* Possibly a DOS device */
    testType1_W(L"C:\\foobar\\nul", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_W(L"C:\\foobar\\nul\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_W(L"C:\\foobar\\nul\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_W(L"C:\\foobar\\nul\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    testType1_W(L"C:\\foobar\\toto", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_W(L"C:\\foobar\\toto\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_W(L"C:\\foobar\\toto\\\\", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
    testType1_W(L"C:\\foobar\\toto\\*", 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    wcscpy(Buffer, L"C:\\foobar\\");
    wcscat(Buffer, exenameW);
    testType1_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    wcscpy(Buffer, L"C:\\foobar\\.\\");
    wcscat(Buffer, exenameW);
    testType1_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);
/***********************/

/*** Existing path ***/
    wcscpy(Buffer, OSDirW);
    testType2_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    wcscpy(Buffer, OSDirW);
    wcscat(Buffer, L"\\");
    testType1_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);

    wcscpy(Buffer, OSDirW);
    wcscat(Buffer, L"\\\\");
    testType1_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);

    wcscpy(Buffer, OSDirW);
    wcscat(Buffer, L"\\*");
    testType2_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    wcscpy(Buffer, OSDirW);
    wcscat(Buffer, L"\\.\\*");
    testType2_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    wcscpy(Buffer, OSDirW);
    wcscat(Buffer, L"\\system32\\..\\*");
    testType2_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    /* Possibly a DOS device */
    wcscpy(Buffer, OSDirW);
    wcscat(Buffer, L"\\nul");
    testType1_W(Buffer, 0xdeadbeef, (HANDLE)0x00000001, 0xdeadbeef, FALSE);

    wcscpy(Buffer, OSDirW);
    wcscat(Buffer, L"\\nul\\");
    testType1_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    wcscpy(Buffer, OSDirW);
    wcscat(Buffer, L"\\nul\\\\");
    testType1_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    wcscpy(Buffer, OSDirW);
    wcscat(Buffer, L"\\nul\\*");
    testType1_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    wcscpy(Buffer, OSDirW);
    wcscat(Buffer, L"\\toto");
    testType1_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_FILE_NOT_FOUND, TRUE);

    wcscpy(Buffer, OSDirW);
    wcscat(Buffer, L"\\toto\\");
    testType1_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    wcscpy(Buffer, OSDirW);
    wcscat(Buffer, L"\\toto\\\\");
    testType1_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    wcscpy(Buffer, OSDirW);
    wcscat(Buffer, L"\\toto\\*");
    testType1_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, ERROR_PATH_NOT_FOUND, TRUE);

    // wcscpy(Buffer, baseW);
    // wcscat(Buffer, L"\\");
    // wcscat(Buffer, exenameW);
    // testType2_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);

    // wcscpy(Buffer, baseW);
    // wcscat(Buffer, L"\\.\\");
    // wcscat(Buffer, exenameW);
    // testType2_W(Buffer, 0xdeadbeef, INVALID_HANDLE_VALUE, 0xdeadbeef);
/*********************/

    return;
}


static int init(void)
{
    LPSTR p;
    size_t i;

    myARGC = winetest_get_mainargs(&myARGV);
    if (!GetCurrentDirectoryA(sizeof(baseA)/sizeof(baseA[0]), baseA)) return 0;
    strcpy(selfnameA, myARGV[0]);

    /* Strip the path of selfname */
    if ((p = strrchr(selfnameA, '\\')) != NULL)
        exenameA = p + 1;
    else
        exenameA = selfnameA;

    if ((p = strrchr(exenameA, '/')) != NULL)
        exenameA = p + 1;

    if (!GetWindowsDirectoryA(OSDirA, sizeof(OSDirA)/sizeof(OSDirA[0]))) return 0;

    /* Quick-and-dirty conversion ANSI --> UNICODE without the Win32 APIs */
    for (i = 0 ; i <= strlen(baseA) ; ++i)
    {
        baseW[i] = (WCHAR)baseA[i];
    }
    for (i = 0 ; i <= strlen(selfnameA) ; ++i)
    {
        selfnameW[i] = (WCHAR)selfnameA[i];
    }
    exenameW = selfnameW + (exenameA - selfnameA);
    for (i = 0 ; i <= strlen(OSDirA) ; ++i)
    {
        OSDirW[i] = (WCHAR)OSDirA[i];
    }

    return 1;
}

START_TEST(FindFiles)
{
    int b  = init();
    ok(b, "Basic init of FindFiles test\n");
    if (!b) return;

    Test_FindFirstFileA();
    Test_FindFirstFileW();
}
