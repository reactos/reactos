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
W32kCreateBrushIndirect (
	CONST LOGBRUSH	* lb
	);
HBRUSH
STDCALL
W32kCreateDIBPatternBrush (
	HGLOBAL	hDIBPacked,
	UINT	ColorSpec
	);
HBRUSH
STDCALL
W32kCreateDIBPatternBrushPt (
	CONST VOID	* PackedDIB,
	UINT		Usage
	);
HBRUSH
STDCALL
W32kCreateHatchBrush (
	INT		Style,
	COLORREF	Color
	);
HBRUSH
STDCALL
W32kCreatePatternBrush (
	HBITMAP	hBitmap
	);
HBRUSH
STDCALL
W32kCreateSolidBrush (
	COLORREF	Color
	);
BOOL
STDCALL
W32kFixBrushOrgEx (
	VOID
	);
BOOL
STDCALL
W32kPatBlt (
	HDC	hDC,
	INT	XLeft,
	INT	YLeft,
	INT	Width,
	INT	Height,
	DWORD	ROP
	);
BOOL
STDCALL
W32kSetBrushOrgEx (
	HDC	hDC,
	INT	XOrg,
	INT	YOrg,
	LPPOINT	Point
	);
#endif

