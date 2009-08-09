#ifndef _WIN32K_RECT_H
#define _WIN32K_RECT_H

BOOL APIENTRY
NtGdiUnionRect(PRECT Dest, const RECT* Src1, const RECT* Src2);
BOOL APIENTRY
NtGdiSetRect(PRECT Rect, int left, int top, int right, int bottom);
BOOL APIENTRY
NtGdiSetEmptyRect(PRECT Rect);
BOOL APIENTRY
NtGdiIsEmptyRect(const RECT* Rect);
BOOL APIENTRY
NtGdiIntersectRect(PRECT Dest, const RECT* Src1, const RECT* Src2);
BOOL APIENTRY
NtGdiOffsetRect(LPRECT Rect, int x, int y);

#endif /* _WIN32K_RECT_H */
