/*
 *  @doc    INTERNAL
 *
 *  @module LINESRV.CXX -- line services interface
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *      Sujal Parikh <nl>
 *      Paul Parker
 *
 *  History: <nl>
 *      11/20/97     cthrash created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

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

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_TEXTXFRM_HXX_
#define X_TEXTXFRM_HXX_
#include <textxfrm.hxx>
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_RUBY_H_
#define X_RUBY_H_
#include <ruby.h>
#endif

#ifndef X_HIH_H_
#define X_HIH_H_
#include <hih.h>
#endif

#ifndef X_TATENAK_H_
#define X_TATENAK_H_
#include <tatenak.h>
#endif

#ifndef X_WARICHU_H_
#define X_WARICHU_H_
#include <warichu.h>
#endif

#ifndef X_ROBJ_H_
#define X_ROBJ_H_
#include <robj.h>
#endif

#ifndef X_LSHYPH_H_
#define X_LSHYPH_H_
#include <lshyph.h>
#endif

#ifndef X_LSKYSR_H_
#define X_LSKYSR_H_
#include <lskysr.h>
#endif

#ifndef X_LSEMS_H_
#define X_LSEMS_H_
#include <lsems.h>
#endif

#ifndef X_LSPAP_H_
#define X_LSPAP_H_
#include <lspap.h>
#endif

#ifndef X_LSCHP_H_
#define X_LSCHP_H_
#include <lschp.h>
#endif

#ifndef X_LSTABS_H_
#define X_LSTABS_H_
#include <lstabs.h>
#endif

#ifndef X_LSTXM_H_
#define X_LSTXM_H_
#include <lstxm.h>
#endif

#ifndef X_OBJDIM_H_
#define X_OBJDIM_H_
#include <objdim.h>
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include <flowlyt.hxx>
#endif

DeclareLSTag( tagLSCallBack, "Trace callbacks" );
DeclareLSTag( tagLSAsserts, "Show LS Asserts" );
DeclareLSTag( tagLSFetch, "Trace FetchPap/FetchRun" );
DeclareLSTag( tagLSNoBlast, "No blasting at render time" );
DeclareLSTag( tagLSIME, "Trace IME" );
DeclareLSTag( tagAssertOnHittestingWithLS, "Enable hit-testing assertions.");

MtDefine(LineServices, Mem, "LineServices")
MtDefine(LineServicesMem, LineServices, "CLineServices::NewPtr/ReallocPtr")
MtDefine(CLineServices, LineServices, "CLineServices")
MtDefine(CLineServices_arySynth_pv, CLineServices, "CLineServices::_arySynth::_pv")
MtDefine(CLineServices_aryOneRuns_pv, CLineServices, "CLineServices::_aryOneRuns::_pv")
MtDefine(CLineServices_aryLineFlags_pv,  CLineServices, "CLineServices::_aryLineFlags::_pv")
MtDefine(CLineServices_aryLineCounts_pv, CLineServices, "CLineServices::_aryLineCounts::_pv")
MtDefine(CLineServicesCalculatePositionsOfRangeOnLine_aryLsqsubinfo_pv, Locals, "CLineServices::CalculatePositionsOfRangeOnLine::aryLsqsubinfo_pv");
MtDefine(CLineServicesCalculateRectsOfRangeOnLine_aryLsqsubinfo_pv, Locals, "CLineServices::CalculateRectsOfRangeOnLine::aryLsqsubinfo_pv");
MtDefine(COneRun, LineServices, "COneRun")

#define ONERUN_NORMAL    0x00
#define ONERUN_SYNTHETIC 0x01
#define ONERUN_ANTISYNTH 0x02

enum KASHIDA_PRIORITY
{
    KASHIDA_PRIORITY1,  // SCRIPT_JUSTIFY_ARABIC_TATWEEL
    KASHIDA_PRIORITY2,  // SCRIPT_JUSTIFY_ARABIC_SEEN
    KASHIDA_PRIORITY3,  // SCRIPT_JUSTIFY_ARABIC_HA
    KASHIDA_PRIORITY4,  // SCRIPT_JUSTIFY_ARABIC_ALEF
    KASHIDA_PRIORITY5,  // SCRIPT_JUSTIFY_ARABIC_RA
    KASHIDA_PRIORITY6,  // SCRIPT_JUSTIFY_ARABIC_BARA
    KASHIDA_PRIORITY7,  // SCRIPT_JUSTIFY_ARABIC_BA
    KASHIDA_PRIORITY8,  // SCRIPT_JUSTIFY_ARABIC_NORMAL
    KASHIDA_PRIORITY9,  // Max - lowest priority
};

//-----------------------------------------------------------------------------
//
//  Function:   InitLineServices (global)
//
//  Synopsis:   Instantiates a instance of the CLineServices object and makes
//              the requisite calls into the LineServices DLL.
//
//  Returns:    HRESULT
//              *ppLS - pointer to newly allocated CLineServices object
//
//-----------------------------------------------------------------------------

HRESULT
InitLineServices(
    CMarkup *pMarkup,           // IN
    BOOL fStartUpLSDLL,         // IN
    CLineServices ** ppLS)      // OUT
{
    HRESULT hr = S_OK;
    CLineServices * pLS;

    // Note: this assertion will fire sometimes if you're trying to put
    // pointers to member functions into the lsimethods structure.  Note that
    // pointer to member functions are different from function pointers -- ask
    // one of the Borland guys (like ericvas) to explain.  Sometimes they will
    // be bigger than 4 bytes, causing mis-alignment between our structure and
    // LS's, and thus this assertion to fire.

#if defined(UNIX) || defined(_MAC) // IEUNIX uses 8/12 bytes Method ptrs.
    AssertSz(sizeof(CLineServices::LSIMETHODS) == sizeof(LSIMETHODS),
             "Line Services object callback struct has unexpectedly changed.");
    AssertSz(sizeof(CLineServices::LSCBK) == sizeof(LSCBK),
             "Line Services callback struct has unexpectedly changed.");
#endif
    //
    // Create our Line Services interface object
    //

    pLS = new CLineServices(pMarkup);
    if (!pLS)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    WHEN_DBG(pLS->_lockRecrsionGuardFetchRun = FALSE;)
            
    pLS->_plsc = NULL;
    if (fStartUpLSDLL)
    {
        hr = THR(StartUpLSDLL(pLS, pMarkup));
        if (hr)
        {
            delete pLS;
            goto Cleanup;
        }
    }

    //
    // Return value
    //

    *ppLS = pLS;

Cleanup:
    RRETURN(hr);
}


HRESULT
StartUpLSDLL(CLineServices *pLS, CMarkup *pMarkup)
{
    LSCONTEXTINFO lsci;
    HRESULT hr = S_OK;

    if (pMarkup != pLS->GetMarkup())
        pLS->_treeInfo._tpFrontier.Reinit(pMarkup, 0);

    if (pLS->_plsc)
        goto Cleanup;

    //
    // Fetch the Far East object handlers
    //

    hr = THR( pLS->SetupObjectHandlers() );
    if (hr)
        goto Cleanup;

    //
    // Populate the LSCONTEXTINFO
    //

    lsci.version = 0;
    lsci.cInstalledHandlers = CLineServices::LSOBJID_COUNT;
#if !defined(UNIX) && !defined(_MAC)
    *(CLineServices::LSIMETHODS **)&lsci.pInstalledHandlers = pLS->s_rgLsiMethods;
#else
    {
        static BOOL fInitDone = FALSE;
        if (!fInitDone)
        {
            // Copy s_rgLsiMethods -> s_unix_rgLsiMethods
            pLS->InitLsiMethodStruct();
            fInitDone = TRUE;
        }
    }
    lsci.pInstalledHandlers = pLS->s_unix_rgLsiMethods;
#endif
    lsci.lstxtcfg = pLS->s_lstxtcfg;
    lsci.pols = (POLS)pLS;
#if !defined(UNIX) && !defined(_MAC)
    *(CLineServices::LSCBK *)&lsci.lscbk = CLineServices::s_lscbk;
#else
    CLineServices::s_lscbk.fill(&lsci.lscbk);
#endif
    lsci.fDontReleaseRuns = TRUE;

    //
    // Call in to Line Services
    //

    hr = HRFromLSERR( LsCreateContext( &lsci, &pLS->_plsc ) );
    if (hr)
        goto Cleanup;

    //
    // Set Expansion/Compression tables
    //

    hr = THR( pLS->SetModWidthPairs() );
    if (hr)
        goto Cleanup;
            
    //
    // Runtime sanity check
    //

    WHEN_DBG( pLS->InitTimeSanityCheck() );

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function:   DeinitLineServices (global)
//
//  Synopsis:   Frees a CLineServes object.
//
//  Returns:    HRESULT
//
//-----------------------------------------------------------------------------

HRESULT
DeinitLineServices(CLineServices * pLS)
{
    HRESULT hr = S_OK;

    if (pLS->_plsc)
        hr = HRFromLSERR( LsDestroyContext( pLS->_plsc ) );

    delete pLS;

    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function:   SetupObjectHandlers (member)
//
//  Synopsis:   LineServices uses object handlers for special textual
//              representation.  There are six such objects in Trident,
//              and for five of these, the callbacks are implemented by
//              LineServices itself.  The sixth object, our handle for
//              embedded/nested objects, is implemented in lsobj.cxx.
//
//  Returns:    S_OK - Success
//              E_FAIL - A LineServices error occurred
//
//-----------------------------------------------------------------------------

HRESULT
CLineServices::SetupObjectHandlers()
{
    HRESULT hr = E_FAIL;
    ::LSIMETHODS *pLsiMethod;

#if !defined(UNIX) && !defined(_MAC)
    pLsiMethod = (::LSIMETHODS *)s_rgLsiMethods;
#else
    pLsiMethod = s_unix_rgLsiMethods;
#endif

    if (lserrNone != LsGetRubyLsimethods( pLsiMethod + LSOBJID_RUBY ))
        goto Cleanup;

    if (lserrNone != LsGetTatenakayokoLsimethods( pLsiMethod + LSOBJID_TATENAKAYOKO ))
        goto Cleanup;

    if (lserrNone != LsGetHihLsimethods( pLsiMethod + LSOBJID_HIH ))
        goto Cleanup;

    if (lserrNone != LsGetWarichuLsimethods( pLsiMethod + LSOBJID_WARICHU ))
        goto Cleanup;

    if (lserrNone != LsGetReverseLsimethods( pLsiMethod + LSOBJID_REVERSE ))
        goto Cleanup;

    hr = S_OK;

Cleanup:

    return hr;
}

//-----------------------------------------------------------------------------
//
//  Function:   NewPtr (member, LS callback)
//
//  Synopsis:   A client-side allocation routine for LineServices.
//
//  Returns:    Pointer to buffer allocated, or NULL if out of memory.
//
//-----------------------------------------------------------------------------

void* WINAPI
CLineServices::NewPtr(DWORD cb)
{
    void * p;

    p = MemAlloc( Mt(LineServicesMem), cb );

    MemSetName((p, "CLineServices::NewPtr"));

    return p;
}

//-----------------------------------------------------------------------------
//
//  Function:   DisposePtr (member, LS callback)
//
//  Synopsis:   A client-side 'free' routine for LineServices
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------

void  WINAPI
CLineServices::DisposePtr(void* p)
{
    MemFree(p);
}

//-----------------------------------------------------------------------------
//
//  Function:   ReallocPtr (member, LS callback)
//
//  Synopsis:   A client-side reallocation routine for LineServices
//
//  Returns:    Pointer to new buffer, or NULL if out of memory
//
//-----------------------------------------------------------------------------

void* WINAPI
CLineServices::ReallocPtr(void* p, DWORD cb)
{
    void * q = p;
    HRESULT hr;

    hr = MemRealloc( Mt(LineServicesMem), &q, cb );

    return hr ? NULL : q;
}

LSERR WINAPI
CLineServices::GleanInfoFromTheRun(COneRun *por, COneRun **pporOut)
{
    LSERR         lserr = lserrNone;
    const         CCharFormat *pCF;
    BOOL          fWasTextRun = TRUE;
    SYNTHTYPE     synthCur = SYNTHTYPE_NONE;
    LONG          cp = por->Cp();
    LONG          nDirLevel;
    LONG          nSynthDirLevel;
    COneRun     * porOut = por;
    BOOL          fLastPtp = por->_ptp == _treeInfo._ptpLayoutLast;
    BOOL          fNodeRun = por->_ptp->IsNode();
    CTreeNode   * pNodeRun = fNodeRun ? por->_ptp->Branch() : NULL;
    
    por->_fHidden = FALSE;
    porOut->_fNoTextMetrics = FALSE;

    if (   _fIsEditable
        && !fLastPtp
        && fNodeRun
        && !por->_fSynthedForEditGlyph
        && por->_ptp->ShowTreePos()
       )
    {
        lserr = AppendSynth(por, SYNTHTYPE_GLYPH, &porOut);
        if (lserr == lserrNone && porOut)
        {
            porOut->_lsCharProps.idObj = LSOBJID_GLYPH;
            lserr = SetRenderingHighlights(porOut);
            if (lserr != lserrNone)
                goto Done;
            por->_fSynthedForEditGlyph = TRUE;
            _lineFlags.AddLineFlagForce(cp -1, FLAG_HAS_NOBLAST);
        }
        goto Done;
    }
    
    if (   _pMeasurer->_fMeasureFromTheStart
        && (cp - _cpStart) < _pMeasurer->_cchPreChars
       )
    {
        WhiteAtBOL(cp);
        por->MakeRunAntiSynthetic();
        goto Done;
    }

    //
    // Take care of hidden runs. We will simply anti-synth them
    //
    if (   por->_fCharsForNestedElement
        && por->GetCF()->IsDisplayNone()
       )
    {
        Assert(!_fIsEditable);
        if (IsFirstNonWhiteOnLine(cp))
            WhiteAtBOL(cp, por->_lscch);
        _lineCounts.AddLineCount(cp, LC_HIDDEN, por->_lscch);
        por->MakeRunAntiSynthetic();
        goto Done;
    }

    BEGINSUPPRESSFORQUILL
    // BR with clear causes a break after the line break,
    // where as clear on phrase or block elements should clear
    // before the phrase or block element comes into scope.
    // Clear on aligned elements is handled separately, so
    // do not mark the line with clear flags for aligned layouts.
    if (   cp != _cpStart
        && fNodeRun
        && pNodeRun->Tag() != ETAG_BR
        && !por->GetFF()->_fAlignedLayout
        && _pMeasurer->TestForClear(_pMarginInfo, cp - 1, TRUE, por->GetFF())
       )
    {
        lserr = TerminateLine(por, TL_ADDEOS, &porOut);
        Assert(lserr != lserrNone || porOut->_synthType != SYNTHTYPE_NONE);
        goto Cleanup;
    }
    ENDSUPPRESSFORQUILL
            
    if (   !por->_fCharsForNestedLayout
        && fNodeRun
       )
    {
        CElement  *pElement          = pNodeRun->Element();
        BOOL       fFirstOnLineInPre = FALSE;
        const      CCharFormat *pCF  = por->GetCF();

        // This run can never be hidden, no matter what the HTML says
        Assert(!por->_fHidden);

        if (!fLastPtp && IsFirstNonWhiteOnLine(cp))
        {
            WhiteAtBOL(cp);

            //
            // If we have a <BR> inside a PRE tag then we do not want to
            // break if the <BR> is the first thing on this line. This is
            // because there should have been a \r before the <BR> which
            // caused one line break and we should not have the <BR> break
            // another line. The only execption is when the <BR> is the
            // first thing in the paragraph. In this case we *do* want to
            // break (the exception was discovered via bug 47870).
            //
            if (   _pPFFirst->HasPre(_fInnerPFFirst)
                && !(_lsMode == LSMODE_MEASURER ? _li._fFirstInPara : _pli->_fFirstInPara)
               )
            {
                fFirstOnLineInPre = TRUE;
            }
        }

        if (   por->_ptp->IsEdgeScope()
            && (   _pFlowLayout->IsElementBlockInContext(pElement)
                || fLastPtp
               )
            && (   !fFirstOnLineInPre
                || (   por->_ptp->IsEndElementScope()
                    && pElement->_fBreakOnEmpty
                    && pElement->Tag() == ETAG_PRE
                   )
               )
            && pElement->Tag() != ETAG_BR
           )
        {
#if 0            
            Assert(   fLastPtp
                   || !IsFirstNonWhiteOnLine(cp)
                   || pElement->_fBreakOnEmpty
                   || (ETAG_LI == pElement->Tag())
                  );
#endif
            
            lserr = TerminateLine(por,
                                  ((fLastPtp || por->_fSynthedForEditGlyph) ? TL_ADDEOS : TL_ADDLBREAK),
                                  &porOut);
            if (lserr != lserrNone || !porOut)
            {
                lserr = lserrOutOfMemory;
                goto Done;
            }

            // Hide the run that contains the WCH_NODE for the end
            // edge in the layout
            por->_fHidden = TRUE;

            if (fLastPtp)
            {
                if (   IsFirstNonWhiteOnLine(cp)
                    && (   _fIsEditable
                        || _pFlowLayout->GetContentTextLength() == 0
                       )
                    )
                {
                    BEGINSUPPRESSFORQUILL
                    CCcs *pccs = GetCcs(por, _pci->_hdc, _pci);
                    if (pccs)
                        RecalcLineHeight(pccs, &_li);
                    ENDSUPPRESSFORQUILL

                    // This line is not a dummy line. It has a height. The
                    // code later will treat it as a dummy line. Prevent
                    // that from happening.
                    _fLineWithHeight = TRUE;
                }
                goto Done;
            }

            //
            // Bug66768: If we came here at BOL without the _fHasBulletOrNum flag being set, it means
            // that the previous line had a <BR> and we will terminate this line at the </li> so that
            // all it contains is the </li>. In this case we do infact want the line to have a height
            // so users can type there.
            //
            else if (   ETAG_LI == pElement->Tag()
                     && por->_ptp->IsEndNode()
                     && !_li._fHasBulletOrNum
                     && IsFirstNonWhiteOnLine(cp)
                    )
            {
                CCcs *pccs = GetCcs(por, _pci->_hdc, _pci);
                if (pccs)
                    RecalcLineHeight(pccs, &_li);
                _fLineWithHeight = TRUE;
            }
        }
        else  if (   por->_ptp->IsEndNode()
                  && pElement->Tag() == ETAG_BR
                  && !fFirstOnLineInPre
                 )
        {
            _lineFlags.AddLineFlag(cp - 1, FLAG_HAS_A_BR);
            AssertSz(por->_ptp->IsEndElementScope(), "BR's cannot be proxied!");
            Assert(por->_lscch == 1);
            Assert(por->_lscchOriginal == 1);

            lserr = TerminateLine(por, TL_ADDNONE, &porOut);
            if (lserr != lserrNone)
                goto Done;
            if (!porOut)
                porOut = por;

            por->FillSynthData(SYNTHTYPE_LINEBREAK);
            _pMeasurer->TestForClear(_pMarginInfo, cp, FALSE, por->GetFF());
            if (IsFirstNonWhiteOnLine(cp))
                _fLineWithHeight = TRUE;
        }
        //
        // Handle premature Ruby end here
        // Example: <RUBY>some text</RUBY>
        //
        else if (   pElement->Tag() == ETAG_RUBY
                 && por->_ptp->IsEndNode()       // this is an end ruby tag
                 && _fIsRuby                     // and we currently have an open ruby
                 && !_fIsRubyText                // and we have not yet closed it off
                 && !IsFrozen())
        {
            COneRun *porTemp = NULL;
            Assert(por->_lscch == 1);
            Assert(por->_lscchOriginal == 1);

            // if we got here then we opened a ruby but never ended the main
            // text (i.e., with an RT).  Now we want to end everything, so this
            // involves first appending a synthetic to end the main text and then
            // another one to end the (nonexistent) pronunciation text.
            lserr = AppendILSControlChar(por, SYNTHTYPE_ENDRUBYMAIN, &porOut);
            Assert(lserr != lserrNone || porOut->_synthType != SYNTHTYPE_NONE);
            lserr = AppendILSControlChar(por, SYNTHTYPE_ENDRUBYTEXT, &porTemp);
            Assert(lserr != lserrNone || porTemp->_synthType != SYNTHTYPE_NONE);

            // We set this to FALSE because por will eventually be marked as
            // "not processed yet", which means that the above condition will trip
            // again unless we indicate that the ruby is now closed
            _fIsRuby = FALSE;
        }
        else if (   _fNoBreakForMeasurer
                 && por->_ptp->IsEndNode()
                 && (   pElement->Tag() == ETAG_NOBR
                     || pElement->Tag() == ETAG_WBR
                    )
                 && !IsFrozen()
                )
        {
            //
            // NOTES on WBR handling: By ending the NOBR ILS obj here, we create a
            // break oportunity.  When fetchrun gets called again, the check below
            // will see (from the pcf) that the text still can't break, but that
            // it's not within an ILS pfnFmt, so it will just start a new one. Also,
            // since wbr is in the story, we can just substitute it, and don't need
            // to create a synthetic char.
            //
            // BUGBUG (mikejoch) This will cause grief if the NOBR is overlapped with
            // a reverse object. In fact there are numerous problems mixing NOBRs
            // with reverse objects; this will need to be addressed separately.
            // FUTURE (mikejoch) It would be a good idea to actually write a real word
            // breaking function for NOBRs instead of terminating and restarting the
            // object like this. Doing so would get rid of this elseif, reduce the
            // synthetic store, and generally be clearer.
            //
            if (pElement->Tag() == ETAG_WBR)
                _lineFlags.AddLineFlag(cp, FLAG_HAS_EMBED_OR_WBR);

            por->FillSynthData(SYNTHTYPE_ENDNOBR);
        }
        else
        {
            //
            // A normal phrase element start or end run. Just make
            // it antisynth so that it is hidden from LS
            //
            por->MakeRunAntiSynthetic();
        }

        if (!por->IsAntiSyntheticRun())
        {
            //
            // Empty lines will need some height!
            //
            if (pElement->_fBreakOnEmpty)
            {
                if (IsFirstNonWhiteOnLine(cp))
                {
                    // We provide a line height only if something in our whitespace
                    // is not providing some visual representation of height. So if our
                    // whitespace was either aligned or abspos'd then we do NOT provide
                    // any height since these sites provide some height of their own.
                    if (   _lineCounts.GetLineCount(por->Cp(), LC_ALIGNEDSITES) == 0
                        && _lineCounts.GetLineCount(por->Cp(), LC_ABSOLUTESITES) == 0
                       )
                    {
                        CCcs *pccs = GetCcs(por, _pci->_hdc, _pci);
                        if (pccs)
                            RecalcLineHeight(pccs, &_li);
                        _fLineWithHeight = TRUE;
                    }
                }
            }

            //
            // If we have already decided to give the line a height then we want
            // to get the text metrics else we do not want the text metrics. The
            // reasons are explained in the blurb below.
            //
            if (!_fLineWithHeight)
            {
                //
                // If we have not anti-synth'd the run, it means that we have terminated
                // the line -- either due to a block element or due to a BR element. In either
                // of these 2 cases, if the element did not have break on empty, then we
                // do not want any of them to induce a descent. If it did have a break
                // on empty then we have already computed the heights, so there is no
                // need to do so again.
                //
                por->_fNoTextMetrics = TRUE;
            }
        }

        if (pCF->IsRelative(por->_fInnerCF))
        {
            _lineFlags.AddLineFlag(cp, FLAG_HAS_RELATIVE);
        }

        // Set the flag on the line if the current pcf has a background
        // image or color
        if(pCF->HasBgImage(por->_fInnerCF) || pCF->HasBgColor(por->_fInnerCF))
        {
            //
            // NOTE(SujalP): If _cpStart has a background, and nothing else ends up
            // on the line, then too we want to draw the background. But since the
            // line is empty cpMost == _cpStart and hence GetLineFlags will not
            // find this flag. To make sure that it does, we subtract 1 from the cp.
            // (Bug  43714).
            //
            (cp == _cpStart)
                    ? _lineFlags.AddLineFlagForce(cp - 1, FLAG_HAS_BACKGROUND)
                    : _lineFlags.AddLineFlag(cp, FLAG_HAS_BACKGROUND);
        }

        BEGINSUPPRESSFORQUILL
        if (pCF->_fBidiEmbed && _pBidiLine == NULL && !IsFrozen())
        {
            _pBidiLine = new CBidiLine(_treeInfo, _cpStart, _li._fRTL, _pli);
            Assert(GetDirLevel(por->_lscpBase) == 0);
        }
        ENDSUPPRESSFORQUILL

        // Some cases here which we need to let thru for the orther glean code
        // to look at........

        goto Done;
    }

    //
    // Figure out CHP, the layout info.
    //
    pCF = IsAdornment() ? _pCFLi : por->GetCF();
    Assert(pCF);

    if (pCF->IsVisibilityHidden())
    {
        _lineFlags.AddLineFlag(cp, FLAG_HAS_NOBLAST);
    }
    
    BEGINSUPPRESSFORQUILL
    // If we've transitioned directions, begin or end a reverse object.
    if (_pBidiLine != NULL &&
        (nDirLevel = _pBidiLine->GetLevel(cp)) !=
        (nSynthDirLevel = GetDirLevel(por->_lscpBase)))
    {
        if (!IsFrozen())
        {
            // Determine the type of synthetic character to add.
            if (nDirLevel > nSynthDirLevel)
            {
                synthCur = SYNTHTYPE_REVERSE;
            }
            else
            {
                synthCur = SYNTHTYPE_ENDREVERSE;
            }

            // Add the new synthetic character.
            lserr = AppendILSControlChar(por, synthCur, &porOut);
            Assert(lserr != lserrNone || porOut->_synthType != SYNTHTYPE_NONE);
            goto Cleanup;
        }
    }
    else if(pCF->_bSpecialObjectFlagsVar != _bSpecialObjectFlagsVar)
    {
        if(!IsFrozen())
        {
            if(CheckForSpecialObjectBoundaries(por, &porOut))
                goto Cleanup;
        }
    }
    ENDSUPPRESSFORQUILL

    CHPFromCF( por, pCF );

    por->_brkopt = (pCF->_fLineBreakStrict ? fBrkStrict : 0) |
                   (pCF->_fNarrow ? 0 : fCscWide);

    // Note the relative stuff
    if (    !por->_fCharsForNestedLayout
        &&  pCF->IsRelative(por->_fInnerCF))
    {
        _lineFlags.AddLineFlag(cp, FLAG_HAS_RELATIVE);
    }

    // Set the flag on the line if the current pcf has a background
    // image or color
    if(pCF->HasBgImage(por->_fInnerCF) || pCF->HasBgColor(por->_fInnerCF))
    {
        _lineFlags.AddLineFlag(cp, FLAG_HAS_BACKGROUND);
    }

    if (!por->_fCharsForNestedLayout)
    {
        const TCHAR chFirst = por->_pchBase[0];

        //
        // Currently the only nested elements we have other than layouts are hidden
        // elements. These are taken care of before we get here, so we should
        // never be here with this flag on.
        //
        Assert(!por->_fCharsForNestedElement);
        
        // A regular text run.

        // Arye: Might be in edit mode on last empty line and trying
        // to measure. Need to fail gracefully.
        // AssertSz( *ppwchRun, "Expected some text.");

        //
        // Begin nasty exception code to deal with all the possible
        // special characters in the run.
        //

        Assert(pCF != NULL);
        BEGINSUPPRESSFORQUILL
        if (_pBidiLine == NULL && (IsRTLChar(chFirst) || pCF->_fBidiEmbed))
        {
            if (!IsFrozen())
            {
                _pBidiLine = new CBidiLine(_treeInfo, _cpStart, _li._fRTL, _pli);
                Assert(GetDirLevel(por->_lscpBase) == 0);

                if (_pBidiLine != NULL && _pBidiLine->GetLevel(cp) > 0)
                {
                    synthCur = SYNTHTYPE_REVERSE;
                    // Add the new synthetic character.
                    lserr = AppendILSControlChar(por, synthCur, &porOut);
                    Assert(lserr != lserrNone || porOut->_synthType != SYNTHTYPE_NONE);
                    goto Cleanup;
                }
            }
        }
        ENDSUPPRESSFORQUILL

        // Don't blast disabled lines.
        if (pCF->_fDisabled)
        {
            _lineFlags.AddLineFlag(cp, FLAG_HAS_NOBLAST);
        }

        // NB (cthrash) If an NBSP character is at the beginning of a
        // text run, LS will convert that to a space before calling
        // GetRunCharWidths.  This implies that we will fail to recognize
        // the presence of NBSP chars if we only check at GRCW. So an
        // additional check is required here.  Similarly, if our 

        if ( chFirst == WCH_NBSP )
        {
            _lineFlags.AddLineFlag(cp, FLAG_HAS_NBSP);
        }
        
        lserr = ChunkifyTextRun(por, &porOut);
        if (lserr != lserrNone)
            goto Cleanup;

        if (porOut != por)
            goto Cleanup;
    }
    else
    {
        //
        // It has to be some layout other than the layout we are measuring
        //
        Assert(   por->Branch()->GetUpdatedLayout()
               && por->Branch()->GetUpdatedLayout() != _pFlowLayout
              );

#if DBG == 1
        CElement *pElementLayout = por->Branch()->GetUpdatedLayout()->ElementOwner();
        long cpElemStart = pElementLayout->GetFirstCp();

        // Count the characters in this site, so LS can skip over them on this line.
        Assert(por->_lscch == GetNestedElementCch(pElementLayout));
#endif

        _fHasSites = _fMinMaxPass;
        
        // We check if this site belongs on its own line.
        // If so, we terminate this line with an EOS marker.
        if (IsOwnLineSite(por))
        {
            // This guy belongs on his own line.  But we only have to terminate the
            // current line if he's not the first thing on this line.
            if (cp != _cpStart)
            {
                Assert(!por->_fHidden);

                // We're not first on line.  Terminate this line!
                lserr = TerminateLine(por, TL_ADDEOS, &porOut);
                Assert(lserr != lserrNone || porOut->_synthType != SYNTHTYPE_NONE);
                goto Cleanup;
            }
            // else we are first on line, so even though this guy needs to be
            // on his own line, keep going, because he is!
        }

        // If we kept looking after a single line site in _fScanForCR and we
        // came here, it means that we have another site after the single site and
        // hence should terminate the line
        else if (   _fScanForCR
                 && _fSingleSite
                )
        {
            lserr = TerminateLine(por, TL_ADDEOS, &porOut);
            goto Cleanup;
        }

        // Whatever this is, it is under a different site, so we have
        // to give LS an embedded object notice, and later recurse back
        // to format this other site.  For tables and spans and such, we have
        // to count and skip over the characters in this site.
        por->_lsCharProps.idObj = LSOBJID_EMBEDDED;

        Assert(cp == cpElemStart - 1);

        // ppwchRun shouldn't matter for a LSOBJID_EMBEDDED, but chunkify
        // objectrun might modify for putting in the symbols for aligned and
        // abspos'd sites
        fWasTextRun = FALSE;
        lserr = ChunkifyObjectRun(por, &porOut);
        if (lserr != lserrNone)
            goto Cleanup;

        por = porOut;
    }

    lserr = SetRenderingHighlights(por);
    if (lserr != lserrNone)
        goto Cleanup;

    if (fWasTextRun && !por->_fHidden)
    {
        lserr = CheckForUnderLine(por);
        if (lserr != lserrNone)
            goto Cleanup;

        if (_chPassword)
        {
            lserr = CheckForPassword(por);
            if (lserr != lserrNone)
                goto Cleanup;
        }
        else
        {
            lserr = CheckForTransform(por);
            if (lserr != lserrNone)
                goto Cleanup;
        }
    }

Cleanup:
    // Some characters we don't want contributing to the line height.
    // Non-textual runs shouldn't, don't bother for hidden runs either.
    //
    // ALSO, we want text metrics if we had a BR or block element
    // which was not break on empty
    porOut->_fNoTextMetrics |=   porOut->_fHidden
                              || porOut->_lsCharProps.idObj != LSOBJID_TEXT;

Done:
    *pporOut = porOut;
    return lserr;
}


BOOL
IsPreLikeNode(CTreeNode * pNode)
{
    ELEMENT_TAG eTag = pNode->Tag();
    return eTag == ETAG_PRE || eTag == ETAG_XMP || eTag == ETAG_PLAINTEXT || eTag == ETAG_LISTING;
}

//-----------------------------------------------------------------------------
//
//  Function:   CheckForSpecialObjectBoundaries
//
//  Synopsis:   This function checks to see whether special objects should
//              be opened or closed based on the CF properties
//
//  Returns:    The one run to submit to line services via the pporOut
//              parameter.  Will only change this if necessary, and will
//              return TRUE if that parameter changed.
//
//-----------------------------------------------------------------------------

BOOL  WINAPI
CLineServices::CheckForSpecialObjectBoundaries(
    COneRun *por,
    COneRun **pporOut)
{
    BOOL fRet = FALSE;
    LSERR lserr;
    const CCharFormat *pCF = por->GetCF();
    Assert(pCF);

    if(pCF->_fIsRuby && !_fIsRuby)
    {
        Assert(!_fIsRubyText);

        // Open up a new ruby here (we only enter here in the open ruby case,
        // the ruby object is closed when we see an /RT)
        _fIsRuby = TRUE;
#ifdef RUBY_OVERHANG
        // We set this flag here so that LS will try to do modify the width of
        // run, which will trigger a call to FetchRubyWidthAdjust
        por->_lsCharProps.fModWidthOnRun = TRUE;
#endif
        _yMaxHeightForRubyBase = 0;
        _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST);
        _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_RUBY);
        lserr = AppendILSControlChar(por, SYNTHTYPE_RUBYMAIN, pporOut);
        Assert(lserr != lserrNone || (*pporOut)->_synthType != SYNTHTYPE_NONE);
        fRet = TRUE;
    }
    else if(pCF->_fIsRubyText != _fIsRubyText)
    {
        Assert(_fIsRuby);

        // if _fIsRubyText is true, that means we have now arrived at text that
        // is no longer Ruby Text.  So, we should close the Ruby object by passing
        // ENDRUBYTEXT to Line Services
        if(_fIsRubyText)
        {
           _fIsRubyText = FALSE;
           _fIsRuby = FALSE;
           lserr = AppendILSControlChar(por, SYNTHTYPE_ENDRUBYTEXT, pporOut);
           Assert(lserr != lserrNone || (*pporOut)->_synthType != SYNTHTYPE_NONE);
           fRet = TRUE;
        }
        // if _fIsRubyText is false, that means that we are now entering text that
        // is Ruby text.  So, we must tell Line Services that we are no longer
        // giving it main text.
        else
        {
            _fIsRubyText = TRUE;
            lserr = AppendILSControlChar(por, SYNTHTYPE_ENDRUBYMAIN, pporOut);
            Assert(lserr != lserrNone || (*pporOut)->_synthType != SYNTHTYPE_NONE);
            fRet = TRUE;
        }
    }
    else
    {
        BOOL fNoBreak = pCF->HasNoBreak(por->_fInnerCF);
        if (fNoBreak != !!_fNoBreakForMeasurer)
        {
            Assert(!IsFrozen());

            //
            // phrase elements inside PRE's which have layout will not have the HasPre bit turned
            // on and hence we will still start a NOBR object for them. The problem then is that
            // we need to terminate the line for '\r'. To do this we will have to scan the text.
            // To minimize the scanning we find out if we are really in such a situation.
            //
            if (_pMarkup->SearchBranchForCriteriaInStory(por->Branch(), IsPreLikeNode))
            {
                _fScanForCR = TRUE;
                goto Cleanup;
            }

            if (!IsOwnLineSite(por))
            {
                // Begin or end NOBR block.
                // BUGBUG (paulpark) this won't work with nearby-nested bidi's and nobr's because
                // the por's we cache in _arySynth are not accurately postioned.
                lserr = AppendILSControlChar(por,
                                             (fNoBreak ? SYNTHTYPE_NOBR : SYNTHTYPE_ENDNOBR),
                                             pporOut);
                Assert(lserr != lserrNone || (*pporOut)->_synthType != SYNTHTYPE_NONE);
                fRet = TRUE;
            }
        }
    }

Cleanup:    
    return fRet;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetAutoNumberInfo (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetAutoNumberInfo(
    LSKALIGN* plskalAnm,    // OUT
    PLSCHP plsChpAnm,       // OUT
    PLSRUN* pplsrunAnm,     // OUT                                  
    WCHAR* pwchAdd,         // OUT
    PLSCHP plsChpWch,       // OUT                                 
    PLSRUN* pplsrunWch,     // OUT                                  
    BOOL* pfWord95Model,    // OUT
    long* pduaSpaceAnm,     // OUT
    long* pduaWidthAnm)     // OUT
{
    LSTRACE(GetAutoNumberInfo);
    LSNOTIMPL(GetAutoNumberInfo);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetNumericSeparators (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetNumericSeparators(
    PLSRUN plsrun,          // IN
    WCHAR* pwchDecimal,     // OUT
    WCHAR* pwchThousands)   // OUT
{
    LSTRACE(GetNumericSeparators);

    // BUGBUG (cthrash) Should set based on locale.

    *pwchDecimal = L'.';
    *pwchThousands = L',';

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   CheckForDigit (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::CheckForDigit(
    PLSRUN plsrun,      // IN
    WCHAR wch,          // IN
    BOOL* pfIsDigit)    // OUT
{
    LSTRACE(CheckForDigit);

    // BUGBUG (mikejoch) IsCharDigit() doesn't check for international numbers.

    *pfIsDigit = IsCharDigit(wch);

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FetchPap (member, LS callback)
//
//  Synopsis:   Callback to fetch paragraph properties for the current line.
//
//  Returns:    lserrNone
//              lserrOutOutMemory
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FetchPap(
    LSCP   lscp,        // IN
    PLSPAP pap)         // OUT
{
    LSTRACE(FetchPap);
    LSERR      lserr = lserrNone;
    const      CParaFormat *pPF;
    CTreePos  *ptp;
    CTreeNode *pNode;
    LONG       cp;
    BOOL       fInnerPF;
    CComplexRun *pcr = NULL;
    CElement  *pElementFL = _pFlowLayout->ElementContent();

    Assert(lscp <= _treeInfo._lscpFrontier);

    if (lscp < _treeInfo._lscpFrontier)
    {
        COneRun *por = FindOneRun(lscp);
        Assert(por);
        if(!por)
            goto Cleanup;
        ptp = por->_ptp;
        pPF = por->_pPF;
        fInnerPF = por->_fInnerPF;
        cp = por->Cp();
        pcr = por->GetComplexRun();
    }
    else
    {
        //
        // The problem is that we are at the end of the list
        // and hence we cannot find the interesting one-run. In this
        // case we have to use the frontier information. However,
        // the frontier information maybe exhausted, so we need to
        // refresh it by calling AdvanceTreePos() here.
        //
        if (!_treeInfo._cchRemainingInTreePos && !_treeInfo._fHasNestedElement)
        {
            if (!_treeInfo.AdvanceTreePos())
            {
                lserr = lserrOutOfMemory;
                goto Cleanup;
            }
        }
        ptp = _treeInfo._ptpFrontier;
        pPF = _treeInfo._pPF; // should we use CLineServices::_pPFFirst here???
        fInnerPF = _treeInfo._fInnerPF;
        cp  = _treeInfo._cpFrontier;
    }

    Assert(ptp);
    Assert(pPF);

    //
    // Set up paragraph properties
    //
    PAPFromPF (pap, pPF, fInnerPF, pcr);

    pap->cpFirst = cp;

    // BUGBUG SLOWBRANCH: GetBranch is **way** too slow to be used here.
    pNode = ptp->GetBranch();
    if (pNode->Element() == pElementFL)
        pap->cpFirstContent = _treeInfo._cpLayoutFirst;
    else
    {
        CTreeNode *pNodeBlock = _treeInfo._pMarkup->SearchBranchForBlockElement(pNode, _pFlowLayout);
        if (pNodeBlock)
        {
            CElement *pElementPara = pNodeBlock->Element();
            pap->cpFirstContent = (pElementPara == pElementFL) ?
                                  _treeInfo._cpLayoutFirst :
                                  pElementPara->GetFirstCp();
        }
        else
            pap->cpFirstContent = _treeInfo._cpLayoutFirst;
    }

Cleanup:
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   FetchTabs (member, LS callback)
//
//  Synopsis:   Callback to return tab positions for the current line.
//
//              LineServices calls the callback when it encounters a tab in
//              the line, but does not pass the plsrun.  The cp is supposed to
//              be used to locate the paragraph.
//
//              Instead of allocating a buffer for the return value, we return
//              a table that resides on the CLineServices object.  The tab
//              values are in twips.
//
//  Returns:
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FetchTabs(
    LSCP lscp,                      // IN
    PLSTABS plstabs,                // OUT
    BOOL* pfHangingTab,             // OUT
    long* pduaHangingTab,           // OUT
    WCHAR* pwchHangingTabLeader )   // OUT
{
    LSTRACE(FetchTabs);

    LONG cTab = _pPFFirst->GetTabCount(_fInnerPFFirst);

    // This tab might end up on the current line, so we can't blast.

    _fSawATab = TRUE;

    // Note: lDefaultTab is a constant defined in textedit.h

    plstabs->duaIncrementalTab = lDefaultTab;

    // BUGBUG (cthrash) What to do for hanging tabs? CSS? Auto?

    *pfHangingTab = FALSE;
    *pduaHangingTab = 0;
    *pwchHangingTabLeader = 0;

    AssertSz(cTab >= 0 && cTab <= MAX_TAB_STOPS, "illegal tab count");

    if (!_pPFFirst->HasTabStops(_fInnerPFFirst) && cTab < 2)
    {
        if (cTab == 1)
        {
            plstabs->duaIncrementalTab = _pPFFirst->GetTabPos(_pPFFirst->_rgxTabs[0]);
        }

        plstabs->iTabUserDefMac = 0;
        plstabs->pTab = NULL;
    }
    else
    {
        LSTBD * plstbd = _alstbd + cTab - 1;

        while (cTab)
        {
            long uaPos;
            long lAlign;
            long lLeader;

            THR( _pPFFirst->GetTab( --cTab, &uaPos, &lAlign, &lLeader ) );

            Assert( lAlign >= 0 && lAlign < tomAlignBar &&
                    lLeader >= 0 && lLeader < tomLines );

            // NB (cthrash) To ensure that the LSKTAB cast is safe, we
            // verify that that define's haven't changed values in
            // CLineServices::InitTimeSanityCheck().

            plstbd->lskt = LSKTAB(lAlign);
            plstbd->ua = uaPos;
            plstbd->wchTabLeader = s_achTabLeader[lLeader];
            plstbd--;

        }

        plstabs->iTabUserDefMac = cTab;
        plstabs->pTab = _alstbd;
    }

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetBreakThroughTab (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetBreakThroughTab(
    long uaRightMargin,         // IN
    long uaTabPos,              // IN
    long* puaRightMarginNew)    // OUT
{
    LSTRACE(GetBreakThroughTab);
    LSNOTIMPL(GetBreakThroughTab);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   CheckParaBoundaries (member, LS callback)
//
//  Synopsis:   Callback to determine whether two cp's reside in different
//              paragraphs (block elements in HTML terms).
//
//  Returns:    lserrNone
//              *pfChanged - TRUE if cp's are in different block elements.
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::CheckParaBoundaries(
    LSCP lscpOld,       // IN
    LSCP lscpNew,       // IN
    BOOL* pfChanged)    // OUT
{
    LSTRACE(CheckParaBoundaries);
    *pfChanged = FALSE;
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetRunCharWidths (member, LS callback)
//
//  Synopsis:   Callback to return character widths of text in the current run,
//              represented by plsrun.
//
//  Returns:    lserrNone
//              rgDu - array of character widths
//              pduDu - sum of widths in rgDu, upto *plimDu characters
//              plimDu - character count in rgDu
//
//-----------------------------------------------------------------------------

#if DBG != 1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif // DBG != 1

LSERR WINAPI
CLineServices::GetRunCharWidths(
    PLSRUN plsrun,              // IN
    LSDEVICE lsDeviceID,        // IN
    LPCWSTR pwchRun,            // IN
    DWORD cwchRun,              // IN
    long du,                    // IN
    LSTFLOW kTFlow,             // IN
    int* rgDu,                  // OUT
    long* pduDu,                // OUT
    long* plimDu)               // OUT
{
    LSTRACE(GetRunCharWidths);

    LSERR lserr = lserrNone;
    pwchRun = plsrun->_fMakeItASpace ? _T(" ") : pwchRun;
    const WCHAR * pch = pwchRun;
    int * pdu = rgDu;
    int * pduEnd = rgDu + cwchRun;
    long duCumulative = 0;
    LONG cpCurr = plsrun->Cp();
    const CCharFormat *pCF = plsrun->GetCF();
    CCcs *pccs;
    CBaseCcs *pBaseCcs;
    HDC hdc;
    LONG duChar = 0;
    WHEN_DBG(LONG xLetterSpacingRemovedDbg = 0);

    Assert( cwchRun );

    pccs = GetCcs(plsrun, _pci->_hdc, _pci);
    if (!pccs)
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }
    hdc = pccs->GetHDC();
    pBaseCcs = pccs->GetBaseCcs();
    
    //
    // CSS
    //
    Assert(IsAdornment() || !plsrun->_fCharsForNestedElement);

    //
    // Add up the widths
    //
    while (pdu < pduEnd)
    {
        // The tight inner loop.
        TCHAR ch = *pch++;

        ProcessOneChar(cpCurr, pdu, rgDu, ch);
        
        Verify( pBaseCcs->Include( hdc, ch, duChar ) );
        
        duCumulative += *pdu++ = duChar;

        if (duCumulative > du)
            break;
    }

    if (GetLetterSpacing(pCF) || pCF->HasCharGrid(TRUE))
    {
        lserr = GetRunCharWidthsEx(plsrun, pwchRun, cwchRun,
                                   du, rgDu, pduDu, plimDu
#if DBG==1
                                   , &xLetterSpacingRemovedDbg
#endif
                                  );
    }
    else
    {
        *pduDu = duCumulative;
        *plimDu = pdu - rgDu;
    }
    
#if DBG == 1
    if (lserr == lserrNone)
    {
        pccs = GetCcs(plsrun, _pci->_hdc, _pci);
        if (!pccs)
        {
            lserr = lserrOutOfMemory;
            goto Cleanup;
        }
        
        // If we are laying out a character grid, skip all the debug verification
        if(pCF->HasCharGrid(TRUE))
            goto Cleanup;

        //
        // In debug mode, confirm that all the fast tweaks give the same
        // answer as the really old slow way.
        //

        int* rgDuDbg= new int[ cwchRun ];  // no meters in debug mode.
        long limDuDbg;  // i.e. *plimDu
        DWORD cchDbg = cwchRun;
        const WCHAR * pchDbg = pwchRun;
        int * pduDbg = rgDuDbg;
        long duCumulativeDbg = 0;
        LONG cpCurrDbg = plsrun->Cp();
        int xLetterSpacing = GetLetterSpacing(pCF);

        if (rgDuDbg != NULL)
        {
          while (cchDbg--)
          {
              long duCharDbg = 0;
              TCHAR chDbg = *pchDbg++;

              pccs->Include( chDbg, duCharDbg );

              duCharDbg += xLetterSpacing;
              *pduDbg++ = duCharDbg;
              duCumulativeDbg += duCharDbg;

              if (duCumulativeDbg > du)
                  break;

              cpCurrDbg++;
          }

          *(pduDbg - 1)   -= xLetterSpacingRemovedDbg;
          duCumulativeDbg -= xLetterSpacingRemovedDbg;

          // *pduDuDbg = duCumulativeDbg;
          limDuDbg = pchDbg - pwchRun;


          // Calculation done.  Check results.
          Assert( duCumulativeDbg == *pduDu );  // total distance measured.
          Assert( limDuDbg == *plimDu );        // characters measured.
          for(int i=0; i< limDuDbg; i++)
          {
              Assert( rgDu[i] == rgDuDbg[i] );
          }

          delete rgDuDbg;
        }
    }
#endif  // DBG == 1

Cleanup:
    return lserr;
}

#if DBG != 1
#pragma optimize("",on)
#endif // DBG != 1

LSERR
CLineServices::GetRunCharWidthsEx(
    PLSRUN plsrun,
    LPCWSTR pwchRun,
    DWORD cwchRun,
    LONG du,
    int *rgDu,
    long *pduDu,
    long *plimDu
#if DBG==1
    , LONG *pxLetterSpacingRemovedDbg
#endif
)
{
    LONG lserr = lserrNone;
    LONG duCumulative = 0;
    LONG cpCurr = plsrun->Cp();
    int *pdu = rgDu;
    int *pduEnd = rgDu + cwchRun;
    const CCharFormat *pCF = plsrun->GetCF();
    LONG  xLetterSpacing = GetLetterSpacing(pCF);
    CCcs *pccs;
    
    pccs = GetCcs(plsrun, _pci->_hdc, _pci);
    if (!pccs)
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }

    _lineFlags.AddLineFlagForce(cpCurr, FLAG_HAS_NOBLAST);
    WHEN_DBG(_lineFlags.AddLineFlagForce(cpCurr, FLAG_HAS_LETTERSPACING);)

    if (pCF->HasCharGrid(TRUE))
    {
        long lGridSize = GetCharGridSize();

        switch (pCF->GetLayoutGridType(TRUE))
        {
        case styleLayoutGridTypeStrict:
        case styleLayoutGridTypeFixed:
            // we have strict or fixed layout grid for this run
            {
                if (plsrun->IsOneCharPerGridCell())
                {
                    for (pdu = rgDu; pdu < pduEnd;)
                    {
                        if (xLetterSpacing < 0)
                            MeasureCharacter(pwchRun[pdu - rgDu], cpCurr, rgDu, pccs, pdu);
                        *pdu += xLetterSpacing;
                        *pdu = GetClosestGridMultiple(lGridSize, *pdu);
                        duCumulative += *pdu++;
                        if (duCumulative > du)
                            break;
                    }
                }
                else
                {
                    BOOL fLastInChain = FALSE;
                    LONG duOuter = 0;

                    // Get width of this run
                    for (pdu = rgDu; pdu < pduEnd;)
                    {
                        if (xLetterSpacing < 0)
                            MeasureCharacter(pwchRun[pdu - rgDu], cpCurr, rgDu, pccs, pdu);
                       *pdu += xLetterSpacing;
                        duCumulative += *pdu++;
                        if (duCumulative > du)
                        {
                            duOuter = *(pdu-1);
                            duCumulative -= duOuter;
                            fLastInChain = TRUE;
                            break;
                        }
                    }

                    if (!fLastInChain)
                    {
                        // Fetch next run
                        COneRun * pNextRun = 0;
                        LPCWSTR  lpcwstrJunk;
                        DWORD    dwJunk;
                        BOOL     fJunk;
                        LSCHP    lschpJunk;
                        FetchRun(plsrun->_lscpBase + plsrun->_lscch, &lpcwstrJunk, &dwJunk, &fJunk, &lschpJunk, 
                            &pNextRun);
                        fLastInChain = !plsrun->IsSameGridClass(pNextRun);
                    }
                    if (!fLastInChain)
                    {   // we are inside a runs chain with same grid properties
                        plsrun->_xWidth = duCumulative;
                    }
                    else
                    {   // we are at the end of runs chain with same grid properties

                        // Get width of runs chain
                        LONG duChainWidth = duCumulative;
                        COneRun * pFirstRun = plsrun;
                        while (pFirstRun->_pPrev)
                        {
                            while (pFirstRun->_pPrev && !pFirstRun->_pPrev->_ptp->IsText())
                            {
                                pFirstRun = pFirstRun->_pPrev;
                            }
                            if (plsrun->IsSameGridClass(pFirstRun->_pPrev))
                            {
                                duChainWidth += pFirstRun->_pPrev->_xWidth;
                                pFirstRun = pFirstRun->_pPrev;
                            }
                            else
                                break;
                        }

                        // Update width and draw offset of the chain
                        LONG duGrid = GetClosestGridMultiple(lGridSize, duChainWidth);
/*                        if (pCF->HasCharGrid(FALSE))
                        {
                            duGrid = GetClosestGridMultiple(GetCharGridSize(), duGrid);
                        }
*/                        LONG lOffset = (duGrid - duChainWidth)/2;
                        plsrun->_xWidth = duCumulative + duGrid - duChainWidth - lOffset;
                        plsrun->_xDrawOffset = lOffset;
                        COneRun * pPrevRun = plsrun->_pPrev;
                        while (pPrevRun)
                        {
                            while (pPrevRun && !pPrevRun->_ptp->IsText())
                            {
                                pPrevRun = pPrevRun->_pPrev;
                            }
                            if (plsrun->IsSameGridClass(pPrevRun))
                            {
                                pPrevRun->_xDrawOffset = lOffset;
                                pPrevRun = pPrevRun->_pPrev;
                            }
                            else
                                break;
                        }
                        duCumulative += duOuter + duGrid - duChainWidth;
                    }
                }
            }
            break;

        case styleLayoutGridTypeLoose:
        default:
            // we have loose layout grid or layout grid type is not set for this run
            for ( pdu = rgDu; pdu < pduEnd; )
            {
                if (xLetterSpacing < 0)
                    MeasureCharacter(pwchRun[pdu - rgDu], cpCurr, rgDu, pccs, pdu);
                *pdu += xLetterSpacing;
                *pdu += LooseTypeWidthIncrement(pwchRun[pdu - rgDu], (plsrun->_brkopt == fCscWide), lGridSize);
                duCumulative += *pdu++;
                if (duCumulative > du)
                    break;
            }
        }
    }
    else
    {
        //
        // But we must be careful to deal with negative letter spacing,
        // since that will cause us to have to actually get some character
        // widths since the line length will be extended.  If the
        // letterspacing is positive, then we know all the letters we're
        // gonna use have already been measured.
        //
        for ( pdu = rgDu; pdu < pduEnd; )
        {
            if (xLetterSpacing < 0)
                MeasureCharacter(pwchRun[pdu - rgDu], cpCurr, rgDu, pccs, pdu);
            *pdu += xLetterSpacing;
            duCumulative += *pdu++;
            if ( duCumulative > du )
                break;
        }

        if (   xLetterSpacing
            && pdu >= pduEnd
            && !_pCFLi
           )
        {
            LPCWSTR  lpcwstrJunk;
            DWORD    dwJunk;
            BOOL     fJunk;
            LSCHP    lschpJunk;
            COneRun *por;
            int      xLetterSpacingNextRun = 0;

            lserr = FetchRun(plsrun->_lscpBase + plsrun->_lscch,
                             &lpcwstrJunk, &dwJunk, &fJunk, &lschpJunk, &por);

            if (   lserr == lserrNone
                && por->_ptp != _treeInfo._ptpLayoutLast
               )
            {
                xLetterSpacingNextRun = GetLetterSpacing(por->GetCF());
            }
            if (xLetterSpacing != xLetterSpacingNextRun)
            {
                *(pdu - 1)   -= xLetterSpacing;
                duCumulative -= xLetterSpacing;
                WHEN_DBG(*pxLetterSpacingRemovedDbg = xLetterSpacing;)
            }
        }
    }

    *pduDu = duCumulative;
    *plimDu = pdu - rgDu;

