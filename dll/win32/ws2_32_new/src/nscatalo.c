/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/nscatalo.c
 * PURPOSE:     Namespace Catalog Object
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

/* DATA **********************************************************************/

#define NSCATALOG_NAME      "NameSpace_Catalog5"
#define WsNcLock()          EnterCriticalSection((LPCRITICAL_SECTION)&Catalog->Lock);
#define WsNcUnlock()        LeaveCriticalSection((LPCRITICAL_SECTION)&Catalog->Lock);

/* FUNCTIONS *****************************************************************/

PNSCATALOG
WSAAPI
WsNcAllocate(VOID)
{
    PNSCATALOG Catalog;

    /* Allocate the catalog */
    Catalog = HeapAlloc(WsSockHeap, HEAP_ZERO_MEMORY, sizeof(*Catalog));

    /* Return it */
    return Catalog;
}

BOOLEAN
WSAAPI
WsNcOpen(IN PNSCATALOG Catalog,
         IN HKEY ParentKey)
{
    LONG ErrorCode;
    DWORD CreateDisposition;
    HKEY CatalogKey, NewKey;
    //DWORD CatalogEntries = 0;
    DWORD RegType = REG_DWORD;
    DWORD RegSize = sizeof(DWORD);
    DWORD UniqueId = 0;
    DWORD NewData = 0;

    /* Initialize the catalog lock and namespace list */
    InitializeCriticalSection((LPCRITICAL_SECTION)&Catalog->Lock);
    InitializeListHead(&Catalog->CatalogList);

    /* Open the Catalog Key */
    ErrorCode = RegOpenKeyEx(ParentKey,
                             NSCATALOG_NAME,
                             0,
                             MAXIMUM_ALLOWED,
                             &CatalogKey);

    /* If we didn't find the key, create it */
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* Fake that we opened an existing key */
        CreateDisposition = REG_OPENED_EXISTING_KEY;
    }
    else if (ErrorCode == ERROR_FILE_NOT_FOUND)
    {
        /* Create the Catalog Name */
        ErrorCode = RegCreateKeyEx(ParentKey,
                                   NSCATALOG_NAME,
                                   0,
                                   NULL,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_ALL_ACCESS,
                                   NULL,
                                   &CatalogKey,
                                   &CreateDisposition);
    }

    /* Fail if that didn't work */
    if (ErrorCode != ERROR_SUCCESS) return FALSE;

    /* Check if we had to create the key */
    if (CreateDisposition == REG_CREATED_NEW_KEY)
    {
        /* Write the count of entries (0 now) */
        ErrorCode = RegSetValueEx(CatalogKey,
                                  "Num_Catalog_Entries",
                                  0,
                                  REG_DWORD,
                                  (LPBYTE)&NewData,
                                  sizeof(NewData));
        if (ErrorCode != ERROR_SUCCESS)
        {
            /* Close the key and fail */
            RegCloseKey(CatalogKey);
            return FALSE;
        }

        /* Write the first catalog entry Uniqe ID */
        NewData = 1;
        ErrorCode = RegSetValueEx(CatalogKey,
                                  "Serial_Access_Num",
                                  0,
                                  REG_DWORD,
                                  (LPBYTE)&NewData,
                                  sizeof(NewData));
        if (ErrorCode != ERROR_SUCCESS)
        {
            /* Close the key and fail */
            RegCloseKey(CatalogKey);
            return FALSE;
        }

        /* Create a key for this entry */
        ErrorCode = RegCreateKeyEx(CatalogKey,
                                   "Catalog_Entries",
                                   0,
                                   NULL,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_ALL_ACCESS,
                                   NULL,
                                   &NewKey,
                                   &CreateDisposition);
        if (ErrorCode != ERROR_SUCCESS)
        {
            /* Close the key and fail */
            RegCloseKey(CatalogKey);
            return FALSE;
        }

        /* Close the key since we don't need it */
        RegCloseKey(NewKey);
    }
    else
    {
        /* Read the serial number */
        ErrorCode = RegQueryValueEx(CatalogKey,
                                    "Serial_Access_Num",
                                    0,
                                    &RegType,
                                    (LPBYTE)&UniqueId,
                                    &RegSize);

        /* Check if it's missing for some reason */
        if (ErrorCode != ERROR_SUCCESS)
        {
            /* Write the first catalog entry Unique ID */
            NewData = 1;
            ErrorCode = RegSetValueEx(CatalogKey,
                                      "Serial_Access_Num",
                                      0,
                                      REG_DWORD,
                                      (LPBYTE)&NewData,
                                      sizeof(NewData));
        }
    }

    /* Set the Catalog Key */
    Catalog->CatalogKey = CatalogKey;
    return TRUE;
}

