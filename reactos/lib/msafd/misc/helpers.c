/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        misc/helpers.c
 * PURPOSE:     Helper DLL management
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <msafd.h>
#include <helpers.h>

CRITICAL_SECTION HelperDLLDatabaseLock;
LIST_ENTRY HelperDLLDatabaseListHead;

PWSHELPER_DLL CreateHelperDLL(
    LPWSTR LibraryName)
{
    PWSHELPER_DLL HelperDLL;

    HelperDLL = HeapAlloc(GlobalHeap, 0, sizeof(WSHELPER_DLL));
    if (!HelperDLL)
        return NULL;

    InitializeCriticalSection(&HelperDLL->Lock);
    HelperDLL->hModule = NULL;
    lstrcpyW(HelperDLL->LibraryName, LibraryName);
    HelperDLL->Mapping = NULL;

    EnterCriticalSection(&HelperDLLDatabaseLock);
    InsertTailList(&HelperDLLDatabaseListHead, &HelperDLL->ListEntry);
    LeaveCriticalSection(&HelperDLLDatabaseLock);

    AFD_DbgPrint(MAX_TRACE, ("Returning helper at (0x%X).\n", HelperDLL));

    return HelperDLL;
}


INT DestroyHelperDLL(
    PWSHELPER_DLL HelperDLL)
{
    INT Status;

    AFD_DbgPrint(MAX_TRACE, ("HelperDLL (0x%X).\n", HelperDLL));

    EnterCriticalSection(&HelperDLLDatabaseLock);
    RemoveEntryList(&HelperDLL->ListEntry);
    LeaveCriticalSection(&HelperDLLDatabaseLock);

    if (HelperDLL->hModule) {
        Status = UnloadHelperDLL(HelperDLL);
    } else {
        Status = NO_ERROR;
    }

    if (HelperDLL->Mapping)
        HeapFree(GlobalHeap, 0, HelperDLL->Mapping);

    DeleteCriticalSection(&HelperDLL->Lock);

    HeapFree(GlobalHeap, 0, HelperDLL);

    return Status;
}


PWSHELPER_DLL LocateHelperDLL(
    LPWSAPROTOCOL_INFOW lpProtocolInfo)
{
    PLIST_ENTRY CurrentEntry;
    PWSHELPER_DLL HelperDLL;
    UINT i;

    EnterCriticalSection(&HelperDLLDatabaseLock);
    CurrentEntry = HelperDLLDatabaseListHead.Flink;
    while (CurrentEntry != &HelperDLLDatabaseListHead) {
	    HelperDLL = CONTAINING_RECORD(CurrentEntry,
                                      WSHELPER_DLL,
                                      ListEntry);

        for (i = 0; i < HelperDLL->Mapping->Rows; i++) {
            if ((lpProtocolInfo->iAddressFamily == (INT) HelperDLL->Mapping->Mapping[i].AddressFamily) &&
                (lpProtocolInfo->iSocketType    == (INT) HelperDLL->Mapping->Mapping[i].SocketType) &&
                ((lpProtocolInfo->iProtocol     == (INT) HelperDLL->Mapping->Mapping[i].Protocol) ||
                (lpProtocolInfo->iSocketType    == SOCK_RAW))) {
                LeaveCriticalSection(&HelperDLLDatabaseLock);
                AFD_DbgPrint(MAX_TRACE, ("Returning helper DLL at (0x%X).\n", HelperDLL));
                return HelperDLL;
            }
        }

        CurrentEntry = CurrentEntry->Flink;
    }
    LeaveCriticalSection(&HelperDLLDatabaseLock);

    AFD_DbgPrint(MAX_TRACE, ("Could not locate helper DLL.\n"));

    return NULL;
}


INT GetHelperDLLEntries(
    PWSHELPER_DLL HelperDLL)
{
    PVOID e;

    e = GetProcAddress(HelperDLL->hModule, "WSHAddressToString");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHAddressToString) = e;

    e = GetProcAddress(HelperDLL->hModule, "WSHEnumProtocols");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHEnumProtocols) = e;

    e = GetProcAddress(HelperDLL->hModule, "WSHGetBroadcastSockaddr");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHGetBroadcastSockaddr) = e;

    e = GetProcAddress(HelperDLL->hModule, "WSHGetProviderGuid");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHGetProviderGuid) = e;

	e = GetProcAddress(HelperDLL->hModule, "WSHGetSockaddrType");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHGetSockaddrType) = e;

    e = GetProcAddress(HelperDLL->hModule, "WSHGetSocketInformation");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHGetSocketInformation) = e;

    e = GetProcAddress(HelperDLL->hModule, "WSHGetWildcardSockaddr");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHGetWildcardSockaddr) = e;

    e = GetProcAddress(HelperDLL->hModule, "WSHGetWinsockMapping");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHGetWinsockMapping) = e;

    e = GetProcAddress(HelperDLL->hModule, "WSHGetWSAProtocolInfo");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHGetWSAProtocolInfo) = e;

    e = GetProcAddress(HelperDLL->hModule, "WSHIoctl");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHIoctl) = e;

    e = GetProcAddress(HelperDLL->hModule, "WSHJoinLeaf");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHJoinLeaf) = e;

    e = GetProcAddress(HelperDLL->hModule, "WSHNotify");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHNotify) = e;

    e = GetProcAddress(HelperDLL->hModule, "WSHOpenSocket");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHOpenSocket) = e;

    e = GetProcAddress(HelperDLL->hModule, "WSHOpenSocket2");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHOpenSocket2) = e;

    e = GetProcAddress(HelperDLL->hModule, "WSHSetSocketInformation");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHSetSocketInformation) = e;

    e = GetProcAddress(HelperDLL->hModule, "WSHStringToAddress");
    if (!e) return ERROR_BAD_PROVIDER;
	((PVOID) HelperDLL->EntryTable.lpWSHStringToAddress) = e;

    return NO_ERROR;
}


