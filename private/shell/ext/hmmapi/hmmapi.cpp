#include "pch.hxx"
#include <winuser.h>
#include <hlink.h>
#include <shellapi.h>
#define INITGUID
#include <initguid.h>
#include <exdisp.h>
#include <tchar.h>

#define USENAVIGATE     1
#define USEATTFILE		1
#define USEPOST			0

#define ARRAYSIZE(buf) (sizeof(buf) / sizeof(buf[0]))

HINSTANCE       g_hInstMAPI = NULL;

////////////////////////////////////////////////////////////////////////
//
//  dll entry point
//
////////////////////////////////////////////////////////////////////////
STDAPI_(BOOL) APIENTRY DllMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpRsrvd)
{
    switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			g_hInstMAPI = hDll;
			break;

		case DLL_PROCESS_DETACH:
			break;
	} // switch
    return(TRUE);
}

BOOL FRunningOnNTEx(LPDWORD pdwVersion)
{
    static BOOL fIsNT = 2 ;
    static DWORD dwVersion = (DWORD)0;
    OSVERSIONINFO VerInfo;
    
    // If we have calculated this before just pass that back.
    // else find it now.
    //
    if (fIsNT == 2)
    {
        VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        
        GetVersionEx(&VerInfo);
        // Also, we don't check for failure on the above call as it
        // should succeed if we are on NT 4.0 or Win 9X!
        //
        fIsNT = (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
        if (fIsNT)
            dwVersion = VerInfo.dwMajorVersion;
    }
    if (pdwVersion)
        *pdwVersion = dwVersion;
    
    return fIsNT;
}
// Then next 2 functions are stollen from shlwapi. Needed to modiy them, because
// we had to handle SFN.
// Also there is a bug in the Ansi versin of ExpandEnvironmentStrings, where the
// function returns the number of bytes the string would have if it would be
// UNICODE. Since we have to convert the string anyway to SFN I use lstrlen to 
// get the real length.
//
//  If the given environment variable exists as the first part of the path,
//  then the environment variable is inserted into the output buffer.
//
//  Returns TRUE if pszResult is filled in.
//
//  Example:  Input  -- C:\WINNT\SYSTEM32\FOO.TXT -and- lpEnvVar = %SYSTEMROOT%
//            Output -- %SYSTEMROOT%\SYSTEM32\FOO.TXT
//
BOOL MyUnExpandEnvironmentString(LPCTSTR pszPath, LPCTSTR pszEnvVar, LPTSTR pszResult, UINT cbResult)
{
    TCHAR szEnvVar[MAX_PATH];
    DWORD dwEnvVar = SHExpandEnvironmentStrings(pszEnvVar, szEnvVar, ARRAYSIZE(szEnvVar));

    if (dwEnvVar)
    {
        // Convert the string to short file name
        GetShortPathName(szEnvVar, szEnvVar, ARRAYSIZE(szEnvVar));
        dwEnvVar = lstrlen(szEnvVar);
        if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, szEnvVar, dwEnvVar, pszPath, dwEnvVar) == 2)
        {
            if (lstrlen(pszPath) - (int)dwEnvVar + lstrlen(pszEnvVar) < (int)cbResult)
            {
                lstrcpy(pszResult, pszEnvVar);
                lstrcat(pszResult, pszPath + dwEnvVar);
                return TRUE;
            }
        }
    }
    return FALSE;
}


// note: %USERPROFILE% is relative to the user making the call, so this does
// not work if we are being impresonated from a service, for example
// dawrin installs apps from the system process this way
STDAPI_(BOOL) MyPathUnExpandEnvStrings(LPCTSTR pszPath, LPTSTR pszBuf, UINT cchBuf)
{
    if (pszPath && pszBuf)
    {
        return (MyUnExpandEnvironmentString(pszPath, TEXT("%USERPROFILE%"), pszBuf, cchBuf)       ||
                MyUnExpandEnvironmentString(pszPath, TEXT("%ALLUSERSPROFILE%"), pszBuf, cchBuf)   ||
                MyUnExpandEnvironmentString(pszPath, TEXT("%ProgramFiles%"), pszBuf, cchBuf)      ||
                MyUnExpandEnvironmentString(pszPath, TEXT("%SystemRoot%"), pszBuf, cchBuf)        ||
                MyUnExpandEnvironmentString(pszPath, TEXT("%SystemDrive%"), pszBuf, cchBuf));
    }
    else
    {
        return FALSE;
    }
}



static void GetPostUrl(LPSTR lpszData, DWORD dwSize)
{
	HKEY hkDefClient;
	HKEY hkClient;
	TCHAR szClient[64];
	DWORD type;
	DWORD dwClientSize = sizeof(TCHAR) * 64;

	LONG err = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Clients\\Mail"), &hkClient);
	if (err == ERROR_SUCCESS)
	{
		err = RegQueryValueEx(hkClient, NULL, 0, &type, (LPBYTE)szClient, &dwClientSize);
		if (err == ERROR_SUCCESS)
		{
			err = RegOpenKey(hkClient, szClient, &hkDefClient);
			if (err == ERROR_SUCCESS)
			{
				DWORD type;
				err = RegQueryValueEx(hkDefClient, TEXT("posturl"), 0, &type, (LPBYTE)lpszData, &dwSize);
				RegCloseKey(hkDefClient);
			}
		}
		RegCloseKey(hkClient);
	}
	if (err != ERROR_SUCCESS)
    {
        LoadString(g_hInstMAPI, IDS_DEFAULTPOSTURL, lpszData, dwSize);
    }
}

typedef HRESULT (STDAPICALLTYPE DynNavigate)(DWORD grfHLNF, LPBC pbc, IBindStatusCallback *pibsc,
							 LPCWSTR pszTargetFrame, LPCWSTR pszUrl, LPCWSTR pszLocation);
typedef DynNavigate FAR *LPDynNavigate;

STDAPI HlinkFrameNavigateNHL(DWORD grfHLNF, LPBC pbc, IBindStatusCallback *pibsc,
							 LPCWSTR pszTargetFrame, LPCWSTR pszUrl, LPCWSTR pszLocation)
{
	HRESULT hr;
    HINSTANCE   hinst;
	LPDynNavigate fpNavigate = NULL;

	hinst = LoadLibraryA("SHDOCVW.DLL");

    // If that failed because the module was not be found,
    // then try to find the module in the directory we were
    // loaded from.

    if (!hinst)
	goto Error;

    fpNavigate = (LPDynNavigate)GetProcAddress(hinst, "HlinkFrameNavigateNHL");
    if (!fpNavigate)
	goto Error;

    hr = fpNavigate(grfHLNF, pbc, pibsc, pszTargetFrame, pszUrl, pszLocation);

	FreeLibrary(hinst);
	return hr;

Error:
    return GetLastError();
}

