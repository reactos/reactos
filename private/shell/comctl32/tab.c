#include "ctlspriv.h"
#include "tab.h"
#if defined(MAINWIN)
#include <mainwin.h>
#endif

#define BMOVECURSORONCLICK  FALSE
#define BMOVECURSORONDRAG   TRUE

#pragma code_seg(CODESEG_INIT)
BOOL NEAR PASCAL Tab_OnGetItemRect(PTC ptc, int iItem, LPRECT lprc);

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
        wc.style           = CS_GLOBALCLASS | CS_DBLCLKS | CS_HREDRAW |  CS_VREDRAW;
        wc.cbWndExtra      = sizeof(PTC);
        wc.cbClsExtra      = 0;

        return RegisterClass(&wc);
    }

    return TRUE;
}

#pragma code_seg()

void Tab_VFlipRect(PTC ptc, LPRECT prc);
void FAR PASCAL FlipRect(LPRECT prc);
void FAR PASCAL VertInvalidateRect(HWND hwnd, LPRECT qrc, BOOL b, BOOL fVert);
void FAR PASCAL VertDrawEdge(HDC hdc, LPRECT qrc, UINT edgeType, UINT grfFlags,
                               BOOL fVert);
void FAR PASCAL VertPatBlt(HDC hdc1, int x1, int y1, int w, int h,
                          DWORD rop, BOOL fVert);


LRESULT TabDragCallback(HWND hwnd, UINT code, WPARAM wp, LPARAM lp)
{
    PTC ptc = (PTC)GetWindowInt(hwnd, 0);
    LRESULT lres;

    switch (code)
    {
    case DPX_ENTER:
    case DPX_LEAVE:
        ptc->iDragTab = -1;
        ptc->dwDragDelay = 0;
        lres = 1;
        break;

    case DPX_DRAGHIT:
        if (lp)
        {
            BOOL fResetDelay = TRUE;
            int iTab;
            POINT pt;
            pt.x = ((POINTL *)lp)->x;
            pt.y = ((POINTL *)lp)->y;


            MapWindowPoints(NULL, ptc->ci.hwnd, &pt, 1);
            iTab = Tab_OnHitTest(ptc, pt.x, pt.y, NULL);

            if ((iTab != ptc->iSel))
            {
                if (iTab >= 0)
                {
                    DWORD dwHitTime = GetTickCount();

                    if (dwHitTime >= ptc->dwDragDelay)
                    {
                        if (ptc->dwDragDelay)
                        {
                            ChangeSel(ptc, iTab, TRUE, BMOVECURSORONDRAG);

                            // present no target if validation failed
                            // this will prevent accidental drops
                            if (ptc->iSel != iTab)
                                iTab = -1;
                        }
                        else
                        {
                            ptc->dwDragDelay = dwHitTime + TAB_DRAGDELAY;
                            fResetDelay = FALSE;
                        }
                    }
                    else if (iTab == ptc->iDragTab)
                        fResetDelay = FALSE;
                }

                ptc->iDragTab = iTab;
            }

            if (fResetDelay)
                ptc->dwDragDelay = 0;

            lres = (LRESULT)iTab;
        }
        else
            lres = -1;
        break;

    case DPX_GETOBJECT:
        lres = (LRESULT)GetItemObject(&ptc->ci, TCN_GETOBJECT, &IID_IDropTarget, (LPNMOBJECTNOTIFY)lp);
        break;

    case DPX_SELECT:
        if (((int)wp) >= 0)
        {
            SendMessage(ptc->ci.hwnd, TCM_HIGHLIGHTITEM, wp,
                MAKELPARAM((lp != DROPEFFECT_NONE), 0));
        }
        lres = 0;
        break;

    default:
        lres = -1;
        break;
    }

    return lres;
}


void VertSmoothScrollWindow(HWND hwnd, int dx, int dy, LPCRECT lprcSrc, LPCRECT lprcClip, HRGN hrgn, LPRECT lprcUpdate, UINT fuScroll, BOOL fVert, UINT uScrollMin)
{
    RECT rcSrc;
    RECT rcClip;
    
    if (fVert) {
        SWAP(dx, dy, int);
        
        if (lprcSrc) {
            rcSrc = *lprcSrc;
            lprcSrc = &rcSrc;
            FlipRect(&rcSrc);
        }
        if (lprcClip) {
            rcClip = *lprcClip;
            lprcClip = &rcClip;
            FlipRect(&rcClip);
        }
        
    }
    {
#ifndef UNIX
        SMOOTHSCROLLINFO si = 
        {
            sizeof(si),
            SSIF_MINSCROLL,
            hwnd, 
            dx, 
            dy, 
            lprcSrc, 
            lprcClip, 
            hrgn, 
            lprcUpdate, 
            fuScroll,
            SSI_DEFAULT,
            uScrollMin,
            uScrollMin
        };
#else
        SMOOTHSCROLLINFO si;
        si.cbSize=sizeof(si);
        si.fMask= SSIF_MINSCROLL;
        si.hwnd= hwnd;
        si.dx=dx;
        si.dy=dy;
        si.lprcSrc=lprcSrc;
        si.lprcClip=lprcClip;
        si.hrgnUpdate=hrgn;
        si.lprcUpdate=lprcUpdate;
        si.fuScroll=fuScroll;
        si.uMaxScrollTime=SSI_DEFAULT;
        si.cxMinScroll=uScrollMin;
        si.cyMinScroll= uScrollMin;
        si.pfnScrollProc = NULL;
#endif
        SmoothScrollWindow(&si);
    }
    
    if (fVert) {
        
        if (lprcUpdate)
            FlipRect(lprcUpdate);
    }
}

void Tab_SmoothScrollWindow(PTC ptc, int dx, int dy, LPRECT lprcSrc, LPRECT lprcClip, 
                            HRGN hrgn, LPRECT lprcUpdate, UINT fuScroll, UINT uScrollMin) 
{
    RECT rcSrc;
    RECT rcClip;
    if (Tab_Bottom(ptc)) {
        dy *= -1;
        if (lprcSrc) {
            rcSrc = *lprcSrc;
            lprcSrc = &rcSrc;
            Tab_VFlipRect(ptc, lprcSrc);
        }
        
        if (lprcClip) {
            rcClip = *lprcClip;
            lprcClip = &rcClip;
            Tab_VFlipRect(ptc, lprcClip);
        }
        
    }
    
    VertSmoothScrollWindow(ptc->ci.hwnd, dx, dy, lprcSrc, lprcClip, hrgn, lprcUpdate, fuScroll, Tab_Vertical(ptc), uScrollMin);

    if (lprcUpdate) {
        Tab_VFlipRect(ptc, lprcClip);
    }

}


void Tab_InvalidateRect(PTC ptc, LPRECT prc, BOOL b) 
{
    RECT rc = *prc;
    Tab_VFlipRect(ptc, &rc);
    VertInvalidateRect((ptc)->ci.hwnd, &rc, b, Tab_Vertical(ptc));
}

void Tab_DrawEdge(HDC hdc, LPRECT prc, UINT uType, UINT uFlags, PTC ptc) 
{
    RECT rc = *prc;
    Tab_VFlipRect(ptc, &rc);
    if (Tab_Bottom(ptc)) {
        
        
        UINT uNewFlags;

        if (uFlags & BF_DIAGONAL) {
            uNewFlags = uFlags & ~(BF_RIGHT | BF_LEFT);
            if (uFlags & BF_LEFT)
                uNewFlags |= BF_RIGHT;
            if (uFlags & BF_RIGHT) 
                uNewFlags |= BF_LEFT;
        } else {

            uNewFlags = uFlags & ~(BF_TOP | BF_BOTTOM);
            if (uFlags & BF_TOP)
                uNewFlags |= BF_BOTTOM;
            if (uFlags & BF_BOTTOM) 
                uNewFlags |= BF_TOP;
        }
        uFlags = uNewFlags;
    }
    VertDrawEdge(hdc, &rc, uType, uFlags, Tab_Vertical(ptc));
}
        
void Tab_PatBlt(HDC hdc, int x1, int y1, int w, int h, UINT rop, PTC ptc) 
{
    RECT rc;
    rc.top = y1;
    rc.left = x1;
    rc.right = x1+w;
    rc.bottom = y1+h;
    Tab_VFlipRect(ptc, &rc);
    VertPatBlt(hdc, rc.left, rc.top, RECTWIDTH(rc) , RECTHEIGHT(rc), rop, Tab_Vertical(ptc));
}
        

void NormalizeRect(LPRECT prc)
{
    if (prc->right < prc->left) {
        SWAP(prc->right, prc->left, int);
    }
    
    if (prc->bottom < prc->top) {
        SWAP(prc->bottom, prc->top, int);
    }
}

void VFlipRect(LPRECT prcClient, LPRECT prc)
{
    int iTemp = prc->bottom;
    
    prc->bottom = prcClient->bottom - (prc->top - prcClient->top);
    prc->top = prcClient->bottom - (iTemp - prcClient->top);
}

// diagonal flip.
void Tab_DFlipRect(PTC ptc, LPRECT prc)
{
    if (Tab_Vertical(ptc)) {
        FlipRect(prc);
    }
}

// vertical support is done much like the trackbar control.  we're going
// to flip the coordinate system.  this means that tabs will be added from top down.
void Tab_GetClientRect(PTC ptc, LPRECT prc)
{
    GetClientRect(ptc->ci.hwnd, prc);
    Tab_DFlipRect(ptc, prc);
}

// vertical flip
void Tab_VFlipRect(PTC ptc, LPRECT prc)
{
    if (Tab_Bottom(ptc)) {
        RECT rcClient;
        Tab_GetClientRect(ptc, &rcClient);
        VFlipRect(&rcClient, prc);

    }
}

void Tab_VDFlipRect(PTC ptc, LPRECT prc)
{
    Tab_VFlipRect(ptc, prc);
    Tab_DFlipRect(ptc, prc);
}

// real coordinates to tab coordinates
void Tab_DVFlipRect(PTC ptc, LPRECT prc)
{
    Tab_DFlipRect(ptc, prc);
    Tab_VFlipRect(ptc, prc);
}


#define Tab_ImageList_GetIconSize(ptc, pcx, pcy) VertImageList_GetIconSize((ptc)->himl, pcx, pcy, Tab_Vertical(ptc))
void FAR PASCAL VertImageList_GetIconSize(HIMAGELIST himl, LPINT pcx, LPINT pcy, BOOL fVert)
{
    ImageList_GetIconSize(himl, pcx, pcy);
    if (fVert) {
                
        // if we're in vertical mode, the width is really the height.
        // we won't draw the bitmaps sideways.  we'll rely on people
        // authoring them that way.
        int iTemp = *pcy;
        *pcy = *pcx;
        *pcx = iTemp;
        
    }
}

void FAR PASCAL VertImageList_Draw(HIMAGELIST himl, int iIndex, HDC hdc, int x, int y, UINT uFlags, BOOL fVert)
{
    if (fVert) {
        int iTemp;

        iTemp = y;
        y = x;
        x = iTemp;

        // since we draw from the upper left, flipping the x/y axis means we still draw from the upper left.
        // all we need to do is swap x and y.  we don't need to offset
    }
        
    ImageList_Draw( himl,  iIndex,  hdc,  x,  y,  uFlags);
}
void Tab_ImageList_Draw(PTC ptc, int iImage, HDC hdc, int x, int y, UINT uFlags) 
{
    RECT rc;
    int cxImage, cyImage;
    
    Tab_ImageList_GetIconSize(ptc, &cxImage, &cyImage);

    if (Tab_Bottom(ptc)) {
        y += cyImage;
    }
    rc.top = rc.bottom = y;
    Tab_VFlipRect(ptc, &rc);
    y = rc.top;
    
    VertImageList_Draw((ptc)->himl, iImage, hdc, x, y, uFlags, Tab_Vertical(ptc));
}

#if 1 // DRAWTEXT_DOES_VERTICAL

void Tab_DrawText(HDC hdc, LPTSTR lpsz, int nCount, LPRECT lprc, UINT uFormat, PTC ptc)
{
    RECT rcTemp = *lprc;
    Tab_VDFlipRect(ptc, &rcTemp);
    if (Tab_Vertical(ptc))
        uFormat |= DT_BOTTOM;
   
#ifdef KEYBOARDCUES
    if (CCGetUIState(&(ptc->ci)) & UISF_HIDEACCEL)
    {
        uFormat |= DT_HIDEPREFIX;
    }
#endif

    DrawText(hdc, lpsz, nCount, &rcTemp, uFormat);
}

void Tab_DrawTextEx(HDC hdc, LPTSTR lpsz, int nCount, LPRECT lprc, UINT uFormat, LPDRAWTEXTPARAMS lpParams, PTC ptc)
{
    RECT rcTemp = *lprc;
    Tab_VDFlipRect(ptc, &rcTemp);
    if (Tab_Vertical(ptc))
        uFormat |= DT_BOTTOM;
   
#ifdef KEYBOARDCUES
    if (CCGetUIState(&(ptc->ci)) & UISF_HIDEACCEL)
    {
        uFormat |= DT_HIDEPREFIX;
    }
#endif

    DrawTextEx(hdc, lpsz, nCount, &rcTemp, uFormat, lpParams);
}

#else

#define Tab_DrawText(hdc, lpsz, nCount, lprc, uFormat, ptc) DrawText(hdc, lpsz, nCount, lprc, uFormat)
#define Tab_DrawTextEx(hdc, lpsz, nCount, lprc, uFormat, lpParam, ptc) DrawTextEx(hdc, lpsz, nCount, lprc, uFormat, lpParam)

#endif

