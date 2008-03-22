INT
Test_SetWorldTransform(PTESTINFO pti)
{
	PGDI_TABLE_ENTRY pEntry;
	HDC hScreenDC, hDC;
	DC_ATTR* pDC_Attr;

	/* Create a DC */
	hScreenDC = GetDC(NULL);
	hDC = CreateCompatibleDC(hScreenDC);
	ReleaseDC(NULL, hScreenDC);
	SetGraphicsMode(hDC, GM_ADVANCED);

	pEntry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hDC);
	pDC_Attr = pEntry->UserData;

	DeleteDC(hDC);

	return APISTATUS_NORMAL;
}
