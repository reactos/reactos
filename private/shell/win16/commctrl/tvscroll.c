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
    
    iShownIndex = hWalk->iShownIndex + 1;
    if (iShownIndex <= 0)
    {
	// BUGBUG: We should #define the special TVITEM_HIDDEN value and check
	// for it explicitly
	// This can happen if TV_SortCB passes in a hidden item
	return(-1);
    }

    while ((hWalk = TV_GetNextVisItem(hWalk)) != NULL)
        hWalk->iShownIndex = iShownIndex++;

//#ifdef DEBUG
//	DebugMsg(DM_TRACE, "tv: updated show indexes (now %d items)", (int)iShownIndex);
//#endif
    return (int)iShownIndex;
}

//
// in:
//	hItem	expanded node to count decendants of
//
// returns:
// 	total number of expanded descendants below the given item.
//

UINT NEAR TV_CountVisibleDescendants(HTREEITEM hItem)
{
    UINT cnt;

    for (cnt = 0, hItem = hItem->hKids; hItem; hItem = hItem->hNext)
    {
        cnt++;
        if (hItem->hKids && (hItem->state & TVIS_EXPANDED))
            cnt += TV_CountVisibleDescendants(hItem);
    }
    return cnt;
}



UINT NEAR TV_ScrollBelow(PTREE pTree, HTREEITEM hItem, BOOL fRedrawParent, BOOL fDown)
{
    RECT    rc;
    int     iTop;
    UINT    cnt;

    // Do nothing if the item is not visible
    if (!ITEM_VISIBLE(hItem))
	return 0;

    iTop = hItem->iShownIndex - pTree->hTop->iShownIndex;

    rc.left = 0;
    rc.top = iTop * pTree->cyItem;
    rc.right = pTree->cxWnd;
    rc.bottom = pTree->cyWnd;
    if (pTree->fRedraw)
        InvalidateRect(pTree->hwnd, &rc, TRUE);

    if (fRedrawParent)
    {
        cnt = TV_CountVisibleDescendants(hItem);
    }
    else
    {
        cnt = 1;
    }

    return(cnt);
}

// ----------------------------------------------------------------------------
//
//  Returns the width of the widest shown item in the tree
//
// ----------------------------------------------------------------------------

UINT NEAR TV_RecomputeMaxWidth(PTREE pTree)
{
    HTREEITEM hItem;
    WORD wMax = 0;

    // REVIEW: this might not be the most efficient traversal of the tree

    for (hItem = pTree->hRoot->hKids; hItem; hItem = TV_GetNextVisItem(hItem))
    {
        if (wMax < FULL_WIDTH(pTree, hItem))
            wMax = FULL_WIDTH(pTree, hItem);
    }

    return((UINT)wMax);
}


// ----------------------------------------------------------------------------
//
//  Returns the horizontal text extent of the given item's text
//
// ----------------------------------------------------------------------------

WORD NEAR TV_GetItemTextWidth(HDC hdc, PTREE pTree, HTREEITEM hItem)
{
    TV_ITEM sItem;
    char szTemp[MAX_PATH];
    SIZE size = {0,0};

    sItem.pszText = szTemp;
    sItem.cchTextMax = sizeof(szTemp);

    TV_GetItem(pTree, hItem, TVIF_TEXT, &sItem);

    GetTextExtentPoint(hdc, sItem.pszText, lstrlen(sItem.pszText), &size);
    return size.cx + (g_cxLabelMargin * 2);
}


// ----------------------------------------------------------------------------
//
//  Compute the text extent and the full width (indent, image, and text) of
//  the given item.
//
// ----------------------------------------------------------------------------

