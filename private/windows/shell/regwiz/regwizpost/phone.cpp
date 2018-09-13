/*

	File : Phone.CPP
*/

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <tchar.h>
#include <ATK_RAS.H>

#include "cntryinf.h"
#include "resource.h"
#include "phonebk.h"
#include "util.h"
#include "rw_common.h"


#define TYPE_SIGNUP_ANY			0x82
#define MASK_SIGNUP_ANY			0xB2

#define TYPE_REGULAR_USAGE		0x42
#define MASK_REGULAR_USAGE		0x73


#define MAX_BUFFER				1024 * 5
#define RAS_ENTRY_NAME			_T("REGWIZ")



//
//  This function expands and substitutes Environment variable
//  in input string
//
//
static void GetAbsolutePath( LPTSTR input,LPTSTR output)
	{
		if(_tcschr(input,_T('%')) == NULL) {
			_tcscpy(output,	input);
			return ;
		}

		if(input[0] == _T('%'))
		{
			LPTSTR token = _tcstok(input,_T("%"));
			if(token != NULL)
			{
				LPTSTR sztemp;
				sztemp = _tgetenv( token );
				if(sztemp != NULL)
				{
					_tcscpy(output ,sztemp);
				}
				token = _tcstok(NULL,_T("\0"));
				if(token != NULL)
				{
					_tcscat(output ,token);
				}
			}
		}
		else
		{
			LPTSTR token = _tcstok(input,_T("%"));
			if(token != NULL)
			{
				_tcscpy(output ,token);
				token = _tcstok(NULL,_T("%"));
				if(token != NULL)
				{
					LPTSTR sztemp;
					sztemp = _tgetenv( token );
					if(sztemp != NULL)
					{
						_tcscat(output ,sztemp);
					}
					token = _tcstok(NULL,_T("\0"));
					if(token != NULL)
					{
						_tcscat(output ,token);
					}
				}
			}
		}
		
		GetAbsolutePath(output,output);
	}




#define MAX_ALTERNATES     5
#define MAX_DUN_ENTRYNAME  256
class DUNFileProcess {
public :
	DUNFileProcess(HINSTANCE hIns, TCHAR *czFile);
	//~DUNFileProcess();
	DWORD CreateRasEntry( RASENTRY	*pRasEntry, // Pointer of the RASENTRY structre to be created
				    LPTSTR szUserName,  // User name of the ISP
				    LPTSTR szPassword); // Password for the ISP

	HINSTANCE m_hInstance;
	TCHAR     m_szDUNFile[MAX_PATH];
	TCHAR     m_szDunEntry[MAX_ALTERNATES][MAX_DUN_ENTRYNAME];
	TCHAR * GetDunFileRoot();
	TCHAR * GetFromResource(int iResId);

};

DUNFileProcess :: DUNFileProcess (HINSTANCE hIns, TCHAR *czFileName)
{
	m_hInstance = hIns;
	_tcscpy(m_szDUNFile,GetDunFileRoot());
	_tcscat(m_szDUNFile, _T("\\"));
	_tcscat(m_szDUNFile,czFileName);
	
		
}



//
// This gets the directory from where the ICWIP.DUN can be found
// The default is the C:\Progra~1\ICW-IN~1\\
//
TCHAR * DUNFileProcess :: GetDunFileRoot()
{
	HKEY    hKey;
	LONG  regStatus;
	TCHAR   czTemp[256];
	//TCHAR  uszRegKey[]="SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\ICWCONN1.EXE";
	//TCHAR  uszR[ ]= "Path";
	DWORD  dwInfoSize;
	static TCHAR  szDunRoot[256] = _T("c:\\Progra~1\\ICW-IN~1\\");

	dwInfoSize = 256;
	regStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					GetFromResource(IDS_ICW_DIRECTORY_ENTRY),
					0,KEY_READ ,&hKey);
	if (regStatus == ERROR_SUCCESS) {
		// Get The Path
		dwInfoSize = MAX_PATH;
		RegQueryValueEx(hKey,
			GetFromResource(IDS_ICW_DIRECTORY_ENTRY_VALUE),
			NULL,0,
			(LPBYTE) czTemp,
			&dwInfoSize);
		GetAbsolutePath(czTemp,szDunRoot);
		size_t sLen = _tcslen(szDunRoot);
		szDunRoot[sLen-1] = _T('\0');
			
	}
	else {
		;
	}
	return szDunRoot;

}

