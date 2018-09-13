//+----------------------------------------------------------------------------
// File: scroll.cxx
//
// Description: Utility function on CDisplay
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

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPRGN_HXX_
#define X_DISPRGN_HXX_
#include "disprgn.hxx"
#endif

#ifndef X_EPHRASE_HXX_
#define X_EPHRASE_HXX_
#include "ephrase.hxx"
#endif


#pragma warning(disable:4706) /* assignment within conditional expression */

MtDefine(CDisplayShowSelectedRange_aryInvalRects_pv, Locals, "CDisplay::ShowSelectedRange aryInvalRects::_pv");
MtDefine(CDisplayRegionFromElement_aryChunks_pv, Locals, "CDisplay::RegionFromElement::aryChunks_pv");
MtDefine(CDisplayGetWigglyFromRange_aryWigglyRect_pv, Locals, "CDisplay::GetWigglyFromRange::aryWigglyRect_pv");

DeclareTag(tagNotifyLines, "NotifyLines", "Fix-up a line cch from Notify");

// ================================  Line info retrieval  ====================================

/*
 *  CDisplay::YposFromLine(ili)
 *
 *  @mfunc
 *      Computes top of line position
 *
 *  @rdesc
 *      top position of given line (relative to the first line)
 *      Computed by accumulating CLine _yHeight's for each line with
 *      _fForceNewLine true.
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

    yPos = 0;
    for (long i=0; i < ili; i++)
    {
        pli = Elem(i);
        if (pli->_fForceNewLine)
        {
            yPos += pli->_yHeight;
        }
    }

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
    LONG ili,               // Line we're interested in (if <lt> 0 means caret line)
    LONG *pyHeight) const   // Returns top of line relative to display
{
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
    CFlowLayout *   pFlowLayout    = GetFlowLayout();
    CElement *      pElementFL     = pFlowLayout->ElementOwner();
    BOOL            fIsDirty       = pFlowLayout->IsDirty();
    long            cpFirst        = pFlowLayout->GetContentFirstCp();
    long            cpDirty        = cpFirst + pFlowLayout->Cp();
    long            cchNew         = pFlowLayout->CchNew();
    long            cchOld         = pFlowLayout->CchOld();
    long            cpMax          = _dcpCalcMax;
    long            cchDelta       = pnf->CchChanged();
#if DBG==1
    long            dcpLastCalcMax = _dcpCalcMax;
    long            iLine          = -1;
    long            cchLine        = -1;
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

    if (    fIsDirty
        &&  pFlowLayout->Cp() < cpMax)
    {
        if (pFlowLayout->Cp() + cchOld >= cpMax)
        {
            cpMax = pFlowLayout->Cp();
        }
        else
        {
            cpMax += cchNew - cchOld;
        }
    }

    //
    // If the change is past the end of the line array, exit
    //

    if ((pnf->Cp(cpFirst) - cpFirst) > cpMax)
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

    if (    cchDelta
        &&  (   !fIsDirty
            ||  pnf->Cp(cpFirst) <  cpDirty
            ||  pnf->Cp(cpFirst) >= cpDirty + cchNew))
    {
        CLinePtr    rp(this);
        long        cchCurrent  = pElementFL->GetElementCch();
        long        cchPrevious = cchCurrent - (fIsDirty
                                                    ? cchNew - cchOld
                                                    : 0);
        long        cpPrevious  = !fIsDirty || pnf->Cp(cpFirst) < cpDirty
                                        ? pnf->Cp(cpFirst)
                                        : pnf->Cp(cpFirst) + (cchOld - cchNew);

        //
        // Adjust the maximum calculated cp
        // (Sanitize the value as well - invalid values can result when handling notifications from
        //  elements that extend outside the layout)
        //

        _dcpCalcMax += cchDelta;
        if (_dcpCalcMax < 0)
        {
            _dcpCalcMax = 0;
        }
        else if (_dcpCalcMax > cchPrevious)
        {
            _dcpCalcMax = cchPrevious;
        }

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
        rp.RpSetCp(cpPrevious, FALSE, FALSE);

        if (cchDelta > 0)
        {
            //
            // We adding at the end of the site?
            //
            if (pnf->Handler()->GetLastCp() == pnf->Cp(cpFirst) + cchDelta)
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
        Assert(cchDelta > 0 || !rp.GetLineIndex() || ( rp->_cch + cchDelta ) >= 0);
        WHEN_DBG(iLine = rp.GetLineIndex());
        WHEN_DBG(cchLine = rp->_cch);

        //
        // Update the number of characters in the line
        // (Sanitize the character count if the delta is too large - this can happen for
        //  notifications from elements that extend outside the layout)
        //

        rp->_cch += cchDelta;
        if (!rp.GetLineIndex())
        {
            rp->_cch = max(rp->_cch, 0L);
        }
        else if (rp.IsLastTextLine())
        {
            rp->_cch = min(rp->_cch, rp.RpGetIch() + (cchCurrent - (cpPrevious - cpFirst)));
        }
    }

Cleanup:
#if DBG==1
    if (iLine >= 0)
    {
        TraceTagEx((tagNotifyLines, TAG_NONAME,
                    "NotifyLine: (%d) Element(0x%x,%S) ChldNd(%d) dcp(%d) cchDelta(%d) line(%d) cch(%d,%d) dcpCalcMax(%d,%d)",
                    pnf->SerialNumber(),
                    pFlowLayout->ElementOwner(),
                    pFlowLayout->ElementOwner()->TagName(),
                    pnf->Node()->_nSerialNumber,
                    pnf->Cp(cpFirst) - cpFirst,
                    cchDelta,
                    iLine,
                    cchLine, max(0L, cchLine + cchDelta),
                    dcpLastCalcMax, _dcpCalcMax));
    }
    else
    {
        TraceTagEx((tagNotifyLines, TAG_NONAME,
                    "NotifyLine: (%d) Element(0x%x,%S) dcp(%d) dcpCalcMax(%d) delta(%d) IGNORED",
                    pnf->SerialNumber(),
                    pFlowLayout->ElementOwner(),
                    pFlowLayout->ElementOwner()->TagName(),
                    pnf->Cp(cpFirst) - cpFirst,
                    _dcpCalcMax,
                    cchDelta));
    }
#endif
    return;
}

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
    DWORD           grfFlags,
    LONG            iliStart,
    LONG            iliFinish) const
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
    if (myRc.bottom < 0)
        myRc.bottom = 0;
    
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

    if (iliStart < 0)
        iliStart = 0;

    if (iliFinish < 0)
        iliFinish = LineCount();

    Assert(iliStart < iliFinish && iliFinish <= LineCount());

    ili  = iliStart;
    pli  = Elem(ili);;

    if (iliStart > 0)
    {
        yli  = -pli->GetYTop();
        cpli = CpFromLine(ili);
    }
    else
    {
        yli  = 0;
        cpli = GetFirstCp();
    }

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

    while ( ili < iliFinish
        &&  yIntersect >= yli + _yMostNeg)
    {
        pli = Elem(ili);

        //
        //  Skip over lines that should be ignored
        //  These include:
        //      1. Lines that do not intersect
        //      2. Hidden and dummy lines
        //      3. Relative lines (when requested)
        //      4. Line chunks that do not intersect the x-offset
        //      5. Its an aligned line and we have been asked to skip aligned lines
        //
        
        if (    yIntersect >= yli + pli->GetYLineTop()
            &&  yIntersect < yli + pli->GetYLineBottom()
            &&  !pli->_fHidden
            &&  (   !pli->_fDummyLine
                ||  !fInBrowse)
            &&  (   !pli->_fRelative
                ||  !(grfFlags & LFP_IGNORERELATIVE))
            &&  (   pli->_fForceNewLine
                ||  (!pli->_fRTL
                            ? myRc.left <= pli->GetTextRight(ili == LineCount() - 1)
                            : myRc.left >= pli->GetRTLTextLeft()))
            &&  (   (!pli->IsFrame()
                ||   !(grfFlags & LFP_IGNOREALIGNED)))
           )

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
                                                ||  myRc.left > pli->GetTextRight(ili == LineCount() -1))
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

    //
    // if we are lookig for an exact line hit and
    // do not have a candidate line, it's a miss
    //
    if (iliCandidate < 0 && grfFlags & LFP_EXACTLINEHIT)
    {
        return -1;
    }

    Assert(ili <= LineCount());


    //
    // we better have a candidate, if yIntersect < yli + _yMostNeg
    //
    Assert( iliCandidate >= 0 || yIntersect >= yli + _yMostNeg || (grfFlags & LFP_IGNORERELATIVE));

    //
    //  No intersecting line was found, take either the candidate or last line
    //
    
    //
    //  ili == LineCount() - is TRUE only if the point we are looking for is
    //  below all the content or we found a candidate line but are performing
    //  a Z-Order search on a layout with lines with negative margin.
    //
    if (    ili == iliFinish

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
    POINT       pt,               // Point to compute cp at (client coords)
    CLinePtr  * const prp,         // Returns line pointer at cp (may be NULL)
    CTreePos ** pptp,             // pointer to return TreePos corresponding to the cp
    DWORD       dwCFPFlags,       // flags to CpFromPoint
    BOOL      * pfRightOfCp,
    LONG      * pcchPreChars)
{
    CMessage msg;
    HTC htc;
    CTreeNode * pNodeElementTemp;
    DWORD dwFlags = HT_SKIPSITES | HT_VIRTUALHITTEST | HT_IGNORESCROLL;
    LONG cpHit;
    CFlowLayout *pFlowLayout = GetFlowLayout();
    CTreeNode * pNode = pFlowLayout->GetFirstBranch();
    CRect rcClient;

    Assert(pNode);

    CElement  * pContainer = pNode->GetContainer();

    pFlowLayout->GetContentRect(&rcClient, COORDSYS_GLOBAL);

    if (pt.x < rcClient.left)
        pt.x = rcClient.left;
    if (pt.x >= rcClient.right)
        pt.x = rcClient.right - 1;
    if (pt.y < rcClient.top)
        pt.y = rcClient.top;
    if (pt.y >= rcClient.bottom)
        pt.y = rcClient.bottom - 1;
    
    msg.pt = pt;

    if (dwCFPFlags & CFP_ALLOWEOL)
        dwFlags |= HT_ALLOWEOL;
    if (!(dwCFPFlags & CFP_IGNOREBEFOREAFTERSPACE))
        dwFlags |= HT_DONTIGNOREBEFOREAFTER;
    if (!(dwCFPFlags & CFP_EXACTFIT))
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
        &&  (   pNodeElementTemp->IsContainer()
             && pNodeElementTemp->GetContainer() != pContainer
            )
       )
    {
        htc= HTC_NO;
    }

    if (htc >= HTC_YES && msg.resultsHitTest._cpHit >= 0)
    {
        cpHit = msg.resultsHitTest._cpHit;
        if (prp)
            prp->RpSet(msg.resultsHitTest._iliHit, msg.resultsHitTest._ichHit);
        if (pfRightOfCp)
            *pfRightOfCp = msg.resultsHitTest._fRightOfCp;
        if (pcchPreChars)
            *pcchPreChars = msg.resultsHitTest._cchPreChars;
    }
    else
    {
        CPoint ptLocal(pt);
        //
        // We now need to convert pt to client coordinates from global coordinates
        // before we can call CpFromPoint...
        //
        pFlowLayout->TransformPoint(&ptLocal, COORDSYS_GLOBAL, COORDSYS_CONTENT);
        cpHit = CpFromPoint(ptLocal, prp, pptp, NULL, dwCFPFlags,
                            pfRightOfCp, NULL, pcchPreChars, NULL);
    }
    return cpHit;
}

LONG
CDisplay::CpFromPoint(
    POINT       pt,                     // Point to compute cp at (site coords)
    CLinePtr  * const prp,         // Returns line pointer at cp (may be NULL)
    CTreePos ** pptp,             // pointer to return TreePos corresponding to the cp
    CLayout  ** ppLayout,          // can be NULL
    DWORD       dwFlags,
    BOOL      * pfRightOfCp,
    BOOL      * pfPseudoHit,
    LONG      * pcchPreChars,
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
                                            ? LFP_IGNOREALIGNED
                                            : 0) |
                                        (dwFlags & CFP_NOPSEUDOHIT
                                            ? LFP_EXACTLINEHIT
                                            : 0));
    if(ili < 0)
        return -1;
        
                         
    return CpFromPointEx(ili, yLine, cp, pt, prp, pptp, ppLayout,
                         dwFlags, pfRightOfCp, pfPseudoHit,
                         pcchPreChars, pci);
                        
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
    DWORD      dwFlags,
    BOOL      *pfRightOfCp,
    BOOL      *pfPseudoHit,
    LONG      *pcchPreChars,
    CCalcInfo *pci)
{
    CFlowLayout *pFlowLayout = GetFlowLayout();
    CElement    *pElementFL  = pFlowLayout->ElementOwner();
    CCalcInfo    CI;
    CLine       *pli = Elem(ili);
    LONG         cch = 0;
    LONG         dx = 0;
    BOOL         fPseudoHit = FALSE;
    CTreePos    *ptp = NULL;
    CTreeNode   *pNode;
    LONG         cchPreChars = 0;
    
    if (!pci)
    {
        CI.Init(pFlowLayout);
        pci = &CI;
    }

    if (   dwFlags & CFP_IGNOREBEFOREAFTERSPACE
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
                *ppLayout = pli->_pNodeLayout->GetUpdatedLayout();
            }
            cch = 0;

            if(pfRightOfCp)
                *pfRightOfCp = TRUE;

            fPseudoHit = TRUE;
        }
        else
        {
            if (    !(dwFlags & CFP_IGNOREBEFOREAFTERSPACE)
                &&  pli->_fHasNestedRunOwner
                &&  yLine + pli->_yHeight <= pt.y)
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
                LONG yHeightRubyBase = 0;

                AssertSz((pli != NULL) || (ili == 0),
                         "CDisplay::CpFromPoint invalid line pointer");

                 if (!me._pLS)
                    return -1;

                // Get character in the line
                me.SetCp(cp, NULL);

                // The y-coordinate should be relative to the baseline, and positive going up
                cch = pli->CchFromXpos(me, pt.x, yLine + pli->_yHeight - pli->_yDescent - pt.y, &dx, 
                                       _fRTL, dwFlags & CFP_EXACTFIT, &yHeightRubyBase);
                cchPreChars = me._cchPreChars;
                
                if (pfRightOfCp)
                    *pfRightOfCp = dx < 0;

                if (ppLayout)
                {
                    ptp = me.GetPtp();
                    if (ptp->IsBeginElementScope())
                    {
                        pNode = ptp->Branch();
                        if (   pNode->NeedsLayout()
                            && pNode->IsInlinedElement()
                           )
                        {
                            *ppLayout = pNode->GetUpdatedLayout();
                        }
                        else
                        {
                            *ppLayout = NULL;
                        }
                    }
                }

                // Don't allow click at EOL to select EOL marker and take into account
                // single line edits as well

                if (!(dwFlags & CFP_ALLOWEOL) && cch == pli->_cch)
                {
                    long cpPtp;

                    ptp = me.GetPtp();
                    Assert(ptp);

                    cpPtp = ptp->GetCp();

                    //
                    // cch > 0 && we are not in the middle of a text run,
                    // skip past all the previous node/pointer tree pos's.
                    // and position the measurer just after text.
                    if(cp < cpPtp && cpPtp == me.GetCp())
                    {
                        while (cp < cpPtp)
                        {
                            CTreePos *ptpPrev = ptp->PreviousTreePos();

                            if (!ptpPrev->GetBranch()->IsDisplayNone())
                            {
                                if (ptpPrev->IsText())
                                    break;
                                if (   ptpPrev->IsEndElementScope()
                                    && ptpPrev->Branch()->NeedsLayout()
                                   )
                                    break;
                            }
                            ptp = ptpPrev;
                            Assert(ptp);
                            cch   -= ptp->GetCch();
                            cpPtp -= ptp->GetCch();
                        }

                        while(ptp->GetCch() == 0)
                            ptp = ptp->NextTreePos();
                    }
                    else if (pElementFL->GetFirstBranch()->GetParaFormat()->HasInclEOLWhite(TRUE))
                    {
                        CTxtPtr tpTemp(GetMarkup(), cp + cch);
                        if (tpTemp.GetPrevChar() == WCH_CR)
                        {
                            cch--;
                            if (cp + cch < cpPtp)
                                ptp = NULL;
                        }
                    }

                    me.SetCp(cp + cch, ptp);
                }

                // Check if the pt is within bounds *vertically* too.
                if (dwFlags & CFP_IGNOREBEFOREAFTERSPACE)
                {
                    LONG top, bottom;

                    ptp = me.GetPtp();
                    if (   ptp->IsBeginElementScope()
                        && ptp->Branch()->NeedsLayout()
                        && ptp->Branch()->IsInlinedElement()
                       )
                    {
                        // Hit a site. Check if we are within the boundaries
                        // of the site.
                        RECT rc;
                        CLayout *pLayout = ptp->Branch()->GetUpdatedLayout();
                        
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
                                              pt.x,
                                              me.CurrBranch()->GetCharFormat());
                        top -= yHeightRubyBase;
                        bottom -= yHeightRubyBase;
                    }

                    // BUGBUG (t-ramar): this is fine for finding 99% of the
                    // pseudo hits, but if someone has a ruby with wider base
                    // text than pronunciation text, or vice versa, hits in the
                    // whitespace that results will not register as pseudo hits.
                    if (    pt.y <  top
                        ||  pt.y >= bottom
                        ||  (   !_fRTL
                            &&  (   pt.x <  pli->GetTextLeft()
                                ||  pt.x >= pli->GetTextRight(ili == LineCount() - 1)))
                        ||  (   _fRTL
                            && (   -pt.x <  pli->GetRTLTextRight()
                                || -pt.x >=  pli->GetRTLTextLeft())))
                    {
                        fPseudoHit = TRUE;
                    }
                }
                
                cp = (LONG)me.GetCp();

                ptp = me.GetPtp();
            }
        }
    }

    if(prp)
        prp->RpSet(ili, cch);

    if(pfPseudoHit)
        *pfPseudoHit = fPseudoHit;

    if(pcchPreChars)
        *pcchPreChars = cchPreChars;

    if(pptp)
    {
        LONG ich;

        *pptp = ptp ? ptp : pFlowLayout->GetContentMarkup()->TreePosAtCp(cp, &ich);
    }

    return cp;
}

//+----------------------------------------------------------------------------
//
// Member:      CDisplay::PointFromTp(tp, prcClient, fAtEnd, pt, prp, taMode,
//                                    pci, pfComplexLine, pfRTLFlow)
//
// Synopsis:    return the origin that corresponds to a given text pointer,
//              relative to the view
//
//-----------------------------------------------------------------------------

LONG CDisplay::PointFromTp(
    LONG        cp,         // point for the cp to be computed
    CTreePos *  ptp,        // tree pos for the cp passed in, can be NULL
    BOOL fAtEnd,            // Return end of previous line for ambiguous cp
    BOOL fAfterPrevCp,      // Return the trailing point of the previous cp (for an ambigous bidi cp)
    POINT &pt,              // Returns point at cp in client coords
    CLinePtr * const prp,   // Returns line pointer at tp (may be null)
    UINT taMode,            // Text Align mode: top, baseline, bottom
    CCalcInfo *pci,
    BOOL *pfComplexLine,
    BOOL *pfRTLFlow)
{
    CFlowLayout * pFL = GetFlowLayout();
    CLinePtr    rp(this);
    BOOL        fLastTextLine;
    CCalcInfo   CI;
    BOOL        fRTLLine;
    BOOL        fAtEndOfRubyBase;
    RubyInfo rubyInfo = {-1, 0, 0};

    //
    // If cp points to a node position somewhere between ruby base and ruby text, 
    // then we have to remember for later use that we are at the end of ruby text
    // (fAtEndOfRubyBase is set to TRUE).
    // If cp points to a node at the and of ruby element, then we have to move
    // cp to the beginning of text which follows ruby or to the beginning of block
    // element, whichever is first.
    //
    fAtEndOfRubyBase = FALSE;

    CTreePos * ptpNode = ptp ? ptp : GetMarkup()->TreePosAtCp(cp, NULL);
    LONG cpOffset = 0;
    while ( ptpNode && ptpNode->IsNode() )
    {
        ELEMENT_TAG eTag = ptpNode->Branch()->Element()->Tag();
        if ( eTag == ETAG_RT )
        {
            fAtEndOfRubyBase = ptpNode->IsBeginNode();
            break;
        }
        else if ( eTag == ETAG_RP )
        {
            if (ptpNode->IsBeginNode())
            {
                //
                // At this point we check where the RP tag is located: before or 
                // after RT tag. If this is before RT tag then we set fAtEndOfRubyBase
                // to TRUE, in the other case we do nothing.
                //
                // If RT tag is a parent of RP tag then we are after RT tag. 
                // In the other case we are before RT tag.
                //
                CTreeNode * pParent = ptpNode->Branch()->Parent();
                while ( pParent )
                {
                    if ( pParent->Tag() == ETAG_RUBY )
                    {
                        fAtEndOfRubyBase = TRUE;
                        break;
                    }
                    else if ( pParent->Tag() == ETAG_RT )
                    {
                        break;
                    }
                    pParent = pParent->Parent();
                }
            }
            break;
        }
        else if ( ptpNode->IsEndNode() && eTag == ETAG_RUBY )
        {
            for (;;)
            {
                cpOffset++;
                ptpNode = ptpNode->NextTreePos();
                if ( !ptpNode || !ptpNode->IsNode() || 
                    ptpNode->Branch()->Element()->IsBlockElement() )
                {
                    cp += cpOffset;
                    break;
                }
            }
            break;
        }
        cpOffset++;
        ptpNode = ptpNode->NextNonPtrTreePos();
    }

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

    if (!WaitForRecalc(min(cp + rp->_cch, GetLastCp()), -1, pci))
        return -1;

    if(!rp.RpSetCp(cp, fAtEnd))
        return -1;

    if (rp.IsValid())
        fRTLLine = rp->_fRTL;
    else if(ptp)
        fRTLLine = ptp->GetBranch()->GetParaFormat()->HasRTL(TRUE);
    else
        fRTLLine = _fRTL;

    if(pfRTLFlow)
        *pfRTLFlow = fRTLLine;

    pt.y = YposFromLine(rp, pci);

    if(!_fRTL)
    {
        pt.x = rp.IsValid()
            ? rp->_xLeft + rp->_xLeftMargin + (fRTLLine && !(rp.RpGetIch()) ? rp->_xWidth : 0)
                    : 0;
    }
    else
    {
        pt.x = rp.IsValid()
            ? rp->_xRight + rp->_xRightMargin  + (!fRTLLine && !(rp.RpGetIch()) ? rp->_xWidth : 0)
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

        Assert(rp.IsValid());

        me._li._xLeft = rp->_xLeft;
        me._li._xLeftMargin = rp->_xLeftMargin;
        me._li._xWidth = rp->_xWidth;
        me._li._xRight = rp->_xRight;
        me._li._xRightMargin = rp->_xRightMargin;
        me._li._fRTL = rp->_fRTL;

        // can we also add the _cch and _cchWhite here so we can pass them to 
        // the BidiLine stuff?
        me._li._cch = rp->_cch;
        me._li._cchWhite = rp->_cchWhite;

        LONG xCalc = me.MeasureText(rp.RpGetIch(), rp->_cch, fAfterPrevCp, pfComplexLine, pfRTLFlow, &rubyInfo);

        // Remember we ignore trailing spaces at the end of the line
        // in the width, therefore the x value that MeasureText finds can
        // be greater than the width in the line so we truncate to the
        // previously calculated width which will ignore the spaces.
        // pt.x += min(xCalc, rp->_xWidth);
        //
        // Why anyone would want to ignore the trailing spaces at the end
        // of the line is beyond me. For certain, we DON'T want to ignore
        // them when placing the caret at the end of a line with trailing
        // spaces. If you can figure out a reason to ignore the spaces,
        // please do, just leave the caret placement case intact. - Arye
        if (_fRTL == (unsigned) fRTLLine)
            pt.x += xCalc;
        else
            pt.x += rp->_xWidth - xCalc;
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
                if ( rubyInfo.cp != -1 && !fAtEndOfRubyBase )
                {
                    pt.y -= rubyInfo.yHeightRubyBase + pli->_yDescent - pli->_yTxtDescent;
                }
                
                if (!pli->IsFrame() &&
                    (taMode & TA_BASELINE) == TA_BASELINE)
                {
                    if ( rubyInfo.cp != -1 && !fAtEndOfRubyBase )
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
        if(!IsRectEmpty(prcLine))
        {
            UnionRect(prcBound, prcBound, prcLine);
        }
        else if (paryRects->Size() == 1)
        {
            *prcBound = *prcLine;
        }
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
    CSize size;
    CLayout *pLayout = pli->_pNodeLayout->GetUpdatedLayout();

    long xProposed = pLayout->GetXProposed();
    long yProposed = pLayout->GetYProposed();

    pLayout->GetSize(&size);

    // add the curent line to the region
    prcLine->top = yProposed + yPos + pli->_yBeforeSpace;

    if(!fRTL)
    {
        prcLine->left = xProposed + pli->_xLeftMargin + pli->_xLeft;
        prcLine->right = prcLine->left + size.cx;
    }
    else
    {
        prcLine->right = -(xProposed + pli->_xRightMargin + pli->_xRight);
        prcLine->left = prcLine->right - size.cx;
    }
    prcLine->bottom = prcLine->top + size.cy;

}

//+----------------------------------------------------------------------------
//
// Function:    ComputeIndentsFromParentNode
//
// Synopsis:    Compute the indent for a given Node and a left and/or
//              right aligned site that a current line is aligned to.
//
//-----------------------------------------------------------------------------

void 
CDisplay::ComputeIndentsFromParentNode(CCalcInfo * pci,         // (IN)
                                       CTreeNode * pNode,       // (IN) node we want to compute indents for
                                       DWORD  dwFlags,          // (IN) flags from RFE
                                       LONG * pxLeftIndent,     // (OUT) the node is indented this many pixels from left edge of the layout
                                       LONG * pxRightIndent)    // (OUT) ...
{
    CElement  * pElement = pNode->Element();
    CElement  * pElementFL = GetFlowLayoutElement();
    LONG        xParentLeftPadding, xParentRightPadding;
    LONG        xParentLeftBorder, xParentRightBorder;

    const CParaFormat *pPF = pNode->GetParaFormat();
    BOOL  fInner = pNode->Element() == pElementFL;

    Assert(pNode);

    // GetLeft/RightIndent returns the cumulative CSS margin (+ some other gunk
    // like list bullet offsets).
    
    LONG        xLeftMargin  = pPF->GetLeftIndent(pci, fInner);
    LONG        xRightMargin = pPF->GetRightIndent(pci, fInner);

    // We only want to include the area for the bullet for hit-testing;
    // we _don't_ draw backgrounds and borders around the bullet for list items.
    if (    dwFlags == RFE_HITTEST
        &&  pPF->_bListPosition != styleListStylePositionInside
        &&  (   pElement->IsFlagAndBlock(TAGDESC_LISTITEM)
            ||  pElement->IsFlagAndBlock(TAGDESC_LIST)))

    {
        if (!pPF->HasRTL(fInner))
        {
            xLeftMargin -= pPF->GetBulletOffset(pci, fInner);
        }
        else
        {
            xRightMargin -= pPF->GetBulletOffset(pci, fInner);
        }
    }

    // Compute the padding and border space cause by the current
    // element's ancestors (up to the layout).
    if ( pNode->Element() == pElementFL )
    {
        // If the element in question is actually the layout owner,
        //b then we don't want to offset by our own border/padding,
        // so set the values to 0.  
        xParentLeftPadding = xParentLeftBorder =
            xParentRightPadding = xParentRightBorder = 0;
    }
    else
    {
        // We need to get the cumulative sum of our ancestor's borders/padding.
        
        pNode->Parent()->Element()->ComputeHorzBorderAndPadding( pci, pNode->Parent(), pElementFL,
                                &xParentLeftBorder, &xParentLeftPadding,
                                &xParentRightBorder, &xParentRightPadding );
                                
        // The return results of ComputeHorzBorderAndPadding() DO NOT include
        // the border or padding of the layout itself; this makes sense for
        // borders, because the layout's border is outside the bounds we're
        // interested in.  However, we do want to account for the layout's
        // padding since that's inside the bounds.  We fetch that separately
        // here, and add it to the cumulative padding.

        long lPadding[PADDING_MAX];
        GetPadding( pci, lPadding, pci->_smMode == SIZEMODE_MMWIDTH );
        
        xParentLeftPadding += lPadding[PADDING_LEFT];
        xParentRightPadding += lPadding[PADDING_RIGHT];
    }

    // The element is indented by the sum of CSS margins and ancestor
    // padding/border.  This indent value specifically ignores aligned/floated
    // elements, per CSS!
    *pxLeftIndent = xLeftMargin + xParentLeftBorder + xParentLeftPadding;
    *pxRightIndent = xRightMargin + xParentRightBorder + xParentRightPadding;
}

//+----------------------------------------------------------------------------
//
// Member:      RegionFromElement
//
// Synopsis:    for a given element, find the set of rects (or lines) that this
//              element occupies in the display. The rects returned are relative
//              the site origin.
//              Certain details about the returned region are determined by the
//              call type parameter:..
//
//-----------------------------------------------------------------------------

void
CDisplay::RegionFromElement(CElement       * pElement,  // (IN)
                            CDataAry<RECT> * paryRects, // (OUT)
                            CPoint         * pptOffset, // == NULL, point to offset the rects by (IN param)
                            CFormDrawInfo  * pDI,       // == NULL
                            DWORD dwFlags,              // == 0
                            LONG cpClipStart,           // == -1, (IN)
                            LONG cpClipFinish,          // == -1, (IN) clip range
                            RECT * prcBound)            // == NULL, (OUT param) returns bounding rect that ignores clipping

{
    CFlowLayout *       pFL = GetFlowLayout();
    CElement *          pElementFL = pFL->ElementOwner();
    CTreePos *          ptpStart;
    CTreePos *          ptpFinish;
    CCalcInfo           CI;
    RECT                rcLine;
    CPoint              ptTrans = g_Zero.pt;
    BOOL                fScreenCoord = dwFlags & RFE_SCREENCOORD  ? TRUE : FALSE;

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
        CLSMeasurer         me(this, pDI);
        CMarkup           * pMarkup = pFL->GetContentMarkup();
        BOOL                fBlockElement;
        BOOL                fTightRects = (dwFlags & RFE_TIGHT_RECTS);
        BOOL                fIgnoreBlockness = (dwFlags & RFE_IGNORE_BLOCKNESS);
        BOOL                fIncludeAlignedElements = !(dwFlags & RFE_IGNORE_ALIGNED);
        BOOL                fIgnoreClipForBoundRect = (dwFlags & RFE_IGNORE_CLIP_RC_BOUNDS);
        BOOL                fIgnoreRel = (dwFlags & RFE_IGNORE_RELATIVE);
        BOOL                fNestedRel = (dwFlags & RFE_NESTED_REL_RECTS);
        BOOL                fScrollIntoView = (dwFlags & RFE_SCROLL_INTO_VIEW);
        BOOL                fNeedToMeasureLine;
        LONG                cp, cpStart, cpFinish, cpElementLast;
        LONG                cpElementStart, cpElementFinish;
        LONG                iCurLine, iFirstLine, ich;
        LONG                yPos;
        LONG                xParentRightIndent = 0;
        LONG                xParentLeftIndent = 0;
        LONG                yParentPaddBordTop = 0;
        LONG                yParentPaddBordBottom = 0;
        LONG                yTop;
        BOOL                fFirstLine;
        CLinePtr            rp(this);
        BOOL                fRestorePtTrans = FALSE;
        CStackDataAry<RECT, 8> aryChunks(Mt(CDisplayRegionFromElement_aryChunks_pv));

        if (!me._pLS)
            goto Cleanup;
    
        // Do we treat the element we're finding the region for
        // as a block element?  Things that influence this decision:
        // If RFE_SELECTION was specified, it means we're doing
        // finding a region for selection, which is always character
        // based (never block based).  RFE_SELECTION causes fIgnoreBlockness to light up.
        // The only time RFE should get called on an element that
        // needs layout is when the pElement == pElementFL.  When
        // this happens, even though the element _is_ block, we
        // want to treat it as though it isn't, since the caller
        // in this situation wants the rects of some text that's
        // directly under the layout (e.g. "x" in <BODY>x<DIV>...</DIV></BODY>)
        // BUGBUG: (KTam) this would be a lot more obvious if we changed
        // the !pElement->NeedsLayout() condition to pElement != pElementFL
        fBlockElement =  !fIgnoreBlockness &&   
                          pElement->IsBlockElement() &&
                         !pElement->NeedsLayout();   

        if (pDI)
            CI.Init(pDI, pFL);
        else
            CI.Init(pFL);
    
        long            xParentWidth;
        long            yParentHeight;
        // Fetch parent's width and height (i.e. view width & height minus padding)
        GetViewWidthAndHeightForChild(&CI, &xParentWidth, &yParentHeight);
        CI.SizeToParent(xParentWidth, yParentHeight);
    
        // If caller hasn't specified non-relative behaviour, then account for
        // relativeness on the layout by fetching the relative offset in ptTrans.
        if(!fIgnoreRel)
            pNode->GetRelTopLeft(pElementFL, &CI, &ptTrans.x, &ptTrans.y);
    
        // Transform the relative offset to global coords if caller specified
        // RFE_SCREENCOORD.        
        if (fScreenCoord)
            pFL->TransformPoint( &ptTrans, COORDSYS_CONTENT, COORDSYS_GLOBAL);
    
        // Combine caller-specified offset (if any) into relative offset.
        if(pptOffset)
        {
            ptTrans.x += pptOffset->x;
            ptTrans.y += pptOffset->y;
        }

        cpStart  = pFL->GetContentFirstCp();
        cpFinish = pFL->GetContentLastCp();        
    
        // get the cp range for the element
        cpElementStart  = ptpStart->GetCp();
        cpElementFinish = ptpFinish->GetCp();
    
        // Establish correct cp's and treepos's, including clipping stuff..
        // We may have elements overlapping multiple layout's, so clip the cp range
        // to the current flow layout's bounds,
        cpStart       = max(cpStart, cpElementStart);
        cpFinish      = min(cpFinish, cpElementFinish);
        cpElementLast = cpFinish;
    
        // clip cpFinish to max calced cp
        cpFinish      = min(cpFinish, GetMaxCpCalced());
    
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

        if ( !fIgnoreClipForBoundRect )
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
    
        if(!LineCount())
            return;
    
        // BUGBUG: we pass in absolute cp here so RpSetCp must call
        // pElementFL->GetFirstCp while we have just done this.  We
        // should optimize this out by having a relative version of RpSetCp.
    
        // skip frames and position it at the beginning of the line
        rp.RpSetCp(cpStart, /* fAtEnd = */ FALSE, TRUE);
        
        // (cpStart - rp.GetIch) == cp of beginning of line (pointed to by rp)
        // If fFirstLine is true, it means that the first line of the element
        // is within the clipping range (accounted for by cpStart).
        fFirstLine = cpElementStart >= cpStart - rp.GetIch();

        // BUGBUG: (KTam) re-enable this assert when Sujal fixes
        // CpFromPointEx().
        // Assert( cpStart != cpFinish );
        // Update 12/09/98: It is valid for cpStart == cpFinish
        // under some circumstances!  When a hyperlink jump is made to
        // a local anchor (#) that's empty (e.g. <A name="foo"></A>, 
        // the clipping range passed in is empty! (HOT bug 60358)
        // If the element has no content then return the null rectangle
        if(cpStart == cpFinish)
        {
            CLine * pli = Elem(rp);

            yPos = YposFromLine(rp, &CI);

            rcLine.top = yPos + rp->GetYTop();
            rcLine.bottom = yPos + rp->GetYBottom();

            rcLine.left = rp->GetTextLeft();

            if(rp.GetIch())
            {
                me.SetCp(cpStart - rp.GetIch(), NULL);
                me._li = *pli;

                rcLine.left +=  me.MeasureText(rp.RpGetIch(), pli->_cch);
            }
            rcLine.right = rcLine.left;

            AppendRectToElemRegion(paryRects, prcBound, &rcLine,
                                    ptTrans,
                                    cpStart, cpClipStart, cpClipFinish);
            return;
        }
    
        // Compute the padding and border space for the first line
        // and the last line for ancestors that are entering and leaving
        // scope respectively.  For example, consider an element x such that:
    
        // < elem 1 >
        //   < elem 2 >
        //     < ..... >
        //       < elem x >
        //         <!-- some content -->
        //       </ elem x >
        //     </ ..... >
        //   </ elem 2 >
        // </ elem 1 >
        // 
        // I.e., elem x is immediately preceded by some number of other
        // elements _entering_ scope (with no content in between them).
        // The opening of such elements may introduce vertical space in
        // terms of border and padding; we want to compute that so we
        // can adjust for it when determining the rect of the first line
        // of elem x.
        // 
        // A similar situation exists with respect to elements _leaving_
        // scope immediately following the closing of element x.
        
        if(pElement && fBlockElement && pElement != pElementFL)
        {
            ComputeVertPaddAndBordFromParentNode(&CI,
                    ptpStart, ptpFinish,
                    &yParentPaddBordTop, &yParentPaddBordBottom);
        }
    
        iCurLine = iFirstLine = rp;
    
        // Now that we know what line contains the cp, let's try to compute the
        // bounding rectangle.
    
        // If the line has aligned images in it and if they are at the begining of the
        // the line, back up all the frame lines that are before text and contained
        // within the element of interest.  This only necessary when doing selection
        // and hit testing, since the region for borders/backgrounds ignores aligned
        // and floated stuff.
    
        // (recall rp is like a smart ptr that points to a particular cp by
        // holding a line index (ili) AND an offset into the line's chars
        // (ich), and that we called CLinePtr::RpSetCp() some time back.
    
        // (recall frame lines are those that were created expressly for
        // aligned elems; they are the result of "breaking" a line, which
        // we had to do when we were measuring and saw it had aligned elems)
        
        if ( fIncludeAlignedElements  && rp->HasAligned() )
        {
            LONG diLine = -1;
            CLine * pli;
    
            // rp[x] means "Given that rp is indexing line y, return the
            // line indexed by y - x".
    
            // A line is "blank" if it's "clear" or "frame".  Here we
            // walk backwards through the line array until all consecutive
            // frame lines before text are passed.
            while((iCurLine + diLine >= 0) && (pli = &rp[diLine]) && pli->IsBlankLine())
            {
                // Stop walking back if we've found a frame line that _isn't_ "before text",
                // or one whose ending cp is prior to the beginning of our clipping range
                // (which means it's not contained in the element of interest)
                // Consider: <div><img align=left><b><img align=left>text</b>
                // The region for the bold element includes the 2nd image but
                // not the 1st, but both frame lines will show up in front of the bold line.
                // The logic below notes that the last cp of the 1st img is before the cpStart
                // of the bold element, and breaks so we don't include it.
                if (    pli->IsFrame()
                    &&  (   !pli->_fFrameBeforeText
                        ||  pli->_pNodeLayout->Element()->GetLastCp() < cpStart))
                {
                    break;
                }
    
                diLine--;
            }
            iFirstLine = iCurLine + diLine + 1;
        }
    
        // compute the ypos for the first line
        yTop = yPos = YposFromLine(iFirstLine, &CI);
    
        // For calls other than backgrounds/borders, add all the frame lines
        // before the current line under the influence of
        // the element to the region.
    
        if ( fIncludeAlignedElements )
        {
            for ( ; iFirstLine < iCurLine; iFirstLine++)
            {
                CLine * pli = Elem(iFirstLine);
    
                // If the element of interest is block level, find out
                // how much it's indented (left and right) in from its
                // parent layout.
                if (fBlockElement)
                {
                    CTreeNode * pNodeTemp = pMarkup->SearchBranchForScopeInStory(ptpStart->GetBranch(), pElement);
                    if (pNodeTemp)
                    {
                        //Assert(pNodeTemp);
                        ComputeIndentsFromParentNode( &CI, pNodeTemp, dwFlags,
                                                      &xParentLeftIndent,
                                                      &xParentRightIndent);
                    }
                }
    
                // If it's a frame line, add the line's rc to the region.
                if (pli->IsFrame())
                {
                    long cpLayoutStart  = pli->_pNodeLayout->Element()->GetFirstCp();
                    long cpLayoutFinish = pli->_pNodeLayout->Element()->GetLastCp();
    
                    if (cpLayoutStart >= cpStart && cpLayoutFinish <= cpFinish)
                    {
                        RcFromAlignedLine(&rcLine, pli, yPos,
                                            fBlockElement, fFirstLine, _fRTL,
                                            xParentLeftIndent, xParentRightIndent);
    
                        AppendRectToElemRegion(paryRects, prcBound, &rcLine,
                                               ptTrans,
                                               cpFinish, cpClipStart, cpClipFinish);
                    }
                }
                // if it's not a frame line, it MUST be a clear line (since we
                // only walked by while seeing blank lines).  All clear lines
                // force a new physical line, so account for change in yPos.
                else
                {
                    Assert( pli->IsClear() && pli->_fForceNewLine );
                    yPos += pli->_yHeight;
                }
            }
        }
    
        // now add all the lines that are contained by the range
        for ( cp = cpStart; cp < cpFinish; iCurLine++ )
        {
            BOOL    fRTLLine;
            LONG    xStart = 0;
            LONG    xEnd = 0;
            long    yTop, yBottom;
            LONG    cchAdvance = min(rp.GetCchRemaining(), cpFinish - cp);
            CLine * pli;
            LONG    i;
            LONG    cChunk;
            CPoint  ptTempForPtTrans(0,0);
                    
            Assert(!fRestorePtTrans);
            if ( iCurLine >= LineCount() )
            {
                // (srinib) - please check with one of the text
                // team members if anyone hits this assert. We want
                // to investigate. We shoud never be here unless
                // we run out of memory.
                AssertSz(FALSE, "WaitForRecalc failed to measure lines till cpFinish");
                break;
            }
    
            pli = Elem(iCurLine);
    
            // When drawing backgrounds, we skip frame lines contained
            // within the element (since aligned stuff doesn't affect our borders/background)
            // but for hit testing and selection we need to account for them.
            if ( !fIncludeAlignedElements  && pli->IsFrame() )
            {
                goto AdvanceToNextLineInRange;
            }

            // If line is relative then add in the relative offset
            // In the case of Wigglys we ignore the line's relative positioning, but want any nested
            // elements to be handled CLineServices::CalcRectsOfRangeOnLine
            if (   fNestedRel 
                && pli->_fRelative
               )
            {
                CPoint ptTemp;
                CTreeNode *pNodeNested = pMarkup->TreePosAtCp(cp, &ich)->GetBranch();

                // We only want to adjust for nested elements that do not have layouts. The pElement
                // we passed in is handled by the fIgnoreRel flag. Layout elements are handled in
                // CalcRectsOfRegionOnLine
                if(    pNodeNested->Element() != pElement 
                   && !pNodeNested->HasLayout()
                  )
                {
                    pNodeNested->GetRelTopLeft(pElementFL, &CI, &ptTemp.x, &ptTemp.y);
                    ptTempForPtTrans = ptTrans;
                    if(!fIgnoreRel)
                    {
                        ptTrans.x = ptTemp.x - ptTrans.x;
                        ptTrans.y = ptTemp.y - ptTrans.y;
                    }
                    else
                    {
                        // We were told to ignore the pElement's relative positioning. Therefore
                        // we only want to adjust by the amount of the nested element's relative
                        // offset from pElement
                        long xElemRelLeft = 0, yElemRelTop = 0;
                        pNode->GetRelTopLeft(pElementFL, &CI, &xElemRelLeft, &yElemRelTop);
                        ptTrans.x = ptTemp.x - xElemRelLeft;
                        ptTrans.y = ptTemp.y - yElemRelTop;
                    }
                    fRestorePtTrans = TRUE;
                }
            }
            
            fRTLLine = pli->_fRTL;
    
            // If the element of interest is block level, find out
            // how much it's indented (left and right) in from its
            // parent layout.
            if ( fBlockElement )
            {
                if ( cp != cpStart )
                {
                    ptpStart = pMarkup->TreePosAtCp(cp, &ich);
                }
    
                CTreeNode * pNodeTemp = pMarkup->SearchBranchForScopeInStory(ptpStart->GetBranch(), pElement);
                // (fix this for IE5:RTM, #46824) srinib - if we are in the inclusion, we
                // wont find the node.
                if ( pNodeTemp )
                {
                    //Assert(pNodeTemp);
                    ComputeIndentsFromParentNode( &CI, pNodeTemp, dwFlags,
                                                  &xParentLeftIndent,
                                                  &xParentRightIndent);
                }
            }
    
            // For RTL lines, we will work from the right side. LTR lines work
            // from the left, of course.
    
            // WARNING!!! Be sure that any changes that are made to the LTR
            // case are reflected in the RTL case.           
    
            if (!fRTLLine)
            {              
                if ( fBlockElement )
                {
                    // Block elems, except when processing selection:
                    
                    // _fFirstFragInLine means it's the first logical line (chunk) in the physical line.
                    // If that is the case, the starting edge is just the indent from the parent
                    // (margin due to floats is ignored).
                    // If it's not the first logical line, then treat element as though it were inline.
                    xStart = pli->_fFirstFragInLine
                                ? xParentLeftIndent
                                : pli->_xLeftMargin + pli->_xLeft;
                    // _fForceNewLine means it's the last logical line (chunk) in a physical line.
                    // If that is the case, the line or view width includes the parent's right indent,
                    // so we need to subtract it out.  We also do this for dummy lines (actually,
                    // we should think about whether it's possible to skip processing of dummy lines altogether!)
                    // Otherwise, xLeftMargin+xLineWidth doesn't already include parent's right indent
                    // (because there's at least 1 other line to the right of this one), so we don't
                    // need to subtract it out, UNLESS the line is right aligned, in which case
                    // it WILL include it so we DO need to subtract it (wheee..).
                    xEnd = ( pli->_fForceNewLine || pli->_fDummyLine )
                            ? max(pli->_xLineWidth, GetViewWidth() ) - xParentRightIndent
                            : pli->_xLeftMargin + pli->_xLineWidth - (pli->IsRightAligned() ? xParentRightIndent : 0);
                }
                else
                {
                    // Inline elems, and all selections begin at the text of
                    // the element, which is affected by margin.
                    xStart = pli->_xLeftMargin + pli->_xLeft;
                    // GetTextRight() tells us where the text ends, which is
                    // just what we want.
                    xEnd = pli->GetTextRight(iCurLine == LineCount() - 1);
                    // Only include whitespace for selection
                    if ( fIgnoreBlockness )
                    {
                        xEnd += pli->_xWhite;
                    }
                }
            }
            else
            {
                if ( fBlockElement )
                {
                    xStart = pli->_fFirstFragInLine
                                ? xParentRightIndent
                                : pli->_xRightMargin + pli->_xRight;
                    xEnd = ( pli->_fForceNewLine || pli->_fDummyLine )
                            ? max(pli->_xLineWidth, GetViewWidth() ) - xParentLeftIndent
                            : pli->_xRightMargin + pli->_xLineWidth - (pli->IsLeftAligned ? xParentLeftIndent : 0);
                }
                else
                {
                    xStart = pli->_xRightMargin + pli->_xRight;
                    xEnd = pli->GetRTLTextLeft();
                    // Only include whitespace for selection
                    if ( fIgnoreBlockness )
                    {
                        xEnd += pli->_xWhite;
                    }
                }
            }
    
            if (xEnd < xStart)
            {
                // Only clear lines can have a _xLineWidth < _xWidth + _xLeft + _xRight ...
                Assert(pli->IsClear());
                xEnd = xStart;
            }
    
            // Set the top and bottom
            if (fBlockElement)
            {
                yTop = yPos;
                yBottom = yPos + max(pli->_yHeight, pli->GetYBottom());
    
                if (fFirstLine)
                {
                    yTop += pli->_yBeforeSpace + 
                            min(0L, pli->GetYHeightTopOff()) +
                            yParentPaddBordTop;
    
                    Assert (yBottom >= yTop);
                }
                else
                {
                    yTop += min(0L, pli->GetYTop());
                }
    
                if (pli->_fForceNewLine && cp + cchAdvance >= cpElementLast)
                {
                    yBottom -= yParentPaddBordBottom;
                }
            }
            else
            {
                yTop = yPos + pli->GetYTop();
                yBottom = yPos + pli->GetYBottom();

                // 66677: Let ScrollIntoView scroll to top of yBeforeSpace on first line.
                if (fScrollIntoView && yPos==0)
                    yTop = 0;
            }
    
            aryChunks.DeleteAll();
            cChunk = 0;
    
            // At this point we've found the bounds (xStart, xEnd, yTop, yBottom)
            // for a line that is part of the range.  Under certain circumstances,
            // this is insufficient, and we need to actually do measurement.  These
            // circumstances are:
            // 1.) If we're doing selection, we only need to measure partially
            // selected lines (i.e. at most we need to measure 2 lines -- the
            // first and the last).  For completely selected lines, we'll
            // just use the line bounds.  BUGBUG: this MAY introduce selection
            // turds, since LS uses tight-rects to draw the selection, and
            // our line bounds may not be as wide as LS tight-rects measurement
            // (recall that LS measurement catches whitespace that we sometimes
            // omit -- this may be fixable by adjusting our treatment of xWhite
            // and/or cchWhite).
            // Determination of partially selected lines is done as follows:
            // rp.GetIch() != 0 implies it starts mid-line,
            // rp.GetCchRemaining() != 0 implies it ends mid-line (the - _cchWhite
            // makes us ignore whitespace chars at the end of the line)
            // 2.) For all other situations, we're choosing to measure all
            // non-block elements.  This means that situations specifying
            // tight-rects (backgrounds, focus rects) will get them for
            // non-block elements (which is what we want), and hit-testing
            // will get the right rect if the element is inline and doesn't
            // span the entire line.  BUGBUG: there is a possible perf
            // improvement here; for hit-testing we really only need to
            // measure when the inline element doesn't span the entire line
            // (right now we measure when hittesting all inline elements);
            // this condition is the same as that for selecting partial lines,
            // however there may be subtler issues here that need investigation.
            if ( (dwFlags & RFE_SELECTION) == RFE_SELECTION )
            {
                fNeedToMeasureLine =    !rp->IsBlankLine()
                                     && (   rp.GetIch()
                                         || max(0L, (LONG)(rp.GetCchRemaining() - (fBlockElement ? rp->_cchWhite : 0))) > cchAdvance );
            }
            else
            {
                fNeedToMeasureLine = !fBlockElement;
            }


            if ( fNeedToMeasureLine )
            {
                // (KTam) why do we need to set the measurer's cp to the 
                // beginning of the line? (cp - rp.GetIch() is beg. of line)
                Assert(!rp.GetIch() || cp == cpStart);
                ptpStart = pMarkup->TreePosAtCp(cp - rp.GetIch(), &ich);
                me.SetCp(cp - rp.GetIch(), ptpStart);
                me._li = *pli;
                // Get chunk info for the (possibly partial) line.
                // Chunk info is returned in aryChunks.
                cChunk = me.MeasureRangeOnLine(rp.GetIch(), cchAdvance, *pli, yPos, &aryChunks, dwFlags);
                if (cChunk == 0)
                {
                    xEnd = xStart;
                }
            }

            // cChunk == 0 if we didn't need to measure the line, or if we tried
            // to measure it and MeasureRangeOnLine() failed.
            if ( cChunk == 0 )
            {
                rcLine.left = xStart;
                rcLine.right = xEnd;
                rcLine.top = yTop;
                rcLine.bottom = yBottom; 
    
                // Adjust xStart and xEnd to the display coordinates.
                if ((BOOL) _fRTL != fRTLLine)
                {
                    int xWidth = pli->_xLeftMargin + pli->_xRightMargin + pli->_xLineWidth;
                    rcLine.left = xWidth - rcLine.left;
                    rcLine.right = xWidth - rcLine.right;
                }

                // adjust for origin at top right
                if (_fRTL)
                {
                    rcLine.left = -rcLine.left;
                    rcLine.right = -rcLine.right;
                }
    
                // make sure we have a positive rect
                if(rcLine.left > rcLine.right)
                {
                    long temp = rcLine.right;
                    rcLine.right = rcLine.left;
                    rcLine.left = temp;
                }
    
                AppendRectToElemRegion(paryRects, prcBound, &rcLine,
                                       ptTrans,
                                       cp, cpClipStart, cpClipFinish);
    
            }
            else
            {
                Assert(aryChunks.Size() > 0);
    
                i = 0;
                while (i < aryChunks.Size())
                {
                    RECT rcChunk = aryChunks[i++];
                    LONG xStartChunk = xStart + rcChunk.left;
                    LONG xEndChunk;
    
                    // if it is the first or the last chunk, use xStart & xEnd
                    if (fBlockElement && (i == 1 || i == aryChunks.Size()))
                    {
                        if (i == 1)
                            xStartChunk = xStart;

                        if(i == aryChunks.Size())
                            xEndChunk = xEnd;
                        else
                            xEndChunk = xStart + rcChunk.right;
                    }
                    else
                    {
                         xEndChunk = xStart + rcChunk.right;
                    }
    
                    // Adjust xStart and xEnd to the display coordinates.
                    if ((BOOL) _fRTL != fRTLLine)
                    {
                        int xWidth = pli->_xLeftMargin + pli->_xRightMargin + pli->_xLineWidth;
                        xStartChunk = xWidth - xStartChunk;
                        xEndChunk = xWidth - xEndChunk;
                    }
    
                    // adjust for origin at top right
                    if (_fRTL)
                    {
                        xStartChunk = -xStartChunk;
                        xEndChunk = -xEndChunk;
                    }
    
                    if (xStartChunk <= xEndChunk)
                    {
                        rcLine.left     = xStartChunk;
                        rcLine.right    = xEndChunk;
                    }
                    else
                    {
                        rcLine.left     = xEndChunk;
                        rcLine.right    = xStartChunk;
                    }
    
                    if(!fTightRects)
                    {
                        rcLine.top = yTop;
                        rcLine.bottom = yBottom; 
                    }
                    else
                    {
                        rcLine.top = rcChunk.top;
                        rcLine.bottom = rcChunk.bottom; 
                    }
    
                    AppendRectToElemRegion(paryRects, prcBound, &rcLine,
                                           ptTrans,
                                           cp, cpClipStart, cpClipFinish);
                }
            }
    
        AdvanceToNextLineInRange:

            if (fRestorePtTrans)
            {
                ptTrans = ptTempForPtTrans;
                fRestorePtTrans = FALSE;
            }
            
            cp += cchAdvance;
    
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
    
        // For calls for selection or hittesting (but not background/borders),
        // if the last line contains any aligned images, check to see if
        // there are any aligned lines following the current line that come
        // under the scope of the element
        if ( fIncludeAlignedElements )
        {
            if(!rp.GetIch())
                rp.AdjustBackward();
    
            iCurLine = rp;
    
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
    
                    if (fBlockElement)
                    {
                        CTreeNode * pNodeTemp = pMarkup->SearchBranchForScopeInStory (ptpFinish->GetBranch(), pElement);
                        if (pNodeTemp)
                        {
                            // Assert(pNodeTemp);
                            ComputeIndentsFromParentNode( &CI, pNodeTemp, dwFlags,
                                                          &xParentLeftIndent, 
                                                          &xParentRightIndent);
                        }
                    }
    
                    // if the current line is a frame line and if the site contained
                    // in it is contained by the current element include it other wise
                    if(cpStart <= cpLayoutStart && cpFinish >= cpLayoutFinish)
                    {
                        RcFromAlignedLine(&rcLine, pli, yPos,
                                            fBlockElement, fFirstLine, _fRTL,
                                            xParentLeftIndent, xParentRightIndent);
    
                        AppendRectToElemRegion(paryRects, prcBound, &rcLine,
                                               ptTrans,
                                               cpFinish, cpClipStart, cpClipFinish);
                    }
                    diLine++;
                }
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
    long                cch )
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
        // 1) There is no guarantee that cp passed in will be at the beginning of a line
        // 2) In the loop below we decrement cch by the count of chars in the line
        //
        // This would be correct if the cp passed in was the beginning of the line
        // but since it is not, we need to bump up the char count by the offset
        // of cp within the line. If at BOL then this would be 0. (bug 47687 fix 2)
        //
        cch += rp.RpGetIch();
        
        //
        //  Extend the rectangle over the affected lines
        //

        for (; cch > 0 && ili < LineCount(); ili++)
        {
            pli = Elem(ili);

            yTop    = rc.top + pli->GetYLineTop();
            yBottom = rc.bottom + pli->GetYLineBottom();

            rc.top    = min(rc.top, yTop);
            rc.bottom = max(rc.bottom, yBottom);

            // BUGBUG: we need to get the right pNode.. how?
            // If the line is relative, apply its relative offset
            // to the rect
            /*
            if ( pli->_fRelative )
            {
                long xRelOffset, yRelOffset;
                CCalcInfo CI( pFlowLayout );

                Assert( pNode );
                pNode->GetRelTopLeft( pFlowLayout->ElementOwner(), &CI, &xRelOffset, &yRelOffset );

                rc.top += yRelOffset;
                rc.bottom += yRelOffset;
            }
            */

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
    pLayout = pNode->GetUpdatedNearestLayout();

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
        ili = PointFromTp(cp, ptp, fAtEnd, FALSE, pt, prp, taMode, pci);
        if(ili > 0)
            rp.SetRun(ili, 0);
    }
    else
    {
        ili = rp;

        pt.y = YposFromLine(rp, pci);

        if(!_fRTL)
        {
            pt.x = rp->_xLeft + rp->_xLeftMargin;
        }
        else
        {
            pt.x = rp->_xRight + rp->_xRightMargin;
        }

        if (prp)
            *prp = rp;

    }

    return rp;
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
    BOOL fHasListItem = FALSE;
    CLine *pLine = NULL; // Keep compiler happy.
    CMarkup *pMarkup = GetMarkup();
    CTreePos *ptp;
    LONG cchNotUsed;
    Assert(prcView);
    Assert(prcInval);

    ptp = pMarkup->TreePosAtCp(cpFirst, &cchNotUsed);
    // BUGBUG (sujalp): We might want to search for other interesting
    // list elements here.
    CElement *pElement = pMarkup->SearchBranchForTag(ptp->GetBranch(), ETAG_OL)->SafeElement();
    if (   pElement
        && pElement->IsBlockElement()
       )
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

    if(max(0l, yHeightMax) != max(0l, _yHeight))
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
        if (!pLine->_fCanBlastToScreen)
            WriteString(g_f, _T("Noblast - "));
        if (pLine->_fRTL)
            WriteString(g_f, _T("RTL - "));

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

            for(long i = 0; i < prdnc->Size(); i++)
            {
                CRelDispNode * prdn = (*prdnc)[i];

                WriteString(g_f, _T("\tElement: "));
                WriteString(g_f, (TCHAR *)prdn->GetElement()->TagName());
                WriteHelp(g_f, _T(", SN:<0d>,"), prdn->GetElement()->SN());

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

void CDisplay::DumpLineText(HANDLE hFile)
{
    if (Count() > 0)
    {
        CTxtPtr tp(GetMarkup(), GetFirstCp());
        DumpLineText(hFile, &tp, 0);
    }
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

        ptp->GetRawText( cchChunk, chBuff );

        WriteFormattedString( hFile, chBuff, cchChunk );

        if (pLine->_cch > cchChunk)
        {
            WriteString( hFile, _T("..."));
        }
        ptp->AdvanceCp(pLine->_cch);
    }
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
    BOOL fSelected    ) 
{
    CFlowLayout * pFlowLayout = GetFlowLayout();
    CElement * pElement = pFlowLayout->ElementContent();
    CStackDataAry < RECT, CACHED_INVAL_RECTS > aryInvalRects(Mt(CDisplayShowSelectedRange_aryInvalRects_pv));
    CTreePos* pPrevPos = NULL;
    
    AssertSz(pFlowLayout->IsInPlace(),
        "CDisplay::ShowSelected() called when not in-place active");

    int cpClipStart = ptpStart->GetCp( WHEN_DBG(FALSE));
    int cpClipFinish = ptpEnd->GetCp( WHEN_DBG(FALSE));
    Assert( cpClipStart <= cpClipFinish);

    if ( cpClipFinish > cpClipStart) // don't bother with cpClipFinish==cpClipStart
    {
        //
        // BUGBUG we make the minimum selection size 3 chars to plaster over any off-by-one
        // problems with Region-From-Element, we also don't do this trick if a TreePos is
        // at an element - as RFE may get confused.        
        //
        if ( cpClipFinish + 1 < GetLastCp() && ptpEnd->IsPointer() )
            cpClipFinish++; 
        if (  cpClipStart- 1 > GetFirstCp() && ptpStart->IsPointer() )
        {
            //
            // Make sure you're not about to set the Cp to that of another valid TreePos.
            //
            pPrevPos = ptpStart->PreviousTreePos();
            Assert( pPrevPos );
            if ( pPrevPos && pPrevPos->GetCp(WHEN_DBG(FALSE) ) != cpClipStart - 1 )
                cpClipStart--;
        }    
        WaitForRecalc(min(GetLastCp(), long(cpClipFinish)), -1);
      
        RegionFromElement( pElement, &aryInvalRects, NULL, NULL, RFE_SELECTION, cpClipStart, cpClipFinish, NULL ); 

#if DBG == 1
        TraceTag((tagDisplayShowSelected, "cpClipStart=%d, cpClipFinish=%d fSelected:%d", cpClipStart, cpClipFinish, fSelected));

        RECT dbgRect;
        int i;
        for ( i = 0; i < aryInvalRects.Size(); i++)
        {
            dbgRect = aryInvalRects[i];
            TraceTag((tagDisplayShowInval,"InvalRect Left:%d Top:%d, Right:%d, Bottom:%d", dbgRect.left, dbgRect.top, dbgRect.right, dbgRect.bottom));
        }
#endif 

        pFlowLayout->Invalidate(&aryInvalRects[0], aryInvalRects.Size());           

    }


    //
    // The following bunch of code can be removed except that the RTL people need
    // to look at it for fixing RTL selection. Once they are done we should throw
    // this code away.
    //
#ifdef NEVER
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

    pFlowLayout->GetClientRect((CRect *)&rcView);       // Get view rectangle
    rcClip = rcView;

    // Rotate viewport in case of vertical control
    // ??? CF - This need to be re-worked and stuck in one place...
    if(pFlowLayout->_fVertical)
    {
        SetMapMode(hdc, MM_ANISOTROPIC);
        SetWindowExtEx(hdc, 1, 1, NULL );
        SetViewportExtEx(hdc, -1, 1, NULL);
        SetViewportOrgEx(hdc, _xWidth, 0, NULL);
    }

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
        {
            POINT pt;
            tp = cp;

            if(PointFromTp(tp, FALSE, FALSE, pt, &rp, TA_TOP, &CI, &fComplexLine) < 0)
            {
                fResult = FALSE;
                goto Cleanup;
            }
            if(!_fRTL)
                rc.left = fFirstCharacterSelected ? -1 : pt.x;
            else
                rc.right = fFirstCharacterSelected ? rcView.right + 1 : pt.x;
            rc.top = pt.y;
        }

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

                // NOTE: (paulnel) Before correcting this for Relative lines, be sure to 
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

                if (PointFromTp(tp, FALSE, FALSE, pt, NULL, TA_TOP, &CI) < 0)
                {
                    fResult = FALSE;
                    goto Cleanup;
                }
                if(!_fRTL)
                    rc.right = pt.x;
                else
                    rc.left = pt.x;
            }
            else
            {
                if(!_fRTL)
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

        pFlowLayout->Invalidate(&aryInvalRects[0], aryInvalRects.Size());
    }

    fResult = TRUE;

