#include "ctlspriv.h"

#define RECOMPUTE  32767

#ifndef TCS_FORCEICONLEFT
#define TCS_FORCEICONLEFT       0x0010  // 0nly for fixed width mode
#define TCS_FORCELABELLEFT      0x0020  // 0nly for fixed width mode
#endif

// tab control item structure

typedef struct { // ti
    RECT rc;	    // for hit testing and drawing
    int iImage;	    // image index
    int xLabel;	    // position of the text for drawing
    int yLabel;
    int xImage;	    // Position of the icon for drawing
    int yImage;
    int iRow;		// what row is it in?
    LPSTR pszText;
    union {
        LPARAM lParam;
        BYTE   abExtra[1];
#if defined(WINDOWS_ME)
	UINT etoRtlReading;
#endif
    };
} TABITEM, FAR *LPTABITEM;

typedef struct {
    HWND hwnd;          // window handle for this instance
    HWND hwndArrows;    // Hwnd Arrows.
    HDPA hdpa;          // item array structure
    UINT flags;		// TCF_ values (internal state bits)
    LONG style;
    int  cbExtra;       // extra bytes allocated for each item
    HFONT hfontLabel;   // font to use for labels
    int iSel;           // index of currently-focused item
    int iNewSel;        // index of next potential selection

    int cxItem;         // width of all tabs
    int cxMinTab;       // width of minimum tab
    int cyTabs;         // height of a row of tabs
    int cxTabs;     // The right hand edge where tabs can be painted.

    int cxyArrows;      // width and height to draw arrows
    int iFirstVisible;  // the index of the first visible item.
                        // wont fit and we need to scroll.
    int iLastVisible;   // Which one was the last one we displayed?

    int cxPad;           // Padding space between edges and text/image
    int cyPad;           // should be a multiple of c?Edge

    int iTabWidth;	// size of each tab in fixed width mode
    int iTabHeight;	// settable size of each tab
    int iLastRow;	// number of the last row.

    int cyText;         // where to put the text vertically
    int cyIcon;         // where to put the icon vertically

    HIMAGELIST himl;	// images,
    HWND hwndToolTips;
#ifdef  FE_IME
    HIMC hPrevImc;      // previous input context handle
#endif
} TC, NEAR *PTC;

#define HASIMAGE(ptc, pitem) (ptc->himl && pitem->iImage != -1)

// tab control flag values
#define TCF_FOCUSED     0x0001
#define TCF_MOUSEDOWN   0x0002
#define TCF_DRAWSUNKEN  0x0004
#define TCF_REDRAW      0x0010  /* Value from WM_SETREDRAW message */
#define TCF_BUTTONS	0x0020  /* draw using buttons instead of tabs */

#define ID_ARROWS       1

// Some helper macros for checking some of the flags...
#define Tab_RedrawEnabled(ptc)	    	(ptc->flags & TCF_REDRAW)
#define Tab_Count(ptc)              	DPA_GetPtrCount((ptc)->hdpa)
#define Tab_GetItemPtr(ptc, i)      	((LPTABITEM)DPA_GetPtr((ptc)->hdpa, (i)))
#define Tab_FastGetItemPtr(ptc, i)      ((LPTABITEM)DPA_FastGetPtr((ptc)->hdpa, (i)))

#define Tab_DrawButtons(ptc)		((BOOL)(ptc->style & TCS_BUTTONS))
#define Tab_MultiLine(ptc)		((BOOL)(ptc->style & TCS_MULTILINE))
#define Tab_RaggedRight(ptc)		((BOOL)(ptc->style & TCS_RAGGEDRIGHT))
#define Tab_FixedWidth(ptc)		((BOOL)(ptc->style & TCS_FIXEDWIDTH))
#define Tab_ForceLabelLeft(ptc)         ((BOOL)(ptc->style & TCS_FORCELABELLEFT))
#define Tab_ForceIconLeft(ptc)          ((BOOL)(ptc->style & TCS_FORCEICONLEFT))
#define Tab_FocusOnButtonDown(ptc)	((BOOL)(ptc->style & TCS_FOCUSONBUTTONDOWN))
#define Tab_OwnerDraw(ptc)		((BOOL)(ptc->style & TCS_OWNERDRAWFIXED))
#define Tab_FocusNever(ptc)		((BOOL)(ptc->style & TCS_FOCUSNEVER))

LRESULT CALLBACK Tab_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void NEAR PASCAL InvalidateItem(PTC ptc, int iItem, BOOL bErase);
void NEAR PASCAL CalcPaintMetrics(PTC ptc, HDC hdc);
void NEAR PASCAL Tab_OnHScroll(PTC ptc, HWND hwndCtl, UINT code, int pos);
BOOL NEAR Tab_FreeItem(PTC ptc, TABITEM FAR* pitem);
void NEAR Tab_UpdateArrows(PTC ptc, BOOL fSizeChanged);
int NEAR PASCAL ChangeSel(PTC ptc, int iNewSel,  BOOL bSendNotify);
BOOL NEAR PASCAL RedrawAll(PTC ptc, UINT uFlags);
BOOL FAR PASCAL Tab_Init(HINSTANCE hinst);
void NEAR PASCAL UpdateToolTipRects(PTC ptc);

#pragma code_seg(CODESEG_INIT)

BOOL FAR PASCAL Tab_Init(HINSTANCE hinst)
{
    WNDCLASS wc;

    if (!GetClassInfo(hinst, c_szTabControlClass, &wc)) {
#ifndef WIN32
	extern LRESULT CALLBACK _Tab_WndProc(HWND, UINT, WPARAM, LPARAM);
    	wc.lpfnWndProc     = _Tab_WndProc;
#else
    	wc.lpfnWndProc     = Tab_WndProc;
#endif

    	wc.hCursor         = LoadCursor(NULL, IDC_ARROW);
    	wc.hIcon           = NULL;
    	wc.lpszMenuName    = NULL;
    	wc.hInstance       = hinst;
    	wc.lpszClassName   = c_szTabControlClass;
    	wc.hbrBackground   = (HBRUSH)(COLOR_3DFACE + 1);
    	wc.style           = CS_GLOBALCLASS | CS_HREDRAW;
    	wc.cbWndExtra      = sizeof(PTC);
    	wc.cbClsExtra      = 0;

	return RegisterClass(&wc);
    }

    return TRUE;
}

#pragma code_seg()


void NEAR PASCAL Tab_Scroll(PTC ptc, int dx, int iNewFirstIndex)
{
    int i;
    int iMax;
    RECT rc;
    LPTABITEM pitem = NULL;

    // don't stomp on edge unless first item is selected
    rc.left = g_cxEdge;
    rc.right = ptc->cxTabs;   // Dont scroll beyond tabs.
    rc.top = 0;
    rc.bottom = ptc->cyTabs + 2 * g_cyEdge;  // Only scroll in the tab area

    // See if we can scroll the window...
    // DebugMsg(DM_TRACE, "Tab_Scroll dx=%d, iNew=%d\n\r", dx, iNewFirstIndex);
    ScrollWindowEx(ptc->hwnd, dx, 0, NULL, &rc,
            NULL, NULL, SW_INVALIDATE | SW_ERASE);

    // We also need to update the item rectangles and also
    // update the internal variables...
    iMax = Tab_Count(ptc) - 1;
    for (i = iMax; i >= 0; i--)
    {
        pitem = Tab_FastGetItemPtr(ptc, i);
        OffsetRect(&pitem->rc, dx, 0);
        pitem->xLabel += dx;       // also need to offset text
	pitem->xImage += dx;
    }

    // If the previously last visible item is not fully visible
    // now, we need to invalidate it also.
    //
    if (ptc->iLastVisible > iMax)
        ptc->iLastVisible = iMax;

    for (i = ptc->iLastVisible; i>= 0; i--)
    {
        pitem = Tab_GetItemPtr(ptc, i);
        if (pitem) {
            if (pitem->rc.right <= ptc->cxTabs)
                break;
            InvalidateItem(ptc, ptc->iLastVisible, TRUE);
        }
    }

    if ((i == ptc->iLastVisible) && pitem)
    {
        // The last previously visible item is still fully visible, so
        // we need to invalidate to the right of it as there may have been
        // room for a partial item before, that will now need to be drawn.
        rc.left = pitem->rc.right;
        InvalidateRect(ptc->hwnd, &rc, TRUE);
    }

    ptc->iFirstVisible = iNewFirstIndex;

    if (ptc->hwndArrows)
        SendMessage(ptc->hwndArrows, UDM_SETPOS, 0, MAKELPARAM(iNewFirstIndex, 0));

    UpdateToolTipRects(ptc);
}


void NEAR PASCAL Tab_OnHScroll(PTC ptc, HWND hwndCtl, UINT code, int pos)
{
    // Now process the Scroll messages
    if (code == SB_THUMBPOSITION)
    {
        //
        // For now lets simply try to set that item as the first one
        //
        {
            // If we got here we need to scroll
            LPTABITEM pitem = Tab_GetItemPtr(ptc, pos);
            int dx = 0;

            if (pitem)
                dx = -pitem->rc.left + g_cxEdge;

            if (dx || !pitem) {
                Tab_Scroll(ptc, dx, pos);
                UpdateWindow(ptc->hwnd);
            }
        }
    }
}

void NEAR Tab_OnSetRedraw(PTC ptc, BOOL fRedraw)
{
    if (fRedraw) {
	ptc->flags |= TCF_REDRAW;
    } else {
	ptc->flags &= ~TCF_REDRAW;
    }
}

void NEAR Tab_OnSetFont(PTC ptc, HFONT hfont, BOOL fRedraw)
{
    Assert(ptc);

    if (!hfont)
        return;

    if (hfont != ptc->hfontLabel)
    {
        ptc->hfontLabel = hfont;
	ptc->cxItem = ptc->cyTabs = RECOMPUTE;

        RedrawAll(ptc, RDW_INVALIDATE | RDW_ERASE);
    }
}


