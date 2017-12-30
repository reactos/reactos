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



VOID
NTAPI
RFONT_vGetTextMetrics(
    PRFONT prfnt,
    TMW_INTERNAL *ptmwi)
{
    PIFIMETRICS pifi = prfnt->ppfe->pifi;

    ptmwi->tmw.tmAscent = (prfnt->fddm.fxMaxAscender + 8) / 16;
    ptmwi->tmw.tmDescent = (prfnt->fddm.fxMaxDescender + 8) / 16;
    ptmwi->tmw.tmHeight = ptmwi->tmw.tmAscent + ptmwi->tmw.tmDescent;
    ptmwi->tmw.tmInternalLeading = 0; // FIXME
    ptmwi->tmw.tmExternalLeading = 0; // FIXME
    ptmwi->tmw.tmAveCharWidth = 0; // FIXME
    ptmwi->tmw.tmMaxCharWidth = prfnt->fddm.cxMax;
    ptmwi->tmw.tmWeight = pifi->usWinWeight;
    ptmwi->tmw.tmOverhang = 0; // FIXME
    ptmwi->tmw.tmDigitizedAspectX = pifi->ptlAspect.x;
    ptmwi->tmw.tmDigitizedAspectY = pifi->ptlAspect.y;
    ptmwi->tmw.tmFirstChar = pifi->wcFirstChar;
    ptmwi->tmw.tmLastChar = pifi->wcLastChar;
    ptmwi->tmw.tmDefaultChar = pifi->wcDefaultChar;
    ptmwi->tmw.tmBreakChar = pifi->wcBreakChar;
    ptmwi->tmw.tmItalic = ((pifi->fsSelection & FM_SEL_ITALIC) != 0);
    ptmwi->tmw.tmUnderlined = ((pifi->fsSelection & FM_SEL_UNDERSCORE) != 0);
    ptmwi->tmw.tmStruckOut = ((pifi->fsSelection & FM_SEL_STRIKEOUT) != 0);
    ptmwi->tmw.tmPitchAndFamily = pifi->jWinPitchAndFamily;
    ptmwi->tmw.tmCharSet = pifi->jWinCharSet;
    ptmwi->tmdiff.chFirst = pifi->chFirstChar;
    ptmwi->tmdiff.chLast = pifi->chLastChar;
    ptmwi->tmdiff.chDefault = pifi->chDefaultChar;
    ptmwi->tmdiff.chBreak = pifi->chBreakChar;
    ptmwi->tmdiff.cjotma = 0; // FIXME
}

VOID
NTAPI
RFONT_vGetETM(
    PRFONT prfnt,
    EXTTEXTMETRIC *petm)
{
    PIFIMETRICS pifi = prfnt->ppfe->pifi;

    petm->emSize = sizeof(EXTTEXTMETRIC);
    petm->emPointSize = 0; // FIXME
    petm->emOrientation = 0; // FIXME
    petm->emMasterHeight = 0; // FIXME
    petm->emMinScale = 0; // FIXME
    petm->emMaxScale = 0; // FIXME
    petm->emMasterUnits = 0; // FIXME
    petm->emCapHeight = 0; // FIXME
    petm->emXHeight = 0; // FIXME
    petm->emLowerCaseAscent = 0; // FIXME
    petm->emLowerCaseDescent = 0; // FIXME
    petm->emSlant = 0; // FIXME
    petm->emSuperScript = 0; // FIXME
    petm->emSubScript = 0; // FIXME
    petm->emSuperScriptSize = 0; // FIXME
    petm->emSubScriptSize = 0; // FIXME
    petm->emUnderlineOffset = 0; // FIXME
    petm->emUnderlineWidth = 0; // FIXME
    petm->emDoubleUpperUnderlineOffset = 0; // FIXME
    petm->emDoubleLowerUnderlineOffset = 0; // FIXME
    petm->emDoubleUpperUnderlineWidth = 0; // FIXME
    petm->emDoubleLowerUnderlineWidth = 0; // FIXME
    petm->emStrikeOutOffset = 0; // FIXME
    petm->emStrikeOutWidth = 0; // FIXME
    petm->emKernPairs = pifi->cKerningPairs;
    petm->emKernTracks = 0; // FIXME

}


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

    /* Enter SEH for buffer copy */
    _SEH2_TRY
    {
        /* Probe and fill TMW_INTERNAL */
        ProbeForWrite(ptmwi, cj, 1);
        RFONT_vGetTextMetrics(prfnt, ptmwi);
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
    PDC pdc;
    PRFONT prfnt;
    BOOL bResult = TRUE;

    /* Lock the DC */
    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        return FALSE;
    }

    /* Get pointer to RFONT */
    prfnt = DC_prfnt(pdc);

    /* Enter SEH for buffer copy */
    _SEH2_TRY
    {
        /* Probe and fill EXTTEXTMETRIC */
        ProbeForWrite(petm, sizeof(EXTTEXTMETRIC), 1);
        RFONT_vGetETM(prfnt, petm);
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

