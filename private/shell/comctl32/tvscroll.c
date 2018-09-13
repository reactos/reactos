#include "ctlspriv.h"
#include "treeview.h"

BOOL NEAR TV_EnsureVisible(PTREE pTree, TREEITEM FAR * hItem);

// ----------------------------------------------------------------------------
//
//  Updates the iShownIndex for every item below (in list order) a given item
//
// ----------------------------------------------------------------------------

int NEAR TV_UpdateShownIndexes(PTREE pTree, HTREEITEM hWalk)
{
    WORD iShownIndex;

    if (hWalk == pTree->hRoot) {
        hWalk = pTree->hRoot->hKids;
        if (hWalk) {
            hWalk->iShownIndex = 0;
        } else {
            return -1;
        }
    }

    iShownIndex = hWalk->iShownIndex + hWalk->iIntegral;
    if (iShownIndex <= 0)
    {
        // BUGBUG: We should #define the special TVITEM_HIDDEN value and check
        // for it explicitly
        // This can happen if TV_SortCB passes in a hidden item
        return(-1);
    }

    while ((hWalk = TV_GetNextVisItem(hWalk)) != NULL) {
        hWalk->iShownIndex = iShownIndex;
        iShownIndex += (WORD) hWalk->iIntegral;
    }

//#ifdef DEBUG
//      TraceMsg(TF_TREEVIEW, "tv: updated show indexes (now %d items)", (int)iShownIndex);
//#endif
    return (int)iShownIndex;
}

//
// in:
//      hItem   expanded node to count decendants of
//
// returns:
//      total number of expanded descendants below the given item.
//

UINT NEAR TV_CountVisibleDescendants(HTREEITEM hItem)
{
    UINT cnt;

    for (cnt = 0, hItem = hItem->hKids; hItem; hItem = hItem->hNext)
    {
        cnt += hItem->iIntegral;
        if (hItem->hKids && (hItem->state & TVIS_EXPANDED))
            cnt += TV_CountVisibleDescendants(hItem);
    }
    return cnt;
}

//  scrolls nItems in the direction of fDown starting from iTopShownIndex
void TV_ScrollItems(PTREE pTree, int nItems, int iTopShownIndex, BOOL fDown)
{
    RECT rc;
    rc.left = 0;
    rc.top = (iTopShownIndex+1) * pTree->cyItem;
    rc.right = pTree->cxWnd;
    rc.bottom = pTree->cyWnd;

    {
#ifndef UNIX
        SMOOTHSCROLLINFO si =
        {
            sizeof(si),
            SSIF_MINSCROLL | SSIF_MAXSCROLLTIME,
            pTree->ci.hwnd,
            0,
            ((fDown)?1:-1) * nItems * pTree->cyItem,
            &rc,
            &rc,
            NULL,
            NULL,
            SW_ERASE|SW_INVALIDATE,
            pTree->uMaxScrollTime,
            1,
            1
        };
#else
        SMOOTHSCROLLINFO si;
        si.cbSize = sizeof(si);
        si.fMask = SSIF_MINSCROLL | SSIF_MAXSCROLLTIME;
        si.hwnd = pTree->ci.hwnd;
        si.dx = 0;
        si.dy = ((fDown)?1:-1) * nItems * pTree->cyItem;
        si.lprcSrc = &rc;
        si.lprcClip = &rc;
        si.hrgnUpdate = NULL;
        si.lprcUpdate = NULL;
        si.fuScroll = SW_ERASE|SW_INVALIDATE;
        si.uMaxScrollTime = pTree->uMaxScrollTime;
        si.cxMinScroll = 1;
        si.cyMinScroll = 1;
        si.pfnScrollProc = NULL;
#endif

        SmoothScrollWindow(&si);
    }
    TV_UpdateToolTip(pTree);
}

//
//  If fRedrawParent is FALSE, then the return value is garbage.
//  If fRedrawParent is TRUE, then returns the number of children scrolled.
//
//  Does not update iShownIndex for any items.
//
UINT NEAR TV_ScrollBelow(PTREE pTree, HTREEITEM hItem, BOOL fRedrawParent, BOOL fDown)
{
    int     iTop;
    UINT    cnt;

    // Do nothing if the item is not visible
    if (!ITEM_VISIBLE(hItem))
        return 0;
    
    cnt = hItem->iIntegral; // default return val
    if (pTree->fRedraw) {
        UINT cVisDesc;
        BOOL fEffect;

        // iTop is the top edge (client coordinates) of the bottom integral
        // cell of the item that just got expanded/contracted.
        // (Confused yet?  I sure am.)
        iTop = hItem->iShownIndex - pTree->hTop->iShownIndex + hItem->iIntegral - 1;
        cVisDesc = TV_CountVisibleDescendants(hItem);

        // See if the item being expanded/contracted has any effect on the
        // screen.  If not, then don't TV_ScrollItems or we will end up
        // double-counting them when we do post-scroll adjustment.
        if (fDown)
        {
            // When scrolling down, we have an effect if the item that just
            // got expanded was below the top of the screen
            fEffect = iTop >= 0;
        }
        else
        {
            // When scrolling up, we have an effect if any of the items
            // that just got collapsed out were below the top of the screen
            fEffect = (int)(iTop + cVisDesc) >= 0;
        }

        if (fEffect)
            TV_ScrollItems(pTree, cVisDesc, iTop, fDown);
        TV_InvalidateItem(pTree, hItem, TRUE);

        if (fRedrawParent)
            cnt = cVisDesc;

    } else {

        if (fRedrawParent)
            cnt = TV_CountVisibleDescendants(hItem);

    }

    return(cnt);
}

