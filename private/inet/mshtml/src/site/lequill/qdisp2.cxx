//+----------------------------------------------------------------------------
// File: qdisp2.cxx
//
// Description: Utility function on CDisplay
//
// This is the version customized for hosting Quill.
//
//-----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_LSM_HXX
#define X_LSM_HXX
#include "lsm.hxx"
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_MARQUEE_HXX_
#define X_MARQUEE_HXX_
#include "marquee.hxx"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

#ifndef X_RCLCLPTR_HXX_
#define X_RCLCLPTR_HXX_
#include "rclclptr.hxx"
#endif

#ifndef X_QUILGLUE_HXX_
#define X_QUILGLUE_HXX_
#include "quilglue.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_DISPRGN_HXX_
#define X_DISPRGN_HXX_
#include "disprgn.hxx"
#endif


#pragma warning(disable:4706) /* assignment within conditional expression */

MtDefine(CDisplayShowSelectedRange_aryInvalRects_pv, Locals, "CDisplay::ShowSelectedRange aryInvalRects::_pv")

DeclareTag(tagNotifyLines, "NotifyLines", "Fix-up a line cch from Notify")

const CCharFormat *
CDisplay::CFFromLine(CLine * pli, LONG cp)
{
#ifdef MERGEFUN // rtp
    CFlowLayout *   pFlowLayout = GetFlowLayout();
    CElement    *   pElementFL  = pFlowLayout->Element();
    const CCharFormat * pCF;

    Assert (pli);

    // get the char format for the start of line
    if(pli->IsFrame())
    {
        // frame line, get it from the site
        pCF = pli->_pNodeLayout->GetCharFormat();
    }
    else
    {
        CTreeNode * pRunOwner;

        rtp.SetCp(cp);
        rtp.AdvanceToNonEmpty();

        pRunOwner = rtp.GetRunOwnerBranch(pFlowLayout);

        if(pRunOwner->Element() != pElementFL)
            pCF = pRunOwner->GetCharFormat();
        else
            pCF = rtp.CurrBranch()->GetCharFormat();
    }

    return pCF;
#else
    return NULL;
#endif
}

// ================================  Line info retrieval  ====================================

/*
 *  CDisplay::YposFromLine(ili)
 *
 *  @mfunc
 *      Computes top of line position
 *
 *  @rdesc
 *      top position of given line (relative to the first line)
 */
LONG CDisplay::YposFromLine(
    LONG        ili,       //@parm Line we're interested in
    CCalcInfo * pci)
{
    LONG yPos;
    CLine * pli;

    // if the flowlayout is hidden, all we have is zero height lines.
    if(GetFlowLayout()->IsDisplayNone())
        return 0;


    if(!WaitForRecalcIli(ili, pci))          // out of range, use last valid line
    {
        ili = LineCount() -1;
        ili = (ili > 0) ? ili : 0;
    }

#ifndef DISPLAY_TREE
    yPos = GetYScroll() + GetFirstVisibleDY();
    cli = ili - GetFirstVisibleLine();

    for(ili = GetFirstVisibleLine(); cli > 0; cli--, ili++)
    {
        pli = Elem(ili);
        if (pli->_fForceNewLine)
        {
            yPos += pli->_yHeight;
        }
    }

    for(ili = GetFirstVisibleLine() - 1; cli < 0; cli++, ili--)
    {
        pli = Elem(ili);
        if (pli->_fForceNewLine)
        {
            yPos -= pli->_yHeight;
        }
    }
#else
    yPos = 0;
    for (long i=0; i < ili; i++)
    {
        pli = Elem(i);
        if (pli->_fForceNewLine)
        {
            yPos += pli->_yHeight;
        }
    }
#endif

    return yPos;
}


/*
 *  CDisplay::CpFromLine(ili, pyHeight)
 *
 *  @mfunc
 *      Computes cp at start of given line
 *      (and top of line position relative to this display)
 *
 *  @rdesc
 *      cp of given line
 */
LONG CDisplay::CpFromLine (
    LONG ili,       //@parm Line we're interested in (if <lt> 0 means caret line)
    LONG *pyHeight) //@parm Returns top of line relative to display
{
#ifndef DISPLAY_TREE
    LONG    cli;
    CLine * pli;
    LONG    y      = GetYScroll() + GetFirstVisibleDY();
    LONG    cp     = GetFirstVisibleCp();
    LONG    iStart = GetFirstVisibleLine();

    cli = ili - GetFirstVisibleLine();
    if(cli < 0 && -cli >= ili)
    {
        // Closer to first line than to first visible line,
        // so start at the first line
        cli    = ili;
        y      = 0;
        cp     = GetFirstCp();
        iStart = 0;
    }
    else if( cli <= 0 )
    {
        CheckView();

        for(ili = GetFirstVisibleLine()-1; cli < 0; cli++, ili--)
        {
            pli = Elem(ili);
            if(pli->_fForceNewLine)
                 y -= pli->_yHeight;
            cp -= pli->_cch;
        }

        goto end;
    }

    for(ili = iStart; cli > 0; cli--, ili++)
    {
        pli = Elem(ili);
        if(_fPrinting || !WaitForRecalcIli(ili))
            break;
        if(pli->_fForceNewLine)
            y += pli->_yHeight;
        cp += pli->_cch;
    }

end:
#else
    long    cp = GetFirstCp();
    long    y  = 0;
    CLine * pli;

    for (long i=0; i < ili; i++)
    {
        pli = Elem(i);
        if (pli->_fForceNewLine)
        {
            y += pli->_yHeight;
        }
        cp += pli->_cch;
    }
#endif

    if(pyHeight)
        *pyHeight = y;

    return cp;
}

//+----------------------------------------------------------------------------
//
//  Member:     Notify
//
//  Synopsis:   Adjust all internal caches in response to a text change
//              within a nested CTxtSite
//
//  Arguments:  pnf - Notification that describes the change
//
//  NOTE: Only those caches which the notification manager is unaware of
//        need updating (e.g., _dcpCalcMax). Others, such as outstanding
//        change transactions, are handled by the notification manager.
//
//-----------------------------------------------------------------------------

inline BOOL LineIsNotInteresting(CLinePtr& rp)
{
    return rp->IsFrame() || rp->IsClear();
}

void
CDisplay::Notify(CNotification * pnf)
{
    CFlowLayout *   pFlowLayout         = GetFlowLayout();
    CElement *      pElementFL          = pFlowLayout->ElementOwner();
    long            cpFirst             = pFlowLayout->GetContentFirstCp();
    long            cpMax               = _dcpCalcMax;
    long            cchDelta            = pnf->CchChanged();
#if DBG==1
    long            dcpLastCalcMax      = _dcpCalcMax;
#ifndef DISPLAY_TREE
    long            dcpLastFirstVisible = GetFirstVisibleDCp();
#endif
    long            iLine               = -1;
    long            cchLine             = -1;
#endif

#ifndef DISPLAY_TREE
    Assert(GetFirstVisibleDCp() <= _dcpCalcMax);
#endif

    //
    // If no lines yet exist, exit
    //

    if (!LineCount())
        goto Cleanup;

    //
    // Determine the end of the line array
    // (This is normally the maximum calculated cp, but that cp must
    //  be adjusted to take into account outstanding changes.
    //  Changes which overlap the maximum calculated cp effectively
    //  move it to the first cp affected by the change. Changes which
    //  come entirely before move it by the delta in characters of the
    //  change.)
    //

    if (pFlowLayout->IsDirty() && pFlowLayout->Cp() < cpMax)
    {
        if (pFlowLayout->Cp() + pFlowLayout->CchOld() >= cpMax)
        {
            cpMax = pFlowLayout->Cp();
        }
        else
        {
            cpMax += pFlowLayout->CchNew() - pFlowLayout->CchOld();
        }
    }

    //
    // If the change is past the end of the line array, exit
    //

    if ((pnf->Cp() - cpFirst) > cpMax)
        goto Cleanup;

    //
    // If the change is entirely before or after any pending changes,
    // update the cch of the affected line, the maximum calculated cp,
    // and, if necessary, the first visible cp
    // (Changes which occur within the range of a pending change,
    //  need not be tracked since the affected lines will be completely
    //  recalculated. Additionally, the outstanding change which
    //  represents those changes will itself be updated (by other means)
    //  to cover the changes within the range.)
    // NOTE: The "old" cp must be used to search the line array
    //

    if (cchDelta)
    {
        LONG cpFirstDirty = pFlowLayout->GetContentFirstCp() + pFlowLayout->Cp();
        
        if (    !pFlowLayout->IsDirty()
            ||  pnf->Cp() <  cpFirstDirty
            ||  pnf->Cp() >= cpFirstDirty + pFlowLayout->CchNew()
           )
        {
    #if DBG == 1
            long        cchText = pElementFL->GetElementCch() -
                                        (pFlowLayout->IsDirty()
                                            ? pFlowLayout->CchNew() - pFlowLayout->CchOld()
                                            : 0);
    #endif
            CLinePtr    rp(this);
            long        cp = (!pFlowLayout->IsDirty() || pnf->Cp() < (pFlowLayout->Cp() + cpFirst)
                                    ? pnf->Cp()
                                    : pnf->Cp() + (pFlowLayout->CchOld() - pFlowLayout->CchNew()));

            //
            // Adjust the maximum calculated cp
            //

            _dcpCalcMax += cchDelta;
            Assert(_dcpCalcMax >= 0);
            Assert(_dcpCalcMax <= cchText);

            //----------------------------------------------------------------------------------
            //
            // BEGIN HACK ALERT! BEGIN HACK ALERT! BEGIN HACK ALERT! BEGIN HACK ALERT! 
            //
            // All the code here to find out the correct line to add or remove chars from.
            // Its impossible to accurately detect a line to which characters can be added.
            // So this code makes a best attempt!
            //
            //

            //
            // Find and adjust the affected line
            //
            rp.RpSetCp(cp, FALSE, FALSE);

            if (cchDelta > 0)
            {
                //
                // We adding at the end of the site?
                //
                if (pnf->Handler()->GetLastCp() == pnf->Cp() + cchDelta)
                {
                    //
                    // We are adding to the end of the site, we need to go back to a line which
                    // has characters so that we can tack on these characters to that line.
                    // Note that we have to go back _atleast_ once so that its the prev line
                    // onto which we tack on these characters.
                    //
                    // Note: This also handles the case when the site was empty (no chars) and
                    // we are adding characters to it. In this case we will add to the first
                    // line prior to the site, because there is no line in the line array where
                    // we can add these chars (since it was empty in the first place). If we
                    // cannot find a line with characters before this line -- could be we are
                    // adding chars to an empty table at beg. of doc., then we will find the
                    // line _after_ cp to add it. If we cannot find that either then we will
                    // bail out.
                    //

                    //
                    // So go back once only if we were ambigously positioned (We will be correctly
                    // positioned if this is last line and last cp of handler is the last cp of this
                    // flowlayout). Dont do anything if you cannot go back -- in that
                    // case we will go forward later on.
                    //
                    if (rp.RpGetIch() == 0)
                        rp.PrevLine(FALSE, FALSE);

                    //
                    // OK, so look backwards to find a line with characters.
                    //
                    while (LineIsNotInteresting(rp))
                    {
                        if (!rp.PrevLine(FALSE, FALSE))
                            break;
                    }

                    //
                    // If we broke out of the while look above, it would mean that we did not
                    // find an interesting line before the one at which we were positioned.
                    // This should only happen when we have an empty site (like table) at the
                    // beginning of the document. In this case we will go forward (the code
                    // outside this if block) to find a line. YUCK! This is not ideal but is
                    // the best we can do in this situation.
                    //
                }

                //
                // We will fall into the following while loop in 3 cases:
                // 1) When we were positioned at the beginning of the site: In this case
                //    we have to make sure that we add the chars to an interesting line.
                //    (note the 3rd possibility, i.e. adding in the middle of a site is trivial
                //    since the original RpSetCp will position us to an interesting line).
                // 2) We were positioned at the end of the site but were unable to find a prev
                //    line, hence we are now looking forward.
                //
                // If we cannot find _any_ interesting line then we shrug our shoulders
                // and bail out.
                //
                while (LineIsNotInteresting(rp))
                {
                    if (!rp.NextLine(FALSE, FALSE))
                        goto Cleanup;
                }


                // BUGBUG: Arye - It shouldn't be necessary to do this here, it should be possible
                // to do it only in the case where we're adding to the end. Since this code
                // is likely to go away with the display tree I'm not going to spend a lot of
                // time making that work right now.
                // Before this, however, in edit mode we might end up on the last (BLANK) line.
                // This is bad, nothing is there to change the number of characters,
                // so just back up.
                if (rp->_cch == 0 && pElementFL->IsEditable() &&
                    rp.GetLineIndex() == LineCount() - 1)
                {
                    do  
                    {
                        if (!rp.PrevLine(FALSE, FALSE))
                            goto Cleanup;
                    } while(LineIsNotInteresting(rp));
                }

                //
                // Right, if we are here then we have a line into which we can pour
                // the characters.
                //
            }

            //
            // We are removing chars. Easy problem; just find a line from which we can remove
            // the chars!
            //
            else
            {
                while (!rp->_cch)
                {
                    if (!rp.NextLine(FALSE, FALSE))
                    {
                        Assert("No Line to goto, doing nothing");
                        goto Cleanup;
                    }
                }
            }

            //
            //
            // END HACK ALERT! END HACK ALERT! END HACK ALERT! END HACK ALERT! END HACK ALERT! 
            //
            //----------------------------------------------------------------------------------

            Assert(rp.GetLineIndex() >= 0);
            Assert(cchDelta > 0 || ( rp->_cch + cchDelta ) >= 0);
            WHEN_DBG(iLine = rp.GetLineIndex());
            WHEN_DBG(cchLine = rp->_cch);

            rp->_cch += cchDelta;
        }
    }


Cleanup:
#if DBG==1
    if (iLine >= 0)
    {
#ifndef DISPLAY_TREE
        TraceTagEx((tagNotifyLines, TAG_NONAME,
                    "NotifyLine: (%d) Element(0x%x,%S) ChldNd(%d) dcp(%d) cchDelta(%d) line(%d) cch(%d,%d) dcpCalcMax(%d,%d) dcpFirstVisible(%d,%d)",
                    pnf->SerialNumber(),
                    pFlowLayout->_pElementLayout,
                    pFlowLayout->_pElementLayout->TagName(),
                    pnf->Node()->_nSerialNumber,
                    pnf->Cp() - cpFirst,
                    cchDelta,
                    iLine,
                    cchLine, cchLine + cchDelta,
                    dcpLastCalcMax, _dcpCalcMax,
                    dcpLastFirstVisible, GetFirstVisibleDCp()));
#else
        TraceTagEx((tagNotifyLines, TAG_NONAME,
                    "NotifyLine: (%d) Element(0x%x,%S) ChldNd(%d) dcp(%d) cchDelta(%d) line(%d) cch(%d,%d) dcpCalcMax(%d,%d)",
                    pnf->SerialNumber(),
                    pFlowLayout->ElementOwner(),
                    pFlowLayout->ElementOwner()->TagName(),
                    pnf->Node()->_nSerialNumber,
                    pnf->Cp() - cpFirst,
                    cchDelta,
                    iLine,
                    cchLine, cchLine + cchDelta,
                    dcpLastCalcMax, _dcpCalcMax));
#endif
    }
    else
    {
#ifndef DISPLAY_TREE
        TraceTagEx((tagNotifyLines, TAG_NONAME,
                    "NotifyLine: (%d) Element(0x%x,%S) dcp(%d) dcpCalcMax(%d) dcpFirstVisible(%d) delta(%d) IGNORED",
                    pnf->SerialNumber(),
                    pFlowLayout->_pElementLayout,
                    pFlowLayout->_pElementLayout->TagName(),
                    pnf->Cp() - cpFirst,
                    _dcpCalcMax,
                    GetFirstVisibleDCp(),
                    cchDelta));
#else
        TraceTagEx((tagNotifyLines, TAG_NONAME,
                    "NotifyLine: (%d) Element(0x%x,%S) dcp(%d) dcpCalcMax(%d) delta(%d) IGNORED",
                    pnf->SerialNumber(),
                    pFlowLayout->ElementOwner(),
                    pFlowLayout->ElementOwner()->TagName(),
                    pnf->Cp() - cpFirst,
                    _dcpCalcMax,
                    cchDelta));
#endif
    }
#endif
    return;
}


#ifndef DISPLAY_TREE
//
// Function: FindVisualStartLine
//
// Description: If the current line has margins or is an aligned line
//              find a text line before the current line that does not
//              have margins. The aligned line that is adds the margin
//              to the current line could be the one we are looking for.
//
void
CDisplay::FindVisualStartLine(LONG &ili, CLine *&pli, LONG &cpLi, LONG &yLi)
{
    while(   ili > 0 &&
            (Elem(ili)->HasMargins() ||
             Elem(ili - 1)->HasMargins() || // if the current line
             Elem(ili - 1)->IsFrame() ||    // large before space
             Elem(ili)->IsFrame()))
    {
        pli = Elem(--ili);
        cpLi -= pli->_cch;
        if(pli->_fForceNewLine)
            yLi -= pli->_yHeight;
    }
}
#endif


