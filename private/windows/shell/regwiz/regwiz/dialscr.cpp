/*
	File : DialScr.cpp
	Dialup Screen for RegWiz using Wizard 97 control

	02/13/98  Suresh Krishnan
	Modifications :
	Date 5/27/98 : Suresh 

	DisplayPhoneNumber() was displaying  junk if countries do not have area code 
	This is because the TAPI  call was failing because of the format represented in canonical form .
	Presently if are code is NULL then,  araecode is ignored in canonical form.
	


*/

#include <Windows.h>
#include "RegPage.h"
#include <stdio.h>
#include "ATK_RAS.h"

#include <tchar.h>
#include "resource.h"
#include "RegWizMain.h"
#include "dialogs.h"
#include <sudefs.h>
#include "rw_common.h"
#include "rwpost.h"
#include "regutil.h"
#include "mcm.h"

extern BOOL bPostSuccessful;

static  RASENTRY	 theRasEntry;
static  TCHAR szIspUserName[256];   // Temporary ISP Acoount Name
static  TCHAR szIspUserPassword[256]; // temp ISP Account Password

#define  MSN_SITE_DUN  _T("REGWIZ")

void  DialingProperties (HINSTANCE hIns, HWND hParent); //

DWORD ConfigureDUN ( HWND hWnd, HINSTANCE hInstance,
					 RASENTRY	*pRasEntry,
					 TCHAR		*szUserName,
					 TCHAR		*szPassword,
					 int    iModemIndex);

//
//  Global Variables
static HINSTANCE	m_hInstance;
static HWND			m_hWnd;
static HANDLE		m_hThread;
static HANDLE		hRasNotifyEvt;
static HANDLE		hRasKillEvt;
static HRASCONN		hRasConn;
static int siExitThread = 0;   // Set if the USer wants to Terminate
static DWORD    dwRasError = 0; // To Store Ras reported Error
static  int siPreviousRasState = RASCS_OpenPort;
static  int siCurrentRasState  = RASCS_OpenPort;

static DWORD DialThread(PVOID pData);  // the thread fun used for RAS connection
static void  RasDialFunc( UINT unMsg, RASCONNSTATE rasconnstate, DWORD dwError );
class  DialupHelperClass {
public :
	DialupHelperClass(HINSTANCE hIns, HWND hWnd);
	~DialupHelperClass();
	BOOL InvokeDialupSettings();
	CheckForDialingProperties();
	DisplayPhoneNumber();

	DWORD	CreateRasDialThread();
	BOOL	WaitForRasThread( HANDLE	hThread, BOOL fTimeOut);
	void	InitForStaticFunction( HINSTANCE hIns, HWND hWnd);
	void	DestroyRasThread(BOOL bRetry);

};


//
//  FUNCTION: GetRasConnState( RASCONNSTATE )
//
//  PURPOSE: get the index to the corresponding string
//
//  PARAMETERS:
//    rasconn - ras connection state
//
//  RETURN VALUE:
//    index into stringtable.
//
//  COMMENTS:
//
UINT GetRasConnState( RASCONNSTATE rasconn )
{
	

    switch( rasconn )
    {
        case RASCS_OpenPort:
            return IDS_OPENPORT;
        case RASCS_PortOpened:
            return IDS_PORTOPENED;
        case RASCS_ConnectDevice:
            return IDS_CONNECTDEVICE;
        case RASCS_DeviceConnected:
            return IDS_DEVICECONNECTED;
        case RASCS_AllDevicesConnected:
            return IDS_ALLDEVICESCONNECTED;
        case RASCS_Authenticate:
            return IDS_AUTHENTICATE;
        case RASCS_AuthNotify:
            return IDS_AUTHNOTIFY;
        case RASCS_AuthRetry:
            return IDS_AUTHRETRY;
        case RASCS_AuthCallback:
            return IDS_AUTHCALLBACK;
        case RASCS_AuthChangePassword:
            return IDS_AUTHCHANGEPASSWORD;
        case RASCS_AuthProject:
            return IDS_AUTHPROJECT;
        case RASCS_AuthLinkSpeed:
            return IDS_AUTHLINKSPEED;
        case RASCS_AuthAck:
            return IDS_AUTHACK;
        case RASCS_ReAuthenticate:
            return IDS_REAUTHENTICATE;
        case RASCS_Authenticated:
            return IDS_AUTHENTICATED;
        case RASCS_PrepareForCallback:
            return IDS_PREPAREFORCALLBACK;
        case RASCS_WaitForModemReset:
            return IDS_WAITFORMODEMRESET;
        case RASCS_WaitForCallback:
            return IDS_WAITFORCALLBACK;
        case RASCS_Interactive:
            return IDS_INTERACTIVE;
        case RASCS_RetryAuthentication:
            return IDS_RETRYAUTHENTICATION;
        case RASCS_CallbackSetByCaller:
            return IDS_CALLBACKSETBYCALLER;
        case RASCS_PasswordExpired:
            return IDS_PASSWORDEXPIRED;
        case RASCS_Connected:
            return IDS_CONNECTED;
        case RASCS_Disconnected:
            return IDS_DISCONNECTED;
        default:
            return IDS_RAS_UNDEFINED_ERROR;
    }
}


