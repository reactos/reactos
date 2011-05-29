/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include "font.h"

#define NDEBUG
#include <debug.h>

W32KAPI
BOOL
APIENTRY
NtGdiGetTextMetricsW(
    IN HDC hdc,
    OUT TMW_INTERNAL *ptmwi,
    IN ULONG cj)
{
    PDC pdc;
    PRFONT prfnt;
    PIFIMETRICS pifi;
    BOOL bResult = TRUE;

    /* verify that the buffer is large enough */
    if (cj < sizeof(TMW_INTERNAL)) return FALSE;

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        return FALSE;
    }

    /* Get pointer to RFONT and IFI */
    prfnt = DC_prfnt(pdc);
    pifi = prfnt->ppfe->pifi;

    /* Enter SEH for buffer copy */
    _SEH2_TRY
    {
        /* Probe and fill TMW_INTERNAL */
        ProbeForWrite(ptmwi, cj, 1);
        RtlCopyMemory(&ptmwi->TextMetric, prfnt->ptmw, sizeof(TEXTMETRICW));
        ptmwi->Diff.cjotma = 0; // FIXME: what is this?
        ptmwi->Diff.chFirst = pifi->chFirstChar;
        ptmwi->Diff.chLast = pifi->chLastChar;
        ptmwi->Diff.ChDefault = pifi->chDefaultChar;
        ptmwi->Diff.ChBreak = pifi->chBreakChar;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        bResult = FALSE;
    }
    _SEH2_END

    /* Unlock the DC and return */
    DC_UnlockDc(pdc);
    return bResult;
}

W32KAPI
BOOL
APIENTRY
NtGdiGetETM(
    IN HDC hdc,
    OUT EXTTEXTMETRIC *petm)
{
    ASSERT(FALSE);
    return FALSE;
}

W32KAPI
BOOL
APIENTRY
NtGdiGetCharABCWidthsW(
    IN HDC hdc,
    IN UINT wchFirst,
    IN ULONG cwch,
    IN OPTIONAL PWCHAR pwch,
    IN FLONG fl,
    OUT PVOID pvBuf)
{
    ASSERT(FALSE);
    return FALSE;
}

W32KAPI
DWORD
APIENTRY
NtGdiGetCharacterPlacementW(
    IN HDC hdc,
    IN LPWSTR pwsz,
    IN INT nCount,
    IN INT nMaxExtent,
    IN OUT LPGCP_RESULTSW pgcpw,
    IN DWORD dwFlags)
{
    ASSERT(FALSE);
    return 0;
}

W32KAPI
BOOL
APIENTRY
NtGdiGetCharWidthW(
    IN HDC hdc,
    IN UINT wcFirst,
    IN UINT cwc,
    IN OPTIONAL PWCHAR pwc,
    IN FLONG fl,
    OUT PVOID pvBuf)
{
    ASSERT(FALSE);
    return FALSE;
}

W32KAPI
BOOL
APIENTRY
NtGdiGetCharWidthInfo(
    IN HDC hdc,
    OUT PCHWIDTHINFO pChWidthInfo)
{
    ASSERT(FALSE);
    return FALSE;
}

W32KAPI
ULONG
APIENTRY
NtGdiGetOutlineTextMetricsInternalW(
    IN HDC hdc,
    IN ULONG cjotm,
    OUT OPTIONAL OUTLINETEXTMETRICW *potmw,
    OUT TMDIFF *ptmd)
{
    ASSERT(FALSE);
    return 0;
}

W32KAPI
BOOL
APIENTRY
NtGdiGetTextExtent(
    IN HDC hdc,
    IN LPWSTR lpwsz,
    IN INT cwc,
    OUT LPSIZE psize,
    IN UINT flOpts)
{
    ASSERT(FALSE);
    return FALSE;
}

W32KAPI
BOOL
APIENTRY
NtGdiGetTextExtentExW(
    IN HDC hdc,
    IN OPTIONAL LPWSTR lpwsz,
    IN ULONG cwc,
    IN ULONG dxMax,
    OUT OPTIONAL ULONG *pcCh,
    OUT OPTIONAL PULONG pdxOut,
    OUT LPSIZE psize,
    IN FLONG fl)
{
    ASSERT(FALSE);
    return FALSE;
}