// The FakeCustomDraw functions are used when you want the customdraw client
// to set up a HDC so you can do stuff like GetTextExtent.
//
//  Usage:
//
//      TVFAKEDRAW tvfd;
//      TreeView_BeginFakeCustomDraw(pTree, &tvfd);
//      for each item you care about {
//          TreeView_BeginFakeItemDraw(&tvfd, hitem);
//          <party on the HDC in tvfd.nmcd.nmcd.hdc>
//          TreeView_EndFakeItemDraw(&tvfd);
//      }
//      TreeView_EndFakeCustomDraw(&tvfd);
//

void TreeView_BeginFakeCustomDraw(PTREE pTree, PTVFAKEDRAW ptvfd)
{
    ptvfd->nmcd.nmcd.hdc = GetDC(pTree->ci.hwnd);
    ptvfd->nmcd.nmcd.uItemState = 0;
    ptvfd->nmcd.nmcd.dwItemSpec = 0;
    ptvfd->nmcd.nmcd.lItemlParam = 0;
    ptvfd->hfontPrev = (HFONT)GetCurrentObject(ptvfd->nmcd.nmcd.hdc, OBJ_FONT);

    //
    //  Since we aren't actually painting anything, we pass an empty
    //  paint rectangle.  Gosh, I hope no app faults when it sees an
    //  empty paint rectangle.
    //
    SetRectEmpty(&ptvfd->nmcd.nmcd.rc);

    ptvfd->pTree = pTree;
    ptvfd->dwCustomPrev = pTree->ci.dwCustom;

    pTree->ci.dwCustom = CIFakeCustomDrawNotify(&pTree->ci, CDDS_PREPAINT, &ptvfd->nmcd.nmcd);
}

DWORD TreeView_BeginFakeItemDraw(PTVFAKEDRAW ptvfd, HTREEITEM hitem)
{
    PTREE pTree = ptvfd->pTree;

    // Note that if the client says CDRF_SKIPDEFAULT (i.e., is owner-draw)
    // we measure the item anyway, because that's what IE4 did.

    ptvfd->nmcd.nmcd.dwItemSpec = (DWORD_PTR)hitem;
    ptvfd->nmcd.nmcd.lItemlParam = hitem->lParam;

    if (hitem->state & TVIS_BOLD) {
        SelectFont(ptvfd->nmcd.nmcd.hdc, pTree->hFontBold);
    } else {
        SelectFont(ptvfd->nmcd.nmcd.hdc, pTree->hFont);
    }

    if (!(pTree->ci.dwCustom & CDRF_SKIPDEFAULT)) {
        // Font should not depend on colors or flags since those change
        // dynamically but we cache the width info forever.  So we don't
        // need to set up uItemState.
        ptvfd->nmcd.clrText = pTree->clrText;
        ptvfd->nmcd.clrTextBk = pTree->clrBk;
        ptvfd->nmcd.iLevel = hitem->iLevel;
        ptvfd->dwCustomItem = CIFakeCustomDrawNotify(&pTree->ci, CDDS_ITEMPREPAINT, &ptvfd->nmcd.nmcd);
    } else {
        ptvfd->dwCustomItem = CDRF_DODEFAULT;
    }

    return ptvfd->dwCustomItem;
}

void TreeView_EndFakeItemDraw(PTVFAKEDRAW ptvfd)
{
    PTREE pTree = ptvfd->pTree;

    if (!(ptvfd->dwCustomItem & CDRF_SKIPDEFAULT) &&
         (ptvfd->dwCustomItem & CDRF_NOTIFYPOSTPAINT)) {
        CIFakeCustomDrawNotify(&pTree->ci, CDDS_ITEMPOSTPAINT, &ptvfd->nmcd.nmcd);
    }
}

void TreeView_EndFakeCustomDraw(PTVFAKEDRAW ptvfd)
{
    PTREE pTree = ptvfd->pTree;

    // notify parent afterwards if they want us to
    if (!(pTree->ci.dwCustom & CDRF_SKIPDEFAULT) &&
        pTree->ci.dwCustom & CDRF_NOTIFYPOSTPAINT) {
        CIFakeCustomDrawNotify(&pTree->ci, CDDS_POSTPAINT, &ptvfd->nmcd.nmcd);
    }

    // Restore previous state
    pTree->ci.dwCustom = ptvfd->dwCustomPrev;
    SelectObject(ptvfd->nmcd.nmcd.hdc, ptvfd->hfontPrev);
    ReleaseDC(pTree->ci.hwnd, ptvfd->nmcd.nmcd.hdc);
}


// ----------------------------------------------------------------------------
//
//  Returns the width of the widest shown item in the tree
//
// ----------------------------------------------------------------------------

UINT NEAR TV_RecomputeMaxWidth(PTREE pTree)
{
    if (!(pTree->ci.style & TVS_NOSCROLL)) {
        HTREEITEM hItem;
        WORD wMax = 0;

        // REVIEW: this might not be the most efficient traversal of the tree

        for (hItem = pTree->hRoot->hKids; hItem; hItem = TV_GetNextVisItem(hItem))
        {
            if (wMax < FULL_WIDTH(pTree, hItem))
                wMax = FULL_WIDTH(pTree, hItem);
        }

        return((UINT)wMax);
    } else {
        return pTree->cxWnd;
    }
}


// ----------------------------------------------------------------------------
//
//  Returns the horizontal text extent of the given item's text
//
// ----------------------------------------------------------------------------

WORD NEAR TV_GetItemTextWidth(HDC hdc, PTREE pTree, HTREEITEM hItem)
{
    TVITEMEX sItem;
    TCHAR szTemp[MAX_PATH];
    SIZE size = {0,0};

    sItem.pszText = szTemp;
    sItem.cchTextMax = ARRAYSIZE(szTemp);

    TV_GetItem(pTree, hItem, TVIF_TEXT, &sItem);

    GetTextExtentPoint(hdc, sItem.pszText, lstrlen(sItem.pszText), &size);
    return (WORD)(size.cx + (g_cxLabelMargin * 2));
}


