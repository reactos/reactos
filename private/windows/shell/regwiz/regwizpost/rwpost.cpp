/////////////////////////////////////////
//   File:          RWPost.cpp
//
//////////////////////////////////////////   

//#define STRICT
//   Include Files
//
//Modifications :
//MDX1	03/11/99  Suresh
//	In SendHTTPData() the MSID will be got from Cookie 
//	it will no longer be red from the Registry 
//
//


#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "resource.h"
#include "Ithread.h"
#include "icw.h"
#include "util.h"
#include "tcp.h"
#include "rw_common.h"
#include "dconv.h"
#include "RwPost.h"
#include "mcm.h"
#include "ATK_RAS.h"
#include "tcp.h"
#define  CONNECTION_TIME_OUT  1000 * 60 
#define  MAX_PROXY_AUTH_RETRY  2

static CInternetThread		theInternetClass;
extern BOOL bOemDllLoaded;
extern HANDLE hOemDll;
static  DWORD dwConnectionStatus = DIALUP_NOT_REQUIRED; 

extern DWORD InvokePost(HWND hWnd,CInternetThread *p);

void InitializeInetThread(HINSTANCE hIns)
{	
	theInternetClass.Initialize(hIns);
}

//
//	Returns 
//	DIALUP_NOT_REQUIRED  : Use Network for tx
//  DIALUP_REQUIRED       : Use Dialupo for Tx  
//  RWZ_ERROR_NOTCPIP      : No TCP/IPO
//  CONNECTION_CANNOT_BE_ESTABLISHED  : No modem or RAS setup

