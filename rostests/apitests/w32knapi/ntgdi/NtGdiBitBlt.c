
INT
Test_NtGdiBitBlt(PTESTINFO pti)
{
	BOOL bRet;

	/* Test invalid dc */
	SetLastError(ERROR_SUCCESS);
	bRet = NtGdiBitBlt((HDC)0x123456, 0, 0, 10, 10, (HDC)0x123456, 10, 10, SRCCOPY, 0, 0);
	TEST(bRet == FALSE);
	TEST(GetLastError() == ERROR_SUCCESS);

	return APISTATUS_NORMAL;
}
