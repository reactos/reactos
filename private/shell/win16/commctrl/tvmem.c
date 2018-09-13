#include "ctlspriv.h"
#include "treeview.h"

// in:
//	hItem	item to delete
// 	flags	controls how/what to delete
//		TVDI_NORMAL		delete this node and all children
//		TVDI_NONOTIFY		don't send notify messages
//		TVDI_CHILDRENONLY	just delete the kids (not the item)

void NEAR TV_DeleteItemRecurse(PTREE pTree, TREEITEM FAR * hItem, UINT flags)
{
    TREEITEM FAR *hKid;
    TREEITEM FAR *hNext;
    TREEITEM FAR *hParent;

    ValidateTreeItem(hItem, FALSE);

    // remove all kids (and their kids)
    for (hKid = hItem->hKids; hKid; hKid = hNext) {
        hNext = hKid->hNext;

	// recurse on each child
        TV_DeleteItemRecurse(pTree, hKid, flags & ~TVDI_CHILDRENONLY);
    }

    if ((flags & TVDI_CHILDRENONLY) || !hItem->hParent)
        return;

    if (!(flags & TVDI_NONOTIFY))    // BUGBUG: this is not set by anyone
    {
        NM_TREEVIEW nm;
        // Let the app clean up after itself
        nm.itemOld.hItem = hItem;
        nm.itemOld.lParam = hItem->lParam;
        nm.itemNew.mask = 0;
        nm.itemOld.mask = (TVIF_HANDLE | TVIF_PARAM);
        SendNotify(pTree->hwndParent, pTree->hwnd, TVN_DELETEITEM, &nm.hdr);
    }

    hParent = hItem->hParent;
    Assert(hParent);

    // unlink ourselves from the parent child chain

    if (hParent->hKids == hItem) {
        hParent->hKids = hItem->hNext;
        hKid = NULL;
    } else {
	// not the first child, find our previous item (linear search!)
	hKid = TV_GetNextItem(pTree, hItem, TVGN_PREVIOUS);
	Assert(hKid);
	hKid->hNext = hItem->hNext;
    }

    pTree->cItems--;

    TV_ScrollBarsAfterRemove(pTree, hItem);

    if (hItem->lpstr != LPSTR_TEXTCALLBACK)
        Str_Set(&hItem->lpstr, NULL);

#ifdef DEBUG
    hItem->dbg_sig = 0;
#endif

    // be careful from here down.  hItem is unlinked but
    // still has some valid fields

    // Check to see if the user has deleted one of the
    // special items that is stored in the main tree structure.
    if (hItem == pTree->htiEdit)
        pTree->htiEdit = NULL;

    if (hItem == pTree->hDropTarget)
	pTree->hDropTarget = NULL;

    // if the caret escaped the collapsed area and landed on us, push it away
    if (pTree->hCaret == hItem) {
	HTREEITEM hTemp;
    	if (hItem->hNext)
    	    hTemp = hItem->hNext;
    	else {
    	    hTemp = VISIBLE_PARENT(hItem);
            if (!hTemp)
                hTemp = hKid;  // set above when we unlinked from the previous item
        }
        // Reset the caret to NULL as to not try to reference our
        // invalidated item.
        pTree->hCaret = NULL;
    	TV_SelectItem(pTree, TVGN_CARET, hTemp, !(flags & TVDI_NOSELCHANGE), FALSE, 0);
	Assert(pTree->hCaret != hItem);
    }


    // BUGBUG: might want to really do this
    Assert(pTree->hItemPainting != hItem);

    ControlFree(pTree->hheap, hItem);
}


