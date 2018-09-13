#ifdef WIN32

// These things have direct equivalents.

// Shouldn't be using these things.
#define WINCAPI __cdecl
#define _huge
#define _export
#define _loadds
#define SELECTOROF(x)   ((WPARAM)(x))
#define OFFSETOF(x)     ((WPARAM)(x))
#define ISLPTR(pv)      ((BOOL)pv)
#define MAKELP(hmem,off) ((LPVOID)((LPBYTE)hmem+off))
#define MAKELRESULTFROMUINT(i)  ((LRESULT)i)
#define ISVALIDHINSTANCE(hinst) BOOLFROMPTR(hinst)

// HIWORD is typically used to detect whether a pointer parameter
// is a real pointer or is a MAKEINTATOM.  HIWORD64 is the Win64-compatible
// version of this usage.  It does *NOT* return the top word of a 64-bit value.
// Rather, it returns the top 48 bits of the 64-bit value.
//
// Yes, the name sucks.  Any better ideas?
//
// BOOLFROMPTR is used when you have a pointer or a ULONG_PTR
// and you want to turn it into a BOOL.  In Win32,
// sizeof(BOOL) == sizeof(LPVOID) so a straight cast works.
// In Win64, you have to do it the slow way because pointers are 64-bit.
//
#ifdef _WIN64
#define HIWORD64(p)     ((ULONG_PTR)(p) >> 16)
#define BOOLFROMPTR(p)  ((p) != 0)
#define SPRINTF_PTR		"%016I64x"
#else
#define HIWORD64        HIWORD
#define BOOLFROMPTR(p)  ((BOOL)(p))
#define SPRINTF_PTR		"%08x"
#endif

#define DATASEG_READONLY    ".text"	        // don't use this, compiler does this for you
#define DATASEG_PERINSTANCE "INSTDATA"      // per instance data (per process)
#ifdef WINNT
#define DATASEG_SHARED
#else
#define DATASEG_SHARED      "SHARED"        // global global data (shared across process)
#endif
#define CODESEG_INIT        ".text"

#define GetWindowInt        GetWindowLongPtr
#define SetWindowInt        SetWindowLongPtr
#define SetWindowID(hwnd,id)    SetWindowLongPtr(hwnd, GWLP_ID, id)
#define GetClassCursor(hwnd)    ((HCURSOR)GetClassLongPtr(hwnd, GCLP_HCURSOR))
#define GetClassIcon(hwnd)      ((HICON)GetClassLongPtr(hwnd, GCLP_HICON))
#define BOOL_PTR                INT_PTR

#ifdef WINNT

#else

typedef TBYTE TUCHAR;

#endif


#else  // !WIN32

typedef LPCSTR LPCTSTR;
typedef LPSTR  LPTSTR;
typedef const short far *LPCWSTR;
#define TEXT(x) (x)

#define ISLPTR(pv)      	(SELECTOROF(pv))
#define MAKELRESULTFROMUINT(i)  MAKELRESULT(i,0)
#define ISVALIDHINSTANCE(hinst) ((UINT)hinst>=(UINT)HINSTANCE_ERROR)

#define DATASEG_READONLY    "_TEXT"
#define DATASEG_PERINSTANCE
#define DATASEG_SHARED
#define CODESEG_INIT 	    "_INIT"

#define GetWindowInt    	GetWindowWord
#define SetWindowInt    	SetWindowWord
#define SetWindowID(hwnd,id)    SetWindowWord(hwnd, GWW_ID, id)
#define GetClassCursor(hwnd)    ((HCURSOR)GetClassWord(hwnd, GCW_HCURSOR))
#define GetClassIcon(hwnd)      ((HICON)GetClassWord(hwnd, GCW_HICON))

#define MAKEPOINTS(l)     (*((POINTS FAR*)&(l)))

#define GlobalAlloc16(f, s) GlobalAlloc(f, s)
#define GlobalLock16(h)     GlobalLock(h)
#define GlobalUnlock16(h)   GlobalUnlock(h)
#define GlobalFree16(h)     GlobalFree(h)
#define GlobalSize16(h)     GlobalSize(h)

#endif // WIN32