void NEAR PASCAL Tab_ExtTextOut(HDC hdc, int x, int y, UINT uFlags, LPRECT prc, 
                                LPTSTR lpsz, UINT cch, CONST INT *pdw, PTC ptc)
{
    RECT rcTemp;

    rcTemp.left = rcTemp.right = x;
    if (Tab_Bottom(ptc) && !Tab_Vertical(ptc)) {

        // first we need to move the top point because if we're drawing on Tab_Bottom, then
        // text won't extend down from y.
        y += ptc->tmHeight;
    }
    rcTemp.top = rcTemp.bottom = y;
    Tab_VDFlipRect(ptc, &rcTemp);
    x = rcTemp.left;
    y = rcTemp.bottom;
    
    rcTemp = *prc;
    Tab_VDFlipRect(ptc, &rcTemp);
    ExtTextOut(hdc, x, y, uFlags, &rcTemp, lpsz, cch, pdw);
}

void NEAR PASCAL VertDrawFocusRect(HDC hdc, LPRECT lprc, BOOL fVert)
{
    
    RECT rc;
    
    rc = *lprc;
    if (fVert)
        FlipRect(&rc);
    
    DrawFocusRect(hdc, &rc);
}

void Tab_DrawFocusRect(HDC hdc, LPRECT lprc, PTC ptc) 
{
    RECT rc = *lprc;
    Tab_VFlipRect(ptc, &rc);
    VertDrawFocusRect(hdc, &rc, Tab_Vertical(ptc));
}


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
    // DebugMsg(DM_TRACE, TEXT("Tab_Scroll dx=%d, iNew=%d\n\r"), dx, iNewFirstIndex);
    Tab_SmoothScrollWindow(ptc, dx, 0, NULL, &rc,
            NULL, NULL, SW_INVALIDATE | SW_ERASE, SSI_DEFAULT);

    // We also need to update the item rectangles and also
    // update the internal variables...
    iMax = Tab_Count(ptc) - 1;
    for (i = iMax; i >= 0; i--)
    {
        pitem = Tab_FastGetItemPtr(ptc, i);
        OffsetRect(&pitem->rc, dx, 0);
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
            Tab_InvalidateItem(ptc, ptc->iLastVisible, TRUE);
        }
    }

    if ((i == ptc->iLastVisible) && pitem)
    {
        // The last previously visible item is still fully visible, so
        // we need to invalidate to the right of it as there may have been
        // room for a partial item before, that will now need to be drawn.
        rc.left = pitem->rc.right;
        Tab_InvalidateRect(ptc, &rc, TRUE);
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
                UpdateWindow(ptc->ci.hwnd);
            }
        }
    }
}

void NEAR Tab_OnSetRedraw(PTC ptc, BOOL fRedraw)
{
    if (fRedraw) {
        ptc->flags |= TCF_REDRAW;
        RedrawAll(ptc, RDW_INVALIDATE);
    } else {
        ptc->flags &= ~TCF_REDRAW;
    }
}

void NEAR Tab_OnSetFont(PTC ptc, HFONT hfont, BOOL fRedraw)
{
    ASSERT(ptc);

    if (!ptc->hfontLabel || hfont != ptc->hfontLabel)
    {
        if (ptc->flags & TCF_FONTCREATED) {
            DeleteObject(ptc->hfontLabel);
            ptc->flags &= ~TCF_FONTCREATED;
            ptc->hfontLabel = NULL;
        }
        
        if (!hfont) {
            // set back to system font
            ptc->hfontLabel = g_hfontSystem;
        } else {
            ptc->flags |= TCF_FONTSET;
            ptc->hfontLabel = hfont;
            ptc->ci.uiCodePage = GetCodePageForFont(hfont);
        }
        ptc->cxItem = ptc->cyTabs = RECOMPUTE;
        

        if (Tab_Vertical(ptc)) {
            // make sure that the font is drawn vertically
            LOGFONT lf;
            GetObject(ptc->hfontLabel, sizeof(lf), &lf);
            
            if (Tab_Bottom(ptc)) {
                lf.lfEscapement = 2700;
            } else {
                lf.lfEscapement = 900; // 90 degrees
            }
            lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
            
            ptc->hfontLabel = CreateFontIndirect(&lf);
            ptc->flags |= TCF_FONTCREATED;
        }

        RedrawAll(ptc, RDW_INVALIDATE | RDW_ERASE);
    }
}


BOOL NEAR Tab_OnCreate(PTC ptc)
{
    HDC hdc;

#ifdef MAINWIN
        // IEUNIX : Using CreateWindowEx with special style to get rid of unwanted border.
        // Mainwin problem
        DWORD exStyle = WS_EX_MW_UNMANAGED_WINDOW;
#else
        DWORD exStyle = 0;
#endif

    ptc->hdpa = DPA_Create(4);
    if (!ptc->hdpa)
        return FALSE;

#ifdef DEBUG
#if 0
    if ((GetAsyncKeyState(VK_SHIFT) < 0) &&
        (GetAsyncKeyState(VK_CONTROL) < 0))
        ptc->ci.style |= TCS_SCROLLOPPOSITE;
    
    if ((GetAsyncKeyState(VK_SHIFT) < 0) &&
        (GetAsyncKeyState(VK_MENU) < 0))
        ptc->ci.style |= TCS_VERTICAL;

    if ((GetAsyncKeyState(VK_SHIFT) < 0) &&
        (GetAsyncKeyState(VK_MENU) < 0))
        ptc->ci.style |= TCS_VERTICAL;
    
    if ((GetAsyncKeyState(VK_SHIFT) < 0) &&
        (GetAsyncKeyState(VK_CONTROL) < 0))
        ptc->ci.style |= TCS_BOTTOM;

#endif

#endif

#ifdef DEBUG
    if (IsFlagSet(g_dwPrototype, PTF_FLATLOOK))
    {
        TraceMsg(TF_GENERAL, "Using flat look for tab controls.");
        ptc->ci.style |= TCS_FLATBUTTONS | TCS_HOTTRACK;
    }
#endif

    // make sure we don't have invalid bits set
    if (!Tab_FixedWidth(ptc)) {
        ptc->ci.style &= ~(TCS_FORCEICONLEFT | TCS_FORCELABELLEFT);
    }
    
    if (Tab_Vertical(ptc)) {
        ptc->ci.style |= TCS_MULTILINE;
        //ptc->ci.style &= ~TCS_BUTTONS;
    }
    
    if (Tab_ScrollOpposite(ptc)) {
        ptc->ci.style |= TCS_MULTILINE;
        ptc->ci.style &= ~TCS_BUTTONS;
    }

    if (Tab_FlatButtons(ptc)) {
        ptc->dwStyleEx |= TCS_EX_FLATSEPARATORS;
    }

    // make us always clip siblings
    SetWindowLong(ptc->ci.hwnd, GWL_STYLE, WS_CLIPSIBLINGS | ptc->ci.style);

    ptc->flags = TCF_REDRAW;        // enable redraw
    ptc->cbExtra = sizeof(LPARAM);  // default extra size
    ptc->iSel = -1;
    ptc->iHot = -1;
    ptc->cxItem = ptc->cyTabs = RECOMPUTE;
    ptc->cxPad = g_cxEdge * 3;
    ptc->cyPad = (g_cyEdge * 3/2);
    ptc->iFirstVisible = 0;
    ptc->hwndArrows = NULL;
    ptc->iLastRow = -1;
    ptc->iNewSel = -1;
    ptc->iLastTopRow = -1;

    hdc = GetDC(NULL);
    ptc->iTabWidth = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);

    //BUGBUG remove this after move to commctrl
    InitDitherBrush();

    if (ptc->ci.style & TCS_TOOLTIPS) {
        TOOLINFO ti;
        // don't bother setting the rect because we'll do it below
        // in FlushToolTipsMgr;
        ti.cbSize = sizeof(ti);
        ti.uFlags = TTF_IDISHWND;
        ti.hwnd = ptc->ci.hwnd;
        ti.uId = (UINT_PTR)ptc->ci.hwnd;
        ti.lpszText = 0;

        ptc->hwndToolTips = CreateWindowEx(exStyle, c_szSToolTipsClass, TEXT(""),
                                              WS_POPUP,
                                              CW_USEDEFAULT, CW_USEDEFAULT,
                                              CW_USEDEFAULT, CW_USEDEFAULT,
                                              ptc->ci.hwnd, NULL, HINST_THISDLL,
                                              NULL);
        if (ptc->hwndToolTips)
            SendMessage(ptc->hwndToolTips, TTM_ADDTOOL, 0,
                        (LPARAM)(LPTOOLINFO)&ti);
        else
            ptc->ci.style &= ~(TCS_TOOLTIPS);
    }

#if defined(FE_IME) || !defined(WINNT)
    if (g_fDBCSInputEnabled)
        ptc->hPrevImc = ImmAssociateContext(ptc->ci.hwnd, 0L);
#endif
    return TRUE;
}


void NEAR Tab_OnDestroy(PTC ptc)
{
    int i;

#if defined(FE_IME) || !defined(WINNT)
    if (g_fDBCSInputEnabled)
        ImmAssociateContext(ptc->ci.hwnd, ptc->hPrevImc);
#endif

    if ((ptc->ci.style & TCS_TOOLTIPS) && IsWindow(ptc->hwndToolTips)) {
        DestroyWindow(ptc->hwndToolTips);
    }

    for (i = 0; i < Tab_Count(ptc); i++)
        Tab_FreeItem(ptc, Tab_FastGetItemPtr(ptc, i));

    DPA_Destroy(ptc->hdpa);

    if (ptc->hDragProxy)
        DestroyDragProxy(ptc->hDragProxy);

    if (ptc->flags & TCF_FONTCREATED) {
        DeleteObject(ptc->hfontLabel);
    }
    
    if (ptc) {
        SetWindowInt(ptc->ci.hwnd, 0, 0);
        NearFree((HLOCAL)ptc);
    }

    //BUGBUG remove this after move to commctrl
    TerminateDitherBrush();
}

// returns true if it actually moved

void NEAR PASCAL PutzRowToBottom(PTC ptc, int iRowMoving)
{
    int i;
    LPTABITEM pitem;
    int dy;
    RECT rcTabs;
    

    Tab_GetClientRect(ptc, &rcTabs);
    
    if (Tab_ScrollOpposite(ptc)) {
        // in scroll mode, the iRow doesn't change.  only the rc's do.
        int yOldTop;
        int yNewTop;
        
        int iLastTopRow = ptc->iLastTopRow == -1 ? ptc->iLastRow : ptc->iLastTopRow;

        if (iRowMoving == iLastTopRow) {
            if (ptc->iLastTopRow == -1)
                ptc->iLastTopRow = iRowMoving;
            return; // already at the bottom;
        }

            
        
        // this is the height of the tab's empty area... which is the amount
        // of space a tab must move to get from the top to the bottom
        dy = rcTabs.bottom - rcTabs.top - (ptc->cyTabs * (ptc->iLastRow + 1)) - g_cyEdge;
        
        for (i = Tab_Count(ptc) - 1; i >= 0; i--) {
            pitem = Tab_FastGetItemPtr(ptc, i);
            DebugMsg(DM_TRACE, TEXT("Putzing %s %d %d %d %d"), pitem->pszText, pitem->rc.left, pitem->rc.top, pitem->rc.right, pitem->rc.bottom);
            
            // save this for scrolling below
            if (pitem->iRow == iRowMoving) {
                yNewTop = pitem->rc.bottom;
            } else if (pitem->iRow == iLastTopRow) {
                yOldTop = pitem->rc.bottom;
            }
            
            if (pitem->iRow > iRowMoving) {
                // this item should be on the bottom
                
                if (pitem->iRow <= iLastTopRow) {
                    // but it's not...
                    OffsetRect(&pitem->rc, 0, dy);
                    
                }
                
            } else {
                // this item should be on the top
                
                if (pitem->iRow > iLastTopRow) {
                    // but it's not... so move it
                    OffsetRect(&pitem->rc, 0, -dy);
                }
            }
            
            if ((pitem->iRow == iLastTopRow) && iLastTopRow > iRowMoving) {
                // in this case, we need to get the yOldTop AFTER it's moved.
                yOldTop = pitem->rc.bottom;
            }
            DebugMsg(DM_TRACE, TEXT("Putzing %s %d %d %d %d"), pitem->pszText, pitem->rc.left, pitem->rc.top, pitem->rc.right, pitem->rc.bottom);
            
        }
        
        if (ptc->iLastTopRow != -1) {
            // if it wasn't a full recalc, then we need to do some scrollwindow crap.
            int dy;
            // first find the topmost parent
            
            dy = yOldTop - yNewTop;
            if (yNewTop > yOldTop) {
                rcTabs.top = yOldTop;
                rcTabs.bottom = yNewTop;
            } else {
                rcTabs.top = yNewTop;
                rcTabs.bottom = yOldTop;
            }
            
            Tab_SmoothScrollWindow(ptc, 0, dy, NULL, &rcTabs, NULL, NULL, SW_ERASE |SW_INVALIDATE, 1);
            InflateRect(&rcTabs, g_cxEdge, g_cyEdge);
            Tab_InvalidateRect(ptc, &rcTabs, FALSE);
        }

        ptc->iLastTopRow = iRowMoving;
        
    } else {
        
        if (iRowMoving == ptc->iLastRow)
            return; // already at the bottom;

        // no scrolling.  just set the iRow var appropriatesly

        for (i = Tab_Count(ptc) -1 ;i >= 0; i--) {
            pitem = Tab_FastGetItemPtr(ptc, i);
            if (pitem->iRow > iRowMoving) {
                
                // if the row is higher than the row that's being selected,
                // it drops one.
                pitem->iRow--;
                dy = -ptc->cyTabs;
                
            } else if (pitem->iRow == iRowMoving) {
                // save this
                rcTabs.top = pitem->rc.top;
                
                // if it's on the row that's moving down, we assign it to iLastRow and
                //calculate how far it needs to go.
                dy = ptc->cyTabs * (ptc->iLastRow - iRowMoving);
                pitem->iRow = ptc->iLastRow;

            } else
                continue;

            pitem->rc.top += dy;
            pitem->rc.bottom += dy;
        }
        
        rcTabs.bottom = ptc->cyTabs * (ptc->iLastRow + 1);
            
        Tab_SmoothScrollWindow(ptc, 0, rcTabs.bottom - rcTabs.top, NULL, &rcTabs, NULL, NULL, SW_ERASE |SW_INVALIDATE, 1);
        UpdateWindow(ptc->ci.hwnd);
        // invalidate the little bit below the
        rcTabs.bottom += 2*g_cyEdge;
        rcTabs.top = rcTabs.bottom - 3 * g_cyEdge;
        Tab_InvalidateRect(ptc, &rcTabs, TRUE);
    }
    UpdateToolTipRects(ptc);
}

