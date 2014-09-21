/*
 * dll/win32/gdi32/misc/stubsa.c
 *
 * GDI32.DLL Stubs for ANSI functions
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
PolyTextOutA( HDC hdc, const POLYTEXTA *pptxt, INT cStrings )
{
    for (; cStrings>0; cStrings--, pptxt++)
        if (!ExtTextOutA( hdc, pptxt->x, pptxt->y, pptxt->uiFlags, &pptxt->rcl, pptxt->lpstr, pptxt->n, pptxt->pdx ))
            return FALSE;
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
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
BOOL
WINAPI
GetICMProfileA(
    HDC		hdc,
    LPDWORD pBufSize,
    LPSTR		pszFilename
)
{
    WCHAR filenameW[MAX_PATH];
    DWORD buflen = MAX_PATH;
    BOOL ret = FALSE;

    if (!hdc || !pBufSize || !pszFilename) return FALSE;

    if (GetICMProfileW(hdc, &buflen, filenameW))
    {
        ULONG len = WideCharToMultiByte(CP_ACP, 0, filenameW, -1, NULL, 0, NULL, NULL);
        if (*pBufSize >= len)
        {
            WideCharToMultiByte(CP_ACP, 0, filenameW, -1, pszFilename, *pBufSize, NULL, NULL);
            ret = TRUE;
        }
        else SetLastError(ERROR_INSUFFICIENT_BUFFER);
        *pBufSize = len;
    }

    return ret;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
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
WINAPI
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
WINAPI
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
 * @implemented
 */
UINT
WINAPI
GetStringBitmapA(HDC hdc,
                 LPSTR psz,
                 BOOL DoCall,
                 UINT cj,
                 BYTE *lpSB)
{

    NTSTATUS Status;
    PWSTR pwsz;
    UINT retValue = 0;

    if (DoCall)
    {
        Status = HEAP_strdupA2W ( &pwsz, psz );
        if ( !NT_SUCCESS (Status) )
        {
            SetLastError (RtlNtStatusToDosError(Status));
        }
        else
        {
            retValue = NtGdiGetStringBitmapW(hdc, pwsz, 1, lpSB, cj);
            HEAP_free ( pwsz );
        }
    }

    return retValue;

}


/* EOF */
