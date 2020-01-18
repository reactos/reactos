/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for ShellExecCmdLine
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#include "shelltest.h"
#include <shlwapi.h>
#include <strsafe.h>
#include <versionhelpers.h>

#define NDEBUG
#include <debug.h>
#include <stdio.h>

//#define ShellExecCmdLine ShellExecCmdLine

#ifndef SECL_NO_UI
    #define SECL_NO_UI          0x2
    #define SECL_LOG_USAGE      0x8
    #define SECL_USE_IDLIST     0x10
    #define SECL_ALLOW_NONEXE   0x20
    #define SECL_RUNAS          0x40
#endif

#ifdef ShellExecCmdLine

#define shell32_hInstance   GetModuleHandle(NULL)
#define IDS_FILE_NOT_FOUND  (-1)

static const WCHAR wszOpen[] = L"open";
static const WCHAR wszExe[] = L".exe";
static const WCHAR wszCom[] = L".com";

static __inline void __SHCloneStrW(WCHAR **target, const WCHAR *source)
{
	*target = (WCHAR *)SHAlloc((lstrlenW(source) + 1) * sizeof(WCHAR) );
	lstrcpyW(*target, source);
}

static LPCWSTR
SplitParams(LPCWSTR psz, LPWSTR pszArg0, size_t cchArg0)
{
    LPCWSTR pch;
    size_t ich = 0;
    if (*psz == L'"')
    {
        // 1st argument is quoted. the string in quotes is quoted 1st argument.
        // [pch] --> [pszArg0+ich]
        for (pch = psz + 1; *pch && ich + 1 < cchArg0; ++ich, ++pch)
        {
            if (*pch == L'"' && pch[1] == L'"')
            {
                // doubled double quotations found!
                pszArg0[ich] = L'"';
            }
            else if (*pch == L'"')
            {
                // single double quotation found!
                ++pch;
                break;
            }
            else
            {
                // otherwise
                pszArg0[ich] = *pch;
            }
        }
    }
    else
    {
        // 1st argument is unquoted. non-space sequence is 1st argument.
        // [pch] --> [pszArg0+ich]
        for (pch = psz; *pch && !iswspace(*pch) && ich + 1 < cchArg0; ++ich, ++pch)
        {
            pszArg0[ich] = *pch;
        }
    }
    pszArg0[ich] = 0;

    // skip space
    while (iswspace(*pch))
        ++pch;

    return pch;
}

