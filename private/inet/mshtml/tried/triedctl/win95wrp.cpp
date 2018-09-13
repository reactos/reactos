//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1999.
//
//  File:       win95wrp.cpp
//
//  This file was taken from the Davinci sources and adapted for TriEdit
//  on 3/11/98 in order to get rid of the external dependency for the TriEdit SDK
//  The adaptation process included getting rid of several APIs that were not being
//  supported and moving some APIs from the supported to unsupported group
//
//  Contents:   Unicode wrapper API, used only on Win95
//
//  Functions:  About 125 Win32 function wrappers
//
//  Notes:      'sz' is used instead of the "correct" hungarian 'psz'
//              throughout to enhance readability.
//
//              Not all of every Win32 function is wrapped here.  Some
//              obscurely-documented features may not be handled correctly
//              in these wrappers.  Caller beware.
//
//              These are privately exported for use by the Shell.
//              All memory allocation is done on the stack.
//
//----------------------------------------------------------------------------

// Includes ------------------------------------------------------------------

#include "stdafx.h"

// The following two lines will ensure that no mapping from Foo to OFoo will take place
// and the real windows APIs will get called from this file
#define __WIN95WRP_CPP__
#include "win95wrp.h"

#include <mbstring.h>
#include <commctrl.h>
#include <shlobj.h>

// Function prototypes
inline LONG UnicodeToAnsi(LPSTR szOut, LPCWSTR pwszIn, LONG cbOut, LONG cbIn = -1) throw();
inline LONG AnsiToUnicode(LPWSTR pwszOut, LPCSTR szIn, LONG cbOut, LONG cbIn = -1) throw();
static void CvtDevmode(DEVMODEA *pdma, const DEVMODEW *pdmw) throw();

BOOL g_fWin95;
BOOL g_fOSInit = FALSE;

// Debug ----------------------------------------------------------------------
#ifdef _DEBUG
#define Assert(f)   ((f) ? 0 : AssertFail(#f))
#define Verify(f)   Assert(f)
#define Debug(f)    (f)
#else
#define Assert(f)   (0)
#define Verify(f)   (f)
#define Debug(f)    (0)
#endif

#ifdef DEBUG
int AssertFail(const CHAR *pszMsg) throw()
{
	int wRet = MessageBoxA(NULL, pszMsg, "Assert Failed in Win95 layer",
						   MB_ABORTRETRYIGNORE | MB_DEFBUTTON3 |
						   MB_SYSTEMMODAL | MB_ICONHAND );

	switch (wRet)
		{
		case IDABORT:
			FatalAppExit(0, L"BOO HOO");
			break;

		case IDRETRY:
			DebugBreak();
			// deliberately fall through to IDIGNORE in order to continue

		case IDIGNORE:

			// go aways
			break;
		}
	return 0;
}
#else
#define AssertFail(s) (0)
#endif  // ! DEBUG

// This macro determines whether a LPTSTR is an atom or string pointer
#define FATOM(x)    (!(HIWORD(x)))

// OffsetOf       - Return the byte offset into s of m
#define	OffsetOf(s,m)	(size_t)(((unsigned char*)&(((s*)0)->m))-((unsigned char*)0))

inline LONG UnicodeToAnsi(LPSTR szDestString, LPCWSTR pwszSrcString,
						  LONG  cbDestString, LONG    cbSrcString ) throw()
{

	Assert(-1 != cbDestString && (!cbDestString || szDestString));
	return WideCharToMultiByte(CP_ACP, 0, pwszSrcString, cbSrcString,
							   szDestString, cbDestString, NULL, NULL);

}

inline LONG AnsiToUnicode(LPWSTR pwszDestString, LPCSTR szSrcString,
						  LONG   cbDestString,   LONG   cbSrcString ) throw()
{

	Assert(-1 != cbDestString && (!cbDestString || pwszDestString));
	return MultiByteToWideChar(CP_ACP, 0, szSrcString, cbSrcString,
							   pwszDestString, cbDestString );
}

static void CvtDevmode(
	DEVMODEA *pdma,
	const DEVMODEW *pdmw
	) throw()
{
	Verify(0 <= UnicodeToAnsi((LPSTR)pdma->dmDeviceName, pdmw->dmDeviceName, CCHDEVICENAME));

	memcpy(&pdma->dmSpecVersion, &pdmw->dmSpecVersion, OffsetOf(DEVMODE,dmFormName) -
		OffsetOf(DEVMODE,dmSpecVersion));

	Verify(0 <= UnicodeToAnsi((LPSTR)pdma->dmFormName, pdmw->dmFormName, CCHFORMNAME));

	memcpy(&pdma->dmLogPixels, &pdmw->dmLogPixels, sizeof(DEVMODE)-OffsetOf(DEVMODE, dmLogPixels));

	// Make sure we copy the extra driver bits.
	if ( pdmw->dmDriverExtra )
		memcpy((char*)pdma + sizeof(DEVMODEA), (char*)pdmw + sizeof(DEVMODEW), pdmw->dmDriverExtra );
}


inline bool FWide() throw()
{
	if (!g_fOSInit)
	{
		OSVERSIONINFOA osvi;
		osvi.dwOSVersionInfoSize = sizeof(osvi);

		GetVersionExA(&osvi);
		g_fWin95 = (VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId);
		g_fOSInit = TRUE;
	}

	Assert(g_fOSInit);
	return !g_fWin95;
}

//  The implementation of the Unicode to ANSI (MBCS) convertion macros use the
//  _alloca() function to allocate memory from the stack instead of the heap.
//  Allocating memory from the stack is much faster than allocating memory on
//  the heap, and the memory is automatically freed when the function is exited.
//  In addition, these macros avoid calling WideCharToMultiByte more than one
//  time.  This is done by allocating a little bit more memory than is
//  necessary.  We know that an MBC will convert into at most one WCHAR and
//  that for each WCHAR we will have a maximum of two MBC bytes.  By allocating
//  a little more than necessary, but always enough to handle the conversion
//  the second call to the conversion function is avoided.  The call to the
//  helper function UnicodeToAnsi reduces the number of argument pushes that
//  must be done in order to perform the conversion (this results in smaller
//  code, than if it called WideCharToMultiByte directly).
//
//  In order for the macros to store the temporary length and the pointer to
//  the resultant string, it is necessary to declare some local variables
//  called _len and _sz in each function that uses these conversion macros.
//  This is done by invoking the PreConvert macro in each function before any
//  uses of Convert or ConverWithLen.  (PreConvert just need to be invoked once
//  per function.)

#define PreConvert() \
	LONG   _len;     \
	LPSTR  _sz;      \
	LONG   _lJunk;   \
	_lJunk; // Unused sometimes

// stack-allocates a char buffer of size cch
#define SzAlloc(cch)  ((LPSTR)_alloca(cch))

// stack-allocates a wchar buffer of size cch
#define SzWAlloc(cch) ((LPWSTR)_alloca(cch * sizeof(WCHAR)))

// Returns a properly converted string,
//   or NULL string on failure or szFrom == NULL
// On return the variable passed via pnTo will have the output byte count
//   (including the trailing '\0' iff the nFrom is -1)
#define ConvertWithLen(szFrom, nFrom, pnTo) \
			(!szFrom ? NULL : \
				(_len = (-1 == nFrom ? (wcslen(szFrom) + 1) : nFrom) * \
						sizeof(WCHAR), \
				 _sz = SzAlloc(_len + sizeof(WCHAR)), \
				 Debug(_sz[_len] = '\0'), \
				 (0 > ((*pnTo) = UnicodeToAnsi(_sz, szFrom, _len, nFrom)) ? \
				  (AssertFail("Convert failed in Unicode wrapper"), NULL) : \
				  (Assert('\0' == _sz[_len]), _sz) ) ) )
#define Convert(szFrom) ConvertWithLen(szFrom, -1, &_lJunk)

// There are strings which are blocks of strings end to end with a trailing '\0'
// to indicate the true end.  These strings are used with the REG_MULTI_SZ
// option of the Reg... routines and the lpstrFilter field of the OPENFILENAME
// structure used in the GetOpenFileName and GetSaveFileName routines.  To help
// in converting these strings here are two routines which calculate the length
// of the Unicode and ASNI versions (including all '\0's!):

size_t
cUnicodeMultiSzLen
(
LPCWSTR lpsz
) throw()
{
	size_t cRet = 0;
	while (*lpsz)
		{
		size_t c = wcslen(lpsz) + 1;
		cRet += c;
		lpsz += c;
		}
	return cRet + 1;
}

size_t
cAnsiMultiSzLen
(
LPCSTR lpsz
) throw()
{
	size_t cRet = 0;
	while (*lpsz)
		{
		size_t c = _mbslen((const unsigned char*)lpsz) + 1;
		cRet += c;
		lpsz += c;
		}
	return cRet + 1;
}

extern "C"{

// Added by VanK for DHTMLEdit OCX
HINTERNET
WINAPI
OInternetOpenW(LPCWSTR lpszAgent, DWORD dwAccessType, LPCWSTR lpszProxy, LPCWSTR lpszProxyBypass, DWORD dwFlags)
{
	if(FWide())
		return InternetOpenW(lpszAgent, dwAccessType, lpszProxy, lpszProxyBypass, dwFlags);

	PreConvert();

	LPSTR szAgent	= NULL;
	LPSTR szProxy	= NULL;
	LPSTR szBypass	= NULL;

	if ( NULL != lpszAgent )
		szAgent = Convert(lpszAgent);
	if ( NULL != lpszProxy )
		szProxy = Convert(lpszProxy);
	if ( NULL != lpszProxyBypass )
		szBypass = Convert(lpszProxyBypass);

	return InternetOpenA(szAgent, dwAccessType, szProxy, szBypass, dwFlags);
}


HINTERNET
WINAPI
OInternetOpenUrlW(HINTERNET hInternet, LPCWSTR lpszUrl, LPCWSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext)
{
	if(FWide())
		return InternetOpenUrlW(hInternet, lpszUrl, lpszHeaders, dwHeadersLength, dwFlags, dwContext);

	PreConvert();

	LPSTR szURL		= NULL;
	LPSTR szHead	= NULL;

	if ( NULL != lpszUrl )
		szURL = Convert(lpszUrl);
	if ( NULL != lpszHeaders )
		szHead = Convert(lpszHeaders);

	return InternetOpenUrlA(hInternet, szURL, szHead, dwHeadersLength, dwFlags, dwContext);
}


HRESULT
__stdcall
OURLOpenBlockingStreamW(
	LPUNKNOWN				pCaller,	// In
	LPCWSTR					wszURL,		// In
	LPSTREAM				*ppStream,	// Out
	DWORD					dwReserved,	// In
	LPBINDSTATUSCALLBACK	lpfnCB		// In
	)
{
	if(FWide())
		return URLOpenBlockingStreamW(pCaller, wszURL, ppStream, dwReserved, lpfnCB);

	PreConvert();
	LPSTR szURL = Convert(wszURL);
	return URLOpenBlockingStreamA(pCaller, szURL, ppStream, dwReserved, lpfnCB);
}

// Added by VanK for DHTMLEdit OCX
// lpdwUrlLength must be set even on failure; callers use a zero length to determine
// how many bytes to actually allocate.
// NOTE that contrary to all expectations, lpdwUrlLength is a byte count, not a character count!
BOOL
WINAPI
OInternetCreateUrlW(
	LPURL_COMPONENTSW	lpUrlComponents,	// In
	DWORD				dwFlags,			// In
	LPWSTR				lpwszUrl,			// Out
	LPDWORD				lpdwUrlLength		// In/Out
	)
{
	Assert ( lpUrlComponents );
	Assert ( lpdwUrlLength );

	if(FWide())
		return InternetCreateUrlW(lpUrlComponents, dwFlags, lpwszUrl, lpdwUrlLength);

	PreConvert();
	DWORD cchLen = (*lpdwUrlLength) / sizeof(WCHAR);	// dwUrlLength is a count of bytes
	LPSTR szUrl = SzAlloc(*lpdwUrlLength);
	URL_COMPONENTSA	urlaComp;
	memset ( &urlaComp, 0, sizeof ( urlaComp ) );	// In case this is ever recompiled with a larger structure

	urlaComp.dwStructSize		= sizeof(URL_COMPONENTSA);
    urlaComp.lpszScheme			= Convert(lpUrlComponents->lpszScheme); 
	urlaComp.dwSchemeLength		= lpUrlComponents->dwSchemeLength;
	urlaComp.nScheme			= lpUrlComponents->nScheme;
    urlaComp.lpszHostName		= Convert(lpUrlComponents->lpszHostName);
	urlaComp.dwHostNameLength	= lpUrlComponents->dwHostNameLength;
	urlaComp.nPort				= lpUrlComponents->nPort;
	urlaComp.lpszUserName		= Convert(lpUrlComponents->lpszUserName);
	urlaComp.dwUserNameLength	= lpUrlComponents->dwUserNameLength;
	urlaComp.lpszPassword		= Convert(lpUrlComponents->lpszPassword);
	urlaComp.dwPasswordLength	= lpUrlComponents->dwPasswordLength;
	urlaComp.lpszUrlPath		= Convert(lpUrlComponents->lpszUrlPath);
	urlaComp.dwUrlPathLength	= lpUrlComponents->dwUrlPathLength;
	urlaComp.lpszExtraInfo		= Convert(lpUrlComponents->lpszExtraInfo);
	urlaComp.dwExtraInfoLength	= lpUrlComponents->dwExtraInfoLength;

	BOOL bfResult = InternetCreateUrlA(&urlaComp, dwFlags, szUrl, &cchLen);
	*lpdwUrlLength = cchLen*sizeof(WCHAR);	// Return even on fail; this tells how much to allocate on next call.
	if ( bfResult )
	{
		LONG lRet = AnsiToUnicode(lpwszUrl, szUrl, *lpdwUrlLength, cchLen);
		*lpdwUrlLength = lRet * sizeof(WCHAR);
	}
	return bfResult;
}

// Added by VanK for DHTMLEdit OCX
//	INCOMPLETE IMPLEMENTATION:
//	This implementation is not designed to work in the case where string pointers
//	are specified as NULL and their corresponding lengths non-zero.
//	Normally, this results in a pointer to the first character of the corresponding
//	component to be stored in the pointer.
//
//	IMPLEMENTATION NOTE:
//	Returned strings are terminated.  Although the system implementation seems to assume
//	that the buffers are one character larger than the character count supplied, and
//	thereby equipped to accept an additional terminator, this behavior is never clearly
//	documented.  I feel this behavior is too unsafe, so ONE CHARACTER LESS than the
//	buffer size in characters is the maximum size of the string that can be returned.
BOOL WINAPI OInternetCrackUrlW(
	LPCWSTR				lpwszUrl,		// In
	DWORD				dwUrlLength,	// In
	DWORD				dwFlags,		// In
	LPURL_COMPONENTSW	lpUrlComponents	// Out
	)
{
	if(FWide())
		return InternetCrackUrlW(lpwszUrl, dwUrlLength, dwFlags, lpUrlComponents);

	// Test our limitation restrictions:
	Assert ( ((NULL != lpUrlComponents->lpszScheme) && (0 < lpUrlComponents->dwSchemeLength)) ||
			 ((NULL == lpUrlComponents->lpszScheme) && (0 == lpUrlComponents->dwSchemeLength)));
	Assert ( ((NULL != lpUrlComponents->lpszHostName) && (0 < lpUrlComponents->dwHostNameLength)) ||
			 ((NULL == lpUrlComponents->lpszHostName) && (0 == lpUrlComponents->dwHostNameLength)));
	Assert ( ((NULL != lpUrlComponents->lpszUserName) && (0 < lpUrlComponents->dwUserNameLength)) ||
			 ((NULL == lpUrlComponents->lpszUserName) && (0 == lpUrlComponents->dwUserNameLength)));
	Assert ( ((NULL != lpUrlComponents->lpszPassword) && (0 < lpUrlComponents->dwPasswordLength)) ||
			 ((NULL == lpUrlComponents->lpszPassword) && (0 == lpUrlComponents->dwPasswordLength)));
	Assert ( ((NULL != lpUrlComponents->lpszUrlPath) && (0 < lpUrlComponents->dwUrlPathLength)) ||
			 ((NULL == lpUrlComponents->lpszUrlPath) && (0 == lpUrlComponents->dwUrlPathLength)));
	Assert ( ((NULL != lpUrlComponents->lpszExtraInfo) && (0 < lpUrlComponents->dwExtraInfoLength)) ||
			 ((NULL == lpUrlComponents->lpszExtraInfo) && (0 == lpUrlComponents->dwExtraInfoLength)));

	PreConvert();
	LPCSTR	szURLIn		= Convert(lpwszUrl);
	LPSTR	szScheme	= NULL;
	LPSTR	szHostName	= NULL;
	LPSTR	szUserName	= NULL;
	LPSTR	szPassword	= NULL;
	LPSTR	szUrlPath	= NULL;
	LPSTR	szExtraInfo	= NULL;

	URL_COMPONENTSA	urlaComp;
	memset ( &urlaComp, 0, sizeof ( urlaComp ) );	// In case this is ever recompiled with a larger structure
	
	if ( 0 != lpUrlComponents->dwSchemeLength && NULL != lpUrlComponents->lpszScheme )
	{
		szScheme = SzAlloc((lpUrlComponents->dwSchemeLength+1)*sizeof(WCHAR));
	}
	if ( 0 != lpUrlComponents->dwHostNameLength && NULL != lpUrlComponents->lpszHostName )
	{
		szHostName = SzAlloc((lpUrlComponents->dwHostNameLength+1)*sizeof(WCHAR));
	}
	if ( 0 != lpUrlComponents->dwUserNameLength && NULL != lpUrlComponents->lpszUserName )
	{
		szUserName = SzAlloc((lpUrlComponents->dwUserNameLength+1)*sizeof(WCHAR));
	}
	if ( 0 != lpUrlComponents->dwPasswordLength && NULL != lpUrlComponents->lpszPassword )
	{
		szPassword = SzAlloc((lpUrlComponents->dwPasswordLength+1)*sizeof(WCHAR));
	}
	if ( 0 != lpUrlComponents->dwUrlPathLength && NULL != lpUrlComponents->lpszUrlPath )
	{
		szUrlPath = SzAlloc((lpUrlComponents->dwUrlPathLength+1)*sizeof(WCHAR));
	}
	if ( 0 != lpUrlComponents->dwExtraInfoLength && NULL != lpUrlComponents->lpszExtraInfo )
	{
		szExtraInfo = SzAlloc((lpUrlComponents->dwExtraInfoLength+1)*sizeof(WCHAR));
	}

	urlaComp.dwStructSize		= sizeof(URL_COMPONENTSA);
    urlaComp.lpszScheme			= szScheme;
	urlaComp.dwSchemeLength		= lpUrlComponents->dwSchemeLength;
	urlaComp.nScheme			= lpUrlComponents->nScheme;
    urlaComp.lpszHostName		= szHostName;
	urlaComp.dwHostNameLength	= lpUrlComponents->dwHostNameLength;
	urlaComp.nPort				= lpUrlComponents->nPort;
	urlaComp.lpszUserName		= szUserName;
	urlaComp.dwUserNameLength	= lpUrlComponents->dwUserNameLength;
	urlaComp.lpszPassword		= szPassword;
	urlaComp.dwPasswordLength	= lpUrlComponents->dwPasswordLength;
	urlaComp.lpszUrlPath		= szUrlPath;
	urlaComp.dwUrlPathLength	= lpUrlComponents->dwUrlPathLength;
	urlaComp.lpszExtraInfo		= szExtraInfo;
	urlaComp.dwExtraInfoLength	= lpUrlComponents->dwExtraInfoLength;

	BOOL bfResult = InternetCrackUrlA ( szURLIn, dwUrlLength, dwFlags, &urlaComp );
	
	if ( bfResult )
	{
		lpUrlComponents->nScheme = urlaComp.nScheme;
		lpUrlComponents->nPort = urlaComp.nPort;

		if ( NULL != szScheme )
		{
			lpUrlComponents->dwSchemeLength = AnsiToUnicode(
				lpUrlComponents->lpszScheme, szScheme,
				lpUrlComponents->dwSchemeLength, urlaComp.dwSchemeLength+1) - 1;
		}
		if ( NULL != szHostName )
		{
			lpUrlComponents->dwHostNameLength = AnsiToUnicode(
				lpUrlComponents->lpszHostName, szHostName,
				lpUrlComponents->dwHostNameLength, urlaComp.dwHostNameLength+1) - 1;
		}
		if ( NULL != szUserName )
		{
			lpUrlComponents->dwUserNameLength = AnsiToUnicode(
				lpUrlComponents->lpszUserName, szUserName,
				lpUrlComponents->dwUserNameLength, urlaComp.dwUserNameLength+1) - 1;
		}
		if ( NULL != szPassword )
		{
			lpUrlComponents->dwPasswordLength = AnsiToUnicode(
				lpUrlComponents->lpszPassword, szPassword,
				lpUrlComponents->dwPasswordLength, urlaComp.dwPasswordLength+1) - 1;
		}
		if ( NULL != szUrlPath )
		{
			lpUrlComponents->dwUrlPathLength = AnsiToUnicode(
				lpUrlComponents->lpszUrlPath, szUrlPath,
				lpUrlComponents->dwUrlPathLength, urlaComp.dwUrlPathLength+1) - 1;
		}
		if ( NULL != szExtraInfo )
		{
			lpUrlComponents->dwExtraInfoLength = AnsiToUnicode(
				lpUrlComponents->lpszExtraInfo, szExtraInfo,
				lpUrlComponents->dwExtraInfoLength, urlaComp.dwExtraInfoLength+1) - 1;
		}
	}
	return bfResult;
}

// Added by VanK for DHTMLEdit OCX
BOOL
WINAPI
ODeleteUrlCacheEntryW(
	LPCWSTR	lpwszUrlName	// In
	)
{
	if(FWide())
		return DeleteUrlCacheEntryW(lpwszUrlName);

	PreConvert();
	LPSTR szUrlName = Convert(lpwszUrlName);
	return DeleteUrlCacheEntryA(szUrlName);
}

BOOL
WINAPI
OAppendMenuW(
	HMENU hMenu,
	UINT uFlags,
	UINT uIDnewItem,
	LPCWSTR lpnewItem
	)
{
	if(FWide())
		return AppendMenuW(hMenu, uFlags, uIDnewItem, lpnewItem);

	if(MF_STRING != uFlags)
		return AppendMenuA(hMenu, uFlags, uIDnewItem, (LPSTR)lpnewItem);

	PreConvert();
	LPSTR sz = Convert(lpnewItem);
	return AppendMenuA(hMenu, uFlags, uIDnewItem, sz);
}

LRESULT
WINAPI
OCallWindowProcW(
	WNDPROC lpPrevWndFunc,
	HWND hWnd,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam)
{
	if(FWide())
		return CallWindowProcW(lpPrevWndFunc, hWnd, Msg, wParam, lParam);

	return CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);  //$ CONSIDER - Not really wrapped
}

DWORD
WINAPI
OCharLowerBuffW(
	LPWSTR lpsz,
	DWORD cchLength)
{
	if(!lpsz)
		return 0;

	if(FWide())
		return CharLowerBuffW(lpsz, cchLength);

	DWORD i = 0;
	while(i++ < cchLength)
		{
		*lpsz = towlower(*lpsz);
		lpsz++;
		}
	return i;
}

LPWSTR
WINAPI
OCharLowerW(
	LPWSTR lpsz)
{
	if(!lpsz)
		return NULL;

	if(FWide())
		return CharLowerW(lpsz);

	// Checking if it's a single byte character.
	if(FATOM(lpsz))
		{
		return (LPWSTR)towlower((WCHAR)LOWORD(lpsz));
		}

	LPWSTR lp = lpsz;

	while(*lp)
		{
		*lp = towlower(*lp);
		lp++;
		}
	return lpsz;
}

// From: Mark Ashton on 5/29/97
LPWSTR
WINAPI
OCharPrevW(
	LPCWSTR lpszStart,
	LPCWSTR lpszCurrent)
{
	return (LPWSTR)((lpszStart != lpszCurrent) ? lpszCurrent - 1 : lpszCurrent);
}

BOOL
WINAPI
OCharToOemW(
	LPCWSTR lpszSrc,
	LPSTR lpszDst)
{
	if(FWide())
		{
		Assert((LPSTR) lpszSrc != lpszDst);
		return CharToOemW(lpszSrc, lpszDst);
		}

	PreConvert();
	LPSTR sz = Convert(lpszSrc);

	return CharToOemA(sz, lpszDst);
}

LPWSTR
WINAPI
OCharUpperW(
	LPWSTR lpsz)
{
	if(!lpsz)
		return NULL;

	if(FWide())
		return CharUpperW(lpsz);

	// Checking if it's a single byte character.
	if(FATOM(lpsz))
		{
		return (LPWSTR)towupper((WCHAR)LOWORD(lpsz));
		}

	LPWSTR lp = lpsz;

	while(*lp)
		{
		*lp = towupper(*lp);
		lp++;
		}
	return lpsz;
}

// From: Mark Ashton on 5/8/97
BOOL
WINAPI
OCopyFileW(
	LPCWSTR lpExistingFileName,
	LPCWSTR lpNewFileName,
	BOOL bFailIfExists
	)
{
	if (FWide())
		return CopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);

	char szExisting[_MAX_PATH], szNew[_MAX_PATH];
	Verify(0 <= UnicodeToAnsi(szExisting, lpExistingFileName, _MAX_PATH));
	Verify(0 <= UnicodeToAnsi(szNew, lpNewFileName, _MAX_PATH));
	return CopyFileA(szExisting, szNew, bFailIfExists);
}

HDC
WINAPI
OCreateDCW(
	LPCWSTR lpszDriver,
	LPCWSTR lpszDevice,
	LPCWSTR lpszOutput,
	CONST DEVMODEW *lpInitData)
{
	Assert(!lpszOutput);
	if(FWide())
		return CreateDCW(lpszDriver, lpszDevice, lpszOutput, lpInitData);

	DEVMODEA *pdma = lpInitData ?
						(DEVMODEA*)SzAlloc(sizeof(DEVMODEA) + lpInitData->dmDriverExtra) :
						NULL;

	PreConvert();
	LPSTR szDriv = Convert(lpszDriver);
	LPSTR szDev = NULL;

	// in Win95, only "display" is allowed as a driver name
	if (szDriv && !lstrcmpiA(szDriv, "display"))
		{
		Assert(!lpszDevice);
		Assert(!lpInitData);
		pdma = NULL;	// Force to NULL.
		}
	else
		{
#ifdef DEBUG
		// For NT we pass this in so only assert if this is
		// not true.
		if (szDriv && lstrcmpiA(szDriv, "winspool"))
			Assert(!lpszDriver);
#endif // DEBUG
		szDriv = NULL;
		Assert(lpszDevice);
		szDev = Convert(lpszDevice);
		if (lpInitData)
			{
			CvtDevmode(pdma, lpInitData);
			}
		}

	return CreateDCA(szDriv, szDev, NULL, pdma);
}