// ----------------------------------------------------------------------------
//
//  Compute the text extent and the full width (indent, image, and text) of
//  the given item.
//
//  If there is a HDC, then we assume that the HDC has been set up with
//  the proper attributes (specifically, the font).  If there is no HDC,
//  then we will set one up, measure the text, then tear it down.
//  If you will be measuring more than one item, it is recommended that
//  the caller set up the HDC and keep re-using it, because creating,
//  initializing, then destroy the HDC is rather slow.
//
// ----------------------------------------------------------------------------

void NEAR TV_ComputeItemWidth(PTREE pTree, HTREEITEM hItem, HDC hdc)
{
    TVFAKEDRAW  tvfd;                    // in case client uses customdraw
    int iOldWidth = hItem->iWidth;

    if (hdc == NULL) {
        TreeView_BeginFakeCustomDraw(pTree, &tvfd);
        TreeView_BeginFakeItemDraw(&tvfd, hItem);
    }
    else
    {
        tvfd.nmcd.nmcd.hdc = hdc;
    }
    
    hItem->iWidth = TV_GetItemTextWidth(tvfd.nmcd.nmcd.hdc, pTree, hItem);

    if (!(pTree->ci.style & TVS_NOSCROLL) && iOldWidth != hItem->iWidth)
        if (pTree->cxMax < FULL_WIDTH(pTree, hItem)) {
            PostMessage(pTree->ci.hwnd, TVMP_CALCSCROLLBARS, 0, 0);
            pTree->cxMax = FULL_WIDTH(pTree, hItem);
        }
    
    if (hdc == NULL)
    {
        TreeView_EndFakeItemDraw(&tvfd);
        TreeView_EndFakeCustomDraw(&tvfd);
    }
}


// ----------------------------------------------------------------------------
//
//  Returns TRUE if the item is expanded, FALSE otherwise
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_IsShowing(HTREEITEM hItem)
{
    for (hItem = hItem->hParent; hItem; hItem = hItem->hParent)
        if (!(hItem->state & TVIS_EXPANDED))
            return FALSE;

    return TRUE;
}


// ----------------------------------------------------------------------------
//
//  If the added item is showing, update the shown (expanded) count, the max
//  item width -- then recompute the scroll bars.
//
//  sets cxMax, cShowing
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_ScrollBarsAfterAdd(PTREE pTree, HTREEITEM hItem)
{
    HTREEITEM   hPrev;

    if (!TV_IsShowing(hItem))
    {
        // item isn't visible -- set index to NOTVISIBLE and return
        hItem->iShownIndex = (WORD)-1;
        return FALSE;
    }

    hPrev = TV_GetPrevVisItem(hItem);

    // increment every shown index after newly added item

    hItem->iShownIndex = (hPrev) ? hPrev->iShownIndex + hPrev->iIntegral : 0;

    TV_UpdateShownIndexes(pTree, hItem);

    pTree->cShowing += hItem->iIntegral;

    TV_ComputeItemWidth(pTree, hItem, NULL);

    TV_CalcScrollBars(pTree);
    return(TRUE);
}


// ----------------------------------------------------------------------------
//
//  If the removed item was showing, update the shown (expanded) count, the max
//  item width -- then recompute the scroll bars.
//
//  sets cxMax, cShowing
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_ScrollBarsAfterRemove(PTREE pTree, HTREEITEM hItem)
{
    HTREEITEM hWalk;
    if (!ITEM_VISIBLE(hItem))
        return FALSE;

    // decrement every shown index after removed item
    hItem->iShownIndex = (WORD)-1;

    hWalk = TV_GetNextVisItem(hItem);
    if (hWalk) {
        hWalk->iShownIndex -= (WORD) hItem->iIntegral;
        TV_UpdateShownIndexes(pTree, hWalk);

        // If we delete the top item, the tree scrolls to the end, so ...
        if (pTree->hTop == hItem) {
            TV_SetTopItem(pTree, hWalk->iShownIndex);
            ASSERT(pTree->hTop != hItem);
        }
    }

    pTree->cShowing -= hItem->iIntegral;

    if (pTree->fRedraw) {
        if (!hItem->iWidth)
            TV_ComputeItemWidth(pTree, hItem, NULL);


        if (!(pTree->ci.style & TVS_NOSCROLL))
            if (pTree->cxMax == FULL_WIDTH(pTree, hItem))
                pTree->cxMax = (WORD) TV_RecomputeMaxWidth(pTree);

        TV_CalcScrollBars(pTree);
    }
    return TRUE;
}


// ----------------------------------------------------------------------------
//
//  Common worker function for
//  TV_ScrollBarsAfterExpand and TV_ScrollBarsAfterCollapse, since they
//  are completely identical save for two lines of code.
//
//  If the expanded items are / collapsed items were showing, update
//  the shown (expanded) count, the max item width -- then recompute
//  the scroll bars.
//
// ----------------------------------------------------------------------------

#define SBAEC_COLLAPSE  0
#define SBAEC_EXPAND    1

