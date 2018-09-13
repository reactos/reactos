//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       keynav.cxx
//
//  Contents:   Implementation of some key navigation code
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X__RANGE_H_
#define X__RANGE_H_
#include "_range.h"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

//+----------------------------------------------------------------------------
//
//  Member:     MoveLineUpOrDown
//
//  Synopsis:   Given a line, will move one line up/down. If there are no lines in
//              the given direction, will find an appropriate line (if one exists)
//              in a containing txtsite.
//
//  Arguments:  [iDir]           : The dirn we are moving in
//              [rp]             : The line to navigate from
//              [xCaret]         : The desired x position of the caret
//              [pcp]            : The New cp position after line up done
//              [pfCaretNotAtBOL]: Is the caret at BOL?
//
//  Returns:    The txtsite where the new cp resides. Could be the same txt site.
//
//-----------------------------------------------------------------------------
CFlowLayout *
CDisplay::MoveLineUpOrDown(NAVIGATE_DIRECTION iDir, CLinePtr& rp, LONG xCaret, LONG *pcp, BOOL *pfCaretNotAtBOL)
{
    CFlowLayout *pFlowLayout = NULL;  // The new txtsite we navigate to
    POINT     pt;               // The point to navigate to

    // Setup the desired x position
    pt.x = xCaret;

    if (    (   iDir == NAVIGATE_UP
            &&  !IsTopLine(rp))
        ||  (   iDir == NAVIGATE_DOWN
            &&  !IsBottomLine(rp)))

    {
        CRect   rc;

        GetFlowLayout()->GetClientRect(&rc);
        // Find the Y position of the line geographically before/after us
        pt.y = YposFromLine(rp, NULL);
        if (iDir == NAVIGATE_UP)
            pt.y--;
        else
            pt.y += rp->GetYLineBottom();
            
        rc.MoveTo(pt);
        
        // Find that line
        rp = LineFromPos(rc, (LFP_ZORDERSEARCH | LFP_IGNOREALIGNED | LFP_IGNORERELATIVE));

        if (rp >= 0)
        {
            // The line we want to navigate is found. Now navigate
            // to that line
            pFlowLayout = NavigateToLine(iDir, rp, pt, pcp, pfCaretNotAtBOL);
        }
    }

// BUGBUG: This should send the request via a notification? (brendand)
    if (!pFlowLayout)
        pFlowLayout = GetFlowLayout()->GetNextFlowLayout(iDir, pt, NULL, pcp, pfCaretNotAtBOL);

    // Return the flowlayout we ended up in
    return pFlowLayout;
}

//+----------------------------------------------------------------------------
//
//  Member:     NavigateToLine
//
//  Synopsis:   Given a line, we will first check if it has any nested sites
//              where a caret could live. If not, then we will place the caret
//              at the desired X position.
//
//  Arguments:  [iDir]           : The dirn we are moving in
//              [rp]             : The line to navigate to
//              [pt]             : The desired position of the caret
//              [pcp]            : The New cp position after line up done
//              [pfCaretNotAtBOL]: Is the caret at BOL?
//
//  Returns:    The txtsite where the new cp resides. Could be the same txt site.
//
//-----------------------------------------------------------------------------
CFlowLayout *
CDisplay::NavigateToLine(NAVIGATE_DIRECTION iDir, CLinePtr& rp, POINT pt, LONG *pcp, BOOL *pfCaretNotAtBOL)
{
    CElement    *pElementFL  = GetFlowLayoutElement();
    CFlowLayout *pFlowLayout = NULL;            // The new flowlayout we want to navigate to
    CTreeNode   *pNodeLayout;                   // The layout element (if any) within the line
    CTreeNode   *pNodeElem;
    CLayout     *pLayout;

    // Find the site in the beneath it (if it exists).
    if (LineCount() != 0 && rp->_fHasNestedRunOwner)
    {
        HTC htc; // The hit test code
        CMessage msg;

        msg.pt = pt;
        // Find out the site which was within this line
        htc = GetFlowLayout()->Doc()->HitTestPoint(&msg, &pNodeElem,
                                 HT_IGNORESCROLL | HT_VIRTUALHITTEST);
        Assert (HTC_NO != htc);
        pLayout = pNodeElem->GetNearestLayout();

        if( pLayout == NULL )
            return NULL;

        pNodeLayout = pLayout->GetFirstBranch();
    }
    else
    {
        pLayout = GetFlowLayout();
        pNodeLayout = pElementFL->GetFirstBranch();
    }

    Assert(pLayout);

    // If a site other than the site containing this line is found AND it
    // belongs to the same ped as this, then that is the site we want to
    // navigate to.
    if (DifferentScope(pNodeLayout, pElementFL) &&
        pNodeLayout->GetContainer() == pElementFL->GetFirstBranch()->GetContainer() )
    {
        // Be sure that the point is *within* that site's *client* rect
        pLayout->RestrictPointToClientRect(&pt);
        // And find out the nearest flowlayout within that layout.
        pFlowLayout = pLayout->GetFlowLayoutAtPoint(pt);
    }

    if(!pFlowLayout)
        pFlowLayout = GetFlowLayout();

    if (pFlowLayout != GetFlowLayout())
    {
        pFlowLayout = pFlowLayout->GetPositionInFlowLayout(iDir, pt, pcp, pfCaretNotAtBOL);
    }
    else
    {
        // There are no nested txtsites in this line. So lets position ourselves
        // in the correct position and return ourself as the text site.
        CLinePtr   rpNew(this);     // The line we end up in.
        TCHAR      ch;
        CTxtPtr    tp(GetMarkup());

        // Found the txtsite, so lets get the cp
        *pcp = CpFromPointReally(pt, &rpNew, NULL, TRUE, FALSE);

        // In any line, we donot want to end up after a line/block/textsite break
        // characters. Be sure of that ... however, also be careful to stay in
        // the same line.
        tp.SetCp(*pcp);
        ch = tp.GetPrevChar();
        while(   rpNew.GetIch()
              && (   IsTxtSiteBreak(ch)
                  || IsSyntheticBlockBreakChar(ch)
                  || IsSyntheticBreakChar(ch)
                  || IsASCIIEOP(ch)
                 )
             )
        {
            if (!tp.AdvanceCp(-1))
                break;
            ch = tp.GetPrevChar();
            rpNew.AdvanceCp(-1);
        }

        // Finally got our cp
        *pcp = tp.GetCp();

        // and the start of BOL'ness
        *pfCaretNotAtBOL = rpNew.GetIch() != 0;
    }

    return pFlowLayout;
}

