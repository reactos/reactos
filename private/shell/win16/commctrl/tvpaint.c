#include "ctlspriv.h"
#include "treeview.h"
#include "image.h"


void NEAR TV_GetBackgroundBrush(PTREE pTree, HDC hdc)
{

#ifdef WIN32
    if (pTree->style & WS_DISABLED)
        pTree->hbrBk = FORWARD_WM_CTLCOLORSTATIC(pTree->hwndParent, hdc, pTree->hwnd, SendMessage);
    else
        pTree->hbrBk = FORWARD_WM_CTLCOLOREDIT(pTree->hwndParent, hdc, pTree->hwnd, SendMessage);
#else
    pTree->hbrBk = FORWARD_WM_CTLCOLOR(pTree->hwndParent, hdc, pTree->hwnd,
        (pTree->style & WS_DISABLED)? CTLCOLOR_STATIC : CTLCOLOR_EDIT,
        SendMessage);
#endif

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
// ----------------------------------------------------------------------------

void NEAR TV_CreateIndentBmps(PTREE pTree)
{
    int  cnt;
    RECT rc;
    HBRUSH hbrOld;
    int xMid, yMid;
    int x, c;
    HBITMAP hBmpOld;
    HDC hdc;

    if (pTree->fRedraw)
        InvalidateRect(pTree->hwnd, NULL, TRUE);

    if (pTree->style & TVS_HASLINES)
    {
        if (pTree->style & TVS_HASBUTTONS)
            cnt = 7;  //   | |-  L   |+ L+  |- L-
        else
            cnt = 3;  //   | |-  L

        if (pTree->style & TVS_LINESATROOT) {
            if (pTree->style & TVS_HASBUTTONS)
                cnt += 3;    // -  -+ --
            else
                cnt += 1;    // -
        }
    }
    else if (pTree->style & TVS_HASBUTTONS)
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

    hbrOld = SelectObject(hdc, g_hbrGrayText);

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

    if (pTree->style & TVS_HASLINES)
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

    if (pTree->style & TVS_HASBUTTONS)
    {
        BOOL fPlus = TRUE;

        x += xMid;

doDrawPlusMinus:
        TV_DrawPlusMinus(hdc, x, yMid, c, g_hbrWindowText, g_hbrGrayText, pTree->hbrBk, fPlus);

        if (pTree->style & TVS_HASLINES)
        {
            TV_DrawDottedLine(hdc, x, 0, yMid - c, TRUE);
            TV_DrawDottedLine(hdc, x + c, yMid, pTree->cxIndent - xMid - c, FALSE);
            TV_DrawDottedLine(hdc, x, yMid + c, yMid - c, TRUE);

            x += pTree->cxIndent;

            TV_DrawPlusMinus(hdc, x, yMid, c, g_hbrWindowText, g_hbrGrayText, pTree->hbrBk, fPlus);

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

    if (pTree->style & TVS_LINESATROOT) {

        // -
        TV_DrawDottedLine(hdc, x + xMid, yMid, pTree->cxIndent - xMid, FALSE);
        x += pTree->cxIndent;

        if (pTree->style & TVS_HASBUTTONS) {
            x += xMid;
            TV_DrawPlusMinus(hdc, x, yMid, c, g_hbrWindowText, g_hbrGrayText, pTree->hbrBk, TRUE);
            TV_DrawDottedLine(hdc, x + c, yMid, pTree->cxIndent - xMid - c, FALSE);
            x += pTree->cxIndent;

            TV_DrawPlusMinus(hdc, x, yMid, c, g_hbrWindowText, g_hbrGrayText, pTree->hbrBk, FALSE);
            TV_DrawDottedLine(hdc, x + c, yMid, pTree->cxIndent - xMid - c, FALSE);
            //  uncomment if there's more to be added
            //x += pTree->cxIndent - xMid;

        }
    }

    if (hbrOld)
        SelectObject(pTree->hdcBits, hbrOld);

}


// ----------------------------------------------------------------------------
//
//  fills in a TV_ITEM structure based by coying data from the item or
//  by calling the callback to get it.
//
//  in:
//	hItem	item to get TV_ITEM struct for
//	mask	which bits of the TV_ITEM struct you want (TVIF_ flags)
//  out:
//	lpItem	TV_ITEM filled in
//
// ----------------------------------------------------------------------------

void NEAR TV_GetItem(PTREE pTree, HTREEITEM hItem, UINT mask, LPTV_ITEM lpItem)
{
    TV_DISPINFO nm;

    if (!hItem || !lpItem)
        return;

    nm.item.mask = 0;

    // We need to check the mask to see if lpItem->pszText is valid
    if (mask & TVIF_TEXT) {
        if (hItem->lpstr == LPSTR_TEXTCALLBACK) {
	    nm.item.mask |= TVIF_TEXT;
	    // caller had to fill in pszText and cchTextMax with valid data
            nm.item.pszText = lpItem->pszText;
	    nm.item.cchTextMax = lpItem->cchTextMax;
	    nm.item.pszText[0] = 0;
	} else {
	    Assert(hItem->lpstr);
	    // we could do this but this is dangerous (when responding
	    // to TVM_GETITEM we would be giving the app a pointer to our data)
            // lpItem->pszText = hItem->lpstr;
	    lstrcpyn(lpItem->pszText, hItem->lpstr, lpItem->cchTextMax);
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
    lpItem->state = nm.item.state = hItem->state;
    lpItem->lParam = nm.item.lParam = hItem->lParam;

    // any items need to be filled in by callback?
    if (nm.item.mask & (TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN)) {
        nm.item.hItem = hItem;

	SendNotify(pTree->hwndParent, pTree->hwnd, TVN_GETDISPINFO, &nm.hdr);

	// copy out things that may have been filled in on the callback
	if (nm.item.mask & TVIF_CHILDREN)
	    lpItem->cChildren = nm.item.cChildren;
	if (nm.item.mask & TVIF_IMAGE)
	    lpItem->iImage = nm.item.iImage;
        if (nm.item.mask & TVIF_SELECTEDIMAGE)
	    lpItem->iSelectedImage = nm.item.iSelectedImage;
	// callback may have redirected pszText to point into its own buffer
	if (nm.item.mask & TVIF_TEXT)
	    lpItem->pszText = nm.item.pszText;
        if (nm.item.mask & TVIF_STATE)
            lpItem->state = (nm.item.state & nm.item.stateMask) | (lpItem->state & ~nm.item.stateMask);

        if (nm.item.mask & TVIF_DI_SETITEM) {

            if (nm.item.mask & TVIF_TEXT)
                if (nm.item.pszText && (nm.item.pszText != LPSTR_TEXTCALLBACK)) {
                    Assert(hItem->lpstr == LPSTR_TEXTCALLBACK);
                    hItem->lpstr = NULL;
                    Str_Set(&hItem->lpstr, nm.item.pszText);
                }
            if (nm.item.mask & TVIF_STATE)
                hItem->state = lpItem->state;
            if (nm.item.mask & TVIF_IMAGE)
                hItem->iImage = lpItem->iImage;
            if (nm.item.mask & TVIF_SELECTEDIMAGE)
                hItem->iSelectedImage = lpItem->iSelectedImage;
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

void NEAR TV_DrawItem(PTREE pTree, HTREEITEM hItem, HDC hdc, int x, int y, UINT flags)
{
    int iLevel = hItem->iLevel;
    UINT cxIndent = pTree->cxIndent;
    COLORREF rgbOldBack, rgbOldText;
    RECT rc;
    int iBack, iText;
    LPSTR lpstr;
    int cch;
    UINT etoFlags = ETO_OPAQUE;
    TV_ITEM ti;
    char szTemp[MAX_PATH];
    int iState = 0;
    HFONT hFont;			//$BOLD

    rc.top = y;
    rc.bottom = rc.top + pTree->cyItem;

    if (flags & TVDI_ERASE) {
	// Opaque the whole item
	rc.left = 0;
	rc.right = pTree->cxWnd;
	FillRect(hdc, &rc, pTree->hbrBk);
    }


    // make sure the callbacks don't invalidate this item
    pTree->hItemPainting = hItem;	

    ti.pszText = szTemp;
    ti.cchTextMax  = sizeof(szTemp);
    ti.stateMask = TVIS_OVERLAYMASK | TVIS_CUT | TVIS_BOLD; //$BOLD
    TV_GetItem(pTree, hItem, TVIF_IMAGE | TVIF_STATE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN, &ti);

    pTree->hItemPainting = NULL;

    if (flags & TVDI_NOTREE)
	iLevel = 0;
    else if ((pTree->style & (TVS_HASLINES | TVS_HASBUTTONS)) &&
              (pTree->style & TVS_LINESATROOT))
	// Make room for the "plus" at the front of the tree
	x += cxIndent;


    x += iLevel * cxIndent;

    // draw image
    if (!(flags & TVDI_NOTREE))
    {

        if (pTree->himlState) {
            iState = TV_StateIndex(&ti);
            if (iState) {
                ImageList_Draw(pTree->himlState, iState, hdc, x,
                               y + (pTree->cyItem - pTree->cyState), ILD_NORMAL);
                x += pTree->cxState;
            }
        }

        if (pTree->hImageList) {
            UINT fStyle = 0;
            COLORREF rgb = CLR_HILIGHT;

            int i = (ti.state & TVIS_SELECTED) ? ti.iSelectedImage : ti.iImage;

            if (ti.state & TVIS_CUT) {
                fStyle |= ILD_BLEND50;
                rgb = ImageList_GetBkColor(pTree->hImageList);
            }

            ImageList_DrawEx(pTree->hImageList, i, hdc,
                           x, y + ((pTree->cyItem - pTree->cyImage) / 2), 0, 0,
                           CLR_DEFAULT, rgb,
                           fStyle | (ti.state & TVIS_OVERLAYMASK));

        }
    }

    if (pTree->hImageList) {
        // even if not drawing image, draw text in right place
        x += pTree->cxImage;
    }

    // draw text
    if ((ti.state & TVIS_DROPHILITED) ||
        (!pTree->hDropTarget && 
         ((ti.state & TVIS_SELECTED) &&
          (pTree->fFocus || (pTree->style & TVS_SHOWSELALWAYS))) &&
         !(flags & TVDI_GRAYCTL))) {
	// selected
        iBack = COLOR_HIGHLIGHT;
        iText = COLOR_HIGHLIGHTTEXT;
    } else {
	// not selected
        iBack = (flags & TVDI_GRAYCTL) ? COLOR_3DFACE : COLOR_WINDOW;
        iText = ((ti.state & TVIS_DISABLED) || (flags & TVDI_GRAYCTL)) ? COLOR_GRAYTEXT : COLOR_WINDOWTEXT;
    }

    if (iBack)
    	rgbOldBack = SetBkColor(hdc, GetSysColor(iBack));
    rgbOldText = SetTextColor(hdc, GetSysColor(iText));

    // if forcing black and transparent, do so.  dc's BkMode should
    // already be set to TRANSPARENT by caller
    if (flags & TVDI_TRANSTEXT)
    {
	SetTextColor(hdc, 0x000000);
	etoFlags = 0;			// don't opaque nothin'
    }

    // Figure out which font to use.	//$BOLD
    if (ti.state & TVIS_BOLD) {		//$BOLD
	hFont = pTree->hFontBold;	//$BOLD
    } else {				//$BOLD
	hFont = pTree->hFont;		//$BOLD
    }					//$BOLD
    hFont = SelectObject(hdc, hFont);	//$BOLD

    lpstr = ti.pszText;
    cch = lstrlen(lpstr);

    if (!hItem->iWidth || (hItem->lpstr == LPSTR_TEXTCALLBACK))
    {
	TV_ComputeItemWidth(pTree, hItem, hdc); //$BOLD
    }

    rc.left = x;
    rc.right = x + hItem->iWidth;

    // Draw the text, unless it's the one we are editing
    if (pTree->htiEdit != hItem || !IsWindow(pTree->hwndEdit) || !IsWindowVisible(pTree->hwndEdit))
    {
        ExtTextOut(hdc, x + g_cxLabelMargin, y + ((pTree->cyItem - pTree->cyText) / 2) + g_cyBorder,
                   etoFlags, &rc, lpstr, cch, NULL);

        // Draw the focus rect, if appropriate.
        if (pTree->fFocus && (hItem == pTree->hCaret) && !(flags & (TVDI_TRANSTEXT | TVDI_GRAYCTL)))
            DrawFocusRect(hdc, &rc);
    }

    if (iBack)
	SetBkColor(hdc, rgbOldBack);
    SetTextColor(hdc, rgbOldText);

    // Restore the original font.	//$BOLD
    SelectObject(hdc, hFont);		//$BOLD

    // Notice that we should have opaque'd the rest of the line above if no tree
    if (!(flags & TVDI_NOTREE))
    {
	if (pTree->hImageList)
	    x -= pTree->cxImage;

        if (iState)
            x -= pTree->cxState;

        if (pTree->style & TVS_HASLINES)
	{
            int i;

            x -= cxIndent;
            if (iLevel-- || (pTree->style & TVS_LINESATROOT))
            {
		// HACK: Special case the first root
		// We will draw a "last" sibling button upside down
		if (iLevel == -1 && hItem == hItem->hParent->hKids)
		{
                    if (hItem->hNext) {
                        i = 2;
                        if (ti.cChildren && (pTree->style & TVS_HASBUTTONS))
                        {
                            i += 2;
                            if (ti.state & TVIS_EXPANDED)
                                i += 2;
                        }

                        StretchBlt(hdc, x, y + pTree->cyItem, cxIndent, -pTree->cyItem, pTree->hdcBits, i * cxIndent, 0, cxIndent, pTree->cyItem, SRCCOPY);
                        i = -1;
                    }
                    else
                    {
                        // first root no siblings
                        // if there's no other item, draw just the button if button mode,
                        if (pTree->style & TVS_HASBUTTONS)
                        {
                            if (ti.cChildren) {
                                // hasbuttons, has lines, lines at root
                                i = (ti.state & TVIS_EXPANDED) ? 9 : 8;
                            } else {
                                i = 7;
                            }
                        }
                        else
                        {
                            i = 3;
                        }
                    }
                }
		else
		{
		    i = (hItem->hNext) ? 1 : 2;
		    if (ti.cChildren && (pTree->style & TVS_HASBUTTONS))
		    {
			i += 2;
			if (ti.state & TVIS_EXPANDED)
			    i += 2;
		    }

		}

                if (i != -1)
                    BitBlt(hdc, x, y, cxIndent, pTree->cyItem, pTree->hdcBits, i * cxIndent, 0, SRCCOPY);

		while ((--iLevel >= 0) || ((pTree->style & TVS_LINESATROOT) && iLevel >= -1))
		{
		    hItem = hItem->hParent;
		    x -= cxIndent;
		    if (hItem->hNext)
			BitBlt(hdc, x, y, cxIndent, pTree->cyItem, pTree->hdcBits, 0, 0, SRCCOPY);
		}
	    }
	}
	else
	{
            if ((pTree->style & TVS_HASBUTTONS) && (iLevel || pTree->style & TVS_LINESATROOT)
                && ti.cChildren)
            {
		int i = (ti.state & TVIS_EXPANDED) ? cxIndent : 0;

		x -= cxIndent;
		BitBlt(hdc, x, y, cxIndent, pTree->cyItem, pTree->hdcBits, i, 0, SRCCOPY);
	    }
	}
    }
}


void NEAR TV_DrawTree(PTREE pTree, HDC hdc, BOOL fErase, LPRECT lprc)
{
    int x, y;
    int iStart, iCnt;
    UINT uFlags;
    RECT rc;

    if (!pTree->fRedraw)
        return;


    x = -pTree->xPos;

    TV_GetBackgroundBrush(pTree, hdc);

    rc = *lprc;

    if ((pTree->cItems == 0) || (!pTree->hTop))
	goto ClearAndReturn;

    iStart = lprc->top / pTree->cyItem;
    y = iStart * pTree->cyItem;

    Assert(ITEM_VISIBLE(pTree->hTop));

    iCnt = pTree->cShowing - pTree->hTop->iShownIndex;

    if (iStart < iCnt)
    {
        HTREEITEM   hItem;
        HFONT       hOldFont;
        int         cVisible = min(iCnt, ((lprc->bottom / pTree->cyItem) + 1)) - iStart;

        for (hItem = pTree->hTop; hItem && iStart; iStart--)
            hItem = TV_GetNextVisItem(hItem);

        hOldFont = pTree->hFont ? SelectObject(hdc, pTree->hFont) : NULL;

	// TVDI_* for all items
	uFlags = (pTree->style & WS_DISABLED) ? TVDI_GRAYCTL : 0;
	if (fErase)
	    uFlags |= TVDI_ERASE;

	// BUGBUG: I've seen this code fault, getting NULL back from
	// TV_GetNextVisItem()

        // loop from the first visible item until either all visible items are
        // drawn or there are no more items to draw
        while (cVisible)
        {
            TV_DrawItem(pTree, hItem, hdc, x, y, uFlags);

	    y += pTree->cyItem;

            // if there is still room for more visible items, figure out the next
            // item to be drawn
            if (--cVisible)
                hItem = TV_GetNextVisItem(hItem);
	}

        if (hOldFont)
            SelectObject(hdc, hOldFont);

	rc.top = y;
    }

ClearAndReturn:
    if (fErase)
	// Opaque out everything we have not drawn explicitly
	FillRect(hdc, &rc, pTree->hbrBk);
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
	BeginPaint(pTree->hwnd, &ps);
        TV_DrawTree(pTree, ps.hdc, ps.fErase, &ps.rcPaint);
        EndPaint(pTree->hwnd, &ps);
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
    int dx, dy;
    int iSrc;

    TV_ITEM ti;

    // BUGBUG??? we know it's already been drawn, so is iWidth valid???
    dx = hItem->iWidth + pTree->cxImage;
    dy = pTree->cyItem;

    if (!(hdcMem = CreateCompatibleDC(NULL)))
	goto CDI_Exit;
    if (!(hbmImage = CreateColorBitmap(dx, dy)))
	goto CDI_Exit;
    if (!(hbmMask = CreateMonoBitmap(dx, dy)))
	goto CDI_Exit;

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
    SelectObject(hdcMem, hbmMask);
    // fill mask with white for transparency
    PatBlt(hdcMem, 0, 0, dx, dy, WHITENESS);
    TV_DrawItem(pTree, hItem, hdcMem, 0, 0,
    		TVDI_NOIMAGE | TVDI_NOTREE | TVDI_TRANSTEXT);

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
    					pTree->hImageList, iSrc, hItem->state & TVIS_OVERLAYMASK);

CDI_Exit:
    if (hdcMem)
	DeleteObject(hdcMem);
    if (hbmImage)
	DeleteObject(hbmImage);
    if (hbmMask)
	DeleteObject(hbmMask);

    return himl;
}
