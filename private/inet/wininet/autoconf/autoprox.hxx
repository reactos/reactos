/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    autoprox.hxx

Abstract:

    Contains interface definition for a specialized DLL that automatically configurares WININET proxy
      information. The DLL can reroute proxy infromation based on a downloaded set of data that matches
      it registered MIME type.  The DLL can also control WININET proxy use, on a request by request basis.

    Contents:

Author:

    Arthur L Bierer (arthurbi) 01-Dec-1996

Revision History:

    01-Dec-1996 arthurbi
        Created

--*/

#ifndef _AUTOPROX_HXX_
#define _AUTOPROX_HXX_

//
// Callback functions implimented in WININET
//
#define INTERNET_MAX_URL_LENGTH 2049

class AUTO_PROXY_HELPER_APIS {

public:
    virtual
    BOOL IsResolvable(
        IN LPSTR lpszHost
        );
    
    virtual
    DWORD GetIPAddress(
        IN OUT LPSTR   lpszIPAddress,
        IN OUT LPDWORD lpdwIPAddressSize
        );

    virtual
    DWORD ResolveHostName(
        IN LPSTR lpszHostName,
        IN OUT LPSTR   lpszIPAddress,
        IN OUT LPDWORD lpdwIPAddressSize
        );

    virtual
    BOOL IsInNet(
        IN LPSTR   lpszIPAddress,
        IN LPSTR   lpszDest,
        IN LPSTR   lpszMask
        );

};


    

//
// external func declariations, note that the DLL does not have to export the full set of these 
//  functions, rather the DLL can export only the functions it impliments.
// 

/*

BOOL
CALLBACK
InternetProxyInfoInvalid (  // NOT implimented
    IN LPSTR lpszMime,
    IN LPSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN LPSTR lpszProxyHostName,
    IN DWORD dwProxyHostNameLength
    );
*/
#ifdef unix
extern "C"
#endif /* unix */
BOOL
CALLBACK
InternetDeInitializeAutoProxyDll(
    IN LPSTR lpszMime,
    IN DWORD dwReserved
    );

#ifdef unix
extern "C"
#endif /* unix */
BOOL
CALLBACK
InternetGetProxyInfo(
    IN LPCSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN LPSTR lpszUrlHostName,
    IN DWORD dwUrlHostNameLength,
    OUT LPSTR * lplpszProxyHostName,
    OUT LPDWORD lpdwProxyHostNameLength
    ) ;

/*
BOOL
CALLBACK
InternetGetProxyInfoEx(
    IN INTERNET_SCHEME tUrlProtocol,
    IN LPCSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN LPSTR lpszUrlHostName,
    IN DWORD dwUrlHostNameLength,
    IN INTERNET_PORT nUrlPort,
    OUT LPINTERNET_SCHEME lptProxyScheme,
    OUT LPSTR * lplpszProxyHostName,
    OUT LPDWORD lpdwProxyHostNameLength,
    OUT LPINTERNET_PORT lpProxyHostPort
    ) ;
*/
#ifdef unix
extern "C"
#endif /* unix */
BOOL
CALLBACK
InternetInitializeAutoProxyDll(
    IN DWORD                  dwVersion,
    IN LPSTR                  lpszDownloadedTempFile,
    IN LPSTR                  lpszMime,
    IN AUTO_PROXY_HELPER_APIS *pAutoProxyCallbacks,
    IN DWORD                  dwReserved
    );
 
#endif /* _AUTOPROX_HXX_ */
