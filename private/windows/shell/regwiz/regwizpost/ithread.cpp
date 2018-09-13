#include <windows.h>
#include <stdio.h>
#include "AtkInternet.h"

#include "resource.h"
#include <ithread.h>
#include "icw.h"
#include "util.h"
#include "rw_common.h"
#include "ATK_inet.h"

#define  RWZ_POST_MAX_RETRY  3

struct _MK
{
	DWORD dwQuery;
	char *s;
} ;

#define MK(x) {x,#x}


static  _MK Queries[] =
{
MK(HTTP_QUERY_MIME_VERSION),
MK(HTTP_QUERY_CONTENT_TYPE),
MK(HTTP_QUERY_CONTENT_TRANSFER_ENCODING),
MK(HTTP_QUERY_CONTENT_ID),
MK(HTTP_QUERY_CONTENT_DESCRIPTION),
MK(HTTP_QUERY_CONTENT_LENGTH),
MK(HTTP_QUERY_ALLOW),
MK(HTTP_QUERY_PUBLIC),
MK(HTTP_QUERY_DATE),
MK(HTTP_QUERY_EXPIRES),
MK(HTTP_QUERY_LAST_MODIFIED),
MK(HTTP_QUERY_MESSAGE_ID),
MK(HTTP_QUERY_URI),
MK(HTTP_QUERY_DERIVED_FROM),
MK(HTTP_QUERY_COST),
MK(HTTP_QUERY_PRAGMA),
MK(HTTP_QUERY_VERSION),
MK(HTTP_QUERY_STATUS_CODE),
MK(HTTP_QUERY_STATUS_TEXT),
MK(HTTP_QUERY_RAW_HEADERS),
MK(HTTP_QUERY_RAW_HEADERS_CRLF),
MK(HTTP_QUERY_REQUEST_METHOD)
};
void QueryForInfo(HINTERNET );
void GetQueryInfo(HINTERNET ,DWORD ,char *);

typedef BOOL (WINAPI *LPICP)(HWND,BOOL,BOOL);
LPICP lpICP;



static int InvokeModemInstallation()
{
	int iErr;
	DWORD dwPrS,dwRet;
	STARTUPINFOA startUpInfo;
	PROCESS_INFORMATION PrcsInfo;
	
	GetStartupInfoA(&startUpInfo);

#ifdef _WIN951
	ShellExecute(NULL,NULL,"control.exe","modem.cpl",
					NULL,SW_SHOW);
	return 1;
#else

	if( dwPrS=  CreateProcessA( NULL ,
				"rundll32.exe shell32.dll,Control_RunDLL modem.cpl", 0, 0,
				 FALSE,CREATE_NEW_CONSOLE, 0, 0,
				 &startUpInfo, &PrcsInfo) )
	{
		dwRet = WaitForSingleObject(PrcsInfo.hProcess, INFINITE);
		switch(dwRet)
		{
		case WAIT_ABANDONED  :
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n ABD " << flush;
			#endif
			break;
		case WAIT_OBJECT_0 :
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n OBJ " << flush;
			#endif
			break;
		case WAIT_TIMEOUT:
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n TOUT " << flush;
			#endif
			break;
		default :
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n Error  " <<  GetLastError() << flush;
			#endif
		}
		return 1;

	}
	else
	{
		iErr = GetLastError();
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\n Error In Modem Configuration"  << iErr << flush;
		#endif
		return 0;
	}
#endif

}

void QueryForInfo(HINTERNET hIntHandle)
{
	int nSize  = sizeof(Queries)/sizeof(_MK);
	for(int i=0;i< nSize;i++)
	{
		GetQueryInfo(hIntHandle,Queries[i].dwQuery,Queries[i].s);
	}
}

void GetQueryInfo(HINTERNET hIntHandle,DWORD dwQuery,char *s)
{
	char  dwbufQuery[512];
	DWORD dwLengthBufQuery;
	BOOL bQuery = ::ATK_HttpQueryInfo(	hIntHandle,
									dwQuery,
									&dwbufQuery,
			                        &dwLengthBufQuery,
									NULL) ;
	
	#ifdef _LOG_IN_FILE
		RW_DEBUG  << "\n" << s << dwbufQuery <<  flush;
	#endif
}



