/****************************************************************************\
* edECRare.c - EC Edit controls Routines Called rarely are to be
* put in a seperate segment _EDECRare. This file contains
* these routines.
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Support Routines common to Single-line and Multi-Line edit controls
* called Rarely.
*
* Created: 02-08-89 sankar
\****************************************************************************/

#include "precomp.h"
#pragma hdrstop


extern LOOKASIDE EditLookaside;

#define WS_EX_EDGEMASK (WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE)

/*
 * Those two macros assume PED can be referred as "ped."
 */
#define GetCharABCWidthsAorW    ((ped)->fAnsi ? GetCharABCWidthsA : GetCharABCWidthsW)
#define GetCharWidthAorW        ((ped)->fAnsi ? GetCharWidthA : GetCharWidthW)

#define umin(a, b)  ((unsigned)(a) < (unsigned)(b) ? (unsigned)(a) : (unsigned)(b))

typedef BOOL (*PFNABCWIDTHS)(HDC, UINT, UINT, LPABC);
typedef BOOL (*PFNCHARWIDTH)(HDC, UINT, UINT, LPINT);

/***************************************************************************\
*
*  GetMaxOverlapChars - Gives maximum number of overlapping characters due to
*                       negative A or C widths.
*
\***************************************************************************/
DWORD GetMaxOverlapChars( void )
{
    return (DWORD) MAKELONG( gpsi->wMaxLeftOverlapChars, gpsi->wMaxRightOverlapChars ) ;
}

/***************************************************************************\
*
*  ECSetMargin()
*
\***************************************************************************/
void ECSetMargin(PED ped, UINT  wFlags, long lMarginValues, BOOL fRedraw)
{
    BOOL fUseFontInfo = FALSE;
    UINT wValue, wOldLeftMargin, wOldRightMargin;


    if (wFlags & EC_LEFTMARGIN)  /* Set the left margin */ {

        if ((int) (wValue = (int)(short)LOWORD(lMarginValues)) < 0) {
            fUseFontInfo = TRUE;
            wValue = min((ped->aveCharWidth / 2), (int)ped->wMaxNegA);
        }

        ped->rcFmt.left += wValue - ped->wLeftMargin;
        wOldLeftMargin = ped->wLeftMargin;
        ped->wLeftMargin = wValue;
    }

    if (wFlags & EC_RIGHTMARGIN)  /* Set the Right margin */ {

        if ((int) (wValue = (int)(short)HIWORD(lMarginValues)) < 0) {
            fUseFontInfo = TRUE;
            wValue = min((ped->aveCharWidth / 2), (int)ped->wMaxNegC);
        }

        ped->rcFmt.right -= wValue - ped->wRightMargin;
        wOldRightMargin = ped->wRightMargin;
        ped->wRightMargin = wValue;
    }

    if (fUseFontInfo) {
        if (ped->rcFmt.right - ped->rcFmt.left < 2 * ped->aveCharWidth) {
            RIPMSG0(RIP_WARNING, "ECSetMargin: rcFmt is too narrow for EC_USEFONTINFO");

            if (wFlags & EC_LEFTMARGIN)  /* Reset the left margin */ {
                ped->rcFmt.left += wOldLeftMargin - ped->wLeftMargin;
                ped->wLeftMargin = wOldLeftMargin;
            }

            if (wFlags & EC_RIGHTMARGIN)  /* Reset the Right margin */ {
                ped->rcFmt.right -= wOldRightMargin - ped->wRightMargin;
                ped->wRightMargin = wOldRightMargin;
            }

            return;
        }
    }

//    NtUserInvalidateRect(ped->hwnd, NULL, TRUE);
    if (fRedraw) {
        ECInvalidateClient(ped, TRUE);
    }
}

// --------------------------------------------------------------------------
//
//  ECCalcMarginfForDBCSFont()
//
// Jun.24.1996 HideyukN - Ported from Windows95 FarEast version (edecrare.c)
// --------------------------------------------------------------------------
void ECCalcMarginForDBCSFont(PED ped, BOOL fRedraw)
{
    if (!ped->fTrueType)
        return;

    if (!ped->fSingle) {
        // wMaxNegA came from ABC CharWidth.
        if (ped->wMaxNegA != 0) {
            ECSetMargin(ped, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                    MAKELONG(EC_USEFONTINFO, EC_USEFONTINFO),fRedraw);
        }
    } else {
        int    iMaxNegA = 0, iMaxNegC = 0;
        int    i;
        PVOID  lpBuffer;
        LPABC  lpABCBuff;
        ABC    ABCInfo;
        HFONT  hOldFont;
        HDC    hdc = NtUserGetDC(ped->hwnd);

        if (!ped->hFont || !(hOldFont = SelectFont(hdc, ped->hFont))) {
            ReleaseDC(ped->hwnd, hdc);
            return;
        }

        if (lpBuffer = UserLocalAlloc(0,sizeof(ABC) * 256)) {
            lpABCBuff = lpBuffer;
            GetCharABCWidthsAorW(hdc, 0, 255, lpABCBuff);
        } else {
            lpABCBuff = &ABCInfo;
            GetCharABCWidthsAorW(hdc, 0, 0, lpABCBuff);
        }

        i = 0;
        while (TRUE) {
            iMaxNegA = min(iMaxNegA, lpABCBuff->abcA);
            iMaxNegC = min(iMaxNegC, lpABCBuff->abcC);
            if (++i == 256)
                break;
            if (lpBuffer) {
                lpABCBuff++;
            } else {
                GetCharABCWidthsAorW(hdc, i, i, lpABCBuff);
            }
        }

        SelectFont(hdc, hOldFont);

        if (lpBuffer) UserLocalFree(lpBuffer);

        ReleaseDC(ped->hwnd, hdc);

        if ((iMaxNegA != 0) || (iMaxNegC != 0))
           ECSetMargin(ped, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                    MAKELONG((UINT)(-iMaxNegC), (UINT)(-iMaxNegA)),fRedraw);
    }

    return;
}

// --------------------------------------------------------------------------
//
//  GetCharDimensionsEx(HDC hDC, HFONT hfont, LPTEXTMETRIC lptm, LPINT lpcy)
//
// Jun.24.1996 HideyukN - Ported from Windows95 FarEast version (wmclient.c)
// --------------------------------------------------------------------------

WCHAR AveCharWidthData[] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
//
// if an app set a font for vertical writing, even though we don't
// handle it with EC, the escapement of tm can be NON 0. Then cxWidth from
// GetCharDimenstions() could be 0 in GetCharDimensions().
// This will break our caller who don't expect 0 at return. So I created
// this entry  for the case the caller set vertical font.
//
//
int UserGetCharDimensionsEx(HDC hDC, HFONT hfont, LPTEXTMETRIC lptm, LPINT lpcy)
{
    int         cxWidth;
    TEXTMETRIC  tm;
    LOGFONTW    lf;
    WCHAR       wchFaceName[LF_FACESIZE];

    //
    // Is this font vertical font ??
    //
    GetTextFaceW(hDC, LF_FACESIZE, wchFaceName);
    if (wchFaceName[0] != L'@') {
        //
        // if not call GDI...
        //
        return(GdiGetCharDimensions(hDC, lptm, lpcy));
    }

    if (!lptm)
        lptm = &tm;

    GetTextMetrics(hDC, lptm);

    // TMPF_FIXED_PITCH
    //
    //   If this bit is set the font is a variable pitch font.
    //   If this bit is clear the font is a fixed pitch font.
    // Note very carefully that those meanings are the opposite of what the constant name implies.
    //
    if (!(lptm->tmPitchAndFamily & TMPF_FIXED_PITCH)) { // If !variable_width font
        // This is fixed pitch font....
        cxWidth = lptm->tmAveCharWidth;
    } else {
        // This is variable pitch font...
        if (hfont && GetObjectW(hfont, sizeof(LOGFONTW), &lf) && (lf.lfEscapement != 0)) {
            cxWidth = lptm->tmAveCharWidth;
        } else {
            SIZE size;
            GetTextExtentPointW(hDC, AveCharWidthData, 52, &size);
            cxWidth = ((size.cx / 26) + 1) / 2;
        }
    }

    if (lpcy)
        *lpcy = lptm->tmHeight;

    return(cxWidth);
}

