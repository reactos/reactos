/*
	File : ASTRA_RAS.h

*/
#include <ATK_RAS.h>



static int siRasDllLoaded = RAS_DLL_NOT_LOADED ;
static HINSTANCE	hRasDllInst=NULL;

static RASGETENTRYPROPERTIES  pRasGetEntryProperties;
static RASSETENTRYPROPERTIES  pRasSetEntryProperties;
static RASDELETEENTRY         pRasDelEntry;
static RASGETERRORSTRING      pRasGetErrorString;
static RASDIAL                pRasDial;
static RASHANGUP              pRasHangup;
static RASENUMDEVICES         pRasEnumDevices;
static RASENUMCONNECTIONS     pRasEnumConnections;
static RASGETCOUNTRYINFO      pRasGetCountryInfo; 
static RASGETCOUNTRYINFO      pRasGetCountryInfoA;
static RASGETCONNECTIONSTATUS pRasGetConnectionStatus;  



int ATK_IsRasDllOk()
{
	if(siRasDllLoaded == RAS_DLL_NOT_LOADED  ) {
		hRasDllInst = LoadLibrary(_T("RASAPI32.DLL"));
		if(hRasDllInst == NULL) {
			siRasDllLoaded = ERROR_LOADING_RAS_DLL;

		}else {
			//
			//
			// Get All Function Pointers;
#ifdef _UNICODE
			pRasGetEntryProperties  = (RASGETENTRYPROPERTIES) 
				GetProcAddress(hRasDllInst, "RasGetEntryPropertiesW");
#else
			pRasGetEntryProperties  = (RASGETENTRYPROPERTIES) 
				GetProcAddress(hRasDllInst, "RasGetEntryPropertiesA");

#endif

#ifdef _UNICODE
			pRasSetEntryProperties=  (RASSETENTRYPROPERTIES)
				GetProcAddress(hRasDllInst, "RasSetEntryPropertiesW");
#else
			pRasSetEntryProperties=  (RASSETENTRYPROPERTIES)
				GetProcAddress(hRasDllInst, "RasSetEntryPropertiesA");
#endif

#ifdef _UNICODE
			pRasDelEntry = (RASDELETEENTRY) 
				GetProcAddress(hRasDllInst, "RasDeleteEntryW");
#else
			pRasDelEntry = (RASDELETEENTRY) 
				GetProcAddress(hRasDllInst, "RasDeleteEntryA");
#endif

#ifdef _UNICODE
			pRasGetErrorString = (RASGETERRORSTRING)
			GetProcAddress(hRasDllInst, "RasGetErrorStringW");
#else
			pRasGetErrorString = (RASGETERRORSTRING)
			GetProcAddress(hRasDllInst, "RasGetErrorStringA");
#endif


#ifdef _UNICODE
			pRasDial = (RASDIAL)
			GetProcAddress(hRasDllInst, "RasDialW");
#else
			pRasDial = (RASDIAL)
			GetProcAddress(hRasDllInst, "RasDialA");
#endif


#ifdef _UNICODE
			pRasHangup = (RASHANGUP)
			GetProcAddress(hRasDllInst, "RasHangUpW");
#else
			pRasHangup = (RASHANGUP)
			GetProcAddress(hRasDllInst, "RasHangUpA");
#endif


#ifdef _UNICODE
			pRasEnumDevices = (RASENUMDEVICES )
					GetProcAddress(hRasDllInst, "RasEnumDevicesW");
#else
				pRasEnumDevices = (RASENUMDEVICES )
					GetProcAddress(hRasDllInst, "RasEnumDevicesA");
#endif

#ifdef _UNICODE
			pRasEnumConnections = (RASENUMCONNECTIONS)
					GetProcAddress(hRasDllInst, "RasEnumConnectionsW");
#else
			pRasEnumConnections = (RASENUMCONNECTIONS)
					GetProcAddress(hRasDllInst, "RasEnumConnectionsA");
#endif

#ifdef _UNICODE
			pRasGetCountryInfo = (RASGETCOUNTRYINFO)
					GetProcAddress(hRasDllInst, "RasGetCountryInfoW");
#else
			pRasGetCountryInfo = (RASGETCOUNTRYINFO)
					GetProcAddress(hRasDllInst, "RasGetCountryInfoA");
#endif
		
			pRasGetCountryInfoA = (RASGETCOUNTRYINFO)
					GetProcAddress(hRasDllInst, "RasGetCountryInfoA");

#ifdef _UNICODE
			pRasGetConnectionStatus  = (RASGETCONNECTIONSTATUS) 
				GetProcAddress(hRasDllInst, "RasGetConnectionStatusW");
#else
			pRasGetConnectionStatus  = (RASGETCONNECTIONSTATUS) 
				GetProcAddress(hRasDllInst, "RasGetConnectionStatusA");
#endif


				
			siRasDllLoaded = RAS_DLL_LOADED;
  		}


	}
	return siRasDllLoaded;
	 
}