Cleanup:
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetLineOrCharGridSize()
//
//  Synopsis:   This function finds out what the height or width of a grid cell 
//              is in pixels, making conversions if necessary.  If this value has 
//              already been calculated, the calculated value is immediately returned
//
//  Returns:    long value of the width of a grid cell in pixels
//
//-----------------------------------------------------------------------------

long WINAPI
CLineServices::GetLineOrCharGridSize(BOOL fGetCharGridSize)
{
    const CUnitValue *pcuv;
    CUnitValue::DIRECTION dir;
    long *lGridSize = fGetCharGridSize ? &_lCharGridSize : &_lLineGridSize;
    
    // If we already have a cached value, return that, or if we haven't set
    // set up the ccs yet return zero
    if(*lGridSize != 0 || !_pPFFirst)
        goto Cleanup;

    pcuv = fGetCharGridSize ? &(_pPFFirst->GetCharGridSize(TRUE)) : &(_pPFFirst->GetLineGridSize(TRUE));
    // The uv should have some value, otherwise we shouldn't even be
    // here making calculations for a non-existent grid.
    switch(pcuv->GetUnitType())
    {
    case CUnitValue::UNIT_NULLVALUE:
        break;

    case CUnitValue::UNIT_ENUM:
        // need to handle "auto" here
        if(pcuv->GetUnitValue() == styleLayoutGridCharAuto && _pccsCache) 
            *lGridSize = fGetCharGridSize ? _pccsCache->GetBaseCcs()->_xMaxCharWidth :
                         _pccsCache->GetBaseCcs()->_yHeight;
        else
            *lGridSize = 0;
        break;

    case CUnitValue::UNIT_PERCENT:
        *lGridSize = ((fGetCharGridSize ? _xWrappingWidth : _pci->_sizeParent.cy) * pcuv->GetUnitValue()) / 10000;
        break;

    default:
        dir = fGetCharGridSize ? CUnitValue::DIRECTION_CX : CUnitValue::DIRECTION_CY;
        *lGridSize = pcuv->GetPixelValue(NULL, dir, fGetCharGridSize ? _xWrappingWidth : _pci->_sizeParent.cy);
        break;
    }

    if (pcuv->IsPercent() && _pFlowLayout && !fGetCharGridSize)
            _pFlowLayout->GetDisplay()->SetVertPercentAttrInfo();

Cleanup:
    return *lGridSize;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetClosestGridMultiple
//
//  Synopsis:   This function just calculates the width of an object in lGridSize
//              multiples.  For example, if lGridSize is 12 and lObjSize is 16,
//              this function would return 24.  If lObjSize is 0, this function
//              will return 0.
//              
//  Returns:    long value of the width in pixels
//
//-----------------------------------------------------------------------------

long WINAPI
CLineServices::GetClosestGridMultiple(long lGridSize, long lObjSize)
{
    long lReturnWidth = lObjSize;
    long lRemainder;
    if(lObjSize == 0 || lGridSize == 0)
        goto Cleanup;

    lRemainder = lObjSize % lGridSize;
    lReturnWidth = lObjSize + lGridSize - (lRemainder ? lRemainder : lGridSize);

Cleanup:
    return lReturnWidth;
}


//-----------------------------------------------------------------------------
//
//  Function:   CheckRunKernability (member, LS callback)
//
//  Synopsis:   Callback to test whether current runs should be kerned.
//
//              We do not support kerning at this time.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::CheckRunKernability(
    PLSRUN plsrunLeft,  // IN
    PLSRUN plsrunRight, // IN
    BOOL* pfKernable)   // OUT
{
    LSTRACE(CheckRunKernability);

    *pfKernable = FALSE;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetRunCharKerning (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetRunCharKerning(
    PLSRUN plsrun,              // IN
    LSDEVICE lsDeviceID,        // IN
    LPCWSTR pwchRun,            // IN
    DWORD cwchRun,              // IN
    LSTFLOW kTFlow,             // IN
    int* rgDu)                  // OUT
{
    LSTRACE(GetRunCharKerning);

    DWORD iwch = cwchRun;
    int *pDu = rgDu;

    while (iwch--)
    {
        *pDu++ = 0;
    }

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetRunTextMetrics (member, LS callback)
//
//  Synopsis:   Callback to return text metrics of the current run
//
//  Returns:    lserrNone
//              plsTxMet - LineServices textmetric structure (lstxm.h)
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetRunTextMetrics(
    PLSRUN   plsrun,            // IN
    LSDEVICE lsDeviceID,        // IN
    LSTFLOW  kTFlow,            // IN
    PLSTXM   plsTxMet)          // OUT
{
    LSTRACE(GetRunTextMetrics);
    const CCharFormat * pCF;
    CCcs *pccs;
    CBaseCcs *pBaseCcs;
    LONG lLineHeight;
    LSERR lserr = lserrNone;
    
    Assert(plsrun);
    
    if (plsrun->_fNoTextMetrics)
    {
        ZeroMemory( plsTxMet, sizeof(LSTXM) );
        goto Cleanup;
    }

    pCF = plsrun->GetCF();
    pccs = GetCcs(plsrun, _pci->_hdc, _pci);
    if (!pccs)
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }
    pBaseCcs = pccs->GetBaseCcs();

    // Cache this in case we do width modification.
    plsTxMet->fMonospaced = pBaseCcs->_fFixPitchFont ? TRUE : FALSE;
    _fHasOverhang |= ((plsrun->_xOverhang = pBaseCcs->_xOverhang) != 0);

    // Keep track of the line heights specified in all the
    // runs so that we can adjust the line height at the end.
    // Note that we don't want to include break character NEVER
    // count towards height.
    lLineHeight = RememberLineHeight(plsrun->Cp(), pCF, pBaseCcs);
    
    if (_fHasSites)
    {
        Assert(_fMinMaxPass);
        plsTxMet->dvAscent = 1;
        plsTxMet->dvDescent = 0;
        plsTxMet->dvMultiLineHeight = 1;
    }
    else
    {
        long dvAscent, dvDescent;
        
        dvDescent = long(pBaseCcs->_yDescent) - pBaseCcs->_yOffset;
        dvAscent  = pBaseCcs->_yHeight - dvDescent;

        plsTxMet->dvAscent = dvAscent;
        plsTxMet->dvDescent = dvDescent;
        plsTxMet->dvMultiLineHeight = lLineHeight;
    }
    
    if (pBaseCcs->_xOverhangAdjust)
    {
        _lineFlags.AddLineFlag(plsrun->Cp(), FLAG_HAS_NOBLAST);
    }

Cleanup:    
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetRunUnderlineInfo (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//  Note(SujalP): Lineservices is a bit wierd. It will *always* want to try to
//  merge runs and that to based on its own algorithm. That algorith however is
//  not the one which IE40 implements. For underlining, IE 40 always has a
//  single underline when we have mixed font sizes. The problem however is that
//  this underline is too far away from the smaller pt text in the mixed size
//  line (however within the dimensions of the line). When we give this to LS,
//  it thinks that the UL is outside the rect of the small character and deems
//  it incorrect and does not call us for a callback. To overcome this problem
//  we tell LS that the UL is a 0 (baseline) but remember the distance ourselves
//  in the PLSRUN.
//
//  Also, since color of the underline can change from run-to-run, we
//  return different underline types to LS so as to prevent it from
//  merging such runs. This also helps avoid merging when we are drawing overlines.
//  Overlines are drawn at different heigths (unlinke underlines) from pt
//  size to pt size. (This probably is a bug -- but this is what IE40 does
//  so lets just go with that for now).
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetRunUnderlineInfo(
    PLSRUN plsrun,          // IN
    PCHEIGHTS heightsPres,  // IN
    LSTFLOW kTFlow,         // IN
    PLSULINFO plsUlInfo)    // OUT
{
    LSTRACE(GetRunUnderlineInfo);
    BYTE  bUnderlineType;
    static BOOL s_fToggleSwitch = FALSE;

    if (!plsrun->_dwImeHighlight)
    {
        bUnderlineType = CFU_CF1UNDERLINE;
    }
    else
    {
        // BUGBUG (cthrash) We need to switch between dotted and solid
        // underlining when the text goes from unconverted to converted.

        bUnderlineType = plsrun->_fUnderlineForIME ? CFU_UNDERLINEDOTTED : 0;
    }

    plsUlInfo->kulbase = bUnderlineType | (s_fToggleSwitch ? CFU_SWITCHSTYLE : 0);
    s_fToggleSwitch = !s_fToggleSwitch;

    plsUlInfo->cNumberOfLines = 1;
    plsUlInfo->dvpUnderlineOriginOffset = 0;
    plsUlInfo->dvpFirstUnderlineOffset = 0;
    plsUlInfo->dvpFirstUnderlineSize = 1;
    plsUlInfo->dvpGapBetweenLines = 0;
    plsUlInfo->dvpSecondUnderlineSize = 0;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetRunStrikethroughInfo (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetRunStrikethroughInfo(
    PLSRUN plsrun,          // IN
    PCHEIGHTS heightPres,   // IN
    LSTFLOW kTFlow,         // IN
    PLSSTINFO plsStInfo)    // OUT
{
    LSTRACE(GetRunStrikethroughInfo);
    LSNOTIMPL(GetRunStrikethroughInfo);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetBorderInfo (member, LS callback)
//
//  Synopsis:   Not implemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetBorderInfo(
    PLSRUN plsrun,      // IN
    LSTFLOW ktFlow,     // IN
    long* pdurBorder,   // OUT
    long* pdupBorder)   // OUT
{
    LSTRACE(GetBorderInfo);

    // This should only ever be called if we set the fBorder flag in lschp.
    LSNOTIMPL(GetBorderInfo);

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   ReleaseRun (member, LS callback)
//
//  Synopsis:   Callback to release plsrun object, which we don't do.  We have
//              a cache of COneRuns which we keep (and grow) for the lifetime
//              of the CLineServices object.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::ReleaseRun(
    PLSRUN plsrun)      // IN
{
    LSTRACE(ReleaseRun);

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   Hyphenate (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::Hyphenate(
    PCLSHYPH plsHyphLast,   // IN
    LSCP cpBeginWord,       // IN
    LSCP cpExceed,          // IN
    PLSHYPH plsHyph)        // OUT
{
    LSTRACE(Hyphenate);
    // FUTURE (mikejoch) Need to adjust cp values if kysr != kysrNil.

    plsHyph->kysr = kysrNil;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetHyphenInfo (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetHyphenInfo(
    PLSRUN plsrun,      // IN
    DWORD* pkysr,       // OUT
    WCHAR* pwchKysr)    // OUT
{
    LSTRACE(GetHyphenInfo);

    *pkysr = kysrNil;
    *pwchKysr = WCH_NULL;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FInterruptUnderline (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FInterruptUnderline(
    PLSRUN plsrunFirst,         // IN
    LSCP cpLastFirst,           // IN
    PLSRUN plsrunSecond,        // IN
    LSCP cpStartSecond,         // IN
    BOOL* pfInterruptUnderline) // OUT
{
    LSTRACE(FInterruptUnderline);
    // FUTURE (mikejoch) Need to adjust cp values if we ever interrupt underlining.

    *pfInterruptUnderline = FALSE;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FInterruptShade (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FInterruptShade(
    PLSRUN plsrunFirst,         // IN
    PLSRUN plsrunSecond,        // IN
    BOOL* pfInterruptShade)     // OUT
{
    LSTRACE(FInterruptShade);

    *pfInterruptShade = TRUE;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FInterruptBorder (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FInterruptBorder(
    PLSRUN plsrunFirst,         // IN
    PLSRUN plsrunSecond,        // IN
    BOOL* pfInterruptBorder)    // OUT
{
    LSTRACE(FInterruptBorder);

    *pfInterruptBorder = FALSE;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FInterruptShaping (member, LS callback)
//
//  Synopsis:   We compare CF between the runs to see if they are different
//              enough to cause an interrup in shaping between the runs
//
//  Arguments:  kTFlow              text direction and orientation
//              plsrunFirst         run pointer for the previous run
//              plsrunSecond        run pointer for the current run
//              pfInterruptShaping  TRUE - Interrupt shaping
//                                  FALSE - Don't interrupt shaping, merge runs
//
//  Returns:    LSERR               lserrNone if succesful
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FInterruptShaping(
    LSTFLOW kTFlow,                 // IN
    PLSRUN plsrunFirst,             // IN
    PLSRUN plsrunSecond,            // IN
    BOOL* pfInterruptShaping)       // OUT
{
    LSTRACE(FInterruptShaping);

    Assert(pfInterruptShaping != NULL &&
           plsrunFirst != NULL && plsrunSecond != NULL);

    CComplexRun * pcr1 = plsrunFirst->GetComplexRun();
    CComplexRun * pcr2 = plsrunSecond->GetComplexRun();

    Assert(pcr1 != NULL && pcr2 != NULL);

    *pfInterruptShaping = (pcr1->GetScript() != pcr2->GetScript());

    if (!*pfInterruptShaping)
    {
        const CCharFormat *pCF1 = plsrunFirst->GetCF();
        const CCharFormat *pCF2 = plsrunSecond->GetCF();

        Assert(pCF1 != NULL && pCF2 != NULL);

        // We want to break the shaping if the formats are not similar format
        *pfInterruptShaping = !pCF1->CompareForLikeFormat(pCF2);
    }

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetGlyphs (member, LS callback)
//
//  Synopsis:   Returns an index of glyph ids for the run passed in
//
//  Arguments:  plsrun              pointer to the run
//              pwch                string of character codes
//              cwch                number of characters in pwch
//              kTFlow              text direction and orientation
//              rgGmap              map of glyph info parallel to pwch
//              prgGindex           array of output glyph indices
//              prgGprop            array of output glyph properties
//              pcgindex            number of glyph indices
//
//  Returns:    LSERR               lserrNone if succesful
//                                  lserrInvalidRun if failure
//                                  lserrOutOfMemory if memory alloc fails
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetGlyphs(
    PLSRUN plsrun,          // IN
    LPCWSTR pwch,           // IN
    DWORD cwch,             // IN
    LSTFLOW kTFlow,         // IN
    PGMAP rgGmap,           // OUT
    PGINDEX* prgGindex,     // OUT
    PGPROP* prgGprop,       // OUT
    DWORD* pcgindex)        // OUT
{
    LSTRACE(GetGlyphs);

    LSERR lserr = lserrNone;
    HRESULT hr = S_OK;
    CComplexRun * pcr;
    DWORD cMaxGly;
    HDC hdc = _pci->_hdc;
    HDC hdcShape = NULL;
    HFONT hfont;
    HFONT hfontOld = NULL;
    SCRIPT_CACHE *psc;
    WORD *pGlyphBuffer = NULL;
    WORD *pGlyph = NULL;
    SCRIPT_VISATTR *pVisAttr = NULL;
    CCcs *pccs = NULL;
    CBaseCcs *pBaseCcs = NULL;
    CStr strTransformedText;
    BOOL fTriedFontLink = FALSE;

    pcr = plsrun->GetComplexRun();
    if (pcr == NULL)
    {
        Assert(FALSE);
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }

    pccs = GetCcs(plsrun, hdc, _pci);
    if (pccs == NULL)
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }

    pBaseCcs = pccs->GetBaseCcs();
    
    hfont = pBaseCcs->_hfont;
    Assert(hfont != NULL);
    psc = pBaseCcs->GetUniscribeCache();
    Assert(psc != NULL);

    // In some fonts in some locales, NBSPs aren't rendered like spaces.
    // Under these circumstances, we need to convert NBSPs to spaces
    // before calling ScriptShape.
    // BUGBUG: Due to a bug in Bidi Win9x GDI, we can't detect that
    // old bidi fonts lack an NBSP (IE5 bug 68214). We hack around this by
    // simply always swapping the space character for the NBSP. Since this
    // only happens for glyphed runs, US perf is not impacted.
    if (_lineFlags.GetLineFlags(plsrun->Cp() + cwch) & FLAG_HAS_NBSP)
    {
        const WCHAR * pwchStop;
        WCHAR * pwch2;

        HRESULT hr = THR(strTransformedText.Set(pwch, cwch));
        if (hr == S_OK)
        {
            pwch = strTransformedText;

            pwch2 = (WCHAR *) pwch;
            pwchStop = pwch + cwch;

            while (pwch2 < pwchStop)
            {
                if (*pwch2 == WCH_NBSP)
                {
                    *pwch2 = L' ';
                }

                pwch2++;
            }
        }
    }

    // Inflate the number of max glyphs to generate
    // A good high end guess is the number of chars plus 6% or 10,
    // whichever is greater.
    cMaxGly = cwch + max(10, (int)cwch >> 4);

    Assert(cMaxGly > 0);
    pGlyphBuffer = (WORD *) NewPtr(cMaxGly * (sizeof(WORD) + sizeof(SCRIPT_VISATTR)));
    // Our memory alloc failed. No point in going on.
    if (pGlyphBuffer == NULL)
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }
    pGlyph = (WORD *) pGlyphBuffer;
    pVisAttr = (SCRIPT_VISATTR *) (pGlyphBuffer + cMaxGly);

    // Repeat the shaping process until it succeds, or fails for a reason different
    // from insufficient memory, a cache fault, or failure to glyph a character using
    // the current font
    do
    {
        // If a prior ::ScriptShape() call failed because it needed the font
        // selected into the hdc, then select the font into the hdc and try
        // again.
        if (hr == E_PENDING)
        {
            // If we have a valid hdcShape, then ScriptShape() failed for an
            // unknown reason. Bail out.
            if (hdcShape != NULL)
            {
                AssertSz(FALSE, "ScriptShape() failed for an unknown reason");
                lserr = LSERRFromHR(hr);
                goto Cleanup;
            }

            // Select the current font into the hdc and set hdcShape to hdc.
            hfontOld = SelectFontEx(hdc, hfont);
            hdcShape = hdc;
        }
        // If a prior ::ScriptShape() call failed because it was unable to
        // glyph a character with the current font, swap the font around and
        // try it again.
        else if (hr == USP_E_SCRIPT_NOT_IN_FONT)
        {
            if (!fTriedFontLink)
            {
                // Unable to find the glyphs in the font. Font link to try an
                // alternate font which might work.
                fTriedFontLink = TRUE;

                // Set the sid for the complex run to match the text (instead
                // of sidDefault.
                Assert(plsrun->_ptp->IsText());
                plsrun->_sid = plsrun->_ptp->Sid();

                // Deselect the font if we selected it.
                if (hdcShape != NULL)
                {
                    Assert(hfontOld != NULL);
                    SelectFontEx(hdc, hfontOld);
                    hdcShape = NULL;
                    hfontOld = NULL;
                }

                // Get the font using the normal sid from the text to fontlink.
                pccs = GetCcs(plsrun, hdc, _pci);
                if (pccs == NULL)
                {
                    lserr = lserrOutOfMemory;
                    goto Cleanup;
                }

                pBaseCcs = pccs->GetBaseCcs();

                // Reset the hfont and psc using the new pccs.
                hfont = pBaseCcs->_hfont;
                Assert(hfont != NULL);
                psc = pBaseCcs->GetUniscribeCache();
                Assert(psc != NULL);
            }
            else
            {
                // We tried to font link but we still couldn't make it work.
                // Blow the SCRIPT_ANALYSIS away and just let GDI deal with it.
                pcr->NukeAnalysis();
            }
        }
        // If ScriptShape() failed because of insufficient buffer space,
        // resize the buffer
        else if (hr == E_OUTOFMEMORY)
        {
            WORD *pGlyphBufferT = NULL;

            // enlarge the glyph count by another 6% of run or 10, whichever is larger.
            cMaxGly += max(10, (int)cwch >> 4);

            Assert(cMaxGly > 0);
            pGlyphBufferT = (WORD *) ReallocPtr(pGlyphBuffer, cMaxGly *
                                                (sizeof(WORD) + sizeof(SCRIPT_VISATTR)));
            if (pGlyphBufferT != NULL)
            {
                pGlyphBuffer = pGlyphBufferT;
                pGlyph = (WORD *) pGlyphBuffer;
                pVisAttr = (SCRIPT_VISATTR *) (pGlyphBuffer + cMaxGly);
            }
            else
            {
                // Memory alloc failure.
                lserr = lserrOutOfMemory;
                goto Cleanup;
            }
        }

        // Try to shape the script again
        hr = ::ScriptShape(hdcShape, psc, pwch, cwch, cMaxGly, pcr->GetAnalysis(),
                           pGlyph, rgGmap, pVisAttr, (int *) pcgindex);

    }
    while (hr == E_PENDING || hr == USP_E_SCRIPT_NOT_IN_FONT || hr == E_OUTOFMEMORY);

    // NB (mikejoch) We shouldn't ever fail except for the OOM case. USP should
    // always be loaded, since we wouldn't get a valid eScript otherwise.
    Assert(hr == S_OK || hr == E_OUTOFMEMORY);
    lserr = LSERRFromHR(hr);

Cleanup:
    // Restore the font if we selected it
    if (hfontOld != NULL)
    {
        Assert(hdcShape != NULL);
        SelectFontEx(hdc, hfontOld);
    }

    // If LS passed us a string which was an aggregate of several runs (which
    // happens if we returned FALSE from FInterruptShaping()) then we need
    // to make sure that the same _sid is stored in each por covered by the
    // aggregate string. Normally this isn't a problem, but if we changed
    // por->_sid for font linking then it becomes necessary. We determine
    // if a change occurred by comparing plsrun->_sid to sidDefault, which
    // is the value plsrun->_sid is always set to for a glyphed run (in
    // ChunkifyTextRuns()).
    if (plsrun->_sid != sidDefault && plsrun->_lscch < (LONG) cwch)
    {
        DWORD sidAlt = plsrun->_sid;
        COneRun * por = plsrun->_pNext;
        LONG lscchMergedRuns = cwch - plsrun->_lscch;

        while (lscchMergedRuns > 0 && por)
        {
            if (por->IsNormalRun() || por->IsSyntheticRun())
            {
                por->_sid = sidAlt;
                lscchMergedRuns -= por->_lscch;
            }
            por = por->_pNext;
        }
    }

    if (lserr == lserrNone)
    {
        // Move the values from the working buffer to the output arguments
        pcr->CopyPtr(pGlyphBuffer);
        *prgGindex = pGlyph;
        *prgGprop = (WORD *) pVisAttr;
    }
    else
    {
        // free up the allocated memory on failure
        if (pGlyphBuffer != NULL)
        {
            DisposePtr(pGlyphBuffer);
        }
        *prgGindex = NULL;
        *prgGprop = NULL;
        *pcgindex = 0;
    }

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetGlyphPositions (member, LS callback)
//
//  Synopsis:   Returns an index of glyph ids for the run passed in
//
//  Arguments:  plsrun              pointer to the run
//              lsDevice            presentation or reference
//              pwch                string of character codes
//              rgGmap              map of glyphs
//              cwch                number of characters in pwch
//              prgGindex           array of output glyph indices
//              prgGprop            array of output glyph properties
//              pcgindex            number of glyph indices
//              kTFlow              text direction and orientation
//              rgDu                array of glyph widths
//              rgGoffset           array of glyph offsets
//
//  Returns:    LSERR               lserrNone if succesful
//                                  lserrModWidthPairsNotSet if failure
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetGlyphPositions(
    PLSRUN plsrun,          // IN
    LSDEVICE lsDevice,      // IN
    LPWSTR pwch,            // IN
    PCGMAP pgmap,           // IN
    DWORD cwch,             // IN
    PCGINDEX rgGindex,      // IN
    PCGPROP rgGprop,        // IN
    DWORD cgindex,          // IN
    LSTFLOW kTFlow,         // IN
    int* rgDu,              // OUT
    PGOFFSET rgGoffset)     // OUT
{
    LSTRACE(GetGlyphPositions);

    LSERR lserr = lserrNone;
    HRESULT hr = S_OK;
    CComplexRun * pcr;
    HDC hdc = _pci->_hdc;
    HDC hdcPlace = NULL;
    HFONT hfont;
    HFONT hfontOld = NULL;
    SCRIPT_CACHE *psc;
    CCcs *pccs = NULL;
    CBaseCcs *pBaseCcs = NULL;

    pcr = plsrun->GetComplexRun();
    if (pcr == NULL)
    {
        Assert(FALSE);
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }

    pccs = GetCcs(plsrun, hdc, _pci);
    if (pccs == NULL)
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }
    pBaseCcs = pccs->GetBaseCcs();
    
    hfont = pBaseCcs->_hfont;
    Assert(hfont != NULL);
    psc = pBaseCcs->GetUniscribeCache();
    Assert(psc != NULL);


    // Try to place the glyphs
    hr = ::ScriptPlace(hdcPlace, psc, rgGindex, cgindex, (SCRIPT_VISATTR *)rgGprop,
                       pcr->GetAnalysis(), rgDu, rgGoffset, NULL);

    // Handle failure
    if(hr == E_PENDING)
    {

        Assert(hdcPlace == NULL);

        // Select the current font into the hdc and set hdcShape to hdc.
        hfontOld = SelectFontEx(hdc, hfont);
        hdcPlace = hdc;

        // Try again
        hr = ::ScriptPlace(hdcPlace, psc, rgGindex, cgindex, (SCRIPT_VISATTR *)rgGprop,
                           pcr->GetAnalysis(), rgDu, rgGoffset, NULL);

    }

    // NB (mikejoch) We shouldn't ever fail except for the OOM case (if USP is
    // allocating the cache). USP should always be loaded, since we wouldn't
    // get a valid eScript otherwise.
    Assert(hr == S_OK || hr == E_OUTOFMEMORY);
    lserr = LSERRFromHR(hr);

Cleanup:
    // Restore the font if we selected it
    if (hfontOld != NULL)
    {
        SelectFontEx(hdc, hfontOld);
    }

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   ResetRunContents (member, LS callback)
//
//  Synopsis:   This callback seems to be more informational.
//
//  Arguments:  plsrun              pointer to the run
//              cpFirstOld          cpFirst before shaping
//              dcpOld              dcp before shaping
//              cpFirstNew          cpFirst after shaping
//              dcpNew              dcp after shaping
//
//  Returns:    LSERR               lserrNone if succesful
//                                  lserrMismatchLineContext if failure
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::ResetRunContents(
    PLSRUN plsrun,      // IN
    LSCP cpFirstOld,    // IN
    LSDCP dcpOld,       // IN
    LSCP cpFirstNew,    // IN
    LSDCP dcpNew)       // IN
{
    LSTRACE(ResetRunContents);
    // FUTURE (mikejoch) Need to adjust cp values if we ever implement this.
    // FUTURE (paulnel) this doesn't appear to be needed for IE. Clarification
    // is being obtained from Line Services
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetGlyphExpansionInfo (member, LS callback)
//
//  Synopsis:   This callback is used for glyph based justification
//              1. For Arabic, it handles kashida insetion
//              2. For cluster characters, (thai vietnamese) it keeps tone 
//                 marks on their base glyphs
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetGlyphExpansionInfo(
    PLSRUN plsrun,                  // IN
    LSDEVICE lsDeviceID,            // IN
    LPCWSTR pwch,                   // IN
    PCGMAP rggmap,                  // IN
    DWORD cwch,                     // IN
    PCGINDEX rgglyph,               // IN
    PCGPROP rgProp,                 // IN
    DWORD cglyph,                   // IN
    LSTFLOW kTFlow,                 // IN
    BOOL fLastTextChunkOnLine,      // IN
    PEXPTYPE rgExpType,             // OUT
    LSEXPINFO* rgexpinfo)           // OUT
{
    LSTRACE(GetGlyphExpansionInfo);

    LSERR lserr = lserrNone;
    SCRIPT_VISATTR *psva = (SCRIPT_VISATTR *)&rgProp[0];
    CComplexRun * pcr;
    const SCRIPT_PROPERTIES *psp = NULL;
    BOOL fKashida = FALSE;
    int iKashidaWidth = 1;  // width of a kashida
    int iPropL = 0;         // index to the connecting glyph left
    int iPropR = 0;         // index to the connecting glyph right
    int iBestPr = -1;       // address of the best priority in a word so far
    int iPrevBestPr = -1;   
    int iKashidaLevel = 0;
    int iBestKashidaLevel = 10;
    BYTE bBestPr = SCRIPT_JUSTIFY_NONE;
    BYTE bCurrentPr = SCRIPT_JUSTIFY_NONE;
    BYTE expType = exptNone;

    pcr = plsrun->GetComplexRun();
    UINT justifyStyle = plsrun->_pPF->_uTextJustify;

    if (pcr == NULL)
    {
        Assert(FALSE);
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }

    // 1. From the script analysis we can tell what language we are dealing with
    //    a. if we are Arabic block languages, we need to do the kashida insertion
    //    b. if we are Thai or Vietnamese, we need to set the expansion type for diacritics 
    //       to none so they remain on their base glyph.
    psp = GetScriptProperties(pcr->GetAnalysis()->eScript);

    // Check to see if we are an Arabic block language

    fKashida = IsInArabicBlock(psp->langid);

    // if we are going to do kashida insertion we need to get the kashida width information
    if(fKashida)
    {
        lserr = GetKashidaWidth(plsrun, &iKashidaWidth);

        if(lserr != lserrNone)
            goto Cleanup;
    }

    //initialize rgExpType
    expType = exptNone;
    memset(rgExpType, expType, sizeof(EXPTYPE)*cglyph);

    // initialize rgexpinfo to zeros
    memset(rgexpinfo, 0, sizeof(LSEXPINFO)*cglyph);

    // Loop through the glyph attributes. We assume logical order here
    while(iPropL < (int)cglyph)
    {
        bCurrentPr = psva[iPropL].uJustification;

        switch(bCurrentPr)
        {
        case SCRIPT_JUSTIFY_NONE:
            rgExpType[iPropL] = exptNone;
            break;

        case SCRIPT_JUSTIFY_ARABIC_BLANK:
            if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1))
            {
                rgExpType[iPropL] = exptAddWhiteSpace;

                switch(justifyStyle)
                {
                case styleTextJustifyInterWord:
                    rgexpinfo[iPropL].prior = 1;
                    rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                    break;
                case styleTextJustifyNewspaper:
                    rgexpinfo[iPropL].prior = 1;
                    rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                    break;
                case styleTextJustifyDistribute:
                    rgexpinfo[iPropL].prior = 2;
                    rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                    break;
                case styleTextJustifyDistributeAllLines:
                    rgexpinfo[iPropL].prior = 1;
                    rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                    break;
                default:
                    rgExpType[iPropL] = exptNone;
                    if(iBestPr >=0)
                    {
                        iPrevBestPr = iPropL;
                        rgExpType[iBestPr] = exptAddInkContinuous;
                        rgexpinfo[iBestPr].prior = 1;
                        rgexpinfo[iBestPr].duMax = lsexpinfInfinity;
                        rgexpinfo[iBestPr].u.AddInkContinuous.duMin = iKashidaWidth;
                        iBestPr = -1;
                        bBestPr = SCRIPT_JUSTIFY_NONE;
                        iBestKashidaLevel = 10;
                    }
                    break;
                }
            }
            // Final character on the line should not get justification
            // flag set.
            else
            {
                rgExpType[iPropL] = exptNone;

                if(justifyStyle < styleTextJustifyInterWord ||
                   justifyStyle > styleTextJustifyDistributeAllLines)
                {
                    if(iBestPr >=0)
                    {
                        iPrevBestPr = iPropL;
                        rgExpType[iBestPr] = exptAddInkContinuous;
                        rgexpinfo[iBestPr].prior = 1;
                        rgexpinfo[iBestPr].duMax = lsexpinfInfinity;
                        rgexpinfo[iBestPr].u.AddInkContinuous.duMin = iKashidaWidth;
                        iBestPr = -1;
                        bBestPr = SCRIPT_JUSTIFY_NONE;
                        iBestKashidaLevel = 10;
                    }
                }
            }
            break;

        case SCRIPT_JUSTIFY_CHARACTER:
            // this value was prefilled to exptAddWhiteSpace
            // we should not be here for Arabic type text
            Assert(!fKashida);
            if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1))
            {
                rgExpType[iPropL] = exptAddWhiteSpace;
                switch(justifyStyle)
                {
                case styleTextJustifyInterWord:
                    rgexpinfo[iPropL].prior = 0;
                    break;
                case styleTextJustifyNewspaper:
                    rgexpinfo[iPropL].prior = 1;
                    break;
                case styleTextJustifyDistribute:
                    rgexpinfo[iPropL].prior = 1;
                    break;
                case styleTextJustifyDistributeAllLines:
                    rgexpinfo[iPropL].prior = 1;
                    break;
                }
                rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
            }
            // Final character on the line should not get justification
            // flag set.
            else
                rgExpType[iPropL] = exptNone;
            break;
        case SCRIPT_JUSTIFY_BLANK:
            // this value was prefilled to exptAddWhiteSpace
            // we should not be here for Arabic type text
            Assert(!fKashida);
            if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1))
            {
                rgExpType[iPropL] = exptAddWhiteSpace;
                switch(justifyStyle)
                {
                case styleTextJustifyInterWord:
                    rgexpinfo[iPropL].prior = 1;
                    break;
                case styleTextJustifyNewspaper:
                    rgexpinfo[iPropL].prior = 1;
                    break;
                case styleTextJustifyDistribute:
                    rgexpinfo[iPropL].prior = 1;
                    break;
                case styleTextJustifyDistributeAllLines:
                    rgexpinfo[iPropL].prior = 1;
                    break;
                }
                rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
            }
            // Final character on the line should not get justification
            // flag set.
            else
                rgExpType[iPropL] = exptNone;
            break;

        // kashida is placed after kashida and seen characters
        case SCRIPT_JUSTIFY_ARABIC_KASHIDA:
            if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1))
            {
                switch(justifyStyle)
                {
                case styleTextJustifyInterWord:
                    rgExpType[iPropL] = exptNone;
                    break;
                case styleTextJustifyNewspaper:
                    rgExpType[iPropL] = exptAddInkContinuous;
                    rgexpinfo[iPropL].prior = 1;
                    rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistribute:
                    rgExpType[iPropL] = exptAddInkContinuous;
                    rgexpinfo[iPropL].prior = 1;
                    rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistributeAllLines:
                    rgExpType[iPropL] = exptAddInkContinuous;
                    rgexpinfo[iPropL].prior = 1;
                    rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                default:
                    iKashidaLevel = KASHIDA_PRIORITY1;
                    break;
                }
            }
            // Final character on the line should not get justification
            // flag set.
            else
                rgExpType[iPropL] = exptNone;
            break;

        case SCRIPT_JUSTIFY_ARABIC_SEEN:
            if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1))
            {
                switch(justifyStyle)
                {
                case styleTextJustifyInterWord:
                    rgExpType[iPropL] = exptNone;
                    break;
                case styleTextJustifyNewspaper:
                    rgExpType[iPropL] = exptAddInkContinuous;
                    rgexpinfo[iPropL].prior = 1;
                    rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistribute:
                    rgExpType[iPropL] = exptAddInkContinuous;
                    rgexpinfo[iPropL].prior = 1;
                    rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistributeAllLines:
                    rgExpType[iPropL] = exptAddInkContinuous;
                    rgexpinfo[iPropL].prior = 1;
                    rgexpinfo[iPropL].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                default:
                    iKashidaLevel = KASHIDA_PRIORITY2;
                    if(iKashidaLevel <= iBestKashidaLevel)
                    {
                        iBestKashidaLevel = iKashidaLevel;
                        // these types of kashida go after this glyph visually
                        for(iPropR = iPropL + 1; iPropR < (int)cglyph && psva[iPropR].fDiacritic; iPropR++);
                        if(iPropR != iPropL)
                            iPropR--;

                        iBestPr = iPropR;
                    }
                    break;
                }
            }
            // Final character on the line should not get justification
            // flag set.
            else
                rgExpType[iPropL] = exptNone;
            break;

        // kashida is placed before the ha and alef
        case SCRIPT_JUSTIFY_ARABIC_HA:
            if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1))
            {
                switch(justifyStyle)
                {
                case styleTextJustifyInterWord:
                    rgExpType[iPropL] = exptNone;
                    break;
                case styleTextJustifyNewspaper:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistribute:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistributeAllLines:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                default:
                    iKashidaLevel = KASHIDA_PRIORITY3;
                    if(iKashidaLevel <= iBestKashidaLevel)
                    {
                        iBestKashidaLevel = iKashidaLevel;
                        iBestPr = iPropL - 1;
                    }
                    break;
                }
            }
            // Final character on the line should not get justification
            // flag set.
            else
                rgExpType[iPropL] = exptNone;
            break;

        case SCRIPT_JUSTIFY_ARABIC_ALEF:
            if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1))
            {
                switch(justifyStyle)
                {
                case styleTextJustifyInterWord:
                    rgExpType[iPropL] = exptNone;
                    break;
                case styleTextJustifyNewspaper:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistribute:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistributeAllLines:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                default:
                    iKashidaLevel = KASHIDA_PRIORITY4;
                    if(iKashidaLevel <= iBestKashidaLevel)
                    {
                        iBestKashidaLevel = iKashidaLevel;
                        iBestPr = iPropL - 1;
                    }
                    break;
                }
            }
            // Final character on the line should not get justification
            // flag set.
            else
                rgExpType[iPropL] = exptNone;
            break;

        // kashida is placed before prior character if prior char
        // is a medial ba type
        case SCRIPT_JUSTIFY_ARABIC_RA:
            if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1))
            {
                switch(justifyStyle)
                {
                case styleTextJustifyInterWord:
                    rgExpType[iPropL] = exptNone;
                    break;
                case styleTextJustifyNewspaper:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistribute:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistributeAllLines:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                default:
                    iKashidaLevel = KASHIDA_PRIORITY5;
                    if(iKashidaLevel <= iBestKashidaLevel)
                    {
                        iBestKashidaLevel = iKashidaLevel;

                        if(psva[iPropL - 1].uJustification == SCRIPT_JUSTIFY_ARABIC_BA)
                            iBestPr = iPropL - 2;
                        else
                            iBestPr = iPropL - 1;
                    }
                    break;
                }
            }
            // Final character on the line should not get justification
            // flag set.
            else
                rgExpType[iPropL] = exptNone;
            break;

        // kashida is placed before last normal type in word
        case SCRIPT_JUSTIFY_ARABIC_BARA:
            if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1))
            {
                switch(justifyStyle)
                {
                case styleTextJustifyInterWord:
                    rgExpType[iPropL] = exptNone;
                    break;
                case styleTextJustifyNewspaper:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistribute:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistributeAllLines:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                default:
                    iKashidaLevel = KASHIDA_PRIORITY6;
                    if(iKashidaLevel <= iBestKashidaLevel)
                    {
                        iBestKashidaLevel = iKashidaLevel;
                
                        // these types of kashida go before this glyph visually
                        iBestPr = iPropL - 1;
                    }
                    break;
                }
            }
            // Final character on the line should not get justification
            // flag set.
            else
                rgExpType[iPropL] = exptNone;
            break;

        // kashida is placed before last normal type in word
        case SCRIPT_JUSTIFY_ARABIC_BA:
            if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1))
            {
                switch(justifyStyle)
                {
                case styleTextJustifyInterWord:
                    rgExpType[iPropL] = exptNone;
                    break;
                case styleTextJustifyNewspaper:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistribute:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistributeAllLines:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                default:
                    iKashidaLevel = KASHIDA_PRIORITY7;
                    if(iKashidaLevel <= iBestKashidaLevel)
                    if(iKashidaLevel <= iBestKashidaLevel)
                    {
                        iBestKashidaLevel = iKashidaLevel;
                
                        // these types of kashida go before this glyph visually
                        iBestPr = iPropL - 1;
                    }
                    break;
                }
            }
            // Final character on the line should not get justification
            // flag set.
            else
                rgExpType[iPropL] = exptNone;
            break;

        // kashida is placed before last normal type in word
        case SCRIPT_JUSTIFY_ARABIC_NORMAL:
            if(!fLastTextChunkOnLine || (DWORD)iPropL != (cglyph - 1))
            {
                switch(justifyStyle)
                {
                case styleTextJustifyInterWord:
                    rgExpType[iPropL] = exptNone;
                    break;
                case styleTextJustifyNewspaper:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistribute:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                case styleTextJustifyDistributeAllLines:
                    rgExpType[iPropL - 1] = exptAddInkContinuous;
                    rgexpinfo[iPropL - 1].prior = 1;
                    rgexpinfo[iPropL - 1].duMax = lsexpinfInfinity;
                    rgexpinfo[iPropL - 1].fCanBeUsedForResidual = TRUE;
                    rgexpinfo[iPropL - 1].u.AddInkContinuous.duMin = iKashidaWidth;
                    break;
                default:
                    iKashidaLevel = KASHIDA_PRIORITY8;
                    if(iKashidaLevel <= iBestKashidaLevel)
                    {
                        iBestKashidaLevel = iKashidaLevel;
                
                        // these types of kashida go before this glyph visually
                        iBestPr = iPropL - 1;
                    }
                    break;
                }
            }
            // Final character on the line should not get justification
            // flag set.
            else
                rgExpType[iPropL] = exptNone;
            break;

        default:
            AssertSz( 0, "We have a new SCRIPT_JUSTIFY type");
        }

        iPropL++;
    }

    if(justifyStyle < styleTextJustifyInterWord ||
       justifyStyle > styleTextJustifyDistributeAllLines)
    {
        if(iBestPr >= 0)
        {
            rgexpinfo[iBestPr].prior = 1;
            rgexpinfo[iBestPr].duMax = lsexpinfInfinity;
            if(fKashida)
                rgexpinfo[iBestPr].u.AddInkContinuous.duMin = iKashidaWidth;
        }
    }
    
