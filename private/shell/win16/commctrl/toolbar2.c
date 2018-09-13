/*
** Toolbar.c
**
** This is it, the incredibly famous toolbar control.  Most of
** the customization stuff is in another file.
*/

#include "ctlspriv.h"
#include "toolbar2.h"
#include "image.h"
#include <limits.h>

extern BOOL WINAPI ImageList_DrawIndirect(IMAGELISTDRAWPARAMS FAR * pimldp);

#define TBIMAGELIST
// these values are defined by the UI gods...
#define DEFAULTBITMAPX 16
#define DEFAULTBITMAPY 15

#define LIST_GAP        g_cxEdge * 2

#define SMALL_DXYBITMAP 16  // new dx dy for sdt images
#define LARGE_DXYBITMAP 24

#define DEFAULTBUTTONX 24
#define DEFAULTBUTTONY 22
// horizontal/vertical space taken up by button chisel, sides,
// and a 1 pixel margin.  used in GrowToolbar.
#define XSLOP 7
#define YSLOP 6

const int g_dxButtonSep = 8;
const int s_xFirstButton = 0;   // was 8 in 3.1
#define s_dxOverlap 0   // was 1 in 3.1

// Globals - since all of these globals are used durring a paint we have to
// take a criticial section around all toolbar paints.  this sucks.
//

const UINT wStateMasks[] = {
    TBSTATE_ENABLED,
    TBSTATE_CHECKED,
    TBSTATE_PRESSED,
    TBSTATE_HIDDEN,
    TBSTATE_INDETERMINATE,
    TBSTATE_HIGHLIGHTED
};

LRESULT CALLBACK ToolbarWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
void NEAR PASCAL TBOnButtonStructSize(PTBSTATE ptb, UINT uStructSize);
BOOL NEAR PASCAL SetBitmapSize(PTBSTATE ptb, int width, int height);
int  NEAR PASCAL AddBitmap(PTBSTATE ptb, int nButtons, HINSTANCE hBMInst, UINT wBMID);
BOOL NEAR PASCAL GrowToolbar(PTBSTATE ptb, int newButWidth, int newButHeight, BOOL bInside);
void NEAR PASCAL TBBuildImageList(PTBSTATE ptb);
BOOL NEAR PASCAL GetItemRect(PTBSTATE ptb, UINT uButton, LPRECT lpRect);
#define TBInvalidateImageList(ptb)  ((ptb)->fHimlValid = FALSE)

//#define HeightWithString(ptb, h) (h + ptb->dyIconFont + 1)

#ifdef IEWIN31_25
#define SEND_WM_COMMAND(hwnd, id, hwndCtl, codeNotify) \
    SendMessage((hwnd), WM_COMMAND, (WPARAM)(int)(id), MAKELPARAM((WORD)(hwndCtl), (WORD)(codeNotify)))

BOOL FAR PASCAL SendItemNotify(PTBSTATE ptb, int iItem, int code)
{
    TBNOTIFY tbn;
    tbn.iItem = iItem;
    // default return from SendNotify is false
    return SendNotify(ptb->ci.hwndParent, ptb->ci.hwnd, code, &tbn.hdr);

    // for compatibility with old users of we do this bogus stuff with WM_COMMAND
    // stuffing in an item index in a field that should be an hwnd
//    return SEND_WM_COMMAND(ptb->ci.hwndParent, GetWindowID(ptb->ci.hwnd), (HWND)iItem, code);
}
#endif

int HeightWithString(PTBSTATE ptb, int h)
{
    if (ptb->ci.style & TBSTYLE_LIST)
        return (max(h, ptb->dyIconFont));
    else if (ptb->dyIconFont)
        return (h + ptb->dyIconFont + 1);
    else
        return (h);
}

int TBWidthOfButton(PTBSTATE ptb, LPTBBUTTON /*PTBBUTTON*/ pButton)
{
    if (pButton->fsStyle & TBSTYLE_SEP)
        return pButton->iBitmap;
    else if ((pButton->fsStyle & TBSTYLE_DROPDOWN) && !(ptb->ci.style & TBSTYLE_FLAT))
        return ptb->iButWidth + (ptb->iButWidth /2);
    else
        return ptb->iButWidth;
}

BOOL NEAR PASCAL TBRecalc(PTBSTATE ptb)
{
    TEXTMETRIC tm;
    int i,j;
    HDC hdc;
    UINT uiStyle = 0;
    int cxMax;
    HFONT hOldFont;

    ptb->dyIconFont = 0;

    if (!ptb->nStrings) {

        cxMax = ptb->iDxBitmap;

    } else {

        SIZE size;
        PTSTR pstr;
        RECT rcText = {0,0,0,0};
        BOOL fEllipsed = FALSE;
        int cxExtra = XSLOP;

        hdc = GetDC(ptb->ci.hwnd);
        if (!hdc)
            return(FALSE);

        hOldFont = SelectObject(hdc, ptb->hfontIcon);
        GetTextMetrics(hdc, &tm);
        if (ptb->nTextRows)
            ptb->dyIconFont = (tm.tmHeight * ptb->nTextRows) +
                (tm.tmExternalLeading * (ptb->nTextRows - 1)); // add an edge ?

        if (ptb->ci.style & TBSTYLE_LIST)
            cxExtra += ptb->iDxBitmap + LIST_GAP;

        cxMax = 0;
        // walk strings to find max width
        for (i=ptb->nStrings-1; i>= 0; i--)
        {
            pstr = ptb->pStrings[i];
            GetTextExtentPoint(hdc, pstr, lstrlen(pstr), &size);
            if (cxMax < size.cx)
                cxMax = size.cx;
        }

        // if cxMax is less than the iButMinWidth - dxBitmap (if LIST) then
        // cxMax = iButMinWidth
        if (ptb->iButMinWidth && (ptb->iButMinWidth > (cxMax + cxExtra)))
            cxMax = ptb->iButMinWidth - cxExtra;

        // Is the cxMax +  dxBitmap (if LIST) more than the max width ?
        if (ptb->iButMaxWidth && (ptb->iButMaxWidth < (cxMax + cxExtra)))
        {
            int cyMax = 0;

            cxMax = ptb->iButMaxWidth - cxExtra;

            uiStyle = DT_CALCRECT;
            if (ptb->nTextRows > 1)
                uiStyle |= DT_WORDBREAK | DT_EDITCONTROL;
            else
                uiStyle |= DT_SINGLELINE;

            // walk strings to set the TBSTATE_ELLIPSES
            for (i=ptb->nStrings-1; i>= 0; i--)
            {
                pstr = ptb->pStrings[i];
                rcText.bottom = ptb->dyIconFont;
                rcText.right = cxMax;

                DrawText(hdc, pstr, lstrlen(pstr), &rcText, uiStyle);
                if (ptb->nTextRows > 1)
                    fEllipsed = (BOOL)(rcText.bottom > ptb->dyIconFont);
                else
                    fEllipsed = (BOOL)(rcText.right > cxMax);

                if (cyMax < rcText.bottom)
                    cyMax = rcText.bottom;

                for (j=0; j < ptb->iNumButtons; j++)
                {
                    if (ptb->Buttons[j].iString == i)
                    {
                        if (fEllipsed)
                            ptb->Buttons[j].fsState |= TBSTATE_ELLIPSES;
                        else
                            ptb->Buttons[j].fsState &= ~TBSTATE_ELLIPSES;
                    }
                } // Find the button using the string
            }

            // Set the text height to the tallest text, with the top end being the number
            // of rows specified by MAXTEXTROWS
            if (ptb->dyIconFont > cyMax)
                ptb->dyIconFont = cyMax;
        }
        else
        {
            for (j=0; j < ptb->iNumButtons; j++)
                ptb->Buttons[j].fsState &= ~TBSTATE_ELLIPSES;

            if ((ptb->nTextRows) && (ptb->dyIconFont > size.cy))
                ptb->dyIconFont = size.cy;
        }

        if (ptb->iButMinWidth && (ptb->iButMinWidth > (cxMax + cxExtra)))
            cxMax = ptb->iButMinWidth - cxExtra;

        if (hOldFont)
            SelectObject(hdc, hOldFont);
        ReleaseDC(ptb->ci.hwnd, hdc);
    }
    
    // at press the button, the button image moves down and to the right
    // we add the one pixel to height of icon text for press image
    ptb->dyIconFont += 1;

    return(GrowToolbar(ptb, cxMax, HeightWithString(ptb, ptb->iDyBitmap), TRUE));
}


BOOL NEAR PASCAL TBChangeFont(PTBSTATE ptb, WPARAM wParam)
{
    HFONT hFont;
    LOGFONT lf;

    if ((wParam != 0) && (wParam != SPI_SETICONTITLELOGFONT) && (wParam != SPI_SETNONCLIENTMETRICS))
        return(FALSE);

    if (!SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0))
        return(FALSE);

    if (!(hFont = CreateFontIndirect(&lf)))
        return(FALSE);

    if (ptb->hfontIcon)
        DeleteObject(ptb->hfontIcon);

    ptb->hfontIcon = hFont;

    return(TBRecalc(ptb));
}

HWND WINAPI CreateToolbarEx(HWND hwnd, DWORD ws, UINT wID, int nBitmaps,
            HINSTANCE hBMInst, UINT wBMID, LPCTBBUTTON lpButtons,
            int iNumButtons, int dxButton, int dyButton,
            int dxBitmap, int dyBitmap, UINT uStructSize)
{

    HWND hwndToolbar = CreateWindow(c_szToolbarClass, NULL, WS_CHILD | ws,
          0, 0, 100, 30, hwnd, (HMENU)wID, HINST_THISDLL, NULL);
    if (hwndToolbar)
    {
        PTBSTATE ptb = (PTBSTATE)GetWindowInt(hwndToolbar, 0);
        TBOnButtonStructSize(ptb, uStructSize);

        if ((dxBitmap && dyBitmap && !SetBitmapSize(ptb, dxBitmap, dyBitmap)) ||
            (dxButton && dyButton && !SetBitmapSize(ptb,dxButton, dyButton)))
        {
            //!!!! do we actually need to deal with this?
            DestroyWindow(hwndToolbar);
            hwndToolbar = NULL;
            goto Error;
        }

        AddBitmap(ptb, nBitmaps, hBMInst, wBMID);
        InsertButtons(ptb, (UINT)-1, iNumButtons, (LPTBBUTTON)lpButtons);

        // ptb may be bogus now after above button insert
    }
Error:
    return hwndToolbar;
}

/* This is no longer declared in COMMCTRL.H.  It only exists for compatibility
** with existing apps; new apps must use CreateToolbarEx.
*/
HWND WINAPI CreateToolbar(HWND hwnd, DWORD ws, UINT wID, int nBitmaps, HINSTANCE hBMInst, UINT wBMID, LPCTBBUTTON lpButtons, int iNumButtons)
{
    // old-style toolbar, so no divider.
    return CreateToolbarEx(hwnd, ws | CCS_NODIVIDER, wID, nBitmaps, hBMInst, wBMID,
                lpButtons, iNumButtons, 0, 0, 0, 0, sizeof(OLDTBBUTTON));
}

#pragma code_seg(CODESEG_INIT)

BOOL FAR PASCAL InitToolbarClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInstance, c_szToolbarClass, &wc)) {
#if !defined(WIN32) && !defined(IEWIN31)
    extern LRESULT CALLBACK _ToolbarWndProc(HWND, UINT, WPARAM, LPARAM);
    wc.lpfnWndProc   = _ToolbarWndProc;
#else
    wc.lpfnWndProc   = (WNDPROC)ToolbarWndProc;
#endif

    wc.lpszClassName = c_szToolbarClass;
    wc.style     = CS_DBLCLKS | CS_GLOBALCLASS;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(PTBSTATE);
    wc.hInstance     = hInstance;   // use DLL instance if in DLL
    wc.hIcon     = NULL;
    wc.hCursor   = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc.lpszMenuName  = NULL;

    if (!RegisterClass(&wc))
        return FALSE;
    }

    return TRUE;
}
#pragma code_seg()

#define BEVEL   2
#define FRAME   1

void NEAR PASCAL PatB(HDC hdc,int x,int y,int dx,int dy, DWORD rgb)
{
    RECT    rc;

    SetBkColor(hdc,rgb);
    rc.left   = x;
    rc.top    = y;
    rc.right  = x + dx;
    rc.bottom = y + dy;

    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
}

#ifdef IEWIN31_25
// win31 doesn't support DT_END_ELLIPSIS so we append them ourselves
int MyDrawText
(
    HDC hDC,            // handle to device context 
    LPCTSTR lpString,   // pointer to string to draw 
    int nCount,         // string length, in characters 
    LPRECT lpRect,      // pointer to structure with formatting dimensions  
    UINT uFormat        // text-drawing flags 
)   
{
    SIZE sz;
    UINT cb;
    UINT cbEllipses;
    int cx;
    char szBuf[255];
    UINT i;

    Assert(lpString)
    Assert(lpRect);

    // If multiple line or non ellipses appended, use the normal DrawText.
    // (I'm lazy, localization for multiline with work break is a nightmare!)
    if (!(uFormat & DT_END_ELLIPSIS) || !(uFormat & DT_SINGLELINE))
        return DrawText(hDC, lpString, nCount, lpRect, uFormat);

    // If the string fits, again use the normal API (assumes no line break!)
    cb = lstrlen(lpString);
    GetTextExtentPoint(hDC, lpString, cb, &sz);
    cx = lpRect->right - lpRect->left;
    if (sz.cx <= cx)
    {
        return DrawText(hDC, lpString, nCount, lpRect, uFormat);
    }

    // Decrease the available space for terminating characters
    cbEllipses = lstrlen(c_szEllipses);
    GetTextExtentPoint(hDC, c_szEllipses, cbEllipses, &sz);
    cx -= sz.cx;
    if (cx <= 0)
    {
        // Not even room for termination so return
        return 0;
    }

    // See how many characters will fit
#ifdef DBCS
    // For DBCS, start at the beginnning and check for dbcs lead bytes
    if (0 == cb)
    {
        // This case should never happen because a zeor lenght string would fit!
        i = 0;
    }
    else
    {                                                
        int iFits = 0;
        
        // We know that the string doesn't fit so we can stop short of i == cb
        for (i = 1; i < cb; ++i)
        {       
            if (IsDBCSLeadByte(lpString[i-1]) && (i < (cb-1)))
            {
                ++i;
            }
            GetTextExtentPoint(hDC, lpString, i, &sz);
            if (sz.cx > cx)
            {
                // Doesn't fit
                break;
            }                         
            
            // Keep track of what fits
            iFits = i;
        } 
        
        i = iFits;
    }
    
#else
    for (i = cb; i > 0; --i)
    {
        GetTextExtentPoint(hDC, lpString, i, &sz);
        if (sz.cx <= cx)
        {
            break;
        }
    }
#endif

    // Truncate, append terminating characters, and draw
    Assert(i < sizeof(szBuf) - cbEllipses - 1);
    lstrcpyn(szBuf, lpString, i+1);     // Add 1 for the null
    lstrcat(szBuf, c_szEllipses);
    return DrawText(hDC, szBuf, i + cbEllipses, lpRect, (uFormat&~DT_END_ELLIPSIS));
}
#endif //IEWIN31_25

