/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CString
 * PROGRAMMER:      Mark Jansen
 */

#include <atlstr.h>
#include <apitest.h>


struct traits_test
{
    const char* strA;
    const wchar_t* strW;
    int str_len;
    int exp_1, exp_2, exp_3, exp_4;
};

traits_test g_Tests[] = {
    // inputs                   outputs
    { NULL, NULL, 0,            0,  0, -1,  0 },
    { NULL, NULL, -1,           0, -1, -1,  0 },
    { NULL, NULL, 1,            0,  1, -1,  0 },

    { "", L"", 0,               0,  0,  0,  0 },
    { "", L"", -1,              0, -1,  0,  1 },
    { "", L"", 1,               0,  1,  0,  1 },

    { "AAABBB", L"AAABBB", 0,   6,  0,  6,  0 },
    { "AAABBB", L"AAABBB", 3,   6,  3,  6,  3 },
    { "AAABBB", L"AAABBB", -1,  6, -1,  6,  7 },
};

static void test_basetypes()
{
    int len;
    char bufA[10];
    wchar_t bufW[10];

    for (size_t n = 0; n < _countof(g_Tests); ++n)
    {
        len = ChTraitsCRT<char>::GetBaseTypeLength(g_Tests[n].strA);
        ok(len == g_Tests[n].exp_1, "Expected len to be %i, was %i for %u (A)\n", g_Tests[n].exp_1, len, n);

        len = ChTraitsCRT<char>::GetBaseTypeLength(g_Tests[n].strA, g_Tests[n].str_len);
        ok(len == g_Tests[n].exp_2, "Expected len to be %i, was %i for %u (A,len)\n", g_Tests[n].exp_2, len, n);

        len = ChTraitsCRT<char>::GetBaseTypeLength(g_Tests[n].strW);
        ok(len == g_Tests[n].exp_3, "Expected len to be %i, was %i for %u (W)\n", g_Tests[n].exp_3, len, n);

        len = ChTraitsCRT<char>::GetBaseTypeLength(g_Tests[n].strW, g_Tests[n].str_len);
        ok(len == g_Tests[n].exp_4, "Expected len to be %i, was %i for %u (W,len)\n", g_Tests[n].exp_4, len, n);

        if (g_Tests[n].strA && g_Tests[n].strW)
        {
            memset(bufA, 'x', sizeof(bufA));
            ChTraitsCRT<char>::ConvertToBaseType(bufA, g_Tests[n].exp_1+1, g_Tests[n].strA);
            char ch = bufA[g_Tests[n].exp_1];
            ok(ch == '\0', "Expected %i to be \\0, was: %c (%i) for %u\n", g_Tests[n].exp_1, ch, (int)ch, n);
            ok(!strcmp(bufA, g_Tests[n].strA), "Expected bufA to be %s, was: %s for %u\n", g_Tests[n].strA, bufA, n);
            ch = bufA[g_Tests[n].exp_1+1];
            ok(ch == 'x', "Expected %i to be 'x', was: %c (%i) for %u\n", g_Tests[n].exp_1+1, ch, (int)ch, n);
        }

        if (g_Tests[n].strA && g_Tests[n].strW)
        {
            memset(bufA, 'x', sizeof(bufA));
            ChTraitsCRT<char>::ConvertToBaseType(bufA, g_Tests[n].exp_1+1, g_Tests[n].strW);
            char ch = bufA[g_Tests[n].exp_1];
            ok(ch == '\0', "Expected %i to be \\0, was: %c (%i) for %u\n", g_Tests[n].exp_1, ch, (int)ch, n);
            ok(!strcmp(bufA, g_Tests[n].strA), "Expected bufA to be %s, was: %s for %u\n", g_Tests[n].strA, bufA, n);
            ch = bufA[g_Tests[n].exp_1+1];
            ok(ch == 'x', "Expected %i to be 'x', was: %c (%i) for %u\n", g_Tests[n].exp_1+1, ch, (int)ch, n);
        }

        // wchar_t --> please note, swapped the expectations from 2 and 4 !
        len = ChTraitsCRT<wchar_t>::GetBaseTypeLength(g_Tests[n].strA);
        ok(len == g_Tests[n].exp_1, "Expected len to be %i, was %i for %u (A)\n", g_Tests[n].exp_1, len, n);

        len = ChTraitsCRT<wchar_t>::GetBaseTypeLength(g_Tests[n].strA, g_Tests[n].str_len);
        ok(len == g_Tests[n].exp_4, "Expected len to be %i, was %i for %u (A,len)\n", g_Tests[n].exp_4, len, n);

        len = ChTraitsCRT<wchar_t>::GetBaseTypeLength(g_Tests[n].strW);
        ok(len == g_Tests[n].exp_3, "Expected len to be %i, was %i for %u (W)\n", g_Tests[n].exp_3, len, n);

        len = ChTraitsCRT<wchar_t>::GetBaseTypeLength(g_Tests[n].strW, g_Tests[n].str_len);
        ok(len == g_Tests[n].exp_2, "Expected len to be %i, was %i for %u (W,len)\n", g_Tests[n].exp_2, len, n);

        if (g_Tests[n].strA && g_Tests[n].strW)
        {
            memset(bufW, 'x', sizeof(bufW));
            ChTraitsCRT<wchar_t>::ConvertToBaseType(bufW, g_Tests[n].exp_1+1, g_Tests[n].strA);
            wchar_t ch = bufW[g_Tests[n].exp_1];
            ok(ch == L'\0', "Expected %i to be \\0, was: %c (%i) for %u\n", g_Tests[n].exp_1, ch, (int)ch, n);
            ok(!wcscmp(bufW, g_Tests[n].strW), "Expected bufW to be %s, was: %s for %u\n", wine_dbgstr_w(g_Tests[n].strW), wine_dbgstr_w(bufW), n);
            ch = bufW[g_Tests[n].exp_1+1];
            ok(ch == 30840, "Expected %i to be %i for %u\n", g_Tests[n].exp_1+1, (int)ch, n);
        }

        if (g_Tests[n].strA && g_Tests[n].strW)
        {
            memset(bufW, 'x', sizeof(bufW));
            ChTraitsCRT<wchar_t>::ConvertToBaseType(bufW, g_Tests[n].exp_1+1, g_Tests[n].strW);
            wchar_t ch = bufW[g_Tests[n].exp_1];
            ok(ch == '\0', "Expected %i to be \\0, was: %c (%i) for %u\n", g_Tests[n].exp_1, ch, (int)ch, n);
            ok(!wcscmp(bufW, g_Tests[n].strW), "Expected bufW to be %s, was: %s for %u\n", wine_dbgstr_w(g_Tests[n].strW), wine_dbgstr_w(bufW), n);
            ch = bufW[g_Tests[n].exp_1+1];
            ok(ch == 30840, "Expected %i to be %i for %u\n", g_Tests[n].exp_1+1, (int)ch, n);
        }
    }
}

