#include <windows.h>
#include <winsock.h>
#include <wininet.h>
#undef INTERNET_MAX_URL_LENGTH
#include "autoprox.hxx"

#define INET_ASSERT
#define DEBUG_PRINT(a,b,c)
#define PERF_LOG(a,b)

DWORD
AUTO_PROXY_HELPER_APIS::ResolveHostName(
    IN LPSTR lpszHostName,
    IN OUT LPSTR   lpszIPAddress,
    IN OUT LPDWORD lpdwIPAddressSize
    )
 
/*++
 
Routine Description:
 
    Resolves a HostName to an IP address by using Winsock DNS.
 
Arguments:
 
    lpszHostName   - the host name that should be used.
 
    lpszIPAddress  - the output IP address as a string.
 
    lpdwIPAddressSize - the size of the outputed IP address string.
 
Return Value:
 
    DWORD
        Win32 error code.
 
--*/
 
{
    //
    // figure out if we're being asked to resolve a name or an address. If
    // inet_addr() succeeds then we were given a string respresentation of an
    // address
    //
 
    DWORD ipAddr;
    LPBYTE address;
    LPHOSTENT lpHostent;
    DWORD ttl;
    DWORD dwIPAddressSize;
    BOOL bFromCache = FALSE;
 
    DWORD error = ERROR_SUCCESS;
 
    ipAddr = inet_addr(lpszHostName);
    if (ipAddr != INADDR_NONE)
    {
        dwIPAddressSize = lstrlen(lpszHostName);
 
        if ( *lpdwIPAddressSize < dwIPAddressSize ||
              lpszIPAddress == NULL )
        {
            *lpdwIPAddressSize = dwIPAddressSize+1;
            error = ERROR_INSUFFICIENT_BUFFER;
            goto quit;
        }
 
        lstrcpy(lpszIPAddress, lpszHostName);
        goto quit;
    }
 
    ipAddr = 0;
    address = (LPBYTE) &ipAddr;
 
    //
    // now try to find the name or address in the cache. If it's not in the
    // cache then resolve it
    //
 
//    if (QueryHostentCache(lpszHostName, address, &lpHostent, &ttl)) {
//        bFromCache = TRUE;
//    } else {
      {
 
        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("resolving %q\n",
                    lpszHostName
                    ));
 
        PERF_LOG(PE_NAMERES_START, 0);
 
        lpHostent = gethostbyname(lpszHostName);
 
        PERF_LOG(PE_NAMERES_END, 0);
 
        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("%q %sresolved\n",
                    lpszHostName,
                    lpHostent ? "" : "NOT "
                    ));
 
 
        //
        // if we successfully resolved the name or address then add the
        // information to the cache
        //
 
        if (lpHostent != NULL)
        {
//            CacheHostent(lpszHostName, lpHostent, LIVE_DEFAULT);
        }
    }
 
 
    if ( lpHostent )
    {
        char *pszAddressStr;
        LPBYTE * addressList;
        struct  in_addr sin_addr;
 
        //     *(LPDWORD)&lpSin->sin_addr = *(LPDWORD)addressList[i];
        //              ((struct sockaddr_in*)lpSockAddr)->sin_addr
        //                   struct  in_addr sin_addr
 
        addressList         = (LPBYTE *)lpHostent->h_addr_list;
        *(LPDWORD)&sin_addr = *(LPDWORD)addressList[0] ;
 
        pszAddressStr = inet_ntoa (sin_addr);
 
        INET_ASSERT(pszAddressStr);
 
        dwIPAddressSize = lstrlen(pszAddressStr);
 
        if ( *lpdwIPAddressSize < dwIPAddressSize ||
              lpszIPAddress == NULL )
        {
            *lpdwIPAddressSize = dwIPAddressSize+1;
            error = ERROR_INSUFFICIENT_BUFFER;
            goto quit;
        }
 
        lstrcpy(lpszIPAddress, pszAddressStr);
 
        goto quit;
 
    }
 
    //
    // otherwise, if we get here its an error
    //
 
    error = ERROR_INTERNET_NAME_NOT_RESOLVED;
 
quit:
 
    if (bFromCache) {
 
        INET_ASSERT(lpHostent != NULL);
 
//        ReleaseHostentCacheEntry(lpHostent);
    }

    return error;
}

BOOL
AUTO_PROXY_HELPER_APIS::IsResolvable(
    IN LPSTR lpszHost
    )
 
/*++
 
Routine Description:
 
    Determines wheter a HostName can be resolved.  Performs a Winsock DNS query,
      and if it succeeds returns TRUE.
 
Arguments:
 
    lpszHost   - the host name that should be used.
 
Return Value:
 
    BOOL
        TRUE - the host is resolved.
 
        FALSE - could not resolve.
 
--*/
 
{
 
    DWORD dwDummySize;
    DWORD error;
 
    error = ResolveHostName(
                lpszHost,
                NULL,
                &dwDummySize
                );
 
    if ( error == ERROR_INSUFFICIENT_BUFFER )
    {
        return TRUE;
    }
    else
    {
        INET_ASSERT(error != ERROR_SUCCESS );
        return FALSE;
    }
 
}
DWORD
AUTO_PROXY_HELPER_APIS::GetIPAddress(
    IN OUT LPSTR   lpszIPAddress,
    IN OUT LPDWORD lpdwIPAddressSize
    )
 
/*++
 
Routine Description:
 
    Acquires the IP address string of this client machine WININET is running on.
 
Arguments:
 
    lpszIPAddress   - the IP address of the machine, returned.
 
    lpdwIPAddressSize - size of the IP address string.
 
Return Value:
 
    DWORD
        Win32 Error.
 
--*/
 
{
 
    CHAR szHostBuffer[255];
    int serr;
 
    serr = gethostname(
                szHostBuffer,
                255-1 
                );
 
    if ( serr != 0)
    {
        return ERROR_INTERNET_INTERNAL_ERROR;
    }
 
    return ResolveHostName(
                szHostBuffer,
                lpszIPAddress,
                lpdwIPAddressSize
                );
 
}

BOOL
AUTO_PROXY_HELPER_APIS::IsInNet(
    IN LPSTR   lpszIPAddress,
    IN LPSTR   lpszDest,
    IN LPSTR   lpszMask
    )
 
/*++
 
Routine Description:
 
    Determines whether a given IP address is in a given dest/mask IP address.
 
Arguments:
 
    lpszIPAddress   - the host name that should be used.
 
    lpszDest        - the IP address dest to check against.
 
    lpszMask        - the IP mask string
 
Return Value:
 
    BOOL
        TRUE - the IP address is in the given dest/mask
 
        FALSE - the IP address is NOT in the given dest/mask
 
--*/
 
{
    DWORD dwDest, dwIpAddr, dwMask;
 
    INET_ASSERT(lpszIPAddress);
    INET_ASSERT(lpszDest);
    INET_ASSERT(lpszMask);
 
    dwIpAddr = inet_addr(lpszIPAddress);
    dwDest = inet_addr(lpszDest);
    dwMask = inet_addr(lpszMask);
 
    if ( dwDest   == INADDR_NONE ||
         dwIpAddr == INADDR_NONE  )
 
    {
        INET_ASSERT(FALSE);
        return FALSE;
    }
 
        if ( (dwIpAddr & dwMask) != dwDest)
    {
        return FALSE;
        }
 
    //
    // Pass, its Matches.
    //
 
    return TRUE;
}

