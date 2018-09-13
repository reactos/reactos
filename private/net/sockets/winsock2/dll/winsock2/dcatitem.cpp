/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    dcatitem.cpp

Abstract:

    This  file  contains  the  class  implementation for the PROTO_CATALOG_ITEM
    class.   This  class  defines  the  interface  to  the  entries that can be
    installed and retrieved in the protocol catalog.

Author:

    Paul Drews (drewsxpa@ashland.intel.com) 31-July-1995

Notes:

    $Revision:   1.16  $

    $Modtime:   08 Mar 1996 13:16:44  $

Revision History:

    most-recent-revision-date email-name
        description

    23-Aug-1995 dirk@mink.intel.com
        Moved includes into precomp.h

    31-July-1995 drewsxpa@ashland.intel.com
        Original created from code separated out from dcatalog module.

--*/


#include "precomp.h"



PROTO_CATALOG_ITEM::PROTO_CATALOG_ITEM()
/*++

Routine Description:

    This  procedure  constructs  an empty PROTO_CATALOG_ITEM object.  The first
    method  invoked  after  this  constructor must be InitializeFromRegistry or
    InitializeFromValues.

Arguments:

    None

Return Value:

    Implicitly  returns  a pointer to a newly created PROTO_CATALOG_ITEM object
    or NULL if there is a memory allocation failure.
--*/
{
    DEBUGF(
        DBG_TRACE,
        ("PROTO_CATALOG_ITEM constructor\n"));

    // Simply initialize embedded pointer values for safety.
    m_LibraryPath[0] = '\0';
    m_Provider = NULL;
    m_reference_count = 1;
#if defined(DEBUG_TRACING)
    InitializeListHead (&m_CatalogLinkage);
#endif
}  // PROTO_CATALOG_ITEM



// The   following   two   defines  determine  the  number  of  digits  in  the
// sequence-numbered  name  of  each  catalog  entry key.  The two defines must
// include  the  same  number.   If there is a way to cause the preprocessor to
// derive both a quoted and unquoted character sequence from the same sequence,
// I don't know what it is.
#define SEQUENCE_KEY_DIGITS 12
#define SEQUENCE_KEY_DIGITS_STRING "12"




INT
PROTO_CATALOG_ITEM::InitializeFromRegistry(
    IN  HKEY  ParentKey,
    IN  INT   SequenceNum
    )
/*++

Routine Description:

    This  procedure initializes the protocol info part of the catalog item from
    information  retrieved  from  the  catalog  portion of the registry.  It is
    assumed that the catalog portion is locked against competing I/O attempts.

Arguments:

    ParentKey   - Supplies  an open registry key for the registry entry that is
                  the  parent  of  the registry item defining the catalog item,
                  i.e., the catalog entry list key.

    SequenceNum - Supplies  the  sequence  number  within the entry list of the
                  target registry entry.

Return Value:

    The  function  returns ERROR_SUCCESS if successful, otherwise it returns an
    appropriate WinSock error code.

--*/
{
    char  keyname[SEQUENCE_KEY_DIGITS + 1];
    HKEY  thiskey;
    LONG  result;
    INT   ReturnCode;

    sprintf(keyname, "%0"SEQUENCE_KEY_DIGITS_STRING"i", SequenceNum);
    result = RegOpenKeyEx(
        ParentKey,     // hkey
        keyname,       // lpszSubKey
        0,             // dwReserved
        KEY_READ,      // samDesired
        & thiskey      // phkResult
        );
    if (result != ERROR_SUCCESS) {
        DEBUGF(
            DBG_ERR,
            ("Corrupted catalog entry key '%s', error = %lu\n",
            keyname,
            result));
        return(WSASYSCALLFAILURE);
    }

    ReturnCode = IoRegistry(
        thiskey,  // EntryKey
        TRUE      // IsRead
        );

    result = RegCloseKey(
        thiskey  // hkey
        );
    if (result != ERROR_SUCCESS) {
        DEBUGF(
            DBG_ERR,
            ("Couldn't close catalog entry, error = %lu\n",
            result));
        return(WSASYSCALLFAILURE);
    }

    return ReturnCode;

}  // InitializeFromRegistry




INT
PROTO_CATALOG_ITEM::InitializeFromValues(
    IN  LPSTR               LibraryPath,
    IN  LPWSAPROTOCOL_INFOW ProtoInfo
    )
