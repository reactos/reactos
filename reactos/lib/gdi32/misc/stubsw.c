/* $Id: stubsw.c,v 1.17 2003/07/24 22:17:15 royce Exp $
 *
 * reactos/lib/gdi32/misc/stubs.c
 *
 * GDI32.DLL Stubs for Unicode functions
 *
 * When you implement one of these functions,
 * remove its stub from this file.
 *
 */
#ifndef UNICODE
#define UNICODE
#endif//UNICODE
#include <windows.h>
#include <win32k/kapi.h>

/*
 * @implemented
 */
int
STDCALL
AddFontResourceExW ( LPCWSTR lpszFilename, DWORD fl, PVOID pvReserved )
{
  /* FIXME handle fl parameter */
  return W32kAddFontResource ( lpszFilename );
}

/*
 * @implemented
 */
int
STDCALL
AddFontResourceW ( LPCWSTR lpszFilename )
{
	return AddFontResourceExW ( lpszFilename, 0, 0 );
}


/*
 * @implemented
 */
HMETAFILE
STDCALL
CopyMetaFileW(
	HMETAFILE	Src,
	LPCWSTR		File
	)
{
	return W32kCopyMetaFile ( Src, File );
}


/*
 * @implemented
 */
HDC
STDCALL
CreateICW(
	LPCWSTR			lpszDriver,
	LPCWSTR			lpszDevice,
	LPCWSTR			lpszOutput,
	CONST DEVMODEW *	lpdvmInit
	)
{
  return W32kCreateIC ( lpszDriver,
		      lpszDevice,
		      lpszOutput,
		      (CONST PDEVMODEW)lpdvmInit );
}


/*
 * @implemented
 */
HDC
STDCALL
CreateMetaFileW(
	LPCWSTR		lpszFile
	)
{
	return W32kCreateMetaFile ( lpszFile );
}


/*
 * @implemented
 */
BOOL
STDCALL
CreateScalableFontResourceW(
	DWORD		fdwHidden,
	LPCWSTR		lpszFontRes,
	LPCWSTR		lpszFontFile,
	LPCWSTR		lpszCurrentPath
	)
{
  return W32kCreateScalableFontResource ( fdwHidden,
					  lpszFontRes,
					  lpszFontFile,
					  lpszCurrentPath );
}


/*
 * @unimplemented
 */
int
STDCALL
DeviceCapabilitiesExW(
	LPCWSTR		pDevice,
	LPCWSTR		pPort,
	WORD		fwCapability,
	LPWSTR		pOutput,
	CONST DEVMODEW	*pDevMode
	)
{
#if 0
  /* FIXME no W32kDeviceCapabilities???? */
  return W32kDeviceCapabilities ( pDevice,
				  pPort,
				  fwCapability,
				  pOutput,
				  pDevMode );
#else
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
#endif
}


/*
 * @implemented
 */
int
STDCALL
EnumFontFamiliesExW(
	HDC		hdc,
	LPLOGFONTW	lpLogFont,
	FONTENUMEXPROC	lpEnumFontFamProc,
	LPARAM		lParam,
	DWORD		dwFlags
	)
{
  return W32kEnumFontFamiliesEx ( hdc, lpLogFont, lpEnumFontFamProc, lParam, dwFlags );
}


/*
 * @implemented
 */
int
STDCALL
EnumFontFamiliesW(
	HDC		hdc,
	LPCWSTR		lpszFamily,
	FONTENUMPROC	lpEnumFontFamProc,
	LPARAM		lParam
	)
{
  return W32kEnumFontFamilies ( hdc, lpszFamily, lpEnumFontFamProc, lParam );
}


/*
 * @implemented
 */
int
STDCALL
EnumFontsW(
	HDC  hDC,
	LPCWSTR lpFaceName,
	FONTENUMPROC  FontFunc,
	LPARAM  lParam
	)
{
  return W32kEnumFonts ( hDC, lpFaceName, FontFunc, lParam );
}


/*
 * @implemented
 */
BOOL
STDCALL
GetCharWidthW (
	HDC	hdc,
	UINT	iFirstChar,
	UINT	iLastChar,
	LPINT	lpBuffer
	)
{
  return W32kGetCharWidth ( hdc, iFirstChar, iLastChar, lpBuffer );
}


/*
 * @implemented
 */
