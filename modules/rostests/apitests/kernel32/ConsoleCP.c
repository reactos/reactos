/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for i18n console test
 * PROGRAMMERS:     Katayama Hirofumi MZ
 */

#include "precomp.h"

#define okCURSOR(hCon, c) do { \
  CONSOLE_SCREEN_BUFFER_INFO __sbi; \
  BOOL expect = GetConsoleScreenBufferInfo((hCon), &__sbi) && \
                __sbi.dwCursorPosition.X == (c).X && __sbi.dwCursorPosition.Y == (c).Y; \
  ok(expect, "Expected cursor at (%d,%d), got (%d,%d)\n", \
     (c).X, (c).Y, __sbi.dwCursorPosition.X, __sbi.dwCursorPosition.Y); \
} while (0)

#define ATTR \
    (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)

static const WCHAR u0414[] = { 0x0414, 0 };             /* Д */
static const WCHAR u9580[] = { 0x9580, 0 };             /* 門 */
static const WCHAR ideograph_space = (WCHAR)0x3000;     /* fullwidth space */
LCID lcidJapanese = MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT), SORT_DEFAULT);
LCID lcidRussian  = MAKELCID(MAKELANGID(LANG_RUSSIAN , SUBLANG_DEFAULT), SORT_DEFAULT);

static BOOL IsCJKCodePage(void)
{
    switch (GetOEMCP())
    {
    case 936:   // Chinese PRC
    case 932:   // Japanese
    case 949:   // Korean
    case 1361:  // Korean (Johab)
    case 950:   // Taiwan
        return TRUE;
    }
    return FALSE;
}

