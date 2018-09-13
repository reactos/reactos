/*

	File : RW_COMMON.CPP
	Date : 04/22/97
	Author : Suresh Krishnan

	Common functions for RegWiz 

*/

#include "RW_Common.h"
#include "resource.h"
#include <ATKInternet.h>

#define STRCONVERT_MAXLEN    1024
static TCHAR gszProductBeingRegistred[128]= _T("");
static TCHAR gszTempProdWithOem[256] =_T("");
//
// This Function returns the Security Descriptor
//
REGSAM RW_GetSecDes() 
{ 
#ifdef _WINDOWS95
	return NULL; 
#else 
	return NULL;
	//
	// To be implemented for NT
#endif
}


void RWDebug :: UseStandardOutput()
{
	fp = stdout;
}

void RWDebug :: CreateLogFile(char *czFName)
{
	char   czTmpPath[256] = "";
	char   czLogFile[256];
	czLogFile[0] = '\0';

#ifdef _LOG_IN_FILE
	if( GetEnvironmentVariableA("TEMP",czTmpPath,228) > 0 ) {
			strcpy(czLogFile,czTmpPath);
	}
	strcat(czLogFile,"\\");
	strcat(czLogFile,czFName);
	fp = fopen(czLogFile,"w");
	if(fp == NULL) {
		m_iError = -1;
	}
#else
	UseStandardOutput();
#endif

		
}

RWDebug& RWDebug:: Write (char *czP)
{
	
	if(m_iError < 0  ) return *this;

	if(fp == NULL) {
		CreateLogFile("REGWIZ.LOG");
		// Fail to create file
		if(m_iError < 0) return *this;
	}

	

	if(czP  && *czP) {
		//OutputDebugStringA(czP);
		
		m_iError = fputs(czP,fp);
		m_iError = fflush(fp);
	}
    return *this;
}


RWDebug& RWDebug:: operator << (int  iv)
{
	sprintf(czTemp,"%d",iv);
	return Write(czTemp);;
}

RWDebug& RWDebug:: operator << (unsigned int  iv)
{
	sprintf(czTemp,"%d",iv);
	return Write(czTemp);;
    
}

RWDebug& RWDebug:: operator << (long  iv)
{
	
	sprintf(czTemp,"%d",iv);
	return Write(czTemp);
    
}

RWDebug& RWDebug:: operator << (unsigned long  iv)
{
	sprintf(czTemp,"%d",iv);
	return Write(czTemp);;
	
}


RWDebug& RWDebug:: operator << (short   iv)
{
	
	sprintf(czTemp,"%d",iv);
	return Write(czTemp);;
    
}

RWDebug& RWDebug:: operator << (unsigned short  iv)
{
	
	sprintf(czTemp,"%d",iv);
	return Write(czTemp);
}

RWDebug& RWDebug:: operator << (char cV)
{
	
	sprintf(czTemp,"%c",cV);
	return Write(czTemp);
    
}

RWDebug& RWDebug:: operator << (float  iv)
{
	
	sprintf(czTemp,"%10.4f",iv);
	return Write(czTemp);
    
}

RWDebug& RWDebug:: operator << (char *czP)
{
	return Write(czP);
	
}

RWDebug& RWDebug:: operator << (const char *czP)
{
	return Write((char *) czP);
	
}
RWDebug& RWDebug :: operator << ( unsigned short *usv) 
{ 
	char *czP;
	if(usv && *usv) {
		czP = ConvertToANSIString(usv);
		if(czP && *czP)
		Write((char *) czP);

	}
	return *this;
}

RWDebug& RWDebug :: operator << ( void *p) 
{ 
	
	if(p) {
		sprintf(czTemp,"Addr :0x%p",p);
		Write(czTemp);

	}
	return *this;
}
 
