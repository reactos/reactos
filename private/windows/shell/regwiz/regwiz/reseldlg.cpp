/*********************************************************************
Registration Wizard

ResellerDialog
10/19/94 - Tracy Ferrier
02/12/98 - Suresh Krishnan
(c) 1994-95 Microsoft Corporation
**********************************************************************/

#include <Windows.h>
#include <stdio.h>
#include <RegPage.h>
#include "RegWizMain.h"
#include "Resource.h"
#include "Dialogs.h"
#include "regutil.h"
#include  <rw_common.h>

static int vDeclineOffers = -1;
static PROPSHEETPAGE  *spAddrSheet=NULL;
static TCHAR  szResellerSubTitle[256]=_T(""); // used for Sub Title

void ConfigureResellerEditFields(CRegWizard* pclRegWizard,HWND hwndDlg);
BOOL ValidateResellerDialog(HWND hwndDlg);
int ValidateResellerEditFields(HWND hwndDlg);

///////////////////////////////
//#define CREATE_TAB_ORDER_FILE
///////////////////////////////

INT_PTR CALLBACK ResellerDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
/*********************************************************************
Dialog Proc for the Registration Wizard dialog that requests the
name, city, and state of the reseller that sold the software to the
user.
**********************************************************************/
{
	CRegWizard* pclRegWizard;
	INT_PTR iRet;
	_TCHAR szInfo[256];
    INT_PTR bStatus;
	TriState shouldInclude;

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
			pclRegWizard = pi->pclRegWizard;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, (LONG_PTR)pi );
			SetControlFont( pi->hBigBoldFont, hwndDlg, IDT_TEXT1);
			
			//UpgradeDlg(hwndDlg);
			HWND hwndResellerNameField = GetDlgItem(hwndDlg,IDC_EDIT1);
			SetFocus(hwndResellerNameField);
			NormalizeDlgItemFont(hwndDlg,IDC_TITLE, RWZ_MAKE_BOLD);
			NormalizeDlgItemFont(hwndDlg,IDC_SUBTITLE);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT1);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT2);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT3);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT9);
						
			//SetWindowText(hwndDlg,pclRegWizard->GetWindowCaption());

			_TCHAR rgchCallingContext[256];
			pclRegWizard->GetInputParameterString(IDS_INPUT_PRODUCTNAME,rgchCallingContext);
			LoadString(pi->hInstance,IDS_RESELLER_SCR_STITLE,szResellerSubTitle,256);
			_tcscat(szResellerSubTitle,_T(" "));
 			_tcscat(szResellerSubTitle,rgchCallingContext);
			_tcscat(szResellerSubTitle,_T("?"));
			spAddrSheet->pszHeaderSubTitle = szResellerSubTitle;


			//ReplaceDialogText(hwndDlg,IDC_SUBTITLE,rgchCallingContext);
			if (pclRegWizard->GetInformationString(kInfoResellerName,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_EDIT1,WM_SETTEXT,0,(LPARAM) szInfo);
			}

			if (pclRegWizard->GetInformationString(kInfoResellerCity,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_EDIT2,WM_SETTEXT,0,(LPARAM) szInfo);
			}
			else if (pclRegWizard->GetInformationString(kInfoCity,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_EDIT2,WM_SETTEXT,0,(LPARAM) szInfo);
			}

			if (pclRegWizard->GetInformationString(kInfoResellerState,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_EDIT3,WM_SETTEXT,0,(LPARAM) szInfo);
			}
			else if (pclRegWizard->GetInformationString(kInfoState,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_EDIT3,WM_SETTEXT,0,(LPARAM) szInfo);
			}

			ConfigureResellerEditFields(pclRegWizard,hwndDlg);

			SendDlgItemMessage(hwndDlg,IDC_EDIT1,EM_SETSEL,0,-1);
			// To remove the default checking of the radio button for the first entry

			shouldInclude = pclRegWizard->GetTriStateInformation(kInfoDeclinesNonMSProducts);
			if (shouldInclude == kTriStateTrue )
			{
				CheckRadioButton(hwndDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO1);
				PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
				vDeclineOffers = 0;
			}
			else if (shouldInclude == kTriStateFalse)
			{
				CheckRadioButton(hwndDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO2);
				PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
				vDeclineOffers = 1;
			}
			else if (shouldInclude == kTriStateUndefined)
			{
				PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK );
				vDeclineOffers = -1;
			}
			
			vDialogInitialized = TRUE;
            return TRUE;
		} // WM_INIT
		break;
		case WM_NOTIFY:
        {   LPNMHDR pnmh = (LPNMHDR)lParam;
            switch( pnmh->code ){
            case PSN_SETACTIVE:
				RW_DEBUG << "\n PSN_ACTIVE   " << (ULONG)wParam << flush;
                //PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK PSWIZB_NEXT );
				pi->iCancelledByUser = RWZ_PAGE_OK;
				//PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK  );
				shouldInclude = pclRegWizard->GetTriStateInformation(kInfoDeclinesNonMSProducts);
				if (shouldInclude == kTriStateTrue ){
					CheckRadioButton(hwndDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO1);
					PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
					vDeclineOffers = 0;
				}
				if (shouldInclude == kTriStateFalse){
					CheckRadioButton(hwndDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO2);
					PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
					vDeclineOffers = 1;
				}
				if (shouldInclude == kTriStateUndefined){
					PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK );
					vDeclineOffers = -1;
				}
				// Enable for previpously entred value in screen
				if(IsDlgButtonChecked(hwndDlg,IDC_RADIO1)){
					PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
				}
				if(IsDlgButtonChecked(hwndDlg,IDC_RADIO2)){
					PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
				}

                break;

            case PSN_WIZNEXT:
				RW_DEBUG << "\n PSN_NEXT  " << (ULONG)wParam << flush;
				
				if(pi->iCancelledByUser == RWZ_CANCELLED_BY_USER ) {
					pi->CurrentPage=pi->TotalPages-1;
					PropSheet_SetCurSel(GetParent(hwndDlg),NULL,pi->TotalPages-1);

				}else {
					iRet=0;
					if( ValidateResellerDialog(hwndDlg) ) {
						pclRegWizard->EndRegWizardDialog((int) wParam);
						SendDlgItemMessage(hwndDlg,IDC_EDIT1,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoResellerName,szInfo);
						SendDlgItemMessage(hwndDlg,IDC_EDIT2,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoResellerCity,szInfo);
						SendDlgItemMessage(hwndDlg,IDC_EDIT3,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoResellerState,szInfo);
						if(vDeclineOffers == -1){
							pclRegWizard->SetTriStateInformation(kInfoDeclinesNonMSProducts,kTriStateUndefined);
						}
						else
						if(vDeclineOffers == 0){
							pclRegWizard->SetTriStateInformation(kInfoDeclinesNonMSProducts,kTriStateFalse);
						}
						else{
							pclRegWizard->SetTriStateInformation(kInfoDeclinesNonMSProducts,kTriStateTrue);
						}
						_stprintf(szInfo,_T("%i"),vDeclineOffers);
						pclRegWizard->SetInformationString(kInfoDeclinesNonMSProducts,szInfo);
						pi->CurrentPage++;
						pi->iLastKeyOperation = RWZ_NEXT_PRESSED;
					}else {
					// Force it it be in this screen
					iRet=-1;
					}
					SetWindowLongPtr( hwndDlg ,DWLP_MSGRESULT, (INT_PTR) iRet);
				}
				break;

            case PSN_WIZBACK:
				
				pi->iLastKeyOperation = RWZ_BACK_PRESSED;
				RW_DEBUG << "\n PSN_BACK  " << (ULONG)wParam << flush;
                pi->CurrentPage--;

                break;
			case PSN_QUERYCANCEL :
					RW_DEBUG << "\n PSN_CANCEL  " << (ULONG)wParam << flush;
				if (CancelRegWizard(pclRegWizard->GetInstance(),hwndDlg)) {
					//pclRegWizard->EndRegWizardDialog(IDB_EXIT) ;
					iRet = 1;
					pi->ErrorPage  = kProductInventoryDialog;
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
		{
			
            switch (wParam)
            {
              case  IDC_RADIO1:
			  case  IDC_RADIO2:
				if (vDialogInitialized){
						// If the 'No' button is checked, the user is declining
						// the "Non-Microsoft product" offers
						if(IsDlgButtonChecked(hwndDlg,IDC_RADIO1))
						{
							vDeclineOffers = 1;
							PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
							//EnableWindow(GetDlgItem(hwndDlg,IDB_NEXT),TRUE);
						}
						else
						if(IsDlgButtonChecked(hwndDlg,IDC_RADIO2))
						{
							vDeclineOffers = 0;
							PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
							//EnableWindow(GetDlgItem(hwndDlg,IDB_NEXT),TRUE);
						}
						else
						{
							vDeclineOffers = -1;
							PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK  );
							//EnableWindow(GetDlgItem(hwndDlg,IDB_NEXT),FALSE);
						}

					}
                    break;
			  default:
				  break;
			}
		}// WM_COMMAND
		break;
		
		
		default :
		bStatus = FALSE;

        break;
    }
    return bStatus;
}






