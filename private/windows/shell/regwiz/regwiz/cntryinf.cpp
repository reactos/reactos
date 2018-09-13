/*********************************************************************
Registration Wizard

CNTRYINF.CPP
This file houses a set of function that use TAPI to access country
code/name information.

11/15/94 - Tracy Ferrier
05/08/97 - Suresh Krishnan
	Modified Country inforamtion retrival  using classes
	The class CCntryInfo  will get  the information of the country list using
	TAPI API.
	And it has methiods GetCountryCode() and GetCountryName() to Accessit.
	Also A Combo List can be generated using the
2/3/98   - Suresh Krishnan
	Added GetCountryCodeUsingTapiId() ;
	This uses RAS API to get the country ID
	
(c) 1994-95 Microsoft Corporation
**********************************************************************/

#include <Windows.h>
#include <stdio.h>
#include <rw_common.h>
#include "cntryinf.h"
#include <ATK_RAS.H>

//#define COMPILE_USING_VC   Enable this if U are compiling for UNICODE using VISULA c++ 5.0 compiler


static DWORD dwAPILowVersion = 0 << 16;
static DWORD dwAPIHighVersion = 3 << 16;

BOOL FGetLocationEntry(HLINEAPP hLineApp, DWORD dwAPI,LINELOCATIONENTRY *pLE);
BOOL FGetLineCountryList(LINECOUNTRYLIST **ppcl);
void CALLBACK CountryLineCallback(DWORD hDevice, DWORD dwMessage, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2, DWORD dwParam3);
BOOL PrepareCountryListFromTapi ( HINSTANCE hInstance,
								  LINECOUNTRYLIST	**pcl);

CCntryInfo  gTapiCountryTable; // Gloabal variable for buildind and maintaininf TAPI cntry info.

#define INVALID_PORTID (DWORD) -1
#ifndef _TAPI
#define _TAPI
#endif
BOOL GetTapiCurrentCountry(HINSTANCE hInstance,DWORD* dwpCountry)
/*********************************************************************
Returns the current code of the user's location, as determined by
Tapi.
**********************************************************************/
{
	#ifdef _TAPI
	DWORD				dwAPI;
	LINELOCATIONENTRY	LE;
	BOOL				fDefCountry;
	HLINEAPP 			hLineApp;
	DWORD 				numDevs;
	LONG tapiStatus = lineInitialize(&hLineApp, hInstance,
		(LINECALLBACK) CountryLineCallback, NULL, &numDevs);
	if (tapiStatus != 0)
	{
		char szMessage[256];
		sprintf(szMessage,"lineInitialize call failed: error = %li",tapiStatus);
		RW_DEBUG << szMessage << "\n"<< flush;
		//MessageBox(NULL,szMessage,_T("TAPI STATUS"),MB_OK);
		return FALSE;
	}
#ifdef SURESH
	DWORD dwAPILowVersion = 1 << 16;
	DWORD dwAPIHighVersion = 4 << 16;
#endif
	
	LINEEXTENSIONID extensionID;
	tapiStatus = lineNegotiateAPIVersion(hLineApp,0,dwAPILowVersion, dwAPIHighVersion,&dwAPI,&extensionID);

//	RW_DEBUG << "Api Version : " << dwAPI << flush;

	 //this gets the currently selected TAPI country
	//fDefCountry = FGetLocationEntry(hLineApp, dwAPI, &LE);
	fDefCountry = FGetLocationEntry(hLineApp, 0x30000, &LE);
	//if (LE.dwCountryID == kCountryCodeNorthAmerica)
	//{
	//	LE.dwCountryID = kCountryCodeUnitedStates;
	//}
	if(fDefCountry) {
		*dwpCountry = LE.dwCountryID;
	}else {
		*dwpCountry = 1; // Default To USA

	}
	lineShutdown(hLineApp);

	#endif	//_TAPI

	return (fDefCountry);
} // FFillCountryList()

//
//
//  Returns 0 if successful
//
//
DWORD GetCountryCodeUsingTapiId(DWORD dwCountryId, DWORD *dwCountryCode)
{
	DWORD dwRet;
	dwRet = 0;
	struct XXForRasCntry{
		RASCTRYINFO    rci;
		TCHAR          czB[256] ; // To Store the Country Name
		
	} Rc;
	DWORD dwSz;
	*dwCountryCode = 1; // Default Value

	// Init the Sizes of Data Struct and Buffer
	Rc.rci.dwSize = sizeof(Rc.rci );
	dwSz = sizeof(Rc);

	Rc.rci.dwCountryID = dwCountryId;
	ATK_RasGetCountryInfo((RASCTRYINFO *)&Rc,&dwSz);
	*dwCountryCode = Rc.rci.dwCountryCode;
	return  dwRet;

}

