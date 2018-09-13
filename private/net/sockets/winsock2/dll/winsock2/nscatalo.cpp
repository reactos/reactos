/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    nscatalo.cpp

Abstract:

    This module contains the implementation of the dcatalog class.

Author:

    Dirk Brandewie dirk@mink.intel.com  25-JUL-1995

Revision History:

    23-Aug-1995 dirk@mink.intel.com
        Moved includes into precomp.h.

--*/

#include "precomp.h"

#define CATALOG_NAME            "NameSpace_Catalog5"
#define CATALOG_ENTRIES_NAME    "Catalog_Entries"
#define NUM_ENTRIES_NAME        "Num_Catalog_Entries"

#define FIRST_SERIAL_NUMBER 1
    // The first access serial number to be assigned on a given system.


NSCATALOG::NSCATALOG()
/*++

Routine Description:

    Constructor for the NSCATALOG object

Arguments:

    NONE

Return Value:

    NONE

--*/
{
    // Initialize members
    m_num_items = 0;
    m_reg_key = NULL;
    m_serial_num = FIRST_SERIAL_NUMBER-1;
    m_namespace_list.Flink = NULL;
    m_classinfo_provider = NULL;
}




BOOL
NSCATALOG::OpenCatalog(
    IN  HKEY   ParentKey
    )
/*++

Routine Description:

    This  procedure  opens the catalog portion of the registry.  If the catalog
    is  not  yet  present,  it  also  initializes  new  first-level  values and
    first-level  subkeys  for  the  catalog.  It is assumed that the catalog is
    locked against competing registry I/O attempts.

Arguments:

    ParentKey  - Supplies  the open registry key representing the parent key of
                 the catalog.


Return Value:

    The function returns TRUE if successful, otherwise it returns FALSE.

--*/
{
    LONG   lresult;
    HKEY   new_key;
    DWORD  key_disposition;

    assert(ParentKey != NULL);

    __try {
        InitializeCriticalSection(&m_nscatalog_lock);
    }
    __except (WS2_EXCEPTION_FILTER ()) {
        return FALSE;
    }
    InitializeListHead(&m_namespace_list);

    //
    // We must first try to open the key before trying to create it.
    // RegCreateKeyEx() will fail with ERROR_ACCESS_DENIED if the current
    // user has insufficient privilege to create the target registry key,
    // even if that key already exists.
    //

    lresult = RegOpenKeyEx(
        ParentKey,                              // hkey
        NSCATALOG::GetCurrentCatalogName(),     // lpszSubKey
        0,                                      // dwReserved
        MAXIMUM_ALLOWED,                        // samDesired
        & new_key                               // phkResult
        );

    if( lresult == ERROR_SUCCESS ) {
        key_disposition = REG_OPENED_EXISTING_KEY;
    } else if( lresult == ERROR_FILE_NOT_FOUND ) {
        lresult = RegCreateKeyEx(
            ParentKey,                          // hkey
            NSCATALOG::GetCurrentCatalogName(), // lpszSubKey
            0,                                  // dwReserved
            NULL,                               // lpszClass
            REG_OPTION_NON_VOLATILE,            // fdwOptions
            KEY_ALL_ACCESS,                     // samDesired
            NULL,                               // lpSecurityAttributes
            & new_key,                          // phkResult
            & key_disposition                   // lpdwDisposition
            );
    }

    if (lresult != ERROR_SUCCESS) {
        return FALSE;
    }

    TRY_START(guard_open) {
        BOOL	bresult;
        DWORD	dwData;
        if (key_disposition == REG_CREATED_NEW_KEY) {
            HKEY	entries_key;
            DWORD	dont_care;
            LONG	lresult;

            DEBUGF(
                DBG_TRACE,
                ("Creating empty ns catalog in registry\n"));


            dwData = 0;
            bresult = WriteRegistryEntry(
                new_key,           // EntryKey
                NUM_ENTRIES_NAME,  // EntryName
                (PVOID) & dwData,  // Data
                REG_DWORD          // TypeFlag
                );
            if (! bresult) {
                DEBUGF(
                    DBG_ERR,
                    ("Writing Num_Entries\n"));
                TRY_THROW(guard_open);
            }


            dwData = FIRST_SERIAL_NUMBER;
            bresult = WriteRegistryEntry(
                new_key,                  // EntryKey
                SERIAL_NUMBER_NAME,       // EntryName
                (PVOID) & dwData,         // Data
                REG_DWORD                 // TypeFlag
                );
            if (! bresult) {
                DEBUGF(
                    DBG_ERR,
                    ("Writing %s\n",
                    SERIAL_NUMBER_NAME));
                TRY_THROW(guard_open);
            }

            lresult = RegCreateKeyEx(
                new_key,                  // hkey
                CATALOG_ENTRIES_NAME,     // lpszSubKey
                0,                        // dwReserved
                NULL,                     // lpszClass
                REG_OPTION_NON_VOLATILE,  // fdwOptions
                KEY_ALL_ACCESS,           // samDesired
                NULL,                     // lpSecurityAttributes
                & entries_key,            // phkResult
                & dont_care               // lpdwDisposition
                );
            if (lresult != ERROR_SUCCESS) {
                DEBUGF(
                    DBG_ERR,
                    ("Creating entries subkey '%s'\n",
                    CATALOG_ENTRIES_NAME));
                TRY_THROW(guard_open);
            }
            lresult = RegCloseKey(
                entries_key  // hkey
                );
            if (lresult != ERROR_SUCCESS) {
                DEBUGF(
                    DBG_ERR,
                    ("Closing entries subkey\n"));
                TRY_THROW(guard_open);
            }

        }  // if REG_CREATED_NEW_KEY
        else {
            bresult = ReadRegistryEntry (
                        new_key,                // EntryKey
                        SERIAL_NUMBER_NAME,     // EntryName
                        (PVOID) &dwData,		// Data
                        sizeof (DWORD),         // MaxBytes
                        REG_DWORD               // TypeFlag
                        );
            if (!bresult) {
                // This must be the first time this version of ws2_32.dll
                // is being run.  We need to update catalog to have this
                // new entry or fail initialization.
            
			    dwData = FIRST_SERIAL_NUMBER;
                bresult = WriteRegistryEntry (
                            new_key,                // EntryKey
                            SERIAL_NUMBER_NAME,     // EntryName
                            (PVOID) &dwData,		// Data
                            REG_DWORD               // TypeFlag
                            );
                if (!bresult) {
                    DEBUGF (DBG_ERR,
                        ("Writing '%s' value.\n", SERIAL_NUMBER_NAME));
				    TRY_THROW (guard_open);
			    }
            }
        } // else

		m_reg_key = new_key;
		return TRUE;

    } TRY_CATCH(guard_open) {
        LONG close_result;

        close_result = RegCloseKey(
            new_key  // hkey
            );
        if (close_result != ERROR_SUCCESS) {
            DEBUGF(
                DBG_ERR,
                ("Closing catalog key\n"));
        }

        return FALSE;
    } TRY_END(guard_open);

}  // OpenCatalog