BOOL NEAR Tab_OnCreate(PTC ptc, CREATESTRUCT FAR* lpCreateStruct)
{
    HDC hdc;

    ptc->hdpa = DPA_Create(4);
    if (!ptc->hdpa)
        return FALSE;

    ptc->style = lpCreateStruct->style;

    // make sure we don't have invalid bits set
    if (!Tab_FixedWidth(ptc)) {
        ptc->style &= ~(TCS_FORCEICONLEFT | TCS_FORCELABELLEFT);
    }

    // make us always clip siblings
    SetWindowLong(ptc->hwnd, GWL_STYLE, WS_CLIPSIBLINGS | ptc->style);

    ptc->flags = TCF_REDRAW;	    // enable redraw
    ptc->cbExtra = sizeof(LPARAM);  // default extra size
    ptc->iSel = -1;
    ptc->cxItem = ptc->cyTabs = RECOMPUTE;
    ptc->cxPad = g_cxEdge * 3;
    ptc->cyPad = (g_cyEdge * 3/2);
    ptc->iFirstVisible = 0;
    ptc->hwndArrows = NULL;
    ptc->iLastRow = -1;
    ptc->iNewSel = -1;

    hdc = GetDC(NULL);
    ptc->iTabWidth = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);

#ifndef WIN31
    //BUGBUG remove this after move to commctrl
    InitDitherBrush();
#endif

    if (ptc->style & TCS_TOOLTIPS) {
        TOOLINFO ti;
        // don't bother setting the rect because we'll do it below
        // in FlushToolTipsMgr;
        ti.cbSize = sizeof(ti);
        ti.uFlags = TTF_IDISHWND;
        ti.hwnd = ptc->hwnd;
        ti.uId = (UINT)ptc->hwnd;
        ti.lpszText = 0;
        ptc->hwndToolTips = CreateWindow(c_szSToolTipsClass, "",	
                                              WS_POPUP,
                                              CW_USEDEFAULT, CW_USEDEFAULT,
                                              CW_USEDEFAULT, CW_USEDEFAULT,
                                              ptc->hwnd, NULL, HINST_THISDLL,
                                              NULL);
        if (ptc->hwndToolTips)
            SendMessage(ptc->hwndToolTips, TTM_ADDTOOL, 0,
                        (LPARAM)(LPTOOLINFO)&ti);
        else
            ptc->style &= ~(TCS_TOOLTIPS);
    }

#ifdef  FE_IME
    ptc->hPrevImc = ImmAssociateContext(ptc->hwnd, 0L);
#endif
    return TRUE;
}


void NEAR Tab_OnDestroy(PTC ptc)
{
    int i;

#ifdef  FE_IME
    ImmAssociateContext(ptc->hwnd, ptc->hPrevImc);
#endif
    for (i = 0; i < Tab_Count(ptc); i++)
        Tab_FreeItem(ptc, Tab_FastGetItemPtr(ptc, i));

    DPA_Destroy(ptc->hdpa);

    if (ptc) {
	SetWindowInt(ptc->hwnd, 0, 0);
	NearFree((HLOCAL)ptc);
    }

#ifndef WIN31
    //BUGBUG remove this after move to commctrl
    TerminateDitherBrush();
#endif
}

// returns true if it actually moved

BOOL NEAR PASCAL PutzRowToBottom(PTC ptc, int iRowMoving)
{
    int i;
    LPTABITEM pitem;
    int dy;

    if (iRowMoving == ptc->iLastRow)
        return FALSE; // already at the bottom;

    for (i = Tab_Count(ptc) -1 ;i >= 0; i--) {
	pitem = Tab_FastGetItemPtr(ptc, i);
	if (pitem->iRow > iRowMoving) {	
	    pitem->iRow--;
	    dy = -ptc->cyTabs;
	} else if (pitem->iRow == iRowMoving) {
	    dy = ptc->cyTabs * (ptc->iLastRow - iRowMoving);
	    pitem->iRow = ptc->iLastRow;

	} else
	    continue;

	pitem->yLabel += dy;
	pitem->yImage += dy;
	pitem->rc.top += dy;
	pitem->rc.bottom += dy;
    }
    UpdateToolTipRects(ptc);
    return TRUE;
}

#define BADNESS(ptc, i) (ptc->cxTabs - Tab_FastGetItemPtr(ptc, i)->rc.right)
// borrow one tab from the prevous row
BOOL NEAR PASCAL BorrowOne(PTC ptc, int iCurLast, int iPrevLast, int iBorrow)
{
    LPTABITEM pitem, pitem2;
    int i;
    int dx;

    // is there room to move the prev item? (might now be if iPrev is huge)
    pitem = Tab_FastGetItemPtr(ptc, iPrevLast);
    pitem2 = Tab_FastGetItemPtr(ptc, iCurLast);

    // if the size of the item is greaterthan the badness
    if (BADNESS(ptc, iCurLast) < (pitem->rc.right - pitem->rc.left))
        return FALSE;

    // otherwise do it.
    // move this one down
    dx = pitem->rc.left - Tab_FastGetItemPtr(ptc, iPrevLast + 1)->rc.left;
    pitem->rc.left -= dx;
    pitem->rc.right -= dx;
    pitem->rc.top = pitem2->rc.top;
    pitem->rc.bottom = pitem2->rc.bottom;
    pitem->xLabel -= dx;
    pitem->xImage -= dx;
    pitem->yLabel = pitem2->yLabel;
    pitem->yImage = pitem2->yImage;
    pitem->iRow = pitem2->iRow;

    // and move all the others over.
    dx = pitem->rc.right - pitem->rc.left;
    for(i = iPrevLast + 1 ; i <= iCurLast ; i++ ) {
        pitem = Tab_FastGetItemPtr(ptc, i);
        pitem->rc.left += dx;
        pitem->rc.right += dx;
        pitem->xLabel += dx;
        pitem->xImage += dx;
    }

    if (iBorrow) {
        if (pitem->iRow > 1) {

            // borrow one from the next row up.
            // setup the new iCurLast as the one right before the one we moved
            // (the one we moved is now the current row's first
            // and hunt backwards until we find an iPrevLast
            iCurLast = iPrevLast - 1;
            while (iPrevLast-- &&
                   Tab_FastGetItemPtr(ptc, iPrevLast)->iRow == (pitem->iRow - 1))
            {
                if (iPrevLast <= 0)
                {
                    // sanity check
                    return FALSE;
                }
            }
            return BorrowOne(ptc, iCurLast, iPrevLast, iBorrow - 1 );
        } else
            return FALSE;

    }
    return TRUE;
}


// fill last row will fiddle around borrowing from the previous row(s)
// to keep from having huge huge bottom tabs
void NEAR PASCAL FillLastRow(PTC ptc)
{
    int hspace;
    int cItems = Tab_Count(ptc);
    int iPrevLast;
    int iBorrow = 0;

    // if no items or one row
    if (!cItems)
        return;


    for (iPrevLast = cItems - 2;
         Tab_FastGetItemPtr(ptc, iPrevLast)->iRow == ptc->iLastRow;
         iPrevLast--)
    {
        // sanity check
        if (iPrevLast <= 0)
        {
            Assert(FALSE);
            return;
        }
    }

    while (iPrevLast &&  (hspace = BADNESS(ptc, cItems-1)) &&
           (hspace > ((ptc->cxTabs/8) + BADNESS(ptc, iPrevLast))))
    {
        // if borrow fails, bail
        if (!BorrowOne(ptc, cItems - 1, iPrevLast, iBorrow++))
            return;
        iPrevLast--;
    }
}

void NEAR PASCAL RightJustify(PTC ptc)
{
    int i;
    LPTABITEM pitem;
    int j;
    int cItems = Tab_Count(ptc);
    int hspace, dwidth, dremainder, moved;

    // don't justify if only one row
    if (ptc->iLastRow < 1)
        return;

    FillLastRow(ptc);

    for ( i = 0; i < cItems; i++ ) {
	int iRow;
	pitem = Tab_FastGetItemPtr(ptc, i) ;
	iRow = pitem->iRow;

	// find the last item in this row
	for( j = i ; j < cItems; j++) {
	    if(Tab_FastGetItemPtr(ptc, j)->iRow != iRow)
		break;
	}

	// how much to fill
	hspace = ptc->cxTabs - Tab_FastGetItemPtr(ptc, j-1)->rc.right - g_cxEdge;
	dwidth = hspace/(j-i);  // amount to increase each by.
	dremainder =  hspace % (j-i); // the remnants
	moved = 0;  // how much we've moved already

	for( ; i < j ; i++ ) {
	    int iHalf = dwidth/2;
	    pitem = Tab_FastGetItemPtr(ptc, i);
	    pitem->rc.left += moved;
	    pitem->xLabel += moved + iHalf;
	    pitem->xImage += moved + iHalf;
	    moved += dwidth + (dremainder ? 1 : 0);
	    if ( dremainder )  dremainder--;
	    pitem->rc.right += moved;
	}
	i--; //dec because the outter forloop incs again.
    }
}

BOOL NEAR Tab_OnDeleteAllItems(PTC ptc)
{
    int i;

    for (i = Tab_Count(ptc); i-- > 0; i) {
        if(ptc->hwndToolTips) {
            TOOLINFO ti;
            ti.cbSize = sizeof(ti);
            ti.hwnd = ptc->hwnd;
            ti.uId = i;
            SendMessage(ptc->hwndToolTips, TTM_DELTOOL, 0, 
                        (LPARAM)(LPTOOLINFO)&ti);
        }
        Tab_FreeItem(ptc, Tab_FastGetItemPtr(ptc, i));
    }

    DPA_DeleteAllPtrs(ptc->hdpa);

    ptc->cxItem = RECOMPUTE;	// force recomputing of all tabs
    ptc->iSel = -1;

    RedrawAll(ptc, RDW_INVALIDATE | RDW_ERASE);
    return TRUE;
}

