#include <precomp.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
BOOL
WINAPI
TextOutA(
    HDC  hdc,
    int  nXStart,
    int  nYStart,
    LPCSTR  lpString,
    int  cchString)
{
    ANSI_STRING StringA;
    UNICODE_STRING StringU;
    BOOL ret;

    if (NULL != lpString)
    {
        RtlInitAnsiString(&StringA, (LPSTR)lpString);
        RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);
    }
    else
        StringU.Buffer = NULL;

    ret = TextOutW(hdc, nXStart, nYStart, StringU.Buffer, cchString);
    RtlFreeUnicodeString(&StringU);
    return ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
TextOutW(
    HDC  hdc,
    int  nXStart,
    int  nYStart,
    LPCWSTR  lpString,
    int  cchString)
{
    return NtGdiExtTextOutW(hdc, nXStart, nYStart, 0, NULL, (LPWSTR)lpString, cchString, NULL, 0);
}


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
PolyTextOutW( HDC hdc, const POLYTEXTW *pptxt, INT cStrings )
{
    for (; cStrings>0; cStrings--, pptxt++)
        if (!ExtTextOutW( hdc, pptxt->x, pptxt->y, pptxt->uiFlags, &pptxt->rcl, pptxt->lpstr, pptxt->n, pptxt->pdx ))
            return FALSE;
    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
GdiGetCodePage(HDC hdc)
{
    PDC_ATTR Dc_Attr;
    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return 0;
    if (Dc_Attr->ulDirty_ & DIRTY_CHARSET) return LOWORD(NtGdiGetCharSet(hdc));
    return LOWORD(Dc_Attr->iCS_CP);
}


/*
 * @unimplemented
 */
int
WINAPI
GetTextCharacterExtra(
    HDC	hDc
)
{
    PDC_ATTR Dc_Attr;

    if (!GdiGetHandleUserData((HGDIOBJ) hDc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return 0;
    return Dc_Attr->lTextExtra;
// return GetDCDWord( hDc, GdiGetTextCharExtra, 0);
}


/*
 * @implemented
 */
int
WINAPI
GetTextCharset(HDC hdc)
{
    /* MSDN docs say this is equivalent */
    return NtGdiGetTextCharsetInfo(hdc,NULL,0);
}



/*
 * @implemented
 */
BOOL
WINAPI
GetTextMetricsA(
    HDC		hdc,
    LPTEXTMETRICA	lptm
)
{
    TMW_INTERNAL tmwi;

    if (! NtGdiGetTextMetricsW(hdc, &tmwi, sizeof(TMW_INTERNAL)))
    {
        return FALSE;
    }

    FONT_TextMetricWToA(&tmwi.TextMetric, lptm);
    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetTextMetricsW(
    HDC		hdc,
    LPTEXTMETRICW	lptm
)
{
    TMW_INTERNAL tmwi;

    if (! NtGdiGetTextMetricsW(hdc, &tmwi, sizeof(TMW_INTERNAL)))
    {
        return FALSE;
    }

    *lptm = tmwi.TextMetric;
    return TRUE;
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentPointA(
    HDC		hdc,
    LPCSTR		lpString,
    int		cchString,
    LPSIZE		lpSize
)
{
    ANSI_STRING StringA;
    UNICODE_STRING StringU;
    BOOL ret;

    RtlInitAnsiString(&StringA, (LPSTR)lpString);
    RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

    ret = GetTextExtentPointW(hdc, StringU.Buffer, cchString, lpSize);

    RtlFreeUnicodeString(&StringU);

    return ret;
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentPointW(
    HDC		hdc,
    LPCWSTR		lpString,
    int		cchString,
    LPSIZE		lpSize
)
{
    return NtGdiGetTextExtent(hdc, (LPWSTR)lpString, cchString, lpSize, 0);
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentExPointW(
    HDC     hdc,
    LPCWSTR lpszStr,
    int     cchString,
    int     nMaxExtent,
    LPINT   lpnFit,
    LPINT   alpDx,
    LPSIZE  lpSize
)
{

    /* Windows doesn't check nMaxExtent validity in unicode version */
    if(nMaxExtent < -1)
        DPRINT("nMaxExtent is invalid: %d\n", nMaxExtent);

    return NtGdiGetTextExtentExW (
               hdc, (LPWSTR)lpszStr, cchString, nMaxExtent, (PULONG)lpnFit, (PULONG)alpDx, lpSize, 0 );
}


/*
 * @implemented
 */
BOOL
WINAPI
GetTextExtentExPointWPri(HDC hdc,
                         LPWSTR lpwsz,
                         ULONG cwc,
                         ULONG dxMax,
                         ULONG *pcCh,
                         PULONG pdxOut,
                         LPSIZE psize)
{
    return NtGdiGetTextExtentExW(hdc,lpwsz,cwc,dxMax,pcCh,pdxOut,psize,0);
}

/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentExPointA(
    HDC		hdc,
    LPCSTR		lpszStr,
    int		cchString,
    int		nMaxExtent,
    LPINT		lpnFit,
    LPINT		alpDx,
    LPSIZE		lpSize
)
{
    NTSTATUS Status;
    LPWSTR lpszStrW;
    BOOL rc = 0;

    if(nMaxExtent < -1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Status = HEAP_strdupA2W ( &lpszStrW, lpszStr );
    if (!NT_SUCCESS (Status))
        SetLastError (RtlNtStatusToDosError(Status));
    else
    {
        rc = NtGdiGetTextExtentExW (
                 hdc, lpszStrW, cchString, nMaxExtent, (PULONG)lpnFit, (PULONG)alpDx, lpSize, 0 );

        HEAP_free ( lpszStrW );
    }

    return rc;
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentPoint32A(
    HDC		hdc,
    LPCSTR		lpString,
    int		cchString,
    LPSIZE		lpSize
)
{
    ANSI_STRING StringA;
    UNICODE_STRING StringU;
    BOOL ret;

    StringA.Buffer = (LPSTR)lpString;
    StringA.Length = cchString;
    RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

    ret = GetTextExtentPoint32W(hdc, StringU.Buffer, cchString, lpSize);

    RtlFreeUnicodeString(&StringU);

    return ret;
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentPoint32W(
    HDC		hdc,
    LPCWSTR		lpString,
    int		cchString,
    LPSIZE		lpSize
)
{
    return NtGdiGetTextExtent(hdc, (LPWSTR)lpString, cchString, lpSize, 0);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetTextExtentExPointI(HDC hdc,
                      LPWORD pgiIn,
                      int cgi,
                      int nMaxExtent,
                      LPINT lpnFit,
                      LPINT alpDx,
                      LPSIZE lpSize)
{
    return NtGdiGetTextExtentExW(hdc,pgiIn,cgi,nMaxExtent,(ULONG *)lpnFit, (PULONG) alpDx,lpSize,GTEF_INDICES);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetTextExtentPointI(HDC hdc,
                    LPWORD pgiIn,
                    int cgi,
                    LPSIZE lpSize)
{
    return NtGdiGetTextExtent(hdc,pgiIn,cgi,lpSize,GTEF_INDICES);
}

/*
 * @implemented
 */
BOOL
WINAPI
ExtTextOutA(
    HDC		hdc,
    int		X,
    int		Y,
    UINT		fuOptions,
    CONST RECT	*lprc,
    LPCSTR		lpString,
    UINT		cchString,
    CONST INT	*lpDx
)
{
    ANSI_STRING StringA;
    UNICODE_STRING StringU;
    BOOL ret;

    RtlInitAnsiString(&StringA, (LPSTR)lpString);
    RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

    ret = ExtTextOutW(hdc, X, Y, fuOptions, lprc, StringU.Buffer, cchString, lpDx);

    RtlFreeUnicodeString(&StringU);

    return ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
ExtTextOutW(
    HDC		hdc,
    int		X,
    int		Y,
    UINT		fuOptions,
    CONST RECT	*lprc,
    LPCWSTR		lpString,
    UINT		cchString,
    CONST INT	*lpDx
)
{
    return NtGdiExtTextOutW(hdc, X, Y, fuOptions, (LPRECT)lprc, (LPWSTR)lpString, cchString, (LPINT)lpDx, 0);
}


/*
 * @implemented
 */
INT
WINAPI
GetTextFaceW(HDC hDC,
             INT nCount,
             PWSTR pFaceName)
{
    /* Validate parameters */
    if (pFaceName && nCount <= 0)
    {
        /* Set last error and return failure */
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Forward to kernel */
    return NtGdiGetTextFaceW(hDC, nCount, pFaceName, FALSE);
}


/*
 * @implemented
 */
int
WINAPI
GetTextFaceA( HDC hdc, INT count, LPSTR name )
{
    INT res;
    LPWSTR nameW;

    /* Validate parameters */
    if (name && count <= 0)
    {
        /* Set last error and return failure */
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    res = GetTextFaceW(hdc, 0, NULL);
    nameW = HeapAlloc( GetProcessHeap(), 0, res * 2 );
    if (nameW == NULL)
    {
        return 0;
    }
    GetTextFaceW( hdc, res, nameW );

    if (name)
    {
        if (count && !WideCharToMultiByte( CP_ACP, 0, nameW, -1, name, count, NULL, NULL))
            name[count-1] = 0;
        res = strlen(name);
    }
    else
        res = WideCharToMultiByte( CP_ACP, 0, nameW, -1, NULL, 0, NULL, NULL);
    HeapFree( GetProcessHeap(), 0, nameW );
    return res;
}


/*
 * @implemented
 */
INT
WINAPI
GetTextFaceAliasW(HDC hdc,
                  int cChar,
                  LPWSTR pszOut)
{
    if ( pszOut && !cChar )
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    return NtGdiGetTextFaceW(hdc,cChar,pszOut,TRUE);
}


BOOL
WINAPI
GetFontResourceInfoW(
    LPCWSTR lpFileName,
    DWORD *pdwBufSize,
    void* lpBuffer,
    DWORD dwType
)
{
    BOOL bRet;
    UNICODE_STRING NtFileName;

    if (!lpFileName || !pdwBufSize || !lpBuffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!RtlDosPathNameToNtPathName_U(lpFileName,
                                      &NtFileName,
                                      NULL,
                                      NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    bRet = NtGdiGetFontResourceInfoInternalW(
               NtFileName.Buffer,
               (NtFileName.Length / sizeof(WCHAR)) + 1,
               1,
               *pdwBufSize,
               pdwBufSize,
               lpBuffer,
               dwType);

    RtlFreeHeap(RtlGetProcessHeap(), 0, NtFileName.Buffer);

    if (!bRet)
    {
        return FALSE;
    }

    return TRUE;
}


/*
 * @unimplemented
 */
int
WINAPI
SetTextCharacterExtra(
    HDC	hDC,
    int	CharExtra
)
{
    INT cExtra = 0x80000000;
    PDC_ATTR Dc_Attr;

    if (CharExtra == cExtra)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return cExtra;
    }
#if 0
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
    {
        return MFDRV_SetTextCharacterExtra( hDC, CharExtra ); // Wine port.
    }
#endif
    if (!GdiGetHandleUserData((HGDIOBJ) hDC, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return cExtra;

    if (NtCurrentTeb()->GdiTebBatch.HDC == hDC)
    {
        if (Dc_Attr->ulDirty_ & DC_FONTTEXT_DIRTY)
        {
            NtGdiFlush(); // Sync up Dc_Attr from Kernel space.
            Dc_Attr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
        }
    }
    cExtra = Dc_Attr->lTextExtra;
    Dc_Attr->lTextExtra = CharExtra;
    return cExtra;
// return GetAndSetDCDWord( hDC, GdiGetSetTextCharExtra, CharExtra, 0, 0, 0 );
}

/*
 * @implemented
 *
 */
UINT
WINAPI
GetTextAlign(HDC hdc)
{
    PDC_ATTR Dc_Attr;
    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return 0;
    return Dc_Attr->lTextAlign;
}


/*
 * @implemented
 *
 */
COLORREF
WINAPI
GetTextColor(HDC hdc)
{
    PDC_ATTR Dc_Attr;
    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return 0;
    return Dc_Attr->ulForegroundClr;
}



/*
 * @unimplemented
 */
UINT
WINAPI
SetTextAlign(HDC hdc,
             UINT fMode)
{
    PDC_ATTR Dc_Attr;
    INT OldMode;
#if 0
    if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_SetTextAlign( hdc, fMode )
                   else
            {
                PLDC pLDC = Dc_Attr->pvLDC;
                if ( !pLDC )
                {
                    SetLastError(ERROR_INVALID_HANDLE);
                    return FALSE;
                }
                if (pLDC->iType == LDC_EMFLDC)
                {
                    if return EMFDRV_SetTextAlign( hdc, fMode )
                              }
                      }
              }
#endif
              if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return GDI_ERROR;

    OldMode = Dc_Attr->lTextAlign;
    Dc_Attr->lTextAlign = fMode; // Raw
    if (Dc_Attr->dwLayout & LAYOUT_RTL)
    {
        if ((fMode & TA_CENTER) != TA_CENTER) fMode ^= TA_RIGHT;
    }
    Dc_Attr->flTextAlign = fMode & TA_MASK;
    return OldMode;
}


/*
 * @implemented
 */
COLORREF
WINAPI
SetTextColor(
    HDC hdc,
    COLORREF crColor
)
{
    PDC_ATTR Dc_Attr;
    COLORREF OldColor = CLR_INVALID;
#if 0
    if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_SetTextColor( hDC, crColor );
        else
        {
            PLDC pLDC = Dc_Attr->pvLDC;
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
            if (pLDC->iType == LDC_EMFLDC)
            {
                if return EMFDRV_SetTextColor( hDC, crColor );
            }
        }
    }
#endif
    if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return OldColor;

    OldColor = (COLORREF) Dc_Attr->ulForegroundClr;
    Dc_Attr->ulForegroundClr = (ULONG) crColor;

    if ( Dc_Attr->crForegroundClr != crColor )
    {
        Dc_Attr->ulDirty_ |= (DIRTY_TEXT|DIRTY_LINE|DIRTY_FILL);
        Dc_Attr->crForegroundClr = crColor;
    }
    return OldColor;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetTextJustification(
    HDC	hdc,
    int	extra,
    int	breaks
)
{
    PDC_ATTR Dc_Attr;
#if 0
    if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_SetTextJustification( hdc, extra, breaks )
                   else
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return FALSE;
            }
#endif
        if (!GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &Dc_Attr)) return FALSE;

        if (NtCurrentTeb()->GdiTebBatch.HDC == hdc)
        {
            if (Dc_Attr->ulDirty_ & DC_FONTTEXT_DIRTY)
            {
                NtGdiFlush(); // Sync up Dc_Attr from Kernel space.
                Dc_Attr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
            }
        }
        Dc_Attr->cBreak = breaks;
        Dc_Attr->lBreakExtra = extra;
        return TRUE;
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

/*
 * @implemented
 */
BOOL
WINAPI
GetETM(HDC hdc,
       EXTTEXTMETRIC *petm)
{
    BOOL Ret = NtGdiGetETM(hdc, petm);

    if (Ret && petm)
        petm->emKernPairs = (WORD)GetKerningPairsA(hdc, 0, 0);

    return Ret;
}
