/*
	File : ASTRA_RAS.h

*/
//typedef DWORD  (WINAPI *RASGETENTRYPROPERTIES) ( LPTSTR, LPTSTR, LPRASENTRY, LPDWORD, LPBYTE, LPDWORD );
//typedef DWORD  (APIENTRY  *RASGETENTRYPROPERTIES) ( LPTSTR, LPTSTR, LPRASENTRY, LPDWORD, LPBYTE, LPDWORD );
#ifndef __ASTRATEK_RAS_WRAPPER
#define __ASTRATEK_RAS_WRAPPER

#include <windows.h>
#include <tchar.h>
#include <ras.h>
#include <raserror.h>
#include <rasdlg.h>


#define  ERROR_LOADING_RAS_DLL   -1
#define  RAS_DLL_LOADED           1
#define  RAS_DLL_NOT_LOADED        0 

typedef DWORD  (APIENTRY *RASGETENTRYPROPERTIES) ( LPTSTR, LPTSTR, LPRASENTRY, LPDWORD, LPBYTE, LPDWORD );
typedef DWORD  (APIENTRY *RASSETENTRYPROPERTIES) ( LPTSTR, LPTSTR, LPRASENTRY, DWORD, LPBYTE, DWORD );
typedef DWORD  (APIENTRY *RASDELETEENTRY)      ( LPTSTR, LPTSTR );

typedef DWORD (APIENTRY *RASGETERRORSTRING) (UINT, LPTSTR, DWORD );
typedef DWORD (APIENTRY *RASDIAL)  ( LPRASDIALEXTENSIONS, LPTSTR, LPRASDIALPARAMS, DWORD,
                   LPVOID, LPHRASCONN );

typedef DWORD (APIENTRY* RASHANGUP) ( HRASCONN );
typedef DWORD (APIENTRY* RASENUMDEVICES) ( LPRASDEVINFO, LPDWORD, LPDWORD );
typedef DWORD (APIENTRY* RASENUMCONNECTIONS) (LPRASCONN , LPDWORD, LPDWORD);
typedef DWORD (APIENTRY* RASGETCOUNTRYINFO)( LPRASCTRYINFO, LPDWORD );
typedef DWORD (APIENTRY* RASGETCONNECTIONSTATUS)( HRASCONN, LPRASCONNSTATUS );


int ATK_IsRasDllOk();

DWORD ATK_RasDial( LPRASDIALEXTENSIONS lpRasDial, 
			 LPTSTR  lpPhBk, 
			 LPRASDIALPARAMS lpDialParam,
			 DWORD dwNotifyType,
			 LPVOID lpNotifier,
			 LPHRASCONN lphRasConn);

DWORD ATK_RasHangUp ( HRASCONN hrasconn );

DWORD ATK_RasGetEntryProperties(
		LPTSTR lpszPhonebook, 
		LPTSTR lpszEntry, 
		LPRASENTRY lpRasEntry, 
		LPDWORD lpdwEntryInfoSize, 
		LPBYTE lpbDeviceInfo, 
		LPDWORD lpdwDeviceInfoSize );

DWORD ATK_RasSetEntryProperties(
		LPTSTR lpszPhonebook, 
		LPTSTR lpszEntry, 
		LPRASENTRY lpRasEntry, 
		DWORD dwEntryInfoSize, 
		LPBYTE lpbDeviceInfo, 
		DWORD dwDeviceInfoSize );

DWORD ATK_RasDeleteEntry( LPTSTR lpszPhonebook,
					   LPTSTR lpszEntry);

DWORD ATK_RasGetErrorString( UINT uErrorValue, 
							 LPTSTR lpszErrorString, 
							 DWORD cBufSize );

DWORD ATK_RasEnumDevices( LPRASDEVINFO lpRasDevInfo, 
						  LPDWORD lpcb, 
						  LPDWORD lpcDevices);
DWORD ATK_RasEnumConnections ( LPRASCONN lprasconn, 
							   LPDWORD lpcb, 
							   LPDWORD lpcConnections);
DWORD ATK_RasGetCountryInfo( LPRASCTRYINFO lpRasCtryInfo, 
					LPDWORD lpdwSize );

DWORD ATK_RasGetCountryInfoA( LPRASCTRYINFO lpRasCtryInfo, 
					LPDWORD lpdwSize );
DWORD ATK_RasGetConnectionStatus( HRASCONN hrasconn,
		LPRASCONNSTATUS lprasconnstatus );
#endif