BOOL NEAR Tab_OnSetItemExtra(PTC ptc, int cbExtra)
{
    if (Tab_Count(ptc) >0 || cbExtra<0)
        return FALSE;

    ptc->cbExtra = cbExtra;

    return TRUE;
}

BOOL NEAR Tab_OnSetItem(PTC ptc, int iItem, const TC_ITEM FAR* ptci)
{
    TABITEM FAR* pitem;
    UINT mask;
    BOOL fRedraw = FALSE;

    mask = ptci->mask;
    if (!mask)
        return TRUE;

    pitem = Tab_GetItemPtr(ptc, iItem);
    if (!pitem)
        return FALSE;

    if (mask & TCIF_TEXT)
    {
        if (!Str_Set(&pitem->pszText, ptci->pszText))
            return FALSE;
        fRedraw = TRUE;
#if defined(WINDOWS_ME)
		pitem->etoRtlReading = (mask & TCIF_RTLREADING) ?ETO_RTLREADING :0;
#endif
    }

    if (mask & TCIF_IMAGE) {
        pitem->iImage = ptci->iImage;
        fRedraw = TRUE;
    }

    if ((mask & TCIF_PARAM) && ptc->cbExtra)
    {
        hmemcpy(pitem->abExtra, &ptci->lParam, ptc->cbExtra);
    }

    if (fRedraw) {
        if (Tab_FixedWidth(ptc)) {
            InvalidateItem(ptc, iItem, FALSE);
        } else {
            ptc->cxItem = ptc->cyTabs = RECOMPUTE;
            RedrawAll(ptc, RDW_INVALIDATE | RDW_NOCHILDREN | RDW_ERASE);
        }
    }
    return TRUE;
}

void NEAR PASCAL Tab_OnMouseMove(PTC ptc, WPARAM fwKeys, int x, int y)
{
    POINT pt={x,y};
    if (fwKeys & MK_LBUTTON && Tab_DrawButtons(ptc)) {

	LPTABITEM pitem = Tab_GetItemPtr(ptc, ptc->iNewSel);

        if (pitem == NULL)
            return;     // nothing to select (empty case)

	if (PtInRect(&pitem->rc, pt)) {
	    if(ptc->flags & TCF_DRAWSUNKEN) {
                // already sunken.. do nothing
                return;
	    }
	} else {
	    if( !(ptc->flags & TCF_DRAWSUNKEN)) {
                // already un-sunken... do nothing
                return;
	    }
	}

        // if got here, then toggle flag
        ptc->flags ^=  TCF_DRAWSUNKEN;
        InvalidateItem(ptc, ptc->iNewSel, FALSE);
    }
}

void NEAR PASCAL Tab_OnButtonUp(PTC ptc, int x, int y, BOOL fNotify)
{
    POINT pt={x,y};

    BOOL fAllow = TRUE;

    if (fNotify)
        fAllow = !SendNotify(GetParent(ptc->hwnd), ptc->hwnd, NM_CLICK, NULL);

    if (ptc->flags & TCF_DRAWSUNKEN) {
	LPTABITEM pitem = Tab_GetItemPtr(ptc, ptc->iNewSel);

        // nothing selected (its empty)
        // only do this if something is selected...  
        // otherwise we still do need to go below and release capture though
        if (pitem) {
            if (PtInRect(&pitem->rc, pt)) {
                int iNewSel = ptc->iNewSel;
                // use iNewSel instead of ptc->iNewSel because the SendNotify could have nuked us
                if (fAllow)
                    ChangeSel(ptc, iNewSel, TRUE);
            } else {
                InvalidateItem(ptc, ptc->iNewSel, FALSE);
            }
            ptc->iNewSel = -1;

            ptc->flags &= ~TCF_DRAWSUNKEN;
        }
    }

    // don't worry about checking DrawButtons because TCF_MOUSEDOWN
    // wouldn't be set otherwise.
    if (ptc->flags & TCF_MOUSEDOWN) {
        int iOldSel = ptc->iNewSel;
        ptc->flags &= ~TCF_MOUSEDOWN; // do this before release  to avoid reentry
        ptc->iNewSel = -1;
        InvalidateItem(ptc, iOldSel, FALSE);
	ReleaseCapture();	
    }
    

}

int NEAR Tab_OnHitTest(PTC ptc, int x, int y, UINT FAR *lpuFlags)
{
    int i;
    int iLast = Tab_Count(ptc);
    POINT pt = {x,y};
    UINT uTemp;

    if (!lpuFlags) lpuFlags = &uTemp;

    for (i = 0; i < iLast; i++) {
	LPTABITEM pitem = Tab_FastGetItemPtr(ptc, i);
	if (PtInRect(&pitem->rc, pt)) {
            if (Tab_OwnerDraw(ptc)) {
                *lpuFlags = TCHT_ONITEM;
            } else if (HASIMAGE(ptc, pitem)) {
                if (x > pitem->xImage || x < pitem->xLabel)
                    *lpuFlags = TCHT_ONITEMICON;
            } else if (x > pitem->xLabel) {
                *lpuFlags = TCHT_ONITEMLABEL;
            } else
                *lpuFlags = TCHT_ONITEM;
            return i;
        }
    }
    *lpuFlags = TCHT_NOWHERE;
    return -1;
}


void NEAR Tab_OnButtonDown(PTC ptc, int x, int y)
{
    int i;
    int iOldSel = -1;

    if (x > ptc->cxTabs)
        return;     // outside the range of the visible tabs

    i = Tab_OnHitTest(ptc, x,y, NULL);

    if (i != -1) {
        iOldSel = ptc->iSel;

        if ((!Tab_FocusNever(ptc))
            && Tab_FocusOnButtonDown(ptc))
        {
            SetFocus(ptc->hwnd);
        }

        if (Tab_DrawButtons(ptc)) {
            ptc->iNewSel = i;
            ptc->flags |= (TCF_DRAWSUNKEN|TCF_MOUSEDOWN);
            SetCapture(ptc->hwnd);
            InvalidateItem(ptc, i, FALSE);
        } else {
            iOldSel = ChangeSel(ptc, i, TRUE);
        }
    }

    if ((!Tab_FocusNever(ptc)) &&
        (iOldSel == i))  // reselect current selection
        // this also catches i == -1 because iOldSel started as -1
    {
        SetFocus(ptc->hwnd);
        UpdateWindow(ptc->hwnd);
    }
}


TABITEM FAR* NEAR Tab_CreateItem(PTC ptc, const TC_ITEM FAR* ptci)
{
    TABITEM FAR* pitem;

    if (pitem = Alloc(sizeof(TABITEM)-sizeof(LPARAM)+ptc->cbExtra))
    {
        if (ptci->mask & TCIF_IMAGE)
            pitem->iImage = ptci->iImage;
        else
            pitem->iImage = -1;

	pitem->xLabel = pitem->yLabel = RECOMPUTE;

        // If specified, copy extra block of memory.
        if (ptci->mask & TCIF_PARAM) {
            if (ptc->cbExtra) {
                hmemcpy(pitem->abExtra, &ptci->lParam, ptc->cbExtra);
            }
        }

        if (ptci->mask & TCIF_TEXT)  {
            if (!Str_Set(&pitem->pszText, ptci->pszText))
            {
                Tab_FreeItem(ptc, pitem);
                return NULL;
            }
#if defined(WINDOWS_ME)
		pitem->etoRtlReading = (ptci->mask & TCIF_RTLREADING) ?ETO_RTLREADING :0;
#endif
        }
    }
    return pitem;
}


void NEAR Tab_UpdateArrows(PTC ptc, BOOL fSizeChanged)
{
    RECT rc;
    BOOL fArrow;

    GetClientRect(ptc->hwnd, &rc);

    if (IsRectEmpty(&rc))
        return;     // Nothing to do yet!

    // See if all of the tabs will fit.
    ptc->cxTabs = rc.right;     // Assume can use whole area to paint

    if (Tab_MultiLine(ptc))
        fArrow = FALSE;
    else {
        CalcPaintMetrics(ptc, NULL);
        fArrow = (ptc->cxItem >= rc.right);
    }

    if (!fArrow)
    {
        // Don't need arrows
        if (ptc->hwndArrows)
        {
            ShowWindow(ptc->hwndArrows, SW_HIDE);
            // BUGBUG:: This is overkill should only invalidate portion
            // that may be impacted, like the last displayed item..
            InvalidateRect(ptc->hwnd, NULL, TRUE);
        }
        if (ptc->iFirstVisible > 0) {
#ifdef DEBUG 
            if (!ptc->hwndArrows) {
                DebugMsg(DM_TRACE, "Scrolling where we wouldnt' have scrolled before");
            }
#endif
            Tab_OnHScroll(ptc, NULL, SB_THUMBPOSITION, 0);
            // BUGBUG:: This is overkill should only invalidate portion
            // that may be impacted, like the last displayed item..
            InvalidateRect(ptc->hwnd, NULL, TRUE);
        }
    }
    else
    {
        int cx;
        int cy;
        int iMaxBtnVal;
        int xSum;
        TABITEM FAR * pitem;


        // We need the buttons as not all of the items will fit
        // BUGBUG:: Should handle big ones...
#if 0
        cx = g_cxVScroll;
        cy = g_cyHScroll;
#else
        cy = ptc->cxyArrows;
        cx = cy * 2;
#endif
        ptc->cxTabs = rc.right - cx;   // Make buttons square

        // Setup what is the range for the buttons.
        xSum = 0;
        for (iMaxBtnVal=0; (ptc->cxTabs + xSum) < ptc->cxItem; iMaxBtnVal++)
        {
            pitem = Tab_GetItemPtr(ptc, iMaxBtnVal);
            if (!pitem)
                break;
            xSum += pitem->rc.right - pitem->rc.left;

        }

        // DebugMsg(DM_TRACE, "Tabs_UpdateArrows iMax=%d\n\r", iMaxBtnVal);
        if (ptc->hwndArrows)
        {
            if (fSizeChanged || !IsWindowVisible(ptc->hwndArrows))
                SetWindowPos(ptc->hwndArrows, NULL,
                             rc.right - cx, ptc->cyTabs - cy, cx, cy,
                             SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);
            // Make sure the range is set
            SendMessage(ptc->hwndArrows, UDM_SETRANGE, 0,
                    MAKELPARAM(iMaxBtnVal, 0));

        }
        else
        {
            InvalidateRect(ptc->hwnd, NULL, TRUE);
            ptc->hwndArrows = CreateUpDownControl(
                    UDS_HORZ | WS_CHILD | WS_VISIBLE,
                    rc.right - cx, ptc->cyTabs - cy, cx, cy,
                    ptc->hwnd, 1, HINST_THISDLL, NULL, iMaxBtnVal, 0,
                    ptc->iFirstVisible);
        }

    }
}

