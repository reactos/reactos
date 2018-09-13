/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    inetadd.c

Abstract:

    This module contains code for the install from internet pages.

Author:

    Dave Hastings (daveh) creation-date 07-Jul-1997

Revision History:

--*/
#include <windows.h>
// bugbug
#include <commctrl.h>
#include "resource.h"
#include "pip.h"
#include "appmgr.h"

WCHAR UrlString[255];

// start dead code
BOOL
InternetSiteInitProc(
    HWND DialogWindow,
	UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    This function handles initialization for IDD_INTERNETSITE.

Arguments:

    DialogWindow - Supplies the handle for the dialog.
    wParam - Supplies the wParam for the WM_INIT message
    lParam - Supplies the lParam for the WM_INIT message
    
Return Value:

    TRUE for success.

--*/
{
    HWND Control;

	switch (Msg) {
		case WM_INITDIALOG:
			Control = GetDlgItem(DialogWindow, IDC_RADIOMICROSOFT);

			//
			// Make the Microsoft radio button checked
			//
			SendMessage(Control, BM_SETCHECK, BST_CHECKED, 0);

		    return TRUE;

		default:

			return TRUE;
	}
}

INT
InternetSiteNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    )
/*++

Routine Description:

    This function handles the WM_NOTIFY messages for the IDD_INTERNETSITE page.

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
	BOOL Result;
    LPWSTR NextPage;
    DWORD ButtonState;

    switch (NotifyHeader->code) {
        case PSN_SETACTIVE:
            //
            // set the correct wizard buttons
            //
            ActiveButtons = PSWIZB_BACK | PSWIZB_NEXT;

            PostMessage(GetParent(DialogWindow), PSM_SETWIZBUTTONS, 0, ActiveButtons);

            CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, 0);
            return TRUE;

        case PSN_WIZNEXT:

            //
            // Figure out which button is selected, and set up the 
            // url string
            //
            ButtonState = SendMessage(
                GetDlgItem(DialogWindow, IDC_RADIOMICROSOFT), 
                BM_GETCHECK, 
                0, 
                0
                );

            if (ButtonState == BST_CHECKED) {
                //
                // N.B.  This code assumes exactly two radiobuttons
                //
                lstrcpy(UrlString, L"http://www.microsoft.com");
            }
            
            //
            // Send us to the correct page
            //

            CheckSetWindowLong(
                DialogWindow, 
                DWL_MSGRESULT, 
                (ULONG)MAKEINTRESOURCE(IDD_ADD_FINISH_INTERNET)
                );

            return TRUE;

        case PSN_WIZBACK:
            //
            // Send us back to the start page
            //
            CheckSetWindowLong(
                DialogWindow, 
                DWL_MSGRESULT, 
                (ULONG)MAKEINTRESOURCE(IDD_ADD_SOURCE)
                );

            return TRUE;

#if DBG
        case PSN_QUERYCANCEL:
            //
            // We shouldn't be here.  Let's complain
            //
            OutputDebugString(L"AppMgr: PSN_QUERYCANCEL handling error\n");

            return FALSE;
#endif

        default:
            return FALSE;
    }
}

INT
InternetSiteCommandProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT Index
    )
/*++

Routine Description:

    This routine handles WM_COMMAND processing for the add program start page.

Arguments:

    Dialog - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this WM_COMMAND message.
    lParam - Supplies the lParam for this WM_COMMAND message.
    Index - Supplies the index to the information for this page

Return Value:

    see WM_COMMAND documentation.

--*/
{
    ULONG TextLength;
    ULONG ActiveButtons;

    switch (LOWORD(wParam)) {

        case IDC_RADIOMICROSOFT:

            PostMessage(
                GetParent(Dialog), 
                PSM_SETWIZBUTTONS, 
                0, 
                PSWIZB_BACK | PSWIZB_NEXT
                );

            return TRUE;
            
        case IDC_RADIOOTHER:

            SetFocus(GetDlgItem(Dialog, IDC_EDIT_INTERNETADDRESS));

            return TRUE;

        case IDC_EDIT_INTERNETADDRESS:

            switch (HIWORD(wParam)) {
                // bugbug set check state for radio button on focus?
                case EN_SETFOCUS:
                case EN_CHANGE:
                    //
                    // See if there is any text in the edit box, and
                    // disable the next button if not
                    //
                    TextLength = GetWindowText(
                        (HWND)lParam,
                        UrlString,
                        255
                        );

                    if (TextLength == 0) {
                        ActiveButtons = PSWIZB_BACK;
                    } else {
                        ActiveButtons = PSWIZB_BACK | PSWIZB_NEXT;
                    }

                    PostMessage(
                        GetParent(Dialog), 
                        PSM_SETWIZBUTTONS, 
                        0, 
                        ActiveButtons
                        );

                    return TRUE;

                default:

                    return FALSE;

            }

        default:
            return FALSE;
    }
}
// end dead code
INT
AddFinishInternetNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    )
/*++

Routine Description:

    This function handles the WM_NOTIFY messages for the IDD_INTERNETSITE page.

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
	BOOL Result;
    LPWSTR NextPage;
    HCURSOR Wait;

    switch (NotifyHeader->code) {
        case PSN_SETACTIVE:
            //
            // set the correct wizard buttons
            //
            ActiveButtons = PSWIZB_FINISH | PSWIZB_BACK;

            PostMessage(GetParent(DialogWindow), PSM_SETWIZBUTTONS, 0, ActiveButtons);

            CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, 0);
            return TRUE;


        case PSN_WIZBACK:
            //
            // Send us back to the start page
            //
            CheckSetWindowLong(
                DialogWindow, 
                DWL_MSGRESULT, 
                (ULONG)MAKEINTRESOURCE(IDD_ADD_SOURCE)
                );

            return TRUE;

        case PSN_WIZFINISH:

            Wait = LoadCursor(NULL, IDC_WAIT);
            SetCursor(Wait);

            // bugbug
            ShellExecute(
                DialogWindow,
                NULL,
                L"http://www.microsoft.com",
                NULL,
                NULL,
                SW_SHOWDEFAULT
                );

            //
            // We don't reset it, because the wizard is existing anyway
            //
            // bugbug does this mess up the control panel?
            //
            return TRUE;

#if DBG
        case PSN_QUERYCANCEL:
            //
            // We shouldn't be here.  Let's complain
            //
            OutputDebugString(L"AppMgr: PSN_QUERYCANCEL handling error\n");

            return FALSE;
#endif

        default:
            return FALSE;
    }
}