void NEAR TV_ComputeItemWidth(PTREE pTree, HTREEITEM hItem, HDC hdc) 
{
    HDC         hdcToUse;
    HFONT       hOldFont;
    TV_ITEM     sItem;   

    if (hdc == NULL) {
        HFONT   hFont;   
        // compute text width and full width
        hdcToUse = GetDC(pTree->hwnd);      
        sItem.stateMask = TVIS_BOLD;        
        TV_GetItem(pTree, hItem, TVIF_STATE, &sItem);   
        hFont = sItem.state & TVIS_BOLD         
            ? pTree->hFontBold : pTree->hFont;
        hOldFont = (hFont) ? SelectObject(hdcToUse, hFont) : NULL;
    }                    
    else                 
    {                    
        hdcToUse = hdc;  
    }                    
    hItem->iWidth = TV_GetItemTextWidth(hdcToUse, pTree, hItem);

    if (hdc == NULL)     
    {                    
        if (hOldFont)    
            SelectObject(hdcToUse, hOldFont);
        ReleaseDC(pTree->hwnd, hdcToUse);    
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

    hItem->iShownIndex = (hPrev) ? hPrev->iShownIndex + 1 : 0;

    TV_UpdateShownIndexes(pTree, hItem);

    pTree->cShowing++;

    TV_ComputeItemWidth(pTree, hItem, NULL);

    if (pTree->cxMax < FULL_WIDTH(pTree, hItem))
        pTree->cxMax = FULL_WIDTH(pTree, hItem);

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
        --hWalk->iShownIndex;
        TV_UpdateShownIndexes(pTree, hWalk);
        
        // If we delete the top item, the tree scrolls to the end, so ...
        if (pTree->hTop == hItem) {
            TV_SetTopItem(pTree, hWalk->iShownIndex);
            Assert(pTree->hTop != hItem);
        }
    }
        
    pTree->cShowing--;

    if (pTree->fRedraw) {
        if (!hItem->iWidth)
            TV_ComputeItemWidth(pTree, hItem, NULL);

        if (pTree->cxMax == FULL_WIDTH(pTree, hItem))
            pTree->cxMax = TV_RecomputeMaxWidth(pTree);

        TV_CalcScrollBars(pTree);
    }
    return TRUE;
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
    WORD cxMax = 0;
    HTREEITEM hWalk;

    if (!ITEM_VISIBLE(hParent))
        return FALSE;

    for (hWalk = hParent->hKids;
         hWalk && (hWalk->iLevel > hParent->iLevel);
         hWalk = TV_GetNextVisItem(hWalk))
    {
         if (!hWalk->iWidth)
             TV_ComputeItemWidth(pTree, hWalk, NULL);
         if (cxMax < FULL_WIDTH(pTree, hWalk))
             cxMax = FULL_WIDTH(pTree, hWalk);
    }

    // update every shown index after expanded parent
    pTree->cShowing = TV_UpdateShownIndexes(pTree, hParent);

    if (cxMax > pTree->cxMax)
        pTree->cxMax = cxMax;

    TV_CalcScrollBars(pTree);
    return(TRUE);
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
    WORD cxMax = 0;
    HTREEITEM hWalk;

    if (!ITEM_VISIBLE(hParent))
        return FALSE;

    for (hWalk = hParent->hKids;
         hWalk && (hWalk->iLevel > hParent->iLevel);
         hWalk = TV_GetNextVisItem(hWalk))
    {
         hWalk->iShownIndex = (WORD)-1;
         if (!hWalk->iWidth)
             TV_ComputeItemWidth(pTree, hWalk, NULL);
         if (cxMax < FULL_WIDTH(pTree, hWalk))
             cxMax = FULL_WIDTH(pTree, hWalk);
    }

    // update every shown index after expanded parent
    pTree->cShowing = TV_UpdateShownIndexes(pTree, hParent);

    if (cxMax == pTree->cxMax)
        pTree->cxMax = TV_RecomputeMaxWidth(pTree);

    TV_CalcScrollBars(pTree);
    return(TRUE);
}


// ----------------------------------------------------------------------------
//
//  Returns the item just below the given item in the tree.
//
// ----------------------------------------------------------------------------

