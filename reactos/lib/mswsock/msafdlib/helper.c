/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

LIST_ENTRY SockHelperDllListHead;

/* FUNCTIONS *****************************************************************/

VOID
WSPAPI
SockFreeHelperDll(IN PHELPER_DATA Helper)
{
    /* Free the DLL */
    FreeLibrary(Helper->hInstance);

    /* Free the mapping */
    RtlFreeHeap(SockPrivateHeap, 0, Helper->Mapping);

    /* Free the DLL Structure itself */
    RtlFreeHeap(SockPrivateHeap, 0, Helper);
}

INT
WSPAPI
SockGetTdiName(PINT AddressFamily, 
               PINT SocketType, 
               PINT Protocol,
               LPGUID ProviderId,
               GROUP Group, 
               DWORD Flags, 
               PUNICODE_STRING TransportName, 
               PVOID *HelperDllContext, 
               PHELPER_DATA *HelperDllData, 
               PDWORD Events)
{
    PHELPER_DATA HelperData;
    PWSTR Transports;
    PWSTR Transport;
    PWINSOCK_MAPPING Mapping;
    PLIST_ENTRY Helpers;
    BOOLEAN SharedLock = TRUE;
    INT ErrorCode;
    BOOLEAN AfMatch = FALSE, ProtoMatch = FALSE, SocketMatch  = FALSE;

    /* Acquire global lock */
    SockAcquireRwLockShared(&SocketGlobalLock);

TryAgain:
    /* Check in our Current Loaded Helpers */
    for (Helpers = SockHelperDllListHead.Flink;
         Helpers != &SockHelperDllListHead; 
         Helpers = Helpers->Flink) 
        {
        /* Get the current helper */
        HelperData = CONTAINING_RECORD(Helpers, HELPER_DATA, Helpers);

        /* See if this Mapping works for us */
        if (SockIsTripleInMapping(HelperData->Mapping,
                                  *AddressFamily,
                                  &AfMatch,
                                  *SocketType,
                                  &SocketMatch,
                                  *Protocol,
                                  &ProtoMatch))
        {
            /* Call the Helper Dll function get the Transport Name */
            if (!HelperData->WSHOpenSocket2) 
            {
                if (!(Flags & WSA_FLAG_MULTIPOINT_ALL))
                {
                    /* DLL Doesn't support WSHOpenSocket2, call the old one */
                    ErrorCode = HelperData->WSHOpenSocket(AddressFamily,
                                                          SocketType,
                                                          Protocol,
                                                          TransportName,
                                                          HelperDllContext,
                                                          Events);
                }
                else
                {
                    /* Invalid flag */
                    ErrorCode = WSAEINVAL;
                }
            }
            else 
            {
                /* Call the new WSHOpenSocket */
                ErrorCode = HelperData->WSHOpenSocket2(AddressFamily,
                                                       SocketType,
                                                       Protocol,
                                                       Group,
                                                       Flags,
                                                       TransportName,
                                                       HelperDllContext,
                                                       Events);
            }

            /* Check for success */
            if (ErrorCode == NO_ERROR)
            {
                /* Reference the helper */
                InterlockedIncrement(&HelperData->RefCount);

                /* Check which lock we acquired */
                if (SharedLock)
                {
                    /* Release the shared lock */
                    SockReleaseRwLockShared(&SocketGlobalLock);
                }
                else
                {
                    /* Release the acquired lock */
                }

                /* Return the Helper Pointers */
                *HelperDllData = HelperData;
                return NO_ERROR;
            }

            /* Check if we don't need a transport name */
            if ((*SocketType == SOCK_RAW) && (TransportName->Buffer))
            {
                /* Free it */
                RtlFreeHeap(RtlGetProcessHeap(), 0, TransportName->Buffer);
                TransportName->Buffer = NULL;
            }
        }
    }

    /* We didn't find a match: try again with RW access */
    if (SharedLock)
    {
        /* Switch locks */
        SockReleaseRwLockShared(&SocketGlobalLock);
        SockAcquireRwLockExclusive(&SocketGlobalLock);

        /* Parse the list again */
        SharedLock = FALSE;
        goto TryAgain;
    }

    /* Get the Transports available */
    ErrorCode = SockLoadTransportList(&Transports);

    /* Check for error */
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        SockReleaseRwLockExclusive(&SocketGlobalLock);
        return ErrorCode;
    }
    
    /* Loop through each transport until we find one that can satisfy us */
    for (Transport = Transports; *Transports; Transport += wcslen(Transport) + 1) 
    {
        /* See what mapping this Transport supports */
        ErrorCode = SockLoadTransportMapping(Transport, &Mapping);
        
        /* Check for error */
        if (ErrorCode != NO_ERROR)
        {
            /* Try the next one */
            continue;
        }

        /* See if this Mapping works for us */
        if (SockIsTripleInMapping(Mapping,
                                  *AddressFamily,
                                  &AfMatch,
                                  *SocketType,
                                  &SocketMatch,
                                  *Protocol,
                                  &ProtoMatch))
        {
            /* It does, so load the DLL associated with it */
            ErrorCode = SockLoadHelperDll(Transport, Mapping, &HelperData);

            /* Check for success */
            if (ErrorCode == NO_ERROR)
            {
                /* Call the Helper Dll function get the Transport Name */
                if (!HelperData->WSHOpenSocket2)
                {
                    /* Check for invalid flag combo */
                    if (!(Flags & WSA_FLAG_MULTIPOINT_ALL))
                    {
                        /* DLL Doesn't support WSHOpenSocket2, call the old one */
                        ErrorCode = HelperData->WSHOpenSocket(AddressFamily,
                                                              SocketType,
                                                              Protocol,
                                                              TransportName,
                                                              HelperDllContext,
                                                              Events);
                    }
                    else
                    {
                        /* Fail */
                        ErrorCode = WSAEINVAL;
                    }
                }
                else 
                {
                    /* Call the newer function */
                    ErrorCode = HelperData->WSHOpenSocket2(AddressFamily,
                                                           SocketType,
                                                           Protocol,
                                                           Group,
                                                           Flags,
                                                           TransportName,
                                                           HelperDllContext,
                                                           Events);
                }

                /* Check for success */
                if (ErrorCode == NO_ERROR)
                {
                    /* Reference the helper */
                    InterlockedIncrement(&HelperData->RefCount);

                    /* Release the lock and free the transports */
                    SockReleaseRwLockExclusive(&SocketGlobalLock);
                    RtlFreeHeap(SockPrivateHeap, 0, Transports);

                    /* Return the Helper Pointer */
                    *HelperDllData = HelperData;
                    return NO_ERROR;
                }

                /* Check if we don't need a transport name */
                if ((*SocketType == SOCK_RAW) && (TransportName->Buffer))
                {
                    /* Free it */
                    RtlFreeHeap(RtlGetProcessHeap(), 0,  TransportName->Buffer);
                    TransportName->Buffer = NULL;
                }

                /* Try again */
                continue;
            }
        }

        /* Free the mapping and continue */
        RtlFreeHeap(SockPrivateHeap, 0, Mapping);
    }

    /* Release the lock and free the transport list */
    SockReleaseRwLockExclusive(&SocketGlobalLock);
    RtlFreeHeap(SockPrivateHeap, 0, Transports);

    /* Check why we didn't find a match */
    if (!AfMatch) return WSAEAFNOSUPPORT;
    if (!ProtoMatch) return WSAEPROTONOSUPPORT;
    if (!SocketMatch) return WSAESOCKTNOSUPPORT;

    /* The comination itself was invalid */
    return WSAEINVAL;
}

