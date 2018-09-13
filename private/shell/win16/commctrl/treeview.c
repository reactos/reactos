#include "ctlspriv.h"
#include "treeview.h"
#include "listview.h"

// BUGBUG -- penwin.h is screwy; define local stuff for now
#define HN_BEGINDIALOG			40		// Lens/EditText/garbage detection dialog is about
                                    // to come up on this hedit/bedit
#define HN_ENDDIALOG			   41		// Lens/EditText/garbage detection dialog has
                                    // just been destroyed
//---------------------------------------------------------


#define IDT_SCROLLWAIT 43


HWND NEAR TV_EditLabel(PTREE pTree, HTREEITEM hItem, LPSTR pszInitial);
void NEAR TV_CancelEditTimer(PTREE pTree);
void FAR PASCAL RescrollEditWindow(HWND hwndEdit);



#ifdef DEBUG
void NEAR ValidateTreeItem(TREEITEM FAR * hItem, BOOL bNullOk)
{
    if (hItem) {
	if (HIWORD(hItem) == 0xFFFF) {
	    switch (LOWORD(hItem)) {
#pragma warning(disable:4309)
	    case LOWORD(TVI_ROOT):
	    case LOWORD(TVI_FIRST):
	    case LOWORD(TVI_LAST):
	    case LOWORD(TVI_SORT):
#pragma warning(default:4309)
		break;

	    default:
		DebugMsg(DM_ERROR, "ValidateTreeItem() Invalid special item");
	        DebugBreak();
	    }
	} else if (hItem->dbg_sig != DEBUG_SIG) {
	    DebugMsg(DM_ERROR, "ValidateTreeItem(): BOGUS HTREEITEM %x", hItem);
	    DebugBreak();
	}
	
    } else if (!bNullOk) {
	DebugMsg(DM_ERROR, "ValidateTreeItem(): NULL HTREEITEM");
        DebugBreak();
    }
}
#else
#define ValidateTreeItem(hItem, bNullOk)
#endif

// ----------------------------------------------------------------------------
//
//  Initialize TreeView on library entry -- register SysTreeView class
//
// ----------------------------------------------------------------------------

#pragma code_seg(CODESEG_INIT)

BOOL FAR TV_Init(HINSTANCE hinst)
{
    WNDCLASS wc;

    if (!GetClassInfo(hinst, c_szTreeViewClass, &wc)) {
#ifndef WIN32
#ifndef IEWIN31
	//
	// Use stab WndProc to avoid loading segment on init.
	//
	LRESULT CALLBACK _TV_WndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
    	wc.lpfnWndProc     = _TV_WndProc;
#else
        wc.lpfnWndProc     = TV_WndProc;
#endif
#else
    	wc.lpfnWndProc	   = TV_WndProc;
#endif
        wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
    	wc.hIcon           = NULL;
    	wc.lpszMenuName    = NULL;
    	wc.hInstance       = hinst;
    	wc.lpszClassName   = c_szTreeViewClass;
    	wc.hbrBackground   = NULL;
    	wc.style           = CS_DBLCLKS | CS_GLOBALCLASS;
    	wc.cbWndExtra      = sizeof(PTREE);
    	wc.cbClsExtra      = 0;

        return RegisterClass(&wc);
    }

    return TRUE;
}
#pragma code_seg()


// ----------------------------------------------------------------------------
//
//  Sends a TVN_BEGINDRAG or TVN_BEGINRDRAG notification with information in the ptDrag and
//  itemNew fields of an NM_TREEVIEW structure
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_SendBeginDrag(PTREE pTree, int code, TREEITEM FAR * hItem, int x, int y)
{
    NM_TREEVIEW nm;

    nm.itemNew.hItem = hItem;
    nm.itemNew.state = hItem->state;
    nm.itemNew.lParam = hItem->lParam;
    nm.itemNew.mask = (TVIF_HANDLE | TVIF_STATE | TVIF_PARAM);
    nm.itemOld.mask = 0;
    nm.ptDrag.x = x;
    nm.ptDrag.y = y;

    return (BOOL)SendNotify(pTree->hwndParent, pTree->hwnd, code, &nm.hdr);
}


// ----------------------------------------------------------------------------
//
//  Sends a TVN_ITEMEXPANDING or TVN_ITEMEXPANDED notification with information
//  in the action and itemNew fields of an NM_TREEVIEW structure
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_SendItemExpand(PTREE pTree, int code, TREEITEM FAR * hItem, UINT action)
{
    NM_TREEVIEW nm;

    nm.itemNew.mask = 0;
    nm.itemNew.hItem = hItem;
    nm.itemNew.state = hItem->state;
    nm.itemNew.lParam = hItem->lParam;
    nm.itemNew.iImage = hItem->iImage;
    nm.itemNew.iSelectedImage = hItem->iSelectedImage;
    switch(hItem->fKids) {
        case KIDS_CALLBACK:
        case KIDS_FORCE_YES:
            nm.itemNew.cChildren = 1;
            nm.itemNew.mask = TVIF_CHILDREN;
            break;
        case KIDS_FORCE_NO:
            nm.itemNew.cChildren = 0;
            nm.itemNew.mask = TVIF_CHILDREN;
            break;
    }
    nm.itemNew.mask |= (TVIF_HANDLE | TVIF_STATE | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE);
    nm.itemOld.mask = 0;

    nm.action = action & TVE_ACTIONMASK;

    return (BOOL)SendNotify(pTree->hwndParent, pTree->hwnd, code, &nm.hdr);
}


// ----------------------------------------------------------------------------
//
//  Sends a TVN_SELCHANGING or TVN_SELCHANGED notification with information in
//  the itemOld and itemNew fields of an NM_TREEVIEW structure
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_SendSelChange(PTREE pTree, int code, TREEITEM FAR * hOldItem, TREEITEM FAR * hNewItem, UINT action)
{
    NM_TREEVIEW nm;

    nm.action = action;

    nm.itemNew.hItem = hNewItem;
    nm.itemNew.state = hNewItem ? hNewItem->state : 0;
    nm.itemNew.lParam = hNewItem ? hNewItem->lParam : 0;
    nm.itemNew.mask = (TVIF_HANDLE | TVIF_STATE | TVIF_PARAM);

    nm.itemOld.hItem = hOldItem;
    nm.itemOld.state = hOldItem ? hOldItem->state : 0;
    nm.itemOld.lParam = hOldItem ? hOldItem->lParam : 0;
    nm.itemOld.mask = (TVIF_HANDLE | TVIF_STATE | TVIF_PARAM);

    return (BOOL)SendNotify(pTree->hwndParent, pTree->hwnd, code, &nm.hdr);
}
// ----------------------------------------------------------------------------
//
//  Returns the first visible item above the given item in the tree.
//
// ----------------------------------------------------------------------------

TREEITEM FAR * NEAR TV_GetPrevVisItem(TREEITEM FAR * hItem)
{
    TREEITEM FAR * hParent = hItem->hParent;
    TREEITEM FAR * hWalk;

    ValidateTreeItem(hItem, FALSE);

    if (hParent->hKids == hItem)
	return VISIBLE_PARENT(hItem);

    for (hWalk = hParent->hKids; hWalk->hNext != hItem; hWalk = hWalk->hNext);

checkKids:
    if (hWalk->hKids && (hWalk->state & TVIS_EXPANDED))
    {
        for (hWalk = hWalk->hKids; hWalk->hNext; hWalk = hWalk->hNext);

        goto checkKids;
    }
    return(hWalk);
}


// ----------------------------------------------------------------------------
//
//  Returns the first visible item below the given item in the tree.
//
// ----------------------------------------------------------------------------

TREEITEM FAR * NEAR TV_GetNextVisItem(TREEITEM FAR * hItem)
{
    ValidateTreeItem(hItem, FALSE);

    if (hItem->hKids && (hItem->state & TVIS_EXPANDED))
        return hItem->hKids;

checkNext:
    if (hItem->hNext)
        return(hItem->hNext);

    hItem = hItem->hParent;
    if (hItem)
        goto checkNext;

    return NULL;
}


// ----------------------------------------------------------------------------
//
//  Determine what part of what item is at the given (x,y) location in the
//  tree's client area.  If the location is outside the client area, NULL is
//  returned with the TVHT_TOLEFT, TVHT_TORIGHT, TVHT_ABOVE, and/or TVHT_BELOW
//  flags set in the wHitCode as appropriate.  If the location is below the
//  last item, NULL is returned with wHitCode set to TVHT_NOWHERE.  Otherwise,
//  the item is returned with wHitCode set to either TVHT_ONITEMINDENT,
//  TVHT_ONITEMBUTTON, TVHT_ONITEMICON, TVHT_ONITEMLABEL, or TVHT_ONITEMRIGHT
//
// ----------------------------------------------------------------------------