INT
NSCATALOG::InitializeFromRegistry(
    IN  HKEY    ParentKey,
    IN  HANDLE  ChangeEvent OPTIONAL
    )
/*++

Routine Description:

    This  procedure takes care of initializing a newly-created name space catalog
    from  the  registry.  If the registry does not currently contain a name space
    catalog,  an  empty catalog is created and the registry is initialized with
    the new empty catalog.

Arguments:

    ParentKey - Supplies  an  open registry key under which the catalog is read
                or  created  as  a  subkey.   The  key may be closed after this
                procedure returns.

Return Value:

    The  function  returns  ERROR_SUCESS if successful, otherwise it returns an
    appropriate WinSock error code.

Implementation Notes:

    lock the catalog
    open catalog, creating empty if required
    read the catalog
    unlock the catalog
--*/
{
    INT return_value;
    BOOL bresult;


    assert(ParentKey != NULL);
    assert(m_reg_key==NULL);

    bresult = OpenCatalog(
        ParentKey
        );
    // Opening  the catalog has the side-effect of creating an empty catalog if
    // needed.
    if (bresult) {
        return_value =  RefreshFromRegistry (ChangeEvent);
    }
    else {
        DEBUGF(
            DBG_ERR,
            ("Unable to create or open ns catalog\n"));
        return_value = WSASYSCALLFAILURE;
    }
    return return_value;

}  // InitializeFromRegistry




INT
NSCATALOG::RefreshFromRegistry(
    IN  HANDLE  ChangeEvent OPTIONAL
    )
