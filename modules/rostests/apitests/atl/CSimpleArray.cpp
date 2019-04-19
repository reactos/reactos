/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CSimpleArray
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

struct CCreature
{
    static int s_nCount;
    static int s_nCopyCount;
    CCreature()
    {
        CCreature::s_nCount++;
    }
    CCreature(const CCreature& c)
    {
        CCreature::s_nCount++;
    }
    ~CCreature()
    {
        CCreature::s_nCount--;
    }
    CCreature& operator=(const CCreature& other)
    {
        CCreature::s_nCopyCount++;
        return *this;
    }
};

int CCreature::s_nCount = 0;
int CCreature::s_nCopyCount = 0;


START_TEST(CSimpleArray)
{
    CSimpleArray<int> array1;

    ok(array1.GetSize() == 0, "Expected array1's size is zero, was %d\n", array1.GetSize());

    array1.Add(123);

    ok(array1.GetSize() == 1, "Expected array1's size is 1, was %d\n", array1.GetSize());
    ok(array1.GetData()[0] == 123, "Expected array1.GetData()[0] is 123, was %d\n", array1.GetData()[0]);
    ok(array1[0] == 123, "Expected array1[0] is 123, was %d\n", array1[0]);

    array1.Add(456);

    ok(array1.GetSize() == 2, "Expected array1's size is 2, was %d\n", array1.GetSize());
    ok(array1.GetData()[0] == 123, "Expected array1.GetData()[0] is 123, was %d\n", array1.GetData()[0]);
    ok(array1[0] == 123, "Expected array1[0] is 123, was %d\n", array1[0]);
    ok(array1.GetData()[1] == 456, "Expected array1.GetData()[1] is 456, was %d\n", array1.GetData()[1]);
    ok(array1[1] == 456, "Expected array1[1] is 456, was %d\n", array1[1]);

    array1.RemoveAll();
    ok(array1.GetSize() == 0, "Expected array1's size is 0, was %d\n", array1.GetSize());

    array1.Add(1);
    array1.Add(1);
    array1.Add(1);
    array1.Add(2);
    array1.Add(2);
    array1.Add(3);
    ok(array1.GetSize() == 6, "Expected array1's size is 6, was %d\n", array1.GetSize());

    array1.Remove(2);
    ok(array1.GetSize() == 5, "Expected array1's size is 5, was %d\n", array1.GetSize());

    array1.Remove(1);
    ok(array1.GetSize() == 4, "Expected array1's size is 4, was %d\n", array1.GetSize());

    ok(array1[0] == 1, "Expected array1[0] is 1, was %d\n", array1[0]);
    ok(array1[1] == 1, "Expected array1[1] is 1, was %d\n", array1[1]);
    ok(array1[2] == 2, "Expected array1[2] is 2, was %d\n", array1[2]);
    ok(array1[3] == 3, "Expected array1[3] is 3, was %d\n", array1[3]);

    ok(CCreature::s_nCount == 0, "Expected CCreature::s_nCount is zero, was: %d\n", CCreature::s_nCount);
    ok(CCreature::s_nCopyCount == 0, "Expected CCreature::s_nCopyCount is zero, was: %d\n", CCreature::s_nCopyCount);

    CSimpleArray<CCreature> array2;
    {
        CCreature creature1, creature2;

        ok(CCreature::s_nCount == 2, "Expected CCreature::s_nCount is 2, was: %d\n", CCreature::s_nCount);
        ok(CCreature::s_nCopyCount == 0, "Expected CCreature::s_nCopyCount is zero, was: %d\n", CCreature::s_nCopyCount);
        array2.Add(creature1);
        ok(CCreature::s_nCount == 3, "Expected CCreature::s_nCount is 3, was: %d\n", CCreature::s_nCount);
        ok(CCreature::s_nCopyCount == 0, "Expected CCreature::s_nCopyCount is zero, was: %d\n", CCreature::s_nCopyCount);
        array2.Add(creature2);
        ok(CCreature::s_nCount == 4, "Expected CCreature::s_nCount is 4, was: %d\n", CCreature::s_nCount);
        ok(CCreature::s_nCopyCount == 0, "Expected CCreature::s_nCopyCount is zero, was: %d\n", CCreature::s_nCopyCount);
    }
    ok(CCreature::s_nCount == 2, "Expected CCreature::s_nCount is 2, was: %d\n", CCreature::s_nCount);
    ok(CCreature::s_nCopyCount == 0, "Expected CCreature::s_nCopyCount is zero, was: %d\n", CCreature::s_nCopyCount);

    {
        CSimpleArray<CCreature> array3(array2), array4, array5;
        ok(CCreature::s_nCount == 4, "Expected CCreature::s_nCount is 4, was: %d\n", CCreature::s_nCount);
        ok(CCreature::s_nCopyCount == 0, "Expected CCreature::s_nCopyCount is zero, was: %d\n", CCreature::s_nCopyCount);

        array4 = array2;
        ok(CCreature::s_nCount == 6, "Expected CCreature::s_nCount is 6, was: %d\n", CCreature::s_nCount);
        ok(CCreature::s_nCopyCount == 0, "Expected CCreature::s_nCopyCount is zero, was: %d\n", CCreature::s_nCopyCount);

        CCreature creature1;
        ok(CCreature::s_nCount == 7, "Expected CCreature::s_nCount is 7, was: %d\n", CCreature::s_nCount);
        ok(CCreature::s_nCopyCount == 0, "Expected CCreature::s_nCopyCount is zero, was: %d\n", CCreature::s_nCopyCount);

        array4.Add(creature1);
        ok(CCreature::s_nCount == 8, "Expected CCreature::s_nCount is 8, was: %d\n", CCreature::s_nCount);
        ok(CCreature::s_nCopyCount == 0, "Expected CCreature::s_nCopyCount is zero, was: %d\n", CCreature::s_nCopyCount);

        array3 = array4;
        ok(CCreature::s_nCount == 9, "Expected CCreature::s_nCount is 9, was: %d\n", CCreature::s_nCount);
        ok(CCreature::s_nCopyCount == 0, "Expected CCreature::s_nCopyCount is zero, was: %d\n", CCreature::s_nCopyCount);

        array5 = array2;
        ok(CCreature::s_nCount == 11, "Expected CCreature::s_nCount is 11, was: %d\n", CCreature::s_nCount);
        ok(CCreature::s_nCopyCount == 0, "Expected CCreature::s_nCopyCount is zero, was: %d\n", CCreature::s_nCopyCount);

        array5 = array2;
        ok(CCreature::s_nCount == 11, "Expected CCreature::s_nCount is 11, was: %d\n", CCreature::s_nCount);
        ok(CCreature::s_nCopyCount == 0, "Expected CCreature::s_nCopyCount is zero, was: %d\n", CCreature::s_nCopyCount);
    }
    ok(CCreature::s_nCount == 2, "Expected CCreature::s_nCount is 2, was: %d\n", CCreature::s_nCount);
    ok(CCreature::s_nCopyCount == 0, "Expected CCreature::s_nCopyCount is zero, was: %d\n", CCreature::s_nCopyCount);

    array2.RemoveAll();
    ok(CCreature::s_nCount == 0, "Expected CCreature::s_nCount is zero, was: %d\n", CCreature::s_nCount);
    ok(CCreature::s_nCopyCount == 0, "Expected CCreature::s_nCopyCount is zero, was: %d\n", CCreature::s_nCopyCount);

    array1.RemoveAll();
    ok(array1.GetSize() == 0, "Expected array1.GetSize() is zero, was: %d\n", array1.GetSize());
    for (int i = 0; i < 100; ++i)
    {
        array1.Add(i);
    }
    ok(array1.GetSize() == 100, "Expected array1.GetSize() is 100, was: %d\n", array1.GetSize());

    array1.RemoveAll();
    ok(array1.GetSize() == 0, "Expected array1.GetSize() is zero, was: %d\n", array1.GetSize());
    array1.Add(123);
    array1.Add(321);
    ok(!!array1.RemoveAt(0), "Expected RemoveAt(0) to succeed\n");
    ok(array1.GetSize() == 1, "Expected array1.GetSize() is 1, was: %d\n", array1.GetSize());
    if (array1.GetSize() == 1)
    {
        ok(array1[0] == 321, "Expected array1[0] is 321, was %d\n", array1[0]);
    }
    ok(!!array1.RemoveAt(0), "Expected RemoveAt(0) to succeed\n");
    ok(array1.GetSize() == 0, "Expected array1.GetSize() is 0, was: %d\n", array1.GetSize());

#ifndef HAVE_APITEST
    printf("CSimpleArray: %i tests executed (0 marked as todo, %i failures), 0 skipped.\n", g_tests_executed, g_tests_failed);
    return g_tests_failed;
#endif
}
