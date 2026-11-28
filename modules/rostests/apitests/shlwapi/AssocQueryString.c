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
    /* Note: NT5 differs from NT6 because NT5 converts a NULL pszExtra to L"" */
    CHAR szPath[MAX_PATH], szAnswer[MAX_PATH];
    CHAR szDebug1[MAX_PATH], szDebug2[MAX_PATH];
    HRESULT hr;
    DWORD cch;

    /* ".txt" */
    lstrcpynA(szPath, ".txt", _countof(szPath));
    cch = _countof(szPath);
    hr = AssocQueryStringA(ASSOCF_NONE, ASSOCSTR_EXECUTABLE, szPath, NULL, szPath, &cch);
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
    hr = AssocQueryStringA(ASSOCF_NONE, ASSOCSTR_EXECUTABLE, szPath, NULL, szPath, &cch);
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
    hr = AssocQueryStringA(ASSOCF_NONE, ASSOCSTR_EXECUTABLE, szPath, NULL, szPath, &cch);
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
    hr = AssocQueryStringW(ASSOCF_NONE, ASSOCSTR_EXECUTABLE, szPath, NULL, szPath, &cch);
    ok_long(hr, S_OK);
    ExpandEnvironmentStringsW(L"%WINDIR%\\system32\\notepad.exe", szAnswer, _countof(szAnswer));
    lstrcpynA(szDebug1, wine_dbgstr_w(szPath), _countof(szDebug1));
    lstrcpynA(szDebug2, wine_dbgstr_w(szAnswer), _countof(szDebug2));
    ok(lstrcmpiW(szPath, szAnswer) == 0, "%s vs %s\n", szDebug1, szDebug2);
    ok_int(cch, lstrlenW(szAnswer) + 1);

    /* s_szTextFileW */
    lstrcpynW(szPath, s_szTextFileW, _countof(szPath));
    cch = _countof(szPath);
    hr = AssocQueryStringW(ASSOCF_NONE, ASSOCSTR_EXECUTABLE, szPath, NULL, szPath, &cch);
    ok_long(hr, S_OK);
    ExpandEnvironmentStringsW(L"%WINDIR%\\system32\\notepad.exe", szAnswer, _countof(szAnswer));
    lstrcpynA(szDebug1, wine_dbgstr_w(szPath), _countof(szDebug1));
    lstrcpynA(szDebug2, wine_dbgstr_w(szAnswer), _countof(szDebug2));
    ok(lstrcmpiW(szPath, szAnswer) == 0, "%s vs %s\n", szDebug1, szDebug2);
    ok_int(cch, lstrlenW(szAnswer) + 1);

    /* NON_EXISTENT_FILENAME_W */
    lstrcpynW(szPath, NON_EXISTENT_FILENAME_W, _countof(szPath));
    cch = _countof(szPath);
    hr = AssocQueryStringW(ASSOCF_NONE, ASSOCSTR_EXECUTABLE, szPath, NULL, szPath, &cch);
    ok_long(hr, S_OK);
    ExpandEnvironmentStringsW(L"%WINDIR%\\system32\\notepad.exe", szAnswer, _countof(szAnswer));
    lstrcpynA(szDebug1, wine_dbgstr_w(szPath), _countof(szDebug1));
    lstrcpynA(szDebug2, wine_dbgstr_w(szAnswer), _countof(szDebug2));
    ok(lstrcmpiW(szPath, szAnswer) == 0, "%s vs %s\n", szDebug1, szDebug2);
    ok_int(cch, lstrlenW(szAnswer) + 1);
}

/*
 * The values below are compared in full, including their registry types.
 * HKCR\InternetShortcut is written by two sources of the same class: the setup
 * hives (hivecls.inf, hivesft.inf) and DllRegisterServer of ieframe.dll
 * (ieframe.rgs). A drift between them is silent at build time and was already
 * found once (unquoted %1 in the open command, string EditFlags), so the test
 * pins the agreed values (ADR 0023a). The test reads the live registry of the
 * guest, that is what really got applied: hive only on a LiveCD, hive plus the
 * .rgs on an installed system.
 */