/*++

Routine Description:

    This  procedure takes care of initializing a newly-created name space catalog
    from  the  registry.  If the registry does not currently contain a protocol
    catalog,  an  empty catalog is created and the registry is initialized with
    the new empty catalog.

Arguments:

    ParentKey - Supplies  an  open registry key under which the catalog is read
                or  created  as  a  subkey.   The  key may be closed after this
                procedure returns.

Return Value:

    The  function  returns  ERROR_SUCESS if successful, otherwise it returns an
    appropriate WinSock error code.

Implementation Notes:

    lock the catalog
    do
		establish event notification for any registry catalog modifications
		RegOpenKey(... entries, entries_key)
		ReadRegistryEntry(... next_id)
		ReadRegistryEntry(... num_items)
		for i in (1 .. num_items)
			item = new catalog item
			item->InitializeFromRegistry(entries_key, i)
			add item to temp list
		end for
		RegCloseKey(... entries_key)
    while registry catalog has changed during read.
    update the catalog
    unlock the catalog

--*/
{
    INT			return_value;
    BOOLEAN		created_event = FALSE;
    DWORD       serial_num;
	LONG        lresult;
	HKEY        entries_key;
	LIST_ENTRY  temp_list;
	PNSCATALOGENTRY  item;
	DWORD       num_entries, next_id;
    BOOL        catalog_changed;

    //
    // Create the event if caller did not provide one
    //
    if (ChangeEvent==NULL) {
        ChangeEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
        if (ChangeEvent==NULL) {
            return WSASYSCALLFAILURE;
        }
        created_event = TRUE;
    }

    // Lock this catalog object
    AcquireCatalogLock ();

    assert(m_reg_key != NULL);

	// Initialize locals to known defaults
	item = NULL;
	InitializeListHead (&temp_list);

    do {
        // Synchronize with writers
        return_value = SynchronizeSharedCatalogAccess (
								m_reg_key,
								ChangeEvent,
								&serial_num);
        if (return_value != ERROR_SUCCESS) {
            // Non-recoverable;
            break;
        }

        // Check if catalog has changed.
        if (m_serial_num == serial_num) {
            return_value = ERROR_SUCCESS;
            break;
        }

		// Open entry key
		lresult = RegOpenKeyEx(
			m_reg_key,             // hkey
			CATALOG_ENTRIES_NAME,  // lpszSubKey
			0,                     // dwReserved
			MAXIMUM_ALLOWED,       // samDesired
			& entries_key          // phkResult
			);
		if (lresult != ERROR_SUCCESS) {
            // Non-recoverable
			DEBUGF(
				DBG_ERR,
				("Opening entries key of registry\n"));
			return_value = WSASYSCALLFAILURE;
            break;
        }

		TRY_START(guard_open) {
			BOOL                 bresult;
			DWORD                seq_num;

			// read number of items in the catalog
			bresult = ReadRegistryEntry(
				m_reg_key,              // EntryKey
				NUM_ENTRIES_NAME,       // EntryName
				(PVOID) & num_entries,  // Data
				sizeof(DWORD),          // MaxBytes
				REG_DWORD               // TypeFlag
				);
			if (! bresult) {
				DEBUGF(
					DBG_ERR,
					("Reading %s from registry\n",
					NUM_ENTRIES_NAME));
                return_value = WSASYSCALLFAILURE;
				TRY_THROW(guard_open);
			}

			// read the items and place on temp list
            InitializeListHead (&temp_list);
			for (seq_num = 1; seq_num <= num_entries; seq_num++) {
				item = new NSCATALOGENTRY();
				if (item == NULL) {
					return_value = WSA_NOT_ENOUGH_MEMORY;
					DEBUGF(
						DBG_ERR,
						("Allocating new proto catalog item\n"));
					TRY_THROW(guard_open);
				}
				return_value = item->InitializeFromRegistry(
					entries_key,  // ParentKey
					(INT)seq_num  // SequenceNum
					);
				if (return_value != ERROR_SUCCESS) {
					item->Dereference ();
					DEBUGF(
						DBG_ERR,
						("Initializing new proto catalog item\n"));
					TRY_THROW(guard_open);
				}
				InsertTailList (&temp_list, &item->m_CatalogLinkage);
			}  // for seq_num

		} TRY_CATCH(guard_open) {

		    assert (return_value!=ERROR_SUCCESS);

		} TRY_END(guard_open);

		// close catalog
		lresult = RegCloseKey(
			entries_key  // hkey
			);
		if (lresult != ERROR_SUCCESS) {
			DEBUGF(
				DBG_ERR,
				("Closing entries key of registry\n"));
			// non-fatal
		}

        //
        // Check if catalog has changed while we were reading it
        // If so, we'll have to retry even though we succeeded
        // in reading it to ensure consistent view of the whole
        // catalog.
        //

        catalog_changed = HasCatalogChanged (ChangeEvent);
        
        if ((return_value==ERROR_SUCCESS) && !catalog_changed) {
            UpdateNamespaceList (&temp_list);

            // Store new catalog parameters
            assert (m_num_items == num_entries);
            m_serial_num = serial_num;
            break;
        }

        //
        // Free the entries we might have read
        //

        while (!IsListEmpty (&temp_list)) {
            PLIST_ENTRY list_member;
			list_member = RemoveHeadList (&temp_list);
			item = CONTAINING_RECORD (list_member,
										NSCATALOGENTRY,
										m_CatalogLinkage);
#if defined(DEBUG_TRACING)
            InitializeListHead (&item->m_CatalogLinkage);
#endif
			item->Dereference ();
		}
    }
    while (catalog_changed); // Retry while catalog is being written over

    //
    // We should have freed or consumed all the items we
    // might have read.
    //
    assert (IsListEmpty (&temp_list));
    
    ReleaseCatalogLock ();

    // Close the event if we created one.
    if (created_event)
        CloseHandle (ChangeEvent);

    return return_value;

}  // RefreshFromRegistry



