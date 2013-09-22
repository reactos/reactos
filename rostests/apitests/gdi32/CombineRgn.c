/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ...
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>

#include <wingdi.h>

void Test_CombineRgn_Params()
{
    HRGN hrgn1, hrgn2, hrgn3;

    hrgn1 = CreateRectRgn(0, 0, 0, 0);
    hrgn2 = CreateRectRgn(0, 0, 10, 10);
    hrgn3 = CreateRectRgn(5, 5, 20, 20);

    SetLastError(0xbadbabe);
    ok_long(CombineRgn(NULL, NULL, NULL, 0), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, 0), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, 6), ERROR);
    ok((GetLastError() == 0xbadbabe) || (GetLastError() == ERROR_INVALID_PARAMETER), "wrong error: %ld\n", GetLastError());

}

void Test_CombineRgn_COPY()
{
    HRGN hrgn1, hrgn2, hrgn3;

    hrgn1 = CreateRectRgn(0, 0, 0, 0);
    hrgn2 = CreateRectRgn(0, 0, 10, 10);
    hrgn3 = CreateRectRgn(5, 5, 20, 20);

    SetLastError(0xbadbabe);
    ok_long(CombineRgn(NULL, NULL, NULL, RGN_COPY), ERROR);
    ok_long(CombineRgn(NULL, hrgn1, NULL, RGN_COPY), ERROR);
    ok_long(CombineRgn(NULL, NULL, hrgn1, RGN_COPY), ERROR);
    ok_long(CombineRgn(NULL, hrgn1, hrgn2, RGN_COPY), ERROR);
    ok((GetLastError() == 0xbadbabe) || (GetLastError() == ERROR_INVALID_HANDLE), "wrong error: %ld\n", GetLastError());

    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_COPY), SIMPLEREGION);
    ok(EqualRgn(hrgn1, hrgn2), "Region is not correct\n");

    ok_long(CombineRgn(hrgn1, hrgn1, NULL, RGN_COPY), SIMPLEREGION);
    ok(EqualRgn(hrgn1, hrgn2), "Region is not correct\n");

    ok_long(CombineRgn(hrgn1, hrgn3, GetStockObject(BLACK_PEN), RGN_COPY), SIMPLEREGION);
    ok(EqualRgn(hrgn1, hrgn3), "Region is not correct\n");

    ok_long(CombineRgn(hrgn1, GetStockObject(BLACK_PEN), hrgn2, RGN_COPY), ERROR);
    ok(EqualRgn(hrgn1, hrgn3), "Region is not correct\n");

    ok_long(CombineRgn(hrgn1, NULL, hrgn2, RGN_COPY), ERROR);
    ok(EqualRgn(hrgn1, hrgn3), "Region is not correct\n");

    ok_long(CombineRgn(hrgn1, NULL, NULL, RGN_COPY), ERROR);
    ok(EqualRgn(hrgn1, hrgn3), "Region is not correct\n");

    ok((GetLastError() == 0xbadbabe) || (GetLastError() == ERROR_INVALID_HANDLE), "wrong error: %ld\n", GetLastError());

}

void Test_CombineRgn_AND()
{
    HRGN hrgn1, hrgn2, hrgn3;

    hrgn1 = CreateRectRgn(0, 0, 0, 0);
    hrgn2 = CreateRectRgn(0, 0, 10, 10);
    hrgn3 = CreateRectRgn(5, 5, 20, 20);

    SetLastError(0xbadbabe);
    ok_long(CombineRgn(NULL, NULL, NULL, RGN_AND), ERROR);
    ok_long(CombineRgn(hrgn1, NULL, NULL, RGN_AND), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, NULL, RGN_AND), ERROR);
    ok_long(CombineRgn(hrgn1, NULL, hrgn2, RGN_AND), ERROR);
    ok_long(CombineRgn(hrgn1, GetStockObject(BLACK_PEN), hrgn2, RGN_AND), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, GetStockObject(BLACK_PEN), RGN_AND), ERROR);
    ok((GetLastError() == 0xbadbabe) || (GetLastError() == ERROR_INVALID_HANDLE), "wrong error: %ld\n", GetLastError());


    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_AND), SIMPLEREGION);
    SetRectRgn(hrgn2, 5, 5, 10, 10);
    ok(EqualRgn(hrgn1, hrgn2), "Region is not correct\n");

    SetRectRgn(hrgn2, 0, 0, 5, 5);
    SetRectRgn(hrgn3, 5, 0, 10, 5);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_AND), NULLREGION);
    SetRectRgn(hrgn2, 0, 0, 0, 0);
    ok(EqualRgn(hrgn1, hrgn2), "Region is not correct\n");

    SetRectRgn(hrgn2, 0, 0, 20, 20);
    SetRectRgn(hrgn3, 5, 5, 10, 10);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_AND), SIMPLEREGION);
    SetRectRgn(hrgn2, 5, 5, 10, 10);
    ok(EqualRgn(hrgn1, hrgn2), "Region is not correct\n");


    SetRectRgn(hrgn2, 0, 0, 30, 10);
    SetRectRgn(hrgn3, 10, 10, 20, 30);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_OR), COMPLEXREGION);
    SetRectRgn(hrgn2, 10, 0, 30, 30);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_AND), COMPLEXREGION);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn1, RGN_AND), COMPLEXREGION);
    SetRectRgn(hrgn2, 10, 10, 30, 30);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_AND), SIMPLEREGION);
    SetRectRgn(hrgn2, 0, 0, 10, 10);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_AND), NULLREGION);

    SetRectRgn(hrgn1, 0, 0, 30, 10);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn1, RGN_AND), SIMPLEREGION);

}

