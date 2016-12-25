/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiDdDeleteDirectDrawObject
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiDdDeleteDirectDrawObject)
{
	HANDLE  hDirectDraw;
	HDC hdc = CreateDCW(L"DISPLAY",NULL,NULL,NULL);
	ok(hdc != NULL, "\n");

	/* Test ReactX */
	ok(NtGdiDdDeleteDirectDrawObject(NULL) == FALSE, "\n");
	ok((hDirectDraw=NtGdiDdCreateDirectDrawObject(hdc)) != NULL, "\n");
	ok(NtGdiDdDeleteDirectDrawObject(hDirectDraw) == TRUE, "\n");

	/* Cleanup ReactX setup */
	DeleteDC(hdc);
	NtGdiDdDeleteDirectDrawObject(hDirectDraw);
}
