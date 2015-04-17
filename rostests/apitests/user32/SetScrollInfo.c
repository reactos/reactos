/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for SetScrollInfo
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#include <wingdi.h>
#include <winuser.h>

START_TEST(SetScrollInfo)
{
    struct
    {
        int nMin;
        int nMax;
        UINT nPage;
        int nPos;
        int nMinResult;
        int nMaxResult;
        UINT nPageResult;
        int nPosResult;
    } tests[] =
    {
            /* min max pg  pos          min max pg  pos */
 /* [0] */  {  0,  0,  0,  0,           0,  0,  0,  0 },
            {  0,  0,  1,  1,           0,  0,  1,  0 },

            /* vary nPage only */
            {  5, 10,  0,  0,           5, 10,  0,  5 },
            {  5, 10,  1,  0,           5, 10,  1,  5 },
            {  5, 10,  4,  0,           5, 10,  4,  5 },
 /* [5] */  {  5, 10,  5,  0,           5, 10,  5,  5 },
            {  5, 10,  6,  0,           5, 10,  6,  5 },
            {  5, 10,  7,  0,           5, 10,  6,  5 },
            {  5, 10, 10,  0,           5, 10,  6,  5 },
            {  5, 10, 11,  0,           5, 10,  6,  5 },
 /* [10] */ {  5, 10, 0xffffffff,  0,   5, 10,  6,  5 },

            /* vary nPos only */
            {  5, 10,  0,  4,           5, 10,  0,  5 },
            {  5, 10,  0,  5,           5, 10,  0,  5 },
            {  5, 10,  0, 10,           5, 10,  0, 10 },
            {  5, 10,  0, 11,           5, 10,  0, 10 },
 /* [15] */ {  5, 10,  0, -1,           5, 10,  0,  5 },
            {  5, 10,  0, 0x80000000,   5, 10,  0,  5 },
            {  5, 10,  0, 0x7fffffff,   5, 10,  0, 10 },

            /* maximum nPos depends on nPage */
            {  5, 10,  0,  9,           5, 10,  0,  9 },
            {  5, 10,  0, 10,           5, 10,  0, 10 },
 /* [20] */ {  5, 10,  0, 11,           5, 10,  0, 10 },
            {  5, 10,  1,  9,           5, 10,  1,  9 },
            {  5, 10,  1, 10,           5, 10,  1, 10 },
            {  5, 10,  1, 11,           5, 10,  1, 10 },
            {  5, 10,  2,  9,           5, 10,  2,  9 },
 /* [25] */ {  5, 10,  2, 10,           5, 10,  2,  9 },
            {  5, 10,  2, 11,           5, 10,  2,  9 },
            {  5, 10,  3,  9,           5, 10,  3,  8 },
            {  5, 10,  3, 10,           5, 10,  3,  8 },
            {  5, 10,  3, 11,           5, 10,  3,  8 },

            /* relation between min and max? */
            /* min max pg  pos                           min max pg  pos */
 /* [30] */ {  5,  6,  0,  0,                            5,  6,  0,  5 },
            {  5,  5,  0,  0,                            5,  5,  0,  5 },
            {  6,  5,  0,  0,                            6,  6,  0,  6 },
            {  7,  5,  0,  0,                            7,  7,  0,  7 },
            { -2,  0,  0,  0,                           -2,  0,  0,  0 },
 /* [35] */ { -2, -1,  0,  0,                           -2, -1,  0, -1 },
            { -1,  0,  0,  0,                           -1,  0,  0,  0 },
            { -1, -1,  0,  0,                           -1, -1,  0, -1 },
            {  0, -1,  0,  0,                            0,  0,  0,  0 },
            {  1, -1,  0,  0,                            1,  1,  0,  1 },
 /* [40] */ { 0x80000000, 0x7fffffff,  0,  0,           0x80000000, 0x7fffffff,  0,  0 },
            { 0x80000001, 0x7fffffff,  0,  0,           0x80000001, 0x7fffffff,  0,  0 },
            { 0x80000000, 0x7ffffffe,  0,  0,           0x80000000, 0x7ffffffe,  0,  0 },
            { 0x7fffffff, 0x80000000,  0,  0,           0x7fffffff, 0x7fffffff,  0, 0x7fffffff },
            {  0, 0x7fffffff,  0,  0,                    0, 0x7fffffff,  0,  0 },
 /* [45] */ { -1, 0x7fffffff,  0,  0,                   -1, 0x7fffffff,  0,  0 },
            { -2, 0x7fffffff,  0,  0,                   -2, 0x7fffffff,  0,  0 },

            /* What happens to nPage when we have a large range? */
            { 0x80000000, 0x7fffffff,  1,  5,           0x80000000, 0x7fffffff,  1,  5 },
            { 0x80000000, 0x7fffffff,  2,  5,           0x80000000, 0x7fffffff,  2,  5 },
            { 0x80000000, 0x7fffffff,  3,  5,           0x80000000, 0x7fffffff,  2,  5 },
 /* [50] */ { 0x80000000, 0x7fffffff, 0x7fffffff,  5,   0x80000000, 0x7fffffff,  2,  5 },
            { 0x80000000, 0x7fffffff, 0x80000000,  5,   0x80000000, 0x7fffffff,  2,  5 },
            { 0x80000000, 0x7fffffff, 0x80000001,  5,   0x80000000, 0x7fffffff,  2,  5 },
            { 0x80000000, 0x7fffffff, 0xffffffff,  5,   0x80000000, 0x7fffffff,  2,  5 },
            { 0x80000001, 0x7fffffff,  1,  5,           0x80000001, 0x7fffffff,  1,  5 },
 /* [55] */ { 0x80000001, 0x7fffffff,  2,  5,           0x80000001, 0x7fffffff,  2,  5 },
            { 0x80000001, 0x7fffffff,  3,  5,           0x80000001, 0x7fffffff,  3,  5 },
            { 0x80000001, 0x7fffffff,  4,  5,           0x80000001, 0x7fffffff,  3,  5 },
            { 0x80000000, 0x7ffffffe,  1,  5,           0x80000000, 0x7ffffffe,  1,  5 },
            { 0x80000000, 0x7ffffffe,  2,  5,           0x80000000, 0x7ffffffe,  2,  5 },
 /* [60] */ { 0x80000000, 0x7ffffffe,  3,  5,           0x80000000, 0x7ffffffe,  3,  5 },
            { 0x80000000, 0x7ffffffe,  4,  5,           0x80000000, 0x7ffffffe,  3,  5 },
            {  0, 0x7fffffff, 0x7fffffff,  5,            0, 0x7fffffff, 0x7fffffff,  1 },
            {  0, 0x7fffffff, 0x80000000,  5,            0, 0x7fffffff, 0x80000000,  0 },
            {  0, 0x7fffffff, 0x80000001,  5,            0, 0x7fffffff, 0x80000000,  0 },
 /* [65] */ {  0, 0x7fffffff, 0x80000002,  5,            0, 0x7fffffff, 0x80000000,  0 },
            { -1, 0x7fffffff, 0x7fffffff,  5,           -1, 0x7fffffff, 0x7fffffff,  1 },
            { -1, 0x7fffffff, 0x80000000,  5,           -1, 0x7fffffff, 0x80000000,  0 },
            { -1, 0x7fffffff, 0x80000001,  5,           -1, 0x7fffffff, 0x80000001, -1 },
            { -1, 0x7fffffff, 0x80000002,  5,           -1, 0x7fffffff, 0x80000001, -1 },
 /* [70] */ { -1, 0x7fffffff, 0x80000003,  5,           -1, 0x7fffffff, 0x80000001, -1 },
            { -2, 0x7fffffff, 0x80000000,  5,           -2, 0x7fffffff, 0x80000000, 0 },
            { -2, 0x7fffffff, 0x80000001,  5,           -2, 0x7fffffff, 0x80000000, 0 },
            { 0xf0000000, 0x7fffffff, 0x90000000,  5,   0xf0000000, 0x7fffffff, 0x70000002,  5 },
            { 0xf0000000, 0x7fffffff, 0x90000001,  5,   0xf0000000, 0x7fffffff, 0x70000002,  5 },
    };
    unsigned i;
    HWND hScroll;
    SCROLLINFO si;
    BOOL success;
    int ret;

    hScroll = CreateWindowExW(0, L"SCROLLBAR", NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    ok(hScroll != NULL, "\n");
    if (!hScroll)
    {
        skip("No scroll bar\n");
        return;
    }

    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
    success = GetScrollInfo(hScroll, SB_CTL, &si);
    ok(success == TRUE, "GetScrollInfo returned %d\n", success);
    ok(si.nMin == 0, "nMin = %d\n", si.nMin);
    ok(si.nMax == 0, "nMax = %d\n", si.nMax);
    ok(si.nPage == 0, "nPage = %u\n", si.nPage);
    ok(si.nPos == 0, "nPos = %d\n", si.nPos);

    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        si.cbSize = sizeof(si);
        si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
        si.nMin = tests[i].nMin;
        si.nMax = tests[i].nMax;
        si.nPage = tests[i].nPage;
        si.nPos = tests[i].nPos;
        ret = SetScrollInfo(hScroll, SB_CTL, &si, FALSE);
        ok(ret == tests[i].nPosResult, "[%d] ret = %d, expected %d\n", i, ret, tests[i].nPosResult);

        FillMemory(&si, sizeof(si), 0x55);
        si.cbSize = sizeof(si);
        si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
        success = GetScrollInfo(hScroll, SB_CTL, &si);
        ok(success == TRUE, "[%d] GetScrollInfo returned %d\n", i, success);
        ok(si.nMin == tests[i].nMinResult, "[%d] nMin = %d, expected %d\n", i, si.nMin, tests[i].nMinResult);
        ok(si.nMax == tests[i].nMaxResult, "[%d] nMax = %d, expected %d\n", i, si.nMax, tests[i].nMaxResult);
        ok(si.nPage == tests[i].nPageResult, "[%d] nPage = %u, expected %u\n", i, si.nPage, tests[i].nPageResult);
        ok(si.nPos == tests[i].nPosResult, "[%d] nPos = %d, expected %d\n", i, si.nPos, tests[i].nPosResult);
    }
    DestroyWindow(hScroll);
}