int NEAR Tab_OnInsertItem(PTC ptc, int iItem, const TC_ITEM FAR* ptci)
{
    TABITEM FAR* pitem;
    int i;

    pitem = Tab_CreateItem(ptc, ptci);
    if (!pitem)
        return -1;

    i = iItem;

    i = DPA_InsertPtr(ptc->hdpa, i, pitem);
    if (i == -1)
    {
        Tab_FreeItem(ptc, pitem);
        return -1;
    }

    if (ptc->iSel < 0)
	ptc->iSel = i;
    else if (ptc->iSel >= i)
	ptc->iSel++;

    if (ptc->iFirstVisible > i)
        ptc->iFirstVisible++;

    ptc->cxItem = RECOMPUTE;	// force recomputing of all tabs

    //Add tab to tooltips..  calculate the rect later
    if(ptc->hwndToolTips) {
        TOOLINFO ti;
        // don't bother setting the rect because we'll do it below
        // in FlushToolTipsMgr;
        ti.cbSize = sizeof(ti);
#ifdef WINDOWS_ME
        ti.uFlags = ptci->mask & TCIF_RTLREADING ?TTF_RTLREADING :0;
#else
        ti.uFlags = 0;
#endif
        ti.hwnd = ptc->hwnd;
        ti.uId = Tab_Count(ptc) - 1 ;
        ti.lpszText = LPSTR_TEXTCALLBACK;
        SendMessage(ptc->hwndToolTips, TTM_ADDTOOL, 0,
                    (LPARAM)(LPTOOLINFO)&ti);
    }

    if (Tab_RedrawEnabled(ptc)) {
        RECT rcInval;
        LPTABITEM pitem;

        if (Tab_DrawButtons(ptc)) {

            if (Tab_FixedWidth(ptc)) {

                CalcPaintMetrics(ptc, NULL);
                if (i == Tab_Count(ptc) - 1) {
                    InvalidateItem(ptc, i, FALSE);
                } else {
                    pitem = Tab_GetItemPtr(ptc, i);
                    GetClientRect(ptc->hwnd, &rcInval);

                    if (pitem) {
                        rcInval.top = pitem->rc.top;
                        if (ptc->iLastRow == 0) {
                            rcInval.left = pitem->rc.left;
                        }
                        Tab_UpdateArrows(ptc, FALSE);
                        RedrawWindow(ptc->hwnd, &rcInval, NULL, RDW_INVALIDATE |RDW_NOCHILDREN);
                    }
                }
                return i;
            }

        } else {

            // in tab mode Clear the selected item because it may move
            // and it sticks high a bit.
            if (ptc->iSel > i) {
                // update now because invalidate erases
                // and the redraw below doesn't.
                InvalidateItem(ptc, ptc->iSel, TRUE);
                UpdateWindow(ptc->hwnd);
            }
	}

        RedrawAll(ptc, RDW_INVALIDATE | RDW_NOCHILDREN);

    }

    return i;
}

// Add/remove/replace item

BOOL NEAR Tab_FreeItem(PTC ptc, TABITEM FAR* pitem)
{
    if (pitem)
    {
        Str_Set(&pitem->pszText, NULL);
        Free(pitem);
    }
    return FALSE;
}

void NEAR PASCAL Tab_OnRemoveImage(PTC ptc, int iItem)
{
    if (ptc->himl && iItem >= 0) {
        int i;
        LPTABITEM pitem;
#ifndef WIN31
        ImageList_Remove(ptc->himl, iItem);
#endif
        for( i = Tab_Count(ptc)-1 ; i >= 0; i-- ) {
            pitem = Tab_FastGetItemPtr(ptc, i);
            if (pitem->iImage > iItem)
                pitem->iImage--;
            else if (pitem->iImage == iItem) {
                pitem->iImage = -1; // if we now don't draw something, inval
                InvalidateItem(ptc, i, FALSE);
            }
        }
    }
}

