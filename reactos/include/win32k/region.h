
#ifndef __WIN32K_REGION_H
#define __WIN32K_REGION_H

#include <win32k/gdiobj.h>

/* Internal region data. Can't use RGNDATA structure because buffer is allocated statically */
typedef struct _ROSRGNDATA {
  RGNDATAHEADER rdh;
  char*          Buffer;
} ROSRGNDATA, *PROSRGNDATA, *LPROSRGNDATA;


#define  RGNDATA_FreeRgn(hRgn)  GDIOBJ_FreeObj((HGDIOBJ)hRgn, GDI_OBJECT_TYPE_REGION, GDIOBJFLAG_DEFAULT)
#define  RGNDATA_LockRgn(hRgn) ((PROSRGNDATA)GDIOBJ_LockObj((HGDIOBJ)hRgn, GDI_OBJECT_TYPE_REGION))
#define  RGNDATA_UnlockRgn(hRgn) GDIOBJ_UnlockObj((HGDIOBJ)hRgn, GDI_OBJECT_TYPE_REGION)
HRGN FASTCALL RGNDATA_AllocRgn(INT n);

BOOL FASTCALL RGNDATA_InternalDelete( PROSRGNDATA Obj );

/*  User entry points */
HRGN STDCALL
NtGdiUnionRectWithRgn(HRGN hDest, CONST PRECT Rect);

INT
STDCALL
NtGdiCombineRgn(HRGN  hDest,
                    HRGN  hSrc1,
                    HRGN  hSrc2,
                    INT  CombineMode);

HRGN
STDCALL
NtGdiCreateEllipticRgn(INT  LeftRect,
                            INT  TopRect,
                            INT  RightRect,
                            INT  BottomRect);

HRGN
STDCALL
NtGdiCreateEllipticRgnIndirect(CONST PRECT  rc);

HRGN
STDCALL
NtGdiCreatePolygonRgn(CONST PPOINT  pt,
                           INT  Count,
                           INT  PolyFillMode);

HRGN
STDCALL
NtGdiCreatePolyPolygonRgn(CONST PPOINT  pt,
                               CONST PINT  PolyCounts,
                               INT  Count,
                               INT  PolyFillMode);

HRGN
STDCALL
NtGdiCreateRectRgn(INT  LeftRect,
                        INT  TopRect,
                        INT  RightRect,
                        INT  BottomRect);

HRGN
STDCALL
NtGdiCreateRectRgnIndirect(CONST PRECT  rc);

HRGN
STDCALL
NtGdiCreateRoundRectRgn(INT  LeftRect,
                             INT  TopRect,
                             INT  RightRect,
                             INT  BottomRect,
                             INT  WidthEllipse,
                             INT  HeightEllipse);

BOOL
STDCALL
NtGdiEqualRgn(HRGN  hSrcRgn1,
                   HRGN  hSrcRgn2);

HRGN
STDCALL
NtGdiExtCreateRegion(CONST PXFORM  Xform,
                          DWORD  Count,
                          CONST PROSRGNDATA  RgnData);

BOOL
STDCALL
NtGdiFillRgn(HDC  hDC,
                  HRGN  hRgn,
                  HBRUSH  hBrush);

BOOL
STDCALL
NtGdiFrameRgn(HDC  hDC,
                   HRGN  hRgn,
                   HBRUSH  hBrush,
                   INT  Width,
                   INT  Height);

INT
STDCALL
NtGdiGetRgnBox(HRGN  hRgn,
                   LPRECT  Rect);

BOOL
STDCALL
NtGdiInvertRgn(HDC  hDC,
                    HRGN  hRgn);

INT
STDCALL
NtGdiOffsetRgn(HRGN  hRgn,
                   INT  XOffset,
                   INT  YOffset);

BOOL
STDCALL
NtGdiPaintRgn(HDC  hDC,
                   HRGN  hRgn);

BOOL
STDCALL
NtGdiPtInRegion(HRGN  hRgn,
                     INT  X,
                     INT  Y);

BOOL
STDCALL
NtGdiRectInRegion(HRGN  hRgn,
                       CONST LPRECT  rc);

INT
STDCALL
NtGdiSelectVisRgn(HDC hdc,
                     HRGN hrgn);

BOOL
STDCALL
NtGdiSetRectRgn(HRGN  hRgn,
                     INT  LeftRect,
                     INT  TopRect,
                     INT  RightRect,
                     INT  BottomRect);

DWORD
STDCALL
NtGdiGetRegionData(HRGN hrgn,
						DWORD count,
						LPRGNDATA rgndata);

HRGN STDCALL REGION_CropRgn(HRGN hDst, HRGN hSrc, const PRECT lpRect, PPOINT lpPt);

HRGN FASTCALL UnsafeIntCreateRectRgnIndirect(CONST PRECT rc);
INT FASTCALL UnsafeIntGetRgnBox(PROSRGNDATA Rgn, LPRECT pRect);
VOID FASTCALL UnsafeIntUnionRectWithRgn(PROSRGNDATA RgnDest, CONST PRECT Rect);
BOOL FASTCALL UnsafeIntRectInRegion(PROSRGNDATA Rgn, CONST LPRECT rc);
#endif

