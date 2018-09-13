/*
**    STATUS.C
**
**    Status bar code
**
*/

#include "ctlspriv.h"

#ifdef UNIX
#include <mainwin.h>
extern eLook MwLook;
extern const CLSID CLSID_CMultiLanguage;
#endif

#define MAX_TOOLTIP_STRING 80
#define SB_HITTEST_NOITEM  -2

typedef struct {
    ULONG_PTR dwString;
    UINT uType;
    int right;
    HICON hIcon;
    SIZE  sizeIcon;
    LPTSTR pszToolTip;
    BOOL fNeedToTip;
} STRINGINFO, NEAR *PSTRINGINFO;

typedef struct {
    CONTROLINFO ci;
    HWND hwndToolTips;
    HFONT hStatFont;
    BOOL bDefFont;

    int nFontHeight;
    int nMinHeight;
    int nBorderX, nBorderY, nBorderPart;
    int nLastX;                 // for invalidating unclipped right side
    int dxGripper;                // 0 if no gripper
#ifdef UNIX
    BOOL bHasGripper;           // TRUE if the status control was created with a gripper.
#endif
    UINT uiCodePage;            // code page

    STRINGINFO sSimple;

    int nParts;
    COLORREF _clrBk;    

    PSTRINGINFO sInfo;
} STATUSINFO, NEAR *PSTATUSINFO;


#define SBT_NORMAL        0xf000
#define SBT_NULL        0x0000    /* Some code depends on this being 0 */
#define SBT_ALLTYPES    0xf000    /* WINDOWS_ME: this does NOT include rtlred */
#define SBT_NOSIMPLE    0x00ff    /* Flags to indicate normal status bar */


#define MAXPARTS 256
// BUGBUG raymondc v6: This limit isn't big enough on large res screens
#define MAX_STATUS_TEXT_LEN 128

#ifdef UNICODE
#define CharNextEx(cp, sz, f) ((sz)+1)
#else
#define CharNextEx CharNextExA
#endif

BOOL NEAR PASCAL SBSetText(PSTATUSINFO pStatusInfo, WPARAM wParam, LPCTSTR lpsz);
void NEAR PASCAL SBSetBorders(PSTATUSINFO pStatusInfo, LPINT lpInt);
void NEAR PASCAL SBSetFont(PSTATUSINFO pStatusInfo, HFONT hFont, BOOL bInvalidate);
void WINAPI DrawStatusTextEx(PSTATUSINFO pStatusInfo, HDC hDC, LPRECT lprc, LPCTSTR pszText, STRINGINFO * psi,  UINT uFlags);
void RecalcTooltipRects(PSTATUSINFO pStatusinfo);
PSTRINGINFO GetStringInfo(PSTATUSINFO pStatusInfo, int nIndex);
int  IndexFromPt(PSTATUSINFO pStatusInfo, POINT pt);
void StatusUpdateToolTips(PSTATUSINFO psi);

void NEAR PASCAL GetNewMetrics(PSTATUSINFO pStatusInfo, HDC hDC, HFONT hNewFont)
{
    HFONT hOldFont;
    /* HACK! Pass in -1 to just delete the old font
     */
    if (hNewFont != (HFONT)-1)
    {
        TEXTMETRIC tm;

        hOldFont = 0;
        if (hNewFont)
        hOldFont = SelectObject(hDC, hNewFont);

        GetTextMetrics(hDC, &tm);

        if (hOldFont)
        SelectObject(hDC, hOldFont);

        pStatusInfo->nFontHeight = tm.tmHeight + tm.tmInternalLeading;

        // For far east font which has no internal leading
        if ( !tm.tmInternalLeading )
             pStatusInfo->nFontHeight += g_cyBorder * 2;

    }
}

void NEAR PASCAL NewFont(PSTATUSINFO pStatusInfo, HFONT hNewFont, BOOL fResize)
{
    HFONT hOldFont;
    BOOL bDelFont;
    HDC hDC;

    hOldFont = pStatusInfo->hStatFont;
    bDelFont = pStatusInfo->bDefFont;

    hDC = GetDC(pStatusInfo->ci.hwnd);

    if (hNewFont) {
        pStatusInfo->hStatFont = hNewFont;
        pStatusInfo->bDefFont = FALSE;
        pStatusInfo->uiCodePage = GetCodePageForFont(hNewFont);
    } else {
        if (bDelFont) {
            /* I will reuse the default font, so don't delete it later
             */
            hNewFont = pStatusInfo->hStatFont;
            bDelFont = FALSE;
        } else {
#ifndef DBCS_FONT
            hNewFont = CCCreateStatusFont();
            if (!hNewFont)
#endif // DBCS_FONT
                hNewFont = g_hfontSystem;


            pStatusInfo->hStatFont = hNewFont;
            pStatusInfo->bDefFont = BOOLFROMPTR(hNewFont);
        }
    }

#ifndef DBCS_FONT
    /* We delete the old font after creating the new one in case they are
     * the same; this should help GDI a little
     */
    if (bDelFont)
        DeleteObject(hOldFont);
#endif

    GetNewMetrics(pStatusInfo, hDC, hNewFont);

    ReleaseDC(pStatusInfo->ci.hwnd, hDC);

    // My font changed, so maybe I should resize to match
    if (fResize)
        SendMessage(pStatusInfo->ci.hwnd, WM_SIZE, 0, 0L);
}

/* We should send messages instead of calling things directly so we can
 * be subclassed more easily.
 */
LRESULT NEAR PASCAL InitStatusWnd(HWND hWnd, LPCREATESTRUCT lpCreate)
{
    int nBorders[3];
    PSTATUSINFO pStatusInfo = (PSTATUSINFO)LocalAlloc(LPTR, sizeof(STATUSINFO));
    if (!pStatusInfo)
        return -1;        // fail the window create

    // Start out with one part
    pStatusInfo->sInfo = (PSTRINGINFO)LocalAlloc(LPTR, sizeof(STRINGINFO));
    if (!pStatusInfo->sInfo)
    {
        LocalFree(pStatusInfo);
        return -1;        // fail the window create
    }

    SetWindowPtr(hWnd, 0, pStatusInfo);
    CIInitialize(&pStatusInfo->ci, hWnd, lpCreate);
    

    pStatusInfo->sSimple.uType = SBT_NOSIMPLE | SBT_NULL;
    pStatusInfo->sSimple.right = -1;
    pStatusInfo->uiCodePage = CP_ACP;

    pStatusInfo->nParts = 1;
    pStatusInfo->sInfo[0].uType = SBT_NULL;
    pStatusInfo->sInfo[0].right = -1;

    pStatusInfo->_clrBk = CLR_DEFAULT;

    // Save the window text in our struct, and let USER store the NULL string
#ifdef UNICODE_WIN9x
    {
        // We force CP_ACP because that's what we used to thunk it up in the first place
        LPWSTR pwsz = ProduceWFromA(CP_ACP, (LPCSTR)lpCreate->lpszName);
        SBSetText(pStatusInfo, 0, pwsz);
        FreeProducedString(pwsz);
    }
#else
    SBSetText(pStatusInfo, 0, lpCreate->lpszName);
#endif
    lpCreate->lpszName = c_szNULL;

    // Don't resize because MFC doesn't like getting funky
    // messages before the window is fully created.  USER will send
    // us a WM_SIZE message after the WM_CREATE returns, so we'll
    // get it sooner or later.
    NewFont(pStatusInfo, 0, FALSE);

    nBorders[0] = -1;     // use default border widths
    nBorders[1] = -1;
    nBorders[2] = -1;

    SBSetBorders(pStatusInfo, nBorders);

#define GRIPSIZE (g_cxVScroll + g_cxBorder)     // make the default look good

    if ((lpCreate->style & SBARS_SIZEGRIP) ||
        ((GetWindowStyle(lpCreate->hwndParent) & WS_THICKFRAME) &&
         !(lpCreate->style & (CCS_NOPARENTALIGN | CCS_TOP | CCS_NOMOVEY))))
#ifdef UNIX
         pStatusInfo->bHasGripper = TRUE;
    else
         pStatusInfo->bHasGripper = FALSE;
    if (!pStatusInfo->bHasGripper || (MwLook == LOOK_MOTIF))
         pStatusInfo->dxGripper = 0;
    else
         pStatusInfo->dxGripper = GRIPSIZE;
#else
        pStatusInfo->dxGripper = GRIPSIZE;
#endif

    return 0;     // success
}