/***************************************************************************\
* ECGetText AorW
*
* Copies at most maxCchToCopy chars to the buffer lpBuffer. Returns
* how many chars were actually copied. Null terminates the string based
* on the fNullTerminate flag:
* fNullTerminate --> at most (maxCchToCopy - 1) characters will be copied
* !fNullTerminate --> at most (maxCchToCopy) characters will be copied
*
* History:
\***************************************************************************/

ICH ECGetText(
    PED ped,
    ICH maxCchToCopy,
    LPSTR lpBuffer,
    BOOL fNullTerminate)
{
    PSTR pText;

    if (maxCchToCopy) {

        /*
         * Zero terminator takes the extra byte
         */
        if (fNullTerminate)
            maxCchToCopy--;
        maxCchToCopy = min(maxCchToCopy, ped->cch);

        /*
         * Zero terminate the string
         */
        if (ped->fAnsi)
            *(LPSTR)(lpBuffer + maxCchToCopy) = 0;
        else
            *(((LPWSTR)lpBuffer) + maxCchToCopy) = 0;

        pText = ECLock(ped);
        RtlCopyMemory(lpBuffer, pText, maxCchToCopy*ped->cbChar);
        ECUnlock(ped);
    }

    return maxCchToCopy;
}

/***************************************************************************\
* ECNcCreate AorW
*
* History:
\***************************************************************************/

BOOL ECNcCreate(
    PED ped,
    PWND pwnd,
    LPCREATESTRUCT lpCreateStruct)
{
    HWND hwnd = HWq(pwnd);
    BOOL fAnsi;

    fAnsi = TestWF(pwnd, WFANSICREATOR);

    /*
     * Initialize the ped
     */
    ped->fEncoded = FALSE;
    ped->iLockLevel = 0;

    ped->chLines = NULL;
    ped->pTabStops = NULL;
    ped->charWidthBuffer = NULL;
    ped->fAnsi = fAnsi ? 1 : 0; // Force TRUE to be 1 because its a 1 bit field
    ped->cbChar = (WORD)(fAnsi ? sizeof(CHAR) : sizeof(WCHAR));
    ped->hInstance = pwnd->hModule;
    // IME
    ped->hImcPrev = NULL_HIMC;

    {
        DWORD dwVer = GETEXPWINVER(lpCreateStruct->hInstance);

        ped->fWin31Compat = (dwVer >= 0x030a);
        ped->f40Compat = (dwVer >= 0x0400);
    }

    //
    // NOTE:
    // The order of the following two checks is important.  People can
    // create edit fields with a 3D and a normal border, and we don't
    // want to disallow that.  But we need to detect the "no 3D border"
    // border case too.
    //
    if (TestWF(pwnd, WEFEDGEMASK))
    {
        ped->fBorder = TRUE;
    }
    else if (TestWF(pwnd, WFBORDER))
    {
        ClearWindowState(pwnd, WFBORDER);
        ped->fFlatBorder = TRUE;
        ped->fBorder = TRUE;
    }

    if (!TestWF(pwnd, EFMULTILINE))
        ped->fSingle = TRUE;

    if (TestWF(pwnd, WFDISABLED))
        ped->fDisabled = TRUE;

    if (TestWF(pwnd, EFREADONLY)) {
        if (!ped->fWin31Compat) {
            /*
             * BACKWARD COMPATIBILITY HACK
             *
             * "MileStone" unknowingly sets the ES_READONLY style. So, we strip this
             * style here for all Win3.0 apps (this style is new for Win3.1).
             * Fix for Bug #12982 -- SANKAR -- 01/24/92 --
             */
             ClearWindowState(pwnd, EFREADONLY);
        } else
            ped->fReadOnly = TRUE;
    }


    /*
     * Allocate storage for the text for the edit controls. Storage for single
     * line edit controls will always get allocated in the local data segment.
     * Multiline will allocate in the local ds but the app may free this and
     * allocate storage elsewhere...
     */
    ped->hText = LOCALALLOC(LHND, CCHALLOCEXTRA*ped->cbChar, ped->hInstance);
    if (!ped->hText) {
        FreeLookasideEntry(&EditLookaside, ped);
        NtUserSetWindowFNID(hwnd, FNID_CLEANEDUP_BIT); /* No ped for this window */
        return FALSE; /* If no_memory error */
    }

    ped->cchAlloc = CCHALLOCEXTRA;
    ped->lineHeight = 1;

    ped->hwnd = hwnd;
    ped->hwndParent = lpCreateStruct->hwndParent;

    ped->wImeStatus = 0;

    return (BOOL)DefWindowProcWorker(pwnd, WM_NCCREATE, 0,
            (LPARAM)lpCreateStruct, fAnsi);
}

/***************************************************************************\
* ECCreate AorW
*
* History:
\***************************************************************************/

BOOL ECCreate(
    PED ped,
    LONG windowStyle)
{
    HDC hdc;

    /*
     * Get values from the window instance data structure and put them in the
     * ped so that we can access them easier.
     */
    if (windowStyle & ES_AUTOHSCROLL)
        ped->fAutoHScroll = 1;
    if (windowStyle & ES_NOHIDESEL)
        ped->fNoHideSel = 1;

    ped->format = (LOWORD(windowStyle) & LOWORD(ES_FMTMASK));
    if (TestWF(ped->pwnd, WEFRIGHT) && !ped->format)
        ped->format = ES_RIGHT;

    ped->cchTextMax = MAXTEXT; /* Max # chars we will initially allow */

    /*
     * Set up undo initial conditions... (ie. nothing to undo)
     */
    ped->ichDeleted = (ICH)-1;
    ped->ichInsStart = (ICH)-1;
    ped->ichInsEnd = (ICH)-1;

    // initial charset value - need to do this BEFORE MLCreate is called
    // so that we know not to fool with scrollbars if nessacary
    hdc = ECGetEditDC(ped, TRUE);
    ped->charSet = (BYTE)GetTextCharset(hdc);
    ECReleaseEditDC(ped, hdc, TRUE);

    // FE_IME
    // EC_INSERT_COMPOSITION_CHARACTER: ECCreate() - call ECInitInsert()
    ECInitInsert(ped, THREAD_HKL());

    if(ped->pLpkEditCallout = fpLpkEditControl) {
        return ped->pLpkEditCallout->EditCreate(ped, HW(ped->pwnd));
    } else
        return TRUE;
}

/***************************************************************************\
* ECNcDestroyHandler AorW
*
* Destroys the edit control ped by freeing up all memory used by it.
*
* History:
\***************************************************************************/

