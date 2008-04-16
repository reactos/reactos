#ifndef __WIN32K_CLIPRGN_H
#define __WIN32K_CLIPRGN_H

#include <include/dc.h>
#include <include/region.h>

INT FASTCALL IntGdiGetClipBox(HDC hDC, LPRECT rc);
INT STDCALL IntGdiSelectVisRgn(HDC hdc, HRGN hrgn);
INT STDCALL IntGdiExtSelectClipRgn (PDC dc, HRGN hrgn, int fnMode);


#endif /* not __WIN32K_CLIPRGN_H */
