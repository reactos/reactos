
INT
Test_AddFontResourceEx(PTESTINFO pti)
{
	WCHAR szFileName[MAX_PATH];
	GetEnvironmentVariableW(L"systemroot", szFileName, MAX_PATH);

	wcscat(szFileName, L"\\Fonts\\cour.ttf");

	SetLastError(ERROR_SUCCESS);
	TEST(AddFontResourceExW(szFileName, 0, 0) != 0);
	TEST(GetLastError() == ERROR_SUCCESS);

	SetLastError(ERROR_SUCCESS);
	TEST(AddFontResourceExW(szFileName, 256, 0) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	return APISTATUS_NORMAL;
}