// ----------------------------------------------------------------------------
//
//  Removes the given item and all children from the tree.
//  Special case: if the given item is the hidden root, all children are
//  removed, but the hidden root is NOT removed.
//
//  sets cItems
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_DeleteItem(PTREE pTree, TREEITEM FAR * hItem, UINT flags)
{
    // BUGBUG: validate hItem

    if (hItem == TVI_ROOT || !hItem)
	hItem = pTree->hRoot;

    // BUGUBG: send TVN_DELETEALLITEMS and TVDI_NONOTIFY if they respond
    // if (hItem == pTree->hRoot)
    //     etc.

    ValidateTreeItem(hItem, FALSE);

    // Collapse first to speed things up (not as much scroll bar recalcs) and
    // to set the top index correctly after the remove.
    TV_Expand(pTree, TVE_COLLAPSE, hItem, FALSE);

    // Invalidate everything below this item; must be done AFTER setting the
    // selection
    if (pTree->style & TVS_HASLINES) {
        if (hItem->hParent == pTree->hRoot || hItem == pTree->hRoot || ITEM_VISIBLE(hItem->hParent)) {
            if (pTree->fRedraw) {
                InvalidateRect(pTree->hwnd, NULL, TRUE);
            }
        } else {
            TV_ScrollBelow(pTree, hItem->hParent, FALSE, FALSE);
        }
    } else {
        TV_ScrollBelow(pTree, hItem, FALSE, FALSE);
    }

    // We can pass in the root to clear all items
    if (hItem == pTree->hRoot)
	flags |= TVDI_CHILDRENONLY;

    TV_DeleteItemRecurse(pTree, hItem, flags);

    Assert(pTree->hRoot); // didn't go too far, did we?

    // maybe everything's gone...
    // check out our cleanup job
    if (!pTree->hRoot->hKids) {
	// the tree itself
	Assert(pTree->cItems == 0);
	pTree->cItems = 0; // just removed it all, didn't we?

	// BUGBUG: this fails because we don't touch hTop if redraw is off
	// in TV_DeleteItemRecurse()
	// AssertMsg(pTree->hTop == NULL, "hTop not NULL, but empty tree");
	pTree->hTop = NULL;

	AssertMsg(pTree->hCaret == NULL, "hCaret not NULL, but empty tree");
	pTree->hCaret = NULL;

	pTree->fNameEditPending = FALSE;
	pTree->cxMax = 0;
	pTree->xPos = 0;

	// the invisible root
	Assert(pTree->hRoot->hNext == NULL);		
	pTree->hRoot->hNext = NULL;
	Assert(pTree->hRoot->hParent == NULL);		
	pTree->hRoot->hParent = NULL;
	Assert(pTree->hRoot->hKids == NULL);		
	pTree->hRoot->hKids = NULL;
	Assert(pTree->hRoot->state & TVIS_EXPANDED);
	pTree->hRoot->state |= TVIS_EXPANDED;
	Assert(pTree->hRoot->iLevel == (BYTE)-1);
	pTree->hRoot->iLevel = (BYTE) -1;
	Assert(pTree->hRoot->iShownIndex == (WORD)-1);
	pTree->hRoot->iShownIndex = (WORD) -1;
    }

    return TRUE;
}


// ----------------------------------------------------------------------------
//
//  Creates the hidden root node for the tree -- all items will trace up to
//  this root, and the first child of the root is the first item in the tree.
//
//  sets hRoot
//
// ----------------------------------------------------------------------------

BOOL NEAR PASCAL TV_CreateRoot(PTREE pTree)
{
    TREEITEM FAR * hRoot = ControlAlloc(pTree->hheap, sizeof(TREEITEM));
    if (!hRoot)
        return FALSE;

    // hRoot->hNext        = NULL;
    // hRoot->hKids        = NULL;
    // hRoot->hParent      = NULL;
    hRoot->iLevel = (BYTE) -1;
    hRoot->state = TVIS_EXPANDED;
    hRoot->iShownIndex = (WORD)-1;
#ifdef DEBUG
    hRoot->dbg_sig = DEBUG_SIG;
#endif

    pTree->hRoot = hRoot;
    return TRUE;
}

#ifdef DEBUG

