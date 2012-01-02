#pragma once

INT     APIENTRY  BITMAP_GetObject(SURFACE * bmp, INT count, LPVOID buffer);
HBITMAP FASTCALL BITMAP_CopyBitmap (HBITMAP  hBitmap);

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
    IN OPTIONAL PVOID pvBits,
	IN FLONG flags);

HBITMAP
FASTCALL
GreCreateDIBitmapInternal(
    IN HDC hDc,
    IN INT cx,
    IN INT cy,
    IN DWORD fInit,
    IN OPTIONAL LPBYTE pjInit,
    IN OPTIONAL PBITMAPINFO pbmi,
    IN DWORD iUsage,
    IN FLONG fl,
    IN HANDLE hcmXform);