BOOL
STDCALL
GetCharWidth32W(
	HDC	hdc,
	UINT	iFirstChar,
	UINT	iLastChar,
	LPINT	lpBuffer
	)
{
  return W32kGetCharWidth32 ( hdc, iFirstChar, iLastChar, lpBuffer );
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetCharWidthFloatW(
	HDC	hdc,
	UINT	iFirstChar,
	UINT	iLastChar,
	PFLOAT	pxBuffer
	)
{
  return W32kGetCharWidthFloat ( hdc, iFirstChar, iLastChar, pxBuffer );
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetCharABCWidthsW(
	HDC	hdc,
	UINT	uFirstChar,
	UINT	uLastChar,
	LPABC	lpabc
	)
{
  return W32kGetCharABCWidths ( hdc, uFirstChar, uLastChar, lpabc );
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetCharABCWidthsFloatW(
	HDC		hdc,
	UINT		iFirstChar,
	UINT		iLastChar,
	LPABCFLOAT	lpABCF
	)
{
  return W32kGetCharABCWidthsFloat ( hdc, iFirstChar, iLastChar, lpABCF );
}


/*
 * @implemented
 */
DWORD
STDCALL
GetGlyphOutlineW(
	HDC		hdc,
	UINT		uChar,
	UINT		uFormat,
	LPGLYPHMETRICS	lpgm,
	DWORD		cbBuffer,
	LPVOID		lpvBuffer,
	CONST MAT2	*lpmat2
	)
{
  return W32kGetGlyphOutline ( hdc, uChar, uFormat, lpgm, cbBuffer, lpvBuffer, (CONST LPMAT2)lpmat2 );
}


/*
 * @implemented
 */
HMETAFILE
STDCALL
GetMetaFileW(
	LPCWSTR	lpszMetaFile
	)
{
  return W32kGetMetaFile ( lpszMetaFile );
}


/*
 * @unimplemented
 */
UINT
APIENTRY
GetOutlineTextMetricsW(
	HDC			hdc,
	UINT			cbData,
	LPOUTLINETEXTMETRICW	lpOTM
	)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentExPointW(
	HDC		hdc,
	LPCWSTR		lpszStr,
	int		cchString,
	int		nMaxExtent,
	LPINT		lpnFit,
	LPINT		alpDx,
	LPSIZE		lpSize
	)
{
  return W32kGetTextExtentExPoint (
    hdc, lpszStr, cchString, nMaxExtent, lpnFit, alpDx, lpSize );
}


/*
 * @unimplemented
 */
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


/*
 * @implemented
 */
HDC
STDCALL
ResetDCW(
	HDC		hdc,
	CONST DEVMODEW	*lpInitData
	)
{
  return W32kResetDC ( hdc, lpInitData );
}


/*
 * @implemented
 */
BOOL
STDCALL
RemoveFontResourceW(
	LPCWSTR	lpFileName
	)
{
  return W32kRemoveFontResource ( lpFileName );
}


/*
 * @implemented
 */
HENHMETAFILE 
STDCALL 
CopyEnhMetaFileW(
	HENHMETAFILE	hemfSrc,
	LPCWSTR		lpszFile
	)
{
  return W32kCopyEnhMetaFile ( hemfSrc, lpszFile );
}


/*
 * @implemented
 */
HDC
STDCALL
CreateEnhMetaFileW(
	HDC		hdc,
	LPCWSTR		lpFileName,
	CONST RECT	*lpRect,
	LPCWSTR		lpDescription
	)
{
  return W32kCreateEnhMetaFile ( hdc, lpFileName, (CONST LPRECT)lpRect, lpDescription );
}


/*
 * @implemented
 */
HENHMETAFILE
STDCALL
GetEnhMetaFileW(
	LPCWSTR	lpszMetaFile
	)
{
  return W32kGetEnhMetaFile ( lpszMetaFile );
}


/*
 * @implemented
 */
UINT
STDCALL
GetEnhMetaFileDescriptionW(
	HENHMETAFILE	hemf,
	UINT		cchBuffer,
	LPWSTR		lpszDescription
	)
{
  return W32kGetEnhMetaFileDescription ( hemf, cchBuffer, lpszDescription );
}


/*
 * @implemented
 */
int 
STDCALL 
StartDocW(
	HDC		hdc, 
	CONST DOCINFO	*a1
	)
{
	return W32kStartDoc ( hdc, (CONST PDOCINFO)a1 );
}


/*
 * @unimplemented
 */
int   
STDCALL 
GetObjectW(
	HGDIOBJ		a0, 
	int		a1, 
	LPVOID		a2
	)
{
	return W32kGetObject ( a0, a1, a2 );
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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

/*
 * @unimplemented
 */
HCOLORSPACE
STDCALL
CreateColorSpaceW(
	LPLOGCOLORSPACE	a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
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