TREEITEM FAR * NEAR TV_CheckHit(PTREE pTree, int x, int y, UINT FAR *wHitCode)
{
    int index;
    TREEITEM FAR * hItem = pTree->hTop;
    int cxState;

    TV_ITEM sItem;

    *wHitCode = 0;

    if (x < 0)
        *wHitCode |= TVHT_TOLEFT;
    else if (x > (int) pTree->cxWnd)
        *wHitCode |= TVHT_TORIGHT;

    if (y < 0)
        *wHitCode |= TVHT_ABOVE;
    else if (y > (int) pTree->cyWnd)
        *wHitCode |= TVHT_BELOW;

    if (*wHitCode)
        return NULL;

    index = y / pTree->cyItem;

    while (hItem && index--)
        hItem = TV_GetNextVisItem(hItem);

    if (!hItem)
    {
        *wHitCode = TVHT_NOWHERE;
        return NULL;
    }

    x -= hItem->iLevel * pTree->cxIndent;
    x += pTree->xPos;

    if ((pTree->style & (TVS_HASLINES | TVS_HASBUTTONS)) &&
        (pTree->style &TVS_LINESATROOT))
    {
	// Subtract some more to make up for the pluses at the root
	x -= pTree->cxIndent;
    }

    TV_GetItem(pTree, hItem, TVIF_CHILDREN, &sItem);
    cxState = TV_StateIndex(&sItem) ? pTree->cxState : 0;
    if (x <= (int) (hItem->iWidth + pTree->cxImage + cxState))
    {

        if (x >= 0) {
            if (pTree->himlState &&  (x < cxState)) {
                *wHitCode = TVHT_ONITEMSTATEICON;
            } else if (pTree->hImageList && (x < (int) pTree->cxImage + cxState)) {
                *wHitCode = TVHT_ONITEMICON;
            } else {
                *wHitCode = TVHT_ONITEMLABEL;
            }
        } else if ((x >= -pTree->cxIndent) && sItem.cChildren && (pTree->style & TVS_HASBUTTONS))
            *wHitCode = TVHT_ONITEMBUTTON;
        else
            *wHitCode = TVHT_ONITEMINDENT;
    }
    else
        *wHitCode = TVHT_ONITEMRIGHT;

    return hItem;
}


void NEAR TV_SendRButtonDown(PTREE pTree, int x, int y)
{
    BOOL fRet = FALSE;
    UINT wHitCode;
    TREEITEM FAR * hItem = TV_CheckHit(pTree, x, y, &wHitCode);
    TREEITEM FAR * hOldDrop = pTree->hDropTarget;

    if (!TV_DismissEdit(pTree, FALSE))   // end any previous editing (accept it)
        return;     // Something happened such that we should not process button down

    //
    // Need to see if the user is going to start a drag operation
    //

    // Show drop highlight to give some indication the item was clicked
    if (hItem != pTree->hCaret)
    {
	TV_SelectItem(pTree, TVGN_DROPHILITE, hItem, FALSE, TRUE, TVC_BYMOUSE);
    }

    if (CheckForDragBegin(pTree->hwnd, x, y))
    {
        // let them start dragging
	if (hItem)
            TV_SendBeginDrag(pTree, TVN_BEGINRDRAG, hItem, x, y);
    }
    else
    {
        SetFocus(pTree->hwnd);  // Activate this window like listview...
        fRet = !SendNotify(pTree->hwndParent, pTree->hwnd, NM_RCLICK, NULL);
    }

    // Reset the drop highlight before continuing if it's still what we changed
    // it to
    if (pTree->hDropTarget == hItem)
    {
	TV_SelectItem(pTree, TVGN_DROPHILITE, hOldDrop, FALSE, TRUE, TVC_BYMOUSE);
    }

    if (fRet)
        SendMessage(pTree->hwndParent, WM_CONTEXTMENU, (WPARAM)pTree->hwnd, GetMessagePos());
}


// ----------------------------------------------------------------------------
//
//  If the given item is visible in the client area, the rectangle that
//  surrounds that item is invalidated
//
// ----------------------------------------------------------------------------

void NEAR TV_InvalidateItem(PTREE pTree, TREEITEM FAR * hItem, UINT fRedraw)
{
    RECT rc;

    if (pTree->fRedraw && TV_GetItemRect(pTree, hItem, &rc, FALSE))
    {
	RedrawWindow(pTree->hwnd, &rc, NULL, fRedraw);
    }
}

int FAR PASCAL ITEM_OFFSET(PTREE pTree, HTREEITEM hItem)
{
    int i = ((pTree->cxIndent * hItem->iLevel) + pTree->cxImage + pTree->cxState);

    if ((pTree->style & (TVS_HASLINES |TVS_LINESATROOT)) == (TVS_HASLINES | TVS_LINESATROOT))
        i += pTree->cxIndent;

    return i;
}

// ----------------------------------------------------------------------------
//
//  If the given item is visible in the client area, the rectangle that
//  surrounds that item is filled into lprc
//
//  Returns TRUE if the item is shown, FALSE otherwise
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_GetItemRect(PTREE pTree, TREEITEM FAR * hItem, LPRECT lprc, BOOL bItemRect)
{
    UINT iOffset;

    ValidateTreeItem(hItem, FALSE);

    if (!ITEM_VISIBLE(hItem))
	return FALSE;

    iOffset = hItem->iShownIndex - pTree->hTop->iShownIndex;

    if (bItemRect) {
        // Calculate where X position should start...
        lprc->left = -pTree->xPos + ITEM_OFFSET(pTree, hItem);
        if (pTree->himlState) {
            // if this particular one doesn't have an image, subtract off cxState;
            TV_ITEM sItem;

            TV_GetItem(pTree, hItem, 0, &sItem);
            if (!TV_StateIndex(&sItem))
                lprc->left -= pTree->cxState;
        }

        if (!pTree->hImageList)
            lprc->left -= pTree->cxImage;
	lprc->right = lprc->left + hItem->iWidth;
    } else {
        lprc->left = 0;
        lprc->right = pTree->cxWnd;
    }

    lprc->top = iOffset * pTree->cyItem;
    lprc->bottom = lprc->top + pTree->cyItem;

    return TRUE;
}

void NEAR TV_OnSetRedraw(PTREE pTree, BOOL fRedraw)
{
    pTree->fRedraw = TRUE && fRedraw;
    if (pTree->fRedraw)
    {
        if (pTree->hTop) {
            Assert(ITEM_VISIBLE(pTree->hTop));
            pTree->cShowing = TV_UpdateShownIndexes(pTree, pTree->hTop);
        }
        //  Must force recalculation of all tree items to get the right cxMax.
        TV_ScrollBarsAfterSetWidth(pTree, NULL);
        InvalidateRect(pTree->hwnd, NULL, TRUE); //REVIEW: could be smarter
    }
}

void NEAR TV_DoExpandRecurse(PTREE pTree, TREEITEM FAR *hItem, BOOL fNotify)
{
    while (hItem) {
        TV_Expand(pTree, TVE_EXPAND, hItem, fNotify);
        if (hItem->hKids) {
            TV_DoExpandRecurse(pTree, hItem->hKids, fNotify);
        }
        hItem = hItem->hNext;
    }
}


void NEAR TV_ExpandRecurse(PTREE pTree, TREEITEM FAR *hItem, BOOL fNotify)
{
    BOOL fRedraw = pTree->fRedraw;

    TV_OnSetRedraw(pTree, FALSE);
    TV_Expand(pTree, TVE_EXPAND, hItem, fNotify);
    TV_DoExpandRecurse(pTree, hItem->hKids, fNotify);
    TV_OnSetRedraw(pTree, fRedraw);
}


void NEAR TV_ExpandParents(PTREE pTree, TREEITEM FAR *hItem, BOOL fNotify)
{

    if (hItem->hParent) {
        TV_ExpandParents(pTree, hItem->hParent, fNotify);

        // make sure this item is not in a collapsed branch
        if (!(hItem->hParent->state & TVIS_EXPANDED)) {
            TV_Expand(pTree, TVE_EXPAND, hItem->hParent, fNotify);
        }
    }
}

// makes sure an item is expanded and scrolled into view

BOOL NEAR TV_EnsureVisible(PTREE pTree, TREEITEM FAR * hItem)
{
    TV_ExpandParents(pTree, hItem, TRUE);
    return TV_ScrollIntoView(pTree, hItem);
}

// ----------------------------------------------------------------------------
//
//  Notify the parent that the selection is about to change.  If the change is
//  accepted, de-select the current selected item and select the given item
//
//  sets hCaret
//
// in:
//	hItem	item to become selected
//	wType	TVGN_ values (TVGN_CARET, TVGN_DROPHILIGHT are only valid values)
//	fNotify	send notify to parent window
//	fUpdate	do UpdateWindow() to force sync painting
//	action	action code to send identifying how selection is being made
//
//  NOTE: Multiple Selection still needs to be added -- this multiplesel code
//        is garbage
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_SelectItem(PTREE pTree, UINT wType, TREEITEM FAR * hItem, BOOL fNotify,
	BOOL fUpdateNow, UINT action)
{
    UINT uRDWFlags = RDW_INVALIDATE;

    if (pTree->hImageList && (ImageList_GetBkColor(pTree->hImageList) == (COLORREF)-1))
        uRDWFlags |= RDW_ERASE;

    ValidateTreeItem(hItem, TRUE);

    switch (wType) {

    case TVGN_FIRSTVISIBLE:
        if (!hItem)
            return FALSE;

        Assert(ITEM_VISIBLE(hItem));
        TV_EnsureVisible(pTree, hItem);
        if (pTree->fVert) TV_SetTopItem(pTree, hItem->iShownIndex);
	break;

    case TVGN_DROPHILITE:

        Assert(hItem == NULL || ITEM_VISIBLE(hItem));

        if (hItem != pTree->hDropTarget) {

	    if (pTree->hDropTarget) {
		pTree->hDropTarget->state &= ~TVIS_DROPHILITED;
	        TV_InvalidateItem(pTree, pTree->hDropTarget, uRDWFlags);
	    }

	    if (hItem) {
		hItem->state |= TVIS_DROPHILITED;
	        TV_InvalidateItem(pTree, hItem, uRDWFlags);
	    }
            pTree->hDropTarget = hItem;
            
            if (pTree->hCaret) {
                TV_InvalidateItem(pTree, pTree->hCaret, uRDWFlags);
            }

	    if (fUpdateNow)
        	UpdateWindow(pTree->hwnd);
        }
	break;

    case TVGN_CARET:

	// REVIEW: we may want to scroll into view in this case
    	// it's already the selected item, just return
    	if (pTree->hCaret != hItem) {

	    TREEITEM FAR * hOldSel;

    	    if (fNotify && TV_SendSelChange(pTree, TVN_SELCHANGING, pTree->hCaret, hItem, action))
    	        return FALSE;

    	    if (pTree->hCaret) {
    	        pTree->hCaret->state &= ~TVIS_SELECTED;
    	        TV_InvalidateItem(pTree, pTree->hCaret, uRDWFlags);
    	    }

	    hOldSel = pTree->hCaret;
    	    pTree->hCaret = hItem;

    	    if (hItem) {
    	        hItem->state |= TVIS_SELECTED;

    	        // make sure this item is not in a collapsed branch
                TV_ExpandParents(pTree, hItem, fNotify);

    	        TV_InvalidateItem(pTree, hItem, uRDWFlags);

    	        if (action == TVC_BYMOUSE) {
    	            // if selected by mouse, let's wait a doubleclick sec before scrolling
    	            SetTimer(pTree->hwnd, IDT_SCROLLWAIT, GetDoubleClickTime(), NULL);
    	            pTree->fScrollWait = TRUE;
    	        } else if (pTree->fRedraw)
    	            TV_ScrollVertIntoView(pTree, hItem);
    	    }

    	    if (fNotify)
    	        TV_SendSelChange(pTree, TVN_SELCHANGED, hOldSel, hItem, action);

    	    if (fUpdateNow)
	        UpdateWindow(pTree->hwnd);
	}
	break;

    default:
	DebugMsg(DM_TRACE, "Invalid type passed to TV_SelectItem");
	return FALSE;
    }

    return TRUE;	// success
}


