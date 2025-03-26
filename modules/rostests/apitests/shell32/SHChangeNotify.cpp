/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for SHChangeNotify
 * COPYRIGHT:   Copyright 2020-2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

// NOTE: This testcase requires shell32_apitest_sub.exe.

#include "shelltest.h"
#include "shell32_apitest_sub.h"
#include <assert.h>

#define NUM_STEP        8
#define NUM_CHECKS      12
#define INTERVAL        0
#define MAX_EVENT_TYPE  6

static HWND s_hMainWnd = NULL, s_hSubWnd = NULL;
static WCHAR s_szSubProgram[MAX_PATH]; // shell32_apitest_sub.exe
static HANDLE s_hThread = NULL;
static INT s_iStage = -1, s_iStep = -1;
static BYTE s_abChecks[NUM_CHECKS] = { 0 }; // Flags for testing
static BOOL s_bGotUpdateDir = FALSE; // Got SHCNE_UPDATEDIR?

static BOOL DoCreateFile(LPCWSTR pszFileName)
{
    HANDLE hFile = ::CreateFileW(pszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ::CloseHandle(hFile);
    return hFile != INVALID_HANDLE_VALUE;
}

static void DoDeleteDirectory(LPCWSTR pszDir)
{
    WCHAR szPath[MAX_PATH];
    ZeroMemory(szPath, sizeof(szPath));
    StringCchCopyW(szPath, _countof(szPath), pszDir); // Double-NULL terminated
    SHFILEOPSTRUCTW FileOp = { NULL, FO_DELETE, szPath, NULL, FOF_NOCONFIRMATION | FOF_SILENT };
    SHFileOperation(&FileOp);
}

static INT GetEventType(LONG lEvent)
{
    switch (lEvent)
    {
        case SHCNE_CREATE:          return 0;
        case SHCNE_DELETE:          return 1;
        case SHCNE_RENAMEITEM:      return 2;
        case SHCNE_MKDIR:           return 3;
        case SHCNE_RMDIR:           return 4;
        case SHCNE_RENAMEFOLDER:    return 5;
        C_ASSERT(5 + 1 == MAX_EVENT_TYPE);
        default:                    return -1;
    }
}

#define FILE_1  L"_TESTFILE_1_.txt"
#define FILE_2  L"_TESTFILE_2_.txt"
#define DIR_1   L"_TESTDIR_1_"
#define DIR_2   L"_TESTDIR_2_"

static WCHAR s_szDir1[MAX_PATH];
static WCHAR s_szDir1InDir1[MAX_PATH];
static WCHAR s_szDir2InDir1[MAX_PATH];
static WCHAR s_szFile1InDir1InDir1[MAX_PATH];
static WCHAR s_szFile1InDir1[MAX_PATH];
static WCHAR s_szFile2InDir1[MAX_PATH];

static void DoDeleteFilesAndDirs(void)
{
    ::DeleteFileW(s_szFile1InDir1);
    ::DeleteFileW(s_szFile2InDir1);
    ::DeleteFileW(s_szFile1InDir1InDir1);
    DoDeleteDirectory(s_szDir1InDir1);
    DoDeleteDirectory(s_szDir2InDir1);
    DoDeleteDirectory(s_szDir1);
}

static void TEST_Quit(void)
{
    CloseHandle(s_hThread);
    s_hThread = NULL;

    PostMessageW(s_hSubWnd, WM_COMMAND, IDNO, 0); // Finish
    DoWaitForWindow(SUB_CLASSNAME, SUB_CLASSNAME, TRUE, TRUE); // Close sub-windows

    DoDeleteFilesAndDirs();
}

static void DoBuildFilesAndDirs(void)
{
    WCHAR szPath1[MAX_PATH];
    SHGetSpecialFolderPathW(NULL, szPath1, CSIDL_PERSONAL, FALSE); // My Documents
    PathAppendW(szPath1, DIR_1);
    StringCchCopyW(s_szDir1, _countof(s_szDir1), szPath1);

    PathAppendW(szPath1, DIR_1);
    StringCchCopyW(s_szDir1InDir1, _countof(s_szDir1InDir1), szPath1);
    PathRemoveFileSpecW(szPath1);

    PathAppendW(szPath1, DIR_2);
    StringCchCopyW(s_szDir2InDir1, _countof(s_szDir2InDir1), szPath1);
    PathRemoveFileSpecW(szPath1);

    PathAppendW(szPath1, DIR_1);
    PathAppendW(szPath1, FILE_1);
    StringCchCopyW(s_szFile1InDir1InDir1, _countof(s_szFile1InDir1InDir1), szPath1);
    PathRemoveFileSpecW(szPath1);
    PathRemoveFileSpecW(szPath1);

    PathAppendW(szPath1, FILE_1);
    StringCchCopyW(s_szFile1InDir1, _countof(s_szFile1InDir1), szPath1);
    PathRemoveFileSpecW(szPath1);

    PathAppendW(szPath1, FILE_2);
    StringCchCopyW(s_szFile2InDir1, _countof(s_szFile2InDir1), szPath1);
    PathRemoveFileSpecW(szPath1);

#define TRACE_PATH(path) trace(#path ": %ls\n", path)
    TRACE_PATH(s_szDir1);
    TRACE_PATH(s_szDir1InDir1);
    TRACE_PATH(s_szFile1InDir1);
    TRACE_PATH(s_szFile1InDir1InDir1);
    TRACE_PATH(s_szFile2InDir1);
#undef TRACE_PATH

    DoDeleteFilesAndDirs();

    ::CreateDirectoryW(s_szDir1, NULL);
    ok_int(!!PathIsDirectoryW(s_szDir1), TRUE);

    DoDeleteDirectory(s_szDir1InDir1);
    ok_int(!PathIsDirectoryW(s_szDir1InDir1), TRUE);
}

static void DoTestEntry(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH];

    szPath1[0] = szPath2[0] = 0;
    SHGetPathFromIDListW(pidl1, szPath1);
    SHGetPathFromIDListW(pidl2, szPath2);

    trace("(0x%lX, '%ls', '%ls')\n", lEvent, szPath1, szPath2);

    if (lEvent == SHCNE_UPDATEDIR)
    {
        trace("Got SHCNE_UPDATEDIR\n");
        s_bGotUpdateDir = TRUE;
        return;
    }

    INT iEventType = GetEventType(lEvent);
    if (iEventType < 0)
        return;

    assert(iEventType < MAX_EVENT_TYPE);

    INT i = 0;
    s_abChecks[i++] |= (lstrcmpiW(szPath1, L"") == 0) << iEventType;
    s_abChecks[i++] |= (lstrcmpiW(szPath1, s_szDir1) == 0) << iEventType;
    s_abChecks[i++] |= (lstrcmpiW(szPath1, s_szDir2InDir1) == 0) << iEventType;
    s_abChecks[i++] |= (lstrcmpiW(szPath1, s_szFile1InDir1) == 0) << iEventType;
    s_abChecks[i++] |= (lstrcmpiW(szPath1, s_szFile2InDir1) == 0) << iEventType;
    s_abChecks[i++] |= (lstrcmpiW(szPath1, s_szFile1InDir1InDir1) == 0) << iEventType;
    s_abChecks[i++] |= (lstrcmpiW(szPath2, L"") == 0) << iEventType;
    s_abChecks[i++] |= (lstrcmpiW(szPath2, s_szDir1) == 0) << iEventType;
    s_abChecks[i++] |= (lstrcmpiW(szPath2, s_szDir2InDir1) == 0) << iEventType;
    s_abChecks[i++] |= (lstrcmpiW(szPath2, s_szFile1InDir1) == 0) << iEventType;
    s_abChecks[i++] |= (lstrcmpiW(szPath2, s_szFile2InDir1) == 0) << iEventType;
    s_abChecks[i++] |= (lstrcmpiW(szPath2, s_szFile1InDir1InDir1) == 0) << iEventType;
    assert(i == NUM_CHECKS);
}

