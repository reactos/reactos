/*********************************************************************
Registration Wizard
cntryinf.h

10/12/94 - Tracy Ferrier
(c) 1994-95 Microsoft Corporation
*********************************************************************/
//#define TAPI_CURRENT_VERSION 0x00010004
#ifndef __CNTRYINF__
#define __CNTRYINF__


#include <tchar.h>
#include <tapi.h>
#define kCountryCodeUnitedStates 1

BOOL CountryCodeFromSzCountryCode(HINSTANCE hInstance,LPTSTR szCountry,DWORD* lpCountry);
DWORD GetCountryCodeUsingTapiId(DWORD dwCountryId, DWORD *dwCountryCode) ;
BOOL GetTapiCurrentCountry(HINSTANCE hInstance,DWORD* dwpCountry);
BOOL FFillCountryList(HINSTANCE hInstance,HWND hwndCB,LPTSTR szCountry,DWORD* lpCountry);


class CCntryInfo {
public :
#ifdef _TAPI
	LINECOUNTRYLIST  *m_pCountry;
#endif
	CCntryInfo();
	~CCntryInfo();
	int  GetCountryCode(_TCHAR *czCountryName);
	int  GetCountryCode( DWORD  dwTapiId);
	_TCHAR * GetCountryName(int iCode =0);
	void FillCountryList(HINSTANCE hInstance,HWND hwndCB);
	int  GetTapiCountryCode(_TCHAR * czCountryName);
	int  GetTapiIDForTheCountryIndex(int iCntryIndex=0); // Useful to Get the Actual TAPI Country Index 

};

extern CCntryInfo     gTapiCountryTable;

#endif //__CNTRYINF__