// remove all the children, but pretend they are still there

BOOL NEAR TV_ResetItem(PTREE pTree, HTREEITEM hItem)
{
    TV_DeleteItem(pTree, hItem, TVDI_CHILDRENONLY);

    hItem->state &= ~TVIS_EXPANDEDONCE;
    hItem->fKids = KIDS_FORCE_YES;	// force children

    return TRUE;
}


// ----------------------------------------------------------------------------
//
//  Expand or collapse an item's children
//  Returns TRUE if any change took place and FALSE if unchanged
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_Expand(PTREE pTree, UINT wCode, TREEITEM FAR * hItem, BOOL fNotify)
{
    WORD fOldState;
    UINT cntVisDescendants;
    TV_ITEM sItem;

    ValidateTreeItem(hItem, FALSE);

    TV_GetItem(pTree, hItem, TVIF_CHILDREN, &sItem);

    if (!(wCode & TVE_ACTIONMASK) || sItem.cChildren == 0)
        return FALSE;		// no children to expand or collapse

    if ((wCode & TVE_ACTIONMASK) == TVE_TOGGLE)
	wCode = (wCode & ~TVE_ACTIONMASK) | ((hItem->state & TVIS_EXPANDED) ? TVE_COLLAPSE : TVE_EXPAND);

    if (((wCode & TVE_ACTIONMASK) == TVE_EXPAND) && !(hItem->state & TVIS_EXPANDEDONCE))
    {
        // if its the first expand, ALWAYS notify the parent
        fNotify = TRUE;
    }

    // at this point the children may be added if they aren't already there (callback)

    if (fNotify && TV_SendItemExpand(pTree, TVN_ITEMEXPANDING, hItem, wCode))
        return FALSE;

    // if (!hItem->hKids && (hItem->fKids == KIDS_FORCE_NO))	// this may be right, but I don't
								// have proof now.
    if (!hItem->hKids)
    {
        // kids we removed, or never there
        TV_InvalidateItem(pTree, hItem, RDW_INVALIDATE);
        return FALSE;
    }

    fOldState = (hItem->state & TVIS_EXPANDED);

    if (hItem->hParent) // never turn off TVIS_EXPANED for the invisible root
    {
	if ((wCode & TVE_ACTIONMASK) == TVE_EXPAND)
	   hItem->state |= TVIS_EXPANDED;
	else
	   hItem->state &= ~TVIS_EXPANDED;
    }

    if (fOldState == (hItem->state & TVIS_EXPANDED))
    {
	if ((wCode & (TVE_ACTIONMASK | TVE_COLLAPSERESET)) == (TVE_COLLAPSE | TVE_COLLAPSERESET))
	{
	    TV_ResetItem(pTree, hItem);
	}

	return FALSE;
    }

    cntVisDescendants = TV_ScrollBelow(pTree, hItem, TRUE, hItem->state & TVIS_EXPANDED);

    if (hItem->state & TVIS_EXPANDED)
    {
        UINT wNewTop, wTopOffset, wLastKid;

        TV_ScrollBarsAfterExpand(pTree, hItem);

        wNewTop = pTree->hTop->iShownIndex;
        wTopOffset = hItem->iShownIndex - wNewTop;

        wLastKid = wTopOffset + cntVisDescendants + 1;
        if (wLastKid > pTree->cFullVisible)
        {
            wNewTop += min(wLastKid - pTree->cFullVisible, wTopOffset);
            TV_SetTopItem(pTree, wNewTop);
        }
    }
    else
    {
        TV_ScrollBarsAfterCollapse(pTree, hItem);
        TV_ScrollVertIntoView(pTree, hItem);

        if (pTree->hCaret)
        {
            TREEITEM FAR * hWalk = pTree->hCaret;
            int i = hWalk->iLevel - hItem->iLevel;

            for (i = hWalk->iLevel - hItem->iLevel; i > 0; i--)
                hWalk = hWalk->hParent;

            if (hWalk == hItem)
                TV_SelectItem(pTree, TVGN_CARET, hItem, fNotify, TRUE, TVC_UNKNOWN);
        }

    }

    if (fNotify)
        TV_SendItemExpand(pTree, TVN_ITEMEXPANDED, hItem, wCode);

    hItem->state |= TVIS_EXPANDEDONCE;

    if ((wCode & (TVE_ACTIONMASK | TVE_COLLAPSERESET)) == (TVE_COLLAPSE | TVE_COLLAPSERESET))
    {
	TV_ResetItem(pTree, hItem);
    }

    return TRUE;
}

BOOL PASCAL BetweenItems(PTREE pTree, HTREEITEM hItem, HTREEITEM hItemStart, HTREEITEM hItemEnd)
{
    while ((hItemStart = TV_GetNextVisItem(hItemStart)) && (hItemEnd != hItemStart))
    {
        if (hItem == hItemStart)
            return TRUE;
    }
    return FALSE;
}

#ifdef  FE_IME
// Now only Korean version is interested in incremental search with composition string.
#define GET_COMP_STRING(hImc, dwFlags, pszCompStr) \
    { \
        int iNumComp; \
        (pszCompStr) = (PSTR)LocalAlloc(LPTR, 1); \
        if (iNumComp = (int)ImmGetCompositionString((hImc), (dwFlags), NULL, 0)) \
            if ((pszCompStr) = (PSTR)LocalReAlloc((pszCompStr), iNumComp+1, LMEM_MOVEABLE)) \
            { \
                ImmGetCompositionString((hImc), (dwFlags), (pszCompStr), iNumComp+1); \
                (pszCompStr)[iNumComp] = '\0'; \
            } \
    }

#define FREE_COMP_STRING(pszCompStr)    LocalFree((HLOCAL)(pszCompStr))

BOOL NEAR TV_OnImeComposition(PTREE pTree, WPARAM wParam, LPARAM lParam)
{
    LPSTR lpsz;
    int iCycle = 0;
    static HTREEITEM hItemStart = NULL;
    HTREEITEM hItem;
    char szTemp[MAX_PATH];
    TV_ITEM ti;
    LPSTR lpszAlt = NULL; // use only if SameChar
    int iLen;
    HIMC hImc;
    char *pszCompStr;
    BOOL fRet = TRUE;

    if (hImc = ImmGetContext(pTree->hwnd))
    {
        if (lParam & GCS_RESULTSTR)
        {
            fRet = FALSE;
            GET_COMP_STRING(hImc, GCS_RESULTSTR, pszCompStr);
            if (pszCompStr)
            {
                IncrementSearchImeCompStr(FALSE, pszCompStr, &lpsz);
                FREE_COMP_STRING(pszCompStr);
            }
        }
        if (lParam & GCS_COMPSTR)
        {
            fRet = TRUE;
            GET_COMP_STRING(hImc, GCS_COMPSTR, pszCompStr);
            if (pszCompStr)
            {
                if (IncrementSearchImeCompStr(TRUE, pszCompStr, &lpsz)) {
                    if (pTree->hCaret) {
                        hItemStart = pTree->hCaret;
                    } else if (pTree->hRoot && pTree->hRoot->hKids) {
                        hItemStart = pTree->hRoot->hKids;
                    } else
                        return fRet;
                }

                if (!lpsz || !*lpsz || !pTree->hRoot || !pTree->hRoot->hKids)
                    return fRet;

                hItem = hItemStart;
                ti.cchTextMax  = sizeof(szTemp);
                iLen = lstrlen(lpsz);
                if (iLen > 2 && SameDBCSChars(lpsz, (WORD)((BYTE)lpsz[0] << 8 | (BYTE)lpsz[1])))
                    lpszAlt = lpsz + iLen - 2;

                do {
                    ti.pszText = szTemp;
                    hItem = TV_GetNextVisItem(hItem);
                    if (!hItem) {
                        iCycle++;
                        hItem = pTree->hRoot->hKids;
                    }

                    TV_GetItem(pTree, hItem, TVIF_TEXT, &ti);
                    if ((ti.pszText != LPSTR_TEXTCALLBACK) &&
                        HIWORD(ti.pszText)) {
                        // DebugMsg(DM_TRACE, "treesearch %d %s %s", (LPSTR)lpsz, (LPSTR)lpsz, (LPSTR)ti.pszText);
                        if (!StrNCmpI(lpsz, ti.pszText, iLen) ||
                            (lpszAlt && !StrNCmpI(lpszAlt, ti.pszText, 2) &&
                             BetweenItems(pTree, hItem, pTree->hCaret, hItemStart)))
                        {
                            DebugMsg(DM_TRACE, "Selecting");
                            TV_SelectItem(pTree, TVGN_CARET, hItem, TRUE, TRUE, TVC_BYKEYBOARD);
                            return fRet;
                        }
                    }
                }  while(iCycle < 2);

                // if they hit the same key twice in a row at the beginning of
                // the search, and there was no item found, they likely meant to
                // retstart the search
                if (lpszAlt) {

                    // first clear out the string so that we won't recurse again
                    IncrementSearchString(0, NULL);
                    TV_OnImeComposition(pTree, wParam, lParam);
                } else {
                    if (!g_iIncrSearchFailed)
                        MessageBeep(0);
                    g_iIncrSearchFailed++;
                }
                FREE_COMP_STRING(pszCompStr);
            }
        }
        ImmReleaseContext(pTree->hwnd, hImc);
    }
    return fRet;
}
#endif

