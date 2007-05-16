typedef struct
{
	INT* passed;
	INT* failed;
	LPDIRECTDRAW7 DirectDraw;
} ENUMCONTEXT;

HRESULT CALLBACK DummyEnumDisplayModes( LPDDSURFACEDESC2 pDDSD, ENUMCONTEXT* Context )
{
	return DDENUMRET_OK;
}

HRESULT CALLBACK EnumDisplayModes( LPDDSURFACEDESC2 pDDSD, ENUMCONTEXT* Context )
{
	INT* passed = Context->passed;
	INT* failed = Context->failed;
	static int setcout = 0;
	if(setcout < 5)
	{
		TEST ( Context->DirectDraw->SetDisplayMode (pDDSD->dwWidth, pDDSD->dwHeight, pDDSD->ddpfPixelFormat.dwRGBBitCount, pDDSD->dwRefreshRate, 0) == DD_OK);
	}
	setcout++;
	return DDENUMRET_OK;
}

BOOL Test_DisplayModes (INT* passed, INT* failed)
{
	/*** FIXME: Also test with surface as parameter; try busy/locked surface as well ***/
	LPDIRECTDRAW7 DirectDraw;

	/* Preparations */
	if (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, NULL) != DD_OK)
	{
		printf("ERROR: Failed to set up ddraw\n");
		return FALSE;
	}

	ENUMCONTEXT Context = {passed, failed, DirectDraw};

	/* The Test */

	// First try with some generic display modes
	TEST ( DirectDraw->SetDisplayMode (1586, 895, 0, 0, 0) == DDERR_UNSUPPORTED );
	TEST ( DirectDraw->SetDisplayMode (0, 0, 0, 0, 0x123) == DDERR_INVALIDPARAMS );

	TEST ( DirectDraw->SetDisplayMode (0, 0, 0, 0, 0) == DD_OK );
	TEST ( DirectDraw->SetDisplayMode (800, 600, 0, 0, 0) == DD_OK );
	TEST ( DirectDraw->SetDisplayMode (0, 0, 16, 0, 0) == DD_OK );

	// does this change the display mode to DDSCL_EXCLUSIVE ?

	// Now try getting vaild modes from driver
	TEST (DirectDraw->EnumDisplayModes(DDEDM_STANDARDVGAMODES, NULL, (PVOID)&Context, NULL) == DDERR_INVALIDPARAMS);
	TEST (DirectDraw->EnumDisplayModes(0, NULL, (PVOID)&Context, (LPDDENUMMODESCALLBACK2)DummyEnumDisplayModes) == DD_OK );
	TEST (DirectDraw->EnumDisplayModes(DDEDM_REFRESHRATES, NULL, (PVOID)&Context, (LPDDENUMMODESCALLBACK2)DummyEnumDisplayModes) == DD_OK );
	TEST (DirectDraw->EnumDisplayModes(DDEDM_STANDARDVGAMODES, NULL, (PVOID)&Context, (LPDDENUMMODESCALLBACK2)DummyEnumDisplayModes) == DD_OK );
	TEST (DirectDraw->EnumDisplayModes(DDEDM_STANDARDVGAMODES|DDEDM_REFRESHRATES, NULL, (PVOID)&Context, (LPDDENUMMODESCALLBACK2)EnumDisplayModes) == DD_OK);

	DirectDraw->Release();

	return TRUE;
}

BOOL Test_GetMonitorFrequency (INT* passed, INT* failed)
{
	HWND hwnd;
	DWORD lpdwFrequency; 
	LPDIRECTDRAW7 DirectDraw;

	/* Preparations */
	if (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, NULL) != DD_OK)
	{
		printf("ERROR: Failed to set up ddraw\n");
		return FALSE;
	}

	if(!( hwnd = CreateBasicWindow() ))
	{
		printf("ERROR: Failed to create window\n");
		DirectDraw->Release();
		return FALSE;
	}

	/* The Test */
	TEST ( DirectDraw->GetMonitorFrequency (NULL) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->GetMonitorFrequency (&lpdwFrequency) == DD_OK );
	TEST ( lpdwFrequency != 0 );

	return TRUE;
}
