/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     SHEvaluateSystemCommandTemplate
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlguid_undoc.h>
#include <shlobj_undoc.h>
#include <shlwapi_undoc.h>
#include <strsafe.h>

static PCWSTR _PathGetArgsLikeCreateProcess(PCWSTR lpString)
{
    PCWSTR pch;
    if (*lpString == L'"')
    {
        pch = StrChrW(lpString + 1, L'"');
        if (pch)
        {
            ++pch;
            if (*pch == L' ')
                ++pch;
            return pch;
        }
    }
    else
    {
        pch = StrChrW(lpString, L' ');
        if (pch)
            return pch + 1;
    }
    return &lpString[lstrlenW(lpString)];
}

static HRESULT _PathCopyExeAndTrim(PWSTR pszBuff, size_t cchBuff, PCWSTR pszSrc, size_t cchSrc)
{
    HRESULT hr = StringCchCopyNW(pszBuff, cchBuff, pszSrc, cchSrc);
    if (SUCCEEDED(hr))
        StrTrimW(pszBuff, L" \t");
    return hr;
}

static BOOL _PathMatchesSuspicious(PCWSTR lpString)
{
    WCHAR pszPath[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, 0, pszPath);
    INT cch = lstrlenW(pszPath);
    return StrCmpNIW(lpString, pszPath, cch) == 0;
}

// This function attempts to find where the "arguments" portion of a command-line path string
static PCWSTR _PathGuessNextBestArgs(PCWSTR pszPath)
{
    PCWSTR pSpaceStart = NULL;
    BOOL bValid = TRUE;
    const DWORD PATH_VALID_CHARS = (
        PATH_CHAR_CLASS_DOT | PATH_CHAR_CLASS_SEMICOLON | PATH_CHAR_CLASS_COMMA |
        PATH_CHAR_CLASS_SPACE | PATH_CHAR_CLASS_OTHER_VALID);

    for (;;)
    {
        const WCHAR ch = *pszPath;
        if (!ch)
            break;

        switch (ch)
        {
            case L' ':
                if (!pSpaceStart)
                    pSpaceStart = pszPath;
                break;

            case L'"':
            case L'%':
                bValid = FALSE;
                break;

            case L'\\':
                bValid = !PathIsUNCW(pszPath);
                if (bValid)
                    pSpaceStart = NULL;
                break;

            default:
                bValid = PathIsValidCharW(ch, PATH_VALID_CHARS);
                break;
        }

        if (!bValid)
            break;

        ++pszPath;
    }

    if (pSpaceStart)
    {
        while (*pSpaceStart == L' ')
            ++pSpaceStart;
        return pSpaceStart;
    }

    return bValid ? pszPath : NULL;
}

static inline BOOL _PathAppend(PCWSTR key1, PCWSTR key2, PWSTR pszDest, size_t cchDest)
{
    return SUCCEEDED(StringCchCopyW(pszDest, cchDest, key1)) &&
           SUCCEEDED(StringCchCatW(pszDest, cchDest, L"\\")) &&
           SUCCEEDED(StringCchCatW(pszDest, cchDest, key2));
}

static VOID _MakeAppPathKey(PCWSTR pszPath, PWSTR pszDest, UINT cchDest)
{
    if (_PathAppend(L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths",
                    pszPath, pszDest, cchDest))
    {
        if (!*PathFindExtensionW(pszPath))
            StringCchCatW(pszDest, cchDest, L".exe");
    }
}

static BOOL _GetAppPath(PCWSTR pszPath, PWSTR pszValue, DWORD cchValue)
{
    WCHAR szSubKey[MAX_PATH];
    _MakeAppPathKey(pszPath, szSubKey, _countof(szSubKey));
    DWORD cbData = cchValue * sizeof(WCHAR);
    LSTATUS error = SHGetValueW(HKEY_LOCAL_MACHINE, szSubKey, NULL, NULL, pszValue, &cbData);
    return error == ERROR_SUCCESS;
}

static HRESULT _PathExeExists(_In_ PCWSTR pszPath)
{
    WCHAR szPath[MAX_PATH];
    StringCchCopyW(szPath, _countof(szPath), pszPath);

    DWORD dwWhich = WHICH_PIF | WHICH_COM | WHICH_EXE | WHICH_BAT | WHICH_CMD | WHICH_OPTIONAL;
    DWORD attrs;
    if (!PathFileExistsDefExtAndAttributesW(szPath, dwWhich, &attrs) ||
        (attrs & FILE_ATTRIBUTE_DIRECTORY))
    {
        return CO_E_APPNOTFOUND;
    }
    return S_OK;
}

static HRESULT
_PathFindInFolder(_In_ INT csidl, _In_ PCWSTR pszSrc, _Out_ PWSTR pszPath, _In_ UINT cchPath)
{
    WCHAR szDir[MAX_PATH];
    HRESULT hr = SHGetFolderPathW(0, csidl, 0, 0, szDir);
    if (FAILED(hr))
        return hr;

    StringCchCopyW(pszPath, cchPath, szDir);
    StringCchCatW(pszPath, cchPath, L"\\");
    hr = StringCchCatW(pszPath, cchPath, pszSrc);
    if (FAILED(hr))
        return hr;

    return _PathExeExists(pszPath);
}

