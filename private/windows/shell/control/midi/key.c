/*
 * KEY.C
 *
 * Copyright (C) 1990 Microsoft Corporation.
 *
 * Edit keymaps dialog box and support functinos.
 */

/* Revision history:
   March 92 Ported to 16/32 common code by Laurie Griffiths (LaurieGr)
*/




/*-=-=-=-=- Include Files       -=-=-=-=-*/

#include "preclude.h"
#include <windows.h>
#include <mmsystem.h>
#include <port1632.h>
#include "hack.h"
#include "cphelp.h"

#include "midimap.h"
#include "midi.h"
#include "extern.h"

/*-=-=-=-=- Prototypes          -=-=-=-=-*/

static MMAPERR PASCAL   MmaperrKeyInit(VOID);
static MMAPERR PASCAL   MmaperrKeyInitNew(VOID);
static VOID PASCAL      KeySize(BOOL);
static VOID PASCAL      KeyPaint(VOID);
static VOID PASCAL      KeyArrowScroll(UINT);
static VOID PASCAL      KeyWindowScroll(UINT, int);
static VOID PASCAL      KeyEditMsg(WORD NotifCode);
static VOID PASCAL      KeyButtonDown(LONG);
static VOID PASCAL      KeySetFocus(UINT, int);
static VOID PASCAL      KeyActiveLine(UINT, UINT);
static int PASCAL       KeySave(HWND, BOOL);

/*-=-=-=-=- Global Definitions  -=-=-=-=-*/

#define PAL_SHOW                0       // Show active line.
#define PAL_HIDE                1       // Hide active line.

#define PSF_REDRAW              0x0001  // Redraw where line used to be.
#define PSF_SHOWIFHIDDEN        0x0002  // Show line if hidden.

#define DEF_KEY_ROWS            16      // number of default key rows

/*-=-=-=-=- Global Variables    -=-=-=-=-*/

