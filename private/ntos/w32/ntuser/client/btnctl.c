/**************************** Module Header ********************************\
* Module Name: btnctl.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Radio Button and Check Box Handling Routines
*
* History:
* ??-???-???? ??????    Ported from Win 3.0 sources
* 01-Feb-1991 mikeke    Added Revalidation code
* 03-Jan-1992 ianja     Neutralized (ANSI/wide-character)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/* ButtonCalcRect codes */
#define CBR_CLIENTRECT 0
#define CBR_CHECKBOX   1
#define CBR_CHECKTEXT  2
#define CBR_GROUPTEXT  3
#define CBR_GROUPFRAME 4
#define CBR_PUSHBUTTON 5

CONST BYTE mpStyleCbr[] = {
    CBR_PUSHBUTTON,  /* BS_PUSHBUTTON */
    CBR_PUSHBUTTON,  /* BS_DEFPUSHBUTTON */
    CBR_CHECKTEXT,   /* BS_CHECKBOX */
    CBR_CHECKTEXT,   /* BS_AUTOCHECKBOX */
    CBR_CHECKTEXT,   /* BS_RADIOBUTTON */
    CBR_CHECKTEXT,   /* BS_3STATE */
    CBR_CHECKTEXT,   /* BS_AUTO3STATE */
    CBR_GROUPTEXT,   /* BS_GROUPBOX */
    CBR_CLIENTRECT,  /* BS_USERBUTTON */
    CBR_CHECKTEXT,   /* BS_AUTORADIOBUTTON */
    CBR_CLIENTRECT,  /* BS_PUSHBOX */
    CBR_CLIENTRECT,  /* BS_OWNERDRAW */
};

#define IMAGE_BMMAX    IMAGE_CURSOR+1
static CONST BYTE rgbType[IMAGE_BMMAX] = {
    BS_BITMAP,          // IMAGE_BITMAP
    BS_ICON,            // IMAGE_CURSOR
    BS_ICON             // IMAGE_ICON
};

#define IsValidImage(imageType, realType, max)   \
    ((imageType < max) && (rgbType[imageType] == realType))

typedef struct tagBTNDATA {
    LPWSTR  lpsz;       // Text string
    PBUTN   pbutn;      // Button data
    WORD    wFlags;     // Alignment flags
} BTNDATA, FAR * LPBTNDATA;

void xxxDrawButton(PBUTN pbutn, HDC hdc, UINT pbfPush);

LOOKASIDE ButtonLookaside;

/***************************************************************************\
*
*  IsPushButton()
*
*  Returns non-zero if the window is a push button.  Returns flags that
*  are interesting if it is.  These flags are
*
*
*
\***************************************************************************/

UINT IsPushButton(
    PWND pwnd)
{
    BYTE bStyle;
    UINT flags;

    bStyle = TestWF(pwnd, BFTYPEMASK);

    flags = 0;

    switch (bStyle) {
        case LOBYTE(BS_PUSHBUTTON):
            flags |= PBF_PUSHABLE;
            break;

        case LOBYTE(BS_DEFPUSHBUTTON):
            flags |= PBF_PUSHABLE | PBF_DEFAULT;
            break;

        default:
            if (TestWF(pwnd, BFPUSHLIKE))
                flags |= PBF_PUSHABLE;
            break;
    }

    return(flags);
}

/***************************************************************************\
*
*  GetAlignment()
*
*  Gets default alignment of button.  If BS_HORZMASK and/or BS_VERTMASK
*  is specified, uses those.  Otherwise, uses default for button.
*
* It's probably a fine time to describe what alignment flags mean for
* each type of button.  Note that the presence of a bitmap/icon affects
* the meaning of alignments.
*
* (1) Push like buttons
*      With one of {bitmap, icon, text}:
*          Just like you'd expect
*      With one of {bitmap, icon} AND text:
*          Image & text are centered as a unit; alignment means where
*          the image shows up.  E.G., left-aligned means the image
*          on the left, text on the right.
* (2) Radio/check like buttons
*      Left aligned means check/radio box is on left, then bitmap/icon
*          and text follows, left justified.
*      Right aligned means checkk/radio box is on right, preceded by
*          text and bitmap/icon, right justified.
*      Centered has no meaning.
*      With one of {bitmap, icon} AND text:
*          Top aligned means bitmap/icon above, text below
*          Bottom aligned means text above, bitmap/icon below
*      With one of {bitmap, icon, text}
*          Alignments mean what you'd expect.
* (3) Group boxes
*      Left aligned means text is left justified on left side
*      Right aligned means text is right justified on right side
*      Center aligned means text is in middle
*
*
\***************************************************************************/

WORD GetAlignment(
    PWND pwnd)
{
    BYTE bHorz;
    BYTE bVert;

    bHorz = TestWF(pwnd, BFHORZMASK);
    bVert = TestWF(pwnd, BFVERTMASK);

    if (!bHorz || !bVert) {
        if (IsPushButton(pwnd)) {
            if (!bHorz)
                bHorz = LOBYTE(BFCENTER);
        } else {
            if (!bHorz)
                bHorz = LOBYTE(BFLEFT);
        }

        if (!bVert)
            bVert = LOBYTE(BFVCENTER);
    }

    return bHorz | bVert;
}


/***************************************************************************\
*
*  BNSetFont()
*
*  Changes button font, and decides if we can use real bold font for default
*  push buttons or if we have to simulate it.
*
\***************************************************************************/

void BNSetFont(
    PBUTN pbutn,
    HFONT hfn,
    BOOL fRedraw)
{
    PWND pwnd = pbutn->spwnd;

    pbutn->hFont = hfn;

    if (fRedraw && IsVisible(pwnd)) {
        NtUserInvalidateRect(HWq(pwnd), NULL, TRUE);
    }

}


/***************************************************************************\
* xxxBNInitDC
*
* History:
\***************************************************************************/

HBRUSH xxxBNInitDC(
    PBUTN pbutn,
    HDC hdc)
{
    UINT    wColor;
    BYTE    bStyle;
    HBRUSH  hbr;
    PWND pwnd = pbutn->spwnd;

    CheckLock(pwnd);

    /*
     * Set BkMode before getting brush so that the app can change it to
     * transparent if it wants.
     */
    SetBkMode(hdc, OPAQUE);

    bStyle = TestWF(pwnd, BFTYPEMASK);

    switch (bStyle) {
        default:
            if (TestWF(pwnd, WFWIN40COMPAT) && !TestWF(pwnd, BFPUSHLIKE)) {
                wColor = WM_CTLCOLORSTATIC;
                break;
            }

        case LOBYTE(BS_PUSHBUTTON):
        case LOBYTE(BS_DEFPUSHBUTTON):
        case LOBYTE(BS_OWNERDRAW):
        case LOBYTE(BS_USERBUTTON):
            wColor = WM_CTLCOLORBTN;
            break;
    }

    hbr = GetControlBrush(HWq(pwnd), hdc, wColor);

    /*
     * Select in the user's font if set, and save the old font so that we can
     * restore it when we release the dc.
     */
    if (pbutn->hFont) {
        SelectObject(hdc, pbutn->hFont);
    }

    /*
     * Clip output to the window rect if needed.
     */
    if (bStyle != LOBYTE(BS_GROUPBOX)) {
        IntersectClipRect(hdc, 0, 0,
            pwnd->rcClient.right - pwnd->rcClient.left,
            pwnd->rcClient.bottom - pwnd->rcClient.top);
    }

    if (TestWF(pwnd,WEFRTLREADING))
        SetTextAlign(hdc, TA_RTLREADING | GetTextAlign(hdc));

    return(hbr);
}

/***************************************************************************\
* xxxBNGetDC
*
* History:
\***************************************************************************/

