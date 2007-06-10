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
		printf("ERROR: Could not set cooperative level\n");
		DirectDraw->Release();
		return 0;
	}
 
	/* The Test */
 
	DDSURFACEDESC2 Desc = { 0 };
 
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, (IUnknown*)0xdeadbeef) == CLASS_E_NOAGGREGATION );
	TEST ( DirectDraw->CreateSurface(NULL, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->CreateSurface(&Desc, NULL, NULL) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS );
 
	Desc.dwHeight = 200;
	Desc.dwWidth = 200;
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.dwSize = sizeof (DDSURFACEDESC2);
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);
	Desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DD_OK);
	TEST ( DirectDrawSurface && DirectDrawSurface->Release() == 0 );
	DirectDrawSurface = NULL;
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_3DDEVICE;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);
	Desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DD_OK);
	TEST ( DirectDrawSurface && DirectDrawSurface->Release() == 0 );
	DirectDrawSurface = NULL;
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_ALLOCONLOAD;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_ALPHA;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_COMPLEX;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_FLIP;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_FRONTBUFFER;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_HWCODEC;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);
	Desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DD_OK);
	TEST ( DirectDrawSurface && DirectDrawSurface->Release() == 0 );
	DirectDrawSurface = NULL;
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_LIVEVIDEO;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);
	Desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DD_OK);
	TEST ( DirectDrawSurface && DirectDrawSurface->Release() == 0 );
	DirectDrawSurface = NULL;
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_LOCALVIDMEM;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_MIPMAP;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_MODEX;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);
	Desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DD_OK);
	TEST ( DirectDrawSurface && DirectDrawSurface->Release() == DD_OK );
	DirectDrawSurface = NULL;
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_NONLOCALVIDMEM;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);
	Desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DD_OK);
	TEST ( DirectDrawSurface && DirectDrawSurface->Release() == DD_OK );
	DirectDrawSurface = NULL;
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_OPTIMIZED;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_OVERLAY;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);
	Desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
#if 0
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DD_OK);
	TEST ( DirectDrawSurface && DirectDrawSurface->Release() == 0 );
	DirectDrawSurface = NULL;
#endif
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_OWNDC;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_UNSUPPORTED);
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_PALETTE;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DD_OK );
	TEST ( DirectDrawSurface && DirectDrawSurface->Release() == 0 );
	DirectDrawSurface = NULL;
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_STANDARDVGAMODE;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);
	Desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DD_OK);
	TEST ( DirectDrawSurface && DirectDrawSurface->Release() == 0 );
	DirectDrawSurface = NULL;
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);
	Desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DD_OK);
	TEST ( DirectDrawSurface && DirectDrawSurface->Release() == 0 );
	DirectDrawSurface = NULL;
 
	DirectDrawSurface = NULL;
	Desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	Desc.dwFlags = DDSD_CAPS;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);
	Desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DD_OK );
	TEST ( DirectDrawSurface && DirectDrawSurface->Release() == 0 );
	DirectDrawSurface = NULL;
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);
	Desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DD_OK);
	TEST ( DirectDrawSurface && DirectDrawSurface->Release() == 0 );
	DirectDrawSurface = NULL;
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_VIDEOPORT;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);
	Desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DD_OK);
	TEST ( DirectDrawSurface && DirectDrawSurface->Release() == 0 );
	DirectDrawSurface = NULL;
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_VISIBLE;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);
 
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_WRITEONLY;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDCAPS);
 
	Desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	Desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
	TEST ( DirectDraw->CreateSurface(&Desc, &DirectDrawSurface, NULL) == DDERR_INVALIDPARAMS);
 
	DirectDraw->Release();
 
	return TRUE;
}
