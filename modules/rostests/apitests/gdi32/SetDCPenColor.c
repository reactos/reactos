/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetDCPenColor
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

void Test_SetDCPenColor()
{
	HDC hScreenDC, hDC;
	HBITMAP hbmp, hbmpOld;

	// Test an incorrect DC
	SetLastError(ERROR_SUCCESS);
	ok(SetDCPenColor(0, RGB(0,0,0)) == CLR_INVALID, "\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER, "\n");

	// Get the Screen DC
	hScreenDC = GetDC(NULL);
	ok(hScreenDC != 0, "GetDC failed, skipping tests\n");
	if (hScreenDC == NULL) return;

	// Test the screen DC
	SetDCPenColor(hScreenDC, RGB(1,2,3));
	ok(SetDCPenColor(hScreenDC, RGB(4,5,6)) == RGB(1,2,3), "\n");

	// Create a new DC
	hDC = CreateCompatibleDC(hScreenDC);
	ReleaseDC(0, hScreenDC);
	ok(hDC != 0, "CreateCompatibleDC failed, skipping tests\n");
	if (!hDC) return;

	// Select the DC_PEN and check if the pen returned by a new call is DC_PEN
	SelectObject(hDC, GetStockObject(DC_PEN));
	ok(SelectObject(hDC, GetStockObject(BLACK_PEN)) == GetStockObject(DC_PEN), "\n");

	// Test an incorrect color, yes windows sets the color!
	SetDCPenColor(hDC, 0x21123456);
	ok(SetDCPenColor(hDC, RGB(0,0,0)) == 0x21123456, "\n");

	// Test CLR_INVALID, it sets CLR_INVALID!
	SetDCPenColor(hDC, CLR_INVALID);
	ok(SetDCPenColor(hDC, RGB(0,0,0)) == CLR_INVALID, "\n");

	hbmp = CreateBitmap(10, 10, 1, 32, NULL);
	ok(hbmp != 0, "CreateBitmap failed, skipping tests\n");
	if (!hbmp) return;
	hbmpOld = SelectObject(hDC, hbmp);
#if 0 // this only works on 32 bpp screen resolution
	ok(hbmpOld != NULL, "\n");
	SelectObject(hDC, GetStockObject(DC_PEN));
	SetDCPenColor(hDC, 0x123456);
	MoveToEx(hDC, 0, 0, NULL);
	LineTo(hDC, 10, 0);
	ok(GetPixel(hDC, 5, 0) == 0x123456, "\n");
#endif

	// Delete the DC
	SelectObject(hDC, hbmpOld);
	DeleteDC(hDC);
}

START_TEST(SetDCPenColor)
{
    Test_SetDCPenColor();
}

