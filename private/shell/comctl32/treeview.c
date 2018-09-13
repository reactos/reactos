#include "ctlspriv.h"
#include "treeview.h"
#include "listview.h"

// BUGBUG -- penwin.h is screwy; define local stuff for now
#define HN_BEGINDIALOG                  40              // Lens/EditText/garbage detection dialog is about
                                    // to come up on this hedit/bedit
#define HN_ENDDIALOG                       41           // Lens/EditText/garbage detection dialog has
                                    // just been destroyed

//---------------------------------------------------------
#define IDT_SCROLLWAIT 43

//-----------------------
// ToolTip stuff...
//
#define REPEATTIME      SendMessage(pTree->hwndToolTips,TTM_GETDELAYTIME,(WPARAM)TTDT_RESHOW, 0)
#define CHECKFOCUSTIME  (REPEATTIME)
#define IDT_TOOLTIPWAIT   2
#define IDT_FOCUSCHANGE   3
// in tooltips.c
BOOL ChildOfActiveWindow(HWND hwnd);
void TV_HandleStateIconClick(PTREE pTree, HTREEITEM hItem);

HWND TV_EditLabel(PTREE pTree, HTREEITEM hItem, LPTSTR pszInitial);
void TV_CancelEditTimer(PTREE pTree);
BOOL TV_SetItem(PTREE pTree, LPCTVITEMEX ptvi);
void TV_DeleteHotFonts(PTREE pTree);
BOOL TV_IsShowing(HTREEITEM hItem);

LRESULT TV_OnScroll(PTREE ptv, LPNMHDR pnm);

#define TVBD_FROMWHEEL      0x0001
#define TVBD_WHEELFORWARD   0x0002
#define TVBD_WHEELBACK      0x0004

BOOL ValidateTreeItem(TREEITEM FAR * hItem, UINT flags)
{
    BOOL fValid = TRUE;

    /*
     *  Check the values to make sure the new Win64-compatible values
     *  are consistent with the old Win32 values.
     */
    COMPILETIME_ASSERT(
           (DWORD)(ULONG_PTR)TVI_ROOT  == 0xFFFF0000 &&
           (DWORD)(ULONG_PTR)TVI_FIRST == 0xFFFF0001 &&
           (DWORD)(ULONG_PTR)TVI_LAST  == 0xFFFF0002 &&
           (DWORD)(ULONG_PTR)TVI_SORT  == 0xFFFF0003);

    if (hItem) {
        if (HIWORD64(hItem) == HIWORD64(TVI_ROOT)) {
            switch (LOWORD(hItem)) {
//#pragma warning(disable:4309)
            case LOWORD(TVI_ROOT):
            case LOWORD(TVI_FIRST):
            case LOWORD(TVI_LAST):
            case LOWORD(TVI_SORT):
//#pragma warning(default:4309)
                break;

            default:
                AssertMsg(FALSE, TEXT("ValidateTreeItem() Invalid special item"));
                fValid = FALSE;
                break;
            }
        } else {
            __try {
                // Use "volatile" to force memory access at start of struct
                *(volatile LPVOID *)hItem;
                fValid = hItem->wSignature == TV_SIG;
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                fValid = FALSE;
            } __endexcept
        }

    } else if (!flags) {            // The only flag is VTI_NULLOK
        DebugMsg(DM_ERROR, TEXT("ValidateTreeItem(): NULL HTREEITEM"));
        fValid = FALSE;
    }

    return fValid;
}

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
        //
        // Use stab WndProc to avoid loading segment on init.
        //
        LRESULT CALLBACK _TV_WndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
        wc.lpfnWndProc     = _TV_WndProc;
#else
        wc.lpfnWndProc     = TV_WndProc;
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
// If the tooltip bubble is up, then pop it.
//
// ----------------------------------------------------------------------------

void TV_PopBubble(PTREE pTree)
{
    if (pTree->hwndToolTips && pTree->hToolTip)
    {
        pTree->hToolTip = NULL;
        SendMessage(pTree->hwndToolTips, TTM_POP, 0L, 0L);
    }
}


// ----------------------------------------------------------------------------
//
//  Sends a TVN_BEGINDRAG or TVN_BEGINRDRAG notification with information in the ptDrag and
//  itemNew fields of an NM_TREEVIEW structure
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_SendBeginDrag(PTREE pTree, int code, TREEITEM FAR * hItem, int x, int y)
{
    NM_TREEVIEW nm;
        
    TV_PopBubble(pTree);            // dismiss the infotip if we start to drag

    nm.itemNew.hItem = hItem;
    nm.itemNew.state = hItem->state;
    nm.itemNew.lParam = hItem->lParam;
    nm.itemNew.mask = (TVIF_HANDLE | TVIF_STATE | TVIF_PARAM);
    nm.itemOld.mask = 0;
    nm.ptDrag.x = x;
    nm.ptDrag.y = y;

    return (BOOL)CCSendNotify(&pTree->ci, code, &nm.hdr);
}


// ----------------------------------------------------------------------------
//
//  Sends a TVN_ITEMEXPANDING or TVN_ITEMEXPANDED notification with information
//  in the action and itemNew fields of an NM_TREEVIEW structure
//
//  Returns FALSE to allow processing to continue, or TRUE to stop.
//
//  If the hItem is destroyed by the callback, then we always return TRUE.
//
//  Note that the application cannot stop a TVN_ITEMEXPANDED, so the only
//  way a TVN_ITEMEXPANDED can return "Stop" is if the item got destroyed.
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_SendItemExpand(PTREE pTree, int code, TREEITEM FAR * hItem, WPARAM action)
{
    NM_TREEVIEW nm;
    TVWATCHEDITEM wi;
    BOOL fResult;
    BOOL fWatched;

    ASSERT(code == TVN_ITEMEXPANDING || code == TVN_ITEMEXPANDED);

    nm.itemNew.mask = 0;
    nm.itemNew.hItem = hItem;
    if (hItem == TVI_ROOT)
        hItem = pTree->hRoot;
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

    nm.action = (UINT)(action & TVE_ACTIONMASK);

    //
    //  Some apps will delete the item while it is being expanded, since
    //  during expansion, they will realize, "Hey, the thing represented
    //  by this item no longer exists, I'd better delete it."  (E.g,.
    //  Explorer.)  So keep an eye on the item so we don't fault when
    //  this happens.
    //

    // If we can't start a watch, then tough, just send the notification
    // the unsafe way.
    fWatched = TV_StartWatch(pTree, &wi, hItem);

    fResult = (BOOL)CCSendNotify(&pTree->ci, code, &nm.hdr);

    // The app return code from TVN_ITEMEXPANDED is ignored.
    // You can't stop a TVN_ITEMEXPANDED; it's already happened.
    if (code == TVN_ITEMEXPANDED)
        fResult = FALSE;                // Continue processing

    if (fWatched) {
        if (!TV_IsWatchValid(pTree, &wi))
            fResult = TRUE;             // Oh no!  Stop!

        TV_EndWatch(pTree, &wi);
    }

    return fResult;
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

    return (BOOL)CCSendNotify(&pTree->ci, code, &nm.hdr);
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

    DBG_ValidateTreeItem(hItem, 0);

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
    DBG_ValidateTreeItem(hItem, 0);

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
    TREEITEM FAR * hItem = pTree->hTop;
    int cxState;

    TVITEMEX sItem;

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

    {
        int index = y / pTree->cyItem;

        while (hItem && index >= hItem->iIntegral) {
            index -= hItem->iIntegral;
            hItem = TV_GetNextVisItem(hItem);
        }
    }

    if (!hItem)
    {
        *wHitCode = TVHT_NOWHERE;
        return NULL;
    }

    x -= (pTree->cxBorder + (hItem->iLevel * pTree->cxIndent));
    x += pTree->xPos;

    if ((pTree->ci.style & (TVS_HASLINES | TVS_HASBUTTONS)) &&
        (pTree->ci.style &TVS_LINESATROOT))
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
        } else if ((x >= -pTree->cxIndent) && sItem.cChildren && (pTree->ci.style & TVS_HASBUTTONS))
            *wHitCode = TVHT_ONITEMBUTTON;
        else
            *wHitCode = TVHT_ONITEMINDENT;
    }
    else
        *wHitCode = TVHT_ONITEMRIGHT;

    return hItem;
}

//  This is tricky because CheckForDragBegin yields and the app may have
//  destroyed the item we are thinking about dragging
//
//  To give the app some feedback, we give the hItem the drop highlight
//  if it isn't already the caret.  This also allows us to check if the
//  item got deleted behind our back - TV_DeleteItemRecurse makes sure
//  that deleted items are never the hCaret or hDropTarget.
//
//  After TV_CheckForDragBegin, the caller must call TV_FinishCheckDrag
//  to clean up the UI changes that TV_CheckForDragBegin temporarily
//  performed.
//
BOOL TV_CheckForDragBegin(PTREE pTree, HTREEITEM hItem, int x, int y)
{
    BOOL fDrag;

    //
    //  If the item is not the caret, then make it the (temporary)
    //  drop target so the user gets some feedback.
    //
    //  BUGBUG raymondc - If hItem == pTree->hCaret, it still might not
    //  be visible if the control doesn't yet have focus and the treeview
    //  is not marked showselalways.  Maybe we should just always set
    //  hItem to DROPHILITE.
    //
    if (hItem == pTree->hCaret)
    {
        pTree->hOldDrop = NULL;
        pTree->fRestoreOldDrop = FALSE;
    }
    else
    {
        pTree->hOldDrop = pTree->hDropTarget;
        pTree->fRestoreOldDrop = TRUE;
        TV_SelectItem(pTree, TVGN_DROPHILITE, hItem, 0, TVC_BYMOUSE);
        ASSERT(hItem == pTree->hDropTarget);
    }

    //
    //  We are dragging the hItem if CheckForDragBegin says okay,
    //  and TV_DeleteItemRecurse didn't wipe us out.
    //
    fDrag = CheckForDragBegin(pTree->ci.hwnd, x, y) &&
           (hItem == pTree->hDropTarget || hItem == pTree->hCaret);

    return fDrag;
}

void TV_FinishCheckDrag(PTREE pTree)
{
    //
    //  Clean up our temporary UI changes that happened when we started
    //  dragging.
    //
    if (pTree->fRestoreOldDrop)
    {
        HTREEITEM hOldDrop = pTree->hOldDrop;
        pTree->fRestoreOldDrop = FALSE;
        pTree->hOldDrop = NULL;
        TV_SelectItem(pTree, TVGN_DROPHILITE, hOldDrop, 0, TVC_BYMOUSE);
    }
}

void NEAR TV_SendRButtonDown(PTREE pTree, int x, int y)
{
    BOOL fRet = FALSE;
    UINT wHitCode;
    TREEITEM FAR * hItem = TV_CheckHit(pTree, x, y, &wHitCode);
    HWND hwnd = pTree->ci.hwnd;

    if (!TV_DismissEdit(pTree, FALSE))   // end any previous editing (accept it)
        return;     // Something happened such that we should not process button down

    //
    // Need to see if the user is going to start a drag operation
    //

    GetMessagePosClient(pTree->ci.hwnd, &pTree->ptCapture);

    if (TV_CheckForDragBegin(pTree, hItem, x, y))
    {
        // let them start dragging
        if (hItem)
        {
            pTree->htiDrag = hItem;
            TV_SendBeginDrag(pTree, TVN_BEGINRDRAG, hItem, x, y);
        }
    }
    else if (!IsWindow(hwnd))
    {
        return;             // bail!
    }
    else
    {
        SetFocus(pTree->ci.hwnd);  // Activate this window like listview...
        fRet = !CCSendNotify(&pTree->ci, NM_RCLICK, NULL);
    }

    // Don't finish the CheckForDragBegin until after the NM_RCLICK
    // because apps want to display the context menu while the
    // temporary drag UI is still active.
    TV_FinishCheckDrag(pTree);

    if (fRet)
        SendMessage(pTree->ci.hwndParent, WM_CONTEXTMENU, (WPARAM)pTree->ci.hwnd, GetMessagePos());
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

    if (hItem && pTree->fRedraw && TV_GetItemRect(pTree, hItem, &rc, FALSE))
    {
        RedrawWindow(pTree->ci.hwnd, &rc, NULL, fRedraw);
    }
}

//
//  Given an item, compute where the text of this item ends up being painted.
//  Basically, stare at TV_DrawItem and dutifully reproduce all the code that
//  messes with the x-coordinate.
//
int FAR PASCAL ITEM_OFFSET(PTREE pTree, HTREEITEM hItem)
{
    int x = pTree->cxBorder + (hItem->iLevel * pTree->cxIndent);

    // state image
    // BUGBUG -- doesn't handle TVCDRF_NOIMAGES - whose idea was that?
    if (pTree->himlState && TV_StateIndex(hItem))
        x += pTree->cxState;

    // image
    if (pTree->hImageList) {
        // even if not drawing image, draw text in right place
        x += pTree->cxImage;
    }
    
    // "plus" at the front of the tree
    if ((pTree->ci.style & TVS_LINESATROOT) &&
        (pTree->ci.style & (TVS_HASLINES | TVS_HASBUTTONS)))
        x += pTree->cxIndent;


    return x;
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

    if (!hItem)
        return FALSE;

    DBG_ValidateTreeItem(hItem, 0);

    if (!ITEM_VISIBLE(hItem))
        return FALSE;

    iOffset = hItem->iShownIndex - pTree->hTop->iShownIndex;

    if (bItemRect) {
        // Calculate where X position should start...
        lprc->left = -pTree->xPos + ITEM_OFFSET(pTree, hItem);
        lprc->right = lprc->left + hItem->iWidth;
    } else {
        lprc->left = 0;
        lprc->right = pTree->cxWnd;
    }

    lprc->top = iOffset * pTree->cyItem;
    lprc->bottom = lprc->top + (pTree->cyItem * hItem->iIntegral) ;

    return TRUE;
}

void NEAR TV_OnSetRedraw(PTREE pTree, BOOL fRedraw)
{
    pTree->fRedraw = TRUE && fRedraw;
    if (pTree->fRedraw)
    {
        // This use to only refresh the items from hTop down, this is bad as if items are inserted
        // before the visible point within the tree then we would fail!
        if ( pTree->hRoot )
            pTree->cShowing = TV_UpdateShownIndexes(pTree,pTree->hRoot);

        //  Must force recalculation of all tree items to get the right cxMax.
        TV_ScrollBarsAfterSetWidth(pTree, NULL);
        InvalidateRect(pTree->ci.hwnd, NULL, TRUE); //REVIEW: could be smarter
    }
}

