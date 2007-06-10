
DWORD GetPixel (LPDIRECTDRAWSURFACE7 Surface, UINT x, UINT y)
{
    DWORD ret;
    RECT rect = {x, y, x+1, y+1};
    DDSURFACEDESC2 desc = {0};
    desc.dwSize = sizeof(DDSURFACEDESC2);

    if(Surface->Lock(&rect, &desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL))
    {
        printf("ERROR: Unable to lock surface\n");
        return 0xdeadbeef;
    }

    ret = *((DWORD *)desc.lpSurface);

    if(Surface->Unlock (&rect) != DD_OK)
    {
        printf("ERROR: Unable to unlock surface ?!\n");
    }

    return ret;
}

VOID Blt_Test (LPDIRECTDRAWSURFACE7 Surface, INT* passed, INT* failed)
{
	LPDIRECTDRAWSURFACE7 Source;
    if(!CreateSurface(&Source))
        return;

    // The following has been tested with Nvidea hardware
    // the results might differently with other graphic
    // card drivers. - mbosma

    // FIXME: Test Color Key (DDBLT_KEYDEST / DDBLT_KEYSRC / DDBLT_KEYDESTOVERRIDE / DDBLT_KEYSRCOVERRIDE)

    // General Tests
	DDBLTFX bltfx;
    TEST (Surface->Blt(NULL, NULL, NULL, 0, NULL) == DDERR_INVALIDPARAMS);
    TEST (Surface->Blt(NULL, Surface, NULL, 0, NULL) == DD_OK ); // blting to itself

    TEST (Surface->Blt(NULL, NULL, NULL, DDBLT_WAIT|DDBLT_DDFX, &bltfx) == DDERR_INVALIDPARAMS);
	bltfx.dwDDFX = DDBLTFX_NOTEARING;
    TEST (Surface->Blt(NULL, NULL, NULL, DDBLT_WAIT|DDBLT_DDFX, &bltfx) == DDERR_INVALIDPARAMS);
	bltfx.dwSize = sizeof(DDBLTFX);
    TEST (Surface->Blt(NULL, NULL, NULL, DDBLT_WAIT, &bltfx) == DDERR_INVALIDPARAMS);
    TEST (Surface->Blt(NULL, Source, NULL, DDBLT_WAIT|DDBLT_DDFX, &bltfx) == DD_OK); // don't know why this works on a offscreen surfaces

    // Test color filling
	bltfx.dwFillColor = RGB(0, 255, 0);
    TEST (Source->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &bltfx) == DD_OK);
    RECT rect = {100, 100, 200, 200};
	bltfx.dwFillColor = RGB(255, 255, 0);
    TEST (Source->Blt(&rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &bltfx) == DD_OK);
    TEST (GetPixel(Source, 0, 0) == RGB(0, 255, 0));
    TEST (GetPixel(Source, 100, 100) == RGB(255, 255, 0));    

    // Test DestRect and SrcRect
    RECT SourceRect = {100, 100, 200, 200}; 
    RECT DestRect = {0, 0, 200, 100}; 

    TEST (Surface->Blt(&SourceRect, Source, &DestRect, 0, NULL) == DD_OK);
    TEST (GetPixel(Surface, 100, 100) == RGB(0, 255, 0)); // Src bigger: normal blt

    TEST (Surface->Blt(&DestRect, Source, &SourceRect, 0, NULL) == DD_OK);
    TEST (GetPixel(Surface, 0, 0) == 0x00ffbf); // Dest bigger: wtf ??

    DestRect.right = 100; // both are same size now
    TEST (Surface->Blt(&DestRect, Source, &SourceRect, 0, NULL) == DD_OK);
    TEST (GetPixel(Surface, 0, 0) == RGB(255, 255, 0));
    
    RECT TooBig = {100, 100, 200, 250}; 
    TEST (Surface->Blt(&TooBig, Source, &SourceRect, 0, NULL) == DDERR_INVALIDRECT);
    TEST (Surface->Blt(&DestRect, Source, &TooBig, 0, NULL) == DDERR_INVALIDRECT);

    // Test Rotation
	bltfx.dwDDFX = DDBLTFX_MIRRORLEFTRIGHT|DDBLTFX_MIRRORUPDOWN;
    TEST (Surface->Blt(NULL, Source, NULL, DDBLT_WAIT|DDBLT_DDFX, &bltfx) == DD_OK); 
    TEST (GetPixel(Surface, 0, 0) == RGB(255, 255, 0));

	bltfx.dwDDFX = DDBLTFX_ROTATE180;
    TEST (Surface->Blt(NULL, Source, NULL, DDBLT_DDFX, &bltfx) == DDERR_NOROTATIONHW);

	//bltfx.dwRotationAngle = 
    TEST (Surface->Blt(NULL, Source, NULL, DDBLT_ROTATIONANGLE, &bltfx) == DDERR_NOROTATIONHW);

    // Test Raster Operations
	bltfx.dwROP = BLACKNESS;
    TEST (Surface->Blt(NULL, Source, NULL, DDBLT_WAIT|DDBLT_ROP, &bltfx) == DD_OK);
    TEST(GetPixel(Surface, 0, 0) == RGB(0, 0, 0));
	bltfx.dwROP = WHITENESS;
    TEST (Surface->Blt(NULL, Source, NULL, DDBLT_WAIT|DDBLT_ROP, &bltfx) == DD_OK);
    TEST(GetPixel(Surface, 0, 0) == RGB(255, 255, 255));
	bltfx.dwROP = SRCCOPY; // this flag actually does nothing
    TEST (Surface->Blt(NULL, Source, NULL, DDBLT_WAIT|DDBLT_ROP, &bltfx) == DD_OK);
    TEST(GetPixel(Surface, 0, 0) == RGB(0, 255, 0));
	bltfx.dwROP = SRCAND;
    TEST (Surface->Blt(NULL, Source, NULL, DDBLT_WAIT|DDBLT_ROP, &bltfx) == DDERR_NORASTEROPHW); 

    // Test Direct Draw Raster Operations
	bltfx.dwDDROP = 0x123;
    TEST (Surface->Blt(NULL, Source, NULL, DDBLT_WAIT|DDBLT_DDROPS, &bltfx) == DDERR_NODDROPSHW);

    // Streching
	bltfx.dwDDFX = DDBLTFX_ARITHSTRETCHY;
    TEST (Surface->Blt(NULL, Source, NULL, DDBLT_WAIT|DDBLT_DDFX, &bltfx) == DDERR_NOSTRETCHHW);
}

