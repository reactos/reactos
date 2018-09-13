#include "ctlspriv.h"
#include "listview.h"
#include "image.h"
#include <mlang.h>
#include <inetreg.h>

#define __IOleControl_INTERFACE_DEFINED__       // There is a conflict with the IOleControl's def of CONTROLINFO
#include "shlobj.h"

#define IE_SETTINGS          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced")
#define USE_DBL_CLICK_TIMER  TEXT("UseDoubleClickTimer")

int g_bUseDblClickTimer;

#define LVMP_WINDOWPOSCHANGED (WM_USER + 1)
HRESULT WINAPI UninitializeFlatSB(HWND hwnd);

#define COLORISLIGHT(clr) ((5*GetGValue((clr)) + 2*GetRValue((clr)) + GetBValue((clr))) > 8*128)

void NEAR ListView_HandleMouse(LV* plv, BOOL fDoubleClick, int x, int y, UINT keyFlags, BOOL bMouseWheel);

/// function table setup
const PFNLISTVIEW_DRAWITEM pfnListView_DrawItem[4] = {
    ListView_IDrawItem,
    ListView_RDrawItem,
    ListView_IDrawItem,
    ListView_LDrawItem,
};

void ListView_HandleStateIconClick(LV* plv, int iItem);

DWORD ListView_IApproximateViewRect(LV* ,int, int, int);
DWORD ListView_RApproximateViewRect(LV* ,int, int, int);
DWORD ListView_LApproximateViewRect(LV* ,int, int, int);

const PFNLISTVIEW_APPROXIMATEVIEWRECT pfnListView_ApproximateViewRect[4] = {
    ListView_IApproximateViewRect,
    ListView_RApproximateViewRect,
    ListView_IApproximateViewRect,
    ListView_LApproximateViewRect,
};

const PFNLISTVIEW_UPDATESCROLLBARS pfnListView_UpdateScrollBars[4] = {
    ListView_IUpdateScrollBars,
    ListView_RUpdateScrollBars,
    ListView_IUpdateScrollBars,
    ListView_LUpdateScrollBars,
};

const PFNLISTVIEW_ITEMHITTEST pfnListView_ItemHitTest[4] = {
    ListView_IItemHitTest,
    ListView_RItemHitTest,
    ListView_SItemHitTest,
    ListView_LItemHitTest,
};

const PFNLISTVIEW_ONSCROLL pfnListView_OnScroll[4] = {
    ListView_IOnScroll,
    ListView_ROnScroll,
    ListView_IOnScroll,
    ListView_LOnScroll,
};

const PFNLISTVIEW_SCROLL2 pfnListView_Scroll2[4] = {
    ListView_IScroll2,
    ListView_RScroll2,
    ListView_IScroll2,
    ListView_LScroll2,
};

const PFNLISTVIEW_GETSCROLLUNITSPERLINE pfnListView_GetScrollUnitsPerLine[4] = {
    ListView_IGetScrollUnitsPerLine,
    ListView_RGetScrollUnitsPerLine,
    ListView_IGetScrollUnitsPerLine,
    ListView_LGetScrollUnitsPerLine,
};


// redefine to trace at most calls to ListView_SendChange
#define DM_LVSENDCHANGE 0


// BUGBUG -- penwin.h is screwy; define local stuff for now
#define HN_BEGINDIALOG        40    // Lens/EditText/garbage detection dialog is about
                                    // to come up on this hedit/bedit
#define HN_ENDDIALOG          41    // Lens/EditText/garbage detection dialog has
                                    // just been destroyed

//---------------------------------------------------------
// no way am I gonna make TWO function calls where I can do FOUR comparisons!
//
#define RECTS_IN_SIZE( sz, r2 ) (!RECTS_NOT_IN_SIZE( sz, r2 ))

#define RECTS_NOT_IN_SIZE( sz, r2 ) (\
   ( (sz).cx <= (r2).left ) ||\
   ( 0 >= (r2).right ) ||\
   ( (sz).cy <= (r2).top ) ||\
   ( 0 >= (r2).bottom ) )

//---------------------------------------------------------


void NEAR ListView_OnUpdate(LV* plv, int i);
void NEAR ListView_OnDestroy(LV* plv);
BOOL NEAR PASCAL ListView_ValidateScrollParams(LV* plv, int FAR * dx, int FAR *dy);
UINT LV_IsItemOnViewEdge(LV* plv, LISTITEM *pitem);
void PASCAL ListView_ButtonSelect(LV* plv, int iItem, UINT keyFlags, BOOL bSelected);
void NEAR ListView_DeselectAll(LV* plv, int iDontDeselect);
void ListView_LRInvalidateBelow(LV* plv, int i, int fSmoothScroll);
void ListView_IInvalidateBelow(LV* plv, int i);
void NEAR ListView_InvalidateFoldedItem(LV* plv, int iItem, BOOL fSelectionOnly, UINT fRedraw);
void ListView_ReleaseBkImage(LV *plv);
void ListView_RecalcRegion(LV *plv, BOOL fForce, BOOL fRedraw);

BOOL g_fSlowMachine = -1;

#pragma code_seg(CODESEG_INIT)

BOOL FAR ListView_Init(HINSTANCE hinst)
{
    WNDCLASS wc;

    if (!GetClassInfo(hinst, c_szListViewClass, &wc)) {
#ifndef WIN32
        LRESULT CALLBACK _ListView_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        wc.lpfnWndProc     = _ListView_WndProc;
#else
        wc.lpfnWndProc     = ListView_WndProc;
#endif
        wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
        wc.hIcon           = NULL;
        wc.lpszMenuName    = NULL;
        wc.hInstance       = hinst;
        wc.lpszClassName   = c_szListViewClass;
        wc.hbrBackground   = (HBRUSH)(COLOR_WINDOW + 1); // NULL;
        wc.style           = CS_DBLCLKS | CS_GLOBALCLASS;
        wc.cbWndExtra      = sizeof(LV*);
        wc.cbClsExtra      = 0;

        return RegisterClass(&wc);
    }
    return TRUE;
}
#pragma code_seg()


BOOL NEAR ListView_GetRegIASetting(BOOL *pb)
{
    HKEY        hkey;
    BOOL        bRet = FALSE;
    BOOL        bValue = TRUE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, IE_SETTINGS, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        DWORD dwType;
        DWORD dwValue;
        DWORD cbValue = sizeof(DWORD);

        if (RegQueryValueEx(hkey, (LPTSTR)USE_DBL_CLICK_TIMER, 0, &dwType, (LPBYTE)&dwValue, &cbValue) == ERROR_SUCCESS)
        {
            bValue = (BOOL)dwValue;
            bRet = TRUE;
        }
        RegCloseKey(hkey);
    }

    *pb = bValue;
    return bRet;
}


BOOL NEAR ListView_NotifyCacheHint(LV* plv, int iFrom, int iTo)
{
    NM_CACHEHINT nm;

    ASSERT( iFrom <= iTo );
    if (iFrom <= iTo)
    {
        nm.iFrom = iFrom;
        nm.iTo = iTo;
        return !(BOOL)CCSendNotify(&plv->ci, LVN_ODCACHEHINT, &nm.hdr);
    }
    return FALSE;
}

void NEAR ListView_LazyCreateObjects(LV *plv, int iMin, int iMax)
{
    for ( ; iMin < iMax; iMin++)
        MyNotifyWinEvent(EVENT_OBJECT_CREATE, plv->ci.hwnd, OBJID_CLIENT, 1 + iMin);
}

//
//  Owner-data causes MSAA lots of grief, because there is no way to tell
//  MSAA "I just created 25 million items".  You have to tell it one at a
//  time.  Instead of sending out 25 million "add item" notifications, we
//  just send them out as they scroll into view.
//
//  plv->iMSAAMin and plv->iMSAAMax are the range of items we most
//  recently told MSAA about.  MSAAMax is *exclusive*, just like RECTs.
//  It makes the math easier.
//
//  We use iMSAAMin and iMSAAMax to avoid sending blatantly redundant
//  notifications, which would other happen very frequently.
//
void NEAR ListView_LazyCreateWinEvents(LV *plv, int iFrom, int iTo)
{
    int iMin = iFrom;
    int iMax = iTo+1;           // Convert from [From,To] to [Min,Max)

#ifdef LVDEBUG
    DebugMsg(TF_LISTVIEW, TEXT("lv.CWE old: [%d,%d), new=[%d,%d)"),
            plv->iMSAAMin, plv->iMSAAMax,
            iMin, iMax);
#endif

    //
    //  If the incoming range is entirely contained within the existing
    //  range, then there is nothing to do.  This happens a lot.
    //
    if (iMin >= plv->iMSAAMin && iMax <= plv->iMSAAMax)
        return;

    //
    //  If the incoming range is adjacent to or overlaps the low end
    //  of the existing range...  (This happens when scrolling backwards.)
    //
    if (iMin <= plv->iMSAAMin && iMax >= plv->iMSAAMin) {

        // Notify the low end.
        ListView_LazyCreateObjects(plv, iMin, plv->iMSAAMin);

        // Extend the list of things we've notified.
        plv->iMSAAMin = iMin;

        // Remove it from the things left to be notified.
        iMin = plv->iMSAAMax;
    }

    //
    //  Now do the same thing to the top end.
    //  (This happens when scrolling forwards.)
    //
    if (iMax >= plv->iMSAAMax && iMin <= plv->iMSAAMax) {

        // Notify the top end.
        ListView_LazyCreateObjects(plv, plv->iMSAAMax, iMax);

        // Extend the list of things we've notified.
        plv->iMSAAMax = iMax;

        // Remove it from the things left to be notified.
        iMax = plv->iMSAAMin;
    }

    //
    //  If there are still things to be notified, then it means that the
    //  incoming range isn't contiguous with the previous range, so throw
    //  away the old range and just set it to the current range.
    //  (This happens when you grab the scrollbar and jump to a completely
    //  unrelated part of the listview.)
    //
    if (iMin < iMax) {
        plv->iMSAAMin = iMin;
        plv->iMSAAMax = iMax;
        ListView_LazyCreateObjects(plv, iMin, iMax);
    }

#ifdef LVDEBUG
    DebugMsg(TF_LISTVIEW, TEXT("lv.CWE aft: [%d,%d)"), plv->iMSAAMin, plv->iMSAAMax);
#endif

}

LRESULT NEAR ListView_RequestFindItem(LV* plv, CONST LV_FINDINFO* plvfi, int iStart)
{
    NM_FINDITEM nm;

    nm.lvfi = *plvfi;
    nm.iStart = iStart;
    return CCSendNotify(&plv->ci, LVN_ODFINDITEM, &nm.hdr);
}

BOOL NEAR ListView_SendChange(LV* plv, int i, int iSubItem, int code, UINT oldState, UINT newState,
                              UINT changed, LPARAM lParam)
{
    NM_LISTVIEW nm;

    nm.iItem = i;
    nm.iSubItem = iSubItem;
    nm.uNewState = newState;
    nm.uOldState = oldState;
    nm.uChanged = changed;
    nm.ptAction.x = 0;
    nm.ptAction.y = 0;
    nm.lParam = lParam;

    return !CCSendNotify(&plv->ci, code, &nm.hdr);
}

void NEAR ListView_SendODChangeAndInvalidate(LV* plv, int iFrom, int iTo, UINT oldState,
                                UINT newState)
{
    NM_ODSTATECHANGE nm;

    nm.iFrom = iFrom;
    nm.iTo = iTo;
    nm.uNewState = newState;
    nm.uOldState = oldState;

    CCSendNotify(&plv->ci, LVN_ODSTATECHANGED, &nm.hdr);

    // Tell accessibility, "Selection changed in a complex way"
    MyNotifyWinEvent(EVENT_OBJECT_SELECTIONWITHIN, plv->ci.hwnd, OBJID_CLIENT, CHILDID_SELF);

    // considerable speed increase less than 100 to do this method
    // while over 100, the other method works faster
    if ((iTo - iFrom) > 100)
    {
        InvalidateRect( plv->ci.hwnd, NULL, FALSE );
    }
    else
    {
        while (iFrom <= iTo)
        {
            ListView_InvalidateItem(plv, iFrom, TRUE, RDW_INVALIDATE);
            iFrom++;
        }
    }
}

BOOL NEAR ListView_Notify(LV* plv, int i, int iSubItem, int code)
{
    NM_LISTVIEW nm;
    nm.iItem = i;
    nm.iSubItem = iSubItem;
    nm.uNewState = nm.uOldState = 0;
    nm.uChanged = 0;
    nm.lParam = 0;

   if (!ListView_IsOwnerData( plv )) {
       if (code == LVN_DELETEITEM) {
           LISTITEM FAR * pItem = ListView_GetItemPtr(plv, i);
           if (pItem) {
               nm.lParam = pItem->lParam;
           }
       }
   }

    return (BOOL)CCSendNotify(&plv->ci, code, &nm.hdr);
}

BOOL NEAR ListView_GetEmptyText(LV* plv)
{
    NMLVDISPINFO nm;
    BOOL ret;
    TCHAR szText[80];

    if (plv->fNoEmptyText)
        return FALSE;

    if (plv->pszEmptyText)
        return TRUE;

    // For each listview control, we will only send this notify
    // once if necessary.

    memset(&nm, 0, SIZEOF(NMLVDISPINFO));
    nm.item.mask = LVIF_TEXT;
    nm.item.cchTextMax = ARRAYSIZE(szText);
    nm.item.pszText = szText;
    szText[0] = TEXT('\0');

    ret = (BOOL)CCSendNotify(&plv->ci, LVN_GETEMPTYTEXT, &nm.hdr);

    if (ret)
        // save the text so we don't notify again.
        Str_Set(&plv->pszEmptyText, szText);
    else
        // set a flag so we don't notify again.
        plv->fNoEmptyText = TRUE;

    return ret;
}

void NEAR ListView_NotifyFocusEvent(LV *plv)
{
    if (plv->iFocus != -1 && IsWindowVisible(plv->ci.hwnd) && GetFocus() == plv->ci.hwnd)
        MyNotifyWinEvent(EVENT_OBJECT_FOCUS, plv->ci.hwnd, OBJID_CLIENT,
                plv->iFocus+1);
}

//
//  Call this function when the listview has changed in a radical manner.
//  It notifies MSAA that "Whoa, things are completely different now."
//
void NEAR ListView_NotifyRecreate(LV *plv)
{
    MyNotifyWinEvent(EVENT_OBJECT_DESTROY, plv->ci.hwnd, OBJID_CLIENT, CHILDID_SELF);
    MyNotifyWinEvent(EVENT_OBJECT_CREATE, plv->ci.hwnd, OBJID_CLIENT, CHILDID_SELF);
    plv->iMSAAMin = plv->iMSAAMax = 0;
}

int NEAR ListView_OnSetItemCount(LV *plv, int iItems, DWORD dwFlags)
{
   BOOL frt = TRUE;

   // For compatability we assume 0 for flags implies old (Athena) type of functionality and
   // does a Invalidate all otherwise if low bit is set we try to be a bit smarter.  First pass
   // If the first added item is visible invalidate all.  Yes we can do better...
   if (ListView_IsOwnerData( plv )) {
       int iItem;
       int cTotalItemsOld = plv->cTotalItems;
       BOOL fInvalidateAll = ((dwFlags & LVSICF_NOINVALIDATEALL) == 0);

       if ((iItems >= 0) && (iItems <= MAX_LISTVIEWITEMS)) {

           plv->cTotalItems = iItems;

           // check focus
           if (plv->iFocus >= iItems)
              plv->iFocus = -1;
          if (plv->iDropHilite >= iItems)
              plv->iDropHilite = -1;

           // check mark
           if (plv->iMark >= iItems)
              plv->iMark = -1;

           // make sure no selections above number of items
           plv->plvrangeCut->lpVtbl->ExcludeRange(plv->plvrangeCut, iItems, SELRANGE_MAXVALUE );
           if (FAILED(plv->plvrangeSel->lpVtbl->ExcludeRange(plv->plvrangeSel, iItems, SELRANGE_MAXVALUE ))) {
               //BUGBUG:  Return low memory status
               //MemoryLowDlg( plv->ci.hwnd );
               return FALSE;
           }


           plv->rcView.left = RECOMPUTE;  // recompute view rect

           if ( ListView_IsSmallView(plv) || ListView_IsIconView(plv) ) {
               // Call off to the arrange function.
               ListView_OnArrange(plv, LVA_DEFAULT);

               if (!fInvalidateAll)
               {
                   // Try to be smart and invalidate only what we need to.
                   // Add a little logic to erase any message like no items found when
                   // the view was previously empty...
                   if (cTotalItemsOld < iItems)
                       iItem = cTotalItemsOld;
                   else
                       iItem = iItems - 1;  // Get the index

                   if ((iItem >= 0) && (cTotalItemsOld > 0))
                       ListView_IInvalidateBelow(plv, iItem);
                   else
                       fInvalidateAll = TRUE;
               }

           } else {
               ListView_Recompute(plv);
               // if we have empty text and old count was zero... then we should redraw all
               if (plv->pszEmptyText && (cTotalItemsOld == 0) && (iItems > 0))
                   fInvalidateAll = TRUE;

               // Try to do smart invalidates...
               if (!fInvalidateAll)
               {
                   // Try to be smart and invalidate only what we need to.
                   if (cTotalItemsOld < iItems)
                       iItem = cTotalItemsOld;
                   else
                       iItem = iItems - 1;  // Get the index

                   if (iItem >= 0)
                       ListView_LRInvalidateBelow(plv, iItem, FALSE);
               }


               // We may try to resize the column
               ListView_MaybeResizeListColumns(plv, 0, ListView_Count(plv)-1);

               // For compatability we assume 0 for flags implies old type
               // of functionality and scrolls the important item into view.
               // If second bit is set, we leave the scroll position alone.
               if ((dwFlags & LVSICF_NOSCROLL) == 0) {
                   // what is the important item
                   iItem = (plv->iFocus >= 0) ?
                           plv->iFocus :
                           ListView_OnGetNextItem(plv, -1, LVNI_SELECTED);

                   iItem = max(0, iItem);

                   // make important item visable
                   ListView_OnEnsureVisible(plv, iItem, FALSE);
               }
           }


           if (fInvalidateAll)
               InvalidateRect(plv->ci.hwnd, NULL, TRUE);
           ListView_UpdateScrollBars(plv);

           ListView_NotifyRecreate(plv);
           ListView_NotifyFocusEvent(plv);

       } else {
           frt = FALSE;
       }

   } else {
       if (plv->hdpaSubItems)
       {
           int iCol;
           for (iCol = plv->cCol - 1; iCol >= 0; iCol--)
           {
               HDPA hdpa = ListView_GetSubItemDPA(plv, iCol);
               if (hdpa)   // this is optional, call backs don't have them
                   DPA_Grow(hdpa, iItems);
           }
       }

       DPA_Grow(plv->hdpa, iItems);
       DPA_Grow(plv->hdpaZOrder, iItems);
    }

    return frt;
}

typedef struct _LVSortInfo
{
    LV*     plv;
    BOOL fSortIndices;
    PFNLVCOMPARE        pfnCompare;
    LPARAM              lParam;
    BOOL                bPassLP;
} LVSortInfo;

int CALLBACK ListView_SortCallback(LPVOID dw1, LPVOID dw2, LPARAM lParam)
{
    LISTITEM FAR *pitem1;
    LISTITEM FAR *pitem2;
    LVSortInfo FAR *pSortInfo = (LVSortInfo FAR *)lParam;

    ASSERT(!ListView_IsOwnerData(pSortInfo->plv));

    // determine whether  dw1 and dw2 are indices or the real items
    // and assign pitem? accordingly
    if (pSortInfo->fSortIndices) {
        pitem1 = ListView_GetItemPtr(pSortInfo->plv, PtrToUlong(dw1));
        pitem2 = ListView_GetItemPtr(pSortInfo->plv, PtrToUlong(dw2));
    } else {
        pitem1 = (LISTITEM FAR *)dw1;
        pitem2 = (LISTITEM FAR *)dw2;
    }

    if (!pSortInfo->pfnCompare) {
        // Treat NULL pszText like null string.
        LPCTSTR pszText1 = pitem1->pszText ? pitem1->pszText : c_szNULL;
        LPCTSTR pszText2 = pitem2->pszText ? pitem2->pszText : c_szNULL;

        // bugbug, should allow callbacks in text
        if (pszText1 != LPSTR_TEXTCALLBACK &&
            pszText2 != LPSTR_TEXTCALLBACK )
        {
            return lstrcmpi(pitem1->pszText, pitem2->pszText);
        }
        RIPMSG(0, "LVM_SORTITEM(EX): Cannot combine NULL callback with LPSTR_TEXTCALLBACK");
        return -1;
    } else
    {
        if (pSortInfo->bPassLP)
            return(pSortInfo->pfnCompare(pitem1->lParam, pitem2->lParam, pSortInfo->lParam));
        else 
        {
            if (pSortInfo->fSortIndices)
                return(pSortInfo->pfnCompare((LPARAM)dw1, (LPARAM)dw2, pSortInfo->lParam));
            else
            {
                // we want to sort by the indices, but all we've got are pointers to the items
                // and there is no way to get back from that pointer to an index
                ASSERT(0);
                return -1;
            }
        }

    }
    ASSERT(0);
    return -1;
}

VOID ListView_InvalidateTTLastHit(LV* plv, int iNewHit)
{
    if (plv->iTTLastHit == iNewHit)
    {
        plv->iTTLastHit = -1;
        if (plv->pszTip && plv->pszTip != LPSTR_TEXTCALLBACK)
        {
            plv->pszTip[0] = 0;
        }
    }
}

BOOL NEAR PASCAL ListView_SortAllColumns(LV* plv, LVSortInfo FAR * psi)
{
    ASSERT(!ListView_IsOwnerData(plv));

    ListView_InvalidateTTLastHit(plv, plv->iTTLastHit);

    // don't do this optimization if we will need the indices to sort by
    if (psi->bPassLP && ((!plv->hdpaSubItems) || !DPA_GetPtrCount(plv->hdpaSubItems))) {
        psi->fSortIndices = FALSE;
        return (DPA_Sort(plv->hdpa, ListView_SortCallback, (LPARAM)psi));
    } else {
        // if we need to sort several hdpa's, create one DPA of just indices
        // and sort that, then fix up all the dpa's
        BOOL fReturn = FALSE;
        HDPA hdpa;
        int i;
        int iMax;
        void FAR * FAR * ph;
        void FAR * FAR *pNewIndices;

        // initialize the hdpa with indices
        hdpa = DPA_Clone(plv->hdpa, NULL);
        if (hdpa) {
            ASSERT(DPA_GetPtrCount(plv->hdpa) == DPA_GetPtrCount(hdpa));
            ph = pNewIndices = DPA_GetPtrPtr(hdpa);
            iMax = DPA_GetPtrCount(hdpa);
            for( i = 0; i < iMax; ph++, i++) {
                *ph = (LPVOID)(HANDLE)i;
            }

            psi->fSortIndices = TRUE;
            if (DPA_Sort(hdpa, ListView_SortCallback, (LPARAM)psi)) {
#ifdef WIN32
                ph = LocalAlloc(LPTR, sizeof(LPVOID) * iMax);
#else
                ph = Alloc(sizeof(LPVOID) * iMax);
#endif

                if (ph) {
                    int j;
                    void FAR * FAR *pSubItems;

                    // we could get here because bPassLP is false, even if we don't have subitems
                    if (plv->hdpaSubItems && DPA_GetPtrCount(plv->hdpaSubItems))
                    {
                        for (i = DPA_GetPtrCount(plv->hdpaSubItems) - 1; i >= 0; i--) {
                            HDPA hdpaSubItem = ListView_GetSubItemDPA(plv, i);

                            if (hdpaSubItem) {

                                // make sure it's of the right size
                                while (DPA_GetPtrCount(hdpaSubItem) < iMax) {
                                    if (DPA_InsertPtr(hdpaSubItem, iMax, NULL) == -1)
                                        goto Bail;
                                }


                                // actually copy across the dpa with the new indices
                                pSubItems = DPA_GetPtrPtr(hdpaSubItem);
                                for (j = 0; j < iMax; j++) {
                                    ph[j] = pSubItems[PtrToUlong(pNewIndices[j])];
                                }

                                // finally, copy it all back to the pSubItems;
                                hmemcpy(pSubItems, ph, sizeof(LPVOID) * iMax);
                            }
                        }
                    }

                    // now do the main hdpa
                    pSubItems = DPA_GetPtrPtr(plv->hdpa);
                    for (j = 0; j < iMax; j++) {
                        ph[j] = pSubItems[PtrToUlong(pNewIndices[j])];
                    }

                    // finally, copy it all back to the pSubItems;
                    hmemcpy(pSubItems, ph, sizeof(LPVOID) * iMax);
                    fReturn = TRUE;
Bail:
#ifdef WIN32
                    LocalFree(ph);
#else
                    Free(ph);
#endif

                }
            }
            DPA_Destroy(hdpa);
        }
        return fReturn;

    }
}

DWORD ListView_OnApproximateViewRect(LV* plv, int iCount, int iWidth, int iHeight)
{
    if (iCount == -1)
        iCount = ListView_Count(plv);

    if (iWidth == -1)
        iWidth = plv->sizeClient.cx;

    if (iHeight == -1)
        iHeight = plv->sizeClient.cy;

    return _ListView_ApproximateViewRect(plv, iCount, iWidth, iHeight);
}

DWORD ListView_OnSetLVRangeObject(LV* plv, int iWhich, ILVRange *plvrange)
{
    ILVRange **pplvrange;
    switch (iWhich)
    {
    case LVSR_SELECTION:
        pplvrange = &plv->plvrangeSel;
        break;
    case LVSR_CUT:
        pplvrange = &plv->plvrangeCut;
        break;
    default:
        return FALSE;
    }
    if (*pplvrange)
    {
        // Release the old one
        (*pplvrange)->lpVtbl->Release(*pplvrange);
    }
    *pplvrange = plvrange;

    // Hold onto the pointer...
    if (plvrange)
        plvrange->lpVtbl->AddRef(plvrange);

    return TRUE;
}


BOOL NEAR PASCAL ListView_OnSortItems(LV *plv, LPARAM lParam, PFNLVCOMPARE pfnCompare, BOOL bPassLP)
{
    LVSortInfo SortInfo;
    LISTITEM FAR *pitemFocused;
    SortInfo.pfnCompare = pfnCompare;
    SortInfo.lParam     = lParam;
    SortInfo.plv = plv;
    SortInfo.bPassLP = bPassLP;

   if (ListView_IsOwnerData(plv)) {
      RIPMSG(0, "LVM_SORTITEMS: Invalid for owner-data listview");
      return FALSE;
   }

    ListView_DismissEdit(plv, TRUE);    // cancel edits

    // we're going to screw with the indices, so stash away the pointer to the
    // focused item.
    if (plv->iFocus != -1) {
        pitemFocused = ListView_GetItemPtr(plv, plv->iFocus);
    } else
        pitemFocused = NULL;

    if (ListView_SortAllColumns(plv, &SortInfo)) {

        // restore the focused item.
        if (pitemFocused) {
            int i;
            for (i = ListView_Count(plv) - 1; i >= 0 ; i--) {
                if (ListView_GetItemPtr(plv, i) == pitemFocused) {
                    plv->iFocus = i;
                    plv->iMark = i;
                }
            }
        }

        if (ListView_IsSmallView(plv) || ListView_IsIconView(plv))
        {
            ListView_CommonArrange(plv, LVA_DEFAULT, plv->hdpa);
        }
        else if (ListView_IsReportView(plv) || ListView_IsListView(plv))
        {
            InvalidateRect(plv->ci.hwnd, NULL, TRUE);
        }

        // The items in the view have moved around; let apps know
        MyNotifyWinEvent(EVENT_OBJECT_REORDER, plv->ci.hwnd, OBJID_CLIENT, 0);
        return(TRUE);
    }
    return FALSE;
}


void PASCAL ListView_EnableWindow(LV* plv, BOOL wParam)
{
    if (wParam) {
        if (plv->ci.style & WS_DISABLED) {
            plv->ci.style &= ~WS_DISABLED;      // enabled
            ListView_OnSetBkColor(plv, plv->clrBkSave);
        }
    } else {
        if (!(plv->ci.style & WS_DISABLED)) {
            plv->clrBkSave = plv->clrBk;
            plv->ci.style |= WS_DISABLED;       // disabled
            ListView_OnSetBkColor(plv, g_clrBtnFace);
        }
    }
    RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}


BOOL NEAR ListView_IsItemVisible(LV* plv, int i)
// Assumes parmss ok etc for speed. Called inside region calc code.
{
    RECT rcBounds;
    RECT rc, rcScratch;

    // get bounding rect of item
    ListView_GetRects(plv, i, NULL, NULL, &rcBounds, NULL);

    // setup rect for listview client. Should perf this up for
    // multimonitor case where there are dead zones in work area...
    rc.left  = 0;
    rc.top   = 0;
    rc.right = plv->sizeClient.cx;
    rc.bottom= plv->sizeClient.cy;

    return IntersectRect(&rcScratch, &rc, &rcBounds);
}



// Helper for ListView_RecalcRegion
#define BitOn(lpbits, x, y, cx) (*((BYTE *)(lpbits + ((y * cx) + (x / 8)))) & (0x80 >> (x % 8)))

void ListView_RecalcRegion(LV* plv, BOOL fForce, BOOL fRedraw)
{
    HRGN hrgnUnion = NULL;
    HRGN hrgn = NULL;
    int i;
    HDC hdc = NULL;
    BYTE * lpBits = NULL;
    HBITMAP hbmp = NULL, hbmpOld = NULL;
    RECT rc, rcIcon;
    LISTITEM FAR * pitem;
    BITMAP bm;

    // Bail out if we don't need to do any work
    if (!(plv->exStyle & LVS_EX_REGIONAL) || !ListView_RedrawEnabled(plv) ||
        (plv->flags & LVF_INRECALCREGION))
        return;

    // To prevent recursion
    plv->flags |= LVF_INRECALCREGION;

    if ((ListView_Count(plv) > 0)) {
        int cxIcon, cyIcon;
        int dxOffset, dyOffset;

        // Run through first to see if anything changed - bail if not!
        if (!fForce) {
            for (i = 0; i < ListView_Count(plv); i++) {
                pitem = ListView_FastGetItemPtr(plv, i);

                if (!ListView_IsItemVisible(plv, i))
                {
                    if (pitem->hrgnIcon == (HANDLE)-1 || !pitem->hrgnIcon)
                        // Item was invisible and still is. Nothing changed.
                        continue;

                    if (pitem->hrgnIcon)
                    {
                        // Item was visible and now is invisible... Something
                        // changed.
                        pitem->ptRgn.x = RECOMPUTE;
                        pitem->ptRgn.y = RECOMPUTE;
                        DeleteObject(pitem->hrgnIcon);
                        pitem->hrgnIcon = NULL;
                    }
                }

                ListView_GetRects(plv, i, NULL, &rc, NULL, NULL);

                // If the location of the icon or the text rectangle have
                // changed, then we need to continue so that we can recalculate
                // the region.
                if ((pitem->pt.x != pitem->ptRgn.x) ||
                    (pitem->pt.y != pitem->ptRgn.y) ||
                    (!pitem->hrgnIcon) ||
                    !EqualRect((CONST RECT *)&pitem->rcTextRgn, (CONST RECT *)&rc))
                    goto changed;

            }
            // If we go through all the items and nothing changed, then
            // we can return without doing any work!
            ASSERT(i == ListView_Count(plv));
            goto exit;
changed:;
        }

        // Figure out the dimensions of the Icon rectangle - assumes
        // each Icon rectangle is the same size.
        ListView_GetRects(plv, 0, &rcIcon, NULL, NULL, NULL);

        // Center the icon in the rectangle
        ImageList_GetIconSize(plv->himl, &cxIcon, &cyIcon);
        dxOffset = (rcIcon.right - rcIcon.left - cxIcon) / 2;
        dyOffset = (rcIcon.bottom - rcIcon.top - cyIcon) / 2;
        cxIcon = rcIcon.right - rcIcon.left;
        cyIcon = rcIcon.bottom - rcIcon.top;

        if (!(hdc = CreateCompatibleDC(NULL)) ||
            (!(hbmp = CreateBitmap(cxIcon, cyIcon, 1, 1, NULL)))) {
            goto BailOut;
        }

        GetObject(hbmp, SIZEOF(bm), &bm);

        if (!(lpBits = (BYTE *)GlobalAlloc(GPTR, bm.bmWidthBytes * bm.bmHeight)))
            goto BailOut;

        hbmpOld = SelectObject(hdc, hbmp);
        PatBlt(hdc, 0, 0, cxIcon, cyIcon, WHITENESS);

        if (hrgnUnion = CreateRectRgn(0, 0, 0, 0)) {
            for (i = 0; i < ListView_Count(plv); i++)
            {
                int x, y, iResult;
                BOOL fStarted = FALSE;
                LPRECT lprc;

                pitem = ListView_FastGetItemPtr(plv, i);

                if (pitem->pt.y == RECOMPUTE)
                    continue;

                if (!ListView_IsItemVisible(plv, i))
                {
                    // ignore invisible items
                    if (pitem->hrgnIcon && pitem->hrgnIcon!=(HANDLE)-1)
                    {
                        pitem->ptRgn.x = RECOMPUTE;
                        pitem->ptRgn.y = RECOMPUTE;
                        DeleteObject(pitem->hrgnIcon);
                        pitem->hrgnIcon = (HANDLE)-1;
                    }
                    continue;
                }

                // Add the region for the icon text first
                ListView_GetRects(plv, i, &rcIcon, &rc, NULL, NULL);

                // If we're in edit mode always use rcTextRgn
                if (i == plv->iEdit)
                    lprc = &pitem->rcTextRgn;
                else
                    lprc = &rc;

                if (!(hrgn = CreateRectRgnIndirect(lprc)))
                    goto Error;

                iResult = CombineRgn(hrgnUnion, hrgn, hrgnUnion, RGN_OR);

                DeleteObject(hrgn);

                if (iResult == ERROR) {
                    // Error case - out of memory.  Just select in a NULL region.
Error:
                    DeleteObject(hrgnUnion);
                    hrgnUnion = NULL;
                    break;
                }

                // Succeeded, copy the rectangle to rcTextRgn so we
                // can test against it in the future.  Don't copy over
                // it if we are in edit mode, the rectangle is used to
                // store the edit window in that case.
                if (plv->iEdit != i)
                    CopyRect(&pitem->rcTextRgn, (CONST RECT *)&rc);

                // Now create a region for the icon mask - or use the cached one
                if (!pitem->hrgnIcon || pitem->hrgnIcon == (HANDLE)-1) {
//                    (pitem->pt.x != pitem->ptRgn.x) ||
//                    (pitem->pt.y != pitem->ptRgn.y)) {
                    HRGN hrgnIcon = NULL;

                    // On slow machines, we'll just wrap the icon with a rectangle.  But on
                    // faster machines, we'll build a region that corresponds to the
                    // mask for the icon so it looks sweet.
                    if (g_fSlowMachine) {
                        // Modify the rectangle slightly so it looks better

                        // Glue the icon and text rectangles together
                        rcIcon.bottom = rc.top;
                        // Shrink the width of the rectangle so it's only as big as the icon itself
                        InflateRect(&rcIcon, -dxOffset, 0);
                        hrgnIcon = CreateRectRgnIndirect(&rcIcon);
                    } else {
                        // If the image isn't around, get it now.
                        if (pitem->iImage == I_IMAGECALLBACK) {
                            LV_ITEM item;

                            item.iItem = i;
                            item.iSubItem = 0;
                            item.mask = LVIF_IMAGE;
                            item.stateMask = LVIS_ALL;
                            item.pszText = NULL;
                            item.cchTextMax = 0;
                            // BOGUS - do we need to worry about our state
                            // getting messed up during the callback?
                            ListView_OnGetItem(plv, &item);
                        }

                        ImageList_Draw(plv->himl, pitem->iImage, hdc, 0, 0, ILD_MASK | (pitem->state & LVIS_OVERLAYMASK));

                        GetBitmapBits(hbmp, bm.bmWidthBytes * bm.bmHeight, (LPVOID)lpBits);

                        for (y = 0; y < cyIcon; y++) {
                            for (x = 0; x < cxIcon; x++) {
                                if (!fStarted && !BitOn(lpBits, x, y, bm.bmWidthBytes)) {
                                    rc.left = x;
                                    rc.top = y;
                                    rc.bottom = y + 1;
                                    fStarted = TRUE;
                                    if (x == (cxIcon - 1)) {
                                        x++;
                                        goto AddIt;
                                    } else {
                                        continue;
                                    }
                                }

                                if (fStarted && BitOn(lpBits, x, y, bm.bmWidthBytes)) {
AddIt:
                                    rc.right = x;
                                    //
                                    // Mirror the region so that the icons get displayed ok. [samera]
                                    //
                                    if (plv->ci.dwExStyle & RTL_MIRRORED_WINDOW)
                                    {
                                        int iLeft = rc.left;
                                        rc.left = (cxIcon - (rc.right+1));
                                        rc.right = (cxIcon - (iLeft+1));
                                        OffsetRect(&rc, rcIcon.left - dxOffset, rcIcon.top + dyOffset);
                                    }
                                    else
                                        OffsetRect(&rc, rcIcon.left + dxOffset, rcIcon.top + dyOffset);


                                    if (hrgn = CreateRectRgnIndirect(&rc)) {
                                        if (hrgnIcon || (hrgnIcon = CreateRectRgn(0, 0, 0, 0)))
                                            iResult = CombineRgn(hrgnIcon, hrgn, hrgnIcon, RGN_OR);
                                        else
                                            iResult = ERROR;

                                        DeleteObject(hrgn);
                                    }

                                    if (!hrgn || (iResult == ERROR)) {
                                        if (hrgnIcon)
                                            DeleteObject(hrgnIcon);
                                        goto Error;
                                    }

                                    fStarted = FALSE;
                                }
                            }
                        }
                    }

                    if (hrgnIcon) {
                        // Cache it since it takes a long time to build it
                        if (pitem->hrgnIcon && pitem->hrgnIcon != (HANDLE)-1)
                            DeleteObject(pitem->hrgnIcon);
                        pitem->hrgnIcon = hrgnIcon;
                        pitem->ptRgn = pitem->pt;

                        // Add it to the accumulated window region
                        if (ERROR == CombineRgn(hrgnUnion, hrgnIcon, hrgnUnion, RGN_OR))
                            goto Error;
                    }
                } else {
                    OffsetRgn(pitem->hrgnIcon, pitem->pt.x - pitem->ptRgn.x, pitem->pt.y - pitem->ptRgn.y);
                    pitem->ptRgn = pitem->pt;
                    if (ERROR == CombineRgn(hrgnUnion, pitem->hrgnIcon, hrgnUnion, RGN_OR))
                        goto Error;
                }
            }
        }
    }

BailOut:
    if (lpBits)
        GlobalFree((HGLOBAL)lpBits);
    if (hbmp) {
        SelectObject(hdc, hbmpOld);
        DeleteObject(hbmp);
    }
    if (hdc)
        DeleteDC(hdc);

    // Windows takes ownership of the region when we select it in to the window
    SetWindowRgn(plv->ci.hwnd, hrgnUnion, fRedraw);

exit:
    plv->flags &= ~LVF_INRECALCREGION;
}

HIMAGELIST CreateCheckBoxImagelist(HIMAGELIST himl, BOOL fTree, BOOL fUseColorKey, BOOL fMirror)
{
    int cxImage, cyImage;
    HBITMAP hbm;
    HBITMAP hbmTemp;
    COLORREF clrMask;
    HDC hdcDesk = GetDC(NULL);
    HDC hdc;
    RECT rc;
    int nImages = fTree ? 3 : 2;

    if (!hdcDesk)
        return NULL;

    hdc = CreateCompatibleDC(hdcDesk);
    ReleaseDC(NULL, hdcDesk);

    if (!hdc)
        return NULL;

    // Must protect against ImageList_GetIconSize failing in case app
    // gave us a bad himl
    if (himl && ImageList_GetIconSize(himl, &cxImage, &cyImage)) {
        // cxImage and cyImage are okay
    } else {
        cxImage = g_cxSmIcon;
        cyImage = g_cySmIcon;
    }

    himl = ImageList_Create(cxImage, cyImage, ILC_MASK, 0, nImages);
    hbm = CreateColorBitmap(cxImage * nImages, cyImage);

    if (fUseColorKey)
    {
        clrMask = RGB(255,000,255); // magenta
        if (clrMask == g_clrWindow)
            clrMask = RGB(000,000,255); // blue
    }
    else
    {
        clrMask = g_clrWindow;
    }

    // fill
    hbmTemp = SelectObject(hdc, hbm);

    rc.left = rc.top = 0;
    rc.bottom = cyImage;
    rc.right = cxImage * nImages;
    FillRectClr(hdc, &rc, clrMask);

    rc.right = cxImage;
    // now draw the real controls on
    InflateRect(&rc, -g_cxEdge, -g_cyEdge);
    rc.right++;
    rc.bottom++;

    if (fTree)
        OffsetRect(&rc, cxImage, 0);

    DrawFrameControl(hdc, &rc, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_FLAT | 
        (fUseColorKey? 0 : DFCS_TRANSPARENT));
    OffsetRect(&rc, cxImage, 0);
    // [msadek]; For the mirrored case, there is an off-by-one somewhere in MirrorIcon() or System API.
    // Since I will not be touching MirrorIcon() by any mean and no chance to fix a system API,
    // let's compensate for it here.
    if(fMirror)
    {
        OffsetRect(&rc, -1, 0);  
    }

    DrawFrameControl(hdc, &rc, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_FLAT | DFCS_CHECKED | 
        (fUseColorKey? 0 : DFCS_TRANSPARENT));

    SelectObject(hdc, hbmTemp);

    if (fUseColorKey)
    {
        ImageList_AddMasked(himl, hbm, clrMask);
    }
    else
    {
        ImageList_Add(himl, hbm, NULL);
    }

    if(fMirror)
    {
        HICON hIcon = ImageList_ExtractIcon(0, himl, nImages-1);
        MirrorIcon(&hIcon, NULL);
        ImageList_ReplaceIcon(himl, nImages-1, hIcon);
    }

    DeleteDC(hdc);
    DeleteObject( hbm );
    return himl;
}

void ListView_InitCheckBoxes(LV* plv, BOOL fInitializeState)
{
    HIMAGELIST himlCopy = (plv->himlSmall ? plv->himlSmall : plv->himl);
    HIMAGELIST himl;
    BOOL fNoColorKey = FALSE;    // Backwards: If Cleartype is turned on, then we don't use colorkey.
    if (g_bRunOnNT5)
    {
#ifdef CLEARTYPE    // Don't use SPI_CLEARTYPE because it's defined because of APIThk, but not in NT.
        SystemParametersInfo(SPI_GETCLEARTYPE, 0, &fNoColorKey, 0);
#endif
    }

    // [msadek], CheckBoxed need not to be mirrored.
    // mirroer it during imagelist creation time so that it displays correctly
    
    himl = CreateCheckBoxImagelist(himlCopy, FALSE, !fNoColorKey, IS_WINDOW_RTL_MIRRORED(plv->ci.hwnd));
    ImageList_SetBkColor(himl, fNoColorKey ? (CLR_NONE) : (plv->clrBk));
    ListView_OnSetImageList(plv, himl, LVSIL_STATE);

    if (fInitializeState)
        ListView_OnSetItemState(plv, -1, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
}

void ListView_PopBubble(LV *plv)
{
    if (plv->hwndToolTips)
        SendMessage( plv->hwndToolTips, TTM_POP, 0, 0 );
}

DWORD NEAR PASCAL ListView_ExtendedStyleChange(LV* plv, DWORD dwNewStyle, DWORD dwExMask)
{
    DWORD dwOldStyle = plv->exStyle;

    // this messge didn't come till 3, so version is at least 3
    if (plv->ci.iVersion < 3) {
        plv->ci.iVersion = 3;
        // this will change the listview report size and painting algorithm
        // because of the leading edge, so need to re-update scroll bars
        // and repaint everything
        if (ListView_IsReportView(plv))
        {
            ListView_RUpdateScrollBars(plv);
            InvalidateRect(plv->ci.hwnd, NULL, TRUE);
        }
    }

    // Change of styles may also changes tooltip policy, so pop it
    ListView_PopBubble(plv);

    if (dwExMask)
        dwNewStyle = (plv->exStyle & ~ dwExMask) | (dwNewStyle & dwExMask);

    // Currently, the LVS_EX_REGIONAL style is only supported for large icon view
    if (!ListView_IsIconView(plv)) {
        dwNewStyle &= ~(LVS_EX_REGIONAL | LVS_EX_MULTIWORKAREAS);
    }

    // LVS_EX_REGIONAL is not supported for ownerdata
    if (ListView_IsOwnerData(plv)) {
        dwNewStyle &= ~LVS_EX_REGIONAL;
    }

    plv->exStyle = dwNewStyle;

    // do any invalidation or whatever is needed here.
    if ((dwOldStyle ^ dwNewStyle) & LVS_EX_GRIDLINES) {
        if (ListView_IsReportView(plv)) {
            InvalidateRect(plv->ci.hwnd, NULL, TRUE);
        }
    }

    if ((dwOldStyle ^ dwNewStyle) & (LVS_EX_UNDERLINEHOT | LVS_EX_UNDERLINECOLD |
                                     LVS_EX_ONECLICKACTIVATE | LVS_EX_TWOCLICKACTIVATE |
                                     LVS_EX_SUBITEMIMAGES)) {
        InvalidateRect(plv->ci.hwnd, NULL, TRUE);
    }

    if ((dwOldStyle ^ dwNewStyle) & LVS_EX_CHECKBOXES) {
        if (dwNewStyle & LVS_EX_CHECKBOXES) {
            ListView_InitCheckBoxes(plv, TRUE);
        } else {
            // destroy the check boxes!
            HIMAGELIST himl = ListView_OnSetImageList(plv, NULL, LVSIL_STATE);
            if (himl)
                ImageList_Destroy(himl);
        }
    }

    if ((dwOldStyle ^ dwNewStyle) & LVS_EX_FLATSB) {
        if (dwNewStyle & LVS_EX_FLATSB) {
            InitializeFlatSB(plv->ci.hwnd);
        } else {
            UninitializeFlatSB(plv->ci.hwnd);
        }
    }

    if ((dwOldStyle ^ dwNewStyle) & LVS_EX_REGIONAL) {
        if (g_fSlowMachine == -1) {
#ifdef NEVER
            // Because some Alpha machines and faster pentiums were detected
            // as slow machines (bug #30972 in IE4 database), it was decided
            // to turn off this code.
            SYSTEM_INFO si;
            GetSystemInfo(&si);
            g_fSlowMachine = (BOOL)((si.dwProcessorType == PROCESSOR_INTEL_386) ||
                                      (si.dwProcessorType == PROCESSOR_INTEL_486));
#else
            g_fSlowMachine = FALSE;
#endif
        }
        if (dwNewStyle & LVS_EX_REGIONAL) {
            ListView_RecalcRegion(plv, TRUE, TRUE);
        } else {
            int i;
            LISTITEM FAR * pitem;

            // Delete all the cached regions, then NULL out our selected region.
            for (i = 0; i < ListView_Count(plv); i++) {
                pitem = ListView_FastGetItemPtr(plv, i);
                if (pitem->hrgnIcon && pitem->hrgnIcon!=(HANDLE)-1) {
                    DeleteObject(pitem->hrgnIcon);
                }
                pitem->hrgnIcon = NULL;
            }
            SetWindowRgn(plv->ci.hwnd, (HRGN)NULL, TRUE);
        }
        InvalidateRect(plv->ci.hwnd, NULL, TRUE);
    }

    return dwOldStyle;
}

// BUGBUG raymondc v6.0:  Doesn't detect WM_WINDOWPOSCHANGING as a way
// of being shown.  NT5 defview has to hack around it pretty grossly.
// Fix for v6.0.

void NEAR PASCAL LV_OnShowWindow(LV* plv, BOOL fShow)
{
    if (fShow) {
        if (!(plv->flags & LVF_VISIBLE)) {
            plv->flags |= LVF_VISIBLE;
            ListView_UpdateScrollBars(plv);
        }
    } else
        plv->flags &= ~LVF_VISIBLE;

}

LRESULT NEAR PASCAL ListView_OnHelp(LV* plv, LPHELPINFO lpHelpInfo)
{

    //  If we're seeing WM_HELP because of our child header control, then
    //  munge the HELPINFO structure to use the ListView's control id.
    //  win\core\user\combo.c has similiar code to handle the child edit
    //  control of a combo box.
    if ((lpHelpInfo != NULL) && ((plv->ci.style & LVS_TYPEMASK) == LVS_REPORT) &&
        (lpHelpInfo->iCtrlId == LVID_HEADER)) {

        lpHelpInfo->hItemHandle = plv->ci.hwnd;
        lpHelpInfo->iCtrlId = GetWindowID(plv->ci.hwnd);
        //  Shouldn't have to do this: USER would have filled in the appropriate
        //  context id by walking up the parent hwnd chain.
        //lpHelpInfo->dwContextId = GetContextHelpId(hwnd);

    }

    return DefWindowProc(plv->ci.hwnd, WM_HELP, 0, (LPARAM)lpHelpInfo);

}

DWORD NEAR PASCAL ListView_OnSetIconSpacing(LV* plv, LPARAM lParam)
{
    DWORD dwOld = MAKELONG(plv->cxIconSpacing, plv->cyIconSpacing);

    if (lParam == (LPARAM)-1) {
        // go back to using defaults
        plv->flags &= ~LVF_ICONSPACESET;
        plv->cxIconSpacing = (plv->cxIcon + (g_cxIconSpacing - g_cxIcon));
        plv->cyIconSpacing = (plv->cyIcon + (g_cyIconSpacing - g_cyIcon));
    } else {
        if (LOWORD(lParam))
            plv->cxIconSpacing = LOWORD(lParam);
        if (HIWORD(lParam))
            plv->cyIconSpacing = HIWORD(lParam);

        plv->flags |= LVF_ICONSPACESET;
    }
    plv->iFreeSlot = -1;

    return dwOld;
}

BOOL ListView_OnSetCursorMsg(LV* plv)
{
    if (plv->exStyle & (LVS_EX_ONECLICKACTIVATE|LVS_EX_TWOCLICKACTIVATE)) {
        if (plv->iHot != -1) {
            if (((plv->exStyle & LVS_EX_ONECLICKACTIVATE && plv->fOneClickOK)) ||
                ListView_OnGetItemState(plv, plv->iHot, LVIS_SELECTED)) {
                if (!plv->hCurHot)
                    plv->hCurHot = LoadHandCursor(0);
                SetCursor(plv->hCurHot);

                return TRUE;
            }
        }
    }

    return FALSE;
}

void ListView_OnSetHotItem(LV* plv, int iItem)
{
    UINT maskChanged;

    if (iItem != plv->iHot) {
        BOOL fSelectOnly;
        UINT fRedraw = RDW_INVALIDATE;
#ifndef DONT_UNDERLINE
        if (plv->clrTextBk == CLR_NONE)
            fRedraw |= RDW_ERASE;
#endif
        fSelectOnly = ListView_FullRowSelect(plv);
        maskChanged = (plv->exStyle & LVS_EX_BORDERSELECT) ? LVIF_TEXT | LVIF_IMAGE : LVIF_TEXT;
        ListView_InvalidateItemEx(plv, plv->iHot, fSelectOnly, fRedraw, maskChanged);
        ListView_InvalidateItemEx(plv, iItem, fSelectOnly, RDW_INVALIDATE, maskChanged);
        plv->iHot = iItem;
    }
}


/// Usability test prototype
// CHEEBUGBUG
BOOL fShouldFirstClickActivate()
{
    static BOOL fInited = FALSE;
    static BOOL fActivate = TRUE;
    if (!fInited) {
        long cb = 0;
        if (RegQueryValue(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\NoFirstClickActivate"),
                      NULL, &cb) == ERROR_SUCCESS)
            fActivate = FALSE;
        fInited = TRUE;
    }
    return fActivate;
}

BOOL ChildOfDesktop(HWND hwnd)
{
    return IsChild(GetShellWindow(), hwnd);
}


void ListView_OnMouseMove(LV* plv, int x, int y, UINT uFlags)
{
    if (plv->exStyle & (LVS_EX_TRACKSELECT|LVS_EX_ONECLICKACTIVATE|LVS_EX_TWOCLICKACTIVATE)

        // CHEEBUGBUG: for usability testing
        && (ChildOfActiveWindow(plv->ci.hwnd) || fShouldFirstClickActivate() ||
              ChildOfDesktop(plv->ci.hwnd))

       ) {
        int iItem;
        LV_HITTESTINFO ht;
        NMLISTVIEW nm;

        ht.pt.x = x;
        ht.pt.y = y;
        iItem = ListView_OnSubItemHitTest(plv, &ht);
        if (ht.iSubItem != 0) {
            // if we're not in full row select,
            // hitting on a subitem is like hitting on nowhere
            // also, in win95, ownerdraw fixed effectively had full row select
            if (!ListView_FullRowSelect(plv) &&
                !(plv->ci.style & LVS_OWNERDRAWFIXED)) {
                iItem = -1;
                ht.flags = LVHT_NOWHERE;
            }
        }

        if (ht.flags & LVHT_NOWHERE ||
           ht.flags & LVHT_ONITEMSTATEICON) {
            iItem = -1; // this is possible in the list mode (sucks)
        }

        nm.iItem = iItem;
        nm.iSubItem = ht.iSubItem;
        nm.uChanged = 0;
        nm.ptAction.x = x;
        nm.ptAction.y = y;

        if (!CCSendNotify(&plv->ci, LVN_HOTTRACK, &nm.hdr)) {

#ifdef DEBUG
            if ((nm.iItem != -1) && nm.iSubItem != 0)
                nm.iItem = -1;
#endif

            ListView_OnSetHotItem(plv, nm.iItem);
            // Ensure our cursor is correct now since the WM_SETCURSOR
            // message was already generated for this mouse event.
            ListView_OnSetCursorMsg(plv);

            // this lets us know when we've left an item
            // and can then reselect/toggle it on hover events
            if (iItem != plv->iNoHover) {
                plv->iNoHover = -1;
            }
        }
    }
}

BOOL EditBoxHasFocus()
{
    HWND hwndFocus = GetFocus();

    if (hwndFocus) {
        if (SendMessage(hwndFocus, WM_GETDLGCODE, 0, 0) & DLGC_HASSETSEL)
            return TRUE;
    }

    return FALSE;
}

void ListView_OnMouseHover(LV* plv, int x, int y, UINT uFlags)
{
    int iItem;
    BOOL bSelected;
    LV_HITTESTINFO ht;
    BOOL fControl;
    BOOL fShift;
    BOOL fNotifyReturn = FALSE;

    if (GetCapture() || !ChildOfActiveWindow(plv->ci.hwnd) ||
       EditBoxHasFocus())
        return;  // ignore hover while editing or any captured (d/d) operation

    if (CCSendNotify(&plv->ci, NM_HOVER, NULL)) {
        return;
    }

    // REVIEW: right button implies no shift or control stuff
    // Single selection style also implies no modifiers
    //if (RIGHTBUTTON(keyFlags) || (plv->ci.style & LVS_SINGLESEL))
    if ((plv->ci.style & LVS_SINGLESEL)) {
        fControl = FALSE;
        fShift = FALSE;
    } else {
        fControl = GetAsyncKeyState(VK_CONTROL) < 0;
        fShift = GetAsyncKeyState(VK_SHIFT) < 0;
    }

    ht.pt.x = x;
    ht.pt.y = y;
    iItem = ListView_OnHitTest(plv, &ht);

    if (iItem == -1 ||
        iItem == plv->iNoHover)
        return;

    //before we hover select we launch any pending item
    //this prevents clicking on one item and hover selecting other before
    //the timer goes off which result in wrong item being launched
    if (plv->exStyle & LVS_EX_ONECLICKACTIVATE && plv->fOneClickHappened && plv->fOneClickOK)
    {
        HWND hwnd = plv->ci.hwnd;

        KillTimer(plv->ci.hwnd, IDT_ONECLICKHAPPENED);
        plv->fOneClickHappened = FALSE;
        CCSendNotify(&plv->ci, LVN_ITEMACTIVATE, &(plv->nmOneClickHappened.hdr));
        if (!IsWindow(hwnd))
            return;
    }

    plv->iNoHover = iItem;
    bSelected = ListView_OnGetItemState(plv, iItem, LVIS_SELECTED);

    if (ht.flags & (LVHT_ONITEMLABEL | LVHT_ONITEMICON))
    {
        UINT keyFlags = 0;

        if (fShift)
            keyFlags |= MK_SHIFT;
        if (fControl)
            keyFlags |= MK_CONTROL;

        if (!bSelected) {
            // if it wasn't selected, we're about to select it... play
            // a little ditty for us...
            CCPlaySound(c_szSelect);
        }

        ListView_ButtonSelect(plv, iItem, keyFlags, bSelected);

        if (fControl)
        {
            ListView_SetFocusSel(plv, iItem, !fShift, FALSE, !fShift);
        }

        if (!fShift)
            plv->iMark = iItem;

        ListView_OnSetCursorMsg(plv);

        SetFocus(plv->ci.hwnd);    // activate this window

    }
}

BOOL EqualRects(LPRECT prcNew, LPRECT prcOld, int nRects)
{
    int i;
    for (i = 0; i < nRects; i++)
        if (!EqualRect(&prcNew[i], &prcOld[i]))
            return FALSE;
    return TRUE;
}

BOOL ListView_FindWorkArea(LV * plv, POINT pt, short * piWorkArea)
{
    int iWork;
    for (iWork = 0; iWork < plv->nWorkAreas; iWork++)
    {
        if (PtInRect(&plv->prcWorkAreas[iWork], pt))
        {
            *piWorkArea = (short)iWork;
            return TRUE;
        }
    }

    // (dli) default case is the primary work area
    *piWorkArea = 0;
    return FALSE;
}

void ListView_BullyIconsOnWorkarea(LV * plv, HDPA hdpaLostItems)
{
    int ihdpa;
    int iFree = -1;  // the last free slot number
    LVFAKEDRAW lvfd;
    LV_ITEM item;

    // Caller should've filtered this case out
    ASSERT(DPA_GetPtrCount(hdpaLostItems) > 0);

    // Set up in case caller is customdraw
    ListView_BeginFakeCustomDraw(plv, &lvfd, &item);
    item.mask = LVIF_PARAM;
    item.iSubItem = 0;

    // Go through my hdpa list of lost icons and try to place them within bound
    for (ihdpa = 0; ihdpa < DPA_GetPtrCount(hdpaLostItems); ihdpa++)
    {
        POINT ptNew, pt;
        RECT rcBound;
        int cxBound, cyBound;
        int iWidth, iHeight;
        int iItem;
        LISTITEM FAR * pitem;

        iItem = PtrToUlong(DPA_GetPtr(hdpaLostItems, ihdpa));
        pitem = ListView_FastGetItemPtr(plv, iItem);
        pt = pitem->pt;

        iWidth  = RECTWIDTH(plv->prcWorkAreas[pitem->iWorkArea]);
        iHeight = RECTHEIGHT(plv->prcWorkAreas[pitem->iWorkArea]);

        ListView_GetRects(plv, iItem, NULL, NULL, &rcBound, NULL);
        cxBound = RECTWIDTH(rcBound);
        cyBound = RECTHEIGHT(rcBound);

        pt.x -= plv->prcWorkAreas[pitem->iWorkArea].left;
        pt.y -= plv->prcWorkAreas[pitem->iWorkArea].top;

        if (pt.x < (-cxBound / 2)) {
            ptNew.x = 0;
        } else if (pt.x > (iWidth - (cxBound / 2))) {
            ptNew.x = iWidth - cxBound;
        } else
            ptNew.x = pt.x;

        if (pt.y < (-cyBound/2)) {
            ptNew.y = 0;
        } else if (pt.y > (iHeight - (cyBound / 2))) {
            ptNew.y = iHeight - cyBound;
        } else
            ptNew.y = pt.y;

        if ((ptNew.x != pt.x) || (ptNew.y != pt.y))
        {
            BOOL fUpdate;
            RECT rcTest;
            ptNew.x += plv->prcWorkAreas[pitem->iWorkArea].left;
            ptNew.y += plv->prcWorkAreas[pitem->iWorkArea].top;

            // See if the potential rectangle intersects other items.
            rcTest.left = ptNew.x - plv->ptOrigin.x;
            rcTest.top = ptNew.y - plv->ptOrigin.y;
            rcTest.right = rcTest.left + cxBound;
            rcTest.bottom = rcTest.top + cyBound;

            item.iItem = iItem;
            item.lParam = pitem->lParam;
            ListView_BeginFakeItemDraw(&lvfd);

            if (!ListView_IsCleanRect(plv, &rcTest, iItem, &fUpdate, lvfd.nmcd.nmcd.hdc))
            {
                // doh! We hit another item, let's try to find an available location
                // for this item
                BOOL fUpdateSB;
                BOOL fAppendAtEnd = FALSE;
                int cSlots = ListView_GetSlotCountEx(plv, FALSE, pitem->iWorkArea);
                iFree = ListView_FindFreeSlot(plv, iItem, iFree + 1, cSlots, &fUpdateSB, &fAppendAtEnd, lvfd.nmcd.nmcd.hdc);
                if (iFree == -1)
                    goto SetFirstGuess;
                ListView_SetIconPos(plv, pitem, iFree, cSlots);
                ListView_EndFakeItemDraw(&lvfd);
                continue;
            }
SetFirstGuess:
            ListView_EndFakeItemDraw(&lvfd);
            ListView_OnSetItemPosition(plv, iItem, ptNew.x, ptNew.y);
        }
    }
    ListView_EndFakeCustomDraw(&lvfd);
}

#define DPA_LAST    0x7fffffff

//
// ListView_OnSetWorkAreas
//
// set the "work areas" for the list view.
// the "work areas" are a group of sub rectanges of the list view client rect
// where icons are aranged, and parked by default.
//
void NEAR ListView_OnSetWorkAreas(LV* plv, int nWorkAreas, LPRECT prc)
{
    int nOldWorkAreas;
    int iWork;
    BOOL bAutoArrange;
    HDPA hdpaLostItems = NULL;
    RECT rcOldWorkAreas[LV_MAX_WORKAREAS];

    bAutoArrange = plv->ci.style & LVS_AUTOARRANGE;

    nOldWorkAreas = plv->nWorkAreas;

    if (nOldWorkAreas > 0)
    {
        ASSERT(plv->prcWorkAreas != NULL);
        hmemcpy(&rcOldWorkAreas[0], &plv->prcWorkAreas[0], SIZEOF(RECT) * nOldWorkAreas);
    }
    // for the mirrored case, the coordinates are reversed. IsRectEmpty() will always succeed
    if (nWorkAreas == 0 || prc == NULL || ((IsRectEmpty(prc)) && !(plv->ci.dwExStyle & RTL_MIRRORED_WINDOW)))
        plv->nWorkAreas = 0;
    else
    {
        plv->nWorkAreas = min(nWorkAreas, LV_MAX_WORKAREAS);

        if (plv->prcWorkAreas == NULL)
            plv->prcWorkAreas = (LPRECT)LocalAlloc(LPTR, SIZEOF(RECT) * LV_MAX_WORKAREAS);

        if (plv->prcWorkAreas == NULL)
            return;

        //BUGBUG: Should we check if they intersect? This problem is sort of
        // solved (or made more confusing) by ListView_GetFreeSlot since it checks all of the icons for
        // intersection instead of just the ones in the workarea.
        for (iWork = 0; iWork < plv->nWorkAreas; iWork++)
            CopyRect(&plv->prcWorkAreas[iWork], &prc[iWork]);
    }

    // We don't support workareas for owner-data because our icon placement
    // algorithm (ListView_IGetRectsOwnerData) completely ignores workareas
    // and just dumps the icons in a rectangular array starting at (0,0).
    if (!ListView_IsOwnerData(plv) &&
        plv->nWorkAreas > 0 &&
        ((plv->nWorkAreas  != nOldWorkAreas) ||
         (!EqualRects(&plv->prcWorkAreas[0], &rcOldWorkAreas[0], nOldWorkAreas))))
    {
        int iItem;
        LISTITEM FAR * pitem;

        //
        //  Subtle - ListView_Recompute cleans up all the RECOMPUTE icons,
        //  but in order to do that, it needs to have valid work area
        //  rectangles. So the call must happen after the CopyRect but before
        //  the loop that checks the icon positions.
        //
        ListView_Recompute(plv);

        for (iItem = 0; iItem < ListView_Count(plv); iItem++)
        {
            pitem = ListView_FastGetItemPtr(plv, iItem);

            if (pitem->pt.x == RECOMPUTE || pitem->pt.y == RECOMPUTE)
            {
                // ListView_Recompute should've fixed these if we were in
                // an iconical view.
                ASSERT(!(ListView_IsIconView(plv) || ListView_IsSmallView(plv)));
                continue;
            }

            // Try to move me to the same location relative to the same workarea.
            // This will give the cool shift effect when tools bars take the border areas.
            // And we only want to do this for the workareas that changed

            // Don't bully the icons on the workareas, Autoarrange will do the work for us

            if (nOldWorkAreas > 0)
            {
                int iOldWorkArea;
                iOldWorkArea = pitem->iWorkArea;
                if (iOldWorkArea >= plv->nWorkAreas)
                {
                    // My workarea is gone, put me on the primary workarea i.e. #0
                    pitem->iWorkArea = 0;
                    if (!bAutoArrange)
                    {
                        // If this item point location is already in the new primary workarea,
                        // move it out, and let ListView_BullyIconsOnWorkarea arrange it to the
                        // right place. NOTE: this could happen in the case the old secondary monitor
                        // is to the left of the old primary monitor, and user kills the secondary monitor
                        if (PtInRect(&plv->prcWorkAreas[0], pitem->pt))
                        {
                            pitem->pt.x = plv->prcWorkAreas[0].right + 1;
                            plv->iFreeSlot = -1; // an item moved -- old slot info is invalid
                        }
                        goto  InsertLostItemsArray;
                    }
                }
                else if ((!bAutoArrange) && (!EqualRect(&plv->prcWorkAreas[iOldWorkArea], &rcOldWorkAreas[iOldWorkArea])))
                {
                    RECT rcBound;
                    POINT ptCenter;
                    pitem->pt.x += plv->prcWorkAreas[iOldWorkArea].left - rcOldWorkAreas[iOldWorkArea].left;
                    pitem->pt.y += plv->prcWorkAreas[iOldWorkArea].top - rcOldWorkAreas[iOldWorkArea].top;

                    // Use the center of this icon to determine whether it's out of bound
                    ListView_GetRects(plv, iItem, NULL, NULL, &rcBound, NULL);
                    ptCenter.x = pitem->pt.x + RECTWIDTH(rcBound) / 2;
                    ptCenter.y = pitem->pt.y + RECTHEIGHT(rcBound) / 2;

                    // If this shifted me out of bounds, register to be bullied on the workarea
                    if (!PtInRect(&plv->prcWorkAreas[iOldWorkArea], ptCenter))
                    {
InsertLostItemsArray:
                        if (!hdpaLostItems)
                        {
                            hdpaLostItems = DPA_Create(4);
                            if (!hdpaLostItems)
                                // we ran out of memory
                                ASSERT(0);
                        }

                        if (hdpaLostItems)
                            DPA_InsertPtr(hdpaLostItems, DPA_LAST, (LPVOID)iItem);
                    }
                }

            }
            else
            {
                // My first time in a multi-workarea system, so find out my workarea
                if (!ListView_FindWorkArea(plv, pitem->pt, &(pitem->iWorkArea)) && !bAutoArrange)
                    goto InsertLostItemsArray;
            }

            if ((plv->exStyle & LVS_EX_REGIONAL) && (pitem->hrgnIcon))
            {
                if (pitem->hrgnIcon != (HANDLE)-1)
                    DeleteObject(pitem->hrgnIcon);
                pitem->hrgnIcon = NULL;
            }
        }

        if (hdpaLostItems)
        {
            ASSERT(!bAutoArrange);
            if (DPA_GetPtrCount(hdpaLostItems) > 0)
                ListView_BullyIconsOnWorkarea(plv, hdpaLostItems);

            DPA_Destroy(hdpaLostItems);
        }

        if (plv->exStyle & LVS_EX_REGIONAL)
            ListView_RecalcRegion(plv, TRUE, TRUE);

        if ((plv->ci.style & LVS_AUTOARRANGE) &&
            (ListView_IsSmallView(plv) || ListView_IsIconView(plv)))
            ListView_OnArrange(plv, LVA_DEFAULT);
    }

    RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}

void NEAR ListView_OnGetNumberOfWorkAreas(LV* plv, int * pnWorkAreas)
{
    if (pnWorkAreas)
        *pnWorkAreas = plv->nWorkAreas;
}

void NEAR ListView_OnGetWorkAreas(LV* plv, int nWorkAreas, LPRECT prc)
{
    int i;
    ASSERT(prc);
    for (i = 0; i < min(plv->nWorkAreas, nWorkAreas); i++)
    {
        if (i < plv->nWorkAreas)
            CopyRect(&prc[i], &plv->prcWorkAreas[i]);
        else
            // Set the workareas to all zeros if we don't have it.
            ZeroMemory(&prc[i], sizeof(RECT));
    }
}

// test an item to see if it is unfolded (because it is focused)

BOOL ListView_IsItemUnfolded(LV *plv, int item)
{
    return plv && (item >= 0) && ListView_IsIconView(plv) &&
           (plv->flags & LVF_UNFOLDED) && (plv->iFocus == item);
}

BOOL ListView_IsItemUnfoldedPtr(LV *plv, LISTITEM *pitem)
{
    return plv && pitem && ListView_IsIconView(plv) &&
           (plv->flags & LVF_UNFOLDED) && (pitem->state & LVIS_FOCUSED);
}

// Returns TRUE if unfolding the item will be worthwhile
BOOL ListView_GetUnfoldedRect(LV* plv, int iItem, RECT *prc)
{
    ListView_GetRects(plv, iItem, NULL, prc, NULL, NULL);
    return ListView_UnfoldRects(plv, iItem, NULL, prc, NULL, NULL);
}

LRESULT ListView_OnNotify(LV* plv, WPARAM wParam, LPNMHDR pnmh)
{
    // we can't switch on the control ID because the tooltip is a WS_POPUP window
    // and does not have a control ID. (header and tooltip both have 0 as ID)

    if (plv->hwndHdr && (plv->hwndHdr == pnmh->hwndFrom))
    {
        // this is a notify for the header, deal with it as needed

        return ListView_HeaderNotify(plv, (HD_NOTIFY *)pnmh);
    }
    else if (plv->hwndToolTips && (plv->hwndToolTips == pnmh->hwndFrom))
    {
        // implement unfolding the text for items as well as info tip support

        switch (pnmh->code)
        {
        case TTN_NEEDTEXT:
        {
            POINT pt;
            UINT uFlags;
            int iNewHit;
            int iNewSubHit;
            NMTTDISPINFO *pttt = (NMTTDISPINFO *)pnmh;

            GetMessagePosClient(plv->ci.hwnd, &pt);
            iNewHit = _ListView_ItemHitTest(plv, pt.x, pt.y, &uFlags, &iNewSubHit);

            if (iNewHit != plv->iTTLastHit || iNewSubHit != plv->iTTLastSubHit)
            {
                plv->fPlaceTooltip = FALSE;     // Set it to TRUE only if Unfolding tip is set
                Str_Set(&plv->pszTip, NULL);    // clear the old tip

                plv->iTTLastHit = iNewHit;
                plv->iTTLastSubHit = iNewSubHit;

                if ((iNewHit >= 0) && (plv->iEdit == -1))
                {
                    TCHAR szBuf[INFOTIPSIZE], szBuf2[INFOTIPSIZE];
                    BOOL bItemUnfolded;
                    BOOL fInfoTip = FALSE;
                    LPTSTR pszTip = szBuf;  // Use this one first

                    szBuf[0] = 0;
                    szBuf2[0] = 0;

                    // preload the tip text for folder items. this
                    // may be overridden by callback below
                    bItemUnfolded = ListView_IsItemUnfolded2(plv, plv->iTTLastHit, plv->iTTLastSubHit, szBuf, ARRAYSIZE(szBuf));
                    lstrcpyn(szBuf2, szBuf, ARRAYSIZE(szBuf2)); // Backup the unfolding text

                    if (ListView_IsInfoTip(plv) && iNewSubHit == 0)
                    {
                        NMLVGETINFOTIP git;

                        git.dwFlags = bItemUnfolded ? LVGIT_UNFOLDED : 0;
                        git.pszText = szBuf;
                        git.cchTextMax = ARRAYSIZE(szBuf);
                        git.iItem = plv->iTTLastHit;
                        git.iSubItem = 0;
                        git.lParam = 0;

                        // for folded items pszText is prepopulated with the
                        // item text, clients should append to this

                        CCSendNotify(&plv->ci, LVN_GETINFOTIP, &git.hdr);

                        // Sometimes defview gets confused and nulls out the
                        // buffer instead of leaving it alone (sigh)

                        if (szBuf[0] == TEXT('\0'))
                        {
                            pszTip = szBuf2;  // Use the original text
                        }
                        else if (lstrcmp(szBuf, szBuf2) != 0)
                        {
                            // App changed something - there is a real infotip
                            fInfoTip = TRUE;
                        }

                    }
                    
                    //
                    // Set the margins now before the TTN_SHOW because it will be too late then.
                    //
                    // We want fat margins if we're an infotip, thin margins if we're an
                    // in-place tooltip.
                    //
                    if (fInfoTip)
                    {
                        static const RECT rcMargin = {4, 4, 4, 4};
                        SendMessage(plv->hwndToolTips, TTM_SETMARGIN, 0, (LPARAM)&rcMargin);
                        CCSetInfoTipWidth(plv->ci.hwnd, plv->hwndToolTips);

                    }
                    else
                    {
                        static const RECT rcMargin = {0, 0, 0, 0};
                        plv->fPlaceTooltip = TRUE;
                        SendMessage(plv->hwndToolTips, TTM_SETMARGIN, 0, (LPARAM)&rcMargin);
                        CCResetInfoTipWidth(plv->ci.hwnd, plv->hwndToolTips);
                    }

                    Str_Set(&plv->pszTip, pszTip);
                }
            }

            pttt->lpszText = plv->pszTip;     // here it is...
        }
        break;

        // Handle custom draw as we want the tooltip painted as a multi-line that
        // matches the formatting used by the list view.

        case NM_CUSTOMDRAW:
        {
            LPNMTTCUSTOMDRAW pnm = (LPNMTTCUSTOMDRAW) pnmh;

            if (plv->fPlaceTooltip &&
                (pnm->nmcd.dwDrawStage == CDDS_PREPAINT ||
                 pnm->nmcd.dwDrawStage == CDDS_ITEMPREPAINT))
            {
                DWORD dwCustom = 0;

                //
                //  Set up the customdraw DC to match the font of the LV item.
                //
                if (plv->iTTLastHit != -1)
                {
                    LVFAKEDRAW lvfd;
                    LV_ITEM item;
                    ListView_BeginFakeCustomDraw(plv, &lvfd, &item);

                    item.iItem = plv->iTTLastHit;
                    item.iSubItem = plv->iTTLastSubHit;
                    item.mask = LVIF_PARAM;
                    ListView_OnGetItem(plv, &item);
                    dwCustom = ListView_BeginFakeItemDraw(&lvfd);

                    // If client changed the font, then transfer the font
                    // from our private hdc into the tooltip's HDC.  We use
                    // a private HDC because we only want to let the app change
                    // the font, not the colors or anything else.
                    if (dwCustom & CDRF_NEWFONT)
                    {
                        SelectObject(pnm->nmcd.hdc, GetCurrentObject(lvfd.nmcd.nmcd.hdc, OBJ_FONT));
                    }
                    ListView_EndFakeItemDraw(&lvfd);
                    ListView_EndFakeCustomDraw(&lvfd);

                }

                //
                //  The Large Icon tooltip needs to be drawn specially.
                //
                if (ListView_IsIconView(plv))
                {
                    pnm->uDrawFlags &= ~(DT_SINGLELINE|DT_LEFT);
                    pnm->uDrawFlags |= DT_CENTER|DT_LVWRAP;

                    if ( pnm->uDrawFlags & DT_CALCRECT )
                    {
                        pnm->nmcd.rc.right = pnm->nmcd.rc.left + (lv_cxIconSpacing - g_cxLabelMargin * 2);
                        pnm->nmcd.rc.bottom = pnm->nmcd.rc.top + 0x10000;           // big number, no limit!
                    }
                }

                // Don't return other wacky flags to TT, since all we
                // did was change the font (if even that)
                return dwCustom & CDRF_NEWFONT;
            }
        }
        break;

        case TTN_SHOW:
            if (plv->iTTLastHit != -1)
            {
                if (plv->fPlaceTooltip)
                {
                    LPNMTTSHOWINFO psi = (LPNMTTSHOWINFO)pnmh;
                    RECT rcLabel;

                    // In case we're doing subitem hit-testing
                    rcLabel.top = plv->iTTLastSubHit;
                    rcLabel.left = LVIR_LABEL;

                    // reposition to allign with the text rect and
                    // set it to topmost
                    if (plv->iTTLastSubHit && ListView_OnGetSubItemRect(plv, plv->iTTLastHit, &rcLabel)) {
                        LV_ITEM item;

                        // we got the subitem rect. When we draw subitems, we give
                        // them SHDT_EXTRAMARGIN, so we have to also
                        rcLabel.left += g_cxLabelMargin * 3;
                        rcLabel.right -= g_cxLabelMargin * 3;

                        // And take the image into account, too.
                        // ListView_OnGetItem will worry about LVS_EX_SUBITEMIMAGES.
                        item.mask = LVIF_IMAGE;
                        item.iImage = -1;
                        item.iItem = plv->iTTLastHit;
                        item.iSubItem = plv->iTTLastSubHit;
                        ListView_OnGetItem(plv, &item);
                        if (item.iImage != -1)
                            rcLabel.left += plv->cxSmIcon;
                    } else {                    // a tip from subitem zero
                        ListView_GetUnfoldedRect(plv, plv->iTTLastHit, &rcLabel);
                        // SHDrawText actually leaves a g_cxLabelMargin margin
                        rcLabel.left += g_cxLabelMargin;
                        rcLabel.right -= g_cxLabelMargin;
                    }

                    // In report and list views, SHDrawText does vertical
                    // centering (without consulting the custom-draw client,
                    // even, so it just centers by a random amount).
                    if (ListView_IsListView(plv) || ListView_IsReportView(plv))
                    {
                        rcLabel.top += (rcLabel.bottom - rcLabel.top - plv->cyLabelChar) / 2;
                    }

                    SendMessage(plv->hwndToolTips, TTM_ADJUSTRECT, TRUE, (LPARAM)&rcLabel);
                    MapWindowRect(plv->ci.hwnd, HWND_DESKTOP, &rcLabel);

                    if (!ListView_IsIconView(plv))
                    {
                        // In non-large-icon view, the label size may be greater than the rect returned by ListView_GetUnfoldedRect.
                        // So don't specify the size
                        SetWindowPos(plv->hwndToolTips, HWND_TOP,
                                 rcLabel.left, rcLabel.top,
                                 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_HIDEWINDOW);
                    }
                    else
                    {
                        SetWindowPos(plv->hwndToolTips, HWND_TOP,
                                 rcLabel.left, rcLabel.top,
                                 (rcLabel.right - rcLabel.left), (rcLabel.bottom - rcLabel.top),
                                 SWP_NOACTIVATE | SWP_HIDEWINDOW);
                    }
                    // This is an inplace tooltip, so disable animation.
                    psi->dwStyle |= TTS_NOANIMATE;
                    return TRUE;
                }
            }
            break;

        }
    }

    return 0;
}

// Pass the focus to the given window, and then check to see if it exists.
// Passing focus can cause the window to be destroyed (by the Explorer
// when renaming).

BOOL NEAR ListView_SetFocus(HWND hwnd)
{
    SetFocus(hwnd);
    return IsWindow(hwnd);
}

void NEAR ListView_Realize(LV* plv, HDC hdcParam, BOOL fBackground, BOOL fForceRepaint)
{
    if (plv->hpalHalftone)
    {
        HDC hdc = hdcParam ? hdcParam : GetDC(plv->ci.hwnd);

        if (hdc)
        {
            BOOL fRepaint;

            SelectPalette(hdc, plv->hpalHalftone, fBackground);
            fRepaint = RealizePalette(hdc) || fForceRepaint;

            if (!hdcParam)
                ReleaseDC(plv->ci.hwnd, hdc);

            if (fRepaint)
            {
                InvalidateRect(plv->ci.hwnd, NULL, TRUE);
            }
        }
    }
}

BOOL RectInRect(const RECT* prcOuter, const RECT* prcInner)
{
#ifdef COMPLETELYINRECT
    return (prcOuter->left   <= prcInner->left  ) &&
           (prcOuter->top    <= prcInner->top   ) &&
           (prcOuter->right  >= prcInner->right ) &&
           (prcOuter->bottom >= prcInner->bottom);
#else
    RECT rcDummy;
    return IntersectRect(&rcDummy, prcOuter, prcInner);
#endif
}


LRESULT LVGenerateDragImage(LV* plv, SHDRAGIMAGE* pshdi)
{
    LRESULT lRet = 0;
    int iNumSelected = plv->nSelected;
    int iIndex;
    int iSelectedItem;
    RECT rc = {0, 0, 0, 0};
    RECT rcVisRect;
    HBITMAP hbmpOld = NULL;
    HDC  hdcDragImage;

    // First loop through can get the selection rect
    if (ListView_IsOwnerData( plv )) 
    {
        plv->plvrangeSel->lpVtbl->CountIncluded(plv->plvrangeSel, &iNumSelected);
    }

    if (iNumSelected == 0)
        return FALSE;

    GetClientRect(plv->ci.hwnd, &rcVisRect);


    // Loop Through and calculate the enclosing rect.
    for (iIndex = iNumSelected - 1, iSelectedItem = -1; iIndex >= 0; iIndex--)
    {
        iSelectedItem = ListView_OnGetNextItem(plv, iSelectedItem, LVNI_SELECTED);
        if (iSelectedItem != -1)
        {
            RECT rcItemBounds;

            // Make sure this is in the visible region
            if (ListView_GetItemRect(plv->ci.hwnd, iSelectedItem, &rcItemBounds, LVIR_SELECTBOUNDS) &&
                RectInRect(&rcVisRect, &rcItemBounds))
            {
                UnionRect(&rc, &rc, &rcItemBounds);
            }
        }
    }

    hdcDragImage = CreateCompatibleDC(NULL);

    if (!hdcDragImage)
        return 0;

    // After this rc contains the bounds of all the items in Client Coordinates.
    //
    // Mirror the the DC, if the listview is mirrored.
    //
    if (plv->ci.dwExStyle & RTL_MIRRORED_WINDOW)
    {
        SET_DC_RTL_MIRRORED(hdcDragImage);
    }

#define MAX_DRAG_RECT_WIDTH 300
#define MAX_DRAG_RECT_HEIGHT 300
    // If this rect is too big, fix it.
    if (RECTWIDTH(rc) > MAX_DRAG_RECT_WIDTH)
    {
        int iLeft = MAX_DRAG_RECT_WIDTH / 2;
        int iRight = MAX_DRAG_RECT_WIDTH /2;

        int iRectOriginalLeft = rc.left;
        // Is the left boundry outside the visible rect?
        if (rc.left < plv->ptCapture.x - iLeft)
        {
            // Yes, then we have to clip it.
            rc.left = plv->ptCapture.x - iLeft;
        }
        else
        {
            // No? Well then shift the visible rect to the right, so that we have
            // more room.
            iRight += rc.left - (plv->ptCapture.x - iLeft);
        }

        // Is the right boundry outside the visible rect?
        if (rc.right > plv->ptCapture.x + iRight)
        {
            // Yes, then we have to clip it.
            rc.right = plv->ptCapture.x + iRight;
        }
        else
        {
            // No? Then try and add it to the left
            if (rc.left > iRectOriginalLeft)
            {
                rc.left -= iRight - (rc.right - plv->ptCapture.x);
                if (rc.left < iRectOriginalLeft)
                    rc.left = iRectOriginalLeft;
            }
        }
    }

    if (RECTHEIGHT(rc) > MAX_DRAG_RECT_HEIGHT)
    {
        // same for top and bottom:
        // Is the top boundry outside the visible rect?
        int iTop = MAX_DRAG_RECT_HEIGHT / 2;
        int iBottom = MAX_DRAG_RECT_HEIGHT /2;
        int iRectOriginalTop = rc.top;
        if (rc.top < plv->ptCapture.y - iTop)
        {
            // Yes, then we have to clip it.
            rc.top = plv->ptCapture.y - iTop;
        }
        else
        {
            // No? Well then shift the visible rect to the right, so that we have
            // more room.
            iBottom += rc.top - (plv->ptCapture.y - iTop);
        }

        // Is the right boundry outside the visible rect?
        if (rc.bottom > plv->ptCapture.y + iBottom)
        {
            // Yes, then we have to clip it.
            rc.bottom = plv->ptCapture.y + iBottom;
        }
        else
        {
            // No? Then try and add it to the top
            if (rc.top > iRectOriginalTop)
            {
                rc.top -= iBottom - (rc.bottom - plv->ptCapture.y);
                if (rc.top < iRectOriginalTop)
                    rc.top = iRectOriginalTop;
            }
        }
    }

    pshdi->sizeDragImage.cx = RECTWIDTH(rc);
    pshdi->sizeDragImage.cy = RECTHEIGHT(rc);
    pshdi->hbmpDragImage = CreateBitmap( pshdi->sizeDragImage.cx, pshdi->sizeDragImage.cy,
        GetDeviceCaps(hdcDragImage, PLANES), GetDeviceCaps(hdcDragImage, BITSPIXEL),
        NULL);

    if (pshdi->hbmpDragImage)
    {
        LVDRAWITEM lvdi;
        DWORD dwType;
        int cItem;

        RECT  rcImage = {0, 0, pshdi->sizeDragImage.cx, pshdi->sizeDragImage.cy};
        hbmpOld = SelectObject(hdcDragImage, pshdi->hbmpDragImage);

        pshdi->crColorKey = RGB(0xFF, 0x00, 0x55);
        FillRectClr(hdcDragImage, &rcImage, pshdi->crColorKey);
        pshdi->crColorKey = GetPixel(hdcDragImage, 0, 0);

        // Calculate the offset... The cursor should be in the bitmap rect.

        if (plv->ci.dwExStyle & RTL_MIRRORED_WINDOW)
            pshdi->ptOffset.x = rc.right - plv->ptCapture.x;
        else
            pshdi->ptOffset.x = plv->ptCapture.x - rc.left;
        pshdi->ptOffset.y = plv->ptCapture.y - rc.top;

        lvdi.prcClip = NULL;
        lvdi.plv = plv;
        lvdi.nmcd.nmcd.hdc = hdcDragImage;
        lvdi.pitem = NULL;
        dwType = plv->ci.style & LVS_TYPEMASK;
        cItem = ListView_Count(plv);

        // Now loop through again for the paint cycle
        for (iIndex = cItem - 1, iSelectedItem = -1; iIndex >= 0; iIndex--)
        {
            if (ListView_IsOwnerData( plv )) 
            {
                iSelectedItem++;
                plv->plvrangeSel->lpVtbl->NextSelected(plv->plvrangeSel, iSelectedItem, &iSelectedItem);
            }
            else
            {
                LISTITEM FAR* pitem;
                iSelectedItem = (int)(UINT_PTR)DPA_FastGetPtr(plv->hdpaZOrder, iIndex);
                pitem = ListView_FastGetItemPtr(plv, iSelectedItem);
                if (!(pitem->state & LVIS_SELECTED))
                    iSelectedItem = -1;
            }

            if (iSelectedItem != -1)
            {
                int     iOldItemDrawing;
                COLORREF crSave;
                POINT ptOrigin = {-rc.left, -rc.top};     //Offset the rects by...
                RECT  rcItemBounds;
                RECT rcTemp;

                iOldItemDrawing = plv->iItemDrawing;
                plv->iItemDrawing = iSelectedItem;
                lvdi.nmcd.nmcd.dwItemSpec = iSelectedItem;
                ListView_GetRects(plv, iSelectedItem, NULL, NULL, &rcItemBounds, NULL);

                // Make sure this is in the visible region
                if (IntersectRect(&rcTemp, &rcVisRect, &rcItemBounds))
                {
                    ptOrigin.x += rcItemBounds.left;
                    ptOrigin.y += rcItemBounds.top;
                    // these may get changed
                    lvdi.lpptOrg = &ptOrigin;
                    lvdi.flags = 0;
                    lvdi.nmcd.clrText = plv->clrText;
                    lvdi.nmcd.clrTextBk = plv->clrTextBk;

                    // Save the Background color!
                    crSave = plv->clrBk;
                    plv->clrBk = pshdi->crColorKey;

                    ListView_DrawItem(&lvdi);

                    plv->clrBk = crSave;
                }
                plv->iItemDrawing = iOldItemDrawing;
            }
        }

        SelectObject(hdcDragImage, hbmpOld);
        DeleteDC(hdcDragImage);

        // We're passing back the created HBMP.
        return 1;
    }


    return lRet;
}

LRESULT CALLBACK ListView_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LV* plv = ListView_GetPtr(hwnd);

    if (plv == NULL)
    {
        if (uMsg == WM_NCCREATE)
        {
            plv = (LV*)NearAlloc(sizeof(LV));
            if (!plv)
            {
                DebugMsg(DM_ERROR, TEXT("ListView: Out of near memory"));
                return 0L;      // fail the window create
            }

            plv->ci.hwnd = hwnd;
            plv->flags = LVF_REDRAW;    // assume that redrawing enabled!
            plv->iFocus = -1;           // no focus
            plv->iMark = -1;
            plv->iSelCol = -1;
            plv->iDropHilite = -1;      // Assume no item has drop hilite...
            plv->cyItem = plv->cyItemSave = 1; // never let these be zero, not even for a moment
#ifdef WIN32
            plv->hheap = GetProcessHeap();
#else
            // plv->hheap = NULL;  // not used in 16 bits...
#endif
            ListView_SetPtr(hwnd, plv);
        }
        goto DoDefault;
    }

    if ((uMsg >= WM_MOUSEFIRST) && (uMsg <= WM_MOUSELAST)) {
        if (plv->exStyle & (LVS_EX_TRACKSELECT|LVS_EX_ONECLICKACTIVATE|LVS_EX_TWOCLICKACTIVATE)) {
            TRACKMOUSEEVENT tme;

            tme.cbSize = sizeof(tme);
            tme.hwndTrack = plv->ci.hwnd;
            tme.dwHoverTime = plv->dwHoverTime;
            tme.dwFlags = TME_LEAVE | TME_HOVER | TME_QUERY;

            // see what's set
            TrackMouseEvent(&tme);
            tme.dwFlags &= TME_HOVER | TME_LEAVE;

            // set these bits if they aren't already set
            tme.dwFlags ^= TME_LEAVE;
            if (plv->exStyle & LVS_EX_TRACKSELECT) {
                tme.dwFlags ^= TME_HOVER;
            }

            tme.cbSize = sizeof(tme);
            tme.hwndTrack = plv->ci.hwnd;
            tme.dwHoverTime = plv->dwHoverTime;
            // set it if there's anything to set
            if (tme.dwFlags & (TME_HOVER | TME_LEAVE)) {
                TrackMouseEvent(&tme);
            }
        }
    }

    if (uMsg == g_uDragImages)
    {
        return LVGenerateDragImage(plv, (SHDRAGIMAGE*)lParam);
    }

    switch (uMsg)
    {
        HANDLE_MSG(plv, WM_CREATE, ListView_OnCreate);
        HANDLE_MSG(plv, WM_DESTROY, ListView_OnDestroy);
        HANDLE_MSG(plv, WM_ERASEBKGND, ListView_OnEraseBkgnd);
        HANDLE_MSG(plv, WM_COMMAND, ListView_OnCommand);
        HANDLE_MSG(plv, WM_SETFOCUS, ListView_OnSetFocus);
        HANDLE_MSG(plv, WM_KILLFOCUS, ListView_OnKillFocus);

        HANDLE_MSG(plv, WM_HSCROLL, ListView_OnHScroll);
        HANDLE_MSG(plv, WM_VSCROLL, ListView_OnVScroll);
        HANDLE_MSG(plv, WM_GETDLGCODE, ListView_OnGetDlgCode);
        HANDLE_MSG(plv, WM_SETFONT, ListView_OnSetFont);
        HANDLE_MSG(plv, WM_GETFONT, ListView_OnGetFont);
        HANDLE_MSG(plv, WM_TIMER, ListView_OnTimer);
        HANDLE_MSG(plv, WM_SETREDRAW, ListView_OnSetRedraw);
        HANDLE_MSG(plv, WM_NCDESTROY, ListView_OnNCDestroy);

    case WM_SETCURSOR:
        if (ListView_OnSetCursorMsg(plv))
            return TRUE;
        break;

    case WM_PALETTECHANGED:
        if ((HWND)wParam == hwnd)
            break;
    case WM_QUERYNEWPALETTE:
        // Want to pass FALSE if WM_QUERYNEWPALETTE...
        ListView_Realize(plv, NULL, uMsg == WM_PALETTECHANGED, uMsg == WM_PALETTECHANGED);
        return TRUE;

    case LVMP_WINDOWPOSCHANGED:
    case WM_WINDOWPOSCHANGED:
        HANDLE_WM_WINDOWPOSCHANGED(plv, wParam, lParam, ListView_OnWindowPosChanged);
        break;

    case WM_MBUTTONDOWN:
        if (ListView_SetFocus(hwnd) && plv->hwndToolTips)
            RelayToToolTips(plv->hwndToolTips, hwnd, uMsg, wParam, lParam);
        break;

    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        if (plv->hwndToolTips)
            RelayToToolTips(plv->hwndToolTips, hwnd, uMsg, wParam, lParam);
        ListView_OnButtonDown(plv, TRUE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT) wParam);
        break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        if (plv->hwndToolTips)
            RelayToToolTips(plv->hwndToolTips, hwnd, uMsg, wParam, lParam);
        ListView_OnButtonDown(plv, FALSE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT) wParam);
        break;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_NCMOUSEMOVE:
        if (plv->hwndToolTips)
            RelayToToolTips(plv->hwndToolTips, hwnd, uMsg, wParam, lParam);
        break;

    case WM_PRINTCLIENT:
    case WM_PAINT:
        ListView_OnPaint(plv, (HDC)wParam);
        return(0);

    case WM_SHOWWINDOW:
        LV_OnShowWindow(plv, BOOLFROMPTR(wParam));
        break;

    case WM_MOUSEHOVER:
        ListView_OnMouseHover(plv, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT) wParam);
        break;

    case WM_MOUSELEAVE:
        ListView_OnSetHotItem(plv, -1);
        plv->iNoHover = -1;

        break;

    case WM_KEYUP:
        plv->iScrollCount = 0;
        break;

    case WM_KEYDOWN:
        HANDLE_WM_KEYDOWN(plv, wParam, lParam, ListView_OnKey);
        break;

#if defined(FE_IME) || !defined(WINNT)
    case WM_IME_COMPOSITION:
        // Now only Korean version is interested in incremental search with composition string.
        if (g_fDBCSInputEnabled) {
        if (((ULONG_PTR)GetKeyboardLayout(0L) & 0xF000FFFFL) == 0xE0000412L)
        {
            if (ListView_OnImeComposition(plv, wParam, lParam))
            {
                lParam &= ~GCS_RESULTSTR;
                break;
            }
            else
                return 0;
        }
        }
        break;
#endif

    case WM_CHAR:
        if (plv->iPuntChar) {
            plv->iPuntChar--;
            return TRUE;
        } else {
            return HANDLE_WM_CHAR(plv, wParam, lParam, ListView_OnChar);
        }

    case WM_WININICHANGE:
        ListView_OnWinIniChange(plv, wParam, lParam);
        break;

    case WM_NOTIFYFORMAT:
        return CIHandleNotifyFormat(&plv->ci, lParam);

    case WM_ENABLE:
        // HACK: we don't get WM_STYLECHANGE on EnableWindow()
        ListView_EnableWindow(plv, BOOLFROMPTR(wParam));
        break;

    case WM_SYSCOLORCHANGE:
        InitGlobalColors();
        if (plv->ci.style & WS_DISABLED)
        {
            if (!(plv->flags & LVF_USERBKCLR))
                plv->clrBkSave = g_clrWindow;
            ListView_OnSetBkColor(plv, g_clrBtnFace);
        }
        else if (!(plv->flags & LVF_USERBKCLR))
        {
            ListView_OnSetBkColor(plv, g_clrWindow);
        }

        if (plv->exStyle & LVS_EX_CHECKBOXES)
        {
            ListView_InitCheckBoxes(plv, FALSE);
        }

//  98/11/19 #249967 vtan: Always invalidate the list view
//  rectangle so that the color change causes a refresh.

        InvalidateRect(plv->ci.hwnd, NULL, TRUE);
        break;

        // don't use HANDLE_MSG because this needs to go to the default handler
    case WM_SYSKEYDOWN:
        HANDLE_WM_SYSKEYDOWN(plv, wParam, lParam, ListView_OnKey);
        break;

#ifdef KEYBOARDCUES
    case WM_UPDATEUISTATE:
    {
        DWORD dwUIStateMask = MAKEWPARAM(0xFFFF, UISF_HIDEFOCUS);

        // we care only about focus not accel, and redraw only if changed
        if (CCOnUIState(&(plv->ci), WM_UPDATEUISTATE, wParam & dwUIStateMask, lParam))
        {
            if(plv->iFocus >= 0)
            {
                // an item has the focus, invalidate it
                ListView_InvalidateItem(plv, plv->iFocus, FALSE, RDW_INVALIDATE | RDW_ERASE);
            }
        }

        goto DoDefault;
    }
#endif
#ifdef UNICODE
    case LVM_GETITEMA:
        return (LRESULT)ListView_OnGetItemA(plv, (LV_ITEMA *)lParam);

    case LVM_SETITEMA:
        return (LRESULT)ListView_OnSetItemA(plv, (LV_ITEMA *)lParam);

    case LVM_INSERTITEMA:
        return (LRESULT)ListView_OnInsertItemA(plv, (LV_ITEMA *)lParam);

    case LVM_FINDITEMA:
        return (LRESULT)ListView_OnFindItemA(plv, (int)wParam, (LV_FINDINFOA *)lParam);

    case LVM_GETSTRINGWIDTHA:
        return (LRESULT)ListView_OnGetStringWidthA(plv, (LPCSTR)lParam, NULL);

    case LVM_GETCOLUMNA:
        return (LRESULT)ListView_OnGetColumnA(plv, (int)wParam, (LV_COLUMNA *)lParam);

    case LVM_SETCOLUMNA:
        return (LRESULT)ListView_OnSetColumnA(plv, (int)wParam, (LV_COLUMNA *)lParam);

    case LVM_INSERTCOLUMNA:
        return (LRESULT)ListView_OnInsertColumnA(plv, (int)wParam, (LV_COLUMNA *)lParam);

    case LVM_GETITEMTEXTA:
        return (LRESULT)ListView_OnGetItemTextA(plv, (int)wParam, (LV_ITEMA FAR *)lParam);

    case LVM_SETITEMTEXTA:
        if (!lParam)
            return FALSE;

        return (LRESULT)ListView_OnSetItemTextA(plv, (int)wParam,
                                                ((LV_ITEMA *)lParam)->iSubItem,
                                                (LPCSTR)((LV_ITEMA FAR *)lParam)->pszText);

    case LVM_GETBKIMAGEA:
        return (LRESULT)ListView_OnGetBkImageA(plv, (LPLVBKIMAGEA)lParam);

    case LVM_SETBKIMAGEA:
        return (LRESULT)ListView_OnSetBkImageA(plv, (LPLVBKIMAGEA)lParam);

#else

#ifdef DEBUG

    case LVM_GETITEMW:
    case LVM_SETITEMW:
    case LVM_INSERTITEMW:
    case LVM_FINDITEMW:
    case LVM_GETSTRINGWIDTHW:
    case LVM_GETCOLUMNW:
    case LVM_SETCOLUMNW:
    case LVM_INSERTCOLUMNW:
    case LVM_GETITEMTEXTW:
    case LVM_SETITEMTEXTW:
    case LVM_GETBKIMAGEW:
    case LVM_SETBKIMAGEW:
    case LVM_GETISEARCHSTRINGW:
    case LVM_EDITLABELW:
        break;
#endif

#endif
    case WM_STYLECHANGING:
        ListView_OnStyleChanging(plv, (UINT)wParam, (LPSTYLESTRUCT)lParam);
        return 0;

    case WM_STYLECHANGED:
        ListView_OnStyleChanged(plv, (UINT) wParam, (LPSTYLESTRUCT)lParam);
        return 0L;

    case WM_HELP:
        return ListView_OnHelp(plv, (LPHELPINFO)lParam);


    case LVM_GETIMAGELIST:
        return (LRESULT)(UINT_PTR)(ListView_OnGetImageList(plv, (int)wParam));

    case LVM_SETIMAGELIST:
        return (LRESULT)(UINT_PTR)ListView_OnSetImageList(plv, (HIMAGELIST)lParam, (int)wParam);

    case LVM_GETBKCOLOR:
        return (LRESULT)(plv->ci.style & WS_DISABLED ? plv->clrBkSave : plv->clrBk);

    case LVM_SETBKCOLOR:
        plv->flags |= LVF_USERBKCLR;
        if (plv->ci.style & WS_DISABLED) {
            plv->clrBkSave = (COLORREF)lParam;
            return TRUE;
        } else {
            return (LRESULT)ListView_OnSetBkColor(plv, (COLORREF)lParam);
        }

    case LVM_GETTEXTCOLOR:
        return (LRESULT)plv->clrText;
    case LVM_SETTEXTCOLOR:
        plv->clrText = (COLORREF)lParam;
        return TRUE;
    case LVM_GETTEXTBKCOLOR:
        return (LRESULT)plv->clrTextBk;
    case LVM_SETTEXTBKCOLOR:
        plv->clrTextBk = (COLORREF)lParam;
        return TRUE;
    case LVM_GETHOTLIGHTCOLOR:
        return (LRESULT)plv->clrHotlight;
    case LVM_SETHOTLIGHTCOLOR:
        plv->clrHotlight = (COLORREF)lParam;
        return(TRUE);

    case LVM_GETITEMCOUNT:
        if (ListView_IsOwnerData(plv))
            return((LRESULT)plv->cTotalItems);
        else if (!plv->hdpa)
            return(0);
        else
            return((LRESULT)DPA_GetPtrCount(plv->hdpa));
        break;

    case LVM_GETITEM:
        return (LRESULT)ListView_OnGetItem(plv, (LV_ITEM FAR*)lParam);

    case LVM_GETITEMSTATE:
        return (LRESULT)ListView_OnGetItemState(plv, (int)wParam, (UINT)lParam);

    case LVM_SETITEMSTATE:
        if (!lParam)
            return FALSE;

        return (LRESULT)ListView_OnSetItemState(plv, (int)wParam,
                                                ((LV_ITEM FAR *)lParam)->state,
                                                ((LV_ITEM FAR *)lParam)->stateMask);

    case LVM_SETITEMTEXT:
        if (!lParam)
            return FALSE;

        return (LRESULT)ListView_OnSetItemText(plv, (int)wParam,
                                                ((LV_ITEM FAR *)lParam)->iSubItem,
                                                (LPCTSTR)((LV_ITEM FAR *)lParam)->pszText);

    case LVM_GETITEMTEXT:
        return (LRESULT)ListView_OnGetItemText(plv, (int)wParam, (LV_ITEM FAR *)lParam);

    case LVM_GETBKIMAGE:
        return (LRESULT)ListView_OnGetBkImage(plv, (LPLVBKIMAGE)lParam);

    case LVM_SETBKIMAGE:
        return (LRESULT)ListView_OnSetBkImage(plv, (LPLVBKIMAGE)lParam);

    case LVM_SETITEM:
        return (LRESULT)ListView_OnSetItem(plv, (const LV_ITEM FAR*)lParam);

    case LVM_INSERTITEM:
        return (LRESULT)ListView_OnInsertItem(plv, (const LV_ITEM FAR*)lParam);

    case LVM_DELETEITEM:
        return (LRESULT)ListView_OnDeleteItem(plv, (int)wParam);

    case LVM_UPDATE:
        ListView_OnUpdate(plv, (int)wParam);
        UpdateWindow(plv->ci.hwnd);
        return TRUE;

    case LVM_DELETEALLITEMS:
        lParam = (LRESULT)ListView_OnDeleteAllItems(plv);
        // Optimization:  Instead of sending out a zillion EVENT_OBJECT_DESTROY's,
        // we send out a destroy of ourselves followed by a fresh create.
        // For compatibility with IE4, we still send out the REORDER notification.
        MyNotifyWinEvent(EVENT_OBJECT_REORDER, hwnd, OBJID_CLIENT, 0);
        ListView_NotifyRecreate(plv);
        return(lParam);

    case LVM_GETITEMRECT:
        return (LRESULT)ListView_OnGetItemRect(plv, (int)wParam, (RECT FAR*)lParam);

    case LVM_GETSUBITEMRECT:
        return (LRESULT)ListView_OnGetSubItemRect(plv, (int)wParam, (LPRECT)lParam);

    case LVM_SUBITEMHITTEST:
        return (LRESULT)ListView_OnSubItemHitTest(plv, (LPLVHITTESTINFO)lParam);

#ifdef UNICODE
    case LVM_GETISEARCHSTRINGA:
        if (GetFocus() == plv->ci.hwnd)
            return (LRESULT)GetIncrementSearchStringA(&plv->is, plv->ci.uiCodePage, (LPSTR)lParam);
        else
            return 0;

#endif

    case LVM_GETISEARCHSTRING:
        if (GetFocus() == plv->ci.hwnd)
            return (LRESULT)GetIncrementSearchString(&plv->is, (LPTSTR)lParam);
        else
            return 0;

    case LVM_GETITEMSPACING:
        if (wParam)
            return MAKELONG(plv->cxItem, plv->cyItem);
        else
            return MAKELONG(lv_cxIconSpacing, lv_cyIconSpacing);

    case LVM_GETNEXTITEM:
        return (LRESULT)ListView_OnGetNextItem(plv, (int)wParam, (UINT)lParam);

    case LVM_FINDITEM:
        return (LRESULT)ListView_OnFindItem(plv, (int)wParam, (const LV_FINDINFO FAR*)lParam);

    case LVM_SETSELECTIONMARK:
    {
        int iOldMark = plv->iMark;
        int iNewMark = (int)lParam;
        if (iNewMark == -1 || ListView_IsValidItemNumber(plv, iNewMark)) {
            plv->iMark = iNewMark;
        }
        return iOldMark;
    }

    case LVM_GETSELECTIONMARK:
        return plv->iMark;

    case LVM_GETITEMPOSITION:
        return (LRESULT)ListView_OnGetItemPosition(plv, (int)wParam,
                (POINT FAR*)lParam);

    case LVM_SETITEMPOSITION:
        return (LRESULT)ListView_OnSetItemPosition(plv, (int)wParam,
                GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

    case LVM_SETITEMPOSITION32:
        if (!lParam)
            return FALSE;

        return (LRESULT)ListView_OnSetItemPosition(plv, (int)wParam,
                ((LPPOINT)lParam)->x, ((LPPOINT)lParam)->y);

    case LVM_SCROLL:
    {
        int dx = (int)wParam;
        int dy = (int)lParam;
        return (LRESULT)
            (ListView_ValidateScrollParams(plv, &dx, &dy) &&
             ListView_OnScroll(plv, dx, dy));
    }

    case LVM_ENSUREVISIBLE:
        return (LRESULT)ListView_OnEnsureVisible(plv, (int)wParam, BOOLFROMPTR(lParam));

    case LVM_REDRAWITEMS:
        return (LRESULT)ListView_OnRedrawItems(plv, (int)wParam, (int)lParam);

    case LVM_ARRANGE:
        return (LRESULT)ListView_OnArrange(plv, (UINT)wParam);

    case LVM_GETEDITCONTROL:
        return (LRESULT)(UINT_PTR)plv->hwndEdit;

#ifdef UNICODE
    case LVM_EDITLABELA:
        {
        LPWSTR lpEditString = NULL;
        HWND   hRet;

        if (lParam) {
            lpEditString = ProduceWFromA(plv->ci.uiCodePage, (LPSTR)lParam);
        }

        hRet = ListView_OnEditLabel(plv, (int)wParam, lpEditString);

        if (lpEditString) {
            FreeProducedString(lpEditString);
        }

        return (LRESULT)hRet;
        }
#endif

    case LVM_EDITLABEL:
        return (LRESULT)(UINT_PTR)ListView_OnEditLabel(plv, (int)wParam, (LPTSTR)lParam);

    case LVM_HITTEST:
        return (LRESULT)ListView_OnHitTest(plv, (LV_HITTESTINFO FAR*)lParam);

    case LVM_GETSTRINGWIDTH:
        return (LRESULT)ListView_OnGetStringWidth(plv, (LPCTSTR)lParam, NULL);

    case LVM_GETCOLUMN:
        return (LRESULT)ListView_OnGetColumn(plv, (int)wParam, (LV_COLUMN FAR*)lParam);

    case LVM_SETCOLUMN:
        return (LRESULT)ListView_OnSetColumn(plv, (int)wParam, (const LV_COLUMN FAR*)lParam);

    case LVM_SETCOLUMNORDERARRAY:
        return SendMessage(plv->hwndHdr, HDM_SETORDERARRAY, wParam, lParam);

    case LVM_GETCOLUMNORDERARRAY:
        return SendMessage(plv->hwndHdr, HDM_GETORDERARRAY, wParam, lParam);

    case LVM_GETHEADER:
    {
        HWND hwndOld = plv->hwndHdr;
        if (lParam && IsWindow((HWND)lParam)) {
            plv->hwndHdr = (HWND)lParam;
        }
        return (LRESULT)hwndOld;
    }

    case LVM_INSERTCOLUMN:
        return (LRESULT)ListView_OnInsertColumn(plv, (int)wParam, (const LV_COLUMN FAR*)lParam);

    case LVM_DELETECOLUMN:
        return (LRESULT)ListView_OnDeleteColumn(plv, (int)wParam);

    case LVM_CREATEDRAGIMAGE:
        return (LRESULT)(UINT_PTR)ListView_OnCreateDragImage(plv, (int)wParam, (LPPOINT)lParam);


    case LVMI_PLACEITEMS:
        if (plv->uUnplaced) {
            ListView_Recompute(plv);
            ListView_UpdateScrollBars(plv);
        }
        return 0;

    case LVM_GETVIEWRECT:
        if (!lParam)
            return FALSE;

        ListView_GetViewRect2(plv, (RECT FAR*)lParam, plv->sizeClient.cx, plv->sizeClient.cy);
        return (LPARAM)TRUE;

    case LVM_GETCOLUMNWIDTH:
        return (LPARAM)ListView_OnGetColumnWidth(plv, (int)wParam);

    case LVM_SETCOLUMNWIDTH:
        return (LPARAM)ListView_ISetColumnWidth(plv, (int)wParam,
            GET_X_LPARAM(lParam), TRUE);

    case LVM_SETCALLBACKMASK:
        plv->stateCallbackMask = (UINT)wParam;
        return (LPARAM)TRUE;

    case LVM_GETCALLBACKMASK:
        return (LPARAM)(UINT)plv->stateCallbackMask;

    case LVM_GETTOPINDEX:
        return (LPARAM)ListView_OnGetTopIndex(plv);

    case LVM_GETCOUNTPERPAGE:
        return (LPARAM)ListView_OnGetCountPerPage(plv);

    case LVM_GETORIGIN:
        return (LPARAM)ListView_OnGetOrigin(plv, (POINT FAR*)lParam);

    case LVM_SETITEMCOUNT:
        return ListView_OnSetItemCount(plv, (int)wParam, (DWORD)lParam);

    case LVM_GETSELECTEDCOUNT:
        if (ListView_IsOwnerData( plv )) {
            plv->plvrangeSel->lpVtbl->CountIncluded(plv->plvrangeSel, &plv->nSelected);
        }

        return plv->nSelected;

    case LVM_SORTITEMS:
        return ListView_OnSortItems(plv, (LPARAM)wParam, (PFNLVCOMPARE)lParam, TRUE);

    case LVM_SORTITEMSEX:
        return ListView_OnSortItems(plv, (LPARAM)wParam, (PFNLVCOMPARE)lParam, FALSE);

    case LVM_SETEXTENDEDLISTVIEWSTYLE:
        return ListView_ExtendedStyleChange(plv, (DWORD) lParam, (DWORD) wParam);

    case LVM_GETEXTENDEDLISTVIEWSTYLE:
        return plv->exStyle;

    case LVM_GETHOVERTIME:
        return plv->dwHoverTime;

    case LVM_SETHOVERTIME:
    {
        DWORD dwRet = plv->dwHoverTime;
        plv->dwHoverTime = (DWORD)lParam;
        return dwRet;
    }

    case LVM_GETTOOLTIPS:
        return (LRESULT)plv->hwndToolTips;

    case LVM_SETTOOLTIPS:
    {
        HWND hwndToolTips = plv->hwndToolTips;
        plv->hwndToolTips = (HWND)wParam;
        return (LRESULT)hwndToolTips;
    }

    case LVM_SETICONSPACING:
    {
        DWORD dwRet = ListView_OnSetIconSpacing(plv, lParam);

        // rearrange as necessary
        if (ListView_RedrawEnabled(plv) &&
            ((plv->ci.style & LVS_AUTOARRANGE) &&
             (ListView_IsSmallView(plv) || ListView_IsIconView(plv))))
        {
            // Call off to the arrange function.
            ListView_OnArrange(plv, LVA_DEFAULT);
        }
        return dwRet;
    }

    case LVM_SETHOTITEM:
    {
        int iOld = plv->iHot;
        int iNew = (int)wParam;
        if (iNew == -1 || ListView_IsValidItemNumber(plv, iNew)) {
            ListView_OnSetHotItem(plv, (int)wParam);
        }
        return iOld;
    }

    case LVM_GETHOTITEM:
        return plv->iHot;

    // hCurHot is used iff LVS_EX_TRACKSELECT
    case LVM_SETHOTCURSOR:
    {
        HCURSOR hCurOld = plv->hCurHot;
        plv->hCurHot = (HCURSOR)lParam;
        return (LRESULT)hCurOld;
    }

    case LVM_GETHOTCURSOR:
        if (!plv->hCurHot)
            plv->hCurHot = LoadHandCursor(0);
        return (LRESULT)plv->hCurHot;

    case LVM_APPROXIMATEVIEWRECT:
        return ListView_OnApproximateViewRect(plv, (int)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

    case LVM_SETLVRANGEOBJECT:
        return ListView_OnSetLVRangeObject(plv, (int)wParam, (ILVRange *)lParam);

#ifdef LVM_SETWORKAREAS  // until the headers are in ssync.....
    case LVM_SETWORKAREAS:
        ListView_OnSetWorkAreas(plv, (int)wParam, (RECT FAR *)lParam);
        return 0;

    case LVM_GETWORKAREAS:
        ListView_OnGetWorkAreas(plv, (int)wParam, (RECT FAR *)lParam);
        return 0;

    case LVM_GETNUMBEROFWORKAREAS:
        ListView_OnGetNumberOfWorkAreas(plv, (int *)lParam);
        return 0;

    case LVM_RESETEMPTYTEXT:
        plv->fNoEmptyText = FALSE;
        Str_Set(&plv->pszEmptyText, NULL);
        if (ListView_Count(plv) == 0)
            InvalidateRect(plv->ci.hwnd, NULL, TRUE);
        return 1;
#endif
    case WM_SIZE:
        if (plv)
        {
            if (plv->hwndToolTips) {
                TOOLINFO ti;

                if (ListView_IsLabelTip(plv))
                {
                    // A truncated label may have been exposed or vice versa.
                    ListView_InvalidateTTLastHit(plv, plv->iTTLastHit);
                }

                ti.cbSize = sizeof(ti);
                ti.hwnd = plv->ci.hwnd;
                ti.uId = 0;

                // Resize the tooltip control so that it covers the entire
                // area of the window when its parent gets resized.

                GetClientRect( plv->ci.hwnd, &ti.rect );
                SendMessage( plv->hwndToolTips, TTM_NEWTOOLRECT, 0, (LPARAM) &ti );
            }
            // if we're supposed to center the image,
            // we need to do a full redraw on each size
            if ((plv->ulBkImageFlags & LVBKIF_SOURCE_MASK) &&
                (plv->xOffsetPercent || plv->yOffsetPercent)) {
                InvalidateRect(plv->ci.hwnd, NULL, TRUE);
            }

        }
        break;

    case WM_NOTIFY:
        return ListView_OnNotify(plv, wParam, (LPNMHDR)lParam);


    case WM_MOUSEMOVE:
        if (plv->hwndToolTips)
        {
            UINT uFlags;
            int iHit, iSubHit;

            RelayToToolTips(plv->hwndToolTips, hwnd, uMsg, wParam, lParam);

            // check that we are still on the hit item, pop it!
            iHit = _ListView_ItemHitTest( plv, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &uFlags, &iSubHit );

            if (iHit != plv->iTTLastHit || iSubHit != plv->iTTLastSubHit)
                ListView_PopBubble(plv);
        }

        ListView_OnMouseMove(plv, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT) wParam);
        break;

    case WM_GETOBJECT:
        if( lParam == OBJID_QUERYCLASSNAMEIDX )
            return MSAA_CLASSNAMEIDX_LISTVIEW;
        break;

    default:
        {
            LRESULT lres;
            if (CCWndProc(&plv->ci, uMsg, wParam, lParam, &lres))
                return lres;
        }

        // Special handling of magellan mouse message
        if (uMsg == g_msgMSWheel) {
            BOOL            fScroll;
            BOOL            fDataZoom;
            DWORD           dwStyle;
            int             sb;
            SCROLLINFO      si;
            int             cScrollUnitsPerLine;
            int             cPage;
            int             cLinesPerDetant;
            int             cDetants;
            int             dPos;
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
                if (g_ucScrollLines > 0 &&
                    cDetants != 0 &&
                    ((WS_VSCROLL | WS_HSCROLL) & (dwStyle = ListView_GetWindowStyle(plv)))) {

                    sb = (dwStyle & WS_VSCROLL) ? SB_VERT : SB_HORZ;

                    // Get the scroll amount of one line
                    cScrollUnitsPerLine = _ListView_GetScrollUnitsPerLine(plv, sb);
                    ASSERT(cScrollUnitsPerLine > 0);

                    si.cbSize = sizeof(SCROLLINFO);
                    si.fMask = SIF_PAGE | SIF_POS;
                    if (!ListView_GetScrollInfo(plv, sb, &si))
                        return 1;

                    // The size of a page is at least one line, and
                    // leaves one line of overlap
                    cPage = (max(cScrollUnitsPerLine, (int)si.nPage - cScrollUnitsPerLine)) / cScrollUnitsPerLine;

                    // Don't scroll more than one page per detant
                    cLinesPerDetant = (int) min((ULONG) cPage, (ULONG) g_ucScrollLines);

                    dPos = cLinesPerDetant * cDetants * cScrollUnitsPerLine;

                    ListView_DismissEdit(plv, FALSE);
                    ListView_ComOnScroll(
                            plv, SB_THUMBTRACK, si.nPos + dPos, sb, cScrollUnitsPerLine, - 1);
                    ListView_UpdateScrollBars(plv);

                    // After scrolling, the tooltip might need to change
                    // so send the tooltip a fake mousemove message to force
                    // a recompute.  We use WM_NCMOUSEMOVE since our lParam
                    // is in screen coordinates, not client coordinates.
                    ListView_PopBubble(plv);
                    RelayToToolTips(plv->hwndToolTips, plv->ci.hwnd,
                                    WM_NCMOUSEMOVE, HTCLIENT, lParam);
                }
                return 1;
            } else if (fDataZoom) {
                LV_HITTESTINFO ht;
                ht.pt.x = GET_X_LPARAM(lParam);
                ht.pt.y = GET_Y_LPARAM(lParam);
                ScreenToClient(hwnd, &(ht.pt));

                // If we are rolling forward and we hit an item then navigate
                // into that item (simulate dblclk which will open it).  Otherwise
                // just fall through so it isn't handled.  In that case if we
                // are being hosted in explorer it will do a backwards
                // history navigation.
                if ((iWheelDelta > 0) && (ListView_OnSubItemHitTest(plv, &ht) >= 0) &&
                    (ht.flags & LVHT_ONITEM) && cDetants != 0) {
                    BYTE aKeyState[256];
                    // This is a bit yucky but when ListView_HandleMouse sends the
                    // notification to the listview owner we need to make sure that
                    // it doesn't think the shift key is down.  Otherwise it may
                    // perform some "alternate" action but in this case we always
                    // want it to perform the default open action.
                    //
                    // Strip the high bit of VK_SHIFT so that the shift key is
                    // not down.
                    GetKeyboardState(aKeyState);
                    aKeyState[VK_SHIFT] &= 0x7f;
                    SetKeyboardState(aKeyState);
                    ListView_HandleMouse(plv, FALSE, ht.pt.x, ht.pt.y, 0, TRUE);
                    ListView_HandleMouse(plv, TRUE, ht.pt.x, ht.pt.y, 0, TRUE);
                    return 1;
                }
                // else fall through
            }
        }

        break;
    }

DoDefault:
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void NEAR ListView_OnWinIniChange(LV* plv, WPARAM wParam, LPARAM lParam)
{
    // BUGBUG:  will this also catch sysparametersinfo?
    // we need a general way of handling this, not
    // just relying on the listview.
    InitGlobalMetrics(wParam);

    switch (wParam) {
        case 0:
        case SPI_SETNONCLIENTMETRICS:
        case SPI_SETICONTITLELOGFONT:
        case SPI_SETICONMETRICS:
            // If wParam is 0, only reload settings if lParam is 0 too.  This catches the wild-card scenario
            // (like the old plus tab which does WM_WININICHANGE, 0, 0) but allows us to ignore wParam = 0
            // and lParam = lpszSectionName.  Reduces unecessary flashing.
            if (wParam || !lParam)
            {
                if (!(plv->flags & LVF_ICONSPACESET))
                    ListView_OnSetIconSpacing(plv, (LPARAM)-1);

                if (plv->flags & LVF_FONTCREATED)
                    ListView_OnSetFont(plv, NULL, TRUE);

                // Force a recalc of all the icon regions by stripping and
                // then adding back the LVS_EX_REGIONAL bit.
                if (plv->exStyle & LVS_EX_REGIONAL) {
                    ListView_ExtendedStyleChange(plv, 0, LVS_EX_REGIONAL);
                    ListView_ExtendedStyleChange(plv, LVS_EX_REGIONAL, LVS_EX_REGIONAL);
                }
            }
            break;

        default:
            break;
    }

    // If we are in an Iconic view and the user is in autoarrange mode,
    // then we need to arrange the items.
    //
    if ((ListView_IsOwnerData( plv ) || (plv->ci.style & LVS_AUTOARRANGE)) &&
        (ListView_IsSmallView(plv) || ListView_IsIconView(plv)))
    {
        // Call off to the arrange function.
        ListView_OnArrange(plv, LVA_DEFAULT);
    }
}

BOOL NEAR ListView_OnCreate(LV* plv, CREATESTRUCT FAR* lpCreateStruct)
{
#ifdef WINNT
    InitDitherBrush();
#endif


    CIInitialize(&plv->ci, plv->ci.hwnd, lpCreateStruct);

#ifdef DEBUG
    if (GetAsyncKeyState(VK_SHIFT) < 0 &&
        GetAsyncKeyState(VK_CONTROL) < 0) {
        //plv->exStyle |= LVS_EX_SUBITEMIMAGES;
        plv->exStyle |= LVS_EX_FULLROWSELECT;
        plv->ci.style |= LVS_SHOWSELALWAYS;
        SetWindowLong(plv->ci.hwnd, GWL_STYLE, plv->ci.style);
    }
#endif

    plv->dwExStyle = lpCreateStruct->dwExStyle;

    if (plv->ci.style & WS_VISIBLE)
        plv->flags |= LVF_VISIBLE;

    ListView_GetRegIASetting(&g_bUseDblClickTimer);

    if (ListView_IsOwnerData(plv))
    {
        // ownerdata initialization
        plv->plvrangeSel = LVRange_Create();
        if (NULL == plv->plvrangeSel)
           goto error0;

       plv->plvrangeCut = LVRange_Create();
       if (NULL == plv->plvrangeCut)
          goto error0;
    }
    else
    {
        ASSERT(plv->plvrangeSel == NULL);
        ASSERT(plv->plvrangeCut == NULL);

        plv->hdpa = DPA_CreateEx(LV_HDPA_GROW, plv->hheap);
        if (!plv->hdpa)
            goto error0;

        plv->hdpaZOrder = DPA_CreateEx(LV_HDPA_GROW, plv->hheap);
        if (!plv->hdpaZOrder)
            goto error1;
    }

    ASSERT(plv->nWorkAreas == 0);
    ASSERT(plv->prcWorkAreas == NULL);
    plv->iNoHover = -1;
    plv->dwHoverTime = HOVER_DEFAULT;
    plv->iHot = -1;
    plv->iEdit = -1;
    plv->iFocus = -1;
    plv->iDrag = -1;
    plv->iTTLastHit = -1;
    plv->iFreeSlot = -1;
    plv->rcView.left = RECOMPUTE;
    ASSERT(plv->iMSAAMin == plv->iMSAAMax);

    plv->sizeClient.cx = lpCreateStruct->cx;
    plv->sizeClient.cy = lpCreateStruct->cy;

    // Setup flag to say if positions are in small or large view
    if (ListView_IsSmallView(plv))
        plv->flags |= LVF_ICONPOSSML;

    // force calculation of listview metrics
    ListView_OnSetFont(plv, NULL, FALSE);

    plv->cxItem = 16 * plv->cxLabelChar + plv->cxSmIcon;

    // if we're in ownerdraw report mode, the size got saved to cyItemSave
    // at creation time, both need to have this
    if ((plv->ci.style & LVS_OWNERDRAWFIXED) && ListView_IsReportView(plv))
        plv->cyItem = plv->cyItemSave;
    else
        plv->cyItemSave = plv->cyItem;

    ListView_OnSetIconSpacing(plv, (LPARAM)-1);

    ListView_UpdateScrollBars(plv);     // sets plv->cItemCol

    plv->clrBk = CLR_NONE;
    plv->clrText = CLR_DEFAULT;
    plv->clrTextBk = CLR_DEFAULT;
    plv->clrHotlight = CLR_DEFAULT;

    // create the bk brush, and set the imagelists colors if needed
    ListView_OnSetBkColor(plv, g_clrWindow);

    // Initialize report view fields
    plv->xTotalColumnWidth = RECOMPUTE;

    if (ListView_IsReportView(plv))
        ListView_RInitialize(plv, FALSE);

    if (plv->ci.style & WS_DISABLED) {
        plv->ci.style &= ~WS_DISABLED;
        ListView_EnableWindow(plv, FALSE);
    }

    // tooltip for unfolding name lables

    plv->hwndToolTips = CreateWindow(TOOLTIPS_CLASS, NULL,
                                     WS_POPUP|TTS_NOPREFIX, 0, 0, 0, 0,
                                     NULL, NULL, g_hinst, NULL);
    if ( plv->hwndToolTips )
    {
        TOOLINFO ti;

        ti.cbSize = sizeof(ti);
        ti.uFlags = TTF_TRANSPARENT;
        ti.hwnd = plv->ci.hwnd;
        ti.uId = 0;
        ti.hinst = NULL;
        ti.lpszText = LPSTR_TEXTCALLBACK;

        GetClientRect( plv->ci.hwnd, &ti.rect );
        SendMessage( plv->hwndToolTips, TTM_ADDTOOL, 0, (LPARAM) &ti );

        /* Ensure that the tooltips use the same font as the view */
        FORWARD_WM_SETFONT(plv->hwndToolTips, plv->hfontLabel, FALSE, SendMessage);
    }
    ASSERT(plv->hwndToolTips);

    ASSERT(FALSE == plv->fOneClickOK);
    SetTimer(plv->ci.hwnd, IDT_ONECLICKOK, GetDoubleClickTime(), NULL);

    return TRUE;

error1:
    DPA_Destroy(plv->hdpa);
error0:
    if ( plv->plvrangeSel )
        plv->plvrangeSel->lpVtbl->Release( plv->plvrangeSel );
    if ( plv->plvrangeCut)
        plv->plvrangeCut->lpVtbl->Release( plv->plvrangeCut );
    return FALSE;
}

void NEAR PASCAL ListView_DeleteHrgnInval(LV* plv)
{
    if (plv->hrgnInval && plv->hrgnInval != (HRGN)ENTIRE_REGION)
        DeleteObject(plv->hrgnInval);
    plv->hrgnInval = NULL;
}

void NEAR ListView_OnDestroy(LV* plv)
{
    //
    // The tooltip window may or may not exist at this point.  It
    // depends if the owning window of the tips is also being destroy.
    // If so, then the tips are gone already.
    //

    if (IsWindow(plv->hwndToolTips))
        DestroyWindow(plv->hwndToolTips);

    if (plv->hCurHot)
        DestroyCursor(plv->hCurHot);

    plv->hwndToolTips = NULL;

    Str_Set(&plv->pszTip, NULL);
    Str_Set(&plv->pszEmptyText, NULL);

#ifdef WINNT
    TerminateDitherBrush();
#endif

    if (!ListView_IsOwnerData(plv)) {
       // Make sure to notify the app
       ListView_OnDeleteAllItems(plv);
    }

    if ((plv->flags & LVF_FONTCREATED) && plv->hfontLabel) {
        DeleteObject(plv->hfontLabel);
        // plv->flags &= ~LVF_FONTCREATED;
        // plv->hwfontLabel = NULL;
    }
    if (plv->hFontHot)
        DeleteObject(plv->hFontHot);
    ListView_DeleteHrgnInval(plv);

    if (plv->prcWorkAreas)
    {
        // This assert is bogus: If the app created work areas then deleted
        // them, nWorkAreas will be 0 but prcWorkAreas will be non-NULL.
        // ASSERT(plv->nWorkAreas > 0);
        LocalFree(plv->prcWorkAreas);
    }
}

void NEAR ListView_OnNCDestroy(LV* plv)
{

    if ((!(plv->ci.style & LVS_SHAREIMAGELISTS)) || ListView_CheckBoxes(plv)) {

        if (plv->himlState &&
            (plv->himlState != plv->himl) &&
            (plv->himlState != plv->himlSmall))
        {
            ImageList_Destroy(plv->himlState);
        }
    }

    if (!(plv->ci.style & LVS_SHAREIMAGELISTS))
    {
        if (plv->himl)
            ImageList_Destroy(plv->himl);
        if (plv->himlSmall)
            ImageList_Destroy(plv->himlSmall);
    }

    if (ListView_IsOwnerData(plv)) {
        plv->plvrangeSel->lpVtbl->Release( plv->plvrangeSel );
        plv->plvrangeCut->lpVtbl->Release( plv->plvrangeCut );
        plv->cTotalItems = 0;
    }

    ListView_ReleaseBkImage(plv);

    if (plv->hbrBk)
        DeleteBrush(plv->hbrBk);

    if (plv->hdpa)
        DPA_Destroy(plv->hdpa);

    if (plv->hdpaZOrder)
        DPA_Destroy(plv->hdpaZOrder);

    ListView_RDestroy(plv);

    IncrementSearchFree(&plv->is);

    ListView_SetPtr(plv->ci.hwnd, NULL);
    NearFree(plv);
}


// sets the background color for the listview
//
// this creats the brush for drawing the background as well
// as sets the imagelists background color if needed

BOOL NEAR ListView_OnSetBkColor(LV* plv, COLORREF clrBk)
{
    if (plv->clrBk != clrBk)
    {
        if (plv->hbrBk)
        {
            DeleteBrush(plv->hbrBk);
            plv->hbrBk = NULL;
        }

        if (clrBk != CLR_NONE)
        {
            plv->hbrBk = CreateSolidBrush(clrBk);
            if (!plv->hbrBk)
                return FALSE;
        }

        // don't mess with the imagelist color if things are shared

        if (!(plv->ci.style & LVS_SHAREIMAGELISTS)) {

            if (plv->himl)
                ImageList_SetBkColor(plv->himl, clrBk);

            if (plv->himlSmall)
                ImageList_SetBkColor(plv->himlSmall, clrBk);

            if (plv->himlState)
                ImageList_SetBkColor(plv->himlState, clrBk);
        }

        plv->clrBk = clrBk;
    }
    return TRUE;
}

void PASCAL InitBrushOrg(LV* plv, HDC hdc)
{
    int x;
    if (ListView_IsSmallView(plv) || ListView_IsIconView(plv)) {
        x = plv->ptOrigin.x;
    } else if (ListView_IsListView(plv)) {
        x = plv->xOrigin;
    } else {
        x = (int)plv->ptlRptOrigin.x;
    }
    SetBrushOrgEx(hdc, -x, 0, NULL);
}

void NEAR PASCAL ListView_InvalidateRegion(LV* plv, HRGN hrgn)
{
    if (hrgn) {
        if (plv->hrgnInval == NULL) {
            plv->hrgnInval = hrgn;
        } else {

            // union it in if the entire region isn't marked for invalidate
            if (plv->hrgnInval != (HRGN)ENTIRE_REGION) {
                UnionRgn(plv->hrgnInval, plv->hrgnInval, hrgn);
            }
            DeleteObject(hrgn);
        }
    }
}


// ----------------------------------------------------------------------------
//
//  LVSeeThruScroll
//
//  Used when a watermark is the listview's background (detected via clrTextBk
//  being CLR_NONE) to perform a flicker-free scroll of the client area, using
//  an offscreen bitmap
//
//  potential perf issue -- caching DC and/or bitmap instead of create/destroy
//                          on each call
//
//  jeffbog 2/29/96
//
// ----------------------------------------------------------------------------

void LVSeeThruScroll(LV *plv, LPRECT lprcUpdate)
{
    HDC     hdcOff;
    HBITMAP hbmpOff;
    int     x,y,cx,cy;
    HDC     hdc = GetDC(plv->ci.hwnd);

    if (!lprcUpdate)
    {
        x = y = 0;
        cx = plv->sizeClient.cx;
        cy = plv->sizeClient.cy;
    }
    else
    {
        x  = lprcUpdate->left;
        y  = lprcUpdate->top;
        cx = lprcUpdate->right - x;
        cy = lprcUpdate->bottom - y;
    }

    hdcOff  = CreateCompatibleDC(hdc);
    hbmpOff = CreateCompatibleBitmap(hdc, plv->sizeClient.cx, plv->sizeClient.cy);
    SelectObject(hdcOff, hbmpOff);

    SendMessage(plv->ci.hwnd, WM_PRINT, (WPARAM)hdcOff, PRF_CLIENT | PRF_ERASEBKGND);
    BitBlt(hdc, x, y, cx, cy, hdcOff, x, y, SRCCOPY);
    ReleaseDC(plv->ci.hwnd, hdc);
    DeleteDC(hdcOff);
    DeleteObject(hbmpOff);
}

void NEAR ListView_OnPaint(LV* plv, HDC hdc)
{
    PAINTSTRUCT ps;
    RECT rcUpdate;


    // Before handling WM_PAINT, go ensure everything's recomputed...
    //
    if (plv->rcView.left == RECOMPUTE)
        ListView_Recompute(plv);

    // If we're in report view, update the header window: it looks
    // better this way...
    //
    if (ListView_IsReportView(plv) && plv->hwndHdr)
        UpdateWindow(plv->hwndHdr);

    // If nothing to do (i.e., we recieved a WM_PAINT because
    // of an RDW_INTERNALPAINT, and we didn't invalidate anything)
    // don't bother with the Begin/EndPaint.
    //
    if (hdc || GetUpdateRect(plv->ci.hwnd, &rcUpdate, FALSE))
    {
        if (!(plv->flags & LVF_VISIBLE)) {
            plv->flags |= LVF_VISIBLE;
            // We may try to resize the column
            ListView_MaybeResizeListColumns(plv, 0, ListView_Count(plv)-1);
            ListView_UpdateScrollBars(plv);
        }

        // this needs to be done before the beginpaint because it clears
        // out the update region
        if (!(plv->flags & LVF_REDRAW)) {
            // add this region to our local invalidate region
            HRGN hrgn = CreateRectRgn(0, 0, 0,0);
            if (hrgn) {

                // ok if GetUpdateRgn fails... then hrgn will still be
                // and empty region..
                GetUpdateRgn(plv->ci.hwnd, hrgn, FALSE);
                ListView_InvalidateRegion(plv, hrgn);
            }
        }

        if (hdc)
        {
            InitBrushOrg(plv, hdc);
            SetRect(&ps.rcPaint, 0, 0, plv->sizeClient.cx, plv->sizeClient.cy);
            if (ListView_RedrawEnabled(plv))
                ListView_Redraw(plv, hdc, &ps.rcPaint);
        }
        else
        {
            hdc = BeginPaint(plv->ci.hwnd, &ps);
            InitBrushOrg(plv, hdc);
            if (ListView_RedrawEnabled(plv))
                ListView_Redraw(plv, hdc, &ps.rcPaint);
            EndPaint(plv->ci.hwnd, &ps);
        }
    }
}

void ListView_DrawSimpleBackground(LV *plv, HDC hdc, RECT *prcClip)
{
    if (plv->clrBk != CLR_NONE)
    {
        //
        // We just have a simple background color.
        //
        FillRect(hdc, prcClip, plv->hbrBk);
    }
    else
    {
        //
        // Parent HWND draws the background for us.
        //
        SendMessage(plv->ci.hwndParent, WM_ERASEBKGND, (WPARAM)hdc, 0);
    }
}

void ListView_DrawBackground(LV *plv, HDC hdc, RECT *prcClip)
{
    HRGN hrgnClipSave;
    RECT rcClip;

    // Optimize the common/simple case
    if (!(plv->pImgCtx && plv->fImgCtxComplete))
    {
        ListView_DrawSimpleBackground(plv, hdc, prcClip);
        return;
    }

    //
    // Save the old clipping region,
    // since we whack on it a lot.
    //
    hrgnClipSave = CreateRectRgnIndirect(prcClip);
    if (hrgnClipSave)
    {
        if (GetClipRgn(hdc, hrgnClipSave) <= 0)
        {
            DeleteObject(hrgnClipSave);
            hrgnClipSave = NULL;
        }
    }

    //
    // Clip the clipping region to the caller's rectangle,
    // and save the final clipping rectangle in rcClip.
    //
    if (prcClip != NULL)
    {
        IntersectClipRect(hdc, prcClip->left, prcClip->top,
                               prcClip->right, prcClip->bottom);
    }
    GetClipBox(hdc, &rcClip);

    //
    // If we have an image to draw, go draw it and
    // exclue it from the clipping region.
    //
    if (plv->pImgCtx && plv->fImgCtxComplete)
    {
        RECT rcImage, rcClient;
        ULONG ulState;
        SIZE sizeImg;
        POINT ptBackOrg;

        //
        // Compute ptBackOrg (aka scrolling offset), based on view style.
        //
        switch (plv->ci.style & LVS_TYPEMASK)
        {
            case LVS_LIST:
                ptBackOrg.x = -plv->xOrigin;
                ptBackOrg.y = 0;
                break;

            case LVS_REPORT:
                ptBackOrg.x = -plv->ptlRptOrigin.x;
                ptBackOrg.y = -plv->ptlRptOrigin.y + plv->yTop;
                break;

            default:
                ptBackOrg.x = -plv->ptOrigin.x;
                ptBackOrg.y = -plv->ptOrigin.y;
                break;
        }

        ListView_Realize(plv, hdc, TRUE, FALSE);

        switch (plv->ulBkImageFlags & LVBKIF_STYLE_MASK)
        {
        case LVBKIF_STYLE_TILE:
            IImgCtx_Tile(plv->pImgCtx, hdc, &ptBackOrg, prcClip, NULL);
            ExcludeClipRect(hdc, prcClip->left, prcClip->top,
                                 prcClip->right, prcClip->bottom);
            break;

#if 0
        case LVBKIF_STYLE_STRETCH:
            //
            // Stretch the image to the extents of the client & view areas.
            //
            GetClientRect(plv->ci.hwnd, &rcImage);
            UnionRect(&rcImage, &rcImage, &plv->rcView);

            //
            // Adjust for ptBackOrg (scrolling offset).
            //
            rcImage.left += ptBackOrg.x;
            rcImage.top += ptBackOrg.y;
            rcImage.right += ptBackOrg.x;
            rcImage.bottom += ptBackOrg.y;

            //
            // Draw the image, if necessary.
            //
            if (RectVisible(hdc, &rcImage))
            {
                IImgCtx_Draw(plv->pImgCtx, hdc, &rcImage);
                ExcludeClipRect(hdc, prcClip->left, prcClip->top,
                                     prcClip->right, prcClip->bottom);
            }
            break;
#endif
        case LVBKIF_STYLE_NORMAL:
            //
            // Start with the base image.
            //
            IImgCtx_GetStateInfo(plv->pImgCtx, &ulState, &sizeImg, FALSE);
            rcImage.left = 0;
            rcImage.top = 0;
            rcImage.right = sizeImg.cx;
            rcImage.bottom = sizeImg.cy;

            //
            // Adjust for caller offsets.
            //
            GetClientRect(plv->ci.hwnd, &rcClient);
            if (plv->xOffsetPercent)
            {
                LONG dx = plv->xOffsetPercent * (rcClient.right - sizeImg.cx) / 100;

                rcImage.left += dx;
                rcImage.right += dx;
            }
            if (plv->yOffsetPercent)
            {
                LONG dy = plv->yOffsetPercent * (rcClient.bottom - sizeImg.cy) / 100;

                rcImage.top += dy;
                rcImage.bottom += dy;
            }

            //
            // Adjust for ptBackOrg (scrolling offset).
            //
            rcImage.left += ptBackOrg.x;
            rcImage.top += ptBackOrg.y;
            rcImage.right += ptBackOrg.x;
            rcImage.bottom += ptBackOrg.y;

            //
            // Draw the image, if necessary.
            //
            if (RectVisible(hdc, &rcImage))
            {
                IImgCtx_Draw(plv->pImgCtx, hdc, &rcImage);
                ExcludeClipRect(hdc, rcImage.left, rcImage.top,
                                     rcImage.right, rcImage.bottom);
            }
            break;
        }
    }

    //
    // Now draw the rest of the background.
    //
    if (RectVisible(hdc, prcClip))
    {
        ListView_DrawSimpleBackground(plv, hdc, prcClip);
    }

    //
    // Restore old clipping region.
    //
    SelectClipRgn(hdc, hrgnClipSave);
    if (hrgnClipSave)
    {
        DeleteObject(hrgnClipSave);
    }
}

BOOL NEAR ListView_OnEraseBkgnd(LV *plv, HDC hdc)
{
    RECT rcClip;

    // Regional listviews only need to erase if we're on a slow machine
    if (!(plv->exStyle & LVS_EX_REGIONAL) || g_fSlowMachine) {
        //
        // We draw our own background, erase with it.
        //
        GetClipBox(hdc, &rcClip);
        ListView_DrawBackground(plv, hdc, &rcClip);
    }

    return TRUE;
}

void NEAR ListView_OnCommand(LV* plv, int id, HWND hwndCtl, UINT codeNotify)
{
    if (hwndCtl == plv->hwndEdit)
    {
        switch (codeNotify)
        {
        case EN_UPDATE:
#if defined(FE_IME) || !defined(WINNT)
            // We don't want flicker during replacing current selection
            // as we use selection for IME composition.
            //
            if ((g_fDBCSInputEnabled) && (plv->flags & LVF_INSERTINGCOMP))
                break;
#endif
            // We will use the ID of the window as a Dirty flag...
            if (IsWindowVisible(plv->hwndEdit)) {
                SetWindowID(plv->hwndEdit, 1);
                ListView_SetEditSize(plv);
            }
            break;

        case EN_KILLFOCUS:
            // We lost focus, so dismiss edit and save changes
            // (Note that the owner might reject the change and restart
            // edit mode, which traps the user.  Owners need to give the
            // user a way to get out.)
            //
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
             if (!ListView_DismissEdit(plv, FALSE))
                return;
             break;

         case HN_BEGINDIALOG:  // pen windows is bringing up a dialog
             ASSERT(GetSystemMetrics(SM_PENWINDOWS)); // only on a pen system
             plv->fNoDismissEdit = TRUE;
             break;

         case HN_ENDDIALOG: // pen windows has destroyed dialog
             ASSERT(GetSystemMetrics(SM_PENWINDOWS)); // only on a pen system
             plv->fNoDismissEdit = FALSE;
             break;
        }

        // Forward edit control notifications up to parent
        //
        if (IsWindow(hwndCtl))
            FORWARD_WM_COMMAND(plv->ci.hwndParent, id, hwndCtl, codeNotify, SendMessage);
    }
}

void NEAR ListView_OnWindowPosChanged(LV* plv, const WINDOWPOS FAR* lpwpos)
{
    if (!lpwpos || !(lpwpos->flags & SWP_NOSIZE))
    {
        RECT rc;

        int iOldSlots;

        if (ListView_IsOwnerData(plv) &&
                (ListView_IsSmallView(plv) || ListView_IsIconView(plv)))
        {
            iOldSlots = ListView_GetSlotCount(plv, TRUE);
        }

        GetClientRect(plv->ci.hwnd, &rc);
        plv->sizeClient.cx = rc.right;
        plv->sizeClient.cy = rc.bottom;

        if ((plv->ci.style & LVS_AUTOARRANGE) &&
                (ListView_IsSmallView(plv) || ListView_IsIconView(plv)))
        {
            // Call off to the arrange function.
            ListView_OnArrange(plv, LVA_DEFAULT);
        }

        if (ListView_IsOwnerData(plv))
        {
            plv->rcView.left = RECOMPUTE;
            ListView_Recompute(plv);

            ListView_DismissEdit(plv, FALSE);
            if (ListView_IsSmallView(plv) || ListView_IsIconView(plv))
            {
                // Uses the
                int iNewSlots = ListView_GetSlotCount(plv, TRUE);
                if ((iNewSlots != iOldSlots) && (ListView_Count(plv) > min(iNewSlots, iOldSlots)))
                    RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
            }
        }

        ListView_RInitialize(plv, TRUE);

        // Always make sure the scrollbars are updated to the new size
        ListView_UpdateScrollBars(plv);
    }
}


void ListView_InvalidateSelectedOrCutOwnerData(LV* plv, ILVRange *plvrangeSel)
{
    UINT rdwFlags = RDW_INVALIDATE;
    int cItem = ListView_Count( plv );
    DWORD dwType = plv->ci.style & LVS_TYPEMASK;
    int i;
    RECT rcView;

    ASSERT(ListView_IsOwnerData(plv));
    ASSERT(plv );

    GetClientRect( plv->ci.hwnd, &rcView );

    if (plv->clrTextBk == CLR_NONE
        || (plv->himl && (plv->clrBk != ImageList_GetBkColor(plv->himl)))) {
        // always do an erase, otherwise the text background won't paint right
        rdwFlags |= RDW_ERASE;
    }

    // calculate start of items and end of items visible on the view
    //
    switch (dwType)
    {
    case LVS_REPORT:
        i = ListView_RYHitTest(plv, rcView.top);
        cItem = ListView_RYHitTest(plv, rcView.bottom) + 1;
        break;

    case LVS_LIST:
      i = ListView_LCalcViewItem(plv, rcView.left, rcView.top );
      cItem = ListView_LCalcViewItem( plv, rcView.right, rcView.bottom ) + 1;
        break;

   default:
        ListView_CalcMinMaxIndex( plv, &rcView, &i, &cItem );
        break;
    }

   i = max( i, 0 );

   cItem = min( ListView_Count( plv ), cItem );
    if (cItem > i)
    {
        ListView_NotifyCacheHint( plv, i, cItem-1 );
    }

    for (; i < cItem; i++)
    {
        if (plvrangeSel->lpVtbl->IsSelected(plvrangeSel, i ) == S_OK)
        {
            ListView_InvalidateItem(plv, i, FALSE, rdwFlags);
        }
    }
}

void NEAR ListView_RedrawSelection(LV* plv)
{

    if (ListView_IsOwnerData(plv)) {
        ListView_InvalidateSelectedOrCutOwnerData( plv, plv->plvrangeSel );

    } else {

        int i = -1;

        while ((i = ListView_OnGetNextItem(plv, i, LVNI_SELECTED)) != -1) {
            ListView_InvalidateItem(plv, i, TRUE, RDW_INVALIDATE);
        }


        if (ListView_IsReportView(plv)) {
            int iEnd = ListView_RYHitTest(plv, plv->sizeClient.cy) + 1;

            iEnd = min(iEnd, ListView_Count(plv));

            // if we're in report mode, sub items may have selection focus
            for (i = ListView_RYHitTest(plv, 0); i < iEnd; i++) {
                int iCol;

                for (iCol = 1; iCol < plv->cCol; iCol++) {
                    LISTSUBITEM lsi;
                    ListView_GetSubItem(plv, i, iCol, &lsi);
                    if (lsi.state & LVIS_SELECTED) {
                        ListView_InvalidateItem(plv, i, FALSE, RDW_INVALIDATE);
                    }
                    break;
                }
            }
        }
    }

    UpdateWindow( plv->ci.hwnd );
}

void NEAR ListView_OnSetFocus(LV* plv, HWND hwndOldFocus)
{
    ASSERT(gcWheelDelta == 0);

    // due to the way listview call SetFocus on themselves on buttondown,
    // the window can get a strange sequence of focus messages: first
    // set, then kill, and then set again.  since these are not really
    // focus changes, ignore them and only handle "real" cases.
    //
    // But still send out the accessibility notification because USER
    // has already pushed focus back to the listview instead of to the
    // focus item.

    if (hwndOldFocus == plv->ci.hwnd)
    {
        ListView_NotifyFocusEvent(plv);
        return;
    }

    plv->flags |= LVF_FOCUSED | LVF_UNFOLDED;
    if (IsWindowVisible(plv->ci.hwnd))
    {
        if (plv->iFocus != -1)
        {
            ListView_InvalidateItem(plv, plv->iFocus, TRUE, RDW_INVALIDATE);
            ListView_NotifyFocusEvent(plv);
        }

        ListView_RedrawSelection(plv);
    }

    // Let the parent window know that we are getting the focus.
    CCSendNotify(&plv->ci, NM_SETFOCUS, NULL);
}

void NEAR ListView_OnKillFocus(LV* plv, HWND hwndNewFocus)
{
    // Reset wheel scroll amount
    gcWheelDelta = 0;

    // due to the way listview call SetFocus on themselves on buttondown,
    // the window can get a strange sequence of focus messages: first
    // set, then kill, and then set again.  since these are not really
    // focus changes, ignore them and only handle "real" cases.
    if (!plv || hwndNewFocus == plv->ci.hwnd)
        return;

    plv->flags &= ~(LVF_FOCUSED|LVF_UNFOLDED);

    // Blow this off if we are not currently visible (being destroyed!)
    if (IsWindowVisible(plv->ci.hwnd))
    {
        if (plv->iFocus != -1)
        {
            UINT fRedraw = RDW_INVALIDATE;
            if (plv->clrTextBk == CLR_NONE)
                fRedraw |= RDW_ERASE;
            ListView_InvalidateFoldedItem( plv, plv->iFocus, TRUE, fRedraw );
        }
        ListView_RedrawSelection(plv);
    }

    // Let the parent window know that we are losing the focus.
    CCSendNotify(&plv->ci, NM_KILLFOCUS, NULL);
    IncrementSearchString(&plv->is, 0, NULL);
}

void NEAR ListView_DeselectAll(LV* plv, int iDontDeselect)
{
    int i = -1;
    int nSkipped = 0;
    BOOL fWasSelected = FALSE;

    if (iDontDeselect != -1) {
        if (ListView_OnGetItemState(plv, iDontDeselect, LVIS_SELECTED))
            fWasSelected = TRUE;
    }

    if (ListView_IsOwnerData(plv)) {

        // if there's only one item selected, and that item is the iDontDeselect
        // then our work is done...
        plv->plvrangeSel->lpVtbl->CountIncluded(plv->plvrangeSel, &plv->nSelected);
        if (plv->nSelected == 1 && fWasSelected)
            return;

        ListView_InvalidateSelectedOrCutOwnerData(plv, plv->plvrangeSel);

        ListView_OnSetItemState(plv, -1, 0, LVIS_SELECTED);
        if (fWasSelected) {
            ListView_OnSetItemState(plv, iDontDeselect, LVIS_SELECTED, LVIS_SELECTED);
            nSkipped = 1;
        }

   } else {

       if (iDontDeselect != plv->iFocus) {
           ListView_OnSetItemState(plv, plv->iFocus, 0, LVIS_SELECTED);
       }

       while ((plv->nSelected - nSkipped) && (i = ListView_OnGetNextItem(plv, i, LVNI_SELECTED)) != -1) {
           if (i != iDontDeselect) {
               ListView_OnSetItemState(plv, i, 0, LVIS_SELECTED);
           } else {
               if (fWasSelected) {
                   nSkipped++;
               }
           }
       }
    }

    ASSERT((plv->nSelected - nSkipped) == 0);
    plv->nSelected = nSkipped;
}

// toggle the selection state of an item

void NEAR ListView_ToggleSelection(LV* plv, int iItem)
{
    UINT cur_state;
    if (iItem != -1) {
        cur_state = ListView_OnGetItemState(plv, iItem, LVIS_SELECTED);
        ListView_OnSetItemState(plv, iItem, cur_state ^ LVIS_SELECTED, LVIS_SELECTED);
    }
}

// Selects (or toggles) a range of items in the list.
//      The curent iFocus is the starting location
//      iItem - is the ending item
//      fToggle - Well set all of the selection state of all of the items to
//          inverse the starting location
//
void NEAR ListView_SelectRangeTo(LV* plv, int iItem, BOOL fResetRest)
{
    int iMin, iMax;
    int i = -1;
    UINT uSelVal = LVIS_SELECTED;


    if (plv->iMark == -1)
    {
        ListView_SetFocusSel(plv, iItem, TRUE, TRUE, FALSE);
        return;
    }

    if (!fResetRest)
        uSelVal = ListView_OnGetItemState(plv, plv->iMark, LVIS_SELECTED);

    // If we are in report view or list view we simply walk through the
    // indexes to see which items to select or deselect. otherwise it
    // is is based off of the location of the objects being within the
    // rectangle that is defined by
    if (ListView_IsListView(plv) || ListView_IsReportView(plv))
    {
        iMin = min(iItem, plv->iMark);
        iMax = max(iItem, plv->iMark);

        if (ListView_IsOwnerData( plv )) {

            if (fResetRest)
            {
                ListView_DeselectAll( plv, -1 );
            }

            if (iMax > iMin)
            {
                if (LVIS_SELECTED & uSelVal)
                {
                    if (FAILED(plv->plvrangeSel->lpVtbl->IncludeRange(plv->plvrangeSel, iMin, iMax )))
                        return;
                }
                else
                {
                    if (FAILED(plv->plvrangeSel->lpVtbl->ExcludeRange(plv->plvrangeSel, iMin, iMax )))
                        return;
                }
                ListView_SendODChangeAndInvalidate(plv, iMin, iMax, uSelVal ^ LVIS_SELECTED, uSelVal);
            }
            else
            {
                ListView_OnSetItemState(plv, iMin, uSelVal, LVIS_SELECTED);
            }

        }
        else
        {
            if (fResetRest)
            {
                while ((i = ListView_OnGetNextItem(plv, i, LVNI_SELECTED)) != -1)
                {
                    if (i < iMin || i > iMax)
                        ListView_OnSetItemState(plv, i, 0, LVIS_SELECTED);
                }
            }

            while (iMin <= iMax)
            {
                ListView_OnSetItemState(plv, iMin, uSelVal, LVIS_SELECTED);
                iMin++;
            }
        }
    }
    else
    {
        // Iconic views first calculate the bounding rectangle of the two
        // items.
        RECT    rcTemp;
        RECT    rcTemp2;
        RECT    rcBounding;
        POINT   pt;         //

        ListView_GetRects(plv, plv->iMark, NULL, NULL, NULL, &rcTemp);
        ListView_GetRects(plv, iItem, NULL, NULL, NULL, &rcTemp2);
        UnionRect(&rcBounding, &rcTemp, &rcTemp2);

        // since ownerdata icon views are always arranged, we can assume that
        // all items are in order and we can search only those between the
        // indexes found using the bouding rect
        //
        if (ListView_IsOwnerData( plv ))
        {
            ListView_CalcMinMaxIndex( plv, &rcBounding, &iMin, &iMax );
            if (fResetRest)
            {
              ListView_DeselectAll( plv, -1 );
            }

            iMax = min( iMax, ListView_Count( plv ) );
            iMin = max( iMin, 0 );

        }
        else
        {
           iMax = ListView_Count(plv);
           iMin = 0;

        }

        if (ListView_IsOwnerData(plv)  && (iMax > iMin))
        {
            if (LVIS_SELECTED & uSelVal)
            {
                if (FAILED(plv->plvrangeSel->lpVtbl->IncludeRange(plv->plvrangeSel, iMin, iMax - 1 )))
                    return;
            }
            else
            {
                if (FAILED(plv->plvrangeSel->lpVtbl->ExcludeRange(plv->plvrangeSel, iMin, iMax - 1 )))
                    return;
            }

            ListView_SendODChangeAndInvalidate(plv, iMin, iMax, uSelVal ^ LVIS_SELECTED, uSelVal);

        } else {

            for (i = iMin; i < iMax; i++)
            {
                ListView_GetRects(plv, i, NULL, NULL, NULL, &rcTemp2);
                pt.x = (rcTemp2.right + rcTemp2.left) / 2;  // center of item
                pt.y = (rcTemp2.bottom + rcTemp2.top) / 2;

                if (PtInRect(&rcBounding, pt))
                {
                  int iZ;

                  if (!ListView_IsOwnerData( plv ))
                  {
                      iZ = ListView_ZOrderIndex(plv, i);

                      if (iZ > 0)
                          DPA_InsertPtr(plv->hdpaZOrder, 0, DPA_DeletePtr(plv->hdpaZOrder, iZ));
                  }

                      ListView_OnSetItemState(plv, i, uSelVal, LVIS_SELECTED);
                }
                else if (fResetRest)
                    ListView_OnSetItemState(plv, i, 0, LVIS_SELECTED);
            }
        }
    }
}

// makes an item the focused item and optionally selects it
//
// in:
//      iItem           item to get the focus
//      fSelectAlso     select this item as well as set it as the focus
//      fDeselectAll    deselect all items first
//      fToggleSel      toggle the selection state of the item
//
// returns:
//      index of focus item (if focus change was refused)

// Bugbug::this is getting to have a lot of parameters
int NEAR ListView_SetFocusSel(LV* plv, int iItem, BOOL fSelectAlso,
        BOOL fDeselectAll, BOOL fToggleSel)
{
    int iFocus = plv->iFocus;

    // if we're single sel mode, don't bother with this because
    // the set item will do it for us
    if (!(plv->ci.style & LVS_SINGLESEL) && (fDeselectAll))
        ListView_DeselectAll(plv, -1);

    if (iItem != plv->iFocus)
    {
        // remove the old focus
        if (plv->iFocus != -1)
        {
            // If he refuses to give up the focus, bail out.
            if (!ListView_OnSetItemState(plv, plv->iFocus, 0, LVIS_FOCUSED))
                return plv->iFocus;
        }
    }

   if (!ListView_IsOwnerData( plv )) {

       if (fSelectAlso)
       {
           if (ListView_IsIconView(plv) || ListView_IsSmallView(plv))
           {
               int iZ = ListView_ZOrderIndex(plv, iItem);

               if (iZ > 0)
                   DPA_InsertPtr(plv->hdpaZOrder, 0, DPA_DeletePtr(plv->hdpaZOrder, iZ));
           }
       }
    }

    /* Ensure that when moving focus that we refresh the previous focus
     owner properly. */

    if (iFocus != -1 && iFocus != plv->iFocus && ( plv->flags & LVF_UNFOLDED ) )
        ListView_InvalidateFoldedItem( plv, iFocus, FALSE, RDW_INVALIDATE );

    if (plv->iMark == -1)
        plv->iMark = iItem;

    SetTimer(plv->ci.hwnd, IDT_SCROLLWAIT, GetDoubleClickTime(), NULL);
    plv->flags |= LVF_SCROLLWAIT;

    if (fToggleSel)
    {
        ListView_ToggleSelection(plv, iItem);
        ListView_OnSetItemState(plv, iItem, LVIS_FOCUSED, LVIS_FOCUSED);
    }
    else
    {
        UINT flags;

        flags = ((fSelectAlso || plv->ci.style & LVS_SINGLESEL) ?
                 (LVIS_SELECTED | LVIS_FOCUSED) : LVIS_FOCUSED);
        ListView_OnSetItemState(plv, iItem, flags, flags);
    }

    return iItem;
}

UINT GetLVKeyFlags()
{
    UINT uFlags = 0;

    if (GetKeyState(VK_MENU) < 0)
        uFlags |= LVKF_ALT;
    if (GetKeyState(VK_CONTROL) < 0)
        uFlags |= LVKF_CONTROL;
    if (GetKeyState(VK_SHIFT) < 0)
        uFlags |= LVKF_SHIFT;

    return uFlags;
}

void NEAR ListView_OnKey(LV* plv, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    UINT lvni = 0;
    int iNewFocus;
    BOOL fCtlDown;
    BOOL fShiftDown;
    LV_KEYDOWN nm;
    HWND hwnd = plv->ci.hwnd;

    if (!fDown)
        return;

    // Swap the left and right arrow key if the control is mirrored.
    vk = RTLSwapLeftRightArrows(&plv->ci, vk);

    //prevent any change in selected items before the dbl click timer goes off
    //so that we don't launch wrong item(s)
    if (plv->exStyle & LVS_EX_ONECLICKACTIVATE && plv->fOneClickHappened && plv->fOneClickOK)
    {
        //if a key is pressed with a mouse click with one click activate and double click
        //timer, we end up setting up a timer and then processing the keydown
        //this causes an item to be launched right away (from this code) and in case
        //of return being pressed it causes double activation
        //prevent these cases:
        if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU || vk == VK_RETURN)
            return;
        KillTimer(plv->ci.hwnd, IDT_ONECLICKHAPPENED);
        plv->fOneClickHappened = FALSE;
        CCSendNotify(&plv->ci, LVN_ITEMACTIVATE, &(plv->nmOneClickHappened.hdr));
        if (!IsWindow(hwnd))
            return;
    }

    // Notify
    nm.wVKey = (WORD) vk;
    nm.flags = flags;
    if (CCSendNotify(&plv->ci, LVN_KEYDOWN, &nm.hdr)) {
        plv->iPuntChar++;
        return;
    } else if (plv->iPuntChar) {
        // this is tricky...  if we want to punt the char, just increment the
        // count.  if we do NOT, then we must clear the queue of WM_CHAR's
        // this is to preserve the iPuntChar to mean "punt the next n WM_CHAR messages
        MSG msg;
        while(plv->iPuntChar && PeekMessage(&msg, plv->ci.hwnd, WM_CHAR, WM_CHAR, PM_REMOVE)) {
            plv->iPuntChar--;
        }
        ASSERT(!plv->iPuntChar);
    }

    if (ListView_Count(plv) == 0)   // don't blow up on empty list
        return;

    fCtlDown = GetKeyState(VK_CONTROL) < 0;
    fShiftDown = GetKeyState(VK_SHIFT) < 0;

    switch (vk)
    {
    case VK_SPACE:
#ifdef DEBUG
        if (fCtlDown && fShiftDown) {
            SendMessage(plv->ci.hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                        (SendMessage(plv->ci.hwnd, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0) ^ LVS_EX_GRIDLINES) | LVS_EX_CHECKBOXES);
        }
#endif

        // If shift (extend) or control (disjoint) select,
        // then toggle selection state of focused item.
        if (fCtlDown)
        {
            plv->iMark = plv->iFocus;
            ListView_ToggleSelection(plv, plv->iFocus);
            plv->iPuntChar++;
        }

        // BUGBUG: Implement me
        if ( fShiftDown) {
            ListView_SelectRangeTo(plv, plv->iFocus, TRUE);
        }

        if (ListView_CheckBoxes(plv)) {
            if (plv->iFocus != -1)
                ListView_HandleStateIconClick(plv, plv->iFocus);
        }
#ifdef KEYBOARDCUES
        //notify of navigation key usage
        CCNotifyNavigationKeyUsage(&(plv->ci), UISF_HIDEFOCUS);
#endif
        return;
    case VK_RETURN:
#ifdef DEBUG
        if (fCtlDown && fShiftDown) {
            if (plv->iFocus != -1) {
                LV_ITEM lvi;
                lvi.iSubItem = 1;
                lvi.iItem = plv->iFocus;
                lvi.iImage = 3;
                lvi.state = LVIS_SELECTED;
                lvi.stateMask = LVIS_SELECTED;
                lvi.mask = LVIF_STATE | LVIF_IMAGE;
                SendMessage(plv->ci.hwnd, LVM_SETITEM, 0, (LPARAM)&lvi);
            }
            return;
        }
#endif
        CCSendNotify(&plv->ci, NM_RETURN, NULL);

        /// some (comdlg32 for example) destroy on double click
        // we need to bail if that happens because plv is no longer valid
        if (!IsWindow(hwnd))
            return;

        {
            NMITEMACTIVATE nm;

            nm.iItem = plv->iFocus;
            nm.iSubItem = 0;
            nm.uChanged = 0;
            nm.ptAction.x = -1;
            nm.ptAction.y = -1;
            nm.uKeyFlags = GetLVKeyFlags();
            CCSendNotify(&plv->ci, LVN_ITEMACTIVATE, &nm.hdr);
            if (!IsWindow(hwnd))
                return;
        }
#ifdef KEYBOARDCUES
        //notify of navigation key usage
        CCNotifyNavigationKeyUsage(&(plv->ci), UISF_HIDEFOCUS);
#endif
        return;

    case VK_ADD:
        if (ListView_IsReportView(plv) && (GetKeyState(VK_CONTROL) < 0))
        {
            HCURSOR hcurPrev;
            int i;

            hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));
            for (i=0; i < plv->cCol; i++)
            {
                ListView_RSetColumnWidth(plv, i, -1);
            }

            SetCursor(hcurPrev);
#ifdef KEYBOARDCUES
            //notify of navigation key usage
            CCNotifyNavigationKeyUsage(&(plv->ci), UISF_HIDEFOCUS);
#endif
            return;
        }
    }

    if (GetKeyState(VK_MENU) < 0)
        return;

    // For a single selection listview, disable extending the selection
    // by turning off the keyboard modifiers.
    if (plv->ci.style & LVS_SINGLESEL) {
        fCtlDown = FALSE;
        fShiftDown = FALSE;
    }

    //
    // Let the Arrow function attempt to process the key.
    //
    iNewFocus = ListView_Arrow(plv, plv->iFocus, vk);

    // If control (disjoint) selection, don't change selection.
    // If shift (extend) or control selection, don't deselect all.
    //
    if (iNewFocus != -1) {
        if (fShiftDown)
        {
            ListView_SelectRangeTo(plv, iNewFocus, TRUE);
            ListView_SetFocusSel(plv, iNewFocus, FALSE, FALSE, FALSE);
        }
        else {
            if (!fCtlDown)
                plv->iMark = iNewFocus;
            ListView_SetFocusSel(plv, iNewFocus, !fCtlDown, !fShiftDown && !fCtlDown, FALSE);
        }
        IncrementSearchString(&plv->is, 0, NULL);
        CCPlaySound(c_szSelect);
    }

    // on keyboard movement, scroll immediately.
    if (ListView_CancelScrollWait(plv)) {
        ListView_OnEnsureVisible(plv, plv->iFocus, FALSE);
        UpdateWindow(plv->ci.hwnd);
    }
#ifdef KEYBOARDCUES
    //notify of navigation key usage
    CCNotifyNavigationKeyUsage(&(plv->ci), UISF_HIDEFOCUS);
#endif
}

//
//  LVN_INCREMENTALSEARCH gives the app the opportunity to customize
//  incremental search.  For example, if the items are numeric,
//  the app can do numerical search instead of string search.
//
//  App sets pnmfi->lvfi.lParam to the result of the incremental search,
//  or to -2 to fai the search and just beep.
//
//  App can return 2 to indicate that all processing should stop, if
//  app wants to take over incremental search completely.
//
BOOL ListView_IncrementalSearch(LV *plv, int iStartFrom, LPNMLVFINDITEM pnmfi, int *pi)
{
    INT_PTR fRc;

    ASSERT(!(pnmfi->lvfi.flags & LVFI_PARAM));
    pnmfi->lvfi.lParam = -1;

    fRc = CCSendNotify(&plv->ci, LVN_INCREMENTALSEARCH, &pnmfi->hdr);
    *pi = (int)pnmfi->lvfi.lParam;

    // Cannot just return fRc because some apps return 1 to all WM_NOTIFY's
    return fRc == 2;
}

#if defined(FE_IME)
// Now only Korean version is interested in incremental search with composition string.
LPTSTR GET_COMP_STRING(HIMC hImc, DWORD dwFlags)
{
    LONG iNumComp;
    PTSTR pszCompStr;
    iNumComp = ImmGetCompositionString(hImc, dwFlags, NULL, 0);
    pszCompStr = (PTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(iNumComp+1));
    if (pszCompStr)
    {
        if (iNumComp)
            ImmGetCompositionString(hImc, dwFlags, pszCompStr, iNumComp+1);
        pszCompStr[iNumComp] = TEXT('\0');
    }
    return pszCompStr;
}

#define FREE_COMP_STRING(pszCompStr)    LocalFree((HLOCAL)(pszCompStr))

BOOL NEAR ListView_OnImeComposition(LV* plv, WPARAM wParam, LPARAM lParam)
{
    LPTSTR lpsz;
    NMLVFINDITEM nmfi;
    int i;
    int iStartFrom = -1;
    int iLen;
    int iCount;
    HIMC hImc;
    TCHAR *pszCompStr;
    BOOL fRet = TRUE;

    iCount = ListView_Count(plv);

    if (!iCount || plv->iFocus == -1)
        return fRet;

    if (hImc = ImmGetContext(plv->ci.hwnd))
    {
        if (lParam & GCS_RESULTSTR)
        {
            fRet = FALSE;
            pszCompStr = GET_COMP_STRING(hImc, GCS_RESULTSTR);
            if (pszCompStr)
            {
                IncrementSearchImeCompStr(&plv->is, FALSE, pszCompStr, &lpsz);
                FREE_COMP_STRING(pszCompStr);
            }
        }
        if (lParam & GCS_COMPSTR)
        {
            fRet = TRUE;
            pszCompStr = GET_COMP_STRING(hImc, GCS_COMPSTR);
            if (pszCompStr)
            {
                if (IncrementSearchImeCompStr(&plv->is, TRUE, pszCompStr, &lpsz))
                    iStartFrom = plv->iFocus;
                else
                    iStartFrom = ((plv->iFocus - 1) + iCount)% iCount;

                nmfi.lvfi.flags = LVFI_SUBSTRING | LVFI_STRING | LVFI_WRAP;
                nmfi.lvfi.psz = lpsz;
                iLen = lstrlen(lpsz);

                // special case space as the first character
                if ((iLen == 1) && (*lpsz == TEXT(' '))) {
                    if (plv->iFocus != -1) {
                        ListView_OnSetItemState(plv, plv->iFocus, LVIS_SELECTED, LVIS_SELECTED);
                        IncrementSearchString(&plv->is, 0, NULL);
                    }
#ifdef KEYBOARDCUES
                    //notify of navigation key usage
                    CCNotifyNavigationKeyUsage(&(plv->ci), UISF_HIDEFOCUS);
#endif
                    return fRet;
                }

                // Give caller full string in case they want to do something custom
                if (ListView_IncrementalSearch(plv, iStartFrom, &nmfi, &i))
                    return fRet;

#ifdef UNICODE
                if (iLen > 0 && SameChars(lpsz, lpsz[0])) {
#else
                if (iLen > 1 && SameDBCSChars(lpsz, (WORD)((BYTE)lpsz[0] << 8 | (BYTE)lpsz[1]))) {
#endif

                    //
                    //  The user has been typing the same char over and over again.
                    //  Switch from incremental search to Windows 3.1 style search.
                    //
                    iStartFrom = plv->iFocus;
                    nmfi.lvfi.psz = lpsz + iLen - 1;
                }

                if (i == -1)
                    i = ListView_OnFindItem(plv, iStartFrom, &nmfi.lvfi);

                if (!ListView_IsValidItemNumber(plv, i)) {
                    i = -1;
                }

#ifdef LVDEBUG
                DebugMsg(TF_LISTVIEW, TEXT("CIme listsearch %d %s %d"), (LPTSTR)lpsz, (LPTSTR)lpsz, i);
#endif

                if (i != -1) {
                    ListView_SetFocusSel(plv, i, TRUE, TRUE, FALSE);
                    plv->iMark = i;
                    if (ListView_CancelScrollWait(plv))
                            ListView_OnEnsureVisible(plv, i, FALSE);
                } else {
                    // Don't beep on spaces, we use it for selection.
                    IncrementSearchBeep(&plv->is);
                }

#ifdef KEYBOARDCUES
                //notify of navigation key usage
                CCNotifyNavigationKeyUsage(&(plv->ci), UISF_HIDEFOCUS);
#endif
                FREE_COMP_STRING(pszCompStr);
            }
        }
        ImmReleaseContext(plv->ci.hwnd, hImc);
    }
    return fRet;
}

#ifndef UNICODE
BOOL FAR PASCAL SameDBCSChars(LPSTR lpsz,  WORD w)
{
    while (*lpsz) {
        if (IsDBCSLeadByte(*lpsz) == FALSE)
            return FALSE;
        if ((WORD)((BYTE)*lpsz++ << 8 | (BYTE)*lpsz++) != w)
            return FALSE;
    }
    return TRUE;
}
#endif
#endif FE_IME

// REVIEW: We will want to reset ichCharBuf to 0 on certain conditions,
// such as: focus change, ENTER, arrow key, mouse click, etc.
//
void NEAR ListView_OnChar(LV* plv, UINT ch, int cRepeat)
{
    LPTSTR lpsz;
    NMLVFINDITEM nmfi;
    int i;
    int iStartFrom = -1;
    int iLen;
    int iCount;

    iCount = ListView_Count(plv);

    if (!iCount || plv->iFocus == -1)
        return;

    // Don't search for chars that cannot be in a file name (like ENTER and TAB)
    // BUGBUG raymondc fix for v6: The Polish keyboard layout uses CTRL+ALT to
    // enter some normal letters, so don't punt if the CTRL key is down or
    // people in Poland are screwed!
    if (ch < TEXT(' ') || GetKeyState(VK_CONTROL) < 0)
    {
        IncrementSearchString(&plv->is, 0, NULL);
        return;
    }

#ifdef UNICODE_WIN9x
    if (g_fDBCSEnabled && (IsDBCSLeadByteEx(plv->ci.uiCodePage, (BYTE)ch) || plv->uDBCSChar))
    {
        WCHAR wch;

        if (!plv->uDBCSChar)
        {
            // Save DBCS LeadByte character
            plv->uDBCSChar = ch & 0x00ff;
            return;
        }
        else
        {
            // Combine DBCS characters
            plv->uDBCSChar |=  ((ch & 0x00ff) << 8);

            // Convert to UNICODE
            if (MultiByteToWideChar(plv->ci.uiCodePage, MB_ERR_INVALID_CHARS, (LPCSTR)&plv->uDBCSChar, 2, &wch, 1))
            {
                ch = wch;
            }

            plv->uDBCSChar = 0;
        }
    }
    else
    {
        if (ch >= 0x80)     // no need conversion for low ansi character
        {
            WCHAR wch;

            if (MultiByteToWideChar(plv->ci.uiCodePage, 0, (LPCSTR)&ch, 1, &wch, 1))
                ch = wch;
        }
    }
#endif

    if (IncrementSearchString(&plv->is, ch, &lpsz))
        iStartFrom = plv->iFocus;
    else
        iStartFrom = ((plv->iFocus - 1) + iCount)% iCount;

    nmfi.lvfi.flags = LVFI_SUBSTRING | LVFI_STRING | LVFI_WRAP;
    nmfi.lvfi.psz = lpsz;
    iLen = lstrlen(lpsz);

    // special case space as the first character
    if ((iLen == 1) && (*lpsz == ' ')) {
        if (plv->iFocus != -1) {
            ListView_OnSetItemState(plv, plv->iFocus, LVIS_SELECTED, LVIS_SELECTED);
            IncrementSearchString(&plv->is, 0, NULL);
        }
#ifdef KEYBOARDCUES
        //notify of navigation key usage
        CCNotifyNavigationKeyUsage(&(plv->ci), UISF_HIDEFOCUS);
#endif
        return;
    }

    // Give caller full string in case they want to do something custom
    if (ListView_IncrementalSearch(plv, iStartFrom, &nmfi, &i))
        return;

    if (iLen > 0 && SameChars(lpsz, lpsz[0])) {
        //
        //  The user has been typing the same char over and over again.
        //  Switch from incremental search to Windows 3.1 style search.
        //
        iStartFrom = plv->iFocus;
        nmfi.lvfi.psz = lpsz + iLen - 1;
    }

    if (i == -1)
        i = ListView_OnFindItem(plv, iStartFrom, &nmfi.lvfi);

    if (!ListView_IsValidItemNumber(plv, i)) {
        i = -1;
    }

#ifdef LVDEBUG
    DebugMsg(TF_LISTVIEW, TEXT("listsearch %d %s %d"), (LPTSTR)lpsz, (LPTSTR)lpsz, i);
#endif

    if (i != -1) {
        ListView_SetFocusSel(plv, i, TRUE, TRUE, FALSE);
        plv->iMark = i;
        if (ListView_CancelScrollWait(plv))
                ListView_OnEnsureVisible(plv, i, FALSE);
    } else {
        // Don't beep on spaces, we use it for selection.
        IncrementSearchBeep(&plv->is);
    }

#ifdef KEYBOARDCUES
    //notify of navigation key usage
    CCNotifyNavigationKeyUsage(&(plv->ci), UISF_HIDEFOCUS);
#endif
}

BOOL FAR PASCAL SameChars(LPTSTR lpsz, TCHAR c)
{
    while (*lpsz) {
        if (*lpsz++ != c)
            return FALSE;
    }
    return TRUE;
}

UINT NEAR ListView_OnGetDlgCode(LV* plv, MSG FAR* lpmsg)
{
    return DLGC_WANTARROWS | DLGC_WANTCHARS;
}

int ListView_ComputeCYItemSize(LV* plv)
{
    int cy;

    cy = max(plv->cyLabelChar, plv->cySmIcon);

    if (plv->himlState)
        cy = max(cy, plv->cyState);

    cy += g_cyBorder;

    ASSERT(cy);
    return cy;
}

void NEAR ListView_InvalidateCachedLabelSizes(LV* plv)
{
    int i;

    ListView_InvalidateTTLastHit(plv, plv->iTTLastHit);

    // Label wrapping has changed, so we need to invalidate the
    // size of the items, such that they will be recomputed.
    //
    if (!ListView_IsOwnerData( plv ))
    {
        for (i = ListView_Count(plv) - 1; i >= 0; i--)
        {
            LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, i);
            ListView_SetSRecompute(pitem);
        }
    }
    plv->rcView.left = RECOMPUTE;

    if ((plv->ci.style & LVS_OWNERDRAWFIXED) && ListView_IsReportView(plv))
        plv->cyItemSave = ListView_ComputeCYItemSize(plv);
    else {
        plv->cyItem = ListView_ComputeCYItemSize(plv);
    }
}


int LV_GetNewColWidth(LV* plv, int iFirst, int iLast);

void NEAR ListView_OnStyleChanging(LV* plv, UINT gwl, LPSTYLESTRUCT pinfo)
{
    if (gwl == GWL_STYLE) {
        // Don't allow LVS_OWNERDATA to change after creation
        DWORD stylePreserve = LVS_OWNERDATA;

        // Don't allow a LVS_EX_REGIONAL listview to change type, since
        // it must be LVS_ICON
        if (plv->exStyle & LVS_EX_REGIONAL)
            stylePreserve |= LVS_TYPEMASK;

        // Preserve the bits that must be preserved
        pinfo->styleNew ^= (pinfo->styleNew ^ pinfo->styleOld) & stylePreserve;
    }
}

void NEAR ListView_OnStyleChanged(LV* plv, UINT gwl, LPSTYLESTRUCT pinfo)
{
    // Style changed: redraw everything...
    //
    // try to do this smartly, avoiding unnecessary redraws
    if (gwl == GWL_STYLE)
    {
        BOOL fRedraw = FALSE, fShouldScroll = FALSE;
        DWORD changeFlags, styleOld;

        ListView_DismissEdit(plv, FALSE);   // BUGBUG:  FALSE == apply edits.  Is this correct?

        changeFlags = plv->ci.style ^ pinfo->styleNew;
        styleOld = plv->ci.style;

        // (dli) Setting the small icon width here and only in the case when we go
        // from large icon view to some other view because of three reasons:
        // 1. According to chee, we want to set this before we change the style bit in
        // plv or after we scale.
        // 2. We don't want to do it after we scale because we want to set the width to
        // the maximum value so that the items in this listview do not cover each other
        // 3. we do it from large icon view because large icon view has fixed width for
        // each item, small icon view width can be scaled.
        //
        if ((changeFlags & LVS_TYPEMASK) && ((styleOld & LVS_TYPEMASK) == LVS_ICON))
            ListView_ISetColumnWidth(plv, 0,
                                     LV_GetNewColWidth(plv, 0, ListView_Count(plv)-1), FALSE);

        plv->ci.style = pinfo->styleNew;        // change our version

        if (changeFlags & (WS_BORDER | WS_CAPTION | WS_THICKFRAME)) {
            // the changing of these bits affect the size of the window
            // but not until after this message is handled
            // so post ourself a message.
            PostMessage(plv->ci.hwnd, LVMP_WINDOWPOSCHANGED, 0, 0);
        }

        if (changeFlags & LVS_NOCOLUMNHEADER) {
            if (plv->hwndHdr) {
                SetWindowBits(plv->hwndHdr, GWL_STYLE, HDS_HIDDEN,
                              (plv->ci.style & LVS_NOCOLUMNHEADER) ? HDS_HIDDEN : 0);

                fRedraw = TRUE;
                fShouldScroll = TRUE;
            }
        }


        if (changeFlags & LVS_NOLABELWRAP)
        {
            ListView_InvalidateCachedLabelSizes(plv);
            fShouldScroll = TRUE;
            fRedraw = TRUE;
        }

        if (changeFlags & LVS_TYPEMASK)
        {
            ListView_TypeChange(plv, styleOld);
            fShouldScroll = TRUE;
            fRedraw = TRUE;
        }

        if ((changeFlags & LVS_AUTOARRANGE) && (plv->ci.style & LVS_AUTOARRANGE))
        {
            ListView_OnArrange(plv, LVA_DEFAULT);
            fRedraw = TRUE;
        }

        // bugbug, previously, this was the else to
        // (changeFlags & LVS_AUTOARRANGE && (plv->ci.style & LVS_AUTOARRANGE))
        // I'm not sure that was really the right thing..
        if (fShouldScroll)
        {
            // Else we would like to make the most important item to still
            // be visible.  So first we will look for a cursorered item
            // if this fails, we will look for the first selected item,
            // else we will simply ask for the first item (assuming the
            // count > 0
            //
            int i;

            // And make sure the scrollbars are up to date Note this
            // also updates some variables that some views need
            ListView_UpdateScrollBars(plv);

            i = (plv->iFocus >= 0) ? plv->iFocus : ListView_OnGetNextItem(plv, -1, LVNI_SELECTED);
            if ((i == -1)  && (ListView_Count(plv) > 0))
                i = 0;

            if (i != -1)
                ListView_OnEnsureVisible(plv, i, TRUE);

        }

        if (fRedraw)
            RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
    }
    else if (gwl == GWL_EXSTYLE)
    {
        //
        // If the RTL_MIRROR extended style bit had changed, let's
        // repaint the control window.
        //
        if ((plv->ci.dwExStyle&RTL_MIRRORED_WINDOW) !=  (pinfo->styleNew&RTL_MIRRORED_WINDOW))
            RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);

        //
        // Save the new ex-style bits
        //
        plv->ci.dwExStyle = pinfo->styleNew;
    }


    // Change of styles also changes tooltip policy, so pop it
    ListView_PopBubble(plv);

}

void NEAR ListView_TypeChange(LV* plv, DWORD styleOld)
{
    RECT rc;

    //
    //  Invalidate all cached string metrics because customdraw clients
    //  may draw differently depending on the type.  This happens more
    //  often than you might think, not on purpose, but because apps are
    //  buggy.
    //
    //  APP COMPAT!  You'd think this was completely safe.  After all,
    //  all we're doing is invalidating our cache so we ask the parent
    //  afresh the next time we need the strings.  But noooooooo,
    //  Outlook98 will FAULT if you ask it for information that it thinks
    //  you by all rights already know.  Sigh.  So guard this with a v5.
    //
    if (plv->ci.iVersion >= 5 && !ListView_IsOwnerData(plv))
    {
        int i;
        for (i = 0; i < ListView_Count(plv); i++)
        {
            LISTITEM *pitem = ListView_FastGetItemPtr(plv, i);
            ListView_SetSRecompute(pitem);
        }
    }

    switch (styleOld & LVS_TYPEMASK)
    {
    case LVS_REPORT:
        ShowWindow(plv->hwndHdr, SW_HIDE);
        if (styleOld & LVS_OWNERDRAWFIXED) {
            // swap cyItem and cyFixed;
            int temp = plv->cyItem;
            plv->cyItem = plv->cyItemSave;
            plv->cyItemSave = temp;
        }
        break;

    default:
        break;
    }

    // Now handle any special setup needed for the new view
    switch (plv->ci.style & LVS_TYPEMASK)
    {
    case (UINT)LVS_ICON:
        ListView_ScaleIconPositions(plv, FALSE);
        break;

    case (UINT)LVS_SMALLICON:
        ListView_ScaleIconPositions(plv, TRUE);
        break;

    case (UINT)LVS_LIST:
        // We may need to resize the columns
        ListView_MaybeResizeListColumns(plv, 0, ListView_Count(plv)-1);
        break;

    case (UINT)LVS_REPORT:
        // if it's owner draw fixed, we may have to do funky stuff
        if ((styleOld & LVS_TYPEMASK) != LVS_REPORT) {
            plv->cyItemSave = plv->cyItem;
        }
        ListView_RInitialize(plv, FALSE);
        break;

    default:
        break;
    }

    ListView_InvalidateTTLastHit(plv, plv->iTTLastHit);

    GetClientRect(plv->ci.hwnd, &rc);
    plv->sizeClient.cx = rc.right;
    plv->sizeClient.cy = rc.bottom;
}

int NEAR ListView_OnHitTest(LV* plv, LV_HITTESTINFO FAR* pinfo)
{
    UINT flags;
    int x, y;

    if (!pinfo) return -1;

    x = pinfo->pt.x;
    y = pinfo->pt.y;

    pinfo->iItem = -1;
    flags = 0;
    if (x < 0)
        flags |= LVHT_TOLEFT;
    else if (x >= plv->sizeClient.cx)
        flags |= LVHT_TORIGHT;
    if (y < 0)
        flags |= LVHT_ABOVE;
    else if (y >= plv->sizeClient.cy)
        flags |= LVHT_BELOW;

    if (flags == 0)
    {
        pinfo->iItem = _ListView_ItemHitTest(plv, x, y, &flags, NULL);
    }

    pinfo->flags = flags;

    if (pinfo->iItem >= ListView_Count(plv)) {
        pinfo->iItem = -1;
        pinfo->flags = LVHT_NOWHERE;
    }
    return pinfo->iItem;
}

int NEAR ScrollAmount(int large, int iSmall, int unit)
{

    return (((large - iSmall) + (unit - 1)) / unit) * unit;
}

// NOTE: this is duplicated in shell32.dll
//
// checks to see if we are at the end position of a scroll bar
// to avoid scrolling when not needed (avoid flashing)
//
// in:
//      code        SB_VERT or SB_HORZ
//      bDown       FALSE is up or left
//                  TRUE  is down or right
BOOL NEAR PASCAL CanScroll(LV* plv, int code, BOOL bDown)
{
    SCROLLINFO si;

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;

    if (ListView_GetScrollInfo(plv, code, &si))
    {
        if (bDown)
        {
            if (si.nPage)
                si.nMax -= si.nPage - 1;
            return si.nPos < si.nMax;
        }
        else
        {
            return si.nPos > si.nMin;
        }
    }
    else
    {
        return FALSE;
    }
}

// detect if we should auto scroll the window
//
// in:
//      pt  cursor pos in hwnd's client coords
// out:
//      pdx, pdy ammount scrolled in x and y
//
// REVIEW, this should make sure a certain amount of time has passed
// before scrolling.

void NEAR ScrollDetect(LV* plv, POINT pt, int FAR *pdx, int FAR *pdy)
{
    int dx, dy;

    *pdx = *pdy = 0;

    if (!(plv->ci.style & (WS_HSCROLL | WS_VSCROLL)))
        return;

    dx = dy = plv->cyIcon / 16;
    if (ListView_IsReportView(plv)) {
        dy = plv->cyItem;       // we scroll in units of items...
        if (!dx)
            dx = plv->cxSmIcon;
    }
    if (ListView_IsListView(plv))
        dx = plv->cxItem;

    if (!dx)
        dx = 1;

    if (!dy)
        dy = 1;

    // we need to check if we can scroll before acutally doing it
    // since the selection rect is adjusted based on how much
    // we scroll by

    if (plv->ci.style & WS_VSCROLL) { // scroll vertically?

        if (pt.y >= plv->sizeClient.cy) {
            if (CanScroll(plv, SB_VERT, TRUE))
                *pdy = ScrollAmount(pt.y, plv->sizeClient.cy, dy);   // down
        } else if (pt.y <= 0) {
            if (CanScroll(plv, SB_VERT, FALSE))
                *pdy = -ScrollAmount(0, pt.y, dy);     // up
        }
    }

    if (plv->ci.style & WS_HSCROLL) { // horizontally

        if (pt.x >= plv->sizeClient.cx) {
            if (CanScroll(plv, SB_HORZ, TRUE))
                *pdx = ScrollAmount(pt.x, plv->sizeClient.cx, dx);    // right
        } else if (pt.x <= 0) {
            if (CanScroll(plv, SB_HORZ, FALSE))
                *pdx = -ScrollAmount(0, pt.x, dx);    // left
        }
    }

    // BUGBUG: this will potentially scroll outside the bounds of the
    // listview.  we should bound the scroll amount in CanScroll()
    // or ScrollAmount().

    if (*pdx || *pdy) {
        ListView_ValidateScrollParams(plv, pdx, pdy);
    }
}

#define swap(pi1, pi2) {int i = *(pi1) ; *(pi1) = *(pi2) ; *(pi2) = i ;}

void NEAR OrderRect(RECT FAR *prc)
{
    if (prc->left > prc->right)
        swap(&prc->left, &prc->right);

    if (prc->bottom < prc->top)
        swap(&prc->bottom, &prc->top);
}

// in:
//      x, y    starting point in client coords

#define SCROLL_FREQ     (GetDoubleClickTime()/2)     // 1/5 of a second between scrolls

//----------------------------------------------------------------------------
BOOL ShouldScroll(LV* plv, LPPOINT ppt, LPRECT lprc)
{
    ASSERT(ppt);

    if (plv->ci.style & WS_VSCROLL)
    {
        if (ppt->y >= lprc->bottom)
        {
            if (CanScroll(plv, SB_VERT, TRUE))
                return TRUE;
        }
        else if (ppt->y <= lprc->top)
        {
            if (CanScroll(plv, SB_VERT, FALSE))
                return TRUE;
        }
    }

    if (plv->ci.style & WS_HSCROLL)
    {
        if (ppt->x >= lprc->right)
        {
            if (CanScroll(plv, SB_HORZ, TRUE))
                return TRUE;
        }
        else if (ppt->x <= lprc->left)
        {
            if (CanScroll(plv, SB_HORZ, FALSE))
                return TRUE;
        }
    }

    return FALSE;
}

#ifdef WIN95MARQUEE
#define DrawFocusRectClip(a, b, c) DrawFocusRect(a, b)
#else
BOOL DrawFocusRectClip(HDC hdc, CONST RECT * prc, CONST RECT * prcClip)
{
    RECT rc;

    IntersectRect(&rc, prc, prcClip);

    return DrawFocusRect(hdc, &rc);
}
#endif

//----------------------------------------------------------------------------
void NEAR ListView_DragSelect(LV *plv, int x, int y)
{
    RECT rc, rcWindow, rcOld, rcUnion, rcTemp2, rcClip;
    POINT pt;
    MSG32 msg32;
    HDC hdc;
    HWND hwnd = plv->ci.hwnd;
    int i, iEnd, dx, dy;
    BOOL bInOld, bInNew = FALSE, bLocked = FALSE;
    DWORD dwTime, dwNewTime;
    HRGN hrgnUpdate = NULL, hrgnLV = NULL;

    rc.left = rc.right = x;
    rc.top = rc.bottom = y;

    rcOld = rc;

    UpdateWindow(plv->ci.hwnd);

    if (plv->exStyle & LVS_EX_REGIONAL) {
        if ((hrgnUpdate = CreateRectRgn(0,0,0,0)) &&
            (hrgnLV = CreateRectRgn(0,0,0,0)) &&
            (LockWindowUpdate(GetParent(hwnd)))) {
            hdc = GetDCEx(hwnd, NULL, DCX_PARENTCLIP | DCX_LOCKWINDOWUPDATE);
            bLocked = TRUE;
        } else {
            goto BailOut;
        }
    } else {
        hdc = GetDC(hwnd);
    }

    SetCapture(hwnd);

    DrawFocusRect(hdc, &rc);

    GetClientRect(hwnd, &rcClip);
    GetWindowRect(hwnd, &rcWindow);

    dwTime = GetTickCount();

    for (;;)
    {
        // WM_CANCELMODE messages will unset the capture, in that
        // case I want to exit this loop
        if (GetCapture() != hwnd)
        {
            break;
        }

        if (!PeekMessage32(&msg32, NULL, 0, 0, PM_REMOVE, TRUE)) {

            // if the cursor is outside of the window rect
            // we need to generate messages to make autoscrolling
            // keep going

            if (!PtInRect(&rcWindow, msg32.pt))
            {
                // If we may be able to scroll, generate a mouse move.
                if (ShouldScroll(plv, &msg32.pt, &rcWindow))
                    SetCursorPos(msg32.pt.x, msg32.pt.y);
            }
            else
            {
                WaitMessage();
            }
            continue;
        }


        // See if the application wants to process the message...
        if (CallMsgFilter32(&msg32, MSGF_COMMCTRL_DRAGSELECT, TRUE) != 0)
            continue;

        switch (msg32.message)
        {

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDOWN:
            CCReleaseCapture(&plv->ci);
            goto EndOfLoop;


        case WM_TIMER:
            if (msg32.wParam != IDT_MARQUEE)
                goto DoDefault;
            // else fall through

        case WM_MOUSEMOVE:
        {
            int dMax = -1;
            pt = msg32.pt;
            ScreenToClient(hwnd, &pt);

            dwNewTime = GetTickCount();
//            if (1 || (dwNewTime - dwTime) > SCROLL_FREQ)
//          {
                dwTime = dwNewTime;     // reset scroll timer
                ScrollDetect(plv, pt, &dx, &dy);
//          }
//          else
//          {
//              dx = dy = 0;
//          }
            //SetTimer(plv->ci.hwnd, IDT_MARQUEE, SCROLL_FREQ, NULL);

            y -= dy;    // scroll up/down
            x -= dx;    // scroll left/right

            rc.left = x;
            rc.top = y;
            rc.right = pt.x;
            rc.bottom = pt.y;

            // clip drag rect to the window
            //
            if (rc.right > rcClip.right)
                rc.right = rcClip.right;
            if (rc.right < rcClip.left)
                rc.right = rcClip.left;
            if (rc.bottom > rcClip.bottom)
                rc.bottom = rcClip.bottom;
            if (rc.bottom < rcClip.top)
                rc.bottom = rcClip.top;

            OrderRect(&rc);

            if (EqualRect(&rc, &rcOld))
                break;

            // move the old rect
            DrawFocusRect(hdc, &rcOld); // erase old
            if (dx || dy)
                ListView_OnScroll(plv, dx, dy);
            OffsetRect(&rcOld, -dx, -dy);

            //
            // For Report and List view, we can speed things up by
            // only searching through those items that are visible.  We
            // use the hittest to calculate the first item to paint.
            // BUGBUG:: We are using state specific info here...
            //
            UnionRect(&rcUnion, &rc, &rcOld);

            if (ListView_IsReportView(plv))
            {
                i = (int)((plv->ptlRptOrigin.y + rcUnion.top  - plv->yTop)
                        / plv->cyItem);
                iEnd = (int)((plv->ptlRptOrigin.y + rcUnion.bottom  - plv->yTop)
                        / plv->cyItem) + 1;
            }

            else if (ListView_IsListView(plv))
            {
                i = ((plv->xOrigin + rcUnion.left)/ plv->cxItem)
                        * plv->cItemCol + rcUnion.top / plv->cyItem;

                iEnd = ((plv->xOrigin + rcUnion.right)/ plv->cxItem)
                        * plv->cItemCol + rcUnion.bottom / plv->cyItem + 1;
            }

            else
            {
                if (ListView_IsOwnerData( plv ))
                {
                    ListView_CalcMinMaxIndex( plv, &rcUnion, &i, &iEnd );
                }
                else
                {
                    i = 0;
                    iEnd = ListView_Count(plv);
                }
            }

            // make sure our endpoint is in range.
            if (iEnd > ListView_Count(plv))
                iEnd = ListView_Count(plv);

            if (i < 0)
                i = 0;

            if (ListView_IsOwnerData(plv) && (i < iEnd)) {
                ListView_NotifyCacheHint(plv, i, iEnd-1);
            }

            if (bInNew && !(msg32.wParam & (MK_CONTROL | MK_SHIFT))) {
                plv->iMark = -1;
            }

            for (; i  < iEnd; i++) {
                RECT dummy;
                ListView_GetRects(plv, i, NULL, NULL, NULL, &rcTemp2);

                // don't do this infaltion if we're in report&full row mode
                // in that case, just touching is good enough
                if (!(ListView_IsReportView(plv) && ListView_FullRowSelect(plv))) {
                    int cxInflate = (rcTemp2.right - rcTemp2.left) / 4;
                    if (ListView_IsListView(plv)) {
                        cxInflate = min(cxInflate, plv->cxSmIcon);
                    }
                    InflateRect(&rcTemp2, -cxInflate, -(rcTemp2.bottom - rcTemp2.top) / 4);
                }

                bInOld = (IntersectRect(&dummy, &rcOld, &rcTemp2) != 0);
                bInNew = (IntersectRect(&dummy, &rc, &rcTemp2) != 0);

                if (msg32.wParam & MK_CONTROL) {
                    if (bInOld != bInNew) {
                        ListView_ToggleSelection(plv, i);
                    }
                } else {
                    // was there a change?
                    if (bInOld != bInNew) {
                        ListView_OnSetItemState(plv, i, bInOld ? 0 : LVIS_SELECTED, LVIS_SELECTED);
                    }

                    // if no alternate keys are down.. set the mark to
                    // the item furthest from the cursor
                    if (bInNew && !(msg32.wParam & (MK_CONTROL | MK_SHIFT))) {
                        int dItem;
                        dItem = (rcTemp2.left - pt.x) * (rcTemp2.left - pt.x) +
                            (rcTemp2.top - pt.y) * (rcTemp2.top - pt.y);
                        // if it's further away, set this as the mark
                        //DebugMsg(TF_LISTVIEW, "dItem = %d, dMax = %d", dItem, dMax);
                        if (dItem > dMax) {
                            //DebugMsg(TF_LISTVIEW, "taking dItem .. iMark = %d", i);
                            dMax = dItem;
                            plv->iMark = i;
                        }
                    }
                }
            }

            //DebugMsg(TF_LISTVIEW, "Final iMark = %d", plv->iMark);
            if (bLocked) {
                if (GetUpdateRgn(plv->ci.hwnd, hrgnUpdate, FALSE) > NULLREGION) {
                    ValidateRect(plv->ci.hwnd, NULL);
                    GetWindowRgn(plv->ci.hwnd, hrgnLV);
                    CombineRgn(hrgnUpdate, hrgnUpdate, hrgnLV, RGN_AND);
                    SelectClipRgn(hdc, hrgnUpdate);
                    SendMessage(plv->ci.hwnd, WM_PRINTCLIENT, (WPARAM)hdc, 0);
                    SelectClipRgn(hdc, NULL);
                }
            } else {
                UpdateWindow(plv->ci.hwnd);    // make selection draw
            }


            DrawFocusRect(hdc, &rc);

            rcOld = rc;
            break;
        }

        case WM_KEYDOWN:
            switch (msg32.wParam) {
            case VK_ESCAPE:
                ListView_DeselectAll(plv, -1);
                goto EndOfLoop;
            }
        case WM_CHAR:
        case WM_KEYUP:
            // don't process thay keyboard stuff during marquee
            break;


        default:

            // don't process mouse wheel stuff
            if (msg32.message == g_msgMSWheel)
                break;

        DoDefault:
            TranslateMessage32(&msg32, TRUE);
            DispatchMessage32(&msg32, TRUE);
        }
    }

EndOfLoop:
    DrawFocusRect(hdc, &rcOld); // erase old
    ReleaseDC(hwnd, hdc);

BailOut:
    if (hrgnUpdate)
        DeleteObject(hrgnUpdate);
    if (hrgnLV)
        DeleteObject(hrgnLV);
    if (bLocked)
        LockWindowUpdate(NULL);
}


#define SHIFT_DOWN(keyFlags)    (keyFlags & MK_SHIFT)
#define CONTROL_DOWN(keyFlags)  (keyFlags & MK_CONTROL)
#define RIGHTBUTTON(keyFlags)   (keyFlags & MK_RBUTTON)

void PASCAL ListView_ButtonSelect(LV* plv, int iItem, UINT keyFlags, BOOL bSelected)
{
    if (SHIFT_DOWN(keyFlags))
    {
        ListView_SelectRangeTo(plv, iItem, !CONTROL_DOWN(keyFlags));
        ListView_SetFocusSel(plv, iItem, TRUE, FALSE, FALSE);
    }
    else if (!CONTROL_DOWN(keyFlags))
    {
        ListView_SetFocusSel(plv, iItem, TRUE, !bSelected, FALSE);
    }
}

void ListView_HandleStateIconClick(LV* plv, int iItem)
{
    int iState =
        ListView_OnGetItemState(plv, iItem, LVIS_STATEIMAGEMASK);

    iState = STATEIMAGEMASKTOINDEX(iState) -1;
    iState++;
    iState %= ImageList_GetImageCount(plv->himlState);
    iState++;
    ListView_OnSetItemState(plv, iItem, INDEXTOSTATEIMAGEMASK(iState), LVIS_STATEIMAGEMASK);
}

BOOL ListView_RBeginMarquee(LV* plv, int x, int y, LPLVHITTESTINFO plvhti)
{
    if (ListView_FullRowSelect(plv) &&
        ListView_IsReportView(plv) &&
        !(plv->ci.style & LVS_SINGLESEL) &&
        !ListView_OwnerDraw(plv) &&
        plvhti->iSubItem == 0) {
        // can only begin marquee in column 0.

        if (plvhti->flags == LVHT_ONITEM) {
            return TRUE;
        }
    }

    return FALSE;
}

void NEAR ListView_HandleMouse(LV* plv, BOOL fDoubleClick, int x, int y, UINT keyFlags, BOOL bMouseWheel)
{
    LV_HITTESTINFO ht;
    NMITEMACTIVATE nm;
    int iItem, click, drag;
    BOOL bSelected, fHadFocus, fNotifyReturn = FALSE;
    BOOL fActive;
    HWND hwnd = plv->ci.hwnd;

    if (plv->fButtonDown)
        return;
    plv->fButtonDown = TRUE;


    if (plv->exStyle & LVS_EX_ONECLICKACTIVATE && plv->fOneClickHappened && plv->fOneClickOK)
    {
        KillTimer(plv->ci.hwnd, IDT_ONECLICKHAPPENED);
        plv->fOneClickHappened = FALSE;
        CCSendNotify(&plv->ci, LVN_ITEMACTIVATE, &(plv->nmOneClickHappened.hdr));
        if (!IsWindow(hwnd))
            return;
    }

    fHadFocus = (GetFocus() == plv->ci.hwnd);
    click = RIGHTBUTTON(keyFlags) ? NM_RCLICK : NM_CLICK;
    drag  = RIGHTBUTTON(keyFlags) ? LVN_BEGINRDRAG : LVN_BEGINDRAG;

    fActive = ChildOfActiveWindow(plv->ci.hwnd) || fShouldFirstClickActivate() ||
              ChildOfDesktop(plv->ci.hwnd);

#ifdef LVDEBUG
    DebugMsg(TF_LISTVIEW, TEXT("ListView_OnButtonDown %d"), fDoubleClick);
#endif

    SetCapture(plv->ci.hwnd);

    plv->ptCapture.x = x;
    plv->ptCapture.y = y;


    if (!ListView_DismissEdit(plv, FALSE) && GetCapture() != plv->ci.hwnd)
        goto EndButtonDown;
    CCReleaseCapture(&plv->ci);

    // REVIEW: right button implies no shift or control stuff
    // Single selection style also implies no modifiers
    //if (RIGHTBUTTON(keyFlags) || (plv->ci.style & LVS_SINGLESEL))
    if ((plv->ci.style & LVS_SINGLESEL))
        keyFlags &= ~(MK_SHIFT | MK_CONTROL);

    ht.pt.x = x;
    ht.pt.y = y;
    iItem = ListView_OnSubItemHitTest(plv, &ht);
    if (ht.iSubItem != 0) {
        // if we're not in full row select,
        // hitting on a subitem is like hitting on nowhere
        // also, in win95, ownerdraw fixed effectively had full row select
        if (!ListView_FullRowSelect(plv) &&
            !(plv->ci.style & LVS_OWNERDRAWFIXED)) {
            iItem = -1;
            ht.flags = LVHT_NOWHERE;
        }
    }

    nm.iItem = iItem;
    nm.iSubItem = ht.iSubItem;
    nm.uChanged = 0;
    nm.ptAction.x = x;
    nm.ptAction.y = y;
    nm.uKeyFlags = GetLVKeyFlags();

    // FProt Profesional assumed that if the notification structure pointer + 14h bytes
    // had a value 2 that it was a displayinfo structure and they then used offset +2c as lparam...
    nm.uNewState = 0;

    plv->iNoHover = iItem;


    bSelected = (iItem >= 0) && ListView_OnGetItemState(plv, iItem, LVIS_SELECTED);

    if (fDoubleClick)
    {
        //
        // Cancel any name editing that might happen.
        //
        ListView_CancelPendingEdit(plv);
        KillTimer(plv->ci.hwnd, IDT_SCROLLWAIT);

        if (ht.flags & LVHT_NOWHERE) {
            // this would have been done in the first click in win95 except
            // now we blow off the first click on focus change
            if (!SHIFT_DOWN(keyFlags) && !CONTROL_DOWN(keyFlags))
                ListView_DeselectAll(plv, -1);
        }

        click = RIGHTBUTTON(keyFlags) ? NM_RDBLCLK : NM_DBLCLK ;
        if (CCSendNotify(&plv->ci, click, &nm.hdr))
            goto EndButtonDown;

        /// some (comdlg32 for example) destroy on double click
        // we need to bail if that happens because plv is no longer valid
        if (!IsWindow(hwnd))
            return;

        if (click == NM_DBLCLK)
        {
            // these shift control flags are to mirror when we don't send out the activate on the single click,
            // but are in the oneclick activate mode  (see below)
            if (ht.flags & (LVHT_ONITEMLABEL | LVHT_ONITEMICON))
            {
                // possible scenarios below:
                // 1) we're using classic windows style so double click => launch
                // 2) we're using single click activate
                //    a) shift is down and item is selected => launch
                //       this implies that the first click selected it
                //    b) control is down => launch
                //       the first click toggled the selection so if the item was
                //       the only item selected and we double clicked on it
                //       the first click deselects it and no item is selected
                //       so nothing will be launched - this is win95 behavior
                if (!(plv->exStyle & LVS_EX_ONECLICKACTIVATE && plv->fOneClickOK) ||
                    (plv->exStyle & LVS_EX_ONECLICKACTIVATE &&  plv->fOneClickOK &&
                     (SHIFT_DOWN(keyFlags) || CONTROL_DOWN(keyFlags))))
                {
                    CCSendNotify(&plv->ci, LVN_ITEMACTIVATE, &nm.hdr);
                }
            }
            // Double-click on checkbox state icon cycles it just like single click
            else if ((ht.flags & LVHT_ONITEMSTATEICON) && ListView_CheckBoxes(plv)) {
                ListView_HandleStateIconClick(plv, iItem);
            }
        }

        if (!IsWindow(hwnd))
            return;
        goto EndButtonDown;
    }

    if (ht.flags & (LVHT_ONITEMLABEL | LVHT_ONITEMICON))
    {

        // if it wasn't selected, we're about to select it... play
        // a little ditty for us...
        CCPlaySound(c_szSelect);

        if (!RIGHTBUTTON(keyFlags) || (!CONTROL_DOWN(keyFlags) && !SHIFT_DOWN(keyFlags)))
            ListView_ButtonSelect(plv, iItem, keyFlags, bSelected);

        // handle full row select
        // If single-select listview, disable marquee selection.
        //
        // Careful - CheckForDragBegin yields and the app may have
        // destroyed the item we were thinking about dragging!
        //
        if (!bMouseWheel && CheckForDragBegin(plv->ci.hwnd, x, y))
        {
            // should we do a marquee?
            if (ListView_RBeginMarquee(plv, x, y, &ht) &&
                !CCSendNotify(&plv->ci, LVN_MARQUEEBEGIN, &nm.hdr))
            {
                ListView_DragSelect(plv, x, y);
                fNotifyReturn = !CCSendNotify(&plv->ci, click, &nm.hdr);
            }
            else
            {
                // Before we start dragging, make it sure that it is
                // selected and has the focus.
                ListView_SetFocusSel(plv, iItem, TRUE, FALSE, FALSE);

                if (!SHIFT_DOWN(keyFlags))
                    plv->iMark = iItem;

                // Then, we need to update the window before start dragging
                // to show the selection chagne.
                UpdateWindow(plv->ci.hwnd);

                CCSendNotify(&plv->ci, drag, &nm.hdr);

                goto EndButtonDown;
            }
        }

        // CheckForDragBegin yields, so revalidate before continuing
        else if (IsWindow(hwnd))
        {
            // button came up and we are not dragging

            if (!RIGHTBUTTON(keyFlags))
            {
                if (CONTROL_DOWN(keyFlags))
                {
                    // do this on the button up so that ctrl-dragging a range
                    // won't toggle the select.

                    if (SHIFT_DOWN(keyFlags))
                        ListView_SetFocusSel(plv, iItem, FALSE, FALSE, FALSE);
                    else
                    {
                        ListView_SetFocusSel(plv, iItem, TRUE, FALSE, TRUE);
                    }
                }
            }
            if (!SHIFT_DOWN(keyFlags))
                plv->iMark = iItem;

            if (!ListView_SetFocus(plv->ci.hwnd))    // activate this window
                return;

            // now do the deselect stuff
            if (!SHIFT_DOWN(keyFlags) && !CONTROL_DOWN(keyFlags) && !RIGHTBUTTON(keyFlags))
            {
                ListView_DeselectAll(plv, iItem);
                if ((ht.flags & LVHT_ONITEMLABEL) && bSelected &&
                    !(plv->exStyle & (LVS_EX_ONECLICKACTIVATE|LVS_EX_TWOCLICKACTIVATE)))
                {

                    // doing this check for ownerdrawfixed is for compatability.
                    // we don't want to go into edit mode if the user just happened to click
                    // to this window when a different one had focus,
                    // but ms hammer relied upon the notification being sent (and we
                    // don't go into edit mode anyways for ownerdraw)
                    if (fHadFocus ||
                        (plv->ci.style & LVS_OWNERDRAWFIXED)) {
                        // Click on item label.  It was selected and
                        // no modifier keys were pressed and no drag operation
                        // So setup for name edit mode.  Still need to wait
                        // to make sure user is not doing double click.
                        //
                        ListView_SetupPendingNameEdit(plv);
                    }
                }
            }

            fNotifyReturn = !CCSendNotify(&plv->ci, click, &nm.hdr);
            if (!IsWindow(hwnd))
                return;

            if (plv->exStyle & (LVS_EX_ONECLICKACTIVATE|LVS_EX_TWOCLICKACTIVATE))
            {
                if (!RIGHTBUTTON(keyFlags))
                {
                    // We don't ItemActivate within one double-click time of creating
                    // this listview. This is a common occurence for people used to
                    // double-clicking. The first click pops up a new window which
                    // receives the second click and ItemActivates the item...
                    //
                    if ((plv->exStyle & LVS_EX_ONECLICKACTIVATE && plv->fOneClickOK) || bSelected)
                    {
                        if (fActive)
                        {
                            // condition: if we're in a single click activate mode
                            // don't launch if control or shift keys are pressed
                            BOOL bCond = plv->exStyle & LVS_EX_ONECLICKACTIVATE && !CONTROL_DOWN(keyFlags) && !SHIFT_DOWN(keyFlags);

                            if ((bSelected && plv->exStyle & LVS_EX_TWOCLICKACTIVATE) ||
                                (bCond && !g_bUseDblClickTimer))
                            {
                                CCSendNotify(&plv->ci, LVN_ITEMACTIVATE, &nm.hdr);
                                if (!IsWindow(hwnd))
                                    return;
                            }
                            else if (bCond && g_bUseDblClickTimer)
                            {
                                plv->fOneClickHappened = TRUE;
                                plv->nmOneClickHappened = nm;
                                SetTimer(plv->ci.hwnd, IDT_ONECLICKHAPPENED, GetDoubleClickTime(), NULL);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            // IsWindow() failed.  Bail.
            return;
        }
    }
    else if (ht.flags & LVHT_ONITEMSTATEICON)
    {
        // Should activate window and send notificiation to parent...
        if (!ListView_SetFocus(plv->ci.hwnd))   // activate this window
            return;
        fNotifyReturn = !CCSendNotify(&plv->ci, click, &nm.hdr);
        if (fNotifyReturn && ListView_CheckBoxes(plv)) {
            ListView_HandleStateIconClick(plv, iItem);
        }
    }
    else if (ht.flags & LVHT_NOWHERE)
    {
        if (!ListView_SetFocus(plv->ci.hwnd))   // activate this window
            return;

        // If single-select listview, disable marquee selection.
        if (!(plv->ci.style & LVS_SINGLESEL) && CheckForDragBegin(plv->ci.hwnd, x, y) &&
            !CCSendNotify(&plv->ci, LVN_MARQUEEBEGIN, &nm.hdr))
        {
            if (!SHIFT_DOWN(keyFlags) && !CONTROL_DOWN(keyFlags))
                ListView_DeselectAll(plv, -1);
            ListView_DragSelect(plv, x, y);
            fNotifyReturn = !CCSendNotify(&plv->ci, click, &nm.hdr);
        } else if (IsWindow(hwnd)) {
            // if we didn't have focus and aren't showing selection always,
            // make the first click just set focus
            BOOL fDoFirstClickSelection = (fHadFocus || plv->ci.style & LVS_SHOWSELALWAYS ||
                                           CONTROL_DOWN(keyFlags) || SHIFT_DOWN(keyFlags) ||
                                           RIGHTBUTTON(keyFlags));

            if (fDoFirstClickSelection && fActive) {

                if (!SHIFT_DOWN(keyFlags) && !CONTROL_DOWN(keyFlags))
                    ListView_DeselectAll(plv, -1);

                fNotifyReturn = !CCSendNotify(&plv->ci, click, &nm.hdr);
            }
        }
        else
        {
            // IsWindow() failed.  Bail.
            return;
        }
    }

    // re-check the key state so we don't get confused by multiple clicks

    // this needs to check the GetKeyState stuff only when we've gone into
    // a modal loop waiting for the rbutton up.
    if (fNotifyReturn && (click == NM_RCLICK)) // && (GetKeyState(VK_RBUTTON)>=0))
    {
        POINT pt = { x, y };
        ClientToScreen(plv->ci.hwnd, &pt);
        FORWARD_WM_CONTEXTMENU(plv->ci.hwnd, plv->ci.hwnd, pt.x, pt.y, SendMessage);
    }

EndButtonDown:
    if (IsWindow(hwnd))
        plv->fButtonDown = FALSE;
}

void NEAR ListView_OnButtonDown(LV* plv, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    ListView_HandleMouse(plv, fDoubleClick, x, y, keyFlags, FALSE);
}

#define ListView_CancelPendingEdit(plv) ListView_CancelPendingTimer(plv, LVF_NMEDITPEND, IDT_NAMEEDIT)
#define ListView_CancelScrollWait(plv) ListView_CancelPendingTimer(plv, LVF_SCROLLWAIT, IDT_SCROLLWAIT)

BOOL NEAR ListView_CancelPendingTimer(LV* plv, UINT fFlags, int idTimer)
{
    if (plv->flags & fFlags)
    {
        KillTimer(plv->ci.hwnd, idTimer);
        plv->flags &= ~fFlags;
        return TRUE;
    }
    return FALSE;
}

//
// ListView_OnTimer:
//     process the WM_TIMER message.  If the timer id is thta
//     of the name editing, we should then start the name editing mode.
//
void NEAR ListView_OnTimer(LV* plv, UINT id)
{
    KillTimer(plv->ci.hwnd, id);

    if (id == IDT_NAMEEDIT)
    {
        // Kill the timer as we wont need any more messages from it.

        if (ListView_CancelPendingEdit(plv)) {
            // And start name editing mode.
            if (!ListView_OnEditLabel(plv, plv->iFocus, NULL))
            {
                ListView_DismissEdit(plv, FALSE);
                ListView_SetFocusSel(plv, plv->iFocus, TRUE, TRUE, FALSE);
            }
        }
    } else if (id == IDT_SCROLLWAIT) {

        if (ListView_CancelScrollWait(plv)) {
            ListView_OnEnsureVisible(plv, plv->iFocus, TRUE);
        }
    } else if (id == IDT_ONECLICKOK) {
        plv->fOneClickOK = TRUE;
    } else if (id == IDT_ONECLICKHAPPENED) {
        //if (!g_bUseDblClickTimer)
        //{
        ////    EnableWindow(plv->ci.hwnd, TRUE);
        //    SetWindowBits(plv->ci.hwnd, GWL_STYLE, WS_DISABLED, 0);
        //    plv->fOneClickHappened = FALSE;
        //}
        // check the bit just in case they double-clicked
        //else
        if (plv->fOneClickHappened)
        {
            plv->fOneClickHappened = FALSE;
            CCSendNotify(&plv->ci, LVN_ITEMACTIVATE, &(plv->nmOneClickHappened.hdr));
        }
    }
}

//
// ListView_SetupPendingNameEdit:
//      Sets up a timer to begin name editing at a delayed time.  This
//      will allow the user to double click on the already selected item
//      without going into name editing mode, which is especially important
//      in those views that only show a small icon.
//
void NEAR ListView_SetupPendingNameEdit(LV* plv)
{
    SetTimer(plv->ci.hwnd, IDT_NAMEEDIT, GetDoubleClickTime(), NULL);
    plv->flags |= LVF_NMEDITPEND;
}

void NEAR PASCAL ListView_OnHVScroll(LV* plv, UINT code, int pos, int sb)
{
    int iScrollCount = 0;

#ifdef SIF_TRACKPOS
    SCROLLINFO si;

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_TRACKPOS;


    // if we're in 32bits, don't trust the pos since it's only 16bit's worth
    if (ListView_GetScrollInfo(plv, sb, &si))
        pos = (int)si.nTrackPos;
#endif
    ListView_DismissEdit(plv, FALSE);


    _ListView_OnScroll(plv, code, pos, sb);

    switch (code) {
    case SB_PAGELEFT:
    case SB_PAGERIGHT:
        if (plv->iScrollCount < SMOOTHSCROLLLIMIT)
            plv->iScrollCount += 3;
        break;

    case SB_LINELEFT:
    case SB_LINERIGHT:
        if (plv->iScrollCount < SMOOTHSCROLLLIMIT)
            plv->iScrollCount++;
        break;

    case SB_ENDSCROLL:
        plv->iScrollCount = 0;
        break;

    }
}

void NEAR ListView_OnVScroll(LV* plv, HWND hwndCtl, UINT code, int pos)
{
    ListView_OnHVScroll(plv, code, pos, SB_VERT);
}

void NEAR ListView_OnHScroll(LV* plv, HWND hwndCtl, UINT code, int pos)
{
    ListView_OnHVScroll(plv, code, pos, SB_HORZ);
}

int ListView_ValidateOneScrollParam(LV* plv, int iDirection, int dx)
{
    SCROLLINFO si;

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;



    if (!ListView_GetScrollInfo(plv, iDirection, &si))
        return 0;

    si.nMax -= (si.nPage - 1);
    si.nPos += dx;
    if (si.nPos < si.nMin) {
        dx += (int)(si.nMin - si.nPos);
    } else if (si.nPos > si.nMax) {
        dx -= (int)(si.nPos - si.nMax);
    }

    return dx;
}

BOOL NEAR PASCAL ListView_ValidateScrollParams(LV* plv, int FAR * pdx, int FAR *pdy)
{
    int dx = *pdx;
    int dy = *pdy;

    if (plv->ci.style & LVS_NOSCROLL)
        return FALSE;

    if (ListView_IsListView(plv))
    {
        ListView_MaybeResizeListColumns(plv, 0, ListView_Count(plv)-1);
#ifdef COLUMN_VIEW
        if (dx < 0)
            dx = (dx - plv->cxItem - 1) / plv->cxItem;
        else
            dx = (dx + plv->cxItem - 1) / plv->cxItem;

        if (dy)
            return FALSE;
#else
        if (dy < 0)
            dy = (dy - plv->cyItem - 1) / plv->cyItem;
        else
            dy = (dy + plv->cyItem - 1) / plv->cyItem;

        if (dx)
            return FALSE;
#endif
    }
    else if (ListView_IsReportView(plv))
    {
        //
        // Note: This function expects that dy is in number of lines
        // and we are working with pixels so do a conversion use some
        // rounding up and down to make it right
        if (dy > 0)
            dy = (dy + plv->cyItem/2) / plv->cyItem;
        else
            dy = (dy - plv->cyItem/2) / plv->cyItem;
    }

    if (dy) {
        dy = ListView_ValidateOneScrollParam(plv, SB_VERT, dy);
        if (ListView_IsReportView(plv)
#ifndef COLUMN_VIEW
            || ListView_IsListView(plv)
#endif
            ) {
           // convert back to pixels
           dy *= plv->cyItem;
        }
        *pdy = dy;
    }

    if (dx) {

        dx = ListView_ValidateOneScrollParam(plv, SB_HORZ, dx);
#ifdef COLUMN_VIEW
        if (ListView_IsListView(plv)) {
            dx *= plv->cxItem;
        }
#endif
        *pdx = dx;
    }

    return TRUE;
}


BOOL NEAR ListView_OnScroll(LV* plv, int dx, int dy)
{

    if (plv->ci.style & LVS_NOSCROLL)
        return FALSE;

    if (ListView_IsListView(plv))
    {
        // Scale pixel count to column count
        //
#ifdef COLUMN_VIEW
        if (dx < 0)
            dx -= plv->cxItem - 1;
        else
            dx += plv->cxItem - 1;

        dx = dx / plv->cxItem;

        if (dy)
            return FALSE;
#else
        if (dy < 0)
            dy -= plv->cyItem - 1;
        else
            dy += plv->cyItem - 1;

        dy = dy / plv->cyItem;

        if (dx)
            return FALSE;
#endif
    }
    else if (ListView_IsReportView(plv))
    {
        //
        // Note: This function expects that dy is in number of lines
        // and we are working with pixels so do a conversion use some
        // rounding up and down to make it right
        if (dy > 0)
            dy = (dy + plv->cyItem/2) / plv->cyItem;
        else
            dy = (dy - plv->cyItem/2) / plv->cyItem;
    }
    _ListView_Scroll2(plv, dx, dy, 0);
    ListView_UpdateScrollBars(plv);
    return TRUE;
}

BOOL NEAR ListView_OnEnsureVisible(LV* plv, int i, BOOL fPartialOK)
{
        RECT rcBounds;
        RECT rc;
        int dx, dy;

    if (!ListView_IsValidItemNumber(plv, i) || plv->ci.style & LVS_NOSCROLL)
        return FALSE;

    // we need to do this again inside because some callers don't do it.
    // other callers that do this need to do it outside so that
    // they can know not to call us if there's not wait pending
    ListView_CancelScrollWait(plv);

    if (ListView_IsReportView(plv))
        return ListView_ROnEnsureVisible(plv, i, fPartialOK);


        ListView_GetRects(plv, i, &rc, NULL, &rcBounds, NULL);

        if (!fPartialOK)
                rc = rcBounds;

    // If any part of rc is outside of rcClient, then
    // scroll so that all of rcBounds is visible.
    //
        dx = 0;
        if (rc.left < 0 || rc.right >= plv->sizeClient.cx)
        {
                dx = rcBounds.left - 0;
                if (dx >= 0)
                {
                        dx = rcBounds.right - plv->sizeClient.cx;
                        if (dx <= 0)
                                dx = 0;
                        else if ((rcBounds.left - dx) < 0)
                                dx = rcBounds.left - 0; // Not all fits...
                }
        }
        dy = 0;
        if (rc.top < 0 || rc.bottom >= plv->sizeClient.cy)
        {
                dy = rcBounds.top - 0;
                if (dy >= 0)
                {
                        dy = rcBounds.bottom - plv->sizeClient.cy;
                        if (dy < 0)
                                dy = 0;
                }
        }

    if (dx | dy)
        return ListView_OnScroll(plv, dx, dy);

    return TRUE;
}

void NEAR ListView_UpdateScrollBars(LV* plv)
{
    RECT rc;
    DWORD dwStyle;

    if ((plv->ci.style & LVS_NOSCROLL) ||
        (!(ListView_RedrawEnabled(plv))))
        return;

    _ListView_UpdateScrollBars(plv);

    GetClientRect(plv->ci.hwnd, &rc);
    plv->sizeClient.cx = rc.right;
    plv->sizeClient.cy = rc.bottom;

    dwStyle = ListView_GetWindowStyle(plv);
    plv->ci.style = (plv->ci.style & ~(WS_HSCROLL | WS_VSCROLL)) | (dwStyle & WS_HSCROLL | WS_VSCROLL);
}

#ifndef WINNT
#pragma optimize ("gle", off)
// Crappy hack for Sage, which passes unitialized memory to SetWindowPlacement.
// They used to get lucky and get zeros for the max position, but now they end
// up with non-zero stack trash that causes bad things to happen when sage is
// maximized.  Thus, zero a bunch of stack to save their tail...
void ZeroSomeStackForSage()
{
    BYTE aByte[1024];

    memset(aByte, 0, sizeof(aByte));

    aByte;
}
#pragma optimize ("", on)
#endif

void NEAR ListView_OnSetFont(LV* plv, HFONT hfont, BOOL fRedraw)
{
    HDC hdc;
    SIZE siz;
    LOGFONT lf;
    HFONT hfontPrev;
 
    if ((plv->flags & LVF_FONTCREATED) && plv->hfontLabel) {
        DeleteObject(plv->hfontLabel);
        plv->flags &= ~LVF_FONTCREATED;
    }

    if (hfont == NULL) {
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
        hfont = CreateFontIndirect(&lf);
        plv->flags |= LVF_FONTCREATED;
    }

    hdc = GetDC(HWND_DESKTOP);

    hfontPrev = SelectFont(hdc, hfont);

    GetTextExtentPoint(hdc, TEXT("0"), 1, &siz);

    plv->cyLabelChar = siz.cy;
    plv->cxLabelChar = siz.cx;

    GetTextExtentPoint(hdc, c_szEllipses, CCHELLIPSES, &siz);
    plv->cxEllipses = siz.cx;

    SelectFont(hdc, hfontPrev);
    ReleaseDC(HWND_DESKTOP, hdc);

    plv->hfontLabel = hfont;

    plv->ci.uiCodePage = GetCodePageForFont(hfont);

    ListView_InvalidateCachedLabelSizes(plv);

    /* Ensure that our tooltip control uses the same font as the list view is using, therefore
    /  avoiding any nasty formatting problems. */

    if ( plv->hwndToolTips )
    {
        FORWARD_WM_SETFONT( plv->hwndToolTips, plv->hfontLabel, FALSE, SendMessage );
    }

    // If we have a header window, we need to forward this to it also
    // as we have destroyed the hfont that they are using...
    if (plv->hwndHdr) {
        FORWARD_WM_SETFONT(plv->hwndHdr, plv->hfontLabel, FALSE, SendMessage);
        ListView_UpdateScrollBars(plv);
    }

    if (plv->hFontHot) {
        DeleteObject(plv->hFontHot);
        plv->hFontHot = NULL;
    }

    CCGetHotFont(plv->hfontLabel, &plv->hFontHot);
    plv->iFreeSlot = -1;

    if (fRedraw)
        RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
    #ifndef WINNT
    ZeroSomeStackForSage();
    #endif
}

HFONT NEAR ListView_OnGetFont(LV* plv)
{
    return plv->hfontLabel;
}

// This function process the WM_SETREDRAW message by setting or clearing
// a bit in the listview structure, which several places in the code will
// check...
//
// REVIEW: Should probably forward to DefWindowProc()
//
void NEAR ListView_OnSetRedraw(LV* plv, BOOL fRedraw)
{
    if (fRedraw)
    {
        BOOL fChanges = FALSE;
        // Only do work if we're turning redraw back on...
        //
        if (!(plv->flags & LVF_REDRAW))
        {
            plv->flags |= LVF_REDRAW;

            // deal with any accumulated invalid regions
            if (plv->hrgnInval)
            {
                UINT fRedraw = (plv->flags & LVF_ERASE) ? RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW : RDW_UPDATENOW|RDW_INVALIDATE;
                if (plv->hrgnInval == (HRGN)ENTIRE_REGION)
                    plv->hrgnInval = NULL;

                RedrawWindow(plv->ci.hwnd, NULL, plv->hrgnInval, fRedraw);
                ListView_DeleteHrgnInval(plv);
                fChanges = TRUE;
            }
            plv->flags &= ~LVF_ERASE;

            // now deal with the optimized stuff
            if (ListView_IsListView(plv) || ListView_IsReportView(plv))
            {
                if (plv->iFirstChangedNoRedraw != -1)
                {
                    // We may try to resize the column
                    if (!ListView_MaybeResizeListColumns(plv, plv->iFirstChangedNoRedraw,
                            ListView_Count(plv)-1))
                        ListView_OnUpdate(plv, plv->iFirstChangedNoRedraw);
                }
                else
                    ListView_UpdateScrollBars(plv);
            } else {
                int iCount;

                if (plv->iFirstChangedNoRedraw != -1) {
                    for (iCount = ListView_Count(plv) ; plv->iFirstChangedNoRedraw < iCount; plv->iFirstChangedNoRedraw++) {
                        ListView_InvalidateItem(plv, plv->iFirstChangedNoRedraw, FALSE, RDW_INVALIDATE);
                    }
                    fChanges = TRUE;
                }
                if (fChanges)
                    ListView_RecalcRegion(plv, TRUE, TRUE);
                if ((plv->ci.style & LVS_AUTOARRANGE) && fChanges) {
                    ListView_OnUpdate(plv, plv->iFirstChangedNoRedraw);
                } else {
                    ListView_UpdateScrollBars(plv);
                }
            }
        }
    }
    else
    {
        plv->iFirstChangedNoRedraw = -1;
        plv->flags &= ~LVF_REDRAW;
    }
}

HIMAGELIST NEAR ListView_OnGetImageList(LV* plv, int iImageList)
{
    switch (iImageList)
    {
        case LVSIL_NORMAL:
            return plv->himl;

        case LVSIL_SMALL:
            return plv->himlSmall;

        case LVSIL_STATE:
            return plv->himlState;
    }
    ASSERT(0);
    return NULL;
}


HIMAGELIST NEAR ListView_OnSetImageList(LV* plv, HIMAGELIST himl, int iImageList)
{
    HIMAGELIST hImageOld = NULL;

    switch (iImageList)
    {
        case LVSIL_NORMAL:
            hImageOld = plv->himl;
            plv->himl = himl;
            if (himl) {
                ImageList_GetIconSize(himl, &plv->cxIcon , &plv->cyIcon);

                if (!(plv->flags & LVF_ICONSPACESET)) {
                    ListView_OnSetIconSpacing(plv, (LPARAM)-1);
                }
            }
            break;

        case LVSIL_SMALL:
            hImageOld = plv->himlSmall;
            plv->himlSmall = himl;
            if (himl)
                ImageList_GetIconSize(himl, &plv->cxSmIcon , &plv->cySmIcon);
            plv->cxItem = 16 * plv->cxLabelChar + plv->cxSmIcon;
            plv->cyItem = ListView_ComputeCYItemSize(plv);
            if (plv->hwndHdr)
                SendMessage(plv->hwndHdr, HDM_SETIMAGELIST, 0, (LPARAM)himl);
            break;

        case LVSIL_STATE:
            if (himl) {
                ImageList_GetIconSize(himl, &plv->cxState , &plv->cyState);
            } else {
                plv->cxState = 0;
            }
            hImageOld = plv->himlState;
            plv->himlState = himl;
            plv->cyItem = ListView_ComputeCYItemSize(plv);
            break;

        default:
#ifdef LVDEBUG
            DebugMsg(TF_LISTVIEW, TEXT("sh TR - LVM_SETIMAGELIST: unrecognized iImageList"));
#endif
            break;
    }

    if (himl && !(plv->ci.style & LVS_SHAREIMAGELISTS))
        ImageList_SetBkColor(himl, plv->clrBk);

    if (ListView_Count(plv) > 0)
        InvalidateRect(plv->ci.hwnd, NULL, TRUE);

    return hImageOld;
}

#ifdef UNICODE
BOOL NEAR ListView_OnGetItemA(LV* plv, LV_ITEMA *plvi) {
    LPWSTR pszW = NULL;
    LPSTR pszC = NULL;
    BOOL fRet;

    //HACK ALERT -- this code assumes that LV_ITEMA is exactly the same
    // as LV_ITEMW except for the pointer to the string.
    ASSERT(sizeof(LV_ITEMA) == sizeof(LV_ITEMW))

    if (!plvi)
        return FALSE;

    if ((plvi->mask & LVIF_TEXT) && (plvi->pszText != NULL)) {
        pszC = plvi->pszText;
        pszW = LocalAlloc(LMEM_FIXED, plvi->cchTextMax * sizeof(WCHAR));
        if (pszW == NULL)
            return FALSE;
        plvi->pszText = (LPSTR)pszW;
    }

    fRet = ListView_OnGetItem(plv, (LV_ITEM *) plvi);

    if (pszW)
    {
        if (plvi->pszText != LPSTR_TEXTCALLBACKA)
        {
            if (fRet && plvi->cchTextMax)
                ConvertWToAN(plv->ci.uiCodePage, pszC, plvi->cchTextMax, (LPWSTR)plvi->pszText, -1);
            plvi->pszText = pszC;
        }

        LocalFree(pszW);
    }

    return fRet;

}
#endif

BOOL NEAR ListView_OnGetItem(LV* plv, LV_ITEM FAR* plvi)
{
    UINT mask;
    LISTITEM FAR* pitem = NULL;
    LV_DISPINFO nm;

    if (!plvi)
    {
        RIPMSG(0, "LVM_GET(ITEM|ITEMTEXT): Invalid pitem = NULL");
        return FALSE;
    }

    if (!ListView_IsValidItemNumber(plv, plvi->iItem))
    {
#ifdef DEBUG
        // owner data views (e.g. docfind) may change the number of items in listview
        // while we are doing something, thus hitting this rip
        if (!ListView_IsOwnerData(plv))
            RIPMSG(0, "LVM_GET(ITEM|ITEMTEXT|ITEMSTATE): item=%d does not exist", plvi->iItem);
#endif
        return FALSE;
    }

    nm.item.mask = 0;
    mask = plvi->mask;

    if (!ListView_IsOwnerData(plv))
    {
        // Standard listviews
        pitem = ListView_FastGetItemPtr(plv, plvi->iItem);
        ASSERT(pitem);

        // Handle sub-item cases for report view
        //
        if (plvi->iSubItem != 0)
        {
            LISTSUBITEM lsi;

            ListView_GetSubItem(plv, plvi->iItem, plvi->iSubItem, &lsi);
            if (mask & LVIF_TEXT)
            {
                if (lsi.pszText != LPSTR_TEXTCALLBACK)
                {
                    Str_GetPtr0(lsi.pszText, plvi->pszText, plvi->cchTextMax);
                } else {
                    // if this is LVIF_NORECOMPUTE we will update pszText later
                    nm.item.mask |= LVIF_TEXT;
                }
            }

            if ((mask & LVIF_IMAGE) && (plv->exStyle & LVS_EX_SUBITEMIMAGES))
            {
                plvi->iImage = lsi.iImage;
                if (lsi.iImage == I_IMAGECALLBACK)
                    nm.item.mask |= LVIF_IMAGE;
            }

            if (mask & LVIF_STATE) {

                if (ListView_FullRowSelect(plv)) {
                    // if we're in full row select,
                    // the state bit for select and focus follows column 0.
                    lsi.state |= pitem->state & (LVIS_SELECTED | LVIS_FOCUSED | LVIS_DROPHILITED);
                }

                plvi->state = lsi.state & plvi->stateMask;


                if (plv->stateCallbackMask)
                {
                    nm.item.stateMask = (plvi->stateMask & plv->stateCallbackMask);
                    if (nm.item.stateMask)
                    {
                        nm.item.mask |= LVIF_STATE;
                        nm.item.state = 0;
                    }
                }
            }

        } else {

            if (mask & LVIF_TEXT)
            {
                if (pitem->pszText != LPSTR_TEXTCALLBACK)
                {
                    Str_GetPtr0(pitem->pszText, plvi->pszText, plvi->cchTextMax);
                } else {
                    // if this is LVIF_NORECOMPUTE we will update pszText later
                    nm.item.mask |= LVIF_TEXT;
                }
            }

            if (mask & LVIF_IMAGE)
            {
                plvi->iImage = pitem->iImage;
                if (pitem->iImage == I_IMAGECALLBACK)
                    nm.item.mask |= LVIF_IMAGE;
            }

            if (mask & LVIF_INDENT)
            {
                plvi->iIndent = pitem->iIndent;
                if (pitem->iIndent == I_INDENTCALLBACK)
                    nm.item.mask |= LVIF_INDENT;
            }

            if (mask & LVIF_STATE)
            {
                plvi->state = (pitem->state & plvi->stateMask);

                if (plv->stateCallbackMask)
                {
                    nm.item.stateMask = (plvi->stateMask & plv->stateCallbackMask);
                    if (nm.item.stateMask)
                    {
                        nm.item.mask |= LVIF_STATE;
                        nm.item.state = 0;
                    }
                }
            }
        }

        if (mask & LVIF_PARAM)
            plvi->lParam = pitem->lParam;
    }
    else
    {
        // Complete call back for info...

        // Handle sub-item cases for report view
        //
        if (plvi->iSubItem != 0)
        {
            // if there are no subitem images, don't query for them
            if (!(plv->exStyle & LVS_EX_SUBITEMIMAGES))
                mask &= ~LVIF_IMAGE;

            // don't allow indent on the non-0th column
            mask &= ~LVIF_INDENT;
        }

        if (mask & LVIF_PARAM)
            plvi->lParam = 0L;      // Dont have any to return now...

        if (mask & LVIF_STATE)
        {
            plvi->state = 0;

            if ((plvi->iSubItem == 0) || ListView_FullRowSelect(plv))
            {
                if (plvi->iItem == plv->iFocus)
                    plvi->state |= LVIS_FOCUSED;

                if (plv->plvrangeSel->lpVtbl->IsSelected(plv->plvrangeSel, plvi->iItem) == S_OK)
                    plvi->state |= LVIS_SELECTED;

                if (plv->plvrangeCut->lpVtbl->IsSelected(plv->plvrangeCut, plvi->iItem) == S_OK)
                    plvi->state |= LVIS_CUT;

                if (plvi->iItem == plv->iDropHilite)
                    plvi->state |= LVIS_DROPHILITED;

                plvi->state &= plvi->stateMask;
            }

            if (plv->stateCallbackMask)
            {
                nm.item.stateMask = (plvi->stateMask & plv->stateCallbackMask);
                if (nm.item.stateMask)
                {
                    nm.item.mask |= LVIF_STATE;
                    nm.item.state = 0;
                }
            }
        }

        nm.item.mask |= (mask & (LVIF_TEXT | LVIF_IMAGE | LVIF_INDENT));
    }

    if (mask & LVIF_NORECOMPUTE)
    {
        if (nm.item.mask & LVIF_TEXT)
            plvi->pszText = LPSTR_TEXTCALLBACK;
    }
    else if (nm.item.mask)
    {
        nm.item.iItem  = plvi->iItem;
        nm.item.iSubItem = plvi->iSubItem;
        if (ListView_IsOwnerData( plv ))
            nm.item.lParam = 0L;
        else
            nm.item.lParam = pitem->lParam;

        // just in case LVIF_IMAGE is set and callback doesn't fill it in
        // ... we'd rather have a -1 than whatever garbage is on the stack
        nm.item.iImage = -1;
        nm.item.iIndent = 0;
        if (nm.item.mask & LVIF_TEXT)
        {
            RIPMSG(plvi->pszText != NULL, "LVM_GET(ITEM|ITEMTEXT) null string pointer");

            if (plvi->pszText) {
                nm.item.pszText = plvi->pszText;
                nm.item.cchTextMax = plvi->cchTextMax;

                // Make sure the buffer is zero terminated...
                if (nm.item.cchTextMax)
                    *nm.item.pszText = 0;
            } else {
                // Don't make caller smash null pointer
                nm.item.mask &= ~LVIF_TEXT;
            }
        }

        CCSendNotify(&plv->ci, LVN_GETDISPINFO, &nm.hdr);

        // use nm.item.mask to give the app a chance to change values
        if (nm.item.mask & LVIF_INDENT)
            plvi->iIndent = nm.item.iIndent;
        if (nm.item.mask & LVIF_STATE)
            plvi->state ^= ((plvi->state ^ nm.item.state) & nm.item.stateMask);
        if (nm.item.mask & LVIF_IMAGE)
            plvi->iImage = nm.item.iImage;
        if (nm.item.mask & LVIF_TEXT)
            if (mask & LVIF_TEXT)
                plvi->pszText = CCReturnDispInfoText(nm.item.pszText, plvi->pszText, plvi->cchTextMax);
            else
                plvi->pszText = nm.item.pszText;

        if (pitem && (nm.item.mask & LVIF_DI_SETITEM))
        {

            // BUGBUG HACKHACK
            //
            // The SendNotify above can set about a terrible series of events
            // whereby asking for DISPINFO causes the shell to look around
            // (call peekmessage) to see if its got a new async icon for the
            // listview.  This lets other messages be delivered, such as an
            // UPDATEIMAGE of Index == -1 (if the user is changing icon sizing
            // at the same time).  This causes a re-enumeration of the desktop
            // and hence this very listview is torn down and rebuilt while
            // we're sitting here for the DISPINFO to finish.  Thus, as a cheap
            // and dirty solution, I check to see if the item I think I have
            // is the same one I had when I made the notify, and if not, I
            // bail.  Don't blame me, I'm just cleaning up the mess.

            if (!EVAL(pitem == ListView_GetItemPtr(plv, plvi->iItem)))
            {
                return FALSE;
            }

            if (nm.item.iSubItem == 0)
            {
                //DebugMsg(TF_LISTVIEW, "SAVING ITEMS!");
                if (nm.item.mask & LVIF_IMAGE)
                    pitem->iImage = (short) nm.item.iImage;

                if (nm.item.mask & LVIF_INDENT)
                    pitem->iIndent = (short) nm.item.iIndent;

                if (nm.item.mask & LVIF_TEXT)
                    if (nm.item.pszText) {
                        Str_Set(&pitem->pszText, nm.item.pszText);
                    }

                if (nm.item.mask & LVIF_STATE)
                    pitem->state ^= ((pitem->state ^ nm.item.state) & nm.item.stateMask);
            }
            else
            {
                ListView_SetSubItem(plv, &nm.item);
            }
        }
    }

    return TRUE;
}

#ifdef UNICODE
BOOL NEAR ListView_OnSetItemA(LV* plv, LV_ITEMA FAR* plvi) {
    LPWSTR pszW = NULL;
    LPSTR pszC = NULL;
    BOOL fRet;

    // Let ListView_OnSetItem() handle owner-data validation

    //HACK ALERT -- this code assumes that LV_ITEMA is exactly the same
    // as LV_ITEMW except for the pointer to the string.
    ASSERT(sizeof(LV_ITEMA) == sizeof(LV_ITEMW));

    if (!plvi)
        return FALSE;

    if ((plvi->mask & LVIF_TEXT) && (plvi->pszText != NULL)) {
        pszC = plvi->pszText;
        pszW = ProduceWFromA(plv->ci.uiCodePage, pszC);
        if (pszW == NULL)
            return FALSE;
        plvi->pszText = (LPSTR)pszW;
    }

    fRet = ListView_OnSetItem(plv, (const LV_ITEM FAR*) plvi);

    if (pszW != NULL) {
        plvi->pszText = pszC;

        FreeProducedString(pszW);
    }

    return fRet;

}
#endif

BOOL NEAR ListView_OnSetItem(LV* plv, const LV_ITEM FAR* plvi)
{
    LISTITEM FAR* pitem = NULL;
    UINT mask;
    UINT maskChanged;
    UINT rdwFlags=RDW_INVALIDATE;
    int i;
    UINT stateOld, stateNew;
    BOOL fFocused = FALSE;
    BOOL fSelected = FALSE;
    BOOL fStateImageChanged = FALSE;

    if (ListView_IsOwnerData(plv)) {
      RIPMSG(0, "LVM_SETITEM: Invalid for owner-data listview");
      return FALSE;
    }

    if (!plvi)
        return FALSE;

    ASSERT(plvi->iSubItem >= 0);

    if (plv->himl && (plv->clrBk != ImageList_GetBkColor(plv->himl)))
        rdwFlags |= RDW_ERASE;

    mask = plvi->mask;
    if (!mask)
        return TRUE;

    // If we're setting a subitem, handle it elsewhere...
    //
    if (plvi->iSubItem > 0)
        return ListView_SetSubItem(plv, plvi);

    i = plvi->iItem;

    ListView_InvalidateTTLastHit(plv, i);

    pitem = ListView_GetItemPtr(plv, i);
    if (!pitem)
        return FALSE;

    //REVIEW: This is a BOGUS HACK, and should be fixed.
    //This incorrectly calculates the old state (since we may
    // have to send LVN_GETDISPINFO to get it).
    //
    stateOld = stateNew = 0;
    if (mask & LVIF_STATE)
    {
        stateOld = pitem->state & plvi->stateMask;
        stateNew = plvi->state & plvi->stateMask;
    }

    // Prevent multiple selections in a single-select listview.
    if ((plv->ci.style & LVS_SINGLESEL) && (mask & LVIF_STATE) && (stateNew & LVIS_SELECTED)) {
        ListView_DeselectAll(plv, i);

        // Refresh the old state information
        stateOld = pitem->state & plvi->stateMask;
    }

    if (!ListView_SendChange(plv, i, 0, LVN_ITEMCHANGING, stateOld, stateNew, mask, pitem->lParam))
        return FALSE;

    maskChanged = 0;
    if (mask & LVIF_STATE)
    {
        UINT change = (pitem->state ^ plvi->state) & plvi->stateMask;

        if (change)
        {
            pitem->state ^= change;

            maskChanged |= LVIF_STATE;

            // the selection state has changed.. update selected count
            if (change & LVIS_SELECTED)
            {
                fSelected = TRUE;

                if (pitem->state & LVIS_SELECTED) {
                    plv->nSelected++;
                } else {
                    if (plv->nSelected > 0)
                        plv->nSelected--;
                }
            }

            // For some bits we can only invert the label area...
            // fSelectOnlyChange = ((change & ~(LVIS_SELECTED | LVIS_FOCUSED | LVIS_DROPHILITED)) == 0);
            // fEraseItem = ((change & ~(LVIS_SELECTED | LVIS_DROPHILITED)) != 0);

            // try to steal focus from the previous guy.
            if (change & LVIS_FOCUSED)
            {
                BOOL fUnfolded = ListView_IsItemUnfolded(plv, plv->iFocus);
                int iOldFocus = plv->iFocus;
                RECT rcLabel;

                fFocused = TRUE;

                if (plv->iFocus != i) {
                    if ((plv->iFocus == -1) || ListView_OnSetItemState(plv, plv->iFocus, 0, LVIS_FOCUSED)) {
                        ASSERT(pitem->state & LVIS_FOCUSED);
                        plv->iFocus = i;
                        if (plv->iMark == -1)
                            plv->iMark = i;
                    } else {
                        fFocused = FALSE;
                        pitem->state &= ~LVIS_FOCUSED;
                    }
                } else {
                    ASSERT(!(pitem->state & LVIS_FOCUSED));
                    plv->iFocus = -1;
                }

                // If we were previously unfolded and we move the focus we must
                // attempt to refresh the previous focus owner to referect this change.

                if (fUnfolded && !ListView_IsItemUnfolded(plv, iOldFocus) && (plv->iItemDrawing != iOldFocus))
                {
                    ListView_GetUnfoldedRect(plv, iOldFocus, &rcLabel);
                    RedrawWindow(plv->ci.hwnd, &rcLabel, NULL, RDW_INVALIDATE|RDW_ERASE);
                }

                // Kill the tooltip if focus moves, it causes us headaches otherwise!
                ListView_PopBubble(plv);
            }

            if (change & LVIS_CUT ||
                plv->clrTextBk == CLR_NONE)
                rdwFlags |= RDW_ERASE;

            if (change & LVIS_OVERLAYMASK) {
                // Overlay changed, so need to blow away icon region cache
                if (pitem->hrgnIcon) {
                    if (pitem->hrgnIcon != (HANDLE) -1)
                        DeleteObject(pitem->hrgnIcon);
                    pitem->hrgnIcon = NULL;
                }
            }

            fStateImageChanged = (change & LVIS_STATEIMAGEMASK);

        }
    }

    if (mask & LVIF_TEXT)
    {
        // need to do this now because we're changing the text
        // so we need to get the rect of the thing before the text changes
        // but don't redraw the item we are currently painting
        if (plv->iItemDrawing != i)
        {
            ListView_InvalidateItemEx(plv, i, FALSE,
                RDW_INVALIDATE | RDW_ERASE, LVIF_TEXT);
        }

        if (!Str_Set(&pitem->pszText, plvi->pszText))
            return FALSE;

        plv->rcView.left = RECOMPUTE;
        ListView_SetSRecompute(pitem);
        maskChanged |= LVIF_TEXT;
    }

    if (mask & LVIF_INDENT) {
        if (pitem->iIndent != plvi->iIndent)
        {
            pitem->iIndent = (short) plvi->iIndent;
            maskChanged |= LVIF_INDENT;

            if (ListView_IsReportView(plv))
                rdwFlags |= RDW_ERASE;
        }
    }

    if (mask & LVIF_IMAGE)
    {
        if (pitem->iImage != plvi->iImage)
        {
            pitem->iImage = (short) plvi->iImage;
            maskChanged |= LVIF_IMAGE;

            if (pitem->hrgnIcon) {
                if (pitem->hrgnIcon != (HANDLE) -1)
                    DeleteObject(pitem->hrgnIcon);
                pitem->hrgnIcon = NULL;
            }

            // erase if there was a set image
            if (pitem->iImage != I_IMAGECALLBACK)
                rdwFlags |= RDW_ERASE;
        }
    }

    if (mask & LVIF_PARAM)
    {
        if (pitem->lParam != plvi->lParam)
        {
            pitem->lParam = plvi->lParam;
            maskChanged |= LVIF_PARAM;
        }
    }


    if (maskChanged)
    {
        // don't redraw the item we are currently painting
        if (plv->iItemDrawing != i)
            ListView_InvalidateItemEx(plv, i, FALSE, rdwFlags, maskChanged);

        DebugMsg(DM_LVSENDCHANGE, TEXT("LV - SendChange %d %d %d %d"), i, stateOld, stateNew, maskChanged);
        ListView_SendChange(plv, i, 0, LVN_ITEMCHANGED, stateOld, stateNew, maskChanged, pitem->lParam);

        if (maskChanged & LVIF_TEXT)
            MyNotifyWinEvent(EVENT_OBJECT_NAMECHANGE, plv->ci.hwnd, OBJID_CLIENT, i+1);

        if (maskChanged & LVIF_STATE)
        {
            if (fFocused)
                ListView_NotifyFocusEvent(plv);

            if (fSelected)
            {
                if (stateNew & LVIS_SELECTED)
                {
                    MyNotifyWinEvent((plv->nSelected == 1) ? EVENT_OBJECT_SELECTION :
                        EVENT_OBJECT_SELECTIONADD, plv->ci.hwnd, OBJID_CLIENT, i+1);
                }
                else
                {
                    MyNotifyWinEvent(EVENT_OBJECT_SELECTIONREMOVE, plv->ci.hwnd, OBJID_CLIENT, i+1);
                }
            }

            if (fStateImageChanged)
                MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, plv->ci.hwnd, OBJID_CLIENT, i+1);
        }
    }
    return TRUE;
}

UINT NEAR PASCAL ListView_OnGetItemState(LV* plv, int i, UINT mask)
{
    LV_ITEM lvi;

    lvi.mask = LVIF_STATE;
    lvi.stateMask = mask;
    lvi.iItem = i;
    lvi.iSubItem = 0;
    if (!ListView_OnGetItem(plv, &lvi))
        return 0;

    return lvi.state;
}


BOOL NEAR PASCAL ListView_OnSetItemState(LV* plv, int i, UINT data, UINT mask)
{
    UINT rdwFlags = RDW_INVALIDATE;
    LV_ITEM lvi;

    lvi.mask    = LVIF_STATE;
    lvi.state   = data;
    lvi.stateMask = mask;
    lvi.iItem   = i;
    lvi.iSubItem = 0;

    // if the item is -1, we will do it for all items.  We special case
    // a few cases here as to speed it up.  For example if the mask is
    // LVIS_SELECTED and data is zero it implies that we will deselect
    // all items...
    //
    if (ListView_IsOwnerData( plv ))
    {
        UINT uOldData = 0;

        // these are the only two we handled
        mask &= (LVIS_SELECTED | LVIS_FOCUSED | LVIS_CUT | LVIS_DROPHILITED);
        if (!mask)
            return TRUE;

        if (plv->clrTextBk == CLR_NONE
            || (plv->himl && (plv->clrBk != ImageList_GetBkColor(plv->himl))))
        {
            rdwFlags |= RDW_ERASE;
        }

        if (i == -1)
        {

            // request selection state change for all
            if (mask & LVIS_SELECTED)
            {
                if (data & LVIS_SELECTED)
                {  // set selection
                    if ((plv->ci.style & LVS_SINGLESEL))
                    {   // cant make multiple selections in a single-select listview.
                        return( FALSE );
                    }

                    if (plv->cTotalItems)
                    {
                        if (FAILED(plv->plvrangeSel->lpVtbl->IncludeRange(plv->plvrangeSel, 0, plv->cTotalItems - 1)))
                            return( FALSE );
                    }

                    RedrawWindow( plv->ci.hwnd, NULL, NULL, rdwFlags );

                }
                else
                {  // clear selection
                    if (plv->nSelected > 0) {

                        ListView_InvalidateSelectedOrCutOwnerData(plv, plv->plvrangeSel);
                        if (FAILED(plv->plvrangeSel->lpVtbl->Clear( plv->plvrangeSel )))
                            return FALSE;
                    } else {
                        // if nothing was selected, then there's nothing to clear
                        // no change.
                        mask &= ~ LVIS_SELECTED;
                    }
                }
                uOldData |= (LVIS_SELECTED & (mask ^ data));

                // Update our internal count to what the list thinks is the number selected...
                plv->plvrangeSel->lpVtbl->CountIncluded(plv->plvrangeSel, &plv->nSelected);

            }

            // can maybe combine with above code...
            if (mask & LVIS_CUT)
            {
                if (data & LVIS_CUT)
                {  // set selection

                    if (plv->cTotalItems)
                        if (FAILED(plv->plvrangeCut->lpVtbl->IncludeRange(plv->plvrangeCut, 0, plv->cTotalItems - 1)))
                            return( FALSE );

                    RedrawWindow( plv->ci.hwnd, NULL, NULL, rdwFlags );

                }
                else
                {  // clear selection
                    if (plv->plvrangeCut->lpVtbl->IsEmpty(plv->plvrangeCut) != S_OK) {
                        ListView_InvalidateSelectedOrCutOwnerData(plv, plv->plvrangeCut);
                        if (FAILED(plv->plvrangeCut->lpVtbl->Clear( plv->plvrangeCut)))
                            return( FALSE );

                    } else {
                        // if nothing was selected, then there's nothing to clear
                        // no change.
                        mask &= ~ LVIS_CUT;
                    }
                }
                uOldData |= (LVIS_CUT & (mask ^ data));

            }

            // request focus state change
            if (mask & LVIS_FOCUSED)
            {
                if (data & LVIS_FOCUSED)
                {  // cant set focus to all
                    return( FALSE );
                }
                else if (plv->iFocus != -1)
                {
                    int iOldFocus = plv->iFocus;
                    // clear focus
                    uOldData |= (LVIS_FOCUSED & (mask ^ data));
                    plv->iFocus = -1;
                    // notify that the old focus is being lost
                    DebugMsg(DM_LVSENDCHANGE, TEXT("VLV: LVN_ITEMCHANGED: %d %d %d"), iOldFocus, LVIS_FOCUSED, 0);
                    ListView_SendChange(plv, iOldFocus, 0, LVN_ITEMCHANGED, LVIS_FOCUSED, 0, LVIF_STATE, 0);
                    ListView_InvalidateFoldedItem(plv, iOldFocus, TRUE, RDW_INVALIDATE |RDW_ERASE);
                }
            }

            if (mask & LVIS_DROPHILITED)
            {
                if (data & LVIS_DROPHILITED)
                {  // cant set focus to all
                    return( FALSE );
                }
                else if (plv->iDropHilite != -1)
                {
                    int iOldDropHilite = plv->iDropHilite;
                    // clear focus
                    uOldData |= (LVIS_FOCUSED & (mask ^ data));
                    plv->iDropHilite = -1;
                    // notify that the old focus is being lost
                    ListView_SendChange(plv, iOldDropHilite, 0, LVN_ITEMCHANGED, LVIS_DROPHILITED, 0, LVIF_STATE, 0);
                    ListView_InvalidateFoldedItem(plv, iOldDropHilite, TRUE, RDW_INVALIDATE |RDW_ERASE);
                }
            }

            // invalidate and notify if there was a change
            if (uOldData ^ (data & mask)) {
                DebugMsg(DM_LVSENDCHANGE, TEXT("VLV: LVN_ITEMCHANGED: %d %d %d"), i, uOldData, data);
                ListView_SendChange(plv, i, 0, LVN_ITEMCHANGED, uOldData, data, LVIF_STATE, 0);

                if (mask & LVIS_SELECTED)
                {
                    // Tell accessibility, "Selection changed in a complex way"
                    // (There is no "select all" or "select none" notification)
                    MyNotifyWinEvent(EVENT_OBJECT_SELECTIONWITHIN, plv->ci.hwnd, OBJID_CLIENT, CHILDID_SELF);
                }

            }
        }
        else
        {
            if (!ListView_IsValidItemNumber(plv, i))
                return (FALSE);

            // request selection state change
            // and the selection state is new...
            if ((mask & LVIS_SELECTED)) {


                if (((plv->plvrangeSel->lpVtbl->IsSelected(plv->plvrangeSel, i) == S_OK) ? LVIS_SELECTED : 0) ^ (data & LVIS_SELECTED))
                {
                    if (data & LVIS_SELECTED)
                    {  // set selection
                        if ((plv->ci.style & LVS_SINGLESEL))
                        {
                            // in single selection mode, we need to deselect everything else
                            if (!ListView_OnSetItemState(plv, -1, 0, LVIS_SELECTED))
                                return FALSE;
                        }

                        // now select the new item
                        if (FAILED(plv->plvrangeSel->lpVtbl->IncludeRange(plv->plvrangeSel, i, i)))
                            return FALSE;

                    }
                    else
                    {  // clear selection
                        if (FAILED(plv->plvrangeSel->lpVtbl->ExcludeRange(plv->plvrangeSel, i, i )))
                            return( FALSE );
                    }

                    // something actually changed (or else we wouldn't be in this
                    // if block
                    uOldData |= (LVIS_SELECTED & (mask ^ data));

                } else {

                    // nothing changed... so make the uOldData be the same for this bit
                    // else make this the same as
                    uOldData |= (LVIS_SELECTED & (mask & data));
                }

                // Update our internal count to what the list thinks is the number selected...
                plv->plvrangeSel->lpVtbl->CountIncluded(plv->plvrangeSel, &plv->nSelected);
            }

            if ((mask & LVIS_CUT)) {

                if (((plv->plvrangeCut->lpVtbl->IsSelected(plv->plvrangeCut, i) == S_OK) ? LVIS_CUT : 0) ^ (data & LVIS_CUT))
                {
                    if (data & LVIS_CUT)
                    {
                        // now select the new item
                        if (FAILED(plv->plvrangeCut->lpVtbl->IncludeRange(plv->plvrangeCut, i, i )))
                            return FALSE;
                    }
                    else
                    {  // clear selection
                        if (FAILED(plv->plvrangeCut->lpVtbl->ExcludeRange(plv->plvrangeCut, i, i )))
                            return( FALSE );
                    }

                    // something actually changed (or else we wouldn't be in this
                    // if block
                    uOldData |= (LVIS_CUT & (mask ^ data));
                    rdwFlags |= RDW_ERASE;

                } else {

                    // nothing changed... so make the uOldData be the same for this bit
                    // else make this the same as
                    uOldData |= (LVIS_CUT & (mask & data));
                }
            }

            // request focus state change
            if (mask & LVIS_FOCUSED)
            {
                int iOldFocus = plv->iFocus;

                if (data & LVIS_FOCUSED)
                {  // set focus
                    if (i != plv->iFocus)
                    {
                        // we didn't have the focus before
                        plv->iFocus = i;
                        if (plv->iMark == -1)
                            plv->iMark = i;
                        if (iOldFocus != -1) {

                            // we're stealing it from someone
                            // notify of the change
                            DebugMsg(DM_LVSENDCHANGE, TEXT("VLV: LVN_ITEMCHANGED: %d %d %d"), iOldFocus, LVIS_FOCUSED, 0);
                            ListView_SendChange(plv, iOldFocus, 0, LVN_ITEMCHANGED, LVIS_FOCUSED, 0, LVIF_STATE, 0);

                        }
                    } else {
                        // we DID have the focus before
                        uOldData |= LVIS_FOCUSED;
                    }
                }
                else
                {  // clear focus
                    if (i == plv->iFocus)
                    {
                        plv->iFocus = -1;
                        uOldData |= LVIS_FOCUSED;
                    }
                }

            }

            // request focus state change
            if (mask & LVIS_DROPHILITED)
            {
                int iOldDropHilite = plv->iDropHilite;

                if (data & LVIS_DROPHILITED)
                {  // set Drop Hilite
                    if (i != plv->iDropHilite)
                    {
                        // we didn't have the Drop Hilite before
                        plv->iDropHilite = i;
                        if (iOldDropHilite != -1) {

                            // we're stealing it from someone
                            // notify of the change
                            ListView_SendChange(plv, iOldDropHilite, 0, LVN_ITEMCHANGED, LVIS_DROPHILITED, 0, LVIF_STATE, 0);
                            ListView_InvalidateFoldedItem(plv, iOldDropHilite, TRUE, RDW_INVALIDATE |RDW_ERASE);

                        }
                    } else {
                        // we DID have the Drop Hilite before
                        uOldData |= LVIS_DROPHILITED;
                    }
                }
                else
                {  // clear Drop Hilite
                    if (i == plv->iDropHilite)
                    {
                        plv->iDropHilite = -1;
                        uOldData |= LVIS_DROPHILITED;
                    }
                }

            }

            // invalidate and notify if there was a change
            if (uOldData ^ (data & mask)) {
                DebugMsg(DM_LVSENDCHANGE, TEXT("VLV: LVN_ITEMCHANGED: %d %d %d"), i, uOldData, data);
                ListView_SendChange(plv, i, 0, LVN_ITEMCHANGED, uOldData, data, LVIF_STATE, 0);
                ListView_InvalidateItem(plv, i, TRUE, rdwFlags);

                // Kill the tooltip if focus moves, it causes us headaches otherwise!
                if ((uOldData ^ (data & mask)) & LVIS_FOCUSED)
                {
                    ListView_PopBubble(plv);
                    ListView_NotifyFocusEvent(plv);
                }

                // Tell accessibility about the changes
                if (mask & LVIS_SELECTED) {
                    UINT event;

                    if (data & LVIS_SELECTED) {
                        if (plv->nSelected == 1)
                            event = EVENT_OBJECT_SELECTION; // this object is the entire selection
                        else
                            event = EVENT_OBJECT_SELECTIONADD; // this object is selected
                    } else
                        event = EVENT_OBJECT_SELECTIONREMOVE; // this object is unselected
                    MyNotifyWinEvent(event, plv->ci.hwnd, OBJID_CLIENT, i + 1);
                }
            }
        }

    } else {

        if (i != -1) {
            return ListView_OnSetItem(plv, &lvi);
        } else {
            UINT flags = LVNI_ALL;


            if (data == 0)
            {
                switch (mask)
                {
                case LVIS_SELECTED:
                    flags = LVNI_SELECTED;
                    break;
                case LVIS_CUT:
                    flags = LVNI_CUT;
                    break;
                }
            }
            else if ((plv->ci.style & LVS_SINGLESEL) && (mask == LVIS_SELECTED))
                return FALSE;   /* can't select all in single-select listview */
            else if ((mask & data) & LVIS_FOCUSED) {
                return FALSE; // can't set focus to everything
            }

            //
            // Now iterate over all of the items that match our criteria and
            // set their new value.
            //
            while ((lvi.iItem = ListView_OnGetNextItem(plv, lvi.iItem,
                                                       flags)) != -1) {
                ListView_OnSetItem(plv, &lvi);
            }
        }
    }
    return(TRUE);
}

//
// Returns TRUE if the label of an item is not truncated (is unfolded) and FALSE
// otherwise. If FALSE is returned, it also fills the Unfolding text in pszText.
// If TRUE is returned, pszText is set to empty string.
//
BOOL ListView_IsItemUnfolded2(LV* plv, int iItem, int iSubItem, LPTSTR pszText, int cchTextMax)
{
    BOOL bItemUnfolded = ListView_IsItemUnfolded(plv, iItem);

    if (pszText && cchTextMax > 0)    // Sanity checks on input params.
    {
        pszText[0] = 0;


        if (!bItemUnfolded)
        {
            RECT rcLabel;
            LV_ITEM item;

            item.iItem = iItem;
            item.iSubItem = iSubItem;
            item.mask = LVIF_TEXT | LVIF_PARAM;
            if (!ListView_IsIconView(plv))
            {
                if (ListView_IsLabelTip(plv) || ListView_IsInfoTip(plv))
                {
                    BOOL fSuccess;

                    rcLabel.left = LVIR_LABEL;

                    if (iSubItem) {
                        rcLabel.top = iSubItem;
                        fSuccess = ListView_OnGetSubItemRect(plv, iItem, &rcLabel);
                    } else {
                        fSuccess = ListView_OnGetItemRect(plv, iItem, &rcLabel);
                    }

                    if (fSuccess)
                    {
                        TCHAR szText[INFOTIPSIZE];

                        item.pszText = szText;
                        item.cchTextMax = min(ARRAYSIZE(szText), cchTextMax);
                        if (ListView_OnGetItem(plv, &item) && item.pszText != LPSTR_TEXTCALLBACK)
                        {
                            SIZE siz;
                            LVFAKEDRAW lvfd;
                            int cx;

                            ListView_BeginFakeCustomDraw(plv, &lvfd, &item);
                            ListView_BeginFakeItemDraw(&lvfd);

                            //        ---------Label width----------- ---Client width---
                             cx = min(rcLabel.right - g_cxLabelMargin, plv->sizeClient.cx);

                            if (GetTextExtentPoint32(lvfd.nmcd.nmcd.hdc, item.pszText, lstrlen(item.pszText), &siz) &&
                                (rcLabel.left + g_cxLabelMargin + siz.cx) > cx)
                            {
                                lstrcpyn(pszText, item.pszText, item.cchTextMax);
                            }
                            else
                            {
                                // Not truncated after all
                                bItemUnfolded = TRUE;
                            }

                            ListView_EndFakeItemDraw(&lvfd);
                            ListView_EndFakeCustomDraw(&lvfd);
                        }
                    }
                }
            }
            else
            {
                // Large icon view is the only one that folds
                if (ListView_GetUnfoldedRect(plv, iItem, &rcLabel))
                {
                    item.pszText = pszText;
                    item.cchTextMax = cchTextMax;
                    ListView_OnGetItem(plv, &item);
                }
                else
                {
                    // Item was never folded
                    bItemUnfolded = TRUE;
                }
            }
        }
    }
    return bItemUnfolded;
}

#ifdef UNICODE

// Rather than thunking to ListView_OnGetItemText, we let ListView_GetItemA
// do the work.

int NEAR PASCAL ListView_OnGetItemTextA(LV* plv, int i, LV_ITEMA FAR *plvi)
{
    if (!plvi)
        return 0;

    RIPMSG(plvi->pszText != NULL, "LVM_GETITEMTEXT null string pointer");

    plvi->mask = LVIF_TEXT;
    plvi->iItem = i;
    if (!ListView_OnGetItemA(plv, plvi))
        return 0;

    return lstrlenA(plvi->pszText);
}
#endif

int NEAR PASCAL ListView_OnGetItemText(LV* plv, int i, LV_ITEM FAR *plvi)
{
    if (!plvi)
        return 0;

    RIPMSG(plvi->pszText != NULL, "LVM_GETITEMTEXT null string pointer");

    plvi->mask = LVIF_TEXT;
    plvi->iItem = i;
    if (!ListView_OnGetItem(plv, plvi))
        return 0;

    return lstrlen(plvi->pszText);
}


#ifdef UNICODE
BOOL WINAPI ListView_OnSetItemTextA(LV* plv, int i, int iSubItem, LPCSTR pszText) {
    LPWSTR pszW = NULL;
    BOOL fRet;

    // Let ListView_OnSetItemText() handle owner-data validation

    if (pszText != NULL) {
        pszW = ProduceWFromA(plv->ci.uiCodePage, pszText);
        if (pszW == NULL) {
            return FALSE;
        }
    }

    fRet = ListView_OnSetItemText(plv, i, iSubItem, pszW);

    FreeProducedString(pszW);

    return fRet;
}
#endif

BOOL WINAPI ListView_OnSetItemText(LV* plv, int i, int iSubItem, LPCTSTR pszText)
{
    LV_ITEM lvi;

    if (ListView_IsOwnerData(plv))
    {
       RIPMSG(0, "LVM_SETITEMTEXT: Invalid for owner-data listview");
       return FALSE;
    }

    ListView_InvalidateTTLastHit(plv, i);

    lvi.mask = LVIF_TEXT;
    lvi.pszText = (LPTSTR)pszText;
    lvi.iItem = i;
    lvi.iSubItem = iSubItem;

    return ListView_OnSetItem(plv, &lvi);
}

VOID CALLBACK ImgCtxCallback(void * pvImgCtx, void * pvArg)
{
    LV *plv = (LV *)pvArg;
    ULONG ulState;
    SIZE sizeImg;
    IImgCtx *pImgCtx = plv->pImgCtx;

    IImgCtx_GetStateInfo(pImgCtx, &ulState, &sizeImg, TRUE);

    if (ulState & (IMGLOAD_STOPPED | IMGLOAD_ERROR))
    {
        TraceMsg(TF_BKIMAGE, "Error!");
        plv->fImgCtxComplete = FALSE;
    }

    else if (ulState & IMGCHG_COMPLETE)
    {
        TraceMsg(TF_BKIMAGE, "Complete!");
        plv->fImgCtxComplete = TRUE;
        InvalidateRect(plv->ci.hwnd, NULL, TRUE);
    }
}

void ListView_ReleaseBkImage(LV *plv)
{
    if (plv->pImgCtx)
    {
        IImgCtx_Release(plv->pImgCtx);
        plv->pImgCtx = NULL;

        if (plv->hpalHalftone)
        {
            // No need to delete the half tone palette since we really
            // share it with the image context and it will clean up.
            plv->hpalHalftone = NULL;
        }
    }

    if (plv->hbmBkImage)
    {
        DeleteObject(plv->hbmBkImage);
        plv->hbmBkImage = NULL;
    }

    if (plv->pszBkImage)
    {
        LocalFree(plv->pszBkImage);
        plv->pszBkImage = NULL;
    }
}

BOOL WINAPI ListView_OnSetBkImage(LV* plv, LPLVBKIMAGE pbi)
{
    LPCTSTR pszImage = pbi->pszImage;
    BOOL fRet = FALSE;
    LONG fl;

    switch (pbi->ulFlags & LVBKIF_SOURCE_MASK)
    {
    case LVBKIF_SOURCE_NONE:
        TraceMsg(TF_BKIMAGE, "LV SetBkImage to none");
        ListView_ReleaseBkImage(plv);
        break;

    case LVBKIF_SOURCE_HBITMAP:
        TraceMsg(TF_BKIMAGE, "LV SetBkImage to hBitmap %08lX", pbi->hbm);
        ListView_ReleaseBkImage(plv);
        if (pbi->hbm)
        {
            plv->hbmBkImage = pbi->hbm;
            ASSERT(0); // KenSy hasn't implemented init from bitmap yet...
        }
        else
        {
            pbi->ulFlags &= ~LVBKIF_SOURCE_HBITMAP;
        }
        break;

    case LVBKIF_SOURCE_URL:
        TraceMsg(TF_BKIMAGE, "LV SetBkImage to URL");
        ListView_ReleaseBkImage(plv);
        if (pszImage && pszImage[0])
        {
            HRESULT (*pfnCoCreateInstance)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID *);
            HRESULT hr;
            HMODULE hmodOLE;

            plv->pszBkImage = LocalAlloc(LPTR, (lstrlen(pszImage) + 1) * sizeof(TCHAR));
            if (plv->pszBkImage == NULL)
            {
                TraceMsg(TF_BKIMAGE, "Wow, could not allocate memory for string!");
                return FALSE;
            }
            lstrcpy(plv->pszBkImage, pszImage);

            if (((hmodOLE = GetModuleHandle(TEXT("OLE32"))) == NULL) ||
                ((pfnCoCreateInstance = (HRESULT (*)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID *))GetProcAddress(hmodOLE, "CoCreateInstance")) == NULL))
            {
                TraceMsg(TF_BKIMAGE, "Could not find CoCreateInstance!");
                TraceMsg(TF_BKIMAGE, "Did the caller remember to call CoInitialize?");
                return FALSE;
            }

            hr = pfnCoCreateInstance(&CLSID_IImgCtx, NULL, CLSCTX_INPROC_SERVER,
                                     &IID_IImgCtx, (LPVOID *)&plv->pImgCtx);

            if (FAILED(hr))
            {
                TraceMsg(TF_BKIMAGE, "Could not create a pImgCtx!");
                TraceMsg(TF_BKIMAGE, "Did you remember to register IEIMAGE.DLL?");
                return FALSE;
            }
            //
            // Mirror the downloaded image if the listview window is RTL mirrored,
            // so that it would be displayed as is. [samera]
            //
            fl = ((IS_WINDOW_RTL_MIRRORED(plv->ci.hwnd)) ? DWN_MIRRORIMAGE : 0);

#ifdef UNICODE
            hr = IImgCtx_Load(plv->pImgCtx, pszImage, fl);
#else
            {
                //
                // Darn IImgCtx::Load only takes wide URLs, sigh
                //
                LPWSTR pwszImage;

                pwszImage = ProduceWFromA(plv->ci.uiCodePage, pszImage);
                if (!pwszImage)
                {
                    IImgCtx_Release(plv->pImgCtx);
                    plv->pImgCtx = NULL;
                    TraceMsg(TF_BKIMAGE, "Could not convert URL to wide!");
                    return FALSE;
                }
                hr = IImgCtx_Load(plv->pImgCtx, pwszImage, fl);
                FreeProducedString(pwszImage);
            }
#endif
            if (FAILED(hr))
            {
                IImgCtx_Release(plv->pImgCtx);
                plv->pImgCtx = NULL;
                TraceMsg(TF_BKIMAGE, "Could not init a pImgCtx!");
                return FALSE;
            }
        }
        else
        {
            pbi->ulFlags &= ~LVBKIF_SOURCE_URL;
        }
        break;

    default:
        RIPMSG(0, "LVM_SETBKIMAGE: Unsupported image type %d", pbi->ulFlags & LVBKIF_SOURCE_MASK);
        return FALSE;
    }

    plv->ulBkImageFlags = pbi->ulFlags;
    plv->xOffsetPercent = pbi->xOffsetPercent;
    plv->yOffsetPercent = pbi->yOffsetPercent;

    //
    // If we actually created a pImgCtx, initialize it here.
    //
    if (plv->pImgCtx)
    {
        if (plv->hpalHalftone == NULL)
        {
            IImgCtx_GetPalette(plv->pImgCtx, &plv->hpalHalftone);
        }

        plv->fImgCtxComplete = FALSE;
        IImgCtx_SetCallback(plv->pImgCtx, ImgCtxCallback, plv);
        IImgCtx_SelectChanges(plv->pImgCtx, IMGCHG_COMPLETE, 0, TRUE);

        TraceMsg(TF_BKIMAGE, "  SUCCESS!");
        fRet = TRUE;
    }
    InvalidateRect(plv->ci.hwnd, NULL, TRUE);

    return fRet;
}

#ifdef UNICODE
BOOL WINAPI ListView_OnSetBkImageA(LV* plv, LPLVBKIMAGEA pbiA)
{
    BOOL fProducedString = FALSE;
    BOOL fRet;
    LVBKIMAGEW biW;

    CopyMemory(&biW, pbiA, SIZEOF(LVBKIMAGE));

    switch (biW.ulFlags & LVBKIF_SOURCE_MASK)
    {
    case LVBKIF_SOURCE_NONE:
    case LVBKIF_SOURCE_HBITMAP:
        break;

    case LVBKIF_SOURCE_URL:
        if (biW.pszImage != NULL)
        {
            biW.pszImage = ProduceWFromA(plv->ci.uiCodePage, (LPCSTR)biW.pszImage);
            if (biW.pszImage == (LPARAM)NULL)
            {
                return FALSE;
            }
            fProducedString = TRUE;
        }
        break;

    default:
        // Let ListView_OnSetBkImage() complain about the invalid parameter
        break;
    }

    fRet = ListView_OnSetBkImage(plv, &biW);

    if (fProducedString)
    {
        FreeProducedString((LPVOID)biW.pszImage);
    }

    return fRet;
}
#endif

BOOL WINAPI ListView_OnGetBkImage(LV* plv, LPLVBKIMAGE pbi)
{
    BOOL fRet = FALSE;

    if (!IsBadWritePtr(pbi, sizeof(*pbi)))
    {
        pbi->ulFlags = plv->ulBkImageFlags;

        switch (plv->ulBkImageFlags & LVBKIF_SOURCE_MASK)
        {
        case LVBKIF_SOURCE_NONE:
            fRet = TRUE;
            break;

        case LVBKIF_SOURCE_HBITMAP:
            pbi->hbm = plv->hbmBkImage;
            fRet = TRUE;
            break;

        case LVBKIF_SOURCE_URL:
            if (!IsBadWritePtr(pbi->pszImage, pbi->cchImageMax * SIZEOF(TCHAR)))
            {
                lstrcpyn(pbi->pszImage, plv->pszBkImage, pbi->cchImageMax);
                fRet = TRUE;
            }
            break;

        default:
            ASSERT(0);
            break;
        }

        pbi->xOffsetPercent = plv->xOffsetPercent;
        pbi->yOffsetPercent = plv->yOffsetPercent;
    }

    return fRet;
}

#ifdef UNICODE
BOOL WINAPI ListView_OnGetBkImageA(LV* plv, LPLVBKIMAGEA pbiA)
{
    BOOL fRet = FALSE;

    if (!IsBadWritePtr(pbiA, sizeof(*pbiA)))
    {
        pbiA->ulFlags = plv->ulBkImageFlags;

        switch (plv->ulBkImageFlags & LVBKIF_SOURCE_MASK)
        {
        case LVBKIF_SOURCE_NONE:
            fRet = TRUE;
            break;

        case LVBKIF_SOURCE_HBITMAP:
            pbiA->hbm = plv->hbmBkImage;
            fRet = TRUE;
            break;

        case LVBKIF_SOURCE_URL:
            if (!IsBadWritePtr(pbiA->pszImage, pbiA->cchImageMax))
            {
                ConvertWToAN(plv->ci.uiCodePage, pbiA->pszImage,
                             pbiA->cchImageMax, plv->pszBkImage, -1);
                fRet = TRUE;
            }
            break;

        default:
            ASSERT(0);
            break;
        }

        pbiA->xOffsetPercent = plv->xOffsetPercent;
        pbiA->yOffsetPercent = plv->yOffsetPercent;
    }

    return fRet;
}
#endif

void ListView_FreeSubItem(PLISTSUBITEM plsi)
{
    if (plsi) {
        Str_Set(&plsi->pszText, NULL);
        LocalFree(plsi);
    }
}

int NEAR ListView_GetCxScrollbar(LV* plv)
{
    int cx;

    if (((plv->exStyle & LVS_EX_FLATSB) == 0) ||
        !FlatSB_GetScrollProp(plv->ci.hwnd, WSB_PROP_CXVSCROLL, &cx))
    {
        cx = g_cxScrollbar;
    }

    return cx;
}

int NEAR ListView_GetCyScrollbar(LV* plv)
{
    int cy;

    if (((plv->exStyle & LVS_EX_FLATSB) == 0) ||
        !FlatSB_GetScrollProp(plv->ci.hwnd, WSB_PROP_CYHSCROLL, &cy))
    {
        cy = g_cyScrollbar;
    }

    return cy;
}

DWORD NEAR ListView_GetWindowStyle(LV* plv)
{
    DWORD dwStyle;

    if (((plv->exStyle & LVS_EX_FLATSB) == 0) ||
        !FlatSB_GetScrollProp(plv->ci.hwnd, WSB_PROP_WINSTYLE, (LPINT)&dwStyle))
    {
        dwStyle = GetWindowStyle(plv->ci.hwnd);
    }

    return dwStyle;
}

int ListView_SetScrollInfo(LV *plv, int fnBar, LPSCROLLINFO lpsi, BOOL fRedraw)
{
    int iRc;

    if (plv->exStyle & LVS_EX_FLATSB)
    {
        iRc = FlatSB_SetScrollInfo(plv->ci.hwnd, fnBar, lpsi, fRedraw);
    }
    else
    {
        iRc = SetScrollInfo(plv->ci.hwnd, fnBar, lpsi, fRedraw);
    }

    //
    //  You'd think we were finished, but in fact the game is only half over.
    //
    //  Some apps (e.g., Font Folder) will do
    //
    //      SetWindowLong(hwnd, GWL_STYLE, newStyle);
    //
    //  where newStyle toggles the WS_HSCROLL and/or WS_VSCROLL bits.
    //  This causes USER's internal bookkeeping to go completely out
    //  of whack:  The ScrollInfo says that there is a scrollbar, but
    //  the window style says there isn't, or vice versa.  The result
    //  is that we get a scrollbar when we shouldn't or vice versa.
    //
    //  So each time we tweak the scroll info in a manner that changes
    //  the range and page, we kick USER in the head to make sure USER's
    //  view of the world (via style bits) is the same as the scroll
    //  bar's view of the world (via SCROLLINFO).
    //

    //
    //  We should always change SIF_PAGE and SIF_RANGE at the same time.
    //
    ASSERT((lpsi->fMask & (SIF_PAGE | SIF_RANGE)) == 0 ||
           (lpsi->fMask & (SIF_PAGE | SIF_RANGE)) == (SIF_PAGE | SIF_RANGE));

    if ((lpsi->fMask & (SIF_PAGE | SIF_RANGE)) == (SIF_PAGE | SIF_RANGE))
    {
        BOOL fShow;
        fShow = lpsi->nMax && (int)lpsi->nPage <= lpsi->nMax;

#ifdef DEBUG
        {
            DWORD dwStyle, dwScroll, dwWant;
            dwScroll = (fnBar == SB_VERT) ? WS_VSCROLL : WS_HSCROLL;
            //
            //  We can short-circuit some logic with secret knowledge about how
            //  ListView uses SetScrollInfo.
            //
            ASSERT(lpsi->nMin == 0);

            dwWant = fShow ? dwScroll : 0;
            dwStyle = ListView_GetWindowStyle(plv);
            if ((dwStyle & dwScroll) != dwWant)
            {
                TraceMsg(TF_LISTVIEW, "ListView_SetScrollInfo: App twiddled WS_[VH]SCROLL");
            }
        }
#endif

        if (plv->exStyle & LVS_EX_FLATSB)
            FlatSB_ShowScrollBar(plv->ci.hwnd, fnBar, fShow);
        else
            ShowScrollBar(plv->ci.hwnd, fnBar, fShow);
    }

    return iRc;
}

// Add/remove/replace item

BOOL NEAR ListView_FreeItem(LV* plv, LISTITEM FAR* pitem)
{
    ASSERT( !ListView_IsOwnerData(plv));

    if (pitem)
    {
        Str_Set(&pitem->pszText, NULL);
        if (pitem->hrgnIcon && pitem->hrgnIcon!=(HANDLE)-1)
            DeleteObject(pitem->hrgnIcon);
        // NOTE: We never remove items from the image list; that's
        // the app's responsibility.
        // REVIEW: Should we do this?  Or should we just provide
        // a message that will adjust image indices for the guy
        // when one is removed?
        //
        ControlFree(plv->hheap, pitem);
    }
    return FALSE;
}

LISTITEM FAR* NEAR ListView_CreateItem(LV* plv, const LV_ITEM FAR* plvi)
{
    LISTITEM FAR* pitem = ControlAlloc(plv->hheap, sizeof(LISTITEM));

    ASSERT(!ListView_IsOwnerData(plv));

    if (pitem)
    {
        if (plvi->mask & LVIF_STATE) {
            if (plvi->state & ~LVIS_ALL)  {
                DebugMsg(DM_ERROR, TEXT("ListView: Invalid state: %04x"), plvi->state);
                return NULL;
            }

            // If adding a selected item to a single-select listview, deselect
            // any other items.
            if ((plv->ci.style & LVS_SINGLESEL) && (plvi->state & LVIS_SELECTED))
                ListView_DeselectAll(plv, -1);

            pitem->state  = (plvi->state & ~(LVIS_FOCUSED | LVIS_SELECTED));
        }
        if (plvi->mask & LVIF_PARAM)
            pitem->lParam = plvi->lParam;

        if (plvi->mask & LVIF_IMAGE)
            pitem->iImage = (short) plvi->iImage;

        if (plvi->mask & LVIF_INDENT)
            pitem->iIndent = (short) plvi->iIndent;

        pitem->pt.x = pitem->pt.y = RECOMPUTE;
        ListView_SetSRecompute(pitem);

        pitem->pszText = NULL;
        if (plvi->mask & LVIF_TEXT) {
            if (!Str_Set(&pitem->pszText, plvi->pszText))
            {
                ListView_FreeItem(plv, pitem);
                return NULL;
            }
        }
    }
    return pitem;
}

// HACK ALERT!! -- fSmoothScroll is an added parameter!  It allows for smooth
// scrolling when deleting items.  ListView_LRInvalidateBelow is only currently
// called from ListView_OnUpdate and ListView_OnDeleteItem.  Both these calls
// have been modified to work correctly and be backwards compatible.
//
void ListView_LRInvalidateBelow(LV* plv, int i, int fSmoothScroll)
{
    if (ListView_IsListView(plv) || ListView_IsReportView(plv)) {
        RECT rcItem;

        if (!ListView_RedrawEnabled(plv) ||
            (ListView_IsReportView(plv) && (plv->pImgCtx != NULL)))
            fSmoothScroll = FALSE;

        if (i >= 0)
        {
            ListView_GetRects(plv, i, NULL, NULL, &rcItem, NULL);
        }
        else
        {
            rcItem.left = rcItem.top = 0;
            rcItem.right = plv->sizeClient.cx;
            rcItem.bottom = plv->sizeClient.cy;
        }

        // Don't try to scroll over the header part
        if (ListView_IsReportView(plv) && rcItem.top < plv->yTop)
            rcItem.top = plv->yTop;

        // For both List and report view need to erase the item and
        // below.  Note: do simple test to see if there is anything
        // to redraw

        // we can't check for bottom/right > 0 because if we nuked something
        // above or to the left of the view, it may affect us all
        if ((rcItem.top <= plv->sizeClient.cy) &&
            (rcItem.left <= plv->sizeClient.cx))
        {
            rcItem.bottom = plv->sizeClient.cy;

            if (ListView_RedrawEnabled(plv)) {
                if ((plv->clrBk == CLR_NONE) && (plv->pImgCtx == NULL))
                {
                    LVSeeThruScroll(plv, &rcItem);
                }
                else if (ListView_IsReportView(plv) && fSmoothScroll)
                {
#ifndef UNIX
                    SMOOTHSCROLLINFO si =
                    {
                        sizeof(si),
                        SSIF_MINSCROLL,
                        plv->ci.hwnd,
                        0,
                        -(plv->cyItem),
                        &rcItem,
                        &rcItem,
                        NULL,
                        NULL,
                        SW_INVALIDATE|SW_ERASE,
                        SSI_DEFAULT,
                        1,
                        1,
                    };
#else
                    SMOOTHSCROLLINFO si;
                    si.cbSize = sizeof(si);
                    si.fMask = SSIF_MINSCROLL;
                    si.hwnd = plv->ci.hwnd;
                    si.dx = 0;
                    si.dy = -(plv->cyItem);
                    si.lprcSrc = &rcItem;
                    si.lprcClip = &rcItem;
                    si.hrgnUpdate = NULL;
                    si.lprcUpdate = NULL;
                    si.fuScroll = SW_INVALIDATE|SW_ERASE;
                    si.uMaxScrollTime = SSI_DEFAULT;
                    si.cxMinScroll = 1;
                    si.cyMinScroll = 1;
                    si.pfnScrollProc = NULL;
#endif

                    SmoothScrollWindow(&si);
                } else {
                    RedrawWindow(plv->ci.hwnd, &rcItem, NULL, RDW_INVALIDATE | RDW_ERASE);
                }
            } else {
                RedrawWindow(plv->ci.hwnd, &rcItem, NULL, RDW_INVALIDATE | RDW_ERASE);
            }

            if (ListView_IsListView(plv))
            {
                RECT rcClient;
                // For Listview we need to erase the other columns...
                rcClient.left = rcItem.right;
                rcClient.top = 0;
                rcClient.bottom = plv->sizeClient.cy;
                rcClient.right = plv->sizeClient.cx;
                RedrawWindow(plv->ci.hwnd, &rcClient, NULL, RDW_INVALIDATE | RDW_ERASE);
            }
        }
    }
}

// Used in Ownerdata Icon views to try to not invalidate the whole world...
void ListView_IInvalidateBelow(LV* plv, int i)
{
    RECT rcItem;

    if (i >= 0)
    {
        ListView_GetRects(plv, i, NULL, NULL, &rcItem, NULL);
    }
    else
    {
        rcItem.left = rcItem.top = 0;
        rcItem.right = plv->sizeClient.cx;
        rcItem.bottom = plv->sizeClient.cy;
    }

    // For Iconviews we need to invalidate everything to the right of us in this
    // row and everything below the row...
    // below.  Note: do simple test to see if there is anything
    // to redraw

    if ((rcItem.top <= plv->sizeClient.cy) &&
        (rcItem.left <= plv->sizeClient.cx))
    {
        rcItem.right = plv->sizeClient.cx;
        RedrawWindow(plv->ci.hwnd, &rcItem, NULL, RDW_INVALIDATE | RDW_ERASE);

        // Now erase everything below...
        rcItem.top = rcItem.bottom;
        rcItem.bottom = plv->sizeClient.cy;
        rcItem.left = 0;
        RedrawWindow(plv->ci.hwnd, &rcItem, NULL, RDW_INVALIDATE | RDW_ERASE);
    }
}


void NEAR ListView_OnUpdate(LV* plv, int i)
{
    // If in icon/small view, don't call InvalidateItem, since that'll force
    // FindFreeSlot to get called, which is pig-like.  Instead, just
    // force a WM_PAINT message, which we'll catch and call Recompute with.
    //
    if (ListView_IsIconView(plv) || ListView_IsSmallView(plv))
    {
        if (plv->ci.style & LVS_AUTOARRANGE)
            ListView_OnArrange(plv, LVA_DEFAULT);
        else
            RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_INTERNALPAINT | RDW_NOCHILDREN);
    }
    else
    {
        // HACK ALERT!! -- The third parameter is new.  It allows for
        // smooth scrolling when items are deleted in reportview.
        // Passing 0, tells it NOT to scroll.
        //
        ListView_LRInvalidateBelow(plv, i, 0);
    }
    ListView_UpdateScrollBars(plv);
}

#ifdef UNICODE
int NEAR ListView_OnInsertItemA(LV* plv, LV_ITEMA FAR* plvi) {
    LPWSTR pszW = NULL;
    LPSTR pszC = NULL;
    int iRet;

    //HACK ALERT -- this code assumes that LV_ITEMA is exactly the same
    // as LV_ITEMW except for the pointer to the string.
    ASSERT(sizeof(LV_ITEMA) == sizeof(LV_ITEMW));

    if (!plvi)
    {
        return -1;
    }


    if ((plvi->mask & LVIF_TEXT) && (plvi->pszText != NULL)) {
        pszC = plvi->pszText;
        pszW = ProduceWFromA(plv->ci.uiCodePage, pszC);
        if (pszW == NULL)
            return -1;
        plvi->pszText = (LPSTR)pszW;
    }

    iRet = ListView_OnInsertItem(plv, (const LV_ITEM FAR*) plvi);

    if (pszW != NULL) {
        plvi->pszText = pszC;

        FreeProducedString(pszW);
    }

    return iRet;

}
#endif

int NEAR ListView_OnInsertItem(LV* plv, const LV_ITEM FAR* plvi)
{
    int iItem;

    if (!plvi || (plvi->iSubItem != 0))    // can only insert the 0th item
    {
        RIPMSG(0, "ListView_InsertItem: iSubItem must be 0 (app passed %d)", plvi->iSubItem);
        return -1;
    }

    // If sorted, then insert sorted.
    //
    if (plv->ci.style & (LVS_SORTASCENDING | LVS_SORTDESCENDING)
        && !ListView_IsOwnerData( plv ))
    {
        if (plvi->pszText == LPSTR_TEXTCALLBACK)
        {
            DebugMsg(DM_ERROR, TEXT("Don't use LPSTR_TEXTCALLBACK with LVS_SORTASCENDING or LVS_SORTDESCENDING"));
            return -1;
        }
        iItem = ListView_LookupString(plv, plvi->pszText, LVFI_SUBSTRING | LVFI_NEARESTXY, 0);
    }
    else
        iItem = plvi->iItem;

    ListView_InvalidateTTLastHit(plv, plv->iTTLastHit);

    if ( !ListView_IsOwnerData(plv))
    {
        int iZ;
        LISTITEM FAR *pitem = ListView_CreateItem(plv, plvi);
        UINT uSelMask = plvi->mask & LVIF_STATE ?
                (plvi->state & (LVIS_FOCUSED | LVIS_SELECTED))
                : 0;
        UINT uSel = uSelMask;

        if (!pitem)
            return -1;

        iItem = DPA_InsertPtr(plv->hdpa, iItem, pitem);
        if (iItem == -1)
        {
            ListView_FreeItem(plv, pitem);
            return -1;
        }

        plv->cTotalItems++;

        if (plv->hdpaSubItems)
        {
            int iCol;
            // slide all the colum DPAs down to match the location of the
            // inserted item
            //
            for (iCol = plv->cCol - 1; iCol >= 0; iCol--)
            {
                HDPA hdpa = ListView_GetSubItemDPA(plv, iCol);
                if (hdpa)       // this is optional, call backs don't have them
                {
                    // insert a blank item (REVIEW: should this be callback?)

                    // since this can be a tail sparce array,
                    // we need to make sure enough items are there.
                    if (iItem >= DPA_GetPtrCount(hdpa))
                        DPA_SetPtr(hdpa, iItem, NULL);
                    else if (DPA_InsertPtr(hdpa, iItem, NULL) != iItem)
                        goto Failure;
                    // Bad assert since hdpa can be tail sparse
                    // ASSERT(ListView_Count(plv) == DPA_GetPtrCount(hdpa));
                    ASSERT(ListView_Count(plv) >= DPA_GetPtrCount(hdpa));
                }
            }
        }

        // Add item to end of z order
        //
        iZ = DPA_InsertPtr(plv->hdpaZOrder, ListView_Count(plv), (LPVOID)iItem);

        if (iZ == -1)
        {
Failure:
            DebugMsg(TF_LISTVIEW, TEXT("ListView_OnInsertItem() failed"));
            if (DPA_DeletePtr(plv->hdpa, iItem))
                plv->cTotalItems--;
            ListView_FreeItem(plv, pitem);
            return -1;
        }

        // if we inserted before the focus point, move the focus point up one
        if (iItem <= plv->iFocus)
            plv->iFocus++;
        // do the same thing for the mark
        if (iItem <= plv->iMark)
            plv->iMark++;

        // If the item was not added at the end of the list we need
        // to update the other indexes in the list
        if (iItem != ListView_Count(plv) - 1)
        {
            int i2;
            for (i2 = iZ - 1; i2 >= 0; i2--)
            {
                int iItemZ = (int)(UINT_PTR)DPA_FastGetPtr(plv->hdpaZOrder, i2);
                if (iItemZ >= iItem)
                    DPA_SetPtr(plv->hdpaZOrder, i2, (LPVOID)(UINT_PTR)(iItemZ + 1));
            }
        }

        if (ListView_CheckBoxes(plv)) {
            uSelMask |= LVIS_STATEIMAGEMASK;
            uSel |= INDEXTOSTATEIMAGEMASK(1);
        }

        if (uSelMask) {

            // we masked off these in the createitem above.
            // because turning these on means more than setting the bits.
            ListView_OnSetItemState(plv, iItem, uSel, uSelMask);
        }
    }
    else
    {
        //
        // simply adjust selection and count
        //
        if ((iItem >= 0) && (iItem <= MAX_LISTVIEWITEMS))
        {
            if (FAILED(plv->plvrangeSel->lpVtbl->InsertItem(plv->plvrangeSel, iItem )))
            {
                return( -1 );
            }
            plv->cTotalItems++;
            plv->rcView.left = RECOMPUTE;
            ListView_Recompute(plv);
            if (!ListView_IsReportView(plv) && !ListView_IsListView(plv))
                {
                // We need to erase the background so that we don't leave
                // turds from wrapped labels in large icon mode.  This could
                // be optimized by only invalidating to the right of and
                // below the inserted item.
                InvalidateRect( plv->ci.hwnd, NULL, TRUE );
                }
            // if we inserted before the focus point, move the focus point up
            if (iItem <= plv->iFocus)
                plv->iFocus++;
            // do the same thing for the mark
            if (iItem <= plv->iMark)
                plv->iMark++;
        }

    }

    if (!ListView_IsOwnerData(plv))
        ASSERT(ListView_Count(plv) == DPA_GetPtrCount(plv->hdpaZOrder));

    if (ListView_RedrawEnabled(plv))
    {
        // Update region
        ListView_RecalcRegion(plv, TRUE, TRUE);

        // The Maybe resize colmns may resize things in which case the next call
        // to Update is not needed.
        if (!ListView_MaybeResizeListColumns(plv, iItem, iItem))
            ListView_OnUpdate(plv, iItem);

        // this trick makes inserting lots of items cheap
        // even if redraw is enabled.... don't calc or position items
        // until this postmessage comes around
        if (!plv->uUnplaced) {
            PostMessage(plv->ci.hwnd, LVMI_PLACEITEMS, 0, 0);
        }
        plv->uUnplaced++;
    }
    else
    {
        //
        // Special case code to make using SetRedraw work reasonably well
        // for adding items to a listview which is in a non layout mode...
        //
        if ((plv->iFirstChangedNoRedraw == -1) ||
                (iItem < plv->iFirstChangedNoRedraw))
            plv->iFirstChangedNoRedraw = iItem;

    }

    ListView_Notify(plv, iItem, 0, LVN_INSERTITEM);

    MyNotifyWinEvent(EVENT_OBJECT_CREATE, plv->ci.hwnd, OBJID_CLIENT, iItem+1);

    return iItem;
}

BOOL NEAR ListView_OnDeleteItem(LV* plv, int iItem)
{
    int iCount = ListView_Count(plv);

    if (!ListView_IsValidItemNumber(plv, iItem))
        return FALSE;   // out of range

    MyNotifyWinEvent(EVENT_OBJECT_DESTROY, plv->ci.hwnd, OBJID_CLIENT, iItem+1);

    ListView_DismissEdit(plv, TRUE);  // cancel edits

    ListView_OnSetItemState(plv, iItem, 0, LVIS_SELECTED);

    if (plv->iFocus == iItem)
        ListView_OnSetItemState(plv, (iItem==iCount-1 ? iItem-1 : iItem+1), LVIS_FOCUSED, LVIS_FOCUSED);

    ListView_InvalidateTTLastHit(plv, plv->iTTLastHit);

    if (!ListView_IsOwnerData(plv))
    {
        LISTITEM FAR* pitem;
        int iZ;

        if ((plv->rcView.left != RECOMPUTE) && (ListView_IsIconView(plv) || ListView_IsSmallView(plv))) {
            pitem = ListView_FastGetItemPtr(plv, iItem);
            if (LV_IsItemOnViewEdge(plv, pitem)) {
                plv->rcView.left = RECOMPUTE;
            }

        }

        // We don't need to invalidate the item in report view because we
        // will be scrolling on top of it.
        //
        if (!ListView_IsReportView(plv))
            ListView_InvalidateItem(plv, iItem, FALSE, RDW_INVALIDATE | RDW_ERASE);

        // this notify must be done AFTER the Invalidate because some items need callbacks
        // to calculate the rect, but the notify might free it out
        ListView_Notify(plv, iItem, 0, LVN_DELETEITEM);

        // During the notify, the app might've done something really stupid
        // so revalidate the item number pointer so we don't fault
#ifdef DEBUG
        // Validate internally because DPA_DeletePtr will ASSERT if you ask it
        // to delete something that doesn't exist.
        if (!ListView_IsValidItemNumber(plv, iItem))
            pitem = NULL;
        else
#endif
            pitem = DPA_DeletePtr(plv->hdpa, iItem);

        if (!pitem)
        {
            RIPMSG(0, "Something strange happened during LVN_DELETEITEM; abandoning LVM_DELETEITEM");
            return FALSE;
        }

        plv->cTotalItems = DPA_GetPtrCount(plv->hdpa);

        // remove from the z-order, this is a linear search to find this!

        DPA_DeletePtr(plv->hdpaZOrder, ListView_ZOrderIndex(plv, iItem));

        //
        // As the Z-order hdpa is a set of indexes we also need to decrement
        // all indexes that exceed the one we are deleting.
        //
        for (iZ = ListView_Count(plv) - 1; iZ >= 0; iZ--)
        {
            int iItemZ = (int)(UINT_PTR)DPA_FastGetPtr(plv->hdpaZOrder, iZ);
            if (iItemZ > iItem)
                DPA_SetPtr(plv->hdpaZOrder, iZ, (LPVOID)(DWORD)(iItemZ - 1));
        }

        // remove from sub item DPAs if necessary

        if (plv->hdpaSubItems)
        {
            int iCol;
            for (iCol = plv->cCol - 1; iCol >= 0; iCol--)
            {
                HDPA hdpa = ListView_GetSubItemDPA(plv, iCol);
                if (hdpa) {     // this is optional, call backs don't have them
                    PLISTSUBITEM plsi;

                    // These DPAs are tail sparse, so don't get upset if we
                    // try to delete something that's past the end of the list
#ifdef DEBUG
                    plsi = iItem < DPA_GetPtrCount(hdpa) ? DPA_DeletePtr(hdpa, iItem) : NULL;
#else
                    plsi = DPA_DeletePtr(hdpa, iItem);
#endif
                    ListView_FreeSubItem(plsi);
                }
            }
        }

        ListView_FreeItem(plv, pitem);  // ... finaly the item pointer

    }
    else
    {
        //
        // simply notify and then fixup selection state and count
        //
        if ((iItem >= 0) && (iItem <= MAX_LISTVIEWITEMS))
        {
            ListView_Notify(plv, iItem, 0, LVN_DELETEITEM);

            if (FAILED(plv->plvrangeSel->lpVtbl->RemoveItem(plv->plvrangeSel, iItem)))
            {
                // BUGBUG: return out of memory status
                //MemoryLowDlg( plv->ci.hwnd );
                return FALSE;
            }
            plv->cTotalItems--;
            plv->rcView.left = RECOMPUTE;
            ListView_Recompute(plv);

            if (!ListView_IsReportView(plv) && !ListView_IsListView(plv))
                {
                // We need to erase the background so that the last item gets
                // erased in both icon modes and so that we don't leave turds
                // from wrapped labels in large icon mode.  This could be
                // optimized by only invalidating to the right of and below
                // the deleted item.
                InvalidateRect( plv->ci.hwnd, NULL, TRUE );
                }
        }
        else
        {
            return FALSE;
        }
    }

    iCount = ListView_Count(plv);       // regrab count incase someone updated item...

    if (!ListView_IsOwnerData(plv))
        ASSERT(ListView_Count(plv) == DPA_GetPtrCount(plv->hdpaZOrder));

    if (plv->iFocus == iItem) {
        if (plv->iFocus >= iCount) {
            plv->iFocus = iCount - 1;
        }
    } if (plv->iFocus > iItem) {
        plv->iFocus--;          // slide the focus index down
    }

    // same with the mark
    if (plv->iMark == iItem)  { // deleted the mark item

        if (plv->iMark >= iCount) // did we nuke the last item?
            plv->iMark = iCount - 1;

    } else if (plv->iMark > iItem)
        plv->iMark--;          // slide the mark index down

    // Deleting an icon invalidates the icon positioning cache
    plv->iFreeSlot = -1;

    // HACK ALERT!! -- This construct with ReportView steals code from
    // ListView_OnUpdate.  Currently, it will work exactly the same as before,
    // EXCEPT, that it won't call ListView_OnUpdate.  This is to allow us to
    // send a flag to ListView_LRUpdateBelow to tell it we're scrolling up.
    //
    if (ListView_IsReportView(plv)) {

        // if the new count is zero and we will be showing empty text, simply invalidate the
        // rect and redraw, else go through the invalidate below code...
        
        // we don't know if we are going to show empty text if pszEmptyText is NULL, or not
        // because we may get one through notify, so if iCount is 0 invalidate everything
        if (iCount == 0)
            InvalidateRect( plv->ci.hwnd, NULL, TRUE );
        else
            ListView_LRInvalidateBelow(plv,iItem,1);



        if (ListView_RedrawEnabled(plv))
            ListView_UpdateScrollBars(plv);
        else {
            //
            // Special case code to make using SetRedraw work reasonably well
            // for adding items to a listview which is in a non layout mode...
            //
            if ((plv->iFirstChangedNoRedraw != -1) && (iItem < plv->iFirstChangedNoRedraw))
                plv->iFirstChangedNoRedraw--;
        }
    }
    else {
        if (ListView_RedrawEnabled(plv))
            ListView_OnUpdate(plv, iItem);

        else
        {
            ListView_LRInvalidateBelow(plv, iItem, 0);
            //
            // Special case code to make using SetRedraw work reasonably well
            // for adding items to a listview which is in a non layout mode...
            //
            if ((plv->iFirstChangedNoRedraw != -1) && (iItem < plv->iFirstChangedNoRedraw))
                plv->iFirstChangedNoRedraw--;
        }
    }
    ListView_RecalcRegion(plv, TRUE, TRUE);

    return TRUE;
}

BOOL NEAR ListView_OnDeleteAllItems(LV* plv)
{
    int i;
    BOOL bAlreadyNotified;
    BOOL fHasItemData;

    fHasItemData = !ListView_IsOwnerData(plv);

    ListView_DismissEdit(plv, TRUE);    // cancel edits

    // Must neutralize the focus because some apps will call
    // ListView_OnGetNextItem(LVNI_FOCUSED) during delete notifications,
    // so we need to make sure the focus is in a safe place.
    // May as well neutralize the mark, too.
    plv->iMark = plv->iFocus = -1;

    // Also nuke the icon positioning cache
    plv->iFreeSlot = -1;

    bAlreadyNotified = (BOOL)ListView_Notify(plv, -1, 0, LVN_DELETEALLITEMS);

    ListView_InvalidateTTLastHit(plv, plv->iTTLastHit);

    if (fHasItemData || !bAlreadyNotified)
    {
        for (i = ListView_Count(plv) - 1; i >= 0; i--)
        {
            if (!bAlreadyNotified)
                ListView_Notify(plv, i, 0, LVN_DELETEITEM);

            if (fHasItemData)
            {
                ListView_FreeItem(plv, ListView_FastGetItemPtr(plv, i));
                //
                //  CAREFUL!  Applications such as NT Backup call back
                //  into ListView during the LVN_DELETEITEM notification,
                //  so we need to kill this item or we will fault at the
                //  next iteration because everybody relies on
                //  ListView_Count for validation.
                //
                DPA_FastDeleteLastPtr(plv->hdpa);
                plv->cTotalItems--;
            }
        }
    }

   if (ListView_IsOwnerData( plv ))
    {
      if (FAILED(plv->plvrangeSel->lpVtbl->Clear( plv->plvrangeSel )))
        {
            // BUGBUG: return low memory status
            //MemoryLowDlg( plv->ci.hwnd );
        }
        plv->cTotalItems = 0;
    }
    else
    {
        DPA_DeleteAllPtrs(plv->hdpa);
        DPA_DeleteAllPtrs(plv->hdpaZOrder);
        plv->cTotalItems = 0;

        if (plv->hdpaSubItems)
        {
            int iCol;
            for (iCol = plv->cCol - 1; iCol >= 0; iCol--)
            {
                HDPA hdpa = ListView_GetSubItemDPA(plv, iCol);
                if (hdpa) {
                    DPA_EnumCallback(hdpa, ListView_FreeColumnData, 0);
                    DPA_DeleteAllPtrs(hdpa);
                }
            }
        }
    }

    plv->rcView.left = RECOMPUTE;
    plv->xOrigin = 0;
    plv->nSelected = 0;

    plv->ptlRptOrigin.x = 0;
    plv->ptlRptOrigin.y = 0;

    // reset the cxItem width
    if (!(plv->flags & LVF_COLSIZESET))
        plv->cxItem = 16 * plv->cxLabelChar + plv->cxSmIcon;

    RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
    ListView_UpdateScrollBars(plv);

    return TRUE;
}

int PASCAL ListView_IFindNearestItem(LV* plv, int left, int top, UINT vk)
{
   int iMin = -1;

   if (ListView_IsOwnerData( plv ))
   {
      POINT pt;
      int cSlots;

      ASSERT( !ListView_IsReportView( plv ) && !ListView_IsListView( plv ) );

      pt.x = left + plv->ptOrigin.x;
      pt.y = top + plv->ptOrigin.y;

      cSlots = ListView_GetSlotCount( plv, TRUE );
      iMin = ListView_CalcHitSlot( plv, pt, cSlots );

      switch( vk )
      {
      case VK_HOME:
         iMin = 0;
         break;

      case VK_END:
         iMin = ListView_Count( plv ) - 1;
         break;

      case VK_LEFT:
         if (iMin % cSlots)
            iMin -= 1;
         break;

      case VK_RIGHT:
         if ((iMin + 1) % cSlots)
            iMin += 1;
         break;

      case VK_UP:
         if (iMin >= cSlots)
            iMin -= cSlots;
         break;

      case VK_DOWN:
         if (iMin + cSlots < ListView_Count( plv ))
            iMin += cSlots;
         break;

      default: ;
      }

      iMin = max( 0, iMin );
      iMin = min( ListView_Count( plv ) - 1, iMin );

   }
   else
    {
       DWORD dMin = 0;
       int cyItem;
       int yEnd = 0, yLimit = 0, xEnd = 0;
       int iCount;
       int i;

       if (ListView_IsIconView(plv)) {
           cyItem = plv->cyIcon;
       } else {
           cyItem = plv->cyItem;
       }

       iCount = ListView_Count(plv);
       if (iCount == 1)
           return 0;

       if (vk == VK_HOME) {
           yEnd = yLimit = plv->rcView.bottom;
           xEnd = plv->rcView.right;
       } else if (vk == VK_END) {

           yEnd = yLimit = plv->rcView.top;
           xEnd = plv->rcView.left;

       }
       for (i = 0; i < iCount; i++)
       {
           RECT rc;
           int dx;
           DWORD dxAbs, dyAbs;
           int dy;
           DWORD dOffset;

           ListView_GetRects(plv, i, &rc, NULL, NULL, NULL);

           dx = rc.left - left;
           dxAbs = (DWORD)(dx < 0 ? -dx : dx);
           dy = rc.top - top;
           dyAbs = (DWORD)(dy < 0 ? -dy : dy);

           if ((vk == VK_LEFT) && (dxAbs < dyAbs || dx >= 0))
               continue;
           else if ((vk == VK_RIGHT) && (dxAbs < dyAbs || dx <= 0))
               continue;
           else if ((vk == VK_UP) && (dxAbs > dyAbs || dy >= 0))
               continue;
           else if ((vk == VK_DOWN) && (dxAbs > dyAbs || dy <= 0))
               continue;

           if (vk == VK_HOME || vk == VK_END) {

               // home is not the nearest to the top corner, it's the leftmost of the top row.
               // ditto (reversed) for end.  thus we can't use the stuff below. bummer
               if (vk == VK_HOME) {
                   if ((rc.top + cyItem < yEnd) ||  // if it's fully above the highest line so far, take it!
                       ((rc.top < yLimit) &&  // if it's on the same row as the top item to date
                        (rc.left < xEnd))) {
                       iMin = i;
                       xEnd = rc.left;
                       yEnd = rc.top;
                       if (rc.top + cyItem < yLimit)
                           yLimit = rc.top + cyItem;
                   }
               } else {
                   if ((rc.top > yEnd) || //if it's full below the lowest row
                       ((rc.top + cyItem > yLimit) && // if it's on the same row
                        (rc.right > xEnd))) {

                       iMin = i;
                       xEnd = rc.right;
                       yEnd = rc.top;
                       if (rc.top > yLimit)
                           yLimit = rc.top;
                   }
               }

           } else {

               dOffset = ((dxAbs * dxAbs) + (dyAbs * dyAbs));
               if (iMin == -1 || (dMin > dOffset))
               {
                   dMin = dOffset;
                   iMin = i;
               }
           }
       }
    }
    return iMin;
}

int NEAR ListView_Arrow(LV* plv, int iStart, UINT vk)
{
    RECT rcFocus;
    int i;
    int dx;
    int iCount;

    //
    // The algorithm to find which item depends if we are in a view
    // that is arrange(layout) oriented or a sorted (list) view.
    // For the sorted views we will use some optimizations to make
    // it faster
    //
    iCount = ListView_Count(plv);
    if (ListView_IsReportView(plv) || ListView_IsListView(plv))
    {
        //
        // For up and down arrows, simply increment or decrement the
        // index.  Note: in listview this will cause it to wrap columns
        // which is fine as it is compatible with the file manager
        //
        // Assumes only one of these flags is set...

        switch (vk)
        {
        case VK_LEFT:
            if (ListView_IsReportView(plv))
            {
                ListView_ROnScroll(plv, (GetAsyncKeyState(VK_CONTROL) < 0) ? SB_PAGELEFT : SB_LINELEFT, 0, SB_HORZ);
            }
            else
                iStart -= plv->cItemCol;
            break;

        case VK_RIGHT:
            if (ListView_IsReportView(plv))
            {
                // Make this horizontally scroll the report view
                ListView_ROnScroll(plv, (GetAsyncKeyState(VK_CONTROL) < 0) ? SB_PAGERIGHT : SB_LINERIGHT, 0, SB_HORZ);
            }
            else
                iStart += plv->cItemCol;
            break;

        case VK_UP:
            iStart--;
            break;

        case VK_DOWN:
            iStart++;
            break;

        case VK_HOME:
            iStart = 0;
            break;

        case VK_END:
            iStart = iCount -1;
            break;

        case VK_NEXT:
            if (ListView_IsReportView(plv))
            {
                i = iStart; // save away to make sure we dont go wrong way!

                // First go to end of page...
                iStart = (int)(((LONG)(plv->sizeClient.cy - (plv->cyItem)
                        - plv->yTop) + plv->ptlRptOrigin.y) / plv->cyItem);

                // If Same item, increment by page size.
                if (iStart <= i)
                    iStart = i + max(
                            (plv->sizeClient.cy - plv->yTop)/ plv->cyItem - 1,
                            1);

                if (iStart >= iCount)
                    iStart = iCount - 1;

            } else {
                // multiply by 2/3 to give a good feel.. when the item is mostly shown
                // you want to go to the next column
                dx = (plv->sizeClient.cx + (plv->cxItem*2)/3) / plv->cxItem;
                if (!dx)
                    dx = 1;

                iStart += plv->cItemCol *  dx;
                if (plv->cItemCol) {
                    while (iStart >= iCount)
                        iStart -= plv->cItemCol;
                }
            }
            break;

        case VK_PRIOR:

            if (ListView_IsReportView(plv))
            {
                i = iStart; // save away to make sure we dont go wrong way!

                // First go to end of page...
                iStart = (int)(plv->ptlRptOrigin.y / plv->cyItem);

                // If Same item, increment by page size.
                if (iStart >= i)
                    iStart = i - max(
                            (plv->sizeClient.cy - plv->yTop)/ plv->cyItem - 1,
                            1);

                if (iStart < 0)
                    iStart = 0;

            } else {
                dx = (plv->sizeClient.cx + (plv->cxItem*2)/3) / plv->cxItem;
                if (!dx)
                    dx = 1;
                iStart -= plv->cItemCol * dx;
                if (plv->cItemCol) {
                    while (iStart < 0)
                        iStart += plv->cItemCol;
                }

            }
            break;

        default:
            return -1;      // Out of range
        }

        // Make sure it is in range!.
        if ((iStart >= 0) && (iStart < iCount))
            return iStart;
        else if (iCount == 1)
            return 0;
        else
            return -1;
    }

    else
    {
        //
        // Layout type view. we need to use the position of the items
        // to figure out the next item
        //

        if (ListView_IsOwnerData( plv ))
        {
          iStart = max( 0, iStart );

            // if it does not matches any of the entries in the case statement below
            // this is done to skip the call back by the GetRects
            //
            if ( vk != VK_LEFT  &&
                    vk != VK_RIGHT &&
                    vk != VK_UP &&
                    vk != VK_DOWN &&
                    vk != VK_HOME &&
                    vk != VK_END &&
                    vk != VK_NEXT &&
                    vk != VK_PRIOR )
            {
                return -1;
            }
            ListView_GetRects(plv, iStart, &rcFocus, NULL, NULL, NULL);
        }
        else
        {
            if (iStart != -1) {
                ListView_GetRects(plv, iStart, &rcFocus, NULL, NULL, NULL);
            }
        }

        switch (vk)
        {
        // For standard arrow keys just fall out of here.
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
            if (ListView_IsOwnerData( plv ))
            {
                break;
            }
            else
            {
                if (iStart != -1) {
                    // all keys map to VK_HOME except VK_END
                    break;
                }

                // Fall through
                vk = VK_HOME;
            }

        case VK_HOME:
            rcFocus.left = - plv->ptOrigin.x;
            rcFocus.top = - plv->ptOrigin.y;
            break;

        case VK_END:
            rcFocus.left = plv->rcView.right;
            rcFocus.top = plv->rcView.bottom;
            break;

        case VK_NEXT:
            rcFocus.top += plv->sizeClient.cy;
            vk = VK_UP;
            break;

        case VK_PRIOR:
            vk = VK_DOWN;
            rcFocus.top -= plv->sizeClient.cy;
            break;
        default:
            return -1;      // Out of range
        }

        return ListView_IFindNearestItem(plv, rcFocus.left, rcFocus.top, vk);
    }
}

int NEAR ListView_OnGetNextItem(LV* plv, int i, UINT flags)
{
    int iStart = i;
    int cItemMax = ListView_Count(plv);

    // Note that -1 is a valid starting point
    if (i < -1 || i >= cItemMax)
        return -1;

    if (ListView_IsOwnerData( plv ))
    {
        if (flags & (LVNI_CUT | LVNI_DROPHILITED | LVNI_PREVIOUS))
        {
            return( -1 );
        }
    }

    if (flags & LVNI_FOCUSED)
    {
        // we know which item is focused, jump right to it.
        // but we have to mimick the code below exactly for compat:
        //     if directional bits are set, they take precedence.
        if (!(flags & (LVNI_ABOVE | LVNI_BELOW | LVNI_TORIGHT | LVNI_TOLEFT)))
        {
            // there are no more focused items after iFocus
            if (i >= plv->iFocus)
                return -1;

            // subtract one here -- we increment it below
            i = plv->iFocus - 1;
        }
    }

    while (TRUE)
    {
        // BUGBUG: does anyone call this now???
        if (flags & (LVNI_ABOVE | LVNI_BELOW | LVNI_TORIGHT | LVNI_TOLEFT))
        {
            UINT vk;
            if (flags & LVNI_ABOVE)
                vk = VK_UP;
            else if (flags & LVNI_BELOW)
                vk = VK_DOWN;
            else if (flags & LVNI_TORIGHT)
                vk = VK_RIGHT;
            else
                vk = VK_LEFT;

            if (i != -1)
                i = ListView_Arrow(plv, i, vk);
            if (i == -1)
                return i;

        }
        else
        {
            i++;
            if (i == cItemMax)
                return -1;
        }

        // See if any other restrictions are set
        if (flags & ~(LVNI_ABOVE | LVNI_BELOW | LVNI_TORIGHT | LVNI_TOLEFT))
        {
            WORD wItemState;

            if (ListView_IsOwnerData( plv ))
            {
                if (flags & LVNI_FOCUSED)
                {
                    // we check LVNI_FOCUSED before the loop, so i == iFocus
                    ASSERT(i == plv->iFocus && i != -1);
                    if (flags & LVNI_SELECTED)
                    {
                        if (plv->plvrangeSel->lpVtbl->IsSelected(plv->plvrangeSel, i ) != S_OK)
                        {
                            i = -1;
                        }
                    }
                }
                else if (flags & LVNI_SELECTED)
                {
                    i = max( i, 0 );
                    plv->plvrangeSel->lpVtbl->NextSelected(plv->plvrangeSel, i, &i );
                }
                else
                {
                    i = -1;
                }
            }
            else
            {
                {
                    LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, i);
                    wItemState = pitem->state;
                }

                // for LVNI_FOCUSED, we start at the LVIS_FOCUSED element, if we're
                // not on that element, one of the below continues was hit, so
                // we'll never find the element. bail out early.
                if ((flags & LVNI_FOCUSED) && !(wItemState & LVIS_FOCUSED))
                {
                    ASSERT(i == plv->iFocus || i == plv->iFocus+1);
                    return(-1);
                }

                if (((flags & LVNI_SELECTED) && !(wItemState & LVIS_SELECTED)) ||
                    ((flags & LVNI_CUT) && !(wItemState & LVIS_CUT)) ||
                    ((flags & LVNI_DROPHILITED) && !(wItemState & LVIS_DROPHILITED))) {
                    if (i != iStart)
                        continue;
                    else {
                        // we've looped and we can't find anything to fit this criteria
                        return -1;
                    }
                }
            }
        }
        return i;
    }
}

int NEAR ListView_CompareString(LV* plv, int i, LPCTSTR pszFind, UINT flags, int iLen)
{
    // BUGBUG: non protected globals
    int cb;
    TCHAR ach[CCHLABELMAX];
    LV_ITEM item;

    ASSERT(!ListView_IsOwnerData(plv));
    ASSERT(pszFind);

    item.iItem = i;
    item.iSubItem = 0;
    item.mask = LVIF_TEXT;
    item.pszText = ach;
    item.cchTextMax = ARRAYSIZE(ach);
    ListView_OnGetItem(plv, &item);

    if (!(flags & (LVFI_PARTIAL | LVFI_SUBSTRING)))
        return lstrcmpi(item.pszText, pszFind);

    // REVIEW: LVFI_SUBSTRING is not really implemented yet.

    cb = lstrlen(pszFind);
    if (iLen && (cb > iLen)) {
            cb = iLen;
    }

    //
    // If the sub strings not equal then return the ordering based
    // on the entire string.
    //
#ifndef UNIX
    return IntlStrEqNI(item.pszText, pszFind, cb) ? 0 : lstrcmp(item.pszText, pszFind);
#else
    return IntlStrEqNI(item.pszText, pszFind, cb) ? 0 : lstrcmpi(item.pszText, pszFind);
#endif

}

#ifdef UNICODE
int NEAR ListView_OnFindItemA(LV* plv, int iStart, LV_FINDINFOA * plvfi) {
    LPWSTR pszW = NULL;
    LPCSTR pszC = NULL;
    int iRet;

    //HACK ALERT -- this code assumes that LV_FINDINFOA is exactly the same
    // as LV_FINDINFOW except for the pointer to the string.
    ASSERT(sizeof(LV_FINDINFOA) == sizeof(LV_FINDINFOW));

    if (!plvfi)
        return -1;

    if (!(plvfi->flags & LVFI_PARAM) && !(plvfi->flags & LVFI_NEARESTXY)) {
        pszC = plvfi->psz;
        if ((pszW = ProduceWFromA(plv->ci.uiCodePage, pszC)) == NULL)
            return -1;
        plvfi->psz = (LPSTR)pszW;
    }

    iRet = ListView_OnFindItem(plv, iStart, (const LV_FINDINFO FAR *)plvfi);

    if (pszW != NULL) {
        plvfi->psz = pszC;

        FreeProducedString(pszW);
    }

    return iRet;
}
#endif

int NEAR ListView_OnFindItem(LV* plv, int iStart, const LV_FINDINFO FAR* plvfi)
{
    int i;
    int j;
    int cItem;
    UINT flags;

    if (!plvfi)
        return -1;

    if (plvfi->flags & LVFI_NEARESTXY) {
        if (ListView_IsIconView(plv) || ListView_IsSmallView(plv)) {
            return ListView_IFindNearestItem(plv, plvfi->pt.x, plvfi->pt.y, plvfi->vkDirection);
        } else
            return -1;
    }

    // Note that -1 is a valid starting point
    if (iStart < -1 || iStart >= ListView_Count(plv))
        return -1;

    if (ListView_IsOwnerData( plv ))
    {
        // call back to owner for search
        return( (int) ListView_RequestFindItem( plv, plvfi, iStart + 1 ) );
    }
    else
    {
        flags  = plvfi->flags;
        i = iStart;
        cItem = ListView_Count(plv);
        if (flags & LVFI_PARAM)
        {
            LPARAM lParam = plvfi->lParam;

            // Linear search with wraparound...
            //
            for (j = cItem; j-- != 0; )
            {
                ++i;
                if (i == cItem) {
                    if (flags & LVFI_WRAP)
                        i = 0;
                    else
                        break;
                }

                if (ListView_FastGetItemPtr(plv, i)->lParam == lParam)
                    return i;
            }
        }
        else // if (flags & (LVFI_STRING | LVFI_SUBSTRING | LVFI_PARTIAL))
        {
            LPCTSTR pszFind = plvfi->psz;
            if (!pszFind)
                return -1;

            if (plv->ci.style & (LVS_SORTASCENDING | LVS_SORTDESCENDING))
                return ListView_LookupString(plv, pszFind, flags, i + 1);

            for (j = cItem; j-- != 0; )
            {
                ++i;
                if (i == cItem) {
                    if (flags & LVFI_WRAP)
                        i = 0;
                    else
                        break;
                }

                if (ListView_CompareString(plv,
                                           i,
                                           pszFind,
                                           (flags & (LVFI_PARTIAL | LVFI_SUBSTRING)), 0) == 0)
                {
                    return i;
                }
            }
        }
    }
    return -1;
}

BOOL NEAR ListView_OnGetItemRect(LV* plv, int i, RECT FAR* prc)
{
#ifndef UNIX
    LPRECT pRects[LVIR_MAX];
#else
    volatile LPRECT pRects[LVIR_MAX]; /* this is used by the irix compiler */
#endif

    // validate parameters
    if (!ListView_IsValidItemNumber(plv, i))
    {
        RIPMSG(0, "LVM_GETITEMRECT: invalid index %d", i);
        return FALSE;
    }

    if (!prc || prc->left >= LVIR_MAX || prc->left < 0)
    {
        RIPMSG(0, "LVM_GETITEMRECT: invalid rect pointer");
        return FALSE;
    }

    pRects[0] = NULL;
    pRects[1] = NULL;
    pRects[2] = NULL;
    pRects[3] = NULL;

    pRects[prc->left] = prc;
    ListView_GetRects(plv, i, pRects[LVIR_ICON], pRects[LVIR_LABEL],
                      pRects[LVIR_BOUNDS], pRects[LVIR_SELECTBOUNDS]);
    return TRUE;
}

//
// in:
//      plv
//      iItem           MUST be a valid item index (in range)
// out:
//   prcIcon            icon bounding rect
//   prcLabel           label text bounding rect, for details this is the first column
//   prcBounds          entire item (all text and icon), including columns in details
//   prcSelectionBounds union of icon and label rects, does NOT include columns
//                      in details view

// BUGBUG raymondc - Need to pass an HDC parameter for measurement
// since sometimes we do this while painting

void NEAR ListView_GetRects(LV* plv, int iItem,
        RECT FAR* prcIcon, RECT FAR* prcLabel, RECT FAR* prcBounds,
        RECT FAR* prcSelectBounds)
{
    ASSERT(plv);

    if (ListView_IsReportView(plv))
    {
        ListView_RGetRects(plv, iItem, prcIcon, prcLabel, prcBounds, prcSelectBounds);
    }
    else if (ListView_IsListView(plv))
    {
        ListView_LGetRects(plv, iItem, prcIcon, prcLabel, prcBounds, prcSelectBounds);
    }
    else
    {
       if (ListView_IsOwnerData( plv ))
       {
           RECT rcIcon;
           RECT rcTextBounds;
           LISTITEM item;
           
           if (ListView_IsIconView(plv))
               ListView_IGetRectsOwnerData(plv, iItem, &rcIcon, &rcTextBounds, &item, FALSE);
           else if (ListView_IsSmallView(plv))
               ListView_SGetRectsOwnerData(plv, iItem, &rcIcon, &rcTextBounds, &item, FALSE);
           if (prcIcon)
               *prcIcon = rcIcon;
           if (prcLabel)
               *prcLabel = rcTextBounds;
           
           if (prcBounds)
               UnionRect(prcBounds, &rcIcon, &rcTextBounds);
           
           if (prcSelectBounds)
               UnionRect(prcSelectBounds, &rcIcon, &rcTextBounds);
       }
       else
       {
           if (iItem >= ListView_Count(plv))
           {
               return;
           }
           else
           {
               LISTITEM FAR *pitem = ListView_FastGetItemPtr(plv, iItem);
               
               if (pitem->cyFoldedLabel == SRECOMPUTE)
               {
                   ListView_RecomputeLabelSize(plv, pitem, iItem, NULL, FALSE);
               }
               _ListView_GetRectsFromItem(plv, ListView_IsSmallView(plv), pitem,
                   prcIcon, prcLabel, prcBounds, prcSelectBounds);
           }
       }
    }
}

void NEAR ListView_GetRectsOwnerData(LV* plv, int iItem,
        RECT FAR* prcIcon, RECT FAR* prcLabel, RECT FAR* prcBounds,
        RECT FAR* prcSelectBounds, LISTITEM* pitem)
{
    ASSERT(plv);
    ASSERT(ListView_IsOwnerData(plv));

    if (ListView_IsReportView(plv))
    {
        ListView_RGetRects(plv, iItem, prcIcon, prcLabel, prcBounds,
                prcSelectBounds);
    }
    else if (ListView_IsListView(plv))
    {
        ListView_LGetRects(plv, iItem, prcIcon, prcLabel, prcBounds,
                prcSelectBounds);
    }
    else
    {
      RECT rcIcon;
      RECT rcTextBounds;

        if (ListView_IsIconView(plv))
            ListView_IGetRectsOwnerData(plv, iItem, &rcIcon, &rcTextBounds, pitem, TRUE);
        else if (ListView_IsSmallView(plv))
            ListView_SGetRectsOwnerData(plv, iItem, &rcIcon, &rcTextBounds, pitem, TRUE);

        // Don't need to check for folding here, as will have been handled in user data
        // rectangle fetching functions.

       if (prcIcon)
           *prcIcon = rcIcon;
       if (prcLabel)
           *prcLabel = rcTextBounds;

       if (prcBounds)
           UnionRect(prcBounds, &rcIcon, &rcTextBounds);

       if (prcSelectBounds)
           UnionRect(prcSelectBounds, &rcIcon, &rcTextBounds);
    }
}


BOOL NEAR ListView_OnRedrawItems(LV* plv, int iFirst, int iLast)
{
    int iCount = ListView_Count(plv);

    if (iFirst < iCount) {

        if (iLast >= iCount)
            iLast = iCount - 1;

        while (iFirst <= iLast)
            ListView_InvalidateItem(plv, iFirst++, FALSE, RDW_INVALIDATE | RDW_ERASE);
    }
    return TRUE;
}

// fSelectionOnly       use the selection bounds only, ie. don't include
//                      columns in invalidation if in details view
//
void NEAR ListView_InvalidateItemEx(LV* plv, int iItem, BOOL fSelectionOnly,
    UINT fRedraw, UINT maskChanged)
{
    RECT rc;
    LPRECT prcIcon;
    LPRECT prcLabel;
    LPRECT prcBounds;
    LPRECT prcSelectBounds;

    if (iItem == -1)
        return;

    prcIcon = prcLabel = prcBounds = prcSelectBounds = NULL;

    // if we're in owner draw mode, and there's been a new font,
    // we don't really know what the selection bounds is, so always use the bounds
    // in that case... unless we're in fullrowselect mode
    if (ListView_IsOwnerData(plv) && plv->flags & LVF_CUSTOMFONT &&
       !ListView_FullRowSelect(plv)) {
        fSelectionOnly = FALSE;
    }

    // if we're owner draw, there's no such thing as selection only
    if (plv->ci.style & LVS_OWNERDRAWFIXED)
        fSelectionOnly = FALSE;

    if (fSelectionOnly) {
        // In report mode non-fullrowselect,
        // we have to use the full label rectangle rather
        // than just the selection bounds, since the stuff outside the
        // rectangle might need redrawing, too.

        if (ListView_IsReportView(plv) && !ListView_FullRowSelect(plv))
            prcLabel = &rc;
        else
            prcSelectBounds = &rc;
    } else {

        // if _only_the_text_ or _only_the_image_ changed then limit the redraw
        switch (maskChanged) {

        case LVIF_IMAGE:
            prcIcon = &rc;
            break;

        case LVIF_TEXT:
            prcLabel = &rc;
            break;

        default:
            prcBounds = &rc;
            break;
        }
    }

    if (ListView_RedrawEnabled(plv)) {

        ListView_GetRects(plv, iItem,
            prcIcon, prcLabel, prcBounds, prcSelectBounds);

        if (RECTS_IN_SIZE(plv->sizeClient, rc))
        {
            if (plv->exStyle & LVS_EX_BORDERSELECT)
                InflateRect(&rc, 4 + g_cxIconMargin, 4 + g_cyIconMargin);     // account for selection border and seperation since drawing otside of icon
            RedrawWindow(plv->ci.hwnd, &rc, NULL, fRedraw);
        }

    } else {

        // if we're not visible, we'll get a full
        // erase bk when we do become visible, so only do this stuff when
        // we're on setredraw false
        if (!(plv->flags & LVF_REDRAW)) {

            // if we're invalidating that's new (thus hasn't been painted yet)
            // blow it off
            if ((plv->iFirstChangedNoRedraw != -1) &&
                (iItem >= plv->iFirstChangedNoRedraw)) {
                return;
            }

            ListView_GetRects(plv, iItem,
                prcIcon, prcLabel, prcBounds, prcSelectBounds);

            // if it had the erase bit, add it to our region
            if (RECTS_IN_SIZE(plv->sizeClient, rc)) {

                HRGN hrgn = CreateRectRgnIndirect(&rc);

                ListView_InvalidateRegion(plv, hrgn);

                if (fRedraw & RDW_ERASE)
                    plv->flags |= LVF_ERASE;
            }
        }
    }
}

// this returns BF_* flags to indicate which if any edge the item I is touching
// or crossing...
UINT LV_IsItemOnViewEdge(LV* plv, LISTITEM* pitem)
{
    RECT rcItem;
    RECT rcView = plv->rcView;
    UINT uRet = 0;

    // the view rect is enlarged a bit to allow for a little space around
    // the text (see ListView_Recompute())
    rcView.bottom -= g_cyEdge;
    rcView.right -= g_cxEdge;

    _ListView_GetRectsFromItem(plv, ListView_IsSmallView(plv), pitem,
                               NULL, NULL, &rcItem, NULL);
    // translate from window coordinates to listview coordinate
    OffsetRect(&rcItem, plv->ptOrigin.x, plv->ptOrigin.y);

    if (rcItem.right >= rcView.right)
        uRet |= BF_RIGHT;

    if (rcItem.left <= rcView.left)
        uRet |= BF_LEFT;

    if (rcItem.top <= rcView.top)
        uRet |= BF_TOP;

    if (rcItem.bottom >= rcView.bottom)
        uRet |= BF_BOTTOM;

    return uRet;
}

void LV_AdjustViewRectOnMove(LV* plv, LISTITEM *pitem, int x, int y)
{
    plv->iFreeSlot = -1; // The "free slot" cache is no good once an item moves

    // if we have to recompute anyways, don't bother
    if (!ListView_IsOwnerData( plv )) {
        if ((plv->rcView.left != RECOMPUTE) &&
            x != RECOMPUTE && y != RECOMPUTE &&
            pitem->cyFoldedLabel != SRECOMPUTE) {
            RECT rcAfter;
            RECT rcView = plv->rcView;

            // the view rect is enlarged a bit to allow for a little space around
            // the text (see ListView_Recompute())
            rcView.bottom -= g_cyEdge;
            rcView.right -= g_cxEdge;

            if (pitem->pt.x != RECOMPUTE) {
                UINT uEdges;

                uEdges = LV_IsItemOnViewEdge(plv, pitem);

                pitem->pt.x = x;
                pitem->pt.y = y;

                // before and after the move, they need to be touching the
                // same edges or not at all
                if (uEdges != LV_IsItemOnViewEdge(plv, pitem)) {
                    goto FullRecompute;
                }
            } else {
                // if the position wasn't set before
                // we just need to find out what it is afterwards and
                // enlarge the view... we don't need to shrink it
                pitem->pt.x = x;
                pitem->pt.y = y;


            }

            _ListView_GetRectsFromItem(plv, ListView_IsSmallView(plv), pitem,
                                       NULL, NULL, &rcAfter, NULL);
            // translate from window coordinates to listview coordinate
            OffsetRect(&rcAfter, plv->ptOrigin.x, plv->ptOrigin.y);

            // if we make it here, we just have to make sure the new view rect
            // encompases this new item
            UnionRect(&rcView, &rcView, &rcAfter);
            rcView.right += g_cxEdge;
            rcView.bottom += g_cyEdge;

            DebugMsg(TF_LISTVIEW, TEXT("Score! (%d %d %d %d) was (%d %d %d %d)"),
                     rcView.left, rcView.top, rcView.right, rcView.bottom,
                     plv->rcView.left, plv->rcView.top, plv->rcView.right, plv->rcView.bottom);

            plv->rcView = rcView;

        } else {
    FullRecompute:
            plv->rcView.left = RECOMPUTE;
        }
    }

    DebugMsg(TF_LISTVIEW, TEXT("LV -- AdjustViewRect pitem %d -- (%x, %x)"),
             pitem,
             pitem->pt.x, pitem->pt.y);

    pitem->pt.x = x;
    pitem->pt.y = y;

    // Compute the workarea of this item if applicable
    ListView_FindWorkArea(plv, pitem->pt, &(pitem->iWorkArea));
}

BOOL NEAR ListView_OnSetItemPosition(LV* plv, int i, int x, int y)
{
    LISTITEM FAR* pitem;

    if (ListView_IsListView(plv))
        return FALSE;

    if (ListView_IsOwnerData( plv ))
    {
       RIPMSG(0, "LVM_SETITEMPOSITION: Invalid for owner-data listview");
       return FALSE;
    }

    pitem = ListView_GetItemPtr(plv, i);
    if (!pitem)
        return FALSE;

    //
    // this is a hack to fix a bug in OLE drag/drop loop
    //
    if (x >= 0xF000 && x < 0x10000)
    {
        DebugMsg(TF_LISTVIEW, TEXT("LV -- On SetItemPosition fixing truncated negative number 0x%08X"), x);
        x = x - 0x10000;
    }

    if (y >= 0xF000 && y < 0x10000)
    {
        DebugMsg(TF_LISTVIEW, TEXT("LV -- On SetItemPosition fixing truncated negative number 0x%08X"), y);
        y = y - 0x10000;
    }

    ListView_InvalidateTTLastHit(plv, plv->iTTLastHit);

    if (pitem->cyFoldedLabel == SRECOMPUTE)
    {
        ListView_RecomputeLabelSize(plv, pitem, i, NULL, FALSE);
    }

    // erase old

    if (y != pitem->pt.y || x != pitem->pt.x) {
        // Don't invalidate if it hasn't got a position yet
        if (pitem->pt.y != RECOMPUTE) {
            ListView_InvalidateItem(plv, i, FALSE, RDW_INVALIDATE | RDW_ERASE);
        } else if (plv->uUnplaced) {
            // this means an unplaced item got placed
            plv->uUnplaced--;
            if (!plv->uUnplaced) {
                MSG msg;
                // if this is now 0, pull out the postmessage
                PeekMessage(&msg, plv->ci.hwnd, LVMI_PLACEITEMS, LVMI_PLACEITEMS, PM_REMOVE);
            }
        }

        if (y == RECOMPUTE) {
            // if they're setting the new position to be a "any open spot" post that we
            // need to calc this later
            if (!plv->uUnplaced) {
                PostMessage(plv->ci.hwnd, LVMI_PLACEITEMS, 0, 0);
            }
            plv->uUnplaced++;
        }
    }

    DebugMsg(TF_LISTVIEW, TEXT("LV -- On SetItemPosition %d %d %d %d -- (%x, %x)"),
             plv->rcView.left, plv->rcView.top, plv->rcView.right, plv->rcView.bottom,
             pitem->pt.x, pitem->pt.y);


    LV_AdjustViewRectOnMove(plv, pitem, x, y);

    // and draw at new position
    ListView_RecalcRegion(plv, FALSE, TRUE);
    ListView_InvalidateItem(plv, i, FALSE, RDW_INVALIDATE);

    // If autoarrange is turned on, do it now...
    if (ListView_RedrawEnabled(plv)) {
        if (plv->ci.style & LVS_AUTOARRANGE)
            ListView_OnArrange(plv, LVA_DEFAULT);
        else
            ListView_UpdateScrollBars(plv);
    }

    return TRUE;
}

BOOL NEAR ListView_OnGetItemPosition(LV* plv, int i, POINT FAR* ppt)
{
    LISTITEM FAR* pitem;

    //
    // This needs to handle all views as it is used to figure out
    // where the item is during drag and drop and the like
    //
    if (!ppt)
    {
        RIPMSG(0, "LVM_GETITEMPOSITION: Invalid ppt = NULL");
        return FALSE;
    }

    if (ListView_IsListView(plv) || ListView_IsReportView(plv)
        || ListView_IsOwnerData( plv ))
    {
        RECT rcIcon;
        ListView_GetRects(plv, i, &rcIcon, NULL, NULL, NULL);
        ppt->x = rcIcon.left;
        ppt->y = rcIcon.top;

    } else {

        pitem = ListView_GetItemPtr(plv, i);
        if (!pitem)
            return FALSE;

        if (pitem->pt.x == RECOMPUTE)
            ListView_Recompute(plv);

        ppt->x = pitem->pt.x;
        ppt->y = pitem->pt.y;
    }
    return TRUE;
}




BOOL NEAR ListView_OnGetOrigin(LV* plv, POINT FAR* ppt)
{
    if (!ppt) {
        DebugMsg(DM_ERROR, TEXT("ListView_OnGetOrigin: ppt is NULL"));
        return FALSE;
    }

    if (ListView_IsListView(plv) || ListView_IsReportView(plv))
        return FALSE;

    *ppt = plv->ptOrigin;
    return TRUE;
}



#ifdef UNICODE
int NEAR ListView_OnGetStringWidthA(LV* plv, LPCSTR psz, HDC hdc) {
    LPWSTR pszW = NULL;
    int iRet;

    if (!psz)
        return 0;

    if ((psz != NULL) && (pszW = ProduceWFromA(plv->ci.uiCodePage, psz)) == NULL)
        return 0;

    iRet = ListView_OnGetStringWidth(plv, pszW, hdc);

    FreeProducedString(pszW);

    return iRet;
}
#endif

int NEAR ListView_OnGetStringWidth(LV* plv, LPCTSTR psz, HDC hdc)
{
    SIZE siz;
    HDC hdcFree = NULL;
    HFONT hfontPrev;

    if (!psz || psz == LPSTR_TEXTCALLBACK)
        return 0;

    if (!hdc) {
        hdcFree = hdc = GetDC(plv->ci.hwnd);
        hfontPrev = SelectFont(hdc, plv->hfontLabel);
    }

    GetTextExtentPoint(hdc, psz, lstrlen(psz), &siz);

    if (hdcFree) {
        SelectFont(hdc, hfontPrev);
        ReleaseDC(plv->ci.hwnd, hdcFree);
    }

    return siz.cx;
}

int NEAR ListView_OnGetColumnWidth(LV* plv, int iCol)
{
    if (ListView_IsReportView(plv))
        return ListView_RGetColumnWidth(plv, iCol);
    else if (ListView_IsListView(plv))
        return plv->cxItem;

    return 0;
}

BOOL FAR PASCAL ListView_ISetColumnWidth(LV* plv, int iCol, int cx, BOOL fExplicit)
{

    if (ListView_IsListView(plv))
    {
        if (iCol != 0 || cx <= 0)
            return FALSE;

        // if it's different and this is an explicit set, or we've never set it explicitly
        if (plv->cxItem != cx && (fExplicit || !(plv->flags & LVF_COLSIZESET)))
        {
            // REVIEW: Should optimize what gets invalidated here...

            plv->cxItem = cx;
            if (fExplicit)
                plv->flags |= LVF_COLSIZESET;   // Set the fact that we explictly set size!.

            if (ListView_IsLabelTip(plv))
            {
                // A truncated label may have been exposed or vice versa.
                ListView_InvalidateTTLastHit(plv, plv->iTTLastHit);
            }

            RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
            ListView_UpdateScrollBars(plv);
        }
        return TRUE;
    }
    else if (ListView_IsReportView(plv))
    {
        if (ListView_IsLabelTip(plv))
        {
            // A truncated label may have been exposed or vice versa.
            ListView_InvalidateTTLastHit(plv, plv->iTTLastHit);
        }
        return ListView_RSetColumnWidth(plv, iCol, cx);
    } else {
        if (cx && plv->cxItem != cx && (fExplicit || !(plv->flags & LVF_COLSIZESET)))
        {
            // REVIEW: Should optimize what gets invalidated here...
            plv->cxItem = cx;
            if (fExplicit)
                plv->flags |= LVF_COLSIZESET;   // Set the fact that we explictly set size!.

            RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
            ListView_UpdateScrollBars(plv);
        }
        // BUG-FOR-BUG COMPATIBILITY:  IE4 accidentally returned FALSE here.
    }
    return FALSE;
}

void NEAR ListView_Redraw(LV* plv, HDC hdc, RECT FAR* prcClip)
{
    int i;
    int cItem = ListView_Count(plv);
    DWORD dwType = plv->ci.style & LVS_TYPEMASK;
    NMCUSTOMDRAW nmcd;
    LVDRAWITEM lvdi;

    SetBkMode(hdc, TRANSPARENT);
    SelectFont(hdc, plv->hfontLabel);

    nmcd.hdc = hdc;

    /// not implemented yet
    //if (ptb->ci.hwnd == GetFocus())
    //nmcd.uItemState = CDIS_FOCUS;
    //else
    nmcd.uItemState = 0;
    nmcd.lItemlParam = 0;
    nmcd.rc = *prcClip;
    plv->ci.dwCustom = CICustomDrawNotify(&plv->ci, CDDS_PREPAINT, &nmcd);
    if (!(plv->ci.dwCustom & CDRF_SKIPDEFAULT)) {
        // Just before doing any painting, see if the region is up to date...
        ListView_RecalcRegion(plv, FALSE, TRUE);

        //
        // For list view and report view, we can save a lot of time
        // by calculating the index of the first item that may need
        // painting...
        //

        switch (dwType) {
        case LVS_REPORT:
            i = ListView_RYHitTest(plv, prcClip->top);
            cItem = ListView_RYHitTest(plv, prcClip->bottom) + 1;
            break;

        case LVS_LIST:
            i = ListView_LCalcViewItem(plv, prcClip->left, prcClip->top );
            cItem = ListView_LCalcViewItem( plv, prcClip->right, prcClip->bottom ) + 1;
            break;

        default:
            if (ListView_IsOwnerData( plv ))
            {
                ListView_CalcMinMaxIndex( plv, prcClip, &i, &cItem );
                break;
            }
            else
            {
                // REVIEW: we can keep a flag which tracks whether the view is
                // presently in pre-arranged order and bypass Zorder when it is
                i = 0;  // Icon views no such hint
            }
        }

        if (i < 0)
            i = 0;

        cItem = min( ListView_Count( plv ), cItem );
        if (ListView_IsOwnerData( plv ) && (cItem > i))
        {
            ListView_NotifyCacheHint( plv, i, cItem-1 );
            ListView_LazyCreateWinEvents( plv, i, cItem-1);
        }

        lvdi.plv = plv;
        lvdi.nmcd.nmcd.hdc = hdc;
        lvdi.prcClip = prcClip;
        lvdi.pitem = NULL;

        for (; i < cItem; i++)
        {
            BOOL bSuccess;
            int i2;

            if ((dwType == LVS_ICON || dwType == LVS_SMALLICON)
                && (!ListView_IsOwnerData(plv)))
            {
                LISTITEM FAR *pitem;

                // Icon views: Draw back-to-front mapped through
                // Z-order array for proper Z order appearance - If autoarrange
                // is on, we don't need to do this as our arrange code is setup
                // to not overlap items!
                //
                // For the cases where we might have overlap, we sped this up,
                // by converting the hdpaZorder into a list of indexes instead
                // of pointers.  This ovoids the costly convert pointer to
                // index call.
                //
                i2 = (int)(UINT_PTR)DPA_FastGetPtr(plv->hdpaZOrder, (cItem - 1) -i);

                //
                // do a fast clip check on the item so we dont even try to
                // draw it unless it is visible
                //
                // for small icon view we cant clip on the left without
                // getting the text
                //
                // for large icon view we cant clip on the top without
                // getting the text
                //
                // for large icon view in NOLABELWRAP mode, we can't clip
                // on the top without getting the text, nor can we clip to
                // the left or right in case the text is long.
                //
                // we can always clip to the bottom
                //
                pitem = ListView_FastGetItemPtr(plv, i2);

                if (pitem->pt.x != RECOMPUTE)
                {
                    if (pitem->pt.y - plv->ptOrigin.y > prcClip->bottom)
                        continue;

                    if (dwType == LVS_SMALLICON)
                    {
                        if (pitem->pt.x - plv->ptOrigin.x - plv->cxState > prcClip->right)
                            continue;

                        if (pitem->pt.y + plv->cyItem - plv->ptOrigin.y < prcClip->top)
                            continue;
                    }
                    else if (!(plv->ci.style & LVS_NOLABELWRAP))
                    {
                        if (pitem->pt.x - plv->cxIconSpacing - plv->ptOrigin.x > prcClip->right)
                            continue;

                        if (pitem->pt.x + plv->cxIconSpacing - plv->ptOrigin.x < prcClip->left)
                            continue;
                    }
                }
            }
            else
                i2 = i;

            plv->iItemDrawing = i2;

            lvdi.nmcd.nmcd.dwItemSpec = i2;

            // these may get changed
            lvdi.lpptOrg = NULL;
            lvdi.flags = 0;
            lvdi.nmcd.clrText = plv->clrText;
            lvdi.nmcd.clrTextBk = plv->clrTextBk;

            bSuccess = ListView_DrawItem(&lvdi);

            if (!bSuccess) {
                break;
            }
        }

        if ((dwType == LVS_ICON || dwType == LVS_SMALLICON)
            && (ListView_IsOwnerData(plv)) && 
            plv->iFocus != -1) {
            // since there's no zorder in ownerdata, we explicitly draw the focus guy last (again)
            // so that it'll appear on top
            // we may potentially want to do this for all items that are selected
            plv->iItemDrawing = plv->iFocus;

            lvdi.nmcd.nmcd.dwItemSpec = plv->iItemDrawing;

            // these may get changed
            lvdi.lpptOrg = NULL;
            lvdi.flags = 0;
            lvdi.nmcd.clrText = plv->clrText;
            lvdi.nmcd.clrTextBk = plv->clrTextBk;

            ListView_DrawItem(&lvdi);
        }


            
        // this is an NT5/Memphis feature.

        if (ListView_Count(plv) == 0)
        {
            // there're no items in this view
            // check if we need to display some text in this case.

            if (ListView_GetEmptyText(plv))
            {
                RECT rcClip;
                UINT flags = 0;

                // Put some edging between the text and the border of the
                // window so we don't slam up against the border.
                // This keeps DBCS from looking horrid.
                rcClip.left = g_cxEdge;
                rcClip.top = g_cyEdge;

#ifdef WINDOWS_ME
                if (plv->dwExStyle & WS_EX_RTLREADING)
                    flags |= SHDT_RTLREADING;
#endif
                // if its a report view && we have a header then move the text down
                if (ListView_IsReportView(plv) && (!(plv->ci.style & LVS_NOCOLUMNHEADER)))
                    rcClip.top += plv->cyItem;

                // Note: Use the full sizeClient.cx as the right margin
                // in case pszEmptyText is wider than the client rectangle.

                rcClip.left -= (int)plv->ptlRptOrigin.x;
                rcClip.right = plv->sizeClient.cx;
                rcClip.bottom = rcClip.top + plv->cyItem;

                SHDrawText(hdc, plv->pszEmptyText,
                    &rcClip, LVCFMT_LEFT, flags,
                    plv->cyLabelChar, plv->cxEllipses,
                    plv->clrText, plv->clrBk);
            }
        }

        plv->iItemDrawing = -1;

        // post painting.... this is to do any extra (non item) painting
        // such a grid lines
        switch (dwType) {
        case LVS_REPORT:
            ListView_RAfterRedraw(plv, hdc);
            break;
        }

        // notify parent afterwards if they want us to
        if (plv->ci.dwCustom & CDRF_NOTIFYPOSTPAINT) {
            CICustomDrawNotify(&plv->ci, CDDS_POSTPAINT, &nmcd);
        }
    }
}

BOOL NEAR ListView_DrawItem(PLVDRAWITEM plvdi)
{
    BOOL bRet = TRUE;
    UINT state;

    if (!ListView_IsOwnerData( plvdi->plv )) {
        plvdi->pitem = ListView_FastGetItemPtr(plvdi->plv, plvdi->nmcd.nmcd.dwItemSpec);
    }

    // notify on custom draw then do it!
    plvdi->nmcd.nmcd.uItemState = 0;
    plvdi->nmcd.nmcd.lItemlParam = (plvdi->pitem)? plvdi->pitem->lParam : 0;

    if (!(plvdi->flags & LVDI_NOWAYFOCUS))
    {
        if (plvdi->plv->flags & LVF_FOCUSED) {

            // if we're ownerdraw or asked to callback, go
            // fetch the state
            if (!plvdi->pitem || (plvdi->plv->stateCallbackMask & (LVIS_SELECTED | LVIS_FOCUSED))) {

                state = (WORD) ListView_OnGetItemState(plvdi->plv, (int) plvdi->nmcd.nmcd.dwItemSpec,
                                                LVIS_SELECTED | LVIS_FOCUSED);
            } else {
                state = plvdi->pitem->state;
            }


            if (state & LVIS_FOCUSED) {
                plvdi->nmcd.nmcd.uItemState |= CDIS_FOCUS;
            }

            if (state & LVIS_SELECTED) {
                plvdi->nmcd.nmcd.uItemState |= CDIS_SELECTED;
            }
        }

        // NOTE:  This is a bug.  We should set CDIS_SELECTED only if the item
        // really is selected.  But this bug has existed forever so who knows
        // what apps are relying on it.  Standard workaround is for the client
        // to do a GetItemState and reconfirm the LVIS_SELECTED flag.
        // That's what we do in ListView_DrawImageEx.
        if (plvdi->plv->ci.style & LVS_SHOWSELALWAYS) {
            plvdi->nmcd.nmcd.uItemState |= CDIS_SELECTED;
        }
    }

#ifdef KEYBOARDCUES
    if (!(CCGetUIState(&(plvdi->plv->ci)) & UISF_HIDEFOCUS))
    {
        plvdi->nmcd.nmcd.uItemState |= CDIS_SHOWKEYBOARDCUES;
    }
#endif
    plvdi->nmcd.clrText = plvdi->plv->clrText;
    plvdi->nmcd.clrTextBk = (plvdi->plv->ci.style & WS_DISABLED ? plvdi->plv->clrBk : plvdi->plv->clrTextBk);


    // PERF: if we decide to keep LVIS_EX_TWOCLICKACTIVATE, then we can
    // call ListView_OnGetItem for LVIF_TEXT | LVIF_IMAGE | LVIF_STATE
    // and pass the intem info into _ListView_DrawItem below.
    if (plvdi->plv->iHot == (int)plvdi->nmcd.nmcd.dwItemSpec) {
        // Handle the HOT case
        if(plvdi->plv->clrHotlight != CLR_DEFAULT)
            plvdi->nmcd.clrText = plvdi->plv->clrHotlight;
        else
            plvdi->nmcd.clrText = GetSysColor(COLOR_HOTLIGHT);
        // ie4 bug 47635: if hotlight color is the same as the background
        // color you don't see the text -- slam to a visible color in this case.
        if (plvdi->nmcd.clrText == plvdi->nmcd.clrTextBk)
        {
            if (COLORISLIGHT(plvdi->nmcd.clrTextBk))
                plvdi->nmcd.clrText = 0x000000; // black
            else
                plvdi->nmcd.clrText = 0xFFFFFF; // white
        }
        if ((plvdi->plv->exStyle & LVS_EX_ONECLICKACTIVATE) ||
            ((plvdi->plv->exStyle & LVS_EX_TWOCLICKACTIVATE) &&
             ListView_OnGetItemState(plvdi->plv, (int) plvdi->nmcd.nmcd.dwItemSpec, LVIS_SELECTED))) {
            if ((plvdi->plv->exStyle & LVS_EX_UNDERLINEHOT) &&
                (plvdi->plv->hFontHot))
                SelectFont(plvdi->nmcd.nmcd.hdc, plvdi->plv->hFontHot);
            else
                SelectFont(plvdi->nmcd.nmcd.hdc, plvdi->plv->hfontLabel);
            plvdi->nmcd.nmcd.uItemState |= CDIS_HOT;
        }
    } else if ((plvdi->plv->exStyle & LVS_EX_ONECLICKACTIVATE) ||
               ((plvdi->plv->exStyle & LVS_EX_TWOCLICKACTIVATE) &&
                ListView_OnGetItemState(plvdi->plv, (int) plvdi->nmcd.nmcd.dwItemSpec, LVIS_SELECTED))) {
        // Handle the non-hot webview case
        if ((plvdi->plv->exStyle & LVS_EX_UNDERLINECOLD) && (plvdi->plv->hFontHot))
            SelectFont(plvdi->nmcd.nmcd.hdc, plvdi->plv->hFontHot);
        else
            SelectFont(plvdi->nmcd.nmcd.hdc, plvdi->plv->hfontLabel);
    } else {
        // Handle the non-webview case
        SelectFont(plvdi->nmcd.nmcd.hdc, plvdi->plv->hfontLabel);
    }


    plvdi->dwCustom = CICustomDrawNotify(&plvdi->plv->ci, CDDS_ITEMPREPAINT, &plvdi->nmcd.nmcd);

    plvdi->flags &= ~(LVDI_FOCUS | LVDI_SELECTED);
    if (plvdi->nmcd.nmcd.uItemState & CDIS_FOCUS)
        plvdi->flags |= LVDI_FOCUS;

    if (plvdi->nmcd.nmcd.uItemState & CDIS_SELECTED) {
        if (plvdi->plv->flags & LVF_FOCUSED)
            plvdi->flags |= LVDI_SELECTED;
        else
            plvdi->flags |= LVDI_SELECTNOFOCUS;
        if (plvdi->plv->iHot == (int)plvdi->nmcd.nmcd.dwItemSpec)
            plvdi->flags |= LVDI_HOTSELECTED;
    }

    if (!(plvdi->dwCustom & CDRF_SKIPDEFAULT)) {

        if (!ListView_IsOwnerData( plvdi->plv )) {

#ifdef DEBUG_NEWFONT
            if ((plvdi->nmcd.nmcd.dwItemSpec % 3) == 0) {
                plvdi->dwCustom |= CDRF_NEWFONT;
                SelectObject(plvdi->nmcd.nmcd.hdc, GetStockObject(SYSTEM_FONT));
            }
#endif

            if (plvdi->dwCustom & CDRF_NEWFONT) {
                ListView_RecomputeLabelSize(plvdi->plv, plvdi->pitem, (int) plvdi->nmcd.nmcd.dwItemSpec, plvdi->nmcd.nmcd.hdc, FALSE);
            }
        }

        bRet = _ListView_DrawItem(plvdi);


        if (plvdi->dwCustom & CDRF_NOTIFYPOSTPAINT) {
            plvdi->nmcd.iSubItem = 0;
            CICustomDrawNotify(&plvdi->plv->ci, CDDS_ITEMPOSTPAINT, &plvdi->nmcd.nmcd);
        }
        if (plvdi->dwCustom & CDRF_NEWFONT) {
            SelectObject(plvdi->nmcd.nmcd.hdc, plvdi->plv->hfontLabel);
            plvdi->plv->flags |= LVF_CUSTOMFONT;
        }
    }
    return bRet;
}

// NOTE: this function requires a properly selected font.
//
void WINAPI SHDrawText(HDC hdc, LPCTSTR pszText, RECT FAR* prc, int fmt,
                UINT flags, int cyChar, int cxEllipses, COLORREF clrText, COLORREF clrTextBk)
{
    int cchText;
    COLORREF clrSave, clrSaveBk = 0;
    RECT rc;
    UINT uETOFlags = 0;
    BOOL fForeOnly = FALSE;
    TCHAR ach[CCHLABELMAX + CCHELLIPSES];

#ifdef WINDOWS_ME
    int align;
#endif

    // REVIEW: Performance idea:
    // We could cache the currently selected text color
    // so we don't have to set and restore it each time
    // when the color is the same.
    //
    if (!pszText)
        return;

    if (IsRectEmpty(prc))
        return;

#ifdef WINDOWS_ME
    if (flags & SHDT_RTLREADING) {
        align = GetTextAlign(hdc);
        SetTextAlign(hdc, align | TA_RTLREADING);
    }
#endif

    rc = *prc;

    // If needed, add in a little extra margin...
    //
    if (flags & SHDT_EXTRAMARGIN)
    {
        rc.left  += g_cxLabelMargin * 3;
        rc.right -= g_cxLabelMargin * 3;
    }
    else
    {
        rc.left  += g_cxLabelMargin;
        rc.right -= g_cxLabelMargin;
    }

    if ((rc.left >= rc.right) && !(flags & (SHDT_SELECTED | SHDT_DESELECTED | SHDT_SELECTNOFOCUS)))
        return;

    if ((flags & SHDT_ELLIPSES) &&
            ListView_NeedsEllipses(hdc, pszText, &rc, &cchText, cxEllipses))
    {
        // In some cases cchText was comming back bigger than
        // ARRYASIZE(ach), so we need to make sure we don't overflow the buffer

        // if cchText is too big for the buffer, truncate it down to size
        if (cchText >= ARRAYSIZE(ach) - CCHELLIPSES)
            cchText = ARRAYSIZE(ach) - CCHELLIPSES - 1;

        hmemcpy(ach, pszText, cchText * sizeof(TCHAR));
        lstrcpy(ach + cchText, c_szEllipses);

        pszText = ach;

        // Left-justify, in case there's no room for all of ellipses
        //
        fmt = LVCFMT_LEFT;

        cchText += CCHELLIPSES;
    }
    else
    {
        cchText = lstrlen(pszText);
    }

    if (((clrTextBk == CLR_NONE) && !(flags & (SHDT_SELECTED | SHDT_SELECTNOFOCUS))) || (flags & SHDT_TRANSPARENT))
    {
        fForeOnly = TRUE;
        clrSave = SetTextColor(hdc, (flags & SHDT_TRANSPARENT) ? 0 : clrText);
    }
    else
    {
        HBRUSH hbrUse = NULL;
        HBRUSH hbrDelete = NULL;

        uETOFlags |= ETO_OPAQUE;

        if (flags & SHDT_SELECTED)
        {
            clrText = g_clrHighlightText;
            clrTextBk = (flags & SHDT_HOTSELECTED) ? GetSysColor(COLOR_HOTLIGHT) : g_clrHighlight;

            if (flags & SHDT_DRAWTEXT)
                hbrUse = (flags & SHDT_HOTSELECTED) ? GetSysColorBrush(COLOR_HOTLIGHT) : g_hbrHighlight;

        }
        else if (flags & SHDT_SELECTNOFOCUS)
        {
            if ((clrTextBk == CLR_DEFAULT ? g_clrWindow : clrTextBk) == g_clrBtnFace)
            {
                // if the text background color in this mode is the same as the current
                // background, use the color highlight text so that you can actually see somehting
                clrText = g_clrHighlightText;
                clrTextBk = g_clrHighlight;
                if (flags & SHDT_DRAWTEXT)
                    hbrUse = g_hbrHighlight;
            } 
            else 
            {
                clrText = g_clrBtnText;
                clrTextBk = g_clrBtnFace;
                if (flags & SHDT_DRAWTEXT)
                    hbrUse = g_hbrBtnFace;
            }

#ifdef LVDEBUG
            if (GetAsyncKeyState(VK_CONTROL) < 0)
                clrText = g_clrBtnHighlight;
#endif
        }
        else if (clrText == CLR_DEFAULT && clrTextBk == CLR_DEFAULT)
        {
            clrText = g_clrWindowText;
            clrTextBk = g_clrWindow;

            if ( ( flags & (SHDT_DRAWTEXT | SHDT_DESELECTED) ) ==
               (SHDT_DRAWTEXT | SHDT_DESELECTED) )
            {
                hbrUse = g_hbrWindow;
            }
        }
        else
        {
            if (clrText == CLR_DEFAULT)
                clrText =  g_clrWindowText;

            if (clrTextBk == CLR_DEFAULT)
                clrTextBk = g_clrWindow;

            if ( ( flags & (SHDT_DRAWTEXT | SHDT_DESELECTED) ) ==
               (SHDT_DRAWTEXT | SHDT_DESELECTED) )
            {
                hbrUse = CreateSolidBrush(GetNearestColor(hdc, clrTextBk));
                if (hbrUse)
                {
                    hbrDelete = hbrUse;
                }
                else
                    hbrUse = GetStockObject( WHITE_BRUSH );
            }
        }

        // now set it
        clrSave = SetTextColor(hdc, clrText);
        clrSaveBk = SetBkColor(hdc, clrTextBk);
        if (hbrUse) {
            FillRect(hdc, prc, hbrUse);
            if (hbrDelete)
                DeleteObject(hbrDelete);
        }
    }

    // If we want the item to display as if it was depressed, we will
    // offset the text rectangle down and to the left
    if (flags & SHDT_DEPRESSED)
        OffsetRect(&rc, g_cxBorder, g_cyBorder);

    if (flags & SHDT_DRAWTEXT)
    {
        UINT uDTFlags = DT_LVWRAP | DT_END_ELLIPSIS;

        if (flags & SHDT_DTELLIPSIS)
            uDTFlags |= DT_WORD_ELLIPSIS;

        if ( !( flags & SHDT_CLIPPED ) )
            uDTFlags |= DT_NOCLIP;

        if (flags & SHDT_NODBCSBREAK)
            uDTFlags |= DT_NOFULLWIDTHCHARBREAK;

        DrawText(hdc, pszText, cchText, &rc, uDTFlags );
    }
    else
    {
        if (fmt != LVCFMT_LEFT)
        {
            SIZE siz;

            GetTextExtentPoint(hdc, pszText, cchText, &siz);

            if (fmt == LVCFMT_CENTER)
                rc.left = (rc.left + rc.right - siz.cx) / 2;
            else    // fmt == LVCFMT_RIGHT
                rc.left = rc.right - siz.cx;
        }

        // Center vertically in case the bitmap (to the left) is larger than
        // the height of one line
        rc.top += (rc.bottom - rc.top - cyChar) / 2;

        if ( flags & SHDT_CLIPPED )
           uETOFlags |= ETO_CLIPPED;

        ExtTextOut(hdc, rc.left, rc.top, uETOFlags, prc, pszText, cchText, NULL);
    }

    if (flags & (SHDT_SELECTED | SHDT_DESELECTED | SHDT_TRANSPARENT))
    {
        SetTextColor(hdc, clrSave);
        if (!fForeOnly)
            SetBkColor(hdc, clrSaveBk);
    }

#ifdef WINDOWS_ME
    if (flags & SHDT_RTLREADING)
        SetTextAlign(hdc, align);
#endif
}

/*----------------------------------------------------------------
** Create an imagelist to be used for dragging.
**
** 1) create mask and image bitmap matching the select bounds size
** 2) draw the text to both bitmaps (in black for now)
** 3) create an imagelist with these bitmaps
** 4) make a dithered copy of the image onto the new imagelist
**----------------------------------------------------------------*/
HIMAGELIST NEAR ListView_OnCreateDragImage(LV *plv, int iItem, LPPOINT lpptUpLeft)
{
    HWND hwndLV = plv->ci.hwnd;
    RECT rcBounds, rcImage, rcLabel;
    HDC hdcMem = NULL;
    HBITMAP hbmImage = NULL;
    HBITMAP hbmMask = NULL;
    HBITMAP hbmOld;
    HIMAGELIST himl = NULL;
    int dx, dy;
    HIMAGELIST himlSrc;
    LV_ITEM item;
    POINT ptOrg;
    LVDRAWITEM lvdi;
    RECT rcSelBounds;
    BOOL bMirroredWnd = (plv->ci.dwExStyle&RTL_MIRRORED_WINDOW);

    if (!lpptUpLeft)
        return NULL;

    if (iItem >= ListView_Count(plv))
        return NULL;

    if (plv->iHot == iItem) {
        ListView_OnSetHotItem(plv, -1);
        UpdateWindow(plv->ci.hwnd);
    }

    ListView_GetRects(plv, iItem, &rcImage, &rcLabel, &rcBounds, &rcSelBounds);

    if (ListView_IsIconView(plv)) {
        ListView_UnfoldRects(plv, iItem, &rcImage, &rcLabel,
                                         &rcBounds, &rcSelBounds);
        InflateRect(&rcImage, -g_cxIconMargin, -g_cyIconMargin);
    }

    // chop off any extra filler above icon
    ptOrg.x = rcBounds.left - rcSelBounds.left;
    ptOrg.y = rcBounds.top - rcImage.top;
    dx = rcSelBounds.right - rcSelBounds.left;
    dy = rcSelBounds.bottom - rcImage.top;

    lpptUpLeft->x = rcSelBounds.left;
    lpptUpLeft->y = rcImage.top;

    if (!(hdcMem = CreateCompatibleDC(NULL)))
        goto CDI_Exit;
    if (!(hbmImage = CreateColorBitmap(dx, dy)))
        goto CDI_Exit;
    if (!(hbmMask = CreateMonoBitmap(dx, dy)))
        goto CDI_Exit;

    //
    // Mirror the memory DC so that the transition from
    // mirrored(memDC)->non-mirrored(imagelist DCs)->mirrored(screenDC)
    // is consistent. [samera]
    //
    if (bMirroredWnd)
    {
        SET_DC_RTL_MIRRORED(hdcMem);
    }

    // prepare for drawing the item
    SelectObject(hdcMem, plv->hfontLabel);
    SetBkMode(hdcMem, TRANSPARENT);

    lvdi.plv = plv;
    lvdi.nmcd.nmcd.dwItemSpec = iItem;
    lvdi.pitem = NULL;  // make sure it is null as Owner data uses this to trigger things...
    lvdi.nmcd.nmcd.hdc = hdcMem;
    lvdi.lpptOrg = &ptOrg;
    lvdi.prcClip = NULL;
    lvdi.flags = LVDI_NOIMAGE | LVDI_TRANSTEXT | LVDI_NOWAYFOCUS | LVDI_UNFOLDED;
    /*
    ** draw the text to both bitmaps
    */
    hbmOld = SelectObject(hdcMem, hbmImage);
    // fill image with black for transparency
    PatBlt(hdcMem, 0, 0, dx, dy, BLACKNESS);
    ListView_DrawItem(&lvdi);
    if (bMirroredWnd)
        MirrorBitmapInDC(hdcMem, hbmImage);

    lvdi.flags = LVDI_NOIMAGE | LVDI_TRANSTEXT | LVDI_NOWAYFOCUS | LVDI_UNFOLDED;
    SelectObject(hdcMem, hbmMask);
    // fill mask with white for transparency
    PatBlt(hdcMem, 0, 0, dx, dy, WHITENESS);
    ListView_DrawItem(&lvdi);
    if (bMirroredWnd)
        MirrorBitmapInDC(hdcMem, hbmMask);

    // unselect objects that we used
    SelectObject(hdcMem, hbmOld);
    SelectObject(hdcMem, g_hfontSystem);

    himlSrc = ListView_OnGetImageList(plv, !(ListView_IsIconView(plv)));

    /*
    ** make an image list that for now only has the text
    ** we use ImageList_Clone so we get a imagelist that
    ** the same color depth as our own imagelist
    */
    if (!(himl = ImageList_Clone(himlSrc, dx, dy, ILC_MASK, 1, 0)))
        goto CDI_Exit;

    ImageList_SetBkColor(himl, CLR_NONE);
    ImageList_Add(himl, hbmImage, hbmMask);

    /*
    ** make a dithered copy of the image part onto our bitmaps
    ** (need both bitmap and mask to be dithered)
    */
    if (himlSrc)
    {
        item.iItem = iItem;
        item.iSubItem = 0;
        item.mask = LVIF_IMAGE |LVIF_STATE;
        item.stateMask = LVIS_OVERLAYMASK;
        ListView_OnGetItem(plv, &item);

        ImageList_CopyDitherImage(himl, 0, rcImage.left - rcSelBounds.left, 0, himlSrc, item.iImage, ((plv->ci.dwExStyle & dwExStyleRTLMirrorWnd) ? ILD_MIRROR : 0L) | (item.state & LVIS_OVERLAYMASK) );
    }

CDI_Exit:
    if (hdcMem)
        DeleteObject(hdcMem);
    if (hbmImage)
        DeleteObject(hbmImage);
    if (hbmMask)
        DeleteObject(hbmMask);

    return himl;
}


//-------------------------------------------------------------------
// ListView_OnGetTopIndex -- Gets the index of the first visible item
// For list view and report view this calculates the actual index
// for iconic views it alway returns 0
//
int NEAR ListView_OnGetTopIndex(LV* plv)
{
    if (ListView_IsReportView(plv))
        return  (int)((plv->ptlRptOrigin.y) / plv->cyItem);

    else if (ListView_IsListView(plv))
        return  (plv->xOrigin / plv->cxItem) * plv->cItemCol;

    else
        return(0);
}




//-------------------------------------------------------------------
// ListView_OnGetCountPerPage -- Gets the count of items that will fit
// on a page For list view and report view this calculates the
// count depending on the size of the window and for Iconic views it
// will always return the count of items in the list view.
//
int NEAR ListView_OnGetCountPerPage(LV* plv)
{
    if (ListView_IsReportView(plv))
        return (plv->sizeClient.cy - plv->yTop) / plv->cyItem;

    else if (ListView_IsListView(plv))
        return ((plv->sizeClient.cx)/ plv->cxItem)
                * plv->cItemCol;
    else
        return (ListView_Count(plv));
}


/*----------------------------------------------------------------------------
/ ListView_InvalidateFoldedItem implementation
/ -----------------------------
/ Purpose:
/   Provides support for invalidating items within list views.
/
/ Notes:
/   Copes with invalidating the extra region in the list view that requires
/   us to erase the background.  Design to optimise out the ERASURE of the
/   background.
/
/   For details on the API see ListView_InvalidateItem.
/
/ In:
/   plv->ListView structure to work with
/   iItem = item number
/   bSrelectionOnly = refesh the selection
/   fRedraw = Flags for RedrawWindow
/ Out:
/   -
/----------------------------------------------------------------------------*/
void NEAR ListView_InvalidateFoldedItem(LV* plv, int iItem, BOOL fSelectionOnly, UINT fRedraw)
{
    ListView_InvalidateItem( plv, iItem, fSelectionOnly, fRedraw );

    if ( ListView_IsIconView(plv) &&
        ( !ListView_IsItemUnfolded(plv, iItem) || (fRedraw & RDW_ERASE) ) )
    {
        RECT rcLabel;

        if (ListView_GetUnfoldedRect(plv, iItem, &rcLabel))
        {
            RedrawWindow(plv->ci.hwnd, &rcLabel, NULL, fRedraw|RDW_ERASE);
        }
    }
}


/*----------------------------------------------------------------------------
/ ListView_UnfoldedRects implementation
/ ----------------------
/ Purpose:
/   Having previously called get rects, then call this function to ensure
/   that they are correctly unfolded.
/
/ Notes:
/   -
/
/ In:
/   plv-> list view to unfold on
/   iItem = item number
/   prcIcon -> icon bounding box
/   prcLabel -> rectangle for the label structure
/   prcBounds -> bounds rectangle / == NULL for none    / These are currently the same for large icons
/   prcSelectBounds -> selection bounds / == NULL       /
/ Out: TRUE if unfolding the item was worth anything
/   -
/----------------------------------------------------------------------------*/
BOOL NEAR ListView_UnfoldRects(LV* plv, int iItem,
                               RECT * prcIcon, RECT * prcLabel,
                               RECT * prcBounds, RECT * prcSelectBounds)
{
    LISTITEM item;
    LISTITEM FAR* pitem = &item;
    BOOL fRc = FALSE;

    if (!ListView_IsIconView(plv))
        return fRc;

    // If we have a label pointer then expand as required
    // nb - different paths for owner data

    if ( prcLabel )
    {
        if ( !ListView_IsOwnerData(plv) )
        {
            pitem = ListView_GetItemPtr(plv, iItem);
            if (!EVAL(pitem)) {
                // DavidShi was able to get us into here with an invalid
                // item number during a delete notification.  So if the
                // item number is invalid, just return a blank rectangle
                // instead of faulting.
                SetRectEmpty(prcLabel);
                goto doneLabel;
            }
        }
        else
        {
            ListView_RecomputeLabelSize( plv, pitem, iItem, NULL, FALSE );
        }

        if (prcLabel->bottom != prcLabel->top + max(pitem->cyUnfoldedLabel, pitem->cyFoldedLabel))
            fRc = TRUE;

        prcLabel->bottom = prcLabel->top + pitem->cyUnfoldedLabel;

    }
doneLabel:

    // Build the unions if required

    if ( prcBounds && prcIcon && prcLabel )
    {
        UnionRect( prcBounds, prcIcon, prcLabel );
    }
    if ( prcSelectBounds && prcIcon && prcLabel )
    {
        UnionRect( prcSelectBounds, prcIcon, prcLabel );
    }

    return fRc;
}