BOOL NEAR Tab_OnDeleteItem(PTC ptc, int i)
{
    TABITEM FAR* pitem;
    UINT uRedraw;
    RECT rcInval;
    rcInval.left = -1; // special flag...

    if (i >= Tab_Count(ptc))
        return FALSE;

    if (!Tab_DrawButtons(ptc) && (Tab_RedrawEnabled(ptc) || ptc->iSel >= i)) {
	// in tab mode, Clear the selected item because it may move
	// and it sticks high a bit.
	InvalidateItem(ptc, ptc->iSel, TRUE);
    }

    // if its fixed width, don't need to erase everything, just the last one
    if (Tab_FixedWidth(ptc)) {
        int j;

        uRedraw = RDW_INVALIDATE | RDW_NOCHILDREN;
        j = Tab_Count(ptc) -1;
        InvalidateItem(ptc, j, TRUE);

        // update optimization
        if (Tab_DrawButtons(ptc)) {

            if (i == Tab_Count(ptc) - 1) {
                rcInval.left = 0;
                uRedraw = 0;
            } else {
                pitem = Tab_GetItemPtr(ptc, i);
                GetClientRect(ptc->hwnd, &rcInval);

                if (pitem) {
                    rcInval.top = pitem->rc.top;
                    if (ptc->iLastRow == 0) {
                        rcInval.left = pitem->rc.left;
                    }
                }
            }
        }

    } else {
        uRedraw = RDW_INVALIDATE | RDW_NOCHILDREN | RDW_ERASE;
    }
    pitem = DPA_DeletePtr(ptc->hdpa, i);
    if (!pitem)
        return FALSE;


    Tab_FreeItem(ptc, pitem);

    if (ptc->iSel == i)
        ptc->iSel = -1;       // deleted the focus item
    else if (ptc->iSel > i)
        ptc->iSel--;          // slide the foucs index down

    // maintain the first visible
    if (ptc->iFirstVisible > i)
        ptc->iFirstVisible--;

    ptc->cxItem = RECOMPUTE;	// force recomputing of all tabs
    if(ptc->hwndToolTips) {
        TOOLINFO ti;
        ti.cbSize = sizeof(ti);
        ti.hwnd = ptc->hwnd;
        ti.uId = Tab_Count(ptc) ;
	SendMessage(ptc->hwndToolTips, TTM_DELTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
    }

    if (uRedraw && !(uRedraw & RDW_ERASE))
        UpdateWindow(ptc->hwnd);

    if (Tab_RedrawEnabled(ptc)) {
        if (rcInval.left == -1) {
            RedrawAll(ptc, uRedraw);
        } else {

            Tab_UpdateArrows(ptc, FALSE);
            if (uRedraw)
                RedrawWindow(ptc->hwnd, &rcInval, NULL, uRedraw);
        }
    }

    return TRUE;
}



BOOL NEAR Tab_OnGetItem(PTC ptc, int iItem, TC_ITEM FAR* ptci)
{
    UINT mask = ptci->mask;
    const TABITEM FAR* pitem = Tab_GetItemPtr(ptc, iItem);

    if (!pitem)
    {
        // NULL init the the tci struct incase there is no pitem.
        // This is incase the dude calling doesn't check the return
        // from this function. Bug # 7105
        if (mask & TCIF_PARAM)
            ptci->lParam = 0;
        else if (mask & TCIF_TEXT)
            ptci->pszText = 0;
        else if (mask & TCIF_IMAGE)
            ptci->iImage = 0;

        return FALSE;
    }

    if (mask & TCIF_TEXT) {
        if (pitem->pszText)
            lstrcpyn(ptci->pszText, pitem->pszText, ptci->cchTextMax);
        else
            ptci->pszText = 0;
    }


    if ((mask & TCIF_PARAM) && ptc->cbExtra)
        hmemcpy(&ptci->lParam, pitem->abExtra, ptc->cbExtra);

    if (mask & TCIF_IMAGE)
        ptci->iImage = pitem->iImage;

    return TRUE;
}

void NEAR PASCAL InvalidateItem(PTC ptc, int iItem, BOOL bErase)
{
    LPTABITEM pitem = Tab_GetItemPtr(ptc, iItem);

    if (pitem) {
	RECT rc = pitem->rc;
        if (rc.right > ptc->cxTabs)
            rc.right = ptc->cxTabs;  // don't invalidate past our end
	InflateRect(&rc, g_cxEdge, g_cyEdge);
	InvalidateRect(ptc->hwnd, &rc, bErase);
    }
}

BOOL NEAR PASCAL RedrawAll(PTC ptc, UINT uFlags)
{
    if (Tab_RedrawEnabled(ptc)) {
        Tab_UpdateArrows(ptc, FALSE);
        RedrawWindow(ptc->hwnd, NULL, NULL, uFlags);
        return TRUE;
    }
    return FALSE;
}

int NEAR PASCAL ChangeSel(PTC ptc, int iNewSel,  BOOL bSendNotify)
{
    BOOL bErase;
    int iOldSel;
    HWND hwnd;

    if (iNewSel == ptc->iSel)
	return ptc->iSel;

    hwnd = ptc->hwnd;
    // make sure in range
    if (iNewSel < 0) {
        iOldSel = ptc->iSel;
        ptc->iSel = -1;
    } else if (iNewSel < Tab_Count(ptc)) {

	// make sure this is a change that's wanted
	if (bSendNotify)
	{
            if (SendNotify(GetParent(hwnd), hwnd, TCN_SELCHANGING, NULL))
		return ptc->iSel;
	}

	iOldSel = ptc->iSel;
	ptc->iSel = iNewSel;

        // See if we need to make sure the item is visible
        if (Tab_MultiLine(ptc)) {
	    if( !Tab_DrawButtons(ptc) && ptc->iLastRow > 0 && iNewSel != -1) {
		// In multiLineTab Mode bring the row to the bottom.
		if (PutzRowToBottom(ptc, Tab_FastGetItemPtr(ptc, iNewSel)->iRow))
                    RedrawAll(ptc, RDW_INVALIDATE | RDW_NOCHILDREN);
	    }	
	} else   {
	    // In single line mode, slide things over to  show selection
            RECT rcClient;
            int xOffset = 0;
            int iNewFirstVisible;
	    LPTABITEM pitem = Tab_GetItemPtr(ptc, iNewSel);
            Assert (pitem);
            if (!pitem)
                return -1;

            GetClientRect(ptc->hwnd, &rcClient);
            if (pitem->rc.left < g_cxEdge)
            {
                xOffset = -pitem->rc.left + g_cxEdge;        // Offset to get back to zero
                iNewFirstVisible = iNewSel;
            }
            else if ((iNewSel != ptc->iFirstVisible) &&
                    (pitem->rc.right > ptc->cxTabs))
            {
                // A little more tricky new to scroll each tab until we
                // fit on the end
                for (iNewFirstVisible = ptc->iFirstVisible;
                        iNewFirstVisible < iNewSel;)
                {
                    LPTABITEM pitemT = Tab_FastGetItemPtr(ptc, iNewFirstVisible);
                    xOffset -= (pitemT->rc.right - pitemT->rc.left);
                    iNewFirstVisible++;
                    if ((pitem->rc.right + xOffset) < ptc->cxTabs)
                        break;      // Found our new top index
                }
                // If we end up being the first item shown make sure our left
                // end is showing correctly
                if (iNewFirstVisible == iNewSel)
                    xOffset = -pitem->rc.left + g_cxEdge;
            }

            if (xOffset != 0)
	    {
                Tab_Scroll(ptc, xOffset, iNewFirstVisible);
            }
        }
    } else
        return -1;


    // repaint opt: we don't need to erase for buttons because their paint covers all.
    bErase = !Tab_DrawButtons(ptc);
    InvalidateItem(ptc, iOldSel, bErase);
    InvalidateItem(ptc, iNewSel, bErase);
    UpdateWindow(hwnd);

    // if they are buttons, we send the message on mouse up
    if (bSendNotify)
    {
        SendNotify(GetParent(hwnd), hwnd, TCN_SELCHANGE, NULL);
    }

    return iOldSel;
}



void NEAR PASCAL CalcTabHeight(PTC ptc, HDC hdc)
{
    BOOL bReleaseDC = FALSE;

    if (ptc->cyTabs == RECOMPUTE) {
	TEXTMETRIC tm;
    int iYExtra;
#ifndef WIN31
    int cx;
#endif
    int cy=0;

	if (!hdc)
	{
	    bReleaseDC = TRUE;
	    hdc = GetDC(NULL);
	    SelectObject(hdc, ptc->hfontLabel ? ptc->hfontLabel : g_hfontSystem);
	}

	GetTextMetrics(hdc, &tm);
        ptc->cxMinTab = tm.tmAveCharWidth * 6 + ptc->cxPad * 2;
        ptc->cxyArrows = tm.tmHeight + 2 * g_cyEdge;

#ifndef WIN31
        if (ptc->himl)
            ImageList_GetIconSize(ptc->himl, &cx, &cy);
#endif

        if (ptc->iTabHeight) {
            ptc->cyTabs = ptc->iTabHeight;
            if (Tab_DrawButtons(ptc))
                iYExtra = 3 * g_cyEdge; // (for the top edge, button edge and room to drop down)
            else
                iYExtra = 2 * g_cyEdge - 1;

        } else {

            // the height is the max of image or label plus padding.
            // where padding is 2*cypad-edge but at lease an edges
            iYExtra = ptc->cyPad*2;
            if (iYExtra < 2*g_cyEdge)
                iYExtra = 2*g_cyEdge;

            if (!Tab_DrawButtons(ptc))
                iYExtra -= (1 + g_cyEdge);

            // add an edge to the font height because we want a bit of
            // space under the text
            ptc->cyTabs = max(tm.tmHeight + g_cyEdge, cy) + iYExtra;
        }

        // add one so that if it's odd, we'll round up.
        ptc->cyText = (ptc->cyTabs - iYExtra - tm.tmHeight + 1) / 2;
        ptc->cyIcon = (ptc->cyTabs - iYExtra - cy) / 2;

	    if (bReleaseDC)
	    {
	        ReleaseDC(NULL, hdc);
	    }
    }
}

void NEAR PASCAL UpdateToolTipRects(PTC ptc)
{
    if(ptc->hwndToolTips) {
	int i;
	TOOLINFO ti;
        int iMax;
        LPTABITEM pitem;
	
        ti.cbSize = sizeof(ti);
#if defined(WINDOWS_ME)
	// bugbug: should this be rtlreading?
#endif
	ti.uFlags = 0;
	ti.hwnd = ptc->hwnd;
        ti.lpszText = LPSTR_TEXTCALLBACK;
	for ( i = 0, iMax = Tab_Count(ptc); i < iMax;  i++) {
            pitem = Tab_FastGetItemPtr(ptc, i);

	    ti.uId = i;
            ti.rect = pitem->rc;
	    SendMessage(ptc->hwndToolTips, TTM_NEWTOOLRECT, 0, (LPARAM)((LPTOOLINFO)&ti));
	}
    }
}

void PASCAL Tab_GetTextExtentPoint(HDC hdc, LPSTR lpszText, int iCount, LPSIZE lpsize)
{
    char szBuffer[128];

    if (iCount < sizeof(szBuffer)) {
        StripAccelerators(lpszText, szBuffer);
        lpszText = szBuffer;
        iCount = lstrlen(lpszText);
    }
    GetTextExtentPoint(hdc, lpszText, iCount, lpsize);
}

void NEAR PASCAL CalcPaintMetrics(PTC ptc, HDC hdc)
{
    SIZE siz;
    LPTABITEM pitem;
    int i, x, y;
    int xStart;
    int iRow = 0;
    int cItems = Tab_Count(ptc);
    BOOL bReleaseDC = FALSE;

    if (ptc->cxItem == RECOMPUTE)
    {
	    if (!hdc)
	    {
	        bReleaseDC = TRUE;
	        hdc = GetDC(NULL);
	        SelectObject(hdc, ptc->hfontLabel ? ptc->hfontLabel : g_hfontSystem);
	    }

	    CalcTabHeight(ptc, hdc);

        if (Tab_DrawButtons(ptc))
        {
            // start at the edge;
            xStart = 0;
            y = 0;
        }
        else
        {
            xStart = g_cxEdge;
            y = g_cyEdge;
        }
        x = xStart;

	    for (i = 0; i < cItems; i++)
        {
	        int cxImage = 0;
            int cxLabel = 0;
            pitem = Tab_FastGetItemPtr(ptc, i);

            if (pitem->pszText)
            {
                Tab_GetTextExtentPoint(hdc, pitem->pszText, lstrlen(pitem->pszText), &siz);
            }
            else
            {
                siz.cx = 0;
                siz.cy = 0;
            }

#ifndef WIN31
            // if there's an image, count that too
            if (HASIMAGE(ptc, pitem))
            {
                int cy;

                ImageList_GetIconSize(ptc->himl, &cxImage, &cy);
                cxImage += ptc->cxPad;
                siz.cx += cxImage;
            }
#endif
            cxLabel = siz.cx;

	        if (Tab_FixedWidth(ptc))
            {
		        siz.cx = ptc->iTabWidth;
	        }
            else
            {
		        siz.cx += ptc->cxPad * 2;

                // Make sure the tab has a least a minimum width
                if (siz.cx < ptc->cxMinTab)
                    siz.cx = ptc->cxMinTab;
	        }

	        // should we wrap?
	        if (Tab_MultiLine(ptc))
            {
                // two cases to wrap around:
                // case 2: is our right edge past the end but we ourselves
                //   are shorter than the width?
                // case 1: are we already past the end? (this happens if
                //      the previous line had only one item and it was longer
                //      than the tab's width.
                int iTotalWidth = ptc->cxTabs - g_cxEdge;
		        if (x > iTotalWidth ||
                    (x+siz.cx >= iTotalWidth &&
                     (siz.cx < iTotalWidth)))
                {
		            x = xStart;
		            y += ptc->cyTabs;
		            iRow++;

                    if (Tab_DrawButtons(ptc))
                        y += ((g_cyEdge * 3)/2);
		        }
		        pitem->iRow = iRow;
	        }
	
	        pitem->rc.left = x;
	        pitem->rc.right = x + siz.cx;
	        pitem->rc.top = y;
	        pitem->rc.bottom = ptc->cyTabs + y;

            if (!Tab_FixedWidth(ptc) || Tab_ForceLabelLeft(ptc) ||
                Tab_ForceIconLeft(ptc))
            {

                pitem->xImage = pitem->rc.left + ptc->cxPad;

            }
            else
            {
                // in fixed width mode center it
                pitem->xImage = (pitem->rc.left + pitem->rc.right - cxLabel)/2;
            }

            if (pitem->xImage < (pitem->rc.left + g_cxEdge))
                pitem->xImage = pitem->rc.left + g_cxEdge;

            if (Tab_ForceIconLeft(ptc))
            {
                pitem->xLabel = (pitem->rc.left + pitem->rc.right - (cxLabel - cxImage)) / 2;
            }
            else
            {
                pitem->xLabel = pitem->xImage + cxImage;
            }

	        pitem->yImage = pitem->rc.top + ptc->cyPad + ptc->cyIcon - (g_cyEdge/2);
	        pitem->yLabel = pitem->rc.top + ptc->cyPad + ptc->cyText - (g_cyEdge/2);

	        x = pitem->rc.right;

            if (Tab_DrawButtons(ptc))
            {
                x += (g_cxEdge * 3)/2;
            }
	    }

	    ptc->cxItem = x;	// total width of all tabs

        // if we added a line in non-button mode, we need to do a full refresh
        if (ptc->iLastRow != -1 &&
            ptc->iLastRow != iRow &&
            !Tab_DrawButtons(ptc)) {
            InvalidateRect(ptc->hwnd, NULL, TRUE);
        }
	    ptc->iLastRow = (cItems > 0) ? iRow : -1;
	
	    if (Tab_MultiLine(ptc))
        {
	        if (!Tab_RaggedRight(ptc) && !Tab_FixedWidth(ptc))
		        RightJustify(ptc);
	
	        if (!Tab_DrawButtons(ptc) && ptc->iSel != -1)
		        PutzRowToBottom(ptc, Tab_FastGetItemPtr(ptc, ptc->iSel)->iRow);

	    }
        else if ( cItems > 0)
        {
	        // adjust x's to the first visible
	        int dx;
	        pitem = Tab_GetItemPtr(ptc, ptc->iFirstVisible);
            if (pitem) {
                dx = -pitem->rc.left + g_cxEdge;
                for ( i = cItems - 1; i >=0  ; i--) {
                    pitem = Tab_FastGetItemPtr(ptc, i);
                    OffsetRect(&pitem->rc, dx, 0);
                    pitem->xLabel += dx;
                    pitem->xImage += dx;
                }
            }
	    }

	    if (bReleaseDC)
	    {
	        ReleaseDC(NULL, hdc);
	    }

        UpdateToolTipRects(ptc);
    }
}

void NEAR PASCAL DoCorners(HDC hdc, LPRECT prc)
{
    RECT rc;
    COLORREF iOldColor;

    rc = *prc;

    // upper right

    iOldColor = SetBkColor(hdc, g_clrBtnFace);
    rc.left = rc.right - 2;
    rc.bottom = rc.top + 3;
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    rc.top++;

    DrawEdge(hdc, &rc, EDGE_RAISED, BF_SOFT | BF_DIAGONAL_ENDBOTTOMRIGHT);

    rc = *prc;
    rc.right = rc.left + 2;
    rc.bottom = rc.top + 3;
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    rc.top++;
    DrawEdge(hdc, &rc, EDGE_RAISED, BF_SOFT | BF_DIAGONAL_ENDTOPRIGHT);
}

void NEAR PASCAL RefreshArrows(PTC ptc, HDC hdc)
{
    RECT rcClip, rcArrows, rcIntersect;

    if (ptc->hwndArrows && IsWindowVisible(ptc->hwndArrows)) {

        GetClipBox(hdc, &rcClip);
        GetWindowRect(ptc->hwndArrows, &rcArrows);
        MapWindowPoints(NULL, ptc->hwnd, (POINT FAR *)&rcArrows, 2);
        if (IntersectRect(&rcIntersect, &rcClip, &rcArrows))
            RedrawWindow(ptc->hwndArrows, NULL, NULL, RDW_INVALIDATE);
    }
}

void NEAR PASCAL DrawBody(HDC hdc, PTC ptc, LPTABITEM pitem, LPRECT lprc, int i,
                          BOOL fTransparent, int dx, int dy)
{
    BOOL fSelected = (i == ptc->iSel);

    if (Tab_OwnerDraw(ptc)) {
        DRAWITEMSTRUCT dis;
        WORD wID = GetWindowID(ptc->hwnd);

        dis.CtlType = ODT_TAB;
        dis.CtlID = wID;
        dis.itemID = i;
        dis.itemAction = ODA_DRAWENTIRE;
        if (fSelected)
            dis.itemState = ODS_SELECTED;
        else
            dis.itemState = 0;
        dis.hwndItem = ptc->hwnd;
        dis.hDC = hdc;
        dis.rcItem = *lprc;
        dis.itemData =
            (ptc->cbExtra <= sizeof(LPARAM)) ?
                (DWORD)pitem->lParam : (DWORD)(LPBYTE)&pitem->abExtra;

        SendMessage( GetParent(ptc->hwnd) , WM_DRAWITEM, wID,
                    (LPARAM)(DRAWITEMSTRUCT FAR *)&dis);

    } else {
        // draw the text and image
        // draw even if pszText == NULL to blank it out
#ifndef WIN31
#define USE_DRAWTEXT
#endif
#ifdef USE_DRAWTEXT
        BOOL fUseDrawText = FALSE;
        if (pitem->pszText) {

            // only use draw text if there's any underlining to do.
            if (StrChr(pitem->pszText, CH_PREFIX)) {
                fUseDrawText = TRUE;
            }
        }

        // DrawTextEx will not clear the entire area, so we need to.
        // or if there's no text, we need to blank it out
        if ((fUseDrawText || !pitem->pszText) && !fTransparent)
            ExtTextOut(hdc, pitem->xLabel + dx, pitem->yLabel + dy,
                       ETO_OPAQUE, lprc, NULL, 0, NULL);


        if (pitem->pszText) {

            if (fUseDrawText) {
                DRAWTEXTPARAMS dtp;
                dtp.cbSize = sizeof(DRAWTEXTPARAMS);
                dtp.iLeftMargin = pitem->xLabel + dx - lprc->left;
                dtp.iRightMargin = 0;
#if defined(WINDOWS_ME)
                DrawTextEx(hdc, pitem->pszText, -1, lprc, 
                         (pitem->etoRtlReading ?DT_RTLREADING :0) |
                         DT_END_ELLIPSIS | DT_SINGLELINE | DT_VCENTER, &dtp);
#else
                DrawTextEx(hdc, pitem->pszText, -1, lprc, DT_END_ELLIPSIS | DT_SINGLELINE | DT_VCENTER, &dtp);
#endif
            } else {
                ExtTextOut(hdc, pitem->xLabel + dx, pitem->yLabel + dy,
#ifdef WINDOWS_ME
						   pitem->etoRtlReading | 
#endif
                           (fTransparent ? ETO_CLIPPED : ETO_OPAQUE | ETO_CLIPPED),
                           lprc, pitem->pszText, lstrlen(pitem->pszText),
                           NULL);
            }
        }
#else
        ExtTextOut(hdc, pitem->xLabel + dx, pitem->yLabel + dy,
#if defined(WINDOWS_ME)
						   pitem->etoRtlReading | 
#endif
                   (fTransparent ? ETO_CLIPPED : ETO_CLIPPED | ETO_OPAQUE),
                   lprc, pitem->pszText, pitem->pszText ? lstrlen(pitem->pszText) : 0,
                   NULL);
#endif

#ifdef WANNA_BLUR_ME
        if (pitem->pszText) {
            // blurring
            if (fSelected) {
                if (!fTransparent) {
                    SetBkMode(hdc, TRANSPARENT);

                    // guaranteed to be buttons if we got here
                    // becaues can't iSel==i is rejected for tabs in this loop
                    ExtTextOut(hdc, pitem->xLabel + dx + 1, pitem->yLabel + dy,
#if defined(WINDOWS_ME)
						   pitem->etoRtlReading | 
#endif
                               ETO_CLIPPED, lprc, pitem->pszText, lstrlen(pitem->pszText),
                               NULL);

                    SetBkMode(hdc, OPAQUE);
                }
            }
        }
#endif

#ifndef WIN31
        if (HASIMAGE(ptc, pitem))
            ImageList_Draw(ptc->himl, pitem->iImage, hdc,
                           pitem->xImage + dx, pitem->yImage + dy,
                           fTransparent ? ILD_TRANSPARENT : ILD_NORMAL);
#endif

    }
}

void NEAR Tab_Paint(PTC ptc, HDC hdcIn)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rcClient, rcClipBox, rcTest, rcBody;
    int cItems, i;
    int fnNewMode = OPAQUE;
    LPTABITEM pitem;
    HWND hwnd = ptc->hwnd;
    HBRUSH hbrOld;
    UINT uWhichEdges;
    RECT rc;

    GetClientRect(hwnd, &rcClient);
    if (!rcClient.right)
	return;

    if (hdcIn)
    {
        hdc = hdcIn;
        ps.rcPaint = rcClient;
    }
    else
        hdc = BeginPaint(hwnd, &ps);

    // select font first so metrics will have the right size
    SelectObject(hdc, ptc->hfontLabel ? ptc->hfontLabel : g_hfontSystem);
    CalcPaintMetrics(ptc, hdc);

    // draw border all around everything if it's not a button style
    rcClient.top += (ptc->cyTabs * (ptc->iLastRow+1)) + g_cyEdge;
    if(Tab_DrawButtons(ptc)) {
        uWhichEdges = BF_RECT | BF_SOFT;
    } else {
	DrawEdge(hdc, &rcClient, EDGE_RAISED, BF_SOFT | BF_RECT);
        uWhichEdges = BF_LEFT | BF_TOP | BF_RIGHT | BF_SOFT;
    }

    cItems = Tab_Count(ptc);
    if (cItems) {

        RefreshArrows(ptc, hdc);
        SetBkColor(hdc, g_clrBtnFace);
        SetTextColor(hdc, g_clrBtnText);

	if (!Tab_MultiLine(ptc))
	    IntersectClipRect(hdc, 0, 0,
			      ptc->cxTabs, rcClient.bottom);
	
	GetClipBox(hdc, &rcClipBox);
	// draw all but the selected item
	for (i = ptc->iFirstVisible; i < cItems; i++) {
		
            pitem = Tab_FastGetItemPtr(ptc, i);

            if (!Tab_MultiLine(ptc)) {
                // if not multiline, and we're off the screen... we're done
                if (pitem->rc.left > ptc->cxTabs)
                    break;
            }

	    // should we bother drawing this?
            if (i != ptc->iSel || Tab_DrawButtons(ptc)) {
                if (IntersectRect(&rcTest, &rcClipBox, &pitem->rc)) {

                    int dx = 0, dy = 0;  // shift variables if button sunken;
                    UINT edgeType;

                    rc = pitem->rc;
                    rcBody = rc;

                    // Draw the edge around each item
		    if(Tab_DrawButtons(ptc) &&
                       ((ptc->iNewSel == i && ptc->flags & TCF_DRAWSUNKEN) ||
                        (ptc->iSel == i))) {

                        dx = g_cxEdge/2;
                        dy = g_cyEdge/2;
                        edgeType =  EDGE_SUNKEN;

                    } else
                        edgeType = EDGE_RAISED;

                    if (Tab_DrawButtons(ptc) && !Tab_OwnerDraw(ptc)) {

                        // if drawing buttons, show selected by dithering  background
                        // which means we need to draw transparent.
                        if (ptc->iSel == i) {
                            fnNewMode = TRANSPARENT;
                            SetBkMode(hdc, TRANSPARENT);
#ifndef WIN31
                            hbrOld = SelectObject(hdc, g_hbrMonoDither);
#else
                            hbrOld = SelectObject(hdc, g_hbrGray);
#endif
                            SetTextColor(hdc, g_clrBtnHighlight);
                            PatBlt(hdc, rc.left, rc.top, rc.right - rc.left,
                                   rc.bottom - rc.top, PATCOPY);
                            SetTextColor(hdc, g_clrBtnText);
                        }
                    }

                    InflateRect(&rcBody, -g_cxEdge, -g_cyEdge);
                    if (!Tab_DrawButtons(ptc)) {
                        rcBody.bottom += g_cyEdge;
                    }
                    DrawBody(hdc, ptc, pitem, &rcBody, i, fnNewMode == TRANSPARENT,
                             dx, dy);

                    DrawEdge(hdc, &rc, edgeType, uWhichEdges);
                    if (!Tab_DrawButtons(ptc))
                        DoCorners(hdc, &pitem->rc);

                    if (fnNewMode == TRANSPARENT) {
                        fnNewMode = OPAQUE;
                        SelectObject(hdc, hbrOld);
                        SetBkMode(hdc, OPAQUE);
                    }
		}
            }
	}

        if (!Tab_MultiLine(ptc))
            ptc->iLastVisible = i - 1;
        else
            ptc->iLastVisible = cItems - 1;

	// draw the selected one last to make sure it is on top
        pitem = Tab_GetItemPtr(ptc, ptc->iSel);
        if (pitem && (pitem->rc.left <= ptc->cxTabs)) {
            rc = pitem->rc;

            if (!Tab_DrawButtons(ptc)) {
		InflateRect(&rc, g_cxEdge, g_cyEdge);

                if (IntersectRect(&rcTest, &rcClipBox, &rc)) {

                    DrawBody(hdc, ptc, pitem, &rc, ptc->iSel, FALSE, 0,-g_cyEdge);

                    rc.bottom--;  //because of button softness
                    DrawEdge(hdc, &rc, EDGE_RAISED, uWhichEdges);
                    DoCorners(hdc, &rc);

                    // draw that extra bit on the left or right side
                    // if we're on the edge
                    rc.bottom++;
                    rc.top = rc.bottom-1;
                    if (rc.right == rcClient.right) {
                        uWhichEdges = BF_SOFT | BF_RIGHT;

                    } else if (rc.left == rcClient.left) {
                        uWhichEdges = BF_SOFT | BF_LEFT;
                    } else
                        uWhichEdges = 0;

                    if (uWhichEdges)
                        DrawEdge(hdc, &rc, EDGE_RAISED, uWhichEdges);
                }
            }

	}

        // draw the focus rect
        if (GetFocus() == hwnd) {

            if (!pitem && (ptc->iNewSel != -1)) {
                pitem = Tab_GetItemPtr(ptc, ptc->iNewSel);
            }

            if (pitem) {
                rc = pitem->rc;
                if (Tab_DrawButtons(ptc))
                    InflateRect(&rc, -g_cxEdge, -g_cyEdge);
                else
                    InflateRect(&rc, -(g_cxEdge/2), -(g_cyEdge/2));
		DrawFocusRect(hdc, &rc);
	    }
        }
    }

    if (hdcIn == NULL)
        EndPaint(hwnd, &ps);
}

