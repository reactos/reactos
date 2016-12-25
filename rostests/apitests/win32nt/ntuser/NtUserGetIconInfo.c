/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserGetIconInfo
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtUserGetIconInfo)
{
	HICON hIcon;
	ICONINFO iinfo;
	HBITMAP mask, color;
	UNICODE_STRING hInstStr;
	UNICODE_STRING ResourceStr;
	DWORD bpp = 0;

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

	/* Test full param, with local icon */
	hIcon = LoadImageA(GetModuleHandle(NULL),
					   MAKEINTRESOURCE(IDI_ICON),
					   IMAGE_ICON,
					   0,
					   0,
					   LR_DEFAULTSIZE);

	TEST(hIcon != NULL);

	RtlInitUnicodeString(&hInstStr, NULL);
	RtlInitUnicodeString(&ResourceStr, NULL);

	TEST(NtUserGetIconInfo(hIcon,
						   &iinfo,
						   &hInstStr,
						   &ResourceStr,
						   &bpp,
						   FALSE) == TRUE);

	TESTX(hInstStr.Buffer == NULL, "hInstStr.buffer : %p\n", hInstStr.Buffer);
	TEST((LPCTSTR)ResourceStr.Buffer == MAKEINTRESOURCE(IDI_ICON));
	TEST(bpp == 32);

	/* Last param doesn't seem to matter*/
	TEST(NtUserGetIconInfo(hIcon,
						   &iinfo,
						   &hInstStr,
						   &ResourceStr,
						   &bpp,
						   TRUE) == TRUE);

	TESTX(hInstStr.Buffer == NULL, "hInstStr.buffer : %p\n", hInstStr.Buffer);
	TEST((LPCTSTR)ResourceStr.Buffer == MAKEINTRESOURCE(IDI_ICON));
	TEST(bpp == 32);

	DestroyIcon(hIcon);

	/* Test full param, with foreign icon */
	hIcon = LoadImageA(GetModuleHandleA("shell32.dll"),
					   MAKEINTRESOURCE(293),
					   IMAGE_ICON,
					   0,
					   0,
					   LR_DEFAULTSIZE);

	TEST(hIcon != NULL);

    hInstStr.Buffer = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    hInstStr.MaximumLength = MAX_PATH;
    hInstStr.Length = 0;
	RtlInitUnicodeString(&ResourceStr, NULL);

	TEST(NtUserGetIconInfo(hIcon,
						   &iinfo,
						   &hInstStr,
						   &ResourceStr,
						   &bpp,
						   FALSE) == TRUE);

	TEST(hInstStr.Length != 0);
    hInstStr.Buffer[hInstStr.Length] = 0;
	printf("%s,%i: hInstStr.buffer : %S\n", __FUNCTION__, __LINE__, hInstStr.Buffer);
	TEST((LPCTSTR)ResourceStr.Buffer == MAKEINTRESOURCE(293));
	TEST(ResourceStr.Length == 0);
	TEST(ResourceStr.MaximumLength == 0);
	TEST(bpp == 32);

	ZeroMemory(hInstStr.Buffer, MAX_PATH*sizeof(WCHAR));
    hInstStr.Length = 0;
	RtlInitUnicodeString(&ResourceStr, NULL);

	TEST(NtUserGetIconInfo(hIcon,
						   &iinfo,
						   &hInstStr,
						   &ResourceStr,
						   &bpp,
						   TRUE) == TRUE);

    TEST(hInstStr.Length != 0);
    hInstStr.Buffer[hInstStr.Length] = 0;
	printf("%s,%i: hInstStr.buffer : %S\n", __FUNCTION__, __LINE__, hInstStr.Buffer);
	TEST((LPCTSTR)ResourceStr.Buffer == MAKEINTRESOURCE(293));
	TEST(bpp == 32);

	DestroyIcon(hIcon);

}
