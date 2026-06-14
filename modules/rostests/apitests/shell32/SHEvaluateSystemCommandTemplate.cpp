/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for SHEvaluateSystemCommandTemplate
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shelltest.h"
#include <strsafe.h>
#include <versionhelpers.h>

typedef HRESULT (WINAPI *FN_SHEvaluateSystemCommandTemplate)(PCWSTR, PWSTR*, PWSTR*, PWSTR*);
static FN_SHEvaluateSystemCommandTemplate g_fnSHEvaluateSystemCommandTemplate = NULL;

#define ok_wstri(x, y) \
    ok(lstrcmpiW(x, y) == 0, "Wrong string. Expected %s, got %s\n", wine_dbgstr_w(y), wine_dbgstr_w(x))

static BOOL TEST_Init(void)
{
    HINSTANCE hShell32 = GetModuleHandleA("shell32");
    g_fnSHEvaluateSystemCommandTemplate = (FN_SHEvaluateSystemCommandTemplate)
        GetProcAddress(hShell32, "SHEvaluateSystemCommandTemplate");
    if (g_fnSHEvaluateSystemCommandTemplate)
        return TRUE;

    HINSTANCE hSHLWAPI = GetModuleHandleA("shlwapi");
    g_fnSHEvaluateSystemCommandTemplate = (FN_SHEvaluateSystemCommandTemplate)
        GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(552));
    if (g_fnSHEvaluateSystemCommandTemplate)
    {
        trace("shlwapi has SHEvaluateSystemCommandTemplate\n");
        return TRUE;
    }

    skip("SHEvaluateSystemCommandTemplate not found\n");
    return FALSE;
}

static void TEST_notepad(void)
{
    PWSTR app, cmdline, params;
    WCHAR szPath[MAX_PATH], szQuoted[MAX_PATH], szNotepad[MAX_PATH];
    WCHAR szAnswer1[MAX_PATH], szAnswer2[MAX_PATH], szAnswer3[MAX_PATH];
    HRESULT hr;

    GetSystemDirectoryW(szNotepad, _countof(szNotepad));
    StringCchCatW(szNotepad, _countof(szNotepad), L"\\notepad.exe");
    StringCchPrintfW(szAnswer1, _countof(szAnswer1), L"\"%s\"", szNotepad);
    StringCchPrintfW(szAnswer2, _countof(szAnswer2), L"\"%s\" ", szNotepad);
    StringCchPrintfW(szAnswer3, _countof(szAnswer3), L"\"%s\" /A /P", szNotepad);

    hr = g_fnSHEvaluateSystemCommandTemplate(L"notepad.exe", &app, NULL, NULL);
    ok_hr(hr, S_OK);
    ok_wstri(app, szNotepad);
    CoTaskMemFree(app);

    hr = g_fnSHEvaluateSystemCommandTemplate(L"notepad.exe", &app, &cmdline, NULL);
    ok_hr(hr, S_OK);
    ok_wstri(app, szNotepad);
    ok_wstri(cmdline, L"\"notepad.exe\" ");
    CoTaskMemFree(app);
    CoTaskMemFree(cmdline);

    hr = g_fnSHEvaluateSystemCommandTemplate(L"notepad.exe", &app, &cmdline, &params);
    ok_hr(hr, S_OK);
    ok_wstri(app, szNotepad);
    ok_wstri(cmdline, L"\"notepad.exe\" ");
    ok_wstri(params, L"");
    CoTaskMemFree(app);
    CoTaskMemFree(cmdline);
    CoTaskMemFree(params);

    hr = g_fnSHEvaluateSystemCommandTemplate(L"notepad.exe", &app, NULL, &params);
    ok_hr(hr, S_OK);
    ok_wstri(app, szNotepad);
    ok_wstri(params, L"");
    CoTaskMemFree(app);
    CoTaskMemFree(params);

    hr = g_fnSHEvaluateSystemCommandTemplate(L"system32\\notepad.exe", &app, &cmdline, &params);
    ok_hr(hr, E_ACCESSDENIED);
    ok(app == NULL, "app was %s\n", wine_dbgstr_w(app));
    ok(cmdline == NULL, "cmdline was %s\n", wine_dbgstr_w(cmdline));
    ok(params == NULL, "params was %s\n", wine_dbgstr_w(params));
    CoTaskMemFree(app);
    CoTaskMemFree(cmdline);
    CoTaskMemFree(params);

    hr = g_fnSHEvaluateSystemCommandTemplate(L"notepad.exe /A /P", &app, &cmdline, &params);
    ok_hr(hr, S_OK);
    ok_wstri(app, szNotepad);
    ok_wstri(cmdline, L"\"notepad.exe\" /A /P");
    ok_wstri(params, L"/A /P");
    CoTaskMemFree(app);
    CoTaskMemFree(cmdline);
    CoTaskMemFree(params);

    StringCchCopyW(szPath, _countof(szPath), szNotepad);
    hr = g_fnSHEvaluateSystemCommandTemplate(szPath, &app, NULL, NULL);
    ok_hr(hr, S_OK);
    ok_wstri(app, szNotepad);
    CoTaskMemFree(app);

    StringCchCopyW(szPath, _countof(szPath), szNotepad);
    hr = g_fnSHEvaluateSystemCommandTemplate(szPath, &app, NULL, &params);
    ok_hr(hr, S_OK);
    ok_wstri(app, szNotepad);
    ok_wstri(params, L"");
    CoTaskMemFree(app);
    CoTaskMemFree(params);

    StringCchCopyW(szPath, _countof(szPath), szNotepad);
    hr = g_fnSHEvaluateSystemCommandTemplate(szPath, &app, &cmdline, &params);
    ok_hr(hr, S_OK);
    ok_wstri(app, szNotepad);
    ok_wstri(cmdline, szAnswer2);
    ok_wstri(params, L"");
    CoTaskMemFree(app);
    CoTaskMemFree(cmdline);
    CoTaskMemFree(params);

    StringCchPrintfW(szQuoted, _countof(szQuoted), L"\"%s\"", szNotepad);
    hr = g_fnSHEvaluateSystemCommandTemplate(szQuoted, &app, &cmdline, &params);
    ok_hr(hr, S_OK);
    ok_wstri(app, szNotepad);
    ok_wstri(cmdline, szAnswer2);
    ok_wstri(params, L"");
    CoTaskMemFree(app);
    CoTaskMemFree(cmdline);
    CoTaskMemFree(params);

    StringCchPrintfW(szQuoted, _countof(szQuoted), L"\"%s\" /A /P", szNotepad);
    hr = g_fnSHEvaluateSystemCommandTemplate(szQuoted, &app, &cmdline, &params);
    ok_hr(hr, S_OK);
    ok_wstri(app, szNotepad);
    ok_wstri(cmdline, szAnswer3);
    ok_wstri(params, L"/A /P");
    CoTaskMemFree(app);
    CoTaskMemFree(cmdline);
    CoTaskMemFree(params);

    hr = g_fnSHEvaluateSystemCommandTemplate(L"_invalid_\\_path_", &app, &cmdline, &params);
    ok_hr(hr, E_ACCESSDENIED);
    ok(app == NULL, "app was %s\n", wine_dbgstr_w(app));
    ok(cmdline == NULL, "cmdline was %s\n", wine_dbgstr_w(cmdline));
    ok(params == NULL, "params was %s\n", wine_dbgstr_w(params));
    CoTaskMemFree(app);
    CoTaskMemFree(cmdline);
    CoTaskMemFree(params);
}

START_TEST(SHEvaluateSystemCommandTemplate)
{
    if (!TEST_Init())
        return;

    TEST_notepad();
}
