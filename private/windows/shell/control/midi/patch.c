/*
 * PATCH.C
 *
 * Copyright (C) 1990 Microsoft Corporation.
 *
 * Edit patchmaps dialog box and support functions.
 */

/* Revision history:
   March 92 Ported to 16/32 common code by Laurie Griffiths (LaurieGr)
*/
/*-=-=-=-=- Include Files       -=-=-=-=-*/

#include "preclude.h"
#include <windows.h>
#include <mmsystem.h>
#include <port1632.h>
#include <string.h>
#include "hack.h"
#include "midimap.h"
#include "cphelp.h"
#include "midi.h"
#include "extern.h"

#if defined(WIN32)
#define _based(x)
#endif //WIN32

/*-=-=-=-=- Prototypes          -=-=-=-=-*/

static  SZCODE aszPatchNumFormat[] = "%3d";
static MMAPERR PASCAL   MmaperrPatchInit(HWND);
static MMAPERR PASCAL   MmaperrPatchInitNew(VOID);
static VOID PASCAL      PatchEnumKeys(HWND);
static VOID PASCAL      PatchSize(HWND, BOOL);
static VOID PASCAL      PatchPaint(VOID);
static VOID PASCAL      PatchArrowScroll(WPARAM, LPARAM);
static VOID PASCAL      PatchWindowScroll(HWND, UINT, int);
static VOID PASCAL      PatchEditMsg(UINT id , WORD NotifCode);
static VOID PASCAL      PatchComboMsg(HWND hDlg, WORD NotifCode);
static VOID PASCAL      PatchButtonDown(HWND, LONG);
static VOID PASCAL      PatchSetFocus(HWND, UINT, int);
static VOID PASCAL      PatchActiveLine(UINT, UINT);
static int PASCAL       PatchSave(HWND, BOOL);

/*-=-=-=-=- Global Definitions  -=-=-=-=-*/

#define PAL_SHOW                0       // show active line
#define PAL_HIDE                1       // hide active line

#define PSF_REDRAW              0x0001  // redraw where line used to be
#define PSF_SHOWIFHIDDEN        0x0002  // show line if hidden

#define DEF_PATCH_ROWS          16      // number of default patch rows

/*-=-=-=-=- Global Variables    -=-=-=-=-*/

static HWND     hCombo,                 // keymap combo-box control handle
                hScroll,                // scroll bar control handle
                hVolEdit,               // volume edit control handle
                hVolArrow;              // volume arrow control handle
static  HGLOBAL hPatchMap;
static int      nKeys,                  // number of key maps in mapfile
                iPatchBase,
                xArrowOffset,           // patch arrow ctrl positional offset
                xVolArrowOffset;        // volume arrow ctrl positional offset

static  char    szCaption[80];
static  char    szCaptionFormat[80];

/*-=-=-=-=- Functions           -=-=-=-=-*/

/*
 * PATCHBOX
 */