VOID GetBltStatus_Test (LPDIRECTDRAWSURFACE7 Surface, INT* passed, INT* failed)
{
    TEST (Surface->GetBltStatus(0) == DDERR_INVALIDPARAMS);
    TEST (Surface->GetBltStatus(DDGBS_CANBLT) == DD_OK);
    //TEST (Surface->GetBltStatus(DDGBS_ISBLTDONE) == DD_OK);

    // Lock Surface
    DDSURFACEDESC2 desc = {0};
    desc.dwSize = sizeof(DDSURFACEDESC2);
    Surface->Lock(NULL, &desc, DDLOCK_WAIT, NULL);
    TEST (Surface->GetBltStatus(DDGBS_ISBLTDONE) == DD_OK);
    TEST (Surface->GetBltStatus(DDGBS_CANBLT) == DD_OK); // does not return DDERR_SURFACEBUSY for me as msdn says (xp,nvidea)
    Surface->Unlock (NULL);

    // Try to produce busy surface by filling it 500 times
	DDBLTFX	 bltfx;
	bltfx.dwSize = sizeof(DDBLTFX);
	bltfx.dwFillColor = RGB(0, 0, 0);

    int i;
    for(i=0; i<500; i++)
        Surface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &bltfx);

    TEST (Surface->GetBltStatus(DDGBS_ISBLTDONE) == DDERR_WASSTILLDRAWING);
    TEST (Surface->GetBltStatus(DDGBS_CANBLT) == DD_OK);
}

BOOL Test_Blt (INT* passed, INT* failed)
{
	LPDIRECTDRAWSURFACE7 Surface;
    if(!CreateSurface(&Surface))
        return FALSE;

    // Test GetPixel (needs Lock API)
    DDBLTFX	 bltfx;
	bltfx.dwSize = sizeof(DDBLTFX);
	bltfx.dwFillColor = RGB(0, 0, 0);
    Surface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &bltfx);
    if(GetPixel(Surface, 0, 0) != RGB(0, 0, 0))
        return FALSE;

    // The tests
    TEST(Surface->BltBatch(NULL, 0, 0) == DDERR_UNSUPPORTED);
    Blt_Test (Surface, passed, failed);
    GetBltStatus_Test (Surface, passed, failed);

    Surface->Release();
    return TRUE;
}