HDC xxxBNGetDC(
    PBUTN pbutn,
    HBRUSH *lphbr)
{
    HDC hdc;
    PWND pwnd = pbutn->spwnd;

    CheckLock(pwnd);

    if (IsVisible(pwnd)) {
        HBRUSH  hbr;

        hdc = NtUserGetDC(HWq(pwnd));
        hbr = xxxBNInitDC(pbutn, hdc);

        if (lphbr!=NULL)
            *lphbr = hbr;

        return hdc;
    }

    return NULL;
}

/***************************************************************************\
* BNReleaseDC
*
* History:
\***************************************************************************/

void BNReleaseDC(
    PBUTN pbutn,
    HDC hdc)
{
    PWND pwnd = pbutn->spwnd;

    if (TestWF(pwnd,WEFRTLREADING))
        SetTextAlign(hdc, GetTextAlign(hdc) & ~TA_RTLREADING);

    if (pbutn->hFont) {
        SelectObject(hdc, ghFontSys);
    }

    ReleaseDC(HWq(pwnd), hdc);
}

/***************************************************************************\
* xxxBNOwnerDraw
*
* History:
\***************************************************************************/

void xxxBNOwnerDraw(
    PBUTN pbutn,
    HDC hdc,
    UINT itemAction)
{
    DRAWITEMSTRUCT drawItemStruct;
    TL tlpwndParent;
    PWND pwnd = pbutn->spwnd;
    UINT itemState = 0;

    if (TestWF(pwnd, WEFPUIFOCUSHIDDEN)) {
        itemState |= ODS_NOFOCUSRECT;
    }
    if (TestWF(pwnd, WEFPUIACCELHIDDEN)) {
        itemState |= ODS_NOACCEL;
    }
    if (BUTTONSTATE(pbutn) & BST_FOCUS) {
        itemState |= ODS_FOCUS;
    }
    if (BUTTONSTATE(pbutn) & BST_PUSHED) {
        itemState |= ODS_SELECTED;
    }

    if (TestWF(pwnd, WFDISABLED))
        itemState |= ODS_DISABLED;

    drawItemStruct.CtlType = ODT_BUTTON;
    drawItemStruct.CtlID = PtrToUlong(pwnd->spmenu);
    drawItemStruct.itemAction = itemAction;
    drawItemStruct.itemState = itemState;
    drawItemStruct.hwndItem = HWq(pwnd);
    drawItemStruct.hDC = hdc;
    _GetClientRect(pwnd, &drawItemStruct.rcItem);
    drawItemStruct.itemData = 0L;

    /*
     * Send a WM_DRAWITEM message to the parent
     * IanJa:  in this case pMenu is being used as the control ID
     */
    ThreadLock(REBASEPWND(pwnd, spwndParent), &tlpwndParent);
    SendMessage(HW(REBASEPWND(pwnd, spwndParent)), WM_DRAWITEM, (WPARAM)pwnd->spmenu,
            (LPARAM)&drawItemStruct);
    ThreadUnlock(&tlpwndParent);
}

/***************************************************************************\
* CalcBtnRect
*
* History:
\***************************************************************************/

void BNCalcRect(
    PWND pwnd,
    HDC hdc,
    LPRECT lprc,
    int code,
    UINT pbfFlags)
{
    int cch;
    SIZE extent;
    int dy;
    LPWSTR lpName;
    UINT align;

    _GetClientRect(pwnd, lprc);

    align = GetAlignment(pwnd);

    switch (code) {
    case CBR_PUSHBUTTON:
        // Subtract out raised edge all around
        InflateRect(lprc, -SYSMET(CXEDGE), -SYSMET(CYEDGE));

        if (pbfFlags & PBF_DEFAULT)
            InflateRect(lprc, -SYSMET(CXBORDER), -SYSMET(CYBORDER));
        break;

    case CBR_CHECKBOX:
        switch (align & LOBYTE(BFVERTMASK))
        {
        case LOBYTE(BFVCENTER):
                lprc->top = (lprc->top + lprc->bottom - gpsi->oembmi[OBI_CHECK].cy) / 2;
                break;

            case LOBYTE(BFTOP):
            case LOBYTE(BFBOTTOM):
                PSMGetTextExtent(hdc, (LPWSTR)szOneChar, 1, &extent);
                dy = extent.cy + extent.cy/4;

                // Save vertical extent
                extent.cx = dy;

                // Get centered amount

                dy = (dy - gpsi->oembmi[OBI_CHECK].cy) / 2;
                if ((align & LOBYTE(BFVERTMASK)) == LOBYTE(BFTOP))
                    lprc->top += dy;
                else
                    lprc->top = lprc->bottom - extent.cx + dy;
                break;
        }

        if (TestWF(pwnd, BFRIGHTBUTTON))
            lprc->left = lprc->right - gpsi->oembmi[OBI_CHECK].cx;
        else
            lprc->right = lprc->left + gpsi->oembmi[OBI_CHECK].cx;

        break;

    case CBR_CHECKTEXT:
        if (TestWF(pwnd, BFRIGHTBUTTON)) {
            lprc->right -= gpsi->oembmi[OBI_CHECK].cx;

            // More spacing for 4.0 dudes
            if (TestWF(pwnd, WFWIN40COMPAT)) {
                PSMGetTextExtent(hdc, szOneChar, 1, &extent);
                lprc->right -= extent.cx  / 2;
            }
        } else {
            lprc->left += gpsi->oembmi[OBI_CHECK].cx;

            // More spacing for 4.0 dudes
            if (TestWF(pwnd, WFWIN40COMPAT)) {
                PSMGetTextExtent(hdc, szOneChar, 1, &extent);
                lprc->left +=  extent.cx / 2;
            }
        }
        break;

    case CBR_GROUPTEXT:
        if (!pwnd->strName.Length)
            goto EmptyRect;

        lpName = REBASE(pwnd, strName.Buffer);
        if (!(cch = pwnd->strName.Length / sizeof(WCHAR))) {
EmptyRect:
            SetRectEmpty(lprc);
            break;
        }

        PSMGetTextExtent(hdc, lpName, cch, &extent);
        extent.cx += SYSMET(CXEDGE) * 2;

        switch (align & LOBYTE(BFHORZMASK))
        {
        // BFLEFT, nothing
        case LOBYTE(BFLEFT):
            lprc->left += (gpsi->cxSysFontChar - SYSMET(CXBORDER));
            lprc->right = lprc->left + (int)(extent.cx);
            break;

        case LOBYTE(BFRIGHT):
            lprc->right -= (gpsi->cxSysFontChar - SYSMET(CXBORDER));
            lprc->left = lprc->right - (int)(extent.cx);
            break;

        case LOBYTE(BFCENTER):
            lprc->left = (lprc->left + lprc->right - (int)(extent.cx)) / 2;
            lprc->right = lprc->left + (int)(extent.cx);
            break;
        }

        // Center aligned.
        lprc->bottom = lprc->top + extent.cy + SYSMET(CYEDGE);
        break;

    case CBR_GROUPFRAME:
        PSMGetTextExtent(hdc, (LPWSTR)szOneChar, 1, &extent);
        lprc->top += extent.cy / 2;
        break;
    }
}

/***************************************************************************\
*
*  BtnGetMultiExtent()
*
*  Calculates button text extent, given alignment flags.
*
\***************************************************************************/