RWDebug& GetDebugLogStream()
{
	static RWDebug  rwD;
	return rwD;
}
/***
ostream &GetDebugLogStream()
{
	static ostream *os;
	char   czTmpPath[256] = "";
	char   czLogFile[256];

	czLogFile[0] = '\0';
	if(os == NULL)  {
#ifdef _LOG_IN_FILE
		if( GetEnvironmentVariableA("TEMP",czTmpPath,228) > 0 ) {
			strcpy(czLogFile,czTmpPath);
		}
		strcat(czLogFile,"\\REGWIZ.LOG");
		
		os = new ofstream (czLogFile);
		if( os == NULL ){
			os = &cout;
		}
#else
		os = &cout;
#endif

	}
	return *os;
			
}
***/
/*	Value  -- > "CurrentDriveLetterAssignment"		Data -- > "A"
	Value  -- > "Removable"							Data -- > 01
	Value  -- > "Class"								Data -- > "DiskDrive"*/


int RegFindValueInAllSubKey(HINSTANCE hInstance,HKEY key ,LPCTSTR szSubKeyNameToFind,LPCTSTR szValueToFind,LPTSTR szIdentifier,int nType )
{
   DWORD   dwRet,dwIndex,dwSubkeyLen;
   TCHAR   szSubKey[256],szFloppy[256];
   BOOL    bType = FALSE,bRemovable = FALSE,bPrevMassStorage,bPrevFloppy;
   HKEY    hKey;
   static BOOL bMassStorage = FALSE;
   static BOOL bFloppy = FALSE;
	
   bPrevMassStorage =	bMassStorage;
   bPrevFloppy		=	bFloppy;

   if (szSubKeyNameToFind != NULL)
   {
     dwRet=RegOpenKeyEx(
						 key,
						 szSubKeyNameToFind,
						 0,
						 KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
						 &hKey 
						 );

	 if( dwRet == ERROR_SUCCESS)
	 {	 dwIndex = 0;
         while (dwRet == ERROR_SUCCESS )
         {
            dwSubkeyLen = 256;
			dwRet=RegEnumKeyEx(
                           hKey,
                           dwIndex,       
                           szSubKey,
                           &dwSubkeyLen,
                           NULL,
                           NULL,
                           NULL,
                           NULL
                         );
 
            if(dwRet == ERROR_NO_MORE_ITEMS)
            {
				_TCHAR		valueName[80];
				DWORD		valueNameSize,valueSize,n = 0;
				TBYTE		value[80];
				
                do
				{
					valueNameSize=80* sizeof(_TCHAR);
					valueSize=80* sizeof(TBYTE);
					dwRet = RegEnumValue(
										 hKey,
										 n,
										 valueName,
										 &valueNameSize,
										 NULL,
										 NULL,
										 (LPBYTE) value,
										 &valueSize
										 );
					if(dwRet == ERROR_SUCCESS)
					{
						
						if(nType == 1)
						{
							if (!_tcscmp(valueName,_T("Type"))) 
							{
								if (!_tcscmp(szValueToFind,(LPCTSTR)value))
								{
									bType = TRUE;
								}
							}
							if (!_tcscmp(valueName,_T("Identifier"))) 
							{
								_tcscpy(szIdentifier,(LPCTSTR)value);
							}
						}
						else
						if(nType == 2)
						{
							if (!_tcscmp(valueName,_T("Class"))) 
							{
								if (!_tcscmp(szValueToFind,(LPCTSTR)value))
								{
									bType = TRUE;
								}
							}
							if (!_tcscmp(valueName,_T("DeviceDesc"))) 
							{
// bFloppy and bMassStorage are used for handling the conditions when there are multiple 
// Floppy and mass storage media present.
								_tcscpy(szFloppy,(LPCTSTR)value);
								_tcsupr(szFloppy);
								if(_tcsstr(szFloppy,_T("FLOPPY")) != NULL)
								{
									if(!bFloppy)
									{
										_tcscpy(szFloppy,(LPCTSTR)value);
										bFloppy = TRUE;
									}
								}
								else
// if it is not removable or it is a cdrom the condition for type and removable 
// takes care of it.
								{
									if(!bMassStorage)
									{
										bMassStorage = TRUE;
									}
								}

							}
							if (!_tcscmp(valueName,_T("Removable"))) 
							{
								if (*value == 0x01 )
								{
									bRemovable = TRUE;
								}
							}
							/*if (!_tcscmp(valueName,_T("CurrentDriveLetterAssignment"))) 
							{
								_tcscpy(szRemovableTemp,(LPCTSTR)value);
							}*/
							
						}
						n++;
					}

				} while (dwRet == ERROR_SUCCESS);

				if(nType == 1)
				{
					if(bType)
					{
						return REGFIND_FINISH;
					}
					else
					{
						return REGFIND_RECURSE;
					}
				}
				else
				if(nType == 2)
				{

					if(bType && bRemovable )
					{
					/*	_tcscat(szRemovableTemp,_T(":"));
						_tcscat(szRemovableTemp,szIdentifier);
						_tcscpy(szIdentifier,szRemovableTemp);*/
						if(bFloppy != bPrevFloppy )
						{						
							_tcscpy(szIdentifier,szFloppy);	
						}
						if(bFloppy && bMassStorage)
						{
							_TCHAR szMassString[64];
							LoadString(hInstance,IDS_MASS_STRORAGE_ENTRY,szMassString,64);
							_tcscat(szIdentifier,szMassString);	
							return REGFIND_FINISH;
						}
						return REGFIND_RECURSE;
					}
// The bMassStorage flag has to be reset to the previous state. 
					else
					{
						bMassStorage = bPrevMassStorage;
						if(bFloppy != bPrevFloppy)
							bFloppy = bPrevFloppy;
						return REGFIND_RECURSE;
					}
				}            
			}
            else
			{
				if(dwRet == ERROR_SUCCESS)
				{
					int nStatus;
					nStatus = RegFindValueInAllSubKey(hInstance,hKey, szSubKey,szValueToFind,szIdentifier,nType);

					switch(nStatus)
					{
						case REGFIND_FINISH:
							{
								return REGFIND_FINISH;
							}
						case REGFIND_ERROR:
							{
								return REGFIND_ERROR;
							}
						default :
							{
								if(bFloppy != bPrevFloppy)
								  bPrevFloppy = bFloppy;
						
							}
							break;
					}
					
					dwIndex++;
				}
			}
         }
         RegCloseKey(hKey);
      }
	  else
	  {
		  RW_DEBUG << "Error Opening the key " << ConvertToANSIString(szSubKeyNameToFind)<< flush;
	  }
   }
   else
   {
	   RW_DEBUG << "Error: key cannot be NULL" << flush;
   }
 
   return REGFIND_ERROR;
}


