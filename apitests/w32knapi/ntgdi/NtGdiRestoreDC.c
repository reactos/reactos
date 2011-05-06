

static HBRUSH hbrush;
static HBITMAP hbitmap;
static HPEN hpen;
static HFONT hfont;
static HRGN hrgn, hrgn2;

static
void
SetSpecialDCState(HDC hdc)
{
    /* Select spcial Objects */
    SelectObject(hdc, hbrush);
    SelectObject(hdc, hpen);
    SelectObject(hdc, hbitmap);
    SelectObject(hdc, hfont);
    SelectObject(hdc, hrgn);

    /* Colors */
    SetDCBrushColor(hdc, RGB(12,34,56));
    SetDCPenColor(hdc, RGB(23,34,45));

    /* Coordinates */
    SetMapMode(hdc, MM_ANISOTROPIC);
    SetGraphicsMode(hdc, GM_ADVANCED);
    SetWindowOrgEx(hdc, 12, 34, NULL);
    SetViewportOrgEx(hdc, 56, 78, NULL);
    SetWindowExtEx(hdc, 123, 456, NULL);
    SetViewportExtEx(hdc, 234, 567, NULL);



}

static
void
SetSpecialDCState2(HDC hdc)
{
    /* Select spcial Objects */
    SelectObject(hdc, GetStockObject(DC_BRUSH));
    SelectObject(hdc, GetStockObject(DC_PEN));
    SelectObject(hdc, GetStockObject(DEFAULT_BITMAP));
    SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
    SelectObject(hdc, hrgn2);

    /* Colors */
    SetDCBrushColor(hdc, RGB(65,43,21));
    SetDCPenColor(hdc, RGB(54,43,32));

    /* Coordinates */
    SetMapMode(hdc, MM_ISOTROPIC);
    SetGraphicsMode(hdc, GM_COMPATIBLE);
    SetWindowOrgEx(hdc, 43, 21, NULL);
    SetViewportOrgEx(hdc, 87, 65, NULL);
    SetWindowExtEx(hdc, 654, 321, NULL);
    SetViewportExtEx(hdc, 765, 432, NULL);


}

static
void
Test_IsSpecialState(PTESTINFO pti, HDC hdc, BOOL bMemDC)
{
    POINT pt;
    SIZE sz;

    /* Test Objects */
    TEST(SelectObject(hdc, GetStockObject(DC_BRUSH)) == hbrush);
    TEST(SelectObject(hdc, GetStockObject(DC_PEN)) == hpen);
    TEST(SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT)) == hfont);
    if (bMemDC)
    {
        TEST(SelectObject(hdc, GetStockObject(DEFAULT_BITMAP)) == hbitmap);
        TEST(SelectObject(hdc, hrgn2) == (PVOID)1);
    }
    else
    {
        TEST(SelectObject(hdc, GetStockObject(DEFAULT_BITMAP)) == 0);
        TEST(SelectObject(hdc, hrgn2) == (PVOID)2);
    }

    /* Test colors */
    TEST(GetDCBrushColor(hdc) == RGB(12,34,56));
    TEST(GetDCPenColor(hdc) == RGB(23,34,45));

    /* Test coordinates */
    TEST(GetMapMode(hdc) == MM_ANISOTROPIC);
    TEST(GetGraphicsMode(hdc) == GM_ADVANCED);
    GetWindowOrgEx(hdc, &pt);
    TEST(pt.x == 12);
    TEST(pt.y == 34);
    GetViewportOrgEx(hdc, &pt);
    TEST(pt.x == 56);
    TEST(pt.y == 78);
    GetWindowExtEx(hdc, &sz);
    TESTX(sz.cx == 123, "sz.cx == %ld\n", sz.cx);
    TESTX(sz.cy == 456, "sz.cy == %ld\n", sz.cy);
    GetViewportExtEx(hdc, &sz);
    TEST(sz.cx == 234);
    TEST(sz.cy == 567);


}


static
void
Test_SaveRestore(PTESTINFO pti, HDC hdc, BOOL bMemDC)
{
    SetSpecialDCState(hdc);
    NtGdiSaveDC(hdc);
    SetSpecialDCState2(hdc);

    SetLastError(0);
    TEST(NtGdiRestoreDC(hdc, 2) == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);

    SetLastError(0);
    TEST(NtGdiRestoreDC(hdc, 0) == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);

    SetLastError(0);
    TEST(NtGdiRestoreDC(hdc, -2) == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);

    SetLastError(0);
    TEST(NtGdiRestoreDC(hdc, 1) == 1);
    TEST(GetLastError() == 0);

    Test_IsSpecialState(pti, hdc, bMemDC);
}


INT
Test_NtGdiRestoreDC(PTESTINFO pti)
{
    HDC hdc;

    hdc = CreateCompatibleDC(0);
    ASSERT(IsHandleValid(hdc));

    SetLastError(0);
    TEST(NtGdiRestoreDC(0, -10) == 0);
    TEST(GetLastError() == ERROR_INVALID_HANDLE);

    SetLastError(0);
    TEST(NtGdiRestoreDC(hdc, 0) == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);

    SetLastError(0);
    TEST(NtGdiRestoreDC(hdc, 1) == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);

    /* Initialize objects */
    hbrush = CreateSolidBrush(12345);
    ASSERT(IsHandleValid(hbrush));
    hpen = CreatePen(PS_SOLID, 4, RGB(10,12,32));
    ASSERT(IsHandleValid(hpen));
    hbitmap = CreateBitmap(10, 10, 1, 1, NULL);
    ASSERT(IsHandleValid(hbitmap));
    hfont = CreateFont(10, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, 
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                DEFAULT_PITCH, "Arial");
    ASSERT(IsHandleValid(hfont));
    hrgn = CreateRectRgn(12, 14, 14, 17);
    ASSERT(IsHandleValid(hrgn));
    hrgn2 = CreateRectRgn(1, 1, 2, 2);
    ASSERT(IsHandleValid(hrgn2));

    /* Test mem dc */
    Test_SaveRestore(pti, hdc, TRUE);
    DeleteDC(hdc);

    /* Test screen DC */
    hdc = GetDC(0);
    ASSERT(IsHandleValid(hdc));
    Test_SaveRestore(pti, hdc, FALSE);
    ReleaseDC(0, hdc);

    /* Test info dc */
    hdc = CreateICW(L"DISPLAY", NULL, NULL, NULL);
    ASSERT(IsHandleValid(hdc));
    Test_SaveRestore(pti, hdc, FALSE);
    DeleteDC(hdc);



    return APISTATUS_NORMAL;
}