// From: Mark Ashton on 5/8/97
BOOL
WINAPI
OCreateDirectoryW(
	LPCWSTR lpPathName,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes
	)
{
	if (FWide())
		return CreateDirectoryW(lpPathName, lpSecurityAttributes);

	PreConvert();
	LPSTR sz = Convert(lpPathName);
	return CreateDirectoryA(sz, NULL);
}

// From: Mark Ashton on 5/8/97
//       Ted Smith: simpified on 6/25
// Smoke tested by Mark Ashton on 6/25
BOOL
WINAPI
OCreateDirectoryExW(
	LPCWSTR lpTemplateDirectory,
	LPCWSTR lpNewDirectory,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes
	)
{
	if (FWide())
		return CreateDirectoryExW(lpTemplateDirectory, lpNewDirectory, lpSecurityAttributes);

	PreConvert();
	LPSTR szTemplateDirectory = Convert(lpTemplateDirectory);
	LPSTR szNewDirectory      = Convert(lpNewDirectory);
	return CreateDirectoryExA(szTemplateDirectory, szNewDirectory, NULL);
}

HDC
WINAPI
OCreateEnhMetaFileW(
	HDC hdc,
	LPCWSTR lpFileName,
	CONST RECT *lpRect,
	LPCWSTR lpDescription)
{
	if(FWide())
		return CreateEnhMetaFileW(hdc, lpFileName, lpRect, lpDescription);

	PreConvert();
	LPSTR szN = Convert(lpFileName);
	LPSTR szD = ConvertWithLen(lpDescription, cUnicodeMultiSzLen(lpDescription), &_lJunk);
	return  CreateEnhMetaFileA(hdc, szN, lpRect, szD);
}

HANDLE
WINAPI
OCreateEventW(
	LPSECURITY_ATTRIBUTES lpEventAttributes,
	BOOL bManualReset,
	BOOL bInitialState,
	LPCWSTR lpName
	)
{
	if(FWide())
		return CreateEventW(lpEventAttributes, bManualReset, bInitialState, lpName);

	PreConvert();
	LPSTR sz = Convert(lpName);
	return CreateEventA(lpEventAttributes, bManualReset, bInitialState, sz);
}