void NEAR TV_OnChar(PTREE pTree, UINT ch, int cRepeat)
{
    LPSTR lpsz;
    int iCycle = 0;
    static HTREEITEM hItemStart = NULL;
    HTREEITEM hItem;
    char szTemp[MAX_PATH];
    TV_ITEM ti;
    LPSTR lpszAlt = NULL; // use only if SameChar
    int iLen;

    if (IncrementSearchString(ch, &lpsz)) {
        if (pTree->hCaret) {
            hItemStart = pTree->hCaret;
        } else if (pTree->hRoot && pTree->hRoot->hKids) {
            hItemStart = pTree->hRoot->hKids;
        } else
            return;
    }

    if (!lpsz || !*lpsz || !pTree->hRoot || !pTree->hRoot->hKids)
        return;

    hItem = hItemStart;
    ti.cchTextMax  = sizeof(szTemp);
    iLen = lstrlen(lpsz);
    if (iLen > 1 && SameChars(lpsz, lpsz[0]))
        lpszAlt = lpsz + iLen - 1;

    do {
        ti.pszText = szTemp;
        hItem = TV_GetNextVisItem(hItem);
        if (!hItem) {
            iCycle++;
            hItem = pTree->hRoot->hKids;
        }

        TV_GetItem(pTree, hItem, TVIF_TEXT, &ti);
        if ((ti.pszText != LPSTR_TEXTCALLBACK) &&
            HIWORD(ti.pszText)) {
            // DebugMsg(DM_TRACE, "treesearch %d %s %s", (LPSTR)lpsz, (LPSTR)lpsz, (LPSTR)ti.pszText);
            if (!StrNCmpI(lpsz, ti.pszText, iLen) ||
                (lpszAlt && !StrNCmpI(lpszAlt, ti.pszText, 1) &&
                 BetweenItems(pTree, hItem, pTree->hCaret, hItemStart)))
            {
                DebugMsg(DM_TRACE, "Selecting");
                TV_SelectItem(pTree, TVGN_CARET, hItem, TRUE, TRUE, TVC_BYKEYBOARD);
                return;
            }
        }
    }  while(iCycle < 2);

    // if they hit the same key twice in a row at the beginning of
    // the search, and there was no item found, they likely meant to
    // retstart the search
    if (lpszAlt) {

        // first clear out the string so that we won't recurse again
        IncrementSearchString(0, NULL);
        TV_OnChar(pTree, ch, cRepeat);
    } else {
        if (!g_iIncrSearchFailed)
            MessageBeep(0);
        g_iIncrSearchFailed++;
    }
}

// ----------------------------------------------------------------------------
//
//  Handle WM_KEYDOWN messages
//  If control key is down, treat keys as scroll codes; otherwise, treat keys
//  as caret position changes.
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_KeyDown(PTREE pTree, UINT wKey, DWORD dwKeyData)
{
    TREEITEM FAR * hItem;
    UINT wShownIndex;
    TV_KEYDOWN nm;
    BOOL fPuntChar;
    BOOL ret = TRUE;

    // Notify
    nm.wVKey = wKey;
    fPuntChar = (BOOL)SendNotify(pTree->hwndParent, pTree->hwnd, TVN_KEYDOWN, &nm.hdr);

    if (GetKeyState(VK_CONTROL) < 0)
    {
        // control key is down
        UINT wScrollCode;

        switch (wKey)
        {
            case VK_LEFT:
                TV_HorzScroll(pTree, SB_LINEUP, 0);
                break;

            case VK_RIGHT:
                TV_HorzScroll(pTree, SB_LINEDOWN, 0);
                break;

            case VK_PRIOR:
                wScrollCode = SB_PAGEUP;
                goto kdVertScroll;

            case VK_HOME:
                wScrollCode = SB_TOP;
                goto kdVertScroll;

            case VK_NEXT:
                wScrollCode = SB_PAGEDOWN;
                goto kdVertScroll;

            case VK_END:
                wScrollCode = SB_BOTTOM;
                goto kdVertScroll;

            case VK_UP:
                wScrollCode = SB_LINEUP;
                goto kdVertScroll;

            case VK_DOWN:
                wScrollCode = SB_LINEDOWN;
kdVertScroll:
                TV_VertScroll(pTree, wScrollCode, 0);
                break;

            default:
                ret = FALSE;
        }

    } else {

        switch (wKey)
        {
	case VK_RETURN:
            fPuntChar = (BOOL)SendNotify(pTree->hwndParent, pTree->hwnd, NM_RETURN, NULL);
	    break;

        case VK_PRIOR:
            if (pTree->hCaret && (pTree->hCaret->iShownIndex > (pTree->cFullVisible - 1)))
            {
                wShownIndex = pTree->hCaret->iShownIndex - (pTree->cFullVisible - 1);
                goto selectIndex;
            }
            // fall thru

        case VK_HOME:
            wShownIndex = 0;
            goto selectIndex;

        case VK_NEXT:
            if (!pTree->hCaret)
            {
                wShownIndex = 0;
                goto selectIndex;
            }
            wShownIndex = pTree->hCaret->iShownIndex + (pTree->cFullVisible - 1);
            if (wShownIndex < pTree->cShowing)
                goto selectIndex;
            // fall thru

        case VK_END:
            wShownIndex = pTree->cShowing - 1;
selectIndex:
            hItem = TV_GetShownIndexItem(pTree->hRoot->hKids, wShownIndex);
            goto kdSetCaret;
            break;

        case VK_SUBTRACT:

            if (pTree->hCaret) {
                fPuntChar = TRUE;
                TV_Expand(pTree, TVE_COLLAPSE, pTree->hCaret, TRUE);
            }
            break;

        case VK_ADD:
            if (pTree->hCaret) {
                fPuntChar = TRUE;
                TV_Expand(pTree, TVE_EXPAND, pTree->hCaret, TRUE);
            }
            break;

        case VK_MULTIPLY:
            if (pTree->hCaret) {
                fPuntChar = TRUE;
                TV_ExpandRecurse(pTree, pTree->hCaret, TRUE);
            }
            break;

        case VK_LEFT:
            if (pTree->hCaret && (pTree->hCaret->state & TVIS_EXPANDED)) {
                TV_Expand(pTree, TVE_COLLAPSE, pTree->hCaret, TRUE);
                break;
            } else if (pTree->hCaret) {
	        hItem = VISIBLE_PARENT(pTree->hCaret);
                goto kdSetCaret;
	    }
            break;

	case VK_BACK:
	    // get the parent, avoiding the root item
            fPuntChar = TRUE;
	    if (pTree->hCaret) {
	        hItem = VISIBLE_PARENT(pTree->hCaret);
                goto kdSetCaret;
	    }
	    break;

        case VK_UP:
            if (pTree->hCaret)
                hItem = TV_GetPrevVisItem(pTree->hCaret);
            else
                hItem = pTree->hRoot->hKids;

            goto kdSetCaret;
            break;


        case VK_RIGHT:
            if (pTree->hCaret && !(pTree->hCaret->state & TVIS_EXPANDED)) {
                TV_Expand(pTree, TVE_EXPAND, pTree->hCaret, TRUE);
                break;
            } // else fall through

        case VK_DOWN:
            if (pTree->hCaret)
                hItem = TV_GetNextVisItem(pTree->hCaret);
            else
                hItem = pTree->hRoot->hKids;

kdSetCaret:
            if (hItem)
	        TV_SelectItem(pTree, TVGN_CARET, hItem, TRUE, TRUE, TVC_BYKEYBOARD);

            break;

        default:
            ret = FALSE;
        }
    }

    if (fPuntChar) {
        pTree->iPuntChar++;
    } else if (pTree->iPuntChar){
        // this is tricky...  if we want to punt the char, just increment the
        // count.  if we do NOT, then we must clear the queue of WM_CHAR's
        // this is to preserve the iPuntChar to mean "punt the next n WM_CHAR messages
        MSG msg;
        while((pTree->iPuntChar > 0) && PeekMessage(&msg, pTree->hwnd, WM_CHAR, WM_CHAR, PM_REMOVE)) {
            pTree->iPuntChar--;
        }
        Assert(!pTree->iPuntChar);
    }

    return ret;

}


