/****************************************************************************\
*
*  STATIC.C
*
*  Copyright (c) 1985 - 1999, Microsoft Corporation
*
*  Static Dialog Controls Routines
*
*  13-Nov-1990 mikeke   from win3
*  29-Jan-1991 IanJa    StaticPaint -> xxxStaticPaint; partial revalidation
*  01-Nov-1994 ChrisWil merged in Daytona/Chicago w/Ani-Icons.
*
\****************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Local Routines.
 */
VOID   xxxNextAniIconStep(PSTAT);
HANDLE xxxSetStaticImage(PSTAT,HANDLE,BOOL);
VOID xxxStaticLoadImage(PSTAT,LPWSTR);


/*
 * Type table.  This is used for validation of the
 * image-types.  For the PPC release we won't support
 * the metafile format, but others are OK.
 */
#define IMAGE_STMMAX    IMAGE_ENHMETAFILE+1

static BYTE rgbType[IMAGE_STMMAX] = {
    SS_BITMAP,       // IMAGE_BITMAP
    SS_ICON,         // IMAGE_CURSOR
    SS_ICON,         // IMAGE_ICON
    SS_ENHMETAFILE   // IMAGE_ENHMETAFILE
};


/*
 * LOBYTE of SS_ style is index into this array
 */
#define STK_OWNER       0x00
#define STK_IMAGE       0x01
#define STK_TEXT        0x02
#define STK_GRAPHIC     0x03
#define STK_TYPE        0x03

#define STK_ERASE       0x04
#define STK_USEFONT     0x08
#define STK_USETEXT     0x10

BYTE rgstk[] = {
    STK_TEXT | STK_ERASE | STK_USEFONT | STK_USETEXT,       // SS_LEFT
    STK_TEXT | STK_ERASE | STK_USEFONT | STK_USETEXT,       // SS_CENTER
    STK_TEXT | STK_ERASE | STK_USEFONT | STK_USETEXT,       // SS_RIGHT
    STK_IMAGE,                                              // SS_ICON
    STK_GRAPHIC,                                            // SS_BLACKRECT
    STK_GRAPHIC,                                            // SS_GRAYRECT
    STK_GRAPHIC,                                            // SS_WHITERECT
    STK_GRAPHIC,                                            // SS_BLACKFRAME
    STK_GRAPHIC,                                            // SS_GRAYFRAME
    STK_GRAPHIC,                                            // SS_WHITEFRAME
    STK_OWNER,                                              // SS_USERITEM
    STK_TEXT | STK_USEFONT | STK_USETEXT,                   // SS_SIMPLE
    STK_TEXT | STK_ERASE | STK_USEFONT | STK_USETEXT,       // SS_LEFTNOWORDWRAP
    STK_OWNER | STK_USEFONT | STK_USETEXT,                  // SS_OWNERDRAW
    STK_IMAGE,                                              // SS_BITMAP
    STK_IMAGE | STK_ERASE,                                  // SS_ENHMETAFILE
    STK_GRAPHIC,                                            // SS_ETCHEDHORZ
    STK_GRAPHIC,                                            // SS_ETCHEDVERT
    STK_GRAPHIC                                             // SS_ETCHEDFRAME
};

LOOKASIDE StaticLookaside;

/*
 * Common macros for image handling.
 */
#define IsValidImage(imageType, realType, max) \
    ((imageType < max) && (rgbType[imageType] == realType))


/***************************************************************************\
*
*  SetStaticImage()
*
*  Sets bitmap/icon of static guy, either in response to a STM_SETxxxx
*  message, or at create time.
*
\***************************************************************************/