void NEAR DumpItem(TREEITEM FAR *hItem)
{
    LPSTR p;

    if (hItem->lpstr == LPSTR_TEXTCALLBACK)
	p = "(callback)";
    else if (hItem->lpstr == NULL)
        p = "(null)";
    else
	p = hItem->lpstr;

    DebugMsg(DM_TRACE, p);
    DebugMsg(DM_TRACE, "\tstate:%4.4x show index:%3d level:%2d kids:%ld lparam:%4.4x",
	hItem->state, hItem->iShownIndex,
	hItem->iLevel, hItem->fKids, hItem->lParam);

}

#else
#define DumpItem(hItem)
#endif


// ----------------------------------------------------------------------------
//
//  Adds the item described by the given arguments to the tree.
//
//  sets hTop, cItems
//
// ----------------------------------------------------------------------------

TREEITEM FAR * NEAR TV_InsertItem(PTREE pTree, LPTV_INSERTSTRUCT lpis)
{
    TREEITEM FAR *hNewItem, FAR *hItem;
    TREEITEM FAR *hParent = lpis->hParent;
    TREEITEM FAR *hInsertAfter = lpis->hInsertAfter;

    if (!lpis)
        return NULL; //BUGBUG: Validate LPTV_INSERTSTRUCT

    // don't allow undefined bits
    AssertMsg((lpis->item.mask & ~TVIF_ALL) == 0, "Invalid TVIF mask specified");

    TV_DismissEdit(pTree, FALSE);

    ValidateTreeItem(hParent, TRUE);	// NULL means TVI_ROOT
    ValidateTreeItem(hInsertAfter, FALSE);

    hNewItem = ControlAlloc(pTree->hheap, sizeof(TREEITEM));
    if (!hNewItem)
    {
        DebugMsg(DM_ERROR, "TreeView: Out of memory");
        return NULL;
    }

#ifdef DEBUG
    hNewItem->dbg_sig = DEBUG_SIG;
#endif


    if (lpis->item.mask & TVIF_TEXT)
    {
    	//
    	// We will setup the text string next, before we link our self in
    	// as to handle the case where we run out of memory and need to
    	// destroy ourself without having to unlink.
    	//
    	if (!lpis->item.pszText || (lpis->item.pszText == LPSTR_TEXTCALLBACK))
    	{
    	    hNewItem->lpstr = LPSTR_TEXTCALLBACK;
    	}
    	else
    	{
	    if (!Str_Set(&hNewItem->lpstr, lpis->item.pszText))
    	    {
    	        // Memory allocation failure...
    	        DebugMsg(DM_ERROR, "TreeView: Out of memory");
#ifdef DEBUG
    	        hNewItem->dbg_sig = 0;
#endif
    	        ControlFree(pTree->hheap, hNewItem);
    	        return NULL;
    	    }
    	}
    } else
        Str_Set(&hNewItem->lpstr, c_szNULL);

    AssertMsg(hNewItem->lpstr != NULL, "Item added with NULL text");

    if ((hParent == NULL) || (hParent == TVI_ROOT))
    {
        hParent = pTree->hRoot;
        if (!pTree->hTop)
            pTree->hTop = hNewItem;
    }
    else if (!pTree->hRoot->hKids)
    {
#ifdef DEBUG
	hNewItem->dbg_sig = 0;
#endif
        ControlFree(pTree->hheap, hNewItem);
        return NULL;
    }

    // We will do the sort later, so we can handle TEXTCALLBACK things
    if ((hInsertAfter == TVI_FIRST || hInsertAfter == TVI_SORT) || !hParent->hKids)
    {
        hNewItem->hNext = hParent->hKids;
        hParent->hKids = hNewItem;
    }
    else
    {
        // BUGBUG: we should cache the last insert after pointer to try to
	// catch the case of consecutive adds to the end of a node

        if (hInsertAfter == TVI_LAST)
            for (hItem = hParent->hKids; hItem->hNext; hItem = hItem->hNext)
            	;
        else
        {
            for (hItem = hParent->hKids; hItem->hNext; hItem = hItem->hNext)
                if (hItem == hInsertAfter)
                    break;
        }

        hNewItem->hNext = hItem->hNext;
        hItem->hNext = hNewItem;
    }

    // hNewItem->hKids     = NULL;
    hNewItem->hParent   = hParent;
    hNewItem->iLevel    = hParent->iLevel + 1;
    // hNewItem->iWidth = 0;
    // hNewItem->state = 0;
    if (pTree->hTop == hNewItem)
        hNewItem->iShownIndex = 0; // calc me please!
    else
        hNewItem->iShownIndex = (WORD)-1; // calc me please!

    if (lpis->item.mask & TVIF_IMAGE)
        hNewItem->iImage = lpis->item.iImage;

    if (lpis->item.mask & TVIF_SELECTEDIMAGE)
        hNewItem->iSelectedImage = lpis->item.iSelectedImage;

    if (lpis->item.mask & TVIF_PARAM)
        hNewItem->lParam = lpis->item.lParam;

    if (lpis->item.mask & TVIF_STATE)
        hNewItem->state = lpis->item.state & lpis->item.stateMask;

    if ((hNewItem->state & TVIS_BOLD) && !pTree->hFontBold) //$BOLD
        TV_CreateBoldFont(pTree);                           //$BOLD

    // DebugMsg(DM_TRACE, "Tree: Inserting i = %d state = %d", TV_StateIndex(&lpis->item), lpis->item.state);

    if (lpis->item.mask & TVIF_CHILDREN) {
	switch (lpis->item.cChildren) {
	case I_CHILDRENCALLBACK:
	    hNewItem->fKids = KIDS_CALLBACK;
	    break;

	case 0:
	    hNewItem->fKids = KIDS_FORCE_NO;
	    break;

	default:
	    hNewItem->fKids = KIDS_FORCE_YES;
	    break;
	}
    }

    // accept state bits on create?
    // lpis->item.mask & TVIF_STATE

    pTree->cItems++;

    // I don't want to do any callbacks until the item is completed
    // so sorting waits until the end
    // special case an only child for speed
    // (hKids && hKids->hNext means more than one child)
    if ((hInsertAfter == TVI_SORT) && hParent->hKids && hParent->hKids->hNext)
    {
	TV_ITEM sThisItem, sNextItem;
        char szThis[64], szNext[64];	// BUGBUG: these are too small

	sThisItem.pszText = szThis;
	sThisItem.cchTextMax  = sizeof(szThis);
	TV_GetItem(pTree, hNewItem, TVIF_TEXT, &sThisItem);

	sNextItem.pszText = szNext;
	sNextItem.cchTextMax  = sizeof(szNext);

	// We know that the first kid of hParent is hNewItem
	for (hItem = hNewItem->hNext; hItem; hItem = hItem->hNext)
	{
	    TV_GetItem(pTree, hItem, TVIF_TEXT, &sNextItem);

	    if (lstrcmpi(sThisItem.pszText, sNextItem.pszText) < 0)
	    	break;

	    hInsertAfter = hItem;
	}

	// Check if this is still the first item
	if (hInsertAfter != TVI_SORT)
	{
	    // Move this item from the beginning to where it
	    // should be
	    hParent->hKids = hNewItem->hNext;
	    hNewItem->hNext = hInsertAfter->hNext;
	    hInsertAfter->hNext = hNewItem;
	}
    }


    if ((hNewItem->hNext == pTree->hTop) && !pTree->fVert) {

        // there's no scrollbars and we got added before the top
        // item.  we're now the top.
        hNewItem->iShownIndex = 0;
        pTree->hTop = hNewItem;
    }

    if (pTree->fRedraw)
    {
    	if (TV_ScrollBarsAfterAdd(pTree, hNewItem))
	    TV_ScrollBelow(pTree, hNewItem, FALSE, TRUE);

	// connect the lines, add the buttons, etc. on the item above
	// TV_GetPrevVisItem only works after TV_Scroll* stuff is done
        if (pTree->style & TVS_HASLINES) {
            RECT rc;
            RECT rc2;

            if (TV_GetItemRect(pTree, hNewItem, &rc, FALSE)) {

                // find the previous sibling or the parent if no prev sib.
                if (hParent->hKids == hNewItem) {
                    hItem = hParent;
                } else {
                    hItem = hParent->hKids;
                    while ( hItem->hNext != hNewItem ) {
                        Assert(hItem->hNext);
                        hItem = hItem->hNext;
                    }
                }

                // invalidate from there to the new one
                if (TV_GetItemRect(pTree, hItem, &rc2, FALSE)) {
                    rc2.bottom = rc.bottom;
                    RedrawWindow(pTree->hwnd, &rc2, NULL, RDW_INVALIDATE | RDW_ERASE);
                }
            }
        }
    }

    // DumpItem(hNewItem);

    return hNewItem;
}