void _stdcall myCallback(   IN HINTERNET hInternet,
							IN DWORD dwContext,
							IN DWORD dwInternetStatus,
							IN LPVOID lpvStatusInformation OPTIONAL,
							IN DWORD dwStatusInformationLength)
{
	int iX= 100;
	iX++;

	switch(dwInternetStatus)
	{
	case INTERNET_STATUS_RESOLVING_NAME :
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nCallback: RESOLVING_NAME\n" << flush;
		#endif
		break;
	case INTERNET_STATUS_NAME_RESOLVED :
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nCallback: INTERNET_STATUS_NAME_RESOLVED\n" << flush;
		#endif
		break;
	case INTERNET_STATUS_CONNECTING_TO_SERVER :
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nCallback: INTERNET_STATUS_CONNECTING_TO_SERVER\n" << flush;
		#endif
		break;
	case INTERNET_STATUS_CONNECTED_TO_SERVER :
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nCallback: INTERNET_STATUS_CONNECTED_TO_SERVER\n" << flush;
		#endif
		break;
	case INTERNET_STATUS_SENDING_REQUEST :
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nCallback: INTERNET_STATUS_SENDING_REQUEST\n" << flush;
		#endif
		break;
	case INTERNET_STATUS_REQUEST_SENT :
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nCallback: INTERNET_STATUS_REQUEST_SENT\n" << flush;
		#endif
		break;
	case INTERNET_STATUS_RECEIVING_RESPONSE :
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nCallback: INTERNET_STATUS_RECEIVING_RESPONSE\n" << flush;
		#endif
		break;
	case INTERNET_STATUS_RESPONSE_RECEIVED :
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nCallback: INTERNET_STATUS_RESPONSE_RECEIVED\n" << flush;
		#endif
		break;
	case INTERNET_STATUS_REDIRECT :
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nCallback: INTERNET_STATUS_REDIRECT\n" << flush;
		#endif
		break;
	case INTERNET_STATUS_CLOSING_CONNECTION :
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nCallback: INTERNET_STATUS_CLOSING_CONNECTION\n" << flush;
		#endif
		break;
	case INTERNET_STATUS_CONNECTION_CLOSED :
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nCallback: INTERNET_STATUS_CONNECTION_CLOSED\n" << flush;
		#endif
		break;
	case INTERNET_STATUS_HANDLE_CREATED :
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nCallback: INTERNET_STATUS_HANDLE_CREATED\n" << flush;
		#endif
		break;
	case INTERNET_STATUS_REQUEST_COMPLETE :
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nCallback: INTERNET_STATUS_REQUEST_COMPLETE\n" << flush;
		#endif
		break;
	default :
		#ifdef _LOG_IN_FILE
			RW_DEBUG <<"\n Callback : default " << flush;
		#endif
	}
}




void pickup(const char* in, char* out, int *ipProxy)
{
	char czProzy[10]= "80";
	char *pProxy;
	int iCount ;

	pProxy = czProzy;
	
	if( *in == '/' )
		in++;

	if( *in == '/' )
		in++;

	while( *in && (*in != ':') && (*in != ';') )
		*out++ = *in++;
	
	*out = 0;
	
	if(!*in)
		return;

	if ( *in != ':' || *in != ';' )
		*in++;
	
	iCount = 0;
	
	while(*in && ( isspace(*in) || isdigit(*in)) )
	{
		if( isdigit(*in) )
		{
			*pProxy++ = *in;
			iCount++;
		}

		*in++;
	}

	if(iCount)
		*pProxy = 0;
	
	*ipProxy = atoi(czProzy);
}


/* //remove all blabks immediately preceeding ':' and afterwards.
   // eg.  "HELLO WORLD  : 80 " =>
	//     "HELLO WORLD:80"
	Remove all blanks: No blanks are allowed in a machine name?
*/

void RemoveBlank(char *pszStr)
{
	char *p, *q;
	p = pszStr;
	q = p;

	for(;*p;*p++)
	{
		if(!isspace(*p))
		{
			*q++ = *p;
		}
	}
	*q = '\0';
}


int getProxy(const char *in, char *out,int *piPort)
{
	char *s;
	*piPort = 0;	
	if( s= strstr(in, "http://") )
	{
		pickup(s+7,out,piPort);
	}
	else
	if(s= strstr(in, "http=") )
	{
		pickup(s+5,out,piPort);
	}
	else
	if( s= strstr(in, "http:") )
	{
		pickup(s+5,out,piPort);
	}
	else
	if(s= strstr(in, "://") )
	{
		*out = 0;
		return 0;
	}
	else
	if(s= strstr(in, "=") )
	{
		*out = 0;
		return 0;
	}
	else
		pickup(in,out,piPort);	
	return 1;
}


