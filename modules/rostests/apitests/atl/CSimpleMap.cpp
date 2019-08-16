/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CSimpleMap
 * PROGRAMMER:      Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#ifdef HAVE_APITEST
    #include <apitest.h>
#else
    #include "atltest.h"
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

    ok_int(map1.GetSize(), 0);

    map1.Add(1, 2);
    ok_int(map1.GetSize(), 1);
    map1.Add(2, 3);
    ok_int(map1.GetSize(), 2);

    ok_int(map1.Lookup(1), 2);
    ok_int(map1.Lookup(2), 3);
    ok_int(map1.Lookup(-1), 0);

    ok_int(map1.ReverseLookup(2), 1);
    ok_int(map1.ReverseLookup(3), 2);

    ok_int(map1.GetKeyAt(0), 1);
    ok_int(map1.GetKeyAt(1), 2);

    ok_int(map1.GetValueAt(0), 2);
    ok_int(map1.GetValueAt(1), 3);

    map1.SetAt(2, 4);

    ok_int(map1.Lookup(1), 2);
    ok_int(map1.Lookup(2), 4);

    ok_int(map1.ReverseLookup(2), 1);
    ok_int(map1.ReverseLookup(4), 2);

    map1.Remove(1);
    ok_int(map1.GetSize(), 1);
    map1.Remove(2);
    ok_int(map1.GetSize(), 0);

    map1.Add(1, 4);
    ok_int(map1.GetSize(), 1);
    map1.Add(2, 8);
    ok_int(map1.GetSize(), 2);
    map1.Add(3, 12);
    ok_int(map1.GetSize(), 3);

    map1.RemoveAll();
    ok_int(map1.GetSize(), 0);

    ok_int(CMonster::s_nCount, 0);
    ok_int(CMonster::s_nCopyCount, 0);

    CSimpleMap<CMonster, CMonster> map2;
    ok_int(map2.GetSize(), 0);

    ok_int(CMonster::s_nCount, 0);
    ok_int(CMonster::s_nCopyCount, 0);

    {
        CMonster m1;
        ok_int(CMonster::s_nCount, 1);
        ok_int(CMonster::s_nCopyCount, 0);

        CMonster m2;
        ok_int(CMonster::s_nCount, 2);
        ok_int(CMonster::s_nCopyCount, 0);

        map2.Add(m1, m2);
        ok_int(CMonster::s_nCount, 4);
        ok_int(CMonster::s_nCopyCount, 0);
    }

    ok_int(map2.GetSize(), 1);
    ok_int(CMonster::s_nCount, 2);
    ok_int(CMonster::s_nCopyCount, 0);

    {
        CMonster m1;
        ok_int(CMonster::s_nCount, 3);
        ok_int(CMonster::s_nCopyCount, 0);

        CMonster m2;
        ok_int(CMonster::s_nCount, 4);
        ok_int(CMonster::s_nCopyCount, 0);

        map2.Add(m1, m2);
        ok_int(CMonster::s_nCount, 6);
        ok_int(CMonster::s_nCopyCount, 0);
    }

    ok_int(map2.GetSize(), 2);
    ok_int(CMonster::s_nCount, 4);
    ok_int(CMonster::s_nCopyCount, 0);

    map2.RemoveAt(0);
    ok_int(CMonster::s_nCount, 2);
    ok_int(map2.GetSize(), 1);
    ok_int(CMonster::s_nCopyCount, 0);

    map2.RemoveAt(0);
    ok_int(CMonster::s_nCount, 0);
    ok_int(map2.GetSize(), 0);
    ok_int(CMonster::s_nCopyCount, 0);

    CSimpleMap<int, CMonster> map3;
    ok_int(map3.GetSize(), 0);

    CMonster m3;
    ok_int(CMonster::s_nCount, 1);

    map3.Add(1, m3);
    ok_int(map3.GetSize(), 1);
    ok_int(CMonster::s_nCount, 2);

    map3.Add(2, m3);
    ok_int(map3.GetSize(), 2);
    ok_int(CMonster::s_nCount, 3);

    map3.Add(3, m3);
    ok_int(map3.GetSize(), 3);
    ok_int(CMonster::s_nCount, 4);

    map3.Remove(2);
    ok_int(map3.GetSize(), 2);
    ok_int(CMonster::s_nCount, 3);

    map3.RemoveAll();
    ok_int(map3.GetSize(), 0);
    ok_int(CMonster::s_nCount, 1);

    map1.Add(1, 2);
    ok_int(map1.GetSize(), 1);
    map1.Add(2, 3);
    ok_int(map1.GetSize(), 2);

    ok(!!map1.RemoveAt(0), "Expected RemoveAt(0) to succeed\n");
    ok_int(map1.GetSize(), 1);
    ok(!!map1.RemoveAt(0), "Expected RemoveAt(0) to succeed\n");
    ok_int(map1.GetSize(), 0);
}
