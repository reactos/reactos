#include "ctlspriv.h"
#include "treeview.h"
#include "image.h"

extern void  TruncateString(char *sz, int cch);

void NEAR TV_GetBackgroundBrush(PTREE pTree, HDC hdc)
{
    if (pTree->clrBk == (COLORREF)-1) {
        if (pTree->ci.style & WS_DISABLED)
            pTree->hbrBk = FORWARD_WM_CTLCOLORSTATIC(pTree->ci.hwndParent, hdc, pTree->ci.hwnd, SendMessage);
        else
            pTree->hbrBk = FORWARD_WM_CTLCOLOREDIT(pTree->ci.hwndParent, hdc, pTree->ci.hwnd, SendMessage);
    }
}

// ----------------------------------------------------------------------------
//
//  Draws a horizontal or vertical dotted line from the given (x,y) location
//  for the given length (c).
//
// ----------------------------------------------------------------------------

void NEAR TV_DrawDottedLine(HDC hdc, int x, int y, int c, BOOL fVert)
{
    while (c > 0)
    {
        PatBlt(hdc, x, y, 1, 1, PATCOPY);
        if (fVert)
            y += 2;
        else
            x += 2;
        c -= 2;
    }
}


// ----------------------------------------------------------------------------
//
//  Draws a plus or minus sign centered around the given (x,y) location and
//  extending out from that location the given distance (c).
//
// ----------------------------------------------------------------------------

void NEAR TV_DrawPlusMinus(HDC hdc, int x, int y, int c, HBRUSH hbrSign, HBRUSH hbrBox, HBRUSH hbrBk, BOOL fPlus)
{
    int n;
    int p = (c * 7) / 10;
    
    n = p * 2 + 1;
    
    SelectObject(hdc, hbrSign);
    
    if (p >= 5)
    {
        PatBlt(hdc, x - p, y - 1, n, 3, PATCOPY);
        if (fPlus)
            PatBlt(hdc, x - 1, y - p, 3, n, PATCOPY);
        
        SelectObject(hdc, hbrBk);
        p--;
        n -= 2;
    }
    
    PatBlt(hdc, x - p, y, n, 1, PATCOPY);
    if (fPlus)
        PatBlt(hdc, x, y - p, 1, n, PATCOPY);
    
    n = c * 2 + 1;
    
    SelectObject(hdc, hbrBox);
    
    PatBlt(hdc, x - c, y - c, n, 1, PATCOPY);
    PatBlt(hdc, x - c, y - c, 1, n, PATCOPY);
    PatBlt(hdc, x - c, y + c, n, 1, PATCOPY);
    PatBlt(hdc, x + c, y - c, 1, n, PATCOPY);
}


// ----------------------------------------------------------------------------
//
//  Create the bitmaps for the indent area of the tree as follows
//  if  fHasLines &&  fHasButtons --> 7 bitmaps
//  if  fHasLines && !fHasButtons --> 3 bitmaps
//  if !fHasLines &&  fHasButtons --> 2 bitmaps
//
//  sets hStartBmp, hBmp, hdcBits
//
//  If "has lines" then there are three basic bitmaps.
//
//      |       |       |
//      |       +---    +---
//      |       |
//
//  (The plan vertical line does not get buttons.)
//
//  Otherwise, there are no lines, so the basic bitmaps are blank.
//
//  If "has buttons", then the basic bitmaps are augmented with buttons.
//
//       [+]      [-]
//
//  And if you have "lines at root", you get
//
//      __
//
//
//  And if you have "lines at root" with "has buttons", then you also get
//
//      --[+]   --[-]
//
//  So, there are twelve image types.  Here they are, with the code names
//  written underneath.
//
//      |       |       |       |       |       |       |
//      |       +---    +---   [+]--   [+]--   [-]--   [-]--
//      |       |               |               |
//
//     "|"     "|-"    "L"     "|-+"   "L+"    "|--"   "L-"
//
//      ---    [+]--   [-]--   [+]     [-]
//
//     ".-"    ".-+"   ".--"   "+"     "-"
//
//      And the master table of which styles get which images.
//
//
//  LINES   BTNS    ROOT    |   |-  L   |-+ L+  |-- L-  .-  .-+ .-- +   -
//
//           x                                                      0   1
//    x                     0   1   2                   3
//    x                     0   1   2                   3
//    x      x              0   1   2   3   4   5   6
//    x              x      0   1   2                   3
//    x      x       x      0   1   2   3   4   5   6   7   8   9
//
// ----------------------------------------------------------------------------