BOOL NEAR TV_ScrollBarsAfterExpandCollapse(PTREE pTree, HTREEITEM hParent, UINT flags)
{
    WORD cxMax = 0;
    HTREEITEM hWalk;
    TVFAKEDRAW tvfd;

    if (!ITEM_VISIBLE(hParent))
        return FALSE;

    //
    // We're going to be measuring a lot of items, so let's set up
    // our DC ahead of time.
    //
    TreeView_BeginFakeCustomDraw(pTree, &tvfd);

    for (hWalk = hParent->hKids;
         hWalk && (hWalk->iLevel > hParent->iLevel);
         hWalk = TV_GetNextVisItem(hWalk))
    {
         if (flags == SBAEC_COLLAPSE)
            hWalk->iShownIndex = (WORD)-1;
         if (!hWalk->iWidth)
         {
            TreeView_BeginFakeItemDraw(&tvfd, hWalk);
            TV_ComputeItemWidth(pTree, hWalk, tvfd.nmcd.nmcd.hdc);
            TreeView_EndFakeItemDraw(&tvfd);
         }
         if (cxMax < FULL_WIDTH(pTree, hWalk))
             cxMax = FULL_WIDTH(pTree, hWalk);
    }

    TreeView_EndFakeCustomDraw(&tvfd);

    // update every shown index after expanded parent
    pTree->cShowing = TV_UpdateShownIndexes(pTree, hParent);

    // Update the pTree->cxMax if it is affected by the items we
    // expanded/collapsed.

    if (!(pTree->ci.style & TVS_NOSCROLL))
    {
        if (flags == SBAEC_COLLAPSE)
        {
            // If one of our newly-hidden items was responsible for
            // the width being what it is, recompute the max width
            // since we hid those items.
            if (cxMax == pTree->cxMax)
                pTree->cxMax = (WORD) TV_RecomputeMaxWidth(pTree);
        }
        else
        {
            // If one of our newly-shown items was responsible is wider
            // then the previous max, then we have set a new max.
            if (cxMax > pTree->cxMax)
                pTree->cxMax = cxMax;
        }
    }

    TV_CalcScrollBars(pTree);
    return(TRUE);
}


// ----------------------------------------------------------------------------
//
//  If the expanded items are showing, update the shown (expanded) count,
//  the max item width -- then recompute the scroll bars.
//
//  sets cxMax, cShowing
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_ScrollBarsAfterExpand(PTREE pTree, HTREEITEM hParent)
{
    return TV_ScrollBarsAfterExpandCollapse(pTree, hParent, SBAEC_EXPAND);
}


// ----------------------------------------------------------------------------
//
//  If the collapsed items were showing, update the shown (expanded) count,
//  the max item width -- then recompute the scroll bars.
//
//  sets cxMax, cShowing
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_ScrollBarsAfterCollapse(PTREE pTree, HTREEITEM hParent)
{
    return TV_ScrollBarsAfterExpandCollapse(pTree, hParent, SBAEC_COLLAPSE);
}

// ----------------------------------------------------------------------------
//
//  If the added item changed height, then scroll thing around,
//  update the shown (expanded) count, recompute the scroll bars.
//
//  sets cShowing
//
// ----------------------------------------------------------------------------

void NEAR TV_ScrollBarsAfterResize(PTREE pTree, HTREEITEM hItem, int iIntegralPrev, UINT uRDWFlags)
{
    int iMaxIntegral = max(hItem->iIntegral, iIntegralPrev);

    ASSERT(hItem->iIntegral != iIntegralPrev);

    if (pTree->fRedraw)
    {
        int iTop = hItem->iShownIndex - pTree->hTop->iShownIndex +
                    iMaxIntegral - 1;
        if (iTop >= 0)
        {
            int iGrowth = hItem->iIntegral - iIntegralPrev;
            TV_ScrollItems(pTree, abs(iGrowth), iTop, iGrowth > 0);
        }
    }

    // update every shown index after resized item
    pTree->cShowing = TV_UpdateShownIndexes(pTree, hItem);
    TV_CalcScrollBars(pTree);

    // Invalidate based on the worst-case height so we handle
    // both the grow and shrink cases.
    if (pTree->fRedraw)
    {
        RECT rc;
        if (TV_GetItemRect(pTree, hItem, &rc, FALSE))
        {
            rc.bottom = rc.top + pTree->cyItem * iMaxIntegral;
            RedrawWindow(pTree->ci.hwnd, &rc, NULL, uRDWFlags);
        }
    }
}



// ----------------------------------------------------------------------------
//
//  Returns the item just below the given item in the tree.
//
// ----------------------------------------------------------------------------

TREEITEM FAR * NEAR TV_GetNext(TREEITEM FAR * hItem)
{
    DBG_ValidateTreeItem(hItem, FALSE);

    if (hItem->hKids)
        return hItem->hKids;

checkNext:
    if (hItem->hNext)
        return hItem->hNext;

    hItem = hItem->hParent;
    if (hItem)
        goto checkNext;

    return NULL;
}


// ----------------------------------------------------------------------------
//
//  Go through all the items in the tree, recomputing each item's text extent
//  and full width (indent, image, and text).
//
// ----------------------------------------------------------------------------

void NEAR TV_RecomputeItemWidths(PTREE pTree)
{
    HTREEITEM hItem;
    TVFAKEDRAW tvfd;

    TreeView_BeginFakeCustomDraw(pTree, &tvfd);

    hItem = pTree->hRoot->hKids;
    while (hItem)
    {
        TreeView_BeginFakeItemDraw(&tvfd, hItem);
        TV_ComputeItemWidth(pTree, hItem, tvfd.nmcd.nmcd.hdc);
        TreeView_EndFakeItemDraw(&tvfd);
        hItem = TV_GetNext(hItem);
    }
    TreeView_EndFakeCustomDraw(&tvfd);
}


