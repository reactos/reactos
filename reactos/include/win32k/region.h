
#ifndef __WIN32K_REGION_H
#define __WIN32K_REGION_H

INT  W32kCombineRgn(HRGN  hDest,
                    HRGN  hSrc1,
                    HRGN  hSrc2,
                    INT  CombineMode);

HRGN  W32kCreateEllipticRgn(INT  LeftRect,
                            INT  TopRect,
                            INT  RightRect,
                            INT  BottomRect);

HRGN  W32kCreateEllipticRgnIndirect(CONST PRECT  rc);

HRGN  W32kCreatePolygonRgn(CONST PPOINT  pt,
                           INT  Count,
                           INT  PolyFillMode);

HRGN  W32kCreatePolyPolygonRgn(CONST PPOINT  pt,
                               CONST PINT  PolyCounts,
                               INT  Count,
                               INT  PolyFillMode);

HRGN  W32kCreateRectRgn(INT  LeftRect,
                        INT  TopRect,
                        INT  RightRect,
                        INT  BottomRect);

HRGN  W32kCreateRectRgnIndirect(CONST PRECT  rc);

HRGN  W32kCreateRoundRectRgn(INT  LeftRect,
                             INT  TopRect,
                             INT  RightRect,
                             INT  BottomRect,
                             INT  WidthEllipse,
                             INT  HeightEllipse);

BOOL  W32kEqualRgn(HRGN  hSrcRgn1,
                   HRGN  hSrcRgn2);

HRGN  W32kExtCreateRegion(CONST PXFORM  Xform,
                          DWORD  Count,
                          CONST PRGNDATA  RgnData);

BOOL  W32kFillRgn(HDC  hDC,
                  HRGN  hRgn,
                  HBRUSH  hBrush);

BOOL  W32kFrameRgn(HDC  hDC,
                   HRGN  hRgn,
                   HBRUSH  hBrush,
                   INT  Width,
                   INT  Height);

INT  W32kGetRgnBox(HRGN  hRgn,
                   LPRECT  Rect);

BOOL  W32kInvertRgn(HDC  hDC,
                    HRGN  hRgn);

INT  W32kOffsetRgn(HRGN  hRgn,
                   INT  XOffset,
                   INT  YOffset);

BOOL  W32kPaintRgn(HDC  hDC,
                   HRGN  hRgn);

BOOL  W32kPtInRegion(HRGN  hRgn,
                     INT  X,
                     INT  Y);

BOOL  W32kRectInRegion(HRGN  hRgn,
                       CONST LPRECT  rc);

BOOL  W32kSetRectRgn(HRGN  hRgn,
                     INT  LeftRect,
                     INT  TopRect,
                     INT  RightRect,
                     INT  BottomRect);

#endif
               
