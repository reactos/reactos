

#ifndef _WIN32K_CLIPRGN_H
#define _WIN32K_CLIPRGN_H

int  W32kExcludeClipRect(HDC  hDC,
                         int  LeftRect,
                         int  TopRect,
                         int  RightRect,
                         int  BottomRect);
                                    
int  W32kExtSelectClipRgn(HDC  hDC,
                          HRGN  hrgn,
                          int  fnMode);

int  W32kGetClipBox(HDC  hDC,
                    LPRECT  rc);

int  W32kGetMetaRgn(HDC  hDC,
                    HRGN  hrgn);

int  W32kIntersectClipRect(HDC  hDC,
                           int  LeftRect,
                           int  TopRect,
                           int  RightRect,
                           int  BottomRect);

int  W32kOffsetClipRgn(HDC  hDC,
                       int  XOffset,
                       int  YOffset);

BOOL  W32kPtVisible(HDC  hDC,
                    int  X,
                    int  Y);

BOOL  W32kRectVisible(HDC  hDC,
                      CONST PRECT  rc);

BOOL  W32kSelectClipPath(HDC  hDC,
                         int  Mode);

int  W32kSelectClipRgn(HDC  hDC,
                       HRGN  hrgn);

int  W32kSetMetaRgn(HDC  hDC);

#endif
