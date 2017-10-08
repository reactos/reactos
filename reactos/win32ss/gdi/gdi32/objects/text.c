#include <precomp.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
BOOL
WINAPI
TextOutA(
    _In_ HDC hdc,
    _In_ INT nXStart,
    _In_ INT nYStart,
    _In_reads_(cchString) LPCSTR lpString,
    _In_ INT cchString)
{
    ANSI_STRING StringA;
    UNICODE_STRING StringU;
    BOOL bResult;

    if (lpString != NULL)
    {
        RtlInitAnsiString(&StringA, (LPSTR)lpString);
        RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);
    }
    else
    {
        StringU.Buffer = NULL;
    }

    bResult = TextOutW(hdc, nXStart, nYStart, StringU.Buffer, cchString);

    RtlFreeUnicodeString(&StringU);
    return bResult;
}


/*
 * @implemented
 */
BOOL
WINAPI
TextOutW(
    _In_ HDC hdc,
    _In_ INT nXStart,
    _In_ INT nYStart,
    _In_reads_(cchString) LPCWSTR lpString,
    _In_ INT cchString)
{
    return ExtTextOutW(hdc, nXStart, nYStart, 0, NULL, (LPWSTR)lpString, cchString, NULL);
}


/*
 * @unimplemented
 */
BOOL
WINAPI
PolyTextOutA(
    _In_ HDC hdc,
    _In_reads_(cStrings) const POLYTEXTA *pptxt,
    _In_ INT cStrings)
{
    for (; cStrings>0; cStrings--, pptxt++)
    {
        if (!ExtTextOutA(hdc,
                         pptxt->x,
                         pptxt->y,
                         pptxt->uiFlags,
                         &pptxt->rcl,
                         pptxt->lpstr,
                         pptxt->n,
                         pptxt->pdx))
        {
            return FALSE;
        }
    }

    return TRUE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
PolyTextOutW(
    _In_ HDC hdc,
    _In_reads_(cStrings) const POLYTEXTW *pptxt,
    _In_ INT cStrings)
{
    for (; cStrings>0; cStrings--, pptxt++)
    {
        if (!ExtTextOutW(hdc,
                         pptxt->x,
                         pptxt->y,
                         pptxt->uiFlags,
                         &pptxt->rcl,
                         pptxt->lpstr,
                         pptxt->n,
                         pptxt->pdx))
        {
            return FALSE;
        }
    }

    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
GdiGetCodePage(
    _In_ HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (pdcattr->ulDirty_ & DIRTY_CHARSET)
        return LOWORD(NtGdiGetCharSet(hdc));

    return LOWORD(pdcattr->iCS_CP);
}


/*
 * @unimplemented
 */
INT
WINAPI
GetTextCharacterExtra(
    _In_ HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        /* Do not set LastError here! */
        return 0x8000000;
    }

    return pdcattr->lTextExtra;
}


/*
 * @implemented
 */
INT
WINAPI
GetTextCharset(
    _In_ HDC hdc)
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
    _In_  HDC hdc,
    _Out_ LPTEXTMETRICA lptm)
{
    TMW_INTERNAL tmwi;

    if (!NtGdiGetTextMetricsW(hdc, &tmwi, sizeof(TMW_INTERNAL)))
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
    _In_  HDC hdc,
    _Out_ LPTEXTMETRICW lptm)
{
    TMW_INTERNAL tmwi;

    if (!NtGdiGetTextMetricsW(hdc, &tmwi, sizeof(TMW_INTERNAL)))
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
    _In_ HDC hdc,
    _In_reads_(cchString) LPCSTR lpString,
    _In_ INT cchString,
    _Out_ LPSIZE lpsz)
{
    ANSI_STRING StringA;
    UNICODE_STRING StringU;
    BOOL ret;

    RtlInitAnsiString(&StringA, (LPSTR)lpString);
    RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

    ret = GetTextExtentPointW(hdc, StringU.Buffer, cchString, lpsz);

    RtlFreeUnicodeString(&StringU);

    return ret;
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentPointW(
    _In_ HDC hdc,
    _In_reads_(cchString) LPCWSTR lpString,
    _In_ INT cchString,
    _Out_ LPSIZE lpsz)
{
    return NtGdiGetTextExtent(hdc, (LPWSTR)lpString, cchString, lpsz, 0);
}


/*
 * @implemented
 */
BOOL
WINAPI
GetTextExtentExPointW(
    _In_ HDC hdc,
    _In_reads_(cchString) LPCWSTR lpszString,
    _In_ INT cchString,
    _In_ INT nMaxExtent,
    _Out_opt_ LPINT lpnFit,
    _Out_writes_to_opt_(cchString, *lpnFit) LPINT lpnDx,
    _Out_ LPSIZE lpSize)
{

    /* Windows doesn't check nMaxExtent validity in unicode version */
    if (nMaxExtent < -1)
    {
        DPRINT("nMaxExtent is invalid: %d\n", nMaxExtent);
    }

    return NtGdiGetTextExtentExW (
               hdc, (LPWSTR)lpszString, cchString, nMaxExtent, (PULONG)lpnFit, (PULONG)lpnDx, lpSize, 0 );
}


/*
 * @implemented
 */
BOOL
WINAPI
GetTextExtentExPointWPri(
    _In_ HDC hdc,
    _In_reads_(cwc) LPWSTR lpwsz,
    _In_ ULONG cwc,
    _In_ ULONG dxMax,
    _Out_opt_ ULONG *pcCh,
    _Out_writes_to_opt_(cwc, *pcCh) PULONG pdxOut,
    _In_ LPSIZE psize)
{
    return NtGdiGetTextExtentExW(hdc, lpwsz, cwc, dxMax, pcCh, pdxOut, psize, 0);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetTextExtentExPointA(
    _In_ HDC hdc,
    _In_reads_(cchString) LPCSTR lpszStr,
    _In_ INT cchString,
    _In_ INT nMaxExtent,
    _Out_opt_ LPINT lpnFit,
    _Out_writes_to_opt_ (cchString, *lpnFit) LPINT lpnDx,
    _Out_ LPSIZE lpSize)
{
    NTSTATUS Status;
    LPWSTR lpszStrW;
    BOOL bResult = FALSE;

    if (nMaxExtent < -1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Status = HEAP_strdupA2W(&lpszStrW, lpszStr);
    if (!NT_SUCCESS (Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    bResult = NtGdiGetTextExtentExW(hdc,
                                    lpszStrW,
                                    cchString,
                                    nMaxExtent,
                                    (PULONG)lpnFit,
                                    (PULONG)lpnDx,
                                    lpSize,
                                    0);

    HEAP_free(lpszStrW);

    return bResult;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetTextExtentPoint32A(
    _In_ HDC hdc,
    _In_reads_(cchString) LPCSTR lpString,
    _In_ INT cchString,
    _Out_ LPSIZE lpSize)
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
WINAPI
GetTextExtentPoint32W(
    _In_ HDC hdc,
    _In_reads_(cchString) LPCWSTR lpString,
    _In_ int cchString,
    _Out_ LPSIZE lpSize)
{
    return NtGdiGetTextExtent(hdc, (LPWSTR)lpString, cchString, lpSize, 0);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetTextExtentExPointI(
    _In_ HDC hdc,
    _In_reads_(cgi) LPWORD pgiIn,
    _In_ INT cgi,
    _In_ INT nMaxExtent,
    _Out_opt_ LPINT lpnFit,
    _Out_writes_to_opt_(cwchString, *lpnFit) LPINT lpnDx,
    _Out_ LPSIZE lpSize)
{
    return NtGdiGetTextExtentExW(hdc,
                                 pgiIn,
                                 cgi,
                                 nMaxExtent,
                                 (PULONG)lpnFit,
                                 (PULONG)lpnDx,
                                 lpSize,
                                 GTEF_INDICES);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetTextExtentPointI(
    _In_ HDC hdc,
    _In_reads_(cgi) LPWORD pgiIn,
    _In_ int cgi,
    _Out_ LPSIZE lpSize)
{
    return NtGdiGetTextExtent(hdc, pgiIn, cgi, lpSize, GTEF_INDICES);
}

/*
 * @implemented
 */
BOOL
WINAPI
ExtTextOutA(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _In_ UINT fuOptions,
    _In_opt_ const RECT *lprc,
    _In_reads_opt_(cch) LPCSTR lpString,
    _In_ UINT cch,
    _In_reads_opt_(cch) const INT *lpDx)
{
    ANSI_STRING StringA;
    UNICODE_STRING StringU;
    BOOL ret;

    RtlInitAnsiString(&StringA, (LPSTR)lpString);
    RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

    ret = ExtTextOutW(hdc, x, y, fuOptions, lprc, StringU.Buffer, cch, lpDx);

    RtlFreeUnicodeString(&StringU);

    return ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
ExtTextOutW(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _In_ UINT fuOptions,
    _In_opt_ const RECT *lprc,
    _In_reads_opt_(cwc) LPCWSTR lpString,
    _In_ UINT cwc,
    _In_reads_opt_(cwc) const INT *lpDx)
{
    HANDLE_METADC(BOOL,
                  ExtTextOut,
                  FALSE,
                  hdc,
                  x,
                  y,
                  fuOptions,
                  lprc,
                  lpString,
                  cwc,
                  lpDx);

    return NtGdiExtTextOutW(hdc,
                            x,
                            y,
                            fuOptions,
                            (LPRECT)lprc,
                            (LPWSTR)lpString,
                            cwc,
                            (LPINT)lpDx,
                            0);
}


/*
 * @implemented
 */
INT
WINAPI
GetTextFaceW(
    _In_ HDC hdc,
    _In_ INT cwcMax,
    _Out_writes_to_opt_(cwcMax, return) LPWSTR pFaceName)
{
    /* Validate parameters */
    if (pFaceName && cwcMax <= 0)
    {
        /* Set last error and return failure */
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Forward to kernel */
    return NtGdiGetTextFaceW(hdc, cwcMax, pFaceName, FALSE);
}


/*
 * @implemented
 */
INT
WINAPI
GetTextFaceA(
    _In_ HDC hdc,
    _In_ INT cchMax,
    _Out_writes_to_opt_(cchMax, return) LPSTR lpName)
{
    INT res;
    LPWSTR nameW;

    /* Validate parameters */
    if (lpName && cchMax <= 0)
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

    if (lpName)
    {
        if (cchMax && !WideCharToMultiByte( CP_ACP, 0, nameW, -1, lpName, cchMax, NULL, NULL))
            lpName[cchMax-1] = 0;
        res = strlen(lpName);
    }
    else
    {
        res = WideCharToMultiByte( CP_ACP, 0, nameW, -1, NULL, 0, NULL, NULL);
    }

    HeapFree( GetProcessHeap(), 0, nameW );
    return res;
}


/*
 * @implemented
 */
INT
WINAPI
GetTextFaceAliasW(
    _In_ HDC hdc,
    _In_ INT cwcMax,
    _Out_writes_to_opt_(cwcMax, return) LPWSTR pszOut)
{
    if (pszOut && !cwcMax)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    return NtGdiGetTextFaceW(hdc, cwcMax, pszOut, TRUE);
}


BOOL
WINAPI
GetFontResourceInfoW(
    _In_z_ LPCWSTR lpFileName,
    _Inout_ DWORD *pdwBufSize,
    _Out_writes_to_opt_(*pdwBufSize, 1) PVOID lpBuffer,
    _In_ DWORD dwType)
{
    BOOL bRet;
    UNICODE_STRING NtFileName;

    DPRINT("GetFontResourceInfoW: dwType = %lu\n", dwType);

    if (!lpFileName || !pdwBufSize)
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

    return bRet;
}


/*
 * @unimplemented
 */
INT
WINAPI
SetTextCharacterExtra(
    _In_ HDC hdc,
    _In_ INT nCharExtra)
{
    PDC_ATTR pdcattr;
    INT nOldCharExtra;

    if (nCharExtra == 0x80000000)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0x80000000;
    }

    if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE)
    {
        HANDLE_METADC(INT, SetTextCharacterExtra, 0x80000000, hdc, nCharExtra);
    }

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0x8000000;
    }

    if (NtCurrentTeb()->GdiTebBatch.HDC == hdc)
    {
        if (pdcattr->ulDirty_ & DC_FONTTEXT_DIRTY)
        {
            NtGdiFlush(); // Sync up pdcattr from Kernel space.
            pdcattr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
        }
    }

    nOldCharExtra = pdcattr->lTextExtra;
    pdcattr->lTextExtra = nCharExtra;
    return nOldCharExtra;
}

/*
 * @implemented
 *
 */
UINT
WINAPI
GetTextAlign(
    _In_ HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        /* Do not set LastError here! */
        return GDI_ERROR;
    }

    return pdcattr->lTextAlign;
}


/*
 * @implemented
 *
 */
COLORREF
WINAPI
GetTextColor(
    _In_ HDC hdc)
{
    PDC_ATTR pdcattr;

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        /* Do not set LastError here! */
        return CLR_INVALID;
    }

    return pdcattr->ulForegroundClr;
}


/*
 * @unimplemented
 */
UINT
WINAPI
SetTextAlign(
    _In_ HDC hdc,
    _In_ UINT fMode)
{
    PDC_ATTR pdcattr;
    UINT fOldMode;

    HANDLE_METADC(BOOL, SetTextAlign, GDI_ERROR, hdc, fMode);

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return GDI_ERROR;
    }


    fOldMode = pdcattr->lTextAlign;
    pdcattr->lTextAlign = fMode; // Raw
    if (pdcattr->dwLayout & LAYOUT_RTL)
    {
        if ((fMode & TA_CENTER) != TA_CENTER) fMode ^= TA_RIGHT;
    }

    pdcattr->flTextAlign = fMode & TA_MASK;
    return fOldMode;
}


/*
 * @implemented
 */
COLORREF
WINAPI
SetTextColor(
    _In_ HDC hdc,
    _In_ COLORREF crColor)
{
    PDC_ATTR pdcattr;
    COLORREF crOldColor;

    HANDLE_METADC(COLORREF, SetTextColor, CLR_INVALID, hdc, crColor);

    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return CLR_INVALID;
    }

    crOldColor = (COLORREF) pdcattr->ulForegroundClr;
    pdcattr->ulForegroundClr = (ULONG)crColor;

    if (pdcattr->crForegroundClr != crColor)
    {
        pdcattr->ulDirty_ |= (DIRTY_TEXT|DIRTY_LINE|DIRTY_FILL);
        pdcattr->crForegroundClr = crColor;
    }

    return crOldColor;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetTextJustification(
    _In_ HDC hdc,
    _In_ INT nBreakExtra,
    _In_ INT nBreakCount)
{
    PDC_ATTR pdcattr;

    if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE)
    {
        HANDLE_METADC(BOOL, SetTextJustification, FALSE, hdc, nBreakExtra, nBreakCount);
    }

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if (pdcattr == NULL)
    {
        /* Do not set LastError here! */
        return GDI_ERROR;
    }


    if (NtCurrentTeb()->GdiTebBatch.HDC == hdc)
    {
        if (pdcattr->ulDirty_ & DC_FONTTEXT_DIRTY)
        {
            NtGdiFlush(); // Sync up pdcattr from Kernel space.
            pdcattr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
        }
    }

    pdcattr->cBreak = nBreakCount;
    pdcattr->lBreakExtra = nBreakExtra;
    return TRUE;
}

/*
 * @implemented
 */
UINT
WINAPI
GetStringBitmapA(
    _In_ HDC hdc,
    _In_ LPSTR psz,
    _In_ BOOL bDoCall,
    _In_ UINT cj,
    _Out_writes_(cj) BYTE *lpSB)
{

    NTSTATUS Status;
    PWSTR pwsz;
    UINT uResult = 0;

    if (!bDoCall)
    {
        return 0;
    }

    Status = HEAP_strdupA2W(&pwsz, psz);
    if (!NT_SUCCESS(Status))
    {
        SetLastError (RtlNtStatusToDosError(Status));
    }
    else
    {
        uResult = NtGdiGetStringBitmapW(hdc, pwsz, 1, lpSB, cj);
        HEAP_free(pwsz);
    }

    return uResult;

}

/*
 * @implemented
 */
UINT
WINAPI
GetStringBitmapW(
    _In_ HDC hdc,
    _In_ LPWSTR pwsz,
    _In_ BOOL bDoCall,
    _In_ UINT cj,
    _Out_writes_(cj) BYTE *lpSB)
{
    if (!bDoCall)
    {
        return 0;
    }

    return NtGdiGetStringBitmapW(hdc, pwsz, 1, lpSB, cj);

}

/*
 * @implemented
 */
BOOL
WINAPI
GetETM(
    _In_ HDC hdc,
    _Out_ EXTTEXTMETRIC *petm)
{
    BOOL bResult;

    bResult = NtGdiGetETM(hdc, petm);

    if (bResult && petm)
    {
        petm->emKernPairs = (WORD)GetKerningPairsA(hdc, 0, 0);
    }

    return bResult;
}
