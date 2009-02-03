#ifndef __WIN32K_BITMAPS_H
#define __WIN32K_BITMAPS_H

#include "surface.h"

INT     FASTCALL DIB_GetDIBWidthBytes (INT  width, INT  depth);
int     APIENTRY  DIB_GetDIBImageBytes (INT  width, INT  height, INT  depth);
INT     FASTCALL DIB_BitmapInfoSize (const BITMAPINFO * info, WORD coloruse);
INT     APIENTRY  BITMAP_GetObject(SURFACE * bmp, INT count, LPVOID buffer);
HBITMAP FASTCALL IntCreateBitmap(IN SIZEL Size, IN LONG Width, IN ULONG Format, IN ULONG Flags, IN PVOID Bits);
HBITMAP FASTCALL BITMAP_CopyBitmap (HBITMAP  hBitmap);
UINT    FASTCALL BITMAP_GetRealBitsPixel(UINT nBitsPixel);
INT     FASTCALL BITMAP_GetWidthBytes (INT bmWidth, INT bpp);

#endif