static void TEST_AssocQueryStringInternetShortcut(void)
{
    static const WCHAR* const emptyValues[] =
    {
        L"IsShortcut",
        L"NeverShowExt"
    };
    static const struct
    {
        LPCWSTR name;
        LPCWSTR value;
    } prefixes[] =
    {
        { L"www",    L"http://"   },
        { L"home",   L"http://"   },
        { L"mosaic", L"http://"   },
        { L"ftp",    L"ftp://"    },
        { L"gopher", L"gopher://" }
    };
    WCHAR szCommand[MAX_PATH], szValue[MAX_PATH];
    CHAR szDebug[MAX_PATH];
    DWORD dwType, dwValue, cbData, cch, i;
    HKEY hKey, hPrefixesKey;
    LSTATUS error;
    HRESULT hr;

    /* The shell runs this command as is, so %1 must arrive quoted: the default
     * shortcut name ("New Internet Shortcut.url") contains a space and
     * rundll32 would take the first part of the path for its argument. */
    error = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"InternetShortcut\\shell\\open\\command",
                          0, KEY_READ, &hKey);
    ok_long(error, ERROR_SUCCESS);
    if (error == ERROR_SUCCESS)
    {
        cbData = sizeof(szCommand);
        error = RegQueryValueExW(hKey, NULL, NULL, &dwType, (LPBYTE)szCommand, &cbData);
        ok_long(error, ERROR_SUCCESS);
        if (error == ERROR_SUCCESS)
        {
            ok_long(dwType, REG_SZ);
            lstrcpynA(szDebug, wine_dbgstr_w(szCommand), _countof(szDebug));
            ok(lstrcmpiW(szCommand, L"rundll32.exe ieframe.dll,OpenURL \"%1\"") == 0,
               "szCommand was %s\n", szDebug);
        }
        RegCloseKey(hKey);
    }

    error = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"InternetShortcut", 0, KEY_READ, &hKey);
    ok_long(error, ERROR_SUCCESS);
    if (error == ERROR_SUCCESS)
    {
        /* EditFlags is read back by shell32 with GetRegDWORD, a string value
         * would be ignored there. */
        cbData = sizeof(dwValue);
        error = RegQueryValueExW(hKey, L"EditFlags", NULL, &dwType, (LPBYTE)&dwValue, &cbData);
        ok_long(error, ERROR_SUCCESS);
        if (error == ERROR_SUCCESS)
        {
            ok_long(dwType, REG_DWORD);
            ok_long(dwValue, 2);
        }

        for (i = 0; i < _countof(emptyValues); ++i)
        {
            cbData = sizeof(szValue);
            error = RegQueryValueExW(hKey, emptyValues[i], NULL, &dwType, (LPBYTE)szValue, &cbData);
            ok_long(error, ERROR_SUCCESS);
            if (error == ERROR_SUCCESS)
            {
                ok_long(dwType, REG_SZ);
                lstrcpynA(szDebug, wine_dbgstr_w(szValue), _countof(szDebug));
                ok_long(cbData, sizeof(WCHAR));
                ok(szValue[0] == 0, "%s was %s\n", wine_dbgstr_w(emptyValues[i]), szDebug);
            }
        }

        /* Both forms are in use: the hives write REG_EXPAND_SZ
         * "%SystemRoot%\system32\url.dll,0", ieframe.rgs can only write
         * REG_SZ "url.dll,0", because the registrar has no REG_EXPAND_SZ type
         * and stops on an unknown %VAR% in do_preprocess (ADR 0023a). The two
         * forms point to the same icon, so the suffix is compared. */
        cbData = sizeof(szValue);
        error = RegQueryValueExW(hKey, L"DefaultIcon", NULL, &dwType, (LPBYTE)szValue, &cbData);
        ok_long(error, ERROR_SUCCESS);
        if (error == ERROR_SUCCESS)
        {
            lstrcpynA(szDebug, wine_dbgstr_w(szValue), _countof(szDebug));
            ok(dwType == REG_SZ || dwType == REG_EXPAND_SZ,
               "dwType was %lu\n", (unsigned long)dwType);
            cch = (DWORD)lstrlenW(szValue);
            ok(cch >= 9, "szValue was %s\n", szDebug);
            if (cch >= 9)
                ok(lstrcmpiW(szValue + cch - 9, L"url.dll,0") == 0, "szValue was %s\n", szDebug);
        }
        RegCloseKey(hKey);
    }

    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\URL",
                          0, KEY_READ, &hKey);
    ok_long(error, ERROR_SUCCESS);
    if (error == ERROR_SUCCESS)
    {
        cbData = sizeof(szValue);
        error = RegQueryValueExW(hKey, L"DefaultPrefix", NULL, &dwType, (LPBYTE)szValue, &cbData);
        ok_long(error, ERROR_SUCCESS);
        if (error == ERROR_SUCCESS)
        {
            ok_long(dwType, REG_SZ);
            lstrcpynA(szDebug, wine_dbgstr_w(szValue), _countof(szDebug));
            ok(lstrcmpiW(szValue, L"http://") == 0, "szValue was %s\n", szDebug);
        }

        error = RegOpenKeyExW(hKey, L"Prefixes", 0, KEY_READ, &hPrefixesKey);
        ok_long(error, ERROR_SUCCESS);
        if (error == ERROR_SUCCESS)
        {
            for (i = 0; i < _countof(prefixes); ++i)
            {
                cbData = sizeof(szValue);
                error = RegQueryValueExW(hPrefixesKey, prefixes[i].name, NULL, &dwType,
                                         (LPBYTE)szValue, &cbData);
                ok_long(error, ERROR_SUCCESS);
                if (error == ERROR_SUCCESS)
                {
                    ok_long(dwType, REG_SZ);
                    lstrcpynA(szDebug, wine_dbgstr_w(szValue), _countof(szDebug));
                    ok(lstrcmpiW(szValue, prefixes[i].value) == 0, "%s was %s\n",
                       wine_dbgstr_w(prefixes[i].name), szDebug);
                }
            }
            RegCloseKey(hPrefixesKey);
        }
        RegCloseKey(hKey);
    }

    /* Only the API level is checked here: the exact value and its type live in
     * the key above, while AssocQueryStringW may substitute the executable or
     * the arguments on its own. */
    cch = _countof(szCommand);
    hr = AssocQueryStringW(ASSOCF_NONE, ASSOCSTR_COMMAND, L".url", L"open", szCommand, &cch);
    ok_long(hr, S_OK);
    if (hr == S_OK)
    {
        lstrcpynA(szDebug, wine_dbgstr_w(szCommand), _countof(szDebug));
        ok(wcsstr(szCommand, L"ieframe.dll,OpenURL") != NULL, "szCommand was %s\n", szDebug);
    }
}

