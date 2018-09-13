/*********************************************************************
Registration Wizard

Name_FE.cpp
03/11/98 - Suresh Krishanan
Name Screen for Far East Countries

Specification : 02/28/98
	- The Pronunciation field is enable only for JAPAN and for other
	FE countries it is disabled.

	- The USER ID is only for JAPAN and for other FarEast countries
	it should be deleted i.e do not display.

	- While sending the information to the back end, the name will
	be sent as FirstName and the Pronunciation will be
	sent as Last Name.
(c) 1994-95 Microsoft Corporation
**********************************************************************/

#include <Windows.h>
#include "RegPage.h"
#include "RegWizMain.h"
#include "Resource.h"
#include "Dialogs.h"
#include "regutil.h"
#include "rw_common.h"
#include <fe_util.h>


typedef enum
{
	kPurchaseUndefined,
	kPurchaseBySelf,
	kPurchaseByCompany
}PurchaseType;

static PurchaseType vPurchaseType = kPurchaseUndefined;
void ConfigureFENameEditFields(CRegWizard* pclRegWizard,HWND hwndDlg);
BOOL ValidateFENameDialog(HWND hwndDlg);
int ValidateFENameEditFields(HWND hwndDlg);
BOOL GetDefaultCompanyName(CRegWizard* pclRegWizard,LPTSTR szValue);

INT_PTR CALLBACK NameFEDialogProc(	HWND hwndDlg,
								UINT uMsg,
								WPARAM wParam,
								LPARAM lParam )