INT
WSPAPI
SockLoadTransportMapping(IN PWSTR TransportName, 
                         OUT PWINSOCK_MAPPING *Mapping)
{
    PWSTR TransportKey;
    HKEY KeyHandle;
    INT ErrorCode;
    ULONG MappingSize = 0;

    /* Allocate a Buffer */
    TransportKey = SockAllocateHeapRoutine(SockPrivateHeap,
                                           0, 
                                           MAX_PATH * sizeof(WCHAR));
    /* Check for error */
    if (!TransportKey) return ERROR_NOT_ENOUGH_MEMORY;

    /* Generate the right key name */
    wcscpy(TransportKey, L"System\\CurrentControlSet\\Services\\");
    wcscat(TransportKey, TransportName);
    wcscat(TransportKey, L"\\Parameters\\Winsock");

    /* Open the Key */
    ErrorCode = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                              TransportKey, 
                              0, 
                              KEY_READ, 
                              &KeyHandle);

    /* We don't need the Transport Key anymore */
    RtlFreeHeap(SockPrivateHeap, 0, TransportKey);

    /* Check for error */
    if ((ErrorCode != ERROR_MORE_DATA) && (ErrorCode != NO_ERROR))
    {
        /* Close key and fail */
        RegCloseKey(KeyHandle);
        return ErrorCode;
    }

    /* Find out how much space we need for the Mapping */
    ErrorCode = RegQueryValueExW(KeyHandle, 
                                 L"Mapping", 
                                 NULL, 
                                 NULL, 
                                 NULL, 
                                 &MappingSize);

    /* Check for error */
    if ((ErrorCode != ERROR_MORE_DATA) && (ErrorCode != NO_ERROR))
    {
        /* Close key and fail */
        RegCloseKey(KeyHandle);
        return ErrorCode;
    }

    /* Allocate Memory for the Mapping */
    *Mapping = SockAllocateHeapRoutine(SockPrivateHeap, 0, MappingSize);

    /* Check for error */
    if (!(*Mapping))
    {
        /* Close key and fail */
        RegCloseKey(KeyHandle);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Read the Mapping */
    ErrorCode = RegQueryValueExW(KeyHandle, 
                                 L"Mapping", 
                                 NULL, 
                                 NULL, 
                                 (LPBYTE)*Mapping, 
                                 &MappingSize);
    
    /* Check for error */
    if ((ErrorCode != ERROR_MORE_DATA) && (ErrorCode != NO_ERROR))
    {
        /* Close key and fail */
        RegCloseKey(KeyHandle);
        return ErrorCode;
    }

    /* Close key and return */
    RegCloseKey(KeyHandle);
    return NO_ERROR;
}