VOID
NSCATALOG::UpdateNamespaceList (
    PLIST_ENTRY     new_list
    ) 
/*++

Routine Description:

    This procedure carefully updates the catalog to match the one
    just read from the registry.  It takes care of moving item
    that did not change, removing itmes that no longer exists,
    adding new items, as well as establishing new item order.

Arguments:

    new_list    - list of the items just read form the registry

Return Value:

    None.

Implementation Notes:

    move all items from current catalog to old list
	for all items in new list
		if same item exist in old list
			add old item to current catalog and destroy new one
		else
			add new item to current catalog
	end for
	dereference all remaining items in the old list

--*/
{
    LIST_ENTRY      old_list;
    PNSCATALOGENTRY item;
    PLIST_ENTRY     list_member;

    // Move items from current list to old list
	InsertHeadList (&m_namespace_list, &old_list);
	RemoveEntryList (&m_namespace_list);
	InitializeListHead (&m_namespace_list);

	// for all loaded items
	while (!IsListEmpty (new_list)) {
		list_member = RemoveHeadList (new_list);
		item = CONTAINING_RECORD (list_member,
									NSCATALOGENTRY,
									m_CatalogLinkage);

		// check if the same item is in the old list
		list_member = old_list.Flink;
		while (list_member!=&old_list) {
			PNSCATALOGENTRY     old_item;
			old_item = CONTAINING_RECORD (list_member,
									NSCATALOGENTRY,
									m_CatalogLinkage);
            list_member = list_member->Flink;
			if (*(item->GetProviderId()) == *(old_item->GetProviderId())) {
				// it is, use the old one and get rid of the new
				assert (item->GetNamespaceId () == old_item->GetNamespaceId());
#if defined(DEBUG_TRACING)
                InitializeListHead (&item->m_CatalogLinkage);
#endif
				item->Dereference ();

                item = old_item;
				RemoveEntryList (&item->m_CatalogLinkage);
                m_num_items -= 1;
				break;
			}
		}
		// add item to the current list
		InsertTailList (&m_namespace_list, &item->m_CatalogLinkage);
        m_num_items += 1;
	}

	// destroy all remaining items on the old list
	while (!IsListEmpty (&old_list)) {
		list_member = RemoveHeadList (&old_list);
		item = CONTAINING_RECORD (list_member,
									NSCATALOGENTRY,
									m_CatalogLinkage);
#if defined(DEBUG_TRACING)
        InitializeListHead (&item->m_CatalogLinkage);
#endif
        m_num_items -= 1;
		item->Dereference ();
	}

}


INT
NSCATALOG::WriteToRegistry(
    )