#ifndef DISPLAY_TREE
//+----------------------------------------------------------------------------
//
//  Member:     LineFromPos
//
//  Synopsis:   Computes the line at a given x/y and returns the appropriate
//              information
//
//  Arguments:  xPos -
//              yPos -
//              fHitTesting -
//              fSkipFrame  -
//              pyLine      - Y-offset of line (may be NULL)
//              pcpFirst    - cp at start of line (may be NULL)
//              xExposedWidth   - Width of exposed region
//              isfIgnoreFlag   -
//              pci             -
//              fIgnoreRelLines -
//
//  Returns:    Index of found line
//
//-----------------------------------------------------------------------------
LONG CDisplay::LineFromPos (
    LONG                xPos,
    LONG                yPos,
    BOOL                fHitTesting,
    BOOL                fSkipFrame,
    LONG *              pyLine,
    LONG *              pcpFirst,
    LONG                xExposedWidth,
    IGNORESTATUSFLAG    isfIgnoreFlag,
    CCalcInfo *         pci,
    BOOL                fIgnoreRelLines)
{
    CFlowLayout *   pFlowLayout = GetFlowLayout();
    CElement    *   pElementFL  = pFlowLayout->ElementOwner();
    LONG            cpLi;
    LONG            dy;
    LONG            ili;
    LONG            yLi;
    CLine *         pli    = NULL;
    LONG            iliPos = -1;
    LONG            iliCandidateLine = -1;
    LONG            cpLiCandidate    = -1;
    LONG            yLiCandidate     = -1;
    LONG            yNeg        = -1;
    BOOL            fLookForNeg = FALSE;
    BOOL            fBrowseMode = !pElementFL->IsEditable();
    CCalcInfo       CI;

    if(!pci)
    {
        CI.Init(pFlowLayout);
        pci = &CI;
    }

    // for hit testing we should be looking for a point
    // not a line the lies in a given region.
    Assert(!fHitTesting || xExposedWidth == 0);

    // if the flowlayout is hidden, all we have is zero height lines or if
    // WaitForRecalc fails there is nothing much to do.
    if ((pElementFL->IsDisplayNone() && pElementFL->Tag() != ETAG_BODY) ||
        !WaitForRecalc(-1, max(yPos, GetYScroll() + GetViewHeight()), pci) ||
        LineCount() == 0)
    {
        yLi  = 0;
        cpLi = GetFirstCp();
        ili  = -1;
        goto done;
    }

    cpLi = GetFirstVisibleCp();
    ili  = GetFirstVisibleLine();
    yLi  = GetYScroll() + GetFirstVisibleDY();
    dy   = yPos - yLi - max(_yMostPos, -_yMostNeg) - 1;

    // if our point is before the first visible text line
    // lets go back and find the line, after which we may
    // find the point
    if(dy < 0 && ili)
    {
        // we are nearer to the first line
        if(dy < (GetYScroll() + GetFirstVisibleDY()) / 2)
        {
            while (dy < 0 && ili > 0)
            {
                pli     =  Elem(--ili);
                cpLi    -= pli->_cch;

                if(pli->_fForceNewLine)
                {
                    yLi -= pli->_yHeight;
                    dy += pli->_yHeight;
                }
            }
        }
        else
        {
            // we are closer to the top so start looking
            ili = 0;
            yLi = 0;
            cpLi = GetFirstCp();
        }
    }

    // if the current line has margins or is an aligned line
    // find a text line before the current line that does not
    // have margins. The aligned line that is adds the margin
    // to the current line could be the one we are looking for.
    if (!fSkipFrame)
        FindVisualStartLine(ili, pli, cpLi, yLi);

    while ( ili < LineCount() )
    {
        pli = Elem(ili);

        if (fLookForNeg && pli->_fForceNewLine)
        {
            // If we are looking for negative line heights,
            // be sure that we do not look forever.
            yNeg -= pli->_yHeight;
            if (yNeg <= 0)
                break;
        }

        if (pli->IsFrame())
        {
            if (!fSkipFrame)
            {
                // It's a left aligned image and the line is in the exposed
                // region or it's a right aligned image and if the line or
                // the area to the right of it is in the exposed region
                // break
                if (yPos < yLi + pli->GetYLineBottom() && 
                    (!_fRTL
                        ?   xPos + xExposedWidth >=  pli->_xLeftMargin
                        &&  xPos < pli->_xLeftMargin + pli->_xLineWidth
                        :   xPos + xExposedWidth >= pli->_xRightMargin
                        &&  xPos < pli->_xRightMargin + pli->_xLineWidth))
                {
                    iliPos = ili;
                    break;
                }
            }
        }
        else
        {
            // frames are always before 'normal' lines so we
            // don't have to check xPos here
            if (yPos < yLi + pli->GetYLineBottom())
            {
                while(  
                        // CONDN1: Jump accross hidden or dummy lines
                        (   pli->_fHidden
                        ||  (   pli->_fDummyLine
                            &&  fBrowseMode)

                        // CONDN2: Relative lines are hit tested separately before
                        // so just jump past them for now
                        ||  (   pli->_fRelative
                            &&  fIgnoreRelLines)

                        // CONDN3: Multiple chunks:
                        // Look for new lines to the right of this
                        // line (which contains the aligned site).
                        ||  (   !pli->_fForceNewLine
                            &&  (   !pli->_fRTL
                                        ? xPos > pli->GetTextRight()
                                        : xPos < pli->GetRTLTextLeft()))

                        // CONDN4:
                        // Consider the following case:
                        // 1) Line A: the line we hit in the if (yPos < yli ..)
                        //            condn above does not force a new line
                        //            and has a height of say H1. Condn2 will allow
                        //            the loop to go thru once and then ...
                        // 2) Line B: the line after A, which forces a new line
                        //            and its height H2 < H1. This means that
                        //            we will have to test lines B and after
                        //            (based on the yPos) to find the exact line
                        ||  (   pli->_fForceNewLine
                            &&  yPos > yLi + pli->GetYLineBottom()))

                    // Of course we should have more lines to test.
                    &&  (ili + 1) < LineCount())
                {
                    if(pli->_fForceNewLine)
                    {
                        yLi += pli->_yHeight;
                        if (fLookForNeg)
                            yNeg -= pli->_yHeight;
                    }
                    ili++;
                    cpLi += pli->_cch;
                    pli = Elem(ili);
                }

                // If we hit in the area to the left/right of the line
                // then we have have to still test for other lines which
                // can be after us in the line array but before us on the
                // screen. Of course, when we are not ignoring whitespaces
                // then the line hit will be the present one and we will
                // not search for subsequent lines with negative heights
                // (this may change later though ... (sujalp)
                if (
                       (!pli->_fRTL ?
                       // Left -- if we are over the bullet, then we have
                       // still hit the line.
                       xPos < pli->GetTextLeft() -
                                (pli->_fHasBulletOrNum ? pli->_xLeft : 0)
                       : xPos < pli->GetRTLTextLeft())

                       // Right
                    || (!pli->_fRTL ? xPos > pli->GetTextRight()
                                    : xPos > pli->GetRTLTextRight() - 
                                      (pli->_fHasBulletOrNum ? pli->_xRight : 0))

                       // Top
                    || (yPos >= yLi &&
                        yPos <  (yLi + pli->GetYTop()))

                       // Bottom
                    || (yPos >= yLi + pli->GetYBottom() &&
                        yPos <  yLi + pli->_yHeight)
                   )
                {
                    // OK So we hit the empty area around the line.
                    if (_yMostNeg)
                    {
                        if (!fLookForNeg)
                        {
                            fLookForNeg = TRUE;
                            yNeg = -_yMostNeg;
                        }

                        if (isfIgnoreFlag == ISF_DONTIGNORE_LOOKFURTHER)
                        {
                            // NOTE(sujalp):We are not ignoring the before and after
                            // spaces, but we do not select this line right away,
                            // because due to negative line heights, there might be
                            // other lines in *this* lines before/after space. We
                            // have to hit test for those. However, we remember this
                            // line as a potential candidate to return if we end
                            // up finding no lines in this line's before/after
                            // space.
                            iliCandidateLine = ili;
                            yLiCandidate = yLi;
                            cpLiCandidate = cpLi;
                        }
                        else
                        {
                            Assert(isfIgnoreFlag == ISF_DONTIGNORE_DONTLOOKFURTHER);
                            iliPos = ili;
                            break;
                        }
                    }
                    else
                    {
                        // Not ignoring, and no lines with negative
                        // line heights, so we have found out line
                        // and hence just break out!
                        iliPos = ili;
                        break;
                    }
                }
                else if (fLookForNeg &&
                         yPos >= yLi + pli->GetYTop() &&
                         yPos <  yLi + pli->GetYBottom()
                        )
                {
                    // Found the line with negative line height
                    iliPos = ili;
                    break;
                }
                else if (fLookForNeg &&
                         yPos < yLi)
                {
                    // If the position is above the current line and we
                    // are looking for negative lines, then too we need
                    // to look at further lines.
                    ;
                }
                else
                {
                    iliPos = ili;
                    break;
                }
            }
            else if (fLookForNeg &&
                     yPos >= yLi + pli->GetYTop() &&
                     yPos <  yLi + pli->GetYBottom()
                    )
            {
                // Found the line with negative line height
                iliPos = ili;
                break;
            }
            if(pli->_fForceNewLine)
                yLi += pli->_yHeight;
            cpLi += pli->_cch;
        }
        ili++;
    }

    if (ili == LineCount() || iliPos == -1)
    {
        ili--;
        if (pli->_fForceNewLine)
            yLi -= pli->_yHeight;
        cpLi -= pli->_cch;
    }

    // If we are not ignoring spaces, and we have fallen out without
    // finding a line but we had found a candidate line, then return
    // the candidate line.
    if (   iliPos == -1
        && iliCandidateLine != -1
       )
    {
        Assert(_yMostNeg);
        ili = iliCandidateLine;
        yLi = yLiCandidate;
        cpLi = cpLiCandidate;
    }

    pli = Elem(ili);
    if (fSkipFrame && ili && pli->IsFrame())
    {
        while(pli->IsFrame())
        {
            ili--;
            pli = Elem(ili);
        }
        if(pli->_fForceNewLine)
            yLi -= pli->_yHeight;
        cpLi -= pli->_cch;
    }

done:
    if(pyLine)
        *pyLine = yLi;
    if(pcpFirst)
        *pcpFirst = cpLi;

    return ili;
}
#else
//+----------------------------------------------------------------------------
//
//  Member:     LineFromPos
//
//  Synopsis:   Computes the line at a given x/y and returns the appropriate
//              information
//
//  Arguments:  prc      - CRect that describes the area in which to look for the line
//              pyLine   - Returned y-offset of the line (may be NULL)
//              pcpLine  - Returned cp at start of line (may be NULL)
//              grfFlags - LFP_xxxx flags
//
//  Returns:    Index of found line (-1 if no line is found)
//
//-----------------------------------------------------------------------------
LONG CDisplay::LineFromPos (
    const CRect &   rc,
    LONG *          pyLine,
    LONG *          pcpLine,
    DWORD           grfFlags) const
{
    CFlowLayout *   pFlowLayout = GetFlowLayout();
    CElement    *   pElement    = pFlowLayout->ElementOwner();
    CLine *         pli;
    long            ili;
    long            yli;
    long            cpli;
    long            iliCandidate;
    long            yliCandidate;
    long            cpliCandidate;
    BOOL            fCandidateWhiteHit;
    long            yIntersect;
    BOOL            fInBrowse = !pElement->IsEditable();

    CRect myRc( rc );

    if( myRc.top < 0 )
        myRc.top = 0;

    Assert(myRc.bottom >= 0);
    Assert(myRc.top    <= myRc.bottom);
    Assert(myRc.left   <= myRc.right);

    //
    //  If hidden or no lines exist, return immediately
    //

    if (    (   pElement->IsDisplayNone()
            &&  pElement->Tag() != ETAG_BODY)
        ||  LineCount() == 0)
    {
        ili  = -1;
        yli  = 0;
        cpli = GetFirstCp();
        goto FoundIt;
    }

    ili  = 0;
    yli  = 0;
    cpli = GetFirstCp();
    pli  = NULL;

    iliCandidate  = -1;
    yliCandidate  = -1;
    cpliCandidate = -1;
    fCandidateWhiteHit = TRUE;

    //
    //  Set up to intersect top or bottom of the passed rectangle
    //

    yIntersect = (grfFlags & LFP_INTERSECTBOTTOM
                        ? myRc.bottom - 1
                        : myRc.top);

    //
    //  Examine all lines that intersect the passed offsets
    //

    while ( ili < LineCount()
        &&  yIntersect > yli + _yMostNeg)
    {
        pli = Elem(ili);

        //
        //  Skip over lines that should be ignored
        //  These include:
        //      1. Lines that do not intersect
        //      2. Hidden and dummy lines
        //      3. Relative lines (when requested)
        //      4. Line chunks that do not intersect the x-offset
        //
        
        if (    yIntersect >= yli + pli->GetYLineTop()
            &&  yIntersect < yli + pli->GetYLineBottom()
            &&  !pli->_fHidden
            &&  (   !pli->_fDummyLine
                ||  !fInBrowse)
            &&  (   !pli->_fRelative
                ||  (grfFlags & LFP_IGNORERELATIVE))
            &&  (   pli->_fForceNewLine
                ||  (!pli->_fRTL
                            ? myRc.left <= pli->GetTextRight()
                            : myRc.left >= pli->GetRTLTextLeft())))

        {
            //
            //  If searching for the top-most line in z-order,
            //  then save the "hit" line and continue the search
            //  NOTE: Progressing up through the z-order, multiple lines can be hit.
            //        Hits on text always win over hits on whitespace.
            //

            if (grfFlags & LFP_ZORDERSEARCH)
            {
                BOOL    fWhiteHit =  (!pli->_fRTL
                                            ?   (   myRc.left < pli->GetTextLeft() - (pli->_fHasBulletOrNum
                                                                                        ? pli->_xLeft
                                                                                        : 0)
                                                ||  myRc.left > pli->GetTextRight())
                                            :   (   myRc.left < pli->GetRTLTextLeft()
                                                ||  myRc.left > pli->GetRTLTextRight() - (pli->_fHasBulletOrNum
                                                                                            ? pli->_xRight
                                                                                            : 0)))
                                ||  (   yIntersect >= yli
                                    &&  yIntersect <  (yli + pli->GetYTop()))
                                ||  (   yIntersect >= (yli + pli->GetYBottom())
                                    &&  yIntersect <  (yli + pli->_yHeight));

                if (    iliCandidate < 0
                    ||  !fWhiteHit
                    ||  fCandidateWhiteHit)
                {
                    iliCandidate       = ili;
                    yliCandidate       = yli;
                    cpliCandidate      = cpli;
                    fCandidateWhiteHit = fWhiteHit;
                }
            }

            //
            //  Otherwise, the line is found
            //

            else
            {
                goto FoundIt;
            }

        }

        if(pli->_fForceNewLine)
        {
            yli += pli->_yHeight;
        }

        cpli += pli->_cch;
        ili++;
    }

    Assert(ili <= LineCount());


    //
    // we better have a candidate, if yIntersect < yli + _yMostNeg
    //
    Assert( iliCandidate >= 0 || yIntersect >= yli + _yMostNeg);

    //
    //  No intersecting line was found, take either the candidate or last line
    //
    
    //
    //  ili == LineCount() - is TRUE only if the point we are looking for is
    //  below all the content or we found a candidate line but are performing
    //  a Z-Order search on a layout with lines with negative margin.
    //
    if (    ili == LineCount()

    //
    //  Here we don't really need to check if iliCandidate >= 0. It is added
    //  to make the code more robust to handle cases like a negative yIntersect
    //  passed in.
    //
        ||  (   yIntersect < yli + _yMostNeg
            &&  iliCandidate >= 0))
    {
        //
        //  If a candidate line was found, use it
        //

        if (iliCandidate >= 0)
        {
            Assert(yliCandidate  >= 0);
            Assert(cpliCandidate >= 0);

            ili  = iliCandidate;
            yli  = yliCandidate;
            cpli = cpliCandidate;
        }

        //
        //  Otherwise use the last line
        //

        else
        {
            Assert(pli);
            Assert(ili > 0);
            Assert(LineCount());

            ili--;

            if (pli->_fForceNewLine)
            {
                yli -= pli->_yHeight;
            }

            cpli -= pli->_cch;
        }

        //
        //  Ensure that frame lines are not returned if they are to be ignored
        //

        if (grfFlags & LFP_IGNOREALIGNED)
        {
            pli = Elem(ili);

            if (pli->IsFrame())
            {
                while(pli->IsFrame() && ili)
                {
                    ili--;
                    pli = Elem(ili);
                }

                if(pli->_fForceNewLine)
                    yli -= pli->_yHeight;
                cpli -= pli->_cch;
            }
        }
    }

FoundIt:
    Assert(ili < LineCount());

    if(pyLine)
    {
        *pyLine = yli;
    }

    if(pcpLine)
    {
        *pcpLine = cpli;
    }

    return ili;
}
#endif


