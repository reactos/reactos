/*
 *  @doc    INTERNAL
 *
 *  @module LSCOMPLX.CXX -- line services complex script support
 *
 *
 *  Owner: <nl>
 *      Michael Jochimsen <nl>
 *
 *  History: <nl>
 *      04/02/98     mikejoch created
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

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include <wchdefs.h>
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include <txtdefs.h>
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include <flowlyt.hxx>
#endif

MtDefine(CAryDirRun_pv, LineServices, "CAryDirRun::_pv");
MtDefine(CComplexRun, LineServices, "CComplexRun");
MtDefine(CBidiLine, LineServices, "CBidiLine");
MtDefine(CComplexRunComputeAnalysis_aryAnalysis_pv, Locals, "CComplexRun::ComputeAnalysis::aryAnalysis_pv");
MtDefine(CComplexRunThaiTypeBrkcls_aryItemize_pv, Locals, "CComplexRun::ThaiTypeBrkcls::aryItemize::_pv");
MtDefine(CComplexRunThaiTypeBrkcls_aryItems_pv, Locals, "CComplexRun::ThaiTypeBrkcls::aryItems::_pv");

#define DEFAULT_CHARS_TO_EVALUATE ((LONG) 100)

extern CCriticalSection g_csJitting;
extern BYTE g_bUSPJitState;

//-----------------------------------------------------------------------------
//
//  Function:   CBidiLine::CBidiLine
//
//  Synopsis:   Create a complex line. This essentially consists of
//              grabbing copies of the properties that describe the line's
//              place in the backing store, initializing the bidi stack and
//              algorithm state, and priming the DIR_RUN array. If we know the
//              length of the line already the we also go ahead and run the
//              bidi algorithm for the text in the line.
//
//-----------------------------------------------------------------------------
CBidiLine::CBidiLine(
    const CTreeInfo & treeinfo,
    LONG cp,
    BOOL fRTLPara,
    const CLine * pli) :
        _txtptr(treeinfo._pMarkup, cp)
{
#ifdef NO_IMPLICIT_BIDI
    DIR_RUN dr;

    dr.cp = cp;
    dr.iLevel = fRTLPara;
    _aryDirRun.AppendIndirect(&dr);
#else
    const CCharFormat * pCF = NULL;
    CTreeNode * ptn;
    DIR_RUN dr;
    LONG nEmbed;
    WORD wDir;
    WORD wOverride;
    LONG cchOffset;

    // Keep a copy of the flow layout and markup.
    _pFlowLayout = treeinfo._pFlowLayout;
    _pMarkup = treeinfo._pMarkup;
    Assert(_pFlowLayout != NULL && _pMarkup != NULL);

    // The cp should be in the scope of the flow layout.
    Assert(cp >= _pFlowLayout->GetContentFirstCp() && cp <= _pFlowLayout->GetContentLastCp());
    _cpFirst = _cp = cp;
    _cpLim = treeinfo._cpLayoutLast;

     // Initialize the tree pointer and _cchRemaining.
    _ptp = _pMarkup->TreePosAtCp(cp, &cchOffset);
    _cchRemaining = (_ptp->IsText() ? _ptp->Cch() - cchOffset : 0);
    Assert(_ptp != NULL && (!_ptp->IsText() || _cchRemaining > 0));

    // Initialize the bidi stack.
    _fRTLPara = fRTLPara;
    // BUGBUG SLOWBRANCH: GetBranch is very slow, but it is the best thing to
    // use here. It only gets called when the CBidiLine is created, so it isn't
    // actually prohibitive.
    ptn = _ptp->GetBranch();
    nEmbed = 0;
    wDir = 0;
    wOverride = 0;
    while (!ptn->Element()->IsBlockElement() && ptn->GetCurLayout() != _pFlowLayout)
    {
        pCF = ptn->GetCharFormat();
        if (pCF->_fBidiEmbed)
        {
            wDir = (wDir << 1) | pCF->_fRTL;
            wOverride = (wOverride << 1) | pCF->_fBidiOverride;
            nEmbed++;
        }
        ptn = ptn->Parent();
    }

    _iLevel = _fRTLPara;
    // set the static reading order state for the line
    _fVisualLine = (_pMarkup->Doc()->_fVisualOrder &&
                    !ptn->Element()->HasFlag(TAGDESC_LOGICALINVISUAL));
    _dcEmbed = (_fVisualLine || ptn->GetCharFormat()->_fBidiOverride ) 
                ? (_fRTLPara ? RLO : LRO) : (_fRTLPara ? RLE : LRE);
    _iEmbed = 0;
    _iOverflow = 0;
    while (nEmbed > 0)
    {
        // We're starting an embedding. Update the stack and _iLevel.
        BOOL fRTL = (wDir & 1);
        if (_iLevel < 14 + fRTL && _iOverflow == 0)
        {
            _aEmbed[_iEmbed++] = _dcEmbed;
            _iLevel += 1 + ((_iLevel & 1) == fRTL);
            _dcEmbed = (wOverride & 1) ? (fRTL ? RLO : LRO) : (fRTL ? RLE : LRE);
        }
        else
        {
            _iOverflow++;
        }
        wDir >>= 1;
        wOverride >>= 1;
        nEmbed--;
    }

    // Initialize the bidi algorithm state.
    _dcPrev = _dcPrevStrong = GetInitialDirClass(_iLevel & 1);

    // Initialize the run array. We always have at least one entry in the
    // current embedding level (usually the paragraph direction). We don't need
    // to worry about OOM because we're derived from a CStackDataArray with 8
    // element.
    dr.cp = cp;
    dr.iLevel = _iLevel;
    _aryDirRun.AppendIndirect(&dr);
    Assert(_aryDirRun.Size() == 1);

    // Initialize to current run to zero.
    _iRun = 0;

    if (pli != NULL)
    {
        // If we know the length of the line already precompute the runs.
        Assert(cp + pli->_cch <= _cpLim);
        if (pli->_cch > 0)
        {
            EvaluateLayoutToCp(cp + pli->_cch - 1, cp + pli->_cch - 1);
            AdjustTrailingWhitespace(cp + pli->_cch - 1);
        }
    }

#if DBG==1
    _fStringLayout = FALSE;
#endif
#endif // NO_IMPLICIT_BIDI
}


//-----------------------------------------------------------------------------
//
//  Function:   CBidiLine::CBidiLine
//
//  Synopsis:   Create a complex line from a text buffer. This is used to get
//              the ordering for bidi numbering. We basically just zap out most
//              of the properties and pass the text straight into the bidi
//              algorithm.
//
//-----------------------------------------------------------------------------
CBidiLine::CBidiLine(
    BOOL fRTLPara,
    LONG cchText,
    const WCHAR * pchText)
{
#ifdef NO_IMPLICIT_BIDI
    DIR_RUN dr;

    dr.cp = 0;
    dr.iLevel = fRTLPara;
    _aryDirRun.AppendIndirect(&dr);
#else
    DIR_RUN dr;

    // Nuke the flow layout and markup.
    _pFlowLayout = NULL;
    _pMarkup = NULL;

    // Set CPs. These are just indices into pchText.
    _cpFirst = _cp = 0;
    _cpLim = cchText - 1;

     // Nuke the tree pointer and _cchRemaining.
    _ptp = NULL;
    _cchRemaining = 0;

    // Initialize the bidi stack.
    _fRTLPara = fRTLPara;
    _iLevel = _fRTLPara;
    _dcEmbed = _fRTLPara ? RLE : LRE;
    _iEmbed = 0;
    _iOverflow = 0;

    // Initialize the bidi algorithm state.
    _dcPrev = _dcPrevStrong = (_iLevel & 1) ? RTL : LTR;

    // Initialize the run array. We always have at least one entry in the
    // current embedding level. We don't need to worry about OOM because we're
    // derived from a CStackDataArray with 8 elements.
    dr.cp = 0;
    dr.iLevel = _iLevel;
    _aryDirRun.AppendIndirect(&dr);
    Assert(_aryDirRun.Size() == 1);

    // Initialize to current run to zero.
    _iRun = 0;

    // We've already got the text so compute the runs.
    EvaluateLayout(pchText, cchText, BLK, BLK, 0);
    _cp += cchText;

#if DBG==1
    _fStringLayout = TRUE;
#endif
#endif // NO_IMPLICIT_BIDI
}


#if DBG==1
//-----------------------------------------------------------------------------
//
//  Function:   CBidiLine::IsEqual
//
//  Synopsis:   Check to see if the line which would be created using the
//              passed parameters is the same as the current line.
//
//  Returns:    TRUE if the lines would be the same, FALSE if not.
//
//-----------------------------------------------------------------------------
BOOL CBidiLine::IsEqual(
    const CTreeInfo & treeinfo,
    LONG cp,
    BOOL fRTLPara,
    const CLine * pli) const
{
#ifdef NO_IMPLICIT_BIDI
    return _aryDirRun[0].cp == cp;
#else
    Assert(!_fStringLayout);
    return _pFlowLayout == treeinfo._pFlowLayout &&
           _pMarkup == treeinfo._pMarkup &&
           _cpFirst == cp &&
           _cpLim == treeinfo._cpLayoutLast &&
           _fRTLPara == (unsigned)fRTLPara;
#endif // NO_IMPLICIT_BIDI
}
#endif


//-----------------------------------------------------------------------------
//
//  Function:   CBidiLine::GetInitialDirClass
//
//  Synopsis:   Figure out the start state for the bidi algorithm, based on the
//              line direction, document charset, and system locale.
//
//  Returns:    The initial DIRCLS used to prime the bidi algorithm.
//
//-----------------------------------------------------------------------------
DIRCLS
CBidiLine::GetInitialDirClass(
    BOOL fRTLLine)
{
    if (!fRTLLine)
    {
        // LTR lines always start with a LTR DIRCLS.
        return LTR;
    }

    CODEPAGE codepage = _pFlowLayout->Doc()->GetCodePage();

    switch (codepage)
    {
    case CP_ASMO_708:
    case CP_ASMO_720:
    case CP_1256:
    case CP_ISO_8859_6:
        // If the charset is Arabic, set the DIRCLS to be ARA.
        return ARA;
    case CP_UNDEFINED:
    case CP_UCS_2:
    case CP_UCS_2_BIGENDIAN:
    case CP_AUTO:
    case CP_UTF_7:
    case CP_UTF_8:
    case CP_UCS_4:
    case CP_UCS_4_BIGENDIAN:
        // If the charset is ambiguous, and the system locale is Arabic, set
        // the DIRCLS to ARA.
        if(IsInArabicBlock(PRIMARYLANGID(g_lcidUserDefault)))
        {
            return ARA;
        }
    }

    // In all other cases, set the DIRCLS to RTL for RTL lines.
    return RTL;
}


//-----------------------------------------------------------------------------
//
//  Function:   CBidiLine::GetRunCchRemaining
//
//  Synopsis:   Gets the number of characters remaining in the DIR_RUN at cp,
//              limited by cchMax. This will just look in the run array if
//              possible, but if cp is beyond the end of the run array then we
//              will extend the run array by analyzing the text with the bidi
//              algorithm.
//
//  Returns:    The number of characters remaining in the DIR_RUN or cchMax,
//              whichever is greater.
//
//-----------------------------------------------------------------------------
LONG
CBidiLine::GetRunCchRemaining(
    LONG cp,
    LONG cchMax)
{
#ifdef NO_IMPLICIT_BIDI
    return cchMax;
#else
    LONG iRun;
    LONG cch;

    Assert(!_fStringLayout);
    Assert(cp >= _cpFirst && cp <= _cpLim);
    Assert(cchMax > 0);

    // If we don't have the requested run evaluate it with the bidi algorithm.
    if (cp + cchMax > _cp)
    {
        EvaluateLayoutToCp(cp + cchMax - 1);
        Assert(cp < _cp);
    }

    // Find the requested run.
    iRun = FindRun(cp);
    Assert(iRun >= 0 && iRun < _aryDirRun.Size());

    // Get the length of the requested run.
    cch = ((iRun == _aryDirRun.Size() - 1) ? _cp : _aryDirRun[iRun + 1].cp) - cp;

    return min(cchMax, cch);
#endif // NO_IMPLICIT_BIDI
}


//-----------------------------------------------------------------------------
//
//  Function:   CBidiLine::GetDirRun
//
//  Synopsis:   Gets the DIR_RUN at cp.
//
//  Returns:    The DIR_RUN at cp.
//
//-----------------------------------------------------------------------------
const DIR_RUN &
CBidiLine::GetDirRun(
    LONG cp)
{
#ifdef NO_IMPLICIT_BIDI
    return _aryDirRun[0];
#else
    LONG iRun;

    Assert(!_fStringLayout);
    Assert(cp >= _cpFirst && cp <= _cpLim);

    // If we don't have the requested run evaluate it with the bidi algorithm.
    if (cp >= _cp)
    {
        EvaluateLayoutToCp(cp);
        Assert(cp < _cp);
    }

    // Find the requested run.
    iRun = FindRun(cp);
    Assert(iRun >= 0 && iRun < _aryDirRun.Size());

    // Return the requested DIR_RUN.
    return _aryDirRun[iRun];
#endif // NO_IMPLICIT_BIDI
}


//-----------------------------------------------------------------------------
//
//  Function:   CBidiLine::FindRun
//
//  Synopsis:   Find the run which contains the specified cp. Set _iRun and
//              return the run value.
//
//  Returns:    The run that the cp is in.
//
//-----------------------------------------------------------------------------
LONG
CBidiLine::FindRun(
    LONG cp)
{
    Assert(!_fStringLayout);
    Assert(_aryDirRun.Size() != 0);
    Assert(cp >= _cpFirst && cp < _cp);
    Assert(_iRun >= 0 && _iRun < _aryDirRun.Size());

    // If the requested cp is in the range of _aryDirRun[_iRun] then we can
    // just return _iRun. Otherwise we'll have to search for it.
    if (_aryDirRun[_iRun].cp > cp ||
        (_iRun != _aryDirRun.Size() - 1 && _aryDirRun[_iRun + 1].cp <= cp))
    {
        LONG iRun;

        // We have to search for the correct run. If the current run (_iRun)
        // is before cp, then we start searching from _iRun (which we will
        // do most of the time), otherwise we start searching from the start of
        // the run array. We keep searching until we blow out the end of the
        // run array or we find a run beyond cp, at which point we back up
        // one run.
        for (iRun = (_aryDirRun[_iRun].cp <= cp) ? _iRun : 0;
             iRun < _aryDirRun.Size() && _aryDirRun[iRun].cp <= cp;
             iRun++);
        _iRun = iRun - 1;
    }

    // Make sure the current run is in range.
    Assert(_iRun >= 0 && _iRun < _aryDirRun.Size());

    return _iRun;
}


//-----------------------------------------------------------------------------
//
//  Function:   CBidiLine::LogicalToVisual
//
//  Synopsis:   Convert an array of characters from logical to visual order
//              using the levels stored in _aryDirRun[]. The visual string is
//              passed back in pchVisual. The conversion optionally reverse the
//              visual string, which is handy if it will eventually be rendered
//              RTL (as with RTL numbering).
//
//              The algorithm used is not necessarily the fastest out there,
//              but given that this function is primarily used for strings that
//              are no deeper than 2 levels and are virtually never longer than
//              10 characters it should suffice.
//
//-----------------------------------------------------------------------------
void CBidiLine::LogicalToVisual(
    BOOL fRTLBuffer,
    LONG cchText,
    const WCHAR * pchLogical,
    WCHAR * pchVisual)
{
    LONG iLev = 0;
    LONG iRun;
    LONG cRun = _aryDirRun.Size();
    WCHAR * pch1;
    WCHAR * pch2;
    WCHAR ch;

    Assert(_fStringLayout);

    // Copy off the logical buffer.
    CopyMemory(pchVisual, pchLogical, cchText * sizeof(WCHAR));

    // Find run with the deepest level
    for (iRun = 0; iRun < cRun; iRun++)
    {
        iLev = max(iLev, _aryDirRun[iRun].iLevel);
    }

    // For each level, walk through the runs.

    while (iLev > 0)
    {
        for (iRun = 0; iRun < cRun; iRun++)
        {
            // If the run is as deep as the current level, reverse the text
            // under it.
            if (_aryDirRun[iRun].iLevel >= iLev)
            {
                pch1 = pchVisual + _aryDirRun[iRun].cp;
                // Look forward for a run which is shallower than the current
                // level.
                while (++iRun < cRun && _aryDirRun[iRun].iLevel >= iLev);
                pch2 = pchVisual + (iRun == cRun ? _cp : _aryDirRun[iRun].cp) - 1;
                while (pch1 < pch2)
                {
                    ch = *pch1;
                    *pch1 = *pch2;
                    *pch2 = ch;
                    pch1++;
                    pch2--;
                }
                iRun--;
            }
        }
        iLev--;
    }

    // If the fRTLBuffer flag is set reverse the entire buffer.
    if (fRTLBuffer)
    {
        pch1 = pchVisual;
        pch2 = pchVisual + cchText - 1;
        while (pch1 < pch2)
        {
            ch = *pch1;
            *pch1 = *pch2;
            *pch2 = ch;
            pch1++;
            pch2--;
        }
    }
}


//-----------------------------------------------------------------------------
//
//  Function:   CBidiLine::EvaluateLayoutToCp
//
//  Synopsis:   Evaluates the text beginning at _cp up to (and including)
//              cpLine using the bidi algorithm. If cpLine is -1, we evaluate
//              the next DEFAULT_CHARS_TO_EVALUATE characters, limited by
//              _cpLim. We also guarentee that we at least evaluate as far as
//              cp.
//
//-----------------------------------------------------------------------------
void
CBidiLine::EvaluateLayoutToCp(
    LONG cp,
    LONG cpLine)
{
    WCHAR ach[DEFAULT_CHARS_TO_EVALUATE];
    LONG cch;
    const WCHAR * pch;
    const WCHAR * pchLim;
    DIRCLS dcTrail;
    DIRCLS dcTrailForNumeric;
    LONG cchFetch;

    Assert(cp >= _cpFirst && cp <= _cpLim &&
           (cpLine == -1 || (cpLine >= _cpFirst && cpLine <= _cpLim)));

    // While we haven't processed the requested cp...
    while (_cp <= cp)
    {
        // Fetch some text from the store
        cchFetch = (cpLine == -1) ?
                   DEFAULT_CHARS_TO_EVALUATE :
                   min(DEFAULT_CHARS_TO_EVALUATE, (LONG) (cpLine - _cp + 1));
        cch = GetTextForLayout(ach, cchFetch, _cp, &_ptp, &_cchRemaining);
        Assert(cch > 0 && cch <= cchFetch);

        // Find the last non-node character
        for (pch = ach + cch; pch-- > ach && *pch == WCH_NODE; );

        // If there was some non-node text in the retreived data...
        if (pch >= ach)
        {
            BOOL fNeedNumericPunctResolution;

            // Get the DIRCLS of the final character.
            dcTrail = DirClassFromCh(*pch);

            // If the final DIRCLS is a ETM, ESP, or CSP then we'll need to
            // find the character that resolves it.
            fNeedNumericPunctResolution = IsNumericPunctuationClass(dcTrail);
            dcTrailForNumeric = dcTrail;

            // If the final DIRCLS is neutral we'll need to look beyond it to
            // resolve it in EvaluateLayout().
            if (IsIndeterminateClass(dcTrail))
            {
#define TRAIL_BUFFER 32
                CTreePos * ptpTrail;
                LONG cchTrailRemaining;
                LONG cpTrail;
                LONG cchTrail;
                WCHAR achTrail[TRAIL_BUFFER];

                // Record the position of the end of the fetch. We'll start
                // looking from here.
                ptpTrail = _ptp;
                cchTrailRemaining = _cchRemaining;
                cpTrail = _cp + cch;

                if (ptpTrail->IsBeginNode() && cchTrailRemaining > 1)
                {
                    // We're in the midst of an embedding. Go ahead and skip
                    // over all the embedding text, since it will all evaluate
                    // as neutrals and waste lots of our time.

                    // We should never need numeric punctuation; since we
                    // should always be preceeded by a WCH_EMBEDDING (neutral).
                    Assert(!fNeedNumericPunctResolution);
                    cpTrail += cchTrailRemaining - 1;
                    cchTrailRemaining = 1;
                }

                do
                {
                    Assert(cpTrail >= _cpFirst && cpTrail <= _cpLim);

                    // Fetch some more text from the store.
                    cchTrail = GetTextForLayout(achTrail, TRAIL_BUFFER, cpTrail,
                                                &ptpTrail, &cchTrailRemaining);
                    Assert(cchTrail > 0 && cchTrail <= TRAIL_BUFFER);

                    // Advance cpTrail. ptpTrail and cchTrailRemaining have
                    // already been advanced.
                    cpTrail += cchTrail;

                    // Search until we find a non-neutral class (or we run out
                    // of text). Note that WCH_NODE evaluates as neutral.
                    for (pch = achTrail, pchLim = achTrail + cchTrail;
                         pch < pchLim && IsIndeterminateClass(dcTrail);
                         pch++)
                    {
                        dcTrail = DirClassFromCh(*pch);
                        if (dcTrail == FMT)
                        {
                            // FMT covers all the embedding control characters.
                            // We need to differentiate between them.
                            Assert(InRange(*pch, WCH_LRE, WCH_RLO));
                            dcTrail = s_adcEmbedding[*pch - WCH_LRE];
                        }

                        // If we need to find a character to resolve a ETM,
                        // ESP, or CSP then check for it here. Note that ESP
                        // and CSP are resolved by whatever the next character
                        // is, while ETM needs the next non-ETM character.
                        if (fNeedNumericPunctResolution &&
                            *pch != WCH_NODE &&
                            (dcTrail != ETM || IsNumericSeparatorClass(dcTrailForNumeric)))
                        {
                            dcTrailForNumeric = dcTrail;
                            fNeedNumericPunctResolution = FALSE;
                        }
                    }

                // If that pass failed to find a dcTrail that would resolve
                // things suck in some more text.
                } while (IsIndeterminateClass(dcTrail));
            }

            // This flag should always get cleared in the above loop.
            Assert(!fNeedNumericPunctResolution);

            // Let 'er rip! _cp is only needed to record the DIR_RUNs.
            EvaluateLayout(ach, cch, dcTrail, dcTrailForNumeric, _cp);
        }

        // We've now added all cch characters to _aryDirRun[]. Move _cp ahead.
        _cp += cch;
    }
}


//-----------------------------------------------------------------------------
//
//  Function:   CBidiLine::EvaluateLayout
//
//  Synopsis:   Evaluates pchText using the bidi algorithm. When this function
//              is called we assume that we are in the midst of processing
//              EvaluateLayout() several times, hence we carry the following
//              state:
//
//              _dcPrev         Previous resolved character class
//              _dcPrevStrong   Previous strong character class
//              _iLevel         Current level
//              _dcEmbed        Type of current embedding
//              _aEmbed         Bidi stack
//              _iEmbed         Depth of bidi stack
//              _iOverflow      Overflow from bidi stack
//              _fRTLPara       Paragraph direction
//
//              This state is updated by the algorithm to reflect the
//              processing of the text in pchText. The algorithm also updates
//              _aryDirRun to include a list of directions matching the
//              resolved levels of pchText.
//
//              WARNING! This function can only deal with text which is shorter
//              than DEFAULT_CHARS_TO_EVALUATE. The calling functions currently
//              constrain this so it's not a problem. If (at some future time)
//              it becomes necessary to deal with text longer than this, turn
//              on HANDLE_LONG_LAYOUT_TEXT.
//
//-----------------------------------------------------------------------------
void
CBidiLine::EvaluateLayout(
    const WCHAR * pchText,
    LONG cchText,
    DIRCLS dcTrail,
    DIRCLS dcTrailForNumeric,
    LONG cp)
{
    DIRCLS dc;
    DIRCLS dcPrev;
    const WCHAR * pchLim;
    const WCHAR * pch;
    DIRCLS adc[DEFAULT_CHARS_TO_EVALUATE];
    LONG cdc;
    DIRCLS * pdcStart = adc;
    DIRCLS * pdcLim;
    DIRCLS * pdc;
    DIRCLS * pdcNeutral;
    DIRCLS dcNumeral;
    DIRCLS dcNeutral;
    BYTE aLevel[DEFAULT_CHARS_TO_EVALUATE];
    BYTE * pLevelStart = aLevel;
    BYTE * pLevel;
    BYTE iLevel;
    BOOL fRTL = FALSE;

    // Verify our parameters
    Assert(pchText != NULL && !IsIndeterminateClass(dcTrail) && cp <= _cpLim);

    // Verify our state
    Assert(IsFinalClass(_dcPrev) && IsStrongClass(_dcPrevStrong));
    Assert((_iLevel & 1) == IsRTLEmbeddingClass(_dcEmbed));
#if DBG==1
    {
        LONG i;
        LONG iLev;

        iLev = _iLevel;
        for (i = _iEmbed - 1; i >= 0; i--)
        {
            Assert(IsEmbeddingClass(_aEmbed[i]));
            iLev -= 1 + ((iLev & 1) == IsRTLEmbeddingClass(_aEmbed[i]));
        }
        Assert(_iLevel < 16 && iLev == (long)_fRTLPara);
    }
#endif

#ifdef HANDLE_LONG_LAYOUT_TEXT
    // If there are more characters than will fit in our stack buffers
    // allocatate some working buffers off the heap.
    if (cchText > DEFAULT_CHARS_TO_EVALUATE)
    {
        pdcStart = new DIRCLS[cchText];
        pLevelStart = new BYTE[cchText];
        if (pdcStart == NULL || pLevelStart == NULL)
        {
            goto Cleanup;
        }
    }
#else
    Assert(cchText <= DEFAULT_CHARS_TO_EVALUATE);
#endif

    // Convert the characters into DIRCLSs
    pch = pchText;
    pdc = pdcStart;
    pchLim = pchText + cchText;
    for (pch = pchText; pch < pchLim; pch++)
    {
        // Skip nodes. They have no effect on layout.
        if (*pch != WCH_NODE)
        {
            dc = DirClassFromCh(*pch);
            if (dc == FMT)
            {
                // FMT covers all the embedding control characters. We need to
                // differentiate between them.
                Assert(InRange(*pch, WCH_LRE, WCH_RLO));
                dc = s_adcEmbedding[*pch - WCH_LRE];
            }
            *pdc++ = dc;
        }
    }
    cdc = pdc - pdcStart;
    pdcLim = pdc;

    // Resolve breaks, embeddings, and overrides and set base level values.
    pdc = pdcStart;
    pLevel = pLevelStart;
    while (pdc < pdcLim)
    {
        if (IsBreakOrEmbeddingClass(*pdc))
        {
            dc = *pdc;
            switch (dc)
            {
            case BLK:
                // Block breaks reset the embedding level and are resolved as
                // strong characters in the paragraph direction.
                _iEmbed = 0;
                _iLevel = _fRTLPara;
                _dcEmbed = _fVisualLine ? (_fRTLPara ? RLO : LRO) : (_fRTLPara ? RLE : LRE);

                break;
            case SEG:
                // Segment separators are set at the paragraph level and are
                // resolved as strong characters in the paragraph direction.
                // Note that the do NOT reset the stack, so we advance pdc and
                // pLevel and continue.
                *pLevel++ = (BYTE)_fRTLPara;
                *pdc++ = _fRTLPara ? RTL : LTR;
                continue;
            case PDF:
                // PDF resets the embedding level to the previous entry on the
                // stack. It is resolved as a strong character in the direction
                // of the current level before the stack is popped. Since the
                // level and dc do not necessarily match the popped values, we
                // advance pLevel and pdc and continue.
                *pLevel++ = _iLevel;
                *pdc++ = (_iLevel & 1) ? RTL : LTR;
                if (_iEmbed > 0 && _iOverflow == 0)
                {
                    _dcEmbed = _aEmbed[--_iEmbed];
                    fRTL = IsRTLEmbeddingClass(_dcEmbed);
                    _iLevel -= 1 + ((_iLevel & 1) == fRTL);
                }
                else if (_iOverflow > 0)
                {
                    _iOverflow--;
                }
                continue;
            case LRE:
            case RLE:
            case LRO:
            case RLO:
                // The embedding controls push the current embedding onto the
                // stack and set a new embedding direction. They are resolved
                // as strong characters in the direction of the embedding.
                fRTL = IsRTLEmbeddingClass(dc);
                if (_iLevel < 14 + fRTL && _iOverflow == 0)
                {
                    _aEmbed[_iEmbed++] = _dcEmbed;
                    _iLevel += 1 + ((_iLevel & 1) == fRTL);
                    _dcEmbed = dc;
                }
                else
                {
                    _iOverflow++;
                }
                break;
#if DBG==1
            default:
                Assert(FALSE);
                break;
#endif
            }
            // All block characters are resolved as strong characters.
            *pdc = (_iLevel & 1) ? RTL : LTR;
        }
        if (IsOverrideClass(_dcEmbed))
        {
            // If we're currently in an override stomp on the dc and turn it
            // into a strong character in the override direction.
            *pdc = (_dcEmbed == LRO) ? LTR : RTL;
        }
        // Record the level and advance pLevel and pdc.
        *pLevel++ = _iLevel;
        pdc++;
    }
    if (IsBreakOrEmbeddingClass(dcTrail))
    {
        // If the dcTrail class is a break or embedding turn it into the
        // appropriate strong direction.
        switch (dcTrail)
        {
        case BLK:
        case SEG:
            fRTL = _fRTLPara;
            break;
        case PDF:
            fRTL = (_iLevel & 1);
            break;
        case LRE:
        case LRO:
            fRTL = FALSE;
            break;
        case RLE:
        case RLO:
            fRTL = TRUE;
            break;
#if DBG==1
        default:
            Assert(FALSE);
            break;
#endif
        }
        dcTrail = fRTL ? RTL : LTR;
    }

    // Resolve numerals.
    dcPrev = _dcPrevStrong;
    for (pdc = pdcStart; pdc < pdcLim; pdc++)
    {
        if (IsStrongClass(*pdc))
        {
            dcPrev = *pdc;
        }
        else if (*pdc == ENM)
        {
            // European numerals are changed into ENL, ENR, or ANM depending
            // on the previous strong character.
            Assert(IsStrongClass(dcPrev));
            *pdc = s_adcNumeric[dcPrev];
        }
    }
    if (dcTrail == ENM)
    {
        // The trailing character is also changed to ENL, ENR or ANM.
        dcTrail = s_adcNumeric[dcPrev];
    }

    // Update _dcPrevStrong to the last strong class in the text.
    _dcPrevStrong = dcPrev;

    // Resolve numeric punctuation (separators and terminators).
    dcPrev = _dcPrev;
    for (pdc = pdcStart; pdc < pdcLim; pdc++)
    {
        if (IsNumericPunctuationClass(*pdc))
        {
            if (*pdc == ETM)
            {
                DIRCLS * pdcT;

                if (!IsResolvedEuroNumClass(dcPrev))
                {
                    // If the run of ETMs is bounded by a ENL or ENR then we
                    // get dcPrev set to ENL or ENR.
                    for (pdcT = pdc + 1; pdcT < pdcLim && *pdcT == ETM; pdcT++);
                    if (pdcT == pdcLim)
                    {
                        if (dcTrailForNumeric == ENM)
                        {
                            dcTrailForNumeric = s_adcNumeric[_dcPrevStrong];
                        }
                        pdcT = &dcTrailForNumeric;
                    }
                    dcPrev = *pdcT;
                }
                // Turn the ETMs into ENLs or ENRs if they are bounded on
                // either side by such a class, otherwise turn them into NEUs.
                dcNumeral = IsResolvedEuroNumClass(dcPrev) ? dcPrev : NEU;
                for (pdcT = pdc; pdcT < pdcLim && *pdcT == ETM; pdcT++)
                {
                    *pdcT = dcNumeral;
                }
            }
            else
            {
                // If a separator is bounded on both sides by numerals of the same
                // class, and it is not an ESP bounded by ANMs, turn it into such
                // a numeral. Otherwise turn it into a neutral.
                DIRCLS dcNext = (pdc + 1 < pdcLim) ? *(pdc + 1) : dcTrailForNumeric;
                if (dcPrev == dcNext &&
                    (IsResolvedEuroNumClass(dcPrev) || (dcPrev == ANM && *pdc == CSP)) )
                {
                    *pdc = dcPrev;
                }
                else
                {
                    *pdc = NEU;
                }
            }
        }
        dcPrev = *pdc;
    }

    // Resolve neutrals
    dcPrev = _dcPrev;
    pLevel = pLevelStart;
    pdcNeutral = NULL;
    for (pdc = pdcStart; pdc < pdcLim; pdc++, pLevel++)
    {
        dc = *pdc;
        if (!IsNeutralClass(dc))
        {
            Assert(IsFinalClass(dc));
            if (pdcNeutral != NULL)
            {
                // If this is character is not a neutral and it is preceeded by
                // neutrals (pdcNeutral != NULL) figure out what class the
                // neutrals should be resolved as using s_adcNeutral and
                // replace them with that class.
                Assert(pLevel - pLevelStart >= 1);
                dcNeutral = s_adcNeutral[*(pLevel - 1) & 1][dcPrev][dc];
                while (pdcNeutral < pdc)
                {
                    *pdcNeutral++ = dcNeutral;
                }
                pdcNeutral = NULL;
            }
            dcPrev = dc;
        }
        else if (pdcNeutral == NULL)
        {
            if (dc == CBN)
            {
                // If this is a combining mark let if follow the classification
                // of the previous text
                *pdc = dcPrev;
            }
            else
            {
                // If we encounter a run of neutrals record where the run starts.
                pdcNeutral = pdc;
            }
        }
    }
    if (pdcNeutral != NULL)
    {
        Assert(IsFinalClass(dcTrail));
        // Resolve trailing neutrals using dcTrail.
        dcNeutral = s_adcNeutral[_iLevel & 1][dcPrev][dcTrail];
        while (pdcNeutral < pdcLim)
        {
            *pdcNeutral++ = dcNeutral;
        }
    }

    // Update _dcPrev to the last class in the text.
    if (cdc != 0)
    {
        _dcPrev = *(pdcLim - 1);
    }

    // Convert resolved classes to levels using s_abLevelOffset.
    for (pdc = pdcStart, pLevel = pLevelStart; pdc < pdcLim; pdc++, pLevel++)
    {
        Assert(IsFinalClass(*pdc));
        *pLevel += s_abLevelOffset[*pLevel & 1][*pdc];
    }

    // Convert the resolved levels into an array of DIR_RUNs. We merge the
    // nodes back in at this time.
    Assert(_aryDirRun.Size() > 0);
    iLevel = _aryDirRun[_aryDirRun.Size() - 1].iLevel;
    for (pch = pchText, pLevel = pLevelStart; pch < pchLim; pch++, cp++)
    {
        if (*pch != WCH_NODE)
        {
            if (*pLevel != iLevel)
            {
                DIR_RUN dr;
                iLevel = *pLevel;
                dr.iLevel = iLevel;
                dr.cp = cp;
                _aryDirRun.AppendIndirect(&dr);
            }
            pLevel++;
        }
    }
    Assert(pLevel == pLevelStart + cdc);

#ifdef HANDLE_LONG_LAYOUT_TEXT
Cleanup:
    if (cchText > DEFAULT_CHARS_TO_EVALUATE)
    {
        // If we had to allocate working buffers off the heap free them now.
        if (pdcStart != NULL)
        {
            Assert(pdcStart != adc);
            delete pdcStart;
        }
        if (pLevelStart != NULL)
        {
            Assert(pLevelStart != aLevel);
            delete pLevelStart;
        }
    }
#endif
}


//-----------------------------------------------------------------------------
//
//  Function:   CBidiLine::AdjustTrailingWhitespace
//
//  Synopsis:   Adjusts the trailing whitespace on the line to follow the line
//              direction. This gets called after the bidi algorithm has been
//              run over the full line.
//
//-----------------------------------------------------------------------------
void
CBidiLine::AdjustTrailingWhitespace(
    LONG cpEOL)
{
    LONG iRun;
    BOOL fWhite;
    LONG cchValid;
    LONG cchRemaining;
    CTreePos * ptp;
    const WCHAR * pch=NULL;
    LONG cpTrail;
    LONG cp = cpEOL;

    iRun = _aryDirRun.Size() - 1;
    Assert(iRun >= 0);
    if (_aryDirRun[iRun].iLevel == (long)_fRTLPara)
    {
        // If the last run (potential whitespace) is the same as the paragraph
        // direction, then we don't need to adjust it.
        return;
    }

     // Initialize the tree pointer and cchRemaining.
    ptp = _pMarkup->TreePosAtCp(cpEOL, &cchRemaining);
    if (ptp->IsText())
    {
        cchRemaining++;
    }
    Assert(ptp != NULL && (ptp->IsText() ^ (cchRemaining == 0)));

    // Only need to go back as far as the first direction change. At this point
    // there is either non-white text or a level transition.
    cpTrail = _aryDirRun[iRun].cp;
    fWhite = TRUE;
    cchValid = 0;
    while (fWhite && cp >= cpTrail)
    {
        if (cchValid == 0)
        {
            // This is sort of goofy. GetPchReverse() doesn't actaully return
            // a valid pointer to cp, it returns a pointer to cp such that
            // there is text behind pch. This means that pch itself may point
            // into the gap or beyond the end of a block. We fix this by
            // grabbing text at cp + 1 (which is safe due to the trailing
            // WCH_NODE character) and decrementing pch.
            _txtptr.SetCp(cp + 1);
            pch = _txtptr.GetPchReverse(cchValid);
            Assert(pch != NULL);
            pch--;
        }

        AssertSz(pch != NULL, "Checking on a complaint from the compiler.");
        switch (ptp->Type())
        {
        case CTreePos::NodeBeg:
        case CTreePos::NodeEnd:
            Assert(*pch == WCH_NODE && cchRemaining == 0);
            fWhite = !ptp->Branch()->HasLayout();
            if (fWhite)
            {
                cchValid--;
                pch--;
                cp--;
            }
            break;
        case CTreePos::Text:
            if (cchRemaining == 0)
            {
                cchRemaining = ptp->Cch();
            }
            while (fWhite && cchValid && cchRemaining && cp >= cpTrail)
            {
                fWhite = (DirClassFromCh(*pch--) == WSP);
                if (fWhite)
                {
                    cchValid--;
                    cchRemaining--;
                    cp--;
                }
            }
            break;
#if DBG==1
        default:
            // The only other type we should encounter are pointers.
            Assert(ptp->Type() == CTreePos::Pointer);
            break;
#endif
        }

        // Update ptp.
        if (!cchRemaining)
        {
            ptp = ptp->PreviousTreePos();
        }
    }

    if (!fWhite || cp < cpTrail)
    {
        cp++;
    }

    Assert(cp >= cpTrail);
    if (cp == cpTrail)
    {
        // If the last run is completely whitespace just change its level.
        _aryDirRun[iRun].iLevel = _fRTLPara;
    }
    else
    {
        // Add another run that follows the paragraph direction which
        // covers the trailing whitespace.
        DIR_RUN dr;
        dr.iLevel = _fRTLPara;
        dr.cp = cp;
        _aryDirRun.AppendIndirect(&dr);
    }
}


//-----------------------------------------------------------------------------
//
//  Function:   CBidiLine::GetTextForLayout
//
//  Synopsis:   Gets cch characters from the backing store. Directional
//              embeddings are fixed up to use LRE/RLE/PDF type marks, and
//              elements with their own layout are overridden to WCH_EMBEDDING.
//              The final ptp and the number of characters remaining in it are
//              passed back to the caller.
//
//  Returns:    The number of characters in the buffer. This may be less than
//              cch if an end of line (block element, BR, etc) was encountered.
//
//-----------------------------------------------------------------------------
LONG
CBidiLine::GetTextForLayout(
    WCHAR * pch,
    LONG cch,
    LONG cp,
    CTreePos ** pptp,
    LONG * pcchRemaining)
{
    WCHAR * pchStart;
    WCHAR * pchLim;
    LONG cchRemaining = *pcchRemaining;
    CTreePos * ptp = *pptp;
    CTreeNode * ptnText = NULL;

    // Position the tree node for the text format
    ptnText = ptp->GetBranch();

    // Move the _txtptr to the requested cp.
    if ((LONG) _txtptr.GetCp() != cp)
    {
        _txtptr.SetCp(cp);
    }

    pchStart = pch;
    pchLim = pch + min((LONG) (_cpLim - cp + 1), cch);

    // Copy from the store into the buffer.
    while (pch < pchLim)
    {
        const TCHAR * pchRead;
        LONG cchFetch;

        pchRead = _txtptr.GetPch(cchFetch);

        Assert(pchRead != NULL);

        cchFetch = min(cchFetch, (LONG) (pchLim - pch));
        CopyMemory(pch, pchRead, cchFetch * sizeof(WCHAR));
        pch += cchFetch;
        if (pch < pchLim)
        {
            _txtptr.AdvanceCp(cchFetch);
        }
    }

    pch = pchStart;

    // Keep going while we have characters.
    while (pch < pchLim)
    {
        Assert(ptp != NULL);
        Assert(*pch == WCH_NODE || ptp->IsText() || ptp->IsPointer() ||
               (ptp->IsBeginNode() && cchRemaining > 0));

        switch (ptp->Type())
        {
        case CTreePos::NodeBeg:
        {
            // Handle element begin processing.
            CTreeNode * ptn = ptp->Branch();
            CElement * pElement = ptn->Element();

            Assert(ptn != NULL);

            // Update the tree node for the text format
            ptnText = ptn;

            if (pElement->HasFlag(TAGDESC_OWNLINE) ||
                pElement->Tag() == ETAG_BR ||
                pElement->IsBlockElement())
            {
                // If the element would force a new line to be created then
                // replace the node with a CR and cut short the processing.
                *pch++ = WCH_CR;
                pchLim = pch;
            }
            else
            {
                if (!pElement->HasLayout())
                {
                    // We are transitioning into an element's scope. We
                    // need to check to see if the direction level of the
                    // CCharFormat has changed.
                    const CCharFormat * pCF = ptn->GetCharFormat();

                    if (pCF->_fBidiEmbed)
                    {
                        // We've changed levels, so generate a character which
                        // describes the transition.
                        *pch = ((!pCF->_fBidiOverride) ?
                                (!pCF->_fRTL ? WCH_LRE : WCH_RLE) :
                                (!pCF->_fRTL ? WCH_LRO : WCH_RLO));
                    }
                    pch++;
                }
                else if (pElement->GetCurLayout() != _pFlowLayout)
                {
                    // We are entering an element which has its own layout. This will
                    // be handled as an embedding, so all we need to do is treat all
                    // the text under the element as a single neutral run.
                    LONG cchEmbed;
                    CTreePos * ptpEnd = NULL;

                    if (cchRemaining == 0)
                    {
                        // cchRemaining is the number of characters under the
                        // element plus the begin and end node characters.
                        pElement->GetTreeExtent(NULL, &ptpEnd);
                        Assert(ptpEnd != NULL && ptpEnd->IsEndNode() &&
                               ptpEnd->Branch()->Element() == pElement);
                        cchRemaining = ptpEnd->GetCp() - (pch - pchStart + cp) + 1;
                    }

                    cchEmbed = min(cchRemaining, (LONG) (pchLim - pch));
                    cchRemaining -= cchEmbed;
                    while (cchEmbed--)
                    {
                        *pch++ = WCH_SYNTHETICEMBEDDING;
                    }
                    if (cchRemaining == 0)
                    {
                        // Advance the ptp to the end of the element.
                        if (ptpEnd != NULL)
                        {
                            ptp = ptpEnd;
                        }
                        else
                        {
                            pElement->GetTreeExtent(NULL, &ptp);
                            Assert(ptp != NULL && ptp->IsEndNode() &&
                                   ptp->Branch()->Element() == pElement);
                        }
                    }
                }
                else
                {
                    // This is just the beginning of the flow layout. Skip it.
                    Assert(pElement->GetCurLayout() == _pFlowLayout);
                    Assert(ptp->GetCp() == _cpFirst);
                    pch++;
                }
            }
            break;
        }
        case CTreePos::NodeEnd:
        {
            // Handle element end processing.
            CTreeNode * ptn = ptp->Branch();
            CElement * pElement = ptn->Element();

            Assert(ptn != NULL);

            // Update the tree node for the text format
            ptnText = ptn->Parent();

            if (pElement->HasFlag(TAGDESC_OWNLINE) ||
                pElement->Tag() == ETAG_BR ||
                pElement->IsBlockElement() ||
                pElement->GetCurLayout() == _pFlowLayout)
            {
                // If the element would force a new line to be created then
                // replace the node with a CR and cut short the processing.
                *pch = WCH_CR;
                pchLim = pch;
            }
            else
            {
                Assert(!pElement->HasLayout());

                // We are transitioning out of an element's scope. We
                // need to check to see if the direction level of the
                // CCharFormat has changed.
                if (ptn->GetCharFormat()->_fBidiEmbed)
                {
                    // We've changed levels, so generate a PDF.
                    *pch = WCH_PDF;
                }
            }
            pch++;
            break;
        }
        case CTreePos::Text:
        {
            LONG cchText;
            Assert(ptnText != NULL);
            const CCharFormat * pCF = ptnText->GetCharFormat();

            // Return a text run.
            if (cchRemaining == 0)
            {
                cchRemaining = ptp->Cch();
            }
            cchText = min((LONG) (pchLim - pch), cchRemaining);
            if (pCF->_bCharSet == SYMBOL_CHARSET)
            {
                WCHAR * pchSymbol;
                for (pchSymbol = pch + cchText - 1;
                     pchSymbol >= pch;
                     pchSymbol--)
                {
                    *pchSymbol = WCH_SYNTHETICEMBEDDING;
                }
            }
            pch += cchText;
            cchRemaining -= cchText;
            break;
        }
#if DBG==1
        default:
            // The only other type we should encounter are pointers.
            Assert(ptp->Type() == CTreePos::Pointer);
            break;
#endif
        }

        // Update ptp.
        if (!cchRemaining)
        {
            ptp = ptp->NextTreePos();
        }
    }

    if (cchRemaining == 0 && ptp->IsText())
    {
        cchRemaining = ptp->Cch();
    }

    cch = (pch - pchStart);
    *pptp = ptp;
    *pcchRemaining = cchRemaining;

    return cch;
}


//-----------------------------------------------------------------------------
//
//  Table:      CBidiLine::s_adcEmbedding
//
//  Synopsis:   Translates an embedding, override, or PDF character into the
//              appropriate DIRCLS. This is needed because all these characters
//              are in the same partition. If they were in different partitions
//              we wouldn't need this.
//
//-----------------------------------------------------------------------------
const DIRCLS
CBidiLine::s_adcEmbedding[5] =
{
    LRE,    // WCH_LRE (LTR Embedding)
    RLE,    // WCH_RLE (RTL Embedding)
    PDF,    // WCH_PDF (Pop directional formatting)
    LRO,    // WCH_LRE (LTR Override)
    RLO,    // WCH_RLO (RTL Override)
};


//-----------------------------------------------------------------------------
//
//  Table:      CBidiLine::s_adcNumeric
//
//  Synopsis:   Translates an ENM into an ENL, ENR, or ANM based on the
//              preceeding strong character.
//
//-----------------------------------------------------------------------------
const DIRCLS
CBidiLine::s_adcNumeric[3] =
{
    ENL,    // LTR ENM
    ENR,    // RTL ENM
    ANM,    // ARA ENM
};


//-----------------------------------------------------------------------------
//
//  Table:      CBidiLine::s_adcNeutral
//
//  Synopsis:   Translates a neutral into LTR or RTL based on the base
//              embedding direction and the bounding characters. The table is
//              constructed as s_adcNeutral[fRTLEmbedding][dcLead][dcTrail].
//
//-----------------------------------------------------------------------------
const DIRCLS
CBidiLine::s_adcNeutral[2][6][6] =
{
//  Table for LTR embedding. First class is in the rows, second class is the columns.
//  LTR     RTL     ARA     ANM     ENL     ENR
    LTR,    LTR,    LTR,    LTR,    LTR,    LTR,    // LTR
    LTR,    RTL,    RTL,    RTL,    LTR,    RTL,    // RTL
    LTR,    RTL,    RTL,    RTL,    LTR,    RTL,    // ARA
    LTR,    RTL,    RTL,    RTL,    LTR,    RTL,    // ANM
    LTR,    LTR,    LTR,    LTR,    LTR,    LTR,    // ENL
    LTR,    RTL,    RTL,    RTL,    LTR,    RTL,    // ENR

//  Table for RTL embedding. First class is in the rows, second class is the columns.
//  LTR     RTL     ARA     ANM     ENL     ENR
    LTR,    RTL,    RTL,    RTL,    LTR,    RTL,    // LTR
    RTL,    RTL,    RTL,    RTL,    RTL,    RTL,    // RTL
    RTL,    RTL,    RTL,    RTL,    RTL,    RTL,    // ARA
    RTL,    RTL,    RTL,    RTL,    RTL,    RTL,    // ANM
    LTR,    RTL,    RTL,    RTL,    LTR,    RTL,    // ENL
    RTL,    RTL,    RTL,    RTL,    RTL,    RTL,    // ENR
};


//-----------------------------------------------------------------------------
//
//  Table:      CBidiLine::s_abLevelOffset
//
//  Synopsis:   Translates a DIRCLS into a level offset to be added to the base
//              embedding level. The table is constructed as
//              s_abLevelOffset[fRTLEmbedding][dc].
//
//-----------------------------------------------------------------------------
const BYTE
CBidiLine::s_abLevelOffset[2][6] =
{
//  Table for LTR embedding.
    0,  // LTR
    1,  // RTL
    1,  // ARA
    2,  // ANM
    0,  // ENL
    2,  // ENR

//  Table for RTL embedding.
    1,  // LTR
    0,  // RTL
    0,  // ARA
    1,  // ANM
    1,  // ENL
    1,  // ENR
};


//+----------------------------------------------------------------------------
//
//  Function:   CchContextFromScript
//
//  Synopsis:   We return a default script ID based on the language id passed
//              in. There is usually a 1:1 mapping here, but there are
//              exceptions for FE langs. In these cases we try to pick a unique
//              script ID.
//
//  Returns:    Script ID matching the lang.
//
//-----------------------------------------------------------------------------

static const BYTE s_acchContextFromLang[] = 
{
    0,      // LANG_NEUTRAL     0x00
    6,      // LANG_ARABIC      0x01
    0,      // LANG_BULGARIAN   0x02
    0,      // LANG_CATALAN     0x03
    0,      // LANG_CHINESE     0x04
    0,      // LANG_CZECH       0x05
    0,      // LANG_DANISH      0x06
    0,      // LANG_GERMAN      0x07
    0,      // LANG_GREEK       0x08
    0,      // LANG_ENGLISH     0x09
    0,      // LANG_SPANISH     0x0a
    0,      // LANG_FINNISH     0x0b
    0,      // LANG_FRENCH      0x0c
    4,      // LANG_HEBREW      0x0d
    0,      // LANG_HUNGARIAN   0x0e
    0,      // LANG_ICELANDIC   0x0f
    0,      // LANG_ITALIAN     0x10
    0,      // LANG_JAPANESE    0x11
    0,      // LANG_KOREAN      0x12
    0,      // LANG_DUTCH       0x13
    0,      // LANG_NORWEGIAN   0x14
    0,      // LANG_POLISH      0x15
    0,      // LANG_PORTUGUESE  0x16
    0,      //                  0x17
    0,      // LANG_ROMANIAN    0x18
    0,      // LANG_RUSSIAN     0x19
    0,      // LANG_SERBIAN     0x1a
    0,      // LANG_SLOVAK      0x1b
    0,      // LANG_ALBANIAN    0x1c
    0,      // LANG_SWEDISH     0x1d
    3,      // LANG_THAI        0x1e
    0,      // LANG_TURKISH     0x1f
    6,      // LANG_URDU        0x20
    0,      // LANG_INDONESIAN  0x21
    0,      // LANG_UKRAINIAN   0x22
    0,      // LANG_BELARUSIAN  0x23
    0,      // LANG_SLOVENIAN   0x24
    0,      // LANG_ESTONIAN    0x25
    0,      // LANG_LATVIAN     0x26
    0,      // LANG_LITHUANIAN  0x27
    0,      //                  0x28
    6,      // LANG_FARSI       0x29
    0,      // LANG_VIETNAMESE  0x2a
    0,      // LANG_ARMENIAN    0x2b
    0,      // LANG_AZERI       0x2c
    0,      // LANG_BASQUE      0x2d
    0,      //                  0x2e
    0,      // LANG_MACEDONIAN  0x2f
    0,      //                  0x30
    0,      //                  0x31
    0,      //                  0x32
    0,      //                  0x33
    0,      //                  0x34
    0,      //                  0x35
    0,      // LANG_AFRIKAANS   0x36
    0,      // LANG_GEORGIAN    0x37
    0,      // LANG_FAEROESE    0x38
    13,     // LANG_HINDI       0x39
    0,      //                  0x3a
    0,      //                  0x3b
    0,      //                  0x3c
    0,      //                  0x3d
    0,      // LANG_MALAY       0x3e
    0,      // LANG_KAZAK       0x3f
    0,      //                  0x40
    0,      // LANG_SWAHILI     0x41
    0,      //                  0x42
    0,      // LANG_UZBEK       0x43
    0,      // LANG_TATAR       0x44
    13,     // LANG_BENGALI     0x45
    13,     // LANG_PUNJABI     0x46
    13,     // LANG_GUJARATI    0x47
    13,     // LANG_ORIYA       0x48
    13,     // LANG_TAMIL       0x49
    13,     // LANG_TELUGU      0x4a
    13,     // LANG_KANNADA     0x4b
    13,     // LANG_MALAYALAM   0x4c
    13,     // LANG_ASSAMESE    0x4d
    13,     // LANG_MARATHI     0x4e
    13,     // LANG_SANSKRIT    0x4f
    0,      //                  0x50
    0,      //                  0x51
    0,      //                  0x52
    0,      //                  0x53
    0,      //                  0x54
    0,      //                  0x55
    0,      //                  0x56
    13,     // LANG_KONKANI     0x57
    13,     // LANG_MANIPURI    0x58
    6,      // LANG_SINDHI      0x59
    0,      //                  0x5a
    0,      //                  0x5b
    0,      //                  0x5c
    0,      //                  0x5d
    0,      //                  0x5e
    0,      //                  0x5f
    6,      // LANG_KASHMIRI    0x60
    13,     // LANG_NEPALI      0x61
};

BYTE
CchContextFromScript(
    WORD eScript)
{
    const SCRIPT_PROPERTIES * pProp = GetScriptProperties(eScript);
    LANGID lang;

    Assert(pProp != NULL);
    lang = PRIMARYLANGID(pProp->langid);
    if (lang >= 0 && lang < sizeof(s_acchContextFromLang) / sizeof(s_acchContextFromLang[0]))
    {
        return s_acchContextFromLang[lang];
    }
    else
    {
        return sidDefault;
    }
}


//-----------------------------------------------------------------------------
//
//  Function:   CComplexRun::ComputeAnalysis
//
//  Synopsis:   Compute the first SCRIPT_ANALYSIS in the string in pch.
//
//  Returns:    True if the run needs to be glyphed, otherwise false.
//
//-----------------------------------------------------------------------------

void
CComplexRun::ComputeAnalysis(
    const CFlowLayout * pFlowLayout,
    BOOL fRTL,
    BOOL fForceGlyphing,
    WCHAR chNext,
    WCHAR chPassword,
    COneRun * por,
    COneRun * porTail,
    DWORD uLangDigits)
{
    CStackDataAry<SCRIPT_ITEM, 64> aryItem(Mt(CComplexRunComputeAnalysis_aryAnalysis_pv));
    CStr strText;
    const WCHAR * pch = por->_pchBase;
    LONG cch = por->_lscch;
    HRESULT hr;
    INT cRuns = 0;
    INT iItem;
    WORD eScript;
    BOOL fNumeric;

    // Zap the _Analysis structure and initialize fRTL and fLogicalOrder.
    memset(&_Analysis, 0, sizeof(SCRIPT_ANALYSIS));
    _Analysis.fRTL = fRTL;
    _Analysis.fLogicalOrder = TRUE;

    // We disable glyphing when we're dealing with passwords or symbols. Also,
    // if glyphing causes crashes (Win95FE) then bypass it and return. The net
    // effect of this is that we render as if we didn't have USP loaded.
    if (chPassword || por->GetCF()->_bCharSet == SYMBOL_CHARSET ||
        g_fExtTextOutGlyphCrash)
    {
        _Analysis.eScript = SCRIPT_UNDEFINED;
        return;
    }

    if (fForceGlyphing && chNext != WCH_NULL)
    {
        if (THR(strText.Set(pch, cch)) == S_OK &&
            THR(strText.Append(&chNext, 1)) == S_OK)
        {
            pch = strText;
            cch++;
        }
    }

    // Call ScriptItemize().
    if(g_bUSPJitState == JIT_OK)
        hr = ::ScriptItemize(pch, cch, 64, NULL, NULL, &aryItem, &cRuns);
    else
        hr = E_PENDING;

    if (FAILED(hr))
    {
        if(hr == USP10_NOT_FOUND)
        {
            g_csJitting.Enter();
            if(g_bUSPJitState == JIT_OK)
            {
                g_bUSPJitState = JIT_PENDING;

                // We must do this asyncronously.
                IGNORE_HR(GWPostMethodCall(pFlowLayout->Doc(), 
                                           ONCALL_METHOD(CDoc, FaultInUSP, faultinusp), 
                                           0, FALSE, "CDoc::FaultInUSP"));

            }
            g_csJitting.Leave();
        }

        // If ScriptItemize() failed, set the script to SCRIPT_UNDEFINED. This
        // will cause the run to be handled as a regular text run, which, while
        // not acurate, may be legible.
        _Analysis.eScript = SCRIPT_UNDEFINED;
        return;
    }

    // Make sure this is the right size (cRuns + sentinel item).
    Assert(cRuns + 1 == aryItem.Size());

    eScript = aryItem[0].a.eScript;
    fNumeric = IsNumericScript(eScript);

    // Accumulate all the items which have the same script.
    for (iItem = 1; iItem < cRuns && aryItem[iItem].a.eScript == eScript; iItem++);

    // If we're not going to be glyphing this run then we can accumulate
    // additional items.
    if (!fForceGlyphing && uLangDigits == LANG_NEUTRAL && !IsComplexScript(eScript))
    {
        while (iItem < cRuns && !IsComplexScript(aryItem[iItem].a.eScript))
        {
            iItem++;
        }
    }

    Assert(iItem < cRuns + 1);

    // Set the number of characters in the por covered by the _Analysis.
    por->_lscch = min(por->_lscch, (LONG) aryItem[iItem].iCharPos);

    // Determine the script to be used for shaping and set it in the _Analysis.
    if (IsComplexScript(eScript))
    {
        // If the first item needs to be handled as complex text, find all the
        // adjacent SCRIPT_ITEMs which are of the same script.
        _Analysis.eScript = eScript;
    }
    else if (uLangDigits != LANG_NEUTRAL && (fNumeric || IsNumericSeparatorRun(por, porTail)))
    {
        // Merge all the adjacent SCRIPT_ITEMs which have the same
        // script.
        _Analysis.eScript = GetNumericScript(uLangDigits);
    }
    else
    {
        // Not a complex or numeric script. Set the script as
        // SCRIPT_UNDEFINED unless we are forcing glyphing.
        _Analysis.eScript = fForceGlyphing ? eScript : SCRIPT_UNDEFINED;
    }

    // Set the flag indicating whether or not this run needs to be glyphed.
    if (_Analysis.eScript != SCRIPT_UNDEFINED)
    {
        const CCharFormat * pCFBase = por->GetCF();
        BYTE bScriptCharSet = (BYTE)GetScriptCharSet(_Analysis.eScript);

        Assert(pCFBase != NULL);

        por->_lsCharProps.fGlyphBased = TRUE;
        por->_lsCharProps.dcpMaxContext = CchContextFromScript(_Analysis.eScript);

        // Tweak the charset of the CCharFormat to match the script.
        if (IsComplexScript(_Analysis.eScript) &&
            pCFBase->_bCharSet != bScriptCharSet)
        {
            CCharFormat * pCF;

            if (!por->_fMustDeletePcf)
            {
                // We need a custom CF for the por. Base it on the current CF.
                pCF = por->GetOtherCF();
                *pCF = *pCFBase;
            }
            else
            {
                // We've already copied out a custom CF for the por, so we can
                // just scribble on it.
                pCF = (CCharFormat *) pCFBase;
            }

            pCF->_bCharSet = bScriptCharSet;
            pCF->_bCrcFont = pCF->ComputeFontCrc();
        }
    }
    else
    {
        por->_lsCharProps.fGlyphBased = FALSE;
    }
}


//-----------------------------------------------------------------------------
//
//  Function:   CComplexRun::IsNumericSeparatorRun
//
//  Synopsis:   Determines if por is a run consisting of a single numeric
//              separator character surrounded by numbers.
//
//  Returns:    True if the run is a text run consisting a numeric separator
//              bound by numbers.
//
//-----------------------------------------------------------------------------

BOOL
CComplexRun::IsNumericSeparatorRun(
    COneRun * por,
    COneRun * porTail)
{
    // Check if we've got a single numeric separator for the run.
    if (por->_lscch != 1 || !IsNumericSeparatorClass(DirClassFromCh(*(por->_pchBase))))
    {
        return FALSE;
    }

    // Check for a number preceeding the por. We can just look back in the por
    // list to do this.
    if (porTail == por)
    {
        porTail = porTail->_pPrev;
    }
    while (porTail != NULL && !(porTail->IsNormalRun() && porTail->_pchBase != NULL))
    {
        porTail = porTail->_pPrev;
    }
    if (porTail == NULL || !InRange(*(porTail->_pchBase + porTail->_lscch - 1), '0', '9'))
    {
        return FALSE;
    }

    // Check for a number following the por. Usually we'll just be able to look
    // in the text store that was originally attached to this por (before it
    // was chopped up), but sometimes we may need to go to the backing store,
    // in which case we have to watch for both the text and the nodes (we want
    // to ignore non-displaying nodes (like <span>) but anything else causes us
    // to evaluate).
    if (por->_lscchOriginal > 1)
    {
        if (!InRange(*(por->_pchBaseOriginal + 1), '0', '9'))
        {
            return FALSE;
        }
    }
    else
    {
        CTreePos * ptp = por->_ptp;
        CTxtPtr txtptr(ptp->GetMarkup(), por->_lscpBase - por->_chSynthsBefore);
        WCHAR ch;

        while ((ch = txtptr.NextChar()) == WCH_NODE)
        {
            ptp = ptp->NextTreePos();

            while(ptp->IsPointer())
            {
                ptp = ptp->NextTreePos();
            }

            Assert(ptp->IsNode() && ptp->Branch() != NULL);

            CElement * pElement = ptp->Branch()->Element();

            if (pElement->HasFlag(TAGDESC_OWNLINE) ||
                pElement->Tag() == ETAG_BR ||
                pElement->HasLayout())
            {
                return FALSE;
            }
        }
        if (!InRange(ch, '0', '9'))
        {
            return FALSE;
        }
    }

    return TRUE;
}


//-----------------------------------------------------------------------------
//
//  Function:   ThaiTypeBrkcls
//
//  Synopsis:   Makes a SCRIPT_ANALYSIS for the run. This is used by Uniscribe
//              functions for Complex Text handling. The SCRIPT_ANALYSIS is
//              stored in the plsrun for multiple uses.
//
//  Arguments:  plsrun              pointer to the run
//              cp                  the cp to evaluate
//              pbrkclsAfter        Can we break after the current cp?
//              pbrkclsBefore       Can we break before the current cp?
//
//  Returns:    void
//
//-----------------------------------------------------------------------------

void
CComplexRun::ThaiTypeBrkcls(CMarkup * pMarkup, LONG cp, BRKCLS* pbrkclsAfter, BRKCLS* pbrkclsBefore)
{
    long    cchBefore = 0;
    long    cchAfter = 0;

    if(cp < (LONG) _ThaiWord.cp || cp >= (LONG) (_ThaiWord.cp + _ThaiWord.cch))
    {
        // The CP is not in the currently cached word. Bummer. We'll have to
        // compute the word position and length.

        HRESULT hr;
        const TCHAR * pch;
        TCHAR aryItemize[64];
        unsigned short aryNodePos[64];
        CStackDataAry<SCRIPT_ITEM, 8> aryItems(Mt(CComplexRunThaiTypeBrkcls_aryItems_pv));
        SCRIPT_LOGATTR arySLA[64];
        int     cItems;
        int     nItem;
        long    cchCtrlBefore = 0;
        long    cchCtrlAfter = 0;
        long    cch;
        long    cchTotal;
        long    cchValid;
        long    ichBreakBefore;
        long    ichBreakAfter;
        CTxtPtr tp(pMarkup, cp);
        TCHAR chCur;

        // Set up for ScriptItemize(). We need to re-itemize the string instead
        // of using the cached _Analysis struct because we don't know how many
        // characters are involved.

        // Make sure the current character is ThaiType
        Assert(IsThaiTypeChar(tp.GetChar()));

        // Advance until 16 characters have passed or a non-ThaiType character
        // is encountered
        while(cchAfter < 16)
        {
            chCur = tp.NextChar();
            if(IsThaiTypeChar(chCur))
            {
                cchAfter++;
            }
            else if(chCur == WCH_NODE)
            {
                cchCtrlAfter++;
            }
            else
            {
                break;
            }
        }

        // Back up until 47 characters have passed or a non-ThaiType character
        // is encountered
        tp.SetCp(cp);
        while(cchBefore < 47)
        {
            chCur = tp.PrevChar();
            if(IsThaiTypeChar(chCur))
            {
                cchBefore++;
            }
            else if(chCur == WCH_NODE)
            {
                cchCtrlBefore++;
            }
            else
            {
                break;
            }
        }

        // Position the tp to the start of the Thai-type text.
        tp.SetCp(cp - cchBefore - cchCtrlBefore);

        // Get a pointer to the Thai string. We'll only copy data if we must.
        pch = tp.GetPch(cchValid);
        cch = cchBefore + cchAfter + 1;
        cchTotal = cch + cchCtrlBefore + cchCtrlAfter;

        // Strip out any control characters
        long lCount = 0;
        long lTotal = 0;
        long lNode = 0;
        long cchChecked = cchValid;

        while(lCount < cch)
        {
            Assert(lTotal <= cchTotal);

            if(*pch != WCH_NODE)
            {
                Assert(IsThaiTypeChar(*pch));

                aryItemize[lCount] = *pch;
                aryNodePos[lCount] = lNode;
                lCount++;
            }
            else
            {
                lNode++;
            }
            lTotal++;
            cchValid--;

            if(cchValid > 0)
            {
                pch++;
            }
            else if(lCount < cch)
            {
                // we've encountered a gap and need to get some more 
                // characters into the pch
                tp.AdvanceCp(cchChecked);
                pch = tp.GetPch(cchValid);
                cchChecked = cchValid;
            }
        }
        Assert(cch == lCount);

        // Prepare SCRIPT_ITEM buffer
        if (FAILED(aryItems.Grow(8)))
        {
            // We should always be able to grow to 8 itemse as we are based on
            // a CStackDataAry of this size.
            Assert(FALSE);
            goto Error;
        }

        // Call ScriptItemize() wrapper in usp.cxx.
        if(g_bUSPJitState == JIT_OK)
            hr = ScriptItemize(aryItemize, cch, 16, 
                               NULL, NULL, &aryItems, &cItems);
        else
            hr = E_PENDING;

        if (FAILED(hr))
        {
            if(hr == USP10_NOT_FOUND)
            {
                g_csJitting.Enter();
                if(g_bUSPJitState == JIT_OK)
                {
                    g_bUSPJitState = JIT_PENDING;
 
                    // We must do this asyncronously.
                    IGNORE_HR(GWPostMethodCall(pMarkup->Doc(), 
                                               ONCALL_METHOD(CDoc, FaultInUSP, faultinusp), 
                                               0, FALSE, "CDoc::FaultInUSP"));

                }
                g_csJitting.Leave();
           }
            // ScriptItemize() failed (for whatever reason). We are unable to
            // break, so assume we've got a single word and return.
            goto Error;
        }

        // Find the SCRIPT_ITEM containing cp.
        for(nItem = aryItems.Size() - 1;
            cchBefore < aryItems[nItem].iCharPos;
            nItem--);
        if (nItem < 0 || nItem + 1 >= aryItems.Size())
        {
            // Somehow the SCRIPT_ITEM array has gotten screwed up. We can't
            // break, so assume we've got a single word and return.
            goto Error;
        }

        // NB (mikejoch) eScript may have been nuked in GetGlyphs(). If so then
        // this won't match up, but generally it should.
        Assert(_Analysis.eScript == SCRIPT_UNDEFINED ||
               aryItems[nItem].a.eScript == _Analysis.eScript);

        // Adjust pch and cch to correspond to the text indicated by this item.
        cch = aryItems[nItem + 1].iCharPos - aryItems[nItem].iCharPos;
        cchBefore -= aryItems[nItem].iCharPos;
        cchAfter = cch - cchBefore - 1;
        Assert(cchBefore >= 0 && cchAfter >= 0 && cchBefore + cchAfter + 1 == cch);

        // do script break
        hr = ScriptBreak(aryItemize + aryItems[nItem].iCharPos, cch,
                         (SCRIPT_ANALYSIS *) &aryItems[nItem].a,
                         arySLA);

        if (FAILED(hr))
        {
            // ScriptBreak() failed (for whatever reason). We are unable to break,
            // so assume we've got a single word and return.
            goto Error;
        }

        // begin checking for the break before on the current CP
        for(ichBreakBefore = cchBefore;
            ichBreakBefore > 0 && !arySLA[ichBreakBefore].fSoftBreak;
            ichBreakBefore--);

        _ThaiWord.cp = cp - (cchBefore - ichBreakBefore);

        for(ichBreakAfter = cchBefore + 1;
            ichBreakAfter < cch && !arySLA[ichBreakAfter].fSoftBreak;
            ichBreakAfter++);

        Assert(ichBreakAfter <= cch && ichBreakAfter - ichBreakBefore < 64);

        // get the delta between node counts in the word 
        long offset1 = ichBreakBefore + aryItems[nItem].iCharPos;
        long offset2 = ichBreakAfter + aryItems[nItem].iCharPos - 1;

        _ThaiWord.cch = (ichBreakAfter - ichBreakBefore) + 
                        (aryNodePos[offset2] - aryNodePos[offset1]);

    }

    Assert(cp >= (LONG) _ThaiWord.cp && cp < (LONG) (_ThaiWord.cp + _ThaiWord.cch));

    // Can we break before the character?
    if(cp != (LONG) _ThaiWord.cp)
    {
        *pbrkclsBefore = CLineServices::brkclsThaiMiddle;
    }
    else
    {
        *pbrkclsBefore = CLineServices::brkclsThaiFirst;
    }

    // Can we break after the character?
    if(cp != (LONG) (_ThaiWord.cp + _ThaiWord.cch - 1))
    {
        *pbrkclsAfter = CLineServices::brkclsThaiMiddle;
    }
    else
    {
        *pbrkclsAfter = CLineServices::brkclsThaiLast;
    }

    return;

Error:

    *pbrkclsBefore = (cchBefore == 0 ? CLineServices::brkclsThaiFirst : CLineServices::brkclsThaiMiddle);
    *pbrkclsAfter = (cchAfter == 0 ? CLineServices::brkclsThaiLast : CLineServices::brkclsThaiMiddle);

}