// Parameter fHighlight determines whether to draw text highlighted, for
// new TBSTATE_HIGHLIGHTED
//
void NEAR PASCAL DrawString(PTBSTATE ptb, HDC hdc, int x, int y, int dx, int dy, PTSTR pszString,
    BOOL fHighlight)
{
    int oldMode;
    HFONT oldhFont;
    COLORREF oldBkColor;
    COLORREF oldTextColor;
    int len;
    RECT rcText;
    UINT uiStyle = 0;

    if (!(ptb->ci.style & TBSTYLE_LIST) && ((ptb->iDyBitmap + YSLOP + g_cyEdge) >= ptb->iButHeight))
        // there's no room to show the text -- bail out
        return;

    if (fHighlight)
    {
        oldMode = SetBkMode (hdc, OPAQUE);
        oldBkColor = SetBkColor (hdc, g_clrHighlight);
        oldTextColor = SetTextColor (hdc, g_clrHighlightText);
    }
    else
        oldMode = SetBkMode(hdc, TRANSPARENT);
    oldhFont = SelectObject(hdc, ptb->hfontIcon);

    len = lstrlen(pszString);

    uiStyle = DT_END_ELLIPSIS;


    if (ptb->nTextRows > 1)
        uiStyle |= DT_WORDBREAK | DT_EDITCONTROL;
#ifdef IEWIN31_25
    else
        uiStyle |= DT_SINGLELINE;
#endif //IEWIN31_25


    if (ptb->ci.style & TBSTYLE_LIST)
    {
        uiStyle |= DT_LEFT | DT_VCENTER | DT_SINGLELINE;
        dy = max(ptb->dyIconFont, ptb->iDyBitmap);
    }
    else
    {
        uiStyle |= DT_CENTER;

        if (!dy || ptb->dyIconFont < dy)
            dy = ptb->dyIconFont;
    }

    SetRect( &rcText, x, y, x + dx, y + dy);

#ifdef WIN32
    DrawTextEx(hdc, (LPTSTR)pszString, len, &rcText, uiStyle, NULL);
#else
#ifdef IEWIN31_25
    MyDrawText(hdc, (LPTSTR)pszString, len, &rcText, uiStyle);
#else
    DrawText(hdc, (LPTSTR)pszString, len, &rcText, uiStyle);
#endif
#endif

    if (oldhFont)
        SelectObject(hdc, oldhFont);
    SetBkMode(hdc, oldMode);
    if (fHighlight)
    {
        SetBkColor (hdc, oldBkColor);
        SetTextColor (hdc, oldTextColor);
    }
}

// create a mono bitmap mask:
//   1's where color == COLOR_BTNFACE || COLOR_3DHILIGHT
//   0's everywhere else

void NEAR PASCAL CreateMask(PTBSTATE ptb, LPTBBUTTON pTBButton, int xoffset, int yoffset, int dx, int dy, BOOL fDrawGlyph)
{
    IMAGELISTDRAWPARAMS imldp;
    // initalize whole area with 1's
    PatBlt(ptb->hdcMono, 0, 0, dx, dy, WHITENESS);

    // create mask based on color bitmap
    // convert this to 1's

    if (fDrawGlyph)
    {
        imldp.cbSize = sizeof(imldp);
        imldp.himl   = ptb->himl;
        imldp.i      = pTBButton->iBitmap;
        imldp.hdcDst = ptb->hdcMono;
        imldp.x      = xoffset;
        imldp.y      = yoffset;
        imldp.cx     = 0;
        imldp.cy     = 0;
        imldp.xBitmap= 0;
        imldp.yBitmap= 0;
        imldp.rgbBk  = g_clrBtnFace;
        imldp.rgbFg  = CLR_DEFAULT;
        imldp.fStyle = ILD_ROP | ILD_MASK;
        imldp.dwRop  = SRCCOPY;

        ImageList_DrawIndirect(&imldp);

        imldp.fStyle = ILD_ROP | ILD_IMAGE;
        imldp.rgbBk  = g_clrBtnHighlight;
        imldp.dwRop  = SRCPAINT;
        ImageList_DrawIndirect(&imldp);
    }

    if (pTBButton->iString != -1 && (pTBButton->iString < ptb->nStrings))
    {
        xoffset = 1;

        if (ptb->ci.style & TBSTYLE_LIST)
        {
            xoffset += ptb->iDxBitmap + LIST_GAP;
            dx -= ptb->iDxBitmap + LIST_GAP;
        }
        else {
            yoffset += ptb->iDyBitmap + 1;
            dy -= ptb->iDyBitmap + 1;
        }

        // The FALSE in 4th param is so we don't get a box in the mask.
        DrawString(ptb, ptb->hdcMono, xoffset, yoffset, dx - g_cxEdge, dy - g_cyEdge, ptb->pStrings[pTBButton->iString],
            FALSE);
    }
}

void FAR PASCAL DrawBlankButton(HDC hdc, int x, int y, int dx, int dy, UINT state)
{
    RECT r1;

    // face color
    // The Office toolbar sends us bitmaps that are smaller than they claim they are
    // So we need to do the PatB or the window background shows through around the
    // edges of the button bitmap  -jjk
    if (!(state & TBSTATE_CHECKED))
        PatB(hdc, x, y, dx, dy, g_clrBtnFace);

    r1.left = x;
    r1.top = y;
    r1.right = x + dx;
    r1.bottom = y + dy;

    DrawEdge(hdc, &r1, (state & (TBSTATE_CHECKED | TBSTATE_PRESSED)) ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT | BF_SOFT);
}


#define DSPDxax  0x00E20746
#define PSDPxax  0x00B8074A

#define FillBkColor(hdc, prc) ExtTextOut(hdc,0,0,ETO_OPAQUE,prc,NULL,0,NULL)

void NEAR PASCAL DrawFace(PTBSTATE ptb, LPTBBUTTON ptButton, HDC hdc, int x, int y,
            int offx, int offy, int dx, int dy, UINT state)
{
    IMAGELISTDRAWPARAMS imldp;
    BOOL fHotTrack = FALSE;

    if (state & TBSTATE_ENABLED)
    {
        if ((ptb->ci.style & TBSTYLE_FLAT) && (&ptb->Buttons[ptb->nCurHTButton]==ptButton))
            fHotTrack = TRUE;

        // The following is in place to prevent hot tracking during the following conds:
        //  - drag & drop toolbar customization
        //  - when the mouse capture is on a particular button-press.
        // This does _not_ drop out of the loop because we don't want to break update
        // behavior; thus we'll have a little flickering on refresh as we pass over
        // these buttons.
        if (!(state & TBSTATE_PRESSED) && (GetKeyState (VK_LBUTTON) < 0))
            fHotTrack = FALSE;

        if (ptb->ci.style & TBSTYLE_FLAT)
        {
            UINT bdr = 0;

            if (state & (TBSTATE_CHECKED | TBSTATE_PRESSED))
                bdr = BDR_SUNKENOUTER;
            else if (fHotTrack)
                bdr = BDR_RAISEDINNER;

            if (bdr)
            {
                RECT rect;
                GetItemRect(ptb, ((DWORD) ptButton - (DWORD) ptb->Buttons) / sizeof(TBBUTTON), &rect);
                DrawEdge(hdc, &rect, bdr, BF_RECT);
            }
        }
    }

    imldp.himl = NULL;

    if (fHotTrack)
        imldp.himl   = ptb->himlHot ? ptb->himlHot : ptb->himl;
    else if (!(state & TBSTATE_ENABLED) && ptb->himlDisabled)
        imldp.himl   = ptb->himlDisabled;
    else if (ptb->himl)
        imldp.himl   = ptb->himl;

    if (imldp.himl)
    {
        imldp.cbSize = sizeof(imldp);
        imldp.i      = ptButton->iBitmap;
        imldp.hdcDst = hdc;
        imldp.x      = x + offx;
        imldp.y      = y + offy;
        imldp.cx     = 0;
        imldp.cy     = 0;
        imldp.xBitmap= 0;
        imldp.yBitmap= 0;
        imldp.rgbBk  = CLR_DEFAULT;
        imldp.rgbFg  = CLR_DEFAULT;
        imldp.fStyle = ILD_NORMAL;
        if (state & (TBSTATE_CHECKED | TBSTATE_INDETERMINATE))
            imldp.fStyle = ILD_TRANSPARENT;

#ifdef TBHIGHLIGHT_GLYPH
        if (state & TBSTATE_HIGHLIGHTED)
            imldp.fStyle = ILD_TRANSPARENT | ILD_BLEND50;
#endif
        ImageList_DrawIndirect(&imldp);
    }

    if (ptb->pStrings && (ptButton->iString >= 0) && (ptButton->iString < ptb->nStrings))
    {
        if (state & (TBSTATE_PRESSED | TBSTATE_CHECKED))
        {
            x++;
            if (ptb->ci.style & TBSTYLE_LIST)
                y++;
        }

        if (ptb->ci.style & TBSTYLE_LIST)
        {
            x += ptb->iDxBitmap + LIST_GAP;
            dx -= ptb->iDxBitmap + LIST_GAP;
        }
        else {

            y += offy + ptb->iDyBitmap;
            dy -= offy + ptb->iDyBitmap;
        }

        DrawString(ptb, hdc, x + 1, y + 1, dx - g_cxEdge, dy - g_cyEdge, ptb->pStrings[ptButton->iString],
            (state & (TBSTATE_HIGHLIGHTED)) && (ptb->ci.style & TBSTYLE_LIST));
    }
}

void FAR PASCAL DrawButton(HDC hdc, int x, int y, PTBSTATE ptb, LPTBBUTTON ptButton)
{
    int yOffset;
    HBRUSH hbrOld;
    UINT state;
    int dxFace, dyFace;
    int xCenterOffset;
    int dx = TBWidthOfButton(ptb, ptButton);
    int dy = ptb->iButHeight;
    NMCUSTOMDRAW    nmcd;
    DWORD           dwRet;
    COLORREF        clrSave = SetTextColor(hdc, g_clrBtnText);

    state = (UINT)ptButton->fsState;
    // make local copy of state and do proper overriding
    if (state & TBSTATE_INDETERMINATE) {
        if (state & TBSTATE_PRESSED)
            state &= ~TBSTATE_INDETERMINATE;
        else if (state & TBSTATE_ENABLED)
            state = TBSTATE_INDETERMINATE;
        else
            state &= ~TBSTATE_INDETERMINATE;
    }

    nmcd.hdc = hdc;
    nmcd.dwItemSpec = ptButton->idCommand;
    nmcd.uItemState = 0;

    if (state & TBSTATE_CHECKED)
        nmcd.uItemState |= CDIS_CHECKED;

    if (state & TBSTATE_PRESSED)
        nmcd.uItemState |= CDIS_SELECTED;

    if (!(state & TBSTATE_ENABLED))
        nmcd.uItemState |= CDIS_DISABLED;

    if ((ptb->ci.style & TBSTYLE_FLAT) && (&ptb->Buttons[ptb->nCurHTButton]==ptButton))
        nmcd.uItemState |= CDIS_HOT;

    nmcd.lItemlParam = 0;
    dwRet = CICustomDrawNotify(&ptb->ci, CDDS_ITEMPREPAINT, &nmcd);

    if (!(dwRet & CDRF_SKIPDEFAULT))
    {
        dxFace = ptb->iButWidth - (2 * g_cxEdge);// this the witdh of the face, not the entire button (dropdown case)
        dyFace = dy - (2 * g_cyEdge);

        if (!(ptb->ci.style & TBSTYLE_FLAT))
            DrawBlankButton(hdc, x, y, dx, dy, state);

        // move coordinates inside border and away from upper left highlight.
        // the extents change accordingly.
        x += g_cxEdge;
        y += g_cyEdge;

        // calculate offset of face from (x,y).  y is always from the top,
        // so the offset is easy.  x needs to be centered in face.
        yOffset = 1;
        if (ptb->ci.style & TBSTYLE_LIST)
             xCenterOffset = XSLOP / 2;
        else
             xCenterOffset = (dxFace - ptb->iDxBitmap)/2;


        if (state & (TBSTATE_PRESSED | TBSTATE_CHECKED))
        {
        // pressed state moves down and to the right
            xCenterOffset++;
            yOffset++;
        }


        // draw the dithered background
        if ((state & (TBSTATE_CHECKED | TBSTATE_INDETERMINATE)) || ((state & TBSTATE_HIGHLIGHTED)
            && !(ptb->ci.style & TBSTYLE_FLAT)))
        {
            hbrOld = SelectObject(hdc, g_hbrMonoDither);
            if (hbrOld)
            {
                COLORREF clrText, clrBack;
#ifdef TBHIGHLIGHT_BACK
                if (state & TBSTATE_HIGHLIGHTED)
                    clrText = SetTextColor(hdc, g_clrHighlight);
                else
#endif
                    clrText = SetTextColor(hdc, g_clrBtnHighlight); // 0 -> 0
                clrBack = SetBkColor(hdc, g_clrBtnFace);        // 1 -> 1

                // only draw the dither brush where the mask is 1's
                PatBlt(hdc, x, y, dxFace, dyFace, PATCOPY);

                SelectObject(hdc, hbrOld);
                SetTextColor(hdc, clrText);
                SetBkColor(hdc, clrBack);
            }
        }

        // now put on the face
        // TODO: Validate himlDisabled and ensure that the index is in range
        if ((state & TBSTATE_ENABLED) || ptb->himlDisabled)
        {
            // regular version
            DrawFace(ptb, ptButton, hdc, x, y, xCenterOffset, yOffset, dxFace, dyFace, state);
        }

        if (!(state & TBSTATE_ENABLED))
        {
            HBITMAP hbmOld;

            //initialize the monochrome dc
            if (!ptb->hdcMono) {
                ptb->hdcMono = CreateCompatibleDC(hdc);
                if (!ptb->hdcMono)
                    return;
                SetTextColor(ptb->hdcMono, 0L);
            }

            hbmOld = SelectObject(ptb->hdcMono, ptb->hbmMono);


            // disabled version (or indeterminate)
            CreateMask(ptb, ptButton, xCenterOffset, yOffset, dxFace, dyFace, (ptb->himlDisabled == NULL));

            SetTextColor(hdc, 0L);   // 0's in mono -> 0 (for ROP)
            SetBkColor(hdc, 0x00FFFFFF); // 1's in mono -> 1

            // draw glyph's white understrike
            if (!(state & TBSTATE_INDETERMINATE)) {
                hbrOld = SelectObject(hdc, g_hbrBtnHighlight);
                if (hbrOld) {
                    // draw hilight color where we have 0's in the mask
                    BitBlt(hdc, x + 1, y + 1, dxFace, dyFace, ptb->hdcMono, 0, 0, PSDPxax);
                    SelectObject(hdc, hbrOld);
                }
            }

            // gray out glyph
            hbrOld = SelectObject(hdc, g_hbrBtnShadow);
            if (hbrOld) {
                // draw the shadow color where we have 0's in the mask
                BitBlt(hdc, x, y, dxFace, dyFace, ptb->hdcMono, 0, 0, PSDPxax);
                SelectObject(hdc, hbrOld);
            }

            if (state & TBSTATE_CHECKED) {
                BitBlt(ptb->hdcMono, 1, 1, dxFace - 1, dyFace - 1, ptb->hdcMono, 0, 0, SRCAND);
            }

            SelectObject(ptb->hdcMono, hbmOld);
        }

        if ((ptButton->fsStyle & TBSTYLE_DROPDOWN) && !(ptb->ci.style & TBSTYLE_FLAT))
        {
            RECT rc;
            POINT pts[3];
            int iHeight;
            int iWidth;
            HBRUSH hbr;
            HPEN hpen;

            rc.left = x - (2*g_cxEdge) + ptb->iButWidth + (yOffset-1);
            rc.top = y + g_cyBorder;
            rc.bottom = y + dy - (2 * g_cyEdge) - (2*g_cyBorder);
            rc.right = rc.left - ptb->iButWidth + dx;

            DrawEdge(hdc, &rc, EDGE_ETCHED, BF_LEFT);

            rc.left += g_cxEdge;
            rc.right -= g_cxEdge;
            rc.top += yOffset;
            iWidth = RECTWIDTH(rc);
            iWidth -= g_cxEdge;  // make it a little smaller than the rect
            if (iWidth < 3) iWidth = 3;
            iWidth &= (~1); // make it even
            iHeight = (iWidth) / 2;


            pts[0].y = pts[1].y = (RECTHEIGHT(rc) - iHeight + 1)/2 + rc.top; // +1 to bias lower
            pts[2].y = pts[0].y + iHeight;

            pts[0].x = (RECTWIDTH(rc) - iWidth + 1) / 2 + rc.left; // +1 to bias right
            pts[1].x = pts[0].x + iWidth;
            pts[2].x = pts[0].x + iWidth/2;

            hbr = GetStockObject(BLACK_BRUSH);
            hpen = GetStockObject(BLACK_PEN);
            hbr = SelectObject(hdc, hbr);
            hpen = SelectObject(hdc, hpen);
            Polygon(hdc, pts, 3);

            SelectObject(hdc, hbr);
            SelectObject(hdc, hpen);

        }
    }

    if (dwRet & CDRF_NOTIFYPOSTPAINT)
        CICustomDrawNotify(&ptb->ci, CDDS_ITEMPOSTPAINT, &nmcd);

    SetTextColor(hdc, clrSave);
}