//  Treeview item watching implementation
//
//  You need to "watch" an item any time you hold onto its HTREEITEM
//  and then yield control to the application.  If you didn't watch
//  the item, then if the app deletes the item, you end up with a
//  stale HTREEITEM pointer and fault.
//
//  To begin watching an item, call TV_StartWatch with the item you
//  want to start watching.  When finished watching, call TV_EndWatch.
//
//  In between, you can call TV_IsWatchStale() which tells you if the
//  item has been deleted behind your back and you shouldn't use it.
//  Alternatively, use TV_IsWatchValid() which says if it's okay.
//
//  Additional bonus behavior for enumeration:  If the watched item
//  is deleted, we cache the hNext item so that you can step to the
//  item after the one that got deleted.  Note that this works even
//  if the hNext item gets deleted before you get a chance to look,
//  because we just move the cached item to the hNext's hNext.
//
//  Sample usage for watching:
//
//  TVWATCHEDITEM wi;
//  if (TV_StartWatch(pTree, &wi, htiStartHere)) {
//      FunctionThatYields();
//      if (TV_IsWatchValid(pTree, &wi)) {
//          KeepUsing(htiStartHere);
//      } else {
//          // item was deleted while we yielded; stop using it
//      }
//      TV_EndWatch(pTree, &wi);
//  }
//
//  Sample usage for enumerating:
//
//  TVWATCHEDITEM wi;
//  if (TV_StartWatch(pTree, &wi, htiFirst)) {
//      while (TV_GetWatchItem(pTree, &wi)) {
//          FunctionThatYields(TV_GetWatchItem(pTree, &wi));
//          if (TV_IsWatchValid(pTree, &wi)) {
//              KeepUsing(htiStartHere);
//          } else {
//              // item was deleted while we yielded; stop using it
//          }
//          TV_NextWatchItem(pTree, &wi);
//      }
//      TV_EndWatch(pTree, &wi);
//  }
//
//
//

//
//  TV_StartWatch - Begin watching an item.
//
//  Returns FALSE if out of memory.
//
BOOL TV_StartWatch(PTREE pTree, PTVWATCHEDITEM pwi, HTREEITEM htiStart)
{
    pwi->hti = htiStart;
    pwi->fStale = FALSE;
    return DPA_AppendPtr(pTree->hdpaWatch, pwi) != -1;
}

//
//  TV_EndWatch - Remove the item from the watch list.
//
BOOL TV_EndWatch(PTREE pTree, PTVWATCHEDITEM pwi)
{
    int i = DPA_GetPtrCount(pTree->hdpaWatch);
    while (--i >= 0)
    {
        PTVWATCHEDITEM pwiT = DPA_FastGetPtr(pTree->hdpaWatch, i);
        ASSERT(pwiT);
        if (pwi == pwiT)
        {
            DPA_DeletePtr(pTree->hdpaWatch, i);
            return TRUE;
        }
    }
    ASSERT(!"TV_EndWatch: Item not in list");
    return FALSE;
}

//  End of treeview item watching implementation

void NEAR TV_SetItemRecurse(PTREE pTree, TREEITEM FAR *hItem, LPTVITEMEX ptvi)
{
    // Note:  This code assumes nobody will try to delete an item
    //        during a SetItem notification.
    while (hItem) {
        ptvi->hItem = hItem;
        TV_SetItem(pTree, ptvi);
        if (hItem->hKids) {
            TV_SetItemRecurse(pTree, hItem->hKids, ptvi);
        }

        hItem = hItem->hNext;
    }
}

BOOL NEAR TV_DoExpandRecurse(PTREE pTree, TREEITEM FAR *hItem, BOOL fNotify)
{
    TVWATCHEDITEM wi;
    BOOL fRc = FALSE;

    if (TV_StartWatch(pTree, &wi, hItem))
    {
        while ((hItem = TV_GetWatchItem(pTree, &wi))) {

            // was the escape key pressed at any point since the last check?
            if (GetAsyncKeyState(VK_ESCAPE) & 0x1)
                goto failed;

            TV_Expand(pTree, TVE_EXPAND, hItem, fNotify); // yields
            if (TV_IsWatchValid(pTree, &wi)) {
                if (hItem->hKids) {
                    if (!TV_DoExpandRecurse(pTree, hItem->hKids, fNotify))
                        goto failed;
                }
            }
            TV_NextWatchItem(pTree, &wi);
        }
        fRc = TRUE;
    failed:
        TV_EndWatch(pTree, &wi);
    }
    return fRc;
}


void NEAR TV_ExpandRecurse(PTREE pTree, TREEITEM FAR *hItem, BOOL fNotify)
{
    BOOL fRedraw = pTree->fRedraw;

    TV_OnSetRedraw(pTree, FALSE);
    
    // we're going to check this after each expand so clear it first
    GetAsyncKeyState(VK_ESCAPE);
    
    TV_Expand(pTree, TVE_EXPAND, hItem, fNotify);
    // BUGBUG hItem may have gone bad during that TV_Expand
    TV_DoExpandRecurse(pTree, hItem->hKids, fNotify);
    TV_OnSetRedraw(pTree, fRedraw);
}

void NEAR TV_ExpandParents(PTREE pTree, TREEITEM FAR *hItem, BOOL fNotify)
{
    hItem = hItem->hParent;
    if (hItem) {
        TVWATCHEDITEM wi;
        if (TV_StartWatch(pTree, &wi, hItem)) {
            TV_ExpandParents(pTree, hItem, fNotify);

            // Item may have gone invalid during expansion
            if (TV_IsWatchValid(pTree, &wi) &&

                // make sure this item is not in a collapsed branch
                !(hItem->state & TVIS_EXPANDED)) {

                TV_Expand(pTree, TVE_EXPAND, hItem, fNotify);
            }
            TV_EndWatch(pTree, &wi);
        }
    }
}

// makes sure an item is expanded and scrolled into view

BOOL NEAR TV_EnsureVisible(PTREE pTree, TREEITEM FAR * hItem)
{
    TV_ExpandParents(pTree, hItem, TRUE);
    return TV_ScrollIntoView(pTree, hItem);
}

//
//  Walk up the tree towards the root until we find the item at level iLevel.
//  Note the cast to (char) because iLevel is a BYTE, so the root's level is
//  0xFF.  Casting to (char) turns 0xFF it into -1.
//
HTREEITEM TV_WalkToLevel(HTREEITEM hWalk, int iLevel)
{
    int i;
    for (i = (char)hWalk->iLevel - iLevel; i > 0; i--)
        hWalk = hWalk->hParent;
    return hWalk;
}

// this is to handle single expand mode.
// The new selection is toggled, and the old selection is collapsed

// assume that parents of hNewSel are already fully expanded
// to do this, we build a parent dpa for the old and new
// then go through find the first parent node of the old selection that's not in
// the new sel tree.  and expand that.
void TV_ExpandOnSelChange(PTREE pTree, TREEITEM *hNewSel, TREEITEM *hOldSel)
{
    LRESULT dwAbort;
    NM_TREEVIEW nm;
    BOOL fCollapsing;
    TVWATCHEDITEM wiOld, wiNew;

    // Revalidate hNewSel and hOldSel since they may have been deleted
    // during all the notifications that occurred in the meantime.
    if (!ValidateTreeItem(hOldSel, VTI_NULLOK) ||
        !ValidateTreeItem(hNewSel, VTI_NULLOK))
        return;

    if (TV_StartWatch(pTree, &wiOld, hOldSel))
    {
        if (TV_StartWatch(pTree, &wiNew, hNewSel))
        {
            // Let the app clean up after itself
            nm.itemOld.hItem = hOldSel;
            if (hOldSel)
                nm.itemOld.lParam = hOldSel->lParam;
            nm.itemOld.mask = (TVIF_HANDLE | TVIF_PARAM);

            nm.itemNew.hItem = hNewSel;
            if (hNewSel)
                nm.itemNew.lParam = hNewSel->lParam;
            nm.itemNew.mask = (TVIF_HANDLE | TVIF_PARAM);

            dwAbort = CCSendNotify(&pTree->ci, TVN_SINGLEEXPAND, &nm.hdr);

            UpdateWindow(pTree->ci.hwnd);

            // Revalidate hNewSel and hOldSel since they may have been deleted
            // by that notification.
            if (!TV_IsWatchValid(pTree, &wiOld) ||
                !TV_IsWatchValid(pTree, &wiNew))
                goto cleanup;

            // Collapse if the NewSel currently expanded.
            fCollapsing = hNewSel && (hNewSel->state & TVIS_EXPANDED);

            // Note that Ctrl+select allows the user to suppress the collapse
            // of the old selection.
            if ((!(dwAbort & TVNRET_SKIPOLD)) && hOldSel  && (GetKeyState(VK_CONTROL) >= 0)) {

                //
                //  Collapse parents until we reach the common ancestor between
                //  hOldSel and hNewSel.  Note carefully that we don't cache
                //  any HTREEITEMs to avoid revalidation problems.
                //

                //
                //  Find the common ancestor, which might be the tree root.
                //
                int iLevelCommon;

                if (!hNewSel)
                    iLevelCommon = -1;          // common ancestor is root
                else
                {
                    HTREEITEM hItemO, hItemN;
                    iLevelCommon = min((char)hOldSel->iLevel, (char)hNewSel->iLevel);
                    hItemO = TV_WalkToLevel(hOldSel, iLevelCommon);
                    hItemN = TV_WalkToLevel(hNewSel, iLevelCommon);
                    while (iLevelCommon >= 0 && hItemO != hItemN) {
                        iLevelCommon--;
                        hItemO = hItemO->hParent;
                        hItemN = hItemN->hParent;
                    }
                }

                //
                //  Now walk up the tree from hOldSel, collapsing everything
                //  until we reach the common ancestor.  Do not collapse the
                //  common ancestor.
                //

                while ((char)hOldSel->iLevel > iLevelCommon)
                {
                    TV_Expand(pTree, TVE_COLLAPSE, hOldSel, TRUE);
                    if (!TV_IsWatchValid(pTree, &wiOld))
                        break;
                    hOldSel = hOldSel->hParent;
                    TV_RestartWatch(pTree, &wiOld, hOldSel);
                }

            }

            if ((!(dwAbort & TVNRET_SKIPNEW)) && hNewSel && TV_IsWatchValid(pTree, &wiNew)) {
                TV_Expand(pTree, TVE_TOGGLE, hNewSel, TRUE);
                UpdateWindow(pTree->ci.hwnd);

            }

cleanup:
            TV_EndWatch(pTree, &wiNew);
        }
        TV_EndWatch(pTree, &wiOld);
    }
}

// ----------------------------------------------------------------------------
//
//  Notify the parent that the selection is about to change.  If the change is
//  accepted, de-select the current selected item and select the given item
//
//  sets hCaret
//
// in:
//      hItem   item to become selected
//      wType   TVGN_ values (TVGN_CARET, TVGN_DROPHILIGHT are only valid values)
//      flags   combination of flags
//          TVSIF_NOTIFY        - send notify to parent window
//          TVSIF_UPDATENOW     - do UpdateWindow() to force sync painting
//          TVSIF_NOSINGLEEXPAND- don't do single-expand stuff
//      action  action code to send identifying how selection is being made
//
//  NOTE: Multiple Selection still needs to be added -- this multiplesel code
//        is garbage
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_SelectItem(PTREE pTree, WPARAM wType, TREEITEM FAR * hItem, UINT flags, UINT action)
{
    UINT uRDWFlags = RDW_INVALIDATE;

    if (pTree->hImageList && (ImageList_GetBkColor(pTree->hImageList) == (COLORREF)-1))
        uRDWFlags |= RDW_ERASE;

    if (!ValidateTreeItem(hItem, VTI_NULLOK))
        return FALSE;                   // Invalid parameter

    switch (wType) {

    case TVGN_FIRSTVISIBLE:
        if (!hItem)
            return FALSE;

        TV_EnsureVisible(pTree, hItem);
        if (pTree->fVert) TV_SetTopItem(pTree, hItem->iShownIndex);
        break;

    case TVGN_DROPHILITE:

        ASSERT(hItem == NULL || ITEM_VISIBLE(hItem));

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


            if (flags & TVSIF_UPDATENOW)
                UpdateWindow(pTree->ci.hwnd);
        }
        break;

    case TVGN_CARET:

        // REVIEW: we may want to scroll into view in this case
        // it's already the selected item, just return
        if (pTree->hCaret != hItem) {

            TREEITEM FAR * hOldSel;

            if ((flags & TVSIF_NOTIFY) && TV_SendSelChange(pTree, TVN_SELCHANGING, pTree->hCaret, hItem, action))
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
                TV_ExpandParents(pTree, hItem, (flags & TVSIF_NOTIFY));

                TV_InvalidateItem(pTree, hItem, uRDWFlags );

                if (action == TVC_BYMOUSE) {
                    // if selected by mouse, let's wait a doubleclick sec before scrolling
                    SetTimer(pTree->ci.hwnd, IDT_SCROLLWAIT, GetDoubleClickTime(), NULL);
                    pTree->fScrollWait = TRUE;
                } else if (pTree->fRedraw)
                    TV_ScrollVertIntoView(pTree, hItem);
            }
            if (pTree->hwndToolTips)
                TV_Timer(pTree, IDT_TOOLTIPWAIT);

            if (flags & TVSIF_NOTIFY)
                TV_SendSelChange(pTree, TVN_SELCHANGED, hOldSel, hItem, action);

            if ((pTree->ci.style & TVS_SINGLEEXPAND) &&
                !(flags & TVSIF_NOSINGLEEXPAND) &&
                action != TVC_BYKEYBOARD)
            {
                    TV_ExpandOnSelChange(pTree, pTree->hCaret, hOldSel);
            }

            if (flags & TVSIF_UPDATENOW)
                UpdateWindow(pTree->ci.hwnd);

            MyNotifyWinEvent(EVENT_OBJECT_FOCUS, pTree->ci.hwnd, OBJID_CLIENT,
                (LONG_PTR)hItem);
            MyNotifyWinEvent(EVENT_OBJECT_SELECTION, pTree->ci.hwnd, OBJID_CLIENT,
                (LONG_PTR)hItem);
        }
        break;

    default:
        DebugMsg(DM_TRACE, TEXT("Invalid type passed to TV_SelectItem"));
        return FALSE;
    }

    return TRUE;        // success
}

// remove all the children, but pretend they are still there

BOOL NEAR TV_ResetItem(PTREE pTree, HTREEITEM hItem)
{
    TV_DeleteItem(pTree, hItem, TVDI_CHILDRENONLY);

    hItem->state &= ~TVIS_EXPANDEDONCE;
    hItem->fKids = KIDS_FORCE_YES;      // force children

    return TRUE;
}