int PASCAL Tab_FindTab(PTC ptc, int iStart, UINT vk)
{
    int iRow;
    int x;
    int i;
    LPTABITEM pitem = Tab_GetItemPtr(ptc, iStart);

    if (!pitem)
    {
        return(0);
    }

    iRow=  pitem->iRow  + ((vk == VK_UP) ? -1 : 1);
    x = (pitem->rc.right + pitem->rc.left) / 2;

    // find the and item on the iRow at horizontal x
    if (iRow > ptc->iLastRow || iRow < 0)
        return iStart;

    // this relies on the ordering of tabs from left to right , but
    // not necessarily top to bottom.
    for (i = Tab_Count(ptc) - 1 ; i >= 0; i--) {
        pitem = Tab_FastGetItemPtr(ptc, i);
        if (pitem->iRow == iRow) {
            if (pitem->rc.left < x)
                return i;
        }
    }

    // this should never happen.. we should have caught this case in the iRow check
    // right before the for loop.
    Assert(0);
    return iStart;
}

void NEAR PASCAL Tab_SetCurFocus(PTC ptc, int iStart)
{

    if (Tab_DrawButtons(ptc)) {
        if ((iStart >= 0) && (iStart < Tab_Count(ptc)) && (ptc->iNewSel != iStart)) {
            if (ptc->iNewSel != -1)
                InvalidateItem(ptc, ptc->iNewSel, FALSE);
            InvalidateItem(ptc, iStart, FALSE);
            ptc->iNewSel = iStart;
            ptc->flags |= TCF_DRAWSUNKEN;
        }
    } else
        ChangeSel(ptc, iStart, TRUE);
}

