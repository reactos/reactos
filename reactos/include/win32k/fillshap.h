#ifndef __WIN32K_FILLSHAP_H
#define __WIN32K_FILLSHAP_H

BOOL
STDCALL
W32kChord(HDC  hDC,
                int  LeftRect,
                int  TopRect,
                int  RightRect,
                int  BottomRect,
                int  XRadial1,
                int  YRadial1,
                int  XRadial2,
                int  YRadial2);

BOOL
STDCALL
W32kEllipse(HDC  hDC,
                  int  LeftRect,
                  int  TopRect,
                  int  RightRect,
                  int  BottomRect);

BOOL
STDCALL
W32kPie(HDC  hDC,
              int  LeftRect,
              int  TopRect,
              int  RightRect,
              int  BottomRect,
              int  XRadial1,
              int  YRadial1,
              int  XRadial2,
              int  YRadial2);

BOOL
STDCALL
W32kPolygon(HDC  hDC,
                  CONST PPOINT  Points,
                  int  Count);

BOOL
STDCALL
W32kPolyPolygon(HDC  hDC,
                      CONST LPPOINT  Points,
                      CONST LPINT  PolyCounts,
                      int  Count);

BOOL
STDCALL
W32kRectangle(HDC  hDC,
                    int  LeftRect,
                    int  TopRect,
                    int  RightRect,
                    int  BottomRect);

BOOL
STDCALL
W32kRoundRect(HDC  hDC,
                    int  LeftRect,
                    int  TopRect,  
                    int  RightRect, 
                    int  BottomRect,
                    int  Width,
                    int  Height);

#endif

