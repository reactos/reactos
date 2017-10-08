/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CPoint, CSize, CRect
 * PROGRAMMER:      Mark Jansen
 *
 *                  Code based on MSDN samples regarding CPoint, CSize, CRect
 */

#include <apitest.h>
#include <windows.h>
#include <atltypes.h>


#define ok_size(x, y) \
    ok(x == y, "Wrong size, expected '%s' to equal '%s'\n", wine_dbgstr_size(&x), wine_dbgstr_size(&y))

#define ok_point(x, y) \
    ok(x == y, "Wrong point, expected '%s' to equal '%s'\n", wine_dbgstr_point(&x), wine_dbgstr_point(&y))
#define nok_point(x, y) \
    ok(x != y, "Wrong point, expected '%s' NOT to equal '%s'\n", wine_dbgstr_point(&x), wine_dbgstr_point(&y))

#define ok_rect(x, y) \
    ok(x == y, "Wrong rect, expected '%s' to equal '%s'\n", wine_dbgstr_rect(&x), wine_dbgstr_rect(&y))
#define nok_rect(x, y) \
    ok(x != y, "Wrong rect, expected '%s' to NOT equal '%s'\n", wine_dbgstr_rect(&x), wine_dbgstr_rect(&y))


static void test_CSize()
{
    CSize empty;
    ok(empty.cx == 0, "Expected cx to be 0, was %ld\n", empty.cx);
    ok(empty.cy == 0, "Expected cy to be 0, was %ld\n", empty.cy);

    CSize szPointA(10, 25);

    SIZE sz;
    sz.cx = 10;
    sz.cy = 25;
    CSize szPointB(sz);

    POINT pt;
    pt.x = 10;
    pt.y = 25;
    CSize szPointC(pt);

    CPoint ptObject(10, 25);
    CSize szPointD(ptObject);

    DWORD dw = MAKELONG(10, 25);
    CSize szPointE(dw);

    ok_size(szPointA, szPointB);
    ok_size(szPointB, szPointC);
    ok_size(szPointC, szPointD);
    ok_size(szPointD, szPointE);

    ptObject = szPointA + pt;
    CPoint res(20,50);
    ok_point(ptObject, res);

    ptObject = szPointA - pt;
    res = CPoint(0, 0);
    ok_point(ptObject, res);

    CSize sz1(135, 135);
    CSize sz2(135, 135);
    ok_size(sz1, sz2);

    sz1 = CSize(222, 222);
    sz2 = CSize(111, 111);
    ok(sz1 != sz2, "Wrong size, expected '%s' NOT to equal '%s'\n", wine_dbgstr_size(&sz1), wine_dbgstr_size(&sz2));

    sz1 = CSize(100, 100);
    sz2 = CSize(50, 25);
    sz1 += sz2;

    CSize szResult(150, 125);
    ok_size(sz1, szResult);

    sz1 = CSize(100, 100);
    SIZE sz3;
    sz3.cx = 50;
    sz3.cy = 25;

    sz1 += sz3;
    ok_size(sz1, szResult);

    sz1 = CSize(100, 100);
    sz1 -= sz2;

    szResult = CSize(50, 75);
    ok_size(sz1, szResult);

    sz3.cx = 50;
    sz3.cy = 25;

    sz1 = CSize(100, 100);
    sz1 -= sz3;
    ok_size(sz1, szResult);

    sz1 = CSize(100, 100);
    CSize szOut;
    szOut = sz1 + sz2;

    szResult = CSize(150, 125);
    ok_size(szOut, szResult);

    sz3.cx = 50;
    sz3.cy = 25;

    szOut = sz1 + sz3;
    ok_size(szOut, szResult);

    szOut = sz1 - sz2;

    szResult = CSize(50, 75);
    ok_size(szOut, szResult);

    sz3.cx = 50;
    sz3.cy = 25;

    szOut = sz1 - sz3;
    ok_size(szOut, szResult);

    szResult = CSize(-50, -75);

    szOut = -szOut;
    ok_size(szOut, szResult);

    RECT rc = { 1, 2, 3, 4 };

    CRect rcres = sz1 + &rc;
    CRect rcexp(101, 102, 103, 104);
    ok_rect(rcexp, rcres);

    rcres = sz1 - &rc;
    rcexp = CRect(-99, -98, -97, -96);
    ok_rect(rcexp, rcres);
}


