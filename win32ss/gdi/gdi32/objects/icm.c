#include <precomp.h>

#define NDEBUG
#include <debug.h>


HCOLORSPACE
FASTCALL
IntCreateColorSpaceW(
    LPLOGCOLORSPACEW lplcpw,
    BOOL Ascii
)
{
    LOGCOLORSPACEEXW lcpeexw;

    if ((lplcpw->lcsSignature != LCS_SIGNATURE) ||
            (lplcpw->lcsVersion != 0x400) ||
            (lplcpw->lcsSize != sizeof(LOGCOLORSPACEW)))
    {
        SetLastError(ERROR_INVALID_COLORSPACE);
        return NULL;
    }
    RtlCopyMemory(&lcpeexw.lcsColorSpace, lplcpw, sizeof(LOGCOLORSPACEW));

    return NtGdiCreateColorSpace(&lcpeexw);
}

/*
 * @implemented
 */
HCOLORSPACE
WINAPI
CreateColorSpaceW(
    LPLOGCOLORSPACEW lplcpw
)
{
    return IntCreateColorSpaceW(lplcpw, FALSE);
}


/*
 * @implemented
 */
HCOLORSPACE
WINAPI
CreateColorSpaceA(
    LPLOGCOLORSPACEA lplcpa
)
{
    LOGCOLORSPACEW lcpw;

    if ((lplcpa->lcsSignature != LCS_SIGNATURE) ||
            (lplcpa->lcsVersion != 0x400) ||
            (lplcpa->lcsSize != sizeof(LOGCOLORSPACEA)))
    {
        SetLastError(ERROR_INVALID_COLORSPACE);
        return NULL;
    }

    lcpw.lcsSignature  = lplcpa->lcsSignature;
    lcpw.lcsVersion    = lplcpa->lcsVersion;
    lcpw.lcsSize       = sizeof(LOGCOLORSPACEW);
    lcpw.lcsCSType     = lplcpa->lcsCSType;
    lcpw.lcsIntent     = lplcpa->lcsIntent;
    lcpw.lcsEndpoints  = lplcpa->lcsEndpoints;
    lcpw.lcsGammaRed   = lplcpa->lcsGammaRed;
    lcpw.lcsGammaGreen = lplcpa->lcsGammaGreen;
    lcpw.lcsGammaBlue  = lplcpa->lcsGammaBlue;

    RtlMultiByteToUnicodeN( lcpw.lcsFilename,
                            MAX_PATH,
                            NULL,
                            lplcpa->lcsFilename,
                            strlen(lplcpa->lcsFilename) + 1);

    return IntCreateColorSpaceW(&lcpw, FALSE);
}

/*
 * @implemented
 */
HCOLORSPACE
WINAPI
GetColorSpace(HDC hDC)
{
    PDC_ATTR pDc_Attr;

    if (!GdiGetHandleUserData(hDC, GDI_OBJECT_TYPE_DC, (PVOID)&pDc_Attr))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
    return pDc_Attr->hColorSpace;
}


/*
 * @implemented
 */
HCOLORSPACE
WINAPI
SetColorSpace(
    HDC hDC,
    HCOLORSPACE hCS
)
{
    HCOLORSPACE rhCS = GetColorSpace(hDC);

    if (GDI_HANDLE_GET_TYPE(hDC) == GDILoObjType_LO_DC_TYPE)
    {
        if (NtGdiSetColorSpace(hDC, hCS)) return rhCS;
    }
#if 0
    if (GDI_HANDLE_GET_TYPE(hDC) != GDILoObjType_LO_METADC16_TYPE)
    {
        PLDC pLDC = GdiGetLDC(hDC);
        if ( !pLDC )
        {
            SetLastError(ERROR_INVALID_HANDLE);
            return NULL;
        }
        if (pLDC->iType == LDC_EMFLDC && !EMFDC_SetColorSpace( pLDC, hCS ))
        {
            return NULL;
        }
    }
#endif
    return NULL;
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

    if (!hdc || !pBufSize) return FALSE;

    if (GetICMProfileW(hdc, &buflen, filenameW))
    {
        ULONG len = WideCharToMultiByte(CP_ACP, 0, filenameW, -1, NULL, 0, NULL, NULL);

        if (!pszFilename)
        {
            *pBufSize = len;
            return FALSE;
        }

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

/*
 * @unimplemented
 */
int
WINAPI
SetICMMode(
    HDC	hdc,
    int	iEnableICM
)
{
    /*FIXME:  Assume that ICM is always off, and cannot be turned on */
    if (iEnableICM == ICM_OFF) return ICM_OFF;
    if (iEnableICM == ICM_ON) return 0;
    if (iEnableICM == ICM_QUERY) return ICM_OFF;

    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 *
 */
HBITMAP
WINAPI
GdiConvertBitmapV5(
    HBITMAP in_format_BitMap,
    HBITMAP src_BitMap,
    INT bpp,
    INT unuse)
{
    /* FIXME guessing the prototypes */

    /*
     * it have create a new bitmap with desired in format,
     * then convert it src_bitmap to new format
     * and return it as HBITMAP
     */

    return FALSE;
}