DWORD CheckInternetConnectivityExists(HWND hWnd, HINSTANCE hInstance)
{
	static  int iAuthRetry =0; // Retry count for the number of times to invoke Proxy Auth Dlg
	static	CHAR szProxyServer[MAX_PATH];
	CHAR    szProxySettings[MAX_PATH];
	static  int   iChkInternetConnection = 0; 
	static  DWORD sdwConnectionStatus = CONNECTION_CANNOT_BE_ESTABLISHED; 
	BOOL	bNeedsReboot;
	BOOL	bRet; 
	DWORD   dwPingStatus;
	DWORD	dwError= RWZ_NOERROR;
	BOOL	bProxyExists;
	MODEMSTATUS  mStatus;
	DWORD	dwTimeOut = CONNECTION_TIME_OUT;
	int		iProxyPort;
	TCHAR	szUserName[48] = _T(""),
			szPassword[48] = _T("");
	int     iDisableAutoDial;

	iDisableAutoDial=1;

	if( sdwConnectionStatus == DIALUP_NOT_REQUIRED )
	return DIALUP_NOT_REQUIRED;
	if( sdwConnectionStatus == DIALUP_REQUIRED  )
	return DIALUP_REQUIRED;

	// Disable Auto Dial only 
	// if there are not active Dialup Connection
	//
	if( ATK_IsRasDllOk() == RAS_DLL_LOADED ) 
	{
			if( IsDialupConnectionActive() ) 
			{
				iDisableAutoDial=0;
			}
	}
	if(iDisableAutoDial) 
	{
		DisableAutoDial();// Disable the Auto Dial
	}
	// Ping Current Host to check if TCP is 
	// installed /configured.
	// 
	// if it is for the first time 

	//  RWZ_PINGSTATUS_NOTCPIP : if no socket library or get hostname fails  
	//  RWZ_PINGSTATUS_SUCCESS : if gethostname and ping is successful 
	//  RWZ_PINGSTATUS_FAIL    : if gethostname is succesful and ping via icmp fails 
	dwPingStatus = PingHost();

	RW_DEBUG  <<"\n Ping To Host (40: No TCP/IP   41: Success  42: Failure)=: "   << dwPingStatus << flush;

	if (dwPingStatus == RWZ_PINGSTATUS_NOTCPIP )
	{
		return RWZ_ERROR_NOTCPIP;
	} 
	if( dwPingStatus == RWZ_PINGSTATUS_SUCCESS ){
		bProxyExists = theInternetClass.GetSystemProxyServer(szProxyServer,MAX_PATH, &iProxyPort);
		if (1 /*bProxyExists*/) 
		{
			//theInternetClass.GetSystemProxySettings(szProxySettings,MAX_PATH);
			//theInternetClass.SetSystemProxySettings(szProxySettings);
			theInternetClass.SetSystemProxySettings("itgproxy");
			
			if(1 /*Ping(szProxyServer)*/) 
			{
				DWORD dwChkSite;
				int   iExit;
				iExit =0;
				RW_DEBUG  <<"\n Ping Success" << flush;
					theInternetClass.m_UserName[0] = _T('\0');
					theInternetClass.m_Password[0] = _T('\0');

				
				do {
					dwChkSite = ChkSiteAvailability(hWnd, theInternetClass.m_strIISServer,
						dwTimeOut,
						(LPTSTR) theInternetClass.GetProxyServer(),
						theInternetClass.m_UserName,theInternetClass.m_Password);
						
						RW_DEBUG  <<"\n After  ChkSiteAvailability : " << dwChkSite  << flush;

						if( dwChkSite == RWZ_SITE_REQUIRES_AUTHENTICATION) {
							if( iAuthRetry++ > MAX_PROXY_AUTH_RETRY) {
								iExit =1;
							}
							// Modified on 2/4/98
							// No Need to call our Proxy Auth Dlg insted use
							// InternetErrorDlg() to invoke Auth Dlg
							//if(GetProxyAuthenticationInfo(hInstance,ConvertToUnicode(szProxyServer),
							//	theInternetClass.m_UserName,theInternetClass.m_Password)) {
							//}


						}else {
							// Exit because  Connectivity is OK 
							iExit = 1;
						}
				}while(!iExit);

				if( dwChkSite == RWZ_SITE_CONNECTED)
				{
					theInternetClass.m_dwAccessType = INTERNET_OPEN_TYPE_PRECONFIG;
					// Modified on 2/4/98to use the PRECONFIGIED rather than the PROXY specified by
					// the user programatically
					// The INTERNET_OPEN_TYPE_PROXY is changed .....
					// This change is done in order for IE Auth Dlg
					dwError = DIALUP_NOT_REQUIRED;
					sdwConnectionStatus = DIALUP_NOT_REQUIRED;
					dwConnectionStatus = DIALUP_NOT_REQUIRED;
					goto ExitChk;
				}

			}

		}
		
		// No Procy so check for connection using existing LAN 
		// already opened Dialup Connection 
		// Set to NULL Proxy  
		//
				
		theInternetClass.SetProxyServer("",80);// Set it To Null 
		theInternetClass.m_dwAccessType = INTERNET_OPEN_TYPE_DIRECT;
		if( ATK_IsRasDllOk() == RAS_DLL_LOADED ) 
		{
			if( IsDialupConnectionActive() ) 
			{
				// Already Dialup COnnection is Active
				dwError = DIALUP_NOT_REQUIRED;
				sdwConnectionStatus = DIALUP_NOT_REQUIRED;
				dwConnectionStatus = DIALUP_NOT_REQUIRED;
				goto ExitChk;
			}
		}

		//
		// Check Lan Connection
		bRet = CheckHostName( ConvertToANSIString(theInternetClass.m_strIISServer));
		if(bRet)
		{
			theInternetClass.m_dwAccessType = INTERNET_OPEN_TYPE_DIRECT;
			if(ChkSiteAvailability(NULL, theInternetClass.m_strIISServer,
			dwTimeOut,
			_T(""),szUserName,szPassword)) 
			{
				dwError = DIALUP_NOT_REQUIRED;
				sdwConnectionStatus = DIALUP_NOT_REQUIRED;
				dwConnectionStatus = DIALUP_NOT_REQUIRED;
				goto ExitChk;
			}
		}

	}	

	dwError = DIALUP_REQUIRED;	
	sdwConnectionStatus = DIALUP_REQUIRED;
	dwConnectionStatus = DIALUP_NOT_REQUIRED;
	theInternetClass.m_dwAccessType = INTERNET_OPEN_TYPE_DIRECT;
	
	RW_DEBUG << "\n Before  MDMCHK..." << flush;
	//MDMCHK:
	// Install Modem
	//
	/**bNeedsReboot = theInternetClass.InstallModem(hWnd);
	if(bNeedsReboot) 
	{
			// ?????
			//  This will be abnormally terminating the Registration Wizard, 
			//  So support modem installation in  OS which dosent call for a reboot
	}
	**/
	mStatus = MSDetectModemTAPI(hInstance);
	if(mStatus != kMsModemOk ) 
	{
			dwError = CONNECTION_CANNOT_BE_ESTABLISHED;
			sdwConnectionStatus = CONNECTION_CANNOT_BE_ESTABLISHED;
	}

	if(dwError == DIALUP_REQUIRED ) 
	{
		// Load RASPAI32.DLL and Exit if it can not be loaded
		if( ATK_IsRasDllOk() != RAS_DLL_LOADED ) 
		{
			//
			dwError = CONNECTION_CANNOT_BE_ESTABLISHED;
			sdwConnectionStatus = CONNECTION_CANNOT_BE_ESTABLISHED;
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n RASAPI32.DLL NOT FOUND ...";
			#endif
		}
	}

ExitChk:
	#ifdef _LOG_IN_FILE
		RW_DEBUG  <<"\n Chk Connection ( 1 = via NTWK, 2 = DIalup , 3 = Problem) "   << dwError << flush;
	#endif
	theInternetClass.UnLoadInetCfgDll();
	return dwError;


}