void NEAR PASCAL Tab_OnKeyDown(PTC ptc, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    int iStart;
    TC_KEYDOWN nm;

    // Notify
    nm.wVKey = vk;
    nm.flags = flags;
    SendNotify(GetParent(ptc->hwnd), ptc->hwnd, TCN_KEYDOWN, &nm.hdr);

    if (Tab_DrawButtons(ptc)) {
        ptc->flags |= (TCF_DRAWSUNKEN|TCF_MOUSEDOWN);
        if (ptc->iNewSel != -1) {
            iStart = ptc->iNewSel;
        } else {
            iStart = ptc->iSel;
        }
    } else {
        iStart = ptc->iSel;
    }

    switch (vk) {

    case VK_LEFT:
        iStart--;
        break;

    case VK_RIGHT:
        iStart++;
        break;

    case VK_UP:
    case VK_DOWN:
        if (iStart != -1) {
            iStart = Tab_FindTab(ptc, iStart, vk);
            break;
        } // else fall through to set iStart = 0;

    case VK_HOME:
        iStart = 0;
        break;

    case VK_END:
        iStart = Tab_Count(ptc) - 1;
        break;

    case VK_SPACE:
        if (!Tab_DrawButtons(ptc))
            return;
        // else fall through...  in button mode space does selection

    case VK_RETURN:
        ChangeSel(ptc, iStart, TRUE);
        ptc->iNewSel = -1;
        ptc->flags &= ~TCF_DRAWSUNKEN;
        return;

    default:
	return;
    }

    if (iStart < 0)
        iStart = 0;

    Tab_SetCurFocus(ptc, iStart);
}

void NEAR Tab_Size(PTC ptc)
{
    ptc->cxItem = RECOMPUTE;
    Tab_UpdateArrows(ptc, TRUE);
}

BOOL NEAR PASCAL Tab_OnGetItemRect(PTC ptc, int iItem, LPRECT lprc)
{
    LPTABITEM pitem = Tab_GetItemPtr(ptc, iItem);


    if (lprc) {
        CalcPaintMetrics(ptc, NULL);
        if (pitem) {

            // Make sure all the item rects are up-to-date

            *lprc = pitem->rc;
            return TRUE;
        } else {
            lprc->top = 0;
            lprc->bottom = ptc->cyTabs;
            lprc->right = 0;
            lprc->left = 0;
        }
    }
    return FALSE;
}

void PASCAL Tab_StyleChanged(PTC ptc, UINT gwl,  LPSTYLESTRUCT pinfo)
{
#define STYLE_MASK   (TCS_BUTTONS | TCS_MULTILINE | TCS_RAGGEDRIGHT | TCS_FIXEDWIDTH | TCS_FORCELABELLEFT | TCS_FORCEICONLEFT | TCS_OWNERDRAWFIXED)
    if (ptc && (gwl == GWL_STYLE)) {

        if (  (ptc->style & STYLE_MASK) ^ (pinfo->styleNew & STYLE_MASK)) {
            ptc->style = (ptc->style & ~STYLE_MASK)  | (pinfo->styleNew & STYLE_MASK);

            // make sure we don't have invalid bits set
            if (!Tab_FixedWidth(ptc)) {
                ptc->style &= ~(TCS_FORCEICONLEFT | TCS_FORCELABELLEFT);
            }
            ptc->cxItem = RECOMPUTE;
            ptc->cyTabs = RECOMPUTE;
            RedrawAll(ptc, RDW_ERASE | RDW_INVALIDATE);
        }

#define FOCUS_MASK (TCS_FOCUSONBUTTONDOWN | TCS_FOCUSNEVER)
        if ( (ptc->style &  FOCUS_MASK) ^ (pinfo->styleNew & FOCUS_MASK)) {
            ptc->style = (ptc->style & ~FOCUS_MASK)  | (pinfo->styleNew & FOCUS_MASK);
        }
    }
}