/* Russian Code Page 855 */
// NOTE that CP 866 can also be used
static void test_cp855(HANDLE hConOut)
{
    BOOL ret;
    DWORD oldcp;
    int n;
    DWORD len;
    COORD c;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int count;
    WCHAR str[32];
    WORD attr;

    if (!IsValidCodePage(855))
    {
        skip("Codepage 855 not available\n");
        return;
    }

    /* Set code page */
    oldcp = GetConsoleOutputCP();
    SetLastError(0xdeadbeef);
    ret = SetConsoleOutputCP(855);
    if (!ret)
    {
        skip("SetConsoleOutputCP failed with last error %lu\n", GetLastError());
        return;
    }

    /* Get info */
    ret = GetConsoleScreenBufferInfo(hConOut, &csbi);
    ok(ret, "GetConsoleScreenBufferInfo failed\n");
    trace("csbi.dwSize.X:%d, csbi.dwSize.Y:%d\n", csbi.dwSize.X, csbi.dwSize.Y);
    count = csbi.dwSize.X * 3 / 2;
    trace("count: %d\n", count);

    /* "\u0414" */
    {
        /* Output u0414 "count" times at (0,0) */
        c.X = c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);
        for (n = 0; n < count; ++n)
        {
            ret = WriteConsoleW(hConOut, u0414, lstrlenW(u0414), &len, NULL);
            ok(ret && len == lstrlenW(u0414), "WriteConsoleW failed\n");
        }

        /* Check cursor */
        len = count;        /* u0414 is normal width in Russian */
        c.X = (SHORT)(len % csbi.dwSize.X);
        c.Y = (SHORT)(len / csbi.dwSize.X);
        okCURSOR(hConOut, c);

        /* Read characters at (0,0) */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputCharacterW(hConOut, str, 3 * sizeof(WCHAR), c, &len);
        ok(ret, "ReadConsoleOutputCharacterW failed\n");
        ok(len == 6, "len was: %ld\n", len);
        ok(str[0] == 0x414, "str[0] was: 0x%04X\n", str[0]);
        ok(str[1] == 0x414, "str[1] was: 0x%04X\n", str[1]);
        ok(str[2] == 0x414, "str[2] was: 0x%04X\n", str[2]);

        /* Check cursor */
        c.X = 1;
        c.Y = 0;
        ret = SetConsoleCursorPosition(hConOut, c);
        ok(ret, "SetConsoleCursorPosition failed\n");
        okCURSOR(hConOut, c);

        /* Fill by space */
        c.X = c.Y = 0;
        FillConsoleOutputCharacterW(hConOut, L' ', csbi.dwSize.X * csbi.dwSize.Y, c, &len);

        /* Output u0414 "count" times at (1,0) */
        c.X = 1;
        c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);
        for (n = 0; n < count; ++n)
        {
            ret = WriteConsoleW(hConOut, u0414, lstrlenW(u0414), &len, NULL);
            ok(ret && len == lstrlenW(u0414), "WriteConsoleW failed\n");
        }

        /* Check cursor */
        len = 1 + count;
        c.X = (SHORT)(len % csbi.dwSize.X);
        c.Y = (SHORT)(len / csbi.dwSize.X);
        okCURSOR(hConOut, c);

        /* Read characters at (0,0) */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputCharacterW(hConOut, str, 3 * sizeof(WCHAR), c, &len);
        ok(ret, "ReadConsoleOutputCharacterW failed\n");
        ok(len == 6, "len was: %ld\n", len);
        ok(str[0] == L' ', "str[0] was: 0x%04X\n", str[0]);
        ok(str[1] == 0x414, "str[1] was: 0x%04X\n", str[1]);
        ok(str[2] == 0x414, "str[2] was: 0x%04X\n", str[2]);
    }

    /* "\u9580" */
    {
        /* Output u9580 "count" times at (0,0) */
        c.X = c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);
        for (n = 0; n < count; ++n)
        {
            ret = WriteConsoleW(hConOut, u9580, lstrlenW(u9580), &len, NULL);
            ok(ret && len == lstrlenW(u9580), "WriteConsoleW failed\n");
        }

        /* Check cursor */
        len = count;        /* u9580 is normal width in Russian */
        c.X = (SHORT)(len % csbi.dwSize.X);
        c.Y = (SHORT)(len / csbi.dwSize.X);
        okCURSOR(hConOut, c);

        /* Check cursor */
        c.X = 1;
        c.Y = 0;
        ret = SetConsoleCursorPosition(hConOut, c);
        ok(ret, "SetConsoleCursorPosition failed\n");
        okCURSOR(hConOut, c);

        /* Fill by space */
        c.X = c.Y = 0;
        ret = FillConsoleOutputCharacterW(hConOut, L' ', csbi.dwSize.X * csbi.dwSize.Y, c, &len);
        ok(ret, "FillConsoleOutputCharacterW failed\n");
        ok(len == csbi.dwSize.X * csbi.dwSize.Y, "len was: %ld\n", len);

        /* Output u9580 "count" times at (1,0) */
        c.X = 1;
        c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);
        for (n = 0; n < count; ++n)
        {
            ret = WriteConsoleW(hConOut, u9580, lstrlenW(u9580), &len, NULL);
            ok(ret && len == lstrlenW(u9580), "WriteConsoleW failed\n");
        }

        /* Check cursor */
        len = 1 + count;
        c.X = (SHORT)(len % csbi.dwSize.X);
        c.Y = (SHORT)(len / csbi.dwSize.X);
        okCURSOR(hConOut, c);

        /* Fill by ideograph space */
        c.X = c.Y = 0;
        ret = FillConsoleOutputCharacterW(hConOut, ideograph_space, csbi.dwSize.X * csbi.dwSize.Y, c, &len);
        ok(ret, "FillConsoleOutputCharacterW failed\n");
        ok(len == csbi.dwSize.X * csbi.dwSize.Y, "len was: %ld\n", len);

        /* Read characters at (0,0) */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputCharacterW(hConOut, str, 3 * sizeof(WCHAR), c, &len);
        ok(ret, "ReadConsoleOutputCharacterW failed\n");
        ok(len == 6, "len was: %ld\n", len);
        ok(str[0] == ideograph_space || str[0] == L'?', "str[0] was: 0x%04X\n", str[0]);
        ok(str[1] == ideograph_space || str[1] == L'?', "str[1] was: 0x%04X\n", str[1]);
        ok(str[2] == ideograph_space || str[2] == L'?', "str[2] was: 0x%04X\n", str[2]);

        /* Read attr at (0,0) */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, &attr, 1, c, &len);
        ok(ret, "ReadConsoleOutputAttribute failed\n");
        ok(attr == ATTR, "attr was: %d\n", attr);
        ok(len == 1, "len was %ld\n", len);

        /* Read characters at (1,0) */
        c.X = 1;
        c.Y = 0;
        ret = ReadConsoleOutputCharacterW(hConOut, str, 3 * sizeof(WCHAR), c, &len);
        ok(ret, "ReadConsoleOutputCharacterW failed\n");
        ok(len == 6, "len was: %ld\n", len);
        ok(str[0] == ideograph_space || str[0] == L'?', "str[0] was: 0x%04X\n", str[0]);
        ok(str[1] == ideograph_space || str[1] == L'?', "str[1] was: 0x%04X\n", str[1]);
        ok(str[2] == ideograph_space || str[2] == L'?', "str[2] was: 0x%04X\n", str[2]);

        /* Output u9580 "count" once at (1,0) */
        c.X = 1;
        c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);
        ret = WriteConsoleW(hConOut, u9580, lstrlenW(u9580), &len, NULL);
        ok(ret && len == lstrlenW(u9580), "WriteConsoleW failed\n");

        /* Read attr (1,0) */
        c.X = 1;
        c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, &attr, 1, c, &len);
        ok(ret, "ReadConsoleOutputAttribute failed\n");
        ok(attr == ATTR, "attr was: %d\n", attr);
        ok(len == 1, "len was %ld\n", len);

        /* Check cursor */
        c.X = 2;
        c.Y = 0;
        okCURSOR(hConOut, c);

        /* Read characters at (0,0) */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputCharacterW(hConOut, str, 3 * sizeof(WCHAR), c, &len);
        ok(ret, "ReadConsoleOutputCharacterW failed\n");
        ok(len == 6, "len was: %ld\n", len);
        ok(str[0] == ideograph_space || str[0] == L'?', "str[0] was: 0x%04X\n", str[0]);
        ok(str[1] == 0x9580 || str[1] == L'?', "str[1] was: 0x%04X\n", str[1]);
        ok(str[2] == ideograph_space || str[2] == L'?', "str[2] was: 0x%04X\n", str[2]);
    }

    /* Restore code page */
    SetConsoleOutputCP(oldcp);
}