__inline int Tab_InterButtonGap(PTC ptc)
{
    ASSERT(Tab_DrawButtons(ptc));

    if (Tab_FlatButtons(ptc)) {
        return (g_cxEdge * 5);
    } else {
        return (g_cxEdge * 3)/2;
    }
}

//
//  BADNESS is the amount of unused space in the row
//
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

    // dx is the number of extra pixels that aren't part of the pitem->rc.
    // The non-button case of 2 * g_cxEdge is maniacally hard-coded
    // all over the place.  Change it at your own risk.
    if (Tab_DrawButtons(ptc))
        dx = Tab_InterButtonGap(ptc);
    else
        dx = 2 * g_cxEdge;              // inflate by g_cxEdge

    // if the size of the item is greaterthan the badness
    if (BADNESS(ptc, iCurLast) < (pitem->rc.right - pitem->rc.left + dx))
        return FALSE;

    // otherwise do it.
    // move this one down
    dx = pitem->rc.left - Tab_FastGetItemPtr(ptc, iPrevLast + 1)->rc.left;
    pitem->rc.left -= dx;
    pitem->rc.right -= dx;
    pitem->rc.top = pitem2->rc.top;
    pitem->rc.bottom = pitem2->rc.bottom;
    pitem->iRow = pitem2->iRow;

    // and move all the others over.
    dx = pitem->rc.right - pitem->rc.left;
    for(i = iPrevLast + 1 ; i <= iCurLast ; i++ ) {
        pitem = Tab_FastGetItemPtr(ptc, i);
        pitem->rc.left += dx;
        pitem->rc.right += dx;
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
            ASSERT(FALSE);
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
    int k;
    int n;
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

        // count the number of items
        for(n=0,k=i ; k < j ; k++ ) {
            pitem = Tab_FastGetItemPtr(ptc, k);
            if (!(pitem->dwState & TCIS_HIDDEN))
                n++;
        }

        // how much to fill
        hspace = ptc->cxTabs - Tab_FastGetItemPtr(ptc, j-1)->rc.right - g_cxEdge;
        dwidth = hspace/n;  // amount to increase each by.
        dremainder =  hspace % n; // the remnants
        moved = 0;  // how much we've moved already

        for( ; i < j ; i++ ) {
            int iHalf = dwidth/2;
            pitem = Tab_FastGetItemPtr(ptc, i);

            if (!(pitem->dwState & TCIS_HIDDEN)) {
                pitem->rc.left += moved;
                pitem->xLabel += iHalf;
                pitem->xImage += iHalf;
                moved += dwidth + (dremainder ? 1 : 0);
                if ( dremainder )  dremainder--;
                pitem->rc.right += moved;
            }
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
            ti.hwnd = ptc->ci.hwnd;
            ti.uId = i;
            SendMessage(ptc->hwndToolTips, TTM_DELTOOL, 0,
                        (LPARAM)(LPTOOLINFO)&ti);
        }
        Tab_FreeItem(ptc, Tab_FastGetItemPtr(ptc, i));
    }

    DPA_DeleteAllPtrs(ptc->hdpa);

    ptc->cxItem = RECOMPUTE;    // force recomputing of all tabs
    ptc->iSel = -1;
    ptc->iFirstVisible = 0;

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
    BOOL fChanged = FALSE;
    BOOL fFullRedraw = FALSE;

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
        fFullRedraw = TRUE;
        fChanged = TRUE;
#if defined(WINDOWS_ME)
        pitem->etoRtlReading = (mask & TCIF_RTLREADING) ?ETO_RTLREADING :0;
#endif
    }

    if (mask & TCIF_IMAGE) {

        if (pitem->iImage == -1 || 
            ptci->iImage == -1) {
            // went from no image to image... or vice versa
            // means needs full redraw
            
            fFullRedraw = TRUE;
        }
        pitem->iImage = ptci->iImage;
        fChanged = TRUE;
    }

    if ((mask & TCIF_PARAM) && ptc->cbExtra)
    {
        hmemcpy(pitem->DUMMYUNION_MEMBER(abExtra), &ptci->lParam, ptc->cbExtra);
    }
    
    if (mask & TCIF_STATE) {
        DWORD dwOldState = pitem->dwState;
        
        pitem->dwState = 
            (ptci->dwState & ptci->dwStateMask) | 
                (pitem->dwState & ~ptci->dwStateMask);
        
        if (dwOldState != pitem->dwState)
            fChanged = TRUE;

        if ((dwOldState ^ pitem->dwState) & TCIS_HIDDEN)
            fFullRedraw = TRUE;
        
        if ((ptci->dwStateMask & TCIS_BUTTONPRESSED) &&
            !(ptci->dwState & TCIS_BUTTONPRESSED)) {
            // if they turned OFF being pushed and we were pushed because of
            // selection, nuke it now.
            if (ptc->iNewSel == iItem) {
                ptc->iNewSel = -1;
                fChanged = TRUE;
            }
            
            if (ptc->iSel == iItem) {
                ChangeSel(ptc, -1, TRUE, FALSE);
                fChanged = TRUE;
            }
        }
    }

    if (fChanged) {
        if (Tab_FixedWidth(ptc) || !fFullRedraw) {
            Tab_InvalidateItem(ptc, iItem, FALSE);
        } else {
            ptc->cxItem = ptc->cyTabs = RECOMPUTE;
            RedrawAll(ptc, RDW_INVALIDATE | RDW_NOCHILDREN | RDW_ERASE);
        }
    }
    return TRUE;
}

void NEAR PASCAL Tab_OnMouseMove(PTC ptc, WPARAM fwKeys, int x, int y)
{
    POINT pt;
    int iHit;
    pt.x=x; pt.y=y;

    iHit = Tab_OnHitTest(ptc, x, y, NULL);
    
    if (Tab_HotTrack(ptc)) {
        if (iHit != ptc->iHot) {
            Tab_InvalidateItem(ptc, iHit, FALSE);
            Tab_InvalidateItem(ptc, ptc->iHot, FALSE);
            ptc->iHot = iHit;
        }
        
    }
    
    if (fwKeys & MK_LBUTTON && Tab_DrawButtons(ptc)) {

        UINT uFlags;

        if (ptc->iNewSel == -1)
            return;

        if (iHit == ptc->iNewSel) {
            uFlags = TCF_DRAWSUNKEN;

        } else {
            uFlags = 0;
        }

        if ((ptc->flags & TCF_DRAWSUNKEN) != uFlags) {

            // the bit isn't what it should be
            ptc->flags ^=  TCF_DRAWSUNKEN;

            // we need to invalidate on flat buttons because we go from one pixes to 2 pixel edge
            Tab_InvalidateItem(ptc, ptc->iNewSel, Tab_FlatButtons(ptc));
            
        }
    }
}

void NEAR PASCAL Tab_OnButtonUp(PTC ptc, int x, int y, BOOL fNotify)
{
    BOOL fAllow = TRUE;


    if (fNotify) {
        // pass NULL for parent because W95 queryied each time and some
        // folks reparent
        fAllow = !SendNotifyEx(NULL, ptc->ci.hwnd, NM_CLICK, NULL, ptc->ci.bUnicode);
    }

    if (Tab_DrawSunken(ptc)) {
        
        // nothing selected (its empty)
        // only do this if something is selected...
        // otherwise we still do need to go below and release capture though
        if (ptc->iNewSel != -1) {

            
            if (Tab_OnHitTest(ptc, x, y, NULL) == ptc->iNewSel) {

                int iNewSel = ptc->iNewSel;
                // use iNewSel instead of ptc->iNewSel because the SendNotify could have nuked us

                if (fAllow)
                    ChangeSel(ptc, iNewSel, TRUE, BMOVECURSORONCLICK);

                Tab_InvalidateItem(ptc, iNewSel, FALSE);

            } else {
                Tab_InvalidateItem(ptc, ptc->iNewSel, FALSE);
                Tab_InvalidateItem(ptc, ptc->iNewSel, FALSE);
            }

            // the changsel forces an updatewindow,
            // but we might have a border to unpaint(because of the TCF_DRAWSUNKEN
            // so we do another invalidate with just redraw 
            ptc->flags &= ~TCF_DRAWSUNKEN;
            ptc->iNewSel = -1;
        }
    }

    // don't worry about checking DrawButtons because TCF_MOUSEDOWN
    // wouldn't be set otherwise.
    if (ptc->flags & TCF_MOUSEDOWN) {
        int iOldSel = ptc->iNewSel;
        ptc->flags &= ~TCF_MOUSEDOWN; // do this before release  to avoid reentry
        ptc->iNewSel = -1;
        Tab_InvalidateItem(ptc, iOldSel, FALSE);
        CCReleaseCapture(&ptc->ci);
    }

}

int NEAR Tab_OnHitTest(PTC ptc, int x, int y, UINT FAR *lpuFlags)
{
    int i;
    int iLast = Tab_Count(ptc);
    RECT rc;
    POINT pt;
    UINT uTemp;


    rc.left = rc.right = x;
    rc.top = rc.bottom = y;
    Tab_DVFlipRect(ptc, &rc);
    pt.x = rc.left;
    pt.y = rc.top;

    if (!lpuFlags) lpuFlags = &uTemp;

    for (i = 0; i < iLast; i++) {
        LPTABITEM pitem = Tab_FastGetItemPtr(ptc, i);
        if (PtInRect(&pitem->rc, pt)) {
            
            // x now needs to be in pitem coordinates
            x -= pitem->rc.left;
           
            *lpuFlags = TCHT_ONITEM;
            if (!Tab_OwnerDraw(ptc)) {
                if ((x > pitem->xLabel) && x < pitem->xLabel + pitem->cxLabel) {
                    *lpuFlags = TCHT_ONITEMLABEL;
                } else if (HASIMAGE(ptc, pitem)) {
                    int cxImage, cyImage;
                    Tab_ImageList_GetIconSize(ptc, &cxImage, &cyImage);
                    if ((x > pitem->xImage) && (x < (pitem->xImage + cxImage)))
                        *lpuFlags = TCHT_ONITEMICON;
                }
            }
            return i;
        }
    }
    *lpuFlags = TCHT_NOWHERE;
    return -1;
}

void Tab_DeselectAll(PTC ptc, BOOL fExcludeFocus)
{
    int iMax = Tab_Count(ptc) - 1;
    int i;

    if (Tab_DrawButtons(ptc)) {
        for (i = iMax; i >= 0; i--)
        {
            LPTABITEM pitem;

            pitem = Tab_FastGetItemPtr(ptc, i);
            if (!fExcludeFocus || (pitem->dwState & TCIS_BUTTONPRESSED)) {
                TCITEM tci;
                tci.mask = TCIF_STATE;
                tci.dwStateMask = TCIS_BUTTONPRESSED;
                tci.dwState = 0;
                Tab_OnSetItem(ptc, i, &tci);
            }
        }
    }
}

void NEAR Tab_OnRButtonDown(PTC ptc, int x, int y, WPARAM keyFlags)
{
    int i;
    int iOldSel = -1;

    
    if (Tab_Vertical(ptc)) {
        
        if (y > ptc->cxTabs) 
            return;
        
    } else {

        if (x > ptc->cxTabs)
            return;     // outside the range of the visible tabs
    }

    i = Tab_OnHitTest(ptc, x,y, NULL); // we don't swap x,y here because OnHitTest will

    if (i != -1) {

        if (Tab_DrawButtons(ptc) && Tab_MultiSelect(ptc)) {
            TCITEM tci;
            tci.mask = TCIF_STATE;
            tci.dwStateMask = TCIS_BUTTONPRESSED;

            Tab_OnGetItem(ptc, i, &tci);

            // as with the listview, don't deselect anything on right button
            if (!(tci.dwState & TCIS_BUTTONPRESSED)) {
                if (!(GetAsyncKeyState(VK_CONTROL) < 0)) {
                    Tab_DeselectAll(ptc, FALSE);
                }

                // just toggle the pushed state.
                tci.dwState = TCIS_BUTTONPRESSED;
                Tab_OnSetItem(ptc, i, &tci);
            }
        }
    }
}

