/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiGetFontResourceInfoInternalW
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiGetFontResourceInfoInternalW)
{
	BOOL bRet;
	DWORD dwBufSize;
	LOGFONTW logfont;
	UNICODE_STRING NtFileName;

	ASSERT(RtlDosPathNameToNtPathName_U(L".\\test.otf",
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

//	RemoveFontResourceW(szFullFileName);

}