BOOL RegFindTheSubKey(HKEY key,LPCTSTR szSubKeyName,LPCTSTR szSubKeyNameToFind,LPTSTR szData )
{
   DWORD   dwRet,dwIndex,dwSubkeyLen;
   TCHAR   szSubKey[256];
   HKEY    hKey;

   if (szSubKeyName != NULL)
   {
     dwRet=RegOpenKeyEx(
						 key,
						 szSubKeyName,
						 0,
						 KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
						 &hKey 
						);

	 if( dwRet == ERROR_SUCCESS)
	 {	 dwIndex = 0;
         while (dwRet == ERROR_SUCCESS )
         {
            dwSubkeyLen = 256;
			dwRet=RegEnumKeyEx(
                           hKey,
                           dwIndex,       
                           szSubKey,
                           &dwSubkeyLen,
                           NULL,
                           NULL,
                           NULL,
                           NULL
                         );
 
            if(dwRet == ERROR_NO_MORE_ITEMS)
            {
					return FALSE;
            }
            else
			{
  			 if(dwRet == ERROR_SUCCESS)
			 {
				if(!_tcscmp(szSubKey,szSubKeyNameToFind))
				{
					if(RegGetPointingDevice(hKey,szSubKey,szData))
					{
						RegCloseKey(hKey);
						return TRUE;
					}
					else
						return FALSE;
				}
				else
				if(RegFindTheSubKey(hKey,szSubKey,szSubKeyNameToFind,szData))
				{
					return TRUE;
				}
			 }
			 dwIndex++;
			}
         }
         RegCloseKey(hKey);
      }
	  else
	  {
		  RW_DEBUG << "Error Opening the key " << ConvertToANSIString(szSubKeyNameToFind) << flush;
	  }
   }
   else
   {
	   RW_DEBUG << "Error: key cannot be NULL"<< flush;
   }
 
   return FALSE;
}


