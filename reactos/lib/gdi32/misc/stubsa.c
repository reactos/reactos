/* $Id: stubsa.c,v 1.2 2000/02/20 22:52:48 ea Exp $
 *
 * reactos/lib/gdi32/misc/stubs.c
 *
 * GDI32.DLL Stubs for ANSI functions
 *
 * When you implement one of these functions,
 * remove its stub from this file.
 *
 */
#ifdef UNICODE
#undef UNICODE
#endif
#include <windows.h>

static
LPWSTR
STDCALL
AnsiStringToUnicodeString (
	LPCSTR	AnsiString,
	LPWSTR	UnicodeString,
	BOOLEAN	AllocateBuffer
	)
{
	int	Length;
	LPWSTR	_UnicodeString = UnicodeString;

	if (	(NULL == UnicodeString)
		&& (FALSE == AllocateBuffer)
		)
	{
		return NULL;
	}
	Length = (lstrlenA (AnsiString) + 1);
	if (TRUE == AllocateBuffer)
	{
		_UnicodeString = LocalAlloc (
					LMEM_ZEROINIT,
					Length
					);
		if (NULL == _UnicodeString)
		{
			return NULL;
		}
	}
	Length = MultiByteToWideChar (
			CP_ACP,
			0,
			AnsiString,
			-1,
			_UnicodeString,
			Length
			);
	if (0 == Length)
	{
		return NULL;
	}
	return _UnicodeString;
}

	
int
STDCALL
AddFontResourceA(
	LPCSTR		a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HMETAFILE
STDCALL
CopyMetaFileA(
	HMETAFILE	a0,
	LPCSTR		a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HDC
STDCALL
CreateDCA (
	LPCSTR		lpszDriver,
	LPCSTR		lpszDevice,
	LPCSTR		lpszOutput,
	CONST DEVMODE	* lpInitData
	)
{
	LPCWSTR	lpwszDriver = NULL;
	LPCWSTR	lpwszDevice = NULL;
	LPCWSTR	lpwszOutput = NULL;
	HDC	hDC;

	/*
	 * If needed, convert to Unicode
	 * any string parameter.
	 */
	if (NULL != lpszDriver)
	{
		lpwszDriver = AnsiStringToUnicodeString (
				lpszDriver,
				NULL,
				TRUE
				);
		if (NULL == lpwszDriver)
		{
			return 0;
		}
	}
	if (NULL != lpszDevice)
	{
		lpwszDevice = AnsiStringToUnicodeString (
				lpszDevice,
				NULL,
				TRUE
				);
		if (NULL == lpwszDevice)
		{
			return 0;
		}
	}
	if (NULL != lpszOutput)
	{
		lpwszOutput = AnsiStringToUnicodeString (
				lpszOutput,
				NULL,
				TRUE
				);
		if (NULL == lpwszOutput)
		{
			return 0;
		}
	}
	/*
	 * Call the Unicode version
	 * of CreateDC.
	 */
	hDC = CreateDCW (
		lpwszDriver,
		lpwszDevice,
		lpwszOutput,
		lpInitData
		);
	/*
	 * Free Unicode parameters.
	 */
	if (NULL != lpszDriver) LocalFree (lpwszDriver);
	if (NULL != lpszDevice) LocalFree (lpwszDevice);
	if (NULL != lpszOutput) LocalFree (lpwszOutput);
	/*
	 * Return the possible DC handle.
	 */
	return hDC;
}


HFONT
STDCALL
CreateFontIndirectA(
	CONST LOGFONT		*a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HFONT
STDCALL
CreateFontA(
	int	a0,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	DWORD	a5,
	DWORD	a6,
	DWORD	a7,
	DWORD	a8,
	DWORD	a9,
	DWORD	a10,
	DWORD	a11,
	DWORD	a12,
	LPCSTR	a13
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HDC
STDCALL
CreateICA(
	LPCSTR			a0,
	LPCSTR			a1,
	LPCSTR			a2,
	CONST DEVMODE *	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HDC
STDCALL
CreateMetaFileA(
	LPCSTR		a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
CreateScalableFontResourceA(
	DWORD		a0,
	LPCSTR		a1,
	LPCSTR		a2,
	LPCSTR		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
DeviceCapabilitiesExA(
	LPCSTR		a0,
	LPCSTR		a1,
	WORD		a2,
	LPSTR		a3,
	CONST DEVMODE	*a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
EnumFontFamiliesExA(
	HDC		a0,
	LPLOGFONT	a1,
	FONTENUMEXPROC	a2,
	LPARAM		a3,
	DWORD		a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
EnumFontFamiliesA(
	HDC		a0,
	LPCSTR		a1,
	FONTENUMPROC	a2,
	LPARAM		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
EnumFontsA(
	HDC		a0,
	LPCSTR		a1,
	ENUMFONTSPROC	a2,
	LPARAM		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
GetCharWidthA(
	HDC	a0,
	UINT	a1,
	UINT	a2,
	LPINT	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
GetCharWidth32A(
	HDC	a0,
	UINT	a1,
	UINT	a2,
	LPINT	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
APIENTRY
GetCharWidthFloatA(
	HDC	a0,
	UINT	a1,
	UINT	a2,
	PFLOAT	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
APIENTRY
GetCharABCWidthsA(
	HDC	a0,
	UINT	a1,
	UINT	a2,
	LPABC	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
APIENTRY
GetCharABCWidthsFloatA(
	HDC		a0,
	UINT		a1,
	UINT		a2,
	LPABCFLOAT	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
GetGlyphOutlineA(
	HDC		a0,
	UINT		a1,
	UINT		a2,
	LPGLYPHMETRICS	a3,
	DWORD		a4,
	LPVOID		a5,
	CONST MAT2	*a6
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HMETAFILE
STDCALL
GetMetaFileA(
	LPCSTR	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT
APIENTRY
GetOutlineTextMetricsA(
	HDC			a0,
	UINT			a1,
	LPOUTLINETEXTMETRIC	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
APIENTRY
GetTextExtentPointA(
	HDC		hDc,
	LPCSTR		a1,
	int		a2,
	LPSIZE		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
APIENTRY
GetTextExtentPoint32A(
	HDC		hDc,
	LPCSTR		a1,
	int		a2,
	LPSIZE		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
APIENTRY
GetTextExtentExPointA(
	HDC		hDc,
	LPCSTR		a1,
	int		a2,
	int		a3,
	LPINT		a4,
	LPINT		a5,
	LPSIZE		a6
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
GetCharacterPlacementA(
	HDC		hDc,
	LPCSTR		a1,
	int		a2,
	int		a3,
	LPGCP_RESULTS	a4,
	DWORD		a5
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HDC
STDCALL
ResetDCA(
	HDC		a0,
	CONST DEVMODE	*a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
RemoveFontResourceA(
	LPCSTR	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


HENHMETAFILE 
STDCALL 
CopyEnhMetaFileA(
	HENHMETAFILE	a0,
	LPCSTR		a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HDC   
STDCALL 
CreateEnhMetaFileA(
	HDC		a0,
	LPCSTR		a1,
	CONST RECT	*a2,
	LPCSTR		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HENHMETAFILE  
STDCALL 
GetEnhMetaFileA(
	LPCSTR	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT  
STDCALL 
GetEnhMetaFileDescriptionA(
	HENHMETAFILE	a0,
	UINT		a1,
	LPSTR		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL 
STDCALL 
GetTextMetricsA(
	HDC		hdc, 
	LPTEXTMETRIC	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
StartDocA(
	HDC		hdc,
	CONST DOCINFO	*a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int   
STDCALL 
GetObjectA(
	HGDIOBJ		a0, 
	int		a1, 
	LPVOID		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL  
STDCALL 
TextOutA(
	HDC		hdc, 
	int		a1, 
	int		a2, 
	LPCSTR		a3, 
	int		a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL  
STDCALL 
ExtTextOutA(
	HDC		hdc, 
	int		a1, 
	int		a2, 
	UINT		a3, 
	CONST RECT	*a4,
	LPCSTR		a5, 
	UINT		a6, 
	CONST INT	*a7
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

 
BOOL  
STDCALL 
PolyTextOutA(
	HDC			hdc, 
	CONST POLYTEXT		*a1, 
	int			a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
GetTextFaceA(
	HDC	a0,
	int	a1,
	LPSTR	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
GetKerningPairsA(
	HDC		a0,
	DWORD		a1,
	LPKERNINGPAIR	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
GetLogColorSpaceA(
	HCOLORSPACE		a0,
	LPLOGCOLORSPACE	a1,
	DWORD			a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


HCOLORSPACE
STDCALL
CreateColorSpaceA(
	LPLOGCOLORSPACE	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
GetICMProfileA(
	HDC		a0,
	DWORD		a1,	/* MS says LPDWORD! */
	LPSTR		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
SetICMProfileA(
	HDC	a0,
	LPSTR	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
EnumICMProfilesA(
	HDC		a0,
	ICMENUMPROC	a1,
	LPARAM		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
wglUseFontBitmapsA(
	HDC		a0,
	DWORD		a1,
	DWORD		a2,
	DWORD		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
wglUseFontOutlinesA(
	HDC			a0,
	DWORD			a1,
	DWORD			a2,
	DWORD			a3,
	FLOAT			a4,
	FLOAT			a5,
	int			a6,
	LPGLYPHMETRICSFLOAT	a7
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
UpdateICMRegKeyA(
	DWORD	a0,
	DWORD	a1,
	LPSTR	a2,
	UINT	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/* EOF */
