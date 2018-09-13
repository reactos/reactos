/*
	File : AtkInternet.h
	Date :
	
	This is a wrapper function declaration for Winows Internet API.
	This Warpper is to handle the UNICODE as the present Windows Internet API does not support UNICODE
	

*/
#ifndef __ASTRATEK_INTERNET_HEADER
#define __ASTRATEK_INTERNET_HEADER

#include <tchar.h>
#include <windows.h>
#include <wininet.h>
#define USE_ASTRATEK_WRAPPER 1

//
// INTERNET_PROXY_INFO - structure supplied with INTERNET_OPTION_PROXY to get/
// set proxy information on a InternetOpen() handle
//
typedef struct {
    // dwAccessType - INTERNET_OPEN_TYPE_DIRECT, INTERNET_OPEN_TYPE_PROXY, or
    // INTERNET_OPEN_TYPE_PRECONFIG (set only)
    DWORD dwAccessType;
    // lpszProxy - proxy server list
    LPCSTR lpszProxy;
    // lpszProxyBypass - proxy bypass list
    LPCSTR lpszProxyBypass;
} ATK_INTERNET_PROXY_INFOW;

#ifdef _UNICODE
	#ifdef USE_ASTRATEK_WRAPPER
	#define ATK_INTERNET_PROXY_INFO  ATK_INTERNET_PROXY_INFOW
	#else
	#define ATK_INTERNET_PROXY_INFO  INTERNET_PROXY_INFO
	#endif
#else
	#define ATK_INTERNET_PROXY_INFO  INTERNET_PROXY_INFO
#endif // !UNICODE
				 

HINTERNET
ATK_InternetOpenW(
    IN LPCWSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCWSTR lpszProxy OPTIONAL,
    IN LPCWSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    );				  

#ifdef _UNICODE

	#ifdef USE_ASTRATEK_WRAPPER
	#define ATK_InternetOpen  ATK_InternetOpenW
	#else
	#define ATK_InternetOpen  InternetOpen
	#endif
#else
	#define ATK_InternetOpen  InternetOpenA
#endif // !UNICODE




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
    );
#ifdef _UNICODE
	#ifdef USE_ASTRATEK_WRAPPER
	#define ATK_InternetConnect  ATK_InternetConnectW
	#else
	#define ATK_InternetConnect  InternetConnect
	#endif
#else
	#define ATK_InternetConnect  InternetConnectA
#endif // !UNICODE


HINTERNET
ATK_HttpOpenRequestW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszVerb,
    IN LPCWSTR lpszObjectName,
    IN LPCWSTR lpszVersion,
    IN LPCWSTR lpszReferrer OPTIONAL,
    IN LPCWSTR FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );
#ifdef _UNICODE
	#ifdef  USE_ASTRATEK_WRAPPER
	#define ATK_HttpOpenRequest  ATK_HttpOpenRequestW
	#else
	#define ATK_HttpOpenRequest  HttpOpenRequest
	#endif
#else
	#define ATK_HttpOpenRequest  HttpOpenRequestA
#endif // !UNICODE



BOOL
ATK_HttpSendRequestW(
    IN HINTERNET hRequest,
    IN LPCWSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength
    );
#ifdef _UNICODE
	#ifdef  USE_ASTRATEK_WRAPPER
	#define ATK_HttpSendRequest  ATK_HttpSendRequestW
	#else
	#define ATK_HttpSendRequest  HttpSendRequest
	#endif
#else
	#define ATK_HttpSendRequest  HttpSendRequestA
#endif // !UNICODE





BOOL
ATK_InternetGetLastResponseInfoW(
    OUT LPDWORD lpdwError,
    OUT LPWSTR lpszBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    );
#ifdef _UNICODE
	#ifdef  USE_ASTRATEK_WRAPPER
	#define ATK_InternetGetLastResponseInfo  ATK_InternetGetLastResponseInfoW
	#else
	#define ATK_InternetGetLastResponseInfo  InternetGetLastResponseInfo
	#endif
#else
	#define ATK_InternetGetLastResponseInfo  InternetGetLastResponseInfoA
#endif



BOOL
ATK_HttpQueryInfoW(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
	);

#ifdef _UNICODE
	#ifdef  USE_ASTRATEK_WRAPPER
	#define ATK_HttpQueryInfo  ATK_HttpQueryInfoW
	#else
	#define ATK_HttpQueryInfo  HttpQueryInfo
	#endif
#else
	#define ATK_HttpQueryInfo  HttpQueryInfoA
#endif

//
//  In this functin declaration we have modified the first
//  parameter type as TCHAR to have uniformity in coding in tha APP 
//  
BOOL ATK_InternetGetCookieW(IN TCHAR *lpszUrl,
					       IN TCHAR *lpszCookieName,
					  	   OUT TCHAR *lpCookieData,
						   OUT LPDWORD lpdwSize);
#ifdef _UNICODE
	#ifdef  USE_ASTRATEK_WRAPPER
	#define ATK_InternetGetCookie  ATK_InternetGetCookieW
	#else
	#define ATK_InternetGetCookie  InternetGetCookie
	#endif

#else
	#define ATK_InternetGetCookie  InternetGetCookieA
#endif

BOOL
ATK_InternetQueryOptionW(IN HINTERNET hInternet,
						 IN DWORD dwOption,
						 OUT LPVOID lpBuffer,
						 IN OUT LPDWORD lpdwBufferLength);

#ifdef _UNICODE
	#ifdef  USE_ASTRATEK_WRAPPER
	#define ATK_InternetQueryOption  ATK_InternetQueryOptionW
	#else
	#define ATK_InternetQueryOption  InternetQueryOption
	#endif
#else
	#define ATK_InternetQueryOption  InternetQueryOptionA
#endif





#endif
