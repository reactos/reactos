/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    dcatalog.cpp

Abstract:

    This module contains the implementation of the dcatalog class.

Author:

    Dirk Brandewie dirk@mink.intel.com  25-JUL-1995

Revision History:

    23-Aug-1995 dirk@mink.intel.com
        Moved includes into precomp.h.

    27-Jan-1998 vadime@miscrosoft.com
        Implemented dynamic catalog

--*/

#include "precomp.h"

#define CATALOG_NAME            "Protocol_Catalog9"
#define NEXT_CATALOG_ENTRY_NAME "Next_Catalog_Entry_ID"
#define CATALOG_ENTRIES_NAME    "Catalog_Entries"
#define NUM_ENTRIES_NAME        "Num_Catalog_Entries"

#define FIRST_SERIAL_NUMBER 1
    // The first access serial number to be assigned on a given system.
#define FIRST_CATALOG_ENTRY_ID 1001
    // The first catalog entry ID to be assigned on a given system.




DCATALOG::DCATALOG()
/*++

Routine Description:

    Destructor for the DCATALOG object.

Arguments:

    NONE.

Return Value:

    NONE.

--*/
{
    // Initialize members
    m_num_items = 0;
    m_reg_key = NULL;
    m_serial_num = FIRST_SERIAL_NUMBER-1;
    m_next_id = FIRST_CATALOG_ENTRY_ID-1;
    m_protocol_list.Flink = NULL;
}




