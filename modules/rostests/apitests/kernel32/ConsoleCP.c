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

static const WCHAR u0414[] = {0x0414, 0}; /* Д */
static const WCHAR u9580[] = {0x9580, 0}; /* 門 */
static const WCHAR space[] = {L' ', 0};
static const WCHAR ideograph_space = (WCHAR)0x3000; /* fullwidth space */
static const WCHAR s_str[] = {L'A', 0x9580, 'B', 0};
static LCID lcidJapanese = MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT), SORT_DEFAULT);
static LCID lcidRussian  = MAKELCID(MAKELANGID(LANG_RUSSIAN , SUBLANG_DEFAULT), SORT_DEFAULT);
static BOOL s_bIs8Plus;

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
    count = 200;

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
    COORD c, buffSize;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int count;
    WCHAR str[32];
    WORD attr, attrs[16];
    CHAR_INFO buff[16];
    SMALL_RECT sr;

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
    count = 200;

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
        if (s_bIs8Plus)
            ok(len == csbi.dwSize.X * csbi.dwSize.Y / 2, "len was: %ld\n", len);
        else
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
        if (s_bIs8Plus)
        {
            ok(len == 3, "len was: %ld\n", len);
            ok(str[0] == 0x3000, "str[0] was: 0x%04X\n", str[0]);
            ok(str[1] == 0x9580, "str[1] was: 0x%04X\n", str[1]);
            ok(str[2] == 0x3000, "str[2] was: 0x%04X\n", str[2]);
        }
        else
        {
            ok(len == 4, "len was: %ld\n", len);
            ok(str[0] == L' ', "str[0] was: 0x%04X\n", str[0]);
            ok(str[1] == 0x9580, "str[1] was: 0x%04X\n", str[1]);
            ok(str[2] == L' ', "str[2] was: 0x%04X\n", str[2]);
        }
    }

    /* COMMON_LVB_LEADING_BYTE and COMMON_LVB_TRAILING_BYTE for u0414 */
    {
        /* set cursor */
        c.X = c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);

        /* fill by 'A' */
        ret = FillConsoleOutputCharacterW(hConOut, L'A', csbi.dwSize.X * 2, c, &len);
        ok_int(ret, 1);
        ok_long(len, csbi.dwSize.X * 2);

        /* reset buff */
        buffSize.X = ARRAYSIZE(buff);
        buffSize.Y = 1;
        memset(buff, 0x7F, sizeof(buff));

        /* read output */
        c.X = c.Y = 0;
        ZeroMemory(&sr, sizeof(sr));
        sr.Right = buffSize.X - 1;
        ret = ReadConsoleOutputW(hConOut, buff, buffSize, c, &sr);
        ok_int(ret, 1);
        ok_int(sr.Left, 0);
        ok_int(sr.Top, 0);
        ok_int(sr.Right, buffSize.X - 1);
        ok_int(sr.Bottom, 0);

        /* check buff */
        ok_int(buff[0].Char.UnicodeChar, L'A');
        ok_int(buff[0].Attributes, ATTR);

        /* read attr */
        ret = ReadConsoleOutputAttribute(hConOut, &attr, 1, c, &len);
        ok_int(ret, 1);
        ok_int(attr, ATTR);
        ok_long(len, 1);

        /* read char */
        c.X = c.Y = 0;
        memset(str, 0x7F, sizeof(str));
        ret = ReadConsoleOutputCharacterW(hConOut, str, 4, c, &len);
        ok_int(ret, 1);
        ok_int(str[0], L'A');
        ok_int(str[1], L'A');
        ok_int(str[2], L'A');
        ok_int(str[3], L'A');

        /* set cursor */
        c.X = c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);

        /* write u0414 */
        ret = WriteConsoleW(hConOut, u0414, 1, &len, NULL);
        ok_int(ret, 1);
        ok_long(len, 1);

        /* reset buff */
        buffSize.X = ARRAYSIZE(buff);
        buffSize.Y = 1;
        memset(buff, 0x7F, sizeof(buff));

        /* read output */
        c.X = c.Y = 0;
        ZeroMemory(&sr, sizeof(sr));
        sr.Right = buffSize.X - 1;
        ret = ReadConsoleOutputW(hConOut, buff, buffSize, c, &sr);
        ok_int(ret, 1);
        ok_int(sr.Left, 0);
        ok_int(sr.Top, 0);
        ok_int(sr.Right, buffSize.X - 1);
        ok_int(sr.Bottom, 0);

        /* check buff */
        if (s_bIs8Plus)
        {
            ok_int(buff[0].Char.UnicodeChar, 0x0414);
            ok_int(buff[0].Attributes, ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(buff[1].Char.UnicodeChar, 0x0414);
            ok_int(buff[1].Attributes, ATTR | COMMON_LVB_TRAILING_BYTE);
        }
        else
        {
            ok_int(buff[0].Char.UnicodeChar, 0x0414);
            ok_int(buff[0].Attributes, ATTR);
            ok_int(buff[1].Char.UnicodeChar, L'A');
            ok_int(buff[1].Attributes, ATTR);
        }
        ok_int(buff[2].Char.UnicodeChar, L'A');
        ok_int(buff[2].Attributes, ATTR);
        ok_int(buff[3].Char.UnicodeChar, L'A');
        ok_int(buff[3].Attributes, ATTR);

        /* read attr */
        ret = ReadConsoleOutputAttribute(hConOut, attrs, ARRAYSIZE(attrs), c, &len);
        ok_int(ret, 1);
        ok_long(len, ARRAYSIZE(attrs));
        ok_int(attrs[0], ATTR | COMMON_LVB_LEADING_BYTE);
        ok_int(attrs[1], ATTR | COMMON_LVB_TRAILING_BYTE);
        ok_int(attrs[2], ATTR);
        ok_int(attrs[3], ATTR);

        /* read char */
        c.X = c.Y = 0;
        memset(str, 0x7F, sizeof(str));
        ret = ReadConsoleOutputCharacterW(hConOut, str, 4, c, &len);
        ok_int(ret, 1);
        ok_int(str[0], 0x0414);
        ok_int(str[1], L'A');
        ok_int(str[2], L'A');
        if (s_bIs8Plus)
            ok_int(str[3], 0);
        else
            ok_int(str[3], 0x7F7F);

        /* set cursor */
        c.X = 1;
        c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);

        /* write u0414 */
        ret = WriteConsoleW(hConOut, u0414, 1, &len, NULL);
        ok_int(ret, 1);
        ok_long(len, 1);

        /* read output */
        c.X = c.Y = 0;
        ZeroMemory(&sr, sizeof(sr));
        sr.Right = buffSize.X - 1;
        ret = ReadConsoleOutputW(hConOut, buff, buffSize, c, &sr);
        ok_int(ret, 1);
        ok_int(sr.Left, 0);
        ok_int(sr.Top, 0);
        ok_int(sr.Right, buffSize.X - 1);
        ok_int(sr.Bottom, 0);

        /* check buff */
        if (s_bIs8Plus)
        {
            ok_int(buff[0].Char.UnicodeChar, 0x0414);
            ok_int(buff[0].Attributes, ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(buff[1].Char.UnicodeChar, 0x0414);
            ok_int(buff[1].Attributes, ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(buff[2].Char.UnicodeChar, 0x0414);
            ok_int(buff[2].Attributes, ATTR | COMMON_LVB_TRAILING_BYTE);
        }
        else
        {
            ok_int(buff[0].Char.UnicodeChar, L' ');
            ok_int(buff[0].Attributes, ATTR);
            ok_int(buff[1].Char.UnicodeChar, 0x0414);
            ok_int(buff[1].Attributes, ATTR);
            ok_int(buff[2].Char.UnicodeChar, L'A');
            ok_int(buff[2].Attributes, ATTR);
        }
        ok_int(buff[3].Char.UnicodeChar, L'A');
        ok_int(buff[3].Attributes, ATTR);

        /* read attr */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, attrs, ARRAYSIZE(attrs), c, &len);
        ok_int(ret, 1);
        ok_long(len, ARRAYSIZE(attrs));
        if (s_bIs8Plus)
            ok_int(attrs[0], ATTR | COMMON_LVB_LEADING_BYTE);
        else
            ok_int(attrs[0], ATTR);
        ok_int(attrs[1], ATTR | COMMON_LVB_LEADING_BYTE);
        ok_int(attrs[2], ATTR | COMMON_LVB_TRAILING_BYTE);
        ok_int(attrs[3], ATTR);

        /* read char */
        c.X = c.Y = 0;
        memset(str, 0x7F, sizeof(str));
        ret = ReadConsoleOutputCharacterW(hConOut, str, 4, c, &len);
        ok_int(ret, 1);
        if (s_bIs8Plus)
        {
            ok_int(str[0], 0x0414);
            ok_int(str[1], 0x0414);
            ok_int(str[2], L'A');
            ok_int(str[3], 0);
        }
        else
        {
            ok_int(str[0], L' ');
            ok_int(str[1], 0x0414);
            ok_int(str[2], L'A');
            ok_int(str[3], 0x7F7F);
        }

        /* set cursor */
        c.X = csbi.dwSize.X - 1;
        c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);

        /* write u0414 */
        WriteConsoleW(hConOut, u0414, 1, &len, NULL);
        ok_int(ret, 1);
        ok_long(len, 1);

        /* reset buff */
        buffSize.X = ARRAYSIZE(buff);
        buffSize.Y = 1;
        memset(buff, 0x7F, sizeof(buff));

        /* read output */
        c.X = c.Y = 0;
        sr.Left = csbi.dwSize.X - 2;
        sr.Top = 0;
        sr.Right = csbi.dwSize.X - 1;
        sr.Bottom = 0;
        ret = ReadConsoleOutputW(hConOut, buff, buffSize, c, &sr);
        ok_int(ret, 1);
        ok_int(sr.Left, csbi.dwSize.X - 2);
        ok_int(sr.Top, 0);
        ok_int(sr.Right, csbi.dwSize.X - 1);
        ok_int(sr.Bottom, 0);

        /* check buff */
        ok_int(buff[0].Char.UnicodeChar, L'A');
        ok_int(buff[0].Attributes, ATTR);
        ok_int(buff[1].Char.UnicodeChar, L'A');
        ok_int(buff[1].Attributes, ATTR);

        /* read attrs */
        c.X = csbi.dwSize.X - 2;
        c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, attrs, ARRAYSIZE(attrs), c, &len);
        ok_int(ret, 1);
        ok_long(len, ARRAYSIZE(attrs));
        ok_int(attrs[0], ATTR);
        ok_int(attrs[1], ATTR);

        /* read char */
        ret = ReadConsoleOutputCharacterW(hConOut, str, 2, c, &len);
        ok_int(ret, 1);
        ok_int(str[0], L'A');
        ok_int(str[1], L'A');

        /* fill by 'A' */
        c.X = c.Y = 0;
        ret = FillConsoleOutputCharacterW(hConOut, L'A', 10, c, &len);
        ok_int(ret, 1);
        ok_long(len, 10);

        /* fill by u0414 */
        c.X = 1;
        c.Y = 0;
        ret = FillConsoleOutputCharacterW(hConOut, 0x0414, 1, c, &len);
        c.X = c.Y = 0;
        ok_int(ret, 1);
        ok_long(len, 1);

        /* reset buff */
        buffSize.X = ARRAYSIZE(buff);
        buffSize.Y = 1;
        memset(buff, 0x7F, sizeof(buff));

        /* read output */
        c.X = c.Y = 0;
        sr.Left = 0;
        sr.Top = 0;
        sr.Right = 4;
        sr.Bottom = 0;
        ret = ReadConsoleOutputW(hConOut, buff, buffSize, c, &sr);
        ok_int(ret, 1);
        ok_int(sr.Left, 0);
        ok_int(sr.Top, 0);
        ok_int(sr.Right, 4);
        ok_int(sr.Bottom, 0);

        /* check buff */
        ok_int(buff[0].Char.UnicodeChar, 'A');
        ok_int(buff[0].Attributes, ATTR);
        if (s_bIs8Plus)
        {
            ok_int(buff[1].Char.UnicodeChar, 0x0414);
            ok_int(buff[1].Attributes, ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(buff[2].Char.UnicodeChar, 0x0414);
            ok_int(buff[2].Attributes, ATTR | COMMON_LVB_TRAILING_BYTE);
        }
        else
        {
            ok_int(buff[1].Char.UnicodeChar, L' ');
            ok_int(buff[1].Attributes, ATTR);
            ok_int(buff[2].Char.UnicodeChar, 'A');
            ok_int(buff[2].Attributes, ATTR);
        }
        ok_int(buff[3].Char.UnicodeChar, 'A');
        ok_int(buff[3].Attributes, ATTR);
        ok_int(buff[4].Char.UnicodeChar, 'A');
        ok_int(buff[4].Attributes, ATTR);

        /* read attrs */
        c.X = 0;
        c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 4, c, &len);
        ok_int(ret, 1);
        ok_long(len, 4);
        ok_int(attrs[0], ATTR);
        if (s_bIs8Plus)
        {
            ok_int(attrs[1], ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(attrs[2], ATTR | COMMON_LVB_TRAILING_BYTE);
        }
        else
        {
            ok_int(attrs[1], ATTR);
            ok_int(attrs[2], ATTR);
        }
        ok_int(attrs[3], ATTR);
    }

    /* COMMON_LVB_LEADING_BYTE and COMMON_LVB_TRAILING_BYTE for u9580 */
    {
        /* set cursor */
        c.X = c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);

        /* fill by 'A' */
        ret = FillConsoleOutputCharacterW(hConOut, L'A', csbi.dwSize.X * 2, c, &len);
        ok_int(ret, 1);
        ok_long(len, csbi.dwSize.X * 2);

        /* reset buff */
        buffSize.X = ARRAYSIZE(buff);
        buffSize.Y = 1;
        memset(buff, 0x7F, sizeof(buff));

        /* read output */
        c.X = c.Y = 0;
        ZeroMemory(&sr, sizeof(sr));
        sr.Right = buffSize.X - 1;
        ret = ReadConsoleOutputW(hConOut, buff, buffSize, c, &sr);
        ok_int(ret, 1);
        ok_int(sr.Left, 0);
        ok_int(sr.Top, 0);
        ok_int(sr.Right, buffSize.X - 1);
        ok_int(sr.Bottom, 0);

        /* check buff */
        ok_int(buff[0].Char.UnicodeChar, L'A');
        ok_int(buff[0].Attributes, ATTR);

        /* read attr */
        ret = ReadConsoleOutputAttribute(hConOut, &attr, 1, c, &len);
        ok_int(ret, 1);
        ok_int(attr, ATTR);
        ok_long(len, 1);

        /* read char */
        memset(str, 0x7F, sizeof(str));
        ret = ReadConsoleOutputCharacterW(hConOut, str, 2, c, &len);
        ok_int(ret, 1);
        ok_int(str[0], L'A');
        ok_int(str[1], L'A');

        /* set cursor */
        c.X = 0;
        c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);

        /* write u9580 */
        ret = WriteConsoleW(hConOut, u9580, 1, &len, NULL);
        ok_int(ret, 1);
        ok_long(len, 1);

        /* reset buff */
        buffSize.X = ARRAYSIZE(buff);
        buffSize.Y = 1;
        memset(buff, 0x7F, sizeof(buff));

        /* read output */
        c.X = c.Y = 0;
        ZeroMemory(&sr, sizeof(sr));
        sr.Right = buffSize.X - 1;
        ret = ReadConsoleOutputW(hConOut, buff, buffSize, c, &sr);
        ok_int(ret, 1);
        ok_int(sr.Left, 0);
        ok_int(sr.Top, 0);
        ok_int(sr.Right, buffSize.X - 1);
        ok_int(sr.Bottom, 0);

        /* check buff */
        if (s_bIs8Plus)
        {
            ok_int(buff[0].Char.UnicodeChar, 0x9580);
            ok_int(buff[0].Attributes, ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(buff[1].Char.UnicodeChar, 0x9580);
            ok_int(buff[1].Attributes, ATTR | COMMON_LVB_TRAILING_BYTE);
        }
        else
        {
            ok_int(buff[0].Char.UnicodeChar, 0x9580);
            ok_int(buff[0].Attributes, ATTR);
            ok_int(buff[1].Char.UnicodeChar, L'A');
            ok_int(buff[1].Attributes, ATTR);
        }
        ok_int(buff[2].Char.UnicodeChar, L'A');
        ok_int(buff[2].Attributes, ATTR);
        ok_int(buff[3].Char.UnicodeChar, L'A');
        ok_int(buff[3].Attributes, ATTR);

        /* read attr */
        ret = ReadConsoleOutputAttribute(hConOut, attrs, ARRAYSIZE(attrs), c, &len);
        ok_int(ret, 1);
        ok_long(len, ARRAYSIZE(attrs));

        ok_int(attrs[0], ATTR | COMMON_LVB_LEADING_BYTE);
        ok_int(attrs[1], ATTR | COMMON_LVB_TRAILING_BYTE);
        ok_int(attrs[2], ATTR);
        ok_int(attrs[3], ATTR);

        /* read char */
        c.X = c.Y = 0;
        memset(str, 0x7F, sizeof(str));
        ret = ReadConsoleOutputCharacterW(hConOut, str, 4, c, &len);
        ok_int(ret, 1);
        ok_int(str[0], 0x9580);
        ok_int(str[1], L'A');
        ok_int(str[2], L'A');
        if (s_bIs8Plus)
            ok_int(str[3], 0);
        else
            ok_int(str[3], 0x7F7F);

        /* set cursor */
        c.X = 1;
        c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);

        /* write u9580 */
        ret = WriteConsoleW(hConOut, u9580, 1, &len, NULL);
        ok_int(ret, 1);
        ok_long(len, 1);

        /* read output */
        c.X = c.Y = 0;
        ZeroMemory(&sr, sizeof(sr));
        sr.Right = buffSize.X - 1;
        ret = ReadConsoleOutputW(hConOut, buff, buffSize, c, &sr);
        ok_int(ret, 1);
        ok_int(sr.Left, 0);
        ok_int(sr.Top, 0);
        ok_int(sr.Right, buffSize.X - 1);
        ok_int(sr.Bottom, 0);

        /* check buff */
        if (s_bIs8Plus)
        {
            ok_int(buff[0].Char.UnicodeChar, 0x9580);
            ok_int(buff[0].Attributes, ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(buff[1].Char.UnicodeChar, 0x9580);
            ok_int(buff[1].Attributes, ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(buff[2].Char.UnicodeChar, 0x9580);
            ok_int(buff[2].Attributes, ATTR | COMMON_LVB_TRAILING_BYTE);
        }
        else
        {
            ok_int(buff[0].Char.UnicodeChar, L' ');
            ok_int(buff[0].Attributes, ATTR);
            ok_int(buff[1].Char.UnicodeChar, 0x9580);
            ok_int(buff[1].Attributes, ATTR);
            ok_int(buff[2].Char.UnicodeChar, L'A');
            ok_int(buff[2].Attributes, ATTR);
        }
        ok_int(buff[3].Char.UnicodeChar, L'A');
        ok_int(buff[3].Attributes, ATTR);

        /* read attr */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, attrs, ARRAYSIZE(attrs), c, &len);
        ok_int(ret, 1);
        ok_long(len, ARRAYSIZE(attrs));

        if (s_bIs8Plus)
        {
            ok_int(attrs[0], ATTR | COMMON_LVB_LEADING_BYTE);
        }
        else
        {
            ok_int(attrs[0], ATTR);
        }
        ok_int(attrs[1], ATTR | COMMON_LVB_LEADING_BYTE);
        ok_int(attrs[2], ATTR | COMMON_LVB_TRAILING_BYTE);
        ok_int(attrs[3], ATTR);

        /* set cursor */
        c.X = csbi.dwSize.X - 1;
        c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);

        /* write u9580 */
        WriteConsoleW(hConOut, u9580, 1, &len, NULL);
        ok_int(ret, 1);
        ok_long(len, 1);

        /* reset buff */
        buffSize.X = ARRAYSIZE(buff);
        buffSize.Y = 1;
        memset(buff, 0x7F, sizeof(buff));

        /* read output */
        c.X = c.Y = 0;
        sr.Left = csbi.dwSize.X - 2;
        sr.Top = 0;
        sr.Right = csbi.dwSize.X - 1;
        sr.Bottom = 0;
        ret = ReadConsoleOutputW(hConOut, buff, buffSize, c, &sr);
        ok_int(ret, 1);
        ok_int(sr.Left, csbi.dwSize.X - 2);
        ok_int(sr.Top, 0);
        ok_int(sr.Right, csbi.dwSize.X - 1);
        ok_int(sr.Bottom, 0);

        /* check buff */
        ok_int(buff[0].Char.UnicodeChar, 'A');
        ok_int(buff[0].Attributes, ATTR);
        ok_int(buff[1].Char.UnicodeChar, 'A');
        ok_int(buff[1].Attributes, ATTR);

        /* read attr */
        c.X = csbi.dwSize.X - 2;
        c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, attrs, ARRAYSIZE(attrs), c, &len);
        ok_int(ret, 1);
        ok_long(len, ARRAYSIZE(attrs));
        ok_int(attrs[0], ATTR);
        ok_int(attrs[1], ATTR);

        /* read char */
        memset(str, 0x7F, sizeof(str));
        ret = ReadConsoleOutputCharacterW(hConOut, str, 2, c, &len);
        ok_int(ret, 1);
        ok_int(str[0], L'A');
        ok_int(str[1], L'A');

        /* fill by 'A' */
        c.X = c.Y = 0;
        ret = FillConsoleOutputCharacterW(hConOut, L'A', 10, c, &len);
        ok_int(ret, 1);
        ok_long(len, 10);

        /* fill by u9580 */
        c.X = 1;
        c.Y = 0;
        ret = FillConsoleOutputCharacterW(hConOut, 0x9580, 1, c, &len);
        c.X = c.Y = 0;
        ok_int(ret, 1);
        ok_long(len, 1);

        /* reset buff */
        buffSize.X = ARRAYSIZE(buff);
        buffSize.Y = 1;
        memset(buff, 0x7F, sizeof(buff));

        /* read output */
        c.X = c.Y = 0;
        sr.Left = 0;
        sr.Top = 0;
        sr.Right = 4;
        sr.Bottom = 0;
        ret = ReadConsoleOutputW(hConOut, buff, buffSize, c, &sr);
        ok_int(ret, 1);
        ok_int(sr.Left, 0);
        ok_int(sr.Top, 0);
        ok_int(sr.Right, 4);
        ok_int(sr.Bottom, 0);

        /* check buff */
        ok_int(buff[0].Char.UnicodeChar, 'A');
        ok_int(buff[0].Attributes, ATTR);
        if (s_bIs8Plus)
        {
            ok_int(buff[1].Char.UnicodeChar, 0x9580);
            ok_int(buff[1].Attributes, ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(buff[2].Char.UnicodeChar, 0x9580);
            ok_int(buff[2].Attributes, ATTR | COMMON_LVB_TRAILING_BYTE);
        }
        else
        {
            ok_int(buff[1].Char.UnicodeChar, L' ');
            ok_int(buff[1].Attributes, ATTR);
            ok_int(buff[2].Char.UnicodeChar, 'A');
            ok_int(buff[2].Attributes, ATTR);
        }
        ok_int(buff[3].Char.UnicodeChar, 'A');
        ok_int(buff[3].Attributes, ATTR);
        ok_int(buff[4].Char.UnicodeChar, 'A');
        ok_int(buff[4].Attributes, ATTR);

        /* read attrs */
        c.X = 0;
        c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 4, c, &len);
        ok_int(ret, 1);
        ok_long(len, 4);
        ok_int(attrs[0], ATTR);
        if (s_bIs8Plus)
        {
            ok_int(attrs[1], ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(attrs[2], ATTR | COMMON_LVB_TRAILING_BYTE);
        }
        else
        {
            ok_int(attrs[1], ATTR);
            ok_int(attrs[2], ATTR);
        }
        ok_int(attrs[3], ATTR);
    }

    /* FillConsoleOutputAttribute and WriteConsoleOutput */
    {
        c.X = c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);
        for (n = 0; n < count; ++n)
        {
            ret = WriteConsoleW(hConOut, space, lstrlenW(space), &len, NULL);
            ok_int(ret, 1);
            ok_long(len, 1);
        }

        /* fill attrs */
        c.X = c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);
        ret = FillConsoleOutputAttribute(hConOut, 0xFFFF, 2, c, &len);
        ok_int(ret, 1);
        ok_long(len, 2);

        /* read attrs */
        memset(attrs, 0x7F, sizeof(attrs));
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 3, c, &len);
        ok_int(ret, 1);
        ok_long(len, 3);
        if (s_bIs8Plus)
        {
            ok_int(attrs[0], 0xDCFF);
            ok_int(attrs[1], 0xDCFF);
        }
        else
        {
            ok_int(attrs[0], 0xFCFF);
            ok_int(attrs[1], 0xFCFF);
        }
        ok_int(attrs[2], ATTR);

        /* fill attrs */
        c.X = c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);
        ret = FillConsoleOutputAttribute(hConOut, ATTR, 4, c, &len);
        ok_int(ret, 1);
        ok_long(len, 4);

        /* write */
        c.X = c.Y = 0;
        sr.Left = 0;
        sr.Top = 0;
        sr.Right = 4;
        sr.Bottom = 0;
        buff[0].Char.UnicodeChar = L' ';
        buff[0].Attributes = ATTR;
        buff[1].Char.UnicodeChar = 0x9580;
        buff[1].Attributes = ATTR | COMMON_LVB_LEADING_BYTE;
        buff[2].Char.UnicodeChar = 0x9580;
        buff[2].Attributes = ATTR | COMMON_LVB_TRAILING_BYTE;
        buff[3].Char.UnicodeChar = L'A';
        buff[3].Attributes = ATTR;
        buff[4].Char.UnicodeChar = L' ';
        buff[4].Attributes = 0xFFFF;
        buffSize.X = 4;
        buffSize.Y = 1;
        ret = WriteConsoleOutputW(hConOut, buff, buffSize, c, &sr);
        ok_int(ret, 1);
        ok_int(sr.Left, 0);
        ok_int(sr.Top, 0);
        ok_int(sr.Right, 3);
        ok_int(sr.Bottom, 0);

        /* read attrs */
        memset(attrs, 0x7F, sizeof(attrs));
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 6, c, &len);
        ok_int(ret, 1);
        ok_long(len, 6);
        ok_int(attrs[0], ATTR);
        ok_int(attrs[1], ATTR | COMMON_LVB_LEADING_BYTE);
        ok_int(attrs[2], ATTR | COMMON_LVB_TRAILING_BYTE);
        ok_int(attrs[3], ATTR);
        ok_int(attrs[4], ATTR);
        ok_int(attrs[5], ATTR);
    }

    /* WriteConsoleOutputCharacterW and WriteConsoleOutputAttribute */
    {
        c.X = c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);
        for (n = 0; n < count; ++n)
        {
            ret = WriteConsoleW(hConOut, space, lstrlenW(space), &len, NULL);
            ok_int(ret, 1);
            ok_long(len, 1);
        }

        /* write attrs */
        attrs[0] = ATTR;
        attrs[1] = 0xFFFF;
        attrs[2] = ATTR;
        attrs[3] = 0;
        ret = WriteConsoleOutputAttribute(hConOut, attrs, 4, c, &len);
        ok_int(ret, 1);
        ok_long(len, 4);

        /* read attrs */
        memset(attrs, 0x7F, sizeof(attrs));
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 4, c, &len);
        ok_int(ret, 1);
        ok_long(len, 4);
        ok_int(attrs[0], ATTR);
        if (s_bIs8Plus)
            ok_int(attrs[1], 0xDCFF);
        else
            ok_int(attrs[1], 0xFCFF);
        ok_int(attrs[2], ATTR);
        ok_int(attrs[3], 0);

        /* fill attr */
        ret = FillConsoleOutputAttribute(hConOut, ATTR, 4, c, &len);
        ok_int(ret, 1);
        ok_long(len, 4);

        /* write char */
        ret = WriteConsoleOutputCharacterW(hConOut, s_str, 4, c, &len);
        ok_int(ret, 1);
        ok_long(len, 4);

        /* read attrs */
        memset(attrs, 0x7F, sizeof(attrs));
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 4, c, &len);
        ok_int(ret, 1);
        ok_long(len, 4);
        ok_int(attrs[0], ATTR);
        ok_int(attrs[1], ATTR | COMMON_LVB_LEADING_BYTE);
        ok_int(attrs[2], ATTR | COMMON_LVB_TRAILING_BYTE);
        ok_int(attrs[3], ATTR);
    }

    /* Restore code page */
    SetConsoleOutputCP(oldcp);
}

START_TEST(ConsoleCP)
{
    HANDLE hConIn, hConOut;
    OSVERSIONINFOA osver = { sizeof(osver) };

    // https://github.com/reactos/reactos/pull/2131#issuecomment-563189380
    GetVersionExA(&osver);
    s_bIs8Plus = (osver.dwMajorVersion > 6) ||
                 (osver.dwMajorVersion == 6 && osver.dwMinorVersion >= 2);

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