//
//	Returns 
//	DIALUP_NOT_REQUIRED  : Use Network for tx
//  DIALUP_REQUIRED       : Use Dialupo for Tx  
//  RWZ_ERROR_NOTCPIP      : No TCP/IPO
//  CONNECTION_CANNOT_BE_ESTABLISHED  : No modem or RAS setup

DWORD CheckInternetConnectivityExistsOldLogic(HWND hWnd, HINSTANCE hInstance)
{
	static  int iAuthRetry =0; // Retry count for the number of times to invoke Proxy Auth Dlg
	static	CHAR szProxyServer[MAX_PATH];
	CHAR    szProxySettings[MAX_PATH];
	static  int   iChkInternetConnection = 0; 
	static  DWORD sdwConnectionStatus = CONNECTION_CANNOT_BE_ESTABLISHED; 
	BOOL	bNeedsReboot;
	BOOL	bRet; 
	DWORD   dwPingStatus;
	DWORD	dwError= RWZ_NOERROR;
	BOOL	bProxyExists;
	MODEMSTATUS  mStatus;
	DWORD	dwTimeOut = CONNECTION_TIME_OUT;
	int		iProxyPort;
	TCHAR	szUserName[48] = _T(""),
			szPassword[48] = _T("");
	int     iDisableAutoDial;

	iDisableAutoDial=1;

	if( sdwConnectionStatus == DIALUP_NOT_REQUIRED )
	return DIALUP_NOT_REQUIRED;
	if( sdwConnectionStatus == DIALUP_REQUIRED  )
	return DIALUP_REQUIRED;

	// Disable Auto Dial only 
	// if there are not active Dialup Connection
	//
	if( ATK_IsRasDllOk() == RAS_DLL_LOADED ) 
	{
			if( IsDialupConnectionActive() ) 
			{
				iDisableAutoDial=0;
			}
	}
	if(iDisableAutoDial) 
	{
		DisableAutoDial();// Disable the Auto Dial
	}
	// Ping Current Host to check if TCP is 
	// installed /configured.
	// 
	// if it is for the first time 

	//  RWZ_PINGSTATUS_NOTCPIP : if no socket library or get hostname fails  
	//  RWZ_PINGSTATUS_SUCCESS : if gethostname and ping is successful 
	//  RWZ_PINGSTATUS_FAIL    : if gethostname is succesful and ping via icmp fails 
	dwPingStatus = PingHost();

	RW_DEBUG  <<"\n Ping To Host (40: No TCP/IP   41: Success  42: Failure)=: "   << dwPingStatus << flush;

	if (dwPingStatus == RWZ_PINGSTATUS_NOTCPIP )
	{
		return RWZ_ERROR_NOTCPIP;
	} 
	if( dwPingStatus == RWZ_PINGSTATUS_SUCCESS ){
		bProxyExists = theInternetClass.GetSystemProxyServer(szProxyServer,MAX_PATH, &iProxyPort);
		if (bProxyExists) 
		{
			theInternetClass.GetSystemProxySettings(szProxySettings,MAX_PATH);
			theInternetClass.SetSystemProxySettings(szProxySettings);
			
			
			if(Ping(szProxyServer)) 
			{
				DWORD dwChkSite;
				int   iExit;
				iExit =0;
				RW_DEBUG  <<"\n Ping Success" << flush;
					theInternetClass.m_UserName[0] = _T('\0');
					theInternetClass.m_Password[0] = _T('\0');

				
				do {
					dwChkSite = ChkSiteAvailability(hWnd, theInternetClass.m_strIISServer,
						dwTimeOut,
						(LPTSTR) theInternetClass.GetProxyServer(),
						theInternetClass.m_UserName,theInternetClass.m_Password);
						
						RW_DEBUG  <<"\n After  ChkSiteAvailability : " << dwChkSite  << flush;

						if( dwChkSite == RWZ_SITE_REQUIRES_AUTHENTICATION) {
							if( iAuthRetry++ > MAX_PROXY_AUTH_RETRY) {
								iExit =1;
							}
							// Modified on 2/4/98
							// No Need to call our Proxy Auth Dlg insted use
							// InternetErrorDlg() to invoke Auth Dlg
							//if(GetProxyAuthenticationInfo(hInstance,ConvertToUnicode(szProxyServer),
							//	theInternetClass.m_UserName,theInternetClass.m_Password)) {
							//}


						}else {
							// Exit because  Connectivity is OK 
							iExit = 1;
						}
				}while(!iExit);

				if( dwChkSite == RWZ_SITE_CONNECTED)
				{
					theInternetClass.m_dwAccessType = INTERNET_OPEN_TYPE_PRECONFIG;
					// Modified on 2/4/98to use the PRECONFIGIED rather than the PROXY specified by
					// the user programatically
					// The INTERNET_OPEN_TYPE_PROXY is changed .....
					// This change is done in order for IE Auth Dlg
					dwError = DIALUP_NOT_REQUIRED;
					sdwConnectionStatus = DIALUP_NOT_REQUIRED;
					dwConnectionStatus = DIALUP_NOT_REQUIRED;
					goto ExitChk;
				}

			}

		}
		
		// No Procy so check for connection using existing LAN 
		// already opened Dialup Connection 
		// Set to NULL Proxy  
		//
				
		theInternetClass.SetProxyServer("",80);// Set it To Null 
		theInternetClass.m_dwAccessType = INTERNET_OPEN_TYPE_DIRECT;
		if( ATK_IsRasDllOk() == RAS_DLL_LOADED ) 
		{
			if( IsDialupConnectionActive() ) 
			{
				// Already Dialup COnnection is Active
				dwError = DIALUP_NOT_REQUIRED;
				sdwConnectionStatus = DIALUP_NOT_REQUIRED;
				dwConnectionStatus = DIALUP_NOT_REQUIRED;
				goto ExitChk;
			}
		}

		//
		// Check Lan Connection
		bRet = CheckHostName( ConvertToANSIString(theInternetClass.m_strIISServer));
		if(bRet)
		{
			theInternetClass.m_dwAccessType = INTERNET_OPEN_TYPE_DIRECT;
			if(ChkSiteAvailability(NULL, theInternetClass.m_strIISServer,
			dwTimeOut,
			_T(""),szUserName,szPassword)) 
			{
				dwError = DIALUP_NOT_REQUIRED;
				sdwConnectionStatus = DIALUP_NOT_REQUIRED;
				dwConnectionStatus = DIALUP_NOT_REQUIRED;
				goto ExitChk;
			}
		}

	}	

	dwError = DIALUP_REQUIRED;	
	sdwConnectionStatus = DIALUP_REQUIRED;
	dwConnectionStatus = DIALUP_NOT_REQUIRED;
	theInternetClass.m_dwAccessType = INTERNET_OPEN_TYPE_DIRECT;
	
	RW_DEBUG << "\n Before  MDMCHK..." << flush;
//MDMCHK:
	// Install Modem
	//
	bNeedsReboot = theInternetClass.InstallModem(hWnd);
	if(bNeedsReboot) 
	{
			// ?????
			//  This will be abnormally terminating the Registration Wizard, 
			//  So support modem installation in  OS which dosent call for a reboot
	}

	mStatus = MSDetectModemTAPI(hInstance);
	if(mStatus != kMsModemOk ) 
	{
			dwError = CONNECTION_CANNOT_BE_ESTABLISHED;
			sdwConnectionStatus = CONNECTION_CANNOT_BE_ESTABLISHED;
	}

	if(dwError == DIALUP_REQUIRED ) 
	{
		// Load RASPAI32.DLL and Exit if it can not be loaded
		if( ATK_IsRasDllOk() != RAS_DLL_LOADED ) 
		{
			//
			dwError = CONNECTION_CANNOT_BE_ESTABLISHED;
			sdwConnectionStatus = CONNECTION_CANNOT_BE_ESTABLISHED;
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n RASAPI32.DLL NOT FOUND ...";
			#endif
		}
	}

ExitChk:
	#ifdef _LOG_IN_FILE
		RW_DEBUG  <<"\n Chk Connection ( 1 = via NTWK, 2 = DIalup , 3 = Problem) "   << dwError << flush;
	#endif
	theInternetClass.UnLoadInetCfgDll();
	return dwError;


}