// ----------------------------------------------------------------------------
//
//  Sets the tree's indent width per hierarchy level and recompute widths.
//
//  sets cxIndent
//
// ----------------------------------------------------------------------------

void NEAR TV_SetIndent(PTREE pTree, UINT cxIndent)
{
    if (pTree->hImageList) {
        if ((SHORT)cxIndent < pTree->cxImage)
            cxIndent = pTree->cxImage;
    }
    
    if ((SHORT)cxIndent < pTree->cyText)
        cxIndent = pTree->cyText;

    if (cxIndent < MAGIC_MININDENT)
        cxIndent = MAGIC_MININDENT;

    pTree->cxIndent = cxIndent;

    TV_CreateIndentBmps(pTree);
    TV_ScrollBarsAfterSetWidth(pTree, NULL);
}

// ----------------------------------------------------------------------------
//
//  Sets the tree's item height to be the maximum of the image height and text
//  height.  Then recompute the tree's full visible count.
//
//  sets cyItem, cFullVisible
//
// ----------------------------------------------------------------------------

void NEAR TV_SetItemHeight(PTREE pTree)
{
    // height MUST be even with TVS_HASLINES -- go ahead and make it always even
    pTree->cyItem = (max(pTree->cyImage, pTree->cyText) + 1) & ~1;
    pTree->cFullVisible = pTree->cyWnd / pTree->cyItem;

    TV_CreateIndentBmps(pTree);
    TV_CalcScrollBars(pTree);
}

// BUGBUG: does not deal with hfont == NULL

