/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    modify.c

Abstract:

    This module contains page specific routines for the modify/remove pages.

Author:

    Dave Hastings (daveh) creation-date-29-Apr-1997

Notes:

    Bugbug daveh  What do we do about error handling?  Suppose the
    ListView_InsertColumn fails for example.  Do we put up a message
    box?

Revision History:


--*/
#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "pip.h"
#include "appmgr.h"

VOID
RemoveApplication(
    PAPPLICATIONDESCRIPTOR AppDescriptor
    );

VOID
ModifyApplication(
    PAPPLICATIONDESCRIPTOR AppDescriptor
    );


static VOID
PopulateRemoveListView(
    HWND ListViewWindow
    );

static ULONG SelectedProgram = 0xFFFFFFFF;
static HWND ApplicationListView;

BOOL
ModifyProgramInitProc(
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

			ApplicationListView = ListViewWindow;

            CreateApplicationListViewColumns(ListViewWindow);

			PopulateApplicationListView(ListViewWindow, PIP_UNINSTALL | PIP_MODIFY);

            PageInfo[GetWindowLong(DialogWindow, DWL_USER)].Dialog = DialogWindow;

			return TRUE;

		default:

			return TRUE;
	}
}

INT
ModifyProgramNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    )
/*++

Routine Description:

    This function handles the WM_NOTIFY messages for the application selection
    page for remove programs.

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

       case LVN_KEYDOWN:
       case NM_CLICK:

            // bugbug
            WindowState = ItemSelected(DialogWindow, &SelectedProgram);
            // bugbug
            if (WindowState) {
                //
                // Get the capabilites bit
                //
                Item.iItem = SelectedProgram;
                Item.iSubItem = 0;
                Item.mask = LVIF_PARAM;
                ListView_GetItem(
                    GetDlgItem(DialogWindow, IDC_APPLICATION_LIST),
                    &Item
                    );
                Descriptor = (PAPPLICATIONDESCRIPTOR)Item.lParam;
                
                //
                // enable the proper buttons
                //
                if ((Descriptor->Capabilities & PIP_UNINSTALL) && 
                    !(FeatureMask & FEATURE_REMOVE)
                ) {
                    EnableWindow(
                        GetDlgItem(DialogWindow, IDC_MODIFY_REMOVE), 
                        TRUE
                        );
                } else {
                    EnableWindow(
                        GetDlgItem(DialogWindow, IDC_MODIFY_REMOVE), 
                        FALSE
                        );
                }

                // bugbug check for the capability
                EnableWindow(GetDlgItem(DialogWindow, IDC_BUTTON_PROPERTIES), TRUE);

                if ((Descriptor->Capabilities & PIP_MODIFY) &&
                    !(FeatureMask & FEATURE_MODIFY)
                ) {
                    EnableWindow(
                        GetDlgItem(DialogWindow, IDC_BUTTON_MODIFY), 
                        TRUE
                        );
                } else {
                    EnableWindow(
                        GetDlgItem(DialogWindow, IDC_BUTTON_MODIFY), 
                        FALSE
                        );
                }
            } else {
                EnableWindow(
                    GetDlgItem(DialogWindow, IDC_MODIFY_REMOVE), 
                    FALSE
                    );
                EnableWindow(
                    GetDlgItem(DialogWindow, IDC_BUTTON_MODIFY), 
                    FALSE
                    );
                EnableWindow(
                    GetDlgItem(DialogWindow, IDC_BUTTON_PROPERTIES), 
                    FALSE
                    );
            }

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
            // Send us on to the next page
            //
            CheckSetWindowLong(
                DialogWindow, 
                DWL_MSGRESULT, 
                (ULONG)MAKEINTRESOURCE(IDD_MODIFY_FINISH)
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
            EnableWindow(GetDlgItem(DialogWindow, IDC_MODIFY_REMOVE), WindowState);

            CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, 0);
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
ModifyProgramCommandProc(
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
    HCURSOR Cursor;
    
    switch(LOWORD(wParam)) {

        case IDC_MODIFY_REMOVE:
        
            //
            // Get the descriptor of the selected application
            //
            Item.mask = LVIF_PARAM;
            Item.iItem = SelectedProgram;
            Item.iSubItem = 0;
            ListView_GetItem(
                ApplicationListView,
                &Item
                );
            
            //
            // Disable the wizard and put up the hourglass
            //
            Cursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            EnableWindow(
                GetParent(DialogWindow),
                FALSE
                );


            RemoveApplication((PAPPLICATIONDESCRIPTOR)Item.lParam);

            //
            // Renable the wizard
            //
            EnableWindow(
                GetParent(DialogWindow),
                TRUE
                );

            SetCursor(Cursor);

            PostMessage(
                GetParent(DialogWindow), 
                PSM_SETWIZBUTTONS, 
                0, 
                PSWIZB_NEXT | PSWIZB_BACK
                );

            PostUpdateMessage();

            return TRUE;

        case IDC_BUTTON_MODIFY:
        
            //
            // Get the descriptor of the selected application
            //
            Item.mask = LVIF_PARAM;
            Item.iItem = SelectedProgram;
            Item.iSubItem = 0;
            ListView_GetItem(
                ApplicationListView,
                &Item
                );
            
            // bugbug disable the modify button
            ModifyApplication((PAPPLICATIONDESCRIPTOR)Item.lParam);

            PostMessage(
                GetParent(DialogWindow), 
                PSM_SETWIZBUTTONS, 
                0, 
                PSWIZB_NEXT | PSWIZB_BACK
                );
            // bugbug refresh list view
            return TRUE;


        case IDC_BUTTON_PROPERTIES:

            //
            // Get the descriptor of the selected application
            //
            Item.mask = LVIF_PARAM;
            Item.iItem = SelectedProgram;
            Item.iSubItem = 0;
            ListView_GetItem(
                ApplicationListView,
                &Item
                );

            ShowProperties((PAPPLICATIONDESCRIPTOR)Item.lParam, DialogWindow);

            return TRUE;

        default:
            
            return FALSE;
    }
}

INT
ModifyProgramUpdateListViewProc(
    HWND Dialog,
    WPARAM wParam,
    LPARAM lParam,
    UINT PageIndex
    )
/*++

Routine Description:

    This routine handles the WM_UPDATELISTVIEW message.

Arguments:

    Dialog - Supplies the handle of the dialog window
    wParam - Supplies the wParam for the message
    lParam - Supplies the lParam for the message
    PageIndex - Supplies the index of the property page data

Return Value:

    TRUE if the message is handled.

--*/
{
    HWND ListView;

    ListView = GetDlgItem(Dialog, IDC_APPLICATION_LIST);


    UpdateApplicationLists(ListView, PIP_UNINSTALL | PIP_MODIFY);

    return TRUE;
}

