
#ifndef __WIN32K_BRUSH_H
#define __WIN32K_BRUSH_H

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

