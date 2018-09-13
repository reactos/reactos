/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1991
 *  All Rights Reserved.
 *
 *
 *  PIFVID.C
 *  User interface dialogs for GROUP_VID
 *
 *  History:
 *  Created 04-Jan-1993 1:10pm by Jeff Parsons
 */

#include "shellprv.h"
#pragma hdrstop

#ifdef WIN32
#define hModule GetModuleHandle(TEXT("SHELL32.DLL"))
#endif

#define VDD_DEVICE_ID           0x0000A


BINF abinfVid[] = {
    {IDC_WINDOWED,      BITNUM(VID_FULLSCREEN)   | 0x80},
    {IDC_FULLSCREEN,    BITNUM(VID_FULLSCREEN)},
    {IDC_TEXTEMULATE,   BITNUM(VID_TEXTEMULATE)},
    {IDC_DYNAMICVIDMEM, BITNUM(VID_RETAINMEMORY) | 0x80},
};

#ifndef _WIN32_WINNT // no console window toolbar on NT
BINF abinfWin[] ;= {
    {IDC_TOOLBAR,       BITNUM(WIN_TOOLBAR)},   
};
#endif _WIN32_WINNT

BINF abinfWinInit[] = {
    {IDC_WINRESTORE,    BITNUM(WININIT_NORESTORE) | 0x80},
};

// Private function prototypes

void EnableVidDlg(HWND hDlg, PPROPLINK ppl);
void InitVidDlg(HWND hDlg, PPROPLINK ppl);
void ApplyVidDlg(HWND hDlg, PPROPLINK ppl);


// Context-sensitive help ids

const static DWORD rgdwHelp[] = {
    IDC_SCREENUSAGEGRP, IDH_COMM_GROUPBOX,
    IDC_FULLSCREEN,     IDH_DOS_SCREEN_USAGE_FULL,
    IDC_WINDOWED,       IDH_DOS_SCREEN_USAGE_WINDOW,
#ifdef NEW_UNICODE
    IDC_SCREENXBUFLBL,  IDH_DOS_DISPLAY_SCREEN_SETTINGS,
    IDC_SCREENXBUF,     IDH_DOS_DISPLAY_SCREEN_SETTINGS,
    IDC_SCREENYBUFLBL,  IDH_DOS_DISPLAY_SCREEN_SETTINGS,
    IDC_SCREENYBUF,     IDH_DOS_DISPLAY_SCREEN_SETTINGS,
    IDC_WINXSIZELBL,    IDH_DOS_DISPLAY_SCREEN_SETTINGS,
    IDC_WINXSIZE,       IDH_DOS_DISPLAY_SCREEN_SETTINGS,
    IDC_WINYSIZELBL,    IDH_DOS_DISPLAY_SCREEN_SETTINGS,
    IDC_WINYSIZE,       IDH_DOS_DISPLAY_SCREEN_SETTINGS,
#else
    IDC_SCREENLINESLBL, IDH_DOS_DISPLAY_SCREEN_SETTINGS,
    IDC_SCREENLINES,    IDH_DOS_DISPLAY_SCREEN_SETTINGS,
#endif
    IDC_WINDOWUSAGEGRP, IDH_COMM_GROUPBOX,
    IDC_TOOLBAR,        IDH_DOS_WINDOWS_TOOLBAR,
    IDC_SCREENPERFGRP,  IDH_COMM_GROUPBOX,
    IDC_TEXTEMULATE,    IDH_DOS_DISPLAY_ROM,
    IDC_WINRESTORE,     IDH_DOS_SCREEN_RESTORE,
    IDC_DYNAMICVIDMEM,  IDH_DOS_SCREEN_DMA,
    IDC_REALMODEDISABLE,IDH_DOS_REALMODEPROPS,
    0, 0
};

/*
 *  This is a little table that converts listbox indices into
 *  screen lines.
 *
 *  The correspondences are...
 *
 *      IDS_WHATEVER = List box index + IDS_DEFAULTLINES
 *      nLines = awVideoLines[List box index]
 */
#if IDS_25LINES - IDS_DEFAULTLINES != 1 || \
    IDS_43LINES - IDS_DEFAULTLINES != 2 || \
    IDS_50LINES - IDS_DEFAULTLINES != 3
#error Manifest constants damaged.
#endif

#ifdef NEW_UNICODE

// 0xFFFF tags the end of the array
#define AW_END_TAG 0xFFFF