// ----------------------------------------------------------------------------
//
//  If a single item's width changed, alter the max width if needed.
//  If all widths changed, recompute widths and max width.
//  Then recompute the scroll bars.
//
//  sets cxMax
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_ScrollBarsAfterSetWidth(PTREE pTree, HTREEITEM hItem)
{
    if (hItem)
    {
        UINT iOldWidth = FULL_WIDTH(pTree, hItem);
        TV_ComputeItemWidth(pTree, hItem, NULL);

        if (!(pTree->ci.style & TVS_NOSCROLL)) {
            if (pTree->cxMax == iOldWidth)
                pTree->cxMax = (WORD) TV_RecomputeMaxWidth(pTree);
            else
                return(FALSE);
        }
    }
    else
    {
        TV_RecomputeItemWidths(pTree);
        pTree->cxMax = (WORD) TV_RecomputeMaxWidth(pTree);
    }

    TV_CalcScrollBars(pTree);
    return(TRUE);
}


// ----------------------------------------------------------------------------
//
//  Scroll window vertically as needed to make given item fully visible
//  vertically
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_ScrollVertIntoView(PTREE pTree, HTREEITEM hItem)
{
    // This function has crashed in stress before, so we need to assert the incoming parameters.
    ASSERT(hItem);
    ASSERT(pTree && pTree->hTop);

    // Do nothing if the parameters are invalid
    if (!hItem || !pTree || !(pTree->hTop))
        return FALSE;

    // Do nothing if this item is not visible
    if (!ITEM_VISIBLE(hItem))
        return FALSE;

    if (hItem->iShownIndex < pTree->hTop->iShownIndex)
        return(TV_SetTopItem(pTree, hItem->iShownIndex));

    if (hItem->iShownIndex >= (pTree->hTop->iShownIndex + pTree->cFullVisible))
        return(TV_SetTopItem(pTree, hItem->iShownIndex + 1 - pTree->cFullVisible));

    return FALSE;
}


// ----------------------------------------------------------------------------
//
//  Scroll window vertically and horizontally as needed to make given item
//  fully visible vertically and horizontally
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_ScrollIntoView(PTREE pTree, HTREEITEM hItem)
{
    UINT iWidth, iOffset;
    BOOL fChange;

    fChange = TV_ScrollVertIntoView(pTree, hItem);

    // ensure that item's text is fully visible horizontally
    iWidth = pTree->cxImage + pTree->cxState + hItem->iWidth;
    if (iWidth > (UINT)pTree->cxWnd)
        iWidth = pTree->cxWnd; //hItem->iWidth;

    iOffset = ITEM_OFFSET(pTree, hItem);

    if ((int) (iOffset) < pTree->xPos)
        fChange |= TV_SetLeft(pTree, iOffset);
    else if ((iOffset + iWidth) > (UINT)(pTree->xPos + pTree->cxWnd))
        fChange |= TV_SetLeft(pTree, iOffset + iWidth - pTree->cxWnd);

    return fChange;
}


// ----------------------------------------------------------------------------
//
//  Sets position of horizontal scroll bar and scrolls window to match that
//  position
//
//  sets xPos
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_SetLeft(PTREE pTree, int x)
{
    if (!pTree->fHorz || pTree->ci.style & (TVS_NOSCROLL | TVS_NOHSCROLL))
        return(FALSE);

    if (x > (int) (pTree->cxMax - pTree->cxWnd))
        x = (pTree->cxMax - pTree->cxWnd);
    if (x < 0)
        x = 0;

    if (x == pTree->xPos)
        return(FALSE);

    if (pTree->fRedraw) {
#ifndef UNIX
        SMOOTHSCROLLINFO si =
        {
            sizeof(si),
            SSIF_MINSCROLL | SSIF_MAXSCROLLTIME,
            pTree->ci.hwnd,
            pTree->xPos - x,
            0,
            NULL,
            NULL,
            NULL,
            NULL,
            SW_INVALIDATE | SW_ERASE,
            pTree->uMaxScrollTime,
            1,
            1
        };
#else
        SMOOTHSCROLLINFO si;
        si.cbSize = sizeof(si);
        si.fMask = SSIF_MINSCROLL | SSIF_MAXSCROLLTIME;
        si.hwnd = pTree->ci.hwnd;
        si.dx = pTree->xPos - x;
        si.dy = 0;
        si.lprcSrc = NULL;
        si.lprcClip = NULL;
        si.hrgnUpdate = NULL;
        si.lprcUpdate = NULL;
        si.fuScroll = SW_INVALIDATE | SW_ERASE;
        si.uMaxScrollTime = pTree->uMaxScrollTime;
        si.cxMinScroll = 1;
        si.cyMinScroll = 1;
        si.pfnScrollProc = NULL;
#endif
        SmoothScrollWindow(&si);
    }

    pTree->xPos = (SHORT) x;

    SetScrollPos(pTree->ci.hwnd, SB_HORZ, x, TRUE);
    TV_UpdateToolTip(pTree);

    return(TRUE);
}


// ----------------------------------------------------------------------------
//
//  Returns the tree's item that has the given shown index, NULL if no item
//  found with that index.
//
// ----------------------------------------------------------------------------

HTREEITEM NEAR TV_GetShownIndexItem(HTREEITEM hItem, UINT wShownIndex)
{
    HTREEITEM hWalk;

    if (hItem == NULL)
        return NULL;

    ASSERT((int)wShownIndex >= 0);

    for (hWalk = hItem;
         hWalk && (hWalk->iShownIndex <= wShownIndex);
         hWalk = hWalk->hNext) {
        
         hItem = hWalk;
         
         if (hWalk->iShownIndex + (UINT)hWalk->iIntegral > wShownIndex) 
             return hWalk;
    }

    return TV_GetShownIndexItem(hItem->hKids, wShownIndex);
}


