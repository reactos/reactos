


INT
Test_CreateBitmapIndirect(PTESTINFO pti)
{
    HBITMAP win_hBmp;
    BITMAP win_bitmap;

    win_bitmap.bmBits = 0;
    win_bitmap.bmBitsPixel = 1;
    win_bitmap.bmHeight = 0;
    win_bitmap.bmPlanes = 1;
    win_bitmap.bmType = 1;
    win_bitmap.bmWidth = 0;
    win_bitmap.bmWidthBytes = 2;
    win_hBmp = CreateBitmapIndirect(&win_bitmap);
    RTEST(win_hBmp != 0);

    DeleteObject(win_hBmp);

    RtlZeroMemory(&win_bitmap,sizeof(BITMAP));
    win_bitmap.bmBits = 0;
    win_bitmap.bmBitsPixel = 1;
    win_bitmap.bmHeight = 0;
    win_bitmap.bmPlanes = 1;
    win_bitmap.bmType = 1;
    win_bitmap.bmWidth = 0;
    win_bitmap.bmWidthBytes = 1;
    win_hBmp = CreateBitmapIndirect(&win_bitmap);
    RTEST(win_hBmp == 0);

    RtlZeroMemory(&win_bitmap,sizeof(BITMAP));
    win_bitmap.bmBits = 0;
    win_bitmap.bmBitsPixel = 1;
    win_bitmap.bmHeight = 0;
    win_bitmap.bmPlanes = 1;
    win_bitmap.bmType = 1;
    win_bitmap.bmWidth = 0;
    win_bitmap.bmWidthBytes = 3;
    win_hBmp = CreateBitmapIndirect(&win_bitmap);
    RTEST(win_hBmp == 0);

    RtlZeroMemory(&win_bitmap,sizeof(BITMAP));
    win_bitmap.bmBits = 0;
    win_bitmap.bmBitsPixel = 1;
    win_bitmap.bmHeight = 0;
    win_bitmap.bmPlanes = 1;
    win_bitmap.bmType = 1;
    win_bitmap.bmWidth = 0;
    win_bitmap.bmWidthBytes = 4;
    win_hBmp = CreateBitmapIndirect(&win_bitmap);
    RTEST(win_hBmp != 0);

    DeleteObject(win_hBmp);

	return APISTATUS_NORMAL;
}