/*++

Routine Description:

    This  procedure initializes the protocol info part of the catalog item from
    the  values  supplied.  Values are copied from the structures passed by the
    caller,  so  the  caller  is  free  to  deallocate the passed structures on
    return.

Arguments:

    LibraryPath  - Supplies    a    reference   to   a   buffer   holding   the
                   zero-terminated,   fully-qualified   path  of  the  provider
                   library that implements this protocol.  The path may include
                   expandable environment references of the form '%variable%'.

    ProviderName - Supplies a reference to a buffer holding the zero-terminated
                   locally unique name of this provider.

    ProtoInfo    - Supplies  a reference to the fully initialized protocol info
                   structure describing this protocol.

return Value:

    The  function  returns ERROR_SUCCESS if successful, otherwise it returns an
    appropriate WinSock error code.

--*/
{
    size_t  len_needed;
    INT ReturnValue = ERROR_SUCCESS;

    // Copy LibraryPath
    len_needed = lstrlen(LibraryPath) + 1;
    if (len_needed > sizeof(m_LibraryPath)) {
        DEBUGF(
            DBG_ERR,
            ("Library Path Too long (%u) '%s'\n",
            len_needed,
            LibraryPath));
        return(WSAEINVAL);
    }
    lstrcpy(
        m_LibraryPath,
        LibraryPath);

    // Copy ProtoInfo
    m_ProtoInfo = *ProtoInfo;

    return (ReturnValue);

}  // InitializeFromValues




PROTO_CATALOG_ITEM::~PROTO_CATALOG_ITEM()
/*++

Routine Description:

    This  procedure  destroys  a  protocol catalog item, deallocating memory it
    owns.   It  is the caller's responsibility to remove the item from the list
    it  occupies  before  calling  this  procedure.   It  is  also the caller's
    responsibility  to unload and/or destroy any dprovider associated with this
    catalog item if appropriate.

Arguments:

    None

Return Value:

    None
--*/
{
    if (m_Provider!=NULL) {
        m_Provider->Dereference ();
        m_Provider = NULL;
    }
    assert(IsListEmpty (&m_CatalogLinkage));
}  // ~PROTO_CATALOG_ITEM





VOID
PROTO_CATALOG_ITEM::SetProvider(
    IN  PDPROVIDER  Provider
    )
/*++

Routine Description:

    This procedure sets the DPROVIDER associated with a catalog entry.

Arguments:

    Provider - Supplies the new DPROVIDER reference.

Return Value:

    None
--*/
{
    assert (m_Provider==NULL);
    Provider->Reference ();
    m_Provider = Provider;
}  // SetProvider



INT
PROTO_CATALOG_ITEM::WriteToRegistry(
    IN  HKEY  ParentKey,
    IN  INT   SequenceNum
    )
/*++

Routine Description:

    This  procedure  writes  the fully-initialized protocol info portion of the
    catalog  entry  out  to the catalog portion of the registry.  It is assumed
    that  the  catalog  portion of the registry is locked against competing I/O
    attempts.

Arguments:

    ParentKey -   Supplies  the  open registry key of the parent registry entry
                  in which this catalog entry will be written as a subkey.

    Sequencenum - Supplies the sequence number of this catalog entry within the
                  entire set of catalog entries.

Return Value:

    The  function  returns ERROR_SUCCESS if successful, otherwise it returns an
    appropriate WINSOCK error code.
--*/
{
    char  keyname[SEQUENCE_KEY_DIGITS + 1];
    HKEY  thiskey;
    LONG  result;
    INT   ReturnCode;
    DWORD key_disposition;


    sprintf(keyname, "%0"SEQUENCE_KEY_DIGITS_STRING"i", SequenceNum);
    result = RegCreateKeyEx(
        ParentKey,                // hkey
        keyname,                  // lpszSubKey
        0,                        // dwReserved
        NULL,                     // lpszClass
        REG_OPTION_NON_VOLATILE,  // fdwOptions
        KEY_ALL_ACCESS,           // samDesired
        NULL,                     // lpSecurityAttributes
        & thiskey,                // phkResult
        & key_disposition         // lpdwDisposition
        );
    if (result != ERROR_SUCCESS) {
        DEBUGF(
            DBG_ERR,
            ("Error occurred creating catalog entry key (%lu)\n",
            result));
        return(WSASYSCALLFAILURE);
    }
    if (key_disposition == REG_OPENED_EXISTING_KEY) {
        DEBUGF(
            DBG_WARN,
            ("Overwriting a catalog entry key '%s'\n",
            keyname));
    }

    ReturnCode = IoRegistry(
        thiskey,  // EntryKey
        FALSE     // IsRead
        );

    result = RegCloseKey(
        thiskey  // hkey
        );
    if (result != ERROR_SUCCESS) {
        DEBUGF(
            DBG_ERR,
            ("Couldn't close catalog entry, error = %lu\n",
            result));
        return(WSASYSCALLFAILURE);
    }

    return ReturnCode;

}  // WriteToRegistry



// The following typedef is used in packing and unpacking catalog item data for
// reading and writing in the registry.

typedef struct {
    char            LibraryPath[MAX_PATH];
        // The unexpanded path where the provider DLL is found.

    WSAPROTOCOL_INFOW   ProtoInfo;
        // The  protocol information.  Note that if the WSAPROTOCOL_INFOW structure
        // is  ever changed to a non-flat structure (i.e., containing pointers)
        // then  this  type  definition  will  have  to  be changed, since this
        // structure must be strictly flat.
} PACKED_CAT_ITEM;

typedef PACKED_CAT_ITEM * PPACKED_CAT_ITEM;


#define PACKED_ITEM_NAME "PackedCatalogItem"



