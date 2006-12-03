#include <precomp.h>


BOOL
DisplayBlackAndWhite(HWND hwnd,
                     HDC hdcMem,
                     HBITMAP hBitmap)
{
    BITMAPINFO bi;
    BITMAP bitmap;
    BOOL bRes;
    DWORD Count = 0;
    INT i, j;
    PBYTE pBits;
    RECT rc;

    GetObject(hBitmap,
              sizeof(BITMAP),
              &bitmap);

    /* Bitmap header */
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = bitmap.bmWidth;
    bi.bmiHeader.biHeight = bitmap.bmHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = bitmap.bmWidth * bitmap.bmHeight * 4;
    bi.bmiHeader.biClrUsed = 0;
    bi.bmiHeader.biClrImportant = 0;

    /* Buffer */
    pBits = (PBYTE)HeapAlloc(ProcessHeap,
                             0,
                             bitmap.bmWidth * bitmap.bmHeight * 4);
    if (!pBits)
        return FALSE;

    /* get the bits from the original bitmap */
    bRes = GetDIBits(hdcMem,
                     hBitmap,
                     0,
                     bitmap.bmHeight,
                     pBits,
                     &bi,
                     DIB_RGB_COLORS);

    for (i = 0; i < bitmap.bmHeight; i++)
    {
        for (j = 0; j < bitmap.bmWidth; j++)
        {
            DWORD Val = 0;
            INT b, g, r;

            CopyMemory(&Val,
                       &pBits[Count],
                       4);

            /* Get pixels in reverse order */
            b = GetRValue(Val);
            g = GetGValue(Val);
            r = GetBValue(Val);

            // get the average color value
            Val = (r+g+b)/3;

            // assign to RGB color            
            Val = RGB(Val, Val, Val);
            CopyMemory(&pBits[Count], 
                       &Val, 
                       4);

            Count+=4;
        }
    }

    /* Set the new pixel bits */
    SetDIBits(hdcMem,
              hBitmap,
              0,
              bRes,
              pBits,
              &bi,
              DIB_RGB_COLORS);

    HeapFree(ProcessHeap,
             0,
             pBits);

    GetClientRect(hwnd,
                  &rc);

    InvalidateRect(hwnd,
                   &rc,
                   FALSE);

    return TRUE;
}


BOOL
DisplayInvertedColors(HWND hwnd,
                      HDC hdcMem,
                      HBITMAP hBitmap)
{
    BITMAPINFO bi;
    BITMAP bitmap;
    BOOL bRes;
    DWORD Count = 0;
    INT i, j;
    PBYTE pBits;
    RECT rc;

    GetObject(hBitmap,
              sizeof(BITMAP),
              &bitmap);

    /* Bitmap header */
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = bitmap.bmWidth;
    bi.bmiHeader.biHeight = bitmap.bmHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = bitmap.bmWidth * bitmap.bmHeight * 4;
    bi.bmiHeader.biClrUsed = 0;
    bi.bmiHeader.biClrImportant = 0;

    /* Buffer */
    pBits = (PBYTE)HeapAlloc(ProcessHeap,
                             0,
                             bitmap.bmWidth * bitmap.bmHeight * 4);
    if (!pBits)
        return FALSE;

    /* get the bits from the original bitmap */
    bRes = GetDIBits(hdcMem,
                     hBitmap,
                     0,
                     bitmap.bmHeight,
                     pBits,
                     &bi,
                     DIB_RGB_COLORS);

    for (i = 0; i < bitmap.bmHeight; i++)
    {
        for (j = 0; j < bitmap.bmWidth; j++)
        {
            DWORD Val = 0;
            INT b, g, r;

            CopyMemory(&Val,
                       &pBits[Count],
                       4);

            b = 255 - GetRValue(Val);
            g = 255 - GetGValue(Val);
            r = 255 - GetBValue(Val);
            
            Val = RGB(b, g, r);

            CopyMemory(&pBits[Count], 
                       &Val, 
                       4);

            Count+=4;
        }
    }

    /* Set the new pixel bits */
    SetDIBits(hdcMem,
              hBitmap,
              0,
              bRes,
              pBits,
              &bi,
              DIB_RGB_COLORS);

    HeapFree(ProcessHeap,
             0,
             pBits);

    GetClientRect(hwnd,
                  &rc);

    InvalidateRect(hwnd,
                   &rc,
                   FALSE);

    return TRUE;
}