void ECNcDestroyHandler(
    PWND pwnd,
    PED ped)
{
    PWND pwndParent;

    /*
     * ped could be NULL if WM_NCCREATE failed to create it...
     */
    if (ped) {

        /*
         * Free the text buffer (always present?)
         */
        LOCALFREE(ped->hText, ped->hInstance);

        /*
         * Free up undo buffer and line start array (if present)
         */
        if (ped->hDeletedText != NULL)
            UserGlobalFree(ped->hDeletedText);

        /*
         * Free tab stop buffer (if present)
         */
        if (ped->pTabStops)
            UserLocalFree(ped->pTabStops);

        /*
         * Free line start array (if present)
         */
        if (ped->chLines) {
            UserLocalFree(ped->chLines);
        }

        /*
         * Free the character width buffer (if present)
         */
        if (ped->charWidthBuffer)
            UserLocalFree(ped->charWidthBuffer);

        /*
         * Free the cursor bitmap
         */
        if (ped->pLpkEditCallout && ped->hCaretBitmap) {
            DeleteObject(ped->hCaretBitmap);
        }

        /*
         * Last but not least, free the ped
         */
        FreeLookasideEntry(&EditLookaside, ped);
    }

    /*
     * Set the window's fnid status so that we can ignore rogue messages
     */
    NtUserSetWindowFNID(HWq(pwnd), FNID_CLEANEDUP_BIT);

    /*
     * If we're part of a combo box, let it know we're gone
     */
    pwndParent = REBASEPWND(pwnd, spwndParent);
    if (pwndParent && GETFNID(pwndParent) == FNID_COMBOBOX) {
        ComboBoxWndProcWorker(pwndParent, WM_PARENTNOTIFY,
                MAKELONG(WM_DESTROY, PTR_TO_ID(pwnd->spmenu)), (LPARAM)HWq(pwnd), FALSE);
    }
}

/***************************************************************************\
* ECSetPasswordChar AorW
*
* Sets the password char to display.
*
* History:
\***************************************************************************/

void ECSetPasswordChar(
    PED ped,
    UINT pwchar)
{
    HDC hdc;
    SIZE size;

    ped->charPasswordChar = pwchar;

    if (pwchar) {
        hdc = ECGetEditDC(ped, TRUE);
        if (ped->fAnsi)
            GetTextExtentPointA(hdc, (LPSTR)&pwchar, 1, &size);
        else
            GetTextExtentPointW(hdc, (LPWSTR)&pwchar, 1, &size);

        GetTextExtentPointW(hdc, (LPWSTR)&pwchar, 1, &size);
        ped->cPasswordCharWidth = max(size.cx, 1);
        ECReleaseEditDC(ped, hdc, TRUE);
    }
    if (pwchar)
        SetWindowState(ped->pwnd, EFPASSWORD);
    else
        ClearWindowState(ped->pwnd, EFPASSWORD);

    ECEnableDisableIME(ped);
}

/***************************************************************************\
*  GetNegABCwidthInfo()
*    This function fills up the ped->charWidthBuffer buffer with the
*      negative A,B and C widths for all the characters below 0x7f in the
*      currently selected font.
*  Returns:
* TRUE, if the function succeeded.
* FALSE, if GDI calls to get the char widths have failed.
*
* Note: not used if LPK installed
\***************************************************************************/
BOOL   GetNegABCwidthInfo(
    PED ped,
    HDC hdc)
{
    LPABC lpABCbuff;
    int   i;
    int   CharWidthBuff[CHAR_WIDTH_BUFFER_LENGTH]; // Local char width buffer.
    int   iOverhang;

    if (!GetCharABCWidthsA(hdc, 0, CHAR_WIDTH_BUFFER_LENGTH-1, (LPABC)ped->charWidthBuffer)) {
        RIPMSG0(RIP_WARNING, "GetNegABCwidthInfo: GetCharABCWidthsA Failed");
        return FALSE;
    }

   // The (A+B+C) returned for some fonts (eg: Lucida Caligraphy) does not
   // equal the actual advanced width returned by GetCharWidths() minus overhang.
   // This is due to font bugs. So, we adjust the 'B' width so that this
   // discrepancy is removed.
   // Fix for Bug #2932 --sankar-- 02/17/93
   iOverhang = ped->charOverhang;
   GetCharWidthA(hdc, 0, CHAR_WIDTH_BUFFER_LENGTH-1, (LPINT)CharWidthBuff);
   lpABCbuff = (LPABC)ped->charWidthBuffer;
   for(i = 0; i < CHAR_WIDTH_BUFFER_LENGTH; i++) {
        lpABCbuff->abcB = CharWidthBuff[i] - iOverhang
                - lpABCbuff->abcA
                - lpABCbuff->abcC;
        lpABCbuff++;
   }

   return(TRUE);
}

/***************************************************************************\
*
*  ECSize() -
*
*  Handle sizing for an edit control's client rectangle.
*  Use lprc as the bounding rectangle if specified; otherwise use the current
*  client rectangle.
*
\***************************************************************************/

void ECSize(
    PED ped,
    LPRECT lprc,
    BOOL fRedraw)
{
    RECT    rc;

    /*
     *  BiDi VB32 Creates an Edit Control and immediately sends a WM_SIZE
     *  message which causes EXSize to be called before ECSetFont, which
     *  in turn causes a divide by zero exception below. This check for
     *  ped->lineHeight will pick it up safely. [samera] 3/5/97
     */
    if(ped->lineHeight == 0)
        return;

    // assume that we won't be able to display the caret
    ped->fCaretHidden = TRUE;


    if ( lprc )
        CopyRect(&rc, lprc);
    else
        _GetClientRect(ped->pwnd, &rc);

    if (!(rc.right - rc.left) || !(rc.bottom - rc.top)) {
        if (ped->rcFmt.right - ped->rcFmt.left)
            return;

        rc.left     = 0;
        rc.top      = 0;
        rc.right    = ped->aveCharWidth * 10;
        rc.bottom   = ped->lineHeight;
    }

    if (!lprc) {
        // subtract the margins from the given rectangle --
        // make sure that this rectangle is big enough to have these margins.
        if ((rc.right - rc.left) > (int)(ped->wLeftMargin + ped->wRightMargin)) {
            rc.left  += ped->wLeftMargin;
            rc.right -= ped->wRightMargin;
        }
    }

    //
    // Leave space so text doesn't touch borders.
    // For 3.1 compatibility, don't subtract out vertical borders unless
    // there is room.
    //
    if (ped->fBorder) {
        int cxBorder = SYSMET(CXBORDER);
        int cyBorder = SYSMET(CYBORDER);

        if (ped->fFlatBorder)
        {
            cxBorder *= 2;
            cyBorder *= 2;
        }

        if (rc.bottom < rc.top + ped->lineHeight + 2*cyBorder)
            cyBorder = 0;

        InflateRect(&rc, -cxBorder, -cyBorder);
    }

    // Is the resulting rectangle too small?  Don't change it then.
    if ((!ped->fSingle) && ((rc.right - rc.left < (int) ped->aveCharWidth) ||
        ((rc.bottom - rc.top) / ped->lineHeight == 0)))
        return;

    // now, we know we're safe to display the caret
    ped->fCaretHidden = FALSE;

    CopyRect(&ped->rcFmt, &rc);

    if (ped->fSingle)
        ped->rcFmt.bottom = min(rc.bottom, rc.top + ped->lineHeight);
    else
        MLSize(ped, fRedraw);

    if (fRedraw) {
        NtUserInvalidateRect(ped->hwnd, NULL, TRUE);
        // UpdateWindow31(ped->hwnd);    Evaluates to NOP in Chicago - Johnl
    }

    // FE_IME
    // ECSize()  - call ECImmSetCompositionWindow()
    //
    // normally this isn't needed because WM_SIZE will cause
    // WM_PAINT and the paint handler will take care of IME
    // composition window. However when the edit window is
    // restored from maximized window and client area is out
    // of screen, the window will not be redrawn.
    //
    if (ped->fFocus && fpImmIsIME(THREAD_HKL())) {
        POINT pt;

        NtUserGetCaretPos(&pt);
        ECImmSetCompositionWindow(ped, pt.x, pt.y);
    }
}

