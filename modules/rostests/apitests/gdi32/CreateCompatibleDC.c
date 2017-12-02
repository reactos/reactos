/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CreateCompatibleDC
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

void Test_CreateCompatibleDC()
{
	HDC hdcScreen, hOldDC, hdc, hdc2;
	HPEN hOldPen;
	COLORREF color;

	/* Get screen DC */
	hdcScreen = GetDC(NULL);

	/* Test NULL DC handle */
	SetLastError(ERROR_SUCCESS);
	hdc = CreateCompatibleDC(NULL);
	ok(hdc != NULL, "CreateCompatibleDC(NULL) failed\n");
	ok(GetLastError() == ERROR_SUCCESS, "GetLastError() == %ld\n", GetLastError());
	if(hdc) DeleteDC(hdc);

	/* Test invalid DC handle */
	SetLastError(ERROR_SUCCESS);
	hdc = CreateCompatibleDC((HDC)0x123456);
	ok(hdc == NULL, "Expected NULL, got %p\n", hdc);
	ok(GetLastError() == ERROR_SUCCESS, "GetLastError() == %ld\n", GetLastError());
	if(hdc) DeleteDC(hdc);

	hdc = CreateCompatibleDC(hdcScreen);
	ok(hdc != NULL, "CreateCompatibleDC failed\n");

	// Test if first selected pen is BLACK_PEN (? or same as screen DC's pen?)
	hOldPen = SelectObject(hdc, GetStockObject(DC_PEN));
	ok (hOldPen == GetStockObject(BLACK_PEN), "hOldPen == %p\n", hOldPen);
	hOldPen = SelectObject(hdc, GetStockObject(BLACK_PEN));
	ok (hOldPen == GetStockObject(DC_PEN), "hOldPen == %p\n", hOldPen);

	/* Test for the starting Color == RGB(0,0,0) */
	color = SetDCPenColor(hdc, RGB(1,2,3));
	ok(color == RGB(0,0,0), "color == %lx\n", color);

	/* Check for reuse counter */
	hOldDC = hdc;
	DeleteDC(hdc);
	hdc = CreateCompatibleDC(hdcScreen);
	hdc2 = CreateCompatibleDC(hOldDC);
	ok(hdc2 == NULL, "Expected NULL, got %p\n", hdc);
	if (hdc2 != NULL) DeleteDC(hdc2);

    /* Check map mode */
	hdc = CreateCompatibleDC(hdcScreen);
	SetMapMode(hdc, MM_ISOTROPIC);
	hdc2 = CreateCompatibleDC(hdc);
    ok(GetMapMode(hdc2) == MM_TEXT, "GetMapMode(hdc2)==%d\n", GetMapMode(hdc2));

	/* cleanup */
	DeleteDC(hdc);

	ReleaseDC(NULL, hdcScreen);
}

START_TEST(CreateCompatibleDC)
{
    Test_CreateCompatibleDC();
}

