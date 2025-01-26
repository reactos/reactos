/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for GetThemeParseErrorInfo
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <uxtheme.h>
#include <uxundoc.h>

typedef HRESULT (WINAPI *FN_GetThemeParseErrorInfo)(PPARSE_ERROR_INFO);
typedef HRESULT (WINAPI *FN_ParseThemeIniFile)(LPCWSTR, LPWSTR, PARSETHEMEINIFILEPROC, LPVOID);

static BOOL CALLBACK
ParseThemeIniFileProc(
    DWORD dwType,
    LPWSTR pszParam1,
    LPWSTR pszParam2,
    LPWSTR pszParam3,
    DWORD dwParam,
    LPVOID lpData)
{
    return TRUE;
}

START_TEST(GetThemeParseErrorInfo)
{
    HRESULT hr;
    PARSE_ERROR_INFO Info;
    WCHAR szPath[MAX_PATH];

    FN_GetThemeParseErrorInfo GetThemeParseErrorInfo =
        (FN_GetThemeParseErrorInfo)GetProcAddress(GetModuleHandleW(L"uxtheme.dll"), MAKEINTRESOURCEA(48));
    if (!GetThemeParseErrorInfo)
    {
        skip("No GetThemeParseErrorInfo found\n");
        return;
    }

    hr = GetThemeParseErrorInfo(NULL);
    ok_hex(hr, E_POINTER);

    ZeroMemory(&Info, sizeof(Info));
    hr = GetThemeParseErrorInfo(&Info);
    ok_hex(hr, E_INVALIDARG);

    ZeroMemory(&Info, sizeof(Info));
    Info.cbSize = sizeof(Info);
    hr = GetThemeParseErrorInfo(&Info);
    ok_hex(hr, HRESULT_FROM_WIN32(ERROR_RESOURCE_NAME_NOT_FOUND));

    FN_ParseThemeIniFile ParseThemeIniFile =
        (FN_ParseThemeIniFile)GetProcAddress(GetModuleHandleW(L"uxtheme.dll"), MAKEINTRESOURCEA(11));
    if (!ParseThemeIniFile)
    {
        skip("No ParseThemeIniFile found\n");
        return;
    }

    FILE *fout = _wfopen(L"invalid.ini", L"wb");
    fprintf(fout, "[Invalid]\n");
    fprintf(fout, "Invalid=Invalid\n");
    fclose(fout);

    hr = ParseThemeIniFile(L"invalid.ini", szPath, ParseThemeIniFileProc, NULL);
    ok_hex(hr, HRESULT_FROM_WIN32(ERROR_UNKNOWN_PROPERTY));

    DeleteFileW(L"invalid.ini");

    ZeroMemory(&Info, sizeof(Info));
    Info.cbSize = sizeof(Info);
    Info.ErrInfo.szPath0[0] = L'@';
    Info.ErrInfo.szPath1[0] = L'@';
    Info.ErrInfo.szPath2[0] = L'@';
    Info.ErrInfo.szPath3[0] = L'@';
    hr = GetThemeParseErrorInfo(&Info);
    ok_hex(hr, S_OK);
    ok_int(Info.ErrInfo.nID, 160);

    ok(Info.ErrInfo.szPath0[0] != L'@', "Info.ErrInfo.szPath0 was empty\n");
    trace("Info.ErrInfo.szPath0: %S\n", Info.ErrInfo.szPath0); // "Must be Primitive, enum, or type: Invalid"

    ok_int(Info.ErrInfo.szPath1[0], L'@');
    ok_wstr(Info.ErrInfo.szPath2, L"invalid.ini");
    ok_wstr(Info.ErrInfo.szPath3, L"Invalid=Invalid");
    ok_int(Info.ErrInfo.nLineNo, 2);
}
