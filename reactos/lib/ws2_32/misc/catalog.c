/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        misc/catalog.c
 * PURPOSE:     Service Provider Catalog
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <ws2_32.h>
#include <catalog.h>


LIST_ENTRY CatalogListHead;
CRITICAL_SECTION CatalogLock;

PCATALOG_ENTRY CreateCatalogEntry(
    LPWSTR LibraryName)
{
    PCATALOG_ENTRY Provider;

    Provider = HeapAlloc(GlobalHeap, 0, sizeof(CATALOG_ENTRY));
    if (!Provider)
        return NULL;

    InitializeCriticalSection(&Provider->Lock);
    Provider->hModule = (HMODULE)INVALID_HANDLE_VALUE;
    lstrcpyW(Provider->LibraryName, LibraryName);
    Provider->Mapping = NULL;

    EnterCriticalSection(&CatalogLock);
    InsertTailList(&CatalogListHead, &Provider->ListEntry);
    LeaveCriticalSection(&CatalogLock);

    return Provider;
}


INT DestroyCatalogEntry(
    PCATALOG_ENTRY Provider)
{
    INT Status;

    EnterCriticalSection(&CatalogLock);
    RemoveEntryList(&Provider->ListEntry);
    LeaveCriticalSection(&CatalogLock);

    HeapFree(GlobalHeap, 0, Provider->Mapping);

    if (Provider->hModule != (HMODULE)INVALID_HANDLE_VALUE) {
        Status = UnloadProvider(Provider);
    } else {
        Status = NO_ERROR;
    }

    DeleteCriticalSection(&Provider->Lock);

    HeapFree(GlobalHeap, 0, Provider);

    return Status;
}


PCATALOG_ENTRY LocateProvider(
    LPWSAPROTOCOL_INFOW lpProtocolInfo)
{
    PLIST_ENTRY CurrentEntry;
    PCATALOG_ENTRY Provider;
    UINT i;

    EnterCriticalSection(&CatalogLock);
    CurrentEntry = CatalogListHead.Flink;
    while (CurrentEntry != &CatalogListHead) {
	    Provider = CONTAINING_RECORD(CurrentEntry,
                                     CATALOG_ENTRY,
                                     ListEntry);

        for (i = 0; i < Provider->Mapping->Rows; i++) {
            if ((lpProtocolInfo->iAddressFamily == Provider->Mapping->Mapping[i].AddressFamily) &&
                (lpProtocolInfo->iSocketType    == Provider->Mapping->Mapping[i].SocketType) &&
                ((lpProtocolInfo->iProtocol     == Provider->Mapping->Mapping[i].Protocol) ||
                (lpProtocolInfo->iSocketType    == SOCK_RAW))) {
                LeaveCriticalSection(&CatalogLock);
                return Provider;
            }
        }

        CurrentEntry = CurrentEntry->Flink;
    }
    LeaveCriticalSection(&CatalogLock);

    return NULL;
}


INT LoadProvider(
    PCATALOG_ENTRY Provider,
    LPWSAPROTOCOL_INFOW lpProtocolInfo)
{
    INT Status = NO_ERROR;

    WS_DbgPrint(MIN_TRACE, ("Loading provider...\n"));

    if (Provider->hModule == (HMODULE)INVALID_HANDLE_VALUE) {
        /* DLL is not loaded so load it now */
        Provider->hModule = LoadLibrary(Provider->LibraryName);
        if (Provider->hModule != (HMODULE)INVALID_HANDLE_VALUE) {
            Provider->WSPStartup = (LPWSPSTARTUP)GetProcAddress(Provider->hModule,
                                                                "WSPStartup");
            if (Provider->WSPStartup) {
                Status = WSPStartup(MAKEWORD(2, 2),
                                    &Provider->WSPData,
                                    lpProtocolInfo,
                                    UpcallTable,
                                    &Provider->ProcTable);
            } else
                Status = ERROR_BAD_PROVIDER;
        } else
            Status = ERROR_DLL_NOT_FOUND;
    } else
        Status = NO_ERROR;

    WS_DbgPrint(MIN_TRACE, ("Status %d\n", Status));

    return Status;
}


INT UnloadProvider(
    PCATALOG_ENTRY Provider)
{
    INT Status = NO_ERROR;

    if (Provider->hModule != (HMODULE)INVALID_HANDLE_VALUE) {
        Provider->ProcTable.lpWSPCleanup(&Status);

        if (!FreeLibrary(Provider->hModule))
            Status = GetLastError();

        Provider->hModule = (HMODULE)INVALID_HANDLE_VALUE;
    }

    return Status;
}


VOID CreateCatalog(VOID)
{
    PCATALOG_ENTRY Provider;

    InitializeCriticalSection(&CatalogLock);

    InitializeListHead(&CatalogListHead);

    /* FIXME: Read service provider catalog from registry */
#if 1
    Provider = CreateCatalogEntry(L"msafd.dll");
    if (!Provider)
        return;

    Provider->Mapping = HeapAlloc(GlobalHeap, 0, sizeof(WINSOCK_MAPPING) + 3 * sizeof(DWORD));
    if (!Provider->Mapping)
        return;

    Provider->Mapping->Rows    = 1;
    Provider->Mapping->Columns = 3;
    Provider->Mapping->Mapping[0].AddressFamily = AF_INET;
    Provider->Mapping->Mapping[0].SocketType    = SOCK_RAW;
    Provider->Mapping->Mapping[0].Protocol      = 0;
#endif
}


VOID DestroyCatalog(VOID)
{
    PLIST_ENTRY CurrentEntry;
    PCATALOG_ENTRY Provider;

    CurrentEntry = CatalogListHead.Flink;
    while (CurrentEntry != &CatalogListHead) {
	    Provider = CONTAINING_RECORD(CurrentEntry,
                                     CATALOG_ENTRY,
                                     ListEntry);

        DestroyCatalogEntry(Provider);

        CurrentEntry = CurrentEntry->Flink;
    }

    DeleteCriticalSection(&CatalogLock);
}

/* EOF */
