#include <windows.h>
#include <wininet.h>

#define NDEBUG
#include <debug.h>

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  return TRUE;
}

BOOL WINAPI
InternetAutodial(DWORD Flags, DWORD /* FIXME: should be HWND */ Parent)
{
  DPRINT1("InternetAutodial not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}

DWORD WINAPI
InternetAttemptConnect(DWORD Reserved)
{
  DPRINT1("InternetAttemptConnect not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}

BOOL WINAPI
InternetGetConnectedState(LPDWORD Flags, DWORD Reserved)
{
  DPRINT1("InternetGetConnectedState not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}

BOOL WINAPI
InternetAutodialHangup(DWORD Reserved)
{
  DPRINT1("InternetAutodialHangup not implemented\n");

  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  return FALSE;
}

BOOL WINAPI InternetCloseHandle( HINTERNET hInternet )
{
	DPRINT1("InternetCloseHandle not implemented\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	
	return FALSE;
}

BOOL WINAPI
InternetReadFile( HINTERNET hFile,
                  PVOID lpBuffer,
                  DWORD dwNumberOfBytesToRead,
                  PDWORD lpdwNumberOfBytesRead )
{
	DPRINT1("InternetReadFile not implemented\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	
	return FALSE;
}

BOOL WINAPI
HttpQueryInfoW( HINTERNET hRequest,
    						DWORD dwInfoLevel,
								PVOID lpvBuffer,
								PDWORD lpdwBufferLength,
								PDWORD lpdwIndex )
{
	DPRINT1("HttpQueryInfoW not implemented\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	
	return FALSE;
}

BOOL WINAPI
HttpQueryInfoA( HINTERNET hRequest,
    						DWORD dwInfoLevel,
								PVOID lpvBuffer,
								PDWORD lpdwBufferLength,
								PDWORD lpdwIndex )
{
	DPRINT1("HttpQueryInfoW not implemented\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	
	return FALSE;
}

BOOL WINAPI
HttpSendRequestW( HINTERNET hRequest,
                  LPCWSTR lpszHeaders,
                  DWORD dwHeadersLength,
                  PVOID lpOptional,
                  DWORD dwOptionalLength )
{
	DPRINT1("HttpSendRequestW not implemented\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	
	return FALSE;
}

BOOL WINAPI
HttpSendRequestA( HINTERNET hRequest,
                  LPCSTR lpszHeaders,
                  DWORD dwHeadersLength,
                  PVOID lpOptional,
                  DWORD dwOptionalLength )
{
	DPRINT1("HttpSendRequestA not implemented\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	
	return FALSE;
}

HINTERNET WINAPI
HttpOpenRequestW( HINTERNET hConnect,
                  LPCWSTR lpszVerb,
									LPCWSTR lpszObjectName,
									LPCWSTR lpszVersion,
									LPCWSTR lpszReferer,
									LPCWSTR* lpszAcceptTypes,
									DWORD dwFlags,
									DWORD dwContext )
{
	DPRINT1("HttpOpenRequestW not implemented\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	
	return FALSE;
}

HINTERNET WINAPI
HttpOpenRequestA( HINTERNET hConnect,
								  LPCSTR lpszVerb,
								  LPCSTR lpszObjectName,
								  LPCSTR lpszVersion,
								  LPCSTR lpszReferer,
								  LPCSTR *lpszAcceptTypes,
								  DWORD dwFlags,
					  		  DWORD dwContext )
{
	DPRINT1("HttpOpenRequestA not implemented\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	
	return FALSE;
}

HINTERNET WINAPI
InternetConnectW( HINTERNET hInternet,
                  LPCWSTR lpszServerName,
                  INTERNET_PORT nServerPort,
                  LPCWSTR lpszUsername,
                  LPCWSTR lpszPassword,
                  DWORD dwService,
                  DWORD dwFlags,
                  DWORD dwContext )
{
	DPRINT1("InternetConnectW not implemented\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	
	return FALSE;
}

HINTERNET WINAPI
InternetConnectA( HINTERNET hInternet,
                  LPCTSTR lpszServerName,
                  INTERNET_PORT nServerPort,
                  LPCSTR lpszUsername,
                  LPCSTR lpszPassword,
                  DWORD dwService,
                  DWORD dwFlags,
                  DWORD dwContext )
{
	DPRINT1("InternetConnectA not implemented\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	
	return FALSE;
}

BOOL WINAPI
InternetCrackUrlW( LPCWSTR lpszUrl,
                   DWORD dwUrlLength,
                   DWORD dwFlags,
                   LPURL_COMPONENTSW lpUrlComponents )
{
	DPRINT1("InternetCrackUrlW not implemented\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	
	return FALSE;
}

BOOL WINAPI
InternetCrackUrlA( LPCSTR lpszUrl,
                   DWORD dwUrlLength,
                   DWORD dwFlags,
                   LPURL_COMPONENTSA lpUrlComponents )
{
	DPRINT1("InternetCrackUrlA not implemented\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	
	return FALSE;	
}

HINTERNET WINAPI
InternetOpenW( LPCWSTR lpszAgent,
               DWORD dwAccessType,
               LPCWSTR lpszProxyName,
               LPCWSTR lpszProxyBypass,
               DWORD dwFlags )
{
	DPRINT1("InternetOpenW not implemented\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	
	return FALSE;	
}

HINTERNET WINAPI
InternetOpenA( LPCSTR lpszAgent,
               DWORD dwAccessType,
               LPCSTR lpszProxyName,
               LPCSTR lpszProxyBypass,
               DWORD dwFlags )
{
	DPRINT1("InternetOpenA not implemented\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	
	return FALSE;	
}


HINTERNET WINAPI
InternetOpenUrlA(
   HINTERNET hInternet,
   LPCSTR Url,
   LPCSTR Headers,
   DWORD dwHeadersLength,
   DWORD dwFlags,
   DWORD_PTR dwContext
)
{
   DPRINT1("InternetOpenUrlA not implemented\n");
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   
   return FALSE;  
}


HINTERNET WINAPI
InternetOpenUrlW(
   HINTERNET hInternet,
   LPCWSTR Url,
   LPCWSTR Headers,
   DWORD dwHeadersLength,
   DWORD dwFlags,
   DWORD_PTR dwContext
)
{
   DPRINT1("InternetOpenUrlW not implemented\n");
   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   
   return FALSE;  
}
