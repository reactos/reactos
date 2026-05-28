/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for AssocQueryStringA/W
 * COPYRIGHT:   Copyright 2024-2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <apitest.h>
#include <shlwapi.h>
#include <stdio.h>
#include <versionhelpers.h>

static CHAR  s_szTextFileA[MAX_PATH] =  "";
static WCHAR s_szTextFileW[MAX_PATH] = L"";
#define NON_EXISTENT_FILENAME_A  "C:\\ThisIsNotExistentFile.txt"
#define NON_EXISTENT_FILENAME_W L"C:\\ThisIsNotExistentFile.txt"

static void TEST_Start(void)
{
    ExpandEnvironmentStringsA( "%APPDATA%\\This is a test.txt", s_szTextFileA, _countof(s_szTextFileA));
    ExpandEnvironmentStringsW(L"%APPDATA%\\This is a test.txt", s_szTextFileW, _countof(s_szTextFileW));
    fclose(_wfopen(s_szTextFileW, L"w"));
    trace("%s\n", wine_dbgstr_a(s_szTextFileA));
    trace("%s\n", wine_dbgstr_w(s_szTextFileW));
}

static void TEST_End(void)
{
    DeleteFileW(s_szTextFileW);
}

/*
 * "shlwapi_winetest assoc" already has many tests.
 * We just do additional tests in here.
 */

