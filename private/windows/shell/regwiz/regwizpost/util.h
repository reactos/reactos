/**************************************************************************

   File:          util.h
   
   Description:   

**************************************************************************/
#include <tchar.h>

#ifdef __cplusplus
extern "C" 
{
#endif

	BOOL GetRegKeyValue(HINSTANCE hInstance, HKEY hRootKey, LPTSTR szBaseKey,int valueStrID, LPTSTR szValue);
	void DisplayMessage(LPCSTR szMessage, LPCSTR szFormat=0);
	BOOL Succeeded(HANDLE h, LPCSTR strFunctionName);
	BOOL Succeeded1(BOOL h, LPCSTR strFunctionName);

#ifdef __cplusplus
}
#endif