// Constructor
//
CInternetThread::CInternetThread()
    : m_hSession(NULL),
      m_dwAccessType(PRE_CONFIG_INTERNET_ACCESS)
{
	m_strProxyServer = NULL;
	m_strBuffer = NULL;
	m_strIISServer = NULL;
	m_strPath = NULL;
	m_hICWDllInstance  = NULL ; // ICW DLL not loaded
}




// Closes the Internet session so InternetOpen will be called on next PostData.
//
void CInternetThread::ResetSession()
{
   if (m_hSession != NULL)
   {
      ::InternetCloseHandle(m_hSession);
      m_hSession = NULL ;
   }
}


//
//
CInternetThread::~CInternetThread()
{
   if (m_strBuffer)			delete []m_strBuffer;
   if (m_strIISServer)		delete []m_strIISServer;
   if (m_strProxyServer)	delete []m_strProxyServer;
   if (m_UserName) delete []  m_UserName;
   if (m_Password ) delete [] m_Password;
   if (m_strPath) delete [] m_strPath;

   //ResetSession();
   if(m_hICWDllInstance)
   {
	    FreeLibrary(m_hICWDllInstance);
	    #ifdef _LOG_IN_FILE
			RW_DEBUG << "\n Freeing INETCFG.DLL " << flush;
    	#endif
        m_hICWDllInstance = NULL;
   }
}

void CInternetThread ::UnLoadInetCfgDll()
{
	if( m_hICWDllInstance)
	FreeLibrary(m_hICWDllInstance);
	m_hICWDllInstance = NULL;

}

void	CInternetThread::SetBuffer(LPSTR strBuffer)	
{
   if (m_strBuffer)
		delete []m_strBuffer;
   if (strBuffer)
   {
	   m_dwBufferLen = strlen(strBuffer)+1;
	   m_strBuffer = (LPSTR) new CHAR[strlen(strBuffer)+1];
	   strcpy(m_strBuffer, strBuffer);
   }
}

void	CInternetThread::SetBuffer(LPSTR strBuffer, DWORD dwLen)	
{
   if (m_strBuffer)
		delete []m_strBuffer;
   if (strBuffer)
   {
	   m_dwBufferLen = dwLen-1;
	   m_strBuffer = (LPSTR) new CHAR[m_dwBufferLen+1];
	   memset(m_strBuffer, '\0', m_dwBufferLen+1);
	   strncpy(m_strBuffer, strBuffer, m_dwBufferLen);
   }
}

HINSTANCE CInternetThread:: LoadInetCfgDll()
{
	
	if(m_hICWDllInstance)
	{
		return m_hICWDllInstance;
	}
	m_hICWDllInstance = LoadLibrary(_T("INETCFG.DLL"));
	if (NULL == m_hICWDllInstance)
	{
		DisplayMessage("INETCFG.DLL LoadLibrary Failure", "");
		
	}
	return m_hICWDllInstance;

}

BOOL CInternetThread :: InstallModem(HWND hwnd)
{
	RW_DEBUG << "\n---Inside InstallModem" << flush;
#ifdef _WINNT
	/*
	STARTUPINFOA startUpInfo;
	PROCESS_INFORMATION PrcsInfo;
	GetStartupInfoA(&startUpInfo);
	DWORD dwPrS;
	int iErr;
	if( dwPrS=  CreateProcessA( NULL ,
			"rundll32.exe shell32.dll,Control_RunDLL modem.cpl", 0, 0,
			 FALSE,CREATE_NEW_CONSOLE, 0, 0,
			 &startUpInfo, &PrcsInfo) ) {
			WaitForSingleObject(PrcsInfo.hProcess, INFINITE);
	}
	else
	{
		iErr = GetLastError();
		RW_DEBUG << "\n Error In invoking Modem Init  "  << iErr << flush;
	}
	return 0;*/
	BOOL bRet;
	HINSTANCE hInstance = LoadLibrary(_T("modemui.dll"));
	if(hInstance != NULL)
	{
		lpICP = (LPICP) GetProcAddress(hInstance,"InvokeControlPanel");

		// Initialise the Control panel application
		bRet = lpICP(hwnd,FALSE,FALSE);
	
		FreeLibrary(hInstance);
	}
	else
	{
		bRet = FALSE;
		RW_DEBUG << "\n Error Loading modemui.dll" << flush;
	}
	
	return bRet;
	
#else

	HINSTANCE	hInst;
	BOOL        bRestart=FALSE;
	INETCONFIGSYSTEM fpS;

	hInst = LoadInetCfgDll();
	fpS = (INETCONFIGSYSTEM) GetProcAddress(hInst, "InetConfigSystem");
	HRESULT hrs  = (*fpS)(NULL,0x02,&bRestart);
	DWORD dwR = GetLastError();

	return bRestart;
#endif
}

