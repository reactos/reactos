
#ifndef __WIN32K_PAINT_H
#define __WIN32K_PAINT_H

BOOL  W32kGdiFlush(VOID);

DWORD  W32kGdiGetBatchLimit(VOID);

DWORD  W32kGdiSetBatchLimit(DWORD  Limit);

UINT  W32kGetBoundsRect(HDC  hDC,
                        LPRECT  Bounds,
                        UINT  Flags);

COLORREF  W32kSetBkColor(HDC  hDC,
                         COLORREF  Color);

UINT  W32kSetBoundsRect(HDC  hDC,
                        CONST PRECT  Bounds,
                        UINT  Flags);

#endif

