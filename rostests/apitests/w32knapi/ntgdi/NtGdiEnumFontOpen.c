
INT
Test_NtGdiEnumFontOpen(PTESTINFO pti)
{
	HDC hDC;
	ULONG_PTR idEnum;
	ULONG ulCount;
	PGDI_TABLE_ENTRY pEntry;

	hDC = CreateDCW(L"DISPLAY",NULL,NULL,NULL);

	// FIXME: We should load the font first

	idEnum = NtGdiEnumFontOpen(hDC, 2, 0, 32, L"Courier", ANSI_CHARSET, &ulCount);
	ASSERT(idEnum != 0);

	/* we should have a gdi handle here */
	TEST(GDI_HANDLE_GET_TYPE(idEnum) == GDI_OBJECT_TYPE_ENUMFONT);
	pEntry = &GdiHandleTable[GDI_HANDLE_GET_INDEX(idEnum)];
	TEST(pEntry->KernelData != NULL);
	TEST(pEntry->ProcessId == GetCurrentProcessId());
	TEST(pEntry->UserData == 0);
	TEST(pEntry->Type == ((idEnum >> 16) | GDI_OBJECT_TYPE_ENUMFONT));

	/* We should not be able to use DeleteObject() on the handle */
	TEST(DeleteObject((HGDIOBJ)idEnum) == FALSE);

	NtGdiEnumFontClose(idEnum);

	// Test no logfont (NULL): should word
	// Test empty lfFaceName string: should not work


	return APISTATUS_NORMAL;
}