/* Japanese Code Page 932 */
static void test_cp932(HANDLE hConOut)
{
    BOOL ret;
    DWORD oldcp;
    int n;
    DWORD len;
    COORD c;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int count;
    WCHAR str[32];
    WORD attr;

    if (!IsValidCodePage(932))
    {
        skip("Codepage 932 not available\n");
        return;
    }

    /* Set code page */
    oldcp = GetConsoleOutputCP();
    SetLastError(0xdeadbeef);
    ret = SetConsoleOutputCP(932);
    if (!ret)
    {
        skip("SetConsoleOutputCP failed with last error %lu\n", GetLastError());
        return;
    }

    /* Get info */
    ret = GetConsoleScreenBufferInfo(hConOut, &csbi);
    ok(ret, "GetConsoleScreenBufferInfo failed\n");
    trace("csbi.dwSize.X:%d, csbi.dwSize.Y:%d\n", csbi.dwSize.X, csbi.dwSize.Y);
    count = csbi.dwSize.X * 3 / 2;
    trace("count: %d\n", count);

    /* "\u0414" */
    {
        /* Output u0414 "count" times at (0,0) */
        c.X = c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);
        for (n = 0; n < count; ++n)
        {
            ret = WriteConsoleW(hConOut, u0414, lstrlenW(u0414), &len, NULL);
            ok(ret && len == lstrlenW(u0414), "WriteConsoleW failed\n");
        }

        /* Check cursor */
        GetConsoleScreenBufferInfo(hConOut, &csbi);
        len = count * 2;     /* u0414 is fullwidth in Japanese */
        c.X = (SHORT)(len % csbi.dwSize.X);
        c.Y = (SHORT)(len / csbi.dwSize.X);
        okCURSOR(hConOut, c);

        /* Read characters at (0,0) */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputCharacterW(hConOut, str, 3 * sizeof(WCHAR), c, &len);
        ok(ret, "ReadConsoleOutputCharacterW failed\n");
        ok(len == 3, "len was: %ld\n", len);
        ok(str[0] == 0x414, "str[0] was: 0x%04X\n", str[0]);
        ok(str[1] == 0x414, "str[1] was: 0x%04X\n", str[1]);
        ok(str[2] == 0x414, "str[2] was: 0x%04X\n", str[2]);

        /* Check cursor */
        c.X = 1;
        c.Y = 0;
        ret = SetConsoleCursorPosition(hConOut, c);
        ok(ret, "SetConsoleCursorPosition failed\n");
        okCURSOR(hConOut, c);

        /* Fill by space */
        c.X = c.Y = 0;
        FillConsoleOutputCharacterW(hConOut, L' ', csbi.dwSize.X * csbi.dwSize.Y, c, &len);

        /* Output u0414 "count" times at (1,0) */
        c.X = 1;
        c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);
        for (n = 0; n < count; ++n)
        {
            ret = WriteConsoleW(hConOut, u0414, lstrlenW(u0414), &len, NULL);
            ok(ret && len == lstrlenW(u0414), "WriteConsoleW failed\n");
        }

        /* Check cursor */
        len = csbi.dwSize.X + (count - (csbi.dwSize.X - 1) / 2) * 2;
        c.X = (SHORT)(len % csbi.dwSize.X);
        c.Y = (SHORT)(len / csbi.dwSize.X);
        okCURSOR(hConOut, c);

        /* Read characters at (0,0) */
        c.X = 0;
        c.Y = 0;
        ret = ReadConsoleOutputCharacterW(hConOut, str, 3 * sizeof(WCHAR), c, &len);
        ok(ret, "ReadConsoleOutputCharacterW failed\n");
        ok(len == 4, "len was: %ld\n", len);
        ok(str[0] == L' ', "str[0] was: 0x%04X\n", str[0]);
        ok(str[1] == 0x414, "str[1] was: 0x%04X\n", str[1]);
        ok(str[2] == 0x414, "str[2] was: 0x%04X\n", str[2]);
    }

    /* "\u9580" */
    {
        /* Output u9580 "count" times at (0,0) */
        c.X = c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);
        for (n = 0; n < count; ++n)
        {
            ret = WriteConsoleW(hConOut, u9580, lstrlenW(u9580), &len, NULL);
            ok(ret && len == lstrlenW(u9580), "WriteConsoleW failed\n");
        }

        /* Check cursor */
        len = count * 2;     /* u9580 is fullwidth in Japanese */
        c.X = (SHORT)(len % csbi.dwSize.X);
        c.Y = (SHORT)(len / csbi.dwSize.X);
        okCURSOR(hConOut, c);

        /* Check cursor */
        c.X = 1;
        c.Y = 0;
        ret = SetConsoleCursorPosition(hConOut, c);
        ok(ret, "SetConsoleCursorPosition failed\n");
        okCURSOR(hConOut, c);

        /* Fill by space */
        c.X = c.Y = 0;
        ret = FillConsoleOutputCharacterW(hConOut, L' ', csbi.dwSize.X * csbi.dwSize.Y, c, &len);
        ok(ret, "FillConsoleOutputCharacterW failed\n");
        ok(len == csbi.dwSize.X * csbi.dwSize.Y, "len was: %ld\n", len);

        /* Output u9580 "count" times at (1,0) */
        c.X = 1;
        c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);
        for (n = 0; n < count; ++n)
        {
            ret = WriteConsoleW(hConOut, u9580, lstrlenW(u9580), &len, NULL);
            ok(ret && len == lstrlenW(u9580), "WriteConsoleW failed\n");
        }

        /* Check cursor */
        len = csbi.dwSize.X + (count - (csbi.dwSize.X - 1) / 2) * 2;
        c.X = (SHORT)(len % csbi.dwSize.X);
        c.Y = (SHORT)(len / csbi.dwSize.X);
        okCURSOR(hConOut, c);

        /* Fill by ideograph space */
        c.X = c.Y = 0;
        ret = FillConsoleOutputCharacterW(hConOut, ideograph_space, csbi.dwSize.X * csbi.dwSize.Y, c, &len);
        ok(ret, "FillConsoleOutputCharacterW failed\n");
        ok(len == csbi.dwSize.X * csbi.dwSize.Y, "len was: %ld\n", len);

        /* Read characters at (0,0) */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputCharacterW(hConOut, str, 3 * sizeof(WCHAR), c, &len);
        ok(ret, "ReadConsoleOutputCharacterW failed\n");
        ok(len == 3, "len was: %ld\n", len);
        ok(str[0] == ideograph_space, "str[0] was: 0x%04X\n", str[0]);
        ok(str[1] == ideograph_space, "str[1] was: 0x%04X\n", str[1]);
        ok(str[2] == ideograph_space, "str[2] was: 0x%04X\n", str[2]);

        /* Read attr */
        ret = ReadConsoleOutputAttribute(hConOut, &attr, 1, c, &len);
        ok(ret, "ReadConsoleOutputAttribute failed\n");
        ok(attr == ATTR, "attr was: %d\n", attr);
        ok(len == 1, "len was %ld\n", len);

        /* Output u9580 "count" once at (1,0) */
        c.X = 1;
        c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);
        ret = WriteConsoleW(hConOut, u9580, lstrlenW(u9580), &len, NULL);
        ok(ret && len == lstrlenW(u9580), "WriteConsoleW failed\n");

        /* Read attr */
        ret = ReadConsoleOutputAttribute(hConOut, &attr, 1, c, &len);
        ok(ret, "ReadConsoleOutputAttribute failed\n");
        ok(attr == ATTR, "attr was: %d\n", attr);
        ok(len == 1, "len was %ld\n", len);

        /* Check cursor */
        c.X = 3;
        c.Y = 0;
        okCURSOR(hConOut, c);

        /* Read characters */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputCharacterW(hConOut, str, 3 * sizeof(WCHAR), c, &len);
        ok(ret, "ReadConsoleOutputCharacterW failed\n");
        ok(len == 4, "len was: %ld\n", len);
        ok(str[0] == L' ', "str[0] was: 0x%04X\n", str[0]);
        ok(str[1] == 0x9580, "str[1] was: 0x%04X\n", str[1]);
        ok(str[2] == L' ', "str[2] was: 0x%04X\n", str[2]);
    }

    /* Restore code page */
    SetConsoleOutputCP(oldcp);
}

START_TEST(ConsoleCP)
{
    HANDLE hConIn, hConOut;
    FreeConsole();
    ok(AllocConsole(), "Couldn't alloc console\n");

    hConIn = CreateFileA("CONIN$", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    hConOut = CreateFileA("CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hConIn != INVALID_HANDLE_VALUE, "Opening ConIn\n");
    ok(hConOut != INVALID_HANDLE_VALUE, "Opening ConOut\n");

    if (IsValidLocale(lcidRussian, LCID_INSTALLED))
    {
        if (!IsCJKCodePage())
            test_cp855(hConOut);
        else
            skip("Russian testcase is skipped because of CJK\n");
    }
    else
    {
        skip("Russian locale is not installed\n");
    }

    if (IsValidLocale(lcidJapanese, LCID_INSTALLED))
    {
        if (IsCJKCodePage())
            test_cp932(hConOut);
        else
            skip("Japanese testcase is skipped because of not CJK\n");
    }
    else
    {
        skip("Japanese locale is not installed\n");
    }

    CloseHandle(hConIn);
    CloseHandle(hConOut);
    FreeConsole();
    ok(AllocConsole(), "Couldn't alloc console\n");
}
