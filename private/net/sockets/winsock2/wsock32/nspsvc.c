/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    Nspsvc.c

Abstract:

    This module contains support for the Name Space Provider APIs
    GetService() & SetService(). This is a wrapper that uses the
    newer enumeration APIs to emulate these 1.1 functions

Author:

    Chuck Y Chan   (ChuckC)      25-May-1994

Revision History:
    Arnold Miller  <arnoldm)      10-April-1996 Put into winsock2

--*/

#ifdef CHICAGO
#undef UNICODE
#else
#define UNICODE
#define _UNICODE
#endif

#include "winsockp.h"
#ifdef CHICAGO
#include "imported.h"
#endif
#include <nspmisc.h>
#include <rpc.h>
#include <stdlib.h>
#include <align.h>


AFPROTOCOLS afp = {AF_INET, IPPROTO_UDP};

extern GUID HostnameGuid, AddressGuid;

#define LKXBUFSIZE (sizeof(WSAQUERYSET) + 500)  // Is 500 bytes enough?

//
// Forward declare
//

INT
APIENTRY
SetServiceWorker (
    IN     DWORD                dwNameSpace,
    IN     DWORD                dwOperation,
    IN     DWORD                dwFlags,
    IN     LPSERVICE_INFO       lpServiceInfo,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    IN     BOOL                 fUnicodeCaller,
    IN OUT LPDWORD              lpdwStatusFlags
) ;

INT
APIENTRY
GetServiceWorker (
    IN     DWORD                dwNameSpace,
    IN     LPGUID               lpGuid,
    IN     LPTSTR               lpServiceName,
    IN     DWORD                dwProperties,
    IN OUT LPVOID               lpBuffer,
    IN OUT LPDWORD              lpdwBufferSize,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    IN     BOOL                 fUnicodeCaller
) ;

DWORD
APIENTRY
PackBlobWorker(
    IN    LPTSTR                   lpTypeName,
    IN    DWORD                    dwValueCount,
    IN    PSERVICE_TYPE_VALUE_ABS  Values,
    OUT   PBYTE                   *lppBuffer,
    OUT   DWORD                   *lpdBufferSize,
    IN    BOOL                     fIsAnsi
);


#if defined(UNICODE)
DWORD
AllocateUnicodeString (
    IN     LPSTR   lpAnsi,
    IN OUT LPTSTR *lppUnicode
) ;
#endif

DWORD
PackBuffer (
    IN     DWORD           dwNameSpace,
    IN     LPGUID          lpServiceType,
    IN     LPSERVICE_INFO  lpServiceInfo,
    IN     BOOL            fUnicodeCaller,
    IN     LPTSTR          lpServiceName,
    IN OUT LPBYTE         *lppStart,
    IN OUT LPBYTE         *lppEnd,
    IN OUT LPDWORD         lpdwBytesTooFew
);

VOID
PackString(
    IN     LPTSTR  lpSource,
    IN     LPBYTE  lpStartVar,
    IN OUT LPBYTE *lppEnd,
    IN OUT LPBYTE *lppDest,
    IN OUT LPDWORD lpdwBytesTooFew,
    IN     BOOL    fConvertToAnsi
) ;

INT
SSRegistrySettings(
    IN DWORD   dwOperation,
    IN LPTSTR  lpServiceTypeName,
    IN LPGUID  lpServiceType
   );

VOID
FreeAllocatedClassInfo(BOOL fUnicodeCaller,
                       DWORD dwNumberOf,
                       LPWSANSCLASSINFO lpcli);
//
// Utility functions
//

//
// Do the registry add or delete for a SetService call
//