LRESULT CALLBACK Tab_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PTC ptc = (PTC)GetWindowInt((hwnd), 0);

    switch (msg) {
	
    HANDLE_MSG(ptc, WM_HSCROLL, Tab_OnHScroll);

    case WM_CREATE:
        InitGlobalMetrics(0);
        InitGlobalColors();
	ptc = (PTC)NearAlloc(sizeof(TC));
	if (!ptc)
	    return -1;	// fail the window create

	SetWindowInt(hwnd, 0, (int)ptc);
	ptc->hwnd = hwnd;

	if (!Tab_OnCreate(ptc, (LPCREATESTRUCT)lParam))
	    return -1;
	break;

    case WM_DESTROY:
	Tab_OnDestroy(ptc);
	break;

    case WM_SIZE:
	Tab_Size(ptc);
	break;

    case WM_SYSCOLORCHANGE:
        ReInitGlobalColors();
        RedrawAll(ptc, RDW_INVALIDATE | RDW_ERASE);
        break;

    case WM_WININICHANGE:
        InitGlobalMetrics(wParam);
        if ((wParam == SPI_SETNONCLIENTMETRICS) ||
            (!wParam && !lParam))
            RedrawAll(ptc, RDW_INVALIDATE | RDW_ERASE);
	break;

    case WM_PRINTCLIENT:
    case WM_PAINT:
	Tab_Paint(ptc, (HDC)wParam);
	break;

    case WM_STYLECHANGED:
        Tab_StyleChanged(ptc, wParam, (LPSTYLESTRUCT)lParam);
        break;
	
    case WM_MOUSEMOVE:
        RelayToToolTips(ptc->hwndToolTips, hwnd, msg, wParam, lParam);
	Tab_OnMouseMove(ptc, wParam, LOWORD(lParam), HIWORD(lParam));
	break;
	
    case WM_LBUTTONDOWN:
        RelayToToolTips(ptc->hwndToolTips, hwnd, msg, wParam, lParam);
	Tab_OnButtonDown(ptc, LOWORD(lParam), HIWORD(lParam));
	break;
        
    case WM_MBUTTONDOWN:
        SetFocus(hwnd);
        break;

    case WM_RBUTTONUP:
        if (!SendNotify(GetParent(ptc->hwnd), ptc->hwnd, NM_RCLICK, NULL))
            goto DoDefault;
        break;

    case WM_CAPTURECHANGED:
	lParam = -1L; // fall through to LBUTTONUP

    case WM_LBUTTONUP:
        if (msg == WM_LBUTTONUP) {
            RelayToToolTips(ptc->hwndToolTips, hwnd, msg, wParam, lParam);
        }

	Tab_OnButtonUp(ptc, LOWORD(lParam), HIWORD(lParam), (msg == WM_LBUTTONUP));
	break;

    case WM_KEYDOWN:
        HANDLE_WM_KEYDOWN(ptc, wParam, lParam, Tab_OnKeyDown);
	break;

    case WM_KILLFOCUS:
        if (!ptc)
            break;
        
        if (ptc->iNewSel != -1) {
            int iOldSel = ptc->iNewSel;
            ptc->iNewSel = -1;
            InvalidateItem(ptc, iOldSel, FALSE);
            ptc->flags &= ~TCF_DRAWSUNKEN;
        }
        // fall through
    case WM_SETFOCUS:
	InvalidateItem(ptc, ptc->iSel, Tab_OwnerDraw(ptc));
	break;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;

    HANDLE_MSG(ptc, WM_SETREDRAW, Tab_OnSetRedraw);
    HANDLE_MSG(ptc, WM_SETFONT, Tab_OnSetFont);

    case WM_GETFONT:
	return (LRESULT)(UINT)ptc->hfontLabel;

    case WM_NOTIFY: {
        LPNMHDR lpNmhdr = (LPNMHDR)(lParam);
        switch(lpNmhdr->code) {
        case TTN_SHOW:
        case TTN_POP:
        case TTN_NEEDTEXT:
            SendMessage(GetParent(ptc->hwnd), WM_NOTIFY, wParam, lParam);
            break;
        }
    }
	break;

    case TCM_SETITEMEXTRA:
        return (LRESULT)Tab_OnSetItemExtra(ptc, (int)wParam);
		
    case TCM_GETITEMCOUNT:
	return (LRESULT)Tab_Count(ptc);

    case TCM_SETITEM:
	return (LRESULT)Tab_OnSetItem(ptc, (int)wParam, (const TC_ITEM FAR*)lParam);

    case TCM_GETITEM:
	return (LRESULT)Tab_OnGetItem(ptc, (int)wParam, (TC_ITEM FAR*)lParam);
		
    case TCM_INSERTITEM:
	return (LRESULT)Tab_OnInsertItem(ptc, (int)wParam, (const TC_ITEM FAR*)lParam);
	
    case TCM_DELETEITEM:
	return (LRESULT)Tab_OnDeleteItem(ptc, (int)wParam);

    case TCM_DELETEALLITEMS:
	return (LRESULT)Tab_OnDeleteAllItems(ptc);

    case TCM_SETCURFOCUS:
        Tab_SetCurFocus(ptc, wParam);
        break;

    case TCM_GETCURFOCUS:
        if (ptc->iNewSel != -1)
            return ptc->iNewSel;
        // else fall through

    case TCM_GETCURSEL:
	return ptc->iSel;

    case TCM_SETCURSEL:
	return (LRESULT)ChangeSel(ptc, (int)wParam, FALSE);

    case TCM_GETTOOLTIPS:
        return (LRESULT)(UINT)ptc->hwndToolTips;
	
    case TCM_SETTOOLTIPS:
	ptc->hwndToolTips = (HWND)wParam;
	break;

    case TCM_ADJUSTRECT:
        if (lParam) {
            int idy;
            CalcPaintMetrics(ptc, NULL);
#define prc ((RECT FAR *)lParam)
            if (Tab_DrawButtons(ptc)) {
                if (Tab_Count(ptc)) {
                    RECT rc;
                    Tab_OnGetItemRect(ptc, Tab_Count(ptc) - 1, &rc);
                    idy = rc.bottom;
                } else {
                    idy = 0;
                }
            } else {
                idy = (ptc->cyTabs * (ptc->iLastRow + 1));
            }
            idy += g_cyEdge * 2;
            
            if (wParam) {
                // calc a larger rect from the smaller
                prc->left -= g_cxEdge * 2;
                prc->right += g_cxEdge * 2;
                prc->bottom += g_cyEdge * 2;
                prc->top -= idy;

            } else {
                // given the bounds, calc the "client" area
                prc->left += g_cxEdge * 2;
                prc->right -= g_cxEdge * 2;
                prc->bottom -= g_cyEdge * 2;
                prc->top += idy;
            }
        } else
            return -1;
	break;
	
    case TCM_GETITEMRECT:
	return Tab_OnGetItemRect(ptc, (int)wParam, (LPRECT)lParam);

    case TCM_SETIMAGELIST: {
	HIMAGELIST himlOld = ptc->himl;
	ptc->himl = (HIMAGELIST)lParam;
        ptc->cxItem = ptc->cyTabs = RECOMPUTE;
        RedrawAll(ptc, RDW_INVALIDATE | RDW_ERASE);
	return (LRESULT)(UINT)himlOld;
    }
	
    case TCM_GETIMAGELIST:
	return (LRESULT)(UINT)ptc->himl;

    case TCM_REMOVEIMAGE:
        Tab_OnRemoveImage(ptc, (int)wParam);
        break;
	
    case TCM_SETITEMSIZE: {
        int iOldWidth = ptc->iTabWidth;
        int iOldHeight = ptc->iTabHeight;
        int iNewWidth = LOWORD(lParam);
        int iNewHeight = HIWORD(lParam);

#ifndef WIN31
        if (ptc->himl) {
            int cx, cy;
            ImageList_GetIconSize(ptc->himl, &cx, &cy);
            if (iNewWidth < (cx + (2*g_cxEdge)))
                iNewWidth = cx + (2*g_cxEdge);

        }
#endif
        ptc->iTabWidth = iNewWidth;
        ptc->iTabHeight = iNewHeight;


        if (iNewWidth != iOldWidth ||
            iNewHeight != iOldHeight) {
            ptc->cxItem = RECOMPUTE;
            ptc->cyTabs = RECOMPUTE;
            RedrawAll(ptc, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
        }

	return (LRESULT)MAKELONG(iOldWidth, iOldHeight);
    }

    case TCM_SETPADDING:
        ptc->cxPad = LOWORD(lParam);
        ptc->cyPad = HIWORD(lParam);
        break;

    case TCM_GETROWCOUNT:
        CalcPaintMetrics(ptc, NULL);
        return (LRESULT)ptc->iLastRow + 1;
	
    case TCM_HITTEST: {
#define lphitinfo  ((LPTC_HITTESTINFO)lParam)
        return Tab_OnHitTest(ptc, lphitinfo->pt.x, lphitinfo->pt.y, &lphitinfo->flags);
    }

        case WM_NCHITTEST:
        {
            POINT pt;
            
            pt.x = (int)LOWORD(lParam);
            pt.y = (int)HIWORD(lParam);
            ScreenToClient(ptc->hwnd, &pt);
            if (Tab_OnHitTest(ptc, pt.x, pt.y, NULL) == -1)
                return(HTTRANSPARENT);
            else {
                //fall through
            }
        }

    default:
DoDefault:
	return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0L;
}
