#include "ctlspriv.h"
#include "rebar.h"
#include "image.h"

extern HPALETTE CreateHalftonePalette(HDC hdc);
extern BOOL WINAPI ImageList_DrawIndirect(IMAGELISTDRAWPARAMS FAR* pimldp);
#define RB_GRABWIDTH 8

#ifdef WIN32
#define SEND_WM_COMMAND(hwnd, id, hwndCtl, codeNotify) \
    (void)SendMessage((hwnd), WM_COMMAND, MAKEWPARAM((UINT)(id),(UINT)(codeNotify)), (LPARAM)(HWND)(hwndCtl))
#else
    // dont cast result to void since we depend on this hack to get a handle back
    // from some WM_COMMAND messages
#define SEND_WM_COMMAND(hwnd, id, hwndCtl, codeNotify) \
    SendMessage((hwnd), WM_COMMAND, (WPARAM)(int)(id), MAKELPARAM((UINT)(hwndCtl), (codeNotify)))
#endif


// ----------------------------------------------------------------------------
//
// RBSendNotify
//
// sends a notification to parent
//
// ----------------------------------------------------------------------------
void NEAR PASCAL RBSendNotify(PRB prb, int code)
{
#if defined(WIN32) || defined(IEWIN31_25)
    NMHDR   hdr;
    SendNotifyEx(prb->ci.hwndParent, prb->ci.hwnd, code, &hdr, prb->ci.bUnicode);
#else
    SEND_WM_COMMAND(prb->ci.hwndParent, GetWindowID(prb->ci.hwnd), prb->ci.hwnd, code);
#endif
}

// ----------------------------------------------------------------------------
//
// RBCanBandMove
//
// returns TRUE if the given band can be moved and FALSE if it cannot
//
// ----------------------------------------------------------------------------
BOOL NEAR PASCAL RBCanBandMove(PRB prb, PRBB prbb)
{
    if ((prb->ci.style & RBS_FIXEDORDER) && (prbb == prb->rbbList))
        // the first band in fixed order rebars can't be moved
        return(FALSE);

    // fixed size bands can't be moved
    return(!(prbb->fStyle & RBBS_FIXEDSIZE));
}

// ----------------------------------------------------------------------------
//
// RBBCalcMinWidth
//
// calculates minimum width for the given band
//
// ----------------------------------------------------------------------------
void NEAR PASCAL RBBCalcMinWidth(PRB prb, PRBB prbb)
{
    BOOL fMovable = RBCanBandMove(prb, prbb);
    BOOL fVertical;
    int  cEdge;
    BOOL fEmpty = ((prbb->iImage == -1) && !prbb->lpText);

    prbb->cxMin = prbb->cxMinChild;

    if (!fMovable && fEmpty)
        return;

    fVertical = (BOOL)(prb->ci.style & CCS_VERT);
    cEdge = fVertical ? g_cyEdge : g_cxEdge;

    prbb->cxMin += 2 * cEdge;

    if (fMovable)
    {
        prbb->cxMin += RB_GRABWIDTH * (fVertical ? g_cyBorder : g_cxBorder);
        if (fEmpty)
            return;
    }

    prbb->cxMin += 2 * cEdge;

    if (prbb->iImage != -1)
        prbb->cxMin += (fVertical ? prb->cyImage : prb->cxImage);

    if (prbb->lpText)
    {
        if (fVertical)
            prbb->cxMin += prb->cyFont;
        else
            prbb->cxMin += prbb->cxText;
        if (prbb->iImage != -1)
            // has both image and text -- add in edge between 'em
            prbb->cxMin += cEdge;
    }
}

