INT
Test_NtUserGetIconInfo(PTESTINFO pti)
{
	HICON hIcon;
	ICONINFO iinfo;
	HBITMAP mask, color;

	ZeroMemory(&iinfo, sizeof(ICONINFO));

	/* BASIC TESTS */
	hIcon = (HICON) NtUserCallOneParam(0, _ONEPARAM_ROUTINE_CREATEEMPTYCUROBJECT);
	TEST(hIcon != NULL);

	/* Last param is unknown */
	TEST(NtUserGetIconInfo(hIcon, &iinfo, NULL, NULL, NULL, FALSE) == FALSE);
	TEST(NtUserGetIconInfo(hIcon, &iinfo, NULL, NULL, NULL, TRUE) == FALSE);

	TEST(NtUserDestroyCursor(hIcon, 0) == TRUE);

	mask = CreateBitmap(16,16,1,1,NULL);
	color = CreateBitmap(16,16,1,16,NULL);

	iinfo.hbmMask = mask;
	iinfo.hbmColor = color ;
	iinfo.fIcon = TRUE;
	iinfo.xHotspot = 8;
	iinfo.yHotspot = 8;

	hIcon = CreateIconIndirect(&iinfo);
	TEST(hIcon!=NULL);

	// TODO : test last parameter...
	TEST(NtUserGetIconInfo(hIcon, &iinfo, NULL, NULL, NULL, FALSE) == TRUE);

	TEST(iinfo.hbmMask != NULL);
	TEST(iinfo.hbmColor != NULL);
	TEST(iinfo.fIcon == TRUE);
	TEST(iinfo.yHotspot == 8);
	TEST(iinfo.xHotspot == 8);

	TEST(iinfo.hbmMask != mask);
	TEST(iinfo.hbmColor != color);

	/* Does it make a difference? */
	TEST(NtUserGetIconInfo(hIcon, &iinfo, NULL, NULL, NULL, TRUE) == TRUE);

	TEST(iinfo.hbmMask != NULL);
	TEST(iinfo.hbmColor != NULL);
	TEST(iinfo.fIcon == TRUE);
	TEST(iinfo.yHotspot == 8);
	TEST(iinfo.xHotspot == 8);

	TEST(iinfo.hbmMask != mask);
	TEST(iinfo.hbmColor != color);

	DeleteObject(mask);
	DeleteObject(color);

	DestroyIcon(hIcon);

	return APISTATUS_NORMAL;
}