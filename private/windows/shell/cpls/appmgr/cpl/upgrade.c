/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    upgrade.c

Abstract:

    This module contains code for the upgrade pages.

Author:

    Dave Hastings (daveh) creation-date 08-Jul-1997

Revision History:

--*/
#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "pip.h"
#include "appmgr.h"

VOID
UpgradeApplication(
    PAPPLICATIONDESCRIPTOR AppDescriptor
    );

static ULONG SelectedProgram;

BOOL
UpgradeProgramInitProc(
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
#if 0
	currently dead code
    ListViewWindow = GetDlgItem(DialogWindow, IDC_APPLICATION_LIST);

    // ApplicationListView = ListViewWindow;

    PopulateApplicationListView(ListViewWindow, PIP_UPGRADE);
#endif
    return TRUE;

}

INT
UpgradeProgramNotifyProc(
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
        case NM_CLICK:

            // bugbug
            WindowState = ItemSelected(DialogWindow, &SelectedProgram);
            // bugbug
            if (WindowState) {
                EnableWindow(
                    GetDlgItem(DialogWindow, IDC_BUTTONUPGRADE), 
                    TRUE
                    );
            } else {
                EnableWindow(
                    GetDlgItem(DialogWindow, IDC_BUTTONUPGRADE), 
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
                (ULONG)MAKEINTRESOURCE(IDD_UPGRADE_FINISH)
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
UpgradeProgramCommandProc(
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
    HCURSOR Wait, Original;
    
    switch(LOWORD(wParam)) {

        case IDC_BUTTONUPGRADE:
        
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
            
            //
            // Change the cursor
            //
            Wait = LoadCursor(NULL, IDC_WAIT);
            Original = SetCursor(Wait);

            UpgradeApplication((PAPPLICATIONDESCRIPTOR)Item.lParam);

            //
            // Put it back
            //
            SetCursor(Original);

            PostMessage(
                GetParent(DialogWindow), 
                PSM_SETWIZBUTTONS, 
                0, 
                PSWIZB_NEXT | PSWIZB_BACK
                );

            return TRUE;

        default:
            
            return FALSE;
    }
}

VOID
UpgradeApplication(
    PAPPLICATIONDESCRIPTOR AppDescriptor
    )
{
    // bugbug how to wait on the uninstall
    AppDescriptor->Actions.Upgrade(
        AppDescriptor->Identifier
        );

}