static void SimpleNavigate(LPTSTR lpszUrl, BOOL bUseFrame = false)
{
    DWORD cch = (lstrlen(lpszUrl) + 1);
	LPWSTR pwszData = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cch * sizeof(WCHAR));
    if (pwszData)
    {
        SHTCharToUnicode(lpszUrl, pwszData, cch);
        if (bUseFrame)
            HlinkFrameNavigateNHL(HLNF_OPENINNEWWINDOW, NULL, NULL, NULL, pwszData, NULL);
        else
            HlinkSimpleNavigateToString(pwszData, NULL, NULL, NULL, NULL, NULL, 0, 0);
        HeapFree(GetProcessHeap(), 0, (LPVOID)pwszData);
    }
}

// Pack some data into a SAFEARRAY of BYTEs. Return in a VARIANT
static HRESULT GetPostData(LPVARIANT pvPostData, LPTSTR lpszData)
{
	HRESULT hr;
	LPSAFEARRAY psa;
	UINT cElems = lstrlen(lpszData);
	LPSTR pPostData;

	if (!pvPostData)
		return E_POINTER;

	VariantInit(pvPostData);

	psa = SafeArrayCreateVector(VT_UI1, 0, cElems);
	if (!psa)
		return E_OUTOFMEMORY;

	hr = SafeArrayAccessData(psa, (LPVOID*)&pPostData);
	memcpy(pPostData, lpszData, cElems);
	hr = SafeArrayUnaccessData(psa);

	V_VT(pvPostData) = VT_ARRAY | VT_UI1;
	V_ARRAY(pvPostData) = psa;
	return NOERROR;
}

static void DoNavigate(LPTSTR lpszUrl, LPTSTR lpszData, BOOL bPlainIntf = TRUE)
{
	HRESULT hr;
	IWebBrowser2* pWBApp = NULL; // Derived from IWebBrowser
	BSTR bstrURL = NULL, bstrHeaders = NULL;
	VARIANT vFlags = {0};
	VARIANT vTargetFrameName = {0};
	VARIANT vPostData = {0};
	VARIANT vHeaders = {0};
	LPWSTR pwszData = NULL;
	LPTSTR pszUrl = NULL;
    DWORD cch;

	if (FAILED(hr = CoInitialize(NULL)))
		return;

	if (FAILED(hr = CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_SERVER, IID_IWebBrowser2, (LPVOID*)&pWBApp)))
		goto Error;

#if USEPOST
    cch = lstrlen(lpszUrl) + 1;
	pwszData = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cch * sizeof(WCHAR));
	if (!pwszData)
		goto Error;

    SHTCharToUnicode(lpszUrl, pwszData, cch);
	bstrURL = SysAllocString(pwszData);
	HeapFree(GetProcessHeap(), 0, (LPVOID)pwszData);
	if (!bstrURL)
		goto Error;

	bstrHeaders = SysAllocString(L"Content-Type: application/x-www-form-urlencoded\r\n");
	if (!bstrHeaders)
		goto Error;

	V_VT(&vHeaders) = VT_BSTR;
	V_BSTR(&vHeaders) = bstrHeaders;

	hr = GetPostData(&vPostData, lpszData);
	hr = pWBApp->Navigate(bstrURL, &vFlags, &vTargetFrameName, &vPostData, &vHeaders);
#else
    cch = lstrlen(lpszUrl) + lstrlen(lpszData) + 2;
	pszUrl = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cch * sizeof(TCHAR));
	if (!pszUrl)
		goto Error;
	lstrcpy(pszUrl, lpszUrl);
	lstrcat(pszUrl, "?");
	lstrcat(pszUrl, lpszData);
	cch = lstrlen(pszUrl) + 1;
	pwszData = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cch * sizeof(WCHAR));
	if (!pwszData)
	{
		HeapFree(GetProcessHeap(), 0, (LPVOID)pszUrl);
		goto Error;
	}

    SHTCharToUnicode(pszUrl, pwszData, cch);
	HeapFree(GetProcessHeap(), 0, (LPVOID)pszUrl);
	bstrURL = SysAllocString(pwszData);
	HeapFree(GetProcessHeap(), 0, (LPVOID)pwszData);
	if (!bstrURL)
		goto Error;

	hr = pWBApp->Navigate(bstrURL, &vFlags, &vTargetFrameName, &vPostData, &vHeaders);
#endif
	if (bPlainIntf)
	{
		pWBApp->put_AddressBar(VARIANT_FALSE);
		pWBApp->put_MenuBar(VARIANT_FALSE);
		pWBApp->put_ToolBar(VARIANT_FALSE);
	}
	pWBApp->put_Visible(VARIANT_TRUE);

Error:
	if (bstrURL)
		SysFreeString(bstrURL);
	if (bstrHeaders)
		SysFreeString(bstrHeaders);
	VariantClear(&vPostData);
	if (pWBApp)
		pWBApp->Release();
	CoUninitialize();
}

// Helpers for Form Submit - copied from IE3 and modified approriately
//
static char x_hex_digit(int c)
{
    if (c >= 0 && c <= 9)
    {
	return c + '0';
    }
    if (c >= 10 && c <= 15)
    {
	return c - 10 + 'A';
    }
    return '0';
}