BOOL FAR PASCAL _loadds PatchBox(        HWND    hDlg,
                                UINT    uMessage,
                                WPARAM  wParam,
                                LPARAM  lParam  )
{
        int     iRet = FALSE;
        char    szBuf [50];
        MMAPERR mmaperr;

        switch (uMessage) {
        case WM_INITDIALOG :
                hWnd = hDlg;
                SetFocus(GetDlgItem(hDlg, ID_PATCHNUMEDIT));
                if ((mmaperr = MmaperrPatchInit(hDlg)) != MMAPERR_SUCCESS) {
                        VShowError(hDlg, mmaperr);
                        EndDialog(hDlg, FALSE);
                }
                SetScrollRange(hScroll, SB_CTL, 0, iVertMax, FALSE);
                SetScrollPos(hScroll, SB_CTL, iVertPos, TRUE);
		PlaceWindow(hWnd);
                return FALSE;

        case WM_COMMAND :
           {  WORD id = LOWORD(wParam);
#if defined(WIN16)
              WORD NotifCode = HIWORD(lParam);
              HWND hwnd = LOWORD(lParam);
#else
              WORD NotifCode = HIWORD(wParam);
              HWND hwnd = (HWND)lParam;
#endif //WIN16
                switch(id) {
                case  IDH_DLG_MIDI_PATCHEDIT:
                  goto DoHelp;

                case ID_PATCHGHOSTEDITFIRST:

                        /* assume the user back-tabbed before the first
                         * control on the current row, so jump to the
                         * previous row (if iCurPos > 0) or the last row
                         * (if iCurPos == 0)
                         */
                        if (fHidden)    // we shouldn't get these
                                break;  // messages when we're hidden -jyg

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
                                        PatchWindowScroll(hDlg, SB_LINEUP, 0);
                                        iCurPos = 0;
                                }
                                else
                                {
                                        PatchWindowScroll(hDlg, SB_THUMBPOSITION,iVertMax);
                                        iCurPos = nLines - 1;

                                        PatchSetFocus(hDlg, PSF_REDRAW|PSF_SHOWIFHIDDEN, 0L);
                                        PatchActiveLine(PAL_SHOW, SWP_SHOWWINDOW);
                                }
                        }
                        PatchSetFocus(hDlg, PSF_REDRAW, 1);
                        SetFocus(hCombo);
                        break;

                case ID_PATCHGHOSTEDITLAST:

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
                        else if (iCurPos < nLines - 1)
                                iCurPos++;
                        else
                        {
                                if (iVertPos != iVertMax)
                                {
                                        /* at bottom -- scroll down one line */
                                        PatchWindowScroll(hDlg, SB_LINEDOWN, 0);
                                        iCurPos = nLines - 1;
                                }
                                else
                                {
                                        /* wrap to the top cell */
                                        PatchWindowScroll(hDlg, SB_THUMBPOSITION,-iVertMax);
                                        iCurPos = 0;

                                        PatchSetFocus(hDlg, PSF_REDRAW|PSF_SHOWIFHIDDEN, 0L);
                                        PatchActiveLine(PAL_SHOW, SWP_SHOWWINDOW);
                                }
                        }
                        PatchSetFocus(hDlg, PSF_REDRAW, 1);
                        SetFocus(hEdit);

#if defined(WIN16)
                        SendMessage(hEdit, EM_SETSEL, (WPARAM)0, MAKELPARAM(0, 32767));
#else
                        SendMessage(hEdit, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
#endif //WIN16
                        break;

                case IDOK :
                case IDCANCEL :
                        if (NotifCode != BN_CLICKED)
                                break;
                        if (!fReadOnly && (id == IDOK) && fModified) {
                                iRet = PatchSave(hDlg, TRUE);
                                if (iRet == IDCANCEL)
                                        break;
                                iRet = (iRet == IDYES);
                        }
                        else
                                iRet = FALSE;

                        GlobalFree(hPatchMap);
                        nKeys = 0;
                        EndDialog(hDlg, iRet);
                        break;

                case ID_PATCHNUMEDIT :
                case ID_PATCHVOLEDIT :
                        PatchEditMsg(id, NotifCode);
                        break;

                case ID_PATCHCOMBO :
                        PatchComboMsg(hDlg, NotifCode);
                        break;

                case ID_PATCHBASED :
                        if (NotifCode != BN_CLICKED)
                                break;
                        iPatchBase = !iPatchBase;
                        wsprintf(szBuf, aszPatchNumber,
                                !iPatchBase);
                        SetWindowText(hwnd, szBuf);
                        PatchEditMsg(ID_PATCHNUMEDIT, EN_ACTIVATE);
                        SetFocus(hEdit);
#if defined(WIN16)
                        SendMessage(hEdit, EM_SETSEL, (WPARAM)0, MAKELPARAM(0, 32767));
#else
                        SendMessage(hEdit, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
#endif //WIN16
                        InvalidateRect(hWnd, &rcBox, TRUE);
                        break;

                default :
                        return FALSE;
                }
                break;
           } /* end of WM_COMMAND */
        case WM_PAINT :
                PatchPaint();
                break;

        case WM_LBUTTONDOWN :
                PatchButtonDown(hDlg, (LONG)lParam);
                break;

        case WM_VSCROLL :
#if defined(WIN16)
                if ((HWND)HIWORD(lParam) == hScroll)
                        PatchWindowScroll(hDlg, (WORD)wParam, (int)LOWORD(lParam));
                else
                        PatchArrowScroll((WORD)wParam, (LONG)lParam);
#else
                if ((HWND)(lParam) == hScroll)
                        PatchWindowScroll(hDlg, LOWORD(wParam), HIWORD(wParam));
                else
                        PatchArrowScroll(LOWORD(wParam), lParam);

#endif //WIN16
                break;

        case WM_CLOSE :
                PostMessage(hDlg, WM_COMMAND, (WPARAM)IDOK, (LPARAM)0);
                break;

        default:
                if (uMessage == uHelpMessage) {
DoHelp:
                        WinHelp(hWnd, szMidiHlp, HELP_CONTEXT,
                                                IDH_DLG_MIDI_PATCHEDIT);
                        return TRUE;
                }
                else
                        return FALSE;
                break;
        }

        return TRUE;
} /* PatchBox */

