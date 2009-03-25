
INT
Test_NtGdiSaveDC(PTESTINFO pti)
{
    HDC hdc;
    HWND hwnd;

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

    return APISTATUS_NORMAL;
}