/*
 *  CDisplay::LineFromCp(cp, fAtEnd)
 *
 *  @mfunc
 *      Computes line containing given cp.
 *
 *  @rdesc
 *      index of line found, -1 if no line at that cp.
 */
LONG CDisplay::LineFromCp(
    LONG cp,        //@parm cp to look for
    BOOL fAtEnd)    //@parm If true, return previous line for ambiguous cp
{
    CLinePtr rp(this);

    if (GetFlowLayout()->IsDisplayNone() ||
        !WaitForRecalc(cp, -1) ||
        !rp.RpSetCp(cp, fAtEnd))
    {
        return -1;
    }

    return (LONG)rp;
}


//==============================  Point <-> cp conversion  ==============================

LONG CDisplay::CpFromPointReally(
    POINT pt,                     // Point to compute cp at (client coords)
    CLinePtr * const prp,         // Returns line pointer at cp (may be NULL)
    CTreePos ** pptp,             // pointer to return TreePos corresponding to the cp
    BOOL fAllowEOL,               // Click at EOL returns cp after CRLF
    BOOL fIgnoreBeforeAfterSpace, // TRUE if ignoring pbefore/after space
    BOOL fExactFit,               // TRUE if cp should always round down (for element-hit-testing)
    BOOL * pfRightOfCp)
{
    CMessage msg;
    HTC htc;
    CTreeNode * pNodeElementTemp;
    DWORD dwFlags = HT_SKIPSITES | HT_VIRTUALHITTEST | HT_IGNORESCROLL;
    LONG cpHit;
    CTreeNode * pNode = GetFlowLayout()->GetFirstBranch();

    Assert(pNode);

    CElement  * pContainer = pNode->GetContainer();

    msg.pt = pt;
    if (fAllowEOL)
        dwFlags |= HT_ALLOWEOL;
    if (!fIgnoreBeforeAfterSpace)
        dwFlags |= HT_DONTIGNOREBEFOREAFTER;
    if (!fExactFit)
        dwFlags |= HT_NOEXACTFIT;

    //
    // Ideally I would have liked to perform the hit test on _pFlowLayout itself,
    // however when we have relatively positioned lines, then this will miss
    // some lines (bug48689). To avoid missing such lines we have to hittest
    // from the ped. However, doing that has other problems when we are
    // autoselecting. For example, if the point is on a table, the table finds
    // the closest table cell and passes that table cell the message to
    // extend a selection. However, this hittest hits the table and
    // msg._cpHit is not initialized (bug53706). So in this case, do the CpFromPoint
    // in the good ole' traditional manner.
    //

    // Note (yinxie): I changed GetPad()->HitTestPoint to GetContainer->HitTestPoint
    // for the flat world, container is the new notion to replace ped
    // this will ix the bug (IE5 17135), the bugs mentioned above are all checked
    // there is no regression.
    htc = pContainer->HitTestPoint(&msg, &pNodeElementTemp, dwFlags);

    // Take care of INPUT which contains its own private ped. If cpHit is inside the INPUT,
    // change it to be the cp of the INPUT in its contaning ped.
    //if (htc >= HTC_YES && pNodeElementTemp && pNodeElementTemp->Element()->IsMaster())
    //{
    //    msg.resultsHitTest.cpHit = pNodeElementTemp->Element()->GetFirstCp();
    //}
    if (    htc >= HTC_YES
        &&  pNodeElementTemp
        &&  (
                   pNodeElementTemp->IsContainer()
                && pNodeElementTemp->GetContainer() != pContainer
            )
       )
    {
        htc= HTC_NO;
    }

    if (htc >= HTC_YES && msg.resultsHitTest.cpHit >= 0)
    {
        cpHit = msg.resultsHitTest.cpHit;
        if (prp)
            prp->RpSet(msg.resultsHitTest.iliHit, msg.resultsHitTest.ichHit);
        if (pfRightOfCp)
            *pfRightOfCp = msg.resultsHitTest.fRightOfCp;
    }
    else
    {
        cpHit = CpFromPoint(pt, prp, pptp, NULL, fAllowEOL,
                            fIgnoreBeforeAfterSpace, fExactFit,
                            pfRightOfCp, NULL, NULL);
    }
    return cpHit;
}

LONG
CDisplay::CpFromPoint(
    POINT pt,                     // Point to compute cp at (site coords)
    CLinePtr * const prp,         // Returns line pointer at cp (may be NULL)
    CTreePos ** pptp,             // pointer to return TreePos corresponding to the cp
    CLayout ** ppLayout,          // can be NULL
    BOOL fAllowEOL,               // Click at EOL returns cp after CRLF
    BOOL fIgnoreBeforeAfterSpace, // TRUE if ignoring pbefore/after space
    BOOL fExactFit,               // TRUE if cp should always round down (for element-hit-testing)
    BOOL * pfRightOfCp,
    BOOL * pfPsuedoHit,
    CCalcInfo * pci)
{
    CCalcInfo   CI;
    CRect       rc;
    LONG        ili;
    LONG        cp;
    LONG        yLine;

    if(!pci)
    {
        CI.Init(GetFlowLayout());
        pci = &CI;
    }

    // Get line under hit
    GetFlowLayout()->GetClientRect(&rc);
    rc.MoveTo(pt);

    ili = LineFromPos(rc, &yLine, &cp, LFP_ZORDERSEARCH | LFP_IGNORERELATIVE |
                                        (!ppLayout
                                            ?  LFP_IGNOREALIGNED
                                            : 0));
    if(ili < 0)
        return -1;
        
    return CpFromPointEx(ili, yLine, cp, pt, prp, pptp, ppLayout, fAllowEOL,
                         fIgnoreBeforeAfterSpace, fExactFit, pfRightOfCp,
                         pfPsuedoHit, pci);
                        
}

LONG
CDisplay::CpFromPointEx(
    LONG       ili,
    LONG       yLine,
    LONG       cp,
    POINT      pt,                      // Point to compute cp at (site coords)
    CLinePtr  *const prp,               // Returns line pointer at cp (may be NULL)
    CTreePos **pptp,                    // pointer to return TreePos corresponding to the cp
    CLayout  **ppLayout,                // can be NULL
    BOOL       fAllowEOL,               // Click at EOL returns cp after CRLF
    BOOL       fIgnoreBeforeAfterSpace, // TRUE if ignoring pbefore/after space
    BOOL       fExactFit,               // TRUE if cp should always round down (for element-hit-testing)
    BOOL      *pfRightOfCp,
    BOOL      *pfPsuedoHit,
    CCalcInfo *pci)
{
    CFlowLayout *pFlowLayout = GetFlowLayout();
    CElement    *pElementFL  = pFlowLayout->ElementOwner();
    CCalcInfo    CI;
    CLine       *pli = Elem(ili);
    LONG         cch = 0;
    LONG         dx = 0;
    LONG         xPos;
    BOOL         fPsuedoHit = FALSE;
    CTreePos    *ptp = NULL;
    CTreeNode   *pNode;
    
    if (!pci)
    {
        CI.Init(pFlowLayout);
        pci = &CI;
    }

    if (   fIgnoreBeforeAfterSpace
        && (   pli == NULL
            || (   !pli->_fRelative
                && pli->_fSingleSite
               )
           )
       )
    {
        return -1 ;
    }

    if (pli)
    {
        if (pli->IsFrame())
        {
            if(ppLayout)
            {
                *ppLayout = pli->_pNodeLayout->GetLayout();
            }
            cch = 0;

            if(pfRightOfCp)
                *pfRightOfCp = TRUE;
        }
        else
        {
            if (!fIgnoreBeforeAfterSpace &&
                pli->_fHasNestedRunOwner &&
                yLine + pli->_yHeight <= pt.y)
            {
                // If the we are not ignoring whitespaces and we have hit a line
                // which has a nested runowner, but are BELOW the line (happens when
                // that line is the last line in the document) then we want
                // to be at the end of that line. The measurer would put us at the
                // beginning or end depending upon the X position.
                cp += pli->_cch;
            }
            else
            {
                // Create measurer
                CLSMeasurer me(this, pci);
                LONG xRelOffset;
                LONG yHeightRubyBase = 0;

                AssertSz((pli != NULL) || (ili == 0),
                         "CDisplay::CpFromPoint invalid line pointer");

                if (!me._pLS)
                    return -1;

                // Get character in the line
                me.SetCp(cp, NULL);

                xPos = pt.x;

#ifdef FIXUPRELSTUFF
//                    xRelOffset = 0;
#else
                    xRelOffset = 0;
#endif

                // The y-coordinate should be relative to the baseline, and positive going up
                cch = pli->CchFromXpos(me, xPos - xRelOffset, yLine + pli->_yHeight - pli->_yDescent - pt.y, &dx, fExactFit, 
                                       &yHeightRubyBase);

                if (pfRightOfCp)
                    *pfRightOfCp = dx < 0;

                if (ppLayout)
                {
                    ptp = me.GetPtp();
                    if (ptp->IsBeginElementScope())
                    {
                        pNode = ptp->Branch();
                        if (   pNode->HasLayout()
                            && pNode->IsInlinedElement()
                           )
                        {
                            *ppLayout = pNode->GetLayout();
                        }
                        else
                        {
                            *ppLayout = NULL;
                        }
                    }
                }

                // Don't allow click at EOL to select EOL marker and take into account
                // single line edits as well

                if (!fAllowEOL && cch > 0)
                {
                    CTxtPtr tp(GetMarkup(), me.GetCp());
                    
                    // NB: CTxtPtr::IsAfterEOP() returns TRUE for all synthetic breaks
                    if (tp.IsAfterEOP())
                    {
                        // Adjust the position on the line by the amount backed up
                        cch += tp.BackupCpCRLF();

                        me.SetCp(tp.GetCp(), NULL);
                    }

                    ptp = me.GetPtp();
                    Assert(ptp);
                    while (cp <= ptp->GetCp() && !ptp->IsText())
                    {
                        if (ptp->IsEndNode() && ptp->Branch()->Element()->Tag() == ETAG_BR)
                        {
                            ptp = ptp->PreviousTreePos();
                            me.SetPtp(ptp, -1);
                            cch -= 2;
                            break;
                        }
                        else
                        {
                            cch -= ptp->GetCch();
                            ptp = ptp->PreviousTreePos();
                            Assert(ptp);
                        }
                    }
                }

                // Check if the pt is within bounds *vertically* too.
                if (fIgnoreBeforeAfterSpace)
                {
                    LONG top, bottom;

                    ptp = me.GetPtp();
                    if (   ptp->IsBeginElementScope()
                        && ptp->Branch()->HasLayout()
                       )
                    {
                        // Hit a site. Check if we are within the boundaries
                        // of the site.
                        RECT rc;
                        CLayout *pLayout = ptp->Branch()->GetLayout();
                        
// BUGBUG: This is wrong, fix it (brendand)
                        pLayout->GetRect(&rc);

                        top = rc.top;
                        bottom = rc.bottom;
                    }
                    else
                    {
                        GetTopBottomForCharEx(pci,
                                              &top,
                                              &bottom,
                                              yLine,
                                              pli,
                                              xPos - xRelOffset,
                                              me.CurrBranch()->GetCharFormat());
                        top -= yHeightRubyBase;
                        bottom -= yHeightRubyBase;
                    }


                    // BUGBUG (t-ramar): this is fine for finding 99% of the
                    // pseudo hits, but if someone has a ruby with wider base
                    // text than pronunciation text, or vice versa, hits in the
                    // whitespace that results will not register as pseudo hits.
                    if ((pt.y <  top) ||
                        (pt.y >= bottom)
                       )
                    {
                        fPsuedoHit = TRUE;
                    }
                }
                cp = (LONG)me.GetCp();

                ptp = me.GetPtp();
            }
        }
    }

    if(prp)
        prp->RpSet(ili, cch);

    if(pfPsuedoHit)
        *pfPsuedoHit = fPsuedoHit;

    if(pptp)
    {
        LONG ich;

        *pptp = ptp ? ptp : pFlowLayout->GetContentMarkup()->TreePosAtCp(cp, &ich);
    }

    return cp;
}

//+----------------------------------------------------------------------------
//
// Member:      CDisplay::PointFromTp(tp, prcClient, fAtEnd, pt, prp, taMode)
//
// Synopsis:    return the origin that corresponds to a given text pointer,
//              relative to the view
//
//-----------------------------------------------------------------------------

LONG CDisplay::PointFromTp(
    LONG        cp,         // point for the cp to be computed
    CTreePos *  ptp,        // tree pos for the cp passed in, can be NULL
    BOOL fAtEnd,            // Return end of previous line for ambiguous cp
    POINT &pt,              // Returns point at cp in client coords
    CLinePtr * const prp,   // Returns line pointer at tp (may be null)
    UINT taMode,            // Text Align mode: top, baseline, bottom
    CCalcInfo *pci,
    BOOL *pfComplexLine)
{
    CFlowLayout * pFL = GetFlowLayout();
    CLinePtr    rp(this);
    BOOL        fLastTextLine;
    CCalcInfo   CI;
    BOOL        fRTL;
    fRTL = rp.IsValid() ? rp->_fRTL : FALSE;
    RubyInfo rubyInfo = {-1, 0, 0};

    if(!pci)
    {
        CI.Init(pFL);
        pci = &CI;
    }

    if (pFL->IsDisplayNone() ||
        !WaitForRecalc(cp, -1, pci))
        return -1;

    if(!rp.RpSetCp(cp, fAtEnd))
        return -1;

    pt.y = YposFromLine(rp, pci);

    if(!fRTL)
    {
        pt.x = rp.IsValid()
                    ? rp->_xLeft + rp->_xLeftMargin
                    : 0;
    }
    else
    {
        pt.x = rp.IsValid()
                    ? rp->_xRight + rp->_xRightMargin
                    : 0;
    }
    fLastTextLine = rp.IsLastTextLine();

    if (rp.RpGetIch())
    {
        CLSMeasurer me(this, pci);
        if (!me._pLS)
            return -1;
        // Backup to start of line
        if(rp.GetIch())
            me.SetCp(cp - rp.RpGetIch(), NULL);
        else
            me.SetCp(cp, ptp);

        // And measure from there to where we are
        me.NewLine(*rp);

        if (rp.IsValid())
        {
            me._li._xLeft = rp->_xLeft;
            me._li._xLeftMargin = rp->_xLeftMargin;
            me._li._xRight = rp->_xRight;
            me._li._xRightMargin = rp->_xRightMargin;
        }

        LONG xCalc = me.MeasureText(rp.RpGetIch(), rp->_cch, pfComplexLine, &rubyInfo);

        // Remember we ignore trailing spaces at the end of the line
        // in the width, therefore the x value that MeasureText finds can
        // be greater than the width in the line so we truncate to the
        // previously calculated width which will ignore the spaces.
        // pt.x += min(xCalc, rp->_xWidth);
        //
        // Why anyone would want to ignore the trailling spaces at the end
        // of the line is beyond me. For certain, we DON'T want to ignore
        // them when placing the caret at the end of a line with trailling
        // spaces. If you can figure out a reason to ignore the spaces,
        // please do, just leave the caret placement case intact. - Arye
        pt.x += xCalc;
    }

    if (prp)
        *prp = rp;

    if(rp >= 0 && taMode != TA_TOP)
    {
        // Check for vertical calculation request
        if (taMode & TA_BOTTOM)
        {
            const CLine *pli = Elem(rp);

            if(pli)
            {
                pt.y += pli->_yHeight;
                if(rubyInfo.cp != -1)
                {
                    pt.y -= rubyInfo.yHeightRubyBase + pli->_yDescent - pli->_yTxtDescent;
                }
                
                if (!pli->IsFrame() &&
                    (taMode & TA_BASELINE) == TA_BASELINE)
                {
                    if(rubyInfo.cp != -1)
                    {
                        pt.y -= rubyInfo.yDescentRubyText;
                    }
                    else
                    {
                        pt.y -= pli->_yDescent;
                    }
                }
            }
        }

        // Do any specical horizontal calculation
        if (taMode & TA_CENTER)
        {
            CLSMeasurer me(this, pci);

            if (!me._pLS)
                return -1;

            me.SetCp(cp, ptp);

            me.NewLine(*rp);

            pt.x += (me.MeasureText(1, rp->_cch) >> 1);
        }
    }

#ifdef FIXUPRELSTUFF
    // If the line is relative shift it by its relative offset.
    ShiftPointByRelativeOffset(rp, &pt);
#endif

    return rp;
}

//+----------------------------------------------------------------------------
//
// Function:    AppendRectToElemRegion
//
// Synopsis:    Utility function for region from element, which appends the
//              given rect to region if it is within the clip range and
//              updates the bounding rect.
//
//-----------------------------------------------------------------------------
void  AppendRectToElemRegion(CDataAry <RECT> * paryRects, RECT * prcBound,
                             RECT * prcLine, CPoint & ptTrans,
                             LONG cp, LONG cpClipStart, LONG cpClipFinish)
{
    if(ptTrans.x || ptTrans.y)
        OffsetRect(prcLine, ptTrans.x, ptTrans.y);

    if(cp >= cpClipStart && cp <= cpClipFinish)
    {
        paryRects->AppendIndirect(prcLine);
    }
    if(prcBound)
    {
        UnionRect(prcBound, prcBound, prcLine);
    }
}