INT LoadHelperDLL(
    PWSHELPER_DLL HelperDLL)
{
    INT Status = NO_ERROR;

    AFD_DbgPrint(MAX_TRACE, ("Loading helper dll at (0x%X).\n", HelperDLL));

    if (!HelperDLL->hModule) {
        /* DLL is not loaded so load it now */
        HelperDLL->hModule = LoadLibrary(HelperDLL->LibraryName);

        AFD_DbgPrint(MAX_TRACE, ("hModule is (0x%X).\n", HelperDLL->hModule));

        if (HelperDLL->hModule) {
            Status = GetHelperDLLEntries(HelperDLL);
        } else
            Status = ERROR_DLL_NOT_FOUND;
    } else
        Status = NO_ERROR;

    AFD_DbgPrint(MAX_TRACE, ("Status (%d).\n", Status));

    return Status;
}


INT UnloadHelperDLL(
    PWSHELPER_DLL HelperDLL)
{
    INT Status = NO_ERROR;

    AFD_DbgPrint(MAX_TRACE, ("HelperDLL (0x%X) hModule (0x%X).\n", HelperDLL, HelperDLL->hModule));

    if (HelperDLL->hModule) {
        if (!FreeLibrary(HelperDLL->hModule)) {
            Status = GetLastError();
        }
        HelperDLL->hModule = NULL;
    }

    return Status;
}


VOID CreateHelperDLLDatabase(VOID)
{
    PWSHELPER_DLL HelperDLL;

    InitializeCriticalSection(&HelperDLLDatabaseLock);

    InitializeListHead(&HelperDLLDatabaseListHead);

    /* FIXME: Read helper DLL configuration from registry */
    HelperDLL = CreateHelperDLL(L"wshtcpip.dll");
    if (!HelperDLL)
        return;

    HelperDLL->Mapping = HeapAlloc(
      GlobalHeap,
      0,
      3 * sizeof(WINSOCK_MAPPING) + 3 * sizeof(DWORD));
    if (!HelperDLL->Mapping)
        return;

    HelperDLL->Mapping->Rows    = 3;
    HelperDLL->Mapping->Columns = 3;

    HelperDLL->Mapping->Mapping[0].AddressFamily = AF_INET;
    HelperDLL->Mapping->Mapping[0].SocketType    = SOCK_STREAM;
    HelperDLL->Mapping->Mapping[0].Protocol      = IPPROTO_TCP;

    HelperDLL->Mapping->Mapping[1].AddressFamily = AF_INET;
    HelperDLL->Mapping->Mapping[1].SocketType    = SOCK_DGRAM;
    HelperDLL->Mapping->Mapping[1].Protocol      = IPPROTO_UDP;

    HelperDLL->Mapping->Mapping[2].AddressFamily = AF_INET;
    HelperDLL->Mapping->Mapping[2].SocketType    = SOCK_RAW;
    HelperDLL->Mapping->Mapping[2].Protocol      = 0;

    LoadHelperDLL(HelperDLL);
}


VOID DestroyHelperDLLDatabase(VOID)
{
    PLIST_ENTRY CurrentEntry;
    PLIST_ENTRY NextEntry;
    PWSHELPER_DLL HelperDLL;

    CurrentEntry = HelperDLLDatabaseListHead.Flink;
    while (CurrentEntry != &HelperDLLDatabaseListHead) {
        NextEntry = CurrentEntry->Flink;

	      HelperDLL = CONTAINING_RECORD(CurrentEntry,
                                      WSHELPER_DLL,
                                      ListEntry);

        DestroyHelperDLL(HelperDLL);

        CurrentEntry = NextEntry;
    }

    DeleteCriticalSection(&HelperDLLDatabaseLock);
}

/* EOF */
