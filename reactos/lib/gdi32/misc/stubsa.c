/* $Id$
 *
 * reactos/lib/gdi32/misc/stubs.c
 *
 * GDI32.DLL Stubs for ANSI functions
 *
 * When you implement one of these functions,
 * remove its stub from this file.
 *
 */

#include "precomp.h"

#define UNIMPLEMENTED DbgPrint("GDI32: %s is unimplemented, please try again later.\n", __FUNCTION__);


/*
 * @unimplemented
 */
int
STDCALL
DeviceCapabilitiesExA(
	LPCSTR		pDevice,
	LPCSTR		pPort,
	WORD		fwCapability,
	LPSTR		pOutput,
	CONST DEVMODEA	*pDevMode
	)
{
  UNIMPLEMENTED;
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/*
 * @unimplemented
 */
UINT
APIENTRY
GetOutlineTextMetricsA(
	HDC			hdc,
	UINT			cbData,
	LPOUTLINETEXTMETRICA	lpOTM
	)
{
  UNIMPLEMENTED;
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetCharacterPlacementA(
	HDC		hDc,
	LPCSTR		a1,
	int		a2,
	int		a3,
	LPGCP_RESULTSA	a4,
	DWORD		a5
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
StartDocA(
	HDC		hdc,
	CONST DOCINFOA	*a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL  
STDCALL 
PolyTextOutA(
	HDC			hdc, 
	CONST POLYTEXTA		*a1, 
	int			a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetKerningPairsA(
	HDC		a0,
	DWORD		a1,
	LPKERNINGPAIR	a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetLogColorSpaceA(
	HCOLORSPACE		a0,
	LPLOGCOLORSPACEA	a1,
	DWORD			a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
HCOLORSPACE
STDCALL
CreateColorSpaceA(
	LPLOGCOLORSPACEA	a0
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetICMProfileA(
	HDC		hdc,
	LPDWORD pBufSize,
	LPSTR		pszFilename
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetICMProfileA(
	HDC	a0,
	LPSTR	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
int
STDCALL
EnumICMProfilesA(
	HDC		a0,
	ICMENUMPROCA	a1,
	LPARAM		a2
	)
{
  /*
   * FIXME - call NtGdiEnumICMProfiles with NULL for lpstrBuffer
   * to find out how big a buffer we need. Then allocate that buffer
   * and call NtGdiEnumICMProfiles again to have the buffer filled.
   *
   * Finally, step through the buffer ( MULTI-SZ recommended for format ),
   * and convert each string to ANSI, calling the user's callback function
   * until we run out of strings or the user returns FALSE
   */

  UNIMPLEMENTED;
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
wglUseFontBitmapsA(
	HDC		a0,
	DWORD		a1,
	DWORD		a2,
	DWORD		a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
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
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
UpdateICMRegKeyA(
	DWORD	a0,
	LPSTR	a1,
	LPSTR	a2,
	UINT	a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
RemoveFontResourceExA(
	LPCSTR lpFileName,
	DWORD fl,
	PVOID pdv
)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
HFONT
STDCALL
CreateFontIndirectExA(const ENUMLOGFONTEXDVA *elfexd)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetGlyphIndicesA(
	HDC hdc,
	LPCSTR lpstr,
	int c,
	LPWORD pgi,
	DWORD fl
)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetStringBitmapA(HDC hdc,LPSTR psz,BOOL unknown,UINT cj,BYTE *lpSB)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/* EOF */