// lprc is left unchanged, but used as scratch
void WINAPI DrawStatusText(HDC hDC, LPRECT lprc, LPCTSTR pszText, UINT uFlags)
{
    DrawStatusTextEx(NULL, hDC, lprc, pszText, NULL, uFlags);
}    

#ifdef UNICODE
void WINAPI DrawStatusTextA(HDC hDC, LPRECT lprc, LPCSTR pszText, UINT uFlags)
{
     INT     cch;
     LPWSTR     lpw;

     cch = lstrlenA(pszText);
     lpw = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, ((cch + 1) * sizeof(TCHAR)));

     if (!lpw) {
#ifdef DEBUG
        OutputDebugString(TEXT("Alloc failed: DrawStatusTextA\r\n"));
#endif
        return;
     }

     MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszText, cch, lpw, cch);
     DrawStatusTextW(hDC, lprc, lpw, uFlags);

     LocalFree((LPVOID)lpw);
}

#else

//
//    Stub Unicode function for DrawStatusTextW when this
//    code is built ANSI.
//

void WINAPI DrawStatusTextW(HDC hDC, LPRECT lprc, LPCWSTR pszText, UINT uFlags)
{
    SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
    return;
}


#endif

BOOL NEAR PASCAL Status_GetRect(PSTATUSINFO pStatusInfo, int nthPart, LPRECT lprc)
{
    PSTRINGINFO pStringInfo = pStatusInfo->sInfo;

    if (pStatusInfo->sSimple.uType & SBT_NOSIMPLE)
    {
        RECT rc;
        int nRightMargin, i;

        /* Get the client rect and inset the top and bottom.    Then set
         * up the right side for entry into the loop
         */
        GetClientRect(pStatusInfo->ci.hwnd, &rc);

        if (pStatusInfo->dxGripper && !IsZoomed(pStatusInfo->ci.hwndParent))
        {
            rc.right = rc.right - pStatusInfo->dxGripper + pStatusInfo->nBorderX;
        }

        rc.top += pStatusInfo->nBorderY;

        nRightMargin = rc.right - pStatusInfo->nBorderX;
        rc.right = pStatusInfo->nBorderX - pStatusInfo->nBorderPart;

        for (i = 0; i < pStatusInfo->nParts; ++i, ++pStringInfo)
        {
            // WARNING!  This pixel computation is also in PaintStatusWnd,
            // so make sure the two algorithms are in sync.

            if (pStringInfo->right == 0)
                continue;

            rc.left = rc.right + pStatusInfo->nBorderPart;

            rc.right = pStringInfo->right;

            // size the right-most one to the end with room for border
            if (rc.right < 0 || rc.right > nRightMargin)
                rc.right = nRightMargin;

            // if the part is real small, don't show it
            // Bug keep the rc.left valid in case this item happens to
            // be the nthPart.
            if ((rc.right - rc.left) < pStatusInfo->nBorderPart)
                rc.left = rc.right;

            if (i == nthPart)
            {
                *lprc = rc;
                return TRUE;
            }
        }
    }

    return FALSE;

}

void NEAR PASCAL PaintStatusWnd(PSTATUSINFO pStatusInfo, HDC hdcIn, PSTRINGINFO pStringInfo, int nParts, int nBorderX)
{
    PAINTSTRUCT ps;
    RECT rc, rcGripper;
    int nRightMargin, i;
    HFONT hOldFont = NULL;
    UINT uType;
    BOOL bDrawGrip;

    // paint the whole client area
    GetClientRect(pStatusInfo->ci.hwnd, &rc);

    if (hdcIn)
    {
        ps.rcPaint = rc;
        ps.hdc = hdcIn;
    }
    else
        BeginPaint(pStatusInfo->ci.hwnd, &ps);


    rc.top += pStatusInfo->nBorderY;

    bDrawGrip = pStatusInfo->dxGripper && !IsZoomed(pStatusInfo->ci.hwndParent);

    if (bDrawGrip)
        rcGripper = rc;

    nRightMargin = rc.right - nBorderX;
    rc.right = nBorderX - pStatusInfo->nBorderPart;

    if (pStatusInfo->hStatFont)
        hOldFont = SelectObject(ps.hdc, pStatusInfo->hStatFont);

    for (i=0; i<nParts; ++i, ++pStringInfo)
    {
        // WARNING!  This pixel computation is also in Status_GetRect,
        // so make sure the two algorithms are in sync.
        if (pStringInfo->right == 0)
            continue;

        rc.left = rc.right + pStatusInfo->nBorderPart;
        rc.right = pStringInfo->right;

        // size the right-most one to the end with room for border
        if (rc.right < 0 || rc.right > nRightMargin)
            rc.right = nRightMargin;

#ifdef WINDOWS_ME
        if(g_fMEEnabled && (rc.right > (nRightMargin-pStatusInfo->dxGripper)))
            //
            // for MidEast we DONT overpaint the rhs with the grip, this will
            // lose the begining of the text.
            //
            rc.right = nRightMargin-pStatusInfo->dxGripper;
#endif
        
        DebugMsg(TF_STATUS, TEXT("SBPaint: part=%d, x/y=%d/%d"), i, rc.left, rc.right);

        // if the part is real small, don't show it
        if (((rc.right - rc.left) < pStatusInfo->nBorderPart) || !RectVisible(ps.hdc, &rc))
            continue;

        uType = pStringInfo->uType;

        if ((uType&SBT_ALLTYPES) == SBT_NORMAL)
        {
            DrawStatusTextEx(pStatusInfo, ps.hdc, &rc, (LPTSTR)OFFSETOF(pStringInfo->dwString), pStringInfo, uType);
        }
        else
        {
            DrawStatusTextEx(pStatusInfo, ps.hdc, &rc, c_szNULL, pStringInfo, uType);

            if (uType & SBT_OWNERDRAW)
            {
                DRAWITEMSTRUCT di;

                di.CtlID = GetWindowID(pStatusInfo->ci.hwnd);
                di.itemID = i;
                di.hwndItem = pStatusInfo->ci.hwnd;
                di.hDC = ps.hdc;
                di.rcItem = rc;
                InflateRect(&di.rcItem, -g_cxBorder, -g_cyBorder);
                di.itemData = pStringInfo->dwString;

                SaveDC(ps.hdc);
                IntersectClipRect(ps.hdc, di.rcItem.left, di.rcItem.top,
                                    di.rcItem.right, di.rcItem.bottom);
                SendMessage(pStatusInfo->ci.hwndParent, WM_DRAWITEM, di.CtlID,
                            (LPARAM)(LPTSTR)&di);
                RestoreDC(ps.hdc, -1);
            }
        }
    }

    if (bDrawGrip)
    {
        RECT rcTemp;
        COLORREF crBkColorOld;
        COLORREF crBkColor;
        
        pStatusInfo->dxGripper = min(pStatusInfo->dxGripper, pStatusInfo->nFontHeight);

        // draw the grip
        rcGripper.right -= g_cxBorder;                    // inside the borders
        rcGripper.bottom -= g_cyBorder;

        rcGripper.left = rcGripper.right - pStatusInfo->dxGripper;        // make it square
        rcGripper.top += g_cyBorder;
        // rcGripper.top    = rcGripper.bottom - pStatusInfo->dxGripper;

        crBkColor = g_clrBtnFace;
        if ((pStatusInfo->_clrBk != CLR_DEFAULT))
            crBkColor = pStatusInfo->_clrBk;
        
        crBkColorOld = SetBkColor(ps.hdc, crBkColor);
        // BUGBUG:we need to call our own drawframecontrol here
        // because user doesn't obey the bkcolro set
        DrawFrameControl(ps.hdc, &rcGripper, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);

        // clear out the border edges to make this appear on the same level

        // NOTE: these values line up right only for the default scroll bar
        // width. for others this is close enough...

        // right border
        rcTemp.top = rcGripper.bottom - pStatusInfo->dxGripper + g_cyBorder + g_cyEdge;
        rcTemp.left = rcGripper.right;
        rcTemp.bottom = rcGripper.bottom;
        rcTemp.right = rcGripper.right + g_cxBorder;
        FillRectClr(ps.hdc, &rcTemp, crBkColor);
        
        // bottom border
        rcTemp.top = rcGripper.bottom;
        rcTemp.left = rcGripper.left + g_cyBorder + g_cxEdge;
        rcTemp.bottom = rcGripper.bottom +    g_cyBorder;
        rcTemp.right = rcGripper.right + g_cxBorder;
        FillRectClr(ps.hdc, &rcTemp, crBkColor);
        
        SetBkColor(ps.hdc, crBkColorOld);
    }
    
    if (hOldFont)
        SelectObject(ps.hdc, hOldFont);

    if (hdcIn == NULL)
        EndPaint(pStatusInfo->ci.hwnd, &ps);
}


