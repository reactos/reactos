/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    repair.c

Abstract:

    This module contains code for the repair pages.

Author:

    Dave Hastings (daveh) creation-date 20-Jun-1997

Revision History:

--*/
#include <windows.h>
#include <commctrl.h>
#include <msi.h>
#include "resource.h"
#include "pip.h"
#include "appmgr.h"

VOID
RepairApplication(
    PAPPLICATIONDESCRIPTOR Descriptor,
    ULONG Action
    );

BOOL CALLBACK
RepairDialogProc(
    HWND Dialog,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
RepairDialog(
    PAPPLICATIONDESCRIPTOR Descriptor,
    HWND Parent
    );

static ULONG SelectedProgram;

BOOL
RepairSelectInitProc(
    HWND DialogWindow,
	UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    This routine initlializes the page for selecting an application to remove.

Arguments:

    DialogWindow - Supplies the handle of the dialog window
    wParam - Supplies the wParam for the WM_INITDIALOG message
    lParam - Supplies the lParam of the WM_INITTIALOG message

Return Value:

    bugbug

--*/
{
    HWND ListViewWindow;
    LV_ITEM Item;

	switch (Msg) {
		case WM_INITDIALOG:
			ListViewWindow = GetDlgItem(DialogWindow, IDC_APPLICATION_LIST);

            CreateApplicationListViewColumns(ListViewWindow);

			PopulateApplicationListView(ListViewWindow, PIP_REPAIR);

            PageInfo[GetWindowLong(DialogWindow, DWL_USER)].Dialog = DialogWindow;

			return TRUE;

		default:

			return TRUE;
	}

}

INT
RepairSelectNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    )
/*++

Routine Description:

    This function handles the WM_NOTIFY messages for the application selection
    page for repair programs.

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
    BOOL WindowState;
    LV_ITEM Item;
    PAPPLICATIONDESCRIPTOR Descriptor;

    switch (NotifyHeader->code) {
        case NM_CLICK:
            // bugbug
            WindowState = ItemSelected(DialogWindow, &SelectedProgram);
            // bugbug
            EnableWindow(
                GetDlgItem(DialogWindow, IDC_BUTTON_REPAIR),
                WindowState
                );

            return TRUE;

        case PSN_SETACTIVE:
            //
            // set the correct wizard buttons
            //

            ActiveButtons = PSWIZB_BACK;

            PostMessage(
                GetParent(DialogWindow),
                PSM_SETWIZBUTTONS,
                0,
                ActiveButtons
                );

            WindowState = ItemSelected(DialogWindow, &SelectedProgram);
            EnableWindow(GetDlgItem(DialogWindow, IDC_BUTTON_REPAIR), WindowState);

            CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, 0);
            return TRUE;

        case PSN_WIZBACK:
            //
            // Send us back to the start page
            //
            CheckSetWindowLong(
                DialogWindow,
                DWL_MSGRESULT,
                (ULONG)MAKEINTRESOURCE(IDD_WELCOME)
                );

            return TRUE;

        case PSN_WIZNEXT:
            //
            // Send us back to the start page
            //
            CheckSetWindowLong(
                DialogWindow,
                DWL_MSGRESULT,
                (ULONG)MAKEINTRESOURCE(IDD_REPAIR_FINISH)
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
RepairSelectCommandProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPARAM lParam,
    UINT PageIndex
    )
/*++

Routine Description:

    This routine handles the WM_COMMAND messages for the modify
    page.

Arguments:

    DialogWindow - Supplies the handle of the dialog window.
    wParam - Supplies the wParam for this WM_COMMAND message.
    lParam - Supplies the lParam for this WM_COMMAND message.
    PageIndex - Supplies the index to the information for this page

Return Value:

    see WM_COMMAND documentation.

--*/
{
    LVITEM Item;
    BOOL Success;

    switch(LOWORD(wParam)) {

        case IDC_BUTTON_REPAIR:

            //
            // Get the descriptor of the selected application
            //
            Item.mask = LVIF_PARAM;
            Item.iItem = SelectedProgram;
            Item.iSubItem = 0;
            ListView_GetItem(
                GetDlgItem(DialogWindow, IDC_APPLICATION_LIST),
                &Item
                );

            Success = RepairDialog(
                (PAPPLICATIONDESCRIPTOR)Item.lParam, 
                DialogWindow
                );

            if (Success) {
                PostMessage(
                    GetParent(DialogWindow),
                    PSM_SETWIZBUTTONS,
                    0,
                    PSWIZB_NEXT | PSWIZB_BACK
                    );
                // bugbug refresh list view
            }

            return TRUE;

        default:

            return FALSE;
    }
}

BOOL
RepairDialog(
    PAPPLICATIONDESCRIPTOR Descriptor,
    HWND Parent
    )
/*++

Routine Description:

    This function puts up the repiar/reinstall dialog.

Arguments:

    Descriptor - Supplies the information about the application.

Return Value:

   None.

--*/
{
    MSG Msg;
    HWND Dialog;
    INT RetVal;

    RetVal = DialogBoxParam(
        Instance,
        MAKEINTRESOURCE(IDD_REPAIRDLG1),
        Parent,
        RepairDialogProc,
        (LONG)Descriptor
        );

    if (RetVal == TRUE) {
        return TRUE;
    } else if (RetVal == FALSE) {
        return FALSE;
    } else {
        //
        // bugbug error
        //
        return FALSE;
    }
}

BOOL CALLBACK
RepairDialogProc(
    HWND Dialog,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    This routine is the dialog procedure for the repair/replace
    dialog.

Arguments:

    Dialog -- Supplies the handle of the dialog winodow.
    Msg -- Supplies the message number.
    wParam -- Supplies the wParam for the message.
    lParam -- Supplies the lParam for the message.

Return Value:

    TRUE if the message is processed, FALSE otherwise
--*/
{
    WCHAR ActionText[100];
    HWND TextWindow;
    static UINT TextId;
    static WCHAR OriginalActionText[50];
    ULONG Action;
    ULONG Length;

    switch (Msg) {
        case WM_INITDIALOG:

            CheckSetWindowLong(Dialog, DWL_USER, (LONG)lParam);
            TextWindow = GetDlgItem(Dialog, IDC_REPAIR_ACTION);
            GetWindowText(TextWindow, OriginalActionText, 50);

            return TRUE;

        case WM_COMMAND:
            //
            // Handle radio buttons and OK button
            //
            switch (LOWORD(wParam)) {
                case IDC_RADIO_REPAIR:
                case IDC_RADIO_REINSTALL:

                    TextWindow = GetDlgItem(Dialog, IDC_REPAIR_ACTION);
                    //
                    // Set the text correctly
                    //
                    if (LOWORD(wParam) == IDC_RADIO_REPAIR) {
                        TextId = IDS_REPAIR_REPAIR;
                    } else {
                        TextId = IDS_REPAIR_REINSTALL;
                    }

                    lstrcpy(ActionText, OriginalActionText);

                    Length = lstrlen(ActionText);

                    LoadString(
                        Instance,
                        TextId,
                        &ActionText[Length],
                        100 - Length
                        );

                    SetWindowText(
                        TextWindow,
                        ActionText
                        );

                    UpdateWindow(TextWindow);

                    ShowWindow(TextWindow, SW_SHOWNOACTIVATE);

                    //
                    // Enable the OK button
                    //
                    EnableWindow(
                        GetDlgItem(Dialog, IDOK),
                        TRUE
                        );

                    return TRUE;

                case IDOK:
                    //
                    // Repair or reinstall the app as appropriate
                    //
                    if (TextId == IDS_REPAIR_REPAIR) {
                        Action = PIP_REPAIR_REPAIR;
                    } else {
                        Action = PIP_REPAIR_REINSTALL;
                    }

                    RepairApplication(
                        (PAPPLICATIONDESCRIPTOR)GetWindowLong(
                            Dialog,
                            DWL_USER
                            ),
                        Action
                        );

                    EndDialog(Dialog, TRUE);

                    return TRUE;

                case IDCANCEL:

                    EndDialog(Dialog, FALSE);

                    return TRUE;

                default:
                    return FALSE;
            }

        default:
            return FALSE;
    }
}

VOID
RepairApplication(
    PAPPLICATIONDESCRIPTOR Descriptor,
    ULONG Action
    )
/*++

Routine Description:

    This routine repairs or replaces the specified application.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/
{
    Descriptor->Actions.Repair(
        Descriptor->Identifier,
        Action
        );
}