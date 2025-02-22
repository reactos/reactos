/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CSimpleArray
 * PROGRAMMER:      Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#ifdef HAVE_APITEST
    #include <apitest.h>
#else
    #include "atltest.h"
#endif

#include <atlbase.h>
#include <atlsimpcoll.h>

struct CSimpleCreature
{
    static int s_nCount;
    static int s_nCopyCount;
    CSimpleCreature()
    {
        CSimpleCreature::s_nCount++;
    }
    CSimpleCreature(const CSimpleCreature& c)
    {
        CSimpleCreature::s_nCount++;
    }
    ~CSimpleCreature()
    {
        CSimpleCreature::s_nCount--;
    }
    CSimpleCreature& operator=(const CSimpleCreature& other)
    {
        CSimpleCreature::s_nCopyCount++;
        return *this;
    }
};

int CSimpleCreature::s_nCount = 0;
int CSimpleCreature::s_nCopyCount = 0;


START_TEST(CSimpleArray)
{
    CSimpleArray<int> array1;

    ok_int(array1.GetSize(), 0);

    array1.Add(123);

    ok_int(array1.GetSize(), 1);
    ok_int(array1.GetData()[0], 123);
    ok_int(array1[0], 123);

    array1.Add(456);

    ok_int(array1.GetSize(), 2);
    ok_int(array1.GetData()[0], 123);
    ok_int(array1[0], 123);
    ok_int(array1.GetData()[1], 456);
    ok_int(array1[1], 456);

    array1.RemoveAll();
    ok_int(array1.GetSize(), 0);

    array1.Add(1);
    array1.Add(1);
    array1.Add(1);
    array1.Add(2);
    array1.Add(2);
    array1.Add(3);
    ok_int(array1.GetSize(), 6);

    array1.Remove(2);
    ok_int(array1.GetSize(), 5);

    array1.Remove(1);
    ok_int(array1.GetSize(), 4);

    ok_int(array1[0], 1);
    ok_int(array1[1], 1);
    ok_int(array1[2], 2);
    ok_int(array1[3], 3);

    ok_int(CSimpleCreature::s_nCount, 0);
    ok_int(CSimpleCreature::s_nCopyCount, 0);

    CSimpleArray<CSimpleCreature> array2;
    {
        CSimpleCreature creature1, creature2;

        ok_int(CSimpleCreature::s_nCount, 2);
        ok_int(CSimpleCreature::s_nCopyCount, 0);
        array2.Add(creature1);
        ok_int(CSimpleCreature::s_nCount, 3);
        ok_int(CSimpleCreature::s_nCopyCount, 0);
        array2.Add(creature2);
        ok_int(CSimpleCreature::s_nCount, 4);
        ok_int(CSimpleCreature::s_nCopyCount, 0);
    }
    ok_int(CSimpleCreature::s_nCount, 2);
    ok_int(CSimpleCreature::s_nCopyCount, 0);

    {
        CSimpleArray<CSimpleCreature> array3(array2), array4, array5;
        ok_int(CSimpleCreature::s_nCount, 4);
        ok_int(CSimpleCreature::s_nCopyCount, 0);

        array4 = array2;
        ok_int(CSimpleCreature::s_nCount, 6);
        ok_int(CSimpleCreature::s_nCopyCount, 0);

        CSimpleCreature creature1;
        ok_int(CSimpleCreature::s_nCount, 7);
        ok_int(CSimpleCreature::s_nCopyCount, 0);

        array4.Add(creature1);
        ok_int(CSimpleCreature::s_nCount, 8);
        ok_int(CSimpleCreature::s_nCopyCount, 0);

        array3 = array4;
        ok_int(CSimpleCreature::s_nCount, 9);
        ok_int(CSimpleCreature::s_nCopyCount, 0);

        array5 = array2;
        ok_int(CSimpleCreature::s_nCount, 11);
        ok_int(CSimpleCreature::s_nCopyCount, 0);

        array5 = array2;
        ok_int(CSimpleCreature::s_nCount, 11);
        ok_int(CSimpleCreature::s_nCopyCount, 0);
    }
    ok_int(CSimpleCreature::s_nCount, 2);
    ok_int(CSimpleCreature::s_nCopyCount, 0);

    array2.RemoveAll();
    ok_int(CSimpleCreature::s_nCount, 0);
    ok_int(CSimpleCreature::s_nCopyCount, 0);

    array1.RemoveAll();
    ok_int(array1.GetSize(), 0);
    for (int i = 0; i < 100; ++i)
    {
        array1.Add(i);
    }
    ok_int(array1.GetSize(), 100);

    array1.RemoveAll();
    ok_int(array1.GetSize(), 0);
    array1.Add(123);
    array1.Add(321);
    ok(!!array1.RemoveAt(0), "Expected RemoveAt(0) to succeed\n");
    ok_int(array1.GetSize(), 1);
    if (array1.GetSize() == 1)
    {
        ok_int(array1[0], 321);
    }
    ok(!!array1.RemoveAt(0), "Expected RemoveAt(0) to succeed\n");
    ok_int(array1.GetSize(), 0);
}
