/*
 * SETUP.C
 *
 * Copyright (C) 1990 Microsoft Corporation.
 *
 * Edit setups dialog box and support functions.
 */
/* Revision history:
   March 92 Ported to 16/32 common code by Laurie Griffiths (LaurieGr)
*/

#include "preclude.h"
#include <windows.h>
#include <string.h>
#include <mmsystem.h>
#include <port1632.h>
#include "hack.h"
#include "midimap.h"
#include "midi.h"
#include <cphelp.h>
#include "extern.h"
#include "stdio.h"
#include "stdarg.h"

#if defined(WIN32)
#define _based(x)
#endif //WIN32

BOOL FAR PASCAL _loadds FSetupEnumPortsFunc(LPSTR, LPSTR, UINT, HWND, LPSTR);

#define PAL_SHOW                0       // show active line
#define PAL_HIDE                1       // hide active line

#define PSF_REDRAW              0x0001  // redraw where line used to be
#define PSF_SHOWIFHIDDEN        0x0002  // show line if hidden

#define BTN3S_UNCHECK           0       // uncheck a 3-state button
#define BTN3S_CHECK             1       // check a 3-state button
#define BTN3S_GRAY              2       // gray a 3-state button

#define DEF_SETUP_ROWS          16      // number of default setup rows

static HGLOBAL  hSetup;                 // Setup handle
static HWND     hPortList,              // invalid port list box ctrl handle
                hPortCombo,             // port combo-box ctrl handle
                hPatchCombo;            // patch combo-box ctrl handle
static UINT     nPorts;                 // number of ports available
static int      nPatches,               // number of user-defined patchmaps
                iOldPos,                // old position of active edit line
                xArrowOffset;           // arrow control positional offset
static  SZCODE aszNull[] = "";
static  SZCODE aszSetupNumFormat[] = "%3d";

#if DBG
void FAR cdecl dprintf(LPSTR szFormat, ...)
{

    char ach[128];
   va_list va;
    int  s,d;

   va_start(va, szFormat);
    s = vsprintf (ach, szFormat, va);
    va_end(va);
#if 0
    lstrcat(ach,"\n");
    s++;
#endif
    for (d=sizeof(ach)-1; s>=0; s--)
    {
        if ((ach[d--] = ach[s]) == '\n')
            ach[d--] = '\r';
    }

    OutputDebugString("MIDI: ");
    OutputDebugString(ach+d+1);
}
#else
#define dprintf  if (0) ((int (*)(char *, ...)) 0)
#endif //DBG

static  BOOL NEAR PASCAL FHasInvalidPort(
        void)
{
        SETUP FAR* lpSetup;
        WORD    wNumDevs;
        int     i;

        wNumDevs = (WORD)midiOutGetNumDevs();
        // dprintf("numDevs = %d\n", wNumDevs);
        lpSetup = (SETUP FAR*)GlobalLock(hSetup);
        for (i = 0; i < 16; i++)
        {   // dprintf("device id[%d]=%d\n",i,lpSetup->channels[i].wDeviceID);
            if ((lpSetup->channels[i].wDeviceID != LOWORD(MIDI_MAPPER)) &&
                (lpSetup->channels[i].wDeviceID >= wNumDevs))
                break;
        }

        GlobalUnlock(hSetup);
        return (i < 16);
} /* FHasInvalidPort */

static  int PASCAL ISetupSave(
        HWND    hdlg,
        BOOL    bQuery)
{
        SETUP FAR* lpSetup;
        MMAPERR mmaperr;
        int     iRet = 0;           // choose a value not IDCANCEL or IDYES

        if (bQuery)
                if ((iRet = QuerySave()) != IDYES)
                        return iRet;
        if (FHasInvalidPort())
                if (!InvalidPortMsgBox(hdlg))
                        return IDCANCEL;
        lpSetup = (SETUP FAR*)GlobalLock(hSetup);
        mmaperr = mapWrite(MMAP_SETUP, lpSetup);
        GlobalUnlock(hSetup);
        if (mmaperr != MMAPERR_SUCCESS) {
                VShowError(hdlg, mmaperr);
                return IDCANCEL;
        }
        Modify(FALSE);
        if (fNew)
                fNew = FALSE;
        return iRet;
} /* ISetupSave */

/*
 * VSetupEditMsg
 *
 * This function deals with EN_UPDATE and EN_ACTIVATE messages sent
 * to the channel number edit control through the WM_COMMAND message.
 */

static  void PASCAL VSetupEditMsg(
        HWND    hdlg,
        WORD    NotifCode)
{
        SETUP FAR*      lpSetup;
        int     i;
        LPWORD  lpwChan;
        WORD    wChan;
        BOOL    bTranslate;

        switch (NotifCode) {
        case EN_UPDATE:
                lpSetup = (SETUP FAR*)GlobalLock(hSetup);
                lpwChan = &lpSetup->channels[iVertPos + iCurPos].wChannel;
                i = GetDlgItemInt(hdlg, ID_SETUPEDIT, &bTranslate, FALSE);
                if (i > 0 && i <= 16) {
                        if (*lpwChan != (BYTE)(i - 1)) {
                                *lpwChan = (BYTE)(i - 1);
                                Modify(TRUE);
                        }
                } else {
                        char    aszMessage[256];
                        char    aszTitle[32];

                        // may want to experiment with EM_UNDO here.
                        LoadString(hLibInst, IDS_INVALIDDESTINATION, aszMessage, sizeof(aszMessage));
                        LoadString(hLibInst, IDS_USERERROR, aszTitle, sizeof(aszTitle));
                        MessageBox(hdlg, aszMessage, aszTitle, MB_ICONEXCLAMATION | MB_OK);
                        SetDlgItemInt(hdlg, ID_SETUPEDIT,
                                *lpwChan + 1, FALSE);
                }
                GlobalUnlock(hSetup);
                break;
        case EN_ACTIVATE:
                lpSetup = (SETUP FAR*)GlobalLock(hSetup);
                wChan = lpSetup->channels[iVertPos + iCurPos].wChannel;
                GlobalUnlock(hSetup);
                SetDlgItemInt(hdlg, ID_SETUPEDIT, wChan + 1, FALSE);
                break;
        }
} /* VSetupEditMsg */

