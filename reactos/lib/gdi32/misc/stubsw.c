/* $Id: stubsw.c,v 1.1 1999/05/24 20:04:44 ea Exp $
 *
 * reactos/lib/gdi32/misc/stubs.c
 *
 * GDI32.DLL Stubs for Unicode functions
 *
 * When you implement one of these functions,
 * remove its stub from this file.
 *
 */
#define UNICODE
#include <windows.h>


int
STDCALL
AddFontResourceW(
	LPCWSTR		a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HMETAFILE
STDCALL
CopyMetaFileW(
	HMETAFILE	a0,
	LPCWSTR		a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HDC
STDCALL
CreateDCW(
	LPCWSTR		a0,
	LPCWSTR		a1,
	LPCWSTR		a2,
	CONST DEVMODE	*a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HFONT
STDCALL
CreateFontIndirectW(
	CONST LOGFONT		*a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HFONT
STDCALL
CreateFontW(
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
	LPCWSTR	a13
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HDC
STDCALL
CreateICW(
	LPCWSTR			a0,
	LPCWSTR			a1,
	LPCWSTR			a2,
	CONST DEVMODE *	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HDC
STDCALL
CreateMetaFileW(
	LPCWSTR		a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
CreateScalableFontResourceW(
	DWORD		a0,
	LPCWSTR		a1,
	LPCWSTR		a2,
	LPCWSTR		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
DeviceCapabilitiesExW(
	LPCWSTR		a0,
	LPCWSTR		a1,
	WORD		a2,
	LPWSTR		a3,
	CONST DEVMODE	*a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
EnumFontFamiliesExW(
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
EnumFontFamiliesW(
	HDC		a0,
	LPCWSTR		a1,
	FONTENUMPROC	a2,
	LPARAM		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
EnumFontsW(
	HDC		a0,
	LPCWSTR		a1,
	ENUMFONTSPROC	a2,
	LPARAM		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
GetCharWidthW(
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
GetCharWidth32W(
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
GetCharWidthFloatW(
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
GetCharABCWidthsW(
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
GetCharABCWidthsFloatW(
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
GetGlyphOutlineW(
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
GetMetaFileW(
	LPCWSTR	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT
APIENTRY
GetOutlineTextMetricsW(
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
GetTextExtentPointW(
	HDC		hDc,
	LPCWSTR		a1,
	int		a2,
	LPSIZE		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
APIENTRY
GetTextExtentPoint32W(
	HDC		hDc,
	LPCWSTR		a1,
	int		a2,
	LPSIZE		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
APIENTRY
GetTextExtentExPointW(
	HDC		hDc,
	LPCWSTR		a1,
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
GetCharacterPlacementW(
	HDC		hDc,
	LPCWSTR		a1,
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
ResetDCW(
	HDC		a0,
	CONST DEVMODE	*a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
RemoveFontResourceW(
	LPCWSTR	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


HENHMETAFILE 
STDCALL 
CopyEnhMetaFileW(
	HENHMETAFILE	a0, 
	LPCWSTR		a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HDC   
STDCALL 
CreateEnhMetaFileW(
	HDC		a0, 
	LPCWSTR		a1, 
	CONST RECT	*a2, 
	LPCWSTR		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HENHMETAFILE  
STDCALL 
GetEnhMetaFileW(
	LPCWSTR	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT  
STDCALL 
GetEnhMetaFileDescriptionW(
	HENHMETAFILE	a0,
	UINT		a1,
	LPWSTR		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL 
STDCALL 
GetTextMetricsW(
	HDC		hdc, 
	LPTEXTMETRIC	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int 
STDCALL 
StartDocW(
	HDC		hdc, 
	CONST DOCINFO	*a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int   
STDCALL 
GetObjectW(
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
TextOutW(
	HDC		hdc, 
	int		a1, 
	int		a2, 
	LPCWSTR		a3, 
	int		a4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL  
STDCALL 
ExtTextOutW(
	HDC		hdc, 
	int		a1, 
	int		a2, 
	UINT		a3,	 
	CONST RECT	*a4,
	LPCWSTR		a5, 
	UINT		a6, 
	CONST INT	*a7
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
PolyTextOutW(
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
GetTextFaceW(
	HDC	a0,
	int	a1,
	LPWSTR	a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetKerningPairsW(
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
GetLogColorSpaceW(
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
CreateColorSpaceW(
	LPLOGCOLORSPACE	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
GetICMProfileW(
	HDC		a0,
	DWORD		a1,	/* MS says LPDWORD! */
	LPWSTR		a2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
SetICMProfileW(
	HDC	a0,
	LPWSTR	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
EnumICMProfilesW(
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
wglUseFontBitmapsW(
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
wglUseFontOutlinesW(
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
UpdateICMRegKeyW(
	DWORD	a0,
	DWORD	a1,
	LPWSTR	a2,
	UINT	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/* === AFTER THIS POINT I GUESS... ========= 
 * (based on stack size in Norlander's .def)
 * === WHERE ARE THEY DEFINED? =============
 */


DWORD
STDCALL
GetFontResourceInfoW(
	DWORD	a0,
	DWORD	a1,
	DWORD	a2,
	DWORD	a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/* EOF */