void BNMultiExtent(
    WORD wFlags,
    HDC hdc,
    LPRECT lprcMax,
    LPWSTR lpsz,
    int cch,
    PINT pcx,
    PINT pcy)
{
    RECT rcT;

    UINT dtFlags = DT_CALCRECT | DT_WORDBREAK | DT_EDITCONTROL;
    CopyRect(&rcT, lprcMax);

    // Note that since we're just calculating the maximum dimensions,
    // left-justification and top-justification are not important.
    // Also, remember to leave margins horz and vert that follow our rules
    // in DrawBtnText().

    InflateRect(&rcT, -SYSMET(CXEDGE), -SYSMET(CYBORDER));

    if ((wFlags & LOWORD(BS_HORZMASK)) == LOWORD(BS_CENTER))
        dtFlags |= DT_CENTER;

    if ((wFlags & LOWORD(BS_VERTMASK)) == LOWORD(BS_VCENTER))
        dtFlags |= DT_VCENTER;

    DrawTextExW(hdc, lpsz, cch, &rcT, dtFlags, NULL);

    if (pcx)
        *pcx = rcT.right-rcT.left;
    if (pcy)
        *pcy = rcT.bottom-rcT.top;
}

/***************************************************************************\
*
*  BtnMultiDraw()
*
*  Draws multiline button text
*
\***************************************************************************/

BOOL BNMultiDraw(
    HDC hdc,
    LPBTNDATA lpbd,
    int cch,
    int cx,
    int cy)
{
    RECT rcT;
    UINT dtFlags = DT_WORDBREAK | DT_EDITCONTROL;
    PBUTN pbutn = lpbd->pbutn;

    if (TestWF(pbutn->spwnd, WEFPUIACCELHIDDEN)) {
        dtFlags |= DT_HIDEPREFIX;
    } else if (pbutn->fPaintKbdCuesOnly){
        dtFlags |= DT_PREFIXONLY;
    }

    rcT.left    = 0;
    rcT.top     = 0;
    rcT.right   = cx;
    rcT.bottom  = cy;

    // Horizontal alignment
    UserAssert(DT_LEFT == 0);
    switch (lpbd->wFlags & LOWORD(BS_HORZMASK)) {
        case LOWORD(BS_CENTER):
            dtFlags |= DT_CENTER;
            break;

        case LOWORD(BS_RIGHT):
            dtFlags |= DT_RIGHT;
            break;
    }

    // Vertical alignment
    UserAssert(DT_TOP == 0);
    switch (lpbd->wFlags & LOWORD(BS_VERTMASK)) {
        case LOWORD(BS_VCENTER):
            dtFlags |= DT_VCENTER;
            break;

        case LOWORD(BS_BOTTOM):
            dtFlags |= DT_BOTTOM;
            break;
    }

    DrawTextExW(hdc, lpbd->lpsz, cch, &rcT, dtFlags, NULL);
    return(TRUE);
}

/***************************************************************************\
* xxxBNSetCapture
*
* History:
\***************************************************************************/

BOOL xxxBNSetCapture(
    PBUTN pbutn,
    UINT codeMouse)
{
    PWND pwnd = pbutn->spwnd;

    BUTTONSTATE(pbutn) |= codeMouse;

    CheckLock(pwnd);

    if (!(BUTTONSTATE(pbutn) & BST_CAPTURED)) {
        NtUserSetCapture(HWq(pwnd));
        BUTTONSTATE(pbutn) |= BST_CAPTURED;

        /*
         * To prevent redundant CLICK messages, we set the INCLICK bit so
         * the WM_SETFOCUS code will not do a xxxButtonNotifyParent(BN_CLICKED).
         */

        BUTTONSTATE(pbutn) |= BST_INCLICK;

        NtUserSetFocus(HWq(pwnd));

        BUTTONSTATE(pbutn) &= ~BST_INCLICK;
    }
    return(BUTTONSTATE(pbutn) & BST_CAPTURED);
}


/***************************************************************************\
* xxxButtonNotifyParent
*
* History:
\***************************************************************************/

void xxxButtonNotifyParent(
    PWND pwnd,
    UINT code)
{
    TL tlpwndParent;
    PWND pwndParent;            // Parent if it exists

    CheckLock(pwnd);

    if (pwnd->spwndParent)
        pwndParent = REBASEPWND(pwnd, spwndParent);
    else
        pwndParent = pwnd;

    /*
     * Note: A button's pwnd->spmenu is used to store the control ID
     */
    ThreadLock(pwndParent, &tlpwndParent);
    SendMessage(HW(pwndParent), WM_COMMAND,
            MAKELONG(PTR_TO_ID(pwnd->spmenu), code), (LPARAM)HWq(pwnd));
    ThreadUnlock(&tlpwndParent);
}

/***************************************************************************\
* xxxBNReleaseCapture
*
* History:
\***************************************************************************/

void xxxBNReleaseCapture(
    PBUTN pbutn,
    BOOL fCheck)
{
    PWND pwndT;
    UINT check;
    BOOL fNotifyParent = FALSE;
    TL tlpwndT;
    PWND pwnd = pbutn->spwnd;

    CheckLock(pwnd);

    if (BUTTONSTATE(pbutn) & BST_PUSHED) {
        SendMessageWorker(pwnd, BM_SETSTATE, FALSE, 0, FALSE);
        if (fCheck) {
            switch (TestWF(pwnd, BFTYPEMASK)) {
            case BS_AUTOCHECKBOX:
            case BS_AUTO3STATE:
                check = (UINT)((BUTTONSTATE(pbutn) & BST_CHECKMASK) + 1);

                if (check > (UINT)(TestWF(pwnd, BFTYPEMASK) == BS_AUTO3STATE? BST_INDETERMINATE : BST_CHECKED)) {
                    check = BST_UNCHECKED;
                }
                SendMessageWorker(pwnd, BM_SETCHECK, check, 0, FALSE);
                break;

            case BS_AUTORADIOBUTTON:
                pwndT = pwnd;
                do {
                    ThreadLock(pwndT, &tlpwndT);

                    if ((UINT)SendMessage(HW(pwndT), WM_GETDLGCODE, 0, 0L) &
                            DLGC_RADIOBUTTON) {
                        SendMessage(HW(pwndT), BM_SETCHECK, (pwnd == pwndT), 0L);
                    }
                    pwndT = _GetNextDlgGroupItem(REBASEPWND(pwndT, spwndParent),
                            pwndT, FALSE);
                    ThreadUnlock(&tlpwndT);

                } while (pwndT != pwnd);
            }

            fNotifyParent = TRUE;
        }
    }

    if (BUTTONSTATE(pbutn) & BST_CAPTURED) {
        BUTTONSTATE(pbutn) &= ~(BST_CAPTURED | BST_MOUSE);
        NtUserReleaseCapture();
    }

    if (fNotifyParent) {

        /*
         * We have to do the notification after setting the buttonstate bits.
         */
        xxxButtonNotifyParent(pwnd, BN_CLICKED);
    }
}

/***************************************************************************\
*
*  DrawBtnText()
*
*  Draws text of button.
*
\***************************************************************************/

