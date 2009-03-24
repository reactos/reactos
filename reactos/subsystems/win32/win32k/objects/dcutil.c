
#include <w32k.h>

#define NDEBUG
#include <debug.h>

COLORREF FASTCALL
IntGdiSetBkColor(HDC hDC, COLORREF color)
{
    COLORREF oldColor;
    PDC dc;
    PDC_ATTR pdcattr;
    HBRUSH hBrush;

    if (!(dc = DC_LockDc(hDC)))
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return CLR_INVALID;
    }
    pdcattr = dc->pdcattr;
    oldColor = pdcattr->crBackgroundClr;
    pdcattr->crBackgroundClr = color;
    pdcattr->ulBackgroundClr = (ULONG)color;
    pdcattr->ulDirty_ &= ~(DIRTY_BACKGROUND|DIRTY_LINE|DIRTY_FILL); // Clear Flag if set.
    hBrush = pdcattr->hbrush;
    DC_UnlockDc(dc);
    NtGdiSelectBrush(hDC, hBrush);
    return oldColor;
}

INT FASTCALL
IntGdiSetBkMode(HDC hDC, INT Mode)
{
    COLORREF oldMode;
    PDC dc;
    PDC_ATTR pdcattr;

    if (!(dc = DC_LockDc(hDC)))
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return CLR_INVALID;
    }
    pdcattr = dc->pdcattr;
    oldMode = pdcattr->lBkMode;
    pdcattr->jBkMode = Mode;
    pdcattr->lBkMode = Mode;
    DC_UnlockDc(dc);
    return oldMode;
}

UINT
FASTCALL
IntGdiSetTextAlign(HDC  hDC,
                   UINT  Mode)
{
    UINT prevAlign;
    DC *dc;
    PDC_ATTR pdcattr;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return GDI_ERROR;
    }
    pdcattr = dc->pdcattr;
    prevAlign = pdcattr->lTextAlign;
    pdcattr->lTextAlign = Mode;
    DC_UnlockDc(dc);
    return  prevAlign;
}

COLORREF
FASTCALL
IntGdiSetTextColor(HDC hDC,
                   COLORREF color)
{
    COLORREF  oldColor;
    PDC  dc = DC_LockDc(hDC);
    PDC_ATTR pdcattr;
    HBRUSH hBrush;

    if (!dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return CLR_INVALID;
    }
    pdcattr = dc->pdcattr;

    oldColor = pdcattr->crForegroundClr;
    pdcattr->crForegroundClr = color;
    hBrush = pdcattr->hbrush;
    pdcattr->ulDirty_ &= ~(DIRTY_TEXT|DIRTY_LINE|DIRTY_FILL);
    DC_UnlockDc(dc);
    NtGdiSelectBrush(hDC, hBrush);
    return  oldColor;
}

VOID
FASTCALL
DCU_SetDcUndeletable(HDC  hDC)
{
    PDC dc = DC_LockDc(hDC);
    if (!dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return;
    }

    dc->fs |= DC_FLAG_PERMANENT;
    DC_UnlockDc(dc);
    return;
}

#if 0
BOOL FASTCALL
IntIsPrimarySurface(SURFOBJ *SurfObj)
{
    if (PrimarySurface.pSurface == NULL)
    {
        return FALSE;
    }
    return SurfObj->hsurf == PrimarySurface.pSurface; // <- FIXME: WTF?
}
#endif


BOOL APIENTRY
NtGdiCancelDC(HDC  hDC)
{
    UNIMPLEMENTED;
    return FALSE;
}


WORD APIENTRY
IntGdiSetHookFlags(HDC hDC, WORD Flags)
{
    WORD wRet;
    DC *dc = DC_LockDc(hDC);

    if (NULL == dc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return 0;
    }

    wRet = dc->fs & DC_FLAG_DIRTY_RAO; // Fixme wrong flag!

    /* "Undocumented Windows" info is slightly confusing.
     */

    DPRINT("DC %p, Flags %04x\n", hDC, Flags);

    if (Flags & DCHF_INVALIDATEVISRGN)
    {
        /* hVisRgn has to be updated */
        dc->fs |= DC_FLAG_DIRTY_RAO;
    }
    else if (Flags & DCHF_VALIDATEVISRGN || 0 == Flags)
    {
        dc->fs &= ~DC_FLAG_DIRTY_RAO;
    }

    DC_UnlockDc(dc);

    return wRet;
}


