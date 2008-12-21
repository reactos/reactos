
LONG
QueryUserRegValueW(LPWSTR pszSubKey, LPWSTR pszValueName, PVOID pData, LPDWORD cbSize, LPDWORD pType)
{
	HKEY hKey;
	LONG ret;

	RegOpenKeyExW(HKEY_CURRENT_USER, pszSubKey, 0, KEY_QUERY_VALUE, &hKey);
	ret = RegQueryValueExW(hKey, pszValueName, NULL, pType, (LPBYTE)pData, cbSize);
	RegCloseKey(hKey);
	return ret;
}


HWND
CreateTestWindow()
{
	WNDCLASSA wc;

	wc.style = 0;
	wc.lpfnWndProc = DefWindowProcA;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_hInstance;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "testclass";

	RegisterClassA(&wc);
	return CreateWindowA("testclass",
	                     "testwnd",
	                     WS_VISIBLE,
	                     0,
	                     0,
	                     50,
	                     30,
	                     NULL,
	                     NULL,
	                     g_hInstance,
	                     0);

}

void
Test_SPI_87_88(PTESTINFO pti)
{
	DWORD dwRet = 0xdeadbeef;

	TEST(NtUserSystemParametersInfo(87, 0, &dwRet, 0) == TRUE);
	TEST(dwRet == 0xdeadbeef);
	dwRet++;
	TEST(NtUserSystemParametersInfo(87, 0, &dwRet, 0) == TRUE);


	dwRet--;
	TEST(NtUserSystemParametersInfo(87, 0, &dwRet, 0) == TRUE);


}

void
Test_NtUserSystemParametersInfo_Params(PTESTINFO pti)
{
//	UINT uint;
	DWORD data[1000];
	UINT i, uint;
#if 1
	/* Test normal */
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(SPI_GETFOCUSBORDERHEIGHT, 0, &uint, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid SPI code */
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(7, 0, &data, 0) == FALSE);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* Test wrong cbSize member */
	ACCESSTIMEOUT ato;
	ato.cbSize = 1;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(SPI_GETACCESSTIMEOUT, 0, &ato, 0) == FALSE);
	TEST(GetLastError() == ERROR_SUCCESS);
#endif
	/* Test undocumented, but valid SPI codes */
	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4112, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] <= 1);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4113, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);
	data[0] = 0;
	NtUserSystemParametersInfo(4112, 0, &data, 0);
	TEST(data[0] == 1);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4136, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4137, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4138, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4139, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4140, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4141, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4142, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4143, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4144, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4145, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4146, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4147, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4148, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4149, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4150, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4151, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4152, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4153, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4154, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4155, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4156, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] != 0xdeadbeef);
	TEST(data[1] == 0xdeadbeef);

	for(i = 0; i < 1000; i++) data[i] = 0xbeefdead;
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(4157, 0, &data, 0) == TRUE);
	TEST(GetLastError() == ERROR_SUCCESS);
	TEST(data[0] == 0xbeefdead);
	TEST(data[1] == 0xbeefdead);

	/* Test invalid pointer */
	SetLastError(ERROR_SUCCESS);
	TEST(NtUserSystemParametersInfo(SPI_GETFOCUSBORDERHEIGHT, 0, (PVOID)0x80000000, 0) == FALSE);
	TEST(GetLastError() == ERROR_NOACCESS);
	for(i = 0; i < 1000; i++) data[i] = 0xdeadbeef;

	/* test wrong bools */
}

void
Test_NtUserSystemParametersInfo_fWinIni(PTESTINFO pti)
{
	ACCESSTIMEOUT ato;
//	UINT uFocusBorderHeight;
	WCHAR Buffer[6];
	DWORD cbSize;

	ato.cbSize = sizeof(ato);
	NtUserSystemParametersInfo(SPI_GETACCESSTIMEOUT, 0, &ato, 0);
	ato.iTimeOutMSec++;
	NtUserSystemParametersInfo(SPI_SETACCESSTIMEOUT, 0, &ato, 0);
	ato.iTimeOutMSec--;

//	NtUserSystemParametersInfo(SPI_GETFOCUSBORDERHEIGHT, 0, &uFocusBorderHeight, 0);
//	NtUserSystemParametersInfo(SPI_SETFOCUSBORDERHEIGHT, 0, &uFocusBorderHeight, SPIF_UPDATEINIFILE);

	NtUserSystemParametersInfo(SPI_SETACCESSTIMEOUT, 0, &ato, 0);

	cbSize = 6;
	QueryUserRegValueW(L"Control Panel\\Accessibility\\TimeOut", L"TimeToWait", &Buffer, &cbSize, NULL);

}