BOOL FGetLocationEntry(HLINEAPP hLineApp, DWORD dwAPI,LINELOCATIONENTRY *pLE)
 /***************************************************************************
 Allocate memory for and fetch a line country list (LINECOUNTRYLIST) from
TAPI
****************************************************************************/
{
	BOOL fRet = FALSE;

	#ifdef _TAPI
	DWORD dwRet, iLoc;
	LINETRANSLATECAPS ltc, *pltc;
	LPLINELOCATIONENTRY plle;

    if (pLE == NULL) return (fRet);

    ltc.dwTotalSize = sizeof(LINETRANSLATECAPS);
    dwRet = lineGetTranslateCaps(hLineApp, dwAPI, &ltc);
    pltc = (LINETRANSLATECAPS*) LocalAlloc(LPTR, ltc.dwNeededSize+100);
    if (!pltc) return (fRet);

    pltc->dwTotalSize = ltc.dwNeededSize;
    dwRet = lineGetTranslateCaps(hLineApp, dwAPI, pltc);
    plle = (LPLINELOCATIONENTRY) (((LONG_PTR) pltc) + pltc->dwLocationListOffset);
    for (iLoc = 0; iLoc < pltc->dwNumLocations; iLoc ++)
    {
        if (pltc->dwCurrentLocationID == plle->dwPermanentLocationID)
        {
            *pLE = *plle;
            fRet = TRUE;
            break;
        }
        plle ++;
   }

    LocalFree(pltc);
	#endif
    return (fRet);
} // FGetLocationEntry()




void CALLBACK CountryLineCallback(DWORD hDevice, DWORD dwMessage, DWORD dwInstance, DWORD dwParam1,
								  DWORD dwParam2, DWORD dwParam3)
{
;

}


BOOL PrepareCountryListFromTapi(HINSTANCE hInstance,
					  LINECOUNTRYLIST		**pcl )
{
	
	DWORD				dwAPI;
	HLINEAPP hLineApp;
	DWORD numDevs;

	LINECOUNTRYLIST cl;
	


	BOOL fRet = FALSE;

	LONG tapiStatus = lineInitialize(&hLineApp,
		hInstance, (LINECALLBACK) CountryLineCallback,
		NULL, &numDevs);
	if (tapiStatus != 0)
	{
		CHAR szMessage[256];
		sprintf(szMessage,"lineInitialize call failed: error = %li",tapiStatus);
		RW_DEBUG << szMessage <<"\n"<< flush;
		return FALSE;
	}

	
	LINEEXTENSIONID extensionID;
	tapiStatus = lineNegotiateAPIVersion(hLineApp,0,dwAPILowVersion, dwAPIHighVersion,&dwAPI,&extensionID);
	
	*pcl = NULL;
	cl.dwTotalSize = sizeof(LINECOUNTRYLIST);
	// find size needed for list
	if (0 != lineGetCountry(0, 0x10003, &cl)){
	 	goto EndFn;
	}
	*pcl = (LINECOUNTRYLIST *) LocalAlloc(LPTR, cl.dwNeededSize + 100 );
	if (NULL == *pcl){
		goto EndFn;
	}

	(*pcl)->dwTotalSize = cl.dwNeededSize + 100;
	if (0 != lineGetCountry(0, 0x10003, *pcl))
	{
		goto EndFn;
	}
	

EndFn:
	lineShutdown(hLineApp);
	return (fRet);
}




//
//
//
//
//

CCntryInfo :: CCntryInfo()
{
	HINSTANCE hIns= NULL;;

#ifdef _TAPI
	m_pCountry= NULL;
	PrepareCountryListFromTapi(hIns,
					 &m_pCountry);
#endif

		
}

CCntryInfo :: ~CCntryInfo()
{
#ifdef _TAPI
	if(m_pCountry) {
		LocalFree(m_pCountry);
	}
#endif
}

int CCntryInfo :: GetCountryCode( _TCHAR * czCountryName)
{
	
#ifdef _TAPI
	LINECOUNTRYENTRY	*plce;
	_TCHAR *			pTsz;
	PSTR                psz;
	DWORD				iCountry;

#ifdef COMPILE_USING_VC
	return 1;
#endif


	int iRet = -1;
	
	plce = (LINECOUNTRYENTRY *)(((PBYTE) m_pCountry) + m_pCountry->dwCountryListOffset);
	for (iCountry = 0; iCountry < m_pCountry->dwNumCountries; ++iCountry)
	{
		psz = ((PSTR) m_pCountry ) + plce->dwCountryNameOffset;
		pTsz = (PTSTR) psz;
		//pTsz = ConvertToUnicode(psz);
		if(!_tcscmp(czCountryName,pTsz) ){
			return iCountry;
		}
		*plce ++;
	}
	return iRet;
#else
	return -1;
#endif
}