// ----------------------------------------------------------------------------
//
//  Expand or collapse an item's children
//  Returns TRUE if any change took place and FALSE if unchanged
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_Expand(PTREE pTree, WPARAM wCode, TREEITEM FAR * hItem, BOOL fNotify)
{
    WORD fOldState;
    UINT cntVisDescendants;
    TVITEMEX sItem;
    TREEITEM FAR * hItemExpanding;

// deal with the evil invisible root for multiple root trees.
    hItemExpanding = hItem;
    if ((hItem == NULL) || (hItem == TVI_ROOT))
        hItem = pTree->hRoot;

    DBG_ValidateTreeItem(hItem, 0);

    TV_GetItem(pTree, hItem, TVIF_CHILDREN, &sItem);

    if (!(wCode & TVE_ACTIONMASK) || sItem.cChildren == 0)
        return FALSE;           // no children to expand or collapse

    if ((wCode & TVE_ACTIONMASK) == TVE_TOGGLE) {
        wCode = (wCode & ~TVE_ACTIONMASK);

        // if it's not expaned, or not fully expanded, expand now
        wCode |=
            (((!(hItem->state & TVIS_EXPANDED)) ||
              hItem->state & TVIS_EXPANDPARTIAL) ?
             TVE_EXPAND : TVE_COLLAPSE);
    }

    if (((wCode & TVE_ACTIONMASK) == TVE_EXPAND) && !(hItem->state & TVIS_EXPANDEDONCE))
    {
        // if its the first expand, ALWAYS notify the parent
        fNotify = TRUE;
    }

    // at this point the children may be added if they aren't already there (callback)

    if (fNotify && TV_SendItemExpand(pTree, TVN_ITEMEXPANDING, hItemExpanding, wCode))
        return FALSE;

    // if (!hItem->hKids && (hItem->fKids == KIDS_FORCE_NO))    // this may be right, but I don't
                                                                // have proof now.
    if (!hItem->hKids)
    {
        // kids we removed, or never there
        TV_InvalidateItem(pTree, hItem, RDW_INVALIDATE);
        return FALSE;
    }

    fOldState = hItem->state;

    if (hItem->hParent) // never turn off TVIS_EXPANED for the invisible root
    {
        if ((wCode & TVE_ACTIONMASK) == TVE_EXPAND)
           hItem->state |= TVIS_EXPANDED;
        else
           hItem->state &= ~(TVIS_EXPANDED | TVIS_EXPANDPARTIAL);

        if (wCode & TVE_EXPANDPARTIAL) {
            hItem->state |= TVIS_EXPANDPARTIAL;
        } else {
            hItem->state &= ~(TVIS_EXPANDPARTIAL);
        }
    }

    // if we're not changing the expanded state
    // check to see if we're supposed to collapse reset
    if (!(fOldState & TVIS_EXPANDED) &&
        !(hItem->state & TVIS_EXPANDED))
    {
        if ((wCode & (TVE_ACTIONMASK | TVE_COLLAPSERESET)) == (TVE_COLLAPSE | TVE_COLLAPSERESET))
        {
            TV_ResetItem(pTree, hItem);
        }

        return FALSE;
    }

    // if we changed expaneded states, recalc the scrolling
    if ((fOldState ^ hItem->state) & TVIS_EXPANDED) {

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

            // If we collapsed the subtree that contains the caret, then
            // pop the caret back to the last visible ancestor
            // Pass TVIS_NOSINGLEEXPAND so we won't expand an item right
            // after we collapsed it (d'oh!)
            if (pTree->hCaret)
            {
                TREEITEM FAR * hWalk = TV_WalkToLevel(pTree->hCaret, hItem->iLevel);

                if (hWalk == hItem)
                    TV_SelectItem(pTree, TVGN_CARET, hItem, (fNotify ? TVSIF_NOTIFY : 0) | TVSIF_UPDATENOW | TVSIF_NOSINGLEEXPAND, TVC_UNKNOWN);
            }

        }
    } else if ((fOldState ^ hItem->state) & TVIS_EXPANDPARTIAL) {
        // we didn't change the expanded state, only the expand partial
        TV_InvalidateItem(pTree, hItem, RDW_INVALIDATE);
    }

    if (fNotify && TV_SendItemExpand(pTree, TVN_ITEMEXPANDED, hItem, wCode))
        return FALSE;

    hItem->state |= TVIS_EXPANDEDONCE;

    if ((wCode & (TVE_ACTIONMASK | TVE_COLLAPSERESET)) == (TVE_COLLAPSE | TVE_COLLAPSERESET))
    {
        TV_ResetItem(pTree, hItem);
    }

    // BUGBUG raymondc v6 we generate a notification even if nothing happened,
    // which freaks out accessibility.  E.g., app tried to expand something
    // that was already expanded.  Explorer Band does this when you navigate.
    MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, pTree->ci.hwnd, OBJID_CLIENT,
        (LONG_PTR)hItem);

    return TRUE;
}

BOOL PASCAL BetweenItems(PTREE pTree, HTREEITEM hItem, HTREEITEM hItemStart, HTREEITEM hItemEnd)
{
    if (hItemStart) {
        while ((hItemStart = TV_GetNextVisItem(hItemStart)) && (hItemEnd != hItemStart))
        {
            if (hItem == hItemStart)
                return TRUE;
        }
    }
    return FALSE;
}

#ifdef  FE_IME
// Now only Korean version is interested in incremental search with composition string.

#define FREE_COMP_STRING(pszCompStr)    LocalFree((HLOCAL)(pszCompStr))

BOOL NEAR TV_OnImeComposition(PTREE pTree, WPARAM wParam, LPARAM lParam)
{
    LPTSTR lpsz;
    int iCycle = 0;
    HTREEITEM hItem;
    TCHAR szTemp[MAXLABELTEXT];
    TVITEMEX ti;
    LPTSTR lpszAlt = NULL; // use only if SameChar
    int iLen;
    HIMC hImc;
    TCHAR *pszCompStr;
    BOOL fRet = TRUE;

    if (hImc = ImmGetContext(pTree->ci.hwnd))
    {
        if (lParam & GCS_RESULTSTR)
        {
            fRet = FALSE;
            pszCompStr = GET_COMP_STRING(hImc, GCS_RESULTSTR);
            if (pszCompStr)
            {
                IncrementSearchImeCompStr(&pTree->is, FALSE, pszCompStr, &lpsz);
                FREE_COMP_STRING(pszCompStr);
            }
        }
        if (lParam & GCS_COMPSTR)
        {
            fRet = TRUE;
            pszCompStr = GET_COMP_STRING(hImc, GCS_COMPSTR);
            if (pszCompStr)
            {
                if (IncrementSearchImeCompStr(&pTree->is, TRUE, pszCompStr, &lpsz)) {
                    if (pTree->hCaret) {
                        pTree->htiSearch = pTree->hCaret;
                    } else if (pTree->hRoot && pTree->hRoot->hKids) {
                        pTree->htiSearch = pTree->hRoot->hKids;
                    } else
                        return fRet;
                }

                if (!lpsz || !*lpsz || !pTree->hRoot || !pTree->hRoot->hKids)
                    return fRet;

                hItem = pTree->htiSearch;
                ti.cchTextMax  = sizeof(szTemp);
                iLen = lstrlen(lpsz);
#ifdef UNICODE
                if (iLen > 1 && SameChars(lpsz, lpsz[0]))
                    lpszAlt = lpsz + iLen - 1;
#else
                if (iLen > 2 && SameDBCSChars(lpsz, (WORD)((BYTE)lpsz[0] << 8 | (BYTE)lpsz[1])))
                    lpszAlt = lpsz + iLen - 2;
#endif

                do {
                    ti.pszText = szTemp;
                    hItem = TV_GetNextVisItem(hItem);
                    if (!hItem) {
                        iCycle++;
                        hItem = pTree->hRoot->hKids;
                    }

                    TV_GetItem(pTree, hItem, TVIF_TEXT, &ti);
                    if ((ti.pszText != LPSTR_TEXTCALLBACK) &&
                        HIWORD64(ti.pszText)) {
                        // DebugMsg(DM_TRACE, "treesearch %d %s %s", (LPSTR)lpsz, (LPSTR)lpsz, (LPSTR)ti.pszText);
                        if (IntlStrEqNI(lpsz, ti.pszText, iLen) ||
#ifdef UNICODE
                            (lpszAlt && IntlStrEqNI(lpszAlt, ti.pszText, 1) &&
#else
                            (lpszAlt && IntlStrEqNI(lpszAlt, ti.pszText, 2) &&
#endif
                             BetweenItems(pTree, hItem, pTree->hCaret, pTree->htiSearch)))
                        {
                            DebugMsg(DM_TRACE, TEXT("Selecting"));
                            TV_SelectItem(pTree, TVGN_CARET, hItem, TVSIF_NOTIFY | TVSIF_UPDATENOW, TVC_BYKEYBOARD);
#ifdef KEYBOARDCUES
                            //notify of navigation key usage
                            CCNotifyNavigationKeyUsage(&(pTree->ci), UISF_HIDEFOCUS);
#endif
                            return fRet;
                        }
                    }
                }  while(iCycle < 2);

                // if they hit the same key twice in a row at the beginning of
                // the search, and there was no item found, they likely meant to
                // retstart the search
                if (lpszAlt) {

                    // first clear out the string so that we won't recurse again
                    IncrementSearchString(&pTree->is, 0, NULL);
                    TV_OnImeComposition(pTree, wParam, lParam);
                } else {
                    IncrementSearchBeep(&pTree->is);
                }
#ifdef KEYBOARDCUES
                //notify of navigation key usage
                CCNotifyNavigationKeyUsage(&(pTree->ci), UISF_HIDEFOCUS);
#endif
                FREE_COMP_STRING(pszCompStr);
            }
        }
        ImmReleaseContext(pTree->ci.hwnd, hImc);
    }
    return fRet;
}
#endif


void NEAR TV_OnChar(PTREE pTree, UINT ch, int cRepeat)
{
    LPTSTR lpsz;
    int iCycle = 0;
    HTREEITEM hItem;
    TCHAR szTemp[MAXLABELTEXT];
    TVITEMEX ti;
    LPTSTR lpszAlt = NULL; // use only if SameChar
    int iLen;

#ifdef UNICODE_WIN9x
    if (g_fDBCSEnabled && (IsDBCSLeadByteEx(pTree->ci.uiCodePage, (BYTE)ch) || pTree->uDBCSChar))
    {
        WCHAR wch;

        if (!pTree->uDBCSChar)
        {
            // Save DBCS LeadByte character
            pTree->uDBCSChar = ch & 0x00ff;
            return;
        }
        else
        {
            // Combine DBCS characters
            pTree->uDBCSChar |=  ((ch & 0x00ff) << 8);

            // Convert to UNICODE
            if (MultiByteToWideChar(pTree->ci.uiCodePage, MB_ERR_INVALID_CHARS, (LPCSTR)&pTree->uDBCSChar, 2, &wch, 1))
            {
                ch = wch;
            }

            pTree->uDBCSChar = 0;
        }
    }
    else
    {
        if (ch >= 0x80)     // no need conversion for low ansi character
        {
            WCHAR wch;

            if (MultiByteToWideChar(pTree->ci.uiCodePage, 0, (LPCSTR)&ch, 1, &wch, 1))
                ch = wch;
        }
    }
#endif

    if (IncrementSearchString(&pTree->is, ch, &lpsz) || !pTree->htiSearch) {
        if (pTree->hCaret) {
            pTree->htiSearch = pTree->hCaret;
        } else if (pTree->hRoot && pTree->hRoot->hKids) {
            pTree->htiSearch = pTree->hRoot->hKids;
        } else
            return;
    }

    if (!lpsz || !*lpsz || !pTree->hRoot || !pTree->hRoot->hKids)
        return;

    hItem = pTree->htiSearch;
    ti.cchTextMax  = ARRAYSIZE(szTemp);
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
            HIWORD64(ti.pszText)) {
            // DebugMsg(DM_TRACE, TEXT("treesearch %d %s %s"), (LPTSTR)lpsz, (LPTSTR)lpsz, (LPTSTR)ti.pszText);
            if (IntlStrEqNI(lpsz, ti.pszText, iLen) ||
                (lpszAlt && IntlStrEqNI(lpszAlt, ti.pszText, 1) &&
                 BetweenItems(pTree, hItem, pTree->hCaret, pTree->htiSearch)))
            {
                DebugMsg(DM_TRACE, TEXT("Selecting"));
                TV_SelectItem(pTree, TVGN_CARET, hItem, TVSIF_NOTIFY | TVSIF_UPDATENOW, TVC_BYKEYBOARD);
#ifdef KEYBOARDCUES
                //notify of navigation key usage
                CCNotifyNavigationKeyUsage(&(pTree->ci), UISF_HIDEFOCUS);
#endif
                return;
            }
        }
    }  while(iCycle < 2);

    // if they hit the same key twice in a row at the beginning of
    // the search, and there was no item found, they likely meant to
    // retstart the search
    if (lpszAlt) {

        // first clear out the string so that we won't recurse again
        IncrementSearchString(&pTree->is, 0, NULL);
        TV_OnChar(pTree, ch, cRepeat);
    } else {
        IncrementSearchBeep(&pTree->is);
    }
#ifdef KEYBOARDCUES
    //notify of navigation key usage
    CCNotifyNavigationKeyUsage(&(pTree->ci), UISF_HIDEFOCUS);
#endif
}

// ----------------------------------------------------------------------------
//
//  Handle WM_KEYDOWN messages
//  If control key is down, treat keys as scroll codes; otherwise, treat keys
//  as caret position changes.
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_KeyDown(PTREE pTree, WPARAM wKey, LPARAM dwKeyData)
{
    TREEITEM FAR * hItem;
    UINT wShownIndex;
    TV_KEYDOWN nm;
    BOOL fPuntChar;
    BOOL ret = TRUE;

    // Notify
    nm.wVKey = (WORD)wKey;
    fPuntChar = (BOOL)CCSendNotify(&pTree->ci, TVN_KEYDOWN, &nm.hdr);

    wKey = RTLSwapLeftRightArrows(&pTree->ci, wKey);

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
            fPuntChar = (BOOL)CCSendNotify(&pTree->ci, NM_RETURN, NULL);
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
                TV_SelectItem(pTree, TVGN_CARET, hItem, TVSIF_NOTIFY | TVSIF_UPDATENOW, TVC_BYKEYBOARD);

            break;

        case VK_SPACE:
            if ((pTree->ci.style & TVS_CHECKBOXES) && pTree->hCaret)
            {
                TV_HandleStateIconClick(pTree, pTree->hCaret);
                fPuntChar = TRUE; // don't beep
            }
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
        while((pTree->iPuntChar > 0) && PeekMessage(&msg, pTree->ci.hwnd, WM_CHAR, WM_CHAR, PM_REMOVE)) {
            pTree->iPuntChar--;
        }
        ASSERT(!pTree->iPuntChar);
    }

#ifdef KEYBOARDCUES
    if(VK_MENU!=wKey)
    {//notify of navigation key usage
        CCNotifyNavigationKeyUsage(&(pTree->ci), UISF_HIDEFOCUS);
    }
#endif
    return ret;

}


// ----------------------------------------------------------------------------
//
//  Sets the tree's indent width per hierarchy level and recompute widths.
//
//  sets cxIndent
//
// ----------------------------------------------------------------------------

void NEAR TV_SetIndent(PTREE pTree, WPARAM cxIndent)
{
    if (pTree->hImageList) {
        if ((SHORT)cxIndent < pTree->cxImage)
            cxIndent = pTree->cxImage;
    }

    if ((SHORT)cxIndent < pTree->cyText)
        cxIndent = pTree->cyText;

    if (cxIndent < MAGIC_MININDENT)
        cxIndent = MAGIC_MININDENT;

    pTree->cxIndent = (SHORT)cxIndent;

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
    if (!pTree->fCyItemSet)
        pTree->cyItem = (max(pTree->cyImage, pTree->cyText) + 1);
    // height not always even not, only on haslines style.
    if (pTree->cyItem <= 1) {
        pTree->cyItem = 1;          // Don't let it go zero or negative!
    } else if (!(pTree->ci.style & TVS_NONEVENHEIGHT))
        pTree->cyItem &= ~1;

    pTree->cFullVisible = pTree->cyWnd / pTree->cyItem;

    TV_CreateIndentBmps(pTree);
    TV_CalcScrollBars(pTree);
}