/***************************************************************************\
*
*  ECSetFont AorW () -
*
*  Sets the font used in the edit control.  Warning:  Memory compaction may
*  occur if the font wasn't previously loaded.  If the font handle passed
*  in is NULL, assume the system font.
*
\***************************************************************************/
void   ECSetFont(
    PED ped,
    HFONT hfont,
    BOOL fRedraw)
{
    short  i;
    TEXTMETRIC      TextMetrics;
    HDC             hdc;
    HFONT           hOldFont=NULL;
    UINT            wBuffSize;
    LPINT           lpCharWidthBuff;
    DWORD           dwMaxOverlapChars;
    CHWIDTHINFO     cwi;
    UINT            uExtracharPos;

    hdc = NtUserGetDC(ped->hwnd);

    if (ped->hFont = hfont) {
        //
        // Since the default font is the system font, no need to select it in
        // if that's what the user wants.
        //
        if (!(hOldFont = SelectObject(hdc, hfont))) {
            hfont = ped->hFont = NULL;
        }

        //
        // Get the metrics and ave char width for the currently selected font
        //

        //
        // Call Vertical font-aware AveWidth compute function...
        //
        // FE_SB
        ped->aveCharWidth = UserGetCharDimensionsEx(hdc, hfont, &TextMetrics, &ped->lineHeight);

        /*
         * This might fail when people uses network fonts (or bad fonts).
         */
        if (ped->aveCharWidth == 0) {
            RIPMSG0(RIP_WARNING, "ECSetFont: GdiGetCharDimensions failed");
            if (hOldFont != NULL) {
                SelectObject(hdc, hOldFont);
            }

            /*
             * We've messed up the ped so let's reset the font.
             *  Note that we won't recurse more than once because we'll
             *  pass hfont == NULL.
             * Too bad WM_SETFONT doesn't return a value.
             */
            ECSetFont(ped, NULL, fRedraw);
            return;
        }
    } else {
        ped->aveCharWidth = gpsi->cxSysFontChar;
        ped->lineHeight = gpsi->cySysFontChar;
        TextMetrics = gpsi->tmSysFont;
    }

    ped->charOverhang = TextMetrics.tmOverhang;

    //assume that they don't have any negative widths at all.
    ped->wMaxNegA = ped->wMaxNegC = ped->wMaxNegAcharPos = ped->wMaxNegCcharPos = 0;


    // Check if Proportional Width Font
    //
    // NOTE: as SDK doc says about TEXTMETRIC:
    // TMPF_FIXED_PITCH
    // If this bit is set the font is a variable pitch font. If this bit is clear
    // the font is a fixed pitch font. Note very carefully that those meanings are
    // the opposite of what the constant name implies.
    //
    // Thus we have to reverse the value using logical not (fNonPropFont has 1 bit width)
    //
    ped->fNonPropFont = !(TextMetrics.tmPitchAndFamily & FIXED_PITCH);

    // Check for a TrueType font
    // Older app OZWIN chokes if we allocate a bigger buffer for TrueType fonts
    // So, for apps older than 4.0, no special treatment for TrueType fonts.
    if (ped->f40Compat && (TextMetrics.tmPitchAndFamily & TMPF_TRUETYPE)) {
        ped->fTrueType = GetCharWidthInfo(hdc, &cwi);
#if DBG
        if (!ped->fTrueType) {
            RIPMSG0(RIP_WARNING, "ECSetFont: GetCharWidthInfo Failed");
        }
#endif
    } else {
        ped->fTrueType = FALSE;
    }

    // FE_SB
    //
    // In DBCS Windows, Edit Control must handle Double Byte Character
    // if tmCharSet field of textmetrics is double byte character set
    // such as SHIFTJIS_CHARSET(128:Japan), HANGEUL_CHARSET(129:Korea).
    //
    // We call ECGetDBCSVector even when fAnsi is false so that we could
    // treat ped->fAnsi and ped->fDBCS indivisually. I changed ECGetDBCSVector
    // function so that it returns 0 or 1, because I would like to set ped->fDBCS
    // bit field here.
    //
    ped->fDBCS = ECGetDBCSVector(ped,hdc,TextMetrics.tmCharSet);
    ped->charSet = TextMetrics.tmCharSet;

    if (ped->fDBCS) {
        //
        // Free the character width buffer if ped->fDBCS.
        //
        // I expect single GetTextExtentPoint call is faster than multiple
        // GetTextExtentPoint call (because the graphic engine has a cache buffer).
        // See editec.c/ECTabTheTextOut().
        //
        if (ped->charWidthBuffer) {
            LocalFree(ped->charWidthBuffer);
            ped->charWidthBuffer = NULL;
        }

        //
        // if FullWidthChar : HalfWidthChar == 2 : 1....
        //
        // TextMetrics.tmMaxCharWidth = FullWidthChar width
        // ped->aveCharWidth          = HalfWidthChar width
        //
        if (ped->fNonPropFont &&
            ((ped->aveCharWidth * 2) == TextMetrics.tmMaxCharWidth)) {
            ped->fNonPropDBCS = TRUE;
        } else {
            ped->fNonPropDBCS = FALSE;
        }

    } else {

        //
        // Since the font has changed, let us obtain and save the character width
        // info for this font.
        //
        // First left us find out if the maximum chars that can overlap due to
        // negative widths. Since we can't access USER globals, we make a call here.
        //
        if (!(ped->fSingle || ped->pLpkEditCallout)) {  // Is this a multiline edit control with no LPK present?
            //
            // For multiline edit controls, we maintain a buffer that contains
            // the character width information.
            //
            wBuffSize = (ped->fTrueType) ? (CHAR_WIDTH_BUFFER_LENGTH * sizeof(ABC)) :
                                           (CHAR_WIDTH_BUFFER_LENGTH * sizeof(int));

            if (ped->charWidthBuffer) { /* If buffer already present */
                lpCharWidthBuff = ped->charWidthBuffer;
                ped->charWidthBuffer = UserLocalReAlloc(lpCharWidthBuff, wBuffSize, HEAP_ZERO_MEMORY);
                if (ped->charWidthBuffer == NULL) {
                    UserLocalFree((HANDLE)lpCharWidthBuff);
                }
            } else {
                ped->charWidthBuffer = UserLocalAlloc(HEAP_ZERO_MEMORY, wBuffSize);
            }

            if (ped->charWidthBuffer != NULL) {
                if (ped->fTrueType) {
                    ped->fTrueType = GetNegABCwidthInfo(ped, hdc);
                }

                /*
                 * It is possible that the above attempts could have failed and reset
                 * the value of fTrueType. So, let us check that value again.
                 */
                if (!ped->fTrueType) {
                    if (!GetCharWidthA(hdc, 0, CHAR_WIDTH_BUFFER_LENGTH-1, ped->charWidthBuffer)) {
                        UserLocalFree((HANDLE)ped->charWidthBuffer);
                        ped->charWidthBuffer=NULL;
                    } else {
                        /*
                         * We need to subtract out the overhang associated with
                         * each character since GetCharWidth includes it...
                         */
                        for (i=0;i < CHAR_WIDTH_BUFFER_LENGTH;i++)
                            ped->charWidthBuffer[i] -= ped->charOverhang;
                    }
                }
            } /* if (ped->charWidthBuffer != NULL) */
        } /* if (!ped->fSingle) */
    } /* if (ped->fDBCS) */

    {
        /*
         * Calculate MaxNeg A C metrics
         */
        dwMaxOverlapChars = GetMaxOverlapChars();
        if (ped->fTrueType) {
            if (cwi.lMaxNegA < 0)
                ped->wMaxNegA = -cwi.lMaxNegA;
            else
                ped->wMaxNegA = 0;
            if (cwi.lMaxNegC < 0)
                ped->wMaxNegC = -cwi.lMaxNegC;
            else
                ped->wMaxNegC = 0;
            if (cwi.lMinWidthD != 0) {
                ped->wMaxNegAcharPos = (ped->wMaxNegA + cwi.lMinWidthD - 1) / cwi.lMinWidthD;
                ped->wMaxNegCcharPos = (ped->wMaxNegC + cwi.lMinWidthD - 1) / cwi.lMinWidthD;
                if (ped->wMaxNegA + ped->wMaxNegC > (UINT)cwi.lMinWidthD) {
                    uExtracharPos = (ped->wMaxNegA + ped->wMaxNegC - 1) / cwi.lMinWidthD;
                    ped->wMaxNegAcharPos += uExtracharPos;
                    ped->wMaxNegCcharPos += uExtracharPos;
                }
            } else {
                ped->wMaxNegAcharPos = LOWORD(dwMaxOverlapChars);     // Left
                ped->wMaxNegCcharPos = HIWORD(dwMaxOverlapChars);     // Right
            }

        } else if (ped->charOverhang != 0) {
            /*
             * Some bitmaps fonts (i.e., italic) have under/overhangs;
             *  this is pretty much like having negative A and C widths.
             */
            ped->wMaxNegA = ped->wMaxNegC = ped->charOverhang;
            ped->wMaxNegAcharPos = LOWORD(dwMaxOverlapChars);     // Left
            ped->wMaxNegCcharPos = HIWORD(dwMaxOverlapChars);     // Right
        }
    } /* if (ped->fDBCS) */

    if (!hfont) {
        //
        // We are getting the stats for the system font so update the system
        // font fields in the ed structure since we use these when calculating
        // some spacing.
        //
        ped->cxSysCharWidth = ped->aveCharWidth;
        ped->cySysCharHeight= ped->lineHeight;
    } else if (hOldFont)
        SelectObject(hdc, hOldFont);

    if (ped->fFocus) {
        //
        // Update the caret.
        //
        NtUserHideCaret(ped->hwnd);
        NtUserDestroyCaret();

        if (ped->pLpkEditCallout) {
            ped->pLpkEditCallout->EditCreateCaret (ped, hdc, ECGetCaretWidth(), ped->lineHeight, 0);
        }
        else {
            NtUserCreateCaret(ped->hwnd, (HBITMAP)NULL, ECGetCaretWidth(), ped->lineHeight);
        }
        NtUserShowCaret(ped->hwnd);
    }

    ReleaseDC(ped->hwnd, hdc);

    //
    // Update password character.
    //
    if (ped->charPasswordChar)
        ECSetPasswordChar(ped, ped->charPasswordChar);

    //
    // If it is a TrueType font and it's a new app, set both the margins at the
    // max negative width values for all types of the edit controls.
    // (NOTE: Can't use ped->f40Compat here because edit-controls inside dialog
    // boxes without DS_LOCALEDIT style are always marked as 4.0 compat.
    // This is the fix for NETBENCH 3.0)
    //

    if (ped->fTrueType && (GETAPPVER() >= VER40))
        if (ped->fDBCS) {
            // For DBCS TrueType Font, we calc margin from ABC width.
            ECCalcMarginForDBCSFont(ped, fRedraw);
        } else {
            ECSetMargin(ped, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                        MAKELONG(EC_USEFONTINFO, EC_USEFONTINFO), fRedraw);
        }

    //
    // We need to calc maxPixelWidth when font changes.
    // If the word-wrap is ON, then this is done in MLSize() called later.
    //
    if((!ped->fSingle) && (!ped->fWrap))
        MLBuildchLines(ped, 0, 0, FALSE, NULL, NULL);

    //
    // Recalc the layout.
    //
    ECSize(ped, NULL, fRedraw);

    if ( ped->fFocus && fpImmIsIME(THREAD_HKL()) ) {
        ECImmSetCompositionFont( ped );
    }
}