TCHAR * DUNFileProcess ::GetFromResource(int iResId)
{
	
	
	static int iCount=0;
	if(iCount == MAX_ALTERNATES ) {
		iCount = 0;
	}
	LoadString(m_hInstance,iResId,m_szDunEntry[iCount],
		MAX_DUN_ENTRYNAME);
	
	return m_szDunEntry[iCount++];
}

void FillIpAddress1(_TCHAR *czStr, RASIPADDR * pRE )
{
	
	_TCHAR czTemp[4];
	int iCount=0;
	int iIndex=0;
	int Location =0;
	for(int Addr=0; Addr< 4; Addr++) {
		iCount = 0;
		for( ;czStr[iIndex] !=_T('.') && iCount <3;  )
		{
			czTemp[iCount] = czStr[iIndex];
			iCount += _tcslen( czTemp + iCount );
			iIndex += _tcslen( czStr + iIndex );

		}	
		iIndex += _tcslen( czStr + iIndex );
		czTemp[iCount] = _T('\0');
		
		switch(Addr) {
		case 0:
			pRE->a = (BYTE) _ttoi(czTemp);
			break;
		case 1:
			pRE->b = (BYTE) _ttoi(czTemp);
			break;
		case 2:
			pRE->c = (BYTE) _ttoi(czTemp);
			break;
		case 3:
			pRE->d = (BYTE) _ttoi(czTemp);
			break;
		default:
		break;
		}
	}


}


void FillIpAddress(_TCHAR *czStr, RASIPADDR * pRE )
{
	_TCHAR czTemp[4];
	int iCount=0;
	int iIndex=0;
	int Location =0;
	for(int Addr=0; Addr< 4; Addr++) {
		iCount = 0;
		for( ;czStr[iIndex] != _T('.') && iCount <3;  ) {
			czTemp[iCount++] =
				czStr[iIndex];
				iIndex++;

		}	
		iIndex++;
		czTemp[iCount] = _T('\0');
		switch(Addr) {
		case 0:
			pRE->a = (BYTE) _ttoi(czTemp);
			break;
		case 1:
			pRE->b = (BYTE) _ttoi(czTemp);
			break;
		case 2:
			pRE->c = (BYTE) _ttoi(czTemp);
			break;
		case 3:
			pRE->d = (BYTE) _ttoi(czTemp);
			break;
		default:
		break;
		}
	}


}






