
HWND CreateBasicWindow (VOID);

BOOL Test_CreateSurface (INT* passed, INT* failed)
{
	LPDIRECTDRAW7 DirectDraw;
	LPDIRECTDRAWSURFACE7 DirectDrawSurface = NULL;
	HWND hwnd;

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

	if (DirectDraw->SetCooperativeLevel (hwnd, DDSCL_NORMAL) != DD_OK)
	{
		printf("ERROR: you not set cooperative level\n");
		DirectDraw->Release();
		return 0;
	}

	/* The Test */

	DDSURFACEDESC2 Desc = { 0 };

	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, (IUnknown*)0xdeadbeef) == CLASS_E_NOAGGREGATION );
	TEST ( DirectDraw->CreateSurface(NULL, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->CreateSurface(&Desc, NULL, NULL) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS );

	Desc.dwSize = sizeof (DDSURFACEDESC2);
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS );

	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DD_OK );
	TEST ( DirectDrawSurface && DirectDrawSurface->Release() == DD_OK );

	DirectDrawSurface = NULL;
	Desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);

	DirectDrawSurface = NULL;
	Desc.ddsCaps.dwCaps = DDSCAPS_3DDEVICE;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);

	DirectDrawSurface = NULL;
	Desc.ddsCaps.dwCaps = DDSCAPS_ALLOCONLOAD;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);

	DirectDrawSurface = NULL;
	Desc.ddsCaps.dwCaps = DDSCAPS_ALPHA;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);

	DirectDrawSurface = NULL;
	Desc.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);

	DirectDrawSurface = NULL;
	Desc.ddsCaps.dwCaps = DDSCAPS_COMPLEX;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);

	DirectDrawSurface = NULL;
	Desc.ddsCaps.dwCaps = DDSCAPS_FLIP;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);

	DirectDrawSurface = NULL;
	Desc.ddsCaps.dwCaps = DDSCAPS_HWCODEC;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);

	DirectDrawSurface = NULL;
	Desc.ddsCaps.dwCaps = DDSCAPS_LIVEVIDEO;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);

	DirectDrawSurface = NULL;
	Desc.ddsCaps.dwCaps = DDSCAPS_LOCALVIDMEM;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);

	DirectDrawSurface = NULL;
	Desc.ddsCaps.dwCaps = DDSCAPS_MIPMAP;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);

	DirectDrawSurface = NULL;
	Desc.ddsCaps.dwCaps = DDSCAPS_MODEX;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);

	DirectDrawSurface = NULL;
	Desc.ddsCaps.dwCaps = DDSCAPS_MODEX;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);

	DirectDraw->Release();

	return TRUE;
}
