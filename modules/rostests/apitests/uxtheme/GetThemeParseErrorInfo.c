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
    fprintf(fout, "[InvalidKey]\n");
    fprintf(fout, "InvalidValueName=InvalidValue\n");
    fclose(fout);

    hr = ParseThemeIniFile(L"invalid.ini", szPath, ParseThemeIniFileProc, NULL);
    ok_hex(hr, HRESULT_FROM_WIN32(ERROR_UNKNOWN_PROPERTY));

    _wremove(L"invalid.ini");

    ZeroMemory(&Info, sizeof(Info));
    Info.cbSize = sizeof(Info);
    Info.szDescription[0] = L'@';
    Info.szDescription[MAX_PATH] = L'@';
    Info.szFile[0] = L'@';
    Info.szLine[0] = L'@';
    hr = GetThemeParseErrorInfo(&Info);
    ok_hex(hr, S_OK);
    ok_int(Info.nID, 160);

    ok(Info.szDescription[0] != UNICODE_NULL, "Info.szDescription was empty\n");
    ok(Info.szDescription[0] != L'@', "Info.szDescription had no change\n");
    trace("szDescription: %s\n", wine_dbgstr_w(Info.szDescription)); // "Must be Primitive, enum, or type: InvalidValueName"

    ok_int(Info.szDescription[MAX_PATH], L'@');
    ok_wstr(Info.szFile, L"invalid.ini");
    ok_wstr(Info.szLine, L"InvalidValueName=InvalidValue");
    ok_int(Info.nLineNo, 2);
}