static void TEST_AssocQueryStringA(void)
{
    CHAR szPath[MAX_PATH], szAnswer[MAX_PATH];
    CHAR szDebug1[MAX_PATH], szDebug2[MAX_PATH];
    HRESULT hr;
    DWORD cch;

    /* ".txt" */
    lstrcpynA(szPath, ".txt", _countof(szPath));
    cch = _countof(szPath);
    hr = AssocQueryStringA(0, ASSOCSTR_EXECUTABLE, szPath, NULL, szPath, &cch);
    if (IsWindowsVistaOrGreater())
    {
        ExpandEnvironmentStringsA("%WINDIR%\\system32\\notepad.exe", szAnswer, _countof(szAnswer));
        ok_long(hr, S_OK);
        ok_int(cch, lstrlenA(szAnswer) + 1);
    }
    else
    {
        lstrcpynA(szAnswer, ".txt", _countof(szAnswer));
        ok_long(hr, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
        ok_int(cch, _countof(szPath));
    }
    lstrcpynA(szDebug1, wine_dbgstr_a(szPath), _countof(szDebug1));
    lstrcpynA(szDebug2, wine_dbgstr_a(szAnswer), _countof(szDebug2));
    ok(lstrcmpiA(szPath, szAnswer) == 0, "%s vs %s\n", szDebug1, szDebug2);

    /* s_szTextFileA */
    lstrcpynA(szPath, s_szTextFileA, _countof(szPath));
    cch = _countof(szPath);
    hr = AssocQueryStringA(0, ASSOCSTR_EXECUTABLE, szPath, NULL, szPath, &cch);
    if (IsWindowsVistaOrGreater())
    {
        ExpandEnvironmentStringsA("%WINDIR%\\system32\\notepad.exe", szAnswer, _countof(szAnswer));
        ok_long(hr, S_OK);
        ok_int(cch, lstrlenA(szAnswer) + 1);
    }
    else
    {
        lstrcpynA(szAnswer, s_szTextFileA, _countof(szAnswer));
        ok_long(hr, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
        ok_int(cch, _countof(szPath));
    }
    lstrcpynA(szDebug1, wine_dbgstr_a(szPath), _countof(szDebug1));
    lstrcpynA(szDebug2, wine_dbgstr_a(szAnswer), _countof(szDebug2));
    ok(lstrcmpiA(szPath, szAnswer) == 0, "%s vs %s\n", szDebug1, szDebug2);

    /* NON_EXISTENT_FILENAME_A */
    lstrcpynA(szPath, NON_EXISTENT_FILENAME_A, _countof(szPath));
    cch = _countof(szPath);
    hr = AssocQueryStringA(0, ASSOCSTR_EXECUTABLE, szPath, NULL, szPath, &cch);
    if (IsWindowsVistaOrGreater())
    {
        ExpandEnvironmentStringsA("%WINDIR%\\system32\\notepad.exe", szAnswer, _countof(szAnswer));
        ok_long(hr, S_OK);
        ok_int(cch, lstrlenA(szAnswer) + 1);
    }
    else
    {
        lstrcpynA(szAnswer, NON_EXISTENT_FILENAME_A, _countof(szAnswer));
        ok_long(hr, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
        ok_int(cch, _countof(szPath));
    }
    lstrcpynA(szDebug1, wine_dbgstr_a(szPath), _countof(szDebug1));
    lstrcpynA(szDebug2, wine_dbgstr_a(szAnswer), _countof(szDebug2));
    ok(lstrcmpiA(szPath, szAnswer) == 0, "%s vs %s\n", szDebug1, szDebug2);
}

static void TEST_AssocQueryStringW(void)
{
    WCHAR szPath[MAX_PATH], szAnswer[MAX_PATH];
    CHAR szDebug1[MAX_PATH], szDebug2[MAX_PATH];
    HRESULT hr;
    DWORD cch;

    /* ".txt" */
    lstrcpynW(szPath, L".txt", _countof(szPath));
    cch = _countof(szPath);
    hr = AssocQueryStringW(0, ASSOCSTR_EXECUTABLE, szPath, NULL, szPath, &cch);
    ok_long(hr, S_OK);
    ExpandEnvironmentStringsW(L"%WINDIR%\\system32\\notepad.exe", szAnswer, _countof(szAnswer));
    lstrcpynA(szDebug1, wine_dbgstr_w(szPath), _countof(szDebug1));
    lstrcpynA(szDebug2, wine_dbgstr_w(szAnswer), _countof(szDebug2));
    ok(lstrcmpiW(szPath, szAnswer) == 0, "%s vs %s\n", szDebug1, szDebug2);
    ok_int(cch, lstrlenW(szAnswer) + 1);

    /* s_szTextFileW */
    lstrcpynW(szPath, s_szTextFileW, _countof(szPath));
    cch = _countof(szPath);
    hr = AssocQueryStringW(0, ASSOCSTR_EXECUTABLE, szPath, NULL, szPath, &cch);
    ok_long(hr, S_OK);
    ExpandEnvironmentStringsW(L"%WINDIR%\\system32\\notepad.exe", szAnswer, _countof(szAnswer));
    lstrcpynA(szDebug1, wine_dbgstr_w(szPath), _countof(szDebug1));
    lstrcpynA(szDebug2, wine_dbgstr_w(szAnswer), _countof(szDebug2));
    ok(lstrcmpiW(szPath, szAnswer) == 0, "%s vs %s\n", szDebug1, szDebug2);
    ok_int(cch, lstrlenW(szAnswer) + 1);

    /* NON_EXISTENT_FILENAME_W */
    lstrcpynW(szPath, NON_EXISTENT_FILENAME_W, _countof(szPath));
    cch = _countof(szPath);
    hr = AssocQueryStringW(0, ASSOCSTR_EXECUTABLE, szPath, NULL, szPath, &cch);
    ok_long(hr, S_OK);
    ExpandEnvironmentStringsW(L"%WINDIR%\\system32\\notepad.exe", szAnswer, _countof(szAnswer));
    lstrcpynA(szDebug1, wine_dbgstr_w(szPath), _countof(szDebug1));
    lstrcpynA(szDebug2, wine_dbgstr_w(szAnswer), _countof(szDebug2));
    ok(lstrcmpiW(szPath, szAnswer) == 0, "%s vs %s\n", szDebug1, szDebug2);
    ok_int(cch, lstrlenW(szAnswer) + 1);
}

START_TEST(AssocQueryString)
{
    HRESULT hrCoInit = CoInitialize(NULL);

    TEST_Start();
    TEST_AssocQueryStringA();
    TEST_AssocQueryStringW();
    TEST_End();

    if (SUCCEEDED(hrCoInit))
        CoUninitialize();
}
