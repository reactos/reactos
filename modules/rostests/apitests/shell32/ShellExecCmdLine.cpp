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
#include "shell32_apitest_sub.h"

#define NDEBUG
#include <debug.h>
#include <stdio.h>

#ifndef SECL_NO_UI
    #define SECL_NO_UI          0x2
    #define SECL_LOG_USAGE      0x8
    #define SECL_USE_IDLIST     0x10
    #define SECL_ALLOW_NONEXE   0x20
    #define SECL_RUNAS          0x40
#endif

#define ShellExecCmdLine proxy_ShellExecCmdLine

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

// NOTE: You have to sync the following code to dll/win32/shell32/shlexec.cpp.
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
        if (szFile[0] != UNICODE_NULL && szFile[1] == L':' &&
            szFile[2] == UNICODE_NULL)
        {
            PathAddBackslashW(szFile);
        }

        WCHAR szCurDir[MAX_PATH];
        GetCurrentDirectoryW(_countof(szCurDir), szCurDir);
        if (pwszStartDir)
        {
            SetCurrentDirectoryW(pwszStartDir);
        }

        if (PathIsRelativeW(szFile) &&
            GetFullPathNameW(szFile, _countof(szFile2), szFile2, NULL) &&
            PathFileExistsW(szFile2))
        {
            StringCchCopyW(szFile, _countof(szFile), szFile2);
        }
        else if (SearchPathW(NULL, szFile, NULL, _countof(szFile2), szFile2, NULL) ||
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

        if (pwszStartDir)
        {
            SetCurrentDirectoryW(szCurDir);
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

#undef ShellExecCmdLine

typedef HRESULT (WINAPI *SHELLEXECCMDLINE)(HWND, LPCWSTR, LPCWSTR, INT, LPVOID, DWORD);
SHELLEXECCMDLINE g_pShellExecCmdLine = NULL;

typedef struct TEST_ENTRY
{
    INT lineno;
    BOOL result;
    BOOL bAllowNonExe;
    LPCWSTR pwszCommand;
    LPCWSTR pwszStartDir;
} TEST_ENTRY;

static WCHAR s_sub_program[MAX_PATH];
static WCHAR s_win_test_exe[MAX_PATH];
static WCHAR s_sys_bat_file[MAX_PATH];
static WCHAR s_cur_dir[MAX_PATH];

static BOOL
GetSubProgramPath(void)
{
    GetModuleFileNameW(NULL, s_sub_program, _countof(s_sub_program));
    PathRemoveFileSpecW(s_sub_program);
    PathAppendW(s_sub_program, L"shell32_apitest_sub.exe");

    if (!PathFileExistsW(s_sub_program))
    {
        PathRemoveFileSpecW(s_sub_program);
        PathAppendW(s_sub_program, L"testdata\\shell32_apitest_sub.exe");

        if (!PathFileExistsW(s_sub_program))
        {
            return FALSE;
        }
    }

    return TRUE;
}

static const TEST_ENTRY s_entries_1[] =
{
    // NULL
    { __LINE__, 0xBADFACE, FALSE, NULL, NULL },
    { __LINE__, 0xBADFACE, FALSE, NULL, L"." },
    { __LINE__, 0xBADFACE, FALSE, NULL, L"system32" },
    { __LINE__, 0xBADFACE, FALSE, NULL, L"C:\\Program Files" },
    { __LINE__, 0xBADFACE, TRUE, NULL, NULL },
    { __LINE__, 0xBADFACE, TRUE, NULL, L"." },
    { __LINE__, 0xBADFACE, TRUE, NULL, L"system32" },
    { __LINE__, 0xBADFACE, TRUE, NULL, L"C:\\Program Files" },
    // notepad
    { __LINE__, TRUE, FALSE, L"notepad", NULL },
    { __LINE__, TRUE, FALSE, L"notepad", L"." },
    { __LINE__, TRUE, FALSE, L"notepad", L"system32" },
    { __LINE__, TRUE, FALSE, L"notepad", L"C:\\Program Files" },
    { __LINE__, TRUE, FALSE, L"notepad \"Test File.txt\"", NULL },
    { __LINE__, TRUE, FALSE, L"notepad \"Test File.txt\"", L"." },
    { __LINE__, TRUE, TRUE, L"notepad", NULL },
    { __LINE__, TRUE, TRUE, L"notepad", L"." },
    { __LINE__, TRUE, TRUE, L"notepad", L"system32" },
    { __LINE__, TRUE, TRUE, L"notepad", L"C:\\Program Files" },
    { __LINE__, TRUE, TRUE, L"notepad \"Test File.txt\"", NULL },
    { __LINE__, TRUE, TRUE, L"notepad \"Test File.txt\"", L"." },
    // notepad.exe
    { __LINE__, TRUE, FALSE, L"notepad.exe", NULL },
    { __LINE__, TRUE, FALSE, L"notepad.exe", L"." },
    { __LINE__, TRUE, FALSE, L"notepad.exe", L"system32" },
    { __LINE__, TRUE, FALSE, L"notepad.exe", L"C:\\Program Files" },
    { __LINE__, TRUE, FALSE, L"notepad.exe \"Test File.txt\"", NULL },
    { __LINE__, TRUE, FALSE, L"notepad.exe \"Test File.txt\"", L"." },
    { __LINE__, TRUE, TRUE, L"notepad.exe", NULL },
    { __LINE__, TRUE, TRUE, L"notepad.exe", L"." },
    { __LINE__, TRUE, TRUE, L"notepad.exe", L"system32" },
    { __LINE__, TRUE, TRUE, L"notepad.exe", L"C:\\Program Files" },
    { __LINE__, TRUE, TRUE, L"notepad.exe \"Test File.txt\"", NULL },
    { __LINE__, TRUE, TRUE, L"notepad.exe \"Test File.txt\"", L"." },
    // C:\notepad.exe
    { __LINE__, FALSE, FALSE, L"C:\\notepad.exe", NULL },
    { __LINE__, FALSE, FALSE, L"C:\\notepad.exe", L"." },
    { __LINE__, FALSE, FALSE, L"C:\\notepad.exe", L"system32" },
    { __LINE__, FALSE, FALSE, L"C:\\notepad.exe", L"C:\\Program Files" },
    { __LINE__, FALSE, FALSE, L"C:\\notepad.exe \"Test File.txt\"", NULL },
    { __LINE__, FALSE, FALSE, L"C:\\notepad.exe \"Test File.txt\"", L"." },
    { __LINE__, FALSE, TRUE, L"C:\\notepad.exe", NULL },
    { __LINE__, FALSE, TRUE, L"C:\\notepad.exe", L"." },
    { __LINE__, FALSE, TRUE, L"C:\\notepad.exe", L"system32" },
    { __LINE__, FALSE, TRUE, L"C:\\notepad.exe", L"C:\\Program Files" },
    { __LINE__, FALSE, TRUE, L"C:\\notepad.exe \"Test File.txt\"", NULL },
    { __LINE__, FALSE, TRUE, L"C:\\notepad.exe \"Test File.txt\"", L"." },
    // "notepad"
    { __LINE__, TRUE, FALSE, L"\"notepad\"", NULL },
    { __LINE__, TRUE, FALSE, L"\"notepad\"", L"." },
    { __LINE__, TRUE, FALSE, L"\"notepad\"", L"system32" },
    { __LINE__, TRUE, FALSE, L"\"notepad\"", L"C:\\Program Files" },
    { __LINE__, TRUE, FALSE, L"\"notepad\" \"Test File.txt\"", NULL },
    { __LINE__, TRUE, FALSE, L"\"notepad\" \"Test File.txt\"", L"." },
    { __LINE__, TRUE, TRUE, L"\"notepad\"", NULL },
    { __LINE__, TRUE, TRUE, L"\"notepad\"", L"." },
    { __LINE__, TRUE, TRUE, L"\"notepad\"", L"system32" },
    { __LINE__, TRUE, TRUE, L"\"notepad\"", L"C:\\Program Files" },
    { __LINE__, TRUE, TRUE, L"\"notepad\" \"Test File.txt\"", NULL },
    { __LINE__, TRUE, TRUE, L"\"notepad\" \"Test File.txt\"", L"." },
    // "notepad.exe"
    { __LINE__, TRUE, FALSE, L"\"notepad.exe\"", NULL },
    { __LINE__, TRUE, FALSE, L"\"notepad.exe\"", L"." },
    { __LINE__, TRUE, FALSE, L"\"notepad.exe\"", L"system32" },
    { __LINE__, TRUE, FALSE, L"\"notepad.exe\"", L"C:\\Program Files" },
    { __LINE__, TRUE, FALSE, L"\"notepad.exe\" \"Test File.txt\"", NULL },
    { __LINE__, TRUE, FALSE, L"\"notepad.exe\" \"Test File.txt\"", L"." },
    { __LINE__, TRUE, TRUE, L"\"notepad.exe\"", NULL },
    { __LINE__, TRUE, TRUE, L"\"notepad.exe\"", L"." },
    { __LINE__, TRUE, TRUE, L"\"notepad.exe\"", L"system32" },
    { __LINE__, TRUE, TRUE, L"\"notepad.exe\"", L"C:\\Program Files" },
    { __LINE__, TRUE, TRUE, L"\"notepad.exe\" \"Test File.txt\"", NULL },
    { __LINE__, TRUE, TRUE, L"\"notepad.exe\" \"Test File.txt\"", L"." },
    // test program
    { __LINE__, FALSE, FALSE, L"test program", NULL },
    { __LINE__, FALSE, FALSE, L"test program", L"." },
    { __LINE__, FALSE, FALSE, L"test program", L"system32" },
    { __LINE__, FALSE, FALSE, L"test program", L"C:\\Program Files" },
    { __LINE__, FALSE, FALSE, L"test program \"Test File.txt\"", NULL },
    { __LINE__, FALSE, FALSE, L"test program \"Test File.txt\"", L"." },
    { __LINE__, FALSE, TRUE, L"test program", NULL },
    { __LINE__, FALSE, TRUE, L"test program", L"." },
    { __LINE__, FALSE, TRUE, L"test program", L"system32" },
    { __LINE__, FALSE, TRUE, L"test program", L"C:\\Program Files" },
    { __LINE__, FALSE, TRUE, L"test program \"Test File.txt\"", NULL },
    { __LINE__, FALSE, TRUE, L"test program \"Test File.txt\"", L"." },
    // test program.exe
    { __LINE__, FALSE, FALSE, L"test program.exe", NULL },
    { __LINE__, FALSE, FALSE, L"test program.exe", L"." },
    { __LINE__, FALSE, FALSE, L"test program.exe", L"system32" },
    { __LINE__, FALSE, FALSE, L"test program.exe", L"C:\\Program Files" },
    { __LINE__, FALSE, FALSE, L"test program.exe \"Test File.txt\"", NULL },
    { __LINE__, FALSE, FALSE, L"test program.exe \"Test File.txt\"", L"." },
    { __LINE__, FALSE, TRUE, L"test program.exe", NULL },
    { __LINE__, FALSE, TRUE, L"test program.exe", L"." },
    { __LINE__, FALSE, TRUE, L"test program.exe", L"system32" },
    { __LINE__, FALSE, TRUE, L"test program.exe", L"C:\\Program Files" },
    { __LINE__, FALSE, TRUE, L"test program.exe \"Test File.txt\"", NULL },
    { __LINE__, FALSE, TRUE, L"test program.exe \"Test File.txt\"", L"." },
    // test program.bat
    { __LINE__, FALSE, FALSE, L"test program.bat", NULL },
    { __LINE__, FALSE, FALSE, L"test program.bat", L"." },
    { __LINE__, FALSE, FALSE, L"test program.bat", L"system32" },
    { __LINE__, FALSE, FALSE, L"test program.bat", L"C:\\Program Files" },
    { __LINE__, FALSE, FALSE, L"test program.bat \"Test File.txt\"", NULL },
    { __LINE__, FALSE, FALSE, L"test program.bat \"Test File.txt\"", L"." },
    { __LINE__, FALSE, TRUE, L"test program.bat", NULL },
    { __LINE__, FALSE, TRUE, L"test program.bat", L"." },
    { __LINE__, FALSE, TRUE, L"test program.bat", L"system32" },
    { __LINE__, FALSE, TRUE, L"test program.bat", L"C:\\Program Files" },
    { __LINE__, FALSE, TRUE, L"test program.bat \"Test File.txt\"", NULL },
    { __LINE__, FALSE, TRUE, L"test program.bat \"Test File.txt\"", L"." },
    // "test program"
    { __LINE__, FALSE, FALSE, L"\"test program\"", NULL },
    { __LINE__, FALSE, FALSE, L"\"test program\"", L"." },
    { __LINE__, FALSE, FALSE, L"\"test program\"", L"system32" },
    { __LINE__, FALSE, FALSE, L"\"test program\"", L"C:\\Program Files" },
    { __LINE__, FALSE, FALSE, L"\"test program\" \"Test File.txt\"", NULL },
    { __LINE__, FALSE, FALSE, L"\"test program\" \"Test File.txt\"", L"." },
    { __LINE__, FALSE, TRUE, L"\"test program\"", NULL },
    { __LINE__, FALSE, TRUE, L"\"test program\"", L"." },
    { __LINE__, FALSE, TRUE, L"\"test program\"", L"system32" },
    { __LINE__, FALSE, TRUE, L"\"test program\"", L"C:\\Program Files" },
    { __LINE__, FALSE, TRUE, L"\"test program\" \"Test File.txt\"", NULL },
    { __LINE__, FALSE, TRUE, L"\"test program\" \"Test File.txt\"", L"." },
    // "test program.exe"
    { __LINE__, TRUE, FALSE, L"\"test program.exe\"", NULL },
    { __LINE__, TRUE, FALSE, L"\"test program.exe\"", L"." },
    { __LINE__, TRUE, FALSE, L"\"test program.exe\"", L"system32" },
    { __LINE__, TRUE, FALSE, L"\"test program.exe\"", L"C:\\Program Files" },
    { __LINE__, TRUE, FALSE, L"\"test program.exe\" \"Test File.txt\"", NULL },
    { __LINE__, TRUE, FALSE, L"\"test program.exe\" \"Test File.txt\"", L"." },
    { __LINE__, TRUE, TRUE, L"\"test program.exe\"", NULL },
    { __LINE__, TRUE, TRUE, L"\"test program.exe\"", L"." },
    { __LINE__, TRUE, TRUE, L"\"test program.exe\"", L"system32" },
    { __LINE__, TRUE, TRUE, L"\"test program.exe\"", L"C:\\Program Files" },
    { __LINE__, TRUE, TRUE, L"\"test program.exe\" \"Test File.txt\"", NULL },
    { __LINE__, TRUE, TRUE, L"\"test program.exe\" \"Test File.txt\"", L"." },
    // "test program.bat"
    { __LINE__, FALSE, FALSE, L"\"test program.bat\"", NULL },
    { __LINE__, FALSE, FALSE, L"\"test program.bat\"", L"." },
    { __LINE__, FALSE, FALSE, L"\"test program.bat\"", L"system32" },
    { __LINE__, FALSE, FALSE, L"\"test program.bat\"", L"C:\\Program Files" },
    { __LINE__, FALSE, FALSE, L"\"test program.bat\" \"Test File.txt\"", NULL },
    { __LINE__, FALSE, FALSE, L"\"test program.bat\" \"Test File.txt\"", L"." },
    { __LINE__, FALSE, TRUE, L"\"test program.bat\"", NULL },
    { __LINE__, FALSE, TRUE, L"\"test program.bat\"", L"." },
    { __LINE__, FALSE, TRUE, L"\"test program.bat\"", L"system32" },
    { __LINE__, FALSE, TRUE, L"\"test program.bat\"", L"C:\\Program Files" },
    { __LINE__, FALSE, TRUE, L"\"test program.bat\" \"Test File.txt\"", NULL },
    { __LINE__, FALSE, TRUE, L"\"test program.bat\" \"Test File.txt\"", L"." },
    // invalid program
    { __LINE__, FALSE, FALSE, L"invalid program", NULL },
    { __LINE__, FALSE, FALSE, L"invalid program", L"." },
    { __LINE__, FALSE, FALSE, L"invalid program", L"system32" },
    { __LINE__, FALSE, FALSE, L"invalid program", L"C:\\Program Files" },
    { __LINE__, FALSE, FALSE, L"invalid program \"Test File.txt\"", NULL },
    { __LINE__, FALSE, FALSE, L"invalid program \"Test File.txt\"", L"." },
    { __LINE__, FALSE, TRUE, L"invalid program", NULL },
    { __LINE__, FALSE, TRUE, L"invalid program", L"." },
    { __LINE__, FALSE, TRUE, L"invalid program", L"system32" },
    { __LINE__, FALSE, TRUE, L"invalid program", L"C:\\Program Files" },
    { __LINE__, FALSE, TRUE, L"invalid program \"Test File.txt\"", NULL },
    { __LINE__, FALSE, TRUE, L"invalid program \"Test File.txt\"", L"." },
    // \"invalid program.exe\"
    { __LINE__, FALSE, FALSE, L"\"invalid program.exe\"", NULL },
    { __LINE__, FALSE, FALSE, L"\"invalid program.exe\"", L"." },
    { __LINE__, FALSE, FALSE, L"\"invalid program.exe\"", L"system32" },
    { __LINE__, FALSE, FALSE, L"\"invalid program.exe\"", L"C:\\Program Files" },
    { __LINE__, FALSE, FALSE, L"\"invalid program.exe\" \"Test File.txt\"", NULL },
    { __LINE__, FALSE, FALSE, L"\"invalid program.exe\" \"Test File.txt\"", L"." },
    { __LINE__, FALSE, TRUE, L"\"invalid program.exe\"", NULL },
    { __LINE__, FALSE, TRUE, L"\"invalid program.exe\"", L"." },
    { __LINE__, FALSE, TRUE, L"\"invalid program.exe\"", L"system32" },
    { __LINE__, FALSE, TRUE, L"\"invalid program.exe\"", L"C:\\Program Files" },
    { __LINE__, FALSE, TRUE, L"\"invalid program.exe\" \"Test File.txt\"", NULL },
    { __LINE__, FALSE, TRUE, L"\"invalid program.exe\" \"Test File.txt\"", L"." },
    // My Documents
    { __LINE__, TRUE, TRUE, L"::{450d8fba-ad25-11d0-98a8-0800361b1103}", NULL },
    { __LINE__, TRUE, TRUE, L"shell:::{450d8fba-ad25-11d0-98a8-0800361b1103}", NULL },
    // shell:sendto
    { __LINE__, TRUE, TRUE, L"shell:sendto", NULL },
    // iexplore.exe
    { __LINE__, TRUE, FALSE, L"iexplore", NULL },
    { __LINE__, TRUE, FALSE, L"iexplore.exe", NULL },
    { __LINE__, TRUE, TRUE, L"iexplore", NULL },
    { __LINE__, TRUE, TRUE, L"iexplore.exe", NULL },
    // https://google.com
    { __LINE__, TRUE, FALSE, L"https://google.com", NULL },
    { __LINE__, TRUE, TRUE, L"https://google.com", NULL },
    // Test File 1.txt
    { __LINE__, FALSE, FALSE, L"Test File 1.txt", NULL },
    { __LINE__, FALSE, FALSE, L"Test File 1.txt", L"." },
    { __LINE__, FALSE, FALSE, L"Test File 1.txt", L"system32" },
    { __LINE__, FALSE, FALSE, L"Test File 1.txt", s_cur_dir },
    { __LINE__, FALSE, FALSE, L"\"Test File 1.txt\"", NULL },
    { __LINE__, FALSE, FALSE, L"\"Test File 1.txt\"", L"." },
    { __LINE__, FALSE, FALSE, L"\"Test File 1.txt\"", L"system32" },
    { __LINE__, FALSE, FALSE, L"\"Test File 1.txt\"", s_cur_dir },
    { __LINE__, FALSE, FALSE, L"\"Test File 1.txt\" \"Test File.txt\"", NULL },
    { __LINE__, FALSE, FALSE, L"\"Test File 1.txt\" \"Test File.txt\"", L"." },
    { __LINE__, FALSE, FALSE, L"\"Test File 1.txt\" \"Test File.txt\"", L"system32" },
    { __LINE__, FALSE, TRUE, L"Test File 1.txt", NULL },
    { __LINE__, TRUE, TRUE, L"Test File 1.txt", L"." },
    { __LINE__, FALSE, TRUE, L"Test File 1.txt", L"system32" },
    { __LINE__, TRUE, TRUE, L"Test File 1.txt", s_cur_dir },
    { __LINE__, FALSE, TRUE, L"\"Test File 1.txt\"", NULL },
    { __LINE__, TRUE, TRUE, L"\"Test File 1.txt\"", L"." },
    { __LINE__, TRUE, TRUE, L"\"Test File 1.txt\"", L"system32" },
    { __LINE__, TRUE, TRUE, L"\"Test File 1.txt\"", s_cur_dir },
    { __LINE__, FALSE, TRUE, L"\"Test File 1.txt\" \"Test File.txt\"", NULL },
    { __LINE__, TRUE, TRUE, L"\"Test File 1.txt\" \"Test File.txt\"", L"." },
    { __LINE__, TRUE, TRUE, L"\"Test File 1.txt\" \"Test File.txt\"", L"system32" },
    { __LINE__, TRUE, TRUE, L"\"Test File 1.txt\" \"Test File.txt\"", s_cur_dir },
    // Test File 2.bat
    { __LINE__, FALSE, FALSE, L"Test File 2.bat", NULL },
    { __LINE__, FALSE, FALSE, L"Test File 2.bat", L"." },
    { __LINE__, FALSE, FALSE, L"Test File 2.bat", L"system32" },
    { __LINE__, FALSE, FALSE, L"Test File 2.bat", s_cur_dir },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\"", NULL },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\"", L"." },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\"", L"system32" },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\"", s_cur_dir },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\" \"Test File.txt\"", NULL },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\" \"Test File.txt\"", L"." },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\" \"Test File.txt\"", L"system32" },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\" \"Test File.txt\"", s_cur_dir },
    { __LINE__, FALSE, TRUE, L"Test File 2.bat", NULL },
    { __LINE__, FALSE, TRUE, L"Test File 2.bat", L"." },
    { __LINE__, FALSE, TRUE, L"Test File 2.bat", L"system32" },
    { __LINE__, FALSE, TRUE, L"Test File 2.bat", s_cur_dir },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\"", NULL },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\"", L"." },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\"", L"system32" },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\"", s_cur_dir },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\" \"Test File.txt\"", NULL },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\" \"Test File.txt\"", L"." },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\" \"Test File.txt\"", L"system32" },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\" \"Test File.txt\"", s_cur_dir },
};

