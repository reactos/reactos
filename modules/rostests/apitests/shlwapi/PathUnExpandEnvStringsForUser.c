/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Tests for PathUnExpandEnvStringsForUser
 * PROGRAMMERS:     Katayama Hirofumi MZ
 */

#include <apitest.h>
#include <shlwapi.h>
#include <strsafe.h>

#define DO_TEST(Res, hToken, TestStr, ExpStr, Len) \
do { \
    BOOL ret = PathUnExpandEnvStringsForUserW((hToken), (TestStr), OutStr, Len); \
    ok(ret == (Res), "Tested %s, expected returned value %d, got %d\n", \
       wine_dbgstr_w((TestStr)), (Res), ret); \
    if (ret) \
        ok(_wcsicmp(OutStr, (ExpStr)) == 0, "Tested %s, expected %s, got %s\n", \
           wine_dbgstr_w((TestStr)), wine_dbgstr_w((ExpStr)), wine_dbgstr_w(OutStr)); \
} while (0)

// PathUnExpandEnvStringsForUserW
typedef BOOL (WINAPI *PATHUNEXPANDENVSTRINGSFORUSERW)(HANDLE hToken, LPCWSTR pszPath, LPWSTR pszUnExpanded, INT cchUnExpanded);
PATHUNEXPANDENVSTRINGSFORUSERW pPathUnExpandEnvStringsForUserW = NULL;