BOOL
APIENTRY
NtGdiGetDCDword(
    HDC hDC,
    UINT u,
    DWORD *Result)
{
    BOOL Ret = TRUE;
    PDC pdc;
    PDC_ATTR pdcattr;

    DWORD SafeResult = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!Result)
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = pdc->pdcattr;

    switch (u)
    {
        case GdiGetJournal:
            break;

        case GdiGetRelAbs:
            SafeResult = pdcattr->lRelAbs;
            break;

        case GdiGetBreakExtra:
            SafeResult = pdcattr->lBreakExtra;
            break;

        case GdiGerCharBreak:
            SafeResult = pdcattr->cBreak;
            break;

        case GdiGetArcDirection:
            if (pdcattr->dwLayout & LAYOUT_RTL)
                SafeResult = AD_CLOCKWISE - ((pdc->dclevel.flPath & DCPATH_CLOCKWISE) != 0);
            else
                SafeResult = ((pdc->dclevel.flPath & DCPATH_CLOCKWISE) != 0) + AD_COUNTERCLOCKWISE;
            break;

        case GdiGetEMFRestorDc:
            break;

        case GdiGetFontLanguageInfo:
            SafeResult = IntGetFontLanguageInfo(pdc);
            break;

        case GdiGetIsMemDc:
            SafeResult = pdc->dctype;
            break;

        case GdiGetMapMode:
            SafeResult = pdcattr->iMapMode;
            break;

        case GdiGetTextCharExtra:
            SafeResult = pdcattr->lTextExtra;
            break;

        default:
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            Ret = FALSE;
            break;
    }

    if (Ret)
    {
        _SEH2_TRY
        {
            ProbeForWrite(Result, sizeof(DWORD), 1);
            *Result = SafeResult;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            Ret = FALSE;
        }
    }

    DC_UnlockDc(pdc);
    return Ret;
}

BOOL
APIENTRY
NtGdiGetAndSetDCDword(
    HDC hDC,
    UINT u,
    DWORD dwIn,
    DWORD *Result)
{
    BOOL Ret = TRUE;
    PDC pdc;
    PDC_ATTR pdcattr;

    DWORD SafeResult = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    if (!Result)
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = pdc->pdcattr;

    switch (u)
    {
        case GdiGetSetCopyCount:
            SafeResult = pdc->ulCopyCount;
            pdc->ulCopyCount = dwIn;
            break;

        case GdiGetSetTextAlign:
            SafeResult = pdcattr->lTextAlign;
            pdcattr->lTextAlign = dwIn;
            // pdcattr->flTextAlign = dwIn; // Flags!
            break;

        case GdiGetSetRelAbs:
            SafeResult = pdcattr->lRelAbs;
            pdcattr->lRelAbs = dwIn;
            break;

        case GdiGetSetTextCharExtra:
            SafeResult = pdcattr->lTextExtra;
            pdcattr->lTextExtra = dwIn;
            break;

        case GdiGetSetSelectFont:
            break;

        case GdiGetSetMapperFlagsInternal:
            if (dwIn & ~1)
            {
                SetLastWin32Error(ERROR_INVALID_PARAMETER);
                Ret = FALSE;
                break;
            }
            SafeResult = pdcattr->flFontMapper;
            pdcattr->flFontMapper = dwIn;
            break;

        case GdiGetSetMapMode:
            SafeResult = IntGdiSetMapMode(pdc, dwIn);
            break;

        case GdiGetSetArcDirection:
            if (dwIn != AD_COUNTERCLOCKWISE && dwIn != AD_CLOCKWISE)
            {
                SetLastWin32Error(ERROR_INVALID_PARAMETER);
                Ret = FALSE;
                break;
            }
            if (pdcattr->dwLayout & LAYOUT_RTL) // Right to Left
            {
                SafeResult = AD_CLOCKWISE - ((pdc->dclevel.flPath & DCPATH_CLOCKWISE) != 0);
                if (dwIn == AD_CLOCKWISE)
                {
                    pdc->dclevel.flPath &= ~DCPATH_CLOCKWISE;
                    break;
                }
                pdc->dclevel.flPath |= DCPATH_CLOCKWISE;
            }
            else // Left to Right
            {
                SafeResult = ((pdc->dclevel.flPath & DCPATH_CLOCKWISE) != 0) +
                             AD_COUNTERCLOCKWISE;
                if (dwIn == AD_COUNTERCLOCKWISE)
                {
                    pdc->dclevel.flPath &= ~DCPATH_CLOCKWISE;
                    break;
                }
                pdc->dclevel.flPath |= DCPATH_CLOCKWISE;
            }
            break;

        default:
            SetLastWin32Error(ERROR_INVALID_PARAMETER);
            Ret = FALSE;
            break;
    }

    if (Ret)
    {
        _SEH2_TRY
        {
            ProbeForWrite(Result, sizeof(DWORD), 1);
            *Result = SafeResult;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            Ret = FALSE;
        }
    }

    DC_UnlockDc(pdc);
    return Ret;
}