WORD awBuffX[] = { 80, 90, 100, 120, AW_END_TAG };
WORD awBuffY[] = { 25, 43, 50, 60, 70, 80, 90, 100, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 9999, AW_END_TAG };
WORD awWinX[]  = { 80, 90, 100, 120, AW_END_TAG };
WORD awWinY[]  = { 25, 43, 50, 60, 70, 80, 90, 100, AW_END_TAG };

#else

WORD awVideoLines[] = { 0, 25, 43, 50 };

#endif // NEW_UNICODE


BOOL_PTR CALLBACK DlgVidProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PPROPLINK ppl;
    FunctionName(DlgVidProc);

    ppl = (PPROPLINK)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) {
    case WM_INITDIALOG:
        lParam = ((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        ppl = (PPROPLINK)(INT_PTR)lParam;
        InitVidDlg(hDlg, ppl);
        break;

    HELP_CASES(rgdwHelp)                // Handle help messages

    case WM_COMMAND:
        if (LOWORD(lParam) == 0)
            break;                      // message not from a control

        switch (LOWORD(wParam)) {

#ifdef NEW_UNICODE
        case IDC_SCREENXBUF:
        case IDC_SCREENYBUF:
        case IDC_WINXSIZE:
        case IDC_WINYSIZE:
            if ((HIWORD(wParam) == CBN_SELCHANGE) || (HIWORD(wParam) == CBN_EDITCHANGE))
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;
#else
        case IDC_SCREENLINES:
            if (HIWORD(wParam) == CBN_SELCHANGE)
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;
#endif // NEW_UNICODE

        case IDC_WINDOWED:
        case IDC_FULLSCREEN:
        case IDC_WINRESTORE:
        case IDC_TEXTEMULATE:
        case IDC_DYNAMICVIDMEM:
#ifndef _WIN32_WINNT  // no console window toolbar on NT
	case IDC_TOOLBAR:
#endif  _WIN32_WINNT
            if (HIWORD(wParam) == BN_CLICKED)
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {
        case PSN_SETACTIVE:
            AdjustRealModeControls(ppl, hDlg);
            break;

        case PSN_KILLACTIVE:
            // This gives the current page a chance to validate itself
            // SetWindowLong(hDlg, DWL_MSGRESULT, 0);
            break;

        case PSN_APPLY:
            // This happens on OK....
            ApplyVidDlg(hDlg, ppl);
            break;

        case PSN_RESET:
            // This happens on Cancel....
            break;
        }
        break;

    default:
        return FALSE;                   // return 0 when not processing
    }
    return TRUE;
}


void InitVidDlg(HWND hDlg, PPROPLINK ppl)
{
    WORD w;
    HWND hwnd;
    PROPVID vid;
    PROPWIN win;
    TCHAR szBuf[MAX_STRING_SIZE];
    FunctionName(InitVidDlg);

    if (!PifMgr_GetProperties(ppl, MAKELP(0,GROUP_VID),
                        &vid, SIZEOF(vid), GETPROPS_NONE) ||
        !PifMgr_GetProperties(ppl, MAKELP(0,GROUP_WIN),
                        &win, SIZEOF(win), GETPROPS_NONE)) {
        Warning(hDlg, IDS_QUERY_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    SetDlgBits(hDlg, &abinfVid[0], ARRAYSIZE(abinfVid), vid.flVid);
    SetDlgBits(hDlg, &abinfWinInit[0], ARRAYSIZE(abinfWinInit), win.flWinInit);
#ifndef _WIN32_WINNT // no console window toolbar on NT
    SetDlgBits(hDlg, &abinfWin[0], ARRAYSIZE(abinfWin), win.flWin);
#endif  _WIN32_WINNT 

#ifdef NEW_UNICODE
    /*
     *  Fill in the buffer and screen size combo boxes with some intelligent
     *  choices.
     */

    VERIFYTRUE(hwnd = GetDlgItem(hDlg, IDC_SCREENXBUF));
    for (w = 0; awBuffX[w]!=AW_END_TAG; w++) {
        wsprintf( szBuf, TEXT("%3d"), awBuffX[w] );
        VERIFYTRUE(SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)szBuf) == w);
    }
    SendMessage( hwnd, CB_SETCURSEL, 0, 0 );

    VERIFYTRUE(hwnd = GetDlgItem(hDlg, IDC_SCREENYBUF));
    for (w = 0; awBuffY[w]!=AW_END_TAG; w++) {
        wsprintf( szBuf, TEXT("%4d"), awBuffY[w] );
        VERIFYTRUE(SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)szBuf) == w);
    }
    SendMessage( hwnd, CB_SETCURSEL, 0, 0 );

    VERIFYTRUE(hwnd = GetDlgItem(hDlg, IDC_WINXSIZE));
    for (w = 0; awWinX[w]!=AW_END_TAG; w++) {
        wsprintf( szBuf, TEXT("%3d"), awWinX[w] );
        VERIFYTRUE(SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)szBuf) == w);
    }
    SendMessage( hwnd, CB_SETCURSEL, 0, 0 );

    VERIFYTRUE(hwnd = GetDlgItem(hDlg, IDC_WINYSIZE));
    for (w = 0; awWinY[w]!=AW_END_TAG; w++) {
        wsprintf( szBuf, TEXT("%3d"), awWinY[w] );
        VERIFYTRUE(SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)szBuf) == w);
    }
    SendMessage( hwnd, CB_SETCURSEL, 0, 0 );