START_TEST(PathUnExpandEnvStringsForUser)
{
    DWORD ret;
    WCHAR OutStr[MAX_PATH], TestStr[MAX_PATH];
    HINSTANCE hShlwapi;

    hShlwapi = GetModuleHandleW(L"shlwapi");
    if (hShlwapi == NULL)
    {
        skip("shlwapi.dll was not loaded\n");
        return;
    }

    pPathUnExpandEnvStringsForUserW =
        (PATHUNEXPANDENVSTRINGSFORUSERW)GetProcAddress(hShlwapi, "PathUnExpandEnvStringsForUserW");

    if (pPathUnExpandEnvStringsForUserW == NULL)
    {
        trace("PathUnExpandEnvStringsForUserW is not public\n");
        pPathUnExpandEnvStringsForUserW =
            (PATHUNEXPANDENVSTRINGSFORUSERW)GetProcAddress(hShlwapi, (LPCSTR)(LONG_PTR)466);
    }
    if (pPathUnExpandEnvStringsForUserW == NULL)
    {
        skip("PathUnExpandEnvStringsForUserW was not found\n");
        return;
    }

#define PathUnExpandEnvStringsForUserW  (*pPathUnExpandEnvStringsForUserW)

    /* empty string */
    DO_TEST(FALSE, NULL, L"", L"", 0);
    DO_TEST(FALSE, NULL, L"", L"", -1);
    DO_TEST(FALSE, NULL, L"", L"", 2);
    DO_TEST(FALSE, NULL, L"", L"", MAX_PATH);

    /* No unexpansion possible */
    DO_TEST(FALSE, NULL, L"ZZ:\\foobar\\directory", L"", 0);
    DO_TEST(FALSE, NULL, L"ZZ:\\foobar\\directory", L"", -1);
    DO_TEST(FALSE, NULL, L"ZZ:\\foobar\\directory", L"", 2);
    DO_TEST(FALSE, NULL, L"ZZ:\\foobar\\directory", L"", MAX_PATH);

    /* %APPDATA% */
    ret = GetEnvironmentVariableW(L"APPDATA", TestStr, _countof(TestStr));
    ok(ret, "got %lu\n", ret);
    DO_TEST(FALSE, NULL, TestStr, L"%APPDATA%", 0);
    DO_TEST(FALSE, NULL, TestStr, L"%APPDATA%", -1);
    DO_TEST(FALSE, NULL, TestStr, L"%APPDATA%", 2);
    DO_TEST(TRUE, NULL, TestStr, L"%APPDATA%", MAX_PATH);
    StringCbCatW(TestStr, sizeof(TestStr), L"\\TEST");
    DO_TEST(FALSE, NULL, TestStr, L"%APPDATA%\\TEST", 0);
    DO_TEST(FALSE, NULL, TestStr, L"%APPDATA%\\TEST", -1);
    DO_TEST(FALSE, NULL, TestStr, L"%APPDATA%\\TEST", 2);
    DO_TEST(TRUE, NULL, TestStr, L"%APPDATA%\\TEST", MAX_PATH);

    /* %USERPROFILE% */
    ret = GetEnvironmentVariableW(L"USERPROFILE", TestStr, _countof(TestStr));
    ok(ret, "got %lu\n", ret);
    DO_TEST(FALSE, NULL, TestStr, L"%USERPROFILE%", 0);
    DO_TEST(FALSE, NULL, TestStr, L"%USERPROFILE%", -1);
    DO_TEST(FALSE, NULL, TestStr, L"%USERPROFILE%", 2);
    DO_TEST(TRUE, NULL, TestStr, L"%USERPROFILE%", MAX_PATH);
    StringCbCatW(TestStr, sizeof(TestStr), L"\\TEST");
    DO_TEST(FALSE, NULL, TestStr, L"%USERPROFILE%\\TEST", 0);
    DO_TEST(FALSE, NULL, TestStr, L"%USERPROFILE%\\TEST", -1);
    DO_TEST(FALSE, NULL, TestStr, L"%USERPROFILE%\\TEST", 2);
    DO_TEST(TRUE, NULL, TestStr, L"%USERPROFILE%\\TEST", MAX_PATH);

    /* %ALLUSERSPROFILE% */
    ret = GetEnvironmentVariableW(L"ALLUSERSPROFILE", TestStr, _countof(TestStr));
    ok(ret, "got %lu\n", ret);
    DO_TEST(FALSE, NULL, TestStr, L"%ALLUSERSPROFILE%", 0);
    DO_TEST(FALSE, NULL, TestStr, L"%ALLUSERSPROFILE%", -1);
    DO_TEST(FALSE, NULL, TestStr, L"%ALLUSERSPROFILE%", 2);
    DO_TEST(TRUE, NULL, TestStr, L"%ALLUSERSPROFILE%", MAX_PATH);
    StringCbCatW(TestStr, sizeof(TestStr), L"\\TEST");
    DO_TEST(FALSE, NULL, TestStr, L"%ALLUSERSPROFILE%\\TEST", 0);
    DO_TEST(FALSE, NULL, TestStr, L"%ALLUSERSPROFILE%\\TEST", -1);
    DO_TEST(FALSE, NULL, TestStr, L"%ALLUSERSPROFILE%\\TEST", 2);
    DO_TEST(TRUE, NULL, TestStr, L"%ALLUSERSPROFILE%\\TEST", MAX_PATH);

    /* %ProgramFiles% */
    ret = GetEnvironmentVariableW(L"ProgramFiles", TestStr, _countof(TestStr));
    ok(ret, "got %lu\n", ret);
    DO_TEST(FALSE, NULL, TestStr, L"%ProgramFiles%", 0);
    DO_TEST(FALSE, NULL, TestStr, L"%ProgramFiles%", -1);
    DO_TEST(FALSE, NULL, TestStr, L"%ProgramFiles%", 2);
    DO_TEST(TRUE, NULL, TestStr, L"%ProgramFiles%", MAX_PATH);
    StringCbCatW(TestStr, sizeof(TestStr), L"\\TEST");
    DO_TEST(FALSE, NULL, TestStr, L"%ProgramFiles%\\TEST", 0);
    DO_TEST(FALSE, NULL, TestStr, L"%ProgramFiles%\\TEST", -1);
    DO_TEST(FALSE, NULL, TestStr, L"%ProgramFiles%\\TEST", 2);
    DO_TEST(TRUE, NULL, TestStr, L"%ProgramFiles%\\TEST", MAX_PATH);
}
