/*
** Toolbar.c
**
** This is it, the incredibly famous toolbar control.  Most of
** the customization stuff is in another file.
*/

#include "ctlspriv.h"
#include "toolbar.h"
#include "image.h"
#include <limits.h>
#include "apithk.h"

#define __IOleControl_INTERFACE_DEFINED__       // There is a conflich with the IOleControl's def of CONTROLINFO
#include "shlobj.h"


#ifdef MAINWIN
#include <mainwin.h>
void TBSetHotItemWithoutNotification(PTBSTATE ptb, int iPos, DWORD dwReason);
extern void  TruncateString(char *sz, int cch);
#endif

#define TBP_ONRELEASECAPTURE (WM_USER + 0x500)

#define TBIMAGELIST
// these values are defined by the UI gods...
#define DEFAULTBITMAPX 16
#define DEFAULTBITMAPY 15

#define LIST_GAP        (g_cxEdge * 2)
#define DROPDOWN_GAP    (g_cxEdge * 2)
#define CX_TOP_FUDGE    (g_cxEdge * 2)

#define SMALL_DXYBITMAP     16      // new dx dy for sdt images
#define LARGE_DXYBITMAP     24

#define DEFAULTBUTTONX      24
#define DEFAULTBUTTONY      22
// the insert mark is 6 pixels high/wide depending on horizontal or vertical mode...
#define INSERTMARKSIZE      6

const int g_dxButtonSep = 8;
const int s_xFirstButton = 0;   // was 8 in 3.1
#define s_dxOverlap 0           // was 1 in 3.1
#define USE_MIXED_BUTTONS(ptb) (((ptb)->dwStyleEx & TBSTYLE_EX_MIXEDBUTTONS) && ((ptb)->ci.style & TBSTYLE_LIST))
#define BTN_NO_SHOW_TEXT(ptb, ptbb) (!(ptb)->nTextRows || (USE_MIXED_BUTTONS(ptb) && !((ptbb)->fsStyle & BTNS_SHOWTEXT)))
#define BTN_IS_AUTOSIZE(ptb, ptbb) (((ptbb)->fsStyle & BTNS_AUTOSIZE) || (USE_MIXED_BUTTONS(ptb) && !((ptbb)->fsStyle & BTNS_SEP)))
#define DRAW_MONO_BTN(ptb, state)   (!(state & TBSTATE_ENABLED) || ((ptb->ci.style & WS_DISABLED) && ptb->ci.iVersion >= 5))

// Globals - since all of these globals are used durring a paint we have to
// take a criticial section around all toolbar paints.  this sucks.
//

const UINT wStateMasks[] = {
    TBSTATE_ENABLED,
    TBSTATE_CHECKED,
    TBSTATE_PRESSED,
    TBSTATE_HIDDEN,
    TBSTATE_INDETERMINATE,
    TBSTATE_MARKED
};

#define TBISSTRINGPTR(iString)  (((iString) != -1) && (!IS_INTRESOURCE(iString)))

#define TBDraw_State(ptbdraw)   ((ptbdraw)->tbcd.nmcd.uItemState)

LRESULT CALLBACK ToolbarWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
void TBOnButtonStructSize(PTBSTATE ptb, UINT uStructSize);
BOOL SetBitmapSize(PTBSTATE ptb, int width, int height);
int  AddBitmap(PTBSTATE ptb, int nButtons, HINSTANCE hBMInst, UINT_PTR wBMID);
void TBBuildImageList(PTBSTATE ptb);
BOOL GetInsertMarkRect(PTBSTATE ptb, LPRECT lpRect, BOOL fHorizMode);
LPTSTR TB_StrForButton(PTBSTATE ptb, LPTBBUTTONDATA pTBButton);
UINT TBGetDrawTextFlags(PTBSTATE ptb, UINT uiStyle, LPTBBUTTONDATA);
BOOL TBGetMaxSize( PTBSTATE ptb, LPSIZE lpsize );
void TBGetItem(PTBSTATE ptb,LPTBBUTTONDATA ptButton, LPNMTBDISPINFO ptbdi);

#define GT_INSIDE       0x0001
#define GT_MASKONLY     0x0002
BOOL GrowToolbar(PTBSTATE ptb, int newButWidth, int newButHeight, UINT flags);


//Pager Control Functions
LRESULT TB_OnScroll(PTBSTATE ptb, LPNMHDR pnm);
LRESULT TB_OnPagerControlNotify(PTBSTATE ptb,LPNMHDR pnm);
void TBAutoSize(PTBSTATE ptb);
LRESULT TB_OnCalcSize(PTBSTATE ptb, LPNMHDR pnm);

#define TBInvalidateImageList(ptb)  ((ptb)->fHimlValid = FALSE)
#define TBHasStrings(ptb)  ((ptb)->nStrings || (ptb)->fNoStringPool)

#ifdef DEBUG
#if 0
void _InvalidateRect(HWND hwnd, LPRECT prc, BOOL fInval)
{
    if (!(GetAsyncKeyState(VK_SHIFT) < 0) )
        InvalidateRect(hwnd, prc, fInval);
}
void _RedrawWindow(HWND hwnd, LPRECT prc, HANDLE hrgn, UINT uFlags)
{
    if (!(GetAsyncKeyState(VK_SHIFT) < 0) )
        RedrawWindow(hwnd, prc, hrgn, uFlags);
}

void _SetWindowPos(HWND hwnd, HWND hwnd2, int x, int y, int cx, int cy, UINT uFlags)
{
    if (GetAsyncKeyState(VK_SHIFT) < 0)
        uFlags &= ~( SWP_FRAMECHANGED);
    SetWindowPos(hwnd, hwnd2, x, y, cx, cy, uFlags);
}

#define InvalidateRect(hwnd, prc, fInval) _InvalidateRect(hwnd, prc, fInval)
#define RedrawWindow(hwnd, prc, hrgn, uFlags) _RedrawWindow(hwnd, prc, hrgn, uFlags)
#define SetWindowPos(hwnd, hwnd2, x, y, cx, cy, uFlags) _SetWindowPos(hwnd, hwnd2, x, y, cx, cy, uFlags)
#endif
#endif


__inline BOOL TB_IsDropDown(LPTBBUTTONDATA ptbb)
{
    BOOL fRet = (ptbb->fsStyle & (BTNS_DROPDOWN | BTNS_WHOLEDROPDOWN));

    return fRet;
}

__inline BOOL TB_HasDDArrow(PTBSTATE ptb, LPTBBUTTONDATA ptbb)
{
    BOOL fRet = (((ptb->dwStyleEx & TBSTYLE_EX_DRAWDDARROWS) &&
                        (ptbb->fsStyle & BTNS_DROPDOWN)) ||
                  (ptbb->fsStyle & BTNS_WHOLEDROPDOWN));

    return fRet;
}

__inline BOOL TB_HasSplitDDArrow(PTBSTATE ptb, LPTBBUTTONDATA ptbb)
{
    // If the button is both BTNS_DROPDOWN and BTNS_WHOLEDROPDOWN,
    // BTNS_WHOLEDROPDOWN wins.

    BOOL fRet = ((ptb->dwStyleEx & TBSTYLE_EX_DRAWDDARROWS) &&
                (ptbb->fsStyle & BTNS_DROPDOWN) &&
                !(ptbb->fsStyle & BTNS_WHOLEDROPDOWN));

    return fRet;
}

__inline BOOL TB_HasUnsplitDDArrow(PTBSTATE ptb, LPTBBUTTONDATA ptbb)
{
    BOOL fRet = (ptbb->fsStyle & BTNS_WHOLEDROPDOWN);

    return fRet;
}

__inline BOOL TB_HasTopDDArrow(PTBSTATE ptb, LPTBBUTTONDATA ptbb)
{
    BOOL fRet = (!(ptb->ci.style & TBSTYLE_LIST) &&
                TB_HasUnsplitDDArrow(ptb, ptbb) &&
                (ptb->nTextRows > 0) && TB_StrForButton(ptb, ptbb));

    return fRet;
}


BOOL TBIsHotTrack(PTBSTATE ptb, LPTBBUTTONDATA ptButton, UINT state)
{
    BOOL fHotTrack = FALSE;

    if ((ptb->ci.style & TBSTYLE_FLAT) && (&ptb->Buttons[ptb->iHot]==ptButton))
        fHotTrack = TRUE;

    // The following is in place to prevent hot tracking during the following conds:
    //  - drag & drop toolbar customization
    //  - when the mouse capture is on a particular button-press.
    // This does _not_ drop out of the loop because we don't want to break update
    // behavior; thus we'll have a little flickering on refresh as we pass over
    // these buttons.
    if (!(state & TBSTATE_PRESSED) && (GetKeyState (VK_LBUTTON) < 0) &&
        GetCapture() == ptb->ci.hwnd)
    {
        fHotTrack = FALSE;
    }

    if (!fHotTrack && (ptb->iPressedDD == ptButton - ptb->Buttons))
        fHotTrack = TRUE;

    return fHotTrack;
}


UINT StateFromCDIS(UINT uItemState)
{
    UINT state = 0;

    if (uItemState & CDIS_CHECKED)
        state |= TBSTATE_CHECKED;

    if (uItemState & CDIS_SELECTED)
        state |= TBSTATE_PRESSED;

    if (!(uItemState & CDIS_DISABLED))
        state |= TBSTATE_ENABLED;

    if (uItemState & CDIS_MARKED)
        state |= TBSTATE_MARKED;

    if (uItemState & CDIS_INDETERMINATE)
        state |= TBSTATE_INDETERMINATE;

    return state;
}


UINT CDISFromState(UINT state)
{
    UINT uItemState = 0;

    // Here are the TBSTATE - to - CDIS mappings:
    //
    //  TBSTATE_CHECKED         = CDIS_CHECKED
    //  TBSTATE_PRESSED         = CDIS_SELECTED
    // !TBSTATE_ENABLED         = CDIS_DISABLED
    //  TBSTATE_MARKED          = CDIS_MARKED
    //  TBSTATE_INDETERMINATE   = CDIS_INDETERMINATE
    //
    //  Hot tracked item        = CDIS_HOT
    //

    if (state & TBSTATE_CHECKED)
        uItemState |= CDIS_CHECKED;

    if (state & TBSTATE_PRESSED)
        uItemState |= CDIS_SELECTED;

    if (!(state & TBSTATE_ENABLED))
        uItemState |= CDIS_DISABLED;

    if (state & TBSTATE_MARKED)
        uItemState |= CDIS_MARKED;

    if (state & TBSTATE_INDETERMINATE)
        uItemState |= CDIS_INDETERMINATE;

    return uItemState;
}

void FlushToolTipsMgrNow(PTBSTATE ptb);

void TB_ForceCreateTooltips(PTBSTATE ptb)
{
    if (ptb->ci.style & TBSTYLE_TOOLTIPS && !ptb->hwndToolTips)
    {
        TOOLINFO ti;
        // don't bother setting the rect because we'll do it below
        // in TBInvalidateItemRects;
        ti.cbSize = sizeof(ti);
        ti.uFlags = TTF_IDISHWND;
        ti.hwnd = ptb->ci.hwnd;
        ti.uId = (UINT_PTR)ptb->ci.hwnd;
        ti.lpszText = 0;

#ifndef UNIX
        ptb->hwndToolTips = CreateWindow(c_szSToolTipsClass, NULL,
                                         WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                         ptb->ci.hwnd, NULL, HINST_THISDLL, NULL);
#else
        ptb->hwndToolTips = CreateWindowEx( WS_EX_MW_UNMANAGED_WINDOW, c_szSToolTipsClass, NULL,
                                         WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                         ptb->ci.hwnd, NULL, HINST_THISDLL, NULL);
#endif

        if (ptb->hwndToolTips) {
            int i;
            NMTOOLTIPSCREATED nm;

            CCSetInfoTipWidth(ptb->ci.hwnd, ptb->hwndToolTips);

            SendMessage(ptb->hwndToolTips, TTM_ADDTOOL, 0,
                        (LPARAM)(LPTOOLINFO)&ti);

            nm.hwndToolTips = ptb->hwndToolTips;
            CCSendNotify(&ptb->ci, NM_TOOLTIPSCREATED, &nm.hdr);

            // don't bother setting the rect because we'll do it below
            // in TBInvalidateItemRects;
            ti.uFlags = 0;
            ti.lpszText = LPSTR_TEXTCALLBACK;

            for (i = 0; i < ptb->iNumButtons; i++) {
                if (!(ptb->Buttons[i].fsStyle & BTNS_SEP)) {
                    ti.uId = ptb->Buttons[i].idCommand;
                    SendMessage(ptb->hwndToolTips, TTM_ADDTOOL, 0,
                                (LPARAM)(LPTOOLINFO)&ti);
                }
            }

            FlushToolTipsMgrNow(ptb);
        }
    }
}

void TBRelayToToolTips(PTBSTATE ptb, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    TB_ForceCreateTooltips(ptb);
    if (ptb->hwndToolTips) {
        RelayToToolTips(ptb->hwndToolTips, ptb->ci.hwnd, wMsg, wParam, lParam);
    }
}


LRESULT ToolbarDragCallback(HWND hwnd, UINT code, WPARAM wp, LPARAM lp)
{
    PTBSTATE ptb = (PTBSTATE)GetWindowInt(hwnd, 0);
    LRESULT lres;

    switch (code)
    {
    case DPX_DRAGHIT:
        if (lp)
        {
            POINT pt;
            int item;
            pt.x = ((POINTL *)lp)->x;
            pt.y = ((POINTL *)lp)->y;
            MapWindowPoints(NULL, ptb->ci.hwnd, &pt, 1);
            item = TBHitTest(ptb, pt.x, pt.y);

            if (0 <= item && item < ptb->iNumButtons)
                lres = (LRESULT)ptb->Buttons[item].idCommand;
            else
                lres = (LRESULT)-1;
        }
        else
            lres = -1;
        break;

    case DPX_GETOBJECT:
        lres = (LRESULT)GetItemObject(&ptb->ci, TBN_GETOBJECT, &IID_IDropTarget, (LPNMOBJECTNOTIFY)lp);
        break;

    case DPX_SELECT:
        if ((int)wp >= 0)
        {
            NMTBHOTITEM nmhi;
            nmhi.idNew = (int) wp;
            if (!CCSendNotify(&ptb->ci, TBN_DRAGOVER, &nmhi.hdr))
            {
                SendMessage(ptb->ci.hwnd, TB_MARKBUTTON, wp,
                    MAKELPARAM((lp != DROPEFFECT_NONE), 0));
            }
        }
        lres = 0;
        break;

    default:
        lres = -1;
        break;
    }

    return lres;
}

int TBMixedButtonHeight(PTBSTATE ptb, int iIndex)
{
    int iHeight;
    LPTBBUTTONDATA ptbb = &(ptb->Buttons[iIndex]);

    if (ptbb->fsStyle & BTNS_SHOWTEXT)                      // text and icon
        iHeight = max(ptb->iDyBitmap, ptb->dyIconFont);
    else                                                    // icon, no text
        iHeight = ptb->iDyBitmap;

    return iHeight;
}

int TBMixedButtonsHeight(PTBSTATE ptb)
{
    int i;
    int iHeightMax = 0;
    int iHeight;
    ASSERT(ptb->ci.style & TBSTYLE_LIST);
    ASSERT(USE_MIXED_BUTTONS(ptb));
    for (i = 0; i < ptb->iNumButtons; i++) {
        iHeight = TBMixedButtonHeight(ptb, i);
        iHeightMax = max(iHeightMax, iHeight);
    }
    return iHeightMax;
}

int HeightWithString(PTBSTATE ptb, int h)
{
    if (USE_MIXED_BUTTONS(ptb))
    {
        int hMixed = TBMixedButtonsHeight(ptb);
        return (max(h, hMixed));
    }
    else if (ptb->ci.style & TBSTYLE_LIST)
        return (max(h, ptb->dyIconFont));
    else if (ptb->dyIconFont)
        return (h + ptb->dyIconFont + 1);
    else
        return (h);
}

int TBGetSepHeight(PTBSTATE ptb, LPTBBUTTONDATA pbtn)
{
    ASSERT(pbtn->fsStyle & BTNS_SEP);

    if (ptb->ci.style & (CCS_VERT | TBSTYLE_FLAT) )
        return pbtn->DUMMYUNION_MEMBER(cxySep);
    else
        return pbtn->DUMMYUNION_MEMBER(cxySep) * 2 / 3;
}

UINT TBWidthOfString(PTBSTATE ptb, LPTBBUTTONDATA ptbb, HDC hdc)
{
    UINT uiWidth = 0;

    LPTSTR pstr = TB_StrForButton(ptb, ptbb);
    if (pstr)
    {
        HDC hdcCreated = NULL;
        HFONT hOldFont;
        UINT uiStyle;
        RECT rcText = {0,0,1000,10};

        if (!hdc)
        {
            hdcCreated = GetDC(ptb->ci.hwnd);
            hdc = hdcCreated;
        }
        hOldFont = SelectObject(hdc, ptb->hfontIcon);

        uiStyle = DT_CALCRECT | TBGetDrawTextFlags(ptb, 0, ptbb);
        DrawText(hdc, pstr, -1, &rcText, uiStyle);

        uiWidth += rcText.right;

        SelectObject(hdc, hOldFont);
        if (hdcCreated)
            ReleaseDC(ptb->ci.hwnd, hdcCreated);
    }

    return uiWidth;
}

// TBDDArrowAdjustment(ptb, ptbb): the amount by which we change the width of
// this button to accomodate the drop-down arrow.  not necessarily the same as
// ptb->dxDDArrowChar.
int TBDDArrowAdjustment(PTBSTATE ptb, LPTBBUTTONDATA ptbb)
{
    int iAdjust = 0;

    if (TB_HasDDArrow(ptb, ptbb))
    {
        // If a whole dd, non-autosize button, then we'll just use the standard
        // button width which ought to have room for this button (i.e., return 0).

        if (!TB_HasTopDDArrow(ptb, ptbb) || BTN_IS_AUTOSIZE(ptb, ptbb))
        {
            iAdjust += (WORD)ptb->dxDDArrowChar;

            if (TB_HasUnsplitDDArrow(ptb, ptbb))
            {
                // subtract off a bit since there won't be a border
                // around dd arrow part of this button
                iAdjust -= 2 * g_cxEdge;

                if (ptbb->iBitmap != I_IMAGENONE)
                {
                    // nudge over a bit more to overlap bitmap border padding
                    iAdjust -= g_cxEdge;
                }
            }

            if (TB_HasTopDDArrow(ptb, ptbb))
            {
                // If string width >= icon width + iAdjust, then no need
                // to add extra space for the arrow.

                if ((int)TBWidthOfString(ptb, ptbb, NULL) >= ptb->iDxBitmap + iAdjust)
                    iAdjust = 0;
            }
        }
    }

    return max(iAdjust, 0);
}

int TBWidthOfButton(PTBSTATE ptb, LPTBBUTTONDATA pButton, HDC hdc)
{
    RECT rc;
    if (BTN_IS_AUTOSIZE(ptb, pButton)) {
        // if they've set this button for autosize, calculate it and cache
        // it in cx
        if (BTN_NO_SHOW_TEXT(ptb, pButton)) {
            pButton->cx = 0;
            goto CalcIconWidth;
        }

        if (pButton->cx == 0) {
            UINT uiStringWidth = TBWidthOfString(ptb, pButton, hdc);
            pButton->cx = (WORD) ptb->xPad + uiStringWidth;

            if (uiStringWidth) {
                // Since we have a string for this button, we need to add
                // some padding around it.
                if ((ptb->ci.style & TBSTYLE_LIST) && TB_HasSplitDDArrow(ptb, pButton))
                    pButton->cx += (WORD) ptb->iDropDownGap;
                else
                    pButton->cx += 2 * g_cxEdge;
            }

CalcIconWidth:
            if (pButton->iBitmap != I_IMAGENONE) {

                if (ptb->ci.style & TBSTYLE_LIST) {
                    pButton->cx += ptb->iDxBitmap + ptb->iListGap;
                    if (BTN_NO_SHOW_TEXT(ptb, pButton))
                        pButton->cx += g_cxEdge * 2;
                }
                else {
                    // Use wider of string width (pButton->cx so far) and bitmap width.
                    pButton->cx = max(pButton->cx, ptb->iDxBitmap + ptb->xPad);
                }
            }

            pButton->cx += (USHORT)TBDDArrowAdjustment(ptb, pButton);
        }
    }

    if (pButton->cx) {
        return (int)pButton->cx;
    } else if (pButton->fsStyle & BTNS_SEP) {
        if (ptb->ci.style & CCS_VERT) {
            GetWindowRect(ptb->ci.hwnd, &rc);
            return RECTWIDTH(rc);
        } else {
            // Compat: Corel (Font navigator) expects the separators to be
            // 8 pixels wide.  So do not return pButton->cxySep here, since
            // that can be calculated differently depending on the flat style.
            //
            // No.  owner draw items are added by specifying separator, and
            // the iBitmap width which is then copied down to cxySep.
            // the preserving of size for corel needs to be done at that point.
            return pButton->DUMMYUNION_MEMBER(cxySep);
        }
    } else if (!(TBSTYLE_EX_VERTICAL & ptb->dwStyleEx)) {
        return ptb->iButWidth + TBDDArrowAdjustment(ptb, pButton);
    } else {
        return ptb->iButWidth;
    }
}

UINT TBGetDrawTextFlags(PTBSTATE ptb, UINT uiStyle, TBBUTTONDATA* ptbb)
{
    if (ptb->nTextRows > 1)
        uiStyle |= DT_WORDBREAK | DT_EDITCONTROL;
    else
        uiStyle |= DT_SINGLELINE;


    if (ptb->ci.style & TBSTYLE_LIST)
    {
        uiStyle |= DT_LEFT | DT_VCENTER | DT_SINGLELINE;
    }
    else
    {
        uiStyle |= DT_CENTER;
    }

    uiStyle &= ~(ptb->uDrawTextMask);
    uiStyle |= ptb->uDrawText;
    if (ptbb->fsStyle & BTNS_NOPREFIX)
        uiStyle |= DT_NOPREFIX;

#ifndef KEYBOARDCUES
    // This flag tells User's DrawText/Ex NOT to show the prefixes at all
    // when rendering. This only works on NT5.
    if (!ptb->fShowPrefix && g_bRunOnNT5)
#else
    if (CCGetUIState(&(ptb->ci)) & UISF_HIDEACCEL)
#endif
    {
        uiStyle |= DT_HIDEPREFIX;
    }
    return uiStyle;
}

BOOL TBRecalc(PTBSTATE ptb)
{
    TEXTMETRIC tm = {0};
    int i;
    HDC hdc;
    int cxMax = 0, cxMask, cy;
    HFONT hOldFont=NULL;

    if (ptb->fRedrawOff) {
        // redraw is off; defer recalc until redraw is turned back on
        ptb->fRecalc = TRUE;
        return TRUE;    // The recalc "succeeded" - actual work will happen later
    }

    ptb->dyIconFont = 0;
    if (!TBHasStrings(ptb) || !ptb->nTextRows ) {

        cxMax = ptb->iDxBitmap;
        cxMask = cxMax;

    } else {

        SIZE size = {0};
        LPCTSTR pstr;
        RECT rcText = {0,0,0,0};
        int cxExtra = ptb->xPad;

        ptb->iButWidth = 0;

        hdc = GetDC(ptb->ci.hwnd);
        if (!hdc)
            return(FALSE);

        if (ptb->hfontIcon)
            hOldFont = SelectObject(hdc, ptb->hfontIcon);
        GetTextMetrics(hdc, &tm);
        if (ptb->nTextRows)
            ptb->dyIconFont = (tm.tmHeight * ptb->nTextRows) +
                (tm.tmExternalLeading * (ptb->nTextRows - 1)); // add an edge ?

        if (ptb->ci.style & TBSTYLE_LIST)
            cxExtra += ptb->iDxBitmap + ptb->iListGap;

        // default to the image size...
        cxMax = ptb->iDxBitmap;

        // walk strings to find max width
        for (i = 0; i < ptb->iNumButtons; i++)
        {
            if (ptb->Buttons[i].fsState & TBSTATE_HIDDEN)
                continue;

            if (BTN_IS_AUTOSIZE(ptb, &ptb->Buttons[i]))
                ptb->Buttons[i].cx = 0;

            pstr = TB_StrForButton(ptb, &ptb->Buttons[i]);
            if (pstr) 
            {
                if ( ptb->ci.iVersion < 5 )
                {
                    // we used to use GetTextExtentPoint instead of DrawText.  This function would include the width
                    // of the "&" character if it was present.  As a result, it returned larger values and thus created
                    // wider buttons.  Without this extra fudge certain buttons will be about 6 pixels too narrow.
                    GetTextExtentPoint(hdc, pstr, lstrlen(pstr), &size);
                }
                else
                {
                    // wordbreak is not allowed in the calcrect w/ singleline
                    UINT uiStyle = DT_CALCRECT | DT_SINGLELINE | (TBGetDrawTextFlags(ptb, 0, &ptb->Buttons[i]) & ~DT_WORDBREAK);
                    RECT rcTemp = {0,0,0,0};
                    rcTemp.bottom = ptb->dyIconFont;

                    DrawText(hdc, pstr, -1, &rcTemp, uiStyle);
                    size.cx = RECTWIDTH(rcTemp);
                    size.cy = RECTHEIGHT(rcTemp);
                    // BUGBUG: size.cy stuff is fishy -- last one wins
                }
            }
            else
            {
                size.cx = 0;
            }

            if (TB_HasTopDDArrow(ptb, &ptb->Buttons[i])) {
                int iBmpWithArrow = CX_TOP_FUDGE + ptb->iDxBitmap + ptb->dxDDArrowChar;
                size.cx = max(size.cx, iBmpWithArrow);
            }
            else if ((ptb->dwStyleEx & TBSTYLE_EX_VERTICAL) && 
                TB_HasDDArrow(ptb, &ptb->Buttons[i])) {

                // for vertical toolbars, buttons with drop-down arrows
                // are drawn with the same width as normal buttons, so
                // we need to figure them into our max width calculation.

                size.cx += ptb->dxDDArrowChar;
            }

            if (cxMax < size.cx)
                cxMax = size.cx;
        }

        // if cxMax is less than the iButMinWidth - dxBitmap (if LIST) then
        // cxMax = iButMinWidth
        if (ptb->iButMinWidth && (ptb->iButMinWidth > (cxMax + cxExtra)))
            cxMax = ptb->iButMinWidth - cxExtra;

        cxMask = cxMax;

        // Is the cxMax +  dxBitmap (if LIST) more than the max width ?
        if (ptb->iButMaxWidth && (ptb->iButMaxWidth < (cxMax + cxExtra)))
        {
            int cyMax = 0;
            int cxTemp = 0;

            cxMax = ptb->iButMaxWidth - cxExtra;

            // But leave cxMask at its old value since AUTOSIZE buttons
            // are exempt from button truncation.  This exemption is a bug,
            // but IE4 shipped that way so we're stuck with it.  (You can
            // tell it's a bug because we go ahead and flip TBSTATE_ELLIPSIS
            // even on AUTOSIZE buttons, only to "forget" about the ellipsis
            // in TBWidthOfString().)

            // walk strings to set the TBSTATE_ELLIPSES
            for (i = 0; i < ptb->iNumButtons; i++)
            {
                BOOL fEllipsed = FALSE;
                UINT uiStyle;

                if (ptb->Buttons[i].fsState & TBSTATE_HIDDEN)
                    continue;

                if (BTN_NO_SHOW_TEXT(ptb, &ptb->Buttons[i]))
                    pstr = NULL;
                else
                {
                    pstr = TB_StrForButton(ptb, &ptb->Buttons[i]);
                    uiStyle = DT_CALCRECT | TBGetDrawTextFlags(ptb, 0, &ptb->Buttons[i]);
                }

                if (pstr) 
                {
                    int cxMaxText;
                    if ((ptb->dwStyleEx & TBSTYLE_EX_VERTICAL) && 
                        TB_HasDDArrow(ptb, &ptb->Buttons[i]))
                    {
                        // if a drop-down button on a vertical toolbar,
                        // need to make space for drop-down arrow
                        cxMaxText = cxMax - ptb->dxDDArrowChar;
                    } 
                    else 
                    {
                        cxMaxText = cxMax;
                    }
                    // DrawText doesn't like it when cxMaxText <= 0
                    cxMaxText = max(cxMaxText, 1);

                    rcText.bottom = ptb->dyIconFont;
                    rcText.right = cxMaxText;

                    DrawText(hdc, pstr, -1, &rcText, uiStyle);
                    if (ptb->nTextRows > 1)
                    {
                        // width is width of text plus width we might
                        // have lopped off for drop-down arrow
                        int cx = rcText.right + (cxMax - cxMaxText);
                        if (cx > cxTemp)
                        {
                            // this is our new multiline text hack max
                            cxTemp = cx;
                        }
                        fEllipsed = (BOOL)(rcText.bottom > ptb->dyIconFont);
                    }
                    else
                        fEllipsed = (BOOL)(rcText.right > cxMaxText);

                    if (cyMax < rcText.bottom)
                        cyMax = rcText.bottom;
                }

                if (fEllipsed)
                    ptb->Buttons[i].fsState |= TBSTATE_ELLIPSES;
                else
                    ptb->Buttons[i].fsState &= ~TBSTATE_ELLIPSES;
            }

            if (cxTemp && (ptb->nTextRows > 1 ))
                cxMax = cxTemp;

            // Set the text height to the tallest text, with the top end being the number
            // of rows specified by MAXTEXTROWS
            if (ptb->dyIconFont > cyMax)
                ptb->dyIconFont = cyMax;
        }
        else
        {
            for (i = 0; i < ptb->iNumButtons; i++)
                ptb->Buttons[i].fsState &= ~TBSTATE_ELLIPSES;

            if ((ptb->nTextRows) && ptb->iNumButtons && (ptb->dyIconFont > size.cy))
                ptb->dyIconFont = size.cy;
        }

        if (ptb->iButMinWidth && (ptb->iButMinWidth > (cxMax + cxExtra)))
            cxMax = ptb->iButMinWidth - cxExtra;

        if (hOldFont)
            SelectObject(hdc, hOldFont);
        ReleaseDC(ptb->ci.hwnd, hdc);
    }

    //
    //  Need to call GrowToolbar twice, once to grow the mask, and again
    //  to grow the buttons.  (Yes, this is sick.)
    //
    cy = HeightWithString(ptb, ptb->iDyBitmap);

    if (!GrowToolbar(ptb, max(cxMax, cxMask), cy, GT_INSIDE | GT_MASKONLY))
        return(FALSE);

    return(GrowToolbar(ptb, cxMax, cy, GT_INSIDE));
}