/***************************************************************************\
*
*  ECIsCharNumeric AorW () -
*
*  Tests whether the character entered is a numeral.
*  For multiline and singleline edit controls with the ES_NUMBER style.
*
\***************************************************************************/
BOOL ECIsCharNumeric(
    PED ped,
    DWORD keyPress)
{
    WORD wCharType;

    if (ped->fAnsi) {
        char ch = (char)keyPress;
        LCID lcid = (LCID)((ULONG_PTR)THREAD_HKL() & 0xFFFF);
        GetStringTypeA(lcid, CT_CTYPE1, &ch, 1, &wCharType);
    } else {
        WCHAR wch = (WCHAR)keyPress;
        GetStringTypeW(CT_CTYPE1, &wch, 1, &wCharType);
    }
    return (wCharType & C1_DIGIT ? TRUE : FALSE);
}

/***************************************************************************\
*
*  ECEnableDisableIME( PED ped )
*
*
*  xx/xx/9x by somebody     Created for Win95
*  xx/xx/95 by kazum        Ported to NT-J 3.51
*  04/15/96 by takaok       Ported to NT 4.0
*
\***************************************************************************/
VOID ECEnableDisableIME( PED ped )
{
    if ( ped->fReadOnly || ped->charPasswordChar ) {
    //
    // IME should be disabled
    //
        HIMC hImc;
        hImc = fpImmGetContext( ped->hwnd );

        if ( hImc != NULL_HIMC ) {
            fpImmReleaseContext( ped->hwnd, hImc );
            ped->hImcPrev = fpImmAssociateContext( ped->hwnd, NULL_HIMC );
        }

    } else {
    //
    // IME should be enabled
    //
        if ( ped->hImcPrev != NULL_HIMC ) {
            ped->hImcPrev = fpImmAssociateContext( ped->hwnd, ped->hImcPrev );

            //
            // Font and the caret position might be changed while
            // IME was being disabled. Set those now if the window
            // has the focus.
            //
            if ( ped->fFocus ) {
                POINT pt;

                ECImmSetCompositionFont( ped );

                NtUserGetCaretPos( &pt );
                ECImmSetCompositionWindow( ped, pt.x, pt.y  );
            }
        }
    }
    ECInitInsert(ped, THREAD_HKL());
}


/***************************************************************************\
*
*  ECImmSetCompositionWindow( PED ped, LONG x, LONG y )
*
*  xx/xx/9x by somebody     Created for Win95
*  xx/xx/95 by kazum        Ported to NT-J 3.51
*  04/15/96 by takaok       Ported to NT 4.0
\***************************************************************************/
VOID ECImmSetCompositionWindow( PED ped, LONG x, LONG y )
{
    COMPOSITIONFORM cf;
    COMPOSITIONFORM cft;
    RECT rcScreenWindow;
    HIMC hImc;

    hImc = fpImmGetContext( ped->hwnd );
    if ( hImc != NULL_HIMC ) {

        if ( ped->fFocus ) {
            GetWindowRect( ped->hwnd, &rcScreenWindow);
            // assuming RECT.left is the first and and RECT.top is the second field
            MapWindowPoints( ped->hwnd, HWND_DESKTOP, (LPPOINT)&rcScreenWindow, 2);
            if (ped->fInReconversion) {
                DWORD dwPoint = (DWORD)(ped->fAnsi ? SendMessageA : SendMessageW)(ped->hwnd, EM_POSFROMCHAR, ped->ichMinSel, 0);

                x = GET_X_LPARAM(dwPoint);
                y = GET_Y_LPARAM(dwPoint);

                RIPMSG2(RIP_WARNING, "ECImmSetCompositionWindow: fInReconversion (%d,%d)", x, y);
            }
            //
            // The window currently has the focus.
            //
            if (ped->fSingle) {
                //
                // Single line edit control.
                //
                cf.dwStyle = CFS_POINT;
                cf.ptCurrentPos.x = x;
                cf.ptCurrentPos.y = y;
                SetRectEmpty(&cf.rcArea);

            } else {
                //
                // Multi line edit control.
                //
                cf.dwStyle = CFS_RECT;
                cf.ptCurrentPos.x = x;
                cf.ptCurrentPos.y = y;
                cf.rcArea = ped->rcFmt;
            }
            fpImmGetCompositionWindow( hImc, &cft );
            if ( (!RtlEqualMemory(&cf,&cft,sizeof(COMPOSITIONFORM))) ||
                 (ped->ptScreenBounding.x != rcScreenWindow.left)    ||
                 (ped->ptScreenBounding.y  != rcScreenWindow.top) ) {

                ped->ptScreenBounding.x = rcScreenWindow.left;
                ped->ptScreenBounding.y = rcScreenWindow.top;
                fpImmSetCompositionWindow( hImc, &cf );
            }
        }
        fpImmReleaseContext( ped->hwnd, hImc );
    }
}

