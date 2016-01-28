/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/dcatalog.c
 * PURPOSE:     Transport Catalog Object
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

/* DATA **********************************************************************/

#define TCCATALOG_NAME "Protocol_Catalog9"

#define WsTcLock()          EnterCriticalSection((LPCRITICAL_SECTION)&Catalog->Lock);
#define WsTcUnlock()        LeaveCriticalSection((LPCRITICAL_SECTION)&Catalog->Lock);

/* FUNCTIONS *****************************************************************/

PTCATALOG
WSAAPI
WsTcAllocate(VOID)
{
    PTCATALOG Catalog;
    
    /* Allocate the object */
    Catalog = HeapAlloc(WsSockHeap, HEAP_ZERO_MEMORY, sizeof(*Catalog));

    /* Return it */
    return Catalog;
}

BOOL
WSAAPI
WsTcOpen(IN PTCATALOG Catalog,
         IN HKEY ParentKey)
{
    INT ErrorCode;
    DWORD CreateDisposition;
    HKEY CatalogKey, NewKey;
    //DWORD CatalogEntries = 0;
    DWORD RegType = REG_DWORD;
    DWORD RegSize = sizeof(DWORD);
    DWORD UniqueId = 0;
    DWORD NewData = 0;

    /* Initialize the catalog lock and namespace list */
    InitializeCriticalSection((LPCRITICAL_SECTION)&Catalog->Lock);
    InitializeListHead(&Catalog->ProtocolList);

    /* Open the Catalog Key */
    ErrorCode = RegOpenKeyEx(ParentKey,
                             TCCATALOG_NAME,
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
                                   TCCATALOG_NAME,
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

        /* Write the first catalog entry ID */
        NewData = 1001;
        ErrorCode = RegSetValueEx(CatalogKey,
                                  "Next_Catalog_Entry_ID",
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

DWORD
WSAAPI
WsTcInitializeFromRegistry(IN PTCATALOG Catalog,
                           IN HKEY ParentKey,
                           IN HANDLE CatalogEvent)
{
    INT ErrorCode = WSASYSCALLFAILURE;

    /* Open the catalog */
    if (WsTcOpen(Catalog, ParentKey))
    {
        /* Refresh it */
        ErrorCode = WsTcRefreshFromRegistry(Catalog, CatalogEvent);
    }

    /* Return the status */
    return ErrorCode;
}

DWORD
WSAAPI
WsTcRefreshFromRegistry(IN PTCATALOG Catalog,
                        IN HANDLE CatalogEvent)
{
    INT ErrorCode;
    BOOLEAN LocalEvent = FALSE;
    LIST_ENTRY LocalList;
    DWORD UniqueId;
    HKEY EntriesKey;
    DWORD CatalogEntries;
    PTCATALOG_ENTRY CatalogEntry;
    DWORD NextCatalogEntry;
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
    WsTcLock();

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
        
        /* Get the next entry */
        ErrorCode = RegQueryValueEx(Catalog->CatalogKey,
                                    "Next_Catalog_Entry_ID",
                                    0,
                                    &RegType,
                                    (LPBYTE)&NextCatalogEntry,
                                    &RegSize);
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
            CatalogEntry = WsTcEntryAllocate();
            if (!CatalogEntry)
            {
                /* Not enough memory, fail */
                ErrorCode = WSA_NOT_ENOUGH_MEMORY;
                break;
            }

            /* Initialize it from the Registry Key */
            ErrorCode = WsTcEntryInitializeFromRegistry(CatalogEntry,
                                                        EntriesKey,
                                                        i);
            if (ErrorCode != ERROR_SUCCESS)
            {
                /* We failed to get it, dereference the entry and leave */
                WsTcEntryDereference(CatalogEntry);
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
            WsTcUpdateProtocolList(Catalog, &LocalList);

            /* Update and return */
            Catalog->UniqueId = UniqueId;
            Catalog->NextId = NextCatalogEntry;
            break;
        }

        /* We failed and/or catalog data changed, free what we did till now */
        while (!IsListEmpty(&LocalList))
        {
            /* Get the LP Catalog Item */
            Entry = RemoveHeadList(&LocalList);
            CatalogEntry = CONTAINING_RECORD(Entry, TCATALOG_ENTRY, CatalogLink);

            /* Dereference it */
            WsTcEntryDereference(CatalogEntry);
        }
    } while (NewChangesMade);

    /* Release the lock */
    WsTcUnlock();

    /* Close the event, if any was created by us */
    if (LocalEvent) CloseHandle(CatalogEvent);

    /* All Done */
    return ErrorCode;
}

DWORD
WSAAPI
WsTcGetEntryFromAf(IN PTCATALOG Catalog,
                   IN INT AddressFamily,
                   IN PTCATALOG_ENTRY *CatalogEntry)
{
    INT ErrorCode = WSAEINVAL;
    PLIST_ENTRY NextEntry = Catalog->ProtocolList.Flink;
    PTCATALOG_ENTRY Entry;

    /* Assume failure */
    *CatalogEntry = NULL;

    /* Lock the catalog */
    WsTcLock();

    /* Match the Id with all the entries in the List */
    while (NextEntry != &Catalog->ProtocolList)
    {
        /* Get the Current Entry */
        Entry = CONTAINING_RECORD(NextEntry, TCATALOG_ENTRY, CatalogLink);
        NextEntry = NextEntry->Flink;

        /* Check if this is the Catalog Entry ID we want */
        if (Entry->ProtocolInfo.iAddressFamily == AddressFamily)
        {
            /* Check if it doesn't already have a provider */
            if (!Entry->Provider)
            {
                /* Match, load the Provider */
                ErrorCode = WsTcLoadProvider(Catalog, Entry);

                /* Make sure this didn't fail */
                if (ErrorCode != ERROR_SUCCESS) break;
            }

            /* Reference the entry and return it */
            InterlockedIncrement(&Entry->RefCount);
            *CatalogEntry = Entry;
            ErrorCode = ERROR_SUCCESS;
            break;
        }
    }

    /* Release the catalog */
    WsTcUnlock();

    /* Return */
    return ErrorCode;
}

DWORD
WSAAPI
WsTcGetEntryFromCatalogEntryId(IN PTCATALOG Catalog,
                               IN DWORD CatalogEntryId,
                               IN PTCATALOG_ENTRY *CatalogEntry)
{
    PLIST_ENTRY NextEntry = Catalog->ProtocolList.Flink;
    PTCATALOG_ENTRY Entry;

    /* Lock the catalog */
    WsTcLock();

    /* Match the Id with all the entries in the List */
    while (NextEntry != &Catalog->ProtocolList)
    {
        /* Get the Current Entry */
        Entry = CONTAINING_RECORD(NextEntry, TCATALOG_ENTRY, CatalogLink);
        NextEntry = NextEntry->Flink;

        /* Check if this is the Catalog Entry ID we want */
        if (Entry->ProtocolInfo.dwCatalogEntryId == CatalogEntryId)
        {
            /* Check if it doesn't already have a provider */
            if (!Entry->Provider)
            {
                /* Match, load the Provider */
                WsTcLoadProvider(Catalog, Entry);
            }

            /* Reference the entry and return it */
            InterlockedIncrement(&Entry->RefCount);
            *CatalogEntry = Entry;
            break;
        }
    }

    /* Release the catalog */
    WsTcUnlock();

    /* Return */
    return ERROR_SUCCESS;
}

DWORD
WSAAPI
WsTcGetEntryFromTriplet(IN PTCATALOG Catalog,
                        IN INT af,
                        IN INT type,
                        IN INT protocol,
                        IN DWORD StartId,
                        IN PTCATALOG_ENTRY *CatalogEntry)
{
    INT ErrorCode = WSAEINVAL;
    PLIST_ENTRY NextEntry = Catalog->ProtocolList.Flink;
    PTCATALOG_ENTRY Entry;

    /* Assume failure */
    *CatalogEntry = NULL;

    /* Lock the catalog */
    WsTcLock();

    /* Check if we are starting past 0 */
    if (StartId)
    {
        /* Loop the list */
        while (NextEntry != &Catalog->ProtocolList)
        {
            /* Get the Current Entry */
            Entry = CONTAINING_RECORD(NextEntry, TCATALOG_ENTRY, CatalogLink);
            NextEntry = NextEntry->Flink;

            /* Check if this is the ID where we are starting */
            if (Entry->ProtocolInfo.dwCatalogEntryId == StartId) break;
        }
    }

    /* Match the Id with all the entries in the List */
    while (NextEntry != &Catalog->ProtocolList)
    {
        /* Get the Current Entry */
        Entry = CONTAINING_RECORD(NextEntry, TCATALOG_ENTRY, CatalogLink);
        NextEntry = NextEntry->Flink;

        /* Check if Address Family Matches or if it's wildcard */
        if ((Entry->ProtocolInfo.iAddressFamily == af) || (af == AF_UNSPEC))
        {    
            /* Check if Socket Type Matches or if it's wildcard */
            if ((Entry->ProtocolInfo.iSocketType == type) || (type == 0))
            {
                /* Check if Protocol is In Range or if it's wildcard */
                if (((Entry->ProtocolInfo.iProtocol >= protocol) && 
                    ((Entry->ProtocolInfo.iProtocol + 
                      Entry->ProtocolInfo.iProtocolMaxOffset) <= protocol)) ||
                    (protocol == 0))
                {
                    /* Check if it doesn't already have a provider */
                    if (!Entry->Provider)
                    {
                        /* Match, load the Provider */
                        ErrorCode = WsTcLoadProvider(Catalog, Entry);

                        /* Make sure this didn't fail */
                        if (ErrorCode != ERROR_SUCCESS) break;
                    }

                    /* Reference the entry and return it */
                    InterlockedIncrement(&Entry->RefCount);
                    *CatalogEntry = Entry;
                    ErrorCode = ERROR_SUCCESS;
                    break;
                } 
                else 
                {
                    ErrorCode = WSAEPROTONOSUPPORT;
                }
            } 
            else 
            {
                ErrorCode = WSAESOCKTNOSUPPORT;
            }
        } 
        else 
        {
            ErrorCode = WSAEAFNOSUPPORT;
        }
    }

    /* Release the catalog */
    WsTcUnlock();

    /* Return */
    return ErrorCode;
}

PTPROVIDER
WSAAPI
WsTcFindProvider(IN PTCATALOG Catalog,
                 IN LPGUID ProviderId)
{
    PTPROVIDER Provider;
    PLIST_ENTRY Entry;
    PTCATALOG_ENTRY CatalogEntry;

    /* Loop the provider list */
    Entry = Catalog->ProtocolList.Flink;
    while (Entry != &Catalog->ProtocolList)
    {
        /* Get the entry */
        CatalogEntry = CONTAINING_RECORD(Entry, TCATALOG_ENTRY, CatalogLink);

        /* Move to the next one, get the provider */
        Entry = Entry->Flink;
        Provider = CatalogEntry->Provider;

        /* Check for a match */
        if ((Provider) &&
            !(memcmp(&CatalogEntry->ProtocolInfo.ProviderId,
                    ProviderId,
                    sizeof(GUID))))
        {
            /* Found a match */
            return Provider;
        }
    }

    /* No match here */
    return NULL;
}

DWORD
WSAAPI
WsTcLoadProvider(IN PTCATALOG Catalog,
                 IN PTCATALOG_ENTRY CatalogEntry)
{
    INT ErrorCode = ERROR_SUCCESS;
    PTPROVIDER Provider;

    /* Lock the catalog */
    WsTcLock();

    /* Check if we have a provider already */
    if (!CatalogEntry->Provider)
    {
        /* Try to find another instance */
        Provider = WsTcFindProvider(Catalog,
                                    &CatalogEntry->ProtocolInfo.ProviderId);

        /* Check if we found one now */
        if (Provider)
        {
            /* Set this one as the provider */
            WsTcEntrySetProvider(CatalogEntry, Provider);
            ErrorCode = ERROR_SUCCESS;
        }
        else
        {
            /* Nothing found, Allocate a provider */
            if ((Provider = WsTpAllocate()))
            {
                /* Initialize it */
                ErrorCode = WsTpInitialize(Provider,
                                           CatalogEntry->DllPath,
                                           &CatalogEntry->ProtocolInfo);

                /* Ensure success */
                if (ErrorCode == ERROR_SUCCESS)
                {
                    /* Set the provider */
                    WsTcEntrySetProvider(CatalogEntry, Provider);
                }

                /* Dereference it */
                WsTpDereference(Provider);
            }
            else
            {
                /* No memory */
                ErrorCode = WSA_NOT_ENOUGH_MEMORY;
            }
        }
    }

    /* Release the lock */
    WsTcUnlock();
    return ErrorCode;
}

VOID
WSAAPI
WsTcUpdateProtocolList(IN PTCATALOG Catalog,
                       IN PLIST_ENTRY List)
{
    LIST_ENTRY TempList;
    PTCATALOG_ENTRY CatalogEntry, OldCatalogEntry;
    PLIST_ENTRY Entry;

    /* First move from our list to the old one */
    InsertHeadList(&Catalog->ProtocolList, &TempList);
    RemoveEntryList(&Catalog->ProtocolList);
    InitializeListHead(&Catalog->ProtocolList);

    /* Loop every item on the list */
    while (!IsListEmpty(List))
    {
        /* Get the catalog entry */
        Entry = RemoveHeadList(List);
        CatalogEntry = CONTAINING_RECORD(Entry, TCATALOG_ENTRY, CatalogLink);

        /* Check if this item is already on our list */
        Entry = TempList.Flink;
        while (Entry != &TempList)
        {
            /* Get the catalog entry */
            OldCatalogEntry = CONTAINING_RECORD(Entry, TCATALOG_ENTRY, CatalogLink);
            Entry = Entry->Flink;

            /* Check if they match */
            if (CatalogEntry->ProtocolInfo.dwCatalogEntryId ==
                OldCatalogEntry->ProtocolInfo.dwCatalogEntryId)
            {
                /* We have a match, use the old item instead */
                WsTcEntryDereference(CatalogEntry);
                CatalogEntry = OldCatalogEntry;
                RemoveEntryList(&CatalogEntry->CatalogLink);

                /* Decrease the number of protocols we have */
                Catalog->ItemCount--;
                break;
            }
        }

        /* Add this item */
        InsertTailList(&Catalog->ProtocolList, &CatalogEntry->CatalogLink);
        Catalog->ItemCount++;
    }

    /* If there's anything left on the temporary list */
    while (!IsListEmpty(&TempList))
    {
        /* Get the entry */
        Entry = RemoveHeadList(&TempList);
        CatalogEntry = CONTAINING_RECORD(Entry, TCATALOG_ENTRY, CatalogLink);

        /* Remove it */
        Catalog->ItemCount--;
        WsTcEntryDereference(CatalogEntry);
    }
}

VOID
WSAAPI
WsTcEnumerateCatalogItems(IN PTCATALOG Catalog,
                          IN PTCATALOG_ENUMERATE_PROC Callback,
                          IN PVOID Context)
{
    PLIST_ENTRY Entry;
    PTCATALOG_ENTRY CatalogEntry;
    BOOL GoOn = TRUE;

    /* Lock the catalog */
    WsTcLock();

    /* Loop the entries */
    Entry = Catalog->ProtocolList.Flink;
    while (GoOn && (Entry != &Catalog->ProtocolList))
    {
        /* Get the entry */
        CatalogEntry = CONTAINING_RECORD(Entry, TCATALOG_ENTRY, CatalogLink);

        /* Move to the next one and call the callback */
        Entry = Entry->Flink;
        GoOn = Callback(Context, CatalogEntry);
    }

    /* Release lock */
    WsTcUnlock();
}

DWORD
WSAAPI
WsTcFindIfsProviderForSocket(IN PTCATALOG Catalog,
                             IN SOCKET Handle)
{
    PTPROVIDER Provider;
    IN SOCKET NewHandle;
    INT Error;
    DWORD OptionLength;
    PLIST_ENTRY Entry;
    WSAPROTOCOL_INFOW ProtocolInfo;
    DWORD UniqueId;
    INT ErrorCode;
    PTCATALOG_ENTRY CatalogEntry;

    /* Get the catalog lock */
    WsTcLock();

    /* Loop as long as the catalog changes */
CatalogChanged:

    /* Loop every provider */
    Entry = Catalog->ProtocolList.Flink;
    while (Entry != &Catalog->ProtocolList)
    {
        /* Get the catalog entry */
        CatalogEntry = CONTAINING_RECORD(Entry, TCATALOG_ENTRY, CatalogLink);

        /* Move to the next entry */
        Entry = Entry->Flink;

        /* Skip it if it doesn't support IFS */
        if (!(CatalogEntry->ProtocolInfo.dwServiceFlags1 & XP1_IFS_HANDLES)) continue;

        /* Check if we need to load it */
        if (!(Provider = CatalogEntry->Provider))
        {
            /* Load it */
            ErrorCode = WsTcLoadProvider(Catalog, CatalogEntry);
            
            /* Skip it if we failed to load it */
            if (ErrorCode != ERROR_SUCCESS) continue;

            /* Get the provider again */
            Provider = CatalogEntry->Provider;
        }

        /* Reference the entry and get our unique id */
        InterlockedIncrement(&CatalogEntry->RefCount);
        UniqueId = Catalog->UniqueId;

        /* Release the lock now */
        WsTcUnlock();

        /* Get the catalog entry ID */
        OptionLength = sizeof(ProtocolInfo);
        ErrorCode = Provider->Service.lpWSPGetSockOpt(Handle,
                                                      SOL_SOCKET,
                                                      SO_PROTOCOL_INFOW,
                                                      (PCHAR)&ProtocolInfo,
                                                      (LPINT)&OptionLength,
                                                      &Error);

        /* Dereference the entry and check the result */
        WsTcEntryDereference(CatalogEntry);
        if (ErrorCode != ERROR_SUCCESS)
        {
            /* Lock and make sure this provider is still valid */
            WsTcLock();
            if (UniqueId == Catalog->UniqueId) continue;

            /* It changed! We need to start over */
            goto CatalogChanged;
        }

        /* Now get the IFS handle */
        NewHandle = WPUModifyIFSHandle(ProtocolInfo.dwCatalogEntryId,
                                       Handle,
                                       &Error);
        
        /* Check if the socket is invalid */
        if (NewHandle == INVALID_SOCKET) return WSAENOTSOCK;

        /* We suceeded, get out of here */
        return ERROR_SUCCESS;
    }

    /* Unrecognized socket if we get here: note, we still have the lock */
    WsTcUnlock();
    return WSAENOTSOCK;
}

VOID
WSAAPI
WsTcRemoveCatalogItem(IN PTCATALOG Catalog,
                      IN PTCATALOG_ENTRY Entry)
{
    /* Remove the entry from the list */
    RemoveEntryList(&Entry->CatalogLink);

    /* Decrease our count */
    Catalog->ItemCount--;
}

VOID
WSAAPI
WsTcDelete(IN PTCATALOG Catalog)
{
    PLIST_ENTRY Entry;
    PTCATALOG_ENTRY CatalogEntry;

    /* Check if we're initialized */
    if (!Catalog->ProtocolList.Flink) return;

    /* Acquire lock */
    WsTcLock();

    /* Loop every entry */
    Entry = Catalog->ProtocolList.Flink;
    while (Entry != &Catalog->ProtocolList)
    {
        /* Get this entry */
        CatalogEntry = CONTAINING_RECORD(Entry, TCATALOG_ENTRY, CatalogLink);

        /* Remove it */
        WsTcRemoveCatalogItem(Catalog, CatalogEntry);

        /* Dereference it */
        WsTcEntryDereference(CatalogEntry);

        /* Move to the next entry */
        Entry = Catalog->ProtocolList.Flink;
    }

    /* Check if the catalog key is opened */
    if (Catalog->CatalogKey)
    {
        /* Close it */
        RegCloseKey(Catalog->CatalogKey);
        Catalog->CatalogKey = NULL;
    }

    /* Release and delete the lock */
    WsTcUnlock();
    DeleteCriticalSection((LPCRITICAL_SECTION)&Catalog->Lock);

    /* Delete the object */
    HeapFree(WsSockHeap, 0, Catalog);
}