/*
 * VSetupActiveChan
 *
 * This function controls the checked/unchecked/grayed state of the 'Active'
 * button for a specific channel.  If the function is being grayed, it will
 * be disabled as well.
 */

static  void NEAR PASCAL VSetupActiveChan(
        int     iPos,
        LPDWORD lpdwFlags,
        UINT    uCheck)
{
        HWND    hCheck;
        BOOL    fEnable;

        if (lpdwFlags)
                if (uCheck == BTN3S_CHECK)
                        *lpdwFlags |= MMAP_ACTIVE;
                else
                        *lpdwFlags &= ~MMAP_ACTIVE;
        hCheck = GetDlgItem(hWnd, ID_SETUPCHECK + iPos);
        if ((UINT)SendMessage(hCheck, BM_GETCHECK, (WPARAM)0, (LPARAM)0) != uCheck)
                SendMessage(hCheck, BM_SETCHECK, (WPARAM)uCheck, (LPARAM)0);
        fEnable = (uCheck != BTN3S_GRAY);
        if (fEnable != IsWindowEnabled(hCheck))
                EnableWindow(hCheck, fEnable);
} /* VSetupActiveChan */

/*
 * VSetupComboMsg
 *
 * This function deals with the CBN_ACTIVATE and CBN_SELCHANGE messages sent
 * to the port or patch combo boxes through the WM_COMMAND message.
 */

static  void NEAR PASCAL VSetupComboMsg(
        HWND    hdlg,
        UINT    id,
        WORD    NotifCode)
{
        HWND    hCombo = GetDlgItem(hdlg, id);
        SETUP FAR* lpSetup;
        CHANNEL FAR* lpChannel;
        UINT    uIdx;
        char    szBuf[MAXPNAMELEN];

        if (NotifCode != CBN_ACTIVATE && NotifCode != CBN_SELCHANGE)
                return;
        lpSetup = (SETUP FAR*)GlobalLock(hSetup);
        lpChannel = &lpSetup->channels[iVertPos + iCurPos];
        switch (NotifCode) {
        case CBN_ACTIVATE:
                uIdx = MMAP_ID_NOPORT;
                // if its a port combo box message
                if (id == ID_SETUPPORTCOMBO)
                        if (lpChannel->wDeviceID != MMAP_ID_NOPORT)
                                uIdx = lpChannel->wDeviceID;
                        else
                                uIdx = nPorts;
                // otherwise its a patch combo box message
                else if (lpChannel->dFlags & MMAP_PATCHMAP)
                        uIdx = ComboLookup(hCombo, lpChannel->aszPatchName);
                else
                        uIdx = nPatches;
                SendMessage(hCombo, CB_SETCURSEL, (WPARAM)uIdx, (LPARAM)0);
                break;
        case CBN_SELCHANGE:
                GetWindowText(hCombo, szBuf, MAXPNAMELEN);
                // if we're dealing with a port combo message
                if (id == ID_SETUPPORTCOMBO) {
                        // get the index of the newly selected port
                        uIdx = (UINT)SendMessage(hCombo,
                                CB_GETCURSEL, (WPARAM)NULL, (LPARAM)0);
                        // if it's the same as old index, we don't care
                        if (uIdx == lpChannel->wDeviceID)
                                break;
                        // if it's the last port index, it's the[none] entry
                        if (uIdx == (UINT)nPorts) {
                                // set id to bogus port value
                                lpChannel->wDeviceID = MMAP_ID_NOPORT;
                                // deactivate the channel
                                VSetupActiveChan(iCurPos, &lpChannel->dFlags,
                                        BTN3S_GRAY);
                        } else {
                                // ok so it's not the[none] entry
                                // if it used to be[none], activate channel
                                if (lpChannel->wDeviceID == MMAP_ID_NOPORT)
                                        VSetupActiveChan(iCurPos,
                                                &lpChannel->dFlags,
                                                BTN3S_CHECK);
                                // set the id to the new index
                                lpChannel->wDeviceID = (WORD)uIdx;
                        }
                        Modify(TRUE);
                } else {
                        if (!lstrcmpi(szBuf, lpChannel->aszPatchName))
                                break;
                        lstrcpy(lpChannel->aszPatchName, szBuf);
                        if (!lstrcmpi(szBuf, szNone))
                                lpChannel->dFlags &= ~MMAP_PATCHMAP;
                        else
                                lpChannel->dFlags |= MMAP_PATCHMAP;
                        Modify(TRUE);
                }
                break;
        }
        GlobalUnlock(hSetup);
} /* VSetupComboMsg */

