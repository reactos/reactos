/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        misc/handle.c
 * PURPOSE:     Provider handle management
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <ws2_32.h>
#include <handle.h>
#include <catalog.h>

PPROVIDER_HANDLE_BLOCK ProviderHandleTable;
CRITICAL_SECTION ProviderHandleTableLock;

PPROVIDER_HANDLE
GetProviderByHandle(
    PPROVIDER_HANDLE_BLOCK HandleTable,
    HANDLE Handle)
/*
 * FUNCTION: Get the data structure for a handle
 * ARGUMENTS:
 *     HandleTable = Pointer to handle table
 *     Handle      = Handle to get data structure for
 * RETURNS:
 *     Pointer to the data structure identified by the handle on success,
 *     NULL on failure
 */
{
    PPROVIDER_HANDLE_BLOCK Current;
    PLIST_ENTRY CurrentEntry;
    ULONG i;
   
    WS_DbgPrint(MAX_TRACE, ("HandleTable (0x%X)  Handle (0x%X).\n", HandleTable, Handle));

    CurrentEntry = HandleTable->Entry.Flink;
   
    while (CurrentEntry != &HandleTable->Entry) {
        Current = CONTAINING_RECORD(CurrentEntry, PROVIDER_HANDLE_BLOCK, Entry);

	    for (i = 0; i < HANDLE_BLOCK_ENTRIES; i++) {
	        if ((Current->Handles[i].Provider != NULL) && 
                (Current->Handles[i].Handle == Handle)) {

                return &Current->Handles[i];
	        }
        }
	    CurrentEntry = CurrentEntry->Flink;
    }

    return NULL;
}


VOID
CloseAllHandles(PPROVIDER_HANDLE_BLOCK HandleTable)
{
    PPROVIDER_HANDLE_BLOCK Current;
    PLIST_ENTRY CurrentEntry;
    PCATALOG_ENTRY Provider;
    ULONG i;
   
    WS_DbgPrint(MAX_TRACE, ("HandleTable (0x%X).\n", HandleTable));

    CurrentEntry = HandleTable->Entry.Flink;
   
    while (CurrentEntry != &HandleTable->Entry) {
        Current = CONTAINING_RECORD(CurrentEntry, PROVIDER_HANDLE_BLOCK, Entry);
	
	    for (i = 0; i < HANDLE_BLOCK_ENTRIES; i++) {
            Provider = Current->Handles[i].Provider;
	     
	        if (Provider != NULL) {
                DereferenceProviderByPointer(Provider);
                Current->Handles[i].Handle   = (HANDLE)0;
		        Current->Handles[i].Provider = NULL;
                break;
	        }
	    }
	    CurrentEntry = CurrentEntry->Flink;
    }
}


VOID
DeleteHandleTable(PPROVIDER_HANDLE_BLOCK HandleTable)
{
    PPROVIDER_HANDLE_BLOCK Current;
    PLIST_ENTRY CurrentEntry;

    CloseAllHandles(HandleTable);
   
    CurrentEntry = RemoveHeadList(&HandleTable->Entry);
   
    while (CurrentEntry != &HandleTable->Entry) {
        Current = CONTAINING_RECORD(CurrentEntry,
            PROVIDER_HANDLE_BLOCK,
    		Entry);

	    HeapFree(GlobalHeap, 0, Current);

	    CurrentEntry = RemoveHeadList(&HandleTable->Entry);
    }
}


PCATALOG_ENTRY
DeleteProviderHandle(PPROVIDER_HANDLE_BLOCK HandleTable,
                     HANDLE Handle)
{
    PPROVIDER_HANDLE Entry;
    PCATALOG_ENTRY Provider;

    WS_DbgPrint(MAX_TRACE, ("HandleTable (0x%X)  Handle (0x%X).\n", HandleTable, Handle));

    Entry = GetProviderByHandle(HandleTable, Handle);
    if (!Entry) {
	    return NULL;
    }

    Provider = Entry->Provider;

    if (Provider != NULL) {
        Entry->Handle = (HANDLE)0;
	    Entry->Provider = NULL;
    }

    return Provider;
}


