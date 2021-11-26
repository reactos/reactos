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
              sizeof(bitmap),
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

            /* get the average color value */
            Val = (r + g + b) / 3;

            /* assign to RGB color */
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
              sizeof(bitmap),
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



BOOL
DisplayBlur(HWND hwnd,
            HDC hdcMem,
            HBITMAP hBitmap)
{
    BITMAPINFO bi;
    BITMAP bitmap;
    BOOL bRes;
    DWORD Count = 0;
    INT i, j;
    PBYTE pBits, pBitsTemp;
    RECT rc;

    GetObject(hBitmap,
              sizeof(bitmap),
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
    pBitsTemp = (PBYTE)HeapAlloc(ProcessHeap,
                                 0,
                                 bitmap.bmWidth * bitmap.bmHeight * 4);
    if (!pBits || !pBitsTemp)
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
            LONG Val = 0;
            INT b, g, r;
            INT c1, c2, c3, c4, c5;

            CopyMemory(&Val,
                       &pBits[Count],
                       4);

            b = GetRValue(Val);
            g = GetGValue(Val);
            r = GetBValue(Val);

            c1 = r;
            /*  Red */
            if ((Count < ((bitmap.bmHeight - 1) * bitmap.bmWidth * 4lu)) &&
                (Count > (bitmap.bmWidth * 4lu)))
            {
                CopyMemory(&Val, &pBits[Count - (bitmap.bmWidth * 4)], 4);
                c2 = GetBValue(Val);

                CopyMemory(&Val, &pBits[Count + 4], 4);
                c3 = GetBValue(Val);

                CopyMemory(&Val, &pBits[(Count + (bitmap.bmWidth * 4))], 4);
                c4 = GetBValue(Val);

                CopyMemory(&Val, &pBits[Count - 4], 4);
                c5 = GetBValue(Val);

                r = (c1 + c2 + c3 + c4 + c5) / 5;
            }

            /* Green */
            c1 = g;
            if ((Count < ((bitmap.bmHeight - 1) * bitmap.bmWidth * 4lu)) &&
                (Count > (bitmap.bmWidth * 4lu)))
            {
                CopyMemory(&Val, &pBits[(Count - (bitmap.bmWidth * 4lu))], 4);
                c2 = GetGValue(Val);

                CopyMemory(&Val, &pBits[Count + 4], 4);
                c3 = GetGValue(Val);

                CopyMemory(&Val, &pBits[(Count + (bitmap.bmWidth * 4lu))], 4);
                c4 = GetGValue(Val);

                CopyMemory(&Val, &pBits[Count-4], 4);
                c5 = GetGValue(Val);

                g = (c1 + c2 + c3 + c4 + c5) / 5;
            }

            /* Blue */
            c1 = b;
            if ((Count < ((bitmap.bmHeight - 1) * bitmap.bmWidth * 4lu)) &&
                (Count > (bitmap.bmWidth * 4lu)))
            {
                CopyMemory(&Val, &pBits[(Count - (bitmap.bmWidth * 4l))], 4);
                c2 = GetRValue(Val);

                CopyMemory(&Val, &pBits[Count + 4], 4);
                c3 = GetRValue(Val);

                CopyMemory(&Val, &pBits[(Count + (bitmap.bmWidth * 4l))], 4);
                c4 = GetRValue(Val);

               CopyMemory(&Val, &pBits[Count-4], 4);
                c5 = GetRValue(Val);

                b = (c1 + c2 + c3 + c4 + c5) / 5;
            }

            Val = RGB(b, g, r);

            CopyMemory(&pBitsTemp[Count],
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
              pBitsTemp,
              &bi,
              DIB_RGB_COLORS);

    HeapFree(ProcessHeap,
             0,
             pBits);
    HeapFree(ProcessHeap,
             0,
             pBitsTemp);

    GetClientRect(hwnd,
                  &rc);

    InvalidateRect(hwnd,
                   &rc,
                   FALSE);

    return TRUE;
}



BOOL
DisplaySharpness(HWND hwnd,
                 HDC hdcMem,
                 HBITMAP hBitmap)
{
    BITMAPINFO bi;
    BITMAP bitmap;
    BOOL bRes;
    DWORD Count = 0;
    INT i, j;
    PBYTE pBits, pBitsTemp;
    RECT rc;

    GetObject(hBitmap,
              sizeof(bitmap),
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
    pBitsTemp = (PBYTE)HeapAlloc(ProcessHeap,
                                 0,
                                 bitmap.bmWidth * bitmap.bmHeight * 4);
    if (!pBits || !pBitsTemp)
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
            LONG Val = 0;
            INT b, g, r;
            INT c1, c2, c3, c4, c5;

            CopyMemory(&Val,
                       &pBits[Count],
                       4);

            b = GetRValue(Val);
            g = GetGValue(Val);
            r = GetBValue(Val);

            c1 = r;
            /* Red */
            if ((Count < ((bitmap.bmHeight - 1) * bitmap.bmWidth * 4lu)) &&
                (Count > (bitmap.bmWidth * 4lu)))
            {
                CopyMemory(&Val, &pBits[Count - (bitmap.bmWidth * 4l)], 4);
                c2 = GetBValue(Val);

                CopyMemory(&Val, &pBits[Count + 4], 4);
                c3 = GetBValue(Val);

                CopyMemory(&Val, &pBits[(Count + (bitmap.bmWidth * 4l))], 4);
                c4 = GetBValue(Val);

                CopyMemory(&Val, &pBits[Count - 4], 4);
                c5 = GetBValue(Val);

                r = (c1 * 5) - (c2 + c3 + c4 + c5);
            }

            /* Green */
            c1 = g;
            if ((Count < ((bitmap.bmHeight - 1)* bitmap.bmWidth * 4lu)) &&
                (Count > (bitmap.bmWidth * 4lu)))
            {
                CopyMemory(&Val, &pBits[(Count - (bitmap.bmWidth * 4l))], 4);
                c2 = GetGValue(Val);

                CopyMemory(&Val, &pBits[Count + 4], 4);
                c3 = GetGValue(Val);

                CopyMemory(&Val, &pBits[(Count + (bitmap.bmWidth * 4l))], 4);
                c4 = GetGValue(Val);

                CopyMemory(&Val, &pBits[Count - 4], 4);
                c5 = GetGValue(Val);

                g = (c1 * 5) - (c2 + c3 + c4 + c5);
            }

            /* Blue */
            c1 = b;
            if ((Count < ((bitmap.bmHeight - 1) * bitmap.bmWidth * 4lu)) &&
                (Count > (bitmap.bmWidth * 4lu)))
            {
                CopyMemory(&Val, &pBits[(Count - (bitmap.bmWidth * 4l))], 4);
                c2 = GetRValue(Val);

                CopyMemory(&Val, &pBits[Count + 4], 4);
                c3 = GetRValue(Val);

                CopyMemory(&Val, &pBits[(Count+(bitmap.bmWidth * 4l))], 4);
                c4 = GetRValue(Val);

                CopyMemory(&Val, &pBits[Count - 4], 4);
                c5 = GetRValue(Val);

                b = (c1 * 5) - (c2 + c3 + c4 + c5);
            }

            /* Red */
            if (r > 255) r = 255;
            if (r < 0) r = 0;

            /* Green */
            if (g > 255) g = 255;
            if (g < 0)g = 0;

            /* Blue */
            if (b > 255) b = 255;
            if (b < 0) b = 0;

            Val = RGB(b, g, r);

            CopyMemory(&pBitsTemp[Count],
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
              pBitsTemp,
              &bi,
              DIB_RGB_COLORS);

    HeapFree(ProcessHeap,
             0,
             pBits);
    HeapFree(ProcessHeap,
             0,
             pBitsTemp);

    GetClientRect(hwnd,
                  &rc);

    InvalidateRect(hwnd,
                   &rc,
                   FALSE);

    return TRUE;
}
