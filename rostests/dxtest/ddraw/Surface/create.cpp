HWND CreateBasicWindow (VOID);

LPDIRECTDRAW7 DirectDraw;

BOOL TestCaps (char* dummy, DWORD Caps, HRESULT test1, HRESULT test2)
{
    LPDIRECTDRAWSURFACE7 Surface = NULL;
	DDSURFACEDESC2 Desc = { 0 };
	Desc.dwHeight = 200;
	Desc.dwWidth = 200;
	Desc.dwSize = sizeof (DDSURFACEDESC2);
    Desc.ddsCaps.dwCaps = Caps;

    Desc.dwFlags = DDSD_CAPS;
    BOOL ret = DirectDraw->CreateSurface(&Desc, &Surface, NULL) == test1;

    Desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ret = ret && DirectDraw->CreateSurface(&Desc, &Surface, NULL) == test2;

    if ( Surface )
        Surface->Release();

    return ret;
}

BOOL Test_CreateSurface (INT* passed, INT* failed)
{
	LPDIRECTDRAWSURFACE7 Surface = NULL;
	HWND hwnd;

	/* Preparations */
	if (DirectDrawCreateEx(NULL, (VOID**)&DirectDraw, IID_IDirectDraw7, NULL) != DD_OK)
	{
		printf("ERROR: Failed to set up ddraw\n");
		return FALSE;
	}

	TEST ( DirectDraw->CreateSurface(NULL, NULL, NULL) == DDERR_NOCOOPERATIVELEVELSET);

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
	Desc.dwSize = sizeof (DDSURFACEDESC2);
	Desc.dwHeight = 200;
	Desc.dwWidth = 200;

	TEST ( DirectDraw->CreateSurface(&Desc, &Surface, (IUnknown*)0xdeadbeef) == CLASS_E_NOAGGREGATION );
	TEST ( DirectDraw->CreateSurface(NULL, &Surface, NULL) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->CreateSurface(&Desc, NULL, NULL) == DDERR_INVALIDPARAMS );
	TEST ( DirectDraw->CreateSurface(&Desc, &Surface, NULL) == DDERR_INVALIDPARAMS );

    // Test (nearly) all possible cap combinations
    #include "caps_tests.h"

    DirectDraw->Release();

	return TRUE;
}