DWORD SendHTTPData(HWND hWnd, HINSTANCE hInstance)
{
	char czB [MAX_BUFFER + 1]; // Buffer for Tx
	DWORD dwBufSize = MAX_BUFFER;
	DWORD dwOemBufSize;
	DWORD dwRet;
	_TCHAR szValue[256];


	DWORD	dwTimeOut = CONNECTION_TIME_OUT;

	// MDX : 03/11/99 
	// Get MSID From Cookie , No need to check from Registry
	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n*******Getting Cookie********\n"<< flush;
	#endif
	if(dwConnectionStatus == DIALUP_REQUIRED){
			ChkSiteAvailability(hWnd, theInternetClass.m_strIISServer,
				dwTimeOut,
				NULL,
				theInternetClass.m_UserName,
				theInternetClass.m_Password);
	}
	else{
		ChkSiteAvailability(hWnd, theInternetClass.m_strIISServer,
				dwTimeOut,
				(LPTSTR) theInternetClass.GetProxyServer(),
				theInternetClass.m_UserName,
				theInternetClass.m_Password);
	}

	SetMSID(hInstance);
	
	
	dwRet  = PrepareRegWizTxbuffer(hInstance, czB, &dwBufSize);
	dwOemBufSize = MAX_BUFFER - dwBufSize;
	dwRet  = OemTransmitBuffer(hInstance,czB + dwBufSize,&dwOemBufSize);
	dwBufSize += dwOemBufSize;

	switch(dwRet) 
	{
		case  RWZ_NOERROR:
			theInternetClass.SetBuffer(czB, dwBufSize+1);
			theInternetClass.SetSSLFlag(TRUE);
			dwRet = theInternetClass.PostData(hWnd);
			// dwRet = InvokePost(hWnd, &theInternetClass);

			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n PostData() returned: "<<dwRet << flush;
			#endif

			if( dwRet == RWZ_POST_FAILURE  ||  dwRet == RWZ_POST_WITH_SSL_FAILURE)
			{
				// Try posting without SSL only for the modem
				//if(dwConnectionStatus == DIALUP_REQUIRED)
				//{
					#ifdef _LOG_IN_FILE
						RW_DEBUG << "\n Posting Failure : Sending Data without SSL" << flush;
					#endif
					theInternetClass.SetSSLFlag(FALSE);
					dwRet = theInternetClass.PostData(hWnd);
					//dwRet = InvokePost(hWnd, &theInternetClass);
				//}
			}
			#ifdef _LOG_IN_FILE
				RW_DEBUG  << "\n Success ... \t"  << dwRet << flush;
				RW_DEBUG   <<"\n\n\nBuffer\t\t*[" << czB << "]" << flush;
			#endif
			break;
		case  RWZ_NO_INFO_AVAILABLE :
			#ifdef _LOG_IN_FILE
				RW_DEBUG  << _T("\n No Info Available ")  << flush;
			#endif
			break;
		case  RWZ_INVALID_INFORMATION :
			#ifdef _LOG_IN_FILE
				RW_DEBUG  << _T("\n Invalid Info  " ) << flush;
			#endif
			break;
		case  RWZ_BUFFER_SIZE_INSUFFICIENT :
			#ifdef _LOG_IN_FILE
				RW_DEBUG  <<_T("\n Buffer Length In Sufficient ...") << dwRet;
				RW_DEBUG   <<_T("\n\n\nBuffer\t\t") << czB;
			#endif
			break;
		case    RWZ_INTERNAL_ERROR	 :
			#ifdef _LOG_IN_FILE
				RW_DEBUG  << _T("\n Internal Error ....") ;
			#endif
		default:
			break;
	}
	#ifdef _LOG_IN_FILE
		RW_DEBUG  << flush;
	#endif
	return dwRet;
}