// ----------------------------------------------------------------------------
//
// RBBCalcTextExtent
//
// computes the horizontal extent of the given band's title text in the current
// title font for the rebar
//
// returns TRUE if the extent changed; FALSE if it remains the same
//
// ----------------------------------------------------------------------------
BOOL NEAR PASCAL RBBCalcTextExtent(PRB prb, PRBB prbb, HDC hdcIn)
{
    HDC     hdc = hdcIn;
    HFONT   hFontOld;
    SIZE    size;

    if (!prbb->lpText)
        return(FALSE);

    if (!hdcIn && !(hdc = GetDC(prb->ci.hwnd)))
        return(FALSE);

    hFontOld = SelectObject(hdc, prb->hFont);
    GetTextExtentPoint(hdc, prbb->lpText, lstrlen(prbb->lpText), &size);
    SelectObject(hdc, hFontOld);

    if (!hdcIn)
        ReleaseDC(prb->ci.hwnd, hdc);

    if (prbb->cxText == (UINT) size.cx)
        return(FALSE);

    prbb->cxText = size.cx;

    RBBCalcMinWidth(prb, prbb);
    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// RBBGetHeight
//
// returns minimum height for the given band
// TODO: make this a field in the band structure instead of always calling this
//
// ----------------------------------------------------------------------------
UINT NEAR PASCAL RBBGetHeight(PRB prb, PRBB prbb)
{
    UINT cy = 0;
    BOOL fVertical = (BOOL)(prb->ci.style & CCS_VERT);
    UINT cyCheck, cyBorder;

    cyBorder = (fVertical ? g_cxEdge : g_cyEdge) * 2;

    if (prbb->hwndChild)
    {
        cy = prbb->cyMinChild;
        if (!(prbb->fStyle & RBBS_CHILDEDGE))
            // add edge to top and bottom of child window
            cy -= cyBorder;
    }

    if (prbb->lpText)
    {
        cyCheck = (fVertical) ? prbb->cxText : prb->cyFont;

        if (cyCheck > cy)
            cy = cyCheck;
    }

    if (prbb->iImage != -1)
    {
        cyCheck = (fVertical) ? prb->cxImage : prb->cyImage;

        if (cyCheck > cy)
            cy = cyCheck;
    }

    return(cy + cyBorder);
}

// ----------------------------------------------------------------------------
//
// RBGetRowCount
//
// returns the number of rows in the rebar's current configuration
//
// ----------------------------------------------------------------------------
UINT NEAR PASCAL RBGetRowCount(PRB prb)
{
    UINT i;
    UINT cRows = 0;

    for (i = 0; i < prb->cBands; i++)
        if (!prb->rbbList[i].x)
            cRows++;

    return(cRows);
}

// ----------------------------------------------------------------------------
//
// RBGetLineHeight
//
// returns the height of the line of bands from iStart to iEnd, inclusively
//
// ----------------------------------------------------------------------------
UINT NEAR PASCAL RBGetLineHeight(PRB prb, UINT iStart, UINT iEnd)
{
    UINT cy = 0;
    PRBB prbb;
    UINT cyBand;

    if (!(prb->ci.style & RBS_VARHEIGHT))
    {
        // for fixed height bars, line height is maximum height of ALL bands
        iStart = 0;
        iEnd = prb->cBands - 1;
    }

    for (prbb = prb->rbbList + iStart; iStart <= iEnd; prbb++, iStart++)
    {
        cyBand = RBBGetHeight(prb, prbb);
        cy = max(cy, cyBand);
    }

    return(cy);
}

// ----------------------------------------------------------------------------
//
// RBResizeChildren
//
// resizes children to fit properly in their respective bands' bounding rects
//
// ----------------------------------------------------------------------------
void NEAR PASCAL RBResizeChildren(PRB prb, PRBB prbb, PRBB prbbEnd)
{
    int     cx, cy, x, y, cxHeading;
    HDWP    hdwp;
    BOOL    fVertical = (BOOL)(prb->ci.style & CCS_VERT);

    if (!prb->cBands)
        return;

    hdwp = BeginDeferWindowPos(prb->cBands);

    if (!prbb)
        prbb = prb->rbbList;

    if (!prbbEnd)
        prbbEnd = prb->rbbList + (prb->cBands - 1);

    for ( ; prbb <= prbbEnd ; prbb++)
    {
        if (!prbb->hwndChild)
            continue;

        cxHeading = prbb->cxMin - prbb->cxMinChild;
        x = prbb->x + cxHeading;

        cx = prbb->cx - cxHeading;
        if (!(prbb->fStyle & RBBS_FIXEDSIZE))
            cx -= (fVertical ? g_cyEdge : g_cxEdge) * 2;

        if (cx < 0)
            cx = 0;
        y = prbb->y;
        cy = prbb->cy;
        if (prbb->cyMinChild && (prbb->cyMinChild < (UINT) cy))
        {
            y += (cy - prbb->cyMinChild) / 2;
            cy = prbb->cyMinChild;
        }

#ifdef IEWIN31_25
        //
        // HACK HACK HACK:
        //
        // Windows 3.1 and windows 95 behave very differently when you resize a
        // combobox! On Windows 95, the drop-down list size is determined on creation,
        // but on win 3.1, if you get the window rect and then set it using the same
        // dimensions, the dropdown list is hacked off!  So we need to treat combo-box
        // children differently!
        //
        if (cx > 0)
        {
            char szClass[20];
            GetClassName(prbb->hwndChild, szClass, sizeof(szClass)-1);
            if (StrCmpNI(szClass, "combobox", sizeof("combobox")) == 0)
            {
                // HACK: Don't allow the combobox width to be less than the scroll
                // button (plus 1 for border) it bleeds to the left of its extent!
                if (cx <= (GetSystemMetrics(SM_CXVSCROLL) + 1))
                {
                    cx = 0;
                }
                else
                {
                    // We have a combo box, so make the height include the drop-down
                    // list box
                    RECT rcDrop;
                    SendMessage(prbb->hwndChild, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&rcDrop);
                    cy = rcDrop.bottom - rcDrop.top;
                }
            }
        }
#endif  //IEWIN31_25

        if(fVertical)
            DeferWindowPos(hdwp, prbb->hwndChild, NULL, y, x, cy, cx, SWP_NOZORDER);
        else
            DeferWindowPos(hdwp, prbb->hwndChild, NULL, x, y, cx, cy, SWP_NOZORDER);

    }

    EndDeferWindowPos(hdwp);
}

// ----------------------------------------------------------------------------
//
// RBMoveBand
//
// moves the band from one position to another in the rebar's band array,
// updating the rebar's iCapture field as needed
//
// ----------------------------------------------------------------------------
BOOL NEAR PASCAL RBMoveBand(PRB prb, UINT iFrom, UINT iTo)
{
    RBB rbbMove;
    int iShift;
    BOOL fCaptureChanged = (prb->iCapture == -1);

    if (iFrom != iTo)
    {
        rbbMove = prb->rbbList[iFrom];
        if (prb->iCapture == (int) iFrom)
        {
            prb->iCapture = (int) iTo;
            fCaptureChanged = TRUE;
        }

        iShift = (iFrom > iTo) ? -1 : 1;

        while (iFrom != iTo)
        {
            if (!fCaptureChanged && (prb->iCapture == (int) (iFrom + iShift)))
            {
                prb->iCapture = (int) iFrom;
                fCaptureChanged = TRUE;
            }

            prb->rbbList[iFrom] = prb->rbbList[iFrom + iShift];
            iFrom += iShift;
        }
        prb->rbbList[iTo] = rbbMove;
    }
    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// RBRecalc
//
// recomputes bounding rects for all bands in given rebar
//
// ----------------------------------------------------------------------------
UINT NEAR PASCAL RBRecalc(PRB prb)
{
    PRBB    prbb = prb->rbbList;
    PRBB    prbbWalk;
    UINT    cxRow;
    UINT    cxMin;
    UINT    i;
    UINT    j;
    UINT    k;
    UINT    iFixed = 0xFFFF;
    int     cy;
    int     y;
    int     x;
    UINT    cxBar;
    RECT    rc;
    HWND    hwndSize;
    BOOL    fNewLine = FALSE;
    BOOL    fChanged;
    BOOL    fVertical = (BOOL)(prb->ci.style & CCS_VERT);
    BOOL    fBandBorders;

    if (!prb->cBands)
        return(0);

    if ((prb->ci.style & CCS_NORESIZE) || (prb->ci.style & CCS_NOPARENTALIGN))
        // size based on rebar window itself
        hwndSize = prb->ci.hwnd;
    else if (!(hwndSize = GetParent(prb->ci.hwnd)))
        // size based on parent window -- if no parent window, bail now
        return(0);

    GetClientRect(hwndSize, &rc);

    cxBar = (UINT) (fVertical ? (rc.bottom - rc.top) : (rc.right - rc.left));
    rc.right = cxBar;

    fBandBorders = (BOOL)(prb->ci.style & RBS_BANDBORDERS);

    for (i = 0, prbbWalk = prbb; i < prb->cBands; i++, prbbWalk++)
        prbbWalk->cx = prbbWalk->cxRequest;

    y = 0;
    i = 0;

    // Main Loop -- loop until all bands are calculated
    while (i < prb->cBands)
    {
        if (fBandBorders && (y > 0))
            y += g_cyEdge;

ReLoop:
        cxRow = 0;
        cxMin = 0;

        x = 0;

        // Row Loop -- loop until hard line break is found or soft line break
        // is necessary
        for (j = i, prbbWalk = prbb; j < prb->cBands; j++, prbbWalk++)
        {
            if (j > i)
            {
                // not the first band in the row -- check for break style
                if (prbbWalk->fStyle & RBBS_BREAK)
                    break;

                if (fBandBorders)
                    // add in space for vertical etch on palettized display
                    cxMin += g_cxEdge;
            }

            if (prbbWalk->fStyle & RBBS_FIXEDSIZE)
            {
                // remember location of branding brick
                iFixed = j;
                goto UseMin;
            }

            if (prbbWalk->cx < prbbWalk->cxMin)
            {
UseMin:
                prbbWalk->cx = prbbWalk->cxMin;
            }

            cxMin += prbbWalk->cxMin; // update running total of min widths

            // read the assert comment below
            if (j > i)
            {
                // not the first band in row -- check for need to autobreak
                if (cxMin > cxBar)
                    // autobreak here
                    break;

                if (fBandBorders)
                    // add in space for vertical etch on palettized display
                    cxRow += g_cxEdge;
            }

            cxRow += prbbWalk->cx; // update running total of current widths
        }

        if (!i)
        {
            // first row -- handle proper placement of branding band
            if (iFixed == 0xFFFF)
            {
                // branding band not yet found; look in the remaining bands
                k = j;
                while (j < prb->cBands)
                {
                    if (prb->rbbList[j].fStyle & RBBS_FIXEDSIZE)
                    {
                        // branding band found; move to 1st row and recompute
                        RBMoveBand(prb, j, k);
                        goto ReLoop;
                    }
                    j++;
                }
                // no branding band found -- reset j and continue on
                j = k;
            }
            else
                // we have a branding band; move it to
                // the rightmost position in the row
                RBMoveBand(prb, iFixed, j - 1);
        }

        // variant:
        // now the current row of bands is from i to j - 1

        // assert that j != i because then the above variant won't be true
        Assert(j != i);

        if (cxRow > cxBar)
        {
            // bands are too long -- shrink bands from right to left
            for (k = i; k < j; k++)
            {
                prbbWalk--;
                if (prbbWalk->cx > prbbWalk->cxMin)
                {
                    cxRow -= prbbWalk->cx - prbbWalk->cxMin;
                    prbbWalk->cx = prbbWalk->cxMin;
                    if (cxRow <= cxBar)
                    {
                        prbbWalk->cx += cxBar - cxRow;
                        break;
                    }
                }
            }
        }
        else if (cxRow < cxBar)
        {
            // bands are too short -- grow rightmost non-minimized band
            for (k = j - 1; k >= i; k--)
            {
                prbbWalk--;
                if ((k == i) || (prbbWalk->cx > prbbWalk->cxMin))
                {
                    // the k == i check means we've made it to the first
                    // band on this row and so he has to get the cx change
                    prbbWalk->cx += cxBar - cxRow;
                    break;
                }
            }
        }

        // items from index i to index j-1 (inclusive) WILL fit on one line
        cy = RBGetLineHeight(prb, i, j - 1);

        fChanged = FALSE; // set if any bands on current row changed position

        while (i < j)
        {
            // go through row of bands, updating positions and heights,
            // invalidating as needed
            if ((prbb->y != y) || (prbb->x != x) || (prbb->cy != cy))
            {
                fChanged = TRUE;
                rc.left = min(prbb->x, x);
                rc.top = min(prbb->y, y);
                rc.bottom = max(prbb->y + prbb->cy, y + cy);
                if (fBandBorders)
                {
                    // acount for etch line that will need to move
                    rc.left -= g_cxEdge;
                    rc.bottom += g_cyEdge/2;
                }
                if (prb->ci.style & CCS_VERT)
                {
                    UINT uiTemp;
                    uiTemp = rc.left; rc.left = rc.top; rc.top = uiTemp;
                    uiTemp = rc.bottom; rc.bottom = rc.right; rc.right = uiTemp;
                }
                InvalidateRect(prb->ci.hwnd, &rc, TRUE);
            }

            prbb->x = x;
            prbb->y = y;
            prbb->cy = cy;

            x += prbb->cx;
            if (fBandBorders)
                x += g_cxEdge;

            prbb++;
            i++;
        }

        // i and prbb now refer to the first band in the next row of bands
        y += cy;
    }

    prb->cy = y;
    return(y);
}

// ----------------------------------------------------------------------------
//
// RBResize
//
// recomputes bounding rects for all bands and then resizes rebar and children
// based on these rects
//
// ----------------------------------------------------------------------------
void NEAR PASCAL RBResize(PRB prb, BOOL fForceHeightChange)
{
    HWND hwndParent;
    RECT rc;
    UINT cy;

    if (!prb || !(hwndParent = GetParent(prb->ci.hwnd)))
        return;

    cy = prb->cy;
    RBRecalc(prb);

    GetWindowRect(prb->ci.hwnd, &rc);
    MapWindowPoints(HWND_DESKTOP, hwndParent, (LPPOINT)&rc, 2);
    RBResizeChildren(prb, NULL, NULL);
    NewSize(prb->ci.hwnd, prb->cy, prb->ci.style, rc.left, rc.top, rc.right, rc.bottom);
    if ((cy != prb->cy) || fForceHeightChange)
        RBSendNotify(prb, RBN_HEIGHTCHANGE);
}

// ----------------------------------------------------------------------------
//
// RBGetBarHeight
//
// returns total height of given rebar, recalculating heights if necessary
//
// ----------------------------------------------------------------------------
UINT NEAR PASCAL RBGetBarHeight(PRB prb)
{
    return((prb->cBands && !prb->cy) ? RBRecalc(prb) : prb->cy);
}

// ----------------------------------------------------------------------------
//
// RBSetFont
//
// sets the rebar band title font to the current system-wide caption font
//
// ----------------------------------------------------------------------------
BOOL NEAR PASCAL RBSetFont(PRB prb, WPARAM wParam)
{
    HFONT hOldFont;
#ifdef IEWIN31_25
    LOGFONT lf;
#else
    NONCLIENTMETRICS ncm;
#endif
    TEXTMETRIC tm;
    HDC hdc;
    BOOL fChange = FALSE;
    UINT        i;

    if ((wParam != 0) && (wParam != SPI_SETNONCLIENTMETRICS))
        return(FALSE);

#ifdef WIN32
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0))
        return(FALSE);
#else
#ifdef IEWIN31_25
    if (!SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE))
        return(FALSE);
#else
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, (UINT)ncm.cbSize, &ncm, 0))
        return(FALSE);
#endif
#endif

    hOldFont = prb->hFont;

#ifdef IEWIN31_25
    lf.lfWeight = FW_NORMAL;
    if (!(prb->hFont = CreateFontIndirect(&lf)))
#else
    ncm.lfCaptionFont.lfWeight = FW_NORMAL;
    if (!(prb->hFont = CreateFontIndirect(&ncm.lfCaptionFont)))
#endif
    {
        prb->hFont = hOldFont;
        return(FALSE);
    }

    if (hOldFont)
        DeleteObject(hOldFont);

    hdc = GetDC(prb->ci.hwnd);
    if (!hdc)
        return(FALSE);

    hOldFont = SelectObject(hdc, prb->hFont);
    GetTextMetrics(hdc, &tm);

    if (prb->cyFont != (UINT) tm.tmHeight)
    {
        prb->cyFont = tm.tmHeight;
        fChange = TRUE;
    }

    // adjust bands
    for (i = 0; i < prb->cBands; i++)
        fChange |= RBBCalcTextExtent(prb, prb->rbbList + i, hdc);

    SelectObject(hdc, hOldFont);
    ReleaseDC(prb->ci.hwnd, hdc);

    if (fChange)
        RBResize(prb, FALSE);
}

