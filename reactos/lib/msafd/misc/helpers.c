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
#include <helpers.h>

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
	PHELPER_DATA		HelperData;
	PWSTR				Transports;
    PWSTR				Transport;
    PWINSOCK_MAPPING	Mapping;
	PLIST_ENTRY			Helpers;

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
			if(HelperData->WSHOpenSocket2 == NULL ) {

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
    SockLoadTransportList(&Transports);
    
    /* Loop through each transport until we find one that can satisfy us */
    for (Transport = Transports; 
			*Transports != 0; 
			Transport += wcslen(Transport) + 1) {

		/* See what mapping this Transport supports */
        SockLoadTransportMapping(Transport, &Mapping);
        
		/* See if this Mapping works for us */
        if (SockIsTripleInMapping(Mapping, *AddressFamily, *SocketType, *Protocol)) {

            /* It does, so load the DLL associated with it */
            SockLoadHelperDll(Transport, Mapping, &HelperData);

			/* Call the Helper Dll function get the Transport Name */
			if(HelperData->WSHOpenSocket2 == NULL ) {

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
        
		continue;
    }
    return WSAEINVAL;
}

INT
SockLoadTransportMapping(
	PWSTR TransportName, 
	PWINSOCK_MAPPING *Mapping)
{
    PWSTR				TransportKey;
    HKEY				KeyHandle;
    ULONG				MappingSize;

	/* Allocate a Buffer */
    TransportKey = HeapAlloc(GlobalHeap, 0, 512);

	/* Generate the right key name */
    wcscpy(TransportKey, L"System\\CurrentControlSet\\Services\\");
    wcscat(TransportKey, TransportName);
    wcscat(TransportKey, L"\\Parameters\\Winsock");

	/* Open the Key */
    RegOpenKeyExW(HKEY_LOCAL_MACHINE, TransportKey, 0, KEY_READ, &KeyHandle);

    /* Find out how much space we need for the Mapping */
	RegQueryValueExW(KeyHandle, L"Mapping", NULL, NULL, NULL, &MappingSize);

	/* Allocate Memory for the Mapping */
    *Mapping = HeapAlloc(GlobalHeap, 0, MappingSize);

	/* Read the Mapping */
    RegQueryValueExW(KeyHandle, L"Mapping", NULL, NULL, (LPBYTE)*Mapping, &MappingSize);
    
    /* Close key and return*/
    RegCloseKey(KeyHandle);
    return 0;
}

INT 
SockLoadTransportList(
	PWSTR *TransportList)
{
    ULONG	TransportListSize;
	HKEY	KeyHandle;

	/* Open the Transports Key */
    RegOpenKeyExW (HKEY_LOCAL_MACHINE,
					L"SYSTEM\\CurrentControlSet\\Services\\Winsock\\Parameters",  
					0, 
					KEY_READ, 
					&KeyHandle);
    
	/* Get the Transport List Size*/
    RegQueryValueExW(KeyHandle, L"Transports", NULL, NULL, NULL, &TransportListSize);

	/* Allocate Memory for the Transport List */
    *TransportList = HeapAlloc(GlobalHeap, 0, TransportListSize);

    /* Get the Transports */
	RegQueryValueExW (KeyHandle, 
						L"Transports", 
						NULL, 
						NULL, 
						(LPBYTE)*TransportList, 
						&TransportListSize);

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
    PHELPER_DATA	HelperData;
    PWSTR			HelperDllName;
    PWSTR			FullHelperDllName;
    ULONG			HelperDllNameSize;
    PWSTR			HelperKey;
    HKEY			KeyHandle;
    ULONG			DataSize;
    
    /* Allocate space for the Helper Structure and TransportName */
    HelperData = HeapAlloc(GlobalHeap, 0, sizeof(*HelperData) + wcslen(TransportName) + 1);
    
	/* Allocate Space for the Helper DLL Names */
    HelperDllName = HeapAlloc(GlobalHeap, 0, 512);
    FullHelperDllName = HeapAlloc(GlobalHeap, 0, 512);

	/* Allocate Space for the Helper DLL Key */
    HelperKey = HeapAlloc(GlobalHeap, 0, 512);
	
	/* Generate the right key name */
    wcscpy(HelperKey, L"System\\CurrentControlSet\\Services\\");
    wcscat(HelperKey, TransportName);
    wcscat(HelperKey, L"\\Parameters\\Winsock");

	/* Open the Key */
    RegOpenKeyExW(HKEY_LOCAL_MACHINE, HelperKey, 0, KEY_READ, &KeyHandle);
    
	/* Read Size of SockAddr Structures */
    DataSize = sizeof(HelperData->MinWSAddressLength);
	RegQueryValueExW (KeyHandle, 
						L"MinSockaddrLength", 
						NULL, 
						NULL, 
						(LPBYTE)&HelperData->MinWSAddressLength, 
						&DataSize);
    DataSize = sizeof(HelperData->MinWSAddressLength);
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
	RegQueryValueExW (KeyHandle, 
						L"UseDelayedAcceptance", 
						NULL, 
						NULL, 
						(LPBYTE)&HelperData->UseDelayedAcceptance, 
						&DataSize);

	/* Get the name of the Helper DLL*/
    DataSize = 512;
    RegQueryValueExW (KeyHandle, 
						L"HelperDllName", 
						NULL, 
						NULL, 
						(LPBYTE)HelperDllName, 
						&DataSize);

	/* Get the Full name, expanding Environment Strings */
    HelperDllNameSize = ExpandEnvironmentStringsW (HelperDllName,
													FullHelperDllName, 
													256);

	/* Load the DLL */
    HelperData->hInstance = LoadLibraryW(FullHelperDllName);

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
    HelperData->WSHGetWildcardSockaddr = (PWSH_GET_WILDCARD_SOCKEADDR)
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
    ULONG	Row;
    
    /* Loop through Mapping to Find a matching one */
    for (Row = 0; Row < Mapping->Rows; Row++) {
        /* Check of all three values Match */
        if (((INT)Mapping->Mapping[Row].AddressFamily == AddressFamily) && 
			((INT)Mapping->Mapping[Row].SocketType == SocketType) && 
			((INT)Mapping->Mapping[Row].Protocol == Protocol)) {
			return TRUE;
        }
    }
    return FALSE;

}

/* EOF */