/*
	05/05/97 : Proxy returns as http://XX:port
*/
BOOL CInternetThread::GetSystemProxyServer(PCHAR szProxy,
										   DWORD dwBufferLength,
										   int *piProxyPort)
{
  DWORD size = 0;  
  ATK_InternetQueryOption(NULL, INTERNET_OPTION_PROXY,NULL, &size);  
  #ifdef USE_ASTRATEK_WRAPPER
		BYTE* buf = new BYTE[size];
  #else
		BYTE* buf = new BYTE[size* sizeof(_TCHAR)];
  #endif
  ATK_INTERNET_PROXY_INFO* ipi = (ATK_INTERNET_PROXY_INFO*)buf;
  if (!ATK_InternetQueryOption(NULL, INTERNET_OPTION_PROXY,   ipi, &size))  
  {
		//MessageBox(NULL,_T("InternetQueryOption"),_T("False"),IDOK);
		delete[] buf;   
		return FALSE;  
  }
  else
  {
	  if(ipi->dwAccessType != 3)
	  {
			//MessageBox(NULL,_T("AccessType"),_T("False"),IDOK);
			delete[] buf;   
			return FALSE;
	  }
	  else
	  {
			//MessageBox(NULL,ConvertToUnicode(),ConvertToUnicode((LPSTR)ipi->lpszProxy),IDOK);
		  #ifdef USE_ASTRATEK_WRAPPER
			if( getProxy(ipi->lpszProxy,szProxy,piProxyPort)) 	
		  #else
			if( getProxy(ConvertToANSIString(ipi->lpszProxy),szProxy,piProxyPort)) 	
		  #endif
			{
				  RemoveBlank(szProxy);
				 // MessageBox(NULL,_T("after getproxy"),_T("TRUE"),IDOK);
					#ifdef _LOG_IN_FILE
						RW_DEBUG <<  "\n Actual HTTP Proxy [" <<szProxy << "] Port:" << *piProxyPort  << flush;
					#endif
				  delete[] buf;   
				  return TRUE;
			}
			else 
			{
				//MessageBox(NULL,_T("GetProxy"),_T("False"),IDOK);
	     	    delete[] buf;   
		    	return FALSE;
			}
	  }
  }
	
}

//
// This function gets the Actual proxy settings string
void CInternetThread :: GetSystemProxySettings( PCHAR szProxy,
										   DWORD dwBufferLength)

{
  DWORD size = 0;  
  ATK_InternetQueryOption(NULL, INTERNET_OPTION_PROXY,NULL, &size);  

  #ifdef USE_ASTRATEK_WRAPPER
		BYTE* buf = new BYTE[size];
  #else
		BYTE* buf = new BYTE[size* sizeof(_TCHAR)];
  #endif

  
  ATK_INTERNET_PROXY_INFO* ipi = (ATK_INTERNET_PROXY_INFO*)buf;
  ATK_InternetQueryOption(NULL, INTERNET_OPTION_PROXY,   ipi, &size);
  
  #ifdef USE_ASTRATEK_WRAPPER
		strcpy(szProxy,ipi->lpszProxy);
  #else
		strcpy(szProxy,ConvertToANSIString(ipi->lpszProxy));
  #endif
  
  dwBufferLength = strlen(szProxy) +1;
  delete[] buf;   
	
}

//
// IN Parameters:
// LPSTR strProxyServer : Proxy Server name in ANSI char
// int iProxyPort       : Proxy Server Port
//
void CInternetThread::SetSystemProxySettings(LPSTR strProxyServer)
{

   TCHAR  *pProxy;

   if (m_strProxyServer)
		delete []m_strProxyServer;
	
   if (strProxyServer)
   {
	   pProxy = ConvertToUnicode(strProxyServer);
	   m_strProxyServer = (LPTSTR) new TCHAR[_tcslen(pProxy)+11];
	   _tcscpy(m_strProxyServer, pProxy);
	
   }

}

