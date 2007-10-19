typedef struct
{
	INT* passed;
	INT* failed;
	LPDIRECTDRAW7 DirectDraw;
} ENUMCONTEXT;

BOOL Test_GetMonitorFrequency (INT* passed, INT* failed);

HRESULT CALLBACK DummyEnumDisplayModes( LPDDSURFACEDESC2 pDDSD, ENUMCONTEXT* Context )
{
	return DDENUMRET_OK;
}

HRESULT CALLBACK EnumDisplayModes( LPDDSURFACEDESC2 pDDSD, ENUMCONTEXT* Context )
{
	static int setcout = 0;
	if(setcout >= 5)
		return DDENUMRET_OK;

	DWORD lpdwFrequency = 0;
	INT* passed = Context->passed;
	INT* failed = Context->failed;
	DDSURFACEDESC2 DisplayMode = {0};
	DisplayMode.dwSize = sizeof(DDSURFACEDESC2);

	TEST ( pDDSD->dwFlags == DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE);
	TEST ( pDDSD->ddpfPixelFormat.dwFlags == DDPF_RGB | DDPF_PALETTEINDEXED8 || pDDSD->ddpfPixelFormat.dwFlags == DDPF_RGB );
	TEST ( Context->DirectDraw->SetDisplayMode (pDDSD->dwWidth, pDDSD->dwHeight, pDDSD->ddpfPixelFormat.dwRGBBitCount, pDDSD->dwRefreshRate, 0) == DD_OK);
	TEST ( Context->DirectDraw->GetMonitorFrequency (&lpdwFrequency) == DD_OK && lpdwFrequency == pDDSD->dwRefreshRate);
	TEST ( Context->DirectDraw->GetDisplayMode (&DisplayMode) == DD_OK
		&& pDDSD->dwHeight == DisplayMode.dwHeight
		&& pDDSD->dwWidth == DisplayMode.dwWidth
		&& pDDSD->dwRefreshRate == DisplayMode.dwRefreshRate
		&& pDDSD->ddpfPixelFormat.dwRGBBitCount == DisplayMode.ddpfPixelFormat.dwRGBBitCount
		&& DisplayMode.dwFlags == DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE );

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

    Test_GetMonitorFrequency(passed, failed);

	/* First try with some generic display modes */
	TEST ( DirectDraw->SetDisplayMode (1586, 895, 0, 0, 0) == DDERR_UNSUPPORTED );
	TEST ( DirectDraw->SetDisplayMode (0, 0, 0, 0, 0x123) == DDERR_INVALIDPARAMS );

	// does this change the display mode to DDSCL_EXCLUSIVE ?
	TEST ( DirectDraw->SetDisplayMode (0, 0, 0, 0, 0) == DD_OK );
	TEST ( DirectDraw->SetDisplayMode (800, 600, 0, 0, 0) == DD_OK );
	TEST ( DirectDraw->SetDisplayMode (0, 0, 16, 0, 0) == DD_OK );

	TEST ( DirectDraw->GetDisplayMode (NULL) == DDERR_INVALIDPARAMS );
	DDSURFACEDESC2 DisplayMode = {0};
	TEST ( DirectDraw->GetDisplayMode (&DisplayMode) == DDERR_INVALIDPARAMS );

	//* Now try getting vaild modes from drive */
	TEST (DirectDraw->EnumDisplayModes(DDEDM_STANDARDVGAMODES, NULL, (PVOID)&Context, NULL) == DDERR_INVALIDPARAMS);
	TEST (DirectDraw->EnumDisplayModes(0, NULL, (PVOID)&Context, (LPDDENUMMODESCALLBACK2)DummyEnumDisplayModes) == DD_OK );
	TEST (DirectDraw->EnumDisplayModes(DDEDM_REFRESHRATES, NULL, (PVOID)&Context, (LPDDENUMMODESCALLBACK2)DummyEnumDisplayModes) == DD_OK );
	TEST (DirectDraw->EnumDisplayModes(DDEDM_STANDARDVGAMODES, NULL, (PVOID)&Context, (LPDDENUMMODESCALLBACK2)DummyEnumDisplayModes) == DD_OK );
	TEST (DirectDraw->EnumDisplayModes(DDEDM_STANDARDVGAMODES|DDEDM_REFRESHRATES, NULL, (PVOID)&Context, (LPDDENUMMODESCALLBACK2)EnumDisplayModes) == DD_OK);

	TEST (DirectDraw->RestoreDisplayMode() == DD_OK);

	DirectDraw->Release();

	return TRUE;
}

BOOL Test_GetMonitorFrequency (INT* passed, INT* failed)
{
	LPDIRECTDRAW7 DirectDraw;
	LPDDRAWI_DIRECTDRAW_INT This;

	/* Preparations */
	if (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, NULL) != DD_OK)
	{
		printf("ERROR: Failed to set up ddraw\n");
		return FALSE;
	}
	This = (LPDDRAWI_DIRECTDRAW_INT)DirectDraw;

	/* Here we go */
	DWORD lpFreq;
	DWORD temp;
	HRESULT retVal;
	TEST (DirectDraw->GetMonitorFrequency((PDWORD)0xdeadbeef) == DDERR_INVALIDPARAMS);
	TEST (DirectDraw->GetMonitorFrequency(NULL) == DDERR_INVALIDPARAMS);

	// result depends on our graphices card
	retVal = DirectDraw->GetMonitorFrequency((PDWORD)&lpFreq);
	TEST ( retVal == DD_OK || retVal == DDERR_UNSUPPORTED);

	/* Test by hacking interal structures */

	// should return DDERR_UNSUPPORTED
	This->lpLcl->lpGbl->dwMonitorFrequency = 0;
	TEST (DirectDraw->GetMonitorFrequency(&temp) == DDERR_UNSUPPORTED);

	// should return DD_OK
	This->lpLcl->lpGbl->dwMonitorFrequency = 85;
	TEST (DirectDraw->GetMonitorFrequency(&temp) == DD_OK);

	/* Restore */
	This->lpLcl->lpGbl->dwMonitorFrequency =  lpFreq;

	DirectDraw->Release();

	return TRUE;
}