INT
WSPAPI
SockLoadHelperDll(PWSTR TransportName, 
                  PWINSOCK_MAPPING Mapping, 
                  PHELPER_DATA *HelperDllData)
{
    PHELPER_DATA HelperData;
    PWSTR HelperDllName;
    PWSTR FullHelperDllName;
    ULONG HelperDllNameSize;
    PWSTR HelperKey;
    HKEY KeyHandle;
    ULONG DataSize;
    INT ErrorCode;
    PLIST_ENTRY Entry;
    
    /* Allocate space for the Helper Structure and TransportName */
    HelperData = SockAllocateHeapRoutine(SockPrivateHeap,
                                         0, 
                                         sizeof(*HelperData) + 
                                         (DWORD)(wcslen(TransportName) + 1) *
                                         sizeof(WCHAR));

    /* Check for error */
    if (!HelperData) return  ERROR_NOT_ENOUGH_MEMORY;
    
    /* Allocate Space for the Helper DLL Key */
    HelperKey = SockAllocateHeapRoutine(SockPrivateHeap,
                                        0,
                                        MAX_PATH * sizeof(WCHAR));
    
    /* Check for error */
    if (!HelperKey)
    {
        RtlFreeHeap(SockPrivateHeap, 0, HelperData);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Generate the right key name */
    wcscpy(HelperKey, L"System\\CurrentControlSet\\Services\\");
    wcscat(HelperKey, TransportName);
    wcscat(HelperKey, L"\\Parameters\\Winsock");

    /* Open the Key */
    ErrorCode = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                              HelperKey, 
                              0, 
                              KEY_READ, 
                              &KeyHandle);

    /* Free Buffer */
    RtlFreeHeap(SockPrivateHeap, 0, HelperKey);

    /* Check for error */
    if (ErrorCode != NO_ERROR) 
    {
        /* Free the helper data and fail */
        RtlFreeHeap(SockPrivateHeap, 0, HelperData);
        RegCloseKey(KeyHandle);
        return ErrorCode;
    }
    
    /* Read Minimum size of Sockaddr Structures */
    DataSize = sizeof(HelperData->MinWSAddressLength);
    RegQueryValueExW(KeyHandle, 
                     L"MinSockaddrLength", 
                     NULL, 
                     NULL, 
                     (LPBYTE)&HelperData->MinWSAddressLength, 
                     &DataSize);
    
    /* Check for error */
    if (ErrorCode != NO_ERROR) 
    {
        /* Free the helper data and fail */
        RtlFreeHeap(SockPrivateHeap, 0, HelperData);
        RegCloseKey(KeyHandle);
        return ErrorCode;
    }

    /* Read Maximum size of Sockaddr Structures */
    DataSize = sizeof(HelperData->MinWSAddressLength);
    RegQueryValueExW(KeyHandle, 
                     L"MaxSockaddrLength", 
                     NULL, 
                     NULL, 
                     (LPBYTE)&HelperData->MaxWSAddressLength, 
                     &DataSize);
    
    /* Check for error */
    if (ErrorCode != NO_ERROR) 
    {
        /* Free the helper data and fail */
        RtlFreeHeap(SockPrivateHeap, 0, HelperData);
        RegCloseKey(KeyHandle);
        return ErrorCode;
    }

    /* Size of TDI Structures */
    HelperData->MinTDIAddressLength = HelperData->MinWSAddressLength + 6;
    HelperData->MaxTDIAddressLength = HelperData->MaxWSAddressLength + 6;
    
    /* Read Delayed Acceptance Setting */
    DataSize = sizeof(DWORD);
    ErrorCode = RegQueryValueExW(KeyHandle, 
                                 L"UseDelayedAcceptance", 
                                 NULL, 
                                 NULL, 
                                 (LPBYTE)&HelperData->UseDelayedAcceptance, 
                                 &DataSize);

    /* Use defalt if we failed */
    if (ErrorCode != NO_ERROR) HelperData->UseDelayedAcceptance = -1;

    /* Allocate Space for the Helper DLL Names */
    HelperDllName = SockAllocateHeapRoutine(SockPrivateHeap,
                                            0,
                                            MAX_PATH * sizeof(WCHAR));

    /* Check for error */
    if (!HelperDllName) 
    {
        /* Free the helper data and fail */
        RtlFreeHeap(SockPrivateHeap, 0, HelperData);
        RegCloseKey(KeyHandle);
        return ErrorCode;
    }

    /* Allocate space for the expanded version */
    FullHelperDllName = SockAllocateHeapRoutine(SockPrivateHeap,
                                                0,
                                                MAX_PATH * sizeof(WCHAR));
    
    /* Check for error */
    if (!FullHelperDllName) 
    {
        /* Free the helper data and fail */
        RtlFreeHeap(SockPrivateHeap, 0, HelperData);
        RtlFreeHeap(SockPrivateHeap, 0, HelperDllName);
        RegCloseKey(KeyHandle);
        return ErrorCode;
    }

    /* Get the name of the Helper DLL*/
    DataSize = 512;
    ErrorCode = RegQueryValueExW(KeyHandle, 
                                 L"HelperDllName", 
                                 NULL, 
                                 NULL, 
                                 (LPBYTE)HelperDllName, 
                                 &DataSize);

    /* Check for error */
    if (ErrorCode != NO_ERROR) 
    {
        /* Free the helper data and fail */
        RtlFreeHeap(SockPrivateHeap, 0, HelperData);
        RtlFreeHeap(SockPrivateHeap, 0, HelperDllName);
        RtlFreeHeap(SockPrivateHeap, 0, FullHelperDllName);
        RegCloseKey(KeyHandle);
        return ErrorCode;
    }

    /* Get the Full name, expanding Environment Strings */
    HelperDllNameSize = ExpandEnvironmentStringsW(HelperDllName,
                                                  FullHelperDllName, 
                                                  MAX_PATH);

    /* Load the DLL */
    HelperData->hInstance = LoadLibraryW(FullHelperDllName);

    /* Free Buffers */
    RtlFreeHeap(SockPrivateHeap, 0, HelperDllName);
    RtlFreeHeap(SockPrivateHeap, 0, FullHelperDllName);

    /* Return if we didn't Load it Properly */
    if (!HelperData->hInstance) 
    {
        /* Free memory and fail */
        RtlFreeHeap(SockPrivateHeap, 0, HelperData);
        RegCloseKey(KeyHandle);
        return GetLastError();
    }

    /* Close Key */
    RegCloseKey(KeyHandle);

    /* Get the Pointers to the Helper Routines */
    HelperData->WSHOpenSocket =    (PWSH_OPEN_SOCKET)
                                 GetProcAddress(HelperData->hInstance,
                                                "WSHOpenSocket");
    HelperData->WSHOpenSocket2 = (PWSH_OPEN_SOCKET2)
                                  GetProcAddress(HelperData->hInstance,
                                                 "WSHOpenSocket2");
    HelperData->WSHJoinLeaf = (PWSH_JOIN_LEAF)
                               GetProcAddress(HelperData->hInstance,
                                              "WSHJoinLeaf");
    HelperData->WSHNotify = (PWSH_NOTIFY)
                             GetProcAddress(HelperData->hInstance, "WSHNotify");
    HelperData->WSHGetSocketInformation = (PWSH_GET_SOCKET_INFORMATION)
                                           GetProcAddress(HelperData->hInstance, 
                                                          "WSHGetSocketInformation");
    HelperData->WSHSetSocketInformation = (PWSH_SET_SOCKET_INFORMATION)
                                           GetProcAddress(HelperData->hInstance,
                                                          "WSHSetSocketInformation");
    HelperData->WSHGetSockaddrType = (PWSH_GET_SOCKADDR_TYPE)
                                      GetProcAddress(HelperData->hInstance,
                                                     "WSHGetSockaddrType");
    HelperData->WSHGetWildcardSockaddr = (PWSH_GET_WILDCARD_SOCKADDR)
                                          GetProcAddress(HelperData->hInstance,
                                                         "WSHGetWildcardSockaddr");
    HelperData->WSHGetBroadcastSockaddr = (PWSH_GET_BROADCAST_SOCKADDR)
                                           GetProcAddress(HelperData->hInstance,
                                                          "WSHGetBroadcastSockaddr");
    HelperData->WSHAddressToString = (PWSH_ADDRESS_TO_STRING)
                                      GetProcAddress(HelperData->hInstance,
                                                     "WSHAddressToString");
    HelperData->WSHStringToAddress = (PWSH_STRING_TO_ADDRESS)
                                      GetProcAddress(HelperData->hInstance,
                                                     "WSHStringToAddress");
    HelperData->WSHIoctl = (PWSH_IOCTL)
                            GetProcAddress(HelperData->hInstance, "WSHIoctl");

    /* Save the Mapping Structure and transport name */
    HelperData->Mapping = Mapping;
    wcscpy(HelperData->TransportName, TransportName);

    /* Increment Reference Count */
    HelperData->RefCount = 1;

    /* Add it to our list */
    InsertHeadList(&SockHelperDllListHead, &HelperData->Helpers);

    /* Return Pointers */
    *HelperDllData = HelperData;

    /* Check if this one was already load it */
    Entry = HelperData->Helpers.Flink;
    while (Entry != &SockHelperDllListHead)
    {
        /* Get the entry */
        HelperData = CONTAINING_RECORD(Entry, HELPER_DATA, Helpers);

        /* Move to the next one */
        Entry = Entry->Flink;

        /* Check if the names match */
        if (!wcscmp(HelperData->TransportName, (*HelperDllData)->TransportName))
        {
            /* Remove this one */
            RemoveEntryList(&HelperData->Helpers);
            SockDereferenceHelperDll(HelperData);
        }
    }

    /* Return success */
    return NO_ERROR;
}