void NEAR TV_CreateIndentBmps(PTREE pTree)
{
    int  cnt;
    RECT rc;
    HBRUSH hbrOld;
    int xMid, yMid;
    int x, c;
    HBITMAP hBmpOld;
    HBRUSH hbrLine;
    HBRUSH hbrText;
    HDC hdc;

    if (pTree->fRedraw)
        InvalidateRect(pTree->ci.hwnd, NULL, TRUE);
    
    if (pTree->ci.style & TVS_HASLINES)
    {
        if (pTree->ci.style & TVS_HASBUTTONS)
            cnt = 7;  //   | |-  L   |-+ L+  |-- L-
        else
            cnt = 3;  //   | |-  L
        
        if (pTree->ci.style & TVS_LINESATROOT) {
            if (pTree->ci.style & TVS_HASBUTTONS)
                cnt += 3;    // -  -+ --
            else 
                cnt += 1;    // -
        }
    }
    else if (pTree->ci.style & TVS_HASBUTTONS)
        cnt = 2;
    else
        return;

    if (!pTree->hdcBits)
        pTree->hdcBits = CreateCompatibleDC(NULL);
    
    hdc = pTree->hdcBits;
    
    // Get a new background brush, just like an Edit does.
    
    TV_GetBackgroundBrush(pTree, hdc);
    
    hBmpOld = pTree->hBmp;
    pTree->hBmp = CreateColorBitmap(cnt * pTree->cxIndent, pTree->cyItem);
    if (hBmpOld) {
        SelectObject(hdc, pTree->hBmp);
        DeleteObject(hBmpOld);
    } else
        pTree->hStartBmp = SelectObject(hdc, pTree->hBmp);

    if (pTree->clrLine != CLR_DEFAULT)
        hbrLine = CreateSolidBrush(pTree->clrLine);
    else
        hbrLine = g_hbrGrayText;

    if (pTree->clrText != (COLORREF)-1)
        hbrText = CreateSolidBrush(pTree->clrText);
    else
        hbrText = g_hbrWindowText;

    hbrOld = SelectObject(hdc, hbrLine);

    rc.top = 0;
    rc.left = 0;
    rc.right = cnt * pTree->cxIndent;
    rc.bottom = pTree->cyItem;

    FillRect(hdc, &rc, pTree->hbrBk);
    x = 0;
    
    if (pTree->hImageList)
        xMid = (pTree->cxImage - MAGIC_INDENT) / 2;
    else
        xMid = pTree->cxIndent / 2;
    
    yMid = ((pTree->cyItem / 2) + 1) & ~1;
    
    c = (min(xMid, yMid)) / 2;
    
    if (pTree->ci.style & TVS_HASLINES)
    {
        TV_DrawDottedLine(hdc, x + xMid, 0, pTree->cyItem, TRUE);
        x += pTree->cxIndent;
        
        TV_DrawDottedLine(hdc, x + xMid, 0, pTree->cyItem, TRUE);
        TV_DrawDottedLine(hdc, x + xMid, yMid, pTree->cxIndent - xMid, FALSE);
        x += pTree->cxIndent;
        
        TV_DrawDottedLine(hdc, x + xMid, 0, yMid, TRUE);
        TV_DrawDottedLine(hdc, x + xMid, yMid, pTree->cxIndent - xMid, FALSE);
        x += pTree->cxIndent;
    }
    
    if (pTree->ci.style & TVS_HASBUTTONS)
    {
        BOOL fPlus = TRUE;
        
        x += xMid;
        
doDrawPlusMinus:
        TV_DrawPlusMinus(hdc, x, yMid, c, hbrText, hbrLine, pTree->hbrBk, fPlus);
        
        if (pTree->ci.style & TVS_HASLINES)
        {
            TV_DrawDottedLine(hdc, x, 0, yMid - c, TRUE);
            TV_DrawDottedLine(hdc, x + c, yMid, pTree->cxIndent - xMid - c, FALSE);
            TV_DrawDottedLine(hdc, x, yMid + c, yMid - c, TRUE);
            
            x += pTree->cxIndent;
            
            TV_DrawPlusMinus(hdc, x, yMid, c, hbrText, hbrLine, pTree->hbrBk, fPlus);
            
            TV_DrawDottedLine(hdc, x, 0, yMid - c, TRUE);
            TV_DrawDottedLine(hdc, x + c, yMid, pTree->cxIndent - xMid - c, FALSE);
        }
        
        x += pTree->cxIndent;
        
        if (fPlus)
        {
            fPlus = FALSE;
            goto doDrawPlusMinus;
        }
        x -= xMid;
    }
    
    if (pTree->ci.style & TVS_LINESATROOT) {
        
        // -
        TV_DrawDottedLine(hdc, x + xMid, yMid, pTree->cxIndent - xMid, FALSE);
        x += pTree->cxIndent;
        
        if (pTree->ci.style & TVS_HASBUTTONS) {
            x += xMid;
            TV_DrawPlusMinus(hdc, x, yMid, c, hbrText, hbrLine, pTree->hbrBk, TRUE);
            TV_DrawDottedLine(hdc, x + c, yMid, pTree->cxIndent - xMid - c, FALSE);
            x += pTree->cxIndent;
            
            TV_DrawPlusMinus(hdc, x, yMid, c, hbrText, hbrLine, pTree->hbrBk, FALSE);
            TV_DrawDottedLine(hdc, x + c, yMid, pTree->cxIndent - xMid - c, FALSE);
            //  uncomment if there's more to be added
            //x += pTree->cxIndent - xMid;
            
        }
    }
    
    if (hbrOld)
        SelectObject(pTree->hdcBits, hbrOld);
    
    if (pTree->clrLine != CLR_DEFAULT)
        DeleteObject(hbrLine);
    if (pTree->clrText != (COLORREF)-1)
        DeleteObject(hbrText);
}


// ----------------------------------------------------------------------------
//
//  fills in a TVITEM structure based by coying data from the item or
//  by calling the callback to get it.
//
//  in:
//	hItem	item to get TVITEM struct for
//	mask	which bits of the TVITEM struct you want (TVIF_ flags)
//  out:
//	lpItem	TVITEM filled in
//
// ----------------------------------------------------------------------------