// BUGBUG: does not deal with hfont == NULL

void NEAR TV_OnSetFont(PTREE pTree, HFONT hNewFont, BOOL fRedraw)
{
    HDC hdc;
    HFONT hfontSel;
    TCHAR c = TEXT('J');       // for bog
    SIZE size;

    if (pTree->fCreatedFont && pTree->hFont) {
        DeleteObject(pTree->hFont);
        pTree->fCreatedFont = FALSE;
    }

    if (hNewFont == NULL) {
        LOGFONT lf;
        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
#ifdef WINNT
        // B#210235 - because NT4 initializes icontitle logfont with Ansi charset
        // no matter what font is selected, following A/W conversion would fail
        // on non US environment if we use this logfont to get codepage. 
        // The ACP is guaranteed to work with any Ansi apps because these apps
        // assume ACP to be matching to their desired codepage.
        if (staticIsOS(OS_NT4) && !staticIsOS(OS_NT5))
        {
            CHARSETINFO csi;
            TranslateCharsetInfo((DWORD *)g_uiACP, &csi, TCI_SRCCODEPAGE);
            lf.lfCharSet = (BYTE)csi.ciCharset;
        }
#endif
        hNewFont = CreateFontIndirect(&lf);
        pTree->fCreatedFont = TRUE;         // make sure we delete it
    }

    hdc = GetDC(pTree->ci.hwnd);

    hfontSel = hNewFont ? SelectObject(hdc, hNewFont) : NULL;

    // Office9 Setup had a bug where they installed a bogus font,
    // which created okay but all APIs against it (e.g., GetTextExtentPoint)
    // failed!  Protect against failure by pre-setting the value to something
    // non-garbage.
    size.cy = 0;
    GetTextExtentPoint(hdc, &c, 1, &size);
    pTree->cyText = (SHORT)(size.cy + (g_cyBorder * 2));

    if (hfontSel)
        SelectObject(hdc, hfontSel);

    ReleaseDC(pTree->ci.hwnd, hdc);

    pTree->hFont = hNewFont;
    if (pTree->hFontBold) {
        TV_CreateBoldFont(pTree);
    }
    pTree->ci.uiCodePage = GetCodePageForFont(hNewFont);

    TV_DeleteHotFonts(pTree);

    if (pTree->cxIndent == 0)   // first time init?
    {
        if (!pTree->cyItem) pTree->cyItem = pTree->cyText;
        TV_SetIndent(pTree, 16 /*g_cxSmIcon*/ + MAGIC_INDENT);
    }

    TV_ScrollBarsAfterSetWidth(pTree, NULL);
    TV_SetItemHeight(pTree);

    if (pTree->hwndToolTips)
        SendMessage(pTree->hwndToolTips, WM_SETFONT, (WPARAM)pTree->hFont, (LPARAM)TRUE);

    // REVIEW: does this happen as a result of the above?
    // if (fRedraw)
    //    RedrawWindow(pTree->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}

VOID NEAR PASCAL TV_CreateBoldFont(PTREE pTree)
{
    LOGFONT lf;

    if (pTree->hFontBold)
        DeleteObject (pTree->hFontBold);

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
                pTree->cyImage = (SHORT)cy;
                if (pTree->cxIndent < pTree->cxImage)
                    TV_SetIndent(pTree, pTree->cxImage);
                pTree->hImageList = hImage;

                if (!hImageOld && pTree->ci.style & TVS_CHECKBOXES) {
                    TV_InitCheckBoxes(pTree);
                }
            }
            else
            {
                pTree->cxImage = pTree->cyImage = 0;
                pTree->hImageList = NULL;
            }
            break;

        default:
            DebugMsg(DM_TRACE, TEXT("sh TR - TVM_SETIMAGELIST: unrecognized iImageList"));
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

TREEITEM FAR * NEAR TV_GetNextItem(PTREE pTree, TREEITEM FAR * hItem, WPARAM wGetCode)
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

    case TVGN_LASTVISIBLE:
        return TV_GetShownIndexItem(pTree->hRoot->hKids, pTree->cShowing-1);

    case TVGN_CHILD:
        if (!hItem || (hItem == TVI_ROOT))
            return pTree->hRoot->hKids;
        break;
    }

    // all of these require a valid hItem
    if (!ValidateTreeItem(hItem, 0))
        return NULL;

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


void TV_InvalidateInsertMarkRect(PTREE pTree, BOOL fErase)
{
    RECT rc;
    if (TV_GetInsertMarkRect(pTree, &rc))
        InvalidateRect(pTree->ci.hwnd, &rc, fErase);
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
    RECT rc;
    UINT cxOld = pTree->cxWnd;
    if (!cxWnd || !cyWnd)
    {
        GetClientRect(pTree->ci.hwnd, &rc);
        cxWnd = rc.right;
        cyWnd = rc.bottom;
    }
    pTree->cxWnd = (SHORT)cxWnd;
    pTree->cyWnd = (SHORT)cyWnd;
    pTree->cFullVisible = cyWnd / pTree->cyItem;
    
    if (pTree->ci.style & TVS_NOSCROLL)
        pTree->cxMax = (WORD) cxWnd;
    
    TV_CalcScrollBars(pTree);
    if (pTree->cxBorder)
    {
        rc.top = 0;
        rc.bottom = cyWnd;
        rc.right = cxOld;
        rc.left = cxOld - pTree->cxBorder;
        if (rc.left < (int)cxWnd) {
            // invalidate so clipping happens on right on size.
            InvalidateRect(pTree->ci.hwnd, &rc, TRUE);  
        }
    }

    TV_InvalidateInsertMarkRect(pTree, TRUE);

    return TRUE;
}


void TV_HandleStateIconClick(PTREE pTree, HTREEITEM hItem)
{
    TVITEMEX tvi;
    int iState;

    tvi.stateMask = TVIS_STATEIMAGEMASK;
    TV_GetItem(pTree, hItem, TVIF_STATE, &tvi);

    iState = STATEIMAGEMASKTOINDEX(tvi.state & tvi.stateMask);
    iState %= (ImageList_GetImageCount(pTree->himlState) - 1);
    iState++;

    tvi.mask = TVIF_STATE;
    tvi.state = INDEXTOSTATEIMAGEMASK(iState);
    tvi.hItem = hItem;
    TV_SetItem(pTree, &tvi);

}


//
//  Eudora is a pile of crap.
//
//  When they get a NM_DBLCLK notification from a treeview, they say,
//  "Oh, I know that treeview allocates its NMHDR from the stack, and
//  there's this local variable on Treeview's stack I'm really interested
//  in, so I'm going to hard-code an offset from the pnmhdr and read the
//  DWORD at that location so I can get at the local variable.  I will then
//  stop working if this value is zero."
//
//  The conversion to UNICODE changed our stack layout enough that they
//  end up always getting zero -- it's the NULL parameter which is the
//  final argument to CCSendNotify.  Since all this stack layout stuff is
//  sensitive to how the compiler's optimizer feels today, we create a
//  special notify structure Just For Eudora which mimics the stack layout
//  they expected to see in Win95.
//
typedef struct NMEUDORA {
    NMHDR   nmhdr;
    BYTE    Padding[48];
    DWORD   MustBeNonzero;      // Eudora fails to install if this is zero
} NMEUDORA;

// ----------------------------------------------------------------------------
//
//  WM_LBUTTONDBLCLK message -- toggle expand/collapse state of item's children
//  WM_LBUTTONDOWN message -- on item's button, do same as WM_LBUTTONDBLCLK,
//  otherwise select item and ensure that item is fully visible
//
// ----------------------------------------------------------------------------

void NEAR TV_ButtonDown(PTREE pTree, UINT wMsg, UINT wFlags, int x, int y, UINT TVBD_flags)
{
    UINT wHitCode;
    TREEITEM FAR * hItem;
    HWND hwndTree;
    LRESULT lResult;
#ifdef _X86_
    NMEUDORA nmeu;
    nmeu.MustBeNonzero = 1;
    COMPILETIME_ASSERT(FIELD_OFFSET(NMEUDORA, MustBeNonzero) == 0x3C);
#endif

    GetMessagePosClient(pTree->ci.hwnd, &pTree->ptCapture);

    if (!TV_DismissEdit(pTree, FALSE))   // end any previous editing (accept it)
        return;     // Something happened such that we should not process button down


    hItem = TV_CheckHit(pTree, x, y, &wHitCode);

    // Excel likes to destroy the entire tree when it gets a double-click
    // so we need to watch the item in case it vanishes behind our back.
    hwndTree = pTree->ci.hwnd;

    if (wMsg == WM_LBUTTONDBLCLK)
    {
        //
        // Cancel any name editing that might happen.
        //

        TV_CancelEditTimer(pTree);

        if (wHitCode & (TVHT_ONITEM | TVHT_ONITEMBUTTON)) {
            goto ExpandItem;
        }

        //
        // Collapses node above the line double clicked on
        //
        else if ((pTree->ci.style & TVS_HASLINES) && (wHitCode & TVHT_ONITEMINDENT) &&
            (abs(x % pTree->cxIndent - pTree->cxIndent/2) <= g_cxDoubleClk)) {

            int i;

            for (i = hItem->iLevel - x/pTree->cxIndent + ((pTree->ci.style & TVS_LINESATROOT)?1:0); i > 1; i--)
                hItem = hItem->hParent;

ExpandItem:
#ifdef _X86_
            lResult = CCSendNotify(&pTree->ci, wFlags & MK_RBUTTON ? NM_RDBLCLK : NM_DBLCLK, &nmeu.nmhdr);
#else
            lResult = CCSendNotify(&pTree->ci, wFlags & MK_RBUTTON ? NM_RDBLCLK : NM_DBLCLK, NULL);
#endif
            if (!IsWindow(hwndTree))
                goto bail;
            if (!lResult) {
                // don't auto expand this if we're in single expand mode because the first click did it already
                if (!(pTree->ci.style & TVS_SINGLEEXPAND))
                    TV_Expand(pTree, TVE_TOGGLE, hItem, TRUE);
            }

        }

        pTree->fScrollWait = FALSE;

    } else {    // WM_LBUTTONDOWN

        if (wHitCode == TVHT_ONITEMBUTTON)
        {
            if (!CCSendNotify(&pTree->ci, NM_CLICK, NULL)) {
                if (TVBD_flags & TVBD_FROMWHEEL)
                    TV_Expand(pTree, (TVBD_flags & TVBD_WHEELFORWARD) ? TVE_EXPAND : TVE_COLLAPSE, hItem, TRUE);
                else
                    TV_Expand(pTree, TVE_TOGGLE, hItem, TRUE);
            }
        }
        else if (wHitCode & TVHT_ONITEM ||
                ((pTree->ci.style & TVS_FULLROWSELECT) && (wHitCode & (TVHT_ONITEMRIGHT | TVHT_ONITEMINDENT))))
        {
            BOOL fSameItem, bDragging;

            ASSERT(hItem);

            fSameItem = (hItem == pTree->hCaret);

            if (TVBD_flags & TVBD_FROMWHEEL)
                bDragging = FALSE;
            else if (pTree->ci.style & TVS_DISABLEDRAGDROP)
                bDragging = FALSE;
            else {
                bDragging = TV_CheckForDragBegin(pTree, hItem, x, y);
                TV_FinishCheckDrag(pTree);
            }

            if (bDragging)
            {
                pTree->htiDrag = hItem;
                TV_SendBeginDrag(pTree, TVN_BEGINDRAG, hItem, x, y);
                return;
            }

            if (!CCSendNotify(&pTree->ci, NM_CLICK, NULL)) {

                if (wHitCode == TVHT_ONITEMSTATEICON &&
                    (pTree->ci.style & TVS_CHECKBOXES)) {
                    TV_HandleStateIconClick(pTree, hItem);
                } else {

                    // Only set the caret (selection) if not dragging
                    TV_SelectItem(pTree, TVGN_CARET, hItem, TVSIF_NOTIFY | TVSIF_UPDATENOW, TVC_BYMOUSE);

                    if (fSameItem && (wHitCode & TVHT_ONITEMLABEL) && pTree->fFocus)
                    {
                        //
                        // The item and window are currently selected and user clicked
                        // on label.  Try to enter into name editing mode.
                        //
                        SetTimer(pTree->ci.hwnd, IDT_NAMEEDIT, GetDoubleClickTime(), NULL);
                        pTree->fNameEditPending = TRUE;
                    }

                    if (fSameItem && pTree->ci.style & TVS_SINGLEEXPAND) {
                        // single click on the focus item toggles expand state
                        TV_Expand(pTree, TVE_TOGGLE, pTree->hCaret, TRUE);
                    }
                }
            }
        } else {
            CCSendNotify(&pTree->ci, NM_CLICK, NULL);
        }
    }

    if (!pTree->fFocus)
        SetFocus(pTree->ci.hwnd);

bail:;
}


// ----------------------------------------------------------------------------
//
//  Gets the item's text, data, and/or image.
//
// ----------------------------------------------------------------------------
BOOL NEAR TV_OnGetItem(PTREE pTree, LPTVITEMEX ptvi)
{
    if (!ptvi)
        return FALSE;

    if (!ValidateTreeItem(ptvi->hItem, 0))
        return FALSE;           // Invalid parameter

    TV_GetItem(pTree, ptvi->hItem, ptvi->mask, ptvi);

    return TRUE;        // success
}

#ifdef UNICODE
BOOL NEAR TV_OnGetItemA(PTREE pTree, LPTVITEMEXA ptvi)
{
    BOOL bRet;
    LPSTR pszA = NULL;
    LPWSTR pszW = NULL;

    //HACK Alert!  This code assumes that TVITEMA is exactly the same
    // as TVITEMW except for the text pointer in the TVITEM
    ASSERT(sizeof(TVITEMA) == sizeof(TVITEMW));

    if (!IsFlagPtr(ptvi) && (ptvi->mask & TVIF_TEXT) && !IsFlagPtr(ptvi->pszText)) {
        pszA = ptvi->pszText;
        pszW = LocalAlloc(LMEM_FIXED, ptvi->cchTextMax * sizeof(WCHAR));
        if (pszW == NULL) {
            return FALSE;
        }
        ptvi->pszText = (LPSTR)pszW;
    }
    bRet = TV_OnGetItem(pTree, (LPTVITEMEXW)ptvi);
    if (pszA) {
        if (bRet && ptvi->cchTextMax)
            ConvertWToAN(pTree->ci.uiCodePage, pszA, ptvi->cchTextMax, (LPWSTR)(ptvi->pszText), -1);
        LocalFree(pszW);
        ptvi->pszText = pszA;
    }
    return bRet;
}
#endif

// ----------------------------------------------------------------------------
//
//  Sets the item's text, data, and/or image.
//
// ----------------------------------------------------------------------------

