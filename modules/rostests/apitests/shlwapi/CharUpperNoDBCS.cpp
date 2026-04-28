/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for CharUpperNoDBCSA/W and CharLowerNoDBCSA/W
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlwapi.h>
#include <string.h>
#include <wchar.h>

typedef PSTR  (WINAPI *FN_CharLowerNoDBCSA)(PSTR);
typedef PWSTR (WINAPI *FN_CharLowerNoDBCSW)(PWSTR);
typedef PSTR  (WINAPI *FN_CharUpperNoDBCSA)(PSTR);
typedef PWSTR (WINAPI *FN_CharUpperNoDBCSW)(PWSTR);

static FN_CharLowerNoDBCSA g_fnCharLowerNoDBCSA = NULL;
static FN_CharLowerNoDBCSW g_fnCharLowerNoDBCSW = NULL;
static FN_CharUpperNoDBCSA g_fnCharUpperNoDBCSA = NULL;
static FN_CharUpperNoDBCSW g_fnCharUpperNoDBCSW = NULL;

static void Test_CharUpperNoDBCSA(void)
{
    // Basic ASCII lowercase → uppercase
    {
        char buf[] = "hello world";
        PSTR ret = g_fnCharUpperNoDBCSA(buf);
        ok(ret == buf, "ret was %p\n", ret);
        ok_str(buf, "HELLO WORLD");
    }

    // Already uppercase → unchanged
    {
        char buf[] = "HELLO WORLD";
        g_fnCharUpperNoDBCSA(buf);
        ok_str(buf, "HELLO WORLD");
    }

    // Mixed case
    {
        char buf[] = "Hello World";
        g_fnCharUpperNoDBCSA(buf);
        ok_str(buf, "HELLO WORLD");
    }

    // Digits and symbols are unchanged
    {
        char buf[] = "abc123!@#";
        g_fnCharUpperNoDBCSA(buf);
        ok_str(buf, "ABC123!@#");
    }

    // Empty string
    {
        char buf[] = "";
        PCSTR ret = g_fnCharUpperNoDBCSA(buf);
        ok(ret == NULL, "ret was not NULL.\n");
        ok_int(buf[0], ANSI_NULL);
    }

    // NULL pointer → must return NULL without crashing
    {
        PSTR ret = g_fnCharUpperNoDBCSA(NULL);
        ok(ret == NULL, "ret was %p\n", ret);
    }
}

static void Test_CharLowerNoDBCSA(void)
{
    // Basic ASCII uppercase → lowercase
    {
        char buf[] = "HELLO WORLD";
        PSTR ret = g_fnCharLowerNoDBCSA(buf);
        ok(ret == buf, "ret was %p\n", ret);
        ok_str(buf, "hello world");
    }

    // Already lowercase → unchanged
    {
        char buf[] = "hello world";
        g_fnCharLowerNoDBCSA(buf);
        ok_str(buf, "hello world");
    }

    // Mixed case
    {
        char buf[] = "Hello World";
        g_fnCharLowerNoDBCSA(buf);
        ok_str(buf, "hello world");
    }

    // Digits and symbols are unchanged
    {
        char buf[] = "ABC123!@#";
        g_fnCharLowerNoDBCSA(buf);
        ok_str(buf, "abc123!@#");
    }

    // Empty string
    {
        char buf[] = "";
        PCSTR ret = g_fnCharLowerNoDBCSA(buf);
        ok(ret == NULL, "ret was not NULL.\n");
        ok_int(buf[0], ANSI_NULL);
    }

    // NULL pointer → must return NULL without crashing
    {
        PSTR ret = g_fnCharLowerNoDBCSA(NULL);
        ok(ret == NULL, "ret was %p\n", ret);
    }
}

static void Test_CharUpperNoDBCSW(void)
{
    // Basic ASCII lowercase → uppercase (wide)
    {
        wchar_t buf[] = L"hello world";
        PWSTR ret = g_fnCharUpperNoDBCSW(buf);
        ok(ret == buf, "ret was %p\n", ret);
        ok_wstr(buf, L"HELLO WORLD");
    }

    // Already uppercase → unchanged
    {
        wchar_t buf[] = L"HELLO WORLD";
        g_fnCharUpperNoDBCSW(buf);
        ok_wstr(buf, L"HELLO WORLD");
    }

    // Mixed case
    {
        wchar_t buf[] = L"Hello World";
        g_fnCharUpperNoDBCSW(buf);
        ok_wstr(buf, L"HELLO WORLD");
    }

    // Digits and symbols are unchanged
    {
        wchar_t buf[] = L"abc123!@#";
        g_fnCharUpperNoDBCSW(buf);
        ok_wstr(buf, L"ABC123!@#");
    }

    // Empty string
    {
        wchar_t buf[] = L"";
        PCWSTR ret = g_fnCharUpperNoDBCSW(buf);
        ok(ret == NULL, "ret was not NULL.\n");
        ok_int(buf[0], UNICODE_NULL);
    }

    // NULL pointer → must return NULL without crashing
    {
        PWSTR ret = g_fnCharUpperNoDBCSW(NULL);
        ok(ret == NULL, "ret was %p\n", ret);
    }
}