// IN Parameters:
// LPSTR strProxyServer : Proxy Server name in ANSI char
// int iProxyPort       : Proxy Server Port
void CInternetThread::SetProxyServer(LPSTR strProxyServer, int iProxyPort)
{
   TCHAR  czTemp[10];
   TCHAR  *pProxy;

   if (m_strProxyServer)
		delete []m_strProxyServer;
	
   if (strProxyServer)
   {
	   pProxy = ConvertToUnicode(strProxyServer);
	   m_strProxyServer = (LPTSTR) new TCHAR[_tcslen(pProxy)+11];
	   _tcscpy(m_strProxyServer, pProxy);
	   _stprintf(czTemp,_T(":%d"),iProxyPort);
	   _tcscat(m_strProxyServer,czTemp);
   }

}

void CInternetThread::SetIISServer(LPTSTR strIISServer)
{
   if (m_strIISServer)
		delete []m_strIISServer;

   if (strIISServer)
   {
	  m_strIISServer = (LPTSTR) new TCHAR[_tcslen(strIISServer)+1];
	  _tcscpy(m_strIISServer, strIISServer);
   }
}

void CInternetThread::SetServerPath(LPTSTR strPath)
{
   if (m_strPath)
		delete []m_strPath;

   if (strPath)
   {
	  m_strPath = (LPTSTR) new TCHAR[_tcslen(strPath)+1];
	  _tcscpy(m_strPath, strPath);
   }
}

// Verify that rAddress is partially valid. Start the worker thread to get a web page.
DWORD CInternetThread::PostData(HWND hWnd)
{

	int iRetryCount;
	UINT uiRetVal;
	int iExit;

	iExit =0;
	iRetryCount = 0;
	uiRetVal = RWZ_POST_FAILURE;

	if (!m_strIISServer || !m_strPath)
	{
      DisplayMessage ("IIS Server path not found ");
    }
	/*
	do {
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nRetry Posting " << iRetryCount+1  << flush;
		#endif
	   uiRetVal = _PostDataWorker();

	   if(uiRetVal != RWZ_POST_SUCCESS) {
		   iRetryCount++;
	   }else {
		   iExit = 1;
	   }
	   if( uiRetVal == RWZ_POST_WITH_SSL_FAILURE) {
		   // Do not Retry if it is an SSL problem
		   iExit= 1;
	   }


	   if(iRetryCount > RWZ_POST_MAX_RETRY ){
		   iExit = 1;
	   }

   }while(!iExit);
   **/
	uiRetVal = _PostDataWorker(hWnd);
   return uiRetVal;
	
}


// This is the thread function.
//
UINT CInternetThread::PostDataThread(LPVOID pvThread)
{
	
   CInternetThread* pInternetThread = (CInternetThread*) pvThread ;
   if (pInternetThread == NULL || (!pInternetThread->m_strIISServer))
   {
      return FALSE;
   }

   return pInternetThread->_PostDataWorker(NULL) ;

}