#else
    /*
     *  Fill in the "Initial screen size" combo box.  Note that
     *  we bail on low-memory errors.  Note also that if we have
     *  a nonstandard size, we just leave the combo box with no
     *  default selection.
     */

    VERIFYTRUE(hwnd = GetDlgItem(hDlg, IDC_SCREENLINES));
    for (w = 0; w < ARRAYSIZE(awVideoLines); w++) {
        VERIFYTRUE(LoadString(hModule, IDS_DEFAULTLINES + w, szBuf, ARRAYSIZE(szBuf)));
        VERIFYTRUE(SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)szBuf) == w);
        if (vid.cScreenLines == awVideoLines[w]) {
            SendMessage(hwnd, CB_SETCURSEL, w, 0);
        }
    }
    if (!IsBilingualCP(g_uCodePage))
    {
        EnableWindow(GetDlgItem(hDlg, IDC_SCREENLINESLBL), FALSE);
        EnableWindow(hwnd, FALSE);
    }
#endif  // NEW_UNICODE

}

void ApplyVidDlg(HWND hDlg, PPROPLINK ppl)
{
    DWORD dw;
    HWND hwnd;
    PROPVID vid;
    PROPWIN win;
    FunctionName(ApplyVidDlg);

    // Get the current set of properties, then overlay the new settings

    if (!PifMgr_GetProperties(ppl, MAKELP(0,GROUP_VID),
                        &vid, SIZEOF(vid), GETPROPS_NONE) ||
        !PifMgr_GetProperties(ppl, MAKELP(0,GROUP_WIN),
                        &win, SIZEOF(win), GETPROPS_NONE)) {
        Warning(hDlg, IDS_UPDATE_ERROR, MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    GetDlgBits(hDlg, &abinfVid[0], ARRAYSIZE(abinfVid), &vid.flVid);
    GetDlgBits(hDlg, &abinfWinInit[0], ARRAYSIZE(abinfWinInit), &win.flWinInit);

#ifndef _WIN32_WINNT // no console window toolbar on NT
    GetDlgBits(hDlg, &abinfWin[0], ARRAYSIZE(abinfWin), &win.flWin);
#endif  _WIN32_WINNT

#ifdef NEW_UNICODE
    /*
     *  Get the console buffer/window size settings and map to the
     *  appropriate PIF settings.
     */
#else
    /*
     *  If there is no current selection, don't change the cScreenLines
     *  property.  This allows the user to retain an unusual number of
     *  screen lines by simply not touching the field.
     */
    VERIFYTRUE(hwnd = GetDlgItem(hDlg, IDC_SCREENLINES));

    dw = (DWORD) SendMessage(hwnd, CB_GETCURSEL, 0, 0L);
    if (dw < ARRAYSIZE(awVideoLines)) {
        vid.cScreenLines = awVideoLines[dw];
    }
#endif // NEW_UNICODE

    if (!PifMgr_SetProperties(ppl, MAKELP(0,GROUP_VID),
                        &vid, SIZEOF(vid), SETPROPS_NONE) ||
        !PifMgr_SetProperties(ppl, MAKELP(0,GROUP_WIN),
                        &win, SIZEOF(win), SETPROPS_NONE))
        Warning(hDlg, IDS_UPDATE_ERROR, MB_ICONEXCLAMATION | MB_OK);
    else
    if (ppl->hwndNotify) {
        ppl->flProp |= PROP_NOTIFY;
        PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(vid), (LPARAM)MAKELP(0,GROUP_VID));
        PostMessage(ppl->hwndNotify, ppl->uMsgNotify, SIZEOF(win), (LPARAM)MAKELP(0,GROUP_WIN));
    }
}