//
//  This function Enable or Disables the Controls of Wizard 97 control
//  Cancel, Back, Next buttons
//  the hDlg passed is the child of the Wizard control. So we use
//  GetParent to get the handle of Wizard control
//
BOOL RW_EnableWizControl(
					HWND hDlg,
					int	 idControl,
					BOOL fEnable
					)
{
	if (hDlg ==NULL ){
		return FALSE;
	}

	HWND hWnd = GetDlgItem(GetParent( hDlg),idControl);
	if (hWnd){
		EnableWindow(hWnd,fEnable);
	}
	return TRUE;

}



BOOL FEnableControl(
					HWND hDlg,
					int	 idControl,
					BOOL fEnable
					)
{
	if (NULL == hDlg)
	{
		//AssertSz(0,"Null Param");
		return FALSE;
	}

	HWND hWnd = GetDlgItem(hDlg,idControl);
	if (hWnd)
	{
		EnableWindow(hWnd,fEnable);
	}
	return TRUE;
}





DialupHelperClass :: DialupHelperClass( HINSTANCE hIns, HWND hWnd)
{
	m_hInstance = hIns;
	m_hWnd = hWnd;
	hRasNotifyEvt = NULL;
	hRasKillEvt = NULL;
	m_hThread = NULL;
	hRasConn = NULL;

}

DialupHelperClass :: ~DialupHelperClass()
{

}

//
// This function invokes Telephoney Settings of the control Panel
//

BOOL DialupHelperClass :: InvokeDialupSettings()
{	
/*********************************************************************
This function puts up the "Telephon properties " control panel, and
returns only when the user has dismissed the dialog (either after
installing a new modem, or canceling).
Returns: FALSE if an error prevented the dialog from being displayed.
**********************************************************************/

	_TCHAR 				szCmdLine[128];
	STARTUPINFO 		si;
	PROCESS_INFORMATION pi;
	BOOL 				fControlProcessDone = FALSE;
	BOOL 				fProcessStatus;
	//HWND				hwndProcess;
	

	LoadString(m_hInstance ,IDS_DIALINGPROPERTIES,szCmdLine,128);

	si.cb = sizeof(STARTUPINFO);
	si.lpReserved = NULL;
	si.lpDesktop = NULL;
	si.lpTitle = NULL;
	si.dwFlags = 0L;
	si.cbReserved2 = 0;
	si.lpReserved2 = NULL;

	fProcessStatus = CreateProcess(NULL,szCmdLine,NULL,NULL,FALSE,
		CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_PROCESS_GROUP | NORMAL_PRIORITY_CLASS,NULL,NULL,&si,&pi);
	if (fProcessStatus == FALSE)
	{
		return FALSE;
	}
	else
	{
		CloseHandle(pi.hThread);

		DWORD dwRet;
		dwRet = WaitForSingleObject(pi.hProcess, INFINITE);
		switch(dwRet) {
		case WAIT_ABANDONED :
			break;
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			break;
		case WAIT_FAILED:
			DWORD dwLastError;
			dwLastError = GetLastError();
			break;
		default :
			break;
		}
	
	}
	CloseHandle(pi.hProcess);
	return TRUE;
}

//Cancel..
void DialupHelperClass :: DestroyRasThread(BOOL bRetry)
{
	
	if(!bRetry)
	{
		siExitThread  = 1;
		if(hRasNotifyEvt)
		{
			SetEvent (hRasNotifyEvt);
		}
	
		if(m_hThread)
		WaitForSingleObject(m_hThread, INFINITE);
	}

	if( hRasConn != NULL )
	{
		int i =0;
		DWORD dwConnectStatus = 0;
		DWORD dwHangupRet =0;
		RASCONNSTATUS rasConn;
		rasConn.dwSize = sizeof(RASCONNSTATUS);
		try
		{
		
			RW_DEBUG << "Hanging up the connection" << endl;

			dwHangupRet = ATK_RasHangUp( hRasConn );
			if(!dwHangupRet)
			{
				do
				{
					RW_DEBUG << "chek connection status" << endl;
					dwConnectStatus = ATK_RasGetConnectionStatus(hRasConn,&rasConn) ;
					Sleep(100);
					i++;
				}while ((dwConnectStatus != ERROR_INVALID_HANDLE ) || (i < 200));
			}
			else
			{
				RW_DEBUG << "Hangup result: " << dwHangupRet << endl;
			}

			RW_DEBUG << "connection shot dead" << endl;
			hRasConn = NULL;
		}
		catch(...)
		{
			RW_DEBUG << "Error Caught dwHangupRet:" << dwHangupRet << endl;
			//RW_DEBUG << "hRasConn:" << hRasConn << endl;
		}
	}

	if(!bRetry)
	{
		siExitThread  = 0;
		m_hThread = NULL;
	}

	RW_DEBUG << "Exiting DestroyRasThread" << endl;
	return ;	
}

