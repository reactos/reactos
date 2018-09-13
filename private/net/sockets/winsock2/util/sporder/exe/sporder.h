/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    sporder.h

Abstract:

    This header prototypes the 32-Bit Windows functions that are used
    to change the order or WinSock2 transport service providers and
    name space providers.

Revision History:

--*/



int
WSPAPI
WSCWriteProviderOrder (
    IN LPDWORD lpwdCatalogEntryId,
    IN DWORD dwNumberOfEntries
    );
/*++

Routine Description:

    Reorder existing WinSock2 service providers.  The order of the service
    providers determines their priority in being selected for use.  The
    sporder.exe tool will show you the installed provider and their ordering,
    Alternately, WSAEnumProtocols(), in conjunction with this function,
    will allow you to write your own tool.

Arguments:

    lpwdCatalogEntryId  [in]
      An array of CatalogEntryId elements as found in the WSAPROTOCOL_INFO
      structure.  The order of the CatalogEntryId elements is the new
      priority ordering for the service providers.

    dwNumberOfEntries  [in]
      The number of elements in the lpwdCatalogEntryId array.


Return Value:

    ERROR_SUCCESS   - the service providers have been reordered.
    WSAEINVAL       - input parameters were bad, no action was taken.
    ERROR_BUSY      - the routine is being called by another thread or process.
    any registry error code


Comments:

    Here are scenarios in which the WSCWriteProviderOrder function may fail:

      The dwNumberOfEntries is not equal to the number of registered service
      providers.

      The lpwdCatalogEntryId contains an invalid catalog ID.

      The lpwdCatalogEntryId does not contain all valid catalog IDs exactly
      1 time.

      The routine is not able to access the registry for some reason
      (e.g. inadequate user persmissions)

      Another process (or thread) is currently calling the routine.

--*/