//+----------------------------------------------------------------------------
//
// Function:    RcFromAlignedLine
//
// Synopsis:    Utility function for region from element, which computes the
//              rect for a given aligned line
//
//-----------------------------------------------------------------------------
void
RcFromAlignedLine(RECT * prcLine, CLine * pli, LONG yPos,
                  BOOL fBlockElement, BOOL fFirstLine, BOOL fRTL,
                  long xParentLeftIndent, long xParentRightIndent)
{
    // add the curent line to the region
    prcLine->top = yPos + pli->GetYTop();
    if(fBlockElement && !fFirstLine && pli->_yBeforeSpace > 0)
        prcLine->top -= pli->_yBeforeSpace;
    if(!fRTL)
    {
        prcLine->left = pli->_xLeftMargin;
        prcLine->right = pli->GetTextRight();
    }
    else
    {
        prcLine->right = -pli->_xRightMargin;
        prcLine->left = -(pli->GetRTLTextLeft());
    }
    prcLine->bottom = yPos + pli->GetYBottom();

    // if we are computing region for a block element
    // then account for the padding and border from
    // parent block elements
    if(fBlockElement)
    {
        if(!fRTL)
        {
            if(pli->_fLeftAligned)
                prcLine->left += xParentLeftIndent;
            if(pli->_fRightAligned)
                prcLine->right += pli->_xRight - xParentRightIndent;
        }
        else
        {
            if(pli->_fLeftAligned)
                prcLine->left -= pli->_xLeft - xParentLeftIndent;
            if(pli->_fRightAligned)
                prcLine->right -= xParentRightIndent;
        }
    }
    else
    {
        if(!fRTL)
            prcLine->left += pli->_xLeft;
        else
            prcLine->right -= pli->_xRight;
    }
}

//+----------------------------------------------------------------------------
//
// Function:    ComputeIndentsFromParentNode
//
// Synopsis:    Compute the indent for a given Node and a left and/or
//              right aligned site that a current line is aligned to.
//
//-----------------------------------------------------------------------------

void ComputeIndentsFromParentNode(CRecalcLinePtr & rclptr, CCalcInfo * pci,
                           CTreeNode * pNode, CElement * pElementFL,
                           LONG * pxLeftIndent, LONG * pxRightIndent)
{
    CElement*   pElement = pNode->Element();
    CTreeNode * pNodeCommon = NULL;
    LONG        xParentLeftPadding, xParentRightPadding;
    LONG        xParentLeftBorder, xParentRightBorder;

    const CParaFormat *pPF = pNode->GetParaFormat();
    BOOL  fInner = pNode->Element() == pElementFL;

    Assert(pNode);

    // compute the left and right margin for the given element
    // we pay attention to direction to compensate correctly for list items
    LONG        xLeftMargin  = pPF->GetLeftIndent(pci, fInner) - (!pPF->HasRTL(fInner) ?
                                    ((pElement->HasFlag(TAGDESC_LISTITEM) ||
                                      pElement->HasFlag(TAGDESC_LIST))
                                            ? pPF->GetBulletOffset(pci, fInner)
                                            : 0) : 0);
    LONG        xRightMargin = pPF->GetRightIndent(pci, fInner) - (!pPF->HasRTL(fInner) ? 0 :
                                    ((pElement->HasFlag(TAGDESC_LISTITEM) ||
                                      pElement->HasFlag(TAGDESC_LIST))
                                            ? pPF->GetBulletOffset(pci, fInner)
                                            : 0));

    // compute the padding and border space cause by the current
    // elements parent.
    if(pNode->Element() == pElementFL)
    {
        xParentLeftPadding = xParentLeftBorder =
            xParentRightPadding = xParentRightBorder = 0;
    }
    else
    {
        xLeftMargin  += (rclptr._marginInfo.HasLeftMargin()  ? 0 : rclptr._xLayoutLeftIndent);
        xRightMargin += (rclptr._marginInfo.HasRightMargin() ? 0 : rclptr._xLayoutRightIndent);

        pNode->Parent()->Element()->ComputeHorzBorderAndPadding(pci, pNode->Parent(), pElementFL,
                                &xParentLeftBorder, &xParentLeftPadding,
                                &xParentRightBorder, &xParentRightPadding);
    }

    *pxLeftIndent = xLeftMargin + xParentLeftBorder + xParentLeftPadding;
    *pxRightIndent = xRightMargin + xParentRightBorder + xParentRightPadding;

    // if the current line is aligned to a site, find the common margins and
    // subtract the common margins.
    if(rclptr._pNodeLeftTop)
    {
        pNodeCommon = pNode->GetFirstCommonAncestor(
                                        rclptr._pNodeLeftTop,
                                        pElementFL);
        if(SameScope(pNodeCommon, pNode))
        {
            *pxLeftIndent = 0;
        }
        else
        {
            LONG    xCommonLeftBorder, xCommonLeftPadding, xCommonLeftMargin;

            pNodeCommon->Element()->ComputeHorzBorderAndPadding(pci, pNodeCommon, pElementFL,
                                &xCommonLeftBorder, &xCommonLeftPadding, NULL, NULL);

            xCommonLeftMargin = pNodeCommon->GetParaFormat()->GetLeftIndent(pci, pNodeCommon->Element() == pElementFL);

            *pxLeftIndent -= xCommonLeftMargin +
                                xCommonLeftBorder +
                                xCommonLeftPadding;
        }
    }

    // if the current line is aligned to a site, find the common margins and
    // subtract the common margins.
    if(rclptr._pNodeRightTop)
    {
        pNodeCommon = pNode->GetFirstCommonAncestor(
                                        rclptr._pNodeRightTop,
                                        pElementFL);
        if(SameScope(pNodeCommon, pNode))
        {
            *pxRightIndent = 0;
        }
        else
        {
            LONG    xCommonRightBorder, xCommonRightPadding, xCommonRightMargin;

            pNodeCommon->Element()->ComputeHorzBorderAndPadding(pci, pNodeCommon, pElementFL,
                                &xCommonRightBorder, &xCommonRightPadding, NULL, NULL);

            xCommonRightMargin = pNodeCommon->GetParaFormat()->GetRightIndent(pci, pNodeCommon->Element() == pElementFL);


            *pxRightIndent -= xCommonRightMargin +
                                xCommonRightBorder +
                                xCommonRightPadding;
        }
    }
}

//+----------------------------------------------------------------------------
//
// Member:      RegionFromElement
//
// Synopsis:    for a given element, find the set of rects (or lines) that this
//              element occupies in the display. The rects returned are relative
//              the site origin.
//
//-----------------------------------------------------------------------------

void
CDisplay::RegionFromElement(CElement       * pElement,
                            CDataAry<RECT> * paryRects,
                            CPoint         * pptOffset, // == NULL, point to offset the rects by
                            CFormDrawInfo  * pDI,       // == NULL
                            DWORD dwFlags,              // == 0
                            LONG cpClipStart,           // == -1
                            LONG cpClipFinish,          // == -1
                            RECT * prcBound)            // == NULL

{
    CFlowLayout *       pFL = GetFlowLayout();
    CElement *          pElementFL = pFL->ElementOwner();
    CTreePos *          ptpStart;
    CTreePos *          ptpFinish;
    CCalcInfo           CI;
    RECT                rcLine;
    CPoint              ptTrans = g_Zero.pt;
    BOOL                fRTL;
    BOOL                fScreenCoord = dwFlags & RFE_SCREENCOORD ? TRUE : FALSE;
    BOOL                fClipped     = dwFlags & RFE_CLIPPED     ? TRUE : FALSE;

    Assert( pElement->IsInMarkup() );

    // clear the array before filling it
    paryRects->SetSize(0);

    if(prcBound)
        memset(prcBound, 0, sizeof(RECT));

    if(pElementFL->IsDisplayNone() || !pElement || pElement->IsDisplayNone() ||
       pElement->Tag() == ETAG_ROOT || pElementFL->Tag() == ETAG_ROOT)
        return;

    if (pElement == pElementFL)
    {
        pFL->GetContentTreeExtent(&ptpStart, &ptpFinish);
    }
    else
    {
        pElement->GetTreeExtent(&ptpStart, &ptpFinish);
    }
    Assert(ptpStart && ptpFinish);

    // now that we have the scope of the element, find its range.
    // and compute the rects (lines) that contain the range.

    {
        CTreePos          * ptpElemStart;
        CTreePos          * ptpElemFinish;
        CTreeNode         * pNode = pElement->GetFirstBranch();
        const CParaFormat * pPF = pNode->GetParaFormat();
        CLSMeasurer         me(this, pDI);
        CMarkup           * pMarkup = pFL->GetContentMarkup();
        BOOL                fBlockElement;
        LONG                cp, cpStart, cpFinish, cpElementLast;
        LONG                cpElementStart, cpElementFinish;
        LONG                iCurLine, iFirstLine, ich;
        LONG                yPos;
        LONG                xParentRightIndent = 0;
        LONG                xParentLeftIndent = 0;
        LONG                yParentPaddBordTop = 0;
        LONG                yParentPaddBordBottom = 0;
        LONG                yTop;
        BOOL                fPrevLineAligned = TRUE;
        BOOL                fFirstLine = TRUE;
        CLinePtr            rp(this);

        if (!me._pLS)
            goto Cleanup;

        fBlockElement = (!pElement->HasLayout() && pElement->IsBlockElement())
                                            && !(dwFlags & RFE_RANGE_METRIC);
        fRTL = pPF->HasRTL(pElement == pElementFL);

        if (pDI)
        {
            CI.Init(pDI, pFL);
        }
        else
        {
            CI.Init(pFL);
        }

        CRecalcLinePtr  recalcLinePtr(this, &CI);
        long            xParentWidth;
        long            yParentHeight;

        GetViewWidthAndHeightForChild(&CI, &xParentWidth, &yParentHeight);
        CI.SizeToParent(xParentWidth, yParentHeight);

        // If we have been asked to clip, we better be getting the screen coords!
        if (fClipped)
            fScreenCoord = TRUE;


        if(!(dwFlags & RFE_NONRELATIVE))
        {
            pNode->GetRelTopLeft(pElementFL, &CI, &ptTrans.x, &ptTrans.y);
        }

        if (fScreenCoord)
        {
            pFL->TransformPoint( &ptTrans, COORDSYS_CONTENT, COORDSYS_GLOBAL);

        }

        if(pptOffset)
        {
            ptTrans.x += pptOffset->x;
            ptTrans.y += pptOffset->y;
        }

        cpStart  = pFL->GetContentFirstCp();
        cpFinish = pFL->GetContentLastCp();        

#ifdef FIXUP
        // If we have been asked to clip to the visible region
        // then lets start from the first visible cp...
        if (fClipped)
        {
// BUGBUG: Anything to do here? (brendand)
        }
#endif

        // get the cp range for the element
        cpElementStart  = ptpStart->GetCp();
        cpElementFinish = ptpFinish->GetCp();

        cpStart       = max(cpStart, cpElementStart);
        cpFinish      = min(cpFinish, cpElementFinish);
        cpFinish      = min(cpFinish, GetMaxCpCalced());
        cpElementLast = cpFinish;

        if( cpStart != cpElementStart )
            ptpStart  = pMarkup->TreePosAtCp(cpStart, &ich);
        ptpElemStart = ptpStart;

        if( cpFinish != cpElementFinish )
            ptpFinish = pMarkup->TreePosAtCp(cpFinish, &ich);
        ptpElemFinish = ptpFinish;

        if(cpClipStart < cpStart)
            cpClipStart = cpStart;
        if(cpClipFinish == -1 || cpClipFinish > cpFinish)
            cpClipFinish = cpFinish;

        if(!prcBound)
        {
            if(cpStart != cpClipStart)
            {
                cpStart  = cpClipStart;
                ptpStart = pMarkup->TreePosAtCp(cpStart, &ich);
            }
            if(cpFinish != cpClipFinish)
            {
                cpFinish = cpClipFinish;
                ptpFinish = pMarkup->TreePosAtCp(cpFinish, &ich);
            }
        }

        if(!GetFlowLayout()->FExternalLayout() && !LineCount())
            return;

        // BUGBUG: we pass in absolute cp here so RpSetCp must call
        // pElementFL->GetFirstCp while we have just done this.  We
        // should optimize this out by having a relative version of RpSetCp.

        if (!GetFlowLayout()->FExternalLayout())
        {
            // skip frames and position it at the beginning of the line
            rp.RpSetCp(cpStart, FALSE, TRUE);
        }
        
        // if the element has no content then return the null rectangle
        if(cpStart == cpFinish)
        {
            if (!GetFlowLayout()->FExternalLayout())
            {
                CLine * pli = Elem(rp);

                yPos = YposFromLine(rp, &CI);

                rcLine.top = yPos + rp->GetYTop();
                rcLine.bottom = yPos + rp->GetYBottom();

                rcLine.left = rp->GetTextLeft();

                if(rp.GetIch())
                {
                    me.SetCp(cpStart - rp.GetIch(), NULL);
                    me.NewLine(*pli);
                    me._li._xLeft = pli->_xLeft;    // Need this to calc tabs correctly.
                    me._li._xLeftMargin = pli->_xLeftMargin;

                    rcLine.left +=  me.MeasureText(rp.RpGetIch(), pli->_cch);
                }
                rcLine.right = rcLine.left;
            }
            else
            {
                rcLine.top =
                rcLine.bottom =
                rcLine.left =
                rcLine.right = 0;
            }

            AppendRectToElemRegion(paryRects, prcBound, &rcLine,
                                    ptTrans,
                                    cpStart, cpClipStart, cpClipFinish);
            return;
        }

        if (GetFlowLayout()->FExternalLayout())
        {
            if (GetFlowLayout()->GetQuillGlue())
                GetFlowLayout()->GetQuillGlue()->RegionFromElement(pElement, paryRects, cpStart, cpFinish, cpClipStart, cpClipFinish,
                                                                   !fScreenCoord, fClipped, fBlockElement, prcBound);

            return;
        }

        // Compute the padding and border space for the first line
        // and the last line for ancestors that are comming and leaving
        // scope respectively
        if(pElement && fBlockElement && pElement != pElementFL)
        {
            ComputeVertPaddAndBordFromParentNode(&CI,
                    ptpStart, ptpFinish,
                    &yParentPaddBordTop, &yParentPaddBordBottom);
        }

        iCurLine = iFirstLine = rp;

        // now that we know what line contains the cp, let's try to compute the
        // bounding rectangle.

        // if the line has aligned images in it and if they are at the begining of the
        // the line, back up all the frame lines that are before text.
        if(rp->HasAligned())
        {
            LONG diLine = -1;
            CLine * pli;

            while((iCurLine + diLine >= 0) && (pli = &rp[diLine]) && pli->IsBlankLine())
            {
                if(pli->IsFrame() && (!pli->_fFrameBeforeText || pli->_pNodeLayout->Element()->GetLastCp() < cpStart))
                {
                    break;
                }

                diLine--;
            }
            iFirstLine = iCurLine + diLine + 1;
        }

        // compute the ypos for the first line
        yTop = yPos = YposFromLine(iFirstLine, &CI);

        if(fBlockElement)
        {
            recalcLinePtr.Init((CLineArray *)this, 0, NULL);
        }

        // now add all the frame lines before the current line under the influence of
        // the element to the region
        for( ; iFirstLine < iCurLine; iFirstLine++)
        {
            CLine * pli = Elem(iFirstLine);

            // If the current line being measured has invalid margins, a line that is
            // below an aligned line, margins have changed so recalculate margins.
            if (fBlockElement && (fPrevLineAligned ||
                !recalcLinePtr.IsValidMargins(yPos + pli->_yBeforeSpace)))
            {
                CTreeNode * pNodeTemp;

                recalcLinePtr.RecalcMargins(0, iFirstLine, yPos, pli->_yBeforeSpace);

                pNodeTemp = pMarkup->SearchBranchForScopeInStory(ptpStart->GetBranch(), pElement);

                Assert(pNodeTemp);

                ComputeIndentsFromParentNode(recalcLinePtr, &CI, pNodeTemp, pElementFL,
                                            &xParentLeftIndent, &xParentRightIndent);
            }

            if(pli->IsFrame())
            {
                long cpLayoutStart  = pli->_pNodeLayout->Element()->GetFirstCp();
                long cpLayoutFinish = pli->_pNodeLayout->Element()->GetLastCp();

                fPrevLineAligned = TRUE;

                if(cpLayoutStart >= cpStart && cpLayoutFinish <= cpFinish)
                {
                    RcFromAlignedLine(&rcLine, pli, yPos,
                                        fBlockElement, fFirstLine, fRTL,
                                        xParentLeftIndent, xParentRightIndent);

                    AppendRectToElemRegion(paryRects, prcBound, &rcLine,
                                           ptTrans,
                                           cpFinish, cpClipStart, cpClipFinish);
                }
            }
            else if(pli->_fForceNewLine)
            {
                // clear lines
                yPos += pli->_yHeight;
                fPrevLineAligned = FALSE;
            }
        }

        // now add all the lines that are contained by the range
        for (cp = cpStart; cp < cpFinish; iCurLine++)
        {
            LONG    xStart = 0;
            LONG    xEnd = 0;
            LONG    cchAdvance = min(rp.GetCchRemaining(), cpFinish - cp);
            CLine * pli;

            if(iCurLine >= LineCount())
            {
                // (srinib) - please check with one of the text
                // team members if anyone hits this assert. We want
                // to investigate. We shoud never be here unless
                // we run out of memory.
                AssertSz(FALSE, "WaitForRecalc failed to measure lines till cpFinish");
                break;
            }

            pli = Elem(iCurLine);

            if (fBlockElement && (fPrevLineAligned ||
                !recalcLinePtr.IsValidMargins(yPos + pli->_yBeforeSpace)))
            {
                CTreeNode * pNodeTemp;

                recalcLinePtr.RecalcMargins(0, iCurLine, yPos, pli->_yBeforeSpace);

                if(cp != cpStart)
                {
                    ptpStart = pMarkup->TreePosAtCp(cp, &ich);
                }

                pNodeTemp = pMarkup->SearchBranchForScopeInStory(ptpStart->GetBranch(), pElement);

                Assert(pNodeTemp);

                ComputeIndentsFromParentNode(recalcLinePtr, &CI, pNodeTemp, pElementFL,
                                            &xParentLeftIndent, &xParentRightIndent);
            }

            // measure the xpos for the first text line
            if(rp.GetIch() && !rp->IsBlankLine())
            {
                Assert(cp == cpStart);
                ptpStart = pMarkup->TreePosAtCp(cpStart - rp.GetIch(), &ich);
                me.SetCp(cpStart - rp.GetIch(), ptpStart);
                me._li = *pli;
                if(!fRTL)
                {
                    xStart = me.MeasureText(rp.RpGetIch(), pli->_cch);
                }
                else
                {
                    // BUGBUG (a-pauln) this is returning the
                    // line width and not the location of the
                    // start.
                    xStart = -(me.MeasureText(rp.RpGetIch(), pli->_cch));
                }
            }

            fPrevLineAligned = pli->IsFrame();

            // for right-to-left reading blocks we are going to be taking the offset
            // at draw time from view.right. Therefore, in RTL displays we will have
            // xStart be the right side of the line and xEnd be the left. Don't panic,
            // the numbers will be negative.
            if(!fRTL)
                xStart += pli->_xLeftMargin;
            else
                xStart -= pli->_xRightMargin;

            // if the line ends midway then compute the end of the
            // element otherwise use the right end of the text
            if(max(0, (rp.GetCchRemaining() - (fBlockElement ? rp->_cchWhite : 0))) > cchAdvance)
            {
                // if cp == cpStart, measurer is already positioned at the right cp.
                if(cp - rp.GetIch() != me.GetCp())
                {
                    me.SetCp(cp - rp.GetIch(), cp == cpStart ? ptpStart : NULL);
                }

                me._li = *pli;
                if(!fRTL)
                {
                    xEnd = me.MeasureText(rp.GetIch() + cchAdvance, pli->_cch);
                    xEnd += pli->_xLeft;
                }
                else
                {
                    xEnd = -(me.MeasureText(rp.GetIch() + cchAdvance, pli->_cch));
                    xEnd -= pli->_xRight;
                }
            }
            else
            {
                if(fBlockElement)
                {
                    if(!fRTL)
                    {
                        xEnd += (pli->_xRightMargin || pli->IsFrame())
                                    // right edge of the line
                                    ? pli->_xLeftMargin + pli->_xLineWidth
                                    // right edge of the display
                                    : max(pli->_xLeftMargin + pli->_xLineWidth, GetWidth());
                        if(!pli->_fLeftAligned)
                            xEnd -=  xParentRightIndent;
                    }
                    else
                    {
                        xEnd -= (pli->_xLeftMargin || pli->IsFrame())
                                    // left edge of the line
                                    ? pli->_xRightMargin + pli->_xLineWidth
                                    // right edge of the display
                                    : max(pli->_xRightMargin + pli->_xLineWidth, GetWidth());
                        if(!pli->_fRightAligned)
                            xEnd +=  xParentLeftIndent;
                    }
                }
                else
                {
                    if(!fRTL)
                        xEnd += pli->GetTextRight() + pli->_xWhite;
                    else
                    {
                        // BUGBUG (a-pauln) I need to verify, but I don't think
                        // we want the xWhite included in RTL lines.
                        xEnd -= pli->GetRTLTextLeft();
                    }
                }
            }

            // adjust the start of the line to include padding and border space
            if(!pli->_fRightAligned)
            {
                if(fBlockElement)
                {
                    if(!fRTL)
                        xStart += xParentLeftIndent;
                    else
                        xStart -= xParentRightIndent;
                }
                else
                {
                    if(!fRTL)
                        xStart += pli->_xLeft;
                    else
                    {
                        // BUGBUG (a-pauln) Doesn't this mess up xEnd now?
                        xStart -= pli->_xRight;
                    }
                }
            }

            // add the rect to the line
            {
                if(fBlockElement)
                {
                    rcLine.top = yPos;
                    rcLine.bottom = yPos + max(pli->_yHeight, pli->GetYBottom());

                    if(fFirstLine)
                    {
                        rcLine.top += pli->_yBeforeSpace + 
                                      min(0L, pli->GetYHeightTopOff()) +
                                      yParentPaddBordTop;
                    }
                    else
                    {
                        rcLine.top += min(0L, pli->GetYTop());
                    }

                    if(pli->_fForceNewLine && cp + cchAdvance >= cpElementLast)
                    {
                        rcLine.bottom -= yParentPaddBordBottom;
                    }
                }
                else
                {
                    rcLine.top = yPos + pli->GetYTop();
                    rcLine.bottom = yPos + pli->GetYBottom();
                }

                if(!fRTL)
                {
                    rcLine.left     = xStart;
                    rcLine.right    = xEnd;
                }
                else
                {
                    rcLine.left     = xEnd;
                    rcLine.right    = xStart;
                }

                // RTL lines need to be offset the amount of their
                // trailing white space
                AppendRectToElemRegion(paryRects, prcBound, &rcLine,
                                       ptTrans,
                                       cp, cpClipStart, cpClipFinish);

                cp += cchAdvance;
            }

            if(cchAdvance)
            {
                rp.RpAdvanceCp(cchAdvance, FALSE);
            }
            else
                rp.AdjustForward(); // frame lines or clear lines

            if(pli->_fForceNewLine)
            {
                yPos += pli->_yHeight;
                fFirstLine = FALSE;
            }
        }

        if(!rp.GetIch())
            rp.AdjustBackward();

        iCurLine = rp;

        // now if the last line contains any aligned images, check to see if
        // there are any aligned lines following the current line that come
        // under the scope of the element
        if(rp->HasAligned())
        {
            LONG diLine = 1;
            CLine * pli;

            // we dont have to worry about clear lines because, all the aligned lines that
            // follow should be consecutive.
            while((iCurLine + diLine < LineCount()) &&
                    (pli = &rp[diLine]) && pli->IsFrame() && !pli->_fFrameBeforeText)
            {
                long cpLayoutStart  = pli->_pNodeLayout->Element()->GetFirstCp();
                long cpLayoutFinish = pli->_pNodeLayout->Element()->GetLastCp();

                if (fBlockElement && (fPrevLineAligned ||
                    !recalcLinePtr.IsValidMargins(yPos + pli->_yBeforeSpace)))
                {
                    CTreeNode * pNodeTemp;

                    recalcLinePtr.RecalcMargins(0, iCurLine + diLine,
                                                    yPos, pli->_yBeforeSpace);

                    pNodeTemp = pMarkup->SearchBranchForScopeInStory (ptpFinish->GetBranch(), pElement);

                    Assert(pNodeTemp);

                    ComputeIndentsFromParentNode(recalcLinePtr, &CI, pNodeTemp, pElementFL,
                                                &xParentLeftIndent, &xParentRightIndent);
                }

                // if the current line is a frame line and if the site contained
                // in it is contained by the current element include it other wise
                if(cpStart <= cpLayoutStart && cpFinish >= cpLayoutFinish)
                {
                    RcFromAlignedLine(&rcLine, pli, yPos,
                                        fBlockElement, fFirstLine, fRTL,
                                        xParentLeftIndent, xParentRightIndent);

                    AppendRectToElemRegion(paryRects, prcBound, &rcLine,
                                           ptTrans,
                                           cpFinish, cpClipStart, cpClipFinish);
                }
                diLine++;
                fPrevLineAligned = TRUE;
            }
        }
    }
Cleanup:
    return;
}


