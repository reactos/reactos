


INT
Test_SetMapMode(PTESTINFO pti)
{
    HDC hDC;
    SIZE WindowExt, ViewportExt;
    ULONG ulMapMode;

    hDC = CreateCompatibleDC(NULL);
    ASSERT(hDC);

    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);

    ulMapMode = SetMapMode(hDC, MM_ISOTROPIC);
    TEST(ulMapMode == MM_TEXT);
    TEST(WindowExt.cx == 1);
    TEST(WindowExt.cy == 1);
    TEST(ViewportExt.cx == 1);
    TEST(ViewportExt.cy == 1);

    SetLastError(0);
    ulMapMode = SetMapMode(hDC, 0);
    TEST(GetLastError() == 0);
    TEST(ulMapMode == 0);

    /* Go through all valid values */
    ulMapMode = SetMapMode(hDC, 1);
    TEST(ulMapMode == MM_ISOTROPIC);
    ulMapMode = SetMapMode(hDC, 2);
    TEST(ulMapMode == 1);
    ulMapMode = SetMapMode(hDC, 3);
    TEST(ulMapMode == 2);
    ulMapMode = SetMapMode(hDC, 4);
    TEST(ulMapMode == 3);
    ulMapMode = SetMapMode(hDC, 5);
    TEST(ulMapMode == 4);
    ulMapMode = SetMapMode(hDC, 6);
    TEST(ulMapMode == 5);
    ulMapMode = SetMapMode(hDC, 7);
    TEST(ulMapMode == 6);
    ulMapMode = SetMapMode(hDC, 8);
    TEST(ulMapMode == 7);

    /* Test invalid value */
    ulMapMode = SetMapMode(hDC, 9);
    TEST(ulMapMode == 0);
    ulMapMode = SetMapMode(hDC, 10);
    TEST(ulMapMode == 0);

    TEST(GetLastError() == 0);

    /* Test NULL DC */
    ulMapMode = SetMapMode((HDC)0, 2);
    TEST(ulMapMode == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);

    /* Test NULL DC and invalid mode */
    ulMapMode = SetMapMode((HDC)0, 10);
    TEST(ulMapMode == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);

    /* Test invalid DC */
    ulMapMode = SetMapMode((HDC)0x12345, 2);
    TEST(ulMapMode == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);

    /* Test invalid DC and invalid mode */
    ulMapMode = SetMapMode((HDC)0x12345, 10);
    TEST(ulMapMode == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);

    DeleteDC(hDC);

    /* Test a deleted DC */
    ulMapMode = SetMapMode(hDC, 2);
    TEST(ulMapMode == 0);
    TEST(GetLastError() == ERROR_INVALID_PARAMETER);

    /* Test MM_TEXT */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_TEXT);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(WindowExt.cx == 1);
    TEST(WindowExt.cy == 1);
    TEST(ViewportExt.cx == 1);
    TEST(ViewportExt.cy == 1);
    DeleteDC(hDC);

    /* Test MM_ISOTROPIC */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_ISOTROPIC);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(WindowExt.cx == 3600);
    TEST(WindowExt.cy == 2700);
    TEST(ViewportExt.cx == GetDeviceCaps(GetDC(0), HORZRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));
    DeleteDC(hDC);

    /* Test MM_ANISOTROPIC */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_ANISOTROPIC);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(WindowExt.cx == 1);
    TEST(WindowExt.cy == 1);
    TEST(ViewportExt.cx == 1);
    TEST(ViewportExt.cy == 1);

    /* set MM_ISOTROPIC first, the values will be kept */
    SetMapMode(hDC, MM_ISOTROPIC);
    SetMapMode(hDC, MM_ANISOTROPIC);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(WindowExt.cx == 3600);
    TEST(WindowExt.cy == 2700);
    TEST(ViewportExt.cx == GetDeviceCaps(GetDC(0), HORZRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));
    DeleteDC(hDC);

    /* Test MM_LOMETRIC */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_LOMETRIC);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(WindowExt.cx == 3600);
    TEST(WindowExt.cy == 2700);
    TEST(ViewportExt.cx == GetDeviceCaps(GetDC(0), HORZRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));
    DeleteDC(hDC);

    /* Test MM_HIMETRIC */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_HIMETRIC);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(WindowExt.cx == 36000);
    TEST(WindowExt.cy == 27000);
    TEST(ViewportExt.cx == GetDeviceCaps(GetDC(0), HORZRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));
    DeleteDC(hDC);

    /* Test MM_LOENGLISH */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_LOENGLISH);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(WindowExt.cx == 1417);
    TEST(WindowExt.cy == 1063);
    TEST(ViewportExt.cx == GetDeviceCaps(GetDC(0), HORZRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));
    DeleteDC(hDC);

    /* Test MM_HIENGLISH */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_HIENGLISH);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(WindowExt.cx == 14173);
    TEST(WindowExt.cy == 10630);
    TEST(ViewportExt.cx == GetDeviceCaps(GetDC(0), HORZRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));
    DeleteDC(hDC);

    /* Test MM_TWIPS */
    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, MM_TWIPS);
    GetWindowExtEx(hDC, &WindowExt);
    GetViewportExtEx(hDC, &ViewportExt);
    TEST(WindowExt.cx == 20409);
    TEST(WindowExt.cy == 15307);
    TEST(ViewportExt.cx == GetDeviceCaps(GetDC(0), HORZRES));
    TEST(ViewportExt.cy == -GetDeviceCaps(GetDC(0), VERTRES));
    DeleteDC(hDC);

    return APISTATUS_NORMAL;
}
