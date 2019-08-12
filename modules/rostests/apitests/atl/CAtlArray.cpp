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
    #include "atltest.h"
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

        ok_int(CCreature::s_nCtorCount, 0);
        ok_int(CCreature::s_nCtorCount_Default, 0);
        ok_int(CCreature::s_nCCtorCount, 0);
        ok_int(CCreature::s_nDtorCount, 0);
        ok_int(CCreature::s_nOpIsCount, 0);

        array1.SetCount(2);

        ok_int(CCreature::s_nCtorCount, 2);
        ok_int(CCreature::s_nCtorCount_Default, 2);
        ok_int(CCreature::s_nCCtorCount, 0);
        ok_int(CCreature::s_nDtorCount, 0);
        ok_int(CCreature::s_nOpIsCount, 0);

        array1.SetCount(1);

        ok_int(CCreature::s_nCtorCount, 2);
        ok_int(CCreature::s_nCtorCount_Default, 2);
        ok_int(CCreature::s_nCCtorCount, 0);
        ok_int(CCreature::s_nDtorCount, 1);
        ok_int(CCreature::s_nOpIsCount, 0);

        CCreature test(111);

        ok_int(CCreature::s_nCtorCount, 3);
        ok_int(CCreature::s_nCtorCount_Default, 2);
        ok_int(CCreature::s_nCCtorCount, 0);
        ok_int(CCreature::s_nDtorCount, 1);
        ok_int(CCreature::s_nOpIsCount, 0);

        ok_size_t(array1.GetCount(), 1u);
        ok_int(array1[0].m_id, 0x123456);
        ok_int(array1.GetAt(0).m_id, 0x123456);

        array1.Add(test);

        ok_int(CCreature::s_nCtorCount, 3);
        ok_int(CCreature::s_nCtorCount_Default, 2);
        ok_int(CCreature::s_nCCtorCount, 1);
        ok_int(CCreature::s_nDtorCount, 1);
        ok_int(CCreature::s_nOpIsCount, 0);

        ok_size_t(array1.GetCount(), 2u);
        ok_int(array1[0].m_id, 0x123456);
        ok_int(array1.GetAt(0).m_id, 0x123456);
        ok_int(array1[1].m_id, 111);
        ok_int(array1.GetAt(1).m_id, 111);

        test.m_id = 222;
        array1[0] = test;

        ok_int(CCreature::s_nCtorCount, 3);
        ok_int(CCreature::s_nCtorCount_Default, 2);
        ok_int(CCreature::s_nCCtorCount, 1);
        ok_int(CCreature::s_nDtorCount, 1);
        ok_int(CCreature::s_nOpIsCount, 1);

        // Default traits does not call anything when relocating objects!
        array1.SetCount(100);

        ok_int(CCreature::s_nCtorCount, 101);
        ok_int(CCreature::s_nCtorCount_Default, 100);
        ok_int(CCreature::s_nCCtorCount, 1);
        ok_int(CCreature::s_nDtorCount, 1);
        ok_int(CCreature::s_nOpIsCount, 1);

        // Does not compile:
        //CAtlArray<CCreature> array2(array1);

        // Does not compile:
        //CAtlArray<CCreature> array2;
        //array2 = array1;
    }

    // Objects are cleaned up when the list goes away
    ok_int(CCreature::s_nCtorCount, 101);
    ok_int(CCreature::s_nCtorCount_Default, 100);
    ok_int(CCreature::s_nCCtorCount, 1);
    ok_int(CCreature::s_nDtorCount, 102);
    ok_int(CCreature::s_nOpIsCount, 1);
}