//
// Returns  1 if successful
//          0 failure
//
DWORD DUNFileProcess :: CreateRasEntry(
								   RASENTRY	*pRasEntry,
								   LPTSTR szUserName,
								   LPTSTR szPassword)
{
	TCHAR	szSection[MAX_PATH];
    TCHAR	szValue[MAX_PATH];
    TCHAR   szScript[MAX_PATH];
    TCHAR   szWinPath[MAX_PATH];
	DWORD   dwRead;

	BOOL	bRet= FALSE;

	pRasEntry->dwSize		 = sizeof(RASENTRY);

	pRasEntry->dwfOptions	 = RASEO_UseCountryAndAreaCodes |
							   RASEO_ModemLights;	
							


	// Framing
	//
	pRasEntry->dwFrameSize		  = 0;
	_tcscpy(pRasEntry->szScript, _T(""));

	// Auto-Dial
	//
	_tcscpy(pRasEntry->szAutodialDll, _T(""));
	_tcscpy(pRasEntry->szAutodialFunc, _T(""));

	// Device
	//
	_tcscpy(pRasEntry->szDeviceType, RASDT_Modem);
	// _tcscpy(pRasEntry->szDeviceName, GetModemDeviceInformation(m_hInstance));
	// The Modem entry will be created before creating  the Phone book entry

	// X.25
	//
	_tcscpy(pRasEntry->szX25PadType, _T(""));
	_tcscpy(pRasEntry->szX25Address, _T(""));
	_tcscpy(pRasEntry->szX25Facilities, _T(""));
	_tcscpy(pRasEntry->szX25UserData, _T(""));
	pRasEntry->dwChannels = 0;

	// Phone
	//
	_tcscpy(szSection,GetFromResource(IDS_DUN_PHONE_SECTION));
	
	// Area Code
    DWORD  dwAreaCode  = GetPrivateProfileInt (szSection,
		GetFromResource(IDS_DUN_PHONE_AREACODE),
		0,
		m_szDUNFile);
	_tprintf(pRasEntry->szAreaCode, "%d", dwAreaCode);
	
	// Country Code
	pRasEntry->dwCountryCode = GetPrivateProfileInt (szSection,
		GetFromResource(IDS_DUN_PHONE_COUNTRY),
		1, m_szDUNFile);
    RW_DEBUG <<"\n Country Code "  << pRasEntry->dwCountryCode;
	pRasEntry->dwCountryID = GetPrivateProfileInt (szSection, _T("Country_ID"), 1, m_szDUNFile);

    RW_DEBUG <<"\n Country ID "  << pRasEntry->dwCountryID;
	//
	GetPrivateProfileString (szSection,
		GetFromResource(IDS_DUN_PHONE_DIALAS),
		GetFromResource(IDS_DUN_VALUE_YES),
		szValue, sizeof(szValue), m_szDUNFile);
	pRasEntry->dwAlternateOffset=0;

	// TODO : Set Phone No from  Phone Book
	_tcscpy(pRasEntry->szLocalPhoneNumber,_T(""));
	
	// Server
	//	

	// Framing Protocol
	_tcscpy(szSection,GetFromResource(IDS_DUN_SERVER_SECTION));
    GetPrivateProfileString (szSection,
		GetFromResource(IDS_DUN_SERVER_TYPE),
		GetFromResource(IDS_DUN_SERVER_TYPE_PPP), // PPP
		szValue, sizeof (szValue), m_szDUNFile);

	if(!_tcsicmp(szValue,GetFromResource(IDS_DUN_SERVER_TYPE_PPP))) {
		pRasEntry->dwFramingProtocol  = RASFP_Ppp;
	}


	// SW Compression
    GetPrivateProfileString (szSection,GetFromResource(IDS_DUN_SERVER_SW_COMPRESS),
		GetFromResource(IDS_DUN_VALUE_YES),
		szValue, sizeof (szValue), m_szDUNFile);

	if(!_tcsicmp(szValue,GetFromResource(IDS_DUN_VALUE_YES))) {
		pRasEntry->dwfOptions |=RASEO_SwCompression;
	}
	
	//	Negociate TCP		
    GetPrivateProfileString (szSection,
		GetFromResource(IDS_DUN_SERVER_TCPIP),
		GetFromResource(IDS_DUN_VALUE_YES),
		 szValue, sizeof (szValue), m_szDUNFile);

	if(!_tcsicmp(szValue,GetFromResource(IDS_DUN_VALUE_YES))) {
		pRasEntry->dwfNetProtocols = RASNP_Ip;
	}

	// Disable LCP

    GetPrivateProfileString (szSection,
		GetFromResource(IDS_DUN_SERVER_LCP),
		GetFromResource(IDS_DUN_VALUE_YES),
		szValue, sizeof (szValue), m_szDUNFile);
	if(!_tcsicmp(szValue,GetFromResource(IDS_DUN_VALUE_YES))) {
		pRasEntry->dwfOptions |=RASEO_DisableLcpExtensions;
	}


	// TCP/IP
	_tcscpy(szSection, GetFromResource(IDS_DUN_TCP_SECTION));
    GetPrivateProfileString (szSection,
		GetFromResource(IDS_DUN_TCP_IPADDRESS),
		GetFromResource(IDS_DUN_VALUE_NO),
		szValue, sizeof (szValue), m_szDUNFile);

	if(!_tcsicmp(szValue,GetFromResource(IDS_DUN_VALUE_YES))) {
		pRasEntry->dwfOptions	 |=RASEO_SpecificIpAddr;
	}

    GetPrivateProfileString (szSection,
		GetFromResource(IDS_DUN_TCP_SERVERADDRESS),
		GetFromResource(IDS_DUN_VALUE_NO),
		szValue, sizeof (szValue), m_szDUNFile);
	if(!_tcsicmp(szValue,GetFromResource(IDS_DUN_VALUE_YES))) {
		pRasEntry->dwfOptions	 |=RASEO_SpecificNameServers;
	}

    GetPrivateProfileString (szSection,
		GetFromResource(IDS_DUN_TCP_HEADERCOMPRESSION),
		GetFromResource(IDS_DUN_VALUE_NO),
		szValue, sizeof (szValue), m_szDUNFile);
	if(!_tcsicmp(szValue,GetFromResource(IDS_DUN_VALUE_YES))) {
		pRasEntry->dwfOptions	 |=RASEO_IpHeaderCompression;
	}

    GetPrivateProfileString (szSection,
		GetFromResource(IDS_DUN_TCP_GATEWAY),
		GetFromResource(IDS_DUN_VALUE_NO),
		szValue, sizeof (szValue), m_szDUNFile);
	if(!_tcsicmp(szValue,GetFromResource(IDS_DUN_VALUE_YES))) {
		pRasEntry->dwfOptions	 |=RASEO_RemoteDefaultGateway;
	}

	GetPrivateProfileString (szSection,
		GetFromResource(IDS_DUN_TCP_SERVERADDRESS),
		GetFromResource(IDS_DUN_VALUE_NO),
		szValue, sizeof (szValue), m_szDUNFile);
	if(!_tcsicmp(szValue,GetFromResource(IDS_DUN_VALUE_YES))) {

		dwRead = GetPrivateProfileString (szSection,
			GetFromResource(IDS_DUN_TCP_DNS),
			_T("0"), szValue,
			sizeof (szValue), m_szDUNFile);
		if( dwRead >2) {
			FillIpAddress(szValue,&pRasEntry->ipaddrDns);
		}
		//
		//  Get The Secondary Address
		dwRead = GetPrivateProfileString (szSection,
				GetFromResource(IDS_DUN_TCP_DNS_ALT),
				_T("0"), szValue,
			sizeof (szValue), m_szDUNFile);
		if(dwRead > 2) {
			FillIpAddress(szValue,&pRasEntry->ipaddrDnsAlt);
		}
		
	}

	// User Section
	_tcscpy(szSection,
		GetFromResource(IDS_DUN_USER_SECTION));
	GetPrivateProfileString (szSection,
	GetFromResource(IDS_DUN_USER_NAME),
		_T(""), szUserName, MAX_PATH, m_szDUNFile);
	GetPrivateProfileString (szSection,
		GetFromResource(IDS_DUN_USER_PASSWORD),
		_T(""), szPassword, MAX_PATH, m_szDUNFile);



	// Scripting
	_tcscpy(szSection, _T("Scripting"));
    GetPrivateProfileString (szSection, _T("Name"), _T(""), szScript, MAX_PATH, m_szDUNFile);


    if ( lstrcmpi(szScript, _T("") ) != 0 ) {
       if (!GetWindowsDirectory(szWinPath, MAX_PATH) )
       {
          #ifdef _LOG_IN_FILE
            RW_DEBUG << "\n CreateRasEntry : Error 1- Fail to Get Windows Directory" << flush;
          #endif
          return TRUE;    
       }
       //BUGBUG:Needs specail procesing for win9x
       if ( _T('\\') != szWinPath[_tcslen(szWinPath)-1])
       {
           lstrcat(szWinPath,_T("\\"));
       }
       lstrcat(szWinPath,szScript);
       lstrcpy(szScript,szWinPath);
       if (_tcslen(szScript) >=MAX_PATH)
       {
           #ifdef _LOG_IN_FILE
			  RW_DEBUG << "\n CreateRasEntry : Error 1- Script File Path too long" << flush;
		   #endif
           return TRUE;    
       }

    }

    lstrcpy(pRasEntry->szScript,szScript);    

	// Script_File
	TCHAR	szScriptBuffer[MAX_BUFFER];
	_tcscpy(szSection, _T("Script_File"));
    GetPrivateProfileString (szSection, NULL, _T(""), szScriptBuffer, MAX_BUFFER, m_szDUNFile);
	


	
	// Create the .SCP File and Dump the script data
	PTSTR	pKey=szScriptBuffer;
	HANDLE fileHandle = CreateFile(pRasEntry->szScript, GENERIC_WRITE,0, 0, CREATE_ALWAYS, 0, 0);
	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\n Load DUN Strings : Error 1- Fail to create Script File " << flush;
		#endif
		return TRUE;
	}
	

	BOOL bWrite = TRUE;
	DWORD numWrite;
	while (*pKey && bWrite)
	{
        
		GetPrivateProfileString (szSection, pKey , _T(""), szValue, MAX_PATH, m_szDUNFile);
        _tcscat(szValue, _T("\x0d\x0a"));
#ifdef _UNICODE 
        char *czP;		
        czP = ConvertToANSIString(szValue);
        bWrite = WriteFile(fileHandle, czP, strlen(czP)*sizeof(char), &numWrite, 0);
#else
        bWrite = WriteFile(fileHandle, szValue, _tcslen(szValue)*sizeof(_TCHAR), &numWrite, 0);
#endif
		while (*pKey)
		{
			pKey = _tcsinc(pKey);
		}
		pKey = _tcsinc(pKey);
	}
	CloseHandle(fileHandle);
	return TRUE;
}	