INT
SSRegistrySettings(
    IN DWORD   dwOperation,
    IN LPTSTR  lpServiceTypeName,
    IN LPGUID  lpServiceType
   )
{
    HKEY hKey;
    DWORD dwDisposition;
    INT err;

#ifdef CHICAGO
    //
    // Try to load RPCRT4.DL
    //

    if(!DemandLoadRpcrt4())
    {
        return(ERROR_PROC_NOT_FOUND);   // bugbug?
    }
#endif

    //
    // Open the key that stores the name space provider info.
    //

    err = RegCreateKeyEx(
                  HKEY_LOCAL_MACHINE,
                  NSP_SERVICE_KEY_NAME,
                  0,
                  TEXT(""),
                  REG_OPTION_NON_VOLATILE,
                  KEY_READ | KEY_WRITE,
                  NULL,
                  &hKey,
                  &dwDisposition
                  );

    if(err == ERROR_SUCCESS)
    {
        if (dwOperation == SERVICE_ADD_TYPE)
        {
            //
            // if adding a type, create the key if need & set a <GUID>
            // value to have a { XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX }
            // representation of the GUID.
            //

            HKEY    hKeyService = NULL ;
            LPTSTR  lpGuidString = NULL, lpTmpString = NULL ;

            //
            // Open the key corresponding to the service (create if not there).
            //

            err = RegCreateKeyEx(
                      hKey,
                      lpServiceTypeName,
                      0,
                      TEXT(""),
                      REG_OPTION_NON_VOLATILE,
                      KEY_READ | KEY_WRITE,
                      NULL,
                      &hKeyService,
                      &dwDisposition
                      );

            if(err == ERROR_SUCCESS)
            {
                //
                // Convert the GUID to string
                //

#ifdef CHICAGO
                err = lpUuidToString(
#else

                err = UuidToString(
#endif
                          lpServiceType,
                          &lpTmpString) ;

                if (err == NO_ERROR)
                {
                    //
                    // add the braces before & after
                    //

                    if (lpGuidString = ALLOCATE_HEAP((_tcslen(lpTmpString) + 3)
                                                     * sizeof(TCHAR)))
                    {
                        _tcscpy(lpGuidString, TEXT("{")) ;
                        _tcscat(lpGuidString, lpTmpString) ;
                        _tcscat(lpGuidString, TEXT("}")) ;

                        //
                        // write it out
                        //
                        err = RegSetValueEx(
                                  hKeyService,
                                  TEXT("GUID"),
                                  0,
                                  REG_SZ,
                                  (LPBYTE) lpGuidString,
                                  (_tcslen(lpGuidString) + 1) * sizeof(TCHAR)) ;

                        (void) FREE_HEAP(lpGuidString) ;
                    }
                    else
                    {
                        err = GetLastError() ;
                    }

#ifdef CHICAGO
                    (void) lpRpcStringFree(&lpTmpString) ;
#else
                    (void) RpcStringFree(&lpTmpString) ;
#endif
                }

                (void) RegCloseKey(hKeyService) ;

            }
            else
            {
                //
                // deleting the sevice type.just nuke the key
                //
                err = RegDeleteKey(
                          hKey,
                          lpServiceTypeName) ;
            }
        }
        (void) RegCloseKey(hKey) ;
    }
    return(err);
}


// Convert a SERVICE_ADDRESSES structure into a CSADDR_INFO structure.
// This will allocate whatever memory is required leaving the caller to
// free it. A return value of NULL indicates a memory allocation error
//

CSADDR_INFO *
SrvAddrsToCsAddrs(
    PSERVICE_ADDRESSES psa,
    PDWORD  pdwNumberOfCsAddrs)
{
    DWORD  dwTotalSize;
    DWORD  dwX;
    PCSADDR_INFO pcsa;

    //
    // first compute the  size of the memory buffer we will need
    //

    *pdwNumberOfCsAddrs = psa->dwAddressCount;

    //
    // Account for all of the fixed-sized structures
    //
    dwTotalSize = sizeof(CSADDR_INFO) * psa->dwAddressCount;

    pcsa = (PCSADDR_INFO)ALLOCATE_HEAP(dwTotalSize);
    if(pcsa)
    {
        //
        // ready to repack the information.
        // BUGBUG. Don't know what to put in the iProtocol field
        // of the CSADDR_INFO since there is no universal value
        // for the "default" protocol.
        //

        PCSADDR_INFO pcsa1 = pcsa;

        memset(pcsa, 0, dwTotalSize);

        //
        // convert each SERVICE_ADDRESS to a CSADD_ADDR / SOCKET_ADDR pair.
        // Note BUGBUG above!
        //

        for(dwX = 0; dwX < psa->dwAddressCount; dwX++)
        {
            PSOCKET_ADDRESS psocka = &pcsa1->LocalAddr;

//            pcsa1->iSocketType = (int)psa->Addresses[dwX].dwAddressType;
            psocka->iSockaddrLength = (int)psa->Addresses[dwX].dwAddressLength;
            psocka->lpSockaddr = (LPSOCKADDR)psa->Addresses[dwX].lpAddress;
            psocka->lpSockaddr->sa_family =
                        (USHORT)psa->Addresses[dwX].dwAddressType;
            pcsa1++;
        }
    }
    return(pcsa);
}

//
// And the reverse: convert a CSADDR_INFO structure into a
// SERVICE_ADDRESSES
//

PSERVICE_ADDRESSES
CsAddrsToSrvAddrs(
    LPCSADDR_INFO pcsadr,
    DWORD  dwNumberOfCsAddr)
{
    DWORD  dwTotalSize;
    DWORD  dwX;
    PSERVICE_ADDRESSES psas;

    //
    // Compute the total memory size needed.
    //

    if(dwNumberOfCsAddr == 0)
    {
        return(NULL);
    }
    dwTotalSize =  sizeof(SERVICE_ADDRESSES) +
                   (sizeof(SERVICE_ADDRESS) * (dwNumberOfCsAddr - 1));


    psas = (PSERVICE_ADDRESSES)ALLOCATE_HEAP(dwTotalSize);
    if(psas)
    {
        //
        // The buffer is allocated with the SERVICE_ADDRESSES structure
        // first, followed by the requisite number of SERVICE_ADDRESS
        // structures.
        //

        PSERVICE_ADDRESS psa = &psas->Addresses[0];


        psas->dwAddressCount = dwNumberOfCsAddr;
        for(dwX = 0; dwX < dwNumberOfCsAddr; dwX++)
        {
            psa->dwAddressType = pcsadr[dwX].RemoteAddr.lpSockaddr->sa_family;
            psa->dwAddressFlags = 0;
            psa->dwAddressLength = pcsadr[dwX].RemoteAddr.iSockaddrLength;
            psa->dwPrincipalLength = 0;
            psa->lpPrincipal = 0;
            psa->lpAddress = (PBYTE)pcsadr[dwX].RemoteAddr.lpSockaddr;
            psa++;
        }
    }
    return(psas);
}

//
// Convert a SERVICE_TYPE_VALUE into a CLASSINFO structure. This is
// straight-forward requiring only switching around a few values.
// returns a pointer to the structure allocated using LocalAlloc.
//

LPWSANSCLASSINFO
ServiceTypeToClassInfo(
    BOOL   fUnicodeCaller,
    DWORD  dwNumberOf,
    PSERVICE_TYPE_VALUE_ABS pstv)
{
    LPWSANSCLASSINFO lpci, lpci1;
    DWORD dwSaveNumber = dwNumberOf;

    lpci = ALLOCATE_HEAP(sizeof(WSANSCLASSINFO) * dwNumberOf);
    if(lpci1 = lpci)
    {
        memset(lpci, 0,  sizeof(WSANSCLASSINFO)  * dwNumberOf);
        while(dwNumberOf--)
        {
#ifdef UNICODE
            if(!fUnicodeCaller)
            {
                if(AllocateUnicodeString((PCHAR)pstv->lpValueName,
                                         &lpci1->lpszName) != NO_ERROR)
                {
                    FreeAllocatedClassInfo(fUnicodeCaller,
                                           dwSaveNumber,
                                           lpci);
                    lpci = 0;
                    break;
                }
            }
            else
#endif
            {
                lpci1->lpszName =  pstv->lpValueName;
            }
            lpci1->dwNameSpace = pstv->dwNameSpace;
            lpci1->dwValueType = pstv->dwValueType;
            lpci1->dwValueSize = pstv->dwValueSize;
            lpci1->lpValue = pstv->lpValue;
            lpci1++;
            pstv++;
        }
    }
    return(lpci);
}
        
//
// Free an allocated class info
//
VOID
FreeAllocatedClassInfo(BOOL fUnicodeCaller,
                       DWORD dwNumberOf,
                       LPWSANSCLASSINFO lpci)
{
    LPWSANSCLASSINFO lpci1 = lpci;
    
#ifdef UNICODE
    if(!fUnicodeCaller)
    {
        while(dwNumberOf--)
        {
            FREE_HEAP(lpci1->lpszName);
            lpci1++;
        }
    }
#endif
    FREE_HEAP(lpci);
}

//
// Actual WSA functions
//

#if defined(UNICODE)

INT
APIENTRY
SetServiceA (
    IN     DWORD                dwNameSpace,
    IN     DWORD                dwOperation,
    IN     DWORD                dwFlags,
    IN     LPSERVICE_INFOA      lpServiceInfo,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    IN OUT LPDWORD              lpdwStatusFlags
    )
/*++

Routine Description:

    ANSI version of SetService

Arguments:

    See Unicode version of SetService

Return Value:

    See Unicode version of SetService

--*/

{
    LPWSTR lpUnicodeServiceName = NULL ;
    LPWSTR lpUnicodeComment = NULL ;
    LPWSTR lpUnicodeLocale = NULL ;
    LPWSTR lpUnicodeMachineName = NULL ;

    DWORD  err =  NO_ERROR ;
    INT    res = 0 ;

    LPSERVICE_INFOW lpUnicodeServiceInfo = NULL ;

    //
    // check parameters. majority are checked in the worker,
    // but we need access the structure below to map to Unicode,
    // so we check it here.
    //
    if (!lpServiceInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER) ;
        return -1 ;
    }

    //
    // allocate the necessary memory for the unicode buffer
    //
    lpUnicodeServiceInfo =
        (LPSERVICE_INFOW) ALLOCATE_HEAP(sizeof(SERVICE_INFOW)) ;
    if (!lpUnicodeServiceInfo)
    {
        goto ExitPoint ;
    }

    //
    // allocate the necessary memory for the service name string
    //
    if (err = AllocateUnicodeString(lpServiceInfo->lpServiceName,
                                    &lpUnicodeServiceName))
    {
            goto ExitPoint ;
    }

    //
    // allocate the necessary memory for the service comment string
    //
    if (err = AllocateUnicodeString(lpServiceInfo->lpComment,
                                    &lpUnicodeComment))
    {
            goto ExitPoint ;
    }

    //
    // allocate the necessary memory for the locale string
    //
    if (err = AllocateUnicodeString(lpServiceInfo->lpLocale,
                                    &lpUnicodeLocale))
    {
            goto ExitPoint ;
    }


    //
    // allocate the necessary memory for the machine name string
    //
    if (err = AllocateUnicodeString(lpServiceInfo->lpMachineName,
                                    &lpUnicodeMachineName))
    {
            goto ExitPoint ;
    }

    //
    // setup the Unicode Structure
    //
    lpUnicodeServiceInfo->lpServiceType =  lpServiceInfo->lpServiceType ;
    lpUnicodeServiceInfo->lpServiceName =  lpUnicodeServiceName ;
    lpUnicodeServiceInfo->lpComment =  lpUnicodeComment ;
    lpUnicodeServiceInfo->lpLocale =  lpUnicodeLocale ;
    lpUnicodeServiceInfo->dwDisplayHint =  lpServiceInfo->dwDisplayHint ;
    lpUnicodeServiceInfo->dwVersion =  lpServiceInfo->dwVersion ;
    lpUnicodeServiceInfo->dwTime =  lpServiceInfo->dwTime ;
    lpUnicodeServiceInfo->lpMachineName =  lpUnicodeMachineName ;
    lpUnicodeServiceInfo->lpServiceAddress = lpServiceInfo->lpServiceAddress ;
    lpUnicodeServiceInfo->ServiceSpecificInfo.cbSize =
        lpServiceInfo->ServiceSpecificInfo.cbSize ;
    lpUnicodeServiceInfo->ServiceSpecificInfo.pBlobData =
        lpServiceInfo->ServiceSpecificInfo.pBlobData ;

    //
    // call the worker routine
    //
    res =     SetServiceWorker (dwNameSpace,
                                dwOperation,
                                dwFlags,
                                lpUnicodeServiceInfo,
                                lpServiceAsyncInfo,
                                FALSE,                // BLOB is not Unicode
                                lpdwStatusFlags )  ;

ExitPoint:

    if (lpUnicodeServiceName)
        FREE_HEAP(lpUnicodeServiceName) ;
    if (lpUnicodeComment)
        FREE_HEAP(lpUnicodeComment) ;
    if (lpUnicodeLocale)
        FREE_HEAP(lpUnicodeLocale) ;
    if (lpUnicodeMachineName)
        FREE_HEAP(lpUnicodeMachineName) ;

    if (lpUnicodeServiceInfo)
        FREE_HEAP(lpUnicodeServiceInfo) ;

    //
    // if this routine hit an error, set it as last error. else
    // rely on the worker routine to set it.
    //
    if (err)
    {
        SetLastError(err) ;
        res = -1 ;
    }

    return(res) ;
}