void xxxBNDrawText(
    PBUTN pbutn,
    HDC hdc,
    BOOL dbt,
    BOOL fDepress)
{
    RECT    rc;
    HBRUSH  hbr;
    int     x;
    int     y;
    int     cx;
    int     cy;
    LPWSTR   lpName;
    BYTE    bStyle;
    int     cch;
    UINT    dsFlags;
    BTNDATA bdt;
    UINT    pbfPush;
    PWND    pwnd = pbutn->spwnd;

    bStyle = TestWF(pwnd, BFTYPEMASK);

    if (bStyle > sizeof(mpStyleCbr)) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid button style");
    } else if ((bStyle == LOBYTE(BS_GROUPBOX)) && (dbt == DBT_FOCUS))
        return;

    pbfPush = IsPushButton(pwnd);
    if (pbfPush) {
        BNCalcRect(pwnd, hdc, &rc, CBR_PUSHBUTTON, pbfPush);
        IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);

        //
        // This is because we were too stupid to have WM_CTLCOLOR,
        // CTLCOLOR_BTN actually set up the damned button colors.  For
        // old apps, CTLCOLOR_BTN needs to work like CTLCOLOR_STATIC.
        //
        SetBkColor(hdc, SYSRGB(3DFACE));
        SetTextColor(hdc, SYSRGB(BTNTEXT));
        hbr = SYSHBR(BTNTEXT);
    } else {
        BNCalcRect(pwnd, hdc, &rc, mpStyleCbr[bStyle], pbfPush);

        // Skip stuff for ownerdraw buttons, since we aren't going to
        // draw text/image.
        if (bStyle == LOBYTE(BS_OWNERDRAW))
            goto DrawFocus;
        else
            hbr = SYSHBR(WINDOWTEXT);
    }

    // Alignment
    bdt.wFlags = GetAlignment(pwnd);
    bdt.pbutn = pbutn;

    // Bail if we have nothing to draw
    if (TestWF(pwnd, BFBITMAP)) {
        BITMAP bmp;

        // Bitmap button
        if (!pbutn->hImage)
            return;

        GetObject(pbutn->hImage, sizeof(BITMAP), &bmp);
        cx = bmp.bmWidth;
        cy = bmp.bmHeight;

        dsFlags = DST_BITMAP;
        goto UseImageForName;
    } else if (TestWF(pwnd, BFICON)) {
        // Icon button
        if (!pbutn->hImage)
            return;

        NtUserGetIconSize(pbutn->hImage, 0, &cx, &cy);
        cy /= 2;    // The bitmap height is half because a Mask is present in NT

        dsFlags = DST_ICON;
UseImageForName:
        lpName = (LPWSTR)pbutn->hImage;
        cch = TRUE;
    } else {
        // Text button
        if (!pwnd->strName.Length)
            return;

        lpName = REBASE(pwnd, strName.Buffer);
        cch    = pwnd->strName.Length / sizeof(WCHAR);

        if (TestWF(pwnd, BFMULTILINE)) {

            bdt.lpsz = lpName;

            BNMultiExtent(bdt.wFlags, hdc, &rc, lpName, cch, &cx, &cy);

            lpName = (LPWSTR)(LPBTNDATA)&bdt;
            dsFlags = DST_COMPLEX;

        } else {
            SIZE size;

            PSMGetTextExtent(hdc, lpName, cch, &size);
            cx = size.cx;
            cy = size.cy;
            /*
             * If the control doesn't need underlines, set DST_HIDEPREFIX and
             * also do not show the focus indicator
             */
            dsFlags = DST_PREFIXTEXT;
            if (TestWF(pwnd, WEFPUIACCELHIDDEN)) {
                dsFlags |= DSS_HIDEPREFIX;
            } else if (pbutn->fPaintKbdCuesOnly) {
                dsFlags |= DSS_PREFIXONLY;
            }
        }


        //
        // Add on a pixel or two of vertical space to make centering
        // happier.  That way underline won't abut focus rect unless
        // spacing is really tight.
        //
        cy++;
    }

    //
    // ALIGNMENT
    //

    // Horizontal
    switch (bdt.wFlags & LOBYTE(BFHORZMASK)) {
        //
        // For left & right justified, we leave a margin of CXEDGE on either
        // side for eye-pleasing space.
        //
        case LOBYTE(BFLEFT):
            x = rc.left + SYSMET(CXEDGE);
            break;

        case LOBYTE(BFRIGHT):
            x = rc.right - cx - SYSMET(CXEDGE);
            break;

        default:
            x = (rc.left + rc.right - cx) / 2;
            break;
    }

    // Vertical
    switch (bdt.wFlags & LOBYTE(BFVERTMASK)) {
        //
        // For top & bottom justified, we leave a margin of CYBORDER on
        // either side for more eye-pleasing space.
        //
        case LOBYTE(BFTOP):
            y = rc.top + SYSMET(CYBORDER);
            break;

        case LOBYTE(BFBOTTOM):
            y = rc.bottom - cy - SYSMET(CYBORDER);
            break;

        default:
            y = (rc.top + rc.bottom - cy) / 2;
            break;
    }

    //
    // Draw the text
    //
    if (dbt & DBT_TEXT) {
        //
        // This isn't called for USER buttons.
        //
        UserAssert(bStyle != LOBYTE(BS_USERBUTTON));

        if (fDepress) {
            x += SYSMET(CXBORDER);
            y += SYSMET(CYBORDER);
        }

        if (TestWF(pwnd, WFDISABLED)) {
            UserAssert(HIBYTE(BFICON) == HIBYTE(BFBITMAP));
            if (SYSMET(SLOWMACHINE)  &&
                !TestWF(pwnd, BFICON | BFBITMAP) &&
                (GetBkColor(hdc) != SYSRGB(GRAYTEXT)))
            {
                // Perf && consistency with menus, statics
                SetTextColor(hdc, SYSRGB(GRAYTEXT));
            }
            else
                dsFlags |= DSS_DISABLED;
        }

        //
        // Use transparent mode for checked push buttons since we're going to
        // fill background with dither.
        //
        if (pbfPush) {
            switch (BUTTONSTATE(pbutn) & BST_CHECKMASK) {
                case BST_INDETERMINATE:
                    hbr = SYSHBR(GRAYTEXT);
                    dsFlags |= DSS_MONO;
                    // FALL THRU

                case BST_CHECKED:
                    // Drawing on dithered background...
                    SetBkMode(hdc, TRANSPARENT);
                    break;
            }
        }

        //
        // Use brush and colors currently selected into hdc when we grabbed
        // color
        //
        DrawState(hdc, hbr, (DRAWSTATEPROC)BNMultiDraw, (LPARAM)lpName,
            (WPARAM)cch, x, y, cx, cy,
            dsFlags);
    }

    // Draw focus rect.
    //
    // This can get called for OWNERDRAW and USERDRAW buttons. However, only
    // OWNERDRAW buttons let the owner change the drawing of the focus button.
DrawFocus:
    if (dbt & DBT_FOCUS) {
        if (bStyle == LOBYTE(BS_OWNERDRAW)) {
            // For ownerdraw buttons, this is only called in response to a
            // WM_SETFOCUS or WM_KILL FOCUS message.  So, we can check the
            // new state of the focus by looking at the BUTTONSTATE bits
            // which are set before this procedure is called.
            xxxBNOwnerDraw(pbutn, hdc, ODA_FOCUS);
        } else {
            // Don't draw the focus if underlines are not turned on
            if (!TestWF(pwnd, WEFPUIFOCUSHIDDEN)) {

                // Let focus rect always hug edge of push buttons.  We already
                // have the client area setup for push buttons, so we don't have
                // to do anything.
                if (!pbfPush) {

                    RECT rcClient;

                    _GetClientRect(pwnd, &rcClient);
                    if (bStyle == LOBYTE(BS_USERBUTTON))
                        CopyRect(&rc, &rcClient);
                    else {
                        // Try to leave a border all around text.  That causes
                        // focus to hug text.
                        rc.top = max(rcClient.top, y-SYSMET(CYBORDER));
                        rc.bottom = min(rcClient.bottom, rc.top + SYSMET(CYEDGE) + cy);

                        rc.left = max(rcClient.left, x-SYSMET(CXBORDER));
                        rc.right = min(rcClient.right, rc.left + SYSMET(CXEDGE) + cx);
                    }
                } else
                    InflateRect(&rc, -SYSMET(CXBORDER), -SYSMET(CYBORDER));

                // Are back & fore colors set properly?
                DrawFocusRect(hdc, &rc);
            }
        }
    }
}


/***************************************************************************\
*
*  DrawCheck()
*
\***************************************************************************/