/*********************************************************************
Dialog Proc for the Registration Wizard dialog that requests the
user's name and company.
**********************************************************************/
{
	CRegWizard* pclRegWizard = NULL;
	static INT_PTR iRet;
	_TCHAR szInfo[256];
    BOOL bStatus = TRUE;

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
			_TCHAR szCallingContext[64];
			
			pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
			RW_DEBUG << "\n  INIT DIALOG " << pi->iCancelledByUser << flush;
			pi->iCancelledByUser = RWZ_PAGE_OK;
			pclRegWizard = pi->pclRegWizard;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, (LONG_PTR)pi );
			SetControlFont( pi->hBigBoldFont, hwndDlg, IDT_TEXT1);

			//UpgradeDlg(hwndDlg);
			
			
			NormalizeDlgItemFont(hwndDlg,IDC_SUBTITLE);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT1);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT2);
			
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT4); // Company
			NormalizeDlgItemFont(hwndDlg,IDT_DIVISION); //Division

			if(GetFeScreenType() == kFEWithJapaneaseScreen ) {
				// Enable USER ID
				ShowWindow(GetDlgItem(hwndDlg,IDT_USERID),SW_SHOW);
				ShowWindow(GetDlgItem(hwndDlg,IDC_USERID), SW_SHOW);
				EnableWindow (GetDlgItem(hwndDlg,IDC_USERID), TRUE);
				NormalizeDlgItemFont(hwndDlg,IDT_USERID); //Division
			}

			NormalizeDlgItemFont(hwndDlg,IDC_GROUP1);
			
			SetWindowText(hwndDlg,pclRegWizard->GetWindowCaption());

			//pclRegWizard->GetInputParameterString(IDS_INPUT_PRODUCTNAME,szCallingContext);
			//ReplaceDialogText(hwndDlg,IDT_TEXT1,szCallingContext);
			
			if (pclRegWizard->GetInformationString(kInfoFirstName,szInfo)){
				SendDlgItemMessage(hwndDlg,IDC_EDIT1,WM_SETTEXT,0,(LPARAM) szInfo);
				vPurchaseType = kPurchaseBySelf;
			}


			if(GetFeScreenType() == kFEWithJapaneaseScreen ) {
				// FOR JAPAN fill the Pronunciation info  and User ID
				if (pclRegWizard->GetInformationString(kInfoLastName,szInfo)){
					SendDlgItemMessage(hwndDlg,IDC_EDIT2,WM_SETTEXT,0,(LPARAM) szInfo);
				}
				if (pclRegWizard->GetInformationString(kUserId,szInfo)){
					SendDlgItemMessage(hwndDlg,IDC_USERID,WM_SETTEXT,0,(LPARAM) szInfo);
				}
			}else {
				//
				// Disable Pronunciation for other FE countries
				EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT2),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDT_TEXT3),FALSE);
			}


             // fix 381069

            if (pclRegWizard->GetInformationString(kInfoEmailName,szInfo))
            {
                SendDlgItemMessage(hwndDlg,IDC_EDIT4,WM_SETTEXT,0,(LPARAM) szInfo);
            }			
			
			BOOL isCompanyNameValid = FALSE;
			if (pclRegWizard->GetInformationString(kInfoCompany,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_EDIT3,WM_SETTEXT,0,(LPARAM) szInfo);
				if (szInfo[0])
				{
					isCompanyNameValid = TRUE;
					vPurchaseType = kPurchaseByCompany;
					CheckRadioButton(hwndDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO2);
					EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT3),TRUE);
					SendDlgItemMessage(hwndDlg,IDC_EDIT3,WM_SETTEXT,0,(LPARAM) szInfo);
					EnableWindow(GetDlgItem(hwndDlg,IDT_TEXT4),TRUE);
					EnableWindow(GetDlgItem(hwndDlg,IDB_NEXT),TRUE);
					// Enable  Division Name
					EnableWindow(GetDlgItem(hwndDlg,IDC_DIVISION),TRUE);
					EnableWindow(GetDlgItem(hwndDlg,IDT_DIVISION),TRUE);
					//
					//
					if (pclRegWizard->GetInformationString(kDivisionName,szInfo)){
						SendDlgItemMessage(hwndDlg,IDC_DIVISION,WM_SETTEXT,0,(LPARAM) szInfo);
					}

				}
				else
				{
					vPurchaseType = kPurchaseBySelf;
					CheckRadioButton(hwndDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO1);
					EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT3),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDT_TEXT4),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDB_NEXT),TRUE);
					// Disable Division Name
					EnableWindow(GetDlgItem(hwndDlg,IDC_DIVISION),FALSE);
				}
			}
			else
			{
				if(	vPurchaseType != kPurchaseBySelf)
				{
					vPurchaseType = kPurchaseUndefined;
					EnableWindow(GetDlgItem(hwndDlg,IDB_NEXT),FALSE);
				}
				else
				{
					CheckRadioButton(hwndDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO1);
					EnableWindow(GetDlgItem(hwndDlg,IDB_NEXT),TRUE);
				}
			}

			SendDlgItemMessage(hwndDlg,IDC_EDIT1,EM_SETSEL,0,-1);
			ConfigureFENameEditFields(pclRegWizard,hwndDlg);

			HWND hwndNameField = GetDlgItem(hwndDlg,IDC_EDIT1);
			SetFocus(hwndNameField);

			vDialogInitialized = TRUE;
			//pclRegWizard->ActivateRegWizardDialog();
            return TRUE;
		}
		break;
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
				// Cancel is not Pressed
					iRet=0;
					if( ValidateFENameDialog(hwndDlg)) {
						SendDlgItemMessage(hwndDlg,IDC_EDIT1,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoFirstName,szInfo);
						// pronunciation
						if ( GetFeScreenType() == kFEWithJapaneaseScreen ) {
							SendDlgItemMessage(hwndDlg,IDC_EDIT2,WM_GETTEXT,255,(LPARAM) szInfo);
							pclRegWizard->SetInformationString(kInfoLastName,szInfo);
						}

						SendDlgItemMessage(hwndDlg,IDC_EDIT3,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoCompany,szInfo);

						// Division
						SendDlgItemMessage(hwndDlg,IDC_DIVISION,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kDivisionName,szInfo);

                        // E-mail:  fix 381069 
 
                        SendDlgItemMessage(hwndDlg,IDC_EDIT4,WM_GETTEXT,255,(LPARAM) szInfo);
                        pclRegWizard->SetInformationString(kInfoEmailName,szInfo);
						
						// User ID
						if ( GetFeScreenType() == kFEWithJapaneaseScreen ) {
							SendDlgItemMessage(hwndDlg,IDC_USERID,WM_GETTEXT,255,(LPARAM) szInfo);
							pclRegWizard->SetInformationString(kUserId,szInfo);
						}


						pclRegWizard->WriteEnableInformation(kInfoCompany,vPurchaseType == kPurchaseBySelf ? FALSE : TRUE);
						// pclRegWizard->EndRegWizardDialog(wParam);
						pi->iLastKeyOperation = RWZ_NEXT_PRESSED;
						pi->CurrentPage++;
					
					}else {
						// Force it it be in this screen
						iRet=-1;
					}
					SetWindowLongPtr( hwndDlg ,DWLP_MSGRESULT, (INT_PTR) iRet);
				}
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
					pi->ErrorPage  = kNameDialog;
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
        }
        break;
		case WM_COMMAND:{
            if (vPurchaseType == kPurchaseUndefined){
                PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK );
            }
            BOOL selfChecked = IsDlgButtonChecked(hwndDlg,IDC_RADIO1);
            BOOL companyChecked = IsDlgButtonChecked(hwndDlg,IDC_RADIO2);
            if (selfChecked)
            {
                vPurchaseType = kPurchaseBySelf;
                PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_NEXT | PSWIZB_BACK );
                //EnableWindow(GetDlgItem(hwndDlg,IDB_NEXT),TRUE);
            }
            else if (companyChecked)
            {
                vPurchaseType = kPurchaseByCompany;
                PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_NEXT | PSWIZB_BACK);
                // EnableWindow(GetDlgItem(hwndDlg,IDB_NEXT),TRUE);
            }
            BOOL shouldEnable = vPurchaseType == kPurchaseByCompany ? TRUE : FALSE;
            HWND hwndCompanyField = GetDlgItem(hwndDlg,IDC_EDIT3);
            HWND hwndCompanyLabel = GetDlgItem(hwndDlg,IDT_TEXT4);
            if (IsWindowEnabled(hwndCompanyField) != shouldEnable)
            {
                EnableWindow(hwndCompanyField,shouldEnable);
                EnableWindow(hwndCompanyLabel,shouldEnable);
                // Division enable
                EnableWindow(GetDlgItem(hwndDlg,IDT_DIVISION),shouldEnable);
                EnableWindow(GetDlgItem(hwndDlg,IDC_DIVISION),shouldEnable);

                if(!shouldEnable)
                {
                    SetDlgItemText(hwndDlg,IDC_EDIT3,_T(""));
                    SetDlgItemText(hwndDlg,IDC_DIVISION,_T(""));
                }
                else
                {
                    SendDlgItemMessage(hwndDlg,IDC_EDIT3,WM_GETTEXT,255,
                                                            (LPARAM) szInfo);
                    if(!szInfo[0])
                    {
                        if(GetDefaultCompanyName(pclRegWizard,szInfo))	
                        {
                            SendDlgItemMessage(hwndDlg,IDC_EDIT3,WM_SETTEXT,
                                                        255,(LPARAM) szInfo);
                        }
                    }
                }
            }
		 }// WM_COMMAND
		break;
        default:
		bStatus = FALSE;
        break;
    }
    return bStatus;
}


