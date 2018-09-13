#ifdef WIN32

// Shouldn't be using these things.
#define _huge
#define _export
#define _loadds
#define SELECTOROF(x)   ((UINT)(x))
#define OFFSETOF(x)     ((UINT)(x))
#define ISLPTR(pv)      ((BOOL)pv)
#define MAKELP(hmem,off) ((LPVOID)((LPBYTE)hmem+off))
#define MAKELRESULTFROMUINT(i)  ((LRESULT)i)
#define ISVALIDHINSTANCE(hinst) ((BOOL)hinst)

#define DATASEG_READONLY    ".text"
#define DATASEG_PERINSTANCE ".instanc"
#define DATASEG_SHARED      ".data"

#define GetWindowInt    GetWindowLong
#define SetWindowInt    SetWindowLong
#define SetWindowID(hwnd,id)    SetWindowLong(hwnd, GWL_ID, id)

#define CopyIcon(hInst, hIcon)                 CopyIcon(hIcon)
#define LOADICONEX(hInst, hInst2, a, b, c, d)  LoadIconEx(hInst2, a, b, c, d)
#define SHELLEXECUTE(a, b, c, d, e, f)         ShellExecuteEx(a, b, c, d, e, NULL, f)

#define ISVALIDLIBRARY(hLib)            (hLib != NULL)

#else  // WIN32

#define ISLPTR(pv)      (SELECTOROF(pv))
#define MAKELRESULTFROMUINT(i)  MAKELRESULT(i,0)
#define ISVALIDHINSTANCE(hinst) ((UINT)hinst>=(UINT)HINSTANCE_ERROR)

#define DATASEG_READONLY    "_TEXT"
#define DATASEG_PERINSTANCE
#define DATASEG_SHARED

#define GetWindowInt    GetWindowWord
#define SetWindowInt    SetWindowWord
#define SetWindowID(hwnd,id)    SetWindowWord(hwnd, GWW_ID, id)

#define MAKEPOINTS(l)     (*((POINTS FAR*)&(l)))

#define LOADICONEX(hInst, hInst2, a, b, c, d)  LoadIconEx(hInst, hInst2, a, b, c, d)
#define SHELLEXECUTE(a, b, c, d, e, f)         ShellExecute(a, b, c, d, e, f)

#define ISVALIDLIBRARY(hLib)            (hLib >= HINSTANCE_ERROR)

#endif // WIN32

#ifdef WIN32

/*****************************************************************************\
* PWIN32.H - PORTABILITY MAPPING HEADER FILE
*
* This file provides macros to map portable windows code to its 32 bit form.
\*****************************************************************************/

/*-----------------------------------USER------------------------------------*/

/* HELPER MACROS */

#define MAPVALUE(v16, v32)              (v32)
#define MAPTYPE(v16, v32)               v32
#define MAKEMPOINT(l)                   (*((MPOINT *)&(l)))
#define MPOINT2POINT(mpt,pt)            ((pt).x = (mpt).x, (pt).y = (mpt).y)
#define POINT2MPOINT(pt, mpt)           ((mpt).x = (SHORT)(pt).x, (mpt).y = (SHORT)(pt).y)
#define LONG2POINT(l, pt)               ((pt).x = (SHORT)LOWORD(l), (pt).y = (SHORT)HIWORD(l))

#define SETWINDOWUINT(hwnd, index, ui)  (UINT)SetWindowLong(hwnd, index, (LONG)(ui))
#define GETWINDOWUINT(hwnd, index)      (UINT)GetWindowLong(hwnd, index)
#define SETCLASSUINT(hwnd, index, ui)   (UINT)SetClassLong(hwnd, index, (LONG)(ui))
#define GETCLASSUINT(hwnd, index)       (UINT)GetClassLong(hwnd, index)

#define GETCBCLSEXTRA(hwnd)             GETCLASSUINT(hwnd, GCL_CBCLSEXTRA)
#define SETCBCLSEXTRA(hwnd, cb)         SETCLASSUINT(hwnd, GCL_CBCLSEXTRA, cb)
#define GETCBWNDEXTRA(hwnd)             GETCLASSUINT(hwnd, GCL_CBWNDEXTRA)     
#define SETCBWNDEXTRA(hwnd, cb)         SETCLASSUINT(hwnd, GCL_CBWNDEXTRA, cb) 
#define GETCLASSBRBACKGROUND(hwnd)      (HBRUSH)GETCLASSUINT(hwnd, GCL_HBRBACKGROUND)
#define SETCLASSBRBACKGROUND(hwnd, h)   (HBRUSH)SETCLASSUINT(hwnd, GCL_HBRBACKGROUND, h)
#define GETCLASSCURSOR(hwnd)            (HCURSOR)GETCLASSUINT(hwnd, GCL_HCURSOR)
#define SETCLASSCURSOR(hwnd, h)         (HCURSOR)SETCLASSUINT(hwnd, GCL_HCURSOR, h)
#define GETCLASSHMODULE(hwnd)           (HMODULE)GETCLASSUINT(hwnd, GCL_HMODULE)            
#define SETCLASSHMODULE(hwnd, h)        (HMODULE)SETCLASSUINT(hwnd, GCL_HMODULE, h) 
#define GETCLASSICON(hwnd)              (HICON)GETCLASSUINT((hwnd), GCL_HICON)
#define SETCLASSICON(hwnd, h)           (HICON)SETCLASSUINT((hwnd), GCL_HICON, h)
#define GETCLASSSTYLE(hwnd)             GETCLASSUINT((hwnd), GCL_STYLE)            
#define SETCLASSSTYLE(hwnd, style)      SETCLASSUINT((hwnd), GCL_STYLE, style) 
#define GETHWNDINSTANCE(hwnd)           (HINSTANCE)GETWINDOWUINT((hwnd), GWL_HINSTANCE)
#define SETHWNDINSTANCE(hwnd, h)        (HINSTANCE)SETWINDOWUINT((hwnd), GWL_HINSTANCE, h)
#define GETHWNDPARENT(hwnd)             (HWND)GETWINDOWUINT((hwnd), GWL_HWNDPARENT)
#define SETHWNDPARENT(hwnd, h)          (HWND)SETWINDOWUINT((hwnd), GWL_HWNDPARENT, h)
#define GETWINDOWID(hwnd)               GETWINDOWUINT((hwnd), GWL_ID)            
#define SETWINDOWID(hwnd, id)           SETWINDOWUINT((hwnd), GWL_ID, id) 