BOOL RegGetPointingDevice(HKEY Key,LPCTSTR szSubKeyName,LPTSTR szData)
{
   DWORD	dwRet,dwSubkeyLen;
   _TCHAR	szSubKey[256];
   _TCHAR	valueName[80];
   DWORD	valueNameSize;
   TBYTE	value[80];
   DWORD	valueSize;
   DWORD	n = 0;
   HKEY		hKey,hKey2;

   dwRet=RegOpenKeyEx(
						 Key,
						 szSubKeyName,
						 0,
						 KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
						 &hKey 
						);

   if( dwRet == ERROR_SUCCESS)
	{
	  dwSubkeyLen = 256;
      dwRet=RegEnumKeyEx(
                           hKey,
                           0,       
                           szSubKey,
                           &dwSubkeyLen,
                           NULL,
                           NULL,
                           NULL,
                           NULL
                         );

	  dwRet=RegOpenKeyEx(
						 hKey,
						 szSubKey,
						 0,
						 KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
						 &hKey2 
						);
      do
		{
			valueNameSize=80 * sizeof(TCHAR);
			valueSize=80 * sizeof(TCHAR);
		
			dwRet = RegEnumValue(
								 hKey2,
								 n,
								 valueName,
								 &valueNameSize,
								 NULL,
								 NULL,
								 (LPBYTE) value,
								 &valueSize
								 );
			if(dwRet == ERROR_SUCCESS)
			{
				if (!_tcscmp(valueName,_T("Identifier"))) 
				{
					_tcscpy(szData,(LPCTSTR)value);
			        RegCloseKey(hKey2);
				    RegCloseKey(hKey);
					return TRUE;
				}
				n++;
			}
		} while (dwRet == ERROR_SUCCESS);
   }
   RegCloseKey(hKey2);
   RegCloseKey(hKey);
   return FALSE;
}


//
// This function Converts  an UNICODE to String
//
//
LPCTSTR BstrToSz(BSTR pszW)
{
#ifndef _UNICODE 
	ULONG cbAnsi, ulNoOfChars;
	DWORD dwError;
	LPTSTR lpString;
	if(pszW== NULL) 
		return NULL;
	ulNoOfChars = wcslen(pszW)+1;
	cbAnsi =   ulNoOfChars +2;
	lpString = (LPTSTR) CoTaskMemAlloc(cbAnsi);
	if( NULL == lpString )
		return NULL;
	if( WideCharToMultiByte(CP_ACP,0,pszW,ulNoOfChars,lpString,cbAnsi,NULL,NULL) == 0) {
		dwError = GetLastError();
		CoTaskMemFree(lpString);
		lpString = NULL;
	}
	return lpString;
#endif
	return (LPCTSTR)  pszW;


}


int  GetProductRoot (LPTSTR pPath , PHKEY  phKey)
{

    LONG regStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
		pPath,0,KEY_READ |KEY_WRITE,phKey);
	if (regStatus != ERROR_SUCCESS) {
		return 1; // error
	}
	else{
		return 0; // Success 
	}

}


//
//  This function converts an UNICODE STRING to ANSI char String
//
//
char * ConvertToANSIString (LPCTSTR   pszW)
{

#ifdef _UNICODE 
	static char szAString[STRCONVERT_MAXLEN];
	ULONG cbAnsi, ulNoOfChars;
	DWORD dwError;
	LPSTR lpString;
	if(pszW == NULL) 
		return NULL;
	if(*pszW == 0) {
		return NULL;

	}
	ulNoOfChars = wcslen(pszW)+1;
	cbAnsi =   ulNoOfChars +2;
	lpString = (LPSTR) szAString;
	if( WideCharToMultiByte(CP_ACP,0,pszW,ulNoOfChars,lpString,STRCONVERT_MAXLEN,NULL,NULL) == 0) {
		dwError = GetLastError();
		lpString = NULL;
	}
	return lpString;
#else
	return (LPTSTR)  pszW;
#endif
 
}



#ifdef _UNICODE