void Test_CombineRgn_OR()
{
    HRGN hrgn1, hrgn2, hrgn3;

    hrgn1 = CreateRectRgn(0, 0, 0, 0);
    hrgn2 = CreateRectRgn(0, 0, 5, 5);
    hrgn3 = CreateRectRgn(5, 0, 10, 5);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_OR), SIMPLEREGION);
    SetRectRgn(hrgn2, 0, 0, 10, 5);
    ok(EqualRgn(hrgn1, hrgn2), "Region is not correct\n");

    SetRectRgn(hrgn2, 0, 0, 10, 10);
    SetRectRgn(hrgn3, 10, 10, 20, 20);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_OR), COMPLEXREGION);
    SetRectRgn(hrgn2, 10, 0, 20, 10);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_OR), COMPLEXREGION);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn1, RGN_OR), COMPLEXREGION);
    SetRectRgn(hrgn2, 0, 10, 10, 20);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_OR), SIMPLEREGION);
    SetRectRgn(hrgn2, 0, 0, 20, 20);
    ok(EqualRgn(hrgn1, hrgn2), "Region is not correct\n");

}

void Test_CombineRgn_DIFF()
{
    HRGN hrgn1, hrgn2, hrgn3;

    hrgn1 = CreateRectRgn(0, 0, 0, 0);
    hrgn2 = CreateRectRgn(0, 0, 10, 10);
    hrgn3 = CreateRectRgn(5, 0, 10, 5);

    SetLastError(0xbadbabe);
    ok_long(CombineRgn(NULL, NULL, NULL, RGN_DIFF), ERROR);
    ok_long(CombineRgn(hrgn1, NULL, NULL, RGN_DIFF), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, NULL, RGN_DIFF), ERROR);
    ok_long(CombineRgn(hrgn1, NULL, hrgn2, RGN_DIFF), ERROR);
    ok_long(CombineRgn(hrgn1, GetStockObject(BLACK_PEN), hrgn2, RGN_DIFF), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, GetStockObject(BLACK_PEN), RGN_DIFF), ERROR);
    ok((GetLastError() == 0xbadbabe) || (GetLastError() == ERROR_INVALID_HANDLE), "wrong error: %ld\n", GetLastError());

    ok_long(CombineRgn(hrgn1, hrgn1, hrgn1, RGN_DIFF), NULLREGION);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn2, RGN_DIFF), NULLREGION);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_DIFF), NULLREGION);

    ok_long(CombineRgn(hrgn1, hrgn2, hrgn1, RGN_DIFF), SIMPLEREGION);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_DIFF), COMPLEXREGION);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn3, RGN_DIFF), COMPLEXREGION);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_DIFF), NULLREGION);


}

void Test_CombineRgn_XOR()
{
    HRGN hrgn1, hrgn2, hrgn3, hrgn4;

    hrgn1 = CreateRectRgn(0, 0, 0, 0);
    hrgn2 = CreateRectRgn(0, 0, 5, 5);
    hrgn3 = CreateRectRgn(5, 5, 10, 10);
    hrgn4 = CreateRectRgn(0, 0, 0, 0);

    SetLastError(0xbadbabe);
    ok_long(CombineRgn(NULL, NULL, NULL, RGN_XOR), ERROR);
    ok_long(CombineRgn(hrgn1, NULL, NULL, RGN_XOR), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, NULL, RGN_XOR), ERROR);
    ok_long(CombineRgn(hrgn1, NULL, hrgn2, RGN_XOR), ERROR);
    ok_long(CombineRgn(hrgn1, GetStockObject(BLACK_PEN), hrgn2, RGN_XOR), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, GetStockObject(BLACK_PEN), RGN_XOR), ERROR);
    ok((GetLastError() == 0xbadbabe) || (GetLastError() == ERROR_INVALID_HANDLE), "wrong error: %ld\n", GetLastError());

    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_XOR), COMPLEXREGION);
    ok_long(CombineRgn(hrgn4, hrgn2, hrgn3, RGN_OR), COMPLEXREGION);
    ok(EqualRgn(hrgn1, hrgn4), "Region is not correct\n");

    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_XOR), SIMPLEREGION);
    ok(EqualRgn(hrgn1, hrgn3), "Region is not correct\n");
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn3, RGN_XOR), NULLREGION);


}

START_TEST(CombineRgn)
{
    Test_CombineRgn_Params();
    Test_CombineRgn_COPY();
    Test_CombineRgn_AND();
    Test_CombineRgn_OR();
    Test_CombineRgn_DIFF();
    Test_CombineRgn_XOR();
}