// make sure that g_hbmMono is big enough to do masks for this
// size of button.  if not, fail.
BOOL NEAR PASCAL CheckMonoMask(PTBSTATE ptb, int width, int height)
{
    BITMAP bm;
    HBITMAP hbmTemp;

    if (ptb->hbmMono) {
        GetObject(ptb->hbmMono, sizeof(BITMAP), &bm);
        if (width <= bm.bmWidth && height <= bm.bmHeight) {
            return TRUE;
        }
    }


    hbmTemp = CreateMonoBitmap(width, height);
    if (!hbmTemp)
        return FALSE;

    if (ptb->hbmMono)
        DeleteObject(ptb->hbmMono);
    ptb->hbmMono = hbmTemp;
    return TRUE;
}

/*
** GrowToolbar
**
** Attempt to grow the button size.
**
** The calling function can either specify a new internal measurement
** or a new external measurement.
*/
BOOL NEAR PASCAL GrowToolbar(PTBSTATE ptb, int newButWidth, int newButHeight, BOOL bInside)
{
    if (!newButWidth)
        newButWidth = DEFAULTBUTTONX;
    if (!newButHeight)
        newButHeight = DEFAULTBUTTONY;

    // if growing based on inside measurement, get full size
    if (bInside)
    {
        if (ptb->ci.style & TBSTYLE_LIST)
            newButWidth += ptb->iDxBitmap + LIST_GAP;

        newButHeight += YSLOP;
        newButWidth += XSLOP;

        // if toolbar already has strings, don't shrink width it because it
        // might clip room for the string
        if ((newButWidth < ptb->iButWidth) && ptb->nStrings)
            newButWidth = ptb->iButWidth;
    }
    else {
        if (newButHeight == -1)
            newButHeight = ptb->iButHeight;
        if (newButWidth == -1)
            newButWidth = ptb->iButWidth;

        if (newButHeight < ptb->iDyBitmap + YSLOP)
            newButHeight = ptb->iDyBitmap + YSLOP;
        if (newButWidth < ptb->iDxBitmap + XSLOP)
            newButWidth = ptb->iDxBitmap + XSLOP;
    }

    // if the size of the toolbar is actually growing, see if shadow
    // bitmaps can be made sufficiently large.
    if (!ptb->hbmMono || (newButWidth > ptb->iButWidth) || (newButHeight > ptb->iButHeight)) {
    if (!CheckMonoMask(ptb, newButWidth, newButHeight))
        return(FALSE);
    }

    if (!bInside && (ptb->iButWidth != newButWidth) || (ptb->iButHeight != newButHeight))
        InvalidateRect(ptb->ci.hwnd, NULL, TRUE);

    ptb->iButWidth = newButWidth;
    ptb->iButHeight = newButHeight;

    // bar height has 2 pixels above, 2 below
    if (ptb->ci.style & TBSTYLE_FLAT)
        ptb->iYPos = 0;
    else
        ptb->iYPos = 2;

    FlushToolTipsMgr(ptb);

    return TRUE;
}

BOOL NEAR PASCAL SetBitmapSize(PTBSTATE ptb, int width, int height)
{
    int realh;

    if (!width)
        width = 1;
    if (!height)
        height = 1;

    if (width == -1)
        width = ptb->iDxBitmap;

    if (height == -1)
        height = ptb->iDyBitmap;

    realh = height;

    if ((ptb->iDxBitmap == width) && (ptb->iDyBitmap == height))
        return TRUE;

    if (ptb->nStrings)
        realh = HeightWithString(ptb, height);

    if (GrowToolbar(ptb, width, realh, TRUE)) {
        ptb->iDxBitmap = width;
        ptb->iDyBitmap = height;

        // the size changed, we need to rebuild the imagelist
        TBInvalidateImageList(ptb);
        return TRUE;
    }
    return FALSE;
}

void NEAR PASCAL TB_OnSysColorChange(PTBSTATE ptb)
{

    ReInitGlobalColors();
    //  Reset all of the bitmaps
    if (ptb->himl)
        ImageList_SetBkColor(ptb->himl, (ptb->ci.style & TBSTYLE_FLAT) ? CLR_NONE : g_clrBtnFace);
    if (ptb->himlHot)
        ImageList_SetBkColor(ptb->himlHot, (ptb->ci.style & TBSTYLE_FLAT) ? CLR_NONE : g_clrBtnFace);
}

#define CACHE 0x01
#define BUILD 0x02


void PASCAL ReleaseMonoDC(PTBSTATE ptb)
{
    if (ptb->hdcMono) {
        DeleteDC(ptb->hdcMono);
        ptb->hdcMono = NULL;
    }
}

void TB_OnEraseBkgnd(PTBSTATE ptb, HDC hdc)
{
    NMCUSTOMDRAW    nmcd;
    DWORD           dwRes = FALSE;

    nmcd.hdc = hdc;
    nmcd.uItemState = 0;
    nmcd.lItemlParam = 0;

    if (ptb->ci.style & TBSTYLE_CUSTOMERASE) {
        ptb->ci.dwCustom = CICustomDrawNotify(&ptb->ci, CDDS_PREERASE, &nmcd);

    } else {
        ptb->ci.dwCustom = CDRF_DODEFAULT;
    }

    if (!(ptb->ci.dwCustom & CDRF_SKIPDEFAULT))
    {
        // for transparent toolbars, forward erase background to parent
        // but handle thru DefWindowProc in the event parent doesn't paint
        if (!(ptb->ci.style & TBSTYLE_FLAT) ||
            !CCForwardEraseBackground(ptb->ci.hwnd, hdc))
            DefWindowProc(ptb->ci.hwnd, WM_ERASEBKGND, (WPARAM) hdc, 0);
    }

    if (ptb->ci.dwCustom & CDRF_NOTIFYPOSTERASE)
        CICustomDrawNotify(&ptb->ci, CDDS_POSTERASE, &nmcd);
}

void NEAR PASCAL ToolbarPaint(PTBSTATE ptb, HDC hdcIn)
{
    RECT rc;
    HDC hdc;
    PAINTSTRUCT ps;
    int iButton, xButton, yButton, cxBar;
    PTBBUTTON pAllButtons = ptb->Buttons;
    NMCUSTOMDRAW    nmcd;

    GetClientRect(ptb->ci.hwnd, &rc);

    cxBar = rc.right - rc.left;

    if (hdcIn)
    {
        hdc = hdcIn;
    }
    else
        hdc = BeginPaint(ptb->ci.hwnd, &ps);

    if (!rc.right)
        goto Error1;

    //
    // NT 3.x sometimes doesn't initialize the origin properly. Dithering the
    // viewport origin fixes things up.  What a hack! (stevepro, Oct 14/96)
    // (This was fixed in NT 4.0)
    //
    {
        SetViewportOrg(hdc, 1, 1);
        SetViewportOrg(hdc, 0, 0);
    }

    nmcd.hdc = hdc;
    nmcd.uItemState = 0;
    nmcd.lItemlParam = 0;

    ptb->ci.dwCustom = CICustomDrawNotify(&ptb->ci, CDDS_PREPAINT, &nmcd);

    if (!(ptb->ci.dwCustom & CDRF_SKIPDEFAULT))
    {
        if (!ptb->fHimlValid)
            TBBuildImageList(ptb);

        yButton   = ptb->iYPos;
        rc.top    = ptb->iYPos;
        rc.bottom = ptb->iYPos + ptb->iButHeight;


        for (iButton = 0, xButton = ptb->xFirstButton;
                iButton < ptb->iNumButtons; iButton++)
        {
            PTBBUTTON pButton = &pAllButtons[iButton];

            if (!(pButton->fsState & TBSTATE_HIDDEN))
            {
                int cxButton = TBWidthOfButton(ptb, pButton);

                if (!(pButton->fsStyle & TBSTYLE_SEP) || (ptb->ci.style & TBSTYLE_FLAT))
                {
                    // is there anything to draw?
                    rc.left = xButton;
                    rc.right = xButton + cxButton;

                    if (RectVisible(hdc, &rc))
                    {
                        if (pButton->fsStyle & TBSTYLE_SEP)
                        {
                            // must be a flat separator
                            if (ptb->ci.style & CCS_VERT)
                            {
                                int iSave = rc.top;
                                rc.top += ((rc.bottom - rc.top) - 1) / 2;
                                rc.top = iSave;
                                InflateRect(&rc, -g_cxEdge, 0);
                                DrawEdge(hdc, &rc, EDGE_ETCHED, BF_TOP);
                                InflateRect(&rc, g_cxEdge, 0);
                            }
                            else
                            {
                                rc.left += (cxButton - 1) / 2;
                                InflateRect(&rc, 0, -g_cyEdge);
                                DrawEdge(hdc, &rc, EDGE_ETCHED, BF_LEFT);
                                InflateRect(&rc, 0, g_cyEdge);
                            }
                        }
                        else
                            DrawButton(hdc, xButton, yButton, ptb, pButton);
                    }
                }

                xButton += (cxButton - s_dxOverlap);

                if (pButton->fsState & TBSTATE_WRAP)
                {
                    int dy;

                    if (pButton->fsStyle & TBSTYLE_SEP)
                    {
                        if (ptb->ci.style & TBSTYLE_FLAT)
                        {
                            RECT rcMid;

                            rcMid.top = rc.top + ptb->iButHeight + ((pButton->iBitmap - 1) / 2);
                            rcMid.bottom = rcMid.top + g_cxEdge;
                            rcMid.left = g_cxEdge;
                            rcMid.right = cxBar - g_cxEdge;
                            DrawEdge(hdc, &rcMid, EDGE_ETCHED, BF_TOP);
                            dy = ptb->iButHeight + pButton->iBitmap;
                        }
                        else
                            dy = ptb->iButHeight + pButton->iBitmap * 2 / 3;
                    }
                    else
                        dy = ptb->iButHeight;

                    xButton = ptb->xFirstButton;
                    yButton   += dy;
                    rc.top    += dy;
                    rc.bottom += dy;
                }
            }
        }
        ReleaseMonoDC(ptb);
    }

    if (ptb->ci.dwCustom & CDRF_NOTIFYPOSTPAINT)
    {
        nmcd.hdc = hdc;
        nmcd.uItemState = 0;
        nmcd.lItemlParam = 0;
        CICustomDrawNotify(&ptb->ci, CDDS_POSTPAINT, &nmcd);
    }

Error1:
    if (hdcIn == NULL)
        EndPaint(ptb->ci.hwnd, &ps);

}