static const TEST_ENTRY s_entries_2[] =
{
    // Test File 1.txt (with setting path)
    { __LINE__, FALSE, FALSE, L"Test File 1.txt", NULL },
    { __LINE__, FALSE, FALSE, L"Test File 1.txt", L"." },
    { __LINE__, FALSE, FALSE, L"Test File 1.txt", L"system32" },
    { __LINE__, FALSE, FALSE, L"Test File 1.txt", s_cur_dir },
    { __LINE__, FALSE, FALSE, L"\"Test File 1.txt\"", NULL },
    { __LINE__, FALSE, FALSE, L"\"Test File 1.txt\"", L"." },
    { __LINE__, FALSE, FALSE, L"\"Test File 1.txt\"", L"system32" },
    { __LINE__, FALSE, FALSE, L"\"Test File 1.txt\"", s_cur_dir },
    { __LINE__, FALSE, FALSE, L"\"Test File 1.txt\" \"Test File.txt\"", NULL },
    { __LINE__, FALSE, FALSE, L"\"Test File 1.txt\" \"Test File.txt\"", L"." },
    { __LINE__, FALSE, FALSE, L"\"Test File 1.txt\" \"Test File.txt\"", L"system32" },
    { __LINE__, FALSE, FALSE, L"\"Test File 1.txt\" \"Test File.txt\"", s_cur_dir },
    { __LINE__, FALSE, TRUE, L"Test File 1.txt", NULL },
    { __LINE__, TRUE, TRUE, L"Test File 1.txt", L"." },
    { __LINE__, FALSE, TRUE, L"Test File 1.txt", L"system32" },
    { __LINE__, TRUE, TRUE, L"Test File 1.txt", s_cur_dir },
    { __LINE__, TRUE, TRUE, L"\"Test File 1.txt\"", NULL },
    { __LINE__, TRUE, TRUE, L"\"Test File 1.txt\"", L"." },
    { __LINE__, TRUE, TRUE, L"\"Test File 1.txt\"", L"system32" },
    { __LINE__, TRUE, TRUE, L"\"Test File 1.txt\"", s_cur_dir },
    { __LINE__, TRUE, TRUE, L"\"Test File 1.txt\" \"Test File.txt\"", NULL },
    { __LINE__, TRUE, TRUE, L"\"Test File 1.txt\" \"Test File.txt\"", L"." },
    { __LINE__, TRUE, TRUE, L"\"Test File 1.txt\" \"Test File.txt\"", L"system32" },
    { __LINE__, TRUE, TRUE, L"\"Test File 1.txt\" \"Test File.txt\"", s_cur_dir },
    // Test File 2.bat (with setting path)
    { __LINE__, FALSE, FALSE, L"Test File 2.bat", NULL },
    { __LINE__, FALSE, FALSE, L"Test File 2.bat", L"." },
    { __LINE__, FALSE, FALSE, L"Test File 2.bat", L"system32" },
    { __LINE__, FALSE, FALSE, L"Test File 2.bat", s_cur_dir },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\"", NULL },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\"", L"." },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\"", L"system32" },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\"", s_cur_dir },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\" \"Test File.txt\"", NULL },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\" \"Test File.txt\"", L"." },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\" \"Test File.txt\"", L"system32" },
    { __LINE__, FALSE, FALSE, L"\"Test File 2.bat\" \"Test File.txt\"", s_cur_dir },
    { __LINE__, FALSE, TRUE, L"Test File 2.bat", NULL },
    { __LINE__, FALSE, TRUE, L"Test File 2.bat", L"." },
    { __LINE__, FALSE, TRUE, L"Test File 2.bat", L"system32" },
    { __LINE__, FALSE, TRUE, L"Test File 2.bat", s_cur_dir },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\"", NULL },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\"", L"." },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\"", L"system32" },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\"", s_cur_dir },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\" \"Test File.txt\"", NULL },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\" \"Test File.txt\"", L"." },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\" \"Test File.txt\"", L"system32" },
    { __LINE__, FALSE, TRUE, L"\"Test File 2.bat\" \"Test File.txt\"", s_cur_dir },
};

