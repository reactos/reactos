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

VOID ReferenceProviderByPointer(
    PCATALOG_ENTRY Provider)
{
    WS_DbgPrint(MAX_TRACE, ("Provider (0x%X).\n", Provider));

    //EnterCriticalSection(&Provider->Lock);
    Provider->ReferenceCount++;
    //LeaveCriticalSection(&Provider->Lock);
}


VOID DereferenceProviderByPointer(
    PCATALOG_ENTRY Provider)
{
    WS_DbgPrint(MAX_TRACE, ("Provider (0x%X).\n", Provider));

#ifdef DBG
    if (Provider->ReferenceCount <= 0) {
        WS_DbgPrint(MIN_TRACE, ("Provider at 0x%X has invalid reference count (%ld).\n",
            Provider, Provider->ReferenceCount));
    }
#endif

    //EnterCriticalSection(&Provider->Lock);
    Provider->ReferenceCount--;
    //LeaveCriticalSection(&Provider->Lock);

    if (Provider->ReferenceCount == 0) {
        DestroyCatalogEntry(Provider);
    }
}


PCATALOG_ENTRY CreateCatalogEntry(
    LPWSTR LibraryName)
{
    PCATALOG_ENTRY Provider;

	WS_DbgPrint(MAX_TRACE, ("LibraryName (%S).\n", LibraryName));

    Provider = HeapAlloc(GlobalHeap, 0, sizeof(CATALOG_ENTRY));
    if (!Provider) {
		WS_DbgPrint(MIN_TRACE, ("Insufficient memory.\n"));
        return NULL;
	}

	CP

    ZeroMemory(Provider, sizeof(CATALOG_ENTRY));

	CP
    Provider->ReferenceCount = 1;

    //InitializeCriticalSection(&Provider->Lock);
	CP
    Provider->hModule = NULL;
	CP
    lstrcpyW(Provider->LibraryName, LibraryName);
	CP
    Provider->Mapping = NULL;

	CP
    //EnterCriticalSection(&CatalogLock);
	CP
    InsertTailList(&CatalogListHead, &Provider->ListEntry);
	CP
    //LeaveCriticalSection(&CatalogLock);
	CP

    return Provider;
}


INT DestroyCatalogEntry(
    PCATALOG_ENTRY Provider)
{
    INT Status;

    WS_DbgPrint(MAX_TRACE, ("Provider (0x%X).\n", Provider));

CP
    //EnterCriticalSection(&CatalogLock);
    RemoveEntryList(&Provider->ListEntry);
    //LeaveCriticalSection(&CatalogLock);
CP
    HeapFree(GlobalHeap, 0, Provider->Mapping);
CP
    if (Provider->hModule) {
		CP
        Status = UnloadProvider(Provider);
		CP
    } else {
        Status = NO_ERROR;
    }
CP
    //DeleteCriticalSection(&Provider->Lock);

    HeapFree(GlobalHeap, 0, Provider);
CP
    return Status;
}


PCATALOG_ENTRY LocateProvider(
    LPWSAPROTOCOL_INFOW lpProtocolInfo)
{
    PLIST_ENTRY CurrentEntry;
    PCATALOG_ENTRY Provider;
    UINT i;

	WS_DbgPrint(MAX_TRACE, ("lpProtocolInfo (0x%X).\n", lpProtocolInfo));

    //EnterCriticalSection(&CatalogLock);
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
                //LeaveCriticalSection(&CatalogLock);
				WS_DbgPrint(MID_TRACE, ("Returning provider at (0x%X).\n", Provider));
                return Provider;
            }
        }

        CurrentEntry = CurrentEntry->Flink;
    }
    //LeaveCriticalSection(&CatalogLock);

	WS_DbgPrint(MID_TRACE, ("Provider was not found.\n"));

    return NULL;
}


PCATALOG_ENTRY LocateProviderById(
    DWORD CatalogEntryId)
{
    PLIST_ENTRY CurrentEntry;
    PCATALOG_ENTRY Provider;
    UINT i;

	WS_DbgPrint(MAX_TRACE, ("CatalogEntryId (%d).\n", CatalogEntryId));

    //EnterCriticalSection(&CatalogLock);
    CurrentEntry = CatalogListHead.Flink;
    while (CurrentEntry != &CatalogListHead) {
	    Provider = CONTAINING_RECORD(CurrentEntry,
                                     CATALOG_ENTRY,
                                     ListEntry);

        if (Provider->ProtocolInfo.dwCatalogEntryId == CatalogEntryId) {
            //LeaveCriticalSection(&CatalogLock);
			WS_DbgPrint(MID_TRACE, ("Returning provider at (0x%X)  Name (%s).\n",
                Provider, Provider->LibraryName));
            return Provider;
        }

        CurrentEntry = CurrentEntry->Flink;
    }
    //LeaveCriticalSection(&CatalogLock);

	WS_DbgPrint(MID_TRACE, ("Provider was not found.\n"));

    return NULL;
}


