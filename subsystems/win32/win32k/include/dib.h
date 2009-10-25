#ifndef _WIN32K_DIB_H
#define _WIN32K_DIB_H

#include "dc.h"

INT FASTCALL
DIB_BitmapInfoSize (const BITMAPINFO * info, WORD coloruse);
HBITMAP APIENTRY
DIB_CreateDIBSection (PDC dc, PBITMAPINFO bmi, UINT usage, LPVOID *bits, HANDLE section, DWORD offset, DWORD ovr_pitch);
INT APIENTRY
DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, PLONG width, PLONG height, PWORD planes, PWORD bpp, PLONG compr, PLONG size );
INT APIENTRY
DIB_GetDIBImageBytes (INT  width, INT height, INT depth);
INT FASTCALL
DIB_GetDIBWidthBytes (INT width, INT depth);
RGBQUAD * FASTCALL
DIB_MapPaletteColors(PDC dc, CONST BITMAPINFO* lpbmi);

HPALETTE FASTCALL
BuildDIBPalette (CONST BITMAPINFO *bmi, PINT paletteType);

#endif /* _WIN32K_DIB_H */
