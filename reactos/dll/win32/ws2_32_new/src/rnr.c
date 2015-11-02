/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/rnr.c
 * PURPOSE:     Registration n' Resolution Support
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

//#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
INT
WSAAPI
WSAAddressToStringA(IN LPSOCKADDR lpsaAddress,
                    IN DWORD dwAddressLength,
                    IN LPWSAPROTOCOL_INFOA lpProtocolInfo,
                    OUT LPSTR lpszAddressString,
                    IN OUT  LPDWORD lpdwAddressStringLength)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode, Status;
    DWORD CatalogEntryId;
    PTCATALOG Catalog;
    PTCATALOG_ENTRY CatalogEntry;
    LPWSTR UnicodeString;
    DWORD Length = *lpdwAddressStringLength;
    DPRINT("WSAAddressToStringA: %p\n", lpsaAddress);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return SOCKET_ERROR;
    }

    /* Allocate the unicode string */
    UnicodeString = HeapAlloc(WsSockHeap, 0, Length * 2);
    if (!UnicodeString)
    {
        /* No memory; fail */
        SetLastError(WSAENOBUFS);
        return SOCKET_ERROR;
    }

    /* Get the catalog */
    Catalog = WsProcGetTCatalog(Process);

    /* Check if we got custom protocol info */
    if (lpProtocolInfo)
    {
        /* Get the entry ID */
        CatalogEntryId = lpProtocolInfo->dwCatalogEntryId;

        /* Get the entry associated with it */
        ErrorCode = WsTcGetEntryFromCatalogEntryId(Catalog,
                                                   CatalogEntryId,
                                                   &CatalogEntry);
    }
    else
    {
        /* Get it from the address family */
        ErrorCode = WsTcGetEntryFromAf(Catalog,
                                       lpsaAddress->sa_family,
                                       &CatalogEntry);
    }
    
    /* Check for success */
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* Call the provider */
        Status = CatalogEntry->Provider->Service.lpWSPAddressToString(lpsaAddress,
                                                                      dwAddressLength,
                                                                      &CatalogEntry->
                                                                      ProtocolInfo,
                                                                      UnicodeString,
                                                                      lpdwAddressStringLength,
                                                                      &ErrorCode);
        if (Status == ERROR_SUCCESS)
        {
            /* Convert the string */
            WideCharToMultiByte(CP_ACP,
                                0,
                                UnicodeString,
                                -1,
                                lpszAddressString,
                                Length,
                                NULL,
                                NULL);
        }

        /* Dereference the entry */
        WsTcEntryDereference(CatalogEntry);

        /* Free the unicode string */
        HeapFree(WsSockHeap, 0, UnicodeString);

        /* Check for success and return */
        if (Status == ERROR_SUCCESS) return ERROR_SUCCESS;
    }
    else
    {
        /* Free the unicode string */
        HeapFree(WsSockHeap, 0, UnicodeString);
    }

    /* Set the error and return */
    SetLastError(ErrorCode);
    return SOCKET_ERROR;
}

/*
 * @implemented
 */
INT
WSAAPI
WSAAddressToStringW(IN LPSOCKADDR lpsaAddress,
                    IN DWORD dwAddressLength,
                    IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
                    OUT LPWSTR lpszAddressString,
                    IN OUT LPDWORD lpdwAddressStringLength)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode, Status;
    DWORD CatalogEntryId;
    PTCATALOG Catalog;
    PTCATALOG_ENTRY CatalogEntry;
    DPRINT("WSAAddressToStringW: %p\n", lpsaAddress);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return SOCKET_ERROR;
    }

    /* Get the catalog */
    Catalog = WsProcGetTCatalog(Process);

    /* Check if we got custom protocol info */
    if (lpProtocolInfo)
    {
        /* Get the entry ID */
        CatalogEntryId = lpProtocolInfo->dwCatalogEntryId;

        /* Get the entry associated with it */
        ErrorCode = WsTcGetEntryFromCatalogEntryId(Catalog,
                                                   CatalogEntryId,
                                                   &CatalogEntry);
    }
    else
    {
        /* Get it from the address family */
        ErrorCode = WsTcGetEntryFromAf(Catalog,
                                       lpsaAddress->sa_family,
                                       &CatalogEntry);
    }
    
    /* Check for success */
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* Call the provider */
        Status = CatalogEntry->Provider->Service.lpWSPAddressToString(lpsaAddress,
                                                                      dwAddressLength,
                                                                      &CatalogEntry->
                                                                      ProtocolInfo,
                                                                      lpszAddressString,
                                                                      lpdwAddressStringLength,
                                                                      &ErrorCode);

        /* Dereference the entry */
        WsTcEntryDereference(CatalogEntry);

        /* Check for success and return */
        if (Status == ERROR_SUCCESS) return ERROR_SUCCESS;
    }

    /* Set the error and return */
    SetLastError(ErrorCode);
    return SOCKET_ERROR;
}