/***************************************************************************\
*
*  ECImmSetCompositionFont( PED ped )
*
*  xx/xx/9x by somebody     Created for Win95
*  xx/xx/95 by kazum        Ported to NT-J 3.51
*  04/15/96 by takaok       Ported to NT 4.0
\***************************************************************************/
VOID  ECImmSetCompositionFont( PED ped )
{
    HIMC hImc;
    LOGFONTW lf;

    if ( (hImc = fpImmGetContext( ped->hwnd )) != NULL_HIMC ) {

        if (ped->hFont) {
            GetObjectW( ped->hFont,
                        sizeof(LOGFONTW),
                        (LPLOGFONTW)&lf);
        } else {
            GetObjectW( GetStockObject(SYSTEM_FONT),
                        sizeof(LOGFONTW),
                        (LPLOGFONTW)&lf);
        }
        fpImmSetCompositionFontW( hImc, &lf );
        fpImmReleaseContext( ped->hwnd, hImc );
    }
}


/***************************************************************************\
*
*  ECInitInsert( PED ped, HKL hkl )
*
*  this function is called when:
*  1) a edit control window is initialized
*  2) active keyboard layout of current thread is changed
*  3) read only attribute of this edit control is changed
*
*  04/15/96 by takaok       Created
\***************************************************************************/
VOID ECInitInsert( PED ped, HKL hkl )
{
    ped->fKorea = FALSE;
    ped->fInsertCompChr = FALSE;
    ped->fNoMoveCaret = FALSE;
    ped->fResultProcess = FALSE;

    if ( fpImmIsIME(hkl) ) {
        if (  PRIMARYLANGID(LOWORD(HandleToUlong(hkl))) == LANG_KOREAN ) {

            ped->fKorea = TRUE;
        }
        //
        // LATER:this flag should be set based on the IME caps
        // retrieved from IME. (Such IME caps should be defined)
        // For now, we can safely assume that only Korean IMEs
        // set CS_INSERTCHAR.
        //
        if ( ped->fKorea ) {
            ped->fInsertCompChr = TRUE;
        }
    }

    //
    // if we had a composition character, the shape of caret
    // is changed. We need to reset the caret shape.
    //
    if ( ped->fReplaceCompChr ) {
        ped->fReplaceCompChr = FALSE;
        ECSetCaretHandler( ped );
    }
}

/***************************************************************************\
*
*  ECSetCaretHandler( PED ped )
*
* History:
*       07/16/96 by takaok      ported from NT 3.51
*
\***************************************************************************/

void ECSetCaretHandler(PED ped)
{
    HDC     hdc;
    SIZE    size;
    PSTR    pText;

//    if (!ped->fInsertCompChr || ped->fReadOnly)
//        return;

    // In any case destroy caret beforehand otherwise SetCaretPos()
    // will get crazy.. win95d-B#992,B#2370
    //
    if (ped->fFocus) {

        NtUserHideCaret(ped->hwnd);
        DestroyCaret();
        if ( ped->fReplaceCompChr ) {

            hdc = ECGetEditDC(ped, TRUE );
            pText = ECLock(ped);

            if ( ped->fAnsi)
                 GetTextExtentPointA(hdc, pText + ped->ichCaret, 2, &size);
            else
                 GetTextExtentPointW(hdc, (LPWSTR)pText + ped->ichCaret, 1, &size);

            ECUnlock(ped);
            ECReleaseEditDC(ped, hdc, TRUE);

            CreateCaret(ped->hwnd, (HBITMAP)NULL, size.cx, ped->lineHeight);
        }
        else {
            CreateCaret(ped->hwnd,
                        (HBITMAP)NULL,
                        (ped->cxSysCharWidth > ped->aveCharWidth ? 1 : 2),
                        ped->lineHeight);
        }

        hdc = ECGetEditDC(ped, TRUE );
        if ( ped->fSingle )
            SLSetCaretPosition( ped, hdc );
        else
            MLSetCaretPosition( ped, hdc );
        ECReleaseEditDC(ped, hdc, TRUE);
        NtUserShowCaret(ped->hwnd);
    }
}


/***************************************************************************\
*
* LONG ECImeCompoistion( PED ped, WPARAM wParam, LPARAM lParam )
*
* WM_IME_COMPOSITION handler for Korean IME
*
* History:
\***************************************************************************/

extern void MLReplaceSel(PED, LPSTR);

#define GET_COMPOSITION_STRING  (ped->fAnsi ? fpImmGetCompositionStringA : fpImmGetCompositionStringW)

BOOL FAR PASCAL ECResultStrHandler(PED ped)
{
    HIMC himc;
    LPSTR lpStr;
    LONG dwLen;

    ped->fInsertCompChr = FALSE;    // clear the state
    ped->fNoMoveCaret = FALSE;

    if ((himc = fpImmGetContext(ped->hwnd)) == 0) {
        return FALSE;
    }

    dwLen = GET_COMPOSITION_STRING(himc, GCS_RESULTSTR, NULL, 0);

    if (dwLen == 0) {
        fpImmReleaseContext(ped->hwnd, himc);
        return FALSE;
    }

    dwLen *= ped->cbChar;
    dwLen += ped->cbChar;

    lpStr = (LPSTR)UserGlobalAlloc(GPTR, dwLen);
    if (lpStr == NULL) {
        fpImmReleaseContext(ped->hwnd, himc);
        return FALSE;
    }

    GET_COMPOSITION_STRING(himc, GCS_RESULTSTR, lpStr, dwLen);

    if (ped->fSingle) {
        SLReplaceSel(ped, lpStr);
    } else {
        MLReplaceSel(ped, lpStr);
    }

    UserGlobalFree((HGLOBAL)lpStr);

    fpImmReleaseContext(ped->hwnd, himc);

    ped->fReplaceCompChr = FALSE;
    ped->fNoMoveCaret = FALSE;
    ped->fResultProcess = FALSE;

    ECSetCaretHandler(ped);

    return TRUE;
}