BOOL DialupHelperClass ::  CheckForDialingProperties()
{
	HKEY    hKey;
	TCHAR   szTel[256] = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations");
	TCHAR   szCI [48] = _T("CurrentID");
	_TCHAR  czLastStr[48];
	_TCHAR  czNewKey[256];
	DWORD   dwCurrentId;

	LONG	lStatus;
	DWORD   dwInfoSize = 48;
	BOOL    bRetValue;
	bRetValue = TRUE;

	LoadString(m_hInstance,IDS_TELEPHONE_LOC,szTel,256);
	LoadString(m_hInstance,IDS_TELEPHONE_CID,szCI,48);

	lStatus= RegOpenKeyEx(HKEY_LOCAL_MACHINE,szTel,0,KEY_READ,&hKey);
	if (lStatus == ERROR_SUCCESS)
	{
		//  Get Index
		//
		dwInfoSize = sizeof(dwCurrentId);
		lStatus = RegQueryValueEx(hKey,szCI,NULL,0,(  LPBYTE )&dwCurrentId,&dwInfoSize);
		if( lStatus !=  ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			bRetValue = FALSE;
		}
		RegCloseKey(hKey);
	}

	//
	// Now Contine to scan
	//for (int iCount =0; iCount < dwNumEntries; iCount ++ )
		
	_stprintf(czLastStr,_T("\\Location%d"),dwCurrentId);
	_tcscpy(czNewKey,szTel);
	_tcscat(czNewKey,czLastStr);

	lStatus= RegOpenKeyEx(HKEY_LOCAL_MACHINE,czNewKey,0,KEY_READ,&hKey);

	if (lStatus == ERROR_SUCCESS)
	{
			bRetValue = TRUE;
	}
	
	if(!bRetValue)
	{
		_TCHAR szMessage[256];
		LoadString(m_hInstance,IDS_DIALING_MESSAGE,szMessage,256);
		RegWizardMessageEx(m_hInstance,m_hWnd,IDD_INVALID_DLG,szMessage);
		return InvokeDialupSettings();
	}
	return bRetValue;
}

