//  File	 : ATK_INET.CPP
//  Author   : Suresh Krishnan 
//  Date     : 08/05/97
//  Wrapper for INetCFG.DLL exported functions
//  related  function declarations
//
//

#include <windows.h>
#include <tchar.h>
#include <winnt.h>
#include <wininet.h>
#include <stdio.h>
#include "rw_common.h"






typedef HRESULT (WINAPI *INETGETPROXY)(LPBOOL lpfEnable, LPSTR lpszServer, DWORD cbServer, LPSTR lpszOverride, DWORD cbOverride);
typedef HRESULT (WINAPI *INETCONFIGSYSTEM) ( HWND hWndParent, DWORD dwfOptions, LPBOOL lpfNeedsRestart);
typedef HRESULT (WINAPI *INETGETAUTODIAL) (LPBOOL lpEnable, LPSTR lpszEntryName, DWORD cbEntryName);
typedef HRESULT (WINAPI *INETSETAUTODIAL) (BOOL fEnable, LPCSTR lpszEntryName);

#define ERROR_IN_SET       -1
#define INET_DLL_FAILURE   -1

static HINSTANCE  hINetCfg= NULL;
static INETGETPROXY	    fpGetProxy=NULL;
static INETCONFIGSYSTEM fpGetConfigSystem=NULL;
static INETGETAUTODIAL  fpGetAutoDial=NULL;
static INETSETAUTODIAL  fpSetAutoDial=NULL;
static INETCONFIGSYSTEM fpInetConfig =NULL;  

typedef struct {
	char szActiveConnection[256];
	BOOL fStatus;
    enum ISPStateFlag {NotValidState,ValidState } iState ;
} ISPState; 

static ISPState  gIspState = { "",
						0,
						ISPState::NotValidState};




int INetCfgSetup()
{
	if(hINetCfg) {
		return 1;
	}
	hINetCfg = LoadLibrary( _T("INETCFG.DLL") );  //  Load INetCfg.DLL and store globally
	if( !hINetCfg )
	{                                   
	  //  return if the DLL can not loaded
	  //
	  return 0;
	}
	


	fpGetProxy = (INETGETPROXY) GetProcAddress(hINetCfg,"InetGetProxy");
	fpGetAutoDial = ( INETGETAUTODIAL) GetProcAddress(hINetCfg, "InetGetAutodial");
	fpSetAutoDial = ( INETSETAUTODIAL) GetProcAddress(hINetCfg, "InetSetAutodial");
	fpInetConfig = (INETCONFIGSYSTEM) GetProcAddress(hINetCfg, "InetConfigSystem");
	return 1;
	
}

HRESULT ATK_InetGetAutoDial(LPBOOL lpEnable, LPSTR lpszEntryName, DWORD cbEntryName)
{
	if(INetCfgSetup()) {
	return (*fpGetAutoDial)(lpEnable, lpszEntryName, cbEntryName);
	}else {
		return INET_DLL_FAILURE;
	}
}

HRESULT ATK_InetSetAutoDial(BOOL fEnable, LPCSTR lpszEntryName)
{
	if(INetCfgSetup()) {
		return (*fpSetAutoDial)(fEnable, lpszEntryName);
	}else{
		return INET_DLL_FAILURE;
	}
}

HRESULT ATK_InetConfigSystem( HWND hwndParent,
							 DWORD dwfOptions,
							 LPBOOL lpfNeedsRestart)
{
	if(INetCfgSetup()) {
		return (*fpInetConfig)( hwndParent,dwfOptions, 
			lpfNeedsRestart);
	}else{
		return INET_DLL_FAILURE;
	}

}

HRESULT ATK_InetGetProxy( LPBOOL lpfEnable,
						  LPSTR  lpszServer,
						  DWORD  cbServer,
						  LPSTR  lpszOverride,
						  DWORD  cbOverride)
{
	if(INetCfgSetup()) {
		return (*fpGetProxy)( lpfEnable,
						  lpszServer,
						  cbServer,
						  lpszOverride,
						  cbOverride);
	}else{
		return INET_DLL_FAILURE;
	}


}



void GetAutoDialConfiguration()
{

	DWORD dwError;
	DWORD dwSz=256;
	HRESULT  hr;
	ISPState *pS= &gIspState; 
	hr = ATK_InetGetAutoDial(&pS->fStatus,
		pS->szActiveConnection,
		dwSz);
	if(hr) {
		dwError = GetLastError();

		RW_DEBUG << "\nGet AutoDial :***Error " <<hr  << ":"<<  dwError << flush;
		pS->iState = ISPState::NotValidState;
	}else {
		pS->iState = ISPState::ValidState;
	}
	
}

DWORD SetAutoDialStateThread(void *vp)
{
	ISPState  *pState;
	pState = (ISPState *) vp;
	ATK_InetSetAutoDial(pState->fStatus,
		pState->szActiveConnection);
	RW_DEBUG <<"\nSet Auto Dial Configuration" << pState->szActiveConnection << " =>" << pState->fStatus << flush;
	ExitThread(0);
	return 0;

}

//
//  This function calls the ICW function InetSetAutoDial()
//  this function waits for the above function to be over by 10 seconds 
//  if it does not complete then it calls terminate thread and abondens the operation 
int ChangeInterNetAutoDial(ISPState *pStatus )
{
	int iReturn;
	DWORD dwTimeOut = 10*1000;
	DWORD dwCreationFlags=0; // Start without  CREATE_SUSPENDED 
	DWORD ThreadId;
	
	iReturn = NO_ERROR;

	HANDLE hParent = CreateThread(NULL, 
	0,
	(LPTHREAD_START_ROUTINE) SetAutoDialStateThread,
	(void *) pStatus,
	dwCreationFlags, 
	&ThreadId );
	iReturn = NO_ERROR;

	DWORD dwRet = WaitForSingleObject(hParent,
		dwTimeOut);
	switch(dwRet) {
	case WAIT_ABANDONED :
		break;
	case WAIT_OBJECT_0 :
		CloseHandle(hParent);
		break;
	case WAIT_TIMEOUT :
		//TerminateThread(hParent,0);
		iReturn = ERROR_IN_SET;
		break;
	default:
		break;
	}
	return iReturn;
	

}


int ResetAutoDialConfiguration()
{
	int iRet;
	iRet = NO_ERROR;
	if(gIspState.iState == ISPState::NotValidState ){
		//
		// Not alid So No need to Reset 
		return iRet;
	}
	return ChangeInterNetAutoDial(&gIspState);
}

int DisableAutoDial()
{
	ISPState  IspState = { "",
						0,
						ISPState::NotValidState};
	
	return ChangeInterNetAutoDial(&IspState);
}

void UnLoadInetCfgLib()
{
	if(hINetCfg){
		FreeLibrary(hINetCfg);  //  Load INetCfg.DLL and store globally
		hINetCfg = NULL;

	}

}