/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetTextExtentExPoint
 * PROGRAMMERS:     Timo Kreuzer
 *                  Katayama Hirofumi MZ
 */

#include "precomp.h"

void Test_GetTextExtentExPoint()
{
    INT nFit;
    SIZE size;
    BOOL result;

    SetLastError(0);

    result = GetTextExtentExPointA(GetDC(0), "test", 4, 1000, &nFit, NULL, &size);
    ok_int(result, 1);
    ok_int(nFit, 4);
    ok_err(0);
    printf("nFit = %d\n", nFit);

    result = GetTextExtentExPointA(GetDC(0), "test", 4, 1, &nFit, NULL, &size);
    ok_int(result, 1);
    ok_int(nFit, 0);
    ok_err(0);
    printf("nFit = %d\n", nFit);

    result = GetTextExtentExPointA(GetDC(0), "test", 4, 0, &nFit, NULL, &size);
    ok_int(result, 1);
    ok_int(nFit, 0);
    ok_err(0);

    result = GetTextExtentExPointA(GetDC(0), "test", 4, -1, &nFit, NULL, &size);
    ok_int(result, 1);
    ok_int(nFit, 4);
    ok_err(0);

    result = GetTextExtentExPointA(GetDC(0), "test", 4, -2, &nFit, NULL, &size);
    ok_int(result, 0);
    ok_err(87);

    result = GetTextExtentExPointW(GetDC(0), L"test", 4, -10, &nFit, NULL, &size);
    ok_int(result, 1);

    result = GetTextExtentExPointA(GetDC(0), "test", 4, -10, &nFit, NULL, &size);
    ok_int(result, 0);
}

START_TEST(GetTextExtentExPoint)
{
    Test_GetTextExtentExPoint();
}