INT
WSAAPI
WsNcInitializeFromRegistry(IN PNSCATALOG Catalog,
                           IN HKEY ParentKey,
                           IN HANDLE CatalogEvent)
{
    INT ErrorCode = WSASYSCALLFAILURE;

    /* Open the catalog */
    if (WsNcOpen(Catalog, ParentKey))
    {
        /* Refresh it */
        ErrorCode = WsNcRefreshFromRegistry(Catalog, CatalogEvent);
    }

    /* Return the status */
    return ErrorCode;
}

INT
WSAAPI
WsNcRefreshFromRegistry(IN PNSCATALOG Catalog,
                        IN HANDLE CatalogEvent)
{
    INT ErrorCode;
    BOOLEAN LocalEvent = FALSE;
    LIST_ENTRY LocalList;
    DWORD UniqueId;
    HKEY EntriesKey;
    DWORD CatalogEntries;
    PNSCATALOG_ENTRY CatalogEntry;
    BOOL NewChangesMade;
    PLIST_ENTRY Entry;
    DWORD RegType = REG_DWORD;
    DWORD RegSize = sizeof(DWORD);
    DWORD i;

    /* Check if we got an event */
    if (!CatalogEvent)
    {
        /* Create an event ourselves */
        CatalogEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!CatalogEvent) return WSASYSCALLFAILURE;
        LocalEvent = TRUE;
    }

    /* Lock the catalog */
    WsNcLock();

    /* Initialize our local list for the loop */
    InitializeListHead(&LocalList);

    /* Start looping */
    do
    {
        /* Setup notifications for the catalog entry */
        ErrorCode = WsSetupCatalogProtection(Catalog->CatalogKey,
                                             CatalogEvent,
                                             &UniqueId);
        if (ErrorCode != ERROR_SUCCESS) break;

        /* Check if we've changed till now */
        if (UniqueId == Catalog->UniqueId)
        {
            /* We haven't, so return */
            ErrorCode = ERROR_SUCCESS;
            break;
        }

        /* Now Open the Entries */
        ErrorCode = RegOpenKeyEx(Catalog->CatalogKey,
                                 "Catalog_Entries",
                                 0,
                                 MAXIMUM_ALLOWED,
                                 &EntriesKey);
        if (ErrorCode != ERROR_SUCCESS)
        {
            /* Critical failure */
            ErrorCode = WSASYSCALLFAILURE;
            break;
        }

        /* Find out how many there are */
        ErrorCode = RegQueryValueEx(Catalog->CatalogKey,
                                    "Num_Catalog_Entries",
                                    0,
                                    &RegType,
                                    (LPBYTE)&CatalogEntries,
                                    &RegSize);
        if (ErrorCode != ERROR_SUCCESS)
        {
            /* Critical failure */
            ErrorCode = WSASYSCALLFAILURE;
            break;
        }

        /* Initialize them all */
        for (i = 1; i <= CatalogEntries; i++) 
        {
            /* Allocate a Catalog Entry Structure */
            CatalogEntry = WsNcEntryAllocate();
            if (!CatalogEntry)
            {
                /* Not enough memory, fail */
                ErrorCode = WSA_NOT_ENOUGH_MEMORY;
                break;
            }

            /* Initialize it from the Registry Key */
            ErrorCode = WsNcEntryInitializeFromRegistry(CatalogEntry,
                                                        EntriesKey,
                                                        i);
            if (ErrorCode != ERROR_SUCCESS)
            {
                /* We failed to get it, dereference the entry and leave */
                WsNcEntryDereference(CatalogEntry);
                break;
            }

            /* Insert it to our List */
            InsertTailList(&LocalList, &CatalogEntry->CatalogLink);
        }

        /* Close the catalog key */
        RegCloseKey(EntriesKey);

        /* Check if we changed during our read and if we have success */
        NewChangesMade = WsCheckCatalogState(CatalogEvent);
        if (!NewChangesMade && ErrorCode == ERROR_SUCCESS)
        {
            /* All is good, update the protocol list */
            WsNcUpdateNamespaceList(Catalog, &LocalList);

            /* Update and return */
            Catalog->UniqueId = UniqueId;
            break;
        }

        /* We failed and/or catalog data changed, free what we did till now */
        while (!IsListEmpty(&LocalList))
        {
            /* Get the LP Catalog Item */
            Entry = RemoveHeadList(&LocalList);
            CatalogEntry = CONTAINING_RECORD(Entry, NSCATALOG_ENTRY, CatalogLink);

            /* Dereference it */
            WsNcEntryDereference(CatalogEntry);
        }
    } while (NewChangesMade);

    /* Release the lock */
    WsNcUnlock();

    /* Close the event, if any was created by us */
    if (LocalEvent) CloseHandle(CatalogEvent);

    /* All Done */
    return ErrorCode;
}