#else

/*****************************************************************************\
* PWIN16.H - PORTABILITY MAPPING HEADER FILE
*
* This file provides macros to map portable windows code to its 16 bit form.
\*****************************************************************************/

/*-----------------------------------USER------------------------------------*/
 
/* HELPER MACROS */

#define MAPVALUE(v16, v32)              (v16)
#define MAPTYPE(v16, v32)               v16
#define MAKEMPOINT(l)                   (*((MPOINT FAR *)&(l)))
#define MPOINT2POINT(mpt, pt)           (pt = *(POINT FAR *)&(mpt))
#define POINT2MPOINT(pt, mpt)           (mpt = *(MPOINT FAR *)&(pt))
#define LONG2POINT(l, pt)               ((pt).x = (INT)LOWORD(l), (pt).y = (INT)HIWORD(l))

#define GETWINDOWUINT(hwnd, index)      (UINT)GetWindowWord(hwnd, index)
#define SETWINDOWUINT(hwnd, index, ui)  (UINT)SetWindowWord(hwnd, index, (WORD)(ui))
#define SETCLASSUINT(hwnd, index, ui)   (UINT)SetClassWord(hwnd, index, (WORD)(ui))
#define GETCLASSUINT(hwnd, index)       (UINT)GetClassWord(hwnd, index)

#define GETCBCLSEXTRA(hwnd)             GETCLASSUINT(hwnd, GCW_CBCLSEXTRA)
#define SETCBCLSEXTRA(hwnd, cb)         SETCLASSUINT(hwnd, GCW_CBCLSEXTRA, cb)
#define GETCBWNDEXTRA(hwnd)             GETCLASSUINT(hwnd, GCW_CBWNDEXTRA)     
#define SETCBWNDEXTRA(hwnd, cb)         SETCLASSUINT(hwnd, GCW_CBWNDEXTRA, cb) 
#define GETCLASSBRBACKGROUND(hwnd)      (HBRUSH)GETCLASSUINT(hwnd, GCW_HBRBACKGROUND)
#define SETCLASSBRBACKGROUND(hwnd, h)   (HBRUSH)SETCLASSUINT(hwnd, GCW_HBRBACKGROUND, h)
#define GETCLASSCURSOR(hwnd)            (HCURSOR)GETCLASSUINT(hwnd, GCW_HCURSOR)
#define SETCLASSCURSOR(hwnd, h)         (HCURSOR)SETCLASSUINT(hwnd, GCW_HCURSOR, h)
#define GETCLASSHMODULE(hwnd)           (HMODULE)GETCLASSUINT(hwnd, GCW_HMODULE)            
#define SETCLASSHMODULE(hwnd, h)        (HMODULE)SETCLASSUINT(hwnd, GCW_HMODULE, h) 
#define GETCLASSICON(hwnd)              (HICON)GETCLASSUINT((hwnd), GCW_HICON)
#define SETCLASSICON(hwnd, h)           (HICON)SETCLASSUINT((hwnd), GCW_HICON, h)
#define GETCLASSSTYLE(hwnd)             GETCLASSUINT((hwnd), GCW_STYLE)            
#define SETCLASSSTYLE(hwnd, style)      SETCLASSUINT((hwnd), GCW_STYLE, style) 
#define GETHWNDINSTANCE(hwnd)           (HMODULE)GETWINDOWUINT((hwnd), GWW_HINSTANCE)
#define SETHWNDINSTANCE(hwnd, h)        (HMODULE)SETWINDOWUINT((hwnd), GWW_HINSTANCE, h)
#define GETHWNDPARENT(hwnd)             (HWND)GETWINDOWUINT((hwnd), GWW_HWNDPARENT)
#define SETHWNDPARENT(hwnd, h)          (HWND)SETWINDOWUINT((hwnd), GWW_HWNDPARENT, h)
#define GETWINDOWID(hwnd)               GETWINDOWUINT((hwnd), GWW_ID)            
#define SETWINDOWID(hwnd, id)           SETWINDOWUINT((hwnd), GWW_ID, id) 

#endif // WIN32