HRESULT WINAPI ShellExecCmdLine(
    HWND hwnd,
    LPCWSTR pwszCommand,
    LPCWSTR pwszStartDir,
    int nShow,
    LPVOID pUnused,
    DWORD dwSeclFlags)
{
    SHELLEXECUTEINFOW info;
    DWORD dwSize, dwError, dwType, dwFlags = SEE_MASK_DOENVSUBST | SEE_MASK_NOASYNC;
    LPCWSTR pszVerb = NULL;
    WCHAR szFile[MAX_PATH], szFile2[MAX_PATH];
    HRESULT hr;
    LPCWSTR pchParams;
    LPWSTR lpCommand = NULL;

    if (pwszCommand == NULL)
        RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE,
                       1, (ULONG_PTR*)pwszCommand);

    __SHCloneStrW(&lpCommand, pwszCommand);
    StrTrimW(lpCommand, L" \t");

    if (dwSeclFlags & SECL_NO_UI)
        dwFlags |= SEE_MASK_FLAG_NO_UI;
    if (dwSeclFlags & SECL_LOG_USAGE)
        dwFlags |= SEE_MASK_FLAG_LOG_USAGE;
    if (dwSeclFlags & SECL_USE_IDLIST)
        dwFlags |= SEE_MASK_INVOKEIDLIST;

    if (dwSeclFlags & SECL_RUNAS)
    {
        dwSize = 0;
        hr = AssocQueryStringW(0, ASSOCSTR_COMMAND, lpCommand, L"RunAs", NULL, &dwSize);
        if (SUCCEEDED(hr) && dwSize != 0)
        {
            pszVerb = L"runas";
        }
    }

    if (UrlIsFileUrlW(lpCommand))
    {
        StringCchCopyW(szFile, _countof(szFile), lpCommand);
        pchParams = NULL;
    }
    else
    {
        pchParams = SplitParams(lpCommand, szFile, _countof(szFile));
        if (SearchPathW(NULL, szFile, NULL, _countof(szFile2), szFile2, NULL) ||
            SearchPathW(NULL, szFile, wszExe, _countof(szFile2), szFile2, NULL) ||
            SearchPathW(NULL, szFile, wszCom, _countof(szFile2), szFile2, NULL) ||
            SearchPathW(pwszStartDir, szFile, NULL, _countof(szFile2), szFile2, NULL) ||
            SearchPathW(pwszStartDir, szFile, wszExe, _countof(szFile2), szFile2, NULL) ||
            SearchPathW(pwszStartDir, szFile, wszCom, _countof(szFile2), szFile2, NULL))
        {
            StringCchCopyW(szFile, _countof(szFile), szFile2);
        }
        else if (SearchPathW(NULL, lpCommand, NULL, _countof(szFile2), szFile2, NULL) ||
                 SearchPathW(NULL, lpCommand, wszExe, _countof(szFile2), szFile2, NULL) ||
                 SearchPathW(NULL, lpCommand, wszCom, _countof(szFile2), szFile2, NULL) ||
                 SearchPathW(pwszStartDir, lpCommand, NULL, _countof(szFile2), szFile2, NULL) ||
                 SearchPathW(pwszStartDir, lpCommand, wszExe, _countof(szFile2), szFile2, NULL) ||
                 SearchPathW(pwszStartDir, lpCommand, wszCom, _countof(szFile2), szFile2, NULL))
        {
            StringCchCopyW(szFile, _countof(szFile), szFile2);
            pchParams = NULL;
        }

        if (!(dwSeclFlags & SECL_ALLOW_NONEXE))
        {
            if (!GetBinaryTypeW(szFile, &dwType))
            {
                SHFree(lpCommand);

                if (!(dwSeclFlags & SECL_NO_UI))
                {
                    WCHAR szText[128 + MAX_PATH], szFormat[128];
                    LoadStringW(shell32_hInstance, IDS_FILE_NOT_FOUND, szFormat, _countof(szFormat));
                    StringCchPrintfW(szText, _countof(szText), szFormat, szFile);
                    MessageBoxW(hwnd, szText, NULL, MB_ICONERROR);
                }
                return CO_E_APPNOTFOUND;
            }
        }
        else
        {
            if (GetFileAttributesW(szFile) == INVALID_FILE_ATTRIBUTES)
            {
                SHFree(lpCommand);

                if (!(dwSeclFlags & SECL_NO_UI))
                {
                    WCHAR szText[128 + MAX_PATH], szFormat[128];
                    LoadStringW(shell32_hInstance, IDS_FILE_NOT_FOUND, szFormat, _countof(szFormat));
                    StringCchPrintfW(szText, _countof(szText), szFormat, szFile);
                    MessageBoxW(hwnd, szText, NULL, MB_ICONERROR);
                }
                return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }
        }
    }

    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = dwFlags;
    info.hwnd = hwnd;
    info.lpVerb = pszVerb;
    info.lpFile = szFile;
    info.lpParameters = (pchParams && *pchParams) ? pchParams : NULL;
    info.lpDirectory = pwszStartDir;
    info.nShow = nShow;
    if (ShellExecuteExW(&info))
    {
        if (info.lpIDList)
            CoTaskMemFree(info.lpIDList);

        SHFree(lpCommand);

        return S_OK;
    }

    dwError = GetLastError();

    SHFree(lpCommand);

    return HRESULT_FROM_WIN32(dwError);
}
#else
    typedef HRESULT (WINAPI *SHELLEXECCMDLINE)(HWND, LPCWSTR, LPCWSTR, INT, LPVOID, DWORD);
    SHELLEXECCMDLINE g_pShellExecCmdLine = NULL;