HANDLE
WINAPI
OCreateFileW(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile
	)
{
	// Don't even attempt this on Win95!
	Assert(0 != wcsncmp(lpFileName, L"\\\\?\\", 4));

	if(FWide())
		return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
			dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	PreConvert();
	LPSTR sz = Convert(lpFileName);
	return CreateFileA(sz, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
		dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

HFONT
WINAPI
OCreateFontIndirectW(CONST LOGFONTW * plfw)
{
	Assert(plfw);

	if(FWide())
		return CreateFontIndirectW(plfw);

	LOGFONTA  lfa;

	// It's assumed here that sizeof(LOGFONTA) <= sizeof (LOGFONTW);
	memcpy(&lfa, plfw, sizeof(LOGFONTA));

	Verify(0 <= UnicodeToAnsi(lfa.lfFaceName, plfw->lfFaceName, LF_FACESIZE));
	return CreateFontIndirectA(&lfa);
}

// From: Mark Ashton on 5/29/97
HFONT
OCreateFontW(
	int nHeight, // logical height of font
	int nWidth, // logical average character width
	int nEscapement, // angle of escapement
	int nOrientation, // base-line orientation angle
	int fnWeight, // font weight
	DWORD fdwItalic, // italic attribute flag
	DWORD fdwUnderline, // underline attribute flag
	DWORD fdwStrikeOut, // strikeout attribute flag
	DWORD fdwCharSet, // character set identifier
	DWORD fdwOutputPrecision, // output precision
	DWORD fdwClipPrecision, // clipping precision
	DWORD fdwQuality, // output quality
	DWORD fdwPitchAndFamily, // pitch and family
	LPCWSTR lpszFace) // pointer to typeface name string
{
	if (FWide())
		return CreateFontW(nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet, fdwOutputPrecision, fdwClipPrecision, fdwQuality, fdwPitchAndFamily, lpszFace);
	PreConvert();
	LPSTR sz = Convert(lpszFace);
	return CreateFontA(nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet, fdwOutputPrecision, fdwClipPrecision, fdwQuality, fdwPitchAndFamily, sz);
}

HWND
WINAPI
OCreateMDIWindowW(
	LPWSTR lpClassName,
	LPWSTR lpWindowName,
	DWORD dwStyle,
	int X,
	int Y,
	int nWidth,
	int nHeight,
	HWND hWndParent,
	HINSTANCE hInstance,
	LPARAM lParam
	)
{
	if(FWide())
		return CreateMDIWindowW(lpClassName, lpWindowName, dwStyle,
			X, Y, nWidth, nHeight, hWndParent, hInstance, lParam);

	PreConvert();
	LPSTR szClass = Convert(lpClassName);
	LPSTR szWin   = Convert(lpWindowName);

	return CreateMDIWindowA(szClass, szWin, dwStyle,
			X, Y, nWidth, nHeight, hWndParent, hInstance, lParam);
}

HDC
WINAPI
OCreateMetaFileW(LPCWSTR lpstr)
{
	if(FWide())
		return CreateMetaFileW(lpstr);

	PreConvert();
	LPSTR sz = Convert(lpstr);
	return CreateMetaFileA(sz);
}

HANDLE
WINAPI
OCreateSemaphoreW(
	LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
	LONG lInitialCount,
	LONG lMaximumCount,
	LPCWSTR lpName
	)
{
	if(FWide())
		return CreateSemaphoreW(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);

	PreConvert();
	LPSTR sz = Convert(lpName);
	return CreateSemaphoreA(lpSemaphoreAttributes, lInitialCount, lMaximumCount, sz);
}

HWND  
WINAPI
OCreateWindowExW( DWORD dwExStyle,
			  LPCWSTR lpClassName,
				LPCWSTR lpWindowName,
				DWORD dwStyle,
				int X,
				int Y,
				int nWidth,
				int nHeight,
				HWND hWndParent ,
				HMENU hMenu,
				HINSTANCE hInstance,
				LPVOID lpParam )
{
	if(FWide())
		return CreateWindowExW(dwExStyle,
				lpClassName,
				lpWindowName,
				dwStyle,
				X,
				Y,
				nWidth,
				nHeight,
				hWndParent ,
				hMenu,
				hInstance,
				lpParam );

	PreConvert();

	LPSTR szClass;
	if (FATOM(lpClassName))
		{
		// is it an atom?
		szClass = (LPSTR) lpClassName;
		}
	else
		{
		// otherwise convert the string
		szClass = Convert(lpClassName);
		}
	LPSTR szWindow = Convert(lpWindowName);

	return CreateWindowExA (dwExStyle, szClass, szWindow, dwStyle, X, Y,
						 nWidth, nHeight, hWndParent, hMenu, hInstance,
						 lpParam);

}

HSZ
WINAPI
ODdeCreateStringHandleW(
	DWORD idInst,
	LPCWSTR psz,
	int iCodePage)
{
	if(FWide())
		{
		Assert(CP_WINUNICODE == iCodePage);
		return DdeCreateStringHandleW(idInst, psz, iCodePage);
		}
	PreConvert();
	LPSTR sz = Convert(psz);
	return DdeCreateStringHandleA(idInst, sz, CP_WINANSI);
}

UINT
WINAPI
ODdeInitializeW(
	LPDWORD pidInst,
	PFNCALLBACK pfnCallback,
	DWORD afCmd,
	DWORD ulRes)
{
	if(FWide())
		return DdeInitializeW(pidInst, pfnCallback, afCmd, ulRes);
	return DdeInitializeA(pidInst, pfnCallback, afCmd, ulRes);
}

LRESULT
WINAPI
ODefFrameProcW(
	HWND hWnd,
	HWND hWndMDIClient ,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	if(FWide())
		return DefFrameProcW(hWnd, hWndMDIClient , uMsg, wParam, lParam);

	return DefFrameProcA(hWnd, hWndMDIClient , uMsg, wParam, lParam);
}

LRESULT
WINAPI
ODefMDIChildProcW(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	if(FWide())
		return DefMDIChildProcW(hWnd, uMsg, wParam, lParam);

	return DefMDIChildProcA(hWnd, uMsg, wParam, lParam);
}

LRESULT
WINAPI
ODefWindowProcW(
	HWND hWnd,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam)
{
	if(FWide())
		return DefWindowProcW( hWnd, Msg,wParam, lParam);

	return DefWindowProcA( hWnd, Msg,wParam, lParam);
}

BOOL
WINAPI
ODeleteFileW(
	LPCWSTR pwsz)
{
	if(FWide())
		return DeleteFileW(pwsz);

	PreConvert();
	LPSTR sz = Convert(pwsz);
	return DeleteFileA(sz);
}

LRESULT
WINAPI
ODialogBoxIndirectParamW(
	HINSTANCE hInstance,
	LPCDLGTEMPLATEW hDialogTemplate,
	HWND hWndParent ,
	DLGPROC lpDialogFunc,
	LPARAM dwInitParam)
{
	if(FWide())
		return DialogBoxIndirectParamW(hInstance, hDialogTemplate, hWndParent ,
			lpDialogFunc, dwInitParam);

	return DialogBoxIndirectParamA(hInstance, hDialogTemplate, hWndParent ,
			lpDialogFunc, dwInitParam);
}

LRESULT
WINAPI
ODialogBoxParamW(
	HINSTANCE hInstance,
	LPCWSTR lpTemplateName,
	HWND hWndParent ,
	DLGPROC lpDialogFunc,
	LPARAM dwInitParam)
{
	if(FWide())
		return DialogBoxParamW(hInstance, lpTemplateName, hWndParent , lpDialogFunc, dwInitParam);

	if(FATOM(lpTemplateName))
		return DialogBoxParamA(hInstance, (LPSTR)lpTemplateName, hWndParent , lpDialogFunc, dwInitParam);

	PreConvert();
	LPSTR sz = Convert(lpTemplateName);
	return DialogBoxParamA(hInstance, sz, hWndParent , lpDialogFunc, dwInitParam);
}

LRESULT
WINAPI
ODispatchMessageW(
	CONST MSG *lpMsg)
{
	if(FWide())
		return DispatchMessageW(lpMsg);

	return DispatchMessageA(lpMsg);
}

int
WINAPI
ODrawTextW(
	HDC hDC,
	LPCWSTR lpString,
	int nCount,
	LPRECT lpRect,
	UINT uFormat)
{
	// NOTE OS may write 3 characters beyond end of lpString so make room!


	if(FWide())
		return DrawTextW(hDC, lpString, nCount, lpRect, uFormat);

	bool fModifyString = (uFormat & DT_MODIFYSTRING) &&
						 (uFormat & (DT_END_ELLIPSIS | DT_PATH_ELLIPSIS));

	const int nBuff = WideCharToMultiByte(CP_ACP, 0, lpString, nCount,
										  NULL, 0, NULL, NULL );
	Assert(0 <= nBuff);

	// OS may write beyond end of buffer so make room!
	const LPSTR sz = SzAlloc(nBuff + 4);

	Verify(nBuff == WideCharToMultiByte(CP_ACP, 0, lpString, nCount,
										sz, nBuff, NULL, NULL ));

	if (fModifyString)
		{
		// DrawTextA doesn't nessacerily '\0' terminate the output,
		// so have termiators ready
		memcpy(sz + nBuff, "\0\0\0\0", 4);
		}

	const int iDrawTextReturn = DrawTextA(hDC, sz, nBuff - 1, lpRect, uFormat);

	// With certain flags, DrawText modifies the string, truncating it with
	// an ellipsis.  We need to convert back and update the string passed to
	// the wrapper before we return.
	if (fModifyString && 0 <= iDrawTextReturn)
		{
		Assert('\0' == sz[nBuff + 3]); // Verify not too many were overwritten

		// The windows function prototype has lpString as constant even
		//    though the string gets modified!
		const int nStringLen = -1 != nCount ? nCount : wcslen(lpString);
		Verify(0 <= AnsiToUnicode(const_cast<LPWSTR>(lpString), sz,
								 nStringLen + 4 ));
		}
	return iDrawTextReturn;
}

// Written by Bill Hiebert on 9/4/97
// Smoke tested by Bill Hiebert 9/4/97
int
WINAPI
ODrawTextExW(HDC hdc, LPWSTR pwsz, int cb, LPRECT lprect, UINT dwDTFormat, LPDRAWTEXTPARAMS lpDTParams)
{
	Assert(-1 != cb);
	Assert(!(DT_MODIFYSTRING & dwDTFormat));

	if(FWide())
		return DrawTextExW(hdc, pwsz, cb, lprect, dwDTFormat, lpDTParams);

	PreConvert();
	LONG  n = 0;
	LPSTR sz = ConvertWithLen(pwsz, cb, &n);

	return DrawTextExA(hdc, sz, n, lprect, dwDTFormat, lpDTParams);
}


// Written for Carlos Gomes on 6/26/97 by Ted Smith
// Smoke tested by Carlos Gomes on 6/26
DWORD
WINAPI
OExpandEnvironmentStringsW(
	LPCWSTR lpSrc,
	LPWSTR lpDst,
	DWORD nSize
	)
{
	if (FWide())
		return ExpandEnvironmentStringsW(lpSrc, lpDst, nSize);

	PreConvert();
	LPSTR szSrc = Convert(lpSrc);
	LPSTR szDst = SzAlloc(sizeof(WCHAR) * nSize);
	DWORD dwRet = ExpandEnvironmentStringsA(szSrc, szDst, sizeof(WCHAR) * nSize);

	if (dwRet)
		{
		LONG lRet = AnsiToUnicode(lpDst, szDst, nSize, min(dwRet, sizeof(WCHAR) * nSize));
		if (dwRet < (DWORD) lRet)
			{
			dwRet = lRet;
			}
		}
	else if (lpDst && 0 < nSize)
		{
		*lpDst = L'\0';
		}

	return dwRet;
}

VOID
WINAPI
OFatalAppExitW(
	UINT uAction,
	LPCWSTR lpMessageText
	)
{
	if(FWide())
		FatalAppExitW(uAction, lpMessageText);

	PreConvert();
	LPSTR sz = Convert(lpMessageText);
	FatalAppExitA(uAction, sz);
}

// From: Mark Ashton on 5/8/97
HANDLE
WINAPI
OFindFirstChangeNotificationW(
	LPCWSTR lpPathName,
	BOOL bWatchSubtree,
	DWORD dwNotifyFilter
	)
{
	if (FWide())
		return FindFirstChangeNotificationW(lpPathName, bWatchSubtree, dwNotifyFilter);

	PreConvert();
	LPSTR sz = Convert(lpPathName);
	return FindFirstChangeNotificationA(sz, bWatchSubtree, dwNotifyFilter);
}

// From: Mark Ashton on 5/8/97
HANDLE
WINAPI
OFindFirstFileW(
	LPCWSTR lpFileName,
	LPWIN32_FIND_DATAW lpFindFileData
	)
{
	if (FWide())
		return FindFirstFileW(lpFileName, lpFindFileData);

	PreConvert();
	LPSTR sz = Convert(lpFileName);
	WIN32_FIND_DATAA findFileData;
	HANDLE h = FindFirstFileA(sz, &findFileData);
	if (INVALID_HANDLE_VALUE != h)
		{
		lpFindFileData->dwFileAttributes    = findFileData.dwFileAttributes;
		lpFindFileData->ftCreationTime      = findFileData.ftCreationTime;
		lpFindFileData->ftLastAccessTime    = findFileData.ftLastAccessTime;
		lpFindFileData->ftLastWriteTime     = findFileData.ftLastWriteTime;
		lpFindFileData->nFileSizeHigh       = findFileData.nFileSizeHigh;
		lpFindFileData->nFileSizeLow        = findFileData.nFileSizeLow;
		lpFindFileData->dwReserved0         = findFileData.dwReserved0;
		lpFindFileData->dwReserved1         = findFileData.dwReserved1;
		Verify(0 <= AnsiToUnicode(lpFindFileData->cFileName, findFileData.cFileName, _MAX_PATH));
		Verify(0 <= AnsiToUnicode(lpFindFileData->cAlternateFileName, findFileData.cAlternateFileName, 14));
		}
	return h;
}

// From: Mark Ashton on 5/8/97
BOOL
WINAPI
OFindNextFileW(
	HANDLE hFindFile,
	LPWIN32_FIND_DATAW lpFindFileData
	)
{
	if (FWide())
		return FindNextFileW(hFindFile, lpFindFileData);

	WIN32_FIND_DATAA findFileData;
	BOOL fFlag = FindNextFileA(hFindFile, &findFileData);
	if (fFlag)
		{
		lpFindFileData->dwFileAttributes    = findFileData.dwFileAttributes;
		lpFindFileData->ftCreationTime      = findFileData.ftCreationTime;
		lpFindFileData->ftLastAccessTime    = findFileData.ftLastAccessTime;
		lpFindFileData->ftLastWriteTime     = findFileData.ftLastWriteTime;
		lpFindFileData->nFileSizeHigh       = findFileData.nFileSizeHigh;
		lpFindFileData->nFileSizeLow        = findFileData.nFileSizeLow;
		lpFindFileData->dwReserved0         = findFileData.dwReserved0;
		lpFindFileData->dwReserved1         = findFileData.dwReserved1;
		Verify(0 <= AnsiToUnicode(lpFindFileData->cFileName, findFileData.cFileName, _MAX_PATH));
		Verify(0 <= AnsiToUnicode(lpFindFileData->cAlternateFileName, findFileData.cAlternateFileName, 14));
		}
	return fFlag;
}

HRSRC
WINAPI
OFindResourceW(
	HINSTANCE hModule,
	LPCWSTR lpName,
	LPCWSTR lpType
	)
{
	if(FWide())
		return FindResourceW(hModule, lpName, lpType);

	LPCSTR szName = (LPCSTR)lpName;
	LPCSTR szType = (LPCSTR)lpType;

	PreConvert();
	if(!FATOM(lpName))
		szName = Convert(lpName);
	if(!FATOM(lpType))
		szType = Convert(lpType);

	return FindResourceA(hModule, szName, szType);
}

HWND
WINAPI
OFindWindowW(
	LPCWSTR lpClassName ,
	LPCWSTR lpWindowName)
{
	if(FWide())
		return FindWindowW(lpClassName , lpWindowName);

	PreConvert();
	LPSTR szClass = Convert(lpClassName);
	LPSTR szWnd   = Convert(lpWindowName);

	return FindWindowA(szClass, szWnd);
}

// Bill Hiebert of IStudio on 6/13/97 added support for the
//   FORMAT_MESSAGE_ALLOCATE_BUFFER flag
// Bill donated a bugfix for 1819 on 8/1/97

DWORD
WINAPI
OFormatMessageW(
	DWORD dwFlags,
	LPCVOID lpSource,
	DWORD dwMessageId,
	DWORD dwLanguageId,
	LPWSTR lpBuffer,
	DWORD nSize,
	va_list *Arguments)
{

	if (FWide())
		return FormatMessageW(dwFlags, lpSource, dwMessageId, dwLanguageId,
							  lpBuffer, nSize, Arguments );

	DWORD dwRet;

	LPSTR szBuffer = NULL;

	if (!(dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER))
		{
		Assert(!IsBadWritePtr(lpBuffer, nSize * sizeof(WCHAR)));
		szBuffer = SzAlloc(sizeof(WCHAR) * nSize);
		}

	if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
		{
		PreConvert();
		LPSTR szSource = Convert((LPWSTR)lpSource);

		if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
			{   // Must pass address of szBuffer
			dwRet = FormatMessageA(dwFlags, szSource, dwMessageId, dwLanguageId,
				(char*)&szBuffer, sizeof(WCHAR) * nSize, Arguments);
			}
		else
			{
			dwRet = FormatMessageA(dwFlags, szSource, dwMessageId, dwLanguageId,
				szBuffer, sizeof(WCHAR) * nSize, Arguments);
			}
		}
	else
		{
		if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
			{   // Must pass address of szBuffer
			dwRet = FormatMessageA(dwFlags, lpSource, dwMessageId, dwLanguageId,
					(char*)&szBuffer, sizeof(WCHAR) * nSize, Arguments);
			}
		else
			{
			dwRet = FormatMessageA(dwFlags, lpSource, dwMessageId, dwLanguageId,
					szBuffer, sizeof(WCHAR) * nSize, Arguments);
			}
		}

	if (dwRet)
		{
		if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
			{ // szBuffer contains LocalAlloc ptr to new string. lpBuffer is a
			  // WCHAR** when FORMAT_MESSAGE_ALLOCATE_BUFFER is defined.
			WCHAR* pTemp = (WCHAR*)LocalAlloc(NONZEROLPTR, (dwRet + 1) * sizeof(WCHAR) );
			dwRet = pTemp == NULL? 0 : AnsiToUnicode(pTemp, szBuffer, dwRet + 1);
			LocalFree(szBuffer);
			if (dwRet)
				{
				*(WCHAR**)lpBuffer = pTemp;
				}
			return dwRet;
			}
		else
			{ // Just convert
			return AnsiToUnicode(lpBuffer, szBuffer, nSize);
			}
		}
	else if (lpBuffer && 0 < nSize)
		{
		*lpBuffer = L'\0';
		}

	return dwRet;
}

BOOL  
APIENTRY
OGetCharABCWidthsFloatW(
	HDC     hdc,
	UINT    uFirstChar,
	UINT    uLastChar,
	LPABCFLOAT      lpABC)
{
	if(FWide())
		return GetCharABCWidthsFloatW(hdc, uFirstChar, uLastChar, lpABC);

	return GetCharABCWidthsFloatA(hdc, uFirstChar, uLastChar, lpABC);
}

BOOL
APIENTRY
OGetCharABCWidthsW(
	HDC hdc,
	UINT uFirstChar,
	UINT uLastChar,
	LPABC lpABC)
{
	if(FWide())
		return GetCharABCWidthsW(hdc, uFirstChar, uLastChar, lpABC);

	return GetCharABCWidthsA(hdc, uFirstChar, uLastChar, lpABC);
}

BOOL
APIENTRY
OGetCharWidthFloatW(
	HDC     hdc,
	UINT    iFirstChar,
	UINT    iLastChar,
	PFLOAT  pBuffer)
{
	if(FWide())
		return GetCharWidthFloatW(hdc, iFirstChar, iLastChar, pBuffer);

	return GetCharWidthFloatA(hdc, iFirstChar, iLastChar, pBuffer);
}


BOOL
WINAPI
OGetCharWidthW(
	HDC hdc,
	UINT iFirstChar,
	UINT iLastChar,
	LPINT lpBuffer)
{
	if(FWide())
		return GetCharWidth32W(hdc, iFirstChar, iLastChar, lpBuffer);

	return GetCharWidth32A(hdc, iFirstChar, iLastChar, lpBuffer);
}

// Static buffers for GetClassInfo[Ex] to return the classname
// and menuname in Unicode, when running on an Ansi system.
// The contract of GetClassInfo is that it returns const ptrs
// back to the class name and menu name.  Unfortuntely, this
// prevents us from translating these back from Ansi to Unicode,
// without having some static buffers to use.  Since we strongly
// believe that the only people calling this are doing it just to
// see if it succeeds or not, so they know whether the class is
// already registered, we've willing to just have one set of
// static buffers to use.
// CAUTION: this will work as long as two threads don't call
// GetClassInfo[Ex] at the same time!
static WCHAR g_szClassName[256];
static WCHAR g_szMenuName[256];

#ifdef DEBUG
static DWORD g_dwCallingThread = 0;    // debug global for ensuring one thread.
#endif // DEBUG

BOOL
WINAPI
OGetClassInfoW
(
HINSTANCE hInstance,
LPCWSTR lpClassName,
LPWNDCLASSW lpWndClass
)
{
	if (FWide())
		return GetClassInfoW(hInstance, lpClassName, lpWndClass);

	PreConvert();
	LPSTR szClassName = Convert(lpClassName);
	BOOL fRet = GetClassInfoA(hInstance, szClassName, (LPWNDCLASSA)lpWndClass);
	if (!fRet)
		{
		return false;
		}

	// if ClassName or MenuName aren't atom's, we need to
	// translate them back to Unicode.  We use our static
	// buffers above.  See note about why and the CAUTION!
#ifdef DEBUG
	if (!g_dwCallingThread)
		g_dwCallingThread = GetCurrentThreadId();
	Assert(GetCurrentThreadId() == g_dwCallingThread);
#endif // DEBUG

	if (!FATOM(lpWndClass->lpszMenuName))
		{
		Assert(strlen((LPCSTR)lpWndClass->lpszMenuName) <
				(sizeof(g_szMenuName)/sizeof(WCHAR)));
		if (!AnsiToUnicode(g_szMenuName, (LPCSTR)lpWndClass->lpszMenuName,
				strlen((LPCSTR)lpWndClass->lpszMenuName)+1))
			{
			return false;
			}
		lpWndClass->lpszMenuName = g_szMenuName;
		}

	if (!FATOM(lpWndClass->lpszClassName))
		{
		Assert(strlen((LPCSTR)lpWndClass->lpszClassName) <
				(sizeof(g_szClassName)/sizeof(WCHAR)));
		if (!AnsiToUnicode(g_szClassName, (LPCSTR)lpWndClass->lpszClassName,
				strlen((LPCSTR)lpWndClass->lpszClassName)+1))
			{
			return false;
			}
		lpWndClass->lpszClassName = g_szClassName;
		}

	return fRet;
}

BOOL
WINAPI
OGetClassInfoExW
(
HINSTANCE hInstance,
LPCWSTR lpClassName,
LPWNDCLASSEXW lpWndClass
)
{
	if (FWide())
		return GetClassInfoExW(hInstance, lpClassName, lpWndClass);

	PreConvert();
	LPSTR szClassName = Convert(lpClassName);
	BOOL fRet = GetClassInfoExA(hInstance, szClassName, (LPWNDCLASSEXA)lpWndClass);
	if (!fRet)
		{
		return false;
		}

	// if ClassName or MenuName aren't atom's, we need to
	// translate them back to Unicode.  We use our static
	// buffers above.  See note about why and the CAUTION!
#ifdef DEBUG
	if (!g_dwCallingThread)
		g_dwCallingThread = GetCurrentThreadId();
	Assert(GetCurrentThreadId() == g_dwCallingThread);
#endif // DEBUG

	if (!FATOM(lpWndClass->lpszMenuName))
		{
		Assert(strlen((LPCSTR)lpWndClass->lpszMenuName) <
				(sizeof(g_szMenuName)/sizeof(WCHAR)));
		if (!AnsiToUnicode(g_szMenuName, (LPCSTR)lpWndClass->lpszMenuName,
				strlen((LPCSTR)lpWndClass->lpszMenuName)+1))
			{
			return false;
			}
		lpWndClass->lpszMenuName = g_szMenuName;
		}

	if (!FATOM(lpWndClass->lpszClassName))
		{
		Assert(strlen((LPCSTR)lpWndClass->lpszClassName) <
				(sizeof(g_szClassName)/sizeof(WCHAR)));
		if (!AnsiToUnicode(g_szClassName, (LPCSTR)lpWndClass->lpszClassName,
				strlen((LPCSTR)lpWndClass->lpszClassName)+1))
			{
			return false;
			}
		lpWndClass->lpszClassName = g_szClassName;
		}

	return fRet;
}

DWORD
WINAPI
OGetClassLongW(
	HWND hWnd,
	int nIndex)
{
	if(FWide())
		return GetClassLongW(hWnd, nIndex);
	return GetClassLongA(hWnd, nIndex);  //$UNDONE_POST_98 Watch out for GCL_MENUNAME, etc!
}

DWORD
WINAPI
OSetClassLongW(
	HWND hWnd,
	int nIndex,
	LONG dwNewLong)
{
	if (FWide())
		return SetClassLongW(hWnd, nIndex, dwNewLong);

	return SetClassLongA(hWnd, nIndex, dwNewLong);  //$UNDONE_POST_98 Watch out for GCL_MENUNAME, etc!

}

int
WINAPI
OGetClassNameW(
	HWND hWnd,
	LPWSTR lpClassName,
	int nMaxCount)
{
	if(FWide())
		return GetClassNameW(hWnd, lpClassName, nMaxCount);

	LPSTR sz = SzAlloc(sizeof(WCHAR) * nMaxCount + 2);
	int nRet = GetClassNameA(hWnd, sz, sizeof(WCHAR) * nMaxCount);

	// $UNDONE_POST_98: This is bogus, we should do this like OLoadStringW
	if (nRet)
		{
		// force null-termination
		sz[sizeof(WCHAR) * nMaxCount] = '\0';
		sz[sizeof(WCHAR) * nMaxCount + 1] = '\0';

		// need a temporary wide string
		LPWSTR wsz = SzWAlloc(2 * nMaxCount + 1);

		nRet = min(AnsiToUnicode(wsz, sz, 2 * nMaxCount + 1), nMaxCount);

		// copy the requested number of characters
		if (lpClassName)
			{
			memcpy(lpClassName, wsz, nRet * sizeof(WCHAR));
			}

		return nRet;
		}

	else if (lpClassName && 0 < nMaxCount)
		{
		*lpClassName = L'\0';
		}

	return nRet;
}

DWORD
WINAPI
OGetCurrentDirectoryW(
	DWORD nBufferLength,
	LPWSTR lpBuffer)
{
	if (FWide())
		return GetCurrentDirectoryW(nBufferLength, lpBuffer);

	LPSTR sz = SzAlloc(sizeof(WCHAR) * nBufferLength);
	DWORD dwRet = GetCurrentDirectoryA(sizeof(WCHAR) * nBufferLength, sz);

	// $UNDONE_POST_98: This is bogus, we should do this like OLoadStringW
	if (dwRet)
		{
		return AnsiToUnicode(lpBuffer, sz, nBufferLength);
		}
	else if (lpBuffer && 0 < nBufferLength)
		{
		*lpBuffer = L'\0';
		}

	return dwRet;
}

UINT
WINAPI
OGetDlgItemTextW(
	HWND hDlg,
	int nIDDlgItem,
	LPWSTR lpString,
	int nMaxCount)
{
	if(FWide())
		return GetDlgItemTextW(hDlg, nIDDlgItem, lpString, nMaxCount);

	LPSTR sz = SzAlloc(sizeof(WCHAR) * nMaxCount);
	UINT uRet = GetDlgItemTextA(hDlg, nIDDlgItem, sz, sizeof(WCHAR) * nMaxCount);

	// $UNDONE_POST_98: This is bogus, we should do this like OLoadStringW
	if(uRet)
		{
		return AnsiToUnicode(lpString, sz, nMaxCount);
		}
	else if (lpString && 0 < nMaxCount)
		{
		*lpString = L'\0';
		}

	return uRet;
}

DWORD
WINAPI
OGetFileAttributesW(
	LPCWSTR lpFileName
	)
{
	if(FWide())
		return GetFileAttributesW(lpFileName);

	PreConvert();
	LPSTR sz = Convert(lpFileName);
	return GetFileAttributesA(sz);
}

DWORD
WINAPI
OGetFullPathNameW(
	LPCWSTR lpFileName,
	DWORD nBufferLength,
	LPWSTR lpBuffer,
	LPWSTR *lpFilePart
	)
{
	if(FWide())
		return GetFullPathNameW(lpFileName, nBufferLength, lpBuffer, lpFilePart);

	PreConvert();
	LPSTR szFile = Convert(lpFileName);
	LPSTR szBuffer = SzAlloc(sizeof(WCHAR) * nBufferLength);
	LPSTR pszFile;

	DWORD dwRet = GetFullPathNameA(szFile ,sizeof(WCHAR) * nBufferLength, szBuffer , &pszFile);

	// $UNDONE_POST_98: This is bogus, we should do this like OLoadStringW
	if(dwRet)
		{
		DWORD dwNoOfChar = AnsiToUnicode(lpBuffer, szBuffer , nBufferLength);
		*pszFile = '\0';
		*lpFilePart = lpBuffer + AnsiToUnicode(NULL, szBuffer, 0);
		return dwNoOfChar;
		}

	return dwRet;
}

DWORD
WINAPI
OGetGlyphOutlineW(
	HDC     hdc,
	UINT    uChar,
	UINT    uFormat,
	LPGLYPHMETRICS      lpgm,
	DWORD       cbBuffer,
	LPVOID      lpvBuffer,
	CONST MAT2 *    lpmat2)
{
	if (FWide())
		return GetGlyphOutlineW(hdc, uChar, uFormat, lpgm, cbBuffer, lpvBuffer, lpmat2);

	return GetGlyphOutlineA(hdc, uChar, uFormat, lpgm, cbBuffer, lpvBuffer, lpmat2);
}

DWORD
WINAPI
OGetKerningPairsW(
	HDC         hdc,
	DWORD       nNumPairs,
	LPKERNINGPAIR       lpkrnpair)
{
	if(FWide())
		return GetKerningPairsW(hdc, nNumPairs, lpkrnpair);

	return GetKerningPairsA(hdc, nNumPairs, lpkrnpair);
}

BOOL
WINAPI
OGetMessageW(
	LPMSG lpMsg,
	HWND hWnd ,
	UINT wMsgFilterMin,
	UINT wMsgFilterMax)
{
	if(FWide())
		return GetMessageW(lpMsg, hWnd , wMsgFilterMin, wMsgFilterMax);

	return GetMessageA(lpMsg, hWnd , wMsgFilterMin, wMsgFilterMax);
}

DWORD
WINAPI
OGetModuleFileNameW(
	HINSTANCE hModule,
	LPWSTR pwszFilename,
	DWORD nSize
	)
{
	if(FWide())
		return GetModuleFileNameW(
			hModule,
			pwszFilename,
			nSize
			);

	LPSTR sz    = SzAlloc(sizeof(WCHAR) * nSize);
	DWORD dwRet = GetModuleFileNameA(hModule, sz, sizeof(WCHAR) * nSize);
	// $UNDONE_POST_98: This is bogus, we should do this like OLoadStringW
	if (dwRet)
		{
		return AnsiToUnicode(pwszFilename, sz, nSize, dwRet + 1);
		}
	else if (pwszFilename && 0 < nSize)
		{
		*pwszFilename = L'\0';
		}

	return dwRet;
}

HMODULE
WINAPI
OGetModuleHandleW(
	LPCWSTR lpModuleName
	)
{
	if(FWide())
		return GetModuleHandleW(lpModuleName);

	PreConvert();
	LPSTR sz = Convert(lpModuleName);

	return GetModuleHandleA(sz);
}

// From: Mark Ashton on 5/8/97
//       Ted Smith: Re-wrote fixing handling of non-trivial parameters on 6/25
// Smoke tested by Mark Ashton on 6/25

APIENTRY
OGetOpenFileNameW
(
LPOPENFILENAMEW lpofn
)
{
	if (FWide())
		return GetOpenFileNameW(lpofn);

	Assert(!(lpofn->Flags & OFN_ENABLEHOOK));

	PreConvert();
	OPENFILENAMEA ofn;
	ofn.lStructSize       = sizeof(OPENFILENAMEA);
	ofn.hwndOwner         = lpofn->hwndOwner;
	ofn.hInstance         = lpofn->hInstance;
	ofn.lpstrFilter       = ConvertWithLen(lpofn->lpstrFilter, cUnicodeMultiSzLen(lpofn->lpstrFilter), &_lJunk);
	ofn.lpstrCustomFilter = ConvertWithLen(lpofn->lpstrCustomFilter, lpofn->nMaxCustFilter, &ofn.nMaxCustFilter);
	ofn.nFilterIndex      = lpofn->nFilterIndex;
	ofn.lpstrFile         = ConvertWithLen(lpofn->lpstrFile, lpofn->nMaxFile, &ofn.nMaxFile);
	ofn.lpstrFileTitle    = ConvertWithLen(lpofn->lpstrFileTitle, lpofn->nMaxFileTitle, &ofn.nMaxFileTitle);
	ofn.lpstrInitialDir   = Convert(lpofn->lpstrInitialDir);
	ofn.lpstrTitle        = Convert(lpofn->lpstrTitle);
	ofn.Flags             = lpofn->Flags;
	ofn.nFileOffset       = lpofn->nFileOffset;
	ofn.nFileExtension    = lpofn->nFileExtension;
	ofn.lpstrDefExt       = Convert(lpofn->lpstrDefExt);
	ofn.lCustData         = lpofn->lCustData;
	ofn.lpfnHook          = NULL;
	ofn.lpTemplateName    = ((lpofn->Flags & OFN_ENABLETEMPLATE) &&
							 !FATOM(lpofn->lpTemplateName) ) ?
							Convert(lpofn->lpTemplateName) :
							(LPSTR)lpofn->lpTemplateName;
	BOOL fFlag = GetOpenFileNameA(&ofn);
	if (fFlag)
		{
		Assert(lpofn->hwndOwner == ofn.hwndOwner);
		Assert(lpofn->hInstance == ofn.hInstance);
		if (ofn.lpstrCustomFilter)
			{
			lpofn->nMaxCustFilter = AnsiToUnicode(lpofn->lpstrCustomFilter, ofn.lpstrCustomFilter, lpofn->nMaxCustFilter, strlen(ofn.lpstrCustomFilter) + 1);
			}
		lpofn->nFilterIndex   = ofn.nFilterIndex;
		Assert(lpofn->nMaxFile == ofn.nMaxFile);
		Verify(0 <= AnsiToUnicode(lpofn->lpstrFile, ofn.lpstrFile, lpofn->nMaxFile, strlen(ofn.lpstrFile) + 1));
		if (ofn.lpstrFileTitle)
			{
			lpofn->nMaxFileTitle = AnsiToUnicode(lpofn->lpstrFileTitle, ofn.lpstrFileTitle, strlen(ofn.lpstrFileTitle) + 1);
			}
		lpofn->Flags          = ofn.Flags;
		lpofn->nFileOffset    = ofn.nFileOffset;
		lpofn->nFileExtension = ofn.nFileExtension;
		Assert(lpofn->lCustData == ofn.lCustData);
		Assert(lpofn->lpfnHook == ofn.lpfnHook);
		}
	else if (lpofn->lpstrFile)
		{   // if buffer too small first 2 bytes are the required size
		memcpy(lpofn->lpstrFile, ofn.lpstrFile, sizeof(short));
		}
	return fFlag;
}

UINT
APIENTRY
OGetOutlineTextMetricsW(
	HDC hdc,
	UINT cbData,
	LPOUTLINETEXTMETRICW lpOTM)
{
	// *** TextMetrics defines BYTE elements in the structure for the
	// value of first first/last character defined in the font.
	// Problem for DBCS.

	if(FWide())
		return GetOutlineTextMetricsW(hdc, cbData, lpOTM);

	return GetOutlineTextMetricsA(hdc, cbData, (LPOUTLINETEXTMETRICA)lpOTM); //$ UNDONE_POST_98 - This doesn't convert the embedded Names...
}

UINT
WINAPI
OGetPrivateProfileIntW(
	LPCWSTR lpAppName,
	LPCWSTR lpKeyName,
	INT nDefault,
	LPCWSTR lpFileName)
{
	if(FWide())
		return GetPrivateProfileIntW(lpAppName, lpKeyName, nDefault, lpFileName);

	PreConvert();
	LPSTR szAppName  = Convert(lpAppName);
	LPSTR szKeyName  = Convert(lpKeyName);
	LPSTR szFileName = Convert(lpFileName);

	return GetPrivateProfileIntA(szAppName, szKeyName, nDefault, szFileName);
}

DWORD
WINAPI
OGetPrivateProfileStringW(
	LPCWSTR lpAppName,
	LPCWSTR lpKeyName,
	LPCWSTR lpDefault,
	LPWSTR lpReturnedString,
	DWORD nSize,
	LPCWSTR lpFileName)
{
	if(FWide())
		return GetPrivateProfileStringW(lpAppName, lpKeyName, lpDefault, lpReturnedString,
		  nSize, lpFileName);

	PreConvert();
	LPSTR szAppName  = Convert(lpAppName);
	LPSTR szKeyName  = Convert(lpKeyName);
	LPSTR szDefault  = Convert(lpDefault);
	LPSTR szFileName = Convert(lpFileName);

	LPSTR szReturnedString = SzAlloc(sizeof(WCHAR) * nSize);

	DWORD dwRet = GetPrivateProfileStringA(szAppName, szKeyName, szDefault, szReturnedString,
	  sizeof(WCHAR) * nSize, szFileName);

	// I hope this doesn't fail because there's no clear failure value in the docs

	DWORD dwNoOfChar = AnsiToUnicode(lpReturnedString, szReturnedString, nSize);

	if (dwNoOfChar)
		return dwRet;
	else
		{
		LPWSTR lpTempString = SzWAlloc(sizeof(WCHAR) * nSize);
		if (AnsiToUnicode(lpTempString, szReturnedString, sizeof(WCHAR) * nSize))
			{
			if (lpAppName && lpKeyName)
				{
				lpTempString[nSize - 1] = L'\0';
				wcsncpy(lpReturnedString, lpTempString, nSize);
				return nSize - 1;
				}
			else
				{
				lpTempString[nSize - 1] = L'\0';
				lpTempString[nSize - 2] = L'\0';
				wcsncpy(lpReturnedString, lpTempString, nSize);
				return nSize - 2;
				}
			}
		}

	return dwRet;
}


int
WINAPI
OGetObjectW(
	HGDIOBJ hgdiobj,
	int cbBuffer,
	LPVOID lpvObject)
{
	if(FWide())
		return GetObjectW(hgdiobj, cbBuffer, lpvObject);

	DWORD dwObj = GetObjectType(hgdiobj);
	if (OBJ_FONT == dwObj)
		{
		//$CONSIDER: This effects all getobject call, performance?
		Assert(cbBuffer == sizeof(LOGFONTW));
		LOGFONTA  lfa;
		LOGFONTW *plfw = (LOGFONTW *)lpvObject;

		int nRet = GetObjectA(hgdiobj, sizeof(lfa), &lfa);

		if(nRet)
			{
			memcpy(plfw, &lfa, sizeof(LOGFONTA));
			Verify(0 <= AnsiToUnicode(plfw->lfFaceName, lfa.lfFaceName, LF_FACESIZE));
			}

		return nRet;
		}
	else
		{
		return GetObjectA(hgdiobj, cbBuffer, lpvObject);
		}
}

UINT
WINAPI
OGetProfileIntW(
	LPCWSTR lpAppName,
	LPCWSTR lpKeyName,
	INT nDefault
	)
{
	if(FWide())
		return GetProfileIntW(lpAppName, lpKeyName, nDefault);

	PreConvert();
	LPSTR szApp = Convert(lpAppName);
	LPSTR szKey = Convert(lpKeyName);

	return GetProfileIntA(szApp, szKey, nDefault);
}

HANDLE
WINAPI
OGetPropW(
	HWND hWnd,
	LPCWSTR lpString)
{
	if(FWide())
		return GetPropW(hWnd, lpString);

	if(FATOM(lpString))
		return GetPropA(hWnd, (LPSTR)lpString);

	PreConvert();
	LPSTR sz = Convert(lpString);
	return GetPropA(hWnd, sz);
}

// From: Mark Ashton on 5/29/97
//       Ted Smith: Re-wrote fixing handling of non-trivial parameters on 6/25
// Smoke tested by Mark Ashton on 6/25

APIENTRY
OGetSaveFileNameW(LPOPENFILENAMEW lpofn)
{
	if (FWide())
		return GetSaveFileNameW(lpofn);

	PreConvert();
	Assert(!(lpofn->Flags & OFN_ENABLEHOOK));

	OPENFILENAMEA ofn;
	ofn.lStructSize       = sizeof(OPENFILENAMEA);
	ofn.hwndOwner         = lpofn->hwndOwner;
	ofn.hInstance         = lpofn->hInstance;
	ofn.lpstrFilter       = ConvertWithLen(lpofn->lpstrFilter, cUnicodeMultiSzLen(lpofn->lpstrFilter), &_lJunk);
	ofn.lpstrCustomFilter = ConvertWithLen(lpofn->lpstrCustomFilter, lpofn->nMaxCustFilter, &ofn.nMaxCustFilter);
	ofn.nFilterIndex      = lpofn->nFilterIndex;
	ofn.lpstrFile         = ConvertWithLen(lpofn->lpstrFile, lpofn->nMaxFile, &ofn.nMaxFile);
	ofn.lpstrFileTitle    = ConvertWithLen(lpofn->lpstrFileTitle, lpofn->nMaxFileTitle, &ofn.nMaxFileTitle);
	ofn.lpstrInitialDir   = Convert(lpofn->lpstrInitialDir);
	ofn.lpstrTitle        = Convert(lpofn->lpstrTitle);
	ofn.Flags             = lpofn->Flags;
	ofn.nFileOffset       = lpofn->nFileOffset;
	ofn.nFileExtension    = lpofn->nFileExtension;
	ofn.lpstrDefExt       = Convert(lpofn->lpstrDefExt);
	ofn.lCustData         = lpofn->lCustData;
	ofn.lpfnHook          = NULL;
	ofn.lpTemplateName    = ((lpofn->Flags & OFN_ENABLETEMPLATE) &&
							 !FATOM(lpofn->lpTemplateName) ) ?
							Convert(lpofn->lpTemplateName) :
							(LPSTR)lpofn->lpTemplateName;
	BOOL fFlag = GetSaveFileNameA(&ofn);
	if (fFlag)
		{
		Assert(lpofn->hwndOwner == ofn.hwndOwner);
		Assert(lpofn->hInstance == ofn.hInstance);
		if (ofn.lpstrCustomFilter)
			{
			lpofn->nMaxCustFilter = AnsiToUnicode(lpofn->lpstrCustomFilter, ofn.lpstrCustomFilter, lpofn->nMaxCustFilter, ofn.nMaxCustFilter);
			}
		lpofn->nFilterIndex   = ofn.nFilterIndex;
		Assert(lpofn->nMaxFile == ofn.nMaxFile);
		Verify(0 <= AnsiToUnicode(lpofn->lpstrFile, ofn.lpstrFile, lpofn->nMaxFile, ofn.nMaxFile));
		if (ofn.lpstrFileTitle)
			{
			lpofn->nMaxFileTitle = AnsiToUnicode(lpofn->lpstrFileTitle, ofn.lpstrFileTitle, lpofn->nMaxFileTitle);
			}
		lpofn->Flags          = ofn.Flags;
		lpofn->nFileOffset    = ofn.nFileOffset;
		lpofn->nFileExtension = ofn.nFileExtension;
		Assert(lpofn->lCustData == ofn.lCustData);
		Assert(lpofn->lpfnHook == ofn.lpfnHook);
		}
	else if (lpofn->lpstrFile)
		{   // if buffer too small first 2 bytes are the required size
		memcpy(lpofn->lpstrFile, ofn.lpstrFile, sizeof(short));
		}
	return fFlag;
}

DWORD
WINAPI
OGetTabbedTextExtentW(
	HDC hDC,
	LPCWSTR lpString,
	int nCount,
	int nTabPositions,
	LPINT lpnTabStopPositions)
{
	Assert(-1 != nCount);

	if(FWide())
		return GetTabbedTextExtentW(hDC, lpString, nCount, nTabPositions, lpnTabStopPositions);

	PreConvert();
	LONG  n = 0;
	LPSTR sz = ConvertWithLen(lpString, nCount, &n);

	return GetTabbedTextExtentA(hDC, sz, n, nTabPositions, lpnTabStopPositions);
}

// From: Mark Ashton on 5/8/97
UINT
WINAPI
OGetTempFileNameW(
	LPCWSTR lpPathName,
	LPCWSTR lpPrefixString,
	UINT uUnique,
	LPWSTR lpTempFileName
	)
{
	if (FWide())
		return GetTempFileNameW(lpPathName, lpPrefixString, uUnique, lpTempFileName);

	char szPathName[_MAX_PATH];
	Verify(0 <= UnicodeToAnsi(szPathName, lpPathName, _MAX_PATH));

	char szPrefixString[_MAX_PATH];
	Verify(0 <= UnicodeToAnsi(szPrefixString, lpPrefixString, _MAX_PATH));

	char szTempFilename[_MAX_PATH];
	UINT dwRet = GetTempFileNameA(szPathName, szPrefixString, uUnique, szTempFilename);
	if (dwRet)
		{
		Verify(0 <= AnsiToUnicode(lpTempFileName, szTempFilename, _MAX_PATH));
		}
	return dwRet;
}

// From: Mark Ashton on 5/8/97
DWORD
WINAPI
OGetTempPathW(
	DWORD nBufferLength,
	LPWSTR lpBuffer
	)
{
	if (FWide())
		return GetTempPathW(nBufferLength, lpBuffer);

	char szPath[_MAX_PATH];
	DWORD dwRet = GetTempPathA(_MAX_PATH, szPath);
	if (dwRet)
		{
		Verify(0 <= AnsiToUnicode(lpBuffer, szPath, nBufferLength));
		}
	return dwRet;
}

BOOL
APIENTRY
OGetTextExtentPoint32W(
					HDC hdc,
					LPCWSTR pwsz,
					int cb,
					LPSIZE pSize
					)
{
	Assert(-1 != cb);

	if(FWide())
		return GetTextExtentPoint32W(hdc, pwsz, cb, pSize);

	PreConvert();
	LONG  n = 0;
	LPSTR sz = ConvertWithLen(pwsz, cb, &n);

	return GetTextExtentPoint32A(hdc, sz, n, pSize);
}

BOOL
APIENTRY
OGetTextExtentPointW(
					HDC hdc,
					LPCWSTR pwsz,
					int cb,
					LPSIZE pSize
					)
{
	Assert(-1 != cb);

	if(FWide())
		return GetTextExtentPointW(hdc, pwsz, cb, pSize);

	PreConvert();
	LONG  n = 0;
	LPSTR sz = ConvertWithLen(pwsz, cb, &n);
	return GetTextExtentPointA(hdc, sz, n, pSize);
}

BOOL
APIENTRY OGetTextExtentExPointW(
					HDC hdc,
					LPCWSTR lpszStr,
					int cchString,
					int nMaxExtent,
					LPINT lpnFit,
					LPINT alpDx,
					LPSIZE pSize
					)
{
	Assert(-1 != cchString);

	if(FWide())
		return GetTextExtentExPointW(hdc, lpszStr, cchString,
									 nMaxExtent, lpnFit, alpDx, pSize);

	PreConvert();
	LONG  n = 0;
	LPSTR sz = ConvertWithLen(lpszStr, cchString, &n);
	return GetTextExtentExPointA(hdc, sz, n, nMaxExtent, lpnFit, alpDx, pSize);

}

LONG
WINAPI
OGetWindowLongW(
	HWND hWnd,
	int nIndex)
{
	if(FWide())
		return GetWindowLongW(hWnd, nIndex);

	return GetWindowLongA(hWnd, nIndex);
}

BOOL
WINAPI
OGetTextMetricsW(
	HDC hdc,
	LPTEXTMETRICW lptm)
{
	if(FWide())
		return GetTextMetricsW(hdc, lptm);

	TEXTMETRICA tma;

	memcpy(&tma, lptm, OffsetOf(TEXTMETRIC, tmFirstChar));

	// tmFirstChar is defined as BYTE.
	// $CONSIDER : will fail for DBCS !!

	wctomb((LPSTR)&tma.tmFirstChar, lptm->tmFirstChar);
	wctomb((LPSTR)&tma.tmLastChar, lptm->tmLastChar);
	wctomb((LPSTR)&tma.tmDefaultChar, lptm->tmDefaultChar);
	wctomb((LPSTR)&tma.tmBreakChar, lptm->tmBreakChar);

	memcpy(&tma.tmItalic, &lptm->tmItalic, sizeof(TEXTMETRIC) - OffsetOf(TEXTMETRIC, tmItalic));

	BOOL fRet = GetTextMetricsA(hdc, &tma);

	if(fRet)
		{
		memcpy(&lptm->tmItalic, &tma.tmItalic, sizeof(TEXTMETRIC) - OffsetOf(TEXTMETRIC, tmItalic));

		// Convert tma.tmFirstChar (1 byte char) to lptm->tmFirstChar
		mbtowc(&lptm->tmFirstChar, (LPSTR)&tma.tmFirstChar, 1);
		mbtowc(&lptm->tmLastChar, (LPSTR)&tma.tmLastChar, 1);
		mbtowc(&lptm->tmDefaultChar, (LPSTR)&tma.tmDefaultChar, 1);
		mbtowc(&lptm->tmBreakChar, (LPSTR)&tma.tmBreakChar, 1);

		memcpy(lptm, &tma, OffsetOf(TEXTMETRIC, tmFirstChar));
		}

	return fRet;
}

// From: Mark Ashton on 5/8/97
BOOL
WINAPI
OGetUserNameW (
	LPWSTR lpBuffer,
	LPDWORD nSize
	)
{
	if (FWide())
		return GetUserNameW(lpBuffer, nSize);

	DWORD dwLen = *nSize;
	LPSTR sz = SzAlloc(dwLen);

	BOOL fFlag = GetUserNameA(sz, nSize);
	if (fFlag)
		{
		*nSize = AnsiToUnicode(lpBuffer, sz, dwLen);
		}
	return fFlag;
}

BOOL
WINAPI
OGetVolumeInformationW(
	LPCWSTR lpRootPathName,
	LPWSTR lpVolumeNameBuffer,
	DWORD nVolumeNameSize,
	LPDWORD lpVolumeSerialNumber,
	LPDWORD lpMaximumComponentLength,
	LPDWORD lpFileSystemFlags,
	LPWSTR lpFileSystemNameBuffer,
	DWORD nFileSystemNameSize
	)
{
	if(FWide())
		return GetVolumeInformationW(lpRootPathName, lpVolumeNameBuffer, nVolumeNameSize, lpVolumeSerialNumber,
			lpMaximumComponentLength, lpFileSystemFlags, lpFileSystemNameBuffer, nFileSystemNameSize);

	PreConvert();
	LPSTR szRoot = Convert(lpRootPathName);
	LPSTR szName = SzAlloc(sizeof(WCHAR) * nVolumeNameSize);
	LPSTR szSysName = SzAlloc(sizeof(WCHAR) * nFileSystemNameSize);

	BOOL fRet = GetVolumeInformationA(szRoot, szName, sizeof(WCHAR) * nVolumeNameSize, lpVolumeSerialNumber,
			lpMaximumComponentLength, lpFileSystemFlags, szSysName, sizeof(WCHAR) * nFileSystemNameSize);

	if(fRet)
		{
		if (!AnsiToUnicode(lpVolumeNameBuffer, szName, nVolumeNameSize) ||
			!AnsiToUnicode(lpFileSystemNameBuffer, szSysName, nFileSystemNameSize))
			{
			fRet = false;
			}
		}
	if (!fRet)
		{
		if (lpVolumeNameBuffer && 0 < nVolumeNameSize)
			{
			*lpVolumeNameBuffer = L'\0';
			}

		if (lpFileSystemNameBuffer && 0 < nFileSystemNameSize)
			{
			*lpFileSystemNameBuffer = L'\0';
			}
		}

	return fRet;
}

int
WINAPI
OGetWindowTextLengthW(
	HWND hWnd)
{
	if(FWide())
		return GetWindowTextLengthW(hWnd);

	return GetWindowTextLengthA(hWnd);
}

int
WINAPI
OGetWindowTextW(
	HWND hWnd,
	LPWSTR lpString,
	int nMaxCount)
{

	/*******  Blackbox Testing results for GetWindowText Win32 API ******

	TestCase    lpString    nMaxCount   Return Value    *lpString modified
	======================================================================
	Testing GetWindowTextW on WinNT :-
		A       not NULL        0           0               No
		B           NULL        0           0               No
		C           NULL    not 0           0               No
		D       not NULL    not 0       # of chars w/o      Yes
										\0 terminator

	Testing GetWindowTextA on Win95 :-
		A       not NULL        0           0               Yes
		B           NULL        0               GPF!!
		C           NULL    not 0               GPF!!
		D       not NULL    not 0       # of chars w/o      Yes
										\0 terminator
	*********************************************************************/

	if(FWide())
		return GetWindowTextW(hWnd, lpString, nMaxCount);

	LPSTR sz = SzAlloc(sizeof(WCHAR) * nMaxCount);
	int nRet = GetWindowTextA(hWnd, sz, sizeof(WCHAR) * nMaxCount);
	// $UNDONE_POST_98: This is bogus, we should do this like OLoadStringW
	if(nRet)
		{
		return AnsiToUnicode(lpString, sz, nMaxCount);
		}
	else
		{
		// GetWindowText() returns 0 when you call it on a window which
		// has no text (e.g. edit control without any text). It also initializes
		// the buffer passed in to receive the text to "\0". So we should initialize
		// the buffer passed in before returning.
		if (lpString && 0 < nMaxCount)
			{
			*lpString = L'\0';
			}
		}

	return nRet;
}

ATOM
WINAPI
OGlobalAddAtomW(
	LPCWSTR lpString
	)
{
	if(FWide())
		return GlobalAddAtomW(lpString);

	PreConvert();
	LPSTR sz = Convert(lpString);
	return GlobalAddAtomA(sz);
}

// From: Josh Kaplan on 8/12/97
UINT
WINAPI
OGlobalGetAtomNameW(
	ATOM nAtom,
	LPWSTR lpBuffer,
	int nSize
	)
{
	if(FWide())
		return GlobalGetAtomNameW(nAtom, lpBuffer, nSize);

	LPSTR sz = SzAlloc(sizeof(WCHAR) * nSize);
	if (GlobalGetAtomNameA(nAtom, sz, sizeof(WCHAR) * nSize))
		{
		// $UNDONE_POST_98: This is bogus, we should do this like OLoadStringW
		return AnsiToUnicode(lpBuffer, sz, nSize) - 1;
		}

	if (lpBuffer && 0 < nSize)
		{
		*lpBuffer = L'\0';
		}
	return 0;
}

BOOL
WINAPI
OGrayStringW(
	HDC hDC,
	HBRUSH hBrush,
	GRAYSTRINGPROC lpOutputFunc,
	LPARAM lpData,
	int nCount,
	int X,
	int Y,
	int nWidth,
	int nHeight)
{
	if(FWide())
		return GrayStringW(hDC, hBrush, lpOutputFunc, lpData, nCount, X, Y, nWidth, nHeight);

	if (!lpOutputFunc)
		{
		PreConvert();
		LPSTR szData = Convert((LPCWSTR) lpData);
		return GrayStringA(hDC, hBrush, lpOutputFunc, (LPARAM) szData, nCount, X, Y, nWidth, nHeight);
		}

	return GrayStringA(hDC, hBrush, lpOutputFunc, lpData, nCount, X, Y, nWidth, nHeight);
}

BOOL
WINAPI
OInsertMenuW(
	HMENU hMenu,
	UINT uPosition,
	UINT uFlags,
	UINT uIDNewItem,
	LPCWSTR lpNewItem
	)
{
	if(FWide())
		return InsertMenuW(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);

	if(uFlags & (MF_BITMAP | MF_OWNERDRAW))
		return InsertMenuA(hMenu, uPosition, uFlags, uIDNewItem, (LPSTR)lpNewItem);

	PreConvert();
	LPSTR sz = Convert(lpNewItem);
	return InsertMenuA(hMenu, uPosition, uFlags, uIDNewItem, sz);
}

BOOL
WINAPI
OIsBadStringPtrW(
	LPCWSTR lpsz,
	UINT ucchMax
	)
{
	if(FWide())
		return IsBadStringPtrW(lpsz, ucchMax);

	return IsBadStringPtrA((LPSTR) lpsz, ucchMax * sizeof(WCHAR));  //$UNDONE_POST_98 - We should use IsBadReadPtr(strlen)!
}


BOOL
WINAPI
OIsCharAlphaNumericW(
	WCHAR wch)
{
	if(FWide())
		return IsCharAlphaNumericW(wch);

	//$CONSIDER: we really want to use MB_CUR_MAX, but that is
	// not a defined constant
	CHAR psz[4];

	int cch = WideCharToMultiByte(CP_ACP, 0, &wch, 1, (CHAR *) psz, 4, NULL, NULL);
	if (1 == cch)
		{
		return IsCharAlphaNumericA(*psz);
		}
	else if (1 < cch)
		{
		// It's a multi-byte character, so treat it as alpha
		// Note: we are not sure that this is entirely correct
		return true;
		}
	else
		{
		return false;
		}
}

BOOL
WINAPI
OIsCharAlphaW(
	WCHAR wch)
{
	if(FWide())
		return IsCharAlphaW(wch);

	//$CONSIDER: we really want to use MB_CUR_MAX, but that is
	// not a defined constant
	CHAR psz[4];

	int cch = WideCharToMultiByte(CP_ACP, 0, &wch, 1, (CHAR *) psz, 4, NULL, NULL);
	if(1 == cch)
		{
		return IsCharAlphaA(*psz);
		}
	else if (1 < cch)
		{
		// It's a multi-byte character, so treat it as alpha
		// Note: we are not sure that this is entirely correct
		return true;
		}
	else
		{
		return false;
		}
}

BOOL
WINAPI
OIsDialogMessageW(
	HWND hDlg,
	LPMSG lpMsg)
{
	// WARNING!!!
	// Bug #6488. We have run into problems due to using IsDialogMessageW on
	// WinNT Japanese. The fix for the bug was calling ANSI version of
	// IsDialogMessage irrespective of whether we are running on NT or Win95.
	// The shell is compiled MBCS (not UNICODE) and they are always using the
	// ANSI versions of the routines. lpMsg passed by shell contains MBCS
	// characters & not UNICODE. So in cases where you get the message
	// structure from the Shell, you will have to call the IsDialogMessageA
	// directly and not use this wrapper.

	if(FWide())
		return IsDialogMessageW(hDlg, lpMsg);

	return IsDialogMessageA(hDlg, lpMsg);
}

// From: Mark Ashton on 5/8/97
//		 Bill Hieber - 2/5/98 fixed buffer size problem.
int
WINAPI
OLCMapStringW(
	LCID     Locale,
	DWORD    dwMapFlags,
	LPCWSTR lpSrcStr,
	int      cchSrc,
	LPWSTR  lpDestStr,
	int      cchDest)
{
	if (FWide())
		return LCMapStringW(Locale, dwMapFlags, lpSrcStr, cchSrc, lpDestStr, cchDest);

	// lpSrcStr is not required to be '\0' terminated. Note that we don't support -1!
	Assert(cchSrc != -1);
	LPSTR sz = SzAlloc(cchSrc * 2);
	int dw = WideCharToMultiByte(CP_ACP, 0, lpSrcStr, cchSrc, sz, cchSrc * 2, NULL, NULL);

	LPSTR dst = cchDest ? SzAlloc(cchDest*2) : NULL;
	int dwRet = LCMapStringA(Locale, dwMapFlags, sz, dw, dst, cchDest*2);
	if (dwRet && cchDest)
		{
		dwRet = MultiByteToWideChar(CP_ACP, 0, dst, dwRet, lpDestStr, cchDest);
		}
	return dwRet;
}

HACCEL
WINAPI
OLoadAcceleratorsW(
	HINSTANCE hInst,
	LPCWSTR   lpTableName)
{
	if(FWide())
		return LoadAcceleratorsW(hInst, lpTableName);

	if(FATOM(lpTableName))
		return LoadAcceleratorsA(hInst, (LPSTR)lpTableName);

	PreConvert();
	LPSTR sz = Convert(lpTableName);
	return LoadAcceleratorsA(hInst, sz);
}

HBITMAP
WINAPI
OLoadBitmapW(
	HINSTANCE hInstance,
	LPCWSTR lpBitmapName)
{
	if(FWide())
		return LoadBitmapW(hInstance, lpBitmapName);

	if(FATOM(lpBitmapName))
		return LoadBitmapA(hInstance, (LPSTR)lpBitmapName);

	PreConvert();
	LPSTR sz = Convert(lpBitmapName);
	return LoadBitmapA(hInstance, sz);
}

HCURSOR
WINAPI
OLoadCursorW(
	HINSTANCE hInstance,
	LPCWSTR lpCursorName)
{
	if(FWide())
		return LoadCursorW(
			hInstance,
			lpCursorName);

	if (FATOM(lpCursorName))
		return LoadCursorA(hInstance, (LPSTR) lpCursorName);

	PreConvert();
	LPSTR sz = Convert(lpCursorName);
	return LoadCursorA(hInstance, sz);
}

HICON
WINAPI
OLoadIconW(
	HINSTANCE hInstance,
	LPCWSTR lpIconName)
{
	if(FWide())
		return LoadIconW(hInstance, lpIconName);

	if(FATOM(lpIconName))
		return LoadIconA(hInstance, (LPSTR)lpIconName);

	PreConvert();
	LPSTR sz = Convert(lpIconName);
	return LoadIconA(hInstance, sz);
}

HINSTANCE
WINAPI
OLoadLibraryW(
	LPCWSTR pwszFileName
	)
{
	if(FWide())
		return LoadLibraryW(pwszFileName);

	PreConvert();
	LPSTR sz = Convert(pwszFileName);
	return LoadLibraryA(sz);
}

HMODULE
WINAPI
OLoadLibraryExW(
	LPCWSTR lpLibFileName,
	HANDLE hFile,
	DWORD dwFlags
	)
{
	if(FWide())
		return LoadLibraryExW(lpLibFileName, hFile, dwFlags);

	PreConvert();
	LPSTR sz = Convert(lpLibFileName);
	return LoadLibraryExA(sz, hFile, dwFlags);
}

HMENU
WINAPI
OLoadMenuIndirectW(
	CONST MENUTEMPLATEW *lpMenuTemplate)
{
	if(FWide())
		return LoadMenuIndirectW(lpMenuTemplate);

	//$NOTE: For both the ANSI and the Unicode version of this function,
	//the strings in the MENUITEMTEMPLATE structure must be Unicode strings

	return LoadMenuIndirectA(lpMenuTemplate);
}

HMENU
WINAPI
OLoadMenuW(
	HINSTANCE hInstance,
	LPCWSTR lpMenuName)
{
	if(FWide())
		return LoadMenuW(hInstance, lpMenuName);

	if(FATOM(lpMenuName))
		return LoadMenuA(hInstance, (LPCSTR)lpMenuName);

	PreConvert();
	LPSTR sz = Convert(lpMenuName);
	return LoadMenuA(hInstance, sz);
}

int
WINAPI
OLoadStringW(
	HINSTANCE hInstance,
	UINT uID,
	LPWSTR lpBuffer,
	int nBufferMax)
{
	if(FWide())
		return LoadStringW(hInstance, uID,  lpBuffer, nBufferMax);

	LPSTR sz = SzAlloc(sizeof(WCHAR) * nBufferMax);
	int nRet = LoadStringA(hInstance, uID, sz, sizeof(WCHAR) * nBufferMax);

	if (!nRet)
		{
		if (lpBuffer && 0 < nBufferMax)
			{
			*lpBuffer = L'\0';
			}
		return 0;
		}

	LONG lRet = AnsiToUnicode(lpBuffer, sz, nBufferMax, nRet + 1); // '\0'
	if (lRet)
		{
		return lRet - 1;
		}

	LPWSTR szBuff = SzWAlloc(nRet + 1);
	lRet = AnsiToUnicode(szBuff, sz, nRet + 1, nRet + 1);
	Assert(lRet);
	memcpy(lpBuffer, szBuff, sizeof(WCHAR) * nBufferMax);
	lpBuffer[nBufferMax - 1] = L'\0';
	return nBufferMax - 1;
}

LPWSTR
WINAPI
OlstrcatW(
	LPWSTR lpString1,
	LPCWSTR lpString2
	)
{
	if (!lpString1 || !lpString2)
		return lpString1;

	return wcscat(lpString1, lpString2);
}

int
WINAPI
OlstrcmpiW(
	LPCWSTR lpString1,
	LPCWSTR lpString2
	)
{
	if(FWide())
		return lstrcmpiW(lpString1, lpString2);

	PreConvert();
	LPSTR psz1 = lpString1 ? Convert(lpString1) : NULL;
	LPSTR psz2 = lpString2 ? Convert(lpString2) : NULL;

	return lstrcmpiA(psz1, psz2);
}

int
WINAPI
OlstrcmpW(
	LPCWSTR lpString1,
	LPCWSTR lpString2
	)
{
	if(FWide())
		return lstrcmpW(lpString1, lpString2);

	PreConvert();
	LPSTR psz1 = lpString1 ? Convert(lpString1) : NULL;
	LPSTR psz2 = lpString2 ? Convert(lpString2) : NULL;

	return lstrcmpA(psz1, psz2);
}

LPWSTR
WINAPI
OlstrcpyW(
	LPWSTR lpString1,
	LPCWSTR lpString2
	)
{
	if (!lpString1)
		return lpString1;

	if (!lpString2)
		lpString2 = L"";

	return wcscpy(lpString1, lpString2);
}

// From: Mark Ashton on 5/8/97
//       Ted Smith added null string pointer handling
LPWSTR
WINAPI
OlstrcpynW(
	LPWSTR lpString1,
	LPCWSTR lpString2,
	int iMaxLength
	)
{
	if (!lpString1)
		{
		return lpString1;
		}

	if (!lpString2)
		{
		lpString2 = L"";
		}

	if(FWide())
		return lstrcpynW(lpString1, lpString2, iMaxLength);

	lpString1[--iMaxLength] = L'\0';
	return wcsncpy(lpString1, lpString2, iMaxLength);
}

int
WINAPI
OlstrlenW(
	LPCWSTR lpString
	)
{
	return lpString ? wcslen(lpString) : 0;
}

UINT
WINAPI
OMapVirtualKeyW(
	UINT uCode,
	UINT uMapType)
{
	// The only person using this so far is using uMapType == 0
	Assert(2 != uMapType);
	if (FWide())
		return MapVirtualKeyW(uCode, uMapType);
	return MapVirtualKeyA(uCode, uMapType);
}

int
WINAPI
OMessageBoxW(
	HWND hWnd ,
	LPCWSTR lpText,
	LPCWSTR lpCaption,
	UINT uType)
{
	if(FWide())
		return MessageBoxW(hWnd, lpText, lpCaption, uType);

	PreConvert();
	LPSTR szText = Convert(lpText);
	LPSTR szCap  = Convert(lpCaption);

	return MessageBoxA(hWnd, szText, szCap, uType);
}

int
WINAPI
OMessageBoxIndirectW(
	LPMSGBOXPARAMSW lpmbp)
{
	Assert(!IsBadWritePtr((void*)lpmbp, sizeof MSGBOXPARAMSW));
	Assert(sizeof MSGBOXPARAMSW == lpmbp->cbSize);
	Assert(sizeof MSGBOXPARAMSW == sizeof MSGBOXPARAMSA);

	if(FWide())
		return MessageBoxIndirectW(lpmbp);

	PreConvert();

	MSGBOXPARAMSA mbpa;
	memcpy(&mbpa, lpmbp, sizeof MSGBOXPARAMSA);

	if (!FATOM(lpmbp->lpszText))
		{
		mbpa.lpszText = Convert(lpmbp->lpszText);
		}
	if (!FATOM(lpmbp->lpszCaption))
		{
		mbpa.lpszCaption = Convert(lpmbp->lpszCaption);
		}
	if ((lpmbp->dwStyle & MB_USERICON) && !FATOM(lpmbp->lpszIcon))
		{
		mbpa.lpszIcon = Convert(lpmbp->lpszIcon);
		}

	return MessageBoxIndirectA(&mbpa);
}

BOOL
WINAPI
OModifyMenuW(
	HMENU hMnu,
	UINT uPosition,
	UINT uFlags,
	UINT uIDNewItem,
	LPCWSTR lpNewItem
	)
{
	if(FWide())
		return ModifyMenuW(hMnu, uPosition, uFlags, uIDNewItem, lpNewItem);

	if (MF_STRING == uFlags)
		{
		PreConvert();
		LPSTR sz = Convert(lpNewItem);
		return ModifyMenuA(hMnu, uPosition, uFlags, uIDNewItem, sz);
		}
	else
		return ModifyMenuA(hMnu, uPosition, uFlags, uIDNewItem, (LPSTR) lpNewItem);

}


// From: Mark Ashton on 5/29/97
BOOL
WINAPI
OMoveFileExW(
	LPCWSTR lpExistingFileName,
	LPCWSTR lpNewFileName,
	DWORD dwFlags
	)
{
	if (FWide())
		return MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);

	PreConvert();
	LPSTR szOld = Convert(lpExistingFileName);
	LPSTR szNew = Convert(lpNewFileName);

	return MoveFileExA(szOld, szNew, dwFlags);
}

BOOL
WINAPI
OMoveFileW(
	LPCWSTR lpExistingFileName,
	LPCWSTR lpNewFileName)
{
	if(FWide())
		return MoveFileW(lpExistingFileName, lpNewFileName);

	PreConvert();
	LPSTR szOld = Convert(lpExistingFileName);
	LPSTR szNew = Convert(lpNewFileName);

	return MoveFileA(szOld, szNew);
}

HANDLE
WINAPI
OLoadImageW(
	HINSTANCE hinst,
	LPCWSTR lpszName,
	UINT uType,
	int cxDesired,
	int cyDesired,
	UINT fuLoad)
{
	if (FWide())
		{
		Assert(!(LR_LOADFROMFILE & fuLoad));
		return LoadImageW(hinst, lpszName, uType, cxDesired, cyDesired, fuLoad);
		}

	if (!FATOM(lpszName))
		{
		PreConvert();
		LPSTR pszName = Convert(lpszName);
		return LoadImageA(hinst, pszName, uType, cxDesired, cyDesired, fuLoad);
		}
	 else
		return LoadImageA(hinst, (LPSTR) lpszName, uType, cxDesired, cyDesired, fuLoad);
}

BOOL
WINAPI
OOemToCharW(
	LPCSTR lpszSrc,
	LPWSTR lpszDst)
{
	if(FWide())
		{
		Assert(lpszSrc != (LPCSTR) lpszDst);
		return OemToCharW(lpszSrc, lpszDst);
		}

	DWORD cb = _mbslen((const unsigned char *)lpszSrc);
	LPSTR szDst = SzAlloc(cb);
	BOOL fRet = OemToCharA(lpszSrc, szDst);
	if(fRet)
		{
		Verify(0 <= AnsiToUnicode(lpszDst, szDst, cb));
		}
	return fRet;
}

VOID
WINAPI
OOutputDebugStringW(
	LPCWSTR lpOutputString
	)
{
	if(FWide())
		{
		OutputDebugStringW(lpOutputString);
		return;
		}

	PreConvert();
	LPSTR sz = Convert(lpOutputString);
	OutputDebugStringA(sz);
}

BOOL
WINAPI
OPeekMessageW(
	LPMSG lpMsg,
	HWND hWnd ,
	UINT wMsgFilterMin,
	UINT wMsgFilterMax,
	UINT wRemoveMsg)
{
	if(FWide())
		return PeekMessageW(lpMsg, hWnd , wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

	return PeekMessageA(lpMsg, hWnd , wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}

BOOL
WINAPI
OPostMessageW(
	HWND hWnd,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam)
{
	if(FWide())
		return PostMessageW(hWnd, Msg, wParam, lParam);

	return PostMessageA(hWnd, Msg, wParam, lParam);
}

BOOL
WINAPI
OPostThreadMessageW(
	DWORD idThread,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam)
 {
	if (FWide())
		return PostThreadMessageW(idThread, Msg, wParam, lParam);

	return PostThreadMessageA(idThread, Msg, wParam, lParam);
 }


// From: Mark Ashton on 5/8/97
LONG
APIENTRY
ORegCreateKeyExW(
	HKEY hKey,
	LPCWSTR lpSubKey,
	DWORD Reserved,
	LPWSTR lpClass,
	DWORD dwOptions,
	REGSAM samDesired,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	PHKEY phkResult,
	LPDWORD lpdwDisposition
	)
{
	Assert(lpSubKey);
	if(FWide())
		return RegCreateKeyExW(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired,
			lpSecurityAttributes, phkResult, lpdwDisposition);

	PreConvert();
	LPSTR sz = Convert(lpSubKey);
	LPSTR sz2 = Convert(lpClass);
	return RegCreateKeyExA(hKey, sz, Reserved, sz2, dwOptions, samDesired,
			lpSecurityAttributes, phkResult, lpdwDisposition);
}

// From: Mark Ashton on 5/8/97
LONG
APIENTRY
ORegCreateKeyW (
	HKEY hKey,
	LPCWSTR lpSubKey,
	PHKEY phkResult
	)
{
	if (FWide())
		return RegCreateKeyW(hKey, lpSubKey, phkResult);

	PreConvert();
	LPSTR sz = Convert(lpSubKey);
	return RegCreateKeyA(hKey, sz, phkResult);
}

// From: Mark Ashton on 5/8/97
LONG
APIENTRY
ORegEnumKeyW (
	HKEY hKey,
	DWORD dwIndex,
	LPWSTR lpName,
	DWORD cbName
	)
{
	if (FWide())
		return RegEnumKeyW(hKey, dwIndex, lpName, cbName);

	LPSTR sz = SzAlloc(cbName);
	LONG dwRet = RegEnumKeyA(hKey, dwIndex, sz, cbName);
	Verify(0 <= AnsiToUnicode(lpName, sz, cbName));
	return dwRet;
}

//  Van Kichline
//  IHammer group
//  Not supported: REG_MULTI_SZ
//
LONG
APIENTRY
ORegEnumValueW (
	HKEY hKey,
	DWORD dwIndex,
	LPWSTR lpValueName,
	LPDWORD lpcbValueName,  // Documentation indicates this is a count of characters, despite the Hungarian.
	LPDWORD lpReserved,
	LPDWORD lpType,         // May be NULL, but we need to know it on return if lpData is not NULL.
	LPBYTE lpData,          // May be NULL
	LPDWORD lpcbData        // May be NULL is lpData is NULL
	)
{
	if (FWide())
		return RegEnumValueW(hKey, dwIndex, lpValueName, lpcbValueName, lpReserved, lpType, lpData, lpcbData);

	// Required pointers:
	if (!lpValueName || !lpcbValueName || !lpcbData && lpData)
		{
		Assert(lpValueName);
		Assert(lpcbValueName);
		Assert(!lpcbData && lpData);
		return E_POINTER;
		}

	// If NULL was specified for lpType, we need to supply our own so we can check for string results.
	DWORD dwPrivateType = 0;
	if (!lpType)
		{
		lpType = &dwPrivateType;
		}

	DWORD cbValueName  = *lpcbValueName;
	DWORD dwOrigCbData = lpcbData ? *lpcbData : 0;
	LPSTR pchValueName = SzAlloc(*lpcbValueName);

	LONG lResult = RegEnumValueA(hKey, dwIndex, pchValueName, &cbValueName, lpReserved, lpType, lpData, lpcbData);

	if (ERROR_SUCCESS == lResult)
		{
		*lpcbValueName = AnsiToUnicode(lpValueName, pchValueName, min(*lpcbValueName, cbValueName + 1)) - 1; // Returned value does NOT include terminating NULL

		if (lpData)
			{
			// If the resulting data was a string, convert it in place.
			switch (*lpType)
				{
				case REG_MULTI_SZ:
					// Not supported
					Assert(0 && REG_MULTI_SZ);
					lResult = E_FAIL;
					break;
				case REG_EXPAND_SZ:
				case REG_SZ:
					{
					Assert(lpcbData);
					LPSTR pszTemp = SzAlloc(*lpcbData); // is the number of bytes!
					memcpy(pszTemp, lpData, *lpcbData);
					*lpcbData = AnsiToUnicode((LPWSTR)lpData, pszTemp, dwOrigCbData/sizeof(WCHAR), *lpcbData) * sizeof(WCHAR);

					//	It's possible to encounter a second stage overflow, if lpData >= sizeof(Unicode)/2
					if ( 0 == *lpcbData )
						{
						lResult = ERROR_MORE_DATA;
						}
					}
					break;
				}
			}
		}

	return lResult;
}

LONG 
APIENTRY ORegOpenKeyW(HKEY hKey, LPCWSTR pwszSubKey, PHKEY phkResult)
{
	if(FWide())
		return RegOpenKeyW(hKey, pwszSubKey, phkResult);

	PreConvert();
	LPSTR sz = Convert(pwszSubKey);

	return RegOpenKeyA(hKey, sz, phkResult);
}

LONG
APIENTRY
ORegDeleteKeyW(
	HKEY hKey,
	LPCWSTR pwszSubKey
	)
{
	Assert(pwszSubKey);
	if(FWide())
		return RegDeleteKeyW(hKey, pwszSubKey);

	PreConvert();
	LPSTR sz = Convert(pwszSubKey);
	return RegDeleteKeyA(hKey, sz);
}

LONG
APIENTRY
ORegDeleteValueW(
	HKEY hKey,
	LPWSTR lpValueName
	)
{
	if(FWide())
		return RegDeleteValueW (hKey, lpValueName);

	PreConvert();
	LPSTR sz = Convert(lpValueName);
	return RegDeleteValueA(hKey, sz);
}

ATOM
WINAPI
ORegisterClassW(
	CONST WNDCLASSW *lpWndClass)
{
	if(FWide())
		return RegisterClassW(lpWndClass);

	WNDCLASSA wc;
	memcpy(&wc, lpWndClass, sizeof(wc));

	PreConvert();

	if (!(IsBadReadPtr(wc.lpszMenuName, sizeof(* wc.lpszMenuName)) ||
		  IsBadReadPtr(lpWndClass->lpszMenuName, sizeof (*(lpWndClass->lpszMenuName)))))
		{
		wc.lpszMenuName = Convert(lpWndClass->lpszMenuName);
		}

	wc.lpszClassName = Convert(lpWndClass->lpszClassName);

	return RegisterClassA(&wc);
}

ATOM
WINAPI
ORegisterClassExW(CONST WNDCLASSEXW * lpWndClass)
{
	if (FWide())
		return RegisterClassExW(lpWndClass);

	WNDCLASSEXA wc;
	memcpy(&wc, lpWndClass, sizeof(wc));

	PreConvert();

	if (!FATOM(wc.lpszMenuName))
		{
		wc.lpszMenuName = Convert(lpWndClass->lpszMenuName);
		}

	if (!FATOM(wc.lpszClassName))
		wc.lpszClassName = Convert(lpWndClass->lpszClassName);

	return RegisterClassExA(&wc);
}

BOOL
WINAPI
OUnregisterClassW
(
LPCTSTR  lpClassName,   // address of class name string
HINSTANCE  hInstance    // handle of application instance
)
{
	if(FWide())
		return UnregisterClassW(lpClassName, hInstance);

	if (FATOM(lpClassName))
		return UnregisterClassW(lpClassName, hInstance);

	PreConvert();
	LPSTR sz = Convert(lpClassName);

	return UnregisterClassA(sz, hInstance);
}

UINT
WINAPI
ORegisterClipboardFormatW(
	LPCWSTR lpszFormat)
{
	if(FWide())
		return RegisterClipboardFormatW(lpszFormat);

	PreConvert();
	LPSTR sz = Convert(lpszFormat);

	return RegisterClipboardFormatA(sz);
}

UINT 
WINAPI
ORegisterWindowMessageW(LPCWSTR lpString)
{
	if(FWide())
		return RegisterWindowMessageW(lpString);

	PreConvert();
	LPSTR sz = Convert(lpString);
	return  RegisterWindowMessageA(sz);
}

LONG
APIENTRY
ORegOpenKeyExW (
	HKEY hKey,
	LPCTSTR lpSubKey,
	DWORD ulOptions,
	REGSAM samDesired,
	PHKEY phkResult
	)
{
	if(FWide())
		return RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);

	PreConvert();
	LPSTR sz = Convert(lpSubKey);

	return RegOpenKeyExA(hKey, sz, ulOptions, samDesired, phkResult);
}

LONG
APIENTRY
ORegQueryInfoKeyW (
	HKEY hKey,
	LPWSTR lpClass,
	LPDWORD lpcbClass,
	LPDWORD lpReserved,
	LPDWORD lpcSubKeys,
	LPDWORD lpcbMaxSubKeyLen,
	LPDWORD lpcbMaxClassLen,
	LPDWORD lpcValues,
	LPDWORD lpcbMaxValueNameLen,
	LPDWORD lpcbMaxValueLen,
	LPDWORD lpcbSecurityDescriptor,
	PFILETIME lpftLastWriteTime
	)
{
	Assert(!lpClass && !lpcbClass); //$ UNDONE_POST_98 - Not wrapped yet!
	if(FWide())
		return RegQueryInfoKeyW(hKey, lpClass, lpcbClass, lpReserved,
								lpcSubKeys, lpcbMaxSubKeyLen,
								lpcbMaxClassLen, lpcValues, lpcbMaxValueNameLen,
								lpcbMaxValueLen, lpcbSecurityDescriptor,
								lpftLastWriteTime );

	if (lpClass && (!lpcbClass || IsBadWritePtr(lpcbClass, sizeof(lpcbClass))))
		{
		// lpcbClass must be valid if lpClass is non-NULL
		return ERROR_INVALID_PARAMETER;
		}

	return RegQueryInfoKeyA(hKey, NULL, NULL, lpReserved,
							lpcSubKeys, lpcbMaxSubKeyLen,
							lpcbMaxClassLen, lpcValues, lpcbMaxValueNameLen,
							lpcbMaxValueLen, lpcbSecurityDescriptor,
							lpftLastWriteTime );
}

LONG 
APIENTRY ORegQueryValueW(HKEY hKey, LPCWSTR pwszSubKey, LPWSTR pwszValue,
	PLONG   lpcbValue)
{
	if(FWide())
		return RegQueryValueW(hKey, pwszSubKey, pwszValue, lpcbValue);

	LONG  cb;
	LONG  lRet    = 0;
	LPSTR szValue = NULL;
	PreConvert();
	LPSTR sz = Convert(pwszSubKey);

	lRet = RegQueryValueA(hKey, sz, NULL, &cb);

	if(ERROR_SUCCESS != lRet)
		{
		return lRet;
		}
	// If the caller was just asking for the size of the value, jump out
	//  now, without actually retrieving and converting the value.

	if (!pwszValue)
		{
		// Adjust size of buffer to report, to account for CHAR -> WCHAR
		*lpcbValue = cb * sizeof(WCHAR);
		goto Exit;
		}


	// If the caller was asking for the value, but allocated too small
	// of a buffer, set the buffer size and jump out.

	if (*lpcbValue < (LONG) (cb * sizeof(WCHAR)))
		{
		//$UNDONE_POST_98: We should actually use the nubmer of bytes required, not some
		// wild guess as we are here

		// Adjust size of buffer to report, to account for CHAR -> WCHAR
		*lpcbValue = cb * sizeof(WCHAR);
		lRet = ERROR_MORE_DATA;
		goto Exit;
		}

	// Otherwise, retrieve and convert the value.

	szValue = SzAlloc(cb);

	lRet = RegQueryValueA(hKey, sz, szValue, &cb);

	if (ERROR_SUCCESS == lRet)
		{
		Verify(0 <= AnsiToUnicode(pwszValue, szValue, cb));

		//$UNDONE_POST_98: We should actually use the nubmer of bytes required, not some
		// wild guess as we are here

		// Adjust size of buffer to report, to account for CHAR -> WCHAR
		*lpcbValue = cb * sizeof(WCHAR);
		}
	else if (pwszValue && 0 < cb)
		{
		*pwszValue = L'\0';
		}

Exit:

	return lRet;
}

LONG
APIENTRY
ORegSetValueExW(
	HKEY hKey,
	LPCWSTR lpValueName,
	DWORD Reserved,
	DWORD dwType,
	CONST BYTE* lpData,
	DWORD cbData
	)
{
	if(FWide())
		return RegSetValueExW(hKey, lpValueName, Reserved, dwType, lpData, cbData);

	PreConvert();
	LPSTR sz = Convert(lpValueName);

	LONG lRet;

	// NOTE: when calling RegSetValueExA, if the data type is
	// REG_SZ, REG_EXPAND_SZ, or REG_MULTI_SZ, then the API expects the strings
	// to be ansi also.
	if (REG_SZ == dwType || REG_EXPAND_SZ == dwType)
		{
		DWORD dwData = 0;
		LPSTR szData = ConvertWithLen((LPTSTR)lpData, -1, &dwData);
		lRet = RegSetValueExA(hKey, sz, Reserved, dwType, (CONST BYTE *)szData, dwData);
		}
	else if (REG_MULTI_SZ == dwType)
		{
		DWORD dwData = 0;
		LPSTR szData = ConvertWithLen((LPWSTR)lpData,
									  cUnicodeMultiSzLen((LPWSTR)lpData),
									  &dwData );
		lRet = RegSetValueExA(hKey, sz, Reserved, dwType, (CONST BYTE *)szData, dwData);
		}
	else
		{
		lRet = RegSetValueExA(hKey, sz, Reserved, dwType, lpData, cbData);
		}

	return lRet;
}

LONG 
APIENTRY ORegSetValueW(HKEY hKey, LPCWSTR lpSubKey, DWORD dwType,
	LPCWSTR lpData, DWORD cbData)
{
	Assert(REG_SZ == dwType);

	if(FWide())
		return RegSetValueW(hKey, lpSubKey, dwType,
			lpData, cbData);

	PreConvert();
	LPSTR szKey   = Convert(lpSubKey);
	LPSTR szValue = Convert(lpData);

	return RegSetValueA(hKey, szKey, dwType, szValue, cbData);
}

LONG
APIENTRY
ORegQueryValueExW (
	HKEY hKey,
	LPCWSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData
	)
{
	Assert(lpcbData || !lpData); // lpcbData can be NULL only if lpData is NULL
	if(FWide())
		return RegQueryValueExW (
			hKey,
			lpValueName,
			lpReserved,
			lpType,
			lpData,
			lpcbData
			);

	LPBYTE lpTempBuffer;
	DWORD dwTempType;
	DWORD cb, cbRequired;
	LONG  lRet;
	PreConvert();
	LPSTR sz = Convert(lpValueName);

	lRet = RegQueryValueExA(hKey, sz, lpReserved, &dwTempType, NULL, &cb);

	if(ERROR_SUCCESS != lRet)
		{
		return lRet;
		}

	// If the caller was just asking for the size of the value, jump out
	//  now, without actually retrieving and converting the value.

	if (!lpData)
		{
		switch (dwTempType)
			{
			case REG_EXPAND_SZ:
			case REG_MULTI_SZ:
			case REG_SZ:
				// Adjust size of buffer to report, to account for CHAR -> WCHAR

				*lpcbData = cb * sizeof(WCHAR);
				break;

			default:
				*lpcbData = cb;
				break;
			}

		// Set the type, if required.
		if (lpType)
			{
			*lpType = dwTempType;
			}

		goto Exit;
		}


	//
	// Determine the size of buffer needed
	//

	switch (dwTempType)
		{
		case REG_EXPAND_SZ:
		case REG_MULTI_SZ:
		case REG_SZ:
			cbRequired = cb * sizeof(WCHAR);
			break;

		default:
			cbRequired = cb;
			break;
		}

	// If the caller was asking for the value, but allocated too small
	// of a buffer, set the buffer size and jump out.

	if (*lpcbData < cbRequired)
		{
		// Adjust size of buffer to report, to account for CHAR -> WCHAR
		*lpcbData = cbRequired;

		// Set the type, if required.
		if (lpType)
			{
			*lpType = dwTempType;
			}

		lRet = ERROR_MORE_DATA;
		goto Exit;
		}

	// Otherwise, retrieve and convert the value.

	switch (dwTempType)
		{
		case REG_EXPAND_SZ:
		case REG_MULTI_SZ:
		case REG_SZ:

			lpTempBuffer = (LPBYTE)SzAlloc(cbRequired);

			lRet = RegQueryValueExA(hKey,
									sz,
									lpReserved,
									&dwTempType,
									lpTempBuffer,
									&cb);

			if (ERROR_SUCCESS == lRet)
				{
				switch (dwTempType)
					{
					case REG_EXPAND_SZ:
					case REG_MULTI_SZ:
					case REG_SZ:

						*lpcbData = AnsiToUnicode((LPWSTR)lpData, (LPSTR)lpTempBuffer, *lpcbData, cb);
						Verify(0 <= *lpcbData);
						*lpcbData = cb * sizeof(WCHAR); // Result it in BYTES!

						// Set the type, if required.
						if (lpType)
							{
							*lpType = dwTempType;
							}
						break;
					}
				}

			goto Exit;

		default:

			//
			// No conversion of out parameters needed.  Just call narrow
			// version with args passed in, and return directly.
			//

			lRet = RegQueryValueExA(hKey,
									sz,
									lpReserved,
									lpType,
									lpData,
									lpcbData);

		}

Exit:

	return lRet;
}

HANDLE
WINAPI
ORemovePropW(
	HWND hWnd,
	LPCWSTR lpString)
{
	if(FWide())
		return RemovePropW(hWnd, lpString);

	if(FATOM(lpString))
		return RemovePropA(hWnd, (LPSTR)lpString);

	PreConvert();
	LPSTR sz = Convert(lpString);
	return RemovePropA(hWnd, sz);
}

LRESULT
WINAPI
OSendDlgItemMessageW(
	HWND hDlg,
	int nIDDlgItem,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam)
{
	if(FWide())
		return SendDlgItemMessageW(hDlg, nIDDlgItem, Msg, wParam, lParam);

	PreConvert();
	switch (Msg)
		{
		case LB_ADDSTRING:
		case LB_INSERTSTRING:
		case LB_SELECTSTRING:
		case LB_FINDSTRING:
		case LB_FINDSTRINGEXACT:
		case CB_ADDSTRING:
		case CB_INSERTSTRING:
		case CB_SELECTSTRING:
		case CB_FINDSTRING:
		case CB_FINDSTRINGEXACT:
			{
			lParam = (LPARAM)Convert((LPWSTR)lParam);
			break;
			}
		}

	return SendDlgItemMessageA(hDlg, nIDDlgItem, Msg, wParam, lParam);
}

LRESULT
WINAPI
OSendMessageW(
	HWND hWnd,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam)
{
	// incase TCHAR strings are being passed in lParam the caller
	// will have to do the proper conversions PlatformToInternal or
	// InternalToPlatform

	if(FWide())
		return SendMessageW(hWnd, Msg, wParam, lParam);

	return SendMessageA(hWnd, Msg, wParam, lParam);
}

BOOL
WINAPI
OSendNotifyMessageW(
	HWND hWnd,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam)
{
	if(FWide())
		return SendNotifyMessageW(hWnd, Msg, wParam, lParam);

	return SendNotifyMessageA(hWnd, Msg, wParam, lParam);
}

BOOL
WINAPI
OSetDlgItemTextW(
	HWND hDlg,
	int nIDDlgItem,
	LPCWSTR lpString)
{
	if(FWide())
		return SetDlgItemTextW(hDlg, nIDDlgItem, lpString);

	PreConvert();
	LPSTR sz = Convert(lpString);
	return SetDlgItemTextA(hDlg, nIDDlgItem, sz);
}

BOOL
WINAPI
OSetFileAttributesW(
	LPCWSTR lpFileName,
	DWORD dwFileAttributes
	)
{
	if (FWide())
		return SetFileAttributesW(lpFileName, dwFileAttributes);

	PreConvert();
	LPSTR sz = Convert(lpFileName);
	return SetFileAttributesA(sz, dwFileAttributes);
}

BOOL
WINAPI
OSetPropW(
	HWND hWnd,
	LPCWSTR lpString,
	HANDLE hData)
{
	if(FWide())
		return SetPropW(hWnd, lpString, hData);

	if(FATOM(lpString))
		return SetPropA(hWnd, (LPSTR)lpString, hData);

	PreConvert();
	LPSTR sz = Convert(lpString);
	return SetPropA(hWnd, sz, hData);
}

BOOL
WINAPI
OSetMenuItemInfoW(
	HMENU hMenu,
	UINT uItem,
	BOOL fByPosition,
	LPCMENUITEMINFOW lpcmii
	)
{
	Assert(!IsBadWritePtr((void*)lpcmii, sizeof MENUITEMINFOW));
	Assert(sizeof MENUITEMINFOW == lpcmii->cbSize);
	Assert(sizeof MENUITEMINFOW == sizeof MENUITEMINFOA);

	if (FWide())
		return SetMenuItemInfoW(hMenu, uItem, fByPosition, lpcmii);

	MENUITEMINFOA mii;
	memcpy(&mii, lpcmii, sizeof MENUITEMINFOA);

	if (!(lpcmii->fMask & MIIM_TYPE) ||
		MFT_STRING != (lpcmii->fType &
				  (MFT_BITMAP | MFT_SEPARATOR | MFT_OWNERDRAW | MFT_STRING) ) )
		{
		return SetMenuItemInfoA(hMenu, uItem, fByPosition, &mii);
		}

	PreConvert();
	mii.dwTypeData = Convert(lpcmii->dwTypeData);
	return SetMenuItemInfoA(hMenu, uItem, fByPosition, &mii);
}

LONG
WINAPI
OSetWindowLongW(
	HWND hWnd,
	int nIndex,
	LONG dwNewLong)
{
	if(FWide())
		return SetWindowLongW(hWnd, nIndex, dwNewLong);

	return SetWindowLongA(hWnd, nIndex, dwNewLong);
}

HHOOK
WINAPI
OSetWindowsHookExW(
	int idHook,
	HOOKPROC lpfn,
	HINSTANCE hmod,
	DWORD dwThreadId)
{
	if(FWide())
		return SetWindowsHookExW(idHook, lpfn, hmod, dwThreadId);

	return SetWindowsHookExA(idHook, lpfn, hmod, dwThreadId);  //$ CONSIDER - Not really wrapped
}

BOOL
WINAPI
OSetWindowTextW(
	HWND hWnd,
	LPCWSTR lpString)
{
	if(FWide())
		return SetWindowTextW(hWnd, lpString);

	PreConvert();
	LPSTR sz = Convert(lpString);
	return SetWindowTextA(hWnd, sz);
}

LONG
WINAPI
OTabbedTextOutW(
	HDC hDC,
	int X,
	int Y,
	LPCWSTR lpString,
	int nCount,
	int nTabPositions,
	LPINT lpnTabStopPositions,
	int nTabOrigin)
{
	Assert(-1 != nCount);

	if(FWide())
		return TabbedTextOutW(hDC, X, Y, lpString, nCount, nTabPositions,
			lpnTabStopPositions, nTabOrigin);

	PreConvert();
	LONG  n = 0;
	LPSTR sz = ConvertWithLen(lpString, nCount, &n);

	return TabbedTextOutA(hDC, X, Y, sz, n, nTabPositions,
						  lpnTabStopPositions, nTabOrigin );
}

#if 0
// FOR OLE CTL: THIS MAGLES INTERFACE MEMBERS BY SAME NAME
int
WINAPI
OTranslateAcceleratorW(
	HWND hWnd,
	HACCEL hAccTable,
	LPMSG lpMsg)
{
	if(FWide())
		return TranslateAcceleratorW(hWnd, hAccTable, lpMsg);

	return TranslateAcceleratorA(hWnd, hAccTable, lpMsg);
}
#endif

SHORT
WINAPI
OVkKeyScanW(
	WCHAR ch)
{
	if (FWide())
		return VkKeyScanW(ch);
	TCHAR szW[2];
	char szA[2];
	szW[0] = ch;
	szW[1] = L'\0';
	Verify(0 <= UnicodeToAnsi(szA, szW, 2));
	return VkKeyScanA(szA[0]);
}

BOOL
WINAPI
OWinHelpW(
	HWND hWndMain,
	LPCWSTR lpszHelp,
	UINT uCommand,
	DWORD dwData
	)
{
	if(FWide())
		return WinHelpW(hWndMain, lpszHelp, uCommand,dwData);

	PreConvert();
	LPSTR sz = Convert(lpszHelp);
	return WinHelpA(hWndMain, sz, uCommand, dwData);
}

BOOL
WINAPI
OWritePrivateProfileStringW(
	LPCWSTR lpAppName,
	LPCWSTR lpKeyName,
	LPCWSTR lpString,
	LPCWSTR lpFileName)
{
	if(FWide())
		return WritePrivateProfileStringW(lpAppName, lpKeyName, lpString, lpFileName);

	PreConvert();
	LPSTR szAppName  = Convert(lpAppName);
	LPSTR szKeyName  = Convert(lpKeyName);
	LPSTR szString   = Convert(lpString);
	LPSTR szFileName = Convert(lpFileName);

	return WritePrivateProfileStringA(szAppName, szKeyName, szString, szFileName);
}

int 
WINAPIV
OwsprintfW(LPWSTR pwszOut, LPCWSTR pwszFormat, ...)
{
	va_list vaArgs;
	va_start(vaArgs, pwszFormat);
	int retval;

	if(FWide())
		retval = wvsprintfW(pwszOut, pwszFormat, vaArgs);
	else
		retval = _vstprintf(pwszOut, pwszFormat, vaArgs); //$CONSIDER Why isn't this vswprint?

	va_end(vaArgs);
	return retval;
}

BOOL
WINAPI
OGetVersionExW(
	LPOSVERSIONINFOW lpVersionInformation
	)
{
	if(FWide())
		return GetVersionExW(lpVersionInformation);

	if (lpVersionInformation->dwOSVersionInfoSize < sizeof(OSVERSIONINFOW))
		return false;

	OSVERSIONINFOA  osviVersionInfo;
	osviVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

	int fRetval = GetVersionExA(&osviVersionInfo);

	if (fRetval)
		{
		memcpy(lpVersionInformation, &osviVersionInfo, sizeof(OSVERSIONINFOA));

		Verify(0 <= AnsiToUnicode(lpVersionInformation->szCSDVersion,
								 osviVersionInfo.szCSDVersion,
								 sizeof(lpVersionInformation->szCSDVersion)
								 /sizeof(lpVersionInformation->szCSDVersion[0])));
		}

	return fRetval;
}

LONG
APIENTRY
ORegEnumKeyExW (
	HKEY hKey,
	DWORD dwIndex,
	LPWSTR lpName,
	LPDWORD lpcbName,
	LPDWORD lpReserved,
	LPWSTR lpClass,
	LPDWORD lpcbClass,
	PFILETIME lpftLastWriteTime
	)
{
	if(FWide())
		return RegEnumKeyExW (
			hKey,
			dwIndex,
			lpName,
			lpcbName,
			lpReserved,
			lpClass,
			lpcbClass,
			lpftLastWriteTime
			);

	LPSTR szName, szClass;
	DWORD cbName, cbClass;

	if (lpcbName)
		{
		cbName = sizeof(WCHAR) * *lpcbName;
		szName = lpName ? SzAlloc(cbName) : NULL;
		}
	else
		{
		szName = NULL;
		cbName = 0;
		}

	if (lpcbClass)
		{
		cbClass = sizeof(WCHAR) * (*lpcbClass);
		szClass = lpClass ? SzAlloc(cbClass) : NULL;
		}
	else
		{
		szClass = NULL;
		cbClass = 0;
		}

	LONG lRet = RegEnumKeyExA(hKey, dwIndex, szName, &cbName, lpReserved,
							  szClass, &cbClass, lpftLastWriteTime );

	if (ERROR_SUCCESS != lRet)
		{
		return lRet;
		}

	// Get the number of characters instead of number of bytes.
	if (lpcbName)
		{
		DWORD dwNoOfChar = AnsiToUnicode((LPWSTR) lpName, (LPSTR) szName, *lpcbName);
		if (cbName && !dwNoOfChar)
			{
			return ERROR_BUFFER_OVERFLOW;
			}

		*lpcbName = dwNoOfChar;
		}

	if (lpcbClass && lpClass)
		{
		DWORD dwNoOfChar = AnsiToUnicode((LPWSTR) lpClass, (LPSTR) szClass, *lpcbClass);

		if (cbClass && !dwNoOfChar)
			{
			return ERROR_BUFFER_OVERFLOW;
			}

		*lpcbClass = dwNoOfChar;
		}

	return lRet;

}

HANDLE
WINAPI
OCreateFileMappingW(
	HANDLE hFile,
	LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
	DWORD flProtect,
	DWORD dwMaximumSizeHigh,
	DWORD dwMaximumSizeLow,
	LPCWSTR lpName
	)
{
	if(FWide())
		return CreateFileMappingW(hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName);

	PreConvert();
	LPSTR sz = Convert(lpName);
	return CreateFileMappingA(hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, sz);
}

LRESULT
WINAPI
ODefDlgProcW(
	HWND hDlg,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam)
{
	// incase TCHAR strings are being passed in lParam the caller
	// will have to do the proper conversions PlatformToInternal or
	// InternalToPlatform

	if(FWide())
		return DefDlgProcW(hDlg, Msg, wParam, lParam);

	return DefDlgProcA(hDlg, Msg, wParam, lParam);
}

int
WINAPI
OGetLocaleInfoW(
	LCID     Locale,
	LCTYPE   LCType,
	LPWSTR  lpLCData,
	int      cchData)
{
	DWORD dwRet;

	if (FWide())
		return GetLocaleInfoW(Locale, LCType, lpLCData, cchData);

	if (!cchData || !lpLCData)
		return GetLocaleInfoA(Locale, LCType, NULL, cchData);

	int cchDataAnsi = sizeof(WCHAR) * cchData;
	LPSTR szBuffer = SzAlloc(cchDataAnsi);

	dwRet = GetLocaleInfoA(Locale, LCType, szBuffer, cchDataAnsi);
	// $UNDONE_POST_98: This is bogus, we should do this like OLoadStringW
	if(dwRet)
		{
		return AnsiToUnicode(lpLCData, szBuffer, cchData, dwRet);
		}
	else if (lpLCData && 0 < cchData)
		{
		*lpLCData = L'\0';
		}

	return dwRet;
}

BOOL
WINAPI
OSetLocaleInfoW(
	LCID     Locale,
	LCTYPE   LCType,
	LPCWSTR lpLCData)
{
	if (FWide())
		return SetLocaleInfoW(Locale, LCType, lpLCData);
	PreConvert();
	LPSTR sz = Convert(lpLCData);

	return SetLocaleInfoA(Locale, LCType, sz);
}

// $UNDONE_POST_98$ Workaround because StgCreateDocfile is not reentrant.
//          We were getting ACCESS DENIED errors when multiple threads opened
//             temp files simultaneously.

//-----------------------------------------------------------------------------
// Name: StgCreateDocfileCriticalSection
//
// Description:
// Used solely by OStgCreateDocfile in order to protect its call to
// StgCreateDocfile from simultaneously entry by multiple threads.
//
//-----------------------------------------------------------------------------
class StgCreateDocfileCriticalSection
{
public:
	StgCreateDocfileCriticalSection() {InitializeCriticalSection(&m_critsec);}
	~StgCreateDocfileCriticalSection() {DeleteCriticalSection(&m_critsec);}
	VOID VEnter() {EnterCriticalSection(&m_critsec);}
	VOID VLeave() {LeaveCriticalSection(&m_critsec);}
private:
	CRITICAL_SECTION m_critsec;
};

//-----------------------------------------------------------------------------
// Name: OStgCreateDocfile
//
// Description:
// Wrapper for StgCreateDocfile to protect against reentrancy bug in OLE.
//
// Thread-Safety: Bullet-proof
//
// Return Values: same HRESULT as StgCreateDocfile
//-----------------------------------------------------------------------------
HRESULT
WINAPI
OStgCreateDocfile
(
const WCHAR * pwcsName,
DWORD grfMode,
DWORD reserved,
IStorage ** ppstgOpen
)
{
	HRESULT hrReturn;
	static StgCreateDocfileCriticalSection Crit;
	Crit.VEnter();
// Change: Vank
// This definitions was infinitely recursive.  The 64 bit compiler caught it and refused to let it pass.
#ifdef StgCreateDocfile
#undef StgCreateDocfile
	hrReturn = StgCreateDocfile(pwcsName, grfMode, reserved, ppstgOpen);
#define StgCreateDocfile OStgCreateDocfile
#else
	hrReturn = StgCreateDocfile(pwcsName, grfMode, reserved, ppstgOpen);
#endif
// End change: VanK
	Crit.VLeave();
	return hrReturn;
}

int
WINAPI
OStartDocW
(
HDC hDC,
CONST DOCINFOW * pdiDocW
)
{
	if (FWide())
		return StartDocW(hDC, pdiDocW);

	DOCINFOA diDocA;

	PreConvert();

	diDocA.lpszDocName  = Convert(pdiDocW->lpszDocName);
	diDocA.lpszOutput   = Convert(pdiDocW->lpszOutput);
	diDocA.lpszDatatype = Convert(pdiDocW->lpszDatatype);
	diDocA.cbSize       = sizeof(DOCINFOA);
	diDocA.fwType       = pdiDocW->fwType;

	return StartDocA(hDC, &diDocA);

}

BOOL
WINAPI
OSystemParametersInfoW(
	UINT uiAction,
	UINT uiParam,
	PVOID pvParam,
	UINT fWinIni)
{
	if (FWide())
		return SystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni);

	switch (uiAction)
		{   // unsupported actions
		case SPI_GETHIGHCONTRAST:
		case SPI_GETICONMETRICS:
		case SPI_GETICONTITLELOGFONT:
		case SPI_GETNONCLIENTMETRICS:
		case SPI_GETSERIALKEYS:
		case SPI_GETSOUNDSENTRY:

		case SPI_SETDESKWALLPAPER:
		case SPI_SETHIGHCONTRAST:
		case SPI_SETICONMETRICS:
		case SPI_SETICONTITLELOGFONT:
		case SPI_SETNONCLIENTMETRICS:
		case SPI_SETSERIALKEYS:
		case SPI_SETSOUNDSENTRY:
			AssertFail("No Unicode Wrapper Available for Win32 API - SystemParametersInfoW");
			return 0;
		};
	return SystemParametersInfoA(uiAction, uiParam, pvParam, fWinIni);
}

LPWSTR
WINAPI
OCharNextW(
LPCWSTR lpsz)
{
	if ( FWide() )
		return CharNextW( lpsz );

	if (*lpsz == L'\0')
		{
		return const_cast<LPWSTR>(lpsz);
		}

	return const_cast<LPWSTR>(lpsz + 1);
}


#ifdef DEBUG
BOOL
APIENTRY
OAbortSystemShutdownW(
	LPWSTR lpMachineName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - AbortSystemShutdownW");
	return 0;
}

BOOL
WINAPI
OAccessCheckAndAuditAlarmW (
	LPCWSTR SubsystemName,
	LPVOID HandleId,
	LPWSTR ObjectTypeName,
	LPWSTR ObjectName,
	PSECURITY_DESCRIPTOR SecurityDescriptor,
	DWORD DesiredAccess,
	PGENERIC_MAPPING GenericMapping,
	BOOL ObjectCreation,
	LPDWORD GrantedAccess,
	LPBOOL AccessStatus,
	LPBOOL pfGenerateOnClose
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - AccessCheckAndAuditAlarmW");
	return 0;
}

int 
WINAPI OAddFontResourceW(LPCWSTR)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - AddFontResourceW");
	return 0;
}