static  void NEAR PASCAL VSetupActiveLine(
        UINT    uCase,
        UINT    uFlags)
{
        static const UINT uDefFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER;

        switch (uCase) {
        case PAL_SHOW:
                if (!fHidden)
                        return;
                uFlags |= SWP_SHOWWINDOW;
                fHidden = FALSE;
                break;
        case PAL_HIDE:
                if (fHidden)
                        return;
                uFlags |= SWP_HIDEWINDOW;
                fHidden = TRUE;
                break;
        }
        uFlags |= uDefFlags;
        SetWindowPos(hEdit, NULL, 0L, 0L, 0L, 0L, uFlags);
        SetWindowPos(hArrow, NULL, 0L, 0L, 0L, 0L, uFlags);
        SetWindowPos(hPortCombo, NULL, 0L, 0L, 0L, 0L, uFlags);
        SetWindowPos(hPatchCombo, NULL, 0L, 0L, 0L, 0L, uFlags);
        if (uCase == PAL_SHOW)
                SetFocus(hEdit);
} /* VSetupActiveLine */

static  void NEAR PASCAL VSetupSetFocus(
        HWND    hdlg,
        UINT    uFlags,
        int     xPos)
{
        HWND    hOldCheck;
        HWND    hNewCheck;
        RECT    rc;
        DWORD   dwFlags;
        int     yPos = rcBox.top + iCurPos * yChar;

        // change tabstop flag on new checkbox
        hNewCheck = GetDlgItem(hdlg, ID_SETUPCHECK + iCurPos);
        dwFlags = (DWORD)GetWindowLong(hNewCheck, GWL_STYLE);
        dwFlags |= WS_TABSTOP;
        SetWindowLong(hNewCheck, GWL_STYLE, (LPARAM)dwFlags);
        // set mnemonic on new window
        SetWindowText(hNewCheck, aszSourceMnumonic);
        // take the tabstop away from the old checkbox if it exists
        if (iOldPos > -1) {
                hOldCheck = GetDlgItem(hdlg, ID_SETUPCHECK + iOldPos);
                dwFlags = (DWORD)GetWindowLong(hOldCheck, GWL_STYLE);
                dwFlags &= ~(DWORD)WS_TABSTOP;
                SetWindowLong(hOldCheck, GWL_STYLE, (LPARAM)dwFlags);
                // take away mnemonic from old window
                SetWindowText(hOldCheck, aszNull);
        }
        iOldPos = iCurPos;
        GetWindowRect(hEdit, &rc);
        SetWindowPos(hEdit, NULL, rgxPos[1], yPos, 0L, 0L,
                SWP_NOZORDER | SWP_NOSIZE);
        VSetupEditMsg(hdlg, EN_ACTIVATE);
//      UpdateWindow(hEdit);
        SetWindowPos(hArrow, NULL, rgxPos[1] + xArrowOffset, yPos, 0L,
                0L, SWP_NOZORDER | SWP_NOSIZE);
        SetWindowPos(hPortCombo, NULL, rgxPos[2], yPos, 0L, 0L,
                SWP_NOZORDER | SWP_NOSIZE);
        VSetupComboMsg(hdlg, ID_SETUPPORTCOMBO, CBN_ACTIVATE);
        SetWindowPos(hPatchCombo, NULL, rgxPos[3], yPos, 0L, 0L,
                SWP_NOZORDER | SWP_NOSIZE);
        VSetupComboMsg(hdlg, ID_SETUPPATCHCOMBO, CBN_ACTIVATE);
        if ((fHidden) && (uFlags & PSF_SHOWIFHIDDEN)) {
                VSetupActiveLine(PAL_SHOW, 0L);
                UpdateWindow(hArrow);
                UpdateWindow(hPortCombo);
                UpdateWindow(hPatchCombo);
        }
//      ValidateRect(hdlg, NULL);
        if (uFlags & PSF_REDRAW && rc.right) {
                ScreenToClient(hdlg, (LPPOINT)&rc);
                ScreenToClient(hdlg, (LPPOINT)&rc + 1);
                rc.right = rcBox.right + 1;
                InvalidateRect(hdlg, &rc, FALSE);
                UpdateWindow(hdlg);
        }
        if (!xPos)
                return;
        if (xPos < rgxPos[2]) {
                SetFocus(hEdit);
#if defined(WIN16)
                SendMessage(hEdit, EM_SETSEL, (WPARAM)NULL, MAKELPARAM(0, 32767));
#else
                SendMessage(hEdit, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
#endif //WIN16
        } else if (xPos < rgxPos[3])
                SetFocus(hPortCombo);
        else if (xPos < rgxPos[4])
                SetFocus(hPatchCombo);
} /* VSetupSetFocus */

/*
 * VSetupSize
 */
static
void PASCAL VSetupSize(
        HWND    hdlg,
        BOOL    fMaximize)
{
        RECT    rcOK;
        RECT    rc3S;
        RECT    rcWnd;
        LONG    lDBU;
        UINT    xBU;
        UINT    yBU;
        int     xButton;
        int     xCenter;
        int     yTopMar;
        int     yBotMar;
        int     yLeftOver;
        int     yMiniBotMar;
        int     i;

        lDBU = GetDialogBaseUnits();
        xBU = LOWORD(lDBU);
        yBU = HIWORD(lDBU);
        // get the rectangle of the OK button
        GetClientRect(GetDlgItem(hdlg, IDOK), &rcOK);
        // get x-extent of button
        xButton = rcOK.right - rcOK.left;
        // top margin is 2 characters
        yTopMar = (16 * yBU) / 8 - 6; // cookie land
        // bottom margin is 2 * minimum bottom margin dialog units +
        // height of button in pixels
        yBotMar = (VERTMARGIN * 2 *yBU) / 8 + rcOK.bottom - rcOK.top;
        if (fMaximize) {
                // get the rectangle of a 3-state button
                GetClientRect(GetDlgItem(hdlg, ID_SETUPCHECK), &rc3S);
                SetWindowPos(hdlg, NULL, 0L, 0L,
                        rcBox.right - rcBox.left +
                        (rc3S.right - rc3S.left) +
                        (HORZMARGIN * 3 * xBU) / 4 +
                        (GetSystemMetrics(SM_CXDLGFRAME) + 1) * 2,
                        (DEF_SETUP_ROWS * 10 * yBU) / 8 +
                        yTopMar + yBotMar +
                        GetSystemMetrics(SM_CYCAPTION) +
                        (GetSystemMetrics(SM_CYDLGFRAME) + 1) * 2,
                        SWP_NOZORDER | SWP_NOMOVE);
        }
        // get the x and y extents of the client rectangle
        GetClientRect(hdlg, &rcWnd);
        xClient = rcWnd.right - rcWnd.left;
        yClient = rcWnd.bottom - rcWnd.top;
        // yChar is the height of one row in pixels - 1
        yChar = (10 * yBU) / 8 - 1;
        // xChar is the average width of a character
        xChar = xBU;
        // yBox is the room we actually have to display setup rows
        yBox = yClient - yTopMar - yBotMar;
        // nLines is the number of setup rows we can display
        nLines = min(16, yBox / yChar);
        // yLeftOver is how many pixels are left over
        yLeftOver = yBox - nLines * yChar;
        // add half the leftovers to the top margin
        yTopMar += yLeftOver / 2;
        // calculate scroll bar maximum and position
        iVertMax = max(0, 16 - nLines);
        iVertPos = min(iVertMax, iVertPos);
        // rcBox is the box of rows and columns inside the client area
        SetRect(&rcBox,
                rcBox.left,
                yTopMar,
                rcBox.right,
                yTopMar + nLines * yChar);
        // xCenter is used to center the OK and CANCEL buttons horizontally
        xCenter = (rcBox.right - rcBox.left - xButton * 3) / 4;
        // yMiniBotMar is the spacing above and below the button
        yMiniBotMar = (VERTMARGIN * yBU) / 8 + yLeftOver / 4;
        SetWindowPos(GetDlgItem(hdlg, IDOK), NULL, rcBox.left + xCenter,
                rcBox.bottom + yMiniBotMar, 0L, 0L,
                SWP_NOSIZE | SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hdlg, IDCANCEL), NULL,
                rcBox.left + xButton + xCenter * 2,
                rcBox.bottom + yMiniBotMar, 0L, 0L,
                SWP_NOSIZE | SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hdlg, IDH_DLG_MIDI_SETUPEDIT), NULL,
                rcBox.left + xButton * 2 + xCenter * 3,
                rcBox.bottom + yMiniBotMar, 0L, 0L,
                SWP_NOSIZE | SWP_NOZORDER);
        // this loop could be optimized
        // then again, so could the u.s. judicial system
        for (i = 0; i < 16; i++, yTopMar += yChar) {
                HWND    hCheck;

                hCheck = GetDlgItem(hdlg, ID_SETUPCHECK + i);
                if (i < nLines) {
                        SetWindowPos(hCheck, NULL,
                                rcBox.right + (HORZMARGIN * xBU) / 4,
                                yTopMar + 3, 0L, 0L,
                                SWP_NOSIZE | SWP_NOZORDER);
                        ShowWindow(hCheck, SW_SHOWNORMAL);
                } else
                        ShowWindow(hCheck, SW_HIDE);
        }
        if (iCurPos >= 0 && iCurPos < nLines)
                VSetupSetFocus(hdlg, PSF_SHOWIFHIDDEN, 1);
        else
                VSetupActiveLine(PAL_HIDE, SWP_NOREDRAW);
} /* VSetupSize */

