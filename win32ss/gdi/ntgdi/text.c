/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/text.c
 * PURPOSE:         Text/Font
 * PROGRAMMER:
 */

/** Includes ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/*
   This is a hack. See CORE-1091.

   It is needed because ReactOS does not support raster fonts now.
   After Raster Font support is added, then it can be removed.
   Find the current font's logfont for testing its lf.lfFaceName.

   The ftGdiGetTextMetricsW function currently in ReactOS will always return a Truetype font
   because we cannot yet handle raster fonts. So it will return flags
   TMPF_VECTOR and TMPF_TRUETYPE, which can cause problems in edit boxes.
 */

VOID FASTCALL
IntTMWFixUp(
   _In_ HDC hDC,
   _Inout_ PTMW_INTERNAL ptm)
{
    LOGFONTW lf;
    HFONT hCurrentFont;

    hCurrentFont = NtGdiGetDCObject(hDC, GDI_OBJECT_TYPE_FONT);
    GreGetObject(hCurrentFont, sizeof(LOGFONTW), &lf);

    /* To compensate for the GetTextMetricsW call changing the PitchAndFamily
     * to a TrueType one when we have a 'Raster' font as our input we filter
     * out the problematic TrueType and Vector bits.
     * Our list below checks for Raster Font Facenames. */
    DPRINT("Font Facename is '%S'.\n", lf.lfFaceName);
    if ((_wcsicmp(lf.lfFaceName, L"Courier") == 0) ||
        (_wcsicmp(lf.lfFaceName, L"FixedSys") == 0) ||
        (_wcsicmp(lf.lfFaceName, L"Helv") == 0) ||
        (_wcsicmp(lf.lfFaceName, L"MS Sans Serif") == 0) ||
        (_wcsicmp(lf.lfFaceName, L"MS Serif") == 0) ||
        (_wcsicmp(lf.lfFaceName, L"System") == 0) ||
        (_wcsicmp(lf.lfFaceName, L"Terminal") == 0) ||
        (_wcsicmp(lf.lfFaceName, L"Tms Rmn") == 0))
    {
        ptm->TextMetric.tmPitchAndFamily &= ~(TMPF_TRUETYPE | TMPF_VECTOR);
    }
}

/** Functions *****************************************************************/

BOOL FASTCALL
GreTextOutW(
    _In_ HDC hdc,
    _In_ INT nXStart,
    _In_ INT nYStart,
    _In_reads_(cchString) PCWCH lpString,
    _In_ INT cchString)
{
    return GreExtTextOutW(hdc, nXStart, nYStart, 0, NULL, lpString, cchString, NULL, 0);
}

/*
   flOpts :
   GetTextExtentPoint32W = 0
   GetTextExtentPointW   = 1
 */
BOOL
FASTCALL
GreGetTextExtentW(
    _In_ HDC hDC,
    _In_reads_(cwc) PCWCH lpwsz,
    _In_ INT cwc,
    _Out_ PSIZE psize,
    _In_ UINT flOpts)
{
    PDC pdc;
    PDC_ATTR pdcattr;
    BOOL Result;
    PTEXTOBJ TextObj;

    if (!cwc)
    {
        psize->cx = 0;
        psize->cy = 0;
        return TRUE;
    }

    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    pdcattr = pdc->pdcattr;

    TextObj = RealizeFontInit(pdcattr->hlfntNew);
    if ( TextObj )
    {
        Result = TextIntGetTextExtentPoint( pdc,
                                            TextObj,
                                            lpwsz,
                                            cwc,
                                            0,
                                            NULL,
                                            0,
                                            psize,
                                            flOpts);
        TEXTOBJ_UnlockText(TextObj);
    }
    else
        Result = FALSE;

    DC_UnlockDc(pdc);
    return Result;
}

/*
   fl :
   GetTextExtentExPointW = 0 and everything else that uses this.
   GetTextExtentExPointI = 1
 */