void NEAR TV_GetItem(PTREE pTree, HTREEITEM hItem, UINT mask, LPTVITEMEX lpItem)
{
    TV_DISPINFO nm;
    
    if (!hItem || !lpItem)
        return;
    
    DBG_ValidateTreeItem(hItem, FALSE);

    nm.item.mask = 0;
    
    // We need to check the mask to see if lpItem->pszText is valid
    // And even then, it might not be, so be paranoid
    if ((mask & TVIF_TEXT) && lpItem->pszText && lpItem->cchTextMax) {
        if (hItem->lpstr == LPSTR_TEXTCALLBACK) {
            nm.item.mask |= TVIF_TEXT;
            // caller had to fill in pszText and cchTextMax with valid data
            nm.item.pszText = lpItem->pszText;
            nm.item.cchTextMax = lpItem->cchTextMax;
            nm.item.pszText[0] = 0;

        } else {
            ASSERT(hItem->lpstr);
            // we could do this but this is dangerous (when responding
            // to TVM_GETITEM we would be giving the app a pointer to our data)
            // lpItem->pszText = hItem->lpstr;
            lstrcpyn(lpItem->pszText, hItem->lpstr, lpItem->cchTextMax);
#ifndef UNICODE
            // only call truncate string if the source string is larger than the dest buffer
            // this is to deal with corel draw who passes in a bogus cchTextMax value
            //
            // We used to always call TruncateString when cchTextMax is MAX_PATH, but
            // McAfee Virus program (QFE1381) passes MAX_PATH with a smaller than MAX_PATH buffer
            // so we must always check the string length first.  They luck out
            // and lstrlen(hItem->lpstr) is smaller than max path so we don't truncate.
            //
            if (lstrlen(hItem->lpstr) >= lpItem->cchTextMax) {
                // takes care of broken dbcs sequence, note lstrcpyn puts nul at
                // cchTextMax-1 if exceeded
                TruncateString(lpItem->pszText, lpItem->cchTextMax);
            }
#endif
        }

    }
    
    if (mask & TVIF_IMAGE) {
        if (hItem->iImage == (WORD)I_IMAGECALLBACK)
            nm.item.mask |= TVIF_IMAGE;
        else
            lpItem->iImage = hItem->iImage;
    }
    
    if (mask & TVIF_SELECTEDIMAGE) {
        if (hItem->iSelectedImage == (WORD)I_IMAGECALLBACK)
            nm.item.mask |= TVIF_SELECTEDIMAGE;
        else
            lpItem->iSelectedImage = hItem->iSelectedImage;
    }
    
    if (mask & TVIF_INTEGRAL) {
        lpItem->iIntegral = hItem->iIntegral;
    }
    
    if (mask & TVIF_CHILDREN) {
        switch (hItem->fKids) {
        case KIDS_COMPUTE:
            lpItem->cChildren = hItem->hKids ? 1 : 0;// the actual count doesn't matter
            break;
            
        case KIDS_FORCE_YES:
            lpItem->cChildren = 1;// the actual count doesn't matter
            break;
            
        case KIDS_FORCE_NO:
            lpItem->cChildren = 0;
            break;
            
        case KIDS_CALLBACK:
            nm.item.mask |= TVIF_CHILDREN;
            break;
        }
    }

    // copy out constant parameters (and prepare for callback)
    // IE4 and IE5.0 did this unconditionally
    lpItem->state = nm.item.state = hItem->state;

    //
    //  NOTICE!  We do not set TVIF_STATE nm.item.mask and we do not
    //  check for TVIF_STATE in the "any items need to be filled in
    //  by callback?" test a few lines below.  This is necessary for
    //  backwards compat.  IE5 and earlier did not call the app back
    //  if the only thing you asked for was TVIF_STATE.  You can't
    //  change this behavior unless you guard it with a version check, or
    //  apps will break.  (They'll get callbacks when they didn't used to.)
    //  Besides, nobody knows that they can customize the state, so it's
    //  not like we're missing out on anything.
    //

#ifdef DEBUG_TEST_BOLD
    if ((((int)hItem) / 100) % 2)
        lpItem->state |= TVIS_BOLD;
    if (!pTree->hFontBold)
        TV_CreateBoldFont(pTree);
#endif
    
    lpItem->lParam = nm.item.lParam = hItem->lParam;
    
    // any items need to be filled in by callback?
    if (nm.item.mask & (TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN)) {
        nm.item.hItem = hItem;
        
        CCSendNotify(&pTree->ci, TVN_GETDISPINFO, &nm.hdr);

        // copy out things that may have been filled in on the callback
        if (nm.item.mask & TVIF_CHILDREN)
            lpItem->cChildren = nm.item.cChildren;
        if (nm.item.mask & TVIF_IMAGE)
            lpItem->iImage = nm.item.iImage;
        if (nm.item.mask & TVIF_SELECTEDIMAGE)
            lpItem->iSelectedImage = nm.item.iSelectedImage;
        // callback may have redirected pszText to point into its own buffer
        if (nm.item.mask & TVIF_TEXT)
            if (mask & TVIF_TEXT) // did the *original* mask specify TVIF_TEXT?
                lpItem->pszText = CCReturnDispInfoText(nm.item.pszText, lpItem->pszText, lpItem->cchTextMax);
            else
                lpItem->pszText = nm.item.pszText;   // do what we used to do
        if (nm.item.mask & TVIF_STATE) {
            lpItem->state = (nm.item.state & nm.item.stateMask) | (lpItem->state & ~nm.item.stateMask);
            if ((lpItem->state & TVIS_BOLD) && !pTree->hFontBold)
                TV_CreateBoldFont(pTree);
        }
        
        
        if (nm.item.mask & TVIF_DI_SETITEM) {
            
            if (nm.item.mask & TVIF_TEXT)
                if (nm.item.pszText) {
                    ASSERT(hItem->lpstr == LPSTR_TEXTCALLBACK);
                    Str_Set(&hItem->lpstr, nm.item.pszText);
                }
                if (nm.item.mask & TVIF_STATE) {
                    // if the bold bit changed, then the width changed
                    if ((hItem->state ^ lpItem->state) & TVIS_BOLD)
                        hItem->iWidth = 0;
                    hItem->state = (WORD) lpItem->state;
                }
                if (nm.item.mask & TVIF_IMAGE)
                    hItem->iImage = (WORD) lpItem->iImage;
                if (nm.item.mask & TVIF_SELECTEDIMAGE)
                    hItem->iSelectedImage = (WORD) lpItem->iSelectedImage;
                if (nm.item.mask & TVIF_CHILDREN) {
                    switch(nm.item.cChildren) {
                    case I_CHILDRENCALLBACK:
                        hItem->fKids = KIDS_CALLBACK;
                        break;
                        
                    case 0:
                        hItem->fKids = KIDS_FORCE_NO;
                        break;
                        
                    default:
                        hItem->fKids = KIDS_FORCE_YES;
                        break;
                    }
                    
                }
        }
    }
}


// ----------------------------------------------------------------------------
//
//  Draws the given item starting at the given (x,y) and extending down and to
//  the right.
//
// ----------------------------------------------------------------------------

BOOL NEAR TV_ShouldItemDrawBlue(PTREE pTree, TVITEMEX *ti, UINT flags) 
{
    return  ( (ti->state & TVIS_DROPHILITED) ||
        (!pTree->hDropTarget && 
        !(flags & TVDI_GRAYCTL) &&
        (ti->state & TVIS_SELECTED) &&
        pTree->fFocus));
}

#define TV_ShouldItemDrawDisabled(pTree, pti, flags) (flags & TVDI_GRAYCTL)

