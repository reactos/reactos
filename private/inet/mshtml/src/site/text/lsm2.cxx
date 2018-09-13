/*
 *  LSM2.CXX -- CLSMeasurer class
 *
 *  Authors:
 *      Sujal Parikh
 *      Chris Thrasher
 *      Paul  Parker
 *
 *  History:
 *      2/27/98     sujalp created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#ifndef X_LSM_HXX_
#define X_LSM_HXX_
#include "lsm.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_NUMCONV_HXX_
#define X_NUMCONV_HXX_
#include "numconv.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_OBJDIM_H_
#define X_OBJDIM_H_
#include <objdim.h>
#endif

#ifndef X_POBJDIM_H_
#define X_POBJDIM_H_
#include <pobjdim.h>
#endif

#ifndef X_HEIGHTS_H_
#define X_HEIGHTS_H_
#include <heights.h>
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include <flowlyt.hxx>
#endif

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

ExternTag(tagDontReuseLinkFonts);

#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

//+----------------------------------------------------------------------------
//
//  Member:     CLSMeasurer::Resync()
//
//  Synopsis:   This is temporary fn till we are using the CLSMeasurer and
//              CMeasurer. Essentiall does me = lsme
//
//-----------------------------------------------------------------------------

void
CLSMeasurer::Resync()
{
    _cAlignedSites = _pLS->_cAlignedSites;
    _cAlignedSitesAtBeginningOfLine = _pLS->_cAlignedSitesAtBOL;
    _cchWhiteAtBeginningOfLine = _pLS->_cWhiteAtBOL;
    _fLastWasBreak = _pLS->_li._fHasBreak;
}

// Wrapper to tell if an element is a list item or not.
inline BOOL
IsListItem (CElement *pElement)
{
    return !!pElement->HasFlag(TAGDESC_LISTITEM);
}

//+----------------------------------------------------------------------------
//  Member:     CLSMeasurer::MeasureListIndent()
//
//  Synopsis:   Compute and indent of line due to list properties (bullets and
//              numbering) in device units
//
//-----------------------------------------------------------------------------

void CLSMeasurer::MeasureListIndent()
{
    const   CParaFormat *pPF;
    BOOL    fInner = FALSE; // Keep retail compiler happy
    LONG    dxOffset = 0;
    LONG    dxPFOffset;

    pPF = MeasureGetPF(&fInner);
    dxPFOffset = pPF->GetBulletOffset(GetCalcInfo(), fInner);

    // Adjust the line height if the current line has a bullet or number
    if(_li._fHasBulletOrNum)
    {
        SIZE sizeImg;

        if (pPF->GetImgCookie(fInner) && 
            MeasureImage(pPF->GetImgCookie(fInner), &sizeImg))
        {
            // If we have an image cookie, try measuring the image.
            // If it has not come in yet or does not exist, fall through
            // to either bullet or number measuring.

            dxOffset = sizeImg.cx;

            // Adjust line height if necessary
            if(sizeImg.cy > _li._yHeight - _li._yDescent)
                _li._yHeight = sizeImg.cy + _li._yDescent;

            _li._yBulletHeight = sizeImg.cy;
        }
        else
        {
            switch (pPF->GetListing(fInner).GetType())
            {
                case CListing::BULLET:
                    MeasureSymbol(chDisc, &dxOffset);
                    break;

                case CListing::NUMBERING:
                    MeasureNumber(pPF, &dxOffset);
                    break;
            }
        }

        dxOffset = max(dxOffset, LXTODX(LIST_FIRST_REDUCTION_TWIPS));
    }

    if(dxOffset > dxPFOffset)
    {
        if (!pPF->HasRTL(fInner))
        {
            _li._xLeft += dxOffset - dxPFOffset;
        }
        else
        {
            _li._xRight += dxOffset - dxPFOffset;
        }
    }

}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::MeasureNumber(pxWidth, pyHeight)
//
// Synopsis:    Computes number width and height (if any)
//
// Returns:     number width and height
//
//-----------------------------------------------------------------------------

void CLSMeasurer::MeasureNumber(const CParaFormat *ppf, LONG *pxWidth)
{
    CCcs        * pccs;
    CTreeNode   * pNodeLI;

    pNodeLI = GetMarkup()->SearchBranchForTagInStory(GetPtp()->GetBranch(), ETAG_LI);
    Assert(pNodeLI && pNodeLI->Element()->IsTagAndBlock(ETAG_LI));
    
    pccs = GetCcsNumber(pNodeLI->GetCharFormat());

    AssertSz(pxWidth, "CLSMeasurer::MeasureNumber: invalid arg(s)");

    Assert(pccs);

    // BUGBUG (cthrash) Currently we employ Netscape-sytle numbering.
    // This means we don't adjust for the size of the index value,
    // keeping the offset constant regardless of the size of the index value
    // string.
    *pxWidth = 0;

    if (pccs)
    {
        SHORT yAscent, yDescent;

        pccs->GetBaseCcs()->GetAscentDescent(&yAscent, &yDescent);
        _li._yBulletHeight = yAscent + yDescent;
        _pLS->RecalcLineHeight(pccs, &_li);
        pccs->Release();
    }
}

//+----------------------------------------------------------------------------
//
// Member:      CLSMeasurer::GetCcsSymbol() (used for symbols & bullets)
//
// Synopsis:    Get CCcs for symbol font
//
// Returns:     ptr to symbol font CCcs
//
//-----------------------------------------------------------------------------

// Default character format for a bullet
static CCharFormat s_cfBullet;

CCcs *
CLSMeasurer::GetCcsSymbol(
    TCHAR               chSymbol,
    const CCharFormat * pcf,
    CCharFormat *       pcfRet)
{
    CCcs *              pccs = NULL;
    CCharFormat         cf;
    CCharFormat *       pcfUsed = (pcfRet != NULL) ? pcfRet : &cf;
    static BOOL         s_fBullet = FALSE;

    if (!s_fBullet)
    {
        // N.B. (johnv) For some reason, Win95 does not render the Windings font properly
        //  for certain characters at less than 7 points.  Do not go below that size!
        s_cfBullet.SetHeightInTwips( TWIPS_FROM_POINTS ( 7 ) );
        s_cfBullet._bCharSet = SYMBOL_CHARSET;
        s_cfBullet._fNarrow = FALSE;
        s_cfBullet._bPitchAndFamily = (BYTE) FF_DONTCARE;
        s_cfBullet._latmFaceName= fc().GetAtomWingdings();
        s_cfBullet._bCrcFont = s_cfBullet.ComputeFontCrc();

        s_fBullet = TRUE;
    }

    // Use bullet char format
    *pcfUsed = s_cfBullet;

    pcfUsed->_ccvTextColor    = pcf->_ccvTextColor;

    // Since we always cook up the bullet character format, we don't need
    // to cache it.
    pccs = fc().GetCcs(_pci->_hdc, _pci, pcfUsed);

    // Important - CM_SYMBOL is a special mode where out WC chars are actually
    // zero-extended MB chars.  This allows us to have a codepage-independent
    // call to ExTextOutA. (cthrash)

    pccs->GetBaseCcs()->_bConvertMode = CM_SYMBOL;

#if DBG==1
    if(!pccs)
    {
        TraceTag((tagError, "CRchMeasurer::GetCcsBullet(): no CCcs"));
    }
#endif

    return pccs;
}

//+----------------------------------------------------------------------------
//
//  Member:     CLSMeasurer::GetCcsNumber()
//
//  Synopsis:   Get CCcs for numbering font
//
//  Returns:    ptr to numbering CCcs
//
//  Comment:    The font for the number could change with every instance of a
//              number, because it is subject to the implied formatting of the
//              LI.
//
//-----------------------------------------------------------------------------

CCcs *
CLSMeasurer::GetCcsNumber ( const CCharFormat * pCF, CCharFormat * pCFRet)
{
    CCharFormat cf = *pCF;

    cf._fSubscript = cf._fSuperscript = FALSE;
    cf._bCrcFont = cf.ComputeFontCrc();

    if(pCFRet)
        *pCFRet = cf;

    return fc().GetCcs( _pci->_hdc, _pci, &cf );
}

//+----------------------------------------------------------------------------
//
//  Member:     CLSMeasurer::MeasureSymbol()
//
//  Synopsis:   Measures the special character in WingDings
//
//  Returns:    Nothing
//
//  Note:       that this function returns ascent of the font
//              rather than the entire height. This means that the
//              selected symbol (bullet) character should NOT have a descender.
//
//-----------------------------------------------------------------------------

void CLSMeasurer::MeasureSymbol (TCHAR chSymbol, LONG *pxWidth)
{
    const      CCharFormat *pCF;
    LONG       xWidthTemp;
    CTreeNode *pNode;
    CCcs      *pccs;

    AssertSz(pxWidth, "CLSMeasurer::MeasureSymbol: invalid arg(s)");
    
    pNode = GetMarkup()->SearchBranchForTagInStory(GetPtp()->GetBranch(), ETAG_LI);
    Assert(pNode && pNode->Element()->IsTagAndBlock(ETAG_LI));
    pCF = pNode->GetCharFormat();
    pccs = GetCcsSymbol(chSymbol, pCF);

    xWidthTemp = 0;

    if(pccs)
    {
        if(!pccs->Include(chSymbol, xWidthTemp))
        {
            TraceTag((tagError,
                "CLSMeasurer::MeasureSymbol(): Error filling CCcs"));
        }

        xWidthTemp += pccs->GetBaseCcs()->_xUnderhang + pccs->GetBaseCcs()->_xOverhangAdjust;
    }

    *pxWidth = xWidthTemp;

    if (pccs)
    {
        SHORT yAscent, yDescent;
        CTreePos *ptpStart;
        
        _pFlowLayout->GetContentTreeExtent(&ptpStart, NULL);
        
        pccs->GetBaseCcs()->GetAscentDescent(&yAscent, &yDescent);
        _li._yBulletHeight = yAscent + yDescent;
        pccs->Release();

        // Get the height of normal text in the site.
        // I had originally used the height of the LI,
        // but Netscape doesn't seem to do that. It's
        // possible that they actually have a fixed
        // height for the bullets.
        pCF = ptpStart->Branch()->GetCharFormat();
        pccs = fc().GetCcs(_pci->_hdc, _pci, pCF);
        _pLS->RecalcLineHeight(pccs, &_li);
        pccs->Release();
    }
}

BOOL
CLSMeasurer::MeasureImage(long lImgCookie, SIZE * psizeImg)
{
    CDoc    * pDoc = _pFlowLayout->Doc();
    CImgCtx * pImgCtx = pDoc->GetUrlImgCtx(lImgCookie);

    if (!pImgCtx || !(pImgCtx->GetState(FALSE, psizeImg) & IMGLOAD_COMPLETE))
    {
        psizeImg->cx = psizeImg->cy = 0;
        return FALSE;
    }
    else if (pDoc->IsPrintDoc())
    {
        CDoc * pRootDoc = pDoc->GetRootDoc();
        psizeImg->cx = pRootDoc->_dci.DocPixelsFromWindowX(psizeImg->cx);
        psizeImg->cy = pRootDoc->_dci.DocPixelsFromWindowY(psizeImg->cy);
    }

    return TRUE;
}


//-----------------------------------------------------------------------------
//
// Member:      TestForClear
//
// Synopsis:    Tests if the clear bit is to be set and returns the result
//
//-----------------------------------------------------------------------------

BOOL
CLSMeasurer::TestForClear(const CMarginInfo *pMarginInfo, LONG cp, BOOL fShouldMarginsExist, const CFancyFormat *pFF)
{
    //
    // If margins are not necessary for clear to be turned on, then lets ignore it
    // and just check the flags inside the char format. Bug 47575 shows us that
    // if clear has been applied to BR's and that line contains a aligned image, then 
    // the margins have not been setup yet (since the image will be measure
    // *after* the line is measured. However, if we do not turn on clear left/right
    // here then we will never clear the margins!
    //
    BOOL fClearLeft  =    (!fShouldMarginsExist || pMarginInfo->HasLeftMargin())
                       && pFF->_fClearLeft;
    BOOL fClearRight =    (!fShouldMarginsExist || pMarginInfo->HasRightMargin())
                       && pFF->_fClearRight;
    
    if (cp >= 0)
    {
        Assert(_pLS);
        if (fClearLeft)
            _pLS->_lineFlags.AddLineFlag(cp, FLAG_HAS_CLEARLEFT);
        if (fClearRight)
            _pLS->_lineFlags.AddLineFlag(cp, FLAG_HAS_CLEARRIGHT);
    }
    
    return fClearLeft || fClearRight;
}

#pragma optimize("", on)
