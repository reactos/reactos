//+---------------------------------------------------------------------
//
//  File:       wutils.cxx
//
//  Contents:   Windows helper functions
//
//----------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

DeclareTagOther(tagCombineRects,    "Combine Rects",        "CombineRects status")

//+----------------------------------------------------------------------------
//
//  Function:   CombineRectsAggressive
//
//  Synopsis:   Given an array of non-overlapping rectangles sorted top-to-bottom,
//              left-to-right combine them agressively, where there may
//              be some extra area create (but not too much).
//
//  Arguments:  pcRects     - Number of RECTs passed
//              arc         - Array of RECTs
//
//  Returns:    pcRects - Count of combined RECTs
//              arc     - Array of combined RECTs
//
//  NOTE: 1) RECTs must be sorted top-to-bottom, left-to-right and cannot be overlapping
//        2) arc is not reduced in size, the unused entries at the end are ignored
//
//-----------------------------------------------------------------------------
void
CombineRectsAggressive(int *pcRects, RECT *arc)
{
    int cRects;
    int iDest, iSrc=0;
    int cTemp=0;
    int aryTemp[MAX_INVAL_RECTS]; // touching rects to the right and bottom of rect iDest
    int aryWhichComb[MAX_INVAL_RECTS];  // index matches arc, value is which 
                                        // combined rect the invalid rect belongs
                                        // to. Zero = not set yet.
    int cCombined, marker, index, index2;
    int areaCombined, aryArea[MAX_INVAL_RECTS];
    RECT arcCombined[MAX_INVAL_RECTS];      // accumulates combined rects from arc
    
    
    Assert(pcRects);
    Assert(arc);
    
    // If there are no RECTs to combine, return immediately
    if (*pcRects <= 1)
        return;
    
#if DBG==1
    int areaOrig=0, areaNew=0;
    if ( IsTagEnabled(tagCombineRects) )
    {
        for (iDest=0; iDest < *pcRects; iDest++)
        {
            areaOrig += (arc[iDest].right - arc[iDest].left) * 
                        (arc[iDest].bottom - arc[iDest].top);
        }
    }
#endif // DBG==1
    
    // Combine rects aggressively. Touching rects are combined together,
    // which may make our final regions include areas that wasn't in 
    // the original list of rects. 
    memset( aryWhichComb, 0, sizeof(aryWhichComb) ); 
    memset( aryArea, 0, sizeof(aryArea) );
    cCombined = 0;
    for (iDest=0, cRects = *pcRects; iDest < cRects; iDest++)
    {
        // Combine abutting rects. Iterate through the array of rects 
        // (arranged from top-to-bottom, left-to-right) and enumerate 
        // those that touch to the right and bottom. Since this misses
        // those touching to the right and upwards we keep a list of 
        // which original rect belongs to which rect that is going to be 
        // passed back. As touching rects are accumulated, we check to 
        // see if any of these already belong to a rect that is going to 
        // be passed back. If it is, we add the ones currently being 
        // accumulated to that one.
        cTemp = 0;
        marker = cCombined + 1;
        aryTemp[cTemp++] = iDest;
        if ( aryWhichComb[iDest] > 0 )
            marker = aryWhichComb[iDest];
        
        for ( iSrc=iDest+1; 
        arc[iDest].bottom > arc[iSrc].top && 
            iSrc < cRects; iSrc++ )
        {
            Assert(arc[iDest].right <= arc[iSrc].left);
            if ( arc[iDest].right == arc[iSrc].left )
            {
                // I don't think this ever happens. but just in case it 
                // does this will do the right thing
                Assert( 0 && "Horizontal abutting invalid rects" );
                if ( aryWhichComb[iSrc] > 0 )
                    marker = aryWhichComb[iSrc];
                aryTemp[cTemp++] = iSrc;
            }
        }
        
        // Check rects below for abuttment. 
        for ( ; arc[iDest].bottom == arc[iSrc].top && 
            arc[iDest].right > arc[iSrc].left && 
            iSrc < cRects; iSrc++ )
        {
            if ( arc[iDest].left < arc[iSrc].right )
            {
                if ( aryWhichComb[iSrc] > 0 )
                    marker = aryWhichComb[iSrc];
                aryTemp[cTemp++] = iSrc;
            }
        }
        
        if ( cCombined + 1 == marker )
        {
            // this group of invalid rects doesn't combine with any 
            // existing rect that is going to be returned. Start a one
            arcCombined[cCombined] = arc[iDest];
            cCombined++;
        }
        
        // Add all rects accumulated on this pass to the rect that will be passed back.
        index = marker - 1;
        while ( --cTemp >= 0 )
        {
            index2 = aryTemp[cTemp];
            if ( aryWhichComb[index2] != marker )
            {
                aryWhichComb[index2] = marker;
                arcCombined[index].left    = min( arcCombined[index].left,   arc[index2].left );
                arcCombined[index].top     = min( arcCombined[index].top,    arc[index2].top );
                arcCombined[index].right   = max( arcCombined[index].right,  arc[index2].right );
                arcCombined[index].bottom  = max( arcCombined[index].bottom, arc[index2].bottom );
                aryArea[index] += (arc[index2].right - arc[index2].left) * 
                                  (arc[index2].bottom - arc[index2].top);
            }
        }
    }
    
    // check to make sure each rect meets the fitness criteria
    // don't want to be creating excessively large non-invalid
    // regions to draw
    cRects = cCombined;
    for ( index=0, marker=1; index<cCombined; index++, marker++ )
    {
        areaCombined = (arcCombined[index].right - arcCombined[index].left) *
                       (arcCombined[index].bottom - arcCombined[index].top);
        if ( areaCombined > 1000 && areaCombined > 3*aryArea[index] )
        {
            // scrap combined rect and fall back on just combining
            // areas that will not create any extra space.
#if DBG==1
            TraceTag((tagCombineRects, "EXTRA AREA TOO BIG! Inval area:%d, Extra Area:%d", 
                     aryArea[index], areaCombined-aryArea[index]));
#endif
            int index3=cRects, count=0;
            for ( index2=0; index2<*pcRects; index2++ )
            {
                if ( marker == aryWhichComb[index2] )
                {
                    arcCombined[index3++] = arc[index2];
                    count++;
                }
            }
#if DBG==1
            TraceTag((tagCombineRects, "Sending in %d rects.", count ));
#endif
            CombineRects( &count, &(arcCombined[cRects]));
#if DBG==1
            TraceTag((tagCombineRects, "Got back %d rects.", count ));
#endif
            cRects += count-1;
            memmove( &arcCombined[index], &arcCombined[index+1], 
                     (cRects-index)*sizeof(arcCombined[0]) );
            memmove( &aryArea[index], &aryArea[index+1], 
                     (cRects-index)*sizeof(aryArea[0]) );
            cCombined--;
            index--;
        }
    }
    
    
    // Weed out rects that may now be totally enclosed by the extra 
    // region gain in combining rects.
    for ( iSrc=cRects-1; iSrc>=0; iSrc-- )
    {   
        for ( iDest=0; iDest<cRects; iDest++ )
        {
            if ( arcCombined[iSrc].left   >= arcCombined[iDest].left  &&
                 arcCombined[iSrc].top    >= arcCombined[iDest].top   &&
                 arcCombined[iSrc].right  <= arcCombined[iDest].right &&
                 arcCombined[iSrc].bottom <= arcCombined[iDest].bottom &&
                 iDest != iSrc )
            {
#if DBG==1
                TraceTag((tagCombineRects, "Weeding out rect %d (total rects=%d)", 
                         iSrc, cRects ));
#endif
                memmove( &(arcCombined[iSrc]), &(arcCombined[iSrc+1]), 
                        (cRects-1-iSrc)*sizeof(arcCombined[0]) );
                cRects--;
                break;
            }
        }
    }
    // set output vars
    memmove( arc, arcCombined, cRects*sizeof(arc[0]) );
    *pcRects = cRects;
    

#if DBG==1
        if ( IsTagEnabled(tagCombineRects) )
        {
            // statistics for combining rects
            for (iDest=0; iDest < cRects; iDest++)
            {
                areaNew += (arc[iDest].right - arc[iDest].left) * (arc[iDest].bottom - arc[iDest].top);
            }
            TraceTag((tagCombineRects, "Previous # rects:%d, New # rects:%d", *pcRects, cRects ));
            TraceTag((tagCombineRects, "Excess drawing area: %d pixels, %d %% growth in the region", 
                                        areaNew-areaOrig, (100*(areaNew-areaOrig))/areaOrig));
            if ( areaNew-areaOrig < 0 )
                TraceTag((tagCombineRects, "Didn't draw something we should have??"));
        }
#endif // DBG==1

    return;
}

