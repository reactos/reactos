/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for OffsetClipRgn
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>
#include <wingdi.h>

#define CLIPRGN 1

void Test_OffsetClipRgn()
{
    HDC hdc;
    HRGN hrgn, hrgn2;
    //RECT rect;

    hdc = CreateCompatibleDC(NULL);
    ok(hdc != 0, "CreateCompatibleDC failed, skipping tests.\n");
    if (!hdc) return;

    hrgn2 = CreateRectRgn(0, 0, 0, 0);

    /* Test NULL DC */
    SetLastError(0x12345);
    ok_int(OffsetClipRgn(NULL, 0, 0), ERROR);
    ok_int(GetLastError(), ERROR_INVALID_HANDLE);

    /* Test invalid DC */
    SetLastError(0x12345);
    ok_int(OffsetClipRgn((HDC)(ULONG_PTR)0x12345, 0, 0), ERROR);
    ok((GetLastError() == 0x12345) || (GetLastError() == ERROR_INVALID_HANDLE), "Expected 0x12345 or ERROR_INVALID_HANDLE, got %ld\n", GetLastError());
    SetLastError(0x12345);

    /* Test without a clip region set */
    SetLastError(0x12345);
    ok_int(SelectClipRgn(hdc, NULL), SIMPLEREGION);
    ok_int(OffsetClipRgn(hdc, 0, 0), SIMPLEREGION);
    ok_int(GetLastError(), 0x12345);
    SetLastError(0x12345);

    /* Set a clip region */
    hrgn = CreateRectRgn(10, 10, 20, 30);
    ok_int(SelectClipRgn(hdc, hrgn), NULLREGION);
    DeleteObject(hrgn);
    ok_int(OffsetClipRgn(hdc, 10, 10), SIMPLEREGION);
    ok_int(GetRandomRgn(hdc, hrgn2, CLIPRGN), 1);
    hrgn = CreateRectRgn(20, 20, 30, 40);
    ok_int(EqualRgn(hrgn, hrgn2), TRUE);

    /* Set different scaling */
    SetMapMode(hdc, MM_ANISOTROPIC);
    ok_int(SetViewportExtEx(hdc, 100, 100, NULL), 1);
    ok_int(SetWindowExtEx(hdc, 200, 50, NULL), 1);
    ok_int(OffsetClipRgn(hdc, 10, 10), SIMPLEREGION);
    ok_int(GetRandomRgn(hdc, hrgn2, CLIPRGN), 1);
    hrgn = CreateRectRgn(25, 40, 35, 60);
    ok_int(EqualRgn(hrgn, hrgn2), TRUE);

#if 0
    /* Set different scaling */
    SetMapMode(hdc, MM_ANISOTROPIC);
    ok_int(SetViewportExtEx(hdc, 100, 100, NULL), 1);
    ok_int(SetWindowExtEx(hdc, 80, 350, NULL), 1);
    ok_int(OffsetClipRgn(hdc, 10, 10), SIMPLEREGION);
    ok_int(GetRandomRgn(hdc, hrgn2, CLIPRGN), 1);
    hrgn = CreateRectRgn(33, 23, 43, 43);
    ok_int(EqualRgn(hrgn, hrgn2), TRUE);
#endif

    ok_int(GetLastError(), 0x12345);

}

START_TEST(OffsetClipRgn)
{
    Test_OffsetClipRgn();
}