BOOL ValidateResellerDialog(HWND hwndDlg)
/*********************************************************************
Returns TRUE if all required user input is valid in the Reseller
dialog.  If any required edit field input is empty,
ValidateResellerDialog will put up a message box informing the user
of the problem, and set the focus to the offending control.
**********************************************************************/
{
	int iInvalidEditField = ValidateResellerEditFields(hwndDlg);
	if (iInvalidEditField == NULL)
	{
		return TRUE;
	}
	else
	{
		_TCHAR szLabel[128];
		_TCHAR szMessage[256];
		CRegWizard::GetEditTextFieldAttachedString(hwndDlg,iInvalidEditField,szLabel,128);
		HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hwndDlg,GWLP_HINSTANCE);
		LoadAndCombineString(hInstance,szLabel,IDS_BAD_PREFIX,szMessage);
		RegWizardMessageEx(hInstance,hwndDlg,IDD_INVALID_DLG,szMessage);
		HWND hwndResellerField = GetDlgItem(hwndDlg,iInvalidEditField);
		SetFocus(hwndResellerField);
		return FALSE;
	}
}


int ValidateResellerEditFields(HWND hwndDlg)
/*********************************************************************
ValidateResellerEditFields validates all edit fields in the Reseller
dialog.  If any required field is empty, the ID of the first empty
edit field control will be returned as the function result.  If all
fields are OK, NULL will be returned.
**********************************************************************/
{
	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT1)) return IDC_EDIT1;
	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT2)) return IDC_EDIT2;
	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT3)) return IDC_EDIT3;
	return NULL;
}


void ConfigureResellerEditFields(CRegWizard* pclRegWizard,HWND hwndDlg)
/*********************************************************************
**********************************************************************/
{
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT1,kAddrSpecResellerName,IDT_TEXT1);
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT2,kAddrSpecResellerCity,IDT_TEXT2);
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT3,kAddrSpecResellerState,IDT_TEXT3);
}



//
//  This function is calles during the creation and deletion of
//  Address Property Sheet
//  Store the Address of PPROPSHEETPAGE so the Subtitle can be changed
//
//
//
UINT CALLBACK ResellerPropSheetPageProc(HWND hwnd,
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
