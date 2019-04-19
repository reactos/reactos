/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CSimpleMap
 * PROGRAMMER:      Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#ifdef HAVE_APITEST
    #include <apitest.h>
#else
    #include <stdlib.h>
    #include <stdio.h>
    #include <stdarg.h>
    int g_tests_executed = 0;
    int g_tests_failed = 0;
    void ok_func(const char *file, int line, bool value, const char *fmt, ...)
    {
        va_list va;
        va_start(va, fmt);
        if (!value)
        {
            printf("%s (%d): ", file, line);
            vprintf(fmt, va);
            g_tests_failed++;
        }
        g_tests_executed++;
        va_end(va);
    }
    #undef ok
    #define ok(value, ...)  ok_func(__FILE__, __LINE__, value, __VA_ARGS__)
    #define START_TEST(x)   int main(void)
#endif

#include <atlbase.h>
#include <atlsimpcoll.h>

struct CMonster
{
    static int s_nCount;
    static int s_nCopyCount;

    CMonster()
    {
        CMonster::s_nCount++;
    }
    CMonster(const CMonster& c)
    {
        CMonster::s_nCount++;
    }
    ~CMonster()
    {
        CMonster::s_nCount--;
    }
    CMonster& operator=(const CMonster& other)
    {
        CMonster::s_nCopyCount++;
        return *this;
    }
};

int CMonster::s_nCount = 0;
int CMonster::s_nCopyCount = 0;

