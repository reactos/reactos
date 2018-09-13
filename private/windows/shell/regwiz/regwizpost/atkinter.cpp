/*
	File : ATKInternet.cpp

	This is a wrapper function for internet API which accepts UNICODE.
	Basically these wrapper converts the UNICODE string to normal SBCS and invokes the
	appropriate SBCS  internet function


*/
#ifdef _UNICODE


#include <tchar.h>
#include "AtkInternet.h"
#include "rw_common.h"


#define MAX_SZ   260

HINTERNET  ATK_InternetOpenW(
    IN LPCWSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCWSTR lpszProxy OPTIONAL,
    IN LPCWSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    )

{
	char *pRet;

	char  szAgent[MAX_SZ] = "" ;
	char  szProxy[MAX_SZ] ="";
	char  szProxyByPass[MAX_SZ]="";

	pRet =  ConvertToANSIString (lpszAgent);
	if(pRet) {
	strcpy(szAgent, pRet);
	}

	pRet = ConvertToANSIString (lpszProxy);
	if(pRet) {
	strcpy(szProxy, pRet);
	}

	pRet = ConvertToANSIString (lpszProxyBypass);
	if(pRet) {
	strcpy(szProxyByPass, ConvertToANSIString (lpszProxyBypass));
	}

	return InternetOpenA(
    szAgent,
    dwAccessType,
    szProxy ,
    szProxyByPass,
	dwFlags);


}




HINTERNET
ATK_InternetConnectW(
    IN HINTERNET hInternet,
    IN LPCWSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPCWSTR lpszUserName OPTIONAL,
    IN LPCWSTR lpszPassword OPTIONAL,
    IN DWORD dwService,
    IN DWORD dwFlags,
    IN DWORD dwContext
    )
{

	char szServerName[MAX_SZ]="";
	char szUserName[MAX_SZ]="";
	char szPassword[MAX_SZ]="";
	char *pRet;

	pRet = ConvertToANSIString (lpszServerName);
	if(pRet) {
		strcpy(szServerName,pRet);
	}

	pRet = ConvertToANSIString (lpszUserName);
	if(pRet) {
		strcpy(szUserName,pRet);
	}
	pRet = ConvertToANSIString (lpszPassword);
	if(pRet) {
		strcpy(szPassword,pRet);
	}

	return InternetConnectA(
    hInternet,
    szServerName,
    nServerPort,
    szUserName ,
    szPassword ,
    dwService,
    dwFlags,
    dwContext
    );


}



HINTERNET
ATK_HttpOpenRequestW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszVerb,
    IN LPCWSTR lpszObjectName,
    IN LPCWSTR lpszVersion,
    IN LPCWSTR lpszReferrer OPTIONAL,
    IN LPCWSTR FAR * lpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

{
	char szVerb[MAX_SZ]= "";
	char szObjectName[MAX_SZ]="";
	char szVersion[MAX_SZ]="";
	char szReferrer[MAX_SZ]="";
	char szAcceptTypes[MAX_SZ]="";
	char *pRet;

	pRet = ConvertToANSIString (lpszVerb);
	if(pRet) {
		strcpy(szVerb,pRet);
	}

	pRet = ConvertToANSIString (lpszObjectName);
	if(pRet) {
		strcpy(szObjectName,pRet);
	}

	pRet = ConvertToANSIString (lpszVersion);
	if(pRet) {
		strcpy(szVersion,pRet);
	}
	pRet = ConvertToANSIString (lpszReferrer);
	if(pRet) {
		strcpy(szReferrer,pRet);
	}
//	strcpy(szAcceptTypes,ConvertToANSIString (lpszAcceptTypes));

	return HttpOpenRequestA(
    hConnect,
    szVerb,
    szObjectName,
    szVersion,
    szReferrer ,
    NULL,
    dwFlags,
    dwContext
    );

}



BOOL
ATK_HttpSendRequestW(
    IN HINTERNET hRequest,
    IN LPCWSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength
    )
{
	char szHeaders[MAX_SZ]="";
	char *pRet;
	pRet =  ConvertToANSIString (lpszHeaders);
	if(pRet) {
		strcpy(szHeaders,pRet);
	}
	return HttpSendRequestA(
    hRequest,
    szHeaders ,
    dwHeadersLength,
    lpOptional,
    dwOptionalLength);


}


BOOL
ATK_InternetGetLastResponseInfoW(
    OUT LPDWORD lpdwError,
    OUT LPWSTR lpszBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )
{
	DWORD dwError;
	DWORD dwErrorLen;
	CHAR szErrorInfo[256];	
	dwErrorLen = 256;
	BOOL bRet;

	bRet = InternetGetLastResponseInfoA(&dwError,
							szErrorInfo,
							&dwErrorLen);
#ifdef _DEBUG
	RW_DEBUG << "\nInternet GetLastResponse ErrorNo:" << dwError ;
	RW_DEBUG << "\n\tErrorInfo:"<<szErrorInfo<<flush;
#endif

	return bRet;
	

}


BOOL ATK_HttpQueryInfoW(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
	)
{
	BOOL bRet;

	bRet = HttpQueryInfoA(hRequest, dwInfoLevel,lpBuffer,
			lpdwBufferLength, lpdwIndex);
#ifdef _DEBUG
	RW_DEBUG << "\nHttpQueryInfo:" ;
	RW_DEBUG << "\n\tBuffer  Length "<< *lpdwBufferLength <<flush;
#endif
	return bRet;


}




BOOL ATK_InternetGetCookieW(IN TCHAR *lpszUrl,
						      IN TCHAR *lpszCookieName,
							  OUT TCHAR *lpCookieData,
							  OUT LPDWORD lpdwSize)
{
	char czUrl[256]="";
	char czCookieName[128]="";
	char czCookieData[256]="";
	char *pRet= NULL;
	TCHAR *pTP;
	BOOL  bRetVal;
#ifdef _UNICODE
	
	// Presently the UNICODE version is not implemented in
	// the NT SDK
	//and the declaration defined in WININET.H is
	//	BOOLAPI
	//	InternetGetCookieW(
    //	IN LPCSTR lpszUrl,
    //	IN LPCWSTR lpszCookieName,
    //	OUT LPWSTR lpCookieData,
    //	IN OUT LPDWORD lpdwSize
    //);
// So Convert the first parameter to  ANSI String
	pRet = ConvertToANSIString(lpszUrl);
	if(pRet) strcpy(czUrl,pRet);

	pRet = ConvertToANSIString(lpszCookieName);
	if(pRet) strcpy(czCookieName,pRet);

	#ifdef  USE_ASTRATEK_WRAPPER
	bRetVal = InternetGetCookieA(czUrl,
							czCookieName,
							czCookieData,
							lpdwSize);
	//
	// Convert the Return Values to Unicode
	 pTP= ConvertToUnicode(czCookieData);
	 if(pTP) {
		 _tcscpy(lpCookieData,pTP);
	 }else {
		lpCookieData[0]= _T('\0');
	 }

	#else
	bRetVal = InternetGetCookieW(czUrl,
							lpszCookieName,
							lpCookieData,
							lpdwSize);

	#endif
	
	 return bRetVal;
#endif

}


BOOL ATK_InternetQueryOption(IN HINTERNET hInternet,
							 IN DWORD dwOption,
							 OUT LPVOID lpBuffer,
							 IN OUT LPDWORD lpdwBufferLength
							)
{
	BOOL  bRetVal;
	bRetVal = InternetQueryOptionA(hInternet,
							dwOption,
							lpBuffer,
							lpdwBufferLength);

    return bRetVal;

}

#endif