//
//  Caution:  Depending on the user's color scheme, a Gray item may
//  end up looking Blue if Gray would otherwise be invisible.  So make
//  sure that there are other cues that the user can use to tell whether
//  the item is "Really Blue" or "Gray masquerading as Blue".
//
//  For example, you might get both is if the treeview is
//  participating in drag/drop while it is not the active window,
//  because the selected item gets "Gray masquerading as Blue" and
//  the drop target gets "Really Blue".  But we special-case that
//  and turn off the selection while we are worrying about drag/drop,
//  so there is no confusion after all.
//
BOOL TV_ShouldItemDrawGray(PTREE pTree, TVITEMEX *pti, UINT flags) 
{
    return  ((flags & TVDI_GRAYCTL) ||
        (!pTree->hDropTarget && 
        ((pti->state & TVIS_SELECTED) &&
        (!pTree->fFocus && (pTree->ci.style & TVS_SHOWSELALWAYS)) )));
}

//
//  Draw a descender line for the item.  It is the caller's job to
//  draw the appropriate glyph at level 0.
//
void
TV_DrawDescender(PTREE pTree, HDC hdc, int x, int y, HTREEITEM hItem)
{
    int i;
    for (i = 1; i < hItem->iIntegral; i++)
        BitBlt(hdc, x, y + i * pTree->cyItem, pTree->cxIndent, pTree->cyItem, pTree->hdcBits, 0, 0, SRCCOPY);

}

//
//  Erase any previous descender line for the item.
//
void
TV_EraseDescender(PTREE pTree, HDC hdc, int x, int y, HTREEITEM hItem)
{
    RECT rc;
    rc.left = x;
    rc.right = x + pTree->cxIndent;
    rc.top = y + pTree->cyItem;
    rc.bottom = y + hItem->iIntegral * pTree->cyItem;
    FillRect(hdc, &rc, pTree->hbrBk);
}

//
//  Draw (or erase) descenders for siblings and children.
//
void TV_DrawKinDescender(PTREE pTree, HDC hdc, int x, int y, HTREEITEM hItem, UINT state)
{
    if (hItem->hNext)   // Connect to next sibling
        TV_DrawDescender(pTree, hdc, x, y, hItem);
    else
        TV_EraseDescender(pTree, hdc, x, y, hItem);

    // If any bonus images, then need to connect the image to the kids.
    if (pTree->himlState || pTree->hImageList) {
        if (state & (TVIS_EXPANDED | TVIS_EXPANDPARTIAL)) // Connect to expanded kids
            TV_DrawDescender(pTree, hdc, x + pTree->cxIndent, y, hItem);
        else
            TV_EraseDescender(pTree, hdc, x + pTree->cxIndent, y, hItem);
    }
}