HANDLE
CreateProviderHandleTable(PPROVIDER_HANDLE_BLOCK HandleTable,
                          HANDLE Handle,
                          PCATALOG_ENTRY Provider)
{
    PPROVIDER_HANDLE_BLOCK NewBlock;
    PLIST_ENTRY CurrentEntry;
    ULONG i;

    WS_DbgPrint(MAX_TRACE, ("HandleTable (0x%X)  Handle (0x%X)  Provider (0x%X).\n", HandleTable, Handle, Provider));

    /* Scan through the currently allocated handle blocks looking for a free slot */
    CurrentEntry = HandleTable->Entry.Flink;
    while (CurrentEntry != &HandleTable->Entry) {
        PPROVIDER_HANDLE_BLOCK Block = CONTAINING_RECORD(
            CurrentEntry, PROVIDER_HANDLE_BLOCK, Entry);

        for (i = 0; i < HANDLE_BLOCK_ENTRIES; i++) {
            WS_DbgPrint(MAX_TRACE, ("Considering slot %ld containing 0x%X.\n", i, Block->Handles[i].Provider));
	        if (!Block->Handles[i].Provider) {
                Block->Handles[i].Handle   = Handle;
		        Block->Handles[i].Provider = Provider;
    	        return Handle;
	        }
	    }
	    CurrentEntry = CurrentEntry->Flink;
    }
   
    /* Add a new handle block to the end of the list */
    NewBlock = (PPROVIDER_HANDLE_BLOCK)HeapAlloc(
        GlobalHeap, 0, sizeof(PROVIDER_HANDLE_BLOCK));

    if (!NewBlock) {
		WS_DbgPrint(MIN_TRACE, ("Insufficient memory.\n"));
        return NULL;
	}

    ZeroMemory(NewBlock, sizeof(PROVIDER_HANDLE_BLOCK));
    InsertTailList(&HandleTable->Entry, &NewBlock->Entry);

    NewBlock->Handles[0].Handle   = Handle;
    NewBlock->Handles[0].Provider = Provider;

    return Handle;
}


HANDLE
CreateProviderHandle(HANDLE Handle,
                     PCATALOG_ENTRY Provider)
{
    HANDLE h;

    //EnterCriticalSection(&ProviderHandleTableLock);

    h = CreateProviderHandleTable(ProviderHandleTable, Handle, Provider);

    //LeaveCriticalSection(&ProviderHandleTableLock);

    if (h != NULL) {
        ReferenceProviderByPointer(Provider);
    }

    return h;
}

    
BOOL
ReferenceProviderByHandle(HANDLE Handle,
                          PCATALOG_ENTRY* Provider)
/*
 * FUNCTION: Increments the reference count for a provider and returns a pointer to it
 * ARGUMENTS:
 *     Handle   = Handle for the provider
 *     Provider = Address of buffer to place pointer to provider
 * RETURNS:
 *     TRUE if handle was valid, FALSE if not
 */
{
    PPROVIDER_HANDLE ProviderHandle;

	WS_DbgPrint(MAX_TRACE, ("Handle (0x%X)  Provider (0x%X).\n", Handle, Provider));

    //EnterCriticalSection(&ProviderHandleTableLock);

    ProviderHandle = GetProviderByHandle(ProviderHandleTable, Handle);

    //LeaveCriticalSection(&ProviderHandleTableLock);

    if (ProviderHandle) {
        ReferenceProviderByPointer(ProviderHandle->Provider);
        *Provider = ProviderHandle->Provider;
    }

    return (ProviderHandle != NULL);
}


BOOL
CloseProviderHandle(HANDLE Handle)
{
    PCATALOG_ENTRY Provider;
   
    WS_DbgPrint(MAX_TRACE, ("Handle (0x%X).\n", Handle));

    //EnterCriticalSection(&ProviderHandleTableLock);

    Provider = DeleteProviderHandle(ProviderHandleTable, Handle);
    if (!Provider) {
		WS_DbgPrint(MIN_TRACE, ("Insufficient memory.\n"));
        return FALSE;
	}

    //LeaveCriticalSection(&ProviderHandleTableLock);

    DereferenceProviderByPointer(Provider);

    return TRUE;
}


BOOL
InitProviderHandleTable(VOID)
{
    ProviderHandleTable = (PPROVIDER_HANDLE_BLOCK)
        HeapAlloc(GlobalHeap, 0, sizeof(PROVIDER_HANDLE_BLOCK));
    if (!ProviderHandleTable) {
		WS_DbgPrint(MIN_TRACE, ("Insufficient memory.\n"));
        return FALSE;
	}

    WS_DbgPrint(MIN_TRACE, ("ProviderHandleTable at 0x%X.\n", ProviderHandleTable));

    ZeroMemory(ProviderHandleTable, sizeof(PROVIDER_HANDLE_BLOCK));

    InitializeListHead(&ProviderHandleTable->Entry);

    //InitializeCriticalSection(&ProviderHandleTableLock);

    return TRUE;
}


VOID
FreeProviderHandleTable(VOID)
{
    DeleteHandleTable(ProviderHandleTable);

    //DeleteCriticalSection(&ProviderHandleTableLock);
}

/* EOF */