/*++

Routine Description:

    This procedure writes the "entries" and "numentries" portion of the catalog
    out  to  the  registry.  

Arguments:

Return Value:

    If  the  function  is  successful,  it  returns ERROR_SUCCESS, otherwise it
    returns an appropriate WinSock error code.

Implementation Notes:
	lock catalog object
	acquire registry catalog lock (exclusive)
    RegCreateKeyEx(... entries, entries_key)
    RegDeleteSubkeys (... entries_key)
    while (get item from catalog)
        num_items++;
        item->WriteToRegistry(entries_key, num_items)
    end while
    RegCloseKey(... entries_key)
    WriteRegistryEntry(... num_items)
	WriteRegistryEntry(... nex_id)
	release registry catalog
	unlock catalog object
--*/
{
    LONG lresult;
    HKEY access_key, entries_key;
    DWORD dont_care;
    INT return_value;
    BOOL bresult;

	// lock the catalog object
    AcquireCatalogLock ();
    assert (m_reg_key!=NULL);
    assert (m_serial_num!=0);

	// Get exclusive access to the registry
	// This also verifies that registry has not change since
	// it was last read
    return_value = AcquireExclusiveCatalogAccess (
							m_reg_key,
							m_serial_num,
							&access_key);
    if (return_value == ERROR_SUCCESS) {
		// Create or open existing entries key
        lresult = RegCreateKeyEx(
            m_reg_key,                // hkey
            CATALOG_ENTRIES_NAME,     // lpszSubKey
            0,                        // dwReserved
            NULL,                     // lpszClass
            REG_OPTION_NON_VOLATILE,  // fdwOptions
            KEY_ALL_ACCESS,           // samDesired
            NULL,                     // lpSecurityAttributes
            & entries_key,            // phkResult
            & dont_care               // lpdwDisposition
            );
        if (lresult == ERROR_SUCCESS) {
            TRY_START(any_failure) {
                PLIST_ENTRY          ListMember;
                PNSCATALOGENTRY     item;
                DWORD               num_items = 0;

                lresult = RegDeleteSubkeys (entries_key);

				// Write catalog items to registry
                ListMember = m_namespace_list.Flink;
                while (ListMember != & m_namespace_list) {
                    item = CONTAINING_RECORD(
                        ListMember,
                        NSCATALOGENTRY,
                        m_CatalogLinkage);
                    ListMember = ListMember->Flink;
                    num_items += 1;
                    return_value = item->WriteToRegistry(
                        entries_key,  // ParentKey
                        num_items     // SequenceNum
                        );
                    if (return_value != ERROR_SUCCESS) {
                        DEBUGF(
                            DBG_ERR,
                            ("Writing item (%lu) to registry\n",
                            num_items));
                        TRY_THROW(any_write_failure);
                    }
                }  // while get item

                assert (m_num_items == num_items);
				// Write number of items
                bresult = WriteRegistryEntry(
                    m_reg_key,             // EntryKey
                    NUM_ENTRIES_NAME,     // EntryName
                    (PVOID) & m_num_items,// Data
                    REG_DWORD             // TypeFlag
                    );
                if (! bresult) {
                    DEBUGF(
                        DBG_ERR,
                        ("Writing %s value\n",
                        NUM_ENTRIES_NAME));
                    return_value = WSASYSCALLFAILURE;
                    TRY_THROW(any_write_failure);
                }

            } TRY_CATCH(any_write_failure) {
                if (return_value == ERROR_SUCCESS) {
                    return_value = WSASYSCALLFAILURE;
                }
            } TRY_END(any_write_failure);

			// Close entries key
            lresult = RegCloseKey(
                entries_key  // hkey
                );
            if (lresult != ERROR_SUCCESS) {
                DEBUGF(
                    DBG_ERR,
                    ("Closing entries key of registry\n"));
				// Non-fatal
            }
        }

		// Release registry
        ReleaseExclusiveCatalogAccess (
							m_reg_key,
							m_serial_num, 
							access_key);
    }

	// Unlock catalog object
	ReleaseCatalogLock();
    return return_value;

}  // WriteToRegistry


NSCATALOG::~NSCATALOG()
/*++

Routine Description:

    This  function  destroys the catalog object.  It takes care of removing and
    dereferencing  all  of  the  catalog  entries  in  the catalog.  This includes
    dereferencing  all  of the NSPROVIDER objects referenced by the catalog. 

Arguments:

    None

Return Value:

    None

Implementation Notes:

    lock the catalog
    for each catalog entry
        remove the entry
        dereference the entry
    end for
    close registry key
    unlock the catalog
    delete catalog lock
--*/
{
    PLIST_ENTRY  this_linkage;
    PNSCATALOGENTRY  this_item;
    LONG        lresult;

    DEBUGF(
        DBG_TRACE,
        ("Catalog destructor\n"));

    //
    // Check if we were fully initialized.
    //
    if (m_namespace_list.Flink==NULL) {
        return;
    }
    AcquireCatalogLock();
    while ((this_linkage = m_namespace_list.Flink) != & m_namespace_list) {
        this_item = CONTAINING_RECORD(
            this_linkage,        // address
            NSCATALOGENTRY,      // type
            m_CatalogLinkage     // field
            );
        RemoveCatalogItem(
            this_item  // CatalogItem
            );
        this_item->Dereference ();
    }  // while (get entry linkage)

    if (m_reg_key!=NULL) {
        lresult = RegCloseKey (m_reg_key);
        if (lresult != ERROR_SUCCESS) {
            DEBUGF (DBG_ERR,
                ("Closing catalog registry key, err: %ld.\n", lresult));
        }
        m_reg_key = NULL;
    }
    ReleaseCatalogLock();
    DeleteCriticalSection(&m_nscatalog_lock);
}  // ~NSCATALOG




