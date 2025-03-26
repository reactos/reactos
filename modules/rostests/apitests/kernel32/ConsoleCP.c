/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for i18n console.
 * COPYRIGHT:   Copyright 2017-2020 Katayama Hirofumi MZ
 *              Copyright 2020-2022 Hermès Bélusca-Maïto
 */

#include "precomp.h"

#define okCURSOR(hCon, c) \
do { \
  CONSOLE_SCREEN_BUFFER_INFO __sbi; \
  BOOL expect = GetConsoleScreenBufferInfo((hCon), &__sbi) && \
                __sbi.dwCursorPosition.X == (c).X && __sbi.dwCursorPosition.Y == (c).Y; \
  ok(expect, "Expected cursor at (%d,%d), got (%d,%d)\n", \
     (c).X, (c).Y, __sbi.dwCursorPosition.X, __sbi.dwCursorPosition.Y); \
} while (0)

#define ATTR    (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)

static const WCHAR u0414[] = {0x0414, 0}; /* Д */
static const WCHAR u9580[] = {0x9580, 0}; /* 門 */
static const WCHAR space[] = {L' ', 0};
static const WCHAR ideograph_space = (WCHAR)0x3000; /* fullwidth space */
static const WCHAR s_str[] = {L'A', 0x9580, 'B', 0};
static const LCID lcidJapanese = MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT), SORT_DEFAULT);
static const LCID lcidRussian  = MAKELCID(MAKELANGID(LANG_RUSSIAN , SUBLANG_DEFAULT), SORT_DEFAULT);

static UINT s_uOEMCP;
static BOOL s_bIs8Plus;

static BOOL IsCJKCodePage(_In_ UINT CodePage)
{
    switch (CodePage)
    {
    case 932:   // Japanese
    case 949:   // Korean
    case 1361:  // Korean (Johab)
    case 936:   // Chinese PRC
    case 950:   // Taiwan
        return TRUE;
    }
    return FALSE;
}

static __inline
LANGID MapCJKCPToLangId(_In_ UINT CodePage)
{
    switch (CodePage)
    {
    case 932:   // Japanese (Shift-JIS)
        return MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
    case 949:   // Korean (Hangul/Wansung)
    // case 1361:  // Korean (Johab)
        return MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN);
    case 936:   // Chinese PRC (Chinese Simplified)
        return MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
    case 950:   // Taiwan (Chinese Traditional)
        return MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
    default:
        return MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    }
}

static BOOL ChangeOutputCP_(
    _In_ const char* file,
    _In_ int line,
    _In_ UINT CodePage)
{
    BOOL bSuccess;

    /* Validate the code page */
    bSuccess = IsValidCodePage(CodePage);
    if (!bSuccess)
    {
        skip_(file, line)("Code page %d not available\n", CodePage);
        return FALSE;
    }

    /* Set the new code page */
    SetLastError(0xdeadbeef);
    bSuccess = SetConsoleOutputCP(CodePage);
    if (!bSuccess)
        skip_(file, line)("SetConsoleOutputCP(%d) failed with last error %lu\n", CodePage, GetLastError());
    return bSuccess;
}

#define ChangeOutputCP(CodePage) \
    ChangeOutputCP_(__FILE__, __LINE__, CodePage)


#define cmpThreadLangId(file, line, ExpectedLangId) \
do { \
    LANGID ThreadLangId; \
    ThreadLangId = LANGIDFROMLCID(NtCurrentTeb()->CurrentLocale); \
    trace_((file), (line))("Thread LangId %d, expecting %d...\n", \
                           ThreadLangId, (ExpectedLangId)); \
    ok_((file), (line))(ThreadLangId == (ExpectedLangId),   \
                        "Thread LangId %d, expected %d\n",  \
                        ThreadLangId, (ExpectedLangId));    \
} while (0)