BOOL TBChangeFont(PTBSTATE ptb, WPARAM wParam, HFONT hFont)
{
    LOGFONT lf;
    BOOL fWasFontCreated = ptb->fFontCreated;

    if ((wParam != 0) && (wParam != SPI_SETICONTITLELOGFONT) && (wParam != SPI_SETNONCLIENTMETRICS))
        return(FALSE);

    if (!SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0))
        return(FALSE);

    if (!hFont) {
        if (!(hFont = CreateFontIndirect(&lf)))
            return(FALSE);
        ptb->fFontCreated = TRUE;
    } else {
        ptb->fFontCreated = FALSE;
    }

    if (ptb->hfontIcon && fWasFontCreated)
        DeleteObject(ptb->hfontIcon);

    ptb->hfontIcon = hFont;

    return(TBRecalc(ptb));
}

void TBSetFont(PTBSTATE ptb, HFONT hFont, BOOL fInval)
{
    TBChangeFont(ptb, 0, hFont);
    if (fInval)
        InvalidateRect(ptb->ci.hwnd, NULL, TRUE);

}

HWND WINAPI CreateToolbarEx(HWND hwnd, DWORD ws, UINT wID, int nBitmaps,
            HINSTANCE hBMInst, UINT_PTR wBMID, LPCTBBUTTON lpButtons,
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
        TBInsertButtons(ptb, (UINT)-1, iNumButtons, (LPTBBUTTON)lpButtons, TRUE);

        // ptb may be bogus now after above button insert
    }
Error:
    return hwndToolbar;
}

/* This is no longer declared in COMMCTRL.H.  It only exists for compatibility
** with existing apps; new apps must use CreateToolbarEx.
*/
HWND WINAPI CreateToolbar(HWND hwnd, DWORD ws, UINT wID, int nBitmaps, HINSTANCE hBMInst, UINT_PTR wBMID, LPCTBBUTTON lpButtons, int iNumButtons)
{
    // old-style toolbar, so no divider.
    return CreateToolbarEx(hwnd, ws | CCS_NODIVIDER, wID, nBitmaps, hBMInst, wBMID,
                lpButtons, iNumButtons, 0, 0, 0, 0, sizeof(OLDTBBUTTON));
}

#pragma code_seg(CODESEG_INIT)