BOOL
WINAPI
OAddFormW(
	HANDLE  hPrinter,
	DWORD   Level,
	LPBYTE  pForm
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - AddFormW");
	return 0;
}

BOOL
WINAPI
OAddJobW(
	HANDLE  hPrinter,
	DWORD   Level,
	LPBYTE  pData,
	DWORD   cbBuf,
	LPDWORD pcbNeeded
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - AddJobW");
	return 0;
}

BOOL
WINAPI
OAddMonitorW(
	LPWSTR   pName,
	DWORD   Level,
	LPBYTE  pMonitors
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - AddMonitorW");
	return 0;
}

BOOL
WINAPI
OAddPortW(
	LPWSTR   pName,
	HWND    hWnd,
	LPWSTR   pMonitorName
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - AddPortW");
	return 0;
}

HANDLE
WINAPI
OAddPrinterW(
	LPWSTR   pName,
	DWORD   Level,
	LPBYTE  pPrinter
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - AddPrinterW");
	return 0;
}

BOOL
WINAPI
OAddPrinterConnectionW(
	LPWSTR   pName
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - AddPrinterConnectionW");
	return 0;
}

BOOL
WINAPI
OAddPrinterDriverW(
	LPWSTR   pName,
	DWORD   Level,
	LPBYTE  pDriverInfo
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - AddPrinterDriverW");
	return 0;
}