#ifdef UNICODE
BOOL NEAR TV_SetItemA(PTREE pTree, LPTVITEMEXA ptvi)
{
    LPSTR pszA = NULL;
    BOOL lRet;

    //HACK Alert!  This code assumes that TVITEMA is exactly the same
    // as TVITEMW except for the text pointer in the TVITEM
    ASSERT(sizeof(TVITEMA) == sizeof(TVITEMW));

    if (!IsFlagPtr(ptvi) && (ptvi->mask & TVIF_TEXT) && !IsFlagPtr(ptvi->pszText)) {
        pszA = ptvi->pszText;
        ptvi->pszText = (LPSTR)ProduceWFromA(pTree->ci.uiCodePage, pszA);

        if (ptvi->pszText == NULL) {
            ptvi->pszText = pszA;
            return -1;
        }
    }

    lRet = TV_SetItem(pTree, (LPCTVITEMEX)ptvi);

    if (pszA) {
        FreeProducedString(ptvi->pszText);
        ptvi->pszText = pszA;
    }

    return lRet;
}
#endif

BOOL NEAR TV_SetItem(PTREE pTree, LPCTVITEMEX ptvi)
{
    UINT uRDWFlags = RDW_INVALIDATE;
    BOOL fEraseIfTransparent = FALSE;
    HTREEITEM hItem;
    BOOL bActualChange = FALSE; // HACK: We want to keep track of which
                                // attributes were changed from CALLBACK to
                                // "real", and don't invalidate if those were
                                // the only changes
    int iIntegralPrev;
    BOOL fName = FALSE;
    BOOL fFocusSel = FALSE;
    BOOL fRecalcWidth = FALSE;
    BOOL fStateImageChange = FALSE;

    if (!ptvi)
        return FALSE;

    hItem = ptvi->hItem;

    // deal with the evil invisible root for multiple root trees.
    if (hItem == TVI_ROOT)
    {
        hItem = pTree->hRoot;
    }

    if (!ValidateTreeItem(hItem, 0))
        return FALSE;

    iIntegralPrev = hItem->iIntegral;

    // BUGBUG: send ITEMCHANING and ITEMCHANGED msgs

    if (ptvi->mask & TVIF_TEXT)
    {
        uRDWFlags = RDW_INVALIDATE |RDW_ERASE;
        bActualChange = TRUE;

        if (!ptvi->pszText)
        {
            Str_Set(&hItem->lpstr, LPSTR_TEXTCALLBACK);
        }
        else
        {
            if (!Str_Set(&hItem->lpstr, ptvi->pszText))
            {
                //
                // Memory allocation failed -  The best we can do now
                // is to set the item back to callback, and hope that
                // the top level program can handle it.
                //
                DebugMsg(DM_ERROR, TEXT("TreeView: Out of memory"));
                hItem->lpstr = LPSTR_TEXTCALLBACK;
            }
        }

        fRecalcWidth = TRUE;
        fName = TRUE;
    }

    if (ptvi->mask & TVIF_PARAM)
    {
        bActualChange = TRUE;
        hItem->lParam = ptvi->lParam;
    }

    if (ptvi->mask & TVIF_IMAGE)
    {
        if (hItem->iImage != (WORD)I_IMAGECALLBACK) {
            bActualChange = TRUE;
            fEraseIfTransparent = TRUE;
            if (pTree->hImageList && (ImageList_GetBkColor(pTree->hImageList) == (COLORREF)-1))
                uRDWFlags |= RDW_ERASE;

        }
        hItem->iImage = (SHORT)ptvi->iImage;
    }

    if (ptvi->mask & TVIF_SELECTEDIMAGE)
    {
        if (hItem->iSelectedImage != (WORD)I_IMAGECALLBACK)
            bActualChange = TRUE;
        hItem->iSelectedImage = (SHORT)ptvi->iSelectedImage;
    }

    if (ptvi->mask & TVIF_CHILDREN)
    {
        if (hItem->fKids != KIDS_CALLBACK)
            bActualChange = TRUE;

        if (ptvi->cChildren == I_CHILDRENCALLBACK) {
            hItem->fKids = KIDS_CALLBACK;
        } else {
            if (ptvi->cChildren)
                hItem->fKids = KIDS_FORCE_YES;
            else
                hItem->fKids = KIDS_FORCE_NO;
        }

        //
        // If this item currently has no kid, reset the item.
        //
        if ((ptvi->cChildren == I_CHILDRENCALLBACK) && (hItem->hKids == NULL))
        {
            hItem->state &= ~TVIS_EXPANDEDONCE;
            if (hItem->hParent)
                hItem->state &= ~TVIS_EXPANDED;
        }
    }

    if (ptvi->mask & TVIF_INTEGRAL)
    {
        if (LOWORD(ptvi->iIntegral) > 0)
            hItem->iIntegral = LOWORD(ptvi->iIntegral);
    }

    if (ptvi->mask & TVIF_STATE)
    {
        // don't & ptvi->state with TVIS_ALL because win95 didn't
        // and setting TVIS_FOCUS was retrievable even though we don't use it
        UINT change = (hItem->state ^ ptvi->state) & ptvi->stateMask;

        if (change)
        {
            // BUGBUG: (TVIS_SELECTED | TVIS_DROPHILITED) changes
            // should effect tree state
            hItem->state ^= change;
            bActualChange = TRUE;
            fEraseIfTransparent = TRUE;

            if (hItem->state & TVIS_BOLD) {
                if (!pTree->hFontBold)
                    TV_CreateBoldFont(pTree);
             }

            if (change & TVIS_BOLD){
                // do this because changing the boldness
                uRDWFlags |= RDW_ERASE;
                fRecalcWidth = TRUE;
            }

            fStateImageChange = change & TVIS_STATEIMAGEMASK;
            if (fStateImageChange) {
                uRDWFlags |= RDW_ERASE;
                // Adding/removing a state image changes the ITEM_OFFSET
                // If old image was 0, then we are adding.
                // If new image is 0, then we are removing.
                // (If old=new, then we don't get into this code path, so we
                // don't have to worry about that case.)
                if (!(hItem->state & TVIS_STATEIMAGEMASK) || // new
                    !((hItem->state ^ change) & TVIS_STATEIMAGEMASK)) { // old
                    fRecalcWidth = TRUE;
                }
            }

            fFocusSel = ((change & TVIS_SELECTED) != 0);
        }
    }

    if (fRecalcWidth) {
        hItem->iWidth = 0;          // Invalidate old width
        if (TV_IsShowing(hItem)) {
            TV_ScrollBarsAfterSetWidth(pTree, hItem);
        }
    }

    // force a redraw if something changed AND if we are not
    // inside of a paint of this guy (callbacks will set the
    // item on the paint callback to implement lazy data schemes)

    if (bActualChange && (pTree->hItemPainting != hItem))
    {
        if (fEraseIfTransparent) {
            if (pTree->hImageList) {
                if (ImageList_GetBkColor(pTree->hImageList) == CLR_NONE) {
                    uRDWFlags |= RDW_ERASE;
                }
            }

        }

        // If item height changed, then we've got a lot of cleaning up
        // to do.
        if (hItem->iIntegral != iIntegralPrev)
        {
            TV_ScrollBarsAfterResize(pTree, hItem, iIntegralPrev, uRDWFlags);
        }
        else
        {
            TV_InvalidateItem(pTree, hItem, uRDWFlags);
        }

        // REVIEW: we might need to update the scroll bars if the
        // text length changed!
    }

    if (bActualChange)
    {
        if (fName)
            MyNotifyWinEvent(EVENT_OBJECT_NAMECHANGE, pTree->ci.hwnd, OBJID_CLIENT,
                (LONG_PTR)hItem);

        if (fFocusSel)
        {
            MyNotifyWinEvent(EVENT_OBJECT_FOCUS, pTree->ci.hwnd, OBJID_CLIENT,
                (LONG_PTR)hItem);
            MyNotifyWinEvent(((hItem->state & TVIS_SELECTED) ?
                EVENT_OBJECT_SELECTIONADD : EVENT_OBJECT_SELECTIONREMOVE),
                pTree->ci.hwnd, OBJID_CLIENT, (LONG_PTR)hItem);
        }

        if (fStateImageChange)
            MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, pTree->ci.hwnd, OBJID_CLIENT,
                (LONG_PTR)hItem);
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

BOOL TV_IsItemTruncated(PTREE pTree, TREEITEM *hItem, LPRECT lprc)
{
    if (TV_GetItemRect(pTree,hItem,lprc,TRUE)) {
        lprc->left -= g_cxEdge;
        lprc->top -= g_cyBorder;
        if ((lprc->left + hItem->iWidth) > pTree->cxWnd) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL TV_HandleTTNShow(PTREE pTree, LPNMHDR lpnm)
{
    if (pTree->hToolTip && pTree->fPlaceTooltip) {
        LPNMTTSHOWINFO psi = (LPNMTTSHOWINFO)lpnm;
        RECT rc;
        TVITEMEX item;

        // Now get the text associated with that item
        item.stateMask = TVIS_BOLD;
        TV_GetItem(pTree, pTree->hToolTip, TVIF_STATE, &item);
        SendMessage(pTree->hwndToolTips, WM_SETFONT, (WPARAM)((item.state & TVIS_BOLD) ? pTree->hFontBold : pTree->hFont), 0);

        TV_GetItemRect(pTree, pTree->hToolTip, &rc, TRUE);

        MapWindowRect(pTree->ci.hwnd, HWND_DESKTOP, &rc);
        // We draw the text with margins, so take those into account too.
        // These values come from TV_DrawItem...
        rc.top += g_cyBorder;
        rc.left += g_cxLabelMargin;

        //
        //  At this point, (rc.left, rc.top) are the coordinates we pass
        //  to DrawText.  Ask the tooltip how we should position it so the
        //  tooltip text shows up in precisely the same location.
        //
        // BUGBUG raymondc v6: wrong coordinates if app has used TVM_SETITEMHEIGHT

        SendMessage(pTree->hwndToolTips, TTM_ADJUSTRECT, TRUE, (LPARAM)&rc);
        SetWindowPos(pTree->hwndToolTips, NULL, rc.left, rc.top,0,0,
                     SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);
        // This is an inplace tooltip, so disable animation.
        psi->dwStyle |= TTS_NOANIMATE;
        // handled!
        return TRUE;
    }

    return FALSE;
}

//
//  Copy the font from the treeview item into the tooltip so the tooltip
//  shows up in the correct font.
//
BOOL TV_HandleTTCustomDraw(PTREE pTree, LPNMTTCUSTOMDRAW pnm)
{
    if (pTree->hToolTip && pTree->fPlaceTooltip &&
        (pnm->nmcd.dwDrawStage == CDDS_PREPAINT ||
         pnm->nmcd.dwDrawStage == CDDS_ITEMPREPAINT))
    {
        //
        //  Set up the customdraw DC to match the font of the TV item.
        //
        TVFAKEDRAW tvfd;
        DWORD dwCustom = 0;
        TreeView_BeginFakeCustomDraw(pTree, &tvfd);
        dwCustom = TreeView_BeginFakeItemDraw(&tvfd, pTree->hToolTip);

        // If client changed the font, then transfer the font
        // from our private hdc into the tooltip's HDC.  We use
        // a private HDC because we only want to let the app change
        // the font, not the colors or anything else.
        if (dwCustom & CDRF_NEWFONT)
        {
            SelectObject(pnm->nmcd.hdc, GetCurrentObject(tvfd.nmcd.nmcd.hdc, OBJ_FONT));
        }
        TreeView_EndFakeItemDraw(&tvfd);
        TreeView_EndFakeCustomDraw(&tvfd);

        // Don't return other wacky flags to TT, since all we
        // did was change the font (if even that)
        return dwCustom & CDRF_NEWFONT;

    }
    return CDRF_DODEFAULT;

}

BOOL TV_SetToolTipTarget(PTREE pTree, HTREEITEM hItem)
{
    // update the item we're showing the bubble for...
    if (pTree->hToolTip != hItem) {
        // the hide will keep us from flashing
        ShowWindow(pTree->hwndToolTips, SW_HIDE);
        UpdateWindow(pTree->hwndToolTips);
        pTree->hToolTip = hItem;
        SendMessage(pTree->hwndToolTips, TTM_UPDATE, 0, 0);
        return TRUE;
    }
    return FALSE;
}

TREEITEM* TV_ItemAtCursor(PTREE pTree, LPRECT prc)
{
    RECT rc;
    UINT wHitCode;
    TREEITEM* hItem;

    GetCursorPos((LPPOINT)&rc);
    ScreenToClient(pTree->ci.hwnd, (LPPOINT)&rc);
    hItem = TV_CheckHit(pTree,rc.left,rc.top,&wHitCode);

    if (prc)
        *prc = rc;
    if (!(wHitCode & TVHT_ONITEM))
        hItem = NULL;

    return hItem;
}

BOOL TV_UpdateToolTipTarget(PTREE pTree)
{
    RECT rc;
    TREEITEM *hItem = TV_ItemAtCursor(pTree, &rc);

    if (!(pTree->ci.style & TVS_NOTOOLTIPS) 
            && !TV_IsItemTruncated(pTree, hItem, &rc)
            && !(pTree->ci.style & TVS_INFOTIP))
        hItem = NULL;
//    else if (!(pTree->ci.style & TVS_NOTOOLTIPS)
//                    || (pTree->ci.style & TVS_INFOTIP))
    return TV_SetToolTipTarget(pTree, hItem);
}

BOOL TV_UpdateToolTip(PTREE pTree)
{
    if (pTree->hwndToolTips && pTree->fRedraw)
       return (TV_UpdateToolTipTarget(pTree));
    return TRUE;
}

BOOL TV_SetInsertMark(PTREE pTree, HTREEITEM hItem, BOOL fAfter)
{
    if (!ValidateTreeItem(hItem, VTI_NULLOK))   // NULL means remove insert mark
        return FALSE;

    TV_InvalidateInsertMarkRect(pTree, TRUE); // Make sure the old one gets erased

    pTree->fInsertAfter = BOOLIFY(fAfter);
    pTree->htiInsert = hItem;

    TV_InvalidateInsertMarkRect(pTree, FALSE); // Make sure the new one gets drawn

    return TRUE;
}

BOOL TV_GetInfoTip(PTREE pTree, LPTOOLTIPTEXT lpttt, HTREEITEM hti, LPTSTR szBuf, int cch)
{
    NMTVGETINFOTIP git;

    szBuf[0] = 0;
    git.pszText = szBuf;
    git.cchTextMax = cch;
    git.hItem = hti;
    git.lParam = hti->lParam;

    // for folded items pszText is prepopulated with the
    // item text, clients should append to this

    CCSendNotify(&pTree->ci, TVN_GETINFOTIP, &git.hdr);

    CCSetInfoTipWidth(pTree->ci.hwnd, pTree->hwndToolTips);
    Str_Set(&pTree->pszTip, git.pszText);
    lpttt->lpszText = pTree->pszTip;
#ifdef WINDOWS_ME
    if(pTree->ci.style & TVS_RTLREADING){
    lpttt->uFlags |= TTF_RTLREADING;
    }
#endif // WINDOWS_ME
    return lpttt->lpszText && lpttt->lpszText[0];
}




void TV_HandleNeedText(PTREE pTree, LPTOOLTIPTEXT lpttt)
{
    TVITEMEX tvItem;
    TCHAR szBuf[INFOTIPSIZE];
    RECT rc;
    HTREEITEM hItem;

    // No distracting tooltips while in-place editing, please
    if (pTree->htiEdit)
    {
        return;
    }

    // If the cursor isn't over anything, then stop
    hItem = TV_ItemAtCursor(pTree, &rc);
    if (!hItem)
        return;

    // If the item has an infotip, then use it
    if (pTree->ci.style & TVS_INFOTIP) {
        if (hItem && TV_GetInfoTip(pTree, lpttt, hItem, szBuf, ARRAYSIZE(szBuf))) {
            pTree->fPlaceTooltip = FALSE;
            pTree->hToolTip = hItem;
            return;
        }
    }

    // Else it isn't an infotip
    CCResetInfoTipWidth(pTree->ci.hwnd, pTree->hwndToolTips);

    // If the item is not truncated, then no need for a tooltip
    if (!TV_IsItemTruncated(pTree, hItem, &rc))
    {
        tvItem.hItem = NULL;
        return;
    }

    // Display an in-place tooltip for the item
    pTree->fPlaceTooltip = TRUE;
    pTree->hToolTip = hItem;
    tvItem.hItem = hItem;
    tvItem.mask = TVIF_TEXT | TVIF_STATE;
    tvItem.pszText = szBuf;
    tvItem.stateMask = TVIS_DROPHILITED | TVIS_SELECTED;
    COMPILETIME_ASSERT(MAXLABELTEXT <= ARRAYSIZE(szBuf));
    tvItem.cchTextMax = MAXLABELTEXT;
    TV_OnGetItem(pTree,&tvItem);

    Str_Set(&pTree->pszTip, tvItem.pszText);
    lpttt->lpszText = pTree->pszTip;
    DebugMsg(DM_TRACE, TEXT("TV_HandleNeedText for %d returns %s"), tvItem.hItem, lpttt->szText);
}

#ifdef UNICODE
//
//  Visual Studio 5.0 Books Online (part of VB 5.0) subclasses
//  us and responds NFR_ANSI, so we end up getting TTN_NEEDTEXTA
//  instead of TTN_NEEDTEXTW.  We can't risk forcing the tooltip
//  to UNICODE because some other apps may have done this on purpose
//  (because they intend to intercept TTN_NEEDTEXTA and do custom tooltips).
//  So support the ANSI tooltip notification so VB stays happy.
//  Note: This doesn't have to be efficient, as it's an error case anyway.
//
void TV_HandleNeedTextA(PTREE pTree, LPTOOLTIPTEXTA lptttA)
{
    TOOLTIPTEXT ttt;
    ttt.szText[0] = TEXT('\0');
    ttt.hdr       = lptttA->hdr;
    ttt.lpszText  = ttt.szText;
    ttt.hinst     = lptttA->hinst;
    ttt.uFlags    = lptttA->uFlags;
    ttt.lParam    = lptttA->lParam;

    TV_HandleNeedText(pTree, &ttt);
    if (pTree->pszTipA)
        LocalFree(pTree->pszTipA);
    pTree->pszTipA = ProduceAFromW(pTree->ci.uiCodePage, ttt.lpszText);
    lptttA->lpszText = pTree->pszTipA;
    lptttA->uFlags  = ttt.uFlags;
}
#endif

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
            KillTimer(pTree->ci.hwnd, IDT_NAMEEDIT);

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
            KillTimer(pTree->ci.hwnd, IDT_SCROLLWAIT);
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
            // We lost focus, so dismiss edit and save changes
            // (Note that the owner might reject the change and restart
            // edit mode, which traps the user.  Owners need to give the
            // user a way to get out.)

#if 0       // BUGBUG raymondc v6.0
            //
            //  Fix horrible undocumented hanging problem:  LVN_ENDLABELEDIT
            //  is sent in response to EN_KILLFOCUS, which is send in response
            //  to WM_KILLFOCUS, and it is undocumented that you cannot display
            //  UI during WM_KILLFOCUS when a journal record hook is active,
            //  because the presence of a hook forces serialization of activation,
            //  and so when you put up UI, you generate activation changes, which
            //  get stuck because you haven't finished responding to the previous
            //  WM_KILLFOCUS message yet.
            //
            //  See NT bug 414634.
            //
            if (InSendMessage())
                ReplyMessage(0);
#endif

            if (!TV_DismissEdit(pTree, FALSE))
               return;
            break;

        case HN_BEGINDIALOG: // penwin is bringing up a dialog
            ASSERT(GetSystemMetrics(SM_PENWINDOWS)); // only on a pen system
            pTree->fNoDismissEdit = TRUE;
            break;

        case HN_ENDDIALOG: // penwin has destroyed dialog
            ASSERT(GetSystemMetrics(SM_PENWINDOWS)); // only on a pen system
            pTree->fNoDismissEdit = FALSE;
            break;
        }

        // Forward edit control notifications up to parent
        //
        if (IsWindow(hwndCtl))
            FORWARD_WM_COMMAND(pTree->ci.hwndParent, id, hwndCtl, codeNotify, SendMessage);
    }
}

