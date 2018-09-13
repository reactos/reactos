/*
 *  @doc    INTERNAL
 *
 *  @module LSOBJ.CXX -- line services object handlers
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      12/18/97     cthrash created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

// NB (cthrash) The implemetation here is largely modeled on lscsk.cpp in Quill

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif

#ifndef X_TCELL_HXX_
#define X_TCELL_HXX_
#include "tcell.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_FMTI_H_
#define X_FMTI_H_
#include <fmti.h>
#endif

#ifndef X_LSFRUN_H_
#define X_LSFRUN_H_
#include <lsfrun.h>
#endif

#ifndef X_OBJDIM_H_
#define X_OBJDIM_H_
#include <objdim.h>
#endif

#ifndef X_LSDNFIN_H_
#define X_LSDNFIN_H_
#include <lsdnfin.h>
#endif

#ifndef X_LSQSUBL_H_
#define X_LSQSUBL_H_
#include <lsqsubl.h>
#endif

#ifndef X_LSDNSET_H_
#define X_LSDNSET_H_
#include <lsdnset.h>
#endif

#ifndef X_PLOCCHNK_H_
#define X_PLOCCHNK_H_
#include <plocchnk.h>
#endif

#ifndef X_LOCCHNK_H_
#define X_LOCCHNK_H_
#include <locchnk.h>
#endif

#ifndef X_POSICHNK_H_
#define X_POSICHNK_H_
#include <posichnk.h>
#endif

#ifndef X_PPOSICHN_H_
#define X_PPOSICHN_H_
#include <pposichn.h>
#endif

#ifndef X_BRKO_H_
#define X_BRKO_H_
#include <brko.h>
#endif

#ifndef X_LSQOUT_H_
#define X_LSQOUT_H_
#include <lsqout.h>
#endif

#ifndef X_LSQIN_H_
#define X_LSQIN_H_
#include <lsqin.h>
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include <flowlyt.hxx>
#endif

#define brkcondUnknown BRKCOND(-1)

ExternTag(tagLSCallBack);

DeclareLSTag( tagTraceILSBreak, "Trace ILS breaking");

MtDefine(CDobjBase, LineServices, "CDobjBase")
MtDefine(CNobrDobj, LineServices, "CNobrDobj")
MtDefine(CEmbeddedDobj, LineServices, "CEmbeddedDobj")
MtDefine(CGlyphDobj, LineServices, "CGlyphDobj")
MtDefine(CILSObjBase, LineServices, "CILSObjBase")
MtDefine(CEmbeddedILSObj, LineServices, "CEmbeddedILSObj")
MtDefine(CNobrILSObj, LineServices, "CNobrILSObj")
MtDefine(CGlyphILSObj, LineServices, "CGlyphILSObj")

// Since lnobj is worthless as far as we're concerned, we just point it back
// to the ilsobj.  lnobj's are instantiated once per object type per line.
typedef struct lnobj
{
    PILSOBJ pilsobj;
} LNOBJ;

LSERR WINAPI
CLineServices::CreateILSObj(  // not static.
    PLSC plsc,          // IN
    PCLSCBK plscbk,     // IN
    DWORD idObj,        // IN
    PILSOBJ* ppilsobj)   // OUT
{
    LSTRACE(CreateILSObj);

    switch( idObj )
    {
    case LSOBJID_EMBEDDED:
        *ppilsobj= (PILSOBJ)(new CEmbeddedILSObj(this, plscbk));
        break;
    case LSOBJID_NOBR:
        *ppilsobj= (PILSOBJ)(new CNobrILSObj(this, plscbk));
        break;
    case LSOBJID_GLYPH:
        *ppilsobj= (PILSOBJ)(new CGlyphILSObj(this, plscbk));
        break;
    default:
        AssertSz(0, "Unknown lsobj_id");
    }

    return *ppilsobj ? lserrNone : lserrOutOfMemory;
}

// This function is a callback from LineServices (the pfnFmt callback)
// Line services calls this to get the formattnig information for
// a non-text object in the layout, i.e. an image or a table or a div.
// We create a dobj and give it back to LS here.
// This is like a dobj constructor.
LSERR WINAPI
CEmbeddedILSObj::Fmt(  // this = plnobj
    PCFMTIN pfmtin,    // IN
    FMTRES* pfmtres)   // OUT
{
    LSTRACE(Fmt);

    BOOL            fOwnLine;
    INT             xMinWidth;
    LONG            cchSite;
    LONG            xWidth, yHeight;
    OBJDIM          objdim;
    stylePosition   bPositionType;
    LSERR           lserr           = lserrNone;
    CLineServices * pLS             = _pLS;
    CFlowLayout   * pFlowLayout     = pLS->_pFlowLayout;
    PLSRUN          plsrun          = PLSRUN(pfmtin->lsfrun.plsrun);
    CLayout       * pLayout         = plsrun->GetLayout(pFlowLayout);
    CElement      * pElementLayout;
    CTreeNode     * pNodeLayout;
    const CCharFormat  *pCF;
    const CFancyFormat *pFF;
    CEmbeddedDobj* pdobj= new CEmbeddedDobj(this, pfmtin->plsdnTop);

    if (!pdobj)
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }

    // pLayout is the guy we're being asked to format here.
    Assert( pLayout && pLayout != pFlowLayout );

    pElementLayout  = pLayout->ElementOwner();
    pNodeLayout     = pElementLayout->GetFirstBranch();
    pCF             = pNodeLayout->GetCharFormat();
    pFF             = pNodeLayout->GetFancyFormat();
    bPositionType   = pNodeLayout->GetCascadedposition();

    // for overlapping layouts curtail the range of characters measured
    cchSite = pLS->GetNestedElementCch(pElementLayout);

    ZeroMemory( &objdim, sizeof(OBJDIM) );

    // Let's see if this an 'ownline' thingy.  Note that even if the element
    // is not by default and 'ownline' element, we may have morphed it into
    // one -- then too it has to be a block element. If it is not one (like
    // a span, then it will not live on its own line).  Check here.

    fOwnLine = pLS->IsOwnLineSite(plsrun);

    Assert(pElementLayout->IsInlinedElement());

    // Certain sites that only Microsoft supports can break with any
    // characters, so we hack that in right here.
    // BUGBUG (cthrash) This is goofy.  We should have a better way to
    // determine this than checking tag types.

    pdobj->_fIsBreakingSite =    pElementLayout->Tag() == ETAG_OBJECT
                              || pElementLayout->Tag() == ETAG_IFRAME
                              || pElementLayout->Tag() == ETAG_MARQUEE

    // This is really unfortunate -- if a site is percent sized then it becomes a breaking
    // site inside table cells. This is primarily for IE4x compat. See IE bug 42336 (SujalP)

                              || pFF->_cuvWidth.GetUnitType() == CUnitValue::UNIT_PERCENT

    // One last thing - if we have a morphed non-ownline element inside
    // a table, it's considered a breaking site.

                              || (!fOwnLine && !pElementLayout->_fSite);

    // If it's on its own line, and not first on line, FetchRun should have
    // terminated the line before we got here.
    // Assert( !( fOwnLine && !pfmtin->lsfgi.fFirstOnLine) );

    pFlowLayout->GetSiteWidth( pLayout, pLS->_pci,
                               pLS->_lsMode == CLineServices::LSMODE_MEASURER,
                               pLS->_xWrappingWidth,
                               &xWidth, &yHeight, &xMinWidth);

    // v-Dimension computed in VerticalAlignObjects
    // BUGBUG (cthrash) We have rounding errors in LS; don't pass zero

    objdim.heightsRef.dvAscent = 1;
    objdim.heightsRef.dvDescent = 0;
    objdim.heightsRef.dvMultiLineHeight = 1;
    if (_pLS->_fMinMaxPass)
        objdim.heightsPres = objdim.heightsRef;

    if(pLS->_fIsRuby && !pLS->_fIsRubyText)
    {
        pLS->_yMaxHeightForRubyBase = max(pLS->_yMaxHeightForRubyBase, yHeight);
    }

    // We need to store two widths in the dobj: The width corresponding to
    // the wrapping width (urColumnMax) and the minimum width.  LsGetMinDur,
    // however, does not recognize two widths for ILS objects.  We therefore
    // cache the difference, and account for these in an enumeration callback
    // after the LsGetMinDur pass.

    if (!pLS->_fMinMaxPass)
    {
        pdobj->_dvMinMaxDelta = 0;
        objdim.dur = xWidth;
    }
    else
    {
        pdobj->_dvMinMaxDelta = xWidth - xMinWidth;
        objdim.dur = xMinWidth;
    }

    if (pCF->HasCharGrid(TRUE))
    {
        long lCharGridSize = pLS->GetCharGridSize();
        pdobj->_dvMinMaxDelta = pLS->GetClosestGridMultiple(lCharGridSize, pdobj->_dvMinMaxDelta);
        objdim.dur = pLS->GetClosestGridMultiple(lCharGridSize, objdim.dur);
    }

    if (fOwnLine)
    {
        // If we are inside a something like say a PRE then too we do
        // not terminate, because if there was a \r right after the ownline site
        // then we will allow that \r to break the line. We do not check if the
        // subsequent char is a \r because there might be goop(comments, hidden
        // stuff etc etc) between this site and the \r. Hence here we just march
        // forward and if we run into text or layout later on we will terminate
        // the line then. This way we will eat up the goop if any in between.
        if (!pLS->_fScanForCR)
        {
            COneRun *porOut;
            COneRun *por = pLS->_listFree.GetFreeOneRun(plsrun);

            if (!por)
            {
                lserr = lserrOutOfMemory;
                goto Cleanup;
            }

            Assert(plsrun->IsNormalRun());
            Assert(plsrun->_lscch == plsrun->_lscchOriginal);
            por->_lscpBase += plsrun->_lscch;

            // If this object has to be on its own line, then it clearly
            // ends the current line.
            lserr = pLS->TerminateLine(por, CLineServices::TL_ADDEOS, &porOut);
            if (lserr != lserrNone)
                goto Cleanup;

            // Free the one run
            pLS->_listFree.SpliceIn(por);
        }
        else
        {
            // Flip this bit so that we will setup pfmtres properly later on
            fOwnLine = FALSE;
        }

        // If we have an 'ownline' site, by definition this is a breaking
        // site, meaning we can (or more precisely should) break on either
        // side of the site.
        pdobj->_fIsBreakingSite = TRUE;

        //
        // NOTE(SujalP): Bug 65906.
        // We originally used to set ourselves up to collect after space only
        // for morphed elements. As it turns out, we want to collect after space
        // from margins of _all_ ownline sites (including morphed elements),
        // because margins for ownline-sites are not accounted for in
        // VerticalAlignObjects (see CLayout::GetMarginInfo for more details --
        // it returns 0 for ownline sites).
        //
        pLS->_pNodeForAfterSpace = pNodeLayout;

        //
        // All ownline sites have their x position to be 0
        //
        if (pElementLayout->HasFlag(TAGDESC_OWNLINE))
        {
            pLayout->SetXProposed(0);
        }
    }

    if (fOwnLine)
    {
        *pfmtres = fmtrCompletedRun;
    }
    else
    {
        const long urWrappingWidth = max(pfmtin->lsfgi.urColumnMax, pLS->_xWrappingWidth);

        *pfmtres = (pfmtin->lsfgi.fFirstOnLine ||
                    pfmtin->lsfgi.urPen + objdim.dur <= urWrappingWidth)
                   ? fmtrCompletedRun
                   : fmtrExceededMargin;
    }

    // This is an accessor to the dnode, telling LS to set the dnode pointing
    // to our dobj.
    lserr = LsdnFinishRegular( pLS->_plsc, cchSite, pfmtin->lsfrun.plsrun,
                               pfmtin->lsfrun.plschp, (struct dobj*)pdobj, &objdim);

Cleanup:

    return lserr;
}



// This is like a dobj constructor.
LSERR WINAPI
CNobrILSObj::Fmt(  // this = plnobj
    PCFMTIN pfmtin,     // IN
    FMTRES* pfmtres)    // OUT
{
    LSTRACE(Fmt);

    LSERR lserr = lserrNone;
    CLineServices * pLS = _pLS;
    FMTRES subfmtres;
    OBJDIM objdim;
    LSTFLOW lstflowDontcare;
    COneRun* por= (COneRun*) pfmtin->lsfrun.plsrun;
    //LONG lscpStart= pLS->LSCPFromCP(por->Cp());
    LONG lscpStart= por->_lscpBase + 1; // +1 to jump over the synth character
    LSCP lscpLast;
    LSCP lscpUsed;
    BOOL fSuccess;

    CNobrDobj* pdobj = new CNobrDobj(this, pfmtin->plsdnTop);
    if (!pdobj)
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }
    pdobj->_plssubline = NULL;

    AssertSz( ! pLS->_fNoBreakForMeasurer, "Can't nest NOBR's");

    pLS->_fNoBreakForMeasurer = TRUE;

    pdobj->_brkcondBefore = brkcondUnknown;
    pdobj->_brkcondAfter = brkcondUnknown;

    // Tell LS to prepare a subline
    lserr= LsCreateSubline(pLS->_plsc, lscpStart, MAX_MEASURE_WIDTH, pfmtin->lsfgi.lstflow, FALSE);
    if (lserr != lserrNone)
    {
        goto Cleanup;
    }

    // Tell LS to populate it.
    do
    {
        PLSDNODE plsdnStart;
        PLSDNODE plsdnEnd;

        // Format as much as we can - note we move max to maximum positive value.
        lserr = LsFetchAppendToCurrentSubline(pLS->_plsc,
                                              0,
                                              &s_lsescEndNOBR[0],
                                              NBREAKCHARS,
                                              &fSuccess,
                                              &subfmtres,
                                              &lscpLast,
                                              &plsdnStart,
                                              &plsdnEnd);

        if (lserr != lserrNone)
        {
            goto Cleanup;
        }
        Assert(subfmtres != fmtrExceededMargin); // we are passing max width!

    } while (   (subfmtres != fmtrCompletedRun)  //Loop until one of the conditions is met.
             && !fSuccess
            );

    *pfmtres = subfmtres;

    lserr = LsFinishCurrentSubline(pLS->_plsc, &pdobj->_plssubline);
    if (lserr != lserrNone)
        goto Cleanup;

    // get Object dimensions.
    lserr = LssbGetObjDimSubline(pdobj->_plssubline, &lstflowDontcare, &objdim);
    if (lserr != lserrNone)
        goto Cleanup;

    // We add 2 to include the two synthetic characters at the beginning and end.
    // That's just how it goes, okay.
    lscpUsed = lscpLast - lscpStart + 2;

    // Tell Line Services we have a special object which (a) is not to be broken,
    // and (b) have different min and max widths due to trailing spaces.

    lserr = LsdnSubmitSublines( pLS->_plsc,
                                pfmtin->plsdnTop,
                                1,
                                &pdobj->_plssubline,
                                TRUE, //pLS->_fExpansionOrCompression,  // fUseForJustification
                                TRUE, //pLS->_fExpansionOrCompression,  // fUseForCompression
                                FALSE,  // fUseForDisplay
                                FALSE,  // fUseForDecimalTab
                                TRUE ); // fUseForTrailingArea  

    // Give the data back to LS.
    lserr = LsdnFinishRegular( pLS->_plsc, lscpUsed,
                               pfmtin->lsfrun.plsrun,
                               pfmtin->lsfrun.plschp, (struct dobj *) pdobj, &objdim);
    
    
    if (lserr != lserrNone)
        goto Cleanup;

    pLS->_fMayHaveANOBR = TRUE;

Cleanup:
    if (lserr != lserrNone && pdobj)
    {
        if (pdobj->_plssubline)
            LsDestroySubline(pdobj->_plssubline);  // Destroy the subline.
        delete pdobj;
    }

    pLS->_fNoBreakForMeasurer = FALSE;

    return lserr;
}



LSERR WINAPI
CGlyphILSObj::Fmt(  // this = plnobj
    PCFMTIN pfmtin,    // IN
    FMTRES* pfmtres)   // OUT
{
    LSTRACE(Fmt);
    LSERR           lserr           = lserrNone;
    OBJDIM          objdim;
    PLSRUN          por            = PLSRUN(pfmtin->lsfrun.plsrun);
    CLineServices * pLS             = _pLS;

    CGlyphDobj* pdobj= new CGlyphDobj(this, pfmtin->plsdnTop);
    if (!pdobj)
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }

    // For breaking we need to know two things:
    //  (a) is this a glyph represnting a no-scope?
    //  (b) if not, is this representing a begin or end tag?

    pdobj->_fBegin = por->_ptp->IsBeginNode();
    pdobj->_fNoScope = FALSE;

    pdobj->_RenderInfo.pImageContext = NULL;
    Assert(pLS->_fIsEditable);
    if (!por->_ptp->ShowTreePos(&pdobj->_RenderInfo))
    {
        AssertSz(0, "Inconsistent info in one run");
        lserr = lserrInvalidRun;
        goto Cleanup;
    }
    Assert(pdobj->_RenderInfo.pImageContext);

    objdim.heightsRef.dvAscent = pdobj->_RenderInfo.height;
    objdim.heightsRef.dvDescent = 0;
    objdim.heightsRef.dvMultiLineHeight = pdobj->_RenderInfo.height;
    objdim.heightsPres = objdim.heightsRef;
    objdim.dur = pdobj->_RenderInfo.width;

    // This is an accessor to the dnode, telling LS to set the dnode pointing
    // to our dobj.
    lserr = LsdnFinishRegular( pLS->_plsc, por->_lscch, pfmtin->lsfrun.plsrun,
                               pfmtin->lsfrun.plschp, (struct dobj*)pdobj, &objdim);

    *pfmtres = fmtrCompletedRun;

Cleanup:
    return lserr;
}



LSERR WINAPI
CEmbeddedILSObj::FmtResume(  // this = plnobj
    const BREAKREC* rgBreakRecord,      // IN
    DWORD nBreakRecord,                 // IN
    PCFMTIN pfmtin,                     // IN
    FMTRES* pfmtres)                    // OUT
{
    LSTRACE(FmtResume);
    LSNOTIMPL(FmtResume);

    // I believe that this should never get called for most embedded objects.
    // The only possible exception that comes to mind are marquees, and I do
    // not believe that they can wrap around lines.

    return lserrNone;
}

LSERR WINAPI /* static */
CEmbeddedDobj::GetModWidthPrecedingChar(
    PDOBJ pdobj,            // IN
    PLSRUN plsrun,          // IN
    PLSRUN plsrunText,      // IN
    PCHEIGHTS heightsRef,   // IN
    WCHAR wch,              // IN
    MWCLS mwcls,            // IN
    long* pdurChange)       // OUT
{
    LSTRACE(EmbeddedGetModWidthPrecedingChar);

    *pdurChange = 0;

    return lserrNone;
}