BOOL NEAR PASCAL SetStatusText(PSTATUSINFO pStatusInfo, PSTRINGINFO pStringInfo, UINT uPart, LPCTSTR lpStr)
{
    PTSTR pString;
    UINT wLen;
    int nPart;
    RECT rc;

    nPart = LOBYTE(uPart);

    /* Note it is up to the app the dispose of the previous itemData for
     * SBT_OWNERDRAW
     */
    if ((pStringInfo->uType&SBT_ALLTYPES) == SBT_NORMAL)
        LocalFree((HLOCAL)OFFSETOF(pStringInfo->dwString));

    /* Set to the NULL string in case anything goes wrong
     *
     * But be careful to preserve simple-ness if this is the simple
     * pane being updated.
     */
    if (nPart == 0xFF)
    {
        pStringInfo->uType = (uPart & 0xff00) | (pStringInfo->uType & 0x00ff);
        nPart = 0;          // There is only one simple part, so we are part 0
    }
    else
    {
        pStringInfo->uType = uPart & 0xff00;
    }
    pStringInfo->uType &= ~SBT_ALLTYPES;
    pStringInfo->uType |= SBT_NULL;

    /* Invalidate the rect of this pane.
     *
     * Note that we don't check whether the pane is actually visible
     * in the current status bar mode.  The result is some gratuitous
     * invalidates and updates.  Oh well.
     */
    GetClientRect(pStatusInfo->ci.hwnd, &rc);
    if (nPart)
        rc.left = pStringInfo[-1].right;
    if (pStringInfo->right > 0)
        rc.right = pStringInfo->right;
    InvalidateRect(pStatusInfo->ci.hwnd, &rc, FALSE);

    switch (uPart&SBT_ALLTYPES)
    {
        case 0:
            /* If lpStr==NULL, we have the NULL string
             */
            if (HIWORD64(lpStr))
            {
                wLen = lstrlen(lpStr);
                if (wLen)
                {
                    pString = (PTSTR)LocalAlloc(LPTR, (wLen+1)*sizeof(TCHAR));
                    pStringInfo->dwString = (ULONG_PTR)(LPTSTR)pString;
                    if (pString)
                    {
                        pStringInfo->uType |= SBT_NORMAL;

                        /* Copy the string
                         */
                        lstrcpy(pString, lpStr);

                        /* Replace unprintable characters (like CR/LF) with spaces
                         */
                        for ( ; *pString;
                              pString=(PTSTR)OFFSETOF(CharNextEx((WORD)pStatusInfo->uiCodePage, pString, 0)))
                            if ((unsigned)(*pString)<(unsigned)TEXT(' ') && *pString!= TEXT('\t'))
                                *pString = TEXT(' ');
                    }
                    else
                    {
                        /* We return FALSE to indicate there was an error setting
                         * the string
                         */
                        return(FALSE);
                    }
                }
            }
            else if (LOWORD(lpStr))
            {
                /* We don't allow this anymore; the app needs to set the ownerdraw
                 * bit for ownerdraw.
                 */
                return(FALSE);
            }
            break;

        case SBT_OWNERDRAW:
            pStringInfo->uType |= SBT_OWNERDRAW;
            pStringInfo->dwString = (ULONG_PTR)lpStr;
            break;

        default:
            return(FALSE);
    }

    UpdateWindow(pStatusInfo->ci.hwnd);
    return(TRUE);
}

BOOL NEAR PASCAL SetStatusParts(PSTATUSINFO pStatusInfo, int nParts, LPINT lpInt)
{
    int i;
    int prev;
    PSTRINGINFO pStringInfo, pStringInfoTemp;
    BOOL bRedraw = FALSE;

    if (nParts != pStatusInfo->nParts)
    {
        TOOLINFO ti = {0};
        int n;

        if (pStatusInfo->hwndToolTips)
        {
            ti.cbSize = sizeof(ti);
            ti.hwnd = pStatusInfo->ci.hwnd;
            ti.lpszText = LPSTR_TEXTCALLBACK;
    
            for(n = 0; n < pStatusInfo->nParts; n++)
            {
                ti.uId = n;
                SendMessage(pStatusInfo->hwndToolTips, TTM_DELTOOL, 0, (LPARAM)&ti);
            }
        }

        bRedraw = TRUE;

        /* Note that if nParts > pStatusInfo->nParts, this loop
         * does nothing
         */
        for (i=pStatusInfo->nParts-nParts,
            pStringInfo=&pStatusInfo->sInfo[nParts]; i>0;
            --i, ++pStringInfo)
        {
            if ((pStringInfo->uType&SBT_ALLTYPES) == SBT_NORMAL)
                LocalFree((HLOCAL)OFFSETOF(pStringInfo->dwString));
            pStringInfo->uType = SBT_NULL;
        }

        /* Realloc to the new size and store the new pointer
         */
        pStringInfoTemp = (PSTRINGINFO)CCLocalReAlloc(pStatusInfo->sInfo,
                                             nParts * sizeof(STRINGINFO));
        if (!pStringInfoTemp)
            return(FALSE);
        pStatusInfo->sInfo = pStringInfoTemp;

        /* Note that if nParts < pStatusInfo->nParts, this loop
         * does nothing
         */
        for (i=nParts-pStatusInfo->nParts,
             pStringInfo=&pStatusInfo->sInfo[pStatusInfo->nParts]; i>0;
             --i, ++pStringInfo)
        {
            pStringInfo->uType = SBT_NULL;
            pStringInfo->right = 0;
        }
        pStatusInfo->nParts = nParts;

        StatusUpdateToolTips(pStatusInfo);
    }

    //
    //  Under stress, apps such as Explorer might pass coordinates that
    //  result in status bar panes with negative width, so make sure
    //  each edge is at least as far to the right as the previous.
    //
    prev = 0;
    for (i=0, pStringInfo=pStatusInfo->sInfo; i<nParts;
         ++i, ++pStringInfo, ++lpInt)
    {
        int right = *lpInt;
        // The last component is allowed to have *lpInt = -1.
        // Otherwise, make sure the widths are nondecreasing.
        if (!(right == -1 && i == nParts - 1) && right < prev)
            right = prev;
        DebugMsg(TF_STATUS, TEXT("SBSetParts: part=%d, rlimit=%d (%d)"), i, right, *lpInt);
        if (pStringInfo->right != right)
        {
            bRedraw = TRUE;
            pStringInfo->right = right;
        }
        prev = right;
    }

    /* Only redraw if necesary (if the number of parts has changed or
     * a border has changed)
     */
    if (bRedraw)
        InvalidateRect(pStatusInfo->ci.hwnd, NULL, TRUE);

    RecalcTooltipRects(pStatusInfo);

    return TRUE;
}

