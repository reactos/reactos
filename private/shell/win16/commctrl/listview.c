#include "ctlspriv.h"
#include "listview.h"
#include "image.h"

#ifndef LVS_SHOWSELALWAYS
#define LVS_SHOWSELALWAYS 0x008
#endif

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

#ifndef IEWIN31_25
// Moved to notify.c
LRESULT WINAPI SendNotify(HWND hwndTo, HWND hwndFrom, int code, NMHDR FAR* pnmhdr)
{
    NMHDR nmhdr;
    int id;

    id = hwndFrom ? GetDlgCtrlID(hwndFrom) : 0;

    if (!pnmhdr)
        pnmhdr = &nmhdr;

    pnmhdr->hwndFrom = hwndFrom;
    pnmhdr->idFrom = id;
    pnmhdr->code = code;

    return(SendMessage(hwndTo, WM_NOTIFY, (WPARAM)id, (LPARAM)pnmhdr));
}
#endif //IEWIN31_25

//#ifndef WIN31   // we only want SendNotify for prop-sheets
#if !defined( WIN31 ) || defined( IEWIN31_25 )

#pragma code_seg(CODESEG_INIT)

BOOL FAR ListView_Init(HINSTANCE hinst)
{
    WNDCLASS wc;

    if (!GetClassInfo(hinst, c_szListViewClass, &wc)) {
#ifndef WIN32
#ifndef IEWIN31
    //
    // Use stab WndProc to avoid loading segment on init.
    //
    LRESULT CALLBACK _ListView_WndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
        wc.lpfnWndProc     = _ListView_WndProc;
#else
        wc.lpfnWndProc     = ListView_WndProc;
#endif
#else
        wc.lpfnWndProc     = ListViewWndProc;
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

BOOL NEAR ListView_SendChange(LV* plv, int i, int iSubItem, int code, UINT oldState, UINT newState,
                              UINT changed, int x, int y, LPARAM lParam)
{
    NM_LISTVIEW nm;

    nm.iItem = i;
    nm.iSubItem = iSubItem;
    nm.uNewState = newState;
    nm.uOldState = oldState;
    nm.uChanged = changed;
    nm.ptAction.x = x;
    nm.ptAction.y = y;
    nm.lParam = lParam;

    return !(BOOL)SendNotify(plv->hwndParent, plv->hwnd, code, &nm.hdr);
}

BOOL NEAR ListView_Notify(LV* plv, int i, int iSubItem, int code)
{
    NM_LISTVIEW nm;
    nm.iItem = i;
    nm.iSubItem = iSubItem;
    nm.uNewState = nm.uOldState = 0;
    nm.uChanged = 0;
    nm.lParam = 0;

    if (code == LVN_DELETEITEM) {
        LISTITEM FAR * pItem = ListView_GetItemPtr(plv, i);
        if (pItem) {
            nm.lParam = pItem->lParam;
        }
    }

    return (BOOL)SendNotify(plv->hwndParent, plv->hwnd, code, &nm.hdr);
}

int NEAR ListView_OnSetItemCount(LV *plv, int iItems)
{
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
    return 0;
}

typedef struct _LVSortInfo
{
    LV*     plv;
    BOOL fSortIndices;
    PFNLVCOMPARE    pfnCompare;
    LPARAM      lParam;
} LVSortInfo;

int CALLBACK ListView_SortCallback(LPVOID dw1, LPVOID dw2, LPARAM lParam)
{
    LISTITEM FAR *pitem1;
    LISTITEM FAR *pitem2;
    LVSortInfo FAR *pSortInfo = (LVSortInfo FAR *)lParam;

    // determine whether  dw1 and dw2 are indices or the real items
    // and assign pitem? accordingly
    if (pSortInfo->fSortIndices) {
        pitem1 = ListView_GetItemPtr(pSortInfo->plv, (UINT)(DWORD)dw1);
        pitem2 = ListView_GetItemPtr(pSortInfo->plv, (UINT)(DWORD)dw2);
    } else {
        pitem1 = (LISTITEM FAR *)dw1;
        pitem2 = (LISTITEM FAR *)dw2;
    }

    if (!pSortInfo->pfnCompare) {
        // bugbug, should allow callbacks in text
        if (pitem1->pszText && (pitem1->pszText != LPSTR_TEXTCALLBACK) &&
            pitem2->pszText && (pitem2->pszText != LPSTR_TEXTCALLBACK) )
        {
            return lstrcmpi(pitem1->pszText, pitem2->pszText);
        }
    } else {
    return(pSortInfo->pfnCompare(pitem1->lParam, pitem2->lParam, pSortInfo->lParam));
    }
    Assert(0);
    return -1;
}

BOOL NEAR PASCAL ListView_SortAllColumns(LV* plv, LVSortInfo FAR * psi)
{
    if ((!plv->hdpaSubItems) || !DPA_GetPtrCount(plv->hdpaSubItems)) {
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
            Assert(DPA_GetPtrCount(plv->hdpa) == DPA_GetPtrCount(hdpa));
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
                                ph[j] = pSubItems[(UINT)(DWORD)pNewIndices[j]];
                            }

                            // finally, copy it all back to the pSubItems;
                            hmemcpy(pSubItems, ph, sizeof(LPVOID) * iMax);
                        }
                    }

                    // now do the main hdpa
                    pSubItems = DPA_GetPtrPtr(plv->hdpa);
                    for (j = 0; j < iMax; j++) {
                        ph[j] = pSubItems[(int)(DWORD)pNewIndices[j]];
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

BOOL NEAR PASCAL ListView_OnSortItems(LV *plv, LPARAM lParam, PFNLVCOMPARE pfnCompare)
{
    LVSortInfo SortInfo;
    LISTITEM FAR *pitemFocused;
    SortInfo.pfnCompare = pfnCompare;
    SortInfo.lParam     = lParam;
    SortInfo.plv = plv;

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
            InvalidateRect(plv->hwnd, NULL, TRUE);
        }

        return(TRUE);
    }
    return FALSE;
}

