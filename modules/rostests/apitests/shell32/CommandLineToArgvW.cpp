/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for CommandLineToArgvW
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <atlstr.h>

static VOID
DoEntry(INT lineno, LPCWSTR cmdline, INT expected_argc, ...)
{
    va_list va;
    va_start(va, expected_argc);

    INT real_argc;
    LPWSTR *real_argv = CommandLineToArgvW(cmdline, &real_argc);

    INT common = min(expected_argc, real_argc);
    for (INT i = 1; i < common; ++i)
    {
        CStringA str1 = wine_dbgstr_w(va_arg(va, LPCWSTR));
        CStringA str2 = wine_dbgstr_w(real_argv[i]);
        ok(str1 == str2, "Line %d: %s != %s\n", lineno, (LPCSTR)str1, (LPCSTR)str2);
    }

    INT diff = abs(expected_argc - real_argc);
    while (diff-- > 0)
    {
        ok(expected_argc == real_argc,
           "Line %d: expected_argc %d and real_argc %d are different\n",
           lineno, expected_argc, real_argc);
    }

    LocalFree(real_argv);
    va_end(va);
}

START_TEST(CommandLineToArgvW)
{
    DoEntry(__LINE__, L"", 1);
    DoEntry(__LINE__, L"test.exe", 1);
    DoEntry(__LINE__, L"test.exe ", 1);
    DoEntry(__LINE__, L"test.exe\t", 1);
    DoEntry(__LINE__, L"test.exe\r", 1);
    DoEntry(__LINE__, L"test.exe\n", 1);
    DoEntry(__LINE__, L"test.exe\v", 1);
    DoEntry(__LINE__, L"test.exe\f", 1);
    DoEntry(__LINE__, L"test.exe\u3000", 1);
    DoEntry(__LINE__, L"\"This is a test.exe\"", 1);
    DoEntry(__LINE__, L"\"This is a test.exe\" ", 1);
    DoEntry(__LINE__, L"\"This is a test.exe\"\t", 1);
    DoEntry(__LINE__, L"\"This is a test.exe\"\r", 2, L"\r");
    DoEntry(__LINE__, L"\"This is a test.exe\"\n", 2, L"\n");
    DoEntry(__LINE__, L"\"This is a test.exe\"\v", 2, L"\v");
    DoEntry(__LINE__, L"\"This is a test.exe\"\f", 2, L"\f");
    DoEntry(__LINE__, L"\"This is a test.exe\"\u3000", 2, L"\u3000");
    DoEntry(__LINE__, L"test.exe a", 2, L"a");
    DoEntry(__LINE__, L"test.exe\ta", 2, L"a");
    DoEntry(__LINE__, L"test.exe\ra", 2, L"a");
    DoEntry(__LINE__, L"test.exe\na", 2, L"a");
    DoEntry(__LINE__, L"test.exe\va", 2, L"a");
    DoEntry(__LINE__, L"test.exe\fa", 2, L"a");
    DoEntry(__LINE__, L"test.exe\u3000" L"a", 1);
    DoEntry(__LINE__, L"test.exe  a", 2, L"a");
    DoEntry(__LINE__, L"test.exe \ta", 2, L"a");
    DoEntry(__LINE__, L"test.exe \ra", 2, L"\ra");
    DoEntry(__LINE__, L"test.exe \na", 2, L"\na");
    DoEntry(__LINE__, L"test.exe \va", 2, L"\va");
    DoEntry(__LINE__, L"test.exe \fa", 2, L"\fa");
    DoEntry(__LINE__, L"test.exe a ", 2, L"a");
    DoEntry(__LINE__, L"test.exe a\t", 2, L"a");
    DoEntry(__LINE__, L"test.exe a\r", 2, L"a\r");
    DoEntry(__LINE__, L"test.exe a\n", 2, L"a\n");
    DoEntry(__LINE__, L"test.exe a\v", 2, L"a\v");
    DoEntry(__LINE__, L"test.exe a\f", 2, L"a\f");
    DoEntry(__LINE__, L"test.exe \"a\" ", 2, L"a");
    DoEntry(__LINE__, L"test.exe \"a\"\t", 2, L"a");
    DoEntry(__LINE__, L"test.exe \"a\"\r", 2, L"a\r");
    DoEntry(__LINE__, L"test.exe \"a\"\n", 2, L"a\n");
    DoEntry(__LINE__, L"test.exe \"a\"\v", 2, L"a\v");
    DoEntry(__LINE__, L"test.exe \"a\"\f", 2, L"a\f");
    DoEntry(__LINE__, L"test.exe \u3000" L"a", 2, L"\u3000" L"a");
    DoEntry(__LINE__, L"test.exe \"a b\"", 2, L"a b");
    DoEntry(__LINE__, L"test.exe \"a\tb\"", 2, L"a\tb");
    DoEntry(__LINE__, L"test.exe \"a\rb\"", 2, L"a\rb");
    DoEntry(__LINE__, L"test.exe \"a\nb\"", 2, L"a\nb");
    DoEntry(__LINE__, L"test.exe \"a\vb\"", 2, L"a\vb");
    DoEntry(__LINE__, L"test.exe \"a\fb\"", 2, L"a\fb");
    DoEntry(__LINE__, L"test.exe \"a\u3000" L"b\"", 2, L"a\u3000" L"b");
    DoEntry(__LINE__, L"test.exe a b c", 4, L"a", L"b", L"c");
    DoEntry(__LINE__, L"test.exe a b \"c", 4, L"a", L"b", L"c");
    DoEntry(__LINE__, L"test.exe \"a b\" \"c d\"", 3, L"a b", L"c d");
    DoEntry(__LINE__, L"test.exe \"a \" d\"", 3, L"a ", L"d");
    DoEntry(__LINE__, L"test.exe \"0 1\"\" 2", 3, L"0 1\"", L"2");
    DoEntry(__LINE__, L"test.exe \"0 1\"\"\" 2", 2, L"0 1\" 2");
}