VOID
WSAAPI
WsNcEnumerateCatalogItems(IN PNSCATALOG Catalog,
                          IN PNSCATALOG_ENUMERATE_PROC Callback,
                          IN PVOID Context)
{
    PLIST_ENTRY Entry;
    PNSCATALOG_ENTRY CatalogEntry;
    BOOL GoOn = TRUE;

    /* Lock the catalog */
    WsNcLock();

    /* Loop the entries */
    Entry = Catalog->CatalogList.Flink;
    while (GoOn && (Entry != &Catalog->CatalogList))
    {
        /* Get the entry */
        CatalogEntry = CONTAINING_RECORD(Entry, NSCATALOG_ENTRY, CatalogLink);

        /* Move to the next one and call the callback */
        Entry = Entry->Flink;
        GoOn = Callback(Context, CatalogEntry);
    }

    /* Release lock */
    WsNcUnlock();
}

INT
WSAAPI
WsNcLoadProvider(IN PNSCATALOG Catalog,
                 IN PNSCATALOG_ENTRY CatalogEntry)
{
    INT ErrorCode = ERROR_SUCCESS;
    PNS_PROVIDER Provider;

    /* Lock the catalog */
    WsNcLock();

    /* Check if we have a provider already */
    if (!CatalogEntry->Provider)
    {
        /* Allocate a provider */
        if ((Provider = WsNpAllocate()))
        {
            /* Initialize it */
            ErrorCode = WsNpInitialize(Provider,
                                       CatalogEntry->DllPath,
                                       &CatalogEntry->ProviderId);

                /* Ensure success */
                if (ErrorCode == ERROR_SUCCESS)
                {
                    /* Set the provider */
                    WsNcEntrySetProvider(CatalogEntry, Provider);
                }

            /* Dereference it */
            WsNpDereference(Provider);
        }
        else
        {
            /* No memory */
            ErrorCode = WSA_NOT_ENOUGH_MEMORY;
        }
    }

    /* Release the lock */
    WsNcUnlock();
    return ErrorCode;
}

INT
WSAAPI
WsNcGetServiceClassInfo(IN PNSCATALOG Catalog,
                        IN OUT LPDWORD BugSize,
                        IN OUT LPWSASERVICECLASSINFOW lpServiceClassInfo)
{
    /* Not yet implemented in the spec? */
    SetLastError(ERROR_SUCCESS);
    return SOCKET_ERROR;
}

VOID
WSAAPI
WsNcUpdateNamespaceList(IN PNSCATALOG Catalog,
                        IN PLIST_ENTRY List)
{
    LIST_ENTRY TempList;
    PNSCATALOG_ENTRY CatalogEntry, OldCatalogEntry;
    PLIST_ENTRY Entry;

    /* First move from our list to the old one */
    InsertHeadList(&Catalog->CatalogList, &TempList);
    RemoveEntryList(&Catalog->CatalogList);
    InitializeListHead(&Catalog->CatalogList);

    /* Loop every item on the list */
    while (!IsListEmpty(List))
    {
        /* Get the catalog entry */
        Entry = RemoveHeadList(List);
        CatalogEntry = CONTAINING_RECORD(Entry, NSCATALOG_ENTRY, CatalogLink);

        /* Check if this item is already on our list */
        Entry = TempList.Flink;
        while (Entry != &TempList)
        {
            /* Get the catalog entry */
            OldCatalogEntry = CONTAINING_RECORD(Entry, NSCATALOG_ENTRY, CatalogLink);
            Entry = Entry->Flink;

            /* Check if they match */
            if (memcmp(&CatalogEntry->ProviderId,
                       &OldCatalogEntry->ProviderId,
                       sizeof(GUID)))
            {
                /* We have a match, use the old item instead */
                WsNcEntryDereference(CatalogEntry);
                CatalogEntry = OldCatalogEntry;
                RemoveEntryList(&CatalogEntry->CatalogLink);

                /* Decrease the number of protocols we have */
                Catalog->ItemCount--;
                break;
            }
        }

        /* Add this item */
        InsertTailList(&Catalog->CatalogList, &CatalogEntry->CatalogLink);
        Catalog->ItemCount++;
    }

    /* If there's anything left on the temporary list */
    while (!IsListEmpty(&TempList))
    {
        /* Get the entry */
        Entry = RemoveHeadList(&TempList);
        CatalogEntry = CONTAINING_RECORD(Entry, NSCATALOG_ENTRY, CatalogLink);

        /* Remove it */
        Catalog->ItemCount--;
        WsNcEntryDereference(CatalogEntry);
    }
}

