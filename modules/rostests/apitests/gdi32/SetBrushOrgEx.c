/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetBrushOrgEx
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

void Test_Set(ULONG ulLine, HDC hdc, INT x, INT y, LPPOINT ppt, BOOL bExp, DWORD dwErrExp)
{
    BOOL bResult;

    SetLastError(0);

    _SEH2_TRY
    {
        bResult = SetBrushOrgEx(hdc, x, y, ppt);
    }
    _SEH2_EXCEPT(1)
    {
        bResult = -1;
    }
    _SEH2_END;

    ok(bResult == bExp, "line %ld: Wrong result, expected %d, got %d\n",
       ulLine, bExp, bResult);
    ok(GetLastError() == dwErrExp,"line %ld: Wrong error, expected %lx, got %lx\n",
       ulLine, dwErrExp, GetLastError());
}

#define TEST_SET(hdc, x, y, ppt, bExp, dwErrExp) \
    Test_Set(__LINE__, hdc, x, y, ppt, bExp, dwErrExp)

void Test_SetBrushOrgEx()
{
    HDC hdc;
    POINT ptOldOrg;

    hdc = CreateCompatibleDC(0);
    ok(hdc != 0, "could not ceate DC\n");

    TEST_SET(0, 0, 0, NULL, 0, ERROR_INVALID_HANDLE);
    TEST_SET(0, 0, 0, (LPPOINT)-1, 0, ERROR_INVALID_HANDLE);
    TEST_SET(0, 0, 0, &ptOldOrg, 0, ERROR_INVALID_HANDLE);
    TEST_SET(hdc, 1, 2, &ptOldOrg, 1, 0);
    ok_long(ptOldOrg.x, 0);
    ok_long(ptOldOrg.y, 0);
    SetBrushOrgEx(hdc, 0, 0, &ptOldOrg);
    ok_long(ptOldOrg.x, 1);
    ok_long(ptOldOrg.y, 2);

    ptOldOrg.x = 0; ptOldOrg.y = 0;
    TEST_SET(hdc, 1, 2, (LPPOINT)-1, -1, 0);
    SetBrushOrgEx(hdc, 0, 0, &ptOldOrg);
    ok_long(ptOldOrg.x, 0);
    ok_long(ptOldOrg.y, 0);


    TEST_SET(hdc, -10000, -20000000, &ptOldOrg, 1, 0);
    ok_long(ptOldOrg.x, 0);
    ok_long(ptOldOrg.y, 0);
    SetBrushOrgEx(hdc, 0, 0, &ptOldOrg);
    ok_long(ptOldOrg.x, -10000);
    ok_long(ptOldOrg.y, -20000000);


}

START_TEST(SetBrushOrgEx)
{
    Test_SetBrushOrgEx();
}

