#ifdef WIN32

// These things have direct equivalents.

// Shouldn't be using these things.
#define _huge
#define _export
#define SELECTOROF(x)   ((UINT_PTR)(x))
#define OFFSETOF(x)     ((UINT_PTR)(x))
#define ISLPTR(pv)      (pv)
#define MAKELP(hmem,off) ((LPVOID)((LPBYTE)hmem+off))
#define MAKELRESULTFROMUINT(i)  ((LRESULT)i)
#define ISVALIDHINSTANCE(hinst) (hinst)

#define DATASEG_READONLY    TEXT(".text")
#define DATASEG_PERINSTANCE TEXT(".instanc")
#define DATASEG_SHARED      TEXT(".data")

#define GetWindowInt    GetWindowLong
#define SetWindowInt    SetWindowLong
#define SetWindowID(hwnd,id)    SetWindowLongPtr(hwnd, GWLP_ID, id)
#define MCopyIconEx(hinst, hicon, cx, cy, flags) CopyIconEx(hicon, cx, cy, flags)
#define MLoadIconEx(hinst1, hinst2, lpsz, cx, cy, flags) LoadIconEx(hinst2, lpsz, cx, cy, flags)

#endif // WIN32