int   GetDeaultCfgAreaCode(HINSTANCE hInstance, DWORD *CountryCode, DWORD *AreaCode)
{
	HKEY    hKey;
	TCHAR   szTel[256] = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations");
	TCHAR   szCI [48] = _T("CurrentID");
	TCHAR   szNE [48]  = _T("NumEntries");
	TCHAR   szID  [48] = _T("ID");
	TCHAR   szCountryReg[48] = _T("Country");
	_TCHAR  szAreaCodeReg[48];
	_TCHAR  czLastStr[48];
	_TCHAR  czNewKey[256];
	_TCHAR  szAreaCode[48];
	DWORD   dwCurrentId;
	DWORD   dwNumEntries;
	DWORD   dwId;

	LONG	lStatus;
	DWORD dwInfoSize = 48;
	DWORD   dwAreaCode = 0;
	int    iRetValue;
	iRetValue =0;
	*CountryCode=1;  // Default USA
	*AreaCode = 1; // Default Area Code

	LoadString(hInstance,IDS_TELEPHONE_LOC,szTel,256);
	LoadString(hInstance,IDS_TELEPHONE_CID,szCI,48);
	LoadString(hInstance,IDS_TELEPHONE_NENT,szNE,48);
	LoadString(hInstance,IDS_TELEPHONE_ID,szID,48);
	LoadString(hInstance,IDS_TELEPHONE_COUNTRY,szCountryReg,48);

	lStatus= RegOpenKeyEx(HKEY_LOCAL_MACHINE,szTel,0,KEY_READ,&hKey);
	if (lStatus == ERROR_SUCCESS)
	{
		//  Get Index
		//
		dwInfoSize = sizeof(dwCurrentId);
		lStatus = RegQueryValueEx(hKey,szCI,NULL,0,(  LPBYTE )&dwCurrentId,&dwInfoSize);
		if( lStatus !=  ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			return iRetValue;
		}
		//
		/*dwInfoSize = sizeof(dwNumEntries);
		lStatus = RegQueryValueEx(hKey,szNE,NULL,0,(  LPBYTE )&dwNumEntries,&dwInfoSize);
		if( lStatus !=  ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			return iRetValue;
		}*/
		
		RegCloseKey(hKey);

	}

	//
	// Now Contine to scan
	//for (int iCount =0; iCount < dwNumEntries; iCount ++ )
		
	_stprintf(czLastStr,_T("\\Location%d"),dwCurrentId);
	_tcscpy(czNewKey,szTel);
	_tcscat(czNewKey,czLastStr);

	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n RegKey Location:"<< czNewKey << flush;
	#endif

	lStatus= RegOpenKeyEx(HKEY_LOCAL_MACHINE,czNewKey,0,KEY_READ,&hKey);

	if (lStatus == ERROR_SUCCESS)
	{
			dwInfoSize = sizeof(dwCurrentId);
/*			lStatus = RegQueryValueEx(hKey,szID,NULL,0,(  LPBYTE )&dwId,&dwInfoSize);
			if( lStatus ==  ERROR_SUCCESS)
				if(dwId == dwCurrentId)
					dwInfoSize = sizeof(dwCurrentId);; */
			lStatus = RegQueryValueEx(hKey,szCountryReg,NULL,0,(  LPBYTE )CountryCode,&dwInfoSize);

			LoadString(hInstance,IDS_TAPI_AREA_CODE,szAreaCodeReg,48);
			dwInfoSize = 48;
			lStatus = RegQueryValueEx(hKey,szAreaCodeReg,NULL,0,(  LPBYTE )szAreaCode,&dwInfoSize);
			if( lStatus ==  ERROR_SUCCESS)
			{
				*AreaCode = _ttol(szAreaCode);
			}

			RegCloseKey(hKey);
			return dwAreaCode;
	}
	return iRetValue;
}