/*DWORD PostHTTPData(HINSTANCE hInstance)
{
	DWORD dwRet = RWZ_POST_FAILURE;
	DWORD dwRetStatus;

	dwRetStatus = CheckWithDisplayInternetConnectivityExists(hInstance,2);

	switch (dwRetStatus)
	{
	case DIALUP_NOT_REQUIRED  :
		if ((dwRet =  PostDataWithWindowMessage(hInstance)) 
				== RWZ_POST_SUCCESS){
			;
		}
		else {
		}
		break;
	case DIALUP_REQUIRED      :
		 dwRet=DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIAL), NULL,(DLGPROC)FDlgProc,
			 (LPARAM)hInstance);
		 if(dwRet == -1 ) {
			 // Error in creating the Dialogue

		 }
		switch ( dwRet)  {
		case RWZ_ERROR_LOCATING_MSN_FILES :
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n Signup :Error Locating MSN File " << flush;
			#endif
			break;
		case RWZ_ERROR_LOCATING_DUN_FILES   :
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n Signup :Error Locating DUN File " << flush;
			#endif
			break;
		case RWZ_ERROR_MODEM_IN_USE :
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n Signup :Error Modem Already in use by another Application " << flush;
			#endif
			break;
		case RWZ_ERROR_MODEM_CFG_ERROR:
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n Signup :Modem Configuration Error " << flush;
			#endif
		case RWZ_ERROR_TXFER_CANCELLED_BY_USER :
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n Signup     :HTTP Post Cancelled by User  " << flush;
			#endif
			break;
		case RWZ_ERROR_SYSTEMERROR :
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n Signup     : System Resource Allocation Error  " << flush;
			#endif
			break;
		case RWZ_ERROR_NODIALTONE :
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n Signup     : Modem Error No Dialtone " << flush;
			#endif
			break;
		default :
			break;
		}

				 
		
	case  CONNECTION_CANNOT_BE_ESTABLISHED  :
	default :
		// It is unexpected . ? to Do
	
	break;
	
	}
	return dwRet;
}
**/

