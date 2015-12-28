/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiGetStockObject
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtGdiGetStockObject)
{
    HANDLE handle = NULL;
    BITMAP bitmap;

    /* BRUSH testing */
    handle = (HANDLE) NtGdiGetStockObject(WHITE_BRUSH);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BRUSH);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(LTGRAY_BRUSH);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BRUSH);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(GRAY_BRUSH);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BRUSH);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(DKGRAY_BRUSH);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BRUSH);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(BLACK_BRUSH);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BRUSH);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(NULL_BRUSH);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BRUSH);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    /* PEN testing */
    handle = (HANDLE) NtGdiGetStockObject(WHITE_PEN);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_PEN);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) != 0);

    handle = (HANDLE) NtGdiGetStockObject(BLACK_PEN);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_PEN);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) != 0);

    handle = (HANDLE) NtGdiGetStockObject(NULL_PEN);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_PEN);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    /* Not inuse ? */
    RTEST(NtGdiGetStockObject(9) == 0);

    /* FONT testing */
    handle = (HANDLE) NtGdiGetStockObject(OEM_FIXED_FONT);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_FONT);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) != 0);

    handle = (HANDLE) NtGdiGetStockObject(ANSI_FIXED_FONT);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_FONT);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(ANSI_VAR_FONT);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_FONT);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(SYSTEM_FONT);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_FONT);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(DEVICE_DEFAULT_FONT);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_FONT);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(SYSTEM_FIXED_FONT);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_FONT);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(DEFAULT_GUI_FONT);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_FONT);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    /* PALETTE testing */
    handle = (HANDLE) NtGdiGetStockObject(DEFAULT_PALETTE);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_PALETTE);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    /* DC testing */
    handle = (HANDLE) NtGdiGetStockObject(DC_BRUSH);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BRUSH);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    handle = (HANDLE) NtGdiGetStockObject(DC_PEN);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_PEN);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);


    /*  ? testing */
    handle = (HANDLE) NtGdiGetStockObject(20);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_COLORSPACE);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    /* value 21 is getting back 1x1 1Bpp Bitmap */
    handle = (HANDLE) NtGdiGetStockObject(21);
    RTEST(handle != 0);
    RTEST(GDI_HANDLE_GET_TYPE(handle) == GDI_OBJECT_TYPE_BITMAP);
    RTEST(GDI_HANDLE_IS_STOCKOBJ(handle) == TRUE);

    RTEST(GetObject(handle, sizeof(BITMAP), &bitmap) == sizeof(BITMAP));
    RTEST(bitmap.bmType == 0);
    RTEST(bitmap.bmWidth == 1);
    RTEST(bitmap.bmHeight == 1);
    RTEST(bitmap.bmWidthBytes == 2);
    RTEST(bitmap.bmPlanes == 1);
    RTEST(bitmap.bmBitsPixel == 1);
    RTEST(bitmap.bmBits == 0);


    RTEST(NtGdiGetStockObject(22) == 0);
    RTEST(NtGdiGetStockObject(23) == 0);

}
