#line 2 "CreateCompatibleDC.c"

#include "../gditest.h"

BOOL Test_CreateCompatibleDC(INT* passed, INT* failed)
{
	HDC hDCScreen, hOldDC, hDC, hDC2;

	// Create a DC
	hDCScreen = GetDC(NULL);
	if (hDCScreen == NULL)
	{
		return FALSE;
	}

	hDC = CreateCompatibleDC(hDCScreen);
	TEST(hDC != NULL);

	// Test if first selected pen is BLACK_PEN (? or same as screen DC's pen?)
	TEST(SelectObject(hDC, GetStockObject(DC_PEN)) == GetStockObject(BLACK_PEN));
	TEST(SelectObject(hDC, GetStockObject(BLACK_PEN)) == GetStockObject(DC_PEN));

	// Test for the starting Color == RGB(0,0,0)
	TEST(SetDCPenColor(hDC, RGB(1,2,3)) == RGB(0,0,0));

	// Check for reuse counter
	hOldDC = hDC;
	DeleteDC(hDC);
	hDC = CreateCompatibleDC(hDCScreen);
	hDC2 = CreateCompatibleDC(hOldDC);
	TEST(hDC2 == NULL);
	if (hDC2 != NULL) DeleteDC(hDC2);

	// cleanup
	DeleteDC(hDC);

	ReleaseDC(NULL, hDCScreen);
	return TRUE;
}
