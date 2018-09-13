///////////////////////////////
//   File:          tcp.cpp
//   
//   Description:   

//	#include statements
//
#include <windows.h>
#include <stdio.h>
#include <winsock.h>
#include "tcp.h"
#include "util.h"
#include "rw_common.h"


#define   RW_TCP_NOT_INITILIZED   0
#define   RW_TCP_INITILIZED       1 

#define   RW_ICMP_NOT_INITILIZED   0
#define   RW_ICMP_INITILIZED       1

static int siTcpStatus = RW_TCP_NOT_INITILIZED;
static HINSTANCE  hIcmp= NULL;
static WSADATA wsa;

int ResolveHostByThread(LPSTR pHost);
int ResolveHostByAddrThread(LPSTR pHost);

#define GET_HOST_TOUT (15 * 1000)
#define PING_TOUT     (15 * 1000)
static struct hostent *gphostent= NULL;

BOOL InitForTcp()
{
	BOOL	bRet= FALSE;


	if( siTcpStatus == RW_TCP_INITILIZED ) 
	return TRUE;

	if (! WSAStartup(0x0101, &wsa )) {
		siTcpStatus = RW_TCP_INITILIZED;
		bRet= TRUE;
	}
	return bRet;

}

BOOL InitForIcmp()
{
	if(hIcmp)
		return TRUE;
	hIcmp = LoadLibrary( _T("ICMP.DLL") );  //  Load ICMP.DLL and store globally
	if( ! hIcmp )
	{                                   //  Whine if unable to load the DLL
	  DisplayMessage("Unable to locate ICMP.DLL");
	  return( FALSE );
	}
	return TRUE;

}

void CloseForTcpIcmp()
{
	if (hIcmp)
		FreeLibrary(hIcmp);  //  When app is closed, free the ICMP DLL
	if(siTcpStatus == RW_TCP_INITILIZED)
	WSACleanup();		  //  And clean up sockets
	hIcmp = NULL;
	siTcpStatus = RW_TCP_NOT_INITILIZED;

}
//
// Tries to get the host name and pings using ICMP
// returns
//  RWZ_PINGSTATUS_NOTCPIP : if no socket library or get hostname fails  
//  RWZ_PINGSTATUS_SUCCESS : if gethostname and ping is successful 
//  RWZ_PINGSTATUS_FAIL    : if gethostname is succesful and ping via icmp fails 

DWORD  PingHost()
{
	DWORD 	dwRet= 0;
	char	szIPAddress[80];
	dwRet = RWZ_PINGSTATUS_NOTCPIP;
	if(!InitForTcp()) {
		return dwRet; // Tcp is not installed 
	}
	memset(szIPAddress, '\0', 80);
	if (!gethostname(szIPAddress, 80))
	{
		 
		if (Ping(szIPAddress)){
			dwRet =  RWZ_PINGSTATUS_SUCCESS;
		}else {
			dwRet =  RWZ_PINGSTATUS_FAIL;
		}
	}

	return dwRet;
}


	



BOOL Ping(LPSTR szIPAddress)
{
	BOOL bRet= FALSE;

	if( !InitForIcmp())
		return bRet;

	if(!InitForTcp()) {
		return FALSE; // Tcp is not installed 
	}


	static struct sockaddr_in saDestAddr;

	char szBuffer[64];
	DWORD *dwIPAddr, dwStatus;
	HANDLE hIP;
	struct hostent *phostent;
	PIP_ECHO_REPLY pIpe;

	if(!ResolveHostByThread(szIPAddress)) {
		gphostent = gethostbyname(szIPAddress);
		phostent = gphostent;
	}else {
		phostent= NULL;
	}
	if( ! phostent ){
		RW_DEBUG << "\n Resolving by Address "  << flush;
		int iError;
		iError = 0;
		iError = WSAGetLastError ();
		RW_DEBUG << "\n Get Host By Name Error " << iError  << flush;
		if(iError){
			WSASetLastError (0);
			//return 0;
		}

		saDestAddr.sin_addr.s_addr = inet_addr(szIPAddress);
		if( saDestAddr.sin_addr.s_addr !=INADDR_NONE ) {
			if(!ResolveHostByAddrThread((LPSTR)&saDestAddr.sin_addr.s_addr)) {
				gphostent = gethostbyaddr((LPSTR)&saDestAddr.sin_addr.s_addr,4, PF_INET) ;
				phostent = gphostent;
			}else {
				phostent= NULL;
			}
			
		}
		if(!phostent)
		{
			DisplayMessage(szIPAddress , "Unable to obtain an IP address for %s");
			return bRet;
		}
		
	}
	



    dwIPAddr = (DWORD *)( *phostent->h_addr_list );

	ICMPCREATEFILE	pIcmpCreateFile;
	pIcmpCreateFile = (ICMPCREATEFILE) GetProcAddress(hIcmp, "IcmpCreateFile");
	if (NULL == pIcmpCreateFile)
	{
		DisplayMessage("IcmpCreateFile GetProc Error", "");
		return FALSE;
	}

    ICMPCLOSEHANDLE	pIcmpCloseHandle;
 	pIcmpCloseHandle = (ICMPCLOSEHANDLE) GetProcAddress(hIcmp, "IcmpCloseHandle");
	if (NULL == pIcmpCloseHandle)
	{
		DisplayMessage("IcmpCloseHandle GetProc Error", "");
		return bRet;
	}

	ICMPSENDECHO	pIcmpSendEcho;
	pIcmpSendEcho = (ICMPSENDECHO) GetProcAddress(hIcmp, "IcmpSendEcho");
	if (NULL == pIcmpSendEcho)
	{
		DisplayMessage("IcmpSendEcho GetProc Error", "");
		return bRet;
	}

	if( ! pIcmpCreateFile || ! pIcmpCloseHandle || ! pIcmpSendEcho )
	{
		DisplayMessage("Unable to locate required API functions", "");
		return bRet;
	}


	hIP = pIcmpCreateFile();
	if( hIP == INVALID_HANDLE_VALUE )
	{
	  DisplayMessage("Unable to open PING service");
	  return bRet;
	}

	memset( szBuffer, '\xAA', 64 );
	pIpe = (PIP_ECHO_REPLY)LocalAlloc(LPTR, sizeof(IP_ECHO_REPLY) + 64);
	if (pIpe)
	{
		pIpe->Data = szIPAddress;
		pIpe->DataSize = 64;      

		dwStatus = pIcmpSendEcho( hIP, *dwIPAddr, szBuffer, 64, NULL, pIpe, 
								sizeof(IP_ECHO_REPLY) + 64, PING_TOUT );
		if(dwStatus)
		{
			bRet = TRUE;
		}
		LocalFree(pIpe);
		pIcmpCloseHandle(hIP);
   }
   
   
   return bRet;
}