typedef struct OPENWNDS
{
    UINT count;
    HWND *phwnd;
} OPENWNDS;

static OPENWNDS s_wi0 = { 0 }, s_wi1 = { 0 };

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    OPENWNDS *info = (OPENWNDS *)lParam;
    info->phwnd = (HWND *)realloc(info->phwnd, (info->count + 1) * sizeof(HWND));
    if (!info->phwnd)
        return FALSE;
    info->phwnd[info->count] = hwnd;
    ++(info->count);
    return TRUE;
}

static void CleanupNewlyCreatedWindows(void)
{
    EnumWindows(EnumWindowsProc, (LPARAM)&s_wi1);
    for (UINT i1 = 0; i1 < s_wi1.count; ++i1)
    {
        BOOL bFound = FALSE;
        for (UINT i0 = 0; i0 < s_wi0.count; ++i0)
        {
            if (s_wi1.phwnd[i1] == s_wi0.phwnd[i0])
            {
                bFound = TRUE;
                break;
            }
        }
        if (!bFound)
            PostMessageW(s_wi1.phwnd[i1], WM_CLOSE, 0, 0);
    }
    free(s_wi1.phwnd);
    ZeroMemory(&s_wi1, sizeof(s_wi1));
}

static void DoEntry(const TEST_ENTRY *pEntry)
{
    HRESULT hr;
    DWORD dwSeclFlags;
    BOOL result;

    if (pEntry->bAllowNonExe)
        dwSeclFlags = SECL_NO_UI | SECL_ALLOW_NONEXE;
    else
        dwSeclFlags = SECL_NO_UI;

    _SEH2_TRY
    {
        if (IsReactOS())
        {
            hr = proxy_ShellExecCmdLine(NULL, pEntry->pwszCommand, pEntry->pwszStartDir,
                                        SW_SHOWNORMAL, NULL, dwSeclFlags);
        }
        else
        {
            hr = (*g_pShellExecCmdLine)(NULL, pEntry->pwszCommand, pEntry->pwszStartDir,
                                        SW_SHOWNORMAL, NULL, dwSeclFlags);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = 0xBADFACE;
    }
    _SEH2_END;

    if (hr == 0xBADFACE)
        result = hr;
    else
        result = (hr == S_OK);

    ok(result == pEntry->result, "Line %d: result expected %d, was %d\n",
       pEntry->lineno, pEntry->result, result);

    CleanupNewlyCreatedWindows();
}

START_TEST(ShellExecCmdLine)
{
    using namespace std;

    if (!IsReactOS())
    {
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
    }

    if (!GetSubProgramPath())
    {
        skip("shell32_apitest_sub.exe is not found\n");
        return;
    }

    // record open windows
    if (!EnumWindows(EnumWindowsProc, (LPARAM)&s_wi0))
    {
        skip("EnumWindows failed\n");
        free(s_wi0.phwnd);
        return;
    }

    // s_win_test_exe
    GetWindowsDirectoryW(s_win_test_exe, _countof(s_win_test_exe));
    PathAppendW(s_win_test_exe, L"test program.exe");
    BOOL ret = CopyFileW(s_sub_program, s_win_test_exe, FALSE);
    if (!ret)
    {
        skip("Please retry with admin rights\n");
        free(s_wi0.phwnd);
        return;
    }

    FILE *fp;

    // s_sys_bat_file
    GetSystemDirectoryW(s_sys_bat_file, _countof(s_sys_bat_file));
    PathAppendW(s_sys_bat_file, L"test program.bat");
    fp = _wfopen(s_sys_bat_file, L"wb");
    fclose(fp);
    ok_int(PathFileExistsW(s_sys_bat_file), TRUE);

    // "Test File 1.txt"
    fp = fopen("Test File 1.txt", "wb");
    ok(fp != NULL, "failed to create a test file\n");
    fclose(fp);
    ok_int(PathFileExistsA("Test File 1.txt"), TRUE);

    // "Test File 2.bat"
    fp = fopen("Test File 2.bat", "wb");
    ok(fp != NULL, "failed to create a test file\n");
    fclose(fp);
    ok_int(PathFileExistsA("Test File 2.bat"), TRUE);

    // s_cur_dir
    GetCurrentDirectoryW(_countof(s_cur_dir), s_cur_dir);

    // do tests
    for (size_t i = 0; i < _countof(s_entries_1); ++i)
    {
        DoEntry(&s_entries_1[i]);
    }
    SetEnvironmentVariableW(L"PATH", s_cur_dir);
    for (size_t i = 0; i < _countof(s_entries_2); ++i)
    {
        DoEntry(&s_entries_2[i]);
    }

    Sleep(2000);
    CleanupNewlyCreatedWindows();

    // clean up
    ok(DeleteFileW(s_win_test_exe), "failed to delete the test file\n");
    ok(DeleteFileW(s_sys_bat_file), "failed to delete the test file\n");
    ok(DeleteFileA("Test File 1.txt"), "failed to delete the test file\n");
    ok(DeleteFileA("Test File 2.bat"), "failed to delete the test file\n");
    free(s_wi0.phwnd);

    DoWaitForWindow(CLASSNAME, CLASSNAME, TRUE, TRUE);
    Sleep(100);
}
