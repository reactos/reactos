#ifndef __WIN32K_BRUSH_H
#define __WIN32K_BRUSH_H

#include <win32k/gdiobj.h>

/* Internal interface */

#define NB_HATCH_STYLES  6

/*
 * The layout of this structure is taken from "Windows Graphics Programming"
 * book written by Feng Yuan.
 *
 * DON'T MODIFY THIS STRUCTURE UNLESS REALLY NEEDED AND EVEN THEN ASK ON
 * A MAILING LIST FIRST.
 */

typedef struct
{
   ULONG AttrFlags;
   COLORREF lbColor;
} BRUSHATTR, *PBRUSHATTR;

typedef struct
{
   ULONG ulStyle;
   HBITMAP hbmPattern;
   HANDLE hbmClient;
   ULONG flAttrs;

   ULONG ulBrushUnique;
   BRUSHATTR *pBrushAttr;
   BRUSHATTR BrushAttr;
   ULONG Unknown30;
   ULONG bCacheGrabbed;
   COLORREF crBack;
   COLORREF crFore;
   ULONG ulPalTime;
#if 0
   ULONG ulSurfTime;
   PVOID ulRealization;
   ULONG Unknown4C[3];
#else
   BRUSHOBJ BrushObject;
   ULONG Unknown50[2];
#endif
   POINT ptPenWidth;
   ULONG ulPenStyle;
   DWORD *pStyle;
   ULONG dwStyleCount;
   ULONG Unknown6C;
} GDIBRUSHOBJ, *PGDIBRUSHOBJ;

/* GDI Brush Attributes */

#define GDIBRUSH_NEED_BK_CLR		0x0002 /* Background color is needed */
#define GDIBRUSH_DITHER_OK		0x0004 /* Allow color dithering */
#define GDIBRUSH_IS_SOLID		0x0010 /* Solid brush */
#define GDIBRUSH_IS_HATCH		0x0020 /* Hatch brush */
#define GDIBRUSH_IS_BITMAP		0x0040 /* DDB pattern brush */
#define GDIBRUSH_IS_DIB			0x0080 /* DIB pattern brush */ 
#define GDIBRUSH_IS_NULL		0x0100 /* Null/hollow brush */
#define GDIBRUSH_IS_GLOBAL		0x0200 /* Stock objects */
#define GDIBRUSH_IS_PEN			0x0400 /* Pen */
#define GDIBRUSH_IS_OLDSTYLEPEN		0x0800 /* Geometric pen */
#define GDIBRUSH_IS_MASKING		0x8000 /* Pattern bitmap is used as transparent mask (?) */
#define GDIBRUSH_CACHED_IS_SOLID	0x80000000 

#define  BRUSHOBJ_AllocBrush() ((HBRUSH) GDIOBJ_AllocObj (sizeof(GDIBRUSHOBJ), GDI_OBJECT_TYPE_BRUSH, NULL))
#define  BRUSHOBJ_FreeBrush(hBrush) GDIOBJ_FreeObj((HGDIOBJ)hBrush, GDI_OBJECT_TYPE_BRUSH, GDIOBJFLAG_DEFAULT)
#define  BRUSHOBJ_LockBrush(hBrush) ((PGDIBRUSHOBJ)GDIOBJ_LockObj((HGDIOBJ)hBrush, GDI_OBJECT_TYPE_BRUSH))
#define  BRUSHOBJ_UnlockBrush(hBrush) GDIOBJ_UnlockObj((HGDIOBJ)hBrush, GDI_OBJECT_TYPE_BRUSH)

HBRUSH STDCALL
NtGdiCreateBrushIndirect(
   CONST LOGBRUSH *LogBrush);

HBRUSH STDCALL
NtGdiCreateDIBPatternBrush(
   HGLOBAL hDIBPacked,
   UINT ColorSpec);

HBRUSH STDCALL
NtGdiCreateDIBPatternBrushPt(
   CONST VOID *PackedDIB,
   UINT Usage);

HBRUSH STDCALL
NtGdiCreateHatchBrush(
   INT Style,
   COLORREF Color);

HBRUSH STDCALL
NtGdiCreatePatternBrush(
   HBITMAP hBitmap);

HBRUSH STDCALL
NtGdiCreateSolidBrush(
   COLORREF Color);

BOOL STDCALL
NtGdiFixBrushOrgEx(
   VOID);

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

BOOL STDCALL
NtGdiSetBrushOrgEx(
   HDC hDC,
   INT XOrg,
   INT YOrg,
   LPPOINT Point);

#endif