#endif

typedef struct TEST_ENTRY
{
    INT lineno;
    HRESULT hr;
    BOOL bAllowNonExe;
    LPCWSTR pwszWindowClass;
    LPCWSTR pwszCommand;
    LPCWSTR pwszStartDir;
} TEST_ENTRY;

static const char s_testfile1[] = "Test File.txt";
static const char s_testfile2[] = "Test File.bat";
static char s_notepad[] = "notepad.exe";

static const TEST_ENTRY s_entries[] =
{
    // NULL
    { __LINE__, (HRESULT)0xDEADFACE, FALSE, NULL, NULL, NULL },
    { __LINE__, (HRESULT)0xDEADFACE, FALSE, NULL, NULL, L"." },
    { __LINE__, (HRESULT)0xDEADFACE, FALSE, NULL, NULL, L"system32" },
    { __LINE__, (HRESULT)0xDEADFACE, FALSE, NULL, NULL, L"C:\\Program Files" },
    { __LINE__, (HRESULT)0xDEADFACE, TRUE, NULL, NULL, NULL },
    { __LINE__, (HRESULT)0xDEADFACE, TRUE, NULL, NULL, L"." },
    { __LINE__, (HRESULT)0xDEADFACE, TRUE, NULL, NULL, L"system32" },
    { __LINE__, (HRESULT)0xDEADFACE, TRUE, NULL, NULL, L"C:\\Program Files" },
    // notepad
    { __LINE__, S_OK, FALSE, L"Notepad", L"notepad", NULL },
    { __LINE__, S_OK, FALSE, L"Notepad", L"notepad", L"." },
    { __LINE__, S_OK, FALSE, L"Notepad", L"notepad", L"system32" },
    { __LINE__, S_OK, FALSE, L"Notepad", L"notepad", L"C:\\Program Files" },
    { __LINE__, S_OK, FALSE, L"Notepad", L"notepad \"Test File.txt\"", NULL },
    { __LINE__, S_OK, FALSE, L"Notepad", L"notepad \"Test File.txt\"", L"." },
    { __LINE__, S_OK, TRUE, L"Notepad", L"notepad", NULL },
    { __LINE__, S_OK, TRUE, L"Notepad", L"notepad", L"." },
    { __LINE__, S_OK, TRUE, L"Notepad", L"notepad", L"system32" },
    { __LINE__, S_OK, TRUE, L"Notepad", L"notepad", L"C:\\Program Files" },
    { __LINE__, S_OK, TRUE, L"Notepad", L"notepad \"Test File.txt\"", NULL },
    { __LINE__, S_OK, TRUE, L"Notepad", L"notepad \"Test File.txt\"", L"." },
    // notepad.exe
    { __LINE__, S_OK, FALSE, L"Notepad", L"notepad.exe", NULL },
    { __LINE__, S_OK, FALSE, L"Notepad", L"notepad.exe", L"." },
    { __LINE__, S_OK, FALSE, L"Notepad", L"notepad.exe", L"system32" },
    { __LINE__, S_OK, FALSE, L"Notepad", L"notepad.exe", L"C:\\Program Files" },
    { __LINE__, S_OK, FALSE, L"Notepad", L"notepad.exe \"Test File.txt\"", NULL },
    { __LINE__, S_OK, FALSE, L"Notepad", L"notepad.exe \"Test File.txt\"", L"." },
    { __LINE__, S_OK, TRUE, L"Notepad", L"notepad.exe", NULL },
    { __LINE__, S_OK, TRUE, L"Notepad", L"notepad.exe", L"." },
    { __LINE__, S_OK, TRUE, L"Notepad", L"notepad.exe", L"system32" },
    { __LINE__, S_OK, TRUE, L"Notepad", L"notepad.exe", L"C:\\Program Files" },
    { __LINE__, S_OK, TRUE, L"Notepad", L"notepad.exe \"Test File.txt\"", NULL },
    { __LINE__, S_OK, TRUE, L"Notepad", L"notepad.exe \"Test File.txt\"", L"." },
    // C:\notepad.exe
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"C:\\notepad.exe", NULL },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"C:\\notepad.exe", L"." },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"C:\\notepad.exe", L"system32" },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"C:\\notepad.exe", L"C:\\Program Files" },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"C:\\notepad.exe \"Test File.txt\"", NULL },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"C:\\notepad.exe \"Test File.txt\"", L"." },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"C:\\notepad.exe", NULL },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"C:\\notepad.exe", L"." },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"C:\\notepad.exe", L"system32" },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"C:\\notepad.exe", L"C:\\Program Files" },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"C:\\notepad.exe \"Test File.txt\"", NULL },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"C:\\notepad.exe \"Test File.txt\"", L"." },
    // "notepad"
    { __LINE__, S_OK, FALSE, L"Notepad", L"\"notepad\"", NULL },
    { __LINE__, S_OK, FALSE, L"Notepad", L"\"notepad\"", L"." },
    { __LINE__, S_OK, FALSE, L"Notepad", L"\"notepad\"", L"system32" },
    { __LINE__, S_OK, FALSE, L"Notepad", L"\"notepad\"", L"C:\\Program Files" },
    { __LINE__, S_OK, FALSE, L"Notepad", L"\"notepad\" \"Test File.txt\"", NULL },
    { __LINE__, S_OK, FALSE, L"Notepad", L"\"notepad\" \"Test File.txt\"", L"." },
    { __LINE__, S_OK, TRUE, L"Notepad", L"\"notepad\"", NULL },
    { __LINE__, S_OK, TRUE, L"Notepad", L"\"notepad\"", L"." },
    { __LINE__, S_OK, TRUE, L"Notepad", L"\"notepad\"", L"system32" },
    { __LINE__, S_OK, TRUE, L"Notepad", L"\"notepad\"", L"C:\\Program Files" },
    { __LINE__, S_OK, TRUE, L"Notepad", L"\"notepad\" \"Test File.txt\"", NULL },
    { __LINE__, S_OK, TRUE, L"Notepad", L"\"notepad\" \"Test File.txt\"", L"." },
    // "notepad.exe"
    { __LINE__, S_OK, FALSE, L"Notepad", L"\"notepad.exe\"", NULL },
    { __LINE__, S_OK, FALSE, L"Notepad", L"\"notepad.exe\"", L"." },
    { __LINE__, S_OK, FALSE, L"Notepad", L"\"notepad.exe\"", L"system32" },
    { __LINE__, S_OK, FALSE, L"Notepad", L"\"notepad.exe\"", L"C:\\Program Files" },
    { __LINE__, S_OK, FALSE, L"Notepad", L"\"notepad.exe\" \"Test File.txt\"", NULL },
    { __LINE__, S_OK, FALSE, L"Notepad", L"\"notepad.exe\" \"Test File.txt\"", L"." },
    { __LINE__, S_OK, TRUE, L"Notepad", L"\"notepad.exe\"", NULL },
    { __LINE__, S_OK, TRUE, L"Notepad", L"\"notepad.exe\"", L"." },
    { __LINE__, S_OK, TRUE, L"Notepad", L"\"notepad.exe\"", L"system32" },
    { __LINE__, S_OK, TRUE, L"Notepad", L"\"notepad.exe\"", L"C:\\Program Files" },
    { __LINE__, S_OK, TRUE, L"Notepad", L"\"notepad.exe\" \"Test File.txt\"", NULL },
    { __LINE__, S_OK, TRUE, L"Notepad", L"\"notepad.exe\" \"Test File.txt\"", L"." },
    // test program.exe
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"test program.exe", NULL },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"test program.exe", L"." },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"test program.exe", L"system32" },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"test program.exe", L"C:\\Program Files" },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"test program.exe \"Test File.txt\"", NULL },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"test program.exe \"Test File.txt\"", L"." },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"test program.exe", NULL },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"test program.exe", L"." },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"test program.exe", L"system32" },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"test program.exe", L"C:\\Program Files" },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"test program.exe \"Test File.txt\"", NULL },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"test program.exe \"Test File.txt\"", L"." },
    // "test program"
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"test program\"", NULL },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"test program\"", L"." },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"test program\"", L"system32" },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"test program\"", L"C:\\Program Files" },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"test program\" \"Test File.txt\"", NULL },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"test program\" \"Test File.txt\"", L"." },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"test program\"", NULL },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"test program\"", L"." },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"test program\"", L"system32" },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"test program\"", L"C:\\Program Files" },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"test program\" \"Test File.txt\"", NULL },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"test program\" \"Test File.txt\"", L"." },
    // "test program.exe"
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"test program.exe\"", NULL },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"test program.exe\"", L"." },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"test program.exe\"", L"system32" },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"test program.exe\"", L"C:\\Program Files" },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"test program.exe\" \"Test File.txt\"", NULL },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"test program.exe\" \"Test File.txt\"", L"." },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"test program.exe\"", NULL },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"test program.exe\"", L"." },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"test program.exe\"", L"system32" },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"test program.exe\"", L"C:\\Program Files" },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"test program.exe\" \"Test File.txt\"", NULL },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"test program.exe\" \"Test File.txt\"", L"." },
    // invalid program
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"invalid program", NULL },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"invalid program", L"." },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"invalid program", L"system32" },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"invalid program", L"C:\\Program Files" },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"invalid program \"Test File.txt\"", NULL },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"invalid program \"Test File.txt\"", L"." },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"invalid program", NULL },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"invalid program", L"." },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"invalid program", L"system32" },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"invalid program", L"C:\\Program Files" },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"invalid program \"Test File.txt\"", NULL },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"invalid program \"Test File.txt\"", L"." },
    // \"invalid program.exe\"
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"invalid program.exe\"", NULL },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"invalid program.exe\"", L"." },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"invalid program.exe\"", L"system32" },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"invalid program.exe\"", L"C:\\Program Files" },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"invalid program.exe\" \"Test File.txt\"", NULL },
    { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", L"\"invalid program.exe\" \"Test File.txt\"", L"." },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"invalid program.exe\"", NULL },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"invalid program.exe\"", L"." },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"invalid program.exe\"", L"system32" },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"invalid program.exe\"", L"C:\\Program Files" },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"invalid program.exe\" \"Test File.txt\"", NULL },
    { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", L"\"invalid program.exe\" \"Test File.txt\"", L"." },
};