static  MMAPERR PASCAL MmaperrInitNew(void)
{
        SETUP FAR*      lpSetup;
        WORD     wChan;

        if ((hSetup = GlobalAlloc(GHND, (DWORD)sizeof(SETUP))) == NULL)
                return MMAPERR_MEMORY;
        lpSetup = (SETUP FAR*)GlobalLock(hSetup);
        lstrcpy(lpSetup->aszSetupName, szCurrent);
        lstrcpy(lpSetup->aszSetupDescription, szCurDesc);
        for (wChan = 0; wChan < 16; wChan++) {
                lpSetup->channels[wChan].wChannel = wChan;
                lpSetup->channels[wChan].wDeviceID = MMAP_ID_NOPORT;
                lstrcpy(lpSetup->channels[wChan].aszPatchName, szNone);
        }
        GlobalUnlock(hSetup);
        return MMAPERR_SUCCESS;
} /* MmaperrInitNew */

/*
 * VSetupEnumPorts
 *
 * Enumerate available port names and throw them in a combo box.
 */

static  void PASCAL VSetupEnumPorts(
        void)
{
        MIDIOUTCAPS moCaps;
        UINT     i;

        nPorts = midiOutGetNumDevs();
        for (i = 0; i < nPorts; i++) {
                midiOutGetDevCaps(i, &moCaps, sizeof(MIDIOUTCAPS));
                SendMessage(hPortCombo, CB_ADDSTRING, (WPARAM)NULL,
                        (LPARAM)(LPSTR)moCaps.szPname);
        }
} /* VSetupEnumPorts */

/*
 * MmaperrEnumPatches
 *
 * This function calls mapEnumerate to enumerate all the user-defined
 * patchmaps into a combo box.
 */
