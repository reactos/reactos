/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for OpenAs_RunDLL
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shelltest.h"
#include <stdio.h>
#include <process.h>

// OpenAs_RunDLLA
typedef void (WINAPI *OPENAS_RUNDLLA)(HWND, HINSTANCE, LPCSTR, int);
// OpenAs_RunDLLW
typedef void (WINAPI *OPENAS_RUNDLLW)(HWND, HINSTANCE, LPCWSTR, int);

static OPENAS_RUNDLLA pOpenAs_RunDLLA = NULL;
static OPENAS_RUNDLLW pOpenAs_RunDLLW = NULL;

struct TEST_ENTRY
{
    int         nLine;
    BOOL        bWide;
    HINSTANCE   hInst;
    int         nRet;
    BOOL        bCreateFile;
    LPCSTR      pszFileA;
    LPCWSTR     pszFileW;
    DWORD       dwError;
};

#define COUNT       5
#define INTERVAL    200

#define DEATH       999
#define OPENED      1
#define NOT_OPENED  0

#define BAD_INST   ((HINSTANCE)0xDEAD)
#define BAD_SZ_A   ((LPCSTR)0xDEAD)
#define BAD_SZ_W   ((LPCWSTR)0xDEAD)

static const TEST_ENTRY s_TestEntries[] =
{
    // ANSI
    {__LINE__, FALSE, NULL, OPENED, FALSE, NULL, NULL, 0 },
    {__LINE__, FALSE, NULL, OPENED, FALSE, "", NULL, 0 },
    {__LINE__, FALSE, NULL, OPENED, FALSE, "invalid file name.txt", NULL, 0 },
    {__LINE__, FALSE, NULL, DEATH, FALSE, BAD_SZ_A, NULL, 0 },
    {__LINE__, FALSE, NULL, OPENED, TRUE, "created file.txt", NULL, 0 },
    {__LINE__, FALSE, BAD_INST, OPENED, FALSE, NULL, NULL, 0 },
    {__LINE__, FALSE, BAD_INST, OPENED, FALSE, "invalid file name.txt", NULL, 0 },
    {__LINE__, FALSE, BAD_INST, DEATH, FALSE, BAD_SZ_A, NULL, 0xDEADFACE },
    {__LINE__, FALSE, BAD_INST, OPENED, TRUE, "created file.txt", NULL, 0 },
    // WIDE
    {__LINE__, TRUE, NULL, OPENED, FALSE, NULL, NULL, 0x80070490 },
    {__LINE__, TRUE, NULL, OPENED, FALSE, NULL, L"", ERROR_NO_SCROLLBARS },
    {__LINE__, TRUE, NULL, OPENED, FALSE, NULL, L"invalid file name.txt", ERROR_NO_SCROLLBARS },
    {__LINE__, TRUE, NULL, OPENED, FALSE, NULL, BAD_SZ_W, 0xDEADFACE },
    {__LINE__, TRUE, NULL, OPENED, TRUE, NULL, L"created file.txt", ERROR_NO_SCROLLBARS },
    {__LINE__, TRUE, BAD_INST, OPENED, FALSE, NULL, NULL, 0x80070490 },
    {__LINE__, TRUE, BAD_INST, OPENED, FALSE, NULL, L"invalid file name.txt", ERROR_NO_SCROLLBARS },
    {__LINE__, TRUE, BAD_INST, OPENED, FALSE, NULL, BAD_SZ_W, 0xDEADFACE },
    {__LINE__, TRUE, BAD_INST, OPENED, TRUE, NULL, L"created file.txt", ERROR_NO_SCROLLBARS },
};

static HANDLE s_hThread = NULL;
static INT s_nRet = NOT_OPENED;

static unsigned __stdcall
watch_thread_proc(void *arg)
{
    for (int i = 0; i < COUNT; ++i)
    {
        Sleep(INTERVAL);

        HWND hwnd = FindWindowA("#32770", "Open With");
        if (hwnd == NULL)
            continue;

        if (!IsWindowVisible(hwnd))
        {
            trace("not visible\n");
            continue;
        }

        s_nRet = OPENED;
        PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), 0);
        break;
    }
    return 0;
}