static void TEST_TrickyPointers(void)
{
    HRESULT hr;
    CHAR  szPathA[MAX_PATH], szOutputA[MAX_PATH];
    WCHAR szPathW[MAX_PATH], szOutputW[MAX_PATH];
    DWORD cch;
    BOOL bVistaPlus = IsWindowsVistaOrGreater();

    lstrcpynA(szPathA, s_szTextFileA, _countof(szPathA));
    cch = _countof(szOutputA);
    _SEH2_TRY
    {
        hr = AssocQueryStringA(ASSOCF_NONE, ASSOCSTR_EXECUTABLE, szPathA, NULL, szOutputA, (PDWORD)UlongToPtr(cch));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = 0xDEADFACE;
    }
    _SEH2_END;

    if (bVistaPlus)
        ok_long(hr, 0xDEADFACE);
    else
        ok_long(hr, HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

    lstrcpynW(szPathW, s_szTextFileW, _countof(szPathW));
    cch = _countof(szOutputW);
    _SEH2_TRY
    {
        hr = AssocQueryStringW(ASSOCF_NONE, ASSOCSTR_EXECUTABLE, szPathW, NULL, szOutputW, (PDWORD)UlongToPtr(cch));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = 0xDEADFACE;
    }
    _SEH2_END;

    if (bVistaPlus)
        ok_long(hr, 0xDEADFACE);
    else
        ok_long(hr, S_OK);
}

START_TEST(AssocQueryString)
{
    HRESULT hrCoInit = CoInitialize(NULL);

    TEST_Start();
    TEST_AssocQueryStringA();
    TEST_AssocQueryStringW();
    TEST_AssocQueryStringInternetShortcut();
    TEST_TrickyPointers();
    TEST_End();

    if (SUCCEEDED(hrCoInit))
        CoUninitialize();
}