LSERR WINAPI /* static */
CNobrDobj::GetModWidthPrecedingChar(
    PDOBJ pdobj,            // IN
    PLSRUN plsrun,          // IN
    PLSRUN plsrunText,      // IN
    PCHEIGHTS heightsRef,   // IN
    WCHAR wch,              // IN
    MWCLS mwcls,            // IN
    long* pdurChange)       // OUT
{
    LSTRACE(NobrGetModWidthPrecedingChar);

    // BUGBUG (cthrash) This is wrong.  We need to determine the MWCLS of the
    // first char of the NOBR and determine how much we should adjust the width.

    *pdurChange = 0;

    return lserrNone;
}

LSERR WINAPI /* static */
CGlyphDobj::GetModWidthPrecedingChar(
    PDOBJ pdobj,            // IN
    PLSRUN plsrun,          // IN
    PLSRUN plsrunText,      // IN
    PCHEIGHTS heightsRef,   // IN
    WCHAR wch,              // IN
    MWCLS mwcls,            // IN
    long* pdurChange)       // OUT
{
    LSTRACE(GlyphGetModWidthPrecedingChar);

    *pdurChange = 0;

    return lserrNone;
}

LSERR WINAPI /* static */
CEmbeddedDobj::GetModWidthFollowingChar(
    PDOBJ pdobj,            // IN
    PLSRUN plsrun,          // IN
    PLSRUN plsrunText,      // IN
    PCHEIGHTS heightsRef,   // IN
    WCHAR wch,              // IN
    MWCLS mwcls,            // IN
    long* pdurChange)       // OUT
{
    LSTRACE(EmbeddedGetModWidthFollowingChar);

    *pdurChange = 0;

    return lserrNone;
}

