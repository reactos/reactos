
void
ReadBits(HDC hDC, PDWORD OutBits)
{
	int x,y;

	for (y = 0; y < 8; y++)
	{
		DWORD Row = 0;
		for (x = 0; x < 8; x++)
			Row |= (0x80 & GetPixel(hDC, 2 + x, 3 + y)) >> x;
		OutBits[y] = Row;
	}
}


INT
Test_NtGdiSetDIBitsToDeviceInternal(PTESTINFO pti)
{
	static const DWORD InBits[8] = { 0x81, 0x7E, 0x5A, 0x7E, 0x7E, 0x42, 0x7E, 0x81 };
	DWORD OutBits[8];
	XFORM xform;

	HWND hWnd = CreateWindowW(L"Static", NULL, WS_VISIBLE,
	                          100, 100, 200, 200,
	                          NULL, NULL, NULL, NULL);
	/* This DC has an nonzero origin */
	HDC hDC = GetDC(hWnd);
	struct
	{
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD bmiColors[2];
	} bmi;
	int x, y;

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = 8;
	bmi.bmiHeader.biHeight = -8;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 1;
	bmi.bmiHeader.biCompression = 0;
	bmi.bmiHeader.biSizeImage = 0;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;
	*(DWORD *)&bmi.bmiColors[0] = 0x000000;
	*(DWORD *)&bmi.bmiColors[1] = 0xFFFFFF;

	/* The destination coordinates are relative to the DC origin */
	TEST(NtGdiSetDIBitsToDeviceInternal(hDC, 2, 3, 8, 8, 0, 0, 0, 8,
	                                    (PVOID)InBits, (BITMAPINFO *)&bmi, DIB_RGB_COLORS,
	                                    sizeof(InBits), sizeof(bmi), TRUE, NULL));

	/* Now get the data from the screen, and see if it matches */
	ReadBits(hDC, OutBits);

	TEST(memcmp(InBits, OutBits, sizeof(InBits)) == 0);

	/* Change transformation */
	GetWorldTransform(hDC, &xform);
	xform.eM11 = 2;
	xform.eM22 = 2;
	xform.eDx = 10;
	SetWorldTransform(hDC, &xform);

	TEST(NtGdiSetDIBitsToDeviceInternal(hDC, 2, 3, 8, 8, 0, 0, 0, 8,
	                                    (PVOID)InBits, (BITMAPINFO *)&bmi, DIB_RGB_COLORS,
	                                    sizeof(InBits), sizeof(bmi), TRUE, NULL));

	xform.eM11 = 1;
	xform.eM22 = 1;
	xform.eDx = 0;
	SetWorldTransform(hDC, &xform);

	/* Now get the data from the screen, and see if it matches */
	for (y = 0; y < 8; y++)
	{
		DWORD Row = 0;
		for (x = 0; x < 8; x++)
			Row |= (0x80 & GetPixel(hDC, 2 + x, 3 + y)) >> x;
		OutBits[y] = Row;
	}

	TEST(memcmp(InBits, OutBits, sizeof(InBits)) == 0);


	ReleaseDC(hWnd, hDC);
	DestroyWindow(hWnd);

	return APISTATUS_NORMAL;
}