BOOL NEAR PASCAL GetItemRect(PTBSTATE ptb, UINT uButton, LPRECT lpRect)
{
    UINT iButton, xPos, yPos;
    PTBBUTTON pButton;

    if (uButton >= (UINT)ptb->iNumButtons
        || (ptb->Buttons[uButton].fsState & TBSTATE_HIDDEN))
    {
        return FALSE;
    }

    xPos = ptb->xFirstButton;
    yPos = ptb->iYPos;

    for (iButton = 0, pButton = ptb->Buttons; iButton < uButton; iButton++, pButton++)
    {
        if (!(pButton->fsState & TBSTATE_HIDDEN))
        {
            xPos += TBWidthOfButton(ptb, pButton) - s_dxOverlap;

            if (pButton->fsState & TBSTATE_WRAP)
            {
                    yPos += ptb->iButHeight;
                    if (pButton->fsStyle & TBSTYLE_SEP)
                    {
                        if (ptb->ci.style & TBSTYLE_FLAT)
                            yPos += pButton->iBitmap;
                        else
                            yPos += pButton->iBitmap * 2 / 3;
                    }
                    xPos = ptb->xFirstButton;
            }
        }
    }

    // pButton should now point at the required button, and xPos should be
    // its left edge.  Note that we already checked if the button was hidden above

    lpRect->left   = xPos;
    lpRect->right  = xPos + TBWidthOfButton(ptb, pButton);
    lpRect->top    = yPos;
    lpRect->bottom = yPos + ptb->iButHeight;

    return TRUE;
}


void NEAR PASCAL InvalidateButton(PTBSTATE ptb, PTBBUTTON pButtonToPaint, BOOL fErase)
{
    RECT rc;

    if (GetItemRect(ptb, pButtonToPaint-ptb->Buttons, &rc))
    {
        InvalidateRect(ptb->ci.hwnd, &rc, fErase);
    }
}

// do hit testing by sliding the origin of the supplied point
//
// returns:
//  >= 0    index of non sperator item hit
//  < 0 index of seperator or nearest non seperator item (area just below and to the left)
//
// +--------------------------------------
// |      -1    -1    -1    -1
// |      btn   sep   btn
// |    +-----+     +-----+
// |    |     |     |     |
// | -1 |  0  | -1  |  2  | -3
// |    |     |     |     |
// |    +-----+     +-----+
// |
// | -1   -1    -1    -2    -3
//

int FAR PASCAL TBHitTest(PTBSTATE ptb, int xPos, int yPos)
{
    int prev = 0;
    int last = 0;
    int i;
    RECT rc;

    if (ptb->iNumButtons == 0)
        return(-1);

    for (i=0; i<ptb->iNumButtons; i++)
    {
        // BUGBUG.. this makes it n**2
        if (GetItemRect(ptb, i, &rc))
        {
            if (yPos >= rc.top && yPos <= rc.bottom)
            {
                if (xPos >= rc.left && xPos <= rc.right)
                {
                    if (ptb->Buttons[i].fsStyle & TBSTYLE_SEP)
                        return - i - 1;
                    else
                        return i;
                }
                else
                {
                    prev = i + 1;
                }
             }
             else
             {
                last = i;
             }
        }
    }

    if (prev)
            return -1 - prev;
    else if (yPos > rc.bottom)
        // this means that we are off the bottom of the toolbar
        return(- i - 1);

    return last + 1;
}

int NEAR PASCAL CountRows(PTBSTATE ptb)
{
    PTBBUTTON pButton, pBtnLast;
    int rows = 1;

    pBtnLast = &(ptb->Buttons[ptb->iNumButtons]);
    for (pButton = ptb->Buttons; pButton<pBtnLast; pButton++) {
        if (pButton->fsState & TBSTATE_WRAP) {
            rows++;
            if (pButton->fsStyle & TBSTYLE_SEP)
                rows++;
        }
    }

    return rows;
}


/**** WrapToolbar:
 * The buttons in the toolbar is layed out from left to right,
 * top to bottom. If adding another button to the current row,
 * while computing the layout, would cause that button to extend
 * beyond the right edge or the client area, then locate a break-
 * point (marked with the TBSTATE_WRAP flag). A break-point is:
 *
 * a) The right-most separator on the current row.
 *
 * b) The right-most button if there is no separator on the current row.
 *
 * A new row is also started at the end of any button group (sequence
 * of buttons that are dlimited by separators) that are taller than
 * or equal to two rows.
 */

void NEAR PASCAL WrapToolbar(PTBSTATE ptb, int dx, LPRECT lpRect, int FAR *pRows)
{
    PTBBUTTON pButton, pBtnT, pBtnLast;
    int xPos, yPos, xMax;
    BOOL bFoundIt;
    BOOL bNextBreak = FALSE;

    xMax = ptb->iButWidth;
    xPos = ptb->xFirstButton;
    yPos = ptb->iYPos;
    pBtnLast = &(ptb->Buttons[ptb->iNumButtons]);


    if (pRows)
        (*pRows)=1;

    for (pButton = ptb->Buttons; pButton<pBtnLast; pButton++)
    {
        pButton->fsState &= ~TBSTATE_WRAP;

        if (!(pButton->fsState & TBSTATE_HIDDEN))
        {
            xPos += TBWidthOfButton(ptb, pButton) - s_dxOverlap;

            // The current row exceeds the right edge. Wrap it.
            if (!(pButton->fsStyle&TBSTYLE_SEP) && (xPos > dx)) {

                for (pBtnT=pButton, bFoundIt = FALSE;
                     pBtnT>ptb->Buttons && !(pBtnT->fsState & TBSTATE_WRAP);
                     pBtnT--)
                {
                    if ((pBtnT->fsStyle & TBSTYLE_SEP) &&
                        !(pBtnT->fsState & TBSTATE_HIDDEN))
                    {
                        pBtnT->fsState |= TBSTATE_WRAP;
                        xPos = ptb->xFirstButton;
                        if (ptb->ci.style & TBSTYLE_FLAT)
                            yPos += pBtnT->iBitmap + ptb->iButHeight;
                        else
                            yPos += pBtnT->iBitmap * 2 / 3 + ptb->iButHeight;
                        bFoundIt = TRUE;
                        pButton = pBtnT;
                        bNextBreak = FALSE;
                        if (pRows)
                            (*pRows)++;
                        break;
                    }
                }

                // Did we find a separator? Force a wrap anyway!
                if (bFoundIt==FALSE)
                {
                    pBtnT = pButton;
                    if (pButton!=ptb->Buttons) {
                        /* Back-up to first non-hidden button. */
                        do {
                           pBtnT--;
                           } while ((pBtnT>ptb->Buttons) &&
                                    (pBtnT->fsState & TBSTATE_HIDDEN));

                        /* Is it already wrapped? */
                        if (pBtnT->fsState & TBSTATE_WRAP)
                            pBtnT = pButton;
                    }

                    pBtnT->fsState |= TBSTATE_WRAP;
                    xPos = ptb->xFirstButton;
                    yPos += ptb->iButHeight;
                    pButton = pBtnT;
                    bNextBreak = TRUE;
                }

        // Count another row.
        if (pRows)
            (*pRows)++;
            }
            else
            {
                pButton->fsState &= ~TBSTATE_WRAP;
                if ((pButton->fsStyle&TBSTYLE_SEP) && (bNextBreak))
                {
                        bNextBreak = FALSE;
                        pButton->fsState |= TBSTATE_WRAP;
                        xPos = ptb->xFirstButton;
                        if (ptb->ci.style & TBSTYLE_FLAT)
                            yPos += ptb->iButHeight + pButton->iBitmap;
                        else
                            yPos += ptb->iButHeight + pButton->iBitmap * 2 / 3 ;
                        if (pRows)
                                (*pRows)+=2;
                }
            }
            if (!(pButton->fsStyle&TBSTYLE_SEP))
                xMax = max(xPos, xMax);
        }
    }

    if (lpRect)
    {
        lpRect->left = 0;
    lpRect->right = xMax;
    lpRect->top = 0;
    lpRect->bottom = yPos + ptb->iYPos + ptb->iButHeight;
    }
}



BOOL NEAR PASCAL BoxIt(PTBSTATE ptb, int height, BOOL fLarger, LPRECT lpRect)
{
    int dx, bwidth;
    int rows, prevRows, prevWidth;
    RECT rcCur;

    if (height<1)
        height = 1;

    rows = CountRows(ptb);
    if (height==rows || ptb->iNumButtons==0)
    {
        GetClientRect(ptb->ci.hwnd, lpRect);
        return FALSE;
    }

    bwidth = ptb->iButWidth-s_dxOverlap;
    prevRows = ptb->iNumButtons+1;
        prevWidth = bwidth;
    for (rows=height+1, dx = bwidth; rows>height;dx+=bwidth/4)
    {
        WrapToolbar(ptb, dx, &rcCur, &rows);
        if (rows<prevRows && rows>height)
        {
            prevWidth = dx;
            prevRows = rows;
        }
    }

    if (rows<height && fLarger)
    {
        WrapToolbar(ptb, prevWidth, &rcCur, NULL);
    }

    if (lpRect)
        *lpRect = rcCur;

        return TRUE;
}


int FAR PASCAL PositionFromID(PTBSTATE ptb, int id)
{
    int i;

    // Handle case where this is sent at the wrong time..
    if (ptb == NULL)
        return -1;

    // note, we don't skip seperators, so you better not have conflicting
    // cmd ids and seperator ids.
    for (i = 0; i < ptb->iNumButtons; i++)
        if (ptb->Buttons[i].idCommand == id)
            return i;       // position found

    return -1;      // ID not found!
}

// check a radio button by button index.
// the button matching idCommand was just pressed down.  this forces
// up all other buttons in the group.
// this does not work with buttons that are forced up with

void NEAR PASCAL MakeGroupConsistant(PTBSTATE ptb, int idCommand)
{
    int i, iFirst, iLast, iButton;
    int cButtons = ptb->iNumButtons;
    PTBBUTTON pAllButtons = ptb->Buttons;

    iButton = PositionFromID(ptb, idCommand);

    if (iButton < 0)
        return;

    // assertion

//    if (!(pAllButtons[iButton].fsStyle & TBSTYLE_CHECK))
//  return;

    // did the pressed button just go down?
    if (!(pAllButtons[iButton].fsState & TBSTATE_CHECKED))
        return;         // no, can't do anything

    // find the limits of this radio group

    for (iFirst = iButton; (iFirst > 0) && (pAllButtons[iFirst].fsStyle & TBSTYLE_GROUP); iFirst--)
        if (!(pAllButtons[iFirst].fsStyle & TBSTYLE_GROUP))
            iFirst++;

    cButtons--;
    for (iLast = iButton; (iLast < cButtons) && (pAllButtons[iLast].fsStyle & TBSTYLE_GROUP); iLast++);

    if (!(pAllButtons[iLast].fsStyle & TBSTYLE_GROUP))
        iLast--;

    // search for the currently down button and pop it up
    for (i = iFirst; i <= iLast; i++) {
        if (i != iButton) {
            // is this button down?
            if (pAllButtons[i].fsState & TBSTATE_CHECKED) {
                pAllButtons[i].fsState &= ~TBSTATE_CHECKED;     // pop it up
                InvalidateButton(ptb, &pAllButtons[i], TRUE);
                break;          // only one button is down right?
            }
        }
    }
}

void NEAR PASCAL DestroyStrings(PTBSTATE ptb)
{
    PTSTR *p;
    PTSTR end = 0, start = 0;
    int i;

    p = ptb->pStrings;
    for (i = 0; i < ptb->nStrings; i++) {
        if (!((*p < end) && (*p > start))) {
            start = (*p);
            end = start + (LocalSize((HANDLE)*p) / sizeof(TCHAR));
            LocalFree((HANDLE)*p);
        }
    p++;
    }

    LocalFree((HANDLE)ptb->pStrings);
}

#define MAXSTRINGSIZE 1024
int NEAR PASCAL AddStrings(PTBSTATE ptb, WPARAM wParam, LPARAM lParam)
{
    int i = 0,j = 0, cxMax = 0;
    LPTSTR lpsz;
    PTSTR  pString, psz;
    int numstr;
    PTSTR *pFoo;
    PTSTR *pOffset;
    TCHAR cSeparator;
    int len;

    // read the string as a resource
    if (wParam != 0) {
        pString = (PTSTR)LocalAlloc(LPTR, (MAXSTRINGSIZE * sizeof (TCHAR)));
        if (!pString)
            return -1;
        i = LoadString((HINSTANCE)wParam, LOWORD(lParam), (LPTSTR)pString, MAXSTRINGSIZE);
        if (!i) {
            LocalFree(pString);
            return -1;
    }
    // realloc string buffer to actual needed size
    LocalReAlloc(pString, (i+1) * sizeof (TCHAR), LMEM_MOVEABLE);

    // convert separators to '\0' and count number of strings
    cSeparator = *pString;
#if !defined(UNICODE) // && defined(DBCS)
    for (numstr = 0, psz = pString + 1, i--; i; i--, psz = AnsiNext(psz)) {
        {
            // extra i-- if DBCS
            if (AnsiPrev(pString, psz)==(psz-2))
                i--;
            if (*psz == cSeparator) {
                if (i != 1)     // We don't want to count the second terminator as another string
                numstr++;

                *psz = 0;   // terminate with 0
            }
        }
        // shift string to the left to overwrite separator identifier
        *(psz - 1) = *psz;
    }
#else
    for (numstr = 0, psz = pString + 1, i--; i; i--, psz++) {
        if (*psz == cSeparator) {
                if (i != 1)     // We don't want to count the second terminator as another string
                    numstr++;

        *psz = 0;   // terminate with 0
        }
        // shift string to the left to overwrite separator identifier
        *(psz - 1) = *psz;
    }
#endif
    }
    // read explicit string.  copy it into local memory, too.
    else {
        // find total length and number of strings
        for (i = 0, numstr = 0, lpsz = (LPTSTR)lParam;;) {
                i++;
                if (*lpsz == 0) {
                numstr++;
                if (*(lpsz + 1) == 0)
                    break;
            }
            lpsz++;
        }
        pString = (PTSTR)LocalAlloc(LPTR, (i * sizeof (TCHAR)));
        if (!pString)
            return -1;
        hmemcpy(pString, (void FAR *)lParam, i * sizeof(TCHAR));
    }

    // make room for increased string pointer table
    if (ptb->pStrings)
        pFoo = (PTSTR *)LocalReAlloc(ptb->pStrings,
            (ptb->nStrings + numstr) * sizeof(PTSTR), LMEM_MOVEABLE);
    else
        pFoo = (PTSTR *)LocalAlloc(LPTR, numstr * sizeof(PTSTR));
    if (!pFoo) {
        LocalFree(pString);
        return -1;
    }

//    hdc = GetDC(ptb->ci.hwnd);
//    if (!hdc)
//        return 1;

    ptb->pStrings = pFoo;
    // pointer to next open slot in string index table.
    pOffset = ptb->pStrings + ptb->nStrings;

    for (i = 0; i < numstr; i++, pOffset++)
    {
        *pOffset = pString;
        len = lstrlen(pString);
        pString += len + 1;
    }
    // is the world big enough to handle the larger buttons?
    if (!TBRecalc(ptb))
    {
        // back out changes.
        if (ptb->nStrings == 0) {
            LocalFree(ptb->pStrings);
            ptb->pStrings = 0;
        }
        else
            ptb->pStrings = (PTSTR *)LocalReAlloc(ptb->pStrings,
                    ptb->nStrings * sizeof(PTSTR), LMEM_MOVEABLE);
        LocalFree(pString);
        return -1;
    }

    i = ptb->nStrings;
    ptb->nStrings += numstr;
    return i;               // index of first added string
}