LRESULT ECImeComposition(PED ped, WPARAM wParam, LPARAM lParam)
{
    INT ich;
    LRESULT lReturn = 1;
    HDC hdc;
    BOOL fSLTextUpdated = FALSE;
    ICH iResult;
    HIMC hImc;
    BYTE TextBuf[4];

    if (!ped->fInsertCompChr) {
        if (lParam & GCS_RESULTSTR) {
            ECInOutReconversionMode(ped, FALSE);

            if (ped->wImeStatus & EIMES_GETCOMPSTRATONCE) {
ResultAtOnce:
                ECResultStrHandler(ped);
                lParam &= ~GCS_RESULTSTR;
            }
        }
        return DefWindowProcWorker(ped->pwnd, WM_IME_COMPOSITION, wParam, lParam, ped->fAnsi);
    }

    // In case of Ansi edit control, the length of minimum composition string
    // is 2. Check here maximum byte of edit control.
    if( ped->fAnsi && ped->cchTextMax == 1 ) {
        HIMC hImc;

        hImc = fpImmGetContext( ped->hwnd );
        fpImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_CANCEL, 0L);
        fpImmReleaseContext( ped->hwnd, hImc );
        NtUserMessageBeep(MB_ICONEXCLAMATION);
        return lReturn;
    }

    // Don't move this after CS_NOMOVECARET check.
    // In case if skip the message, fNoMoveCaret should not be set.
    if ((lParam & CS_INSERTCHAR) && ped->fResultProcess) {

        // Now we're in result processing. GCS_RESULTSTR ends up
        // to WM_IME_CHAR and WM_CHAR. Since WM_CHAR is posted,
        // the message(s) will come later than this CS_INSERTCHAR
        // message. This composition character should be handled
        // after the WM_CHAR message(s).
        //
        if(ped->fAnsi)
            PostMessageA(ped->hwnd, WM_IME_COMPOSITION, wParam, lParam);
        else
            PostMessageW(ped->hwnd, WM_IME_COMPOSITION, wParam, lParam);
        ped->fResultProcess = FALSE;
        return lReturn;
    }

//
// If fReplaceCompChr is TRUE, we change the shape of caret. A block
// caret is displayed on the composition character. From the user's
// point of view, there is no difference if the caret is before the
// composition character or after the composition character. When
// the composition character is finalized, the insertion point should
// be moved to after the character, any way. Therefore checking
// CS_NOMOVECARET bit doesn't make sense in our current implementation.
// [takaok]
//
#if 0
    if (lParam & CS_NOMOVECARET)
        ped->fNoMoveCaret=TRUE;   // stick to current caret pos.
    else
        ped->fNoMoveCaret=FALSE;
#endif

    if (lParam & GCS_RESULTSTR) {

        if (ped->wImeStatus & EIMES_GETCOMPSTRATONCE) {
            goto ResultAtOnce;
        }

        ped->fResultProcess=TRUE;
        if ( ped->fReplaceCompChr ) {
            //
            // we have a DBCS character to be replaced.
            // let's delete it before inserting the new one.
            //
            ich = (ped->fAnsi) ? 2 : 1;
            ped->fReplaceCompChr = FALSE;
            ped->ichMaxSel = min(ped->ichCaret + ich, ped->cch);
            ped->ichMinSel = ped->ichCaret;
            if ( ECDeleteText( ped ) > 0 ) {
                if ( ped->fSingle ) {
                    //
                    // Update the display
                    //
                    ECNotifyParent(ped, EN_UPDATE);
                    hdc = ECGetEditDC(ped,FALSE);
                    SLDrawText(ped, hdc, 0);
                    ECReleaseEditDC(ped,hdc,FALSE);
                    //
                    // Tell parent our text contents changed.
                    //
                    ECNotifyParent(ped, EN_CHANGE);
                }
            }
            ECSetCaretHandler( ped );
        }

    } else if(lParam & CS_INSERTCHAR) {

        //
        // If we are in the middle of a mousedown command, don't do anything.
        //
        if (ped->fMouseDown) {
            return lReturn;
        }

        //
        // We can safely assume that interimm character is always DBCS.
        //
        ich = ( ped->fAnsi ) ? 2 : 1;

        if ( ped->fReplaceCompChr ) {
            //
            // we have a character to be replaced.
            // let's delete it before inserting the new one.
            // when we have a composition characters, the
            // caret is placed before the composition character.
            //
            ped->ichMaxSel = min(ped->ichCaret+ich, ped->cch);
            ped->ichMinSel = ped->ichCaret;
        }

        //
        // let's delete current selected text or composition character
        //
        if ( ped->fSingle ) {
            if ( ECDeleteText( ped ) > 0 ) {
                fSLTextUpdated = TRUE;
            }
        } else {
            MLDeleteText( ped );
        }

        //
        // When the composition charcter is canceled, IME may give us NULL wParam,
        // with CS_INSERTCHAR flag on. We shouldn't insert a NULL character.
        //
        if ( wParam != 0 ) {

            if ( ped->fAnsi ) {
                TextBuf[0] = HIBYTE(LOWORD(wParam)); // leading byte
                TextBuf[1] = LOBYTE(LOWORD(wParam)); // trailing byte
                TextBuf[2] = '\0';
            } else {
                TextBuf[0] = LOBYTE(LOWORD(wParam));
                TextBuf[1] = HIBYTE(LOWORD(wParam));
                TextBuf[2] = '\0';
                TextBuf[3] = '\0';
            }

            if ( ped->fSingle ) {

                iResult = SLInsertText( ped, (LPSTR)TextBuf, ich );
                if (iResult == 0) {
                    /*
                     * Couldn't insert the text, for e.g. the text exceeded the limit.
                     */
                    NtUserMessageBeep(0);
                } else if (iResult > 0) {
                    /*
                     * Remember we need to update the text.
                     */
                    fSLTextUpdated = TRUE;
                }

            } else {

                iResult = MLInsertText( ped, (LPSTR)TextBuf, ich, TRUE);
            }

            if ( iResult > 0 ) {
                //
                // ped->fReplaceCompChr will be reset:
                //
                // 1) when the character is finalized.
                //    we will receive GCS_RESULTSTR
                //
                // 2) when the character is canceled.
                //
                //    we will receive WM_IME_COMPOSITION|CS_INSERTCHAR
                //    with wParam == 0 (in case of user types backspace
                //    at the first element of composition character).
                //
                //      or
                //
                //    we will receive WM_IME_ENDCOMPOSITION message
                //
                ped->fReplaceCompChr = TRUE;

                //
                // Caret should be placed BEFORE the composition
                // character.
                //
                ped->ichCaret = max( 0, ped->ichCaret - ich);
                ECSetCaretHandler( ped );
            } else {

                //
                // We failed to insert a character. We might run out
                // of memory, or reached to the text size limit. let's
                // cancel the composition character.
                //
                hImc = fpImmGetContext(ped->hwnd);
                fpImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
                fpImmReleaseContext(ped->hwnd, hImc);

                ped->fReplaceCompChr = FALSE;
                ECSetCaretHandler( ped );
            }
        } else {
            //
            // the composition character is canceled.
            //
            ped->fReplaceCompChr = FALSE;
            ECSetCaretHandler( ped );
        }

        //
        // We won't notify parent the text change
        // because the composition character has
        // not been finalized.
        //
        if ( fSLTextUpdated ) {

            //
            // Update the display
            //
            ECNotifyParent(ped, EN_UPDATE);

            hdc = ECGetEditDC(ped,FALSE);

            if ( ped->fReplaceCompChr ) {
                //
                // move back the caret to the original position
                // temporarily so that our new block cursor can
                // be located within the visible area of window.
                //
                ped->ichCaret = min( ped->cch, ped->ichCaret + ich);
                SLScrollText(ped, hdc);
                ped->ichCaret = max( 0, ped->ichCaret - ich);
            } else {
                SLScrollText(ped, hdc);
            }
            SLDrawText(ped, hdc, 0);

            ECReleaseEditDC(ped,hdc,FALSE);

            //
            // Tell parent our text contents changed.
            //
            ECNotifyParent(ped, EN_CHANGE);
        }
        return lReturn;
    }

    return DefWindowProcWorker(ped->pwnd, WM_IME_COMPOSITION, wParam, lParam, ped->fAnsi);
}