int CCntryInfo :: GetCountryCode( DWORD  dwTapiId)
{
	
#ifdef _TAPI
	LINECOUNTRYENTRY	*plce;
	_TCHAR *			pTsz;
	PSTR                psz;
	DWORD				iCountry;

#ifdef COMPILE_USING_VC
	return 1;
#endif


	int iRet = 0;
	plce = (LINECOUNTRYENTRY *)(((PBYTE) m_pCountry) + m_pCountry->dwCountryListOffset);
	for (iCountry = 0; iCountry < m_pCountry->dwNumCountries; ++iCountry)
	{
		if( plce->dwCountryID  == dwTapiId){
				return iCountry;
		}
		*plce ++;
	}
	return iRet;
#else
	return -1;
#endif
}


_TCHAR * CCntryInfo :: GetCountryName(int iCode)
{
#ifdef _TAPI
	LINECOUNTRYENTRY	*plce;

	PSTR				psz;
	PTSTR                pTsz;
	int iRet = -1;

	if( iCode < 0 ){
		iCode = 0;
	}
	if ( iCode > (int)m_pCountry->dwNumCountries) {
		iCode = 0;
	}
	plce = (LINECOUNTRYENTRY *)(((PBYTE) m_pCountry) + m_pCountry->dwCountryListOffset);
	psz = ((PSTR) m_pCountry ) + plce[iCode].dwCountryNameOffset;
	pTsz = (PTSTR) psz;
	//pTsz = ConvertToUnicode(psz);
	return pTsz;
#else
	return NULL;
#endif
}

int CCntryInfo :: GetTapiCountryCode(_TCHAR * czCountryName)
{
	LINECOUNTRYENTRY	*plce;
	PSTR				psz;
	_TCHAR *			pTsz;
	DWORD				iCountry;

	int iRet = 0;
	
	plce = (LINECOUNTRYENTRY *)(((PBYTE) m_pCountry) + m_pCountry->dwCountryListOffset);
	for (iCountry = 0; iCountry < m_pCountry->dwNumCountries; ++iCountry)
	{
		psz = ((PSTR) m_pCountry ) + plce->dwCountryNameOffset;
		pTsz = (PTSTR) psz;
		//pTsz = ConvertToUnicode(psz);
		if(!_tcscmp(czCountryName,pTsz) ){
			return plce->dwCountryID;
		}
		
		*plce ++;
	}
	return iRet;
}
//
// Used for Field checking
int CCntryInfo::GetTapiIDForTheCountryIndex ( int iCode)
{
	return GetTapiCountryCode(GetCountryName(iCode));
}



//
//   Adds the country information in the ComboBox specified in hwndCB
//
//
//
void CCntryInfo :: FillCountryList(HINSTANCE hInstance,
								   HWND hwndCB)
{

#ifdef _TAPI
	LINECOUNTRYENTRY	*plce;
	PSTR				psz;
	PTSTR               pTsz;
	DWORD				iCountry;

	if(	hwndCB == NULL || m_pCountry == NULL ) {
		// if the country list or Combo control handle is Null
		return ;
	}
	int iRet = -1;
	
	plce = (LINECOUNTRYENTRY *)(((PBYTE) m_pCountry) + m_pCountry->dwCountryListOffset);
	for (iCountry = 0; iCountry < m_pCountry->dwNumCountries; ++iCountry)
	{
		psz = ((PSTR) m_pCountry ) + plce->dwCountryNameOffset;
		pTsz = (PTSTR) psz;
		LRESULT dwAddStatus = SendMessage(hwndCB, CB_ADDSTRING, 0, (LPARAM) pTsz);
/**
#ifdef COMPILE_USING_VC
		DWORD dwAddStatus = SendMessage(hwndCB, CB_ADDSTRING, 0, (LPARAM) psz);
#else
		pTsz= ConvertToUnicode(psz);
		DWORD dwAddStatus = SendMessage(hwndCB, CB_ADDSTRING, 0, (LPARAM) pTsz);
#endif
**/		

		if (dwAddStatus == CB_ERR){
			return;
		}
		*plce ++;
	}
#endif


}