BOOL DialupHelperClass :: DisplayPhoneNumber(void)
{
	HLINEAPP hLineApp;
	_TCHAR szAddressIn[256];
	HWND hCtl;

	DWORD dwAPI,dwDevice,dwAPIHighVersion = 0x30000;
	
    LPLINETRANSLATEOUTPUT lpTranslateOutput;
	
	lpTranslateOutput = (LPLINETRANSLATEOUTPUT)LocalAlloc (LPTR, sizeof(LINETRANSLATEOUTPUT));

    lpTranslateOutput->dwTotalSize = sizeof(LINETRANSLATEOUTPUT);

    if(FGetDeviceID(m_hInstance, &hLineApp, &dwAPI, &dwDevice,0))
	{
		_TCHAR szTemp[256];
	#ifdef _LOG_IN_FILE
		 RW_DEBUG  << "\n After FGetDeviceID" << flush;
	#endif
	
		// Put the number in the canonical form -> +1 (201) 2220577
		
		_itot(theRasEntry.dwCountryCode,szTemp,10);
		_tcscpy(szAddressIn,_T("+"));
		_tcscat(szAddressIn,szTemp);
		if(theRasEntry.szAreaCode[0] == 0 ) {
			_tcscat(szAddressIn,_T(" "));;
		}else {
			_tcscat(szAddressIn,_T(" ("));
			_tcscat(szAddressIn,theRasEntry.szAreaCode);
			_tcscat(szAddressIn,_T(") "));
		}
		
		_tcscat(szAddressIn,theRasEntry.szLocalPhoneNumber);

	#ifdef _LOG_IN_FILE
		 RW_DEBUG  << "\n Device:" <<dwDevice << "Phone number:" <<ConvertToANSIString(szAddressIn)<< flush;
	#endif
	
		long lRet = lineTranslateAddress(hLineApp,dwDevice,dwAPIHighVersion,(LPCTSTR)szAddressIn,0,0,
										lpTranslateOutput);
	
		if(lRet == 0)
		{
			size_t sizeNeeded;
			sizeNeeded = lpTranslateOutput->dwNeededSize;
			
			LocalFree(lpTranslateOutput);


			// Make sure the buffer exists, is valid and big enough.
		    lpTranslateOutput = (LPLINETRANSLATEOUTPUT)LocalAlloc (LPTR,sizeNeeded);
			
			lpTranslateOutput->dwTotalSize = sizeNeeded;

			if (lpTranslateOutput == NULL)
		        return FALSE;


			lRet = lineTranslateAddress(hLineApp,dwDevice,dwAPIHighVersion,(LPCTSTR)szAddressIn,0,0,
										lpTranslateOutput);
			if(lRet == 0)
			{
				_TCHAR szTemp[256] ;
				#ifdef _LOG_IN_FILE
					 RW_DEBUG  << "\n lineTranslateAddress  returns true:" << flush;
				#endif
				
				RW_DEBUG  << "\n dwTotalSize:" << lpTranslateOutput->dwTotalSize<< endl;
				RW_DEBUG  << "\n dwNeededSize:" << lpTranslateOutput->dwNeededSize  << endl;
				RW_DEBUG  << "\n dwUsedSize:" << lpTranslateOutput->dwUsedSize << endl;
				RW_DEBUG  << "\n dwDisplayableStringSize:" << lpTranslateOutput->dwDisplayableStringSize << endl;
				RW_DEBUG  << "\n dwDisplayableStringOffset:" << lpTranslateOutput->dwDisplayableStringOffset << endl;
				RW_DEBUG  << "\n dwDialableStringSize:" << lpTranslateOutput->dwDialableStringSize << endl;
				RW_DEBUG  << "\n dwDialableStringOffset:" << lpTranslateOutput->dwDialableStringOffset << endl;
				RW_DEBUG  << "\n dwDestCountry:" << lpTranslateOutput->dwDestCountry << endl;
				RW_DEBUG  << "\n dwCurrentCountry:" << lpTranslateOutput->dwCurrentCountry << endl;
			
				hCtl = GetDlgItem(m_hWnd,IDC_PHONENUMBER);
				if (hCtl)
				{
					#ifdef _LOG_IN_FILE
					// RW_DEBUG  << "\n Full Phone number:" <<ConvertToANSIString(szTemp)<< flush;
					#endif

					SetWindowText(hCtl,(LPCTSTR)(((LPCSTR)lpTranslateOutput) + lpTranslateOutput->dwDisplayableStringOffset ));

					LocalFree(lpTranslateOutput);

					return TRUE;
				}
				else
				{
					LocalFree(lpTranslateOutput);
				}
			}
		}
		else
		{
			
			#ifdef _LOG_IN_FILE
				 RW_DEBUG  << "\n*Error in  lineTranslateAddress  returned:" << lRet << flush;
			#endif
   		    hCtl = GetDlgItem(m_hWnd,IDC_PHONENUMBER);
			SetWindowText(hCtl,szAddressIn);

		}
	}
	else
	{
		#ifdef _LOG_IN_FILE
			 RW_DEBUG  << "\n FGetDeviceID failed" << flush;
		#endif

	}
	return FALSE;
}




DWORD  DialupHelperClass :: CreateRasDialThread()
{
	DWORD	dwTID;
	DWORD	dwEnd;
		 					
	if (m_hThread)	//if we are already doing this..
	{
		if (STILL_ACTIVE == GetExitCodeThread(m_hThread,&dwEnd)) //pass back the exit code from the thread.
		{
			//AssertSz(0,"Already have a thread dialing..");
			WaitForRasThread(m_hThread,FALSE);			//wait for thread as long as it takes.
		}	
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	hRasNotifyEvt = CreateEvent(NULL, FALSE,FALSE,NULL);
	hRasKillEvt   = CreateEvent(NULL, FALSE,FALSE,NULL);

	if(hRasNotifyEvt == NULL ) {
		return DIALFAILED;
	}
    //launch a thread to do dialing.
	m_hThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)
		DialThread ,NULL,0,&dwTID);						
	if (NULL == m_hThread)
	{
		
		return DIALFAILED;
	}
		
	return DIALSUCCESS;
	
}



