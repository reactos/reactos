/*********************************************************************
Registration Wizard
Addr_fe.cpp
Address Dialog screen for Far East Countries
In getting the phone numbers the Area Code , Phone Number and Extension is got seperately.
And while sending the information to the backend the Area code is combined with Phone number. 

03/10/98 - Suresh Krishnan
(c) 1994-95 Microsoft Corporation
**********************************************************************/
#include <Windows.h>
#include <regpage.h>

#include <Winnt.h>
#include <stdio.h>
#include "RegWizMain.h"
#include "Resource.h"
#include "Dialogs.h"
#include "regutil.h"
#include "cstattxt.h"
#include "cbitmap.h"
#include "cntryinf.h"
#include <rw_common.h>


static PROPSHEETPAGE  *spAddrSheet=NULL;
//static int vDeclineOffers = -1;
void ConfigureFEAddrEditFields(CRegWizard* pclRegWizard,HWND hwndDlg);
void SetRegWizardCountryCode(CRegWizard* pclRegWizard,HWND hwndDlg);
BOOL ValidateFEAddrDialog(CRegWizard* pclRegWizard,HWND hwndDlg);
int ValidateFEAddrEditFields(CRegWizard* pclRegWizard,HWND hwndDlg);

///////////////////////////////
//#define CREATE_TAB_ORDER_FILE
///////////////////////////////

#ifdef CREATE_TAB_ORDER_FILE
void CreateAddrDlgTabOrderString(HWND hwndDlg);
BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam);
#endif
INT_PTR CALLBACK AddressFEDialogProc(HWND hwndDlg, 
					UINT uMsg, 
					WPARAM wParam, LPARAM lParam)