static void Test_CharLowerNoDBCSW(void)
{
    // Basic ASCII uppercase → lowercase (wide)
    {
        wchar_t buf[] = L"HELLO WORLD";
        PWSTR ret = g_fnCharLowerNoDBCSW(buf);
        ok(ret == buf, "ret was %p\n", ret);
        ok_wstr(buf, L"hello world");
    }

    // Already lowercase → unchanged
    {
        wchar_t buf[] = L"hello world";
        g_fnCharLowerNoDBCSW(buf);
        ok_wstr(buf, L"hello world");
    }

    // Mixed case
    {
        wchar_t buf[] = L"Hello World";
        g_fnCharLowerNoDBCSW(buf);
        ok_wstr(buf, L"hello world");
    }

    // Digits and symbols are unchanged
    {
        wchar_t buf[] = L"ABC123!@#";
        g_fnCharLowerNoDBCSW(buf);
        ok_wstr(buf, L"abc123!@#");
    }

    // Empty string
    {
        wchar_t buf[] = L"";
        PCWSTR ret = g_fnCharLowerNoDBCSW(buf);
        ok(ret == NULL, "ret was not NULL.\n");
        ok_int(buf[0], UNICODE_NULL);
    }

    // NULL pointer → must return NULL without crashing
    {
        PWSTR ret = g_fnCharLowerNoDBCSW(NULL);
        ok(ret == NULL, "ret was %p\n", ret);
    }
}

static void Test_RoundTrip(void)
{
    {
        char upper[] = "Hello World 123";
        char lower[] = "Hello World 123";
        g_fnCharUpperNoDBCSA(upper);        // "HELLO WORLD 123"
        g_fnCharLowerNoDBCSA(lower);        // "hello world 123"

        char rt1[] = "hello world 123";
        g_fnCharUpperNoDBCSA(rt1);          // Lower then Upper → "HELLO WORLD 123"
        ok_str(rt1, upper);

        char rt2[] = "HELLO WORLD 123";
        g_fnCharLowerNoDBCSA(rt2);          // Upper then Lower → "hello world 123"
        ok_str(rt2, lower);
    }

    {
        wchar_t upper[] = L"Hello World 123";
        wchar_t lower[] = L"Hello World 123";
        g_fnCharUpperNoDBCSW(upper);
        g_fnCharLowerNoDBCSW(lower);

        wchar_t rt1[] = L"hello world 123";
        g_fnCharUpperNoDBCSW(rt1);
        ok_wstr(rt1, upper);

        wchar_t rt2[] = L"HELLO WORLD 123";
        g_fnCharLowerNoDBCSW(rt2);
        ok_wstr(rt2, lower);
    }
}

static void Test_NoDBCS(void)
{
    BYTE buf[] = { 0x82, 'a', ANSI_NULL };
    g_fnCharUpperNoDBCSA((PSTR)buf);
    ok_int(buf[2], ANSI_NULL);
}

START_TEST(CharUpperNoDBCS)
{
    HINSTANCE hSHLWAPI = LoadLibraryW(L"shlwapi");
    if (!hSHLWAPI)
    {
        skip("shlwapi not found\n");
        return;
    }

    g_fnCharLowerNoDBCSA = (FN_CharLowerNoDBCSA)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(453));
    g_fnCharLowerNoDBCSW = (FN_CharLowerNoDBCSW)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(454));
    g_fnCharUpperNoDBCSA = (FN_CharUpperNoDBCSA)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(451));
    g_fnCharUpperNoDBCSW = (FN_CharUpperNoDBCSW)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(452));
    if (!g_fnCharLowerNoDBCSA || !g_fnCharLowerNoDBCSW ||
        !g_fnCharUpperNoDBCSA || !g_fnCharUpperNoDBCSW)
    {
        skip("CharUpperNoDBCSA/W or CharLowerNoDBCSA/W not found\n");
        FreeLibrary(hSHLWAPI);
        return;
    }

    Test_CharUpperNoDBCSA();
    Test_CharLowerNoDBCSA();
    Test_CharUpperNoDBCSW();
    Test_CharLowerNoDBCSW();
    Test_RoundTrip();
    Test_NoDBCS();

    FreeLibrary(hSHLWAPI);
}