Cleanup:
    return fResult;

#endif // NEVER
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
    BOOL            fMinMax)
{
    long lPadding[PADDING_MAX];

    Assert(pxWidthParent && pyHeightParent);

    GetPadding(ppri, lPadding, fMinMax);

    *pxWidthParent  = GetViewWidth() -
                        lPadding[PADDING_LEFT] -
                        lPadding[PADDING_RIGHT];
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
    BOOL            fMinMax)
{
    CElement  * pElementFL   = GetFlowLayoutElement();
    CTreeNode * pNode        = pElementFL->GetFirstBranch();
    CDoc      * pDoc         = pElementFL->Doc();
    ELEMENT_TAG etag         = pElementFL->Tag();
    LONG        lFontHeight  = pNode->GetCharFormat()->GetHeightInTwips(pDoc);
    long        lParentWidth = fMinMax
                                ? ppri->_sizeParent.cx
                                : GetViewWidth();
    long        lPaddingTop, lPaddingLeft, lPaddingRight, lPaddingBottom;
    const CFancyFormat * pFF = pNode->GetFancyFormat();


    if( etag == ETAG_MARQUEE && !pElementFL->IsEditable() && !_fPrinting    )
    {
        lPaddingTop    = 
        lPaddingBottom = DYNCAST(CMarquee, pElementFL)->_lYMargin;
    }
    else
    {
        lPaddingTop    = 
        lPaddingBottom = 0;
    }

    if (etag != ETAG_TC)
    {
        lPaddingTop +=
            pFF->_cuvPaddingTop.YGetPixelValue(ppri, lParentWidth, lFontHeight);
                            // (srinib) parent width is intentional as per css spec

        lPaddingBottom +=
            pFF->_cuvPaddingBottom.YGetPixelValue(ppri, lParentWidth, lFontHeight);
                            // (srinib) parent width is intentional as per css spec

        lPaddingLeft =
            pFF->_cuvPaddingLeft.XGetPixelValue(ppri, lParentWidth, lFontHeight);


        lPaddingRight =
            pFF->_cuvPaddingRight.XGetPixelValue(ppri, lParentWidth, lFontHeight);

        if (etag == ETAG_BODY)
        {
            lPaddingLeft  +=
                pFF->_cuvMarginLeft.XGetPixelValue(ppri, lParentWidth, lFontHeight);
            lPaddingRight +=
                pFF->_cuvMarginRight.XGetPixelValue(ppri, lParentWidth, lFontHeight);
            lPaddingTop +=
                pFF->_cuvMarginTop.YGetPixelValue(ppri, lParentWidth, lFontHeight);
            lPaddingBottom +=
                pFF->_cuvMarginBottom.YGetPixelValue(ppri, lParentWidth, lFontHeight);
         }
    }
    else
    {
        lPaddingLeft = 0;
        lPaddingRight = 0; 
    }

    //
    // negative padding is not supported. What does it really mean?
    //
    lPadding[PADDING_TOP] = lPaddingTop > SHRT_MAX
                             ? SHRT_MAX
                             : lPaddingTop < 0 ? 0 : lPaddingTop;
    lPadding[PADDING_BOTTOM] = lPaddingBottom > SHRT_MAX
                             ? SHRT_MAX
                             : lPaddingBottom < 0 ? 0 : lPaddingBottom;
    lPadding[PADDING_LEFT] = lPaddingLeft > SHRT_MAX
                             ? SHRT_MAX
                             : lPaddingLeft < 0 ? 0 : lPaddingLeft;
    lPadding[PADDING_RIGHT] = lPaddingRight > SHRT_MAX
                             ? SHRT_MAX
                             : lPaddingRight < 0 ? 0 : lPaddingRight;

    _fContainsVertPercentAttr |= pFF->_fPercentVertPadding;
    _fContainsHorzPercentAttr |= pFF->_fPercentHorzPadding;
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
    CCcs     *pccs = fc().GetCcs (pci->_hdc, pci, pcf);
    CBaseCcs *pBaseCcs;

    Assert(pci && pTop && pBottom && pli && pcf);
    Assert (pccs);

    pBaseCcs = pccs->GetBaseCcs();
    
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
    *pTop = *pBottom - pBaseCcs->_yHeight - pBaseCcs->_yOffset -
            (pli->_yTxtDescent - pBaseCcs->_yDescent);
    
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

    if(!_fRTL)
    {
        prcClip->left   = xOrigin + pli->GetTextLeft();
        prcClip->right  = xOrigin + pli->GetTextRight();
    }
    else
    {
        prcClip->right  = xOrigin - pli->GetRTLTextRight();
        prcClip->left   = xOrigin - pli->GetRTLTextLeft();
    }
    if (pli->_fForceNewLine)
    {
        if (!pli->_fRTL)
        {
            prcClip->right += pli->_xWhite;
        }
        else
        {
            prcClip->left -= pli->_xWhite;
        }
    }
    prcClip->top    = top + pli->GetYTop();
    prcClip->bottom = top + pli->GetYBottom();
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

    // Walk up the tree as long as we're seeing "open tags w/o intervening
    // content".  Once we see some content, the vertical accumulation of
    // border and padding stops.
    while(    ptpCurr
          &&  !ptpCurr->IsText() )
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
        else if ( ptpCurr->IsEndNode() )
        {
            CTreeNode * pNode = ptpCurr->Branch();
            CElement  * pElement = pNode->Element();

            // The presence of no scape elements does not end a block of
            // "open tags w/o intervening content", because they don't
            // take up space in the rendering.  E.g.
            // <blockquote>
            //  <blockquote>
            //   <script>
            //        this script element doesn't interrupt the stack of
            //        open blockquotes w/o intervening content.
            //   </script>
            //    <blockquote>
            //
            // 
            if ( !pElement->IsNoScope() )
                break;
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

//+----------------------------------------------------------------------------
//
// Member:      GetWigglyFromRange
//
// Synopsis:    Gets rectangles to use for focus rects. This
//              element or range.
//
//-----------------------------------------------------------------------------

HRESULT
CDisplay::GetWigglyFromRange(CDocInfo * pdci, long cp, long cch, CShape ** ppShape)
{
    CStackDataAry<RECT, 8> aryWigglyRects(Mt(CDisplayGetWigglyFromRange_aryWigglyRect_pv));
    HRESULT         hr = S_FALSE;
    CWigglyShape *  pShape = NULL;
    CMarkup *       pMarkup = GetMarkup();
    long            ich, cRects;
    CTreePos *      ptp      = pMarkup->TreePosAtCp(cp, &ich);
    CTreeNode *     pNode    = ptp->GetBranch();
    CElement *      pElement = pNode->Element();

    if (!cch)
    {
        goto Cleanup;
    }

    pShape = new CWigglyShape;
    if (!pShape)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // get all of the rectangles that apply and load them into CWigglyShape's
    // array of CRectShapes.
    // RegionFromElement will give rects for any chunks within the number of
    // lines required.
    RegionFromElement( pElement, 
                       &aryWigglyRects, 
                       NULL, 
                       NULL, 
                       RFE_ELEMENT_RECT | RFE_IGNORE_RELATIVE | RFE_NESTED_REL_RECTS, 
                       cp,               // of lines that have different heights
                       cp + cch, 
                       NULL ); 

    for(cRects = 0; cRects < aryWigglyRects.Size(); cRects++)
    {
        CRectShape * pWiggly = NULL;

        pWiggly = new CRectShape;
        if (!pWiggly)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pWiggly->_rect = aryWigglyRects[cRects];

        // Expand the focus shape by shifting the left edges by 1 pixel.
        // This is a hack to get around the fact that the Windows
        // font rasterizer occasionally uses the leftmost pixel column of its
        // measured text area, and if we don't do this, the focus rect
        // overlaps that column.  IE bug #76378, taken for NT5 RTM (ARP).
        if ( pWiggly->_rect.left > 0 )
        {
            --(pWiggly->_rect.left);
        }

        pShape->_aryWiggly.Append(pWiggly);

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