/*
 * MmaperrPatchInit
 */
static  MMAPERR PASCAL MmaperrPatchInit(
        HWND hDlg)
{
        PATCHMAP FAR* lpPatch;
        LONG    lDBU;
        MMAPERR mmaperr;
        UINT    xBU,
                yBU,
                xWidth, // width of current column
                xArrow, // width of arrow control
                xEdit,  // width of edit controls
                yEdit,  // height of edit/arrow controls
                xCombo, // width of combo boxes
                yCombo; // height of combo boxes
        int     i;

        fHidden = FALSE;
        iVertPos = 0;
        iCurPos = 0;
        nKeys = 0;
        nLines = 0; // necessary?
        iPatchBase = 1; // getprofile

        hEdit = GetDlgItem(hWnd, ID_PATCHNUMEDIT);
        hArrow = GetDlgItem(hWnd, ID_PATCHNUMARROW);
        hVolEdit = GetDlgItem(hWnd, ID_PATCHVOLEDIT);
        hVolArrow = GetDlgItem(hWnd, ID_PATCHVOLARROW);
        hCombo = GetDlgItem(hWnd, ID_PATCHCOMBO);
        hScroll = GetDlgItem(hWnd, ID_PATCHSCROLL);

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
        xCombo = (64 * xBU) / 4;        // 16 chars wide
        yCombo = (46 * yBU) / 8;        // oreos

        rcBox.left = (HORZMARGIN * xBU) / 4;
        for (i = 0, rcBox.right = rcBox.left; i < 6; i++) {
                rgxPos [i] = rcBox.right;

                switch (i) {

                case 0 :
                        // width of src patch # text (3.5 chars wide)
                        rcBox.right += xEdit;
                        break;

                case 1 :
                        // width of src patch name text
                        rcBox.right += (72 * xBU) / 4; // 18 chars wide
                        break;

                case 2 :
                        // width of dst patch # edit control
                        xWidth = xEdit;
                        SetWindowPos(hEdit, NULL, 0L, 0L,
                                xWidth, yEdit, SWP_NOZORDER | SWP_NOMOVE);

                        // set global arrow control offset to proper position
                        rcBox.right += xArrowOffset = xWidth - 1;

                        // width of dst patch # arrow control
                        xWidth = xArrow;
                        SetWindowPos(hArrow, NULL, 0L, 0L,
                                xWidth, yEdit, SWP_NOZORDER | SWP_NOMOVE);
                        rcBox.right += xWidth - 1;
                        break;

                case 3 :
                        // width of volume % edit control
                        xWidth = xEdit;
                        SetWindowPos(hVolEdit, NULL, 0L, 0L,
                                xWidth, yEdit, SWP_NOZORDER | SWP_NOMOVE);

                        // set global arrow control offset to proper position
                        rcBox.right += xVolArrowOffset = xWidth - 1;

                        // width of volume % arrow control
                        xWidth = xArrow;
                        SetWindowPos(hVolArrow, NULL, 0L, 0L,
                                xWidth, yEdit, SWP_NOZORDER | SWP_NOMOVE);
                        rcBox.right += xWidth - 1;
                        break;

                case 4 :
                        // width of keymap combo box.  I'd use MMAP_MAXLEN
                        xWidth = xCombo;
                        SetWindowPos(hCombo, NULL, 0L, 0L,
                                xWidth, yCombo, SWP_NOZORDER | SWP_NOMOVE);
                        rcBox.right += xWidth - 1;
                        break;

                case 5 :
                        break;

                }
        }

        if (!nKeys)
                PatchEnumKeys(hCombo);

        if (!fNew) {
                if ((hPatchMap = GlobalAlloc(GHND, sizeof(PATCHMAP))) == NULL)
                        return MMAPERR_MEMORY;
                lpPatch = (PATCHMAP FAR*)GlobalLock(hPatchMap);
                if ((mmaperr = mapReadPatchMap(szCurrent, lpPatch)) == MMAPERR_SUCCESS)
                        if (lstrcmp(lpPatch->aszPatchMapDescription, szCurDesc)) {
                                lstrcpy(lpPatch->aszPatchMapDescription, szCurDesc);
                                fNew = TRUE;
                        }
                GlobalUnlock(hPatchMap);
                if (mmaperr != MMAPERR_SUCCESS) {
                        GlobalFree(hPatchMap);
                        hPatchMap = NULL;
                        return mmaperr;
                }
        } else if ((mmaperr = MmaperrPatchInitNew()) != MMAPERR_SUCCESS)
                return mmaperr;

        LoadString(hLibInst, IDS_PATCHES ,szCaptionFormat, sizeof(szCaptionFormat));
        wsprintf(szCaption, szCaptionFormat, (LPSTR)szCurrent);
        SetWindowText(hWnd, szCaption);

        SendMessage(GetDlgItem(hWnd, ID_PATCHDESTMNEM),
                WM_SETFONT, (WPARAM)hFont, (LPARAM)0);
        SendMessage(GetDlgItem(hWnd, ID_PATCHVOLMNEM),
                WM_SETFONT, (WPARAM)hFont, (LPARAM)0);
        SendMessage(GetDlgItem(hWnd, ID_PATCHKEYMNEM),
                WM_SETFONT, (WPARAM)hFont, (LPARAM)0);
        PatchSize(hDlg, TRUE);
        i = rcBox.top - yChar + 5;
        SetWindowPos(GetDlgItem(hWnd, ID_PATCHDESTMNEM), NULL,
                rgxPos [2] - xChar + 7, i, 0L, 0L, SWP_NOSIZE | SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hWnd, ID_PATCHVOLMNEM), NULL,
                rgxPos [3] + xChar - 6, i, 0L, 0L, SWP_NOSIZE | SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hWnd, ID_PATCHKEYMNEM), NULL,
                rgxPos [4] + xChar, i, 0L, 0L, SWP_NOSIZE | SWP_NOZORDER);