/*********************************************************************
Dialog Proc for the Registration Wizard dialog that requests the 
user's address,phone, etc.
**********************************************************************/
{
	CRegWizard* pclRegWizard;
	INT_PTR iRet;
	_TCHAR szInfo[256];
	_TCHAR szTemp[256];
    INT_PTR bStatus;
	static int iXY = 0;
	HWND hWnd;
	
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
						
	
		
			HWND hwndStateField = GetDlgItem(hwndDlg,IDC_EDIT4);
			SetFocus(hwndStateField);
			
			//NormalizeDlgItemFont(hwndDlg,IDC_SUBTITLE);
			//NormalizeDlgItemFont(hwndDlg,IDT_TEXT1);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT2);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT3);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT4);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT5);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT6);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT7);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT8);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT9);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT10);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT12);
		
			SetWindowText(hwndDlg,pclRegWizard->GetWindowCaption());

			
			BOOL boughtByCompany = pclRegWizard->IsInformationWriteEnabled(kInfoCompany);
			int stringID1 = boughtByCompany ? IDS_ADDRDLG_TEXT1B : IDS_ADDRDLG_TEXT1A;
			int stringID2 = boughtByCompany ? IDS_ADDRDLG_TEXT2B : IDS_ADDRDLG_TEXT2A;
			LoadString(pi->hInstance,stringID1,szTemp,256);
			//HWND hWnd = GetDlgItem(hwndDlg,IDT_TEXT1);
			//SetWindowText(hWnd,szTemp);

			LoadString(pi->hInstance,stringID2,szTemp,256);
			hWnd = GetDlgItem(hwndDlg,IDT_TEXT2);
			SetWindowText(hWnd,szTemp);

			//new CStaticText(pclRegWizard->GetInstance(),hwndDlg,IDT_TEXT1,stringID1,NULL);
			//new CStaticText(pclRegWizard->GetInstance(),hwndDlg,IDT_TEXT2,stringID2,NULL);
			if (pclRegWizard->GetInformationString(kInfoMailingAddress,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_EDIT1,WM_SETTEXT,0,(LPARAM) szInfo);
			}
/*			if (pclRegWizard->GetInformationString(kInfoAdditionalAddress,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_EDIT2,WM_SETTEXT,0,(LPARAM) szInfo);
			}*/
			if (pclRegWizard->GetInformationString(kInfoCity,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_EDIT3,WM_SETTEXT,0,(LPARAM) szInfo);
			}
			if (pclRegWizard->GetInformationString(kInfoState,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_EDIT4,WM_SETTEXT,0,(LPARAM) szInfo);
			}
			if (pclRegWizard->GetInformationString(kInfoZip,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_EDIT5,WM_SETTEXT,0,(LPARAM) szInfo);
			}
			if (pclRegWizard->GetInformationString(kInfoPhoneNumber,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_EDIT6,WM_SETTEXT,0,(LPARAM) szInfo);
			}
		/*	if (pclRegWizard->GetInformationString(kInfoEmailName,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_EDIT7,WM_SETTEXT,0,(LPARAM) szInfo);
			}*/
			if (pclRegWizard->GetInformationString(kInfoPhoneExt,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_EDIT8,WM_SETTEXT,0,(LPARAM) szInfo);
			}
			if (pclRegWizard->GetInformationString(kAreaCode,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_AREACODE,WM_SETTEXT,0,(LPARAM) szInfo);
			}

		
			pclRegWizard->ResolveCurrentCountryCode();

			gTapiCountryTable.FillCountryList(pclRegWizard->GetInstance(), 
				GetDlgItem(hwndDlg,IDC_COMBO1)	);
			PTSTR psz = gTapiCountryTable.GetCountryName ( pclRegWizard->GetCountryCode());
			SendMessage(GetDlgItem(hwndDlg,IDC_COMBO1),
			CB_SELECTSTRING, (WPARAM) -1,(LPARAM) psz); //select this country
			if (pclRegWizard->GetInformationString(kInfoCountry,szInfo))
			{
				SendDlgItemMessage(hwndDlg,IDC_COMBO1,CB_SELECTSTRING,(WPARAM) -1,(LPARAM) szInfo);
			}
			ConfigureFEAddrEditFields(pclRegWizard,hwndDlg);

			#ifdef CREATE_TAB_ORDER_FILE
			CreateAddrDlgTabOrderString(hwndDlg);
			FResSetDialogTabOrder(hwndDlg,IDS_TAB_ADDRESS);
			#endif

			vDialogInitialized = TRUE;
            return TRUE;
		} // WM_INIT
		case WM_NOTIFY:
        {   LPNMHDR pnmh = (LPNMHDR)lParam;
            switch( pnmh->code ){
            case PSN_SETACTIVE:
                pi->iCancelledByUser = RWZ_PAGE_OK;

				{
						BOOL boughtByCompany = pclRegWizard->IsInformationWriteEnabled(kInfoCompany);
						int stringID1 = boughtByCompany ? IDS_ADDRDLG_TEXT1B : IDS_ADDRDLG_TEXT1A;
						int stringID2 = boughtByCompany ? IDS_ADDRDLG_TEXT2B : IDS_ADDRDLG_TEXT2A;
						LoadString(pi->hInstance,stringID1,szTemp,256);
						//HWND hWnd = GetDlgItem(hwndDlg,IDT_TEXT1);
					//	SetWindowText(hWnd,szTemp);
						LoadString(pi->hInstance,stringID2,szTemp,256);
						hWnd = GetDlgItem(hwndDlg,IDT_TEXT2);
						SetWindowText(hWnd,szTemp);
				}
						

				//if(spAddrSheet) {
				//	spAddrSheet->pszHeaderTitle = MAKEINTRESOURCE(IDS_WELCOME_SCR_TITLE);
				//}

				PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK | PSWIZB_NEXT );
                break;
            case PSN_WIZNEXT:
				iRet=0;
				if(pi->iCancelledByUser == RWZ_CANCELLED_BY_USER ) {
					pi->CurrentPage=pi->TotalPages-1;
					PropSheet_SetCurSel(GetParent(hwndDlg),NULL,pi->TotalPages-1);

				}else {
					if( ValidateFEAddrDialog(pclRegWizard,hwndDlg)) {
						ConfigureFEAddrEditFields(pclRegWizard,hwndDlg);
						SendDlgItemMessage(hwndDlg,IDC_EDIT1,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoMailingAddress,szInfo);
					/*	SendDlgItemMessage(hwndDlg,IDC_EDIT2,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoAdditionalAddress,szInfo);*/
						SendDlgItemMessage(hwndDlg,IDC_EDIT3,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoCity,szInfo);
						SendDlgItemMessage(hwndDlg,IDC_EDIT4,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoState,szInfo);
						SendDlgItemMessage(hwndDlg,IDC_EDIT5,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoZip,szInfo);
						SendDlgItemMessage(hwndDlg,IDC_EDIT6,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoPhoneNumber,szInfo);
					/*	SendDlgItemMessage(hwndDlg,IDC_EDIT7,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoEmailName,szInfo);*/
						SendDlgItemMessage(hwndDlg,IDC_EDIT8,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoPhoneExt,szInfo);
						
						// Get Area Code 
						SendDlgItemMessage(hwndDlg,IDC_AREACODE,WM_GETTEXT,255,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kAreaCode,szInfo);


						LRESULT selIndex = SendDlgItemMessage(hwndDlg,IDC_COMBO1,CB_GETCURSEL,0,0L);
						SendDlgItemMessage(hwndDlg,IDC_COMBO1,CB_GETLBTEXT,selIndex,(LPARAM) szInfo);
						pclRegWizard->SetInformationString(kInfoCountry,szInfo);
					
						SetRegWizardCountryCode(pclRegWizard,hwndDlg);
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
                pi->CurrentPage--;

                break;
			case PSN_QUERYCANCEL :
				iRet=0;
				if (CancelRegWizard(pclRegWizard->GetInstance(),hwndDlg)) {
					//pclRegWizard->EndRegWizardDialog(IDB_EXIT) ;
					iRet = 1;
					pi->ErrorPage  = kAddressDialog;
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
			if (HIWORD(wParam) == CBN_KILLFOCUS){
				ConfigureFEAddrEditFields(pclRegWizard,hwndDlg);
			}
		} // WM_COMMAND
		break;
	    default:
		bStatus = FALSE;
        break;
    }
    return bStatus;
}



BOOL ValidateFEAddrDialog(CRegWizard* pclRegWizard,HWND hwndDlg)
/*********************************************************************
Returns TRUE if all required user input is valid in the Address
dialog.  If any required edit field input is empty, ValidateAddrDialog
will put up a message box informing the user of the problem, and set
the focus to the offending control.
**********************************************************************/
{
	int iInvalidEditField = ValidateFEAddrEditFields(pclRegWizard,hwndDlg);
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
		HWND hwndInvField = GetDlgItem(hwndDlg,iInvalidEditField);
		SetFocus(hwndInvField);
		return FALSE;
	}
}