#else // defined(UNICODE)

INT
APIENTRY
SetServiceW (
    IN     DWORD                dwNameSpace,
    IN     DWORD                dwOperation,
    IN     DWORD                dwFlags,
    IN     LPSERVICE_INFOW      lpServiceInfo,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    IN OUT LPDWORD              lpdwStatusFlags
    )
{

    SetLastError( ERROR_NOT_SUPPORTED );
    return -1;

} // SetServiceW

#endif // defined(UNICODE)

INT
APIENTRY
SetService (
    IN     DWORD                dwNameSpace,
    IN     DWORD                dwOperation,
    IN     DWORD                dwFlags,
    IN     LPSERVICE_INFO       lpServiceInfo,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    IN OUT LPDWORD              lpdwStatusFlags
    )
/*++

Routine Description:

    Unicode version of SetService. This function is called to
    set information about a network service.

Arguments:

    dwNameSpace         - Name space affected by the operation. NS_DEFAULT
                          is used to mean all default name spaces.

    dwOperation         - Operation to perform, like SERVICE_REGISTER,
                          SERVICE_DEREGISTER.

    dwFlags             - flags for the opertation, like SERVICE_FLAG_DEFER

    dwServiceInfo       - pointer to SERVICE_INFO structure containing the
                          information to set.

    lpServiceAsyncInfo  - currently must ne NULL.

    lpdwStatusFlags     - used to return info bits about the operation.

Return Value:



--*/

{
    INT    res ;

    //
    // call the worker routine
    //
    res =     SetServiceWorker (dwNameSpace,
                                dwOperation,
                                dwFlags,
                                lpServiceInfo,
                                lpServiceAsyncInfo,
#if defined(UNICODE)
                                TRUE,                  // BLOB is Unicode
#else
                                FALSE,
#endif
                                lpdwStatusFlags )  ;

    return res ;
}

#if defined (UNICODE)

INT
APIENTRY
GetServiceA (
    IN     DWORD                dwNameSpace,
    IN     LPGUID               lpGuid,
    IN     LPSTR                lpServiceName,
    IN     DWORD                dwProperties,
    IN OUT LPVOID               lpBuffer,
    IN OUT LPDWORD              lpdwBufferSize,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo
    )
/*++

Routine Description:

    ANSI verson of GetService. Obtains information about the service.
    If dwNameSpace is NS_DEFAULT,may return >1 NS_SERVICE_INFO structures,
    one for each name space.

Arguments:

    See GetServiceW.

Return Value:

    See GetServiceW.

--*/

{
    INT res ;
    DWORD err ;
    LPWSTR lpUnicodeServiceName = NULL ;

    //
    // allocate the necessary memory for the service name string
    //
    if (err = AllocateUnicodeString(lpServiceName,
                                    &lpUnicodeServiceName))
    {
        res = -1 ;
        goto ExitPoint ;
    }

    res =    GetServiceWorker ( dwNameSpace,
                                lpGuid,
                                lpUnicodeServiceName,
                                dwProperties,
                                lpBuffer,
                                lpdwBufferSize,
                                lpServiceAsyncInfo,
                                FALSE ) ;   // we dont want Unicode Blob
ExitPoint:

    if (lpUnicodeServiceName)
        FREE_HEAP(lpUnicodeServiceName) ;

    return res ;
}

#else // defined(UNICODE)

INT
APIENTRY
GetServiceW (
    IN     DWORD                dwNameSpace,
    IN     LPGUID               lpGuid,
    IN     LPWSTR               lpServiceName,
    IN     DWORD                dwProperties,
    IN OUT LPVOID               lpBuffer,
    IN OUT LPDWORD              lpdwBufferSize,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo
    )
{

    SetLastError( ERROR_NOT_SUPPORTED );
    return -1;

} // GetServiceW

#endif // defined(UNICODE)

INT
APIENTRY
GetService (
    IN     DWORD                dwNameSpace,
    IN     LPGUID               lpGuid,
    IN     LPTSTR               lpServiceName,
    IN     DWORD                dwProperties,
    IN OUT LPVOID               lpBuffer,
    IN OUT LPDWORD              lpdwBufferSize,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo
    )
/*++

Routine Description:

    Unicode verson of GetService. Obtains information about the service.
    If dwNameSpace is NS_DEFAULT,may return >1 NS_SERVICE_INFO structures,
    one for each name space.

Arguments:

    dwNameSpace         - name spaces to query. if NS_DEFAULT, then all default
                          name spaces.

    lpGuid              - GUID defining type of service to query

    lpServiceName       - name of service instance to query

    dwProperties        - what properties to ask for

    lpBuffer            - return buffer to be filled with NS_SERVICE_INFO
                          structures

    lpdwBufferSize      - on input, size of buffer. used to return required
                          size if buffer too small.

    lpServiceAsyncInfo  - currently must be NULL.

Return Value:

    number of structures returned if successful. zero if no matching
    structure returned. -1 if an error occured.


--*/

{
    INT res ;

    res =    GetServiceWorker ( dwNameSpace,
                                lpGuid,
                                lpServiceName,
                                dwProperties,
                                lpBuffer,
                                lpdwBufferSize,
                                lpServiceAsyncInfo,
#if defined(UNICODE)
                                TRUE ) ;   // yes, we want Unicode Blob
#else
                                FALSE ) ;   // no Unicode Blob
#endif
    return res ;
}

//
// Worker routines
//


#if OLDXBYY

INT
APIENTRY
SetServiceWorker (
    IN     DWORD                dwNameSpace,
    IN     DWORD                dwOperation,
    IN     DWORD                dwFlags,
    IN     LPSERVICE_INFO       lpServiceInfo,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    IN     BOOL                 fUnicodeCaller,
    IN OUT LPDWORD              lpdwStatusFlags
    )
/*++

Routine Description:

    Worker function for SetService.

Arguments:

   As SetServiceW, except that fUnicodeCaller has been added
   which tells us if the call originated as Unicode or Ansi. We
   need distinguish because of the BLOB data which we cannot interpret.

Return Value:

   Zero if no error. SOCKET_ERROR otherwise. Caller may issue
   WSAGetLastError() to get more detailed error info.

--*/