static void StartWatchGUI(const TEST_ENTRY *pEntry)
{
    s_nRet = NOT_OPENED;
    s_hThread = (HANDLE)_beginthreadex(NULL, 0, watch_thread_proc, NULL, 0, NULL);
}

static void DoEntry(const TEST_ENTRY *pEntry)
{
    if (pEntry->bWide)
    {
        if (pEntry->bCreateFile)
        {
            FILE *fp = _wfopen(pEntry->pszFileW, L"wb");
            ok(fp != NULL, "Line %d: Unable to create file '%s'\n", pEntry->nLine, wine_dbgstr_w(pEntry->pszFileW));
            fclose(fp);
        }

        StartWatchGUI(pEntry);
        SetLastError(0xDEADFACE);

        _SEH2_TRY
        {
            (*pOpenAs_RunDLLW)(NULL, pEntry->hInst, pEntry->pszFileW, SW_SHOWDEFAULT);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            s_nRet = DEATH;
        }
        _SEH2_END;

        DWORD dwError = GetLastError();
        ok(pEntry->dwError == dwError, "Line %d: error expected %ld, was %ld\n", pEntry->nLine, pEntry->dwError, dwError);

        WaitForSingleObject(s_hThread, INFINITE);
        CloseHandle(s_hThread);

        if (pEntry->bCreateFile)
        {
            ok(DeleteFileW(pEntry->pszFileW), "Line %d: DeleteFileA failed\n", pEntry->nLine);
        }
    }
    else
    {
        if (pEntry->bCreateFile)
        {
            FILE *fp = fopen(pEntry->pszFileA, "wb");
            ok(fp != NULL, "Line %d: Unable to create file '%s'\n", pEntry->nLine, pEntry->pszFileA);
            fclose(fp);
        }

        StartWatchGUI(pEntry);
        SetLastError(0xDEADFACE);

        _SEH2_TRY
        {
            (*pOpenAs_RunDLLA)(NULL, pEntry->hInst, pEntry->pszFileA, SW_SHOWDEFAULT);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            s_nRet = DEATH;
        }
        _SEH2_END;

        DWORD dwError = GetLastError();
        ok(pEntry->dwError == dwError, "Line %d: error expected %ld, was %ld\n", pEntry->nLine, pEntry->dwError, dwError);

        WaitForSingleObject(s_hThread, INFINITE);
        CloseHandle(s_hThread);

        if (pEntry->bCreateFile)
        {
            ok(DeleteFileA(pEntry->pszFileA), "Line %d: DeleteFileA failed\n", pEntry->nLine);
        }
    }

    // FIXME: This function probably returns void
    //ok(pEntry->nRet == s_nRet, "Line %d: s_nRet expected %d, was %d\n", pEntry->nLine, pEntry->nRet, s_nRet);
}

START_TEST(OpenAs_RunDLL)
{
    HINSTANCE hShell32 = LoadLibraryA("shell32.dll");
    if (hShell32 == NULL)
    {
        skip("Unable to load shell32.dll\n");
        return;
    }

    pOpenAs_RunDLLA = (OPENAS_RUNDLLA)GetProcAddress(hShell32, "OpenAs_RunDLLA");
    if (pOpenAs_RunDLLA == NULL)
    {
        skip("Unable to get OpenAs_RunDLLA\n");
        return;
    }

    pOpenAs_RunDLLW = (OPENAS_RUNDLLW)GetProcAddress(hShell32, "OpenAs_RunDLLW");
    if (pOpenAs_RunDLLW == NULL)
    {
        skip("Unable to get OpenAs_RunDLLW\n");
        return;
    }

    TCHAR szCurDir[MAX_PATH], szTempDir[MAX_PATH];

    ok(GetCurrentDirectory(_countof(szCurDir), szCurDir), "GetCurrentDirectory failed\n");
    ok(GetTempPath(_countof(szTempDir), szTempDir), "GetTempPath failed\n");

    if (!SetCurrentDirectory(szTempDir))
    {
        skip("SetCurrentDirectory failed\n");
    }
    else
    {
        for (size_t i = 0; i < _countof(s_TestEntries); ++i)
        {
            const TEST_ENTRY *pEntry = &s_TestEntries[i];
            DoEntry(pEntry);
        }

        ok(SetCurrentDirectory(szCurDir), "SetCurrentDirectory failed\n");
    }

    FreeLibrary(hShell32);
}
