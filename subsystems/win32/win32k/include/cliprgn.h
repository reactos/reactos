#pragma once

#include <include/dc.h>
#include <include/region.h>

INT FASTCALL IntGdiGetClipBox(PDC, RECTL* rc);
INT FASTCALL IntGdiExtSelectClipRgn (PDC, PROSRGNDATA, int);

INT FASTCALL GdiGetClipBox(HDC hDC, RECTL *rc);
INT FASTCALL GdiSelectVisRgn(HDC hdc, HRGN hrgn);
INT FASTCALL GdiExtSelectClipRgn (PDC dc, HRGN hrgn, int fnMode);