{
    DWORD err = NO_ERROR ;
    BOOL fOneSucceeded = FALSE ;
    LPBYTE lpTmpBlobData = NULL ;
    DWORD  dwTmpBlobDataSize ;
    PLIST_ENTRY listEntry;
    SERVICE_INFO TmpSvcInfo ;

    //
    // check parameters.
    // we currently dont support async operation.
    //

    if ( ARGUMENT_PRESENT( lpServiceAsyncInfo ) )
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return -1;
    }

    if ( !ARGUMENT_PRESENT( lpServiceInfo ) ||
         !ARGUMENT_PRESENT( lpdwStatusFlags ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }

    *lpdwStatusFlags = 0 ;

    //
    // If our NSP internal structures have not yet been initialized,
    // initialize them now.
    //

    if ( !NspInitialized )
    {
        err = InitializeNsp( );
        if ( err != NO_ERROR )
        {
            SetLastError( err );
            return -1;
        }
    }

    //
    // we make a copy of the structure since we may modify it.
    //

    memcpy(&TmpSvcInfo,lpServiceInfo,sizeof(TmpSvcInfo)) ;

    //
    // the only operation where we take explicit action is ADD_TYPE and
    // DELETE_TYPE. everything else gets done by providers.
    //

    if ((dwOperation == SERVICE_ADD_TYPE) ||
        (dwOperation == SERVICE_DELETE_TYPE))
    {
        LPTSTR                      lpServiceTypeName = NULL ;
#ifndef CHICAGO
        LPSERVICE_TYPE_INFO         lpServiceTypeInfo ;
#endif
        LPSERVICE_TYPE_INFO_ABS     lpSvcTypeInfoAbs  ;

        //
        // make sure Blob has valid data
        //
        if ((TmpSvcInfo.ServiceSpecificInfo.cbSize == 0) ||
            (TmpSvcInfo.ServiceSpecificInfo.pBlobData == NULL))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return -1;
        }

        //
        // get the name of the service type being added.
        // if not unicode caller, we need convert to unicode.
        //

        lpSvcTypeInfoAbs  = (LPSERVICE_TYPE_INFO_ABS)
                                 TmpSvcInfo.ServiceSpecificInfo.pBlobData ;
#if defined(UNICODE)
        if (fUnicodeCaller)
        {
#endif
             lpServiceTypeName = lpSvcTypeInfoAbs->lpTypeName ;
#if defined(UNICODE)
        }
        else
        {
             err = AllocateUnicodeString(
                       ((LPSERVICE_TYPE_INFO_ABSA)lpSvcTypeInfoAbs)->lpTypeName,
                       &lpServiceTypeName)  ;
             if (err != NO_ERROR)
             {
                 SetLastError(err) ;
                 return -1 ;
             }
        }
#endif

        //
        // make sure the service type name is not empty
        //
        if ( lpServiceTypeName  &&
             *lpServiceTypeName != 0 )
        {
            err = SSRegistrySettings(
                           dwOperation,
                           lpServiceTypeName,
                           TmpSvcInfo.lpServiceType);
        }
        else
        {
            err =  ERROR_INVALID_PARAMETER ;
        }

        if ( err != NO_ERROR )
        {
            SetLastError(err) ;
#if defined(UNICODE)
            if (!fUnicodeCaller)
                (void) FREE_HEAP(lpServiceTypeName) ;
#endif
            return -1;
        }

        //
        // bail out if have error here
        //

        if ( err != NO_ERROR )
        {
#if defined(UNICODE)
            if (!fUnicodeCaller)
                (void) FREE_HEAP(lpServiceTypeName) ;
#endif
            SetLastError(err) ;
            return -1;
        }


        //
        // For ADD and DELETE type, the blob may contain pointers.
        // We convert this to be self relative before passing on to
        // providers.
        //
        err = PackBlobWorker(
                  lpSvcTypeInfoAbs->lpTypeName,
                  lpSvcTypeInfoAbs->dwValueCount,
                  lpSvcTypeInfoAbs->Values,
                  &lpTmpBlobData,
                  &dwTmpBlobDataSize,
                  !fUnicodeCaller) ;

        //
        // overwrite the pointers in our private copy
        //

        if (err == NO_ERROR)
        {
            TmpSvcInfo.ServiceSpecificInfo.pBlobData =
                lpTmpBlobData ;
            TmpSvcInfo.ServiceSpecificInfo.cbSize =
                dwTmpBlobDataSize ;
        }

        //
        // if not unicode, we allocated this string.
        // free it now as we are done with it.
        //
#if defined(UNICODE)
        if (!fUnicodeCaller)
            (void) FREE_HEAP(lpServiceTypeName) ;
        lpServiceTypeName = NULL ;
#endif

        //
        // cleanup, & exit if error
        //

        if ( err != NO_ERROR )
        {
            SetLastError(err) ;
            return -1;
        }
    }

    //
    // Loop through our name spaces, performing the operation
    // requested. This loop only goes thru more than one iteration if
    // the nameSpace is 0 (Default NameSpaces).
    //

    for ( listEntry = NameSpaceListHead.Flink;
          listEntry != &NameSpaceListHead;
          listEntry = listEntry->Flink )
    {
        PNAME_SPACE_INFO lpNameSpace;
        DWORD error ;

        lpNameSpace = CONTAINING_RECORD(
                        listEntry,
                        NAME_SPACE_INFO,
                        NameSpaceListEntry
                        );

        //
        // Determine whether we should make use of this name space
        // provider.
        //

        if ( !IsValidNameSpace( dwNameSpace, lpNameSpace ) )
        {
            continue;
        }

        //
        // make sure function is supported.
        //

        if (lpNameSpace->SetServiceProc == NULL)
        {
            continue;
        }

        error = lpNameSpace->SetServiceProc(dwOperation,
                                            dwFlags,
                                            fUnicodeCaller,
                                            &TmpSvcInfo) ;
        if (error != NO_ERROR)
        {
            *lpdwStatusFlags |= SET_SERVICE_PARTIAL_SUCCESS ;
            SetLastError(error) ;
        }
        else
        {
            fOneSucceeded = TRUE ;
        }
    }

    if (lpTmpBlobData)
        (void) FREE_HEAP(lpTmpBlobData) ;

    return (fOneSucceeded ? 0 : -1) ;

}

INT
APIENTRY
GetServiceWorker (
    IN     DWORD                dwNameSpace,
    IN     LPGUID               lpGuid,
    IN     LPTSTR               lpServiceName,
    IN     DWORD                dwProperties,
    IN OUT LPVOID               lpBuffer,
    IN OUT LPDWORD              lpdwBufferSize,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    IN     BOOL                 fUnicodeCaller
)
/*++

Routine Description:

    Worker function for GetService.

Arguments:

   As GetServiceW, except that fUnicodeCaller has been added
   which tells us if the call originated as Unicode or Ansi. We
   need distinguish because of the BLOB data which we cannot interpret.

Return Value:

   Number of NS_SERVICE_INFO structures obtained if no error.
   SOCKET_ERROR otherwise. Caller may issue WSAGetLastError()
   to get more detailed error info.

--*/

{
    DWORD          err = NO_ERROR ;
    INT            cEntries = 0 ;
    PLIST_ENTRY    listEntry;
    DWORD          dwBytesTooFew, dwTmpBufferSize ;
    BYTE           DummyBuffer[1] ;
    LPBYTE         lpStart, lpEnd ;
    LPSERVICE_INFO lpServiceInfo = NULL ;
    BOOL           fMatchingNspCalled = FALSE ;

    //
    // we currently dont support async operation
    //

    if ( ARGUMENT_PRESENT( lpServiceAsyncInfo ) )
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return -1;
    }

    //
    // check the other parameters
    //

    if ( !ARGUMENT_PRESENT( lpGuid ) ||
         !ARGUMENT_PRESENT( lpdwBufferSize ) ||
         !ARGUMENT_PRESENT( lpServiceName ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }

    //
    // If our NSP internal structures have not yet been initialized,
    // initialize them now.
    //

    if ( !NspInitialized )
    {
        err = InitializeNsp();

        if ( err != NO_ERROR )
        {
            SetLastError( err );
            return -1;
        }
    }

    //
    // if buffer is NULL, user is just asking for size.
    // set BufferSize to 0 and proceed. we will find the size.
    //

    if ( !ARGUMENT_PRESENT( lpBuffer ) )
    {
        lpBuffer = &DummyBuffer[0] ;
        *lpdwBufferSize = 0 ;
    }

    //
    // allocate tmp buffer. make the tmp buffer at least as big as user's
    // buffer and no less than 4K.
    //

    dwTmpBufferSize = (*lpdwBufferSize < 4096) ? 4096 : *lpdwBufferSize ;
    lpServiceInfo = (LPSERVICE_INFO) ALLOCATE_HEAP(dwTmpBufferSize) ;

    if (!lpServiceInfo)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY) ;
        return(-1) ;
    }

    //
    // init variables & pointers in the buffer.
    //
    dwBytesTooFew = 0 ;
    lpStart = (LPBYTE) lpBuffer ;
    lpEnd   = (LPBYTE) ROUND_DOWN_POINTER((LPBYTE)lpBuffer + *lpdwBufferSize,
                                           ALIGN_LPDWORD) ;

    //
    // Loop through our name spaces, performing the operation
    // requested. This loop only goes thru more than one iteration if
    // the NameSpace is 'Default NameSpaces'. We dont stop on errors,
    // but continue to get as many as we can.
    //

    for ( listEntry = NameSpaceListHead.Flink;
          listEntry != &NameSpaceListHead;
          listEntry = listEntry->Flink )
    {
        PNAME_SPACE_INFO lpNameSpace;
        DWORD error ;

        lpNameSpace = CONTAINING_RECORD(
                        listEntry,
                        NAME_SPACE_INFO,
                        NameSpaceListEntry
                        );

        //
        // Determine whether we should make use of this name space
        // provider.
        //

        if ( !IsValidNameSpace( dwNameSpace, lpNameSpace ) )
        {
            continue;
        }

        //
        // make sure function is supported.
        //

        if (lpNameSpace->GetServiceProc == NULL)
        {
            continue;
        }

        fMatchingNspCalled = TRUE ;

        error = lpNameSpace->GetServiceProc(lpGuid,
                                            (LPWSTR)lpServiceName,
                                            dwProperties,
                                            fUnicodeCaller,
                                            lpServiceInfo,
                                            &dwTmpBufferSize) ;
        if (error != NO_ERROR)
        {
            SetLastError(error) ;
        }
        else
        {

            err = PackBuffer ( lpNameSpace->NameSpace,
                               lpGuid,
                               lpServiceInfo,
                               fUnicodeCaller,
                               lpServiceName,
                               &lpStart,
                               &lpEnd,
                               &dwBytesTooFew ) ;

            if (err == NO_ERROR)
            {
                //
                // successfully found one. increment count
                //
                cEntries++ ;
            }
            else if (err == ERROR_INSUFFICIENT_BUFFER)
            {
                //
                // continue. not interesting error to report right now.
                // we will check for dwBytesTooFew > 0 later.
                //
            }
            else
            {
                SetLastError(err) ;
            }
        }

    }  // for all namespaces

    (void) FREE_HEAP(lpServiceInfo) ;

    //
    // if the buffer is not big enough, set the number of bytes required
    // and return -1.
    //
    if (dwBytesTooFew)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER) ;
        *lpdwBufferSize += dwBytesTooFew ;
        return -1 ;
    }

    //
    // no matching provider was called. set the error since no-one
    // else would have done so.
    //
    if (!fMatchingNspCalled)
        SetLastError(ERROR_SERVICE_NOT_FOUND) ;

    return cEntries ;
}