static LPCSTR StringFromChecks(void)
{
    static char s_sz[2 * NUM_CHECKS + 1];

    char *pch = s_sz;
    for (INT i = 0; i < NUM_CHECKS; ++i)
    {
        WCHAR sz[3];
        StringCchPrintfW(sz, _countof(sz), L"%02X", s_abChecks[i]);
        *pch++ = sz[0];
        *pch++ = sz[1];
    }

    assert((pch - s_sz) + 1 == sizeof(s_sz));

    *pch = 0;
    return s_sz;
}

struct TEST_ANSWER
{
    INT lineno;
    LPCSTR answer;
};

static void DoStepCheck(INT iStage, INT iStep, LPCSTR checks)
{
    assert(0 <= iStep);
    assert(iStep < NUM_STEP);

    assert(0 <= iStage);
    assert(iStage < NUM_STAGE);

    if (s_bGotUpdateDir)
    {
        ok_int(TRUE, TRUE);
        return;
    }

    LPCSTR answer = NULL;
    INT lineno;
    switch (iStage)
    {
        case 0:
        case 1:
        case 3:
        case 6:
        case 9:
        {
            static const TEST_ANSWER c_answers[] =
            {
                { __LINE__, "000000010000010000000000" }, // 0
                { __LINE__, "000000040000000000000400" }, // 1
                { __LINE__, "000000000200020000000000" }, // 2
                { __LINE__, "000000000000080000000000" }, // 3
                { __LINE__, "000000000001010000000000" }, // 4
                { __LINE__, "000000000002020000000000" }, // 5
                { __LINE__, "000000000000000020000000" }, // 6
                { __LINE__, "000010000000100000000000" }, // 7
            };
            C_ASSERT(_countof(c_answers) == NUM_STEP);
            lineno = c_answers[iStep].lineno;
            answer = c_answers[iStep].answer;
            break;
        }
        case 2:
        case 4:
        case 5:
        case 7:
        {
            static const TEST_ANSWER c_answers[] =
            {
                { __LINE__, "000000000000000000000000" }, // 0
                { __LINE__, "000000000000000000000000" }, // 1
                { __LINE__, "000000000000000000000000" }, // 2
                { __LINE__, "000000000000000000000000" }, // 3
                { __LINE__, "000000000000000000000000" }, // 4
                { __LINE__, "000000000000000000000000" }, // 5
                { __LINE__, "000000000000000000000000" }, // 6
                { __LINE__, "000000000000000000000000" }, // 7
            };
            C_ASSERT(_countof(c_answers) == NUM_STEP);
            lineno = c_answers[iStep].lineno;
            answer = c_answers[iStep].answer;
            break;
        }
        case 8:
        {
            static const TEST_ANSWER c_answers[] =
            {
                { __LINE__, "000000010000010000000000" }, // 0
                { __LINE__, "000000040000000000000400" }, // 1
                { __LINE__, "000000000200020000000000" }, // 2
                { __LINE__, "000000000000080000000000" }, // 3
                { __LINE__, "000000000001010000000000" }, // 4 // Recursive case
                { __LINE__, "000000000002020000000000" }, // 5 // Recursive case
                { __LINE__, "000000000000000020000000" }, // 6
                { __LINE__, "000010000000100000000000" }, // 7
            };
            C_ASSERT(_countof(c_answers) == NUM_STEP);
            lineno = c_answers[iStep].lineno;
            answer = c_answers[iStep].answer;
            if (iStep == 4 || iStep == 5) // Recursive cases
            {
                if (lstrcmpA(checks, "000000000000000000000000") == 0)
                {
                    trace("Warning! Recursive cases...\n");
                    answer = "000000000000000000000000";
                }
            }
            break;
        }
        default:
        {
            assert(0);
            break;
        }
    }

    ok(lstrcmpA(checks, answer) == 0,
       "Line %d: '%s' vs '%s' at Stage %d, Step %d\n", lineno, checks, answer, iStage, iStep);
}

