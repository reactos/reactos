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


START_TEST(CAtlList)
{
    CAtlList<int> list1;

    ok(list1.GetCount() == 0, "Expected list1's size is zero, was %d\n", list1.GetCount());
    list1.AddTail(56);
    ok(list1.GetCount() == 1, "Expected list1's size is 1, was %d\n", list1.GetCount());
    POSITION head = list1.AddHead(12);
    ok(list1.GetCount() == 2, "Expected list1's size is 2, was %d\n", list1.GetCount());
    POSITION tail = list1.AddTail(90);
    ok(list1.GetCount() == 3, "Expected list1's size is 3, was %d\n", list1.GetCount());

    list1.InsertBefore(head, -123);
    list1.InsertAfter(head, 34);    // no longer head, but the POSITION should still be valid..

    list1.InsertBefore(tail, 78);
    list1.InsertAfter(tail, -44);

    int expected[] = {-123, 12, 34, 56, 78, 90, -44 };
    int expected_size = sizeof(expected) / sizeof(expected[0]);
    int index = 0;
    POSITION it = list1.GetHeadPosition();
    while (it != NULL)
    {
        ok(index < expected_size, "Too many items, expected %d, got %d!\n", expected_size, (index+1));
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

#ifndef HAVE_APITEST
    printf("CAtlList: %i tests executed (0 marked as todo, %i failures), 0 skipped.\n", g_tests_executed, g_tests_failed);
    return g_tests_failed;
#endif
}
