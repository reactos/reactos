/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for SetScrollRange
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

START_TEST(SetScrollRange)
{
    struct
    {
        INT nMin;
        INT nMax;
        BOOL result;
    } tests[] =
    {
        {  0,                0,     TRUE },
        {  0,          INT_MAX,     TRUE },
        { -1,          INT_MAX,     FALSE },
        { INT_MIN,     INT_MAX,     FALSE },
        { INT_MIN,     0,           FALSE },
        { INT_MIN,     -1,          TRUE },
    };
    unsigned i;
    HWND hScroll;
    BOOL success;
    DWORD error;
    INT newMin, newMax;

    hScroll = CreateWindowExW(0, L"SCROLLBAR", NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    ok(hScroll != NULL, "CreateWindowEx failed with %lu\n", GetLastError());
    if (!hScroll)
    {
        skip("No scroll bar\n");
        return;
    }

    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        (void)SetScrollRange(hScroll, SB_CTL, 123, 456, FALSE);
        SetLastError(0xdeaff00d);
        success = SetScrollRange(hScroll, SB_CTL, tests[i].nMin, tests[i].nMax, FALSE);
        error = GetLastError();
        (void)GetScrollRange(hScroll, SB_CTL, &newMin, &newMax);
        if (tests[i].result)
        {
            ok(success == TRUE, "SetScrollRange(%d, %d) failed with %d %lu\n", tests[i].nMin, tests[i].nMax, success, error);
            ok(newMin == tests[i].nMin, "nMin was changed to %d\n", tests[i].nMin);
            ok(newMax == tests[i].nMax, "nMax was changed to %d\n", tests[i].nMax);
        }
        else
        {
            ok(success == FALSE, "SetScrollRange(%d, %d) succeeded with %d\n", tests[i].nMin, tests[i].nMax, success);
            ok(error == ERROR_INVALID_SCROLLBAR_RANGE, "Error %lu\n", error);
            ok(newMin == 123, "nMin was changed to %d\n", newMin);
            ok(newMax == 456, "nMax was changed to %d\n", newMax);
        }
    }

    DestroyWindow(hScroll);
}