// ----------------------------------------------------------------------------
//
// RBDrawBand
//
// draws the title icon and title text of the given band into the given DC
//
// ----------------------------------------------------------------------------
void NEAR PASCAL RBDrawBand(PRB prb, PRBB prbb, HDC hdc)
{
    IMAGELISTDRAWPARAMS imldp;
    int                 xStart, yCenter;
    COLORREF            clrBackSave, clrForeSave;
    int                 iModeSave;
    BOOL                fVertical = (BOOL)(prb->ci.style & CCS_VERT);
    UINT                yStart;
    NMCUSTOMDRAW        nmcd;
    DWORD               dwRet;

    clrForeSave = SetTextColor(hdc, g_clrBtnText);
    clrBackSave = SetBkColor(hdc, g_clrBtnFace);
    if (prbb->hbmBack)
        iModeSave = SetBkMode(hdc, TRANSPARENT);

    nmcd.hdc = hdc;
    nmcd.dwItemSpec = prbb->wID;
    nmcd.uItemState = 0;
    nmcd.lItemlParam = 0;
    dwRet = CICustomDrawNotify(&prb->ci, CDDS_ITEMPREPAINT, &nmcd);

    if (!(dwRet & CDRF_SKIPDEFAULT))
    {
        yCenter = prbb->y + (prbb->cy / 2);

        xStart = prbb->x;
        if (RBCanBandMove(prb, prbb))
        {
            RECT rc;
            int  c;

            if (fVertical)
            {
                c = 3 * g_cyBorder;
                xStart += 2 * g_cyBorder;

                SetRect(&rc, prbb->y + g_cxEdge, xStart, prbb->y + prbb->cy - g_cxEdge, xStart + c);
                DrawEdge(hdc, &rc, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE);
                rc.top += c;
                rc.bottom += c;
                DrawEdge(hdc, &rc, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE);
                xStart += 2 * c;
            }
            else
            {
                c = 3 * g_cxBorder;
                xStart += 2 * g_cxBorder;

                SetRect(&rc, xStart, prbb->y + g_cyEdge, xStart + c, prbb->y + prbb->cy - g_cyEdge);
                DrawEdge(hdc, &rc, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE);
                rc.left += c;
                rc.right += c;
                DrawEdge(hdc, &rc, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE);
                xStart += 2 * c;
            }
        }
        xStart += 2 * (fVertical ? g_cyEdge : g_cxEdge);

        if (prbb->iImage != -1)
        {
            yStart = yCenter - ((fVertical ? prb->cxImage : prb->cyImage) / 2);
            imldp.cbSize = sizeof(imldp);
            imldp.himl   = prb->himl;
            imldp.i      = prbb->iImage;
            imldp.hdcDst = hdc;
            imldp.x      = (fVertical ? yStart : xStart);
            imldp.y      = (fVertical ? xStart : yStart);
            imldp.cx     = 0;
            imldp.cy     = 0;
            imldp.xBitmap= 0;
            imldp.yBitmap= 0;
            imldp.rgbBk  = CLR_DEFAULT;
            imldp.rgbFg  = CLR_DEFAULT;
            imldp.fStyle = ILD_TRANSPARENT;

            ImageList_DrawIndirect(&imldp);
            xStart +=  (fVertical ? (prb->cyImage + g_cyEdge) : (prb->cxImage + g_cxEdge));
        }

        if (prbb->lpText)
        {
            HFONT hFontSave = SelectObject(hdc, prb->hFont);
            if(fVertical)
                TextOut(hdc, yCenter - (prbb->cxText / 2), xStart,
                    prbb->lpText, lstrlen(prbb->lpText));
            else
                TextOut(hdc, xStart, yCenter - (prb->cyFont / 2),
                    prbb->lpText, lstrlen(prbb->lpText));

            SelectObject(hdc, hFontSave);
        }
    }

    if (dwRet & CDRF_NOTIFYPOSTPAINT)
        CICustomDrawNotify(&prb->ci, CDDS_ITEMPOSTPAINT, &nmcd);

    if (prbb->hbmBack)
        SetBkMode(hdc, iModeSave);
    SetTextColor(hdc, clrForeSave);
    SetBkColor(hdc, clrBackSave);

}

// ----------------------------------------------------------------------------
//
// RBPaint
//
// processes WM_PAINT message
//
// ----------------------------------------------------------------------------
void NEAR PASCAL RBPaint(PRB prb, HDC hdcIn)
{
    HDC         hdc = hdcIn;
    PAINTSTRUCT ps;
    UINT        i;
    NMCUSTOMDRAW    nmcd;

    if (!hdcIn)
        hdc = BeginPaint(prb->ci.hwnd, &ps);

    nmcd.hdc = hdc;
    nmcd.uItemState = 0;
    nmcd.lItemlParam = 0;
    prb->ci.dwCustom = CICustomDrawNotify(&prb->ci, CDDS_PREPAINT, &nmcd);

    if (!(prb->ci.dwCustom & CDRF_SKIPDEFAULT))
    {
        for (i = 0; i < prb->cBands; i++)
            RBDrawBand(prb, prb->rbbList + i, hdc);
    }

    if (prb->ci.dwCustom & CDRF_NOTIFYPOSTPAINT)
        CICustomDrawNotify(&prb->ci, CDDS_POSTPAINT, &nmcd);

    if (!hdcIn)
        EndPaint(prb->ci.hwnd, &ps);
}

// ----------------------------------------------------------------------------
//
// RBTileBlt
//
// Fills the given rectangle with the rebar's background bitmap, tiling if
// necessary
//
// ----------------------------------------------------------------------------
void NEAR PASCAL RBTileBlt(PRB prb, PRBB prbb, UINT x, UINT y, UINT cx, UINT cy, HDC hdcDst, HDC hdcSrc)
{
    UINT xOff = 0;
    UINT yOff = 0;
    BOOL fxTile, fyTile;
    int cxPart, cyPart;

    if (!(prbb->fStyle & RBBS_FIXEDBMP))
    {
        if (prb->ci.style & CCS_VERT)
        {
            xOff = -prbb->y;
            yOff = -prbb->x;
        }
        else
        {
            xOff = -prbb->x;
            yOff = -prbb->y;
        }
    }

    xOff += x;
    if (xOff >= prbb->cxBmp)
        xOff %= prbb->cxBmp;

    yOff += y;
    if (yOff >= prbb->cyBmp)
        yOff %= prbb->cyBmp;

ReCheck:
    fxTile = ((xOff + cx) > prbb->cxBmp);
    fyTile = ((yOff + cy) > prbb->cyBmp);

    if (!fxTile && !fyTile)
    {
        // no tiling needed -- blt and leave
        BitBlt(hdcDst, x, y, cx, cy, hdcSrc, xOff, yOff, SRCCOPY);
        return;
    }

    if (!fxTile)
    {
        // vertically tile
        cyPart = prbb->cyBmp - yOff;
        BitBlt(hdcDst, x, y, cx, cyPart, hdcSrc, xOff, yOff, SRCCOPY);
        y += cyPart;
        cy -= cyPart;
        yOff = 0;
        goto ReCheck;
    }

    if (!fyTile)
    {
        // horizontally tile
        cxPart = prbb->cxBmp - xOff;
        BitBlt(hdcDst, x, y, cxPart, cy, hdcSrc, xOff, yOff, SRCCOPY);
        x += cxPart;
        cx -= cxPart;
        xOff = 0;
        goto ReCheck;
    }

    // tile both ways
    cyPart = prbb->cyBmp - yOff;
    RBTileBlt(prb, prbb, x, y, cx, cyPart, hdcDst, hdcSrc);
    y += cyPart;
    cy -= cyPart;
    yOff = 0;
    goto ReCheck;
}

// ----------------------------------------------------------------------------
//
// RBHitTest
//
// returns the index to the band that the given point lies in, or -1 if outside
// of all bands.  Also, sets flags to indicate which part of the band the
// point lies in.
//
// ----------------------------------------------------------------------------
int FAR PASCAL RBHitTest(PRB prb, LPRBHITTESTINFO prbht)
{
    int i;
    PRBB prbb = prb->rbbList;
    POINT pt;
    BOOL fVert = (BOOL)(prb->ci.style & CCS_VERT);
    int  cx;

    if (fVert)
    {
        pt.x = prbht->pt.y;
        pt.y = prbht->pt.x;
    }
    else
        pt = prbht->pt;

    for (i = 0; i < (int) prb->cBands; i++)
    {
        if ((pt.x >= prbb->x) && (pt.y >= prbb->y) &&
            (pt.x <= (prbb->x + prbb->cx)) && (pt.y <= (prbb->y + prbb->cy)))
        {
            cx = prbb->cxMin - prbb->cxMinChild;
            if (pt.x <= (int) (prbb->x + cx))
            {
                cx = RB_GRABWIDTH * (fVert ? g_cyBorder : g_cxBorder);
                if (RBCanBandMove(prb, prb->rbbList + i) &&
                    (pt.x <= (int) (prbb->x + cx)))
                    prbht->flags = RBHT_GRABBER;
                else
                    prbht->flags = RBHT_CAPTION;
            }
            else
                prbht->flags = RBHT_CLIENT;

            prbht->iBand = i;
            return(i);
            break;
        }
        prbb++;
    }

    prbht->flags = RBHT_NOWHERE;
    prbht->iBand = -1;
    return(-1);
}


