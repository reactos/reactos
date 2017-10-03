#pragma once

INT     APIENTRY  BITMAP_GetObject(SURFACE * bmp, INT count, LPVOID buffer);
HBITMAP FASTCALL BITMAP_CopyBitmap (HBITMAP  hBitmap);

BOOL
NTAPI
GreSetBitmapOwner(
    _In_ HBITMAP hbmp,
    _In_ ULONG ulOwner);

HBITMAP
NTAPI
GreCreateBitmap(
    _In_ ULONG nWidth,
    _In_ ULONG nHeight,
    _In_ ULONG cPlanes,
    _In_ ULONG cBitsPixel,
    _In_opt_ PVOID pvBits);

HBITMAP
NTAPI
GreCreateBitmapEx(
    _In_ ULONG nWidth,
    _In_ ULONG nHeight,
    _In_ ULONG cjWidthBytes,
    _In_ ULONG iFormat,
    _In_ USHORT fjBitmap,
    _In_ ULONG cjSizeImage,
    _In_opt_ PVOID pvBits,
    _In_ FLONG flags);

HBITMAP
NTAPI
GreCreateDIBitmapInternal(
    IN HDC hDc,
    IN INT cx,
    IN INT cy,
    IN DWORD fInit,
    IN OPTIONAL LPBYTE pjInit,
    IN OPTIONAL PBITMAPINFO pbmi,
    IN DWORD iUsage,
    IN FLONG fl,
    IN UINT cjMaxBits,
    IN HANDLE hcmXform);

BOOL
NTAPI
UnsafeSetBitmapBits(
    _Inout_ PSURFACE psurf,
    _In_ ULONG cjBits,
    _In_ const VOID *pvBits);

BOOL
NTAPI
GreGetBitmapDimension(
    _In_ HBITMAP hBitmap,
    _Out_ LPSIZE psizDim);

