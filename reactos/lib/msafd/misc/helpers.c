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

//CRITICAL_SECTION HelperDLLDatabaseLock;
LIST_ENTRY HelperDLLDatabaseListHead;

PWSHELPER_DLL CreateHelperDLL(
    LPWSTR LibraryName)
{
    PWSHELPER_DLL HelperDLL;

    HelperDLL = HeapAlloc(GlobalHeap, 0, sizeof(WSHELPER_DLL));
    if (!HelperDLL) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient memory.\n"));
        return NULL;
    }

    //InitializeCriticalSection(&HelperDLL->Lock);
    HelperDLL->hModule = NULL;
    lstrcpyW(HelperDLL->LibraryName, LibraryName);
    HelperDLL->Mapping = NULL;

    //EnterCriticalSection(&HelperDLLDatabaseLock);
    InsertTailList(&HelperDLLDatabaseListHead, &HelperDLL->ListEntry);
    //LeaveCriticalSection(&HelperDLLDatabaseLock);

    AFD_DbgPrint(MAX_TRACE, ("Returning helper at (0x%X).\n", HelperDLL));

    return HelperDLL;
}


INT DestroyHelperDLL(
    PWSHELPER_DLL HelperDLL)
{
    INT Status;

    AFD_DbgPrint(MAX_TRACE, ("HelperDLL (0x%X).\n", HelperDLL));

    //EnterCriticalSection(&HelperDLLDatabaseLock);
    RemoveEntryList(&HelperDLL->ListEntry);
    //LeaveCriticalSection(&HelperDLLDatabaseLock);

    CP

    if (HelperDLL->hModule) {
        Status = UnloadHelperDLL(HelperDLL);
    } else {
        Status = NO_ERROR;
    }

    CP

    if (HelperDLL->Mapping)
        HeapFree(GlobalHeap, 0, HelperDLL->Mapping);

    //DeleteCriticalSection(&HelperDLL->Lock);

    HeapFree(GlobalHeap, 0, HelperDLL);

    CP

    return Status;
}


PWSHELPER_DLL LocateHelperDLL(
    LPWSAPROTOCOL_INFOW lpProtocolInfo)
{
    PLIST_ENTRY CurrentEntry;
    PWSHELPER_DLL HelperDLL;
    UINT i;

    //EnterCriticalSection(&HelperDLLDatabaseLock);
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
                //LeaveCriticalSection(&HelperDLLDatabaseLock);
                AFD_DbgPrint(MAX_TRACE, ("Returning helper DLL at (0x%X).\n", HelperDLL));
                return HelperDLL;
            }
        }

        CurrentEntry = CurrentEntry->Flink;
    }
    //LeaveCriticalSection(&HelperDLLDatabaseLock);

    AFD_DbgPrint(MAX_TRACE, ("Could not locate helper DLL.\n"));

    return NULL;
}


#define GET_ENTRY_POINT(helper, name) { \
    PVOID entry;                        \
                                        \
    entry = GetProcAddress(helper->hModule, "##name"); \
    if (!entry)                         \
        return ERROR_BAD_PROVIDER;      \
    (*(PULONG*)helper->EntryTable.lp##name) = entry; \
}


INT GetHelperDLLEntries(
    PWSHELPER_DLL HelperDLL)
{
    GET_ENTRY_POINT(HelperDLL, WSHAddressToString);
    GET_ENTRY_POINT(HelperDLL, WSHEnumProtocols);
    GET_ENTRY_POINT(HelperDLL, WSHGetBroadcastSockaddr);
    GET_ENTRY_POINT(HelperDLL, WSHGetProviderGuid);
    GET_ENTRY_POINT(HelperDLL, WSHGetSockaddrType);
    GET_ENTRY_POINT(HelperDLL, WSHGetSocketInformation);
    GET_ENTRY_POINT(HelperDLL, WSHGetWildcardSockaddr);
    GET_ENTRY_POINT(HelperDLL, WSHGetWinsockMapping);
    GET_ENTRY_POINT(HelperDLL, WSHGetWSAProtocolInfo);
    GET_ENTRY_POINT(HelperDLL, WSHIoctl);
    GET_ENTRY_POINT(HelperDLL, WSHJoinLeaf);
    GET_ENTRY_POINT(HelperDLL, WSHNotify);
    GET_ENTRY_POINT(HelperDLL, WSHOpenSocket);
    GET_ENTRY_POINT(HelperDLL, WSHOpenSocket2);
    GET_ENTRY_POINT(HelperDLL, WSHSetSocketInformation);
    GET_ENTRY_POINT(HelperDLL, WSHStringToAddress);
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
        if (HelperDLL->hModule) {
            Status = GetHelperDLLEntries(HelperDLL);
        } else
            Status = ERROR_DLL_NOT_FOUND;
    } else
        Status = NO_ERROR;

    AFD_DbgPrint(MIN_TRACE, ("Status (%d).\n", Status));

    return Status;
}


INT UnloadHelperDLL(
    PWSHELPER_DLL HelperDLL)
{
    INT Status = NO_ERROR;

    AFD_DbgPrint(MAX_TRACE, ("HelperDLL (0x%X).\n", HelperDLL));

    if (HelperDLL->hModule) {
        if (!FreeLibrary(HelperDLL->hModule))
            Status = GetLastError();

        HelperDLL->hModule = NULL;
    }

    return Status;
}


VOID CreateHelperDLLDatabase(VOID)
{
    PWSHELPER_DLL HelperDLL;

    CP

    //InitializeCriticalSection(&HelperDLLDatabaseLock);

    InitializeListHead(&HelperDLLDatabaseListHead);

    /* FIXME: Read helper DLL configuration from registry */
    HelperDLL = CreateHelperDLL(L"wshtcpip.dll");
    if (!HelperDLL) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient memory.\n"));
        return;
    }

    CP

    HelperDLL->Mapping = HeapAlloc(GlobalHeap, 0, sizeof(WINSOCK_MAPPING) + 3 * sizeof(DWORD));
    if (!HelperDLL->Mapping) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient memory.\n"));
        return;
    }

    CP

    HelperDLL->Mapping->Rows    = 1;
    HelperDLL->Mapping->Columns = 3;
    HelperDLL->Mapping->Mapping[0].AddressFamily = AF_INET;
    HelperDLL->Mapping->Mapping[0].SocketType    = SOCK_RAW;
    HelperDLL->Mapping->Mapping[0].Protocol      = 0;

    CP

    LoadHelperDLL(HelperDLL);

    CP
}


VOID DestroyHelperDLLDatabase(VOID)
{
    PLIST_ENTRY CurrentEntry;
    PLIST_ENTRY NextEntry;
    PWSHELPER_DLL HelperDLL;

    CP

    CurrentEntry = HelperDLLDatabaseListHead.Flink;
    while (CurrentEntry != &HelperDLLDatabaseListHead) {
        NextEntry = CurrentEntry->Flink;
	    HelperDLL = CONTAINING_RECORD(CurrentEntry,
                                      WSHELPER_DLL,
                                      ListEntry);

        DestroyHelperDLL(HelperDLL);

        CurrentEntry = NextEntry;
    }

    CP

    //DeleteCriticalSection(&HelperDLLDatabaseLock);
}

/* EOF */
