/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiDdCreateDirectDrawObject
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiDdCreateDirectDrawObject)
{
	HANDLE  hDirectDraw;
	HDC hdc = CreateDCW(L"DISPLAY",NULL,NULL,NULL);
	ok(hdc != NULL, "\n");

	/* Test ReactX */
	ok(NtGdiDdCreateDirectDrawObject(NULL) == NULL, "\n");
	ok((hDirectDraw = NtGdiDdCreateDirectDrawObject(hdc)) != NULL, "\n");

	/* Cleanup ReactX setup */
	DeleteDC(hdc);
	NtGdiDdDeleteDirectDrawObject(hDirectDraw);
}