BOOL InitToolbarClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInstance, c_szToolbarClass, &wc))
    {
        wc.lpfnWndProc   = (WNDPROC)ToolbarWndProc;

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

void PatB(HDC hdc,int x,int y,int dx,int dy, DWORD rgb)
{
    RECT    rc;

    SetBkColor(hdc,rgb);
    rc.left   = x;
    rc.top    = y;
    rc.right  = x + dx;
    rc.bottom = y + dy;

    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
}

#ifndef UNICODE
// Get actual number of characters that will be drawn into the given
// rectangle by DrawTextEx. This is to avoid using DT_END_ELLIPSIS
// on FarEast Win95 (golden) as it could sometimes put more characters
// than what the rect actually could hold.
UINT GetLengthDrawn(PTBSTATE ptb, HDC hdc, LPSTR psz, int cch, LPRECT lprc, UINT uiStyle)
{
    DRAWTEXTPARAMS dtParams = {0};
    HDC hdcMem;
    HFONT hfontOld;

    ASSERT(psz);
    ASSERT(lprc);
    ASSERT(ptb);

    hdcMem = CreateCompatibleDC(hdc);

    hfontOld=SelectObject(hdcMem, ptb->hfontIcon);

    dtParams.cbSize = sizeof (dtParams);
    DrawTextEx(hdcMem, (LPSTR)psz, cch, lprc, uiStyle, &dtParams);

    SelectObject(hdcMem, hfontOld);
    DeleteDC(hdcMem);

    return dtParams.uiLengthDrawn;
}
#endif
// Parameter fHighlight determines whether to draw text highlighted, for
// new TBSTATE_MARKED
//
void DrawString(HDC hdc, int x, int y, int dx, int dy, PTSTR pszString,
                            BOOL fHighlight, TBDRAWITEM * ptbdraw)
{
    int oldMode;
    COLORREF oldBkColor;
    COLORREF oldTextColor;
    RECT rcText;
    UINT uiStyle = 0;
    PTBSTATE ptb;
    LPTBBUTTONDATA ptbb;

    ASSERT(ptbdraw);

    ptb = ptbdraw->ptb;
    ptbb = ptbdraw->pbutton;

    if (!(ptb->ci.style & TBSTYLE_LIST) && ((ptb->iDyBitmap + ptb->yPad + g_cyEdge) >= ptb->iButHeight))
        // there's no room to show the text -- bail out
        return;

    if (BTN_NO_SHOW_TEXT(ptb, ptbb))
        // don't show text for this button -- bail out
        return;

    if (fHighlight)
    {
        oldMode = SetBkMode (hdc, ptbdraw->tbcd.nHLStringBkMode);
        oldBkColor = SetBkColor (hdc, ptbdraw->tbcd.clrMark);
        oldTextColor = SetTextColor (hdc, ptbdraw->tbcd.clrTextHighlight);
    }
    else
        oldMode = SetBkMode(hdc, ptbdraw->tbcd.nStringBkMode);

    uiStyle = TBGetDrawTextFlags(ptb, DT_END_ELLIPSIS, ptbb);

    // If we're ex_vertical want to center the text
    if (!(ptb->dwStyleEx & TBSTYLE_EX_VERTICAL))
    {
        if (ptb->ci.style & TBSTYLE_LIST)
        {
            dy = max(ptb->dyIconFont, ptb->iDyBitmap);
        }
        else
        {
            if (!dy || ptb->dyIconFont < dy)
                dy = ptb->dyIconFont;
        }
    }


    SetRect( &rcText, x, y, x + dx, y + dy);

#ifndef UNICODE
    if (g_fDBCSEnabled && !g_bRunOnMemphis
       && (ptbb->fsState & TBSTATE_ELLIPSES)
       && (ptb->nTextRows > 1))
    {
        LPSTR psz;
        UINT uiLengthDrawn;
        // FarEast Win95 has a bug that DT_END_ELLIPSIS can
        // miscalculate the number of characters that fit in
        // the specified rectangle. We have to avoid to use
        // DT_END_ELLIPSIS for the platform putting ellipsis
        // ourselves. Memphis will fix the bug so we won't do
        // this for them.
        //
        uiStyle &= ~DT_END_ELLIPSIS;

        psz = StrDup(pszString);
        if (psz)
        {
            uiLengthDrawn = GetLengthDrawn(ptb, hdc, psz, -1, &rcText, uiStyle);
            if (uiLengthDrawn > 3)
            {
                TruncateString(psz, uiLengthDrawn-2);
                lstrcat(psz, "...");
            }
            DrawText(hdc, (LPTSTR)psz, -1, &rcText, uiStyle);
            LocalFree(psz);
        }
    }
    else
#endif
    DrawText(hdc, (LPTSTR)pszString, -1, &rcText, uiStyle);

    SetBkMode(hdc, oldMode);
    if (fHighlight)
    {
        SetBkColor (hdc, oldBkColor);
        SetTextColor (hdc, oldTextColor);
    }
}

LPTSTR TB_StrForButton(PTBSTATE ptb, LPTBBUTTONDATA pTBButton)
{
    if (TBISSTRINGPTR(pTBButton->iString))
        return (LPTSTR)pTBButton->iString;
    else {
        if (pTBButton->iString != -1 &&
            pTBButton->iString < ptb->nStrings)
            return ptb->pStrings[pTBButton->iString];
        return NULL;
    }
}

HIMAGELIST TBGetImageList(PTBSTATE ptb, int iMode, int iIndex)
{
    HIMAGELIST himl = NULL;

    ASSERT(iMode <= HIML_MAX);
    if (iIndex >= 0 && iIndex < ptb->cPimgs) {
        himl = ptb->pimgs[iIndex].himl[iMode];
    }

    return himl;
}

//
//  v5 toolbars support multiple imagelists.  To use images from an alternate
//  imagelist, set the imagelist handle via TB_SETIMAGELIST(iIndex, himlAlt)
//  and set your button's iImage to MAKELONG(iImage, iIndex).
//
//  APP COMPAT:  GroupWise 5.5 passes crap as the iIndex (even though it
//  was documented as "must be zero"), so we enable this functionality
//  only for v5 toolbars.  IE4 ignored the iIndex, which is why they got
//  away with it up until now.
//
#define MAX_TBIMAGELISTS 20             // arbitrary limit

HIMAGELIST TBSetImageList(PTBSTATE ptb, int iMode, int iIndex, HIMAGELIST himl)
{
    HIMAGELIST himlOld = NULL;

    // Watch out for app compat or for totally bogus parameters
    if (ptb->ci.iVersion < 5 || iIndex < 0 || iIndex >= MAX_TBIMAGELISTS)
        iIndex = 0;

    ASSERT(iMode <= HIML_MAX);
    if (iIndex >= ptb->cPimgs) {
        // asking for more than we have, realloc.

        void *p = CCLocalReAlloc(ptb->pimgs, (iIndex+1) * SIZEOF(TBIMAGELISTS));
        if (p) {
            ptb->pimgs = (TBIMAGELISTS*)p;
            ZeroMemory(&ptb->pimgs[ptb->cPimgs], (iIndex + 1 - ptb->cPimgs) * sizeof(TBIMAGELISTS));
            ptb->cPimgs = iIndex + 1;  // iIndex is 0 based, but cPimgs is 1 based (it's a count, not an index)
        }
    }

    if (iIndex < ptb->cPimgs) {
        himlOld = ptb->pimgs[iIndex].himl[iMode];
        ptb->pimgs[iIndex].himl[iMode] = himl;
    }

    return himlOld;
}

// create a mono bitmap mask:
//   1's where color == COLOR_BTNFACE || COLOR_3DHILIGHT
//   0's everywhere else

void CreateMask(int xoffset, int yoffset, int dx, int dy, BOOL fDrawGlyph, TBDRAWITEM * ptbdraw)
{
    LPTSTR psz;
    IMAGELISTDRAWPARAMS imldp;
    HIMAGELIST himl;
    PTBSTATE ptb = ptbdraw->ptb;
    LPTBBUTTONDATA pTBButton = ptbdraw->pbutton;
    // initalize whole area with 1's
    PatBlt(ptb->hdcMono, 0, 0, dx, dy, WHITENESS);

    // create mask based on color bitmap
    // convert this to 1's

    himl = TBGetImageList(ptb, HIML_NORMAL, ptbdraw->iIndex);
    if (fDrawGlyph && himl)
    {
        imldp.cbSize = sizeof(imldp);
        imldp.himl   = himl;
        imldp.i      = ptbdraw->iImage;
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

    psz = TB_StrForButton(ptb, pTBButton);
    if (psz)
    {
        xoffset = 1;
        yoffset = 1;

        if (ptb->ci.style & TBSTYLE_LIST)
        {
            if (!(pTBButton->iBitmap == I_IMAGENONE &&
                (pTBButton->fsStyle & BTNS_AUTOSIZE)))
            {
                xoffset += ptb->iDxBitmap + ptb->iListGap;
                dx -= ptb->iDxBitmap + ptb->iListGap;
            }
        }
        else 
        {
            yoffset += ptb->iDyBitmap + 1;
            dy -= ptb->iDyBitmap + 1;
        }

        if (!(ptb->dwStyleEx & TBSTYLE_EX_VERTICAL))
        {
            dx -= g_cxEdge;
            dy -= g_cyEdge;
        }

        // The FALSE in 4th param is so we don't get a box in the mask.
        DrawString(ptb->hdcMono, xoffset, yoffset, dx, dy, psz,
                   FALSE, ptbdraw);
    }
}

void DrawBlankButton(HDC hdc, int x, int y, int dx, int dy, TBDRAWITEM * ptbdraw)
{
    RECT r1;
    UINT state;

    // face color
    // The Office toolbar sends us bitmaps that are smaller than they claim they are
    // So we need to do the PatB or the window background shows through around the
    // edges of the button bitmap  -jjk
    ASSERT(ptbdraw);

    state = ptbdraw->state;

    if (!(state & TBSTATE_CHECKED))
        PatB(hdc, x, y, dx, dy, ptbdraw->tbcd.clrBtnFace);

    if  ( !(ptbdraw->dwCustom & TBCDRF_NOEDGES))
    {
        r1.left = x;
        r1.top = y;
        r1.right = x + dx;
        r1.bottom = y + dy;

        DrawEdge(hdc, &r1, (state & (TBSTATE_CHECKED | TBSTATE_PRESSED)) ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT | BF_SOFT);
    }
}

// these are raster ops
#define DSPDxax     0x00E20746  // BUGBUG: not used
#define PSDPxax     0x00B8074A

HWND g_hwndDebug = NULL;

void DrawFace(HDC hdc, int x, int y, int offx, int offy, int dxText,
              int dyText, TBDRAWITEM * ptbdraw)
{
    LPTSTR psz;
    IMAGELISTDRAWPARAMS imldp;
    BOOL fHotTrack = FALSE;
    UINT state;
    PTBSTATE ptb;
    LPTBBUTTONDATA ptButton;
    BOOL fImage;        // !fImage means no image (as opposed to a blank image)

    ASSERT(ptbdraw);

    ptb = ptbdraw->ptb;
    ptButton = ptbdraw->pbutton;

    // AutosizeTextNoImage
    if ((ptb->ci.style & TBSTYLE_LIST) &&
        (ptbdraw->iImage == I_IMAGENONE) &&
        (ptButton->fsStyle & BTNS_AUTOSIZE)) {
        fImage = FALSE;
    } else {
        fImage = TRUE;
    }

    state = ptbdraw->state;

    if (state & TBSTATE_ENABLED)
    {
        fHotTrack = ptbdraw->fHotTrack;

        if (ptb->ci.style & TBSTYLE_FLAT)
        {
            UINT bdr = 0;

            if (state & (TBSTATE_PRESSED | TBSTATE_CHECKED))
                bdr = BDR_SUNKENOUTER;
            else if (fHotTrack)
                bdr = BDR_RAISEDINNER;

            if (bdr)
            {
                RECT rc;
                TB_GetItemRect(ptb, (UINT)(ptButton - ptb->Buttons), &rc);

                if (TB_HasSplitDDArrow(ptb, ptButton))
                    rc.right -= ptb->dxDDArrowChar;

                if (!(ptbdraw->dwCustom & TBCDRF_NOEDGES) && ptb)
                    CCDrawEdge(hdc, &rc, bdr, BF_RECT, &(ptb->clrsc));
            }
        }
    }

    imldp.himl = NULL;

    if (fHotTrack || (state & TBSTATE_CHECKED)) {
        imldp.himl   = TBGetImageList(ptb, HIML_HOT, ptbdraw->iIndex);
        if (!imldp.himl)
            imldp.himl = TBGetImageList(ptb, HIML_NORMAL, ptbdraw->iIndex);
    } else if (DRAW_MONO_BTN(ptb, state) && (imldp.himl = TBGetImageList(ptb, HIML_DISABLED, ptbdraw->iIndex))) {
        // assigned in if statement
    } else if (imldp.himl = TBGetImageList(ptb, HIML_NORMAL, ptbdraw->iIndex)) {
        // assigned in if statement
    }

    if (imldp.himl && (ptbdraw->iImage != -1) && fImage)
    {
        COLORREF rgbBk = ptbdraw->tbcd.clrBtnFace;
        if (ptb->ci.style & TBSTYLE_TRANSPARENT) 
            rgbBk = CLR_NONE;
        
        if (ptb->dwStyleEx & TBSTYLE_EX_INVERTIBLEIMAGELIST)
            rgbBk = CLR_DEFAULT;

        imldp.cbSize = sizeof(imldp);
        imldp.i      = ptbdraw->iImage;
        imldp.hdcDst = hdc;
        imldp.x      = x + offx;
        imldp.y      = y + offy;
        imldp.cx     = 0;
        imldp.cy     = 0;
        imldp.xBitmap= 0;
        imldp.yBitmap= 0;
        imldp.rgbBk  = rgbBk;
        imldp.rgbFg  = CLR_DEFAULT;
        imldp.fStyle = ILD_NORMAL;
        if (state & (TBSTATE_CHECKED | TBSTATE_INDETERMINATE))
            imldp.fStyle = ILD_TRANSPARENT;

#ifdef TBHIGHLIGHT_GLYPH
        if ((state & TBSTATE_MARKED) && !(ptbdraw->dwCustom & TBCDRF_NOMARK))
            imldp.fStyle = ILD_TRANSPARENT | ILD_BLEND50;
#endif

        if (ptbdraw->dwCustom & TBCDRF_BLENDICON)
            imldp.fStyle = ILD_TRANSPARENT | ILD_BLEND50;

        ImageList_DrawIndirect(&imldp);
#ifdef DEBUG
        if (g_hwndDebug == ptb->ci.hwnd) {
            imldp.hdcDst = GetDC(NULL);
            ImageList_DrawIndirect(&imldp);
            ReleaseDC(NULL, imldp.hdcDst);
        }
#endif
    }

    psz = TB_StrForButton(ptb, ptButton);
    if (psz)
    {
        BOOL bHighlight = (state & TBSTATE_MARKED) && (ptb->ci.style & TBSTYLE_LIST) &&
                          !(ptbdraw->dwCustom & TBCDRF_NOMARK);

        if ((state & (TBSTATE_PRESSED | TBSTATE_CHECKED)) &&
            !(ptbdraw->dwCustom & TBCDRF_NOOFFSET))
        {
            x++;
            if (ptb->ci.style & TBSTYLE_LIST)
                y++;
        }

        if (ptb->ci.style & TBSTYLE_LIST)
        {
            if (fImage)
            {
                x += ptb->iDxBitmap + ptb->iListGap;
                dxText -= ptb->iDxBitmap + ptb->iListGap;
            }
            else
            {
                // fudge for I_IMAGENONE buttons
                x += g_cxEdge;
            }
        }
        else
        {
            y += offy + ptb->iDyBitmap;
            dyText -= offy + ptb->iDyBitmap;
        }

        DrawString(hdc, x + 1, y + 1, dxText, dyText, psz, bHighlight, ptbdraw);
    }
}

void InitTBDrawItem(TBDRAWITEM * ptbdraw, PTBSTATE ptb, LPTBBUTTONDATA pbutton,
                    UINT state, BOOL fHotTrack, int dxText, int dyText)
{
    NMTBCUSTOMDRAW * ptbcd;
    NMCUSTOMDRAW * pnmcd;

    ASSERT(ptbdraw);

    ptbdraw->ptb = ptb;
    ptbdraw->pbutton = pbutton;
    ptbdraw->fHotTrack = fHotTrack;
    ptbdraw->iIndex = GET_HIML_INDEX(pbutton->DUMMYUNION_MEMBER(iBitmap));
    ptbdraw->iImage = GET_IMAGE_INDEX(pbutton->DUMMYUNION_MEMBER(iBitmap));
    ptbdraw->state = state;

    ptbcd = &ptbdraw->tbcd;

    ptbcd->hbrMonoDither = g_hbrMonoDither;
    ptbcd->hbrLines = GetStockObject(BLACK_BRUSH);
    ptbcd->hpenLines = GetStockObject(BLACK_PEN);
    ptbcd->clrMark = g_clrHighlight;
    ptbcd->clrBtnHighlight = g_clrBtnHighlight;
    ptbcd->clrTextHighlight = g_clrHighlightText;
    ptbcd->clrBtnFace = g_clrBtnFace;
    ptbcd->nStringBkMode = TRANSPARENT;
    ptbcd->nHLStringBkMode = OPAQUE;
    ptbcd->clrText = g_clrBtnText;
    SetRect(&ptbcd->rcText, 0, 0, dxText, dyText);

    pnmcd = (NMCUSTOMDRAW *)ptbcd;

    pnmcd->uItemState = CDISFromState(state);

#ifdef KEYBOARDCUES
#if 0
    // BUGBUG: Custom draw stuff for UISTATE (stephstm)
    if (CCGetUIState(&(ptb->ci), KC_TBD))
        pnmcd->uItemState |= CDIS_SHOWKEYBOARDCUES;
#endif
#endif
    if ((ptb->ci.style & TBSTYLE_FLAT) && fHotTrack)
        pnmcd->uItemState |= CDIS_HOT;
}

void DrawButton(HDC hdc, int x, int y, PTBSTATE ptb, LPTBBUTTONDATA ptButton, BOOL fActive)
{
    // BUGBUG: cleanup -- separate layout calculation & rendering

    int yOffset;
    HBRUSH hbrOld;
    UINT state;
    int dxFace, dyFace;
    int dxText, dyText;
    int xCenterOffset;
    int dx = TBWidthOfButton(ptb, ptButton, hdc);
    HFONT oldhFont;
    int dy = ptb->iButHeight;
    TBDRAWITEM tbdraw = { 0 };
    NMTBCUSTOMDRAW * ptbcd = &tbdraw.tbcd;
    NMCUSTOMDRAW * pnmcd = (NMCUSTOMDRAW *)ptbcd;
    COLORREF clrSave;
    BOOL fHotTrack;
    HFONT hFontNoAntiAlias = NULL;

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

    if (!fActive) {
        state &= ~TBSTATE_ENABLED;
    }

    fHotTrack = TBIsHotTrack(ptb, ptButton, state);

    pnmcd->hdc = hdc;
    pnmcd->dwItemSpec = ptButton->idCommand;
    pnmcd->uItemState = 0;
    pnmcd->lItemlParam = (LPARAM)ptButton->dwData;
    SetRect(&pnmcd->rc, x, y, x + dx, y + dy);

    dxText = dx - (3 * g_cxEdge);

    if (ptb->dwStyleEx & TBSTYLE_EX_VERTICAL)
    {
        dyText = dy;
    }
    else
    {
        dyText = dy - (2 * g_cyEdge);
    }

    InitTBDrawItem(&tbdraw, ptb, ptButton, state, fHotTrack, dxText, dyText);

    tbdraw.dwCustom = CICustomDrawNotify(&ptb->ci, CDDS_ITEMPREPAINT, (NMCUSTOMDRAW *)ptbcd);

    // We gotta update our concept of hotness
    tbdraw.fHotTrack = fHotTrack = pnmcd->uItemState & CDIS_HOT;

    if (!(tbdraw.dwCustom & CDRF_SKIPDEFAULT ))
    {
        // Get the state back from what custom draw may have set
        state = tbdraw.state = StateFromCDIS(pnmcd->uItemState);

        dxFace = dx - (2 * g_cxEdge);
        dyFace = dy - (2 * g_cyEdge);
        dxText = ptbcd->rcText.right - ptbcd->rcText.left;
        dyText = ptbcd->rcText.bottom - ptbcd->rcText.top;

        if (TB_HasDDArrow(ptb, ptButton) && !TB_HasTopDDArrow(ptb, ptButton)) {
            int iAdjust = TBDDArrowAdjustment(ptb, ptButton);
            dxFace -= iAdjust;
            dxText -= iAdjust;
        }

        // Should we display the font using the GDI AntiAliasing?
        if (!ptb->fAntiAlias)
        {
            // No. Must be doing drag and drop. We don't want to AntiAlias because the
            // Purple color key will show through and it looks ugly.
            LOGFONT lfFont;

            if (GetObject(ptb->hfontIcon, sizeof(lfFont), &lfFont))
            {
                lfFont.lfQuality = NONANTIALIASED_QUALITY;
                hFontNoAntiAlias = CreateFontIndirect(&lfFont);
            }
        }

        if (hFontNoAntiAlias)
            oldhFont = SelectObject(hdc, hFontNoAntiAlias);
        else
            oldhFont = SelectObject(hdc, ptb->hfontIcon);

        clrSave = SetTextColor(hdc, ptbcd->clrText);

        if (!(ptb->ci.style & TBSTYLE_FLAT))
            DrawBlankButton(hdc, x, y, dx, dy, &tbdraw);


        // move coordinates inside border and away from upper left highlight.
        // the extents change accordingly.
        x += g_cxEdge;
        y += g_cyEdge;

        if (ptb->dwStyleEx & TBSTYLE_EX_VERTICAL)
        {
            yOffset = (ptb->iButHeight - ptb->iDyBitmap) / 2;
        }
        else
        {
            // calculate offset of face from (x,y).  y is always from the top,
            // so the offset is easy.  x needs to be centered in face.
            // center it taking the padding into account the padding area
            yOffset = (ptb->yPad - (2 * g_cyEdge)) / 2;
        }

        if (yOffset < 0)
            yOffset = 0;


        if ((ptb->ci.style & TBSTYLE_LIST) && !BTN_NO_SHOW_TEXT(ptb, ptButton)) {
            xCenterOffset = ptb->xPad / 2;
        } else if (TB_HasTopDDArrow(ptb, ptButton)) {
            //
            // Layout of "top dropdown" buttons looks like this:
            //
            //       icon            
            // fudge   |  dropdown arrow
            //    |    |    |
            //    v    v    v
            // +-+-+-------+--+-+
            // | | |       |  | |
            // | | |       |  | |
            // +-+-+-------+--+-+
            // |     <text>     |
            // +----------------+
            //
            // |<--- dxFace --->|
            //
            // xCenterOffset is the offset at which to start drawing the icon.
            //
            xCenterOffset = (dxFace + CX_TOP_FUDGE - (ptb->iDxBitmap + ptb->dxDDArrowChar)) / 2;
        } else {
            xCenterOffset = (dxFace - ptb->iDxBitmap) / 2;
        }

        if (state & (TBSTATE_PRESSED | TBSTATE_CHECKED) &&
            !(tbdraw.dwCustom & TBCDRF_NOOFFSET))
        {
            // pressed state moves down and to the right
            xCenterOffset++;
            yOffset++;
        }


        // draw the dithered background
        if  ((!fHotTrack || ptb->ci.iVersion < 5) &&
             (((state & (TBSTATE_CHECKED | TBSTATE_INDETERMINATE)) ||
              ((state & TBSTATE_MARKED) &&
               !(ptb->ci.style & TBSTYLE_FLAT) &&
               !(tbdraw.dwCustom & TBCDRF_NOMARK)))))
        {

            //Custom Draw can set hbrMonoDither to be NULL. Validate it before using it
            hbrOld = ptbcd->hbrMonoDither ? SelectObject(hdc, ptbcd->hbrMonoDither) : NULL;
            if (hbrOld)
            {
                COLORREF clrText, clrBack;

#ifdef TBHIGHLIGHT_BACK
                if (state & TBSTATE_MARKED)
                    clrText = SetTextColor(hdc, ptbcd->clrMark);
                else
#endif
                clrText = SetTextColor(hdc, ptbcd->clrBtnHighlight); // 0 -> 0
                clrBack = SetBkColor(hdc, ptbcd->clrBtnFace);        // 1 -> 1

                // only draw the dither brush where the mask is 1's
                PatBlt(hdc, x, y, dxFace, dyFace, PATCOPY);

                SelectObject(hdc, hbrOld);
                SetTextColor(hdc, clrText);
                SetBkColor(hdc, clrBack);
            }
        }

        // Paint the background of the hot-tracked item if the
        // custom draw said so
        if ((tbdraw.dwCustom & TBCDRF_HILITEHOTTRACK) && fHotTrack)
        {
            PatB(hdc, pnmcd->rc.left, pnmcd->rc.top,
                 pnmcd->rc.right - pnmcd->rc.left, pnmcd->rc.bottom - pnmcd->rc.top,
                 ptbcd->clrHighlightHotTrack);
        }

        tbdraw.iImage = ptButton->DUMMYUNION_MEMBER(iBitmap);
        if((ptButton->DUMMYUNION_MEMBER(iBitmap) == I_IMAGECALLBACK) && ptb->fHimlNative)
        {
            NMTBDISPINFO  tbgdi = {0};
            tbgdi.dwMask  = TBNF_IMAGE;
            TBGetItem(ptb,ptButton,&tbgdi);
            tbdraw.iImage = tbgdi.iImage;
        }
        tbdraw.iIndex = GET_HIML_INDEX(tbdraw.iImage);
        tbdraw.iImage = GET_IMAGE_INDEX(tbdraw.iImage);

        // Now put on the face.
        // TODO: Validate himlDisabled and ensure that the index is in range
        if (!DRAW_MONO_BTN(ptb, state) ||
            TBGetImageList(ptb, HIML_DISABLED, tbdraw.iIndex))
        {
            // regular version
            int yStart = y;

            if (ptb->dwStyleEx & TBSTYLE_EX_VERTICAL)
                yStart -= g_cyEdge;

            DrawFace(hdc, x, yStart, xCenterOffset, yOffset, dxText, dyText, &tbdraw);
        }

        if (DRAW_MONO_BTN(ptb, state))
        {
            HBITMAP hbmOld;

            //initialize the monochrome dc
            if (!ptb->hdcMono) {
                ptb->hdcMono = CreateCompatibleDC(hdc);
                if (!ptb->hdcMono)
                    return;
                SetTextColor(ptb->hdcMono, 0L);
                SelectObject(ptb->hdcMono, ptb->hfontIcon);
            }

            hbmOld = SelectObject(ptb->hdcMono, ptb->hbmMono);

            //
            // If we a mirrored DC, mirror the Memory DC so that
            // text written on the bitmap won't get flipped.
            //
            if ((IS_DC_RTL_MIRRORED(hdc)) &&
                (!(IS_DC_RTL_MIRRORED(ptb->hdcMono))))
            {
                SET_DC_RTL_MIRRORED(ptb->hdcMono);
            }


            // disabled version (or indeterminate)
            CreateMask(xCenterOffset, yOffset, dxFace, dyFace, (TBGetImageList(ptb, HIML_DISABLED, tbdraw.iIndex) == NULL), &tbdraw);

            SetTextColor(hdc, 0L);       // 0's in mono -> 0 (for ROP)
            SetBkColor(hdc, 0x00FFFFFF); // 1's in mono -> 1

            // draw glyph's etched-effect
            if (!(state & TBSTATE_INDETERMINATE) &&
                !(tbdraw.dwCustom & TBCDRF_NOETCHEDEFFECT)) {

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

        if (TB_HasDDArrow(ptb, ptButton))
        {
            WORD wDSAFlags = DCHF_TRANSPARENT | DCHF_FLIPPED;
            BOOL fPressedDD = ((ptb->Buttons + ptb->iPressedDD) == ptButton);

            RECT rc;
            if (TB_HasTopDDArrow(ptb, ptButton)) {
                // position the dd arrow up next to the bitmap
                rc.left = x + xCenterOffset + ptb->iDxBitmap;
                rc.right = rc.left + ptb->dxDDArrowChar;
                rc.top = y + yOffset;
                rc.bottom = rc.top + ptb->iDyBitmap;
            }
            else 
            {
                // position the dd arrow to the right of the text & bitmap
                TB_GetItemRect(ptb, (UINT)(ptButton - ptb->Buttons), &rc);
                rc.left = rc.right - ptb->dxDDArrowChar;
            }

            if (TB_HasUnsplitDDArrow(ptb, ptButton)) {
                // if a non-split dd arrow, don't draw a border.
                wDSAFlags |= DCHF_NOBORDER;
            }

            if (DRAW_MONO_BTN(ptb, state)) {
                // DFCS_INACTIVE means "draw the arrow part grayed"
                wDSAFlags |= DCHF_INACTIVE;
            }
            // if TB_HasTopDDArrow, we've already offset rect, so don't draw DCHF_PUSHED
            else if ((fPressedDD || (state & (TBSTATE_CHECKED | TBSTATE_PRESSED))) &&
                   !TB_HasTopDDArrow(ptb, ptButton)) {
                // DCHF_PUSHED means "offset the arrow and draw indented border"
                wDSAFlags |= DCHF_PUSHED;
            } 
            else if (fHotTrack || !(ptb->ci.style & TBSTYLE_FLAT)) {
                // DCHF_HOT means "draw raised border"
                // non-flat dropdown arrows are either pushed or hot
                wDSAFlags |= DCHF_HOT;
            }

            DrawScrollArrow(hdc, &rc, wDSAFlags);
        }

        SelectObject(hdc, oldhFont);
        SetTextColor(hdc, clrSave);

        if (hFontNoAntiAlias)
        {
            DeleteObject(hFontNoAntiAlias);
        }
    }

    if (tbdraw.dwCustom & CDRF_NOTIFYPOSTPAINT)
        CICustomDrawNotify(&ptb->ci, CDDS_ITEMPOSTPAINT, (NMCUSTOMDRAW *)ptbcd);
}

// make sure that g_hbmMono is big enough to do masks for this
// size of button.  if not, fail.
BOOL CheckMonoMask(PTBSTATE ptb, int width, int height)
{
    BITMAP bm;
    HBITMAP hbmTemp;

    if (ptb->hbmMono) {
        GetObject(ptb->hbmMono, sizeof(BITMAP), &bm);
        if (width <= bm.bmWidth && height <= bm.bmHeight) {
            return TRUE;
        }
    }


    // Add a bit of fudge to keep this from being reallocated too often.
    hbmTemp = CreateMonoBitmap(width+8, height+8);
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
** (GT_INSIDE) or a new external measurement.
**
** GT_MASKONLY updates the mono mask and nothing else.
*/
BOOL GrowToolbar(PTBSTATE ptb, int newButWidth, int newButHeight, UINT flags)
{
    if (!newButWidth)
        newButWidth = DEFAULTBUTTONX;
    if (!newButHeight)
        newButHeight = DEFAULTBUTTONY;

    // if growing based on inside measurement, get full size
    if (flags & GT_INSIDE)
    {
        if (ptb->ci.style & TBSTYLE_LIST)
            newButWidth += ptb->iDxBitmap + ptb->iListGap;

        newButHeight += ptb->yPad;
        newButWidth += ptb->xPad;

        // if toolbar already has strings, don't shrink width it because it
        // might clip room for the string
        if ((newButWidth < ptb->iButWidth) && ptb->nStrings &&
            (ptb->ci.iVersion < 5 || ptb->nTextRows > 0))
            newButWidth = ptb->iButWidth;
    }
    else {
        if (newButHeight == -1)
            newButHeight = ptb->iButHeight;
        if (newButWidth == -1)
            newButWidth = ptb->iButWidth;

        if (newButHeight < ptb->iDyBitmap + ptb->yPad)
            newButHeight = ptb->iDyBitmap + ptb->yPad;
        if (newButWidth < ptb->iDxBitmap + ptb->xPad)
            newButWidth = ptb->iDxBitmap + ptb->xPad;
    }

    // if the size of the toolbar is actually growing, see if shadow
    // bitmaps can be made sufficiently large.
    if (!ptb->hbmMono || (newButWidth > ptb->iButWidth) || (newButHeight > ptb->iButHeight)) {
        if (!CheckMonoMask(ptb, newButWidth, newButHeight))
            return(FALSE);
    }

    if (flags & GT_MASKONLY)
        return(TRUE);

    if (!(flags & GT_INSIDE) && ((ptb->iButWidth != newButWidth) || (ptb->iButHeight != newButHeight)))
        InvalidateRect(ptb->ci.hwnd, NULL, TRUE);

    ptb->iButWidth = newButWidth;
    ptb->iButHeight = newButHeight;

    // bar height has 2 pixels above, 2 below
    if (ptb->ci.style & TBSTYLE_FLAT)
        ptb->iYPos = 0;
    else
        ptb->iYPos = 2;

    TBInvalidateItemRects(ptb);

    return TRUE;
}

BOOL SetBitmapSize(PTBSTATE ptb, int width, int height)
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

    if (TBHasStrings(ptb))
        realh = HeightWithString(ptb, height);

    if (GrowToolbar(ptb, width, realh, GT_INSIDE)) {
        ptb->iDxBitmap = width;
        ptb->iDyBitmap = height;

        // the size changed, we need to rebuild the imagelist
        InvalidateRect(ptb->ci.hwnd, NULL, TRUE);
        TBInvalidateImageList(ptb);
        return TRUE;
    }
    return FALSE;
}

void TB_OnSysColorChange(PTBSTATE ptb)
{

    int i;
    InitGlobalColors();
    //  Reset all of the bitmaps

    for (i = 0; i < ptb->cPimgs; i++) {
        HIMAGELIST himl = TBGetImageList(ptb, HIML_NORMAL, i);
        if (himl)
            ImageList_SetBkColor(himl, (ptb->ci.style & TBSTYLE_TRANSPARENT) ? CLR_NONE : g_clrBtnFace);
        himl = TBGetImageList(ptb, HIML_HOT, i);
        if (himl)
            ImageList_SetBkColor(himl, (ptb->ci.style & TBSTYLE_TRANSPARENT) ? CLR_NONE : g_clrBtnFace);
    }
}

#define CACHE 0x01
#define BUILD 0x02


void PASCAL ReleaseMonoDC(PTBSTATE ptb)
{
    if (ptb->hdcMono) {
        SelectObject(ptb->hdcMono, g_hfontSystem);
        DeleteDC(ptb->hdcMono);
        ptb->hdcMono = NULL;
    }
}

void TB_OnEraseBkgnd(PTBSTATE ptb, HDC hdc)
{
    NMTBCUSTOMDRAW  tbcd = { 0 };
    DWORD           dwRes = FALSE;

    tbcd.nmcd.hdc = hdc;

    if (ptb->ci.style & TBSTYLE_CUSTOMERASE) {
        ptb->ci.dwCustom = CICustomDrawNotify(&ptb->ci, CDDS_PREERASE, (NMCUSTOMDRAW *)&tbcd);
    } else {
        ptb->ci.dwCustom = CDRF_DODEFAULT;
    }

    if (!(ptb->ci.dwCustom & CDRF_SKIPDEFAULT))
    {
        // for transparent toolbars, forward erase background to parent
        // but handle thru DefWindowProc in the event parent doesn't paint
        if (!(ptb->ci.style & TBSTYLE_TRANSPARENT) ||
            !CCForwardEraseBackground(ptb->ci.hwnd, hdc))
            DefWindowProc(ptb->ci.hwnd, WM_ERASEBKGND, (WPARAM) hdc, 0);
    }

    if (ptb->ci.dwCustom & CDRF_NOTIFYPOSTERASE)
        CICustomDrawNotify(&ptb->ci, CDDS_POSTERASE, (NMCUSTOMDRAW *)&tbcd);
}

void PASCAL DrawInsertMark(HDC hdc, LPRECT prc, BOOL fHorizMode, COLORREF clr)
{
    HPEN hPnMark = CreatePen(PS_SOLID, 1, clr);
    HPEN hOldPn;
    POINT rgPoint[4];
    if (!hPnMark)
        hPnMark = (HPEN)GetStockObject(BLACK_PEN);    // fallback to draw with black pen
    hOldPn = (HPEN)SelectObject(hdc, (HGDIOBJ)hPnMark);

    if ( fHorizMode )
    {
        int iXCentre = (prc->left + prc->right) /2;

        rgPoint[0].x = iXCentre + 1;
        rgPoint[0].y = prc->top + 2;
        rgPoint[1].x = iXCentre + 3;
        rgPoint[1].y = prc->top;
        rgPoint[2].x = iXCentre - 2;
        rgPoint[2].y = prc->top;
        rgPoint[3].x = iXCentre;
        rgPoint[3].y = prc->top + 2;

        // draw the top bit...
        Polyline( hdc, rgPoint, 4 );

        rgPoint[0].x = iXCentre;
        rgPoint[0].y = prc->top;
        rgPoint[1].x = iXCentre;
        rgPoint[1].y = prc->bottom - 1;
        rgPoint[2].x = iXCentre + 1;
        rgPoint[2].y = prc->bottom - 1;
        rgPoint[3].x = iXCentre + 1;
        rgPoint[3].y = prc->top;

        // draw the middle...
        Polyline( hdc, rgPoint, 4 );

        rgPoint[0].x = iXCentre + 1;
        rgPoint[0].y = prc->bottom - 3;
        rgPoint[1].x = iXCentre + 3;
        rgPoint[1].y = prc->bottom - 1;
        rgPoint[2].x = iXCentre - 2;
        rgPoint[2].y = prc->bottom - 1;
        rgPoint[3].x = iXCentre;
        rgPoint[3].y = prc->bottom - 3;

        // draw the bottom bit...
        Polyline( hdc, rgPoint, 4 );
    }
    else
    {
        int iYCentre = (prc->top + prc->bottom) /2;

        rgPoint[0].x = prc->left + 2;
        rgPoint[0].y = iYCentre;
        rgPoint[1].x = prc->left;
        rgPoint[1].y = iYCentre - 2;
        rgPoint[2].x = prc->left;
        rgPoint[2].y = iYCentre + 3;
        rgPoint[3].x = prc->left + 2;
        rgPoint[3].y = iYCentre + 1;

        // draw the top bit...
        Polyline( hdc, rgPoint, 4 );

        rgPoint[0].x = prc->left;
        rgPoint[0].y = iYCentre;
        rgPoint[1].x = prc->right - 1;
        rgPoint[1].y = iYCentre;
        rgPoint[2].x = prc->right - 1;
        rgPoint[2].y = iYCentre + 1;
        rgPoint[3].x = prc->left;
        rgPoint[3].y = iYCentre + 1;

        // draw the middle...
        Polyline( hdc, rgPoint, 4 );

        rgPoint[0].x = prc->right - 3;
        rgPoint[0].y = iYCentre;
        rgPoint[1].x = prc->right - 1;
        rgPoint[1].y = iYCentre - 2;
        rgPoint[2].x = prc->right - 1;
        rgPoint[2].y = iYCentre + 3;
        rgPoint[3].x = prc->right - 3;
        rgPoint[3].y = iYCentre + 1;

        // draw the bottom bit...
        Polyline( hdc, rgPoint, 4 );
    }

    SelectObject( hdc, hOldPn );
    DeleteObject((HGDIOBJ)hPnMark);
}

BOOL TBIsRectClipped(PTBSTATE ptb, LPRECT prc)
{
    RECT rc;
    RECT rcTB;

    if (ptb->dwStyleEx & TBSTYLE_EX_MULTICOLUMN)
        CopyRect(&rcTB, &ptb->rc);
    else
        GetClientRect(ptb->ci.hwnd, &rcTB);

    if (IntersectRect(&rc, &rcTB, prc)) {
        if (EqualRect(prc, &rc))
            return FALSE;
    }

    return TRUE;
}

BOOL TBShouldDrawButton(PTBSTATE ptb, LPRECT prcBtn, HDC hdc)
{
    // don't bother drawing buttons that aren't in the dc clipping region
    if (RectVisible(hdc, prcBtn)) {
        if (ptb->dwStyleEx & TBSTYLE_EX_HIDECLIPPEDBUTTONS)
            return !TBIsRectClipped(ptb, prcBtn);
        else
            return TRUE;
    }

    return FALSE;
}

// goin horizontal . . .
void DrawToolbarH(PTBSTATE ptb, HDC hdc, LPRECT prc)
{
    int iButton, xButton, yButton, cxBar;
    LPTBBUTTONDATA pAllButtons = ptb->Buttons;
    cxBar = prc->right - prc->left;

    yButton   = ptb->iYPos;
    prc->top    = ptb->iYPos;
    prc->bottom = ptb->iYPos + ptb->iButHeight;   // BUGBUG (scotth): what if first btn is a separator?


    for (iButton = 0, xButton = ptb->xFirstButton;
            iButton < ptb->iNumButtons; iButton++)
    {
        LPTBBUTTONDATA pButton = &pAllButtons[iButton];
        if (!(pButton->fsState & TBSTATE_HIDDEN))
        {
            int cxButton = TBWidthOfButton(ptb, pButton, hdc);

            // Is there anything to draw?
            if (!(pButton->fsStyle & BTNS_SEP) || (ptb->ci.style & TBSTYLE_FLAT))
            {
                // Yes
                prc->left = xButton;
                prc->right = xButton + cxButton;

                if (TBShouldDrawButton(ptb, prc, hdc))
                {
                    // Draw separator?
                    if (pButton->fsStyle & BTNS_SEP)
                    {
                        // Yes; must be a flat separator.  Is this toolbar vertical?
                        if (ptb->ci.style & CCS_VERT)
                        {
                            // Yes; draw a horizontal separator.  Center w/in the
                            // button rect
                            int iSave = prc->top;
                            prc->top += (TBGetSepHeight(ptb, pButton) - 1) / 2;
                            InflateRect(prc, -g_cxEdge, 0);
                            CCDrawEdge(hdc, prc, EDGE_ETCHED, BF_TOP, &(ptb->clrsc));
                            InflateRect(prc, g_cxEdge, 0);
                            prc->top = iSave;
                        }
                        else
                        {
                            // No; draw a vertical separator
                            prc->left += (cxButton - 1) / 2;
                            InflateRect(prc, 0, -g_cyEdge);
                            CCDrawEdge(hdc, prc, EDGE_ETCHED, BF_LEFT, &(ptb->clrsc));
                            InflateRect(prc, 0, g_cyEdge);
                        }
                    }
                    else
                    {
                        // No
                        DrawButton(hdc, xButton, yButton, ptb, pButton, ptb->fActive);
                    }
                }
            }

            xButton += (cxButton - s_dxOverlap);

            if (pButton->fsState & TBSTATE_WRAP)
            {
                int dy;

                if (pButton->fsStyle & BTNS_SEP)
                {
                    if (ptb->ci.style & CCS_VERT)
                        dy = TBGetSepHeight(ptb, pButton);
                    else
                    {
                        if (ptb->ci.style & TBSTYLE_FLAT)
                        {
                            // Draw a separator across the entire toolbar to separate rows.
                            // For horizontal toolbars only.
                            RECT rcMid;
                            rcMid.top = prc->top + ptb->iButHeight + ((TBGetSepHeight(ptb, pButton) - 1) / 2);
                            rcMid.bottom = rcMid.top + g_cxEdge;
                            rcMid.left = g_cxEdge;
                            rcMid.right = cxBar - g_cxEdge;

                            CCDrawEdge(hdc, &rcMid, EDGE_ETCHED, BF_TOP, &(ptb->clrsc));
                        }

                        dy = ptb->iButHeight + TBGetSepHeight(ptb, pButton);
                    }
                }
                else
                    dy = ptb->iButHeight;

                xButton = ptb->xFirstButton;
                yButton   += dy;
                prc->top    += dy;
                prc->bottom += dy;
            }
        }
    }
}

// goin vertical . . .
void DrawToolbarV(PTBSTATE ptb, HDC hdc, LPRECT prc)
{
    int iButton, xButton, yButton, cyBar;
    LPTBBUTTONDATA pAllButtons = ptb->Buttons;
    NMTBCUSTOMDRAW  tbcd = { 0 };
    LPTBBUTTONDATA pButton = pAllButtons;

    cyBar = prc->bottom - prc->top;

    xButton = ptb->xFirstButton;
    prc->left = xButton;
    prc->right = prc->left + ptb->iButWidth;

    for (iButton = 0, yButton = 0;
            iButton < ptb->iNumButtons; iButton++, pButton++)
    {
        if (!(pButton->fsState & TBSTATE_HIDDEN))
        {
            // Is there anything to draw?
            if (!(pButton->fsStyle & BTNS_SEP) || (ptb->ci.style & TBSTYLE_FLAT))
            {
                int cyButton;
                
                if (pButton->fsStyle & BTNS_SEP)
                    cyButton = TBGetSepHeight(ptb, pButton);
                else
                    cyButton = ptb->iButHeight;

                prc->top = yButton;
                prc->bottom = yButton + cyButton;

                if (TBShouldDrawButton(ptb, prc, hdc))
                {
                    // Draw separator?
                    if (pButton->fsStyle & BTNS_SEP)
                    {
                        DWORD dwCustRet;
                        NMTBCUSTOMDRAW  tbcd = { 0 };

                        tbcd.nmcd.hdc = hdc;
                        tbcd.nmcd.dwItemSpec = -1;
                        CopyRect(&tbcd.nmcd.rc, prc);

                        dwCustRet = CICustomDrawNotify(&ptb->ci, CDDS_ITEMPREPAINT, (NMCUSTOMDRAW *)&tbcd);

                        if ( !(CDRF_SKIPDEFAULT &  dwCustRet) )
                        {
                            // Yes; must be a flat separator.
                            InflateRect(prc, -g_cxEdge, 0);
                            CCDrawEdge(hdc, prc, EDGE_ETCHED, BF_TOP, &(ptb->clrsc));
                            InflateRect(prc, g_cxEdge, 0);
                        }
                    }
                    else
                    {
                        // No
                        DrawButton(hdc, xButton, yButton, ptb, pButton, ptb->fActive);
                    }
                }
                
                yButton += cyButton;
            }

            if (pButton->fsState & TBSTATE_WRAP)
            {
                int dx;
            
                if (ptb->ci.style & TBSTYLE_FLAT)
                {
                    // Draw a separator vertival across the entire toolbar to separate cols.
                    // For vertical toolbars only.

                    RECT rcMid;

                    rcMid.top = ptb->rc.top + g_cxEdge;
                    rcMid.bottom = ptb->rc.bottom - g_cxEdge;
                    rcMid.left = xButton + ptb->iButWidth;
                    rcMid.right = rcMid.left + g_cxEdge;
                    CCDrawEdge(hdc, &rcMid, EDGE_ETCHED, BF_LEFT, &(ptb->clrsc));
                }

                dx = ptb->iButWidth + g_cxEdge;

                yButton  = 0;
                xButton += dx;
                prc->left += dx;
                prc->right += dx;
            }
        }
    }
}

COLORREF TB_GetInsertMarkColor(PTBSTATE ptb)
{
    if (ptb->clrim == CLR_DEFAULT)
        return g_clrBtnText;
    else
        return ptb->clrim;
}

void TBPaint(PTBSTATE ptb, HDC hdcIn)
{
    RECT rc;
    HDC hdc;
    PAINTSTRUCT ps;
    NMTBCUSTOMDRAW  tbcd = { 0 };

    GetClientRect(ptb->ci.hwnd, &rc);
    if (hdcIn)
    {
        hdc = hdcIn;
    }
    else
        hdc = BeginPaint(ptb->ci.hwnd, &ps);

    if (!rc.right)
        goto Error1;

    tbcd.nmcd.hdc = hdc;
    tbcd.nmcd.rc = rc;
    ptb->ci.dwCustom = CICustomDrawNotify(&ptb->ci, CDDS_PREPAINT, (NMCUSTOMDRAW *)&tbcd);

    if (!(ptb->ci.dwCustom & CDRF_SKIPDEFAULT))
    {
        if (!ptb->fHimlValid)
            TBBuildImageList(ptb);

        if (ptb->dwStyleEx & TBSTYLE_EX_VERTICAL)
            DrawToolbarV(ptb, hdc, &rc);
        else
            DrawToolbarH(ptb, hdc, &rc);

        if (ptb->iInsert!=-1)
        {
            BOOL fHorizMode = !(ptb->ci.style & CCS_VERT);
            RECT rc;
            if (GetInsertMarkRect(ptb, &rc, fHorizMode))
            {
                DrawInsertMark(hdc, &rc, fHorizMode, TB_GetInsertMarkColor(ptb));
            }
        }

        ReleaseMonoDC(ptb);
    }

    if (ptb->ci.dwCustom & CDRF_NOTIFYPOSTPAINT)
    {
        tbcd.nmcd.hdc = hdc;
        tbcd.nmcd.uItemState = 0;
        tbcd.nmcd.lItemlParam = 0;
        CICustomDrawNotify(&ptb->ci, CDDS_POSTPAINT, (NMCUSTOMDRAW *)&tbcd);
    }

Error1:
    if (hdcIn == NULL)
        EndPaint(ptb->ci.hwnd, &ps);

}

void TB_GetItemDropDownRect(PTBSTATE ptb, UINT uButton, LPRECT lpRect)
{
    TB_GetItemRect(ptb,uButton,lpRect);
    lpRect->left = lpRect->right - ptb->dxDDArrowChar;
}

int TBHeightOfButton(PTBSTATE ptb, LPTBBUTTONDATA ptbb)
{
    int dy;

	if ((ptbb->fsStyle & BTNS_SEP)  && 
		(ptbb->fsState & TBSTATE_WRAP || ptb->dwStyleEx & TBSTYLE_EX_VERTICAL))
	{
		if (!(ptb->ci.style & CCS_VERT) && !(ptb->dwStyleEx & TBSTYLE_EX_VERTICAL)) 
		{
			dy = TBGetSepHeight(ptb, ptbb) + ptb->iButHeight;
		} 
		else 
		{
			dy = TBGetSepHeight(ptb, ptbb);
		}
	}
	else
	{
		dy = ptb->iButHeight;
	}

    return dy;
}

void TB_CalcItemRects(PTBSTATE ptb)
{
    int iButton, xPos, yPos;

    ASSERT(!ptb->fItemRectsValid);

    xPos = ptb->xFirstButton;
    yPos = ptb->iYPos;

    for (iButton = 0; iButton < ptb->iNumButtons; iButton++)
    {
        int xPosButton;
        LPTBBUTTONDATA pButton = &ptb->Buttons[iButton];

        if (!(pButton->fsState & TBSTATE_HIDDEN))
        {
            if ((pButton->fsState & TBSTATE_WRAP) && (pButton->fsStyle & BTNS_SEP))
                xPosButton = ptb->xFirstButton;
            else
                xPosButton = xPos;

            pButton->pt.x = xPosButton;
            pButton->pt.y = yPos;

            if (ptb->dwStyleEx & TBSTYLE_EX_VERTICAL)
            {
                if (pButton->fsState & TBSTATE_WRAP)
                {
                    xPos += (ptb->iButWidth + g_cxEdge);    // to not overwrite the edge.
                    yPos = 0;
                }
                else if (pButton->fsStyle & BTNS_SEP)
                    yPos += (TBGetSepHeight(ptb, pButton));
                else
                    yPos += ptb->iButHeight;
            }
            else // standard horizontal toolbar.
            {
                xPos += TBWidthOfButton(ptb, pButton, NULL) - s_dxOverlap;

                if (pButton->fsState & TBSTATE_WRAP)
                {
                    yPos += ptb->iButHeight;

                    if (pButton->fsStyle & BTNS_SEP)
                    {
                        if (ptb->ci.style & CCS_VERT) {
                            yPos -= ptb->iButHeight;
                        }
                        yPos += (TBGetSepHeight(ptb, pButton));
                    }

                    xPos = ptb->xFirstButton;
                }
            }
        }
    }
}

BOOL TB_GetItemRect(PTBSTATE ptb, UINT uButton, LPRECT lpRect)
{
    int dy = ptb->iButHeight;

    if (uButton >= (UINT)ptb->iNumButtons
        || (ptb->Buttons[uButton].fsState & TBSTATE_HIDDEN))
    {
        return FALSE;
    }

    if (!ptb->fItemRectsValid) {
        TB_CalcItemRects(ptb);
        ptb->fItemRectsValid = TRUE;
    }

    lpRect->left   = ptb->Buttons[uButton].pt.x;
    lpRect->right  = lpRect->left + TBWidthOfButton(ptb, &ptb->Buttons[uButton], NULL);
    lpRect->top    = ptb->Buttons[uButton].pt.y;
    lpRect->bottom = lpRect->top + TBHeightOfButton(ptb, &ptb->Buttons[uButton]);

    return TRUE;
}

void InvalidateButton(PTBSTATE ptb, LPTBBUTTONDATA pButtonToPaint, BOOL fErase)
{
    RECT rc;

    if (TB_GetItemRect(ptb, (UINT) (pButtonToPaint - ptb->Buttons), &rc))
    {
        InvalidateRect(ptb->ci.hwnd, &rc, fErase);
    }
}

/*----------------------------------------------------------
Purpose: Toggles the button as a dropdown

Returns: TRUE if handled
*/
BOOL TBToggleDropDown(PTBSTATE ptb, int iPos, BOOL fEatMsg)
{
    BOOL bRet = FALSE;
    LPTBBUTTONDATA ptbButton = &ptb->Buttons[iPos];

    ASSERT(TB_IsDropDown(ptbButton));

    if (ptbButton->fsState & TBSTATE_ENABLED)
    {
        UINT nVal;

        ptb->iPressedDD = iPos;

        if (TB_HasUnsplitDDArrow(ptb, ptbButton))
            ptbButton->fsState |= TBSTATE_PRESSED;

        InvalidateButton(ptb, ptbButton, TRUE);
        UpdateWindow(ptb->ci.hwnd);

        MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, ptb->ci.hwnd, OBJID_CLIENT, iPos+1);

        nVal = (UINT) SendItemNotify(ptb, ptbButton->idCommand, TBN_DROPDOWN);
        if (TBDDRET_DEFAULT == nVal || TBDDRET_TREATPRESSED == nVal)
        {
            if (fEatMsg)
            {
                MSG msg;

                PeekMessage(&msg, ptb->ci.hwnd, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE);
            }

            ptb->iPressedDD = -1;

            if (TB_HasUnsplitDDArrow(ptb, ptbButton))
                ptbButton->fsState &= ~TBSTATE_PRESSED;

            InvalidateButton(ptb, ptbButton, TRUE);
            UpdateWindow(ptb->ci.hwnd);

            MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, ptb->ci.hwnd, OBJID_CLIENT, iPos+1);
        }

        bRet = (TBDDRET_DEFAULT == nVal);
    }
    return bRet;
}


void TBInvalidateButton(PTBSTATE ptb, int i, BOOL fErase)
{
    if (i != -1) {
        InvalidateButton(ptb, &ptb->Buttons[i], fErase);
   }
}


void TBSetHotItem(PTBSTATE ptb, int iPos, DWORD dwReason)
{
    HWND hwnd;

    if ((ptb->ci.style & TBSTYLE_FLAT) ) {

        // Either one of these values can be -1, but refrain
        // from processing if both are negative b/c it is wasteful
        // and very common

        if ((ptb->iHot != iPos || (dwReason & HICF_RESELECT)) &&
            (0 <= ptb->iHot || 0 <= iPos) &&
            iPos < ptb->iNumButtons)
        {
            NMTBHOTITEM nmhot = {0};

            // Has the mouse moved away from the toolbar but
            // do we still anchor the highlight?
            if (0 > iPos && ptb->fAnchorHighlight && (dwReason & HICF_MOUSE))
                return ;        // Yes; deny the hot item change

            // Send a notification about the hot item change
            if (0 > ptb->iHot)
            {
                if (iPos >= 0)
                    nmhot.idNew = ptb->Buttons[iPos].idCommand;
                nmhot.dwFlags = HICF_ENTERING;
            }
            else if (0 > iPos)
            {
                if (ptb->iHot >= 0 && ptb->iHot < ptb->iNumButtons)
                    nmhot.idOld = ptb->Buttons[ptb->iHot].idCommand;
                nmhot.dwFlags = HICF_LEAVING;
            }
            else
            {
                if (ptb->iHot < ptb->iNumButtons)
                    nmhot.idOld = ptb->Buttons[ptb->iHot].idCommand;
                nmhot.idNew = ptb->Buttons[iPos].idCommand;
            }
            nmhot.dwFlags |= dwReason;

            // must save this for revalidation
            hwnd = ptb->ci.hwnd;

            if (CCSendNotify(&ptb->ci, TBN_HOTITEMCHANGE, &nmhot.hdr))
                return;         // deny the hot item change

            // Revalidate the window
            if (!IsWindow(hwnd)) return;

            TBInvalidateButton(ptb, ptb->iHot, TRUE);
            if ((iPos < 0) || !(ptb->Buttons[iPos].fsState & TBSTATE_ENABLED))
                iPos = -1;

            ptb->iHot = iPos;

            if (GetFocus() == ptb->ci.hwnd)
                MyNotifyWinEvent(EVENT_OBJECT_FOCUS, ptb->ci.hwnd, OBJID_CLIENT, iPos + 1);

            TBInvalidateButton(ptb, ptb->iHot, TRUE);

            if ((iPos >= 0 && iPos < ptb->iNumButtons) &&
                (TB_IsDropDown(&ptb->Buttons[iPos])) &&
                (dwReason & HICF_TOGGLEDROPDOWN))
            {
                TBToggleDropDown(ptb, iPos, FALSE);
            }
        }
    }
}

BOOL GetInsertMarkRect(PTBSTATE ptb, LPRECT prc, BOOL fHorizMode)
{
    BOOL fRet = TB_GetItemRect(ptb, ptb->iInsert, prc);
    if (fRet)
    {
        // if we are in horizontal mode, we need a vertical insertion marker
        if ( fHorizMode )
        {
            if (ptb->fInsertAfter)
                prc->left = prc->right;
            else
                prc->right = prc->left;

            prc->left -= INSERTMARKSIZE/2;
            prc->right += INSERTMARKSIZE/2 + 1;
        }
        else
        {
            if (ptb->fInsertAfter)
                prc->top = prc->bottom;
            else
                prc->bottom = prc->top;

            prc->top -= INSERTMARKSIZE/2;
            prc->bottom += INSERTMARKSIZE/2 + 1;
        }
    }
    return fRet;
}

void TBInvalidateMark(PTBSTATE ptb)
{
    RECT rc;

    if (GetInsertMarkRect(ptb, &rc, !(ptb->ci.style & CCS_VERT)))
    {
        InvalidateRect(ptb->ci.hwnd, &rc, TRUE);
    }
}

void TBSetInsertMark(PTBSTATE ptb, LPTBINSERTMARK ptbim)
{
    if (ptbim->iButton != ptb->iInsert ||
        BOOLIFY(ptb->fInsertAfter) != BOOLIFY(ptbim->dwFlags & TBIMHT_AFTER))
    {
        if (ptb->iInsert != -1)
            TBInvalidateMark(ptb);

        ptb->iInsert = ptbim->iButton;
        ptb->fInsertAfter = BOOLIFY(ptbim->dwFlags & TBIMHT_AFTER);

        if (ptb->iInsert != -1)
            TBInvalidateMark(ptb);
    }
}

void TBCycleHotItem(PTBSTATE ptb, int iStart, int iDirection, UINT nReason)
{
    int i;
    int iPrev;
    NMTBWRAPHOTITEM nmwh;

    nmwh.iDir = iDirection;
    nmwh.nReason = nReason;


    //When cycling around the menu, without this check, the second to last menu
    //item would be selected.
    if (iStart == -1 && iDirection == -1)
        iStart = 0;

    for (i = 0; i < ptb->iNumButtons; i++)
    {
        iPrev = iStart;
        iStart += iDirection + ptb->iNumButtons;
        iStart %= ptb->iNumButtons;

        if ( ( iPrev + iDirection >= ptb->iNumButtons) || (iPrev + iDirection < 0) )
        {
            nmwh.iStart = iStart;
            if (CCSendNotify(&ptb->ci, TBN_WRAPHOTITEM, &nmwh.hdr))
                return;
        }

        if (ptb->Buttons[iStart].fsState & TBSTATE_ENABLED &&
            !(ptb->Buttons[iStart].fsState & TBSTATE_HIDDEN) &&
            !(ptb->Buttons[iStart].fsStyle & BTNS_SEP))
        {
            // if the old hot item was dropped down, undrop it.
            if (ptb->iHot != -1 && ptb->iHot == ptb->iPressedDD)
                TBToggleDropDown(ptb, ptb->iHot, FALSE);

            TBSetHotItem(ptb, iStart, nReason);
            break;
        }
    }
}


// Do hit testing by sliding the origin of the supplied point
//
// returns:
//  >= 0    index of non separator item hit
//  < 0     index of separator or nearest non separator item (area
//          just below and to the left)
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

int TBHitTest(PTBSTATE ptb, int xPos, int yPos)
{
    int prev = 0;
    int last = 0;
    int i;
    RECT rc;

    if (ptb->iNumButtons == 0)
        return(-1);

    for (i=0; i<ptb->iNumButtons; i++)
    {
        if (TB_GetItemRect(ptb, i, &rc))
        {
            // ignore this button if hidden because of HideClippedButtons style
            if (!(ptb->dwStyleEx & TBSTYLE_EX_HIDECLIPPEDBUTTONS) || !(TBIsRectClipped(ptb, &rc)))
            {
                // From PtInRect docs:
                //   A point is within a rectangle if it lies on the left or top
                //   side or is within all four sides. A point on the right or
                //   bottom side is considered outside the rectangle.

                if (yPos >= rc.top && yPos < rc.bottom)
                {
                    if (xPos >= rc.left && xPos < rc.right)
                    {
                        if (ptb->Buttons[i].fsStyle & BTNS_SEP)
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
    }

    if (prev)
        return -1 - prev;
    else if (yPos > rc.bottom)
        // this means that we are off the bottom of the toolbar
        return(- i - 1);

    return last + 1;
}

// Same as above except:
//  - returns TRUE if the cursor is on the button edge.
//  - returns FALSE is the cursor is b/t buttons or on the button itself

BOOL TBInsertMarkHitTest(PTBSTATE ptb, int xPos, int yPos, LPTBINSERTMARK ptbim)
{
    TBINSERTMARK prev = {-1, TBIMHT_AFTER|TBIMHT_BACKGROUND}; // best guess if we hit a row
    TBINSERTMARK last = {-1, TBIMHT_AFTER|TBIMHT_BACKGROUND}; // best guess if we don't
    int i;

    // restrict hit testing depending upon whether we are vertical or horizontal
    BOOL fHorizMode = !(ptb->ci.style & CCS_VERT);

    for (i=0; i<ptb->iNumButtons; i++)
    {
        RECT rc;

        if (TB_GetItemRect(ptb, i, &rc))
        {
            if (yPos >= rc.top && yPos < rc.bottom)
            {
                if (xPos >= rc.left && xPos < rc.right)
                {
                    ptbim->iButton = i;

                    if ( fHorizMode )
                    {
                        if (xPos < rc.left + g_cxEdge*4)
                        {
                            ptbim->dwFlags = 0;
                            return TRUE;
                        }
                        else if (xPos > rc.right - g_cxEdge*4)
                        {
                            ptbim->dwFlags = TBIMHT_AFTER;
                            return TRUE;
                        }
                    }
                    else
                    {
                        // vertical....
                        if (yPos < rc.top + g_cyEdge*4)
                        {
                            ptbim->dwFlags = 0;
                            return TRUE;
                        }
                        else if (yPos > rc.bottom - g_cyEdge*4)
                        {
                            ptbim->dwFlags = TBIMHT_AFTER;
                            return TRUE;
                        }
                    }

                    // else we are just on a button...
                    ptbim->dwFlags = 0;
                    return FALSE;
                }
                else
                {
                    if (xPos < rc.left)
                    {
                        // since buttons are laid out left to right
                        // and rows are laid out top to bottom,
                        // if we ever hit this case, we can't hit anything else
                        ptbim->iButton = i;
                        ptbim->dwFlags = TBIMHT_BACKGROUND;
                        return FALSE;
                    }
                    else // (xPos > rc.right)
                    {
                        // remember the last one we've seen on this row
                        prev.iButton = i;
                    }
                }
            }
            else
            {
                if (yPos < rc.top)
                {
                    if (prev.iButton != -1)
                    {
                        *ptbim = prev;
                    }
                    else
                    {
                        ptbim->iButton = i;
                        ptbim->dwFlags = TBIMHT_BACKGROUND;
                    }
                }
                else
                {
                    // remember the last one we've seen
                    last.iButton = i;
                }
            }
        }
    }

    if (prev.iButton != -1)
        *ptbim = prev;
    else
        *ptbim = last;

    return FALSE;
}

int CountRows(PTBSTATE ptb)
{
    LPTBBUTTONDATA pButton, pBtnLast;
    int rows = 1;

    // BUGBUG (scotth): this doesn't look vertical-friendly
    // chrisny:  semantically no, technically it will work like a charm :-)

    pBtnLast = &(ptb->Buttons[ptb->iNumButtons]);
    for (pButton = ptb->Buttons; pButton<pBtnLast; pButton++) {
        if (pButton->fsState & TBSTATE_WRAP) {
            rows++;
            if (pButton->fsStyle & BTNS_SEP)
                rows++;
        }
    }

    return rows;
}

#define CountCols(ptb)  CountRows(ptb)

void WrapToolbarCol(PTBSTATE ptb, int dy, LPRECT lpRect, int *pCols)
{
    LPTBBUTTONDATA pButton, pBtnLast, pBtnPrev;
    LPTBBUTTONDATA pbtnLastVisible = NULL;
    LPTBBUTTONDATA pbtnPrev = NULL;
    int xPos, yPos;
    int dyButton;
    int yPosWrap = 0;
    int cCols = 1;

    DEBUG_CODE( int cItemsPerCol = 0; )

    ASSERT(ptb->dwStyleEx & TBSTYLE_EX_VERTICAL);
    TraceMsg(TF_TOOLBAR, "Toolbar: calculating WrapToolbar");

    // dy must be at least the button height, otherwise the final
    // rect is mis-calculated and will be too big.
    if (dy < ptb->iButHeight)
        dy = ptb->iButHeight;

    dyButton = ptb->iButHeight;
    xPos = ptb->xFirstButton;
    yPos = ptb->iYPos;
    pBtnLast = &(ptb->Buttons[ptb->iNumButtons]);
    ptb->szCached.cx = -1;
    ptb->szCached.cy = -1;

    if (pCols)
        (*pCols) = 1;

    pBtnPrev = ptb->Buttons;

    for (pButton = ptb->Buttons; pButton < pBtnLast; pButton++)
    {
        DEBUG_CODE( cItemsPerCol++; )

        // we nuke the wrap state at the start of the loop.
        // so we don't know if/when we are adding on a wrap bit that wasn't there
        // before.  we overstep the button, then back up when we've gone too far,
        pButton->fsState &= ~TBSTATE_WRAP;
        if (!(pButton->fsState & TBSTATE_HIDDEN))
        {
            if (pButton->fsStyle & BTNS_SEP)
                yPos += (TBGetSepHeight(ptb, pButton));
            else
                yPos += dyButton;
            // Is this button out of bounds?
            if (yPos > dy)
            {
                // Yes; wrap it.
                if ((pButton->fsStyle & BTNS_SEP) &&
                    yPos - TBGetSepHeight(ptb, pButton) > yPosWrap)
                {
                    yPosWrap = yPos - TBGetSepHeight(ptb, pButton); // wrap at first in next col.
                }
                else if (yPos - dyButton > yPosWrap)
                    yPosWrap = yPos - dyButton; // wrap at first in next col.

                if (xPos + ptb->iButWidth <= ptb->sizeBound.cx)
                    xPos += ptb->iButWidth;
                yPos = dyButton;
                cCols++;
                pBtnPrev->fsState |= TBSTATE_WRAP;

                DEBUG_CODE( cItemsPerCol = 0; )
            }
           // button in bounds gets handled above.
            pBtnPrev = pButton; // save previous for wrap point
        }
    }
    yPos = yPosWrap ? yPosWrap : yPos;
    if (pCols)
        *pCols = cCols;
    ptb->rc.left = 0;
    ptb->rc.right = xPos + ptb->iButWidth;
    ptb->rc.top = 0;
    ptb->rc.bottom = yPos;

    if (lpRect)
        CopyRect(lpRect, &ptb->rc);

    InvalidateRect(ptb->ci.hwnd, NULL, TRUE);
}

/**** WrapToolbar: * The buttons in the toolbar is layed out from left to right,
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
 * of buttons that are delimited by separators) that are taller than
 * or equal to two rows.
 */

void WrapToolbar(PTBSTATE ptb, int dx, LPRECT lpRect, int *pRows)
{
    BOOL fInvalidate = FALSE;
    LPTBBUTTONDATA pButton, pBtnT, pBtnLast;
    LPTBBUTTONDATA pbtnLastVisible = NULL;
    LPTBBUTTONDATA pbtnPrev = NULL;
    BOOL fLastVisibleWrapped = FALSE;
    int xPos, yPos, xMax;
    int dyButton;
    BOOL bWrapAtNextSeparator = FALSE;

    ASSERT(!(ptb->dwStyleEx & TBSTYLE_EX_VERTICAL));
    TraceMsg(TF_TOOLBAR, "Toolbar: calculating WrapToolbar");

    if (ptb->iNumButtons == 0) {
        // no buttons, so we're not going to go through the loop below; initialize 
        // dyButton to 0 so that we fill in lpRect with 0 height.  this fixes ideal 
        // size calculation for empty toolbars (NT5 #180430)
        dyButton = 0;
    } else {
        if (dx < ptb->iButWidth) {
            // dx must be at least the button width, otherwise the final
            // rect is mis-calculated and will be too big.
            dx = ptb->iButWidth;
        }
        dyButton = ptb->iButHeight;
    }

    xMax = 0;
    xPos = ptb->xFirstButton;
    yPos = ptb->iYPos;
    pBtnLast = &(ptb->Buttons[ptb->iNumButtons]);
    ptb->szCached.cx = -1;
    ptb->szCached.cy = -1;

    if (pRows)
        (*pRows)=1;

    for (pButton = ptb->Buttons; pButton < pBtnLast; pButton++)
    {
        // we nuke the wrap state at the start of the loop.
        // so we don't know if/when we are adding on a wrap bit that wasn't there
        // before.  we overstep the button, then back up when we've gone too far,
        // so we can't simply keep the at the start of the loop
        // we need to keep it over to the next iteration
        BOOL fNextLastVisibleWrapped = (pButton->fsState & TBSTATE_WRAP);
        LPTBBUTTONDATA pbtnSav = pButton;

        pButton->fsState &= ~TBSTATE_WRAP;

        if (!(pButton->fsState & TBSTATE_HIDDEN))
        {
            LPTBBUTTONDATA pbtnNextLastVisible = pButton;

            xPos += TBWidthOfButton(ptb, pButton, NULL) - s_dxOverlap;

            // Is this a normal button and is the button out of bounds?
            if (!(pButton->fsStyle & BTNS_SEP) && (xPos > dx)) {

                // Yes; wrap it.  Go back to the first non-hidden separator
                // as a break-point candidate.
                for (pBtnT=pButton;
                     pBtnT>ptb->Buttons && !(pBtnT->fsState & TBSTATE_WRAP);
                     pBtnT--)
                {
                    if ((pBtnT->fsStyle & BTNS_SEP) &&
                        !(pBtnT->fsState & TBSTATE_HIDDEN))
                    {
                        yPos += (TBGetSepHeight(ptb, pBtnT)) + dyButton;
                        bWrapAtNextSeparator = FALSE;
                        if (pRows)
                            (*pRows)++;

                        goto SetWrapHere;
                    }
                }

                pBtnT = pButton;

                // Are we at the first button?
                if (pButton != ptb->Buttons) {
                    // No; back up to first non-hidden button
                    do {
                        pBtnT--;
                    } while ((pBtnT>ptb->Buttons) &&
                             (pBtnT->fsState & TBSTATE_HIDDEN));

                    // Is it already wrapped?
                    if (pBtnT->fsState & TBSTATE_WRAP)
                    {
                        // Yes; wrap the button we were looking at originally
                        pBtnT = pButton;
                    }
                }

                // Wrap at the next separator because we've now wrapped in the middle
                // of a group of buttons.
                bWrapAtNextSeparator = TRUE;
                yPos += dyButton;

SetWrapHere:
                pBtnT->fsState |= TBSTATE_WRAP;

                // find out if this wrap bit is new...
                // it isn't if this button was the last visible button
                // and that last visible button started off wrapped
                if (pBtnT != pbtnLastVisible || !fLastVisibleWrapped)
                    fInvalidate = TRUE;

                xPos = ptb->xFirstButton;
                pButton = pBtnT;

                // Count another row.
                if (pRows)
                    (*pRows)++;
            }
            else
            {
                // No; this is a separator (in or out of bounds) or a button that is in-bounds.

                if (pButton->fsStyle & BTNS_SEP)
                {
                    if (ptb->ci.style & CCS_VERT)
                    {
                        if (pbtnPrev && !(pbtnPrev->fsState & TBSTATE_WRAP))
                        {
                            pbtnPrev->fsState |= TBSTATE_WRAP;
                            yPos += dyButton;
                        }
                        xPos = ptb->xFirstButton;
                        yPos += TBGetSepHeight(ptb, pButton);
                        pButton->fsState |= TBSTATE_WRAP;
                        if (pRows)
                            (*pRows)++;
                    }
                    else if (bWrapAtNextSeparator)
                    {
                        bWrapAtNextSeparator = FALSE;
                        pButton->fsState |= TBSTATE_WRAP;
                        xPos = ptb->xFirstButton;
                        yPos += dyButton + (TBGetSepHeight(ptb, pButton));
                        if (pRows)
                            (*pRows)+=2;
                    }
                }

                // This button is visible and it's one we cached at the top of the loop
                // set it for the next loop
                if (pButton == pbtnNextLastVisible) {
                    ASSERT(!(pButton->fsState & TBSTATE_HIDDEN));
                    if (!(pButton->fsState & TBSTATE_HIDDEN)) {

                        // we don't know that we're not going to re-wrap an item that was initially wrapped
                        // until this point
                        if (pbtnLastVisible && fLastVisibleWrapped && !(pbtnLastVisible->fsState & TBSTATE_WRAP))
                            fInvalidate = TRUE;

                        pbtnLastVisible = pButton;
                        fLastVisibleWrapped = fNextLastVisibleWrapped;
                    }
                }
            }
            if (!(pButton->fsStyle&BTNS_SEP))
                xMax = max(xPos, xMax);

            pbtnPrev = pbtnSav;
        }
    }

    if (lpRect)
    {
        lpRect->left = 0;
        lpRect->right = xMax;
        lpRect->top = 0;
        lpRect->bottom = yPos + ptb->iYPos + dyButton;
    }

    if (fInvalidate)
        InvalidateRect(ptb->ci.hwnd, NULL, TRUE);
}


// only called from TB_SETROWS so no worry's about TBSTYLE_EX_MULTICOLUMN
BOOL BoxIt(PTBSTATE ptb, int height, BOOL fLarger, LPRECT lpRect)
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


int PositionFromID(PTBSTATE ptb, LONG_PTR id)
{
    int i;

    // Handle case where this is sent at the wrong time..
    if (ptb == NULL || id == -1)
        return -1;

    // note, we don't skip separators, so you better not have conflicting
    // cmd ids and separator ids.
    for (i = 0; i < ptb->iNumButtons; i++)
        if (ptb->Buttons[i].idCommand == id)
            return i;       // position found

    return -1;      // ID not found!
}

// check a radio button by button index.
// the button matching idCommand was just pressed down.  this forces
// up all other buttons in the group.
// this does not work with buttons that are forced up with

void MakeGroupConsistant(PTBSTATE ptb, int idCommand)
{
    int i, iFirst, iLast, iButton;
    int cButtons = ptb->iNumButtons;
    LPTBBUTTONDATA pAllButtons = ptb->Buttons;

    iButton = PositionFromID(ptb, idCommand);

    if (iButton < 0)
        return;

    // assertion

//    if (!(pAllButtons[iButton].fsStyle & BTNS_CHECK))
//  return;

    // did the pressed button just go down?
    if (!(pAllButtons[iButton].fsState & TBSTATE_CHECKED))
        return;         // no, can't do anything

    // find the limits of this radio group

    // there was a bug here since win95 days -- ; there was no ; at the end of for loop
    // and if was part of it -- some apps may rely on that (reljai 6/16/98)
    for (iFirst = iButton; (iFirst > 0) && (pAllButtons[iFirst].fsStyle & BTNS_GROUP); iFirst--);
    
    if (!(pAllButtons[iFirst].fsStyle & BTNS_GROUP))
        iFirst++;

    cButtons--;
    for (iLast = iButton; (iLast < cButtons) && (pAllButtons[iLast].fsStyle & BTNS_GROUP); iLast++);

    if (!(pAllButtons[iLast].fsStyle & BTNS_GROUP))
        iLast--;

    // search for the currently down button and pop it up
    for (i = iFirst; i <= iLast; i++) {
        if (i != iButton) {
            // is this button down?
            if (pAllButtons[i].fsState & TBSTATE_CHECKED) {
                pAllButtons[i].fsState &= ~TBSTATE_CHECKED;     // pop it up
                TBInvalidateButton(ptb, i, TRUE);
                break;          // only one button is down right?
            }
        }
    }
}

void DestroyStrings(PTBSTATE ptb)
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

// gets the iString from pStrings and copies it to pszText.
// returns the lstrlen.
// pszText can be null to just fetch the length.
int TBGetString(PTBSTATE ptb, int iString, int cchText, LPTSTR pszText)
{
    int iRet = -1;
    if (iString < ptb->nStrings) {
        iRet = lstrlen(ptb->pStrings[iString]);
        if (pszText) {
            lstrcpyn(pszText, ptb->pStrings[iString], cchText);
        }
    }

    return iRet;
}

#ifdef UNICODE

// gets the iString from pStrings and copies it to pszText.
// returns the lstrlen.
// pszText can be null to just fetch the length.
int TBGetStringA(PTBSTATE ptb, int iString, int cchText, LPSTR pszText)
{
    int iRet = -1;
    if (iString < ptb->nStrings) {

        iRet = lstrlenW(ptb->pStrings[iString]);
        if (pszText) {
            WideCharToMultiByte (CP_ACP, 0, ptb->pStrings[iString],
                                 -1, pszText, cchText, NULL, NULL);
        }
    }

    return iRet;
}
#endif

#define MAXSTRINGSIZE 1024
int TBAddStrings(PTBSTATE ptb, WPARAM wParam, LPARAM lParam)
{
    int i = 0,j = 0, cxMax = 0;
    LPTSTR lpsz;
    PTSTR  pString, pStringAlloc, psz;
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
        psz = LocalReAlloc(pString, (i+1) * sizeof (TCHAR), LMEM_MOVEABLE);
        if (psz)
            pString = psz;

        // convert separators to '\0' and count number of strings
        cSeparator = *pString;
#ifndef UNICODE
        for (numstr = 0, psz = pString + 1, i--; i; i--, psz++ ) {
            if (*psz == cSeparator) {
                if (i != 1)     // We don't want to count the second terminator as another string
                    numstr++;

                *psz = 0;    // terminate with 0
            }
            // extra i-- if DBCS
            if (IsDBCSLeadByte(*psz))
            {
                *(WORD *)(psz-1) = *(WORD *)psz;
                psz++;
                i--;
            }
            else
            {
                // shift string to the left to overwrite separator identifier
                *(psz - 1) = *psz;
            }
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

        // Common mistake is to forget to check the return value of
        // LoadLibrary and accidentally pass wParam=NULL.
        if (IS_INTRESOURCE(lParam))
            return -1;

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
        hmemcpy(pString, (void *)lParam, i * sizeof(TCHAR));
    }

    pStringAlloc = pString;         // in case something bad happens

    // make room for increased string pointer table
    pFoo = (PTSTR *)CCLocalReAlloc(ptb->pStrings,
            (ptb->nStrings + numstr) * sizeof(PTSTR));
    if (!pFoo) {
        goto Failure;
    }

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
    i = ptb->nStrings;
    ptb->nStrings += numstr;
    if (!TBRecalc(ptb))
    {
        ptb->nStrings -= numstr;
        // back out changes.
        pFoo = (PTSTR *)CCLocalReAlloc(ptb->pStrings,
                    ptb->nStrings * sizeof(PTSTR));
        if (pFoo)
            ptb->pStrings = pFoo;
         // don't get mad if pFoo == NULL; it means the shrink failed, no big deal

Failure:
        LocalFree(pStringAlloc);
        return -1;
    }

    return i;               // index of first added string
}

void MapToStandardBitmaps(HINSTANCE *phinst, UINT_PTR *pidBM, int *pnButtons)
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

//
//  the PBITMAP points to the BITMAP structure that was GetObject'd from
//  the hbm, except that pbm->bmWidth and pbm->bmHeight have been adjusted
//  to represent the *desired* height and width, not the actual height
//  and width.
//
HBITMAP _CopyBitmap(PTBSTATE ptb, HBITMAP hbm, PBITMAP pbm)
{
    HBITMAP hbmCopy = NULL;
    HDC hdcWin;
    HDC hdcSrc, hdcDest;

    // Old code called CreateColorBitmap, which is bad on multimon systems
    // because it will create a bitmap that ImageList_AddMasked can't handle,
    // resulting in disabled toolbar buttons looking like crap.

    // so we have to create the bitmap copy in the same format as the source

    hdcWin = GetDC(ptb->ci.hwnd);
    hdcSrc = CreateCompatibleDC(hdcWin);
    hdcDest = CreateCompatibleDC(hdcWin);
    if (hdcWin && hdcSrc && hdcDest) {

        SelectObject(hdcSrc, hbm);

        if (pbm->bmBits) {
            // Source was a DIB section.  Create a DIB section in the same
            // color format with the same palette.
            //
            // Man, creating a DIB section is so annoying.

            struct {                    // Our private version of BITMAPINFO
                BITMAPINFOHEADER bmiHeader;
                RGBQUAD bmiColors[256];
            } bmi;
            UINT cBitsPixel;
            LPVOID pvDummy;

            ZeroMemory(&bmi.bmiHeader, sizeof(bmi.bmiHeader));

            bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
            bmi.bmiHeader.biWidth = pbm->bmWidth;
            bmi.bmiHeader.biHeight = pbm->bmHeight;
            bmi.bmiHeader.biPlanes = 1;

            // DIB color depths must be exactly 1, 4, 8 or 24.
            cBitsPixel = pbm->bmPlanes * pbm->bmBitsPixel;
            if (cBitsPixel <= 1)
                bmi.bmiHeader.biBitCount = 1;
            else if (cBitsPixel <= 4)
                bmi.bmiHeader.biBitCount = 4;
            else if (cBitsPixel <= 8)
                bmi.bmiHeader.biBitCount = 8;
            else
                goto CreateDDB; // ImageList_AddMasked doesn't like DIBs deeper than 8bpp

            // And get the color table too
            ASSERT(bmi.bmiHeader.biBitCount <= 8);
            bmi.bmiHeader.biClrUsed = GetDIBColorTable(hdcSrc, 0, 1 << bmi.bmiHeader.biBitCount, bmi.bmiColors);

            ASSERT(bmi.bmiHeader.biCompression == BI_RGB);
            ASSERT(bmi.bmiHeader.biSizeImage == 0);

            hbmCopy = CreateDIBSection(hdcWin, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS, &pvDummy, NULL, 0);

        } else {
            // Source was a DDB.  Create a duplicate DDB.
        CreateDDB:
            // Since the caller may have dorked the bmWidth,
            // we have to recompute the bmWidthBytes, because GDI
            // gets mad if it's not exactly right, even in the bmBits == NULL
            // case.
            pbm->bmBits = NULL;
            pbm->bmWidthBytes = ((pbm->bmBitsPixel * pbm->bmWidth + 15) >> 4) << 1;
            hbmCopy = CreateBitmapIndirect(pbm);

        }

        SelectObject(hdcDest, hbmCopy);

        // fill the background
        PatB(hdcDest, 0, 0, pbm->bmWidth, pbm->bmHeight, g_clrBtnFace);

        BitBlt(hdcDest, 0, 0, pbm->bmWidth, pbm->bmHeight,
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
    HIMAGELIST himl = TBGetImageList(ptb, HIML_NORMAL, 0);
    if (!himl) {
        himl = ImageList_Create(ptb->iDxBitmap, ptb->iDyBitmap, ILC_MASK | ILC_COLORDDB, 4, 4);
        if (!himl)
            return(FALSE);
        TBSetImageList(ptb, HIML_NORMAL, 0, himl);
        ImageList_SetBkColor(himl, (ptb->ci.style & TBSTYLE_TRANSPARENT) ? CLR_NONE : g_clrBtnFace);
    }

    if (pTemp->hInst) {
        // can't use LoadImage(..., LR_MAP3DCOLORS) - more than 3 colors
        hbm = hbmTemp = CreateMappedBitmap(pTemp->hInst, pTemp->wID, CMB_DIBSECTION, NULL, 0);

    } else if (pTemp->wID) {
        hbm = (HBITMAP)pTemp->wID;
    }

    if (hbm) {

        //
        // Fix up bitmaps that aren't iDxBitmap x iDyBitmap
        //

        BITMAP bm;

        GetObject( hbm, sizeof(bm), &bm);

        if (bm.bmWidth < ptb->iDxBitmap) {
            bm.bmWidth = ptb->iDxBitmap;
        }

        if (bm.bmHeight < ptb->iDyBitmap) {
            bm.bmHeight = ptb->iDyBitmap;
        }

        // The error cases we are catching are:
        // If the pTemp->nButtons is 0 then we assume there is one button
        // If width of the bitmap is less than what it is supposed to be, we fix it.
        if (!pTemp->nButtons)
            bm.bmWidth = ptb->iDxBitmap;
        else if (pTemp->nButtons > (bm.bmWidth / ptb->iDxBitmap))
            bm.bmWidth = ptb->iDxBitmap * pTemp->nButtons;

        // Must preserve color depth to keep ImageList_AddMasked happy
        // And if we started with a DIB section, then create a DIB section.
        // (Curiously, CopyImage does not preserve DIB-ness.)
        hbm = (HBITMAP)_CopyBitmap(ptb, hbm, &bm);

    }

    // AddMasked parties on the bitmap, so we want to use a local copy
    if (hbm) {
        ImageList_AddMasked(himl, hbm, g_clrBtnFace);

        DeleteObject(hbm);
    }

    if (hbmTemp) {
        DeleteObject(hbmTemp);
    }

    return(TRUE);

}

void TBBuildImageList(PTBSTATE ptb)
{
    int i;
    PTBBMINFO pTemp;
    HIMAGELIST himl;

    ptb->fHimlValid = TRUE;

    // is the parent dealing natively with imagelists?  if so,
    // don't do this back compat building
    if (ptb->fHimlNative)
        return;

    himl = TBSetImageList(ptb, HIML_NORMAL, 0, NULL);
    ImageList_Destroy(himl);

    for (i = 0, pTemp = ptb->pBitmaps; i < ptb->nBitmaps; i++, pTemp++) {

        TBAddBitmapToImageList(ptb, pTemp);
    }

}

/* Adds a new bitmap to the list of BMs available for this toolbar.
 * Returns the index of the first button in the bitmap or -1 if there
 * was an error.
 */
int AddBitmap(PTBSTATE ptb, int nButtons, HINSTANCE hBMInst, UINT_PTR idBM)
{
    PTBBMINFO pTemp;
    int nBM, nIndex;

    // map things to the standard toolbar images
    if (hBMInst == HINST_COMMCTRL)        // -1
    {
        // set the proper dimensions...
        if (idBM & 1)
            SetBitmapSize(ptb, LARGE_DXYBITMAP, LARGE_DXYBITMAP);
        else
            SetBitmapSize(ptb, SMALL_DXYBITMAP, SMALL_DXYBITMAP);

        MapToStandardBitmaps(&hBMInst, &idBM, &nButtons);
    }

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

    }

    pTemp = (PTBBMINFO)CCLocalReAlloc(ptb->pBitmaps,
            (ptb->nBitmaps + 1)*sizeof(TBBMINFO));
    if (!pTemp)
        return(-1);
    ptb->pBitmaps = pTemp;

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

int PASCAL TBLoadImages(PTBSTATE ptb, UINT_PTR id, HINSTANCE hinst)
{
    int iTemp;
    TBBMINFO bmi;
    HIMAGELIST himl;

    MapToStandardBitmaps(&hinst, &id, &iTemp);

    bmi.hInst = hinst;
    bmi.wID = id;
    bmi.nButtons = iTemp;

    himl = TBGetImageList(ptb, HIML_NORMAL, 0);
    if (himl)
        iTemp = ImageList_GetImageCount(himl);
    else
        iTemp = 0;

    if (!TBAddBitmapToImageList(ptb, &bmi))
        return(-1);

    ptb->fHimlNative = TRUE;
    return iTemp;
}

BOOL ReplaceBitmap(PTBSTATE ptb, LPTBREPLACEBITMAP lprb)
{
    int nBM;
    PTBBMINFO pTemp;

    int iTemp;

    MapToStandardBitmaps(&lprb->hInstOld, &lprb->nIDOld, &iTemp);
    MapToStandardBitmaps(&lprb->hInstNew, &lprb->nIDNew, &lprb->nButtons);

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


void TBInvalidateItemRects(PTBSTATE ptb)
{
    // Invalidate item rect cache
    ptb->fItemRectsValid = FALSE;

    // Invalidate the tooltips
    ptb->fTTNeedsFlush = TRUE;

    // Invalidate the ideal size cache
    ptb->szCached.cx = -1;
    ptb->szCached.cy = -1;
}

void FlushToolTipsMgrNow(PTBSTATE ptb) {

    // change all the rects for the tool tips mgr.  this is
    // cheap, and we don't do it often, so go ahead
    // and do them all.
    if(ptb->hwndToolTips) {
        UINT i;
        TOOLINFO ti;
        LPTBBUTTONDATA pButton;

        ti.cbSize = SIZEOF(ti);
        ti.hwnd = ptb->ci.hwnd;
        ti.lpszText = LPSTR_TEXTCALLBACK;
        for ( i = 0, pButton = ptb->Buttons;
             i < (UINT)ptb->iNumButtons;
             i++, pButton++) {

            if (!(pButton->fsStyle & BTNS_SEP)) {
                ti.uId = pButton->idCommand;

                if (!TB_GetItemRect(ptb, i, &ti.rect) ||
                   ((ptb->dwStyleEx & TBSTYLE_EX_HIDECLIPPEDBUTTONS) && TBIsRectClipped(ptb, &ti.rect))) {

                    ti.rect.left = ti.rect.right = ti.rect.top = ti.rect.bottom = 0;
                }

                SendMessage(ptb->hwndToolTips, TTM_NEWTOOLRECT, 0, (LPARAM)((LPTOOLINFO)&ti));
            }
        }

        ptb->fTTNeedsFlush = FALSE;
    }
}

BOOL TBReallocButtons(PTBSTATE ptb, UINT uButtons)
{
    LPTBBUTTONDATA ptbbNew;
    LPTBBUTTONDATA pOldCaptureButton;

    if (!ptb || !ptb->uStructSize)
        return FALSE;

    // When we realloc the Button array, make sure all interior pointers
    //  move with it.  (This should probably be an index.)
    pOldCaptureButton = ptb->pCaptureButton;

    // realloc the button table
    ptbbNew = (LPTBBUTTONDATA)CCLocalReAlloc(ptb->Buttons,
                                             uButtons * sizeof(TBBUTTONDATA));

    if (!ptbbNew) return FALSE;

    if (pOldCaptureButton)
        ptb->pCaptureButton = (LPTBBUTTONDATA)(
                        (LPBYTE)ptbbNew +
                          ((LPBYTE)pOldCaptureButton - (LPBYTE)ptb->Buttons));
    ptb->Buttons = ptbbNew;

    return TRUE;
}

BOOL TBInsertButtons(PTBSTATE ptb, UINT uWhere, UINT uButtons, LPTBBUTTON lpButtons, BOOL fNative)
{
    LPTBBUTTONDATA pOut;
    LPTBBUTTONDATA ptbbIn;
    UINT    uAdded;
    UINT    uStart;
    BOOL fRecalc;
    int idHot = -1;

    if (!TBReallocButtons(ptb, ptb->iNumButtons + uButtons))
        return FALSE;

    // comments by chee (not the original author) so they not be
    // exactly right... be warned.

    // if where points beyond the end, set it at the end
    if (uWhere > (UINT)ptb->iNumButtons)
        uWhere = ptb->iNumButtons;

    // Need to save these since the values gues toasted.
    uAdded = uButtons;
    uStart = uWhere;

    // Correct the hot item when we add something something. Since the hot item is index based, the index
    // has probrably changed
    if (ptb->iHot >= 0 && ptb->iHot < ptb->iNumButtons)
        idHot = ptb->Buttons[ptb->iHot].idCommand;

    // move buttons above uWhere up uButton spaces
    // the uWhere gets inverted and counts to zero..
    //
    // REVIEW: couldn't this be done with MoveMemory?
    //  MoveMemory(&ptb->Buttons[uWhere], &ptb->Buttons[uWhere+uButtons], sizeof(ptb->Buttons[0])*(ptb->iNumButtons - uWhere));
    //
    for (ptbbIn = &ptb->Buttons[ptb->iNumButtons-1], pOut = ptbbIn+uButtons,
         uWhere=(UINT)ptb->iNumButtons-uWhere; uWhere>0;
     --ptbbIn, --pOut, --uWhere)
        *pOut = *ptbbIn;

    // only need to recalc if there are strings & room enough to actually show them
    fRecalc = (TBHasStrings(ptb) && ((ptb->ci.style & TBSTYLE_LIST) || ((ptb->iDyBitmap + ptb->yPad + g_cyEdge) < ptb->iButHeight)));

    // now do the copy.
    for (lpButtons=(LPTBBUTTON)((LPBYTE)lpButtons+ptb->uStructSize*(uButtons-1)),
        ptb->iNumButtons+=(int)uButtons;  // init
        uButtons>0; //test
        --pOut, lpButtons=(LPTBBUTTON)((LPBYTE)lpButtons-ptb->uStructSize), --uButtons)
    {
        TBInputStruct(ptb, pOut, lpButtons);

        // If this button is a seperator, then should not use the string
        // buffer passed in, because it could be bogus data.
        if (pOut->fsStyle & BTNS_SEP)
            pOut->iString = -1;

        if (TBISSTRINGPTR(pOut->iString)) {
            LPTSTR psz = (LPTSTR)pOut->iString;
#ifdef UNICODE
            if (!fNative) {
                psz = ProduceWFromA(ptb->ci.uiCodePage, (LPSTR)psz);
            }
#endif
            pOut->iString = 0;
            Str_Set((LPTSTR*)&pOut->iString, psz);

#ifdef UNICODE
            if (!fNative)
                FreeProducedString(psz);
#endif
            if (!ptb->fNoStringPool)
                fRecalc = TRUE;

            ptb->fNoStringPool = TRUE;
        }

        if(ptb->hwndToolTips && !(lpButtons->fsStyle & BTNS_SEP)) {
            TOOLINFO ti;
            // don't bother setting the rect because we'll do it below
            // in TBInvalidateItemRects;
            ti.cbSize = sizeof(ti);
            ti.uFlags = 0;
            ti.hwnd = ptb->ci.hwnd;
            ti.uId = lpButtons->idCommand;
            ti.lpszText = LPSTR_TEXTCALLBACK;
            SendMessage(ptb->hwndToolTips, TTM_ADDTOOL, 0,
                (LPARAM)(LPTOOLINFO)&ti);
        }

        if (pOut->fsStyle & BTNS_SEP && pOut->DUMMYUNION_MEMBER(cxySep) <= 0)
        {

            // Compat: Corel (Font navigator) expects the separators to be
            // 8 pixels wide.
            // as do many old apps.
            //
            // so if it's not flat or not vertical, put it to defautl to win95 size
            pOut->DUMMYUNION_MEMBER(cxySep) = g_dxButtonSep;
        }
    }

    // Re-compute layout if toolbar is wrappable.
    if ((ptb->dwStyleEx & TBSTYLE_EX_MULTICOLUMN) || 
        (ptb->ci.style & TBSTYLE_WRAPABLE))
    {
        // NOTE: we used to do send ourself a message instead of call directly...
        //SendMessage(ptb->ci.hwnd, TB_AUTOSIZE, 0, 0);
        TBAutoSize(ptb);
    }

    TBInvalidateItemRects(ptb);

    // adding and removing buttons during toolbar customization shouldn't
    // result in recalcing the sizes of buttons.
    if (fRecalc && !ptb->hdlgCust)
        TBRecalc(ptb);

    //
    // Reorder notification so apps can go requery what's on the toolbar if
    // more than 1 button was added; otherwise, just say create.
    //
    if (uAdded == 1)
        MyNotifyWinEvent(EVENT_OBJECT_CREATE, ptb->ci.hwnd, OBJID_CLIENT,
            uWhere+1);
    else
        MyNotifyWinEvent(EVENT_OBJECT_REORDER, ptb->ci.hwnd, OBJID_CLIENT, 0);

    // was there a hot item before the delete?
    if (idHot != -1)
    {
        // Yes; Then update it to the current index
        ptb->iHot = PositionFromID(ptb, idHot);
    }

    TBInvalidateItemRects(ptb);

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
BOOL DeleteButton(PTBSTATE ptb, UINT uIndex)
{
    TBNOTIFY tbn = { 0 };
    LPTBBUTTONDATA pIn, pOut;
    BOOL fRecalc;
    int idHot = -1;



    if (uIndex >= (UINT)ptb->iNumButtons)
        return FALSE;

    if (&ptb->Buttons[uIndex] == ptb->pCaptureButton) {
        if (ptb->uStructSize == 0x14)
            ptb->fRequeryCapture = TRUE;
        if (!CCReleaseCapture(&ptb->ci)) return FALSE;
        ptb->pCaptureButton = NULL;
    }

    // Correct the hot item when we remove something. Since the hot item is index based, the index
    // has probrably changed
    if (ptb->iHot >= 0 && ptb->iHot < ptb->iNumButtons)
        idHot = ptb->Buttons[ptb->iHot].idCommand;

    // Notify Active Accessibility of the delete
    MyNotifyWinEvent(EVENT_OBJECT_DESTROY, ptb->ci.hwnd, OBJID_CLIENT, uIndex+1);

    // Notify client of the delete
    tbn.iItem = ptb->Buttons[uIndex].idCommand;
    TBOutputStruct(ptb, &ptb->Buttons[uIndex], &tbn.tbButton);
    CCSendNotify(&ptb->ci, TBN_DELETINGBUTTON, &tbn.hdr);


    if (TBISSTRINGPTR(ptb->Buttons[uIndex].iString))
        Str_Set((LPTSTR*)&ptb->Buttons[uIndex].iString, NULL);

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
    if (((ptb->ci.style & TBSTYLE_WRAPABLE)
            || (ptb->dwStyleEx & TBSTYLE_EX_MULTICOLUMN)) && fRecalc)
    {
        RECT rc;
        HWND hwnd = ptb->ci.hwnd;

        if (!(ptb->ci.style & CCS_NORESIZE) && !(ptb->ci.style & CCS_NOPARENTALIGN))
            hwnd = GetParent(hwnd);

        GetWindowRect(hwnd, &rc);

        if (ptb->ci.style & TBSTYLE_WRAPABLE)
            WrapToolbar(ptb, rc.right - rc.left, &rc, NULL);
        else
            WrapToolbarCol(ptb, ptb->sizeBound.cy, &rc, NULL);
    }

    // was there a hot item before the delete?
    if (idHot != -1)
    {
        // Yes; Then update it to the current index
        ptb->iHot = PositionFromID(ptb, idHot);
    }


    InvalidateRect(ptb->ci.hwnd, NULL, TRUE);

    TBInvalidateItemRects(ptb);

    return TRUE;
}

// move button at location iOld to location iNew, sliding everything
// after iNew UP.
BOOL PASCAL TBMoveButton(PTBSTATE ptb, UINT iOld, UINT iNew)
{
    TBBUTTONDATA tbd, *ptbdOld, *ptbdNew;

    if (iOld >= (UINT)ptb->iNumButtons)
        return FALSE;

    if (iNew > (UINT)ptb->iNumButtons-1)
        iNew = (UINT)ptb->iNumButtons-1;

    if (iOld == iNew)
        return FALSE;

    TBInvalidateItemRects(ptb);

    ptbdOld = &(ptb->Buttons[iOld]);
    ptbdNew = &(ptb->Buttons[iNew]);

    tbd = *ptbdOld;

#if 0
    if (iOld < iNew)
        MoveMemory(ptbdOld+1, ptbdOld, (iNew - iOld) * SIZEOF(tbd));
    else
        MoveMemory(ptbdNew, ptbdNew+1, (iOld - iNew) * SIZEOF(tbd));
#else
    {
        TBBUTTONDATA *ptbdSrc;
        TBBUTTONDATA *ptbdDst;
        int iCount, iInc;

        if (iOld < iNew)
        {
            // move [iOld+1..iNew] to [iOld..iNew-1]
            iCount = iNew - iOld;
            iInc = 1;
            ptbdSrc = ptbdOld + 1;
            ptbdDst = ptbdOld;

            if (ptb->pCaptureButton > ptbdOld && ptb->pCaptureButton <= ptbdNew)
                ptb->pCaptureButton--;
        }
        else
        {
            ASSERT(iNew < iOld);

            // move [iNew..iOld-1] to [iNew+1..iOld]
            iCount = iOld - iNew;
            iInc = -1;
            ptbdSrc = ptbdNew + iCount - 1;
            ptbdDst = ptbdNew + iCount;

            if (ptb->pCaptureButton >= ptbdNew && ptb->pCaptureButton < ptbdOld)
                ptb->pCaptureButton++;
        }

        do {
            *ptbdDst = *ptbdSrc;
            ptbdDst += iInc;
            ptbdSrc += iInc;
            iCount--;
        } while (iCount);
    }
#endif

    *ptbdNew = tbd;

    if (ptb->pCaptureButton == ptbdOld)
        ptb->pCaptureButton = ptbdNew;

    TBAutoSize(ptb);
    InvalidateRect(ptb->ci.hwnd, NULL, TRUE);

    return TRUE;
}


// deal with old TBBUTON structs for compatibility
void TBInputStruct(PTBSTATE ptb, LPTBBUTTONDATA pButtonInt, LPTBBUTTON pButtonExt)
{
    pButtonInt->DUMMYUNION_MEMBER(iBitmap) = pButtonExt->iBitmap;
    pButtonInt->idCommand = pButtonExt->idCommand;
    pButtonInt->fsState = pButtonExt->fsState;
    pButtonInt->fsStyle = pButtonExt->fsStyle;
    pButtonInt->cx = 0;

    if (ptb->uStructSize >= sizeof(TBBUTTON))
    {
        pButtonInt->dwData = pButtonExt->dwData;
        pButtonInt->iString = pButtonExt->iString;
    }
    else
    {
        /* It is assumed the only other possibility is the OLDBUTTON struct */
        /* We don't care about dwData */
        pButtonInt->dwData = 0;
        pButtonInt->iString = -1;
    }
}


void TBOutputStruct(PTBSTATE ptb, LPTBBUTTONDATA pButtonInt, LPTBBUTTON pButtonExt)
{
    ZeroMemory(pButtonExt, ptb->uStructSize);
    pButtonExt->iBitmap = pButtonInt->DUMMYUNION_MEMBER(iBitmap);
    pButtonExt->idCommand = pButtonInt->idCommand;
    pButtonExt->fsState = pButtonInt->fsState;
    pButtonExt->fsStyle = pButtonInt->fsStyle;

    // We're returning cx in the bReserved field
    COMPILETIME_ASSERT(FIELD_OFFSET(TBBUTTONDATA, cx) == FIELD_OFFSET(TBBUTTON, bReserved));
    COMPILETIME_ASSERT(sizeof(pButtonInt->cx) <= sizeof(pButtonExt->bReserved));
    ((LPTBBUTTONDATA)pButtonExt)->cx = pButtonInt->cx;

    if (ptb->uStructSize >= sizeof(TBBUTTON))
    {
        pButtonExt->dwData = pButtonInt->dwData;
        pButtonExt->iString = pButtonInt->iString;
    }
}

void TBOnButtonStructSize(PTBSTATE ptb, UINT uStructSize)
{
    /* You are not allowed to change this after adding buttons.
    */
    if (ptb && !ptb->iNumButtons)
    {
        ptb->uStructSize = uStructSize;
    }
}

void TBAutoSize(PTBSTATE ptb)
{
    HWND hwndParent;
    RECT rc;
    int nTBThickness = 0;

    if (ptb->fRedrawOff) {
        // redraw is off; defer autosize until redraw is turned back on
        ptb->fRecalc = TRUE;
        return;
    }

    if (ptb->dwStyleEx & TBSTYLE_EX_MULTICOLUMN)
    {
        ASSERT(ptb->dwStyleEx & TBSTYLE_EX_VERTICAL);
        nTBThickness = ptb->iButWidth * CountCols(ptb) + g_cyEdge * 2;
    }
    else
        nTBThickness = ptb->iButHeight * CountRows(ptb) + g_cxEdge * 2;

    hwndParent = GetParent(ptb->ci.hwnd);
    if (!hwndParent)
        return;

    if ((ptb->ci.style & TBSTYLE_WRAPABLE)
                    || (ptb->dwStyleEx & TBSTYLE_EX_MULTICOLUMN))
    {
        RECT rcNew;

        if ((ptb->ci.style & CCS_NORESIZE) || (ptb->ci.style & CCS_NOPARENTALIGN))
            GetWindowRect(ptb->ci.hwnd, &rc);
        else
            GetWindowRect(hwndParent, &rc);

        if (ptb->ci.style & TBSTYLE_WRAPABLE)
            WrapToolbar(ptb, rc.right - rc.left, &rcNew, NULL);
        else
            WrapToolbarCol(ptb, ptb->sizeBound.cy, &rcNew, NULL);

        // Some sample app found a bug in our autosize code which this line
        // fixes. Unfortunately Carbon Copy 32 (IE4 bug 31943) relies on the
        // broken behavior and fixing this clips the buttons.
        //
        //nTBThickness = rcNew.bottom - rcNew.top + g_cxEdge;
    }

    if ((ptb->ci.style & TBSTYLE_WRAPABLE) ||
        (ptb->dwStyleEx & (TBSTYLE_EX_MULTICOLUMN | TBSTYLE_EX_HIDECLIPPEDBUTTONS)))
    {
        TBInvalidateItemRects(ptb);
    }

    GetWindowRect(ptb->ci.hwnd, &rc);
    MapWindowPoints(HWND_DESKTOP, hwndParent, (LPPOINT)&rc, 2);
    NewSize(ptb->ci.hwnd, nTBThickness, ptb->ci.style,
            rc.left, rc.top, rc.right, rc.bottom);
}

void TBSetStyle(PTBSTATE ptb, DWORD dwStyle)
{
    BOOL fSizeChanged = FALSE;

    if ((BOOL)(ptb->ci.style & TBSTYLE_WRAPABLE) != (BOOL)(dwStyle & TBSTYLE_WRAPABLE))
    {
        int i;
        fSizeChanged = TRUE;

        for (i=0; i<ptb->iNumButtons; i++)
            ptb->Buttons[i].fsState &= ~TBSTATE_WRAP;
    }

    ptb->ci.style = dwStyle;

    if (fSizeChanged)
        TBRecalc(ptb);

    TBAutoSize(ptb);

    TraceMsg(TF_TOOLBAR, "toolbar window style changed %x", ptb->ci.style);
}

void TBSetStyleEx(PTBSTATE ptb, DWORD dwStyleEx, DWORD dwStyleMaskEx)
{
    BOOL fSizeChanged = FALSE;

    if (dwStyleMaskEx)
        dwStyleEx = (ptb->dwStyleEx & ~dwStyleMaskEx) | (dwStyleEx & dwStyleMaskEx);

    // Second, we can validate a few of the bits:
    // Multicolumn should never be set w/o the vertical style...
    ASSERT((ptb->dwStyleEx & TBSTYLE_EX_VERTICAL) || !(ptb->dwStyleEx & TBSTYLE_EX_MULTICOLUMN));
    // also can't be set with hide clipped buttons style (for now)
    ASSERT(!(ptb->dwStyleEx & TBSTYLE_EX_HIDECLIPPEDBUTTONS) || !(ptb->dwStyleEx & TBSTYLE_EX_MULTICOLUMN));
    // ...but just in case someone gets it wrong, we'll set the vertical
    // style and rip off the hide clipped buttons style
    if (dwStyleEx & TBSTYLE_EX_MULTICOLUMN)
    {
        dwStyleEx |= TBSTYLE_EX_VERTICAL;
        dwStyleEx &= ~TBSTYLE_EX_HIDECLIPPEDBUTTONS;
    }

    // Then, some things need to be tweaked when they change
    if ((ptb->dwStyleEx ^ dwStyleEx) & TBSTYLE_EX_MULTICOLUMN)
    {
        int i;
        // Clear all the wrap states if we're changing multicolumn styles
        for (i = 0; i < ptb->iNumButtons; i++)
            ptb->Buttons[i].fsState &= ~TBSTATE_WRAP;

        fSizeChanged = TRUE;
    }
    if ((ptb->dwStyleEx ^ dwStyleEx) & TBSTYLE_EX_MIXEDBUTTONS)
    {
        int i;
        for (i = 0; i < ptb->iNumButtons; i++)
            (ptb->Buttons[i]).cx = 0;

        fSizeChanged = TRUE;
        
        InvalidateRect(ptb->ci.hwnd, NULL, TRUE);
    }
    if ((ptb->dwStyleEx ^ dwStyleEx) & TBSTYLE_EX_HIDECLIPPEDBUTTONS)
        InvalidateRect(ptb->ci.hwnd, NULL, TRUE);

    ptb->dwStyleEx = dwStyleEx;

    if (ptb->dwStyleEx & TBSTYLE_EX_VERTICAL)
        TBSetStyle(ptb, CCS_VERT);      // vertical sep and insert mark orientation

    if (fSizeChanged)
    {
        TBRecalc(ptb);
        TBAutoSize(ptb);
    }

    TraceMsg(TF_TOOLBAR, "toolbar window extended style changed %x", ptb->dwStyleEx);
}


LRESULT TB_OnSetImage(PTBSTATE ptb, LPTBBUTTONDATA ptbButton, int iImage)
{
    if (!ptb->fHimlNative) {
        if (ptb->fHimlValid) {
            if (!TBGetImageList(ptb, HIML_NORMAL, 0) ||
                iImage >= ImageList_GetImageCount(TBGetImageList(ptb, HIML_NORMAL, 0)))
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

            if (iImage >= (int)nTot)
                return FALSE;
        }
    }

    ptbButton->DUMMYUNION_MEMBER(iBitmap) = iImage;

    InvalidateButton(ptb, ptbButton, FALSE);
    UpdateWindow(ptb->ci.hwnd);
    return TRUE;
}

void TB_OnDestroy(PTBSTATE ptb)
{
    HWND hwnd = ptb->ci.hwnd;
    int i;

    for (i = 0; i < ptb->iNumButtons; i++) {
        if (TBISSTRINGPTR(ptb->Buttons[i].iString))
            Str_Set((LPTSTR*)&ptb->Buttons[i].iString, NULL);
    }

    //
    // If the toolbar created tooltips, then destroy them.
    //
    if ((ptb->ci.style & TBSTYLE_TOOLTIPS) && IsWindow(ptb->hwndToolTips)) {
        DestroyWindow (ptb->hwndToolTips);
        ptb->hwndToolTips = NULL;
    }

    if (ptb->hDragProxy)
        DestroyDragProxy(ptb->hDragProxy);

    if (ptb->hbmMono)
        DeleteObject(ptb->hbmMono);

    ReleaseMonoDC(ptb);

    if (ptb->nStrings > 0)
        DestroyStrings(ptb);

    if (ptb->hfontIcon && ptb->fFontCreated)
        DeleteObject(ptb->hfontIcon);

    // only do this destroy if pBitmaps exists..
    // this is our signal that it was from an old style toolba
    // and we created it ourselves.
    if (ptb->pBitmaps)
        ImageList_Destroy(TBGetImageList(ptb, HIML_NORMAL, 0));

    if (ptb->pBitmaps)
        LocalFree(ptb->pBitmaps);

    // couldn't have created tb if pimgs creation failed
    CCLocalReAlloc(ptb->pimgs, 0);

    Str_Set(&ptb->pszTip, NULL);
    if (ptb->Buttons) LocalFree(ptb->Buttons);
    LocalFree((HLOCAL)ptb);
    SetWindowInt(hwnd, 0, 0);

    TerminateDitherBrush();
}

void TB_OnSetState(PTBSTATE ptb, LPTBBUTTONDATA ptbButton, BYTE bState, int iPos)
{
    BYTE fsState;
    fsState = bState ^ ptbButton->fsState;
    ptbButton->fsState = bState;

    if (fsState)
    {
        if (ptb->fRedrawOff)
        {
            ptb->fInvalidate = ptb->fRecalc = TRUE;
        }
        else
        {
            if (fsState & TBSTATE_HIDDEN)
            {
                InvalidateRect(ptb->ci.hwnd, NULL, TRUE);
                TBRecalc(ptb);
            }
            else
                InvalidateButton(ptb, ptbButton, TRUE);

            MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, ptb->ci.hwnd, OBJID_CLIENT,
                             iPos+1);
        }
    }
}

void TB_OnSetCmdID(PTBSTATE ptb, LPTBBUTTONDATA ptbButton, UINT idCommand)
{
    UINT uiOldID;

    uiOldID = ptbButton->idCommand;
    ptbButton->idCommand = idCommand;

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

        ti.uId = idCommand;
        SendMessage(ptb->hwndToolTips, TTM_ADDTOOL, 0,
                    (LPARAM)(LPTOOLINFO)&ti);
    }
}



LRESULT TB_OnSetButtonInfo(PTBSTATE ptb, int idBtn, LPTBBUTTONINFO ptbbi)
{
    int iPos;
    BOOL fInvalidateAll = FALSE;

    if (ptbbi->cbSize != SIZEOF(TBBUTTONINFO))
        return 0;

    if (ptbbi->dwMask & TBIF_BYINDEX)
        iPos = idBtn;
    else
        iPos = PositionFromID(ptb, idBtn);

    if (iPos != -1)
    {
        LPTBBUTTONDATA ptbButton;
        BOOL fInvalidate = FALSE;

        ptbButton = ptb->Buttons + iPos;

        if (ptbbi->dwMask & TBIF_STYLE) {
            if ((ptbButton->fsStyle ^ ptbbi->fsStyle) & (BTNS_DROPDOWN | BTNS_WHOLEDROPDOWN))
            {
                // Width may have changed!
                fInvalidateAll = TRUE;
            }
            if ((ptbButton->fsStyle ^ ptbbi->fsStyle) & BTNS_AUTOSIZE)
                ptbButton->cx = 0;

            ptbButton->fsStyle = ptbbi->fsStyle;
            fInvalidate = TRUE;
        }

        if (ptbbi->dwMask & TBIF_STATE) {
            TB_OnSetState(ptb, ptbButton, ptbbi->fsState, iPos);
        }

        if (ptbbi->dwMask & TBIF_IMAGE) {
            TB_OnSetImage(ptb, ptbButton, ptbbi->iImage);
        }

        if (ptbbi->dwMask & TBIF_SIZE) {
            ptbButton->cx = ptbbi->cx;
            fInvalidate = TRUE;
            fInvalidateAll = TRUE;
        }

        if (ptbbi->dwMask & TBIF_TEXT) {

            // changing the text on an autosize button means recalc
            if (BTN_IS_AUTOSIZE(ptb, ptbButton)) {
                fInvalidateAll = TRUE;
                ptbButton->cx = (WORD)0;
            }

            ptb->fNoStringPool = TRUE;
            if (!TBISSTRINGPTR(ptbButton->iString)) {
                ptbButton->iString = 0;
            }

            Str_Set((LPTSTR*)&ptbButton->iString, ptbbi->pszText);
            fInvalidate = TRUE;

        }

        if (ptbbi->dwMask & TBIF_LPARAM) {
            ptbButton->dwData = ptbbi->lParam;
        }

        if (ptbbi->dwMask & TBIF_COMMAND) {
            TB_OnSetCmdID(ptb, ptbButton, ptbbi->idCommand);
        }

        if (fInvalidateAll || fInvalidate) {
            TBInvalidateItemRects(ptb);
            if (fInvalidateAll)
                InvalidateRect(ptb->ci.hwnd, NULL, TRUE);
            else
                InvalidateButton(ptb, ptbButton, TRUE);
        }

        return TRUE;
    }

    return FALSE;
}

LRESULT TB_OnGetButtonInfo(PTBSTATE ptb, int idBtn, LPTBBUTTONINFO ptbbi)
{
    int iPos;

    if (ptbbi->cbSize != SIZEOF(TBBUTTONINFO))
        return -1;

    if (ptbbi->dwMask & TBIF_BYINDEX)
        iPos = idBtn;
    else
        iPos = PositionFromID(ptb, idBtn);
    if (iPos >= 0 && iPos < ptb->iNumButtons)
    {
        LPTBBUTTONDATA ptbButton;
        ptbButton = ptb->Buttons + iPos;

        if (ptbbi->dwMask & TBIF_STYLE) {
            ptbbi->fsStyle = ptbButton->fsStyle;
        }

        if (ptbbi->dwMask & TBIF_STATE) {
            ptbbi->fsState = ptbButton->fsState;
        }

        if (ptbbi->dwMask & TBIF_IMAGE) {
            ptbbi->iImage = ptbButton->DUMMYUNION_MEMBER(iBitmap);
        }

        if (ptbbi->dwMask & TBIF_SIZE) {
            ptbbi->cx = (WORD) ptbButton->cx;
        }

        if (ptbbi->dwMask & TBIF_TEXT) {

            if (TBISSTRINGPTR(ptbButton->iString)) {
                lstrcpyn(ptbbi->pszText, (LPCTSTR)ptbButton->iString, ptbbi->cchText);
            }
        }

        if (ptbbi->dwMask & TBIF_LPARAM) {
            ptbbi->lParam = ptbButton->dwData;
        }

        if (ptbbi->dwMask & TBIF_COMMAND) {
            ptbbi->idCommand = ptbButton->idCommand;
        }
    } else
        iPos = -1;

    return iPos;
}

UINT GetAccelerator(LPTSTR psz)
{
    UINT ch = (UINT)-1;
    LPTSTR pszAccel = psz;
    // then prefixes are allowed.... see if it has one
    do 
    {
        pszAccel = StrChr(pszAccel, CH_PREFIX);
        if (pszAccel) 
        {
            pszAccel = FastCharNext(pszAccel);

            // handle having &&
            if (*pszAccel != CH_PREFIX)
                ch = *pszAccel;
            else
                pszAccel = FastCharNext(pszAccel);
        }
    } 
    while (pszAccel && (ch == (UINT)-1));

    return ch;
}


UINT TBButtonAccelerator(PTBSTATE ptb, LPTBBUTTONDATA ptbn)
{
    UINT ch = (UINT)-1;
    LPTSTR psz = TB_StrForButton(ptb, ptbn);

    if (psz && *psz) {
        if (!(ptb->uDrawTextMask & ptb->uDrawText & DT_NOPREFIX)) {
            ch = GetAccelerator(psz);
        }

        if (ch == (UINT)-1) {
            // no prefix found.  use the first char
#ifndef UNICODE
            if (IsDBCSLeadByte((BYTE)*psz))
                ch = ((BYTE)*psz << 8) | (BYTE)*(psz + 1);
            else
#endif
            ch = (UINT)*psz;
        }
    }
    return (UINT)ch;
}


/*----------------------------------------------------------
Purpose: Returns the number of buttons that have the passed
            in char as their accelerator

*/
int TBHasAccelerator(PTBSTATE ptb, UINT ch)
{
    int i;
    int c = 0;
    for (i = 0; i < ptb->iNumButtons; i++)
    {
        if (!ChrCmpI((WORD)TBButtonAccelerator(ptb, &ptb->Buttons[i]), (WORD)ch))
            c++;
    }

    if (c == 0)
    {
        NMCHAR nm = {0};
        nm.ch = ch;
        nm.dwItemPrev = 0;
        nm.dwItemNext = -1;

        // The duplicate accelerator is used to expand or execute a menu item,
        // if we determine that there are no items, we still want to ask the 
        // owner if there are any...

        if (CCSendNotify(&ptb->ci, TBN_MAPACCELERATOR, &nm.hdr) &&
            nm.dwItemNext >= 0)
        {
            c++;
        }
    }

    return c;
}

/*----------------------------------------------------------
Purpose: Returns TRUE if the character maps to more than one
         button.

*/
BOOL TBHasDupChar(PTBSTATE ptb, UINT ch)
{
    BOOL bRet = FALSE;
    NMTBDUPACCELERATOR nmda;

    int c = 0;

    nmda.ch = ch;

    // BUGBUG (lamadio): this is going away
    if (CCSendNotify(&ptb->ci, TBN_DUPACCELERATOR, &nmda.hdr))
    {
        bRet = nmda.fDup;
    }
    else
    {
        if (TBHasAccelerator(ptb, ch) > 1)
            bRet = TRUE;
    }

    return bRet;
}


/*----------------------------------------------------------
Purpose: Returns the index of the item whose accelerator matches
         the given character.  Starts at the current hot item.

Returns: -1 if nothing found

*/
int TBItemFromAccelerator(PTBSTATE ptb, UINT ch, BOOL * pbDup)
{
    int iRet = -1;
    int i;
    int iStart = ptb->iHot;

    NMTBWRAPACCELERATOR nmwa;
    NMCHAR nm = {0};
    nm.ch = ch;
    nm.dwItemPrev = iStart;
    nm.dwItemNext = -1;

    // Ask the client if they want to handle this keyboard press
    if (CCSendNotify(&ptb->ci, TBN_MAPACCELERATOR, &nm.hdr) &&
        (int)nm.dwItemNext > iStart && (int)nm.dwItemNext < ptb->iNumButtons)
    {
        // They handled it, so we're just going to return the position
        // that they said.
        iRet =  nm.dwItemNext;
    }
    else for (i = 0; i < ptb->iNumButtons; i++)
    {

        if ( iStart + 1 >= ptb->iNumButtons )
        {
            nmwa.ch = ch;
            if (CCSendNotify(&ptb->ci, TBN_WRAPACCELERATOR, &nmwa.hdr))
                return nmwa.iButton;
        }

        iStart += 1 + ptb->iNumButtons;
        iStart %= ptb->iNumButtons;

        if ((ptb->Buttons[iStart].fsState & TBSTATE_ENABLED) &&
            !ChrCmpI((WORD)TBButtonAccelerator(ptb, &ptb->Buttons[iStart]), (WORD)ch))
        {
            iRet = iStart;
            break;
        }

    }

    *pbDup = TBHasDupChar(ptb, ch);

    return iRet;
}


BOOL TBOnChar(PTBSTATE ptb, UINT ch)
{
    NMCHAR nm = {0};
    BOOL bDupChar;
    int iPos = TBItemFromAccelerator(ptb, ch, &bDupChar);
    BOOL fHandled = FALSE;

    // Send the notification.  Parent may want to change the next button.
    nm.ch = ch;
    nm.dwItemPrev = (0 <= ptb->iHot) ? ptb->Buttons[ptb->iHot].idCommand : -1;
    nm.dwItemNext = (0 <= iPos) ? ptb->Buttons[iPos].idCommand : -1;
    if (CCSendNotify(&ptb->ci, NM_CHAR, (LPNMHDR)&nm))
        return TRUE;

    iPos = PositionFromID(ptb, nm.dwItemNext);

    if (-1 != iPos)
    {
        DWORD dwFlags = HICF_ACCELERATOR;

        if (ptb->iHot == iPos)
            dwFlags |= HICF_RESELECT;

        if (bDupChar)
            dwFlags |= HICF_DUPACCEL;

        TBSetHotItem(ptb, iPos, dwFlags);

        if (bDupChar)
            iPos = -1;

        fHandled = TRUE;
    } else {

        // handle this here instead of VK_KEYDOWN
        // because a typical thing to do is to pop down a menu
        // which will beep when it gets the WM_CHAR resulting from
        // the VK_KEYDOWN
        switch (ch) {
        case ' ':
        case 13:
            if (ptb->iHot != -1)
            {
                LPTBBUTTONDATA ptbButton = &ptb->Buttons[ptb->iHot];
                if (TB_IsDropDown(ptbButton) &&
                    !TB_HasSplitDDArrow(ptb, ptbButton))
                {
                    iPos = ptb->iHot;
                    fHandled = TRUE;
                }
                break;
            }
        }
    }

    if (-1 != iPos) {
        LPTBBUTTONDATA ptbButton = &ptb->Buttons[iPos];
        if (TB_IsDropDown(ptbButton))
            TBToggleDropDown(ptb, iPos, FALSE);
    }
#ifdef KEYBOARDCUES
    //notify of navigation key usage
    CCNotifyNavigationKeyUsage(&(ptb->ci), UISF_HIDEFOCUS | UISF_HIDEACCEL);
#endif

    return fHandled;
}


BOOL TBOnMapAccelerator(PTBSTATE ptb, UINT ch, UINT * pidCmd)
{
    int iPos;
    BOOL bDupChar;

    ASSERT(IS_VALID_WRITE_PTR(pidCmd, UINT));

    iPos = TBItemFromAccelerator(ptb, ch, &bDupChar);
    if (-1 != iPos)
    {
        *pidCmd = ptb->Buttons[iPos].idCommand;
        return TRUE;
    }
    return FALSE;
}


BOOL TBOnKey(PTBSTATE ptb, int nVirtKey, UINT uFlags)
{
    NMKEY nm;

    // Send the notification
    nm.nVKey = nVirtKey;
    nm.uFlags = uFlags;
    if (CCSendNotify(&ptb->ci, NM_KEYDOWN, &nm.hdr))
        return TRUE;

    // Swap the left and right arrow key if the control is mirrored.
    nVirtKey = RTLSwapLeftRightArrows(&ptb->ci, nVirtKey);

    if (ptb->iHot != -1 && TB_IsDropDown(&ptb->Buttons[ptb->iHot])) {
        // if we're on a dropdown button and you hit the up/down arrow (left/rigth in vert mode)
        // then drop the button down.
        // escape undrops it if it's dropped
        switch (nVirtKey) {
        case VK_RIGHT:
        case VK_LEFT:
            if (!(ptb->ci.style & CCS_VERT))
                break;
            goto DropDown;

        case VK_DOWN:
        case VK_UP:
            if ((ptb->ci.style & CCS_VERT) || (ptb->dwStyleEx & TBSTYLE_EX_VERTICAL))
                break;
            goto DropDown;


        case VK_ESCAPE:
            if (ptb->iHot != ptb->iPressedDD)
                break;
DropDown:
            TBToggleDropDown(ptb, ptb->iHot, FALSE);
#ifdef KEYBOARDCUES
            //notify of navigation key usage
            CCNotifyNavigationKeyUsage(&(ptb->ci), UISF_HIDEFOCUS | UISF_HIDEACCEL);
#endif
            return TRUE;
        }
    }


    switch (nVirtKey) {
    case VK_RIGHT:
    case VK_DOWN:
        TBCycleHotItem(ptb, ptb->iHot, 1, HICF_ARROWKEYS);
        break;

    case VK_LEFT:
    case VK_UP:
        TBCycleHotItem(ptb, ptb->iHot, -1, HICF_ARROWKEYS);
        break;

    case VK_SPACE:
    case VK_RETURN:
        if (ptb->iHot != -1)
        {
            FORWARD_WM_COMMAND(ptb->ci.hwndParent, ptb->Buttons[ptb->iHot].idCommand, ptb->ci.hwnd, BN_CLICKED, SendMessage);
        }
        break;

    default:
        return FALSE;
    }

#ifdef KEYBOARDCUES
    //notify of navigation key usage
    CCNotifyNavigationKeyUsage(&(ptb->ci), UISF_HIDEFOCUS | UISF_HIDEACCEL);
#endif
    return TRUE;
}

#ifdef UNICODE
LRESULT TB_OnSetButtonInfoA(PTBSTATE ptb, int idBtn, LPTBBUTTONINFOA ptbbiA)
{
    TBBUTTONINFO tbbi = *(LPTBBUTTONINFO)ptbbiA;
    WCHAR szText[256];

    if ((ptbbiA->dwMask & TBIF_TEXT) && ptbbiA->pszText)
    {
        tbbi.pszText = szText;
        tbbi.cchText = ARRAYSIZE(szText);

        MultiByteToWideChar(CP_ACP, 0, (LPCSTR) ptbbiA->pszText, -1,
                            szText, ARRAYSIZE(szText));
    }

    return TB_OnSetButtonInfo(ptb, idBtn, (LPTBBUTTONINFO)&tbbi);
}

LRESULT TB_OnGetButtonInfoA(PTBSTATE ptb, int idBtn, LPTBBUTTONINFOA ptbbiA)
{
    LPTBBUTTONDATA ptbButton;
    int iPos;
    DWORD dwMask = ptbbiA->dwMask;

    ptbbiA->dwMask &= ~TBIF_TEXT;

    iPos = (int) TB_OnGetButtonInfo(ptb, idBtn, (LPTBBUTTONINFO)ptbbiA);

    if (iPos != -1)
    {
        ptbButton = ptb->Buttons + iPos;

        ptbbiA->dwMask = dwMask;
        if (ptbbiA->dwMask & TBIF_TEXT) {
            if (TBISSTRINGPTR(ptbButton->iString)) {
                WideCharToMultiByte (CP_ACP, 0, (LPCTSTR)ptbButton->iString,
                                     -1, ptbbiA->pszText , ptbbiA->cchText, NULL, NULL);
            } else {
                ptbbiA->pszText[0] = 0;
            }
        }
    }

    return iPos;
}

#endif


void TBOnMouseMove(PTBSTATE ptb, HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    if (ptb->fActive)
    {
        BOOL fSameButton;
        BOOL fDragOut = FALSE;
        int iPos;

        // do drag notifies/drawing first
        if (ptb->pCaptureButton != NULL)
        {
            if (hwnd != GetCapture())
            {
                //DebugMsg(DM_TRACE, TEXT("capture isn't us"));
                SendItemNotify(ptb, ptb->pCaptureButton->idCommand, TBN_ENDDRAG);

                // Revalidate after calling out
                if (!IsWindow(hwnd)) return;

                // if the button is still pressed, unpress it.
                if (EVAL(ptb->pCaptureButton) &&
                    (ptb->pCaptureButton->fsState & TBSTATE_PRESSED))
                    SendMessage(hwnd, TB_PRESSBUTTON, ptb->pCaptureButton->idCommand, 0L);
                ptb->pCaptureButton = NULL;
                ptb->fRightDrag = FALSE; // just in case we were right dragging
            }
            else
            {
                //DebugMsg(DM_TRACE, TEXT("capture IS us, and state is enabled"));
                iPos = TBHitTest(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                fSameButton = (iPos >= 0 && ptb->pCaptureButton == ptb->Buttons + iPos);

                // notify on first drag out
                if (!fSameButton && !ptb->fDragOutNotify)
                {
                    ptb->fDragOutNotify = TRUE;
                    fDragOut = (BOOL)SendItemNotify(ptb, ptb->pCaptureButton->idCommand, TBN_DRAGOUT);

                    // Revalidate after calling out
                    if (!IsWindow(hwnd)) return;

                }

                // Check for ptb->pCaptureButton in case it was somehow nuked
                // in TBN_DRAGOUT.
                // This happens in the case when dragging an item out of start menu. When the
                // notify TBN_DRAGOUT is received, they go into a modal drag drop loop. Before
                // This loop finishes, the file is moved, causing a shell change notify to nuke
                // the button, which invalidates pCatpure button. So I'm getting rid of the
                // eval (lamadio) 4.14.98

                if (ptb->pCaptureButton &&
                    (ptb->pCaptureButton->fsState & TBSTATE_ENABLED) &&
                    (fSameButton == !(ptb->pCaptureButton->fsState & TBSTATE_PRESSED)) &&
                    !ptb->fRightDrag)
                {
                    //DebugMsg(DM_TRACE, TEXT("capture IS us, and Button is different"));

                    ptb->pCaptureButton->fsState ^= TBSTATE_PRESSED;

                    InvalidateButton(ptb, ptb->pCaptureButton, TRUE);

                    MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd,
                        OBJID_CLIENT, (ptb->pCaptureButton - ptb->Buttons) + 1);
                }
            }
        }

        if (!fDragOut)
        {
            TBRelayToToolTips(ptb, wMsg, wParam, lParam);

            // Support hot tracking?
            if ((ptb->ci.style & TBSTYLE_FLAT) )
            {
                // Yes; set the hot item
                iPos = TBHitTest(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
#ifdef UNIX
                if (wParam & MK_LBUTTON)
                   TBSetHotItem(ptb, iPos, HICF_MOUSE | HICF_LMOUSE);
                else
#endif
                TBSetHotItem(ptb, iPos, HICF_MOUSE);

                // Track mouse events now?
                if (!ptb->fMouseTrack && !ptb->fAnchorHighlight)
                {
                    // Yes
                    TRACKMOUSEEVENT tme;

                    tme.cbSize = sizeof(TRACKMOUSEEVENT);
                    tme.dwFlags = TME_LEAVE;
                    tme.hwndTrack = hwnd;
                    ptb->fMouseTrack = TRUE;
                    TrackMouseEvent(&tme);
                }
            }
        }
    }

}


void TBHandleLButtonDown(PTBSTATE ptb, LPARAM lParam, int iPos)
{
    LPTBBUTTONDATA ptbButton;
    HWND hwnd = ptb->ci.hwnd;
    if (iPos >= 0 && iPos < ptb->iNumButtons)
    {
        POINT pt;
        RECT rcDropDown;

        LPARAM_TO_POINT(lParam, pt);

#ifdef UNIX
        TBSetHotItemWithoutNotification(ptb, iPos, HICF_MOUSE | HICF_LMOUSE);
#endif /* UNIX */

        // should this check for the size of the button struct?
        ptbButton = ptb->Buttons + iPos;

        if (TB_IsDropDown(ptbButton))
            TB_GetItemDropDownRect(ptb, iPos, &rcDropDown);

        if (TB_IsDropDown(ptbButton) &&
            (!TB_HasSplitDDArrow(ptb, ptbButton) || PtInRect(&rcDropDown, pt))) {

            // Was the dropdown handled?
            if (!TBToggleDropDown(ptb, iPos, TRUE))
            {
                // No; consider it a drag-out
                ptb->pCaptureButton = ptbButton;
                SetCapture(hwnd);

                ptb->fDragOutNotify = FALSE;
                SendItemNotify(ptb, ptb->pCaptureButton->idCommand, TBN_BEGINDRAG);
                GetMessagePosClient(ptb->ci.hwnd, &ptb->ptCapture);
            }

        } else {
            ptb->pCaptureButton = ptbButton;
            SetCapture(hwnd);

            if (ptbButton->fsState & TBSTATE_ENABLED)
            {
                ptbButton->fsState |= TBSTATE_PRESSED;
                InvalidateButton(ptb, ptbButton, TRUE);
                UpdateWindow(hwnd);         // immediate feedback

                MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd,
                    OBJID_CLIENT, iPos+1);
            }

            ptb->fDragOutNotify = FALSE;

            // pCaptureButton may have changed
            if (ptb->pCaptureButton)
                SendItemNotify(ptb, ptb->pCaptureButton->idCommand, TBN_BEGINDRAG);
            GetMessagePosClient(ptb->ci.hwnd, &ptb->ptCapture);
        }
    }
}


void TBOnLButtonDown(PTBSTATE ptb, HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    int iPos;
    NMCLICK nm = {0};

    ptb->fRequeryCapture = FALSE;
    TBRelayToToolTips(ptb, wMsg, wParam, lParam);

    iPos = TBHitTest(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    if ((ptb->ci.style & CCS_ADJUSTABLE) &&
        (((wParam & MK_SHIFT) && !(ptb->ci.style & TBSTYLE_ALTDRAG)) ||
         ((GetKeyState(VK_MENU) & ~1) && (ptb->ci.style & TBSTYLE_ALTDRAG))))
    {
        MoveButton(ptb, iPos);
    }
    else {
        TBHandleLButtonDown(ptb, lParam, iPos);
    }

    if ((iPos >= 0) && (iPos < ptb->iNumButtons))
    {
        nm.dwItemSpec = ptb->Buttons[iPos].idCommand;
        nm.dwItemData = ptb->Buttons[iPos].dwData;
    }
    else
        nm.dwItemSpec = (UINT_PTR) -1;

    LPARAM_TO_POINT(lParam, nm.pt);

    CCSendNotify(&ptb->ci, NM_LDOWN, (LPNMHDR )&nm);
}


void TBOnLButtonUp(PTBSTATE ptb, HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    int iPos = -1;
    NMCLICK nm = { 0 };

    TBRelayToToolTips(ptb, wMsg, wParam, lParam);
    if (lParam != (LPARAM)-1)
        iPos = TBHitTest(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

    if (ptb->fRequeryCapture && iPos >= 0) {
        // hack for broderbund (and potentially mand mfc apps.
        // on button down, they delete the pressed button and insert another one right underneat that
        // has pretty much the same characteristics.
        // on win95, we allowed pCaptureButton to temporarily point to bogus crap.
        // now we validate against it.
        // we detect this case on delete now and if the creation size (uStructSize == old 0x14 size)
        // we reget the capture button here
        ptb->pCaptureButton = &ptb->Buttons[iPos];
    }

    if (ptb->pCaptureButton != NULL) {

        int idCommand = ptb->pCaptureButton->idCommand;

        if (!CCReleaseCapture(&ptb->ci)) return;

        SendItemNotify(ptb, idCommand, TBN_ENDDRAG);
        if (!IsWindow(hwnd)) return;

        if (ptb->pCaptureButton && (ptb->pCaptureButton->fsState & TBSTATE_ENABLED) && iPos >=0
            && (ptb->pCaptureButton == ptb->Buttons+iPos)) {

            ptb->pCaptureButton->fsState &= ~TBSTATE_PRESSED;

            if (ptb->pCaptureButton->fsStyle & BTNS_CHECK) {
                if (ptb->pCaptureButton->fsStyle & BTNS_GROUP) {

                    // group buttons already checked can't be force
                    // up by the user.

                    if (ptb->pCaptureButton->fsState & TBSTATE_CHECKED) {
                        ptb->pCaptureButton = NULL;
                        return; // bail!
                    }

                    ptb->pCaptureButton->fsState |= TBSTATE_CHECKED;
                    MakeGroupConsistant(ptb, idCommand);
                } else {
                    ptb->pCaptureButton->fsState ^= TBSTATE_CHECKED; // toggle
                }
            }
            InvalidateButton(ptb, ptb->pCaptureButton, TRUE);
            ptb->pCaptureButton = NULL;

            MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd,  OBJID_CLIENT,
                iPos+1);

            FORWARD_WM_COMMAND(ptb->ci.hwndParent, idCommand, hwnd, BN_CLICKED, SendMessage);

#ifdef UNIX
                /* There is a special toolbar code depended on WM_MOUSEMOVE
                 * message (mfc400/barcore.c CControlBar::PreTranslateMessage)
                 * MS Windows Posts WM_MOUSEMOVE message after WM_LBUTTONDOWN.
                 * but MainWin does not post this message.
                 * MS Windows always Post WM_MOUSEMOVE message while MainWin
                 * always Send WM_MOUSEMOVE message.
                 * We need Post this message in order for toolbar to work
                 * correctly */
                PostMessage(hwnd, WM_MOUSEMOVE, 0x0000, lParam);
#endif

            // do not dereference ptb... it might have been destroyed on the WM_COMMAND.
            // if the window has been destroyed, bail out.
            if (!IsWindow(hwnd))
                return;

            goto SendUpClick;
        }
        else {
            ptb->pCaptureButton = NULL;
        }
    }
    else
    {
SendUpClick:
        if ((iPos >= 0) && (iPos < ptb->iNumButtons)) {
            nm.dwItemSpec = ptb->Buttons[iPos].idCommand;
            nm.dwItemData = ptb->Buttons[iPos].dwData;
        } else
            nm.dwItemSpec = (UINT_PTR) -1;

        LPARAM_TO_POINT(lParam, nm.pt);

        CCSendNotify(&ptb->ci, NM_CLICK, (LPNMHDR )&nm);
    }
}


BOOL CALLBACK GetUpdateRectEnumProc(HWND hwnd, LPARAM lParam)
{
    PTBSTATE ptb = (PTBSTATE)lParam;

    if (IsWindowVisible(hwnd))
    {
        RECT rcInvalid;

        if (GetUpdateRect(hwnd, &rcInvalid, FALSE))
        {
            RECT rcNew;

            MapWindowPoints(hwnd, ptb->ci.hwnd, (LPPOINT)&rcInvalid, 2);
            UnionRect(&rcNew, &rcInvalid, &ptb->rcInvalid);
            ptb->rcInvalid = rcNew;
        }
    }

    return TRUE;
}

void TB_OnSize(PTBSTATE ptb, int nWidth, int nHeight)
{
    if (ptb->dwStyleEx & TBSTYLE_EX_HIDECLIPPEDBUTTONS)
    {
        // figure out which buttons intersect the resized region
        // and invalidate the rects for those buttons
        //
        // +---------------+------+
        // |               |     <--- rcResizeH
        // |               |      |
        // +---------------+------+
        // |   ^           |      |
        // +---|-----------+------+
        //     rcResizeV
        
        int i;
        RECT rcResizeH, rcResizeV;

        SetRect(&rcResizeH, min(ptb->rc.right, nWidth),
                            ptb->rc.top,
                            max(ptb->rc.right, nWidth),
                            min(ptb->rc.bottom, nHeight));

        SetRect(&rcResizeV, ptb->rc.left,
                            min(ptb->rc.bottom, nHeight),
                            min(ptb->rc.right, nWidth),
                            max(ptb->rc.bottom, nHeight));

        for (i = 0; i < ptb->iNumButtons; i++)
        {
            RECT rc, rcBtn;
            TB_GetItemRect(ptb, i, &rcBtn);
            if (IntersectRect(&rc, &rcBtn, &rcResizeH) ||
                IntersectRect(&rc, &rcBtn, &rcResizeV))
            {
                InvalidateRect(ptb->ci.hwnd, &rcBtn, TRUE);
            }
        }

        SetRect(&ptb->rc, 0, 0, nWidth, nHeight);
    }
}

BOOL TB_TranslateAccelerator(HWND hwnd, LPMSG lpmsg)
{
    if (!lpmsg)
        return FALSE;

    if (GetFocus() != hwnd)
        return FALSE;

    switch (lpmsg->message) {

    case WM_KEYUP:
    case WM_KEYDOWN:

        switch (lpmsg->wParam) {

        case VK_RIGHT:
        case VK_LEFT:
        case VK_UP:
        case VK_DOWN:
        case VK_ESCAPE:
        case VK_SPACE:
        case VK_RETURN:
            TranslateMessage(lpmsg);
            DispatchMessage(lpmsg);
            return TRUE;
        }
        break;

    case WM_CHAR:
        switch (lpmsg->wParam) {

        case VK_ESCAPE:
        case VK_SPACE:
        case VK_RETURN:
            TranslateMessage(lpmsg);
            DispatchMessage(lpmsg);
            return TRUE;
        }
        break;

    }

    return FALSE;
}

void TBInitMetrics(PTBSTATE ptb)
{
    // init our g_clr's
    InitGlobalColors();

    // get the size of a drop down arrow
    ptb->dxDDArrowChar = GetSystemMetrics(SM_CYMENUCHECK);
}

LRESULT TBGenerateDragImage(PTBSTATE ptb, SHDRAGIMAGE* pshdi)
{
    HBITMAP hbmpOld = NULL;
    NMTBCUSTOMDRAW  tbcd = { 0 };
    HDC  hdcDragImage;
    // Do we have a hot item?
    if (ptb->iHot == -1)
        return 0;       // No? Return...

    hdcDragImage = CreateCompatibleDC(NULL);

    if (!hdcDragImage)
        return 0;

    //
    // Mirror the the DC, if the toolbar is mirrored.
    //
    if (ptb->ci.dwExStyle & RTL_MIRRORED_WINDOW)
    {
        SET_DC_RTL_MIRRORED(hdcDragImage);
    }

    tbcd.nmcd.hdc = hdcDragImage;
    ptb->ci.dwCustom = CICustomDrawNotify(&ptb->ci, CDDS_PREPAINT, (NMCUSTOMDRAW *)&tbcd);
    pshdi->sizeDragImage.cx = TBWidthOfButton(ptb, &ptb->Buttons[ptb->iHot], hdcDragImage);
    pshdi->sizeDragImage.cy = ptb->iButHeight;
    pshdi->hbmpDragImage = CreateBitmap( pshdi->sizeDragImage.cx, pshdi->sizeDragImage.cy,
        GetDeviceCaps(hdcDragImage, PLANES), GetDeviceCaps(hdcDragImage, BITSPIXEL),
        NULL);

    if (pshdi->hbmpDragImage)
    {
        DWORD dwStyle;
        RECT  rc = {0, 0, pshdi->sizeDragImage.cx, pshdi->sizeDragImage.cy};
        hbmpOld = SelectObject(hdcDragImage, pshdi->hbmpDragImage);

        pshdi->crColorKey = RGB(0xFF, 0x00, 0x55);

        FillRectClr(hdcDragImage, &rc, pshdi->crColorKey);

        // We want the button to be drawn transparent. This is a hack, because I
        // don't want to rewrite the draw code. Fake a transparent draw.
        dwStyle = ptb->ci.style;
        ptb->ci.style |= TBSTYLE_TRANSPARENT;
        ptb->fAntiAlias = FALSE;

        DrawButton(hdcDragImage, 0, 0, ptb, &ptb->Buttons[ptb->iHot], TRUE);

        ptb->fAntiAlias = TRUE;
        ptb->ci.style = dwStyle;

        TB_GetItemRect(ptb, ptb->iHot, &rc);
        if (PtInRect(&rc, ptb->ptCapture))
        {
           if (ptb->ci.dwExStyle & RTL_MIRRORED_WINDOW)
               pshdi->ptOffset.x = rc.right - ptb->ptCapture.x;
           else
               pshdi->ptOffset.x = ptb->ptCapture.x - rc.left;
           pshdi->ptOffset.y = ptb->ptCapture.y - rc.top;
        }

        SelectObject(hdcDragImage, hbmpOld);
        DeleteDC(hdcDragImage);

        // We're passing back the created HBMP.
        return 1;
    }

    return 0;
}

LRESULT CALLBACK ToolbarWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPTBBUTTONDATA ptbButton;
    int iPos;
    LRESULT dw;
    PTBSTATE ptb = (PTBSTATE)GetWindowPtr0(hwnd);   // GetWindowPtr(hwnd, 0)

    if (uMsg == WM_NCCREATE)
    {
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;

        InitDitherBrush();

        // create the state data for this toolbar

        ptb = (PTBSTATE)LocalAlloc(LPTR, sizeof(TBSTATE));
        if (!ptb)
            return 0;   // WM_NCCREATE failure is 0

        // note, zero init memory from above
        CIInitialize(&ptb->ci, hwnd, lpcs);
        ptb->xFirstButton = s_xFirstButton;
        ptb->iHot = -1;
        ptb->iPressedDD = -1;
        ptb->iInsert = -1;
        ptb->clrim = CLR_DEFAULT;
        ptb->fAntiAlias = TRUE; // Anti Alias fonts by default.
        // initialize system metric-dependent stuff
        TBInitMetrics(ptb);

        // horizontal/vertical space taken up by button chisel, sides,
        // and a 1 pixel margin.  used in GrowToolbar.
        ptb->xPad = 7;
        ptb->yPad = 6;
        ptb->fShowPrefix = TRUE;

        ptb->iListGap = LIST_GAP;
        ptb->iDropDownGap = DROPDOWN_GAP;

        ptb->clrsc.clrBtnHighlight = ptb->clrsc.clrBtnShadow = CLR_DEFAULT;

        ASSERT(ptb->uStructSize == 0);
        ASSERT(ptb->hfontIcon == NULL);  // initialize to null.
        ASSERT(ptb->iButMinWidth == 0);
        ASSERT(ptb->iButMaxWidth == 0);
#ifndef UNICODE
        ASSERT(ptb->bLeadByte == 0);
#endif

        ptb->nTextRows = 1;
        ptb->fActive = TRUE;

        // IE 3 passes in TBSTYLE_FLAT, but they really
        // wanted TBSTYLE_TRANSPARENT also.
        //
        if (ptb->ci.style & TBSTYLE_FLAT) {
            ptb->ci.style |= TBSTYLE_TRANSPARENT;
        }

#ifdef DEBUG
        if (IsFlagSet(g_dwPrototype, PTF_FLATLOOK))
        {
            TraceMsg(TF_TOOLBAR, "Using flat look for toolbars.");
            ptb->ci.style |= TBSTYLE_FLAT;
        }
#endif

        // Now Initialize the hfont we will use.
        TBChangeFont(ptb, 0, NULL);

        // grow the button size to the appropriate girth
        if (!SetBitmapSize(ptb, DEFAULTBITMAPX, DEFAULTBITMAPX))
        {
            goto Failure;
        }

        SetWindowPtr(hwnd, 0, ptb);

        if (!(ptb->ci.style & (CCS_TOP | CCS_NOMOVEY | CCS_BOTTOM)))
        {
            ptb->ci.style |= CCS_TOP;
            SetWindowLong(hwnd, GWL_STYLE, ptb->ci.style);
        }

        return TRUE;

Failure:
        if (ptb) {
            ASSERT(!ptb->Buttons);  // App hasn't had a change to AddButtons yet
            LocalFree(ptb);
        }
        return FALSE;
    }

    if (!ptb)
        goto DoDefault;

    switch (uMsg) {

    case WM_CREATE:
        if (ptb->ci.style & TBSTYLE_REGISTERDROP)
        {
            ptb->hDragProxy = CreateDragProxy(ptb->ci.hwnd, ToolbarDragCallback, TRUE);
        }
        goto DoDefault;

    case WM_DESTROY:
        TB_OnDestroy(ptb);
        break;

    case WM_KEYDOWN:
        if (TBOnKey(ptb, (int) wParam, HIWORD(lParam)))
            break;
        goto DoDefault;

#ifdef KEYBOARDCUES
    case WM_UPDATEUISTATE:
    {
        if (CCOnUIState(&(ptb->ci), WM_UPDATEUISTATE, wParam, lParam))
        {
            BOOL fSmooth = FALSE;
#ifdef CLEARTYPE    // Don't use SPI_CLEARTYPE because it's defined because of APIThk, but not in NT.
            SystemParametersInfo(SPI_GETCLEARTYPE, 0, &fSmooth, 0);
#endif
            // We erase background only if we are removing underscores or focus rect,
            // or if Font smooting is enabled
            InvalidateRect(hwnd, NULL, 
                 fSmooth || ((UIS_SET == LOWORD(wParam)) ? TRUE : FALSE));
        }

        goto DoDefault;
    }
#else
    case WM_KEYBOARDCUES:
    {
        // This message is sent by a window when that window know that it is being accessed
        // by the keyboard. We default to Show Keyboard Cues until this message is sent.
        // Then we respect that setting.

        LRESULT lres = (ptb->fShowPrefix)? KC_ON : KC_OFF;
        
        switch(LOWORD(wParam))
        {
        case KC_ON:     ptb->fShowPrefix = TRUE; break;    // Turn on Keyboard Cues
        case KC_OFF:    ptb->fShowPrefix = FALSE; break;    // Turn off Keyboard Cues
        // case KC_QUERY: For this we just return the previous state.
        }

        // If the current state is different then the last state, 
        // then we want to repaint
        if ((KC_ON == lres) ^ ptb->fShowPrefix)
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);

        return lres;
    }
#endif

    case WM_GETDLGCODE:
        return (LRESULT) (DLGC_WANTARROWS | DLGC_WANTCHARS);

    case WM_SYSCHAR:
    case WM_CHAR:
#ifndef UNICODE
        if (ptb->bLeadByte)
        {
            wParam = (ptb->bLeadByte << 8) | (wParam & 0x00FF);
            ptb->bLeadByte = 0;
        }
        else if (IsDBCSLeadByte((BYTE)wParam))
        {
            ptb->bLeadByte = (BYTE)wParam;
            break;
        }
#endif
        if (!TBOnChar(ptb, (UINT) wParam) &&
            (ptb->ci.iVersion >= 5))
        {
            // didn't handle it & client is >= v5
            // forward to default handler
            goto DoDefault;
        }
        break;

    case WM_SETFOCUS:
        if (ptb->iHot == -1) {
            // set hot the first enabled button
            TBCycleHotItem(ptb, -1, 1, HICF_OTHER);
        }
        break;

    case WM_KILLFOCUS:
        TBSetHotItem(ptb, -1, HICF_OTHER);
        break;

    case WM_SETFONT:
        TBSetFont(ptb, (HFONT)wParam, (BOOL)lParam);
        return TRUE;

    case WM_NCCALCSIZE:
        // let defwindowproc handle the standard borders etc...
        dw = DefWindowProc(hwnd, uMsg, wParam, lParam ) ;

        // add the extra edge at the top of the toolbar to seperate from the menu bar
        if (!(ptb->ci.style & CCS_NODIVIDER))
        {
            ((NCCALCSIZE_PARAMS *)lParam)->rgrc[0].top += g_cyEdge;
        }

        return dw;

    case WM_NCHITTEST:
        return HTCLIENT;

    case WM_NCACTIVATE:

        // only make sense to do this stuff if we're top level
        if ((BOOLIFY(ptb->fActive) != (BOOL)wParam && !GetParent(hwnd))) {
            int iButton;

            ptb->fActive = (BOOL) wParam;

            for (iButton = 0; iButton < ptb->iNumButtons; iButton++) {
                ptbButton = &ptb->Buttons[iButton];
                InvalidateButton(ptb, ptbButton, FALSE);
            }
        }
        // fall through...

    case WM_NCPAINT:
        // old-style toolbars are forced to be without dividers above
        if (!(ptb->ci.style & CCS_NODIVIDER))
        {
            RECT rc;
            HDC hdc = GetWindowDC(hwnd);
            GetWindowRect(hwnd, &rc);
            MapWindowRect(NULL, hwnd, &rc); // screen -> client

                rc.bottom = -rc.top;                // bottom of NC area
                rc.top = rc.bottom - g_cyEdge;

            CCDrawEdge(hdc, &rc, BDR_SUNKENOUTER, BF_TOP | BF_BOTTOM, &(ptb->clrsc));
            ReleaseDC(hwnd, hdc);
        }
        goto DoDefault;

    case WM_ENABLE:
        if (wParam) {
            ptb->ci.style &= ~WS_DISABLED;
        } else {
            ptb->ci.style |= WS_DISABLED;
        }
        InvalidateRect(hwnd, NULL, ptb->ci.style & TBSTYLE_TRANSPARENT);
        goto DoDefault;

    case WM_PRINTCLIENT:
    case WM_PAINT:
        if (ptb->fTTNeedsFlush)
            FlushToolTipsMgrNow(ptb);

        if (ptb->fRedrawOff)
        {
            if (!wParam)
            {
                HDC hdcPaint;
                PAINTSTRUCT ps;

                hdcPaint = BeginPaint(hwnd, &ps);
                EndPaint(hwnd, &ps);
            }

            // we got a paint region, so invalidate
            // when we get redraw back on...
            ptb->fInvalidate = TRUE;
        }
        else
        {
            TBPaint(ptb, (HDC)wParam);
        }
        break;

    case WM_SETREDRAW:
        {
            // HACKHACK: only respect WM_SETREDRAW message if tbstyle is flat
            // HACKHACK: fixes appcompat bug #60120
            if (ptb->ci.style & TBSTYLE_FLAT || 
                ptb->dwStyleEx & TBSTYLE_EX_VERTICAL || 
                (ptb->ci.iVersion >= 5))
            {
                BOOL fRedrawOld = !ptb->fRedrawOff;

                if ( wParam && ptb->fRedrawOff )
                {
                    if ( ptb->fInvalidate )
                    {
                        // If font smoothing is enabled, then we need to erase the background too.
                        BOOL fSmooth = FALSE;
#ifdef CLEARTYPE    // Don't use SPI_CLEARTYPE because it's defined because of APIThk, but not in NT.
                        SystemParametersInfo(SPI_GETCLEARTYPE, 0, &fSmooth, 0);
#endif


                        // invalidate before turning back on ...
                        RedrawWindow( hwnd, NULL, NULL, (fSmooth? RDW_ERASE: 0)  | RDW_INVALIDATE );
                        ptb->fInvalidate = FALSE;
                    }
                    ptb->fRedrawOff = FALSE;

                    if ( ptb->fRecalc )
                    {
                        // recalc & autosize after turning back on
                        TBRecalc(ptb);
                        TBAutoSize(ptb);
                        ptb->fRecalc = FALSE;
                    }
                }
                else
                {
                    ptb->fRedrawOff = !wParam;
                }

                if (ptb->ci.iVersion >= 5)
                    return fRedrawOld;
            }
            else
            {
                goto DoDefault;
            }
        }
        break;

    case WM_ERASEBKGND:
        TB_OnEraseBkgnd(ptb, (HDC) wParam);
        return(TRUE);

    case WM_SYSCOLORCHANGE:
        TB_OnSysColorChange(ptb);
        if (ptb->hwndToolTips)
            SendMessage(ptb->hwndToolTips, uMsg, wParam, lParam);
        break;

    case TB_GETROWS:
        return CountRows(ptb);
        break;

    case TB_GETPADDING:
        lParam = MAKELONG(-1, -1);
        // fall through
    case TB_SETPADDING:
    {
        LRESULT lres = MAKELONG(ptb->xPad, ptb->yPad);
        int xPad = GET_X_LPARAM(lParam);
        int yPad = GET_Y_LPARAM(lParam);
        if (xPad != -1)
            ptb->xPad = xPad;
        if (yPad != -1)
            ptb->yPad = yPad;
        return lres;
    }


    case TB_SETROWS:
        {
            RECT rc;

            if (BoxIt(ptb, LOWORD(wParam), HIWORD(wParam), &rc))
            {
                TBInvalidateItemRects(ptb);
                SetWindowPos(hwnd, NULL, 0, 0, rc.right, rc.bottom,
                             SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
                InvalidateRect(hwnd, NULL, TRUE);
            }
            if (lParam)
                *((RECT *)lParam) = rc;
        }
        break;

    case WM_MOVE:
        // JJK TODO: This needs to be double buffered to get rid of the flicker
        if (ptb->ci.style & TBSTYLE_TRANSPARENT)
            InvalidateRect(hwnd, NULL, TRUE);
        goto DoDefault;

    case WM_SIZE:
        TB_OnSize(ptb, LOWORD(lParam), HIWORD(lParam));
        // fall through
    case TB_AUTOSIZE:
        TBAutoSize(ptb);
        break;

    case WM_COMMAND:
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
        SendMessage(ptb->ci.hwndParent, uMsg, wParam, lParam);
        break;

    case WM_RBUTTONDBLCLK:
        if (!CCSendNotify(&ptb->ci, NM_RDBLCLK, NULL))
            goto DoDefault;
        break;

    case WM_RBUTTONUP:
        {
            NMCLICK nm = {0};
            int iIndex;

            if (ptb->pCaptureButton != NULL)
            {
                if (!CCReleaseCapture(&ptb->ci)) break;
                ptb->pCaptureButton = NULL;
                ptb->fRightDrag = FALSE;
            }

            iIndex = TBHitTest(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            if ((iIndex >= 0) && (iIndex < ptb->iNumButtons)) {
                nm.dwItemSpec = ptb->Buttons[iIndex].idCommand;
                nm.dwItemData = ptb->Buttons[iIndex].dwData;
            } else
                nm.dwItemSpec = (UINT_PTR) -1;

            LPARAM_TO_POINT(lParam, nm.pt);

            if (!CCSendNotify(&ptb->ci, NM_RCLICK, (LPNMHDR )&nm))
                goto DoDefault;
        }
        break;

    case WM_LBUTTONDBLCLK:
        iPos = TBHitTest(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        if (iPos < 0 && (ptb->ci.style & CCS_ADJUSTABLE))
        {
            iPos = -1 - iPos;
            CustomizeTB(ptb, iPos);
        } else {
            TBHandleLButtonDown(ptb, lParam, iPos);
        }
        break;

    case WM_LBUTTONDOWN:
        TBOnLButtonDown(ptb, hwnd, uMsg, wParam, lParam);
        break;

    case WM_CAPTURECHANGED:
        // do this only for newer apps because some apps
        // do stupid things like delete a button when you
        // mouse down and add it back in immediately.
        // also do it on a post because we call ReleaseCapture
        // internally and only want to catch this on external release
        if (ptb->ci.iVersion >= 5)
            PostMessage(hwnd, TBP_ONRELEASECAPTURE, 0, 0);

        //
        //  QFE fix for Autodesk.  We used to hold capture
        //  even though the app wanted it back.  Oops.
        //
        else if (ptb->fRightDrag && ptb->pCaptureButton) {
            CCReleaseCapture(&ptb->ci);
            ptb->fRightDrag = FALSE;
        }

        break;

    case TBP_ONRELEASECAPTURE:
        if (ptb->pCaptureButton) {
            // abort current capture
            // simulate a lost capture mouse move.  this will restore state
            TBOnMouseMove(ptb, hwnd, WM_MOUSEMOVE, 0, (LPARAM)-1);
            ptb->pCaptureButton = NULL;
        }
        break;


    case WM_RBUTTONDOWN:

        if (ptb->pCaptureButton) {
            // abort current capture
            if (hwnd == GetCapture()) {
                // we were left clicking.   abort that now
                if (!CCReleaseCapture(&ptb->ci)) break;
                // simulate a lost capture mouse move.  this will restore state
                TBOnMouseMove(ptb, hwnd, WM_MOUSEMOVE, 0, (LPARAM)-1);
            }
        }

        iPos = TBHitTest(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

        // we need to check VK_RBUTTON because some idiot apps subclass us to pick off rbuttondown to do their menu
        // (instead of up, or the notify, or wm_contextmenu)
        // then after it's done and the button is up, they then send us a button down
        if ((iPos >= 0) && (iPos < ptb->iNumButtons) && (GetAsyncKeyState(VK_RBUTTON) < 0))
        {
            ptb->pCaptureButton = ptb->Buttons + iPos;
            ptb->fRightDrag = TRUE;
            SetCapture(hwnd);
            GetMessagePosClient(ptb->ci.hwnd, &ptb->ptCapture);

            SendItemNotify(ptb, ptb->pCaptureButton->idCommand, TBN_BEGINDRAG);
            if (!IsWindow(hwnd)) break;
            ptb->fDragOutNotify = FALSE;
        }
        break;

    case WM_MOUSELEAVE:
        {
            TRACKMOUSEEVENT tme;

            // We only track mouse events on the flat style (for
            // hot tracking)
            ASSERT(ptb->ci.style & TBSTYLE_FLAT);

            // Cancel the mouse event tracking
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_CANCEL | TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            ptb->fMouseTrack = FALSE;

#ifdef UNIX
            TBSetHotItem(ptb, -1, HICF_MOUSE | HICF_LMOUSE);
#else
            TBSetHotItem(ptb, -1, HICF_MOUSE);
#endif /* UNIX */
        }
        break;

    case WM_MOUSEMOVE:
        TBOnMouseMove(ptb, hwnd, uMsg, wParam, lParam);
        break;

    case WM_LBUTTONUP:
        TBOnLButtonUp(ptb, hwnd, uMsg, wParam, lParam);
        break;

    case WM_WININICHANGE:
        InitGlobalMetrics(wParam);
        if (ptb->fFontCreated)
            TBChangeFont(ptb, wParam, NULL);
        if (ptb->hwndToolTips)
            SendMessage(ptb->hwndToolTips, uMsg, wParam, lParam);

        // recalc & redraw
        TBInitMetrics(ptb);
        TBRecalc(ptb);
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_NOTIFYFORMAT:
        return CIHandleNotifyFormat(&ptb->ci, lParam);
        break;


    case WM_NOTIFY:
#define lpNmhdr ((LPNMHDR)(lParam))
        //The following statement traps all pager control notification messages.
        if((lpNmhdr->code <= PGN_FIRST)  && (lpNmhdr->code >= PGN_LAST)) {
            return TB_OnPagerControlNotify(ptb, lpNmhdr);
        }
    {
        LRESULT lres = 0;
        if (lpNmhdr->code == TTN_NEEDTEXT) {
            int i = PositionFromID(ptb, lpNmhdr->idFrom);
            BOOL fEllipsied = FALSE;
            LRESULT lres;
            LPTOOLTIPTEXT lpnmTT = ((LPTOOLTIPTEXT) lParam);

            if (i != -1) {
                // if infotip not supported, try for TTN_NEEDTEXT in client.
                if (!TBGetInfoTip(ptb, lpnmTT, &ptb->Buttons[i]))
                    lres = SendNotifyEx(ptb->ci.hwndParent, (HWND) -1,
                                        lpNmhdr->code, lpNmhdr, ptb->ci.bUnicode);

#define IsTextPtr(lpszText)  (((lpszText) != LPSTR_TEXTCALLBACK) && (!IS_INTRESOURCE(lpszText)))

                fEllipsied = (BOOL)(ptb->Buttons[i].fsState & TBSTATE_ELLIPSES);

                // if we don't get a string from TTN_NEEDTEXT try to use the title text.
                if ((lpNmhdr->code == TTN_NEEDTEXT) &&
                    (BTN_NO_SHOW_TEXT(ptb, &ptb->Buttons[i]) || fEllipsied) &&
                    lpnmTT->lpszText && IsTextPtr(lpnmTT->lpszText) &&
                    !lpnmTT->lpszText[0])
                {
                    LPCTSTR psz = TB_StrForButton(ptb, &ptb->Buttons[i]);
                    if (psz)
                        lpnmTT->lpszText = (LPTSTR)psz;
                }
            }
        } else {
            //
            // We are just going to pass this on to the
            // real parent.  Note that -1 is used as
            // the hwndFrom.  This prevents SendNotifyEx
            // from updating the NMHDR structure.
            //
            lres = SendNotifyEx(ptb->ci.hwndParent, (HWND) -1,
                                lpNmhdr->code, lpNmhdr, ptb->ci.bUnicode);
        }
        return(lres);
    }

    case WM_STYLECHANGING:
        if (wParam == GWL_STYLE)
        {
            LPSTYLESTRUCT lpStyle = (LPSTYLESTRUCT) lParam;

            // is MFC dorking with just our visibility bit?
            if ((lpStyle->styleOld ^ lpStyle->styleNew) == WS_VISIBLE)
            {
                if (lpStyle->styleNew & WS_VISIBLE)
                {
                    BOOL fSmooth = FALSE;
#ifdef CLEARTYPE    // Don't use SPI_CLEARTYPE because it's defined because of APIThk, but not in NT.
                    SystemParametersInfo(SPI_GETCLEARTYPE, 0, &fSmooth, 0);
#endif

                    // MFC trying to make us visible,
                    // convert it to WM_SETREDRAW instead.
                    DefWindowProc(hwnd, WM_SETREDRAW, TRUE, 0);

                    // Reinvalidate everything we lost when we
                    // did the WM_SETREDRAW stuff.
                    RedrawWindow(hwnd, &ptb->rcInvalid, NULL, (fSmooth? RDW_ERASE: 0)  | RDW_INVALIDATE | RDW_ALLCHILDREN);
                    ZeroMemory(&ptb->rcInvalid, SIZEOF(ptb->rcInvalid));
                }
                else
                {
                    // Save the invalid rectangle in ptb->rcInvalid since
                    // WM_SETREDRAW will blow it away.
                    ZeroMemory(&ptb->rcInvalid, SIZEOF(ptb->rcInvalid));
                    GetUpdateRect(ptb->ci.hwnd, &ptb->rcInvalid, FALSE);
                    EnumChildWindows(ptb->ci.hwnd, GetUpdateRectEnumProc, (LPARAM)ptb);

                    // MFC trying to make us invisible,
                    // convert it to WM_SETREDRAW instead.
                    DefWindowProc(hwnd, WM_SETREDRAW, FALSE, 0);
                }
            }
        }
        break;

    case WM_STYLECHANGED:
        if (wParam == GWL_STYLE)
        {
            TBSetStyle(ptb, ((LPSTYLESTRUCT)lParam)->styleNew);
        }
        else if (wParam == GWL_EXSTYLE)
        {
            //
            // If the RTL_MIRROR extended style bit had changed, let's
            // repaint the control window
            //
            if ((ptb->ci.dwExStyle&RTL_MIRRORED_WINDOW) !=
                (((LPSTYLESTRUCT)lParam)->styleNew&RTL_MIRRORED_WINDOW))
                TBAutoSize(ptb);

            //
            // Save the new ex-style bits
            //
            ptb->ci.dwExStyle = ((LPSTYLESTRUCT)lParam)->styleNew;

        }
        return 0;

    case TB_GETIDEALSIZE:
        {
            NMPGCALCSIZE nm;
            LPSIZE psize = (LPSIZE) lParam;
            ASSERT(psize);  // This should never be NULL
            nm.dwFlag = wParam ? PGF_CALCHEIGHT : PGF_CALCWIDTH;
            nm.iWidth = psize->cx;
            nm.iHeight = psize->cy;
            TB_OnCalcSize(ptb, (LPNMHDR)&nm);

            // Since both values may have changed, reset the out-param.
            psize->cy = nm.iHeight;
            psize->cx = nm.iWidth;
            return 1;
        }

    case TB_SETSTYLE:
        TBSetStyle(ptb, (DWORD) lParam);
        break;

    case TB_GETSTYLE:
        return (ptb->ci.style);

    case TB_GETBUTTONSIZE:
        return (MAKELONG(ptb->iButWidth,ptb->iButHeight));

    case TB_SETBUTTONWIDTH:
        if (ptb->iButMinWidth  != LOWORD(lParam) ||
            ptb->iButMaxWidth != HIWORD(lParam)) {

            ptb->iButMinWidth  = LOWORD(lParam);
            ptb->iButMaxWidth = HIWORD(lParam);
            ptb->iButWidth = 0;
            TBRecalc(ptb);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return TRUE;

    case TB_TRANSLATEACCELERATOR:
        return TB_TranslateAccelerator(hwnd, (LPMSG)lParam);

    case TB_SETSTATE:
        iPos = PositionFromID(ptb, wParam);
        if (iPos < 0)
            return FALSE;
        ptbButton = ptb->Buttons + iPos;

        TB_OnSetState(ptb, ptbButton, (BYTE)(LOWORD(lParam)), iPos);
        TBInvalidateItemRects(ptb);
        return TRUE;

    // set the cmd ID of a button based on its position
    case TB_SETCMDID:
        if (wParam >= (UINT)ptb->iNumButtons)
            return FALSE;

        TB_OnSetCmdID(ptb, &ptb->Buttons[wParam], (UINT)lParam);
        return TRUE;

    case TB_GETSTATE:
        iPos = PositionFromID(ptb, wParam);
        if (iPos < 0)
            return -1L;
        return ptb->Buttons[iPos].fsState;

#ifdef UNICODE
    case TB_MAPACCELERATORA:
    {
        char szAcl[2];
        WCHAR wszAcl[2];
        szAcl[0] = (BYTE)wParam;
        szAcl[1] = '\0';
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szAcl, ARRAYSIZE(szAcl),
                                               wszAcl, ARRAYSIZE(wszAcl));
        // no need to check return we just take junk if MbtoWc has failed
        wParam = (WPARAM)wszAcl[0];
    }
    // fall through...
#endif
    case TB_MAPACCELERATOR:
#ifndef UNICODE
        // prevent sign extension of high ansi chars
        wParam = (WORD)(BYTE)wParam;
#endif
        return TBOnMapAccelerator(ptb, (UINT)wParam, (UINT *)lParam);

    case TB_ENABLEBUTTON:
    case TB_CHECKBUTTON:
    case TB_PRESSBUTTON:
    case TB_HIDEBUTTON:
    case TB_INDETERMINATE:
    case TB_MARKBUTTON:
    {
        BYTE fsState;

        iPos = PositionFromID(ptb, wParam);
        if (iPos < 0)
            return FALSE;
        ptbButton = &ptb->Buttons[iPos];
        fsState = ptbButton->fsState;

        if (LOWORD(lParam))
            ptbButton->fsState |= wStateMasks[uMsg - TB_ENABLEBUTTON];
        else
            ptbButton->fsState &= ~wStateMasks[uMsg - TB_ENABLEBUTTON];

        // did this actually change the state?
        if (fsState != ptbButton->fsState) {
            // is this button a member of a group?
            if ((uMsg == TB_CHECKBUTTON) && (ptbButton->fsStyle & BTNS_GROUP))
                MakeGroupConsistant(ptb, (int)wParam);

            if (uMsg == TB_HIDEBUTTON) {
                InvalidateRect(hwnd, NULL, TRUE);
                TBInvalidateItemRects(ptb);
            } else
                InvalidateButton(ptb, ptbButton, TRUE);

            MyNotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_CLIENT, iPos+1);
        }
        return(TRUE);
    }

    case TB_ISBUTTONENABLED:
    case TB_ISBUTTONCHECKED:
    case TB_ISBUTTONPRESSED:
    case TB_ISBUTTONHIDDEN:
    case TB_ISBUTTONINDETERMINATE:
    case TB_ISBUTTONHIGHLIGHTED:
        iPos = PositionFromID(ptb, wParam);
        if (iPos < 0)
            return(-1L);
        return (LRESULT)ptb->Buttons[iPos].fsState & wStateMasks[uMsg - TB_ISBUTTONENABLED];

    case TB_ADDBITMAP:
    case TB_ADDBITMAP32:    // only for compatibility with mail
        {
            LPTBADDBITMAP pab = (LPTBADDBITMAP)lParam;
            return AddBitmap(ptb, (int) wParam, pab->hInst, pab->nID);
        }

    case TB_REPLACEBITMAP:
        return ReplaceBitmap(ptb, (LPTBREPLACEBITMAP)lParam);

#ifdef UNICODE
    case TB_ADDSTRINGA:
        {
        LPWSTR lpStrings;
        UINT   uiCount;
        LPSTR  lpAnsiString = (LPSTR) lParam;
        int    iResult;
        BOOL   bAllocatedMem = FALSE;

        if (!wParam && !IS_INTRESOURCE(lpAnsiString)) {
            //
            // We have to figure out how many characters
            // are in this string.
            //
            
            uiCount = 0;

            while (TRUE) {
               uiCount++;
               if ((*lpAnsiString == 0) && (*(lpAnsiString+1) == 0)) {
                  uiCount++;  // needed for double null
                  break;
               }

               lpAnsiString++;
            }

            lpStrings = LocalAlloc(LPTR, uiCount * sizeof(TCHAR));

            if (!lpStrings)
                return -1;

            bAllocatedMem = TRUE;

            MultiByteToWideChar(CP_ACP, 0, (LPCSTR) lParam, uiCount,
                                lpStrings, uiCount);

        } else {
            lpStrings = (LPWSTR)lParam;
        }

        iResult = TBAddStrings(ptb, wParam, (LPARAM)lpStrings);

        if (bAllocatedMem)
            LocalFree(lpStrings);

        return iResult;
        }
#endif

    case TB_ADDSTRING:
        return TBAddStrings(ptb, wParam, lParam);

    case TB_GETSTRING:
        return TBGetString(ptb, HIWORD(wParam), LOWORD(wParam), (LPTSTR)lParam);

#ifdef UNICODE
    case TB_GETSTRINGA:
        return TBGetStringA(ptb, HIWORD(wParam), LOWORD(wParam), (LPSTR)lParam);

    case TB_ADDBUTTONSA:
        return TBInsertButtons(ptb, (UINT)-1, (UINT) wParam, (LPTBBUTTON)lParam, FALSE);

    case TB_INSERTBUTTONA:
        return TBInsertButtons(ptb, (UINT) wParam, 1, (LPTBBUTTON)lParam, FALSE);
#endif

    case TB_ADDBUTTONS:
        return TBInsertButtons(ptb, (UINT)-1, (UINT) wParam, (LPTBBUTTON)lParam, TRUE);

    case TB_INSERTBUTTON:
        return TBInsertButtons(ptb, (UINT) wParam, 1, (LPTBBUTTON)lParam, TRUE);

    case TB_DELETEBUTTON:
        return DeleteButton(ptb, (UINT) wParam);

    case TB_GETBUTTON:
        if (wParam >= (UINT)ptb->iNumButtons)
            return(FALSE);

        TBOutputStruct(ptb, ptb->Buttons + wParam, (LPTBBUTTON)lParam);
        return TRUE;

    case TB_SETANCHORHIGHLIGHT:
        BLOCK
        {
            BOOL bAnchor = BOOLIFY(ptb->fAnchorHighlight);
            ptb->fAnchorHighlight = BOOLFROMPTR(wParam);
            return bAnchor;
        }
        break;

    case TB_GETANCHORHIGHLIGHT:
        return BOOLIFY(ptb->fAnchorHighlight);

    case TB_HASACCELERATOR:
        ASSERT(IS_VALID_WRITE_PTR(lParam, int*));
        *((int*)lParam) = TBHasAccelerator(ptb, (UINT)wParam);
        break;

    case TB_SETHOTITEM:
        lParam = HICF_OTHER;
        // Fall through
    case TB_SETHOTITEM2:
        BLOCK
        {
            int iPos = ptb->iHot;

            TBSetHotItem(ptb, (int)wParam, (DWORD)lParam);
            return iPos;
        }
        break;

    case TB_GETHOTITEM:
        return ptb->iHot;

    case TB_SETINSERTMARK:
        TBSetInsertMark(ptb, (LPTBINSERTMARK)lParam);
        break;

    case TB_GETINSERTMARK:
    {
        LPTBINSERTMARK ptbim = (LPTBINSERTMARK)lParam;

        ptbim->iButton = ptb->iInsert;
        ptbim->dwFlags = ptb->fInsertAfter ? TBIMHT_AFTER : 0;
        return TRUE;
    }

    case TB_SETINSERTMARKCOLOR:
    {
        LRESULT lres = (LRESULT)TB_GetInsertMarkColor(ptb);
        ptb->clrim = (COLORREF) lParam;
        return lres;
    }

    case TB_GETINSERTMARKCOLOR:
        return TB_GetInsertMarkColor(ptb);

    case TB_INSERTMARKHITTEST:
    return (LRESULT)TBInsertMarkHitTest(ptb, ((LPPOINT)wParam)->x, ((LPPOINT)wParam)->y, (LPTBINSERTMARK)lParam);

    case TB_MOVEBUTTON:
        return (LRESULT)TBMoveButton(ptb, (UINT)wParam, (UINT)lParam);

    case TB_GETMAXSIZE:
        return (LRESULT)TBGetMaxSize(ptb, (LPSIZE) lParam );

    case TB_BUTTONCOUNT:
        return ptb->iNumButtons;

    case TB_COMMANDTOINDEX:
        return PositionFromID(ptb, wParam);

#ifdef UNICODE
    case TB_SAVERESTOREA:
        {
        LPWSTR lpSubKeyW, lpValueNameW;
        TBSAVEPARAMSA * lpSaveA = (TBSAVEPARAMSA *) lParam;
        BOOL bResult;

        lpSubKeyW = ProduceWFromA (CP_ACP, lpSaveA->pszSubKey);
        lpValueNameW = ProduceWFromA (CP_ACP, lpSaveA->pszValueName);

        bResult = SaveRestoreFromReg(ptb, (BOOL) wParam, lpSaveA->hkr, lpSubKeyW, lpValueNameW);

        FreeProducedString(lpSubKeyW);
        FreeProducedString(lpValueNameW);

        return bResult;
        }
#endif

    case TB_SAVERESTORE:
        {
            TBSAVEPARAMS* psr = (TBSAVEPARAMS *)lParam;
            return SaveRestoreFromReg(ptb, (BOOL) wParam, psr->hkr, psr->pszSubKey, psr->pszValueName);
        }

    case TB_CUSTOMIZE:
        CustomizeTB(ptb, ptb->iNumButtons);
        break;

    case TB_GETRECT:
        // PositionFromID() accepts NULL ptbs!
        wParam = PositionFromID(ptb, wParam);
        // fall through
    case TB_GETITEMRECT:
        if (!lParam)
            break;
        return TB_GetItemRect(ptb, (UINT) wParam, (LPRECT)lParam);

    case TB_BUTTONSTRUCTSIZE:
        TBOnButtonStructSize(ptb, (UINT) wParam);
        break;

    case TB_SETBUTTONSIZE:
        return GrowToolbar(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);

    case TB_SETBITMAPSIZE:
        return SetBitmapSize(ptb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

    case TB_SETIMAGELIST:
    {
        HIMAGELIST himl = (HIMAGELIST)lParam;
        HIMAGELIST himlOld = TBSetImageList(ptb, HIML_NORMAL, (int) wParam, himl);
        ptb->fHimlNative = TRUE;

        if (!ptb->uStructSize) {
            // because this is stupid to require a call to TB_BUTTONSTRUCTSIZE... on other control requires this
            ptb->uStructSize = 20;
            ASSERT(ptb->uStructSize == SIZEOF(TBBUTTON));
        }

        // The bitmap size is based on the primary image list
        if (wParam == 0)
        {
            int cx = 0, cy = 0;
            if (himl) {
                // Update the bitmap size based on this image list
                ImageList_GetIconSize(himl, &cx, &cy);
            }
            SetBitmapSize(ptb, cx, cy);
        }

        return (LRESULT)himlOld;
    }

    case TB_GETIMAGELIST:
        return (LRESULT)TBGetImageList(ptb, HIML_NORMAL, (int) wParam);

    case TB_GETIMAGELISTCOUNT:
        return ptb->cPimgs;

    case TB_SETHOTIMAGELIST:
        return (LRESULT)TBSetImageList(ptb, HIML_HOT, (int) wParam, (HIMAGELIST)lParam);

    case TB_GETHOTIMAGELIST:
        return (LRESULT)TBGetImageList(ptb, HIML_HOT, (int) wParam);

    case TB_GETDISABLEDIMAGELIST:
        return (LRESULT)TBGetImageList(ptb, HIML_DISABLED, (int) wParam);

    case TB_SETDISABLEDIMAGELIST:
        return (LRESULT)TBSetImageList(ptb, HIML_DISABLED, (int) wParam, (HIMAGELIST)lParam);

    case TB_GETOBJECT:
        if (IsEqualIID((IID *)wParam, &IID_IDropTarget))
        {
            // if we have not already registered create an unregistered target now
            if (ptb->hDragProxy == NULL)
                ptb->hDragProxy = CreateDragProxy(ptb->ci.hwnd, ToolbarDragCallback, FALSE);

            if (ptb->hDragProxy)
                return (LRESULT)GetDragProxyTarget(ptb->hDragProxy, (IDropTarget **)lParam);
        }
        return E_FAIL;

    case WM_GETFONT:
        return (LRESULT)(ptb? ptb->hfontIcon : 0);

    case TB_LOADIMAGES:
        return TBLoadImages(ptb, (UINT_PTR) wParam, (HINSTANCE)lParam);

    case TB_GETTOOLTIPS:
        TB_ForceCreateTooltips(ptb);
        return (LRESULT)ptb->hwndToolTips;

    case TB_SETTOOLTIPS:
        ptb->hwndToolTips = (HWND)wParam;
        break;

    case TB_SETPARENT:
        {
            HWND hwndOld = ptb->ci.hwndParent;

        ptb->ci.hwndParent = (HWND)wParam;
        return (LRESULT)hwndOld;
        }


#ifdef UNICODE
    case TB_GETBUTTONINFOA:
        return TB_OnGetButtonInfoA(ptb, (int)wParam, (LPTBBUTTONINFOA)lParam);

    case TB_SETBUTTONINFOA:
        return TB_OnSetButtonInfoA(ptb, (int)wParam, (LPTBBUTTONINFOA)lParam);
#endif

    case TB_GETBUTTONINFO:
        return TB_OnGetButtonInfo(ptb, (int)wParam, (LPTBBUTTONINFO)lParam);

    case TB_SETBUTTONINFO:
        return TB_OnSetButtonInfo(ptb, (int)wParam, (LPTBBUTTONINFO)lParam);

    case TB_CHANGEBITMAP:
        iPos = PositionFromID(ptb, wParam);
        if (iPos < 0)
            return(FALSE);

        //
        // Check to see if the new bitmap ID is
        // valid.
        //
        ptbButton = &ptb->Buttons[iPos];
        return TB_OnSetImage(ptb, ptbButton, LOWORD(lParam));

    case TB_GETBITMAP:
        iPos = PositionFromID(ptb, wParam);
        if (iPos < 0)
            return(FALSE);
        ptbButton = &ptb->Buttons[iPos];
        return ptbButton->DUMMYUNION_MEMBER(iBitmap);

#ifdef UNICODE
    case TB_GETBUTTONTEXTA:
        iPos = PositionFromID(ptb, wParam);
        if (iPos >= 0) {
            LPTSTR psz;

            ptbButton = &ptb->Buttons[iPos];
            psz = TB_StrForButton(ptb, ptbButton);
            if (psz)
            {
                // Passing a 0 for the length of the buffer when the
                // buffer is NULL returns the number bytes required
                // to convert the string.
                int cbBuff = WideCharToMultiByte (CP_ACP, 0, psz,
                    -1, NULL, 0, NULL, NULL);

                // We used to pass an obscenly large number for the buffer length,
                // but on checked builds, this causes badness. So no we double-dip
                // into WideCharToMultiByte to calculate the real size required.
                if (lParam)
                {
                    WideCharToMultiByte (CP_ACP, 0, psz,
                        -1, (LPSTR)lParam, cbBuff, NULL, NULL);
                }

                // WideChar include a trailing NULL but we don't want to.
                return cbBuff - 1;
            }
        }
        return -1;
#endif

    case TB_GETBUTTONTEXT:
        iPos = PositionFromID(ptb, wParam);
        if (iPos >= 0) {
            LPCTSTR psz;

            ptbButton = &ptb->Buttons[iPos];
            psz = TB_StrForButton(ptb, ptbButton);
            if (psz) {
                if (lParam) {
                    lstrcpy((LPTSTR)lParam, psz);
                }
                return lstrlen(psz);
            }
        }
        return -1;


    case TB_GETBITMAPFLAGS:
        {
            DWORD fFlags = 0;
            HDC hdc = GetDC(NULL);

            if (GetDeviceCaps(hdc, LOGPIXELSY) >= 120)
                fFlags |= TBBF_LARGE;

            ReleaseDC(NULL, hdc);

            return fFlags;
        }

    case TB_SETINDENT:
        ptb->xFirstButton = (int) wParam;
        InvalidateRect (hwnd, NULL, TRUE);
        TBInvalidateItemRects(ptb);
        return 1;

    case TB_SETMAXTEXTROWS:

        if (ptb->nTextRows != (int)wParam) {
            ptb->nTextRows = (int) wParam;
            TBRecalc(ptb);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 1;

    case TB_SETLISTGAP:
        ptb->iListGap = (int) wParam;
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case TB_SETDROPDOWNGAP:
        ptb->iDropDownGap = (int) wParam;
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case TB_GETTEXTROWS:
        return ptb->nTextRows;

    case TB_HITTEST:
        return TBHitTest(ptb, ((LPPOINT)lParam)->x, ((LPPOINT)lParam)->y);

    case TB_SETDRAWTEXTFLAGS:
    {
        UINT uOld = ptb->uDrawText;
        ptb->uDrawText = (UINT) (lParam & wParam);
        ptb->uDrawTextMask = (UINT) wParam;
        return uOld;
    }

    case TB_GETEXTENDEDSTYLE:
        return (ptb->dwStyleEx);

    case TB_SETEXTENDEDSTYLE:
    {
        DWORD dwRet = ptb->dwStyleEx;
        TBSetStyleEx(ptb, (DWORD) lParam, (DWORD) wParam);
        return dwRet;
    }
    case TB_SETBOUNDINGSIZE:
    {
        LPSIZE lpSize = (LPSIZE)lParam;
        ptb->sizeBound = *lpSize;
        break;
    }
    case TB_GETCOLORSCHEME:
    {
        LPCOLORSCHEME lpclrsc = (LPCOLORSCHEME) lParam;
        if (lpclrsc) {
            if (lpclrsc->dwSize == sizeof(COLORSCHEME))
                *lpclrsc = ptb->clrsc;
        }
        return (LRESULT) lpclrsc;
    }

    case TB_SETCOLORSCHEME:
    {
        if (lParam) {
            if (((LPCOLORSCHEME) lParam)->dwSize == sizeof(COLORSCHEME)) {
                ptb->clrsc.clrBtnHighlight = ((LPCOLORSCHEME) lParam)->clrBtnHighlight;
                ptb->clrsc.clrBtnShadow = ((LPCOLORSCHEME) lParam)->clrBtnShadow;
                InvalidateRect(hwnd, NULL, FALSE);
                if (ptb->ci.style & WS_BORDER)
                    CCInvalidateFrame(hwnd);
            }
        }
    }
    break;

    case WM_GETOBJECT:
        if( lParam == OBJID_QUERYCLASSNAMEIDX )
            return MSAA_CLASSNAMEIDX_TOOLBAR;
        goto DoDefault;

    case WM_NULL:
            // Trap failed RegsiterWindowMessages;
        break;

    default:
    {
        LRESULT lres;
        if (g_uDragImages == uMsg)
            return TBGenerateDragImage(ptb, (SHDRAGIMAGE*)lParam);

        if (CCWndProc(&ptb->ci, uMsg, wParam, lParam, &lres))
            return lres;
    }
DoDefault:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0L;
}


int TB_CalcWidth(PTBSTATE ptb, int iHeight)
{
    RECT rc;
    int iWidth = 0;
    int iMaxBtnWidth = 0;  // ptb->iButWidth isn't always width of widest button
    LPTBBUTTONDATA pButton, pBtnLast;
    pBtnLast = &(ptb->Buttons[ptb->iNumButtons]);

    for(pButton = ptb->Buttons; pButton < pBtnLast; pButton++)
    {
        if (!(pButton->fsState & TBSTATE_HIDDEN))
        {
            int iBtnWidth = TBWidthOfButton(ptb, pButton, NULL);
            iWidth += iBtnWidth - s_dxOverlap;
            iMaxBtnWidth = max(iMaxBtnWidth, iBtnWidth);
        }

    }

    if (ptb->ci.style & TBSTYLE_WRAPABLE) {
        //Make sure the height is a multiple of button height
        iHeight -= (iHeight % ptb->iButHeight);
        if (iHeight < ptb->iButHeight)
            iHeight = ptb->iButHeight;

        WrapToolbar(ptb, iWidth, &rc, NULL);

        // if wrapping at full width gives us a height that's too big,
        // then there's nothing we can do because widening it still keeps us at 1 row
        if (iHeight > RECTHEIGHT(rc)) {
            int iPrevWidth;
            BOOL fDivide = TRUE; //first start by dividing for speed, then narrow it down by subtraction

            TraceMsg(TF_TOOLBAR, "Toolbar: performing expensive width calculation!");

            while (iMaxBtnWidth < iWidth) {
                iPrevWidth = iWidth;
                if (fDivide)
                    iWidth = (iWidth * 2) / 3;
                else
                    iWidth -= ptb->iButWidth;

                if (iWidth == iPrevWidth)
                    break;

                WrapToolbar(ptb, iWidth, &rc, NULL);

                if (iHeight < RECTHEIGHT(rc)) {
                    iWidth = iPrevWidth;
                    if (fDivide) {
                        // we've overstepped on dividing.  go to the previous width
                        // that was ok, and now try subtracting one button at a time
                        fDivide = FALSE;
                    } else
                        break;
                }
            };

            WrapToolbar(ptb, iWidth, &rc, NULL);
            iWidth = max(RECTWIDTH(rc), iMaxBtnWidth);
        }


        // WrapToolbar above has the side effect of actually modifying
        // the layout.  we need to restore it after doing all this calculations
        TBAutoSize(ptb);
    }

    return iWidth;
}


LRESULT TB_OnScroll(PTBSTATE ptb, LPNMHDR pnm)
{
    POINT pt, ptTemp;
    LPNMPGSCROLL pscroll = (LPNMPGSCROLL)pnm;
    int iDir = pscroll->iDir;
    RECT rcTemp, rc = pscroll->rcParent;
    int parentsize = 0;
    int scroll = pscroll->iScroll;
    int iButton = 0;
    int iButtonSize  = ptb->iButHeight;
    int y = 0;
    int iCurrentButton = 0;
   //This variable holds the number of buttons in a row
    int iButInRow = 0;

    pt.x = pscroll->iXpos;
    pt.y = pscroll->iYpos;
    ptTemp = pt;

    //We need to add the offset of the toolbar to the scroll position to get the
    //correct scroll positon in terms of toolbar window
    pt.x += ptb->xFirstButton;
    pt.y += ptb->iYPos;
    ptTemp = pt;


    if ((iDir == PGF_SCROLLUP) || (iDir == PGF_SCROLLDOWN))
    {
        //Vertical Mode
        if (ptb->iButWidth == 0 )
        {
            iButInRow = 1;
        }
        else
        {
            iButInRow = RECTWIDTH(rc) / ptb->iButWidth;
        }

    }
    else
    {
        //Horizontal Mode
        iButInRow =  1;
    }
    // if the parent height/width is less than button height/width then set the  number of
    // buttons in a row to be 1
    if (0 == iButInRow)
    {
        iButInRow = 1;
    }

    iCurrentButton = TBHitTest(ptb, pt.x + 1, pt.y + 1);

    //if the button is negative then we have hit a seperator.
    //Convert the index of the seperator into button index
    if (iCurrentButton < 0)
         iCurrentButton = -iCurrentButton - 1;

    switch ( iDir )
    {
    case PGF_SCROLLUP:
    case PGF_SCROLLLEFT:
        if(iDir == PGF_SCROLLLEFT)
        {
            FlipRect(&rc);
            FlipPoint(&pt);
            FlipPoint(&ptTemp);
            iButtonSize = ptb->iButWidth;
        }

        //Check if any button is partially visible at the left/top. if so then set the bottom
        // of that button to be our current offset and then scroll. This avoids skipping over
        // certain buttons when partial buttons are displayed at the left or top
        y = pt.y;
        TB_GetItemRect(ptb, iCurrentButton, &rcTemp);
        if(iDir == PGF_SCROLLLEFT)
        {
            FlipRect(&rcTemp);
        }

        if (rcTemp.top  <  y-1)
        {
            iCurrentButton += iButInRow;
        }

        //Now do the actual calculation

        parentsize = RECTHEIGHT(rc);

        //if  the control key is down and we have more than parentsize size of child window
        // then scroll by that amount
        if (pscroll->fwKeys & PGK_CONTROL)

        {
            if ((y - parentsize) > 0 )
            {
                scroll = parentsize;
            }
            else
            {
                scroll = y;
                return 0L;
            }

        } else  if ((y - iButtonSize) > 0 ){
        // we dont have control key down so scroll by one buttonsize
            scroll = iButtonSize;

        } else {
            scroll = pt.y;
            return 0L;
        }
        ptTemp.y -= scroll;

        if(iDir == PGF_SCROLLLEFT)
        {
            FlipPoint(&ptTemp);
        }

        iButton = TBHitTest(ptb, ptTemp.x, ptTemp.y);

        //if the button is negative then we have hit a seperator.
        //Convert the index of the seperator into button index
        if (iButton < 0)
            iButton = -iButton -1 ;

       // if  the hit test gives us the same button as our prevbutton then set the button
       // to one button to the left  of the prev button

       if ((iButton == iCurrentButton) && (iButton >= iButInRow))
       {
           iButton -= iButInRow;
           if ((ptb->Buttons[iButton].fsStyle & BTNS_SEP)  && (iButton >= iButInRow))
           {
               iButton -= iButInRow;
           }
       }
       //When scrolling left if we end up in the middle of some button then we align it to the
       //right of that button this is to avoid scrolling more than the pager window width but if the
       // button happens to be the left button of  our current button then we end up in not scrolling
       //if thats the case then move one more button to the left.


       if (iButton == iCurrentButton-iButInRow)
       {
           iButton -= iButInRow;
       }

       TB_GetItemRect(ptb, iButton, &rcTemp);
       if(iDir == PGF_SCROLLLEFT)
       {
           FlipRect(&rcTemp);
       }
       scroll = pt.y - rcTemp.bottom;
       //Set the scroll value
       pscroll->iScroll = scroll;
       break;

    case PGF_SCROLLDOWN:
    case PGF_SCROLLRIGHT:
        {
            RECT rcChild;
            int childsize;

            GetWindowRect(ptb->ci.hwnd, &rcChild);
            if( iDir == PGF_SCROLLRIGHT)
            {
                FlipRect(&rcChild);
                FlipRect(&rc);
                FlipPoint(&pt);
                FlipPoint(&ptTemp);
                iButtonSize = ptb->iButWidth;
            }

            childsize = RECTHEIGHT(rcChild);
            parentsize = RECTHEIGHT(rc);

            //if  the control key is down and we have more than parentsize size of child window
            // then scroll by that amount

            if (pscroll->fwKeys & PGK_CONTROL)
            {
                if ((childsize - pt.y - parentsize) > parentsize)
                {
                    scroll = parentsize;
                }
                else
                {
                    scroll = childsize - pt.y - parentsize;
                    return 0L;
                }

            } else if (childsize - pt.y - parentsize > iButtonSize) {
            // we dont have control key down so scroll by one buttonsize
                scroll = iButtonSize;

            } else {
                pscroll->iScroll = childsize - pt.y - parentsize;
                return 0L;
            }
            ptTemp.y += scroll;

            if(iDir == PGF_SCROLLRIGHT)
            {
                FlipPoint(&ptTemp);
            }

            iButton = TBHitTest(ptb, ptTemp.x, ptTemp.y);

            //if the button is negative then we have hit a seperator.
            //Convert the index of the seperator into button index
                if (iButton < 0)
                iButton = -iButton - 1 ;

            if ((iButton == iCurrentButton) && ((iButton + iButInRow) < ptb->iNumButtons))
            {
                iButton += iButInRow;
                if ((ptb->Buttons[iButton].fsStyle & BTNS_SEP)  && ((iButton + iButInRow) < ptb->iNumButtons))
                {
                    iButton += iButInRow;
                }
            }

            TB_GetItemRect(ptb, iButton, &rcTemp);
            if(iDir == PGF_SCROLLRIGHT)
            {
                FlipRect(&rcTemp);
            }
            scroll = rcTemp.top  - pt.y ;

            //Set the scroll value
            pscroll->iScroll = scroll;
            break;
        }
    }
    return 0L;
}

int TB_CalcHeight(PTBSTATE ptb)
{
    int iHeight = 0;
    int i;

    ASSERT(ptb->dwStyleEx & TBSTYLE_EX_VERTICAL);
    ASSERT(!(ptb->dwStyleEx & TBSTYLE_EX_MULTICOLUMN));

    for (i = 0; i < ptb->iNumButtons; i++)
    {
        if (!(ptb->Buttons[i].fsState & TBSTATE_HIDDEN))
        {
            if (ptb->Buttons[i].fsStyle & BTNS_SEP)
                iHeight += (TBGetSepHeight(ptb, &ptb->Buttons[i]));
            else
                iHeight += ptb->iButHeight;
        }
    }

    return iHeight;
}

LRESULT TB_OnCalcSize(PTBSTATE ptb, LPNMHDR pnm)
{
    LPNMPGCALCSIZE pcalcsize = (LPNMPGCALCSIZE)pnm;
    RECT rc;

    switch(pcalcsize->dwFlag)
    {
    case PGF_CALCHEIGHT:

        if (ptb->szCached.cx == pcalcsize->iWidth)
            pcalcsize->iHeight = ptb->szCached.cy;
        else
        {
            if (ptb->dwStyleEx & TBSTYLE_EX_MULTICOLUMN)
            {
                WrapToolbarCol(ptb, ptb->sizeBound.cy,  &rc, NULL);
                pcalcsize->iWidth = RECTWIDTH(rc);
                pcalcsize->iHeight = RECTHEIGHT(rc);
            }
            else if (ptb->dwStyleEx & TBSTYLE_EX_VERTICAL)
            {
                pcalcsize->iHeight = TB_CalcHeight(ptb);
            }
            else
            {
                // BUGBUG: this WrapToolbar call can modify toolbar layout ...
                // seems busted.  should perhaps call TBAutoSize after to restore.
                WrapToolbar(ptb, pcalcsize->iWidth,  &rc, NULL);
                pcalcsize->iHeight = RECTHEIGHT(rc);
            }
        }
        break;

    case PGF_CALCWIDTH:
        if (ptb->szCached.cy == pcalcsize->iHeight) {
            pcalcsize->iWidth = ptb->szCached.cx;
        } else {
            pcalcsize->iWidth = TB_CalcWidth(ptb, pcalcsize->iHeight);
        }
        break;
    }

    ptb->szCached.cx = pcalcsize->iWidth;
    ptb->szCached.cy = pcalcsize->iHeight;
    return 0L;
}

LRESULT TB_OnPagerControlNotify(PTBSTATE ptb, LPNMHDR pnm)
{
    switch(pnm->code) {
    case PGN_SCROLL:
        return TB_OnScroll(ptb, pnm);
        break;
    case PGN_CALCSIZE:
        return TB_OnCalcSize(ptb, pnm);
        break;
    }
    return 0L;
}


BOOL TBGetMaxSize( PTBSTATE ptb, LPSIZE lpsize )
{
    // need to calc the number of buttons and then the number of separators...
    int iButton;
    LPTBBUTTONDATA pAllButtons = ptb->Buttons;
    int iRealButtons = 0;
    int iSeparators = 0;

    if ( !lpsize )
        return FALSE;
    if (ptb->dwStyleEx & TBSTYLE_EX_MULTICOLUMN)
    {
        ASSERT(ptb->dwStyleEx & TBSTYLE_EX_VERTICAL);
        lpsize->cx = RECTWIDTH(ptb->rc);
        lpsize->cy = RECTHEIGHT(ptb->rc);
        return TRUE;
    }
    for (iButton = 0; iButton < ptb->iNumButtons; iButton++)
    {
        LPTBBUTTONDATA pButton = &pAllButtons[iButton];

        if (!( pButton->fsState & TBSTATE_HIDDEN ))
        {
            if ( pButton->fsStyle & BTNS_SEP )
                iSeparators ++;
            else
                iRealButtons ++;
        }
    }

    // BUGBUG: g_dxButtonSep is handy, but what if the separator style changes,
    // BUGBUG: for example we don't distinguish between flat and non-flat separators..
    if ( ptb->ci.style & CCS_VERT )
    {
        // we are vertical ...
        lpsize->cx = ptb->iButWidth;
        lpsize->cy = ptb->iButHeight * iRealButtons + g_dxButtonSep * iSeparators;
    }
    else
    {
        lpsize->cx = ptb->iButWidth * iRealButtons + g_dxButtonSep * iSeparators;
        lpsize->cy = ptb->iButHeight;
    }
    return TRUE;
}


void TBGetItem(PTBSTATE ptb, LPTBBUTTONDATA ptButton, LPNMTBDISPINFO ptbdi)
{

    ptbdi->idCommand = ptButton->idCommand;
    ptbdi->iImage  =  -1;
    ptbdi->lParam  = ptButton->dwData;


    CCSendNotify(&ptb->ci, TBN_GETDISPINFO, &(ptbdi->hdr));

    if(ptbdi->dwMask & TBNF_DI_SETITEM) {
        if(ptbdi->dwMask & TBNF_IMAGE)
            ptButton->DUMMYUNION_MEMBER(iBitmap) = ptbdi->iImage;
    }

}

BOOL TBGetInfoTip(PTBSTATE ptb, LPTOOLTIPTEXT lpttt, LPTBBUTTONDATA pTBButton)
{
    NMTBGETINFOTIP git;
    TCHAR   szBuf[INFOTIPSIZE];

    szBuf[0] = 0;
    git.pszText = szBuf;
    git.cchTextMax = ARRAYSIZE(szBuf);
    git.iItem = pTBButton->idCommand;
    git.lParam = pTBButton->dwData;

    CCSendNotify(&ptb->ci, TBN_GETINFOTIP, &git.hdr);

    if (git.pszText && git.pszText[0]) {
        // if they didn't fill anything in, go to the default stuff
        // without modifying the notify structure

        Str_Set(&ptb->pszTip, git.pszText);
        lpttt->lpszText = ptb->pszTip;
        return lpttt->lpszText && lpttt->lpszText[0];
    }

    return FALSE;
}
#ifdef UNIX
void TBSetHotItemWithoutNotification(PTBSTATE ptb, int iPos, DWORD dwReason)
{
    if ((ptb->ci.style & TBSTYLE_FLAT) ) {

        // Either one of these values can be -1, but refrain
        // from processing if both are negative b/c it is wasteful
        // and very common

        if ((ptb->iHot != iPos || (dwReason & HICF_RESELECT)) &&
            (0 <= ptb->iHot || 0 <= iPos) &&
            iPos < ptb->iNumButtons)
        {
            NMTBHOTITEM nmhot = {0};

            // Has the mouse moved away from the toolbar but
            // do we still anchor the highlight?
            if (0 > iPos && ptb->fAnchorHighlight && (dwReason & HICF_MOUSE))
                return ;        // Yes; deny the hot item change

            // Send a notification about the hot item change
            if (0 > ptb->iHot)
            {
                if (iPos >= 0)
                    nmhot.idNew = ptb->Buttons[iPos].idCommand;
                nmhot.dwFlags = HICF_ENTERING;
            }
            else if (0 > iPos)
            {
                if (ptb->iHot >= 0)
                    nmhot.idOld = ptb->Buttons[ptb->iHot].idCommand;
                nmhot.dwFlags = HICF_LEAVING;
            }
            else
            {
                nmhot.idOld = ptb->Buttons[ptb->iHot].idCommand;
                nmhot.idNew = ptb->Buttons[iPos].idCommand;
            }
            nmhot.dwFlags |= dwReason;

            //if (CCSendNotify(&ptb->ci, TBN_HOTITEMCHANGE, &nmhot.hdr))
            //    return;         // deny the hot item change

            TBInvalidateButton(ptb, ptb->iHot, TRUE);
            if ((iPos < 0) || !(ptb->Buttons[iPos].fsState & TBSTATE_ENABLED))
                iPos = -1;

            ptb->iHot = iPos;

            TBInvalidateButton(ptb, ptb->iHot, TRUE);
        }
    }
}
#endif