// ----------------------------------------------------------------------------
//
//  Sets position of vertical scroll bar and scrolls window to match that
//  position
//
//  sets hTop
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_SmoothSetTopItem(PTREE pTree, UINT wNewTop, UINT uSmooth)
{
    HTREEITEM hItem = pTree->hRoot->hKids;
    UINT wOldTop;

    if (!hItem)
        return FALSE;
    
    if ((pTree->ci.style & TVS_NOSCROLL) || (wNewTop == (UINT)-1) || (pTree->cShowing <= pTree->cFullVisible)) {
        // we've wrapped around (treat as a negative index) -- use min pos
        // or there aren't enough items to scroll
        wNewTop = 0;
    } else if (wNewTop > (UINT)(pTree->cShowing - pTree->cFullVisible)) {
        // we've gone too far down -- use max pos
        wNewTop = (pTree->cShowing - pTree->cFullVisible);

    }

    // if there's no room for anything to show. peg at the end
    if (wNewTop > 0 && wNewTop >= pTree->cShowing) {
        wNewTop = pTree->cShowing - 1;
    }

    hItem = TV_GetShownIndexItem(hItem, wNewTop);

    // BUGBUG (scotth): refreshing in regedit sometimes hits this case
    ASSERT(hItem);

    if (NULL == hItem || pTree->hTop == hItem)
        return FALSE;
    // need to refetch because wNewTop couldhave pointed to the middle of this item,
    // which is not allowed
    wNewTop = hItem->iShownIndex;
    
    wOldTop = pTree->hTop->iShownIndex;

    pTree->hTop = hItem;

    if (pTree->fRedraw) {
#ifndef UNIX
        SMOOTHSCROLLINFO si =
        {
            sizeof(si),
            SSIF_MINSCROLL | SSIF_MAXSCROLLTIME,
            pTree->ci.hwnd,
            0,
            (int) (wOldTop - wNewTop) * (int) pTree->cyItem,
            NULL,
            NULL,
            NULL,
            NULL,
            SW_INVALIDATE | SW_ERASE | uSmooth,
            pTree->uMaxScrollTime,
            1,
            1
        };
#else
        SMOOTHSCROLLINFO si;
        si.cbSize = sizeof(si);
        si.fMask = SSIF_MINSCROLL | SSIF_MAXSCROLLTIME;
        si.hwnd = pTree->ci.hwnd;
        si.dx = 0;
        si.dy = (int) (wOldTop - wNewTop) * (int) pTree->cyItem;
        si.lprcSrc = NULL;
        si.lprcClip = NULL;
        si.hrgnUpdate = NULL;
        si.lprcUpdate = NULL;
        si.fuScroll = SW_INVALIDATE | SW_ERASE | uSmooth;
        si.uMaxScrollTime = pTree->uMaxScrollTime;
        si.cxMinScroll = 1;
        si.cyMinScroll = 1;
        si.pfnScrollProc = NULL;
#endif
        SmoothScrollWindow(&si);
    }

    SetScrollPos(pTree->ci.hwnd, SB_VERT, wNewTop, TRUE);
    TV_UpdateToolTip(pTree);

    return(TRUE);
}


// ----------------------------------------------------------------------------
//
//  Computes the horizontal and vertical scroll bar ranges, pages, and
//  positions, adding or removing the scroll bars as needed.
//
//  sets fHorz, fVert
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_CalcScrollBars(PTREE pTree)
{
    // UINT wMaxPos;
    BOOL fChange = FALSE;
    SCROLLINFO si;
    
    if (pTree->ci.style & TVS_NOSCROLL)
        return FALSE;

    si.cbSize = sizeof(SCROLLINFO);

    if (!(pTree->ci.style & TVS_NOHSCROLL))
    {
        if ((SHORT)pTree->cxMax > (SHORT)pTree->cxWnd)
        {
            if (!pTree->fHorz)
            {
                fChange = TRUE;
                pTree->fHorz = TRUE;
            }

            si.fMask = SIF_PAGE | SIF_RANGE;
            si.nMin = 0;
            si.nMax = pTree->cxMax - 1;
            si.nPage = pTree->cxWnd;

            TV_SetLeft(pTree, (UINT)SetScrollInfo(pTree->ci.hwnd, SB_HORZ, &si, TRUE));
        }
        else if (pTree->fHorz)
        {
            TV_SetLeft(pTree, 0);
            SetScrollRange(pTree->ci.hwnd, SB_HORZ, 0, 0, TRUE);

            pTree->fHorz = FALSE;
            fChange = TRUE;
        }
    }

    if (pTree->cShowing > pTree->cFullVisible)
    {
        if (!pTree->fVert)
        {
            pTree->fVert = TRUE;
            fChange = TRUE;
        }

        si.fMask = SIF_PAGE | SIF_RANGE;
        si.nMin = 0;
        si.nMax = pTree->cShowing - 1;
        si.nPage = pTree->cFullVisible;

        TV_SetTopItem(pTree, (UINT)SetScrollInfo(pTree->ci.hwnd, SB_VERT, &si, TRUE));

    }
    else if (pTree->fVert)
    {
        TV_SetTopItem(pTree, 0);
        SetScrollRange(pTree->ci.hwnd, SB_VERT, 0, 0, TRUE);

        pTree->fVert = FALSE;
        fChange = TRUE;
    }

    if (fChange)
        TV_SizeWnd(pTree, 0, 0);

    return(TRUE);
}