BOOL
WSPAPI
SockIsTripleInMapping(IN PWINSOCK_MAPPING Mapping, 
                      IN INT AddressFamily,
                      OUT PBOOLEAN AfMatch,
                      IN INT SocketType, 
                      OUT PBOOLEAN SockMatch,
                      IN INT Protocol,
                      OUT PBOOLEAN ProtoMatch)
{
    ULONG Row;
    BOOLEAN FoundAf = FALSE, FoundProto = FALSE, FoundSocket = FALSE;
    
    /* Loop through Mapping to Find a matching one */
    for (Row = 0; Row < Mapping->Rows; Row++)
    {
        /* Check Address Family */
        if ((INT)Mapping->Mapping[Row].AddressFamily == AddressFamily)
        {
            /* Remember that we found it */
            FoundAf = TRUE;
        }

        /* Check Socket Type */
        if ((INT)Mapping->Mapping[Row].SocketType == SocketType)
        {
            /* Remember that we found it */
            FoundSocket = TRUE;
        }

        /* Check Protocol (SOCK_RAW and AF_NETBIOS can skip this check) */
        if (((INT)Mapping->Mapping[Row].SocketType == SocketType) ||
            (AddressFamily == AF_NETBIOS) || (SocketType == SOCK_RAW))
        {
            /* Remember that we found it */
            FoundProto = TRUE;
        }

        /* Check of all three values Match */
        if (FoundProto && FoundSocket && FoundAf)
        {
            /* Return success */
            *AfMatch = *SockMatch = *ProtoMatch = TRUE;
            return TRUE;
        }
    }

    /* Return whatever we found */
    if (FoundAf) *AfMatch = TRUE;
    if (FoundSocket) *SockMatch = TRUE;
    if (FoundProto) *ProtoMatch = TRUE;

    /* Fail */
    return FALSE;
}

INT
WSPAPI
SockNotifyHelperDll(IN PSOCKET_INFORMATION Socket,
                    IN DWORD Event)
{
    INT ErrorCode;

    /* See if this event matters */
    if (!(Socket->HelperEvents & Event)) return NO_ERROR;

    /* See if we have a helper... */
    if (!(Socket->HelperData)) return NO_ERROR;

    /* Get TDI handles */
    ErrorCode = SockGetTdiHandles(Socket);
    if (ErrorCode != NO_ERROR) return ErrorCode;

    /* Call the notification */
    return Socket->HelperData->WSHNotify(Socket->HelperContext,
                                         Socket->Handle,
                                         Socket->TdiAddressHandle,
                                         Socket->TdiConnectionHandle,
                                         Event);
}
