/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for CAtlArray
 * COPYRIGHT:   Copyright 2016-2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 *              Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
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
#include <atlcoll.h>

struct CCreature
{
    static int s_nCtorCount;
    static int s_nCtorCount_Default;
    static int s_nCCtorCount;
    static int s_nDtorCount;
    static int s_nOpIsCount;

    int m_id;

    CCreature(int id = 0x123456)
        :m_id(id)
    {
        CCreature::s_nCtorCount++;
        if (id == 0x123456)
            s_nCtorCount_Default++;
    }
    CCreature(const CCreature& c)
        :m_id(c.m_id)
    {
        CCreature::s_nCCtorCount++;
    }
    ~CCreature()
    {
        CCreature::s_nDtorCount++;
    }
    CCreature& operator=(const CCreature& other)
    {
        m_id = other.m_id;
        CCreature::s_nOpIsCount++;
        return *this;
    }
};

int CCreature::s_nCtorCount = 0;
int CCreature::s_nCtorCount_Default = 0;
int CCreature::s_nCCtorCount = 0;
int CCreature::s_nDtorCount = 0;
int CCreature::s_nOpIsCount = 0;


START_TEST(CAtlArray)
{
    {
        CAtlArray<CCreature> array1;

        ok(CCreature::s_nCtorCount == 0, "Expected CCreature::s_nCtorCount is zero, was: %d\n", CCreature::s_nCtorCount);
        ok(CCreature::s_nCtorCount_Default == 0, "Expected CCreature::s_nCtorCount_Default is zero, was: %d\n", CCreature::s_nCtorCount_Default);
        ok(CCreature::s_nCCtorCount == 0, "Expected CCreature::s_nCCtorCount is zero, was: %d\n", CCreature::s_nCCtorCount);
        ok(CCreature::s_nDtorCount == 0, "Expected CCreature::s_nDtorCount is zero, was: %d\n", CCreature::s_nDtorCount);
        ok(CCreature::s_nOpIsCount == 0, "Expected CCreature::s_nOpIsCount is zero, was: %d\n", CCreature::s_nOpIsCount);

        array1.SetCount(2);

        ok(CCreature::s_nCtorCount == 2, "Expected CCreature::s_nCtorCount is 2, was: %d\n", CCreature::s_nCtorCount);
        ok(CCreature::s_nCtorCount_Default == 2, "Expected CCreature::s_nCtorCount_Default is 2, was: %d\n", CCreature::s_nCtorCount_Default);
        ok(CCreature::s_nCCtorCount == 0, "Expected CCreature::s_nCCtorCount is zero, was: %d\n", CCreature::s_nCCtorCount);
        ok(CCreature::s_nDtorCount == 0, "Expected CCreature::s_nDtorCount is zero, was: %d\n", CCreature::s_nDtorCount);
        ok(CCreature::s_nOpIsCount == 0, "Expected CCreature::s_nOpIsCount is zero, was: %d\n", CCreature::s_nOpIsCount);

        array1.SetCount(1);

        ok(CCreature::s_nCtorCount == 2, "Expected CCreature::s_nCtorCount is 2, was: %d\n", CCreature::s_nCtorCount);
        ok(CCreature::s_nCtorCount_Default == 2, "Expected CCreature::s_nCtorCount_Default is 2, was: %d\n", CCreature::s_nCtorCount_Default);
        ok(CCreature::s_nCCtorCount == 0, "Expected CCreature::s_nCCtorCount is zero, was: %d\n", CCreature::s_nCCtorCount);
        ok(CCreature::s_nDtorCount == 1, "Expected CCreature::s_nDtorCount is 1, was: %d\n", CCreature::s_nDtorCount);
        ok(CCreature::s_nOpIsCount == 0, "Expected CCreature::s_nOpIsCount is zero, was: %d\n", CCreature::s_nOpIsCount);

        CCreature test(111);

        ok(CCreature::s_nCtorCount == 3, "Expected CCreature::s_nCtorCount is 3, was: %d\n", CCreature::s_nCtorCount);
        ok(CCreature::s_nCtorCount_Default == 2, "Expected CCreature::s_nCtorCount_Default is 2, was: %d\n", CCreature::s_nCtorCount_Default);
        ok(CCreature::s_nCCtorCount == 0, "Expected CCreature::s_nCCtorCount is zero, was: %d\n", CCreature::s_nCCtorCount);
        ok(CCreature::s_nDtorCount == 1, "Expected CCreature::s_nDtorCount is 1, was: %d\n", CCreature::s_nDtorCount);
        ok(CCreature::s_nOpIsCount == 0, "Expected CCreature::s_nOpIsCount is zero, was: %d\n", CCreature::s_nOpIsCount);

        ok(array1.GetCount() == 1u, "Expected GetCount() to be 1, was %u\n", array1.GetCount());
        ok(array1[0].m_id == 0x123456, "Got %d\n", array1[0].m_id);
        ok(array1.GetAt(0).m_id == 0x123456, "Got %d\n", array1.GetAt(0).m_id);

        array1.Add(test);

        ok(CCreature::s_nCtorCount == 3, "Expected CCreature::s_nCtorCount is 3, was: %d\n", CCreature::s_nCtorCount);
        ok(CCreature::s_nCtorCount_Default == 2, "Expected CCreature::s_nCtorCount_Default is 2, was: %d\n", CCreature::s_nCtorCount_Default);
        ok(CCreature::s_nCCtorCount == 1, "Expected CCreature::s_nCCtorCount is 1, was: %d\n", CCreature::s_nCCtorCount);
        ok(CCreature::s_nDtorCount == 1, "Expected CCreature::s_nDtorCount is 1, was: %d\n", CCreature::s_nDtorCount);
        ok(CCreature::s_nOpIsCount == 0, "Expected CCreature::s_nOpIsCount is zero, was: %d\n", CCreature::s_nOpIsCount);

        ok(array1.GetCount() == 2u, "Expected GetCount() to be 2, was %u\n", array1.GetCount());
        ok(array1[0].m_id == 0x123456, "Got %d\n", array1[0].m_id);
        ok(array1.GetAt(0).m_id == 0x123456, "Got %d\n", array1.GetAt(0).m_id);
        ok(array1[1].m_id == 111, "Got %d\n", array1[1].m_id);
        ok(array1.GetAt(1).m_id == 111, "Got %d\n", array1.GetAt(1).m_id);

        test.m_id = 222;
        array1[0] = test;

        ok(CCreature::s_nCtorCount == 3, "Expected CCreature::s_nCtorCount is 3, was: %d\n", CCreature::s_nCtorCount);
        ok(CCreature::s_nCtorCount_Default == 2, "Expected CCreature::s_nCtorCount_Default is 2, was: %d\n", CCreature::s_nCtorCount_Default);
        ok(CCreature::s_nCCtorCount == 1, "Expected CCreature::s_nCCtorCount is 1, was: %d\n", CCreature::s_nCCtorCount);
        ok(CCreature::s_nDtorCount == 1, "Expected CCreature::s_nDtorCount is 1, was: %d\n", CCreature::s_nDtorCount);
        ok(CCreature::s_nOpIsCount == 1, "Expected CCreature::s_nOpIsCount is 1, was: %d\n", CCreature::s_nOpIsCount);

        // Default traits does not call anything when relocating objects!
        array1.SetCount(100);

        ok(CCreature::s_nCtorCount == 101, "Expected CCreature::s_nCtorCount is 101, was: %d\n", CCreature::s_nCtorCount);
        ok(CCreature::s_nCtorCount_Default == 100, "Expected CCreature::s_nCtorCount_Default is 100, was: %d\n", CCreature::s_nCtorCount_Default);
        ok(CCreature::s_nCCtorCount == 1, "Expected CCreature::s_nCCtorCount is 1, was: %d\n", CCreature::s_nCCtorCount);
        ok(CCreature::s_nDtorCount == 1, "Expected CCreature::s_nDtorCount is 1, was: %d\n", CCreature::s_nDtorCount);
        ok(CCreature::s_nOpIsCount == 1, "Expected CCreature::s_nOpIsCount is 1, was: %d\n", CCreature::s_nOpIsCount);

        // Does not compile:
        //CAtlArray<CCreature> array2(array1);

        // Does not compile:
        //CAtlArray<CCreature> array2;
        //array2 = array1;
    }

    // Objects are cleaned up when the list goes away
    ok(CCreature::s_nCtorCount == 101, "Expected CCreature::s_nCtorCount is 101, was: %d\n", CCreature::s_nCtorCount);
    ok(CCreature::s_nCtorCount_Default == 100, "Expected CCreature::s_nCtorCount_Default is 100, was: %d\n", CCreature::s_nCtorCount_Default);
    ok(CCreature::s_nCCtorCount == 1, "Expected CCreature::s_nCCtorCount is 1, was: %d\n", CCreature::s_nCCtorCount);
    ok(CCreature::s_nDtorCount == 102, "Expected CCreature::s_nDtorCount is 102, was: %d\n", CCreature::s_nDtorCount);
    ok(CCreature::s_nOpIsCount == 1, "Expected CCreature::s_nOpIsCount is 1, was: %d\n", CCreature::s_nOpIsCount);

#ifndef HAVE_APITEST
    printf("CAtlArray: %i tests executed (0 marked as todo, %i failures), 0 skipped.\n", g_tests_executed, g_tests_failed);
    return g_tests_failed;
#endif
}