START_TEST(CSimpleMap)
{
    CSimpleMap<int, int> map1;

    ok(map1.GetSize() == 0, "Expected map1's size is zero, was %d\n", map1.GetSize());

    map1.Add(1, 2);
    ok(map1.GetSize() == 1, "Expected map1's size is 1, was %d\n", map1.GetSize());
    map1.Add(2, 3);
    ok(map1.GetSize() == 2, "Expected map1's size is 2, was %d\n", map1.GetSize());

    ok(map1.Lookup(1) == 2, "Expected map1.Lookup(1) is 2, was %d\n", map1.Lookup(1));
    ok(map1.Lookup(2) == 3, "Expected map1.Lookup(2) is 3, was %d\n", map1.Lookup(2));
    ok(map1.Lookup(-1) == 0, "Expected map1.Lookup(-1) is 0, was %d\n", map1.Lookup(-1));

    ok(map1.ReverseLookup(2) == 1, "Expected map1.ReverseLookup(2) is 1, was %d\n", map1.ReverseLookup(2));
    ok(map1.ReverseLookup(3) == 2, "Expected map1.ReverseLookup(3) is 2, was %d\n", map1.ReverseLookup(3));

    ok(map1.GetKeyAt(0) == 1, "Expected map1.GetKeyAt(0) is 1, was %d\n", map1.GetKeyAt(0));
    ok(map1.GetKeyAt(1) == 2, "Expected map1.GetKeyAt(1) is 2, was %d\n", map1.GetKeyAt(1));

    ok(map1.GetValueAt(0) == 2, "Expected map1.GetValueAt(0) is 2, was %d\n", map1.GetValueAt(0));
    ok(map1.GetValueAt(1) == 3, "Expected map1.GetValueAt(1) is 3, was %d\n", map1.GetValueAt(1));

    map1.SetAt(2, 4);

    ok(map1.Lookup(1) == 2, "Expected map1.Lookup(1) is 2, was %d\n", map1.Lookup(1));
    ok(map1.Lookup(2) == 4, "Expected map1.Lookup(2) is 4, was %d\n", map1.Lookup(2));

    ok(map1.ReverseLookup(2) == 1, "Expected map1.ReverseLookup(2) is 1, was %d\n", map1.ReverseLookup(2));
    ok(map1.ReverseLookup(4) == 2, "Expected map1.ReverseLookup(4) is 2, was %d\n", map1.ReverseLookup(4));

    map1.Remove(1);
    ok(map1.GetSize() == 1, "Expected map1's size is 1, was %d\n", map1.GetSize());
    map1.Remove(2);
    ok(map1.GetSize() == 0, "Expected map1's size is 0, was %d\n", map1.GetSize());

    map1.Add(1, 4);
    ok(map1.GetSize() == 1, "Expected map1's size is 1, was %d\n", map1.GetSize());
    map1.Add(2, 8);
    ok(map1.GetSize() == 2, "Expected map1's size is 2, was %d\n", map1.GetSize());
    map1.Add(3, 12);
    ok(map1.GetSize() == 3, "Expected map1's size is 3, was %d\n", map1.GetSize());

    map1.RemoveAll();
    ok(map1.GetSize() == 0, "Expected map1's size is 0, was %d\n", map1.GetSize());

    ok(CMonster::s_nCount == 0, "Expected CMonster::s_nCount is 0, was %d\n", CMonster::s_nCount);
    ok(CMonster::s_nCopyCount == 0, "Expected CMonster::s_nCopyCount is 0, was %d\n", CMonster::s_nCopyCount);

    CSimpleMap<CMonster, CMonster> map2;
    ok(map2.GetSize() == 0, "Expected map2's size is zero, was %d\n", map2.GetSize());

    ok(CMonster::s_nCount == 0, "Expected CMonster::s_nCount is 0, was %d\n", CMonster::s_nCount);
    ok(CMonster::s_nCopyCount == 0, "Expected CMonster::s_nCopyCount is 0, was %d\n", CMonster::s_nCopyCount);

    {
        CMonster m1;
        ok(CMonster::s_nCount == 1, "Expected CMonster::s_nCount is 1, was %d\n", CMonster::s_nCount);
        ok(CMonster::s_nCopyCount == 0, "Expected CMonster::s_nCopyCount is 0, was %d\n", CMonster::s_nCopyCount);

        CMonster m2;
        ok(CMonster::s_nCount == 2, "Expected CMonster::s_nCount is 2, was %d\n", CMonster::s_nCount);
        ok(CMonster::s_nCopyCount == 0, "Expected CMonster::s_nCopyCount is 0, was %d\n", CMonster::s_nCopyCount);

        map2.Add(m1, m2);
        ok(CMonster::s_nCount == 4, "Expected CMonster::s_nCount is 4, was %d\n", CMonster::s_nCount);
        ok(CMonster::s_nCopyCount == 0, "Expected CMonster::s_nCopyCount is 0, was %d\n", CMonster::s_nCopyCount);
    }

    ok(map2.GetSize() == 1, "Expected map2's size is 1, was %d\n", map2.GetSize());
    ok(CMonster::s_nCount == 2, "Expected CMonster::s_nCount is 2, was %d\n", CMonster::s_nCount);
    ok(CMonster::s_nCopyCount == 0, "Expected CMonster::s_nCopyCount is 0, was %d\n", CMonster::s_nCopyCount);

    {
        CMonster m1;
        ok(CMonster::s_nCount == 3, "Expected CMonster::s_nCount is 3, was %d\n", CMonster::s_nCount);
        ok(CMonster::s_nCopyCount == 0, "Expected CMonster::s_nCopyCount is 0, was %d\n", CMonster::s_nCopyCount);

        CMonster m2;
        ok(CMonster::s_nCount == 4, "Expected CMonster::s_nCount is 4, was %d\n", CMonster::s_nCount);
        ok(CMonster::s_nCopyCount == 0, "Expected CMonster::s_nCopyCount is 0, was %d\n", CMonster::s_nCopyCount);

        map2.Add(m1, m2);
        ok(CMonster::s_nCount == 6, "Expected CMonster::s_nCount is 6, was %d\n", CMonster::s_nCount);
        ok(CMonster::s_nCopyCount == 0, "Expected CMonster::s_nCopyCount is 0, was %d\n", CMonster::s_nCopyCount);
    }

    ok(map2.GetSize() == 2, "Expected map2's size is 2, was %d\n", map2.GetSize());
    ok(CMonster::s_nCount == 4, "Expected CMonster::s_nCount is 4, was %d\n", CMonster::s_nCount);
    ok(CMonster::s_nCopyCount == 0, "Expected CMonster::s_nCopyCount is 0, was %d\n", CMonster::s_nCopyCount);

    map2.RemoveAt(0);
    ok(CMonster::s_nCount == 2, "Expected CMonster::s_nCount is 2, was %d\n", CMonster::s_nCount);
    ok(map2.GetSize() == 1, "Expected map2's size is 1, was %d\n", map2.GetSize());
    ok(CMonster::s_nCopyCount == 0, "Expected CMonster::s_nCopyCount is 0, was %d\n", CMonster::s_nCopyCount);

    map2.RemoveAt(0);
    ok(CMonster::s_nCount == 0, "Expected CMonster::s_nCount is 0, was %d\n", CMonster::s_nCount);
    ok(map2.GetSize() == 0, "Expected map2's size is 0, was %d\n", map2.GetSize());
    ok(CMonster::s_nCopyCount == 0, "Expected CMonster::s_nCopyCount is 0, was %d\n", CMonster::s_nCopyCount);

    CSimpleMap<int, CMonster> map3;
    ok(map3.GetSize() == 0, "Expected map3's size is 0, was %d\n", map3.GetSize());

    CMonster m3;
    ok(CMonster::s_nCount == 1, "Expected CMonster::s_nCount is 1, was %d\n", CMonster::s_nCount);

    map3.Add(1, m3);
    ok(map3.GetSize() == 1, "Expected map3's size is 1, was %d\n", map3.GetSize());
    ok(CMonster::s_nCount == 2, "Expected CMonster::s_nCount is 2, was %d\n", CMonster::s_nCount);

    map3.Add(2, m3);
    ok(map3.GetSize() == 2, "Expected map3's size is 2, was %d\n", map3.GetSize());
    ok(CMonster::s_nCount == 3, "Expected CMonster::s_nCount is 3, was %d\n", CMonster::s_nCount);

    map3.Add(3, m3);
    ok(map3.GetSize() == 3, "Expected map3's size is 3, was %d\n", map3.GetSize());
    ok(CMonster::s_nCount == 4, "Expected CMonster::s_nCount is 4, was %d\n", CMonster::s_nCount);

    map3.Remove(2);
    ok(map3.GetSize() == 2, "Expected map3's size is 2, was %d\n", map3.GetSize());
    ok(CMonster::s_nCount == 3, "Expected CMonster::s_nCount is 3, was %d\n", CMonster::s_nCount);

    map3.RemoveAll();
    ok(map3.GetSize() == 0, "Expected map3's size is 0, was %d\n", map3.GetSize());
    ok(CMonster::s_nCount == 1, "Expected CMonster::s_nCount is 1, was %d\n", CMonster::s_nCount);

    map1.Add(1, 2);
    ok(map1.GetSize() == 1, "Expected map1's size is 1, was %d\n", map1.GetSize());
    map1.Add(2, 3);
    ok(map1.GetSize() == 2, "Expected map1's size is 2, was %d\n", map1.GetSize());

    ok(!!map1.RemoveAt(0), "Expected RemoveAt(0) to succeed\n");
    ok(map1.GetSize() == 1, "Expected map1's size is 1, was %d\n", map1.GetSize());
    ok(!!map1.RemoveAt(0), "Expected RemoveAt(0) to succeed\n");
    ok(map1.GetSize() == 0, "Expected map1's size is 0, was %d\n", map1.GetSize());

#ifndef HAVE_APITEST
    printf("CSimpleMap: %i tests executed (0 marked as todo, %i failures), 0 skipped.\n", g_tests_executed, g_tests_failed);
    return g_tests_failed;
#endif
}