LSERR WINAPI /* static */
CNobrDobj::GetModWidthFollowingChar(
    PDOBJ pdobj,            // IN
    PLSRUN plsrun,          // IN
    PLSRUN plsrunText,      // IN
    PCHEIGHTS heightsRef,   // IN
    WCHAR wch,              // IN
    MWCLS mwcls,            // IN
    long* pdurChange)       // OUT
{
    LSTRACE(NobrGetModWidthFollowingChar);

    // BUGBUG (cthrash) This is wrong.  We need to determine the MWCLS of the
    // last char of the NOBR and determine how much we should adjust the width.
    
    *pdurChange = 0; 

    return lserrNone;
}

LSERR WINAPI /* static */
CGlyphDobj::GetModWidthFollowingChar(
    PDOBJ pdobj,            // IN
    PLSRUN plsrun,          // IN
    PLSRUN plsrunText,      // IN
    PCHEIGHTS heightsRef,   // IN
    WCHAR wch,              // IN
    MWCLS mwcls,            // IN
    long* pdurChange)       // OUT
{
    LSTRACE(GlyphGetModWidthFollowingChar);

    *pdurChange = 0; // stick to the text

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   TruncateChunk (static member, LS callback)
//
//  Synopsis:   From LS docs: Purpose    To obtain the exact position within
//              a chunk where the chunk crosses the right margin.  A chunk is
//              a group of contiguous LS objects which are of the same type.
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::TruncateChunk(
    PCLOCCHNK plocchnk,     // IN
    PPOSICHNK pposichnk)    // OUT
{
    LSTRACE(TruncateChunk);
    LSERR lserr = lserrNone;
    long idobj;

    if (plocchnk->clschnk == 1)
    {
        idobj = 0;
    }
    else
    {
        long urColumnMax;
        long urTotal;
        OBJDIM objdim;

        urColumnMax = PDOBJ(plocchnk->plschnk[0].pdobj)->GetPLS()->_xWrappingWidth;
        urTotal = plocchnk->lsfgi.urPen;

        Assert(urTotal <= urColumnMax);

        for (idobj = 0; idobj < (long)plocchnk->clschnk; idobj++)
        {
            PDOBJ pdobj = (PDOBJ) plocchnk->plschnk[idobj].pdobj;
            lserr = pdobj->QueryObjDim(&objdim);
            if (lserr != lserrNone)
                goto Cleanup;

            urTotal += objdim.dur;
            if (urTotal > urColumnMax)
                break;
        }

        Assert(urTotal > urColumnMax);
        Assert(idobj < (long)plocchnk->clschnk);
    }

    // Tell it which chunk didn't fit.

    pposichnk->ichnk = idobj;

    // Tell it which cp inside this chunk the break occurs at.
    // In our case, it's always an all-or-nothing proposition.  So include the whole dobj

    pposichnk->dcp = plocchnk->plschnk[idobj].dcp;

    TraceTag((tagTraceILSBreak,
              "Truncate(cchnk=%d cpFirst=%d) -> ichnk=%d",
              plocchnk->clschnk,
              plocchnk->plschnk->cpFirst,
              idobj
             ));

Cleanup:

    return lserr;
}

inline BOOL
CanBreakPair( BRKCOND brkcondBefore, BRKCOND brkcondAfter )
{
    // brkcondPlease = 0
    // brkcondCan    = 1
    // brkcondNever  = 2
    //
    //
    //         | Please |   Can  |  Never
    // --------+--------+--------+---------
    //  Please |  TRUE  |  TRUE  |  FALSE
    // --------+--------+--------+---------
    //  Can    |  TRUE  |  FALSE |  FALSE
    // --------+--------+--------+---------
    //  Never  |  FALSE |  FALSE |  FALSE

    return (int(brkcondBefore) + int(brkcondAfter)) < 2;
}

#if DBG==1
void
DumpBrkopt(
    char *szType,
    BOOL fMinMaxPass,
    PCLOCCHNK plocchnk,
    PCPOSICHNK pposichnk,
    BRKCOND brkcond,
    PBRKOUT pbrkout)
{
    static const char * achBrkCondStr[3] = { "Please", "Can", "Never" };

    // Make sure we don't try to break before the first object on the line.  This is bad.

    AssertSz(   !pbrkout->fSuccessful
             || !plocchnk->lsfgi.fFirstOnLine
             || pbrkout->posichnk.ichnk > 0
             || pbrkout->posichnk.dcp > 0,
             "Bad breaking position.  Cannot break at BOL.");

    TraceTag((tagTraceILSBreak,
              "%s(brkcond=%s cchnk=%d ichnk=%d) minmax=%s urPen=%d "
              "-> %s(%s) on %d(%s)",
              szType,
              achBrkCondStr[brkcond],
              plocchnk->clschnk,
              pposichnk->ichnk,
              fMinMaxPass ? "yes" : "no",
              plocchnk->lsfgi.urPen,
              pbrkout->fSuccessful ? "true" : "false",
              achBrkCondStr[pbrkout->brkcond],
              pbrkout->posichnk.ichnk,
              pbrkout->posichnk.dcp > 0 ? "after" : "before"));
}
#else
#define DumpBrkopt(a,b,c,d,e,f)
#endif

LSERR WINAPI /* static */
CEmbeddedDobj::FindPrevBreakChunk(
    PCLOCCHNK plocchnk,     // IN
    PCPOSICHNK pposichnk,   // IN
    BRKCOND brkcond,        // IN
    PBRKOUT pbrkout)        // OUT
{
    BOOL            fSuccessful;
    BOOL            fBreakAfter = pposichnk->ichnk == ichnkOutside;
    long            ichnk = fBreakAfter
                            ? long(plocchnk->clschnk) - 1L
                            : long(pposichnk->ichnk);
    CEmbeddedDobj * pdobj = (CEmbeddedDobj *)(plocchnk->plschnk[ichnk].pdobj);
    CLineServices * pLS = pdobj->GetPLS();
    LSERR           lserr;

    WHEN_DBG( BRKCOND brkcondIn = brkcond );

    if (!pdobj->_fIsBreakingSite)
    {
        if (fBreakAfter)
        {
            //
            // If fBreakAfter is TRUE, we have the following scenario (| represents wrapping width):
            //
            // AAA<IMG><IMG>B|BB
            //

            if (!pLS->_fIsTD || !pLS->_fMinMaxPass || (pLS->_xTDWidth < MAX_MEASURE_WIDTH))
            {
                // If we're not in a TD, we can break after the dobj, the subsequent dobj permitting.
                // If the subsequent dobj is not willing, we can break between the dobjs in this chunk.

                if (brkcond == brkcondPlease)
                {
                    fSuccessful = TRUE;
                    brkcond = brkcondPlease;
                }
                else
                {
                    fBreakAfter = FALSE;
                    fSuccessful = ichnk > 0 || !plocchnk->lsfgi.fFirstOnLine;
                    brkcond = fSuccessful ? brkcondPlease : brkcondNever;
                }
            }
            else
            {
                fSuccessful = FALSE;
                brkcond = brkcondCan;
            }
        }
        else if (ichnk == 0 && plocchnk->lsfgi.fFirstOnLine)
        {
            // We can never break before the first dobj on the line.

            fSuccessful = FALSE;
            brkcond = brkcondNever;
        }
        else
        {
            //
            // fBreakAfter is FALSE, meaning we're splitting up our dobj chunks
            //
            // AAA<IMG><IM|G>BBB
            //

            if (pLS->_fIsTD)
            {
                if (pLS->_xTDWidth == MAX_MEASURE_WIDTH)
                {
                    // TD has no stated width.
                    
                    if (ichnk == 0)
                    {
                        // To do this correctly in the MinMaxPass, we would need to know the
                        // brkcond of the character preceeding our chunk.  The incoming
                        // brkcond, however, is meaningless unless ichnk==ichnkOutside.
                        
                        fSuccessful = FALSE;//!pLS->_fMinMaxPass;
                        brkcond = brkcondCan;
                    }
                    else
                    {
                        // Our TD has no width, so we can break only if our wrapping width
                        // has been exceeded.  Note we will be called during LsGetMinDurBreaks
                        // even if our wrapping width isn't exceeded, because we don't know
                        // yet what that width is.

                        fSuccessful = !pLS->_fMinMaxPass || ((CEmbeddedDobj *)(plocchnk->plschnk[ichnk-1].pdobj))->_fIsBreakingSite;
                        brkcond = fSuccessful ? brkcondPlease : brkcondCan;
                    }
                }
                else
                {
                    // TD has a stated width.

                    if (pLS->_fMinMaxPass)
                    {
                        if (ichnk == 0)
                        {
                            fSuccessful = plocchnk->lsfgi.urColumnMax > pLS->_xTDWidth;
                            brkcond = fSuccessful ? brkcondPlease : brkcondCan;
                        }
                        else
                        {
                            // When we're called from LsGetMinDurBreaks, urColumnMax is the
                            // proposed wrapping width.  If this is greater than the TDs
                            // stated width, we can break.  Otherwise, we can't.

                            fSuccessful = plocchnk->lsfgi.urColumnMax > pLS->_xTDWidth;
                            brkcond = fSuccessful ? brkcondPlease : brkcondNever;
                        }
                    }
                    else
                    {
                        // If we're not in a min-max pass, we reach this point only if this
                        // particular dobj has caused an overflow.  We should break before it.

                        fSuccessful = TRUE;
                        brkcond = brkcondPlease;
                    }
                }
            }
            else
            {
                // We're not in a TD, meaning we will break between the dobjs in our chunks.

                if (ichnk)
                {
                    fSuccessful = TRUE;
                    brkcond = brkcondPlease;
                }
                else
                {
                    fSuccessful = TRUE;
                    brkcond = fSuccessful ? brkcondCan : brkcondNever;
                }
            }
        }
    }
    else
    {
        // We have a dobj around which we always break, such as a MARQUEE.
        // We will break as long as we're not at the beginning of the line.

        if (fBreakAfter)
        {
            fSuccessful = brkcond != brkcondNever;
            brkcond = brkcondPlease;
        }
        else
        {
            fSuccessful = ichnk > 0;
            brkcond = plocchnk->lsfgi.fFirstOnLine ? brkcondNever : brkcondPlease;
        }
    }

    pbrkout->fSuccessful = fSuccessful;
    pbrkout->brkcond = brkcond;
    pbrkout->posichnk.ichnk = ichnk;
    pbrkout->posichnk.dcp = fBreakAfter ? plocchnk->plschnk[ichnk].dcp : 0;

    lserr = fSuccessful ? pdobj->QueryObjDim(&pbrkout->objdim) : lserrNone;

    DumpBrkopt( "Prev(E)", pLS->_fMinMaxPass, plocchnk, pposichnk, brkcondIn, pbrkout );

    return lserr;
}

LSERR WINAPI /* static */
CEmbeddedDobj::FindNextBreakChunk(
    PCLOCCHNK plocchnk,     // IN
    PCPOSICHNK pposichnk,   // IN
    BRKCOND brkcond,        // IN
    PBRKOUT pbrkout)        // OUT
{
    BOOL    fBreakBefore = (pposichnk->ichnk == ichnkOutside);
    long    ichnk = fBreakBefore
                    ? 0
                    : pposichnk->ichnk;
    CEmbeddedDobj *pdobj = (CEmbeddedDobj *)(plocchnk->plschnk[ichnk].pdobj);
    CLineServices * pLS = pdobj->GetPLS();
    LSERR   lserr;
    BOOL    fSuccessful;

    WHEN_DBG( BRKCOND brkcondIn = brkcond );

    Assert(!pLS->_fMinMaxPass);
    
    if (!pdobj->_fIsBreakingSite)
    {
        if (fBreakBefore)
        {
            fBreakBefore = !plocchnk->lsfgi.fFirstOnLine;
            fSuccessful = fBreakBefore || (plocchnk->clschnk > 1);
            brkcond = fSuccessful ? brkcondPlease : brkcondCan;
        }
        else
        {
            if (ichnk == (long(plocchnk->clschnk) - 1))
            {
                // If this is the last dobj of the chunk, we need to ask the next
                // object if it's okay to break.

                fSuccessful = FALSE;
                brkcond = brkcondCan;
            }
            else
            {
                if (pLS->_fIsTD && pLS->_xTDWidth == MAX_MEASURE_WIDTH)
                {
                    // We are in TD with no width, so we cannot break between our dobjs.

                    fSuccessful = ((CEmbeddedDobj *)(plocchnk->plschnk[ichnk+1].pdobj))->_fIsBreakingSite;;
                    brkcond = fSuccessful ? brkcondPlease : brkcondCan;
                }
                else
                {
                    // We are not in a TD, or we're in a TD with a width.
                    // We've exceeded the wrapping width, so we can break.

                    fSuccessful = TRUE;
                    brkcond = brkcondPlease;
                }
            }
        }
    }
    else
    {
        fSuccessful = TRUE;
        brkcond = brkcondPlease;
    }

    pbrkout->fSuccessful = fSuccessful;
    pbrkout->brkcond = brkcond;
    pbrkout->posichnk.ichnk = ichnk;
    pbrkout->posichnk.dcp = fBreakBefore ? 0 : plocchnk->plschnk[ichnk].dcp;

    lserr = pdobj->QueryObjDim(&pbrkout->objdim);

    DumpBrkopt( "Next(E)", pLS->_fMinMaxPass, plocchnk, pposichnk, brkcondIn, pbrkout );

    return lserr;
}

LSERR WINAPI /* static */
CNobrDobj::FindPrevBreakChunk(
    PCLOCCHNK plocchnk,     // IN
    PCPOSICHNK pposichnk,   // IN
    BRKCOND brkcond,        // IN
    PBRKOUT pbrkout)        // OUT
{
    BOOL    fBreakAfter = pposichnk->ichnk == ichnkOutside;
    long    ichnk = fBreakAfter
                    ? long(plocchnk->clschnk) - 1L
                    : long(pposichnk->ichnk);
    LSERR   lserr;
    BOOL    fSuccessful;

    WHEN_DBG( CLineServices * pLS = ((CNobrDobj *)plocchnk->plschnk[0].pdobj)->GetPLS() );
    WHEN_DBG( BRKCOND brkcondIn = brkcond );

    if (fBreakAfter)
    {
        // break after?

        CNobrDobj * pdobj = (CNobrDobj *)plocchnk->plschnk[ichnk].pdobj;
        BRKCOND brkcondNext = brkcond;
        COneRun * por = (COneRun *)plocchnk->plschnk[ichnk].plsrun;

        // compute the brkcond of the last char of the last chunk.

        brkcond = pdobj->GetBrkcondAfter(por, plocchnk->plschnk[ichnk].dcp);

        // can we break immediately after the last NOBR?

        fSuccessful = CanBreakPair(brkcond, brkcondNext);

        if (!fSuccessful)
        {
            fBreakAfter = FALSE;

            if (plocchnk->clschnk > 1)
            {
                // if we have more than one NOBR objects, we can always break between them.

                Assert(!por->_fIsArtificiallyStartedNOBR);

                fSuccessful = TRUE;
                brkcond = brkcondPlease;
            }
            else
            {
                // if we only have one NOBR object, we need to see if we can break before it.

                if (plocchnk->lsfgi.fFirstOnLine)
                {
                    brkcond = brkcondNever;
                    fSuccessful = FALSE;
                }
                else
                {
                    brkcond = pdobj->GetBrkcondBefore(por);
                    fSuccessful = brkcond != brkcondNever;
                }
            }
        }
    }
    else
    {
        if (ichnk)
        {
            // break in between?

            fSuccessful = TRUE;
            brkcond = brkcondPlease; // IE4 compat: for <NOBR>X</NOBR><NOBR>Y</NOBR> we are always willing to break between X and Y.
        }
        else if (plocchnk->lsfgi.fFirstOnLine)
        {
            fSuccessful = FALSE;
        }
        else
        {
            // break before?

            CNobrDobj * pdobj = (CNobrDobj *)plocchnk->plschnk[0].pdobj;

            brkcond = pdobj->GetBrkcondBefore((COneRun *)plocchnk->plschnk[0].plsrun);
            fSuccessful = FALSE;//brkcond != brkcondNever;
        }
    }

    pbrkout->fSuccessful = fSuccessful;
    pbrkout->brkcond = brkcond;
    pbrkout->posichnk.ichnk = ichnk;
    pbrkout->posichnk.dcp = fBreakAfter ? plocchnk->plschnk[ichnk].dcp : 0;

    if (fSuccessful)
    {
        lserr = ((CNobrDobj *)plocchnk->plschnk[ichnk].pdobj)->QueryObjDim(&pbrkout->objdim);
    }
    else
    {
        lserr = lserrNone;
    }

    DumpBrkopt( "Prev(N)", pLS->_fMinMaxPass, plocchnk, pposichnk, brkcondIn, pbrkout );

    return lserr;
}

LSERR WINAPI /* static */
CNobrDobj::FindNextBreakChunk(
    PCLOCCHNK plocchnk,     // IN
    PCPOSICHNK pposichnk,   // IN
    BRKCOND brkcond,        // IN
    PBRKOUT pbrkout)        // OUT
{
    BOOL    fBreakBefore = (pposichnk->ichnk == ichnkOutside);
    long    ichnk = fBreakBefore
                    ? 0
                    : pposichnk->ichnk;
    LSERR   lserr;
    BOOL    fSuccessful;

    WHEN_DBG( CLineServices * pLS = ((CNobrDobj *)plocchnk->plschnk[0].pdobj)->GetPLS() );
    WHEN_DBG( BRKCOND brkcondIn = brkcond );

    if (fBreakBefore)
    {
        // Break before?

        CNobrDobj * pdobj = (CNobrDobj *)plocchnk->plschnk[0].pdobj;
        BRKCOND brkcondBefore = brkcond;

        // Determine if we have an appropriate breaking boundary

        brkcond = pdobj->GetBrkcondBefore((COneRun *)plocchnk->plschnk[0].plsrun);
        fSuccessful = CanBreakPair(brkcondBefore, brkcond);

        if (!fSuccessful)
        {
            fBreakBefore = FALSE;

            if (plocchnk->clschnk > 1)
            {
                // if we have more than one NOBR objects, we can always break between them.

                fSuccessful = TRUE;
                brkcond = brkcondPlease;
            }
            else
            {
                // if we only have one NOBR object, we need to see if we can break after it.

                brkcond = pdobj->GetBrkcondAfter((COneRun *)plocchnk->plschnk[0].plsrun, plocchnk->plschnk[0].dcp );
                fSuccessful = FALSE;//brkcond != brkcondNever;
            }
        }
    }
    else
    {
        if (ichnk < (long(plocchnk->clschnk) - 1))
        {
            // if we're not the last NOBR object, we can always break after it.

            fSuccessful = TRUE;
            brkcond = brkcondPlease;
        }
        else
        {
            // break after?

            CNobrDobj * pdobj = (CNobrDobj *)plocchnk->plschnk[ichnk].pdobj;

            brkcond = pdobj->GetBrkcondAfter((COneRun *)plocchnk->plschnk[ichnk].plsrun, plocchnk->plschnk[ichnk].dcp );
            fSuccessful = FALSE;
        }
    }

    pbrkout->fSuccessful = fSuccessful;
    pbrkout->brkcond = brkcond;
    pbrkout->posichnk.ichnk = ichnk;
    pbrkout->posichnk.dcp = fBreakBefore ? 0 : plocchnk->plschnk[ichnk].dcp;

    lserr = ((CNobrDobj *)plocchnk->plschnk[ichnk].pdobj)->QueryObjDim(&pbrkout->objdim);

    DumpBrkopt( "Next(N)", pLS->_fMinMaxPass, plocchnk, pposichnk, brkcondIn, pbrkout );

    return lserr;
}

LSERR WINAPI /* static */
CGlyphDobj::FindPrevBreakChunk(
    PCLOCCHNK plocchnk,     // IN
    PCPOSICHNK pposichnk,   // IN
    BRKCOND brkcond,        // IN
    PBRKOUT pbrkout)        // OUT
{
    BOOL         fSuccessful;
    BOOL         fBreakAfter = (pposichnk->ichnk == ichnkOutside);
    long         ichnk = fBreakAfter
                         ? long(plocchnk->clschnk) - 1L
                         : long(pposichnk->ichnk);
    CGlyphDobj * pdobj = (CGlyphDobj *)(plocchnk->plschnk[ichnk].pdobj);
    LSERR        lserr;

    WHEN_DBG( CLineServices * pLS = pdobj->GetPLS() );

    if (fBreakAfter)
    {
        //                         Second DOBJ
        //
        //                       TEXT   NOSCOPE   BEGIN    END
        //            --------+-------+--------+--------+--------
        //   First    TEXT    |  n/a  | before | before |  no
        //    DOBJ    NOSCOPE | after | after  | after  | after
        //            BEGIN   |   no  | after  |  no    |  no
        //            END     | after | after  | after  |  no
        //

        BOOL fNextWasEnd = FALSE;

        do
        {
            pdobj = (CGlyphDobj *)(plocchnk->plschnk[ichnk].pdobj);

            // break after NOSCOPE, or END unless followed by END

            fSuccessful =    pdobj->_fNoScope
                          || (!pdobj->_fBegin && !fNextWasEnd);

            if (fSuccessful)
                break;

            fNextWasEnd = !pdobj->_fBegin;

        } while (ichnk--);

        if (!fSuccessful)
        {
            // break before text and BEGIN

            ichnk = 0;
            fSuccessful = !plocchnk->lsfgi.fFirstOnLine && pdobj->_fBegin;
            fBreakAfter = FALSE;
        }
    }
    else
    {
        if (pdobj->_fNoScope)
        {
            // break before NOSCOPE

            fSuccessful = TRUE;
        }
        else
        {
            // break before BEGIN preceeded by TEXT/END/NOSCOPE

            if (ichnk)
            {
                if (pdobj->_fBegin)
                {
                    CGlyphDobj * pdobjPrev = (CGlyphDobj *)(plocchnk->plschnk[ichnk-1].pdobj);

                    fSuccessful = pdobjPrev->_fNoScope || !pdobjPrev->_fBegin;
                }
                else
                {
                    fSuccessful = FALSE;
                }
            }
            else
            {
                fSuccessful = !plocchnk->lsfgi.fFirstOnLine && pdobj->_fBegin;
            }
        }
    }

    pbrkout->fSuccessful = fSuccessful;
    pbrkout->brkcond = fSuccessful ? brkcondPlease : brkcondNever;
    pbrkout->posichnk.ichnk = ichnk;
    pbrkout->posichnk.dcp = fBreakAfter ? plocchnk->plschnk[ichnk].dcp : 0;

    lserr = pdobj->QueryObjDim(&pbrkout->objdim);

    DumpBrkopt( "Prev(G)", pLS->_fMinMaxPass, plocchnk, pposichnk, brkcond, pbrkout );

    return lserr;
}

LSERR WINAPI /* static */
CGlyphDobj::FindNextBreakChunk(
    PCLOCCHNK plocchnk,     // IN
    PCPOSICHNK pposichnk,   // IN
    BRKCOND brkcond,        // IN
    PBRKOUT pbrkout)        // OUT
{
    BOOL         fSuccessful;
    BOOL         fBreakBefore = (pposichnk->ichnk == ichnkOutside);
    long         ichnk = fBreakBefore
                         ? 0
                         : long(pposichnk->ichnk);
    CGlyphDobj * pdobj = (CGlyphDobj *)(plocchnk->plschnk[ichnk].pdobj);
    LSERR        lserr;

    WHEN_DBG( CLineServices * pLS = pdobj->GetPLS() );

    if (fBreakBefore)
    {
        //                         Second DOBJ
        //
        //                       TEXT   NOSCOPE   BEGIN    END
        //            --------+-------+--------+--------+--------
        //   First    TEXT    |  n/a  | before | before |  no
        //    DOBJ    NOSCOPE | after | after  | after  | after
        //            BEGIN   |   no  | after  |  no    |  no
        //            END     | after | after  | after  |  no
        //

        BOOL fPrevWasEnd = TRUE;

        do
        {
            pdobj = (CGlyphDobj *)(plocchnk->plschnk[ichnk].pdobj);

            // break after NOSCOPE, or END unless followed by END

            fSuccessful =    pdobj->_fNoScope
                          || (pdobj->_fBegin && fPrevWasEnd);

            if (fSuccessful)
                break;

            fPrevWasEnd = !pdobj->_fBegin;

        } while (++ichnk < long(plocchnk->clschnk) );

        if (!fSuccessful)
        {
            // break after END

            ichnk = long(plocchnk->clschnk) - 1;
            fSuccessful = !pdobj->_fBegin;
            fBreakBefore = FALSE;
        }
    }
    else
    {
        if (pdobj->_fNoScope)
        {
            // break after NOSCOPE

            fSuccessful = TRUE;
        }
        else
        {
            // break after END followed by TEXT/BEGIN/NOSCOPE

            if (ichnk < (long(plocchnk->clschnk) - 1))
            {
                if (!pdobj->_fBegin)
                {
                    CGlyphDobj * pdobjNext = (CGlyphDobj *)(plocchnk->plschnk[ichnk+1].pdobj);

                    fSuccessful = pdobjNext->_fNoScope || pdobjNext->_fBegin;
                }
                else
                {
                    fSuccessful = TRUE;
                }
            }
            else
            {
                fSuccessful = !pdobj->_fBegin;
            }
        }
    }

    pbrkout->fSuccessful = fSuccessful;
    pbrkout->brkcond = fSuccessful ? brkcondPlease : brkcondNever;
    pbrkout->posichnk.ichnk = ichnk;
    pbrkout->posichnk.dcp = fBreakBefore ? 0 : plocchnk->plschnk[ichnk].dcp;

    lserr = pdobj->QueryObjDim(&pbrkout->objdim);

    DumpBrkopt( "Next(G)", pLS->_fMinMaxPass, plocchnk, pposichnk, brkcond, pbrkout );

    return lserr;
}

LSERR WINAPI /* static */
CLineServices::ForceBreakChunk(
    PCLOCCHNK plocchnk,     // IN
    PCPOSICHNK pposichnk,   // IN
    PBRKOUT pbrkout)        // OUT
{
    LSTRACE(ForceBreakChunk);

    TraceTag((tagTraceILSBreak,
              "CLineServices::ForceBreakChunk(ichnk=%d)",
              pposichnk->ichnk));

    ZeroMemory( pbrkout, sizeof(BRKOUT) );

    pbrkout->fSuccessful = fTrue;

    if (   plocchnk->lsfgi.fFirstOnLine
        && pposichnk->ichnk == 0
        || pposichnk->ichnk == ichnkOutside)
    {
        PDOBJ pdobj  = (PDOBJ) plocchnk->plschnk[0].pdobj;

        pbrkout->posichnk.dcp = plocchnk->plschnk[0].dcp;
        pdobj->QueryObjDim(&pbrkout->objdim);
    }
    else
    {
        pbrkout->posichnk.ichnk = pposichnk->ichnk;
        pbrkout->posichnk.dcp = 0;
    }

    return lserrNone;
}

LSERR WINAPI /* static */
CEmbeddedDobj::SetBreak(
    PDOBJ pdobj,                    // IN
    BRKKIND brkkind,                // IN
    DWORD nBreakRecord,             // IN
    BREAKREC* rgBreakRecord,        // OUT
    DWORD* pnActualBreakRecord)     // OUT
{
    LSTRACE(EmbeddedSetBreak);


    // This function is called telling us that a new break has occured
    // in the line that we're in.  This is so we can adjust our geometry
    // for it.  We're not gonna do that.  So we ignore it.
    // This function is actually a lot like a "commit" saying that the
    // break is actually being used.  The break is discovered using
    // findnextbreak and findprevbreak functions, and truncate and stuff
    // like that.

    return lserrNone;
}


LSERR WINAPI /* static */
CNobrDobj::SetBreak(
    PDOBJ pdobj,                    // IN
    BRKKIND brkkind,                // IN
    DWORD nBreakRecord,             // IN
    BREAKREC* rgBreakRecord,        // OUT
    DWORD* pnActualBreakRecord)     // OUT
{
    LSTRACE(NobrSetBreak);

    // This function is called telling us that a new break has occured
    // in the line that we're in.  This is so we can adjust our geometry
    // for it.  We're not gonna do that.  So we ignore it.
    // This function is actually a lot like a "commit" saying that the
    // break is actually being used.  The break is discovered using
    // findnextbreak and findprevbreak functions, and truncate and stuff
    // like that.

    return lserrNone;
}

LSERR WINAPI /* static */
CGlyphDobj::SetBreak(
    PDOBJ pdobj,                    // IN
    BRKKIND brkkind,                // IN
    DWORD nBreakRecord,             // IN
    BREAKREC* rgBreakRecord,        // OUT
    DWORD* pnActualBreakRecord)     // OUT
{
    LSTRACE(GlyphSetBreak);


    // This function is called telling us that a new break has occured
    // in the line that we're in.  This is so we can adjust our geometry
    // for it.  We're not gonna do that.  So we ignore it.
    // This function is actually a lot like a "commit" saying that the
    // break is actually being used.  The break is discovered using
    // findnextbreak and findprevbreak functions, and truncate and stuff
    // like that.

    return lserrNone;
}


LSERR WINAPI /* static */
CEmbeddedDobj::GetSpecialEffectsInside(
    PDOBJ pdboj,        // IN
    UINT* pEffectsFlag) // OUT
{
    LSTRACE(EmbeddedGetSpecialEffectsInside);

    *pEffectsFlag = 0;

    return lserrNone;
}

LSERR WINAPI /* static */
CEmbeddedDobj::FExpandWithPrecedingChar(
    PDOBJ pdobj,            // IN
    PLSRUN plsrun,          // IN
    PLSRUN plsrunText,      // IN
    WCHAR wchar,            // IN
    MWCLS mwcls,            // IN
    BOOL* pfExpand)         // OUT
{
    LSTRACE(EmbeddedFExpandWithPrecedingChar);

    *pfExpand = TRUE;

    return lserrNone;
}

LSERR WINAPI
CEmbeddedDobj::FExpandWithFollowingChar(
    PDOBJ pdobj,            // IN
    PLSRUN plsrun,          // IN
    PLSRUN plsrunText,      // IN
    WCHAR wchar,            // IN
    MWCLS mwcls,            // IN
    BOOL* pfExpand)         // OUT
{
    LSTRACE(EmbeddedFExpandWithFollowingChar);

    *pfExpand = TRUE;

    return lserrNone;
}


LSERR WINAPI /* static */
CNobrDobj::GetSpecialEffectsInside(
    PDOBJ pdboj,        // IN
    UINT* pEffectsFlag) // OUT
{
    LSTRACE(NobrGetSpecialEffectsInside);

    *pEffectsFlag = 0;

    return lserrNone;
}

LSERR WINAPI /* static */
CNobrDobj::FExpandWithPrecedingChar(
    PDOBJ pdobj,            // IN
    PLSRUN plsrun,          // IN
    PLSRUN plsrunText,      // IN
    WCHAR wchar,            // IN
    MWCLS mwcls,            // IN
    BOOL* pfExpand)         // OUT
{
    LSTRACE(NobrFExpandWithPrecedingChar);

    *pfExpand = TRUE;

    return lserrNone;
}

LSERR WINAPI
CNobrDobj::FExpandWithFollowingChar(
    PDOBJ pdobj,            // IN
    PLSRUN plsrun,          // IN
    PLSRUN plsrunText,      // IN
    WCHAR wchar,            // IN
    MWCLS mwcls,            // IN
    BOOL* pfExpand)         // OUT
{
    LSTRACE(NobrFExpandWithFollowingChar);

    *pfExpand = TRUE;

    return lserrNone;
}

LSERR WINAPI /* static */
CGlyphDobj::GetSpecialEffectsInside(
    PDOBJ pdboj,        // IN
    UINT* pEffectsFlag) // OUT
{
    LSTRACE(GlyphGetSpecialEffectsInside);

    *pEffectsFlag = 0;

    return lserrNone;
}


LSERR WINAPI /* static */
CGlyphDobj::FExpandWithPrecedingChar(
    PDOBJ pdobj,            // IN
    PLSRUN plsrun,          // IN
    PLSRUN plsrunText,      // IN
    WCHAR wchar,            // IN
    MWCLS mwcls,            // IN
    BOOL* pfExpand)         // OUT
{
    LSTRACE(GlyphFExpandWithPrecedingChar);

    *pfExpand = FALSE;

    return lserrNone;
}

LSERR WINAPI
CGlyphDobj::FExpandWithFollowingChar(
    PDOBJ pdobj,            // IN
    PLSRUN plsrun,          // IN
    PLSRUN plsrunText,      // IN
    WCHAR wchar,            // IN
    MWCLS mwcls,            // IN
    BOOL* pfExpand)         // OUT
{
    LSTRACE(GlyphFExpandWithFollowingChar);

    *pfExpand = FALSE;

    return lserrNone;
}

LSERR WINAPI /* static */
CEmbeddedDobj::CalcPresentation(
    PDOBJ pdobj,                // IN
    long dup,                   // IN
    LSKJUST lskjust,            // IN
    BOOL fLastVisibleOnLine )   // IN
{
    LSTRACE(EmbeddedCalcPresentation);
    return lserrNone;
}


LSERR WINAPI /* static */
CNobrDobj::CalcPresentation(
    PDOBJ pdobj,                // IN
    long dup,                   // IN
    LSKJUST lskjust,            // IN
    BOOL fLastVisibleOnLine)    // IN
{
    LSTRACE(NobrCalcPresentation);

    // This is taken from the Reverse object's code (in robj.c).

    CNobrDobj* pNobr= (CNobrDobj*)pdobj;

    LSERR lserr;
    BOOL fDone;

    /* Make sure that justification line has been made ready for presentation */
    lserr = LssbFDonePresSubline(pNobr->_plssubline, &fDone);

    if ((lserrNone == lserr) && !fDone)
    {
        lserr = LsMatchPresSubline(pNobr->_plssubline);
    }

    return lserr;
}

LSERR WINAPI /* static */
CGlyphDobj::CalcPresentation(
    PDOBJ pdobj,                // IN
    long dup,                   // IN
    LSKJUST lskjust,            // IN
    BOOL fLastVisibleOnLine)    // IN
{
    LSTRACE(GlyphCalcPresentation);
    return lserrNone;
}

LSERR WINAPI /* static */
CEmbeddedDobj::QueryPointPcp(
    PDOBJ pdobj,            // IN
    PCPOINTUV pptuvQuery,   // IN
    PCLSQIN plsqIn,         // IN
    PLSQOUT plsqOut)        // OUT
{
    LSTRACE(EmbeddedQueryPointPcp);

    CLineServices* pLS = pdobj->GetPLS();
    CFlowLayout * pFlowLayout = pLS->_pFlowLayout;
    PLSRUN plsrun = PLSRUN(plsqIn->plsrun);
    CLayout * pLayout = plsrun->GetLayout(pFlowLayout);
    long xWidth;

    ZeroMemory( plsqOut, sizeof(LSQOUT) );

    pFlowLayout->GetSiteWidth( pLayout, pLS->_pci, FALSE, 0, &xWidth);

    plsqOut->dupObj  = xWidth;
    plsqOut->plssubl = NULL;

    plsqOut->lstextcell.cpStartCell = pLayout->GetContentFirstCp();
    plsqOut->lstextcell.cpEndCell   = pLayout->GetContentLastCp();

    plsqOut->lstextcell.dupCell      = xWidth;
    plsqOut->lstextcell.pCellDetails = NULL;

    return lserrNone;
}


LSERR WINAPI /* static */
CNobrDobj::QueryPointPcp(
    PDOBJ pdobj,            // IN
    PCPOINTUV pptuvQuery,   // IN
    PCLSQIN plsqIn,         // IN
    PLSQOUT plsqOut)        // OUT
{
    LSTRACE(NobrQueryPointPcp);
    PLSSUBL plssubl= ((CNobrDobj*)pdobj)->_plssubline;

    // I agree with my dimensions reported during formatting
    plsqOut->pointUvStartObj.u = 0;
    plsqOut->pointUvStartObj.v = 0;
    plsqOut->heightsPresObj = plsqIn->heightsPresRun;
    plsqOut->dupObj = plsqIn->dupRun;

    // I am not terminal object and here is my subline for you to continue querying
    plsqOut->plssubl = plssubl;

    // my subline starts where my object starts
    plsqOut->pointUvStartSubline.u = 0;
    plsqOut->pointUvStartSubline.v = 0;

    // I am not terminal, so textcell should not be filled by me,
    // but maybe it's a good idea to do something like:
    ZeroMemory(&plsqOut->lstextcell, sizeof(plsqOut->lstextcell));

    return lserrNone;
}

LSERR WINAPI /* static */
CGlyphDobj::QueryPointPcp(
    PDOBJ pdobj,            // IN
    PCPOINTUV pptuvQuery,   // IN
    PCLSQIN plsqIn,         // IN
    PLSQOUT plsqOut)        // OUT
{
    LSTRACE(GlyphQueryPointPcp);
    CGlyphDobj *pGlyphdobj = DYNCAST(CGlyphDobj, pdobj);

    PLSRUN plsrun = PLSRUN(plsqIn->plsrun);

    ZeroMemory( plsqOut, sizeof(LSQOUT) );

    plsqOut->dupObj  = pGlyphdobj->_RenderInfo.width;
    plsqOut->plssubl = NULL;

    plsqOut->lstextcell.cpStartCell = plsrun->_lscpBase;
    plsqOut->lstextcell.cpEndCell   = plsrun->_lscpBase + 1;

    plsqOut->lstextcell.dupCell      = pGlyphdobj->_RenderInfo.width;
    plsqOut->lstextcell.pCellDetails = NULL;

    return lserrNone;
}


// It's not documented, but I'm guessing that this routine is asking for
// a bounding box around the foreign object.
// From the quill code:
//    "Given a CP, we return the dimensions of the object.
//     Because we have a simple setup where there is only one dobj per CP,
//     it is easy to do."
LSERR WINAPI /* static */
CEmbeddedDobj::QueryCpPpoint(
    PDOBJ pdobj,            // IN: dobj to query
    LSDCP dcp,              // IN: dcp for the query
    PCLSQIN plsqIn,         // IN: rectangle info of querying dnode
    PLSQOUT plsqOut)        // OUT: rectangle info of this cp
{
    LSTRACE(QueryCpPpoint);

    CLineServices* pLS = pdobj->GetPLS();
    CFlowLayout * pFlowLayout = pLS->_pFlowLayout;

    PLSRUN plsrun = PLSRUN(plsqIn->plsrun);  // why do I have to cast this?
    CLayout * pLayout = plsrun->GetLayout(pFlowLayout);
    long xWidth;
    pFlowLayout->GetSiteWidth( pLayout, pLS->_pci, FALSE, 0, &xWidth);

    // Do we need to set cpStartCell and cpEndCell?

    plsqOut->plssubl= NULL;
    plsqOut->lstextcell.dupCell= xWidth;

    return lserrNone;
}


LSERR WINAPI /* static */
CNobrDobj::QueryCpPpoint(
    PDOBJ pdobj,            // IN: dobj to query
    LSDCP dcp,              // IN: dcp for the query
    PCLSQIN plsqIn,         // IN: rectangle info of querying dnode
    PLSQOUT plsqOut)        // OUT: rectangle info of this cp
{
    LSTRACE(QueryCpPpoint);

    PLSSUBL plssubl= ((CNobrDobj*)pdobj)->_plssubline;

    // I agree with my dimensions reported during formatting
    plsqOut->pointUvStartObj.u = 0;
    plsqOut->pointUvStartObj.v = 0;
    plsqOut->heightsPresObj = plsqIn->heightsPresRun;
    plsqOut->dupObj = plsqIn->dupRun;

    // I am not terminal object and here is my subline for you to continue querying
    plsqOut->plssubl = plssubl;

    // my subline starts where my object starts
    plsqOut->pointUvStartSubline.u = 0;
    plsqOut->pointUvStartSubline.v = 0;

    // I am not terminal, so textcell should not be filled by me,
    // but maybe it's a good idea to do something like:
    ZeroMemory(&plsqOut->lstextcell, sizeof(plsqOut->lstextcell));

    return lserrNone;
}


LSERR WINAPI /* static */
CGlyphDobj::QueryCpPpoint(
    PDOBJ pdobj,            // IN: dobj to query
    LSDCP dcp,              // IN: dcp for the query
    PCLSQIN plsqIn,         // IN: rectangle info of querying dnode
    PLSQOUT plsqOut)        // OUT: rectangle info of this cp
{
    LSTRACE(QueryCpPpoint);
    CGlyphDobj *pGlyphdobj = DYNCAST(CGlyphDobj, pdobj);

    // Do we need to set cpStartCell and cpEndCell?
    plsqOut->plssubl= NULL;
    plsqOut->lstextcell.dupCell= pGlyphdobj->_RenderInfo.width;

    return lserrNone;
}

LSERR WINAPI    // static
CEmbeddedDobj::Enum(
    PDOBJ pdobj,                // IN
    PLSRUN plsrun,              // IN
    PCLSCHP plschp,             // IN
    LSCP cpFirst,               // IN
    LSDCP dcp,                  // IN
    LSTFLOW lstflow,            // IN
    BOOL fReverseOrder,         // IN
    BOOL fGeometryNeeded,       // IN
    const POINT * pptStart,     // IN
    PCHEIGHTS pheightsPres,     // IN
    long dupRun)                // IN
{
    LSTRACE(Enum);

    CLineServices *pLS = pdobj->GetPLS();

    //
    // During a min-max pass, we set the width of the dobj to the minimum
    // width.  The maximum width therefore is off by the delta amount.
    //

    pLS->_dvMaxDelta += pdobj->_dvMinMaxDelta;

    return lserrNone;
}

LSERR WINAPI    // static
CNobrDobj::Enum(
    PDOBJ pdobj,                // IN
    PLSRUN plsrun,              // IN
    PCLSCHP plschp,             // IN
    LSCP cpFirst,               // IN
    LSDCP dcp,                  // IN
    LSTFLOW lstflow,            // IN
    BOOL fReverseOrder,         // IN
    BOOL fGeometryNeeded,       // IN
    const POINT * pptStart,     // IN
    PCHEIGHTS pheightsPres,     // IN
    long dupRun)                // IN
{
    LSTRACE(Enum);

    return lserrNone;
}

LSERR WINAPI    // static
CGlyphDobj::Enum(
    PDOBJ pdobj,                // IN
    PLSRUN plsrun,              // IN
    PCLSCHP plschp,             // IN
    LSCP cpFirst,               // IN
    LSDCP dcp,                  // IN
    LSTFLOW lstflow,            // IN
    BOOL fReverseOrder,         // IN
    BOOL fGeometryNeeded,       // IN
    const POINT * pptStart,     // IN
    PCHEIGHTS pheightsPres,     // IN
    long dupRun)                // IN
{
    LSTRACE(Enum);
    return lserrNone;
}

LSERR WINAPI                 // static
CEmbeddedDobj::Display(      // this = PDOBJ pdobj,
    PDOBJ pdobj,             // IN
    PCDISPIN pdispin)        // IN
{
    LSTRACE(Display);
    PLSRUN plsrun = PLSRUN(pdispin->plsrun);
    CEmbeddedILSObj *pEmbeddedILSObj = DYNCAST(CEmbeddedILSObj, DYNCAST(CEmbeddedDobj, pdobj)->_pilsobj);
    CLSRenderer  *pRenderer = pEmbeddedILSObj->_pLS->GetRenderer();

    //
    // Ignore the return value of this, but the call updates the _xAccumulatedWidth
    // if the run needs to be skipped. This allows following runs to be displayed
    // correctly if they are relatively positioned (Bug 46346).
    //
    pRenderer->ShouldSkipThisRun(plsrun, pdispin->dup);
    return lserrNone;
}


LSERR WINAPI     // static
CNobrDobj::Display(
    PDOBJ pdobj,                // IN
    PCDISPIN pdispin)   // IN
{
    LSTRACE(Display);
    CNobrDobj* pNobr= (CNobrDobj *)pdobj;

    return LsDisplaySubline(pNobr->_plssubline, // The subline to be drawn
                            &pdispin->ptPen,    // The point at which to draw the line
                            pdispin->kDispMode, // Draw in transparent mode
                            pdispin->prcClip    // The clipping rectangle
                           );
}

LSERR WINAPI                 // static
CGlyphDobj::Display(         // this = PDOBJ pdobj,
    PDOBJ pdobj,             // IN
    PCDISPIN pdispin)        // IN
{
    LSTRACE(Display);
    CGlyphRenderInfoType *pRenderInfo = &(DYNCAST(CGlyphDobj, pdobj)->_RenderInfo);
    extern void DrawTextSelectionForRect(HDC hdc, CRect *prc, CRect *prcClip, BOOL fSwapColor);

    if (pRenderInfo->pImageContext)
    {
        CRect rc;
        PLSRUN plsrun = PLSRUN(pdispin->plsrun);
        CGlyphILSObj *pGlyphILSObj = DYNCAST(CGlyphILSObj, DYNCAST(CGlyphDobj, pdobj)->_pilsobj);
        CLSRenderer  *pRenderer = pGlyphILSObj->_pLS->GetRenderer();

        if (pRenderer->ShouldSkipThisRun(plsrun, pdispin->dup))
            goto Cleanup;

        pRenderer->_ptCur.x = rc.left = pdispin->ptPen.x - pRenderer->_xAccumulatedWidth;
        rc.right = rc.left + pRenderInfo->width;

        rc.bottom = pdispin->ptPen.y + pRenderer->_li._yHeight - pRenderer->_li._yDescent;
        rc.top    = rc.bottom - pRenderInfo->height;

        pRenderInfo->pImageContext->Draw(pRenderer->_hdc, &rc);
        if (plsrun->_fSelected)
            DrawTextSelectionForRect(pRenderer->_hdc, &rc, &rc, plsrun->GetCF()->SwapSelectionColors());
    }

Cleanup:
    return lserrNone;
}

CDobjBase::CDobjBase(PILSOBJ pilsobjNew, PLSDNODE plsdn)
: _pilsobj(pilsobjNew), _plsdnTop(plsdn)
{}


CEmbeddedDobj::CEmbeddedDobj(PILSOBJ pilsobjNew, PLSDNODE plsdn)
: CDobjBase(pilsobjNew, plsdn)
{}


CNobrDobj::CNobrDobj(PILSOBJ pilsobjNew, PLSDNODE plsdn)
: CDobjBase(pilsobjNew, plsdn), _plssubline(NULL)
{}


CGlyphDobj::CGlyphDobj(PILSOBJ pilsobjNew, PLSDNODE plsdn)
: CDobjBase(pilsobjNew, plsdn), _plssubline(NULL)
{}

LSERR WINAPI /* static */
CEmbeddedDobj::DestroyDObj(
    PDOBJ pdobj)        // IN
{
    LSTRACE(DestroyDObj);
    delete (CEmbeddedDobj *)pdobj;
    return lserrNone;
}


LSERR WINAPI /* static */
CNobrDobj::DestroyDObj(
    PDOBJ pdobj)        // IN
{
    LSTRACE(DestroyDObj);
    LsDestroySubline(((CNobrDobj *)pdobj)->_plssubline);
    delete (CNobrDobj *)pdobj;
    return lserrNone;
}

LSERR WINAPI /* static */
CGlyphDobj::DestroyDObj(
    PDOBJ pdobj)        // IN
{
    LSTRACE(DestroyDObj);
    delete (CGlyphDobj *)pdobj;
    return lserrNone;
}


CILSObjBase::CILSObjBase(CLineServices* pols, PCLSCBK plscbk)
: _pLS(pols)
{
    // We don't need plscbk.
}

CILSObjBase::~CILSObjBase()
{
}


LSERR WINAPI
CILSObjBase::DestroyILSObj()  // this= pilsobj
{
    LSTRACE(DestroyILSObj);

    delete this;

    return lserrNone;
}


LSERR WINAPI
CILSObjBase::CreateLNObj(PLNOBJ* pplnobj) // this= pcilsobj
{
    LSTRACE(CreateLNObj);

    // All LNobj's are the same object as our ilsobj.
    // This object's lifetime is determined by the ilsobj
    *pplnobj= (PLNOBJ) this;

    return lserrNone;
}

LSERR WINAPI
CILSObjBase::DestroyLNObj() // this= plnobj
{
    LSTRACE(DestroyLNObj);

    // Do nothing, since we never really created one.
    return lserrNone;
}


LSERR WINAPI
CILSObjBase::SetDoc(PCLSDOCINF)  // this = pilsobj
{
    LSTRACE(SetDoc);

    // We don't have anything to do.
    return lserrNone;
}


CEmbeddedILSObj::CEmbeddedILSObj(CLineServices* pols, PCLSCBK plscbk)
: CILSObjBase(pols,plscbk)
{}


CNobrILSObj::CNobrILSObj(CLineServices* pols, PCLSCBK plscbk)
: CILSObjBase(pols,plscbk)
{}

CGlyphILSObj::CGlyphILSObj(CLineServices* pols, PCLSCBK plscbk)
: CILSObjBase(pols,plscbk)
{}

LSERR WINAPI
CNobrILSObj::FmtResume(  // this = plnobj
    const BREAKREC* rgBreakRecord,      // IN
    DWORD nBreakRecord,                 // IN
    PCFMTIN pfmtin,                     // IN
    FMTRES* pfmtres)                    // OUT
{
    LSTRACE(FmtResume);
    LSNOTIMPL(FmtResume);

    // I believe that this should never get called for most embedded objects.
    // The only possible exception that comes to mind are marquees, and I do
    // not believe that they can wrap around lines.

    return lserrNone;
}

LSERR WINAPI
CGlyphILSObj::FmtResume(  // this = plnobj
    const BREAKREC* rgBreakRecord,      // IN
    DWORD nBreakRecord,                 // IN
    PCFMTIN pfmtin,                     // IN
    FMTRES* pfmtres)                    // OUT
{
    LSTRACE(FmtResume);
    LSNOTIMPL(FmtResume);

    // I believe that this should never get called for most embedded objects.
    // The only possible exception that comes to mind are marquees, and I do
    // not believe that they can wrap around lines.

    return lserrNone;
}

BRKCOND
CNobrDobj::GetBrkcondBefore(COneRun * por)
{
    if (_brkcondBefore == brkcondUnknown)
    {
        if (!por->_fIsArtificiallyStartedNOBR)
        {
            CLineServices::BRKCLS brkcls, dummy;

            // find the first character of the nobr.
            while (!por->IsNormalRun())
            {
                por = por->_pNext;
            }

            if (por->_pchBase != NULL)
            {
                GetPLS()->GetBreakingClasses( por, 0, por->_pchBase[0], &brkcls, &dummy );
                _brkcondBefore = CLineServices::s_rgbrkcondBeforeChar[brkcls];
            }
            else
            {
                _brkcondBefore = brkcondPlease;
            }
        }
        else
        {
            _brkcondBefore = brkcondNever;
        }               
    }

    return _brkcondBefore;
}

BRKCOND
CNobrDobj::GetBrkcondAfter(COneRun * por, LONG dcp)
{
    if (_brkcondAfter == brkcondUnknown)
    {
        if (!por->_fIsArtificiallyTerminatedNOBR)
        {
            CLineServices::BRKCLS brkcls, dummy;
            COneRun * porText = por;

            // find the last character of the nobr.
            // the last cp is the [endnobr] character, which we're not interested in; hence
            // the loop stops at 1

            while (dcp > 1)
            {
                if (por->IsNormalRun())
                    porText = por;

                dcp -= por->_lscch;
                por = por->_pNext;
            };

            if (porText->_pchBase != NULL)
            {
                GetPLS()->GetBreakingClasses( porText, 0, porText->_pchBase[porText->_lscch-1], &brkcls, &dummy );
                _brkcondAfter = CLineServices::s_rgbrkcondAfterChar[brkcls];
            }
            else
            {
                _brkcondAfter = brkcondPlease;
            }
        }
        else
        {
            _brkcondAfter = brkcondNever;
        }
    }

    return _brkcondAfter;
}