void NEAR TV_OnSetFont(PTREE pTree, HFONT hNewFont, BOOL fRedraw)
{
    HDC hdc;
    HFONT hfontSel;
    char c = 'J';	// for bog
    SIZE size;

    if (pTree->fCreatedFont && pTree->hFont) {
        DeleteObject(pTree->hFont);
	pTree->fCreatedFont = FALSE;
    }

    if (hNewFont == NULL) {
        LOGFONT lf;
        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
        hNewFont = CreateFontIndirect(&lf);
        pTree->fCreatedFont = TRUE;         // make sure we delete it
    }

    hdc = GetDC(pTree->hwnd);

    hfontSel = hNewFont ? SelectObject(hdc, hNewFont) : NULL;

    GetTextExtentPoint(hdc, &c, 1, &size);
    pTree->cyText = size.cy + (g_cyBorder * 2);

    if (hfontSel)
        SelectObject(hdc, hfontSel);

    ReleaseDC(pTree->hwnd, hdc);

    pTree->hFont = hNewFont;
    if (pTree->hFontBold) {
        TV_CreateBoldFont(pTree);
    }

    if (pTree->cxIndent == 0)	// first time init?
    {
        if (!pTree->cyItem) pTree->cyItem = pTree->cyText;
        TV_SetIndent(pTree, 16 /*g_cxSmIcon*/ + MAGIC_INDENT);
    }

    TV_ScrollBarsAfterSetWidth(pTree, NULL);
    TV_SetItemHeight(pTree);

    // REVIEW: does this happen as a result of the above?
    // if (fRedraw)
    //    RedrawWindow(pTree->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}


VOID NEAR PASCAL TV_CreateBoldFont(PTREE pTree)	
{
    LOGFONT lf;

    if (pTree->hFontBold)
        DeleteObject(pTree->hFontBold);	     //$BOLD

    GetObject(pTree->hFont, sizeof (lf), &lf);
    lf.lfWeight = FW_BOLD;
    pTree->hFontBold = CreateFontIndirect(&lf);
}


HIMAGELIST NEAR TV_SetImageList(PTREE pTree, HIMAGELIST hImage, int iImageIndex)
{
    int cx, cy;
    HIMAGELIST hImageOld = NULL;

    switch (iImageIndex) {

        case TVSIL_STATE:

	    hImageOld = pTree->himlState;
            pTree->himlState = hImage;
            if (hImage) {
                ImageList_GetIconSize(hImage, &pTree->cxState , &pTree->cyState);
            } else {
                pTree->cxState = 0;
            }
            break;

        case TVSIL_NORMAL:
	    hImageOld = pTree->hImageList;
            if (hImage && ImageList_GetIconSize(hImage, &cx, &cy))
            {
                pTree->cxImage = (cx + MAGIC_INDENT);
                pTree->cyImage = cy;
                if (pTree->cxIndent < pTree->cxImage)
                    TV_SetIndent(pTree, pTree->cxImage);
                pTree->hImageList = hImage;
            }
            else
            {
                pTree->cxImage = pTree->cyImage = 0;
                pTree->hImageList = NULL;
            }
            break;

	default:
	    DebugMsg(DM_TRACE, "sh TR - TVM_SETIMAGELIST: unrecognized iImageList");
	    break;

    }

    TV_ScrollBarsAfterSetWidth(pTree, NULL);
    TV_SetItemHeight(pTree);

    return hImageOld;
}


// ----------------------------------------------------------------------------
//
//  Gets the item with the described relationship to the given item, NULL if
//  no item can be found with that relationship.
//
// ----------------------------------------------------------------------------

TREEITEM FAR * NEAR TV_GetNextItem(PTREE pTree, TREEITEM FAR * hItem, UINT wGetCode)
{
    switch (wGetCode) {
    case TVGN_ROOT:
        return pTree->hRoot->hKids;

    case TVGN_DROPHILITE:
	return pTree->hDropTarget;

    case TVGN_CARET:
	return pTree->hCaret;

    case TVGN_FIRSTVISIBLE:
        return pTree->hTop;

    case TVGN_CHILD:
        // Ian's wish for orthogonality
	if (!hItem)	// probably should include || (hItem == TVI_ROOT))
            return pTree->hRoot->hKids;
	break;
    }

    // all of these require a valid hItem
    if (!hItem)
        return NULL;

    ValidateTreeItem(hItem, FALSE);

    switch (wGetCode) {
    case TVGN_NEXTVISIBLE:
        return TV_GetNextVisItem(hItem);

    case TVGN_PREVIOUSVISIBLE:
        return TV_GetPrevVisItem(hItem);

    case TVGN_NEXT:
        return hItem->hNext;

    case TVGN_PREVIOUS:
        if (hItem->hParent->hKids == hItem)
            return NULL;
        else {
            TREEITEM FAR * hWalk;
            for (hWalk = hItem->hParent->hKids; hWalk->hNext != hItem; hWalk = hWalk->hNext);
            return hWalk;
        }

    case TVGN_PARENT:
        return VISIBLE_PARENT(hItem);

    case TVGN_CHILD:
	return hItem->hKids;
    }

    return NULL;
}


// ----------------------------------------------------------------------------
//
//  Returns the number of items (including the partially visible item at the
//  bottom based on the given flag) that fit in the tree's client window.
//
// ----------------------------------------------------------------------------

LRESULT NEAR TV_GetVisCount(PTREE pTree, BOOL fIncludePartial)
{
    int  i;

    if (!fIncludePartial)
        return(MAKELRESULTFROMUINT(pTree->cFullVisible));

    i = pTree->cFullVisible;

    if (pTree->cyWnd - (i * pTree->cyItem))
        i++;

    return i;
}


// ----------------------------------------------------------------------------
//
//  recomputes tree's fields that rely on the tree's client window size
//
//  sets cxWnd, cyWnd, cFullVisible
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_SizeWnd(PTREE pTree, UINT cxWnd, UINT cyWnd)
{
    if (!cxWnd || !cyWnd)
    {
        RECT rc;
        GetClientRect(pTree->hwnd, &rc);
        cxWnd = rc.right;
        cyWnd = rc.bottom;
    }

//    if ((pTree->cxWnd == cxWnd) && (pTree->cyWnd == cyWnd))
//        return FALSE;

    pTree->cxWnd = cxWnd;
    pTree->cyWnd = cyWnd;
    pTree->cFullVisible = cyWnd / pTree->cyItem;
    TV_CalcScrollBars(pTree);
    return TRUE;
}


// ----------------------------------------------------------------------------
//
//  WM_LBUTTONDBLCLK message -- toggle expand/collapse state of item's children
//  WM_LBUTTONDOWN message -- on item's button, do same as WM_LBUTTONDBLCLK,
//  otherwise select item and ensure that item is fully visible
//
// ----------------------------------------------------------------------------

void NEAR TV_ButtonDown(PTREE pTree, UINT wMsg, UINT wFlags, int x, int y)
{
    UINT wHitCode;
    TREEITEM FAR * hItem;

    if (!TV_DismissEdit(pTree, FALSE))   // end any previous editing (accept it)
        return;     // Something happened such that we should not process button down


    hItem = TV_CheckHit(pTree, x, y, &wHitCode);

    if (wMsg == WM_LBUTTONDBLCLK)
    {
        //
        // Cancel any name editing that might happen.
        //
        TV_CancelEditTimer(pTree);

        if (wHitCode & (TVHT_ONITEM | TVHT_ONITEMBUTTON)) {

            if (!SendNotify(pTree->hwndParent, pTree->hwnd, wFlags & MK_RBUTTON ? NM_RDBLCLK : NM_DBLCLK, NULL))
                TV_Expand(pTree, TVE_TOGGLE, hItem, TRUE);
	}

        pTree->fScrollWait = FALSE;

    } else {    // WM_LBUTTONDOWN

            if (wHitCode == TVHT_ONITEMBUTTON)
            {
                if (!SendNotify(pTree->hwndParent, pTree->hwnd, NM_CLICK, NULL)) {
                    TV_Expand(pTree, TVE_TOGGLE, hItem, TRUE);
                }
            }
            else if (wHitCode & TVHT_ONITEM)
            {
                BOOL fSameItem, bDragging;
                TREEITEM FAR * hOldDrop;

                Assert(hItem);

                fSameItem = (hItem == pTree->hCaret);

                // Show drop highlight to give some indication the item was clicked
                hOldDrop = pTree->hDropTarget;
                if (!fSameItem)
                {
                    TV_SelectItem(pTree, TVGN_DROPHILITE, hItem, FALSE, FALSE, TVC_BYMOUSE);
                }

                bDragging =  (pTree->style & TVS_DISABLEDRAGDROP) ?
                    FALSE : CheckForDragBegin(pTree->hwnd, x, y);

                // Reset the drop highlight before continuing, but don't update
                // right away, in case we are just about to change the selection
                TV_SelectItem(pTree, TVGN_DROPHILITE, hOldDrop, FALSE, FALSE, TVC_BYMOUSE);

                if (bDragging)
                {
                    TV_SendBeginDrag(pTree, TVN_BEGINDRAG, hItem, x, y);
                    return;
                }

                if (!SendNotify(pTree->hwndParent, pTree->hwnd, NM_CLICK, NULL)) {
                    // Only set the caret (selection) if not dragging
                    TV_SelectItem(pTree, TVGN_CARET, hItem, TRUE, TRUE, TVC_BYMOUSE);

                    if (fSameItem && (wHitCode & TVHT_ONITEMLABEL) && pTree->fFocus)
                    {
                        //
                        // The item and window are currently selected and user clicked
                        // on label.  Try to enter into name editing mode.
                        //
                        SetTimer(pTree->hwnd, IDT_NAMEEDIT, GetDoubleClickTime(), NULL);
                        pTree->fNameEditPending = TRUE;
                    }
                }
            }
    }

    if (!pTree->fFocus)
        SetFocus(pTree->hwnd);
}


// ----------------------------------------------------------------------------
//
//  Gets the item's text, data, and/or image.
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_OnGetItem(PTREE pTree, LPTV_ITEM ptvi)
{
    // BUGBUG: validate
    if (!ptvi || !ptvi->hItem)
        return FALSE;

    TV_GetItem(pTree, ptvi->hItem, ptvi->mask, ptvi);

    return TRUE;	// success
}


// ----------------------------------------------------------------------------
//
//  Sets the item's text, data, and/or image.
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_SetItem(PTREE pTree, const TV_ITEM FAR *ptvi)
{
    UINT uRDWFlags = RDW_INVALIDATE;
    BOOL fEraseIfTransparent = FALSE;
    BOOL bActualChange = FALSE;	// HACK: We want to keep track of which
				// attributes were changed from CALLBACK to
				// "real", and don't invalidate if those were
				// the only changes

    if (!ptvi || !ptvi->hItem)
        return FALSE;

    ValidateTreeItem(ptvi->hItem, FALSE);

    // BUGBUG: send ITEMCHANING and ITEMCHANGED msgs

    if (ptvi->mask & TVIF_TEXT)
    {
        uRDWFlags = RDW_INVALIDATE |RDW_ERASE;
        bActualChange = TRUE;
        if (ptvi->hItem->lpstr != LPSTR_TEXTCALLBACK)
	{
	    Str_Set(&ptvi->hItem->lpstr, NULL);
	}

        if (!ptvi->pszText || (ptvi->pszText == LPSTR_TEXTCALLBACK))
        {
            ptvi->hItem->lpstr = LPSTR_TEXTCALLBACK;
        }
        else
        {
            if (ptvi->hItem->lpstr == LPSTR_TEXTCALLBACK)
                ptvi->hItem->lpstr = NULL; // otherwise realloc's gonna bomb
	    if (!Str_Set(&ptvi->hItem->lpstr, ptvi->pszText))
            {
                //
                // Memory allocation failed -  The best we can do now
                // is to set the item back to callback, and hope that
                // the top level program can handle it.
                //
                DebugMsg(DM_ERROR, "TreeView: Out of memory");
                ptvi->hItem->lpstr = LPSTR_TEXTCALLBACK;
            }
        }

        ptvi->hItem->iWidth = 0;
        TV_ScrollBarsAfterSetWidth(pTree, ptvi->hItem);
    }

    if (ptvi->mask & TVIF_PARAM)
    {
	bActualChange = TRUE;
	ptvi->hItem->lParam = ptvi->lParam;
    }

    if (ptvi->mask & TVIF_IMAGE)
    {
	if (ptvi->hItem->iImage != (WORD)I_IMAGECALLBACK) {
	    bActualChange = TRUE;
            fEraseIfTransparent = TRUE;
            if (pTree->hImageList && (ImageList_GetBkColor(pTree->hImageList) == (COLORREF)-1))
                uRDWFlags |= RDW_ERASE;

        }
	ptvi->hItem->iImage = ptvi->iImage;
    }

    if (ptvi->mask & TVIF_SELECTEDIMAGE)
    {
	if (ptvi->hItem->iSelectedImage != (WORD)I_IMAGECALLBACK)
	    bActualChange = TRUE;
	ptvi->hItem->iSelectedImage = ptvi->iSelectedImage;
    }

    if (ptvi->mask & TVIF_CHILDREN)
    {
	if (ptvi->hItem->fKids != KIDS_CALLBACK)
	    bActualChange = TRUE;

	if (ptvi->cChildren == I_CHILDRENCALLBACK) {
	    ptvi->hItem->fKids = KIDS_CALLBACK;
	} else {
	    if (ptvi->cChildren)
		ptvi->hItem->fKids = KIDS_FORCE_YES;
	    else
		ptvi->hItem->fKids = KIDS_FORCE_NO;
	}

        //
        // If this item currently has no kid, reset the item.
        //
        if ((ptvi->cChildren == I_CHILDRENCALLBACK) && (ptvi->hItem->hKids == NULL))
        {
            ptvi->hItem->state &= ~TVIS_EXPANDEDONCE;
            if (ptvi->hItem->hParent)
		ptvi->hItem->state &= ~TVIS_EXPANDED;
        }
    }

    if (ptvi->mask & TVIF_STATE)
    {
        UINT change = (ptvi->hItem->state ^ ptvi->state) & ptvi->stateMask;

        if (change)
        {
            // BUGBUG: (TVIS_FOCUSED | TVIS_SELECTED | TVIS_DROPHILITED) changes
            // should effect tree state
            ptvi->hItem->state ^= change;
            bActualChange = TRUE;
            fEraseIfTransparent = TRUE;
            if (ptvi->hItem->state & TVIS_BOLD) {
                if (!pTree->hFontBold)
                    TV_CreateBoldFont(pTree);	
            }

            if (change & TVIS_BOLD) {
                // do this because changing the boldness
                uRDWFlags |= RDW_ERASE;
                ptvi->hItem->iWidth = 0;
                TV_ScrollBarsAfterSetWidth(pTree, ptvi->hItem);
            }
        }
    }

    // force a redraw if something changed AND if we are not
    // inside of a paint of this guy (callbacks will set the
    // item on the paint callback to implement lazy data schemes)

    if (bActualChange && (pTree->hItemPainting != ptvi->hItem))
    {

        if (fEraseIfTransparent) {
            if (pTree->hImageList) {
                if (ImageList_GetBkColor(pTree->hImageList) == CLR_NONE) {
                    uRDWFlags |= RDW_ERASE;
                }
            }

        }
        TV_InvalidateItem(pTree, ptvi->hItem, uRDWFlags);

        // REVIEW: we might need to update the scroll bars if the
        // text length changed!
    }

    return TRUE;
}


// ----------------------------------------------------------------------------
//
//  Calls TV_CheckHit to get the hit test results and then package it in a
//  structure back to the app.
//
// ----------------------------------------------------------------------------

HTREEITEM NEAR TV_OnHitTest(PTREE pTree, LPTV_HITTESTINFO lptvh)
{
    if (!lptvh)
        return 0; //BUGBUG: Validate LPTVHITTEST

    lptvh->hItem = TV_CheckHit(pTree, lptvh->pt.x, lptvh->pt.y, &lptvh->flags);

    return lptvh->hItem;
}


// ----------------------------------------------------------------------------
//
//  TV_Timer
//
//  Checks to see if it is our name editing timer.  If so it  calls of to
//  do name editing
//
// ----------------------------------------------------------------------------
LRESULT NEAR TV_Timer(PTREE pTree, UINT uTimerId)
{
    switch (uTimerId)
    {
        case IDT_NAMEEDIT:
            // Kill the timer as we wont need any more messages from it.
            KillTimer(pTree->hwnd, IDT_NAMEEDIT);

            if (pTree->fNameEditPending)
            {
                // And start name editing mode.
                if (!TV_EditLabel(pTree, pTree->hCaret, NULL))
                {
                    TV_DismissEdit(pTree, FALSE);
                }

                // remove the flag...
                pTree->fNameEditPending = FALSE;
            }
            break;

        case IDT_SCROLLWAIT:
            KillTimer(pTree->hwnd, IDT_SCROLLWAIT);
            if (pTree->fScrollWait)
            {
                if (pTree->hCaret) {
                    TV_ScrollVertIntoView(pTree, pTree->hCaret);
                }
                pTree->fScrollWait = FALSE;
            }
            break;

    }
    return 0;
}

// ----------------------------------------------------------------------------
//
//  TV_Command
//
//  Process the WM_COMMAND.  See if it is an input from our edit windows.
//  if so we may want to dismiss it, and or set it is being dirty...
//
// ----------------------------------------------------------------------------
void NEAR TV_Command(PTREE pTree, int id, HWND hwndCtl, UINT codeNotify)
{
    if ((pTree != NULL) && (hwndCtl == pTree->hwndEdit))
    {
        switch (codeNotify)
        {
        case EN_UPDATE:
            // We will use the ID of the window as a Dirty flag...
            SetWindowID(pTree->hwndEdit, 1);
            TV_SetEditSize(pTree);
            break;

        case EN_KILLFOCUS:
            // We lost focus, so dismiss edit and do not commit changes
            // as if the validation fails and we attempt to display
            // an error message will cause the system to hang!
            if (!TV_DismissEdit(pTree, FALSE))
               return;
            break;

        case HN_BEGINDIALOG: // penwin is bringing up a dialog
            Assert(GetSystemMetrics(SM_PENWINDOWS)); // only on a pen system
            pTree->fNoDismissEdit = TRUE;
            break;

        case HN_ENDDIALOG: // penwin has destroyed dialog
            Assert(GetSystemMetrics(SM_PENWINDOWS)); // only on a pen system
            pTree->fNoDismissEdit = FALSE;
            break;
        }

        // Forward edit control notifications up to parent
        //
        if (IsWindow(hwndCtl))
            FORWARD_WM_COMMAND(pTree->hwndParent, id, hwndCtl, codeNotify, SendMessage);
    }
}

void NEAR TV_OnStyleChanged(PTREE pTree, UINT gwl, LPSTYLESTRUCT pinfo)
{
    // Style changed: redraw everything...
    //
    // try to do this smartly, avoiding unnecessary redraws
    if (gwl == GWL_STYLE)
    {
        DWORD changeFlags;

        TV_DismissEdit(pTree, FALSE);   // Cancels edits

        changeFlags = pTree->style ^ pinfo->styleNew; // those that changed
        pTree->style = pinfo->styleNew;	// change our version

	if (changeFlags & (TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT))
            TV_CreateIndentBmps(pTree);
    }
}

void NEAR TV_OnWinIniChange(PTREE pTree, WPARAM wParam)
{
    if (!wParam ||
        (wParam == SPI_SETNONCLIENTMETRICS) ||
        (wParam == SPI_SETICONTITLELOGFONT)) {

        if (pTree->fCreatedFont)
            TV_OnSetFont(pTree, NULL, TRUE);

        if (!pTree->fIndentSet) {
            // this will validate against the minimum
            TV_SetIndent(pTree, 0);
        }
    }
}

// ----------------------------------------------------------------------------
//
//  TV_WndProc
//
//  Take a guess.
//
// ----------------------------------------------------------------------------

LRESULT CALLBACK TV_WndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    PTREE pTree = (PTREE)GetWindowInt(hwnd, 0);

    switch (wMsg)
    {
        case TVM_INSERTITEM:
            return (LRESULT)TV_InsertItem(pTree, (LPTV_INSERTSTRUCT)lParam);

        case TVM_DELETEITEM:
            // Assume if items are being deleted that name editing is invalid.
            TV_DismissEdit(pTree, FALSE);
            return TV_DeleteItem(pTree, (TREEITEM FAR *)lParam, TVDI_NORMAL);

        case TVM_GETNEXTITEM:
            return (LRESULT)TV_GetNextItem(pTree, (TREEITEM FAR *)lParam, wParam);

	case TVM_GETITEMRECT:
	    // lParam points to hItem to get rect from on input
            if (!lParam)
                return 0;
	    return (LRESULT)TV_GetItemRect(pTree, *(HTREEITEM FAR *)lParam, (LPRECT)lParam, wParam);

        case TVM_GETITEM:
            return (LRESULT)TV_OnGetItem(pTree, (LPTV_ITEM)lParam);

        case TVM_SETITEM:
            return (LRESULT)TV_SetItem(pTree, (const TV_ITEM FAR *)lParam);

        case TVM_ENSUREVISIBLE:
	    return TV_EnsureVisible(pTree, (TREEITEM FAR *)lParam);

        case TVM_SETIMAGELIST:
            return (LRESULT)(UINT)TV_SetImageList(pTree, (HIMAGELIST)lParam, (int)wParam);

        case TVM_EXPAND:
	    // BUGBUG: Validate TREEITEM FAR*
	    if (!lParam)
	        return (LRESULT)NULL;
            return TV_Expand(pTree, wParam, (TREEITEM FAR *)lParam, FALSE);

        case TVM_HITTEST:
            return (LRESULT)TV_OnHitTest(pTree, (LPTV_HITTESTINFO)lParam);

        case TVM_GETCOUNT:
            return MAKELRESULTFROMUINT(pTree->cItems);

        case TVM_GETIMAGELIST:
	    switch (wParam) {
	    case TVSIL_NORMAL:
	        return MAKELRESULTFROMUINT(pTree->hImageList);
	    case TVSIL_STATE:
	        return MAKELRESULTFROMUINT(pTree->himlState);
	    default:
	        return 0;
	    }

        case TVM_GETISEARCHSTRING:
            if (GetFocus() == pTree->hwnd)
                return (LRESULT)GetIncrementSearchString((LPSTR)lParam);
            else
                return 0;

    	case TVM_EDITLABEL:
            return MAKELRESULTFROMUINT(TV_EditLabel(pTree, (HTREEITEM)lParam,
                    (LPSTR)wParam));

        case TVM_GETVISIBLECOUNT:
            return TV_GetVisCount(pTree, (BOOL) wParam);

        case TVM_SETINDENT:
            TV_SetIndent(pTree, (UINT)wParam);
            pTree->fIndentSet = TRUE;
            break;

        case TVM_GETINDENT:
            return MAKELRESULTFROMUINT(pTree->cxIndent);

	case TVM_CREATEDRAGIMAGE:
	    if (!pTree->hImageList || !lParam)
		return (LRESULT)NULL;
	    else
		return MAKELRESULTFROMUINT(TV_CreateDragImage(pTree, (TREEITEM FAR *)lParam));

        case TVM_GETEDITCONTROL:
            return (LRESULT)(UINT)pTree->hwndEdit;

	case TVM_SORTCHILDREN:
	    return TV_SortChildren(pTree, (TREEITEM FAR *)lParam, (BOOL)wParam);

	case TVM_SORTCHILDRENCB:
	    return TV_SortChildrenCB(pTree, (TV_SORTCB FAR *)lParam, (BOOL)wParam);

        case TVM_SELECTITEM:
            return TV_SelectItem(pTree, wParam, (TREEITEM FAR *)lParam, TRUE, TRUE, TVC_UNKNOWN);

	case TVM_ENDEDITLABELNOW:
	    return TV_DismissEdit(pTree, (BOOL)wParam);

#ifdef  FE_IME
    case WM_IME_COMPOSITION:
        // Now only Korean version is interested in incremental search with composition string.
        if (((DWORD)GetKeyboardLayout(0L) & 0xF000FFFFL) == 0xE0000412L)
        {
            if (TV_OnImeComposition(pTree, wParam, lParam))
            {
                lParam &= ~GCS_RESULTSTR;
                goto CallDWP;
            }
            else
                break;
        }
        goto CallDWP;
#endif

        case WM_CHAR:
            if (pTree->iPuntChar) {
                pTree->iPuntChar--;
                return TRUE;
            } else {
                return HANDLE_WM_CHAR(pTree, wParam, lParam, TV_OnChar);
            }

        case WM_CREATE:
            return TV_OnCreate(hwnd, (LPCREATESTRUCT)lParam);

        case WM_DESTROY:
            TV_DestroyTree(pTree);
            break;

        case WM_WININICHANGE:
            TV_OnWinIniChange(pTree, wParam);
            break;

	case WM_STYLECHANGED:
            TV_OnStyleChanged(pTree, wParam, (LPSTYLESTRUCT)lParam);
	    break;

        case WM_SETREDRAW:
            TV_OnSetRedraw(pTree, (BOOL)wParam);
            break;

        case WM_PRINTCLIENT:
        case WM_PAINT:
            TV_Paint(pTree, (HDC)wParam);
            break;
	
        case WM_ERASEBKGND:
            {
            RECT rc;

            TV_GetBackgroundBrush(pTree, (HDC) wParam);
            GetClipBox((HDC) wParam, &rc);
            FillRect((HDC) wParam, &rc, pTree->hbrBk);
            }
	    return TRUE;

        case WM_GETDLGCODE:
            return (LRESULT) (DLGC_WANTARROWS | DLGC_WANTCHARS);

        case WM_HSCROLL:
	    TV_HorzScroll(pTree, GET_WM_HSCROLL_CODE(wParam, lParam), GET_WM_HSCROLL_POS(wParam, lParam));
            break;

        case WM_VSCROLL:
	    TV_VertScroll(pTree, GET_WM_VSCROLL_CODE(wParam, lParam), GET_WM_VSCROLL_POS(wParam, lParam));
            break;

        case WM_KEYDOWN:
            if (TV_KeyDown(pTree, wParam, lParam))
                IncrementSearchString(0, NULL);
            goto CallDWP;

        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
            TV_ButtonDown(pTree, wMsg, wParam, LOWORD(lParam), HIWORD(lParam));
            break;

    case WM_KILLFOCUS:
        if (pTree) {
            pTree->fFocus = FALSE;
            if (pTree->hCaret)
            {
                TV_InvalidateItem(pTree, pTree->hCaret, RDW_INVALIDATE);
                UpdateWindow(pTree->hwnd);
            }
            SendNotify(pTree->hwndParent, pTree->hwnd, NM_KILLFOCUS, NULL);
            IncrementSearchString(0, NULL);
        }
        break;

        case WM_SETFOCUS:
            pTree->fFocus = TRUE;
            if (pTree->hCaret)
            {
                TV_InvalidateItem(pTree, pTree->hCaret, RDW_INVALIDATE);
            }
            else
                TV_SelectItem(pTree, TVGN_CARET, pTree->hTop, TRUE, TRUE, TVC_UNKNOWN);

            SendNotify(pTree->hwndParent, pTree->hwnd, NM_SETFOCUS, NULL);
            break;

        case WM_GETFONT:
            return MAKELRESULTFROMUINT(pTree->hFont);

        case WM_SETFONT:
            TV_OnSetFont(pTree, (HFONT) wParam, (BOOL) lParam);
	    break;

        case WM_SIZE:
            TV_SizeWnd(pTree, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_ENABLE:
	    // HACK: we don't get WM_STYLECHANGE on EnableWindow()
	    if (wParam)
	        pTree->style &= ~WS_DISABLED;	// enabled
	    else
	        pTree->style |= WS_DISABLED;	// disabled
	    TV_CreateIndentBmps(pTree); // This invalidates the whole window!
            break;

        case WM_SYSCOLORCHANGE:
            ReInitGlobalColors();
	    TV_CreateIndentBmps(pTree); // This invalidates the whole window!
            break;

        case WM_RBUTTONDOWN:
            TV_SendRButtonDown(pTree, LOWORD(lParam), HIWORD(lParam));
            break;

        case WM_TIMER:
            TV_Timer(pTree, wParam);
            break;

        case WM_COMMAND:
            TV_Command(pTree, (int)GET_WM_COMMAND_ID(wParam, lParam), GET_WM_COMMAND_HWND(wParam, lParam),
                    (UINT)GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        case WM_MBUTTONDOWN:
            SetFocus(hwnd);
            goto CallDWP;

        case WM_SYSKEYDOWN:
            TV_KeyDown(pTree, wParam, lParam);
            //fall through

        default:
CallDWP:
            return(DefWindowProc(hwnd, wMsg, wParam, lParam));
    }

    return(0L);
}

// NOTE: there is very similar code in the listview
//
// Totally disgusting hack in order to catch VK_RETURN
// before edit control gets it.
//
LRESULT CALLBACK TV_EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PTREE pTree = (PTREE)GetWindowInt(GetParent(hwnd), 0);
    Assert(pTree);

    switch (msg) {
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_RETURN:
            TV_DismissEdit(pTree, FALSE);
            return 0L;

        case VK_ESCAPE:
            TV_DismissEdit(pTree, TRUE);
            return 0L;
        }
        break;

    case WM_CHAR:
        switch (wParam) {
        case VK_RETURN:
            // Eat the character, so edit control wont beep!
            return 0L;
        }
    }

    return CallWindowProc(pTree->pfnEditWndProc, hwnd, msg, wParam, lParam);
}


