BOOL CreateSurface(LPDIRECTDRAWSURFACE7* pSurface);

BOOL Test_Misc (INT* passed, INT* failed)
{
	LPDIRECTDRAWSURFACE7 Surface;
    if(!CreateSurface(&Surface))
        return FALSE;

    TEST (Surface->Initialize(NULL, NULL) == DDERR_ALREADYINITIALIZED);

    // GetCaps
    DDSCAPS2 Caps;
    TEST (Surface->GetCaps((DDSCAPS2*)0xdeadbeef) == DDERR_INVALIDPARAMS);
    TEST (Surface->GetCaps(&Caps) == DD_OK && Caps.dwCaps == 0x10004040 
        && Caps.dwCaps2 == Caps.dwCaps3 == Caps.dwCaps4 == 0); // FIXME: Replace 0x10004040

    // GetDC / ReleaseDC
    HDC hdc;
    TEST (Surface->GetDC((HDC*)0xdeadbeef) == DDERR_INVALIDPARAMS);
    TEST (Surface->ReleaseDC((HDC)0xdeadbeef) == DDERR_NODC);
    TEST (Surface->ReleaseDC(GetDC(NULL)) == DDERR_NODC);

    TEST (Surface->GetDC(&hdc) == DD_OK);
    TEST (MoveToEx(hdc, 0, 0, NULL) == TRUE); // validate hdc
    TEST (Surface->Blt(NULL, NULL, NULL, 0, NULL) == DDERR_SURFACEBUSY); // check lock
    TEST (Surface->ReleaseDC(hdc) == DD_OK);

    // ChangeUniquenessValue / GetUniquenessValue
    DWORD Value;
    // FIXME: find out other apis which increases the uniqueness value
    TEST (Surface->GetUniquenessValue(&Value) == DD_OK && Value == 2);
    TEST (Surface->Blt(NULL, NULL, NULL, 0, NULL) == DDERR_INVALIDPARAMS); // Even this increases the uniqueness value
    TEST (Surface->GetUniquenessValue(&Value) == DD_OK && Value == 3);
    TEST (Surface->ChangeUniquenessValue() == DD_OK);
    TEST (Surface->GetUniquenessValue(&Value) == DD_OK && Value == 4);

    // GetPixelFormat
    DDPIXELFORMAT PixelFormat = {0};
    // FIXME: Produce DDERR_INVALIDSURFACETYPE
    TEST (Surface->GetPixelFormat((LPDDPIXELFORMAT)0xdeadbeef) == DDERR_INVALIDPARAMS);
    TEST (Surface->GetPixelFormat(&PixelFormat));
    PixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    TEST (Surface->GetPixelFormat(&PixelFormat) == DD_OK && PixelFormat.dwFlags == 0x40); // FIXME: Find out what 0x40 is
    
    // GetSurfaceDesc / SetSurfaceDesc
    DDSURFACEDESC2 Desc = {0};
    // FIXME: Produce DDERR_INVALIDSURFACETYPE
    TEST (Surface->GetSurfaceDesc((LPDDSURFACEDESC2)0xdeadbeef) == DDERR_INVALIDPARAMS);
    TEST (Surface->GetSurfaceDesc(&Desc));
    Desc.dwSize = sizeof(DDSURFACEDESC2);
    TEST (Surface->GetSurfaceDesc(&Desc) == DD_OK && Desc.dwFlags == 0x100f); // FIXME: Find out what 0x100f is
    TEST (memcmp ((PVOID)&Desc.ddpfPixelFormat, (PVOID)&PixelFormat, sizeof(DDPIXELFORMAT)) == 0);
    // FIXME: Test SetSurfaceDesc

    // GetDDInterface
    IUnknown* iface;
    TEST(Surface->GetDDInterface((LPVOID*)0xdeadbeef) == DDERR_INVALIDPARAMS);
    TEST(Surface->GetDDInterface((LPVOID*)&iface) == DD_OK && iface);
    TEST(iface->Release() == 1); // FIXME: Test the interface further

    Surface->Release();

    return TRUE;
}
