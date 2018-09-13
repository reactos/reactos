#ifndef _RWPOST_H_
#define	_RWPOST_H_

/**************************************************************************
   File:          RWPOST.h
   Description:   
**************************************************************************/
#define MAX_BUFFER	5*1024

#ifdef __cplusplus
extern "C" 
{
#endif
DWORD CheckInternetConnectivityExists( HWND hWnd, HINSTANCE hInstance);
DWORD_PTR CheckWithDisplayInternetConnectivityExists(HINSTANCE hIns,HWND hwnd,int iMsgType=0);
//DWORD SendHTTPData(HINSTANCE hInstance,LPTSTR czB, DWORD *dwBufSize);
DWORD SendHTTPData(HWND hWnd, HINSTANCE hInstance);
DWORD PostHTTPData(HINSTANCE hInstance);
DWORD_PTR PostDataWithWindowMessage( HINSTANCE hIns);
void  InitializeInetThread(HINSTANCE hInstance);

#ifdef __cplusplus
}
#endif

#define  RWZ_SITE_CONNECTED   1
#define  RWZ_FAIL_TOCONNECTTOSITE  0
#define  RWZ_SITE_REQUIRES_AUTHENTICATION 2

DWORD  ChkSiteAvailability( HWND hWnd,
			   LPCTSTR szIISServer,
			   DWORD   dwTimeOut,
			   LPTSTR szProxyServer,
               LPTSTR	szUserName ,
   			   LPTSTR  szPassword);

extern RECT gRect;


#endif	//_INTERFACE_H_