INT LoadProvider(
    PCATALOG_ENTRY Provider,
    LPWSAPROTOCOL_INFOW lpProtocolInfo)
{
    INT Status;

    WS_DbgPrint(MAX_TRACE, ("Loading provider at (0x%X)  Name (%S).\n",
        Provider, Provider->LibraryName));

    if (!Provider->hModule) {
        CP
        /* DLL is not loaded so load it now */
        Provider->hModule = LoadLibrary(Provider->LibraryName);

        CP
        if (Provider->hModule) {
            CP
            Provider->WSPStartup = (LPWSPSTARTUP)GetProcAddress(Provider->hModule,
                                                                "WSPStartup");
            CP
            if (Provider->WSPStartup) {
				WS_DbgPrint(MAX_TRACE, ("Calling WSPStartup at (0x%X).\n", Provider->WSPStartup));
                Status = Provider->WSPStartup(MAKEWORD(2, 2),
                    &Provider->WSPData,
                    lpProtocolInfo,
                    UpcallTable,
                    &Provider->ProcTable);
                CP
            } else
                Status = ERROR_BAD_PROVIDER;
        } else
            Status = ERROR_DLL_NOT_FOUND;
    } else
        Status = NO_ERROR;

    WS_DbgPrint(MAX_TRACE, ("Status (%d).\n", Status));

    return Status;
}


INT UnloadProvider(
    PCATALOG_ENTRY Provider)
{
    INT Status = NO_ERROR;

    WS_DbgPrint(MAX_TRACE, ("Unloading provider at (0x%X)\n", Provider));

    if (Provider->hModule) {
        WS_DbgPrint(MAX_TRACE, ("Calling WSPCleanup at (0x%X).\n",
            Provider->ProcTable.lpWSPCleanup));
        Provider->ProcTable.lpWSPCleanup(&Status);

        if (!FreeLibrary(Provider->hModule)) {
            WS_DbgPrint(MIN_TRACE, ("Could not free library.\n"));
            Status = GetLastError();
        }

        Provider->hModule = (HMODULE)INVALID_HANDLE_VALUE;
    }

    WS_DbgPrint(MAX_TRACE, ("Status (%d).\n", Status));

    return Status;
}


VOID CreateCatalog(VOID)
{
    PCATALOG_ENTRY Provider;

	CP

	// FIXME: Crash
    //InitializeCriticalSection(&CatalogLock);

	CP

    InitializeListHead(&CatalogListHead);

	CP

    /* FIXME: Read service provider catalog from registry */
#if 1
    Provider = CreateCatalogEntry(L"msafd.dll");
    if (!Provider) {
		WS_DbgPrint(MIN_TRACE, ("Could not create catalog entry.\n"));
        return;
	}

	CP
    /* Assume one Service Provider with id 1 */
    Provider->ProtocolInfo.dwCatalogEntryId = 1;

	CP

    Provider->Mapping = HeapAlloc(GlobalHeap, 0, sizeof(WINSOCK_MAPPING) + 3 * sizeof(DWORD));
    if (!Provider->Mapping) {
		WS_DbgPrint(MIN_TRACE, ("Insufficient memory.\n"));
        return;
	}

	CP

    Provider->Mapping->Rows    = 1;
    Provider->Mapping->Columns = 3;
    Provider->Mapping->Mapping[0].AddressFamily = AF_INET;
    Provider->Mapping->Mapping[0].SocketType    = SOCK_RAW;
    Provider->Mapping->Mapping[0].Protocol      = 0;

	CP
#endif
}


VOID DestroyCatalog(VOID)
{
    PLIST_ENTRY CurrentEntry;
	PLIST_ENTRY NextEntry;
    PCATALOG_ENTRY Provider;

    CurrentEntry = CatalogListHead.Flink;
    while (CurrentEntry != &CatalogListHead) {
		NextEntry = CurrentEntry->Flink;
	    Provider = CONTAINING_RECORD(CurrentEntry,
                                     CATALOG_ENTRY,
                                     ListEntry);
        DestroyCatalogEntry(Provider);
        CurrentEntry = NextEntry;
    }
    //DeleteCriticalSection(&CatalogLock);
}

/* EOF */