// ----------------------------------------------------------------------------
//
//  Handles horizontal scrolling.
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_HorzScroll(PTREE pTree, UINT wCode, UINT wNewPos)
{
    BOOL fChanged;

    TV_DismissEdit(pTree, FALSE);

    switch (wCode)
    {
        case SB_BOTTOM:
            wNewPos = pTree->cxMax - pTree->cxWnd;
            break;

        case SB_ENDSCROLL:
            wNewPos = pTree->xPos;
            break;

        case SB_LINEDOWN:
            wNewPos = pTree->xPos + MAGIC_HORZLINE;
            break;

        case SB_LINEUP:
            wNewPos = pTree->xPos - MAGIC_HORZLINE;
            break;

        case SB_PAGEDOWN:
            wNewPos = pTree->xPos + (pTree->cxWnd - MAGIC_HORZLINE);
            break;

        case SB_PAGEUP:
            wNewPos = pTree->xPos - (pTree->cxWnd - MAGIC_HORZLINE);
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            break;

        case SB_TOP:
            wNewPos = 0;
            break;
    }

    if (fChanged = TV_SetLeft(pTree, wNewPos))
        UpdateWindow(pTree->ci.hwnd);

    return(fChanged);
}


// ----------------------------------------------------------------------------
//
//  Handles vertical scrolling.
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_VertScroll(PTREE pTree, UINT wCode, UINT wPos)
{
    UINT wNewPos = 0;
    UINT wOldPos;
    BOOL fChanged;
    UINT uSmooth = 0;

    if (!pTree->hTop)
        return FALSE;
    
    wOldPos = pTree->hTop->iShownIndex;
    TV_DismissEdit(pTree, FALSE);

    switch (wCode)
    {
        case SB_BOTTOM:
            wNewPos = pTree->cShowing - pTree->cFullVisible;
            break;

        case SB_ENDSCROLL:
            wNewPos = wOldPos;
            break;

        case SB_LINEDOWN:
            wNewPos = wOldPos + pTree->hTop->iIntegral;
            break;

        case SB_LINEUP:
            wNewPos = wOldPos - 1;
            if (wNewPos > wOldPos)
                wNewPos = 0;
            break;

        case SB_PAGEDOWN:
            wNewPos = wOldPos + (pTree->cFullVisible - 1);
            break;

        case SB_PAGEUP:
            wNewPos = wOldPos - (pTree->cFullVisible - 1);
            if (wNewPos > wOldPos)
                wNewPos = 0;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            uSmooth = SSW_EX_IMMEDIATE;
            wNewPos = wPos;
            break;

        case SB_TOP:
            wNewPos = 0;
            break;
    }

    if (fChanged = TV_SmoothSetTopItem(pTree, wNewPos, uSmooth))
        UpdateWindow(pTree->ci.hwnd);
    return(fChanged);
}


#ifdef DEBUG
static int nCompares;
#endif

typedef struct {
    LPTSTR lpstr;
    BOOL bCallBack;
    HTREEITEM hItem;
} TVCOMPARE, FAR *LPTVCOMPARE;

// Pointer comparision function for Sort and Search functions.
// lParam is lParam passed to sort/search functions.  Returns
// -1 if p1 < p2, 0 if p1 == p2, and 1 if p1 > p2.
//
int CALLBACK TV_DefCompare(LPTVCOMPARE sCmp1, LPTVCOMPARE sCmp2, LPARAM lParam)
{
#ifdef DEBUG
        ++nCompares;
#endif

        return lstrcmpi(sCmp1->lpstr, sCmp2->lpstr);
}


int CALLBACK TV_CompareItems(LPTVCOMPARE sCmp1, LPTVCOMPARE sCmp2, LPARAM lParam)
{
        TV_SORTCB FAR *pSortCB = (TV_SORTCB FAR *)lParam;
#ifdef DEBUG
        ++nCompares;
#endif

        return(pSortCB->lpfnCompare(sCmp1->hItem->lParam, sCmp2->hItem->lParam,
                pSortCB->lParam));
}


UINT NEAR TV_CountKids(HTREEITEM hItem)
{
    int cnt;

    for (cnt = 0, hItem = hItem->hKids; hItem; hItem = hItem->hNext)
        cnt++;

    return cnt;
}


// BUGBUG: bRecurse not implemented