void xxxButtonDrawCheck(
    PBUTN pbutn,
    HDC hdc,
    HBRUSH hbr)
{
    RECT rc;
    int bm;
    UINT flags;
    BOOL fDoubleBlt = FALSE;
    TL tlpwnd;
    PWND pwnd = pbutn->spwnd;
    PWND pwndParent;

    BNCalcRect(pwnd, hdc, &rc, CBR_CHECKBOX, 0);

    flags = 0;
    if (BUTTONSTATE(pbutn) & BST_CHECKMASK)
        flags |= DFCS_CHECKED;
    if (BUTTONSTATE(pbutn) & BST_PUSHED)
        flags |= DFCS_PUSHED;
    if (TestWF(pwnd, WFDISABLED))
        flags |= DFCS_INACTIVE;

    bm = OBI_CHECK;
    switch (TestWF(pwnd, BFTYPEMASK)) {
        case BS_AUTORADIOBUTTON:
        case BS_RADIOBUTTON:
            fDoubleBlt = TRUE;
            bm = OBI_RADIO;
            flags |= DFCS_BUTTONRADIO;
            break;

        case BS_3STATE:
        case BS_AUTO3STATE:
            if ((BUTTONSTATE(pbutn) & BST_CHECKMASK) == BST_INDETERMINATE) {
                bm = OBI_3STATE;
                flags |= DFCS_BUTTON3STATE;
                break;
            }
            // FALL THRU

        default:
            flags |= DFCS_BUTTONCHECK;
            break;
    }

    rc.right = rc.left + gpsi->oembmi[bm].cx;
    rc.bottom = rc.top + gpsi->oembmi[bm].cy;

    ThreadLockAlways(pwnd->spwndParent, &tlpwnd);
    pwndParent = REBASEPWND(pwnd, spwndParent);
    PaintRect(HW(pwndParent), HWq(pwnd), hdc, hbr, &rc);
    ThreadUnlock(&tlpwnd);

    if (TestWF(pwnd, BFFLAT) && gpsi->BitCount != 1) {
        flags |= DFCS_MONO | DFCS_FLAT;
        DrawFrameControl(hdc, &rc, DFC_BUTTON, flags);
    } else {

        switch (flags & (DFCS_CHECKED | DFCS_PUSHED | DFCS_INACTIVE))
        {
        case 0:
            break;

        case DFCS_CHECKED:
            bm += DOBI_CHECK;
            break;

        // These are mutually exclusive!
        case DFCS_PUSHED:
        case DFCS_INACTIVE:
            bm += DOBI_DOWN;        // DOBI_DOWN == DOBI_INACTIVE
            break;

        case DFCS_CHECKED | DFCS_PUSHED:
            bm += DOBI_CHECKDOWN;
            break;

        case DFCS_CHECKED | DFCS_INACTIVE:
            bm += DOBI_CHECKDOWN + 1;
            break;
        }

        if (fDoubleBlt) {
            // This is a diamond-shaped radio button -- Blt with a mask so that
            // the exterior keeps the same color as the window's background
            DWORD clrTextSave = SetTextColor(hdc, 0x00000000L);
            DWORD clrBkSave   = SetBkColor(hdc, 0x00FFFFFFL);
            POEMBITMAPINFO pOem = gpsi->oembmi + OBI_RADIOMASK;

            NtUserBitBltSysBmp(hdc, rc.left, rc.top, pOem->cx, pOem->cy,
                    pOem->x, pOem->y, SRCAND);

            pOem = gpsi->oembmi + bm;
            NtUserBitBltSysBmp(hdc, rc.left, rc.top, pOem->cx, pOem->cy,
                    pOem->x, pOem->y, SRCINVERT);

            SetTextColor(hdc, clrTextSave);
            SetBkColor(hdc, clrBkSave);
        } else {
            POEMBITMAPINFO pOem = gpsi->oembmi + bm;
            DWORD dwROP = 0;
#ifdef USE_MIRRORING
            // We do not want to mirror the check box.
            if (MIRRORED_HDC(hdc)) {
                dwROP = NOMIRRORBITMAP;
            }
#endif
            NtUserBitBltSysBmp(hdc, rc.left, rc.top, pOem->cx, pOem->cy, 
                    pOem->x, pOem->y, SRCCOPY | dwROP);
        }
    }
}


/***************************************************************************\
* xxxButtonDrawNewState
*
* History:
\***************************************************************************/

void xxxButtonDrawNewState(
    PBUTN pbutn,
    HDC hdc,
    HBRUSH hbr,
    UINT sOld)
{
    PWND pwnd = pbutn->spwnd;

    CheckLock(pwnd);

    if (sOld != (UINT)(BUTTONSTATE(pbutn) & BST_PUSHED)) {
        UINT    pbfPush;

        pbfPush = IsPushButton(pwnd);

        switch (TestWF(pwnd, BFTYPEMASK)) {
        case BS_GROUPBOX:
        case BS_OWNERDRAW:
            break;

        default:
            if (!pbfPush) {
                xxxButtonDrawCheck(pbutn, hdc, hbr);
                break;
            }

        case BS_PUSHBUTTON:
        case BS_DEFPUSHBUTTON:
        case BS_PUSHBOX:
            xxxDrawButton(pbutn, hdc, pbfPush);
            break;
        }
    }
}

/***************************************************************************\
*
*  DrawButton()
*
*  Draws push-like button with text
*
\***************************************************************************/

void xxxDrawButton(
    PBUTN pbutn,
    HDC hdc,
    UINT pbfPush)
{
    RECT rc;
    UINT flags = 0;
    UINT state = 0;
    PWND pwnd = pbutn->spwnd;

    if (BUTTONSTATE(pbutn) & BST_PUSHED)
        state |= DFCS_PUSHED;

    if (!pbutn->fPaintKbdCuesOnly) {
        if (BUTTONSTATE(pbutn) & BST_CHECKMASK)
            state |= DFCS_CHECKED;

        if (TestWF(pwnd, WFWIN40COMPAT))
            flags = BF_SOFT;

        if (TestWF(pwnd, BFFLAT))
            flags |= BF_FLAT | BF_MONO;

        _GetClientRect(pwnd, &rc);

        if (pbfPush & PBF_DEFAULT) {
            DrawFrame(hdc, &rc, 1, DF_WINDOWFRAME);
            InflateRect(&rc, -SYSMET(CXBORDER), -SYSMET(CYBORDER));

            if (state & DFCS_PUSHED)
                flags |= BF_FLAT;
        }

        DrawPushButton(hdc, &rc, state, flags);
    }

    xxxBNDrawText(pbutn, hdc, DBT_TEXT | (BUTTONSTATE(pbutn) &
           BST_FOCUS ? DBT_FOCUS : 0), (state & DFCS_PUSHED));
}


/***************************************************************************\
* xxxBNPaint
*
* History:
\***************************************************************************/

