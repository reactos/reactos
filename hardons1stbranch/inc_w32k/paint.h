#ifndef __WIN32K_PAINT_H
#define __WIN32K_PAINT_H

typedef struct _PATRECT {
	RECT r;
	HBRUSH hBrush;
} PATRECT, * PPATRECT;

BOOL STDCALL
NtGdiPatBlt(
   HDC hDC,
   INT XLeft,
   INT YLeft,
   INT Width,
   INT Height,
   DWORD ROP);

BOOL STDCALL
NtGdiPolyPatBlt(
   HDC hDC,
   DWORD dwRop,
   PPATRECT pRects,
   INT cRects,
   ULONG Reserved);

BOOL STDCALL
NtGdiPatBlt(
   HDC hDC,
   INT XLeft,
   INT YLeft,
   INT Width,
   INT Height,
   DWORD ROP);

BOOL STDCALL NtGdiGdiFlush (VOID);
DWORD STDCALL NtGdiGdiGetBatchLimit (VOID);
DWORD STDCALL NtGdiGdiSetBatchLimit (DWORD  Limit);
UINT STDCALL NtGdiGetBoundsRect (HDC hDC, LPRECT Bounds, UINT Flags);
COLORREF STDCALL NtGdiSetBkColor (HDC hDC, COLORREF Color);
UINT STDCALL NtGdiSetBoundsRect (HDC hDC, CONST PRECT Bounds, UINT Flags);

#endif

