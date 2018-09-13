/*****************************************************************************
 *
 *  UI.c
 *
 *  Copyright (c) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      The thing that does UI.
 *
 *****************************************************************************/

#include "msnspa.h"

/*****************************************************************************
 *
 *  Overview:
 *
 *      UI for the app is done on the main thread.  The app itself
 *      is not in the taskbar or in the tray.  The only way to get to
 *      it is to Alt+Tab to it.  When you minimize it, it vanishes again.
 *
 *      BUGBUG -- someday create an optional tray icon
 *
 *  Details:
 *
 *          The main window
 *          is just a dialog box.  The window we create is just a
 *          subclassed static control.
 *
 *          By stealing an existing control, we don't need to register
 *          our own bogus class.
 *
 *          We hide from the taskbar by having a hidden owner.
 *
 *          We show up in Alt+Tab because the hidden owner is
 *          not marked WS_EX_TOOLWINDOW.
 *
 *          We vanish on minimization by hiding ourselves, parking
 *          the minimized window "in outer space" so it doesn't bother
 *          the user.  Then re-show it so it can take part in Alt+Tab.
 *
 *****************************************************************************/

/*
 *  These are the co-ordinates of outer space.  Note that we can't base
 *  this on GetSystemMetrics(SM_C[XY]SCREEN) because the user might be
 *  running multiple monitors, and we need to be sure we are outside all
 *  monitors.  So we pick a really absurd value for outer space.
 */
#define XOUTERSPACE         (-32000)
#define YOUTERSPACE         (-32000)

HWND g_hwndMain;
HWND g_hwndDlg;
int g_cMailUsers;
int g_cNewsUsers;

/*****************************************************************************
 *
 *  @func   void | UI_UpdateCounts |
 *
 *          Update the counts of things.
 *
 *****************************************************************************/

void INTERNAL
UI_UpdateCounts(void)
{
    SetDlgItemInt(g_hwndDlg, IDC_MAIL, g_cMailUsers, FALSE);
    SetDlgItemInt(g_hwndDlg, IDC_NEWS, g_cNewsUsers, FALSE);

    EnableWindow(GetDlgItem(g_hwndDlg, IDOK), !(g_cMailUsers | g_cNewsUsers));

}

/*****************************************************************************
 *
 *  @func   BOOL | UI_OnSysCommand |
 *
 *          Munge some commands around.
 *
 *****************************************************************************/

BOOL INTERNAL
UI_OnSysCommand(HWND hdlg, WPARAM wp)
{
    switch (wp & ~0xF) {
    case SC_CLOSE:
        FORWARD_WM_COMMAND(hdlg, IDCANCEL, 0, 0, PostMessage);
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************
 *
 *  @func   BOOL | UI_OnCommand |
 *
 *          Munge some commands around.
 *
 *****************************************************************************/

BOOL INTERNAL
UI_OnCommand(HWND hdlg, WPARAM wp)
{
    ANIMATIONINFO aniOld, aniNew;

    switch (wp) {
    case IDCANCEL:
        /*
         *  There is no way to minimize a hidden window.
         */

        aniOld.cbSize = sizeof(aniOld);
        SystemParametersInfo(SPI_GETANIMATION, sizeof(aniOld), &aniOld, 0);

        aniNew.cbSize = sizeof(aniNew);
        aniNew.iMinAnimate = 0;
        SystemParametersInfo(SPI_SETANIMATION, sizeof(aniNew), &aniNew, 0);

        ShowWindow(hdlg, SW_MINIMIZE);

        SystemParametersInfo(SPI_SETANIMATION, sizeof(aniOld), &aniOld, 0);

        SetWindowPos(hdlg, HWND_BOTTOM,
                     XOUTERSPACE, YOUTERSPACE, 0, 0,
                     SWP_NOACTIVATE | SWP_NOSIZE | SWP_SHOWWINDOW);
        return TRUE;

    case IDOK:
        FORWARD_WM_CLOSE(hdlg, PostMessage);
        return TRUE;
    }

    return FALSE;
}

/*****************************************************************************
 *
 *  @func   BOOL | UI_OnClose |
 *
 *          Note that various weird conditions can lead to us getting
 *          here while there are active sessions, so re-check before
 *          leaving.
 *
 *****************************************************************************/

void INTERNAL
UI_OnClose(HWND hdlg)
{
    if (IsWindowEnabled(GetDlgItem(hdlg, IDOK))) {
        DestroyWindow(hdlg);
    }
}

/*****************************************************************************
 *
 *  @func   BOOL | UI_DlgProc |
 *
 *          Our dialog procedure.
 *
 *****************************************************************************/

BOOL CALLBACK
UI_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_SYSCOMMAND:
        return UI_OnSysCommand(hdlg, wp);

    case WM_COMMAND:
        return UI_OnCommand(hdlg, wp);

    case WM_NCPAINT:
        if (IsIconic(hdlg)) {
            return TRUE;
        }
        break;

    case WM_CLOSE:
        UI_OnClose(hdlg);
        return TRUE;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return FALSE;
}

/*****************************************************************************
 *
 *  @func   HWND | UI_Init |
 *
 *          Initialize the UI stuff.
 *
 *  @returns
 *
 *          Nonzero on success.
 *
 *****************************************************************************/

HWND INTERNAL
UI_Init(void)
{
    g_hwndMain = CreateWindow(
        "static",                       /* Class Name */
        "",                             /* Title */
        WS_OVERLAPPEDWINDOW | WS_MINIMIZE,
                                        /* Style (note: not visible) */
        CW_USEDEFAULT, CW_USEDEFAULT,   /* Position */
        CW_USEDEFAULT, CW_USEDEFAULT,   /* Size */
        NULL,                           /* Parent */
        NULL,                           /* Use class menu */
        g_hinst,                        /* Instance */
        0);                             /* No special parameters */

//    SubclassWindow(g_hwndMain, Main_WndProc);

    SendMessage(g_hwndMain, WM_SETICON, ICON_BIG,
                (LPARAM)LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_MAIN)));

    /*
     *  Create the dialog not visible because we're going to be shoving
     *  it around.
     */
    g_hwndDlg = CreateDialog(g_hinst, MAKEINTRESOURCE(IDD_MAIN),
                             g_hwndMain, UI_DlgProc);

    /*
     *  Tell the system that the window should be parked in outer space.
     */
    SetWindowPos(g_hwndDlg, HWND_BOTTOM,
                 XOUTERSPACE, YOUTERSPACE, 0, 0,
                 SWP_NOACTIVATE | SWP_NOSIZE | SWP_SHOWWINDOW);

    return g_hwndDlg;
}

/*****************************************************************************
 *
 *  @func   void | UI_Term |
 *
 *          Clean up the UI stuff.
 *
 *****************************************************************************/

void INTERNAL
UI_Term(void)
{
    DestroyWindow(g_hwndMain);
}
