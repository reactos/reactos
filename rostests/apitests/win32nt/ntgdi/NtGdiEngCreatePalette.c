/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiEngCreatePalette
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiEngCreatePalette)
{
	HPALETTE hPal;
	ULONG Colors[3] = {1,2,3};
	PENTRY pEntry;

	hPal = NtGdiEngCreatePalette(PAL_RGB, 3, Colors, 0xff000000, 0x00ff0000, 0x0000ff00);

	TEST(hPal != 0);
	TEST(GDI_HANDLE_GET_TYPE(hPal) == GDI_OBJECT_TYPE_PALETTE);
	pEntry = &GdiHandleTable[GDI_HANDLE_GET_INDEX(hPal)];
	TEST(pEntry->einfo.pobj != NULL);
	TEST(pEntry->ObjectOwner.ulObj == GetCurrentProcessId());
	TEST(pEntry->pUser == 0);
	//TEST(pEntry->Type == (((UINT)hPal >> 16) | GDI_OBJECT_TYPE_PALETTE));

}
