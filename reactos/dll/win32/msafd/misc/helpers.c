/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        misc/helpers.c
 * PURPOSE:     Helper DLL management
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *				Alex Ionescu (alex@relsoft.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 *	 Alex 16/07/2004 - Complete Rewrite
 */

#include <msafd.h>

#include <winreg.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msafd);

CRITICAL_SECTION HelperDLLDatabaseLock;
LIST_ENTRY HelperDLLDatabaseListHead;


INT
SockGetTdiName(
    PINT AddressFamily,
    PINT SocketType,
    PINT Protocol,
    GROUP Group,
    DWORD Flags,
    PUNICODE_STRING TransportName,
    PVOID *HelperDllContext,
    PHELPER_DATA *HelperDllData,
    PDWORD Events)
{
    PHELPER_DATA        HelperData;
    PWSTR               Transports;
    PWSTR               Transport;
    PWINSOCK_MAPPING	Mapping;
    PLIST_ENTRY	        Helpers;
    INT                 Status;

    TRACE("AddressFamily %p, SocketType %p, Protocol %p, Group %u, Flags %lx, TransportName %p, HelperDllContext %p, HelperDllData %p, Events %p\n",
        AddressFamily, SocketType, Protocol, Group, Flags, TransportName, HelperDllContext, HelperDllData, Events);

    /* Check in our Current Loaded Helpers */
    for (Helpers = SockHelpersListHead.Flink;
         Helpers != &SockHelpersListHead;
         Helpers = Helpers->Flink ) {

        HelperData = CONTAINING_RECORD(Helpers, HELPER_DATA, Helpers);

        /* See if this Mapping works for us */
        if (SockIsTripleInMapping (HelperData->Mapping,
                                   *AddressFamily,
                                   *SocketType,
                                   *Protocol)) {

            /* Call the Helper Dll function get the Transport Name */
            if (HelperData->WSHOpenSocket2 == NULL ) {

                /* DLL Doesn't support WSHOpenSocket2, call the old one */
                HelperData->WSHOpenSocket(AddressFamily,
                                          SocketType,
                                          Protocol,
                                          TransportName,
                                          HelperDllContext,
                                          Events
                                          );
            } else {
                HelperData->WSHOpenSocket2(AddressFamily,
                                           SocketType,
                                           Protocol,
                                           Group,
                                           Flags,
                                           TransportName,
                                           HelperDllContext,
                                           Events
                                           );
            }

            /* Return the Helper Pointers */
            *HelperDllData = HelperData;
            return NO_ERROR;
        }
    }

    /* Get the Transports available */
    Status = SockLoadTransportList(&Transports);

    /* Check for error */
    if (Status) {
        WARN("Can't get transport list\n");
        return Status;
    }

    /* Loop through each transport until we find one that can satisfy us */
    for (Transport = Transports;
         *Transports != 0;
         Transport += wcslen(Transport) + 1) {
        TRACE("Transport: %S\n", Transports);

        /* See what mapping this Transport supports */
        Status = SockLoadTransportMapping(Transport, &Mapping);

        /* Check for error */
        if (Status) {
            ERR("Can't get mapping for %S\n", Transports);
            HeapFree(GlobalHeap, 0, Transports);
            return Status;
        }

        /* See if this Mapping works for us */
        if (SockIsTripleInMapping(Mapping, *AddressFamily, *SocketType, *Protocol)) {

            /* It does, so load the DLL associated with it */
            Status = SockLoadHelperDll(Transport, Mapping, &HelperData);

            /* Check for error */
            if (Status) {
                ERR("Can't load helper DLL for Transport %S.\n", Transport);
                HeapFree(GlobalHeap, 0, Transports);
                HeapFree(GlobalHeap, 0, Mapping);
                return Status;
            }

            /* Call the Helper Dll function get the Transport Name */
            if (HelperData->WSHOpenSocket2 == NULL) {
                /* DLL Doesn't support WSHOpenSocket2, call the old one */
                HelperData->WSHOpenSocket(AddressFamily,
                                          SocketType,
                                          Protocol,
                                          TransportName,
                                          HelperDllContext,
                                          Events
                                          );
            } else {
                HelperData->WSHOpenSocket2(AddressFamily,
                                           SocketType,
                                           Protocol,
                                           Group,
                                           Flags,
                                           TransportName,
                                           HelperDllContext,
                                           Events
                                           );
            }

            /* Return the Helper Pointers */
            *HelperDllData = HelperData;
            /* We actually cache these ... the can't be freed yet */
            /*HeapFree(GlobalHeap, 0, Transports);*/
            /*HeapFree(GlobalHeap, 0, Mapping);*/
            return NO_ERROR;
        }

        HeapFree(GlobalHeap, 0, Mapping);
    }
    HeapFree(GlobalHeap, 0, Transports);
    return WSAEINVAL;
}

INT
SockLoadTransportMapping(
    PWSTR TransportName,
    PWINSOCK_MAPPING *Mapping)
{
    PWSTR               TransportKey;
    HKEY                KeyHandle;
    ULONG               MappingSize;
    LONG                Status;

    TRACE("TransportName %ws\n", TransportName);

    /* Allocate a Buffer */
    TransportKey = HeapAlloc(GlobalHeap, 0, (54 + wcslen(TransportName)) * sizeof(WCHAR));

    /* Check for error */
    if (TransportKey == NULL) {
        ERR("Buffer allocation failed\n");
        return WSAEINVAL;
    }

    /* Generate the right key name */
    wcscpy(TransportKey, L"System\\CurrentControlSet\\Services\\");
    wcscat(TransportKey, TransportName);
    wcscat(TransportKey, L"\\Parameters\\Winsock");

    /* Open the Key */
    Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, TransportKey, 0, KEY_READ, &KeyHandle);

    /* We don't need the Transport Key anymore */
    HeapFree(GlobalHeap, 0, TransportKey);

    /* Check for error */
    if (Status) {
        ERR("Error reading transport mapping registry\n");
        return WSAEINVAL;
    }

    /* Find out how much space we need for the Mapping */
    Status = RegQueryValueExW(KeyHandle, L"Mapping", NULL, NULL, NULL, &MappingSize);

    /* Check for error */
    if (Status) {
        ERR("Error reading transport mapping registry\n");
        return WSAEINVAL;
    }

    /* Allocate Memory for the Mapping */
    *Mapping = HeapAlloc(GlobalHeap, 0, MappingSize);

    /* Check for error */
    if (*Mapping == NULL) {
        ERR("Buffer allocation failed\n");
        return WSAEINVAL;
    }

    /* Read the Mapping */
    Status = RegQueryValueExW(KeyHandle, L"Mapping", NULL, NULL, (LPBYTE)*Mapping, &MappingSize);

    /* Check for error */
    if (Status) {
        ERR("Error reading transport mapping registry\n");
        HeapFree(GlobalHeap, 0, *Mapping);
        return WSAEINVAL;
    }

    /* Close key and return */
    RegCloseKey(KeyHandle);
    return 0;
}