static  MMAPERR NEAR PASCAL MmaperrEnumPatches(void)
{
        MMAPERR mmaperr;

        mmaperr = mapEnumerate(MMAP_PATCH, EnumFunc, MMENUM_BASIC, hPatchCombo, NULL);
        if (mmaperr != MMAPERR_SUCCESS)
                return mmaperr;
        nPatches = (int)(LONG)SendMessage(hPatchCombo, CB_GETCOUNT, (WPARAM)NULL, (LPARAM)0);
        SendMessage(hPatchCombo, CB_ADDSTRING, (WPARAM)NULL, (LPARAM)(LPSTR)szNone);
        return MMAPERR_SUCCESS;
} /* MmaperrEnumPatches */

static  BOOL NEAR PASCAL FInitSetup(
        HWND    hdlg)
{
        SETUP FAR* lpSetup;
        LONG    lDBU;
        UINT    xBU;
        UINT    yBU;
        UINT    xWidth; // width of current column
        UINT    xEdit;  // width of edit controls
        UINT    yEdit;  // height of edit/arrow controls
        UINT    xCombo; // width of combo boxes
        UINT    yCombo; // height of combo boxes
        UINT    wCheck;
        char    szCaption[80];
        char    szCaptionFormat[80];
        MMAPERR mmaperr;
        int     i;

        if (!fNew) {
                if ((hSetup = GlobalAlloc(GHND, sizeof(SETUP))) == NULL) {
                        mmaperr = MMAPERR_MEMORY;
exit00:                 VShowError(hdlg, mmaperr);
                        return FALSE;
                }
        } else if ((mmaperr = MmaperrInitNew()) != MMAPERR_SUCCESS)
                goto exit00;
        fHidden = FALSE;
        iVertPos = 0;
        iCurPos = 0;
        nLines = 0; // necessary?
        nPatches = 0;
        nPorts = 0;
        hPortList = GetDlgItem(hdlg, ID_SETUPPORTLIST);
        hEdit = GetDlgItem(hdlg, ID_SETUPEDIT);
        hArrow = GetDlgItem(hdlg, ID_SETUPARROW);
        hPortCombo = GetDlgItem(hdlg, ID_SETUPPORTCOMBO);
        hPatchCombo = GetDlgItem(hdlg, ID_SETUPPATCHCOMBO);
        if (fReadOnly)
        {
                EnableWindow(GetDlgItem(hdlg,IDOK),FALSE);
                SendMessage(hdlg, DM_SETDEFID, (WPARAM)IDCANCEL, (LPARAM)0);
        }
        lDBU = GetDialogBaseUnits();
        xBU = LOWORD(lDBU);
        yBU = HIWORD(lDBU);
        xEdit = (40 * xBU) / 4;         // 10 chars wide
        yEdit = (10 * yBU) / 8;         // 10 is a magic cookie
        xCombo = (64 * xBU) / 4;        // 16 characters long
        yCombo = (46 * yBU) / 8;        // 46 is a magic cookie
        rcBox.left = (HORZMARGIN * xBU) / 4;
        rcBox.right = rcBox.left;
        rgxPos[0] = rcBox.right;
        rcBox.right += xEdit;
        rgxPos[1] = rcBox.right;
        xWidth = xEdit;
        SetWindowPos(hEdit, NULL, 0L, 0L, xWidth, yEdit,
                SWP_NOZORDER | SWP_NOMOVE);
        // set global arrow control offset to proper position
        rcBox.right += xArrowOffset = xWidth - 1;
        // width of dst channel arrow control
        xWidth = (10 * xBU) / 4;
        SetWindowPos(hArrow, NULL, 0L, 0L, xWidth, yEdit,
                SWP_NOZORDER | SWP_NOMOVE);
        rcBox.right += xWidth - 1;
        rgxPos[2] = rcBox.right;
        xWidth = (80 * xBU) / 4;        // 20 characters long
        SetWindowPos(hPortCombo, NULL, 0L, 0L, xWidth,
                yCombo, SWP_NOZORDER | SWP_NOMOVE);
        rcBox.right += xWidth - 1;
        rgxPos[3] = rcBox.right;
        xWidth = xCombo;
        SetWindowPos(hPatchCombo, NULL, 0L, 0L, xWidth,
                yCombo, SWP_NOZORDER | SWP_NOMOVE);
        rcBox.right += xWidth - 1;
        rgxPos[4] = rcBox.right;
        if (!nPorts)
                VSetupEnumPorts();
        lpSetup = (SETUP FAR*)GlobalLock(hSetup);
        if (!nPatches)
                if ((mmaperr = MmaperrEnumPatches()) != MMAPERR_SUCCESS) {
exit01:                 GlobalUnlock(hSetup);
                        GlobalFree(hSetup);
                        goto exit00;
                }
        if (!fNew) {
                mmaperr = mapReadSetup(szCurrent, lpSetup);
                if (mmaperr == MMAPERR_INVALIDPORT) {
                        if (!InvalidPortMsgBox(hdlg)) {
                                GlobalUnlock(hSetup);
                                GlobalFree(hSetup);
                                return FALSE;
                        }
                        mmaperr = mapEnumerate( MMAP_PORTS
                                              , FSetupEnumPortsFunc
                                              , MMENUM_BASIC
                                              , NULL
                                              , (LPSTR)szCurrent
                                              );
                }
                if (mmaperr != MMAPERR_SUCCESS){
                        goto exit01;
                }
                if (lstrcmp(lpSetup->aszSetupDescription, szCurDesc)) {
                        lstrcpy(lpSetup->aszSetupDescription, szCurDesc);
                        fNew = TRUE;
                }
        }
        SendMessage(hPortCombo, CB_ADDSTRING, (WPARAM)NULL, (LPARAM)(LPSTR)szNone);
        for (i = 0; i < 16; i++) {
                // check a 3-state button if the channel is active, or gray
                // it if the channel is not mapped to a port
                if (lpSetup->channels[i].wDeviceID == MMAP_ID_NOPORT)
                        wCheck = BTN3S_GRAY;
                else if (lpSetup->channels[i].dFlags & MMAP_ACTIVE)
                        wCheck = BTN3S_CHECK;
                else
                        continue;
                VSetupActiveChan(i, 0L, wCheck);
        }
        GlobalUnlock(hSetup);

        LoadString(hLibInst, IDS_SETUPS ,szCaptionFormat, sizeof(szCaptionFormat));
        wsprintf(szCaption, szCaptionFormat, (LPSTR)szCurrent);
        SetWindowText(hWnd, szCaption);

        SendMessage(GetDlgItem(hdlg, ID_SETUPDESTMNEM),
                WM_SETFONT, (WPARAM)hFont, (LPARAM)0);
        SendMessage(GetDlgItem(hdlg, ID_SETUPPORTMNEM),
                WM_SETFONT, (WPARAM)hFont, (LPARAM)0);
        SendMessage(GetDlgItem(hdlg, ID_SETUPPATCHMNEM),
                WM_SETFONT, (WPARAM)hFont, (LPARAM)0);
        VSetupSize(hdlg, TRUE);
        SetWindowPos(GetDlgItem(hdlg, ID_SETUPDESTMNEM), NULL,
                rgxPos[1], yChar, 0L, 0L, SWP_NOSIZE | SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hdlg, ID_SETUPPORTMNEM), NULL,
                rgxPos[2], yChar, 0L, 0L, SWP_NOSIZE | SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hdlg, ID_SETUPPATCHMNEM), NULL,
                rgxPos[3], yChar, 0L, 0L, SWP_NOSIZE | SWP_NOZORDER);
        Modify(fNew);
        return TRUE;
} /* FInitSetup */