//
//  returns 0 if Error
//          1 if Succesful
//
//
DWORD  GetRASEntries(HINSTANCE hInstance,
					 RASENTRY *pRasEntry,
					 LPTSTR szUserName,
					 LPTSTR szPassword)
{
	HKEY    hKey;
	DWORD_PTR	dwPhoneID;
	DWORD	dwCountry;
	DWORD   dwCountryCode;
	DWORD   dwAreaCode = 0;
	TCHAR   szResEntry[256];

	
	dwPhoneID = 0;

	//LoadString(hInstance,IDS_PHONEBOOK_ENTRY,szResEntry,255);
	//
	//  Presently this string is not passed as the Phone bokk is
	//  still not a UNICODE version    CXG 6-0497
	//

	HRESULT hr	= PhoneBookLoad("MSICW", &dwPhoneID);
	if (!dwPhoneID)
	{
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\n Fn: GetRasEntries : Error-1 Unable to locate MSN.ISP " << flush;
		#endif
		return FALSE;
	}
	//if (!GetTapiCurrentCountry(m_hInst, &dwCountry)) { on6/4/97
	if (!GetTapiCurrentCountry(hInstance, &dwCountry))
	{

		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\n Fn: GetRasEntries : Error-2  Unable To Get Country Code " << flush;
		#endif
		return FALSE ;
	}
	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n Fn: GetRasEntries : Get Current Country " <<  dwCountry << flush;
	#endif

	GetDeaultCfgAreaCode(hInstance,&dwCountryCode,&dwAreaCode);

	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n Fn: GetRasEntries : GetDeaultCfgAreaCode  Country " <<  dwCountryCode << "Area " << dwAreaCode << flush;
		RW_DEBUG << "\n Country ID: " << dwCountry << flush;
	#endif

	SUGGESTINFO SuggestInfo;
	SuggestInfo.dwCountryID = dwCountry;
	SuggestInfo.bMask	= MASK_SIGNUP_ANY;
	SuggestInfo.fType	= TYPE_SIGNUP_ANY;
	SuggestInfo.wAreaCode	= dwAreaCode;
	SuggestInfo.wNumber  	= 1;
	
	int nIndex = PhoneBookSuggestNumbers(dwPhoneID, &SuggestInfo);
	if (nIndex != 0)
	{
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\n Fn: GetRasEntries : Error-3  In Phone Suggest Number" << flush;
		#endif
		return FALSE;
	}
	PACCESSENTRY m_rgAccessEntry = *(SuggestInfo.rgpAccessEntry);
	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\n Fn: GetRasEntries : Phone Book Suggestions " << flush;
		RW_DEBUG << "\n\t Index " << m_rgAccessEntry->dwIndex ;
		RW_DEBUG << "\n\t Phone Number Type "  << m_rgAccessEntry->fType;
		RW_DEBUG << "\n\t StateId " << m_rgAccessEntry->wStateID;
		RW_DEBUG << "\n\t CountryId " << m_rgAccessEntry->dwCountryID;
		RW_DEBUG << "\n\t AreaCode "  <<  m_rgAccessEntry->dwAreaCode ;
		RW_DEBUG << "\n\t CityName " << m_rgAccessEntry-> szCity;
		RW_DEBUG << "\n\t Access Number " << m_rgAccessEntry->szAccessNumber;
		RW_DEBUG << "\n\t Actual Area Code "  << m_rgAccessEntry->szAreaCode << flush;
	#endif

	//BOOL FillRASEntries(RASENTRY	*pRasEntry);

	DUNFileProcess    dfp(hInstance,
		ConvertToUnicode(m_rgAccessEntry->szDataCenter));
	if(!dfp.CreateRasEntry(pRasEntry,szUserName,szPassword))
	{
		#ifdef _LOG_IN_FILE
		  RW_DEBUG << "\n GetRas Entries -Error 4 : Processinfg DUN File "  << flush;	
		#endif
		return FALSE;
	}
	pRasEntry->dwCountryID  = dwCountry;
	GetCountryCodeUsingTapiId(dwCountryCode,&dwCountryCode) ;
	pRasEntry->dwCountryCode  =dwCountryCode;

	_tcscpy(pRasEntry->szLocalPhoneNumber, ConvertToUnicode(m_rgAccessEntry->szAccessNumber));
	_tcscpy(pRasEntry->szAreaCode,ConvertToUnicode(m_rgAccessEntry->szAreaCode));

	RW_DEBUG << "\n Counntry ID " <<pRasEntry->dwCountryID  << " Country Code " << pRasEntry->dwCountryCode  << flush;

	RW_DEBUG << "\n AreaCode: " << ConvertToANSIString(pRasEntry->szAreaCode);
	RW_DEBUG << "Local No Real: " << m_rgAccessEntry->szAccessNumber << flush;
	RW_DEBUG << "Local No: " << ConvertToANSIString(pRasEntry->szLocalPhoneNumber) << flush;


	return TRUE;
}