static const unsigned char isAcceptable[96] =
/*   0 1 2 3 4 5 6 7 8 9 A B C D E F */
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0,    /* 2x   !"#$%&'()*+,-./  */
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,    /* 3x  0123456789:;<=>?  */
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 4x  @ABCDEFGHIJKLMNO  */
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,    /* 5x  PQRSTUVWXYZ[\]^_  */
 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 6x  `abcdefghijklmno  */
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};   /* 7x  pqrstuvwxyz{\}~  DEL */

// Performs URL-encoding of null-terminated strings. Pass NULL in pbOut
// to find buffer length required. Note that '\0' is not written out.

// 2/9/99  cchLimit param added for safety -- no more than cchLimit chars are
// written out.  If pbOut is NULL then cchLimit is ignored.  If the caller uses 
// the style Buffer[URLEncode(Buffer, ...)] = 0, then cchLimit should be the 
// buffer size minus one.
   
int
URLEncode(LPTSTR pbOut, const char * pchIn, const int cchLimit)
{
    int     lenOut = 0;
    char *  pchOut = (char *)pbOut;

    for (; *pchIn && (!pchOut || lenOut < cchLimit); pchIn++, lenOut++)
    {
        if (*pchIn == ' ')
        {
            if (pchOut)
                *pchOut++ = '+';
        }
        else if (*pchIn >= 32 && *pchIn <= 127 && isAcceptable[*pchIn - 32])
        {
            if (pchOut)
                *pchOut++ = (TCHAR)*pchIn;
        }
        else
        {
            if (pchOut) 
            {
                if (lenOut <= cchLimit - 3)
                {
                    // enough room for this encoding
                    *pchOut++ = '%';
                    *pchOut++ = x_hex_digit((*pchIn >> 4) & 0xf);
                    *pchOut++ = x_hex_digit(*pchIn & 0xf);
                    lenOut += 2; 
                }
                else
                    return lenOut;
            }
            else
                lenOut += 2; // for expression handles 3rd inc  
        }
    }

    return lenOut;
}


///////////////////////////////////////////////////////////////////////
//
// MAPILogon
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPILogon(ULONG ulUIParam,
			   LPSTR lpszProfileName,
			   LPSTR lpszPassword,
			   FLAGS flFlags,
			   ULONG ulReserved,
			   LPLHANDLE lplhSession)
{
	*lplhSession = 1;
	return SUCCESS_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
//
// MAPILogoff
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPILogoff(LHANDLE lhSession,
			    ULONG ulUIParam,
			    FLAGS flFlags,
			    ULONG ulReserved)
{
	return SUCCESS_SUCCESS;
}


///////////////////////////////////////////////////////////////////////
//
// MAPIFreeBuffer
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPIFreeBuffer(LPVOID lpv)
{
	return MAPI_E_FAILURE;
}


///////////////////////////////////////////////////////////////////////
//
// MAPISendMail
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPISendMail(LHANDLE lhSession,          
                  ULONG ulUIParam,
                  lpMapiMessage lpMessage,
                  FLAGS flFlags,
                  ULONG ulReserved)
{
    TCHAR szUrl[256];

    GetPostUrl(szUrl, sizeof(TCHAR) * 256);
    
    // Calculate the buffer size needed to create the url
    ULONG i;
#if USENAVIGATE
    DWORD dwUrlSize = 32; // "?action=compose" + slop
#else
    DWORD dwUrlSize = lstrlen(szUrl) + 32; // "?action=compose" + slop
#endif
    DWORD dwMaxSize = 0;
    DWORD dwSize;
    DWORD dwFileSizes = 0;
    HANDLE hFile;

    if (lpMessage->lpszSubject)
    {
        dwSize = URLEncode(NULL, lpMessage->lpszSubject, 0);
        dwMaxSize = max(dwMaxSize, dwSize + 1);
        dwUrlSize += dwMaxSize + 9; // "&subject=%s"
    }
    if (lpMessage->lpszNoteText)
    {
        dwSize = URLEncode(NULL, lpMessage->lpszNoteText, 0);
        dwMaxSize = max(dwMaxSize, dwSize + 1);
        dwUrlSize += dwSize + 6; // "&body=%s"
    }

    for (i = 0; i < lpMessage->nRecipCount; i++)
    {
        dwSize = URLEncode(NULL, lpMessage->lpRecips[i].lpszName, 0);
        dwMaxSize = max(dwMaxSize, dwSize + 1);
        dwUrlSize += dwSize + 4; // "&to=%s" || "&cc=%s"
        if (lpMessage->lpRecips[i].ulRecipClass == MAPI_BCC)
            dwUrlSize++; // extra character for bcc
    }

    if (lpMessage->nFileCount)
    {
        dwUrlSize += 14; // "&filecount=xxx"
        for (i = 0; i < lpMessage->nFileCount; i++)
        {
            if (!lpMessage->lpFiles[i].lpszPathName)
                continue;

            TCHAR szFileSize[32];

            hFile = CreateFile(lpMessage->lpFiles[i].lpszPathName, 0 /*GENERIC_READ*/, 0 /*FILE_SHARE_READ*/, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE)
            {
                LPVOID lpMsgBuf;
                FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
                MessageBox(NULL, (char*)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION);
                LocalFree(lpMsgBuf);
                continue;
            }
            dwSize = GetFileSize(hFile, NULL);
            CloseHandle(hFile);
            if (dwSize == -1)
                continue;
            dwFileSizes += dwSize;
            wnsprintf(szFileSize, ARRAYSIZE(szFileSize), "&size%d=%d", i, dwSize);
            dwSize = lstrlen(szFileSize);
            dwMaxSize = max(dwMaxSize, dwSize + 1);
            dwUrlSize += dwSize;


            dwSize = URLEncode(NULL, lpMessage->lpFiles[i].lpszPathName, 0) + 4;    // in case we need to append a ^
            dwMaxSize = max(dwMaxSize, dwSize + 1);
            dwUrlSize += dwSize + 9; // "&pathxxx=%s"

            if (lpMessage->lpFiles[i].lpszFileName)
            {
                dwSize = URLEncode(NULL, lpMessage->lpFiles[i].lpszFileName, 0);
                dwMaxSize = max(dwMaxSize, dwSize + 1);
                dwUrlSize += dwSize + 9; // "&filexxx=%s"
            }
#if USEATTFILE
            else 
            {
                // ATTFILE code further down just tacks on the path when lpszFileName is NULL
                dwUrlSize += URLEncode(NULL, lpMessage->lpFiles[i].lpszPathName, 0) + 4;
            }
#endif
        }
    }

#if USEATTFILE
    dwSize = ARRAYSIZE("&attfile=") + (URLEncode(NULL, "::", 0) * lpMessage->nFileCount * 3);
#else
    dwSize = ARRAYSIZE("&filecount=xxx");
#endif
    dwMaxSize = max(dwMaxSize, dwSize + 1);
    dwUrlSize += dwSize;

    LPTSTR pszData = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwUrlSize * sizeof(TCHAR));

    if (!pszData)
        return MAPI_E_FAILURE;

    LPTSTR pszBuf = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwMaxSize * sizeof(TCHAR));

    if (!pszBuf) 
    {
        HeapFree(GetProcessHeap(), 0, (LPVOID) pszData);
        return MAPI_E_FAILURE;
    }

    // Build the URL
#if USENAVIGATE
    lstrcpyn(pszData, "action=compose", dwUrlSize);
#else
    wnsprintf(pszData, dwUrlSize, "%s?action=compose", szUrl);
#endif

    for (i = 0; i < lpMessage->nRecipCount; i++)
    {
        switch (lpMessage->lpRecips[i].ulRecipClass)
        {
            case MAPI_TO:
                StrCatBuff(pszData, "&to=", dwUrlSize);
                break;
            case MAPI_CC:
                StrCatBuff(pszData, "&cc=", dwUrlSize);
                break;
            case MAPI_BCC:
                StrCatBuff(pszData, "&bcc=", dwUrlSize);
                break;
        }
        pszBuf[URLEncode(pszBuf, lpMessage->lpRecips[i].lpszName, dwMaxSize-1)] = 0;
        StrCatBuff(pszData, pszBuf, dwUrlSize);
    }
    if (lpMessage->lpszSubject)
    {
        StrCatBuff(pszData, "&subject=", dwUrlSize);
        pszBuf[URLEncode(pszBuf, lpMessage->lpszSubject, dwMaxSize-1)] = 0;
        StrCatBuff(pszData, pszBuf, dwUrlSize);
    }
    if (lpMessage->lpszNoteText)
    {
        StrCatBuff(pszData, "&body=", dwUrlSize);
        pszBuf[URLEncode(pszBuf, lpMessage->lpszNoteText, dwMaxSize-1)] = 0;
        StrCatBuff(pszData, pszBuf, dwUrlSize);
    }
    if (lpMessage->nFileCount)
    {
#if USEATTFILE
        TCHAR szSep[32];
        TCHAR szPath[MAX_PATH];
        TCHAR szTemp[MAX_PATH];
        GetTempPath(MAX_PATH - 1, szTemp);
        BOOL bIsTemp;

        StrCatBuff(pszData, "&attfile=", dwUrlSize);
#else
        wnsprintf(pszBuf, dwMaxSize, "&filecount=%d", lpMessage->nFileCount);
        StrCatBuff(pszData, pszBuf, dwUrlSize);
#endif
        for (i = 0; i < lpMessage->nFileCount; i++)
        {
            if (!lpMessage->lpFiles[i].lpszPathName)
                continue;

            bIsTemp = FALSE;
            lstrcpyn(szPath, lpMessage->lpFiles[i].lpszPathName, ARRAYSIZE(szPath));
            hFile = CreateFile(szPath, 0, 0 /*GENERIC_READ, FILE_SHARE_READ*/, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile == INVALID_HANDLE_VALUE)
                continue;
            dwSize = GetFileSize(hFile, NULL);
            // Handle the case where this is a temporary file
            if (CompareString(LOCALE_SYSTEM_DEFAULT, 0, szTemp, lstrlen(szTemp), szPath, lstrlen(szTemp)) == CSTR_EQUAL)
            {
                // If the file was created in the last 2 seconds assume that it is really temporary
                FILETIME ftLastWrite, ftSystem;
                LARGE_INTEGER liLastWrite, liSystem;
                if (GetFileTime(hFile, NULL, NULL, &ftLastWrite))
                {
                    GetSystemTimeAsFileTime(&ftSystem);
                    liLastWrite.LowPart = ftLastWrite.dwLowDateTime;
                    liLastWrite.HighPart = ftLastWrite.dwHighDateTime;
                    liSystem.LowPart = ftSystem.dwLowDateTime;
                    liSystem.HighPart = ftSystem.dwHighDateTime;
                    //jeffif (liLastWrite.QuadPart - liSystem.QuadPart < 30000000L)
                        bIsTemp = TRUE;
                }
            }
            CloseHandle(hFile);
            if (dwSize == -1)
                continue;
            if (bIsTemp)
            {
                StrCatBuff(szPath, "^", ARRAYSIZE(szPath));
                MoveFile(lpMessage->lpFiles[i].lpszPathName, szPath);
                SetFileAttributes(szPath, FILE_ATTRIBUTE_READONLY);
            }
#if USEATTFILE
            szSep[URLEncode(szSep, "::", ARRAYSIZE(szSep)-1)] = 0;
            pszBuf[URLEncode(pszBuf, szPath, dwMaxSize-1)] = 0;
            StrCatBuff(pszData, pszBuf, dwUrlSize);
            StrCatBuff(pszData, szSep, dwUrlSize);
            if (lpMessage->lpFiles[i].lpszFileName)
            {
                pszBuf[URLEncode(pszBuf, lpMessage->lpFiles[i].lpszFileName, dwMaxSize-1)] = 0;
                StrCatBuff(pszData, pszBuf, dwUrlSize);
            }
            else
                StrCatBuff(pszData, pszBuf, dwUrlSize);
            StrCatBuff(pszData, szSep, dwUrlSize);
            wnsprintf(szSep, ARRAYSIZE(szSep), "^%d;", dwSize);
            pszBuf[URLEncode(pszBuf, szSep, dwMaxSize-1)] = 0;
            StrCatBuff(pszData, pszBuf, dwUrlSize);
#else
            wnsprintf(pszBuf, dwMaxSize, "&size%d=%d", i, dwSize);
            StrCatBuff(pszData, pszBuf, dwUrlSize);

            wnsprintf(pszBuf, dwMaxSize, "&path%d=", i);
            StrCatBuff(pszData, pszBuf, dwUrlSize);
            pszBuf[URLEncode(pszBuf, szPath, dwMaxSize-1)] = 0;
            StrCatBuff(pszData, pszBuf, dwUrlSize);

            if (lpMessage->lpFiles[i].lpszFileName)
            {
                wnsprintf(pszBuf, dwMaxSize-1, "&file%d=", i);
                StrCatBuff(pszData, pszBuf, dwUrlSize);
                pszBuf[URLEncode(pszBuf, lpMessage->lpFiles[i].lpszFileName, dwMaxSize-1)] = 0;
                StrCatBuff(pszData, pszBuf, dwUrlSize);
            }
#endif
        }
    }
    HeapFree(GetProcessHeap(), 0, (LPVOID)pszBuf);

#if USENAVIGATE
    DoNavigate(szUrl, pszData, FALSE);
#else
    SimpleNavigate(pszData, TRUE);
#endif
    HeapFree(GetProcessHeap(), 0, (LPVOID)pszData);

    return SUCCESS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
//
// MAPISendDocuments
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPISendDocuments(ULONG ulUIParam,
				   LPSTR lpszDelimChar,
				   LPSTR lpszFullPaths,
				   LPSTR lpszFileNames,
				   ULONG ulReserved)
{
	return MAPI_E_FAILURE;
}


///////////////////////////////////////////////////////////////////////
//
// MAPIAddress
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPIAddress(LHANDLE lhSession,
			     ULONG ulUIParam,
			     LPTSTR lpszCaption,
			     ULONG nEditFields,
			     LPTSTR lpszLabels,
			     ULONG nRecips,
			     lpMapiRecipDesc lpRecips,
			     FLAGS flFlags,
			     ULONG ulReserved,
			     LPULONG lpnNewRecips,
			     lpMapiRecipDesc FAR * lppNewRecips)
{
	return MAPI_E_FAILURE;
}


///////////////////////////////////////////////////////////////////////
//
// MAPIDetails
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPIDetails(LHANDLE lhSession,
			     ULONG ulUIParam,
			     lpMapiRecipDesc lpRecip,
			     FLAGS flFlags,
			     ULONG ulReserved)
{
	return MAPI_E_FAILURE;
}

///////////////////////////////////////////////////////////////////////
//
// MAPIResolveName
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPIResolveName(LHANDLE lhSession,
				 ULONG ulUIParam,
				 LPSTR lpszName,
				 FLAGS flFlags,
				 ULONG ulReserved,
				 lpMapiRecipDesc FAR *lppRecip)
{
	return MAPI_E_FAILURE;
}


///////////////////////////////////////////////////////////////////////
//
// MAPIFindNext
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPIFindNext(LHANDLE lhSession,
			      ULONG ulUIParam,
			      LPSTR lpszMessageType,
			      LPSTR lpszSeedMessageID,
			      FLAGS flFlags,
			      ULONG ulReserved,
			      LPSTR lpszMessageID)
{
	return MAPI_E_FAILURE;
}

///////////////////////////////////////////////////////////////////////
//
// MAPIReadMail
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPIReadMail(LHANDLE lhSession,
			      ULONG ulUIParam,
			      LPSTR lpszMessageID,
			      FLAGS flFlags,
			      ULONG ulReserved,
			      lpMapiMessage FAR *lppMessage)
{
	return MAPI_E_FAILURE;
}


///////////////////////////////////////////////////////////////////////
//
// MAPISaveMail
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPISaveMail(LHANDLE lhSession,
			      ULONG ulUIParam,
			      lpMapiMessage lpMessage,
			      FLAGS flFlags,
			      ULONG ulReserved,
			      LPSTR lpszMessageID)
{
	return MAPI_E_FAILURE;
}

///////////////////////////////////////////////////////////////////////
//
// MAPIDeleteMail
//
///////////////////////////////////////////////////////////////////////

ULONG FAR PASCAL MAPIDeleteMail(LHANDLE lhSession,
				ULONG ulUIParam,
				LPSTR lpszMessageID,
				FLAGS flFlags,
				ULONG ulReserved)
{
	return MAPI_E_FAILURE;
}


///////////////////////////////////////////////////////////////////////
//
// BMAPISendMail
//
///////////////////////////////////////////////////////////////////////

BMAPI_ENTRY BMAPISendMail (LHANDLE                      hSession,
			   ULONG                        ulUIParam,
			   LPVB_MESSAGE         lpM,
			   LPSAFEARRAY *        lppsaRecips,
			   LPSAFEARRAY *        lppsaFiles,
			   ULONG                        flFlags,
			   ULONG                        ulReserved)
{
	return MAPI_E_FAILURE;
}

///////////////////////////////////////////////////////////////////////
//
// BMAPIFindNext
//
///////////////////////////////////////////////////////////////////////
BMAPI_ENTRY BMAPIFindNext( LHANDLE      hSession,       // Session
			   ULONG        ulUIParam,      // UIParam
			   BSTR *       lpbstrType,     // MessageType
			   BSTR *       lpbstrSeed,     // Seed message Id
			   ULONG        flFlags,        // Flags
			   ULONG        ulReserved,     // Reserved
			   BSTR *       lpbstrId) 
{
	return MAPI_E_FAILURE;
}

///////////////////////////////////////////////////////////////////////
//
// BMAPIReadMail
//
///////////////////////////////////////////////////////////////////////
BMAPI_ENTRY BMAPIReadMail( LPULONG      lpulMessage,    // pointer to output data (out)
			   LPULONG      nRecips,        // number of recipients (out)
			   LPULONG      nFiles,         // number of file attachments (out)
			   LHANDLE      hSession,       // Session
			   ULONG        ulUIParam,      // UIParam
			   BSTR *       lpbstrID,       // Message Id
			   ULONG        flFlags,        // Flags
			   ULONG        ulReserved )    // Reserved
{
	return MAPI_E_FAILURE;
}

///////////////////////////////////////////////////////////////////////
//
// BMAPIGetReadMail
//
///////////////////////////////////////////////////////////////////////
BMAPI_ENTRY BMAPIGetReadMail( ULONG             lpMessage,       // Pointer to MAPI Mail
			      LPVB_MESSAGE      lpvbMessage, // Pointer to VB Message Buffer (out)
			      LPSAFEARRAY * lppsaRecips, // Pointer to VB Recipient Buffer (out)
			      LPSAFEARRAY * lppsaFiles,  // Pointer to VB File attachment Buffer (out)
			      LPVB_RECIPIENT lpvbOrig)   // Pointer to VB Originator Buffer (out)
{
	return MAPI_E_FAILURE;
}

///////////////////////////////////////////////////////////////////////
//
// BMAPISaveMail
//
///////////////////////////////////////////////////////////////////////
BMAPI_ENTRY BMAPISaveMail( LHANDLE                      hSession,       // Session
			   ULONG                        ulUIParam,      // UIParam
			   LPVB_MESSAGE         lpM,            // Pointer to VB Message Buffer
			   LPSAFEARRAY *        lppsaRecips,    // Pointer to VB Recipient Buffer
			   LPSAFEARRAY *        lppsaFiles,     // Pointer to VB File Attacment Buffer
			   ULONG                        flFlags,        // Flags
			   ULONG                        ulReserved,     // Reserved
			   BSTR *                       lpbstrID)       // Message ID
{
	return MAPI_E_FAILURE;
}

///////////////////////////////////////////////////////////////////////
//
// BMAPIAddress
//
///////////////////////////////////////////////////////////////////////

BMAPI_ENTRY BMAPIAddress( LPULONG                       lpulRecip,       // Pointer to New Recipient Buffer (out)
			  LHANDLE                       hSession,        // Session
			  ULONG                         ulUIParam,       // UIParam
			  BSTR *                        lpbstrCaption,   // Caption string
			  ULONG                         ulEditFields,    // Number of Edit Controls
			  BSTR *                        lpbstrLabel,     // Label string
			  LPULONG                       lpulRecipients,  // Pointer to number of Recipients (in/out)
			  LPSAFEARRAY *         lppsaRecip,      // Pointer to Initial Recipients VB_RECIPIENT
			  ULONG                         ulFlags,         // Flags
			  ULONG                         ulReserved )     // Reserved
{
	return MAPI_E_FAILURE;
}

///////////////////////////////////////////////////////////////////////
//
// BMAPIGetAddress
//
///////////////////////////////////////////////////////////////////////

BMAPI_ENTRY BMAPIGetAddress (ULONG                      ulRecipientData, // Pointer to recipient data
			     ULONG                      cRecipients,     // Number of recipients
							 LPSAFEARRAY *  lppsaRecips )    // VB recipient array
{
	return MAPI_E_FAILURE;
}

///////////////////////////////////////////////////////////////////////
//
// BMAPIDetails
//
///////////////////////////////////////////////////////////////////////

BMAPI_ENTRY BMAPIDetails (LHANDLE                       hSession,   // Session
			  ULONG                         ulUIParam,      // UIParam
			  LPVB_RECIPIENT        lpVB,           // Pointer to VB recipient stucture
			  ULONG                         ulFlags,    // Flags
			  ULONG                         ulReserved) // Reserved

{
	return MAPI_E_FAILURE;
}

///////////////////////////////////////////////////////////////////////
//
// BMAPIResolveName
//
///////////////////////////////////////////////////////////////////////

BMAPI_ENTRY BMAPIResolveName (LHANDLE                   hSession,     // Session
			      ULONG                     ulUIParam,    // UIParam
			      BSTR                              bstrMapiName, // Name to be resolved
			      ULONG                     ulFlags,      // Flags
			      ULONG                     ulReserved,   // Reserved
			      LPVB_RECIPIENT    lpVB)             // Pointer to VB recipient structure (out)
{
	return MAPI_E_FAILURE;
}

///////////////////////////////////////////////////////////////////////
//
// MailToProtocolHandler
//
///////////////////////////////////////////////////////////////////////

void CALLBACK MailToProtocolHandler(HWND      hwnd,
				    HINSTANCE hinst,
									LPSTR     lpszCmdLine,
									int       nCmdShow)
{
	TCHAR pszUrl[256];

	GetPostUrl(pszUrl, sizeof(TCHAR) * 256);
#if USENAVIGATE
	LPTSTR pszData = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lstrlen(lpszCmdLine) + 32) * sizeof(TCHAR));

	wsprintf(pszData, "action=compose&to=%s", &lpszCmdLine[7]);
	// Convert the extraneous '?' to '&'
	for (LPTSTR p = pszData; *p; p++)
		if (*p == '?')
			*p = '&';

	DoNavigate(pszUrl, pszData, FALSE);

	HeapFree(GetProcessHeap(), 0, (LPVOID)pszData);
#else
	LPTSTR pszData = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lstrlen(lpszCmdLine) + lstrlen(pszUrl) + 32) * sizeof(TCHAR));

	wsprintf(pszData, "%s?action=compose&to=%s", pszUrl, &lpszCmdLine[7]);
	// Convert the extraneous '?' to '&'
	for (LPTSTR p = &pszData[lstrlen(pszUrl) + 1]; *p; p++)
		if (*p == '?')
			*p = '&';

	SimpleNavigate(pszData);

	HeapFree(GetProcessHeap(), 0, (LPVOID)pszData);
#endif
}

///////////////////////////////////////////////////////////////////////
//
// OpenInboxHandler
//
///////////////////////////////////////////////////////////////////////

void CALLBACK OpenInboxHandler(HWND      hwnd,
			       HINSTANCE hinst,
							   LPSTR     lpszCmdLine,
							   int       nCmdShow)
{
	TCHAR pszUrl[256];

	GetPostUrl(pszUrl, sizeof(TCHAR) * 256);

#if USENAVIGATE
	DoNavigate(pszUrl, "action=inbox", FALSE);
#else
	LPTSTR pszData = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (lstrlen(pszUrl) + 32) * sizeof(TCHAR));

	wsprintf(pszData, "%s?action=inbox", pszUrl);
	SimpleNavigate(pszData);

	HeapFree(GetProcessHeap(), 0, (LPVOID)pszData);
#endif
}

///////////////////////////////////////////////////////////////////////
//
// Layout of Registry Usage
//
//
// HKEY_CLASSES_ROOT\mailto
// HKEY_CLASSES_ROOT\mailto\DefaultIcon
// HKEY_CLASSES_ROOT\mailto\shell\open\command
//
// HKEY_LOCAL_MACHINE\SOFTWARE\Clients\Mail
// HKEY_LOCAL_MACHINE\SOFTWARE\Clients\Mail\Hotmail
// HKEY_LOCAL_MACHINE\SOFTWARE\Clients\Mail\Hotmail\Protocols\mailto
// HKEY_LOCAL_MACHINE\SOFTWARE\Clients\Mail\Hotmail\Protocols\mailto\DefaultIcon
// HKEY_LOCAL_MACHINE\SOFTWARE\Clients\Mail\Hotmail\Protocols\mailto\shell\open\command
// HKEY_LOCAL_MACHINE\SOFTWARE\Clients\Mail\Hotmail\shell\open\command
// HKEY_LOCAL_MACHINE\SOFTWARE\Clients\Mail\Hotmail\backup
//
///////////////////////////////////////////////////////////////////////

#define MAILTO          TEXT("mailto")
#define PROTOCOLS       TEXT("Protocols")
#define DEFAULTICON     TEXT("DefaultIcon")
#define COMMAND         TEXT("shell\\open\\command")
#define MAIL            TEXT("SOFTWARE\\Clients\\Mail")
#define POSTURL         TEXT("posturl")
#define BACKUP          TEXT("backup")


///////////////////////////////////////////////////////////////////////
//
// SetRegStringValue
//
///////////////////////////////////////////////////////////////////////

static LONG SetRegStringValue(HKEY hkKey, LPTSTR lpszKey, LPTSTR lpszValue, LPTSTR lpszPath, DWORD dwType)
{
    if (!(dwType == REG_SZ) && !(dwType == REG_EXPAND_SZ))
        return ERROR_INVALID_PARAMETER;

	if (lpszPath)
	{
		TCHAR szValue[MAX_PATH + 32];
		wsprintf(szValue, lpszValue, lpszPath);
		return RegSetValueEx(hkKey, lpszKey, 0, dwType, (LPBYTE)szValue, (lstrlen(szValue) + 1) * sizeof(TCHAR));
	}

	return RegSetValueEx(hkKey, lpszKey, 0, dwType, (LPBYTE)lpszValue, (lstrlen(lpszValue) + 1) * sizeof(TCHAR));
}

///////////////////////////////////////////////////////////////////////
//
// CreateMailToEntries
//
///////////////////////////////////////////////////////////////////////

static LONG CreateMailToEntries(HKEY hkKey, TCHAR* lpszPath, BOOL fRegExpandSz)
{
	LONG err;
	HKEY hkMailToProt;
	HKEY hkDefaultIcon;
	HKEY hkCommand;

	err = RegCreateKey(hkKey, MAILTO, &hkMailToProt);
	if (err == ERROR_SUCCESS)
	{
		err = SetRegStringValue(hkMailToProt, NULL, TEXT("URL:MailTo Protocol"), NULL, REG_SZ);
		if (err == ERROR_SUCCESS)
		{
			DWORD editFlags = 2; 
			err = RegSetValueEx(hkMailToProt, TEXT("EditFlags"), 0, REG_BINARY, (LPBYTE)&editFlags, sizeof(DWORD));
		}
		if (err == ERROR_SUCCESS)
			err = SetRegStringValue(hkMailToProt, TEXT("URL Protocol"), TEXT(""), NULL, REG_SZ);

		if (err == ERROR_SUCCESS)
			err = RegCreateKey(hkMailToProt, DEFAULTICON, &hkDefaultIcon);
		if (err == ERROR_SUCCESS)
		{
			err = SetRegStringValue(hkDefaultIcon, NULL, "%s,1", lpszPath, fRegExpandSz?REG_EXPAND_SZ:REG_SZ);
			RegCloseKey(hkDefaultIcon);
		}

		if (err == ERROR_SUCCESS)
			err = RegCreateKey(hkMailToProt, COMMAND, &hkCommand);
		if (err == ERROR_SUCCESS)
		{
            DWORD dwNTVer = 0;
            // BUGBUG: Only the rundll32 on NT5 can handle double quotes around the path
            // Lucky on Win9x and NT4 the epand sz path will never be a long file name and the old
            // rundll32 works, but we cannot have double quotes
            if (FRunningOnNTEx(&dwNTVer) && (dwNTVer >= 5))
            {
                err = SetRegStringValue(hkCommand, NULL, "rundll32.exe \"%s\",MailToProtocolHandler %%1", lpszPath, fRegExpandSz?REG_EXPAND_SZ:REG_SZ);
            }
            else
            {
                err = SetRegStringValue(hkCommand, NULL, "rundll32.exe %s,MailToProtocolHandler %%1", lpszPath, fRegExpandSz?REG_EXPAND_SZ:REG_SZ);
            }
			RegCloseKey(hkCommand);
		}
		RegCloseKey(hkMailToProt);
	}
	return err;
}

///////////////////////////////////////////////////////////////////////
//
// DoAddService
//
///////////////////////////////////////////////////////////////////////

STDAPI DoAddService(LPSTR lpszService, LPSTR lpszPostURL)
{
	LONG err;
    TCHAR szLongPath[MAX_PATH];
    TCHAR szPath[MAX_PATH];
	HKEY hkClientsMail;
	HKEY hkService;
	HKEY hkProtocols;
	HKEY hkCommand;
	HKEY hkBackup;
    BOOL fExistingMailClient = FALSE;
    BOOL fRegExpandSz = FALSE;

    GetModuleFileName(g_hInstMAPI, szLongPath, MAX_PATH);  // get path to this DLL
    GetShortPathName(szLongPath, szPath, MAX_PATH);

	// First setup the info for the protocol in clients section
	err = RegCreateKey(HKEY_LOCAL_MACHINE, MAIL, &hkClientsMail);
	if (err == ERROR_SUCCESS)
	{
        fRegExpandSz = MyPathUnExpandEnvStrings(szPath, szLongPath, ARRAYSIZE(szLongPath));
        if (fRegExpandSz)
            lstrcpy(szPath, szLongPath);
		err = RegCreateKey(hkClientsMail, lpszService, &hkService);
		if (err == ERROR_SUCCESS)
		{
			err = SetRegStringValue(hkService, NULL, lpszService, NULL, REG_SZ);
			if (err == ERROR_SUCCESS)
            {
                err = SetRegStringValue(hkService, TEXT("DLLPath"), szPath, NULL,
                                          fRegExpandSz?REG_EXPAND_SZ:REG_SZ);
            }
			if (err == ERROR_SUCCESS && lpszPostURL && lstrlen(lpszPostURL))
				err = SetRegStringValue(hkService, TEXT("posturl"), lpszPostURL, NULL, REG_SZ);
			if (err == ERROR_SUCCESS)
				err = RegCreateKey(hkService, PROTOCOLS, &hkProtocols);
			if (err == ERROR_SUCCESS)
			{
				err = CreateMailToEntries(hkProtocols, szPath, fRegExpandSz);
				RegCloseKey(hkProtocols);
			}
			if (err == ERROR_SUCCESS)
				err = RegCreateKey(hkService, COMMAND, &hkCommand);
			if (err == ERROR_SUCCESS)
			{
                DWORD dwNTVer = 0;
                // BUGBUG: Only the rundll32 on NT5 can handle double quotes around the path
                // Lucky on Win9x and NT4 the epand sz path will never be a long file name and the old
                // rundll32 works, but we cannot have double quotes
                if (FRunningOnNTEx(&dwNTVer) && (dwNTVer >= 5))
                {
				    err = SetRegStringValue(hkCommand, NULL, "rundll32.exe \"%s\",OpenInboxHandler", szPath,
                                            fRegExpandSz?REG_EXPAND_SZ:REG_SZ);
                }
                else
                {
				    err = SetRegStringValue(hkCommand, NULL, "rundll32.exe %s,OpenInboxHandler", szPath,
                                            fRegExpandSz?REG_EXPAND_SZ:REG_SZ);
                }
				RegCloseKey(hkCommand);
			}
			if (err == ERROR_SUCCESS)
				err = RegCreateKey(hkService, BACKUP, &hkBackup);
			if (err == ERROR_SUCCESS)
			{
				TCHAR szValue[MAX_PATH];
				DWORD size;
				DWORD type;
				HKEY hkDefaultIcon;
				HKEY hkCommand;

				err = RegOpenKey(HKEY_CLASSES_ROOT, TEXT("mailto\\DefaultIcon"), &hkDefaultIcon);
				if (err == ERROR_SUCCESS)
				{
					size = sizeof(TCHAR) * MAX_PATH;
					err = RegQueryValueEx(hkDefaultIcon, NULL, 0, &type, (LPBYTE)szValue, &size);
					if (err == ERROR_SUCCESS)
						err = RegSetValueEx(hkBackup, DEFAULTICON, 0, type, (LPBYTE)szValue, size);
					RegCloseKey(hkDefaultIcon);
				}

				err = RegOpenKey(HKEY_CLASSES_ROOT, TEXT("mailto\\shell\\open\\command"), &hkCommand);
				if (err == ERROR_SUCCESS)
				{
					size = sizeof(TCHAR) * MAX_PATH;
					err = RegQueryValueEx(hkCommand, NULL, 0, &type, (LPBYTE)szValue, &size);
					if (err == ERROR_SUCCESS)
                    {
                        fExistingMailClient = TRUE;
						err = RegSetValueEx(hkBackup, TEXT("command"), 0, type, (LPBYTE)szValue, size);
                    }
					RegCloseKey(hkCommand);
				}

				size = sizeof(TCHAR) * MAX_PATH;
				err = RegQueryValueEx(hkClientsMail, NULL, 0, &type, (LPBYTE)szValue, &size);
				if (err == ERROR_SUCCESS)
					err = RegSetValueEx(hkBackup, TEXT("mail"), 0, type, (LPBYTE)szValue, size);

				RegCloseKey(hkBackup);
			}
		    RegCloseKey(hkService);
		}
		if (err == ERROR_SUCCESS && !fExistingMailClient)
			SetRegStringValue(hkClientsMail, NULL, lpszService, NULL, REG_SZ);
	    RegCloseKey(hkClientsMail);
	}
	if (err == ERROR_SUCCESS && !fExistingMailClient)
		err = CreateMailToEntries(HKEY_CLASSES_ROOT, szPath, fRegExpandSz);

    //
    // REVIEW Backup fails sometimes. Need to clean up registry changes and 
    // probably remove all backup registry entirely.
    // For now just safe to return S_OK
    // 
#if 0
    if (err != ERROR_SUCCESS)
	return HRESULT_FROM_WIN32(err);
#else
    return S_OK;
#endif
}

///////////////////////////////////////////////////////////////////////
//
// DeleteKeyAndSubKeys
//
///////////////////////////////////////////////////////////////////////

static LONG DeleteKeyAndSubKeys(HKEY hkIn, LPCTSTR pszSubKey)
{
    HKEY  hk;
    TCHAR szTmp[MAX_PATH];
    DWORD dwTmpSize;
    long  l;
    int   x;

	l = RegOpenKeyEx(hkIn, pszSubKey, 0, KEY_ALL_ACCESS, &hk);
	if (l != ERROR_SUCCESS) 
		return l;

    // loop through all subkeys, blowing them away.
    //
    x = 0;
    while (l == ERROR_SUCCESS)
	{
	dwTmpSize = MAX_PATH;
	l = RegEnumKeyEx(hk, 0, szTmp, &dwTmpSize, 0, NULL, NULL, NULL);
	if (l != ERROR_SUCCESS)
	    break;

	l = DeleteKeyAndSubKeys(hk, szTmp);
    }

    // there are no subkeys left, [or we'll just generate an error and return FALSE].
    // let's go blow this dude away.
    //
	RegCloseKey(hk);
    return RegDeleteKey(hkIn, pszSubKey);
}

///////////////////////////////////////////////////////////////////////
//
// DoRemoveService
//
///////////////////////////////////////////////////////////////////////

STDAPI DoRemoveService(LPSTR lpszService)
{
	TCHAR szValue[MAX_PATH];
	DWORD size;
	LONG err;
	DWORD type;
	HKEY hkDefaultIcon;
	HKEY hkCommand;
	HKEY hkBackup;
	HKEY hkService;
	HKEY hkClientsMail;

    //
	// Restore the previous values if HMMAPI is the current provider
    //
	err = RegOpenKey(HKEY_LOCAL_MACHINE, MAIL, &hkClientsMail);
	if (err == ERROR_SUCCESS)
	{
        //
        // Find the name of the current provider
        //
        TCHAR szCurrent[MAX_PATH];
        DWORD cb = sizeof(szCurrent);
        err = RegQueryValueEx(hkClientsMail, NULL, NULL, NULL, (LPBYTE)szCurrent, &cb);
        if (err == ERROR_SUCCESS)
        {
            //
            // Check if it is HMMAPI
            //
            if (StrCmp(szCurrent, lpszService) == 0)
            {
		        err = RegOpenKey(hkClientsMail, lpszService, &hkService);
		        if (err == ERROR_SUCCESS)
		        {
			        err = RegOpenKey(hkService, BACKUP, &hkBackup);
			        if (err == ERROR_SUCCESS)
			        {
				        err = RegOpenKey(HKEY_CLASSES_ROOT, TEXT("mailto\\DefaultIcon"), &hkDefaultIcon);
				        if (err == ERROR_SUCCESS)
				        {
                            size = sizeof(TCHAR) * MAX_PATH;
					        err = RegQueryValueEx(hkBackup, DEFAULTICON, 0, &type, (LPBYTE)szValue, &size);
					        if (err == ERROR_SUCCESS)
						        err = RegSetValueEx(hkDefaultIcon, NULL, 0, type, (LPBYTE)szValue, size);
					        RegCloseKey(hkDefaultIcon);
				        }

				        err = RegOpenKey(HKEY_CLASSES_ROOT, TEXT("mailto\\shell\\open\\command"), &hkCommand);
				        if (err == ERROR_SUCCESS)
				        {
					        size = sizeof(TCHAR) * MAX_PATH;
					        err = RegQueryValueEx(hkBackup, TEXT("command"), 0, &type, (LPBYTE)szValue, &size);
					        if (err == ERROR_SUCCESS)
						        err = RegSetValueEx(hkCommand, NULL, 0, type, (LPBYTE)szValue, size);
					        RegCloseKey(hkCommand);
				        }

				        size = sizeof(TCHAR) * MAX_PATH;
				        err = RegQueryValueEx(hkBackup, TEXT("mail"), 0, &type, (LPBYTE)szValue, &size);
				        if (err == ERROR_SUCCESS)
					        err = RegSetValueEx(hkClientsMail, NULL, 0, type, (LPBYTE)szValue, size);

				        RegCloseKey(hkBackup);
			        }
			        RegCloseKey(hkService);
		        }
            }
            err = DeleteKeyAndSubKeys(hkClientsMail, lpszService);
        }
        RegCloseKey(hkClientsMail);
	}

    //
    // REVIEW Backup fails sometimes. Need to clean up registry changes and 
    // probably remove all backup registry entirely.
    // For now just safe to return S_OK
    // 
#if 0
    if (err != ERROR_SUCCESS)
	return HRESULT_FROM_WIN32(err);
#else
    return S_OK;
#endif
}

///////////////////////////////////////////////////////////////////////
//
// AddService
//
///////////////////////////////////////////////////////////////////////

void CALLBACK AddService(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	LPSTR lpszService = lpszCmdLine;
	LPSTR lpszPostUrl = NULL;

	if (*lpszService == '"')
	{
		lpszService++;
		lpszPostUrl = StrChr(lpszService, '"');
		if (lpszPostUrl)
		{
			*lpszPostUrl = 0;
			lpszPostUrl++;
			while (*lpszPostUrl && *lpszPostUrl == ' ')
				lpszPostUrl++;
			if (*lpszPostUrl == 0)
				lpszPostUrl = NULL;
		}
	}
	else
	{
		lpszPostUrl = StrChr(lpszService, ' ');
		if (lpszPostUrl)
		{
			*lpszPostUrl = 0;
			lpszPostUrl++;
		}
	}
	DoAddService(lpszService, lpszPostUrl);
}

///////////////////////////////////////////////////////////////////////
//
// RemoveService
//
///////////////////////////////////////////////////////////////////////

void CALLBACK RemoveService(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	DWORD dwLen = (lpszCmdLine) ? lstrlen(lpszCmdLine) : 0;

	if (dwLen)
	{
		if (*lpszCmdLine == '"' && lpszCmdLine[dwLen - 1] == '"')
		{
			lpszCmdLine[dwLen - 1] = 0;
			lpszCmdLine++;
		}
		DoRemoveService(lpszCmdLine);
	}
}

///////////////////////////////////////////////////////////////////////
//
// DllRegisterServer
//
///////////////////////////////////////////////////////////////////////

STDAPI DllRegisterServer(void)
{
    return DoAddService(TEXT("Hotmail"), NULL);
}

///////////////////////////////////////////////////////////////////////
//
// DllUnregisterServer
//
///////////////////////////////////////////////////////////////////////

STDAPI DllUnregisterServer(void)
{
    return DoRemoveService(TEXT("Hotmail"));
}