Cleanup:

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetGlyphExpansionInkInfo (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetGlyphExpansionInkInfo(
    PLSRUN plsrun,              // IN
    LSDEVICE lsDeviceID,        // IN
    GINDEX gindex,              // IN
    GPROP gprop,                // IN
    LSTFLOW kTFlow,             // IN
    DWORD cAddInkDiscrete,      // IN
    long* rgDu)                 // OUT
{
    LSTRACE(GetGlyphExpansionInkInfo);
    LSNOTIMPL(GetGlyphExpansionInkInfo);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FTruncateBefore (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FTruncateBefore(
    PLSRUN plsrunCur,       // IN
    LSCP cpCur,             // IN
    WCHAR wchCur,           // IN
    long durCur,            // IN
    PLSRUN plsrunPrev,      // IN
    LSCP cpPrev,            // IN
    WCHAR wchPrev,          // IN
    long durPrev,           // IN
    long durCut,            // IN
    BOOL* pfTruncateBefore) // OUT
{
    LSTRACE(FTruncateBefore);
    // FUTURE (mikejoch) Need to adjust cp values if we ever implement this.
    LSNOTIMPL(FTruncateBefore);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FHangingPunct (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FHangingPunct(
    PLSRUN plsrun,
    MWCLS mwcls,
    WCHAR wch,
    BOOL* pfHangingPunct)
{
    LSTRACE(FHangingPunct);

    *pfHangingPunct = FALSE;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetSnapGrid (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetSnapGrid(
    WCHAR* rgwch,           // IN
    PLSRUN* rgplsrun,       // IN
    LSCP* rgcp,             // IN
    DWORD cwch,             // IN
    BOOL* rgfSnap,          // OUT
    DWORD* pwGridNumber)    // OUT
{
    LSTRACE(GetSnapGrid);
    // FUTURE (mikejoch) Need to adjust cp values if we ever do grid justification.

    *pwGridNumber = 0;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   FCancelHangingPunct (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FCancelHangingPunct(
    LSCP cpLim,                // IN
    LSCP cpLastAdjustable,      // IN
    WCHAR wch,                  // IN
    MWCLS mwcls,                // IN
    BOOL* pfCancelHangingPunct) // OUT
{
    LSTRACE(FCancelHangingPunct);
    // FUTURE (mikejoch) Need to adjust cp values if we ever implement this.
    LSNOTIMPL(FCancelHangingPunct);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   ModifyCompAtLastChar (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::ModifyCompAtLastChar(
    LSCP cpLim,             // IN
    LSCP cpLastAdjustable,  // IN
    WCHAR wchLast,          // IN
    MWCLS mwcls,            // IN
    long durCompLastRight,  // IN
    long durCompLastLeft,   // IN
    long* pdurChangeComp)   // OUT
{
    LSTRACE(ModifyCompAtLastChar);
    // FUTURE (mikejoch) Need to adjust cp values if we ever implement this.
    LSNOTIMPL(ModifyCompAtLastChar);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   EnumText (member, LS callback)
//
//  Synopsis:   Enumeration function, currently unimplemented
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::EnumText(
    PLSRUN plsrun,           // IN
    LSCP cpFirst,            // IN
    LSDCP dcp,               // IN
    LPCWSTR rgwch,           // IN
    DWORD cwch,              // IN
    LSTFLOW lstflow,         // IN
    BOOL fReverseOrder,      // IN
    BOOL fGeometryProvided,  // IN
    const POINT* pptStart,   // IN
    PCHEIGHTS pheightsPres,  // IN
    long dupRun,             // IN
    BOOL fCharWidthProvided, // IN
    long* rgdup)             // IN
{
    LSTRACE(EnumText);

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   EnumTab (member, LS callback)
//
//  Synopsis:   Enumeration function, currently unimplemented
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::EnumTab(
    PLSRUN plsrun,          // IN
    LSCP cpFirst,           // IN
    WCHAR * rgwch,          // IN                   
    WCHAR wchTabLeader,     // IN
    LSTFLOW lstflow,        // IN
    BOOL fReversedOrder,    // IN
    BOOL fGeometryProvided, // IN
    const POINT* pptStart,  // IN
    PCHEIGHTS pheightsPres, // IN
    long dupRun)
{
    LSTRACE(EnumTab);

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   EnumPen (member, LS callback)
//
//  Synopsis:   Enumeration function, currently unimplemented
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::EnumPen(
    BOOL fBorder,           // IN
    LSTFLOW lstflow,        // IN
    BOOL fReverseOrder,     // IN
    BOOL fGeometryProvided, // IN
    const POINT* pptStart,  // IN
    long dup,               // IN
    long dvp)               // IN
{
    LSTRACE(EnumPen);

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetObjectHandlerInfo (member, LS callback)
//
//  Synopsis:   Returns an object handler for the client-side functionality
//              of objects which are handled primarily by LineServices.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetObjectHandlerInfo(
    DWORD idObj,        // IN
    void* pObjectInfo)  // OUT
{
    LSTRACE(GetObjectHandlerInfo);

    switch (idObj)
    {
        case LSOBJID_RUBY:
#if !defined(UNIX) && !defined(_MAC)
            Assert( sizeof(RUBYINIT) == sizeof(::RUBYINIT) );
            *(RUBYINIT *)pObjectInfo = s_rubyinit;
#else
            int iSize = InitRubyinit();
            Assert( sizeof(RUBYINIT) == sizeof(::RUBYINIT) + iSize );
            *(::RUBYINIT *)pObjectInfo = s_unix_rubyinit;
#endif
            break;

        case LSOBJID_TATENAKAYOKO:
#if !defined(UNIX) && !defined(_MAC)
            Assert( sizeof(TATENAKAYOKOINIT) == sizeof(::TATENAKAYOKOINIT) );
            *(TATENAKAYOKOINIT *)pObjectInfo = s_tatenakayokoinit;
#else
            iSize = InitTatenakayokoinit();
            Assert( sizeof(TATENAKAYOKOINIT) == sizeof(::TATENAKAYOKOINIT) + iSize);
            *(::TATENAKAYOKOINIT *)pObjectInfo = s_unix_tatenakayokoinit;
#endif
            break;

        case LSOBJID_HIH:
#if !defined(UNIX) && !defined(_MAC)
            Assert( sizeof(HIHINIT) == sizeof(::HIHINIT) );
            *(HIHINIT *)pObjectInfo = s_hihinit;
#else
            iSize = InitHihinit();
            Assert( sizeof(HIHINIT) == sizeof(::HIHINIT) + iSize);
            *(::HIHINIT *)pObjectInfo = s_unix_hihinit;
#endif
            break;

        case LSOBJID_WARICHU:
#if !defined(UNIX) && !defined(_MAC)
            Assert( sizeof(WARICHUINIT) == sizeof(::WARICHUINIT) );
            *(WARICHUINIT *)pObjectInfo = s_warichuinit;
#else
            iSize = InitWarichuinit();
            Assert( sizeof(WARICHUINIT) == sizeof(::WARICHUINIT) + iSize);
            *(::WARICHUINIT *)pObjectInfo = s_unix_warichuinit;
#endif
            break;

        case LSOBJID_REVERSE:
#if !defined(UNIX) && !defined(_MAC)
            Assert( sizeof(REVERSEINIT) == sizeof(::REVERSEINIT) );
            *(REVERSEINIT *)pObjectInfo = s_reverseinit;
#else
            iSize = InitReverseinit();
            Assert( sizeof(REVERSEINIT) == sizeof(::REVERSEINIT) + iSize);
            *(::REVERSEINIT *)pObjectInfo = s_unix_reverseinit;
#endif
            break;

        default:
            AssertSz(0,"Unknown LS object ID.");
            break;
    }

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   AssertFailed (member, LS callback)
//
//  Synopsis:   Assert callback for LineServices
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------

void WINAPI
CLineServices::AssertFailed(
    char* szMessage,
    char* szFile,
    int   iLine)
{
    LSTRACE(AssertFailed);

#if DBG==1
    if (IsTagEnabled(tagLSAsserts))
    {
        DbgExAssertImpl( szFile, iLine, szMessage );
    }
#endif
}

//-----------------------------------------------------------------------------
//
//  Function:   ChunkifyTextRun
//
//  Synopsis:   Break up a text run if necessary.
//
//  Returns:    lserr.
//
//-----------------------------------------------------------------------------

LSERR
CLineServices::ChunkifyTextRun(COneRun *por, COneRun **pporOut)
{
    LONG    cchRun;
    LPCWSTR pwchRun;
    BOOL    fHasInclEOLWhite = por->_pPF->HasInclEOLWhite(por->_fInnerPF);
    const   DWORD cpCurr = por->Cp();
    LSERR   lserr = lserrNone;
    
    *pporOut = por;
    
    //
    // 1) If there is a whitespace at the beginning of line, we
    //    do not want to show the whitespace (0 width -- the right
    //    way to do it is to say that the run is hidden).
    //
    if (IsFirstNonWhiteOnLine(cpCurr))
    {
        const TCHAR * pwchRun = por->_pchBase;
        DWORD cp = cpCurr;
        cchRun  = por->_lscch;

        if (!fHasInclEOLWhite)
        {
            while (cchRun)
            {
                const TCHAR ch = *pwchRun++;

                if (!IsWhite(ch))
                    break;

                // Note a whitespace at BOL
                WhiteAtBOL(cp);
                //_lineFlags.AddLineFlag(cp, FLAG_HAS_NOBLAST);
                cp++;

                // Goto next character
                cchRun--;
            }
        }


        //
        // Did we find any whitespaces at BOL? If we did, then
        // create a chunk with those whitespace and mark them
        // as hidden.
        //
        if (cchRun != por->_lscch)
        {
            por->_lscch -= cchRun;
            por->_fHidden = TRUE;
            goto Cleanup;
        }
    }

    //
    // 2. Fold whitespace after an aligned or abspos'd site if there
    //    was a whitespace *before* the aligned site. The way we do this
    //    folding is by saying that the present space is hidden.
    //
    {
        const TCHAR chFirst = *por->_pchBase;
        if (   !fHasInclEOLWhite
            && !_fIsEditable
           )
        {
            if (   IsWhite(chFirst)
                && NeedToEatThisSpace(por)
               )
            {
                _lineFlags.AddLineFlag(cpCurr, FLAG_HAS_NOBLAST);
                por->_lscch = 1;
                por->_fHidden = TRUE;
                goto Cleanup;
            }
        }

        if (WCH_NONREQHYPHEN == chFirst)
        {
            _lineFlags.AddLineFlag(cpCurr, FLAG_HAS_NOBLAST);
        }
    }
    
    if (   !fHasInclEOLWhite
        && IsFirstNonWhiteOnLine(cpCurr)
       )
    {
        //
        // 3. Note any \n\r's
        //
        const TCHAR ch = *por->_pchBase;

        if (ch == TEXT('\r') || ch == TEXT('\n'))
        {
            WhiteAtBOL(cpCurr);
            _lineFlags.AddLineFlag(cpCurr, FLAG_HAS_NOBLAST);
        }
    }

    if (_fScanForCR)
    {
        cchRun = por->_lscch;
        pwchRun = por->_pchBase;

        if (WCH_CR == *pwchRun)
        {
            // If all we have on the line are sites, then we do not want the \r to
            // contribute to height of the line and hence we set _fNoTextMetrics to TRUE.
            if (LineHasOnlySites(por->Cp()))
                por->_fNoTextMetrics = TRUE;
            
            lserr = TerminateLine(por, TL_ADDNONE, pporOut);
            if (lserr != lserrNone)
                goto Cleanup;
            if (*pporOut == NULL)
            {
                //
                // All lines ending in a carriage return have the BR flag set on them.
                //
                _lineFlags.AddLineFlag(cpCurr, FLAG_HAS_A_BR);
                por->FillSynthData(SYNTHTYPE_LINEBREAK);
                *pporOut = por;
            }
            goto Cleanup;
        }
        // if we came here after a single site in _fScanForCR mode, it means that we
        // have some text after the single site and hence it should be on the next
        // line. Hence terminate the line here. (If it were followed by a \r, then
        // it would have fallen in the above case which would have consumed that \r)
        else if (_fSingleSite)
        {
            lserr = TerminateLine(por, TL_ADDEOS, pporOut);
            goto Cleanup;
        }
        else
        {
            LONG cch = 0;
            while (cch != cchRun)
            {
                if (WCH_CR == *pwchRun)
                    break;
                cch++;
                pwchRun++;
            }
            por->_lscch = cch;
        }
    }

    Assert(por->_ptp && por->_ptp->IsText() && por->_sid == DWORD(por->_ptp->Sid()));

    BEGINSUPPRESSFORQUILL
    //
    // BUGBUG(CThrash): Please improve this sid check!
    //
    if (   _pBidiLine != NULL
        || sidAsciiLatin != por->_sid
        || g_iNumShape != NUMSHAPE_NONE
        || _li._fLookaheadForGlyphing)
    {
        BOOL fGlyph = FALSE;
        BOOL fForceGlyphing = FALSE;
        BOOL fNeedBidiLine = (_pBidiLine != NULL);
        BOOL fRTL = FALSE;
        DWORD uLangDigits = LANG_NEUTRAL;
        WCHAR chNext = WCH_NULL;

        //
        // 4. Note any glyphable etc chars
        //
        cchRun = por->_lscch;
        pwchRun = por->_pchBase;
        while (cchRun-- && !(fGlyph && fNeedBidiLine))
        {
            const TCHAR ch = *pwchRun++;

            fGlyph |= IsGlyphableChar(ch);
            fNeedBidiLine |= IsRTLChar(ch);
        }

        //
        // 5. Break run based on the text direction.
        //
        if (fNeedBidiLine && _pBidiLine == NULL)
        {
            _pBidiLine = new CBidiLine(_treeInfo, _cpStart, _li._fRTL, _pli);
        }
        if (_pBidiLine != NULL)
        {
            por->_lscch = _pBidiLine->GetRunCchRemaining(por->Cp(), por->_lscch);
            // FUTURE (mikejoch) We are handling symmetric swapping by forced
            // glyphing of RTL runs. We may be able to do this faster by
            // swapping symmetric characters in CLSRenderer::TextOut().
            fRTL = fForceGlyphing = _pBidiLine->GetLevel(por->Cp()) & 1;
        }

        //
        // 6. Break run based on the digits.
        //
        if (g_iNumShape != NUMSHAPE_NONE)
        {
            cchRun = por->_lscch;
            pwchRun = por->_pchBase;
            while (cchRun && !InRange(*pwchRun, ',', '9'))
            {
                pwchRun++;
                cchRun--;
            }
            if (cchRun)
            {
                if (g_iNumShape == NUMSHAPE_NATIVE)
                {
                    uLangDigits = g_uLangNationalDigits;
                    fGlyph = TRUE;
                }
                else
                {
                    COneRun * porContext;

                    Assert(g_iNumShape == NUMSHAPE_CONTEXT && InRange(*pwchRun, ',', '9'));

                    // Scan back for first strong text.
                    cchRun = pwchRun - por->_pchBase;
                    pwchRun--;
                    while (cchRun != 0 && (!IsStrongClass(DirClassFromCh(*pwchRun)) || InRange(*pwchRun, WCH_LRM, WCH_RLM)))
                    {
                        cchRun--;
                        pwchRun--;
                    }
                    porContext = _listCurrent._pTail;
                    if (porContext == por)
                    {
                        porContext = porContext->_pPrev;
                    }
                    while (cchRun == 0 && porContext != NULL)
                    {
                        if (porContext->IsNormalRun() && porContext->_pchBase != NULL)
                        {
                            cchRun = porContext->_lscch;
                            pwchRun = porContext->_pchBase + cchRun - 1;
                            while (cchRun != 0 && (!IsStrongClass(DirClassFromCh(*pwchRun)) || InRange(*pwchRun, WCH_LRM, WCH_RLM)))
                            {
                                cchRun--;
                                pwchRun--;
                            }
                        }
                        porContext = porContext->_pPrev;
                    }

                    if (cchRun != 0)
                    {
                        if (ScriptIDFromCh(*pwchRun) == DefaultScriptIDFromLang(g_uLangNationalDigits))
                        {
                            uLangDigits = g_uLangNationalDigits;
                            fGlyph = TRUE;
                        }
                    }
                    else if (IsRTLLang(g_uLangNationalDigits) && _li._fRTL)
                    {
                        uLangDigits = g_uLangNationalDigits;
                        fGlyph = TRUE;
                    }
                }
            }
        }

        //
        // 7. Check if we should have glyphed the prior run (for combining
        //    Latin diacritics; esp. Vietnamese)
        //
        if (_lsMode == LSMODE_MEASURER)
        {
            if (fGlyph && !_li._fLookaheadForGlyphing &&
                IsCombiningMark(*(por->_pchBase)))
            {
                // We want to break the shaping if the formats are not similar
                COneRun * porPrev = _listCurrent._pTail;

                while (porPrev != NULL && !porPrev->IsNormalRun())
                {
                    porPrev = porPrev->_pPrev;
                }

                if (porPrev != NULL && !porPrev->_lsCharProps.fGlyphBased)
                {
                    const CCharFormat *pCF1 = por->GetCF();
                    const CCharFormat *pCF2 = porPrev->GetCF();

                    Assert(pCF1 != NULL && pCF2 != NULL);
                    _li._fLookaheadForGlyphing = pCF1->CompareForLikeFormat(pCF2);
                }
            }
        }
        else
        {
            if (_li._fLookaheadForGlyphing)
            {
                Assert (por->_lscch >= 1);

                CTxtPtr txtptr(_pMarkup, por->Cp() + por->_lscch - 1);

                // N.B. (mikejoch) This is an extremely non-kosher lookup to do
                // here. It is quite possible that chNext will be from an
                // entirely different site. If that happens, though, it will
                // only cause the unnecessary glyphing of this run, which
                // doesn't actually affect the visual appearence.
                while ((chNext = txtptr.NextChar()) == WCH_NODE);
                if (IsCombiningMark(chNext))
                {
                    // Good chance this run needs to be glyphed with the next one.
                    // Turn glyphing on.
                    fGlyph = fForceGlyphing = TRUE;
                }
            }
        }

        //
        // 8. Break run based on the script.
        //
        if (fGlyph || fRTL)
        {
            CComplexRun * pcr = por->GetNewComplexRun();

            if (pcr == NULL)
                return lserrOutOfMemory;
                
            pcr->ComputeAnalysis(_pFlowLayout, fRTL, fForceGlyphing, chNext,
                                 _chPassword, por, _listCurrent._pTail, uLangDigits);

            // Something on the line needs glyphing.
            if (por->_lsCharProps.fGlyphBased)
            {
                _fGlyphOnLine = TRUE;

#ifndef NO_UTF16
                if (por->_sid != sidSurrogateA && por->_sid != sidSurrogateB)
#endif
                {
                    por->_sid = sidDefault;
                }
            }
        }
    }
    ENDSUPPRESSFORQUILL

Cleanup:
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   NeedToEatThisSpace
//
//  Synopsis:   Decide if the current space needs to be eaten. We eat any space
//              after a abspos or aligned site *IF* that site was preceeded by
//              a space too.
//
//  Returns:    lserr.
//
//-----------------------------------------------------------------------------

BOOL
CLineServices::NeedToEatThisSpace(COneRun *porIn)
{
    BOOL fMightFold = FALSE;
    BOOL fFold      = FALSE;
    CElement   *pElementLayout;
    CTreeNode  *pNode;
    COneRun    *por;

    por = FindOneRun(porIn->_lscpBase);
    if (   por == NULL
        && porIn->_lscpBase >= _listCurrent._pTail->_lscpBase
       )
        por = _listCurrent._pTail;

    //
    // BUGBUG(SujalP): We will not fold across hidden stuff... need to fix this...
    //
    while(por)
    {
        if (por->_fCharsForNestedLayout)
        {
            pNode = por->Branch();
            Assert(pNode->NeedsLayout());
            pElementLayout = pNode->GetUpdatedLayout()->ElementOwner();
            if (!pElementLayout->IsInlinedElement())
                fMightFold = TRUE;
            else
                break;
        }
        else
            break;
        por = por->_pPrev;
    }

    if (fMightFold)
    {
        if (!por)
        {
            fFold = TRUE;
        }
        else if (!por->_fCharsForNestedElement)
        {
            Assert(por->_pchBase);
            TCHAR ch = por->_pchBase[por->_lscch-1];
            if (   ch == _T(' ')
                || ch == _T('\t')
               )
            {
                fFold = TRUE;
            }
        }
    }

    return fFold;
}

//-----------------------------------------------------------------------------
//
//  Function:   ChunkifyObjectRun
//
//  Synopsis:   Breakup a object run if necessary.
//
//  Returns:    lserr.
//
//-----------------------------------------------------------------------------

LSERR
CLineServices::ChunkifyObjectRun(COneRun *por, COneRun **pporOut)
{
    CElement        *pElementLayout;
    CLayout         *pLayout;
    CTreeNode       *pNode;
    BOOL             fInlinedElement;
    BOOL             fIsAbsolute = FALSE;
    LSERR            lserr  = lserrNone;
    LONG             cp     = por->Cp();
    COneRun         *porOut = por;

    Assert(por->_lsCharProps.idObj == LSOBJID_EMBEDDED);
    pNode = por->Branch();
    Assert(pNode);
    pLayout = pNode->GetUpdatedLayout();
    Assert(pLayout);
    pElementLayout = pLayout->ElementOwner();
    Assert(pElementLayout);
    fInlinedElement = pElementLayout->IsInlinedElement();

    //
    // Setup all the various flags and counts
    //
    if (fInlinedElement)
    {
        _lineCounts.AddLineCount(cp, LC_INLINEDSITES, por->_lscch);
    }
    else
    {
        fIsAbsolute = pNode->IsAbsolute((stylePosition)por->GetFF()->_bPositionType);

        if (!fIsAbsolute)
        {
            _lineCounts.AddLineCount(cp, LC_ALIGNEDSITES, por->_lscch);
            _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_ALIGNED);
        }
        else
        {
            //
            // This is the only opportunity for us to measure abspos'd sites
            //
            if (_lsMode == LSMODE_MEASURER)
            {
                _lineCounts.AddLineCount(cp, LC_ABSOLUTESITES, por->_lscch);
                pLayout->SetXProposed(0);
                pLayout->SetYProposed(0);
            }
            _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_ABSOLUTE_ELT);
        }
    }
    _lineFlags.AddLineFlag(cp, FLAG_HAS_EMBED_OR_WBR);

    if (por->_fCharsForNestedRunOwner)
    {
        _lineFlags.AddLineFlag(cp, FLAG_HAS_NESTED_RO);
        BEGINSUPPRESSFORQUILL
        _pMeasurer->_pRunOwner = pLayout;
        ENDSUPPRESSFORQUILL
    }

    if (IsOwnLineSite(por))
    {
        _fSingleSite = TRUE;
        _li._fSingleSite = TRUE;
        _li._fHasEOP = TRUE;
    }

    if (!fInlinedElement)
    {
        // Since we are not showing this run, lets just anti-synth it!
        por->MakeRunAntiSynthetic();

        // And remember that these chars are white at BOL
        if (IsFirstNonWhiteOnLine(cp))
        {
            AlignedAtBOL(cp, por->_lscch);
        }
    }

    *pporOut = porOut;
    return lserr;
}

//+==============================================================================
//
//  Method: GetRenderingHighlights
//
//  Synopsis: Get the type of highlights between these two cp's by going through
//            the array of HighlightSegments on
//
//  A 'Highlight' denotes a "non-content-based" way of changing the rendering
// of something. Currently the only highlight is for selection. This may change.
//
//-------------------------------------------------------------------------------

//
// BUGBUG ( marka ) - this routine has been made generic enough so it could work
// for any type of highlighting. However we'll assume there's just Selection
// as then we can take 'quick outs' and bail out of looping through the entire array all the time
//
//

DWORD
CLineServices::GetRenderingHighlights( int cpLineMin, int cpLineMax )
{
    int i;
    HighlightSegment** ppHighlight;
    DWORD dwHighlight = HIGHLIGHT_TYPE_None;

    CLSRenderer *pRenderer = GetRenderer();

    if ( pRenderer->_cpSelMax != - 1 )
    {
        for ( i = pRenderer->_aryHighlight.Size(), ppHighlight = pRenderer->_aryHighlight;
            i > 0 ;
            i --, ppHighlight++ )
        {
            if ( (  (*ppHighlight)->_cpStart <= cpLineMin ) && ( (*ppHighlight)->_cpEnd >= cpLineMax) )
            {
                dwHighlight |= (*ppHighlight)->_dwHighlightType ;
            }
        }
    }
    return dwHighlight;
}

//+====================================================================================
//
// Method: SetRenderingMarkup
//
// Synopsis: Set any 'markup' on this COneRun
//
// BUGBUG: marka - currently the only type of Markup is Selection. We see if the given
//         run is selected, and if so we mark-it.
//
//         This used to be called ChunkfiyForSelection. However, under due to the new selection
//         model's use of TreePos's the Run is automagically chunkified for us
//
//------------------------------------------------------------------------------------



LSERR
CLineServices::SetRenderingHighlights(COneRun  *por)
{
    DWORD dwHighlight;
    DWORD dwImeHighlight;    
    int cpMin = por->Cp()  ;
    int cpMax = por->Cp() + por->_lscch ;

    // If we are not rendering we should do nothing
    if (_lsMode != LSMODE_RENDERER)
        goto Cleanup;

    // We will not show selection if it is hidden or its an object.
    if (   por->_fHidden
        || por->_lsCharProps.idObj == LSOBJID_EMBEDDED
       )
        goto Cleanup;

    dwHighlight = GetRenderingHighlights( cpMin, cpMax );

#ifndef NO_IME
    dwImeHighlight = (dwHighlight / HIGHLIGHT_TYPE_ImeInput) & 7;

    if ( dwImeHighlight )
    {
        por->ImeHighlight( _pFlowLayout, dwImeHighlight );
    }
#endif // NO_IME

    if ( ( dwHighlight & HIGHLIGHT_TYPE_Selected) != 0 )
    {
        por->Selected(_pFlowLayout, TRUE);
    }

Cleanup:

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   CheckForUnderLine
//
//  Synopsis:   Check if the current run is underlined/overlined.
//
//  Returns:    lserr.
//
//-----------------------------------------------------------------------------

LSERR
CLineServices::CheckForUnderLine(COneRun *por)
{
    LSERR lserr = lserrNone;

    if (!por->_dwImeHighlight)
    {
        const CCharFormat *pCF = por->GetCF();

        por->_lsCharProps.fUnderline =    (   _fIsEditable
                                           || !pCF->IsVisibilityHidden())
                                       && (   pCF->_fUnderline
                                           || pCF->_fOverline
                                           || pCF->_fStrikeOut);
    }
    else
    {
        por->_lsCharProps.fUnderline = por->_fUnderlineForIME;
    }

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   CheckForTransform
//
//  Synopsis:   If text transformation is necessary for the run, then do it here.
//
//  Returns:    lserr.
//
//-----------------------------------------------------------------------------

inline LSERR
CLineServices::CheckForTransform(COneRun  *por)
{
    LSERR lserr = lserrNone;
    const CCharFormat* pCF = por->GetCF();
    if( pCF->IsTextTransformNeeded() )
    {
        TCHAR chPrev = WCH_NULL;
        
        _lineFlags.AddLineFlag(por->Cp(), FLAG_HAS_NOBLAST);

        if (pCF->_bTextTransform == styleTextTransformCapitalize)
        {
            COneRun *porTemp = FindOneRun(por->_lscpBase - 1);

            // If no run found on this line before this run, then its the first
            // character on this line, and hence needs to be captilaized. This is
            // done implicitly by initializing chPrev to WCH_NULL.
            while (porTemp)
            {
                // If it is anti-synth, then its not shown at all or
                // is not in the flow, hence we ignore it for the purposes
                // of transformations. If it is synthetic then we will need
                // to look at runs before the synthetic to determine the
                // prev character. Hence, we only need to investigate normal runs
                if (porTemp->IsNormalRun())
                {
                    // If the previous run had a nested layout, then we will ignore it.
                    // The rule says that if there is a layout in the middle of a word
                    // then the character after the layout is *NOT* capitalized. If the
                    // layout is at the beginning of the word then the character needs
                    // to be capitalized. In essence, we completely ignore layouts when
                    // deciding whether a char should be capitalized, i.e. if we had
                    // the combination:
                    // <charX><layout><charY>, then capitalization of <charY> depends
                    // only on what <charX> -- ignoring the fact that there is a layout
                    // (It does not matter if the layout is hidden, aligned, abspos'd
                    // or relatively positioned).
                    if (   !porTemp->_fCharsForNestedLayout
                        && porTemp->_synthType == SYNTHTYPE_NONE
                       )
                    {
                        Assert(porTemp->_ptp->IsText());
                        chPrev = porTemp->_pchBase[porTemp->_lscch - 1];
                        break;
                    }
                }
                porTemp = porTemp->_pPrev;
            }
        }

        por->_pchBase = (TCHAR *)TransformText(por->_cstrRunChars, por->_pchBase, por->_lscch,
                                               pCF->_bTextTransform, chPrev);
        if (por->_pchBase == NULL)
            lserr = lserrOutOfMemory;
    }
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   CheckForPassword
//
//  Synopsis:   If text transformation is necessary for the run, then do it here.
//
//  Returns:    lserr.
//
//-----------------------------------------------------------------------------

LSERR
CLineServices::CheckForPassword(COneRun  *por)
{
    LSERR lserr = lserrNone;
    CStr strPassword;
    HRESULT hr;
    
    Assert(_chPassword);

    for (LONG i = 0; i < por->_lscch; i++)
    {
        hr = strPassword.Append(&_chPassword, 1);
        if (hr != S_OK)
        {
            lserr = lserrOutOfMemory;
            goto Cleanup;
        }
    }
    por->_pchBase = por->SetString(strPassword);
    if (por->_pchBase == NULL)
        lserr = lserrOutOfMemory;
    
Cleanup:
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   AdjustForNavBRBug (member)
//
//  Synopsis:   Navigator will break between a space and a BR character at
//              the end of a line if the BR character, when it's width is
//              treated like that of a space character, will cause line to
//              overflow.  This is idiotic, but necessary for compat.
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------

LSERR
CLineServices::AdjustCpLimForNavBRBug(
    LONG xWrapWidth,        // IN
    LSLINFO *plslinfo )     // IN/OUT
{
    LSERR lserr = lserrNone;

    // check 1: (a) We must not be in a PRE and (b) the line
    // must have at least three chars.
    if (   !_pPFFirst->HasPre(_fInnerPFFirst)
        && _cpLim - plslinfo->cpFirstVis >= 3)
    {
        COneRun *por = FindOneRun(_lscpLim - 1);
        if (!por)
            goto Cleanup;

        // check 2: Line must have ended in a BR
        if (   por->_ptp->IsEndNode()
            && por->Branch()->Tag() == ETAG_BR
           )
        {
            // check 3: The BR char must be preceeded by a space char.

            // Go to the begin BR
            por = por->_pPrev;
            if (!(   por
                  && por->IsAntiSyntheticRun()
                  && por->_ptp->IsBeginNode()
                  && por->Branch()->Tag() == ETAG_BR)
               )
                goto Cleanup;

            // Now go one more beyond that to check for the space
            do
            {
                por = por->_pPrev;
            }
            while (por && por->IsAntiSyntheticRun() && !por->_fCharsForNestedLayout);
            if (!por)
                goto Cleanup;

            // BUGBUG(SujalP + CThrash): This will not work if the space was
            // in, say, a reverse object. But then this *is* a NAV bug. If
            // somebody complains vehemently, then we will fix it...
            if (   por->IsNormalRun()
                && por->_ptp->IsText()
                && WCH_SPACE == por->_pchBase[por->_lscch - 1]
               )
            {
                if (_fMinMaxPass)
                    ((CDisplay *)_pMeasurer->_pdp)->SetNavHackPossible();

                // check 4: must have overflowed, because the width of a BR
                // character is included in _xWidth
                if (!_fMinMaxPass && _li._xWidth > xWrapWidth)
                {
                    // check 5:  The BR alone cannot be overflowing.  We must
                    // have at least one pixel more to break before the BR.

                    HRESULT hr;
                    LSTEXTCELL lsTextCell;

                    hr = THR( QueryLineCpPpoint( _lscpLim, FALSE, NULL, &lsTextCell ) );

                    if (   S_OK == hr
                        && (_li._xWidth - lsTextCell.dupCell) > xWrapWidth)
                    {
                        // Break between the space and the BR.  Yuck! Also 2
                        // here because one for open BR and one for close BR
                        _cpLim -= 2;

                        // The char for open BR was antisynth'd in the lscp
                        // space, so just reduce by one char for the close BR.
                        _lscpLim--;
                    }
                }
            }
        }
    }

Cleanup:
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   AdjustForRelElementAtEnd (member)
//
//  Synopsis: In our quest to put as much on the line as possible we will end up
//      putting the begin splay for a relatively positioned element on this
//      line(the first character in this element will be on the next line)
//      This is not really acceptable for positioning purposes and hence
//      we detect that this happened and chop off the relative element
//      begin splay (and any chars anti-synth'd after it) so that they will
//      go to the next line. (Look at bug 54162).
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
VOID
CLineServices::AdjustForRelElementAtEnd()
{
    //
    // By looking for _lscpLim - 1, we find the last but-one-run. After
    // this run, there will possibly be antisynthetic runs (all at the same
    // lscp -- but different cp's -- as the last char of this run) which
    // have ended up on this line. It is these set of antisynthetic runs
    // that we have to investigate to find any ones which begin a relatively
    // positioned element. If we do find one, we will stop and modify cpLim
    // so as to not include the begin splay and any antisynth's after it.
    //
    COneRun *por = FindOneRun(_lscpLim - 1);

    Assert(por);

    //
    // Go to the start of the antisynth chunk (if any).
    //
    por = por->_pNext;

    //
    // Walk while we have antisynth runs
    //
    while (   por
           && por->IsAntiSyntheticRun()
           && _lscpLim >= por->_lscpBase + por->_lscch
          )
    {
        //
        // If it begins a relatively positioned element, then we want to
        // stop and modify the cpLim
        //
        if (   por->_ptp->IsBeginElementScope()
            && por->GetCF()->IsRelative(por->_fInnerCF))
        {
            _cpLim = por->Cp();
            break;
        }
        Assert(por->_lscch == 1);
        por = por->_pNext;
    }
}

//-----------------------------------------------------------------------------
//
//  Function:   ComputeWhiteInfo (member)
//
//  Synopsis:   A post pass for the CMeasurer to compute whitespace
//              information (_cchWhite and _xWhite) on the associated
//              CLine object (_li).
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------

HRESULT
CLineServices::ComputeWhiteInfo(LSLINFO *plslinfo, LONG *pxMinLineWidth, DWORD dwlf,
                                LONG durWithTrailing, LONG durWithoutTrailing)
{
    HRESULT hr = S_OK;
    BOOL  fInclEOLWhite = _pPFFirst->HasInclEOLWhite(_fInnerPFFirst);
    CMarginInfo *pMarginInfo = (CMarginInfo*)_pMarginInfo;

    // Note that cpLim is no longer an LSCP.  It's been converted by the
    // caller.

    const LONG cpLim = _cpLim;
    CTxtPtr txtptr( _pMarkup, cpLim );
    const TCHAR chBreak = txtptr.GetPrevChar();

    LONG  lscpLim = _lscpLim - 1;
    COneRun *porLast = FindOneRun(lscpLim);

    WHEN_DBG(LONG cchWhiteTest=0);


    Assert( _cpLim == _cpStart + _li._cch );

    //
    // Compute all the flags for the line
    //
    Assert(dwlf == _lineFlags.GetLineFlags(cpLim));
    _li._fHasAbsoluteElt      = (dwlf & FLAG_HAS_ABSOLUTE_ELT) ? TRUE : FALSE;
    _li._fHasAligned          = (dwlf & FLAG_HAS_ALIGNED)      ? TRUE : FALSE;
    _li._fHasEmbedOrWbr       = (dwlf & FLAG_HAS_EMBED_OR_WBR) ? TRUE : FALSE;
    _li._fHasNestedRunOwner   = (dwlf & FLAG_HAS_NESTED_RO)    ? TRUE : FALSE;
    _li._fHasBackground       = (dwlf & FLAG_HAS_BACKGROUND)   ? TRUE : FALSE;
    _li._fHasNBSPs            = (dwlf & FLAG_HAS_NBSP)         ? TRUE : FALSE;
    _fHasRelative             = (dwlf & FLAG_HAS_RELATIVE)     ? TRUE : FALSE;
    _fFoundLineHeight         = (dwlf & FLAG_HAS_LINEHEIGHT)   ? TRUE : FALSE;
    pMarginInfo->_fClearLeft |= (dwlf & FLAG_HAS_CLEARLEFT)    ? TRUE : FALSE;
    pMarginInfo->_fClearRight|= (dwlf & FLAG_HAS_CLEARRIGHT)   ? TRUE : FALSE;
    
    _li._fHasBreak            = (dwlf & FLAG_HAS_A_BR)         ? TRUE : FALSE;

    // Lines containing \r's also need to be marked _fHasBreak

    if (   !_li._fHasBreak
        && _fExpectCRLF
        && plslinfo->endr == endrEndPara
       )
    {
        _li._fHasBreak = TRUE;
    }
    
    _pFlowLayout->_fContainsRelative |= _fHasRelative;

    // If all we have is whitespaces till here then mark it as a dummy line
    if (IsDummyLine(cpLim))
    {
        const LONG cchHidden = _lineCounts.GetLineCount(cpLim, LC_HIDDEN);
        const LONG cch = (_cpLim - _cpStart) - cchHidden;

        _li._fDummyLine = TRUE;
        _li._fForceNewLine = FALSE;

        // If this line was a dummy line because all it contained was hidden
        // characters, then we need to mark the entire line as hidden.  Also
        // if the paragraph contains nothing (except a blockbreak), we also
        // need the hide the line.  Note that LI's are an exception to this
        // rule -- even if all we have on the line is a blockbreak, we don't
        // want to hide it if it's an LI. (LI's are excluded in the dummy
        // line check).
        if ( cchHidden
             && (   cch == 0
                 || _li._fFirstInPara
                )
           )
        {
            _li._fHidden = TRUE;
            _li._yBeforeSpace = 0;
        }
    }

    //
    // Also find out all the relevant counts
    //
    _cInlinedSites  = _lineCounts.GetLineCount(cpLim, LC_INLINEDSITES);
    _cAbsoluteSites = _lineCounts.GetLineCount(cpLim, LC_ABSOLUTESITES);
    _cAlignedSites  = _lineCounts.GetLineCount(cpLim, LC_ALIGNEDSITES);

    //
    // And the relevant values
    //
    if (_fFoundLineHeight)
    {
        _lMaxLineHeight = max(plslinfo->dvpMultiLineHeight,
                              _lineCounts.GetMaxLineValue(cpLim, LC_LINEHEIGHT));
    }
    
    //
    // Consume trailing WCH_NOSCOPE/WCH_BLOCKBREAK characters.
    //

    _li._cchWhite = 0;
    _li._xWhite = 0;

    if (   porLast
        && porLast->_ptp->IsNode()
        && porLast->Branch()->Tag() != ETAG_BR
       )
    {
        // BUGBUG (cthrash) We're potentially tacking on characters but are
        // not included their widths.  These character can/will have widths in
        // edit mode.  The problem is, LS never measured them, so we'd have
        // to measure them separately.

        CTxtPtr txtptrT( txtptr );
        long dcch;

        // If we have a site that lives on it's own line, we may have stopped
        // fetching characters prematurely.  Specifically, we may have left
        // some space characters behind.

        while (TEXT(' ') == txtptrT.GetChar())
        {
            if (!txtptrT.AdvanceCp(1))
                break;
        }
        dcch = txtptrT.GetCp() - txtptr.GetCp();

        _li._cchWhite += (SHORT)dcch;
        _li._cch += dcch;
    }

    WHEN_DBG(cchWhiteTest = _li._cchWhite);

    //
    // Compute _cchWhite and _xWhite of line
    //
    if (!fInclEOLWhite)
    {
        BOOL  fDone = FALSE;
        TCHAR ch;
        COneRun *por = porLast;
        LONG index = por ? lscpLim - por->_lscpBase : 0;
        WHEN_DBG (CTxtPtr txtptrTest(txtptr));

        while (por && !fDone)
        {
            // BUGBUG(SujalP): As noted below, this does not work with
            // aligned/abspos'd sites at EOL
            if (por->_fCharsForNestedLayout)
            {
                fDone = TRUE;
                break;
            }

            if (por->IsNormalRun())
            {
                for(LONG i = index; i >= 0; i--)
                {
                    ch = por->_pchBase[i];
                    if (   IsWhite(ch)
                        // If its a no scope char and we are not showing it then
                        // we treat it like a whitespace.
                        // BUGBUG(SujalP) We also need this check only
                        // if the site is aligned/abspos'd
                       )
                    {
                        _li._cchWhite++;
                        txtptr.AdvanceCp(-1);
                    }
                    else
                    {
                        fDone = TRUE;
                        break;
                    }
                }
            }
            por = por->_pPrev;
            index = por ? por->_lscch - 1 : 0;
        }

#if DBG==1            
        {
            long durWithTrailingDbg, durWithoutTrailingDbg;
            LSERR lserr = GetLineWidth( &durWithTrailingDbg,
                                        &durWithoutTrailingDbg );
            Assert(lserr || durWithTrailingDbg == durWithTrailing);
            Assert(lserr || durWithoutTrailingDbg == durWithoutTrailing);
        }
#endif
        _li._xWhite  = durWithTrailing - durWithoutTrailing;
        _li._xWidth -= _li._xWhite;

        if ( porLast && chBreak == WCH_NODE && !_fScanForCR )
        {
            CTreePos *ptp = porLast->_ptp;

            if (   ptp->IsEndElementScope()
                && ptp->Branch()->Tag() == ETAG_BR
               )
            {
                LONG cp = CPFromLSCP( plslinfo->cpFirstVis );
                _li._fEatMargin = LONG(txtptr.GetCp()) == cp + 1;
            }
        }
    }
    else if (_fScanForCR)
    {
        HRESULT hr;
        LSTEXTCELL  lsTextCell;
        CTxtPtr tp(_pMarkup, cpLim);
        TCHAR ch = tp.GetChar();
        TCHAR chPrev = tp.GetPrevChar();
        BOOL fDecWidth = FALSE;
        LONG cpJunk;
        
        if (   chPrev == _T('\n')
            || chPrev == _T('\r')
           )
        {
            fDecWidth = TRUE;
        }
        else if (ch == WCH_NODE)
        {
            CTreePos *ptpLast = _pMarkup->TreePosAtCp(cpLim - 1, &cpJunk);

            if (   ptpLast->IsEndNode()
                && (   ptpLast->Branch()->Tag() == ETAG_BR
                    || IsPreLikeNode(ptpLast->Branch())
                   )
               )
            {
                fDecWidth = TRUE;
            }
        }

        if (fDecWidth)
        {
            // The width returned by LS includes the \n, which we don't want
            // included in CLine::_xWidth.
            hr = THR( QueryLineCpPpoint( _lscpLim - 1, FALSE, NULL, &lsTextCell ) );
            if (!hr)
            {
                _li._xWidth -= lsTextCell.dupCell; // note _xWhite is unchanged
                if (pxMinLineWidth)
                    *pxMinLineWidth -= lsTextCell.dupCell;
            }
        }
    }
    else
    {
        _li._cchWhite = plslinfo->cpFirstVis - _cpStart;
    }

    //
    // If the white at the end of the line meets the white at the beginning
    // of a line, then we need to shift the BOL white to the EOL white.
    //

    if (_cWhiteAtBOL + _li._cchWhite >= _li._cch)
    {
        _li._cchWhite = _li._cch;
    }

    //
    // Find out if the last char on the line has any overhang, and if so set it on
    // the line.
    //
    if (_fHasOverhang)
    {
        COneRun *por = porLast;

        while (por)
        {
            if (por->_ptp->IsText())
            {
                _li._xLineOverhang = por->_xOverhang;
                if (pxMinLineWidth)
                    *pxMinLineWidth += por->_xOverhang;
                break;
            }
            else if(por->_fCharsForNestedLayout)
                break;
            // Continue for synth and antisynth runs
            por = por->_pPrev;
        }
    }
    
    // Fold the S_FALSE case in -- don't propagate.
    hr = SUCCEEDED(hr) ? S_OK : hr;
    if (hr)
        goto Cleanup;

    DecideIfLineCanBeBlastedToScreen(_cpStart + _li._cch - _li._cchWhite, dwlf);

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function:   DecideIfLineCanBeBlastedToScreen
//
//  Synopsis:   Decides if it is possible for a line to be blasted to screen
//              in a single shot.
//
//  Params:     The last cp in the line
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
void
CLineServices::DecideIfLineCanBeBlastedToScreen(LONG cpEndLine, DWORD dwlf)
{
    // By default you cannot blast a line to the screen
    _li._fCanBlastToScreen = FALSE;

#if DBG==1
    if (IsTagEnabled(tagLSNoBlast))
        goto Cleanup;
#endif

    //
    // 0) If we have eaten a space char or have fontlinking, we cannot blast
    //    line to screen
    //
    if (dwlf & FLAG_HAS_NOBLAST)
        goto Cleanup;

    //
    // 1) No justification
    //
    if (_pPFFirst->GetBlockAlign(_fInnerPFFirst) == htmlBlockAlignJustify)
        goto Cleanup;

    //
    // 2) Only simple LTR
    //
    if (_pBidiLine != NULL)
        goto Cleanup;

    //
    // 3) Cannot handle passwords for blasting.
    //
    if (_chPassword)
        goto Cleanup;

    //
    // 4) If there is a glyph on the line then do not blast.
    //
    if (_fGlyphOnLine)
        goto Cleanup;

    //
    // 5) There's IME highlighting, so we can't blast.
    //
    if (_fSawATab)
        goto Cleanup;

    //
    // 6) There's IME highlighting, so we can't blast.
    //
    if (_fHasIMEHighlight)
        goto Cleanup;

    //
    // None of the restrictions apply, lets blast the sucker to the screen!
    //
    _li._fCanBlastToScreen = TRUE;

Cleanup:
    return;
}

LONG
CLineServices::RememberLineHeight(LONG cp, const CCharFormat *pCF, const CBaseCcs *pBaseCcs)
{
    long lAdjLineHeight;
    
    AssertSz(pCF->_cuvLineHeight.GetUnitType() != CUnitValue::UNIT_PERCENT,
             "Percent units should have been converted to points in ApplyInnerOuterFormats!");
    
    // If there's no height set, get out quick.
    if (pCF->_cuvLineHeight.IsNull())
    {
        lAdjLineHeight = pBaseCcs->_yHeight + abs(pBaseCcs->_yOffset);
    }

    // Apply the CSS Attribute LINE_HEIGHT
    else
    {
        const CUnitValue *pcuvUseThis = &pCF->_cuvLineHeight;
        long lFontHeight = 1;
        
        if (pcuvUseThis->GetUnitType() == CUnitValue::UNIT_FLOAT)
        {
            CUnitValue cuv;
            cuv = pCF->_cuvLineHeight;
            cuv.ScaleTo ( CUnitValue::UNIT_EM );
            pcuvUseThis = &cuv;
            lFontHeight = _pci->DocPixelsFromWindowY( pCF->GetHeightInTwips( _pFlowLayout->Doc() ), TRUE );
        }

        lAdjLineHeight = pcuvUseThis->YGetPixelValue(_pci, 0, lFontHeight );
        NoteLineHeight(cp, lAdjLineHeight);
    }

    if(pCF->HasLineGrid(TRUE))
    {
        _lineFlags.AddLineFlag(cp, FLAG_HAS_NOBLAST);
        lAdjLineHeight = GetClosestGridMultiple(GetLineGridSize(), lAdjLineHeight);
        NoteLineHeight(cp, lAdjLineHeight);
    }

    return lAdjLineHeight;
}


//-------------------------------------------------------------------------
//
//  Member:     VerticalAlignObjects
//
//  Synopsis:   Process all vertically aligned objects and adjust the line
//              height
//
//-------------------------------------------------------------------------
void
CLineServices::VerticalAlignObjects(CLSMeasurer& lsme, unsigned long cSites, long xLineShift)
{
    LONG        cch;
    LONG        yTxtAscent  = _li._yHeight - _li._yDescent;
    LONG        yTxtDescent = _li._yDescent;
    LONG        yDescent    = _li._yDescent;
    LONG        yAscent     = yTxtAscent;
    LONG        yAbsHeight  = 0;
    LONG        yTmpAscent  = 0;
    LONG        yTmpDescent = 0;
    BOOL        fMeasuring  = TRUE;
    BOOL        fPositioning= FALSE;
    CElement *  pElementFL = _pFlowLayout->ElementOwner();
    CLayout  *  pRunOwner;
    CLayout  *  pLayout;
    CElement *  pElementLayout;
    const       CCharFormat *pCF = NULL;
    CTreeNode * pNode;
    CTreePos  * ptpStart;
    CTreePos  * ptp;
    LONG        ich;
    LONG        cchAdvanceStart;
    LONG        cchAdvance;
    // this is the display's margins.
    CMarginInfo *       pMarginInfo = (CMarginInfo*)_pMarginInfo;
    // this is site margins
    long        xLeftMargin = 0;
    long        xRightMargin = 0;

    htmlControlAlign    atAbs = htmlControlAlignNotSet;
    BOOL        fRTLDisplay = _pFlowLayout->GetDisplay()->IsRTL();
    LONG        cchPreChars = lsme._fMeasureFromTheStart ? 0 : lsme._cchPreChars;

    ptpStart = _pFlowLayout->GetContentMarkup()->TreePosAtCp(lsme.GetCp() - _li._cch - cchPreChars, &ich);
    Assert(ptpStart);
    cchAdvanceStart = min( long(_li._cch + cchPreChars), ptpStart->GetCch() - ich );

    // first pass we measure the line's baseline and height
    // second pass we set the rcProposed of the site relative to the line.
    while (fMeasuring || fPositioning)
    {
        cch = _li._cch + cchPreChars;
        lsme.Advance(-cch);
        ptp = ptpStart;
        cchAdvance = cchAdvanceStart;
        pRunOwner = _pFlowLayout;

        while (cch)
        {
            if(ptp->IsBeginElementScope())
            {
                pNode = ptp->Branch();

                pLayout = pElementFL != pNode->Element() && pNode->NeedsLayout()
                            ? pNode->GetUpdatedLayout()
                            : NULL;
            }
            else
            {
                pNode = ptp->GetBranch();
                pLayout = NULL;
            }

            pElementLayout = pLayout ? pNode->Element() : NULL;
            pCF = pNode->GetCharFormat();

            // If we are transisitioning from a normal chunk to relative or
            // relative to normal chunk, add a new chunk to the line. Relative
            // elements can end in the prechar's so look for transition in
            // prechar's too.
            if(fMeasuring && _fHasRelative && !pCF->IsDisplayNone()
                && (    ptp->IsBeginElementScope()
                    ||  ptp->IsText()
                    ||  (   ptp->IsEndElementScope()
                    &&  cch > (_li._cch - (lsme._fMeasureFromTheStart ? lsme._cchPreChars : 0)))))
            {
                TestForTransitionToOrFromRelativeChunk(
                    lsme,
                    pCF->IsRelative(SameScope(pNode, pElementFL)),
                    pElementLayout);
            }


            // If the current branch is a site and not the current CTxtSite
            if (pLayout &&
                (   _fIsEditable
                ||  !pLayout->IsDisplayNone()))
            {
                htmlControlAlign    atSite;
                BOOL                fOwnLine;
                LONG                lBorderSpace;
                LONG                lVPadding = pElementLayout->TestClassFlag(CElement::ELEMENTDESC_VPADDING) ? 1 : 0;
                LONG                yObjHeight;
                ELEMENT_TAG         etag = pElementLayout->Tag();
                CTreeNode *         pNodeLayout = pElementLayout->GetFirstBranch();
                BOOL                fAbsolute = pNodeLayout->IsAbsolute();

                atSite   = pElementLayout->GetSiteAlign();
                fOwnLine = pElementLayout->HasFlag(TAGDESC_OWNLINE);

                if(fAbsolute || pElementLayout->IsInlinedElement())
                {
                    LONG    yProposed = 0;
                    LONG    yTopMargin, yBottomMargin;

                    pLayout->GetMarginInfo(_pci, &xLeftMargin, &yTopMargin, &xRightMargin, &yBottomMargin);
                    
                    // Do the horizontal positioning. We can do it either during
                    // measuring or during vertical positioning. We arbitrarily
                    // chose to do it during measuring.
                    if (fMeasuring)
                    {
                        //if (   !fOwnLine
                        //    ||  fAbsolute)
                        {
                            LONG xPosOfCp = 0;
                            if (!fAbsolute || pNode->GetFancyFormat()->_fAutoPositioned)
                            {
                                BOOL fRTLFlow;
                                LSCP lscp = LSCPFromCP(lsme.GetCp());

                                //
                                // If editable, then the char at the above computed lscp
                                // might be the glyph for that layout rather than the layout
                                // itself. To do this right, we need to we need to find the
                                // run at that lscp, and if it is a glyph-synthetic run then
                                // we need to go to the next run which contains the nested
                                // layout and look at its lscp.
                                //
                                if (_fIsEditable && !fAbsolute)
                                {
                                    COneRun *por = FindOneRun(lscp);
                                    if (   por
                                        && por->IsSyntheticRun()
                                        && por->_synthType == SYNTHTYPE_GLYPH
                                       )
                                    {
                                        por = por->_pNext;

                                        //
                                        // Check that this run exists, and has nested layout
                                        // and that the nested layout is indeed the one we
                                        // are hunting for.
                                        //
                                        Assert(por);
                                        Assert(por->_fCharsForNestedElement);
                                        Assert(por->_fCharsForNestedLayout);
                                        Assert(por->_ptp->Branch() == pNode);
                                        lscp = por->_lscpBase;
                                    }
                                }

                                xPosOfCp = CalculateXPositionOfLSCP(lscp, FALSE, &fRTLFlow);
                                
                                // Adjust xPosOfCp if we have any RTL cases
                                // Bug 45767 - check for flow not being same direction of line
                                if(fRTLDisplay || _li._fRTL || fRTLFlow)
                                {
                                    if(!fAbsolute)
                                    {
                                        CSize size;
                                        pLayout->GetSize(&size);

                                        AdjustXPosOfCp(fRTLDisplay, _li._fRTL, fRTLFlow, &xPosOfCp, size.cx);
                                    }
                                    else if(fRTLDisplay)
                                    {
                                        // To make the xPosOfCp have the correct measurement for the
                                        // right to left display tree node, we make it negative because
                                        // origin (0,0) is top/right in RTL display nodes.
                                        xPosOfCp = -xPosOfCp;
                                    }

                                }

                                if(pCF->HasCharGrid(FALSE))
                                {
                                    long xWidth;

                                    _pFlowLayout->GetSiteWidth(pLayout, _pci, FALSE, _xWidthMaxAvail, &xWidth);
                                    xPosOfCp += (GetClosestGridMultiple(GetCharGridSize(), xWidth) - xWidth)/2;
                                }

                                // absolute margins are added in CLayout::HandlePositionRequest
                                // due to reverse flow issues.
                                if(!fAbsolute)
                                {
                                    if(!fRTLDisplay)
                                        xPosOfCp += xLeftMargin;
                                    else
                                        xPosOfCp += xRightMargin;
                                }

                                pLayout->SetXProposed(xPosOfCp);
                            }
                            
                            if (fAbsolute)
                            {
// BUGBUG (paulnel) What about when the left or right has been specified and the width is auto?
//                  The xPos needs to be adjusted to properly place this case.

                                // fix for #3214, absolutely positioned sites with width auto
                                // need to be measure once we know the line shift, since their
                                // size depends on the xPosition in the line.
                                CSaveCalcInfo sci(_pci);
                                long xWidth;

                                _pci->_fUseOffset    = TRUE;
                                _pci->_xOffsetInLine = xLineShift + xPosOfCp;

                                _pFlowLayout->GetSiteWidth(pLayout, _pci, TRUE, _xWidthMaxAvail, &xWidth );
                            }
                        }
                    }

                    if(!fAbsolute)
                    {
                        CSize size;
                        
                        if(etag == ETAG_HR)
                            lBorderSpace = GRABSIZE;
                        else
                            // Netscape has a pixel descent and ascent to the line if one of the sites
                            // spans the entire line vertically(#20029, #25534).
                            lBorderSpace = lVPadding;

                        pLayout->GetSize(&size);
                        yObjHeight = max(0L, size.cy + yTopMargin + yBottomMargin)+ (2 * lBorderSpace);
                        
                        if(pCF->_fIsRubyText)
                        {
                            RubyInfo *pRubyInfo = GetRubyInfoFromCp(lsme.GetCp());
                            if(pRubyInfo)
                            {
                                yObjHeight += pRubyInfo->yHeightRubyBase - yTxtDescent + pRubyInfo->yDescentRubyText;
                            }                        
                        }

                        switch (atSite)
                        {
                        // align to the baseline of the text
                        case htmlControlAlignNotSet:
                        case htmlControlAlignBottom:
                        case htmlControlAlignBaseline:
                            {
                                LONG lDefDescent = (pElementLayout->TestClassFlag(CElement::ELEMENTDESC_HASDEFDESCENT))
                                                        ? 4
                                                        : 0;

                                if(fMeasuring)
                                {
                                    yTmpDescent = lBorderSpace + lDefDescent;
                                    yTmpAscent  = yObjHeight - yTmpDescent;
                                }
                                else
                                {
                                    yProposed += yAscent - yObjHeight + 2 * lBorderSpace + lDefDescent;
                                }
                                break;
                            }

                        // align to the top of the text
                        case htmlControlAlignTextTop:
                            if(fMeasuring)
                            {
                                yTmpAscent  = yTxtAscent + lBorderSpace;
                                yTmpDescent = yObjHeight - yTxtAscent - lBorderSpace;
                            }
                            else
                            {
                                yProposed += yAscent - yTxtAscent + lBorderSpace;
                            }
                            break;

                        // center of the image aligned to the baseline of the text
                        case htmlControlAlignMiddle:
                        case htmlControlAlignCenter:
                            if(fMeasuring)
                            {
                                yTmpAscent  = (yObjHeight + 1)/2; // + 1 for round off
                                yTmpDescent = yObjHeight/2;
                            }
                            else
                            {
                                yProposed += (yAscent + yDescent - yObjHeight) / 2 + lBorderSpace;
                            }
                            break;

                        // align to the top, absmiddle and absbottom of the line, doesn't really
                        // effect the ascent and descent directly, so we store the
                        // absolute height of the object and recompute the ascent
                        // and descent at the end.
                        case htmlControlAlignAbsMiddle:
                            if(fPositioning)
                            {
                                yProposed += (yAscent + yDescent - yObjHeight) / 2 + lBorderSpace;
                                break;
                            } // fall through when measuring and update max abs height
                        case htmlControlAlignTop:
                            if(fPositioning)
                            {
                                yProposed += lBorderSpace;
                                break;
                            } // fall through when measuring and update the max abs height
                        case htmlControlAlignAbsBottom:
                            if(fMeasuring)
                            {
                                yTmpAscent = 0;
                                yTmpDescent = 0;
                                if(yObjHeight > yAbsHeight)
                                {
                                    yAbsHeight = yObjHeight;
                                    atAbs = atSite;
                                }
                            }
                            else
                            {
                                yProposed += yAscent + yDescent - yObjHeight + lBorderSpace;
                            }
                            break;

                        default:        // we don't want to do anything for
                            if(fOwnLine)
                            {
                                if(fMeasuring)
                                {
                                    yTmpDescent = lBorderSpace;
                                    yTmpAscent  = yObjHeight - lBorderSpace;
                                }
                                else
                                {
                                    yProposed += yAscent -  yObjHeight + lBorderSpace;
                                }
                            }
                            break;      // left/center/right aligned objects
                        }

                        // Keep track of the max ascent and descent
                        if(fMeasuring)
                        {
                            if(yTmpAscent > yAscent)
                                yAscent = yTmpAscent;
                            if(yTmpDescent > yDescent)
                                yDescent = yTmpDescent;
                        }
                        else if(pCF->HasLineGrid(FALSE))
                        {
                            pLayout->SetYProposed(_li._yHeight - yObjHeight - _li._yDescent + yTopMargin);
                        }
                        else
                        {
                            pLayout->SetYProposed(yProposed + lsme._cyTopBordPad + yTopMargin);
                        }
                    }
                }

                //
                // If positioning, add the current layout to the display tree
                //
                if (    fPositioning
                    &&  (   _pci->_smMode == SIZEMODE_NATURAL
                        ||  _pci->_smMode == SIZEMODE_SET
                        ||  _pci->_smMode == SIZEMODE_FULLSIZE)
                    &&  !pElementLayout->IsAligned())
                {
                    const CFancyFormat * pFF = pElementLayout->GetFirstBranch()->GetFancyFormat();
                    long xPos;

                    if(!fRTLDisplay)
                        xPos = pMarginInfo->_xLeftMargin + _li._xLeft;
                    else
                        xPos = - pMarginInfo->_xRightMargin - _li._xRight;

                    if (    !pFF->_fPositioned
                        &&  !pCF->_fRelative)
                    {
                        
                        // we are not using GetYTop for to get the offset of the line because
                        // the before space is not added to the height yet.
                        lsme._pDispNodePrev =
                            _pFlowLayout->GetDisplay()->AddLayoutDispNode(
                            _pci,
                            pElementLayout->GetFirstBranch(),
                            xPos,
                            lsme._yli + _li._yBeforeSpace + (_li._yHeight - _li._yExtent) / 2,
                            lsme._pDispNodePrev);
                    }
                    else
                    {
                        //
                        // If top and bottom or left and right are "auto", position the object
                        //
                        xPos += pLayout->GetXProposed();

                        if (fAbsolute)
                            pLayout->SetYProposed(lsme._cyTopBordPad);
                        
                        CPoint ptAuto(xPos,
                                      lsme._yli + _li._yBeforeSpace + (_li._yHeight - _li._yExtent) / 2 +
                                      pLayout->GetYProposed());

                        pElementLayout->ZChangeElement( 0, &ptAuto);
                    }
                }
            }

            if (pElementLayout)
            {
                //
                //  setup cchAdvance to skip the current layout
                //
                cchAdvance = lsme.GetNestedElementCch(pElementLayout, &ptp);
                Assert(ptp);
            }

            cch -= cchAdvance;
            lsme.Advance(cchAdvance);
            ptp = ptp->NextTreePos();
            cchAdvance = min(cch, ptp->GetCch());
        }

        // We have just finished measuring, update the line's ascent and descent.
        if(fMeasuring)
        {
            // If we have ALIGN_TYPEABSBOTTOM or ALIGN_TYPETOP, they do not contribute
            // to ascent or descent based on the baseline
            if(yAbsHeight > yAscent + yDescent)
            {
                if(atAbs == htmlControlAlignAbsMiddle)
                {
                    LONG yDiff = yAbsHeight - yAscent - yDescent;
                    yAscent += (yDiff + 1) / 2;
                    yDescent += yDiff / 2;
                }
                else if(atAbs == htmlControlAlignAbsBottom)
                {
                    yAscent = yAbsHeight - yDescent;
                }
                else
                {
                    yDescent = yAbsHeight - yAscent;
                }
            }

            // now update the line height
            _li._yHeight = yAscent + yDescent;
            _li._yDescent = yDescent;
            
            if(pCF && pCF->HasLineGrid(FALSE))
            {
                LONG yNormalHeight = _li._yHeight;
                _li._yHeight = GetClosestGridMultiple(GetLineGridSize(), _li._yHeight);
                
                // We don't want to cook up our own line descent if the biggest thing on this 
                // line is just text... our descent has already been calculated for us in that
                // case
                if(yNormalHeight != yTxtAscent + yTxtDescent)
                    _li._yDescent += (_li._yHeight - yNormalHeight)/2 + (_li._yHeight - yNormalHeight)%2;
            }


            // Without this line, line heights specified through
            // styles would override the natural height of the
            // image. This would be cool, but the W3C doesn't
            // like it. Absolute & aligned sites do not affect
            // line height.
            if(_cInlinedSites)
                _fFoundLineHeight = FALSE;

            Assert(_li._yHeight >= 0);

            // Allow last minute adjustment to line height, we need
            // to call this here, because when positioning all the
            // site in line for the display tree, we want the correct
            // YTop.
            AdjustLineHeight();

            // And now the positioning pass
            fMeasuring = FALSE;
            fPositioning = TRUE;
        }
        else
        {
            fPositioning = FALSE;
        }
    }
}

//-----------------------------------------------------------------------------
//
//  Member:     CLineServices::GetRubyInfoFromCp(LONG cpRubyText)
//
//  Synopsis:   Linearly searches through the list of ruby infos and
//              returns the info of the ruby object that contains the given
//              cp.  Note that this function should only be called with a
//              cp that corresponds to a position within Ruby pronunciation
//              text.
//
//              BUGBUG (t-ramar): this code does not take advantage of
//              the fact that this array is sorted by cp, but it does depend 
//              on this fact.  This may be a problem because the entries in this 
//              array are appended in the FetchRubyPosition Line Services callback.
//
//-----------------------------------------------------------------------------

RubyInfo *
CLineServices::GetRubyInfoFromCp(LONG cpRubyText)
{
    RubyInfo *pRubyInfo = NULL;
    int i;

    if((RubyInfo *)_aryRubyInfo == NULL)
        goto Cleanup;

    for (i = _aryRubyInfo.Size(), pRubyInfo = _aryRubyInfo;
         i > 0;
         i--, pRubyInfo++)
    {
        if(pRubyInfo->cp > cpRubyText)
            break;
    }
    pRubyInfo--;
    
    // if this assert fails, chances are that the cp isn't doesn't correspond
    // to a position within some ruby pronunciation text
    Assert(pRubyInfo >= (RubyInfo *)_aryRubyInfo);

Cleanup:
    return pRubyInfo;
}


//-----------------------------------------------------------------------------
//
//  Member:     CLSMeasurer::AdjustLineHeight()
//
//  Synopsis:   Adjust for space before/after and line spacing rules.
//
//-----------------------------------------------------------------------------
void
CLineServices::AdjustLineHeight()
{
    // This had better be true.
    Assert (_li._yHeight >= 0);

    // Need to remember these for hit testing.
    _li._yExtent = _li._yHeight;

    // Only do this if there is a line height used somewhere.
    if (_lMaxLineHeight != LONG_MIN && _fFoundLineHeight)
    {
        _li._yDescent += (_lMaxLineHeight - _li._yHeight) / 2;
        _li._yHeight = _lMaxLineHeight;
    }
}

//-----------------------------------------------------------------------------
//
// Member:      CLineServices::MeasureLineShift (fZeroLengthLine)
//
// Synopsis:    Computes and returns the line x shift due to alignment
//
//-----------------------------------------------------------------------------
LONG
CLineServices::MeasureLineShift(LONG cp, LONG xWidthMax, BOOL fMinMax, LONG * pdxRemainder)
{
    long    xShift;
    UINT    uJustified;

    Assert(_li._fRTL == (unsigned)_pPFFirst->HasRTL(_fInnerPFFirst));

    xShift = ComputeLineShift(
                        (htmlAlign)_pPFFirst->GetBlockAlign(_fInnerPFFirst),
                        _pFlowLayout->GetDisplay()->IsRTL(),
                        _li._fRTL,
                        fMinMax,
                        xWidthMax,
                        _li._xLeft + _li._xWidth +
                            _li._xLineOverhang + _li._xRight,
                        &uJustified,
                        pdxRemainder);
    _li._fJustified = uJustified;
    return xShift;
}

//-----------------------------------------------------------------------------
//
// Member:      CalculateXPositionOfLSCP
//
// Synopsis:    Calculates the X position for LSCP
//
//-----------------------------------------------------------------------------

LONG
CLineServices::CalculateXPositionOfLSCP(
    LSCP lscp,          // LSCP to return the position of.
    BOOL fAfterPrevCp,  // Return the trailing point of the previous LSCP (for an ambigous bidi cp)
    BOOL* pfRTLFlow)    // Flow direction of LSCP.
{
    LSTEXTCELL lsTextCell;
    HRESULT hr;
    BOOL fRTLFlow = FALSE;
    BOOL fUsePrevLSCP = FALSE;
    LONG xRet;

    if (fAfterPrevCp && _pBidiLine != NULL)
    {
        LSCP lscpPrev = FindPrevLSCP(lscp, &fUsePrevLSCP);
        if (fUsePrevLSCP)
        {
            lscp = lscpPrev;
        }
    }

    hr = THR( QueryLineCpPpoint(lscp, FALSE, NULL, &lsTextCell, &fRTLFlow ) );

    if(pfRTLFlow)
        *pfRTLFlow = fRTLFlow;

    xRet = lsTextCell.pointUvStartCell.u;

    // If we're querying for a character which cannot be measured (e.g. a
    // section break char), then LS returns the last character it could
    // measure.  To get the x-position, we add the width of this character.

    if (S_OK == hr && (lsTextCell.cpEndCell < lscp || fUsePrevLSCP))
    {
        if((unsigned)fRTLFlow == _li._fRTL)
            xRet += lsTextCell.dupCell;
        else
        {
            xRet -= lsTextCell.dupCell;
            //
            // What is happening here is that we are being positioned at say pixel
            // pos 10 (xRet=10) and are asked to draw reverese a character which is
            // 11 px wide. So we would then draw at px10, at px9 ... and finally at
            // px 0 -- for at grand total of 11 px. Having drawn at 0, we would be
            // put back at -1. While the going back by 1px is correct, at the BOL
            // this will put us at -1, which is unaccepatble and hence the max with 0.
            //
            xRet = max(0L, xRet);
        }
    }
    else if (hr == S_OK && lsTextCell.cCharsInCell > 1 &&
             lscp > lsTextCell.cpStartCell)
    {
        long lClusterAdjust = MulDivQuick(lscp - lsTextCell.cpStartCell,
                                          lsTextCell.dupCell, lsTextCell.cCharsInCell);
        // we have multiple cps mapped to one glyph. This simply places the caret
        // a percentage of space between beginning and end
        if((unsigned)fRTLFlow == _li._fRTL)
            xRet += lClusterAdjust;
        else
            xRet -= lClusterAdjust;
    }

    Assert(hr || xRet >= 0);

    return hr ? 0 : xRet;
}

//+----------------------------------------------------------------------------
//
// Member:      CLineServices::CalcPositionsOfRangeOnLine
//
// Synopsis:    Find the position of a stretch of text starting at cpStart and
//              and running to cpEnd, inclusive. The text may be broken into
//              multiple rects if the line has reverse objects (runs with
//              mixed directionallity) in it.
//
// Returns:     The number of chunks in the range. Usually this will just be
//              one. If an error occurs it will be zero. The actual width of
//              text (in device units) is returned in paryChunks as rects from
//              the beginning of the line. The top and bottom entries of each
//              rect will be 0. No assumptions should be made about the order
//              of the rects; the first rect may or may not be the leftmost or
//              rightmost.
//
//-----------------------------------------------------------------------------
LONG
CLineServices::CalcPositionsOfRangeOnLine(
    LONG cpStart,
    LONG cpEnd,
    LONG xShift,
    CDataAry<RECT> * paryChunks)
{
    CStackDataAry<LSQSUBINFO, 4> aryLsqsubinfo(Mt(CLineServicesCalculatePositionsOfRangeOnLine_aryLsqsubinfo_pv));
    LSTEXTCELL lsTextCell;
    LSCP lscpStart = LSCPFromCP(max(cpStart, _cpStart));
    LSCP lscpEnd = LSCPFromCP(cpEnd);
    HRESULT hr;
    BOOL fSublineReverseFlow = FALSE;
    LONG xStart;
    LONG xEnd;
    RECT rcChunk;
    LONG i;
    LSTFLOW tflow = (!_li._fRTL ? lstflowES : lstflowWS);

    Assert(paryChunks != NULL && paryChunks->Size() == 0);
    Assert(cpStart <= cpEnd);

    rcChunk.top = rcChunk.bottom = 0;

    aryLsqsubinfo.Grow(4); // Guaranteed to succeed since we're working from the stack.

    hr = THR(QueryLineCpPpoint(lscpStart, FALSE, &aryLsqsubinfo, &lsTextCell, FALSE));

    xStart = lsTextCell.pointUvStartCell.u;

    for (i = aryLsqsubinfo.Size() - 1; i >= 0; i--)
    {
        const LSQSUBINFO &qsubinfo = aryLsqsubinfo[i];
        const LSQSUBINFO &qsubinfoParent = aryLsqsubinfo[max((LONG)(i - 1), 0L)];

        if (lscpEnd < (LSCP) (qsubinfo.cpFirstSubline + qsubinfo.dcpSubline))
        {
            // lscpEnd is in this subline. Break out.
            break;
        }

        // If the subline and its parent are going in different directions
        // stuff the current range into the chunk array and move xStart to
        // the "end" (relative to the parent) of the current subline.
        if ((qsubinfo.lstflowSubline & fUDirection) !=
            (qsubinfoParent.lstflowSubline & fUDirection))
        {
            // Append the start of the chunk to the chunk array.
            rcChunk.left = xShift + xStart;

            fSublineReverseFlow = !!((qsubinfo.lstflowSubline ^ tflow) & fUDirection);

            // Append the end of the chunk to the chunk array.
            // If the subline flow doesn't match the line direction then we're
            // moving against the flow of the line and we will subtract the
            // subline width from the subline start to find the end point.
            rcChunk.right = xShift + qsubinfo.pointUvStartSubline.u + (fSublineReverseFlow ?
                            -qsubinfo.dupSubline : qsubinfo.dupSubline);

            // do some reverse flow cleanup before inserting rect into the array
            if(rcChunk.left > rcChunk.right)
            {
                Assert(fSublineReverseFlow);
                long temp = rcChunk.left;
                rcChunk.left = rcChunk.right + 1;
                rcChunk.right = temp + 1;
            }

            paryChunks->AppendIndirect(&rcChunk);

            xStart = qsubinfo.pointUvStartSubline.u + (fSublineReverseFlow ? 1 : -1);
        }
    }

    aryLsqsubinfo.Grow(4); // Guaranteed to succeed since we're working from the stack.

    hr = THR(QueryLineCpPpoint(lscpEnd, FALSE, &aryLsqsubinfo, &lsTextCell, FALSE));

    xEnd = lsTextCell.pointUvStartCell.u;
    if (lscpEnd > lsTextCell.cpEndCell)
    {
        xEnd += ( (aryLsqsubinfo.Size() == 0 ||
                  !((aryLsqsubinfo[aryLsqsubinfo.Size() - 1].lstflowSubline ^ tflow) & fUDirection)) ?
                  lsTextCell.dupCell : -lsTextCell.dupCell );
    }

    for (i = aryLsqsubinfo.Size() - 1; i >= 0; i--)
    {
        const LSQSUBINFO &qsubinfo = aryLsqsubinfo[i];
        const LSQSUBINFO &qsubinfoParent = aryLsqsubinfo[max((LONG)(i - 1), 0L)];

        if (lscpStart >= qsubinfo.cpFirstSubline)
        {
            // lscpStart is in this subline. Break out.
            break;
        }

        // If the subline and its parent are going in different directions
        // stuff the current range into the chunk array and move xEnd to
        // the "start" (relative to the parent) of the current subline.
        if ((qsubinfo.lstflowSubline & fUDirection) !=
            (qsubinfoParent.lstflowSubline & fUDirection))
        {
            fSublineReverseFlow = !!((qsubinfo.lstflowSubline ^ tflow) & fUDirection);

            if (xEnd != qsubinfo.pointUvStartSubline.u)
            {
                // Append the start of the chunk to the chunk array.
                rcChunk.left = xShift + qsubinfo.pointUvStartSubline.u;
 
                // Append the end of the chunk to the chunk array.
                rcChunk.right = xShift + xEnd;

                // do some reverse flow cleanup before inserting rect into the array
                if(rcChunk.left > rcChunk.right)
                {
                    Assert(fSublineReverseFlow);
                    long temp = rcChunk.left;
                    rcChunk.left = rcChunk.right + 1;
                    rcChunk.right = temp + 1;
                }

                paryChunks->AppendIndirect(&rcChunk);
            }

            // If the subline flow doesn't match the line direction then we're
            // moving against the flow of the line and we will subtract the
            // subline width from the subline start to find the end point.
            xEnd = qsubinfo.pointUvStartSubline.u +
                   (fSublineReverseFlow ? -(qsubinfo.dupSubline - 1) : (qsubinfo.dupSubline - 1));
        }
    }

    rcChunk.left = xShift + xStart;
    rcChunk.right = xShift + xEnd;
    // do some reverse flow cleanup before inserting rect into the array
    if(rcChunk.left > rcChunk.right)
    {
        long temp = rcChunk.left;
        rcChunk.left = rcChunk.right + 1;
        rcChunk.right = temp + 1;
    }
    paryChunks->AppendIndirect(&rcChunk);

    return paryChunks->Size();
}


//+----------------------------------------------------------------------------
//
// Member:      CLineServices::CalcRectsOfRangeOnLine
//
// Synopsis:    Find the position of a stretch of text starting at cpStart and
//              and running to cpEnd, inclusive. The text may be broken into
//              multiple runs if different font sizes or styles are used, or there
//              is mixed directionallity in it.
//
// Returns:     The number of chunks in the range. Usually this will just be
//              one. If an error occurs it will be zero. The actual width of
//              text (in device units) is returned in paryChunks as rects of
//              offsets from the beginning of the line. 
//              No assumptions should be made about the order of the chunks;
//              the first chunk may or may not be the chunk which includes
//              cpStart.
//
//-----------------------------------------------------------------------------
LONG
CLineServices::CalcRectsOfRangeOnLine(
    LONG cpStart,
    LONG cpEnd,
    LONG xShift,
    LONG yPos,
    CDataAry<RECT> * paryChunks,
    DWORD dwFlags)
{
    CStackDataAry<LSQSUBINFO, 4> aryLsqsubinfo(Mt(CLineServicesCalculateRectsOfRangeOnLine_aryLsqsubinfo_pv));
    HRESULT hr;
    LSTEXTCELL lsTextCell;
    LSCP lscpRunStart = LSCPFromCP(max(cpStart, _cpStart));
    LSCP lscpEnd = min(LSCPFromCP(cpEnd), _lscpLim);
    BOOL fSublineReverseFlow;
    LSTFLOW tflow = (!_li._fRTL ? lstflowES : lstflowWS);
    LONG xStart;
    LONG xEnd;
    LONG yTop = 0;
    LONG yBottom = 0;
    RECT rcChunk;
    RECT rcLast = { 0 };
    COneRun * porCurrent = _listCurrent._pHead;

    // we should never come in here with an LSCP that is in the middle of a COneRun. Those types (for selection)
    // should go through CalcPositionsOfRangeOnLine.
    
    // move quickly to the por that has the right lscpstart
    while(porCurrent->_lscpBase < lscpRunStart)
    {
        porCurrent = porCurrent->_pNext;

        // If we assert here, something is messed up. Please investigate
        Assert(porCurrent != NULL);
        // if we reached the end of the list we need to bail out.
        if(porCurrent == NULL)
            return paryChunks->Size();
    }

    // for selection we want to start highlight invalidation at beginning of the run
    // to avoid vertical line turds with RTL text.
    if(dwFlags & RFE_SELECTION)
        lscpRunStart = porCurrent->_lscpBase;

    Assert(paryChunks != NULL && paryChunks->Size() == 0);
    Assert(cpStart <= cpEnd);

    while(lscpRunStart < lscpEnd)
    {
        // if we reached the end of the list we need to bail out.
        if(porCurrent == NULL)
            return paryChunks->Size();

        switch(porCurrent->_fType)
        {
        case ONERUN_NORMAL:
            {
                aryLsqsubinfo.Grow(4); // Guaranteed to succeed since we're working from the stack.

                hr = THR(QueryLineCpPpoint(lscpRunStart, FALSE, &aryLsqsubinfo, &lsTextCell, FALSE));

                // 1. If we failed, return what we have so far
                // 2. If LS returns less than the cell we have asked for we have a problem with LS.
                //    To solve this problem (especially for Vietnamese) we will go to the next
                //    COneRun in the list and try again.
                if(hr || lsTextCell.cpStartCell < lscpRunStart)
                {
                    lscpRunStart = porCurrent->_lscpBase + porCurrent->_lscch;
                    break;
                }

                long  nDepth = aryLsqsubinfo.Size() - 1;
                Assert(nDepth >= 0);
                const LSQSUBINFO &qsubinfo = aryLsqsubinfo[nDepth];
                WHEN_DBG( DWORD dwIDObj = qsubinfo.idobj );
                long lAscent = qsubinfo.heightsPresRun.dvAscent;

                // now set the end position based on which way the subline flows
                fSublineReverseFlow = !!((qsubinfo.lstflowSubline ^ tflow) & fUDirection);

                xStart = lsTextCell.pointUvStartCell.u + (!_li._fRTL ? 0 : -1);

                // If we're querying for a character which cannot be measured (e.g. a
                // section break char), then LS returns the last character it could
                // measure. Therefore, if we are okay, use dupRun for the distance.
                // Otherwise, query the last character of porCurrent. This prevents us
                // having to loop when LS creates dobj's that are cch of five (5).
                if((LSCP)(porCurrent->_lscpBase + porCurrent->_lscch) <= (LSCP)(qsubinfo.cpFirstRun + qsubinfo.dcpRun))
                {
                    xEnd = xStart + (!fSublineReverseFlow
                                     ? qsubinfo.dupRun
                                     : -qsubinfo.dupRun);
                }
                else
                {
                    aryLsqsubinfo.Grow(4); // Guaranteed to succeed since we're working from the stack.
                    long lscpLast = min(lscpRunStart + porCurrent->_lscch, _lscpLim);

                    hr = THR(QueryLineCpPpoint(lscpLast, FALSE, &aryLsqsubinfo, &lsTextCell, FALSE));

                    // BUGBUG: (paulnel) LineServices ignores hyphens as a part of the line...even though they say it is in the subline
                    // if we querried on a valid lscp, yet LS give us back a lesser value, add the width of the last cell given.
                    xEnd = lsTextCell.pointUvStartCell.u + (!_li._fRTL ? 0 : -1) + 
                               (lsTextCell.cpStartCell >= lscpLast ? 0 : lsTextCell.dupCell);
                }

                // get the top and bottom for the rect
                if(porCurrent->_fCharsForNestedLayout)
                {
// NOTICE: Absolutely positioned, aligned, and Bold elements are ONERUN_ANTISYNTH types. See note below
                    Assert(dwIDObj == LSOBJID_EMBEDDED);
            
                    RECT rc;
                    long cyHeight;
                    const CCharFormat* pCF = porCurrent->GetCF();
                    CTreeNode *pNodeCur = porCurrent->_ptp->Branch();
                    CLayout *pLayout = pNodeCur->GetUpdatedLayout();

                    pLayout->GetRect(&rc);

                    cyHeight = rc.bottom - rc.top;

                    // XProposed and YProposed have been set to the amount of margin
                    // the layout has.                
                    xStart = pLayout->GetXProposed();
                    xEnd = xStart + (rc.right - rc.left);

                    yTop = yPos + _pMeasurer->_li._yBeforeSpace + pLayout->GetYProposed();
                    yBottom = yTop + cyHeight;

                    // take care of any nested relatively positioned elements
                    if(   pCF->_fRelative 
                       && (dwFlags & RFE_NESTED_REL_RECTS))
                    {
                        long xRelLeft = 0, yRelTop = 0;

                        // get the layout's relative positioning to its parent. The parent's relative
                        // positioning would be adjusted in RegionFromElement
                        CTreeNode * pNodeParent = pNodeCur->Parent();
                        if(pNodeParent)
                        {
                            pNodeCur->GetRelTopLeft(pNodeParent->Element(), _pci, &xRelLeft, &yRelTop);
                        }

                        xStart += xRelLeft;
                        xEnd += xRelLeft;
                        yTop += yRelTop;
                        yBottom += yRelTop;
                    }
                
                }
                else
                {
                    Assert(dwIDObj == LSOBJID_TEXT || dwIDObj == LSOBJID_GLYPH);
                    const CCharFormat* pCF = porCurrent->GetCF();
                 
                    // The current character does not have height. Throw it out.
                    // 
                    if(lAscent == 0)
                        break;

                    yBottom = yPos + _pMeasurer->_li._yHeight 
                              - _pMeasurer->_li._yDescent + _pMeasurer->_li._yTxtDescent;

                    yTop = yBottom - _pMeasurer->_li._yTxtDescent - lAscent;

                    // If we are ruby text, adjust the height to the correct position above the line.
                    if(pCF->_fIsRubyText)
                    {
                        RubyInfo *pRubyInfo = GetRubyInfoFromCp(porCurrent->_lscpBase);

                        if(pRubyInfo)
                        {
                            yBottom = yPos + _pMeasurer->_li._yHeight - _pMeasurer->_li._yDescent 
                                + pRubyInfo->yDescentRubyBase - pRubyInfo->yHeightRubyBase;
                            yTop = yBottom - pRubyInfo->yDescentRubyText - lAscent;
                        }
                    }
                }

                rcChunk.left = xShift + xStart;
                rcChunk.top = yTop;
                rcChunk.bottom = yBottom;
                rcChunk.right = xShift + xEnd;

                // do some reverse flow cleanup before inserting rect into the array
                if(rcChunk.left > rcChunk.right)
                {
                    long temp = rcChunk.left;
                    rcChunk.left = rcChunk.right;
                    rcChunk.right = temp;
                }
                
                // In the event we have <A href="x"><B>text</B></A> we get two runs of
                // the same rect. One for the Anchor and one for the bold. These two
                // rects will xor themselves when drawing the wiggly and look like they
                // did not select. This patch resolves this issue for the time being.
                if( !(rcChunk.left == rcLast.left && 
                      rcChunk.right == rcLast.right &&
                      rcChunk.top == rcLast.top &&
                      rcChunk.bottom == rcLast.bottom))
                    paryChunks->AppendIndirect(&rcChunk);

                rcLast = rcChunk;

                lscpRunStart = porCurrent->_lscpBase + porCurrent->_lscch;
            }
            break;

        case ONERUN_SYNTHETIC:
            // We want to set the lscpRunStart to move to the next start position
            // when dealing with synthetics (reverse objects, etc.)
            lscpRunStart = porCurrent->_lscpBase + porCurrent->_lscch;
            break;

        case ONERUN_ANTISYNTH:
            // NOTICE:
            // this case covers absolutely positioned elements and aligned elements
            // However, per BrendanD and SujalP, this is not the correct place
            // to implement focus rects for these elements. Some
            // work needs to be done to the CAdorner to properly 
            // handle absolutely positioned elements. RegionFromElement should handle
            // frames for aligned objects.
            break;

        default:
            Assert("Missing COneRun type");
            break;
        }
        
        porCurrent = porCurrent->_pNext;
    }

    return paryChunks->Size();
}

//-----------------------------------------------------------------------------
//
// Member:      CLineServices::RecalcLineHeight()
//
// Synopsis:    Reset the height of the the line we are measuring if the new
//              run of text is taller than the current maximum in the line.
//
//-----------------------------------------------------------------------------
void CLineServices::RecalcLineHeight(CCcs * pccs, CLine *pli)
{
    AssertSz(pli,  "we better have a line!");
    AssertSz(pccs, "we better have a some metric's here");

    if(pccs)
    {
        SHORT yAscent;
        SHORT yDescent;

        pccs->GetBaseCcs()->GetAscentDescent(&yAscent, &yDescent);

        if(yAscent < pli->_yHeight - pli->_yDescent)
            yAscent = pli->_yHeight - pli->_yDescent;
        if(yDescent > pli->_yDescent)
            pli->_yDescent = yDescent;

        pli->_yHeight        = yAscent + pli->_yDescent;

        Assert(pli->_yHeight >= 0);
    }
}


//-----------------------------------------------------------------------------
//
// Member:      TestFortransitionToOrFromRelativeChunk
//
// Synopsis:    Test if we are transitioning from a relative chunk to normal
//                chunk or viceversa
//
//-----------------------------------------------------------------------------
void
CLineServices::TestForTransitionToOrFromRelativeChunk(
    CLSMeasurer& lsme,
    BOOL fRelative,
    CElement *pElementLayout)
{
    CTreeNode * pNodeRelative = NULL;
    CElement  * pElementFL = _pFlowLayout->ElementOwner();

    // if the current line is relative and the chunk is not
    // relative or if the current line is not relative and the
    // the current chunk is relative then break out.
    if(fRelative)
    {
        pNodeRelative = lsme.CurrBranch()->GetCurrentRelativeNode(pElementFL);
    }
    if (DifferentScope(_pElementLastRelative, pNodeRelative))
    {
        UpdateLastChunkInfo(lsme,
                            pNodeRelative->SafeElement(),
                            pElementLayout
                           );
    }
}

//-----------------------------------------------------------------------------
//
// Member:        UpdateLastChunkInfo
//
// Synopsis:    We have just transitioned from a relative chunk to normal chunk
//              or viceversa, or inbetween relative chunks, so update the last
//              chunk information.
//
//-----------------------------------------------------------------------------
void
CLineServices::UpdateLastChunkInfo(
    CLSMeasurer& lsme,
    CElement *pElementRelative,
    CElement *pElementLayout )
{
    LONG xPosCurrChunk;

    if (long(lsme.GetCp()) > _cpLastChunk)
    {
        xPosCurrChunk = AddNewChunk(lsme.GetCp());
    }
    else
    {
        xPosCurrChunk = _xPosLastChunk;
    }

    _cpLastChunk = lsme.GetCp();
    _xPosLastChunk = xPosCurrChunk;
    _pElementLastRelative = pElementRelative;
    _fLastChunkSingleSite =    pElementLayout
                            && pElementLayout->IsOwnLineElement(_pFlowLayout);
    _fLastChunkHasBulletOrNum = pElementRelative && pElementRelative->IsTagAndBlock(ETAG_LI);
}


//-----------------------------------------------------------------------------
//
// Member:      AddNewChunk
//
// Synopsis:    Adds a new chunk of either relative or non-relative text to
//              the line.
//
//-----------------------------------------------------------------------------
LONG
CLineServices::AddNewChunk(LONG cp)
{
    BOOL fRTLFlow = FALSE;
    BOOL fRTLDisplay = _pFlowLayout->GetDisplay()->IsRTL();
    CLSLineChunk * plcNew = new CLSLineChunk();
    LONG xPosCurrChunk = CalculateXPositionOfCp(cp, FALSE, &fRTLFlow);

    // BUGBUG (PaulNel) This is only a bandaid. Some serious consideration needs to be
    // given to handling relative text areas that have flows in different directions.
    // It is not too difficult to come up with a case where chunks would be discontiguous.
    // How should that be handled? The coding to take care of this situation will be
    // significantly intrusive, but should be taken care of, nonetheless. Perhaps IE6
    // timeframe will be best to address this issue.

    if(_li._fRTL != (unsigned)fRTLFlow)
    {
        AdjustXPosOfCp(fRTLDisplay, _li._fRTL, fRTLFlow, &xPosCurrChunk, _li._xWidth);
        if(_li._fRTL)
            xPosCurrChunk--;
        // In a RTL display, 0 is on the right. Here we don't want to put a negative
        // value into the chunk's width, so change the sign.
        if(fRTLDisplay)
            xPosCurrChunk = -xPosCurrChunk;
    }

    if (!plcNew)
        goto Cleanup;

    plcNew->_cch             = cp  - _cpLastChunk;
    plcNew->_xWidth          = xPosCurrChunk - _xPosLastChunk;
    plcNew->_fRelative       = _pElementLastRelative != NULL;
    plcNew->_fSingleSite     = _fLastChunkSingleSite;
    plcNew->_fHasBulletOrNum = _fLastChunkHasBulletOrNum;

    // append this chunk to the line
    if(_plcLastChunk)
    {
        Assert(!_plcLastChunk->_plcNext);

        _plcLastChunk->_plcNext = plcNew;
        _plcLastChunk = plcNew;
    }
    else
    {
        _plcLastChunk = _plcFirstChunk = plcNew;
    }

Cleanup:
    return xPosCurrChunk;
}

//-----------------------------------------------------------------------------
//
// Member:      WidthOfChunksSoFarCore
//
// Synopsis:    This function finds the width of all the chunks collected so far.
//              This is needed because we need to position sites in a line relative
//              to the BOL. So if a line is going to be chunked up (because of
//              relative stuff before it), then we need to subtract their total width.
//
//-----------------------------------------------------------------------------
LONG
CLineServices::WidthOfChunksSoFarCore()
{
    CLSLineChunk *plc = _plcFirstChunk;
    LONG xWidth = 0;

    Assert(plc);
    while(plc)
    {
        xWidth += plc->_xWidth;
        plc = plc->_plcNext;
    }
    return xWidth;
}

//-----------------------------------------------------------------------------
//
//  Function:   Selected
//
//  Synopsis:   Mark the run as being selected. If selected, then also set the
//              background color.
//
//  Returns:    lserr.
//
//-----------------------------------------------------------------------------

void
COneRun::Selected(CFlowLayout *pFlowLayout, BOOL fSelected)
{
    _fSelected = fSelected;
    if (fSelected)
    {
        SetCurrentBgColor(pFlowLayout);
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   ImeHighlight
//
//  Synopsis:   Set the appropriate formatting for the run.
//
//  Returns:    Nothing
//
//  TODO:       IME Share work
//
//-----------------------------------------------------------------------------

void
COneRun::ImeHighlight(CFlowLayout *pFlowLayout, DWORD dwImeHighlight)
{
    Assert( dwImeHighlight >= 1 && dwImeHighlight <= 6 );

    switch (dwImeHighlight)
    {
        case 1: // ATTR_INPUT
        case 3: // ATTR_CONVERTED
        case 5: // ATTR_ERROR
            _fUnderlineForIME = TRUE;
            break;

        case 2: // ATTR_TARGET_CONVERTED
        case 4: // ATTR_TARGET_NOTCONVERTED
        case 6: // Hangul IME mode
            SetCurrentBgColor(pFlowLayout);
            _fUnderlineForIME = FALSE;
            break;
    }

    _dwImeHighlight = dwImeHighlight;
}

CCharFormat *
COneRun::GetOtherCF()
{
    if (!_fMustDeletePcf)
    {
        _pCF = new CCharFormat();
        if( _pCF != NULL )
            _fMustDeletePcf = TRUE;
    }
    Assert(_pCF != NULL);

    return _pCF;
}

//-----------------------------------------------------------------------------
//
// Member:      SetCurrentBgColor()
//
// Synopsis:    Set the current background color for the current chunk.
//
//-----------------------------------------------------------------------------

void
COneRun::SetCurrentBgColor(CFlowLayout *pFlowLayout)
{
    CTreeNode *pNode = Branch();
    CElement  * pElementFL = _fCharsForNestedRunOwner ?
                             Branch()->Element():
                             pFlowLayout->ElementOwner();
    const CFancyFormat * pFF;

    _fHasBackColor = TRUE;

    while(pNode)
    {
        pFF = pNode->GetFancyFormat();

        if (pFF->_ccvBackColor.IsDefined())
        {
            _ccvBackColor = pFF->_ccvBackColor;
            goto Cleanup;
        }
        else
        {
            if (DifferentScope(pNode, pElementFL))
                pNode = pNode->Parent();
            else
                pNode = NULL;
        }
    }
    
    _ccvBackColor.Undefine();

Cleanup:
    return;
}

//-----------------------------------------------------------------------------
//
// Member:      AdjustXPosOfCp()
//
// Synopsis:    used to adjust xPosofCp when flow is opposite direction
//
//-----------------------------------------------------------------------------
void
CLineServices::AdjustXPosOfCp(BOOL fRTLDisplay, BOOL fRTLLine, BOOL fRTLFlow, long* pxPosOfCp, long xLayoutWidth)
{
    if (fRTLDisplay != fRTLLine)
    {
        *pxPosOfCp = _li._xWidth - *pxPosOfCp;
    }
    if (fRTLDisplay != fRTLFlow)
    {
        *pxPosOfCp -= xLayoutWidth - 1;
    }
}

//-----------------------------------------------------------------------------
//
// Member:      GetKashidaWidth()
//
// Synopsis:    gets the width of the kashida character (U+0640) for Arabic
//              justification
//
//-----------------------------------------------------------------------------
LSERR CLineServices::GetKashidaWidth(PLSRUN plsrun, int *piKashidaWidth)
{
    LSERR lserr = lserrNone;
    HRESULT hr = S_OK;
    HDC hdc = _pci->_hdc;
    HFONT hfontOld = NULL;
    HDC hdcFontProp = NULL;
    CCcs *pccs = NULL;
    CBaseCcs *pBaseCcs = NULL;
    SCRIPT_CACHE *psc = NULL;

    SCRIPT_FONTPROPERTIES  sfp;
    sfp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);

    pccs = GetCcs(plsrun, _pci->_hdc, _pci);
    if (pccs == NULL)
    {
        lserr = lserrOutOfMemory;
        goto Cleanup;
    }
    pBaseCcs = pccs->GetBaseCcs();

    psc = pBaseCcs->GetUniscribeCache();
    Assert(psc != NULL);

    hr = ScriptGetFontProperties(hdcFontProp, psc, &sfp);
    
    AssertSz(hr != E_INVALIDARG, "You might need to update USP10.DLL");

    // Handle failure
    if(hr == E_PENDING)
    {
        Assert(hdcFontProp == NULL);

        // Select the current font into the hdc and set hdcFontProp to hdc.
        hfontOld = SelectFontEx(hdc, pBaseCcs->_hfont);
        hdcFontProp = hdc;

        hr = ScriptGetFontProperties(hdcFontProp, psc, &sfp);

    }
    Assert(hr == S_OK || hr == E_OUTOFMEMORY);

    lserr = LSERRFromHR(hr);

    if(lserr == lserrNone)
        *piKashidaWidth = max(sfp.iKashidaWidth, 1);

Cleanup:
    // Restore the font if we selected it
    if (hfontOld != NULL)
    {
        SelectFontEx(hdc, hfontOld);
    }

    return lserr;
}

