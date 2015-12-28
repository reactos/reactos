/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiEnumFontOpen
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiEnumFontOpen)
{
	HDC hDC;
	ULONG_PTR idEnum;
	ULONG ulCount;
	PENTRY pEntry;

	hDC = CreateDCW(L"DISPLAY",NULL,NULL,NULL);

	// FIXME: We should load the font first

	idEnum = NtGdiEnumFontOpen(hDC, 2, 0, 32, L"Courier", ANSI_CHARSET, &ulCount);
	ASSERT(idEnum != 0);

	/* we should have a gdi handle here */
	TEST(GDI_HANDLE_GET_TYPE(idEnum) == GDI_OBJECT_TYPE_ENUMFONT);
	pEntry = &GdiHandleTable[GDI_HANDLE_GET_INDEX(idEnum)];
	TEST(pEntry->einfo.pobj != NULL);
	TEST(pEntry->ObjectOwner.ulObj == GetCurrentProcessId());
	TEST(pEntry->pUser == NULL);
	TEST(pEntry->FullUnique == (idEnum >> 16));
	TEST(pEntry->Objt == GDI_OBJECT_TYPE_ENUMFONT >> 16);
	TEST(pEntry->Flags == 0);

	/* We should not be able to use DeleteObject() on the handle */
	TEST(DeleteObject((HGDIOBJ)idEnum) == FALSE);

	NtGdiEnumFontClose(idEnum);

	// Test no logfont (NULL): should word
	// Test empty lfFaceName string: should not work

}