/*
 * @implemented
 */
INT
WSAAPI
WSALookupServiceEnd(IN HANDLE hLookup)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    PNSQUERY Query = hLookup;
    DPRINT("WSALookupServiceEnd: %lx\n", hLookup);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return SOCKET_ERROR;
    }

    /* Check for a valid handle, then validate and reference it */
    if (!(Query) || !(WsNqValidateAndReference(Query)))
    {
        /* Fail */
        SetLastError(WSA_INVALID_HANDLE);
        return SOCKET_ERROR;
    }

    /* Do the lookup */
    ErrorCode = WsNqLookupServiceEnd(Query);

    /* Remove the validation reference */
    WsNqDereference(Query);

    /* Remove the keep-alive */
    WsNqDereference(Query);

    /* Return */
    return ERROR_SUCCESS;
}

/*
 * @implemented
 */
INT
WSAAPI
WSALookupServiceBeginA(IN LPWSAQUERYSETA lpqsRestrictions,
                       IN DWORD dwControlFlags,
                       OUT LPHANDLE lphLookup)
{
    INT ErrorCode;
    LPWSAQUERYSETW UnicodeQuerySet = NULL;
    DWORD UnicodeQuerySetSize = 0;
    DPRINT("WSALookupServiceBeginA: %p\n", lpqsRestrictions);

    /* Verifiy pointer */
    if (IsBadReadPtr(lpqsRestrictions, sizeof(*lpqsRestrictions)))
    {
        /* Invalid */
        SetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    /* Clear the reserved fields */
    lpqsRestrictions->dwOutputFlags = 0;
    lpqsRestrictions->lpszComment = NULL;
    lpqsRestrictions->dwNumberOfCsAddrs = 0;

    /* Find out the side we'll need */
    ErrorCode = MapAnsiQuerySetToUnicode(lpqsRestrictions,
                                         &UnicodeQuerySetSize,
                                         UnicodeQuerySet);

    /* We should've failed */
    if (ErrorCode == WSAEFAULT)
    {
        /* Allocate the buffer we'll need */
        UnicodeQuerySet = HeapAlloc(WsSockHeap, 0, UnicodeQuerySetSize);
        if (UnicodeQuerySet)
        {
            /* Do the conversion for real */
            ErrorCode = MapAnsiQuerySetToUnicode(lpqsRestrictions,
                                                 &UnicodeQuerySetSize,
                                                 UnicodeQuerySet);
            if (ErrorCode == ERROR_SUCCESS)
            {
                /* Now call the Unicode function */
                ErrorCode = WSALookupServiceBeginW(UnicodeQuerySet,
                                                   dwControlFlags,
                                                   lphLookup);
            }
            else
            {
                /* Fail, conversion failed */
                SetLastError(ErrorCode);
            }

            /* Free our buffer */
            HeapFree(WsSockHeap, 0, UnicodeQuerySet);
        }
        else
        {
            /* No memory to allocate */
            SetLastError(WSAEFAULT);
        }
    }
    else
    {
        /* We couldn't get the size for some reason */
        SetLastError(ErrorCode);
    }

    /* Return to caller */
    return ErrorCode == ERROR_SUCCESS ? ErrorCode : SOCKET_ERROR;
}

/*
 * @implemented
 */
INT 
WINAPI
WSALookupServiceBeginW(IN LPWSAQUERYSETW lpqsRestrictions,
                       IN DWORD dwControlFlags,
                       OUT LPHANDLE lphLookup)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    PNSQUERY Query;
    DPRINT("WSALookupServiceBeginW: %p\n", lpqsRestrictions);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return SOCKET_ERROR;
    }

    /* Verify pointers */
    if (IsBadWritePtr(lphLookup, sizeof(*lphLookup)) ||
        IsBadReadPtr(lpqsRestrictions, sizeof(*lpqsRestrictions)))
    {
        /* They are invalid; fail */
        SetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    /* Create a new query object */
    if ((Query = WsNqAllocate()))
    {
        /* Initialize it */
        WsNqInitialize(Query);

        /* Do the lookup */
        ErrorCode = WsNqLookupServiceBegin(Query,
                                           lpqsRestrictions,
                                           dwControlFlags,
                                           WsProcGetNsCatalog(Process));

        /* Check for success */
        if (ErrorCode == ERROR_SUCCESS)
        {
            /* Return the handle */
            *lphLookup = Query;
        }
        else
        {
            /* Fail */
            *lphLookup = NULL;
            WsNqDelete(Query);
        }
    }
    else
    {
        /* No memory */
        ErrorCode = SOCKET_ERROR;
        SetLastError(WSAENOBUFS);
    }
    
    /* Return */
    return ErrorCode;
}