void
Test_SPI_GETSETBEEP(PTESTINFO pti)
{
	BOOL bOrig, bTemp = 0;
	WCHAR szReg[10];
	DWORD cbSize;

	/* Get original value */
	NtUserSystemParametersInfo(SPI_GETBEEP, 0, &bOrig, 0);

	/* Value 0 */
	NtUserSystemParametersInfo(SPI_SETBEEP, 0, NULL, SPIF_UPDATEINIFILE);
	NtUserSystemParametersInfo(SPI_GETBEEP, 0, &bTemp, 0);
	TEST(bTemp == 0);
	cbSize = sizeof(szReg);
	TEST(QueryUserRegValueW(L"Control Panel\\Sound", L"Beep", &szReg, &cbSize, NULL) == ERROR_SUCCESS);
	TEST(_wcsicmp(szReg, L"No") == 0);

	/* Value 1 */
	NtUserSystemParametersInfo(SPI_SETBEEP, 1, NULL, SPIF_UPDATEINIFILE);
	NtUserSystemParametersInfo(SPI_GETBEEP, 0, &bTemp, 0);
	TEST(bTemp == 1);
	cbSize = sizeof(szReg);
	TEST(QueryUserRegValueW(L"Control Panel\\Sound", L"Beep", &szReg, &cbSize, NULL) == ERROR_SUCCESS);
	TEST(_wcsicmp(szReg, L"Yes") == 0);

	/* Value 2 */
	NtUserSystemParametersInfo(SPI_SETBEEP, 2, NULL, SPIF_UPDATEINIFILE);
	NtUserSystemParametersInfo(SPI_GETBEEP, 0, &bTemp, 0);
	TEST(bTemp == 1);
	cbSize = sizeof(szReg);
	TEST(QueryUserRegValueW(L"Control Panel\\Sound", L"Beep", &szReg, &cbSize, NULL) == ERROR_SUCCESS);
	TEST(_wcsicmp(szReg, L"Yes") == 0);

	/* Restore original value */
	NtUserSystemParametersInfo(SPI_SETBEEP, 0, &bOrig, SPIF_UPDATEINIFILE);
}

INT
Test_SPI_SETDESKWALLPAPER(PTESTINFO pti)
{
	/* Get old Wallpaper */
//	NtUserSystemParametersInfo(SPI_GET_DESKWALLPAPER, 0,

//	TEST(NtUserSystemParametersInfo(SPI_SETDESKWALLPAPER, 0, &hNewWallPaper, 0) == 0);
	return 0;
}

INT
Test_SPI_GETSTICKYKEYS(PTESTINFO pti)
{
	STICKYKEYS sk;

	sk.cbSize = sizeof(STICKYKEYS)+1;
	TEST(NtUserSystemParametersInfo(SPI_GETSTICKYKEYS, 0, &sk, 0) == 0);


	sk.cbSize = sizeof(STICKYKEYS);
	NtUserSystemParametersInfo(SPI_GETSTICKYKEYS, 0, &sk, 0);
	printf("sk.dwFlags = %lx\n", sk.dwFlags);

	return APISTATUS_NORMAL;
}

INT
Test_NtUserSystemParametersInfo(PTESTINFO pti)
{
	HWND hWnd;

	hWnd = CreateTestWindow();
	ASSERT(hWnd);
	Test_NtUserSystemParametersInfo_Params(pti);

	Test_NtUserSystemParametersInfo_fWinIni(pti);

	Test_SPI_GETSETBEEP(pti);
	Test_SPI_SETDESKWALLPAPER(pti);

	Test_SPI_GETSTICKYKEYS(pti);

	Test_SPI_87_88(pti);

	DestroyWindow(hWnd);

	return APISTATUS_NORMAL;
}
