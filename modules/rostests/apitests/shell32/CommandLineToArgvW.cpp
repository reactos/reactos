/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for CommandLineToArgvW
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <atlstr.h>

static VOID
DoEntry(INT lineno, LPWSTR cmdline, INT argc_minus_1, ...)
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
        ok(lstrcmpW(arg1, arg2) == 0, "Line %d: %s != %s\n", lineno, (LPCSTR)str1, (LPCSTR)str2);
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
    {
        WCHAR cmdline[] = L"";
        DoEntry(__LINE__, cmdline, 0);
    }
    {
        WCHAR cmdline[] = L"test.exe";
        DoEntry(__LINE__, cmdline, 0);
    }
    {
        WCHAR cmdline[] = L"test.exe ";
        DoEntry(__LINE__, cmdline, 0);
    }
    {
        WCHAR cmdline[] = L"test.exe\t";
        DoEntry(__LINE__, cmdline, 0);
    }
    {
        WCHAR cmdline[] = L"test.exe\r";
        DoEntry(__LINE__, cmdline, 0);
    }
    {
        WCHAR cmdline[] = L"test.exe\n";
        DoEntry(__LINE__, cmdline, 0);
    }
    {
        WCHAR cmdline[] = L"\"This is a test.exe\"";
        DoEntry(__LINE__, cmdline, 0);
    }
    {
        WCHAR cmdline[] = L"\"This is a test.exe\" ";
        DoEntry(__LINE__, cmdline, 0);
    }
    {
        WCHAR cmdline[] = L"\"This is a test.exe\"\t";
        DoEntry(__LINE__, cmdline, 0);
    }
    {
        WCHAR cmdline[] = L"\"This is a test.exe\"\r";
        DoEntry(__LINE__, cmdline, 1, L"\r");
    }
    {
        WCHAR cmdline[] = L"\"This is a test.exe\"\n";
        DoEntry(__LINE__, cmdline, 1, L"\n");
    }
    {
        WCHAR cmdline[] = L"test.exe a";
        DoEntry(__LINE__, cmdline, 1, L"a");
    }
    {
        WCHAR cmdline[] = L"test.exe\ta";
        DoEntry(__LINE__, cmdline, 1, L"a");
    }
    {
        WCHAR cmdline[] = L"test.exe\ra";
        DoEntry(__LINE__, cmdline, 1, L"a");
    }
    {
        WCHAR cmdline[] = L"test.exe\na";
        DoEntry(__LINE__, cmdline, 1, L"a");
    }
    {
        WCHAR cmdline[] = L"test.exe a b c";
        DoEntry(__LINE__, cmdline, 3, L"a", L"b", L"c");
    }
    {
        WCHAR cmdline[] = L"test.exe a b \"c";
        DoEntry(__LINE__, cmdline, 3, L"a", L"b", L"c");
    }
    {
        WCHAR cmdline[] = L"test.exe \"a b\" \"c d\"";
        DoEntry(__LINE__, cmdline, 2, L"a b", L"c d");
    }
    {
        WCHAR cmdline[] = L"test.exe \"a \" d\"";
        DoEntry(__LINE__, cmdline, 2, L"a ", L"d");
    }
    {
        WCHAR cmdline[] = L"test.exe \"0 1\"\" 2";
        DoEntry(__LINE__, cmdline, 2, L"0 1\"", L"2");
    }
    {
        WCHAR cmdline[] = L"test.exe \"0 1\"\"\" 2";
        DoEntry(__LINE__, cmdline, 1, L"0 1\" 2");
    }
}