HANDLE xxxSetStaticImage(
    PSTAT  pstat,
    HANDLE hImage,
    BOOL   fDeleteIt)
{
    UINT   bType;
    RECT   rc;
    RECT   rcWindow;
    HANDLE hImageOld;
    DWORD  dwRate;
    UINT   cicur;
    BOOL   fAnimated = FALSE;
    PWND   pwnd = pstat->spwnd;
    HWND   hwnd = HWq(pwnd);

    CheckLock(pwnd);

    bType = TestWF(pwnd, SFTYPEMASK);

    /*
     * If this is an old-ani-icon, then delete its timer.
     */
    if ((bType == SS_ICON) && pstat->cicur > 1) {
        /*
         *  Old cursor was an animated cursor, so kill
         *  the timer that is used to animate it.
         */
        NtUserKillTimer(hwnd, IDSYS_STANIMATE);
    }

    /*
     * Initialize the old-image return value.
     */
    hImageOld = pstat->hImage;

    rc.right = rc.bottom = 0;

    if (hImage != NULL) {

        switch (bType) {

            case SS_ENHMETAFILE: {
                /*
                 * We do NOT resize the window.
                 */
                rc.right  = pwnd->rcClient.right  - pwnd->rcClient.left;
                rc.bottom = pwnd->rcClient.bottom - pwnd->rcClient.top;
                break;
            }

            case SS_BITMAP: {

                    BITMAP bmp;

                    if (GetObject(hImage, sizeof(BITMAP), &bmp)) {
                        rc.right  = bmp.bmWidth;
                        rc.bottom = bmp.bmHeight;
                    }
                }
                break;

            case SS_ICON: {

                    NtUserGetIconSize(hImage, 0, &rc.right, &rc.bottom);
                    rc.bottom /= 2;

                    pstat->cicur = 0;
                    pstat->iicur = 0;

                    // Perhaps we can do something like shell\cpl\main\mouseptr.c
                    // here, and make GetCursorFrameInfo obsolete.
                    if (GetCursorFrameInfo(hImage, NULL, 0, &dwRate, &cicur)) {
                        fAnimated = (cicur > 1);
                        pstat->cicur = cicur;
                    }
                }
                break;
        }
    }

    pstat->hImage = hImage;
    pstat->fDeleteIt = fDeleteIt;


    /*
     *  Resize static to fit.
     *  Do NOT do this for SS_CENTERIMAGE
     */
    if (!TestWF(pwnd, SFCENTERIMAGE))
    {
        /*
         *  Get current window rect in parent's client coordinates.
         */
        GetRect(pwnd, &rcWindow, GRECT_WINDOW | GRECT_PARENTCOORDS);

        /*
         * Get new window dimensions
         */
        rc.left = 0;
        rc.top = 0;

        if (rc.right && rc.bottom) {
            _AdjustWindowRectEx(&rc, pwnd->style, FALSE, pwnd->ExStyle);
            rc.right  -= rc.left;
            rc.bottom -= rc.top;
        }

        rc.left = rcWindow.left;
        rc.top = rcWindow.top;

        NtUserSetWindowPos(
                hwnd,
                HWND_TOP,
                0,
                0,
                rc.right,
                rc.bottom,
                SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
    }

    if (TestWF(pwnd, WFVISIBLE)) {
        NtUserInvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }


    /*
     * If this is an aimated-icon, then start the timer for
     * the animation sequence.
     */
    if(fAnimated) {
        // Perhaps we can do something like shell\cpl\main\mouseptr.c
        // here, and make GetCursorFrameInfo obsolete.
        GetCursorFrameInfo(pstat->hImage, NULL, pstat->iicur, &dwRate, &cicur);
        dwRate = max(200, dwRate * 100 / 6);
        NtUserSetTimer(hwnd, IDSYS_STANIMATE, dwRate, NULL);
    }

    return hImageOld;
}


/***************************************************************************\
*
*  StaticLoadImage()
*
*  Loads the icon or bitmap from the app's resource file if a name was
*  specified in the dialog template.  We assume that the name is the name
*  of the resource to load.
*
\***************************************************************************/

VOID xxxStaticLoadImage(
    PSTAT  pstat,
    LPWSTR lpszName)
{
    HANDLE hImage = NULL;
    PWND   pwnd = pstat->spwnd;

    CheckLock(pwnd);

    if (lpszName && *lpszName) {

        /*
         * Only try to load the icon/bitmap if the string is non null.
         */
        if (*(BYTE FAR *)lpszName == 0xFF)
            lpszName = MAKEINTRESOURCE(((LPWORD)lpszName)[1]);

        /*
         * Load the image.  If it can't be found in the app, try the
         * display driver.
         */
        if (lpszName) {

            switch (TestWF(pwnd, SFTYPEMASK)) {
            case SS_BITMAP:

                /*
                 * If the window is not owned by the server, first call
                 * back out to the client.
                 */
                if (!gfServerProcess && pwnd->hModule)
                    hImage = LoadBitmap(pwnd->hModule, lpszName);

                /*
                 * If the above didn't load it, try loading it from the
                 * display driver (hmod == NULL).
                 */
                if (hImage == NULL)
                    hImage = LoadBitmap(NULL, lpszName);
                break;

            case SS_ICON:
                if (TestWF(pwnd, SFREALSIZEIMAGE)) {
                    if (!gfServerProcess && pwnd->hModule) {
                        hImage = LoadImage(pwnd->hModule, lpszName, IMAGE_ICON, 0, 0, 0);
                    }
                } else {
                    /*
                     * If the window is not owned by the server, first call
                     * back out to the client.  Try loading both icons/cursor
                     * types.
                     */
                    if (!gfServerProcess && pwnd->hModule) {

                        hImage = LoadIcon(pwnd->hModule, lpszName);

                        /*
                         * We will also try to load a cursor-format if the
                         * window is a 4.0 compatible.  Icons/Cursors are really
                         * the same.  We don't do this for 3.x apps for the
                         * usual compatibility reasons.
                         */
                        if ((hImage == NULL) && TestWF(pwnd, WFWIN40COMPAT)) {
                            hImage = LoadCursor(pwnd->hModule, lpszName);
                        }
                    }

                    /*
                     * If the above didn't load it, try loading it from the
                     * display driver (hmod == NULL).
                     */
                    if (hImage == NULL) {
                        hImage = LoadIcon(NULL, lpszName);
                    }
                }

                break;
            }

            /*
             * Set the image if it was loaded.
             */
            if (hImage)
                xxxSetStaticImage(pstat, hImage, TRUE);

        }
    }
}


/***************************************************************************\
* StaticCallback()
*
* Draws text statics, called by DrawState.
*
* History:
\***************************************************************************/

BOOL CALLBACK StaticCallback(
    HDC  hdc,
    PWND pwnd,
    BOOL fUnused,
    int  cx,
    int  cy)
{
    UINT   style;
    LPWSTR lpszName;
    RECT   rc;
    BYTE   bType;

    UNREFERENCED_PARAMETER(fUnused);

    bType = TestWF(pwnd, SFTYPEMASK);
    UserAssert(rgstk[bType] & STK_USETEXT);

    if (pwnd->strName.Length) {
        lpszName = REBASE(pwnd, strName.Buffer);

        style = DT_NOCLIP | DT_EXPANDTABS;

        if (bType != LOBYTE(SS_LEFTNOWORDWRAP)) {
            style |= DT_WORDBREAK;
            style |= (UINT)(bType - LOBYTE(SS_LEFT));

            if (TestWF(pwnd, SFEDITCONTROL))
                style |= DT_EDITCONTROL;
        }

        switch (TestWF(pwnd, SFELLIPSISMASK)) {
        case HIBYTE(LOWORD(SS_ENDELLIPSIS)):
            style |= DT_END_ELLIPSIS | DT_SINGLELINE;
            break;

        case HIBYTE(LOWORD(SS_PATHELLIPSIS)):
            style |= DT_PATH_ELLIPSIS | DT_SINGLELINE;
            break;

        case HIBYTE(LOWORD(SS_WORDELLIPSIS)):
            style |= DT_WORD_ELLIPSIS | DT_SINGLELINE;
            break;
        }

        if (TestWF(pwnd, SFNOPREFIX))
            style |= DT_NOPREFIX;

        if (TestWF(pwnd, SFCENTERIMAGE))
            style |= DT_VCENTER | DT_SINGLELINE;

        rc.left     = 0;
        rc.top      = 0;
        rc.right    = cx;
        rc.bottom   = cy;

        if (TestWF(pwnd, WEFPUIACCELHIDDEN)) {
            style |= DT_HIDEPREFIX;
        } else if (((PSTATWND)pwnd)->pstat->fPaintKbdCuesOnly) {
            style |= DT_PREFIXONLY;
        }

        DrawTextExW(hdc, lpszName, -1, &rc, (DWORD)style, NULL);

    }

    return(TRUE);
}


/***************************************************************************\
* xxxStaticPaint
*
* History:
\***************************************************************************/

void xxxStaticPaint(
    PSTAT pstat,
    HDC   hdc,
    BOOL  fClip)
{
    PWND   pwndParent;
    RECT   rc;
    UINT   cmd;
    BYTE   bType;
    BOOL   fFont;
    HBRUSH hbrControl;
    UINT   oldAlign;
#ifdef USE_MIRRORING
    DWORD  dwOldLayout=0;
#endif

    HANDLE hfontOld = NULL;
    PWND pwnd = pstat->spwnd;
    HWND hwnd = HWq(pwnd);

    CheckLock(pwnd);

    if (TestWF(pwnd, WEFRTLREADING))
    {
        oldAlign = GetTextAlign(hdc);
        SetTextAlign(hdc, oldAlign | TA_RTLREADING);
    }

    bType = TestWF(pwnd, SFTYPEMASK);
    _GetClientRect(pwnd, &rc);

    if (fClip) {
        IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
    }

    fFont = (rgstk[bType] & STK_USEFONT) && (pstat->hFont != NULL);

    if (fFont) {
        hfontOld = SelectObject(hdc, pstat->hFont);
    }


    /*
     * Send WM_CTLCOLORSTATIC to all statics (even frames) for 1.03
     * compatibility.
     */
    SetBkMode(hdc, OPAQUE);
    hbrControl = GetControlBrush(hwnd, hdc, WM_CTLCOLORSTATIC);


    /*
     * Do we erase the background?  We don't for SS_OWNERDRAW
     * and STK_GRAPHIC kind of things.
     */
    pwndParent = REBASEPWND(pwnd, spwndParent);
    if ((rgstk[bType] & STK_ERASE) && !pstat->fPaintKbdCuesOnly) {
        PaintRect(HW(pwndParent), hwnd, hdc, hbrControl, &rc);
    }

    switch (LOBYTE(bType)) {
    case SS_ICON:

        if (pstat->hImage) {

            int     cx;
            int     cy;
            POINT   pt;

#ifdef USE_MIRRORING
            if (TestWF(pwnd,WEFLAYOUTRTL)) {
                dwOldLayout = SetLayoutWidth(hdc, -1, 0);
            }
#endif
            /*
             * Perform the correct rect-setup.
             */
            if (TestWF(pwnd,SFCENTERIMAGE)) {

                NtUserGetIconSize(pstat->hImage, pstat->iicur, &cx, &cy);
                cy >>= 1;

                rc.left   = (rc.right  - cx) / 2;
                rc.right  = (rc.left   + cx);
                rc.top    = (rc.bottom - cy) / 2;
                rc.bottom = (rc.top    + cy);

            } else {

                cx = rc.right  - rc.left;
                cy = rc.bottom - rc.top;
            }


            /*
             * Output the icon.  If it's animated, we indicate
             * the step-frame to output.
             */
            if (GETFNID(pwndParent) == FNID_DESKTOP) {
                SetBrushOrgEx(hdc, 0, 0, &pt);
            } else {
                SetBrushOrgEx(
                        hdc,
                        pwndParent->rcClient.left - pwnd->rcClient.left,
                        pwndParent->rcClient.top - pwnd->rcClient.top,
                        &pt);
            }

            DrawIconEx(hdc, rc.left, rc.top, pstat->hImage, cx, cy,
                       pstat->iicur, hbrControl, DI_NORMAL);

            SetBrushOrgEx(hdc, pt.x, pt.y, NULL);
#ifdef USE_MIRRORING
            if (TestWF(pwnd,WEFLAYOUTRTL)) {
                SetLayoutWidth(hdc, -1, dwOldLayout);
            }
#endif
        } else {

            /*
             * Empty!  Need to erase.
             */
            PaintRect(HW(pwndParent), hwnd, hdc, hbrControl, &rc);
        }
        break;

    case SS_BITMAP:

        if (pstat->hImage) {

            BITMAP  bmp;
            HBITMAP hbmpT;
            RECT    rcTmp;
            BOOL    fErase;


            /*
             * Get the bitmap information.  If this fails, then we
             * can assume somethings wrong with its format...don't
             * draw in this case.
             */
            if (GetObject(pstat->hImage, sizeof(BITMAP), &bmp)) {

                if (TestWF(pwnd, SFCENTERIMAGE)) {

                    fErase = ((bmp.bmWidth  < rc.right) ||
                              (bmp.bmHeight < rc.bottom));

                    rc.left   = (rc.right  - bmp.bmWidth)  >> 1;
                    rc.right  = (rc.left   + bmp.bmWidth);
                    rc.top    = (rc.bottom - bmp.bmHeight) >> 1;
                    rc.bottom = (rc.top    + bmp.bmHeight);

                } else {

                    fErase = FALSE;
                }

                /*
                 * Select in the bitmap and blt it to the client-surface.
                 */
                RtlEnterCriticalSection(&gcsHdc);
                hbmpT = SelectObject(ghdcBits2, pstat->hImage);
                StretchBlt(hdc, rc.left, rc.top, rc.right-rc.left,
                           rc.bottom-rc.top, ghdcBits2, 0, 0, bmp.bmWidth,
                           bmp.bmHeight, SRCCOPY|NOMIRRORBITMAP);

                /*
                 * Only need to erase the background if the image is
                 * centered and it's smaller than the client-area.
                 */
                if (fErase) {

                    HBRUSH hbr;

                    if (hbr = CreateSolidBrush(GetPixel(ghdcBits2, 0, 0))) {

                        POLYPATBLT PolyData;

                        ExcludeClipRect(hdc, rc.left, rc.top,
                                        rc.right, rc.bottom);

                        _GetClientRect(pwnd, &rcTmp);

                        PolyData.x  = 0;
                        PolyData.y  = 0;
                        PolyData.cx = rcTmp.right;
                        PolyData.cy = rcTmp.bottom;
                        PolyData.BrClr.hbr = hbr;

                        PolyPatBlt(hdc,PATCOPY,&PolyData,1,PPB_BRUSH);

                        DeleteObject(hbr);
                    }
                }

                if (hbmpT) {
                    SelectObject(ghdcBits2, hbmpT);
                }
                RtlLeaveCriticalSection(&gcsHdc);
            }
        }
        break;

    case SS_ENHMETAFILE:

        if (pstat->hImage) {

            RECT rcl;

            rcl.left   = rc.left;
            rcl.top    = rc.top;
            rcl.right  = rc.right;
            rcl.bottom = rc.bottom;

            PlayEnhMetaFile(hdc, pstat->hImage, &rcl);
        }
        break;

    case SS_OWNERDRAW: {

            DRAWITEMSTRUCT dis;

            dis.CtlType    = ODT_STATIC;
            dis.CtlID      = PtrToUlong(pwnd->spmenu);
            dis.itemAction = ODA_DRAWENTIRE;
            dis.itemState  = (TestWF(pwnd, WFDISABLED) ? ODS_DISABLED : 0);
            dis.hwndItem   = hwnd;
            dis.hDC        = hdc;
            dis.itemData   = 0L;
            dis.rcItem     = rc;

            if (TestWF(pwnd, WEFPUIACCELHIDDEN)) {
                dis.itemState |= ODS_NOACCEL;
            }

            /*
             * Send a WM_DRAWITEM message to the parent.
             */
            SendMessage(HW(pwndParent), WM_DRAWITEM, (WPARAM)dis.CtlID, (LPARAM)&dis);
        }
        break;

    case SS_LEFT:
    case SS_CENTER:
    case SS_RIGHT:
    case SS_LEFTNOWORDWRAP:

        if (pwnd->strName.Length) {

            UINT dstFlags;

            dstFlags = DST_COMPLEX;

            if (TestWF(pwnd, WFDISABLED)) {
#ifdef LATER
                if (SYSMET(SLOWMACHINE) &&
                   (GetBkColor(hdc)  != SYSRGB(GRAYTEXT))) {

                    SetTextColor(hdc, SYSRGB(GRAYTEXT));
                }
                else
#endif
                    dstFlags |= DSS_DISABLED;
            }

            DrawState(hdc, SYSHBR(WINDOWTEXT),
                (DRAWSTATEPROC)StaticCallback,(LPARAM)pwnd, (WPARAM)TRUE,
                rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                dstFlags);
        }
        break;

    case SS_SIMPLE: {

            LPWSTR lpName;
            UINT cchName;


            /*
             * The "Simple" bType assumes everything, including the following:
             *  1. The Text exists and fits on one line.
             *  2. The Static item is always enabled.
             *  3. The Static item is never changed to be a shorter string.
             *  4. The Parent never responds to the CTLCOLOR message
             */
            if (pwnd->strName.Length) {
                lpName = REBASE(pwnd, strName.Buffer);
                cchName = pwnd->strName.Length / sizeof(WCHAR);
            } else {
                lpName = (LPWSTR)szNull;
                cchName = 0;
            }

            if (TestWF(pwnd,SFNOPREFIX)) {
                ExtTextOut(hdc, rc.left, rc.top, ETO_OPAQUE | ETO_CLIPPED,
                                &rc, lpName, cchName, 0L);
            } else {
                /*
                 * Use OPAQUE for speed.
                 */
                DWORD dwFlags;
                if (TestWF(pwnd, WEFPUIACCELHIDDEN)) {
                    dwFlags = DT_HIDEPREFIX;
                } else if (pstat->fPaintKbdCuesOnly) {
                    dwFlags = DT_PREFIXONLY;
                } else {
                    dwFlags = 0;
                }

                PSMTextOut(hdc, rc.left, rc.top,
                        lpName, cchName, dwFlags);
            }
        }
        break;

    case SS_BLACKFRAME:
        cmd = (COLOR_3DDKSHADOW << 3);
        goto StatFrame;

    case SS_GRAYFRAME:
        cmd = (COLOR_3DSHADOW << 3);
        goto StatFrame;

    case SS_WHITEFRAME:
        cmd = (COLOR_3DHILIGHT << 3);
StatFrame:
        DrawFrame(hdc, &rc, 1, cmd);
        break;

    case SS_BLACKRECT:
        hbrControl = SYSHBR(3DDKSHADOW);
        goto StatRect;

    case SS_GRAYRECT:
        hbrControl = SYSHBR(3DSHADOW);
        goto StatRect;

    case SS_WHITERECT:
        hbrControl = SYSHBR(3DHILIGHT);
StatRect:
        PaintRect(HW(pwndParent), hwnd, hdc, hbrControl, &rc);
        break;

    case SS_ETCHEDFRAME:
        DrawEdge(hdc, &rc, EDGE_ETCHED, BF_RECT);
        break;
    }

    if (hfontOld) {
        SelectObject(hdc, hfontOld);
    }

    if (TestWF(pwnd, WEFRTLREADING)) {
        SetTextAlign(hdc, oldAlign);
    }

}


/***************************************************************************\
*
*  StaticRepaint()
*
\***************************************************************************/

void StaticRepaint(
    PSTAT pstat)
{
    PWND pwnd = pstat->spwnd;

    if (IsVisible(pwnd)) {

        HDC hdc;
        HWND hwnd = HWq(pwnd);

        if (hdc = NtUserGetDC(hwnd)) {
            xxxStaticPaint(pstat, hdc, TRUE);
            NtUserReleaseDC(hwnd, hdc);
        }
    }
}


/***************************************************************************\
*
*  StaticNotifyParent()
*
*  Sends WM_COMMAND notification messages.
*
\***************************************************************************/

// LATER mikeke why do we have multiple versions of notifyparent?

LRESULT FAR PASCAL StaticNotifyParent(
    PWND pwnd,
    PWND pwndParent,
    int  nCode)
{
    LRESULT lret;
    TL   tlpwndParent;

    UserAssert(pwnd);

    if (!pwndParent) {
        pwndParent = REBASEPWND(pwnd, spwndParent);
    }

    ThreadLock(pwndParent, &tlpwndParent);
    lret = SendMessage(HW(pwndParent), WM_COMMAND,
                       MAKELONG(PTR_TO_ID(pwnd->spmenu), nCode), (LPARAM)HWq(pwnd));
    ThreadUnlock(&tlpwndParent);

    return lret;
}

/***************************************************************************\
* StaticWndProc
*
* History:
\***************************************************************************/

LRESULT APIENTRY StaticWndProcWorker(
    PWND  pwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam,
    DWORD fAnsi)
{
    HWND        hwnd = HWq(pwnd);
    BYTE        bType;
    PSTAT       pstat;
    static BOOL fInit = TRUE;

    CheckLock(pwnd);

    VALIDATECLASSANDSIZE(pwnd, FNID_STATIC);
    INITCONTROLLOOKASIDE(&StaticLookaside, STAT, spwnd, 8);

    /*
     * If the control is not interested in this message,
     * pass it to DefWindowProc.
     */
    if (!FWINDOWMSG(message, FNID_STATIC))
        return DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi);

    /*
     * Get the pstat for the given window now since we will use it a lot in
     * various handlers. This was stored using SetWindowLong(hwnd,0,pstat) when
     * we initially created the static control.
     */
    pstat = ((PSTATWND)pwnd)->pstat;

    /*
     * Get the control's type
     */
    bType = TestWF(pwnd, SFTYPEMASK);

    switch (message) {
    case STM_GETICON:
        wParam = IMAGE_ICON;

    case STM_GETIMAGE:
        if (IsValidImage(wParam, bType, IMAGE_STMMAX)) {
            return (LRESULT)pstat->hImage;
        }
        break;

    case STM_SETICON:
        lParam = (LPARAM)wParam;
        wParam = IMAGE_ICON;

    case STM_SETIMAGE:
        if (IsValidImage(wParam, bType, IMAGE_STMMAX)) {
            return (LRESULT)xxxSetStaticImage(pstat, (HANDLE)lParam, FALSE);
        }
        break;

    case WM_ERASEBKGND:

        /*
         * The control will be erased in xxxStaticPaint().
         */
        return TRUE;

    case WM_PRINTCLIENT:
        xxxStaticPaint(pstat, (HDC)wParam, FALSE);
        break;

    case WM_PAINT:
        {
            HDC         hdc;
            PAINTSTRUCT ps;

            if ((hdc = (HDC)wParam) == NULL) {
                hdc = NtUserBeginPaint(hwnd, &ps);
            }

            if (IsVisible(pwnd)) {
                xxxStaticPaint(pstat, hdc, !wParam);
            }

            /*
             * If hwnd was destroyed, BeginPaint was automatically undone.
             */
            if (!wParam) {
                NtUserEndPaint(hwnd, &ps);
            }
        }
        break;

    case WM_CREATE:

        if ((rgstk[bType] & STK_TYPE) == STK_IMAGE) {
            /*
             *  Pull the name from LPCREATESTRUCT like Win95 does
             */
            LPWSTR  lpszName;
            LPSTR   lpszAnsiName;
            struct {
                WORD tag;
                BYTE ordLo;
                BYTE ordHi;
            } dwUnicodeOrdinal;

            if (fAnsi) {
                /*
                 *  Convert the ANSI string to unicode if it exists
                 */
                lpszAnsiName = (LPSTR)((LPCREATESTRUCT)lParam)->lpszName;
                if (lpszAnsiName) {
                    if (lpszAnsiName[0] == (CHAR)0xff) {
                        /*
                         * Convert ANSI ordinal to UNICODE ordinal
                         */
                        dwUnicodeOrdinal.tag = 0xFFFF;
                        dwUnicodeOrdinal.ordLo = lpszAnsiName[1];
                        dwUnicodeOrdinal.ordHi = lpszAnsiName[2];
                        lpszName = (LPWSTR)&dwUnicodeOrdinal;
                    } else {
                        MBToWCSEx(0, lpszAnsiName, -1, &lpszName, -1, TRUE);
                    }
                } else {
                    lpszName = NULL;
                }
            } else {
                lpszName = (LPWSTR)(((LPCREATESTRUCT)lParam)->lpszName);
            }

            /*
             *  Load the image
             */
            xxxStaticLoadImage(pstat, lpszName);

            if (fAnsi &&
                    lpszName &&
                    lpszName != (LPWSTR)&dwUnicodeOrdinal) {
                /*
                 *  Free the converted ANSI string
                 */
                LocalFree(lpszName);
            }
        } else if (bType == SS_ETCHEDHORZ || bType == SS_ETCHEDVERT) {

            /*
             * Resize static window to fit edge.  Horizontal dudes
             * make bottom one edge from top, vertical dudes make
             * right edge one edge from left.
             */

            RECT    rcClient;

            _GetClientRect(pwnd, &rcClient);
            if (bType == SS_ETCHEDHORZ)
                rcClient.bottom = SYSMET(CYEDGE);
            else
                rcClient.right = SYSMET(CXEDGE);

            NtUserSetWindowPos(hwnd, HWND_TOP, 0, 0, rcClient.right,
                rcClient.bottom, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;

    case WM_DESTROY:
        if (((rgstk[bType] & STK_TYPE) == STK_IMAGE) &&
            (pstat->hImage != NULL)                  &&
            (pstat->fDeleteIt)) {

            if (bType == SS_BITMAP) {
                DeleteObject(pstat->hImage);
            } else if (bType == SS_ICON) {
                if (pstat->cicur > 1) {
                    /*
                     *  Kill the animated cursor timer
                     */
                    NtUserKillTimer(hwnd, IDSYS_STANIMATE);
                }
                NtUserDestroyCursor((HCURSOR)(pstat->hImage), CURSOR_CALLFROMCLIENT);
            }
        }
        break;

    case WM_NCCREATE:
        if (TestWF(pwnd,WEFRIGHT)) {
            NtUserAlterWindowStyle(hwnd, SS_TYPEMASK, SS_RIGHT);
        }

        if (TestWF(pwnd, SFSUNKEN) ||
            ((bType == LOBYTE(SS_ETCHEDHORZ)) || (bType == LOBYTE(SS_ETCHEDVERT)))) {
            SetWindowState(pwnd, WEFSTATICEDGE);
        }
        goto CallDWP;

    case WM_NCDESTROY:
    case WM_FINALDESTROY:
        if (pstat) {
            Unlock(&pstat->spwnd);
            FreeLookasideEntry(&StaticLookaside, pstat);
        }
        NtUserSetWindowFNID(hwnd, FNID_CLEANEDUP_BIT);
        break;

    case WM_NCHITTEST:
        return (TestWF(pwnd, SFNOTIFY) ? HTCLIENT : HTTRANSPARENT);

    case WM_LBUTTONDOWN:
    case WM_NCLBUTTONDOWN:
        if (TestWF(pwnd, SFNOTIFY)) {

            /*
             * It is acceptable for an app to destroy a static label
             * in response to a STN_CLICKED notification.
             */
            StaticNotifyParent(pwnd, NULL, STN_CLICKED);
        }
        break;

    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
        if (TestWF(pwnd, SFNOTIFY)) {

            /*
             * It is acceptable for an app to destroy a static label in
             * response to a STN_DBLCLK notification.
             */
            StaticNotifyParent(pwnd, NULL, STN_DBLCLK);
        }
        break;

    case WM_SETTEXT:
        /*
         *  No more hack to set icon/bitmap via WM_SETTEXT!
         */
        if (rgstk[bType] & STK_USETEXT) {
            if (_DefSetText(hwnd, (LPWSTR)lParam, fAnsi)) {
                StaticRepaint(pstat);
                return TRUE;
            }
        }
        break;

    case WM_ENABLE:
        StaticRepaint(pstat);
        if (TestWF(pwnd, SFNOTIFY)) {
            StaticNotifyParent(pwnd, NULL, (wParam ? STN_ENABLE : STN_DISABLE));
        }
        break;

    case WM_GETDLGCODE:
        return (LONG)DLGC_STATIC;

    case WM_SETFONT:

        /*
         * wParam - handle to the font
         * lParam - if true, redraw else don't
         */
        if (rgstk[bType] & STK_USEFONT) {

            pstat->hFont = (HANDLE)wParam;

            if (lParam && TestWF(pwnd, WFVISIBLE)) {
                NtUserInvalidateRect(hwnd, NULL, TRUE);
                UpdateWindow(hwnd);
            }
        }
        break;

    case WM_GETFONT:
        if (rgstk[bType] & STK_USEFONT) {
            return (LRESULT)pstat->hFont;
        }
        break;

    case WM_TIMER:
        if (wParam == IDSYS_STANIMATE) {
            xxxNextAniIconStep(pstat);
        }
        break;

    /*
     *  case WM_GETTEXT:
     *  No more hack to get icon/bitmap via WM_GETTEXT!
     */

    case WM_INPUTLANGCHANGEREQUEST:
        if (IS_IME_ENABLED() || IS_MIDEAST_ENABLED()) {
            /*
             * #115190
             * If the window is one of controls on top of dialogbox,
             * let the parent dialog handle it.
             */
            if (TestwndChild(pwnd) && pwnd->spwndParent) {
                PWND pwndParent = REBASEALWAYS(pwnd, spwndParent);
                if (pwndParent) {
                    PCLS pclsParent = REBASEALWAYS(pwndParent, pcls);

                    UserAssert(pclsParent != NULL);
                    if (pclsParent->atomClassName == gpsi->atomSysClass[ICLS_DIALOG]) {
                        return SendMessageWorker(pwndParent, message, wParam, lParam, FALSE);
                    }
                }
            }
        }
        goto CallDWP;

    case WM_UPDATEUISTATE:
        {
            /*
             * DWP will change the UIState bits accordingly
             */
            DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi);

            if (HIWORD(wParam) & UISF_HIDEACCEL) {
                /*
                 * Change in AccelHidden state: need to repaint
                 */
                if (ISSSTEXTOROD(bType)) {
                    pstat->fPaintKbdCuesOnly = TRUE;
                    StaticRepaint(pstat);
                    pstat->fPaintKbdCuesOnly = FALSE;
                }
            }
        }
        break;

    default:
CallDWP:
        return DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi);
    }

    return 0L;
}