//+----------------------------------------------------------------------------
//
//  Function:   CombineRects
//
//  Synopsis:   Given an array of non-overlapping rectangles sorted top-to-bottom,
//              left-to-right combine them such that they create no extra area.
//
//  Arguments:  pcRects     - Number of RECTs passed
//              arc         - Array of RECTs
//
//  Returns:    pcRects - Count of combined RECTs
//              arc     - Array of combined RECTs
//
//  NOTE: 1) RECTs must be sorted top-to-bottom, left-to-right and cannot be overlapping
//        2) arc is not reduced in size, the unused entries at the end are ignored
//
//-----------------------------------------------------------------------------
void
CombineRects(int *pcRects, RECT *arc)
{
    int cRects;
    int iDest, iSrc=0;


    Assert(pcRects);
    Assert(arc);

    // If there are no RECTs to combine, return immediately
    if (*pcRects <= 1)
        return;

    // Combine RECTs of similar shape with adjoining boundaries
    for (iDest=0, cRects=*pcRects-1; iDest < cRects; iDest++)
    {
        // First, combine left-to-right those RECTs with the same top and bottom
        // (Since the array is sorted top-to-bottom, left-to-right, adjoining RECTs
        //  with the same top and bottom will be contiguous in the array. As a result,
        //  the loop only needs to continue looking at elements until one is found whose
        //  top or bottom coordinates do not match, or whose left edge is not adjoining.)
        for (iSrc=iDest+1;
            iSrc <= cRects && arc[iDest].top    == arc[iSrc].top    &&
                              arc[iDest].bottom == arc[iSrc].bottom &&
                              arc[iDest].right  >= arc[iSrc].left;
            iSrc++)
        {
            arc[iDest].right = arc[iSrc].right;
        }

        // If RECTs were combined, shift those remaining downward and adjust the total count
        if ((iSrc-1) > iDest)
        {
            cRects -= iSrc - iDest - 1;
            memmove(&arc[iDest+1], &arc[iSrc], cRects*sizeof(arc[0]));
        }

        // Next, combine top-to-bottom those RECTs whose bottoms and tops meet
        // (Again, since the array is sorted top-to-bottom, left-to-right, RECTs which share
        //  the left and right coordinates and touch bottom-to-top may not be next to one
        //  another in the array. The loop must scan until it founds an element whose top
        //  or left edge exceeds that of the destination RECT. It will skip elements whose
        //  tops occur above the bottom of the destination or which occur left of the
        //  destination. It will combine elements, one at a time, which touch bottom-to-top
        //  and have matching left/right coordinates.)
        for (iSrc=iDest+1; iSrc <= cRects; )
        {
            if (arc[iDest].bottom < arc[iSrc].top)
                break;

            else if (arc[iDest].bottom == arc[iSrc].top)
            {
                if (arc[iDest].left < arc[iSrc].left)
                    break;

                else if (arc[iDest].left  == arc[iSrc].left &&
                         arc[iDest].right == arc[iSrc].right)
                {
                    arc[iDest].bottom = arc[iSrc].bottom;
                    memmove(&arc[iSrc], &arc[iSrc+1], (cRects-iSrc)*sizeof(arc[0]));
                    cRects--;
                    continue;
                }
            }

            iSrc++;
        }
    }

    // Adjust the returned number RECTs
    *pcRects = cRects + 1;
    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   BoundingRectl
//
//  Synopsis:   Returns a rectangle that bounds the two rectangled.  Unlike UnionRectl,
//              this function correctly computes a bounding rectangle when one of the
//              rectangles is just a point (has 0 width and height).
//
//  Arguments:  [prclDst]   - resulting RECTL
//              [prclSrc1]  - input RECTL
//              [prclSrc2]  - input RECTL
//
//  Returns:    TRUE if the result is not an empty RECTL
//              FALSE if the result is an empty RECTL
//----------------------------------------------------------------------------

BOOL BoundingRectl(RECTL *prclDst, const RECTL *prclSrc1, const RECTL *prclSrc2)
{
    prclDst->left = min(prclSrc1->left, prclSrc2->left);
    prclDst->top = min(prclSrc1->top, prclSrc2->top);
    prclDst->right = max(prclSrc1->right, prclSrc2->right);
    prclDst->bottom = max(prclSrc1->bottom, prclSrc2->bottom);
    return prclDst->right - prclDst->left + prclDst->bottom - prclDst->top;
}

//+---------------------------------------------------------------------------
//
//  Function:   NextEventTime
//
//  Synopsis:   Returns a value which can be use to determine when a given
//              number of milliseconds has passed.
//
//  Arguments:  [ulDelta] -- Number of milliseconds after which IsTimePassed
//                           will return TRUE.
//
//  Returns:    A timer value.  Guaranteed not to be zero.
//
//  Notes:      Due to the algorithm used in IsTimePassed, [ulDelta] cannot
//              be greater than ULONG_MAX/2.
//
//----------------------------------------------------------------------------

ULONG
NextEventTime(ULONG ulDelta)
{
    ULONG ulCur;
    ULONG ulRet;

    Assert(ulDelta < ULONG_MAX/2);

    ulCur = GetTickCount();

    if ((ULONG_MAX - ulCur) < ulDelta)
        ulRet = ulDelta - (ULONG_MAX - ulCur);
    else
        ulRet = ulCur + ulDelta;

    if (ulRet == 0)
        ulRet = 1;

    return ulRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsTimePassed
//
//  Synopsis:   Returns TRUE if the current time is later than the given time.
//
//  Arguments:  [ulTime] -- Timer value returned from NextEventTime().
//
//  Returns:    TRUE if the current time is later than the given time.
//
//----------------------------------------------------------------------------

BOOL
IsTimePassed(ULONG ulTime)
{
    ULONG ulCur = GetTickCount();

    if ((ulCur > ulTime) && (ulCur - ulTime < ULONG_MAX/2))
        return TRUE;

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CalcScrollDelta
//
//  Synopsis:   Calculates the distance needed to scroll to make one
//              rectangle visible in another.
//
//  Arguments:  prcOuter    Outer (clipping) rectangle (determines visibility)
//              prcInner    Inner rectangle which needs to be visible
//              sizeGutter  Extra slop to use when scrolling
//              psizeScroll Amount to scroll
//              psizeOffset Offset in outer of inner after scrolling
//                          (always calculated, even if scrolling is not needed)
//              vp, hp      Where to "pin" inner RECT within the outer RECT
//
//  Returns:    TRUE if scrolling required.
//
//----------------------------------------------------------------------------

BOOL
CalcScrollDelta(
        RECT *      prcOuter,
        RECT *      prcInner,
        SIZE        sizeGutter,
        SIZE *      psizeScroll,
        SIZE *      psizeOffset,
        SCROLLPIN   spVert,
        SCROLLPIN   spHorz)
{
    int         i;
    int         cxLeft;
    int         cxRight;
    SIZE        sizeOffset;
    SCROLLPIN   sp;

    Assert(prcOuter);
    Assert(prcInner);
    Assert(psizeScroll);

    if (!psizeOffset)
    {
        psizeOffset = &sizeOffset;
    }

    if (prcOuter->left   <= prcInner->left   &&
        prcOuter->right  >= prcInner->right  &&
        prcOuter->top    <= prcInner->top    &&
        prcOuter->bottom >= prcInner->bottom &&
        spVert == SP_MINIMAL                 &&
        spHorz == SP_MINIMAL)
    {
        psizeScroll->cx =
        psizeScroll->cy = 0;
        psizeOffset->cx = prcInner->left - prcOuter->left - sizeGutter.cx;
        psizeOffset->cy = prcInner->top  - prcOuter->top  - sizeGutter.cy;
        return FALSE;
    }

    sp = spHorz;
    for (i = 0; i < 2; i++)
    {
        // Calculate amount necessary to "pin" the left edge
        cxLeft = (&prcInner->left)[i] -
                 (&prcOuter->left)[i] -
                 (&sizeGutter.cx)[i];
        (&sizeOffset.cx)[i] = 0;

        // Examine right edge only if not "pin"ing to the left
        if (sp != SP_TOPLEFT)
        {
            Assert(sp == SP_BOTTOMRIGHT || sp == SP_MINIMAL);

            cxRight = (&prcOuter->right)[i] -
                      (&sizeGutter.cx)[i]   -
                      (&prcInner->right)[i];

            // "Pin" the inner RECT to the right side of the outer RECT
            if (sp == SP_BOTTOMRIGHT)
            {
                (&sizeOffset.cx)[i] = ((&prcOuter->right)[i] - (&prcOuter->left)[i]) -
                                      ((&prcInner->right)[i] - (&prcInner->left)[i]);
                if ((&sizeOffset.cx)[i] < 0)
                {
                    (&sizeOffset.cx)[i] = 0;
                }
                cxLeft = -cxRight;
            }

            // Otherwise, move the minimal amount necessary to make the
            // inner RECT visible within the outer RECT
            // (This code will try to make the entire inner RECT visible
            //  and gives preference to the left edge)
            else if (cxLeft > 0)
            {
                if (cxRight >= 0)
                {
                    (&sizeOffset.cx)[i] = cxLeft;
                    cxLeft = 0;
                }
                else if (-cxRight < cxLeft)
                {
                    (&sizeOffset.cx)[i] = cxLeft + cxRight;
                    cxLeft = -cxRight;
                }
            }
        }
        (&psizeScroll->cx)[i] = cxLeft;
        sp = spVert;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CalcInsetScroll
//
//  Synopsis:   Compute amount to scroll based on position of point in
//              scrolling inset.   Return TRUE if amount to scroll is
//              non-zero, FALSE otherwise.
//
//  Arguments:  prcl        The rectangle to check.
//              sizeGutter  The gutter.
//              ptl         The point to check.
//              psizeScroll The amount to scroll.
//
//  Returns:    True if should scroll
//
//  Notes:      The values returned in sizel will get larger as the
//              point given in [ppt] gets farther into the inset region (closer
//              to the edge of the window.)
//
//              If ptl is outside the rectangle, this method will return TRUE
//              and the scroll values will be equivalent to the values given
//              when the point is at the very outer edge of the inset region.
//
//----------------------------------------------------------------------------

BOOL
CalcInsetScroll(
    RECT *  prc,
    SIZE    sizeGutter,
    POINT   pt,
    SIZES  * psizeScroll)
{
    RECT rc = *prc;
    int   i;

    // Determines how quickly the scrolling speeds up as the user moves the
    // cursor farther into the inset region.
    const int SCROLL_SCALE_FACTOR = 1;

    psizeScroll->cx = psizeScroll->cy = 0;

    InflateRect(&rc, -sizeGutter.cx, -sizeGutter.cy);

    for (i = 0; i < 2; i++)
    {
        if ((&pt.x)[i] > (&rc.right)[i])
        {
            (&psizeScroll->cx)[i] = min((&pt.x)[i] - (&rc.right)[i],
                    (&sizeGutter.cx)[i]) * SCROLL_SCALE_FACTOR;
        }
        else if ((&pt.x)[i] < (&rc.left)[i])
        {
            (&psizeScroll->cx)[i] = -(min((&rc.left)[i] - (&pt.x)[i],
                    (&sizeGutter.cx)[i]) * SCROLL_SCALE_FACTOR);
        }
    }

    return (psizeScroll->cy != 0) || (psizeScroll->cx != 0);
}


//+------------------------------------------------------------------------
//
//  Member:     CCurs::CCurs
//
//  Synopsis:   Constructor.  Loads the specified cursor and pushes it
//              on top of the cursor stack.  If the cursor id matches a
//              standard Windows cursor, then the cursor is loaded from
//              the system.  Otherwise, the cursor is loaded from this
//              instance.
//
//  Arguments:  idr - The resource id
//
//-------------------------------------------------------------------------
CCurs::CCurs(LPCTSTR idr)
{
    _hcrsOld = SetCursorIDC(idr);
    _hcrs    = GetCursor();
}



//+------------------------------------------------------------------------
//
//  Member:     CCurs::~CCurs
//
//  Synopsis:   Destructor.  Pops the cursor specified in the constructor
//              off the top of the cursor stack.  If the active cursor has
//              changed in the meantime, through some other mechanism, then
//              the old cursor is not restored.
//
//-------------------------------------------------------------------------
CCurs::~CCurs( )
{
    if (GetCursor() == _hcrs)
    {
        ShowCursor(FALSE);
        SetCursor(_hcrsOld);
        ShowCursor(TRUE);
    }
}



//+------------------------------------------------------------------------
//
//  Function:   SetCursorIDC
//
//  Synopsis:   Set the cursor.  If the cursor id matches a standard
//              Windows cursor, then the cursor is loaded from the system.
//              Otherwise, the cursor is loaded from this instance.
//
//  Arguments:  idr - IDC_xxx from Windows.
//
//-------------------------------------------------------------------------

// This is done to avoid messiness of changing WINVER to 0x0500
#ifndef IDC_HAND
#define IDC_HAND            MAKEINTRESOURCE(32649)
#endif

HCURSOR
SetCursorIDC(LPCTSTR idr)
{
    HCURSOR hcursor, hcurRet;

    // No support for loading cursor by string id.
    Assert(IS_INTRESOURCE(idr));

#ifndef WIN16
    // If IDC_HYPERLINK is specified, lets use the windows
    // hand cursor in NT5 and memphis (44417)
    if(g_dwPlatformVersion >= 0x050000 &&
       idr == MAKEINTRESOURCE(IDC_HYPERLINK))
    {
        idr = IDC_HAND;
    }
#endif

    // We assume that if it's greater than IDC_ARROW,
    // then it's a system cursor.

    hcursor = LoadCursorA(
        // IEUNIX BUGBUG (DWORD)idr should be changed to remove the high order 
        // WORD as for some unix platforms this may not be 0.
        ((DWORD_PTR)idr >= (DWORD_PTR)IDC_ARROW) ? NULL : g_hInstCore,
        (char *)idr);
    // NOTE (lmollico): cursors are in mshtml.dll

    // BUGBUG: (jbeda) If we were looking for IDC_HAND and didn't get it, 
    // try for IDC_HYPERLINK.  This shouldn't be necessary once memphis
    // implements the hand cursor
    if(!hcursor && idr == IDC_HAND)
    {
        idr = MAKEINTRESOURCE(IDC_HYPERLINK);
        hcursor = LoadCursorA(g_hInstCore, (char *)idr);
        // NOTE (lmollico): cursors are in mshtml.dll
    }

    Assert(hcursor && "Failed to load cursor");

    hcurRet = GetCursor();
    if (   hcursor
        && hcurRet != hcursor
       )
    {
        // BUGBUG(sujalp): The windows SetCursor() call here has an *ugly* flash
        // in the incorrect position when the cursor's are changing. (Bug29467).
        // (This might be related to windows first showing the new cursor and then
        // setting its hotspot). To avoid this flash, we hide the cursor just
        // before changing the cursor and we enable it after changing the cursor.
        ShowCursor(FALSE);
        hcurRet = SetCursor(hcursor);
        ShowCursor(TRUE);
    }

    return hcurRet;
}


//  BUGBUG: This routine will be needed when Win95 fixes their bug
//          which prevents us from using LoadLibraryEx(.....,AS_DATAFILE)
//          to load the resource DLL.

#if NEVER
//+---------------------------------------------------------------
//
//  Function:   FormsCreateDialogParam
//
//  Synopsis:   Wraps the Windows API for CreateDialogParam, accounts for
//              the code/resource DLL split.
//
//
//  Arguments:  [hinst] -- instance of the module with the resource
//              [lpstrId] -- the identifier of the RCDATA resource
//              [hWndParent] -- hwnd of the parent window of the dialog box
//              [lpDialogFunc] -- the dialog proc
//              [dwInitParam] -- the initialization param to be passed to WM_DIALOGINIT
//
//  Returns:    the hwnd of the dialog box if successful, NULL otherwise
//
//  Notes:      This is an experimental implementation, it tosses the global
//              handle to the template immediately.
//
//----------------------------------------------------------------

HWND FormsCreateDialogParam(HINSTANCE hInstResource,
                            LPCTSTR  lpstrId,
                            HWND  hWndParent,
                            DLGPROC  lpDialogFunc,
                            LPARAM  dwInitParam)
{
    LPCDLGTEMPLATE  lpTemplate;
    HWND hwnd;

    lpTemplate = (LPCDLGTEMPLATE)GetResource(hInstResource,lpstrId,RT_DIALOG,NULL);
    hwnd = CreateDialogIndirectParam(g_hInstCore, lpTemplate,hWndParent, lpDialogFunc,  dwInitParam);

    return hwnd;
}


//+---------------------------------------------------------------
//
//  Function:   GetChildWindowRect
//
//  Synopsis:   Gets the rectangle of the child window in
//              its parent window coordinates
//
//  Arguments:  hwndChild   The child window
//              prc         The rectangle to fill with child's coordinates
//
//  Notes:      This function gets the screen coordinates of the child
//              then maps them into the client coordinates of its parent.
//
//----------------------------------------------------------------

void
GetChildWindowRect(HWND hwndChild, LPRECT prc)
{
    HWND hwndParent;

    // get the screen coordinates of the child window
    GetWindowRect(hwndChild, prc);

    // get the parent window of the child
    if ((hwndParent = GetParent(hwndChild)) != NULL)
    {
        ScreenToClient(hwndParent, (POINT *)&prc->left);
        ScreenToClient(hwndParent, (POINT *)&prc->right);
    }
}
#endif


//+---------------------------------------------------------------------------
//
//  Function:   VBShiftState
//
//  Synopsis:   Helper function, returns shift state for KeyDown/KeyUp events
//
//  Arguments:  (none)
//
//  Returns:    USHORT
//
//  Notes:      This function maps the keystate supplied by Windows to
//              1, 2 and 4 (which are numbers from VB)
//
//----------------------------------------------------------------------------

short
VBShiftState()
{
    short sKeyState = 0;

    if (GetKeyState(VK_SHIFT) & 0x8000)
        sKeyState |= VB_SHIFT;

    if (GetKeyState(VK_CONTROL) & 0x8000)
        sKeyState |= VB_CONTROL;

    if (GetKeyState(VK_MENU) & 0x8000)
        sKeyState |= VB_ALT;

    return sKeyState;
}

short
VBShiftState(DWORD grfKeyState)
{
    short sKeyState = 0;

    if (grfKeyState & MK_SHIFT)
        sKeyState |= VB_SHIFT;

    if (grfKeyState & MK_CONTROL)
        sKeyState |= VB_CONTROL;

    if (grfKeyState & MK_ALT)
        sKeyState |= VB_ALT;

    return sKeyState;
}

//+---------------------------------------------------------------------------
//
//  Function:   VBButtonState
//
//  Synopsis:   Helper function, returns button state for Mouse events
//
//  Arguments:  w -- word containing mouse ButtonState
//
//  Returns:    short
//
//  Notes:      This function maps the buttonstate supplied by Windows to
//              1, 2 and 4 (which are numbers from VB)
//
//----------------------------------------------------------------------------

short
VBButtonState(WPARAM w)
{
    short sButtonState = 0;

    if (w & MK_LBUTTON)
        sButtonState |= VB_LBUTTON;
    if (w & MK_RBUTTON)
        sButtonState |= VB_RBUTTON;
    if (w & MK_MBUTTON)
        sButtonState |= VB_MBUTTON;

    return sButtonState;
}

//+------------------------------------------------------------------------
//
//  Function:   UpdateChildTree
//
//  Synopsis:   Calls UpdateWindow for a window, then recursively calls
//              UpdateWindow its children.
//
//  Arguments:  [hWnd]      Window to update, along with its children
//
//-------------------------------------------------------------------------
void
UpdateChildTree(HWND hWnd)
{
    //
    // The RedrawWindow call seems to miss the hWnd you actually pass in, or
    // else doesn't validate the update region after it has been redrawn, thus
    // the need for the UpdateWindow() call.
    //
    if (hWnd)
    {
        UpdateWindow(hWnd);
        RedrawWindow(hWnd,
                     (GDIRECT *)NULL,
                     NULL,
                     RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_INTERNALPAINT
                    );
    }
}


#ifdef NEVER
//+------------------------------------------------------------------------
//
//  Function:  InClientArea
//
//  Synopsis:   Checks if point is within a hWnd's client area.
//
//  Arguments:  [hWnd]      Window handle
//              [pt]        a point
//
//-------------------------------------------------------------------------

BOOL
InClientArea(POINTL pt, HWND hWnd)
{
    RECT rc;
    ::GetClientRect(hWnd, &rc);

    ClientToScreen(hWnd, (POINT *) &rc.left);
    ClientToScreen(hWnd, (POINT *) &rc.right);

    return ::PtInRect(&rc,  (POINT&) pt);

}


//+----------------------------------------------------------------------------
//
//  Function:   IsWindowActive
//
//  Synopsis:   Determines if an HWND is active or not
//
//              The window is considered active when the following conditions
//              are met:
//                  a) The HWND is the active window for the thread or
//                     is a child of the active window
//                     (If another thread owns the active window, then
//                      GetActiveWindow on this thread will return NULL)
//                  b) The HWND has focus or is a child of the window
//                     which has focus
//                     (Again, if window with focus resides on another thread
//                      this GetFocus will return NULL)
//
//              These conditions are the closest analogy to what occurs when a
//              frame window receives either WM_ACTIVATE or WM_MDIACTIVATE
//
//  Arguments:  [hwnd] - Window handle to check
//
//  Returns:    TRUE if active, FALSE otherwise
//
//-----------------------------------------------------------------------------
BOOL
IsWindowActive(
    HWND    hwnd)
{
    HWND    hwndActive = GetActiveWindow();
    HWND    hwndFocus  = GetFocus();

    return (hwndActive &&
            hwndFocus  &&
            (hwndActive == hwnd || IsChild(hwndActive, hwnd)) &&
            (hwndFocus == hwnd  || IsChild(hwnd, hwndFocus)));
}


//+----------------------------------------------------------------------------
//
//  Function:   IsWindowPopup
//
//  Synopsis:   Returns TRUE if the passed HWND is contained within a window
//              that has WS_POPUP set
//
//  Arguments:  [hwnd] - Window handle to check
//
//  Returns:    TRUE if contained in a popup window, FALSE otherwise
//
//-----------------------------------------------------------------------------
BOOL
IsWindowPopup(
    HWND    hwnd)
{
    while (hwnd && !(GetWindowLong(hwnd, GWL_STYLE) & WS_POPUP))
        hwnd = GetParent(hwnd);

    return hwnd != NULL;
}
#endif // NEVER

//+----------------------------------------------------------------------------
//
//  Function:   GetOwningMDIChild
//
//  Synopsis:   Return the HWND of the MDI child which contains the passed
//              HWND
//
//  Arguments:  [hwnd] - Window handle for which to retrieve owning MDI child
//
//  Returns:    HWND of MDI child (if it exists), NULL otherwise
//
//-----------------------------------------------------------------------------
#ifndef WIN16
HWND
GetOwningMDIChild(
    HWND    hwnd)
{
    HWND    hwndMDIChild = hwnd;

    while (hwndMDIChild &&
           !(::GetWindowLong(hwndMDIChild, GWL_EXSTYLE) & WS_EX_MDICHILD))
    {
        hwndMDIChild = ::GetParent(hwndMDIChild);
    }

    return hwndMDIChild;
}
#endif //WIN16

//+----------------------------------------------------------------------------
//
//  Function:   GetOwnerOfParent
//
//  Synopsis:   Return the outer most owner/parent HWND of the given window
//              HWND
//
//  Arguments:  [hwnd] - Window handle for which to retrieve its owner
//
//  Returns:    HWND of its outer most owner/parent, itself otherwise
//
//-----------------------------------------------------------------------------
HWND
GetOwnerOfParent(HWND hwnd)
{
    HWND    hwndOwner = hwnd;
    HWND    hwndParent;

    Assert(hwnd);

    // Get outer most parent
    do
    {
        hwndParent = ::GetParent(hwndOwner);
        if (hwndParent)
        {
            hwndOwner = hwndParent;
        }
    } while (hwndParent);

    Assert(hwndOwner);

    // Get its owner
    hwndParent = ::GetWindow(hwndOwner, GW_OWNER);
    if (hwndParent)
    {
        hwndOwner = hwndParent;
    }

    return hwndOwner;
}

