/*********************************************************************
Registration Wizard

msgdlg.cpp
11/22/94 - Tracy Ferrier
(c) 1994-95 Microsoft Corporation
**********************************************************************/

#include <Windows.h>
#include "Resource.h"
#include "regutil.h"
#include <stdio.h>
#include "sysinv.h"

#define REPLACE_TITLE	     0
#define RETAIN_DEFAUT_TITLE  1

extern _TCHAR szWindowsCaption[256];

static INT_PTR CALLBACK MsgDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static int iShowTitle =REPLACE_TITLE;

int RegWizardMessage(HINSTANCE hInstance,HWND hwndParent, int dlgID)
/*********************************************************************
Puts up the dialog box with the given ID, and maintains control until
the user dismisses it.  The ID of the control used for the dismissal
will be returned as the function result.

Note: pass NULL for hwndParent if there is to be no parent window
for the message dialog.
**********************************************************************/
{
	iShowTitle =REPLACE_TITLE;
	if(dlgID == IDD_INPUTPARAM_ERR ||
		dlgID == IDD_ANOTHERCOPY_ERROR ) {
		iShowTitle = RETAIN_DEFAUT_TITLE;
	}
	int hitButton = (int) DialogBoxParam(hInstance,MAKEINTRESOURCE(dlgID),hwndParent, MsgDialogProc, NULL);
	return hitButton;
}


int RegWizardMessageEx(HINSTANCE hInstance,HWND hwndParent, int dlgID, LPTSTR szSub)
/*********************************************************************
Puts up the dialog box with the given ID, and maintains control until
the user dismisses it.  The ID of the control used for the dismissal
will be returned as the function result.

If the specified dialog has a text field with IDT_TEXT1, and the
text within that field has a %s specifier, that specifier will be
replaced with the string pointed to by the szSub parameter.

Note: pass NULL for hwndParent if there is to be no parent window
for the message dialog.
**********************************************************************/
{
	iShowTitle =REPLACE_TITLE;
	int hitButton = (int) DialogBoxParam(hInstance,MAKEINTRESOURCE(dlgID),hwndParent, MsgDialogProc,(LPARAM) szSub);
	return hitButton;
}



INT_PTR CALLBACK MsgDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
/*********************************************************************
Proc for a standard "message" dialog box.
**********************************************************************/
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
		{
			RECT parentRect,dlgRect;
			HWND hwndParent = GetParent(hwndDlg);
			if (hwndParent)
			{
				GetWindowRect(hwndParent,&parentRect);
				GetWindowRect(hwndDlg,&dlgRect);
				int newX = parentRect.left + (parentRect.right - parentRect.left)/2 - (dlgRect.right - dlgRect.left)/2;
				int newY = parentRect.top + (parentRect.bottom - parentRect.top)/2 - (dlgRect.bottom - dlgRect.top)/2;
				MoveWindow(hwndDlg,newX,newY,dlgRect.right - dlgRect.left,dlgRect.bottom - dlgRect.top,FALSE);
			}
			else
			{
				int horiz,vert;
				GetDisplayCharacteristics(&horiz,&vert,NULL);
				GetWindowRect(hwndDlg,&dlgRect);
				int newX = horiz/2 - (dlgRect.right - dlgRect.left)/2;
				int newY = vert/2 - (dlgRect.bottom - dlgRect.top)/2;
				MoveWindow(hwndDlg,newX,newY,dlgRect.right - dlgRect.left,dlgRect.bottom - dlgRect.top,FALSE);
			}
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT1);

			if( iShowTitle ==REPLACE_TITLE )
			SetWindowText(hwndDlg,szWindowsCaption);

			if (lParam != NULL)
			{
				LPTSTR szSub = (LPTSTR) lParam;
				ReplaceDialogText(hwndDlg,IDT_TEXT1,szSub);
			}
            return TRUE;
		}
        case WM_COMMAND:
			EndDialog(hwndDlg,wParam);
			break;
        default:
            break;
    }
    return FALSE;
}