//+----------------------------------------------------------------------------
//
// Member:      RegionFromRange
//
// Synopsis:    Return the set of rectangles that encompass a range of
//              characters
//
//-----------------------------------------------------------------------------

void
CDisplay::RegionFromRange(
    CDataAry<RECT> *    paryRects,
    long                cp,
    long                cch)
{
    CFlowLayout *   pFlowLayout = GetFlowLayout();
    long            cpFirst     = pFlowLayout->GetContentFirstCp();
    CLinePtr        rp(this);
    CLine *         pli;
    CRect           rc;
    long            ili;
    long            yTop, yBottom;

    if (    pFlowLayout->IsRangeBeforeDirty(cp - cpFirst, cch)
        &&  rp.RpSetCp(cp, FALSE, TRUE))
    {
        //
        //  First, determine the starting line
        //

        ili = rp;

        if(rp->HasAligned())
        {
            while(ili > 0)
            {
                pli = Elem(ili);

                if (    !pli->IsFrame()
                    ||  !pli->_fFrameBeforeText
                    ||  pli->_pNodeLayout->Element()->GetFirstCp() < cp)
                    break;

                Assert(pli->_cch == 0);
                ili--;
            }
        }

        //
        //  Start with an empty rectangle (whose width is that of the client rectangle)
        //

        GetFlowLayout()->GetClientRect(&rc);

        rc.top    =
        rc.bottom = YposFromLine(rp);

        //
        //  Extend the rectangle over the affected lines
        //

        for (; cch && ili < LineCount(); ili++)
        {
            pli = Elem(ili);

            yTop    = rc.top + pli->GetYLineTop();
            yBottom = rc.bottom + pli->GetYLineBottom();

            rc.top    = min(rc.top, yTop);
            rc.bottom = max(rc.bottom, yBottom);

            Assert( !pli->IsFrame()
                ||  !pli->_cch);
            cch -= pli->_cch;
        }

        //
        //  Save the invalid rectangle
        //

        if (rc.top != rc.bottom)
        {
            paryRects->AppendIndirect(&rc);
        }
    }
}


//+----------------------------------------------------------------------------
//
// Member:      RenderedPointFromTp
//
// Synopsis:    Find the rendered position of a given cp. For cp that corresponds
//              normal text return its position in the line. For cp that points
//              to an aligned site find the aligned line rather than its poition
//              in the text flow. This function also takes care of relatively
//              positioned lines. Returns point relative to the display
//
//-----------------------------------------------------------------------------

LONG
CDisplay::RenderedPointFromTp(
    LONG        cp,         // point for the cp to be computed
    CTreePos *  ptp,        // tree pos for the cp passed in, can be NULL
    BOOL        fAtEnd,     // Return end of previous line for ambiguous cp
    POINT &     pt,         // Returns point at cp in client coords
    CLinePtr * const prp,   // Returns line pointer at tp (may be null)
    UINT taMode,            // Text Align mode: top, baseline, bottom
    CCalcInfo * pci)
{
    CFlowLayout * pFlowLayout = GetFlowLayout();
    CLayout     * pLayout = NULL;
    CElement    * pElementLayout = NULL;
    CLinePtr    rp(this);
    LONG        ili;
    BOOL        fAlignedSite = FALSE;
    CCalcInfo   CI;
    CTreeNode * pNode;

    if(!pci)
    {
        CI.Init(pFlowLayout);
        pci = &CI;
    }

    if (pFlowLayout->IsDisplayNone() || !WaitForRecalc(cp, -1, pci))
        return -1;

    // now position the line array point to the cp in the rtp.
    // Skip frames, let us worry about them latter.
    if(!rp.RpSetCp(cp, FALSE))
        return -1;

    if(!ptp)
    {
        LONG ich;
        ptp = pFlowLayout->GetContentMarkup()->TreePosAtCp(cp, &ich);
    }

    pNode   = ptp->GetBranch();
    pLayout = pNode->GetNearestLayout();

    if(pLayout != pFlowLayout)
    {
        pElementLayout = pLayout ? pLayout->ElementOwner() : NULL;

        // is the current run owner the txtsite, if not get the runowner
        if(rp->_fHasNestedRunOwner)
        {
            CLayout *   pRunOwner = pFlowLayout->GetContentMarkup()->GetRunOwner(pNode, pFlowLayout);

            if(pRunOwner != pFlowLayout)
            {
                pLayout = pRunOwner;
                pElementLayout = pLayout->ElementOwner();
            }
        }

        // if the site is left or right aligned and not a hr
        if (    !pElementLayout->IsInlinedElement()
            &&  !pElementLayout->IsAbsolute())
        {
            long    iDLine = -1;
            BOOL    fFound = FALSE;

            fAlignedSite = TRUE;

            // run back and forth in the line array and figure out
            // which line belongs to the current site

            // first pass let's go back, look at lines before the current line
            while(rp + iDLine >= 0 && !fFound)
            {
                CLine *pLine = &rp[iDLine];

                if(pLine->IsClear() || (pLine->IsFrame() && pLine->_fFrameBeforeText))
                {
                    if(pLine->IsFrame() && pElementLayout == pLine->_pNodeLayout->Element())
                    {
                        fFound = TRUE;

                        // now back up the linePtr to point to this line
                        rp.SetRun(rp + iDLine, 0);
                        break;
                    }
                }
                else
                {
                    break;
                }
                iDLine--;
            }

            // second pass let's go forward, look at lines after the current line.
            if(!fFound)
            {
                iDLine = 1;
                while(rp + iDLine < LineCount())
                {
                    CLine *pLine = &rp[iDLine];

                    // If it is a frame line
                    if(pLine->IsFrame() && !pLine->_fFrameBeforeText)
                    {
                        if(pElementLayout == pLine->_pNodeLayout->Element())
                        {
                            fFound = TRUE;

                            // now adjust the linePtr to point to this line
                            rp.SetRun(rp + iDLine, 0);
                            break;
                        }
                    }
                    else
                        break;
                    iDLine++;
                }
            }

            // if we didn't find an aligned line, we are in deep trouble.
            Assert(fFound);
        }
    }

    if(!fAlignedSite)
    {
        // If it is not an aligned site then use PointFromTp
        ili = PointFromTp(cp, ptp, fAtEnd, pt, prp, taMode, pci);
        if(ili > 0)
            rp.SetRun(ili, 0);
    }
    else
    {
        ili = rp;

        pt.y = YposFromLine(rp, pci);

        if(!rp->_fRTL)
        {
            pt.x = rp->_xLeft + rp->_xLeftMargin;
        }
        else
        {
            pt.x = rp->_xRight + rp->_xRightMargin;
        }

        if (prp)
            *prp = rp;

#ifdef FIXUPRELSTUFF
        // If the line is relative shift it by its relative offset.
        ShiftPointByRelativeOffset(rp, &pt);
#endif
    }

    return rp;
}

/*
 *  CDisplay::GetLineText(ili, pchBuff, cchMost)
 *
 *  @mfunc
 *      Copy given line of this display into a character buffer
 *
 *  @rdesc
 *      number of character copied
 */
LONG CDisplay::GetLineText(
    LONG ili,           //@parm Line to get text of
    TCHAR *pchBuff,     //@parm Buffer to stuff text into
    LONG cchMost)       //@parm Length of buffer
{
    if (!GetFlowLayout()->IsDisplayNone())
    {
#ifdef MERGEFUN // rtp
        CRchTxtPtr tp (GetFlowLayoutElement());

        // BUGBUG (alexgo): take out the >= 0 test once we start using
        // unsigned line indexes

        if( ili >= 0 && (ili < LineCount() || WaitForRecalcIli(ili)))
        {
            cchMost = min(cchMost, (LONG)Elem(ili)->_cch);
            if(cchMost > 0)
            {
                tp = CpFromLine(ili, NULL);
                return tp._rpTX.GetText(cchMost, pchBuff);
            }
        }
#endif
    }
    *pchBuff = TEXT('\0');
    return 0;
}


/*
 *  CDisplay::UpdateViewForLists()
 *
 *  @mfunc
 *      Invalidate the number regions for numbered list items.
 *
 *  @params
 *      prcView:   The rect for this display
 *      tpFirst:  Where the change happened -- the place where we check for
 *                parentedness by an OL.
 *      iliFirst: The first line where we start checking for a line beginning a
 *                list item. It may not necessarily be the line containing
 *                tpFirst. Could be any line after it.
 *      yPos:     The yPos for the line iliFirst.
 *      prcInval: The rect which returns the invalidation region
 *
 *  @rdesc
 *      TRUE if updated anything else FALSE
 */
