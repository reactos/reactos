INT
Test_SetDCPenColor(PTESTINFO pti)
{
	HDC hScreenDC, hDC;
	HBITMAP hbmp;

	// Test an incorrect DC
	SetLastError(ERROR_SUCCESS);
	RTEST(SetDCPenColor(0, RGB(0,0,0)) == CLR_INVALID);
	RTEST(GetLastError() == ERROR_INVALID_PARAMETER);

	// Get the Screen DC
	hScreenDC = GetDC(NULL);
	if (hScreenDC == NULL) return FALSE;

	// Test the screen DC
	SetDCPenColor(hScreenDC, RGB(1,2,3));
	RTEST(SetDCPenColor(hScreenDC, RGB(4,5,6)) == RGB(1,2,3));

	// Create a new DC
	hDC = CreateCompatibleDC(hScreenDC);
	ReleaseDC(0, hScreenDC);
	ASSERT(hDC);

	// Select the DC_PEN and check if the pen returned by a new call is DC_PEN
	SelectObject(hDC, GetStockObject(DC_PEN));
	RTEST(SelectObject(hDC, GetStockObject(BLACK_PEN)) == GetStockObject(DC_PEN));

	// Test an incorrect color, yes windows sets the color!
	SetDCPenColor(hDC, 0x21123456);
	RTEST(SetDCPenColor(hDC, RGB(0,0,0)) == 0x21123456);

	// Test CLR_INVALID, it sets CLR_INVALID!
	SetDCPenColor(hDC, CLR_INVALID);
	RTEST(SetDCPenColor(hDC, RGB(0,0,0)) == CLR_INVALID);

	hbmp = CreateBitmap(10, 10, 1, 32, NULL);
	ASSERT(hbmp);

	SelectObject(hDC, hbmp);
	SelectObject(hDC, GetStockObject(DC_PEN));
	SetDCPenColor(hDC, 0x123456);
	MoveToEx(hDC, 0, 0, NULL);
	LineTo(hDC, 10, 0);
	TEST(GetPixel(hDC, 5, 0) == 0x123456);

	// Delete the DC
	DeleteDC(hDC);

	return APISTATUS_NORMAL;
}