// ----------------------------------------------------------------------------
//
// RBEraseBkgnd
//
// processes WM_ERASEBKGND message by drawing band borders, if necessary, and
// filling in the rebar bands with their background color
//
// ----------------------------------------------------------------------------
BOOL NEAR PASCAL RBEraseBkgnd(PRB prb, HDC hdc, int iBand)
{
    BOOL fVertical = (BOOL)(prb->ci.style & CCS_VERT);
    NMCUSTOMDRAW    nmcd;
    DWORD           dwItemRet;
    BOOL            fBandBorders;
    RECT            rcClient;
    HDC             hdcMem = NULL;
    HPALETTE        hpalSave = NULL;
    UINT            i;
    PRBB            prbb = prb->rbbList;

    nmcd.hdc = hdc;
    nmcd.uItemState = 0;
    nmcd.lItemlParam = 0;
    prb->ci.dwCustom = CICustomDrawNotify(&prb->ci, CDDS_PREERASE, &nmcd);

    if (!(prb->ci.dwCustom & CDRF_SKIPDEFAULT))
    {
        if (fBandBorders = (BOOL)(prb->ci.style & RBS_BANDBORDERS))
            GetClientRect(prb->ci.hwnd, &rcClient);

        for (i = 0; i < prb->cBands; i++, prbb++)
        {
            if (fVertical)
                SetRect(&nmcd.rc, prbb->y, prbb->x, prbb->y + prbb->cy, prbb->x + prbb->cx);
            else
                SetRect(&nmcd.rc, prbb->x, prbb->y, prbb->x + prbb->cx, prbb->y + prbb->cy);

            if (fBandBorders)
            {
                if (prbb->x)
                {
                    // draw etch between bands on same row
                    if (fVertical)
                    {
                        nmcd.rc.right += g_cxEdge / 2;
                        nmcd.rc.top -= g_cyEdge;
                        DrawEdge(hdc, &nmcd.rc, EDGE_ETCHED, BF_TOP);
                        nmcd.rc.right -= g_cxEdge / 2;
                        nmcd.rc.top += g_cyEdge;
                    }
                    else
                    {
                        nmcd.rc.bottom += g_cyEdge / 2;
                        nmcd.rc.left -= g_cxEdge;
                        DrawEdge(hdc, &nmcd.rc, EDGE_ETCHED, BF_LEFT);
                        nmcd.rc.bottom -= g_cyEdge / 2;
                        nmcd.rc.left += g_cxEdge;
                    }
                }
                else
                {
                    // draw etch between rows
                    if (fVertical)
                    {
                        rcClient.right = prbb->y + prbb->cy + g_cxEdge;
                        DrawEdge(hdc, &rcClient, EDGE_ETCHED, BF_RIGHT);
                    }
                    else
                    {
                        rcClient.bottom = prbb->y + prbb->cy + g_cyEdge;
                        DrawEdge(hdc, &rcClient, EDGE_ETCHED, BF_BOTTOM);
                    }
                }
            }

            nmcd.dwItemSpec = prbb->wID;
            nmcd.uItemState = 0;
            dwItemRet = CICustomDrawNotify(&prb->ci, CDDS_ITEMPREERASE, &nmcd);

            if (!(dwItemRet & CDRF_SKIPDEFAULT))
            {
                if (prbb->hbmBack)
                {
                    if (!hdcMem)
                    {
                        if (!(hdcMem = CreateCompatibleDC(hdc)))
                            continue;

                        if (prb->hpal)
                        {
// Hack: For IEWIN31_25, we set the palette in the custom notify.
#ifndef IEWIN31_25

                            hpalSave = SelectPalette(hdc, prb->hpal, TRUE);
                            RealizePalette(hdc);
#endif
                        }
                    }

                    SelectObject(hdcMem, prbb->hbmBack);

                    RBTileBlt(prb, prbb, nmcd.rc.left, nmcd.rc.top, nmcd.rc.right - nmcd.rc.left,
                            nmcd.rc.bottom - nmcd.rc.top, hdc, hdcMem);
                }
                else
                {
                    HBRUSH hbr = CreateSolidBrush(prbb->clrBack);
                    FillRect(hdc, &nmcd.rc, hbr);
                    DeleteObject(hbr);
                }
            }

            if (dwItemRet & CDRF_NOTIFYPOSTERASE)
                CICustomDrawNotify(&prb->ci, CDDS_ITEMPOSTERASE, &nmcd);
        }

        if (hdcMem)
        {
            DeleteDC(hdcMem);
#ifndef IEWIN31_25
            if (prb->hpal)
                SelectPalette(hdc, hpalSave, FALSE);
#endif
        }
    }

    if (prb->ci.dwCustom & CDRF_NOTIFYPOSTERASE)
    {
        nmcd.uItemState = 0;
        nmcd.dwItemSpec = 0;
        nmcd.lItemlParam = 0;
        CICustomDrawNotify(&prb->ci, CDDS_POSTERASE, &nmcd);
    }

    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// RBGetBarInfo
//
// retrieves the indicated values from the rebar's internal structure
//
// ----------------------------------------------------------------------------
BOOL NEAR PASCAL RBGetBarInfo(PRB prb, LPREBARINFO lprbi)
{
    if (!prb || (lprbi->cbSize != sizeof(REBARINFO)))
        return(FALSE);

    if (lprbi->fMask & RBIM_IMAGELIST)
        lprbi->himl = prb->himl;

    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// RBSetBarInfo
//
// sets the indicated values in the rebar's internal structure, recalculating
// and refreshing as needed
//
// ----------------------------------------------------------------------------
BOOL NEAR PASCAL RBSetBarInfo(PRB prb, LPREBARINFO lprbi)
{
    if (!prb || (lprbi->cbSize != sizeof(REBARINFO)))
        return(FALSE);

    if (lprbi->fMask & RBIM_IMAGELIST)
    {
        HIMAGELIST himl = prb->himl;
        UINT    cxOld, cyOld;

        //todo:validate lprbi->himl
        prb->himl = lprbi->himl;
        cxOld = prb->cxImage;
        cyOld = prb->cyImage;
        ImageList_GetIconSize(prb->himl, &prb->cxImage, &prb->cyImage);
        if ((prb->cxImage != cxOld) || (prb->cyImage != cyOld))
        {
            UINT i;

            for (i = 0; i < prb->cBands; i++)
                RBBCalcMinWidth(prb, prb->rbbList + i);

            RBResize(prb, FALSE);
        }
        else
            InvalidateRect(prb->ci.hwnd, NULL, TRUE);
        lprbi->himl = himl;
    }

    return(TRUE);
}

PRBB RBPtrFromID(PRB prb, UINT wID)
{
    UINT i;
    PRBB prbb = prb->rbbList;

    for (i = 0; i < prb->cBands; i++, prbb++)
    {
        if (prbb->wID == wID)
            return(prbb);
    }
    return(NULL);
}

// ----------------------------------------------------------------------------
//
// RBGetBandInfo
//
// retrieves the indicated values from the specified band's internal structure
//
// ----------------------------------------------------------------------------
BOOL NEAR PASCAL RBGetBandInfo(PRB prb, UINT uBand, LPREBARBANDINFO lprbbi)
{
    PRBB prbb;

    if (!prb || (uBand >= prb->cBands) || (lprbbi->cbSize != sizeof(REBARBANDINFO)))
        return(FALSE);

    prbb = prb->rbbList + uBand;

    if (lprbbi->fMask & RBBIM_SIZE)
        lprbbi->cx = prbb->cxRequest;

    if (lprbbi->fMask & RBBIM_STYLE)
        lprbbi->fStyle = prbb->fStyle;

    if (lprbbi->fMask & RBBIM_COLORS)
    {
        lprbbi->clrFore = prbb->clrFore;
        lprbbi->clrBack = prbb->clrBack;
    }

    if (lprbbi->fMask & RBBIM_TEXT)
    {
        UINT cch = prbb->lpText ? lstrlen(prbb->lpText) : 0;

        if (!lprbbi->cch || !lprbbi->lpText || (lprbbi->cch <= cch))
            lprbbi->cch = cch + 1;
        else if (prbb->lpText)
            lstrcpy(lprbbi->lpText, prbb->lpText);
        else
            // no text -- so just make it an empty string
            lprbbi->lpText[0] = 0;
    }

    if (lprbbi->fMask & RBBIM_IMAGE)
        lprbbi->iImage = prbb->iImage;

    if (lprbbi->fMask & RBBIM_CHILD)
        lprbbi->hwndChild = prbb->hwndChild;

    if (lprbbi->fMask & RBBIM_CHILDSIZE)
    {
        lprbbi->cxMinChild = prbb->cxMinChild;
        lprbbi->cyMinChild = prbb->cyMinChild;
    }

    if (lprbbi->fMask & RBBIM_BACKGROUND)
        lprbbi->hbmBack = prbb->hbmBack;

    if (lprbbi->fMask & RBBIM_ID)
        lprbbi->wID = prbb->wID;

    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// RBSetBandInfo
//
// sets the indicated values in the specified band's internal structure,
// recalculating and refreshing as needed
//
// ----------------------------------------------------------------------------
BOOL NEAR PASCAL RBSetBandInfo(PRB prb, UINT uBand, LPREBARBANDINFO lprbbi)
{
    PRBB    prbb;
    BOOL    fRefresh = FALSE;
    BOOL    fRecalc  = FALSE;
    RECT    rc;

    if (!prb || (uBand >= prb->cBands) || (lprbbi->cbSize != sizeof(REBARBANDINFO)))
        return(FALSE);

    prbb = prb->rbbList + uBand;

    if (lprbbi->fMask & RBBIM_TEXT)
    {
        UINT    cbText;
        LPTSTR  lpNew = NULL;

        if ((!prbb->lpText && lprbbi->lpText) || lstrcmp(lprbbi->lpText, prbb->lpText))
        {
            if (lprbbi->lpText && (cbText = lstrlen(lprbbi->lpText)))
            {
                if (!(lpNew = (LPTSTR)LocalAlloc(LPTR, (cbText + 1)*sizeof(TCHAR))))
                    return(FALSE);

                lstrcpy(lpNew, lprbbi->lpText);
            }

            if (prbb->lpText)
                LocalFree((HLOCAL) prbb->lpText);

            prbb->lpText = lpNew;
            if (RBBCalcTextExtent(prb, prbb, NULL))
                fRecalc = TRUE;
            else
                fRefresh = TRUE;
        }
    }

    if (lprbbi->fMask & RBBIM_STYLE)
    {
        UINT fStylePrev = prbb->fStyle;
        prbb->fStyle = lprbbi->fStyle;

        if (prbb->fStyle ^ fStylePrev)
            fRecalc = TRUE;

        if ((prbb->fStyle & RBBS_FIXEDSIZE) && !(fStylePrev & RBBS_FIXEDSIZE))
            prbb->cxMin = prbb->cx;
        else if (!(prbb->fStyle & RBBS_FIXEDSIZE) && (fStylePrev & RBBS_FIXEDSIZE))
            RBBCalcMinWidth(prb, prbb);
    }

    if (lprbbi->fMask & RBBIM_SIZE)
    {
        if (prbb->cxRequest != (int) lprbbi->cx)
        {
            fRecalc = TRUE;
            prbb->cxRequest = (int) lprbbi->cx;
        }

        if (prbb->fStyle & RBBS_FIXEDSIZE)
            prbb->cxMin = prbb->cxRequest;
    }

    if (lprbbi->fMask & RBBIM_COLORS)
    {
        prbb->clrFore = lprbbi->clrFore;
        prbb->clrBack = lprbbi->clrBack;
        fRefresh = TRUE;
    }

    if ((lprbbi->fMask & RBBIM_IMAGE) && (prbb->iImage != lprbbi->iImage))
    {
        BOOL fToggleBmp = ((prbb->iImage == -1) || (lprbbi->iImage == -1));

        prbb->iImage = lprbbi->iImage;

        if (fToggleBmp)
        {
            fRecalc = TRUE;
            RBBCalcMinWidth(prb, prbb);
        }
        else
            fRefresh = TRUE;
    }

    if (lprbbi->fMask & RBBIM_CHILD)
    {
        if (prbb->hwndChild)
            ShowWindow(prbb->hwndChild, SW_HIDE);

        prbb->hwndChild = lprbbi->hwndChild;
        SetParent(prbb->hwndChild, prb->ci.hwnd);
        ShowWindow(prbb->hwndChild, SW_SHOW);
        //RBResizeChildren(prb, NULL, NULL);
        fRecalc = TRUE;
    }

    if (lprbbi->fMask & RBBIM_CHILDSIZE)
    {
        if (prbb->cyMinChild != lprbbi->cyMinChild)
        {
            prbb->cyMinChild = lprbbi->cyMinChild;
            // TODO:  revisit optimization:
            // if (RBBGetHeight(prb, prbb) != (UINT) prbb->cy)
            fRecalc = TRUE;
        }

        if (prbb->cxMinChild != lprbbi->cxMinChild)
        {
            prbb->cxMinChild = lprbbi->cxMinChild;
            RBBCalcMinWidth(prb, prbb);
        }

    }

#ifdef IEWIN31_25
    if (lprbbi->fMask & RBBIM_BACKGROUND)
    {
        BITMAP  bmp;

        if (lprbbi->hbmBack && !GetObject(lprbbi->hbmBack, sizeof(BITMAP), &bmp))
            return(FALSE);

        prbb->hbmBack = lprbbi->hbmBack;
        prbb->cxBmp = bmp.bmWidth;
        prbb->cyBmp = bmp.bmHeight;
        fRefresh = TRUE;
    }
#else
    if (lprbbi->fMask & RBBIM_BACKGROUND)
    {
        DIBSECTION  dib;

        if (lprbbi->hbmBack && !GetObject(lprbbi->hbmBack, sizeof(DIBSECTION), &dib))
            return(FALSE);

        prbb->hbmBack = lprbbi->hbmBack;
        prbb->cxBmp = dib.dsBm.bmWidth;
        prbb->cyBmp = dib.dsBm.bmHeight;
        fRefresh = TRUE;
    }
#endif

    if (lprbbi->fMask & RBBIM_ID)
        prbb->wID = lprbbi->wID;

    if (fRecalc)
        RBResize(prb, FALSE);
    if (fRefresh)
    {
        if (prb->ci.style & CCS_VERT)
            SetRect(&rc, prbb->y, prbb->x, prbb->y + prbb->cy, prbb->x + prbb->cx);
        else
            SetRect(&rc, prbb->x, prbb->y, prbb->x + prbb->cx, prbb->y + prbb->cy);
        InvalidateRect(prb->ci.hwnd, &rc, TRUE);
    }

    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// RBReallocBands
//
// reallocates the array of bands pointed to by prb->rbbList to the given
// number of bands
//
// ----------------------------------------------------------------------------

BOOL NEAR PASCAL RBReallocBands(PRB prb, UINT cBands)
{
    PRBB rbbList;

    if (!cBands)
    {
        if (prb->rbbList)
        {
            LocalFree((HLOCAL) prb->rbbList);
            prb->rbbList = NULL;
        }
        return(TRUE);
    }

    if (!prb->rbbList)
    {
        if (!(rbbList = (PRBB) LocalAlloc(LMEM_FIXED, sizeof(RBB) * cBands)))
            return(FALSE);
    }
    else
    {
        rbbList = (PRBB) LocalReAlloc(prb->rbbList, cBands * sizeof(RBB), LMEM_MOVEABLE);
        if (!rbbList)
            return(FALSE);
    }

    prb->rbbList = rbbList;
    return(TRUE);
}        

// ----------------------------------------------------------------------------
//
// RBDeleteBand
//
// deletes the indicated band from the rebar's band array (rbbList) and
// decrements the rebar's band count (cBands)
//
// ----------------------------------------------------------------------------
BOOL NEAR PASCAL RBDeleteBand(PRB prb, UINT uBand)
{
    PRBB prbb;
    PRBB prbbStop;
    BOOL fRecalcFirst;

    if (!prb || (uBand >= prb->cBands))
        return(FALSE);

    prbb = prb->rbbList + uBand;

    if (prbb->lpText)
    {
        LocalFree((HLOCAL) prbb->lpText);
        prbb->lpText = NULL;
    }

    // don't destroy the hbmBack 'cause it's given to us by app

    prb->cBands--;

    fRecalcFirst = (!uBand && prb->cBands);

    if (prbb->hwndChild)
        ShowWindow(prbb->hwndChild, SW_HIDE);

    prbbStop = prb->rbbList + prb->cBands;
    for ( ; prbb < prbbStop; prbb++)
        *prbb = *(prbb + 1);

    if (fRecalcFirst)
    {
        if ((prb->rbbList->fStyle & RBBS_FIXEDSIZE) && (prb->cBands > 1))
            // get rid of line break on NEW first item
            prb->rbbList[1].fStyle &= ~RBBS_BREAK;

        if (prb->ci.style & RBS_FIXEDORDER)
            // this is because the min width is now based on it's movability -- and
            // since we are deleting the first item, the new first item becomes
            // immovable
            RBBCalcMinWidth(prb, prb->rbbList);
    }

    RBReallocBands(prb, prb->cBands);

    RBResize(prb, FALSE);
    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// RBInsertBand
//
// inserts a new band at the given position in the rebar's band array (rbbList),
// increments the rebar's band count (cBands), and sets the band's structure
// based on the given REBARBANDINFO structure.
//
// ----------------------------------------------------------------------------
BOOL NEAR PASCAL RBInsertBand(PRB prb, UINT uBand, LPREBARBANDINFO lprbbi)
{
    PRBB prbb;
    UINT i;

    if (!prb || (lprbbi->cbSize != sizeof(REBARBANDINFO)))
        return(FALSE);

    if (uBand == -1)
        uBand = prb->cBands;
    else if (uBand > prb->cBands)
        return(FALSE);

    if (!RBReallocBands(prb, prb->cBands + 1))
        return(FALSE);

    for (i = prb->cBands; i > uBand; i--)
        prb->rbbList[i] = prb->rbbList[i - 1];

    prbb = prb->rbbList + uBand;

    prb->cBands++;

    prbb->fStyle = 0;
    prbb->clrFore = 0x00000000;
    prbb->clrBack = 0x00FFFFFF;
    prbb->lpText = NULL;
    prbb->cxText = 0;
    prbb->iImage = -1;
    prbb->hwndChild = NULL;
    prbb->cxMinChild = 0;
    prbb->cyMinChild = 0;
    prbb->hbmBack = 0;
    prbb->x = 0;
    prbb->y = 0;
    prbb->cx = 0;
    prbb->cy = 0;
    prbb->cxRestored = 0;
    prbb->cxRequest = 0;
    RBBCalcMinWidth(prb, prbb);

    if (!RBSetBandInfo(prb, uBand, lprbbi))
    {
        RBDeleteBand(prb, uBand);
        return(FALSE);
    }

    return(TRUE);
}

#pragma code_seg(CODESEG_INIT)

LRESULT CALLBACK ReBarWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);

BOOL FAR PASCAL InitReBarClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInstance, c_szReBarClass, &wc))
    {
#ifndef WIN32
#ifndef IEWIN31
        extern LRESULT CALLBACK _ReBarWndProc(HWND, UINT, WPARAM, LPARAM);
        wc.lpfnWndProc  = _ReBarWndProc;
#else
        wc.lpfnWndProc  = (WNDPROC) ReBarWndProc;
#endif
#else
        wc.lpfnWndProc  = (WNDPROC) ReBarWndProc;
#endif

        wc.lpszClassName= c_szReBarClass;
        wc.style        = CS_GLOBALCLASS;
        wc.cbClsExtra   = 0;
        wc.cbWndExtra   = sizeof(PRB);
        wc.hInstance    = hInstance;   // use DLL instance if in DLL
        wc.hIcon        = NULL;
        wc.hCursor      = NULL;
        wc.hbrBackground= (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszMenuName = NULL;

        if (!RegisterClass(&wc))
            return(FALSE);
    }

    return(TRUE);
}
#pragma code_seg()


// ----------------------------------------------------------------------------
//
// RBToggleBand
//
// switches a band between it's maximized and minimized state, based on where
// the user clicked
//
// ----------------------------------------------------------------------------
void NEAR PASCAL RBToggleBand(PRB prb)
{
    PRBB prbb = prb->rbbList + prb->iCapture;
    RECT rc;
    int  x;
    PRBB prbbNext;
    int  xRight;
    BOOL fBandBorders = (BOOL)(prb->ci.style & RBS_BANDBORDERS);

    if (!prbb->x)
    {
        //left-most band -- slide the next band accordingly

        if (prb->iCapture == (int) (prb->cBands - 1))
            // last guy in list, nothing to move
            return;

        prbbNext = prbb + 1;

        if (prbbNext->fStyle & (RBBS_BREAK | RBBS_FIXEDSIZE))
            // nobody else on this line or next guy isn't sizable -- see ya'
            return;

        // toggle prbb between maximized and restored
        if (prbbNext->cx == prbbNext->cxMin)
        {
            // prbb currently maximized -- toggle to restored
            if (!prbb->cxRestored || (prbb->cx < prbb->cxRestored))
                // not yet maximized or restored size is bigger than current size
                x = prbb->x + prbb->cxMin;
            else
                x = prbb->x + prbb->cxRestored;

            if (fBandBorders)
                x += g_cxEdge;
        }
        else
        {
            // prbb currently not maximized -- save restored size and maximize
            prbb->cxRestored = prbb->cx;
            x = prbbNext->x + prbbNext->cx - prbbNext->cxMin;
        }
    }
    else
    {
        prbbNext = prbb--;

        // toggle prbbNext between maximized and restored
        if (prbb->cx == prbb->cxMin)
        {
            // prbbNext currently maximized -- toggle to restored
            x = prbbNext->x + prbbNext->cx;
            if (!prbbNext->cxRestored || (prbbNext->cx < prbbNext->cxRestored))
                // not yet maximized or restored size is bigger than current size
                x -= prbbNext->cxMin;
            else
                x -= prbbNext->cxRestored;
        }
        else
        {
            // prbbNext currently not maximized -- save restored size and maximize
            prbbNext->cxRestored = prbbNext->cx;
            x = prbb->x + prbb->cxMin;

            if (fBandBorders)
                x += g_cxEdge;
        }
    }

    SetRect(&rc, prbb->x, prbb->y, prbb->x + prbb->cx, prbb->y + prbb->cy);
    rc.left = prbb->x + prbb->cx;

    xRight = prbbNext->x + prbbNext->cx;
    prbbNext->x = x;
    prbbNext->cx = xRight - x;
    prbb->cx = x - prbb->x;

    if (rc.left > x)
    {
        rc.right = rc.left;
        rc.left = x;
    }
    else
        rc.right = x;

    rc.right += prbbNext->cxMin;
    if (fBandBorders)
    {
        // acount for etch line in preceding band that will need to move
        prbb->cx -= g_cxEdge;
        rc.left -= g_cxEdge;
        rc.bottom += g_cyEdge / 2;
    }
    if (prb->ci.style & CCS_VERT)
    {
        UINT uiTemp;
        uiTemp = rc.left; rc.left = rc.top; rc.top = uiTemp;
        uiTemp = rc.bottom; rc.bottom = rc.right; rc.right = uiTemp;
    }
    InvalidateRect(prb->ci.hwnd, &rc, TRUE);
    RBResizeChildren(prb, prbb, prbbNext);
    UpdateWindow(prb->ci.hwnd);
}


// ----------------------------------------------------------------------------
//
// RBSetCursor
//
// sets the cursor to either the move cursor or the arrow cursor, depending
// on whether or not the cursor is on a band's caption
//
// ----------------------------------------------------------------------------
void NEAR PASCAL RBSetCursor(PRB prb, int x, int y)
{
    int             iBand;
    RBHITTESTINFO   rbht;

    rbht.pt.x = x;
    rbht.pt.y = y;
    iBand = RBHitTest(prb, &rbht);

    if (rbht.flags == RBHT_GRABBER)
    {
        SetCursor(LoadCursor(NULL, (prb->ci.style & CCS_VERT) ? IDC_SIZENS : IDC_SIZEWE));
        return;
    }
    else if ((rbht.flags == RBHT_CAPTION) && RBCanBandMove(prb, prb->rbbList + iBand))
    {
        SetCursor(LoadCursor(HINST_THISDLL, MAKEINTRESOURCE(IDC_HAND)));
        return;
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

// ----------------------------------------------------------------------------
//
// RBDragBandVert
//
// checks for sufficient vertical drag movement and, if so, vertically
// rearranges bands on rebar.  Returns TRUE if any rearranging was performed,
// FALSE otherwise.
//
// ----------------------------------------------------------------------------
BOOL FAR PASCAL RBDragBandVert(PRB prb, int x, int y)
{
    PRBB prbb = prb->rbbList + prb->iCapture;
    PRBB prbbLast = prb->rbbList + prb->cBands;
    PRBB prbbWalk;

    if (y <= (prbb->y - (prbb->cy / 2)))
    {
        // try to move up one line
        if (prbb->y)
        {
            // can only move up if it's not in the top row

            // walk back 'til we find a break (or autobreak) -- we KNOW we're
            // going to find a band with this style 'cause we're not on the
            // first row
            prbbWalk = prbb;
            while (prbbWalk->x)
                prbbWalk--;

            // bands will be reordered if this rebar is not a fixed order rebar
            if (!(prb->ci.style & RBS_FIXEDORDER))
            {
                if (prbb == prbbWalk)
                    prbb->fStyle &= ~RBBS_BREAK;

                prbbWalk--;
                // walk back to find correct horizontal position for band
                while (prbbWalk->x && ((prbbWalk->x + ((prbbWalk->cxMin * 2) / 3)) > x))
                    prbbWalk--;
                prbbWalk++;

                if (prbbWalk->fStyle & RBBS_BREAK)
                {
                    prbb->fStyle |= RBBS_BREAK;
                    prbbWalk->fStyle &= ~RBBS_BREAK;
                }

                // rearrange bands
                RBMoveBand(prb, prb->iCapture,
                            ((WORD) prbbWalk - (WORD) prb->rbbList) / sizeof(RBB));
                return(TRUE);
            }
            else if (prbbWalk->fStyle & RBBS_BREAK)
            {
                // explicit break (i.e. not an autobreak) -- move break to next
                // band
                prbbWalk->fStyle &= ~RBBS_BREAK;
                if (prbb->fStyle & RBBS_DRAGBREAK)
                    prbb->fStyle &= ~RBBS_DRAGBREAK;
                else if (prbb < (prbbLast - 1))
                {
                    if (!((prbb + 1)->fStyle & RBBS_FIXEDSIZE) &&
                        !((prbb + 1)->fStyle & RBBS_BREAK))
                    {
                        (prbb + 1)->fStyle |= RBBS_BREAK;
                        if (prb->ci.style & RBS_FIXEDORDER)
                            prbb->fStyle |= RBBS_DRAGBREAK;
                    }
                }

                return(TRUE);
            }
        }
    }
    else if (y >= (prbb->y + (prbb->cy * 3 / 2)))
    {
        prbbWalk = prbb;

        // can only move down if it's not already on it's own line
        if (!prbb->x)
        {
            // first band on line -- we can do something about this if the
            // rebar isn't fixed order
            if (!(prb->ci.style & RBS_FIXEDORDER))
            {
                BOOL fFound = FALSE;

                while (++prbbWalk < prbbLast)
                {
                    if (!prbbWalk->x)
                        break;

                    if (!(prbbWalk->fStyle & RBBS_FIXEDSIZE))
                        fFound = TRUE;
                }

                if (fFound)
                {
                    // this band has a non-fixedsize band after it on the same
                    // row; so it can be moved to next row.

                    if (prbb->y)
                    {
                        (prbb + 1)->fStyle |= RBBS_BREAK;
                        prbb->fStyle &= ~RBBS_BREAK;
                    }
                    goto MoveDown;
                }
            }
        }
        else
        {
            if (!(prb->ci.style & RBS_FIXEDORDER))
            {
                while (++prbbWalk < prbbLast)
                {
                    if (!prbbWalk->x)
                        break;
                }

                if (prbb != (prbbWalk - 1))
                {
MoveDown:
                    if (prbbWalk == prbbLast)
                        prbb->fStyle |= RBBS_BREAK;
                    else
                    {
                        if (x > 0)
                        {
                            prbbWalk++; // skip first band on next row 
                                
                            // walk forward to find correct horizontal position
                            // for band
                            while ((prbbWalk < prbbLast) && prbbWalk->x &&
                                (prbbWalk->x + ((prbbWalk->cxMin * 2) / 3) < x))
                                prbbWalk++;
                        }

                        if ((prbbWalk < prbbLast) && (!prbbWalk->x))
                        {
                            prbb->fStyle |= RBBS_BREAK;
                                prbbWalk->fStyle &= ~RBBS_BREAK;
                            prbb->fStyle &= ~RBBS_DRAGBREAK; //???
                        }
                    }
                    prbbWalk--;

                    RBMoveBand(prb, prb->iCapture,
                            ((WORD) prbbWalk - (WORD) prb->rbbList) / sizeof(RBB));
                    return(TRUE);
                }
            }

            prbb->fStyle |= RBBS_BREAK;

            if (prb->ci.style & RBS_FIXEDORDER)
            {
                if (prbb->fStyle & RBBS_DRAGBREAK)
                {
                    if (prb->iCapture < (int) (prb->cBands - 1))
                    {
                        if (!((prbb + 1)->fStyle & RBBS_FIXEDSIZE))
                        {
                            (prbb + 1)->fStyle &= ~RBBS_BREAK;
                            prbb->fStyle &= ~RBBS_DRAGBREAK;
                        }
                    }
                }
                else
                    prbb->fStyle |= RBBS_DRAGBREAK;
            }

            return(TRUE);
        }
    }
    return(FALSE);
}

// ----------------------------------------------------------------------------
//
// RBDragSize
//
// adjust the captured band's starting location to the given location and
// redraw
//
// ----------------------------------------------------------------------------
BOOL RBDragSize(PRB prb, int xLeft)
{
    RECT    rc;
    PRBB    prbb = prb->rbbList + prb->iCapture;
    PRBB    prbbPrev = prbb - 1;
    int     xRight;
    BOOL    fBandBorders = (BOOL)(prb->ci.style & RBS_BANDBORDERS);
    BOOL    fRight;

    if (prbb->x == xLeft)
        return(FALSE);

    // band has moved within valid range -- adjust band sizes and redraw
    // window
    fRight = (prbb->x < xLeft);

    SetRect(&rc, prbb->x, prbb->y, prbb->x + prbb->cxMin, prbb->y + prbb->cy);
    xRight = prbb->x + prbb->cx;
    prbb->x = xLeft;
    prbb->cx = xRight - xLeft;
    prbb->cxRequest = prbb->cx;

    if (fRight)
    {
        //moving right
        prbbPrev->cx = prbb->x - prbbPrev->x;
        if (fBandBorders)
        {
            prbbPrev->cx -= g_cxEdge;
            rc.left -= g_cxEdge;
        }
        prbbPrev->cxRequest = prbbPrev->cx;

        //check for compacting of following bands

        while (prbb->cx < prbb->cxMin)
        {
            prbb->cx = prbb->cxMin;
            prbb->cxRequest = prbb->cx;
            xLeft += prbb->cxMin;
            if (fBandBorders)
                xLeft += g_cxEdge;
            prbb++;
            xRight = prbb->x + prbb->cx;
            prbb->x = xLeft;
            prbb->cx = xRight - xLeft;
            prbb->cxRequest = prbb->cx;
        }
        rc.right = xLeft + prbb->cxMin;
    }
    else
    {
        //moving left

        //check for compacting of preceding bands
CompactPrevious:
        if (fBandBorders)
            xLeft -= g_cxEdge;
        prbbPrev->cx = xLeft - prbbPrev->x;
        prbbPrev->cxRequest = prbbPrev->cx;
        if (prbbPrev->cx < prbbPrev->cxMin)
        {
            prbbPrev->x = xLeft - prbbPrev->cxMin;
            prbbPrev->cx = prbbPrev->cxMin;
            prbbPrev->cxRequest = prbbPrev->cx;
            xLeft = prbbPrev->x;
            prbbPrev--;
            goto CompactPrevious;
        }
        rc.left = xLeft;
    }

    if (fBandBorders)
        rc.bottom += g_cyEdge / 2;

    RBResizeChildren(prb, NULL, NULL);
    if (prb->ci.style & CCS_VERT)
    {
        // for vertical oriented rebar, reverse x & y (using xLeft as swap var)
        xLeft = rc.left;    rc.left   = rc.top;     rc.top   = xLeft;
        xLeft = rc.bottom;  rc.bottom = rc.right;   rc.right = xLeft;
    }
    InvalidateRect(prb->ci.hwnd, &rc, TRUE);
    UpdateWindow(prb->ci.hwnd);
    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// RBDragBand
//
// resizes the currently tracked band based on the user's mouse movement as
// indicated in the given point (x, y)
//
// ----------------------------------------------------------------------------
void FAR PASCAL RBDragBand(PRB prb, int x, int y)
{
    PRBB    prbb;
    PRBB    prbbLast = prb->rbbList + prb->cBands;
    int     xLeft;
    BOOL    fBandBorders = (BOOL)(prb->ci.style & RBS_BANDBORDERS);

    if (prb->ci.style & CCS_VERT)
    {
        // for vertical oriented rebar, reverse x & y (using xLeft as swap var)
        xLeft = x;
        x = y;
        y = xLeft;
    }

    if (!prb->fFullOnDrag)
    {
        // don't begin dragging until mouse is moved outside of an edge-thick
        // tolerance border
        if ((y < (prb->ptCapture.y - g_cyEdge)) || (y > (prb->ptCapture.y + g_cyEdge)) ||
            (x < (prb->ptCapture.x - g_cxEdge)) || (x > (prb->ptCapture.x + g_cxEdge)))
            prb->fFullOnDrag = TRUE;
        else
            return;
    }

    if (RBDragBandVert(prb, x, y))
        // there were vertical changes -- resize the rebar
        RBResize(prb, FALSE);

    // don't set prbb until now since above RBResize could've changed iCapture
    prbb = prb->rbbList + prb->iCapture;

    xLeft = prb->xStart + (x - prb->ptCapture.x);

    if (!prbb->x)
    {
        // first band in row

        // for fixed order rebars, can't move first band in row
        if (!(prb->ci.style & RBS_FIXEDORDER))
        {
            PRBB prbbNext = prbb + 1;

            if ((prbbNext < prbbLast) && prbbNext->x &&
                !(prbbNext->fStyle & RBBS_FIXEDSIZE) &&
                (x > (prbbNext->x + ((prbbNext->cxMin * 2) / 3))))
            {
                // next band is a non-fixedsize band on same row and there's
                // been sufficient drag motion -- switch the order of these
                // two bands
                if (prb->iCapture > 0)
                {
                    // we're not on the first row -- move the break style from
                    // the captured band to the next band
                    prbb->fStyle &= ~RBBS_BREAK;
                    prbbNext->fStyle |= RBBS_BREAK;
                }
                RBMoveBand(prb, prb->iCapture, prb->iCapture + 1);
                RBResize(prb, FALSE);
            }
        }
    }
    else
    {
        // now we can assume there is a previous band since prbb->x is nonzero
        int     xLimit;
        PRBB    prbbWalk;

        // perform bounds checking

        // calc minimum
        prbbWalk = prbb;
        xLimit = 0;
        while (prbbWalk->x)
        {
            prbbWalk--;
            xLimit += prbbWalk->cxMin;
            if (fBandBorders)
                xLimit += g_cxEdge;
        }

        if (xLeft < xLimit)
        {
            xLeft = xLimit;

            if (!(prb->ci.style & RBS_FIXEDORDER))
            {
                // for non-fixed order rebars, move this band forward on row
                prbbWalk = prbb;

                // walk preceding bands in row to first band that can handle
                // new position of dragged band
                while (prbbWalk->x &&
                    (x < (xLimit - (((prbbWalk - 1)->cxMin * 2) / 3))))
                {
                    prbbWalk--;
                    xLimit -= prbbWalk->cxMin;
                    if (fBandBorders)
                        xLimit -= g_cxEdge;
                }

                if (prbbWalk < prbb)
                {
                    // this band can be moved in front of at least one band
                    if (prbbWalk->fStyle & RBBS_BREAK)
                    {
                        // we're moving dragged band to beginning of row;
                        // transfer break style from
                        prbbWalk->fStyle &= ~RBBS_BREAK;
                        prbb->fStyle |= RBBS_BREAK;
                    }
                    RBMoveBand(prb, prb->iCapture,
                        ((WORD) prbbWalk - (WORD) prb->rbbList) / sizeof(RBB));
                    RBResize(prb, FALSE);
                    xLeft = xLimit;
                }
            }
        }
        else
        {
            // calc maximum
            prbbWalk = prbb;

            xLimit = 0;
            while ((prbbWalk < prbbLast) && prbbWalk->x )
            {
                if (fBandBorders && (prbbWalk != prbb))
                    xLimit += g_cxEdge;
                xLimit += prbbWalk->cxMin;
                prbbWalk++;
            }
            prbbWalk--;
            xLimit = prbbWalk->x + prbbWalk->cx - xLimit;

            if (xLeft > xLimit)
            {
                xLeft = xLimit;

                if (!(prb->ci.style & RBS_FIXEDORDER))
                {
                    // for non-fixed order rebars, move this band later on row
                    prbbWalk = prbb + 1;

                    x -= prbb->cxMin;
                    if (fBandBorders)
                        x -= g_cxEdge;

                    // walk following bands in row to first band that can handle
                    // new position of dragged band
                    while ((prbbWalk < prbbLast) && prbbWalk->x &&
                        !(prbbWalk->fStyle & RBBS_FIXEDSIZE) &&
                        (x > (xLimit + ((prbbWalk->cxMin * 2) / 3))))
                    {
                        if (fBandBorders)
                            xLimit += g_cxEdge;
                        xLimit += prbbWalk->cxMin;
                        prbbWalk++;
                    }
                    prbbWalk--;

                    if (prbbWalk > prbb)
                    {
                        RBMoveBand(prb, prb->iCapture,
                            ((WORD) prbbWalk - (WORD) prb->rbbList) / sizeof(RBB));
                        RBResize(prb, FALSE);
                        xLeft = xLimit;
                    }
                }
            }
        }
        RBDragSize(prb, xLeft);
    }
}

// ----------------------------------------------------------------------------
//
// RBDestroy
//
// frees all memory allocated by rebar, including rebar structure
//
// ----------------------------------------------------------------------------
BOOL NEAR PASCAL RBDestroy(PRB prb)
{
    UINT c = prb->cBands;
    while (c--)
        RBDeleteBand(prb, c);

    if (prb->hpal)
        DeleteObject(prb->hpal);

    if (prb->hFont)
        DeleteObject(prb->hFont);

    if ((prb->ci.style & RBS_TOOLTIPS) && IsWindow(prb->hwndToolTips))
    {
        DestroyWindow (prb->hwndToolTips);
        prb->hwndToolTips = NULL;
    }


    // don't destroy the himl 'cause it's given to us by app

    SetWindowInt(prb->ci.hwnd, 0, 0);
    LocalFree((HLOCAL) prb);
    return(TRUE);
}

// ----------------------------------------------------------------------------
//
// RBInitPaletteHack
//
// this is a hack to use the halftone palette until we have a way of asking
// the client what palette they are using
//
// ----------------------------------------------------------------------------
BOOL NEAR PASCAL RBInitPaletteHack(PRB prb)
{
    HDC hdc = CreateCompatibleDC(NULL);
    if (!hdc)
        return(FALSE);
    if (GetDeviceCaps(hdc, BITSPIXEL) <= 8)
        prb->hpal = CreateHalftonePalette(hdc);  // this is a hack
    DeleteDC(hdc);
    return(TRUE);
}

LRESULT CALLBACK ReBarWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    PRB     prb = (PRB) GetWindowInt(hwnd, 0);
    int     iBand;

    switch (wMsg)
    {
        case WM_NCCREATE:
#define lpcs ((LPCREATESTRUCT) lParam)
            InitGlobalColors();

            if (!(prb = (PRB) LocalAlloc(LPTR, sizeof(RB))))
                return(0L);

            SetWindowInt(hwnd, 0, (int) prb);

            prb->iCapture = -1;

            // note, zero init memory from above
            CIInitialize(&prb->ci, hwnd, lpcs);

            RBInitPaletteHack(prb);

            if (!(prb->ci.style & (CCS_TOP | CCS_NOMOVEY | CCS_BOTTOM)))
            {
                prb->ci.style |= CCS_TOP;
                SetWindowLong(hwnd, GWL_STYLE, prb->ci.style);
            }

            if (prb->ci.style & RBS_TOOLTIPS)
            {
                TOOLINFO ti;
                // don't bother setting the rect because we'll do it below
                // in FlushToolTipsMgr;
                ti.cbSize = sizeof(ti);
                ti.uFlags = TTF_IDISHWND;
                ti.hwnd = hwnd;
                ti.uId = (UINT)hwnd;
                ti.lpszText = 0;

                prb->hwndToolTips = CreateWindow(c_szSToolTipsClass, NULL,
                    WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    hwnd, NULL, lpcs->hInstance, NULL);

                SendMessage(prb->hwndToolTips, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO) &ti);
            }

            RBSetFont(prb, 0);

            if (lpcs->lpCreateParams)
                RBSetBarInfo(prb, (LPREBARINFO) (lpcs->lpCreateParams));
#undef lpcs
            return TRUE;

        case WM_DESTROY:
            if (!prb)
                break;
            RBDestroy(prb);
            SetWindowInt(hwnd, 0, 0);
            break;

        case WM_NCHITTEST:
            return HTCLIENT;

        case WM_NCCALCSIZE:
            if (prb && (prb->ci.style & WS_BORDER))
            {
                InflateRect((LPRECT) lParam, -g_cxEdge, -g_cyEdge);
                break;
            }
            goto CallDWP;

        case WM_NCPAINT:
            if (prb && (prb->ci.style & WS_BORDER))
            {
                RECT rc;
                HDC hdc;
                HWND hwndParent = GetParent(hwnd);

                GetWindowRect(hwnd, &rc);
                MapWindowPoints(HWND_DESKTOP, hwndParent, (LPPOINT)&rc, 2);
                hdc = GetWindowDC(hwnd);
                DrawEdge(hdc, &rc, EDGE_ETCHED, BF_RECT);
                ReleaseDC(hwnd, hdc);
                break;
            }
            goto CallDWP;

        case WM_PAINT:
            if (!prb)
                break;

        case WM_PRINTCLIENT:
            RBPaint(prb, (HDC)wParam);
            break;

        case WM_ERASEBKGND:
            if (RBEraseBkgnd(prb, (HDC) wParam, -1))
                return(TRUE);
            goto CallDWP;

        case WM_SYSCOLORCHANGE:
            if (!prb)
                break;

            if (prb->hpal)
                DeleteObject(prb->hpal);

            RBInitPaletteHack(prb);

            if (prb->hwndToolTips)
                SendMessage(prb->hwndToolTips, wMsg, wParam, lParam);

            ReInitGlobalColors();
            break;

        case WM_SIZE:
            if (!prb)
                break;

            RBResize(prb, FALSE);
            break;

        case WM_GETFONT:
            return((LRESULT) (prb ? prb->hFont : NULL));

        case WM_COMMAND:
        case WM_DRAWITEM:
        case WM_MEASUREITEM:
        case WM_VKEYTOITEM:
        case WM_CHARTOITEM:
            if (!prb)
                break;

            SendMessage(prb->ci.hwndParent, wMsg, wParam, lParam);
            break;

        case WM_LBUTTONDOWN:
            if (prb)
            {
                RBHITTESTINFO rbht;

                rbht.pt.x = GET_X_LPARAM(lParam);
                rbht.pt.y = GET_Y_LPARAM(lParam);

                RelayToToolTips(prb->hwndToolTips, hwnd, wMsg, wParam, lParam);

                iBand = RBHitTest(prb, &rbht);

                if ((iBand != -1) && (rbht.flags != RBHT_CLIENT) && RBCanBandMove(prb, prb->rbbList + iBand))
                {
                    prb->iCapture = iBand;
                    prb->ptCapture.x = (prb->ci.style & CCS_VERT) ? rbht.pt.y : rbht.pt.x;
                    prb->ptCapture.y = (prb->ci.style & CCS_VERT) ? rbht.pt.x : rbht.pt.y;
                    prb->xStart = prb->rbbList[iBand].x;
                    SetCapture(hwnd);
                    prb->fFullOnDrag = FALSE;
                    //SendItemNotify(prb, iBand, RBN_BEGINDRAG);
                }
            }
            break;

        case WM_SETCURSOR:
            if (prb)
            {
                // Give the parent first crack, if it sets the cursor then
                // leave it at that.  Otherwise if the cursor is over our
                // window then set it to what we want it to be.
                if (!DefWindowProc(hwnd, wMsg, wParam, lParam) &&
                    (hwnd == (HWND)wParam))
                {
                    POINT   pt;
                    lParam = GetMessagePos();
                    pt.x = ((prb->ci.style & CCS_VERT) ? GET_Y_LPARAM(lParam) : GET_X_LPARAM(lParam));
                    pt.y = ((prb->ci.style & CCS_VERT) ? GET_X_LPARAM(lParam) : GET_Y_LPARAM(lParam));
                    MapWindowPoints(NULL, prb->ci.hwnd, &pt, 1);
                    RBSetCursor(prb, pt.x, pt.y);
                }
                return TRUE;
            }
            break;

        case WM_MOUSEMOVE:
            if (!prb)
                break;

            RelayToToolTips(prb->hwndToolTips, hwnd, wMsg, wParam, lParam);

            if (prb->iCapture != -1)
            {
                // captured band -- mouse is down
                if (hwnd != GetCapture())
                {
                    //SendItemNotify(prb, ?, RBN_ENDDRAG);
                    prb->iCapture = -1;
                }
                else
                    RBDragBand(prb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            }
//  Don't set the cursor here.  It is unecessary and will mess up our
//  cursor processing code in the WM_SETCURSOR handler.
//            else
//                RBSetCursor(prb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;

        case WM_LBUTTONUP:
            if (!prb)
                break;

            RelayToToolTips(prb->hwndToolTips, hwnd, wMsg, wParam, lParam);

            if (prb->iCapture != -1)
            {
                ReleaseCapture();
                // if there was no significant mouse motion, treat as a click
                if (!prb->fFullOnDrag)
                    RBToggleBand(prb);

                prb->rbbList[prb->iCapture].fStyle &= ~RBBS_DRAGBREAK;
                prb->iCapture = -1;
            }
            break;
#ifdef IEWIN31_25
        // Simulate WM_CONTEXTMENU commands on windows 3.1
        case WM_RBUTTONUP:
        {
            POINT pt;

            if (!prb)
                break;

            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            ClientToScreen(hwnd, &pt);
            SendMessage(prb->ci.hwndParent, WM_CONTEXTMENU, (WPARAM)hwnd, MAKELONG(pt.x, pt.y));
            break;
        }

        // Pass on context menus from children on to our parent
        case WM_CONTEXTMENU:
        {
            if (!prb)
                break;

            SendMessage(prb->ci.hwndParent, WM_CONTEXTMENU, wParam, lParam);
            break;
        }

#endif

        case WM_WININICHANGE:
            InitGlobalMetrics(wParam);
            RBSetFont(prb, wParam);

            if (prb->hwndToolTips)
                SendMessage(prb->hwndToolTips, wMsg, wParam, lParam);

            break;

        case WM_NOTIFYFORMAT:
            return(CIHandleNotifyFormat(&prb->ci, lParam));

        case WM_NOTIFY:
            if (!prb)
                break;

            // We are just going to pass this on to the real parent
            // Note that -1 is used as the hwndFrom.  This prevents SendNotifyEx
            // from updating the NMHDR structure.
            return(SendNotifyEx(prb->ci.hwndParent, (HWND) -1,
                     ((LPNMHDR) lParam)->code, (LPNMHDR) lParam, prb->ci.bUnicode));

        case WM_STYLECHANGED:
#define lpss ((LPSTYLESTRUCT) lParam)
            if (!prb)
                break;

            if (wParam == GWL_STYLE)
            {
                prb->ci.style = ((LPSTYLESTRUCT)lParam)->styleNew;
                // update to reflect style change
#ifndef WINNT
                DebugMsg(DM_TRACE, TEXT("rebar window style changed %x"), prb->ci.style);
#endif
                if ((lpss->styleOld & CCS_VERT) != (lpss->styleNew & CCS_VERT))
                {
                    UINT i;
                    for (i = 0; i < prb->cBands; i++)
                       RBBCalcMinWidth(prb,(PRBB)&(prb->rbbList[i]));
                    RBResize(prb, TRUE);
                }
            }
            break;

#ifdef UNICODE
    case RB_INSERTBANDA:
        {
            LPWSTR lpStrings;
            UINT   uiCount;
            LPSTR  lpAnsiString;
            int    iResult;

            if (!prb )
                break;

            if (!(((LPREBARBANDINFOA)lParam)->fMask & RBBIM_TEXT))
                return RBInsertBand(prb, (UINT) wParam, (LPREBARBANDINFO) lParam);

            lpAnsiString = ((LPREBARBANDINFOA)lParam)->lpText;
            uiCount = lstrlenA(lpAnsiString)+1;

            lpStrings = GlobalAlloc (GPTR, uiCount * sizeof(TCHAR));

            if (!lpStrings)
                return -1;

            MultiByteToWideChar(CP_ACP, 0, (LPCSTR) ((LPREBARBANDINFOA)lParam)->lpText, uiCount,
                                lpStrings, uiCount);

            ((LPREBARBANDINFOA)lParam)->lpText = (LPSTR)lpStrings;
            iResult = RBInsertBand(prb, (UINT) wParam, (LPREBARBANDINFO) lParam);
            ((LPREBARBANDINFOA)lParam)->lpText = lpAnsiString;

            GlobalFree(lpStrings);

            return iResult;
        }
#endif

        case RB_INSERTBAND:
            if (!prb)
                break;

            return(RBInsertBand(prb, (UINT) wParam, (LPREBARBANDINFO) lParam));

        case RB_DELETEBAND:
            if (!prb)
                break;

            return(RBDeleteBand(prb, (UINT) wParam));

        case RB_GETBANDINFO:
            return(RBGetBandInfo(prb, (UINT) wParam, (LPREBARBANDINFO) lParam));

        case RB_GETROWCOUNT:
            if (!prb)
                break;

            return(RBGetRowCount(prb));

        case RB_GETROWHEIGHT:
            if (prb && (wParam < prb->cBands))
            {
                UINT i, j;
                i = (UINT) wParam;
                j = i;

                // move back to start of line
                while (prb->rbbList[i].x)
                    i--;

                while ((j + 1) < prb->cBands)
                {
                    if (!prb->rbbList[j].x)
                        break;

                    j++;
                }

                return(RBGetLineHeight(prb, i, j));
            }
            break;

        case RB_GETBANDCOUNT:
            if (!prb)
                break;

            return(prb->cBands);

#ifdef UNICODE
        case RB_SETBANDINFOA: {
            LPWSTR lpStrings;
            UINT   uiCount;
            LPSTR  lpAnsiString = ((LPREBARBANDINFOA)lParam)->lpText;
            int    iResult;

            if (!prb )
                break;

            if (!(((LPREBARBANDINFOA)lParam)->fMask & RBBIM_TEXT))
                return RBSetBandInfo(prb, (UINT) wParam, (LPREBARBANDINFO) lParam);

            uiCount = lstrlenA(lpAnsiString)+1;

            lpStrings = GlobalAlloc (GPTR, uiCount * sizeof(TCHAR));

            if (!lpStrings)
                return -1;

            MultiByteToWideChar(CP_ACP, 0, (LPCSTR) ((LPREBARBANDINFOA)lParam)->lpText, uiCount,
                                lpStrings, uiCount);

            ((LPREBARBANDINFOA)lParam)->lpText = (LPSTR)lpStrings;
            iResult = RBSetBandInfo(prb, (UINT) wParam, (LPREBARBANDINFO) lParam);
            ((LPREBARBANDINFOA)lParam)->lpText = lpAnsiString;

            GlobalFree(lpStrings);

            return iResult;
        }

#endif // UNICODE
        case RB_SETBANDINFO:
            return(RBSetBandInfo(prb, (UINT) wParam, (LPREBARBANDINFO) lParam));

        case RB_GETBARINFO:
            return(RBGetBarInfo(prb, (LPREBARINFO) lParam));

        case RB_SETBARINFO:
            return(RBSetBarInfo(prb, (LPREBARINFO) lParam));

        case RB_SETPARENT:
            if (prb)
            {
                HWND hwndOld = prb->ci.hwndParent;
                prb->ci.hwndParent = (HWND) wParam;
                return((LRESULT)(UINT) hwndOld);
            }
            break;

        case RB_GETRECT:
            if (prb && (wParam < prb->cBands))
            {
                PRBB prbb = prb->rbbList + wParam;
                LPRECT lprc = (LPRECT) lParam;

                lprc->left = prbb->x;
                lprc->top = prbb->y;
                lprc->right = prbb->x + prbb->cx;
                lprc->bottom = prbb->y + prbb->cy;

                return(TRUE);
            }
            break;

        case RB_HITTEST:
            if (!prb)
            {
                LPRBHITTESTINFO prbht = (LPRBHITTESTINFO) lParam;
                prbht->flags = RBHT_NOWHERE;
                prbht->iBand = -1;
                return(-1);
            }
            return(RBHitTest(prb, (LPRBHITTESTINFO) lParam));

        default:
CallDWP:
            return(DefWindowProc(hwnd, wMsg, wParam, lParam));
    }

    return(0L);
}
