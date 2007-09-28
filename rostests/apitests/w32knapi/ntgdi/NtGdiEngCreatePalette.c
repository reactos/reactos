
INT
Test_NtGdiEngCreatePalette(PTESTINFO pti)
{
	HPALETTE hPal;
	ULONG Colors[3] = {1,2,3};
	PGDI_TABLE_ENTRY pEntry;

	hPal = NtGdiEngCreatePalette(PAL_RGB, 3, Colors, 0xff000000, 0x00ff0000, 0x0000ff00);

	TEST(hPal != 0);
	TEST(GDI_HANDLE_GET_TYPE(hPal) == GDI_OBJECT_TYPE_PALETTE);
	pEntry = &GdiHandleTable[GDI_HANDLE_GET_INDEX(hPal)];
	TEST(pEntry->KernelData != NULL);
	TEST(pEntry->ProcessId == GetCurrentProcessId());
	TEST(pEntry->UserData == 0);
	TEST(pEntry->Type == (((UINT)hPal >> 16) | GDI_OBJECT_TYPE_PALETTE));

    return APISTATUS_NORMAL;
}
