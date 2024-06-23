/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiCreateBitmap
 * PROGRAMMERS:
 */

#include "../win32nt.h"

void Test_NtGdiCreateBitmap_Params(void)
{
    HBITMAP hBmp;
    BITMAP bitmap;
    BYTE BitmapData[10] = {0x11, 0x22, 0x33};

    /* Test simple params */
    SetLastError(ERROR_SUCCESS);
    ok((hBmp = NtGdiCreateBitmap(1, 1, 1, 1, NULL)) != NULL, "hBmp was NULL.\n");
    ok_long(GetLastError(), ERROR_SUCCESS);
    DeleteObject(hBmp);

    /* Test all zero */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(0, 0, 0, 0, NULL), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test cx == 0 */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(0, 1, 1, 1, NULL), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test negative cx */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(-10, 1, 1, 1, NULL), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test cy == 0 */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(1, 0, 1, 1, NULL), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test negative cy */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(1, -2, 1, 1, NULL), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test negative cy and valid bits */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(1, -2, 1, 1, BitmapData), NULL);
    ok_long(GetLastError(), ERROR_SUCCESS);

    /* Test negative cy and invalid bits */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(1, -2, 1, 1, (BYTE*)(LONG_PTR)0x80001234), NULL);
    ok_long(GetLastError(), ERROR_SUCCESS);

#ifndef _WIN64 // Win64 doesn't fail here
    /* Test huge size */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(100000, 100000, 1, 1, NULL), NULL);
    ok_long(GetLastError(), ERROR_NOT_ENOUGH_MEMORY);
