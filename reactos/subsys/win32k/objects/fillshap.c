
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <win32k/fillshap.h>

// #define NDEBUG
#include <internal/debug.h>

BOOL  W32kChord(HDC  hDC,
                int  LeftRect,
                int  TopRect,
                int  RightRect,
                int  BottomRect,
                int  XRadial1,
                int  YRadial1,
                int  XRadial2,
                int  YRadial2)
{
  UNIMPLEMENTED;
}

BOOL  W32kEllipse(HDC  hDC,
                  int  LeftRect,
                  int  TopRect,
                  int  RightRect,
                  int  BottomRect)
{
  UNIMPLEMENTED;
}

BOOL  W32kPie(HDC  hDC,
              int  LeftRect,
              int  TopRect,
              int  RightRect,
              int  BottomRect,
              int  XRadial1,
              int  YRadial1,
              int  XRadial2,
              int  YRadial2)
{
  UNIMPLEMENTED;
}

BOOL  W32kPolygon(HDC  hDC,
                  CONST PPOINT  Points,
                  int  Count)
{
  UNIMPLEMENTED;
}

BOOL  W32kPolyPolygon(HDC  hDC,
                      CONST LPPOINT  Points,
                      CONST LPINT  PolyCounts,
                      int  Count)
{
  UNIMPLEMENTED;
}

BOOL  W32kRectangle(HDC  hDC,
                    int  LeftRect,
                    int  TopRect,
                    int  RightRect,
                    int  BottomRect)
{
  UNIMPLEMENTED;
}

BOOL  W32kRoundRect(HDC  hDC,
                    int  LeftRect,
                    int  TopRect,  
                    int  RightRect, 
                    int  BottomRect,
                    int  Width,
                    int  Height)
{
  UNIMPLEMENTED;
}


