
#include "precomp.h"

// FIXME user32

void
Test_GetIconInfo(BOOL fIcon, DWORD screen_bpp)
{
    HICON hicon;
    ICONINFO iconinfo, iconinfo2;
    BITMAP bitmap;

    iconinfo.fIcon = fIcon;
    iconinfo.xHotspot = 0;
    iconinfo.yHotspot = 0;
    iconinfo.hbmMask = NULL;
    iconinfo.hbmColor = NULL;

    hicon = CreateIconIndirect(&iconinfo);
    ok(hicon == 0, "should fail\n");

    iconinfo.hbmMask = CreateBitmap(8, 16, 1, 1, NULL);
    hicon = CreateIconIndirect(&iconinfo);
    ok(hicon != 0, "should not fail\n");

    ok(GetIconInfo(hicon, &iconinfo2), "\n");
    ok(iconinfo2.fIcon == iconinfo.fIcon, "\n");
    if (fIcon)
    {
        ok(iconinfo2.xHotspot == 4, "%ld\n", iconinfo2.xHotspot);
        ok(iconinfo2.yHotspot == 4, "%ld\n", iconinfo2.yHotspot);
    }
    else
    {
        ok(iconinfo2.xHotspot == 0, "%ld\n", iconinfo2.xHotspot);
        ok(iconinfo2.yHotspot == 0, "%ld\n", iconinfo2.yHotspot);
    }
    ok(iconinfo2.hbmMask != NULL, "\n");
    ok(iconinfo2.hbmMask != iconinfo.hbmMask, "\n");
    ok(iconinfo2.hbmColor == NULL, "\n");
    DeleteObject(iconinfo2.hbmMask);

    ok(GetIconInfo(hicon, &iconinfo2), "\n");
    ok(iconinfo2.fIcon == iconinfo.fIcon, "\n");
    if (fIcon)
    {
        ok(iconinfo2.xHotspot == 4, "%ld\n", iconinfo2.xHotspot);
        ok(iconinfo2.yHotspot == 4, "%ld\n", iconinfo2.yHotspot);
    }
    else
    {
        ok(iconinfo2.xHotspot == 0, "%ld\n", iconinfo2.xHotspot);
        ok(iconinfo2.yHotspot == 0, "%ld\n", iconinfo2.yHotspot);
    }
    ok(iconinfo2.hbmMask != NULL, "\n");
    ok(iconinfo2.hbmMask != iconinfo.hbmMask, "\n");
    ok(iconinfo2.hbmColor == NULL, "\n");
    DeleteObject(iconinfo2.hbmMask);
    ok(DestroyIcon(hicon), "\n");

    iconinfo.hbmColor = CreateBitmap(2, 2, 1, 1, NULL);
    hicon = CreateIconIndirect(&iconinfo);
    ok(hicon != 0, "should not fail\n");

    ok(GetIconInfo(hicon, &iconinfo2), "\n");
    ok(iconinfo2.fIcon == iconinfo.fIcon, "\n");
    if (fIcon)
    {
        ok(iconinfo2.xHotspot == 4, "%ld\n", iconinfo2.xHotspot);
        ok(iconinfo2.yHotspot == 8, "%ld\n", iconinfo2.yHotspot);
    }
    else
    {
        ok(iconinfo2.xHotspot == 0, "%ld\n", iconinfo2.xHotspot);
        ok(iconinfo2.yHotspot == 0, "%ld\n", iconinfo2.yHotspot);
    }
    ok(iconinfo2.hbmMask != NULL, "\n");
    ok(iconinfo2.hbmMask != iconinfo.hbmMask, "\n");
    ok(iconinfo2.hbmColor != NULL, "\n");
    ok(iconinfo2.hbmColor != iconinfo.hbmColor, "\n");

    ok(GetObject(iconinfo2.hbmMask, sizeof(bitmap), &bitmap), "GetObject failed\n");
    ok(bitmap.bmType == 0, "\n");
    ok(bitmap.bmWidth == 8, "\n");
    ok(bitmap.bmHeight == 16, "\n");
    ok(bitmap.bmWidthBytes == 2, "\n");
    ok(bitmap.bmPlanes == 1, "\n");
    ok(bitmap.bmBitsPixel == 1, "\n");
    ok(bitmap.bmBits == NULL, "\n");

    ok(GetObject(iconinfo2.hbmColor, sizeof(bitmap), &bitmap), "GetObject failed\n");
    ok(bitmap.bmType == 0, "\n");
    ok(bitmap.bmWidth == 8, "\n");
    ok(bitmap.bmHeight == 16, "\n");
    ok(bitmap.bmWidthBytes == 8 * bitmap.bmBitsPixel / 8, "\n");
    ok(bitmap.bmPlanes == 1, "\n");
    ok(bitmap.bmBitsPixel == screen_bpp, "%d\n", bitmap.bmBitsPixel);
    ok(bitmap.bmBits == NULL, "\n");
    DeleteObject(iconinfo2.hbmMask);
    DeleteObject(iconinfo2.hbmColor);
    ok(DestroyIcon(hicon), "\n");

    DeleteObject(iconinfo.hbmMask);
    iconinfo.hbmMask = NULL;
    hicon = CreateIconIndirect(&iconinfo);
    ok(hicon == 0, "should fail\n");

    DeleteObject(iconinfo.hbmColor);
    iconinfo.hbmColor = CreateCompatibleBitmap(GetDC(0), 16, 16);
    hicon = CreateIconIndirect(&iconinfo);
    ok(hicon == 0, "should fail\n");

    iconinfo.hbmMask = CreateCompatibleBitmap(GetDC(0), 8, 16);
    hicon = CreateIconIndirect(&iconinfo);
    ok(hicon != 0, "should not fail\n");

    ok(GetIconInfo(hicon, &iconinfo2), "\n");

    ok(GetObject(iconinfo2.hbmMask, sizeof(bitmap), &bitmap), "GetObject failed\n");
    ok(bitmap.bmType == 0, "\n");
    ok(bitmap.bmWidth == 8, "%ld\n", bitmap.bmWidth);
    ok(bitmap.bmHeight == 16, "%ld\n", bitmap.bmHeight);
    ok(bitmap.bmWidthBytes == 2, "%ld\n", bitmap.bmWidthBytes);
    ok(bitmap.bmPlanes == 1, "%d\n", bitmap.bmPlanes);
    ok(bitmap.bmBitsPixel == 1, "%d\n", bitmap.bmBitsPixel);
    ok(bitmap.bmBits == NULL, "\n");

    ok(GetObject(iconinfo2.hbmColor, sizeof(bitmap), &bitmap), "GetObject failed\n");
    ok(bitmap.bmType == 0, "\n");
    ok(bitmap.bmWidth == 8, "%ld\n", bitmap.bmWidth);
    ok(bitmap.bmHeight == 16, "%ld\n", bitmap.bmHeight);
    ok(bitmap.bmWidthBytes == screen_bpp, "%ld\n", bitmap.bmWidthBytes);
    ok(bitmap.bmPlanes == 1, "%d\n", bitmap.bmPlanes);
    ok(bitmap.bmBitsPixel == screen_bpp, "%d\n", bitmap.bmBitsPixel);
    ok(bitmap.bmBits == NULL, "\n");
    DeleteObject(iconinfo2.hbmMask);
    DeleteObject(iconinfo2.hbmColor);
    ok(DestroyIcon(hicon), "\n");
}


