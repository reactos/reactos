#ifndef __WIN32K_BRUSH_H
#define __WIN32K_BRUSH_H

#include <win32k/gdiobj.h>

/*  Internal interface  */

#define NB_HATCH_STYLES  6

#define  BRUSHOBJ_AllocBrush()  \
  ((HBRUSH) GDIOBJ_AllocObj (sizeof (BRUSHOBJ), GO_BRUSH_MAGIC))
#define  BRUSHOBJ_FreeBrush(hBrush)  GDIOBJ_FreeObj((HGDIOBJ)hBrush, GO_BRUSH_MAGIC, GDIOBJFLAG_DEFAULT)
/*#define  BRUSHOBJ_HandleToPtr(hBrush)  \
  ((PBRUSHOBJ) GDIOBJ_HandleToPtr ((HGDIOBJ) hBrush, GO_BRUSH_MAGIC))
#define  BRUSHOBJ_PtrToHandle(pBrushObj)  \
  ((HBRUSH) GDIOBJ_PtrToHandle ((PGDIOBJ) pBrushObj, GO_BRUSH_MAGIC))
*/
#define  BRUSHOBJ_LockBrush(hBrush) ((PBRUSHOBJ)GDIOBJ_LockObj((HGDIOBJ)hBrush, GO_BRUSH_MAGIC))
#define  BRUSHOBJ_UnlockBrush(hBrush) GDIOBJ_UnlockObj((HGDIOBJ)hBrush, GO_BRUSH_MAGIC)

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

