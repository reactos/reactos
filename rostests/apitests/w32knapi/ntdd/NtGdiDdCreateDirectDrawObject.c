
INT
Test_NtGdiDdCreateDirectDrawObject(PTESTINFO pti)
{
	HANDLE  hDirectDraw;
	HDC hdc = CreateDCW(L"DISPLAY",NULL,NULL,NULL);
	ASSERT(hdc != NULL);

	/* Test ReactX */
	RTEST(NtGdiDdCreateDirectDrawObject(NULL) == NULL);
	RTEST((hDirectDraw=NtGdiDdCreateDirectDrawObject(hdc)) != NULL);

	/* Cleanup ReactX setup */
	DeleteDC(hdc);
	NtGdiDdDeleteDirectDrawObject(hDirectDraw);

	return APISTATUS_NORMAL;
}