BOOL
CDisplay::UpdateViewForLists(
             RECT       *prcView,
             LONG        cpFirst,
             long        iliFirst,
             long        yPos,
             RECT       *prcInval)
{
#ifdef MERGEFUN // rtp
    BOOL fHasListItem = FALSE;
    CLine *pLine = NULL; // Keep compiler happy.

    Assert(prcView);
    Assert(prcInval);

    // BUGBUG (sujalp): We might want to search for other interesting
    // list elements here.
    if (((CRchTxtPtr)tpFirst).SearchBranchForTag(ETAG_OL))
    {
        while (iliFirst < LineCount()   &&
               yPos     < prcView->bottom
              )
        {
            pLine = Elem(iliFirst);

            if (pLine->_fHasBulletOrNum)
            {
                fHasListItem = TRUE;
                break;
            }

            if(pLine->_fForceNewLine)
                yPos += pLine->_yHeight;

            iliFirst++;
        }

        if (fHasListItem)
        {
            // Invalidate the complete strip starting at the current
            // line, right down to the bottom of the view. And only
            // invalidate the strip containing the numbers not the
            // lines themselves.
            prcInval->top    = yPos;
            prcInval->left   = prcView->left;
            prcInval->right  = pLine->_xLeft;
            prcInval->bottom = prcView->bottom;
        }
    }

    return fHasListItem;
#else
    return FALSE;
#endif
}

#ifdef WIN16
#pragma code_seg ( "DISP2_2_TEXT" )
#endif


// ================================  DEBUG methods  ============================================


#if DBG==1
/*
 *  CDisplay::CheckLineArray()
 *
 *  @mfunc
 *      Ensure that the total amount of text in the line array is the same as
 *      that contained in the runs under the scope of the associated CTxtSite.
 *      Additionally, verify the total height of the contained lines matches
 *      the total calculated height.
 */

VOID CDisplay::CheckLineArray()
{
    // If we are marked as needing a recalc or if we are in the process of a
    // background recalc, we cannot verify the line array
    if(!_fRecalcDone)
    {
        return;
    }

    LONG            yHeight  = 0;
    LONG            yHeightMax = 0;
    LONG            cchSum   = 0;
    LONG            cchTotal = GetFlowLayout()->GetContentTextLength();
    LONG            ili      = LineCount();
    CLine const *   pli      = Elem(0);

    while(ili--)
    {
        if(pli->_fForceNewLine)
        {
            yHeight += pli->_yHeight;
            if(yHeightMax < yHeight)
                yHeightMax = yHeight;
        }
        cchSum += pli->_cch;
        pli++;
    }

    if(cchSum != cchTotal)
    {
        TraceTag((tagWarning, "cchSum (%d) != cchTotal (%d)", cchSum, GetFlowLayoutElement()->GetElementCch()));
        AssertSz(FALSE, "cchSum != cchTotal");
    }

    if(yHeightMax != _yHeight)
    {
        TraceTag((tagWarning, "sigma (*this)[]._yHeight = %ld, _yHeight = %ld", yHeight, _yHeight));
        AssertSz(FALSE, "sigma (*this)[]._yHeight != _yHeight");
    }
}


/*
 *  CDisplay::CheckView()
 *
 *  @mfunc
 *      Checks coherence between _iliFirstVisible, _dcpFirstVisible
 *      and _dyFirstVisible
 */
void CDisplay::CheckView()
{
#ifndef DISPLAY_TREE
    LONG yHeight;
    CLinePtr rp(this);

    VerifyFirstVisible();

    yHeight = 0;
    rp = GetFirstVisibleLine();
    while(rp.PrevLine(FALSE, FALSE))
    {
        if (rp->_fForceNewLine)
            yHeight += rp->_yHeight;

    }

    if(yHeight != GetYScroll() + GetFirstVisibleDY())
    {
        TraceTag((tagWarning,
                  "sigma CLine._yHeight = %ld, CDisplay.yFirstLine = %ld",
                  yHeight, GetYScroll() + GetFirstVisibleDY()));
        AssertSz(FALSE, "CLine._yHeight != VIEW.yFirstLine");
    }
#endif
}


/*
 *  CDisplay::VerifyFirstVisible
 *
 *  @mfunc  checks the coherence between FirstVisible line and FirstVisible dCP
 *
 *  @rdesc  TRUE if things are hunky dory; FALSE otherwise
 */
BOOL CDisplay::VerifyFirstVisible()
{
#ifndef DISPLAY_TREE
    CLinePtr    rp(this);
    LONG        cchSum;

    if (GetFlowLayout()->FExternalLayout())
        return TRUE;

    Assert(GetFirstVisibleDCp() >= 0);
    Assert(GetFirstVisibleDCp() <= (GetLastCp() - GetFirstCp()));
    Assert(GetMaxCpCalced() >= GetFirstVisibleCp());

    cchSum = 0;

    rp = GetFirstVisibleLine();
    while(rp.PrevLine(TRUE, FALSE))
    {
        cchSum += rp->_cch;
    }
    if(cchSum != GetFirstVisibleDCp())
    {
        TraceTag((tagWarning, "CDisplay: FirstVisible line(%d - cch: %d) and FirstVisibleDCp(%d) are not in-sync",
                              GetFirstVisibleLine(), cchSum, GetFirstVisibleDCp()));
        AssertSz(FALSE, "cch to GetFirstVisibleLine() != GetFirstVisibleDCp()");

        return FALSE;
    }
#endif

    return TRUE;
}
#endif

#if DBG==1 || defined(DUMPTREE)
void CDisplay::DumpLines()
{
    CTxtPtr      tp ( GetMarkup(), GetFirstCp() );
    LONG            yHeight = 0;

    if (!InitDumpFile())
        return;

    WriteString( g_f,
        _T("\r\n------------- LineArray -------------------------------\r\n" ));


    long    nLines = Count();
    CLine * pLine;

    WriteHelp(g_f, _T("CTxtSite     : 0x<0x>\r\n"), GetFlowLayoutElement());
    WriteHelp(g_f, _T("TextLength   : <0d>\r\n"), (long)GetFlowLayout()->GetContentTextLength());
    WriteHelp(g_f, _T("Bottom margin : <0d>\r\n"), (long)_yBottomMargin);

    for(long iLine = 0; iLine < nLines; iLine++)
    {
        pLine = Elem(iLine);

        WriteHelp(g_f, _T("\r\nLine: <0d> - "), (long)iLine);
        WriteHelp(g_f, _T("Cp: <0d> - "), (long)tp.GetCp());

        // Write out all the flags.
        if (pLine->_fHasBulletOrNum)
            WriteString(g_f, _T("HasBulletOrNum - "));
        if (pLine->_fNewList)
            WriteString(g_f, _T("NewList - "));
        if (pLine->_fHasBreak)
            WriteString(g_f, _T("HasBreak - "));
        if (pLine->_fHasEOP)
            WriteString(g_f, _T("HasEOP - "));
        if (pLine->_fFirstInPara)
            WriteString(g_f, _T("FirstInPara - "));
        if (pLine->_fForceNewLine)
            WriteString(g_f, _T("ForceNewLine - "));
        if (pLine->_fHasNestedRunOwner)
            WriteString(g_f, _T("HasNestedRunOwner - "));
        if (pLine->_fDummyLine)
            WriteString(g_f, _T("DummyLine - "));
        if (pLine->_fHidden)
            WriteString(g_f, _T("Hidden - "));
        if (pLine->_fRelative)
            WriteString(g_f, _T("IsRelative - "));
        if (pLine->_fFirstFragInLine)
            WriteString(g_f, _T("FirstFrag - "));
        if (pLine->_fPartOfRelChunk)
            WriteString(g_f, _T("PartOfRelChunk - "));
        if (pLine->_fHasBackground)
            WriteString(g_f, _T("HasBackground - "));

        if(pLine->IsFrame())
        {
            WriteString(g_f, _T("\r\n\tFrame "));
            WriteString(g_f, pLine->_fFrameBeforeText ?
                                _T("(B) ") :
                                _T("(A) "));
        }
        else if (pLine->IsClear())
        {
            WriteString(g_f, _T("\r\n\tClear     "));
        }
        else
        {
            WriteHelp(g_f, _T("\r\n\tcch = <0d>  "), (long)pLine->_cch);
        }

        // Need to cast stuff to (long) when using <d> as Format Specifier for Win16.
        WriteHelp(g_f, _T("y-Offset = <0d>, "), (long)yHeight);
        WriteHelp(g_f, _T("left-margin = <0d>, "), (long)pLine->_xLeftMargin);
        WriteHelp(g_f, _T("right-margin = <0d>, "), (long)pLine->_xRightMargin);
        WriteHelp(g_f, _T("xWhite = <0d>, "), (long)pLine->_xWhite);
        WriteHelp(g_f, _T("cchWhite = <0d> "), (long)pLine->_cchWhite);
        WriteHelp(g_f, _T("overhang = <0d>, "), (long)pLine->_xLineOverhang);
        WriteHelp(g_f, _T("\r\n\tleft  = <0d>, "), (long)pLine->_xLeft);
        WriteHelp(g_f, _T("right = <0d>, "), (long)pLine->_xRight);
        WriteHelp(g_f, _T("line-width = <0d>, "), (long)pLine->_xLineWidth);
        WriteHelp(g_f, _T("width = <0d>, "), (long)pLine->_xWidth);
        WriteHelp(g_f, _T("height = <0d>, "), (long)pLine->_yHeight);

        WriteHelp(g_f, _T("\r\n\tbefore-space = <0d>, "), (long)pLine->_yBeforeSpace);
        WriteHelp(g_f, _T("descent = <0d>, "), (long)pLine->_yDescent);
        WriteHelp(g_f, _T("txt-descent = <0d>, "), (long)pLine->_yTxtDescent);
        WriteHelp(g_f, _T("extent = <0d>, "), (long)pLine->_yExtent);

        if(pLine->_cch)
        {
            WriteString( g_f, _T("\r\n\ttext = '"));
            DumpLineText(g_f, &tp, iLine);
        }

        WriteString( g_f, _T("\'\r\n"));
        
        if (pLine->_fForceNewLine)
            yHeight += pLine->_yHeight;
    }

    if(GetFlowLayout()->_fContainsRelative)
    {
        CRelDispNodeCache * prdnc = GetRelDispNodeCache();

        if(prdnc)
        {
            WriteString( g_f, _T("   -- relative disp node cache --  \r\n"));

            CElement * pElement = NULL;

            for(long i = 0; i < prdnc->Size(); i++)
            {
                CRelDispNode * prdn = (*prdnc)[i];

                WriteString(g_f, _T("\tElement: "));
                WriteString(g_f, (TCHAR *)prdn->_pElement->TagName());
                WriteHelp(g_f, _T(", SN:<0d>,"), prdn->_pElement->SN());

                WriteHelp(g_f, _T("\t\tLine: <0d>, "), prdn->_ili);
                WriteHelp(g_f, _T("cLines: <0d>, "), prdn->_cLines);
                WriteHelp(g_f, _T("yLine: <0d>, "), prdn->_yli);
                WriteHelp(g_f, _T("ptOffset(<0d>, <1d>), "), prdn->_ptOffset.x, prdn->_ptOffset.y);
                WriteHelp(g_f, _T("DispNode: <0x>,\r\n"), prdn->_pDispNode);
            }

            WriteString(g_f, _T("\r\n"));
        }
    }

    CloseDumpFile();
}

void
CDisplay::DumpLineText(HANDLE hFile, CTxtPtr* ptp, long iLine)
{
    CLine * pLine = Elem(iLine);
    
    if(pLine->_cch)
    {
        TCHAR   chBuff [ 100 ];
        long    cchChunk;

        cchChunk = min( pLine->_cch, long( ARRAY_SIZE( chBuff ) ) );

        ptp->GetText( cchChunk, chBuff );

        WriteFormattedString( hFile, chBuff, cchChunk );

        if (pLine->_cch > cchChunk)
        {
            WriteString( hFile, _T("..."));
        }
        ptp->AdvanceCp(pLine->_cch);
    }
}

#ifdef DISPLAY_TREE
#if 0
void
CDisplay::DumpRegionCollection()
{
    CDispRegion *   pRegion;
    int         i;

    WriteString( g_f,
        _T("\r\n------------- Region Collection -------------------------------\r\n" ));

    for(i = 0, pRegion = _aryRegionCollection; i < _aryRegionCollection.Size(); i++, pRegion++)
    {
        WriteHelp(g_f, _T("\tRegion: <0d> Line: <1d>, "), i, pRegion->_iLine);
        WriteHelp(g_f, _T("cp: <0d>, "), pRegion->_cp);
        WriteHelp(g_f, _T("nLines: <0d>, "), pRegion->_nLines);
        WriteHelp(g_f, _T("rc (t: <0d>, "), pRegion->_rc.top);
        WriteHelp(g_f, _T("l: <0d>, "), pRegion->_rc.left);
        WriteHelp(g_f, _T("r: <0d>, "), pRegion->_rc.right);
        WriteHelp(g_f, _T("b: <0d>)\r\n"), pRegion->_rc.bottom);
    }
}
#endif
#endif

#endif


#ifndef DISPLAY_TREE
/*
 *  CDisplay::GetCliVisible(pcpMostVisible)
 *
 *  @mfunc
 *      Get count of visible lines and update _cpMostVisible for PageDown()
 *
 *  @rdesc
 *      count of visible lines
 */
LONG CDisplay::GetCliVisible(
    LONG* pcpMostVisible) const     //@parm Returns cpMostVisible
{
    LONG cli     = 0;                           // Initialize count
    LONG ili     = GetFirstVisibleLine();       // Start with 1st visible line
    LONG yHeight = GetFirstVisibleDY();
    LONG cp;

    for(cp = GetFirstVisibleCp();
        yHeight < _yHeightView && ili < LineCount();
        cli++, ili++)
    {
        const CLine* pli = Elem(ili);
        if(pli->_fForceNewLine)
            yHeight += pli->_yHeight;
        cp += pli->_cch;
    }

    if(pcpMostVisible)
        *pcpMostVisible = cp;

    return cli;
}
#endif

//==================================  Inversion (selection)  ============================



 //+==============================================================================
 //
 // Method: ShowSelected
 //
 // Synopsis: The "Selected-ness" between the two CTreePos's has changed.
 //           We tell the renderer about it - via Invalidate.
 //
 //           We also need to set the TextSelectionNess of any "sites"
 //           on screen
 //
 //-------------------------------------------------------------------------------
#define CACHED_INVAL_RECTS 20

DeclareTag(tagDisplayShowSelected, "Selection", "Selection CDisplay::ShowSelected output")
DeclareTag(tagDisplayShowInval, "Selection", "Selection show inval rects")