BOOL
WINAPI
OAddPrintProcessorW(
	LPWSTR   pName,
	LPWSTR   pEnvironment,
	LPWSTR   pPathName,
	LPWSTR   pPrintProcessorName
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - AddPrintProcessorW");
	return 0;
}

BOOL
WINAPI
OAddPrintProvidorW(
	LPWSTR  pName,
	DWORD    level,
	LPBYTE   pProvidorInfo
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - AddPrintProvidorW");
	return 0;
}

LONG
WINAPI
OAdvancedDocumentPropertiesW(
	HWND    hWnd,
	HANDLE  hPrinter,
	LPWSTR   pDeviceName,
	PDEVMODEW pDevModeOutput,
	PDEVMODEW pDevModeInput
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - AdvancedDocumentPropertiesW");
	return 0;
}

MMRESULT WINAPI OauxGetDevCapsW(UINT uDeviceID, LPAUXCAPSW pac, UINT cbac)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - auxGetDevCapsW");
	return 0;
}

BOOL
WINAPI
OBackupEventLogW (
	HANDLE hEventLog,
	LPCWSTR lpBackupFileName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - BackupEventLogW");
	return 0;
}

HANDLE
WINAPI
OBeginUpdateResourceW(
	LPCWSTR pFileName,
	BOOL bDeleteExistingResources
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - BeginUpdateResourceW");
	return 0;
}