static HRESULT _PathFindInSystem(_Inout_ PWSTR pszPath, _In_ UINT cchPath)
{
    WCHAR szPath[MAX_PATH];
    HRESULT hr = _PathFindInFolder(CSIDL_SYSTEM, pszPath, szPath, _countof(szPath));
    if (FAILED(hr))
        hr = _PathFindInFolder(CSIDL_WINDOWS, pszPath, szPath, _countof(szPath));
    if (FAILED(hr))
        return hr;
    return StringCchCopyW(pszPath, cchPath, szPath);
}

/*************************************************************************
 * SHEvaluateSystemCommandTemplate [SHELL32.482] (Vista+)
 * SHEvaluateSystemCommandTemplate [SHLWAPI.552] (XP SP1 and SP2)
 *
 * https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shevaluatesystemcommandtemplate
 */
EXTERN_C
HRESULT WINAPI
SHEvaluateSystemCommandTemplate(
    _In_ PCWSTR pszCmdTemplate,
    _Outptr_ PWSTR *ppszApplication,
    _Outptr_opt_ PWSTR *ppszCommandLine,
    _Outptr_opt_ PWSTR *ppszParameters)
{
    HRESULT hr;
    WCHAR szExe[MAX_PATH], szProgram[MAX_PATH];
    PCWSTR pszArgs = _PathGetArgsLikeCreateProcess(pszCmdTemplate);
    BOOL bQuoted;

    UINT cchArgs = (UINT)(pszArgs - pszCmdTemplate);
    hr = _PathCopyExeAndTrim(szExe, _countof(szExe), pszCmdTemplate, cchArgs);
    if (FAILED(hr))
        goto Exit;

    // Unquote if necessary
    bQuoted = (szExe[0] == L'"');
    if (bQuoted)
        PathUnquoteSpacesW(szExe);

    StringCchCopyW(szProgram, _countof(szProgram), szExe);

    if (PathIsAbsolute(szExe))
    {
        if (bQuoted)
        {
            hr = _PathExeExists(szExe);
        }
        else // Not quoted
        {
            if (_PathMatchesSuspicious(szExe)) // ProgramFiles-likely?
                hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
            else
                hr = _PathExeExists(szExe);
        }

        // Detect the best arguments position
        while (FAILED(hr))
        {
            if (bQuoted || !*pszArgs)
                break;

            pszArgs = _PathGuessNextBestArgs(pszArgs);
            if (!pszArgs)
                break;

            cchArgs = (UINT)(pszArgs - pszCmdTemplate);
            hr = _PathCopyExeAndTrim(szExe, _countof(szExe), pszCmdTemplate, cchArgs);
            if (FAILED(hr))
                break;

            hr = _PathExeExists(szExe);
        }
    }
    else
    {
        if (!PathIsFileSpecW(szExe))
        {
            hr = E_ACCESSDENIED;
            goto Exit;
        }

        if (_GetAppPath(szExe, szExe, _countof(szExe)))
        {
            StringCchCopyW(szProgram, _countof(szProgram), PathFindFileNameW(szExe));
            hr = S_OK;
        }
        else if (SHWindowsPolicyEx(POLID_UsePathEnvVarForCommandTemplates, FALSE))
        {
            const DWORD PATH_VALID_CHARS = (
                PATH_CHAR_CLASS_DOT | PATH_CHAR_CLASS_SEMICOLON | PATH_CHAR_CLASS_COMMA |
                PATH_CHAR_CLASS_SPACE | PATH_CHAR_CLASS_OTHER_VALID);
            hr = PathFindOnPathExW(szExe, NULL, PATH_VALID_CHARS) ? S_OK : CO_E_APPNOTFOUND;
        }
        else
        {
            hr = _PathFindInSystem(szExe, _countof(szExe));
        }
    }

Exit:
    *ppszApplication = NULL;
    if (ppszCommandLine)
        *ppszCommandLine = NULL;
    if (ppszParameters)
        *ppszParameters = NULL;

    if (!pszArgs)
        pszArgs = L"";

    // Create output strings
    if (SUCCEEDED(hr))
        hr = SHStrDupW(szExe, ppszApplication);

    if (SUCCEEDED(hr) && ppszCommandLine)
    {
        size_t cch = lstrlenW(szProgram) + lstrlenW(pszArgs) + 4; // 4 for '"', '"', ' ', NUL
        hr = SHCoAlloc(cch * sizeof(WCHAR), (PVOID*)ppszCommandLine);
        if (SUCCEEDED(hr))
            hr = StringCchPrintfW(*ppszCommandLine, cch, L"\"%s\" %s", szProgram, pszArgs);
    }

    if (SUCCEEDED(hr) && ppszParameters)
        hr = SHStrDupW(pszArgs, ppszParameters);

    if (FAILED(hr))
    {
        // Clean up
        if (*ppszApplication)
        {
            CoTaskMemFree(*ppszApplication);
            *ppszApplication = NULL;
        }
        if (ppszCommandLine && *ppszCommandLine)
        {
            CoTaskMemFree(*ppszCommandLine);
            *ppszCommandLine = NULL;
        }
        if (ppszParameters && *ppszParameters)
        {
            CoTaskMemFree(*ppszParameters);
            *ppszParameters = NULL;
        }
    }

    return hr;
}