VOID CDisplay::ShowSelected(
    CTreePos* ptpStart,
    CTreePos* ptpEnd,
    BOOL fSelected ) 
{
    CFlowLayout * pFlowLayout = GetFlowLayout();
    CElement * pElement = pFlowLayout->ElementContent();
    CStackDataAry < RECT, CACHED_INVAL_RECTS > aryInvalRects(Mt(CDisplayShowSelectedRange_aryInvalRects_pv));

    AssertSz(pFlowLayout->IsInPlace(),
        "CDisplay::ShowSelected() called when not in-place active");

#ifdef NEVER        
    TextSelectSites( ptpStart, ptpEnd, fSelected) ; 
#endif

    int cpClipStart = ptpStart->GetCp( WHEN_DBG(FALSE));
    int cpClipFinish = ptpEnd->GetCp( WHEN_DBG(FALSE));
    Assert( cpClipStart <= cpClipFinish);

    if ( cpClipFinish > cpClipStart) // don't bother with cpClipFinish==cpClipStart
    {
        //
        // marka BUGBUG - sometimes RegionFromElement will leave left-over rectangles in invalidation
        // investigate further ( or check still need this under display tree ).
        //
        cpClipFinish++; 
      
        RegionFromElement( pElement, &aryInvalRects, NULL, NULL, 0 , cpClipStart, cpClipFinish, NULL ); 

        WHEN_DBG( RECT dbgRect = aryInvalRects[0]);
        TraceTag((tagDisplayShowSelected, "cpClipStart=%d, cpClipFinish=%d fSelected:%d", cpClipStart, cpClipFinish, fSelected));
        TraceTag((tagDisplayShowInval,"InvalRect Left:%d Top:%d, Right:%d, Bottom:%d", dbgRect.left, dbgRect.top, dbgRect.right, dbgRect.bottom));

        pFlowLayout->Invalidate(&aryInvalRects[0], INVAL_CHILDWINDOWS, aryInvalRects.Size());           

    }
    
 #ifdef MERGEFUN // rtp
    BOOL       fResult;
    LONG       cpMost;
    RECT       rc;
    RECT       rcView;
    RECT       rcClip;
    LONG       cpLineStart;
    BOOL       fFirstCharacterSelected = FALSE;
    BOOL       fLastCharacterSelected = FALSE;
    CCalcInfo  CI;
    CFlowLayout * pFlowLayout = GetFlowLayout();
    CStackDataAry < RECT, CACHED_INVAL_RECTS > aryInvalRects(Mt(CDisplayShowSelectedRange_aryInvalRects_pv));

    AssertSz(pFlowLayout->IsInPlace(),
        "CDisplay::ShowSelected() called when not in-place active");

    CI.Init(pFlowLayout);

    // nothing to show if the flowlayout is hidden
    if (pFlowLayout->IsDisplayNone() ||
        !WaitForRecalcView(&CI))           // Ensure all visible lines are recalculated
        return FALSE;

    // Short circuit it right away if we nothing to do ...
    if (!cch)
        return TRUE;

    pFlowLayout->GetClientRect((CRect *)&rcView, NULL);       // Get view rectangle
    rcClip = rcView;

#if 0
    // Rotate viewport in case of vertical control
    // ??? CF - This need to be re-worked and stuck in one place...
    if(pFlowLayout->_fVertical)
    {
        SetMapMode(hdc, MM_ANISOTROPIC);
        SetWindowExtEx(hdc, 1, 1, NULL );
        SetViewportExtEx(hdc, -1, 1, NULL);
        SetViewportOrgEx(hdc, _xWidth, 0, NULL);
    }
#endif

    Assert(cp >= GetFirstCp() && cp <= GetLastCp());
    if(cch < 0)                     // Define cpMost, set cp = cpMin,
    {                               //  and cch = |cch|
        cpMost = cp - cch;
        cch    = -cch;
    }
    else
    {
        cpMost = cp;
        cp    -= cch;
    }

    // If normal selection, then select all the sites within
    if (cp < cpMost)
    {
        TextSelectSites (cp, cpMost, fSelected, &rc);
        if (!IsRectEmpty (&rc))
        {
            THR(aryInvalRects.AppendIndirect(&rc));
        }
    }

    {
        BOOL fComplexLine = FALSE;
        CLinePtr   rp(this);
        CRchTxtPtr tp(GetFlowLayoutElement());

        // Take care of the overhangs in case of inversion
        // but be sure not to cross site boundaries!
        {
            CTreePosList *eRuns = &GetPed()->GetList();
            CElementRunPtr erp(eRuns);
            long iRun;

            fFirstCharacterSelected = cp <= GetFirstCp();
            fLastCharacterSelected = cpMost >= (GetLastCp() - 1);

            if (!fFirstCharacterSelected)
            {
                eRuns->RunAndIchFromCp(cp-1, &iRun, NULL);
                erp.SetIRun(iRun);
                erp.AdvanceToNonEmpty();
                if (erp.GetOwningFlowLayout() == pFlowLayout)
                {
                    cp--;
                    cch++;
                }
            }

            if (!fLastCharacterSelected)
            {
                eRuns->RunAndIchFromCp(cpMost+1, &iRun, NULL);
                erp.SetIRun(iRun);
                erp.RetreatToNonEmpty();
                if (erp.GetOwningFlowLayout() == pFlowLayout)
                {
                    cpMost++;
                    cch++;
                }
            }
        }

        // Compute first line to invert and where to start on it
#ifndef DISPLAY_TREE
        if (cp >= GetFirstVisibleCp())
#endif
        {
            POINT pt;
            tp = cp;

            if(PointFromTp(tp, FALSE, pt, &rp, TA_TOP, &CI, &fComplexLine) < 0)
            {
                fResult = FALSE;
                goto Cleanup;
            }
            if(!rp->_fRTL)
                rc.left = fFirstCharacterSelected ? -1 : pt.x;
            else
                rc.right = fFirstCharacterSelected ? rcView.right + 1 : pt.x;
            rc.top = pt.y;
        }
#ifndef DISPLAY_TREE
        else
        {
            cp = GetFirstVisibleCp();
            rp = GetFirstVisibleLine();
            if(!rp->_fRTL)
                rc.left = -1;
            else
                rc.right = rcView.right + 1;
            rc.top = rcView.top + GetFirstVisibleDY();
        }
#endif

        // loop on all lines of range
        while (cp < cpMost && rc.top < rcClip.bottom && rp.IsValid())
        {
            RECT rcLine;
            RECT rcNoClip;

            cpLineStart = cp;
            cp += rp->_cch - rp.RpGetIch();

            if (rp->_fHidden)
            {
                if( !rp.NextLine(TRUE,FALSE) )
                {
                    break;
                }
                continue;
            }

            CLine *pli = rp.CurLine();
            long xOrigin;

            xOrigin = 0;

            GetClipRectForLine(&rcLine, rc.top, xOrigin, pli);

            if (rp->_fRelative || fComplexLine)
            {
                RECT rcRelative = rcLine;

                // NOTE: (a-pauln) Before correcting this for Relative lines, be sure to 
                // account for how this impacts lines with flows in opposite directions. 
                // This is needed because selections can occur in discontiguous chunks.

                // BUGBUG(SujalP): We will paint the whole relatively positioned line
                // even if one of its characters is selected. We of course can do better,
                // like we are doing for normal lines
                ShiftRectByRelativeOffset(rp, &rcRelative);
                if (rcRelative.right > rcRelative.left)
                {
                    THR(aryInvalRects.AppendIndirect(&rcRelative));
                }
            }

            rc.top    = rcLine.top;
            rc.bottom = rcLine.bottom;

            // Save rc into rcNoClip
            rcNoClip.bottom = rc.bottom ;
            rcNoClip.top = rc.top ;

            rc.bottom = min(rc.bottom, rcView.bottom);
            rc.top    = max(rc.top,    rcView.top);

            if (rc.left == -1)
            {
                rc.left = rcLine.left;
            }

            if (cp > cpMost)
            {
                // Last line of range is inverted partially
                POINT pt;
                tp = cpMost;

                if (PointFromTp(tp, FALSE, pt, NULL, TA_TOP, &CI) < 0)
                {
                    fResult = FALSE;
                    goto Cleanup;
                }
                if(!rp->_fRTL)
                    rc.right = pt.x;
                else
                    rc.left = pt.x;
            }
            else
            {
                if(!rp->_fRTL)
                    rc.right = rcLine.right;
                else
                    rc.left = rcLine.left;
            }

            // Save rc into rcNoClip
            if(rc.left < rc.right)
            {
                rcNoClip.left = rc.left ;
                rcNoClip.right = rc.right ;
            }
            else
            {
                // We get in the situation when the flow
                // is opposite of the line.
                // BUGFIX for 14405 and 21778
                rcNoClip.left = rc.right ;
                rcNoClip.right = rc.left ;
                rc.left = rcNoClip.left;
                rc.right = rcNoClip.right;
            }

            rc.left = max(rc.left, rcView.left);
            rc.right = min(rc.right, rcView.right);

            if (!rp->_fRelative && !fComplexLine)
            {
                if (rc.right > rc.left)
                {
                    THR(aryInvalRects.AppendIndirect(&rc));
                }
            }

            rc.left = -1;

            rc.top = rcNoClip.top - rp->GetYTop();
            if (rp->_fForceNewLine)
            {
                rc.top += rp->GetYHeight();
            }

            // BUGBUG (alexgo): fix this use of break
            if( !rp.NextLine(TRUE,FALSE) )
            {
                break;
            }
        }

#if 0
        // Reset viewport in case of vertical control: needs work...
        if(pFlowLayout->_fVertical)
        {
            SetMapMode(hdc, MM_TEXT);
            SetWindowExtEx(hdc, 1, 1, NULL );
            SetViewportExtEx(hdc, 1, 1, NULL);
            SetViewportOrgEx(hdc, 0, 0, NULL);
        }
#endif

        pFlowLayout->Invalidate(&aryInvalRects[0], INVAL_CHILDWINDOWS,
                              aryInvalRects.Size());
    }

    fResult = TRUE;

Cleanup:
    return fResult;

#endif
}


//+-------------------------------------------------------------------------
//
//  Method:     TextSelectSites
//
//  Synopsis:   Marks sites as being text selected/deselected if they fall in the
//              text selection range.
//
//  Arguments:  ptpStart, the Start TreePos
//              ptpEnd, theEnding TreePos
//  Returns:    nothing
//
//--------------------------------------------------------------------------
void
CDisplay::TextSelectSites ( 
    CTreePos * ptpStart, 
    CTreePos* ptpEnd,
    BOOL fSelected  )
{
    CTreePos* pCurTreePos ;
    CTreeNode* pNode;

    CFlowLayout * pFlowLayout = GetFlowLayout();
    CElement    * pElementFL  = pFlowLayout->ElementOwner();
    CLayout *pLayout;
    
    //
    // Enumerate over all the CTreePos's examining layout changes.
    //

    for ( pCurTreePos = ptpStart; 
          pCurTreePos && pCurTreePos != ptpEnd; 
          pCurTreePos = pCurTreePos->NextTreePos()  )
    {   
        if ( ( pCurTreePos) && ( pCurTreePos->IsBeginElementScope() ) )
        {
            pNode = pCurTreePos->Branch();  
            if ( pNode->HasLayout() )
            {
                pLayout = pNode->GetLayout();
                //
                // marka - you cannot do partial text-site selection, hence just set the style on 
                // the whole layout.
                //
                if (pLayout &&
                    (pNode->IsRoot() || !pLayout->IsFlowLayout()) &&
                    pNode->Element() != pElementFL)
                {
                    const CCharFormat *pCF = pNode->GetCharFormat();

                    // Set the site text selected bits appropriately
                    pLayout->SetSiteTextSelection (
                        fSelected,
                        pCF->SwapSelectionColors());
                }
            }
        }
    }


#ifdef MERGEFUN // erunptr & rtp
    CFlowLayout * pFlowLayout = GetFlowLayout();
    CElement    * pElementFL  = pFlowLayout->Element();

    Assert(cpMin  >= GetFirstCp() && cpMin  <= GetLastCp());
    Assert(cpMost >= GetFirstCp() && cpMost <= GetLastCp());

    AssertSz (prc, "Must collect rects") ;
    SetRectEmpty (prc) ;

    if (pFlowLayout->ContainsChildLayout()) // NODETACH BUGBUG: why was fRaw = TRUE?
    {
        CTreePosList *  eRuns = & GetPed()->GetList();
        CElementRunPtr  erp(eRuns);
        LONG            iRun, iStart, iStop, cRun;
        CTreeNode *     pNode;
        CTreeNode *     pRunOwner;
        CLayout *       pLayout;

        eRuns->RunAndIchFromCp (cpMin, &iStart, NULL);
        eRuns->RunAndIchFromCp (cpMost-1, &iStop, NULL);

        erp.SetIRun(iStop);
        erp.AdvanceToNonEmpty();
        iStop = erp.GetIRun();

        erp.SetIRun(iStart);
        erp.AdvanceToNonEmpty();
        iStart = erp.GetIRun();

        Assert(iStart >= pElementFL->GetFirstRun() &&
               iStart <= pElementFL->GetLastRun()  &&
               iStop  >= pElementFL->GetFirstRun() &&
               iStop  <= pElementFL->GetLastRun());

        for (iRun = iStart; iRun <= iStop; iRun += cRun)
        {
            pRunOwner = erp.GetRunOwnerBranch(pFlowLayout);

            if (pRunOwner && pRunOwner->Element() != pElementFL && !pRunOwner->IsPed())
            {
                pNode = pRunOwner;
                cRun  = eRuns->GetElementNumRuns(pRunOwner->Element());

                Assert(iStop >= (iRun + cRun - 1));
            }
            else
            {
                pNode = eRuns->GetBranchAbs (iRun);
                cRun  = 1;
            }

            pLayout = pNode->GetLayout();

            if (pLayout &&
                (pNode->IsPed() || !pLayout->IsFlowLayout()) &&
                pNode->Element() != pElementFL)
            {
                const CCharFormat *pCF = pNode->GetCharFormat();

                // Set the site text selected bits appropriately
                pLayout->SetSiteTextSelection (
                    fSelected,
                    pCF->SwapSelectionColors());

                // Collect the rects of the selected sites which are
                // left or right aligned. We need this to invert such
                // sites, since they do not occur in the normal text
                // stream. For inlined sites (which occur in the text
                // stream), the inversion is done within the draw of
                // of the text, hence we donot need to invert them
                // specially (and therefore donot collect their
                // rects here. (sujalp)
                if (!pNode->IsInlinedElement() || pFlowLayout->_fContainsAbsolute)
                {
                    CRect   rc;

                    pLayout->GetRect(&rc, COORDSYS_GLOBAL);
                    UnionRect(prc, prc, &rc);
                }
            }

            if (erp.GetIRun() + cRun > iStop)
            {
                break;
            }
            else if (pRunOwner->Element() != pElementFL)
            {
                erp.SetIRun(erp.GetIRun() + cRun);
            }
            else
            {
                erp.MoveToNextRun();
            }
        }
    }
#endif
}


//+----------------------------------------------------------------------------
//
// Member:      CDisplay::GetViewHeightAndWidthForChild
//
// Synopsis:    returns the view width/height - padding
//
//-----------------------------------------------------------------------------
void
CDisplay::GetViewWidthAndHeightForChild(
    CParentInfo *   ppri,
    long *          pxWidthParent,
    long *          pyHeightParent,
    BOOL            fMinMax) const
{
    long lPadding[PADDING_MAX];

    Assert(pxWidthParent && pyHeightParent);

    GetPadding(ppri, lPadding, fMinMax);

    *pxWidthParent  = GetViewWidth() -
                        lPadding[PADDING_LEFT] -
                        lPadding[PADDING_RIGHT] -
                        GetCaret();
    *pyHeightParent = GetViewHeight() -
                        lPadding[PADDING_TOP] -
                        lPadding[PADDING_BOTTOM];
}

//+----------------------------------------------------------------------------
//
// Member:      CDisplay::GetPadding
//
// Synopsis:    returns the top/left/right/bottom padding of the current
//              flowlayout
//
//-----------------------------------------------------------------------------
void
CDisplay::GetPadding(
    CParentInfo *   ppri,
    long            lPadding[],
    BOOL            fMinMax) const
{
    CElement  * pElementFL   = GetFlowLayoutElement();
    CTreeNode * pNode        = pElementFL->GetFirstBranch();
    CDoc      * pDoc         = pElementFL->Doc();
    ELEMENT_TAG etag         = pElementFL->Tag();
    LONG        lFontHeight  = pNode->GetCharFormat()->GetHeightInTwips(pDoc);
    long        lParentWidth = fMinMax
                                ? ppri->_sizeParent.cx
                                : GetViewWidth();

    const CFancyFormat * pFF = pNode->GetFancyFormat();


    if( etag == ETAG_MARQUEE && !pElementFL->IsEditable() && !_fPrinting    )
    {
        lPadding[PADDING_TOP]    = 
        lPadding[PADDING_BOTTOM] = DYNCAST(CMarquee, pElementFL)->_lYMargin;
    }
    else
    {
        lPadding[PADDING_TOP]    = 
        lPadding[PADDING_BOTTOM] = 0;
    }

    lPadding[PADDING_TOP] +=
        pFF->_cuvSpaceBefore.YGetPixelValue(ppri, lParentWidth, lFontHeight) +
        pFF->_cuvPaddingTop.YGetPixelValue(ppri, lParentWidth, lFontHeight);
                            // (srinib) parent width is intentional as per css spec

    lPadding[PADDING_BOTTOM] +=
        pFF->_cuvSpaceAfter.YGetPixelValue(ppri, lParentWidth, lFontHeight) +
        pFF->_cuvPaddingBottom.YGetPixelValue(ppri, lParentWidth, lFontHeight);
                            // (srinib) parent width is intentional as per css spec

    lPadding[PADDING_LEFT] =
        pFF->_cuvPaddingLeft.XGetPixelValue(ppri, lParentWidth, lFontHeight);


    lPadding[PADDING_RIGHT] =
        pFF->_cuvPaddingRight.XGetPixelValue(ppri, lParentWidth, lFontHeight);

    if (etag == ETAG_BODY)
    {
        lPadding[PADDING_LEFT]  +=
            pFF->_cuvMarginLeft.XGetPixelValue(ppri, lParentWidth, lFontHeight);
        lPadding[PADDING_RIGHT] +=
            pFF->_cuvMarginRight.XGetPixelValue(ppri, lParentWidth, lFontHeight);
    }

    if(lPadding[PADDING_TOP] > SHRT_MAX)
        lPadding[PADDING_TOP] = SHRT_MAX;

    if(lPadding[PADDING_BOTTOM] > SHRT_MAX)
        lPadding[PADDING_BOTTOM] = SHRT_MAX;

    if(lPadding[PADDING_LEFT] > SHRT_MAX)
        lPadding[PADDING_LEFT] = SHRT_MAX;

    if(lPadding[PADDING_RIGHT] > SHRT_MAX)
        lPadding[PADDING_RIGHT] = SHRT_MAX;
}


//+----------------------------------------------------------------------------
//
// Member:      GetRectForChar
//
// Synopsis:    Returns the top, the bottom and the width for a character
//
//  Notes:      If pWidth is not NULL, the witdh of ch is returned in it.
//              Otherwise, ch is ignored.
//-----------------------------------------------------------------------------

