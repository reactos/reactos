//#define STRICT
//   Include Files
//
#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
//#include <wininet.h>
#include  <ATKInternet.h>

#include "rwpost.h"
#include "rw_common.h"

#ifdef _DEBUG
#include <iostream.h>
#endif



static void _stdcall  myCallback(
    IN HINTERNET hInternet,
    IN DWORD_PTR dwContext,
    IN DWORD dwInternetStatus,
    IN LPVOID lpvStatusInformation OPTIONAL,
    IN DWORD dwStatusInformationLength
	);

//
// returns
// #define  RWZ_SITE_CONNECTED   1
// #define  RWZ_FAIL_TOCONNECTTOSITE  0
// #define  RWZ_SITE_REQUIRES_AUTHENTICATION 2
//
DWORD   ChkSiteAvailability( HWND hwndProxyParent, LPCTSTR szIISServer,DWORD   dwTimeOut,LPTSTR szProxyServer ,LPTSTR	szUserName,
				LPTSTR  szPassword)
				
{
	DWORD						dwRet ;
	DWORD						dwLen=1024;
	DWORD						dwAccessType ;
	HANDLE						hEvent;		
	HINTERNET					hSession;
	HINTERNET					hConnect;
	HINTERNET					hHttpFile;
	INTERNET_STATUS_CALLBACK	hI;
	char  bufQuery[32]="" ;

	DWORD dwbufQuery;
	DWORD dwLengthBufQuery;
	BOOL bQuery;

	// Date : 2/4/98
	// The below var are added for incorporatinf IE Auth Dlg
	// We are changing the Connectivity Check Via Proxy using
	// INTERNET_OPEN_PRECONFIG
	DWORD	dwProxyAuthError; // Store error returned by InternetErrorDlg
	LPTSTR  pUserName;
	LPTSTR  pPassword;
	LPTSTR  pProxyServerName;


	
	pUserName		 =_T("");
	pPassword		 =_T("");
	pProxyServerName =NULL;
	

	if(_tcscmp(szProxyServer,_T(""))){
		dwAccessType = INTERNET_OPEN_TYPE_PRECONFIG;
	}else{
		dwAccessType = INTERNET_OPEN_TYPE_DIRECT;
	}

	#ifdef _DEBUG
		RW_DEBUG << "\nServer   is :" << ConvertToANSIString(szIISServer) <<"\n" <<flush;
		RW_DEBUG << "\nTimeout  is :" << dwTimeOut <<"\n" <<flush;
		if(_tcscmp(szProxyServer,_T(""))){
			RW_DEBUG << "Proxy    is :" << ConvertToANSIString(szProxyServer) <<"\n" <<flush;
		}else{
			RW_DEBUG << "Proxy    is : NULL" <<"\n" <<flush;
		}
	#endif
	
	hSession = ATK_InternetOpen	(	_T("Registration Wizard"),
										dwAccessType,
						                pProxyServerName,
										NULL, // List of Addesss to ByPass
										INTERNET_FLAG_ASYNC
				   					);

	if (hSession == NULL){
		dwRet = RWZ_FAIL_TOCONNECTTOSITE;
		#ifdef _DEBUG
			RW_DEBUG << "InternetOpen Failed\n" << flush;
		#endif
		return dwRet;
	}
    hConnect = ATK_InternetConnect(	hSession,
									szIISServer,
									INTERNET_INVALID_PORT_NUMBER,
									pUserName,
									pPassword,
									INTERNET_SERVICE_HTTP,
									0,
									1
								   );
	if(hConnect == NULL){
		::InternetCloseHandle(hSession);
		#ifdef _DEBUG
		RW_DEBUG << "InternetConnect Failed\n" << flush;
		#endif
		dwRet = RWZ_FAIL_TOCONNECTTOSITE;
		return dwRet;
	}
	hEvent		= CreateEvent(NULL, FALSE,FALSE,_T("HTTPConnect"));
	hHttpFile	= ATK_HttpOpenRequest(	hConnect,
										_T("GET"),
										_T("/register.asp"),	
										HTTP_VERSION,
										NULL,
										0,
										INTERNET_FLAG_DONT_CACHE,
										(DWORD_PTR)hEvent
										);
	if (hHttpFile == NULL){
	#ifdef _DEBUG
			RW_DEBUG << "HttpOpenRequest Failed\n" << flush;
	#endif
		dwRet =  RWZ_FAIL_TOCONNECTTOSITE;
		CloseHandle(hEvent);
		::InternetCloseHandle(hConnect);
		::InternetCloseHandle(hSession);
		return dwRet;
	}
	//
	// Http Open is Success so continue further
	hI = InternetSetStatusCallback(hHttpFile,myCallback);
	BOOL bSendRequest = ATK_HttpSendRequest(hHttpFile, NULL, 0, 0, 0);
	DWORD dwWaitStatus = WaitForSingleObject(hEvent,dwTimeOut) ;
	// Time out Error
	if( dwWaitStatus == WAIT_TIMEOUT ){
		#ifdef _DEBUG
		RW_DEBUG << "Timeout Happened\n" << flush;
		#endif
		RW_DEBUG << "Timeout Happened\n" << flush;
		dwRet = RWZ_FAIL_TOCONNECTTOSITE;
		goto FinishFn;
	}
	if(dwWaitStatus == WAIT_FAILED){
	#ifdef _DEBUG
		RW_DEBUG << "WAIT_FAILED returned from WaitForSingleObject\n" << flush;
	#endif
		dwRet = RWZ_FAIL_TOCONNECTTOSITE;
		goto FinishFn;
	}

	dwLengthBufQuery = sizeof (dwbufQuery);
	//DWORD dwLengthBufQuery = sizeof ( bufQuery);
	bQuery = ATK_HttpQueryInfo(	hHttpFile,
										HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
										//HTTP_QUERY_CONTENT_DESCRIPTION ,
										//HTTP_QUERY_CONTENT_LENGTH ,
										&dwbufQuery,
										//bufQuery ,
				                        &dwLengthBufQuery,
										NULL) ;
	RW_DEBUG << "\nChecking Connection  To Site  via Proxy Returns[" << dwbufQuery << "]" <<flush;
	switch(dwbufQuery) {
		case HTTP_STATUS_OK:
		case HTTP_STATUS_SERVICE_UNAVAIL:
		dwRet = RWZ_SITE_CONNECTED;
		break;
		case HTTP_STATUS_PROXY_AUTH_REQ:
		// Invoking the IE Proxy Server Authentication Dialog
				dwProxyAuthError = 	InternetErrorDlg(hwndProxyParent,hHttpFile,
				ERROR_INTERNET_INCORRECT_PASSWORD,
				FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
				NULL);
				RW_DEBUG << "\nInetErrorDialog REturns " << dwProxyAuthError << flush;
				dwRet = RWZ_SITE_REQUIRES_AUTHENTICATION;
				//
				// We are not checking  for the return value because
				// connection has to be reestablished to really check
				// the info provided is valid
				/**
				02/07/98
				switch (dwProxyAuthError) {
					case ERROR_SUCCESS:
						dwRet = RWZ_SITE_CONNECTED;
						break;
					case ERROR_CANCELLED :
					case ERROR_INTERNET_FORCE_RETRY:
						dwRet = RWZ_SITE_REQUIRES_AUTHENTICATION;
						default:
						break;
					}
					**/
			break; // End of Proxy Auth
		case 0:
		case HTTP_STATUS_SERVER_ERROR:
			dwRet = RWZ_FAIL_TOCONNECTTOSITE;
			RW_DEBUG << "\n Problem in Proxy Server With connection Via Proxy " << dwbufQuery << flush;
			break;
		default:
		dwRet = RWZ_SITE_CONNECTED;
		break;	
	}			

FinishFn :
	CloseHandle(hEvent);
	::InternetCloseHandle(hHttpFile);
	::InternetCloseHandle(hConnect);
	::InternetCloseHandle(hSession);
	return dwRet;
}

