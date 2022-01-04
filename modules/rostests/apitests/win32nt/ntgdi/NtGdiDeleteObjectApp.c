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
    ok_int(NtGdiDeleteObjectApp(0), 0);
    ok_long(GetLastError(), 0);

    /* Try to delete something with a stockbit */
    SetLastError(0);
    ok_int(NtGdiDeleteObjectApp((PVOID)(GDI_HANDLE_STOCK_MASK | 0x1234)), 1);
    ok_long(GetLastError(), 0);

    /* Delete a compatible DC */
    SetLastError(0);
    hdc = CreateCompatibleDC(NULL);
    ok_int(GdiIsHandleValid(hdc), 1);
    ok_int(NtGdiDeleteObjectApp(hdc), 1);
    ok_long(GetLastError(), 0);
    ok_int(GdiIsHandleValid(hdc), 0);

#ifdef _WIN64
    /* Test upper 32 bits */
    SetLastError(0);
    hdc = (HDC)((ULONG64)CreateCompatibleDC(NULL) | 0xFFFFFFFF00000000ULL);
    ok_int(GdiIsHandleValid(hdc), 1);
    ok_int(NtGdiDeleteObjectApp(hdc), 1);
    ok_long(GetLastError(), 0);
    ok_int(GdiIsHandleValid(hdc), 0);

    SetLastError(0);
    hdc = (HDC)((ULONG64)CreateCompatibleDC(NULL) | 0x537F9F2F00000000ULL);
    ok_int(GdiIsHandleValid(hdc), 1);
    ok_int(NtGdiDeleteObjectApp(hdc), 1);
    ok_long(GetLastError(), 0);
    ok_int(GdiIsHandleValid(hdc), 0);
#endif

    /* Delete a display DC */
    SetLastError(0);
    hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
    ok_int(GdiIsHandleValid(hdc), 1);
    ok((hpen=SelectObject(hdc, GetStockObject(WHITE_PEN))) != NULL, "hpen was NULL.\n");
    SelectObject(hdc, hpen);
    ok(NtGdiDeleteObjectApp(hdc) != 0, "NtGdiDeleteObjectApp(hdc) was zero.\n");
    ok_long(GetLastError(), 0);
    ok_int(GdiIsHandleValid(hdc), 1);
    ok_ptr(SelectObject(hdc, GetStockObject(WHITE_PEN)), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Once more */
    SetLastError(0);
    hdc = GetDC(0);
    ok_int(GdiIsHandleValid(hdc), 1);
    ok(NtGdiDeleteObjectApp(hdc) != 0, "NtGdiDeleteObjectApp(hdc) was zero.\n");
    ok_long(GetLastError(), 0);
    ok_int(GdiIsHandleValid(hdc), 1);
    ok_ptr(SelectObject(hdc, GetStockObject(WHITE_PEN)), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);
    /* Make sure */
    ok_ptr((void *)NtUserCallOneParam((DWORD_PTR)hdc, ONEPARAM_ROUTINE_RELEASEDC), NULL);


    /* Delete a brush */
    SetLastError(0);
    hbrush = CreateSolidBrush(0x123456);
    ok_int(GdiIsHandleValid(hbrush), 1);
    ok_int(NtGdiDeleteObjectApp(hbrush), 1);
    ok_long(GetLastError(), 0);
    ok_int(GdiIsHandleValid(hbrush), 0);

    /* Try to delete a stock brush */
    SetLastError(0);
    hbrush = GetStockObject(BLACK_BRUSH);
    ok_int(GdiIsHandleValid(hbrush), 1);
    ok_int(NtGdiDeleteObjectApp(hbrush), 1);
    ok_long(GetLastError(), 0);
    ok_int(GdiIsHandleValid(hbrush), 1);

    /* Delete a bitmap */
    SetLastError(0);
    hbmp = CreateBitmap(10, 10, 1, 1, NULL);
    ok_int(GdiIsHandleValid(hbmp), 1);
    ok_int(NtGdiDeleteObjectApp(hbmp), 1);
    ok_long(GetLastError(), 0);
    ok_int(GdiIsHandleValid(hbmp), 0);

    /* Create a DC for further use */
    hdc = CreateCompatibleDC(NULL);
    ok(hdc != NULL, "hdc was NULL.\n");

    /* Try to delete a brush that is selected into a DC */
    SetLastError(0);
    hbrush = CreateSolidBrush(0x123456);
    ok_int(GdiIsHandleValid(hbrush), 1);
    ok(NtGdiSelectBrush(hdc, hbrush) != NULL, "NtGdiSelectBrush(hdc, hbrush) was NULL.\n");
    ok_int(NtGdiDeleteObjectApp(hbrush), 1);
    ok_long(GetLastError(), 0);
    ok_int(GdiIsHandleValid(hbrush), 1);

    /* Try to delete a bitmap that is selected into a DC */
    SetLastError(0);
    hbmp = CreateBitmap(10, 10, 1, 1, NULL);
    ok_int(GdiIsHandleValid(hbmp), 1);
    ok(NtGdiSelectBitmap(hdc, hbmp) != NULL, "NtGdiSelectBitmap(hdc, hbmp) was NULL.\n");

    ok_int(NtGdiDeleteObjectApp(hbmp), 1);
    ok_long(GetLastError(), 0);
    ok_int(GdiIsHandleValid(hbmp), 1);

    /* Bitmap get's deleted as soon as we dereference it */
    NtGdiSelectBitmap(hdc, GetStockObject(DEFAULT_BITMAP));
    ok_int(GdiIsHandleValid(hbmp), 0);

    ok_int(NtGdiDeleteObjectApp(hbmp), 1);
    ok_long(GetLastError(), 0);
    ok_int(GdiIsHandleValid(hbmp), 0);

    /* Try to delete a brush that is selected into a DC */
    SetLastError(0);
    hbrush = CreateSolidBrush(123);
    ok_int(GdiIsHandleValid(hbrush), 1);
    ok(NtGdiSelectBrush(hdc, hbrush) != NULL, "NtGdiSelectBrush(hdc, hbrush) was NULL.\n");

    ok_int(NtGdiDeleteObjectApp(hbrush), 1);
    ok_long(GetLastError(), 0);
    ok_int(GdiIsHandleValid(hbrush), 1);
}
