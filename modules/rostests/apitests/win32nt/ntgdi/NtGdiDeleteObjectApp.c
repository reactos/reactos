/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiDeleteObjectApp
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiDeleteObjectApp)
{
    HDC hdc;
    HBITMAP hbmp;
    HBRUSH hbrush;
    HPEN hpen;

    /* Try to delete 0 */
    SetLastError(0);
    TEST(NtGdiDeleteObjectApp(0) == 0);
    TEST(GetLastError() == 0);

    /* Try to delete something with a stockbit */
    SetLastError(0);
    TEST(NtGdiDeleteObjectApp((PVOID)(GDI_HANDLE_STOCK_MASK | 0x1234)) == 1);
    TEST(GetLastError() == 0);

    /* Delete a compatible DC */
    SetLastError(0);
    hdc = CreateCompatibleDC(NULL);
    ASSERT(GdiIsHandleValid(hdc) == 1);
    TEST(NtGdiDeleteObjectApp(hdc) == 1);
    TEST(GetLastError() == 0);
    TEST(GdiIsHandleValid(hdc) == 0);

    /* Delete a display DC */
    SetLastError(0);
    hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
    ASSERT(GdiIsHandleValid(hdc) == 1);
    TEST((hpen=SelectObject(hdc, GetStockObject(WHITE_PEN))) != NULL);
    SelectObject(hdc, hpen);
    TEST(NtGdiDeleteObjectApp(hdc) != 0);
    TEST(GetLastError() == 0);
    TEST(GdiIsHandleValid(hdc) == 1);
    TEST(SelectObject(hdc, GetStockObject(WHITE_PEN)) == NULL);
    TESTX(GetLastError() == ERROR_INVALID_PARAMETER, "GetLasterror returned 0x%08x\n", (unsigned int)GetLastError());

    /* Once more */
    SetLastError(0);
    hdc = GetDC(0);
    ASSERT(GdiIsHandleValid(hdc) == 1);
    TEST(NtGdiDeleteObjectApp(hdc) != 0);
    TEST(GetLastError() == 0);
    TEST(GdiIsHandleValid(hdc) == 1);
    TEST(SelectObject(hdc, GetStockObject(WHITE_PEN)) == NULL);
    TESTX(GetLastError() == ERROR_INVALID_PARAMETER, "GetLasterror returned 0x%08x\n", (unsigned int)GetLastError());
    /* Make sure */
    TEST(NtUserCallOneParam((DWORD_PTR)hdc, ONEPARAM_ROUTINE_RELEASEDC) == 0);

    /* Delete a brush */
    SetLastError(0);
    hbrush = CreateSolidBrush(0x123456);
    ASSERT(GdiIsHandleValid(hbrush) == 1);
    TEST(NtGdiDeleteObjectApp(hbrush) == 1);
    TEST(GetLastError() == 0);
    TEST(GdiIsHandleValid(hbrush) == 0);

    /* Try to delete a stock brush */
    SetLastError(0);
    hbrush = GetStockObject(BLACK_BRUSH);
    ASSERT(GdiIsHandleValid(hbrush) == 1);
    TEST(NtGdiDeleteObjectApp(hbrush) == 1);
    TEST(GetLastError() == 0);
    TEST(GdiIsHandleValid(hbrush) == 1);

    /* Delete a bitmap */
    SetLastError(0);
    hbmp = CreateBitmap(10, 10, 1, 1, NULL);
    ASSERT(GdiIsHandleValid(hbmp) == 1);
    TEST(NtGdiDeleteObjectApp(hbmp) == 1);
    TEST(GetLastError() == 0);
    TEST(GdiIsHandleValid(hbmp) == 0);

    /* Create a DC for further use */
    hdc = CreateCompatibleDC(NULL);
    ASSERT(hdc);

    /* Try to delete a brush that is selected into a DC */
    SetLastError(0);
    hbrush = CreateSolidBrush(0x123456);
    ASSERT(GdiIsHandleValid(hbrush) == 1);
    TEST(NtGdiSelectBrush(hdc, hbrush) != NULL);
    TEST(NtGdiDeleteObjectApp(hbrush) == 1);
    TEST(GetLastError() == 0);
    TEST(GdiIsHandleValid(hbrush) == 1);

    /* Try to delete a bitmap that is selected into a DC */
    SetLastError(0);
    hbmp = CreateBitmap(10, 10, 1, 1, NULL);
    ASSERT(GdiIsHandleValid(hbmp) == 1);
    TEST(NtGdiSelectBitmap(hdc, hbmp) != NULL);

    TEST(NtGdiDeleteObjectApp(hbmp) == 1);
    TEST(GetLastError() == 0);
    TEST(GdiIsHandleValid(hbmp) == 1);

    /* Bitmap get's deleted as soon as we dereference it */
    NtGdiSelectBitmap(hdc, GetStockObject(DEFAULT_BITMAP));
    TEST(GdiIsHandleValid(hbmp) == 0);

    TEST(NtGdiDeleteObjectApp(hbmp) == 1);
    TEST(GetLastError() == 0);
    TEST(GdiIsHandleValid(hbmp) == 0);

    /* Try to delete a brush that is selected into a DC */
    SetLastError(0);
    hbrush = CreateSolidBrush(123);
    ASSERT(GdiIsHandleValid(hbrush) == 1);
    TEST(NtGdiSelectBrush(hdc, hbrush) != NULL);

    TEST(NtGdiDeleteObjectApp(hbrush) == 1);
    TEST(GetLastError() == 0);
    TEST(GdiIsHandleValid(hbrush) == 1);
}
