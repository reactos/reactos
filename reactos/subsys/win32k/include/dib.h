#ifndef _WIN32K_DIB_H
#define _WIN32K_DIB_H

#include <win32k/dc.h>

INT INTERNAL_CALL
DIB_BitmapInfoSize (const BITMAPINFO * info, WORD coloruse);
HBITMAP INTERNAL_CALL
DIB_CreateDIBSection (PDC dc, PBITMAPINFO bmi, UINT usage, LPVOID *bits, HANDLE section, DWORD offset, DWORD ovr_pitch);
INT INTERNAL_CALL
DIB_GetBitmapInfo (const BITMAPINFOHEADER *header, PDWORD width, PINT height, PWORD bpp, PWORD compr);
INT INTERNAL_CALL
DIB_GetDIBImageBytes (INT  width, INT height, INT depth);
INT INTERNAL_CALL
DIB_GetDIBWidthBytes (INT width, INT depth);
RGBQUAD * INTERNAL_CALL
DIB_MapPaletteColors(PDC dc, CONST BITMAPINFO* lpbmi);

HPALETTE INTERNAL_CALL
BuildDIBPalette (PBITMAPINFO bmi, PINT paletteType);

#endif /* _WIN32K_DIB_H */
