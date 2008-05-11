
INT
Test_NtGdiGetFontResourceInfoInternalW(PTESTINFO pti)
{
	WCHAR szFullFileName[MAX_PATH+1];
	BOOL bRet;
	DWORD dwBufSize;
	LOGFONTW logfont;
	UNICODE_STRING NtFileName;

	GetCurrentDirectoryW(MAX_PATH, szFullFileName);
	wcscat(szFullFileName, L"\\test.otf");

	ASSERT(AddFontResourceW(szFullFileName) != 0);

	ASSERT(RtlDosPathNameToNtPathName_U(szFullFileName,
	                                    &NtFileName,
	                                    NULL,
	                                    NULL));

	dwBufSize = sizeof(logfont);
	memset(&logfont, 0x0, dwBufSize);

	bRet = NtGdiGetFontResourceInfoInternalW(
		NtFileName.Buffer,
		(NtFileName.Length / sizeof(WCHAR)) +1,
		1,
		dwBufSize,
		&dwBufSize,
		&logfont,
		2);

	TEST(bRet != FALSE);

	printf("lfHeight = %ld\n", logfont.lfHeight);
	printf("lfWidth = %ld\n", logfont.lfWidth);
	printf("lfFaceName = %ls\n", logfont.lfFaceName);

	RemoveFontResourceW(szFullFileName);

	return APISTATUS_NORMAL;
}

