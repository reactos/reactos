/* $Id: stubsw.c,v 1.28 2004/04/09 20:03:13 navaraf Exp $
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
  UNICODE_STRING Filename;

  /* FIXME handle fl parameter */
  RtlInitUnicodeString(&Filename, lpszFilename);
  return NtGdiAddFontResource ( &Filename, fl );
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
HDC
STDCALL
CreateICW(
	LPCWSTR			lpszDriver,
	LPCWSTR			lpszDevice,
	LPCWSTR			lpszOutput,
	CONST DEVMODEW *	lpdvmInit
	)
{
  UNICODE_STRING Driver, Device, Output;
  
  if(lpszDriver)
    RtlInitUnicodeString(&Driver, lpszDriver);
  if(lpszDevice)
    RtlInitUnicodeString(&Device, lpszDevice);
  if(lpszOutput)
    RtlInitUnicodeString(&Output, lpszOutput);
  return NtGdiCreateIC ((lpszDriver ? &Driver : NULL),
		      (lpszDevice ? &Device : NULL),
		      (lpszOutput ? &Output : NULL),
		      (CONST PDEVMODEW)lpdvmInit );
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
  return NtGdiCreateScalableFontResource ( fdwHidden,
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
  /* FIXME no NtGdiDeviceCapabilities???? */
  return NtGdiDeviceCapabilities ( pDevice,
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
 * @unimplemented
 */
int
STDCALL
EnumFontsW(
	HDC  hDC,
	LPCWSTR lpFaceName,
	FONTENUMPROCW  FontFunc,
	LPARAM  lParam
	)
{
#if 0
  return NtGdiEnumFonts ( hDC, lpFaceName, FontFunc, lParam );
#else
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
#endif
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
  return NtGdiGetCharWidthFloat ( hdc, iFirstChar, iLastChar, pxBuffer );
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
  return NtGdiGetCharABCWidths ( hdc, uFirstChar, uLastChar, lpabc );
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
  return NtGdiGetCharABCWidthsFloat ( hdc, iFirstChar, iLastChar, lpABCF );
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
  return NtGdiGetGlyphOutline ( hdc, uChar, uFormat, lpgm, cbBuffer, lpvBuffer, (CONST LPMAT2)lpmat2 );
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
  return NtGdiGetTextExtentExPoint (
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
	LPGCP_RESULTSW	a4,
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
  return NtGdiResetDC ( hdc, lpInitData );
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
  return NtGdiRemoveFontResource ( lpFileName );
}


/*
 * @implemented
 */
int 
STDCALL 
StartDocW(
	HDC		hdc,
	CONST DOCINFOW	*a1
	)
{
	return NtGdiStartDoc ( hdc, (DOCINFOW *)a1 );
}


/*
 * @unimplemented
 */
BOOL
STDCALL
PolyTextOutW(
	HDC			hdc,
	CONST POLYTEXTW		*a1,
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
	LPLOGCOLORSPACEW	a1,
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
	LPLOGCOLORSPACEW	a0
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
	HDC		hDC,
	ICMENUMPROCW	lpEnumICMProfilesFunc,
	LPARAM		lParam
	)
{
  /*
   * FIXME - call NtGdiEnumICMProfiles with NULL for lpstrBuffer
   * to find out how big a buffer we need. Then allocate that buffer
   * and call NtGdiEnumICMProfiles again to have the buffer filled.
   *
   * Finally, step through the buffer ( MULTI-SZ recommended for format ),
   * and call the user's callback function until we run out of strings or
   * the user returns FALSE
   */
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
BOOL
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