#else         // OLDXBY

INT
APIENTRY
SetServiceWorker (
    IN     DWORD                dwNameSpace,
    IN     DWORD                dwOperation,
    IN     DWORD                dwFlags,
    IN     LPSERVICE_INFO       lpServiceInfo,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    IN     BOOL                 fUnicodeCaller,
    IN OUT LPDWORD              lpdwStatusFlags
    )
/*++

Routine Description:

    Worker function for SetService.

Arguments:

   As SetServiceW, except that fUnicodeCaller has been added
   which tells us if the call originated as Unicode or Ansi. We
   need distinguish because of the BLOB data which we cannot interpret.

Return Value:

   Zero if no error. SOCKET_ERROR otherwise. Caller may issue
   WSAGetLastError() to get more detailed error info.

--*/

{
    INT err = NO_ERROR, err1 ;
    BOOL fOneSucceeded = FALSE ;
    PLIST_ENTRY listEntry;
    SERVICE_INFO TmpSvcInfo ;
    LPCSADDR_INFO pcsadr;
    WSAQUERYSET wsaq;
    WSAVERSION wsaver;
    WSADATA wsaData;
    DWORD dwType;

    //
    // check parameters.
    // we currently dont support async operation.
    //

    if ( ARGUMENT_PRESENT( lpServiceAsyncInfo ) )
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return -1;
    }

    if ( !ARGUMENT_PRESENT( lpServiceInfo ) ||
         !ARGUMENT_PRESENT( lpdwStatusFlags ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }

    *lpdwStatusFlags = 0 ;

    //
    // we make a copy of the structure since we may modify it.
    //

    memcpy(&TmpSvcInfo,lpServiceInfo,sizeof(TmpSvcInfo)) ;

    //
    // Note, this call goes to the wsock32 routine first, so the
    // version must be the 1.1 WINSOCK. Pretty hokey, but that's
    // the way it is.
    //
    WSAStartup(0x101, &wsaData);

    //
    // the only operation where we take explicit action is ADD_TYPE and
    // DELETE_TYPE. everything else gets done by providers.
    //

    switch(dwOperation)
    {
    case SERVICE_ADD_TYPE:
    case SERVICE_DELETE_TYPE:

    {
        HKEY                        hKey ;
        LPTSTR                      lpServiceTypeName = NULL ;
        LPSERVICE_TYPE_INFO         lpServiceTypeInfo ;
        DWORD                       dwDisposition = 0 ;
        LPSERVICE_TYPE_INFO_ABS     lpSvcTypeInfoAbs  ;

        //
        // make sure Blob has valid data
        //
        if ((TmpSvcInfo.ServiceSpecificInfo.cbSize == 0) ||
            (TmpSvcInfo.ServiceSpecificInfo.pBlobData == NULL))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            err = -1;
            goto OuttaHere;
        }

        //
        // get the name of the service type being added.
        // if not unicode caller, we need convert to unicode.
        //

        lpSvcTypeInfoAbs  = (LPSERVICE_TYPE_INFO_ABS)
                                 TmpSvcInfo.ServiceSpecificInfo.pBlobData ;
#if defined(UNICODE)
        if (fUnicodeCaller)
        {
#endif
             lpServiceTypeName = lpSvcTypeInfoAbs->lpTypeName ;
#if defined(UNICODE)
        }
        else
        {
             err = AllocateUnicodeString(
                       ((LPSERVICE_TYPE_INFO_ABSA)lpSvcTypeInfoAbs)->lpTypeName,
                       &lpServiceTypeName)  ;
             if (err != NO_ERROR)
             {
                 SetLastError(err) ;
                 err = -1;
                 goto OuttaHere;
             }
        }
#endif

        //
        // make sure the service type name is not empty
        //
        if ( !lpServiceTypeName  ||
             *lpServiceTypeName == 0 )
        {
            err =  ERROR_INVALID_PARAMETER ;
        }
        else
        {
            err = SSRegistrySettings(dwOperation,
                                     lpServiceTypeName,
                                     TmpSvcInfo.lpServiceType);
        }

        if ( err != NO_ERROR )
        {
            SetLastError(err) ;
#if defined(UNICODE)
            if (!fUnicodeCaller)
                (void) FREE_HEAP(lpServiceTypeName) ;
#endif
            err = -1;
            goto OuttaHere;
        }

        if (dwOperation == SERVICE_ADD_TYPE)
        {
            LPWSANSCLASSINFO lpcli;
            WSASERVICECLASSINFO wsacl;

            lpcli = ServiceTypeToClassInfo(
                          fUnicodeCaller,
                          lpSvcTypeInfoAbs->dwValueCount,
                          lpSvcTypeInfoAbs->Values);

            if(lpcli)
            {
                wsacl.lpServiceClassId = TmpSvcInfo.lpServiceType;
                wsacl.lpszServiceClassName = lpServiceTypeName;
                wsacl.dwCount = lpSvcTypeInfoAbs->dwValueCount;
                wsacl.lpClassInfos = lpcli;
                err = WSAInstallServiceClass(&wsacl);
                FreeAllocatedClassInfo(fUnicodeCaller,
                                       lpSvcTypeInfoAbs->dwValueCount,
                                       lpcli);
                if(err != NO_ERROR)
                {
                    err = GetLastError();
                }
            }
        }

        else
        {
            //
            // need to uninstall the information
            //

            err = WSARemoveServiceClass(TmpSvcInfo.lpServiceType);
            if(err != NO_ERROR)
            {
                err = GetLastError();
            }

        }
#if defined(UNICODE)
        if (!fUnicodeCaller)
            (void) FREE_HEAP(lpServiceTypeName) ;
#endif

        if ( err != NO_ERROR )
        {
            SetLastError(err) ;
            err = -1;
        }
        goto OuttaHere;
    }

    //
    // Not ADD_TYPE or DELETE_TYPE. Do the others by issuing
    // a WSASetService call.

    case SERVICE_REGISTER:

        dwType = RNRSERVICE_REGISTER;
        break;

    case SERVICE_DEREGISTER:

        dwType = RNRSERVICE_DEREGISTER;
        break;

    default:
        goto OuttaHere;
    }    // switch

    memset(&wsaq, 0, sizeof(wsaq));
    if(dwOperation == SERVICE_REGISTER)
    {
        //
        // registration includes addresses that must be
        // coerced from SERVICE_ADDRESSES to CSADDR_INFO.
        //

        pcsadr =  SrvAddrsToCsAddrs(lpServiceInfo->lpServiceAddress,
                                    &wsaq.dwNumberOfCsAddrs);
        
    }
    else
    {
        pcsadr = 0;
    }

    wsaq.lpcsaBuffer = pcsadr;

    wsaq.dwSize = sizeof(wsaq);
    wsaq.lpszServiceInstanceName = lpServiceInfo->lpServiceName;
    wsaq.lpServiceClassId = lpServiceInfo->lpServiceType;
    wsaq.lpszComment = lpServiceInfo->lpComment;
    wsaq.dwNameSpace = dwNameSpace;
    if(lpServiceInfo->dwVersion)
    {
        wsaq.lpVersion = &wsaver;
        wsaver.dwVersion = lpServiceInfo->dwVersion;
        wsaver.ecHow = COMP_EQUAL;
    }
    
    //
    // BUGBUG. Locale, display hint, time, and machine name are lost
    // N.B. dwFlags ignored.
    //


    err = WSASetService(&wsaq,
                        dwType,
                        0);
    if(pcsadr)
    {
        FREE_HEAP(pcsadr);
    }

