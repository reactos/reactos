#ifndef __WIN32K_DIB_H
#define __WIN32K_DIB_H

#include <win32k/dc.h>

INT     FASTCALL DIB_BitmapInfoSize (const BITMAPINFO * info, WORD coloruse);
HBITMAP STDCALL  DIB_CreateDIBSection (PDC dc, PBITMAPINFO bmi, UINT usage, LPVOID *bits, HANDLE section, DWORD offset, DWORD ovr_pitch);
INT     STDCALL  DIB_GetBitmapInfo (const BITMAPINFOHEADER *header, PDWORD width, PINT height, PWORD bpp, PWORD compr);
INT     STDCALL  DIB_GetDIBImageBytes (INT  width, INT height, INT depth);
INT     FASTCALL DIB_GetDIBWidthBytes (INT width, INT depth);
RGBQUAD * FASTCALL DIB_MapPaletteColors (PDC dc, LPBITMAPINFO lpbmi);

PPALETTEENTRY STDCALL DIBColorTableToPaletteEntries(PPALETTEENTRY palEntries, const RGBQUAD *DIBColorTable, ULONG ColorCount);
HPALETTE FASTCALL BuildDIBPalette (PBITMAPINFO bmi, PINT paletteType);
	
#endif /* __WIN32K_DIB_H */
