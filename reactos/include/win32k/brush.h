#ifndef __WIN32K_BRUSH_H
#define __WIN32K_BRUSH_H

#include <win32k/gdiobj.h>

/*  Internal interface  */

#define NB_HATCH_STYLES  6

#define  BRUSHOBJ_AllocBrush()  \
  ((HBRUSH) GDIOBJ_AllocObj (sizeof (BRUSHOBJ), GDI_OBJECT_TYPE_BRUSH, NULL))
#define  BRUSHOBJ_FreeBrush(hBrush)  GDIOBJ_FreeObj((HGDIOBJ)hBrush, GDI_OBJECT_TYPE_BRUSH, GDIOBJFLAG_DEFAULT)
#define  BRUSHOBJ_LockBrush(hBrush) ((PBRUSHOBJ)GDIOBJ_LockObj((HGDIOBJ)hBrush, GDI_OBJECT_TYPE_BRUSH))
#define  BRUSHOBJ_UnlockBrush(hBrush) GDIOBJ_UnlockObj((HGDIOBJ)hBrush, GDI_OBJECT_TYPE_BRUSH)

HBRUSH
STDCALL
NtGdiCreateBrushIndirect (
	CONST LOGBRUSH	* lb
	);
HBRUSH
STDCALL
NtGdiCreateDIBPatternBrush (
	HGLOBAL	hDIBPacked,
	UINT	ColorSpec
	);
HBRUSH
STDCALL
NtGdiCreateDIBPatternBrushPt (
	CONST VOID	* PackedDIB,
	UINT		Usage
	);
HBRUSH
STDCALL
NtGdiCreateHatchBrush (
	INT		Style,
	COLORREF	Color
	);
HBRUSH
STDCALL
NtGdiCreatePatternBrush (
	HBITMAP	hBitmap
	);
HBRUSH
STDCALL
NtGdiCreateSolidBrush (
	COLORREF	Color
	);
BOOL
STDCALL
NtGdiFixBrushOrgEx (
	VOID
	);
BOOL
STDCALL
NtGdiPatBlt (
	HDC	hDC,
	INT	XLeft,
	INT	YLeft,
	INT	Width,
	INT	Height,
	DWORD	ROP
	);
BOOL
STDCALL
NtGdiPolyPatBlt(
	HDC hDC,
	DWORD dwRop,
	PPATRECT pRects,
	int cRects,
	ULONG Reserved
	);
BOOL
STDCALL
NtGdiPatBlt (
	HDC	hDC,
	INT	XLeft,
	INT	YLeft,
	INT	Width,
	INT	Height,
	DWORD	ROP
	);
BOOL
STDCALL
NtGdiSetBrushOrgEx (
	HDC	hDC,
	INT	XOrg,
	INT	YOrg,
	LPPOINT	Point
	);
#endif