void xxxBNPaint(
    PBUTN pbutn,
    HDC hdc)
{
    UINT bsWnd;
    RECT rc;
    HBRUSH  hbr;
    HBRUSH hbrBtnSave;
    TL tlpwndParent;
    UINT pbfPush;
    PWND pwnd = pbutn->spwnd;
    PWND pwndParent;

    CheckLock(pwnd);

    hbr = xxxBNInitDC(pbutn, hdc);

    bsWnd = TestWF(pwnd, BFTYPEMASK);
    pbfPush = IsPushButton(pwnd);
    if (!pbfPush && !pbutn->fPaintKbdCuesOnly) {
        _GetClientRect(pwnd, &rc);

        if ((bsWnd != LOBYTE(BS_OWNERDRAW)) &&
            (bsWnd != LOBYTE(BS_GROUPBOX))) {
             ThreadLock(pwnd->spwndParent, &tlpwndParent);
             pwndParent = REBASEPWND(pwnd, spwndParent);
             PaintRect(HW(pwndParent), HWq(pwnd), hdc, hbr, &rc);
             ThreadUnlock(&tlpwndParent);
        }

        hbrBtnSave = SelectObject(hdc, hbr);
    }

    switch (bsWnd) {
    case BS_CHECKBOX:
    case BS_RADIOBUTTON:
    case BS_AUTORADIOBUTTON:
    case BS_3STATE:
    case BS_AUTOCHECKBOX:
    case BS_AUTO3STATE:
        if (!pbfPush) {
            xxxBNDrawText(pbutn, hdc,
                DBT_TEXT | (BUTTONSTATE(pbutn) & BST_FOCUS ? DBT_FOCUS : 0), FALSE);
            if (!pbutn->fPaintKbdCuesOnly) {
                xxxButtonDrawCheck(pbutn, hdc, hbr);
            }
            break;
        }
        /*
         * Fall through for PUSHLIKE buttons
         */

    case BS_PUSHBUTTON:
    case BS_DEFPUSHBUTTON:
        xxxDrawButton(pbutn, hdc, pbfPush);
        break;

    case BS_PUSHBOX:
        xxxBNDrawText(pbutn, hdc,
            DBT_TEXT | (BUTTONSTATE(pbutn) & BST_FOCUS ? DBT_FOCUS : 0), FALSE);

        xxxButtonDrawNewState(pbutn, hdc, hbr, 0);
        break;

    case BS_USERBUTTON:
        xxxButtonNotifyParent(pwnd, BN_PAINT);

        if (BUTTONSTATE(pbutn) & BST_PUSHED) {
            xxxButtonNotifyParent(pwnd, BN_PUSHED);
        }
        if (TestWF(pwnd, WFDISABLED)) {
            xxxButtonNotifyParent(pwnd, BN_DISABLE);
        }
        if (BUTTONSTATE(pbutn) & BST_FOCUS) {
            xxxBNDrawText(pbutn, hdc, DBT_FOCUS, FALSE);
        }
        break;

    case BS_OWNERDRAW:
        xxxBNOwnerDraw(pbutn, hdc, ODA_DRAWENTIRE);
        break;

    case BS_GROUPBOX:
        if (!pbutn->fPaintKbdCuesOnly) {
            BNCalcRect(pwnd, hdc, &rc, CBR_GROUPFRAME, 0);
            DrawEdge(hdc, &rc, EDGE_ETCHED, BF_RECT |
                (TestWF(pwnd, BFFLAT) ? BF_FLAT | BF_MONO : 0));

            BNCalcRect(pwnd, hdc, &rc, CBR_GROUPTEXT, 0);
            ThreadLock(pwnd->spwndParent, &tlpwndParent);
            pwndParent = REBASEPWND(pwnd, spwndParent);
            PaintRect(HW(pwndParent), HWq(pwnd), hdc, hbr, &rc);
            ThreadUnlock(&tlpwndParent);
        }

        /*
         * FillRect(hdc, &rc, hbrBtn);
         */
        xxxBNDrawText(pbutn, hdc, DBT_TEXT, FALSE);
        break;
    }

    if (!pbfPush)
        SelectObject(hdc, hbrBtnSave);

    /*
     * Release the font which may have been loaded by xxxButtonInitDC.
     */
    if (pbutn->hFont) {
        SelectObject(hdc, ghFontSys);
    }
}
/***************************************************************************\
* RepaintButton
*
\***************************************************************************/
void RepaintButton (PBUTN pbutn)
{
    HDC hdc = xxxBNGetDC(pbutn, NULL);
    if (hdc != NULL) {
        xxxBNPaint(pbutn, hdc);
        BNReleaseDC(pbutn, hdc);
    }
}
/***************************************************************************\
* ButtonWndProc
*
* WndProc for buttons, check boxes, etc.
*
* History:
\***************************************************************************/