/*
 * @implemented
 */
INT
WINAPI
WSALookupServiceNextW(IN HANDLE hLookup,
                      IN DWORD dwControlFlags,
                      IN OUT LPDWORD lpdwBufferLength,
                      OUT LPWSAQUERYSETW lpqsResults)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    PNSQUERY Query = hLookup;
    DPRINT("WSALookupServiceNextW: %lx\n", hLookup);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return SOCKET_ERROR;
    }

    /* Check for a valid handle, then validate and reference it */
    if (!(Query) || !(WsNqValidateAndReference(Query)))
    {
        /* Fail */
        SetLastError(WSA_INVALID_HANDLE);
        return SOCKET_ERROR;
    }

    /* Do the lookup */
    ErrorCode = WsNqLookupServiceNext(Query,
                                      dwControlFlags,
                                      lpdwBufferLength,
                                      lpqsResults);

    /* Remove the validation reference */
    WsNqDereference(Query);

    /* Return */
    return ErrorCode;
}

/*
 * @implemented
 */
INT
WSAAPI
WSALookupServiceNextA(IN HANDLE hLookup,
                      IN DWORD dwControlFlags,
                      IN OUT LPDWORD lpdwBufferLength,
                      OUT LPWSAQUERYSETA lpqsResults)
{
    LPWSAQUERYSETW UnicodeQuerySet;
    DWORD UnicodeQuerySetSize = *lpdwBufferLength;
    INT ErrorCode;
    DPRINT("WSALookupServiceNextA: %lx\n", hLookup);

    /* Check how much the user is giving */
    if (UnicodeQuerySetSize >= sizeof(WSAQUERYSETW))
    {
        /* Allocate the buffer we'll use */
        UnicodeQuerySet = HeapAlloc(WsSockHeap, 0, UnicodeQuerySetSize);
        if (!UnicodeQuerySet) UnicodeQuerySetSize = 0;
    }
    else
    {
        /* His buffer is too small */
        UnicodeQuerySetSize = 0;
        UnicodeQuerySet = NULL;
    }

    /* Call the Unicode Function */
    ErrorCode = WSALookupServiceNextW(hLookup,
                                      dwControlFlags,
                                      &UnicodeQuerySetSize,
                                      UnicodeQuerySet);
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* Not convert to ANSI */
        ErrorCode = MapUnicodeQuerySetToAnsi(UnicodeQuerySet,
                                             lpdwBufferLength,
                                             lpqsResults);
        if (ErrorCode != ERROR_SUCCESS) SetLastError(ErrorCode);
    }
    else
    {
        /* Check if we ran out of space */
        if (GetLastError() == WSAEFAULT)
        {
            /* Return how much space we'll need, including padding */
            *lpdwBufferLength = UnicodeQuerySetSize +
                                ((sizeof(ULONG) * 6) - (6 * 1));
        }
    }

    /* If we had a local buffer, free it */
    if (UnicodeQuerySet) HeapFree(WsSockHeap, 0, UnicodeQuerySet);

    /* Return to caller */
    return ErrorCode == ERROR_SUCCESS ? ErrorCode : SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSPAPI
WSANSPIoctl(HANDLE hLookup,
            DWORD dwControlCode,
            LPVOID lpvInBuffer,
            DWORD cbInBuffer,
            LPVOID lpvOutBuffer,
            DWORD cbOutBuffer,
            LPDWORD lpcbBytesReturned,
            LPWSACOMPLETION lpCompletion)
{
    DPRINT("WSANSPIoctl: %lx\n", hLookup);
    return 0;
}

/*
 * @unimplemented
 */
INT
WSAAPI
WSARemoveServiceClass(IN LPGUID lpServiceClassId)
{
    DPRINT("WSARemoveServiceClass: %lx\n", lpServiceClassId);
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSAAPI
WSASetServiceA(IN LPWSAQUERYSETA lpqsRegInfo,
               IN WSAESETSERVICEOP essOperation,
               IN DWORD dwControlFlags)
{
    DPRINT("WSASetServiceA: %lx\n", lpqsRegInfo);
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSAAPI
WSASetServiceW(IN LPWSAQUERYSETW lpqsRegInfo,
               IN WSAESETSERVICEOP essOperation,
               IN DWORD dwControlFlags)
{
    DPRINT("WSASetServiceW: %lx\n", lpqsRegInfo);
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSAAPI
WSAGetServiceClassInfoA(IN LPGUID lpProviderId,
                        IN LPGUID lpServiceClassId,
                        IN OUT LPDWORD lpdwBufferLength,
                        OUT LPWSASERVICECLASSINFOA lpServiceClassInfo)
{
    DPRINT("WSAGetServiceClassInfoA: %lx\n", lpProviderId);
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSAAPI
WSAGetServiceClassInfoW(IN LPGUID lpProviderId,
                        IN LPGUID lpServiceClassId,
                        IN OUT LPDWORD lpdwBufferLength,
                        OUT LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    DPRINT("WSAGetServiceClassInfoW: %lx\n", lpProviderId);
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSAAPI
WSAGetServiceClassNameByClassIdA(IN LPGUID lpServiceClassId,
                                 OUT LPSTR lpszServiceClassName,
                                 IN OUT LPDWORD lpdwBufferLength)
{
    DPRINT("WSAGetServiceClassNameByClassIdA: %lx\n", lpServiceClassId);
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSAAPI
WSAGetServiceClassNameByClassIdW(IN LPGUID lpServiceClassId,
                                 OUT LPWSTR lpszServiceClassName,
                                 IN OUT LPDWORD lpdwBufferLength)
{
    DPRINT("WSAGetServiceClassNameByClassIdW: %lx\n", lpServiceClassId);
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSAAPI
WSAInstallServiceClassA(IN LPWSASERVICECLASSINFOA lpServiceClassInfo)
{
    DPRINT("WSAInstallServiceClassA: %lx\n", lpServiceClassInfo);
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSAAPI
WSAEnumNameSpaceProvidersA(IN OUT LPDWORD lpdwBufferLength,
                           OUT LPWSANAMESPACE_INFOA lpnspBuffer)
{
    DPRINT("WSAEnumNameSpaceProvidersA: %lx\n", lpnspBuffer);
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSAAPI
WSAEnumNameSpaceProvidersW(IN OUT LPDWORD lpdwBufferLength,
                           OUT LPWSANAMESPACE_INFOW lpnspBuffer)
{
    DPRINT("WSAEnumNameSpaceProvidersW: %lx\n", lpnspBuffer);
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @unimplemented
 */
INT
WSAAPI
WSAInstallServiceClassW(IN LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    DPRINT("WSAInstallServiceClassW: %lx\n", lpServiceClassInfo);
    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

/*
 * @implemented
 */
INT
WSAAPI
WSAStringToAddressA(IN LPSTR AddressString,
                    IN INT AddressFamily,
                    IN LPWSAPROTOCOL_INFOA lpProtocolInfo,
                    OUT LPSOCKADDR lpAddress,
                    IN OUT  LPINT lpAddressLength)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode, Status;
    DWORD CatalogEntryId;
    PTCATALOG Catalog;
    PTCATALOG_ENTRY CatalogEntry;
    LPWSTR UnicodeString;
    DWORD Length = (DWORD)strlen(AddressString) + 1;
    DPRINT("WSAStringToAddressA: %s\n", AddressString);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return SOCKET_ERROR;
    }

    /* Allocate the unicode string */
    UnicodeString = HeapAlloc(WsSockHeap, 0, Length * 2);
    if (!UnicodeString)
    {
        /* No memory; fail */
        SetLastError(WSAENOBUFS);
        return SOCKET_ERROR;
    }

    /* Convert the string */
    MultiByteToWideChar(CP_ACP, 0, AddressString, -1, UnicodeString, Length);

    /* Get the catalog */
    Catalog = WsProcGetTCatalog(Process);

    /* Check if we got custom protocol info */
    if (lpProtocolInfo)
    {
        /* Get the entry ID */
        CatalogEntryId = lpProtocolInfo->dwCatalogEntryId;

        /* Get the entry associated with it */
        ErrorCode = WsTcGetEntryFromCatalogEntryId(Catalog,
                                                   CatalogEntryId,
                                                   &CatalogEntry);
    }
    else
    {
        /* Get it from the address family */
        ErrorCode = WsTcGetEntryFromAf(Catalog, AddressFamily, &CatalogEntry);
    }
    
    /* Check for success */
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* Call the provider */
        Status = CatalogEntry->Provider->Service.lpWSPStringToAddress(UnicodeString,
                                                              AddressFamily,
                                                              &CatalogEntry->
                                                              ProtocolInfo,
                                                              lpAddress,
                                                              lpAddressLength,
                                                              &ErrorCode);

        /* Dereference the entry */
        WsTcEntryDereference(CatalogEntry);

        /* Free the unicode string */
        HeapFree(WsSockHeap, 0, UnicodeString);

        /* Check for success and return */
        if (Status == ERROR_SUCCESS) return ERROR_SUCCESS;
    }
    else
    {
        /* Free the unicode string */
        HeapFree(WsSockHeap, 0, UnicodeString);
    }

    /* Set the error and return */
    SetLastError(ErrorCode);
    return SOCKET_ERROR;
}

/*
 * @implemented
 */
INT
WSAAPI
WSAStringToAddressW(IN LPWSTR AddressString,
                    IN INT AddressFamily,
                    IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
                    OUT LPSOCKADDR lpAddress,
                    IN OUT LPINT lpAddressLength)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode, Status;
    DWORD CatalogEntryId;
    PTCATALOG Catalog;
    PTCATALOG_ENTRY CatalogEntry;
    DPRINT("WSAStringToAddressW: %S\n", AddressString);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Leave now */
        SetLastError(ErrorCode);
        return SOCKET_ERROR;
    }

    /* Get the catalog */
    Catalog = WsProcGetTCatalog(Process);

    /* Check if we got custom protocol info */
    if (lpProtocolInfo)
    {
        /* Get the entry ID */
        CatalogEntryId = lpProtocolInfo->dwCatalogEntryId;

        /* Get the entry associated with it */
        ErrorCode = WsTcGetEntryFromCatalogEntryId(Catalog,
                                                   CatalogEntryId,
                                                   &CatalogEntry);
    }
    else
    {
        /* Get it from the address family */
        ErrorCode = WsTcGetEntryFromAf(Catalog, AddressFamily, &CatalogEntry);
    }
    
    /* Check for success */
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* Call the provider */
        Status = CatalogEntry->Provider->Service.lpWSPStringToAddress(AddressString,
                                                                      AddressFamily,
                                                                      &CatalogEntry->
                                                                      ProtocolInfo,
                                                                      lpAddress,
                                                                      lpAddressLength,
                                                                      &ErrorCode);

        /* Dereference the entry */
        WsTcEntryDereference(CatalogEntry);

        /* Check for success and return */
        if (Status == ERROR_SUCCESS) return ERROR_SUCCESS;
    }

    /* Set the error and return */
    SetLastError(ErrorCode);
    return SOCKET_ERROR;
}
