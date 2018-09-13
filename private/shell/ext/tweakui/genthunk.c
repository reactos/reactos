/*
 * genthunk - Generic thunks
 */

#ifndef WIN32

#include "tweakui.h"

const char CODESEG szKernel32[] = "KERNEL32";

/*****************************************************************************
 *
 *  hwnd32Hwnd converts a 16-bit hwnd to a 32-bit hwnd.  Note that we
 *  extend with 0xFFFF to be compatible with NT.
 *
 *****************************************************************************/

#define hwnd32Hwnd(hwnd) MAKELONG(hwnd, 0xFFFF)

/*****************************************************************************
 *
 *  CallProcEx32W (a.k.a. ThunkMeHarder)
 *
 *	Generic wrapper for thunking.
 *
 *	lpszDll -> ASCIIZ DLL name
 *	lpszProc -> ASCIIZ procedure name
 *	c = number of arguments
 *	dwMask = bitmask; if argument N is a pointer, then bit c-N is set.
 *			  Yes, it's backwards.
 *	arg0, arg1, arg2, ... = arguments
 *
 *	Returns whatever the procedure returns, or 0 on error.
 *
 *	To aid in building dwMask, the macro ARGPTR(k,n) builds the
 *	appropriate bitmask that indicates that argument k of n is
 *	a pointer.  k is zero-based, of course.
 *
 *	This is pretty gross evil code that is Intel x86-specific.
 *	But hey, it's Win16.  That's to be expected.
 *
 *	CallProc32W is insane.  It's a variadic function that uses
 *	the pascal calling convention.  (It probably makes more sense
 *	when you're stoned.)
 *
 *****************************************************************************/

DWORD PASCAL GetProcAddressEx32W(DWORD hinst, LPCSTR lpsz);

DWORD WINAPI LoadLibraryEx32W(LPCSTR lpszLib, DWORD dw1, DWORD dw2);
DWORD WINAPI GetProcAddress32W(DWORD hinst, LPCSTR lpsz);
DWORD WINAPI FreeLibrary32W(DWORD hinst);
DWORD WINAPI CallProc32W(DWORD pfn, DWORD dwMask, DWORD c);

#define CallProcEx32W ThunkMeHarder
#define ARGPTR(k,n) (1<<(n-k-1))

DWORD _cdecl
ThunkMeHarder(LPCSTR lpszDll, LPCSTR lpszProc, UINT c, DWORD dwMask, ...)
{
    DWORD dwRc = 0;
    DWORD hdll, pfn;
    if (SELECTOROF(lpszDll) ?
			(HIWORD(hdll = LoadLibraryEx32W(lpszDll, 0, 0)) &&
			 HIWORD(pfn = GetProcAddressEx32W(hdll, lpszProc)))
			: (hdll = 0, pfn = (DWORD)lpszProc, 1)) {
	_asm {
	    mov	cx, c	/* cx = number of arguments */
	    xor	si, si	/* si = number of bytes pushed */
	    jcxz	PushesDone
	PushHarder:
	    _emit 0x66	    	/* 32-bit override */
	    push word ptr dwMask[4+si]
	    add	si, 4
	    loop	PushHarder
	PushesDone:
	}
	dwRc = CallProc32W(pfn, dwMask, c);
    }
    if (HIWORD(hdll)) FreeLibrary32W(hdll);
    return dwRc;
}

/*****************************************************************************
 *
 *  GetProcAddressEx32W
 *
 *  The same as GetProcAddress32W, except it also understands ordinals.
 *
 *  Yes, this is a rather incestuous relationship we have with ThunkMeHarder,
 *  but with a name like that, it's almost expected...
 *
 *  (The Shell VxD does a very similar thing... I've done this before...)
 *
 *****************************************************************************/

const char CODESEG szGetProcAddress[] = "GetProcAddress";

