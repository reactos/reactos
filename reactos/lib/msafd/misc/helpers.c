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
            if ((lpProtocolInfo->iAddressFamily == HelperDLL->Mapping->Mapping[i].AddressFamily) &&
                (lpProtocolInfo->iSocketType    == HelperDLL->Mapping->Mapping[i].SocketType) &&
                ((lpProtocolInfo->iProtocol     == HelperDLL->Mapping->Mapping[i].Protocol) ||
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


#define GET_ENTRY_POINT(helper, exportname, identifier) { \
    PVOID entry;                        \
                                        \
    entry = GetProcAddress(helper->hModule, exportname); \
    if (!entry)                         \
        return ERROR_BAD_PROVIDER;      \
    ((PVOID)helper->EntryTable.##identifier) = entry; \
}


INT GetHelperDLLEntries(
    PWSHELPER_DLL HelperDLL)
{
    GET_ENTRY_POINT(HelperDLL, "WSHAddressToString", lpWSHAddressToString);
    GET_ENTRY_POINT(HelperDLL, "WSHEnumProtocols", lpWSHEnumProtocols);
    GET_ENTRY_POINT(HelperDLL, "WSHGetBroadcastSockaddr", lpWSHGetBroadcastSockaddr);
    GET_ENTRY_POINT(HelperDLL, "WSHGetProviderGuid", lpWSHGetProviderGuid);
    GET_ENTRY_POINT(HelperDLL, "WSHGetSockaddrType", lpWSHGetSockaddrType);
    GET_ENTRY_POINT(HelperDLL, "WSHGetSocketInformation", lpWSHGetSocketInformation);
    GET_ENTRY_POINT(HelperDLL, "WSHGetWildcardSockaddr", lpWSHGetWildcardSockaddr);
    GET_ENTRY_POINT(HelperDLL, "WSHGetWinsockMapping", lpWSHGetWinsockMapping);
    GET_ENTRY_POINT(HelperDLL, "WSHGetWSAProtocolInfo", lpWSHGetWSAProtocolInfo);
    GET_ENTRY_POINT(HelperDLL, "WSHIoctl", lpWSHIoctl);
    GET_ENTRY_POINT(HelperDLL, "WSHJoinLeaf", lpWSHJoinLeaf);
    GET_ENTRY_POINT(HelperDLL, "WSHNotify", lpWSHNotify);
    GET_ENTRY_POINT(HelperDLL, "WSHOpenSocket", lpWSHOpenSocket);
    GET_ENTRY_POINT(HelperDLL, "WSHOpenSocket2", lpWSHOpenSocket2);
    GET_ENTRY_POINT(HelperDLL, "WSHSetSocketInformation", lpWSHSetSocketInformation);
    GET_ENTRY_POINT(HelperDLL, "WSHStringToAddress", lpWSHStringToAddress);

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
            CP
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

    HelperDLL->Mapping = HeapAlloc(GlobalHeap, 0, sizeof(WINSOCK_MAPPING) + 3 * sizeof(DWORD));
    if (!HelperDLL->Mapping)
        return;

    HelperDLL->Mapping->Rows    = 1;
    HelperDLL->Mapping->Columns = 3;
    HelperDLL->Mapping->Mapping[0].AddressFamily = AF_INET;
    HelperDLL->Mapping->Mapping[0].SocketType    = SOCK_RAW;
    HelperDLL->Mapping->Mapping[0].Protocol      = 0;

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