HIMAGELIST CreateCheckBoxImagelist(HIMAGELIST himl, BOOL fTree, BOOL fUseColorKey, BOOL fMirror);
void TV_CreateToolTips(PTREE pTree);

void TV_InitCheckBoxes(PTREE pTree)
{
    HIMAGELIST himl;
    TVITEMEX ti;
    BOOL fNoColorKey = FALSE;    // Backwards: If Cleartype is turned on, then we don't use colorkey.

    if (g_bRunOnNT5)
    {
#ifdef CLEARTYPE    // Don't use SPI_CLEARTYPE because it's defined because of APIThk, but not in NT.
        SystemParametersInfo(SPI_GETCLEARTYPE, 0, &fNoColorKey, 0);
#endif
    }

    himl = CreateCheckBoxImagelist(pTree->hImageList, TRUE, !fNoColorKey, IS_WINDOW_RTL_MIRRORED(pTree->ci.hwnd));
    if (pTree->hImageList) 
    {
        COLORREF cr = ImageList_GetBkColor(pTree->hImageList);
        ImageList_SetBkColor(himl, fNoColorKey? (CLR_NONE) : (cr));
    }

    TV_SetImageList(pTree, himl, TVSIL_STATE);

    ti.mask = TVIF_STATE;
    ti.state = INDEXTOSTATEIMAGEMASK(1);
    ti.stateMask = TVIS_STATEIMAGEMASK;
    TV_SetItemRecurse(pTree, pTree->hRoot, &ti);
}

void NEAR TV_OnStyleChanged(PTREE pTree, WPARAM gwl, LPSTYLESTRUCT pinfo)
{
    // Style changed: redraw everything...
    //
    // try to do this smartly, avoiding unnecessary redraws
    if (gwl == GWL_STYLE)
    {
        DWORD changeFlags;
        DWORD styleNew;

        TV_DismissEdit(pTree, FALSE);   // BUGBUG:  FALSE == accept changes.  Is this right?

        // You cannot combine TVS_HASLINES and TVS_FULLROWSELECT
        // because it doesn't work
        styleNew = pinfo->styleNew;
        if (styleNew & TVS_HASLINES) {
            if (styleNew & TVS_FULLROWSELECT) {
                DebugMsg(DM_ERROR, TEXT("Cannot combine TVS_HASLINES and TVS_FULLROWSELECT"));
            }
            styleNew &= ~TVS_FULLROWSELECT;
        }

        changeFlags = pTree->ci.style ^ styleNew; // those that changed
        pTree->ci.style = styleNew;               // change our version

#ifdef WINDOWS_ME
        pTree->ci.style &= ~TVS_RTLREADING;
        pTree->ci.style |= (pinfo->styleNew & TVS_RTLREADING);       
#endif

        if (changeFlags & (TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT))
            TV_CreateIndentBmps(pTree);

        if (changeFlags & TVS_CHECKBOXES) {
            if (pTree->ci.style & TVS_CHECKBOXES) {
                TV_InitCheckBoxes(pTree);
            }
        }

        if (changeFlags & TVS_NOTOOLTIPS) {
            if (pTree->ci.style & TVS_NOTOOLTIPS) {
                DestroyWindow(pTree->hwndToolTips);
                pTree->hwndToolTips = NULL;
            } else {
                TV_CreateToolTips(pTree);
            }
        }

        if (changeFlags & TVS_TRACKSELECT) {
            if (!(pTree->ci.style & TVS_TRACKSELECT)) {
                if (pTree->hHot) {
                    TV_InvalidateItem(pTree, pTree->hHot, RDW_INVALIDATE | RDW_ERASE);
                    pTree->hHot = NULL;
                }
            }
        }
        // Checkboxes and stuff may have changed width - go recompute
        TV_ScrollBarsAfterSetWidth(pTree, NULL);
    }
#ifdef WINDOWS_ME
    else if (gwl == GWL_EXSTYLE)
    {
        DWORD changeFlags;
        changeFlags = (pinfo->styleNew & WS_EX_RTLREADING) ?TVS_RTLREADING :0;

        if (changeFlags ^ (pTree->ci.style & TVS_RTLREADING))
        {
            pTree->ci.style ^= TVS_RTLREADING;
            TV_DismissEdit(pTree, FALSE);   // Cancels edits

            DestroyWindow(pTree->hwndToolTips);
            pTree->hwndToolTips = NULL;
            TV_CreateToolTips(pTree);
        }
    }
#endif

}