void PASCAL ListView_EnableWindow(LV* plv, BOOL wParam)
{
    if (wParam) {
        if (plv->style & WS_DISABLED) {
            plv->style &= ~WS_DISABLED; // enabled
            ListView_OnSetBkColor(plv, plv->clrBkSave);
        }
    } else {
        if (!(plv->style & WS_DISABLED)) {
            plv->clrBkSave = plv->clrBk;
            plv->style |= WS_DISABLED;  // disabled
            ListView_OnSetBkColor(plv, g_clrBtnFace);
        }
    }
    RedrawWindow(plv->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}

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

#ifndef IEWIN31_25
LRESULT NEAR PASCAL ListView_OnHelp(LV* plv, LPHELPINFO lpHelpInfo)
{

    //  If we're seeing WM_HELP because of our child header control, then
    //  munge the HELPINFO structure to use the ListView's control id.
    //  win\core\user\combo.c has similiar code to handle the child edit
    //  control of a combo box.
    if ((lpHelpInfo != NULL) && ((plv->style & LVS_TYPEMASK) == LVS_REPORT) &&
        (lpHelpInfo->iCtrlId == LVID_HEADER)) {

        lpHelpInfo->hItemHandle = plv->hwnd;
        lpHelpInfo->iCtrlId = GetWindowID(plv->hwnd);
        //  Shouldn't have to do this: USER would have filled in the appropriate
        //  context id by walking up the parent hwnd chain.
        //lpHelpInfo->dwContextId = GetContextHelpId(hwnd);

    }

    return DefWindowProc(plv->hwnd, WM_HELP, 0, (LPARAM)lpHelpInfo);

}
#endif

LRESULT CALLBACK ListView_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LV* plv = ListView_GetPtr(hwnd);

    if (plv == NULL)
    {
        if (msg == WM_NCCREATE)
        {
            plv = (LV*)NearAlloc(sizeof(LV));
            if (!plv)
            {
                DebugMsg(DM_ERROR, "ListView: Out of near memory");
                return 0L;  // fail the window create
            }

            plv->hwnd = hwnd;
            plv->flags = LVF_REDRAW;    // assume that redrawing enabled!
            if (GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE)
                plv->flags |= LVF_VISIBLE;
        plv->iFocus = -1;       // no focus
            plv->iMark = -1;
            plv->iSelCol = -1;
#ifdef WIN32
            plv->hheap = GetProcessHeap();
#else
            // plv->hheap = NULL;  // not used in 16 bits...
#endif
            ListView_SetPtr(hwnd, plv);
        }
        else
        goto DoDefault;
    }

    if (msg == WM_NCDESTROY)
    {
        LRESULT result = HANDLE_WM_NCDESTROY(plv, wParam, lParam, ListView_OnNCDestroy);

        NearFree(plv);
        ListView_SetPtr(hwnd, NULL);

        return result;
    }

    switch (msg)
    {
        HANDLE_MSG(plv, WM_CREATE, ListView_OnCreate);
        HANDLE_MSG(plv, WM_DESTROY, ListView_OnDestroy);
        HANDLE_MSG(plv, WM_ERASEBKGND, ListView_OnEraseBkgnd);
        HANDLE_MSG(plv, WM_COMMAND, ListView_OnCommand);
        HANDLE_MSG(plv, WM_SETFOCUS, ListView_OnSetFocus);
        HANDLE_MSG(plv, WM_KILLFOCUS, ListView_OnKillFocus);
        HANDLE_MSG(plv, WM_LBUTTONDOWN, ListView_OnButtonDown);
        HANDLE_MSG(plv, WM_RBUTTONDOWN, ListView_OnButtonDown);
        HANDLE_MSG(plv, WM_LBUTTONDBLCLK, ListView_OnButtonDown);
        HANDLE_MSG(plv, WM_RBUTTONDBLCLK, ListView_OnButtonDown);

        HANDLE_MSG(plv, WM_HSCROLL, ListView_OnHScroll);
        HANDLE_MSG(plv, WM_VSCROLL, ListView_OnVScroll);
        HANDLE_MSG(plv, WM_GETDLGCODE, ListView_OnGetDlgCode);
        HANDLE_MSG(plv, WM_SETFONT, ListView_OnSetFont);
        HANDLE_MSG(plv, WM_GETFONT, ListView_OnGetFont);
        HANDLE_MSG(plv, WM_NOTIFY, ListView_ROnNotify);
        HANDLE_MSG(plv, WM_TIMER, ListView_OnTimer);
        HANDLE_MSG(plv, WM_SETREDRAW, ListView_OnSetRedraw);

    case WM_WINDOWPOSCHANGED:
        HANDLE_WM_WINDOWPOSCHANGED(plv, wParam, lParam, ListView_OnWindowPosChanged);
        break;

    case WM_MBUTTONDOWN:
        SetFocus(hwnd);
        break;

    case WM_PRINTCLIENT:
    case WM_PAINT:
        ListView_OnPaint(plv, (HDC)wParam);
        return(0);

    case WM_SHOWWINDOW:
        LV_OnShowWindow(plv, wParam);
        break;

    case WM_KEYDOWN:
        HANDLE_WM_KEYDOWN(plv, wParam, lParam, ListView_OnKey);
        break;

#ifdef  FE_IME
    case WM_IME_COMPOSITION:
        // Now only Korean version is interested in incremental search with composition string.
        if (((DWORD)GetKeyboardLayout(0L) & 0xF000FFFFL) == 0xE0000412L)
        {
            if (ListView_OnImeComposition(plv, wParam, lParam))
            {
                lParam &= ~GCS_RESULTSTR;
                break;
            }
            else
                return 0;
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
        ListView_OnWinIniChange(plv, wParam);
    break;

    case WM_ENABLE:
        // HACK: we don't get WM_STYLECHANGE on EnableWindow()
        ListView_EnableWindow(plv, wParam);
        break;

    case WM_SYSCOLORCHANGE:
        ReInitGlobalColors();
        if (plv->style & WS_DISABLED) {
            if (!(plv->flags & LVF_USERBKCLR))
                plv->clrBkSave = g_clrWindow;
            ListView_OnSetBkColor(plv, g_clrBtnFace);
            goto DoInvalidation;
        }
        else if (!(plv->flags & LVF_USERBKCLR)) {
            ListView_OnSetBkColor(plv, g_clrWindow);
DoInvalidation:
            InvalidateRect(plv->hwnd, NULL, TRUE);
        }
        break;

        // don't use HANDLE_MSG because this needs to go to the default handler
    case WM_SYSKEYDOWN:
        HANDLE_WM_SYSKEYDOWN(plv, wParam, lParam, ListView_OnKey);
        break;

    case WM_STYLECHANGED:
        ListView_OnStyleChanged(plv, wParam, (LPSTYLESTRUCT)lParam);
        return 0L;

#ifndef IEWIN31_25
    case WM_HELP:
        return ListView_OnHelp(plv, (LPHELPINFO)lParam);
#endif

    case LVM_GETIMAGELIST:
        return (LRESULT)(UINT)(ListView_OnGetImageList(plv, (int)wParam));

    case LVM_SETIMAGELIST:
        return (LRESULT)(UINT)ListView_OnSetImageList(plv, (HIMAGELIST)lParam, (int)wParam);

    case LVM_GETBKCOLOR:
        return (LRESULT)(plv->style & WS_DISABLED ? plv->clrBkSave : plv->clrBk);

    case LVM_SETBKCOLOR:
        plv->flags |= LVF_USERBKCLR;
        if (plv->style & WS_DISABLED) {
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

    case LVM_GETITEMCOUNT:
        return (LRESULT)ListView_Count(plv);

    case LVM_GETITEM:
        return (LRESULT)ListView_OnGetItem(plv, (LV_ITEM FAR*)lParam);

    case LVM_GETITEMSTATE:
        return (LRESULT)ListView_OnGetItemState(plv, (int)wParam, (UINT)lParam);

    case LVM_SETITEMSTATE:
        return (LRESULT)ListView_OnSetItemState(plv, (int)wParam,
                                                ((LV_ITEM FAR *)lParam)->state,
                                                ((LV_ITEM FAR *)lParam)->stateMask);

    case LVM_SETITEMTEXT:
        return (LRESULT)ListView_OnSetItemText(plv, (int)wParam,
                                                ((LV_ITEM FAR *)lParam)->iSubItem,
                                                (LPCSTR)((LV_ITEM FAR *)lParam)->pszText);

    case LVM_GETITEMTEXT:
        return (LRESULT)ListView_OnGetItemText(plv, (int)wParam, (LV_ITEM FAR *)lParam);

    case LVM_SETITEM:
        return (LRESULT)ListView_OnSetItem(plv, (const LV_ITEM FAR*)lParam);

    case LVM_INSERTITEM:
        return (LRESULT)ListView_OnInsertItem(plv, (const LV_ITEM FAR*)lParam);

    case LVM_DELETEITEM:
        return (LRESULT)ListView_OnDeleteItem(plv, (int)wParam);

    case LVM_UPDATE:
        ListView_OnUpdate(plv, (int)wParam);
        return TRUE;

    case LVM_DELETEALLITEMS:
        return (LRESULT)ListView_OnDeleteAllItems(plv);

    case LVM_GETITEMRECT:
        return (LRESULT)ListView_OnGetItemRect(plv, (int)wParam, (RECT FAR*)lParam);

    case LVM_GETISEARCHSTRING:
        if (GetFocus() == plv->hwnd)
            return (LRESULT)GetIncrementSearchString((LPSTR)lParam);
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

    case LVM_GETITEMPOSITION:
        return (LRESULT)ListView_OnGetItemPosition(plv, (int)wParam,
                (POINT FAR*)lParam);

    case LVM_SETITEMPOSITION:
        return (LRESULT)ListView_OnSetItemPosition(plv, (int)wParam,
                (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

    case LVM_SETITEMPOSITION32:
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
        return (LRESULT)ListView_OnEnsureVisible(plv, (int)wParam, (BOOL)lParam);

    case LVM_REDRAWITEMS:
        return (LRESULT)ListView_OnRedrawItems(plv, (int)wParam, (int)lParam);

    case LVM_ARRANGE:
        return (LRESULT)ListView_OnArrange(plv, (UINT)wParam);

    case LVM_GETEDITCONTROL:
        return (LRESULT)(UINT)plv->hwndEdit;

    case LVM_EDITLABEL:
        return (LRESULT)(UINT)ListView_OnEditLabel(plv, (int)wParam, (LPSTR)lParam);

    case LVM_HITTEST:
        return (LRESULT)ListView_OnHitTest(plv, (LV_HITTESTINFO FAR*)lParam);

    case LVM_GETSTRINGWIDTH:
        return (LRESULT)ListView_OnGetStringWidth(plv, (LPCSTR)lParam);

    case LVM_GETCOLUMN:
        return (LRESULT)ListView_OnGetColumn(plv, (int)wParam, (LV_COLUMN FAR*)lParam);

    case LVM_SETCOLUMN:
        return (LRESULT)ListView_OnSetColumn(plv, (int)wParam, (const LV_COLUMN FAR*)lParam);

    case LVM_INSERTCOLUMN:
        return (LRESULT)ListView_OnInsertColumn(plv, (int)wParam, (const LV_COLUMN FAR*)lParam);

    case LVM_DELETECOLUMN:
        return (LRESULT)ListView_OnDeleteColumn(plv, (int)wParam);

    case LVM_CREATEDRAGIMAGE:
        return (LRESULT)(UINT)ListView_OnCreateDragImage(plv, (int)wParam, (LPPOINT)lParam);

    case LVM_GETVIEWRECT:
        ListView_GetViewRect2(plv, (RECT FAR*)lParam, plv->sizeClient.cx, plv->sizeClient.cy);
        return (LPARAM)TRUE;

    case LVM_GETCOLUMNWIDTH:
        return (LPARAM)ListView_OnGetColumnWidth(plv, (int)wParam);

    case LVM_SETCOLUMNWIDTH:
        return (LPARAM)ListView_ISetColumnWidth(plv, (int)wParam, (int)(short)LOWORD(lParam), TRUE);

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
    return ListView_OnSetItemCount(plv, (int)wParam);

    case LVM_GETSELECTEDCOUNT:
        return plv->nSelected;

    case LVM_SORTITEMS:
    return ListView_OnSortItems(plv, (LPARAM)wParam, (PFNLVCOMPARE)lParam);

    default:
        break;
    }

DoDefault:
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void NEAR ListView_OnWinIniChange(LV* plv, WPARAM wParam)
{
    // BUGBUG:  will this also catch sysparametersinfo?
    // we need a general way of handling this, not
    // just relying on the listview.
    InitGlobalMetrics(wParam);

    if (!wParam ||
        (wParam == SPI_SETNONCLIENTMETRICS) ||
        (wParam == SPI_SETICONTITLELOGFONT)) {

        if (plv->flags & LVF_FONTCREATED)
            ListView_OnSetFont(plv, NULL, TRUE);

    }

    // If we are in an Iconic view and the user is in autoarrange mode,
    // then we need to arrange the items.
    //
    if ((plv->style & LVS_AUTOARRANGE) &&
            (ListView_IsSmallView(plv) || ListView_IsIconView(plv)))
    {
        // Call off to the arrange function.
        ListView_OnArrange(plv, LVA_DEFAULT);
    }
}

BOOL NEAR ListView_OnCreate(LV* plv, CREATESTRUCT FAR* lpCreateStruct)
{
    plv->hwndParent = lpCreateStruct->hwndParent;
    plv->style = lpCreateStruct->style;

    plv->hdpa = DPA_CreateEx(LV_HDPA_GROW, plv->hheap);
    if (!plv->hdpa)
    goto error0;

    plv->hdpaZOrder = DPA_CreateEx(LV_HDPA_GROW, plv->hheap);
    if (!plv->hdpaZOrder)
    goto error1;

    // start out NULL -- if someone wants them, do LVM_SETIMAGELIST
    plv->himl = plv->himlSmall = NULL;

    //plv->hwndEdit = NULL;
    plv->iEdit = -1;
    plv->iFocus = -1;
    plv->iDrag = -1;
    plv->rcView.left = RECOMPUTE;
    plv->sizeClient.cx = lpCreateStruct->cx;
    plv->sizeClient.cy = lpCreateStruct->cy;

    //plv->ptOrigin.x = 0;
    //plv->ptOrigin.y = 0;

    // Setup flag to say if positions are in small or large view
    if (ListView_IsSmallView(plv))
        plv->flags |= LVF_ICONPOSSML;

    // force calculation of listview metrics
    ListView_OnSetFont(plv, NULL, FALSE);

    //plv->xOrigin = 0;
    plv->cxItem = 16 * plv->cxLabelChar + plv->cxSmIcon;

    // if we're in ownerdraw report mode, the size got saved to cyItemSave
    // at creation time, both need to have this
    if ((plv->style & LVS_OWNERDRAWFIXED) && ListView_IsReportView(plv))
        plv->cyItem = plv->cyItemSave;
    else
        plv->cyItemSave = plv->cyItem;


    ListView_UpdateScrollBars(plv);     // sets plv->cItemCol

    //plv->hbrBk = NULL;
    plv->clrBk = CLR_NONE;

    plv->clrText = CLR_DEFAULT;
    plv->clrTextBk = CLR_DEFAULT;

    // create the bk brush, and set the imagelists colors if needed
    ListView_OnSetBkColor(plv, g_clrWindow);

    // Initialize report view fields
    //plv->yTop = 0;
    //plv->ptlRptOrigin.x = 0;
    //plv->ptlRptOrigin.y = 0;
    //plv->hwndHdr = NULL;
    plv->xTotalColumnWidth = RECOMPUTE;

    if (ListView_IsReportView(plv))
        ListView_RInitialize(plv, FALSE);

    if (plv->style & WS_DISABLED) {
        plv->style &= ~WS_DISABLED;
        ListView_EnableWindow(plv, FALSE);
    }

    return TRUE;

error1:
    DPA_Destroy(plv->hdpa);
error0:
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
    // Make sure to notify the app
    ListView_OnDeleteAllItems(plv);

    if ((plv->flags & LVF_FONTCREATED) && plv->hfontLabel) {
        DeleteObject(plv->hfontLabel);
    // plv->flags &= ~LVF_FONTCREATED;
    // plv->hwfontLabel = NULL;
    }
    ListView_DeleteHrgnInval(plv);
}

void NEAR ListView_OnNCDestroy(LV* plv)
{
    if (!(plv->style & LVS_SHAREIMAGELISTS))
    {
        if (plv->himl)
            ImageList_Destroy(plv->himl);
        if (plv->himlSmall)
            ImageList_Destroy(plv->himlSmall);
        if (plv->himlState)
            ImageList_Destroy(plv->himlState);
    }

    if (plv->hbrBk)
        DeleteBrush(plv->hbrBk);

    if (plv->hdpa)
        DPA_Destroy(plv->hdpa);

    if (plv->hdpaZOrder)
        DPA_Destroy(plv->hdpaZOrder);

    ListView_RDestroy(plv);
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

        if (!(plv->style & LVS_SHAREIMAGELISTS)) {

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
#ifdef WIN32
    {
        POINT pt;
        SetBrushOrgEx(hdc, -x, 0, &pt);
    }
#else
    SetBrushOrg(hdc, -x, 0);
#endif

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
    if (hdc || GetUpdateRect(plv->hwnd, &rcUpdate, FALSE))
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
                GetUpdateRgn(plv->hwnd, hrgn, FALSE);
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
            hdc = BeginPaint(plv->hwnd, &ps);
            InitBrushOrg(plv, hdc);
            if (ListView_RedrawEnabled(plv))
                ListView_Redraw(plv, hdc, &ps.rcPaint);

            EndPaint(plv->hwnd, &ps);
        }
    }
}


BOOL NEAR ListView_OnEraseBkgnd(LV* plv, HDC hdc)
{
    if (plv->clrBk != CLR_NONE)
    {
        //
        // If we have a background color, erase with it.
        //

        RECT rc;

        GetClipBox(hdc, &rc);
        FillRect(hdc, &rc, plv->hbrBk);
    }
    else
    {
        //
        //  If not, pass it up to the parent.
        //

        SendMessage(plv->hwndParent, WM_ERASEBKGND, (UINT)hdc, 0);
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
#ifdef FE_IME
            // We don't want flicker during replacing current selection
            // as we use selection for IME composition.
            //
            if (plv->flags & LVF_INSERTINGCOMP)
                break;
#endif
            // We will use the ID of the window as a Dirty flag...
            SetWindowID(plv->hwndEdit, 1);
            ListView_SetEditSize(plv);
            break;

        case EN_KILLFOCUS:
            // We lost focus, so dismiss edit and do not commit changes
            // as if the validation fails and we attempt to display
            // an error message will cause the system to hang!
             if (!ListView_DismissEdit(plv, FALSE))
                return;
             break;

         case HN_BEGINDIALOG:  // pen windows is bringing up a dialog
             Assert(GetSystemMetrics(SM_PENWINDOWS)); // only on a pen system
             plv->fNoDismissEdit = TRUE;
             break;

         case HN_ENDDIALOG: // pen windows has destroyed dialog
             Assert(GetSystemMetrics(SM_PENWINDOWS)); // only on a pen system
             plv->fNoDismissEdit = FALSE;
             break;
        }

        // Forward edit control notifications up to parent
        //
        if (IsWindow(hwndCtl))
            FORWARD_WM_COMMAND(plv->hwndParent, id, hwndCtl, codeNotify, SendMessage);
    }
}

void NEAR ListView_OnWindowPosChanged(LV* plv, const WINDOWPOS FAR* lpwpos)
{
    if (!(lpwpos->flags & SWP_NOSIZE))
    {
        RECT rc;
        GetClientRect(plv->hwnd, &rc);
        plv->sizeClient.cx = rc.right;
        plv->sizeClient.cy = rc.bottom;

        if ((plv->style & LVS_AUTOARRANGE) &&
                (ListView_IsSmallView(plv) || ListView_IsIconView(plv)))
        {
            // Call off to the arrange function.
            ListView_OnArrange(plv, LVA_DEFAULT);
        }

        ListView_RInitialize(plv, TRUE);

        // Always make sure the scrollbars are updated to the new size
        ListView_UpdateScrollBars(plv);
    }
}

void NEAR ListView_RedrawSelection(LV* plv)
{
    int i = -1;

    while ((i = ListView_OnGetNextItem(plv, i, LVNI_SELECTED)) != -1) {
        ListView_InvalidateItem(plv, i, TRUE, RDW_INVALIDATE);
    }

    UpdateWindow( plv->hwnd );
}

void NEAR ListView_OnSetFocus(LV* plv, HWND hwndOldFocus)
{
    // due to the way listview call SetFocus on themselves on buttondown,
    // the window can get a strange sequence of focus messages: first
    // set, then kill, and then set again.  since these are not really
    // focus changes, ignore them and only handle "real" cases.
    if (hwndOldFocus == plv->hwnd)
    return;

    plv->flags |= LVF_FOCUSED;
    if (IsWindowVisible(plv->hwnd))
    {
        if (plv->iFocus != -1)
            ListView_InvalidateItem(plv, plv->iFocus, TRUE, RDW_INVALIDATE);
        ListView_RedrawSelection(plv);
    }

    // Let the parent window know that we are getting the focus.
    SendNotify(plv->hwndParent, plv->hwnd, NM_SETFOCUS, NULL);
}

void NEAR ListView_OnKillFocus(LV* plv, HWND hwndNewFocus)
{
    // due to the way listview call SetFocus on themselves on buttondown,
    // the window can get a strange sequence of focus messages: first
    // set, then kill, and then set again.  since these are not really
    // focus changes, ignore them and only handle "real" cases.
    if (!plv || hwndNewFocus == plv->hwnd)
    return;

    plv->flags &= ~LVF_FOCUSED;

    // Blow this off if we are not currently visible (being destroyed!)
    if (IsWindowVisible(plv->hwnd))
    {
        if (plv->iFocus != -1)
            ListView_InvalidateItem(plv, plv->iFocus, TRUE, RDW_INVALIDATE);
        if (!(plv->style & LVS_SHOWSELALWAYS))
            ListView_RedrawSelection(plv);
    }

    // Let the parent window know that we are losing the focus.
    SendNotify(plv->hwndParent, plv->hwnd, NM_KILLFOCUS, NULL);
    IncrementSearchString(0, NULL);
}

void NEAR ListView_DeselectAll(LV* plv, int iDontDeselect)
{
    int i;
    int nSkipped = 0;

    i = -1;

    if (iDontDeselect != plv->iFocus) {
        ListView_OnSetItemState(plv, plv->iFocus, 0, LVIS_SELECTED);
    }

    while ((plv->nSelected - nSkipped) && (i = ListView_OnGetNextItem(plv, i, LVNI_SELECTED)) != -1) {
        if (i != iDontDeselect) {
            ListView_OnSetItemState(plv, i, 0, LVIS_SELECTED);
        } else {
            if (ListView_OnGetItemState(plv, i, LVIS_SELECTED)) {
                nSkipped++;
            }
        }
    }

    Assert((plv->nSelected - nSkipped) == 0);
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
        iMax = ListView_Count(plv);

        for (i = 0; i < iMax; i++)
        {
            ListView_GetRects(plv, i, NULL, NULL, NULL, &rcTemp2);
            pt.x = (rcTemp2.right + rcTemp2.left) / 2;  // center of item
            pt.y = (rcTemp2.bottom + rcTemp2.top) / 2;

            if (PtInRect(&rcBounding, pt))
            {
                int iZ = ListView_ZOrderIndex(plv, i);
                if (iZ > 0)
                    DPA_InsertPtr(plv->hdpaZOrder, 0, DPA_DeletePtr(plv->hdpaZOrder, iZ));

                ListView_OnSetItemState(plv, i, uSelVal, LVIS_SELECTED);
            }
            else if (fResetRest)
                ListView_OnSetItemState(plv, i, 0, LVIS_SELECTED);
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
    UINT flags;

    if (plv->style & LVS_SINGLESEL) {
        // we know there's only one thing selected, so go deselect it.
        if (iItem != plv->iFocus) {
            ListView_OnSetItemState(plv, plv->iFocus, 0, LVIS_SELECTED);
        }
    } else if (fDeselectAll)
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

    if (fSelectAlso)
    {
        if (ListView_IsIconView(plv) || ListView_IsSmallView(plv))
        {
            int iZ = ListView_ZOrderIndex(plv, iItem);

            if (iZ > 0)
                DPA_InsertPtr(plv->hdpaZOrder, 0, DPA_DeletePtr(plv->hdpaZOrder, iZ));
        }
    }

    plv->iFocus = iItem;
    if (plv->iMark == -1)
        plv->iMark = iItem;

    SetTimer(plv->hwnd, IDT_SCROLLWAIT, GetDoubleClickTime(), NULL);
    plv->flags |= LVF_SCROLLWAIT;

    if (fToggleSel)
    {
        ListView_ToggleSelection(plv, iItem);
        ListView_OnSetItemState(plv, plv->iFocus, LVIS_FOCUSED, LVIS_FOCUSED);
    }
    else
    {
        flags = (fSelectAlso ? (LVIS_SELECTED | LVIS_FOCUSED) : LVIS_FOCUSED);
        ListView_OnSetItemState(plv, plv->iFocus, flags, flags);
    }

    return iItem;
}

void NEAR ListView_OnKey(LV* plv, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    UINT lvni = 0;
    int iNewFocus;
    BOOL fCtlDown;
    BOOL fShiftDown;
    LV_KEYDOWN nm;

    if (!fDown)
        return; 

    // Notify
    nm.wVKey = vk;
    nm.flags = flags;
    if (SendNotify(plv->hwndParent, plv->hwnd, LVN_KEYDOWN, &nm.hdr)) {
        plv->iPuntChar++;
        return;
    } else if (plv->iPuntChar) {
        // this is tricky...  if we want to punt the char, just increment the
        // count.  if we do NOT, then we must clear the queue of WM_CHAR's
        // this is to preserve the iPuntChar to mean "punt the next n WM_CHAR messages
        MSG msg;
        while(plv->iPuntChar && PeekMessage(&msg, plv->hwnd, WM_CHAR, WM_CHAR, PM_REMOVE)) {
            plv->iPuntChar--;
        }
        Assert(!plv->iPuntChar);
    }

    if (ListView_Count(plv) == 0)   // don't blow up on empty list
    return;

    fCtlDown = GetKeyState(VK_CONTROL) < 0;
    fShiftDown = GetKeyState(VK_SHIFT) < 0;

    switch (vk)
    {
    case VK_SPACE:
        // If shift (extend) or control (disjoint) select,
        // then toggle selection state of focused item.
        if (fCtlDown) {
            plv->iMark = plv->iFocus;
            ListView_ToggleSelection(plv, plv->iFocus);
            plv->iPuntChar++;
        }

        // BUGBUG: Implement me
        if ( fShiftDown) {
            ListView_SelectRangeTo(plv, plv->iFocus, TRUE);
        }
        return;
    case VK_RETURN:
        SendNotify(plv->hwndParent, plv->hwnd, NM_RETURN, NULL);
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
            return;
        }
    }

    if (GetKeyState(VK_MENU) < 0)
        return;

    // For a single selection listview, disable extending the selection
    // by turning off the keyboard modifiers.
    if (plv->style & LVS_SINGLESEL) {
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
        IncrementSearchString(0, NULL);
    }

    // on keyboard movement, scroll immediately.
    if (ListView_CancelScrollWait(plv)) {
        ListView_OnEnsureVisible(plv, plv->iFocus, FALSE);
        UpdateWindow(plv->hwnd);
    }
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

BOOL NEAR ListView_OnImeComposition(LV* plv, WPARAM wParam, LPARAM lParam)
{
    LPSTR lpsz;
    LV_FINDINFO lvfi;
    int i;
    int iStartFrom = -1;
    int iLen;
    int iCount;
    HIMC hImc;
    char *pszCompStr;
    BOOL fRet = TRUE;

    iCount = ListView_Count(plv);

    if (!iCount || plv->iFocus == -1)
        return fRet;

    if (hImc = ImmGetContext(plv->hwnd))
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
                if (IncrementSearchImeCompStr(TRUE, pszCompStr, &lpsz))
                    iStartFrom = plv->iFocus;
                else
                    iStartFrom = ((plv->iFocus - 1) + iCount)% iCount;

                lvfi.flags = LVFI_SUBSTRING | LVFI_STRING | LVFI_WRAP;
                lvfi.psz = lpsz;
                iLen = lstrlen(lpsz);

                // special case space as the first character
                if ((iLen == 1) && (*lpsz == ' ')) {
                    if (plv->iFocus != -1) {
                        ListView_OnSetItemState(plv, plv->iFocus, LVIS_SELECTED, LVIS_SELECTED);
                        IncrementSearchString(0, NULL);
                    }
                    return fRet;
                }

                i = ListView_OnFindItem(plv, iStartFrom, &lvfi);
#ifdef LVDEBUG
                DebugMsg(DM_TRACE, "CIme listsearch %08lX %s %d", (LPSTR)lpsz, (LPSTR)lpsz, i);
#endif

                if (i != -1) {

                    // if they're hitting the same char, make sure there's not a closer match
                    // (find "new" before "nn")
                    if (iLen > 2 && SameDBCSChars(lpsz, (WORD)((BYTE)lpsz[0] << 8 | (BYTE)lpsz[1]))) {
                        int i2;

                        lvfi.psz = lpsz + iLen - 2;
                        i2 = ListView_OnFindItem(plv, (plv->iFocus) % iCount, &lvfi);

                        // see if i2 is between iStartFrom and i
                        if (i < plv->iFocus) i += iCount;
                        if (i2 < plv->iFocus) i2 += iCount;
                        if (i2 < i && i2 > plv->iFocus)
                            i = i2;
                        i %= iCount;
#ifdef LVDEBUG
                        DebugMsg(DM_TRACE, "CIme listsearch2 %d %d", i2, i);
#endif
                    }

                    ListView_SetFocusSel(plv, i, TRUE, TRUE, FALSE);
                    plv->iMark = i;
                    if (ListView_CancelScrollWait(plv))
                        ListView_OnEnsureVisible(plv, i, FALSE);
                } else {

                    // if they hit the same key twice in a row at the beginning of
                    // the search, and there was no item found, they likely meant to
                    // retstart the search
                    if (iLen > 2 && SameDBCSChars(lpsz, (WORD)((BYTE)lpsz[0] << 8 | (BYTE)lpsz[1]))) {

                        // first clear out the string so that we won't recurse again
                        IncrementSearchString(0, NULL);
                        ListView_OnImeComposition(plv, wParam, lParam);
                    } else {
                        // Don't beep on spaces, we use it for selection.
                        if (!g_iIncrSearchFailed)
                            MessageBeep(0);
                        g_iIncrSearchFailed++;
                    }
                }
                FREE_COMP_STRING(pszCompStr);
            }
        }
        ImmReleaseContext(plv->hwnd, hImc);
    }
    return fRet;
}

BOOL FAR PASCAL SameDBCSChars(LPSTR lpsz,  WORD w)
{
    while (*lpsz) {
        if (IsDBCSLeadByte((BYTE)*lpsz) == FALSE)
            return FALSE;
        if ((WORD)((BYTE)*lpsz++ << 8 | (BYTE)*lpsz++) != w)
            return FALSE;
    }
    return TRUE;
}
#endif

// REVIEW: We will want to reset ichCharBuf to 0 on certain conditions,
// such as: focus change, ENTER, arrow key, mouse click, etc.
//
void NEAR ListView_OnChar(LV* plv, UINT ch, int cRepeat)
{
    LPSTR lpsz;
    LV_FINDINFO lvfi;
    int i;
    int iStartFrom = -1;
    int iLen;
    int iCount;

    iCount = ListView_Count(plv);

    if (!iCount || plv->iFocus == -1)
        return;

    // Don't search for chars that cannot be in a file name (like ENTER and TAB)
    if (ch < ' ' || GetKeyState(VK_CONTROL) < 0)
    {
        IncrementSearchString(0, NULL);
        return;
    }

    if (IncrementSearchString(ch, &lpsz))
        iStartFrom = plv->iFocus;
    else
        iStartFrom = ((plv->iFocus - 1) + iCount)% iCount;

    lvfi.flags = LVFI_SUBSTRING | LVFI_STRING | LVFI_WRAP;
    lvfi.psz = lpsz;
    iLen = lstrlen(lpsz);

    // special case space as the first character
    if ((iLen == 1) && (*lpsz == ' ')) {
        if (plv->iFocus != -1) {
            ListView_OnSetItemState(plv, plv->iFocus, LVIS_SELECTED, LVIS_SELECTED);
            IncrementSearchString(0, NULL);
        }
        return;
    }

    i = ListView_OnFindItem(plv, iStartFrom, &lvfi);
#ifdef LVDEBUG
    DebugMsg(DM_TRACE, "listsearch %d %s %d", (LPSTR)lpsz, (LPSTR)lpsz, i);
#endif

    if (i != -1) {

        // if they're hitting the same char, make sure there's not a closer match
        // (find "new" before "nn")
        if (iLen > 1 && SameChars(lpsz, lpsz[0])) {
            int i2;

            lvfi.psz = lpsz + iLen - 1;
            i2 = ListView_OnFindItem(plv, (plv->iFocus) % iCount, &lvfi);

            // see if i2 is between iStartFrom and i
            if (i < plv->iFocus) i += iCount;
            if (i2 < plv->iFocus) i2 += iCount;
            if (i2 < i && i2 > plv->iFocus)
                i = i2;
            i %= iCount;
#ifdef LVDEBUG
            DebugMsg(DM_TRACE, "listsearch2 %d %d", i2, i);
#endif
        }

        ListView_SetFocusSel(plv, i, TRUE, TRUE, FALSE);
        plv->iMark = i;
        if (ListView_CancelScrollWait(plv))
            ListView_OnEnsureVisible(plv, i, FALSE);
    } else {

        // if they hit the same key twice in a row at the beginning of
        // the search, and there was no item found, they likely meant to
        // retstart the search
        if (iLen > 1 && SameChars(lpsz, lpsz[0])) {

            // first clear out the string so that we won't recurse again
            IncrementSearchString(0, NULL);
            ListView_OnChar(plv, ch, cRepeat);
        } else {
            // Don't beep on spaces, we use it for selection.
            if (!g_iIncrSearchFailed)
                MessageBeep(0);
            g_iIncrSearchFailed++;
        }

    }
}

BOOL FAR PASCAL SameChars(LPSTR lpsz, char c)
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

void NEAR ListView_InvalidateCachedLabelSizes(LV* plv)
{
    int i;
    // Label wrapping has changed, so we need to invalidate the
    // size of the items, such that they will be recomputed.
    //
    if (plv->style & LVS_NOITEMDATA)
    {
        for (i = ListView_Count(plv) - 1; i >= 0; i--)
        {
            ListView_NIDSetItemCXLabel(plv, i, SRECOMPUTE);
        }
    }
    else
    {
        for (i = ListView_Count(plv) - 1; i >= 0; i--)
        {
            LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, i);
            pitem->cxSingleLabel = pitem->cxMultiLabel = pitem->cyMultiLabel = SRECOMPUTE;

        }
    }
    plv->rcView.left = RECOMPUTE;

    if ((plv->style & LVS_OWNERDRAWFIXED) && ListView_IsReportView(plv))
        plv->cyItemSave = max(plv->cyLabelChar, plv->cySmIcon) + g_cyBorder;
    else {
        plv->cyItem = max(plv->cyLabelChar, plv->cySmIcon) + g_cyBorder;
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

        ListView_DismissEdit(plv, FALSE);   // Cancels edits

        changeFlags = plv->style ^ pinfo->styleNew;
        styleOld = plv->style;
        plv->style = pinfo->styleNew;   // change our version

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

        if ((changeFlags & LVS_AUTOARRANGE) && (plv->style & LVS_AUTOARRANGE))
        {
            ListView_OnArrange(plv, LVA_DEFAULT);
            fRedraw = TRUE;
        }

        // bugbug, previously, this was the else to
        // (changeFlags & LVS_AUTOARRANGE && (plv->style & LVS_AUTOARRANGE))
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
            RedrawWindow(plv->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
    }
}

void NEAR ListView_TypeChange(LV* plv, DWORD styleOld)
{
    RECT rc;

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
    switch (plv->style & LVS_TYPEMASK)
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

    GetClientRect(plv->hwnd, &rc);
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
        if (ListView_IsSmallView(plv))
            pinfo->iItem = ListView_SItemHitTest(plv, x, y, &flags);
        else if (ListView_IsListView(plv))
            pinfo->iItem = ListView_LItemHitTest(plv, x, y, &flags);
        else if (ListView_IsIconView(plv))
            pinfo->iItem = ListView_IItemHitTest(plv, x, y, &flags);
        else if (ListView_IsReportView(plv))
            pinfo->iItem = ListView_RItemHitTest(plv, x, y, &flags);
    }

    pinfo->flags = flags;

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

BOOL NEAR PASCAL CanScroll(HWND hwnd, int code, BOOL bDown)
{
    SCROLLINFO si;

#ifdef IEWIN31_25
    LV* plv = ListView_GetPtr(hwnd);

    if ( 1 )    // This is obsolete but to minimize changing original code...
    {
        int   n1, n2;
        GetScrollRange( hwnd, code, &n1, &n2 );
        si.nMin = n1;
        si.nMax = n2;
        si.nPos = GetScrollPos( hwnd, code );
        si.nPage = ( code == SB_HORZ ) ? plv->cxScrollPage : plv->cyScrollPage;
#else //IEWIN31_25
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;

    if (GetScrollInfo(hwnd, code, &si))
    {
#endif //IEWIN31_25
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

    if (!(plv->style & (WS_HSCROLL | WS_VSCROLL)))
        return;

    dx = dy = plv->cyIcon / 2;
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

    if (plv->style & WS_VSCROLL) { // scroll vertically?

        if (pt.y >= plv->sizeClient.cy) {
            if (CanScroll(plv->hwnd, SB_VERT, TRUE))
                *pdy = ScrollAmount(pt.y, plv->sizeClient.cy, dy);   // down
        } else if (pt.y <= 0) {
            if (CanScroll(plv->hwnd, SB_VERT, FALSE))
                *pdy = -ScrollAmount(0, pt.y, dy);     // up
        }
    }

    if (plv->style & WS_HSCROLL) { // horizontally

        if (pt.x >= plv->sizeClient.cx) {
            if (CanScroll(plv->hwnd, SB_HORZ, TRUE))
                *pdx = ScrollAmount(pt.x, plv->sizeClient.cx, dx);    // right
        } else if (pt.x <= 0) {
            if (CanScroll(plv->hwnd, SB_HORZ, FALSE))
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
BOOL ShouldScroll(LV *plv, LPPOINT ppt, LPRECT lprc)
{
    Assert(ppt);

    if (plv->style & WS_VSCROLL)
        {
        if (ppt->y >= lprc->bottom)
        {
            if (CanScroll(plv->hwnd, SB_VERT, TRUE))
                return TRUE;
        }
        else if (ppt->y <= lprc->top)
        {
            if (CanScroll(plv->hwnd, SB_VERT, FALSE))
                return TRUE;
        }
    }

    if (plv->style & WS_HSCROLL)
    {
        if (ppt->x >= lprc->right)
        {
            if (CanScroll(plv->hwnd, SB_HORZ, TRUE))
                return TRUE;
        }
        else if (ppt->x <= lprc->left)
        {
            if (CanScroll(plv->hwnd, SB_HORZ, FALSE))
                return TRUE;
        }
    }

    return FALSE;
}

//----------------------------------------------------------------------------
void NEAR ListView_DragSelect(LV *plv, int x, int y)
{
    RECT rc, rcWindow, rcOld, rcUnion, rcTemp2;
    POINT pt;
    MSG32 msg32;
    HDC hdc;
    HWND hwnd = plv->hwnd;
    int i, iEnd, dx, dy;
    BOOL bInOld, bInNew;
    DWORD dwTime, dwNewTime;

    rc.left = rc.right = x;
    rc.top = rc.bottom = y;

    rcOld = rc;

    SetCapture(hwnd);
    hdc = GetDC(hwnd);

    DrawFocusRect(hdc, &rc);

    GetWindowRect(hwnd, &rcWindow);

    dwTime = GetTickCount();

    for (;;)
    {
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
        continue;
    }


        // WM_CANCELMODE messages will unset the capture, in that
        // case I want to exit this loop

        if (GetCapture() != hwnd)
        {
            break;
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
            ReleaseCapture();
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
            if ((dwNewTime - dwTime) > SCROLL_FREQ)
        {
                dwTime = dwNewTime; // reset scroll timer
                ScrollDetect(plv, pt, &dx, &dy);
        }
        else
        {
        dx = dy = 0;
        }
            SetTimer(plv->hwnd, IDT_MARQUEE, SCROLL_FREQ, NULL);

            y -= dy;    // scroll up/down
            x -= dx;    // scroll left/right

            rc.left = x;
            rc.top = y;
            rc.right = pt.x;
            rc.bottom = pt.y;

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
                i = 0;
                iEnd = ListView_Count(plv);
            }

            // make sure our endpoint is in range.
            if (iEnd > ListView_Count(plv))
                iEnd = ListView_Count(plv);

            if (i < 0)
                i = 0;

            if (bInNew && !(msg32.wParam & (MK_CONTROL | MK_SHIFT))) {
                plv->iMark = -1;
            }

            for (; i  < iEnd; i++) {
                RECT dummy;
                ListView_GetRects(plv, i, NULL, NULL, NULL, &rcTemp2);
                InflateRect(&rcTemp2, -(rcTemp2.right - rcTemp2.left) / 4, -(rcTemp2.bottom - rcTemp2.top) / 4);

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
                        //DebugMsg(DM_TRACE, "dItem = %d, dMax = %d", dItem, dMax);
                        if (dItem > dMax) {
                            //DebugMsg(DM_TRACE, "taking dItem .. iMark = %d", i);
                            dMax = dItem;
                            plv->iMark = i;
                        }
                    }
                }
            }

            //DebugMsg(DM_TRACE, "Final iMark = %d", plv->iMark);
            UpdateWindow(plv->hwnd);    // make selection draw

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
        DoDefault:
            TranslateMessage32(&msg32, TRUE);
            DispatchMessage32(&msg32, TRUE);
        }
    }

EndOfLoop:
    DrawFocusRect(hdc, &rcOld); // erase old
    ReleaseDC(hwnd, hdc);
}


#define SHIFT_DOWN(keyFlags)    (keyFlags & MK_SHIFT)
#define CONTROL_DOWN(keyFlags)  (keyFlags & MK_CONTROL)
#define RIGHTBUTTON(keyFlags)   (keyFlags & MK_RBUTTON)


void NEAR ListView_OnButtonDown(LV* plv, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    int iItem;
    BOOL bSelected;
    LV_HITTESTINFO ht;
    BOOL fNotifyReturn;

    int click = RIGHTBUTTON(keyFlags) ? NM_RCLICK : NM_CLICK;
    int drag  = RIGHTBUTTON(keyFlags) ? LVN_BEGINRDRAG : LVN_BEGINDRAG;

#ifdef LVDEBUG
    DebugMsg(DM_TRACE, "ListView_OnButtonDown %d", fDoubleClick);
#endif

#if 0
    if (!ListView_DismissEdit(plv, FALSE))   // end any previous editing (accept it)
        return;     // Something happened such that we should not process button down
#endif

    SetCapture(plv->hwnd);
    if (!ListView_DismissEdit(plv, FALSE) && GetCapture() != plv->hwnd)
        return;
    ReleaseCapture();

    // REVIEW: right button implies no shift or control stuff
    // Single selection style also implies no modifiers
    //if (RIGHTBUTTON(keyFlags) || (plv->style & LVS_SINGLESEL))
    if ((plv->style & LVS_SINGLESEL))
        keyFlags &= ~(MK_SHIFT | MK_CONTROL);

    ht.pt.x = x;
    ht.pt.y = y;
    iItem = ListView_OnHitTest(plv, &ht);

    bSelected = (iItem >= 0) && ListView_OnGetItemState(plv, iItem, LVIS_SELECTED);

    if (fDoubleClick)
    {
        //
        // Cancel any name editing that might happen.
        //
        ListView_CancelPendingEdit(plv);
        KillTimer(plv->hwnd, IDT_SCROLLWAIT);
#if 0
        if ((ht.flags & (LVHT_ONITEMLABEL | LVHT_ONITEMICON)) &&
            !bSelected && !SHIFT_DOWN(keyFlags) && !CONTROL_DOWN(keyFlags))
        {
            ListView_SetFocusSel(plv, iItem, TRUE, TRUE, FALSE);
        }
#endif

        SendNotify(plv->hwndParent, plv->hwnd, RIGHTBUTTON(keyFlags) ? NM_RDBLCLK : NM_DBLCLK, NULL);
        return;
    }

    if (ht.flags & (LVHT_ONITEMLABEL | LVHT_ONITEMICON))
    {
        if (SHIFT_DOWN(keyFlags)) {
            ListView_SelectRangeTo(plv, iItem, !CONTROL_DOWN(keyFlags));
            ListView_SetFocusSel(plv, iItem, TRUE, FALSE, FALSE);
        } else if (!CONTROL_DOWN(keyFlags)) {
            ListView_SetFocusSel(plv, iItem, TRUE, !bSelected, FALSE);
        }

        if (CheckForDragBegin(plv->hwnd, x, y))
        {
        // Before we start dragging, make it sure that it is
        // selected and has the focus.
            ListView_SetFocusSel(plv, iItem, TRUE, FALSE, FALSE);

        // Then, we need to update the window before start dragging
        // to show the selection chagne.
        UpdateWindow(plv->hwnd);

        // let the caller start dragging
            ListView_SendChange(plv, iItem, 0, drag, 0, 0, 0, x, y, 0);
        return;
        }
        else
        {
            // button came up and we are not dragging

            if (CONTROL_DOWN(keyFlags)) {
                // do this on the button up so that ctrl-dragging a range
                // won't toggle the select.

                if (SHIFT_DOWN(keyFlags))
                    ListView_SetFocusSel(plv, iItem, FALSE, FALSE, FALSE);
                else {
                    ListView_SetFocusSel(plv, iItem, TRUE, FALSE, TRUE);
                }
            }

            if (!SHIFT_DOWN(keyFlags))
                plv->iMark = iItem;

            SetFocus(plv->hwnd);    // activate this window

            // now do the deselect stuff
            if (!SHIFT_DOWN(keyFlags) && !CONTROL_DOWN(keyFlags) && !RIGHTBUTTON(keyFlags))
            {
                ListView_DeselectAll(plv, iItem);
                if ((ht.flags & LVHT_ONITEMLABEL) && bSelected)
                {
                    // Click on item label.  It was selected and
                    // no modifier keys were pressed and no drag operation
                    // So setup for name edit mode.  Still need to wait
                    // to make sure user is not doing double click.
                    //
                    ListView_SetupPendingNameEdit(plv);
                }
            }

            fNotifyReturn = !SendNotify(plv->hwndParent, plv->hwnd, click, NULL);
        }
    }
    else if (ht.flags & LVHT_ONITEMSTATEICON)
    {
        // Should activate window and send notificiation to parent...
        SetFocus(plv->hwnd);    // activate this window
        fNotifyReturn = !SendNotify(plv->hwndParent, plv->hwnd, click, NULL);
    }
    else if (ht.flags & LVHT_NOWHERE)
    {
        if (!SHIFT_DOWN(keyFlags) && !CONTROL_DOWN(keyFlags))
            ListView_DeselectAll(plv, -1);

        SetFocus(plv->hwnd);    // activate this window

    // If single-select listview, disable marquee selection.
        if (!(plv->style & LVS_SINGLESEL) && CheckForDragBegin(plv->hwnd, x, y))
        {
            ListView_DragSelect(plv, x, y);
        }
        fNotifyReturn = !SendNotify(plv->hwndParent, plv->hwnd, click, NULL);
    }

    if (fNotifyReturn && (click == NM_RCLICK))
        SendMessage(plv->hwndParent, WM_CONTEXTMENU, (WPARAM)plv->hwnd, GetMessagePos());
}

#define ListView_CancelPendingEdit(plv) ListView_CancelPendingTimer(plv, LVF_NMEDITPEND, IDT_NAMEEDIT)
#define ListView_CancelScrollWait(plv) ListView_CancelPendingTimer(plv, LVF_SCROLLWAIT, IDT_SCROLLWAIT)

BOOL NEAR ListView_CancelPendingTimer(LV* plv, UINT fFlags, int idTimer)
{
    if (plv->flags & fFlags)
    {
        KillTimer(plv->hwnd, idTimer);
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
    }

    KillTimer(plv->hwnd, id);
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
    SetTimer(plv->hwnd, IDT_NAMEEDIT, GetDoubleClickTime(), NULL);
    plv->flags |= LVF_NMEDITPEND;
}

void NEAR PASCAL _ListView_OnScroll(LV* plv, UINT code, int pos, int sb)
{
#ifdef SIF_TRACKPOS
#ifndef IEWIN31_25
    SCROLLINFO si;

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_TRACKPOS;


    // if we're in 32bits, don't trust the pos since it's only 16bit's worth
    if (GetScrollInfo(plv->hwnd, sb, &si))
        pos = (int)si.nTrackPos;
#endif //IEWIN31_25
#endif
    ListView_DismissEdit(plv, FALSE);

    if (ListView_IsIconView(plv) || ListView_IsSmallView(plv))
        ListView_IOnScroll(plv, code, pos, sb);
    else if (ListView_IsListView(plv))
        ListView_LOnScroll(plv, code, pos);
    else if (ListView_IsReportView(plv))
        ListView_ROnScroll(plv, code, pos, sb);
}
void NEAR ListView_OnVScroll(LV* plv, HWND hwndCtl, UINT code, int pos)
{
    _ListView_OnScroll(plv, code, pos, SB_VERT);
}

void NEAR ListView_OnHScroll(LV* plv, HWND hwndCtl, UINT code, int pos)
{
    _ListView_OnScroll(plv, code, pos, SB_HORZ);
}

int ListView_ValidateOneScrollParam(LV* plv, int iDirection, int dx)
{
    SCROLLINFO si;

#ifdef IEWIN31_25
    {
        int   n1, n2;
        GetScrollRange( plv->hwnd, iDirection, &n1, &n2 );
        si.nMin = n1;
        si.nMax = n2;
        si.nPos = GetScrollPos( plv->hwnd, iDirection );
        si.nPage = ( iDirection == SB_HORZ ) ? plv->cxScrollPage : plv->cyScrollPage;
    }
#else //IEWIN31_25
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;

    if (!GetScrollInfo(plv->hwnd, iDirection, &si))
        return 0;
#endif //IEWIN31_25
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

    if (plv->style & LVS_NOSCROLL)
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

    if (plv->style & LVS_NOSCROLL)
        return FALSE;

    if (ListView_IsIconView(plv))
    {
        ListView_IScroll2(plv, dx, dy);
    }
    else if (ListView_IsSmallView(plv))
    {
        ListView_IScroll2(plv, dx, dy);
    }

    else if (ListView_IsListView(plv))
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
        ListView_LScroll2(plv, dx, 0);
#else
        if (dy < 0)
            dy -= plv->cyItem - 1;
        else
            dy += plv->cyItem - 1;

        dy = dy / plv->cyItem;

        if (dx)
            return FALSE;
        ListView_LScroll2(plv, 0, dy);
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
        ListView_RScroll2(plv, dx, dy);
    }
    ListView_UpdateScrollBars(plv);
    return TRUE;
}

BOOL NEAR ListView_OnEnsureVisible(LV* plv, int i, BOOL fPartialOK)
{
    RECT rcBounds;
    RECT rc;
    int dx, dy;

    if (i < 0 || i >= ListView_Count(plv) || plv->style & LVS_NOSCROLL)
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

    if ((plv->style & LVS_NOSCROLL) ||
        (!(ListView_RedrawEnabled(plv))))
        return;
    if (ListView_IsIconView(plv) || ListView_IsSmallView(plv))
        ListView_IUpdateScrollBars(plv);
    else if (ListView_IsListView(plv))
        ListView_LUpdateScrollBars(plv);
    else if (ListView_IsReportView(plv))
        ListView_RUpdateScrollBars(plv);

    GetClientRect(plv->hwnd, &rc);
    plv->sizeClient.cx = rc.right;
    plv->sizeClient.cy = rc.bottom;

    dwStyle = GetWindowLong(plv->hwnd, GWL_STYLE);
    plv->style = (plv->style & ~(WS_HSCROLL | WS_VSCROLL)) | (dwStyle & WS_HSCROLL | WS_VSCROLL);
}

// BUGBUG: does not deal with hfont == NULL

void NEAR ListView_OnSetFont(LV* plv, HFONT hfont, BOOL fRedraw)
{
    HDC hdc;
    SIZE siz;

    if ((plv->flags & LVF_FONTCREATED) && plv->hfontLabel) {
        DeleteObject(plv->hfontLabel);
    plv->flags &= ~LVF_FONTCREATED;
    }

    if (hfont == NULL) {
        LOGFONT lf;
        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
        hfont = CreateFontIndirect(&lf);
        plv->flags |= LVF_FONTCREATED;
    }

    hdc = GetDC(HWND_DESKTOP);

    SelectFont(hdc, hfont);

    GetTextExtentPoint(hdc, "0", 1, &siz);

    plv->cyLabelChar = siz.cy;
    plv->cxLabelChar = siz.cx;

    GetTextExtentPoint(hdc, c_szEllipses, CCHELLIPSES, &siz);
    plv->cxEllipses = siz.cx;

    ReleaseDC(HWND_DESKTOP, hdc);

    plv->hfontLabel = hfont;

    ListView_InvalidateCachedLabelSizes(plv);

    // If we have a header window, we need to forward this to it also
    // as we have destroyed the hfont that they are using...
    if (plv->hwndHdr) {
        FORWARD_WM_SETFONT(plv->hwndHdr, plv->hfontLabel, FALSE, SendMessage);
        ListView_UpdateScrollBars(plv);
    }

    if (fRedraw)
        RedrawWindow(plv->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
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

                RedrawWindow(plv->hwnd, NULL, plv->hrgnInval, fRedraw);
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
                if ((plv->style & LVS_AUTOARRANGE) && fChanges) {
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
    Assert(0);
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
            if (himl)
                ImageList_GetIconSize(himl, &plv->cxIcon , &plv->cyIcon);
            break;

        case LVSIL_SMALL:
        hImageOld = plv->himlSmall;
            plv->himlSmall = himl;
            if (himl)
                ImageList_GetIconSize(himl, &plv->cxSmIcon , &plv->cySmIcon);
            plv->cxItem = 16 * plv->cxLabelChar + plv->cxSmIcon;
            plv->cyItem = max(plv->cyLabelChar, plv->cySmIcon) + g_cyBorder;
            break;

        case LVSIL_STATE:
            if (himl) {
                ImageList_GetIconSize(himl, &plv->cxState , &plv->cyState);
            } else {
                plv->cxState = 0;
            }
        hImageOld = plv->himlState;
            plv->himlState = himl;
            break;

    default:
#ifdef LVDEBUG
        DebugMsg(DM_TRACE, "sh TR - LVM_SETIMAGELIST: unrecognized iImageList");
#endif
        break;
    }

    if (himl && !(plv->style & LVS_SHAREIMAGELISTS))
        ImageList_SetBkColor(himl, plv->clrBk);

    if (ListView_Count(plv) > 0)
        InvalidateRect(plv->hwnd, NULL, TRUE);

    return hImageOld;
}

BOOL NEAR ListView_OnGetItem(LV* plv, LV_ITEM FAR* plvi)
{
    UINT mask;
    LISTITEM FAR* pitem = NULL;
    LV_DISPINFO nm;

    if (!plvi)
        return FALSE;

    nm.item.mask = 0;
    mask = plvi->mask;

    if ((plv->style & LVS_NOITEMDATA) == 0)
    {

        // Standard listviews
        pitem = ListView_GetItemPtr(plv, plvi->iItem);
        if (pitem == NULL)
            return FALSE;       // number was out of range!

        // Handle sub-item cases for report view
        //
        if (plvi->iSubItem != 0)
        {
            if (mask & ~LVIF_TEXT)
            {
#ifdef LVDEBUG
                DebugMsg(DM_ERROR, "ListView: Invalid LV_ITEM mask: %04x", mask);
#endif
                return FALSE;
            }

            if (mask & LVIF_TEXT)
            {
                LPSTR psz = ListView_RGetItemText(plv, plvi->iItem, plvi->iSubItem);

                if (psz != LPSTR_TEXTCALLBACK)
                {
                    Str_GetPtr(psz, plvi->pszText, plvi->cchTextMax);
                    return TRUE;
                }

                nm.item.mask |= LVIF_TEXT;
            }
        }

        if (mask & LVIF_TEXT)
        {
            if (pitem->pszText != LPSTR_TEXTCALLBACK)
                Str_GetPtr(pitem->pszText, plvi->pszText, plvi->cchTextMax);
            else
                nm.item.mask |= LVIF_TEXT;
        }

        if (mask & LVIF_PARAM)
            plvi->lParam = pitem->lParam;

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

        if (mask & LVIF_IMAGE)
        {
            if (pitem->iImage == I_IMAGECALLBACK)
                nm.item.mask |= LVIF_IMAGE;
            else
                plvi->iImage = pitem->iImage;
        }
    }
    else
    {
        // Complete call back for info...

        // Handle sub-item cases for report view
        //
        if (plvi->iSubItem != 0)
        {
            if (mask & ~LVIF_TEXT)
            {
#ifdef LVDEBUG
                DebugMsg(DM_ERROR, "ListView: Invalid LV_ITEM mask: %04x", mask);
#endif
                return FALSE;
            }
        }


        if (mask & LVIF_PARAM)
            plvi->lParam = 0L;      // Dont have any to return now...

        if (mask & LVIF_STATE)
        {
            plvi->state = (ListView_NIDGetItemState(plv, plvi->iItem) & plvi->stateMask);
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

        nm.item.mask |= (mask & (LVIF_TEXT | LVIF_IMAGE));
    }

    if (nm.item.mask)
    {
        nm.item.iItem  = plvi->iItem;
        nm.item.iSubItem = plvi->iSubItem;
        if (plv->style & LVS_NOITEMDATA)
            nm.item.lParam = 0L;
        else
            nm.item.lParam = pitem->lParam;

    // just in case LVIF_IMAGE is set and callback doesn't fill it in
    // ... we'd rather have a -1 than whatever garbage is on the stack
    nm.item.iImage = -1;

        if (nm.item.mask & LVIF_TEXT)
        {
            Assert(plvi->pszText);

            nm.item.pszText = plvi->pszText;
            nm.item.cchTextMax = plvi->cchTextMax;

            // Make sure the buffer is zero terminated...
            if (nm.item.cchTextMax)
                *nm.item.pszText = 0;
        }

        SendNotify(plv->hwndParent, plv->hwnd, LVN_GETDISPINFO, &nm.hdr);

        // use nm.item.mask to give the app a chance to change values
        if (nm.item.mask & LVIF_STATE)
            plvi->state ^= ((plvi->state ^ nm.item.state) & nm.item.stateMask);
        if (nm.item.mask & LVIF_IMAGE)
            plvi->iImage = nm.item.iImage;
        if (nm.item.mask & LVIF_TEXT)
            plvi->pszText = nm.item.pszText;

        if (pitem && (nm.item.mask & LVIF_DI_SETITEM)) {

            //DebugMsg(DM_TRACE, "SAVING ITEMS!");
            if (nm.item.mask & LVIF_IMAGE)
                pitem->iImage = nm.item.iImage;

            if (nm.item.mask & LVIF_TEXT  && (nm.item.iSubItem == 0))
                if (nm.item.pszText && (nm.item.pszText != LPSTR_TEXTCALLBACK)) {
                    Assert(pitem->pszText == LPSTR_TEXTCALLBACK);
                    pitem->pszText = NULL;
                    Str_Set(&pitem->pszText, nm.item.pszText);
                }

            if (nm.item.mask & LVIF_STATE)
                pitem->state ^= ((pitem->state ^ nm.item.state) & nm.item.stateMask);
        }
    }

    return TRUE;
}

BOOL NEAR ListView_OnSetItem(LV* plv, const LV_ITEM FAR* plvi)
{
    LISTITEM FAR* pitem;
    UINT mask;
    UINT maskChanged;
    UINT rdwFlags=RDW_INVALIDATE;
    int i;
    UINT stateOld, stateNew;
    BOOL fHasItemData = ((plv->style & LVS_NOITEMDATA) == 0);

    if (!plvi)
        return FALSE;

    Assert(plvi->iSubItem >= 0);

    if (plv->himl && (plv->clrBk != ImageList_GetBkColor(plv->himl)))
        rdwFlags |= RDW_ERASE;

    mask = plvi->mask;
    if (!mask)
        return TRUE;

    // If we do not have item data, we only allow the user to change
    // state
    if (!fHasItemData && (mask != LVIF_STATE))
        return(FALSE);

    // If we're setting a subitem, handle it elsewhere...
    //
    if (plvi->iSubItem > 0)
        return ListView_SetSubItem(plv, plvi);

    i = plvi->iItem;

    if (fHasItemData)
    {
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
    }
    else
    {
        if (i >= ListView_Count(plv))
            return FALSE;

        //REVIEW: Same hack as above
        //
        stateOld = stateNew = 0;
        if (mask & LVIF_STATE)
        {
            stateOld = ListView_NIDGetItemState(plv, i) & plvi->stateMask;
            stateNew = plvi->state & plvi->stateMask;
        }
    }

    // Prevent multiple selections in a single-select listview.
    if ((plv->style & LVS_SINGLESEL) && (mask & LVIF_STATE) && (stateNew & LVIS_SELECTED))
    ListView_DeselectAll(plv, i);

    if (!ListView_SendChange(plv, i, 0, LVN_ITEMCHANGING, stateOld, stateNew, mask, 0, 0, pitem->lParam))
        return FALSE;

    maskChanged = 0;
    if (mask & LVIF_STATE)
    {

        if (fHasItemData)
        {
            UINT change = (pitem->state ^ plvi->state) & plvi->stateMask;

            if (change)
            {
                pitem->state ^= change;

                maskChanged |= LVIF_STATE;

                // the selection state has changed.. update selected count
                if (change & LVIS_SELECTED) {
                    if (pitem->state & LVIS_SELECTED) {
                        plv->nSelected++;
                    } else {
                        plv->nSelected--;
                    }
                }

                // For some bits we can only invert the label area...
                // fSelectOnlyChange = ((change & ~(LVIS_SELECTED | LVIS_FOCUSED | LVIS_DROPHILITED)) == 0);
                // fEraseItem = ((change & ~(LVIS_SELECTED | LVIS_DROPHILITED)) != 0);

                // try to steal focus from the previous guy.
                if ((change & LVIS_FOCUSED) && (plv->iFocus != i)) {
                    if ((plv->iFocus == -1) || ListView_OnSetItemState(plv, plv->iFocus, 0, LVIS_FOCUSED)) {
                        pitem->state |= LVIS_FOCUSED;
                        plv->iFocus = i;
                        if (plv->iMark == -1)
                            plv->iMark = i;
                    }
                }
                if (change & LVIS_CUT)
                    rdwFlags |= RDW_ERASE;
            }
        }
        else
        {
            WORD wState = ListView_NIDGetItemState(plv, i);
            WORD change = (wState ^ plvi->state) & plvi->stateMask;

            if (change)
            {
                wState ^= change;

                maskChanged |= LVIF_STATE;

                if (change & LVIS_SELECTED) {
                    if (wState & LVIS_SELECTED) {
                        plv->nSelected++;
                    } else {
                        plv->nSelected--;
                    }
                }

                // try to steal focus from the previous guy.
                if (change & LVIS_FOCUSED && plv->iFocus != -1 && plv->iFocus != i)
                    if (ListView_OnSetItemState(plv, plv->iFocus, 0, LVIS_FOCUSED))
                        wState |= LVIS_FOCUSED;

                //
                // We need to update the data in the DPA
                ListView_NIDSetItemState(plv, i, wState);

                if (change & LVIS_CUT)
                    rdwFlags |= RDW_ERASE;
            }
        }
    }



    if (mask & LVIF_TEXT)
    {
        RECT rc;

        // need to do this now because we're changing the text
        // so we need to get the rect of the thing before the text changes
        ListView_GetRects(plv, i, NULL, &rc, NULL, NULL);
        // don't redraw the item we are currently painting
        if (plv->iItemDrawing != i)
            InvalidateRect(plv->hwnd, &rc, TRUE); // invalidate with erase

        if (plvi->pszText == LPSTR_TEXTCALLBACK)
        {
        if (pitem->pszText != LPSTR_TEXTCALLBACK)
        Str_Set(&pitem->pszText, NULL);

            pitem->pszText = LPSTR_TEXTCALLBACK;
        }
        else
    {
        if (pitem->pszText == LPSTR_TEXTCALLBACK)
        pitem->pszText = NULL;

            if (!Str_Set(&pitem->pszText, plvi->pszText))
                return FALSE;
        }

        plv->rcView.left = RECOMPUTE;
        pitem->cyMultiLabel = pitem->cxSingleLabel = pitem->cxMultiLabel = SRECOMPUTE;
        maskChanged |= LVIF_TEXT;
    }

    if (mask & LVIF_IMAGE)
    {
        if (pitem->iImage != plvi->iImage)
        {
            pitem->iImage = plvi->iImage;
            maskChanged |= LVIF_IMAGE;

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
            ListView_InvalidateItem(plv, i, FALSE, rdwFlags);

        ListView_SendChange(plv, i, 0, LVN_ITEMCHANGED, stateOld, stateNew, maskChanged, 0, 0, pitem->lParam);
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
    LV_ITEM lvi;

    lvi.mask    = LVIF_STATE;
    lvi.state   = data;
    lvi.stateMask = mask;
    lvi.iItem   = i;
    lvi.iSubItem = 0;

    // HACK?
    // if the item is -1, we will do it for all items.  We special case
    // a few cases here as to speed it up.  For example if the mask is
    // LVIS_SELECTED and data is zero it implies that we will deselect
    // all items...
    //
    if (i != -1)
        return ListView_OnSetItem(plv, &lvi);
    else
    {
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
    else if ((plv->style & LVS_SINGLESEL) && (mask == LVIS_SELECTED))
        return FALSE;   /* can't select all in single-select listview */

        //
        // Now iterate over all of the items that match our criteria and
        // set their new value.
        //
        while ((lvi.iItem = ListView_OnGetNextItem(plv, lvi.iItem,
                flags)) != -1) {
            ListView_OnSetItem(plv, &lvi);
        }
        return(TRUE);
    }
}

int NEAR PASCAL ListView_OnGetItemText(LV* plv, int i, LV_ITEM FAR *plvi)
{
    Assert(plvi->pszText);
    plvi->mask = LVIF_TEXT;
    plvi->iItem = i;
    if (!ListView_OnGetItem(plv, plvi))
        return 0;

    return lstrlen(plvi->pszText);
}

BOOL WINAPI ListView_OnSetItemText(LV* plv, int i, int iSubItem, LPCSTR pszText)
{
    LV_ITEM lvi;

    lvi.mask = LVIF_TEXT;
    lvi.pszText = (LPSTR)pszText;
    lvi.iItem = i;
    lvi.iSubItem = iSubItem;

    return ListView_OnSetItem(plv, &lvi);
}

// Add/remove/replace item

BOOL NEAR ListView_FreeItem(LV* plv, LISTITEM FAR* pitem)
{
    if (pitem)
    {
        if (pitem->pszText != LPSTR_TEXTCALLBACK)
            Str_Set(&pitem->pszText, NULL);

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

    if (pitem)
    {
        if (plvi->mask & LVIF_STATE) {
            if (plvi->state & ~LVIS_ALL)  {
                DebugMsg(DM_ERROR, "ListView: Invalid state: %04x", plvi->state);
                return NULL;
            }

        // If adding a selected item to a single-select listview, deselect
        // any other items.
        if ((plv->style & LVS_SINGLESEL) && (plvi->state & LVIS_SELECTED))
        ListView_DeselectAll(plv, -1);

            pitem->state  = (plvi->state & ~(LVIS_FOCUSED | LVIS_SELECTED));
        }
        if (plvi->mask & LVIF_PARAM)
            pitem->lParam = plvi->lParam;

        if (plvi->mask & LVIF_IMAGE)
            pitem->iImage = plvi->iImage;


        plv->rcView.left = pitem->pt.x = pitem->pt.y = RECOMPUTE;
        pitem->cxSingleLabel = pitem->cxMultiLabel = pitem->cyMultiLabel = SRECOMPUTE;

        pitem->pszText = NULL;
        if (plvi->mask & LVIF_TEXT) {
            if (plvi->pszText == LPSTR_TEXTCALLBACK)
            {
                pitem->pszText = LPSTR_TEXTCALLBACK;
            }
            else if (!Str_Set(&pitem->pszText, plvi->pszText))
            {
                ListView_FreeItem(plv, pitem);
                return NULL;
            }
        }

    }
    return pitem;
}

void ListView_LRInvalidateBelow(LV* plv, int i)
{
    if (ListView_IsListView(plv) || ListView_IsReportView(plv)) {
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

        // For both List and report view need to erase the item and
        // below.  Note: do simple test to see if there is anything
        // to redraw

        // we can't check for bottom/right > 0 because if we nuked something
        // above or to the left of the view, it may affect us all
        if ((rcItem.top <= plv->sizeClient.cy) &&
            (rcItem.left <= plv->sizeClient.cx))
        {
            rcItem.bottom = plv->sizeClient.cy;
            RedrawWindow(plv->hwnd, &rcItem, NULL, RDW_INVALIDATE | RDW_ERASE);
            if (ListView_IsListView(plv))
            {
                RECT rcClient;
                // For Listview we need to erase the other columns...
                rcClient.left = rcItem.right;
                rcClient.top = 0;
                rcClient.bottom = plv->sizeClient.cy;
                rcClient.right = plv->sizeClient.cx;
                RedrawWindow(plv->hwnd, &rcClient, NULL, RDW_INVALIDATE | RDW_ERASE);
            }
        }
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
        if (plv->style & LVS_AUTOARRANGE)
            ListView_OnArrange(plv, LVA_DEFAULT);
        else
            RedrawWindow(plv->hwnd, NULL, NULL, RDW_INTERNALPAINT | RDW_NOCHILDREN);
    }
    else
    {
        ListView_LRInvalidateBelow(plv, i);
    }
    ListView_UpdateScrollBars(plv);
}

int NEAR ListView_OnInsertItem(LV* plv, const LV_ITEM FAR* plvi)
{
    int iItem;

    if (!plvi || (plvi->iSubItem != 0))    // can only insert the 0th item
    {
        DebugMsg(DM_ERROR, "ListView_InsertItem: iSubItem must be 0");
        return -1;
    }

    // If sorted, then insert sorted.
    //
    if (plv->style & (LVS_SORTASCENDING | LVS_SORTDESCENDING))
    {
        if (plvi->pszText == LPSTR_TEXTCALLBACK)
        {
            DebugMsg(DM_ERROR, "Don't use LPSTR_TEXTCALLBACK with LVS_SORTASCENDING or LVS_SORTDESCENDING");
            return -1;
        }
        iItem = ListView_LookupString(plv, plvi->pszText, LVFI_SUBSTRING | LVFI_NEARESTXY, 0);
    }
    else
        iItem = plvi->iItem;

    if ((plv->style & LVS_NOITEMDATA) == 0)
    {
    int iZ;
        LISTITEM FAR *pitem = ListView_CreateItem(plv, plvi);
        UINT uSelMask = plvi->mask & LVIF_STATE ?
        (plvi->state & (LVIS_FOCUSED | LVIS_SELECTED))
        : 0;

        if (!pitem)
            return -1;

        iItem = DPA_InsertPtr(plv->hdpa, iItem, pitem);
        if (iItem == -1)
        {
            ListView_FreeItem(plv, pitem);
            return -1;
        }

    if (plv->hdpaSubItems)
    {
        int iCol;
        // slide all the colum DPAs down to match the location of the
        // inserted item
        //
        for (iCol = plv->cCol - 1; iCol >= 0; iCol--)
        {
            HDPA hdpa = ListView_GetSubItemDPA(plv, iCol);
            if (hdpa)   // this is optional, call backs don't have them
            {
                // insert a blank item (REVIEW: should this be callback?)
                if (DPA_InsertPtr(hdpa, iItem, NULL) != iItem)
                    goto Failure;
            Assert(ListView_Count(plv) == DPA_GetPtrCount(hdpa));
            }
        }
    }

        // Add item to end of z order
        //
        iZ = DPA_InsertPtr(plv->hdpaZOrder, ListView_Count(plv), (LPVOID)iItem);

        if (iZ == -1)
        {
Failure:
        DebugMsg(DM_TRACE, "ListView_OnInsertItem() failed");
            DPA_DeletePtr(plv->hdpa, iItem);
            ListView_FreeItem(plv, pitem);
            return -1;
        }


        // If the item was not added at the end of the list we need
        // to update the other indexes in the list
        if (iItem != ListView_Count(plv) - 1)
        {
            int i2;
            for (i2 = iZ - 1; i2 >= 0; i2--)
            {
                int iItemZ = (int)(DWORD)DPA_FastGetPtr(plv->hdpaZOrder, i2);
                if (iItemZ >= iItem)
                    DPA_SetPtr(plv->hdpaZOrder, i2, (LPVOID)(DWORD)(iItemZ + 1));
            }
        }

        if (uSelMask) {
            // we masked off these in the createitem above.
            // because turning these on means more than setting the bits.
            ListView_OnSetItemState(plv, iItem, uSelMask, uSelMask);
        }
    }
    else
    {
        // The item has no data associated with it.  Simply insert RECOMPUTE and
        // state = 0 for the item.
        // For now we wont insert in the zorder one as we wont support those
        // views that need it...
        iItem = DPA_InsertPtr(plv->hdpa, iItem, (void *)MAKELONG(0, SRECOMPUTE));
        if (iItem == -1)
            return -1;
    }

    Assert(ListView_Count(plv) == DPA_GetPtrCount(plv->hdpaZOrder));

    if (ListView_RedrawEnabled(plv))
    {
        // The Maybe resize colmns may resize things in which case the next call
        // to Update is not needed.
        if (!ListView_MaybeResizeListColumns(plv, iItem, iItem))
            ListView_OnUpdate(plv, iItem);
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

    return iItem;
}

BOOL NEAR ListView_OnDeleteItem(LV* plv, int iItem)
{
    int iCount = ListView_Count(plv);

    if ((iItem < 0) || (iItem >= iCount))
    return FALSE;   // out of range

    ListView_DismissEdit(plv, FALSE);

    ListView_OnSetItemState(plv, iItem, 0, LVIS_SELECTED);

    if ((plv->style & LVS_NOITEMDATA) == 0)
    {
    LISTITEM FAR* pitem;
    int iZ;

        if ((plv->rcView.left != RECOMPUTE) && (ListView_IsIconView(plv) || ListView_IsSmallView(plv))) {
            RECT rc;

            ListView_GetRects(plv, iItem, NULL, NULL, &rc, NULL);
            if (plv->rcView.left == rc.left ||
                plv->rcView.right == rc.right ||
                plv->rcView.top == rc.top ||
                plv->rcView.bottom == rc.bottom) {
                plv->rcView.left = RECOMPUTE;
            }

        }
        ListView_InvalidateItem(plv, iItem, FALSE, RDW_INVALIDATE | RDW_ERASE);

        // this notify must be done AFTER the Invalidate because some items need callbacks
        // to calculate the rect, but the notify might free it out
        ListView_Notify(plv, iItem, 0, LVN_DELETEITEM);

        pitem = DPA_DeletePtr(plv->hdpa, iItem);
    // if (!pitem)          // we validate iItem is in range
    //     return FALSE;        // so this is not necessary

        // remove from the z-order, this is a linear search to find this!

        DPA_DeletePtr(plv->hdpaZOrder, ListView_ZOrderIndex(plv, iItem));

        //
        // As the Z-order hdpa is a set of indexes we also need to decrement
        // all indexes that exceed the one we are deleting.
        //
        for (iZ = ListView_Count(plv) - 1; iZ >= 0; iZ--)
        {
            int iItemZ = (int)(DWORD)DPA_FastGetPtr(plv->hdpaZOrder, iZ);
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
            if (hdpa) { // this is optional, call backs don't have them
                LPSTR psz = DPA_DeletePtr(hdpa, iItem);
            if (psz != LPSTR_TEXTCALLBACK)
                Str_Set(&psz, NULL);
            Assert(ListView_Count(plv) == DPA_GetPtrCount(hdpa));
        }
        }
    }

        ListView_FreeItem(plv, pitem);  // ... finaly the item pointer

    }
    else
    {
        // For items that have no data we simply notify and remove the item
    // iItem was already validated so this should not fail
        //
        ListView_Notify(plv, iItem, 0, LVN_DELETEITEM);
        DPA_DeletePtr(plv->hdpa, iItem);
    }



    iCount = ListView_Count(plv);   // regrab count incase someone updated item...

    Assert(ListView_Count(plv) == DPA_GetPtrCount(plv->hdpaZOrder));

    if (plv->iFocus == iItem)  { // deleted the focus item

        if (plv->iFocus >= iCount) // did we nuke the last item?
            plv->iFocus = iCount - 1;

    } else if (plv->iFocus > iItem)
        plv->iFocus--;          // slide the focus index down

    // same with the mark
    if (plv->iMark == iItem)  { // deleted the mark item

        if (plv->iMark >= iCount) // did we nuke the last item?
            plv->iMark = iCount - 1;

    } else if (plv->iMark > iItem)
        plv->iMark--;          // slide the mark index down

    if (ListView_RedrawEnabled(plv))
        ListView_OnUpdate(plv, iItem);
    else
    {

        ListView_LRInvalidateBelow(plv, iItem);
        //
        // Special case code to make using SetRedraw work reasonably well
        // for adding items to a listview which is in a non layout mode...
        //
        if ((plv->iFirstChangedNoRedraw != -1) &&
                (iItem < plv->iFirstChangedNoRedraw))
            plv->iFirstChangedNoRedraw--;
    }

    return TRUE;
}

BOOL NEAR ListView_OnDeleteAllItems(LV* plv)
{
    int i;
    BOOL bAlreadyNotified;
    BOOL fHasItemData;

    ListView_DismissEdit(plv, FALSE);    // cancel edits

    bAlreadyNotified = (BOOL)ListView_Notify(plv, -1, 0, LVN_DELETEALLITEMS);

    fHasItemData = ((plv->style & LVS_NOITEMDATA) == 0);

    if (fHasItemData || !bAlreadyNotified)
    {
        for (i = ListView_Count(plv) - 1; i >= 0; i--)
        {
            if (!bAlreadyNotified)
                ListView_Notify(plv, i, 0, LVN_DELETEITEM);

            if (fHasItemData)
                ListView_FreeItem(plv, ListView_FastGetItemPtr(plv, i));
        }
    }

    DPA_DeleteAllPtrs(plv->hdpa);
    DPA_DeleteAllPtrs(plv->hdpaZOrder);

    if (plv->hdpaSubItems)
    {
        int iCol;
        for (iCol = plv->cCol - 1; iCol >= 0; iCol--)
        {
            HDPA hdpa = ListView_GetSubItemDPA(plv, iCol);
            if (hdpa) {
        ListView_FreeColumnData(hdpa);
                DPA_DeleteAllPtrs(hdpa);
        }
        }
    }

    plv->rcView.left = RECOMPUTE;
    plv->ptOrigin.x = plv->ptOrigin.y = 0;
    plv->xOrigin = 0;
    plv->iMark = plv->iFocus = -1;
    plv->nSelected = 0;

    plv->ptlRptOrigin.x = 0;
    plv->ptlRptOrigin.y = 0;

    // reset the cxItem width
    if (!(plv->flags & LVF_COLSIZESET))
        plv->cxItem = 16 * plv->cxLabelChar + plv->cxSmIcon;

    RedrawWindow(plv->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
    ListView_UpdateScrollBars(plv);

    return TRUE;
}


int PASCAL ListView_IFindNearestItem(LV* plv, int left, int top, UINT vk)
{
    DWORD dMin;
    int iMin = -1;
    int cyItem;
    int yEnd, yLimit, xEnd;
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
                ListView_ROnScroll(plv, SB_LINELEFT, 0, SB_HORZ);
            }
            else
                iStart -= plv->cItemCol;
            break;

        case VK_RIGHT:
            if (ListView_IsReportView(plv))
            {
                // Make this horizontally scroll the report view
                ListView_ROnScroll(plv, SB_LINERIGHT, 0, SB_HORZ);
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
                iStart = (int)(((LONG)(plv->sizeClient.cy - (plv->cyItem - 1)
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

        if (iStart != -1) {
            ListView_GetRects(plv, iStart, &rcFocus, NULL, NULL, NULL);
        }

        switch (vk)
        {
        // For standard arrow keys just fall out of here.
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
            if (iStart != -1) {
                // all keys map to VK_HOME except VK_END
                break;
            }

            // Fall through
            vk = VK_HOME;
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
    int cItemMax = ListView_Count(plv);

    if (i < -1 || i >= cItemMax)
        return -1;

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

            if ((plv->style & LVS_NOITEMDATA) == 0)
            {
                LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, i);
                wItemState = pitem->state;
            }
            else
                wItemState = ListView_NIDGetItemState(plv, i);


            if ((flags & LVNI_FOCUSED) && !(wItemState & LVIS_FOCUSED))
                continue;
            if ((flags & LVNI_SELECTED) && !(wItemState & LVIS_SELECTED))
                continue;
            if ((flags & LVNI_CUT) && !(wItemState & LVIS_CUT))
                continue;
            if ((flags & LVNI_DROPHILITED) && !(wItemState & LVIS_DROPHILITED))
                continue;
        }
        return i;
    }
}

int NEAR ListView_CompareString(LV* plv, int i, LPCSTR pszFind, UINT flags, int iLen)
{
    // BUGBUG: non protected globals
    int cb;
    char ach[CCHLABELMAX];
    char szTemp[CCHLABELMAX+1];
    LV_ITEM item;

    Assert(pszFind);

    item.iItem = i;
    item.iSubItem = 0;
    item.mask = LVIF_TEXT;
    item.pszText = ach;
    item.cchTextMax = sizeof(ach);
    ListView_OnGetItem(plv, &item);

    if (!(flags & (LVFI_PARTIAL | LVFI_SUBSTRING)))
        return lstrcmpi(item.pszText, pszFind);

    // REVIEW: LVFI_SUBSTRING is not really implemented yet.

    cb = lstrlen(pszFind);
    if (iLen && (cb > iLen)) {
        cb = iLen;
    }

    // TEMPORARY until we have a strcmpni that doesn't modify the buffer
    lstrcpyn(szTemp, pszFind, sizeof(szTemp));

    return StrCmpNI(item.pszText, szTemp, cb);
}

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

    if (iStart < -1 || iStart >= ListView_Count(plv))
        return -1;

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
        LPCSTR pszFind = plvfi->psz;
        if (!pszFind)
            return -1;

        if (plv->style & (LVS_SORTASCENDING | LVS_SORTDESCENDING))
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
    return -1;
}

BOOL NEAR ListView_OnGetItemRect(LV* plv, int i, RECT FAR* prc)
{
    LPRECT pRects[LVIR_MAX];

    // validate parameters
    if (i < 0 || i >= ListView_Count(plv) || !prc || prc->left >= LVIR_MAX || prc->left < 0)
    {
        DebugMsg(DM_ERROR, "ListView: invalid index or rect pointer");
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

void NEAR ListView_GetRects(LV* plv, int iItem,
        RECT FAR* prcIcon, RECT FAR* prcLabel, RECT FAR* prcBounds,
        RECT FAR* prcSelectBounds)
{
    Assert(plv);

    if (ListView_IsReportView(plv)) {
        ListView_RGetRects(plv, iItem, prcIcon, prcLabel, prcBounds,
                prcSelectBounds);
    } else if (ListView_IsListView(plv)) {
        ListView_LGetRects(plv, iItem, prcIcon, prcLabel, prcBounds,
                prcSelectBounds);
    } else {
        LISTITEM FAR *pitem = ListView_FastGetItemPtr(plv, iItem);

        _ListView_GetRectsFromItem(plv, ListView_IsSmallView(plv), pitem,
                                   prcIcon, prcLabel, prcBounds, prcSelectBounds);
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
void NEAR ListView_InvalidateItem(LV* plv, int iItem, BOOL fSelectionOnly, UINT fRedraw)
{
    RECT rcBounds, rcSelectBounds;
    LPRECT prcToUse = fSelectionOnly ? &rcSelectBounds : &rcBounds;

    if (ListView_RedrawEnabled(plv)) {
        ListView_GetRects(plv, iItem, NULL, NULL, &rcBounds, &rcSelectBounds);

        if( RECTS_IN_SIZE( plv->sizeClient, *prcToUse ) )
            RedrawWindow(plv->hwnd, prcToUse, NULL, fRedraw);
    } else {
        // if we're not visible, we'll get a full
        // erase bk when we do become visible, so only do this stuff when
        // we're on setredraw false
        if (!(plv->flags & LVF_REDRAW)) {
            // if we're invalidating that's new (thus hasn't been painted yet)
            // blow it off
            if (plv->iFirstChangedNoRedraw != -1 &&
                iItem >= plv->iFirstChangedNoRedraw)
                return;


            ListView_GetRects(plv, iItem, NULL, NULL, &rcBounds, &rcSelectBounds);

            // if it had the erase bit, add it to our region
            if (RECTS_IN_SIZE(plv->sizeClient, *prcToUse)) {
                HRGN hrgn = CreateRectRgnIndirect(prcToUse);
                ListView_InvalidateRegion(plv, hrgn);
                if (fRedraw & RDW_ERASE)
                    plv->flags |= LVF_ERASE;
            }
        }
    }
}

BOOL NEAR ListView_OnSetItemPosition(LV* plv, int i, int x, int y)
{
    LISTITEM FAR* pitem;

    if (ListView_IsListView(plv))
        return FALSE;

    if (plv->style & LVS_NOITEMDATA)
    {
        Assert(FALSE);
        return(FALSE);
    }

    pitem = ListView_GetItemPtr(plv, i);
    if (!pitem)
        return FALSE;

    // erase old

    // Don't invalidate if it hasn't got a position yet
    if (pitem->pt.y != RECOMPUTE)
        ListView_InvalidateItem(plv, i, FALSE, RDW_INVALIDATE | RDW_ERASE);

    pitem->pt.x = x;
    pitem->pt.y = y;

    plv->rcView.left = RECOMPUTE;

    // and draw at new position

    ListView_InvalidateItem(plv, i, FALSE, RDW_INVALIDATE);

    // If autoarrange is turned on, do it now...
    if (ListView_RedrawEnabled(plv)) {
        if (plv->style & LVS_AUTOARRANGE)
            ListView_OnArrange(plv, LVA_DEFAULT);
        else
            ListView_UpdateScrollBars(plv);
    }

    return TRUE;
}

BOOL NEAR ListView_OnGetItemPosition(LV* plv, int i, POINT FAR* ppt)
{
    LISTITEM FAR* pitem;

    Assert(ppt);

    //
    // This needs to handle all views as it is used to figure out
    // where the item is during drag and drop and the like
    //
    if (ListView_IsListView(plv) || ListView_IsReportView(plv))
    {
        RECT rcIcon;
        ListView_GetRects(plv, i, &rcIcon, NULL, NULL, NULL);
        ppt->x = rcIcon.left;
        ppt->y = rcIcon.top;

    } else {

        if (plv->style & LVS_NOITEMDATA)
        {
            Assert(FALSE);
            return(FALSE);
        }
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
    Assert(ppt);

    if (ListView_IsListView(plv) || ListView_IsReportView(plv))
        return FALSE;

    *ppt = plv->ptOrigin;
    return TRUE;
}



int NEAR ListView_OnGetStringWidth(LV* plv, LPCSTR psz)
{
    HDC hdc;
    SIZE siz;

    if (!psz)
        return 0;

    hdc = GetDC(plv->hwnd);
    SelectFont(hdc, plv->hfontLabel);
    GetTextExtentPoint(hdc, psz, lstrlen(psz), &siz);
    ReleaseDC(plv->hwnd, hdc);

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
        if (iCol != 0)
            return FALSE;

        if (plv->cxItem != cx)
        {
            // REVIEW: Should optimize what gets invalidated here...

            //int iCol = plv->xOrigin / plv->cxItem;

            plv->cxItem = cx;
            //plv->xOrigin = iCol * cx;
            if (fExplicit)
                plv->flags |= LVF_COLSIZESET;   // Set the fact that we explictly set size!.

            RedrawWindow(plv->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
            ListView_UpdateScrollBars(plv);
        }
        return TRUE;
    }
    else if (ListView_IsReportView(plv))
    {
        return ListView_RSetColumnWidth(plv, iCol, cx);
    }
    return FALSE;
}

void NEAR ListView_Redraw(LV* plv, HDC hdc, RECT FAR* prcClip)
{
    int i;
    UINT flags;
    int cItem = ListView_Count(plv);
    DWORD dwType = plv->style & LVS_TYPEMASK;

    SetBkMode(hdc, TRANSPARENT);
    SelectFont(hdc, plv->hfontLabel);

    //
    // For list view and report view, we can save a lot of time
    // by calculating the index of the first item that may need
    // painting...
    //

    switch (dwType) {
        case LVS_REPORT:
            i = ListView_RYHitTest(plv, prcClip->top);
            break;

        case LVS_LIST:
            i = ListView_LItemHitTest(plv, prcClip->left, prcClip->top, &flags);
            break;

        default:
            i = 0;  // Icon views no such hint
            // REVIEW: we can keep a flag which tracks whether the view is
            // presently in pre-arranged order and bypass Zorder when it is
    }

    if (i < 0)
        i = 0;

    for (; i < cItem; i++)
    {
        int i2;

        if (dwType == LVS_ICON || dwType == LVS_SMALLICON)
        {
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
            i2 = (int)(DWORD)DPA_FastGetPtr(plv->hdpaZOrder, (cItem - 1) -i);

        } else
            i2 = i;

        plv->iItemDrawing = i2;

        if (!ListView_DrawItem(plv, i2, hdc, NULL, prcClip, 0))
            break;
    }
    plv->iItemDrawing = -1;
}

BOOL NEAR ListView_DrawItem(LV* plv, int i, HDC hdc, LPPOINT lpptOrg, RECT FAR* prcClip, UINT flags)
{
    LISTITEM FAR* pitem = ListView_FastGetItemPtr(plv, i);

    if (!(flags & LVDI_NOWAYFOCUS))
    {
        if ((plv->flags & LVF_FOCUSED) || plv->hwndEdit)
            flags |= (LVDI_FOCUS | LVDI_SELECTED);
        else if (plv->style & LVS_SHOWSELALWAYS)
            flags |= LVDI_SELECTED;
    }

    if (ListView_IsReportView(plv))
    {
        return ListView_RDrawItem(plv, i, pitem, hdc, lpptOrg, prcClip, flags);
    }
    else if (ListView_IsListView(plv))
    {
        ListView_LDrawItem(plv, i, pitem, hdc, lpptOrg, prcClip, flags);
    }
    else if (ListView_IsSmallView(plv))
    {
        ListView_SDrawItem(plv, i, hdc, lpptOrg, prcClip, flags);
    }
    else
    {
        ListView_IDrawItem(plv, i, hdc, lpptOrg, prcClip, flags);
    }

    return TRUE;
}

// NOTE: this function requires a properly selected font.
//
void WINAPI SHDrawText(HDC hdc, LPCSTR pszText, RECT FAR* prc, int fmt,
        UINT flags, int cyChar, int cxEllipses, COLORREF clrText, COLORREF clrTextBk)
{
    int cchText;
    COLORREF clrSave, clrSaveBk;
    RECT rc;
    UINT uETOFlags = 0;
    char ach[CCHLABELMAX + CCHELLIPSES];

    // REVIEW: Performance idea:
    // We could cache the currently selected text color
    // so we don't have to set and restore it each time
    // when the color is the same.
    //
    if (!pszText)
        return;

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

    if ((flags & SHDT_ELLIPSES) &&
            ListView_NeedsEllipses(hdc, pszText, &rc, &cchText, cxEllipses))
    {
        hmemcpy(ach, pszText, cchText);
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

    if (flags & SHDT_TRANSPARENT)
    clrSave = SetTextColor(hdc, 0x000000);
    else
    {
        uETOFlags |= ETO_OPAQUE;

    if (flags & SHDT_SELECTED)
    {
            clrSave = SetTextColor(hdc, g_clrHighlightText);
            clrSaveBk = SetBkColor(hdc, g_clrHighlight);

            if( flags & SHDT_DRAWTEXT )
            {
                FillRect(hdc, prc, g_hbrHighlight);
            }
    }
    else
    {
        if (clrText == CLR_DEFAULT && clrTextBk == CLR_DEFAULT)
        {
        clrSave = SetTextColor(hdc, g_clrWindowText);
                clrSaveBk = SetBkColor(hdc, g_clrWindow);

                if( ( flags & (SHDT_DRAWTEXT | SHDT_DESELECTED) ) ==
                        (SHDT_DRAWTEXT | SHDT_DESELECTED) )
                {
                    FillRect(hdc, prc, g_hbrWindow);
                }
        }
        else
        {
            HBRUSH hbr;
                if (clrText == CLR_DEFAULT)
                    clrText =  g_clrWindowText;

                if (clrTextBk == CLR_DEFAULT)
                    clrTextBk = g_clrWindow;

        clrSave = SetTextColor(hdc, clrText);
        clrSaveBk = SetBkColor(hdc, clrTextBk);

                if( ( flags & (SHDT_DRAWTEXT | SHDT_DESELECTED) ) ==
                        (SHDT_DRAWTEXT | SHDT_DESELECTED) )
                {
                    hbr = CreateSolidBrush(GetNearestColor(hdc, clrTextBk));
            if (hbr)
                    {
                        FillRect(hdc, prc, hbr);
                        DeleteObject(hbr);
                    }
                    else
                        FillRect(hdc, prc, GetStockObject( WHITE_BRUSH ) );
                }
        }
    }
    }

    // If we want the item to display as if it was depressed, we will
    // offset the text rectangle down and to the left
    if (flags & SHDT_DEPRESSED)
        OffsetRect(&rc, g_cxBorder, g_cyBorder);

    if (flags & SHDT_DRAWTEXT)
    {
#ifdef WIN32
        UINT uDTFlags = DT_LVWRAP | DT_WORD_ELLIPSIS;
#else
        UINT uDTFlags = DT_LVWRAP;
#endif

        if( !( flags & SHDT_CLIPPED ) )
            uDTFlags |= DT_NOCLIP;

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

        if( flags & SHDT_CLIPPED )
           uETOFlags |= ETO_CLIPPED;

        ExtTextOut(hdc, rc.left, rc.top, uETOFlags, prc, pszText, cchText, NULL);
    }

    if (flags & (SHDT_SELECTED | SHDT_DESELECTED | SHDT_TRANSPARENT))
    {
        SetTextColor(hdc, clrSave);
        if (! (flags & SHDT_TRANSPARENT))
            SetBkColor(hdc, clrSaveBk);
    }
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
    HWND hwndLV = plv->hwnd;
    RECT rcBounds, rcImage;
    HDC hdcMem = NULL;
    HBITMAP hbmImage = NULL;
    HBITMAP hbmMask = NULL;
    HBITMAP hbmOld;
    HIMAGELIST himl = NULL;
    int dx, dy;
    HIMAGELIST himlSrc;
    LV_ITEM item;
    POINT ptOrg;

    ListView_GetRects(plv, iItem, &rcImage, NULL, NULL, &rcBounds);

    if (ListView_IsIconView(plv))
        InflateRect(&rcImage, -g_cxIconMargin, -g_cyIconMargin);

    ptOrg.x = 0;
    // chop off any extra filler above icon
    ptOrg.y = rcBounds.top - rcImage.top;
    dx = rcBounds.right - rcBounds.left;
    dy = rcBounds.bottom - rcBounds.top + ptOrg.y;

    lpptUpLeft->x = rcBounds.left - ptOrg.x;
    lpptUpLeft->y = rcBounds.top - ptOrg.y;

    if (!(hdcMem = CreateCompatibleDC(NULL)))
    goto CDI_Exit;
    if (!(hbmImage = CreateColorBitmap(dx, dy)))
    goto CDI_Exit;
    if (!(hbmMask = CreateMonoBitmap(dx, dy)))
    goto CDI_Exit;

    // prepare for drawing the item
    SelectObject(hdcMem, plv->hfontLabel);
    SetBkMode(hdcMem, TRANSPARENT);

    /*
    ** draw the text to both bitmaps
    */
    hbmOld = SelectObject(hdcMem, hbmImage);
    // fill image with black for transparency
    PatBlt(hdcMem, 0, 0, dx, dy, BLACKNESS);
    ListView_DrawItem(plv, iItem, hdcMem, &ptOrg, NULL,
                LVDI_NOIMAGE | LVDI_TRANSTEXT | LVDI_NOWAYFOCUS);

    SelectObject(hdcMem, hbmMask);
    // fill mask with white for transparency
    PatBlt(hdcMem, 0, 0, dx, dy, WHITENESS);
    ListView_DrawItem(plv, iItem, hdcMem, &ptOrg, NULL,
                LVDI_NOIMAGE | LVDI_TRANSTEXT | LVDI_NOWAYFOCUS);

    // unselect objects that we used
    SelectObject(hdcMem, hbmOld);
    SelectObject(hdcMem, g_hfontSystem);

    /*
    ** make an image list that for now only has the text
    */

    //
    // BUGBUG: To fix a pri-1 M7 bug, we create a shared image list.
    //
    if (!(himl = ImageList_Create(dx, dy, ILC_MASK, 1, 0)))
    goto CDI_Exit;
    ImageList_SetBkColor(himl, CLR_NONE);
    ImageList_Add(himl, hbmImage, hbmMask);

    /*
    ** make a dithered copy of the image part onto our bitmaps
    ** (need both bitmap and mask to be dithered)
    */
    himlSrc = ListView_OnGetImageList(plv, !(ListView_IsIconView(plv)));
    if (himlSrc)
    {
    item.iItem = iItem;
    item.iSubItem = 0;
    item.mask = LVIF_IMAGE |LVIF_STATE;
        item.stateMask = LVIS_OVERLAYMASK;
    ListView_OnGetItem(plv, &item);

    ImageList_CopyDitherImage(himl, 0, rcImage.left - rcBounds.left, 0, himlSrc, item.iImage, item.state & LVIS_OVERLAYMASK);
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

#endif //!WIN31 || IEWIN31_25