#ifdef LATER    // fyi: window 98 equiv.
LRESULT ECImeComposition(PED ped, WPARAM wParam, LPARAM lParam)
{
    INT ich;
    LRESULT lReturn = 1;
    HDC hdc;
    BOOL fSLTextUpdated = FALSE;
    ICH iResult;
    HIMC hImc;
    BYTE TextBuf[4];

    // In case of Ansi edit control, the length of minimum composition string
    // is 2. Check here maximum byte of edit control.
    if( ped->fAnsi && ped->cchTextMax == 1 ) {
        HIMC hImc;

        hImc = fpImmGetContext( ped->hwnd );
        fpImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_CANCEL, 0L);
        fpImmReleaseContext( ped->hwnd, hImc );
        MessageBeep(MB_ICONEXCLAMATION);
        return lReturn;
    }

    // Don't move this after CS_NOMOVECARET check.
    // In case if skip the message, fNoMoveCaret should not be set.
    if ((lParam & CS_INSERTCHAR) && ped->fResultProcess) {

        // Now we're in result processing. GCS_RESULTSTR ends up
        // to WM_IME_CHAR and WM_CHAR. Since WM_CHAR is posted,
        // the message(s) will come later than this CS_INSERTCHAR
        // message. This composition character should be handled
        // after the WM_CHAR message(s).
        //
        (ped->fAnsi ? PostMessageA : PostMessageW)(ped->hwnd, WM_IME_COMPOSITION, wParam, lParam);
        ped->fResultProcess = FALSE;
        return lReturn;
    }

    ped->fNoMoveCaret = (lParam & CS_NOMOVECARET) != 0;

    if (lParam & GCS_RESULTSTR) {
        ECInOutReconversionMode(ped, FALSE);

        if (ped->wImeStatus & EIMS_GETCOMPSTRATONCE) {
            ECGetCompStrAtOnce(ped);

            goto PassToDefaultWindowProc;
        }

        // Getting into result processing
        ped->fResultProcess = TRUE;
    }
    else if (lParam & CS_INSERTCHAR) {
        ped->fInsertCompChr = TRUE; // Process this composition character.

        (ped->fSingleLine ? SLChar : MLChar)(ped, wParam, 0);

        if (ped->fInsretCompChr) {
            ped->fReplaceCompChr = TRUE;    // The next character will replace this.
            ped->fInsertCompChr = FALSE;    // Clear the state for the next character.
        }

        ECSetCaretHandler(ped);
        return 0;
    }

PassToDefaultWindowProc:
    return DefWindowProcWorker(ped->pwnd, WM_IME_COMPOSITION, wParam, lParam, ped->fAnsi);
}
#endif


/***************************************************************************\
*
* BOOL HanjaKeyHandler( PED ped )
*
* VK_HANJA handler - Korean only
*
* History: July 15,1996 takaok  ported from NT 3.51
\***************************************************************************/
BOOL HanjaKeyHandler( PED ped )
{
    BOOL changeSelection = FALSE;

    if (ped->fKorea && !ped->fReadOnly) {
        ICH oldCaret = ped->ichCaret;

        if (ped->fReplaceCompChr)
                return FALSE;

        if (ped->ichMinSel < ped->ichMaxSel)
            ped->ichCaret = ped->ichMinSel;

        if (!ped->cch || ped->cch == ped->ichCaret) {
            ped->ichCaret = oldCaret;
            NtUserMessageBeep(MB_ICONEXCLAMATION);
            return FALSE;
        }

        if (ped->fAnsi) {
            if (fpImmEscapeA(THREAD_HKL(), fpImmGetContext(ped->hwnd),
                IME_ESC_HANJA_MODE, (ECLock(ped) + ped->ichCaret * ped->cbChar))) {
                changeSelection = TRUE;
            }
            else
                ped->ichCaret = oldCaret;
            ECUnlock(ped);
        }
        else {
            if (fpImmEscapeW(THREAD_HKL(), fpImmGetContext(ped->hwnd),
                IME_ESC_HANJA_MODE, (ECLock(ped) + ped->ichCaret * ped->cbChar))) {
                changeSelection = TRUE;
            }
            else
                ped->ichCaret = oldCaret;
            ECUnlock(ped);
        }
    }
    return changeSelection;
}


//////////////////////////////////////////////////////////////////////////////
// EcImeRequestHandler()
//
// Handles WM_IME_REQUEST message originated by IME
//
// Histroy:
// 27-Mar-97 Hiroyama Created
//////////////////////////////////////////////////////////////////////////////


LRESULT EcImeRequestHandler(PED ped, WPARAM dwSubMsg, LPARAM lParam)
{
    LRESULT lreturn = 0L;

    switch (dwSubMsg) {
    case IMR_CONFIRMRECONVERTSTRING:
        // Edit control does not allow IME to change it.
        break;

    case IMR_RECONVERTSTRING:
        //
        // CHECK VERSION of the structure
        //
        if (lParam && ((LPRECONVERTSTRING)lParam)->dwVersion != 0) {
            RIPMSG1(RIP_WARNING, "EcImeRequestHandler: RECONVERTSTRING dwVersion is not expected.",
                ((LPRECONVERTSTRING)lParam)->dwVersion);
            return 0L;
        }

        if (ped && ped->fFocus && ped->hText && fpImmIsIME(THREAD_HKL())) {
            UINT cchLen = ped->ichMaxSel - ped->ichMinSel;    // holds character count.
            if (cchLen == 0) {
                // if we have no selection,
                // just return 0.
                break;
            }

            UserAssert(ped->cbChar == sizeof(BYTE) || ped->cbChar == sizeof(WCHAR));

            // This Edit Control has selection.
            if (lParam == 0) {
                //
                // IME just want to get required size for buffer.
                // cchLen + 1 is needed to reserve room for trailing L'\0'.
                //       ~~~~
                lreturn = sizeof(RECONVERTSTRING) + (cchLen + 1) * ped->cbChar;
            } else {
                LPRECONVERTSTRING lpRCS = (LPRECONVERTSTRING)lParam;
                LPVOID lpSrc;
                LPVOID lpDest = (LPBYTE)lpRCS + sizeof(RECONVERTSTRING);

                // check buffer size
                // if the given buffer is smaller than actual needed size,
                // shrink our size to fit the buffer
                if ((INT)lpRCS->dwSize <= sizeof(RECONVERTSTRING) + cchLen * ped->cbChar) {
                    RIPMSG0(RIP_WARNING, "EcImeRequest: ERR09");
                    cchLen = (lpRCS->dwSize - sizeof(RECONVERTSTRING)) / ped->cbChar - ped->cbChar;
                }

                lpRCS->dwStrOffset = sizeof(RECONVERTSTRING); // buffer begins just after RECONVERTSTRING
                lpRCS->dwCompStrOffset =
                lpRCS->dwTargetStrOffset = 0;
                lpRCS->dwStrLen =
                lpRCS->dwCompStrLen =
                lpRCS->dwTargetStrLen = cchLen; // StrLen means TCHAR count

                lpSrc = ECLock(ped);
                if (lpSrc == NULL) {
                    RIPMSG0(RIP_WARNING, "EcImeRequestHandler: LOCALLOCK(ped) failed.");
                } else {
                    RtlCopyMemory(lpDest,
                                  (LPBYTE)lpSrc + ped->ichMinSel * ped->cbChar,
                                  cchLen * ped->cbChar);
                    // Null-Terminate the string
                    if (ped->fAnsi) {
                        LPBYTE psz = (LPBYTE)lpDest;
                        psz[cchLen] = '\0';
                    } else {
                        LPWSTR pwsz = (LPWSTR)lpDest;
                        pwsz[cchLen] = L'\0';
                    }
                    ECUnlock(ped);
                    // final buffer size
                    lreturn = sizeof(RECONVERTSTRING) + (cchLen + 1) * ped->cbChar;

                    ECInOutReconversionMode(ped, TRUE);
                    ECImmSetCompositionWindow(ped, 0, 0);
                }
            }

        }
        break;
    }

    return lreturn;
}
