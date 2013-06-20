/*
 * dll/win32/gdi32/misc/stubsw.c
 *
 * GDI32.DLL Stubs for Unicode functions
 *
 * When you implement one of these functions,
 * remove its stub from this file.
 *
 */

#include <precomp.h>
#include <debug.h>


/*
 * @unimplemented
 */
BOOL
WINAPI
PolyTextOutW( HDC hdc, const POLYTEXTW *pptxt, INT cStrings )
{
    for (; cStrings>0; cStrings--, pptxt++)
        if (!ExtTextOutW( hdc, pptxt->x, pptxt->y, pptxt->uiFlags, &pptxt->rcl, pptxt->lpstr, pptxt->n, pptxt->pdx ))
            return FALSE;
    return TRUE;
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
    HDC		hdc,
    LPDWORD		size,
    LPWSTR		filename
)
{
    if (!hdc || !size || !filename) return FALSE;

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
    HANDLE f;

    UNIMPLEMENTED;

    /* fHidden=1 - only visible for the calling app, read-only, not
     * enumerated with EnumFonts/EnumFontFamilies
     * lpszCurrentPath can be NULL
     */

    /* If the output file already exists, return the ERROR_FILE_EXISTS error as specified in MSDN */
    if ((f = CreateFileW(lpszFontRes, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)) != INVALID_HANDLE_VALUE)
    {
        CloseHandle(f);
        SetLastError(ERROR_FILE_EXISTS);
        return FALSE;
    }
    return FALSE; /* create failed */
}


/* EOF */