static void test_CPoint()
{
    CPoint empty;

    ok(empty.x == 0, "Expected x to be 0, was %ld\n", empty.x);
    ok(empty.y == 0, "Expected y to be 0, was %ld\n", empty.y);

    CPoint ptTopLeft(0, 0);
    POINT ptHere;
    ptHere.x = 35;
    ptHere.y = 95;

    CPoint ptMFCHere(ptHere);

    SIZE sHowBig;
    sHowBig.cx = 300;
    sHowBig.cy = 10;

    CPoint ptMFCBig(sHowBig);
    DWORD dwSize;
    dwSize = MAKELONG(35, 95);

    CPoint ptFromDouble(dwSize);
    ok_point(ptFromDouble, ptMFCHere);

    CPoint ptStart(100, 100);
    ptStart.Offset(35, 35);

    CPoint ptResult(135, 135);
    ok_point(ptStart, ptResult);

    ptStart = CPoint(100, 100);
    POINT pt;

    pt.x = 35;
    pt.y = 35;

    ptStart.Offset(pt);
    ok_point(ptStart, ptResult);

    ptStart = CPoint(100, 100);
    SIZE size;

    size.cx = 35;
    size.cy = 35;

    ptStart.Offset(size);
    ok_point(ptStart, ptResult);

    CPoint ptFirst(256, 128);
    CPoint ptTest(256, 128);
    ok_point(ptFirst, ptTest);

    pt.x = 256;
    pt.y = 128;
    ok_point(ptTest, pt);

    ptTest = CPoint(111, 333);
    nok_point(ptFirst, ptTest);

    pt.x = 333;
    pt.y = 111;
    nok_point(ptTest, pt);

    ptStart = CPoint(100, 100);
    CSize szOffset(35, 35);

    ptStart += szOffset;

    ok_point(ptResult, ptStart);

    ptStart = CPoint(100, 100);

    ptStart += size;
    ok_point(ptResult, ptStart);

    ptStart = CPoint(100, 100);

    ptStart -= szOffset;

    ptResult = CPoint(65, 65);
    ok_point(ptResult, ptStart);


    ptStart = CPoint(100, 100);

    ptStart -= size;
    ok_point(ptResult, ptStart);

    ptStart = CPoint(100, 100);
    CPoint ptEnd;

    ptEnd = ptStart + szOffset;

    ptResult = CPoint(135, 135);
    ok_point(ptResult, ptEnd);

    ptEnd = ptStart + size;
    ok_point(ptResult, ptEnd);

    ptEnd = ptStart + pt;
    ptResult = CPoint(433, 211);
    ok_point(ptResult, ptEnd);

    ptEnd = ptStart - szOffset;
    ptResult = CPoint(65, 65);
    ok_point(ptResult, ptEnd);

    ptEnd = ptStart - size;
    ok_point(ptResult, ptEnd);

    szOffset = ptStart - pt;
    CSize expected(-233, -11);
    ok_size(szOffset, expected);

    ptStart += pt;
    ptResult = CPoint(433, 211);
    ok_point(ptResult, ptStart);

    ptStart -= pt;
    ptResult = CPoint(100, 100);
    ok_point(ptResult, ptStart);

    ptTest = CPoint(35, 35);
    ptTest = -ptTest;

    CPoint ptNeg(-35, -35);
    ok_point(ptTest, ptNeg);

    RECT rc = { 1, 2, 3, 4 };

    CRect rcres = ptStart + &rc;
    CRect rcexp(101, 102, 103, 104);
    ok_rect(rcexp, rcres);

    rcres = ptStart - &rc;
    rcexp = CRect(-99, -98, -97, -96);
    ok_rect(rcexp, rcres);
}


