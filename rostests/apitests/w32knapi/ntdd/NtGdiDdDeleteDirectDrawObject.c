
INT
Test_NtGdiDdDeleteDirectDrawObject(PTESTINFO pti)
{
	HANDLE  hDirectDraw;
	HDC hdc = CreateDCW(L"DISPLAY",NULL,NULL,NULL);
	ASSERT(hdc != NULL);

	/* Test ReactX */
	RTEST(NtGdiDdDeleteDirectDrawObject(NULL) == FALSE);
	RTEST((hDirectDraw=NtGdiDdCreateDirectDrawObject(hdc)) != NULL);
	ASSERT(hDirectDraw != NULL);
	RTEST(NtGdiDdDeleteDirectDrawObject(hDirectDraw) == TRUE);

	/* Cleanup ReactX setup */
	DeleteDC(hdc);
	Syscall(L"NtGdiDdDeleteDirectDrawObject", 1, &hDirectDraw);

	return APISTATUS_NORMAL;
}
