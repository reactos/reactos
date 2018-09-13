#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "resource.h"
#include "rw_common.h"
#include "rwpost.h"
#include "sysinv.h"
#include "regutil.h"

#define POST_DATA_WITH_DISPLAY 1

static HWND  m_hDlg = NULL;
static HINSTANCE m_hInst=NULL;
static DWORD    dwRasError = 0; // To Store Ras reported Error
static HANDLE hRasNotifyEvt=  NULL ; // Event handle used for RAS Notififiation
static int siMsgType=0;  // used to choose the message to be displayed
static int siOperation=0;
extern BOOL bOemDllLoaded;
extern HANDLE hOemDll;
extern _TCHAR szWindowsCaption[256];

INT_PTR CALLBACK  DisplayDlgWindowWithOperation(
				HWND hDlg, 			//dialog window
				UINT uMsg,
				WPARAM wParam,
				LPARAM lParam
			)
			
//BOOL CALLBACK  DisplayMSNConnection(
	//	HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	
	DWORD	dwEnd = 0;
	BOOL	fRet = TRUE;
	HWND hwndParent ;
	switch(uMsg){
		case WM_INITDIALOG:	
			
			RECT parentRect,dlgRect;
			m_hDlg = hDlg;
			m_hInst   = ( HINSTANCE) lParam;
			hwndParent = GetParent(hDlg);
						
			if (hwndParent)
			{
				GetWindowRect(hwndParent,&parentRect);
				GetWindowRect(hDlg,&dlgRect);
				int newX = parentRect.left + (parentRect.right - parentRect.left)/2 - (dlgRect.right - dlgRect.left)/2;
				int newY = parentRect.top + (parentRect.bottom - parentRect.top)/2 - (dlgRect.bottom - dlgRect.top)/2;
				MoveWindow(hDlg,newX,newY,dlgRect.right - dlgRect.left,dlgRect.bottom - dlgRect.top,FALSE);
			}
			else
			{
				int horiz,vert;
				GetDisplayCharacteristics(&horiz,&vert,NULL);
				GetWindowRect(hDlg,&dlgRect);
				int newX = horiz/2 - (dlgRect.right - dlgRect.left)/2;
				int newY = vert/2 - (dlgRect.bottom - dlgRect.top)/2;
				MoveWindow(hDlg,newX,newY,dlgRect.right - dlgRect.left,dlgRect.bottom - dlgRect.top,FALSE);
			}
			NormalizeDlgItemFont(hDlg,IDC_TEXT1);
			
			SetWindowText(hDlg,szWindowsCaption);

			ReplaceDialogText(hDlg,IDC_TEXT1,GetProductBeingRegistred());

			if(siMsgType)
			{
				ReplaceDialogText(hDlg,IDC_TEXT1,GetProductBeingRegistred());
			}
			else
			{

				if(siOperation == POST_DATA_WITH_DISPLAY)
				{
					TCHAR szValueName[256] = _T("");
					HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hDlg,GWLP_HINSTANCE);
					LoadString(hInstance,IDS_POST_MESSAGE,szValueName,128);
					SendDlgItemMessage(hDlg,IDC_TEXT1,WM_SETTEXT,0,(LPARAM)szValueName);
				}
			}
			
			PostMessage( m_hDlg, WM_COMMAND, (WPARAM) IDOK,0 );
			return TRUE;
  			goto LReturn;
			break;
		case WM_COMMAND:
		switch (LOWORD(wParam)){
			case IDOK  :
				if(siOperation == POST_DATA_WITH_DISPLAY) {
					dwEnd = SendHTTPData(m_hDlg,m_hInst);
					goto LEnd;

				}
			
				dwEnd = CheckInternetConnectivityExists(m_hDlg,
					m_hInst);
				goto LEnd;

			case IDCANCEL:
				goto LEnd;
			default:
				fRet = FALSE;
				goto LReturn;
			}
		break;
		default :
		fRet = FALSE;
		goto LReturn;
		break;
	}
LEnd:
	EndDialog(hDlg,dwEnd);
	m_hDlg = NULL;	//no more dialog	
LReturn:	
	return fRet;
}



//  Function : CheckWithDisplayInternetConnectivityExists( )
//  This function calls the Background fucntion CheckInternetConnectivityExists() 		
//  whcih checks for the  connectivity cfg to the MSN.
//  The return value is based on the return value of CheckInternetConnectivityExists()
//	Returns
//	DIALUP_NOT_REQUIRED  : Use Network for tx
//  DIALUP_REQUIRED       : Use Dialupo for Tx
//  RWZ_ERROR_NOTCPIP      : No TCP/IPO
//  CONNECTION_CANNOT_BE_ESTABLISHED  : No modem or RAS setup


DWORD_PTR CheckWithDisplayInternetConnectivityExists(HINSTANCE hIns,HWND hwnd,int iMsgType)
{
	
	INT_PTR dwRet;	
	siMsgType = iMsgType;
	RW_DEBUG << "\n Before invoking Dlg   Display Internet Connection "  <<  flush;
	dwRet=DialogBoxParam(hIns, MAKEINTRESOURCE(IDD_VERIFY_CONNECTION), hwnd,DisplayDlgWindowWithOperation,
			 (LPARAM)hIns);
	siMsgType = 0;
	if(dwRet == -1 ) {
			 // Error in creating the Dialogue
	
	}
	RW_DEBUG << "\n In Chk With Display Internet Connection "  <<  flush;
	return dwRet;
}


DWORD_PTR PostDataWithWindowMessage( HINSTANCE hIns)
{
	INT_PTR dwRet;	
	siMsgType = 0;
	siOperation = POST_DATA_WITH_DISPLAY;
	RW_DEBUG << "\n Invoking PostDataDlg   Display Internet Connection "  <<  flush;
	dwRet=DialogBoxParam(hIns, MAKEINTRESOURCE(IDD_VERIFY_CONNECTION), NULL,DisplayDlgWindowWithOperation,
			 (LPARAM)hIns);
	siMsgType = 0;
	siOperation = 0;
	if(dwRet == -1 ) {
			 // Error in creating the Dialogue
	
	}
	return dwRet;

}
