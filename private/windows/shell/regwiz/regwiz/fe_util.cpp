/*
	FE_UTIL.CPP
	FareEastCountries heper functions
	Author : Suresh Krishnan
	Date   :03/02/98

*/

#include <fe_util.h>
#include <tchar.h>
#include <windows.h>
#include <winnls.h>
#include <rw_common.h>
#include <resource.h>


FeCountriesIndex gWhatFECountry    = kNotInitialised;
FeScreenType     gWhichFEScreenTye =kFEWithNonJapaneaseScreen;



typedef struct {
	int m_iMaxCountries;
	int m_iCountryCode[MAX_FE_COUNTRIES_SUPPORTED];
	FeScreenType m_iScreenType[MAX_FE_COUNTRIES_SUPPORTED];
}FEInfoTable;

static FEInfoTable     sFETable;




void GetFECountryListFromResource(HINSTANCE hIns )
{
	sFETable.m_iMaxCountries=0;
	int iCount =0;
	int iTokLen;
	int iResLen;
	_TCHAR	seps[] = _T(",");
	_TCHAR *pDummy;

	LPTSTR	token;
	TCHAR 	buf[80];
    TCHAR   tcSrc[512];
	iResLen = LoadString(hIns,IDS_FECOUNTRY_LIST,tcSrc,512);
	token = _tcstok( tcSrc, seps );
    sFETable.m_iMaxCountries = 0;
//	token = _tcstok( NULL, seps );
	while( token != NULL ) {
		
		_tcscpy(buf,token);
		iTokLen= _tcslen(token);
		if( iTokLen < 3) {
			goto  FinishScan; // Error in string format so skip
		}
		sFETable.m_iMaxCountries = iCount+1;
		// Get What Type of screen to use
		if( token[iTokLen-1] == _T('1')) {
			sFETable.m_iScreenType[iCount]=kFEWithJapaneaseScreen;
		}else{
			sFETable.m_iScreenType[iCount]=kFEWithNonJapaneaseScreen;
		}
		//Get the Country Code
		//buf[iTokLen-2] = _T('\0');
		sFETable.m_iCountryCode[iCount]= _tcstol(buf,&pDummy,16);
		
		iCount++;
		if(iCount >= MAX_FE_COUNTRIES_SUPPORTED ) {
			goto FinishScan;
			// Presently our Table supports 256  entries

		}
		/* Get next token: */
		token = _tcstok( NULL, seps );

   }

   FinishScan :
   RW_DEBUG  << "\n Total FE Countries Cfg " << sFETable.m_iMaxCountries;
   for(int ij=0;ij<sFETable.m_iMaxCountries;ij++) {
	   RW_DEBUG  <<"\nCountry " << sFETable.m_iCountryCode[ij] << " ScreenType" << sFETable.m_iScreenType[ij] << flush;
   }

}

//
//  This function checks if the dwCurCountry has an entry in the FE Table
//  if so it pFeType as FE country and gives the corrosponding screen type
//  in pFeScrType
//
DWORD MapCountryLcidWithFETable(DWORD dwCurCountry,
						  FeCountriesIndex *pFeType,
						  FeScreenType      *pFeScrType
						  )
{
	DWORD dwReturn;
	int iIndex;
	dwReturn = 0;
	for(iIndex = 0; iIndex < sFETable.m_iMaxCountries;iIndex++) {
		if( (DWORD)sFETable.m_iCountryCode[iIndex] == dwCurCountry ) {
			// It matches in the FE list
			*pFeType  = kFarEastCountry;
			*pFeScrType =sFETable.m_iScreenType[iIndex];
			return dwReturn;

		}

	}
	return dwReturn;

}


//
//	This fucntion gets the current LCID of the system
//  uisng GetSystemDefaultLCID().
//
//
FeCountriesIndex IsFarEastCountry(HINSTANCE hIns)
{
	LCID  lcRet;
	int   RegSettings;
	if( gWhatFECountry == kNotInitialised ) {
		GetFECountryListFromResource(hIns);
		lcRet = GetSystemDefaultLCID();

		RW_DEBUG << "\n GetSystemLCID Returns :"<< lcRet << flush;	
		gWhatFECountry = kNotAFECountry;
		MapCountryLcidWithFETable(lcRet, &gWhatFECountry,
			&gWhichFEScreenTye);
	}else {
		
		;
	}
	return gWhatFECountry;
}

FeScreenType  GetFeScreenType()
{
	return gWhichFEScreenTye;
}