BOOL
WINAPI
OBuildCommDCBW(
	LPCWSTR lpDef,
	LPDCB lpDCB
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - BuildCommDCBW");
	return 0;
}

BOOL
WINAPI
OBuildCommDCBAndTimeoutsW(
	LPCWSTR lpDef,
	LPDCB lpDCB,
	LPCOMMTIMEOUTS lpCommTimeouts
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - BuildCommDCBAndTimeoutsW");
	return 0;
}

BOOL
WINAPI
OCallMsgFilterW(
	LPMSG lpMsg,
	int nCode)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CallMsgFilterW");
	return 0;
}

BOOL
WINAPI
OCallNamedPipeW(
	LPCWSTR lpNamedPipeName,
	LPVOID lpInBuffer,
	DWORD nInBufferSize,
	LPVOID lpOutBuffer,
	DWORD nOutBufferSize,
	LPDWORD lpBytesRead,
	DWORD nTimeOut
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CallNamedPipeW");
	return 0;
}

LONG
WINAPI
OChangeDisplaySettingsW(
	LPDEVMODEW lpDevMode,
	DWORD dwFlags)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ChangeDisplaySettingsW");
	return 0;
}

BOOL
WINAPI
OChangeMenuW(
	HMENU hMenu,
	UINT cmd,
	LPCWSTR lpszNewItem,
	UINT cmdInsert,
	UINT flags)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ChangeMenuW");
	return 0;
}

#if 0 //$UNDONE_POST_98 - We should wrap these as being NT only...
BOOL
WINAPI
OChangeServiceConfigW(
	SC_HANDLE    hService,
	DWORD        dwServiceType,
	DWORD        dwStartType,
	DWORD        dwErrorControl,
	LPCWSTR     lpBinaryPathName,
	LPCWSTR     lpLoadOrderGroup,
	LPDWORD      lpdwTagId,
	LPCWSTR     lpDependencies,
	LPCWSTR     lpServiceStartName,
	LPCWSTR     lpPassword,
	LPCWSTR     lpDisplayName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ChangeServiceConfigW");
	return 0;
}
#endif

BOOL
WINAPI
OCharToOemBuffW(
	LPCWSTR lpszSrc,
	LPSTR lpszDst,
	DWORD cchDstLength)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CharToOemBuffW");
	return 0;
}

DWORD
WINAPI
OCharUpperBuffW(
	LPWSTR lpsz,
	DWORD cchLength)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CharUpperBuffW");
	return 0;
}

BOOL
WINAPI
OChooseColorW(
	LPCHOOSECOLORW lpcc)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ChooseColorW");
	return 0;
}

BOOL
APIENTRY OChooseFontW(LPCHOOSEFONTW pchfw)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ChooseFontW");
	return 0;
}

BOOL
WINAPI
OClearEventLogW (
	HANDLE hEventLog,
	LPCWSTR lpBackupFileName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ClearEventLogW");
	return 0;
}

BOOL
WINAPI
OCommConfigDialogW(
	LPCWSTR lpszName,
	HWND hWnd,
	LPCOMMCONFIG lpCC
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CommConfigDialogW");
	return 0;
}

int
WINAPI
OCompareStringW(
	LCID     Locale,
	DWORD    dwCmpFlags,
	LPCWSTR lpString1,
	int      cchCount1,
	LPCWSTR lpString2,
	int      cchCount2)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CompareStringW");
	return 0;
}

BOOL
WINAPI
OConfigurePortW(
	LPWSTR   pName,
	HWND    hWnd,
	LPWSTR   pPortName
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ConfigurePortW");
	return 0;
}

int
WINAPI
OCopyAcceleratorTableW(
	HACCEL hAccelSrc,
	LPACCEL lpAccelDst,
	int cAccelEntries)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CopyAcceleratorTableW");
	return 0;
}

HENHMETAFILE 
WINAPI 
OCopyEnhMetaFileW(HENHMETAFILE, LPCWSTR)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CopyEnhMetaFileW");
	return 0;
}

HMETAFILE 
WINAPI 
OCopyMetaFileW(HMETAFILE, LPCWSTR)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CopyMetaFileW");
	return 0;
}

HACCEL
WINAPI
OCreateAcceleratorTableW(
	LPACCEL, int)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreateAcceleratorTableW");
	return 0;
}

WINAPI 
OCreateColorSpaceW(LPLOGCOLORSPACEW)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreateColorSpaceW");
	return 0;
}

HDESK
WINAPI
OCreateDesktopW(
	LPWSTR lpszDesktop,
	LPWSTR lpszDevice,
	LPDEVMODEW pDevmode,
	DWORD dwFlags,
	DWORD dwDesiredAccess,
	LPSECURITY_ATTRIBUTES lpsa)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreateDesktopW");
	return 0;
}

HWND
WINAPI
OCreateDialogIndirectParamW(
	HINSTANCE hInstance,
	LPCDLGTEMPLATEW lpTemplate,
	HWND hWndParent,
	DLGPROC lpDialogFunc,
	LPARAM dwInitParam)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreateDialogIndirectParamW");
	return 0;
}

HWND
WINAPI
OCreateDialogParamW(
	HINSTANCE hInstance,
	LPCWSTR lpTemplateName,
	HWND hWndParent ,
	DLGPROC lpDialogFunc,
	LPARAM dwInitParam)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreateDialogParamW");
	return 0;
}

HDC
WINAPI
OCreateICW(
	LPCWSTR lpszDriver,
	LPCWSTR lpszDevice,
	LPCWSTR lpszOutput,
	CONST DEVMODEW *lpdvmInit)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreateICW");
	return 0;
}

HANDLE
WINAPI
OCreateMailslotW(
	LPCWSTR lpName,
	DWORD nMaxMessageSize,
	DWORD lReadTimeout,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreateMailslotW");
	return 0;
}

HANDLE
WINAPI
OCreateMutexW(
	LPSECURITY_ATTRIBUTES lpMutexAttributes,
	BOOL bInitialOwner,
	LPCWSTR lpName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreateMutexW");
	return 0;
}

HANDLE
WINAPI
OCreateNamedPipeW(
	LPCWSTR lpName,
	DWORD dwOpenMode,
	DWORD dwPipeMode,
	DWORD nMaxInstances,
	DWORD nOutBufferSize,
	DWORD nInBufferSize,
	DWORD nDefaultTimeOut,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreateNamedPipeW");
	return 0;
}

BOOL
WINAPI
OCreateProcessW(
	LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreateProcessW");
	return 0;
}

BOOL
WINAPI
OCreateProcessAsUserW (
	HANDLE hToken,
	LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreateProcessAsUserW");
	return 0;
}

HPROPSHEETPAGE
WINAPI
OCreatePropertySheetPageW(
	LPCPROPSHEETPAGEW lpcpsp
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreatePropertySheetPageW");
	return 0;
}

BOOL
WINAPI
OCreateScalableFontResourceW(DWORD, LPCWSTR, LPCWSTR, LPCWSTR)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreateScalableFontResourceW");
	return 0;
}

#if 0 //$UNDONE_POST_98 - We should wrap these as being NT only...
SC_HANDLE
WINAPI
OCreateServiceW(
	SC_HANDLE    hSCManager,
	LPCWSTR     lpServiceName,
	LPCWSTR     lpDisplayName,
	DWORD        dwDesiredAccess,
	DWORD        dwServiceType,
	DWORD        dwStartType,
	DWORD        dwErrorControl,
	LPCWSTR     lpBinaryPathName,
	LPCWSTR     lpLoadOrderGroup,
	LPDWORD      lpdwTagId,
	LPCWSTR     lpDependencies,
	LPCWSTR     lpServiceStartName,
	LPCWSTR     lpPassword
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreateServiceW");
	return 0;
}
#endif

HWND WINAPI OCreateStatusWindowW(LONG style, LPCWSTR lpszText, HWND hwndParent, UINT wID)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreateStatusWindowW");
	return 0;
}

HWINSTA
WINAPI
OCreateWindowStationW(
	LPWSTR lpwinsta,
	DWORD dwReserved,
	DWORD dwDesiredAccess,
	LPSECURITY_ATTRIBUTES lpsa)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - CreateWindowStationW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ODceErrorInqTextW (
	IN RPC_STATUS RpcStatus,
	OUT unsigned short __RPC_FAR * ErrorText
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DceErrorInqTextW");
	return 0;
}

BOOL
WINAPI
ODefineDosDeviceW(
	DWORD dwFlags,
	LPCWSTR lpDeviceName,
	LPCWSTR lpTargetPath
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DefineDosDeviceW");
	return 0;
}

BOOL
WINAPI
ODeleteFormW(
	HANDLE  hPrinter,
	LPWSTR   pFormName
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DeleteFormW");
	return 0;
}

BOOL
WINAPI
ODeleteMonitorW(
	LPWSTR   pName,
	LPWSTR   pEnvironment,
	LPWSTR   pMonitorName
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DeleteMonitorW");
	return 0;
}

BOOL
WINAPI
ODeletePortW(
	LPWSTR   pName,
	HWND    hWnd,
	LPWSTR   pPortName
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DeletePortW");
	return 0;
}

BOOL
WINAPI
ODeletePrinterConnectionW(
	LPWSTR   pName
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DeletePrinterConnectionW");
	return 0;
}

BOOL
WINAPI
ODeletePrinterDriverW(
   LPWSTR    pName,
   LPWSTR    pEnvironment,
   LPWSTR    pDriverName
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DeletePrinterDriverW");
	return 0;
}

BOOL
WINAPI
ODeletePrintProcessorW(
	LPWSTR   pName,
	LPWSTR   pEnvironment,
	LPWSTR   pPrintProcessorName
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DeletePrintProcessorW");
	return 0;
}

BOOL
WINAPI
ODeletePrintProvidorW(
	LPWSTR   pName,
	LPWSTR   pEnvironment,
	LPWSTR   pPrintProvidorName
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DeletePrintProvidorW");
	return 0;
}

int  
WINAPI 
ODeviceCapabilitiesW(LPCWSTR, LPCWSTR, WORD,
								LPWSTR, CONST DEVMODEW *)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DeviceCapabilitiesW");
	return 0;
}

int
WINAPI
ODlgDirListW(
	HWND hDlg,
	LPWSTR lpPathSpec,
	int nIDListBox,
	int nIDStaticPath,
	UINT uFileType)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DlgDirListW");
	return 0;
}

int
WINAPI
ODlgDirListComboBoxW(
	HWND hDlg,
	LPWSTR lpPathSpec,
	int nIDComboBox,
	int nIDStaticPath,
	UINT uFiletype)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DlgDirListComboBoxW");
	return 0;
}

BOOL
WINAPI
ODlgDirSelectComboBoxExW(
	HWND hDlg,
	LPWSTR lpString,
	int nCount,
	int nIDComboBox)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DlgDirSelectComboBoxExW");
	return 0;
}

BOOL
WINAPI
ODlgDirSelectExW(
	HWND hDlg,
	LPWSTR lpString,
	int nCount,
	int nIDListBox)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DlgDirSelectExW");
	return 0;
}

DWORD
WINAPI
ODocumentPropertiesW(
	HWND      hWnd,
	HANDLE    hPrinter,
	LPWSTR   pDeviceName,
	PDEVMODEW pDevModeOutput,
	PDEVMODEW pDevModeInput,
	DWORD     fMode
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DocumentPropertiesW");
	return 0;
}

DWORD   
APIENTRY 
ODoEnvironmentSubstW(LPWSTR szString, UINT cbString)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DoEnvironmentSubstW");
	return 0;
}

UINT 
APIENTRY 
ODragQueryFileW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DragQueryFileW");
	return 0;
}

BOOL
WINAPI 
ODrawStateW(HDC, HBRUSH, DRAWSTATEPROC, LPARAM, WPARAM, int, int, int, int, UINT)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DrawStateW");
	return 0;
}

BOOL
WINAPI
OEndUpdateResourceW(
	HANDLE      hUpdate,
	BOOL        fDiscard
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EndUpdateResourceW");
	return 0;
}

BOOL
WINAPI
OEnumCalendarInfoW(
	CALINFO_ENUMPROCW lpCalInfoEnumProc,
	LCID              Locale,
	CALID             Calendar,
	CALTYPE           CalType)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumCalendarInfoW");
	return 0;
}

BOOL
WINAPI
OEnumDateFormatsW(
	DATEFMT_ENUMPROCW lpDateFmtEnumProc,
	LCID              Locale,
	DWORD             dwFlags)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumDateFormatsW");
	return 0;
}

#if 0 //$UNDONE_POST_98 - We should wrap these as being NT only...
BOOL
WINAPI
OEnumDependentServicesW(
	SC_HANDLE               hService,
	DWORD                   dwServiceState,
	LPENUM_SERVICE_STATUSW  lpServices,
	DWORD                   cbBufSize,
	LPDWORD                 pcbBytesNeeded,
	LPDWORD                 lpServicesReturned
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumDependentServicesW");
	return 0;
}
#endif

BOOL
WINAPI
OEnumDesktopsW(
	HWINSTA hwinsta,
	DESKTOPENUMPROCW lpEnumFunc,
	LPARAM lParam)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumDesktopsW");
	return 0;
}

BOOL
WINAPI
OEnumDisplaySettingsW(
	LPCWSTR lpszDeviceName,
	DWORD iModeNum,
	LPDEVMODEW lpDevMode)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumDisplaySettingsW");
	return 0;
}

int
WINAPI 
OEnumFontFamiliesW(HDC, LPCWSTR, FONTENUMPROCW, LPARAM)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumFontFamiliesW");
	return 0;
}

int
WINAPI
OEnumFontFamiliesExW(HDC, LPLOGFONTW,FONTENUMPROCW, LPARAM,DWORD)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumFontFamiliesExW");
	return 0;
}

int
WINAPI
OEnumFontsW(HDC, LPCWSTR,  FONTENUMPROCW, LPARAM)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumFontsW");
	return 0;
}

BOOL
WINAPI
OEnumFormsW(
	HANDLE  hPrinter,
	DWORD   Level,
	LPBYTE  pForm,
	DWORD   cbBuf,
	LPDWORD pcbNeeded,
	LPDWORD pcReturned
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumFormsW");
	return 0;
}

WINAPI
OEnumICMProfilesW(HDC,ICMENUMPROCW,LPARAM)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumICMProfilesW");
	return 0;
}

BOOL
WINAPI
OEnumJobsW(
	HANDLE  hPrinter,
	DWORD   FirstJob,
	DWORD   NoJobs,
	DWORD   Level,
	LPBYTE  pJob,
	DWORD   cbBuf,
	LPDWORD pcbNeeded,
	LPDWORD pcReturned
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumJobsW");
	return 0;
}

BOOL
WINAPI
OEnumMonitorsW(
	LPWSTR   pName,
	DWORD   Level,
	LPBYTE  pMonitors,
	DWORD   cbBuf,
	LPDWORD pcbNeeded,
	LPDWORD pcReturned
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumMonitorsW");
	return 0;
}

BOOL
WINAPI
OEnumPortsW(
	LPWSTR   pName,
	DWORD   Level,
	LPBYTE  pPorts,
	DWORD   cbBuf,
	LPDWORD pcbNeeded,
	LPDWORD pcReturned
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumPortsW");
	return 0;
}

BOOL
WINAPI
OEnumPrinterDriversW(
	LPWSTR   pName,
	LPWSTR   pEnvironment,
	DWORD   Level,
	LPBYTE  pDriverInfo,
	DWORD   cbBuf,
	LPDWORD pcbNeeded,
	LPDWORD pcReturned
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumPrinterDriversW");
	return 0;
}

BOOL
WINAPI
OEnumPrintersW(
	DWORD   Flags,
	LPWSTR   Name,
	DWORD   Level,
	LPBYTE  pPrinterEnum,
	DWORD   cbBuf,
	LPDWORD pcbNeeded,
	LPDWORD pcReturned
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumPrintersW");
	return 0;
}

BOOL
WINAPI
OEnumPrintProcessorDatatypesW(
	LPWSTR   pName,
	LPWSTR   pPrintProcessorName,
	DWORD   Level,
	LPBYTE  pDatatypes,
	DWORD   cbBuf,
	LPDWORD pcbNeeded,
	LPDWORD pcReturned
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumPrintProcessorDatatypesW");
	return 0;
}

BOOL
WINAPI
OEnumPrintProcessorsW(
	LPWSTR   pName,
	LPWSTR   pEnvironment,
	DWORD   Level,
	LPBYTE  pPrintProcessorInfo,
	DWORD   cbBuf,
	LPDWORD pcbNeeded,
	LPDWORD pcReturned
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumPrintProcessorsW");
	return 0;
}

int
WINAPI
OEnumPropsW(
	HWND hWnd,
	PROPENUMPROCW lpEnumFunc)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumPropsW");
	return 0;
}

int
WINAPI
OEnumPropsExW(
	HWND hWnd,
	PROPENUMPROCEXW lpEnumFunc,
	LPARAM lParam)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumPropsExW");
	return 0;
}

INT
APIENTRY
OEnumProtocolsW (
	IN     LPINT           lpiProtocols,
	IN OUT LPVOID          lpProtocolBuffer,
	IN OUT LPDWORD         lpdwBufferLength
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumProtocolsW");
	return 0;
}

BOOL
WINAPI
OEnumResourceLanguagesW(
	HMODULE hModule,
	LPCWSTR lpType,
	LPCWSTR lpName,
	ENUMRESLANGPROC lpEnumFunc,
	LONG lParam
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumResourceLanguagesW");
	return 0;
}

BOOL
WINAPI
OEnumResourceNamesW(
	HMODULE hModule,
	LPCWSTR lpType,
	ENUMRESNAMEPROC lpEnumFunc,
	LONG lParam
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumResourceNamesW");
	return 0;
}

BOOL
WINAPI
OEnumResourceTypesW(
	HMODULE hModule,
	ENUMRESTYPEPROC lpEnumFunc,
	LONG lParam
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumResourceTypesW");
	return 0;
}

#if 0 //$UNDONE_POST_98 - We should wrap these as being NT only...
BOOL
WINAPI
OEnumServicesStatusW(
	SC_HANDLE               hSCManager,
	DWORD                   dwServiceType,
	DWORD                   dwServiceState,
	LPENUM_SERVICE_STATUSW  lpServices,
	DWORD                   cbBufSize,
	LPDWORD                 pcbBytesNeeded,
	LPDWORD                 lpServicesReturned,
	LPDWORD                 lpResumeHandle
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumServicesStatusW");
	return 0;
}
#endif

BOOL
WINAPI
OEnumSystemCodePagesW(
	CODEPAGE_ENUMPROCW lpCodePageEnumProc,
	DWORD              dwFlags)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumSystemCodePagesW");
	return 0;
}

BOOL
WINAPI
OEnumSystemLocalesW(
	LOCALE_ENUMPROCW lpLocaleEnumProc,
	DWORD            dwFlags)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumSystemLocalesW");
	return 0;
}

BOOL
WINAPI
OEnumTimeFormatsW(
	TIMEFMT_ENUMPROCW lpTimeFmtEnumProc,
	LCID              Locale,
	DWORD             dwFlags)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumTimeFormatsW");
	return 0;
}

BOOL
WINAPI
OEnumWindowStationsW(
	WINSTAENUMPROCW lpEnumFunc,
	LPARAM lParam)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - EnumWindowStationsW");
	return 0;
}

HICON
APIENTRY
OExtractAssociatedIconW(HINSTANCE hInst, LPWSTR lpIconPath, LPWORD lpiIcon)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ExtractAssociatedIconW");
	return 0;
}

HICON
APIENTRY
OExtractIconW(HINSTANCE hInst, LPCWSTR lpszExeFileName, UINT nIconIndex)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ExtractIconW");
	return 0;
}