int ValidateFEAddrEditFields(CRegWizard* pclRegWizard,HWND hwndDlg)
/*********************************************************************
ValidateFEAddrEditFields validates all edit fields in the Address
dialog.  If any required field is empty, the ID of the first empty
edit field control will be returned as the function result.  If all 
fields are OK, NULL will be returned.
**********************************************************************/
{
	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT1)) return IDC_EDIT1; 
//	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT2)) return IDC_EDIT2; 
	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT3)) return IDC_EDIT3; 
	if(pclRegWizard->GetCountryCode() == 0)
	{
		if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT4)) 
			return IDC_EDIT4; 
	}
	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT5)) return IDC_EDIT5; 
	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT6)) return IDC_EDIT6;
//	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT7)) return IDC_EDIT7;
	if (!CRegWizard::IsEditTextFieldValid(hwndDlg,IDC_EDIT8)) return IDC_EDIT8;

	return NULL; 
}


void ConfigureFEAddrEditFields(CRegWizard* pclRegWizard,HWND hwndDlg)
/*********************************************************************
**********************************************************************/
{
	SetRegWizardCountryCode(pclRegWizard,hwndDlg);
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT1,kAddrSpecAddress,IDT_TEXT2);
//	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT2,kAddrSpecAddress2,IDT_TEXT4);
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT3,kAddrSpecCity,IDT_TEXT5);
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT4,kAddrSpecState,IDT_TEXT6);
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT5,kAddrSpecPostalCode,IDT_TEXT7);
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT6,kAddrSpecPhone,IDT_TEXT8);
//	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT7,kAddrSpecEmailName,IDT_TEXT12);
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_EDIT8,kAddrSpecExtension,IDT_TEXT9);
	pclRegWizard->ConfigureEditTextField(hwndDlg,IDC_AREACODE,kAddrSpecAreaCode,IDT_AREACODE);
}




#ifdef CREATE_TAB_ORDER_FILE
void CreateAddrDlgTabOrderString(HWND hwndDlg)
/*********************************************************************
Creates a comma delimited list of ID's for all controls belonging to
the given dialog, and writes the list to a text file.
**********************************************************************/
{
	HANDLE hFile = CreateFile(_T"c:\\ADDRTAB.TXT",GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		EnumChildWindows(hwndDlg,(WNDENUMPROC) EnumChildProc,(LPARAM) hFile);
		CloseHandle(hFile);
	}
}
#endif


#ifdef CREATE_TAB_ORDER_FILE
BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam)
/*********************************************************************
**********************************************************************/
{
	_TCHAR rgch[128];
	int iCtrlId = GetDlgCtrlID(hwndChild);
	LPTSTR sz = GetFocus() == hwndChild ? _T"F" : _T"";
	wsprintf(rgch,_T"%i%s,",iCtrlId,sz);

	HANDLE hFile = (HANDLE) lParam;
	DWORD dwBytesWritten;
	WriteFile(hFile,rgch,_tcslen(rgch) * sizeof(_TCHAR),&dwBytesWritten,NULL);
	return TRUE;
}
#endif

//
//  This function is calles during the creation and deletion of
//  Address Property Sheet 
//  Store the Address of PPROPSHEETPAGE so the Subtitle can be changed
//
//
//
UINT CALLBACK AddressFEPropSheetPageProc(HWND hwnd, 
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
