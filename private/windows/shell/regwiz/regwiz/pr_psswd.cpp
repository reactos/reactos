/*

	File : PR_PSSWD.CPP
	
	Module for Displaying and getting the  Username and password for
	Proxy Server

*/


#include <Windows.h>
#include <tchar.h>
#include "RegWizMain.h"
#include "Resource.h"
#include "Dialogs.h"
#include "regutil.h"
#include "sysinv.h"
#include "rw_common.h"

#define MAX_USR_NAME_SZ   48
#define MAX_PSSWD_SZ      48
#define MAX_PRXOY_SRV_SZ  48

typedef struct _TmpProxyInfo{

	TCHAR  m_User[MAX_USR_NAME_SZ];
	TCHAR  m_Passwd[MAX_PSSWD_SZ];
	TCHAR  m_PrxySrv[MAX_PRXOY_SRV_SZ];
	HINSTANCE m_hInst;
}__TMPXX;

static _TmpProxyInfo  sPrInf;

INT_PTR CALLBACK  DisplayProxyAuthentication(
				HWND hDlg, 			//dialog window
				UINT uMsg,
				WPARAM wParam,
				LPARAM lParam
			)
			

{
	TCHAR szInfo[256];
	DWORD	dwEnd = 0;
	BOOL	fRet = TRUE;
	HWND hwndParent ;
	switch(uMsg){
		case WM_INITDIALOG:	
			
			RECT parentRect,dlgRect;
		
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
			
			//SetWindowText(hDlg,szWindowsCaption);

		
			if(sPrInf.m_User)
				SendDlgItemMessage(hDlg,IDC_USERNAME,WM_SETTEXT,0,(LPARAM) sPrInf.m_User);

			if(sPrInf.m_Passwd)
				SendDlgItemMessage(hDlg,IDC_PASSWORD,WM_SETTEXT,0,(LPARAM) sPrInf.m_Passwd);
			ReplaceDialogText(hDlg,IDC_TEXT1,sPrInf.m_PrxySrv);
			SendDlgItemMessage(hDlg,IDC_USERNAME,EM_SETSEL,0,-1);
			return TRUE;
  			goto LReturn;
			break;
		case WM_COMMAND:
		switch (LOWORD(wParam)){
			case IDOK  :
				SendDlgItemMessage(hDlg,IDC_USERNAME,WM_GETTEXT,255,
					(LPARAM) szInfo);
				_tcscpy(sPrInf.m_User,szInfo);

				SendDlgItemMessage(hDlg,IDC_PASSWORD,WM_GETTEXT,255,
					(LPARAM) szInfo);
				_tcscpy(sPrInf.m_Passwd,szInfo);
				goto LEnd;

			case IDCANCEL:
				dwEnd = 1;
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
LReturn:	
	return fRet;
}








//
//  returns 1 if Cancel is pressed
//

DWORD_PTR GetProxyAuthenticationInfo(HINSTANCE hIns,TCHAR *czProxy,
								 TCHAR *czUserName,TCHAR *czPswd)
{
	DWORD_PTR dwRet;	

	RW_DEBUG << "\n Invoking PROXY DIALOG "  <<  flush;

	sPrInf.m_hInst = hIns;
	if(czProxy) {
		_tcscpy(sPrInf.m_PrxySrv,czProxy);
	}
	else {
		//
		// Return from mfunction without displaying the Dld
		return -1;

	}
	if(czUserName) {
		_tcscpy(sPrInf.m_User,czUserName);
	}else {
		sPrInf.m_User[0] = _T('\0');
	}
	if(czPswd) {
		_tcscpy(sPrInf.m_Passwd,czPswd);
	}else {
		sPrInf.m_Passwd[0] = _T('\0');
	}

	dwRet=DialogBoxParam(hIns, MAKEINTRESOURCE(IDD_PROXY_LOG), NULL,DisplayProxyAuthentication,
			 (LPARAM)hIns);

	if(dwRet == -1 ) {
			 // Error in creating the Dialogue
		return 0;
	
	}

	//
	// Get from the Dialog
	if(sPrInf.m_User) {
		_tcscpy(czUserName, sPrInf.m_User);
	}else {
		czUserName[0] = _T('\0');
	}
	if(sPrInf.m_Passwd) {
		_tcscpy(czPswd,sPrInf.m_Passwd);
	}else {
		czPswd[0] = _T('\0');
	}
	RW_DEBUG << "\nAfter PROXY DIALOG \tUser[" << czUserName  << "]" << flush;
    //RW_DEBUG << "\n\t\tPassword[" << czPswd <<"]" << flush;
	return dwRet;

}