DWORD ConfigureDUN ( HWND hWnd, HINSTANCE hInstance,
					 RASENTRY	*pRasEntry,
					 TCHAR	*szUserName,
					 TCHAR	*szPassword,
					 int        iModemIndex)
{
	
	DWORD dwEntrySize;
	DWORD dwRet;
	
	// Prepare RASENTRY for Dial-up
	//
	
	if(!GetRASEntries(hInstance,
		pRasEntry,
		szUserName,
		szPassword) )
	{
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\n Fn : ConfigureDUN   : Error3  Error Getting Ras Entries " << flush;
		#endif
		return FALSE;
	}
	_tcscpy(pRasEntry->szDeviceName, GetModemDeviceInformation(hInstance, iModemIndex));

#ifdef _WIN95
	//
	//  remove Old entry
	//  Verify if name exists in Registry
	//  if so remove the name from Registry
	//
	//
	//
	HANDLE  hRegKey ;
	LONG regStatus;
	DWORD dwInfoSize;
	char  szParam[512];

	regStatus = RegOpenKeyEx(HKEY_CURRENT_USER,
					"RemoteAccess\\Addresses",0,
					KEY_READ,&hRegKey);
	if (regStatus == ERROR_SUCCESS)	{
		//  Key Exists  so search if REG WIZ exists
		dwInfoSize = 512;
		regStatus = RegQueryValueEx(hRegKey,RAS_ENTRY_NAME,NULL,0,
			(unsigned char*) szParam,&dwInfoSize);
		if (regStatus == ERROR_SUCCESS)
		{
			// The Key Exits So delete the Entry
			RegDeleteValue(hRegKey,RAS_ENTRY_NAME);

			#ifdef _LOG_IN_FILE
				RW_DEBUG << "\nRegistry Entry found  "  << RAS_ENTRY_NAME <<
				"  and Deleting the Entry "  <<  flush;
			#endif
			
		}
		 RegCloseKey(hRegKey);

	}
#else
	ATK_RasDeleteEntry(NULL,RAS_ENTRY_NAME);
#endif

	
	dwEntrySize  = sizeof(RASENTRY);
	#ifdef _LOG_IN_FILE
		RW_DEBUG << "\nAfter Ras Get Properties :";
		RW_DEBUG << "\n\t Device Name " << ConvertToANSIString(pRasEntry->szDeviceName) << flush;
	#endif

	dwRet = ATK_RasSetEntryProperties( NULL,
						   RAS_ENTRY_NAME,
						   pRasEntry,
						   dwEntrySize ,
						   NULL,
						   0);
	
	if(dwRet ==  ERROR_CANNOT_OPEN_PHONEBOOK )
	{
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nFun  ConfigureDUN : ERROR_CANNOT_OPEN_PHONEBOOK  "  << dwRet  << flush;
		#endif
		return FALSE;
	}
	else
	if(dwRet ==  ERROR_BUFFER_INVALID )
	{
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\nFun  ConfigureDUN : ERROR_BUFFER_INVALID  "  << dwRet  << flush;
		#endif
		return FALSE;
	}
	else
	if(dwRet)
	{
		#ifdef _LOG_IN_FILE
			RW_DEBUG << "\n ConfigureDUN : Unknown/Undocumented  Error "  << dwRet  << flush;
		#endif
		return FALSE;
	}
	return TRUE;
}
