#ifndef __WIN32K_FILLSHAP_H
#define __WIN32K_FILLSHAP_H

BOOL
STDCALL
NtGdiChord(HDC  hDC,
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
NtGdiEllipse(HDC  hDC,
                  int  LeftRect,
                  int  TopRect,
                  int  RightRect,
                  int  BottomRect);

BOOL
STDCALL
NtGdiPie(HDC  hDC,
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
NtGdiPolygon(HDC  hDC,
                  CONST PPOINT  Points,
                  int  Count);

BOOL
STDCALL
NtGdiPolyPolygon(HDC  hDC,
                      CONST LPPOINT  Points,
                      CONST LPINT  PolyCounts,
                      int  Count);

BOOL
STDCALL
NtGdiRectangle(HDC  hDC,
                    int  LeftRect,
                    int  TopRect,
                    int  RightRect,
                    int  BottomRect);

BOOL
STDCALL
NtGdiRoundRect(HDC  hDC,
                    int  LeftRect,
                    int  TopRect,  
                    int  RightRect, 
                    int  BottomRect,
                    int  Width,
                    int  Height);

#endif

