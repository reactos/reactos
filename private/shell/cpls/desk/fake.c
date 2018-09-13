///////////////////////////////////////////////////////////////////////////////
// Fake.c
//
// this code gives OEMs a bogus Settings page to patch/modify
//
// OEM display pages patch the snot out of the Settings page
// the problem is that we have changed the setting page, and
// even if the OEM code could deal with our changes, the
// settings page is not even on the same property sheet
// as the extensions.
//
// some OEMs hook the setting page by hooking the TabContol and
// watching for a mouse click to activate the Settings page
// then they modify the page.  NOTE they dont patch the page
// if it is activated via the keyboard only the mouse!
//
// some OEM pages find the Setting page by name.
// we have named the real settings page "Settings " and
// the fake page is "Settings"
//
// some OEM pages assume the last page is the Settings page.
// we make sure the fake settings is always last.
//
///////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "winuser.h"
#pragma hdrstop
#include "cplext.h"

#ifndef TCIS_HIDDEN
#define TCIS_HIDDEN 0x0004
#pragma message("using local version of TCIS_HIDDEN")
#endif

#undef MATROX_HACK

/*
 * Global stuff
 */
static LONG     TabWndProc;             // SysTabControl WndProc
static LONG     DlgWndProc;             // Dialog WndProc

#ifdef MATROX_HACK
static PROPSHEETHEADER psh;             // saved prop sheet header
#endif

/*
 * Local Constant Declarations
 */
int  CALLBACK DeskPropSheetCallback(HWND hDlg, UINT code, LPARAM lParam);
BOOL CALLBACK FakeSettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
int  CALLBACK DeskPropSheetCallback(HWND hDlg, UINT code, LPARAM lParam);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
#define ODS(sz) (OutputDebugStringA(sz), OutputDebugStringA("\r\n"))
#else
#define ODS(sz)
#endif

///////////////////////////////////////////////////////////////////////////////
// AddFakeSettingsPage
///////////////////////////////////////////////////////////////////////////////
void AddFakeSettingsPage(PROPSHEETHEADER * ppsh)
{
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hpsp;

    ZeroMemory(&psp, sizeof(psp));

    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_DEFAULT;
    psp.hInstance   = hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(DLG_FAKE_SETTINGS);
    psp.pfnDlgProc  = FakeSettingsDlgProc;
    psp.lParam      = 0L;

    //
    // some OEMs find the Property sheet by window title
    // so make sure we have the title they expect.
    //
#ifdef MATROX_HACK
    if (ppsh->hwndParent && IsWindowVisible(ppsh->hwndParent))
    {
        psh = *ppsh;
        ppsh->pszCaption = MAKEINTRESOURCE(IDS_DISPLAY_TITLE);
        psp.dwFlags |= PSP_PREMATURE;
    }
#endif

    if (hpsp = CreatePropertySheetPage(&psp))
    {
        ppsh->phpage[ppsh->nPages++] = hpsp;

        ppsh->pfnCallback = DeskPropSheetCallback;
        ppsh->dwFlags    |= PSH_USECALLBACK;
    }
}

///////////////////////////////////////////////////////////////////////////////
// FakeSettingsDlgProc
///////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK FakeSettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (hDlg && DlgWndProc)
    {
        if (GetWindowLong(hDlg, GWL_WNDPROC) != DlgWndProc)
        {
            SetWindowLong(hDlg, GWL_WNDPROC, DlgWndProc);
            ODS("FakeSettingsDlgProc: **** Dialog is sublassed!");
        }

        if (GetWindowLong(hDlg, 4) != (LONG)FakeSettingsDlgProc)
        {
            SetWindowLong(hDlg, 4, (LONG)FakeSettingsDlgProc);
            ODS("FakeSettingsDlgProc: **** DialogProc is changed!");
        }
    }

    switch (message)
    {
        case WM_INITDIALOG:
            return TRUE;

        case WM_DESTROY:
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_CHANGEDRV)
            {
            }
            break;
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// HideFakeSettingsPage
//
// hide the bogus settings page from view so the user cant see it.
//
// this code uses a new TabCtrl item style TCIS_HIDDEN that is only
// in the Win98/NT5/IE4.01 COMMCTRL
//
///////////////////////////////////////////////////////////////////////////////
void HideFakeSettingsPage(HWND hDlg)
{
    HWND hwnd;

#ifdef DEBUG
    if (GetAsyncKeyState(VK_SHIFT))
        return;
#endif

    if (hwnd = PropSheet_GetTabControl(hDlg))
    {
        TCITEM tci;
        tci.mask = TCIF_STATE;
        tci.dwStateMask = TCIS_HIDDEN;
        tci.dwState = TCIS_HIDDEN;
        TabCtrl_SetItem(hwnd, TabCtrl_GetItemCount(hwnd)-1, &tci);
    }
}

///////////////////////////////////////////////////////////////////////////////
// DeskPropSheetCallback
//
// what this callback does is look for someone subclassing the
// tab control, if we detect this we put the correct WndProc
// back.
//
// we also hide the fake settings page after all the entensions
// have initialized
//
///////////////////////////////////////////////////////////////////////////////
int CALLBACK DeskPropSheetCallback(HWND hDlg, UINT code, LPARAM lParam)
{
    HWND hwnd;

    WNDCLASS wc;

    switch (code)
    {
        case PSCB_INITIALIZED:
            ODS("DeskPropSheetCallback: PSCB_INITIALIZED");

            if ((hwnd = PropSheet_GetTabControl(hDlg)) && TabWndProc)
            {
                if (GetWindowLong(hwnd, GWL_WNDPROC) != TabWndProc)
                {
                    SetWindowLong(hwnd, GWL_WNDPROC, TabWndProc);
                    ODS("DeskPropSheetCallback: **** TabControl is sublassed!");
                }
            }

            if (hDlg && DlgWndProc)
            {
                if (GetWindowLong(hDlg, GWL_WNDPROC) != DlgWndProc)
                {
                    SetWindowLong(hDlg, GWL_WNDPROC, DlgWndProc);
                    ODS("DeskPropSheetCallback: **** Dialog is sublassed!");
                }
            }

#ifdef MATROX_HACK
            //
            // restore the caption
            //
            if (psh.pszCaption)
            {
                ShowWindow(hDlg, SW_SHOW);
                Sleep(10);
                PropSheet_SetTitle(hDlg, psh.dwFlags, psh.pszCaption);
                PropSheet_SetTitle(GetParent(psh.hwndParent),
                    PSH_PROPTITLE, MAKEINTRESOURCE(IDS_DISPLAY_TITLE));
                psh.pszCaption = 0;
            }
#endif
            //
            // hide the settings page so the user cant see it.
            //
            HideFakeSettingsPage(hDlg);
            break;

        case PSCB_PRECREATE:
            ODS("DeskPropSheetCallback: PSCB_PRECREATE");

            ZeroMemory(&wc, sizeof(wc));
            GetClassInfo(NULL, WC_DIALOG, &wc);
            DlgWndProc = (LONG)wc.lpfnWndProc;

            ZeroMemory(&wc, sizeof(wc));
            GetClassInfo(NULL, WC_TABCONTROL, &wc);
            TabWndProc = (LONG)wc.lpfnWndProc;
            break;
    }

    return 0;
}
