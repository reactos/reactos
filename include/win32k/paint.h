#ifndef __WIN32K_PAINT_H
#define __WIN32K_PAINT_H

BOOL STDCALL NtGdiGdiFlush (VOID);
DWORD STDCALL NtGdiGdiGetBatchLimit (VOID);
DWORD STDCALL NtGdiGdiSetBatchLimit (DWORD  Limit);
UINT STDCALL NtGdiGetBoundsRect (HDC hDC, LPRECT Bounds, UINT Flags);
COLORREF STDCALL NtGdiSetBkColor (HDC hDC, COLORREF Color);
UINT STDCALL NtGdiSetBoundsRect (HDC hDC, CONST PRECT Bounds, UINT Flags);

#endif