INT
PROTO_CATALOG_ITEM::IoRegistry(
    IN  HKEY  EntryKey,
    IN  BOOL  IsRead)
/*++

Routine Description:

    This  procedure  performs  the  actual  input  or  output  of catalog entry
    information  from  or  to  the  registry.   It  is assumed that the catalog
    portion of the registry is locked against competing I/O attempts.

Arguments:

    EntryKey - Supplies  the open registry key where the catalog entry is to be
               read or written.

    IsRead   - Supplies  a  BOOL  determining  disposition.  TRUE indicates the
               entry  is  to  be  read  into  memory  from the registry.  FALSE
               indicates  the  entry  is  to  be written out from memory to the
               registry.

Return Value:

    The  function  returns ERROR_SUCCESS if successful, otherwise it returns an
    appropriate WINSOCK error code.

Implementation Notes:

    An  early  implementation represented the catalog item as a single registry
    key  with multiple named values.  Each named value corresponded to a single
    member  variable  of  the  catalog  entry  or  field  of  the protocol info
    structure.   Thus each catalog item consisted of about 20 named values in a
    registry key.  Unfortunately, the registry has very poor performance for so
    many values.

    Therefore  the  implementation  has been changed to pack the entire catalog
    item  into  a single REG_BINARY value.  Each catalog item then has a single
    key  with  a  single  large  REG_BINARY value.  This mitigates the registry
    performance problem.
--*/
{
    DWORD             packed_size;
    PPACKED_CAT_ITEM  packed_buf = NULL;
    INT               return_value;

    return_value = ERROR_SUCCESS;


    TRY_START(guard_memalloc) {
        // Determine required size of packed structure
        if (IsRead) {
            LONG lresult;

            lresult = RegQueryValueEx(
                EntryKey,          // hkey
                PACKED_ITEM_NAME,  // lpszValueName
                0,                 // lpdwReserved
                NULL,              // lpdwType
                NULL,              // lpbData
                & packed_size      // lpcbData
                );
            if (lresult != ERROR_SUCCESS) {
                DEBUGF(
                    DBG_ERR,
                    ("querying length of %s entry\n", PACKED_ITEM_NAME));
                return_value = WSASYSCALLFAILURE;
                TRY_THROW(guard_memalloc);
            }
        } // if IsRead
        else { // not IsRead
            packed_size = sizeof(PACKED_CAT_ITEM);
        } // else not IsRead


        // Allocate memory for packed structure
        packed_buf = (PPACKED_CAT_ITEM) new char[packed_size];
        if (packed_buf == NULL) {
            DEBUGF(
                DBG_ERR,
                ("allocating space for packed entry\n"));
            return_value = WSA_NOT_ENOUGH_MEMORY;
            TRY_THROW(guard_memalloc);
        }


        // If writing, then initialize the packed structure
        if (! IsRead) {
            lstrcpy(
                packed_buf->LibraryPath,
                m_LibraryPath);
            packed_buf->ProtoInfo = m_ProtoInfo;
        } // if ! IsRead


        // Read or write the structure
        { // declaration block
            BOOL io_result;
            WSABUF  io_descr;

            io_descr.len = packed_size;
            io_descr.buf = (char FAR *) packed_buf;
            if (IsRead) {
                io_result = ReadRegistryEntry(
                    EntryKey,             // EntryKey
                    PACKED_ITEM_NAME,     // EntryName
                    (PVOID) & io_descr,   // Data
                    packed_size,          // MaxBytes
                    REG_BINARY            // TypeFlag
                    );
            } // if IsRead
            else { // not IsRead
                io_result = WriteRegistryEntry(
                    EntryKey,             // EntryKey
                    PACKED_ITEM_NAME,     // EntryName
                    (PVOID) & io_descr,   // Data
                    REG_BINARY            // TypeFlag
                    );
            } // else not IsRead
            if (! io_result) {
                DEBUGF(
                    DBG_ERR,
                    ("error %s registry entry\n",
                    IsRead ? "reading" : "writing"));
                return_value = WSASYSCALLFAILURE;
                TRY_THROW(guard_memalloc);
            }
            if (io_descr.len != packed_size) {
                DEBUGF(
                    DBG_ERR,
                    ("Registry entry expected (%lu), got (%lu)\n",
                    packed_size,
                    io_descr.len));
                return_value = WSASYSCALLFAILURE;
                TRY_THROW(guard_memalloc);
            }
        } // declaration block


        // If reading, extract data from the packed structure
        if (IsRead) {
            lstrcpy(
                m_LibraryPath,
                packed_buf->LibraryPath);
            m_ProtoInfo = packed_buf->ProtoInfo;
        } // if IsRead


        // deallocate the packed structure
        delete packed_buf;

    } TRY_CATCH(guard_memalloc) {
        if (return_value == ERROR_SUCCESS) {
            return_value = WSASYSCALLFAILURE;
        }
        if (packed_buf!=NULL)
            delete packed_buf;
    } TRY_END(guard_memalloc);

    return(return_value);

}  // IoRegistry