OuttaHere:
    err1 = GetLastError();
    WSACleanup();
    SetLastError(err1);
    return(err);
}

INT
APIENTRY
GetServiceWorker (
    IN     DWORD                dwNameSpace,
    IN     LPGUID               lpGuid,
    IN     LPTSTR               lpServiceName,
    IN     DWORD                dwProperties,
    IN OUT LPVOID               lpBuffer,
    IN OUT LPDWORD              lpdwBufferSize,
    IN     LPSERVICE_ASYNC_INFO lpServiceAsyncInfo,
    IN     BOOL                 fUnicodeCaller
)
/*++

Routine Description:

    Worker function for GetService.

Arguments:

   As GetServiceW, except that fUnicodeCaller has been added
   which tells us if the call originated as Unicode or Ansi. We
   need distinguish because of the BLOB data which we cannot interpret.

Return Value:

   Number of NS_SERVICE_INFO structures obtained if no error.
   SOCKET_ERROR otherwise. Caller may issue WSAGetLastError()
   to get more detailed error info.

--*/

{
    DWORD          err = NO_ERROR ;
    DWORD          dwBytesTooFew, dwTmpBufferSize, dwTempSize ;
    BYTE           DummyBuffer[sizeof(WSAQUERYSET)] ;
    LPBYTE         lpStart, lpEnd ;
    BOOL           fMatchingNspCalled = FALSE ;
    PWSAQUERYSET   pwsaq;
    HANDLE         hLookup = 0;
    BOOL           fGotAtLeastOne = FALSE;
    SERVICE_INFO   si;
    DWORD          cEntries = 0;
    WSADATA        wsaData;

    //
    // we currently dont support async operation
    //

    if ( ARGUMENT_PRESENT( lpServiceAsyncInfo ) )
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return -1;
    }

    //
    // check the other parameters
    //

    if ( !ARGUMENT_PRESENT( lpGuid ) ||
         !ARGUMENT_PRESENT( lpdwBufferSize ) ||
         !ARGUMENT_PRESENT( lpServiceName ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }

    //
    // if buffer is NULL, user is just asking for size.
    // set BufferSize to 0 and proceed. we will find the size.
    //

    if ( !ARGUMENT_PRESENT( lpBuffer ) )
    {
        lpBuffer = &DummyBuffer[0] ;
        *lpdwBufferSize = 0 ;
    }

    //
    // allocate tmp buffer. 
    //

    dwTmpBufferSize = *lpdwBufferSize + sizeof(WSAQUERYSETW);
    dwTmpBufferSize = max(dwTmpBufferSize, 4096);  // be generous
                                                   // to avoid overflow
    pwsaq = (PWSAQUERYSET)ALLOCATE_HEAP(dwTmpBufferSize) ;

    if (!pwsaq)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY) ;
        return(-1) ;
    }

    memset(pwsaq, 0, dwTmpBufferSize);

    //
    // init variables & pointers in the buffer.
    //
    dwBytesTooFew = 0 ;
    lpStart = (LPBYTE) lpBuffer ;
    lpEnd   = (LPBYTE) ROUND_DOWN_POINTER((LPBYTE)lpBuffer + *lpdwBufferSize,
                                           ALIGN_LPDWORD) ;

    //
    // Get it started with a call to WSALookupServiceBegin
    //

    pwsaq->dwSize = sizeof(*pwsaq);
    pwsaq->lpszServiceInstanceName = lpServiceName;
    pwsaq->lpServiceClassId = lpGuid;
    pwsaq->dwNameSpace = dwNameSpace;
   

    //
    // Note, this call goes to the wsock32 routine first, so the
    // version must be the 1.1 WINSOCK. Pretty hokey, but that's
    // the way it is.
    //
    WSAStartup(0x101, &wsaData);

    //
    // BUGBUG. Can't do the RES_SERVICE stuff just yet
    //

#ifdef CHICAGO
    err = (*ws2_WSALookupServiceBegin)(
#else
    err = WSALookupServiceBegin(pwsaq,
#endif
                                LUP_RETURN_VERSION |
                                LUP_RETURN_COMMENT    |
                                LUP_RETURN_ADDR,
                                &hLookup);

    memset(&si, 0, sizeof(si));

    while(err == NO_ERROR)
    {
        //
        // The lookup  is primed. Fetch actual matches. For each match,
        // construct a SERVICE_INFO and use the 1.1 Packing routines
        //

        dwTempSize = dwTmpBufferSize;
#ifdef CHICAGO
        err = (*ws2_WSALookupServiceNext)(
#else
        err = WSALookupServiceNext(
#endif
                                   hLookup,
                                   0,
                                   &dwTempSize,
                                   pwsaq);

        //
        // if it worked, pack it in. Since the allocated buffer is quite large,
        // it is unlikely it overflowed
                                   

        if(err == NO_ERROR)
        {
            //
            // got some. Build a ServiceInfo structure so that
            // the 1.1 Packing routines work
            //

            fGotAtLeastOne = TRUE;


            si.lpServiceAddress = CsAddrsToSrvAddrs(
                                     pwsaq->lpcsaBuffer,
                                     pwsaq->dwNumberOfCsAddrs);

            si.lpComment = pwsaq->lpszComment;
            if(pwsaq->lpVersion)
            {
                si.dwVersion = pwsaq->lpVersion->dwVersion;
            }
            else
            {
                si.dwVersion = 0;
            }

            err = PackBuffer ( pwsaq->dwNameSpace,
                               lpGuid,
                               &si,
                               fUnicodeCaller,
                               lpServiceName,
                               &lpStart,
                               &lpEnd,
                               &dwBytesTooFew ) ;

            if(si.lpServiceAddress)
            {
                FREE_HEAP(si.lpServiceAddress);
            }

            if(err == NO_ERROR)
            {
                cEntries++;
            }
        }
        else
        {
            int err1 = GetLastError();

            if((err1 == NO_DATA)
                    ||
               (err1 = WSA_E_NO_MORE))
            {
                err = NO_ERROR;
            }
            break;
        }
    }


    if(hLookup)
    {
        if(err != NO_ERROR)
        {
            err = GetLastError();
        }
#ifdef CHICAGO
        (*ws2_WSALookupServiceEnd)(hLookup);
#else
        WSALookupServiceEnd(hLookup);
#endif
        SetLastError(err);
    }

    WSACleanup();

    FREE_HEAP(pwsaq) ;

    //
    // if the buffer is not big enough, set the number of bytes required
    // and return -1.
    //
    if (dwBytesTooFew)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER) ;
        *lpdwBufferSize += dwBytesTooFew ;
        return(-1) ;
    }

    return cEntries ;
}

#endif             // OLDXBY

DWORD
PackBuffer (
    IN     DWORD           dwNameSpace,
    IN     LPGUID          lpServiceType,
    IN     LPSERVICE_INFO  lpServiceInfo,
    IN     BOOL            fUnicodeCaller,
    IN     LPTSTR          lpServiceName,
    IN OUT LPBYTE         *lppStart,
    IN OUT LPBYTE         *lppEnd,
    IN OUT LPDWORD         lpdwBytesTooFew
)
/*++

Routine Description:

   Pack a buffer with fixed size structure at front & variable size
   data at end.

Arguments:

   dwNameSpace      - part of info in structure

   lpServiceType    - part of info in structure

   lpServiceInfo    - part of info in structure

   fUnicodeCaller   - was the caller calling W or A version?

   lpServiceName    - part of info in structure

   lppStart         - starting point to store fixed structure

   lppEnd           - just past end of buffer. this pointer is
                      moved back before variable data is packed.

   lpdwBytesTooFew  - count of bytes too few. this is always
                      incremented as it may already contain value from
                      earlier calls.

Return Value:

   NO_ERROR if successful. Win32 error otherwise.

--*/

