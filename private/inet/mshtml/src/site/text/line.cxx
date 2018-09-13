/*
 *  LINE.C
 *
 *  Purpose:
 *      CLine class
 *
 *  Authors:
 *      Original RichEdit code: David R. Fulmer
 *      Christian Fortini
 *      Murray Sargent
 */

#include "headers.hxx"

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X__LINE_H_
#define X__LINE_H_
#include "_line.h"
#endif

#ifndef X_LSM_HXX_
#define X_LSM_HXX_
#include "lsm.hxx"
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

MtDefine(CLinePtr, Tree, "CLinePtr")
MtDefine(CLine, Tree, "CLine")

ExternTag(tagAssertOnHittestingWithLS);

/*
 *  CLine::CchFromXPos
 *
 *  Purpose:
 *      Computes cp corresponding to a x position in a line
 *
 *  Arguments:
 *      me                  measurer position at start of line
 *      x                   xpos to search for
 *      y                   ypos to search for
 *      pdx                 returns adjustment to x at returned cp
 *      fRTLDisplay         Is the display right-to-left?
 *      fExactFit           Do we need to fit exactly?
 *      pyHeightRubyBase    Support for ruby
 *
 *      HACKHACK (t-ramar): This function calls DiscardLine() at the 
 *              end, but we would like to keep information about Ruby Base
 *              heights around for hit testing purposes.  Thus, there is an
 *              optional parameter:
 *                  ppRubyInfo (out):  is the information pertaining to
 *                                     ruby object that contains the cp
 *                                     (NULL by default)
 *              It's also important to note that this value is ignored if
 *              line flag FLAG_HAS_RUBY is not set.  In this case the 
 *              value pointed to by ppRubyInfo will remain unchanged.
 *  Returns
 *      cp of character found
 *
 *  Note:
 *      me is moved to returned cp
 *
 *  TKTK CF
 *      this should probably be done by a CRenderer to allow
 *      left - right justification
 */