DWORD  DialThread(PVOID pData)
{
    RASDIALPARAMS rdParams;
    DWORD dwRet;
    _TCHAR  szBuf[256];
	int iExit;
	DWORD dwPostRet;
	int iTimeOut;

    // setup RAS Dial Parameters
    rdParams.dwSize = sizeof(RASDIALPARAMS);
    lstrcpy(rdParams.szEntryName,MSN_SITE_DUN);
    rdParams.szPhoneNumber[0] = '\0';
	
	GetDlgItemText(m_hWnd,IDC_PHONENUMBER,rdParams.szPhoneNumber,RAS_MaxPhoneNumber+1);

    rdParams.szCallbackNumber[0] = '*';
    rdParams.szCallbackNumber[1] = '\0';

    //rdParams.szUserName[0] = '\0';
    //rdParams.szPassword[0] = '\0';
	_tcscpy(szIspUserName,_T("RegWizNT30@gn.microsoft.com"));
	_tcscpy(szIspUserPassword,_T("RegSupNT"));

	_tcscpy(rdParams.szUserName,szIspUserName);
	_tcscpy(rdParams.szPassword,szIspUserPassword);
    rdParams.szDomain[0] = '*';
    rdParams.szDomain[1] = '\0';
    hRasConn = NULL;
	#ifdef _LOG_IN_FILE
		 RW_DEBUG  << "\n Before RAS Dial " << flush;
	     RW_DEBUG  << "\n UserName:" << ConvertToANSIString(szIspUserName) << flush;
		 RW_DEBUG  << "\n UserPassword:" << ConvertToANSIString(szIspUserPassword) << flush;
	#endif


    dwRet = ATK_RasDial( NULL, NULL, &rdParams, 0L, (RASDIALFUNC) RasDialFunc, &hRasConn);
		#ifdef _LOG_IN_FILE
			 RW_DEBUG  << "\n After  RAS Dial " << flush;
			
		#endif

    if ( dwRet ){
        if ( ATK_RasGetErrorString( (UINT)dwRet, (LPTSTR)szBuf, 256 ) != 0 )
            wsprintf( (LPTSTR)szBuf, _T("Undefined RAS Dial Error (%ld)."), dwRet );
		LoadString(m_hInstance, IDS_MODEM_ALREADY_INUSE, szBuf, 64 );
		#ifdef _LOG_IN_FILE
			RW_DEBUG  << "\n" << "Undefined Error"  << flush;
		#endif
		SetDlgItemText( m_hWnd, ID_LABELINIT, (LPCTSTR) szBuf );
		PostMessage( m_hWnd, WM_COMMAND,
						(WPARAM) IDEND, RWZ_ERROR_MODEM_IN_USE );
		
        return TRUE;
    }
	iExit = 0;
	iTimeOut = 0;
	do
	{
		dwRet = WaitForSingleObject(hRasNotifyEvt,100);
		switch(dwRet)
		{
		case WAIT_ABANDONED :
			iExit = 1;
			break;
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			break;
		default :
			break;
		}
		//
		//  Check if it is necessary to  kill this thread operation
		//
		if( siExitThread  )
		{
			iExit = 1;
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n RAS Thread : User request to Kill The RAS Thread " << dwRasError << flush;
			#endif
		}
		else
		{
			// Check if there is any RAS Error
			if(  dwRasError )
			{
				#ifdef _LOG_IN_FILE
					RW_DEBUG << "\n RAS Thread : Error  " << dwRasError << flush;
				#endif
				iExit = 1;
				PostMessage( m_hWnd, WM_COMMAND,
				(WPARAM) IDD_DIALUP_ERROR, dwRasError );
				dwRasError = 0;

			}
			else
			{
			//
			// Check if RAS Connection is established
				if(siCurrentRasState   == RASCS_Connected )
				{

					#ifdef _LOG_IN_FILE
						RW_DEBUG << "\n RAS Thread : Connected To MSN Site \n Txmit Data  " << flush;
						
					#endif
					iExit =1;
					dwPostRet = SendHTTPData(m_hWnd, m_hInstance);
					#ifdef _LOG_IN_FILE
						RW_DEBUG << "\n RAS Thread : After Posting Data  " << dwPostRet  << flush;
						
					#endif
					PostMessage( m_hWnd, WM_COMMAND,
						(WPARAM) IDEND, dwPostRet );

					//
					// Send Data by HTP Post
					//
				}
			}
		}

	}while(!iExit);
	//
	//  Action  to be done while exiting the Thread
	//

	if(hRasNotifyEvt)
	{
		CloseHandle(hRasNotifyEvt); // CLose The Event Object
	}

	#ifdef _LOG_IN_FILE
		 RW_DEBUG  << "\n Before  Exiting RAS Dial Thread " << flush;
	#endif

    return TRUE;
}