TREEITEM FAR * NEAR TV_GetNext(TREEITEM FAR * hItem)
{
    ValidateTreeItem(hItem, FALSE);

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
    HDC hdc = GetDC(pTree->hwnd);
    HFONT hOldFont = NULL;
    HTREEITEM hItem = pTree->hRoot->hKids;

    if (pTree->hFont)
        hOldFont = SelectObject(hdc, pTree->hFont);

    while (hItem)
    {
        //
        //BUGBUG:  Optimize TV_ComputeItemWidth so it
        //         can be called every time.
        //

        if (hItem->state & TVIS_BOLD)
            TV_ComputeItemWidth(pTree, hItem, NULL);
        else
            hItem->iWidth = TV_GetItemTextWidth(hdc, pTree, hItem);
        hItem = TV_GetNext(hItem);
    }

    if (hOldFont)
        SelectObject(hdc, hOldFont);

    ReleaseDC(pTree->hwnd, hdc);
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

        if (pTree->cxMax < FULL_WIDTH(pTree, hItem))
            pTree->cxMax = FULL_WIDTH(pTree, hItem);
        else if (pTree->cxMax == iOldWidth)
            pTree->cxMax = TV_RecomputeMaxWidth(pTree);
        else
            return(FALSE);
    }
    else
    {
        TV_RecomputeItemWidths(pTree);
        pTree->cxMax = TV_RecomputeMaxWidth(pTree);
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
    if (!pTree->fHorz)
        return(FALSE);

    if (x > (int) (pTree->cxMax - pTree->cxWnd))
        x = (pTree->cxMax - pTree->cxWnd);
    if (x < 0)
        x = 0;

    if (x == pTree->xPos)
        return(FALSE);

    if (pTree->fRedraw) {
        ScrollWindowEx(pTree->hwnd, pTree->xPos - x, 0, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
    }
    
    pTree->xPos = x;

    SetScrollPos(pTree->hwnd, SB_HORZ, x, TRUE);

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

    Assert((int)wShownIndex >= 0);

    for (hWalk = hItem->hNext;
         hWalk && (hWalk->iShownIndex <= wShownIndex);
         hWalk = hWalk->hNext)
         hItem = hWalk;

    if (hItem->iShownIndex == wShownIndex)
        return hItem;

    // Assert(hItem->hKids == NULL);

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

BOOL NEAR TV_SetTopItem(PTREE pTree, UINT wNewTop)
{
    HTREEITEM hItem = pTree->hRoot->hKids;
    UINT wOldTop;

    if (!hItem)
        return FALSE;

    if ((wNewTop == (UINT)-1) || (pTree->cShowing <= pTree->cFullVisible)) {
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

    Assert(hItem);

    if (pTree->hTop == hItem)
        return FALSE;

    wOldTop = pTree->hTop->iShownIndex;

    pTree->hTop = hItem;

    if (pTree->fRedraw) {
        ScrollWindowEx(pTree->hwnd, 0, (int) (wOldTop - wNewTop) * (int) pTree->cyItem,
                       NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_ERASE);
    }

    SetScrollPos(pTree->hwnd, SB_VERT, wNewTop, TRUE);

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

    si.cbSize = sizeof(SCROLLINFO);

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

#ifdef IEWIN31_25
        SetScrollRange(pTree->hwnd, SB_HORZ, (int)si.nMin, (int)(si.nMax-si.nPage+1), TRUE);
        TV_SetLeft(pTree, GetScrollPos( pTree->hwnd, SB_HORZ));
#else
        TV_SetLeft(pTree, (UINT)SetScrollInfo(pTree->hwnd, SB_HORZ, &si, TRUE));
#endif
    }
    else if (pTree->fHorz)
    {
        TV_SetLeft(pTree, 0);
        SetScrollRange(pTree->hwnd, SB_HORZ, 0, 0, TRUE);

        pTree->fHorz = FALSE;
        fChange = TRUE;
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

#ifdef IEWIN31_25
        SetScrollRange(pTree->hwnd, SB_VERT, (int)si.nMin, (int)(si.nMax-si.nPage+1), TRUE);
        TV_SetTopItem(pTree, GetScrollPos( pTree->hwnd, SB_VERT));
#else
        TV_SetTopItem(pTree, (UINT)SetScrollInfo(pTree->hwnd, SB_VERT, &si, TRUE));
#endif

    }
    else if (pTree->fVert)
    {
        TV_SetTopItem(pTree, 0);
        SetScrollRange(pTree->hwnd, SB_VERT, 0, 0, TRUE);

        pTree->fVert = FALSE;
        fChange = TRUE;
    }

    if (fChange)
        TV_SizeWnd(pTree, 0, 0);

    return(TRUE);
}

#define MAGIC_HORZLINE 5


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
        UpdateWindow(pTree->hwnd);

    return(fChanged);
}


// ----------------------------------------------------------------------------
//
//  Handles vertical scrolling.
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_VertScroll(PTREE pTree, UINT wCode, UINT wPos)
{
    UINT wNewPos;
    UINT wOldPos = pTree->hTop->iShownIndex;
    BOOL fChanged;

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
            wNewPos = wOldPos + 1;
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
            wNewPos = wPos;
            break;

        case SB_TOP:
            wNewPos = 0;
            break;
    }

    if (fChanged = TV_SetTopItem(pTree, wNewPos))
        UpdateWindow(pTree->hwnd);
    return(fChanged);
}