UINT 
WINAPI 
OExtractIconExW(LPCWSTR lpszFile, int nIconIndex, HICON FAR *phiconLarge, HICON FAR *phiconSmall, UINT nIcons)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ExtractIconExW");
	return 0;
}

// Commented since gdi32.dll on Win95 provides the wrapper for this function.
/*
BOOL
WINAPI 
OExtTextOutW(HDC, int, int, UINT, CONST RECT *,LPCWSTR, UINT, CONST INT *)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ExtTextOutW");
	return 0;
}
*/

BOOL
WINAPI
OFillConsoleOutputCharacterW(
	HANDLE hConsoleOutput,
	WCHAR  cCharacter,
	DWORD  nLength,
	COORD  dwWriteCoord,
	LPDWORD lpNumberOfCharsWritten
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - FillConsoleOutputCharacterW");
	return 0;
}

LPWSTR 
APIENTRY 
OFindEnvironmentStringW(LPWSTR szEnvVar)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - FindEnvironmentStringW");
	return 0;
}

HINSTANCE
APIENTRY
OFindExecutableW(LPCWSTR lpFile, LPCWSTR lpDirectory, LPWSTR lpResult)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - FindExecutableW");
	return 0;
}

HRSRC
WINAPI
OFindResourceExW(
	HMODULE hModule,
	LPCWSTR lpType,
	LPCWSTR lpName,
	WORD    wLanguage
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - FindResourceExW");
	return 0;
}

APIENTRY
OFindTextW(LPFINDREPLACEW)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - FindTextW");
	return 0;
}

HWND
WINAPI 
OFindWindowExW(HWND, HWND, LPCWSTR, LPCWSTR)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - FindWindowExW");
	return 0;
}

int
WINAPI
OFoldStringW(
	DWORD    dwMapFlags,
	LPCWSTR lpSrcStr,
	int      cchSrc,
	LPWSTR  lpDestStr,
	int      cchDest)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - FoldStringW");
	return 0;
}

BOOL
WINAPI
OGetBinaryTypeW(
	LPCWSTR lpApplicationName,
	LPDWORD lpBinaryType
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetBinaryTypeW");
	return 0;
}

DWORD
WINAPI
OGetCharacterPlacementW(HDC, LPCWSTR, int, int, LPGCP_RESULTSW, DWORD)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetCharacterPlacementW");
	return 0;
}

BOOL
WINAPI
OGetCharWidth32W(HDC, UINT, UINT, LPINT)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetCharWidth32W");
	return 0;
}

LPWSTR
WINAPI
OGetCommandLineW(
	VOID
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetCommandLineW");
	return 0;
}

DWORD
WINAPI
OGetCompressedFileSizeW(
	LPCWSTR lpFileName,
	LPDWORD lpFileSizeHigh
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetCompressedFileSizeW");
	return 0;
}

BOOL
WINAPI
OGetComputerNameW (
	LPWSTR lpBuffer,
	LPDWORD nSize
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetComputerNameW");
	return 0;
}

DWORD
WINAPI
OGetConsoleTitleW(
	LPWSTR lpConsoleTitle,
	DWORD nSize
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetConsoleTitleW");
	return 0;
}

int
WINAPI
OGetCurrencyFormatW(
	LCID     Locale,
	DWORD    dwFlags,
	LPCWSTR lpValue,
	CONST CURRENCYFMTW *lpFormat,
	LPWSTR  lpCurrencyStr,
	int      cchCurrency)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetCurrencyFormatW");
	return 0;
}

int
WINAPI
OGetDateFormatW(
	LCID     Locale,
	DWORD    dwFlags,
	CONST SYSTEMTIME *lpDate,
	LPCWSTR lpFormat,
	LPWSTR  lpDateStr,
	int      cchDate)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetDateFormatW");
	return 0;
}

BOOL
WINAPI
OGetDefaultCommConfigW(
	LPCWSTR lpszName,
	LPCOMMCONFIG lpCC,
	LPDWORD lpdwSize
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetDefaultCommConfigW");
	return 0;
}

BOOL
WINAPI
OGetDiskFreeSpaceW(
	LPCWSTR lpRootPathName,
	LPDWORD lpSectorsPerCluster,
	LPDWORD lpBytesPerSector,
	LPDWORD lpNumberOfFreeClusters,
	LPDWORD lpTotalNumberOfClusters
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetDiskFreeSpaceW");
	return 0;
}

UINT
WINAPI
OGetDriveTypeW(
	LPCWSTR lpRootPathName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetDriveTypeW");
	return 0;
}

HENHMETAFILE
WINAPI
OGetEnhMetaFileW(LPCWSTR)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetEnhMetaFileW");
	return 0;
}

UINT
WINAPI
OGetEnhMetaFileDescriptionW(HENHMETAFILE, UINT, LPWSTR )
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetEnhMetaFileDescriptionW");
	return 0;
}

DWORD
WINAPI
OGetEnvironmentVariableW(
	LPCWSTR lpName,
	LPWSTR lpBuffer,
	DWORD nSize
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetEnvironmentVariableW");
	return 0;
}

INT
APIENTRY
OGetExpandedNameW(
	LPWSTR,
	LPWSTR
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetExpandedNameW");
	return 0;
}

BOOL
WINAPI
OGetFileSecurityW (
	LPCWSTR lpFileName,
	SECURITY_INFORMATION RequestedInformation,
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	DWORD nLength,
	LPDWORD lpnLengthNeeded
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetFileSecurityW");
	return 0;
}

short
WINAPI
OGetFileTitleW
(
LPCWSTR pwszFile,
LPWSTR pwszOut,
WORD w
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetFileTitleW");
	return 0;
}

BOOL
WINAPI
OGetFileVersionInfoW(
	LPWSTR lpszFile,
	DWORD dwHandle,
	DWORD cbBuf,
	LPVOID lpvData)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetFileVersionInfoW");
	return 0;
}

DWORD
WINAPI
OGetFileVersionInfoSizeW(
	LPWSTR lpszFile,
	LPDWORD lpdwHandle)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetFileVersionInfoSizeW");
	return 0;
}

BOOL
WINAPI
OGetFormW(
	HANDLE  hPrinter,
	LPWSTR   pFormName,
	DWORD   Level,
	LPBYTE  pForm,
	DWORD   cbBuf,
	LPDWORD pcbNeeded
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetFormW");
	return 0;
}

WINAPI OGetICMProfileW(HDC,LPDWORD,LPWSTR)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetICMProfileW");
	return 0;
}

BOOL
WINAPI
OGetJobW(
   HANDLE   hPrinter,
   DWORD    JobId,
   DWORD    Level,
   LPBYTE   pJob,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetJobW");
	return 0;
}

BOOL
WINAPI
OGetKeyboardLayoutNameW(
	LPWSTR pwszKLID)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetKeyboardLayoutNameW");
	return 0;
}

WINAPI OGetLogColorSpaceW(HCOLORSPACE,LPLOGCOLORSPACEW,DWORD)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetLogColorSpaceW");
	return 0;
}

DWORD
WINAPI
OGetLogicalDriveStringsW(
	DWORD nBufferLength,
	LPWSTR lpBuffer
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetLogicalDriveStringsW");
	return 0;
}

BOOL
WINAPI
OGetMenuItemInfoW(
	HMENU,
	UINT,
	BOOL,
	LPMENUITEMINFOW
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetMenuItemInfoW");
	return 0;
}

HMETAFILE   WINAPI OGetMetaFileW(LPCWSTR)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetMetaFileW");
	return 0;
}

INT
APIENTRY
OGetNameByTypeW (
	IN     LPGUID          lpServiceType,
	IN OUT LPWSTR         lpServiceName,
	IN     DWORD           dwNameLength
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetNameByTypeW");
	return 0;
}

BOOL
WINAPI
OGetNamedPipeHandleStateW(
	HANDLE hNamedPipe,
	LPDWORD lpState,
	LPDWORD lpCurInstances,
	LPDWORD lpMaxCollectionCount,
	LPDWORD lpCollectDataTimeout,
	LPWSTR lpUserName,
	DWORD nMaxUserNameSize
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetNamedPipeHandleStateW");
	return 0;
}

int
WINAPI
OGetNumberFormatW(
	LCID     Locale,
	DWORD    dwFlags,
	LPCWSTR lpValue,
	CONST NUMBERFMTW *lpFormat,
	LPWSTR  lpNumberStr,
	int      cchNumber)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetNumberFormatW");
	return 0;
}

BOOL
WINAPI
OGetPrinterW(
	HANDLE  hPrinter,
	DWORD   Level,
	LPBYTE  pPrinter,
	DWORD   cbBuf,
	LPDWORD pcbNeeded
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetPrinterW");
	return 0;
}

DWORD
WINAPI
OGetPrinterDataW(
	HANDLE   hPrinter,
	LPWSTR    pValueName,
	LPDWORD  pType,
	LPBYTE   pData,
	DWORD    nSize,
	LPDWORD  pcbNeeded
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetPrinterDataW");
	return 0;
}

BOOL
WINAPI
OGetPrinterDriverW(
	HANDLE  hPrinter,
	LPWSTR   pEnvironment,
	DWORD   Level,
	LPBYTE  pDriverInfo,
	DWORD   cbBuf,
	LPDWORD pcbNeeded
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetPrinterDriverW");
	return 0;
}

BOOL
WINAPI
OGetPrinterDriverDirectoryW(
	LPWSTR   pName,
	LPWSTR   pEnvironment,
	DWORD   Level,
	LPBYTE  pDriverDirectory,
	DWORD   cbBuf,
	LPDWORD pcbNeeded
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetPrinterDriverDirectoryW");
	return 0;
}

BOOL
WINAPI
OGetPrintProcessorDirectoryW(
	LPWSTR   pName,
	LPWSTR   pEnvironment,
	DWORD   Level,
	LPBYTE  pPrintProcessorInfo,
	DWORD   cbBuf,
	LPDWORD pcbNeeded
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetPrintProcessorDirectoryW");
	return 0;
}

DWORD
WINAPI
OGetPrivateProfileSectionW(
	LPCWSTR lpAppName,
	LPWSTR lpReturnedString,
	DWORD nSize,
	LPCWSTR lpFileName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetPrivateProfileSectionW");
	return 0;
}

DWORD
WINAPI
OGetPrivateProfileSectionNamesW(
	LPWSTR lpszReturnBuffer,
	DWORD nSize,
	LPCWSTR lpFileName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetPrivateProfileSectionNamesW");
	return 0;
}

BOOL
WINAPI
OGetPrivateProfileStructW(
	LPCWSTR lpszSection,
	LPCWSTR lpszKey,
	LPVOID   lpStruct,
	UINT     uSizeStruct,
	LPCWSTR szFile
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetPrivateProfileStructW");
	return 0;
}

DWORD
WINAPI
OGetProfileSectionW(
	LPCWSTR lpAppName,
	LPWSTR lpReturnedString,
	DWORD nSize
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetProfileSectionW");
	return 0;
}

DWORD
WINAPI
OGetProfileStringW(
	LPCWSTR lpAppName,
	LPCWSTR lpKeyName,
	LPCWSTR lpDefault,
	LPWSTR lpReturnedString,
	DWORD nSize
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetProfileStringW");
	return 0;
}

#if 0 //$UNDONE_POST_98 - We should wrap these as being NT only...
BOOL
WINAPI
OGetServiceDisplayNameW(
	SC_HANDLE               hSCManager,
	LPCWSTR                lpServiceName,
	LPWSTR                 lpDisplayName,
	LPDWORD                 lpcchBuffer
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetServiceDisplayNameW");
	return 0;
}

BOOL
WINAPI
OGetServiceKeyNameW(
	SC_HANDLE               hSCManager,
	LPCWSTR                lpDisplayName,
	LPWSTR                 lpServiceName,
	LPDWORD                 lpcchBuffer
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetServiceKeyNameW");
	return 0;
}
#endif

DWORD
WINAPI
OGetShortPathNameW(
	LPCWSTR lpszLongPath,
	LPWSTR  lpszShortPath,
	DWORD    cchBuffer
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetShortPathNameW");
	return 0;
}

VOID
WINAPI
OGetStartupInfoW(
	LPSTARTUPINFOW lpStartupInfo
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetStartupInfoW");
	return;
}

BOOL
WINAPI
OGetStringTypeExW(
	LCID     Locale,
	DWORD    dwInfoType,
	LPCWSTR lpSrcStr,
	int      cchSrc,
	LPWORD   lpCharType)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetStringTypeExW");
	return 0;
}

UINT
WINAPI
OGetSystemDirectoryW(
	LPWSTR lpBuffer,
	UINT uSize
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetSystemDirectoryW");
	return 0;
}

int
WINAPI
OGetTimeFormatW(
	LCID     Locale,
	DWORD    dwFlags,
	CONST SYSTEMTIME *lpTime,
	LPCWSTR lpFormat,
	LPWSTR  lpTimeStr,
	int      cchTime)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetTimeFormatW");
	return 0;
}

INT
APIENTRY
OGetTypeByNameW (
	IN     LPWSTR         lpServiceName,
	IN OUT LPGUID          lpServiceType
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetTypeByNameW");
	return 0;
}

BOOL
WINAPI
OGetUserObjectInformationW(
	HANDLE hObj,
	int nIndex,
	PVOID pvInfo,
	DWORD nLength,
	LPDWORD lpnLengthNeeded)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetUserObjectInformationW");
	return 0;
}

UINT
WINAPI
OGetWindowsDirectoryW(
	LPWSTR lpBuffer,
	UINT uSize
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetWindowsDirectoryW");
	return 0;
}

ATOM
WINAPI
OGlobalFindAtomW(
	LPCWSTR lpString
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GlobalFindAtomW");
	return 0;
}

RPC_STATUS RPC_ENTRY
OI_RpcServerUnregisterEndpointW (
	IN unsigned short * Protseq,
	IN unsigned short * Endpoint
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - I_RpcServerUnregisterEndpointW");
	return 0;
}

HIMAGELIST
WINAPI
OImageList_LoadImageW(HINSTANCE hi, LPCWSTR lpbmp, int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImageList_LoadImageW");
	return 0;
}

WINAPI
OImmConfigureIMEW(HKL, HWND, DWORD, LPVOID)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmConfigureIMEW");
	return 0;
}

WINAPI
OImmEnumRegisterWordW(HKL, REGISTERWORDENUMPROCW, LPCWSTR lpszReading, DWORD, LPCWSTR lpszRegister, LPVOID)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmEnumRegisterWordW");
	return 0;
}

WINAPI
OImmEscapeW(HKL, HIMC, UINT, LPVOID)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmEscapeW");
	return 0;
}

WINAPI
OImmGetCandidateListW(HIMC, DWORD deIndex, LPCANDIDATELIST, DWORD dwBufLen)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmGetCandidateListW");
	return 0;
}

WINAPI
OImmGetCandidateListCountW(HIMC, LPDWORD lpdwListCount)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmGetCandidateListCountW");
	return 0;
}

WINAPI
OImmGetCompositionFontW(HIMC, LPLOGFONTW)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmGetCompositionFontW");
	return 0;
}

WINAPI
OImmGetCompositionStringW(HIMC, DWORD, LPVOID, DWORD)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmGetCompositionStringW");
	return 0;
}

WINAPI
OImmGetConversionListW(HKL, HIMC, LPCWSTR, LPCANDIDATELIST, DWORD dwBufLen, UINT uFlag)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmGetConversionListW");
	return 0;
}

WINAPI
OImmGetDescriptionW(HKL, LPWSTR, UINT uBufLen)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmGetDescriptionW");
	return 0;
}

WINAPI
OImmGetGuideLineW(HIMC, DWORD dwIndex, LPWSTR, DWORD dwBufLen)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmGetGuideLineW");
	return 0;
}

WINAPI
OImmGetIMEFileNameW(HKL, LPWSTR, UINT uBufLen)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmGetIMEFileNameW");
	return 0;
}

WINAPI
OImmGetRegisterWordStyleW(HKL, UINT nItem, LPSTYLEBUFW)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmGetRegisterWordStyleW");
	return 0;
}

WINAPI
OImmInstallIMEW(LPCWSTR lpszIMEFileName, LPCWSTR lpszLayoutText)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmInstallIMEW");
	return 0;
}

WINAPI
OImmIsUIMessageW(HWND, UINT, WPARAM, LPARAM)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmIsUIMessageW");
	return 0;
}

WINAPI
OImmRegisterWordW(HKL, LPCWSTR lpszReading, DWORD, LPCWSTR lpszRegister)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmRegisterWordW");
	return 0;
}

WINAPI
OImmSetCompositionFontW(HIMC, LPLOGFONTW)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmSetCompositionFontW");
	return 0;
}

WINAPI
OImmSetCompositionStringW(HIMC, DWORD dwIndex, LPCVOID lpComp, DWORD, LPCVOID lpRead, DWORD)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmSetCompositionStringW");
	return 0;
}

WINAPI
OImmUnregisterWordW(HKL, LPCWSTR lpszReading, DWORD, LPCWSTR lpszUnregister)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ImmUnregisterWordW");
	return 0;
}

BOOL
APIENTRY
OInitiateSystemShutdownW(
	LPWSTR lpMachineName,
	LPWSTR lpMessage,
	DWORD dwTimeout,
	BOOL bForceAppsClosed,
	BOOL bRebootAfterShutdown
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - InitiateSystemShutdownW");
	return 0;
}

BOOL
WINAPI
OInsertMenuItemW(
	HMENU,
	UINT,
	BOOL,
	LPCMENUITEMINFOW
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - InsertMenuItemW");
	return 0;
}

BOOL
WINAPI
OIsCharLowerW(
	WCHAR ch)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - IsCharLowerW");
	return 0;
}

BOOL
WINAPI
OIsCharUpperW(
	WCHAR ch)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - IsCharUpperW");
	return 0;
}

MMRESULT
WINAPI
OjoyGetDevCapsW(UINT uJoyID, LPJOYCAPSW pjc, UINT cbjc)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - joyGetDevCapsW");
	return 0;
}

HCURSOR
WINAPI
OLoadCursorFromFileW(
	LPCWSTR    lpFileName)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - LoadCursorFromFileW");
	return 0;
}

HKL
WINAPI
OLoadKeyboardLayoutW(
	LPCWSTR pwszKLID,
	UINT Flags)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - LoadKeyboardLayoutW");
	return 0;
}

BOOL
WINAPI
OLogonUserW (
	LPWSTR lpszUsername,
	LPWSTR lpszDomain,
	LPWSTR lpszPassword,
	DWORD dwLogonType,
	DWORD dwLogonProvider,
	PHANDLE phToken
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - LogonUserW");
	return 0;
}

INT
APIENTRY
OLZOpenFileW(
	LPWSTR,
	LPOFSTRUCT,
	WORD
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - LZOpenFileW");
	return 0;
}

UINT
WINAPI
OMapVirtualKeyExW(
	UINT uCode,
	UINT uMapType,
	HKL dwhkl)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - MapVirtualKeyExW");
	return 0;
}

HRESULT
WINAPI
OMIMEAssociationDialogW(HWND hwndParent,
									   DWORD dwInFlags,
									   PCWSTR pcszFile,
									   PCWSTR pcszMIMEContentType,
									   PWSTR pszAppBuf,
									   UINT ucAppBufLen)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - MIMEAssociationDialogW");
	return 0;
}

DWORD
APIENTRY
OMultinetGetConnectionPerformanceW(
		LPNETRESOURCEW lpNetResource,
		LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct
		)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - MultinetGetConnectionPerformanceW");
	return 0;
}

BOOL
WINAPI
OObjectCloseAuditAlarmW (
	LPCWSTR SubsystemName,
	LPVOID HandleId,
	BOOL GenerateOnClose
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ObjectCloseAuditAlarmW");
	return 0;
}

BOOL
WINAPI
OObjectOpenAuditAlarmW (
	LPCWSTR SubsystemName,
	LPVOID HandleId,
	LPWSTR ObjectTypeName,
	LPWSTR ObjectName,
	PSECURITY_DESCRIPTOR pSecurityDescriptor,
	HANDLE ClientToken,
	DWORD DesiredAccess,
	DWORD GrantedAccess,
	PPRIVILEGE_SET Privileges,
	BOOL ObjectCreation,
	BOOL AccessGranted,
	LPBOOL GenerateOnClose
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ObjectOpenAuditAlarmW");
	return 0;
}

BOOL
WINAPI
OObjectPrivilegeAuditAlarmW (
	LPCWSTR SubsystemName,
	LPVOID HandleId,
	HANDLE ClientToken,
	DWORD DesiredAccess,
	PPRIVILEGE_SET Privileges,
	BOOL AccessGranted
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ObjectPrivilegeAuditAlarmW");
	return 0;
}

BOOL
WINAPI
OOemToCharBuffW(
	LPCSTR lpszSrc,
	LPWSTR lpszDst,
	DWORD cchDstLength)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - OemToCharBuffW");
	return 0;
}

HANDLE
WINAPI
OOpenBackupEventLogW (
	LPCWSTR lpUNCServerName,
	LPCWSTR lpFileName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - OpenBackupEventLogW");
	return 0;
}

HDESK
WINAPI
OOpenDesktopW(
	LPWSTR lpszDesktop,
	DWORD dwFlags,
	BOOL fInherit,
	DWORD dwDesiredAccess)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - OpenDesktopW");
	return 0;
}

HANDLE
WINAPI
OOpenEventW(
	DWORD dwDesiredAccess,
	BOOL bInheritHandle,
	LPCWSTR lpName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - OpenEventW");
	return 0;
}

HANDLE
WINAPI
OOpenEventLogW (
	LPCWSTR lpUNCServerName,
	LPCWSTR lpSourceName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - OpenEventLogW");
	return 0;
}

HANDLE
WINAPI
OOpenFileMappingW(
	DWORD dwDesiredAccess,
	BOOL bInheritHandle,
	LPCWSTR lpName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - OpenFileMappingW");
	return 0;
}

HANDLE
WINAPI
OOpenMutexW(
	DWORD dwDesiredAccess,
	BOOL bInheritHandle,
	LPCWSTR lpName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - OpenMutexW");
	return 0;
}

BOOL
WINAPI
OOpenPrinterW(
	LPWSTR    pPrinterName,
	LPHANDLE phPrinter,
	LPPRINTER_DEFAULTSW pDefault
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - OpenPrinterW");
	return 0;
}

#if 0 //$UNDONE_POST_98 - We should wrap these as being NT only...
SC_HANDLE
WINAPI
OOpenSCManagerW(
	LPCWSTR lpMachineName,
	LPCWSTR lpDatabaseName,
	DWORD   dwDesiredAccess
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - OpenSCManagerW");
	return 0;
}
#endif

HANDLE
WINAPI
OOpenSemaphoreW(
	DWORD dwDesiredAccess,
	BOOL bInheritHandle,
	LPCWSTR lpName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - OpenSemaphoreW");
	return 0;
}

#if 0 //$UNDONE_POST_98 - We should wrap these as being NT only...
SC_HANDLE
WINAPI
OOpenServiceW(
	SC_HANDLE   hSCManager,
	LPCWSTR    lpServiceName,
	DWORD       dwDesiredAccess
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - OpenServiceW");
	return 0;
}
#endif

HWINSTA
WINAPI
OOpenWindowStationW(
	LPWSTR lpszWinSta,
	BOOL fInherit,
	DWORD dwDesiredAccess)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - OpenWindowStationW");
	return 0;
}

APIENTRY OPageSetupDlgW( LPPAGESETUPDLGW )
{
	AssertFail("No Unicode Wrapper Available for Win32 API - PageSetupDlgW");
	return 0;
}

BOOL
WINAPI
OPeekConsoleInputW(
	HANDLE hConsoleInput,
	PINPUT_RECORD lpBuffer,
	DWORD nLength,
	LPDWORD lpNumberOfEventsRead
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - PeekConsoleInputW");
	return 0;
}

BOOL
WINAPI
OPolyTextOutW(HDC, CONST POLYTEXTW *, int)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - PolyTextOutW");
	return 0;
}

APIENTRY
OPrintDlgW(LPPRINTDLGW lppd)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - PrintDlgW");
	return 0;
}

DWORD
WINAPI
OPrinterMessageBoxW(
	HANDLE  hPrinter,
	DWORD   Error,
	HWND    hWnd,
	LPWSTR   pText,
	LPWSTR   pCaption,
	DWORD   dwType
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - PrinterMessageBoxW");
	return 0;
}

BOOL
WINAPI
OPrivilegedServiceAuditAlarmW (
	LPCWSTR SubsystemName,
	LPCWSTR ServiceName,
	HANDLE ClientToken,
	PPRIVILEGE_SET Privileges,
	BOOL AccessGranted
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - PrivilegedServiceAuditAlarmW");
	return 0;
}

int
WINAPI
OPropertySheetW(
	LPCPROPSHEETHEADERW lpcpsh
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - PropertySheetW");
	return 0;
}

DWORD
WINAPI
OQueryDosDeviceW(
	LPCWSTR lpDeviceName,
	LPWSTR lpTargetPath,
	DWORD ucchMax
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - QueryDosDeviceW");
	return 0;
}

#if 0 //$UNDONE_POST_98 - We should wrap these as being NT only...
BOOL
WINAPI
OQueryServiceConfigW(
	SC_HANDLE               hService,
	LPQUERY_SERVICE_CONFIGW lpServiceConfig,
	DWORD                   cbBufSize,
	LPDWORD                 pcbBytesNeeded
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - QueryServiceConfigW");
	return 0;
}

BOOL
WINAPI
OQueryServiceLockStatusW(
	SC_HANDLE                       hSCManager,
	LPQUERY_SERVICE_LOCK_STATUSW    lpLockStatus,
	DWORD                           cbBufSize,
	LPDWORD                         pcbBytesNeeded
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - QueryServiceLockStatusW");
	return 0;
}
#endif

BOOL
WINAPI
OReadConsoleW(
	HANDLE hConsoleInput,
	LPVOID lpBuffer,
	DWORD nNumberOfCharsToRead,
	LPDWORD lpNumberOfCharsRead,
	LPVOID lpReserved
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ReadConsoleW");
	return 0;
}

BOOL
WINAPI
OReadConsoleInputW(
	HANDLE hConsoleInput,
	PINPUT_RECORD lpBuffer,
	DWORD nLength,
	LPDWORD lpNumberOfEventsRead
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ReadConsoleInputW");
	return 0;
}

BOOL
WINAPI
OReadConsoleOutputW(
	HANDLE hConsoleOutput,
	PCHAR_INFO lpBuffer,
	COORD dwBufferSize,
	COORD dwBufferCoord,
	PSMALL_RECT lpReadRegion
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ReadConsoleOutputW");
	return 0;
}