#if defined(WIN16)
        SendMessage(hWnd, WM_COMMAND, (WPARAM)ID_PATCHBASED, MAKELPARAM(GetDlgItem(hWnd, ID_PATCHBASED), BN_CLICKED));
#else
        SendMessage( hWnd
                   , WM_COMMAND
                   , (WPARAM)MAKELONG(ID_PATCHBASED, BN_CLICKED)
                   , (LPARAM)GetDlgItem(hWnd, ID_PATCHBASED)
                   );
#endif //WIN16
        Modify(fNew);
        return MMAPERR_SUCCESS;
} /* MmaperrPatchInit */

/*
 * MmaperrPatchInitNew
 */
static  MMAPERR PASCAL MmaperrPatchInitNew(
        VOID)
{
        PATCHMAP FAR*  lpPatch;
        UINT    u;

        if ((hPatchMap = GlobalAlloc(GHND,
                (DWORD)sizeof(PATCHMAP))) == NULL)
                return MMAPERR_MEMORY;
        lpPatch = (PATCHMAP FAR*)GlobalLock(hPatchMap);
        lstrcpy(lpPatch->aszPatchMapName, szCurrent);
        lstrcpy(lpPatch->aszPatchMapDescription, szCurDesc);
        for (u = 0; u < MIDIPATCHSIZE; u++) {
                lpPatch->keymaps[u].bVolume = 100;
                lpPatch->keymaps[u].bDestination = (BYTE)u;
                lstrcpy(lpPatch->keymaps[u].aszKeyMapName, szNone);
        }
        GlobalUnlock(hPatchMap);
        return MMAPERR_SUCCESS;
} /* MmaperrPatchInitNew */

/*
 * PATCHENUMKEYS
 */
static
VOID PASCAL PatchEnumKeys(HWND hCombo)
{
        mapEnumerate(MMAP_KEY, EnumFunc, MMENUM_BASIC, hCombo, NULL);
        SendMessage(hCombo, CB_ADDSTRING, (WPARAM)0, (LPARAM)(LPSTR)szNone);
        nKeys = (int)(LONG)SendMessage(hCombo, CB_GETCOUNT, (WPARAM)NULL, (LPARAM)0);
} /* PatchEnumKeys */

/*
 * PATCHSIZE
 */
