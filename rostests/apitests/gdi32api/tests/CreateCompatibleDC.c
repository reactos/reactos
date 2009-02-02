INT
Test_CreateCompatibleDC(PTESTINFO pti)
{
	HDC hDCScreen, hOldDC, hDC, hDC2;

	/* Get screen DC */
	hDCScreen = GetDC(NULL);
	ASSERT(hDCScreen != NULL);

	/* Test NULL DC handle */
	SetLastError(ERROR_SUCCESS);
	hDC = CreateCompatibleDC(NULL);
	RTEST(hDC != NULL);
	RTEST(GetLastError() == ERROR_SUCCESS);
	if(hDC) DeleteDC(hDC);

	/* Test invalid DC handle */
	SetLastError(ERROR_SUCCESS);
	hDC = CreateCompatibleDC((HDC)0x123456);
	RTEST(hDC == NULL);
	RTEST(GetLastError() == ERROR_SUCCESS);
	if(hDC) DeleteDC(hDC);

	hDC = CreateCompatibleDC(hDCScreen);
	RTEST(hDC != NULL);

	// Test if first selected pen is BLACK_PEN (? or same as screen DC's pen?)
	RTEST(SelectObject(hDC, GetStockObject(DC_PEN)) == GetStockObject(BLACK_PEN));
	RTEST(SelectObject(hDC, GetStockObject(BLACK_PEN)) == GetStockObject(DC_PEN));

	// Test for the starting Color == RGB(0,0,0)
	RTEST(SetDCPenColor(hDC, RGB(1,2,3)) == RGB(0,0,0));

	// Check for reuse counter
	hOldDC = hDC;
	DeleteDC(hDC);
	hDC = CreateCompatibleDC(hDCScreen);
	hDC2 = CreateCompatibleDC(hOldDC);
	RTEST(hDC2 == NULL);
	if (hDC2 != NULL) DeleteDC(hDC2);

    /* Check map mode */
	hDC = CreateCompatibleDC(hDCScreen);
	SetMapMode(hDC, MM_ISOTROPIC);
	hDC2 = CreateCompatibleDC(hDC);
    TEST(GetMapMode(hDC2) == MM_TEXT);

	// cleanup
	DeleteDC(hDC);

	ReleaseDC(NULL, hDCScreen);
	return APISTATUS_NORMAL;
}