VOID  RasDialFunc( UINT unMsg, RASCONNSTATE rasconnstate, DWORD dwError )
{

    _TCHAR szMessage[256];
	
	DWORD dwRet = WaitForSingleObject(hRasKillEvt,3);
	// We Were killed Hangup
	if(dwRet ==  WAIT_OBJECT_0)
	{
		if(hRasConn != NULL)
		{
			ATK_RasHangUp( hRasConn );
			RW_DEBUG << "Hanging up in RasDialFunc" << endl;
		}
		else
		{
			RW_DEBUG << "Tried Hanging up in RasDialFunc but hRasConn is NULL" << endl;
		}

		if(hRasKillEvt)
		{
			CloseHandle(hRasKillEvt); // CLose The Event Object
		}
		return;
	}

    LoadString(m_hInstance, GetRasConnState( (RASCONNSTATE) rasconnstate), szMessage, 64 );
	//SetWindowText(m_hDlg,(LPCTSTR) szMessage);
	RW_DEBUG  << "\n" << ConvertToANSIString(szMessage) << flush;
    SetDlgItemText( m_hWnd, ID_LABELINIT, (LPCTSTR) szMessage );
	
    if (dwError)  // error occurred


    {
        if ( ATK_RasGetErrorString( (UINT)dwError, szMessage, 256 ) != 0 )
            wsprintf( (LPTSTR)szMessage, _T("Undefined RAS Dial Error.") );

        RW_DEBUG  << "\n Exiting with Error " << ConvertToANSIString(szMessage);
		
		if( dwError == ERROR_USER_DISCONNECTION )
			return;
		dwRasError  = dwError; // Set The Error


        //PostMessage( m_hDlg, WM_COMMAND, (WPARAM) IDD_DIALUP_ERROR, dwError );

    }
    else if ( RASCS_DONE & rasconnstate)
    {
		RW_DEBUG  << "\n" << " RACS_DONE .... " ;
		
        //EndDialog(m_hDlg, TRUE);          // Exit the dialog
    }
    siPreviousRasState = siCurrentRasState ;
	siCurrentRasState  = rasconnstate;
	if( hRasNotifyEvt) {
		// Set The Event So the RAS Processing Thread can wake up
		SetEvent(hRasNotifyEvt);
	}
	

    return ;

}


BOOL DialupHelperClass :: WaitForRasThread(
					HANDLE	hThread,
					BOOL fTimeOut
					)
{
	BOOL 	fRet = TRUE;

	if (hThread){
		DWORD dwRet=WAIT_TIMEOUT;
		if (WAIT_TIMEOUT == dwRet){
			TerminateThread(hThread,0);
			fRet = FALSE;
		}
	}
	return fRet;
}





INT_PTR CALLBACK DialupScreenProc(HWND hwndDlg,
					  UINT uMsg, WPARAM wParam, LPARAM lParam)