void TV_OnMouseMove(PTREE pTree, DWORD dwPos, WPARAM wParam)
{
    if (pTree->ci.style & TVS_TRACKSELECT) {
        POINT pt;
        HTREEITEM hHot;
        UINT wHitCode;

        pt.x = GET_X_LPARAM(dwPos);
        pt.y = GET_Y_LPARAM(dwPos);

        hHot = TV_CheckHit(pTree,pt.x,pt.y,&wHitCode);

        if (!(pTree->ci.style & TVS_FULLROWSELECT) &&
            !(wHitCode & TVHT_ONITEM)) {
            hHot = NULL;
        }

        if (hHot != pTree->hHot) {
            TV_InvalidateItem(pTree, pTree->hHot, RDW_INVALIDATE);
            TV_InvalidateItem(pTree, hHot, RDW_INVALIDATE);
            pTree->hHot = hHot;
            // update now so that we won't have an invalid area
            // under the tooltips
            UpdateWindow(pTree->ci.hwnd);
        }
    }

    if (pTree->hwndToolTips) {

        if (!TV_UpdateToolTip(pTree)) {
            RelayToToolTips(pTree->hwndToolTips, pTree->ci.hwnd, WM_MOUSEMOVE, wParam, dwPos);
        }
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

void TV_OnSetBkColor(PTREE pTree, COLORREF clr)
{
    if (pTree->clrBk != (COLORREF)-1) {
        DeleteObject(pTree->hbrBk);
    }

    pTree->clrBk = clr;
    if (clr != (COLORREF)-1) {
        pTree->hbrBk = CreateSolidBrush(clr);
    }
    TV_CreateIndentBmps(pTree); // This also invalidates
}

BOOL TV_TranslateAccelerator(HWND hwnd, LPMSG lpmsg)
{
    if (!lpmsg)
        return FALSE;

    if (GetFocus() != hwnd)
        return FALSE;

    switch (lpmsg->message) {

    case WM_KEYUP:
    case WM_KEYDOWN:

        if (GetKeyState(VK_CONTROL) < 0) {
            switch (lpmsg->wParam) {
            case VK_LEFT:
            case VK_RIGHT:
            case VK_PRIOR:
            case VK_HOME:
            case VK_NEXT:
            case VK_END:
            case VK_UP:
            case VK_DOWN:
                TranslateMessage(lpmsg);
                DispatchMessage(lpmsg);
                return TRUE;
            }
        } else {

            switch (lpmsg->wParam) {

            case VK_RETURN:
            case VK_PRIOR:
            case VK_HOME:
            case VK_NEXT:
            case VK_END:
            case VK_SUBTRACT:
            case VK_ADD:
            case VK_MULTIPLY:
            case VK_LEFT:
            case VK_BACK:
            case VK_UP:
            case VK_RIGHT:
            case VK_DOWN:
            case VK_SPACE:
                TranslateMessage(lpmsg);
                DispatchMessage(lpmsg);
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

// ----------------------------------------------------------------------------
//
//  TV_WndProc
//
//  Take a guess.
//
// ----------------------------------------------------------------------------

LRESULT CALLBACK TV_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PTREE pTree = (PTREE)GetWindowPtr(hwnd, 0);

    if (pTree) 
    {
        if ((uMsg >= WM_MOUSEFIRST) && (uMsg <= WM_MOUSELAST) &&
            (pTree->ci.style & TVS_TRACKSELECT) && !pTree->fTrackSet) 
        {

            TRACKMOUSEEVENT tme;

            pTree->fTrackSet = TRUE;
            tme.cbSize = sizeof(tme);
            tme.hwndTrack = pTree->ci.hwnd;
            tme.dwFlags = TME_LEAVE;

            TrackMouseEvent(&tme);
        }
        else if (uMsg == g_uDragImages)
        {
            return TV_GenerateDragImage(pTree, (SHDRAGIMAGE*)lParam);
        }

    } 
    else 
    {
        if (uMsg == WM_CREATE) 
            return TV_OnCreate(hwnd, (LPCREATESTRUCT)lParam);

        goto DoDefault;
    }



    switch (uMsg)
    {
        case WM_MOUSELEAVE:
            pTree->fTrackSet = FALSE;
            TV_InvalidateItem(pTree, pTree->hHot, RDW_INVALIDATE);
            pTree->hHot = NULL;
            TV_PopBubble(pTree);
            break;
            
        case TVMP_CALCSCROLLBARS:
            TV_CalcScrollBars(pTree);
            break;
            

        case TVM_GETITEMSTATE:
            {
                TVITEMEX tvi;

                tvi.mask = TVIF_STATE;
                tvi.stateMask = (UINT) lParam;
                tvi.hItem = (HTREEITEM)wParam;
                if (!TV_OnGetItem(pTree, &tvi))
                    return 0;

                return tvi.state;
            }
            
        case TVM_SETBKCOLOR:
        {
            LRESULT lres = (LRESULT)pTree->clrBk;
            TV_OnSetBkColor(pTree, (COLORREF)lParam);
            return lres;
        }

        case TVM_SETTEXTCOLOR:
        {
            LRESULT lres = (LRESULT)pTree->clrText;
            pTree->clrText = (COLORREF)lParam;
            TV_CreateIndentBmps(pTree); // This also invalidates
            return lres;
        }

        case TVM_GETBKCOLOR:
            return (LRESULT)pTree->clrBk;

        case TVM_GETTEXTCOLOR:
            return (LRESULT)pTree->clrText;

        case TVM_GETSCROLLTIME:
            return (LRESULT)pTree->uMaxScrollTime;

        case TVM_SETSCROLLTIME:
        {
            UINT u = pTree->uMaxScrollTime;
            pTree->uMaxScrollTime = (UINT)wParam;
            return (LRESULT)u;
        }


#ifdef UNICODE
        case TVM_INSERTITEMA:
            if (!lParam)
                return 0;

            return (LRESULT)TV_InsertItemA(pTree, (LPTV_INSERTSTRUCTA)lParam);

        case TVM_GETITEMA:
            if (!lParam)
                return 0;

            return (LRESULT)TV_OnGetItemA(pTree, (LPTVITEMEXA)lParam);

        case TVM_SETITEMA:
            if (!lParam)
                return 0;

            return (LRESULT)TV_SetItemA(pTree, (LPTVITEMEXA)lParam);

#endif
        case TVM_INSERTITEM:
            return (LRESULT)TV_InsertItem(pTree, (LPTV_INSERTSTRUCT)lParam);

        case TVM_DELETEITEM:
            // Assume if items are being deleted that name editing is invalid.
            TV_DismissEdit(pTree, TRUE);
            return TV_DeleteItem(pTree, (TREEITEM FAR *)lParam, TVDI_NORMAL);

        case TVM_GETNEXTITEM:
            return (LRESULT)TV_GetNextItem(pTree, (TREEITEM FAR *)lParam, wParam);

        case TVM_GETITEMRECT:
            // lParam points to hItem to get rect from on input
            if (!lParam)
                return 0;
            if (!ValidateTreeItem(*(HTREEITEM FAR *)lParam, 0))
                return 0;               // Invalid parameter
            return (LRESULT)TV_GetItemRect(pTree, *(HTREEITEM FAR *)lParam, (LPRECT)lParam, (BOOL)wParam);

        case TVM_GETITEM:
            return (LRESULT)TV_OnGetItem(pTree, (LPTVITEMEX)lParam);

        case TVM_SETITEM:
            return (LRESULT)TV_SetItem(pTree, (LPCTVITEMEX)lParam);

        case TVM_ENSUREVISIBLE:
            if (!ValidateTreeItem((HTREEITEM)lParam, 0))
                return 0;
            return TV_EnsureVisible(pTree, (TREEITEM FAR *)lParam);

        case TVM_SETIMAGELIST:
            return (LRESULT)(ULONG_PTR)TV_SetImageList(pTree, (HIMAGELIST)lParam, (int)wParam);

        case TVM_EXPAND:
            if (!ValidateTreeItem((HTREEITEM)lParam, 0))
                return FALSE;               // invalid parameter
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

#ifdef UNICODE
        case TVM_GETISEARCHSTRINGA:
            if (GetFocus() == pTree->ci.hwnd)
                return (LRESULT)GetIncrementSearchStringA(&pTree->is, pTree->ci.uiCodePage, (LPSTR)lParam);
            else
                return 0;
#endif

        case TVM_GETISEARCHSTRING:
            if (GetFocus() == pTree->ci.hwnd)
                return (LRESULT)GetIncrementSearchString(&pTree->is, (LPTSTR)lParam);
            else
                return 0;

#ifdef UNICODE
        case TVM_EDITLABELA:
            {
            LPWSTR lpEditString = NULL;
            HWND   hRet;

            if (wParam) {
                lpEditString = ProduceWFromA(pTree->ci.uiCodePage, (LPSTR)wParam);
            }

            hRet = TV_EditLabel(pTree, (HTREEITEM)lParam, lpEditString);

            if (lpEditString) {
                FreeProducedString(lpEditString);
            }

            return MAKELRESULTFROMUINT(hRet);
            }
#endif

        case TVM_EDITLABEL:
            return MAKELRESULTFROMUINT(TV_EditLabel(pTree, (HTREEITEM)lParam,
                    (LPTSTR)wParam));


        case TVM_GETVISIBLECOUNT:
            return TV_GetVisCount(pTree, (BOOL) wParam);

        case TVM_SETINDENT:
            TV_SetIndent(pTree, wParam);
            pTree->fIndentSet = TRUE;
            break;

        case TVM_GETINDENT:
            return MAKELRESULTFROMUINT(pTree->cxIndent);

        case TVM_CREATEDRAGIMAGE:
            return MAKELRESULTFROMUINT(TV_CreateDragImage(pTree, (TREEITEM FAR *)lParam));

        case TVM_GETEDITCONTROL:
            return (LRESULT)(ULONG_PTR)pTree->hwndEdit;

        case TVM_SORTCHILDREN:
            return TV_SortChildren(pTree, (TREEITEM FAR *)lParam, (BOOL)wParam);

        case TVM_SORTCHILDRENCB:
            return TV_SortChildrenCB(pTree, (TV_SORTCB FAR *)lParam, (BOOL)wParam);

        case TVM_SELECTITEM:
            return TV_SelectItem(pTree, wParam, (TREEITEM FAR *)lParam, TVSIF_NOTIFY | TVSIF_UPDATENOW, TVC_UNKNOWN);

        case TVM_ENDEDITLABELNOW:
            return TV_DismissEdit(pTree, (BOOL)wParam);

        case TVM_GETTOOLTIPS:
            return (LRESULT)(ULONG_PTR)pTree->hwndToolTips;

        case TVM_SETTOOLTIPS:{
            HWND hwndOld = pTree->hwndToolTips;

            pTree->hwndToolTips = (HWND)wParam;
            return (LRESULT)(ULONG_PTR)hwndOld;
        }

        case TVM_GETITEMHEIGHT:
            return pTree->cyItem;

        case TVM_SETITEMHEIGHT:
        {
            int iOld = pTree->cyItem;
            pTree->fCyItemSet = (wParam != (WPARAM)-1);
            pTree->cyItem = (SHORT)wParam; // must be even
            TV_SetItemHeight(pTree);
            return iOld;
        }
        case TVM_SETBORDER:
        {
            int cyOld = pTree->cyBorder
                , cxOld = pTree->cxBorder;

            if (wParam & TVSBF_YBORDER)
                pTree->cyBorder = HIWORD(lParam);
            if (wParam & TVSBF_XBORDER)
                pTree->cxBorder = LOWORD(lParam);

            TV_CalcScrollBars(pTree);
            return MAKELONG(cxOld, cyOld);
        }
        case TVM_GETBORDER:
            return MAKELONG(pTree->cxBorder, pTree->cyBorder);
        case TVM_SETINSERTMARK:
            return TV_SetInsertMark(pTree, (TREEITEM FAR *)lParam, (BOOL) wParam);
        
        case TVM_SETINSERTMARKCOLOR:
        {
            LRESULT lres = (LRESULT)pTree->clrim;
            pTree->clrim = (COLORREF) lParam;
            TV_InvalidateInsertMarkRect(pTree, FALSE); // Repaint in new color
            return lres;
        }
        case TVM_GETINSERTMARKCOLOR:
            return pTree->clrim;

        case TVM_TRANSLATEACCELERATOR:
            return TV_TranslateAccelerator(hwnd, (LPMSG)lParam);

        case TVM_SETLINECOLOR:
        {
            LRESULT lres = (LRESULT)pTree->clrLine;
            pTree->clrLine = (COLORREF)lParam;
            TV_CreateIndentBmps(pTree); // This also invalidates
            return lres;
        }

        case TVM_GETLINECOLOR:
            return (LRESULT)pTree->clrLine;

#if defined(FE_IME) || !defined(WINNT)
        case WM_IME_COMPOSITION:
            // Now only Korean version is interested in incremental search with composition string.
            if (g_fDBCSInputEnabled) {
            if (((ULONG_PTR)GetKeyboardLayout(0L) & 0xF000FFFFL) == 0xE0000412L)
            {
                if (TV_OnImeComposition(pTree, wParam, lParam))
                {
                    lParam &= ~GCS_RESULTSTR;
                    goto DoDefault;
                }
                else
                    break;
            }
            }
            goto DoDefault;
#endif

        case WM_CHAR:
            if (pTree->iPuntChar) {
                pTree->iPuntChar--;
                return TRUE;
            } else {
                return HANDLE_WM_CHAR(pTree, wParam, lParam, TV_OnChar);
            }

        case WM_DESTROY:
            TV_DestroyTree(pTree);
            break;

        case WM_SETCURSOR:
            {
                NMMOUSE nm;
                HTREEITEM hItem;
                nm.dwHitInfo = lParam;
                hItem = TV_ItemAtCursor(pTree, NULL);
                if(hItem)
                {
                    nm.dwItemSpec = (ULONG_PTR)hItem;
                    nm.dwItemData = (ULONG_PTR)(hItem->lParam);
                }
                else
                {
                    nm.dwItemSpec = 0;
                    nm.dwItemData = 0;
                }
                             
#ifdef UNIX
		if (pTree->hwndEdit != (HWND)wParam)
#endif
                if (CCSendNotify(&pTree->ci, NM_SETCURSOR, &nm.hdr)) 
                {
                    return 0;
                }
            }
            if (pTree->ci.style & TVS_TRACKSELECT) {
                if (pTree->hHot) {
                    if (!pTree->hCurHot)
                        pTree->hCurHot = LoadHandCursor(0);
                    SetCursor(pTree->hCurHot);
                    return TRUE;
                }
            }
            goto DoDefault;
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
                FillRect((HDC)wParam, &rc, pTree->hbrBk);
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
                IncrementSearchString(&pTree->is, 0, NULL);
                goto DoDefault;


        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
            TV_ButtonDown(pTree, uMsg, (UINT) wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
            break;

        case WM_KILLFOCUS:
            // Reset wheel scroll amount
            gcWheelDelta = 0;

            pTree->fFocus = FALSE;
            if (pTree->hCaret)
            {
                TV_InvalidateItem(pTree, pTree->hCaret, RDW_INVALIDATE);
                UpdateWindow(pTree->ci.hwnd);
            }
            CCSendNotify(&pTree->ci, NM_KILLFOCUS, NULL);
            IncrementSearchString(&pTree->is, 0, NULL);
            break;

        case WM_SETFOCUS:
            ASSERT(gcWheelDelta == 0);

            pTree->fFocus = TRUE;
            if (pTree->hCaret)
            {
                TV_InvalidateItem(pTree, pTree->hCaret, RDW_INVALIDATE);
                MyNotifyWinEvent(EVENT_OBJECT_FOCUS, hwnd, OBJID_CLIENT, (LONG_PTR)pTree->hCaret);
            }
            else
                TV_SelectItem(pTree, TVGN_CARET, pTree->hTop, TVSIF_NOTIFY | TVSIF_UPDATENOW, TVC_INTERNAL);

            CCSendNotify(&pTree->ci, NM_SETFOCUS, NULL);
            break;

        case WM_GETFONT:
            return MAKELRESULTFROMUINT(pTree->hFont);

        case WM_SETFONT:
            TV_OnSetFont(pTree, (HFONT) wParam, (BOOL) lParam);
            break;

        case WM_SIZE:
            TV_SizeWnd(pTree, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;

        case WM_ENABLE:
            // HACK: we don't get WM_STYLECHANGE on EnableWindow()
            if (wParam)
                pTree->ci.style &= ~WS_DISABLED;        // enabled
            else
                pTree->ci.style |= WS_DISABLED; // disabled
            TV_CreateIndentBmps(pTree); // This invalidates the whole window!
            break;

        case WM_SYSCOLORCHANGE:
            InitGlobalColors();
#if 0
            if(pTree->hwndToolTips) {
                SendMessage(pTree->hwndToolTips, TTM_SETTIPBKCOLOR, (WPARAM)GetSysColor(COLOR_WINDOW), 0);
                SendMessage(pTree->hwndToolTips, TTM_SETTIPTEXTCOLOR, (WPARAM)GetSysColor(COLOR_WINDOWTEXT), 0);
            }
#endif

            TV_CreateIndentBmps(pTree); // This invalidates the whole window!
            break;

        case WM_RBUTTONDOWN:
            TV_SendRButtonDown(pTree, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;

        case WM_TIMER:
            TV_Timer(pTree, (UINT) wParam);
            break;

    case WM_MOUSEMOVE:
        TV_OnMouseMove(pTree, (DWORD) lParam, wParam);
        break;

        case WM_COMMAND:
            TV_Command(pTree, (int)GET_WM_COMMAND_ID(wParam, lParam), GET_WM_COMMAND_HWND(wParam, lParam),
                    (UINT)GET_WM_COMMAND_CMD(wParam, lParam));
            break;

        case WM_NOTIFY: {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            if ((lpnm->code <= PGN_FIRST) && (PGN_LAST <= lpnm->code)) {
                LRESULT TV_OnPagerControlNotify(PTREE pTree, LPNMHDR pnm);

                return TV_OnPagerControlNotify(pTree, lpnm);
            }
            if (lpnm->hwndFrom == pTree->hwndToolTips) {
                switch (lpnm->code) {
                case TTN_NEEDTEXT:
                    TV_HandleNeedText(pTree, (LPTOOLTIPTEXT)lpnm);
                    break;

#ifdef UNICODE
                case TTN_NEEDTEXTA:
                    TV_HandleNeedTextA(pTree, (LPTOOLTIPTEXTA)lpnm);
                    break;
#endif

                case TTN_SHOW:
                    return TV_HandleTTNShow(pTree, lpnm);

                case NM_CUSTOMDRAW:
                    return TV_HandleTTCustomDraw(pTree, (LPNMTTCUSTOMDRAW)lpnm);
                }
            }
            break;
        }

        case WM_NOTIFYFORMAT:
        return CIHandleNotifyFormat(&pTree->ci, lParam);

        case WM_MBUTTONDOWN:
            SetFocus(hwnd);
            goto DoDefault;

        case WM_GETOBJECT:
            if( lParam == OBJID_QUERYCLASSNAMEIDX )
                return MSAA_CLASSNAMEIDX_TREEVIEW;
            goto DoDefault;

#ifdef KEYBOARDCUES
        case WM_UPDATEUISTATE:
        {
            DWORD dwUIStateMask = MAKEWPARAM(0xFFFF, UISF_HIDEFOCUS);

            if (CCOnUIState(&(pTree->ci), WM_UPDATEUISTATE, wParam & dwUIStateMask, lParam))
                if (pTree->hCaret)
                    TV_InvalidateItem(pTree, pTree->hCaret, TRUE);

            goto DoDefault;
        }
#endif
        case WM_SYSKEYDOWN:
            TV_KeyDown(pTree, wParam, lParam);
            //fall through

        default:
            // Special handling of magellan mouse message
            if (uMsg == g_msgMSWheel) {
                BOOL  fScroll;
                BOOL  fDataZoom;
                DWORD dwStyle;
                int   cScrollLines;
                int   cPage;
                int   pos;
                int   cDetants;
                int iWheelDelta;

                if (g_bRunOnNT || g_bRunOnMemphis)
                {
                    iWheelDelta = (int)(short)HIWORD(wParam);
                    fScroll = !(wParam & (MK_SHIFT | MK_CONTROL));
                    fDataZoom = (BOOL) (wParam & MK_SHIFT);
                }
                else
                {
                    iWheelDelta = (int)wParam;
                    fDataZoom = (GetKeyState(VK_SHIFT) < 0);
                    fScroll = !fDataZoom && GetKeyState(VK_CONTROL) >= 0;
                }

                // Update count of scroll amount
                gcWheelDelta -= iWheelDelta;
                cDetants = gcWheelDelta / WHEEL_DELTA;
                if (cDetants != 0) {
                    gcWheelDelta %= WHEEL_DELTA;
                }

                if (fScroll) {
                    if (    g_ucScrollLines > 0 &&
                            cDetants != 0 &&
                            (WS_VSCROLL | WS_HSCROLL) & (dwStyle = GetWindowStyle(hwnd))) {

                        if (dwStyle & WS_VSCROLL) {
                            cPage = max(1, (pTree->cFullVisible - 1));
                            cScrollLines =
                                          cDetants *
                                          min(g_ucScrollLines, (UINT) cPage);

                            pos = max(0, pTree->hTop->iShownIndex + cScrollLines);
                            TV_VertScroll(pTree, SB_THUMBPOSITION, pos);
                        } else {
                            cPage = max(MAGIC_HORZLINE,
                                        (pTree->cxWnd - MAGIC_HORZLINE)) /
                                    MAGIC_HORZLINE;

                            cScrollLines =
                                          cDetants *
                                          (int) min((ULONG) cPage, g_ucScrollLines) *
                                          MAGIC_HORZLINE;

                            pos = max(0, pTree->xPos + cScrollLines);
                            TV_HorzScroll(pTree, SB_THUMBPOSITION, pos);
                        }
                    }
                    return 1;
                } else if (fDataZoom) {
                    UINT wHitCode;
                    POINT pt;

                    pt.x = GET_X_LPARAM(lParam);
                    pt.y = GET_Y_LPARAM(lParam);
                    ScreenToClient(hwnd, &pt);

                    // If we are rolling forward and hit an item then navigate into that
                    // item or expand tree (simulate lbuttondown which will do it).  We
                    // also need to handle rolling backwards over the ITEMBUTTON so
                    // that we can collapse the tree in that case.  Otherwise
                    // just fall through so it isn't handled.  In that case if we
                    // are being hosted in explorer it will do a backwards
                    // history navigation.
                    if (TV_CheckHit(pTree, pt.x, pt.y, &wHitCode) &&
                        (wHitCode & (TVHT_ONITEM | TVHT_ONITEMBUTTON))) {
                        UINT uFlags = TVBD_FROMWHEEL;
                        uFlags |= (iWheelDelta > 0) ? TVBD_WHEELFORWARD : TVBD_WHEELBACK;

                        if ((uFlags & TVBD_WHEELFORWARD) || (wHitCode == TVHT_ONITEMBUTTON)) {
                            TV_ButtonDown(pTree, WM_LBUTTONDOWN, 0, pt.x, pt.y, uFlags);
                            return 1;
                        }
                    }
                    // else fall through
                }
            } else {
                LRESULT lres;
                if (CCWndProc(&pTree->ci, uMsg, wParam, lParam, &lres))
                    return lres;
            }

DoDefault:
            return(DefWindowProc(hwnd, uMsg, wParam, lParam));
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
    ASSERT(pTree);

    if (!pTree)
        return 0L;  // wierd cases can get here...

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
    UINT seips;

    if (pTree->htiEdit == NULL)
        return;

    TV_GetItemRect(pTree, pTree->htiEdit, &rcLabel, TRUE);

    // get exact the text bounds (acount for borders used when drawing)

    InflateRect(&rcLabel, -g_cxLabelMargin, -g_cyBorder);

    seips = 0;
#ifdef DEBUG
    // If we are in one of the no-scroll modes then it's possible for the
    // resulting rectangle not to be visible.  Similarly, if the item itself
    // isn't visible, then the resulting rectangle is definitely not visible.
    // Tell SetEditInPlaceSize not to freak out in those cases.
    if ((pTree->ci.style & (TVS_NOSCROLL | TVS_NOHSCROLL)) ||
        !ITEM_VISIBLE(pTree->htiEdit))
        seips |= SEIPS_NOSCROLL;
#endif

    SetEditInPlaceSize(pTree->hwndEdit, &rcLabel, (HFONT)SendMessage(pTree->hwndEdit, WM_GETFONT, 0, 0), seips);
}


void NEAR TV_CancelEditTimer(PTREE pTree)
{
    if (pTree->fNameEditPending)
    {
        KillTimer(pTree->ci.hwnd, IDT_NAMEEDIT);
        pTree->fNameEditPending = FALSE;
    }
}

// BUGBUG: very similar code in lvicon.c


HWND NEAR TV_EditLabel(PTREE pTree, HTREEITEM hItem, LPTSTR pszInitial)
{
    TCHAR szLabel[MAXLABELTEXT];
    TV_DISPINFO nm;

    if (!(pTree->ci.style & TVS_EDITLABELS))
        return NULL;

    if (!ValidateTreeItem(hItem, 0))
        return NULL;

    TV_DismissEdit(pTree, FALSE);


    // Now get the text associated with that item
    nm.item.pszText = szLabel;
    nm.item.cchTextMax = ARRAYSIZE(szLabel);
    nm.item.stateMask = TVIS_BOLD;
    // this cast is ok as long as TVIF_INTEGRAL or anything past it isn't asked for
    TV_GetItem(pTree, hItem, TVIF_TEXT | TVIF_STATE, (LPTVITEMEX)&nm.item);

    // Must subtract one from ARRAYSIZE(szLabel) because Edit_LimitText
    // doesn't include the terminating NULL
    pTree->hwndEdit = CreateEditInPlaceWindow(pTree->ci.hwnd,
        pszInitial? pszInitial : nm.item.pszText, ARRAYSIZE(szLabel) - 1,
        WS_BORDER | WS_CLIPSIBLINGS | WS_CHILD | ES_LEFT | ES_AUTOHSCROLL,
        (nm.item.state & TVIS_BOLD) ? pTree->hFontBold : pTree->hFont);

    if (pTree->hwndEdit) {
        if (pszInitial)     // if initialized, it's dirty.
            SetWindowID(pTree->hwndEdit, 1);
        //
        // Now notify the parent of this window and see if they want it.
        // We do it after we cretae the window, but before we show it
        // such that our parent can query for it and do things like limit
        // the number of characters that are input
        nm.item.hItem = hItem;
        nm.item.state = hItem->state;
        nm.item.lParam = hItem->lParam;
        nm.item.mask = (TVIF_HANDLE | TVIF_STATE | TVIF_PARAM | TVIF_TEXT);

        if ((BOOL)CCSendNotify(&pTree->ci, TVN_BEGINLABELEDIT, &nm.hdr))
        {
            DestroyWindow(pTree->hwndEdit);
            pTree->hwndEdit = NULL;
            return NULL;
        }

        TV_PopBubble(pTree);

        TV_ScrollIntoView(pTree, hItem);

        pTree->pfnEditWndProc = SubclassWindow(pTree->hwndEdit, TV_EditWndProc);

        pTree->htiEdit = hItem;

        TV_SetEditSize(pTree);

        // Show the window and set focus to it.  Do this after setting the
        // size so we don't get flicker.
        SetFocus(pTree->hwndEdit);
        ShowWindow(pTree->hwndEdit, SW_SHOW);
        TV_InvalidateItem(pTree, hItem, RDW_INVALIDATE | RDW_ERASE);

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
    if (!IsWindowVisible(pTree->ci.hwnd))
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

    // TV_DeleteItemRecurse will set htiEdit to NULL if the program
    // deleted the items out from underneath us (while we are waiting
    // for the edit timer).
    htiEdit = pTree->htiEdit;

    if (htiEdit != NULL)
    {
        TV_DISPINFO nm;
        TCHAR szLabel[MAXLABELTEXT];

        DBG_ValidateTreeItem(htiEdit, 0);

        // Initialize notification message.
        nm.item.hItem = htiEdit;
        nm.item.lParam = htiEdit->lParam;
        nm.item.mask = 0;

        if (fCancel)
            nm.item.pszText = NULL;
        else {
            Edit_GetText(hwndEdit, szLabel, ARRAYSIZE(szLabel));
            nm.item.pszText = szLabel;
            nm.item.cchTextMax = ARRAYSIZE(szLabel);
            nm.item.mask |= TVIF_TEXT;
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

        fOkToContinue = (BOOL)CCSendNotify(&pTree->ci, TVN_ENDLABELEDIT, &nm.hdr);
        if (fOkToContinue && !fCancel)
        {
            // BUGBUG raymondc: The caller might have deleted the item in
            // response to the edit.  We should revalidate here (or make
            // delete item invalidate our edit item).  Treat a deletion
            // as if it were a rejected edit.

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
                    TV_SetItem(pTree, (LPTVITEMEX)&nm.item);
                }
                else
                {
                    CCSendNotify(&pTree->ci, TVN_SETDISPINFO, &nm.hdr);
                }
            }
        }
    }

    // If we did not reenter edit mode before now reset the edit state
    // variables to NULL
    if (hwndEdit == pTree->hwndEdit)
    {
        pTree->htiEdit = NULL;
        pTree->hwndEdit = NULL; // so we don't get reentered on the kill focus
    }

    // done with the edit control
    DestroyWindow(hwndEdit);

    return fOkToContinue;
}

LRESULT TV_OnCalcSize(PTREE pTree, LPNMHDR pnm)
{
    LPNMPGCALCSIZE pcalcsize = (LPNMPGCALCSIZE)pnm;

    switch(pcalcsize->dwFlag) {
    case PGF_CALCHEIGHT:
        pcalcsize->iHeight = pTree->cShowing * pTree->cyItem;
        TraceMsg(TF_WARNING, "tv.PGF_CALCHEIGHT: cShow=%d cShow*cyItem=%d AWR()=%d",
            pTree->cShowing, pTree->cShowing * pTree->cyItem, pcalcsize->iHeight);
        break;

    case PGF_CALCWIDTH:
        break;
    }
    return 0L;
}

LRESULT TV_OnPagerControlNotify(PTREE pTree, LPNMHDR pnm)
{
    switch(pnm->code) {
    case PGN_SCROLL:
        return TV_OnScroll(pTree, pnm);
        break;
    case PGN_CALCSIZE:
        return TV_OnCalcSize(pTree, pnm);
        break;
    }
    return 0L;
}

LRESULT TV_OnScroll(PTREE pTree, LPNMHDR pnm)
{
  
    LPNMPGSCROLL pscroll = (LPNMPGSCROLL)pnm;
    RECT rc = pscroll->rcParent;
    RECT rcTemp;
    int iDir = pscroll->iDir;
    int dyScroll = pscroll->iScroll;
    TREEITEM FAR * hItem;
    UINT uCode;
    int parentsize;
    TREEITEM FAR *  hPrevItem;
    TREEITEM FAR *  hNextItem;
    int y;
    
#ifndef UNIX
    POINT pt = {pscroll->iXpos, pscroll->iYpos};
    POINT ptTemp = pt;
    TREEITEM FAR *  hCurrentItem = TV_CheckHit(pTree, pt.x + 1, pt.y + 1 , &uCode);
#else
    POINT pt; 
    POINT ptTemp;
    TREEITEM FAR *  hCurrentItem;

    pt.x = pscroll->iXpos;
    pt.y = pscroll->iYpos;

    ptTemp = pt;
    hCurrentItem = TV_CheckHit(pTree, pt.x + 1, pt.y + 1 , &uCode);
#endif

    switch(iDir)
    {
        case PGF_SCROLLUP:
            //Check if any Item is partially visible at the left/top. if so then set the bottom 
            // of that Item to be our current offset and then scroll. This avoids skipping over
            // certain Items when partial Items are displayed at the left or top
            y = pt.y;       
            TV_GetItemRect(pTree,hCurrentItem,&rcTemp, TRUE);
 
            if (rcTemp.top  <  y-1)
            {
                hCurrentItem =TV_GetNextItem(pTree,hCurrentItem,TVGN_NEXTVISIBLE);
            }

            // Now do the calculation
            parentsize = RECTHEIGHT(rc);

            //if  the control key is down and we have more than parentsize size of child window
            // then scroll by that amount
            if ((pscroll->fwKeys & PGK_CONTROL) && ((pt.y - parentsize) > 0))
            {
                dyScroll = parentsize;
            } else if ((pt.y - pTree->cyItem) > 0) {
            // we dont have control key down so scroll by one buttonsize    
                dyScroll = pTree->cyItem;
            } else {
                pscroll->iScroll = pt.y;
                return 0L;
            }
            ptTemp.y -= dyScroll;
            hItem = TV_CheckHit(pTree, ptTemp.x, ptTemp.y, &uCode);

            if (hItem)
            {
                // if  the hit test gives us the same Item as our CurrentItem then set the Item 
                // to one Item to the top/left  of the  CurrentItem 

                hPrevItem = TV_GetNextItem(pTree,hCurrentItem, TVGN_PREVIOUSVISIBLE);
                if ((hItem == hCurrentItem) && ( hPrevItem != NULL))
                {
                    hItem = hPrevItem;
                }

                //When scrolling left if we end up in the middle of some Item then we align it to the 
                //right of that Item this is to avoid scrolling more than the pager window width but if the
                // Item happens to be the left Item of  our current Item then we end up in not scrolling
                //if thats the case then move one more Item to the left.


                if (hItem == hPrevItem) 
                {
                    hItem = TV_GetNextItem(pTree, hItem, TVGN_PREVIOUSVISIBLE);
                    if(!hItem)
                    {
                        dyScroll = pt.y;
                        break;
                    }
                }

                TV_GetItemRect(pTree,hItem,&rcTemp, TRUE);
                dyScroll = pt.y - rcTemp.bottom;
            }
            break;
        case PGF_SCROLLDOWN:
        {
            RECT rcChild;
            int childsize;

            GetWindowRect(pTree->ci.hwnd, &rcChild);
            childsize = RECTHEIGHT(rcChild);
            parentsize = RECTHEIGHT(rc);

            //if  the control key is down and we have more than parentsize size of child window
            // then scroll by that amount
            if ((pscroll->fwKeys & PGK_CONTROL) && ((childsize - pt.y - parentsize) > parentsize))
            {
                dyScroll = parentsize;
            } else if ( (childsize - pt.y - parentsize) > (pTree->cyItem * hCurrentItem->iIntegral) ) {
            // we dont have control key down so scroll by one buttonsize    
                dyScroll = pTree->cyItem * hCurrentItem->iIntegral;
            } else {
                pscroll->iScroll = childsize - pt.y - parentsize;
                return 0L;
            }
            ptTemp.y += dyScroll;

            hItem = TV_CheckHit(pTree, ptTemp.x, ptTemp.y, &uCode);

            if (hItem)
            {
                if ((hItem == hCurrentItem) && 
                    ((hNextItem = TV_GetNextItem(pTree,hItem,TVGN_NEXTVISIBLE)) != NULL))
                {
                    hItem = hNextItem;
                }
                TV_GetItemRect(pTree, hItem, &rcTemp, TRUE);
                dyScroll = rcTemp.top  - pt.y ;
            }

            break;
        }
    }
    //Set the scroll value
    pscroll->iScroll = dyScroll;
    return 0L;
 }
