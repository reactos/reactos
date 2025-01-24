BOOL CreateSurface(LPDIRECTDRAWSURFACE7* pSurface);

BOOL Test_PrivateData (INT* passed, INT* failed)
{
	LPDIRECTDRAWSURFACE7 Surface;
    DWORD size, dummy = 0xBAADF00D;
    GUID guid = { 0 };
    GUID guid2 = { 0x1 };

    if(!CreateSurface(&Surface))
        return FALSE;

    // General test
    TEST(Surface->SetPrivateData(guid, NULL, 0, 0) == DDERR_INVALIDPARAMS);
    TEST(Surface->SetPrivateData(guid, (LPVOID)&dummy, 0, 0) == DDERR_INVALIDPARAMS);
    TEST(Surface->SetPrivateData(guid, (LPVOID)0xdeadbeef, sizeof(DWORD), 0) == DDERR_INVALIDPARAMS);
    TEST(Surface->SetPrivateData(guid, (LPVOID)&dummy, sizeof(DWORD), 0) == DD_OK);

    TEST(Surface->GetPrivateData(guid, NULL, 0) == DDERR_INVALIDPARAMS);
    TEST(Surface->GetPrivateData(guid, &dummy, 0) == DDERR_INVALIDPARAMS);
    size = 0;
    TEST(Surface->GetPrivateData(guid, &dummy, &size) == DDERR_MOREDATA && size == sizeof(DWORD));
    size = 2;
    TEST(Surface->GetPrivateData(guid, NULL, &size) == DDERR_MOREDATA && size == sizeof(DWORD));
    TEST(Surface->GetPrivateData(guid, NULL, &size) == DDERR_INVALIDPARAMS);
    TEST(Surface->GetPrivateData(guid, &dummy, &size) == DD_OK && dummy == 0xBAADF00D);
    TEST(Surface->GetPrivateData(guid2, NULL, 0) == DDERR_NOTFOUND);

    TEST(Surface->FreePrivateData(guid) == DD_OK);
    TEST(Surface->FreePrivateData(guid) == DDERR_NOTFOUND);

    // Test for DDSPD_VOLATILE flag
    TEST(Surface->SetPrivateData(guid, (LPVOID)&dummy, sizeof(DWORD), DDSPD_VOLATILE) == DD_OK);
    size = 0;
    TEST(Surface->GetPrivateData(guid, NULL, &size) == DDERR_MOREDATA && size == sizeof(DWORD));
    TEST(Surface->GetPrivateData(guid, &dummy, &size) == DD_OK && dummy == 0xBAADF00D);

	DDBLTFX	 bltfx;
	bltfx.dwSize = sizeof(DDBLTFX);
	bltfx.dwFillColor = RGB(0, 0, 0);
	if(Surface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &bltfx) != DD_OK)
        printf("ERROR: Failed to draw to surface !");
    TEST(Surface->GetPrivateData(guid, &dummy, &size) == DDERR_EXPIRED);

    // TODO: Test for DDSPD_IUNKNOWNPOINTER (see http://msdn.microsoft.com/archive/default.asp?url=/archive/en-us/ddraw7/directdraw7/ddref_5qyf.asp (DEAD_LINK))

    Surface->Release();
    return TRUE;
}
