
INT
Test_NtGdiDeleteObjectApp(PTESTINFO pti)
{
    HDC hdc;
    HBITMAP hbmp;
    HBRUSH hbrush;

    /* Try to delete 0 */
    SetLastError(0);
    TEST(NtGdiDeleteObjectApp(0) == 0);
    TEST(GetLastError() == 0);

    /* Try to delete something with a stockbit */
    SetLastError(0);
    TEST(NtGdiDeleteObjectApp((PVOID)(GDI_HANDLE_STOCK_MASK | 0x1234)) == 1);
    TEST(GetLastError() == 0);

    /* Delete a DC */
    SetLastError(0);
    hdc = CreateCompatibleDC(NULL);
    ASSERT(IsHandleValid(hdc) == 1);
    TEST(NtGdiDeleteObjectApp(hdc) == 1);
    TEST(GetLastError() == 0);
    TEST(IsHandleValid(hdc) == 0);

    /* Delete a brush */
    SetLastError(0);
    hbrush = CreateSolidBrush(0x123456);
    ASSERT(IsHandleValid(hbrush) == 1);
    TEST(NtGdiDeleteObjectApp(hbrush) == 1);
    TEST(GetLastError() == 0);
    TEST(IsHandleValid(hbrush) == 0);

    /* Try to delete a stock brush */
    SetLastError(0);
    hbrush = GetStockObject(BLACK_BRUSH);
    ASSERT(IsHandleValid(hbrush) == 1);
    TEST(NtGdiDeleteObjectApp(hbrush) == 1);
    TEST(GetLastError() == 0);
    TEST(IsHandleValid(hbrush) == 1);

    /* Delete a bitmap */
    SetLastError(0);
    hbmp = CreateBitmap(10, 10, 1, 1, NULL);
    ASSERT(IsHandleValid(hbmp) == 1);
    TEST(NtGdiDeleteObjectApp(hbmp) == 1);
    TEST(GetLastError() == 0);
    TEST(IsHandleValid(hbmp) == 0);

    /* Create a DC for further use */
    hdc = CreateCompatibleDC(NULL);
    ASSERT(hdc);

    /* Try to delete a brush that is selected into a DC */
    SetLastError(0);
    hbrush = CreateSolidBrush(0x123456);
    ASSERT(IsHandleValid(hbrush) == 1);
    TEST(NtGdiSelectBrush(hdc, hbrush));
    TEST(NtGdiDeleteObjectApp(hbrush) == 1);
    TEST(GetLastError() == 0);
    TEST(IsHandleValid(hbrush) == 1);

    /* Try to delete a bitmap that is selected into a DC */
    SetLastError(0);
    hbmp = CreateBitmap(10, 10, 1, 1, NULL);
    ASSERT(IsHandleValid(hbmp) == 1);
    TEST(NtGdiSelectBitmap(hdc, hbmp));

    TEST(NtGdiDeleteObjectApp(hbmp) == 1);
    TEST(GetLastError() == 0);
    TEST(IsHandleValid(hbmp) == 1);

    /* Bitmap get's deleted as soon as we dereference it */
    NtGdiSelectBitmap(hdc, GetStockObject(DEFAULT_BITMAP));
    TEST(IsHandleValid(hbmp) == 0);

    TEST(NtGdiDeleteObjectApp(hbmp) == 1);
    TEST(GetLastError() == 0);
    TEST(IsHandleValid(hbmp) == 0);

    /* Try to delete a brush that is selected into a DC */
    SetLastError(0);
    hbrush = CreateSolidBrush(123);
    ASSERT(IsHandleValid(hbrush) == 1);
    TEST(NtGdiSelectBrush(hdc, hbrush));

    TEST(NtGdiDeleteObjectApp(hbrush) == 1);
    TEST(GetLastError() == 0);
    TEST(IsHandleValid(hbrush) == 1);

    return APISTATUS_NORMAL;
}