void NEAR TV_SetEditSize(PTREE pTree)
{
    RECT rcLabel;

    if (pTree->htiEdit == NULL)
        return;

    TV_GetItemRect(pTree, pTree->htiEdit, &rcLabel, TRUE);

    // get exact the text bounds (acount for borders used when drawing)

    InflateRect(&rcLabel, -g_cxLabelMargin, -g_cyBorder);

    SetEditInPlaceSize(pTree->hwndEdit, &rcLabel, pTree->hFont, FALSE);
}


void NEAR TV_CancelEditTimer(PTREE pTree)
{
    if (pTree->fNameEditPending)
    {
        KillTimer(pTree->hwnd, IDT_NAMEEDIT);
        pTree->fNameEditPending = FALSE;
    }
}

// BUGBUG: very similar code in lvicon.c


HWND NEAR TV_EditLabel(PTREE pTree, HTREEITEM hItem, LPSTR pszInitial)
{
    char szLabel[MAX_PATH];
    TV_DISPINFO nm;

    if (!hItem || !(pTree->style & TVS_EDITLABELS))
        return NULL;

    TV_DismissEdit(pTree, FALSE);


    // Now get the text associated with that item
    nm.item.pszText = szLabel;
    nm.item.cchTextMax = sizeof(szLabel);
    nm.item.stateMask = TVIS_BOLD;
    TV_GetItem(pTree, hItem, TVIF_TEXT | TVIF_STATE, &nm.item);

    pTree->hwndEdit = CreateEditInPlaceWindow(pTree->hwnd,
        pszInitial? pszInitial : nm.item.pszText, sizeof(szLabel),
        WS_BORDER | WS_CLIPSIBLINGS | WS_CHILD | ES_LEFT | ES_AUTOHSCROLL,
        (nm.item.state & TVIS_BOLD) ? pTree->hFontBold : pTree->hFont);

    if (pTree->hwndEdit) {
        //
        // Now notify the parent of this window and see if they want it.
        // We do it after we cretae the window, but before we show it
        // such that our parent can query for it and do things like limit
        // the number of characters that are input
        nm.item.hItem = hItem;
        nm.item.state = hItem->state;
        nm.item.lParam = hItem->lParam;
        nm.item.mask = (TVIF_HANDLE | TVIF_STATE | TVIF_PARAM | TVIF_TEXT );

        if ((BOOL)SendNotify(pTree->hwndParent, pTree->hwnd, TVN_BEGINLABELEDIT, &nm.hdr))
        {
            DestroyWindow(pTree->hwndEdit);
            pTree->hwndEdit = NULL;
            return NULL;
        }

        TV_ScrollIntoView(pTree, hItem);

        // Ok To continue - so Show the window and set focus to it.
        SetFocus(pTree->hwndEdit);
        ShowWindow(pTree->hwndEdit, SW_SHOW);

        pTree->pfnEditWndProc = SubclassWindow(pTree->hwndEdit, TV_EditWndProc);

        pTree->htiEdit = hItem;

        TV_SetEditSize(pTree);
        RescrollEditWindow(pTree->hwndEdit);
    }

    return pTree->hwndEdit;
}