static BOOL
doTest_CP_ThreadLang_(
    _In_ const char* file,
    _In_ int line,
    _In_ UINT CodePage,
    _In_ LANGID ExpectedLangId)
{
    UINT newcp;

    /* Verify and set the new code page */
    if (!ChangeOutputCP_(file, line, CodePage))
    {
        skip_(file, line)("Code page %d expected to be valid!\n", CodePage);
        return FALSE;
    }

    newcp = GetConsoleOutputCP();
    ok_(file, line)(newcp == CodePage, "Console output CP is %d, expected %d\n", newcp, CodePage);

    /* Verify that the thread lang ID is the expected one */
    cmpThreadLangId(file, line, ExpectedLangId);
    return TRUE;
}

#define doTest_CP_ThreadLang(...) \
    doTest_CP_ThreadLang_(__FILE__, __LINE__, ##__VA_ARGS__)

static VOID test_CP_ThreadLang(VOID)
{
    /* Save the initial current thread locale. It is (re)initialized after
     * attaching to a console. Don't use GetThreadLocale() as the latter
     * can return a replacement value in case CurrentLocale is 0. */
    LANGID ThreadLangId = LANGIDFROMLCID(NtCurrentTeb()->CurrentLocale);
    UINT oldcp = GetConsoleOutputCP();

    if (IsCJKCodePage(s_uOEMCP))
    {
        /* We are on a CJK system */

        /* Is the console in CJK? If so, current thread should be in CJK language */
        if (!IsCJKCodePage(oldcp))
        {
            skip("CJK system but console CP not in CJK\n");
        }
        else
        {
            /* Check that the thread lang ID matches what the console set */
            LANGID LangId = MapCJKCPToLangId(oldcp);
            cmpThreadLangId(__FILE__, __LINE__, LangId);
        }

        /* Set the code page to OEM USA (non-CJK codepage that is supported).
         * Verify that the thread lang ID has changed to non-CJK language. */
        doTest_CP_ThreadLang(437, MapCJKCPToLangId(437));

        /* Set the code page to the default system CJK codepage.
         * Check that the thread lang ID matches what the console set. */
        doTest_CP_ThreadLang(s_uOEMCP, MapCJKCPToLangId(s_uOEMCP));
    }
    else
    {
        /* We are on a non-CJK system */

        /* Code pages: Japanese, Korean, Chinese Simplified/Traditional */
        UINT CJKCodePages[] = {932, 949, 936, 950};
        UINT newcp;
        USHORT i;

        /* Switch to a different code page (OEM USA) than the current one.
         * In such setup, the current thread lang ID should not change. */
        newcp = (s_uOEMCP == 437 ? 850 : 437);
        doTest_CP_ThreadLang(newcp, ThreadLangId);

        /* Try switching to a CJK codepage, if possible, but
         * the thread lang ID should not change either... */

        /* Retry as long as no valid CJK codepage has been found */
        for (i = 0; i < ARRAYSIZE(CJKCodePages); ++i)
        {
            newcp = CJKCodePages[i];
            if (IsValidCodePage(newcp))
                break; // Found a valid one.
        }
        if (i >= ARRAYSIZE(CJKCodePages))
        {
            /* No valid CJK code pages on the system */
            skip("CJK system but console CP not in CJK\n");
        }
        else
        {
            /* Verify that the thread lang ID remains the same */
            doTest_CP_ThreadLang(newcp, ThreadLangId);
        }
    }

    /* Restore code page */
    SetConsoleOutputCP(oldcp);
}


/* Russian Code Page 855 */
// NOTE that CP 866 can also be used
static void test_cp855(HANDLE hConOut)
{
    BOOL ret;
    UINT oldcp;
    int n;
    DWORD len;
    COORD c;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int count;
    WCHAR str[32];
    WORD attrs[16];

    /* Set code page */
    oldcp = GetConsoleOutputCP();
    if (!ChangeOutputCP(855))
    {
        skip("Codepage 855 not available\n");
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
        ok_long(len, 6);
        ok_int(str[0], 0x414);
        ok_int(str[1], 0x414);
        ok_int(str[2], 0x414);

        /* Read attributes at (0,0) */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 6, c, &len);
        ok(ret, "ReadConsoleOutputAttribute failed\n");
        ok_long(len, 6);
        ok_int(attrs[0], ATTR);

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
        ok_long(len, 6);
        ok_int(str[0], L' ');
        ok_int(str[1], 0x414);
        ok_int(str[2], 0x414);
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
        ok_long(len, csbi.dwSize.X * csbi.dwSize.Y);

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
        ok_long(len, csbi.dwSize.X * csbi.dwSize.Y);

        /* Read characters at (0,0) */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputCharacterW(hConOut, str, 3 * sizeof(WCHAR), c, &len);
        ok(ret, "ReadConsoleOutputCharacterW failed\n");
        ok_long(len, 6);
        ok(str[0] == ideograph_space || str[0] == L'?', "str[0] was: 0x%04X\n", str[0]);
        ok(str[1] == ideograph_space || str[1] == L'?', "str[1] was: 0x%04X\n", str[1]);
        ok(str[2] == ideograph_space || str[2] == L'?', "str[2] was: 0x%04X\n", str[2]);

        /* Read attributes at (0,0) */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 6, c, &len);
        ok(ret, "ReadConsoleOutputAttribute failed\n");
        ok_long(len, 6);
        ok_int(attrs[0], ATTR);

        /* Read characters at (1,0) */
        c.X = 1;
        c.Y = 0;
        ret = ReadConsoleOutputCharacterW(hConOut, str, 3 * sizeof(WCHAR), c, &len);
        ok(ret, "ReadConsoleOutputCharacterW failed\n");
        ok_long(len, 6);
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

        /* Read attributes at (1,0) */
        c.X = 1;
        c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 1, c, &len);
        ok(ret, "ReadConsoleOutputAttribute failed\n");
        ok_long(len, 1);
        ok_int(attrs[0], ATTR);

        /* Check cursor */
        c.X = 2;
        c.Y = 0;
        okCURSOR(hConOut, c);

        /* Read characters at (0,0) */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputCharacterW(hConOut, str, 3 * sizeof(WCHAR), c, &len);
        ok(ret, "ReadConsoleOutputCharacterW failed\n");
        ok_long(len, 6);
        ok(str[0] == ideograph_space || str[0] == L'?', "str[0] was: 0x%04X\n", str[0]);
        ok(str[1] == u9580[0] || str[1] == L'?', "str[1] was: 0x%04X\n", str[1]);
        ok(str[2] == ideograph_space || str[2] == L'?', "str[2] was: 0x%04X\n", str[2]);
    }

    /* Restore code page */
    SetConsoleOutputCP(oldcp);
}

