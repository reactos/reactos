/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Welcome.c

Abstract:

    This module contains the page specific routines for the Welcome page of
    of the application manager.

Author:

    Dave Hastings (daveh) creation-date-29-Apr-1997

Revision History:

--*/
#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#include "resource.h"
#include "pip.h"
#include "appmgr.h"

static LPWSTR
RadioButtonsSelected(
    HWND DialogWindow
    );

HWND WelcomeToolTip;

BOOL
WelcomeInitProc(
    HWND DialogWindow,
	UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    This function handles initialization for IDD_WELCOME.  Currently,
    it just creates the tool tip for the unimplemented functionality.

Arguments:

    DialogWindow - Supplies the handle for the dialog.
    wParam - Supplies the wParam for the WM_INIT message
    lParam - Supplies the lParam for the WM_INIT message
    
Return Value:

    TRUE for success.

--*/
{
    TOOLINFO ToolInfo;
    RECT r;
    POINT p;
    HWND Control;

	switch (Msg) {
		case WM_INITDIALOG:
#if 0
			//
			// Create the tool tip window
			//
			WelcomeToolTip = CreateWindowEx(
				0,
				TOOLTIPS_CLASS,
				NULL,
				TTS_ALWAYSTIP,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				DialogWindow,
				NULL,
				Instance,
				NULL
				);

			//
			// Attach the unimplemented feature tip to the internet radio button
			//
			UnimplementedFeature(DialogWindow, WelcomeToolTip, IDC_RADIOUPGRADE);
#endif
            //
            // Disable the appropriate set of options, based on policy
            // Note:  These options are not mutually exclusive
            //
            if (FeatureMask & FEATURE_ADD) {
                //
                // Disable the add programs button
                //
                Control = GetDlgItem(DialogWindow, IDC_ADDRADIO);
                EnableWindow(Control, FALSE);
            }

            if (FeatureMask & FEATURE_REPAIR) {
                //
                // Disable the add programs button
                //
                Control = GetDlgItem(DialogWindow, IDC_RADIO_REPAIR);
                EnableWindow(Control, FALSE);
            }

            if (FeatureMask & FEATURE_MODIFYBRANCH) {
                //
                // Disable the add programs button
                //
                Control = GetDlgItem(DialogWindow, IDC_REMOVERADIO);
                EnableWindow(Control, FALSE);
            }

            //
            // Set up the correct fonts
            //
            SetExteriorTitleFont(DialogWindow);
			
            return TRUE;

        default:
            return TRUE;

	}

    return TRUE;
}

INT
WelcomeNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    )
/*++

Routine Description:

    This function handles the WM_NOTIFY messages for the Welcome page.

Arguments:

    DialogWindow - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this notify message
    NotifyHeader - Supplies a pointer to the notify header for this message
    PageIndex - Supplies the index of this page in the PageInfo structure

Return Value:

    non-zero if the message was handled.  DWL_MSGRESULT is set to the 
    return value for the message

--*/
{
    DWORD ActiveButtons;
    LPWSTR NextPage;

    switch (NotifyHeader->code) {
        case PSN_SETACTIVE:
            //
            // Assynchronously gather information about machine
            // configuration.  This is used to help determine
            // what features are present
            //
            GetMachineConfig();

            //
            // set the correct wizard buttons
            //
            if (RadioButtonsSelected(DialogWindow)) {
                //
                // One of the radio buttons is selected
                // so we can display the next button
                //
                ActiveButtons = PSWIZB_NEXT;
            } else {
                ActiveButtons = 0;
            }

            PostMessage(GetParent(DialogWindow), PSM_SETWIZBUTTONS, 0, ActiveButtons);
            CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, 0);
            return TRUE;

        case PSN_WIZNEXT:
            //
            // Insure we will be going to the correct page
            //
            NextPage = RadioButtonsSelected(DialogWindow);

            CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, (ULONG)NextPage);

            return TRUE;

        default:
            return FALSE;
    }
}

static LPWSTR
RadioButtonsSelected(
    HWND DialogWindow
    )
/*++

Routine Description:

    This routine check the state of the radio buttons to figure out if one of them is selected.

Arguments:

    DialogWindow - Supplies the handle of the dialog window.
    
Return Value:

    Dialog template ID of the page to go to.

--*/
{
    HWND Control;
    LRESULT ButtonState;

    Control = GetDlgItem(DialogWindow, IDC_ADDRADIO);

    ButtonState = SendMessage(Control, BM_GETCHECK, 0, 0);

    if (ButtonState == BST_CHECKED) {
        return MAKEINTRESOURCE(IDD_ADD_SOURCE);
    }

    Control = GetDlgItem(DialogWindow, IDC_RADIOUPGRADE);

    ButtonState = SendMessage(Control, BM_GETCHECK, 0, 0);

    if (ButtonState == BST_CHECKED) {
        return MAKEINTRESOURCE(IDD_UPGRADE_PROGRAM);
    }

    Control = GetDlgItem(DialogWindow, IDC_REMOVERADIO);

    ButtonState = SendMessage(Control, BM_GETCHECK, 0, 0);

    if (ButtonState == BST_CHECKED) {
        return MAKEINTRESOURCE(IDD_MODIFY_PROGRAM);
    }

    Control = GetDlgItem(DialogWindow, IDC_RADIO_REPAIR);

    ButtonState = SendMessage(Control, BM_GETCHECK, 0, 0);

    if (ButtonState == BST_CHECKED) {
        return MAKEINTRESOURCE(IDD_REPAIR_PROGRAM);
    }

    return FALSE;
}

INT
WelcomeCommandProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT Index
    )
/*++

Routine Description:

    This routine handles WM_COMMAND processing for the Welcome page.

Arguments:

    Dialog - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this WM_COMMAND message.
    lParam - Supplies the lParam for this WM_COMMAND message.
    Index - Supplies the index to the information for this page

Return Value:

    see WM_COMMAND documentation.

--*/
{
    switch (LOWORD(wParam)) {
        case IDC_REMOVERADIO:
        case IDC_ADDRADIO:
        case IDC_RADIO_REPAIR:
        case IDC_RADIOUPGRADE:
            SendMessage(GetParent(Dialog), PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT);
            return TRUE;

        default:
            return FALSE;
    }
}