TCHAR* ConvertToUnicode(char FAR* szA)
{
  static TCHAR  achW[STRCONVERT_MAXLEN]; 
  MultiByteToWideChar(CP_ACP, 0, szA, -1, achW, STRCONVERT_MAXLEN);  
  return achW; 
}

#else
//
//  in case of 
TCHAR * ConvertToUnicode(TCHAR * szW) 
{
	return szW;
}
#endif

TCHAR * GetProductBeingRegistred()
{
	return  gszProductBeingRegistred;
}

void SetProductBeingRegistred(TCHAR *szProduct)
{
	_tcscpy (gszProductBeingRegistred,szProduct);
}

void GetWindowsDirectory(TCHAR *szParamRegKey,
						 TCHAR *czBuf)
{
	HKEY hKey;
	TCHAR szRetVal[128];
	DWORD dwSize= 128;
	LONG regStatus ;

	_tcscpy(czBuf,_T("")); // Set Path to Blank


	GetProductRoot(szParamRegKey , &hKey);
	if(!hKey) {
		return ;
	}
	regStatus = RegQueryValueEx(hKey,
		_T("SystemRoot"),
		NULL,
		0,
		(LPBYTE) szRetVal,
		&dwSize);
	if (regStatus == ERROR_SUCCESS){
		// Verifty the Value 
		//
		_tcscpy(czBuf,szRetVal);
	}
	RegCloseKey(hKey);
	
}


/*
	Returns 1 if successful
	        0 if failure
*/
int GetOemManufacturer (TCHAR *szProductRegKey, TCHAR *szBuf )
{ 
	TCHAR szSection[] = _T("general");
	TCHAR szKeyName[] = _T("Manufacturer");
	TCHAR szOemFile[MAX_PATH] = _T("");
	
	GetSystemDirectory(szOemFile,MAX_PATH);
	_tcscat(szOemFile,_T("\\Oeminfo.ini"));

	if( CheckOEMdll()== OEM_NO_ERROR ) {
		GetPrivateProfileString (szSection,
							 szKeyName,
							 _T(""),
							 szBuf, 
							 256,
							 szOemFile
							 );
		return 1;
	}
	else {
		return 0;
	}
}

void SetMSID(HINSTANCE hInstance)
{
 
 DWORD	dwRet;
 _TCHAR szKeyName[256];
 HKEY	hIDKey;
 TCHAR  szMSID[256];
 
 
 if(!GetMSIDfromCookie(hInstance,szMSID)){
	 szMSID[0] = _T('\0');
 }
 else {
	
	_TCHAR szPartialKey[256];
	int resSize = LoadString(hInstance,IDS_KEY2,szKeyName,255);
	_tcscat(szKeyName,_T("\\"));
	resSize = LoadString(hInstance,IDS_KEY3,szPartialKey,255);
	_tcscat(szKeyName,szPartialKey);
	_tcscat(szKeyName,_T("\\"));
	resSize = LoadString(hInstance,IDS_KEY4,szPartialKey,255);
	_tcscat(szKeyName,szPartialKey);

	dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKeyName,0,KEY_ALL_ACCESS,&hIDKey);
	if (dwRet == ERROR_SUCCESS) {
		// Store MSID into
		dwRet = RegSetValueEx(hIDKey,_T("MSID"),NULL,REG_SZ,(CONST BYTE *)szMSID,
								_tcslen((LPCTSTR)szMSID)*sizeof(TCHAR) );
		RegCloseKey(hIDKey);
	}

 }
}

