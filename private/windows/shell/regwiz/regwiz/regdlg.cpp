/*********************************************************************
Registration Wizard

RegDlg.cpp
10/28/94 - Tracy Ferrier
02/12/98 - Suresh Krishnan
(c) 1994-95 Microsoft Corporation
08/20/98  : Removed the substitution of Product name in SubTitle
**********************************************************************/

#include <Windows.h>
#include "RegPage.h"
#include "RegWizMain.h"
#include "Resource.h"
#include "Dialogs.h"
#include "regutil.h"
#include <stdio.h>
#include "cntryinf.h"
#include <rw_common.h>
#include <rwpost.h>

static TCHAR szRegisterSubTitle[1024]=_T("");
static PROPSHEETPAGE  *spAddrSheet=NULL;
// Private functions
int LogFileQuery(HWND hwndParentDlg);
INT_PTR CALLBACK LogFileQueryDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK RegisterDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
/*********************************************************************
Main entry point for the Registration Wizard.
**********************************************************************/
{
	CRegWizard* pclRegWizard = NULL;
	static INT_PTR iRet;
	_TCHAR szInfo[256];
    INT_PTR bStatus = TRUE;

	PageInfo *pi = (PageInfo *)GetWindowLongPtr( hwndDlg, GWLP_USERDATA );
	if(pi)
	{
		pclRegWizard = pi->pclRegWizard;
	};

    switch (uMsg)
    {
		case WM_CLOSE:
			if (CancelRegWizard(pclRegWizard->GetInstance(),hwndDlg)) pclRegWizard->EndRegWizardDialog(IDB_EXIT);
            break;	
		case WM_DESTROY:
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, NULL );
			break;				
        case WM_INITDIALOG:
		{
			_TCHAR szInfo[256];
			pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
			pclRegWizard = pi->pclRegWizard;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, (LONG_PTR)pi );
			SetControlFont( pi->hBigBoldFont, hwndDlg, IDT_TEXT1);

			//UpgradeDlg(hwndDlg);
			
			//NormalizeDlgItemFont(hwndDlg,IDC_TITLE,RWZ_MAKE_BOLD);
			//NormalizeDlgItemFont(hwndDlg,IDC_SUBTITLE);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT1);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT2);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT3);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT4);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT5);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT6);
			
			pclRegWizard->GetInputParameterString(IDS_INPUT_PRODUCTNAME,szInfo);
			ReplaceDialogText(hwndDlg,IDT_TEXT1,szInfo);


			SetWindowText(hwndDlg,pclRegWizard->GetWindowCaption());
			
			
			
			if (pclRegWizard->GetInformationString(kInfoProductID,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDT_TEXT6,WM_SETTEXT,0,(LPARAM) szInfo);
			}
			/***
			// Update the Sub title by substituting the product name
			_TCHAR rgchCallingContext[256];
			pclRegWizard->GetInputParameterString(IDS_INPUT_PRODUCTNAME,rgchCallingContext);
			LoadString(pi->hInstance,IDS_REGISTER_SCR_STITLE,szRegisterSubTitle,256);
			_tcscat(szRegisterSubTitle,_T(" "));
 			_tcscat(szRegisterSubTitle,rgchCallingContext);
			_tcscat(szRegisterSubTitle,_T("."));
			spAddrSheet->pszHeaderSubTitle = szRegisterSubTitle;
			**/


			if (pclRegWizard->GetCountryCode() == kCountryCodeUnitedStates)
			{
				HWND hwndCtrl;
				hwndCtrl = GetDlgItem(hwndDlg,IDT_TEXT3);
				ShowWindow(hwndCtrl,SW_HIDE);
				hwndCtrl = GetDlgItem(hwndDlg,IDT_TEXT4);
				ShowWindow(hwndCtrl,SW_HIDE);
				hwndCtrl = GetDlgItem(hwndDlg,IDT_TEXT5);
				ShowWindow(hwndCtrl,SW_HIDE);
				hwndCtrl = GetDlgItem(hwndDlg,IDC_ENVELOPE);
				ShowWindow(hwndCtrl,SW_HIDE);
			}

            return TRUE;
		}// WM_INIT
		break;
		case WM_NOTIFY:
        {
			LPNMHDR pnmh = (LPNMHDR)lParam;
            switch( pnmh->code )
			{
            case PSN_SETACTIVE:
                //PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK PSWIZB_NEXT );
				pi->iCancelledByUser = RWZ_PAGE_OK;
				PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
                break;

            case PSN_WIZNEXT:
				iRet=0;
				if(pi->iCancelledByUser == RWZ_CANCELLED_BY_USER )
				{
					pi->CurrentPage=pi->TotalPages-1;
					PropSheet_SetCurSel(GetParent(hwndDlg),NULL,pi->TotalPages-1);
				}
				else
				{
					if (pclRegWizard->GetTriStateInformation(kInfoIncludeProducts) == kTriStateUndefined){	
						pclRegWizard->SetTriStateInformation(kInfoIncludeProducts,kTriStateFalse);
					}
					pclRegWizard->WriteInfoToRegistry();
					
					//
					// Check  for the type of Connectivity
					//
					if(pi->dwConnectionType == DIALUP_REQUIRED)
					{
						pi->CurrentPage++;
						// go to the Dialup Screen
					}
					else
					{
						//
						//
						// Post The  Date
						pi->iError =PostDataWithWindowMessage(pclRegWizard->GetInstance());
						pi->CurrentPage=pi->TotalPages-1;
						PropSheet_SetCurSel(GetParent(hwndDlg),NULL,pi->TotalPages-1);
					}
					pi->iLastKeyOperation = RWZ_NEXT_PRESSED;
				}

				//SetWindowLongPtr( hwndDlg ,DWLP_MSGRESULT, (INT_PTR) iRet);
				break;

            case PSN_WIZBACK:
				pi->iLastKeyOperation = RWZ_BACK_PRESSED;
                pi->CurrentPage--;

                break;
			case PSN_QUERYCANCEL :
				iRet=0;
				if (CancelRegWizard(pclRegWizard->GetInstance(),hwndDlg)) {
					//pclRegWizard->EndRegWizardDialog(IDB_EXIT) ;
					iRet = 1;
					pi->ErrorPage  = kRegisterDialog;
					pi->iError     = RWZ_ERROR_CANCELLED_BY_USER;
					SetWindowLongPtr( hwndDlg,DWLP_MSGRESULT, (INT_PTR) iRet);
					pi->iCancelledByUser = RWZ_CANCELLED_BY_USER;
					PropSheet_PressButton (GetParent( hwndDlg ),PSBTN_NEXT);
				}
				else
				{
					//
					// Prevent Cancell Operation as User does not want to Cancel
					iRet = 1;
				}
				SetWindowLongPtr( hwndDlg,DWLP_MSGRESULT, (INT_PTR) iRet);
				break;
				default:
                //bStatus = FALSE;
                break;
            }
        }
        break;

        default:
			bStatus = FALSE;
            break;
    }
    return bStatus;
}



int LogFileQuery(HWND hwndParentDlg)
/**********************************************************************
Puts up a dialog asking the user if he/she would like a text log of all
registration information.

Returns:
IDB_YES
IDB_NO
IDB_CANCEL
***********************************************************************/
{
	HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hwndParentDlg,GWLP_HINSTANCE);
	int hitButton = (int) DialogBox(hInstance,MAKEINTRESOURCE(IDD_LOGFILE_QUERY),hwndParentDlg,
		LogFileQueryDialogProc);
	return hitButton;
}


INT_PTR CALLBACK LogFileQueryDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
/*********************************************************************
Proc for our "Do you want a log file" dialog.
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

//
//  This function is calles during the creation and deletion of
//  Address Property Sheet
//  Store the Address of PPROPSHEETPAGE so the Subtitle can be changed
//
//
//
UINT CALLBACK RegisterPropSheetPageProc(HWND hwnd,
								UINT uMsg,
								LPPROPSHEETPAGE ppsp
								)
{
	
	switch(uMsg) {
	case PSPCB_CREATE :
		spAddrSheet = ppsp;
	default:
		break;

	}
	return 1;

}