static HWND     hScroll;                // scroll bar control handle
static char     * _based(_segname("_CODE")) szNotes [] = {              // Note descriptions
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
static  SZCODE aszMidiSection[] = "midicpl";
static  SZCODE aszMidiKey[] = "keyview";
static  SZCODE aszKeyNumFormat[] = "%3d";
static  SZCODE aszNoteOctaveFormat[] = "     %s %d";
static BOOL     fKeyView;
static int      xArrowOffset;           // key arrow ctrl positional offset

/*-=-=-=-=- Functions           -=-=-=-=-*/

/*
 * KEYBOX
 */
BOOL FAR PASCAL _loadds KeyBox( HWND    hDlg,
                        UINT    uMessage,
                        WPARAM  wParam,
                        LPARAM  lParam  )
{
        int     iRet = FALSE;
        MMAPERR mmaperr;

        switch (uMessage) {

        case WM_INITDIALOG :
                hWnd = hDlg;
                SetFocus(GetDlgItem(hDlg, ID_KEYEDIT));
                if ((mmaperr = MmaperrKeyInit()) != MMAPERR_SUCCESS) {
                        VShowError(hDlg, mmaperr);
                        EndDialog(hDlg, FALSE);
                }
                SetScrollRange(hScroll, SB_CTL, 0, iVertMax, FALSE);
                SetScrollPos(hScroll, SB_CTL, iVertPos, TRUE);
		PlaceWindow(hWnd);
                return FALSE;
        case WM_COMMAND :
            {   WORD id = LOWORD(wParam);
#if defined(WIN16)
                WORD NotifCode = HIWORD(lParam);
                HWND hwnd = LOWORD(lParam);
#else
                WORD NotifCode = HIWORD(wParam);
                HWND hwnd = (HWND)lParam;
#endif //WIN16

                switch (id) {
                case  IDH_DLG_MIDI_KEYEDIT:
                  goto DoHelp;

                case ID_KEYGHOSTEDITFIRST:

                        /* assume the user back-tabbed before the first
                         * control on the current row, so jump to the
                         * previous row (if iCurPos > 0) or the last row
                         * (if iCurPos == 0)
                         */

                        if (fHidden)    //we shouldn't get these messages
                                break;  //when we're hidden -jyg

                        if (NotifCode != EN_SETFOCUS)
                                break;
                        if (iCurPos < 0)
                                /* do nothing */ ;
                        else if (iCurPos > 0)
                                iCurPos--;
                        else
                        {
                                if (iVertPos != 0)
                                {
                                        /* at top -- scroll up one line */
                                        KeyWindowScroll(SB_LINEUP, 0);
                                        iCurPos = 0;
                                }
                                else
                                {
                                        /* wrap to bottom cell */
                                        KeyWindowScroll(SB_THUMBPOSITION, iVertMax);
                                        iCurPos = nLines - 1;

                                        KeySetFocus(PSF_REDRAW|PSF_SHOWIFHIDDEN, 0);
                                        KeyActiveLine(PAL_SHOW, SWP_SHOWWINDOW);
                                }
                        }
                        KeySetFocus(PSF_REDRAW, 1);

                        break;

                case ID_KEYGHOSTEDITLAST:

                        /* assume the user forward-tabbed beyond the last
                         * control on the current row, so jump to the
                         * next row (if iCurPos < nLines - 1) or the first row
                         * (if iCurPos == nLines - 1)
                         */
                        if (fHidden)    //we shouldn't get these messages
                                break;  //when we're hidden -jyg

                        if (NotifCode != EN_SETFOCUS)
                                break;
                        if (iCurPos < 0)
                                /* do nothing */ ;
                        else
                        if (iCurPos < nLines - 1)
                                iCurPos++;
                        else
                        {
                                if (iVertPos != iVertMax)
                                {
                                        /* at bottom -- scroll down one line */
                                        KeyWindowScroll(SB_LINEDOWN, 0);
                                        iCurPos = nLines - 1;
                                }
                                else
                                {
                                        /* wrap to top cell */
                                                                                /* wrap to the top cell */
                                        KeyWindowScroll(SB_THUMBPOSITION,-iVertMax);
                                        iCurPos = 0;

                                        KeySetFocus(PSF_REDRAW|PSF_SHOWIFHIDDEN, 0);
                                        KeyActiveLine(PAL_SHOW, SWP_SHOWWINDOW);
                                }
                        }
                        KeySetFocus(PSF_REDRAW, 1);
                        break;

                case IDOK :
                case IDCANCEL :
                        if (NotifCode != BN_CLICKED)
                                break;
                        if (!fReadOnly && (id == IDOK) && (fModified)) {
                                iRet = KeySave(hDlg, TRUE);
                                if (iRet == IDCANCEL)
                                        break;
                                iRet = (iRet == IDYES);
                        } else
                                iRet = FALSE;
                        GlobalFree(hKeyMap);
                        EndDialog(hDlg, iRet);
                        break;

                case ID_KEYEDIT :
                        KeyEditMsg(NotifCode);
                        break;

                default :
                        return FALSE;
                }
                break;
            } /* end of WM_COMMAND */
        case WM_PAINT :
                KeyPaint();
                break;

        case WM_LBUTTONDOWN :
                KeyButtonDown((LONG)lParam);
                break;

        case WM_VSCROLL :
#if defined(WIN16)
                if ((HWND)HIWORD(lParam) == hScroll)
                        KeyWindowScroll(LOWORD(wParam), (int)LOWORD(lParam));
#else
                if ((HWND)lParam == hScroll)
                        KeyWindowScroll(LOWORD(wParam), (int)HIWORD(wParam));
#endif //WIN16

                else KeyArrowScroll(LOWORD(wParam));
                break;

        case WM_CLOSE :
                PostMessage(hDlg, WM_COMMAND, (WPARAM)IDOK, (LPARAM)0);
                break;

        default:
                if (uMessage == uHelpMessage) {
DoHelp:
                        WinHelp(hWnd, szMidiHlp, HELP_CONTEXT,
                                                IDH_DLG_MIDI_KEYEDIT);
                        return TRUE;
                }
                else
                        return FALSE;
                break;
        }

        return TRUE;
} /* KeyBox */

/*
 * MmaperrKeyInit
 */
static  MMAPERR PASCAL MmaperrKeyInit(
        VOID)
{
        LPMIDIKEYMAP lpKey;
        LONG    lDBU;
        MMAPERR mmaperr;
        UINT    xBU,
                yBU,
                xWidth, // width of current column
                xArrow, // width of arrow control
                xEdit,  // width of edit controls
                yEdit;  // height of edit/arrow controls
        int     i;
        char    szCaption[80];
        char    szCaptionFormat[80];

        fHidden = FALSE;
        iVertPos = 35;
        iCurPos = 0;
        nLines = 0; // necessary?

        hEdit = GetDlgItem(hWnd, ID_KEYEDIT);
        hArrow = GetDlgItem(hWnd, ID_KEYARROW);
        hScroll = GetDlgItem(hWnd, ID_KEYSCROLL);

        if (fReadOnly)
        {
                EnableWindow(GetDlgItem(hWnd,IDOK),FALSE);
                SendMessage(hWnd, DM_SETDEFID, (WPARAM)IDCANCEL, (LPARAM)0);
        }

        lDBU = GetDialogBaseUnits();
        xBU = LOWORD(lDBU);
        yBU = HIWORD(lDBU);

        xArrow = (10 * xBU) / 4;        // about yea big
        xEdit = (40 * xBU) / 4;         // 10 chars wide
        yEdit = (10 * yBU) / 8;         // cookie heaven

        rcBox.left = (HORZMARGIN * xBU) / 4;
        for (i = 0, rcBox.right = rcBox.left; i < 4; i++) {
                rgxPos [i] = rcBox.right;

                switch (i) {

                case 0 :
                        // width of src key # text (3.5 chars wide)
                        rcBox.right += xEdit;
                        break;

                case 1 :
                        // width of src key (percussion) name text
                        rcBox.right += (64 * xBU) / 4; // 16 chars wide
                        break;

                case 2 :
                        // width of dst key # edit control
                        xWidth = xEdit;
                        SetWindowPos(hEdit, NULL, 0L, 0L,
                                xWidth, yEdit, SWP_NOZORDER | SWP_NOMOVE);

                        // set global arrow control offset to proper position
                        rcBox.right += xArrowOffset = xWidth - 1;

                        // width of dst key # arrow control
                        xWidth = xArrow;
                        SetWindowPos(hArrow, NULL, 0L, 0L,
                                xWidth, yEdit, SWP_NOZORDER | SWP_NOMOVE);
                        rcBox.right += xWidth - 1;
                        break;

                case 3 :
                        break;

                }
        }

        if (!fNew) {
                DWORD   dwSize;

                dwSize = mapGetSize(MMAP_KEY, szCurrent);
                if (dwSize < MMAPERR_MAXERROR)
                        return (MMAPERR)dwSize;
                if ((hKeyMap = GlobalAlloc(GHND, dwSize)) == NULL)
                        return MMAPERR_MEMORY;
                lpKey = (LPMIDIKEYMAP)GlobalLock(hKeyMap);
                mmaperr = mapRead( MMAP_KEY, szCurrent, (LPVOID)lpKey);
                if (mmaperr == MMAPERR_SUCCESS)
                        if (lstrcmp(lpKey->szDesc, szCurDesc)) {
                                lstrcpy(lpKey->szDesc, szCurDesc);
                                fNew = TRUE;
                        }
                GlobalUnlock (hKeyMap);
                if (mmaperr != MMAPERR_SUCCESS) {
                        GlobalFree(hKeyMap);
                        hKeyMap = NULL;
                        return mmaperr;
                }
        } else if ((mmaperr = MmaperrKeyInitNew()) != MMAPERR_SUCCESS)
                return mmaperr;

        LoadString(hLibInst, IDS_KEYS, szCaptionFormat, sizeof(szCaptionFormat));
        wsprintf(szCaption, szCaptionFormat, (LPSTR)szCurrent);
        SetWindowText(hWnd, szCaption);

        SendMessage(GetDlgItem(hWnd, ID_KEYDESTMNEM),
                WM_SETFONT, (WPARAM)hFont, (LPARAM)0);
        KeySize(TRUE);
        SetWindowPos(GetDlgItem(hWnd, ID_KEYDESTMNEM), NULL,
                rgxPos [2], yChar, 0L, 0L, SWP_NOSIZE | SWP_NOZORDER);
        Modify(fNew);
        fKeyView = GetProfileInt(aszMidiSection, aszMidiKey, 0);
        return MMAPERR_SUCCESS;
} /* MmaperrKeyInit */

/*
 * MmaperrKeyInitNew
 */
static  MMAPERR PASCAL MmaperrKeyInitNew(
        VOID)
{
        LPMIDIKEYMAP    lpKey;
        int     i;

        if ((hKeyMap = GlobalAlloc(GHND,
                (DWORD)sizeof(MIDIKEYMAP))) == NULL)
                return MMAPERR_MEMORY;
        lpKey = (LPMIDIKEYMAP)GlobalLock(hKeyMap);
        lstrcpy(lpKey->szName, szCurrent);
        lstrcpy(lpKey->szDesc, szCurDesc);
        for (i = 0; i < MIDIPATCHSIZE; i++)
                lpKey->bKMap [i] = (BYTE) i;
        GlobalUnlock(hKeyMap);
        return MMAPERR_SUCCESS;
} /* MmaperrKeyInitNew */

/*
 * KEYSIZE
 */
static
VOID PASCAL KeySize(BOOL fMaximize)
{
        RECT    rcOK,
                rcWnd;
        LONG    lDBU;
        UINT    xBU,
                yBU;
        int     xButton,
                xCenter,
                yTopMar,
                yBotMar,
                yLeftOver,
                yBox,
                yMiniBotMar;

        lDBU = GetDialogBaseUnits();
        xBU = LOWORD(lDBU);
        yBU = HIWORD(lDBU);

        // get the rectangle of the OK button
        GetClientRect(GetDlgItem(hWnd, IDOK), &rcOK);

        // get x-extent of button
        xButton = rcOK.right - rcOK.left;

        // top margin is 2 characters
        yTopMar = (16 * yBU) / 8 - 6; // cookie land

        // bottom margin is 2 * minimum bottom margin dialog units +
        // height of button in pixels
        yBotMar = (VERTMARGIN * 2 * yBU) / 8 + rcOK.bottom - rcOK.top;

        if (fMaximize) {
                // maximize the key box
                SetWindowPos(hWnd, NULL, 0L, 0L,
                        rcBox.right - rcBox.left +
                        (2 * HORZMARGIN * xBU) / 4 +
                        GetSystemMetrics(SM_CXVSCROLL) +
                        (GetSystemMetrics(SM_CXDLGFRAME) + 1) * 2,
                        (DEF_KEY_ROWS * 10 * yBU) / 8 +
                        yTopMar + yBotMar +
                        GetSystemMetrics(SM_CYCAPTION) +
                        GetSystemMetrics(SM_CYDLGFRAME) * 2,
                        SWP_NOZORDER | SWP_NOMOVE);
        }

        // get the x and y extents of the client rectangle
        GetClientRect(hWnd, &rcWnd);
        xClient = rcWnd.right - rcWnd.left;
        yClient = rcWnd.bottom - rcWnd.top;

        // yChar is the height of one row in pixels - 1
        yChar = (10 * yBU) / 8 - 1;

        // xChar is the average width of a character
        xChar = xBU;

        // yBox is the room we actually have to display patchmap rows
        yBox = yClient - yTopMar - yBotMar;

        // nLines is the number of setup rows we can display
        // what is this 16 doing here?
        nLines = min(16, yBox / yChar);

        // yLeftOver is how many pixels are left over
        yLeftOver = yBox - nLines * yChar;

        // add half the leftovers to the top margin
        yTopMar += yLeftOver / 2;

        // rcBox is the box of rows and columns inside the client area
        SetRect(
                &rcBox,
                rcBox.left,
                yTopMar,
                rcBox.right,
                yTopMar + nLines * yChar);

        // xCenter is used to center the OK and CANCEL buttons horizontally
        xCenter =(rcBox.right - rcBox.left - xButton * 3) / 4;

        // yMiniBotMar is the spacing above and below the button
        yMiniBotMar = (VERTMARGIN * yBU) / 8 + yLeftOver / 4;

        SetWindowPos(
                GetDlgItem(hWnd, IDOK),
                NULL,
                rcBox.left + xCenter,
                rcBox.bottom + yMiniBotMar,
                0,
                0,
                SWP_NOSIZE | SWP_NOZORDER);

        SetWindowPos(
                GetDlgItem(hWnd, IDCANCEL),
                NULL,
                rcBox.left + xButton + xCenter * 2,
                rcBox.bottom + yMiniBotMar,
                0,
                0,
                SWP_NOSIZE | SWP_NOZORDER);

        SetWindowPos(
                GetDlgItem(hWnd, IDH_DLG_MIDI_KEYEDIT),
                NULL,
                rcBox.left + xButton * 2 + xCenter * 3,
                rcBox.bottom + yMiniBotMar,
                0,
                0,
                SWP_NOSIZE | SWP_NOZORDER);

        SetWindowPos(
                hScroll,
                NULL,
                rcBox.right,
                rcBox.top,
                GetSystemMetrics(SM_CXVSCROLL),
                rcBox.bottom - rcBox.top + 1,
                SWP_NOZORDER);

        iVertMax = max(0, MIDIPATCHSIZE - nLines);
        iVertPos = min(iVertMax, iVertPos);
        SetScrollRange(hScroll, SB_CTL, 0, iVertMax, FALSE);
        SetScrollPos(hScroll, SB_CTL, iVertPos, TRUE);

        if (iCurPos >= 0 && iCurPos < nLines)
                KeySetFocus(PSF_SHOWIFHIDDEN, 1);
        else
                KeyActiveLine(PAL_HIDE, SWP_NOREDRAW);
} /* KeySize */

/*
 * KEYPAINT
 */
static
VOID PASCAL KeyPaint(VOID)
{
        HPEN    hPen = NULL;
        LPMIDIKEYMAP lpKey;
        PAINTSTRUCT ps;
        int     i,
                iVert,
                iLeft,
                nBegin,
                nEnd,
                iTop,
                iBottom,
                iOctave,
                iNote;
        BOOL    fSelected = FALSE;
        char    szBuf [30];

        BeginPaint(hWnd, &ps);

        if (!ps.rcPaint.bottom)
                goto DonePainting;

        hPen = SelectObject(ps.hdc, GetStockObject(BLACK_PEN));
        hFont = SelectObject(ps.hdc, hFont);
        fSelected = TRUE;

        SetTextColor(ps.hdc, GetSysColor(COLOR_WINDOWTEXT));
        SetBkMode(ps.hdc, TRANSPARENT);

        if (ps.rcPaint.top < rcBox.top) {
                iVert = yChar; // rcBox.top - (yChar * 2) - 1;
                TextOut(ps.hdc, 11, iVert, aszSourceKey, lstrlen(aszSourceKey));
                TextOut(ps.hdc, rgxPos [1] + xChar - 10, iVert,
                        aszSourceKeyName, lstrlen(aszSourceKeyName));
        }

        // calculate top and bottom y coordinates of invalid area
        iTop = max(ps.rcPaint.top, rcBox.top);

        // if top is below the box, forget about painting
        if (iTop > rcBox.bottom)
                goto DonePainting;

        iBottom = min(ps.rcPaint.bottom, rcBox.bottom);

        // calculate left x coordinate of invalid area
        iLeft = max(ps.rcPaint.left, rcBox.left);

        // calculate beginning and ending data row to be repainted
        nBegin = max(0, (iTop - rcBox.top) / yChar);
        nEnd = min(nLines, (iBottom - rcBox.top) / yChar);

        for (i = 0; i < 4; i++) {
                MMoveTo(ps.hdc, rgxPos [i], iTop);
                LineTo(ps.hdc, rgxPos [i], iBottom + 1);
        }
        // vertical position of first line we have to draw
        iVert = rcBox.top + nBegin * yChar;

        // lock the map
        lpKey = (LPMIDIKEYMAP)GlobalLock(hKeyMap);
        iOctave = (iVertPos + nBegin) / 12 - 1;
        iNote = (iVertPos + nBegin) % 12;
        for (i = nBegin; i <= nEnd; i++, iVert += yChar) {
                MMoveTo(ps.hdc, iLeft, iVert);
                LineTo(ps.hdc, min(ps.rcPaint.right, rcBox.right), iVert);

                if (i == nLines)
                        break;

                if (iLeft < rgxPos [1]) {
                        wsprintf(szBuf, aszKeyNumFormat, iVertPos + i);
                        TextOut(ps.hdc, rcBox.left + 2, iVert + 2, szBuf, 3);
                }
                if (iLeft < rgxPos [2]) {
                        if (!fKeyView) {
                                if (!LoadString(hLibInst, IDS_KEYMAP_BASE +
                                                iVertPos + i, szBuf, sizeof(szBuf)))
                                        LoadString(hLibInst, IDS_RESERVED, szBuf, sizeof(szBuf));
                        }
                        else wsprintf(szBuf, aszNoteOctaveFormat,
                                (LPSTR)szNotes [iNote], iOctave);
                        TextOut(ps.hdc, rgxPos [1] + 2, iVert + 2, szBuf,
                                lstrlen(szBuf));
                }
                if (iLeft < rgxPos [3] && i != iCurPos) {
                        wsprintf(szBuf, aszKeyNumFormat, lpKey->bKMap [iVertPos + i]);
                        TextOut(ps.hdc, rgxPos [2] + 2, iVert + 2, szBuf, 3);
                }
                if (++iNote > 11) {
                        iNote = 0;
                        iOctave++;
                }
        }
        GlobalUnlock(hKeyMap);

DonePainting:
        if (fSelected) {
                hFont = SelectObject(ps.hdc, hFont);
                hPen = SelectObject(ps.hdc, hPen);
        }
        EndPaint(hWnd, &ps);
} /* KeyPaint */

/*
 * KEYARROWSCROLL
 *
 * Interpret a scroll message for the arrow control and modify
 * the value in corresponding Keymap array.
 */
static
VOID PASCAL KeyArrowScroll(WPARAM wParam)
{
        LPMIDIKEYMAP lpKey;
        BYTE    bKey;

        lpKey = (LPMIDIKEYMAP)GlobalLock(hKeyMap);
        bKey = lpKey->bKMap [iVertPos + iCurPos];
        GlobalUnlock(hKeyMap);

        switch (wParam) {

        case SB_LINEDOWN :
                if (!bKey--)
                        bKey = 127;
                break;

        case SB_LINEUP :
                if (++bKey > 127)
                        bKey = 0;
                break;

        default:
                break;
        }
        SetDlgItemInt(hWnd, ID_KEYEDIT, bKey, FALSE);
} /* KeyArrowScroll */

/*
 * KEYWINDOWSCROLL
 */
static
VOID PASCAL KeyWindowScroll(    WPARAM  wParam,
                                int     iPos   )
{
        HDC     hDC;
        RECT    rc;
        int     iVertInc;
        BOOL    fWillBeVisible;         // will active line be visible?

        switch (wParam) {

        case SB_LINEUP :
                iVertInc = -1;
                break;

        case SB_LINEDOWN :
                iVertInc = 1;
                break;

        case SB_PAGEUP :
                iVertInc = min(-1, -nLines);
                break;

        case SB_PAGEDOWN :
                iVertInc = max(1, nLines);
                break;

        case SB_THUMBTRACK :
        case SB_THUMBPOSITION :
                iVertInc = iPos - iVertPos;
                break;

        default :
                iVertInc = 0;
        }

        iVertInc = max (-iVertPos, min(iVertInc, iVertMax - iVertPos));

        if (iVertInc != 0)
        {
                iVertPos += iVertInc;
                iCurPos -= iVertInc;

                if (iCurPos < 0 || iCurPos >= nLines) {
                        SetFocus(NULL);
                        fWillBeVisible = FALSE;
                }
                else
                        fWillBeVisible = TRUE;

                SetScrollPos(hScroll, SB_CTL, iVertPos, TRUE);

                hDC = GetDC(hWnd);
                ScrollDC(hDC, 0L, -yChar * iVertInc, &rcBox, &rcBox, NULL,
                        &rc);
                ReleaseDC(hWnd, hDC);

                if (!fHidden)
                        KeyActiveLine(PAL_HIDE, SWP_NOREDRAW);

                if (fWillBeVisible) {
                        KeySetFocus(0, 0);
                        KeyActiveLine(PAL_SHOW, SWP_NOREDRAW);
                }

                InvalidateRect(hWnd, &rc, TRUE);
                UpdateWindow(hWnd);
        }
} /* KeyWindowScroll */

/*
 * KEYEDITMSG
 *
 * This function deals with EN_UPDATE and EN_ACTIVATE messages sent
 * to the key number edit control through the WM_COMMAND message.
 */
static
VOID PASCAL KeyEditMsg(WORD NotifCode)
{
        LPMIDIKEYMAP lpKey;
        LPBYTE  lpbKey;
        UINT    uVal;
        BOOL    bTranslate;

        if (NotifCode != EN_UPDATE && NotifCode != EN_ACTIVATE)
                return;

        lpKey = (LPMIDIKEYMAP)GlobalLock(hKeyMap);

        switch (NotifCode) {

        case EN_UPDATE :
                lpbKey = &lpKey->bKMap [iVertPos + iCurPos];
                uVal = (UINT)GetDlgItemInt(hWnd, ID_KEYEDIT, &bTranslate, FALSE);
                if (uVal <= 127) {
                        if (*lpbKey != (BYTE) uVal) {
                                *lpbKey = (BYTE)uVal;
                                Modify(TRUE);
                        }
                }
                else SetDlgItemInt(hWnd, ID_KEYEDIT, *lpbKey, FALSE);
                break;

        case EN_ACTIVATE :
                SetDlgItemInt(hWnd, ID_KEYEDIT, lpKey->bKMap [iVertPos +
                        iCurPos], FALSE);
                break;

        default :
                break;

        }

        GlobalUnlock(hKeyMap);
} /* KeyEditMsg */

/*
 * KEYBUTTONDOWN
 */
static
VOID PASCAL KeyButtonDown(LONG lParam)
{
        int     x = LOWORD(lParam),
                y = HIWORD(lParam),
                iPos;
        UINT    uFlags;

        if (x < rcBox.left || x > rcBox.right)
                return;
        if (y < rcBox.top || y > rcBox.bottom)
                return;

        iPos = min(nLines - 1, (y - rcBox.top) / yChar);

        if (iPos == iCurPos)
                return;

        if (iCurPos >= 0 && iCurPos < nLines) {
                uFlags = PSF_REDRAW;
                UpdateWindow(hWnd);
        }
        else uFlags = PSF_SHOWIFHIDDEN;

        iCurPos = iPos;
        KeySetFocus(uFlags, x);
} /* KeyButtonDown */

/*
 * KEYSETFOCUS
 */
static
VOID PASCAL KeySetFocus(        UINT    uFlags,
                                int     xPos )
{
        RECT    rc;
        int     yPos = rcBox.top + iCurPos * yChar;

        KeyEditMsg(EN_ACTIVATE);

        GetWindowRect(hEdit, &rc);
        /* on NT this returns a BOOL success indicator.  So what? */

        SetWindowPos(hEdit, NULL, rgxPos [2], yPos, 0L, 0L,
                SWP_NOZORDER | SWP_NOSIZE);
        SetWindowPos(hArrow, NULL, rgxPos [2] + xArrowOffset, yPos, 0,
                0L, SWP_NOZORDER | SWP_NOSIZE);

        if (fHidden && uFlags & PSF_SHOWIFHIDDEN) {
                KeyActiveLine(PAL_SHOW, 0L);
                UpdateWindow(hEdit);
                UpdateWindow(hArrow);
        }

        if (uFlags & PSF_REDRAW && rc.right) {
                ScreenToClient(hWnd, (LPPOINT)&rc);
                ScreenToClient(hWnd, (LPPOINT)&rc + 1);
                rc.right = rcBox.right + 1;
                InvalidateRect(hWnd, &rc, FALSE);
                UpdateWindow(hWnd);
        }
        if (!fHidden)
                SetFocus(hEdit);
#if defined(WIN16)
        SendMessage(hEdit, EM_SETSEL, (WPARAM)NULL, MAKELPARAM(0, 32767));
#else
        SendMessage(hEdit, EM_SETSEL, (WPARAM)NULL, (LPARAM)-1);
#endif //WIN16
} /* KeySetFocus */

/*
 * KEYACTIVELINE
 */
static
VOID PASCAL KeyActiveLine(      UINT    uCase,
                                UINT    uFlags )
{
        static const UINT uDefFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER;

        switch (uCase) {

        case PAL_SHOW :
                if (!fHidden)
                        return;

                uFlags |= SWP_SHOWWINDOW;
                fHidden = FALSE;
                break;

        case PAL_HIDE :
                if (fHidden)
                        return;

                uFlags |= SWP_HIDEWINDOW;
                fHidden = TRUE;
                break;

        default :
                break;

        }
        uFlags |= uDefFlags;
        SetWindowPos(hEdit, NULL, 0L, 0L, 0L, 0L, uFlags);
        SetWindowPos(hArrow, NULL, 0L, 0L, 0L, 0L, uFlags);

        if (uCase == PAL_SHOW && !fHidden)
                SetFocus(hEdit);
} /* KeyActiveLine */

/*
 * KEYSAVE
 */
static  int PASCAL KeySave(
        HWND    hdlg,
        BOOL    bQuery)
{
        LPMIDIKEYMAP    lpKey;
        MMAPERR mmaperr;
        int     iRet = 0;    // Should be a value which is NOT IDCANCEL or IDYES

        if (bQuery) {
                iRet = QuerySave();

                if (iRet != IDYES)
                        return iRet;
        }
        lpKey = (LPMIDIKEYMAP)GlobalLock(hKeyMap);
        mmaperr = mapWrite(MMAP_KEY, (LPVOID)lpKey);
        GlobalUnlock(hKeyMap);
        if (mmaperr != MMAPERR_SUCCESS) {
                VShowError(hdlg, mmaperr);
                return IDCANCEL;
        }
        Modify(FALSE);
        if (fNew)
                fNew = FALSE;

        return iRet;
} /* KeySave */
