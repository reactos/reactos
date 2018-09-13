/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    apdetect.cxx

Abstract:

    This is the overall generic wrappers and entry code to the 
      auto-proxy, auto-detection code, that sends DHCP informs,
      and mundges through DNS to find an URL for proxy configuration

Author:

    Arthur Bierer (arthurbi)  15-Jul-1998

Environment:

    User Mode - Win32

Revision History:

    Arthur Bierer (arthurbi)  15-Jul-1998
        Created

    Josh Cohen (joshco)     7-oct-1998
        added proxydetecttype
        
--*/

#include <wininetp.h>
#include "aproxp.h"

#ifndef UNIX
INTERNETAPI
#endif /* UNIX */

#include "apdetect.h"

// this isnt the most efficient thing
//  should probably move this to autoprox class...
//    
     
DWORD
WINAPI
GetProxyDetectType(VOID) 
{
    static DWORD _adType = PROXY_AUTO_DETECT_TYPE_DEFAULT;
    static DWORD UseCached = 0;

    if (! UseCached ) 
    {
        InternetReadRegistryDword("AutoProxyDetectType",
                              (LPDWORD)&(_adType)
                              );

        if (_adType & PROXY_AUTO_DETECT_CACHE_ME ) 
        {
             UseCached = 1;
        }

    }
    return _adType;
}
  
BOOL
WINAPI
DetectAutoProxyUrl(
    IN OUT LPSTR lpszAutoProxyUrl,
    IN DWORD dwAutoProxyUrlLength,
    IN DWORD dwDetectFlags
    )
{
    BOOL fRet = FALSE;
    DWORD error;

    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "DetectAutoProxyUrl",
                 "%x, %u",
                 lpszAutoProxyUrl,
                 dwAutoProxyUrlLength                 
                 ));

    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) {            
            goto quit;
        }
    }

    error = LoadWinsock();

    if ( error != ERROR_SUCCESS ) {
        goto quit;
    }

    {
        CIpConfig Interfaces;
        if ( (GetProxyDetectType() & PROXY_AUTO_DETECT_TYPE_DHCP) & dwDetectFlags ) {

            //Interfaces.GetAdapterInfo();       
            if ( Interfaces.DoInformsOnEachInterface(lpszAutoProxyUrl, dwAutoProxyUrlLength) )
            {
                //printf("success on DHCP search: got %s\n", szAutoProxyUrl);
                fRet = TRUE;
                goto quit;
            }
        }
        if ( (GetProxyDetectType() & PROXY_AUTO_DETECT_TYPE_DNS_A) & dwDetectFlags ) {
    
            if ( QueryWellKnownDnsName(lpszAutoProxyUrl, dwAutoProxyUrlLength) == ERROR_SUCCESS)
            {
                //printf("success on well qualified name search: got %s\n", szAutoProxyUrl);
                fRet = TRUE;
                goto quit;
            }
        }
    }

quit:

    DEBUG_LEAVE(fRet);

    return fRet;
}