// Allocation strategy seems to differ a bit between us and MS's atl.
// if someone cares enough to find out why, feel free to change the macro below.
#define ALLOC_EXPECT(a, b)  b


#undef ok
#undef _T

#define TEST_NAMEX(name)        void test_##name##W()
#define CStringX                CStringW
#define _X(x)                   L ## x
#define XCHAR                   WCHAR
#define dbgstrx(x)              wine_dbgstr_w(x)
#define ok                      ok_("CStringW:\n" __FILE__, __LINE__)
#define GetWindowsDirectoryX    GetWindowsDirectoryW
#include "CString.inl"


#undef CStringX
#undef TEST_NAMEX
#undef _X
#undef XCHAR
#undef dbgstrx
#undef ok
#undef GetWindowsDirectoryX

#define TEST_NAMEX(name)        void test_##name##A()
#define CStringX                CStringA
#define _X(x)                   x
#define XCHAR                   CHAR
#define dbgstrx(x)              (const char*)x
#define ok                      ok_("CStringA:\n" __FILE__, __LINE__)
#define GetWindowsDirectoryX    GetWindowsDirectoryA
#include "CString.inl"


START_TEST(CString)
{
    test_basetypes();

    if ((ALLOC_EXPECT(1, 2)) == 2)
    {
        skip("Ignoring real GetAllocLength() lenght\n");
    }

    test_operators_initW();
    test_operators_initA();

    test_compareW();
    test_compareA();

    test_findW();
    test_findA();

    test_formatW();
    test_formatA();

    test_substrW();
    test_substrA();

    test_replaceW();
    test_replaceA();

    test_trimW();
    test_trimA();

    test_envW();
    test_envA();
}