void NEAR PASCAL SBSetFont(PSTATUSINFO pStatusInfo, HFONT hFont, BOOL bInvalidate)
{
    NewFont(pStatusInfo, hFont, TRUE);
    if (bInvalidate)
    {
        // BUGBUG do we need the updatenow flag?
        RedrawWindow(pStatusInfo->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

BOOL NEAR PASCAL SBSetText(PSTATUSINFO pStatusInfo, WPARAM wParam, LPCTSTR lpsz)
{
    BOOL bRet;
    UINT idChild;

    DebugMsg(TF_STATUS, TEXT("SBSetText(%04x, [%s])"), wParam, lpsz);

    /* This is the "simple" status bar pane
     */
    if (LOBYTE(wParam) == 0xff)
    {
        UINT uSimple;

        // Note that we do not allow OWNERDRAW for a "simple" status bar
        if (wParam & SBT_OWNERDRAW)
            return FALSE;

        //
        //  IE4 BUG-FOR-BUG COMPATIBILITY:  In IE4, changing the SIMPLE
        //  status bar text while you were in complex mode caused the simple
        //  text to be painted briefly.  It would get cleaned up the next time
        //  the window got invalidated.
        //
        //  Corel Gallery actually RELIES ON THIS BUG!
        //
        //  Since the bad text got cleaned up on every invalidate, they
        //  "worked around" their bug by doing SB_SETTEXT in their idle loop,
        //  so the "correct" text gets repainted no matter what.
        //
        //  So if we have an old status bar, emulate the bug by temporarily
        //  setting the status bar into SIMPLE mode for the duration of the
        //  SetStatusText call.

        uSimple = pStatusInfo->sSimple.uType;
        if (pStatusInfo->ci.iVersion < 5)
            pStatusInfo->sSimple.uType = (uSimple & 0xFF00);

        bRet = SetStatusText(pStatusInfo, &pStatusInfo->sSimple,
                             (UINT) wParam, lpsz);

        if (pStatusInfo->ci.iVersion < 5)
            pStatusInfo->sSimple.uType |= LOBYTE(uSimple);

        idChild = 0;
    }
    else
    {

        if ((UINT)pStatusInfo->nParts <= (UINT)LOBYTE(wParam))
            bRet = FALSE;
        else
            bRet = SetStatusText(pStatusInfo, &pStatusInfo->sInfo[LOBYTE(wParam)],
                                 (UINT) wParam, lpsz);

        idChild = LOBYTE(wParam);
    }

    if (bRet)
        MyNotifyWinEvent(EVENT_OBJECT_NAMECHANGE, pStatusInfo->ci.hwnd,
            OBJID_CLIENT, idChild+1);

    return bRet;
}

//
//  iPart - which part we are querying
//  lpOutBuf - output buffer, NULL if no output desired
//  cchOutBuf - size of output buffer in characters
//  flags - zero or more of the following flags
//
//      SBGT_ANSI       - Output buffer is ANSI
//      SBGT_UNICODE    - Output buffer is unicode
//      SBGT_TCHAR      - Output buffer is TCHAR
//      SBGT_OWNERDRAWOK- Return refdata for owner-draw
//
//  If item is a string, and output buffer is provided, then returns
//  output string length (not including null) in low word, flags in
//  high word.
//
//  If item is a string, and no output buffer is provided, then returns
//  source string length (not including null) in low word, flags in
//  high word.
//
//  If item is owner-draw and SBGT_OWNERDRAWOK is set, then return the
//  refdata for the owner-draw item.
//
//  If item is owner-draw and SBGT_OWNERDRAWOK is clear, then treats
//  string as if it were empty.
//

#define     SBGT_ANSI           0
#define     SBGT_UNICODE        1
#define     SBGT_OWNERDRAWOK    2

#ifdef UNICODE
#define     SBGT_TCHAR          SBGT_UNICODE
#else
#define     SBGT_TCHAR          SBGT_ANSI
#endif

// Value for cchOutBuf to indicate largest possible output buffer size
// We cannot use -1 because StrCpyNW thinks -1 means a negative-size buffer.
// Since the maximum value we return is 0xFFFF (LOWORD), and the return value
// doesn't include the trailing null, the largest incoming buffer is one
// greater.
#define     SBGT_INFINITE       0x00010000

LRESULT SBGetText(PSTATUSINFO pStatusInfo, WPARAM iPart, LPVOID lpOutBuf, int cchOutBuf, UINT flags)
{
    UINT uType;
    PTSTR pString;
    ULONG_PTR dwString;
    UINT wLen;

    if (!pStatusInfo || (UINT)pStatusInfo->nParts<=iPart)
        return(0);

    if (pStatusInfo->sSimple.uType & SBT_NOSIMPLE) {
        uType = pStatusInfo->sInfo[iPart].uType;
        dwString = pStatusInfo->sInfo[iPart].dwString;
    } else {
        uType = pStatusInfo->sSimple.uType;
        dwString = pStatusInfo->sSimple.dwString;
    }

    // Catch the boundary condition early so we only have to check lpOutBuf
    if (cchOutBuf == 0)
        lpOutBuf = NULL;

    if ((uType&SBT_ALLTYPES) == SBT_NORMAL)
    {
        pString = (PTSTR)dwString;
#ifdef UNICODE
        if (flags & SBGT_UNICODE)
        {
            if (lpOutBuf)
            {
                StrCpyNW(lpOutBuf, pString, cchOutBuf);
                wLen = lstrlenW(lpOutBuf);
            }
            else
                wLen = lstrlen(pString);
        }
        else
        {
            // We have to use ProduceAFromW because WideCharToMultiByte
            // will simply fail if the output buffer isn't big enough,
            // but we want to copy as many as will fit.
            LPSTR pStringA = ProduceAFromW(pStatusInfo->ci.uiCodePage, pString);
            if (pStringA)
            {
                if (lpOutBuf)
                {
                    lstrcpynA(lpOutBuf, pStringA, cchOutBuf);
                    wLen = lstrlenA(lpOutBuf);
                }
                else
                    wLen = lstrlenA(pStringA);  // Return required ANSI buf size
                FreeProducedString(pStringA);
            }
            else
            {
                if (lpOutBuf)
                    *(LPSTR)lpOutBuf = '\0';
                wLen = 0;               // Eek, horrible memory problem
            }
        }
#else
        if (lpOutBuf)
        {
            lstrcpyn(lpOutBuf, pString, cchOutBuf);
            wLen = lstrlen(lpOutBuf);
        }
        else
            wLen = lstrlen(pString);
#endif

        /* Set this back to 0 to return to the app
         */
        uType &= ~SBT_ALLTYPES;
    }
    else
    {
        if (lpOutBuf)
        {
#ifdef UNICODE
            if (flags & SBGT_UNICODE)
                *(LPWSTR)lpOutBuf = L'\0';
            else
                *(LPSTR)lpOutBuf = '\0';
#else
            *(LPTSTR)lParam = TEXT('\0');
#endif
        }
        wLen = 0;

        // Only SB_GETTEXT[AW] returns the raw owner-draw refdata
        if ((uType&SBT_ALLTYPES)==SBT_OWNERDRAW && (flags & SBGT_OWNERDRAWOK))
            return(dwString);
    }

    return(MAKELONG(wLen, uType));
}

void NEAR PASCAL SBSetBorders(PSTATUSINFO pStatusInfo, LPINT lpInt)
{
    // pStatusInfo->nBorderX = lpInt[0] < 0 ? 0 : lpInt[0];
    pStatusInfo->nBorderX = 0;

    // pStatusInfo->nBorderY = lpInt[1] < 0 ? 2 * g_cyBorder : lpInt[1];
    pStatusInfo->nBorderY = g_cyEdge;

    // pStatusInfo->nBorderPart = lpInt[2] < 0 ? 2 * g_cxBorder : lpInt[2];
    pStatusInfo->nBorderPart = g_cxEdge;
}

void StatusUpdateToolTips(PSTATUSINFO psi)
{
    if (psi->hwndToolTips)
    {
        TOOLINFO ti = {0};
        int n;

        ti.cbSize = sizeof(ti);
        ti.hwnd = psi->ci.hwnd;
        ti.lpszText = LPSTR_TEXTCALLBACK;
        for(n = 0; n < psi->nParts; n++)
        {
            ti.uId = n;
            SendMessage(psi->hwndToolTips, TTM_ADDTOOL, 0, (LPARAM)&ti);
        }

    }
}
void StatusForceCreateTooltips(PSTATUSINFO psi)
{
    if (psi->ci.style & SBT_TOOLTIPS && !psi->hwndToolTips) 
    {
        TOOLINFO ti = {0};
#ifdef UNIX
        // IEUNIX : CreateWindowEx is used with special extended flag to get
        // rid of unwanted border around the tooltip.
        DWORD dwExStyle = 0;
        dwExStyle |= WS_EX_MW_UNMANAGED_WINDOW;
        psi->hwndToolTips = CreateWindowEx(dwExStyle, c_szSToolTipsClass, NULL, WS_POPUP | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, psi->ci.hwnd, NULL, HINST_THISDLL, NULL);
#else
        psi->hwndToolTips = CreateWindow(c_szSToolTipsClass, NULL, WS_POPUP | TTS_ALWAYSTIP, 
                                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                    psi->ci.hwnd, NULL, HINST_THISDLL, NULL);

#endif

        ti.cbSize = sizeof(ti);
        ti.hwnd = psi->ci.hwnd;
        ti.lpszText = LPSTR_TEXTCALLBACK;
        ti.uId = SB_SIMPLEID;
        SendMessage(psi->hwndToolTips, TTM_ADDTOOL, 0, (LPARAM)&ti);
        StatusUpdateToolTips(psi);
        RecalcTooltipRects(psi);
    }
}

LRESULT CALLBACK StatusWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PSTATUSINFO pStatusInfo = GetWindowPtr(hWnd, 0);
    NMCLICK nm;
    int nNotification;

    if (!pStatusInfo && uMsg != WM_CREATE) 
        goto DoDefault;
    
    switch (uMsg)
    {
        case WM_CREATE:
                return InitStatusWnd(hWnd, (LPCREATESTRUCT)lParam);
                
        case WM_SYSCOLORCHANGE:
#ifdef UNIX
            InitGlobalColors();
            InitGlobalMetrics(0);
#endif
            if (pStatusInfo->hwndToolTips)
                SendMessage(pStatusInfo->hwndToolTips, uMsg, wParam, lParam);
            break;

        case WM_MOUSEMOVE:  
        case WM_LBUTTONDOWN:
            StatusForceCreateTooltips(pStatusInfo);
            RelayToToolTips(pStatusInfo->hwndToolTips, hWnd, uMsg, wParam, lParam);
            break;
#ifdef UNIX
        case WM_LOOKSWITCH:
            InitGlobalMetrics(0);
            if (pStatusInfo->bHasGripper)
                pStatusInfo->dxGripper = (MwLook == LOOK_MOTIF) ? 0 : GRIPSIZE;
            break;
#endif
        case WM_STYLECHANGED:
        {
            if (wParam == GWL_EXSTYLE)
            {
                //
                // If the RTL_MIRROR extended style bit had changed, let's
                // repaint the control window
                //
                if ((pStatusInfo->ci.dwExStyle&RTL_MIRRORED_WINDOW) !=  
                    (((LPSTYLESTRUCT)lParam)->styleNew&RTL_MIRRORED_WINDOW))
                    InvalidateRect(pStatusInfo->ci.hwnd, NULL, TRUE);

                //
                // Save the new ex-style bits
                //
                pStatusInfo->ci.dwExStyle = ((LPSTYLESTRUCT)lParam)->styleNew;
            }
        }
        return 0;

#ifdef WINNT
#     if WM_SETTINGCHANGE != WM_WININICHANGE
#         pragma error( WM_SETTINGCHANGE value is not WM_WININICHANGE)
#     endif
#endif
        case WM_SETTINGCHANGE:
            InitGlobalColors();
            InitGlobalMetrics(wParam);

            if (pStatusInfo->hwndToolTips)
                SendMessage(pStatusInfo->hwndToolTips, uMsg, wParam, lParam);
                
            if (pStatusInfo->dxGripper)
                pStatusInfo->dxGripper = GRIPSIZE;

            if (wParam == SPI_SETNONCLIENTMETRICS)
            {
                if (pStatusInfo->bDefFont)
                {
                    if (pStatusInfo->hStatFont)
                    {
                        DeleteObject(pStatusInfo->hStatFont);
                        pStatusInfo->hStatFont = NULL;
                        pStatusInfo->bDefFont = FALSE;
                        SBSetFont(pStatusInfo, 0, TRUE);
                    }
                }
            }
            break;

        case WM_DESTROY:
            if (pStatusInfo)
            {
                int i;
                PSTRINGINFO pStringInfo;

                // FALSE = Don't resize while being destroyed...
                NewFont(pStatusInfo, (HFONT)-1, FALSE);
                for (i=pStatusInfo->nParts-1, pStringInfo=pStatusInfo->sInfo;
                     i>=0; --i, ++pStringInfo)
                {
                    if ((pStringInfo->uType&SBT_ALLTYPES) == SBT_NORMAL)
                        LocalFree((HLOCAL)OFFSETOF(pStringInfo->dwString));
                    Str_Set(&pStringInfo->pszToolTip, NULL);
                }

                if ((pStatusInfo->sSimple.uType&SBT_ALLTYPES) == SBT_NORMAL)
                    LocalFree((HLOCAL)OFFSETOF(pStatusInfo->sSimple.dwString));

                if (IsWindow(pStatusInfo->hwndToolTips))
                    DestroyWindow(pStatusInfo->hwndToolTips);

                Str_Set(&pStatusInfo->sSimple.pszToolTip, NULL);

                if (pStatusInfo->sInfo)
                    LocalFree(pStatusInfo->sInfo);
                LocalFree((HLOCAL)pStatusInfo);
                SetWindowInt(hWnd, 0, 0);

            }
            break;

        case WM_NCHITTEST:
            if (pStatusInfo->dxGripper && !IsZoomed(pStatusInfo->ci.hwndParent))
            {
                RECT rc;

                // already know height is valid.    if the width is in the grip,
                // show the sizing cursor
                GetWindowRect(pStatusInfo->ci.hwnd, &rc);
                
                //
                // If this is a RTL mirrored status window, then measure
                // from the near edge (screen coordinates) since Screen
                // Coordinates are not RTL mirrored.
                // [samera]
                //
                if (pStatusInfo->ci.dwExStyle&RTL_MIRRORED_WINDOW) {
                    if (GET_X_LPARAM(lParam) < (rc.left + pStatusInfo->dxGripper))
                        return HTBOTTOMLEFT;
                } else if (GET_X_LPARAM(lParam) > (rc.right - pStatusInfo->dxGripper)) {
                    return HTBOTTOMRIGHT;
                }
            }
            goto DoDefault;

        case WM_SETTEXT:
        {
#ifndef WINNT
#ifdef UNICODE
            LPTSTR lpsz;
#endif
#endif
            wParam = 0;
            uMsg = SB_SETTEXT;
#ifndef WINNT
#ifdef UNICODE
            lpsz = ProduceWFromA(pStatusInfo->uiCodePage, (LPSTR)lParam);
            if (lpsz)
            {
                BOOL bRet;

                bRet = SBSetText(pStatusInfo, wParam, (LPCTSTR)lpsz);
                FreeProducedString(lpsz);
                return bRet;
            }
#endif
#endif
        }
            /* Fall through */
        case SB_SETTEXT:
            return SBSetText(pStatusInfo, wParam, (LPCTSTR)lParam);

#ifdef UNICODE
        case SB_SETTEXTA:
        {
            BOOL bRet, bAlloced = FALSE;
            LPTSTR lpsz;

            if (!(wParam & SBT_OWNERDRAW)) {
                lpsz = ProduceWFromA(pStatusInfo->uiCodePage, (LPSTR)lParam);
                bAlloced = TRUE;
            } else {
                lpsz = (LPTSTR)lParam;
            }

            if (!pStatusInfo)
                bRet = FALSE;
            else
            {
                bRet = SBSetText(pStatusInfo, wParam, (LPCTSTR)lpsz);
            }

            if (bAlloced) {
                FreeProducedString(lpsz);
            }
            return bRet;
        }
#elif WIN32
        case SB_SETTEXTW:
        {
            BOOL bRet = FALSE;
            LPSTR lpsz = NULL;
            
            lpsz = ProduceAFromW(CP_ACP, (LPCWSTR)lParam);
            
            if (lpsz)
            {
                bRet = SBSetText(pStatusInfo, wParam, lpsz);
                FreeProducedString(lpsz);
            }
            return bRet;
        }
#endif

        // The WM_GETTEXT and WM_GETTEXTLENGTH messages must return a
        // character count, no flags.  (Otherwise USER gets mad at us.)
        // So we throw away the flags by returning only the LOWORD().
#ifdef UNICODE_WIN9x
        case WM_GETTEXT:
            return LOWORD(SBGetText(pStatusInfo, 0, (LPVOID)lParam, (int)wParam, SBGT_ANSI));
        case WM_GETTEXTLENGTH:
            return LOWORD(SBGetText(pStatusInfo, 0, NULL, 0, SBGT_ANSI));
#else
        case WM_GETTEXT:
            return LOWORD(SBGetText(pStatusInfo, 0, (LPVOID)lParam, (int)wParam, SBGT_TCHAR));
        case WM_GETTEXTLENGTH:
            return LOWORD(SBGetText(pStatusInfo, 0, NULL, 0, SBGT_TCHAR));
#endif

        case SB_GETTEXT:
            /* We assume the buffer is large enough to hold the string, just
             * as listboxes do; the app should call SB_GETTEXTLEN first
             */
            return SBGetText(pStatusInfo, wParam, (LPVOID)lParam, SBGT_INFINITE, SBGT_TCHAR | SBGT_OWNERDRAWOK);

        case SB_GETTEXTLENGTH:
            return SBGetText(pStatusInfo, wParam, NULL, 0, SBGT_TCHAR);


#ifdef UNICODE
        case SB_GETTEXTA:
            /* We assume the buffer is large enough to hold the string, just
             * as listboxes do; the app should call SB_GETTEXTLEN first
             */
            return SBGetText(pStatusInfo, wParam, (LPVOID)lParam, SBGT_INFINITE, SBGT_ANSI | SBGT_OWNERDRAWOK);

        case SB_GETTEXTLENGTHA:
            return SBGetText(pStatusInfo, wParam, NULL, 0, SBGT_ANSI);
#endif

        case SB_SETBKCOLOR:
        {
            COLORREF clr = pStatusInfo->_clrBk;
            pStatusInfo->_clrBk = (COLORREF)lParam;
            InvalidateRect(hWnd, NULL, TRUE);
            return clr;
        }

        case SB_SETPARTS:
            if (!wParam || wParam>MAXPARTS)
                return FALSE;

            return SetStatusParts(pStatusInfo, (int) wParam, (LPINT)lParam);

        case SB_GETPARTS:
            if (lParam)
            {
                PSTRINGINFO pStringInfo;
                LPINT lpInt;

                /* Fill in the lesser of the number of entries asked for or
                 * the number of entries there are
                 */
                if (wParam > (WPARAM)pStatusInfo->nParts)
                    wParam = pStatusInfo->nParts;

                for (pStringInfo=pStatusInfo->sInfo, lpInt=(LPINT)lParam;
                    wParam>0; --wParam, ++pStringInfo, ++lpInt)
                    *lpInt = pStringInfo->right;
            }

            /* Always return the number of actual entries
             */
            return(pStatusInfo->nParts);

        case SB_GETBORDERS:
            ((LPINT)lParam)[0] = pStatusInfo->nBorderX;
            ((LPINT)lParam)[1] = pStatusInfo->nBorderY;
            ((LPINT)lParam)[2] = pStatusInfo->nBorderPart;
            return TRUE;
            
        case SB_ISSIMPLE:
            return !(pStatusInfo->sSimple.uType & SBT_NOSIMPLE);

        case SB_GETRECT:
            return Status_GetRect(pStatusInfo, (int)wParam, (LPRECT)lParam);

        case SB_SETMINHEIGHT:     // this is a substitute for WM_MEASUREITEM
            pStatusInfo->nMinHeight = (int) wParam;
            RecalcTooltipRects(pStatusInfo);
            break;

        case SB_SIMPLE:
        {
            BOOL bInvalidate = FALSE;

            if (wParam)
            {
                if (pStatusInfo->sSimple.uType & SBT_NOSIMPLE)
                {
                    pStatusInfo->sSimple.uType &= ~SBT_NOSIMPLE;
                    bInvalidate = TRUE;
                }
            }
            else
            {
                if ((pStatusInfo->sSimple.uType & SBT_NOSIMPLE) == 0)
                {
                    pStatusInfo->sSimple.uType |= SBT_NOSIMPLE;
                    bInvalidate = TRUE;
                }
            }

            if (bInvalidate) {
                DebugMsg(TF_STATUS, TEXT("SB_SIMPLE: %d"), wParam);
                RecalcTooltipRects(pStatusInfo);
                SendNotifyEx(pStatusInfo->ci.hwndParent, pStatusInfo->ci.hwnd, SBN_SIMPLEMODECHANGE, NULL, FALSE);
                InvalidateRect(pStatusInfo->ci.hwnd, NULL, TRUE);
            }
            break;
        }

        case SB_SETICON:
        case SB_GETICON:
        {
            PSTRINGINFO pStringInfo = NULL;

            // -1 implies we are setting the icon for sSimple
            if ((UINT_PTR)-1 == wParam)
                pStringInfo = &pStatusInfo->sSimple;
            else if(wParam < (UINT)pStatusInfo->nParts)
                pStringInfo = &pStatusInfo->sInfo[wParam];
                
            if (uMsg == SB_GETICON)
                return (LRESULT)(pStringInfo ? pStringInfo->hIcon : NULL);
                
            if (pStringInfo && (pStringInfo->hIcon != (HICON)lParam))
            {
                BITMAP bm = {0};
                RECT rc;

                if (lParam)
                {
                    ICONINFO ii;

                    // Save the dimensions of the icon
                    GetIconInfo((HICON)lParam, &ii);
                    GetObject(ii.hbmColor, sizeof(BITMAP), &bm);
                    DeleteObject(ii.hbmColor);
                    DeleteObject(ii.hbmMask);
                }

                pStringInfo->sizeIcon.cx = bm.bmWidth;
                pStringInfo->sizeIcon.cy = bm.bmHeight;
                pStringInfo->hIcon = (HICON)lParam;
                
                Status_GetRect(pStatusInfo, (int)wParam, &rc);
                InvalidateRect(pStatusInfo->ci.hwnd, &rc, TRUE);
                UpdateWindow(pStatusInfo->ci.hwnd);
            }
            return TRUE;
        }

        // HIWORD(wParam) is the cbChar
        // LOWORD(wParam) is the nPart 
        case SB_GETTIPTEXT:
        {
            PSTRINGINFO pStringInfo = GetStringInfo(pStatusInfo, LOWORD(wParam));

            if (pStringInfo)
                lstrcpyn((LPTSTR)lParam, pStringInfo->pszToolTip,  HIWORD(wParam));
                
            break;
        }
        
        case SB_SETTIPTEXT:
        {
            PSTRINGINFO pStringInfo = GetStringInfo(pStatusInfo, (int) wParam);

            if (pStringInfo)
                Str_Set(&pStringInfo->pszToolTip, (LPCTSTR)lParam);

            break;    
        }    
#ifdef UNICODE
        case SB_GETTIPTEXTA:
        {
            PSTRINGINFO pStringInfo = GetStringInfo(pStatusInfo, LOWORD(wParam));

            if (pStringInfo)
                WideCharToMultiByte(CP_ACP, 0, pStringInfo->pszToolTip, -1, (LPSTR)lParam,
                                        HIWORD(wParam), NULL, NULL);
            break;
        }
        
        case SB_SETTIPTEXTA:
        {
            PSTRINGINFO pStringInfo = GetStringInfo(pStatusInfo, (int) wParam);
            LPTSTR lpsz;

            lpsz = ProduceWFromA(pStatusInfo->uiCodePage, (LPSTR)lParam);
                
            if (pStringInfo)
                Str_Set(&pStringInfo->pszToolTip, (LPCTSTR)lpsz);

            LocalFree(lpsz);
            break;    
        }    
#endif

#define lpNmhdr ((LPNMHDR)(lParam))
#define lpnmTT ((LPTOOLTIPTEXT) lParam)
#define IsTextPtr(lpszText)  (((lpszText) != LPSTR_TEXTCALLBACK) && (HIWORD64(lpszText)))
        case WM_NOTIFY:
        {
            PSTRINGINFO pStringInfo = NULL;
            if (lpNmhdr->code == TTN_NEEDTEXT) 
            {
                pStringInfo = GetStringInfo(pStatusInfo, (int) lpNmhdr->idFrom);
                if (!pStringInfo || !pStringInfo->fNeedToTip)
                    break;
            }
            //
            // We are just going to pass this on to the
            // real parent.  Note that -1 is used as
            // the hwndFrom.  This prevents SendNotifyEx
            // from updating the NMHDR structure.
            //
            SendNotifyEx(pStatusInfo->ci.hwndParent, (HWND) -1,
                   lpNmhdr->code, lpNmhdr, pStatusInfo->ci.bUnicode);

            if ((lpNmhdr->code == TTN_NEEDTEXT) && lpnmTT->lpszText 
                && IsTextPtr(lpnmTT->lpszText) && !lpnmTT->lpszText[0])
            {    
                if (pStringInfo)
                    lpnmTT->lpszText = pStringInfo->pszToolTip;
            }
            break;
        }    

        case WM_NOTIFYFORMAT:
            return CIHandleNotifyFormat(&pStatusInfo->ci, lParam);
            
        case WM_SETFONT:
            if (!pStatusInfo)
                return FALSE;

            SBSetFont(pStatusInfo, (HFONT)wParam, (BOOL)lParam);
            return TRUE;
            
        case WM_LBUTTONUP:
            nNotification = NM_CLICK;
            StatusForceCreateTooltips(pStatusInfo);
            RelayToToolTips(pStatusInfo->hwndToolTips, hWnd, uMsg, wParam, lParam);
            goto SendNotify;
        
        case WM_LBUTTONDBLCLK:
            nNotification = NM_DBLCLK;
            goto SendNotify;
        
        case WM_RBUTTONDBLCLK:
            nNotification = NM_RDBLCLK;
            goto SendNotify;
        
        case WM_RBUTTONUP:
            nNotification = NM_RCLICK;
SendNotify:
            LPARAM_TO_POINT(lParam, nm.pt);
            nm.dwItemSpec = IndexFromPt(pStatusInfo, nm.pt);
            if (!SendNotifyEx(pStatusInfo->ci.hwndParent, pStatusInfo->ci.hwnd, nNotification, (LPNMHDR)&nm,FALSE))
                goto DoDefault;
            return 0;

        case WM_GETFONT:
            if (!pStatusInfo)
                return 0;

            return (LRESULT)pStatusInfo->hStatFont;

        case WM_SIZE:
        {
            int nHeight;
            RECT rc;
            LPTSTR lpStr;
            PSTRINGINFO pStringInfo;
            int i, nTabs;

            if (!pStatusInfo)
                return 0;

            GetWindowRect(pStatusInfo->ci.hwnd, &rc);
            rc.right -= rc.left;    // -> dx
            rc.bottom -= rc.top;    // -> dy

            // If there is no parent, then this is a top level window
            if (pStatusInfo->ci.hwndParent)
            {
                ScreenToClient(pStatusInfo->ci.hwndParent, (LPPOINT)&rc);

                //
                // Places the status bar properly
                //
                if (pStatusInfo->ci.dwExStyle&RTL_MIRRORED_WINDOW)
                    rc.left -= rc.right;  
            }

            // need room for text, 3d border, and extra edge
            nHeight = 
                max(pStatusInfo->nFontHeight, g_cySmIcon) + 2 * g_cyBorder ;

            if (nHeight < pStatusInfo->nMinHeight)
                nHeight = pStatusInfo->nMinHeight;
             nHeight += pStatusInfo->nBorderY;

            // we don't have a divider thing -> force CCS_NODIVIDER

            NewSize(pStatusInfo->ci.hwnd, nHeight, GetWindowStyle(pStatusInfo->ci.hwnd) | CCS_NODIVIDER,
                rc.left, rc.top, rc.right, rc.bottom);

            // If the pane is right aligned then we need to invalidate all the pane
            // to force paint the entire pane. because the system will invalidate none if 
            // the status bar get shrieked or only the new added part if the status bar 
            // get grow and this does not work with the right justified text.
            pStringInfo = pStatusInfo->sInfo;
            for (i = 0; i < pStatusInfo->nParts; ++i, ++pStringInfo)
            {
                if ((pStringInfo->uType&SBT_ALLTYPES) == SBT_NORMAL &&
                    (lpStr = (LPTSTR)(pStringInfo->dwString)) != NULL)
                {
                    for ( nTabs = 0; (lpStr = StrChr(lpStr, TEXT('\t'))) != NULL; lpStr++) 
                    {
                        nTabs++;
                    }
                    if ( nTabs >= 2)
                    {
                        Status_GetRect(pStatusInfo, i, &rc);
                        InvalidateRect(pStatusInfo->ci.hwnd, &rc, FALSE);
                    }
                }
            }

            // need to invalidate the right end of the status bar
            // to maintain the finished edge look.
            GetClientRect(pStatusInfo->ci.hwnd, &rc);

            if (rc.right > pStatusInfo->nLastX)
                rc.left = pStatusInfo->nLastX;
            else
                rc.left = rc.right;
            rc.left -= (g_cxBorder + pStatusInfo->nBorderX);
            if (pStatusInfo->dxGripper)
                rc.left -= pStatusInfo->dxGripper;
            else
                rc.left -= pStatusInfo->nBorderPart;
            
#ifdef WINNT
            //
            // REVIEW:    Should we have to erase the bkgnd ?
            //

            InvalidateRect(pStatusInfo->ci.hwnd, &rc, TRUE);
#else
            InvalidateRect(pStatusInfo->ci.hwnd, &rc, FALSE);
#endif
            RecalcTooltipRects(pStatusInfo);
            pStatusInfo->nLastX = rc.right;
            break;
        }

        case WM_PRINTCLIENT:
        case WM_PAINT:
            if (!pStatusInfo)
                break;

            if (pStatusInfo->sSimple.uType & SBT_NOSIMPLE)
                PaintStatusWnd(pStatusInfo, (HDC)wParam, pStatusInfo->sInfo, pStatusInfo->nParts, pStatusInfo->nBorderX);
            else
                PaintStatusWnd(pStatusInfo, (HDC)wParam, &pStatusInfo->sSimple, 1, 0);

            return 0;       

        case WM_ERASEBKGND:
            if (pStatusInfo) {                    
               if (pStatusInfo->_clrBk != CLR_DEFAULT) {
                    RECT rc;
                    GetClientRect(hWnd, &rc);            
                    FillRectClr((HDC)wParam, &rc, pStatusInfo->_clrBk);        
                    return 1;
                }
            }
            goto DoDefault;

        case WM_GETOBJECT:
            if( lParam == OBJID_QUERYCLASSNAMEIDX )
                return MSAA_CLASSNAMEIDX_STATUS;
            goto DoDefault;

        default:
        {
            LRESULT lres;
            if (CCWndProc(&pStatusInfo->ci, uMsg, wParam, lParam, &lres))
                return lres;
        }
            break;
    }

DoDefault:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

#pragma code_seg(CODESEG_INIT)

BOOL FAR PASCAL InitStatusClass(HINSTANCE hInstance)
{
    WNDCLASS rClass;

    if (!GetClassInfo(hInstance, c_szStatusClass, &rClass))
    {
#ifndef WIN32
        extern LRESULT CALLBACK _StatusWndProc(HWND, UINT, WPARAM, LPARAM);
        rClass.lpfnWndProc        = _StatusWndProc;
#else
        rClass.lpfnWndProc        = (WNDPROC)StatusWndProc;
#endif
        rClass.style            = CS_DBLCLKS | CS_GLOBALCLASS |    CS_VREDRAW;
        rClass.cbClsExtra         = 0;
        rClass.cbWndExtra         = sizeof(PSTATUSINFO);
        rClass.hInstance        = hInstance;
        rClass.hIcon            = NULL;
        rClass.hCursor            = LoadCursor(NULL, IDC_ARROW);
        rClass.hbrBackground    = (HBRUSH)(COLOR_BTNFACE+1);
        rClass.lpszMenuName     = NULL;
        rClass.lpszClassName    = c_szStatusClass;

        return RegisterClass(&rClass);
    }
    return TRUE;
}
#pragma code_seg()


HWND WINAPI CreateStatusWindow(LONG style, LPCTSTR pszText, HWND hwndParent, UINT uID)
{
    // remove border styles to fix capone and other stupid apps

    return CreateWindowEx(0, c_szStatusClass, pszText, style & ~(WS_BORDER | CCS_NODIVIDER),
        -100, -100, 10, 10, hwndParent, (HMENU)uID, HINST_THISDLL, NULL);
}

#ifdef UNICODE
HWND WINAPI CreateStatusWindowA(LONG style, LPCSTR pszText, HWND hwndParent,
        UINT uID)
{
    // remove border styles to fix capone and other stupid apps

    return CreateWindowExA(0, STATUSCLASSNAMEA, pszText, style & ~(WS_BORDER | CCS_NODIVIDER),
        -100, -100, 10, 10, hwndParent, (HMENU)uID, HINST_THISDLL, NULL);
}
#else
HWND WINAPI CreateStatusWindowW(LONG style, LPCWSTR pszText, HWND hwndParent,
        UINT uID)
{
    SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
    return NULL;
}

#endif

void WINAPI DrawStatusTextEx(PSTATUSINFO pStatusInfo, HDC hDC, LPRECT lprc, LPCTSTR pszText, STRINGINFO * psi,  UINT uFlags)
{
    int len, nWidth = 0, nHeight = 0;
    HBRUSH hFaceBrush=NULL;
    COLORREF crTextColor, crBkColor;
    UINT uOpts = 0;
    BOOL bNull;
    int nOldMode;
    int i = 0, left = 0;
    LPTSTR lpTab, lpNext;
    TCHAR szBuf[MAX_STATUS_TEXT_LEN];
#if defined(WINDOWS_ME)
    int oldAlign;
#endif
    BOOL fDrawnIcon = FALSE;
    RECT rc = * lprc;

    //
    // IMPORTANT NOTE:
    // pStatusInfo can be NULL, please check before reference.
    //

#if defined(WINDOWS_ME)
    if (uFlags & SBT_RTLREADING)
    {
        oldAlign = GetTextAlign(hDC);
        SetTextAlign(hDC, oldAlign | TA_RTLREADING);
    }
#endif
    if (pszText)
        lstrcpyn(szBuf, pszText, ARRAYSIZE(szBuf));
    else
        szBuf[0] = TEXT('\0');

    //
    // Create the three brushes we need.    If the button face is a solid
    // color, then we will just draw in opaque, instead of using a
    // brush to avoid the flash
    //
    if (GetNearestColor(hDC, g_clrBtnFace) == g_clrBtnFace ||
        !(hFaceBrush = CreateSolidBrush(g_clrBtnFace)))
    {
        uOpts = ETO_CLIPPED | ETO_OPAQUE;
        nOldMode = SetBkMode(hDC, OPAQUE);
    }
    else
    {
        uOpts = ETO_CLIPPED;
        nOldMode = SetBkMode(hDC, TRANSPARENT);
    }
    crTextColor = SetTextColor(hDC, g_clrBtnText);
    if (pStatusInfo && (pStatusInfo->_clrBk != CLR_DEFAULT))
        crBkColor = SetBkColor(hDC, pStatusInfo->_clrBk);
    else
        crBkColor = SetBkColor(hDC, g_clrBtnFace);

    // Draw the hilites

    if (!(uFlags & SBT_NOBORDERS))
        // BF_ADJUST does the InflateRect stuff
        DrawEdge(hDC, &rc, (uFlags & SBT_POPOUT) ? BDR_RAISEDINNER : BDR_SUNKENOUTER, BF_RECT | BF_ADJUST);
    else
        InflateRect(&rc, -g_cxBorder, -g_cyBorder);
        
    if (hFaceBrush)
    {
        HBRUSH hOldBrush = SelectObject(hDC, hFaceBrush);
        if (hOldBrush)
        {
            PatBlt(hDC, rc.left, rc.top,
                   rc.right-rc.left, rc.bottom-rc.top, PATCOPY);
            SelectObject(hDC, hOldBrush);
        }
    }

    for (i=0, lpNext=szBuf, bNull=FALSE; i<3; ++i)
    {
        int cxIcon = 0;
        int leftIcon;
        UINT uiCodePage = pStatusInfo? pStatusInfo->uiCodePage: CP_ACP;
        /* Optimize for NULL left or center strings
         */
        if (!(uFlags & SBT_NOTABPARSING)) {
            if (*lpNext==TEXT('\t') && i<=1)
            {
                ++lpNext;
                continue;
            }
        }

        /* Determine the end of the current string
         */
        for (lpTab=lpNext; ; lpTab=CharNextEx((WORD)uiCodePage, lpTab, 0))
        {
            if (!*lpTab) {
                bNull = TRUE;
                break;
            } else if (!(uFlags & SBT_NOTABPARSING)) {
                if (*lpTab == TEXT('\t'))
                    break;
            }
        }
        *lpTab = TEXT('\0');
        len = lstrlen(lpNext);

        /* i=0 means left, 1 means center, and 2 means right justified text
         */
        MGetTextExtent(hDC, lpNext, len, &nWidth, &nHeight);

        if (psi) {
            if (psi->hIcon && !fDrawnIcon) {
                cxIcon = psi->sizeIcon.cx + g_cxEdge * 2;
                fDrawnIcon = TRUE;
            }
        }

        switch (i) {
            case 0:
                leftIcon = lprc->left + g_cxEdge;
                break;

            case 1:
                leftIcon = (lprc->left + lprc->right - (nWidth + cxIcon)) / 2;
                break;

            default:
                leftIcon = lprc->right - g_cxEdge - (nWidth + cxIcon);
                break;
        }    
        
        left = leftIcon + cxIcon;

        if (psi)
        {
            if (cxIcon) {
                int nTop = rc.top + ((rc.bottom - rc.top)  - (psi->sizeIcon.cy )) / 2 ;

                if (leftIcon > lprc->left) {
                    if (psi->hIcon)
                    {
                        DrawIconEx(hDC, leftIcon, nTop, psi->hIcon,
                                   psi->sizeIcon.cx, psi->sizeIcon.cy, 
                                   0, NULL, DI_NORMAL);
                    }
                }
                rc.left = leftIcon + cxIcon;
            }

            if (!*lpNext && cxIcon)
                psi->fNeedToTip = TRUE;
            else 
                psi->fNeedToTip  = (BOOL)(nWidth >= (rc.right - rc.left));
        }

        ExtTextOut(hDC, left, (rc.bottom - nHeight + rc.top) / 2, uOpts, &rc, lpNext, len, NULL);

        /* Now that we have drawn text once, take off the OPAQUE flag
         */
        uOpts = ETO_CLIPPED;

        if (bNull)
            break;

        *lpTab = TEXT('\t');
        lpNext = lpTab + 1;
    }

#if defined(WINDOWS_ME)
    if (uFlags & SBT_RTLREADING)
        SetTextAlign(hDC, oldAlign);
#endif

    SetTextColor(hDC, crTextColor);
    SetBkColor(hDC, crBkColor);
    SetBkMode(hDC, nOldMode);

    if (hFaceBrush)
        DeleteObject(hFaceBrush);

}

void RecalcTooltipRects(PSTATUSINFO pStatusInfo)
{
    if(pStatusInfo->hwndToolTips) 
    {
        UINT i;
        TOOLINFO ti;
        STRINGINFO * psi;

        ti.cbSize = sizeof(ti);
        ti.hwnd = pStatusInfo->ci.hwnd;
        ti.lpszText = LPSTR_TEXTCALLBACK;

        if (pStatusInfo->sSimple.uType & SBT_NOSIMPLE)
        {
            for ( i = 0, psi = pStatusInfo->sInfo; i < (UINT)pStatusInfo->nParts; i++, psi++) 
            {
                ti.uId = i;
                Status_GetRect(pStatusInfo, i, &ti.rect);
                SendMessage(pStatusInfo->hwndToolTips, TTM_NEWTOOLRECT, 0, (LPARAM)((LPTOOLINFO)&ti));
            }
            SetRect(&ti.rect, 0,0,0,0);
            ti.uId = SB_SIMPLEID;
            SendMessage(pStatusInfo->hwndToolTips, TTM_NEWTOOLRECT, 0, (LPARAM)((LPTOOLINFO)&ti));
        }
        else
        {

            GetClientRect(pStatusInfo->ci.hwnd, &ti.rect);
            InflateRect(&ti.rect, -g_cxBorder, -g_cyBorder);
            ti.uId = SB_SIMPLEID;
            SendMessage(pStatusInfo->hwndToolTips, TTM_NEWTOOLRECT, 0, (LPARAM)((LPTOOLINFO)&ti));
            SetRect(&ti.rect, 0,0,0,0);
            for ( i = 0, psi = pStatusInfo->sInfo; i < (UINT)pStatusInfo->nParts; i++, psi++) 
            {
                ti.uId = i;
                SendMessage(pStatusInfo->hwndToolTips, TTM_NEWTOOLRECT, 0, (LPARAM)((LPTOOLINFO)&ti));
            }
        }
    }    
   return;
}

PSTRINGINFO GetStringInfo(PSTATUSINFO pStatusInfo, int nIndex)
{
    PSTRINGINFO pRet = NULL;

    if (nIndex == SB_SIMPLEID)
        pRet = &pStatusInfo->sSimple;
    else if (nIndex < pStatusInfo->nParts)
        pRet = &pStatusInfo->sInfo[nIndex];

    return pRet;
}

int  IndexFromPt(PSTATUSINFO pStatusInfo, POINT pt)
{
    RECT rc;
    int nPart = 0;

    //
    //  More IE4 bug-for-bug compatibility.  IE4 tested for simple mode
    //  incorrectly.
    //
    if (pStatusInfo->ci.iVersion < 5)
    {
        // This is not a typo!  Well, actually, it *is* a typo, but it's
        // a typo we have to preserve for compatibility.  I don't know if
        // anybody relied on the typo, but I'm playing it safe.
        //
        // The bug was that in IE4, a click on a simple status bar usually
        // came back as SB_HITTEST_NOITEM instead of SB_SIMPLEID.
        //
        // I re-parenthesized the test so typo.pl won't trigger.  The original
        // IE4 code lacked the parentheses.

        if ((!pStatusInfo->sSimple.uType) & SBT_NOSIMPLE)
            return SB_SIMPLEID;
    }
    else
    {
        if (!(pStatusInfo->sSimple.uType & SBT_NOSIMPLE))
            return SB_SIMPLEID;
    }

    for(nPart = 0; nPart < pStatusInfo->nParts; nPart++)
    {
        Status_GetRect(pStatusInfo, nPart, &rc);
        if (PtInRect(&rc, pt))
            return nPart;
    }
    return SB_HITTEST_NOITEM;
}