void _stdcall myCallback(
						    IN HINTERNET hInternet,
						    IN DWORD_PTR dwContext,
						    IN DWORD dwInternetStatus,
						    IN LPVOID lpvStatusInformation OPTIONAL,
						    IN DWORD dwStatusInformationLength
						)
{

switch(dwInternetStatus)
	{
	case INTERNET_STATUS_RESOLVING_NAME :

#ifdef _DEBUG
			 RW_DEBUG << "\nCallback: RESOLVING_NAME\n" << flush;
#endif //_DEBUG
		 break;
	case INTERNET_STATUS_NAME_RESOLVED :

#ifdef _DEBUG
			 RW_DEBUG << "\nCallback: INTERNET_STATUS_NAME_RESOLVED\n" << flush;
#endif //_DEBUG
		 break;

	case INTERNET_STATUS_CONNECTING_TO_SERVER :

#ifdef _DEBUG
			 RW_DEBUG << "\nCallback: INTERNET_STATUS_CONNECTING_TO_SERVER\n" << flush;
#endif //_DEBUG
		 break;

	case INTERNET_STATUS_CONNECTED_TO_SERVER :

#ifdef _DEBUG
			 RW_DEBUG << "\nCallback: INTERNET_STATUS_CONNECTED_TO_SERVER\n" << flush;
#endif //_DEBUG
		 //::InternetCloseHandle(hInternet);
 		 break;

	case INTERNET_STATUS_SENDING_REQUEST :

#ifdef _DEBUG
			 RW_DEBUG << "\nCallback: INTERNET_STATUS_SENDING_REQUEST\n" << flush;
#endif //_DEBUG
		 break;

	case INTERNET_STATUS_REQUEST_SENT :

#ifdef _DEBUG
			 RW_DEBUG << "\nCallback: INTERNET_STATUS_REQUEST_SENT\n" << flush;
#endif //_DEBUG
		 break;

	case INTERNET_STATUS_RECEIVING_RESPONSE :

#ifdef _DEBUG
			 RW_DEBUG << "\nCallback: INTERNET_STATUS_RECEIVING_RESPONSE\n" << flush;
#endif //_DEBUG
		 break;

	case INTERNET_STATUS_RESPONSE_RECEIVED :

#ifdef _DEBUG
			 RW_DEBUG << "\nCallback: INTERNET_STATUS_RESPONSE_RECEIVED\n" << flush;
#endif //_DEBUG
		 break;

	case INTERNET_STATUS_REDIRECT :

#ifdef _DEBUG
			 RW_DEBUG << "\nCallback: INTERNET_STATUS_REDIRECT\n" << flush;
#endif //_DEBUG
		 break;

	case INTERNET_STATUS_CLOSING_CONNECTION :

#ifdef _DEBUG
			 RW_DEBUG << "\nCallback: INTERNET_STATUS_CLOSING_CONNECTION\n" << flush;
#endif //_DEBUG
		 break;

	case INTERNET_STATUS_CONNECTION_CLOSED :

#ifdef _DEBUG
			 RW_DEBUG << "\nCallback: INTERNET_STATUS_CONNECTION_CLOSED\n" << flush;
#endif //_DEBUG
		 break;
	case INTERNET_STATUS_HANDLE_CREATED :

#ifdef _DEBUG
			 RW_DEBUG << "\nCallback: INTERNET_STATUS_HANDLE_CREATED\n" << flush;
#endif //_DEBUG
		 break;
	case INTERNET_STATUS_REQUEST_COMPLETE :

#ifdef _DEBUG
		 RW_DEBUG << "\nCallback: INTERNET_STATUS_REQUEST_COMPLETE\n\n" << flush;

		 INTERNET_ASYNC_RESULT *x;
		 x = (INTERNET_ASYNC_RESULT *)lpvStatusInformation;
		 if(x->dwError == ERROR_SUCCESS)
		 {
			RW_DEBUG << "\nERROR_SUCCESS\n" << flush;
		 }
		 else
		 {
			RW_DEBUG << "\nERROR \ndwResult: " << x->dwResult <<flush;
		 }

		 if(dwStatusInformationLength == ERROR_INTERNET_EXTENDED_ERROR)
		 {
			RW_DEBUG << "\n-NO EXTENDED_ERROR\n" << flush;
		 }
			DWORD dwError;
			DWORD dwErrorLen;
			_TCHAR szErrorInfo[256];	
			dwErrorLen = 256;
			
			ATK_InternetGetLastResponseInfo(&dwError,szErrorInfo,&dwErrorLen) ;
			RW_DEBUG << "\nErrorNo:" << dwError ;
			RW_DEBUG << "\nErrorInfo:"<<szErrorInfo<<flush;
			RW_DEBUG << "\n\n" << flush;
#endif //_DEBUG
			
		 SetEvent((HANDLE)dwContext);
		 break;
	case INTERNET_STATUS_HANDLE_CLOSING :

#ifdef _DEBUG
			 RW_DEBUG << "\nCallback: INTERNET_STATUS_HANDLE_CLOSING\n" << flush;
#endif //_DEBUG
		 SetEvent((HANDLE)dwContext);
		 break;
#ifdef _DEBUG
	default :
		 RW_DEBUG <<"\n Callback : default " << flush;
#endif //_DEBUG

	}
}

#if  0
void main(int argc , char ** argv)
{

	if(argc < 2 )
	{
		RW_DEBUG << "\n\n\nThe Executable Requires the following parameters : \n" << endl;
		RW_DEBUG << " \t\tServer Name \n\t\tTimeout value \n\t\tProxyServer name\n\t\tUserName \n\t\tPassword\n\n" << endl;
		RW_DEBUG << "Following Parameters are mandatory\n\t\tServer Name \n\t\tTimeout value \n" << endl;
		return;
	}
	BOOL bRet = ChkSiteAvailability(	argv[1],atol(argv[2]),
			argv[3],argv[4],argv[5]	);

	if(bRet == TRUE)
	{
		RW_DEBUG << "Returned TRUE" << flush;
	}
	else
	{
		RW_DEBUG << "Returned FALSE" << flush;
	}

}
#endif