static void test_CRect()
{
    CRect empty;
    ok(empty.left == 0, "Expected left to be 0, was %ld\n", empty.left);
    ok(empty.top == 0, "Expected top to be 0, was %ld\n", empty.top);
    ok(empty.Width() == 0, "Expected Width to be 0, was %i\n", empty.Width());
    ok(empty.Height() == 0, "Expected Height to be 0, was %i\n", empty.Height());

    CRect rect(0, 0, 100, 50);
    ok(rect.Width() == 100, "Expected Width to be 100, was %i\n", rect.Width());
    ok(rect.Height() == 50, "Expected Height to be 50, was %i\n", rect.Height());

    RECT sdkRect;
    sdkRect.left = 0;
    sdkRect.top = 0;
    sdkRect.right = 100;
    sdkRect.bottom = 50;

    CRect rect2(sdkRect);
    CRect rect3(&sdkRect);
    ok_rect(rect2, rect);
    ok_rect(rect3, rect);

    CPoint pt(0, 0);
    CSize sz(100, 50);
    CRect rect4(pt, sz);
    ok_rect(rect4, rect2);

    CPoint ptBottomRight(100, 50);
    CRect rect5(pt, ptBottomRight);
    ok_rect(rect5, rect4);

    rect = CRect(210, 150, 350, 900);
    CPoint ptDown;

    ptDown = rect.BottomRight();

    pt = CPoint(350, 900);
    ok_point(ptDown, pt);

    rect2 = CRect(10, 10, 350, 350);
    CPoint ptLow(180, 180);

    rect2.BottomRight() = ptLow;

    rect = CRect(10, 10, 180, 180);
    ok_rect(rect2, rect);

    pt = CPoint(95, 95);
    CPoint pt2 = rect2.CenterPoint();
    ok_point(pt2, pt);

    pt2 = rect2.BottomRight();
    pt = CPoint(180, 180);
    ok_point(pt2, pt);

    pt2 = rect2.TopLeft();
    pt = CPoint(10, 10);
    ok_point(pt2, pt);

    rect2.TopLeft().Offset(3, 3);
    rect3 = CRect(13, 13, 180, 180);
    ok_rect(rect3, rect2);

    CRect rectSource(35, 10, 125, 10);
    CRect rectDest;

    rectDest.CopyRect(&rectSource);

    RECT rectSource2;
    rectSource2.left = 0;
    rectSource2.top = 0;
    rectSource2.bottom = 480;
    rectSource2.right = 640;

    rectDest.CopyRect(&rectSource2);

    rect = CRect(10, 10, 50, 50);

    rect.DeflateRect(1, 2);

    rect2 = CRect(11, 12, 49, 48);
    ok_rect(rect2, rect);

    rect2 = CRect(10, 10, 50, 50);
    CRect rectDeflate(1, 2, 3, 4);

    rect2.DeflateRect(&rectDeflate);
    rect = CRect(11, 12, 47, 46);
    ok_rect(rect2, rect);

    rect2.DeflateRect(sz);
    rect = CRect(111, 62, -53, -4);
    ok_rect(rect2, rect);

    rect2.OffsetRect(sz);
    rect = CRect(211, 112, 47, 46);
    ok_rect(rect2, rect);

    CRect rect1(35, 150, 10, 25);
    rect2 = CRect(35, 150, 10, 25);
    rect3 = CRect(98, 999, 6, 3);

    ok(rect1.EqualRect(rect2), "Expected EqualRect to return TRUE for %s, %s\n", wine_dbgstr_rect(&rect1), wine_dbgstr_rect(&rect2));
    ok(!rect1.EqualRect(rect3), "Expected EqualRect to return FALSE for %s, %s\n", wine_dbgstr_rect(&rect1), wine_dbgstr_rect(&rect3));

    RECT test;
    test.left = 35;
    test.top = 150;
    test.right = 10;
    test.bottom = 25;

    ok(rect1.EqualRect(&test), "Expected EqualRect to return TRUE for %s, %s\n", wine_dbgstr_rect(&rect1), wine_dbgstr_rect(&test));

    rect = test;
    rect2 = CRect(35, 150, 10, 25);
    ok_rect(rect, rect2);

    rect = CRect(0, 0, 300, 300);
    rect.InflateRect(50, 200);

    rect2 = CRect(-50, -200, 350, 500);
    ok_rect(rect, rect2);

    rect.InflateRect(sz);
    rect2 = CRect(-150, -250, 450, 550);
    ok_rect(rect, rect2);

    rect = CRect(20, 30, 80, 70);

    int nHt = rect.Height();

    ok(nHt == 40, "Expected nHt to be 40, was %i\n", nHt);

    CRect rectOne(125, 0, 150, 200);
    CRect rectTwo(0, 75, 350, 95);
    CRect rectInter;

    rectInter.IntersectRect(rectOne, rectTwo);

    rect = CRect(125, 75, 150, 95);
    ok_rect(rectInter, rect);

    CRect rectInter2 = rectOne;
    rectInter2 &= rectTwo;
    rect = CRect(125, 75, 150, 95);
    ok_rect(rectInter2, rect);

    CRect rectNone(0, 0, 0, 0);
    CRect rectSome(35, 50, 135, 150);

    ok(rectNone.IsRectEmpty(), "Expected IsRectEmpty to return TRUE for %s\n", wine_dbgstr_rect(&rectNone));
    ok(!rectSome.IsRectEmpty(), "Expected IsRectEmpty to return FALSE for %s\n", wine_dbgstr_rect(&rectSome));

    CRect rectEmpty(35, 35, 35, 35);
    ok(rectEmpty.IsRectEmpty(), "Expected IsRectEmpty to return TRUE for %s\n", wine_dbgstr_rect(&rectEmpty));

    ok(rectNone.IsRectNull(), "Expected IsRectNull to return TRUE for %s\n", wine_dbgstr_rect(&rectNone));
    ok(!rectSome.IsRectNull(), "Expected IsRectNull to return FALSE for %s\n", wine_dbgstr_rect(&rectSome));

    CRect rectNotNull(0, 0, 35, 50);
    ok(!rectNotNull.IsRectNull(), "Expected IsRectNull to return FALSE for %s\n", wine_dbgstr_rect(&rectNotNull));

    rect1 = CRect(35, 150, 10, 25);
    rect2 = CRect(35, 150, 10, 25);
    rect3 = CRect(98, 999, 6, 3);

    ok_rect(rect1, rect2);

    test.left = 35;
    test.top = 150;
    test.right = 10;
    test.bottom = 25;

    ok_rect(rect1, test);

    nok_rect(rect1, rect3);
    nok_rect(rect3, test);

    rect1 = CRect(100, 235, 200, 335);
    pt = CPoint(35, 65);
    rect2 = CRect(135, 300, 235, 400);

    rect1 += pt;

    ok_rect(rect1, rect2);

    rect1 = CRect(100, 235, 200, 335);
    rect2 = rect1 + pt;
    CRect rectResult(135, 300, 235, 400);
    ok_rect(rectResult, rect2);

    rect2 = rect1 + &test;
    rectResult = CRect(65, 85, 210, 360);
    ok_rect(rectResult, rect2);

    rect2 = rect1 - (LPCRECT)&test;
    rectResult = CRect(135, 385, 190, 310);
    ok_rect(rectResult, rect2);

    rect2 = rect1 - pt;
    rectResult = CRect(65, 170, 165, 270);

    ok_rect(rect2, rectResult);

    rect1 -= pt;
    ok_rect(rect1, rectResult);

    rect1 = CRect(100, 0, 200, 300);
    rect2 = CRect(0, 100, 300, 200);

    rect3 = rect1 & rect2;

    rectResult = CRect(100, 100, 200, 200);
    ok_rect(rectResult, rect3);

    rect3 = rect1 | rect2;
    rectResult = CRect(0, 0, 300, 300);
    ok_rect(rectResult, rect3);

    rect1 |= rect2;
    ok_rect(rectResult, rect1);

    rect1 += sz;
    rectResult = CRect(100, 50, 400, 350);
    ok_rect(rectResult, rect1);

    rect1 += &test;
    rectResult = CRect(65, -100, 410, 375);
    ok_rect(rectResult, rect1);

    rect1 -= sz;
    rectResult = CRect(-35, -150, 310, 325);
    ok_rect(rectResult, rect1);

    rect1 -= &test;
    rectResult = CRect(0, 0, 300, 300);
    ok_rect(rectResult, rect1);

    rect2 = rect1 + sz;
    rectResult = CRect(100, 50, 400, 350);
    ok_rect(rectResult, rect2);

    rect2 = rect1 - sz;
    rectResult = CRect(-100, -50, 200, 250);
    ok_rect(rectResult, rect2);
}


START_TEST(atltypes)
{
    test_CSize();
    test_CPoint();
    test_CRect();
}