LRESULT APIENTRY ButtonWndProcWorker(
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    DWORD fAnsi)
{
    HWND hwnd = HWq(pwnd);
    UINT bsWnd;
    UINT wOldState;
    RECT rc;
    POINT pt;
    HDC hdc;
    HBRUSH      hbr;
    PAINTSTRUCT ps;
    TL tlpwndParent;
    PBUTN pbutn;
    PWND pwndParent;
    static BOOL fInit = TRUE;
    LONG lResult;

    CheckLock(pwnd);

    bsWnd = TestWF(pwnd, BFTYPEMASK);

    VALIDATECLASSANDSIZE(pwnd, FNID_BUTTON);
    INITCONTROLLOOKASIDE(&ButtonLookaside, BUTN, spwnd, 8);

    /*
     * Get the pbutn for the given window now since we will use it a lot in
     * various handlers. This was stored using SetWindowLong(hwnd,0,pbutn) when
     * we initially created the button control.
     */
    pbutn = ((PBUTNWND)pwnd)->pbutn;

    switch (message) {
    case WM_NCHITTEST:
        if (bsWnd == LOBYTE(BS_GROUPBOX)) {
            return (LONG)HTTRANSPARENT;
        } else {
            goto CallDWP;
        }

    case WM_ERASEBKGND:
        if (bsWnd == LOBYTE(BS_OWNERDRAW)) {

            /*
             * Handle erase background for owner draw buttons.
             */
            _GetClientRect(pwnd, &rc);
            ThreadLock(pwnd->spwndParent, &tlpwndParent);
            pwndParent = REBASEPWND(pwnd, spwndParent);
            PaintRect(HW(pwndParent), hwnd, (HDC)wParam, (HBRUSH)CTLCOLOR_BTN, &rc);
            ThreadUnlock(&tlpwndParent);
        }

        /*
         * Do nothing for other buttons, but don't let DefWndProc() do it
         * either.  It will be erased in xxxBNPaint().
         */
        return (LONG)TRUE;

    case WM_PRINTCLIENT:
        xxxBNPaint(pbutn, (HDC)wParam);
        break;

    case WM_PAINT:

        /*
         * If wParam != NULL, then this is a subclassed paint.
         */
        if ((hdc = (HDC)wParam) == NULL)
            hdc = NtUserBeginPaint(hwnd, &ps);

        if (IsVisible(pwnd))
            xxxBNPaint(pbutn, hdc);

        if (!wParam)
            NtUserEndPaint(hwnd, &ps);
        break;

    case WM_SETFOCUS:
        BUTTONSTATE(pbutn) |= BST_FOCUS;
        if ((hdc = xxxBNGetDC(pbutn, NULL)) != NULL) {
            xxxBNDrawText(pbutn, hdc, DBT_FOCUS, FALSE);

            BNReleaseDC(pbutn, hdc);
        }

        if (TestWF(pwnd, BFNOTIFY))
            xxxButtonNotifyParent(pwnd, BN_SETFOCUS);

        if (!(BUTTONSTATE(pbutn) & BST_INCLICK)) {
            switch (bsWnd) {
            case LOBYTE(BS_RADIOBUTTON):
            case LOBYTE(BS_AUTORADIOBUTTON):
                if (!(BUTTONSTATE(pbutn) & BST_DONTCLICK)) {
                    if (!(BUTTONSTATE(pbutn) & BST_CHECKMASK)) {
                        xxxButtonNotifyParent(pwnd, BN_CLICKED);
                    }
                }
                break;
            }
        }
        break;

    case WM_GETDLGCODE:
        switch (bsWnd) {
        case LOBYTE(BS_DEFPUSHBUTTON):
            wParam = DLGC_DEFPUSHBUTTON;
            break;

        case LOBYTE(BS_PUSHBUTTON):
        case LOBYTE(BS_PUSHBOX):
            wParam = DLGC_UNDEFPUSHBUTTON;
            break;

        case LOBYTE(BS_AUTORADIOBUTTON):
        case LOBYTE(BS_RADIOBUTTON):
            wParam = DLGC_RADIOBUTTON;
            break;

        case LOBYTE(BS_GROUPBOX):
            return (LONG)DLGC_STATIC;

        case LOBYTE(BS_CHECKBOX):
        case LOBYTE(BS_AUTOCHECKBOX):

            /*
             * If this is a char that is a '=/+', or '-', we want it
             */
            if (lParam && ((LPMSG)lParam)->message == WM_CHAR) {
                switch (wParam) {
                case TEXT('='):
                case TEXT('+'):
                case TEXT('-'):
                    wParam = DLGC_WANTCHARS;
                    break;

                default:
                    wParam = 0;
                }
            } else {
                wParam = 0;
            }
            break;

        default:
            wParam = 0;
        }
        return (LONG)(wParam | DLGC_BUTTON);

    case WM_CAPTURECHANGED:
        if (BUTTONSTATE(pbutn) & BST_CAPTURED) {
            // Unwittingly, we've been kicked out of capture,
            // so undepress etc.
            if (BUTTONSTATE(pbutn) & BST_MOUSE)
                SendMessageWorker(pwnd, BM_SETSTATE, FALSE, 0, FALSE);
            BUTTONSTATE(pbutn) &= ~(BST_CAPTURED | BST_MOUSE);
        }
        break;

    case WM_KILLFOCUS:

        /*
         * If we are losing the focus and we are in "capture mode", click
         * the button.  This allows tab and space keys to overlap for
         * fast toggle of a series of buttons.
         */
        if (BUTTONSTATE(pbutn) & BST_MOUSE) {

            /*
             * If for some reason we are killing the focus, and we have the
             * mouse captured, don't notify the parent we got clicked.  This
             * breaks Omnis Quartz otherwise.
             */
            SendMessageWorker(pwnd, BM_SETSTATE, FALSE, 0, FALSE);
        }

        xxxBNReleaseCapture(pbutn, TRUE);

        BUTTONSTATE(pbutn) &= ~BST_FOCUS;
        if ((hdc = xxxBNGetDC(pbutn, NULL)) != NULL) {
            xxxBNDrawText(pbutn, hdc, DBT_FOCUS, FALSE);

            BNReleaseDC(pbutn, hdc);
        }

        if (TestWF(pwnd, BFNOTIFY))
            xxxButtonNotifyParent(pwnd, BN_KILLFOCUS);

        /*
         * Since the bold border around the defpushbutton is done by
         * someone else, we need to invalidate the rect so that the
         * focus rect is repainted properly.
         */
        NtUserInvalidateRect(hwnd, NULL, FALSE);
        break;

    case WM_LBUTTONDBLCLK:

        /*
         * Double click messages are recognized for BS_RADIOBUTTON,
         * BS_USERBUTTON, and BS_OWNERDRAW styles.  For all other buttons,
         * double click is handled like a normal button down.
         */
        switch (bsWnd) {
        default:
            if (!TestWF(pwnd, BFNOTIFY))
                goto btnclick;

        case LOBYTE(BS_USERBUTTON):
        case LOBYTE(BS_RADIOBUTTON):
        case LOBYTE(BS_OWNERDRAW):
            xxxButtonNotifyParent(pwnd, BN_DOUBLECLICKED);
            break;
        }
        break;

    case WM_LBUTTONUP:
        if (BUTTONSTATE(pbutn) & BST_MOUSE) {
            xxxBNReleaseCapture(pbutn, TRUE);
        }
        break;

    case WM_MOUSEMOVE:
        if (!(BUTTONSTATE(pbutn) & BST_MOUSE)) {
            break;
        }

        /*
         *** FALL THRU **
         */
    case WM_LBUTTONDOWN:
btnclick:
        if (xxxBNSetCapture(pbutn, BST_MOUSE)) {
            _GetClientRect(pwnd, &rc);
            POINTSTOPOINT(pt, lParam);
            SendMessageWorker(pwnd, BM_SETSTATE, PtInRect(&rc, pt), 0, FALSE);
        }
        break;

    case WM_CHAR:
        if (BUTTONSTATE(pbutn) & BST_MOUSE)
            goto CallDWP;

        if (bsWnd != LOBYTE(BS_CHECKBOX) &&
            bsWnd != LOBYTE(BS_AUTOCHECKBOX))
            goto CallDWP;

        switch (wParam) {
        case TEXT('+'):
        case TEXT('='):
            wParam = 1;    // we must Set the check mark on.
            goto   SetCheck;

        case TEXT('-'):
            wParam = 0;    // Set the check mark off.
SetCheck:
            // Must notify only if the check status changes
            if ((WORD)(BUTTONSTATE(pbutn) & BST_CHECKMASK) != (WORD)wParam)
            {
                // We must check/uncheck only if it is AUTO
                if (bsWnd == LOBYTE(BS_AUTOCHECKBOX))
                {
                    if (xxxBNSetCapture(pbutn, 0))
                    {
                        SendMessageWorker(pwnd, BM_SETCHECK, wParam, 0, FALSE);

                        xxxBNReleaseCapture(pbutn, TRUE);
                    }
                }

                xxxButtonNotifyParent(pwnd, BN_CLICKED);
            }
            break;

        default:
            goto CallDWP;
        }
        break;

    case BM_CLICK:
        // Don't recurse into this code!
        if (BUTTONSTATE(pbutn) & BST_INBMCLICK)
            break;

        BUTTONSTATE(pbutn) |= BST_INBMCLICK;
        SendMessageWorker(pwnd, WM_LBUTTONDOWN, 0, 0, FALSE);
        SendMessageWorker(pwnd, WM_LBUTTONUP, 0, 0, FALSE);
        BUTTONSTATE(pbutn) &= ~BST_INBMCLICK;

        /*
         *** FALL THRU **
         */

    case WM_KEYDOWN:
        if (BUTTONSTATE(pbutn) & BST_MOUSE)
            break;

        if (wParam == VK_SPACE) {
            if (xxxBNSetCapture(pbutn, 0)) {
                SendMessageWorker(pwnd, BM_SETSTATE, TRUE, 0, FALSE);
            }
        } else {
            xxxBNReleaseCapture(pbutn, FALSE);
        }
        break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (BUTTONSTATE(pbutn) & BST_MOUSE) {
            goto CallDWP;
        }

        /*
         * Don't cancel the capture mode on the up of the tab in case the
         * guy is overlapping tab and space keys.
         */
        if (wParam == VK_TAB) {
            goto CallDWP;
        }

        /*
         * WARNING: pwnd is history after this call!
         */
        xxxBNReleaseCapture(pbutn, (wParam == VK_SPACE));

        if (message == WM_SYSKEYUP) {
            goto CallDWP;
        }
        break;

    case BM_GETSTATE:
        return (LONG)BUTTONSTATE(pbutn);

    case BM_SETSTATE:
        wOldState = (UINT)(BUTTONSTATE(pbutn) & BST_PUSHED);
        if (wParam) {
            BUTTONSTATE(pbutn) |= BST_PUSHED;
        } else {
            BUTTONSTATE(pbutn) &= ~BST_PUSHED;
        }

        if ((hdc = xxxBNGetDC(pbutn, &hbr)) != NULL) {
            if (bsWnd == LOBYTE(BS_USERBUTTON)) {
                xxxButtonNotifyParent(pwnd, (UINT)(wParam ? BN_PUSHED : BN_UNPUSHED));
            } else if (bsWnd == LOBYTE(BS_OWNERDRAW)) {
                if (wOldState != (UINT)(BUTTONSTATE(pbutn) & BST_PUSHED)) {
                    /*
                     * Only notify for drawing if state has changed..
                     */
                    xxxBNOwnerDraw(pbutn, hdc, ODA_SELECT);
                }
            } else {
                xxxButtonDrawNewState(pbutn, hdc, hbr, wOldState);
            }

            BNReleaseDC(pbutn, hdc);
        }
        if (FWINABLE() && (wOldState != (BOOL)(BUTTONSTATE(pbutn) & BST_PUSHED))) {
            NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
        }
        break;

    case BM_GETCHECK:
        return (LONG)(BUTTONSTATE(pbutn) & BST_CHECKMASK);

    case BM_SETCHECK:
        switch (bsWnd) {
        case LOBYTE(BS_RADIOBUTTON):
        case LOBYTE(BS_AUTORADIOBUTTON):
            if (wParam) {
                    SetWindowState(pwnd, WFTABSTOP);
            } else {
                    ClearWindowState(pwnd, WFTABSTOP);
            }

            /*
             *** FALL THRU **
             */
        case LOBYTE(BS_CHECKBOX):
        case LOBYTE(BS_AUTOCHECKBOX):
            if (wParam) {
                wParam = 1;
            }
            goto CheckIt;

        case LOBYTE(BS_3STATE):
        case LOBYTE(BS_AUTO3STATE):
            if (wParam > BST_INDETERMINATE) {
                wParam = BST_INDETERMINATE;
            }
CheckIt:
            if ((UINT)(BUTTONSTATE(pbutn) & BST_CHECKMASK) != (UINT)wParam) {
                BUTTONSTATE(pbutn) &= ~BST_CHECKMASK;
                BUTTONSTATE(pbutn) |= (UINT)wParam;

                if (!IsVisible(pwnd))
                    break;

                if ((hdc = xxxBNGetDC(pbutn, &hbr)) != NULL) {
                    if (TestWF(pwnd, BFPUSHLIKE)) {
                        xxxDrawButton(pbutn, hdc, PBF_PUSHABLE);
                    } else {
                        xxxButtonDrawCheck(pbutn, hdc, hbr);
                    }
                    BNReleaseDC(pbutn, hdc);
                }

                if (FWINABLE())
                    NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
            }
            break;
        }
        break;

    case BM_SETSTYLE:
        NtUserAlterWindowStyle(hwnd, BS_TYPEMASK, (DWORD)wParam);

        if (lParam) {
            NtUserInvalidateRect(hwnd, NULL, TRUE);
        }
        if (FWINABLE()) {
            NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
        }
        break;

    case WM_SETTEXT:

        /*
         * In case the new group name is longer than the old name,
         * this paints over the old name before repainting the group
         * box with the new name.
         */
        if (bsWnd == LOBYTE(BS_GROUPBOX)) {
            hdc = xxxBNGetDC(pbutn, &hbr);
            if (hdc != NULL) {
                BNCalcRect(pwnd, hdc, &rc, CBR_GROUPTEXT, 0);
                NtUserInvalidateRect(hwnd, &rc, TRUE);

                pwndParent = REBASEPWND(pwnd, spwndParent);
                ThreadLock(pwnd->spwndParent, &tlpwndParent);
                PaintRect(HW(pwndParent), hwnd, hdc, hbr, &rc);
                ThreadUnlock(&tlpwndParent);

                BNReleaseDC(pbutn, hdc);
            }
        }

        lResult = _DefSetText(hwnd, (LPWSTR)lParam, (BOOL)fAnsi);

        if (FWINABLE()) {
            NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, hwnd, OBJID_WINDOW, INDEXID_CONTAINER);
        }
        goto DoEnable;

        /*
         *** FALL THRU **
         */
    case WM_ENABLE:
        lResult = 0L;
DoEnable:
        RepaintButton(pbutn);
        return lResult;

    case WM_SETFONT:
        /*
         * wParam - handle to the font
         * lParam - if true, redraw else don't
         */
        BNSetFont(pbutn, (HFONT)wParam, (BOOL)(lParam != 0));
        break;

    case WM_GETFONT:
        return (LRESULT)pbutn->hFont;

    case BM_GETIMAGE:
    case BM_SETIMAGE:
        if (!IsValidImage(wParam, TestWF(pwnd, BFIMAGEMASK), IMAGE_BMMAX)) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid button image type");
        } else {
            HANDLE  hOld = pbutn->hImage;

            if (message == BM_SETIMAGE) {
                pbutn->hImage = (HANDLE)lParam;
                if (TestWF(pwnd, WFVISIBLE)) {
                    NtUserInvalidateRect(hwnd, NULL, TRUE);
                }
            }
            return (LRESULT)hOld;
        }
        break;

    case WM_NCDESTROY:
    case WM_FINALDESTROY:
        if (pbutn) {
            Unlock(&pbutn->spwnd);
            FreeLookasideEntry(&ButtonLookaside, pbutn);
        }
        NtUserSetWindowFNID(hwnd, FNID_CLEANEDUP_BIT);
        break;

    case WM_NCCREATE:
        // Borland's OBEX has a button with style 0x98; We didn't strip
        // these bits in win3.1 because we checked for 0x08.
        // Stripping these bits cause a GP Fault in OBEX.
        // For win3.1 guys, I use the old code to strip the style bits.
        //
        if (TestWF(pwnd, WFWIN31COMPAT)) {
            if(((!TestWF(pwnd, WFWIN40COMPAT)) &&
                (((LOBYTE(pwnd->style)) & (LOBYTE(~BS_LEFTTEXT))) == LOBYTE(BS_USERBUTTON))) ||
               (TestWF(pwnd, WFWIN40COMPAT) &&
               (bsWnd == LOBYTE(BS_USERBUTTON))))
            {
                // BS_USERBUTTON is no longer allowed for 3.1 and beyond.
                // Just turn to normal push button.
                NtUserAlterWindowStyle(hwnd, BS_TYPEMASK, 0);
                RIPMSG0(RIP_WARNING, "BS_USERBUTTON no longer supported");
            }
        }
        if (TestWF(pwnd,WEFRIGHT)) {
            NtUserAlterWindowStyle(hwnd, BS_RIGHT | BS_RIGHTBUTTON, BS_RIGHT | BS_RIGHTBUTTON);
        }
        goto CallDWP;

    case WM_INPUTLANGCHANGEREQUEST:

        //
        // #115190
        // If the window is one of controls on top of dialogbox,
        // let the parent dialog handle it.
        //
        if (TestwndChild(pwnd) && pwnd->spwndParent) {
            PWND pwndParent = REBASEPWND(pwnd, spwndParent);
            if (pwndParent) {
                PCLS pclsParent = REBASEALWAYS(pwndParent, pcls);

                UserAssert(pclsParent != NULL);
                if (pclsParent->atomClassName == gpsi->atomSysClass[ICLS_DIALOG]) {
                    RIPMSG0(RIP_VERBOSE, "Button: WM_INPUTLANGCHANGEREQUEST is sent to parent.\n");
                    return SendMessageWorker(pwndParent, message, wParam, lParam, FALSE);
                }
            }
        }
        goto CallDWP;

    case WM_UPDATEUISTATE:
        {
            DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi);
            if (ISBSTEXTOROD(pwnd)) {
                pbutn->fPaintKbdCuesOnly = TRUE;
                RepaintButton(pbutn);
                pbutn->fPaintKbdCuesOnly = FALSE;
            }
        }
        break;

    default:
CallDWP:
        return DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi);
    }

    return 0L;
}

/***************************************************************************\
\***************************************************************************/

LRESULT WINAPI ButtonWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return (0L);
    }

    /*
     * If the control is not interested in this message,
     * pass it to DefWindowProc.
     */
    if (!FWINDOWMSG(message, FNID_BUTTON))
        return DefWindowProcWorker(pwnd, message, wParam, lParam, TRUE);

    return ButtonWndProcWorker(pwnd, message, wParam, lParam, TRUE);
}

LRESULT WINAPI ButtonWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return (0L);
    }

    /*
     * If the control is not interested in this message,
     * pass it to DefWindowProc.
     */
    if (!FWINDOWMSG(message, FNID_BUTTON))
        return DefWindowProcWorker(pwnd, message, wParam, lParam, FALSE);

    return ButtonWndProcWorker(pwnd, message, wParam, lParam, FALSE);
}