/*********************************************************************
Main entry point for the Registration Wizard.
**********************************************************************/
{
	CRegWizard*    pclRegWizard = NULL;
	DialupHelperClass  *pDH;
	int    iMsgId;
	static int iRetry =0;
	static BOOL bIsPhoneBookCreated = TRUE;
	static int    iModemIndex=1; // Index of the Modem
	INT_PTR iRet;
	_TCHAR szInfo[256];
	_TCHAR szMessage[256];	

	
	BOOL bStatus = TRUE;
	PageInfo *pi = (PageInfo *)GetWindowLongPtr( hwndDlg, GWLP_USERDATA );
	if(pi) {
		pclRegWizard = pi->pclRegWizard;
		pDH          = pi->pDialupHelper;
	};

    switch (uMsg)
    {
				
		case WM_DESTROY:
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, NULL );
			break;				
        case WM_INITDIALOG:
		{
			_TCHAR szInfo[256];
			pi = (PageInfo *)((LPPROPSHEETPAGE)lParam)->lParam;
			pclRegWizard = pi->pclRegWizard;
			
			if(pi->pDialupHelper ==  NULL ) {
					pi->pDialupHelper = new DialupHelperClass(pi->hInstance,
										hwndDlg);
			}
			pDH   = pi->pDialupHelper;



			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, (LONG_PTR)pi );
			SetControlFont( pi->hBigBoldFont, hwndDlg, IDT_TEXT1);

			//UpgradeDlg(hwndDlg);
			
			NormalizeDlgItemFont(hwndDlg,IDC_TITLE,RWZ_MAKE_BOLD);
			NormalizeDlgItemFont(hwndDlg,IDC_SUBTITLE);
			NormalizeDlgItemFont(hwndDlg,IDT_TEXT1);
			
			pclRegWizard->GetInputParameterString(IDS_INPUT_PRODUCTNAME,szInfo);
			ReplaceDialogText(hwndDlg,ID_LABELCALLONE,szInfo);
			SetDlgItemText( hwndDlg, IDC_PHONENUMBER, (LPCTSTR) "1 800 795 5675");
			SetWindowText(hwndDlg,pclRegWizard->GetWindowCaption());
			FEnableControl(hwndDlg,IDC_DISCONNECT,FALSE);
			return TRUE;
		}// WM_INIT
		break;
		case WM_NOTIFY:
        {   LPNMHDR pnmh = (LPNMHDR)lParam;
            switch( pnmh->code ){
            case PSN_SETACTIVE:
				

				pi->ErrorPage  = kDialupDialog;
				pi->iError     = RWZ_NOERROR;
				
				bPostSuccessful = TRUE;
				//
				// Check if RAS is installed
				if( ATK_IsRasDllOk() != RAS_DLL_LOADED ) {
				//
				// Error as no RAS DLL
					pi->iError     = RWZ_ERROR_RASDLL_NOTFOUND;
					bPostSuccessful = FALSE;
				
				}else
				{
				// Check if Telephoney is configured
					pDH->CheckForDialingProperties( );
					if(!ConfigureDUN(hwndDlg,
						pi->hInstance,
						&theRasEntry,
						szIspUserName,szIspUserPassword,
						iModemIndex) )
					{
						pi->iError  = RWZ_ERROR_LOCATING_DUN_FILES;
						bPostSuccessful = FALSE;
						#ifdef _LOG_IN_FILE
							RW_DEBUG  << "\n ConfigureDUN PostUnSuccessful" << flush;
						#endif

					}
					else
					{
						
						#ifdef _LOG_IN_FILE
							RW_DEBUG  << "\n ConfigureDUN Successful" << flush;
						#endif
					}
				}
				
				 iRetry = 1;
				//  Check for Errors
				if(pi->iError) {
					pi->iCancelledByUser = RWZ_ABORT_TOFINISH;
					PropSheet_PressButton (GetParent( hwndDlg ),PSBTN_NEXT);
				}else {
					bIsPhoneBookCreated = TRUE;	
					pDH->DisplayPhoneNumber();
					//PropSheet_SetWizButtons( GetParent( hwndDlg ), PSWIZB_BACK PSWIZB_NEXT );
					pi->iCancelledByUser = RWZ_PAGE_OK;
					PropSheet_SetWizButtons( GetParent( hwndDlg ), 0);
				}
            break;

			case PSN_KILLACTIVE :
				if(pi->pDialupHelper) {
					delete pi->pDialupHelper;
					
				}
				pi->pDialupHelper = NULL;
			break;

            case PSN_WIZNEXT:
				iRet=0;
									//
				// Delete any RAS Connection

				pDH->DestroyRasThread(FALSE);

				ATK_RasDeleteEntry(NULL,MSN_SITE_DUN);

				if(pi->iCancelledByUser  == RWZ_CANCELLED_BY_USER  ||
					pi->iCancelledByUser == RWZ_ABORT_TOFINISH)
				{
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
					iRet = 1;
					pi->ErrorPage  = kDialupDialog;
					pi->iError     = RWZ_ERROR_CANCELLED_BY_USER;
					bPostSuccessful = FALSE;

					SetWindowLongPtr( hwndDlg,DWLP_MSGRESULT, (LONG_PTR) iRet);
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
		case WM_COMMAND:
			switch (LOWORD(wParam)){
			case IDDIAL :
					 RW_DEBUG  << "\n IN IDDIAL ....." << flush;
					//
					//

					if(!bIsPhoneBookCreated){
						// Go To the Next Modem
						iModemIndex ++;
						iRetry = 1;
						if(!ConfigureDUN(hwndDlg,
							pi->hInstance,
							&theRasEntry,
							szIspUserName,szIspUserPassword,
							iModemIndex) )
						{
								pi->iError  = RWZ_ERROR_MODEM_CFG_ERROR;
								pi->iCancelledByUser = RWZ_ABORT_TOFINISH;
								bPostSuccessful = FALSE;
								#ifdef _LOG_IN_FILE
									RW_DEBUG  << "\n ConfigureDUN PostUnSuccessful" << flush;
								#endif
								goto PrepareForExit;							
						}
						else
						{
							#ifdef _LOG_IN_FILE
								RW_DEBUG  << "\n ConfigureDUN Successful" << flush;
								RW_DEBUG  << "\n Phone number:" <<ConvertToANSIString(theRasEntry.szAreaCode)<<ConvertToANSIString(theRasEntry.szLocalPhoneNumber)<< flush;
							#endif
						}

						
						pDH->DisplayPhoneNumber();
#ifdef _DISPLAY_MODEM_NAME
						SetDlgItemText( m_hDlg, IDC_MODEM_NAME, (LPCTSTR) theRasEntry.szDeviceName );
#endif
					}
					bIsPhoneBookCreated = TRUE;
					FEnableControl(hwndDlg,IDDIAL,FALSE);
					FEnableControl(hwndDlg,IDC_DISCONNECT,TRUE);
					//FEnableControl(hwndDlg,ID_BTNSETTINGS,FALSE);
					pi->ErrorPage  = kDialupDialog;
					pi->iError     =  0; // Dialup Errors
					bPostSuccessful = TRUE;

					if (pDH->CreateRasDialThread() == DIALFAILED ){
						pi->iError = RWZ_ERROR_SYSTEMERROR;
						bPostSuccessful = FALSE;
						goto  PrepareForExit;
							
					}
					goto CoolExit;
		 	 	
PrepareForExit :
					pi->iCancelledByUser = RWZ_ABORT_TOFINISH;
					PropSheet_PressButton (GetParent( hwndDlg ),PSBTN_NEXT);

CoolExit:			
                FEnableControl(hwndDlg,IDC_DISCONNECT,TRUE);

				break;
			case  IDC_DISCONNECT:
				FEnableControl(hwndDlg,IDC_DISCONNECT,FALSE);
				
				SetDlgItemText(hwndDlg, ID_LABELINIT, (LPCTSTR) _T("Disconnecting device"));
				
				SetEvent(hRasKillEvt);

				pDH->DestroyRasThread(FALSE);
				Sleep(1000);
				// Retry
				pDH->DestroyRasThread(TRUE);
				Sleep(3000);
				
				//RW_DEBUG  << "RasConnection: " << hRasConn <<endl;

				hRasConn = NULL;
				FEnableControl(hwndDlg,IDDIAL,TRUE);
				SetDlgItemText(hwndDlg, ID_LABELINIT, (LPCTSTR) _T("  "));
				break;
			case IDEND: // This Message is sent Afetr Posting
					if (m_hThread){
						CloseHandle(m_hThread);
						m_hThread = NULL;
					}
					pi->iError = lParam;
					if(pi->iError == RWZ_POST_SUCCESS)
					{
						bPostSuccessful = TRUE;
						pi->iCancelledByUser = RWZ_PAGE_OK;
						RW_DEBUG  << "\n Post Successful" << flush;
					}
					else
					{
						bPostSuccessful = FALSE;
						pi->iCancelledByUser = RWZ_ABORT_TOFINISH;
						RW_DEBUG  << "\n Post UNSuccessful" << flush;
					}
					
					PropSheet_PressButton (GetParent( hwndDlg ),PSBTN_NEXT);
			break;

			case IDD_DIALUP_ERROR :
				iMsgId = IDS_MODEM_NODIALTONE;
				RW_DEBUG << "\n IN IDS_DIALUP_ERROR " << (ULONG)lParam << flush;
				switch( lParam ) {
					case ERROR_NO_DIALTONE :
						iMsgId = IDS_MODEM_NODIALTONE;
						goto CntPrcs;
					case ERROR_NO_ANSWER   :
					
						iMsgId = IDS_MODEM_NOANSWER;
						 goto CntPrcs;
	      	            case ERROR_PORT_OR_DEVICE : // 692
					case ERROR_HARDWARE_FAILURE :
					case ERROR_DISCONNECTION : // 628
					case ERROR_FROM_DEVICE  :  // 651 :
						iMsgId = IDS_HARDWARE_FAILURE;
CntPrcs :				pDH->DestroyRasThread(FALSE);
						pDH->DestroyRasThread(TRUE);
						if( iRetry > 3 ){
							 RW_DEBUG  << "\n Automatic  Switch ...." << flush;

							pDH->DestroyRasThread(FALSE);
							bIsPhoneBookCreated = FALSE;
							PostMessage( hwndDlg, WM_COMMAND, (WPARAM) IDDIAL,0 );
							goto LReturn;
						}
						
						if(iRetry++ > 0){
						
							LoadString(m_hInstance,iMsgId,szMessage,256);
							RegWizardMessageEx(m_hInstance,hwndDlg ,IDD_INVALID_DLG,szMessage);
							//MessageBox(m_hDlg,szMessage,szWindowsCaption,MB_OK|MB_ICONEXCLAMATION);
						}
						SetDlgItemText( m_hWnd, ID_LABELINIT, (LPCTSTR) _T("  "));
						FEnableControl(hwndDlg,IDDIAL,TRUE);		//ensure that these are enabled..
						FEnableControl(hwndDlg,IDC_DISCONNECT,FALSE);		
						//FEnableControl(hwndDlg,ID_BTNSETTINGS,TRUE);

						//PostMessage( m_hDlg, WM_COMMAND, (WPARAM) IDOK,0 );
						break;
					case ERROR_LINE_BUSY :
						
						pi->iError = RWZ_ERROR_NO_ANSWER;
						pi->iCancelledByUser = RWZ_ABORT_TOFINISH;
						bPostSuccessful = FALSE;
						PropSheet_PressButton (GetParent( hwndDlg ),PSBTN_NEXT);
						break;
					case ERROR_PORT_NOT_AVAILABLE:
					case ERROR_DEVICE_NOT_READY :
					default :

						RW_DEBUG << "\n RAS ERROR PORT NOT AVAILABLE " << flush;
						pi->iError = RWZ_ERROR_MODEM_CFG_ERROR;
						pi->iCancelledByUser = RWZ_ABORT_TOFINISH;
						bPostSuccessful = FALSE;
						PropSheet_PressButton (GetParent( hwndDlg ),PSBTN_NEXT);
						break;

					}

					break;


			}
		break;
		// WM_COMMAND
        default:
			bStatus = FALSE;
            break;
    }
LReturn :
    return bStatus;
}
