/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for CommandLineToArgvW
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <atlstr.h>

static VOID
DoEntry(INT lineno, LPCWSTR cmdline, INT argc_minus_1, ...)
{
    va_list va;
    va_start(va, argc_minus_1);

    INT real_argc;
    LPWSTR* real_argv = CommandLineToArgvW(cmdline, &real_argc);
    INT real_argc_minus_1 = real_argc - 1;

    INT m = min(argc_minus_1, real_argc_minus_1);
    for (INT i = 0; i < m; ++i)
    {
        LPCWSTR arg1 = va_arg(va, LPCWSTR);
        LPCWSTR arg2 = real_argv[i + 1];
        CStringA str1 = wine_dbgstr_w(arg1);
        CStringA str2 = wine_dbgstr_w(arg2);
        ok(str1 == str2, "Line %d: %s != %s\n", lineno, (LPCSTR)str1, (LPCSTR)str2);
    }

    INT diff = abs(argc_minus_1 - real_argc_minus_1);
    while (diff-- > 0)
    {
        ok(argc_minus_1 == real_argc_minus_1,
           "Line %d: argc_minus_1 %d and real_argc_minus_1 %d are different\n",
           lineno, argc_minus_1, real_argc_minus_1);
    }

    LocalFree(real_argv);
    va_end(va);
}

START_TEST(CommandLineToArgvW)
{
    DoEntry(__LINE__, L"", 0);
    DoEntry(__LINE__, L"test.exe", 0);
    DoEntry(__LINE__, L"test.exe ", 0);
    DoEntry(__LINE__, L"test.exe\t", 0);
    DoEntry(__LINE__, L"test.exe\r", 0);
    DoEntry(__LINE__, L"test.exe\n", 0);
    DoEntry(__LINE__, L"\"This is a test.exe\"", 0);
    DoEntry(__LINE__, L"\"This is a test.exe\" ", 0);
    DoEntry(__LINE__, L"\"This is a test.exe\"\t", 0);
    DoEntry(__LINE__, L"\"This is a test.exe\"\r", 1, L"\r");
    DoEntry(__LINE__, L"\"This is a test.exe\"\n", 1, L"\n");
    DoEntry(__LINE__, L"test.exe a", 1, L"a");
    DoEntry(__LINE__, L"test.exe\ta", 1, L"a");
    DoEntry(__LINE__, L"test.exe\ra", 1, L"a");
    DoEntry(__LINE__, L"test.exe\na", 1, L"a");
    DoEntry(__LINE__, L"test.exe a b c", 3, L"a", L"b", L"c");
    DoEntry(__LINE__, L"test.exe a b \"c", 3, L"a", L"b", L"c");
    DoEntry(__LINE__, L"test.exe \"a b\" \"c d\"", 2, L"a b", L"c d");
    DoEntry(__LINE__, L"test.exe \"a \" d\"", 2, L"a ", L"d");
    DoEntry(__LINE__, L"test.exe \"0 1\"\" 2", 2, L"0 1\"", L"2");
    DoEntry(__LINE__, L"test.exe \"0 1\"\"\" 2", 1, L"0 1\" 2");
}