static DWORD WINAPI StageThreadFunc(LPVOID arg)
{
    BOOL ret;

    trace("Stage %d\n", s_iStage);

    // 0: Create file1 in dir1
    s_iStep = 0;
    trace("Step %d\n", s_iStep);
    SHChangeNotify(0, SHCNF_PATHW | SHCNF_FLUSH, NULL, NULL);
    ZeroMemory(s_abChecks, sizeof(s_abChecks));
    ret = DoCreateFile(s_szFile1InDir1);
    ok_int(ret, TRUE);
    SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW | SHCNF_FLUSH, s_szFile1InDir1, 0);
    ::Sleep(INTERVAL);
    DoStepCheck(s_iStage, s_iStep, StringFromChecks());

    // 1: Rename file1 as file2 in dir1
    ++s_iStep;
    trace("Step %d\n", s_iStep);
    ZeroMemory(s_abChecks, sizeof(s_abChecks));
    ret = MoveFileW(s_szFile1InDir1, s_szFile2InDir1);
    ok_int(ret, TRUE);
    SHChangeNotify(SHCNE_RENAMEITEM, SHCNF_PATHW | SHCNF_FLUSH, s_szFile1InDir1, s_szFile2InDir1);
    ::Sleep(INTERVAL);
    DoStepCheck(s_iStage, s_iStep, StringFromChecks());

    // 2: Delete file2 in dir1
    ++s_iStep;
    trace("Step %d\n", s_iStep);
    ZeroMemory(s_abChecks, sizeof(s_abChecks));
    ret = DeleteFileW(s_szFile2InDir1);
    ok_int(ret, TRUE);
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW | SHCNF_FLUSH, s_szFile2InDir1, NULL);
    ::Sleep(INTERVAL);
    DoStepCheck(s_iStage, s_iStep, StringFromChecks());

    // 3: Create dir1 in dir1
    ++s_iStep;
    trace("Step %d\n", s_iStep);
    ZeroMemory(s_abChecks, sizeof(s_abChecks));
    ret = CreateDirectoryExW(s_szDir1, s_szDir1InDir1, NULL);
    ok_int(ret, TRUE);
    SHChangeNotify(SHCNE_MKDIR, SHCNF_PATHW | SHCNF_FLUSH, s_szDir1InDir1, NULL);
    ::Sleep(INTERVAL);
    DoStepCheck(s_iStage, s_iStep, StringFromChecks());

    // 4: Create file1 in dir1 in dir1
    ++s_iStep;
    trace("Step %d\n", s_iStep);
    ZeroMemory(s_abChecks, sizeof(s_abChecks));
    ret = DoCreateFile(s_szFile1InDir1InDir1);
    ok_int(ret, TRUE);
    SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW | SHCNF_FLUSH, s_szFile1InDir1InDir1, NULL);
    ::Sleep(INTERVAL);
    DoStepCheck(s_iStage, s_iStep, StringFromChecks());

    // 5: Delete file1 in dir1 in dir1
    ++s_iStep;
    trace("Step %d\n", s_iStep);
    ZeroMemory(s_abChecks, sizeof(s_abChecks));
    ret = DeleteFileW(s_szFile1InDir1InDir1);
    ok_int(ret, TRUE);
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW | SHCNF_FLUSH, s_szFile1InDir1InDir1, NULL);
    ::Sleep(INTERVAL);
    DoStepCheck(s_iStage, s_iStep, StringFromChecks());

    // 6: Rename dir1 as dir2 in dir1
    ++s_iStep;
    trace("Step %d\n", s_iStep);
    ZeroMemory(s_abChecks, sizeof(s_abChecks));
    ret = ::MoveFileW(s_szDir1InDir1, s_szDir2InDir1);
    ok_int(ret, TRUE);
    SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_PATHW | SHCNF_FLUSH, s_szDir1InDir1, s_szDir2InDir1);
    ::Sleep(INTERVAL);
    DoStepCheck(s_iStage, s_iStep, StringFromChecks());

    // 7: Remove dir2 in dir1
    ++s_iStep;
    trace("Step %d\n", s_iStep);
    ZeroMemory(s_abChecks, sizeof(s_abChecks));
    ret = RemoveDirectoryW(s_szDir2InDir1);
    ok_int(ret, TRUE);
    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATHW | SHCNF_FLUSH, s_szDir2InDir1, NULL);
    ::Sleep(INTERVAL);
    DoStepCheck(s_iStage, s_iStep, StringFromChecks());

    // 8: Finish
    ++s_iStep;
    assert(s_iStep == NUM_STEP);
    C_ASSERT(NUM_STEP == 8);
    if (s_iStage + 1 < NUM_STAGE)
    {
        ::PostMessage(s_hSubWnd, WM_COMMAND, IDRETRY, 0); // Next stage
    }
    else
    {
        // Finish
        ::PostMessage(s_hSubWnd, WM_COMMAND, IDNO, 0);
        ::PostMessage(s_hMainWnd, WM_COMMAND, IDNO, 0);
    }

    s_iStep = -1;

    return 0;
}