/* Japanese Code Page 932 */
static void test_cp932(HANDLE hConOut)
{
    BOOL ret;
    UINT oldcp;
    int n;
    DWORD len;
    COORD c, buffSize;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int count;
    WCHAR str[32];
    WORD attrs[16];
    CHAR_INFO buff[16];
    SMALL_RECT sr;

    /* Set code page */
    oldcp = GetConsoleOutputCP();
    if (!ChangeOutputCP(932))
    {
        skip("Codepage 932 not available\n");
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
        ok_long(len, 3);
        ok_int(str[0], 0x414);
        ok_int(str[1], 0x414);
        ok_int(str[2], 0x414);

        /* Read attributes at (0,0) */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 6, c, &len);
        ok(ret, "ReadConsoleOutputAttribute failed\n");
        ok_long(len, 6);
        ok_int(attrs[0], ATTR | COMMON_LVB_LEADING_BYTE);
        ok_int(attrs[1], ATTR | COMMON_LVB_TRAILING_BYTE);

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
        ok_long(len, 4);
        ok_int(str[0], L' ');
        ok_int(str[1], 0x414);
        ok_int(str[2], 0x414);
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
        ok_long(len, csbi.dwSize.X * csbi.dwSize.Y);

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
            ok_long(len, csbi.dwSize.X * csbi.dwSize.Y / 2);
        else
            ok_long(len, csbi.dwSize.X * csbi.dwSize.Y);

        /* Read characters at (0,0) */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputCharacterW(hConOut, str, 3 * sizeof(WCHAR), c, &len);
        ok(ret, "ReadConsoleOutputCharacterW failed\n");
        ok_long(len, 3);
        ok_int(str[0], ideograph_space);
        ok_int(str[1], ideograph_space);
        ok_int(str[2], ideograph_space);

        /* Read attributes at (0,0) */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 6, c, &len);
        ok(ret, "ReadConsoleOutputAttribute failed\n");
        ok_long(len, 6);
        ok_int(attrs[0], ATTR | COMMON_LVB_LEADING_BYTE);
        ok_int(attrs[1], ATTR | COMMON_LVB_TRAILING_BYTE);
        ok_int(attrs[2], ATTR | COMMON_LVB_LEADING_BYTE);
        ok_int(attrs[3], ATTR | COMMON_LVB_TRAILING_BYTE);
        ok_int(attrs[4], ATTR | COMMON_LVB_LEADING_BYTE);
        ok_int(attrs[5], ATTR | COMMON_LVB_TRAILING_BYTE);

        /* Output u9580 "count" once at (1,0) */
        c.X = 1;
        c.Y = 0;
        SetConsoleCursorPosition(hConOut, c);
        okCURSOR(hConOut, c);
        ret = WriteConsoleW(hConOut, u9580, lstrlenW(u9580), &len, NULL);
        ok(ret && len == lstrlenW(u9580), "WriteConsoleW failed\n");

        /*
         * Read attributes at (1,0) -
         * Note that if only one attribute of a fullwidth character
         * is retrieved, no leading/trailing byte flag is set!
         */
        c.X = 1;
        c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 1, c, &len);
        ok(ret, "ReadConsoleOutputAttribute failed\n");
        ok_long(len, 1);
        ok_int(attrs[0], ATTR);

        /* Check that the same problem happens for the trailing byte */
        c.X = 2;
        c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 1, c, &len);
        ok(ret, "ReadConsoleOutputAttribute failed\n");
        ok_long(len, 1);
        ok_int(attrs[0], ATTR);

        /* Read attributes at (1,0) */
        c.X = 1;
        c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 2, c, &len);
        ok(ret, "ReadConsoleOutputAttribute failed\n");
        ok_long(len, 2);
        ok_int(attrs[0], ATTR | COMMON_LVB_LEADING_BYTE);
        ok_int(attrs[1], ATTR | COMMON_LVB_TRAILING_BYTE);

        /* Read attributes at (1,0) */
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 3, c, &len);
        ok(ret, "ReadConsoleOutputAttribute failed\n");
        ok_long(len, 3);
        ok_int(attrs[0], ATTR | COMMON_LVB_LEADING_BYTE);
        ok_int(attrs[1], ATTR | COMMON_LVB_TRAILING_BYTE);
        if (s_bIs8Plus)
            ok_int(attrs[2], ATTR | COMMON_LVB_TRAILING_BYTE);
        else
            ok_int(attrs[2], ATTR);

        /* Read attributes at (0,0) */
        c.X = c.Y = 0;
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 4, c, &len);
        ok(ret, "ReadConsoleOutputAttribute failed\n");
        ok_long(len, 4);
        if (s_bIs8Plus)
            ok_int(attrs[0], ATTR | COMMON_LVB_LEADING_BYTE);
        else
            ok_int(attrs[0], ATTR);
        ok_int(attrs[1], ATTR | COMMON_LVB_LEADING_BYTE);
        ok_int(attrs[2], ATTR | COMMON_LVB_TRAILING_BYTE);
        if (s_bIs8Plus)
            ok_int(attrs[3], ATTR | COMMON_LVB_TRAILING_BYTE);
        else
            ok_int(attrs[3], ATTR);

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
            ok_long(len, 3);
            ok_int(str[0], ideograph_space);
            ok_int(str[1], u9580[0]);
            ok_int(str[2], ideograph_space);
        }
        else
        {
            ok_long(len, 4);
            ok_int(str[0], L' ');
            ok_int(str[1], u9580[0]);
            ok_int(str[2], L' ');
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
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 1, c, &len);
        ok_int(ret, 1);
        ok_long(len, 1);
        ok_int(attrs[0], ATTR);

        /* read char */
        c.X = c.Y = 0;
        memset(str, 0x7F, sizeof(str));
        ret = ReadConsoleOutputCharacterW(hConOut, str, 4, c, &len);
        ok_int(ret, 1);
        ok_long(len, 4);
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
        ok_long(len, 3);
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
        ok_long(len, 3);
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
        ok_long(len, 2);
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
        ok_int(buff[0].Char.UnicodeChar, L'A');
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
            ok_int(buff[2].Char.UnicodeChar, L'A');
            ok_int(buff[2].Attributes, ATTR);
        }
        ok_int(buff[3].Char.UnicodeChar, L'A');
        ok_int(buff[3].Attributes, ATTR);
        ok_int(buff[4].Char.UnicodeChar, L'A');
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
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 1, c, &len);
        ok_int(ret, 1);
        ok_long(len, 1);
        ok_int(attrs[0], ATTR);

        /* read char */
        memset(str, 0x7F, sizeof(str));
        ret = ReadConsoleOutputCharacterW(hConOut, str, 2, c, &len);
        ok_int(ret, 1);
        ok_long(len, 2);
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
            ok_int(buff[0].Char.UnicodeChar, u9580[0]);
            ok_int(buff[0].Attributes, ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(buff[1].Char.UnicodeChar, u9580[0]);
            ok_int(buff[1].Attributes, ATTR | COMMON_LVB_TRAILING_BYTE);
        }
        else
        {
            ok_int(buff[0].Char.UnicodeChar, u9580[0]);
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
        ok_long(len, 3);
        ok_int(str[0], u9580[0]);
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
            ok_int(buff[0].Char.UnicodeChar, u9580[0]);
            ok_int(buff[0].Attributes, ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(buff[1].Char.UnicodeChar, u9580[0]);
            ok_int(buff[1].Attributes, ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(buff[2].Char.UnicodeChar, u9580[0]);
            ok_int(buff[2].Attributes, ATTR | COMMON_LVB_TRAILING_BYTE);
        }
        else
        {
            ok_int(buff[0].Char.UnicodeChar, L' ');
            ok_int(buff[0].Attributes, ATTR);
            ok_int(buff[1].Char.UnicodeChar, u9580[0]);
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
        ok_int(buff[0].Char.UnicodeChar, L'A');
        ok_int(buff[0].Attributes, ATTR);
        ok_int(buff[1].Char.UnicodeChar, L'A');
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
        ok_long(len, 2);
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
        ret = FillConsoleOutputCharacterW(hConOut, u9580[0], 1, c, &len);
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
        ok_int(buff[0].Char.UnicodeChar, L'A');
        ok_int(buff[0].Attributes, ATTR);
        if (s_bIs8Plus)
        {
            ok_int(buff[1].Char.UnicodeChar, u9580[0]);
            ok_int(buff[1].Attributes, ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(buff[2].Char.UnicodeChar, u9580[0]);
            ok_int(buff[2].Attributes, ATTR | COMMON_LVB_TRAILING_BYTE);
        }
        else
        {
            ok_int(buff[1].Char.UnicodeChar, L' ');
            ok_int(buff[1].Attributes, ATTR);
            ok_int(buff[2].Char.UnicodeChar, L'A');
            ok_int(buff[2].Attributes, ATTR);
        }
        ok_int(buff[3].Char.UnicodeChar, L'A');
        ok_int(buff[3].Attributes, ATTR);
        ok_int(buff[4].Char.UnicodeChar, L'A');
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
        // Check how Read/WriteConsoleOutput() handle inconsistent DBCS flags.
        buff[0].Char.UnicodeChar = u9580[0];
        buff[0].Attributes = ATTR | COMMON_LVB_LEADING_BYTE;
        buff[1].Char.UnicodeChar = u9580[0];
        buff[1].Attributes = ATTR | COMMON_LVB_LEADING_BYTE;
        buff[2].Char.UnicodeChar = u9580[0];
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

        /* read output */
        sr.Left = 0;
        sr.Top = 0;
        sr.Right = 4;
        sr.Bottom = 0;
        memset(buff, 0x7F, sizeof(buff));
        ret = ReadConsoleOutputW(hConOut, buff, buffSize, c, &sr);
        ok_int(ret, 1);
        ok_int(sr.Left, 0);
        ok_int(sr.Top, 0);
        ok_int(sr.Right, 3);
        ok_int(sr.Bottom, 0);

        /* check buff */
        if (s_bIs8Plus)
        {
            ok_int(buff[0].Char.UnicodeChar, u9580[0]);
            ok_int(buff[0].Attributes, ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(buff[1].Char.UnicodeChar, u9580[0]);
            ok_int(buff[1].Attributes, ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(buff[2].Char.UnicodeChar, u9580[0]);
            ok_int(buff[2].Attributes, ATTR | COMMON_LVB_TRAILING_BYTE);
            ok_int(buff[3].Char.UnicodeChar, L'A');
            ok_int(buff[3].Attributes, ATTR);
            ok_int(buff[4].Char.UnicodeChar, 0x7F7F);
            ok_int(buff[4].Attributes, 0x7F7F);
        }
        else
        {
            ok_int(buff[0].Char.UnicodeChar, u9580[0]);
            ok_int(buff[0].Attributes, ATTR);
            ok_int(buff[1].Char.UnicodeChar, u9580[0]);
            ok_int(buff[1].Attributes, ATTR);
            ok_int(buff[2].Char.UnicodeChar, 0);
            ok_int(buff[2].Attributes, 0);
            ok_int(buff[3].Char.UnicodeChar, 0);
            ok_int(buff[3].Attributes, 0);
            ok_int(buff[4].Char.UnicodeChar, 0x7F7F);
            ok_int(buff[4].Attributes, 0x7F7F);
        }

        /* read attrs */
        c.X = c.Y = 0;
        memset(attrs, 0x7F, sizeof(attrs));
        ret = ReadConsoleOutputAttribute(hConOut, attrs, 6, c, &len);
        ok_int(ret, 1);
        ok_long(len, 6);
        ok_int(attrs[0], ATTR | COMMON_LVB_LEADING_BYTE);
        if (s_bIs8Plus)
        {
            ok_int(attrs[1], ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(attrs[2], ATTR | COMMON_LVB_TRAILING_BYTE);
            ok_int(attrs[3], ATTR);
        }
        else
        {
            ok_int(attrs[1], ATTR | COMMON_LVB_TRAILING_BYTE);
            ok_int(attrs[2], ATTR | COMMON_LVB_LEADING_BYTE);
            ok_int(attrs[3], ATTR | COMMON_LVB_TRAILING_BYTE);
        }
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

    /* Retrieve the system OEM code page */
    s_uOEMCP = GetOEMCP();
    trace("Running on %s system (codepage %d)\n",
          IsCJKCodePage(s_uOEMCP) ? "CJK" : "Non-CJK",
          s_uOEMCP);

    /* Test thread lang ID syncing with console code page */
    test_CP_ThreadLang();

    if (IsValidLocale(lcidRussian, LCID_INSTALLED))
    {
        if (!IsCJKCodePage(s_uOEMCP))
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
        if (IsCJKCodePage(s_uOEMCP))
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