static void DoEntry(const TEST_ENTRY *pEntry)
{
    HRESULT hr;
    DWORD dwSeclFlags;

    if (pEntry->bAllowNonExe)
        dwSeclFlags = SECL_NO_UI | SECL_ALLOW_NONEXE;
    else
        dwSeclFlags = SECL_NO_UI;

    _SEH2_TRY
    {
#ifdef ShellExecCmdLine
        hr = ShellExecCmdLine(NULL, pEntry->pwszCommand, pEntry->pwszStartDir,
                              SW_SHOWNORMAL, NULL, dwSeclFlags);
#else
        hr = (*g_pShellExecCmdLine)(NULL, pEntry->pwszCommand, pEntry->pwszStartDir,
                                    SW_SHOWNORMAL, NULL, dwSeclFlags);
#endif
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = 0xDEADFACE;
    }
    _SEH2_END;

    ok(hr == pEntry->hr, "Line %d: hr expected 0x%lX, was 0x%lX\n", pEntry->lineno, pEntry->hr, hr);

#define RETRY_COUNT     5
#define RETRY_INTERVAL  250
    if (SUCCEEDED(hr) && pEntry->pwszWindowClass)
    {
        BOOL bFound = FALSE;
        Sleep(RETRY_INTERVAL / 2);
        for (int i = 0; i < RETRY_COUNT; ++i)
        {
            HWND hwnd = FindWindowW(pEntry->pwszWindowClass, NULL);
            if (hwnd)
            {
                bFound = TRUE;
                SendMessage(hwnd, WM_CLOSE, 0, 0);
                Sleep(RETRY_INTERVAL);
                break;
            }
            Sleep(RETRY_INTERVAL);
        }
        ok(bFound, "Line %d: The window not found\n", pEntry->lineno);
    }
#undef RETRY_COUNT
#undef RETRY_INTERVAL
}