BOOL  CheckHostName(LPSTR szIISServer)
{
// WSAStartup() is already called	
	if(!InitForTcp()) {
		return FALSE; // Tcp is not installed 
	}
	struct hostent *phostent;
	
	if(!ResolveHostByThread(szIISServer)) {
		phostent = gphostent;
	}else {
		phostent= NULL;
	}
	
	if (phostent == NULL)
		return FALSE;
	else
		return TRUE;
// WSACleanup() will be called later
	
}


//
//  Returns 1 if there is an Error
//          0 if Successful 
DWORD GetHostThread(void *vp)
{
	DWORD dwIsError=1;
	LPSTR  szHost;
	szHost = (LPSTR) vp;
	int iError = 0;

	gphostent = gethostbyname(szHost);
	if( ! gphostent ){
		
		iError = WSAGetLastError ();
		if(iError) {
			WSASetLastError (0); // Reset the  error 
		}
	}
	else {
		dwIsError =0;
	}
	ExitThread(dwIsError);
	return dwIsError;
}

//
//  This function returns Calls the gethostbyname and 
//  returns 0 if Successful and 1 if failure 
//  the return 
//
//
int ResolveHostByThread(LPSTR pHost)
{
	int   iRet=0; 
	DWORD  dwThreadExitCode; 
	DWORD dwCreationFlags=0; // Start CREATE_SUSPENDED 
	DWORD ThreadId;
	RW_DEBUG << "\nResolve " << pHost <<  flush;

	HANDLE hParent = CreateThread(NULL, 
		0,
	(LPTHREAD_START_ROUTINE) GetHostThread,
	(void *) pHost,
	dwCreationFlags, 
	&ThreadId );

	DWORD dwRet = WaitForSingleObject(hParent,GET_HOST_TOUT);
	switch(dwRet) {
	case WAIT_ABANDONED :
		iRet = 1; // Error In Get Host Name 
		break;
	case WAIT_OBJECT_0 :
		RW_DEBUG << "\n\tResolved ( 1 Error, 0 Ok)  ";

		if( GetExitCodeThread(hParent,&dwThreadExitCode) ) {
			iRet = (int) dwThreadExitCode;

		}
		else {
			
		}
		RW_DEBUG << iRet;
		break;
	case WAIT_TIMEOUT :
		RW_DEBUG << "\n\t*** Error  Resolving " << flush;
		iRet = 1; // Error In Get Host Name 
		TerminateThread(hParent,0);
		break;
	default:
		break;
	}
	return iRet;


}


//
//  Returns 1 if there is an Error
//          0 if Successful 
DWORD GetHostByAddrThread(void *vp)
{
	DWORD dwIsError=1;
	LPSTR  szAddr;
	int iError = 0;
	szAddr = (LPSTR) vp;
	
	gphostent = gethostbyaddr(szAddr, 
		4, PF_INET) ;
	if( ! gphostent ){
		
		iError = WSAGetLastError ();
		if(iError) {
			WSASetLastError (0); // Reset the  error 
		}
	}
	else {
		dwIsError =0;
	}
	return dwIsError;
}

//
//  This function returns Calls the gethostbyaddr and 
//  returns 0 if Successful and 1 if failure 
//  the return 
//
//
int ResolveHostByAddrThread(LPSTR pHost)
{
	int   iRet=0; 
	DWORD  dwThreadExitCode; 
	DWORD dwCreationFlags=0; // Start CREATE_SUSPENDED 
	DWORD ThreadId;
	RW_DEBUG << "\nResolve " << pHost << " By Address " << flush;
	HANDLE hParent = CreateThread(NULL, 
		0,
	(LPTHREAD_START_ROUTINE) GetHostByAddrThread,
	(void *) pHost,
	dwCreationFlags, 
	&ThreadId );

	DWORD dwRet = WaitForSingleObject(hParent,GET_HOST_TOUT);
	switch(dwRet) {
	case WAIT_ABANDONED :
		iRet = 1; // Error In Get Host Name 
		break;
	case WAIT_OBJECT_0 :
		RW_DEBUG << "\n\tResolved ( 1 Error, 0 Ok)  ";
		if( GetExitCodeThread(hParent,&dwThreadExitCode) ) {
			iRet = (int) dwThreadExitCode;
		
		}
		else {
			
		}
		RW_DEBUG << iRet << flush;
		break;
	case WAIT_TIMEOUT :
		RW_DEBUG << "\n\t*** Error  Resolving " << flush;
		iRet = 1; // Error In Get Host Name 
		TerminateThread(hParent,0);
		break;
	default:
		break;
	}
	return iRet;


}