START_TEST(GetIconInfo)
{
    HCURSOR hcursor;
    ICONINFO iconinfo2;
    BITMAP bitmap;
    DWORD data[] = {0, 0, 0, 0, 0, 0};
    DWORD bpp, screenbpp, creationbpp;
    DEVMODEW dm;

    dm.dmSize = sizeof(dm);
    dm.dmDriverExtra = 0;

    /* Test icons behaviour regarding display settings */
    ok(EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm), "\n");
    screenbpp = dm.dmBitsPerPel;

    trace("Icon default size: %dx%d.\n", GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    trace("Cursor default size: %dx%d.\n", GetSystemMetrics(SM_CXCURSOR), GetSystemMetrics(SM_CYCURSOR));

    trace("Screen bpp: %lu.\n", screenbpp);
    Test_GetIconInfo(FALSE, screenbpp);
    Test_GetIconInfo(TRUE, screenbpp);

    hcursor = LoadCursor(GetModuleHandle(NULL), "TESTCURSOR");
    ok(hcursor != 0, "should not fail, error %ld\n", GetLastError());
    ok(GetIconInfo(hcursor, &iconinfo2), "\n");
    ok(iconinfo2.fIcon == 0, "\n");
    ok(iconinfo2.xHotspot == 8, "%ld\n", iconinfo2.xHotspot);
    ok(iconinfo2.yHotspot == 29, "%ld\n", iconinfo2.yHotspot);
    ok(iconinfo2.hbmMask != NULL, "\n");
    ok(iconinfo2.hbmColor != NULL, "\n");
    /* Delete objects */
    DeleteObject(iconinfo2.hbmColor);
    DeleteObject(iconinfo2.hbmMask);
    /* Delete cursor */
    DestroyCursor(hcursor);

    /* To sum it up:
     * There are two criteria when using icons: performance and aesthetics (=alpha channel).
     * Performance asks for bit parity with the screen display.
     * Aesthetics needs a 32bpp bitmap.
     * The behaviour is basically : aesthetics first if already loaded.
     * ie: if the 32bpp bitmap were already loaded because of previous display settings, always use it.
     * Otherwise, use the bitmap matching the screen bit depth.
     */

    /* if we use LR_SHARED here, and reverse the loop (32->16), then hbmColor.bmBitsPixel is always 32. */
    for(creationbpp = 16; creationbpp <=32; creationbpp += 8)
    {
        dm.dmBitsPerPel = creationbpp;
        if(ChangeDisplaySettingsExW(NULL, &dm, NULL, 0, NULL) != DISP_CHANGE_SUCCESSFUL)
        {
            skip("Unable to change bpp to %lu.\n", creationbpp);
            continue;
        }
        trace("starting with creationbpp = %lu\n", creationbpp);
        hcursor = LoadImage(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDI_TEST),
            IMAGE_ICON,
            0,
            0,
            LR_DEFAULTCOLOR);
        ok(hcursor != 0, "should not fail\n");

        /* If we reverse the loop here (32->16 bpp), then hbmColor.bmBitsPixel is always 32 */
        for(bpp = 16; bpp <=32; bpp += 8)
        {
            trace("testing resetting to %lu\n", bpp);
            dm.dmBitsPerPel = bpp;
            if(ChangeDisplaySettingsExW(NULL, &dm, NULL, 0, NULL) != DISP_CHANGE_SUCCESSFUL)
            {
                skip("Unable to change bpp to %lu.\n", bpp);
                continue;
            }
            ok(GetIconInfo(hcursor, &iconinfo2), "\n");
            ok(iconinfo2.fIcon == 1, "\n");
            ok(iconinfo2.xHotspot == 24, "%ld\n", iconinfo2.xHotspot);
            ok(iconinfo2.yHotspot == 24, "%ld\n", iconinfo2.yHotspot);
            ok(iconinfo2.hbmMask != NULL, "\n");
            ok(iconinfo2.hbmColor != NULL, "\n");

            ok(GetObject(iconinfo2.hbmMask, sizeof(bitmap), &bitmap), "GetObject failed\n");
            ok(bitmap.bmType == 0, "\n");
            ok(bitmap.bmWidth == 48, "%ld\n", bitmap.bmWidth);
            ok(bitmap.bmHeight == 48, "\n");
            ok(bitmap.bmWidthBytes == 6, "\n");
            ok(bitmap.bmPlanes == 1, "\n");
            ok(bitmap.bmBitsPixel == 1, "\n");
            ok(bitmap.bmBits == NULL, "\n");

            ok(GetObject(iconinfo2.hbmColor, sizeof(bitmap), &bitmap), "GetObject failed\n");
            ok(bitmap.bmType == 0, "\n");
            ok(bitmap.bmWidth == 48, "\n");
            ok(bitmap.bmHeight == 48, "\n");
            ok(bitmap.bmWidthBytes == 48 * bitmap.bmBitsPixel / 8, "\n");
            ok(bitmap.bmPlanes == 1, "\n");
            ok(bitmap.bmBitsPixel == (creationbpp == 32 ? 32 : bpp), "creationbpp: %lu, bpp: %lu:\n", creationbpp, bpp);
            ok(bitmap.bmBits == NULL, "\n");

            /* Delete objects */
            DeleteObject(iconinfo2.hbmColor);
            DeleteObject(iconinfo2.hbmMask);
        }
        ok(DestroyIcon(hcursor), "\n");
    }
    /* Restore display settings */
    dm.dmBitsPerPel = screenbpp;
    if(ChangeDisplaySettingsExW(NULL, &dm, NULL, 0, NULL) != DISP_CHANGE_SUCCESSFUL)
        trace("Unable to go back to previous display settings. Sorry.\n");

    hcursor = CreateCursor(NULL, 1, 2, 4, 4, data, data);
    ok(hcursor != 0, "should not fail\n");
    ok(GetIconInfo(hcursor, &iconinfo2), "\n");
    ok(iconinfo2.fIcon == 0, "\n");
    ok(iconinfo2.xHotspot == 1, "%ld\n", iconinfo2.xHotspot);
    ok(iconinfo2.yHotspot == 2, "%ld\n", iconinfo2.yHotspot);
    ok(iconinfo2.hbmMask != NULL, "\n");
    ok(iconinfo2.hbmColor == NULL, "\n");

    ok(GetObject(iconinfo2.hbmMask, sizeof(bitmap), &bitmap), "GetObject failed\n");
    ok(bitmap.bmType == 0, "\n");
    ok(bitmap.bmWidth == 4, "%ld\n", bitmap.bmWidth);
    ok(bitmap.bmHeight == 8, "%ld\n", bitmap.bmHeight);
    ok(bitmap.bmWidthBytes == 2, "%ld\n", bitmap.bmWidthBytes);
    ok(bitmap.bmPlanes == 1, "\n");
    ok(bitmap.bmBitsPixel == 1, "\n");
    ok(bitmap.bmBits == NULL, "\n");

    /* Delete objects */
    DeleteObject(iconinfo2.hbmColor);
    DeleteObject(iconinfo2.hbmMask);
    /* Delete cursor */
    DestroyCursor(hcursor);
}