DWORD ATK_RasGetEntryProperties(
		LPTSTR lpszPhonebook, 
		LPTSTR lpszEntry, 
		LPRASENTRY lpRasEntry, 
		LPDWORD lpdwEntryInfoSize, 
		LPBYTE lpbDeviceInfo, 
		LPDWORD lpdwDeviceInfoSize )
{
	return (*pRasGetEntryProperties) ( lpszPhonebook, 
				 lpszEntry, 
				 lpRasEntry, 
			  	 lpdwEntryInfoSize, 
				 lpbDeviceInfo, 
				 lpdwDeviceInfoSize);

}


DWORD ATK_RasSetEntryProperties(
		LPTSTR lpszPhonebook, 
		LPTSTR lpszEntry, 
		LPRASENTRY lpRasEntry, 
		DWORD dwEntryInfoSize, 
		LPBYTE lpbDeviceInfo, 
		DWORD dwDeviceInfoSize )
{
	return (*pRasSetEntryProperties) ( 
				 lpszPhonebook, 
				 lpszEntry, 
				 lpRasEntry, 
			  	 dwEntryInfoSize, 
				 lpbDeviceInfo, 
				 dwDeviceInfoSize);


}

DWORD ATK_RasDeleteEntry( LPTSTR lpszPhonebook,
					   LPTSTR lpszEntry)
{
	return (*pRasDelEntry)(lpszPhonebook,lpszEntry);
}

DWORD ATK_RasGetErrorString( UINT uErrorValue, 
							 LPTSTR lpszErrorString, 
							 DWORD cBufSize )
{
	return (*pRasGetErrorString)(uErrorValue,lpszErrorString,cBufSize);

}

DWORD ATK_RasHangUp( HRASCONN hrasconn )
{
	return (*pRasHangup)(hrasconn);
}              


DWORD ATK_RasDial( LPRASDIALEXTENSIONS lpRasDial, 
			 LPTSTR  lpPhBk, 
			 LPRASDIALPARAMS lpDialParam,
			 DWORD dwNotifyType,
			 LPVOID lpNotifier,
			 LPHRASCONN lphRasConn)
{
	return (*pRasDial) ( lpRasDial, 
			 lpPhBk, 
			 lpDialParam,
			 dwNotifyType,
			 lpNotifier,
			 lphRasConn);

}



DWORD ATK_RasEnumDevices( LPRASDEVINFO lpRasDevInfo, 
						  LPDWORD lpcb, 
						  LPDWORD lpcDevices)

{
	return (*pRasEnumDevices)(lpRasDevInfo, 
		lpcb, 
		lpcDevices);

}

DWORD ATK_RasEnumConnections ( LPRASCONN lprasconn, 
							  LPDWORD lpcb, 
							  LPDWORD lpcConnections)
{
	return (*pRasEnumConnections) (lprasconn,lpcb, 
							  lpcConnections);
}

#define  MAX_ACTIVE_RAS_CONNECTION 5
/*

	Returns +VE value if there is an active connection

*/
int IsDialupConnectionActive()
{
	RASCONN RasConn[MAX_ACTIVE_RAS_CONNECTION];
	DWORD   dwSizeConn,dwConnections,dwRet;
	int		iCount;
	dwConnections = 0;
	for(iCount=0;iCount < MAX_ACTIVE_RAS_CONNECTION ;iCount++) {
		RasConn[iCount].dwSize= sizeof(RASCONN);
	}

	dwSizeConn = sizeof(RASCONN) * MAX_ACTIVE_RAS_CONNECTION;
	dwRet = ATK_RasEnumConnections ( RasConn, 
							 &dwSizeConn, 
							 &dwConnections);
	switch(dwRet) 
	{
	case ERROR_BUFFER_TOO_SMALL :
		;
		//cout << "\n Buffer Too Small "<< flush;
	case ERROR_NOT_ENOUGH_MEMORY :
		;
		//cout << "\n Not enough memory  "<< flush;
	default :
		break;
	} 

	if(dwConnections) {
		return ((int)dwConnections);
	}else {
		return 0;
	}


}

DWORD ATK_RasGetCountryInfo( LPRASCTRYINFO lpRasCtryInfo, 
					LPDWORD lpdwSize )
{
 return (*pRasGetCountryInfo) (lpRasCtryInfo,lpdwSize); 
				  
 
} 
 
DWORD ATK_RasGetCountryInfoA( LPRASCTRYINFO lpRasCtryInfo, 
					LPDWORD lpdwSize )
{
 return (*pRasGetCountryInfoA) (lpRasCtryInfo,lpdwSize); 
} 

DWORD ATK_RasGetConnectionStatus( HRASCONN hrasconn,LPRASCONNSTATUS lprasconnstatus )
{
 return (*pRasGetConnectionStatus) (hrasconn,lprasconnstatus); 
} 