void RemoveMSIDEntry(HINSTANCE hInstance)
{
    DWORD	dwRet;		
	 HKEY	hIDKey;
	_TCHAR szPartialKey[256];
	_TCHAR szValue[256];
	 _TCHAR szKeyName[256];
	int resSize = LoadString(hInstance,IDS_KEY2,szKeyName,255);
	_tcscat(szKeyName,_T("\\"));
	resSize = LoadString(hInstance,IDS_KEY3,szPartialKey,255);
	_tcscat(szKeyName,szPartialKey);
	_tcscat(szKeyName,_T("\\"));
	resSize = LoadString(hInstance,IDS_KEY4,szPartialKey,255);
	_tcscat(szKeyName,szPartialKey);
	szValue[0] = _T('\0');
	resSize = LoadString(hInstance,IDS_NOTUSED,szValue,255);

	dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szKeyName,0,KEY_ALL_ACCESS,&hIDKey);
	if (dwRet == ERROR_SUCCESS){
		// Store MSID into
		dwRet = RegSetValueEx(hIDKey,_T("MSID"),NULL,REG_SZ,(CONST BYTE *)szValue,
								_tcslen((LPCTSTR)szValue)* sizeof(_TCHAR));
		RegCloseKey(hIDKey);
	}
	// 
	// Set in Current User
	_tcscpy(szKeyName,_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"));
	dwRet = RegOpenKeyEx(HKEY_CURRENT_USER,szKeyName,0,KEY_ALL_ACCESS,&hIDKey);
	if (dwRet == ERROR_SUCCESS){
		// Store MSID into
		dwRet = RegSetValueEx(hIDKey,_T("MSID"),NULL,REG_SZ,(CONST BYTE *)szValue,
								_tcslen((LPCTSTR)szValue)* sizeof(_TCHAR));
		RegCloseKey(hIDKey);
	}


}

BOOL GetMSIDfromCookie(HINSTANCE hInstance,LPTSTR szMSID)
{	
	_TCHAR szCookieInfo[256],szRegisterSiteURL[256],szTmpURL1[256];
	_TCHAR szTmpURL2[256];
	DWORD dwSize = 256;
	
	_tcscpy(szTmpURL1,_T("http://"));
	LoadString(hInstance, IDS_HTTP_SERVER,szRegisterSiteURL, 255);
	_tcscat(szTmpURL1,szRegisterSiteURL);
	LoadString(hInstance, IDS_HTTP_SERVER_PATH,szTmpURL2, 255);
	
	InternetCombineUrl(	szTmpURL1,	szTmpURL2,	szRegisterSiteURL,
									&dwSize,	ICU_DECODE );

	dwSize = 256;
	//
	
	BOOL bRet = ATK_InternetGetCookie(szRegisterSiteURL,
		_T(""),szCookieInfo,&dwSize );
   
	if(bRet)
	{
		if(szCookieInfo[0] != _T('\0')){
			RW_DEBUG <<"\nAfterInternetGetCookie "  << ConvertToANSIString(szCookieInfo) << flush;
		}

		BOOL bfound = FALSE;

		_TCHAR seps[] = _T("=");
		_TCHAR *token;

		token = _tcstok( szCookieInfo, seps );
	
	    while( token != NULL )
		{
		  if(!_tcscmp(token,_T("GUID")))
		  {
			  bfound = TRUE;
			  break;
		  }
	      token = _tcstok( NULL, seps );
		}

		if(bfound)
		{
		  token = _tcstok( NULL, seps );
		  _tcscpy(szMSID,token);
		 RW_DEBUG <<"\n Cookie Found " << ConvertToANSIString(szMSID) << flush;
		  return TRUE;
	    }
		else
		{
			_tcscpy(szMSID,_T(""));
		}
	}
	else
	{
		_tcscpy(szMSID,_T(""));
	}
	return FALSE;
}

BOOL GetMSIDfromRegistry(HINSTANCE hInstance,LPTSTR szValue)
{
	HKEY hKeyMSID;

	_TCHAR szKeyName[256];
	
	_tcscpy(szKeyName,_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"));
	
	LONG regStatus = RegOpenKeyEx(HKEY_CURRENT_USER,szKeyName,0,KEY_READ,&hKeyMSID);
	if (regStatus == ERROR_SUCCESS)
	{
		_TCHAR szValueName[64];
		unsigned long infoSize = 255;
		LoadString(hInstance,IDS_MSID,szValueName,64);
		regStatus = RegQueryValueEx(hKeyMSID,szValueName,NULL,0,(LPBYTE) szValue,&infoSize);
		if (regStatus == ERROR_SUCCESS)
		{
			RegCloseKey(hKeyMSID);
			RW_DEBUG <<"\n GetCookie from Registry "  << ConvertToANSIString(szValue) << flush;
			return TRUE;
		}
	}
	RegCloseKey(hKeyMSID);
	return FALSE;
}