void
CDisplay::GetRectForChar(
                    CCalcInfo  *pci,
                    LONG       *pTop,
                    LONG       *pBottom,
                    LONG       *pWidth,
                    WCHAR       ch,
                    LONG        yTop,
                    CLine      *pli,
              const CCharFormat*pcf)
{
    CCcs *              pccs    = fc().GetCcs (pci->_hdc, pci, pcf);

    Assert(pci && pTop && pBottom && pli && pcf);
    Assert (pccs);

    if (pWidth)
    {
        pccs->Include(ch, *pWidth);

        // Account for letter spacing (#18676)
        LONG        xLetterSpacing;
        CUnitValue  cuvLetterSpacing = pcf->_cuvLetterSpacing;

        switch (cuvLetterSpacing.GetUnitType())
        {
        case CUnitValue::UNIT_INTEGER:
            xLetterSpacing = (int)cuvLetterSpacing.GetUnitValue();
            break;

        case CUnitValue::UNIT_ENUM:
            xLetterSpacing = 0;     // the only allowable enum value for l-s is normal=0
            break;

        default:
            xLetterSpacing = (int)cuvLetterSpacing.XGetPixelValue(pci, 0,
                GetFlowLayout()->GetFirstBranch()->GetFontHeightInTwips(&cuvLetterSpacing));
        }
        *pWidth += xLetterSpacing;
    }

    *pBottom = yTop + pli->_yHeight - pli->_yDescent + pli->_yTxtDescent;
    *pTop = *pBottom - pccs->_yHeight - pccs->_yOffset -
            (pli->_yTxtDescent - pccs->_yDescent);
    
    pccs->Release();
}


//+----------------------------------------------------------------------------
//
// Member:      GetTopBottomForCharEx
//
// Synopsis:    Returns the top and the bottom for a character
//
// Params:  [pDI]:     The DI
//          [pTop]:    The top is returned in this
//          [pBottom]: The bottom is returned in this
//          [yTop]:    The top of the line containing the character
//          [pli]:     The line itself
//          [xPos]:    The xPos at which we need the top/bottom
//          [pcf]:     Character format for the char
//
//-----------------------------------------------------------------------------
void
CDisplay::GetTopBottomForCharEx(
                             CCalcInfo     *pci,
                             LONG          *pTop,
                             LONG          *pBottom,
                             LONG           yTop,
                             CLine         *pli,
                             LONG           xPos,
                       const CCharFormat    *pcf)
{
    //
    // If we are on a line in a list, and we are on the area occupied
    // (horizontally) by the bullet, then we want to find the height
    // of the bullet.
    //

    if (    pli->_fHasBulletOrNum
        &&  (   (   xPos >= pli->_xLeftMargin
                &&  xPos < pli->GetTextLeft()))
       )
    {
        Assert(pci && pTop && pBottom && pli);

        *pBottom = yTop + pli->_yHeight - pli->_yDescent;
        *pTop = *pBottom - pli->_yBulletHeight;
    }
    else
    {
        GetRectForChar(pci, pTop, pBottom, NULL, 0, yTop, pli, pcf);
    }
}


//+----------------------------------------------------------------------------
//
// Member:      GetClipRectForLine
//
// Synopsis:    Returns the clip rect for a given line
//
// Params:  [prcClip]: Returns the rect for the line
//          [pTop]:    The top is returned in this
//          [pBottom]: The bottom is returned in this
//          [yTop]:    The top of the line containing the character
//          [pli]:     The line itself
//          [pcf]:     Character format for the char
//
//-----------------------------------------------------------------------------
void
CDisplay::GetClipRectForLine(RECT *prcClip, LONG top, LONG xOrigin, CLine *pli) const
{
    Assert(prcClip && pli);

    if(!pli->_fRTL)
    {
        prcClip->left   = xOrigin + pli->GetTextLeft();
        prcClip->right  = xOrigin + pli->GetTextRight() + (pli->_fForceNewLine ? pli->_xWhite : 0);
    }
    else
    {
        prcClip->right  = xOrigin - pli->GetRTLTextRight();
        prcClip->left   = xOrigin - pli->GetRTLTextLeft() - (pli->_fForceNewLine ? pli->_xWhite : 0);
    }
    prcClip->top    = top + pli->GetYTop();
    prcClip->bottom = top + pli->GetYBottom();
}

//=============================  Vertical control support  ================================


/*
 *  CDisplay::ConvSetRect(prc)
 *
 *  Purpose:
 *      Converts a rectangle for a vertical control before passing
 *      it to a Window API
 *
 *  Arguments:
 *      prc     Rectangle to convert
 */
VOID CDisplay::ConvSetRect(LPRECT prc)
{
    RECT rc;

    if(GetFlowLayout()->_fVertical)
    {
        rc.left     = prc->top;
        rc.top      = prc->left;
        rc.right    = prc->bottom;
        rc.bottom   = prc->right;

        LONG xWidth = GetWidth();

        prc->left   = xWidth - rc.right;
        prc->top    = rc.top;
        prc->right  = xWidth - rc.left;
        prc->bottom = rc.bottom;

//      TraceTag(tagReVertical, "Width of Client is %u", _xWidth);
    }
}

/*
 *  CDisplay::ConvGetRect(prc)
 *
 *  Purpose:
 *      Converts a rectangle obtained from Windows for a vertical control
 *
 *  Arguments:
 *      prc     Rectangle to convert
 */
VOID CDisplay::ConvGetRect(LPRECT prc)
{
    RECT    rc;
    INT     xWidth;
    INT     yHeight;

    if(GetFlowLayout()->_fVertical)
    {
        rc          = *prc;
        xWidth      = rc.right - rc.left;
        yHeight     = rc.bottom - rc.top;
        prc->left   = rc.top;
        prc->top    = rc.left;
        prc->right  = rc.top + yHeight;
        prc->bottom = rc.left + xWidth;
    }
}

//=================================  IME support  ===================================

#ifdef DBCS

/*
 *  ConvGetRect
 *
 *  Purpose:
 *      Converts a rectangle obtained from Windows for a vertical control
 *
 *  Arguments:
 *      prc     Rectangle to convert
 */
void ConvGetRect(LPRECT prc, BOOL fVertical)
{
    RECT    rc;
    INT     xWidth;
    INT     yHeight;

    if(fVertical)
    {
        rc          = *prc;
        xWidth      = rc.right - rc.left;
        yHeight     = rc.bottom - rc.top;
        prc->left   = rc.top;
        prc->top    = rc.left;
        prc->right  = rc.top + yHeight;
        prc->bottom = rc.left + xWidth;
    }
}

/*
 *  VwUpdateIMEWindow(ped)
 *
 *  Purpose:
 *      Update position of IME candidate/composition string
 *
 *  Arguments:
 */
VOID VwUpdateIMEWindow(CPED ped)
{
    POINTL  pt;
    RECT    rc;
    SIZE    size;

    if((ped->_dwStyle & ES_NOIME) && (ped->_dwStyle & ES_SELFIME))
        return;

    ConvGetRect(ped->_fVertical, &rc);


    rc.left     = _pdp->GetViewLeft();
    rc.top      = _pdp->GetViewTop();
    rc.right    = _pdp->GetViewWidth() + _pdp->GetViewLeft();
    rc.bottom   = _pdp->GetViewHeight() + _pdp->GetViewTop();

    size.cy     = ped->_yHeightCaret;
    size.cx     = ped->_yHeightCaret;

    if(ped->_fVertical)
    {
        ConvSetRect(&rc);
        pt.y    = (INT)ped->_xCaret - size.cx;
        pt.x    = ped->_xWidth - (INT)ped->_yCaret;
    }
    else
    {
        pt.x    = (INT)ped->_xCaret;
        pt.y    = (INT)ped->_yCaret;
    }

    SetIMECandidatePos (ped->_hwnd, pt, (LPRECT)&rc, &size);
}
#endif

//+----------------------------------------------------------------------------
//
// Member:      ComputeVertPaddAndBordForParentNode
//
// Synopsis:    Computes the vertical padding and border for a given
//              element or range.
//
//-----------------------------------------------------------------------------

void
CDisplay::ComputeVertPaddAndBordFromParentNode(
            CCalcInfo * pci,
            CTreePos * ptpStart, CTreePos * ptpFinish,
            LONG * pyPaddBordTop, LONG * pyPaddBordBottom)
{
    CFlowLayout * pFlowLayout = GetFlowLayout();
    CElement    * pElementFL  = pFlowLayout->ElementOwner();
    CTreePos *    ptpCurr;

    Assert(pci);
    Assert(ptpStart && ptpFinish);

    *pyPaddBordTop = 0;
    *pyPaddBordBottom = 0;

    ptpCurr = ptpStart->PreviousTreePos();

    while(    ptpCurr
          &&  !ptpCurr->IsText()
          &&  !ptpCurr->IsEndNode() )
    {
        if( ptpCurr->IsBeginNode() )
        {
            CTreeNode * pNode = ptpCurr->Branch();
            CElement  * pElement = pNode->Element();

            if(     ! ptpCurr->IsEdgeScope()
               ||   pElement == pElementFL )
               break;

            // add up padding for ptpCurr->Branch()
            if(pElement->IsBlockElement())
            {
                CBorderInfo borderinfo;

                if ( !pElement->_fDefinitelyNoBorders )
                {
                    pElement->_fDefinitelyNoBorders =
                        !GetBorderInfoHelper( pNode, pci, &borderinfo, FALSE );

                   *pyPaddBordTop += borderinfo.aiWidths[BORDER_TOP];
                }

                // Note: _sizeParent.cx is to be used for percent base padding top
                // and bottom as per spec(seems weird though). Please talk to
                // cwilso about this.
                *pyPaddBordTop += pNode->GetFancyFormat()->_cuvPaddingTop.
                                    YGetPixelValue(pci, pci->_sizeParent.cx, 1);
            }
        }

        ptpCurr = ptpCurr->PreviousTreePos();
    }


    // BUGBUG: there might be some overlapping weirdness to be handled here...
    ptpCurr = ptpFinish;
    if( ptpCurr->GetBranch()->Element() != pElementFL )
    {
        ptpCurr = ptpCurr->NextTreePos();

        while(    ptpCurr
              &&  !ptpCurr->IsText()
              &&  !(ptpCurr->IsBeginNode() && ptpCurr->IsEdgeScope()) )
        {
            if( ptpCurr->IsEndNode() && ptpCurr->IsEdgeScope() )
            {
                CTreeNode * pNode = ptpCurr->Branch();
                CElement  * pElement = pNode->Element();

                if( pElement == pElementFL )
                    break;

                // add up padding for ptpCurr->Branch()
                if(pElement->IsBlockElement())
                {
                    CBorderInfo borderinfo;

                    if ( !pElement->_fDefinitelyNoBorders )
                    {
                        pElement->_fDefinitelyNoBorders =
                            !GetBorderInfoHelper( pNode, pci, &borderinfo, FALSE );

                        *pyPaddBordBottom += borderinfo.aiWidths[BORDER_BOTTOM];
                    }

                    // (srinib) Note: _sizeParent.cx is to be used for percent base padding top
                    // and bottom as per spec(seems weird though). Please talk to cwilso
                    // about this if you need to change this
                    *pyPaddBordBottom += pNode->GetFancyFormat()->_cuvPaddingBottom.
                                            YGetPixelValue(pci, pci->_sizeParent.cx, 1);
                }
            }
            ptpCurr = ptpCurr->NextTreePos();
        }
    }
}


HRESULT
CDisplay::GetWigglyFromLineRange(CCalcInfo * pCI, long cpStart, long cch, CWigglyShape * pShape)
{
    HRESULT         hr = S_FALSE;
    CLinePtr        rp(this);
    CRect           rc, rcOld;
    LONG            ili;
    LONG            cp;
    POINT           pt;
    long            cpEnd = cpStart + cch;
    WIGGLYOFFSET    wigglyOffset;
    CWiggly *       pWiggly;
    BOOL            fStartWiggly;
    CMarkup *       pMarkup = GetMarkup();
    CTxtPtr         tp(pMarkup, cpStart);
    
    Assert(cpStart >= GetFirstCp() && cpStart <= GetLastCp());
    Assert(cpEnd   >= GetFirstCp() && cpEnd   <= GetLastCp());

    pWiggly = new CWiggly;
    if (!pWiggly)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    pShape->_aryWiggly.Append(pWiggly);

    fStartWiggly = TRUE;

    // BUGBUG (a-pauln) This gives us incorrect measurements for any complex text
    // We need to look into at least splitting complex text to use line services
    // for proper measurement. This may be a perf hit, but current results are fair to poor.
    // It would be possible to call CLSMeasurer::MeasureText like is done above
    // in PointFromTp().

    for (cp = cpStart; cp < cpEnd; cp++, tp.AdvanceCp(1))
    {
        long        ich;
        CTreePos *  ptp      = pMarkup->TreePosAtCp(cp, &ich);
        CTreeNode * pNode    = ptp->GetBranch();
        CElement *  pElement = pNode->Element();

        if (    ptp->IsNode()
            &&  !pElement->HasLayout())
            continue;

        if (ptp->IsEndNode())
            pNode = pNode->Parent();

        ili = RenderedPointFromTp(cp, ptp, FALSE, pt, &rp, TA_TOP, pCI);
        if (ili < 0)
        {
            hr = S_FALSE;
            goto Cleanup;
        }

        if (pElement->HasLayout())
        {
            CLayout *   pLayout;

            pLayout = pElement->GetLayout();
            Assert(pLayout);

            pLayout->GetRect(&rc);

            // Non-inlined element needs its own separate rect
            if (!pElement->IsInlinedElement())
            {
                CWiggly *       pWigglyAligned = new CWiggly;
                WIGGLYOFFSET    woAligned;

                if (!pWigglyAligned)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                pShape->_aryWiggly.Append(pWigglyAligned);

                pWigglyAligned->_ptTopLeft = rc.TopLeft();

                woAligned.top    = 0;
                woAligned.bottom = rc.Height();
                woAligned.right  = rc.Width();

                pWigglyAligned->_aryOffset.AppendIndirect(&woAligned);

                // Do not update top & bottom here
                continue;
            }
        }
        else
        {
            SIZECOORD       width;

            if(!rp->_fRTL)
            {
                rc.left = pt.x;
            }
            else
            {
                rc.right = pt.x;
            }


            GetRectForChar(pCI, &rc.top, &rc.bottom, &width, tp.GetChar(), pt.y, rp.CurLine(), pNode->GetCharFormat());
            if (width <= 0)
                break;

            if(!rp->_fRTL)
            {
                rc.right = rc.left + width;
            }
            else
            {
                rc.left = rc.right - width;
            }
        }

        if (fStartWiggly)
        {
            fStartWiggly = FALSE;

            pWiggly->_ptTopLeft = CPoint(rc.left, rc.top);

            rcOld.top    =
            rcOld.bottom = rc.top;

            if(!rp->_fRTL)
                rcOld.right = rc.left; 
            else
                rcOld.left = rc.right; 
        }


        wigglyOffset.top    = rc.top - rcOld.top;
        wigglyOffset.bottom = rc.bottom - rcOld.bottom;
        if(!rp->_fRTL)
        {
            wigglyOffset.right = rc.right - rcOld.right;
        }
        else
        {
            wigglyOffset.right = rcOld.left - rc.left;
            // we need to keep shifting the wiggly's left down the line
            // because we are going right to left.
            pWiggly->_ptTopLeft.x = rc.left;
        }
        pWiggly->_aryOffset.AppendIndirect(&wigglyOffset);

        rcOld = rc;
    }

    hr = S_OK;

Cleanup:
    RRETURN1(hr, S_FALSE);
}


HRESULT
CDisplay::GetWigglyFromRange(CDocInfo * pdci, long cp, long cch, CShape ** ppShape)
{
    HRESULT         hr = S_FALSE;
    CCalcInfo       CI;
#ifndef DISPLAY_TREE
    long            cchLine, cpVisible;
#else
    long            cchLine;
#endif
    CWigglyShape *  pShape = NULL;
    CLinePtr        rp(this);
    long            xWidthParent;
    long            yHeightParent;

    if (!cch)
    {
        goto Cleanup;
    }

    CI.Init(pdci, GetFlowLayout());

    GetViewWidthAndHeightForChild(&CI, &xWidthParent, &yHeightParent);

    CI.SizeToParent(xWidthParent, yHeightParent);

    Assert(cp >= GetFirstCp() && cp <= GetLastCp());

    // Compute first line to invert and where to start on it
#ifndef DISPLAY_TREE
    cpVisible = GetFirstVisibleCp();
    if (cp >= cpVisible)
    {
#endif
        POINT   pt;

        if(PointFromTp(cp, NULL, FALSE, pt, &rp, TA_TOP, &CI) < 0)
            goto Cleanup;
#ifndef DISPLAY_TREE
    }
    else
    {
        if (cp + cch <= cpVisible)
            goto Cleanup;

        cch -= (cpVisible - cp);
        cp = cpVisible;
        rp = GetFirstVisibleLine();
    }
#endif

    pShape = new CWigglyShape;
    if (!pShape)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // loop on all lines of the range
    while (cch > 0 && rp.IsValid())
    {
        cchLine = min(cch, rp->_cch - rp.RpGetIch());

        if (!rp->_fHidden)
        {
            hr = THR(GetWigglyFromLineRange(&CI, cp, cchLine, pShape));
            if (hr)
                goto Cleanup;
            cch -= cchLine;
            cp += cchLine;
        }

        if (!rp.NextLine(TRUE, FALSE))
            break;
    }

    *ppShape = pShape;
    hr = S_OK;

Cleanup:
    if (hr && pShape)
    {
        delete pShape;
    }

    RRETURN1(hr, S_FALSE);
}