static
VOID PASCAL PatchSize(HWND hDlg, BOOL fMaximize)
{
        HWND    hBanana;
        RECT    rcBanana;
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

        // top margin is 4 characters
        yTopMar = (32 * yBU) / 8 - 6; // cookie land

        // bottom margin is 2 * minimum bottom margin dialog units +
        // height of button in pixels
        yBotMar = (VERTMARGIN * 2 * yBU) / 8 + rcOK.bottom - rcOK.top;

        if (fMaximize) {
                // maximize the patch box
                SetWindowPos(hWnd, NULL, 0L, 0L,
                        rcBox.right - rcBox.left +
                        (2 * HORZMARGIN * xBU) / 4 +
                        GetSystemMetrics(SM_CXVSCROLL) +
                        (GetSystemMetrics(SM_CXDLGFRAME) + 1) * 2,
                        (DEF_PATCH_ROWS * 10 * yBU) / 8 +
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
        xCenter = (rcBox.right - rcBox.left - xButton * 3) / 4;

        // yMiniBotMar is the spacing above and below the button
        yMiniBotMar = (VERTMARGIN * yBU) / 8 + yLeftOver / 4;

        SetWindowPos(
                GetDlgItem(hWnd, IDOK),
                NULL,
                rcBox.left + xCenter,
                rcBox.bottom + yMiniBotMar,
                0L,
                0L,
                SWP_NOSIZE | SWP_NOZORDER);

        SetWindowPos(
                GetDlgItem(hWnd, IDCANCEL),
                NULL,
                rcBox.left + xButton + xCenter * 2,
                rcBox.bottom + yMiniBotMar,
                0L,
                0L,
                SWP_NOSIZE | SWP_NOZORDER);

        SetWindowPos(
                GetDlgItem(hWnd, IDH_DLG_MIDI_PATCHEDIT),
                NULL,
                rcBox.left + xButton * 2 + xCenter * 3,
                rcBox.bottom + yMiniBotMar,
                0L,
                0L,
                SWP_NOSIZE | SWP_NOZORDER);

        // get the banana button
        hBanana = GetDlgItem(hWnd, ID_PATCHBASED);

        // get the rectangle of the banana button
        GetClientRect(hBanana, &rcBanana);

        // set the banana position
        SetWindowPos(
                hBanana,
                NULL,
                rcBox.left + (rcBox.right - rcBox.left - rcBanana.right) / 2,
                (6 * yBU) / 8,
                0L,
                0L,
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
                PatchSetFocus(hDlg, PSF_SHOWIFHIDDEN, 1);
        else
                PatchActiveLine(PAL_HIDE, SWP_NOREDRAW);
} /* PatchSize */

/*
 * PATCHPAINT
 */
static
VOID PASCAL PatchPaint(VOID)
{
        HPEN            hPen = 0;
        PATCHMAP FAR*   lpPatch;
        PAINTSTRUCT     ps;
        int     i,
                iVert,
                iLeft,
                nBegin,
                nEnd,
                iTop,
                iBottom;
        BOOL    fSelected = FALSE;
        char    szBuf [50];

        BeginPaint(hWnd, &ps);

        if (!ps.rcPaint.bottom)
                goto DonePainting;

        hPen = SelectObject(ps.hdc, GetStockObject(BLACK_PEN));
        hFont = SelectObject(ps.hdc, hFont);
        fSelected = TRUE;

        SetTextColor(ps.hdc, GetSysColor(COLOR_WINDOWTEXT));
        SetBkMode(ps.hdc, TRANSPARENT);

        if (ps.rcPaint.top < rcBox.top) {
                iVert = rcBox.top - yChar + 5;
                TextOut(ps.hdc, 11, iVert, aszSourcePatch, lstrlen(aszSourcePatch));
                TextOut(ps.hdc, rgxPos [1] + xChar * 2 - 15, iVert,
                        aszSourcePatchName, lstrlen(aszSourcePatchName));
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

        for (i = 0; i < 6; i++) {
                MMoveTo(ps.hdc, rgxPos [i], iTop);
                LineTo(ps.hdc, rgxPos [i], iBottom + 1);
        }
        // vertical position of first line we have to draw
        iVert = rcBox.top + nBegin * yChar;

        // lock the map
        lpPatch = (PATCHMAP FAR*)GlobalLock(hPatchMap);
        for (i = nBegin; i <= nEnd; i++, iVert += yChar) {
                MMoveTo(ps.hdc, iLeft, iVert);
                LineTo(ps.hdc, min(ps.rcPaint.right, rcBox.right), iVert);

                if (i == nLines)
                        break;

                if (iLeft < rgxPos [1]) {
                        wsprintf(szBuf, aszPatchNumFormat, iVertPos + iPatchBase + i);
                        TextOut(ps.hdc, rgxPos [0] + xChar, iVert + 2,
                                szBuf, 3);
                }
                if (iLeft < rgxPos [2]) {
                        if (!LoadString(hLibInst, IDS_PATCHMAP_BASE +
                                        iVertPos + i + 1, szBuf, sizeof(szBuf)))
                                LoadString(hLibInst, IDS_RESERVED, szBuf, sizeof(szBuf));
                        TextOut(ps.hdc, rgxPos [1] + 2, iVert + 2,
                                szBuf, lstrlen(szBuf));
                }

                if (i == iCurPos)
                        continue;

                if (iLeft < rgxPos [3]) {
                        wsprintf(szBuf, aszPatchNumFormat, lpPatch->keymaps[iVertPos + i].bDestination + iPatchBase);
                        TextOut(ps.hdc, rgxPos [2] + xChar * 2, iVert + 2,
                                szBuf, 3);
                }
                if (iLeft < rgxPos [4]) {
                        wsprintf(szBuf, aszPatchNumFormat, lpPatch->keymaps[iVertPos + i].bVolume);
                        TextOut(ps.hdc, rgxPos [3] + xChar * 2, iVert + 2,
                                szBuf, 3);
                }
                if (iLeft < rgxPos [5])
                        TextOut(ps.hdc, rgxPos [4] + 2, iVert + 2, lpPatch->keymaps[iVertPos + i].aszKeyMapName,
                                lstrlen(lpPatch->keymaps[iVertPos + i].aszKeyMapName));
        }
        GlobalUnlock(hPatchMap);

DonePainting:
        if (fSelected) {
                hFont = SelectObject(ps.hdc, hFont);
                hPen = SelectObject(ps.hdc, hPen);
        }
        EndPaint(hWnd, &ps);
} /* PatchPaint */

/*
 * PATCHARROWSCROLL
 *
 * Interpret a scroll message for the arrow control.
 */
static
VOID PASCAL PatchArrowScroll(   WPARAM  wParam,    // scroll code only (even on 32 bit)
                                LPARAM  lParam )   // (hwnd,posn) (16 bit),  hwnd (32 bit)
                                                   // posn not actually used.
{
        PATCHMAP FAR* lpPatch;
        UINT    uId;
        BYTE    bPatch,
                bMax,
                bMin;

        lpPatch = (PATCHMAP FAR*)GlobalLock(hPatchMap);

#if defined(WIN16)
        if ((HWND)HIWORD(lParam) == hArrow) {
#else
        if ((HWND)lParam == hArrow) {
#endif
                uId = ID_PATCHNUMEDIT;
                bPatch = lpPatch->keymaps[iVertPos + iCurPos].bDestination + (BYTE)iPatchBase;
                bMax = (BYTE)(127 + iPatchBase);        // MMAP_MAXPATCHES
                bMin = (BYTE)iPatchBase;
        }
        else {
                uId = ID_PATCHVOLEDIT;
                bPatch = lpPatch->keymaps[iVertPos + iCurPos].bVolume;
                bMax = 200;                     // MMAP_MAXVOLUME
                bMin = 0;
        }
        GlobalUnlock(hPatchMap);

        switch (wParam) {

        case SB_LINEDOWN :
                if (bPatch-- == bMin)
                        bPatch = bMax;
                break;

        case SB_LINEUP :
                if (bPatch++ == bMax)
                        bPatch = bMin;
                break;

        default:
                break;
        }
        SetDlgItemInt(hWnd, uId, bPatch, FALSE);
} /* PatchArrowScroll */

/*
 * PATCHWINDOWSCROLL
 */
static
VOID PASCAL PatchWindowScroll(  HWND    hDlg,
                                WPARAM  wParam,
                                int     iPos )
{
        HDC     hDC;
        RECT    rc;
        int     iVertInc;
        BOOL    fWillBeVisible;         // will it be visible after scroll?

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


        iVertInc = max(-iVertPos, min(iVertInc, iVertMax - iVertPos));

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

                // Scroll to the correct position

                SetScrollPos(hScroll, SB_CTL, iVertPos, TRUE);

                hDC = GetDC(hWnd);
                ScrollDC(hDC, 0L, -yChar * iVertInc, &rcBox, &rcBox, NULL, &rc);
                ReleaseDC(hWnd, hDC);

                if (!fHidden)
                        PatchActiveLine(PAL_HIDE, SWP_NOREDRAW);

                if (fWillBeVisible) {
                        PatchSetFocus(hDlg, 0L, 0L);
                        PatchActiveLine(PAL_SHOW, SWP_NOREDRAW);
                }
                InvalidateRect(hWnd, &rc, TRUE);
                UpdateWindow(hWnd);
        }
} /* PatchWindowScroll */

/*
 * PatchEditMsg
 *
 * This function deals with EN_UPDATE and EN_ACTIVATE messages sent
 * to the patch number and volume percent edit controls through the
 * WM_COMMAND message.
 */
static
VOID PASCAL PatchEditMsg( UINT id, WORD NotifCode )
{
        PATCHMAP FAR*  lpPatch;
        UINT
                uVal,                   // value of control
                uMin,                   // min value allowed
                uMax;                   // max value allowed
        BOOL    bTranslate;

        if (NotifCode != EN_UPDATE && NotifCode != EN_ACTIVATE)
                return;

        lpPatch = (PATCHMAP FAR*)GlobalLock(hPatchMap);

        switch (NotifCode) {

        case EN_UPDATE :
                if (id == ID_PATCHNUMEDIT) {
                        uMax = 127 + iPatchBase;// MMAP_MAXPATCHES
                        uMin = iPatchBase;
                }
                else {
                        uMax = 200;             // MMAP_MAXVOLUME
                        uMin = 0;
                }

                uVal = (UINT)GetDlgItemInt(hWnd, id, &bTranslate, TRUE);

                if ((int)uVal < (int)uMin) {
                        uVal = uMin;
                        SetDlgItemInt(hWnd, id, uVal, FALSE);
                } else if (uVal > uMax) {
                        uVal = uMax;
                        SetDlgItemInt(hWnd, id, uVal, FALSE);
                } else {
                        if (id == ID_PATCHNUMEDIT) {
                                uVal -= iPatchBase;
                                if (uVal != lpPatch->keymaps[iVertPos + iCurPos].bDestination) {
                                        lpPatch->keymaps[iVertPos + iCurPos].bDestination = (BYTE)uVal;
                                        Modify(TRUE);
                                }
                        } else if (uVal != lpPatch->keymaps[iVertPos + iCurPos].bVolume) {
                                lpPatch->keymaps[iVertPos + iCurPos].bVolume = (BYTE)uVal;
                                Modify(TRUE);
                        }
                }
                break;

        case EN_ACTIVATE :
                if (id == ID_PATCHNUMEDIT)
                        SetDlgItemInt(hWnd, id, lpPatch->keymaps[iVertPos + iCurPos].bDestination + iPatchBase, FALSE);
                else
                        SetDlgItemInt(hWnd, id, lpPatch->keymaps[iVertPos + iCurPos].bVolume, FALSE);
                break;

        default :
                break;

        }

        GlobalUnlock(hPatchMap);
} /* PatchEditMsg */

/*
 * PatchComboMsg
 *
 * This function deals with the CBN_ACTIVATE and CBN_SELCHANGE messages sent
 * to the keymap name combo box through the WM_COMMAND message.
 */
static
VOID PASCAL PatchComboMsg(HWND hDlg, WORD NotifCode)
{
        PATCHMAP FAR*  lpPatch;
        char    szBuf [MMAP_MAXNAME];

        if (NotifCode != CBN_ACTIVATE && NotifCode != CBN_SELCHANGE)
                return;

        lpPatch = (PATCHMAP FAR*)GlobalLock(hPatchMap);

        switch (NotifCode) {

        case CBN_ACTIVATE :
                SendMessage(hCombo, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(lpPatch->keymaps[iVertPos + iCurPos].aszKeyMapName));
                break;

        case CBN_SELCHANGE:
                GetWindowText(hCombo, szBuf, MMAP_MAXNAME);
                if (!lstrcmpi(szBuf, lpPatch->keymaps[iVertPos + iCurPos].aszKeyMapName))
                        break;
                lstrcpy(lpPatch->keymaps[iVertPos + iCurPos].aszKeyMapName, szBuf);
                Modify(TRUE);
                break;

        default :
                break;
        }

        GlobalUnlock(hPatchMap);
} /* PatchComboMsg */

/*
 * PATCHBUTTONDOWN
 */
static
VOID PASCAL PatchButtonDown(HWND hDlg, LONG lParam)
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
                SendMessage(hCombo, CB_SHOWDROPDOWN, (WPARAM)FALSE, (LPARAM)0);
                UpdateWindow(hWnd);
        }
        else
                uFlags = PSF_SHOWIFHIDDEN;

        iCurPos = iPos;
        PatchSetFocus(hDlg, uFlags, x);
} /* PatchButtonDown */

/*
 * PATCHSETFOCUS
 */
static
VOID PASCAL PatchSetFocus(      HWND    hDlg,
                                UINT    uFlags,
                                int     xPos )
{
        RECT    rc;
        int     yPos = rcBox.top + iCurPos * yChar;

        PatchEditMsg(ID_PATCHNUMEDIT, EN_ACTIVATE);
        PatchEditMsg(ID_PATCHVOLEDIT, EN_ACTIVATE);
        PatchComboMsg(hDlg, CBN_ACTIVATE);

        GetWindowRect(hEdit, &rc);

        SetWindowPos(hEdit, NULL, rgxPos [2], yPos, 0L, 0L,
                SWP_NOZORDER | SWP_NOSIZE);
        SetWindowPos(hArrow, NULL, rgxPos [2] + xArrowOffset, yPos, 0L,
                0L, SWP_NOZORDER | SWP_NOSIZE);
        SetWindowPos(hVolEdit, NULL, rgxPos [3], yPos, 0L, 0L,
                SWP_NOZORDER | SWP_NOSIZE);
        SetWindowPos(hVolArrow, NULL, rgxPos [3] + xVolArrowOffset, yPos,
                0L, 0L, SWP_NOZORDER | SWP_NOSIZE);
        SetWindowPos(hCombo, NULL, rgxPos [4], yPos, 0L, 0L,
                SWP_NOZORDER | SWP_NOSIZE);

        if (fHidden && uFlags & PSF_SHOWIFHIDDEN) {
                PatchActiveLine(PAL_SHOW, 0L);
                UpdateWindow(hEdit);
                UpdateWindow(hArrow);
                UpdateWindow(hVolEdit);
                UpdateWindow(hVolArrow);
                UpdateWindow(hCombo);
        }

        if (uFlags & PSF_REDRAW && rc.right) {
                ScreenToClient(hWnd, (LPPOINT)&rc);
                ScreenToClient(hWnd, (LPPOINT)&rc + 1);
                rc.right = rcBox.right + 1;
                InvalidateRect(hWnd, &rc, FALSE);
                UpdateWindow(hWnd);
        }

        if (xPos < rgxPos [3]) {
                if (!fHidden)
                        SetFocus(hEdit);
#if defined(WIN16)
                SendMessage(hEdit, EM_SETSEL, (WPARAM)NULL, MAKELPARAM(0, 32767));
#else
                SendMessage(hEdit, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
#endif //WIN16
        }
        else if (xPos < rgxPos [4]) {
                if (!fHidden)
                        SetFocus(hVolEdit);
#if defined(WIN16)
                SendMessage(hVolEdit, EM_SETSEL, (WPARAM)NULL, MAKELPARAM(0, 32767));
#else
                SendMessage(hVolEdit, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
#endif //WIN16
        }
        else if (xPos < rgxPos [5] && !fHidden)
                SetFocus(hCombo);
} /* PatchSetFocus */

/*
 * PATCHACTIVELINE
 */
static
VOID PASCAL PatchActiveLine(    UINT    uCase,
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
        SetWindowPos(hVolEdit, NULL, 0L, 0L, 0L, 0L, uFlags);
        SetWindowPos(hVolArrow, NULL, 0L, 0L, 0L, 0L, uFlags);
        SetWindowPos(hCombo, NULL, 0L, 0L, 0L, 0L, uFlags);

        if (uCase == PAL_SHOW && !fHidden)
                SetFocus(hEdit);
} /* PatchActiveLine */

/*
 * PATCHSAVE
 */
static  int PASCAL PatchSave(
        HWND    hDlg,
        BOOL bQuery)
{
        PATCHMAP FAR*  lpPatch;
        MMAPERR mmaperr;
        int     iRet = 0;            // a value other than IDCANCEL or IDYES

        if (bQuery) {
                iRet = QuerySave();
                if (iRet != IDYES)
                        return iRet;
        }
        lpPatch = (PATCHMAP FAR*)GlobalLock(hPatchMap);
        mmaperr = mapWrite(MMAP_PATCH, lpPatch);
        GlobalUnlock(hPatchMap);
        if (mmaperr != MMAPERR_SUCCESS) {
                VShowError(hDlg, mmaperr);
                return IDCANCEL;
        }
        Modify(FALSE);
        if (fNew)
                fNew = FALSE;
        return iRet;
} /* PatchSave */