// This is where all of the actually Internet work is done.
UINT CInternetThread::_PostDataWorker(HWND hWnd)
{
	UINT uiResult = RWZ_POST_FAILURE;
	BOOL bRead;
	HINTERNET hConnect;
	HINTERNET hHttpFile;
	_TCHAR	szHeader[240];
	BOOL bSendRequest;
	DWORD dwCL=0;
	DWORD dwLengthBufQuery ;
	DWORD dwInfoLevel;
	BOOL bQuery;
	CHAR	pBuffer [MAX_PATH] ; // ?? decide with Steve on Bugffer Sz
	DWORD dwBytesRead ;
	// Variables for the SSL / Normal operation
	INTERNET_PORT	nServerPort;
	DWORD			dwFlags;

	// Added for Proxy Server
	LPTSTR  pUserName;
	LPTSTR  pPassword;
	LPTSTR  pProxyServerName;
	pUserName		 =_T("");
	pPassword		 =_T("");
	pProxyServerName =NULL;



	if(m_bPostWithSSL){
		nServerPort	=	INTERNET_DEFAULT_HTTPS_PORT;
		dwFlags		=	INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_SECURE;
	}else {
		nServerPort	=	INTERNET_INVALID_PORT_NUMBER;
		dwFlags		=	INTERNET_FLAG_DONT_CACHE;
	}

	#ifdef _LOG_IN_FILE
				RW_DEBUG << "\nPost Data:\n" << flush;
	#endif

	if (m_hSession == NULL){
      // Initialize the Internet Functions.
		m_hSession = ATK_InternetOpen(_T("Registration Wizard"),
		                              m_dwAccessType,
									  pProxyServerName,
			                          NULL,
					                  0		
									  );

		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\n\tInternet Open:" << m_hSession << flush;
		#endif
		
		if (!Succeeded(m_hSession, "InternetOpen"))
		{
			goto EndFn;
		}
   }

   hConnect = ATK_InternetConnect(m_hSession,
                                          m_strIISServer,
										  nServerPort,
										  pUserName, //	 m_UserName, Changed on 2/4/98 for IE Proxy Auth
										  pPassword, //  m_Password,
                                          INTERNET_SERVICE_HTTP,
                                          0,
                                          0);
										


	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n\tInternet Connection:" << ConvertToANSIString(m_strIISServer) << flush;
	//	RW_DEBUG << "\n\tUserName :" << ConvertToANSIString(m_UserName) << flush;
	//	RW_DEBUG << "\n\tPassword :" << ConvertToANSIString(m_Password) << flush;
	#endif
	
	if (!Succeeded(hConnect, "InternetConnect"))
	{
		goto EndFn;
	}

	hHttpFile = ATK_HttpOpenRequest(hConnect,
                                              _T("POST"),
                                              m_strPath,
                                              HTTP_VERSION,
                                              NULL,
                                              NULL, //szAcceptType,
                                              dwFlags,//INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_SECURE,
                                              0) ;
	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n\t\tInternet Post :" << ConvertToANSIString(m_strPath) << flush;
	#endif
	
	if (! Succeeded(hHttpFile, "HttpOpenRequest"))
	{
		::InternetCloseHandle(hConnect);
		goto EndFn;
	}

	_tcscpy(szHeader, _T("Accept: */*\nContent-Type: application/x-www-form-urlencoded"));
	bSendRequest = ATK_HttpSendRequest(hHttpFile,
	szHeader, -1L , m_strBuffer, m_dwBufferLen);

	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n\t\tSendData:" << m_strBuffer << flush;
	#endif

	if (!Succeeded1(bSendRequest, "HttpSendRequest"))
	{
		::InternetCloseHandle(hConnect);
		goto EndFn;
	}

	// Get size of file.
	dwInfoLevel = HTTP_QUERY_CONTENT_TRANSFER_ENCODING;
	dwLengthBufQuery = sizeof( DWORD);
	bQuery = ATK_HttpQueryInfo(hHttpFile,
                          HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
							&dwCL,
                           &dwLengthBufQuery,
                           NULL) ;
			
	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n Query ContentLength : " << dwCL  << flush;
	#endif
	
	if (! Succeeded1(bQuery, "HttpQueryInfo") )
	{
		::InternetCloseHandle(hConnect);
		goto EndFn;
	}
	memset(pBuffer,0,MAX_PATH);

	bRead = ::InternetReadFile(hHttpFile,	
									pBuffer,	
									MAX_PATH, 	
									&dwBytesRead);	

	#ifdef _LOG_IN_FILE
		RW_DEBUG <<"\n ReturnBuffer " <<  pBuffer << flush;
	#endif
	
	if (!Succeeded1(bRead, "InternetReadFile"))
	{
		DisplayMessage("HTTP POST FAILURE ...");
			::InternetCloseHandle(hConnect);
		goto EndFn;
	}
	
	if(pBuffer[0] == _T('0' ))
	{
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\n Post Successful" << flush;
		#endif

		uiResult = RWZ_POST_SUCCESS;
	}
	else {
		// 438
		if ( (pBuffer[0] == _T('4')) &&  (pBuffer[1] == _T('3')) ) {
			uiResult = RWZ_POST_MSN_SITE_BUSY;
			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\n Post Failure " << flush;
			#endif

		}
		else {
		uiResult = RWZ_POST_WITH_SSL_FAILURE;

		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\n Post with SSL  Failure " << flush;
		#endif
		}
	}

	pBuffer[dwBytesRead] = 0 ;
	::InternetCloseHandle(hConnect);


EndFn:
   return uiResult ;
}

DWORD InvokePost(HWND hWnd, CInternetThread *p)
{
	return p->PostData(hWnd);
}