//+----------------------------------------------------------------------------
//
//  Member:     IsTopLine
//
//  Synopsis:   Given a line, check if it is geographically the top line
//
//  Arguments:  [rp]: The line to check
//
//-----------------------------------------------------------------------------
BOOL
CDisplay::IsTopLine(CLinePtr& rp)
{
    CLinePtr rpTravel(rp);

    while(rpTravel.PrevLine(TRUE, TRUE))
    {
        // If there is a line before this one which forces a new line,
        // then this line is not the top one.
        if (rpTravel->_fForceNewLine)
            return FALSE;
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Member:     IsBottomLine
//
//  Synopsis:   Given a line, check if it is geographically the bototm line
//
//  Arguments:  [rp]: The line to check
//
//
//  Legend:
//
//  ====== the line pointed to by rp
//  ------ the lines after it
//  -----> indicates that the line forces a new line
//
//  This function returns TRUE in all these cases:
//
//  1) ======== ---------->
//  2) ======== ----------
//  3) ========>
//  4) ========
//
//  And it returns FALSE in all these cases
//  5) ======== ---------->
//     --------
//  6) ======== ---------->
//     -------->
//  7) ========>
//     --------
//-----------------------------------------------------------------------------
BOOL
CDisplay::IsBottomLine(CLinePtr& rp)
{
    CLinePtr rpTravel(rp);          // Line ptr used to traverse the line array
    LONG     cNewLinesSeen = 0;     // How many lines having force new line seen?
    BOOL     fLastWasForceNewLine;  // The last line we saw was a force-new-line line

    do
    {
        // Account for this line
        cNewLinesSeen += rpTravel->_fForceNewLine ? 1 : 0;
        fLastWasForceNewLine = !!rpTravel->_fForceNewLine;

        // We cannot decide whether we are the last line when we just
        // see one line having the _fForceNewLine bit set. This would
        // break case 1 and 3 above. If we see atleast 2 lines with
        // this bit then we are sure that this line is not the last line
        // and can return FALSE (case 6)
        if (cNewLinesSeen == 2)
            return FALSE;

    } while(rpTravel.NextLine(TRUE, TRUE));

    // If we have seen only one new line and the last line we saw
    // did not force a new line then we have case 5/7, where we have
    // to return FALSE.
    if (cNewLinesSeen && !fLastWasForceNewLine)
        return FALSE;

    // case     cNewLinesSeen  fLastWasForceNewLine
    // 1        1              TRUE
    // 2        0              FALSE
    // 3        1              TRUE
    // 4        0              TRUE
    return TRUE;
}