INT
ModifyFinishNotifyProc(
    HWND DialogWindow,
    WPARAM wParam,
    LPNMHDR NotifyHeader,
    UINT PageIndex
    )
/*++

Routine Description:

    This function handles the WM_NOTIFY messages for the application selection
    page for remove programs.

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
    WCHAR ApplicationName[256];
    LV_ITEM Item;
    PAPPLICATIONDESCRIPTOR AppDescriptor;

    switch (NotifyHeader->code) {
        case PSN_SETACTIVE:
            //
            // set the correct wizard buttons
            //
            
            PostMessage(
                GetParent(DialogWindow), 
                PSM_SETWIZBUTTONS, 
                0, 
                PSWIZB_BACK | PSWIZB_FINISH
                );
            
            //
            // Set the application name to the correct text
            //
            ListView_GetItemText(
                ApplicationListView,
                SelectedProgram,
                0,
                ApplicationName,
                256
                );

            SendMessage(
                GetDlgItem(DialogWindow, IDC_APPLICATION_NAME),
                WM_SETTEXT,
                0,
                (LPARAM)ApplicationName
                );


            CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, 0);

            return TRUE;

        case PSN_WIZFINISH:
#if 0
            //
            // User has pushed the finish button, so go uninstall the application
            //
            Item.mask = LVIF_PARAM;
            Item.iItem = SelectedProgram;
            Item.iSubItem = 0;
            ListView_GetItem(
                ApplicationListView,
                &Item
                );

            //
            // Start up the uninstall
            //
            AppDescriptor = (PAPPLICATIONDESCRIPTOR)Item.lParam;
            AppDescriptor->Actions.Uninstall(
                AppDescriptor->Identifier
                );
#endif
            CheckSetWindowLong(DialogWindow, DWL_MSGRESULT, 0);

            return TRUE;


        default:
            return FALSE;
    }
}

VOID
RemoveApplication(
    PAPPLICATIONDESCRIPTOR AppDescriptor
    )
{
    // bugbug how to wait on the uninstall
    AppDescriptor->Actions.Uninstall(
        AppDescriptor->Identifier
        );

}

VOID
ModifyApplication(
    PAPPLICATIONDESCRIPTOR AppDescriptor
    )
{
    // bugbug how to wait on the uninstall
    AppDescriptor->Actions.Modify(
        AppDescriptor->Identifier
        );

}

