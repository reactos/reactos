
#include "precomp.h"

START_TEST(DrawIconEx)
{
    HCURSOR hcursor;
    HBITMAP hbmp;
    ICONINFO ii;
    HDC hdcScreen, hdc;
    BOOL ret;
    HBRUSH hbrush;

    ZeroMemory(&ii, sizeof(ii));

    ii.hbmMask = CreateBitmap(8, 16, 1, 1, NULL);
    ok(ii.hbmMask != NULL, "\n");
    hcursor = CreateIconIndirect(&ii);
    ok(hcursor != NULL, "\n");
    DeleteObject(ii.hbmMask);

    hdcScreen = GetDC(0);
    hbmp = CreateCompatibleBitmap(hdcScreen, 8, 8);
    ok(hbmp != NULL, "\n");
    hdc = CreateCompatibleDC(hdcScreen);
    ok(hdc != NULL, "\n");
    ReleaseDC(0, hdcScreen);

    hbmp = SelectObject(hdc, hbmp);
    ok(hbmp != NULL, "\n");

    hbrush = GetStockObject(DKGRAY_BRUSH);
    ok(hbrush != NULL, "\n");

    ret = DrawIconEx(hdc, 0, 0, hcursor, 8, 8, 0, hbrush, DI_NORMAL);
    ok(ret, "\n");
    DestroyCursor(hcursor);

    /* Try with color */
    ii.hbmMask = CreateBitmap(8, 8, 1, 1, NULL);
    ok(ii.hbmMask != NULL, "\n");
    ii.hbmColor = CreateBitmap(8, 8, 16, 1, NULL);
    ok(ii.hbmColor != NULL, "\n");
    hcursor = CreateIconIndirect(&ii);
    ok(hcursor != NULL, "\n");
    DeleteObject(ii.hbmMask);
    DeleteObject(ii.hbmColor);

    ret = DrawIconEx(hdc, 0, 0, hcursor, 8, 8, 0, hbrush, DI_NORMAL);
    ok(ret, "\n");
    DestroyCursor(hcursor);

    hbmp = SelectObject(hdc, hbmp);
    DeleteObject(hbmp);
    DeleteDC(hdc);
}