void NEAR TV_DrawItem(PTREE pTree, HTREEITEM hItem, HDC hdc, int x, int y, UINT flags)
{
    UINT cxIndent = pTree->cxIndent;
    COLORREF rgbOldBack = 0, rgbOldText;
    COLORREF clrBk = CLR_DEFAULT;
    RECT rc;
    int iBack, iText;
    HTREEITEM hItemSave = hItem;
    LPTSTR lpstr;
    int cch;
    UINT etoFlags = ETO_OPAQUE | ETO_CLIPPED;
    TVITEMEX ti;
    TCHAR szTemp[MAX_PATH];
    int iState = 0;
    HFONT hFont;                        //$BOLD
    DWORD dwRet;
    NMTVCUSTOMDRAW nmcd;
    BOOL fItemFocused = ((pTree->fFocus) && (hItem == pTree->hCaret));
    DWORD clrTextTemp, clrTextBkTemp;
    BOOL fSelectedIcon = FALSE;

    rc.top = y;
    rc.bottom = rc.top + (pTree->cyItem * hItem->iIntegral);
    rc.left = 0;
    rc.right = pTree->cxWnd;
    
    if (flags & TVDI_ERASE) {
        // Opaque the whole item
        FillRect(hdc, &rc, pTree->hbrBk);
    }
    
    
    // make sure the callbacks don't invalidate this item
    pTree->hItemPainting = hItem;	
    
    ti.pszText = szTemp;
    ti.cchTextMax  = ARRAYSIZE(szTemp);
    ti.stateMask = TVIS_OVERLAYMASK | TVIS_CUT | TVIS_BOLD; //$BOLD
    TV_GetItem(pTree, hItem, TVIF_IMAGE | TVIF_STATE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM, &ti);
    
    pTree->hItemPainting = NULL;
    
    
    ////////////////
    // set up the HDC

    if (TV_ShouldItemDrawBlue(pTree,&ti,flags)) {
        // selected 
        iBack = COLOR_HIGHLIGHT;
        iText = COLOR_HIGHLIGHTTEXT;
    } else if (TV_ShouldItemDrawDisabled(pTree, &pti, flags)) {
        iBack = COLOR_3DFACE;
        iText = COLOR_GRAYTEXT;
    } else if  (TV_ShouldItemDrawGray(pTree, &ti, flags)) {
        // On some color schemes, the BTNFACE color equals the WINDOW color,
        // and our gray comes out invisible.  In such case, change from gray
        // to blue so you can see it at all.
        if (GetSysColor(COLOR_WINDOW) != GetSysColor(COLOR_BTNFACE))
        {
            iBack = COLOR_BTNFACE;
            iText = COLOR_BTNTEXT;
        }
        else
        {
            iBack = COLOR_HIGHLIGHT;
            iText = COLOR_HIGHLIGHTTEXT;
        }
    } else {
        // not selected
        iBack = COLOR_WINDOW;
        iText = COLOR_WINDOWTEXT;
        if (hItem == pTree->hHot) {
            iText = COLOR_HOTLIGHT;
        }
    }
    
    if (iBack == COLOR_WINDOW && (pTree->clrBk != (COLORREF)-1))
        nmcd.clrTextBk = clrTextBkTemp = pTree->clrBk;
    else
        nmcd.clrTextBk = clrTextBkTemp = GetSysColor(iBack);

    if (iText == COLOR_WINDOWTEXT && (pTree->clrText != (COLORREF)-1))
        nmcd.clrText = clrTextTemp = pTree->clrText;
    else
        nmcd.clrText = clrTextTemp = GetSysColor(iText);

    // if forcing black and transparent, do so.  dc's BkMode should
    // already be set to TRANSPARENT by caller
    if (flags & TVDI_TRANSTEXT)
    {
        nmcd.clrText = clrTextTemp = 0x000000;
        etoFlags = 0;			// don't opaque nothin'
    }
    
    rgbOldBack = SetBkColor(hdc, nmcd.clrTextBk);
    rgbOldText = SetTextColor(hdc, nmcd.clrText);
    
    
#ifdef WINDOWS_ME
    if (pTree->ci.style & TVS_RTLREADING)
        etoFlags |= ETO_RTLREADING;
#endif
    
    // Figure out which font to use.    
    if (ti.state & TVIS_BOLD) {         
        hFont = pTree->hFontBold;
        if (hItem == pTree->hHot) {
            hFont = CCGetHotFont(pTree->hFontBold, &pTree->hFontBoldHot);
        }
    } else {                            
        hFont = pTree->hFont;
        if (hItem == pTree->hHot) {
            hFont = CCGetHotFont(pTree->hFont, &pTree->hFontHot);
        }
    }                                   
    hFont = SelectObject(hdc, hFont);   
    // End HDC setup
    ////////////////
    
    
    // notify on custom draw then do it!
    nmcd.nmcd.hdc = hdc;
    nmcd.nmcd.dwItemSpec = (DWORD_PTR)hItem;
    nmcd.nmcd.uItemState = 0;
    nmcd.nmcd.rc = rc;
    if (flags & TVDI_NOTREE)
        nmcd.iLevel = 0;
    else 
        nmcd.iLevel = hItem->iLevel;
    
    if (ti.state & TVIS_SELECTED) {
        
        fSelectedIcon = TRUE;
        
        if (pTree->fFocus || (pTree->ci.style & TVS_SHOWSELALWAYS))
            nmcd.nmcd.uItemState |= CDIS_SELECTED;
    }
    if (fItemFocused)
        nmcd.nmcd.uItemState |= CDIS_FOCUS;
    if (hItem == pTree->hHot)
        nmcd.nmcd.uItemState |= CDIS_HOT;

#ifdef KEYBOARDCUES
#if 0
    // BUGBUG: Custom draw stuff for UISTATE (stephstm)
    if (CCGetUIState(&(pTree->ci), KC_TBD))
        nmcd.nmcd.uItemState |= CDIS_SHOWKEYBOARDCUES;
#endif
#endif
    nmcd.nmcd.lItemlParam = ti.lParam;
    
    dwRet = CICustomDrawNotify(&pTree->ci, CDDS_ITEMPREPAINT, &nmcd.nmcd);
    if (dwRet & CDRF_SKIPDEFAULT) 
        return;
    
    fItemFocused = (nmcd.nmcd.uItemState & CDIS_FOCUS);
    if (nmcd.nmcd.uItemState & CDIS_SELECTED)
        ti.state |= TVIS_SELECTED;
    else {
        ti.state &= ~TVIS_SELECTED;
    }
    
    if (nmcd.clrTextBk != clrTextBkTemp)
        SetBkColor(hdc, nmcd.clrTextBk);
    
    if (nmcd.clrText != clrTextTemp)
        SetTextColor(hdc, nmcd.clrText);
    
    if (pTree->ci.style & TVS_FULLROWSELECT && 
         !(flags & TVDI_TRANSTEXT)) 
    {
        FillRectClr(hdc, &nmcd.nmcd.rc, GetBkColor(hdc));
        etoFlags |= ETO_OPAQUE;
        clrBk = CLR_NONE;
    }
    
    if (!(flags & TVDI_NOTREE)) {
        if ((pTree->ci.style & (TVS_HASLINES | TVS_HASBUTTONS)) &&
            (pTree->ci.style & TVS_LINESATROOT))
            // Make room for the "plus" at the front of the tree
            x += cxIndent;
    }
    
    
    // deal with margin, etc.
    x += (pTree->cxBorder + (nmcd.iLevel * cxIndent));
    y += pTree->cyBorder;
    
    // draw image
    if ((!(flags & TVDI_NOTREE) && !(dwRet & TVCDRF_NOIMAGES)) || (flags & TVDI_FORCEIMAGE))
    {
        int dx, dy;     // to clip the images within the borders.
        COLORREF clrImage = CLR_HILIGHT;
        COLORREF clrBkImage = clrBk;

        if (flags & TVDI_NOBK)
        {
            clrBkImage = CLR_NONE;
        }


        if (pTree->himlState) {
            iState = TV_StateIndex(&ti);
            // this sucks.  in the treeview, 0 for the state image index
            // means draw nothing... the 0th item is unused.
            // the listview is 0 based and uses the 0th item.  
            if (iState) {
                dx = min(pTree->cxState, pTree->cxMax - pTree->cxBorder - x);
                dy = min(pTree->cyState, pTree->cyItem - (2 * pTree->cyBorder));
                ImageList_DrawEx(pTree->himlState, iState, hdc, x, 
                    y + max(pTree->cyItem - pTree->cyState, 0), dx, dy, clrBk, CLR_DEFAULT, ILD_NORMAL);
                x += pTree->cxState;            
            }
        }
        
        if (pTree->hImageList) {
            UINT fStyle = 0;
            int i = (fSelectedIcon) ? ti.iSelectedImage : ti.iImage;
            
            if (ti.state & TVIS_CUT) {
                fStyle |= ILD_BLEND50;
                clrImage = ImageList_GetBkColor(pTree->hImageList);
            }
            
            dx = min(pTree->cxImage - MAGIC_INDENT, pTree->cxMax - pTree->cxBorder - x);
            dy = min(pTree->cyImage, pTree->cyItem - (2 * pTree->cyBorder));
            ImageList_DrawEx(pTree->hImageList, i, hdc,
                x, y + (max(pTree->cyItem - pTree->cyImage, 0) / 2), dx, dy,
                clrBkImage, clrImage,
                fStyle | (ti.state & TVIS_OVERLAYMASK));
            
        }
    }
    
    if (pTree->hImageList) {
        // even if not drawing image, draw text in right place
        x += pTree->cxImage;
    }
    
    // draw text
    lpstr = ti.pszText;
    cch = lstrlen(lpstr);
    
    if (!hItem->iWidth || (hItem->lpstr == LPSTR_TEXTCALLBACK))
    {
        TV_ComputeItemWidth(pTree, hItem, hdc); //$BOLD
    }
    
    rc.left = x;
    rc.top = y + pTree->cyBorder;
    rc.right = min((x + hItem->iWidth),
                   (pTree->cxMax - pTree->cxBorder));
    rc.bottom-= pTree->cyBorder;
    
    // Draw the text, unless it's the one we are editing
    if (pTree->htiEdit != hItem || !IsWindow(pTree->hwndEdit) || !IsWindowVisible(pTree->hwndEdit))
    {
        ExtTextOut(hdc, x + g_cxLabelMargin, y + ((pTree->cyItem - pTree->cyText) / 2) + g_cyBorder,
            etoFlags, &rc, lpstr, cch, NULL);
    
        // Draw the focus rect, if appropriate.
        if (pTree->fFocus && (fItemFocused) && 
            !(pTree->ci.style & TVS_FULLROWSELECT) &&
            !(flags & (TVDI_TRANSTEXT | TVDI_GRAYCTL))
#ifdef KEYBOARDCUES
			&& !(CCGetUIState(&(pTree->ci)) & UISF_HIDEFOCUS)
#endif
			)
            DrawFocusRect(hdc, &rc);
    }
    
    SetBkColor(hdc, rgbOldBack);
    SetTextColor(hdc, rgbOldText);
    
    // Restore the original font.       //$BOLD
    SelectObject(hdc, hFont);           //$BOLD
    
    // Notice that we should have opaque'd the rest of the line above if no tree
    if (!(flags & TVDI_NOTREE))
    {
        int dx, dy;
        
        if (pTree->hImageList)
            x -= pTree->cxImage;
        
        if (iState)
            x -= pTree->cxState;
        
        if (pTree->ci.style & TVS_HASLINES)
        {
            int i;

            x -= cxIndent;
            if (nmcd.iLevel-- || (pTree->ci.style & TVS_LINESATROOT))
            {
                // HACK: Special case the first root
                // We will draw a "last" sibling button upside down
                if (nmcd.iLevel == -1 && hItem == hItem->hParent->hKids)
                {
                    if (hItem->hNext) {
                        i = 2;              // "L"
                        if (ti.cChildren && (pTree->ci.style & TVS_HASBUTTONS))
                        {
                            i += 2;         // "L+"
                            if ((ti.state & (TVIS_EXPANDED | TVIS_EXPANDPARTIAL)) == TVIS_EXPANDED)
                                i += 2;     // "L-"
                        }
                        
                        dx = min((int)cxIndent, pTree->cxMax - pTree->cxBorder - x);
                        dy = pTree->cyItem - (2 * pTree->cyBorder);
                        StretchBlt(hdc, x, y + pTree->cyItem, cxIndent, -pTree->cyItem, pTree->hdcBits
                            , i * cxIndent, 0, dx, dy, SRCCOPY);
                        i = -1;
                    }
                    else 
                    {
                        // first root no siblings
                        // if there's no other item, draw just the button if button mode,
                        if (pTree->ci.style & TVS_HASBUTTONS)
                        {
                            if (ti.cChildren) {
                                // hasbuttons, has lines, lines at root
                                i = ((ti.state & (TVIS_EXPANDED | TVIS_EXPANDPARTIAL)) == TVIS_EXPANDED) ? 
                                    9 : 8;  // ".--" : ".-+"
                            } else {
                                i = 7;      // ".-"
                            }
                        }
                        else
                        {
                            i = 3;          // ".-"
                        }
                    }
                }
                else
                {
                    i = (hItem->hNext) ? 1 : 2; // "|-" (rep) : "L"
                    if (ti.cChildren && (pTree->ci.style & TVS_HASBUTTONS))
                    {
                        i += 2;                 // "|-+" (rep) : "L+"
                        if ((ti.state & (TVIS_EXPANDED | TVIS_EXPANDPARTIAL)) == TVIS_EXPANDED)
                            i += 2;             // "|--" (rep) : "L-"
                    }
                }
                if (hItem->iIntegral > 1)
                    TV_DrawKinDescender(pTree, hdc, x, y, hItem, ti.state);

                if (i != -1)
                {
                    dx = min((int)cxIndent, pTree->cxMax - pTree->cxBorder - x);
                    dy = pTree->cyItem - (2 * pTree->cyBorder);
                    if ((dx > 0) && (dy > 0))
                        BitBlt(hdc, x, y, dx, dy, pTree->hdcBits
                            , i * cxIndent, 0, SRCCOPY);
                }
                
                while ((--nmcd.iLevel >= 0) || ((pTree->ci.style & TVS_LINESATROOT) && nmcd.iLevel >= -1))
                {
                    hItem = hItem->hParent;
                    x -= cxIndent;
                    if (hItem->hNext)
                    {
                        dx = min((int)cxIndent, (pTree->cxMax - pTree->cxBorder - x));
                        dy = min(pTree->cyItem, pTree->cyWnd - pTree->cyBorder - y);
                        if ((dx > 0) && (dy > 0))
                            BitBlt(hdc, x, y, dx, dy, pTree->hdcBits, 0, 0, SRCCOPY);
                        TV_DrawDescender(pTree, hdc, x, y, hItemSave);
                    }
                }
            }
        }
        else
        {               // no lines
            if ((pTree->ci.style & TVS_HASBUTTONS) && (nmcd.iLevel || pTree->ci.style & TVS_LINESATROOT)
                && ti.cChildren)
            {
                int i = (ti.state & TVIS_EXPANDED) ? cxIndent : 0;
                
                x -= cxIndent;
                dx = min((int)cxIndent, pTree->cxMax - pTree->cxBorder - x);
                dy = min(pTree->cyItem, pTree->cyWnd - pTree->cyBorder - y);
                if ((dx > 0) && (dy > 0))
                    BitBlt(hdc, x, y, dx, dy, pTree->hdcBits, i, 0, SRCCOPY);
            }
        }
    }
    
    
    if (dwRet & CDRF_NOTIFYPOSTPAINT) {
        nmcd.nmcd.dwItemSpec = (DWORD_PTR)hItemSave;
        CICustomDrawNotify(&pTree->ci, CDDS_ITEMPOSTPAINT, &nmcd.nmcd);
    }
}