// BUGBUG: very similar code in lvicon.c

BOOL NEAR TV_DismissEdit(PTREE pTree, BOOL fCancel)
{
    HWND hwndEdit;
    BOOL fOkToContinue = TRUE;
    HTREEITEM htiEdit;

    if (pTree->fNoDismissEdit)
        return FALSE;

    hwndEdit = pTree->hwndEdit;

    if (!hwndEdit) {
        // Also make sure there are no pending edits...
        TV_CancelEditTimer(pTree);
        return TRUE;
    }

    // Assume that if we are not visible that the window is in the
    // process of being destroyed and we should not process the
    // editing of the window...
    if (!IsWindowVisible(pTree->hwnd))
        fCancel = TRUE;

    //
    // We are using the Window ID of the control as a BOOL to
    // state if it is dirty or not.
    switch (GetWindowID(hwndEdit)) {
    case 0:
        // The edit control is not dirty so act like cancel.
        fCancel = TRUE;
        //  FALL THROUGH
    case 1:
        // The edit control is dirty so continue.
        SetWindowID(hwndEdit, 2);    // Don't recurse
        break;
    case 2:
        // We are in the process of processing an update now, bail out
        return TRUE;
    }

    // BUGBUG: this will fail if the program deleted the items out
    // from underneath us (while we are waiting for the edit timer).
    // make delete item invalidate our edit item
    htiEdit = pTree->htiEdit;

    ValidateTreeItem(htiEdit, FALSE);

    if (htiEdit != NULL)
    {
        TV_DISPINFO nm;
        char szLabel[MAX_PATH];

    	// Initialize notification message.
    	nm.item.hItem = htiEdit;
    	nm.item.lParam = htiEdit->lParam;
	nm.item.mask = 0;

    	if (fCancel)
    	    nm.item.pszText = NULL;
    	else {
    	    Edit_GetText(hwndEdit, szLabel, sizeof(szLabel));
    	    nm.item.pszText = szLabel;
    	}

        // Make sure the text redraws properly
	TV_InvalidateItem(pTree, htiEdit, RDW_INVALIDATE | RDW_ERASE);
        pTree->fNoDismissEdit = TRUE; // this is so that we don't recurse due to killfocus
	ShowWindow(hwndEdit, SW_HIDE);
        pTree->fNoDismissEdit = FALSE;
	
        //
        // Notify the parent that we the label editing has completed.
        // We will use the LV_DISPINFO structure to return the new
        // label in.  The parent still has the old text available by
        // calling the GetItemText function.
        //

        fOkToContinue = (BOOL)SendNotify(pTree->hwndParent, pTree->hwnd, TVN_ENDLABELEDIT, &nm.hdr);
        if (fOkToContinue && !fCancel)
        {
            //
            // If the item has the text set as CALLBACK, we will let the
            // ower know that they are supposed to set the item text in
            // their own data structures.  Else we will simply update the
            // text in the actual view.
            //
            // Note: The callee may have set the handle to null to tell
            // us that the handle to item is no longer valid.
            if (nm.item.hItem != NULL)
            {
                if (htiEdit->lpstr != LPSTR_TEXTCALLBACK)
                {
                    // Set the item text (everything's set up in nm.item)
                    //
		    nm.item.mask = TVIF_TEXT;
                    TV_SetItem(pTree, &nm.item);
                }
                else
                {
                    SendNotify(pTree->hwndParent, pTree->hwnd, TVN_SETDISPINFO, &nm.hdr);
                }
            }
        }
    }

    // If we did not reenter edit mode before now reset the edit state
    // variables to NULL
    if (hwndEdit == pTree->hwndEdit)
    {
        pTree->htiEdit = NULL;
        pTree->hwndEdit = NULL;	// so we don't get reentered on the kill focus
    }

    // done with the edit control
    DestroyWindow(hwndEdit);

    return fOkToContinue;
}