static  void PASCAL VSetupButtonDown(
        HWND    hdlg,
        LONG    lParam)
{
        int     x = LOWORD(lParam);
        int     y = HIWORD(lParam);
        int     iPos;
        UINT    uFlags;

        if (x < rcBox.left || x > rcBox.right)
                return;
        if (y < rcBox.top || y > rcBox.bottom)
                return;
        if ((iPos = min(nLines - 1, (y - rcBox.top) / yChar)) == iCurPos)
                return;
        if (iCurPos >= 0 && iCurPos < nLines) {
                uFlags = PSF_REDRAW;
                SendMessage(hPortCombo, CB_SHOWDROPDOWN, (WPARAM)FALSE, (LPARAM)0);
                SendMessage(hPatchCombo, CB_SHOWDROPDOWN, (WPARAM)FALSE, (LPARAM)0);
                UpdateWindow(hdlg);
        } else
                uFlags = PSF_SHOWIFHIDDEN;
        iCurPos = iPos;
        VSetupSetFocus(hdlg, uFlags, x);
} /* VSetupButtonDown */

static  void PASCAL VSetupPaint(
        HWND    hdlg)
{
        HPEN            hPen;    // previous pen to restore
        SETUP FAR*      lpSetup;
        CHANNEL FAR*    lpChannel;
        PAINTSTRUCT     ps;      // from BeginPaint
        RECT    rcText;
        WORD    wDev;
        int     i;      // loop counter
        int     iVert;
        int     nBegin;    // first row to repaint
        int     nEnd;      // last row to repaint
        int     iLeft;     // left of invalid area
        int     iTop;      // top of invalid area
        int     iBottom;   // bottom of invalid area
        char    szBuf[MAXPNAMELEN];
   // uses GLOBAL hFont - Lord knows why.
   // uses GLOBAL rcBox = clipping rectangle???

        BeginPaint(hdlg, &ps);
        if (!ps.rcPaint.bottom) {    // bottom==0 => area must be empty
exit00:         EndPaint(hdlg, &ps);
                return;
        }
        hPen = SelectObject(ps.hdc, GetStockObject(BLACK_PEN));
        hFont = SelectObject(ps.hdc, hFont);
        SetTextColor(ps.hdc, GetSysColor(COLOR_WINDOWTEXT));
        SetBkMode(ps.hdc, TRANSPARENT);
        if (ps.rcPaint.top < rcBox.top) { // need to paint top headings?
                iVert = yChar;
                TextOut(ps.hdc, 11, iVert, aszSourceChannel, lstrlen(aszSourceChannel));
                SetRect(&rcText, rcBox.right + 2, yChar, 0, 0);
                DrawText(ps.hdc, aszActive, -1, &rcText,
                        DT_LEFT | DT_NOCLIP);
        }
        // calculate top and bottom y coordinates of invalid area
        iTop = max(ps.rcPaint.top, rcBox.top);
        // if top is below the box, forget about painting
        if (iTop > rcBox.bottom) {
exit01:         hFont = SelectObject(ps.hdc, hFont);   // restore
                hPen = SelectObject(ps.hdc, hPen);     // restore
                goto exit00;
        }
        iBottom = min(ps.rcPaint.bottom, rcBox.bottom);
        // calculate left x coordinate of invalid area
        iLeft = max(ps.rcPaint.left, rcBox.left);
        // calculate beginning and ending data row to be repainted
        nBegin = max(0, (iTop - rcBox.top) / yChar);
        nEnd = min(nLines, (iBottom - rcBox.top) / yChar);
        // draw vertical lines of the box
        for (i = 0; i < 5; i++) {
                MMoveTo(ps.hdc, rgxPos[i], iTop);
                LineTo(ps.hdc, rgxPos[i], iBottom + 1);
        }
        // vertical position of first line we have to draw
        iVert = rcBox.top + nBegin * yChar;
        // lock the map
        lpSetup = (SETUP FAR*)GlobalLock(hSetup);
        // set up an optimization pointer
        lpChannel = &lpSetup->channels[iVertPos + nBegin];
        for (i = nBegin; i <= nEnd; i++, iVert += yChar, lpChannel++) {
                MMoveTo(ps.hdc, iLeft, iVert);
                LineTo(ps.hdc, rcBox.right, iVert);
                if (i == nLines)
                        break;
                if (iLeft < rgxPos[1]) {
                        wsprintf(szBuf, aszSetupNumFormat, iVertPos + i + 1);
                        TextOut(ps.hdc, rgxPos[0] + xChar, iVert + 2,
                                szBuf, 3);
                }
                if (iLeft < rgxPos[2]) {
                        wsprintf(szBuf, aszSetupNumFormat, lpChannel->wChannel + 1);
                        TextOut(ps.hdc, rgxPos[1] + 2 * xChar, iVert + 2,
                                szBuf, 3);
                }
                if (i == iCurPos)
                        continue;
                if (iLeft < rgxPos[3]) {
                        *szBuf = 0;
                        wDev = lpChannel->wDeviceID;
                        if (wDev == MMAP_ID_NOPORT)
                                wDev = nPorts;
                        if (wDev <= nPorts) {
                                RECT sTextRect;
                                TEXTMETRIC sTM;

                                SendMessage(hPortCombo, CB_GETLBTEXT,
                                        (WPARAM)wDev, (LPARAM)(LPSTR)szBuf);

                                GetTextMetrics(ps.hdc,(LPTEXTMETRIC)&sTM);
                                SetRect((LPRECT)&sTextRect,rgxPos[2]+2,iVert+2,rgxPos[3]-2,iVert+2+sTM.tmHeight);
                                DrawText(ps.hdc, szBuf, -1,(LPRECT)&sTextRect ,DT_NOPREFIX|DT_LEFT|DT_SINGLELINE);
                        }
                }
                if (iLeft < rgxPos[4]) {
              RECT rc;
         SetRect(&rc, rgxPos[3]+2, iVert, rgxPos[4]-2, iVert+yChar);
              DrawText(ps.hdc,
                  lpChannel->aszPatchName,
                  lstrlen(lpChannel->aszPatchName),
             &rc,
             DT_LEFT|DT_SINGLELINE|DT_VCENTER
             );
         // was:
                        // TextOut(ps.hdc, rgxPos[3] + 2, iVert + 2, lpChannel->aszPatchName,
                        //         lstrlen(lpChannel->aszPatchName));
                }
        }
        GlobalUnlock(hSetup);
        goto exit01;
} /* VSetupPaint */

