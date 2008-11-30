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
WINAPI
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
 * @unimplemented
 */
BOOL
WINAPI
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
BOOL
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
GdiAddFontResourceW(
    LPCWSTR lpszFilename,
    FLONG fl,
    DESIGNVECTOR *pdv)
{
    return NtGdiAddFontResourceW((PWSTR)lpszFilename, 0, 0, fl, 0, pdv);
}

/*
 * @implemented
 */
DWORD
WINAPI
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
WINAPI
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
WINAPI
bMakePathNameW(LPWSTR lpBuffer,LPCWSTR lpFileName,LPWSTR *lpFilePart,DWORD unknown)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @implemented
 */
UINT
WINAPI
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


BOOL
WINAPI
CreateScalableFontResourceW(
	DWORD fdwHidden,
	LPCWSTR lpszFontRes,
	LPCWSTR lpszFontFile,
	LPCWSTR lpszCurrentPath
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}
  

/* EOF */