VOID
NSCATALOG::EnumerateCatalogItems(
    IN NSCATALOGITERATION  IterationProc,
    IN PVOID               PassBack
    )
/*++

Routine Description:

    This  procedure enumerates all of the NSCATALOGENTRY structures in the
    catalog  by  calling  the indicated iteration procedure once for each item.
    The called procedure can stop the iteration early by returning FALSE.

    Note  that  the DPROVIDER associated with an enumerated NSCATALOGENTRY
    may  be  NULL.   To retrieve NSCATALOGENTRY structure that has had its
    DPROVIDER      loaded      and      initialized,      you      can      use
    GetCatalogItemFromCatalogEntryId.

Arguments:

    IterationProc - Supplies   a  reference  to  the  catalog  iteration
                    procedure supplied by the client.

    PassBack  - Supplies  a  value uninterpreted by this procedure.  This value
                is  passed  unmodified to the catalog iteration procedure.  The
                client can use this value to carry context between the original
                call site and the iteration procedure.

Return Value:

    None
--*/
{
    PLIST_ENTRY         ListMember;
    PNSCATALOGENTRY CatalogEntry;
    BOOL                enumerate_more;

    assert(IterationProc != NULL);

    enumerate_more = TRUE;

    AcquireCatalogLock ();

    ListMember = m_namespace_list.Flink;

    while (enumerate_more && (ListMember != & m_namespace_list)) {
        CatalogEntry = CONTAINING_RECORD(
            ListMember,
            NSCATALOGENTRY,
            m_CatalogLinkage);
        ListMember = ListMember->Flink;
        enumerate_more = (* IterationProc) (
            PassBack,     // PassBack
            CatalogEntry  // CatalogEntry
            );
    } //while

    ReleaseCatalogLock ();
}  // EnumerateCatalogItems




INT
NSCATALOG::GetCountedCatalogItemFromNameSpaceId(
    IN  DWORD NamespaceId,
    OUT PNSCATALOGENTRY FAR * CatalogItem
    )
/*++

Routine Description:

    Chooses  a  reference  to  a  suitable  catalog  item  given a provider ID.
    structure.   Note  that  any  one  of  multiple  catalog items for the same
    provider ID may be chosen.

    The operation takes care of creating, initializing, and setting a
    NSPROVIDER object  for the retrieved catalog item if necessary.  

Arguments:

    ProviderId  - Supplies  the  identification  of a provider to search for in
                  the catalog.

    CatalogItem - Returns  the reference to the chosen catalog item, or NULL if
                  no suitable entry was found.

Return Value:

    The  function  returns  ERROR_SUCESS if successful, otherwise it returns an
    appropriate WinSock error code.
--*/
{
    PLIST_ENTRY         ListMember;
    INT                 ReturnCode;
    PNSCATALOGENTRY     CatalogEntry;
    BOOL                Found=FALSE;

    assert(CatalogItem != NULL);

    // Prepare for early error return
    * CatalogItem = NULL;
    ReturnCode = WSAEINVAL;

    AcquireCatalogLock();
    ListMember = m_namespace_list.Flink;

    while (ListMember != & m_namespace_list) {
        CatalogEntry = CONTAINING_RECORD(
            ListMember,
            NSCATALOGENTRY,
            m_CatalogLinkage);
        ListMember = ListMember->Flink;
        if (CatalogEntry->GetNamespaceId() == NamespaceId) {
            if (CatalogEntry->GetProvider() == NULL) {
                ReturnCode = LoadProvider(
                    CatalogEntry    // CatalogEntry
                    );
                if (ReturnCode != ERROR_SUCCESS) {
                    break;
                }
            }  // if provider is NULL
            CatalogEntry->Reference ();
            * CatalogItem = CatalogEntry;
            ReturnCode = ERROR_SUCCESS;
            break;
        } //if
    } //while

    ReleaseCatalogLock();
    return(ReturnCode);
}



INT
NSCATALOG::GetCountedCatalogItemFromProviderId(
    IN  LPGUID                ProviderId,
    OUT PNSCATALOGENTRY FAR * CatalogItem
    )
