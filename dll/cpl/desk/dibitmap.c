/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            dll/cpl/desk/dibitmap.c
 * PURPOSE:         DIB loading
 *
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 */

#include "desk.h"

PDIBITMAP
DibLoadImage(LPTSTR lpFilename)
{
    PDIBITMAP lpBitmap;
    GpBitmap  *bitmap;
    BitmapData lock;

    if (GdipCreateBitmapFromFile(lpFilename, &bitmap) != Ok)
    {
        return NULL;
    }

    lpBitmap = HeapAlloc(GetProcessHeap(), 0, sizeof(DIBITMAP));
    if (lpBitmap == NULL)
    {
        GdipDisposeImage((GpImage*)bitmap);
        return NULL;
    }

    lpBitmap->info = HeapAlloc(GetProcessHeap(), 0, sizeof(BITMAPINFO));
    if (lpBitmap->info == NULL)
    {
        HeapFree(GetProcessHeap(), 0, lpBitmap);
        GdipDisposeImage((GpImage*)bitmap);
        return NULL;
    }

    if (GdipGetImageWidth((GpImage*)bitmap, &lpBitmap->width) != Ok ||
        GdipGetImageHeight((GpImage*)bitmap, &lpBitmap->height) != Ok)
    {
        HeapFree(GetProcessHeap(), 0, lpBitmap->info);
        HeapFree(GetProcessHeap(), 0, lpBitmap);
        GdipDisposeImage((GpImage*)bitmap);
        return NULL;
    }

    lpBitmap->bits = HeapAlloc(GetProcessHeap(), 0, lpBitmap->width * lpBitmap->height * 4);
    if (!lpBitmap->bits)
    {
        HeapFree(GetProcessHeap(), 0, lpBitmap->info);
        HeapFree(GetProcessHeap(), 0, lpBitmap);
        GdipDisposeImage((GpImage*)bitmap);
        return NULL;
    }

    ZeroMemory(lpBitmap->info, sizeof(BITMAPINFO));
    lpBitmap->info->bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    lpBitmap->info->bmiHeader.biWidth       = lpBitmap->width;
    lpBitmap->info->bmiHeader.biHeight      = -(INT)lpBitmap->height;
    lpBitmap->info->bmiHeader.biPlanes      = 1;
    lpBitmap->info->bmiHeader.biBitCount    = 32;
    lpBitmap->info->bmiHeader.biCompression = BI_RGB;
    lpBitmap->info->bmiHeader.biSizeImage   = lpBitmap->width * lpBitmap->height * 4;

    lock.Width = lpBitmap->width;
    lock.Height = lpBitmap->height;
    lock.Stride = lpBitmap->width * 4;
    lock.PixelFormat = PixelFormat32bppPARGB;
    lock.Scan0  = lpBitmap->bits;
    lock.Reserved = 0;

    if (GdipBitmapLockBits(bitmap, NULL, ImageLockModeRead | ImageLockModeUserInputBuf, PixelFormat32bppPARGB, &lock) != Ok)
    {
        HeapFree(GetProcessHeap(), 0, lpBitmap->bits);
        HeapFree(GetProcessHeap(), 0, lpBitmap->info);
        HeapFree(GetProcessHeap(), 0, lpBitmap);
        GdipDisposeImage((GpImage*)bitmap);
        return NULL;
    }

    GdipBitmapUnlockBits(bitmap, &lock);
    GdipDisposeImage((GpImage*)bitmap);

    return lpBitmap;
}


VOID
DibFreeImage(PDIBITMAP lpBitmap)
{
    if (lpBitmap == NULL)
        return;

    /* Free the image data */
    if (lpBitmap->bits != NULL)
        HeapFree(GetProcessHeap(), 0, lpBitmap->bits);

    /* Free the bitmap info */
    if (lpBitmap->info != NULL)
        HeapFree(GetProcessHeap(), 0, lpBitmap->info);

    /* Free the bitmap structure */
    if (lpBitmap != NULL)
        HeapFree(GetProcessHeap(), 0, lpBitmap);
}
