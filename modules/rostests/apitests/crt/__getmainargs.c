/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for __getmainargs and __wgetmainargs
 * PROGRAMMER:      Yaroslav Veremenko <yaroslav@veremenko.info>
 */

#include <apitest.h>
#include <stdio.h>
#include <string.h>


const char **__p__acmdln(void);
void __getmainargs(int* argc, char*** argv, char*** env, int expand_wildcards, int* new_mode);
const wchar_t **__p__wcmdln(void);
void __wgetmainargs(int* argc, wchar_t*** wargv, wchar_t*** wenv, int expand_wildcards, int* new_mode);


#define winetest_ok_str(x, y) \
    winetest_ok(strcmp(x, y) == 0, "Wrong string. Expected '%s', got '%s'\n", y, x)
#define winetest_ok_wstr(x, y) \
    winetest_ok(wcscmp(x, y) == 0, "Wrong string. Expected '%s', got '%s'\n", wine_dbgstr_w(y), wine_dbgstr_w(x))
#define ok_argsA  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : ok_argsA_imp
#define ok_argsW  (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : ok_argsW_imp


void
ok_argsA_imp(const char* input_args, const char* arg1, const char* arg2, const char* arg3)
{
    int argc = 0, mode = 0;
    int expect_count = arg3 == NULL ? (arg2 == NULL ? 2 : 3) : 4;
    char** argv, **env;

    /* Remove cached argv, setup our input as program argument. */
    *__p___argv() = NULL;
    *__p__acmdln() = input_args;

    /* Process the commandline stored in _acmdln */
    __getmainargs(&argc, &argv, &env, 0, &mode);

    winetest_ok(argc == expect_count, "Wrong value for argc, expected: %d, got: %d\n", expect_count, argc);
    if(argc != expect_count)
        return;

    winetest_ok_str(argv[0], "test.exe");
    winetest_ok_str(argv[1], arg1);
    if (expect_count > 2)
    {
        winetest_ok_str(argv[2], arg2);
        if (expect_count > 3)
            winetest_ok_str(argv[3], arg3);
    }
}

void
ok_argsW_imp(const wchar_t* input_args, const wchar_t* arg1, const wchar_t* arg2, const wchar_t* arg3)
{
    int argc = 0, mode = 0;
    int expect_count = arg3 == NULL ? (arg2 == NULL ? 2 : 3) : 4;
    wchar_t** argv, **env;

    /* Remove cached wargv, setup our input as program argument. */
    *__p___wargv() = NULL;
    *__p__wcmdln() = input_args;

    /* Process the commandline stored in _wcmdln */
    __wgetmainargs(&argc, &argv, &env, 0, &mode);

    winetest_ok(argc == expect_count, "Wrong value for argc, expected: %d, got: %d\n", expect_count, argc);
    if(argc != expect_count)
        return;

    winetest_ok_wstr(argv[0], L"test.exe");
    winetest_ok_wstr(argv[1], arg1);
    if (expect_count > 2)
    {
        winetest_ok_wstr(argv[2], arg2);
        if (expect_count > 3)
            winetest_ok_wstr(argv[3], arg3);
    }
}

START_TEST(__getmainargs)
{
    ok_argsA("test.exe \"a b c\" d e", "a b c", "d", "e");
    ok_argsA("test.exe \"ab\\\"c\" \"\\\\\" d", "ab\"c", "\\", "d");
    ok_argsA("test.exe a\\\\\\b d\"e f\"g h", "a\\\\\\b", "de fg", "h");
    ok_argsA("test.exe a\\\\\\\"b c d", "a\\\"b", "c", "d");
    ok_argsA("test.exe a\\\\\\\\\"b c\" d e", "a\\\\b c", "d", "e");
    ok_argsA("test.exe a b \"\"", "a", "b", "");
    ok_argsA("test.exe a \"\" b", "a", "", "b");
    ok_argsA("test.exe a \"b\"\" c", "a", "b\"", "c");
    ok_argsA("test.exe a \"b\\\"\" c", "a", "b\"", "c");
    ok_argsA("test.exe a  \"  b\\  \"\"  c", "a", "  b\\  \"", "c");
    ok_argsA("test.exe a  \"b\\  \"\"\"  c\" d", "a", "b\\  \"  c", "d");
    ok_argsA("test.exe a  \"b\\  \"\"\"  \"c  \"\"\"\" d", "a", "b\\  \"  c", "\" d");
    ok_argsA("test.exe a b c  ", "a", "b", "c");
    ok_argsA("test.exe \"a b c\"", "a b c", NULL, NULL);

    ok_argsW(L"test.exe \"a b c\" d e", L"a b c", L"d", L"e");
    ok_argsW(L"test.exe \"ab\\\"c\" \"\\\\\" d", L"ab\"c", L"\\", L"d");
    ok_argsW(L"test.exe a\\\\\\b d\"e f\"g h", L"a\\\\\\b", L"de fg", L"h");
    ok_argsW(L"test.exe a\\\\\\\"b c d", L"a\\\"b", L"c", L"d");
    ok_argsW(L"test.exe a\\\\\\\\\"b c\" d e", L"a\\\\b c", L"d", L"e");
    ok_argsW(L"test.exe a b \"\"", L"a", L"b", L"");
    ok_argsW(L"test.exe a \"\" b", L"a", L"", L"b");
    ok_argsW(L"test.exe a \"b\"\" c", L"a", L"b\"", L"c");
    ok_argsW(L"test.exe a \"b\\\"\" c", L"a", L"b\"", L"c");
    ok_argsW(L"test.exe a  \"  b\\  \"\"  c", L"a", L"  b\\  \"", L"c");
    ok_argsW(L"test.exe a  \"b\\  \"\"\"  c\" d", L"a", L"b\\  \"  c", L"d");
    ok_argsW(L"test.exe a  \"b\\  \"\"\"  \"c  \"\"\"\" d", L"a", L"b\\  \"  c", L"\" d");
    ok_argsW(L"test.exe a b c  ", L"a", L"b", L"c");
    ok_argsW(L"test.exe \"a b c\"", L"a b c", NULL, NULL);
}
