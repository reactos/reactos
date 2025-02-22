/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for AssocQueryStringA/W
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <apitest.h>
#include <shlwapi.h>
#include <stdio.h>
#include <versionhelpers.h>

typedef HRESULT (WINAPI *FN_AssocQueryStringA)(
    ASSOCF cfFlags,
    ASSOCSTR str,
    LPCSTR pszAssoc,
    LPCSTR pszExtra,
    LPSTR pszOut,
    DWORD *pcchOut);
typedef HRESULT (WINAPI *FN_AssocQueryStringW)(
    ASSOCF cfFlags,
    ASSOCSTR str,
    LPCWSTR pszAssoc,
    LPCWSTR pszExtra,
    LPWSTR pszOut,
    DWORD *pcchOut);

static HINSTANCE s_hSHLWAPI = NULL;
static FN_AssocQueryStringA s_fnAssocQueryStringA = NULL;
static FN_AssocQueryStringW s_fnAssocQueryStringW = NULL;
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

    s_hSHLWAPI = LoadLibraryW(L"shlwapi.dll");
    s_fnAssocQueryStringA = (FN_AssocQueryStringA)GetProcAddress(s_hSHLWAPI, "AssocQueryStringA");
    s_fnAssocQueryStringW = (FN_AssocQueryStringW)GetProcAddress(s_hSHLWAPI, "AssocQueryStringW");
    if (!s_fnAssocQueryStringA || !s_fnAssocQueryStringW)
    {
        skip("AssocQueryStringA or AssocQueryStringW not found: %p, %p\n",
             s_fnAssocQueryStringA, s_fnAssocQueryStringW);
        return;
    }

    TEST_Start();
    TEST_AssocQueryStringA();
    TEST_AssocQueryStringW();
    TEST_End();

    FreeLibrary(s_hSHLWAPI);

    if (SUCCEEDED(hrCoInit))
        CoUninitialize();
}