#ifdef DEBUG
static int nCompares;
#endif

typedef struct {
    LPSTR lpstr;
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
			TV_ITEM sItem;
			char szTemp[MAX_PATH];

			sItem.pszText = szTemp;
			sItem.cchTextMax  = sizeof(szTemp);
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
		nItem = DSA_InsertItem(dsaCmp, 0x7fff, &sCompare);
		if (nItem < 0)
		{
			if (sCompare.bCallBack)
			{
				Str_Set(&sCompare.lpstr, NULL);
			}
			goto Error3;
		}

		if (DPA_InsertPtr(dpaSort, 0x7fff, DSA_GetItemPtr(dsaCmp, nItem)) < 0)
		{
			goto Error3;
		}
	}

	// Sort the DPA, then stick them back under the parent in the new order
	DPA_Sort(dpaSort, lpfnDPACompare ? lpfnDPACompare : TV_DefCompare,
		(LPARAM)pSortCB);

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
                    GetClientRect(pTree->hwnd, &rcClient);
                    // Set to maximal positive number, so the whole rest of
                    // the treeview gets invalidated
                    rcUpdate.bottom = rcClient.bottom;
		}
                if (pTree->fRedraw)
                    InvalidateRect(pTree->hwnd, &rcUpdate, TRUE);
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
	DebugMsg(DM_TRACE, "tv.sort: %ld ms; %d cmps", GetTickCount()-dwTime, nCompares);
#endif
    
    {
        int wNewPos;
        // restore the scroll position
        if (GetWindowStyle(pTree->hwnd) & WS_VSCROLL) {
#ifdef IEWIN31_25
            wNewPos = GetScrollPos( pTree->hwnd, SB_VERT );
#else
            SCROLLINFO si;

            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_POS;
            wNewPos = 0;
            if (GetScrollInfo(pTree->hwnd, SB_VERT, &si)) {
                wNewPos = si.nPos;
            }
#endif //IEWIN31_25

        } else {
            wNewPos = 0;
        }

        if (TV_SetTopItem(pTree, wNewPos))
            UpdateWindow(pTree->hwnd);
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
    return TRUE;
}


BOOL NEAR TV_SortChildrenCB(PTREE pTree, LPTV_SORTCB pSortCB, BOOL bRecurse)
{
	return(TV_SortCB(pTree, pSortCB, bRecurse, TV_CompareItems));
}


BOOL NEAR TV_SortChildren(PTREE pTree, HTREEITEM hParent, BOOL bRecurse)
{
	TV_SORTCB sSortCB;

	sSortCB.hParent = hParent;
	return(TV_SortCB(pTree, &sSortCB, bRecurse, NULL));
}