#define INSERTMARKSIZE      6

BOOL TV_GetInsertMarkRect(PTREE pTree, LPRECT prc)
{
    ASSERT(pTree);

    if(pTree->htiInsert && TV_GetItemRect(pTree, pTree->htiInsert, prc, TRUE))
    {
        if (pTree->fInsertAfter)
            prc->top = prc->bottom;
        else
            prc->bottom = prc->top;
        
        prc->top -= INSERTMARKSIZE/2;
        prc->bottom += INSERTMARKSIZE/2 + 1;
        prc->right = pTree->cxWnd - INSERTMARKSIZE;      // should always go all the way to right with pad.
        prc->left -= pTree->cxImage;
        
        return TRUE;
    }
    return FALSE;
}

//  this is implemented in toolbar.c, but we should be able to use 
//  as well as long as we always set fHorizMode to FALSE
void PASCAL DrawInsertMark(HDC hdc, LPRECT prc, BOOL fHorizMode, COLORREF clr);

__inline COLORREF TV_GetInsertMarkColor(PTREE pTree)
{
    if (pTree->clrim == CLR_DEFAULT)
        return g_clrWindowText;
    else
        return pTree->clrim;
}

void NEAR TV_DrawTree(PTREE pTree, HDC hdc, BOOL fErase, LPRECT lprc)
{
    int x;
    int iStart, iCnt;
    UINT uFlags;
    RECT rc;
    NMCUSTOMDRAW nmcd;
    
    if (!pTree->fRedraw)
        return;

    if (pTree->ci.style & TVS_CHECKBOXES)
        if (!pTree->himlState)
            TV_InitCheckBoxes(pTree);
    
    x = -pTree->xPos;
    
    TV_GetBackgroundBrush(pTree, hdc);
    
    rc = *lprc;
    
#ifdef MAINWIN
    if (lprc->top <= 0) { /* fix microsoft BUG */
        iStart = 0;
    } else {
        iStart = lprc->top / pTree->cyItem;
    }
#else
    iStart = lprc->top / pTree->cyItem;
#endif

    if (pTree->cItems && pTree->hTop) {
        ASSERT(ITEM_VISIBLE(pTree->hTop));

        iCnt = pTree->cShowing - pTree->hTop->iShownIndex;
    } else {
        iCnt = 0;                   // Nothing to draw
    }

    nmcd.hdc = hdc;
    /// not implemented yet
    //if (ptb->ci.hwnd == GetFocus()) 
    //nmcd.uItemState = CDIS_FOCUS;
    //else 
    nmcd.uItemState = 0;
    nmcd.lItemlParam = 0;
    nmcd.rc = rc;
    pTree->ci.dwCustom = CICustomDrawNotify(&pTree->ci, CDDS_PREPAINT, &nmcd);
    if (!(pTree->ci.dwCustom & CDRF_SKIPDEFAULT)) {
        
        if (iStart < iCnt)
        {
            HTREEITEM   hItem;
            HFONT       hOldFont;
            RECT        rcT;
            int y = 0;
            
            for (hItem = pTree->hTop; hItem; ) {
                if (iStart > hItem->iIntegral) {
                    iStart -= hItem->iIntegral;
                    y += hItem->iIntegral * pTree->cyItem;
                    hItem = TV_GetNextVisItem(hItem);
                } else
                    break;
            }
            
            hOldFont = pTree->hFont ? SelectObject(hdc, pTree->hFont) : NULL;
            
            // TVDI_* for all items
            uFlags = (pTree->ci.style & WS_DISABLED) ? TVDI_GRAYCTL : 0;
            if (fErase)
                uFlags |= TVDI_ERASE;
            
            // loop from the first visible item until either all visible items are
            // drawn or there are no more items to draw
            for ( ; hItem && y < lprc->bottom; hItem = TV_GetNextVisItem(hItem))
            {
                TV_DrawItem(pTree, hItem, hdc, x, y, uFlags);
                y += pTree->cyItem * hItem->iIntegral;
            }
            
            //
            // handle drawing the InsertMark next to this item.
            //
            if(TV_GetInsertMarkRect(pTree, &rcT))
                DrawInsertMark(hdc, &rcT, FALSE, TV_GetInsertMarkColor(pTree));

            
            if (hOldFont)
                SelectObject(hdc, hOldFont);
            
            rc.top = y;
        }
        
        if (fErase)
            // Opaque out everything we have not drawn explicitly
            FillRect(hdc, &rc, pTree->hbrBk);
        
        // notify parent afterwards if they want us to
        if (pTree->ci.dwCustom & CDRF_NOTIFYPOSTPAINT) {
            CICustomDrawNotify(&pTree->ci, CDDS_POSTPAINT, &nmcd);
        }
    }

}