// ----------------------------------------------------------------------------
//
//  Frees all allocated memory and objects associated with the tree.
//
// ----------------------------------------------------------------------------

void NEAR TV_DestroyTree(PTREE pTree)
{
    HWND hwnd = pTree->hwnd;

    Assert(pTree->hRoot);

    pTree->fRedraw = FALSE;

    // BUGUBG: send TVN_DELETEALLITEMS and TVDI_NONOTIFY if they respond
    TV_DeleteItem(pTree, pTree->hRoot, TVDI_CHILDRENONLY | TVDI_NOSELCHANGE);

    ControlFree(pTree->hheap, pTree->hRoot);

    if (pTree->hdcBits)
    {
        if (pTree->hBmp)
        {
            SelectObject(pTree->hdcBits, pTree->hStartBmp);
            DeleteObject(pTree->hBmp);
        }

        DeleteDC(pTree->hdcBits);
    }

    if (pTree->fCreatedFont && pTree->hFont)
        DeleteObject(pTree->hFont);

    if (pTree->hFontBold)                    //$BOLD
        DeleteObject(pTree->hFontBold);	     //$BOLD

    NearFree(pTree);

    // Don't try to use this var when window is destroyed...
    SetWindowInt(hwnd, 0, 0);
}


// ----------------------------------------------------------------------------
//
//  Allocates space for the tree and initializes the tree's data
//
// ----------------------------------------------------------------------------