static  void PASCAL VSetupArrowScroll(
        HWND    hdlg,
        WORD    ScrollCode)
{
        SETUP FAR* lpSetup;
        WORD    wChan;

        lpSetup = (SETUP FAR*)GlobalLock(hSetup);
        wChan = lpSetup->channels[iVertPos + iCurPos].wChannel;
        GlobalUnlock(hSetup);
        switch (ScrollCode) {
        case SB_LINEDOWN:
                if (!wChan--)
                        wChan = 15;
                break;
        case SB_LINEUP:
                if (++wChan > 15)
                        wChan = 0;
                break;
        }
        SetDlgItemInt(hWnd, ID_SETUPEDIT, wChan + 1, FALSE);
} /* VSetupArrowScroll */

/*
 * VSetupCheckMsg
 */
static  void PASCAL VSetupCheckMsg(
        WORD    id,
        WORD    NotifCode)
{
        SETUP FAR*      lpSetup;
        CHANNEL FAR*    lpChannel;

        if (NotifCode != BN_CLICKED)
                return;
        lpSetup = (SETUP FAR*)GlobalLock(hSetup);
        lpChannel = &lpSetup->channels[iVertPos + id - ID_SETUPCHECK];
        if (lpChannel->wDeviceID != MMAP_ID_NOPORT) {
                VSetupActiveChan( id - ID_SETUPCHECK
                                , &lpChannel->dFlags
                                ,   (lpChannel->dFlags & MMAP_ACTIVE)
                                  ? BTN3S_UNCHECK
                                  : BTN3S_CHECK
                                );
                Modify(TRUE);
        }
        GlobalUnlock(hSetup);
} /* VSetupCheckMsg */