#endif

    /* Test too huge size */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(100000, 100000, 1, 32, NULL), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test huge size and valid bits */
    SetLastError(ERROR_SUCCESS);
    TEST(NtGdiCreateBitmap(1000, 1000, 1, 1, BitmapData) == NULL);
    ok_long(GetLastError(), ERROR_SUCCESS);

    /* Test huge size and invalid bits */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(100000, 100000, 1, 1, (BYTE*)(LONG_PTR)0x80001234), NULL);
    ok_long(GetLastError(), ERROR_SUCCESS);

    /* Test cPlanes == 0 */
    SetLastError(ERROR_SUCCESS);
    ok((hBmp = NtGdiCreateBitmap(1, 1, 0, 1, NULL)) != NULL, "hBmp was NULL.\n");
    ok_long(GetLastError(), ERROR_SUCCESS);
    ok_int(GetObject(hBmp, sizeof(BITMAP), &bitmap), (int)sizeof(BITMAP));
    ok_int(bitmap.bmType, 0);
    ok_int(bitmap.bmWidth, 1);
    ok_int(bitmap.bmHeight, 1);
    ok_int(bitmap.bmWidthBytes, 2);
    ok_int(bitmap.bmPlanes, 1);
    ok_int(bitmap.bmBitsPixel, 1);
    DeleteObject(hBmp);

    /* Test big cPlanes */
    SetLastError(ERROR_SUCCESS);
    ok((hBmp = NtGdiCreateBitmap(1, 1, 32, 1, NULL)) != NULL, "hBmp was NULL.\n");
    ok_long(GetLastError(), ERROR_SUCCESS);
    DeleteObject(hBmp);

    ok_ptr(NtGdiCreateBitmap(1, 1, 33, 1, NULL), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test cBPP == 0 */
    SetLastError(ERROR_SUCCESS);
    ok((hBmp = NtGdiCreateBitmap(1, 1, 1, 0, NULL)) != NULL, "hBmp was NULL.\n");
    ok_long(GetLastError(), ERROR_SUCCESS);
    ok_int(GetObject(hBmp, sizeof(BITMAP), &bitmap), (int)sizeof(BITMAP));
    ok_int(bitmap.bmType, 0);
    ok_int(bitmap.bmWidth, 1);
    ok_int(bitmap.bmHeight, 1);
    ok_int(bitmap.bmWidthBytes, 2);
    ok_int(bitmap.bmPlanes, 1);
    ok_int(bitmap.bmBitsPixel, 1);
    DeleteObject(hBmp);

    /* Test negative cBPP */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(1, 1, 1, -1, NULL), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test bad cBPP */
    SetLastError(ERROR_SUCCESS);
    ok((hBmp = NtGdiCreateBitmap(1, 1, 1, 3, NULL)) != NULL, "hBmp was NULL.\n");
    ok_int(GetObject(hBmp, sizeof(BITMAP), &bitmap), (int)sizeof(BITMAP));
    ok_int(bitmap.bmBitsPixel, 4);
    DeleteObject(hBmp);

    ok((hBmp = NtGdiCreateBitmap(1, 1, 1, 6, NULL)) != NULL, "hBmp was NULL.\n");
    ok_int(GetObject(hBmp, sizeof(BITMAP), &bitmap), (int)sizeof(BITMAP));
    ok_int(bitmap.bmBitsPixel, 8);
    DeleteObject(hBmp);

    ok((hBmp = NtGdiCreateBitmap(1, 1, 1, 15, NULL)) != NULL, "hBmp was NULL.\n");
    ok_int(GetObject(hBmp, sizeof(BITMAP), &bitmap), (int)sizeof(BITMAP));
    ok_int(bitmap.bmBitsPixel, 16);
    DeleteObject(hBmp);

    ok((hBmp = NtGdiCreateBitmap(1, 1, 1, 17, NULL)) != NULL, "hBmp was NULL.\n");
    ok_int(GetObject(hBmp, sizeof(BITMAP), &bitmap), (int)sizeof(BITMAP));
    ok_int(bitmap.bmBitsPixel, 24);
    DeleteObject(hBmp);

    ok((hBmp = NtGdiCreateBitmap(1, 1, 3, 7, NULL)) != NULL, "hBmp was NULL.\n");
    ok_int(GetObject(hBmp, sizeof(BITMAP), &bitmap), (int)sizeof(BITMAP));
    ok_int(bitmap.bmBitsPixel, 24);
    DeleteObject(hBmp);

    ok((hBmp = NtGdiCreateBitmap(1, 1, 1, 25, NULL)) != NULL, "hBmp was NULL.\n");
    ok_int(GetObject(hBmp, sizeof(BITMAP), &bitmap), (int)sizeof(BITMAP));
    ok_int(bitmap.bmBitsPixel, 32);
    DeleteObject(hBmp);

    ok_long(GetLastError(), ERROR_SUCCESS);

    ok_ptr(NtGdiCreateBitmap(1, 1, 1, 33, NULL), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test bad pointer */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(1, 1, 1, 1, (BYTE*)(LONG_PTR)0x80001234), NULL);
    ok_long(GetLastError(), ERROR_SUCCESS);

    /* Test pointer alignment */
    SetLastError(ERROR_SUCCESS);
    ok((hBmp = NtGdiCreateBitmap(1, 1, 1, 1, &BitmapData[1])) != NULL, "hBmp was NULL.\n");
    ok_long(GetLastError(), ERROR_SUCCESS);
    DeleteObject(hBmp);

    /* Test normal params */
    SetLastError(ERROR_SUCCESS);
    ok((hBmp = NtGdiCreateBitmap(5, 7, 2, 4, NULL)) != NULL, "hBmp was NULL.\n");
    ok_long(GetLastError(), ERROR_SUCCESS);
    ok_int(GetObject(hBmp, sizeof(BITMAP), &bitmap), (int)sizeof(BITMAP));
    ok_int(bitmap.bmType, 0);
    ok_int(bitmap.bmWidth, 5);
    ok_int(bitmap.bmHeight, 7);
    ok_int(bitmap.bmWidthBytes, 6);
    ok_int(bitmap.bmPlanes, 1);
    ok_int(bitmap.bmBitsPixel, 8);
    DeleteObject(hBmp);


    /* Test height 0 params */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(1, 0, 1, 1, NULL), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test height -1 params */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(1, -1, 1, 1, NULL), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test witdth 0 params */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(0, 1, 1, 1, NULL), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test witdth -1 params */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(-1, 0, 1, 1, NULL), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test witdth -1 params */
    SetLastError(ERROR_SUCCESS);
    ok_ptr(NtGdiCreateBitmap(0, 0, 1, 1, NULL), NULL);
    ok_long(GetLastError(), ERROR_INVALID_PARAMETER);
}

START_TEST(NtGdiCreateBitmap)
{

    Test_NtGdiCreateBitmap_Params();
//  Test_NtGdiCreateBitmap_Pixel();

}
