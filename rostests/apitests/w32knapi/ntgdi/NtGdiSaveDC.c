
INT
Test_NtGdiSaveDC(PTESTINFO pti)
{
    HDC hdc, hdc2;
    HWND hwnd;
    HBITMAP hbmp1, hbmp2, hOldBmp;

    /* Test 0 hdc */
    TEST(NtGdiSaveDC(0) == 0);

    /* Test info dc */
    hdc = CreateICW(L"DISPLAY",NULL,NULL,NULL);
    TEST(hdc);
    TEST(NtGdiSaveDC(hdc) == 1);
    TEST(NtGdiSaveDC(hdc) == 2);
    DeleteDC(hdc);

    /* Test display dc */
    hdc = GetDC(0);
    TEST(hdc);
    TEST(NtGdiSaveDC(hdc) == 1);
    TEST(NtGdiSaveDC(hdc) == 2);
    ReleaseDC(0, hdc);

    /* Test a mem DC */
    hdc = CreateCompatibleDC(0);
    TEST(hdc);
    TEST(NtGdiSaveDC(hdc) == 1);
    TEST(NtGdiSaveDC(hdc) == 2);
    DeleteDC(hdc);

	/* Create a window */
	hwnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                    10, 10, 100, 100,
	                    NULL, NULL, g_hInstance, 0);
    hdc = GetDC(hwnd);
    TEST(hdc);
    TEST(NtGdiSaveDC(hdc) == 1);
    NtGdiRestoreDC(hdc, 1);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    
    /* test behaviour when a bitmap is selected */
    hbmp1 = CreateBitmap(2, 2, 1, 1, NULL);
    TEST(hbmp1);
    hbmp2 = CreateBitmap(2, 2, 1, 1, NULL);
    TEST(hbmp2);
    hdc = CreateCompatibleDC(0);
    TEST(hdc);
    hdc2 = CreateCompatibleDC(0);
    TEST(hdc2);
    hOldBmp = SelectObject(hdc, hbmp1);
    TEST(hOldBmp);
    TEST(NtGdiSaveDC(hdc) == 1);
    TEST(SelectObject(hdc, hbmp2) == hbmp1);
    TEST(SelectObject(hdc2, hbmp1) == NULL);
    SelectObject(hdc, hOldBmp);
    NtGdiRestoreDC(hdc, 1);
    TEST(GetCurrentObject(hdc, OBJ_BITMAP) == hbmp1);
    /* Again, just to be sure */
    TEST(NtGdiSaveDC(hdc) == 1);
    TEST(NtGdiSaveDC(hdc) == 2);
    TEST(SelectObject(hdc, hbmp2) == hbmp1);
    TEST(SelectObject(hdc2, hbmp1) == NULL);
    SelectObject(hdc, hOldBmp);
    NtGdiRestoreDC(hdc, 2);
    TEST(GetCurrentObject(hdc, OBJ_BITMAP) == hbmp1);
    /*Cleanup */
    SelectObject(hdc, hOldBmp);
    DeleteDC(hdc);
    DeleteDC(hdc2);
    DeleteObject(hbmp1);
    DeleteObject(hbmp2);

    return APISTATUS_NORMAL;
}

