/*********************************************************************
Registration Wizard

exitdlg.cpp
10/13/94 - Tracy Ferrier
(c) 1994-95 Microsoft Corporation
Modification History:
	MXX1 
	Date :02/17/99: 
	By   :SK 
	Change Request :Cancel Dialog should not be displayed
	Function : CancelRegWizard()

**********************************************************************/

#include <Windows.h>
#include "Resource.h"
#include "regutil.h"
#include <stdio.h>

static INT_PTR CALLBACK ExitDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern _TCHAR szWindowsCaption[256];

INT_PTR CancelRegWizard(HINSTANCE hInstance,HWND hwndParentDlg)
/*********************************************************************
Returns TRUE if the user confirms that RegWizard should be canceled.
**********************************************************************/
{
//	HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hwndParentDlg,GWLP_HINSTANCE);
	
	// MXX1  --- Start
	//NT_PTR hitButton = DialogBox(hInstance,MAKEINTRESOURCE(IDD_CANCEL),hwndParentDlg, ExitDialogProc);
	//return hitButton == IDB_YES ? TRUE : FALSE;
	
	// MXX1  --- Finish

	// MXX1  --- Start
	return TRUE;
	// MXX1  --- Finish
}


INT_PTR CALLBACK ExitDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
/*********************************************************************
Proc for dialog displayed when the user hits the 'cancel' button
**********************************************************************/
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
		{
			RECT parentRect,dlgRect;
			HWND hwndParent = GetParent(hwndDlg);

			GetWindowRect(hwndParent,&parentRect);
			GetWindowRect(hwndDlg,&dlgRect);
			int newX = parentRect.left + (parentRect.right - parentRect.left)/2 - (dlgRect.right - dlgRect.left)/2;
			int newY = parentRect.top + (parentRect.bottom - parentRect.top)/2 - (dlgRect.bottom - dlgRect.top)/2;
			MoveWindow(hwndDlg,newX,newY,dlgRect.right - dlgRect.left,dlgRect.bottom - dlgRect.top,FALSE);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT1);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT2);
			
			SetWindowText(hwndDlg,szWindowsCaption);


            return TRUE;
		}
        case WM_COMMAND:
            switch (wParam)
            {
                case IDB_YES:
				case IDB_NO:
					EndDialog(hwndDlg,wParam);
					break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return FALSE;
}

