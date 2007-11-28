#define STAMP_DESIGNVECTOR (0x8000000 + 'd' + ('v' << 8))

INT
Test_AddFontResourceEx(PTESTINFO pti)
{
	WCHAR szFileName[MAX_PATH];

	/* Test NULL filename */
	SetLastError(ERROR_SUCCESS);
	/* Windows crashes, would need SEH here */
//	TEST(AddFontResourceExW(NULL, 0, 0) != 0);
//	TEST(GetLastError() == ERROR_SUCCESS);

	/* Test "" filename */
	SetLastError(ERROR_SUCCESS);
	TEST(AddFontResourceExW(L"", 0, 0) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	GetEnvironmentVariableW(L"systemroot", szFileName, MAX_PATH);
	wcscat(szFileName, L"\\Fonts\\cour.ttf");

	/* Test flags = 0 */
	SetLastError(ERROR_SUCCESS);
	TEST(AddFontResourceExW(szFileName, 0, 0) != 0);
	TEST(GetLastError() == ERROR_SUCCESS);

	SetLastError(ERROR_SUCCESS);
	TEST(AddFontResourceExW(szFileName, 256, 0) == 0);
	TEST(GetLastError() == ERROR_INVALID_PARAMETER);

	/* Test invalid pointer as last parameter */
	TEST(AddFontResourceExW(szFileName, 0, (void*)-1) != 0);


	return APISTATUS_NORMAL;
}