START_TEST(ShellExecCmdLine)
{
    using namespace std;

#ifndef ShellExecCmdLine
    // CHECKME
    if (!IsWindowsVistaOrGreater())
    {
        skip("ShellExecCmdLine is not available on this platform\n");
        return;
    }

    HMODULE hShell32 = GetModuleHandleA("shell32");
    g_pShellExecCmdLine = (SHELLEXECCMDLINE)GetProcAddress(hShell32, (LPCSTR)(INT_PTR)265);
    if (!g_pShellExecCmdLine)
    {
        skip("ShellExecCmdLine is not found\n");
        return;
    }
#endif

    // s_testfile1
    FILE *fp = fopen(s_testfile1, "wb");
    ok(fp != NULL, "failed to create a test file\n");
    fclose(fp);

    // s_testfile2
    fp = fopen(s_testfile2, "wb");
    ok(fp != NULL, "failed to create a test file\n");
    if (fp)
    {
        fprintf(fp, "echo OK\n");
    }
    fclose(fp);

    for (size_t i = 0; i < _countof(s_entries); ++i)
    {
        DoEntry(&s_entries[i]);
    }

    WCHAR buf0[MAX_PATH];
    WCHAR buf1[MAX_PATH];
    WCHAR buf2[MAX_PATH];
    TEST_ENTRY additionals[] =
    {
        { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", buf0, NULL },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", buf0, L"." },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", buf0, L"system32" },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", buf1, NULL },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", buf1, L"." },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", buf1, L"system32" },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", buf2, NULL },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", buf2, L"." },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, L"Notepad", buf2, L"system32" },
        { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", buf0, NULL }, // FIXME
        { __LINE__, S_OK, TRUE, L"Notepad", buf0, L"." },
        { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", buf0, L"system32" }, // FIXME
        { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", buf1, NULL }, // FIXME
        { __LINE__, S_OK, TRUE, L"Notepad", buf1, L"." },
        { __LINE__, S_OK, TRUE, L"Notepad", buf1, L"system32" },
        { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, L"Notepad", buf2, NULL }, // FIXME
        { __LINE__, S_OK, TRUE, L"Notepad", buf2, L"." },
        { __LINE__, S_OK, TRUE, L"Notepad", buf2, L"system32" },
    };

    wsprintfW(buf0, L"%hs", s_testfile1);
    wsprintfW(buf1, L"\"%hs\"", s_testfile1);
    wsprintfW(buf2, L"\"%hs\" \"Test File.txt\"", s_testfile1);
    for (size_t i = 0; i < _countof(additionals); ++i)
    {
        DoEntry(&additionals[i]);
    }

    TEST_ENTRY additionals2[] =
    {
        { __LINE__, CO_E_APPNOTFOUND, FALSE, NULL, buf0, NULL },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, NULL, buf0, L"." },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, NULL, buf0, L"system32" },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, NULL, buf1, NULL },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, NULL, buf1, L"." },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, NULL, buf1, L"system32" },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, NULL, buf2, NULL },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, NULL, buf2, L"." },
        { __LINE__, CO_E_APPNOTFOUND, FALSE, NULL, buf2, L"system32" },
        { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, NULL, buf0, NULL }, // FIXME
        { __LINE__, S_OK, TRUE, NULL, buf0, L"." },
        { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, NULL, buf0, L"system32" },  // FIXME
        { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, NULL, buf1, NULL }, // FIXME
        { __LINE__, S_OK, TRUE, NULL, buf1, L"." },
        { __LINE__, S_OK, TRUE, NULL, buf1, L"system32" },
        { __LINE__, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), TRUE, NULL, buf2, NULL }, // FIXME
        { __LINE__, S_OK, TRUE, NULL, buf2, L"." },
        { __LINE__, S_OK, TRUE, NULL, buf2, L"system32" },
    };

    wsprintfW(buf0, L"%hs", s_testfile2);
    wsprintfW(buf1, L"\"%hs\"", s_testfile2);
    wsprintfW(buf2, L"\"%hs\" \"Test File.txt\"", s_testfile2);
    for (size_t i = 0; i < _countof(additionals2); ++i)
    {
        DoEntry(&additionals2[i]);
    }

    char path[MAX_PATH];
    ok((INT_PTR)FindExecutableA("notepad.exe", NULL, s_notepad) >= 32, "FindExecutableA failed\n");
    ok(GetModuleFileNameA(NULL, path, _countof(path)), "GetModuleFileNameA failed\n");
    char *pch = strrchr(path, '\\');

    if (pch == NULL)
    {
        skip("pch == NULL\n");
    }
    else
    {
        // create "My Directory"
        strcpy(pch, "\\My Directory");
        if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
            ok(CreateDirectoryA(path, NULL), "CreateDirectoryA failed\n");

        // create "My Directory\\Notepad.exe" as clone of Notepad.exe
        strcpy(pch, "\\My Directory\\Notepad.exe");
        ok(CopyFileA(s_notepad, path, FALSE), "CopyFileA failed\n");

        wsprintfW(buf0, L"%hs", path);
        wsprintfW(buf1, L"\"%hs\"", path);
        wsprintfW(buf2, L"\"%hs\" \"Test File.txt\"", path);
        TEST_ENTRY additionals3[] =
        {
            { __LINE__, S_OK, FALSE, NULL, buf0, NULL },
            { __LINE__, S_OK, FALSE, NULL, buf0, L"." },
            { __LINE__, S_OK, FALSE, NULL, buf0, L"system32" },
            { __LINE__, S_OK, FALSE, NULL, buf1, NULL },
            { __LINE__, S_OK, FALSE, NULL, buf1, L"." },
            { __LINE__, S_OK, FALSE, NULL, buf1, L"system32" },
            { __LINE__, S_OK, FALSE, NULL, buf2, NULL },
            { __LINE__, S_OK, FALSE, NULL, buf2, L"." },
            { __LINE__, S_OK, FALSE, NULL, buf2, L"system32" },
            { __LINE__, S_OK, TRUE, NULL, buf0, NULL },
            { __LINE__, S_OK, TRUE, NULL, buf0, L"." },
            { __LINE__, S_OK, TRUE, NULL, buf0, L"system32" },
            { __LINE__, S_OK, TRUE, NULL, buf1, NULL },
            { __LINE__, S_OK, TRUE, NULL, buf1, L"." },
            { __LINE__, S_OK, TRUE, NULL, buf1, L"system32" },
            { __LINE__, S_OK, TRUE, NULL, buf2, NULL },
            { __LINE__, S_OK, TRUE, NULL, buf2, L"." },
            { __LINE__, S_OK, TRUE, NULL, buf2, L"system32" },
        };
        for (size_t i = 0; i < _countof(additionals3); ++i)
        {
            DoEntry(&additionals3[i]);
        }

        DeleteFileA(path);

        strcpy(pch, "\\My Directory");
        RemoveDirectory(path);
    }

    // clean up
    ok(DeleteFileA(s_testfile1), "failed to delete the test file\n");
    ok(DeleteFileA(s_testfile2), "failed to delete the test file\n");
}