BOOL    FAR PASCAL _loadds SetupBox(
        HWND    hdlg,
        UINT    uMessage,
        WPARAM  wParam,
        LPARAM  lParam)
{
        int     iRet;
        HWND    hwndCheckBox;

        switch (uMessage) {
        case WM_INITDIALOG:
                hWnd = hdlg;
                iOldPos = -1;
                SetFocus(GetDlgItem(hdlg, ID_SETUPEDIT));
                if (!FInitSetup(hdlg))
                        EndDialog(hdlg, FALSE);
                SetFocus(hEdit);
      PlaceWindow(hWnd);
                return FALSE;
        case WM_COMMAND:
            {   WORD id = LOWORD(wParam);
#if defined(WIN16)
                WORD NotifCode = HIWORD(lParam);
                HWND hwnd = LOWORD(lParam);
#else
                WORD NotifCode = HIWORD(wParam);
                HWND hwnd = (HWND)lParam;
#endif //WIN16

                switch (id) {
                case  IDH_DLG_MIDI_SETUPEDIT:
                  goto DoHelp;

                case IDOK:
                case IDCANCEL:
                        if (NotifCode != BN_CLICKED)
                                break;
                        if (!fReadOnly && ((id == IDOK) && fModified)) {
                                iRet = ISetupSave(hdlg, TRUE);
                                if (iRet == IDCANCEL)
                                        break;
                                iRet = (iRet == IDYES);
                        } else
                                iRet = FALSE;
                        GlobalFree(hSetup);
                        EndDialog(hdlg, iRet);
                        break;

                case ID_SETUPGHOSTEDITFIRST:

                        /* assume the user back-tabbed before the first
                         * control on the current row, so jump to the
                         * previous row (if iCurPos > 0) or the last row
                         * (if iCurPos == 0)
                         */

                        if (NotifCode != EN_SETFOCUS)
                                break;
                        if (iCurPos < 0)
                                /* do nothing */ ;
                        else
                        if (iCurPos > 0)
                                iCurPos--;
                        else
                                iCurPos = nLines - 1;
                        VSetupSetFocus(hdlg, PSF_REDRAW, 0);
                        hwndCheckBox = GetDlgItem(hdlg, ID_SETUPCHECK + iOldPos);
                        if (IsWindowEnabled(hwndCheckBox))
                                SetFocus(hwndCheckBox);
                        else
                                SetFocus(hPatchCombo);
                        break;

                case ID_SETUPGHOSTEDITLAST:

                        /* assume the user forward-tabbed beyond the last
                         * control on the current row, so jump to the
                         * next row (if iCurPos < nLines - 1) or the first row
                         * (if iCurPos == nLines - 1)
                         */

                        if (NotifCode != EN_SETFOCUS)
                                break;
                        if (iCurPos < 0)
                                /* do nothing */ ;
                        else
                        if (iCurPos < nLines - 1)
                                iCurPos++;
                        else
                                iCurPos = 0;
                        VSetupSetFocus(hdlg, PSF_REDRAW, 0);
                        SetFocus(hEdit);
#if defined(WIN16)
                        SendMessage(hEdit, EM_SETSEL, (WPARAM)NULL, MAKELPARAM(0, 32767));
#else
                        SendMessage(hEdit, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
#endif //WIN16
                        break;

                case ID_SETUPEDIT:
                        VSetupEditMsg(hdlg, NotifCode);
                        break;
                case ID_SETUPPORTCOMBO:
                case ID_SETUPPATCHCOMBO:
                        VSetupComboMsg(hdlg, id, NotifCode);
                        break;
                default:
                        if ((id >= ID_SETUPCHECK) &&
                                (id < ID_SETUPCHECK + 16))
                                VSetupCheckMsg(id, NotifCode);
                        else
                                return FALSE;
                }
                break;
            } /* end of WM_COMMAND */
        case WM_PAINT:
                VSetupPaint(hdlg);
                break;
        case WM_LBUTTONDOWN:
                VSetupButtonDown(hdlg, (LONG)lParam);
                break;
        case WM_VSCROLL:
//              if (HIWORD(lParam))  // in DOS this is the window ID - I don't see why we need the test
                                     // and in NT we don't have that ID anyway
                        VSetupArrowScroll(hdlg, LOWORD(wParam));
                                     // LOWORD(wParam) is scroll code.  See cpArrow:
                                     // actually the whole of wParam is.
                break;
        case WM_CHAR:
                if ((LONG)lParam == 14)
                        if (iCurPos == 15)
                                iCurPos = 0;
                        else
                                iCurPos++;
                else if ((LONG)lParam == 16)
                        if (!iCurPos)
                                iCurPos = 15;
                        else
                                iCurPos--;
                else
                        return FALSE;
                VSetupSetFocus(hdlg, 0L, 0L);
                break;
        case WM_CLOSE:
                PostMessage(hdlg, WM_COMMAND, (WPARAM)IDOK, (LPARAM)0);
                break;

        default:
                if (uMessage == uHelpMessage) {
DoHelp:
                        WinHelp(hWnd, szMidiHlp, HELP_CONTEXT,
                                                IDH_DLG_MIDI_SETUPEDIT);
                        return TRUE;
                }
                else
                        return FALSE;
                break;
        }
        return TRUE;
} /* SetupBox */

/*
 * FSetupEnumPortsFunc
 *
 * This function receives port information for each channel in a setup,
 * determines if the port is not available in the current environment, and
 * if so adds it to a listbox of invalid port names.
 *
 * The types and parameters are weird because it is being forced into the straightjacket
 * of an enumfunc.
 */

BOOL FAR PASCAL _loadds FSetupEnumPortsFunc(
        LPSTR   lpChannel,
        LPSTR   lpPort,
        UINT    uCase,   // unused
        HWND    hCombo,  //unused
        LPSTR   DeviceID)
{
        SETUP FAR* lpSetup;
        INT    Idx;

        if (DeviceID == (LPSTR)MMAP_ID_NOPORT) {
                Idx = ComboLookup(hPortCombo, lpPort);
                if (Idx == CB_ERR) {
                        SendMessage(hPortCombo, CB_ADDSTRING, (WPARAM)NULL,
                                (LPARAM)lpPort);
                        Idx = nPorts++;
                }
                lpSetup = (SETUP FAR*)GlobalLock(hSetup);
                lpSetup->channels[(WORD)(DWORD)lpChannel - 1].wDeviceID = Idx;
                GlobalUnlock(hSetup);
        }
        return TRUE;
} /* FSetupEnumPortsFunc */

