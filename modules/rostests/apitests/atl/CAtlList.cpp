/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for CAtlList
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
#include <atlstr.h>

static void
test_BasicCases()
{
    CAtlList<int> list1;

    ok_size_t(list1.GetCount(), 0);
    list1.AddTail(56);
    ok_size_t(list1.GetCount(), 1);
    POSITION head = list1.AddHead(12);
    ok_size_t(list1.GetCount(), 2);
    POSITION tail = list1.AddTail(90);
    ok_size_t(list1.GetCount(), 3);

    list1.InsertBefore(head, -123);
    list1.InsertAfter(head, 34); // no longer head, but the POSITION should still be valid..

    list1.InsertBefore(tail, 78);
    list1.InsertAfter(tail, -44);

    int expected[] = {-123, 12, 34, 56, 78, 90, -44};
    int expected_size = sizeof(expected) / sizeof(expected[0]);
    int index = 0;
    POSITION it = list1.GetHeadPosition();
    while (it != NULL)
    {
        ok(index < expected_size, "Too many items, expected %d, got %d!\n", expected_size, (index + 1));
        int value = list1.GetNext(it);
        if (index < expected_size)
        {
            ok(value == expected[index], "Wrong value, got %d, expected %d\n", value, expected[index]);
        }
        else
        {
            ok(0, "Extra value: %d\n", value);
        }
        index++;
    }
    ok(it == NULL, "it does still point to something!\n");
}

static CStringW
to_str(const CAtlList<int>& lst)
{
    CStringW tmp;
    POSITION it = lst.GetHeadPosition();
    while (it != NULL)
    {
        int value = lst.GetNext(it);
        tmp.AppendFormat(L"%d,", value);
    }
    return tmp;
}

#define ok_list(lst, expected)                                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        CStringW _value = to_str(lst);                                                                                 \
        ok(_value == (expected), "Wrong value for '%s', expected: " #expected " got: \"%S\"\n", #lst,                  \
           _value.GetString());                                                                                        \
    } while (0)


static void
test_SwapElements()
{
    CAtlList<int> list;
    list.AddTail(1);
    list.AddTail(2);
    list.AddTail(3);

    ok_list(list, "1,2,3,");

    POSITION p1 = list.FindIndex(0);
    POSITION p2 = list.FindIndex(2);

    list.SwapElements(p1, p1);
    ok_list(list, "1,2,3,");

    list.SwapElements(p1, p2);
    ok_list(list, "3,2,1,");

    p1 = list.FindIndex(0);
    p2 = list.FindIndex(1);
    list.SwapElements(p1, p2);
    ok_list(list, "2,3,1,");

    p1 = list.FindIndex(1);
    p2 = list.FindIndex(2);
    list.SwapElements(p1, p2);
    ok_list(list, "2,1,3,");

    p1 = list.FindIndex(0);
    p2 = list.FindIndex(2);
    list.SwapElements(p2, p1);
    ok_list(list, "3,1,2,");
}

static void
test_AppendListToTail()
{
    CAtlList<int> list;
    list.AddTail(1);
    list.AddTail(2);
    list.AddTail(0);
    ok_list(list, "1,2,0,");

    CAtlList<int> list_tail;
    list_tail.AddTail(8);
    list_tail.AddTail(1);
    list_tail.AddTail(0);
    ok_list(list_tail, "8,1,0,");

    list.AddTailList(&list_tail);
    ok_list(list, "1,2,0,8,1,0,");

    list_tail.AddTailList(&list);
    ok_list(list_tail, "8,1,0,1,2,0,8,1,0,");
}

static void
test_AppendListToHead()
{
    CAtlList<int> list_head;
    list_head.AddHead(0);
    list_head.AddHead(0);
    list_head.AddHead(2);
    ok_list(list_head, "2,0,0,");

    CAtlList<int> list;
    list.AddHead(8);
    list.AddHead(9);
    list.AddHead(7);
    ok_list(list, "7,9,8,");

    list.AddHeadList(&list_head);
    ok_list(list, "2,0,0,7,9,8,");

    list_head.AddHeadList(&list);
    ok_list(list_head, "2,0,0,7,9,8,2,0,0,");
}

START_TEST(CAtlList)
{
    test_BasicCases();
    test_SwapElements();
    test_AppendListToTail();
    test_AppendListToHead();
}