BOOL
FASTCALL
GreGetTextExtentExW(
    _In_ HDC hDC,
    _In_ PCWCH String,
    _In_ ULONG Count,
    _In_ ULONG MaxExtent,
    _Out_opt_ PULONG Fit,
    _Out_writes_to_opt_(Count, *Fit) PULONG Dx,
    _Out_ PSIZE pSize,
    _In_ FLONG fl)
{
    PDC pdc;
    PDC_ATTR pdcattr;
    BOOL Result;
    PTEXTOBJ TextObj;

    if ( (!String && Count ) || !pSize )
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( !Count )
    {
        if ( Fit ) Fit = 0;
        return TRUE;
    }

    pdc = DC_LockDc(hDC);
    if (NULL == pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = pdc->pdcattr;

    TextObj = RealizeFontInit(pdcattr->hlfntNew);
    if ( TextObj )
    {
        Result = TextIntGetTextExtentPoint( pdc,
                                            TextObj,
                                            String,
                                            Count,
                                            MaxExtent,
                                            (PINT)Fit,
                                            (PINT)Dx,
                                            pSize,
                                            fl);
        TEXTOBJ_UnlockText(TextObj);
    }
    else
        Result = FALSE;

    DC_UnlockDc(pdc);
    return Result;
}

BOOL
WINAPI
GreGetTextMetricsW(
    _In_  HDC hdc,
    _Out_ LPTEXTMETRICW lptm)
{
   TMW_INTERNAL tmwi;
   if (!ftGdiGetTextMetricsW(hdc, &tmwi)) return FALSE;
   IntTMWFixUp(hdc, &tmwi);
   *lptm = tmwi.TextMetric;
   return TRUE;
}

DWORD
APIENTRY
NtGdiGetCharSet(_In_ HDC hDC)
{
    PDC Dc;
    PDC_ATTR pdcattr;
    DWORD cscp;
    // If here, update everything!
    Dc = DC_LockDc(hDC);
    if (!Dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }
    cscp = ftGdiGetTextCharsetInfo(Dc, NULL, 0);
    pdcattr = Dc->pdcattr;
    pdcattr->iCS_CP = cscp;
    pdcattr->ulDirty_ &= ~DIRTY_CHARSET;
    DC_UnlockDc( Dc );
    return cscp;
}

BOOL
APIENTRY
NtGdiGetRasterizerCaps(
    _Out_ LPRASTERIZER_STATUS praststat,
    _In_ ULONG cjBytes)
{
    NTSTATUS Status = STATUS_SUCCESS;
    RASTERIZER_STATUS rsSafe;

    if (praststat && cjBytes)
    {
        if ( cjBytes >= sizeof(RASTERIZER_STATUS) ) cjBytes = sizeof(RASTERIZER_STATUS);
        if ( ftGdiGetRasterizerCaps(&rsSafe))
        {
            _SEH2_TRY
            {
                ProbeForWrite( praststat,
                sizeof(RASTERIZER_STATUS),
                1);
                RtlCopyMemory(praststat, &rsSafe, cjBytes );
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            if (!NT_SUCCESS(Status))
            {
                SetLastNtError(Status);
                return FALSE;
            }

            return TRUE;
        }
    }
    return FALSE;
}

INT
APIENTRY
NtGdiGetTextCharsetInfo(
    _In_ HDC hdc,
    _Out_opt_ LPFONTSIGNATURE lpSig,
    _In_ DWORD dwFlags)
{
    PDC Dc;
    INT Ret;
    FONTSIGNATURE fsSafe;
    PFONTSIGNATURE pfsSafe = &fsSafe;
    NTSTATUS Status = STATUS_SUCCESS;

    Dc = DC_LockDc(hdc);
    if (!Dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return DEFAULT_CHARSET;
    }

    if (!lpSig) pfsSafe = NULL;

    Ret = HIWORD(ftGdiGetTextCharsetInfo( Dc, pfsSafe, dwFlags));

    if (lpSig)
    {
        if (Ret == DEFAULT_CHARSET)
            RtlZeroMemory(pfsSafe, sizeof(FONTSIGNATURE));

        _SEH2_TRY
        {
            ProbeForWrite( lpSig,
            sizeof(FONTSIGNATURE),
            1);
            RtlCopyMemory(lpSig, pfsSafe, sizeof(FONTSIGNATURE));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            return DEFAULT_CHARSET;
        }
    }
    DC_UnlockDc(Dc);
    return Ret;
}


/*
   fl :
   GetTextExtentExPointW = 0 and everything else that uses this.
   GetTextExtentExPointI = 1
 */
W32KAPI
BOOL
APIENTRY
NtGdiGetTextExtentExW(
    _In_ HDC hDC,
    _In_reads_opt_(Count) PCWCH UnsafeString,
    _In_ ULONG Count,
    _In_ ULONG MaxExtent,
    _Out_opt_ PULONG UnsafeFit,
    _Out_writes_to_opt_(Count, *UnsafeFit) PULONG UnsafeDx,
    _Out_ PSIZE UnsafeSize,
    _In_ FLONG fl)
{
    PDC dc;
    PDC_ATTR pdcattr;
    LPWSTR String;
    SIZE Size;
    NTSTATUS Status;
    BOOLEAN Result;
    INT Fit;
    PINT Dx;
    PTEXTOBJ TextObj;

    if ((LONG)Count < 0)
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* FIXME: Handle fl */

    if (0 == Count)
    {
        Size.cx = 0;
        Size.cy = 0;
        Status = MmCopyToCaller(UnsafeSize, &Size, sizeof(SIZE));
        if (! NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            return FALSE;
        }
        return TRUE;
    }

    String = ExAllocatePoolWithTag(PagedPool, Count * sizeof(WCHAR), GDITAG_TEXT);
    if (NULL == String)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (NULL != UnsafeDx)
    {
        Dx = ExAllocatePoolWithTag(PagedPool, Count * sizeof(INT), GDITAG_TEXT);
        if (NULL == Dx)
        {
            ExFreePoolWithTag(String, GDITAG_TEXT);
            EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }
    else
    {
        Dx = NULL;
    }

    Status = MmCopyFromCaller(String, UnsafeString, Count * sizeof(WCHAR));
    if (! NT_SUCCESS(Status))
    {
        if (NULL != Dx)
        {
            ExFreePoolWithTag(Dx, GDITAG_TEXT);
        }
        ExFreePoolWithTag(String, GDITAG_TEXT);
        SetLastNtError(Status);
        return FALSE;
    }

    dc = DC_LockDc(hDC);
    if (NULL == dc)
    {
        if (NULL != Dx)
        {
            ExFreePoolWithTag(Dx, GDITAG_TEXT);
        }
        ExFreePoolWithTag(String, GDITAG_TEXT);
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = dc->pdcattr;
    TextObj = RealizeFontInit(pdcattr->hlfntNew);
    if ( TextObj )
    {
        Result = TextIntGetTextExtentPoint( dc,
                                            TextObj,
                                            String,
                                            Count,
                                            MaxExtent,
                                            NULL == UnsafeFit ? NULL : &Fit,
                                            Dx,
                                            &Size,
                                            fl);
        TEXTOBJ_UnlockText(TextObj);
    }
    else
        Result = FALSE;
    DC_UnlockDc(dc);

    ExFreePoolWithTag(String, GDITAG_TEXT);
    if (! Result)
    {
        if (NULL != Dx)
        {
            ExFreePoolWithTag(Dx, GDITAG_TEXT);
        }
        return FALSE;
    }

    if (NULL != UnsafeFit)
    {
        Status = MmCopyToCaller(UnsafeFit, &Fit, sizeof(INT));
        if (! NT_SUCCESS(Status))
        {
            if (NULL != Dx)
            {
                ExFreePoolWithTag(Dx, GDITAG_TEXT);
            }
            SetLastNtError(Status);
            return FALSE;
        }
    }

    if (NULL != UnsafeDx)
    {
        Status = MmCopyToCaller(UnsafeDx, Dx, Count * sizeof(INT));
        if (! NT_SUCCESS(Status))
        {
            if (NULL != Dx)
            {
                ExFreePoolWithTag(Dx, GDITAG_TEXT);
            }
            SetLastNtError(Status);
            return FALSE;
        }
    }
    if (NULL != Dx)
    {
        ExFreePoolWithTag(Dx, GDITAG_TEXT);
    }

    Status = MmCopyToCaller(UnsafeSize, &Size, sizeof(SIZE));
    if (! NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return FALSE;
    }

    return TRUE;
}


/*
   flOpts :
   GetTextExtentPoint32W = 0
   GetTextExtentPointW   = 1
 */
BOOL
APIENTRY
NtGdiGetTextExtent(
    _In_ HDC hdc,
    _In_reads_(cwc) PCWCH lpwsz,
    _In_ INT cwc,
    _Out_ PSIZE psize,
    _In_ UINT flOpts)
{
    return NtGdiGetTextExtentExW(hdc, lpwsz, cwc, 0, NULL, NULL, psize, flOpts);
}

BOOL
APIENTRY
NtGdiSetTextJustification(
    _In_ HDC hDC,
    _In_ INT BreakExtra,
    _In_ INT BreakCount)
{
    PDC pDc;
    PDC_ATTR pdcattr;

    pDc = DC_LockDc(hDC);
    if (!pDc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    pdcattr = pDc->pdcattr;

    pdcattr->lBreakExtra = BreakExtra;
    pdcattr->cBreak = BreakCount;

    DC_UnlockDc(pDc);
    return TRUE;
}


W32KAPI
INT
APIENTRY
NtGdiGetTextFaceW(
    _In_ HDC hDC,
    _In_ INT Count,
    _Out_writes_to_opt_(Count, return) PWSTR FaceName,
    _In_ BOOL bAliasName)
{
    PDC Dc;
    PDC_ATTR pdcattr;
    HFONT hFont;
    PTEXTOBJ TextObj;
    NTSTATUS Status;
    SIZE_T fLen;
    INT ret;

    /* FIXME: Handle bAliasName */

    Dc = DC_LockDc(hDC);
    if (Dc == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = Dc->pdcattr;
    hFont = pdcattr->hlfntNew;
    DC_UnlockDc(Dc);

    TextObj = RealizeFontInit(hFont);
    ASSERT(TextObj != NULL);
    fLen = wcslen(TextObj->TextFace) + 1;

    if (FaceName != NULL)
    {
        Count = min(Count, fLen);
        Status = MmCopyToCaller(FaceName, TextObj->TextFace, Count * sizeof(WCHAR));
        if (!NT_SUCCESS(Status))
        {
            TEXTOBJ_UnlockText(TextObj);
            SetLastNtError(Status);
            return 0;
        }
        /* Terminate if we copied only part of the font name */
        if (Count > 0 && Count < fLen)
        {
            FaceName[Count - 1] = '\0';
        }
        ret = Count;
    }
    else
    {
        ret = fLen;
    }

    TEXTOBJ_UnlockText(TextObj);
    return ret;
}

W32KAPI
BOOL
APIENTRY
NtGdiGetTextMetricsW(
    _In_ HDC hDC,
    _Out_ PTMW_INTERNAL pUnsafeTmwi,
    _In_ ULONG cj)
{
    TMW_INTERNAL Tmwi;

    if ( cj <= sizeof(TMW_INTERNAL) )
    {
        if (ftGdiGetTextMetricsW(hDC, &Tmwi))
        {
            IntTMWFixUp(hDC, &Tmwi);
            _SEH2_TRY
            {
                ProbeForWrite(pUnsafeTmwi, cj, 1);
                RtlCopyMemory(pUnsafeTmwi, &Tmwi, cj);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                SetLastNtError(_SEH2_GetExceptionCode());
                _SEH2_YIELD(return FALSE);
            }
            _SEH2_END

            return TRUE;
        }
    }
    return FALSE;
}

/* EOF */
