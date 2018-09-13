/********************************************************************************
/	This is the base file to the Microsoft JScript Proxy Configuration 
/	This file implements the code to provide the script site and the JSProxy psuedo
/	object for the script engine to call against.
/
/	Created		11/27/96	larrysu
/
/
/
/
/
/
/
/
/
*/

#include "dllmain.h"
#ifndef unix
// Avoid operator new multiple defines on unix
#include "crtfree.h"
#endif /* unix */
CScriptSite	*g_ScriptSite = NULL;
BOOL fOleInited = FALSE;

/*******************************************************************************
*	dll initialization and destruction

********************************************************************************/
EXTERN_C
BOOL APIENTRY DllMain(HMODULE hModule,DWORD ul_reason_for_call,LPVOID lpReserved)
{
    
	switch( ul_reason_for_call ) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
		break;
    case DLL_PROCESS_DETACH:		
		break;
    }
    return TRUE;
}


BOOL CALLBACK InternetInitializeAutoProxyDll(DWORD dwVersion, 
							   LPSTR lpszDownloadedTempFile,
							   LPSTR lpszMime,
							   AUTO_PROXY_HELPER_APIS *pAutoProxyCallbacks, 
							   LPAUTO_PROXY_EXTERN_STRUC lpExtraData)
{
	HRESULT	hr = S_OK;
	HANDLE	hFile = 0;
	LPSTR	szScript = NULL;
	DWORD	dwFileSize = 0;
	DWORD	dwBytesRead = 0;
	LPSTR	result;
    LPSTR   szAllocatedScript = NULL;


    if ( !fOleInited ) 
    {
#ifndef unix
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
#else
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#endif /* unix */
    }


	// get the script text from the downloaded file!
	// open the file

    if ( lpExtraData == NULL ||
         lpExtraData->dwStructSize != sizeof(AUTO_PROXY_EXTERN_STRUC) ||
         lpExtraData->lpszScriptBuffer == NULL )
    {
	    if (!lpszDownloadedTempFile)
		    return FALSE;

	    hFile = CreateFile((LPCSTR)lpszDownloadedTempFile,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	    if (hFile == INVALID_HANDLE_VALUE)
		    return FALSE;

	    // Get the size
	    dwFileSize = GetFileSize(hFile,NULL);
	    // allocate the buffer to hold the data.
	    szScript = (LPSTR) GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,dwFileSize+1);
        szAllocatedScript = szScript;
	    // if the memory was allocated
	    if (szScript)
	    {
		    // read the data
		    if (ReadFile(hFile,(LPVOID) szScript,dwFileSize,&dwBytesRead,NULL))
			    // close the file
			    CloseHandle(hFile);
		    else
			    return FALSE;
	    }
    }
    else
    {
        szScript = (LPSTR) lpExtraData->lpszScriptBuffer;
    }

	// Create a new CScriptSite object and initiate it with the autoconfig script.
	g_ScriptSite = new CScriptSite;
	if (g_ScriptSite)
		hr = g_ScriptSite->Init(pAutoProxyCallbacks, szScript);
	else
		hr = E_OUTOFMEMORY;

    if ( szAllocatedScript ) 
    {
	    // Free the script text
	    GlobalFree(szAllocatedScript);
        szAllocatedScript = NULL;
    }

	if (SUCCEEDED(hr))
		return TRUE;
	else
		return FALSE;
}

// This function frees the script engine and destroys the script site.
BOOL CALLBACK InternetDeInitializeAutoProxyDll(LPSTR lpszMime,DWORD dwReserved)
{

	// Release and destroy the CScriptSite object and initiate it with the autoconfig script.
	// DeInit the script site.
	if (g_ScriptSite)
	{
		g_ScriptSite->DeInit();
		g_ScriptSite->Release();
		g_ScriptSite = NULL;
	}

    if ( fOleInited ) 
    {
        CoUninitialize();    
    }

	return TRUE;
}

// This function is called when the host wants to run the script.
BOOL CALLBACK InternetGetProxyInfo(LPCSTR lpszUrl,
								   DWORD dwUrlLength,
								   LPSTR lpszUrlHostName,
								   DWORD dwUrlHostNameLength,
								   LPSTR *lplpszProxyHostName,
								   LPDWORD lpdwProxyHostNameLength)
{
	HRESULT	hr = S_OK;
	LPSTR	szHost;
	
	// The host passed in may be too big.  Copy it an make the 
	// HostLength + 1 position will be slammed with \0.
	szHost = (LPSTR) GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,dwUrlHostNameLength+1);
	if (!szHost)
		return FALSE;
	if(!lstrcpyn(szHost,lpszUrlHostName,dwUrlHostNameLength+1))
	{
		GlobalFree(szHost);
		return FALSE;
	}

	// construct a jscript call with the passed in url and host.
	if (g_ScriptSite)
//		hr = g_ScriptSite->RunScript(lpszUrl,lpszUrlHostName,lplpszProxyHostName);
		hr = g_ScriptSite->RunScript(lpszUrl,szHost,lplpszProxyHostName);

	GlobalFree(szHost);

	if (SUCCEEDED(hr))
	{
		*lpdwProxyHostNameLength = lstrlen(*lplpszProxyHostName) +1;
		return TRUE;
	}
	else
		return FALSE;
}
