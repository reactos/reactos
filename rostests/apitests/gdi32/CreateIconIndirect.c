
#include <apitest.h>

#include <wingdi.h>
#include <winuser.h>


// FIXME user32

void
Test_GetIconInfo(BOOL fIcon)
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
    ok(iconinfo2.hbmMask != iconinfo.hbmColor, "\n");

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
    ok(bitmap.bmBitsPixel == 32, "\n");
    ok(bitmap.bmBits == NULL, "\n");

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
    ok(bitmap.bmWidthBytes == 32, "%ld\n", bitmap.bmWidthBytes);
    ok(bitmap.bmPlanes == 1, "%d\n", bitmap.bmPlanes);
    ok(bitmap.bmBitsPixel == 32, "%d\n", bitmap.bmBitsPixel);
    ok(bitmap.bmBits == NULL, "\n");


}


START_TEST(CreateIconIndirect)
{
    HCURSOR hcursor;
    HICON hicon;
    ICONINFO iconinfo2;
    BITMAP bitmap;
    DWORD data[] = {0, 0, 0, 0, 0, 0};

    Test_GetIconInfo(0);
    Test_GetIconInfo(1);

    hcursor = LoadCursor(NULL, IDC_APPSTARTING);
    ok(hcursor != 0, "should not fail\n");
    ok(GetIconInfo(hcursor, &iconinfo2), "\n");
    ok(iconinfo2.fIcon == 0, "\n");
    ok(iconinfo2.xHotspot == 0, "%ld\n", iconinfo2.xHotspot);
    ok(iconinfo2.yHotspot == 8, "%ld\n", iconinfo2.yHotspot);
    ok(iconinfo2.hbmMask != NULL, "\n");
    ok(iconinfo2.hbmColor != NULL, "\n");

    ok(GetObject(iconinfo2.hbmMask, sizeof(bitmap), &bitmap), "GetObject failed\n");
    ok(bitmap.bmType == 0, "\n");
    ok(bitmap.bmWidth == 32, "%ld\n", bitmap.bmWidth);
    ok(bitmap.bmHeight == 32, "\n");
    ok(bitmap.bmWidthBytes == 4, "\n");
    ok(bitmap.bmPlanes == 1, "\n");
    ok(bitmap.bmBitsPixel == 1, "\n");
    ok(bitmap.bmBits == NULL, "\n");

    ok(GetObject(iconinfo2.hbmColor, sizeof(bitmap), &bitmap), "GetObject failed\n");
    ok(bitmap.bmType == 0, "\n");
    ok(bitmap.bmWidth == 32, "\n");
    ok(bitmap.bmHeight == 32, "\n");
    ok(bitmap.bmWidthBytes == 32 * bitmap.bmBitsPixel / 8, "\n");
    ok(bitmap.bmPlanes == 1, "\n");
    ok(bitmap.bmBitsPixel == 32, "\n");
    ok(bitmap.bmBits == NULL, "\n");

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


    hicon = CreateIcon(0, 4, 4, 1, 1, (PBYTE)data, (PBYTE)data);
    ok(hicon != 0, "should not fail\n");

    ok(GetIconInfo(hicon, &iconinfo2), "\n");
    ok(iconinfo2.fIcon == 0, "\n");
    ok(iconinfo2.xHotspot == 2, "%ld\n", iconinfo2.xHotspot);
    ok(iconinfo2.yHotspot == 2, "%ld\n", iconinfo2.yHotspot);
    ok(iconinfo2.hbmMask != NULL, "\n");
    ok(iconinfo2.hbmColor == NULL, "\n");

}