#ifdef WIN32
void NEAR PASCAL MapToStandardBitmaps(HINSTANCE FAR *phinst, UINT FAR * pidBM, int FAR *pnButtons)
{
    if (*phinst == HINST_COMMCTRL) {
        *phinst = g_hinst;

        // low 2 bits are coded M(mono == ~color) L(large == ~small)
        //  0 0   -> color small
        //  0 1   -> color large
        //  ...
        //  1 1   -> mono  large

        switch (*pidBM)
        {
        case IDB_STD_SMALL_COLOR:
        case IDB_STD_LARGE_COLOR:
        case IDB_STD_SMALL_MONO:
        case IDB_STD_LARGE_MONO:
            *pidBM = IDB_STDTB_SMALL_COLOR + (*pidBM & 1);
            *pnButtons = STD_PRINT + 1;
            break;

        case IDB_HIST_SMALL_COLOR:
        case IDB_HIST_LARGE_COLOR:
        //case IDB_HIST_SMALL_MONO:
        //case IDB_HIST_LARGE_MONO:
            *pidBM = IDB_HISTTB_SMALL_COLOR + (*pidBM & 1);
            *pnButtons = HIST_LAST + 1;
            break;

        case IDB_VIEW_SMALL_COLOR:
        case IDB_VIEW_LARGE_COLOR:
        case IDB_VIEW_SMALL_MONO:
        case IDB_VIEW_LARGE_MONO:
            *pidBM = IDB_VIEWTB_SMALL_COLOR + (*pidBM & 1);
            *pnButtons = VIEW_NEWFOLDER + 1;
            break;
        }
    }
}
#endif

HBITMAP _CopyBitmap(PTBSTATE ptb, HBITMAP hbm, int cx, int cy)
{
    HBITMAP hbmCopy;
    RECT rc = {0 ,0, cx, cy};
    HDC hdcWin;
    HDC hdcSrc, hdcDest;

    hbmCopy = CreateColorBitmap(cx, cy);

    hdcWin = GetDC(ptb->ci.hwnd);
    hdcSrc = CreateCompatibleDC(hdcWin);
    hdcDest = CreateCompatibleDC(hdcWin);

    if (hdcWin && hdcSrc && hdcDest) {

        SelectObject(hdcSrc, hbm);
        SelectObject(hdcDest, hbmCopy);

        // fill the background
        PatB(hdcDest, 0, 0, cx, cy, g_clrBtnFace);

        BitBlt(hdcDest, 0, 0, cx, cy,
               hdcSrc, 0, 0, SRCCOPY);

    }

    if (hdcWin)
        ReleaseDC(ptb->ci.hwnd, hdcWin);

    if (hdcSrc)
        DeleteDC(hdcSrc);
    if (hdcDest)
        DeleteDC(hdcDest);
    return hbmCopy;
}


BOOL TBAddBitmapToImageList(PTBSTATE ptb, PTBBMINFO pTemp)
{
    HBITMAP hbm = NULL, hbmTemp = NULL;

    if (!ptb->himl) {
        ptb->himl = ImageList_Create(ptb->iDxBitmap, ptb->iDyBitmap, ILC_MASK | ILC_COLORDDB, 4, 4);
        if (!ptb->himl)
            return(FALSE);
        ImageList_SetBkColor(ptb->himl, (ptb->ci.style & TBSTYLE_FLAT) ? CLR_NONE : g_clrBtnFace);
    }


    if (pTemp->hInst) {
        hbm = hbmTemp = CreateMappedBitmap(pTemp->hInst, pTemp->wID, 0, NULL, 0);

    } else if (pTemp->wID) {
        hbm = (HBITMAP)pTemp->wID;
    }

    if (hbm) {

        //
        // Fix up bitmaps that aren't iDxBitmap x iDyBitmap
        //

        BITMAP bm;
        int    iMod;

        GetObject( hbm, sizeof(bm), &bm);
        if (bm.bmWidth < ptb->iDxBitmap) {
            bm.bmWidth = ptb->iDxBitmap;
        }
        if (bm.bmHeight < ptb->iDyBitmap) {
            bm.bmHeight = ptb->iDyBitmap;
        }

        iMod = bm.bmWidth % ptb->iDxBitmap;

        if ( iMod != 0) {
            bm.bmWidth += iMod;
        }

        //
        // do this copybitmap rather than CopyImage because we
        // don't want the bits stretched...  we want them back filled with
        // button face.
        //

        hbm = (HBITMAP)_CopyBitmap(ptb, hbm, bm.bmWidth, bm.bmHeight);
    }

    // AddMasked parties on the bitmap, so we want to use a local copy
    if (hbm) {
        ImageList_AddMasked(ptb->himl, hbm, g_clrBtnFace);
        DeleteObject(hbm);
    }

    if (hbmTemp) {
        DeleteObject(hbmTemp);
    }

    return(TRUE);

}

void NEAR PASCAL TBBuildImageList(PTBSTATE ptb)
{
    int i;
    PTBBMINFO pTemp;

    ptb->fHimlValid = TRUE;

    // is the parent dealing natively with imagelists?  if so,
    // don't do this back compat building
    if (ptb->fHimlNative)
        return;

    if (ptb->himl) {
        ImageList_Destroy(ptb->himl);
        ptb->himl = NULL;
    }

    for (i = 0, pTemp = ptb->pBitmaps; i < ptb->nBitmaps; i++, pTemp++) {

        TBAddBitmapToImageList(ptb, pTemp);
    }

}

/* Adds a new bitmap to the list of BMs available for this toolbar.
 * Returns the index of the first button in the bitmap or -1 if there
 * was an error.
 */
int NEAR PASCAL AddBitmap(PTBSTATE ptb, int nButtons, HINSTANCE hBMInst, UINT idBM)
{
    PTBBMINFO pTemp;
    int nBM, nIndex;

    // map things to the standard toolbar images
#ifdef WIN32
    if (hBMInst == HINST_COMMCTRL)        // -1
    {
        // set the proper dimensions...
        if (idBM & 1)
            SetBitmapSize(ptb, LARGE_DXYBITMAP, LARGE_DXYBITMAP);
        else
            SetBitmapSize(ptb, SMALL_DXYBITMAP, SMALL_DXYBITMAP);

        MapToStandardBitmaps(&hBMInst, &idBM, &nButtons);
    }
#endif

    if (ptb->pBitmaps)
    {
      /* Check if the bitmap has already been added
       */
        for (nBM=ptb->nBitmaps, pTemp=ptb->pBitmaps, nIndex=0;
            nBM>0; --nBM, ++pTemp)
        {
            if (pTemp->hInst==hBMInst && pTemp->wID==idBM)
            {
                /* We already have this bitmap, but have we "registered" all
                 * the buttons in it?
                 */
                if (pTemp->nButtons >= nButtons)
                return(nIndex);
                if (nBM == 1)
                {
                /* If this is the last bitmap, we can easily increase the
                 * number of buttons without messing anything up.
                 */
                    pTemp->nButtons = nButtons;
                    return(nIndex);
                }
            }

            nIndex += pTemp->nButtons;
        }

        pTemp = (PTBBMINFO)LocalReAlloc(ptb->pBitmaps,
            (ptb->nBitmaps + 1)*sizeof(TBBMINFO), LMEM_MOVEABLE);
        if (!pTemp)
            return(-1);
        ptb->pBitmaps = pTemp;
    }
    else
    {
        ptb->pBitmaps = (PTBBMINFO)LocalAlloc(LPTR, sizeof(TBBMINFO));
        if (!ptb->pBitmaps)
            return(-1);
    }

    pTemp = ptb->pBitmaps + ptb->nBitmaps;

    pTemp->hInst = hBMInst;
    pTemp->wID = idBM;
    pTemp->nButtons = nButtons;

    if (!TBAddBitmapToImageList(ptb, pTemp))
        return(-1);

    ++ptb->nBitmaps;

    for (nButtons=0, --pTemp; pTemp>=ptb->pBitmaps; --pTemp)
        nButtons += pTemp->nButtons;


    return(nButtons);
}

/* Adds a bitmap to the list of  BMs available for this
 * toolbar. Returns the index of the first button in the bitmap or -1 if there
 * was an error.
 */

#ifdef WIN32
int PASCAL TBLoadImages(PTBSTATE ptb, int id, HINSTANCE hinst)
{
    int iTemp;
    TBBMINFO bmi;

    MapToStandardBitmaps(&hinst, &id, &iTemp);

    bmi.hInst = hinst;
    bmi.wID = id;
    bmi.nButtons = iTemp;
    if (ptb->himl)
        iTemp = ImageList_GetImageCount(ptb->himl);
    else
        iTemp = 0;

    if (!TBAddBitmapToImageList(ptb, &bmi))
        return(-1);

    ptb->fHimlNative = TRUE;
    return iTemp;
}
#endif

BOOL NEAR PASCAL ReplaceBitmap(PTBSTATE ptb, LPTBREPLACEBITMAP lprb)
{
    int nBM;
    PTBBMINFO pTemp;

#ifdef WIN32
    int iTemp;

    MapToStandardBitmaps(&lprb->hInstOld, &lprb->nIDOld, &iTemp);
    MapToStandardBitmaps(&lprb->hInstNew, &lprb->nIDNew, &lprb->nButtons);
#endif

    for (nBM=ptb->nBitmaps, pTemp=ptb->pBitmaps;
         nBM>0; --nBM, ++pTemp)
    {
        if (pTemp->hInst==lprb->hInstOld && pTemp->wID==lprb->nIDOld)
        {
            // number of buttons must match
            pTemp->hInst = lprb->hInstNew;
            pTemp->wID = lprb->nIDNew;
            pTemp->nButtons = lprb->nButtons;
            TBInvalidateImageList(ptb);
            return TRUE;
        }
    }

    return FALSE;
}


void FAR PASCAL FlushToolTipsMgr(PTBSTATE ptb) {

    // change all the rects for the tool tips mgr.  this is
    // cheap, and we don't do it often, so go ahead
    // and do them all.
    if(ptb->hwndToolTips) {
        UINT i;
        TOOLINFO ti;
        PTBBUTTON pButton;

            ti.cbSize = sizeof(ti);
        ti.hwnd = ptb->ci.hwnd;
            ti.lpszText = LPSTR_TEXTCALLBACK;
        for ( i = 0, pButton = ptb->Buttons;
             i < (UINT)ptb->iNumButtons;
             i++, pButton++) {

            if (!(pButton->fsStyle & TBSTYLE_SEP)) {
                ti.uId = pButton->idCommand;
                if (!GetItemRect(ptb, i, &ti.rect))
                    ti.rect.left = ti.rect.right = ti.rect.top = ti.rect.bottom = 0;

                SendMessage(ptb->hwndToolTips, TTM_NEWTOOLRECT, 0, (LPARAM)((LPTOOLINFO)&ti));
            }
        }
    }
}

BOOL FAR PASCAL InsertButtons(PTBSTATE ptb, UINT uWhere, UINT uButtons, LPTBBUTTON lpButtons)
{
    PTBBUTTON pIn, pOut;
    PTBSTATE ptbNew;
#ifdef ACTIVE_ACCESSIBILITY
    UINT    uAdded;
    UINT    uStart;
#endif

    // comments by chee (not the original author) so they not be
    // exactly right... be warned.

    if (!ptb || !ptb->uStructSize)
        return FALSE;

    // enlarge the main structure
    ptbNew = (PTBSTATE)LocalReAlloc(ptb, sizeof(TBSTATE) - sizeof(TBBUTTON)
        + (ptb->iNumButtons + uButtons) * sizeof(TBBUTTON),
        LMEM_MOVEABLE);

    if (!ptbNew)
        return FALSE;

#if defined (DEBUG) && !defined(WINNT)
    if (ptbNew != ptb)
        DebugMsg(DM_TRACE, TEXT("InsertButtons caused the ptb to change!"));
#endif

    ptb = ptbNew;

    SetWindowInt(ptb->ci.hwnd, 0, (int)ptb);

#if TODO
    if (ptb->htbdroptarget)
        ReattachTBDropTarget (ptb->htbdroptarget, ptb);
#endif

    // if where points beyond the end, set it at the end
    if (uWhere > (UINT)ptb->iNumButtons)
        uWhere = ptb->iNumButtons;

#ifdef ACTIVE_ACCESSIBILITY
    // Need to save these since the values gues toasted.
    uAdded = uButtons;
    uStart = uWhere;
#endif

    // move buttons above uWhere up uButton spaces
    // the uWhere gets inverted and counts to zero..
    for (pIn=ptb->Buttons+ptb->iNumButtons-1, pOut=pIn+uButtons,
     uWhere=(UINT)ptb->iNumButtons-uWhere; uWhere>0;
     --pIn, --pOut, --uWhere)
        *pOut = *pIn;

    // now do the copy.
    for (lpButtons=(LPTBBUTTON)((LPBYTE)lpButtons+ptb->uStructSize*(uButtons-1)),
        ptb->iNumButtons+=(int)uButtons;  // init
        uButtons>0; //test
        --pOut, lpButtons=(LPTBBUTTON)((LPBYTE)lpButtons-ptb->uStructSize), --uButtons)
    {
        TBInputStruct(ptb, pOut, lpButtons);

        if(ptb->hwndToolTips && !(lpButtons->fsStyle & TBSTYLE_SEP)) {
            TOOLINFO ti;
            // don't bother setting the rect because we'll do it below
            // in FlushToolTipsMgr;
                ti.cbSize = sizeof(ti);
            ti.uFlags = 0;
            ti.hwnd = ptb->ci.hwnd;
            ti.uId = lpButtons->idCommand;
                ti.lpszText = LPSTR_TEXTCALLBACK;
            SendMessage(ptb->hwndToolTips, TTM_ADDTOOL, 0,
                (LPARAM)(LPTOOLINFO)&ti);
        }

        if ((pOut->fsStyle & TBSTYLE_SEP) && pOut->iBitmap <=0)
            pOut->iBitmap = g_dxButtonSep;
    }

    // Re-compute layout if toolbar is wrapable.
    if (ptb->ci.style & TBSTYLE_WRAPABLE) {
        SendMessage(ptb->ci.hwnd, TB_AUTOSIZE, 0, 0);
    }

    FlushToolTipsMgr(ptb);

    // only need to recalc if there are strings & room enough to actually show them
    if (ptb->nStrings && ((ptb->ci.style & TBSTYLE_LIST) || ((ptb->iDyBitmap + YSLOP + g_cyEdge) < ptb->iButHeight)))
        TBRecalc(ptb);

#ifdef ACTIVE_ACCESSIBILITY
    //
    // Reorder notification so apps can go requery what's on the toolbar if
    // more than 1 button was added; otherwise, just say create.
    //
    if (uAdded == 1)
        MyNotifyWinEvent(EVENT_OBJECT_CREATE, ptb->ci.hwnd, OBJID_CLIENT,
            uWhere+1);
    else
        MyNotifyWinEvent(EVENT_OBJECT_REORDER, ptb->ci.hwnd, OBJID_CLIENT, 0);
#endif

    // We need to completely redraw the toolbar at this point.
    // this MUST be done last!
    // tbrecalc and others will nuke out invalid area and we won't paint if this isn't last
    InvalidateRect(ptb->ci.hwnd, NULL, TRUE);
    return(TRUE);
}