// WM_COPYDATA
static BOOL OnCopyData(HWND hwnd, HWND hwndSender, COPYDATASTRUCT *pCopyData)
{
    if (pCopyData->dwData != 0xBEEFCAFE)
        return FALSE;

    LPBYTE pbData = (LPBYTE)pCopyData->lpData;
    LPBYTE pb = pbData;

    LONG cbTotal = pCopyData->cbData;
    assert(cbTotal >= LONG(sizeof(LONG) + sizeof(DWORD) + sizeof(DWORD)));

    LONG lEvent = *(LONG*)pb;
    pb += sizeof(lEvent);

    DWORD cbPidl1 = *(DWORD*)pb;
    pb += sizeof(cbPidl1);

    DWORD cbPidl2 = *(DWORD*)pb;
    pb += sizeof(cbPidl2);

    LPITEMIDLIST pidl1 = NULL;
    if (cbPidl1)
    {
        pidl1 = (LPITEMIDLIST)CoTaskMemAlloc(cbPidl1);
        CopyMemory(pidl1, pb, cbPidl1);
        pb += cbPidl1;
    }

    LPITEMIDLIST pidl2 = NULL;
    if (cbPidl2)
    {
        pidl2 = (LPITEMIDLIST)CoTaskMemAlloc(cbPidl2);
        CopyMemory(pidl2, pb, cbPidl2);
        pb += cbPidl2;
    }

    assert((pb - pbData) == cbTotal);

    DoTestEntry(lEvent, pidl1, pidl2);

    CoTaskMemFree(pidl1);
    CoTaskMemFree(pidl2);

    return TRUE;
}

static LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            s_hMainWnd = hwnd;
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDYES: // Start testing
                {
                    s_iStage = 0;
                    s_bGotUpdateDir = FALSE;
                    s_hThread = ::CreateThread(NULL, 0, StageThreadFunc, hwnd, 0, NULL);
                    if (!s_hThread)
                    {
                        skip("!s_hThread\n");
                        DestroyWindow(hwnd);
                    }
                    break;
                }
                case IDRETRY: // New stage
                {
                    ::CloseHandle(s_hThread);
                    ++s_iStage;
                    s_bGotUpdateDir = FALSE;
                    s_hThread = ::CreateThread(NULL, 0, StageThreadFunc, hwnd, 0, NULL);
                    if (!s_hThread)
                    {
                        skip("!s_hThread\n");
                        DestroyWindow(hwnd);
                    }
                    break;
                }
                case IDNO: // Quit
                {
                    s_iStage = -1;
                    DestroyWindow(hwnd);
                    break;
                }
            }
            break;

        case WM_COPYDATA:
            if (s_iStage < 0 || s_iStep < 0)
                break;

            OnCopyData(hwnd, (HWND)wParam, (COPYDATASTRUCT*)lParam);
            break;

        case WM_DESTROY:
            ::PostQuitMessage(0);
            break;

        default:
            return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

static BOOL TEST_Init(void)
{
    if (!FindSubProgram(s_szSubProgram, _countof(s_szSubProgram)))
    {
        skip("shell32_apitest_sub.exe not found\n");
        return FALSE;
    }

    // close the SUB_CLASSNAME windows
    DoWaitForWindow(SUB_CLASSNAME, SUB_CLASSNAME, TRUE, TRUE);

    // Execute sub program
    HINSTANCE hinst = ShellExecuteW(NULL, NULL, s_szSubProgram, L"---", NULL, SW_SHOWNORMAL);
    if ((INT_PTR)hinst <= 32)
    {
        skip("Unable to run shell32_apitest_sub.exe.\n");
        return FALSE;
    }

    // prepare for files and dirs
    DoBuildFilesAndDirs();

    // Register main window
    WNDCLASSW wc = { 0, MainWndProc };
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = MAIN_CLASSNAME;
    if (!RegisterClassW(&wc))
    {
        skip("RegisterClassW failed\n");
        return FALSE;
    }

    // Create main window
    HWND hwnd = CreateWindowW(MAIN_CLASSNAME, MAIN_CLASSNAME, WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 400, 100,
                              NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!hwnd)
    {
        skip("CreateWindowW failed\n");
        return FALSE;
    }
    ::ShowWindow(hwnd, SW_SHOWNORMAL);
    ::UpdateWindow(hwnd);

    // Find sub-window
    s_hSubWnd = DoWaitForWindow(SUB_CLASSNAME, SUB_CLASSNAME, FALSE, FALSE);
    if (!s_hSubWnd)
    {
        skip("Unable to find sub-program window.\n");
        return FALSE;
    }

    // Start testing
    SendMessageW(s_hSubWnd, WM_COMMAND, IDYES, 0);

    return TRUE;
}

static void TEST_Main(void)
{
    if (!TEST_Init())
    {
        skip("Unable to start testing.\n");
        TEST_Quit();
        return;
    }

    // Message loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    TEST_Quit();
}

START_TEST(SHChangeNotify)
{
    trace("Please close all Explorer windows before testing.\n");
    trace("Please don't operate your PC while testing.\n");

    DWORD dwOldTick = GetTickCount();
    TEST_Main();
    DWORD dwNewTick = GetTickCount();

    DWORD dwTick = dwNewTick - dwOldTick;
    trace("SHChangeNotify: Total %lu.%lu sec\n", (dwTick / 1000), (dwTick / 100 % 10));
}