LRESULT WINAPI StaticWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return (0L);
    }

    return StaticWndProcWorker(pwnd, message, wParam, lParam, TRUE);
}

LRESULT WINAPI StaticWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return (0L);
    }

    return StaticWndProcWorker(pwnd, message, wParam, lParam, FALSE);
}

/***************************************************************************\
* Next Animated Icon Step
*
* Advances to the next step in an animaged icon.
*
*
\***************************************************************************/

VOID xxxNextAniIconStep(
    PSTAT pstat)
{
    DWORD dwRate;
    PWND pwnd = pstat->spwnd;
    HWND hwnd = HWq(pwnd);

    /*
     * Stop the timer for the next animation step.
     */
    NtUserKillTimer(hwnd, IDSYS_STANIMATE);

    if (++(pstat->iicur) >= pstat->cicur) {
        pstat->iicur = 0;
    }

    // Perhaps we can do something like shell\cpl\main\mouseptr.c
    // here, and make GetCursorFrameInfo obsolete.
    GetCursorFrameInfo(pstat->hImage, NULL, pstat->iicur, &dwRate, &pstat->cicur);
    dwRate = max(200, dwRate * 100 / 6);

    NtUserInvalidateRect(hwnd, NULL, FALSE);
    UpdateWindow(hwnd);

    NtUserSetTimer(hwnd, IDSYS_STANIMATE, dwRate, NULL);
}

