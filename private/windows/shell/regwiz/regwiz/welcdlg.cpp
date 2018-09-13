/*********************************************************************
Registration Wizard

WelcomeDialog.cpp
10/13/94 -  Tracy Ferrier
02/11/98 -  Suresh Krishnan
(c) 1994-95 Microsoft Corporation
8/20/98 - The "Use Windows Update .."  text is disabled for non OS products
**********************************************************************/

#include <Windows.h>
#include <RegPage.h>
#include <Resource.h>
#include "RegWizMain.h"
#include "Dialogs.h"
#include "regutil.h"
#include <stdio.h>
#include "version.h"
#include "rwwin95.h"
#include "rw_common.h"
#include "rwpost.h"

//
//
//  Returns 1 if Success
//          0 if failure
int GetOsName(HINSTANCE hIns, TCHAR *szOsName)
{
	HKEY hKey;
	TCHAR szOsPath[256];
	TCHAR szParamSubKey[64];
	int iRet;
	unsigned long infoSize;
	infoSize = 256; // Size of Buffer

	iRet = 0;
	LONG regStatus ;
	LoadString(hIns,IDS_REREGISTER_OS2,szOsPath,255);
	LoadString(hIns,IDS_INPUT_PRODUCTNAME,szParamSubKey,63);

	regStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szOsPath,0,KEY_READ,&hKey);
	if (regStatus == ERROR_SUCCESS) {
		
		regStatus = RegQueryValueEx(hKey ,szParamSubKey, NULL,0,(LPBYTE) szOsName,&infoSize);
		if (regStatus == ERROR_SUCCESS)
		{
			iRet = 1;
		}
		RegCloseKey(hKey);
	}

	return iRet;

	
}

INT_PTR CALLBACK WelcomeDialogProc(HWND hwndDlg, 
						UINT uMsg, 
						WPARAM wParam, LPARAM lParam)
/*********************************************************************
Main entry point for the Registration Wizard.
**********************************************************************/
{
	 INT_PTR bStatus = TRUE;
	 CRegWizard* pclRegWizard = NULL;
	_TCHAR szCallingContext[256];
	_TCHAR szText2[256];
	_TCHAR szTemp[128];
	_TCHAR szButtonText[48];
	_TCHAR szOsName[256];
	DWORD  dwConnectivity;
	 INT_PTR iRet;
	 int iCurPage;
	 static int iFirstTimeEntry=1; // This is to verify for Network connection
	LONG_PTR lStyle;

	PageInfo *pi = (PageInfo *)GetWindowLongPtr( hwndDlg, GWLP_USERDATA );
	if(pi) {
		pclRegWizard = pi->pclRegWizard;
	}

    switch (uMsg)
    {				
		case WM_DESTROY:
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, NULL );
        break;
        case WM_INITDIALOG:
		{
			pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
			pi->iCancelledByUser = RWZ_PAGE_OK;
			pclRegWizard = pi->pclRegWizard;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, (LONG_PTR)pi );
			SetControlFont( pi->hBigBoldFont, hwndDlg, IDT_TEXT1);
			pclRegWizard->GetInputParameterString(IDS_INPUT_PRODUCTNAME,szCallingContext);
			_tcscpy(szText2,szCallingContext);

			// appending ,so Microsoft can  
			//LoadString(pclRegWizard->GetInstance(),
			//IDS_WELCOME_SCR_TEXT22,szTemp,128);
			//_tcscat(szText2,szTemp);
					
								
			ReplaceDialogText(hwndDlg,IDT_TEXT1,szCallingContext);
			ReplaceDialogText(hwndDlg,IDT_TEXT2,szText2);
			// if the product being registered is not Windows NT OS  then do not 
			// display the Windows Updat site text
			szOsName[0] = _T('\0');
			GetOsName(pclRegWizard->GetInstance(),szOsName);

			
			HWND hParent = GetParent(hwndDlg);

			lStyle = GetWindowLongPtr( hParent, GWL_STYLE);
			lStyle &= ~WS_SYSMENU;
			SetWindowLongPtr(hParent,GWL_STYLE,lStyle);
			
			if(_tcscmp(szCallingContext,szOsName)){
				ShowWindow(GetDlgItem(hwndDlg,IDC_TEXT7),SW_HIDE);//SW_SHOW);
			}

            return TRUE;
		}
		case WM_NOTIFY:
        {   LPNMHDR pnmh = (LPNMHDR)lParam;
            switch( pnmh->code ){
            case PSN_SETACTIVE:
				pi->iCancelledByUser = RWZ_PAGE_OK;
				PropSheet_SetTitle(GetParent( hwndDlg ),0,pclRegWizard->GetWindowCaption());
                PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_NEXT);
				LoadString(pclRegWizard->GetInstance(),
						IDS_REGISTERLATER_BUTTONTEXT,
						szButtonText,16);

				// Limiting the Button Text to 16
				// As mo
				SetWindowText(GetDlgItem( GetParent( hwndDlg ),2),szButtonText); 
            break;
			case PSN_KILLACTIVE  :
				LoadString(pclRegWizard->GetInstance(),
						IDS_CANCEL_BUTTONTEXT,
						szButtonText,16);

				// Limiting the Button Text to 16
				// As mo
				SetWindowText(GetDlgItem( GetParent( hwndDlg ),2),szButtonText); 
				 
			break;
            case PSN_WIZNEXT:
			//  Check if it is cancelled bt the user
			//  if so then switch to the last Page
			//					
				if(pi->iCancelledByUser == RWZ_CANCELLED_BY_USER ) {
					pi->CurrentPage=pi->TotalPages-1;
					PropSheet_SetCurSel(GetParent(hwndDlg),NULL,pi->TotalPages-1);

				}else {
				//
				// User Has Not pressed Next 					
					if(iFirstTimeEntry) {
						iFirstTimeEntry = 0;
						pi->dwConnectionType = 0; // Init to Zero
						dwConnectivity =  (DWORD)CheckWithDisplayInternetConnectivityExists(pi->hInstance,hwndDlg);  
						//dwConnectivity = DIALUP_REQUIRED;
						switch(dwConnectivity) {
							case DIALUP_NOT_REQUIRED :
							case DIALUP_REQUIRED     :
								pi->dwConnectionType = dwConnectivity;
								break;
								//
								// The System is OK so proceed to the next screen 
							case RWZ_ERROR_NOTCPIP: // NO TCP_IP
							case CONNECTION_CANNOT_BE_ESTABLISHED: // NO Proper Modem or RAS
							default :
								// Set the NEXT so it goes to the lase Page
								pi->ErrorPage  = kWelcomeDialog;
								pi->iError     = dwConnectivity;
								pi->CurrentPage=pi->TotalPages-1;
								PropSheet_SetCurSel(GetParent(hwndDlg),NULL,pi->TotalPages-1);

								//pi->iCancelledByUser = RWZ_CANCELLED_BY_USER;
								//PropSheet_PressButton (GetParent( hwndDlg ),PSBTN_NEXT);
							break;
						}
												
					}
					pi->CurrentPage++;
				}
					
					
				
			break;
            case PSN_WIZBACK:
                pi->CurrentPage--;

                break;
			case PSN_QUERYCANCEL :
				iRet = 1;
				pi->ErrorPage  = kWelcomeDialog;
				pi->iError     = RWZ_ERROR_REGISTERLATER ;
				SetWindowLongPtr( hwndDlg,DWLP_MSGRESULT, (INT_PTR) iRet); 
				pi->iCancelledByUser = RWZ_CANCELLED_BY_USER;
				PropSheet_PressButton (GetParent( hwndDlg ),PSBTN_NEXT);
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