{
    LPNS_SERVICE_INFO lpNsServiceInfoNew = (LPNS_SERVICE_INFO) *lppStart ;
    LPSERVICE_INFO lpServiceInfoNew = &(lpNsServiceInfoNew->ServiceInfo) ;
    DWORD  Length ;

    LPBYTE lpTmpComment = NULL,
           lpTmpLocale = NULL,
           lpTmpMachinename = NULL,
           lpTmpName = NULL,
           lpEnd = *lppEnd,
           lpStartVar = *lppStart + sizeof(*lpNsServiceInfoNew) ;

    //
    // figure out whether we have enough room for structure
    //
    if (lpStartVar > lpEnd)
    {
        *lpdwBytesTooFew += (DWORD)(ULONG_PTR)(((LPBYTE)lpStartVar) - ((LPBYTE)lpEnd));
        lpStartVar = lpEnd ;
    }
    else
    {
        lpNsServiceInfoNew->dwNameSpace = dwNameSpace ;
    }
    *lppStart = lpStartVar ;

    //
    // figure out whether we have enough room for the GUID
    //
    if (lpStartVar + sizeof(GUID) > lpEnd)
    {
        *lpdwBytesTooFew += (DWORD)(ULONG_PTR)(((LPBYTE)lpStartVar + sizeof(GUID))
                            - ((LPBYTE)lpEnd)) ;
        lpEnd = lpStartVar ;
    }
    else
    {
        lpEnd -= sizeof(GUID) ;
        RtlCopyMemory(lpEnd, lpServiceType, sizeof(GUID)) ;
        lpServiceInfoNew->lpServiceType = (LPGUID) lpEnd ;
    }

    //
    // ditto for each of the strings. the PackString function
    // does not return errors. it merely increments BytesTooFew as need.
    //
    PackString(lpServiceInfo->lpComment,
               lpStartVar,
               &lpEnd,
               &lpTmpComment,
               lpdwBytesTooFew,
               !fUnicodeCaller) ;   // convert to ansi if caller not unicode

    PackString(lpServiceInfo->lpLocale,
               lpStartVar,
               &lpEnd,
               &lpTmpLocale,
               lpdwBytesTooFew,
               !fUnicodeCaller) ;   // convert to ansi if caller not unicode

    PackString(lpServiceInfo->lpMachineName,
               lpStartVar,
               &lpEnd,
               &lpTmpMachinename,
               lpdwBytesTooFew,
               !fUnicodeCaller) ;   // convert to ansi if caller not unicode

    PackString(lpServiceName,
               lpStartVar,
               &lpEnd,
               &lpTmpName,
               lpdwBytesTooFew,
               !fUnicodeCaller) ;   // convert to ansi if caller not unicode

    //
    // figure out whether we have enough room for the service address
    //
    if (lpServiceInfo->lpServiceAddress)
    {
        DWORD AddressCount =
            lpServiceInfo->lpServiceAddress->dwAddressCount ;
        LPSERVICE_ADDRESS lpAddress =
            &(lpServiceInfo->lpServiceAddress->Addresses[0]) ;
        LPSERVICE_ADDRESS lpTmpAddress ;
        LPSERVICE_ADDRESSES lpTmpServiceAddress ;

        //
        // make sure we have enough space for the structure &
        // the variable array at the end
        //
        Length = sizeof(SERVICE_ADDRESSES) +
                        ((AddressCount ? (AddressCount - 1) : 0) *
                        sizeof(SERVICE_ADDRESS)) ;

        if ((lpStartVar + Length) > lpEnd)
        {
            //
            // not enough space.
            //
            *lpdwBytesTooFew += (DWORD)(ULONG_PTR)(((LPBYTE)lpStartVar + Length) -
                                 ((LPBYTE)lpEnd)) ;
            lpEnd = lpStartVar ;
        }
        else
        {
            lpEnd -= Length ;
            lpTmpServiceAddress = (LPSERVICE_ADDRESSES) lpEnd ;
            lpTmpServiceAddress->dwAddressCount = AddressCount ;

            lpTmpAddress =
                (LPSERVICE_ADDRESS) &(lpTmpServiceAddress->Addresses[0]) ;
        }

        //
        // for each variable array, make sure we have enough for the
        // thang it points to.
        //
        while(AddressCount--)
        {
            DWORD AddressLength = ROUND_UP_COUNT(lpAddress->dwAddressLength,
                                                 ALIGN_DWORD) ;

            DWORD PrincipalLength = ROUND_UP_COUNT(lpAddress->dwPrincipalLength,
                                                   ALIGN_DWORD) ;

            if ((lpStartVar + AddressLength + PrincipalLength) > lpEnd)
            {
                //
                // not enough space.
                //
                *lpdwBytesTooFew += (DWORD)(ULONG_PTR)(((LPBYTE)lpStartVar +
                                              AddressLength +
                                              PrincipalLength) -
                                      (LPBYTE)lpEnd) ;
                lpEnd = lpStartVar ;
            }
            else
            {
                lpTmpAddress->dwAddressType = lpAddress->dwAddressType ;
                lpTmpAddress->dwAddressFlags = lpAddress->dwAddressFlags ;

                //
                // copy address data
                //

                lpTmpAddress->dwAddressLength = lpAddress->dwAddressLength ;
                lpEnd -= AddressLength ;
                RtlCopyMemory(lpEnd,
                              (LPBYTE)lpAddress->lpAddress,
                              lpAddress->dwAddressLength) ;
                lpTmpAddress->lpAddress = lpEnd ;

                //
                // copy principal data
                //

                lpTmpAddress->dwPrincipalLength = lpAddress->dwPrincipalLength ;
                lpEnd -= PrincipalLength ;
                if (lpAddress->lpPrincipal)
                {
                    RtlCopyMemory(lpEnd,
                                  (LPBYTE)lpAddress->lpPrincipal,
                                  lpAddress->dwPrincipalLength) ;
                    lpTmpAddress->lpPrincipal = lpEnd ;
                }
                else
                    lpTmpAddress->lpPrincipal = NULL ;

                lpTmpAddress++ ;
            }

            lpAddress++ ;
        }

        lpServiceInfoNew->lpServiceAddress = *lpdwBytesTooFew ? NULL :
                                             lpTmpServiceAddress ;
    }

    //
    // figure out whether we have enough room for the Blob
    //
    if (Length = ROUND_UP_COUNT(lpServiceInfo->ServiceSpecificInfo.cbSize,
                                ALIGN_DWORD))
    {
        if ((lpStartVar + Length) > lpEnd)
        {
            //
            // not enough space.
            //
            *lpdwBytesTooFew += (DWORD)(ULONG_PTR)(((LPBYTE)lpStartVar + Length) -
                                 ((LPBYTE)lpEnd)) ;
            lpEnd = lpStartVar ;
        }
        else
        {
            //
            // NOTE/WARNING: Blob most NOT contain pointers!!!
            //
            lpEnd -= Length ;
            RtlCopyMemory(lpEnd,
                          lpServiceInfo->ServiceSpecificInfo.pBlobData,
                          Length) ;

            lpServiceInfoNew->ServiceSpecificInfo.cbSize =
                                  lpServiceInfo->ServiceSpecificInfo.cbSize ;

            lpServiceInfoNew->ServiceSpecificInfo.pBlobData = lpEnd ;
        }
    }

    if (*lpdwBytesTooFew > 0)
    {
        //
        // buffer is not big enough. dont copy any data
        //
        *lppEnd = lpEnd ;
        return ERROR_INSUFFICIENT_BUFFER ;
    }

    //
    // copy the rest of the data over to the user's buffer
    //

    //
    // these are DWORDs & are easy to copy
    //
    lpServiceInfoNew->dwDisplayHint =  lpServiceInfo->dwDisplayHint ;
    lpServiceInfoNew->dwVersion =  lpServiceInfo->dwVersion ;
    lpServiceInfoNew->dwTime =  lpServiceInfo->dwTime ;

    //
    // setup strings (already packed at end)
    //
    lpServiceInfoNew->lpServiceName = (LPTSTR) lpTmpName ;
    lpServiceInfoNew->lpComment     = (LPTSTR) lpTmpComment ;
    lpServiceInfoNew->lpLocale      = (LPTSTR) lpTmpLocale ;
    lpServiceInfoNew->lpMachineName = (LPTSTR) lpTmpMachinename ;

    *lppEnd = lpEnd ;

    return NO_ERROR ;
}

VOID
PackString(
    LPTSTR lpSource,
    LPBYTE lpStartVar,
    LPBYTE *lppEnd,
    LPBYTE *lppDest,
    LPDWORD lpdwBytesTooFew,
    BOOL fConvertToAnsi
)
/*++

Routine Description:

    Packs a single string at the end of a buffer.

Arguments:

    lpSource         - source string to copy

    lpStartVar       - start of buffer, lppEnd cannot be backed up
                       beyond this.

    lppEnd           - points just past end of buffer. back up before
                       using. if not enough room (ie. gone < lpStartVar)
                       then set to lpStartVar.

    lppDest          - if enough room, on exit set to *lppEnd. If not,
                       set to NULL.

    lpdwBytesTooFew  - count of bytes too few. this is always
                       incremented as it may already contain value from
                       earlier calls.

    fCovertToAnsi    - convert copy to Ansi

Return Value:

   NO_ERROR if successful. Win32 error otherwise.

--*/

