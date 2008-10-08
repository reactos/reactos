


INT
Test_SetMapMode(PTESTINFO pti)
{
    HDC hDC;
    SIZE WindowExt, ViewportExt;

    hDC = CreateCompatibleDC(NULL);
    ASSERT(hDC);

    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);

    ASSERT(WindowExt.cx == 1);
    ASSERT(WindowExt.cy == 1);
    ASSERT(ViewportExt.cx == 1);
    ASSERT(ViewportExt.cy == 1);

    SetMapMode(hDC, MM_ISOTROPIC);

    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);

    TEST(WindowExt.cx == 3600);
    TEST(WindowExt.cy == 2700);
    TEST(ViewportExt.cx == GetDeviceCaps(GetDC(0), HORZRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));

    DeleteDC(hDC);

    return APISTATUS_NORMAL;
}
