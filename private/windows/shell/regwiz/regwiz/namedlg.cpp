/*********************************************************************
Registration Wizard

NameDialog.cpp
10/13/94 - Tracy Ferrier
02/11/98 - Suresh Krishnan
(c) 1994-95 Microsoft Corporation
Modification History :
	MDX1 :   Suresh
	Date    : 2/12/99
	Function: Modified in void ConfigureNameEditFields(CRegWizard* pclRegWizard,HWND hwndDlg)
	Reason	: The Middle name is set to Null, This looks like the filed validation is not taking
	from the Resoure settings for each TAPI id
**********************************************************************/

#include <Windows.h>
#include "RegPage.h"
#include "RegWizMain.h"
#include "Resource.h"
#include "Dialogs.h"
#include "regutil.h"
#include "rw_common.h"

typedef enum
{
	kPurchaseUndefined,
	kPurchaseBySelf,
	kPurchaseByCompany
}PurchaseType;

static PurchaseType vPurchaseType = kPurchaseUndefined;
void ConfigureNameEditFields(CRegWizard* pclRegWizard,HWND hwndDlg);
BOOL ValidateNameDialog(HWND hwndDlg);
int ValidateNameEditFields(HWND hwndDlg);
BOOL GetDefaultCompanyName(CRegWizard* pclRegWizard,LPTSTR szValue);

INT_PTR CALLBACK NameDialogProc(	HWND hwndDlg,
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
    INT_PTR bStatus = TRUE;

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
			
			HWND hwndNameField = GetDlgItem(hwndDlg,IDC_EDIT1);
			SetFocus(hwndNameField);
			NormalizeDlgItemFont(hwndDlg,IDC_SUBTITLE);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT1);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT2);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT3);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT4);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT12);
			NormalizeDlgItemFont(hwndDlg,IDC_GROUP1);
			
			SetWindowText(hwndDlg,pclRegWizard->GetWindowCaption());

			pclRegWizard->GetInputParameterString(IDS_INPUT_PRODUCTNAME,szCallingContext);
			ReplaceDialogText(hwndDlg,IDT_TEXT1,szCallingContext);
			
			if (pclRegWizard->GetInformationString(kInfoFirstName,szInfo)){
				SendDlgItemMessage(hwndDlg,IDC_EDIT1,WM_SETTEXT,0,(LPARAM) szInfo);
				vPurchaseType = kPurchaseBySelf;
			}
			
			if (pclRegWizard->GetInformationString(kInfoLastName,szInfo)){
				SendDlgItemMessage(hwndDlg,IDC_EDIT2,WM_SETTEXT,0,(LPARAM) szInfo);
				vPurchaseType = kPurchaseBySelf;
			}
			if (pclRegWizard->GetInformationString(kMiddleName,szInfo)){
				SendDlgItemMessage(hwndDlg,IDC_EDIT4,WM_SETTEXT,0,(LPARAM) szInfo);
			}
			OutputDebugString(_T("\n Middle Name "));
			OutputDebugString(szInfo);

			if (pclRegWizard->GetInformationString(kInfoEmailName,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_EDIT5,WM_SETTEXT,0,(LPARAM) szInfo);
			}

			OutputDebugString(_T("\n Email "));
			OutputDebugString(szInfo);

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
					EnableWindow(GetDlgItem(hwndDlg,IDT_TEXT4),TRUE);
					EnableWindow(GetDlgItem(hwndDlg,IDB_NEXT),TRUE);
				}
				else
				{
					vPurchaseType = kPurchaseBySelf;
					CheckRadioButton(hwndDlg,IDC_RADIO1,IDC_RADIO2,IDC_RADIO1);
					EnableWindow(GetDlgItem(hwndDlg,IDC_EDIT3),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDT_TEXT4),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDB_NEXT),TRUE);
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
			ConfigureNameEditFields(pclRegWizard,hwndDlg);
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
					if( ValidateNameDialog(hwndDlg)) {
						SendDlgItemMessage(hwndDlg,IDC_EDIT1,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoFirstName,szInfo);
						SendDlgItemMessage(hwndDlg,IDC_EDIT2,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoLastName,szInfo);
						SendDlgItemMessage(hwndDlg,IDC_EDIT3,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoCompany,szInfo);
						SendDlgItemMessage(hwndDlg,IDC_EDIT4,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kMiddleName,szInfo);
						SendDlgItemMessage(hwndDlg,IDC_EDIT5,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoEmailName,szInfo);
						pclRegWizard->WriteEnableInformation(kInfoCompany,vPurchaseType == kPurchaseBySelf ? FALSE : TRUE);
						pclRegWizard->EndRegWizardDialog(wParam);
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
                if(!shouldEnable)
                {
                    SetDlgItemText(hwndDlg,IDC_EDIT3,_T(""));
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


BOOL GetDefaultCompanyName(CRegWizard* pclRegWizard,LPTSTR szValue)
{
	HKEY hKey;
	_TCHAR szKeyName[256];
	
	pclRegWizard->GetRegKey(szKeyName);	
	
	DWORD dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKeyName,0,KEY_READ,&hKey);

	if (dwStatus == ERROR_SUCCESS)
	{
	  _TCHAR szValueName[64];
	  unsigned long infoSize = 255;
	  LoadString(pclRegWizard->GetInstance(),IDS_COMPANY_NAME,szValueName,64);
	  dwStatus = RegQueryValueEx(hKey,szValueName,NULL,0,(LPBYTE) szValue,&infoSize);
	  if (dwStatus == ERROR_SUCCESS)
	  {
		return TRUE;
	  }
	}
	return FALSE;
}

BOOL ValidateNameDialog(HWND hwndDlg)
/*********************************************************************
Returns TRUE if all required user input is valid in the Name
dialog.  If any required edit field input is empty, ValidateNameDialog
will put up a message box informing the user of the problem, and set
the focus to the offending control.
**********************************************************************/
{
	int iInvalidEditField = ValidateNameEditFields(hwndDlg);
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



int ValidateNameEditFields(HWND hwndDlg)
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
	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT4)) return IDC_EDIT4;
	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT5)) return IDC_EDIT5;
	return NULL;
}

 
void ConfigureNameEditFields(CRegWizard* pclRegWizard,HWND hwndDlg)
/*********************************************************************
**********************************************************************/
{
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT1,kAddrSpecFirstName,IDT_TEXT2);
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT2,kAddrSpecLastName,IDT_TEXT3);
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT3,kAddrSpecCompanyName,IDT_TEXT4);
	//pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT4,kAddrMiddleName,IDT_TEXT5);	
	// comented on 2/12/99  to take Middle initial
	// MDX1
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT5,kAddrSpecEmailName,IDT_TEXT12);

}
