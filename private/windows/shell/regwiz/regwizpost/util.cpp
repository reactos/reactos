

#include <windows.h>
#include <stdio.h>
#include "util.h"
#include "rw_common.h"



/*********************************************************************
Looks for a subkey, under the Registration Database key given in the
szBaseKey parameter, of the form "0000", "0001", etc.  The numerical
equivalent of the subkey is determined by the index value given in
the enumIndex parameter.  The value attached to the valueName
specified in the string resource whose ID is given in valueStrID will
be returned in szValue.

Returns: FALSE if the key specified is not found. 
**********************************************************************/
BOOL GetRegKeyValue(HINSTANCE hInstance, HKEY hRootKey, LPTSTR szBaseKey,int valueStrID, LPTSTR szValue)
{
	BOOL returnVal = FALSE;
	HKEY hKey;
	LONG regStatus = RegOpenKeyEx(hRootKey, szBaseKey, 0, KEY_READ,&hKey);
	if (regStatus == ERROR_SUCCESS)
	{
		_TCHAR szValueName[128];
		//LoadString(hInstance,valueStrID,szValueName,128);
		_tcscpy(szValueName, _T("InternetProfile"));

		unsigned long infoSize = 255;
		//regStatus = RegQueryValueEx(hKey, szValueName, NULL, 0, (unsigned char*) szValue, &infoSize);
		regStatus = RegQueryValueEx(hKey, szValueName, NULL, 0, (LPBYTE) szValue, &infoSize);
		if (regStatus == ERROR_SUCCESS)
		{
			returnVal = TRUE;
		}
		RegCloseKey(hKey);
	}
	return returnVal;
}


void DisplayMessage(LPCSTR szMessage, LPCSTR szFormat) 
{
#ifdef _LOG_IN_FILE
   if (szFormat)
   {
      DWORD dwError = GetLastError() ;
      CHAR errString[1024] ;
	  sprintf(errString, szFormat, szMessage);
	  RW_DEBUG << "\n " << errString << flush;
   }
   else
   {
	   RW_DEBUG << "\n" << szMessage << flush;
   }
#endif
}

BOOL Succeeded1(BOOL h, LPCSTR strFunctionName)
{
   if (h == FALSE)
   {
	  char errString[1024] ;
	  sprintf(errString, "%s returns error %u",
				strFunctionName, GetLastError());

	  #ifdef _LOG_IN_FILE
		RW_DEBUG << "\n Succeeded " << errString << flush;
	  #endif 

      return FALSE;
	
   }
   else
   {
      return TRUE ;
   }
}

BOOL Succeeded(HANDLE h, LPCSTR strFunctionName)
{
   if (h == NULL)
   {
	  char errString[1024] ;
      sprintf(errString, "%s returns error %u", 
				strFunctionName, GetLastError());

	  #ifdef _LOG_IN_FILE
		RW_DEBUG << "\n Succeeded " << errString << flush;
	  #endif 

      return FALSE;
   }
   else
   {
      return TRUE ;
   }
}