LONG CLine::CchFromXpos(CLSMeasurer& me, LONG x, LONG y, LONG *pdx, BOOL fRTLDisplay,
                        BOOL fExactFit, LONG *pyHeightRubyBase) const
{
    CMarginInfo marginInfo;
    LONG dx = 0;
    LONG cpStart = me.GetCp();
    LONG cp = cpStart;
    DWORD uiFlags = 0;
    WHEN_DBG( LONG oldcch = 0; );
    WHEN_DBG( LONG olddx  = 0; );
    BOOL fReverseFlow = FALSE;

    //
    // Bail out if the line has no charaters in it.
    //
    if (_cch == 0)
        return 0;

    if (me._pFlowLayout->IsDisplayNone())
        return 0;
    
    if ( me._pFlowLayout->GetDisplay()->GetWordWrap() && 
         me._pFlowLayout->GetDisplay()->GetWrapLongLines() )
    {
        uiFlags |= MEASURE_BREAKLONGLINES;
    }

    // Determine where x is relative to the line. LTR lines have their origin
    // at the left edge of the text in the line and progress positively to the
    // right; RTL lines have their origin at the  right edge of the text and
    // progress positively to the left.
    if(!fRTLDisplay)
    {
        // X is a positive value relative to the left edge of the display.
        if (!_fRTL)
        {
            x = x - (_xLeftMargin + _xLeft);
        }
        else
        {
            x = -(x - (_xLeftMargin + _xLeft + _xWidth - 1));
        }
    }
    else
    {
        // X is a negative value relative to the right edge of the display.
        x = -x;
        if (_fRTL)
        {
            x = x - (_xRightMargin + _xRight);
        }
        else
        {
            x = -(x - (_xRightMargin + _xRight + _xWidth - 1));
        }
    }


    me._li._cch = 0;                         // Default zero count

    if(x > 0)                          // In between right & left margins
    {
        {
            CLine *pliFirstFrag;
            LONG cpStartContainerLine;
            LSERR lserr = lserrNone;
                
            lserr = me.PrepAndMeasureLine((CLine*)this, &pliFirstFrag, &cpStartContainerLine, &marginInfo, _cch, uiFlags);

            if (lserr == lserrNone)
            {
                if (me._pLS->_plsline)
                {
                    LONG xTrailEdge;

                    Assert(pliFirstFrag->_fRTL == _fRTL);
                    if (!_fRTL)
                    {
                        x += (_xLeft + _xLeftMargin) -
                             (pliFirstFrag->_xLeft + pliFirstFrag->_xLeftMargin);
                    }
                    else
                    {
                        x += (_xRight + _xRightMargin) -
                             (pliFirstFrag->_xRight + pliFirstFrag->_xRightMargin);
                    }

                    if (pliFirstFrag != this)
                    {
                        long durWithTrailing, duIgnore;
                        HRESULT hr = me._pLS->GetLineWidth(&durWithTrailing, &duIgnore);
                        if (hr)
                            goto Cleanup;
                        xTrailEdge = durWithTrailing;
                    }
                    else
                    {
                        xTrailEdge = _xWidth + _xWhite;
                    }
                     
                    // If our point is past the end of the line, the return value of
                    // LsQueryLinePointPcp is invalid.
                    if (x < xTrailEdge)
                    {
                        HRESULT hr;
                        LSTEXTCELL lsTextCell;
                        LSTFLOW kTFlow;
                        
                        // FUTURE: (paulnel) If we find we need any more information
                        // from the LSQSUBLINEINFO we can pass it back here.
                        hr = THR( me._pLS->QueryLinePointPcp( x, y, 
                                            &kTFlow, &lsTextCell ) );

                        if (hr)
                        {
                            AssertSz(0,"QueryLinePointPcp failed.");
                            goto Cleanup;
                        }

                        dx = lsTextCell.pointUvStartCell.u - x;
                        fReverseFlow = (IsRTL() == !(kTFlow & fUDirection));
                        // FUTURE: (mikejoch) Do we still need to test for a ruby here?
                        Assert(me._pLS->_lineFlags.GetLineFlags(cp) & FLAG_HAS_RUBY ||
                               ((!fReverseFlow) ? -dx : dx) >= 0);
                        cp = me._pLS->CPFromLSCP(lsTextCell.cpStartCell);

#if DBG==1
                        // (paulnel) This is a helper for hit testing to see if we 
                        // have the correct character
                        CTxtPtr tp(me._pdp->GetMarkup(), cp);
                        TCHAR ch = tp.GetChar();
                        Assert(!g_Zero.ab[0] || ch);
#endif // DBG
                        if (!fExactFit)
                        {
                            // It is possible to have lsTextCell flowing against
                            // the line. If this is the case dx is positive and
                            // we will need to subtract dupCell from dx if we
                            // advance the cp.
                            if ( ((!fReverseFlow) ? -dx : dx) >= lsTextCell.dupCell / 2 )
                            {
                                cp += lsTextCell.cCharsInCell;
                                dx += (!fReverseFlow) ? lsTextCell.dupCell : -lsTextCell.dupCell;
                            }
                        }
                    }
                    else
                    {
                        cp = cpStart + _cch;
                        // BUGBUG (mikejoch) Shouldn't this be (_xWidth + _xWhite) - x?
                        dx = _xWidth - x;
                    }
                }
                else
                {
                    // If _plsline is NULL, we didn't measure.  This usually
                    // means we had no text to measure, or we had no width.
                    
                    Assert(   IsTagEnabled(tagAssertOnHittestingWithLS)
                           || me._li._cch == 0 );
                    
                    dx = _xWidth - x;
                    cp = cpStart;
                }

                // The IE4 measure has a bug in that if you hit test on a
                // BR, you'll get the cch *after* the BR regardless of
                // where in the BR you're positioned.  Don't assert on
                // these false alarms (cthrash)

  #if DBG==1
                if (IsTagEnabled(tagAssertOnHittestingWithLS))
                {
                    WHEN_DBG( if (!me._fLastWasBreak) )
                    {
                        Assert(cp - cpStart == oldcch);
                        Assert(dx           == olddx);
                    }
                }
  #endif // DBG
            }
        }
        me._li._cch = cp - cpStart;
    }
    else
    {
        CTreePos *  ptp;

        me._pdp->FormattingNodeForLine(me.GetCp(), me.GetPtp(), _cch, &me._cchPreChars, &ptp, NULL);

        me._li._cch = me._cchPreChars;
        dx = -x;
    }

    if (pdx)
    {
        *pdx = (!fReverseFlow ? dx : -dx);
    }

Cleanup:
    if (cpStart != long(me.GetCp()))
        me.SetCp(cpStart, NULL);
    me.Advance(me._li._cch);

    if(me.CurrBranch()->GetCharFormat()->_fIsRubyText && pyHeightRubyBase)
    {
        RubyInfo *pRubyInfo = me._pLS->GetRubyInfoFromCp(me.GetCp());
        if(pRubyInfo) 
        {
            *pyHeightRubyBase = pRubyInfo->yHeightRubyBase;
        }
    }

    me._pLS->DiscardLine();

    return me._li._cch;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLine::TryToKeepTogether
//
//  Synopsis:   Does this line contain any elements that we should try to not
//              split during print pagination?
//
//  Arguments:  none
//
//  Returns :   boolean
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CLine::TryToKeepTogether()
{
    return !IsFrame() && !_fHasNestedRunOwner && _fHasEmbedOrWbr;
}


// =====================  CLinePtr: Line Run Pointer  ==========================

void CLinePtr::Init ( CLineArray & line_arr )
{
    _prgRun = (CRunArray *) & line_arr;
    SetIRun( 0 );
    SetIch( 0 );
}

//
// NOTE (sujalp): Presently this method is only called from the
// constructor. However, we may need it when we need to point the
// rp's to different displays. This functionality may be needed
// when we implement cursor navigation in edit mode between sites.
//
void
CLinePtr::Hijack (CDisplay *pdp)
{
    _pdp = pdp;
    _pdp->InitLinePtr( * this );
}

// Move runptr by a certain number of cch/runs

BOOL
CLinePtr::RpAdvanceCp ( long cch, BOOL fSkipFrame )
{
    // See if this is a multi-line ptr

    Assert( _prgRun );

    if (cch == CRunPtr<CLine>::AdvanceCp(cch))
    {
        if (fSkipFrame)
        {
            CLine *pLine=GetCurrRun();

            // There might not be a valid run pointer.
            if (pLine && pLine->IsFrame())
            {
                int iStep;
                long iRunEnd;

                Assert( NumRuns() );

                if (cch < 0)
                {
                    iStep = -1;
                    iRunEnd = 0;
                }
                else
                {
                    iStep = 1;
                    iRunEnd = NumRuns() - 1;
                }

                while (GetCurrRun()->IsFrame())
                {
                    if (GetIRun() == iRunEnd)
                    {
                        // If the last line is a frame line, then put the
                        // line ptr at the end of the previous line. Assert
                        // that we have a previous line: it will contain the
                        // embedding character for the site in the frame line.
                        Assert(iRunEnd > 0);
                        SetIRun(iRunEnd - 1);
                        if ((pLine = GetCurrRun()) != NULL)
                        {
                            SetIch(pLine->_cch);
                        }

#if DBG==1
                        {
                            CTxtPtr tp(_pdp->GetMarkup(), GetCp() - 1);
                            TCHAR ch = tp.GetChar();

                            Assert( '\r' != ch );
                        }
#endif
                        return TRUE;
                    }
                    else
                    {
                        SetIRun( GetIRun() + iStep );
                    }
                }
            }
        }
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

BOOL
CLinePtr::PrevLine ( BOOL fSkipFrame, BOOL fSkipEmptyLines )
{
    long iRun = GetIRun();

    if (PrevRun())
    {
        while ((fSkipFrame && GetCurrRun()->IsFrame()) ||
               (fSkipEmptyLines && 0 == GetCurrRun()->_cch))
        {
            if (!PrevRun())
            {
                SetIRun( iRun );
                SetIch( 0 );

                return FALSE;
            }
        }

        return TRUE;
    }

    return FALSE;
}

BOOL
CLinePtr::NextLine( BOOL fSkipFrame, BOOL fSkipDummyLines )
{
    long iRun = GetIRun();

    if (NextRun())
    {
        while ((fSkipFrame && GetCurrRun()->IsFrame()) ||
                (fSkipDummyLines && GetCurrRun()->IsClear()))
        {
            if (!NextRun())
            {
                SetIRun( iRun );
                SetIch( GetCurrRun()->_cch );

                return FALSE;
            }
        }

        return TRUE;
    }

    return FALSE;
}

CLine &
CLinePtr::operator [] ( long dRun )
{
    if (_prgRun)
        return * CRunPtr<CLine>::GetRunRel( dRun );

    AssertSz( dRun + GetIRun() == 0 ,
        "LP::[]: inconsistent line ptr");

    return  * (CLine *) CRunPtr<CLine>::GetRunAbs( GetIRun() );
}

/*
 *  CLinePtr::RpSetCp(cp, fAtEnd)
 *
 *  Purpose
 *      Set this line ptr to cp allowing for ambigous cp and taking advantage
 *      of _cpFirstVisible and _iliFirstVisible
 *
 *  Arguments:
 *      cp      position to set this line ptr to
 *      fAtEnd  if ambiguous cp:
 *              if fAtEnd = TRUE, set this line ptr to end of prev line;
 *              else set to start of line (same cp, hence ambiguous)
 *  Return:
 *      TRUE iff able to set to cp
 */
BOOL CLinePtr::RpSetCp(LONG cp, BOOL fAtEnd, BOOL fSkipFrame)
{
    Assert(_prgRun);

    BOOL fRet;
    // Adjust the cp to be relative to the txt site containting the disp
    cp -= _pdp->GetFlowLayout()->GetContentFirstCp();
    {
        SetIRun( 0 );
        SetIch( 0 );
        fRet = RpAdvanceCp(cp, fSkipFrame);  // Start from 0
    }

    //
    // Note(SujalP): As exposed in bug 50281, it can happen that the containing
    // txtsite contains no lines. In which case we cannot really position
    // ourselves anywhere. Return failure.
    //
    if (!CurLine())
    {
        fRet = FALSE;
    }

    //
    // Note(SujalP): Problem exposed in bug34660 -- when we are positioned at a
    // frame line, then the fAtEnd flag is really meaning less, since we base
    // our decision to move to prev or next line based on where in the line
    // is the cp positioned. Since, frame lines contain to characters, we really
    // cannot make that decision at all.
    //
    else if (!CurLine()->IsFrame())
    {
        // Ambiguous-cp caret position, should we be at the end of the line?
        if(fAtEnd)
        {
            if (!GetIch() && PrevLine( fSkipFrame, FALSE ))
            {
                SetIch( GetCurrRun()->_cch );  //  the first, go to end of
            }
        }

        // Or the beginning of the next one?
        else
        {
            if (GetIch() == long( GetCurrRun()->_cch ) && NextLine( fSkipFrame, FALSE ))
            {
                SetIch( 0 );   // Beginning of next line.
            }
        }
    }

    return fRet;
}


/*
 *  CLinePtr::RpBeginLine(void)
 *
 *  Purpose
 *      Move the current character to the beginning of the line.
 *
 *  Return:
 *      change in cp
 *
 *  Note that this will only work on the array type of line pointer.
 *  You will get an assert otherwise.
 */

long
CLinePtr::RpBeginLine ( )
{
    long cch = GetIch();

    Assert( _prgRun );

    SetIch( 0 );

    return cch - GetIch();
}

/*
 *  CLinePtr::RpEndLine(void)
 *
 *  Purpose
 *      Move the current character to the end of the line.
 *
 *  Return:
 *      change in cp
 *
 *  Note that this will only work on the array type of line pointer.
 *  You will get an assert otherwise.
 */
LONG CLinePtr::RpEndLine(void)
{
    LONG cch = GetIch();

    Assert( _prgRun );

    SetIch( GetCurrRun()->_cch );

    return GetIch() - cch;
}

/*
 *  CLinePtr::FindParagraph(fForward)
 *
 *  Purpose
 *      Move this line ptr to paragraph (fForward) ? end : start, and return
 *      change in cp
 *
 *  Arguments:
 *      fForward    TRUE move this line ptr to para end; else to para start
 *
 *  Return:
 *      change in cp
 */

long
CLinePtr::FindParagraph ( BOOL fForward )
{
    LONG cch;

    if(!fForward)                           // Go to para start
    {
        cch = 0;                            // Default already at para start
        if(RpGetIch() != (LONG)(*this)->_cch ||
           !((*this)->_fHasEOP)) // It isn't at para start
        {
            cch = -RpGetIch();              // Go to start of current line
            while(!((*this)->_fFirstInPara) && (*this) > 0)
            {
                if (!PrevLine(TRUE, FALSE)) // Go to start of prev line
                    break;
                cch -= (*this)->_cch;       // Subtract # chars in line
            }
            SetIch( 0 );                       // Leave *this at para start
        }
    }
    else                                    // Go to para end
    {
        cch = (*this)->_cch - RpGetIch();   // Go to end of current line

        while(((*this) < _pdp->LineCount() - 1 ||
                _pdp->WaitForRecalcIli((LONG)*this + 1))
              && !((*this)->_fHasEOP))
        {
            if (!NextLine(TRUE, FALSE))      // Go to start of next line
                break;
            cch += (*this)->_cch;           // Add # chars in line
        }
        SetIch( (*this)->_cch );               // Leave *this at para end
    }
    return cch;
}

/*
 *  CLinePtr::GetAdjustedLineLength
 *
 *  @mfunc  returns the length of the line _without_ EOP markers
 *
 *  @rdesc  LONG; the length of the line
 */
LONG CLinePtr::GetAdjustedLineLength()
{
    CLine * pline = GetCurrRun();
    Assert(pline);
    
    LONG cchJunk;
    LONG cchTrim;
    CTreePos *ptpRet, *ptpPrev;
    
    LONG cpEndLine = _pdp->GetFirstCp() + GetCp() - GetIch() + pline->_cch;
    CTreePos *ptp = _pdp->GetMarkup()->TreePosAtCp(cpEndLine, &cchJunk);
    
    _pdp->EndNodeForLine(cpEndLine, ptp, &cchTrim, &ptpRet, NULL);
    cchTrim = min(cchTrim, pline->_cch);

    LONG cpNewMost = cpEndLine - cchTrim;
    
    if (ptpRet)
    {
        ptpPrev = ptpRet;

        
        if (ptpPrev->GetCch() == 0 || ptpPrev->IsNode() || (ptpPrev->IsText() && ptpPrev->GetCp() >= cpNewMost))
        {
            do
            {
                ptpPrev = ptpPrev->PreviousTreePos();
            }
            while (ptpPrev->GetCch() == 0);
        }
        
        if (   ptpPrev->IsEndElementScope()
            && ptpPrev->Branch()->Tag() == ETAG_BR
           )
        {
            cchTrim += 2;
        }
        else if (ptpPrev->IsText())
        {
            CTxtPtr tp(_pdp->GetMarkup(), cpNewMost - 1);
            if (tp.GetChar() == WCH_ENDPARA1)
                cchTrim++;
        }
    }
    
    cchTrim = min(cchTrim, pline->_cch);
    return pline->_cch - cchTrim;
}


BOOL
CLinePtr::IsLastTextLine()
{
    return _pdp->IsLastTextLine(*this);
}