BOOL PASCAL TV_SortCB(PTREE pTree, TV_SORTCB FAR *pSortCB, BOOL bRecurse,
        PFNDPACOMPARE lpfnDPACompare)
{
        HDPA dpaSort;
        HDSA dsaCmp;
        HTREEITEM hItem, hNext, hFirstMoved;
        LPTVCOMPARE psCompare, FAR *ppsCompare;
        int i, cKids;
        HTREEITEM hParent = pSortCB->hParent;

#ifdef DEBUG
        DWORD dwTime = GetTickCount();
        nCompares = 0;
#endif

        if (!hParent || hParent == TVI_ROOT)
            hParent = pTree->hRoot;

        if (!ValidateTreeItem(hParent, FALSE))
            return FALSE;               // Invalid parameter

        // Code below assumes at least one kid
        cKids = TV_CountKids(hParent);
        if (!cKids)
            return FALSE;

        // Create a DSA for all the extra info we'll need
        dsaCmp = DSA_Create(sizeof(TVCOMPARE), cKids);
        if (!dsaCmp)
            goto Error1;

        // Create a DPA containing all the tree items
        dpaSort = DPA_Create(cKids);
        if (!dpaSort)
            goto Error2;

        for (hItem = hParent->hKids; hItem; hItem = hItem->hNext)
        {
                TVCOMPARE sCompare;
                int nItem;

                // If I can't sort all of them, I don't want to sort any of them

                // We want to cache the text callback for default processing
                if (!lpfnDPACompare && hItem->lpstr==LPSTR_TEXTCALLBACK)
                {
                        TVITEMEX sItem;
                        TCHAR szTemp[MAX_PATH];

                        sItem.pszText = szTemp;
                        sItem.cchTextMax  = ARRAYSIZE(szTemp);
                        TV_GetItem(pTree, hItem, TVIF_TEXT, &sItem);

                        sCompare.lpstr = NULL;
                        sCompare.bCallBack = TRUE;
                        Str_Set(&sCompare.lpstr, sItem.pszText);
                        if (!sCompare.lpstr)
                        {
                                goto Error3;
                        }
                }
                else
                {
                        sCompare.lpstr = hItem->lpstr;
                        sCompare.bCallBack = FALSE;
                }

                // Create the pointer for this guy and add it to the DPA list
                sCompare.hItem = hItem;
                nItem = DSA_AppendItem(dsaCmp, &sCompare);
                if (nItem < 0)
                {
                        if (sCompare.bCallBack)
                        {
                                Str_Set(&sCompare.lpstr, NULL);
                        }
                        goto Error3;
                }

                if (DPA_AppendPtr(dpaSort, DSA_GetItemPtr(dsaCmp, nItem)) < 0)
                {
                        goto Error3;
                }
        }

        // Sort the DPA, then stick them back under the parent in the new order
        DPA_Sort(dpaSort, lpfnDPACompare ? (PFNDPACOMPARE)lpfnDPACompare :
                 (PFNDPACOMPARE) TV_DefCompare, (LPARAM)pSortCB);


        // Look for the first moved item, so we can invalidate a smaller area
        ppsCompare = (LPTVCOMPARE FAR *)DPA_GetPtrPtr(dpaSort);
        if (hParent->hKids != (*ppsCompare)->hItem)
        {
                hParent->hKids = (*ppsCompare)->hItem;
                hFirstMoved = hParent->hKids;
        }
        else
        {
                hFirstMoved = NULL;
        }

        // We do n-1 iterations here
        for (i = DPA_GetPtrCount(dpaSort) - 1; i > 0; --i, ++ppsCompare)
        {
                hNext = (*(ppsCompare+1))->hItem;
                if ((*ppsCompare)->hItem->hNext != hNext && !hFirstMoved)
                {
                        hFirstMoved = hNext;
                }
                (*ppsCompare)->hItem->hNext = hNext;
        }
        (*ppsCompare)->hItem->hNext = NULL;

        TV_UpdateShownIndexes(pTree, hParent);
        if ((pSortCB->hParent == TVI_ROOT) || !hParent) {
            if (pTree->cShowing < pTree->cFullVisible) {
                pTree->hTop = pTree->hRoot->hKids;
            }
        }

        if (hFirstMoved && (hParent->state & TVIS_EXPANDED))
        {
                RECT rcUpdate;

                TV_GetItemRect(pTree, hFirstMoved, &rcUpdate, FALSE);
                if (hParent->hNext)
                {
                        RECT rcTemp;

                        TV_GetItemRect(pTree, hParent->hNext, &rcTemp, FALSE);
                        rcUpdate.bottom = rcTemp.bottom;
                }
                else
                {
                        RECT rcClient;
                        GetClientRect(pTree->ci.hwnd, &rcClient);
                        // Set to maximal positive number, so the whole rest of
                        // the treeview gets invalidated
                        rcUpdate.bottom = rcClient.bottom;
                }
                if (pTree->fRedraw)
                    InvalidateRect(pTree->ci.hwnd, &rcUpdate, TRUE);
        }

Error3:
        DPA_Destroy(dpaSort);
Error2:
        for (i = DSA_GetItemCount(dsaCmp) - 1; i >= 0; --i)
        {
                psCompare = DSA_GetItemPtr(dsaCmp, i);
                if (psCompare->bCallBack)
                {
                        Str_Set(&(psCompare->lpstr), NULL);
                }
        }
        DSA_Destroy(dsaCmp);
Error1:

#ifdef DEBUG
        TraceMsg(TF_TREEVIEW, "tv.sort: %ld ms; %d cmps", GetTickCount()-dwTime, nCompares);
#endif

    {
        int wNewPos;
        // restore the scroll position
        if (GetWindowStyle(pTree->ci.hwnd) & WS_VSCROLL) {
            SCROLLINFO si;

            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_POS;
            wNewPos = 0;
            if (GetScrollInfo(pTree->ci.hwnd, SB_VERT, &si)) {
                wNewPos = si.nPos;
            }

        } else {
            wNewPos = 0;
        }

        if (TV_SetTopItem(pTree, wNewPos))
            UpdateWindow(pTree->ci.hwnd);
    }

    // if the caret is the child of the thing that was sorted, make sure it's
    // visible (but if we're sorting something completely unrelated, don't bother
    if (pTree->hCaret) {
        hItem = pTree->hCaret;
        do {
            // do this first.  if hParent is hCaret, we don't want to ensure visible...
            // only if it's an eventual child
            hItem = hItem->hParent;
            if (hParent == hItem) {
                TV_EnsureVisible(pTree, pTree->hCaret);
            }
        } while(hItem && hItem != pTree->hRoot);
    }

    // The items in the view may have moved around; let apps know
    // Do this last because this call might yield
    MyNotifyWinEvent(EVENT_OBJECT_REORDER, pTree->ci.hwnd, OBJID_CLIENT, 0);

    return TRUE;
}


BOOL NEAR TV_SortChildrenCB(PTREE pTree, LPTV_SORTCB pSortCB, BOOL bRecurse)
{
        return(TV_SortCB(pTree, pSortCB, bRecurse, (PFNDPACOMPARE)TV_CompareItems));
}


BOOL NEAR TV_SortChildren(PTREE pTree, HTREEITEM hParent, BOOL bRecurse)
{
        TV_SORTCB sSortCB;

        sSortCB.hParent = hParent;
        return(TV_SortCB(pTree, &sSortCB, bRecurse, NULL));
}