BOOL
WINAPI
OReadConsoleOutputCharacterW(
	HANDLE hConsoleOutput,
	LPWSTR lpCharacter,
	DWORD nLength,
	COORD dwReadCoord,
	LPDWORD lpNumberOfCharsRead
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ReadConsoleOutputCharacterW");
	return 0;
}

BOOL
WINAPI
OReadEventLogW (
	 HANDLE     hEventLog,
	 DWORD      dwReadFlags,
	 DWORD      dwRecordOffset,
	 LPVOID     lpBuffer,
	 DWORD      nNumberOfBytesToRead,
	 DWORD      *pnBytesRead,
	 DWORD      *pnMinNumberOfBytesNeeded
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ReadEventLogW");
	return 0;
}

LONG
APIENTRY
ORegConnectRegistryW (
	LPWSTR lpMachineName,
	HKEY hKey,
	PHKEY phkResult
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RegConnectRegistryW");
	return 0;
}

HANDLE
WINAPI
ORegisterEventSourceW (
	LPCWSTR lpUNCServerName,
	LPCWSTR lpSourceName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RegisterEventSourceW");
	return 0;
}

#if 0 //$UNDONE_POST_98 - We should wrap these as being NT only...
SERVICE_STATUS_HANDLE
WINAPI
ORegisterServiceCtrlHandlerW(
	LPCWSTR             lpServiceName,
	LPHANDLER_FUNCTION   lpHandlerProc
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RegisterServiceCtrlHandlerW");
	return 0;
}
#endif

LONG
APIENTRY
ORegLoadKeyW (
	HKEY    hKey,
	LPCWSTR  lpSubKey,
	LPCWSTR  lpFile
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RegLoadKeyW");
	return 0;
}

LONG
APIENTRY
ORegQueryMultipleValuesW (
	HKEY hKey,
	PVALENTW val_list,
	DWORD num_vals,
	LPWSTR lpValueBuf,
	LPDWORD ldwTotsize
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RegQueryMultipleValuesW");
	return 0;
}

LONG
APIENTRY
ORegReplaceKeyW (
	HKEY     hKey,
	LPCWSTR  lpSubKey,
	LPCWSTR  lpNewFile,
	LPCWSTR  lpOldFile
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RegReplaceKeyW");
	return 0;
}

LONG
APIENTRY
ORegRestoreKeyW (
	HKEY hKey,
	LPCWSTR lpFile,
	DWORD   dwFlags
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RegRestoreKeyW");
	return 0;
}

LONG
APIENTRY
ORegSaveKeyW (
	HKEY hKey,
	LPCWSTR lpFile,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RegSaveKeyW");
	return 0;
}

LONG
APIENTRY
ORegUnLoadKeyW (
	HKEY    hKey,
	LPCWSTR lpSubKey
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RegUnLoadKeyW");
	return 0;
}

BOOL
WINAPI
ORemoveDirectoryW(
	LPCWSTR lpPathName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RemoveDirectoryW");
	return 0;
}

BOOL 
WINAPI 
ORemoveFontResourceW(LPCWSTR)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RemoveFontResourceW");
	return 0;
}

APIENTRY
OReplaceTextW(LPFINDREPLACEW)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ReplaceTextW");
	return 0;
}

BOOL
WINAPI
OReportEventW (
	 HANDLE     hEventLog,
	 WORD       wType,
	 WORD       wCategory,
	 DWORD      dwEventID,
	 PSID       lpUserSid,
	 WORD       wNumStrings,
	 DWORD      dwDataSize,
	 LPCWSTR   *lpStrings,
	 LPVOID     lpRawData
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ReportEventW");
	return 0;
}

HDC  
WINAPI
OResetDCW(
	HDC hdc,
	CONST DEVMODEW *lpInitData)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ResetDCW");
	return 0;
}

BOOL
WINAPI
OResetPrinterW(
   HANDLE   hPrinter,
   LPPRINTER_DEFAULTSW pDefault
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ResetPrinterW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcBindingFromStringBindingW (
	IN unsigned short __RPC_FAR * StringBinding,
	OUT RPC_BINDING_HANDLE __RPC_FAR * Binding
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcBindingFromStringBindingW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcBindingInqAuthClientW (
	IN RPC_BINDING_HANDLE ClientBinding, OPTIONAL
	OUT RPC_AUTHZ_HANDLE __RPC_FAR * Privs,
	OUT unsigned short __RPC_FAR * __RPC_FAR * ServerPrincName, OPTIONAL
	OUT unsigned long __RPC_FAR * AuthnLevel, OPTIONAL
	OUT unsigned long __RPC_FAR * AuthnSvc, OPTIONAL
	OUT unsigned long __RPC_FAR * AuthzSvc OPTIONAL
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcBindingInqAuthClientW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcBindingToStringBindingW (
	IN RPC_BINDING_HANDLE Binding,
	OUT unsigned short __RPC_FAR * __RPC_FAR * StringBinding
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcBindingToStringBindingW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcEpRegisterNoReplaceW (
	IN RPC_IF_HANDLE IfSpec,
	IN RPC_BINDING_VECTOR * BindingVector,
	IN UUID_VECTOR * UuidVector OPTIONAL,
	IN unsigned short  * Annotation
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcEpRegisterNoReplaceW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcMgmtEpEltInqNextW (
	IN RPC_EP_INQ_HANDLE InquiryContext,
	OUT RPC_IF_ID __RPC_FAR * IfId,
	OUT RPC_BINDING_HANDLE __RPC_FAR * Binding OPTIONAL,
	OUT UUID __RPC_FAR * ObjectUuid OPTIONAL,
	OUT unsigned short __RPC_FAR * __RPC_FAR * Annotation OPTIONAL
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcMgmtEpEltInqNextW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcMgmtInqServerPrincNameW (
	IN RPC_BINDING_HANDLE Binding,
	IN unsigned long AuthnSvc,
	OUT unsigned short __RPC_FAR * __RPC_FAR * ServerPrincName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcMgmtInqServerPrincNameW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcNetworkInqProtseqsW (
	OUT RPC_PROTSEQ_VECTORW __RPC_FAR * __RPC_FAR * ProtseqVector
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcNetworkInqProtseqsW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcNetworkIsProtseqValidW (
	IN unsigned short __RPC_FAR * Protseq
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcNetworkIsProtseqValidW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcNsBindingInqEntryNameW (
	IN RPC_BINDING_HANDLE Binding,
	IN unsigned long EntryNameSyntax,
	OUT unsigned short __RPC_FAR * __RPC_FAR * EntryName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcNsBindingInqEntryNameW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcProtseqVectorFreeW (
	IN OUT RPC_PROTSEQ_VECTORW __RPC_FAR * __RPC_FAR * ProtseqVector
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcProtseqVectorFreeW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcServerInqDefaultPrincNameW (
	IN unsigned long AuthnSvc,
	OUT unsigned short __RPC_FAR * __RPC_FAR * PrincName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcServerInqDefaultPrincNameW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcServerUseProtseqW (
	IN unsigned short __RPC_FAR * Protseq,
	IN unsigned int MaxCalls,
	IN void __RPC_FAR * SecurityDescriptor OPTIONAL
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcServerUseProtseqW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcServerUseProtseqEpW (
	IN unsigned short __RPC_FAR * Protseq,
	IN unsigned int MaxCalls,
	IN unsigned short __RPC_FAR * Endpoint,
	IN void __RPC_FAR * SecurityDescriptor OPTIONAL
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcServerUseProtseqEpW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcServerUseProtseqIfW (
	IN unsigned short __RPC_FAR * Protseq,
	IN unsigned int MaxCalls,
	IN RPC_IF_HANDLE IfSpec,
	IN void __RPC_FAR * SecurityDescriptor OPTIONAL
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcServerUseProtseqIfW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcStringBindingComposeW (
	IN unsigned short __RPC_FAR * ObjUuid OPTIONAL,
	IN unsigned short __RPC_FAR * Protseq OPTIONAL,
	IN unsigned short __RPC_FAR * NetworkAddr OPTIONAL,
	IN unsigned short __RPC_FAR * Endpoint OPTIONAL,
	IN unsigned short __RPC_FAR * Options OPTIONAL,
	OUT unsigned short __RPC_FAR * __RPC_FAR * StringBinding OPTIONAL
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcStringBindingComposeW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcStringBindingParseW (
	IN unsigned short __RPC_FAR * StringBinding,
	OUT unsigned short __RPC_FAR * __RPC_FAR * ObjUuid OPTIONAL,
	OUT unsigned short __RPC_FAR * __RPC_FAR * Protseq OPTIONAL,
	OUT unsigned short __RPC_FAR * __RPC_FAR * NetworkAddr OPTIONAL,
	OUT unsigned short __RPC_FAR * __RPC_FAR * Endpoint OPTIONAL,
	OUT unsigned short __RPC_FAR * __RPC_FAR * NetworkOptions OPTIONAL
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcStringBindingParseW");
	return 0;
}

RPC_STATUS RPC_ENTRY
ORpcStringFreeW (
	IN OUT unsigned short __RPC_FAR * __RPC_FAR * String
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - RpcStringFreeW");
	return 0;
}

BOOL
WINAPI
OScrollConsoleScreenBufferW(
	HANDLE hConsoleOutput,
	CONST SMALL_RECT *lpScrollRectangle,
	CONST SMALL_RECT *lpClipRectangle,
	COORD dwDestinationOrigin,
	CONST CHAR_INFO *lpFill
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ScrollConsoleScreenBufferW");
	return 0;
}

DWORD
WINAPI
OSearchPathW(
	LPCWSTR lpPath,
	LPCWSTR lpFileName,
	LPCWSTR lpExtension,
	DWORD nBufferLength,
	LPWSTR lpBuffer,
	LPWSTR *lpFilePart
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SearchPathW");
	return 0;
}

BOOL
WINAPI
OSendMessageCallbackW(
	HWND hWnd,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam,
	SENDASYNCPROC lpResultCallBack,
	DWORD dwData)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SendMessageCallbackW");
	return 0;
}

LRESULT
WINAPI
OSendMessageTimeoutW(
	HWND hWnd,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam,
	UINT fuFlags,
	UINT uTimeout,
	LPDWORD lpdwResult)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SendMessageTimeoutW");
	return 0;
}

BOOL
WINAPI
OSetComputerNameW (
	LPCWSTR lpComputerName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SetComputerNameW");
	return 0;
}

BOOL
WINAPI
OSetConsoleTitleW(
	LPCWSTR lpConsoleTitle
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SetConsoleTitleW");
	return 0;
}

BOOL
WINAPI
OSetCurrentDirectoryW(
	LPCWSTR lpPathName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SetCurrentDirectoryW");
	return 0;
}

BOOL
WINAPI
OSetDefaultCommConfigW(
	LPCWSTR lpszName,
	LPCOMMCONFIG lpCC,
	DWORD dwSize
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SetDefaultCommConfigW");
	return 0;
}

BOOL
WINAPI
OSetEnvironmentVariableW(
	LPCWSTR lpName,
	LPCWSTR lpValue
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SetEnvironmentVariableW");
	return 0;
}

BOOL
WINAPI
OSetFileSecurityW (
	LPCWSTR lpFileName,
	SECURITY_INFORMATION SecurityInformation,
	PSECURITY_DESCRIPTOR pSecurityDescriptor
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SetFileSecurityW");
	return 0;
}

BOOL
WINAPI
OSetFormW(
	HANDLE  hPrinter,
	LPWSTR   pFormName,
	DWORD   Level,
	LPBYTE  pForm
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SetFormW");
	return 0;
}

WINAPI
OSetICMProfileW(HDC,LPWSTR)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SetICMProfileW");
	return 0;
}

BOOL
WINAPI
OSetJobW(
	HANDLE  hPrinter,
	DWORD   JobId,
	DWORD   Level,
	LPBYTE  pJob,
	DWORD   Command
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SetJobW");
	return 0;
}

BOOL
WINAPI
OSetPrinterW(
	HANDLE  hPrinter,
	DWORD   Level,
	LPBYTE  pPrinter,
	DWORD   Command
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SetPrinterW");
	return 0;
}

DWORD
WINAPI
OSetPrinterDataW(
	HANDLE  hPrinter,
	LPWSTR   pValueName,
	DWORD   Type,
	LPBYTE  pData,
	DWORD   cbData
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SetPrinterDataW");
	return 0;
}

BOOL
WINAPI
OSetUserObjectInformationW(
	HANDLE hObj,
	int nIndex,
	PVOID pvInfo,
	DWORD nLength)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SetUserObjectInformationW");
	return 0;
}

BOOL
WINAPI
OSetVolumeLabelW(
	LPCWSTR lpRootPathName,
	LPCWSTR lpVolumeName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SetVolumeLabelW");
	return 0;
}

HHOOK
WINAPI
OSetWindowsHookW(
	int nFilterType,
	HOOKPROC pfnFilterProc)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SetWindowsHookW");
	return 0;
}

LPITEMIDLIST
WINAPI
OSHBrowseForFolderW(
	LPBROWSEINFO lpbi)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SHBrowseForFolderW");
	return 0;
}

BOOL
WINAPI
OShell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW lpData)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - Shell_NotifyIconW");
	return 0;
}

INT
APIENTRY
OShellAboutW(HWND hWnd, LPCWSTR szApp, LPCWSTR szOtherStuff, HICON hIcon)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ShellAboutW");
	return 0;
}

HINSTANCE
APIENTRY
OShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ShellExecuteW");
	return 0;
}

BOOL
WINAPI
OShellExecuteExW(
	LPSHELLEXECUTEINFOW lpExecInfo)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - ShellExecuteExW");
	return 0;
}


int 
WINAPI
OSHFileOperationW(LPSHFILEOPSTRUCTW lpFileOp)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SHFileOperationW");
	return 0;
}

DWORD
WINAPI
OSHGetFileInfoW(LPCWSTR pszPath, DWORD dwFileAttributes, SHFILEINFOW FAR *psfi, UINT cbFileInfo, UINT uFlags)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SHGetFileInfoW");
	return 0;
}

BOOL
WINAPI
OSHGetNewLinkInfoW(LPCWSTR pszLinkTo, LPCWSTR pszDir, LPWSTR pszName,
							 BOOL FAR * pfMustCopy, UINT uFlags)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SHGetNewLinkInfoW");
	return 0;
}

BOOL
WINAPI
OSHGetPathFromIDListW(
	LPCITEMIDLIST pidl,
	LPTSTR pszPath)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - SHGetPathFromIDListW");
	return 0;
}

BOOL
WINAPI
OsndPlaySoundW(LPCWSTR pszSound, UINT fuSound)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - sndPlaySoundW");
	return 0;
}

DWORD
WINAPI
OStartDocPrinterW(
	HANDLE  hPrinter,
	DWORD   Level,
	LPBYTE  pDocInfo
)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - StartDocPrinterW");
	return 0;
}

#if 0 //$UNDONE_POST_98 - We should wrap these as being NT only...
BOOL
WINAPI
OStartServiceW(
	SC_HANDLE            hService,
	DWORD                dwNumServiceArgs,
	LPCWSTR             *lpServiceArgVectors
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - StartServiceW");
	return 0;
}

BOOL
WINAPI
OStartServiceCtrlDispatcherW(
	LPSERVICE_TABLE_ENTRYW    lpServiceStartTable
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - StartServiceCtrlDispatcherW");
	return 0;
}
#endif

// Commented since gdi32.dll on Win95 provides the wrapper for this function.
/*
BOOL
WINAPI
OTextOutW(HDC, int, int, LPCWSTR, int)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - TextOutW");
	return 0;
}
*/

HRESULT
WINAPI
OTranslateURLW(PCWSTR pcszURL,
										 DWORD dwInFlags,
										 PWSTR *ppszTranslatedURL)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - TranslateURLW");
	return 0;
}

WINAPI
OUpdateICMRegKeyW(DWORD, DWORD, LPWSTR, UINT)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - UpdateICMRegKeyW");
	return 0;
}

HRESULT
WINAPI
OURLAssociationDialogW(HWND hwndParent,
												 DWORD dwInFlags,
												 PCWSTR pcszFile,
												 PCWSTR pcszURL,
												 PWSTR pszAppBuf,
												 UINT ucAppBufLen)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - URLAssociationDialogW");
	return 0;
}

/* client/server */
RPC_STATUS RPC_ENTRY
OUuidFromStringW (
	IN unsigned short __RPC_FAR * StringUuid,
	OUT UUID __RPC_FAR * Uuid
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - UuidFromStringW");
	return 0;
}

DWORD
APIENTRY
OVerFindFileW(
		DWORD uFlags,
		LPWSTR szFileName,
		LPWSTR szWinDir,
		LPWSTR szAppDir,
		LPWSTR szCurDir,
		PUINT lpuCurDirLen,
		LPWSTR szDestDir,
		PUINT lpuDestDirLen
		)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - VerFindFileW");
	return 0;
}

DWORD
APIENTRY
OVerInstallFileW(
		DWORD uFlags,
		LPWSTR szSrcFileName,
		LPWSTR szDestFileName,
		LPWSTR szSrcDir,
		LPWSTR szDestDir,
		LPWSTR szCurDir,
		LPWSTR szTmpFile,
		PUINT lpuTmpFileLen
		)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - VerInstallFileW");
	return 0;
}

DWORD
APIENTRY
OVerLanguageNameW(
		DWORD wLang,
		LPWSTR szLang,
		DWORD nSize
		)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - VerLanguageNameW");
	return 0;
}

BOOL
WINAPI
OVerQueryValueW(
	const LPVOID pBlock,
	LPWSTR lpSubBlock,
	LPVOID *lplpBuffer,
	PUINT puLerr)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - VerQueryValueW");
	return 0;
}

WINAPI
OVkKeyScanExW(
	WCHAR  ch,
	HKL   dwhkl)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - VkKeyScanExW");
	return 0;
}

BOOL
WINAPI
OWaitNamedPipeW(
	LPCWSTR lpNamedPipeName,
	DWORD nTimeOut
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WaitNamedPipeW");
	return 0;
}

MMRESULT
WINAPI
OwaveInGetDevCapsW(UINT uDeviceID, LPWAVEINCAPSW pwic, UINT cbwic)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - waveInGetDevCapsW");
	return 0;
}

MMRESULT
WINAPI
OwaveInGetErrorTextW(MMRESULT mmrError, LPWSTR pszText, UINT cchText)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - waveInGetErrorTextW");
	return 0;
}

MMRESULT
WINAPI
OwaveOutGetDevCapsW(UINT uDeviceID, LPWAVEOUTCAPSW pwoc, UINT cbwoc)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - waveOutGetDevCapsW");
	return 0;
}

MMRESULT
WINAPI
OwaveOutGetErrorTextW(MMRESULT mmrError, LPWSTR pszText, UINT cchText)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - waveOutGetErrorTextW");
	return 0;
}

BOOL
WINAPI
OwglUseFontBitmapsW(HDC, DWORD, DWORD, DWORD)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - wglUseFontBitmapsW");
	return 0;
}

BOOL
WINAPI
OwglUseFontOutlinesW(HDC, DWORD, DWORD, DWORD, FLOAT,
										   FLOAT, int, LPGLYPHMETRICSFLOAT)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - wglUseFontOutlinesW");
	return 0;
}

void 
WINAPI
OWinExecErrorW(HWND hwnd, int error, LPCWSTR lpstrFileName, LPCWSTR lpstrTitle)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WinExecErrorW");
	return;
}

DWORD 
APIENTRY
OWNetAddConnectionW(
	 LPCWSTR   lpRemoteName,
	 LPCWSTR   lpPassword,
	 LPCWSTR   lpLocalName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetAddConnectionW");
	return 0;
}

DWORD 
APIENTRY
OWNetAddConnection2W(
	 LPNETRESOURCEW lpNetResource,
	 LPCWSTR       lpPassword,
	 LPCWSTR       lpUserName,
	 DWORD          dwFlags
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetAddConnection2W");
	return 0;
}

DWORD 
APIENTRY
OWNetAddConnection3W(
	 HWND           hwndOwner,
	 LPNETRESOURCEW lpNetResource,
	 LPCWSTR       lpPassword,
	 LPCWSTR       lpUserName,
	 DWORD          dwFlags
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetAddConnection3W");
	return 0;
}

DWORD 
APIENTRY
OWNetCancelConnectionW(
	 LPCWSTR lpName,
	 BOOL     fForce
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetCancelConnectionW");
	return 0;
}

DWORD 
APIENTRY
OWNetCancelConnection2W(
	 LPCWSTR lpName,
	 DWORD    dwFlags,
	 BOOL     fForce
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetCancelConnection2W");
	return 0;
}

DWORD 
APIENTRY
OWNetConnectionDialog1W(
	LPCONNECTDLGSTRUCTW lpConnDlgStruct
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetConnectionDialog1W");
	return 0;
}

DWORD 
APIENTRY
OWNetDisconnectDialog1W(
	LPDISCDLGSTRUCTW lpConnDlgStruct
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetDisconnectDialog1W");
	return 0;
}

DWORD 
APIENTRY
OWNetEnumResourceW(
	 HANDLE  hEnum,
	 LPDWORD lpcCount,
	 LPVOID  lpBuffer,
	 LPDWORD lpBufferSize
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetEnumResourceW");
	return 0;
}

DWORD 
APIENTRY
OWNetGetConnectionW(
	 LPCWSTR lpLocalName,
	 LPWSTR  lpRemoteName,
	 LPDWORD  lpnLength
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetGetConnectionW");
	return 0;
}

DWORD 
APIENTRY
OWNetGetLastErrorW(
	 LPDWORD    lpError,
	 LPWSTR    lpErrorBuf,
	 DWORD      nErrorBufSize,
	 LPWSTR    lpNameBuf,
	 DWORD      nNameBufSize
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetGetLastErrorW");
	return 0;
}

DWORD 
APIENTRY
OWNetGetNetworkInformationW(
	LPCWSTR          lpProvider,
	LPNETINFOSTRUCT   lpNetInfoStruct
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetGetNetworkInformationW");
	return 0;
}

DWORD 
APIENTRY
OWNetGetProviderNameW(
	DWORD   dwNetType,
	LPWSTR lpProviderName,
	LPDWORD lpBufferSize
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetGetProviderNameW");
	return 0;
}

DWORD 
APIENTRY
OWNetGetUniversalNameW(
	 LPCWSTR lpLocalPath,
	 DWORD    dwInfoLevel,
	 LPVOID   lpBuffer,
	 LPDWORD  lpBufferSize
	 )
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetGetUniversalNameW");
	return 0;
}

DWORD 
APIENTRY
OWNetGetUserW(
	 LPCWSTR  lpName,
	 LPWSTR   lpUserName,
	 LPDWORD   lpnLength
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetGetUserW");
	return 0;
}

DWORD
APIENTRY
OWNetOpenEnumW(
	 DWORD          dwScope,
	 DWORD          dwType,
	 DWORD          dwUsage,
	 LPNETRESOURCEW lpNetResource,
	 LPHANDLE       lphEnum
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetOpenEnumW");
	return 0;
}

DWORD 
APIENTRY
OWNetSetConnectionW(
	LPCWSTR    lpName,
	DWORD       dwProperties,
	LPVOID      pvValues
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetSetConnectionW");
	return 0;
}

DWORD
APIENTRY
OWNetUseConnectionW(
	HWND            hwndOwner,
	LPNETRESOURCEW  lpNetResource,
	LPCWSTR        lpUserID,
	LPCWSTR        lpPassword,
	DWORD           dwFlags,
	LPWSTR         lpAccessName,
	LPDWORD         lpBufferSize,
	LPDWORD         lpResult
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WNetUseConnectionW");
	return 0;
}

BOOL
WINAPI
OWriteConsoleW(
	HANDLE hConsoleOutput,
	CONST VOID *lpBuffer,
	DWORD nNumberOfCharsToWrite,
	LPDWORD lpNumberOfCharsWritten,
	LPVOID lpReserved
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WriteConsoleW");
	return 0;
}

BOOL
WINAPI
OWriteConsoleInputW(
	HANDLE hConsoleInput,
	CONST INPUT_RECORD *lpBuffer,
	DWORD nLength,
	LPDWORD lpNumberOfEventsWritten
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WriteConsoleInputW");
	return 0;
}

BOOL
WINAPI
OWriteConsoleOutputW(
	HANDLE hConsoleOutput,
	CONST CHAR_INFO *lpBuffer,
	COORD dwBufferSize,
	COORD dwBufferCoord,
	PSMALL_RECT lpWriteRegion
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WriteConsoleOutputW");
	return 0;
}

BOOL
WINAPI
OWriteConsoleOutputCharacterW(
	HANDLE hConsoleOutput,
	LPCWSTR lpCharacter,
	DWORD nLength,
	COORD dwWriteCoord,
	LPDWORD lpNumberOfCharsWritten
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WriteConsoleOutputCharacterW");
	return 0;
}

BOOL
WINAPI
OWritePrivateProfileSectionW(
	LPCWSTR lpAppName,
	LPCWSTR lpString,
	LPCWSTR lpFileName
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WritePrivateProfileSectionW");
	return 0;
}

BOOL
WINAPI
OWritePrivateProfileStructW(
	LPCWSTR lpszSection,
	LPCWSTR lpszKey,
	LPVOID   lpStruct,
	UINT     uSizeStruct,
	LPCWSTR szFile
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WritePrivateProfileStructW");
	return 0;
}

BOOL
WINAPI
OWriteProfileSectionW(
	LPCWSTR lpAppName,
	LPCWSTR lpString
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WriteProfileSectionW");
	return 0;
}

BOOL
WINAPI
OWriteProfileStringW(
	LPCWSTR lpAppName,
	LPCWSTR lpKeyName,
	LPCWSTR lpString
	)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - WriteProfileStringW");
	return 0;
}

int
WINAPI
OwvsprintfW(
	LPWSTR,
	LPCWSTR,
	va_list arglist)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - wvsprintfW");
	return 0;
}

DWORD
WINAPI
ODdeQueryStringW(
	DWORD idInst,
	HSZ hsz,
	LPWSTR psz,
	DWORD cchMax,
	int iCodePage)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - DdeQueryStringW");
	return 0;
}

int WINAPI
OGetClipboardFormatNameW(
	UINT format,
	LPWSTR pwsz,
	int cchMaxCount)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetClipboardFormatNameW");
	return 0;
}

int
WINAPI
OGetKeyNameTextW(
	LONG lParam,
	LPWSTR lpString,
	int nSize)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetKeyNameTextW");
	return 0;
}

int
WINAPI
OGetMenuStringW(
	HMENU hMenu,
	UINT uIDItem,
	LPWSTR lpString,
	int nMaxCount,
	UINT uFlag)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetMenuStringW");
	return 0;
}

int
WINAPI
OGetTextFaceW(
	HDC    hdc,
	int    cch,
	LPWSTR lpFaceName)
{
	AssertFail("No Unicode Wrapper Available for Win32 API - GetMenuStringW");
	return 0;
}

#endif    //ifdef DEBUG

} // extern "C"

