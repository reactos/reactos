
#ifndef __WIN32K_BRUSH_H
#define __WIN32K_BRUSH_H

#include <win32k/gdiobj.h>

/*  Internal interface  */

#define NB_HATCH_STYLES  6

#define  BRUSHOBJ_AllocBrush()  \
  ((PBRUSHOBJ) GDIOBJ_AllocObject (sizeof (BRUSHOBJ), GO_BRUSH_MAGIC))
#define  BRUSHOBJ_FreeBrush(hBrush)  GDIOBJ_FreeObject((HGDIOBJ)hBrush)
#define  BRUSHOBJ_HandleToPtr(hBrush)  \
  ((PBRUSHOBJ) GDIOBJ_HandleToPtr ((HGDIOBJ) hBrush, GO_BRUSH_MAGIC))
#define  BRUSHOBJ_PtrToHandle(pBrushObj)  \
  ((HBRUSH) GDIOBJ_PtrToHandle ((PGDIOBJ) pBrushObj, GO_BRUSH_MAGIC))
#define  BRUSHOBJ_LockBrush(hBrush) GDIOBJ_LockObject((HGDIOBJ)hBrush)
#define  BRUSHOBJ_UnlockBrush(hBrush) GDIOBJ_UnlockObject((HGDIOBJ)hBrush)

HBRUSH  W32kCreateBrushIndirect(CONST LOGBRUSH  *lb);
HBRUSH  W32kCreateDIBPatternBrush(HGLOBAL  hDIBPacked,
                                  UINT  ColorSpec);
HBRUSH  W32kCreateDIBPatternBrushPt(CONST VOID  *PackedDIB,
                                    UINT  Usage);
HBRUSH  W32kCreateHatchBrush(INT  Style,
                             COLORREF  Color);
HBRUSH  W32kCreatePatternBrush(HBITMAP  hBitmap);
HBRUSH  W32kCreateSolidBrush(COLORREF  Color);
BOOL  W32kFixBrushOrgEx(VOID);
BOOL  W32kPatBlt(HDC  hDC,
                 INT  XLeft,
                 INT  YLeft,
                 INT  Width,
                 INT  Height,
                 DWORD  ROP);
BOOL  W32kSetBrushOrgEx(HDC  hDC,
                        INT  XOrg,
                        INT  YOrg,
                        LPPOINT  Point);

#endif

