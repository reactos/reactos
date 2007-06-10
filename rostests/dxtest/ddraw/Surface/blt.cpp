
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

VOID GetBltStatus_Test (LPDIRECTDRAWSURFACE7 Surface, INT* passed, INT* failed)
{
    TEST (Surface->GetBltStatus(0) == DDERR_INVALIDPARAMS);
    TEST (Surface->GetBltStatus(DDGBS_CANBLT) == DD_OK);
    TEST (Surface->GetBltStatus(DDGBS_ISBLTDONE) == DD_OK);

    // Lock Surface
    DDSURFACEDESC2 desc = {0};
    desc.dwSize = sizeof(DDSURFACEDESC2);
    Surface->Lock(NULL, &desc, DDLOCK_WAIT, NULL);
    TEST (Surface->GetBltStatus(DDGBS_ISBLTDONE) == DD_OK);
    TEST (Surface->GetBltStatus(DDGBS_CANBLT) == DD_OK); // does not return DDERR_SURFACEBUSY for me as msdn says (xp,nvidea) - mbosma
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
    GetBltStatus_Test (Surface, passed, failed);

    Surface->Release();
    return TRUE;
}