DWORD PASCAL
GetProcAddressEx32W(DWORD hinst, LPCSTR lpsz)
{
    if (SELECTOROF(lpsz)) {			/* Optimization */
	return GetProcAddress32W(hinst, lpsz);
    } else {
	return ThunkMeHarder(szKernel32, szGetProcAddress, 2, 0, hinst, lpsz);
    }
}

/*****************************************************************************
 *
 *  CopyFile
 *
 *****************************************************************************/

const char CODESEG szCopyFileA[] = "CopyFileA";

BOOL PASCAL
CopyFile(LPCSTR lpszSrc, LPCSTR lpszDst, BOOL fFailExists)
{
    return CallProcEx32W(szKernel32, szCopyFileA, 3, ARGPTR(0,3)|ARGPTR(1,3),
			 lpszSrc, lpszDst, (DWORD)fFailExists) != 0;
}

/*****************************************************************************
 *
 *  SHChangeNotify
 *
 *  Actually, we are slimy because we *know* that uFlags is always
 *  SHCNF_PIDL, so no parameters need to be thunked.
 *
 *****************************************************************************/

const char CODESEG szSHChangeNotify[] = "SHChangeNotify";

void PASCAL SHChangeNotify(LONG wEventId, UINT uFlags,
			   const void FAR *dwItem1, const void FAR *dwItem2)
{
    CallProcEx32W(szShell32, szSHChangeNotify, 4, 0,
		  wEventId, (DWORD)uFlags, dwItem1, dwItem2);
}

/*****************************************************************************
 *
 *  SHGetSpecialFolderLocation
 *
 *  Ignore the return value; just check the pidl.
 *
 *****************************************************************************/

const char CODESEG szSHGetSpecialFolderLocation[]
						= "SHGetSpecialFolderLocation";

void PASCAL
SHGetSpecialFolderLocation(HWND hwnd, int nFolder, PIDL FAR *ppidl)
{
    CallProcEx32W(szShell32, szSHGetSpecialFolderLocation, 3, ARGPTR(2,3),
		  hwnd32Hwnd(hwnd), (LONG)nFolder, (LPVOID)ppidl);
}

/*****************************************************************************
 *
 *  SHGetPathFromIDList
 *
 *  Ignore the return value; just check the pidl.
 *
 *****************************************************************************/

const char CODESEG szSHGetPathFromIDList[] = "SHGetPathFromIDList";

void PASCAL
SHGetPathFromIDList(PIDL pidl, LPSTR pszBuf)
{
    CallProcEx32W(szShell32, szSHGetPathFromIDList, 2, ARGPTR(1,2),
		  pidl, pszBuf);
}

/*****************************************************************************
 *
 *  ILFree
 *
 *  This is exported by ordinal.
 *
 *****************************************************************************/

void WINAPI
ILFree(PIDL pidl)
{
    CallProcEx32W(szShell32, MAKEINTRESOURCE(155), 1, 0, pidl);
}

/*****************************************************************************
 *
 *  Shell_GetImageLists
 *
 *  This is exported by ordinal.
 *
 *****************************************************************************/

BOOL PASCAL
Shell_GetImageLists(HIMAGELIST FAR *phiml, HIMAGELIST FAR *phimlSmall)
{
    return (BOOL)CallProcEx32W(szShell32, MAKEINTRESOURCE(71), 2,
			       ARGPTR(0,2)|ARGPTR(1,2), phiml, phimlSmall);
}

/*****************************************************************************
 *
 *  ExtractIconEx
 *
 *****************************************************************************/

const char CODESEG szExtractIconExA[] = "ExtractIconExA";

int PASCAL
ExtractIconEx(LPCSTR pszFile, int iIcon,
	      HICON FAR *phiconLarge, HICON FAR *phiconSmall, int nIcons)
{
    return (int)CallProcEx32W(szShell32, szExtractIconExA, 5,
			      ARGPTR(0,5)|ARGPTR(2,5)|ARGPTR(3,5),
			      pszFile, (LONG)iIcon, phiconLarge, phiconSmall,
			      (LONG)nIcons);
}

#endif /* !WIN32 */