// ----------------------------------------------------------------------------
//
//  Set up for paint, call DrawTree, and clean up after paint.
//
// ----------------------------------------------------------------------------

void NEAR TV_Paint(PTREE pTree, HDC hdc)
{
    PAINTSTRUCT ps;
    
    if (hdc)
    {
        // hdc != 0 indicates a subclassed paint -- use the hdc passed in
        SetRect(&ps.rcPaint, 0, 0, pTree->cxWnd, pTree->cyWnd);
        TV_DrawTree(pTree, hdc, TRUE, &ps.rcPaint);
    }
    else
    {
        BeginPaint(pTree->ci.hwnd, &ps);
        TV_DrawTree(pTree, ps.hdc, ps.fErase, &ps.rcPaint);
        EndPaint(pTree->ci.hwnd, &ps);
    }
}

// ----------------------------------------------------------------------------
// Create an imagelist to be used for dragging.
//
// 1) create mask and image bitmap matching the select bounds size
// 2) draw the text to both bitmaps (in black for now)
// 3) create an imagelist with these bitmaps
// 4) make a dithered copy of the image onto the new imagelist
// ----------------------------------------------------------------------------

HIMAGELIST NEAR TV_CreateDragImage(PTREE pTree, HTREEITEM hItem)
{
    HDC hdcMem = NULL;
    HBITMAP hbmImage = NULL;
    HBITMAP hbmMask = NULL;
    HBITMAP hbmOld;
    HIMAGELIST himl = NULL;
    BOOL bMirroredWnd = (pTree->ci.dwExStyle&RTL_MIRRORED_WINDOW);
    int dx, dy;
    int iSrc;

    TVITEMEX ti;

    if (!pTree->hImageList)
        return NULL;

    if (hItem == NULL)
        hItem = pTree->htiDrag;

    if (hItem == NULL)
        return NULL;

    
    // BUGBUG??? we know it's already been drawn, so is iWidth valid???
    dx = hItem->iWidth + pTree->cxImage;
    dy = pTree->cyItem;
    
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
    if (bMirroredWnd) {
        SET_DC_RTL_MIRRORED(hdcMem);
    }

    // prepare for drawing the item
    if (pTree->hFont)
        SelectObject(hdcMem, pTree->hFont);
    SetBkMode(hdcMem, TRANSPARENT);
    
    /*
    ** draw the text to both bitmaps
    */
    hbmOld = SelectObject(hdcMem, hbmImage);
    // fill image with black for transparency
    PatBlt(hdcMem, 0, 0, dx, dy, BLACKNESS);
    TV_DrawItem(pTree, hItem, hdcMem, 0, 0,
        TVDI_NOIMAGE | TVDI_NOTREE | TVDI_TRANSTEXT);

    //
    // If the header is RTL mirrored, then
    // mirror the Memory DC, so that when copying back
    // we don't get any image-flipping. [samera]
    //
    if (bMirroredWnd)
        MirrorBitmapInDC(hdcMem, hbmImage);

    SelectObject(hdcMem, hbmMask);
    // fill mask with white for transparency
    PatBlt(hdcMem, 0, 0, dx, dy, WHITENESS);
    TV_DrawItem(pTree, hItem, hdcMem, 0, 0,
        TVDI_NOIMAGE | TVDI_NOTREE | TVDI_TRANSTEXT);
    
    //
    // If the header is RTL mirrored, then
    // mirror the Memory DC, so that when copying back
    // we don't get any image-flipping. [samera]
    //
    if (bMirroredWnd)
        MirrorBitmapInDC(hdcMem, hbmMask);

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
    TV_GetItem(pTree, hItem, TVIF_IMAGE, &ti);
    iSrc = ti.iImage;
    
    ImageList_CopyDitherImage(himl, 0, 0, (pTree->cyItem - pTree->cyImage) / 2,
        pTree->hImageList, iSrc, ((pTree->ci.dwExStyle & dwExStyleRTLMirrorWnd) ? ILD_MIRROR : 0L) | (hItem->state & TVIS_OVERLAYMASK));

CDI_Exit:
    if (hdcMem)
        DeleteObject(hdcMem);
    if (hbmImage)
        DeleteObject(hbmImage);
    if (hbmMask)
        DeleteObject(hbmMask);
    
    return himl;
}