/*++

Routine Description:

    Chooses  a  reference  to  a  suitable  catalog  item  given a provider ID.
    structure.

Arguments:

    ProviderId  - Supplies  the  identification  of a provider to search for in
                  the catalog.

    CatalogItem - Returns  the reference to the chosen catalog item, or NULL if
                  no suitable entry was found.

Return Value:

    The  function  returns  ERROR_SUCESS if successful, otherwise it returns an
    appropriate WinSock error code.
--*/
{
    PLIST_ENTRY         ListMember;
    INT                 ReturnCode;
    PNSCATALOGENTRY      CatalogEntry;
    LPGUID              ThisProviderId;

    assert(CatalogItem != NULL);

    // Prepare for early error return
    *CatalogItem = NULL;
    ReturnCode = WSAEINVAL;

    AcquireCatalogLock();

    ListMember = m_namespace_list.Flink;

    while (ListMember != & m_namespace_list) {
        CatalogEntry = CONTAINING_RECORD(
            ListMember,
            NSCATALOGENTRY,
            m_CatalogLinkage);
        ListMember = ListMember->Flink;

        if ( *(CatalogEntry->GetProviderId()) == *ProviderId) {
            if (CatalogEntry->GetProvider() == NULL) {
                ReturnCode = LoadProvider(
                    CatalogEntry    // CatalogEntry
                    );
                if (ReturnCode != ERROR_SUCCESS) {
                    break;
                }
            }  // if provider is NULL
            CatalogEntry->Reference ();
            * CatalogItem = CatalogEntry;
            ReturnCode = ERROR_SUCCESS;
            break;
        } //if
    } //while

    ReleaseCatalogLock();

    return(ReturnCode);
}


VOID
NSCATALOG::AppendCatalogItem(
    IN  PNSCATALOGENTRY  CatalogItem
    )
/*++

Routine Description:

    This procedure appends a catalog item to the end of the (in-memory) catalog
    object.   It becomes the last item in the catalog.  The catalog information
    in the registry is NOT updated.

 Arguments:

    CatalogItem - Supplies a reference to the catalog item to be added.

Return Value:

    None
--*/
{
    assert(CatalogItem != NULL);
    assert(IsListEmpty (&CatalogItem->m_CatalogLinkage));
    InsertTailList(
        & m_namespace_list,               // ListHead
        & CatalogItem->m_CatalogLinkage  // Entry
        );
    m_num_items ++;
}  // AppendCatalogItem




VOID
NSCATALOG::RemoveCatalogItem(
    IN  PNSCATALOGENTRY  CatalogItem
    )
/*++

Routine Description:

    This  procedure removes a catalog item from the (in-memory) catalog object.
    The catalog information in the registry is NOT updated.

Arguments:

    CatalogItem - Supplies a reference to the catalog item to be removed.

Return Value:

    None
--*/
{
    assert(CatalogItem != NULL);
    assert (!IsListEmpty (&CatalogItem->m_CatalogLinkage));

    RemoveEntryList(
        & CatalogItem->m_CatalogLinkage  // Entry
        );
#if defined(DEBUG_TRACING)
    InitializeListHead (&CatalogItem->m_CatalogLinkage);
#endif
    assert(m_num_items > 0);
    m_num_items--;

}  // RemoveCatalogItem




INT WSAAPI
NSCATALOG::GetServiceClassInfo(
    IN OUT  LPDWORD                 lpdwBufSize,
    IN OUT  LPWSASERVICECLASSINFOW  lpServiceClassInfo
    )
