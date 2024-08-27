/*
 * PROJECT:     ReactOS GDI32
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Text drawing API.
 * COPYRIGHT:   Copyright 2014 Timo Kreuzer
 *              Copyright 2017 Katayama Hirofumi MZ
 */

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
    NTSTATUS Status;

    if (lpString != NULL && cchString > 0)
    {
        if (cchString > MAXUSHORT)
            cchString = MAXUSHORT;

        StringA.Length = (USHORT)cchString;
        StringA.MaximumLength = (USHORT)cchString;
        StringA.Buffer = (PCHAR)lpString;

        Status = RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);
        if (!NT_SUCCESS(Status))
        {
            StringU.Buffer = NULL;
            StringU.Length = 0;
        }
    }
    else
    {
        StringU.Buffer = NULL;
        StringU.Length = 0;
    }

    bResult = TextOutW(hdc, nXStart, nYStart,
                       StringU.Buffer, StringU.Length / sizeof(WCHAR));

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

    if (LoadLPK(LPK_GTEP))
        return LpkGetTextExtentExPoint(hdc, lpszString, cchString, nMaxExtent, lpnFit, lpnDx, lpSize, 0, 0);

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
    _In_reads_(cwc) LPCWSTR lpwsz,
    _In_ INT cwc,
    _In_ INT dxMax,
    _Out_opt_ LPINT pcCh,
    _Out_writes_to_opt_(cwc, *pcCh) LPINT pdxOut,
    _In_ LPSIZE psize)
{
    return NtGdiGetTextExtentExW(hdc, (LPWSTR)lpwsz, cwc, dxMax, (PULONG)pcCh, (PULONG)pdxOut, psize, 0);
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

    if (fuOptions & ETO_GLYPH_INDEX)
        return ExtTextOutW(hdc, x, y, fuOptions, lprc, (LPCWSTR)lpString, cch, lpDx);

    StringA.Buffer = (PCHAR)lpString;
    StringA.Length = StringA.MaximumLength = cch;
    RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

    if (StringU.Length != StringA.Length * sizeof(WCHAR))
        DPRINT1("ERROR: Should convert lpDx properly!\n");

    ret = ExtTextOutW(hdc, x, y, fuOptions, lprc, StringU.Buffer, cch, lpDx);

    RtlFreeUnicodeString(&StringU);

    return ret;
}

