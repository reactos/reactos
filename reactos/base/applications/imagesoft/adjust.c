#include "precomp.h"


VOID
AdjustBrightness(PIMAGEADJUST pImgAdj,
                 HDC hdcMem)
{
    BITMAPINFO bi;
    BITMAP bitmap;
    BOOL bRes;
    DWORD Count = 0;
    INT i, j;
    PBYTE pBits;

    /* Bitmap header */
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = pImgAdj->ImageRect.right;
    bi.bmiHeader.biHeight = pImgAdj->ImageRect.bottom;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = pImgAdj->ImageRect.right * 4 * pImgAdj->ImageRect.bottom;
    bi.bmiHeader.biClrUsed = 0;
    bi.bmiHeader.biClrImportant = 0;

    /* Buffer */
    pBits = (PBYTE)HeapAlloc(ProcessHeap,
                             0,
                             pImgAdj->ImageRect.right * 4 * pImgAdj->ImageRect.bottom);

    bRes = GetDIBits(hdcMem,
                     pImgAdj->hBitmap,
                     0,
                     pImgAdj->ImageRect.bottom,
                     pBits,
                     &bi,
                     DIB_RGB_COLORS);

    GetObject(pImgAdj->hBitmap,
              sizeof(BITMAP),
              &bitmap);

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

            /* Red */
            r += pImgAdj->RedVal;
            if (r > 255) r = 255;
            else if (r < 0) r = 0;

            /* Green */
            g += pImgAdj->GreenVal;
            if (g > 255) g = 255;
            else if (g < 0) g = 0;

            /* Blue */
            b += pImgAdj->BlueVal;
            if (b > 255) b = 255;
            else if (b < 0) b = 0;

            /* Store in reverse order */
            Val = RGB(b, g, r);
            CopyMemory(&pBits[Count],
                       &Val,
                       4);

            /* RGB color take 4 bytes.The high-order byte must be zero */
            Count += 4;
        }
    }

    /* Set the new pixel bits */
    SetDIBits(hdcMem,
              pImgAdj->hBitmap,
              0,
              bRes,
              pBits,
              &bi,
              DIB_RGB_COLORS);

    HeapFree(ProcessHeap,
             0,
             pBits);

    InvalidateRect(pImgAdj->hPicPrev,
                   &pImgAdj->ImageRect,
                   FALSE);
}