LRESULT NEAR TV_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreate)
{
    PTREE pTree = NearAlloc(sizeof(TREE));

    if (!pTree)
        return -1;	// fail the create window

#ifdef WIN32
    pTree->hheap = GetProcessHeap();
#endif

    if (!TV_CreateRoot(pTree)) {
        NearFree((HLOCAL)pTree);
        return -1;	// fail the create window
    }

    SetWindowInt(hwnd, 0, (UINT)pTree);

    pTree->hwnd       = hwnd;
    pTree->hwndParent = lpCreate->hwndParent;
    pTree->style      = lpCreate->style;
    pTree->fRedraw    = TRUE;

    // pTree->fHorz        = FALSE;
    // pTree->fVert        = FALSE;
    // pTree->fFocus       = FALSE;
    // pTree->fNameEditPending = FALSE;
    // pTree->cxMax        = 0;
    // pTree->cxWnd        = 0;
    // pTree->cyWnd        = 0;
    // pTree->hTop         = NULL;
    // pTree->hCaret       = NULL;
    // pTree->hDropTarget  = NULL;
    // pTree->cItems       = 0;
    // pTree->cShowing     = 0;
    pTree->cFullVisible = 1;
    // pTree->hdcBits      = NULL;
    // pTree->hBmp         = NULL;
    // pTree->hbrBk        = NULL;
    // pTree->xPos         = 0;
    // pTree->cxIndent     = 0;	// init this for real in TV_OnSetFont()

    TV_OnSetFont(pTree, NULL, TRUE);

    SetScrollRange(hwnd, SB_HORZ, 0, 0, TRUE);
    SetScrollRange(hwnd, SB_VERT, 0, 0, TRUE);

    return 0;	// success
}