#define COLORKEY RGB(0xF4, 0x0, 0x0)

LRESULT TV_GenerateDragImage(PTREE pTree, SHDRAGIMAGE* pshdi)
{
    LRESULT lRet = 0;
    HBITMAP hbmpOld = NULL;
    HTREEITEM hItem = pTree->htiDrag;
    RECT rc;
    HDC  hdcDragImage;

    if (hItem == NULL)
        return FALSE;

    hdcDragImage = CreateCompatibleDC(NULL);

    if (!hdcDragImage)
        return 0;

    // After this rc contains the bounds of all the items in Client Coordinates.
    //
    // Mirror the the DC, if the listview is mirrored.
    //
    if (pTree->ci.dwExStyle & RTL_MIRRORED_WINDOW)
    {
        SET_DC_RTL_MIRRORED(hdcDragImage);
    }

    TV_GetItemRect(pTree, hItem, &rc, TRUE);

    // Subtract off the image...
    rc.left -= pTree->cxImage;

    pshdi->sizeDragImage.cx = RECTWIDTH(rc);
    pshdi->sizeDragImage.cy = RECTHEIGHT(rc);
    pshdi->hbmpDragImage = CreateBitmap( pshdi->sizeDragImage.cx, pshdi->sizeDragImage.cy,
        GetDeviceCaps(hdcDragImage, PLANES), GetDeviceCaps(hdcDragImage, BITSPIXEL),
        NULL);

    if (pshdi->hbmpDragImage)
    {
        COLORREF clrBkSave;
        RECT  rcImage = {0, 0, pshdi->sizeDragImage.cx, pshdi->sizeDragImage.cy};

        hbmpOld = SelectObject(hdcDragImage, pshdi->hbmpDragImage);

        pshdi->crColorKey = COLORKEY;
        FillRectClr(hdcDragImage, &rcImage, pshdi->crColorKey);

        // Calculate the offset... The cursor should be in the bitmap rect.

        if (pTree->ci.dwExStyle & RTL_MIRRORED_WINDOW)
            pshdi->ptOffset.x = rc.right - pTree->ptCapture.x;
        else
            pshdi->ptOffset.x = pTree->ptCapture.x - rc.left;

        pshdi->ptOffset.y = pTree->ptCapture.y - rc.top;

        clrBkSave = pTree->clrBk;

        pTree->clrBk = COLORKEY;

        TV_DrawItem(pTree, hItem, hdcDragImage, 0, 0,
            TVDI_NOTREE | TVDI_TRANSTEXT | TVDI_FORCEIMAGE | TVDI_NOBK);

        pTree->clrBk = clrBkSave;

        SelectObject(hdcDragImage, hbmpOld);
        DeleteDC(hdcDragImage);

        // We're passing back the created HBMP.
        return 1;
    }


    return lRet;
}