BOOL
DCATALOG::OpenCatalog(
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

    assert (m_protocol_list.Flink == NULL);
    __try {
        InitializeCriticalSection(&m_catalog_lock);
    }
    __except (WS2_EXCEPTION_FILTER ()) {
        return FALSE;
    }
    InitializeListHead (&m_protocol_list);

    //
    // We must first try to open the key before trying to create it.
    // RegCreateKeyEx() will fail with ERROR_ACCESS_DENIED if the current
    // user has insufficient privilege to create the target registry key,
    // even if that key already exists.
    //

    lresult = RegOpenKeyEx(
        ParentKey,                              // hkey
        DCATALOG::GetCurrentCatalogName(),      // lpszSubKey
        0,                                      // dwReserved
        MAXIMUM_ALLOWED,                        // samDesired
        & new_key                               // phkResult
        );

    if( lresult == ERROR_SUCCESS ) {
        key_disposition = REG_OPENED_EXISTING_KEY;
    } else if( lresult == ERROR_FILE_NOT_FOUND ) {
        lresult = RegCreateKeyEx(
            ParentKey,                          // hkey
            DCATALOG::GetCurrentCatalogName(),  // lpszSubKey
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
				("Creating empty catalog in registry.\n"));

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

            dwData = FIRST_CATALOG_ENTRY_ID;
            bresult = WriteRegistryEntry(
                new_key,                  // EntryKey
                NEXT_CATALOG_ENTRY_NAME,  // EntryName
                (PVOID) & dwData,         // Data
                REG_DWORD                 // TypeFlag
                );
            if (! bresult) {
                DEBUGF(
                    DBG_ERR,
                    ("Writing %s\n",
                    NEXT_CATALOG_ENTRY_NAME));
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
DCATALOG::InitializeFromRegistry(
    IN  HKEY    ParentKey,
    IN  HANDLE  ChangeEvent OPTIONAL
    )
/*++

Routine Description:

    This  procedure takes care of initializing a newly-created protocol catalog
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
            ("Unable to create or open catalog\n"));
        return_value = WSASYSCALLFAILURE;
    }
    return return_value;

}  // InitializeFromRegistry


INT
DCATALOG::RefreshFromRegistry(
    IN  HANDLE  ChangeEvent OPTIONAL
    )
/*++

Routine Description:

    This  procedure takes care of initializing a newly-created protocol catalog
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
	PPROTO_CATALOG_ITEM  item;
	DWORD       num_entries, next_id;
    BOOL        catalog_changed = TRUE;

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
            // Non-recoverable
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

			// Read id of next catalog entry
			bresult = ReadRegistryEntry(
				m_reg_key,              // EntryKey
				NEXT_CATALOG_ENTRY_NAME,// EntryName
				(PVOID) & next_id,      // Data
				sizeof(DWORD),          // MaxBytes
				REG_DWORD               // TypeFlag
				);
			if (! bresult) {
				DEBUGF(
					DBG_ERR,
					("Reading %s from registry\n",
					NUM_ENTRIES_NAME));
				TRY_THROW(guard_open);
                return_value = WSASYSCALLFAILURE;
			}

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

            assert (IsListEmpty (&temp_list));
			// read the items and place on temp list
			for (seq_num = 1; seq_num <= num_entries; seq_num++) {
				item = new PROTO_CATALOG_ITEM();
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
        }
        TRY_CATCH(guard_open) {
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
            UpdateProtocolList (&temp_list);
	        
            // Store new catalog parameters
	        assert (m_num_items == num_entries);
	        m_next_id = next_id;
            m_serial_num = serial_num;

            break;
        }
            
        //
        // Free the entries we might have read
        //

        while (!IsListEmpty (&temp_list)) {
        	PLIST_ENTRY     list_member;
			list_member = RemoveHeadList (&temp_list);
			item = CONTAINING_RECORD (list_member,
										PROTO_CATALOG_ITEM,
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
DCATALOG::UpdateProtocolList (
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
    LIST_ENTRY          old_list;
    PPROTO_CATALOG_ITEM item;
    PLIST_ENTRY         list_member;

	// Move items from current list to old list
	InsertHeadList (&m_protocol_list, &old_list);
	RemoveEntryList (&m_protocol_list);
	InitializeListHead (&m_protocol_list);

	// for all loaded items
	while (!IsListEmpty (new_list)) {
		list_member = RemoveHeadList (new_list);
		item = CONTAINING_RECORD (list_member,
									PROTO_CATALOG_ITEM,
									m_CatalogLinkage);

		// check if the same item is in the old list
		list_member = old_list.Flink;
		while (list_member!=&old_list) {
			PPROTO_CATALOG_ITEM old_item;
			old_item = CONTAINING_RECORD (list_member,
									PROTO_CATALOG_ITEM,
									m_CatalogLinkage);
            list_member = list_member->Flink;
			if (item->GetProtocolInfo()->dwCatalogEntryId==
				   old_item->GetProtocolInfo()->dwCatalogEntryId) {
				// it is, use the old one and get rid of the new
				assert (*(item->GetProviderId ()) == *(old_item->GetProviderId()));
#if defined(DEBUG_TRACING)
                InitializeListHead (&item->m_CatalogLinkage);
#endif
				item->Dereference ();

				item = old_item;
				RemoveEntryList (&item->m_CatalogLinkage);
#if defined(DEBUG_TRACING)
                InitializeListHead (&item->m_CatalogLinkage);
#endif
                m_num_items -= 1;
				break;
			}
		}
		// add item to the current list
		InsertTailList (&m_protocol_list, &item->m_CatalogLinkage);
        m_num_items += 1;
	}

	// destroy all remaining items on the old list
	while (!IsListEmpty (&old_list)) {
		list_member = RemoveHeadList (&old_list);
		item = CONTAINING_RECORD (list_member,
									PROTO_CATALOG_ITEM,
									m_CatalogLinkage);
#if defined(DEBUG_TRACING)
        InitializeListHead (&item->m_CatalogLinkage);
#endif
        m_num_items -= 1;
		item->Dereference ();
	}
}


INT
DCATALOG::WriteToRegistry(
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
                PPROTO_CATALOG_ITEM  item;
                DWORD                num_items = 0;

                lresult = RegDeleteSubkeys (entries_key);

				// Write catalog items to registry
                ListMember = m_protocol_list.Flink;
                while (ListMember != & m_protocol_list) {
                    item = CONTAINING_RECORD(
                        ListMember,
                        PROTO_CATALOG_ITEM,
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

				// Write next catalog id
                bresult = WriteRegistryEntry(
                    m_reg_key,               // EntryKey
                    NEXT_CATALOG_ENTRY_NAME,// EntryName
                    (PVOID) & m_next_id,    // Data
                    REG_DWORD               // TypeFlag
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



DCATALOG::~DCATALOG()
/*++

Routine Description:

    This  function  destroys the catalog object.  It takes care of removing and
    dereferecing  all  of  the  catalog  entries  in  the catalog.  This includes
    dereferencing  all  of the DPROVIDER objects referenced by the catalog.  

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
    PLIST_ENTRY this_linkage;
    PPROTO_CATALOG_ITEM  this_item;
    LONG        lresult;

    DEBUGF(
        DBG_TRACE,
        ("Catalog destructor\n"));

    //
    // Check if we were fully initialized.
    //
    if (m_protocol_list.Flink==NULL) {
        return;
    }
    AcquireCatalogLock();

    while ((this_linkage = m_protocol_list.Flink) != & m_protocol_list) {
        this_item = CONTAINING_RECORD(
            this_linkage,        // address
            PROTO_CATALOG_ITEM,  // type
            m_CatalogLinkage     // field
            );
        RemoveCatalogItem(
            this_item  // CatalogItem
            );
        this_item->Dereference ();
    }  // while (get entry linkage)

    assert( m_num_items == 0 );
    if (m_reg_key!=NULL) {
        lresult = RegCloseKey (m_reg_key);
        if (lresult != ERROR_SUCCESS) {
            DEBUGF (DBG_ERR,
                ("Closing catalog registry key, err: %ld.\n", lresult));
        }
        m_reg_key = NULL;
    }

    ReleaseCatalogLock();
    DeleteCriticalSection( &m_catalog_lock );

}  // ~DCATALOG




VOID
DCATALOG::EnumerateCatalogItems(
    IN CATALOGITERATION  Iteration,
    IN PVOID             PassBack
    )
/*++

Routine Description:

    This  procedure enumerates all of the DPROTO_CATALOG_ITEM structures in the
    catalog  by  calling  the indicated iteration procedure once for each item.
    The called procedure can stop the iteration early by returning FALSE.

    Note  that  the DPROVIDER associated with an enumerated DPROTO_CATALOG_ITEM
    may  be  NULL.   To retrieve DPROTO_CATALOG_ITEM structure that has had its
    DPROVIDER      loaded      and      initialized,      you      can      use
    GetCatalogItemFromCatalogEntryId.

Arguments:

    Iteration - Supplies   a  reference  to  the  catalog  iteration  procedure
                supplied by the client.

    PassBack  - Supplies  a  value uninterpreted by this procedure.  This value
                is  passed  unmodified to the catalog iteration procedure.  The
                client can use this value to carry context between the original
                call site and the iteration procedure.

Return Value:

    None
--*/
{
    PLIST_ENTRY         ListMember;
    PPROTO_CATALOG_ITEM CatalogEntry;
    BOOL                enumerate_more;

    assert(Iteration != NULL);

    enumerate_more = TRUE;

    AcquireCatalogLock();

    ListMember = m_protocol_list.Flink;

    while (enumerate_more && (ListMember != & m_protocol_list)) {
        CatalogEntry = CONTAINING_RECORD(
            ListMember,
            PROTO_CATALOG_ITEM,
            m_CatalogLinkage);
        ListMember = ListMember->Flink;
        enumerate_more = (* Iteration) (
            PassBack,     // PassBack
            CatalogEntry  // CatalogEntry
            );
    } //while

    ReleaseCatalogLock();

}  // EnumerateCatalogItems




INT
DCATALOG::GetCountedCatalogItemFromCatalogEntryId(
    IN  DWORD                     CatalogEntryId,
    OUT PPROTO_CATALOG_ITEM FAR * CatalogItem
    )
/*++

Routine Description:

    This  procedure  retrieves  a  reference  to a catalog item given a catalog
    entry ID to search for.

    The operation takes care of creating, initializing, and setting a DPROVIDER
    object  for the retrieved catalog item if necessary. 

Arguments:

    CatalogEntryId  - Supplies The ID of a catalog entry to be searched for.

    CatalogItem     - Returns a reference to the catalog item with the matching
                      catalog entry ID if it is found, otherwise returns NULL.

Return Value:

  The  function  returns  ERROR_SUCESS  if  successful, otherwise it returns an
  appropriate WinSock error code.
--*/
{
    PLIST_ENTRY         ListMember;
    INT                 ReturnCode;
    PPROTO_CATALOG_ITEM CatalogEntry;

    assert(CatalogItem != NULL);

    // Prepare for early error return
    * CatalogItem = NULL;
    ReturnCode = WSAEINVAL;

    AcquireCatalogLock();

    ListMember = m_protocol_list.Flink;

    while (ListMember != & m_protocol_list) {
        CatalogEntry = CONTAINING_RECORD(
            ListMember,
            PROTO_CATALOG_ITEM,
            m_CatalogLinkage);
        ListMember = ListMember->Flink;
        if (CatalogEntry->GetProtocolInfo()->dwCatalogEntryId==CatalogEntryId) {
            if (CatalogEntry->GetProvider() == NULL) {
                ReturnCode = LoadProvider(
                    CatalogEntry    // CatalogEntry
                    );
                if (ReturnCode != ERROR_SUCCESS) {
                    break;
                }
            }  // if provider is NULL
            CatalogEntry->Reference ();
            *CatalogItem = CatalogEntry;
            ReturnCode = ERROR_SUCCESS;
            break;
        } //if
    } //while

    ReleaseCatalogLock();
    return(ReturnCode);
}  // GetCatalogItemFromCatalogEntryId

INT
DCATALOG::GetCountedCatalogItemFromAddressFamily(
    IN  INT af,
    OUT PPROTO_CATALOG_ITEM FAR * CatalogItem
    )
/*++

Routine Description:

    This  procedure  retrieves  a  reference  to a catalog item given an
    address  to search for.

    The operation takes care of creating, initializing, and setting a DPROVIDER
    object  for the retrieved catalog item if necessary.  This includes setting
    the DPROVIDER object in all catalog entries for the same provider.

Arguments:

    af  - Supplies The address family to be searched for.

    CatalogItem     - Returns a reference to the catalog item with the matching
                      catalog entry ID if it is found, otherwise returns NULL.

Return Value:

  The  function  returns  ERROR_SUCESS  if  successful, otherwise it returns an
  appropriate WinSock error code.
--*/
{
    PLIST_ENTRY         ListMember;
    INT                 ReturnCode;
    PPROTO_CATALOG_ITEM CatalogEntry;

    assert(CatalogItem != NULL);

    // Prepare for early error return
    * CatalogItem = NULL;
    ReturnCode = WSAEINVAL;

    AcquireCatalogLock();

    ListMember = m_protocol_list.Flink;

    while (ListMember != & m_protocol_list) {
        CatalogEntry = CONTAINING_RECORD(
            ListMember,
            PROTO_CATALOG_ITEM,
            m_CatalogLinkage);
        ListMember = ListMember->Flink;

        if (CatalogEntry->GetProtocolInfo()->iAddressFamily == af) {
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
            //
            // Found something, break out.
            //
            break;
        } //if
    } //while

    ReleaseCatalogLock();
    return(ReturnCode);
}



INT
DCATALOG::GetCountedCatalogItemFromAttributes(
    IN  INT     af,
    IN  INT     type,
    IN  INT     protocol,
    IN  DWORD   StartAfterId OPTIONAL,
    OUT PPROTO_CATALOG_ITEM FAR * CatalogItem
    )
/*++

Routine Description:

    Retrieves a PROTO_CATALOG_ITEM reference, choosing an item from the catalog
    based  on  three parameters (af, type, protocol) to determine which service
    provider  is used.  The procedure selects the first transport provider able
    to support the stipulated address family, socket type, and protocol values.
    If "protocol" is not specifieid (i.e., equal to zero).  the default for the
    specified socket type is used.  However, the address family may be given as
    AF_UNSPEC  (unspecified),  in  which  case the "protocol" parameter must be
    specified.   The protocol number to use is particular to the "communication
    domain" in which communication is to take place.

    The operation takes care of creating, initializing, and setting a DPROVIDER
    object  for the retrieved catalog item if necessary. 

Arguments:

    af          - Supplies an address family specification

    type        - Supplies a socket type specification

    protocol    - Supplies  an  address  family  specific  identification  of a
                  protocol  to  be  used with a socket, or 0 if the caller does
                  not wish to specify a protocol.

    StartAfterId - Optionally (non 0) supplies the catalog id of the item 
                    after which to begin enumeration.

    CatalogItem - Returns  a reference to the catalog item that was found to be
                  a suitable match or NULL if no suitable match was found.


Return Value:

    The  function  returns ERROR_SUCCESS if successful, otherwise it returns an
    appropriate WinSock error code.

Implementation Notes:

    For  each  protocol  item  to  test,  match  first  type, then family, then
    protocol.   Keep  track of the "strongest" match found.  If there was not a
    complete  match,  the  strength of the strongest match determines the error
    code returned.
--*/
{
#define MATCHED_NONE 0
#define MATCHED_TYPE 1
#define MATCHED_TYPE_FAMILY 2
#define MATCHED_TYPE_FAMILY_PROTOCOL 3
#define LARGER_OF(a,b) (((a) > (b)) ? (a) : (b))

    PLIST_ENTRY ListMember;
    PDPROVIDER  LocalProvider;
    INT         ReturnCode;
    PPROTO_CATALOG_ITEM CatalogEntry;
    LPWSAPROTOCOL_INFOW ProtoInfo;
    INT match_strength = MATCHED_NONE;

    assert(CatalogItem != NULL);

    // Prepare for early error returns
    * CatalogItem = NULL;

    // Parameter consistency check:
    if (af == 0) {
        if( protocol == 0 ) {
            //
            // These cannot both be zero.
            //

            return WSAEINVAL;
        }

        DEBUGF(
            DBG_WARN,
            ("Use of AF_UNSPEC is discouraged\n"));
        // Unfortunately we cannot treat this as an error case.
    }

    AcquireCatalogLock();

    ListMember = m_protocol_list.Flink;

    // Find the place to start if asked
    if( StartAfterId != 0 ) {
        while (ListMember != & m_protocol_list) {
            CatalogEntry = CONTAINING_RECORD(
                ListMember,
                PROTO_CATALOG_ITEM,
                m_CatalogLinkage);
            ListMember = ListMember->Flink;
            if (CatalogEntry->GetProtocolInfo()->dwCatalogEntryId==StartAfterId)
                break;
        }
    }


    while ((ListMember != & m_protocol_list) &&
        (match_strength < MATCHED_TYPE_FAMILY_PROTOCOL))
    {
        CatalogEntry = CONTAINING_RECORD(
            ListMember,
            PROTO_CATALOG_ITEM,
            m_CatalogLinkage);
        ListMember = ListMember->Flink;
        ProtoInfo = CatalogEntry->GetProtocolInfo();
        if (ProtoInfo->ProtocolChain.ChainLen != LAYERED_PROTOCOL) {
#define TYPE_WILDCARD_VALUE 0
            // Can  this  entry  support  the  requested  socket  type?   Or is the
            // wildcard type specified?
            if ((ProtoInfo->iSocketType == type) ||
                (type == TYPE_WILDCARD_VALUE)) {
                match_strength = LARGER_OF(
                    match_strength,
                    MATCHED_TYPE);

#define FAMILY_WILDCARD_VALUE AF_UNSPEC
                // Can it support the requested address family?  Or is the wildcard
                // family specified?
                if ((ProtoInfo->iAddressFamily == af) ||
                    (af == FAMILY_WILDCARD_VALUE)) {
                    match_strength = LARGER_OF(
                        match_strength,
                        MATCHED_TYPE_FAMILY);

#define PROTO_IN_RANGE(proto,lo,hi) (((proto) >= (lo)) && ((proto) <= (hi)))
#define IS_BIT_SET(test_val,bitmask) (((test_val) & (bitmask)) == (bitmask))
                    // Is  the  requested  protcol  in  range?  Or is the requested
                    // protocol zero and entry supports protocol zero?
                    {  // declare block
                        int range_lo = ProtoInfo->iProtocol;
                        int range_hi = range_lo + ProtoInfo->iProtocolMaxOffset;
                        if (PROTO_IN_RANGE(protocol, range_lo, range_hi) ||
                            ((protocol == 0) &&
                             IS_BIT_SET(
                                 ProtoInfo->dwProviderFlags,
                                 PFL_MATCHES_PROTOCOL_ZERO))) {
                            match_strength = LARGER_OF(
                                match_strength,
                                MATCHED_TYPE_FAMILY_PROTOCOL);
                        } // if protocol supported
                    } // declare block
                } //if address family supported
            } //if type supported
        } //if not layered protocol
    }  // while


    // Select  an  appropriate error code for "no match" cases, or success code
    // to proceed.
    switch (match_strength) {
        case MATCHED_NONE:
            ReturnCode = WSAESOCKTNOSUPPORT;
            break;

        case MATCHED_TYPE:
            ReturnCode = WSAEAFNOSUPPORT;
            break;

        case MATCHED_TYPE_FAMILY:
            ReturnCode = WSAEPROTONOSUPPORT;
            break;

        case MATCHED_TYPE_FAMILY_PROTOCOL:
            // A full match found, continue
            ReturnCode = ERROR_SUCCESS;
            break;

        default:
            DEBUGF(
                DBG_ERR,
                ("Should not get here\n"));
            ReturnCode = WSASYSCALLFAILURE;

    }  // switch (match_strength)

    if (ReturnCode == ERROR_SUCCESS) {
        if (CatalogEntry->GetProvider() == NULL) {
            ReturnCode = LoadProvider(
                CatalogEntry
                );
            if (ReturnCode != ERROR_SUCCESS) {
                DEBUGF(
                    DBG_ERR,
                    ("Error (%lu) loading chosen provider\n",
                    ReturnCode));
            } // else
        }  // if provider is NULL
    }  // if ReturnCode is ERROR_SUCCESS

    if (ReturnCode == ERROR_SUCCESS) {
        CatalogEntry->Reference ();
        * CatalogItem = CatalogEntry;
    } // if ReturnCode is ERROR_SUCCESS

    ReleaseCatalogLock();

    return ReturnCode;

}  // GetCountedCatalogItemFromAttributes


INT
DCATALOG::FindIFSProviderForSocket(
    SOCKET Socket
    )

/*++

Routine Description:

    This procedure searches the installed providers that support IFS handles
    for one that recognizes the given socket. If one is found, then the
    necessary internal infrastructure is established for supporting the
    socket.

Arguments:

    Socket - The socket.

Return Value:

    If  the  function  is  successful,  it  returns ERROR_SUCCESS, otherwise it
    returns an appropriate WinSock error code.

--*/

{

    INT result;
    INT error;
    INT optionLength;
    PLIST_ENTRY listEntry;
    PPROTO_CATALOG_ITEM catalogItem;
    PDPROVIDER provider;
    WSAPROTOCOL_INFOW protocolInfo;
    SOCKET modifiedSocket;
    DWORD  serial_num;

    //
    // Scan the installed providers.
    //

    AcquireCatalogLock();

Restart:
    for( listEntry = m_protocol_list.Flink ;
         listEntry != &m_protocol_list ;
         listEntry = listEntry->Flink ) {

        catalogItem = CONTAINING_RECORD(
                          listEntry,
                          PROTO_CATALOG_ITEM,
                          m_CatalogLinkage
                          );

        //
        // Skip non-IFS providers.
        //

        if( ( catalogItem->GetProtocolInfo()->dwServiceFlags1 &
                XP1_IFS_HANDLES ) == 0 ) {

            continue;

        }

        //
        // Load the provider if necessary.
        //

        provider = catalogItem->GetProvider();

        if( provider == NULL ) {

            result = LoadProvider(
                         catalogItem
                         );

            if( result != NO_ERROR ) {

                //
                // Could not load the provider. Press on regardless.
                //

                continue;

            }
            provider = catalogItem->GetProvider ();

            assert( provider != NULL );
        }

        //
        // Reference catalog item, remeber current catalog serial
        // number, and release the lock to prevent a deadlock
        // in case provider waits in another thread on catalog lock
        // while holding on of its locks which it may need to acquire
        // while we are calling into it.
        //
        catalogItem->Reference ();
        serial_num = m_serial_num;
        ReleaseCatalogLock ();

        //
        // Try a getsockopt( SO_PROTOCOL_INFOW ) on the socket to determine
        // if the current provider recognizes it. This has the added benefit
        // of returning the dwCatalogEntryId for the socket, which we can
        // use to call WPUModifyIFSHandle().
        //

        optionLength = sizeof(protocolInfo);

        result = provider->WSPGetSockOpt(
                     Socket,
                     SOL_SOCKET,
                     SO_PROTOCOL_INFOW,
                     (char FAR *)&protocolInfo,
                     &optionLength,
                     &error
                     );

        // Do not need catalog item any longer
        catalogItem->Dereference ();
        if( result != ERROR_SUCCESS ) {

            //
            // WPUGetSockOpt() failed, probably because the socket is
            // not recognized. Continue on and try another provider.
            //

            AcquireCatalogLock ();
            //
            // Check if catalog has changed while we are calling
            // into the provider, if so, restart the lookup
            // otherwise, press on.
            //
            if (serial_num==m_serial_num)
                continue;
            else
                goto Restart;

        }

        //
        // Call WPUModifyIFSHandle(). The current implementation doesn't
        // actually modify the handle, but it does setup the necessary
        // internal infrastructure for the socket.
        //
        // Note that provider might have already called this function,
        // in which case our call will have no effect (because we support
        // layered providers that reuse base provider sockets which
        // employ the same method.
        //
        //
        // !!! We should move the "create the DSocket object and setup
        //     all of the internal stuff" from WPUModifyIFSHandle() into
        //     a common function shared with this function.
        //

        modifiedSocket = WPUModifyIFSHandle(
                             protocolInfo.dwCatalogEntryId,
                             Socket,
                             &error
                             );

        if( modifiedSocket == INVALID_SOCKET ) {

            //
            // This error is not continuable, as the provider has
            // recognized the socket, but for some reason we cannot
            // create the necessary internal infrastructure for the
            // socket. We have no choice here except to just bail out
            // and fail the request.
            //
            // !!! The provider may have established internal state for
            //     this socket. Should we invoke provider->WSPCloseSocket()
            //     on it now to remove any such state?
            //

            return WSAENOTSOCK;

        }

        //
        // Success!
        //

        assert( modifiedSocket == Socket );
        return ERROR_SUCCESS;

    }

    //
    // If we made it this far, then no provider recognized the socket.
    //

    ReleaseCatalogLock();
    return WSAENOTSOCK;

} // FindIFSProviderForSocket


DWORD
DCATALOG::AllocateCatalogEntryId (
    VOID
    )
{
    DWORD   id;
    AcquireCatalogLock ();
    assert (m_reg_key!=NULL);

    if (m_next_id!=0)
        id = m_next_id++;
    else
        id = 0;

    ReleaseCatalogLock ();
    return id;
}



VOID
DCATALOG::AppendCatalogItem(
    IN  PPROTO_CATALOG_ITEM  CatalogItem
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
    assert (IsListEmpty (&CatalogItem->m_CatalogLinkage));

    InsertTailList(
        & m_protocol_list,               // ListHead
        & CatalogItem->m_CatalogLinkage  // Entry
       );
    m_num_items++;
}  // AppendCatalogItem




VOID
DCATALOG::RemoveCatalogItem(
    IN  PPROTO_CATALOG_ITEM  CatalogItem
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
    assert(!IsListEmpty (&CatalogItem->m_CatalogLinkage));

    RemoveEntryList(
        & CatalogItem->m_CatalogLinkage  // Entry
        );
#if defined(DEBUG_TRACING)
    InitializeListHead (&CatalogItem->m_CatalogLinkage);
#endif
    assert(m_num_items > 0);
    m_num_items--;
}  // RemoveCatalogItem


LPSTR
DCATALOG::GetCurrentCatalogName()
{
    return CATALOG_NAME;

} // GetCurrentCatalogName


INT
DCATALOG::LoadProvider(
    IN PPROTO_CATALOG_ITEM CatalogEntry
    )
/*++

Routine Description:

    Load   the   provider  described  by  CatalogEntry and set it into
    catalog entry

Arguments:

    CatalogEntry - Supplies  a reference to a protocol catalog entry describing
                   the provider to load.


Return Value:

    The  function  returns ERROR_SUCCESS if successful, otherwise it returns an
    appropriate WinSock error code.
--*/
{
    INT ReturnCode = ERROR_SUCCESS;
    PDPROVIDER LocalProvider;

    assert(CatalogEntry != NULL);

    // Serialize provider loading/unloading
    AcquireCatalogLock ();

    // Check if provider is loaded under the lock
    if (CatalogEntry->GetProvider ()==NULL) {

        // First attempt to find another instance of the provider
        LocalProvider = FindAnotherProviderInstance (
                                CatalogEntry->GetProviderId ());
        if (LocalProvider != NULL) {
            // Success, just set it
            CatalogEntry->SetProvider (LocalProvider);
            ReturnCode = ERROR_SUCCESS;
        }
        else {
            // Create and attempt to load provider object
            LocalProvider = new(DPROVIDER);
            if (LocalProvider !=NULL ) {

                ReturnCode = LocalProvider->Initialize(
                    CatalogEntry->GetLibraryPath(),
                    CatalogEntry->GetProtocolInfo()
                    );
                if (ERROR_SUCCESS == ReturnCode) {
                    CatalogEntry->SetProvider (LocalProvider);
                } //if

                LocalProvider->Dereference ();
            } //if
            else {
                DEBUGF(
                    DBG_ERR,
                    ("Couldn't allocate a DPROVIDER object\n"));
                ReturnCode = WSA_NOT_ENOUGH_MEMORY;
            }
        } // else
    } // if provider not loaded
  
    // Serialize provider loading/unloading
    ReleaseCatalogLock ();
    return(ReturnCode);
}  // LoadProvider




PDPROVIDER
DCATALOG::FindAnotherProviderInstance(
    IN LPGUID ProviderId
    )
/*++

Routine Description:

    Check  all catalog enteries for a provider with the pointer to the provider
    object for the provider.

Arguments:

    ProviderId - Supplies the Provider ID for the catalog enteries to check in.


Return Value:

    Pointer to provider object if found

Implementation notes:

--*/
{
    PLIST_ENTRY ListMember;
    PPROTO_CATALOG_ITEM CatalogEntry;
    PDPROVIDER   LocalProvider;

    ListMember = m_protocol_list.Flink;

    while (ListMember != & m_protocol_list)
    {
        CatalogEntry = CONTAINING_RECORD(
            ListMember,
            PROTO_CATALOG_ITEM,
            m_CatalogLinkage);
        ListMember = ListMember->Flink;
        LocalProvider = CatalogEntry->GetProvider ();
        if( (LocalProvider!=NULL) // This check is much less expensive
                && (*(CatalogEntry->GetProviderId()) == *ProviderId)) {
            return LocalProvider;
        } //if
    } //while

    return NULL;
}  // FindAnotherProviderInstance