/* Notice that the state structure is not realloc'ed smaller at this
 * point.  This is a time optimization, and the fact that the structure
 * will not move is used in other places.
 */
BOOL FAR PASCAL DeleteButton(PTBSTATE ptb, UINT uIndex)
{
    PTBBUTTON pIn, pOut;
    BOOL fRecalc;

    if (uIndex >= (UINT)ptb->iNumButtons)
        return FALSE;

#ifdef ACTIVE_ACCESSIBILITY
    MyNotifyWinEvent(EVENT_OBJECT_DESTROY, ptb->ci.hwnd, OBJID_CLIENT, uIndex+1);
#endif

    if (ptb->hwndToolTips) {
        TOOLINFO ti;

        ti.cbSize = sizeof(ti);
        ti.hwnd = ptb->ci.hwnd;
        ti.uId = ptb->Buttons[uIndex].idCommand;
        SendMessage(ptb->hwndToolTips, TTM_DELTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
    }

    --ptb->iNumButtons;

    pOut = ptb->Buttons + uIndex;

    fRecalc = (pOut->fsState & TBSTATE_WRAP);

    for (pIn = pOut + 1; uIndex<(UINT)ptb->iNumButtons; ++uIndex, ++pIn, ++pOut)
    {
        fRecalc |= (pIn->fsState & TBSTATE_WRAP);
        *pOut = *pIn;
    }

    // We need to completely recalc or redraw the toolbar at this point.
    if ((ptb->ci.style & TBSTYLE_WRAPABLE) && fRecalc)
    {
        RECT rc;
        HWND hwnd = ptb->ci.hwnd;

        if (!(ptb->ci.style & CCS_NORESIZE) && !(ptb->ci.style & CCS_NOPARENTALIGN))
            hwnd = GetParent(hwnd);

        GetWindowRect(hwnd, &rc);

        WrapToolbar(ptb, rc.right - rc.left, &rc, NULL);
        FlushToolTipsMgr(ptb);
    }

    InvalidateRect(ptb->ci.hwnd, NULL, TRUE);

    FlushToolTipsMgr(ptb);

    return TRUE;
}


// deal with old TBBUTON structs for compatibility

void FAR PASCAL TBInputStruct(PTBSTATE ptb, LPTBBUTTON pButtonInt, LPTBBUTTON pButtonExt)
{
    if (ptb->uStructSize >= sizeof(TBBUTTON))
    {
        *pButtonInt = *pButtonExt;
    }
    else
    /* It is assumed the only other possibility is the OLDBUTTON struct */
    {
        *(LPOLDTBBUTTON)pButtonInt = *(LPOLDTBBUTTON)pButtonExt;
        /* We don't care about dwData */
        pButtonInt->dwData = 0;
        pButtonInt->iString = -1;
    }
}


void NEAR PASCAL TBOutputStruct(PTBSTATE ptb, LPTBBUTTON pButtonInt, LPTBBUTTON pButtonExt)
{
    if (ptb->uStructSize >= sizeof(TBBUTTON))
    {
        LPBYTE pOut;
        int i;

        /* Fill the part we know about and fill the rest with 0's
        */
        *pButtonExt = *pButtonInt;
        for (i = ptb->uStructSize - sizeof(TBBUTTON), pOut = (LPBYTE)(pButtonExt + 1);
            i > 0; --i, ++pOut)
        {
            *pOut = 0;
        }
    }
    else
    /* It is assumed the only other possibility is the OLDBUTTON struct */
    {
        *(LPOLDTBBUTTON)pButtonExt = *(LPOLDTBBUTTON)pButtonInt;
    }
}

void NEAR PASCAL TBOnButtonStructSize(PTBSTATE ptb, UINT uStructSize)
{
    /* You are not allowed to change this after adding buttons.
    */
    if (ptb && !ptb->iNumButtons)
    {
            ptb->uStructSize = uStructSize;
    }
}


LRESULT CALLBACK ToolbarWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fSameButton;
    PTBBUTTON ptbButton;
    int iPos;
    BYTE fsState;
    DWORD dw;
    PTBSTATE ptb = (PTBSTATE)GetWindowInt(hwnd, 0);

    switch (wMsg) {
    case WM_NCCREATE:

#define lpcs ((LPCREATESTRUCT)lParam)

        InitDitherBrush();
        InitGlobalColors();

    /* create the state data for this toolbar */

        ptb = (PTBSTATE)LocalAlloc(LPTR, sizeof(TBSTATE) - sizeof(TBBUTTON));
        if (!ptb)
            return 0;   // WM_NCCREATE failure is 0

    // note, zero init memory from above
        CIInitialize(&ptb->ci, hwnd, lpcs);
        ptb->hfontIcon = NULL;  // initialize to null.
        ptb->uStructSize = 0;
        ptb->xFirstButton = s_xFirstButton;
        ptb->nCurHTButton = -1;
        ptb->iButMinWidth = 0;
        ptb->iButMaxWidth = 0;
        ptb->nTextRows = 1;
        // Now Initialize the hfont we will use.
        TBChangeFont(ptb, 0);

        // grow the button size to the appropriate girth
        if (!SetBitmapSize(ptb, DEFAULTBITMAPX, DEFAULTBITMAPX))
        {
            LocalFree((HLOCAL)ptb);
            return 0;   // WM_NCCREATE failure is 0
        }

        SetWindowInt(hwnd, 0, (int)ptb);

        if (!(ptb->ci.style & (CCS_TOP | CCS_NOMOVEY | CCS_BOTTOM)))
        {
            ptb->ci.style |= CCS_TOP;
            SetWindowLong(hwnd, GWL_STYLE, ptb->ci.style);
        }


        if (ptb->ci.style & TBSTYLE_TOOLTIPS)
        {
            TOOLINFO ti;
            // don't bother setting the rect because we'll do it below
            // in FlushToolTipsMgr;
                ti.cbSize = sizeof(ti);
            ti.uFlags = TTF_IDISHWND;
            ti.hwnd = hwnd;
            ti.uId = (UINT)hwnd;
                ti.lpszText = 0;

            ptb->hwndToolTips = CreateWindow(c_szSToolTipsClass, NULL,
            WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            hwnd, NULL, lpcs->hInstance, NULL);

            SendMessage(ptb->hwndToolTips, TTM_ADDTOOL, 0,
                (LPARAM)(LPTOOLINFO)&ti);
        }
        return TRUE;

    case WM_CREATE:
#if TODO
        if (ptb->ci.style & TBSTYLE_DROPPABLE)      // Drag and drop support
        {
            ptb->htbdroptarget = CreateTBDropTarget(ptb);
        }
#endif
        return 0;    // WM_CREATE succeeds with 0

    case WM_DESTROY:
        if (ptb)
        {
              //
              // If the toolbar created tooltips, then destroy them.
              //

            if ((ptb->ci.style & TBSTYLE_TOOLTIPS) && IsWindow(ptb->hwndToolTips)) {
                DestroyWindow (ptb->hwndToolTips);
                ptb->hwndToolTips = NULL;
            }

#if TODO
            if (ptb->ci.style & TBSTYLE_DROPPABLE)
                DestroyTBDropTarget (ptb->htbdroptarget);
#endif

            if (ptb->hbmMono)
                DeleteObject(ptb->hbmMono);

            ReleaseMonoDC(ptb);

            if (ptb->nStrings > 0)
                DestroyStrings(ptb);

            if (ptb->hfontIcon)
                DeleteObject(ptb->hfontIcon);

            // only do this destroy if pBitmaps exists..
            // this is our signal that it was from an old style toolba
            // and we created it ourselves.
            if (ptb->pBitmaps && ptb->himl)
                ImageList_Destroy(ptb->himl);

            if (ptb->pBitmaps)
                LocalFree(ptb->pBitmaps);


            LocalFree((HLOCAL)ptb);
            SetWindowInt(hwnd, 0, 0);
        }
        TerminateDitherBrush();
        break;

    case WM_NCCALCSIZE:
        // let defwindowproc handle the standard borders etc...
        dw = DefWindowProc(hwnd, wMsg, wParam, lParam ) ;

        // add the extra edge at the top of the toolbar to seperate from the menu bar
        if (ptb && !(ptb->ci.style & CCS_NODIVIDER))
        {
                ((NCCALCSIZE_PARAMS FAR *)lParam)->rgrc[0].top += g_cyEdge;
        }

        return dw;

    case WM_NCHITTEST:
        return HTCLIENT;

    case WM_NCACTIVATE: // BUGBUG: really needed? actiave and inactive draw the same.
    case WM_NCPAINT:
        // old-style toolbars are forced to be without dividers above
        if (ptb && !(ptb->ci.style & CCS_NODIVIDER))
        {
            RECT rc;
            HDC hdc = GetWindowDC(hwnd);
            GetWindowRect(hwnd, &rc);
            MapWindowRect(NULL, hwnd, &rc); // screen -> client

                rc.bottom = -rc.top;                // bottom of NC area
                rc.top = rc.bottom - g_cyEdge;

            DrawEdge(hdc, &rc, BDR_SUNKENOUTER, BF_TOP | BF_BOTTOM);
            ReleaseDC(hwnd, hdc);
        }
        goto DoDefault;

    case WM_PRINTCLIENT:
    case WM_PAINT:
        if (ptb)
            ToolbarPaint(ptb, (HDC)wParam);
        break;

    case WM_ERASEBKGND:
        if (ptb)
            TB_OnEraseBkgnd(ptb, (HDC) wParam);
        return(TRUE);

    case WM_SYSCOLORCHANGE:
        if (ptb)
        {
            TB_OnSysColorChange(ptb);
            if (ptb->hwndToolTips)
                SendMessage(ptb->hwndToolTips, wMsg, wParam, lParam);
        }
        break;

    case TB_GETROWS:
        if (ptb)
            return CountRows(ptb);
        break;

    case TB_SETROWS:
        if (ptb)
        {
            RECT rc;

            if (BoxIt(ptb, LOWORD(wParam), HIWORD(wParam), &rc))
            {
                FlushToolTipsMgr(ptb);
                SetWindowPos(hwnd, NULL, 0, 0, rc.right, rc.bottom,
                             SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
                InvalidateRect(hwnd, NULL, TRUE);
            }
            if (lParam)
                *((RECT FAR *)lParam) = rc;
        }
        break;

    case WM_WINDOWPOSCHANGING:
        if (ptb && (ptb->ci.style & TBSTYLE_FLAT))
        {
            LPWINDOWPOS lpwp = (LPWINDOWPOS) lParam;
            RECT rc;
            HWND hwndParent;

            if (!(lpwp->flags & SWP_NOMOVE) && (hwndParent = GetParent(hwnd)))
            {
                GetWindowRect(hwnd, &rc);
                MapWindowPoints(HWND_DESKTOP, hwndParent, (LPPOINT)&rc, 1);
                if ((rc.left != lpwp->x) || (rc.top != lpwp->y))
                    lpwp->flags |= SWP_NOREDRAW;
            }
        }
        goto DoDefault;

    case WM_MOVE:
        // JJK TODO: This needs to be double buffered to get rid of the flicker
        if (ptb && (ptb->ci.style & TBSTYLE_FLAT))
            InvalidateRect(hwnd, NULL, TRUE);
        goto DoDefault;

    case TB_AUTOSIZE:
    case WM_SIZE:
        if (ptb)
        {
            HWND hwndParent;
            RECT rc;

            hwndParent = GetParent(hwnd);
            if (!hwndParent)
                break;

            if (ptb->ci.style & TBSTYLE_WRAPABLE)
            {
                RECT rcNew;
                    if ((ptb->ci.style & CCS_NORESIZE) || (ptb->ci.style & CCS_NOPARENTALIGN)) {
                        GetWindowRect(hwnd, &rc);
                    } else {
                        GetWindowRect(hwndParent, &rc);
                    }

                WrapToolbar(ptb, rc.right - rc.left, &rcNew, NULL);
                FlushToolTipsMgr(ptb);
            }

            GetWindowRect(hwnd, &rc);
            MapWindowPoints(HWND_DESKTOP, hwndParent, (LPPOINT)&rc, 2);
            NewSize(hwnd, ptb->iButHeight * CountRows(ptb) + g_cxEdge * 2, ptb->ci.style,
                    rc.left, rc.top, rc.right, rc.bottom);
        }
        break;

    case WM_COMMAND:
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
        if (ptb)
            SendMessage(ptb->ci.hwndParent, wMsg, wParam, lParam);
        break;

    case WM_RBUTTONDBLCLK:
        if (ptb && !SendNotifyEx(ptb->ci.hwndParent, hwnd, NM_RDBLCLK, NULL, ptb->ci.bUnicode))
            goto DoDefault;
        break;

    case WM_RBUTTONUP:
#ifdef IEWIN31_25
        // Simulate WM_CONTEXTMENU commands on windows 3.1
        if (ptb)
        {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            ClientToScreen(hwnd, &pt);
            SendMessage(ptb->ci.hwndParent, WM_CONTEXTMENU, (WPARAM)hwnd, MAKELONG(pt.x, pt.y));
        }
#endif
        if (ptb && !SendNotifyEx(ptb->ci.hwndParent, hwnd, NM_RCLICK, NULL, ptb->ci.bUnicode))
        {
#ifdef IEWIN31_25
            // default produces second WM_CONTEXTMENU on Windows 95
            break;
#else
            goto DoDefault;
#endif
        }
        break;

    case WM_LBUTTONDBLCLK:
        if (!ptb) {
            break;
        }

        iPos = TBHitTest(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
#ifndef IEWIN31_25
    if (iPos < 0 && (ptb->ci.style & CCS_ADJUSTABLE))
    {
        iPos = -1 - iPos;
        CustomizeTB(ptb, iPos);
        } else {
            goto HandleLButtonDown;
    }
    break;
#else
    goto HandleLButtonDown;
#endif

    case WM_LBUTTONDOWN:
        if (!ptb) {
            break;
        }

        RelayToToolTips(ptb->hwndToolTips, hwnd, wMsg, wParam, lParam);

        iPos = TBHitTest(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
#ifndef IEWIN31_25
        if ((ptb->ci.style & CCS_ADJUSTABLE) &&
                 (((wParam & MK_SHIFT) && !(ptb->ci.style & TBSTYLE_ALTDRAG)) ||
                  ((GetKeyState(VK_MENU) & ~1) && (ptb->ci.style & TBSTYLE_ALTDRAG))))
        {
            MoveButton(ptb, iPos);
        }
        else
#endif
        {
HandleLButtonDown:
            if (iPos >= 0 && iPos < ptb->iNumButtons)
            {
                // should this check for the size of the button struct?
                ptbButton = ptb->Buttons + iPos;

                if (ptbButton->fsStyle & TBSTYLE_DROPDOWN) {

                    if (ptbButton->fsState & TBSTATE_ENABLED) {
                        ptbButton->fsState ^= TBSTATE_PRESSED;
                        InvalidateButton(ptb, ptbButton, TRUE);
                        UpdateWindow(hwnd);

#ifdef ACTIVE_ACCESSIBILITY
                        MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd,
                                OBJID_CLIENT, iPos+1);
#endif // ACTIVE_ACCESSIBILITY

                        if (!SendItemNotify(ptb, ptbButton->idCommand, TBN_DROPDOWN)) {

                            MSG msg;

                            PeekMessage(&msg, ptb->ci.hwnd, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE);

                            ptbButton->fsState ^= TBSTATE_PRESSED;
                            InvalidateButton(ptb, ptbButton, TRUE);
                            UpdateWindow(hwnd);

#ifdef ACTIVE_ACCESSIBILITY
                            MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd,
                                OBJID_CLIENT, iPos+1);
#endif // ACTIVE_ACCESSIBILITY

#ifdef IEWIN31_25
                            // We need to inform the tooltips that the button was released
                            // or they will be disabled until the next click!
                            if (!(GetKeyState(VK_LBUTTON) & KF_UP))
                                RelayToToolTips(ptb->hwndToolTips, hwnd, WM_LBUTTONUP, wParam, lParam);
#endif  // IEWIN31_25

                        
                        }

                    }
                }
                else
                {
                    ptb->pCaptureButton = ptbButton;
                    SetCapture(hwnd);

                    if (ptbButton->fsState & TBSTATE_ENABLED)
                    {
                        ptbButton->fsState |= TBSTATE_PRESSED;
                        InvalidateButton(ptb, ptbButton, TRUE);
                        UpdateWindow(hwnd);         // imedeate feedback

#ifdef ACTIVE_ACCESSIBILITY
                        MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd,
                            OBJID_CLIENT, iPos+1);
#endif // ACTIVE_ACCESSIBILITY
                    }

                    SendItemNotify(ptb, ptb->pCaptureButton->idCommand, TBN_BEGINDRAG);

                }
            }
    }
    break;

//#ifdef WIN32
#ifdef IEWIN31_25
#ifndef NOTRACKMOUSEEVENT
    case WM_MOUSELEAVE:
        if (ptb)
        {
            RECT rectButton;
            TRACKMOUSEEVENT tme;

            Assert(ptb->ci.style & TBSTYLE_FLAT);
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_CANCEL | TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            ptb->fMouseTrack = FALSE;
            if (ptb->nCurHTButton != -1)
                if (GetItemRect(ptb, ptb->nCurHTButton, &rectButton) )
                    InvalidateRect(hwnd, &rectButton, TRUE);
            ptb->nCurHTButton = -1;
        }
        break;
#endif //NOTRACKMOUSEEVENT
#endif //IEWIN31_25
//#endif //WIN32

    case WM_MOUSEMOVE:
        if (ptb)
        {
            RECT rectButton;

            if ((ptb->ci.style & TBSTYLE_FLAT) )
            {
                iPos = TBHitTest(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
//#ifdef WIN32
#ifdef IEWIN31_25
#ifndef NOTRACKMOUSEEVENT
                if (!ptb->fMouseTrack)
                {
                    TRACKMOUSEEVENT tme;

                    tme.cbSize = sizeof(TRACKMOUSEEVENT);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = hwnd;
                    ptb->fMouseTrack = TRUE;
                    TrackMouseEvent(&tme);
                }
#endif //NOTRACKMOUSEEVENT
#endif //IEWIN31_25
//#endif //WIN32

                if ((ptb->nCurHTButton != iPos) && (ptb->nCurHTButton != -1))
                {
                    if (GetItemRect(ptb, ptb->nCurHTButton, &rectButton) )
                        InvalidateRect(hwnd, &rectButton, TRUE);
                    ptb->nCurHTButton = -1;

                }
                if ((iPos >= 0) && (ptb->nCurHTButton != iPos))
                {
                    if ((ptb->Buttons[iPos].fsState & TBSTATE_ENABLED) )
                    {
                        ptb->nCurHTButton = iPos;
                        if (GetItemRect(ptb, iPos, &rectButton) )
                            InvalidateRect(hwnd, &rectButton, TRUE);
                    }
                }
            }

            RelayToToolTips(ptb->hwndToolTips, hwnd, wMsg, wParam, lParam);

            // if the toolbar has lost the capture for some reason, stop
            if (ptb->pCaptureButton == NULL) {
                //DebugMsg(DM_TRACE, TEXT("Bail because pCaptureButton == NULL"));
                break;
            }

            if (hwnd != GetCapture())
            {
                //DebugMsg(DM_TRACE, TEXT("capture isn't us"));
                SendItemNotify(ptb, ptb->pCaptureButton->idCommand, TBN_ENDDRAG);

                // if the button is still pressed, unpress it.
                if (ptb->pCaptureButton->fsState & TBSTATE_PRESSED)
                    SendMessage(hwnd, TB_PRESSBUTTON, ptb->pCaptureButton->idCommand, 0L);
                ptb->pCaptureButton = NULL;
            }
            else if (ptb->pCaptureButton->fsState & TBSTATE_ENABLED)
            {
                //DebugMsg(DM_TRACE, TEXT("capture IS us, and state is enabled"));
                iPos = TBHitTest(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                fSameButton = (iPos >= 0 && ptb->pCaptureButton == ptb->Buttons + iPos);
                if (fSameButton == !(ptb->pCaptureButton->fsState & TBSTATE_PRESSED))
                {
                     //DebugMsg(DM_TRACE, TEXT("capture IS us, and Button is different"));
                    ptb->pCaptureButton->fsState ^= TBSTATE_PRESSED;
                    InvalidateButton(ptb, ptb->pCaptureButton, TRUE);

#ifdef ACTIVE_ACCESSIBILITY
                    MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd,
                        OBJID_CLIENT, (ptb->pCaptureButton - ptb->Buttons) + 1);
#endif // ACTIVE_ACCESSIBILITY

                }
            }
        }
        break;

    case WM_LBUTTONUP:
        if (!ptb) {
            break;
        }

        RelayToToolTips(ptb->hwndToolTips, hwnd, wMsg, wParam, lParam);

        if (ptb->pCaptureButton != NULL) {

            int idCommand = ptb->pCaptureButton->idCommand;

            ReleaseCapture();

            SendItemNotify(ptb, idCommand, TBN_ENDDRAG);

            iPos = TBHitTest(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            if ((ptb->pCaptureButton->fsState & TBSTATE_ENABLED) && iPos >=0
                && (ptb->pCaptureButton == ptb->Buttons+iPos)) {
                ptb->pCaptureButton->fsState &= ~TBSTATE_PRESSED;

                if (ptb->pCaptureButton->fsStyle & TBSTYLE_CHECK) {
                    if (ptb->pCaptureButton->fsStyle & TBSTYLE_GROUP) {

                        // group buttons already checked can't be force
                        // up by the user.

                        if (ptb->pCaptureButton->fsState & TBSTATE_CHECKED) {
                            ptb->pCaptureButton = NULL;
                            break;  // bail!
                        }

                        ptb->pCaptureButton->fsState |= TBSTATE_CHECKED;
                        MakeGroupConsistant(ptb, idCommand);
                    } else {
                        ptb->pCaptureButton->fsState ^= TBSTATE_CHECKED; // toggle
                    }
                }
                InvalidateButton(ptb, ptb->pCaptureButton, TRUE);
                ptb->pCaptureButton = NULL;

#ifdef ACTIVE_ACCESSIBILITY
                MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd,  OBJID_CLIENT,
                    iPos+1);
#endif // ACTIVE_ACCESSIBILITY

                FORWARD_WM_COMMAND(ptb->ci.hwndParent, idCommand, hwnd, BN_CLICKED, SendMessage);
            }
            else {
                ptb->pCaptureButton = NULL;
            }
        }
        break;

    case WM_WININICHANGE:
        if (!ptb) {
            break;
        }

        InitGlobalMetrics(wParam);
        if (ptb)
            TBChangeFont(ptb, wParam);
        if (ptb->hwndToolTips)
            SendMessage(ptb->hwndToolTips, wMsg, wParam, lParam);
        break;

    case WM_NOTIFYFORMAT:
        if (ptb)
            return CIHandleNotifyFormat(&ptb->ci, lParam);
        break;

    case WM_NOTIFY:
#define lpNmhdr ((LPNMHDR)(lParam))

        if (ptb)
        {
            int i = PositionFromID(ptb, lpNmhdr->idFrom);
            BOOL fEllipsied = FALSE;

            //
            // We are just going to pass this on to the
            // real parent.  Note that -1 is used as
            // the hwndFrom.  This prevents SendNotifyEx
            // from updating the NMHDR structure.
            //
            LRESULT lres = SendNotifyEx(ptb->ci.hwndParent, (HWND) -1,
                         lpNmhdr->code, lpNmhdr, ptb->ci.bUnicode);

#define lpnmTT ((LPTOOLTIPTEXT) lParam)

            if (i != -1)
                fEllipsied = (BOOL)(ptb->Buttons[i].fsState & TBSTATE_ELLIPSES);

            if ((lpNmhdr->code == TTN_NEEDTEXT) && ptb->pStrings &&
                (!ptb->nTextRows || fEllipsied) &&
                lpnmTT->lpszText && !lpnmTT->lpszText[0])
            {
                if (i != -1)
                {
                    i = ptb->Buttons[i].iString;
                    if (i < ptb->nStrings)
                        lpnmTT->lpszText = ptb->pStrings[i];
                }
            }
            return(lres);
        }
        break;

    case WM_STYLECHANGING:
        if (!ptb)
            break;

        if (wParam == GWL_STYLE)
        {
            LPSTYLESTRUCT lpStyle = (LPSTYLESTRUCT) lParam;

            if ((lpStyle->styleOld ^ lpStyle->styleNew) == WS_VISIBLE)
                // MFC can't hide a window to save their lives -- and they
                // think that flipping bits directly will actually give them
                // something -- don't let 'em yank the visible bit directly
                // ... morons ... jeffbog 9/13/96
                lpStyle->styleNew |= WS_VISIBLE;
        }
        break;

    case WM_STYLECHANGED:
        if (!ptb) {
            break;
        }

        if (wParam == GWL_STYLE)
        {
            ptb->ci.style = ((LPSTYLESTRUCT)lParam)->styleNew;
            SendMessage (hwnd, TB_AUTOSIZE, 0, 0);
#ifndef WINNT
            DebugMsg(DM_TRACE, TEXT("toolbar window style changed %x"), ptb->ci.style);
#endif
        }
        return 0;

    case TB_SETSTYLE:
        if (ptb)
        {
            BOOL fSizeChanged = FALSE;

            if (!ptb)
                break;

            if ((BOOL)(ptb->ci.style & TBSTYLE_WRAPABLE) != (BOOL)(lParam & TBSTYLE_WRAPABLE))
            {
                int i;
                fSizeChanged = TRUE;

                for (i=0; i<ptb->iNumButtons; i++)
                    ptb->Buttons[i].fsState &= ~TBSTATE_WRAP;
            }

            ptb->ci.style = lParam;

            if (fSizeChanged)
                TBRecalc(ptb);
        }
        break;

    case TB_GETSTYLE:
        if (ptb)
            return (ptb->ci.style);
        break;

    case TB_GETBUTTONSIZE:
        if (ptb)
           return (MAKELONG(ptb->iButWidth,ptb->iButHeight));
        break;

    case TB_SETBUTTONWIDTH:
        if (!ptb) {
            break;
        }
        ptb->iButMinWidth  = LOWORD(lParam);
        ptb->iButMaxWidth = HIWORD(lParam);
        ptb->iButWidth = 0;
        TBRecalc(ptb);
        return TRUE;

    case TB_SETSTATE:
        if (!ptb) {
            break;
        }

        iPos = PositionFromID(ptb, (int)wParam);
        if (iPos < 0)
            return FALSE;
        ptbButton = ptb->Buttons + iPos;

        fsState = (BYTE)(LOWORD(lParam) ^ ptbButton->fsState);
        ptbButton->fsState = (BYTE)LOWORD(lParam);

        if (fsState)
        {
            if (fsState & TBSTATE_HIDDEN)
                InvalidateRect(hwnd, NULL, TRUE);
            else
                InvalidateButton(ptb, ptbButton, TRUE);

#ifdef ACTIVE_ACCESSIBILITY
            MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_CLIENT,
                iPos+1);
#endif
        }

        return TRUE;

    // set the cmd ID of a button based on its position
    case TB_SETCMDID:
        {
        UINT uiOldID;

        if (!ptb) {
            break;
        }

        if (wParam >= (UINT)ptb->iNumButtons)
            return FALSE;

        uiOldID = ptb->Buttons[wParam].idCommand;
        ptb->Buttons[wParam].idCommand = (UINT)lParam;

        //
        // If the app was using tooltips, then
        // we need to update the command id there also.
        //

        if(ptb->hwndToolTips) {
            TOOLINFO ti;

            //
            // Query the old information
            //

            ti.cbSize = sizeof(ti);
            ti.hwnd = ptb->ci.hwnd;
            ti.uId = uiOldID;
            SendMessage(ptb->hwndToolTips, TTM_GETTOOLINFO, 0,
                        (LPARAM)(LPTOOLINFO)&ti);

            //
            // Delete the old tool since we can't just
            // change the command id.
            //

            SendMessage(ptb->hwndToolTips, TTM_DELTOOL, 0,
                        (LPARAM)(LPTOOLINFO)&ti);

            //
            // Add the new tool with the new command id.
            //

            ti.uId = (UINT)lParam;
            SendMessage(ptb->hwndToolTips, TTM_ADDTOOL, 0,
                        (LPARAM)(LPTOOLINFO)&ti);
        }
        }
        return TRUE;

    case TB_GETSTATE:
        if (!ptb) {
            break;
        }

        iPos = PositionFromID(ptb, (int)wParam);
        if (iPos < 0)
            return -1L;
        return ptb->Buttons[iPos].fsState;

    case TB_ENABLEBUTTON:
    case TB_CHECKBUTTON:
    case TB_PRESSBUTTON:
    case TB_HIDEBUTTON:
    case TB_INDETERMINATE:
    case TB_HIGHLIGHTBUTTON:

        if (!ptb) {
            break;
        }

        iPos = PositionFromID(ptb, (int)wParam);
        if (iPos < 0)
            return FALSE;
        ptbButton = &ptb->Buttons[iPos];
        fsState = ptbButton->fsState;

        if (LOWORD(lParam))
            ptbButton->fsState |= wStateMasks[wMsg - TB_ENABLEBUTTON];
        else
            ptbButton->fsState &= ~wStateMasks[wMsg - TB_ENABLEBUTTON];

        // did this actually change the state?
        if (fsState != ptbButton->fsState) {
            // is this button a member of a group?
            if ((wMsg == TB_CHECKBUTTON) && (ptbButton->fsStyle & TBSTYLE_GROUP))
                MakeGroupConsistant(ptb, (int)wParam);

            if (wMsg == TB_HIDEBUTTON) {
                InvalidateRect(hwnd, NULL, TRUE);
                FlushToolTipsMgr(ptb);
            } else
                InvalidateButton(ptb, ptbButton, TRUE);

#ifdef ACTIVE_ACCESSIBILITY
        MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_CLIENT, iPos+1);
#endif // ACTIVE_ACCESSIBILITY

        }
        return(TRUE);

    case TB_ISBUTTONENABLED:
    case TB_ISBUTTONCHECKED:
    case TB_ISBUTTONPRESSED:
    case TB_ISBUTTONHIDDEN:
    case TB_ISBUTTONINDETERMINATE:
    case TB_ISBUTTONHIGHLIGHTED:
        if (!ptb) {
            break;
        }

        iPos = PositionFromID(ptb, (int)wParam);
        if (iPos < 0)
            return(-1L);
        return (LRESULT)ptb->Buttons[iPos].fsState & wStateMasks[wMsg - TB_ISBUTTONENABLED];

    case TB_ADDBITMAP:
#ifdef WIN32
    case TB_ADDBITMAP32:    // only for compatibility with mail
    #define pab ((LPTBADDBITMAP)lParam)
        if (!ptb) {
            break;
        }

        return AddBitmap(ptb, wParam, pab->hInst, pab->nID);
    #undef pab
#else
        return AddBitmap(ptb, wParam, (HINSTANCE)LOWORD(lParam), HIWORD(lParam));
#endif

    case TB_REPLACEBITMAP:
        if (ptb)
            return ReplaceBitmap(ptb, (LPTBREPLACEBITMAP)lParam);
        break;

#ifdef UNICODE
    case TB_ADDSTRINGA:
        {
        LPWSTR lpStrings;
        UINT   uiCount;
        LPSTR  lpAnsiString = (LPSTR) lParam;
        int    iResult;
        BOOL   bAllocatedMem = FALSE;

        if (!ptb) {
            break;
        }

        if (!wParam && lpAnsiString) {
            //
            // We have to figure out how many characters
            // are in this string.
            //
            uiCount = 0;

            while (uiCount < MAXSTRINGSIZE) {
               uiCount++;
               if ((*lpAnsiString == 0) && (*(lpAnsiString+1) == 0)) {
                  uiCount++;  // needed for double null
                  break;
               }

               lpAnsiString++;
            }

            lpStrings = GlobalAlloc (GPTR, uiCount * sizeof(TCHAR));

            if (!lpStrings)
                return -1;

            bAllocatedMem = TRUE;

            MultiByteToWideChar(CP_ACP, 0, (LPCSTR) lParam, uiCount,
                                lpStrings, uiCount);

        } else {
            lpStrings = (LPWSTR)lParam;
        }

        iResult = AddStrings(ptb, wParam, (LPARAM)lpStrings);

        if (bAllocatedMem)
            GlobalFree(lpStrings);

        return iResult;
        }
#endif

    case TB_ADDSTRING:
        if (!ptb) {
            break;
        }
    return AddStrings(ptb, wParam, lParam);

    case TB_ADDBUTTONS:
        if (!ptb) {
            break;
        }
    return InsertButtons(ptb, (UINT)-1, wParam, (LPTBBUTTON)lParam);

    case TB_INSERTBUTTON:
        if (!ptb) {
            break;
        }
    return InsertButtons(ptb, wParam, 1, (LPTBBUTTON)lParam);

    case TB_DELETEBUTTON:
        if (!ptb) {
            break;
        }
    return DeleteButton(ptb, wParam);

    case TB_GETBUTTON:
        if (!ptb) {
            break;
        }

        if (wParam >= (UINT)ptb->iNumButtons)
            return(FALSE);

        TBOutputStruct(ptb, ptb->Buttons + wParam, (LPTBBUTTON)lParam);
        return TRUE;

    case TB_BUTTONCOUNT:
        if (!ptb) {
            break;
        }
        return ptb->iNumButtons;

    case TB_COMMANDTOINDEX:
        if (!ptb) {
            break;
        }
        return PositionFromID(ptb, (int)wParam);

#ifdef UNICODE
    case TB_SAVERESTOREA:
        {
        LPWSTR lpSubKeyW, lpValueNameW;
        TBSAVEPARAMSA * lpSaveA = (TBSAVEPARAMSA *) lParam;
        BOOL bResult;

        if (!ptb) {
            break;
        }

        lpSubKeyW = ProduceWFromA (CP_ACP, lpSaveA->pszSubKey);
        lpValueNameW = ProduceWFromA (CP_ACP, lpSaveA->pszValueName);

        bResult = SaveRestoreFromReg(ptb, wParam, lpSaveA->hkr, lpSubKeyW, lpValueNameW);

        FreeProducedString(lpSubKeyW);
        FreeProducedString(lpValueNameW);

        return bResult;
        }
#endif

#ifndef IEWIN31_25
    case TB_SAVERESTORE:
#ifdef WIN32
    #define psr ((TBSAVEPARAMS *)lParam)
        if (!ptb) {
            break;
        }
        return SaveRestoreFromReg(ptb, wParam, psr->hkr, psr->pszSubKey, psr->pszValueName);
    #undef psr
#else
        return SaveRestore(ptb, wParam, (LPTSTR FAR *)lParam);
#endif
#endif

#ifndef IEWIN31_25
    case TB_CUSTOMIZE:
        if (!ptb) {
            break;
        }
    CustomizeTB(ptb, ptb->iNumButtons);
        break;
#endif

    case TB_GETRECT:
        // PositionFromID() accepts NULL ptbs!
        wParam = PositionFromID(ptb, wParam);
        // fall through
    case TB_GETITEMRECT:
        if (!ptb || !lParam) {
            break;
        }
    return GetItemRect(ptb, wParam, (LPRECT)lParam);

    case TB_BUTTONSTRUCTSIZE:
        if (!ptb) {
            break;
        }
        TBOnButtonStructSize(ptb, wParam);
        break;

    case TB_SETBUTTONSIZE:
        if (!ptb) {
            break;
        }
        return GrowToolbar(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), FALSE);

    case TB_SETBITMAPSIZE:
        if (!ptb) {
            break;
        }
        return SetBitmapSize(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

    case TB_SETIMAGELIST:
        if (ptb)
        {
            HIMAGELIST himl = ptb->himl;

            ptb->himl = (HIMAGELIST)lParam;
            ptb->fHimlNative = TRUE;
            return (LRESULT)himl;
        }
        break;

    case TB_GETIMAGELIST:
        if (ptb)
            return (LRESULT)ptb->himl;
        break;

    case TB_SETHOTIMAGELIST:
        if (ptb)
        {
            HIMAGELIST himl = ptb->himlHot;

            ptb->himlHot = (HIMAGELIST)lParam;
            return (LRESULT)himl;
        }
        break;

    case TB_GETHOTIMAGELIST:
        if (ptb)
            return (LRESULT)ptb->himlHot;
        break;

    case TB_GETDISABLEDIMAGELIST:
        if (ptb)
            return (LRESULT)ptb->himlDisabled;
        break;

    case TB_SETDISABLEDIMAGELIST:
        if (ptb)
        {
            HIMAGELIST himl = ptb->himlDisabled;

            ptb->himlDisabled = (HIMAGELIST)lParam;
            return (LRESULT)himl;
        }
        break;

    case WM_GETFONT:
        return (LRESULT)(UINT)(ptb? ptb->hfontIcon : 0);

#ifdef WIN32
    case TB_LOADIMAGES:
        if (ptb)
            return TBLoadImages(ptb, wParam, (HINSTANCE)lParam);
        break;
#endif

    case TB_GETTOOLTIPS:
        if (!ptb) {
            break;
        }
        return (LRESULT)(UINT)ptb->hwndToolTips;

    case TB_SETTOOLTIPS:
        if (!ptb) {
            break;
        }
        ptb->hwndToolTips = (HWND)wParam;
        break;

    case TB_SETPARENT:
        if (ptb)
        {
            HWND hwndOld = ptb->ci.hwndParent;

        ptb->ci.hwndParent = (HWND)wParam;
        return (LRESULT)(UINT)hwndOld;
        }
        break;

    case TB_CHANGEBITMAP:

        if (!ptb) {
            break;
        }
        iPos = PositionFromID(ptb, (int)wParam);
        if (iPos < 0)
            return(FALSE);

        //
        // Check to see if the new bitmap ID is
        // valid.
        //

        if (ptb->fHimlNative || ptb->fHimlValid) {
            if (!ptb->himl ||
                LOWORD(lParam) >= ImageList_GetImageCount(ptb->himl))
                return FALSE;
        } else {

            PTBBMINFO pTemp;
            int nBitmap;
            UINT nTot;

            // we're not natively himl and we've got some invalid
            // image state, so we need to count the bitmaps ourselvesa
            pTemp = ptb->pBitmaps;
            nTot = 0;

            for (nBitmap=0; nBitmap < ptb->nBitmaps; nBitmap++) {
                nTot += pTemp->nButtons;
                pTemp++;
            }

            if (LOWORD(lParam) >= nTot)
                return FALSE;
        }

        ptbButton = &ptb->Buttons[iPos];
        ptbButton->iBitmap = LOWORD(lParam);
        InvalidateButton(ptb, ptbButton, FALSE);
        return TRUE;

    case TB_GETBITMAP:
        if (!ptb) {
            break;
        }
        iPos = PositionFromID(ptb, (int)wParam);
        if (iPos < 0)
            return(FALSE);
        ptbButton = &ptb->Buttons[iPos];
        return ptbButton->iBitmap;

#ifdef UNICODE
    case TB_GETBUTTONTEXTA:
        if (!ptb) {
            break;
        }
        iPos = PositionFromID(ptb, (int)wParam);
        if (iPos >= 0) {
            ptbButton = &ptb->Buttons[iPos];
            if ((ptbButton->iString > -1) && (ptbButton->iString < ptb->nStrings)) {
                if (lParam) {
                    WideCharToMultiByte (CP_ACP, 0, ptb->pStrings[ptbButton->iString],
                                         -1,(LPSTR)lParam , INT_MAX, NULL, NULL);
                }
                return lstrlen(ptb->pStrings[ptbButton->iString]);
            }
        }
        return -1;
#endif

    case TB_GETBUTTONTEXT:
        if (!ptb) {
            break;
        }
        iPos = PositionFromID(ptb, (int)wParam);
        if (iPos >= 0) {
            ptbButton = &ptb->Buttons[iPos];
            if ((ptbButton->iString > -1) && (ptbButton->iString < ptb->nStrings)) {
                if (lParam) {
                    lstrcpy((LPTSTR)lParam, ptb->pStrings[ptbButton->iString]);
                }
                return lstrlen(ptb->pStrings[ptbButton->iString]);
            }
        }
        return -1;


#ifdef WIN32
    case TB_GETBITMAPFLAGS:
        {
        DWORD fFlags = 0;
        HDC hdc = GetDC(NULL);

        if (GetDeviceCaps(hdc, LOGPIXELSY) >= 120)
            fFlags |= TBBF_LARGE;

        ReleaseDC(NULL, hdc);

        return fFlags;
        }
#endif

    case TB_SETINDENT:
        if (!ptb) {
            break;
        }

        ptb->xFirstButton = wParam;
        InvalidateRect (hwnd, NULL, TRUE);
        FlushToolTipsMgr(ptb);
        return 1;

    case TB_SETMAXTEXTROWS:
        if (!ptb)
            break;

        ptb->nTextRows = wParam;
        TBRecalc(ptb);
        return 1;

    case TB_GETTEXTROWS:
        if (!ptb)
            break;

        return ptb->nTextRows;

    default:
DoDefault:
        return DefWindowProc(hwnd, wMsg, wParam, lParam);
    }

    return 0L;
}