void NEAR Tab_OnLButtonDown(PTC ptc, int x, int y, WPARAM keyFlags)
{
    int i;
    int iOldSel = -1;

    
    if (Tab_Vertical(ptc)) {
        
        if (y > ptc->cxTabs) 
            return;
        
    } else {

        if (x > ptc->cxTabs)
            return;     // outside the range of the visible tabs
    }

    i = Tab_OnHitTest(ptc, x,y, NULL); // we don't swap x,y here because OnHitTest will

    if (i != -1) {
        
        if (Tab_MultiSelect(ptc) && (GetAsyncKeyState(VK_CONTROL) < 0) && Tab_DrawButtons(ptc) ) {
            // just toggle the pushed state.
            TCITEM tci;
            tci.mask = TCIF_STATE;
            tci.dwStateMask = TCIS_BUTTONPRESSED;
            
            Tab_OnGetItem(ptc, i, &tci);
            tci.dwState ^= TCIS_BUTTONPRESSED;
            Tab_OnSetItem(ptc, i, &tci);
            
        } else {
            
            iOldSel = ptc->iSel;

            if ((!Tab_FocusNever(ptc))
                && Tab_FocusOnButtonDown(ptc))
            {
                SetFocus(ptc->ci.hwnd);
            }

            if (Tab_DrawButtons(ptc)) {
                ptc->iNewSel = i;
                ptc->flags |= (TCF_DRAWSUNKEN|TCF_MOUSEDOWN);
                SetCapture(ptc->ci.hwnd);
                // we need to invalidate on flat buttons because we go from one pixes to 2 pixel edge
                Tab_InvalidateItem(ptc, i, Tab_FlatButtons(ptc));
            } else {
                iOldSel = ChangeSel(ptc, i, TRUE, BMOVECURSORONCLICK);
            }
        }
    }

    if ((!Tab_FocusNever(ptc)) &&
        (iOldSel == i))  // reselect current selection
        // this also catches i == -1 because iOldSel started as -1
    {
        SetFocus(ptc->ci.hwnd);
        UpdateWindow(ptc->ci.hwnd);
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
                hmemcpy(pitem->DUMMYUNION_MEMBER(abExtra), &ptci->lParam, ptc->cbExtra);
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

    Tab_GetClientRect(ptc, &rc);

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
    NoArrows:
        // Don't need arrows
        if (ptc->hwndArrows)
        {
            ShowWindow(ptc->hwndArrows, SW_HIDE);
            // BUGBUG:: This is overkill should only invalidate portion
            // that may be impacted, like the last displayed item..
            InvalidateRect(ptc->ci.hwnd, NULL, TRUE);
        }
        if (ptc->iFirstVisible > 0) {
#ifdef DEBUG
            if (!ptc->hwndArrows) {
                DebugMsg(DM_TRACE, TEXT("Scrolling where we wouldnt' have scrolled before"));
            }
#endif
            Tab_OnHScroll(ptc, NULL, SB_THUMBPOSITION, 0);
            // BUGBUG:: This is overkill should only invalidate portion
            // that may be impacted, like the last displayed item..
            InvalidateRect(ptc->ci.hwnd, NULL, TRUE);
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

        //  See how many tabs we have to remove until the last tab becomes
        //  fully visible.
        xSum = 0;                       // Number of pixels in removed tabs
        for (iMaxBtnVal=0; (ptc->cxTabs + xSum) < ptc->cxItem; iMaxBtnVal++)
        {
            pitem = Tab_GetItemPtr(ptc, iMaxBtnVal);
            if (!pitem)
                break;
            xSum += pitem->rc.right - pitem->rc.left;
        }

        // If we removed *all* the tabs, then put the last one back.
        // This happens if the last tab is so huge it doesn't fit into
        // the requisite space no matter how many tabs you remove.
        if (iMaxBtnVal >= Tab_Count(ptc))
        {
            iMaxBtnVal = Tab_Count(ptc) - 1;
        }

        //  If we don't need to remove any tabs, then we guessed wrong about
        //  arrows.  This can happen if there is exactly one tab that doesn't
        //  fit in the requisite space.  No arrow since there is nothing to
        //  scroll to!
        //
        if (iMaxBtnVal <= 0)
        {
            ptc->cxTabs = rc.right;     // Can use whole area to paint
            goto NoArrows;
        }

        if (!ptc->hwndArrows) {
            
            InvalidateRect(ptc->ci.hwnd, NULL, TRUE);
            ptc->hwndArrows = CreateUpDownControl
                (Tab_Vertical(ptc) ? (HDS_VERT | WS_CHILD) : (UDS_HORZ | WS_CHILD), 0, 0, 0, 0,
                 ptc->ci.hwnd, 1, HINST_THISDLL, NULL, iMaxBtnVal, 0,
                 ptc->iFirstVisible);
        }

        // DebugMsg(DM_TRACE, TEXT("Tabs_UpdateArrows iMax=%d\n\r"), iMaxBtnVal);
        if (ptc->hwndArrows)
        {
            rc.left = rc.right - cx;
            rc.top = ptc->cyTabs - cy;
            rc.bottom = ptc->cyTabs;
            Tab_VDFlipRect(ptc, &rc);
            
            if (fSizeChanged || !IsWindowVisible(ptc->hwndArrows))
                SetWindowPos(ptc->hwndArrows, NULL,
                             rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc),
                             SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);
            // Make sure the range is set
            SendMessage(ptc->hwndArrows, UDM_SETRANGE, 0,
                        MAKELPARAM(iMaxBtnVal, 0));

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

    ptc->cxItem = RECOMPUTE;    // force recomputing of all tabs

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
        ti.hwnd = ptc->ci.hwnd;
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
                    Tab_InvalidateItem(ptc, i, FALSE);
                } else {
                    pitem = Tab_GetItemPtr(ptc, i);
                    GetClientRect(ptc->ci.hwnd, &rcInval);

                    if (pitem) {
                        rcInval.top = pitem->rc.top;
                        if (ptc->iLastRow == 0) {
                            rcInval.left = pitem->rc.left;
                        }
                        Tab_UpdateArrows(ptc, FALSE);
                        RedrawWindow(ptc->ci.hwnd, &rcInval, NULL, RDW_INVALIDATE |RDW_NOCHILDREN);
                    }
                }

                MyNotifyWinEvent(EVENT_OBJECT_CREATE, ptc->ci.hwnd, OBJID_CLIENT, i+1);
                return i;
            }

        } else {

            // in tab mode Clear the selected item because it may move
            // and it sticks high a bit.
            if (ptc->iSel > i) {
                // update now because invalidate erases
                // and the redraw below doesn't.
                Tab_InvalidateItem(ptc, ptc->iSel, TRUE);
                UpdateWindow(ptc->ci.hwnd);
            }
        }

        RedrawAll(ptc, RDW_INVALIDATE | RDW_NOCHILDREN);

    }

    MyNotifyWinEvent(EVENT_OBJECT_CREATE, ptc->ci.hwnd, OBJID_CLIENT, i+1);
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

        ImageList_Remove(ptc->himl, iItem);
        for( i = Tab_Count(ptc)-1 ; i >= 0; i-- ) {
            pitem = Tab_FastGetItemPtr(ptc, i);
            if (pitem->iImage > iItem)
                pitem->iImage--;
            else if (pitem->iImage == iItem) {
                pitem->iImage = -1; // if we now don't draw something, inval
                Tab_InvalidateItem(ptc, i, FALSE);
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

    MyNotifyWinEvent(EVENT_OBJECT_DESTROY, ptc->ci.hwnd, OBJID_CLIENT, i+1);

    if (!Tab_DrawButtons(ptc) && (Tab_RedrawEnabled(ptc) || ptc->iSel >= i)) {
        // in tab mode, Clear the selected item because it may move
        // and it sticks high a bit.
        Tab_InvalidateItem(ptc, ptc->iSel, TRUE);
    }

    // if its fixed width, don't need to erase everything, just the last one
    if (Tab_FixedWidth(ptc)) {
        int j;

        uRedraw = RDW_INVALIDATE | RDW_NOCHILDREN;
        j = Tab_Count(ptc) -1;
        Tab_InvalidateItem(ptc, j, TRUE);

        // update optimization
        if (Tab_DrawButtons(ptc)) {

            if (i == Tab_Count(ptc) - 1) {
                rcInval.left = 0;
                uRedraw = 0;
            } else {
                pitem = Tab_GetItemPtr(ptc, i);
                GetClientRect(ptc->ci.hwnd, &rcInval);

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

    ptc->cxItem = RECOMPUTE;    // force recomputing of all tabs
    ptc->iLastTopRow = -1;
    if(ptc->hwndToolTips) {
        TOOLINFO ti;
        ti.cbSize = sizeof(ti);
        ti.hwnd = ptc->ci.hwnd;
        ti.uId = Tab_Count(ptc) ;
        SendMessage(ptc->hwndToolTips, TTM_DELTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
    }

    if (Tab_RedrawEnabled(ptc)) {
        if (rcInval.left == -1) {
            RedrawAll(ptc, uRedraw);
        } else {

            Tab_UpdateArrows(ptc, FALSE);
            if (uRedraw)
                RedrawWindow(ptc->ci.hwnd, &rcInval, NULL, uRedraw);
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
    
    if (mask & TCIF_STATE) {
        ptci->dwState = pitem->dwState & ptci->dwStateMask;
        
        // REViEW... maybe we should maintain the state in the statemask...
        if (ptci->dwStateMask & TCIS_BUTTONPRESSED) {
            if ((ptc->iSel == iItem) ||
                ((ptc->iNewSel == iItem) && Tab_DrawSunken(ptc))) {
                
                ptci->dwState |= TCIS_BUTTONPRESSED;
                
            }
        }
    }


    if ((mask & TCIF_PARAM) && ptc->cbExtra)
        hmemcpy(&ptci->lParam, pitem->DUMMYUNION_MEMBER(abExtra), ptc->cbExtra);

    if (mask & TCIF_IMAGE)
        ptci->iImage = pitem->iImage;
#ifdef WINDOWS_ME
    // A-MSADEK, BUGBUG:
    // TC_ITEM does not have room for querying TCIF_RTLREADING !!
    // it only allows you to set it.
    // This is a hack to return info about tab item reading order
    if((mask & TCIF_RTLREADING) && !(mask & TCIF_TEXT)) {
        if(pitem->etoRtlReading)
            ptci->cchTextMax = 1;
    }       
#endif // WINDOWS_ME

    return TRUE;
}

void NEAR PASCAL Tab_InvalidateItem(PTC ptc, int iItem, BOOL bErase)
{
    if (iItem != -1) {
        LPTABITEM pitem = Tab_GetItemPtr(ptc, iItem);

        if (pitem) {
            RECT rc = pitem->rc;
            if (rc.right > ptc->cxTabs)
                rc.right = ptc->cxTabs;  // don't invalidate past our end
            InflateRect(&rc, g_cxEdge, g_cyEdge);
            if (Tab_FlatButtons(ptc)) {
                rc.right += 2 * g_cxEdge;
            }
            Tab_InvalidateRect(ptc, &rc, bErase);
        }
    }
}

BOOL NEAR PASCAL RedrawAll(PTC ptc, UINT uFlags)
{
    if (ptc && Tab_RedrawEnabled(ptc)) {
        Tab_UpdateArrows(ptc, FALSE);
        RedrawWindow(ptc->ci.hwnd, NULL, NULL, uFlags);
        return TRUE;
    }
    return FALSE;
}

int NEAR PASCAL ChangeSel(PTC ptc, int iNewSel, BOOL bSendNotify,
    BOOL bUpdateCursorPos)
{
    BOOL bErase;
    int iOldSel;
    HWND hwnd;
    SIZE screenDelta;
    RECT rcT;

    if (iNewSel == ptc->iSel)
        return ptc->iSel;

    if (bUpdateCursorPos && Tab_OnGetItemRect(ptc, iNewSel, &rcT))
    {
        screenDelta.cx = rcT.left;
        screenDelta.cy = rcT.top;
    }
    else
    {
        screenDelta.cx = screenDelta.cy = 0;
        bUpdateCursorPos = FALSE;
    }

    hwnd = ptc->ci.hwnd;
    // make sure in range
    if (iNewSel < 0) {
        iOldSel = ptc->iSel;
        ptc->iSel = -1;
    } else if (iNewSel < Tab_Count(ptc)) {

        LPTABITEM pitem = Tab_GetItemPtr(ptc, iNewSel);
        ASSERT(pitem);
        if (!pitem)
            return -1;

        //
        // dont allow a hidden item to get the focus
        //
        // BUGBUG this is not 100% correct, focus will only
        // BUGBUG work right if hidden items are at the begining
        // BUGBUG or end (user will not be able to arrow past it)
        //
        // BUGBUG currenly this is not a bad restriction
        // BUGBUG only desk.cpl uses this flag, and it
        // BUGBUG always hides the last item.
        //
        // BUGBUG if we make this a general flag we will need to
        // BUGBUG fix this.
        //
        if (pitem->dwState & TCIS_HIDDEN)
            return -1;

        // make sure this is a change that's wanted
        if (bSendNotify)
        {
            // pass NULL for parent because W95 queryied each time and some
            // folks reparent
            if (SendNotifyEx(NULL, hwnd, TCN_SELCHANGING, NULL, ptc->ci.bUnicode))
                return ptc->iSel;
        }

        iOldSel = ptc->iSel;
        ptc->iSel = iNewSel;

        // See if we need to make sure the item is visible
        if (Tab_MultiLine(ptc)) {
            if( !Tab_DrawButtons(ptc) && ptc->iLastRow > 0 && iNewSel != -1) {
                // In multiLineTab Mode bring the row to the bottom.
                PutzRowToBottom(ptc, Tab_FastGetItemPtr(ptc, iNewSel)->iRow);
            }
        } else   {
            // In single line mode, slide things over to  show selection
            RECT rcClient;
            int xOffset = 0;
            int iNewFirstVisible = 0;

            GetClientRect(ptc->ci.hwnd, &rcClient);
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

    Tab_DeselectAll(ptc, TRUE);
    
    // repaint opt: we don't need to erase for buttons because their paint covers all.
    bErase = (!Tab_DrawButtons(ptc) || Tab_FlatButtons(ptc));
    if (bErase)
        UpdateWindow(hwnd);
    Tab_InvalidateItem(ptc, iOldSel, bErase);
    Tab_InvalidateItem(ptc, iNewSel, bErase);
    // mfc4.2 relies upon this update window.  they do some crap that
    // forces the window invalid bit to be false on the TCN_SELCHANGE and
    // thereby making us lose this update window
    UpdateWindow(hwnd);

    if (bUpdateCursorPos && Tab_OnGetItemRect(ptc, iNewSel, &rcT))
    {
        POINT ptCursor;

        screenDelta.cx = rcT.left - screenDelta.cx;
        screenDelta.cy = rcT.top  - screenDelta.cy;

        GetCursorPos(&ptCursor);
        SetCursorPos(ptCursor.x + screenDelta.cx, ptCursor.y + screenDelta.cy);
    }

    // if they are buttons, we send the message on mouse up
    if (bSendNotify)
    {
        // pass NULL for parent because W95 queryied each time and some
        // folks reparent
        SendNotifyEx(NULL, hwnd, TCN_SELCHANGE, NULL, ptc->ci.bUnicode);
    }

    MyNotifyWinEvent(EVENT_OBJECT_SELECTION, hwnd, OBJID_CLIENT, ptc->iSel+1);
    // We might've been destroyed during the notify, but GetFocus
    // couldn't possibly return our hwnd in that case, so we're still safe.
    if (GetFocus() == hwnd)
        MyNotifyWinEvent(EVENT_OBJECT_FOCUS, hwnd, OBJID_CLIENT, ptc->iSel+1);

    return iOldSel;
}



void NEAR PASCAL CalcTabHeight(PTC ptc, HDC hdc)
{
    BOOL bReleaseDC = FALSE;

    if (ptc->cyTabs == RECOMPUTE) {
        TEXTMETRIC tm;
        int iYExtra;
        int cx, cy = 0;

        if (!hdc)
        {
            bReleaseDC = TRUE;
            hdc = GetDC(NULL);
            SelectObject(hdc, ptc->hfontLabel);
        }

        GetTextMetrics(hdc, &tm);
        if (!ptc->fMinTabSet) {
            ptc->cxMinTab = tm.tmAveCharWidth * 6 + ptc->cxPad * 2;
        }
        ptc->cxyArrows = tm.tmHeight + 2 * g_cyEdge;

        if (ptc->himl)
            Tab_ImageList_GetIconSize(ptc, &cx, &cy);

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

        ptc->tmHeight = tm.tmHeight;
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
        ti.hwnd = ptc->ci.hwnd;
        ti.lpszText = LPSTR_TEXTCALLBACK;
        for ( i = 0, iMax = Tab_Count(ptc); i < iMax;  i++) {
            pitem = Tab_FastGetItemPtr(ptc, i);

            ti.uId = i;
            ti.rect = pitem->rc;
            Tab_VDFlipRect(ptc, &ti.rect);
            SendMessage(ptc->hwndToolTips, TTM_NEWTOOLRECT, 0, (LPARAM)((LPTOOLINFO)&ti));
        }
    }
}

void PASCAL Tab_GetTextExtentPoint(PTC ptc, HDC hdc, LPTSTR lpszText, int iCount, LPSIZE lpsize)
{
    TCHAR szBuffer[128];

    if (iCount < ARRAYSIZE(szBuffer) && !Tab_Vertical(ptc)) {
        StripAccelerators(lpszText, szBuffer, TRUE);
        lpszText = szBuffer;
        iCount = lstrlen(lpszText);
    }
    GetTextExtentPoint(hdc, lpszText, iCount, lpsize);
}

void PASCAL Tab_InvertRows(PTC ptc)
{
    int i;
    int yTop = g_cyEdge;
    int yNew;
    int iNewRow;
    
    // we want the first item to be on the bottom.
    for (i = Tab_Count(ptc) - 1; i >= 0; i--) {
        LPTABITEM pitem = Tab_FastGetItemPtr(ptc, i);
        iNewRow = ptc->iLastRow - pitem->iRow;
        yNew = yTop + iNewRow * ptc->cyTabs;
        pitem->iRow = iNewRow;
        OffsetRect(&pitem->rc, 0, yNew - pitem->rc.top);
    }
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

    if (ptc->cxItem == RECOMPUTE) {
        
        // if the font hasn't been created yet, let's do it now
        if (!ptc->hfontLabel)
            Tab_OnSetFont(ptc, NULL, FALSE);
        
        if (!hdc)
        {
            bReleaseDC = TRUE;
            hdc = GetDC(NULL);
            SelectObject(hdc, ptc->hfontLabel);
        }

        CalcTabHeight(ptc, hdc);

        if (Tab_DrawButtons(ptc)) {
            // start at the edge;
            xStart = 0;
            y = 0;
        } else {
            xStart = g_cxEdge;
            y = g_cyEdge;
        }
        x = xStart;

        for (i = 0; i < cItems; i++) {
            int cxImage = 0, cy;
            int cxBounds = 0;
            pitem = Tab_FastGetItemPtr(ptc, i);

            if (pitem->pszText) {
                Tab_GetTextExtentPoint(ptc, hdc, pitem->pszText, lstrlen(pitem->pszText), &siz);
            } else  {
                siz.cx = 0;
                siz.cy = 0;
            }

            pitem->cxLabel = siz.cx;

            // if there's an image, count that too
            if (HASIMAGE(ptc, pitem)) {
                Tab_ImageList_GetIconSize(ptc, &cxImage, &cy);

                cxImage += ptc->cxPad;
                siz.cx += cxImage;
            }
            cxBounds = siz.cx;

            if (Tab_FixedWidth(ptc)) {
                siz.cx = ptc->iTabWidth;
            } else {

                siz.cx += ptc->cxPad * 2;
                // Make sure the tab has a least a minimum width
                if (siz.cx < ptc->cxMinTab)
                    siz.cx = ptc->cxMinTab;
            }

            // handle hidden items
            if (pitem->dwState & TCIS_HIDDEN) {
                siz.cx = 0;
                siz.cy = 0;
            }

            // should we wrap?
            if (Tab_MultiLine(ptc)) {
                // two cases to wrap around:
                // case 2: is our right edge past the end but we ourselves
                //   are shorter than the width?
                // case 1: are we already past the end? (this happens if
                //      the previous line had only one item and it was longer
                //      than the tab's width.
                int iTotalWidth = ptc->cxTabs - g_cxEdge;
                if (x > iTotalWidth ||
                    (x+siz.cx >= iTotalWidth &&
                     (siz.cx < iTotalWidth))) {
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
                Tab_ForceIconLeft(ptc)) {

                pitem->xImage = ptc->cxPad;

            } else {
                // in fixed width mode center it
                pitem->xImage = (siz.cx - cxBounds)/2;
            }

            if (pitem->xImage < g_cxEdge)
                pitem->xImage = g_cxEdge;

            if (Tab_ForceIconLeft(ptc)) {
                // Center the text in the space remaining after the icon
                // The math here gets kind of crazy so I'm going to draw
                // a picture.
                //
                //     xImage
                //     |
                //   ->|   |<- cxImage
                //   +-----------------------------------------------+
                //   |  @@@           text text text                 |
                //   |  @@@                                          |
                //   +-----------------------------------------------+
                //   |<----------------- siz.cx -------------------->|
                //         |<-magic->|<--cxLabel--->|
                //                   xLabel
                //
                //  Therefore,
                //
                //  remaining space = siz.cx - cxImage - xImage - cxLabel.
                //  magic = remaining space / 2
                //  xLabel = xImage + cxImage + magic.
                //
                int cxImageTotal = pitem->xImage + cxImage;
                int cxRemaining = siz.cx - cxImageTotal - pitem->cxLabel;
                int cxMagic = cxRemaining / 2;
                pitem->xLabel = cxImageTotal + cxMagic;
            } else {
                // Place the text immediately after the icon
                pitem->xLabel = pitem->xImage + cxImage;

            }
            

            pitem->yImage = ptc->cyPad + ptc->cyIcon - (g_cyEdge/2);
            pitem->yLabel = ptc->cyPad + ptc->cyText - (g_cyEdge/2);

            x = pitem->rc.right;

            if (Tab_DrawButtons(ptc))
                x += Tab_InterButtonGap(ptc);
        }

        ptc->cxItem = x;        // total width of all tabs

        // if we added a line in non-button mode, we need to do a full refresh
        if (ptc->iLastRow != -1 &&
            ptc->iLastRow != iRow &&
            !Tab_DrawButtons(ptc)) {
            InvalidateRect(ptc->ci.hwnd, NULL, TRUE);
        }
        ptc->iLastRow = (cItems > 0) ? iRow : -1;

        if (Tab_MultiLine(ptc)) {
            if (!Tab_RaggedRight(ptc) && !Tab_FixedWidth(ptc))
                RightJustify(ptc);
            
            if (Tab_ScrollOpposite(ptc)) {
                Tab_InvertRows(ptc);
                                // if we have no selection, then the last row is the last top row
                                if (ptc->iSel == -1) 
                                        ptc->iLastTopRow = ptc->iLastRow;
            }

            if (!Tab_DrawButtons(ptc) && ptc->iSel != -1) {
                ptc->iLastTopRow = -1;
                PutzRowToBottom(ptc, Tab_FastGetItemPtr(ptc, ptc->iSel)->iRow);
            }

        } else if ( cItems > 0) {
            // adjust x's to the first visible
            int dx;
            pitem = Tab_GetItemPtr(ptc, ptc->iFirstVisible);
            if (pitem) {
                dx = -pitem->rc.left + g_cxEdge;
                for ( i = cItems - 1; i >=0  ; i--) {
                    pitem = Tab_FastGetItemPtr(ptc, i);
                    OffsetRect(&pitem->rc, dx, 0);
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

void NEAR PASCAL DoCorners(HDC hdc, LPRECT prc, PTC ptc, BOOL fBottom)
{
    RECT rc;
    COLORREF iOldColor;

    iOldColor = SetBkColor(hdc, g_clrBtnFace);

    if (fBottom) {
        // lower right;
        rc = *prc;
        rc.left = rc.right - 2;
        rc.top = rc.bottom - 3;
        
        Tab_ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL, ptc);
        rc.bottom--;
        Tab_DrawEdge(hdc, &rc, EDGE_RAISED, BF_SOFT | BF_DIAGONAL_ENDBOTTOMLEFT, ptc);

        
        // lower left

        rc = *prc;
        rc.right = rc.left + 2;
        rc.top = rc.bottom - 3;
        Tab_ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL, ptc);
        rc.bottom--;
        Tab_DrawEdge(hdc, &rc, EDGE_RAISED, BF_SOFT | BF_DIAGONAL_ENDTOPLEFT, ptc);
        
    } else {
        // upper right
        rc = *prc;
        rc.left = rc.right - 2;
        rc.bottom = rc.top + 3;
        Tab_ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL, ptc);
        rc.top++;
        Tab_DrawEdge(hdc, &rc, EDGE_RAISED, BF_SOFT | BF_DIAGONAL_ENDBOTTOMRIGHT, ptc);


        // upper left

        rc = *prc;
        rc.right = rc.left + 2;
        rc.bottom = rc.top + 3;
        Tab_ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL, ptc);
        rc.top++;
        Tab_DrawEdge(hdc, &rc, EDGE_RAISED, BF_SOFT | BF_DIAGONAL_ENDTOPRIGHT, ptc);
    }
}

void NEAR PASCAL RefreshArrows(PTC ptc, HDC hdc)
{
    RECT rcClip, rcArrows, rcIntersect;

    if (ptc->hwndArrows && IsWindowVisible(ptc->hwndArrows)) {

        GetClipBox(hdc, &rcClip);
        GetWindowRect(ptc->hwndArrows, &rcArrows);
        MapWindowRect(NULL, ptc->ci.hwnd, &rcArrows);
        if (IntersectRect(&rcIntersect, &rcClip, &rcArrows))
            RedrawWindow(ptc->hwndArrows, NULL, NULL, RDW_INVALIDATE);
    }
}

// BUGBUG: the way highlighted tabs (TCIS_HIGHLIGHTED) are drawn right
// now are incredibly ugly. This should be fixed.
//
void NEAR PASCAL DrawBody(HDC hdc, PTC ptc, LPTABITEM pitem, LPRECT lprc, int i,
                          BOOL fTransparent, int dx, int dy)
{
    BOOL fSelected = (i == ptc->iSel);

    if (i == ptc->iHot) {
        if ( !Tab_FlatButtons(ptc) ) {
            SetTextColor(hdc, GetSysColor(COLOR_HOTLIGHT));
        }
    }
    
    if (Tab_OwnerDraw(ptc)) {
        DRAWITEMSTRUCT dis;
        WORD wID = (WORD) GetWindowID(ptc->ci.hwnd);

        dis.CtlType = ODT_TAB;
        dis.CtlID = wID;
        dis.itemID = i;
        dis.itemAction = ODA_DRAWENTIRE;
        if (fSelected)
            dis.itemState = ODS_SELECTED;
        else
            dis.itemState = 0;
        dis.hwndItem = ptc->ci.hwnd;
        dis.hDC = hdc;
        dis.rcItem = *lprc;
        Tab_VDFlipRect(ptc, &dis.rcItem);
        dis.itemData =
            (ptc->cbExtra <= sizeof(LPARAM)) ?
                (DWORD)pitem->DUMMYUNION_MEMBER(lParam) : (ULONG_PTR)(LPBYTE)&pitem->DUMMYUNION_MEMBER(abExtra);

        SendMessage( ptc->ci.hwndParent , WM_DRAWITEM, wID,
                    (LPARAM)(DRAWITEMSTRUCT FAR *)&dis);

    } else {
        // draw the text and image
        // draw even if pszText == NULL to blank it out
        int xLabel;
        int xIcon;
        BOOL fUseDrawText = FALSE;
        if (pitem->pszText) {

            // only use draw text if there's any underlining to do.
            // Draw text does not support vertical drawing, so only do this in horz mode
            if (!Tab_Vertical(ptc) &&
                StrChr(pitem->pszText, CH_PREFIX)) {
                fUseDrawText = TRUE;
            }
        }

        // DrawTextEx will not clear the entire area, so we need to.
        // or if there's no text, we need to blank it out
        if ((fUseDrawText || !pitem->pszText) && !fTransparent)
            Tab_ExtTextOut(hdc, 0, 0,
                       ETO_OPAQUE, lprc, NULL, 0, NULL, ptc);

#if defined(WINDOWS_ME)
#define ETO_ME_CLIPPED (ETO_CLIPPED | pitem->etoRtlReading | (ptc->ci.dwExStyle & WS_EX_RTLREADING ? ETO_RTLREADING : 0 ))
#else 
#define ETO_ME_CLIPPED ETO_CLIPPED
#endif
        xLabel = pitem->rc.left + pitem->xLabel + dx;
        xIcon = pitem->rc.left + pitem->xImage + dx;
        
        if (pitem->pszText) {

            int xVertOffset = 0;

            int oldMode;
            COLORREF oldBkColor;
            COLORREF oldTextColor;

            if (Tab_Vertical(ptc) && !Tab_Bottom(ptc)) {
                
                // add this offset because we need to draw from the bottom up
                xLabel += pitem->cxLabel;
                
                // if we're drawing vertically (on the left)
                // the icon needs to go below (flipped coordinate, on the right)
                if (HASIMAGE(ptc, pitem)) {
                    int cxIcon;
                    int cyIcon;
                    int xLabelNew;
                    
                    Tab_ImageList_GetIconSize(ptc, &cxIcon, &cyIcon);
                    xLabelNew = xIcon + pitem->cxLabel;
                    xIcon = xLabel - cxIcon;
                    xLabel = xLabelNew;
                }                
            }

            if (pitem->dwState & TCIS_HIGHLIGHTED)
            {
                oldMode = SetBkMode (hdc, OPAQUE);
                oldBkColor = SetBkColor (hdc, g_clrHighlight);
                oldTextColor = SetTextColor (hdc, g_clrHighlightText);
            }
            if (fUseDrawText) {
                DRAWTEXTPARAMS dtp;
                int topPrev;
                dtp.cbSize = sizeof(DRAWTEXTPARAMS);
                dtp.iLeftMargin = xLabel - lprc->left;
                dtp.iRightMargin = 0;

                // There is no dtp.iTopMargin so we have to adjust the
                // rectangle instead.  The opaqueing has already been done,
                // so isn't not a problem if we "miss" some pixels since
                // they've already been erased.
                topPrev = lprc->top;
                lprc->top = pitem->rc.top + pitem->yLabel + dy;

#ifdef WINNT
                Tab_DrawTextEx(hdc, pitem->pszText, -1, lprc, DT_SINGLELINE | DT_TOP, &dtp, ptc);
#else   // WINNT
                
#   if defined(WINDOWS_ME)
                Tab_DrawTextEx(hdc, pitem->pszText, -1, lprc,
                         (pitem->etoRtlReading ?DT_RTLREADING :0) |
                         (ptc->ci.dwExStyle & WS_EX_RTLREADING ? DT_RTLREADING : 0 ) |
                         DT_END_ELLIPSIS | DT_SINGLELINE | DT_TOP, &dtp, ptc);
#   else  // WINDOWS_ME
                Tab_DrawTextEx(hdc, pitem->pszText, -1, lprc, DT_END_ELLIPSIS | DT_SINGLELINE | DT_TOP, &dtp, ptc);
#   endif  // WINDOWS_ME
#endif // WINNT

                // Undo our changes to lprc before anybody (else) notices.
                lprc->top = topPrev;
            } else {
                UINT uETOFlags = ETO_ME_CLIPPED;

                if (!fTransparent || (pitem->dwState & TCIS_HIGHLIGHTED))
                    uETOFlags |= ETO_OPAQUE;

                Tab_ExtTextOut(hdc, xLabel, pitem->rc.top + pitem->yLabel + dy,
                    uETOFlags, lprc, pitem->pszText, lstrlen(pitem->pszText),
                    NULL, ptc);
            }

            if (pitem->dwState & TCIS_HIGHLIGHTED) {
                SetBkMode(hdc, oldMode);
                SetBkColor (hdc, oldBkColor);
                SetTextColor (hdc, oldTextColor);
            }

#ifdef WANNA_BLUR_ME
            // blurring
            if (fSelected) {
                if (!fTransparent) {
                    SetBkMode(hdc, TRANSPARENT);

                    // guaranteed to be buttons if we got here
                    // becaues can't iSel==i is rejected for tabs in this loop
                    Tab_ExtTextOut(hdc, xLabel +  1, pitem->rc.top + pitem->yLabel + dy,
                               ETO_ME_CLIPPED, lprc, pitem->pszText, lstrlen(pitem->pszText),
                               NULL, ptc);

                    SetBkMode(hdc, OPAQUE);
                }
            }
#endif

        }

        if (HASIMAGE(ptc, pitem)) {
            UINT uFlags = fTransparent ? ILD_TRANSPARENT : ILD_NORMAL;

            if (pitem->dwState & TCIS_HIGHLIGHTED)
                uFlags |= ILD_BLEND50;

            Tab_ImageList_Draw(ptc, pitem->iImage, hdc, xIcon,
                pitem->rc.top + pitem->yImage + dy, uFlags);
        }

    }
    if (i == ptc->iHot) {
        if ( !Tab_FlatButtons(ptc) ) {
            SetTextColor(hdc, g_clrBtnText);
        }
    }
}

void NEAR PASCAL Tab_DrawItemFrame(PTC ptc, HDC hdc, UINT edgeType, LPTABITEM pitem, int i)
{
    UINT uWhichEdges;
    BOOL fBottom = FALSE;

    if (Tab_DrawButtons(ptc)) {

        if (Tab_FlatButtons(ptc)) {
            if ((edgeType == EDGE_SUNKEN) ||
                (edgeType == BDR_RAISEDINNER)) {
                uWhichEdges = BF_RECT;
            } else {

                if ((ptc->ci.style & TCS_HOTTRACK) &&
                    (i == ptc->iHot))  {
                    edgeType = BDR_RAISEDINNER;
                    uWhichEdges = BF_RECT;
                } else {

                    HPEN hPen, hOldPen;
                    RECT rcEdge;

                    CopyRect (&rcEdge, &pitem->rc);
                    //InflateRect (&rcEdge, -g_cxEdge, -g_cyEdge);

                    hPen = CreatePen (PS_SOLID, 2 * g_cyEdge, GetSysColor(COLOR_3DFACE));
                    hOldPen = SelectObject (hdc, hPen);

                    //
                    // Remove any border in the x direction
                    //

                    MoveToEx (hdc, rcEdge.left, rcEdge.top, NULL);
                    LineTo (hdc, rcEdge.right, rcEdge.top);
                    MoveToEx (hdc, rcEdge.left, rcEdge.bottom, NULL);
                    LineTo (hdc, rcEdge.right, rcEdge.bottom);

                    SelectObject (hdc, hOldPen);
                    DeleteObject (hPen);

                    //
                    // Remove any border in the y direction
                    //

                    hPen = CreatePen (PS_SOLID, 2 * g_cxEdge, GetSysColor(COLOR_3DFACE));
                    hOldPen = SelectObject (hdc, hPen);

                    MoveToEx (hdc, rcEdge.left, rcEdge.top, NULL);
                    LineTo (hdc, rcEdge.left, rcEdge.bottom);
                    MoveToEx (hdc, rcEdge.right, rcEdge.top, NULL);
                    LineTo (hdc, rcEdge.right, rcEdge.bottom);

                    SelectObject (hdc, hOldPen);
                    DeleteObject (hPen);

                    goto DrawCorners;
                }
            }
        } else {
            uWhichEdges = BF_RECT | BF_SOFT;
        }
    } else {
        uWhichEdges = BF_LEFT | BF_TOP | BF_RIGHT | BF_SOFT;
        
        if (Tab_ScrollOpposite(ptc)) {
            ASSERT(ptc->iLastTopRow != -1);
            if (Tab_IsItemOnBottom(ptc, pitem)) {
                fBottom = TRUE;
                uWhichEdges = BF_LEFT | BF_BOTTOM | BF_RIGHT | BF_SOFT;
            }
        }
    }
    
    Tab_DrawEdge(hdc, &pitem->rc, edgeType, uWhichEdges, ptc);
    
DrawCorners:

    if (!Tab_DrawButtons(ptc)) {
        DoCorners(hdc, &pitem->rc, ptc, fBottom);
    } else {
        if (Tab_FlatButtons(ptc) && Tab_FlatSeparators(ptc)) {
           RECT rcEdge;

           CopyRect (&rcEdge, &pitem->rc);
           rcEdge.right += (3 * g_cxEdge);
           DrawEdge(hdc, &rcEdge, EDGE_ETCHED, BF_RIGHT);
        }
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
    HWND hwnd = ptc->ci.hwnd;
    HBRUSH hbrOld = NULL;

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
    if (!ptc->hfontLabel)
        Tab_OnSetFont(ptc, NULL, FALSE);
    SelectObject(hdc, ptc->hfontLabel);
    CalcPaintMetrics(ptc, hdc);

    // now put it in our native orientation if it was vertical
    Tab_DFlipRect(ptc, &rcClient);
    
    Tab_OnAdjustRect(ptc, FALSE, &rcClient);
    InflateRect(&rcClient, g_cxEdge * 2, g_cyEdge * 2);
    rcClient.top += g_cyEdge;

    if(!Tab_DrawButtons(ptc)) {
        DebugMsg(DM_TRACE, TEXT("Drawing at %d %d %d %d"), rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
        Tab_DrawEdge(hdc, &rcClient, EDGE_RAISED, BF_SOFT | BF_RECT, ptc);
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
        Tab_DVFlipRect(ptc, &rcClipBox);

        // draw all but the selected item
        for (i = ptc->iFirstVisible; i < cItems; i++) {

            pitem = Tab_FastGetItemPtr(ptc, i);

            if (pitem->dwState & TCIS_HIDDEN)
                continue;

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

                    
                    rcBody = pitem->rc;

                    // Draw the edge around each item
                    if(Tab_DrawButtons(ptc) &&
                       ((ptc->iNewSel == i && Tab_DrawSunken(ptc)) ||
                        (ptc->iSel == i) ||
                        (pitem->dwState & TCIS_BUTTONPRESSED))) {

                        dx = g_cxEdge/2;
                        dy = g_cyEdge/2;
                        if (Tab_FlatButtons(ptc) &&
                            (ptc->iNewSel == i && Tab_DrawSunken(ptc))) {
                            edgeType = BDR_RAISEDINNER;
                        } else {
                            edgeType =  EDGE_SUNKEN;
                        }

                    } else
                        edgeType = EDGE_RAISED;

                    if (Tab_DrawButtons(ptc) && !Tab_OwnerDraw(ptc)) {

                        // if drawing buttons, show selected by dithering  background
                        // which means we need to draw transparent.
                        if (ptc->iSel == i) {
                            fnNewMode = TRANSPARENT;
                            SetBkMode(hdc, TRANSPARENT);
                            hbrOld = SelectObject(hdc, g_hbrMonoDither);
                            SetTextColor(hdc, g_clrBtnHighlight);
                            Tab_PatBlt(hdc, pitem->rc.left, pitem->rc.top, pitem->rc.right - pitem->rc.left,
                                       pitem->rc.bottom - pitem->rc.top, PATCOPY, ptc);
                            SetTextColor(hdc, g_clrBtnText);
                        }
                    }

                    InflateRect(&rcBody, -g_cxEdge, -g_cyEdge);
                    if (!Tab_DrawButtons(ptc)) {
                        
                        // move the bottom (or top) by an edge to draw where the tab doesn't have an edge.
                        // by doing this, we fill the entire area and don't need to do as many inval with erase
                        if (Tab_IsItemOnBottom(ptc, pitem)) {
                            rcBody.top -= g_cyEdge;
                        } else {
                            rcBody.bottom += g_cyEdge;
                        }
                    }
                    DrawBody(hdc, ptc, pitem, &rcBody, i, fnNewMode == TRANSPARENT,
                             dx, dy);


                    Tab_DrawItemFrame(ptc, hdc, edgeType, pitem, i);

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
            rcBody = pitem->rc;

            if (!Tab_DrawButtons(ptc)) {
                UINT uWhichEdges;
                
                InflateRect(&rcBody, g_cxEdge, g_cyEdge);

                if (IntersectRect(&rcTest, &rcClipBox, &rcBody)) {

                    DrawBody(hdc, ptc, pitem, &rcBody, ptc->iSel, FALSE, 0,-g_cyEdge);

                    rcBody.bottom--;  //because of button softness
                    Tab_DrawEdge(hdc, &rcBody, EDGE_RAISED, 
                                 BF_LEFT | BF_TOP | BF_RIGHT | BF_SOFT,
                                 ptc);
                    DoCorners(hdc, &rcBody, ptc, FALSE);

                    // draw that extra bit on the left or right side
                    // if we're on the edge
                    rcBody.bottom++;
                    rcBody.top = rcBody.bottom-1;
                    if (rcBody.right == rcClient.right) {
                        uWhichEdges = BF_SOFT | BF_RIGHT;

                    } else if (rcBody.left == rcClient.left) {
                        uWhichEdges = BF_SOFT | BF_LEFT;
                    } else
                        uWhichEdges = 0;

                    if (uWhichEdges)
                        Tab_DrawEdge(hdc, &rcBody, EDGE_RAISED, uWhichEdges, ptc);
                }
            }

        }

        // draw the focus rect
        if (GetFocus() == hwnd) {

            if (!pitem && (ptc->iNewSel != -1)) {
                pitem = Tab_GetItemPtr(ptc, ptc->iNewSel);
            }

            if (pitem
#ifdef KEYBOARDCUES
                && !(CCGetUIState(&(ptc->ci))& UISF_HIDEFOCUS)
#endif
                )
            {
                rcBody = pitem->rc;
                if (Tab_DrawButtons(ptc))
                    InflateRect(&rcBody, -g_cxEdge, -g_cyEdge);
                else
                    InflateRect(&rcBody, -(g_cxEdge/2), -(g_cyEdge/2));
                Tab_DrawFocusRect(hdc, &rcBody, ptc);
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
    ASSERT(0);
    return iStart;
}

void NEAR PASCAL Tab_SetCurFocus(PTC ptc, int iStart)
{

    if (Tab_DrawButtons(ptc)) {
        if ((iStart >= 0) && (iStart < Tab_Count(ptc)) && (ptc->iNewSel != iStart)) {
            if (ptc->iNewSel != -1)
                Tab_InvalidateItem(ptc, ptc->iNewSel, FALSE);
            Tab_InvalidateItem(ptc, iStart, FALSE);
            ptc->iNewSel = iStart;
            ptc->flags |= TCF_DRAWSUNKEN;
            
            if (!Tab_MultiLine(ptc)) {
                // scroll into view if necessary
                RECT rc;
                do {
                    Tab_OnGetItemRect(ptc, iStart, &rc);
                    if (rc.right > ptc->cxTabs) {
                        Tab_OnHScroll(ptc, NULL, SB_THUMBPOSITION, ptc->iFirstVisible + 1);
                    } else if (rc.left < 0) {
                        Tab_OnHScroll(ptc, NULL, SB_THUMBPOSITION, iStart);
                        break;
                    } else {
                        break;
                    }
                } while (1);
            }
            
            CCSendNotify(&ptc->ci, TCN_FOCUSCHANGE, NULL);
            MyNotifyWinEvent(EVENT_OBJECT_FOCUS, ptc->ci.hwnd, OBJID_CLIENT,
                iStart+1);
        }
    } else
    {
        int iOld = ptc->iSel;

        ChangeSel(ptc, iStart, TRUE, FALSE);

        if ((iOld != ptc->iSel) && (GetFocus() == ptc->ci.hwnd))
            MyNotifyWinEvent(EVENT_OBJECT_FOCUS, ptc->ci.hwnd, OBJID_CLIENT,
                ptc->iSel+1);
    }
}

void NEAR PASCAL Tab_OnKeyDown(PTC ptc, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    int iStart;
    TC_KEYDOWN nm;

    // Notify
    nm.wVKey = (WORD) vk;
    nm.flags = flags;
    // pass NULL for parent because W95 queryied each time and some
    // folks reparent
    SendNotifyEx(NULL, ptc->ci.hwnd, TCN_KEYDOWN, &nm.hdr, ptc->ci.bUnicode);

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

    vk = RTLSwapLeftRightArrows(&ptc->ci, vk);

    if (Tab_Vertical(ptc)) {
        // remap arrow keys if we're in vertial mode
        switch(vk) {
        case VK_LEFT:
            vk = VK_DOWN;
            break;
            
        case VK_RIGHT:
            vk = VK_UP;
            break;
            
        case VK_DOWN:
            vk = VK_RIGHT;
            break;
            
        case VK_UP:
            vk = VK_LEFT;
            break;
        }
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
        ChangeSel(ptc, iStart, TRUE, FALSE);
        ptc->iNewSel = -1;
        ptc->flags &= ~TCF_DRAWSUNKEN;
#ifdef KEYBOARDCUES
        //notify of navigation key usage
        CCNotifyNavigationKeyUsage(&(ptc->ci), UISF_HIDEFOCUS | UISF_HIDEACCEL);
#endif
        return;

    default:
        return;
    }

    if (iStart < 0)
        iStart = 0;

    Tab_SetCurFocus(ptc, iStart);
#ifdef KEYBOARDCUES
    //notify of navigation key usage
    CCNotifyNavigationKeyUsage(&(ptc->ci), UISF_HIDEFOCUS | UISF_HIDEACCEL);
#endif
}

void NEAR Tab_Size(PTC ptc)
{
    ptc->cxItem = RECOMPUTE;
    Tab_UpdateArrows(ptc, TRUE);
}

BOOL NEAR PASCAL Tab_OnGetItemRect(PTC ptc, int iItem, LPRECT lprc)
{
    LPTABITEM pitem = Tab_GetItemPtr(ptc, iItem);
    BOOL fRet = FALSE;

    if (lprc) {
        CalcPaintMetrics(ptc, NULL);
        if (pitem) {

            // Make sure all the item rects are up-to-date

            *lprc = pitem->rc;
            fRet = TRUE;
        } else {
            lprc->top = 0;
            lprc->bottom = ptc->cyTabs;
            lprc->right = 0;
            lprc->left = 0;
        }

        Tab_VDFlipRect(ptc, lprc);
        
    }
    return fRet;
}

void PASCAL Tab_StyleChanged(PTC ptc, UINT gwl,  LPSTYLESTRUCT pinfo)
{
#define STYLE_MASK   (TCS_BUTTONS | TCS_VERTICAL | TCS_MULTILINE | TCS_RAGGEDRIGHT | TCS_FIXEDWIDTH | TCS_FORCELABELLEFT | TCS_FORCEICONLEFT | TCS_BOTTOM | TCS_RIGHT | TCS_FLATBUTTONS | TCS_OWNERDRAWFIXED | TCS_HOTTRACK)
    if (ptc && (gwl == GWL_STYLE)) {

        DWORD dwChanged = (ptc->ci.style & STYLE_MASK) ^ (pinfo->styleNew & STYLE_MASK);
        // special case.  this is "Insider Trading" app (by papyrus, now kanisa).  they set the 3 on the low byte in ie3 comctl32 when it
        // had no meaning anyways. so we bail on that.
        if (ptc->ci.style == 0x50004000 && pinfo->styleNew == 0x54004003)
            return;
        if (dwChanged) {
            ptc->ci.style = (ptc->ci.style & ~STYLE_MASK)  | (pinfo->styleNew & STYLE_MASK);

            // make sure we don't have invalid bits set
            if (!Tab_FixedWidth(ptc)) {
                ptc->ci.style &= ~(TCS_FORCEICONLEFT | TCS_FORCELABELLEFT);
            }
            ptc->cxItem = RECOMPUTE;
            ptc->cyTabs = RECOMPUTE;
            
            //if the left/right orientation changed
            // we need to re-create the font (if we own it)
            // becaus the text orientation needs to flip by 180
            if ((dwChanged & TCS_VERTICAL) ||
                ((dwChanged & TCS_RIGHT) && Tab_Vertical(ptc))) {
                if (!(ptc->flags & TCF_FONTSET))
                    Tab_OnSetFont(ptc, NULL, FALSE);
            }
                
            if (Tab_RedrawEnabled(ptc))
                Tab_UpdateArrows(ptc, TRUE);
            RedrawAll(ptc, RDW_ERASE | RDW_INVALIDATE);
        }

#define FOCUS_MASK (TCS_FOCUSONBUTTONDOWN | TCS_FOCUSNEVER)
        if ( (ptc->ci.style &  FOCUS_MASK) ^ (pinfo->styleNew & FOCUS_MASK)) {
            ptc->ci.style = (ptc->ci.style & ~FOCUS_MASK)  | (pinfo->styleNew & FOCUS_MASK);
        }
    }
 #ifdef WINDOWS_ME
    if (gwl == GWL_EXSTYLE) {
        ptc->ci.dwExStyle &= ~WS_EX_RTLREADING;
        ptc->ci.dwExStyle |= (pinfo->styleNew & WS_EX_RTLREADING);       
       }
#endif //WINDOWS_ME
}


DWORD Tab_ExtendedStyleChange(PTC ptc, DWORD dwNewStyle, DWORD dwExMask)
{
    DWORD dwOldStyle = ptc->dwStyleEx;

    if (ptc->hDragProxy) {
        DestroyDragProxy(ptc->hDragProxy);
        ptc->hDragProxy = NULL;
    }

    if (dwExMask) 
        dwNewStyle = (ptc->dwStyleEx & ~ dwExMask) | (dwNewStyle & dwExMask);
    
    ptc->dwStyleEx = dwNewStyle;

    // do any invalidation or whatever is needed here.
    if ((dwOldStyle ^ dwNewStyle) & TCS_EX_FLATSEPARATORS) {
        InvalidateRect (ptc->ci.hwnd, NULL, TRUE);
    }

    if (ptc->dwStyleEx & TCS_EX_REGISTERDROP)
        ptc->hDragProxy = CreateDragProxy(ptc->ci.hwnd, TabDragCallback, TRUE);

    return dwOldStyle;
}


//
// APPCOMPAT Assumes that the tab control is on top.  Returns bogus values for
// left, bottom or right.  For app compat reasons, we can't change this
// buggy behavior. (Apps might be relying on the wrong values and fixing them
// up, so if we fix the function, they end up trying to "fix" something that
// wasn't broken, thereby breaking it.)  But we might want to add
// TCM_ADJUSTRECT2 that can handle the left/right/bottom cases.
//
void NEAR PASCAL Tab_OnAdjustRect(PTC ptc, BOOL fGrow, LPRECT prc)
{
    int idy;
    CalcPaintMetrics(ptc, NULL);

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
    
    if (fGrow) {
        // calc a larger rect from the smaller
        prc->top -= idy;
        InflateRect(prc, g_cxEdge * 2, g_cyEdge * 2);
    } else {
        prc->top += idy;
        // given the bounds, calc the "client" area
        InflateRect(prc, -g_cxEdge * 2, -g_cyEdge * 2);
    }

    if (Tab_ScrollOpposite(ptc)) {
        // the sizes are the same, it's just offset wrong vertically
        idy = ptc->cyTabs * (ptc->iLastRow - ptc->iLastTopRow);
        ASSERT(ptc->iLastTopRow != -1);

        if (!fGrow) {
            idy *= -1;
        }
        DebugMsg(DM_TRACE, TEXT("Tab_AdjustRect %d %d %d %d"), prc->left, prc->top, prc->right, prc->bottom);
        OffsetRect(prc, 0, idy);
        DebugMsg(DM_TRACE, TEXT("Tab_AdjustRect %d %d %d %d"), prc->left, prc->top, prc->right, prc->bottom);
    }
}

LRESULT CALLBACK Tab_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PTC ptc = (PTC)GetWindowInt((hwnd), 0);

    if (ptc) {
        if ((uMsg >= WM_MOUSEFIRST) && (uMsg <= WM_MOUSELAST) &&
            Tab_HotTrack(ptc) && !ptc->fTrackSet) {

            TRACKMOUSEEVENT tme;

            ptc->fTrackSet = TRUE;
            tme.cbSize = sizeof(tme);
            tme.hwndTrack = ptc->ci.hwnd;
            tme.dwFlags = TME_LEAVE;

            TrackMouseEvent(&tme);
        }
    } else if (uMsg != WM_CREATE)
        goto DoDefault;
    
    switch (uMsg) {

    HANDLE_MSG(ptc, WM_HSCROLL, Tab_OnHScroll);
    
    case WM_MOUSELEAVE:
        Tab_InvalidateItem(ptc, ptc->iHot, FALSE);
        ptc->iHot = -1;
        ptc->fTrackSet = FALSE;
        break;

    case WM_CREATE:
        InitGlobalColors();
        ptc = (PTC)NearAlloc(sizeof(TC));
        if (!ptc)
            return -1;  // fail the window create

        SetWindowPtr(hwnd, 0, ptc);
        CIInitialize(&ptc->ci, hwnd, (LPCREATESTRUCT)lParam);

        if (!Tab_OnCreate(ptc))
            return -1;

        break;

    case WM_DESTROY:
        Tab_OnDestroy(ptc);
        break;

    case WM_SIZE:
        Tab_Size(ptc);
        break;

    case WM_SYSCOLORCHANGE:
        InitGlobalColors();
        if (!(ptc->flags & TCF_FONTSET))
            Tab_OnSetFont(ptc, NULL, FALSE);
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
        Tab_StyleChanged(ptc, (UINT) wParam, (LPSTYLESTRUCT)lParam);
        break;

    case WM_MOUSEMOVE:
        RelayToToolTips(ptc->hwndToolTips, hwnd, uMsg, wParam, lParam);
        Tab_OnMouseMove(ptc, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

    case WM_LBUTTONDOWN:
        RelayToToolTips(ptc->hwndToolTips, hwnd, uMsg, wParam, lParam);
        Tab_OnLButtonDown(ptc, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
        break;
        
    case WM_LBUTTONDBLCLK:
        if (Tab_DrawButtons(ptc)) {
            MSG msg;
            // on the double click, grab capture until we get the lbutton up and
            // eat it.
            SetCapture(ptc->ci.hwnd);
            while (GetCapture() == ptc->ci.hwnd && 
                   !PeekMessage(&msg, ptc->ci.hwnd, WM_LBUTTONUP, WM_LBUTTONUP, PM_REMOVE)) 
            {
            }
            CCReleaseCapture(&ptc->ci);
        }
        break;

    case WM_MBUTTONDOWN:
        SetFocus(hwnd);
        break;

    case WM_RBUTTONDOWN:
        Tab_OnRButtonDown(ptc, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
        break;
        
    case WM_RBUTTONUP:
        // pass NULL for parent because W95 queryied each time and some
        // folks reparent
        if (!SendNotifyEx(NULL, ptc->ci.hwnd, NM_RCLICK, NULL, ptc->ci.bUnicode))
            goto DoDefault;
        break;

    case WM_CAPTURECHANGED:
        lParam = -1L; // fall through to LBUTTONUP

    case WM_LBUTTONUP:
        if (uMsg == WM_LBUTTONUP) {
            RelayToToolTips(ptc->hwndToolTips, hwnd, uMsg, wParam, lParam);
        }

        Tab_OnButtonUp(ptc, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (uMsg == WM_LBUTTONUP));
        break;

#ifdef KEYBOARDCUES
    case WM_SYSKEYDOWN:
        //notify of navigation key usage
        if (HIWORD(lParam) & KF_ALTDOWN)
            CCNotifyNavigationKeyUsage(&(ptc->ci), UISF_HIDEFOCUS | UISF_HIDEACCEL);
        goto DoDefault;
#endif

    case WM_KEYDOWN:
        HANDLE_WM_KEYDOWN(ptc, wParam, lParam, Tab_OnKeyDown);
        break;

    case WM_KILLFOCUS:

        if (ptc->iNewSel != -1) {
            int iOldSel = ptc->iNewSel;
            ptc->iNewSel = -1;
            Tab_InvalidateItem(ptc, iOldSel, FALSE);
            ptc->flags &= ~TCF_DRAWSUNKEN;
        }
        // fall through
    case WM_SETFOCUS:
        Tab_InvalidateItem(ptc, ptc->iSel, Tab_OwnerDraw(ptc));
        if ((uMsg == WM_SETFOCUS) && (ptc->iSel != -1))
            MyNotifyWinEvent(EVENT_OBJECT_FOCUS, hwnd, OBJID_CLIENT, ptc->iSel+1);
        break;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;

    HANDLE_MSG(ptc, WM_SETREDRAW, Tab_OnSetRedraw);
    HANDLE_MSG(ptc, WM_SETFONT, Tab_OnSetFont);

    case WM_GETFONT:
        return (LRESULT)ptc->hfontLabel;

    case WM_NOTIFYFORMAT:
        return CIHandleNotifyFormat(&ptc->ci, lParam);

    case WM_NOTIFY: {
        LPNMHDR lpNmhdr = (LPNMHDR)(lParam);

        //
        // We are just going to pass this on to the
        // real parent.  Note that -1 is used as
        // the hwndFrom.  This prevents SendNotifyEx
        // from updating the NMHDR structure.
        //

        SendNotifyEx(GetParent(ptc->ci.hwnd), (HWND) -1,
                     lpNmhdr->code, lpNmhdr, ptc->ci.bUnicode);
        }
        break;

#ifdef KEYBOARDCUES
    case WM_UPDATEUISTATE:
        if (CCOnUIState(&(ptc->ci), WM_UPDATEUISTATE, wParam, lParam))
        {
            if (UISF_HIDEFOCUS == HIWORD(wParam))
            {
                // We erase only if we are removing the focus rect or the accel
                Tab_InvalidateItem(ptc, ptc->iSel,
                    (UIS_CLEAR == LOWORD(wParam)) ? TRUE : FALSE);
            }
            else
            {
                if ((UISF_HIDEFOCUS | UISF_HIDEACCEL) & HIWORD(wParam))
                {
                    int i;

                    for (i = ptc->iFirstVisible; i <= ptc->iLastVisible; ++i)
                    {
                        Tab_InvalidateItem(ptc, i,
                            (UIS_CLEAR == LOWORD(wParam)) ? TRUE : FALSE);
                    }
                }
            }
        }

        goto DoDefault;
#endif
    case TCM_SETITEMEXTRA:
        return (LRESULT)Tab_OnSetItemExtra(ptc, (int)wParam);

    case TCM_GETITEMCOUNT:
        return (LRESULT)Tab_Count(ptc);

#ifdef UNICODE
    case TCM_SETITEMA:
        {
        LRESULT lResult;
        TC_ITEMW * pItemW;

        if (!lParam) {
            return FALSE;
        }

        pItemW = ThunkItemAtoW(ptc, (TC_ITEMA FAR*)lParam);

        if (!pItemW) {
            return FALSE;
        }

        lResult = (LRESULT)Tab_OnSetItem(ptc, (int)wParam, pItemW);

        FreeItemW(pItemW);

        return lResult;
        }
#endif

    case TCM_SETITEM:
        if (!lParam) {
            return FALSE;
        }

        return (LRESULT)Tab_OnSetItem(ptc, (int)wParam, (const TC_ITEM FAR*)lParam);

#ifdef UNICODE
    case TCM_GETITEMA:
        {
        LRESULT lResult;
        TC_ITEMW * pItemW;
        LPWSTR pszTextW = NULL;
        TC_ITEMA * pItemA = (TC_ITEMA FAR*)lParam;

        if (!ptc || !pItemA) {
            return FALSE;
        }

        pItemW = GlobalAlloc (GPTR, sizeof(TC_ITEMW) + ptc->cbExtra);

        if (!pItemW) {
            return FALSE;
        }

        if (pItemA->mask & TCIF_TEXT) {
            pszTextW = GlobalAlloc (GPTR, pItemA->cchTextMax * sizeof (TCHAR));

            if (!pszTextW) {
                GlobalFree (pItemW);
                return FALSE;
            }
            pItemW->pszText = pszTextW;
        }

        pItemW->mask       = pItemA->mask;
        pItemW->cchTextMax = pItemA->cchTextMax;
        pItemW->dwStateMask = pItemA->dwStateMask;

        lResult = (LRESULT)Tab_OnGetItem(ptc, (int)wParam, pItemW);

        if (!ThunkItemWtoA (ptc, pItemW, pItemA)) {
            lResult = (LRESULT)FALSE;
        }

        if (pszTextW) {
            GlobalFree (pszTextW);
        }
        GlobalFree (pItemW);

        return lResult;
        }
#endif

    case TCM_GETITEM:
        if (!ptc || !lParam) {
            return FALSE;
        }

        return (LRESULT)Tab_OnGetItem(ptc, (int)wParam, (TC_ITEM FAR*)lParam);

#ifdef UNICODE
    case TCM_INSERTITEMA:
        {
        LRESULT  lResult;
        TC_ITEMW * pItemW;

        if (!lParam) {
            return FALSE;
        }

        pItemW = ThunkItemAtoW(ptc, (TC_ITEMA FAR*)lParam);

        if (!pItemW) {
            return FALSE;
        }

        lResult =  (LRESULT)Tab_OnInsertItem(ptc, (int)wParam, pItemW);

        FreeItemW(pItemW);

        return lResult;
        }
#endif

    case TCM_INSERTITEM:
        if (!lParam) {
            return FALSE;
        }
        return (LRESULT)Tab_OnInsertItem(ptc, (int)wParam, (const TC_ITEM FAR*)lParam);

    case TCM_DELETEITEM:
        return (LRESULT)Tab_OnDeleteItem(ptc, (int)wParam);

    case TCM_DELETEALLITEMS:
        return (LRESULT)Tab_OnDeleteAllItems(ptc);

    case TCM_SETCURFOCUS:
        Tab_SetCurFocus(ptc, (int) wParam);
        break;

    case TCM_GETCURFOCUS:
        if (ptc->iNewSel != -1)
            return ptc->iNewSel;
        // else fall through

    case TCM_GETCURSEL:
        return ptc->iSel;

    case TCM_SETCURSEL:
        return (LRESULT)ChangeSel(ptc, (int)wParam, FALSE, FALSE);

    case TCM_GETTOOLTIPS:
        return (LRESULT)ptc->hwndToolTips;

    case TCM_SETTOOLTIPS:
        ptc->hwndToolTips = (HWND)wParam;
        break;

    case TCM_ADJUSTRECT:
        if (lParam) {
#define prc ((RECT FAR *)lParam)
            Tab_DVFlipRect(ptc, prc);
            Tab_OnAdjustRect(ptc, BOOLFROMPTR( wParam), (LPRECT)lParam);
            Tab_VDFlipRect(ptc, prc);
#undef prc
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
        return (LRESULT)himlOld;
    }

    case TCM_GETIMAGELIST:
        return (LRESULT)ptc->himl;

    case TCM_REMOVEIMAGE:
        Tab_OnRemoveImage(ptc, (int)wParam);
        break;

    case TCM_SETITEMSIZE: {
        int iOldWidth = ptc->iTabWidth;
        int iOldHeight = ptc->iTabHeight;
        int iNewWidth = LOWORD(lParam);
        int iNewHeight = HIWORD(lParam);

        if (ptc->himl) {
            int cx, cy;
            Tab_ImageList_GetIconSize(ptc, &cx, &cy);
            if (iNewWidth < (cx + (2*g_cxEdge)))
                iNewWidth = cx + (2*g_cxEdge);

        }
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
        ptc->cxPad = GET_X_LPARAM(lParam);
        ptc->cyPad = GET_Y_LPARAM(lParam);
        break;

    case TCM_GETROWCOUNT:
        CalcPaintMetrics(ptc, NULL);
        return (LRESULT)ptc->iLastRow + 1;
        
    case TCM_SETMINTABWIDTH:
    {
        int iOld = ptc->cxMinTab;
        if ((int)lParam >= 0) {
            ptc->cxMinTab = (int)lParam;
            ptc->fMinTabSet = TRUE;
        } else {
            ptc->fMinTabSet = FALSE;
        }
        ptc->cyTabs = RECOMPUTE;
        ptc->cxItem = RECOMPUTE;
        InvalidateRect(ptc->ci.hwnd, NULL, TRUE);
        return iOld;
    }
        
    case TCM_DESELECTALL:
        Tab_DeselectAll(ptc, BOOLFROMPTR( wParam));
        break;

    case TCM_SETEXTENDEDSTYLE:
        return Tab_ExtendedStyleChange(ptc, (DWORD) lParam, (DWORD) wParam);

    case TCM_GETEXTENDEDSTYLE:
        return ptc->dwStyleEx;

    case TCM_HITTEST: {
#define lphitinfo  ((LPTC_HITTESTINFO)lParam)
        return Tab_OnHitTest(ptc, lphitinfo->pt.x, lphitinfo->pt.y, &lphitinfo->flags);
    }

    case TCM_HIGHLIGHTITEM:
    {
        LPTABITEM pitem = Tab_GetItemPtr(ptc, (int)wParam);

        if (pitem)
        {
            BOOL fHighlight = LOWORD(lParam) != 0;

            // Don't do anything if state hasn't changed.
            if (fHighlight == ((pitem->dwState & TCIS_HIGHLIGHTED) != 0))
                break;

            if (fHighlight)
                pitem->dwState |= TCIS_HIGHLIGHTED;
            else
                pitem->dwState &= ~TCIS_HIGHLIGHTED;

            Tab_InvalidateItem(ptc, (int)wParam, TRUE);
            return TRUE;
        }
        break;
    }

    case WM_NCHITTEST:
    {
        POINT pt;
            
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ScreenToClient(ptc->ci.hwnd, &pt);
        if (Tab_OnHitTest(ptc, pt.x, pt.y, NULL) == -1)
            return(HTTRANSPARENT);
        else {
            goto DoDefault;
        }
    }

    case WM_GETOBJECT:
        if( lParam == OBJID_QUERYCLASSNAMEIDX )
            return MSAA_CLASSNAMEIDX_TAB;
        break;

    default:
    {
        LRESULT lres;
        if (CCWndProc(&ptc->ci, uMsg, wParam, lParam, &lres))
            return lres;
    }
    
DoDefault:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0L;
}

#ifdef UNICODE

//
// ANSI <=> UNICODE thunks
//

TC_ITEMW * ThunkItemAtoW (PTC ptc, TC_ITEMA * pItemA)
{
    TC_ITEMW *pItemW;
    UINT      cbTextW;
    INT       iResult;

    pItemW = (TC_ITEMW *) GlobalAlloc (GPTR, sizeof(TC_ITEMW) + ptc->cbExtra);

    if (!pItemW) {
        return NULL;
    }

    pItemW->mask        = pItemA->mask;
    pItemW->dwState = pItemA->dwState;
    pItemW->dwStateMask = pItemA->dwStateMask;

    if ((pItemA->mask & TCIF_TEXT) && pItemA->pszText) {
        cbTextW = lstrlenA(pItemA->pszText) + 1;

        pItemW->pszText = (LPWSTR)GlobalAlloc (GPTR, cbTextW * sizeof(TCHAR));

        if (!pItemW->pszText) {
            GlobalFree (pItemW);
            return NULL;
        }

        iResult = MultiByteToWideChar (CP_ACP, 0, pItemA->pszText, -1,
                                       pItemW->pszText, cbTextW);

        if (!iResult) {
            if (GetLastError()) {
                GlobalFree (pItemW->pszText);
                GlobalFree (pItemW);
                return NULL;
            }
        }
    }

    pItemW->cchTextMax = pItemA->cchTextMax;

    if (pItemA->mask & TCIF_IMAGE) {
        pItemW->iImage = pItemA->iImage;
    }

    if (pItemA->mask & TCIF_PARAM) {
        hmemcpy(&pItemW->lParam, &pItemA->lParam, ptc->cbExtra);
    }

    return (pItemW);
}

BOOL ThunkItemWtoA (PTC ptc, TC_ITEMW * pItemW, TC_ITEMA * pItemA)
{
    INT        iResult;


    if (!pItemA) {
        return FALSE;
    }

    pItemA->mask        = pItemW->mask;
    pItemA->dwState = pItemW->dwState;
    pItemA->dwStateMask = pItemW->dwStateMask;

    if ((pItemW->mask & TCIF_TEXT) && pItemW->pszText && pItemW->cchTextMax) {

        iResult = WideCharToMultiByte (CP_ACP, 0, pItemW->pszText, -1,
                                       pItemA->pszText, pItemW->cchTextMax, NULL, NULL);

        if (!iResult) {
            if (GetLastError()) {
                return FALSE;
            }
        }
    }

    pItemA->cchTextMax = pItemW->cchTextMax;

    if (pItemW->mask & TCIF_IMAGE) {
        pItemA->iImage = pItemW->iImage;
    }

    if (pItemW->mask & TCIF_PARAM) {
        hmemcpy(&pItemA->lParam, &pItemW->lParam, ptc->cbExtra);
    }

    return TRUE;
}

BOOL FreeItemW (TC_ITEMW *pItemW)
{

    if ((pItemW->mask & TCIF_TEXT) && pItemW->pszText) {
        GlobalFree (pItemW->pszText);
    }

    GlobalFree (pItemW);

    return TRUE;
}

BOOL FreeItemA (TC_ITEMA *pItemA)
{

    if ((pItemA->mask & TCIF_TEXT) && pItemA->pszText) {
        GlobalFree (pItemA->pszText);
    }

    GlobalFree (pItemA);

    return TRUE;
}


#endif
