/*
 * PROJECT:         ReactOS win32 subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         DC/font related functions
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#include "font.h"

#define NDEBUG
#include <debug.h>

PRFONT
NTAPI
LFONT_prfntFindRFONT(
    IN PLFONT plfnt)
{

    //if (!plfnt->hPFE) return NULL;

    //ppfe = GDIOBJ_LockObject(plfnt->hPFE, 0);

    //if (!ppfe) return NULL;


    return NULL;
}


PRFONT
NTAPI
RealizeFont()
{

    // 1. search for a realization that fits

    //ppfe = PFT_ppfeFindBestMatch(ppft,


    //
    return NULL;
}

PRFONT
NTAPI
DC_prfnt(PDC pdc)
{
    PRFONT prfnt = NULL;
    PLFONT plfnt;
    PPFE ppfe = NULL;

    /* Select "current" font */
    DC_hSelectFont(pdc, pdc->pdcattr->hlfntNew);

    /* Get current LFONT */
    plfnt = pdc->dclevel.plfnt;

    /* Check if the font is already realized */
    if (pdc->hlfntCur == plfnt->baseobj.hHmgr)
    {
        /* Return the RFONT */
        return pdc->prfnt;

    }

    /* Try to find a linked RFONT */
    prfnt = LFONT_prfntFindLinkedRFONT(plfnt, &pdc->dclevel.mxWorldToDevice);

    /* Check if we got one */
    if (!prfnt)
    {
        /* Check if the output device has fonts */
        if (pdc->ppdev->devinfo.cFonts != 0)
        {
            __debugbreak();
            // ppfe = PDEVOBJ_ppfe
                // if ppdev has no associated PFF
                    // if cFonts == -1, call DrvQueryFont with iFile = 0, return is cFonts
                    // create a PFF with cFonts PFEs
                    // link the PFF to the PDEV
                // ppfe = PFF_pfeFindBestMatch(pff, ...)
                    // loop all PFEs of the PFF
                        //  PFE_ulPenalty
            // set plfnt->hPFE
        }

        /* Did we find a PFE yet? */
        if (!ppfe)
        {
            /* Get best matching PFE */
            ppfe = LFONT_ppfe(plfnt);
        }

        /* Try to find a matching RFONT */
        prfnt = 0;//PFE_prfntFindRFONT(ppfe, plfnt->nSize, &pdc->dclevel.mxWorldToDevice);

        /* Allocate a new RFONT */
        prfnt = RFONT_AllocRFONT();

        /* Check if that succeeded */
        if (prfnt)
        {
            __debugbreak();
        }
        else
        {
            /* Fall back to the system font */
            if (!prfnt) prfnt = gprfntSystemTT;
            __debugbreak();
        }
    }

    ASSERT(prfnt);

    /* Reference the new and dereference the old RFONT */
    InterlockedIncrementUL(&prfnt->cSelected);
    if (InterlockedDecrementUL(&((PRFONT)pdc->prfnt)->cSelected) == 0)
    {
        /* Old RFONT is not in use anymore, delete it */
        //RFONT_vDeleteRFONT(pdc->prfnt);
        __debugbreak();
    }

    /* Set as new active font */
    pdc->prfnt = prfnt;
    pdc->hlfntCur = pdc->dclevel.plfnt->baseobj.hHmgr;

    return prfnt;
}

W32KAPI
BOOL
APIENTRY
NtGdiAddRemoteFontToDC(
    IN HDC hdc,
    IN PVOID pvBuffer,
    IN ULONG cjBuffer,
    IN OPTIONAL PUNIVERSAL_FONT_ID pufi)
{
    ASSERT(FALSE);
    return FALSE;
}


W32KAPI
BOOL
APIENTRY
NtGdiAddEmbFontToDC(
    IN HDC hdc,
    IN VOID **pFontID)
{
    ASSERT(FALSE);
    return FALSE;
}

W32KAPI
ULONG
APIENTRY
NtGdiGetEmbedFonts(VOID)
{
    ASSERT(FALSE);
    return FALSE;
}


W32KAPI
DWORD
APIENTRY
NtGdiGetCharSet(
    IN HDC hdc)
{
    ASSERT(FALSE);
#if 0
    PDC pdc;

    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        return -1; // FIXME ???
    }

    prfnt = DC_prfnt(pdc);

    jWinCharSet = prfnt->ppfe->pifi->jWinCharSet;


    return jWinCharSet;
#endif
return 0;
}

W32KAPI
BOOL
APIENTRY
NtGdiSetFontXform(
    IN HDC hdc,
    IN DWORD dwxScale,
    IN DWORD dwyScale)
{
    ASSERT(FALSE);
    return FALSE;
}


W32KAPI
BOOL
APIENTRY
NtGdiSetTextJustification(
    IN HDC hdc,
    IN INT lBreakExtra,
    IN INT cBreak)
{
    ASSERT(FALSE);
    return FALSE;
}


