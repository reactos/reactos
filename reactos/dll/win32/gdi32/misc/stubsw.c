/* $Id$
 *
 * reactos/lib/gdi32/misc/stubs.c
 *
 * GDI32.DLL Stubs for Unicode functions
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
BOOL
STDCALL
PolyTextOutW(
	HDC			hdc,
	CONST POLYTEXTW		*a1,
	int			a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @implemented
 */
DWORD
STDCALL
GetKerningPairsW(HDC hdc,
                 ULONG cPairs,
                 LPKERNINGPAIR pkpDst)
{
    if ((cPairs != 0) || (pkpDst == 0))
    {
        return NtGdiGetKerningPairs(hdc,cPairs,pkpDst);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	LPDWORD		a1,
	LPWSTR		a2
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
SetICMProfileW(
	HDC	a0,
	LPWSTR	a1
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
  UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
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
	LPWSTR	a1,
	LPWSTR	a2,
	UINT	a3
	)
{
	UNIMPLEMENTED;
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
BOOL
STDCALL
EudcLoadLinkW(LPCWSTR pBaseFaceName,LPCWSTR pEudcFontPath,INT iPriority,INT iFontLinkType)
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
EudcUnloadLinkW(LPCWSTR pBaseFaceName,LPCWSTR pEudcFontPath)
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
GdiAddFontResourceW(LPCWSTR lpszFilename,FLONG fl,DESIGNVECTOR *pdv)
{
	UNICODE_STRING Filename;
	//UNIMPLEMENTED;
	//SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	//return 0;

	/* FIXME handle fl parameter */
	RtlInitUnicodeString(&Filename, lpszFilename);
	return NtGdiAddFontResource ( &Filename, fl );
}


/*
 * @implemented
 */
DWORD
STDCALL
GetEUDCTimeStampExW(LPWSTR lpBaseFaceName)
{
    DWORD retValue = 0;

    if (!lpBaseFaceName)
    {
        retValue = NtGdiGetEudcTimeStampEx(NULL,0,FALSE);
    }
    else
    {
        retValue = NtGdiGetEudcTimeStampEx(lpBaseFaceName, wcslen(lpBaseFaceName), FALSE);
    }

    return retValue;
}



/*
 * @unimplemented
 */
BOOL
STDCALL
bInitSystemAndFontsDirectoriesW(LPWSTR *SystemDir,LPWSTR *FontsDir)
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
bMakePathNameW(LPWSTR lpBuffer,LPCWSTR lpFileName,LPWSTR *lpFilePart,DWORD unknown)
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
GetGlyphIndicesW(
	HDC hdc,
	LPCWSTR lpstr,
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
 * @implemented
 */
UINT
STDCALL
GetStringBitmapW(HDC hdc,
                 LPWSTR pwsz,
                 BOOL doCall,
                 UINT cj,
                 BYTE *lpSB)
{
    UINT retValue = 0;

    if (doCall)
    {
        retValue = NtGdiGetStringBitmapW(hdc, pwsz, 1, lpSB, cj);
    }

    return retValue;

}


/* EOF */
