#ifndef _WIN32K_RECT_H
#define _WIN32K_RECT_H

BOOL STDCALL
NtGdiUnionRect(PRECT Dest, const RECT* Src1, const RECT* Src2);
BOOL STDCALL
NtGdiSetRect(PRECT Rect, int left, int top, int right, int bottom);
BOOL STDCALL
NtGdiSetEmptyRect(PRECT Rect);
BOOL STDCALL
NtGdiIsEmptyRect(const RECT* Rect);
BOOL STDCALL
NtGdiIntersectRect(PRECT Dest, const RECT* Src1, const RECT* Src2);
BOOL STDCALL
NtGdiOffsetRect(LPRECT Rect, int x, int y);

#endif /* _WIN32K_RECT_H */