static BOOL bBypassETOWMF = FALSE;

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
    PDC_ATTR pdcattr;

    // Need both, should return a parameter error? No they don't!
    if ( !lpDx && fuOptions & ETO_PDY )
        return FALSE;

    // Now sorting out rectangle.

    // Here again, need both.
    if ( lprc && !(fuOptions & (ETO_CLIPPED|ETO_OPAQUE)) )
    {
        lprc = NULL; // No flags, no rectangle.
    }
    else if (!lprc) // No rectangle, force clear flags if set and continue.
    {
       fuOptions &= ~(ETO_CLIPPED|ETO_OPAQUE);
    }

    if ( !bBypassETOWMF )
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
    }

    if ( GdiConvertAndCheckDC(hdc) == NULL ) return FALSE;

    if (!(fuOptions & (ETO_GLYPH_INDEX | ETO_IGNORELANGUAGE)))
    {
        bBypassETOWMF = TRUE;

        if (LoadLPK(LPK_ETO))
            return LpkExtTextOut(hdc, x, y, fuOptions, lprc, lpString, cwc , lpDx, 0);
    }
    else
    {
        bBypassETOWMF = FALSE;
    }

    /* Get the DC attribute */
    pdcattr = GdiGetDcAttr(hdc);
    if ( pdcattr &&
         !(pdcattr->ulDirty_ & DC_DIBSECTION) &&
         !(pdcattr->lTextAlign & TA_UPDATECP))
    {
        if ( lprc && !cwc )
        {
            if ( fuOptions & ETO_OPAQUE )
            {
                PGDIBSEXTTEXTOUT pgO;

                pgO = GdiAllocBatchCommand(hdc, GdiBCExtTextOut);
                if (pgO)
                {
                    pdcattr->ulDirty_ |= DC_MODE_DIRTY;
                    pgO->Count = cwc;
                    pgO->Rect = *lprc;
                    pgO->Options = fuOptions;
                    /* Snapshot attribute */
                    pgO->ulBackgroundClr = pdcattr->ulBackgroundClr;
                    pgO->ptlViewportOrg  = pdcattr->ptlViewportOrg;
                    return TRUE;
                }
            }
            else // Do nothing, old explorer pops this off.
            {
                DPRINT("GdiBCExtTextOut nothing\n");
                return TRUE;
            }
        }         // Max 580 wchars, if offset 0
        else if ( cwc <= ((GDIBATCHBUFSIZE - sizeof(GDIBSTEXTOUT)) / sizeof(WCHAR)) )
        {
            PGDIBSTEXTOUT pgO;
            PTEB pTeb = NtCurrentTeb();

            pgO = GdiAllocBatchCommand(hdc, GdiBCTextOut);
            if (pgO)
            {
                USHORT cjSize = 0;
                ULONG DxSize = 0;

                if (cwc > 2) cjSize = (cwc * sizeof(WCHAR)) - sizeof(pgO->String);

                /* Calculate buffer size for string and Dx values */
                if (lpDx)
                {
                    /* If ETO_PDY is specified, we have pairs of INTs */
                    DxSize = (cwc * sizeof(INT)) * (fuOptions & ETO_PDY ? 2 : 1);
                    cjSize += DxSize;
                    // The structure buffer holds 4 bytes. Store Dx data then string.
                    // Result one wchar -> Buf[ Dx ]Str[wC], [4][2][X] one extra unused wchar
                    // to assure alignment of 4.
                }

                if ((pTeb->GdiTebBatch.Offset + cjSize ) <= GDIBATCHBUFSIZE)
                {
                    pdcattr->ulDirty_ |= DC_MODE_DIRTY|DC_FONTTEXT_DIRTY;
                    pgO->cbCount = cwc;
                    pgO->x = x;
                    pgO->y = y;
                    pgO->Options = fuOptions;
                    pgO->iCS_CP = 0;

                    if (lprc) pgO->Rect = *lprc;
                    else
                    {
                       pgO->Options |= GDIBS_NORECT; // Tell the other side lprc is nill.
                    }

                    /* Snapshot attributes */
                    pgO->crForegroundClr = pdcattr->crForegroundClr;
                    pgO->crBackgroundClr = pdcattr->crBackgroundClr;
                    pgO->ulForegroundClr = pdcattr->ulForegroundClr;
                    pgO->ulBackgroundClr = pdcattr->ulBackgroundClr;
                    pgO->lBkMode         = pdcattr->lBkMode == OPAQUE ? OPAQUE : TRANSPARENT;
                    pgO->hlfntNew        = pdcattr->hlfntNew;
                    pgO->flTextAlign     = pdcattr->flTextAlign;
                    pgO->ptlViewportOrg  = pdcattr->ptlViewportOrg;

                    pgO->Size = DxSize; // of lpDx then string after.
                    /* Put the Dx before the String to assure alignment of 4 */
                    if (lpDx) RtlCopyMemory( &pgO->Buffer, lpDx, DxSize);

                    if (cwc) RtlCopyMemory( &pgO->String[DxSize/sizeof(WCHAR)], lpString, cwc * sizeof(WCHAR));

                    // Recompute offset and return size
                    pTeb->GdiTebBatch.Offset += cjSize;
                    ((PGDIBATCHHDR)pgO)->Size += cjSize;
                    return TRUE;
                }
                // Reset offset and count then fall through
                pTeb->GdiTebBatch.Offset -= sizeof(GDIBSTEXTOUT);
                pTeb->GdiBatchCount--;
            }
        }
    }
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

    HANDLE_METADC16(INT, SetTextCharacterExtra, 0x80000000, hdc, nCharExtra);

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

    HANDLE_METADC16(BOOL, SetTextJustification, FALSE, hdc, nBreakExtra, nBreakCount);

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