{
    DWORD Length ;

    //
    // init return to NULL.
    //
    *lppDest = NULL ;

    //
    // if NULL, nothing more to do.
    //
    if (!lpSource)
    {
        return ;
    }

    //
    // find length. we word align because we dont know if next item
    // after this string needs to be aligned.
    //

    Length = (_tcslen(lpSource) + 1) *
             (fConvertToAnsi ? sizeof(CHAR) : sizeof(WCHAR)) ;

    Length = ROUND_UP_COUNT(Length, ALIGN_DWORD) ;

    if ((lpStartVar + Length) > *lppEnd)
    {
        //
        // not enough space. calculate how many bytes short &
        // set the end marker to where the variable section
        // is (ie. buffer fully used) so that further calculations
        // are correct.
        //
        *lpdwBytesTooFew += (DWORD)(ULONG_PTR)(((LPBYTE)lpStartVar + Length) -
                            ((LPBYTE)*lppEnd)) ;
        *lppEnd = lpStartVar ;
    }
    else
    {
        //
        // we have required space. move pointer back & copy the data.
        //
        *lppEnd -= Length ;
        *lppDest = *lppEnd ;

#if defined(UNICODE)
        if (fConvertToAnsi)
        {
            UNICODE_STRING UnicodeString ;
            ANSI_STRING    AnsiString ;
            NTSTATUS       NtStatus ;

            AnsiString.MaximumLength = (USHORT) Length ;
            AnsiString.Length = 0 ;
            AnsiString.Buffer = *lppDest ;

            RtlInitUnicodeString(&UnicodeString,lpSource) ;

            NtStatus = RtlUnicodeStringToAnsiString(&AnsiString,
                                                    &UnicodeString,
                                                    FALSE) ; // dont alloc
            if (!NT_SUCCESS(NtStatus))
            {
                //
                // shouldnt happen. if error, just null string out
                //
                **lppDest = 0 ;
            }
        }
        else
        {
#endif // defined(UNICODE
            _tcscpy((LPTSTR)*lppDest, lpSource) ;
#if defined(UNICODE)
        }
#endif
    }
}


#if OLDXBYY

//
// The string handling function table. This is used to switch to either ANSI
// or Unicode string functions. Note, the order of the values is very
// important.
//

typedef  LPWSTR (__cdecl *LPCOPYFUNCTION)(LPVOID,LPVOID) ;
typedef  size_t (__cdecl *LPLENGTHFUNCTION)(LPVOID) ;

#ifdef CHICAGO

char * MyStrCpy( char * d, char * s )
{
    return FSTRCPY( d, s );
}

size_t MyStrLen( char * s )
{
    return FSTRLEN( s );
}

#undef _tcscpy
#undef _tcslen
#define _tcscpy MyStrCpy
#define _tcslen MyStrLen
#endif  // CHICAGO

DWORD
APIENTRY
PackBlobWorker(
         LPTSTR lpTypeName,
         DWORD dwValueCount,
         PSERVICE_TYPE_VALUE_ABS Values,
         PBYTE *lppBuffer,
         DWORD *lpdBufferSize,
         BOOL fIsAnsi
         )

/*++

Routine Description:

    Takes the data describing a service and packs it into a blob. It uses
    an intermdiate structure, SERVICE_TYPE_VALUES_ABS that is easier
    to construct than the blob-based structure. This is called from
    the external APIs and works with either Unicode or ANSI strings.

Author:

    Credit for this function goes to Arnold Miller.

--*/

{
    PBYTE pBuffer, ptr;
    DWORD dwSize, dwIndex;
    size_t Length;
    PSERVICE_TYPE_INFO psi;
    PSERVICE_TYPE_VALUE pv1;
    PSERVICE_TYPE_VALUE_ABS pv;
    DWORD dwType = (fIsAnsi ? 0 : 1);
#ifndef CHICAGO
    LPCOPYFUNCTION pfnCopy = (fIsAnsi ? (LPCOPYFUNCTION) strcpy : (LPCOPYFUNCTION) _tcscpy);
    LPLENGTHFUNCTION pfnLength = (fIsAnsi ? (LPLENGTHFUNCTION) strlen : (LPLENGTHFUNCTION) _tcslen);
#else
    LPCOPYFUNCTION pfnCopy = (fIsAnsi ? (LPCOPYFUNCTION) MyStrCpy : (LPCOPYFUNCTION) _tcscpy);
    LPLENGTHFUNCTION pfnLength = (fIsAnsi ? (LPLENGTHFUNCTION) MyStrLen : (LPLENGTHFUNCTION) _tcslen);
#endif
    //
    // N.B. dwType is the type of string. It's also the log(2) of the
    // size that we use as the left-shift value. The index we compute
    // must match the order of the pointers in apfnCopy and apfnLength above.
    //

    //
    // compute size of fixed areas
    //

    Length = pfnLength(lpTypeName);

    Length = ROUND_UP_COUNT((Length + 1) << dwType, ALIGN_DWORD);

    dwSize = Length + sizeof(SERVICE_TYPE_INFO) +
             (sizeof(SERVICE_TYPE_VALUE) * (dwValueCount - 1));

    //
    // Now we have to compute the size needed by the type names and values.
    //

    for(dwIndex = 0, pv = Values;
        dwIndex < dwValueCount;
        dwIndex++, pv++)
    {
        DWORD dwSS;

        dwSS = pfnLength(pv->lpValueName);

        dwSize += ROUND_UP_COUNT(pv->dwValueSize, ALIGN_DWORD) +
                  ROUND_UP_COUNT((dwSS + 1) << dwType ,
                         ALIGN_DWORD);
    }

    //
    // got the total needed size.
    //

    *lpdBufferSize = dwSize;    // assume we succeed

    pBuffer = (PBYTE)
              ALLOCATE_HEAP(dwSize);

    if(!pBuffer)
    {
        return(GetLastError()) ;
    }

    //
    // Got a buffer. Nothing can fail now, can fail now, can fail now ...
    //

    *lppBuffer = pBuffer;

    //
    // now cast into the buffer
    //

    //
    // The SERVICE_TYPE_INFO starts at the beginning.
    //

    psi = (PSERVICE_TYPE_INFO)pBuffer;

    //
    // Find the first SERVICE_TYPE_VALUE
    //

    pv1 = (PSERVICE_TYPE_VALUE)&psi->Values[0];

    //
    // now the string space. It is after all of the structures
    //

    ptr = (PBYTE)(pv1 + dwValueCount);

    //
    // fill in the SERVICE_TYPE_INFO
    //

    psi->dwTypeNameOffset = (DWORD)(ULONG_PTR)(ptr - pBuffer);
    psi->dwValueCount = dwValueCount;

    //
    // copy the service name into the string space.
    //

    pfnCopy(ptr, lpTypeName);

    ptr += Length;

    //
    // now do the service type values
    //

    for(dwIndex = 0, pv = Values;
        dwIndex < dwValueCount;
        dwIndex++, pv++, pv1++)
    {
        pv1->dwNameSpace = pv->dwNameSpace;
        pv1->dwValueType = pv->dwValueType;
        pv1->dwValueSize = pv->dwValueSize;

        //
        // we allocated sufficient space to DWORD align everything. So we are
        // careful to actually do that.
        //

        //
        // first, do the value
        //

        pv1->dwValueOffset = (DWORD)(ULONG_PTR)(ptr - pBuffer);
        memcpy(ptr, pv->lpValue, pv->dwValueSize);
        ptr += ROUND_UP_COUNT(pv->dwValueSize, ALIGN_DWORD);

        //
        // now the name
        //

        pv1->dwValueNameOffset = (DWORD)(ULONG_PTR)(ptr - pBuffer);

        pfnCopy(ptr, pv->lpValueName);

        //
        // We already computed this, it's too bad we couldn't have saved it. On
        // the assumption there aren't very many of these to do, this wasteful
        // immplementation is probably a good trade-off, however. The only
        // way to save them, and to make it extensible, would have been to
        // allocate a buffer of the proper size and use that as an arrary
        // of Length values. This would have been worth it if we believe that
        // callers had enough values to marshall to make it worthwhile.
        //

        Length = pfnLength(pv->lpValueName);

        Length = ROUND_UP_COUNT((Length + 1) << dwType,
                       ALIGN_DWORD);

        ptr += Length;
    }

    return(NO_ERROR);
}

#endif  // OLDXBYY

