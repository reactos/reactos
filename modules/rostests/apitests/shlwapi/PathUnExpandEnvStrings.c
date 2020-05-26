/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Tests for PathUnExpandEnvStrings
 * PROGRAMMERS:     Hermes Belusca-Maito
 */

#include <apitest.h>
#include <shlwapi.h>
#include <strsafe.h>

#define DO_TEST(Res, TestStr, ExpStr) \
do { \
    BOOL ret = PathUnExpandEnvStringsW((TestStr), OutStr, _countof(OutStr)); \
    ok(ret == (Res), "Tested %s, expected returned value %d, got %d\n", \
       wine_dbgstr_w((TestStr)), (Res), ret); \
    if (ret) \
        ok(_wcsicmp(OutStr, (ExpStr)) == 0, "Tested %s, expected %s, got %s\n", \
           wine_dbgstr_w((TestStr)), wine_dbgstr_w((ExpStr)), wine_dbgstr_w(OutStr)); \
} while (0)

START_TEST(PathUnExpandEnvStrings)
{
    INT len;
    DWORD ret;
    WCHAR TestStr[MAX_PATH];
    WCHAR ExpStr[MAX_PATH];
    WCHAR OutStr[MAX_PATH];

    /*
     * We expect here that the following standard environment variables:
     * %COMPUTERNAME%, %ProgramFiles%, %SystemRoot% and %SystemDrive%
     * are correctly defined.
     */

    /* No unexpansion possible */
    DO_TEST(FALSE, L"ZZ:\\foobar\\directory", L"");

    /* Contrary to what MSDN says, %COMPUTERNAME% does not seeem to be unexpanded... */
    ret = GetEnvironmentVariableW(L"COMPUTERNAME", TestStr, _countof(TestStr));
    ok(ret, "got %lu\n", ret);
    DO_TEST(FALSE, TestStr, L"%COMPUTERNAME%");
#if 0
    StringCchCopyW(TestStr, _countof(TestStr), L"ZZ:\\foobar\\");
    len = wcslen(TestStr);
    ret = GetEnvironmentVariableW(L"COMPUTERNAME", TestStr + len, _countof(TestStr) - len);
    ok(ret, "got %lu\n", ret);
    StringCchCatW(TestStr, _countof(TestStr), L"\\directory");
    DO_TEST(TRUE, TestStr, L"ZZ:\\foobar\\%COMPUTERNAME%\\directory");
#endif

    /*
     * L"%SystemRoot%\\%SystemRoot%" to L"%SystemRoot%\\%SystemRoot%" (no expansion)
     * Unexpansion fails.
     * This shows that given a path string, if PathUnExpandEnvStrings fails,
     * the string could have been already unexpanded...
     */
    DO_TEST(FALSE, L"%SystemRoot%\\%SystemRoot%", L"%SystemRoot%\\%SystemRoot%");

    /*
     * L"<real_SystemRoot><real_SystemRoot>" to L"%SystemRoot%<real_SystemRoot>"
     * example: L"C:\\WindowsC:\\Windows"
     * Unexpansion succeeds only on the first path.
     */
    ret = GetEnvironmentVariableW(L"SystemRoot", TestStr, _countof(TestStr));
    ok(ret, "got %lu\n", ret);
    len = lstrlenW(TestStr);
    ret = GetEnvironmentVariableW(L"SystemRoot", TestStr + len, _countof(TestStr) - len);
    ok(ret, "got %lu\n", ret);

    StringCchCopyW(ExpStr, _countof(ExpStr), L"%SystemRoot%");
    len = lstrlenW(ExpStr);
    ret = GetEnvironmentVariableW(L"SystemRoot", ExpStr + len, _countof(ExpStr) - len);
    ok(ret, "got %lu\n", ret);
    DO_TEST(TRUE, TestStr, ExpStr);

    /*
     * L"%SystemRoot%\\<real_Program_Files>" to L"%SystemRoot%\\%ProgramFiles%"
     * Unexpansion fails.
     */
    StringCchCopyW(TestStr, _countof(TestStr), L"%SystemRoot%\\");
    len = lstrlenW(TestStr);
    ret = GetEnvironmentVariableW(L"ProgramFiles", TestStr + len, _countof(TestStr) - len);
    ok(ret, "got %lu\n", ret);
    DO_TEST(FALSE, TestStr, L"%SystemRoot%\\%ProgramFiles%");

    /*
     * L"<real_SystemRoot>\\%ProgramFiles%" to L"%SystemRoot%\\%ProgramFiles%"
     * Unexpansion succeeds.
     */
    ret = GetEnvironmentVariableW(L"SystemRoot", TestStr, _countof(TestStr));
    ok(ret, "got %lu\n", ret);
    StringCchCatW(TestStr, _countof(TestStr), L"\\%ProgramFiles%");
    DO_TEST(TRUE, TestStr, L"%SystemRoot%\\%ProgramFiles%");

    /*
     * L"<real_SystemRoot>\\notepad.exe <real_SystemRoot>\\file.txt" to L"%SystemRoot%\\notepad.exe %SystemRoot%\\file.txt"
     * Unexpansion succeeds only on the first path, therefore the obtained string is not the one naively expected.
     */
    ret = GetEnvironmentVariableW(L"SystemRoot", TestStr, _countof(TestStr));
    ok(ret, "got %lu\n", ret);
    StringCchCatW(TestStr, _countof(TestStr), L"\\notepad.exe ");
    len = lstrlenW(TestStr);
    ret = GetEnvironmentVariableW(L"SystemRoot", TestStr + len, _countof(TestStr) - len);
    ok(ret, "got %lu\n", ret);
    StringCchCatW(TestStr, _countof(TestStr), L"\\file.txt");
    // DO_TEST(TRUE, TestStr, L"%SystemRoot%\\notepad.exe %SystemRoot%\\file.txt");

    /*
     * L"<real_SystemRoot>\\notepad.exe <real_SystemRoot>\\file.txt" to L"%SystemRoot%\\notepad.exe <real_SystemRoot>\\file.txt"
     * Unexpansion succeeds only on the first path.
     */
    StringCchCopyW(ExpStr, _countof(ExpStr), L"%SystemRoot%\\notepad.exe ");
    len = lstrlenW(ExpStr);
    ret = GetEnvironmentVariableW(L"SystemRoot", ExpStr + len, _countof(ExpStr) - len);
    ok(ret, "got %lu\n", ret);
    StringCchCatW(ExpStr, _countof(ExpStr), L"\\file.txt");
    DO_TEST(TRUE, TestStr, ExpStr);
}
