/*********************************************************************
Registration Wizard

InformDialog.cpp
10/19/94 - Tracy Ferrier
02/11/98 - Suresh Krishnan
(c) 1994-95 Microsoft Corporation
**********************************************************************/

#include <Windows.h>
#include "RegWizMain.h"
#include "Resource.h"
#include <RegPage.h>
#include "Dialogs.h"
#include "regutil.h"
#include <rw_common.h>

INT_PTR CALLBACK InformDialogProc(HWND hwndDlg, 
					   UINT uMsg, 
					   WPARAM wParam, LPARAM lParam)
/*********************************************************************
Dialog Proc for the Registration Wizard dialog that presents the
Product Identification number to the user.
**********************************************************************/
{
	CRegWizard* pclRegWizard;
	INT_PTR iRet;
	_TCHAR szInfo[256];
    INT_PTR bStatus;

	pclRegWizard = NULL;
	bStatus = TRUE;

	PageInfo *pi = (PageInfo *)GetWindowLongPtr( hwndDlg, GWLP_USERDATA );
	if(pi) {
		pclRegWizard = pi->pclRegWizard;
	}

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
			
			pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
			pi->iCancelledByUser = RWZ_PAGE_OK;

			pclRegWizard = pi->pclRegWizard;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, (LONG_PTR)pi );
			//SetControlFont( pi->hBigBoldFont, hwndDlg, IDT_TEXT1);
			pclRegWizard->GetInputParameterString(IDS_INPUT_PRODUCTNAME,szInfo);
			ReplaceDialogText(hwndDlg,IDT_TEXT2,szInfo);
			return TRUE;
		}
		case WM_NOTIFY:
        {   LPNMHDR pnmh = (LPNMHDR)lParam;
            switch( pnmh->code ){
            case PSN_SETACTIVE:
				pi->iCancelledByUser = RWZ_PAGE_OK;
                PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
                break;

            case PSN_WIZNEXT:
				if(pi->iCancelledByUser == RWZ_CANCELLED_BY_USER ) {
					pi->CurrentPage=pi->TotalPages-1;
					PropSheet_SetCurSel(GetParent(hwndDlg),NULL,pi->TotalPages-1);

				}else {
					pi->CurrentPage++;
				}
				break;

            case PSN_WIZBACK:
                pi->CurrentPage--;

                break;
			case PSN_QUERYCANCEL :
				iRet=0;
				if (CancelRegWizard(pclRegWizard->GetInstance(),hwndDlg)) {
					//pclRegWizard->EndRegWizardDialog(IDB_EXIT) ;
					iRet =0;
					iRet = 1;
					pi->ErrorPage  = kInformDialog;
					pi->iError     = RWZ_ERROR_CANCELLED_BY_USER;
					SetWindowLongPtr( hwndDlg,DWLP_MSGRESULT, (INT_PTR) iRet); 
					pi->iCancelledByUser = RWZ_CANCELLED_BY_USER;
					PropSheet_PressButton (GetParent( hwndDlg ),PSBTN_NEXT);

				}else {
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
        } // WM_Notify
		break;
		case WM_COMMAND:
        default:
		bStatus = FALSE;
         break;
    }
    return bStatus;
}

	