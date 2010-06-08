#pragma once

#include "surface.h"

typedef struct tagBITMAPV5INFO
{
    BITMAPV5HEADER bmiHeader;
    RGBQUAD        bmiColors[256];
} BITMAPV5INFO, *PBITMAPV5INFO;

INT     APIENTRY  BITMAP_GetObject(SURFACE * bmp, INT count, LPVOID buffer);
HBITMAP FASTCALL IntCreateBitmap(IN SIZEL Size, IN LONG Width, IN ULONG Format, IN ULONG Flags, IN PVOID Bits);
HBITMAP FASTCALL BITMAP_CopyBitmap (HBITMAP  hBitmap);
UINT    FASTCALL BITMAP_GetRealBitsPixel(UINT nBitsPixel);
INT     FASTCALL BITMAP_GetWidthBytes (INT bmWidth, INT bpp);
NTSTATUS FASTCALL ProbeAndConvertToBitmapV5Info( OUT PBITMAPV5INFO pbmiDst, IN CONST BITMAPINFO* pbmiUnsafe, IN DWORD dwUse);
VOID FASTCALL GetBMIFromBitmapV5Info(IN PBITMAPV5INFO pbmiSrc, OUT PBITMAPINFO pbmiDst, IN DWORD dwUse);

HBITMAP
APIENTRY
GreCreateBitmap(
    IN INT nWidth,
    IN INT nHeight,
    IN UINT cPlanes,
    IN UINT cBitsPixel,
    IN OPTIONAL PVOID pvBits);

HBITMAP
APIENTRY
GreCreateBitmapEx(
    IN INT nWidth,
    IN INT nHeight,
    IN ULONG cjWidthBytes,
    IN ULONG iFormat,
    IN USHORT fjBitmap,
    IN ULONG cjBits,
    IN OPTIONAL PVOID pvBits);

