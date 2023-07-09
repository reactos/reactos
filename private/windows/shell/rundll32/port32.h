#ifdef WIN32

// These things have direct equivalents.
#define hmemcpy memmove
#define lstrcpyn strncpy

// Shouldn't be using these things.
#define _huge
#define _export
#define _loadds
#define SELECTOROF(x)   ((UINT)(x))
#define OFFSETOF(x)     ((UINT)(x))
#define MAKELP(hmem,off) ((LPVOID)((LPBYTE)hmem+off))
#define MAKELRESULTFROMUINT(i)  ((LRESULT)i)
#define ISVALIDHINSTANCE(hinst) ((BOOL)hinst)

#define DATASEG_READONLY    ".rodata"
#define DATASEG_PERINSTANCE ".instance"
#define DATASEG_SHARED                      // default (".data")

#define GetWindowInt    GetWindowLong
#define SetWindowInt    SetWindowLong
#define SetWindowID(hwnd,id)    SetWindowLong(hwnd, GWL_ID, id)

#else  // WIN32

#define MAKELRESULTFROMUINT(i)  MAKELRESULT(i,0)
#define ISVALIDHINSTANCE(hinst) ((UINT)hinst>=HINSTANCE_ERROR)

#define DATASEG_READONLY    "_TEXT"
#define DATASEG_PERINSTANCE
#define DATASEG_SHARED

#define GetWindowInt    GetWindowWord
#define SetWindowInt    SetWindowWord
#define SetWindowID(hwnd,id)    SetWindowWord(hwnd, GWW_ID, id)

#endif // WIN32