/*++

Routine Description:

    Returns the service class info for the service class specified in
    lpServiceClassInfo from the current service clas info enabled namespace
    provider.

Arguments:

    lpdwBufSize - A pointer to the size of the buffer pointed to by
                  lpServiceClassInfo.

    lpServiceClassInfo - A pointer to a service class info struct

Return Value:

    ERROR_SUCCESS on success, Otherwise SOCKET_ERROR.  If the buffer passed in
    is to small to hold the service class info struct, *lpdwBufSize is updated
    to reflect the required buffer size to hole the class info and an error
    value of WSAEINVAL is set with SetLastError().

--*/
{
    SetLastError(ERROR_SUCCESS);
    return(SOCKET_ERROR);
    // This stubbed out until the model for how we find the service class
    // infomation is to be found.
    UNREFERENCED_PARAMETER(lpdwBufSize);
    UNREFERENCED_PARAMETER(lpServiceClassInfo);

#if 0
    INT ReturnCode;
    BOOL ValidAnswer = FALSE;
    DWORD BufSize;
    PNSPROVIDER Provider;


    // Save off the buffer size incase we need it later
    BufSize = *lpdwBufSize;

    if (!m_classinfo_provider){
        m_classinfo_provider = GetClassInfoProvider(
            BufSize,
            lpServiceClassInfo);
        if (!m_classinfo_provider){
            SetLastError(WSAEFAULT);
            return(SOCKET_ERROR);
        } //if
    } //if
    // Call the current classinfo provider.
    ReturnCode = m_classinfo_provider->NSPGetServiceClassInfo(
        lpdwBufSize,
        lpServiceClassInfo
        );

    if (ERROR_SUCCESS == ReturnCode){
        ValidAnswer = TRUE;
    } //if

    if (!ValidAnswer){
        // The name space provider we where using failed to find the class info
        // go find a provider that can answer the question
        ReturnCode = SOCKET_ERROR;
        Provider = GetClassInfoProvider(
            BufSize,
            lpServiceClassInfo);
        if (Provider){
            //We found a provider that can service the request so use this
            //provider until it fails
            m_classinfo_provider = Provider;

            // Now retry the call
             ReturnCode = m_classinfo_provider->NSPGetServiceClassInfo(
                 lpdwBufSize,
                 lpServiceClassInfo
                 );
        } //if
    } //if
    return(ReturnCode);
#endif
}

PNSPROVIDER
NSCATALOG::GetClassInfoProvider(
    IN  DWORD BufSize,
    IN  LPWSASERVICECLASSINFOW  lpServiceClassInfo
    )
/*++

Routine Description:

    Searches for a name space provider to satisfy get service class info
    request

Arguments:

    lpdwBufSize - A pointer to the size of the buffer pointed to by
                  lpServiceClassInfo.

    lpServiceClassInfo - A pointer to a service class info struct

Return Value:

    A pointer to a provider that can satisfy the query or NULL

--*/
{
    UNREFERENCED_PARAMETER(BufSize);
    UNREFERENCED_PARAMETER(lpServiceClassInfo);

    return(NULL);

#if 0
    PLIST_ENTRY ListEntry;
    PNSPROVIDER Provider=NULL;
    PNSCATALOGENTRY CatalogEntry;
    INT ReturnCode;


    ListEntry = m_namespace_list.Flink;

    while (ListEntry != &m_namespace_list){
        CatalogEntry = CONTAINING_RECORD(ListEntry,
                                         NSCATALOGENTRY,
                                         m_CatalogLinkage);
        Provider = CatalogEntry->GetProvider();
        if (Provider &&
            CatalogEntry->GetEnabledState() &&
            CatalogEntry->StoresServiceClassInfo()){
            ReturnCode = Provider->NSPGetServiceClassInfo(
                &BufSize,
                lpServiceClassInfo
                 );
            if (ERROR_SUCCESS == ReturnCode){
                break;
            } //if
        } //if
        Provider = NULL;
        ListEntry = ListEntry->Flink;
    } //while
    return(Provider);
#endif //0
}

INT
NSCATALOG::LoadProvider(
    IN PNSCATALOGENTRY CatalogEntry
    )
/*++

Routine Description:

    Load   the   provider  described  by  CatalogEntry and set it into
    catalog entry

Arguments:

    CatalogEntry - Supplies  a reference to a name sapce catalog entry describing
                   the provider to load.


Return Value:

    The  function  returns ERROR_SUCCESS if successful, otherwise it returns an
    appropriate WinSock error code.
--*/
{
    INT ReturnCode = ERROR_SUCCESS;
    PNSPROVIDER LocalProvider;

    // Serialize provider loading/unloading
    AcquireCatalogLock ();

    // Check if provider is loaded under the lock
    if (CatalogEntry->GetProvider ()==NULL) {

        LocalProvider = new NSPROVIDER;
        if (LocalProvider!=NULL){
            ReturnCode = LocalProvider->Initialize(
                CatalogEntry->GetLibraryPath (),
                CatalogEntry->GetProviderId ()
                );
            if (ERROR_SUCCESS == ReturnCode){
                CatalogEntry->SetProvider (LocalProvider);
            }
            LocalProvider->Dereference ();

        } //if
        else {
            DEBUGF(
                DBG_ERR,
                ("Couldn't allocate a NSPROVIDER object\n"));
            ReturnCode = WSA_NOT_ENOUGH_MEMORY;
        }
    } // if provider not loaded

    // Serialize provider loading/unloading
    ReleaseCatalogLock ();
    return ReturnCode;
}


LPSTR
NSCATALOG::GetCurrentCatalogName()
{
    return CATALOG_NAME;

} // GetCurrentCatalogName