BOOL ValidateFENameDialog(HWND hwndDlg)
/*********************************************************************
Returns TRUE if all required user input is valid in the Name
dialog.  If any required edit field input is empty, ValidateFENameDialog
will put up a message box informing the user of the problem, and set
the focus to the offending control.
**********************************************************************/
{
	int iInvalidEditField = ValidateFENameEditFields(hwndDlg);
	if (iInvalidEditField == NULL)
	{
		return TRUE;
	}
	else
	{
		_TCHAR szAttached[256];
		_TCHAR szMessage[256];
		CRegWizard::GetEditTextFieldAttachedString(hwndDlg,iInvalidEditField,szAttached,256);
		HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hwndDlg,GWLP_HINSTANCE);
		LoadAndCombineString(hInstance,szAttached,IDS_BAD_PREFIX,szMessage);
		RegWizardMessageEx(hInstance,hwndDlg,IDD_INVALID_DLG,szMessage);
		HWND hwndNameField = GetDlgItem(hwndDlg,iInvalidEditField);
		SetFocus(hwndNameField);
		return FALSE;
	}
}



int ValidateFENameEditFields(HWND hwndDlg)
/*********************************************************************
ValidateAddrEditFields validates all edit fields in the Address
dialog.  If any required field is empty, the ID of the edit field
control will be returned as the function result.  If all fields are
OK, NULL will be returned.
**********************************************************************/
{
	
	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT1)) return IDC_EDIT1;
	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT2)) return IDC_EDIT2;
	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT3)) return IDC_EDIT3;
	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_DIVISION)) return IDC_DIVISION;
	if (GetFeScreenType() == kFEWithJapaneaseScreen ) {
		if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_USERID)) return IDC_USERID;
	}
	return NULL;
}


void ConfigureFENameEditFields(CRegWizard* pclRegWizard,HWND hwndDlg)
/*********************************************************************
**********************************************************************/
{
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT1,kAddrSpecFirstName,IDT_TEXT2);
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT2,kAddrSpecLastName,IDT_TEXT3);
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT3,kAddrSpecCompanyName,IDT_TEXT4);
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_DIVISION,kAddrSpecDivision,IDT_DIVISION);
	if(GetFeScreenType() == kFEWithJapaneaseScreen ) {
		pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_USERID,kAddrSpecUserId,IDT_USERID);

	}
}
