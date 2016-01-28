#pragma once

INT FASTCALL DIB_BitmapInfoSize (const BITMAPINFO * info, WORD coloruse);
HBITMAP APIENTRY DIB_CreateDIBSection (PDC dc, CONST BITMAPINFO *bmi, UINT usage, LPVOID *bits, HANDLE section, DWORD offset, DWORD ovr_pitch);
int FASTCALL DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, LONG *width,
                       LONG *height, WORD *planes, WORD *bpp, DWORD *compr, DWORD *size );
INT APIENTRY DIB_GetDIBImageBytes (INT  width, INT height, INT depth);
HPALETTE FASTCALL DIB_MapPaletteColors(PPALETTE ppal, CONST BITMAPINFO* lpbmi);
HPALETTE FASTCALL BuildDIBPalette (CONST BITMAPINFO *bmi);

/* Those functions permit to tranparently work with a BITMAPCOREINFO structure */
BITMAPINFO* FASTCALL DIB_ConvertBitmapInfo(CONST BITMAPINFO* bmi, DWORD Usage);
/* Pass Usage = -1 if you don't want to convert the BITMAPINFO back to BITMAPCOREINFO */
VOID FASTCALL DIB_FreeConvertedBitmapInfo(BITMAPINFO* converted, BITMAPINFO* orig, DWORD Usage);

INT
APIENTRY
GreGetDIBitsInternal(
    HDC hDC,
    HBITMAP hBitmap,
    UINT StartScan,
    UINT ScanLines,
    LPBYTE Bits,
    LPBITMAPINFO Info,
    UINT Usage,
    UINT MaxBits,
    UINT MaxInfo);

HBITMAP
NTAPI
GreCreateDIBitmapFromPackedDIB(
    _In_reads_(cjPackedDIB )PVOID pvPackedDIB,
    _In_ UINT cjPackedDIB,
    _In_ ULONG uUsage);

#define DIB_PAL_BRUSHHACK 3