INT
SockLoadTransportList(
    PWSTR *TransportList)
{
    ULONG	TransportListSize;
    HKEY	KeyHandle;
    LONG	Status;

    TRACE("Called\n");

    /* Open the Transports Key */
    Status = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\CurrentControlSet\\Services\\Winsock\\Parameters",
                            0,
                            KEY_READ,
                            &KeyHandle);

    /* Check for error */
    if (Status) {
        ERR("Error reading transport list registry\n");
        return WSAEINVAL;
    }

    /* Get the Transport List Size */
    Status = RegQueryValueExW(KeyHandle,
                              L"Transports",
                              NULL,
                              NULL,
                              NULL,
                              &TransportListSize);

    /* Check for error */
    if (Status) {
        ERR("Error reading transport list registry\n");
        return WSAEINVAL;
    }

    /* Allocate Memory for the Transport List */
    *TransportList = HeapAlloc(GlobalHeap, 0, TransportListSize);

    /* Check for error */
    if (*TransportList == NULL) {
        ERR("Buffer allocation failed\n");
        return WSAEINVAL;
    }

    /* Get the Transports */
    Status = RegQueryValueExW (KeyHandle,
                               L"Transports",
                               NULL,
                               NULL,
                               (LPBYTE)*TransportList,
                               &TransportListSize);

    /* Check for error */
    if (Status) {
        ERR("Error reading transport list registry\n");
        HeapFree(GlobalHeap, 0, *TransportList);
        return WSAEINVAL;
    }

    /* Close key and return */
    RegCloseKey(KeyHandle);
    return 0;
}