INT
WSAAPI
WsNcGetCatalogFromProviderId(IN PNSCATALOG Catalog,
                             IN LPGUID ProviderId,
                             OUT PNSCATALOG_ENTRY *CatalogEntry)
{
    PLIST_ENTRY NextEntry = Catalog->CatalogList.Flink;
    PNSCATALOG_ENTRY Entry;

    /* Lock the catalog */
    WsNcLock();

    /* Match the Id with all the entries in the List */
    while (NextEntry != &Catalog->CatalogList)
    {
        /* Get the Current Entry */
        Entry = CONTAINING_RECORD(NextEntry, NSCATALOG_ENTRY, CatalogLink);
        NextEntry = NextEntry->Flink;

        /* Check if this is the Catalog Entry ID we want */
        if (!(memcmp(&Entry->ProviderId, ProviderId, sizeof(GUID))))
        {
            /* Check if it doesn't already have a provider */
            if (!Entry->Provider)
            {
                /* Match, load the Provider */
                WsNcLoadProvider(Catalog, Entry);
            }

            /* Reference the entry and return it */
            InterlockedIncrement(&Entry->RefCount);
            *CatalogEntry = Entry;
            break;
        }
    }

    /* Release the catalog */
    WsNcUnlock();

    /* Return */
    return ERROR_SUCCESS;
}

BOOL
WSAAPI
WsNcMatchProtocols(IN DWORD NameSpace,
                   IN LONG AddressFamily,
                   IN LPWSAQUERYSETW QuerySet)
{
    DWORD ProtocolCount = QuerySet->dwNumberOfProtocols;
    LPAFPROTOCOLS AfpProtocols = QuerySet->lpafpProtocols;
    LONG Family;

    /* Check for valid family */
    if (AddressFamily != -1)
    {
        /* Check if it's the magic */
        if (AddressFamily == AF_UNSPEC) return TRUE;
        Family = AddressFamily;
    }
    else
    {
        /* No family given, check for namespace */
        if (NameSpace == NS_SAP)
        {
            /* Use IPX family */
            Family = AF_IPX;
        }
        else
        {
            /* Other namespace, it's valid */
            return TRUE;
        }
    }

    /* Now try to get a match */
    while (ProtocolCount--)
    {
        /* Check this protocol entry */
        if ((AfpProtocols->iAddressFamily == AF_UNSPEC) ||
            (AfpProtocols->iAddressFamily == Family))
        {
            /* Match found */
            return TRUE;
        }

        /* Move to the next one */
        AfpProtocols++;
    }

    /* No match */
    return FALSE;
}

VOID
WSAAPI
WsNcRemoveCatalogItem(IN PNSCATALOG Catalog,
                      IN PNSCATALOG_ENTRY Entry)
{
    /* Remove the entry from the list */
    RemoveEntryList(&Entry->CatalogLink);

    /* Decrease our count */
    Catalog->ItemCount--;
}

VOID
WSAAPI
WsNcDelete(IN PNSCATALOG Catalog)
{
    PLIST_ENTRY Entry;
    PNSCATALOG_ENTRY CatalogEntry;

    /* Check if we're initialized */
    if (!Catalog->CatalogList.Flink) return;

    /* Acquire lock */
    WsNcLock();

    /* Loop every entry */
    Entry = Catalog->CatalogList.Flink;
    while (Entry != &Catalog->CatalogList)
    {
        /* Get this entry */
        CatalogEntry = CONTAINING_RECORD(Entry, NSCATALOG_ENTRY, CatalogLink);

        /* Remove it */
        WsNcRemoveCatalogItem(Catalog, CatalogEntry);

        /* Dereference it */
        WsNcEntryDereference(CatalogEntry);

        /* Move to the next entry */
        Entry = Catalog->CatalogList.Flink;
    }

    /* Check if the catalog key is opened */
    if (Catalog->CatalogKey)
    {
        /* Close it */
        RegCloseKey(Catalog->CatalogKey);
        Catalog->CatalogKey = NULL;
    }

    /* Release and delete the lock */
    WsNcUnlock();
    DeleteCriticalSection((LPCRITICAL_SECTION)&Catalog->Lock);

    /* Delete the object */
    HeapFree(WsSockHeap, 0, Catalog);
}
