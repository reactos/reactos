BOOL STDCALL
W32kUnionRect(PRECT Dest, const RECT* Src1, const RECT* Src2);
BOOL STDCALL
W32kSetRect(PRECT Rect, INT left, INT top, INT right, INT bottom);
BOOL STDCALL
W32kSetEmptyRect(PRECT Rect);
BOOL STDCALL
W32kIsEmptyRect(PRECT Rect);
BOOL STDCALL
W32kIntersectRect(PRECT Dest, const RECT* Src1, const RECT* Src2);
BOOL
W32kOffsetRect(LPRECT Rect, INT x, INT y);