INT
SockLoadHelperDll(
    PWSTR TransportName,
    PWINSOCK_MAPPING Mapping,
    PHELPER_DATA *HelperDllData)
{
    PHELPER_DATA        HelperData;
    PWSTR               HelperDllName;
    PWSTR               FullHelperDllName;
    PWSTR               HelperKey;
    HKEY                KeyHandle;
    ULONG               DataSize;
    LONG                Status;

    /* Allocate space for the Helper Structure and TransportName */
    HelperData = HeapAlloc(GlobalHeap, 0, sizeof(*HelperData) + (wcslen(TransportName) + 1) * sizeof(WCHAR));

    /* Check for error */
    if (HelperData == NULL) {
        ERR("Buffer allocation failed\n");
        return WSAEINVAL;
    }

    /* Allocate Space for the Helper DLL Key */
    HelperKey = HeapAlloc(GlobalHeap, 0, (54 + wcslen(TransportName)) * sizeof(WCHAR));

    /* Check for error */
    if (HelperKey == NULL) {
        ERR("Buffer allocation failed\n");
        HeapFree(GlobalHeap, 0, HelperData);
        return WSAEINVAL;
    }

    /* Generate the right key name */
    wcscpy(HelperKey, L"System\\CurrentControlSet\\Services\\");
    wcscat(HelperKey, TransportName);
    wcscat(HelperKey, L"\\Parameters\\Winsock");

    /* Open the Key */
    Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, HelperKey, 0, KEY_READ, &KeyHandle);

    HeapFree(GlobalHeap, 0, HelperKey);

    /* Check for error */
    if (Status) {
        ERR("Error reading helper DLL parameters\n");
        HeapFree(GlobalHeap, 0, HelperData);
        return WSAEINVAL;
    }

    /* Read Size of SockAddr Structures */
    DataSize = sizeof(HelperData->MinWSAddressLength);
    HelperData->MinWSAddressLength = 16;
    RegQueryValueExW (KeyHandle,
                      L"MinSockaddrLength",
                      NULL,
                      NULL,
                      (LPBYTE)&HelperData->MinWSAddressLength,
                      &DataSize);
    DataSize = sizeof(HelperData->MinWSAddressLength);
    HelperData->MaxWSAddressLength = 16;
    RegQueryValueExW (KeyHandle,
                      L"MaxSockaddrLength",
                      NULL,
                      NULL,
                      (LPBYTE)&HelperData->MaxWSAddressLength,
                      &DataSize);

    /* Size of TDI Structures */
    HelperData->MinTDIAddressLength = HelperData->MinWSAddressLength + 6;
    HelperData->MaxTDIAddressLength = HelperData->MaxWSAddressLength + 6;

    /* Read Delayed Acceptance Setting */
    DataSize = sizeof(DWORD);
    HelperData->UseDelayedAcceptance = FALSE;
    RegQueryValueExW (KeyHandle,
                      L"UseDelayedAcceptance",
                      NULL,
                      NULL,
                      (LPBYTE)&HelperData->UseDelayedAcceptance,
                      &DataSize);

    /* Allocate Space for the Helper DLL Names */
    HelperDllName = HeapAlloc(GlobalHeap, 0, 512);

    /* Check for error */
    if (HelperDllName == NULL) {
        ERR("Buffer allocation failed\n");
        HeapFree(GlobalHeap, 0, HelperData);
        return WSAEINVAL;
    }

    FullHelperDllName = HeapAlloc(GlobalHeap, 0, 512);

    /* Check for error */
    if (FullHelperDllName == NULL) {
        ERR("Buffer allocation failed\n");
        HeapFree(GlobalHeap, 0, HelperDllName);
        HeapFree(GlobalHeap, 0, HelperData);
        return WSAEINVAL;
    }

    /* Get the name of the Helper DLL*/
    DataSize = 512;
    Status = RegQueryValueExW (KeyHandle,
                               L"HelperDllName",
                               NULL,
                               NULL,
                               (LPBYTE)HelperDllName,
                               &DataSize);

    /* Check for error */
    if (Status) {
        ERR("Error reading helper DLL parameters\n");
        HeapFree(GlobalHeap, 0, FullHelperDllName);
        HeapFree(GlobalHeap, 0, HelperDllName);
        HeapFree(GlobalHeap, 0, HelperData);
        return WSAEINVAL;
    }

    /* Get the Full name, expanding Environment Strings */
    ExpandEnvironmentStringsW (HelperDllName,
                               FullHelperDllName,
                               256);

    /* Load the DLL */
    HelperData->hInstance = LoadLibraryW(FullHelperDllName);

    HeapFree(GlobalHeap, 0, HelperDllName);
    HeapFree(GlobalHeap, 0, FullHelperDllName);

    if (HelperData->hInstance == NULL) {
        ERR("Error loading helper DLL\n");
        HeapFree(GlobalHeap, 0, HelperData);
        return WSAEINVAL;
    }

    /* Close Key */
    RegCloseKey(KeyHandle);

    /* Get the Pointers to the Helper Routines */
    HelperData->WSHOpenSocket =	(PWSH_OPEN_SOCKET)
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
							GetProcAddress(HelperData->hInstance,
							"WSHIoctl");

    /* Save the Mapping Structure and transport name */
    HelperData->Mapping = Mapping;
    wcscpy(HelperData->TransportName, TransportName);

    /* Increment Reference Count */
    HelperData->RefCount = 1;

    /* Add it to our list */
    InsertHeadList(&SockHelpersListHead, &HelperData->Helpers);

    /* Return Pointers */
    *HelperDllData = HelperData;
    return 0;
}

BOOL
SockIsTripleInMapping(
    PWINSOCK_MAPPING Mapping,
    INT AddressFamily,
    INT SocketType,
    INT Protocol)
{
    /* The Windows version returns more detailed information on which of the 3 parameters failed...we should do this later */
    ULONG    Row;

    TRACE("Called, Mapping rows = %d\n", Mapping->Rows);

    /* Loop through Mapping to Find a matching one */
    for (Row = 0; Row < Mapping->Rows; Row++) {
        TRACE("Examining: row %d: AF %d type %d proto %d\n",
				Row,
				(INT)Mapping->Mapping[Row].AddressFamily,
				(INT)Mapping->Mapping[Row].SocketType,
				(INT)Mapping->Mapping[Row].Protocol);

        /* Check of all three values Match */
        if (((INT)Mapping->Mapping[Row].AddressFamily == AddressFamily) &&
            ((INT)Mapping->Mapping[Row].SocketType == SocketType) &&
            ((INT)Mapping->Mapping[Row].Protocol == Protocol)) {
            TRACE("Found\n");
            return TRUE;
        }
    }
    WARN("Not found\n");
    return FALSE;
}

/* EOF */
