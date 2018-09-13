/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    nscatitem.cpp

Abstract:

    This  file  contains  the  class  implementation for the NSCATALOGENTRY
    class.   This  class  defines  the  interface  to  the  entries that can be
    installed and retrieved in the namespace provider catalog.

Author:

    Dirk Brandewie (dirk@mink.intel.com) 09-Nov-1995

Notes:

    $Revision:   1.16  $

    $Modtime:   08 Mar 1996 15:36:46  $

Revision History:

    most-recent-revision-date email-name
        description
    09-Nov-1995 dirk@mink.intel.com
        Initial Revision

--*/


#include "precomp.h"



NSCATALOGENTRY::NSCATALOGENTRY()
/*++

Routine Description:

    This  procedure  constructs  an empty NSCATALOGENTRY object.  The first
    method  invoked  after  this  constructor must be InitializeFromRegistry or
    InitializeFromValues.

Arguments:

    None

Return Value:

    Implicitly  returns  a pointer to a newly created NSCATALOGENTRY object
    or NULL if there is a memory allocation failure.
--*/
{
    DEBUGF(
        DBG_TRACE,
        ("NSCATALOGENTRY constructor\n"));

    m_reference_count = 1;
    // Simply initialize embedded pointer values for safety.
#if defined(DEBUG_TRACING)
    InitializeListHead (&m_CatalogLinkage);
#endif
    m_LibraryPath[0] = '\0';
    m_providerDisplayString = NULL;
    m_namespace_id = 0;
    m_provider = NULL;
    m_enabled = TRUE;
    m_version = NULL;
    m_address_family = -1;        // all by default
}  // NSCATALOGENTRY



// The   following   two   defines  determine  the  number  of  digits  in  the
// sequence-numbered  name  of  each  catalog  entry key.  The two defines must
// include  the  same  number.   If there is a way to cause the preprocessor to
// derive both a quoted and unquoted character sequence from the same sequence,
// I don't know what it is.
#define SEQUENCE_KEY_DIGITS 12
#define SEQUENCE_KEY_DIGITS_STRING "12"




INT
NSCATALOGENTRY::InitializeFromRegistry(
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
    INT   sock_result;

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
        return(WSANO_RECOVERY);
    }

    sock_result = IoRegistry(
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
        return(WSANO_RECOVERY);
    }

    return sock_result;

}  // InitializeFromRegistry




INT
NSCATALOGENTRY::InitializeFromValues(
    IN  LPWSTR           LibraryPath,
    IN  LPWSTR           DisplayString,
    IN  LPGUID           ProviderId,
    IN  DWORD            NameSpaceId,
    IN  DWORD            Version
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

    DisplayString  - Supplies a reference to a buffer holding a
                     zero-terminated display string for this namespace
                     privider.

    ProviderId - A pointer to the GUID for this provider.

    NameSpaceId - The ID of the namespace this provider serves.

Return Value:

    The  function  returns ERROR_SUCCESS if successful, otherwise it returns an
    appropriate WinSock error code.

--*/
{
    size_t  len_needed;
    INT ReturnValue = ERROR_SUCCESS;

    // Copy LibraryPath
    len_needed = wcslen(LibraryPath) + 1;
    if (len_needed > sizeof(m_LibraryPath)) {
        DEBUGF(
            DBG_ERR,
            ("Library Path Too long (%u) '%S'\n",
            len_needed,
            LibraryPath));
        return(WSAEINVAL);
    }
    wcscpy(
        m_LibraryPath,
        LibraryPath);

    // Copy DisplayString
    m_providerDisplayString =  new WCHAR[wcslen(DisplayString) + 1];
    if (m_providerDisplayString != NULL) {
        (void) wcscpy(
            m_providerDisplayString,
            DisplayString);
    } else {
        ReturnValue = WSAENOBUFS;
    }


    m_providerId = *ProviderId;
    m_namespace_id = NameSpaceId;
    m_version = Version;
    m_address_family = -1;    // BUGBUG. For now

    return (ReturnValue);

}  // InitializeFromValues




NSCATALOGENTRY::~NSCATALOGENTRY()
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
    assert (IsListEmpty (&m_CatalogLinkage));
    if (m_providerDisplayString != NULL) {
        delete m_providerDisplayString;
        m_providerDisplayString = NULL;
    } else {
        DEBUGF(
            DBG_WARN,
            ("Destructing uninitialized protocol catalog item\n"));
    }
    if (m_provider!=NULL)
    {
        m_provider->Dereference ();
        m_provider = NULL;
    } //if

}  // ~NSCATALOGENTRY





VOID
NSCATALOGENTRY::SetProvider(
    IN  PNSPROVIDER  Provider
    )
/*++

Routine Description:

    This procedure sets the NSPROVIDER associated with a catalog entry.

Arguments:

    Provider - Supplies the new NSPROVIDER reference.

Return Value:

    None
--*/
{
    assert (m_provider==NULL);
    Provider->Reference ();
    m_provider = Provider;
}  // SetProvider



INT
NSCATALOGENTRY::WriteToRegistry(
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
    INT   sock_result;
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
        return(WSANO_RECOVERY);
    }
    if (key_disposition == REG_OPENED_EXISTING_KEY) {
        DEBUGF(
            DBG_WARN,
            ("Overwriting a catalog entry key '%s'\n",
            keyname));
    }

    sock_result = IoRegistry(
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
        return(WSANO_RECOVERY);
    }

    return sock_result;

}  // WriteToRegistry




VOID
NSCATALOGENTRY::Enable(
    IN BOOLEAN EnableValue
    )
/*++

Routine Description:

    Sets the enabled state of this catalog entry

Arguments:

    EnableValue - The new state value.

Return Value:

    NONE

--*/
{
    m_enabled = EnableValue;
}





INT
NSCATALOGENTRY::IoRegistry(
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
--*/
{
    BOOL io_result;
    DWORD  val;
    WSABUF carrier;
    INT pathLength;
    CHAR ansiPath[MAX_PATH];

    // The  library pathnames are expandable against environment variables.  So
    // technically they should be REG_EXPAND_SZ instead of REG_SZ.  However, as
    // of  09-14-1995,  the  registry  editor does not display REG_EXPAND_SZ as
    // strings.   So  to  ease debugging and diagnostics, the library pathnames
    // are written into the registry as REG_SZ instead.

    // char m_LibraryPath[MAX_PATH];
    if (IsRead) {
        io_result = ReadRegistryEntry(
            EntryKey,                                     // EntryKey
            "LibraryPath",                                // EntryName
            ansiPath,                                     // Data
            sizeof(ansiPath),                             // MaxBytes
            REG_SZ                                        // TypeFlag
            );

        if( io_result ) {
            pathLength = MultiByteToWideChar(
                CP_ACP,                                   // CodePage
                0,                                        // dwFlags
                ansiPath,                                 // lpMultiByteStr
                -1,                                       // cchMultiByte
                m_LibraryPath,                            // lpWideCharStr
                sizeof(m_LibraryPath) / sizeof(WCHAR)     // cchWideChar
                );

            io_result = ( pathLength > 0 );
        }
    } else {
        pathLength = WideCharToMultiByte(
            CP_ACP,                                       // CodePage
            0,                                            // dwFlags
            m_LibraryPath,                                // lpWideCharStr
            -1,                                           // cchWideChar
            ansiPath,                                     // lpMultiByteStr
            sizeof(ansiPath),                             // cchMultiByte
            NULL,
            NULL
            );

        if( pathLength == 0 ) {

            io_result = FALSE;

        } else {

            io_result = WriteRegistryEntry(
                EntryKey,                                     // EntryKey
                "LibraryPath",                                // EntryName
                ansiPath,                                     // Data
                REG_SZ                                        // TypeFlag
                );

        }

    }

    if (! io_result) {
        DEBUGF(
            DBG_ERR,
            ("error %s registry entry\n",
            IsRead ? "reading" : "writing"));
        return WSANO_RECOVERY;
    }

    // PCHAR m_providerDisplayString
    if (IsRead) {
        // Determine size and allocate space for dynamically allocated parts.
        LONG lresult;

        // RegQueryValueEx  includes  the  null  terminator  when returning the
        // length of a string.
        lresult = RegQueryValueEx(
            EntryKey,            // hkey
            "DisplayString",      // lpszValueName
            0,                   // lpdwReserved
            NULL,                // lpdwType
            NULL,                // lpbData
            & val                // lpcbData
            );
        if (lresult != ERROR_SUCCESS) {
            DEBUGF(
                DBG_ERR,
                ("querying length of ProviderName entry\n"));
            return WSANO_RECOVERY;
        }
        if (m_providerDisplayString != NULL) {
            DEBUGF(
                DBG_ERR,
                ("should never be re-reading a catalog entry\n"));
            return WSANO_RECOVERY;
        }
        if (val >= MAX_PATH) {
            DEBUGF(
                DBG_ERR,
                ("cannot handle provider names > MAX_PATH\n"));
            return WSANO_RECOVERY;
        }
        m_providerDisplayString =  new WCHAR[val];
        if (m_providerDisplayString == NULL) {
            return WSANO_RECOVERY;
        }
    }  // if (isRead)


    // PCHAR m_providerDisplayString
    if (IsRead) {
        io_result = ReadRegistryEntry(
            EntryKey,                                     // EntryKey
            "DisplayString",                              // EntryName
            ansiPath,                                     // Data
            sizeof(ansiPath),                             // MaxBytes
            REG_SZ                                            // TypeFlag
            );

        if( io_result ) {
            pathLength = MultiByteToWideChar(
                CP_ACP,                                   // CodePage
                0,                                        // dwFlags
                ansiPath,                                 // lpMultiByteStr
                -1,                                       // cchMultiByte
                m_providerDisplayString,                  // lpWideCharStr
                val                                       // cchWideChar
                );

            io_result = ( pathLength > 0 );
        }
    } else {
        pathLength = WideCharToMultiByte(
            CP_ACP,                                       // CodePage
            0,                                            // dwFlags
            m_providerDisplayString,                      // lpWideCharStr
            -1,                                           // cchWideChar
            ansiPath,                                     // lpMultiByteStr
            sizeof(ansiPath),                             // cchMultiByte
            NULL,
            NULL
            );

        if( pathLength == 0 ) {

            io_result = FALSE;

        } else {

            io_result = WriteRegistryEntry(
                EntryKey,                                     // EntryKey
                "DisplayString",                              // EntryName
                ansiPath,                                     // Data
                REG_SZ                                        // TypeFlag
                );

        }
    }
    if (! io_result) {
        DEBUGF(
            DBG_ERR,
            ("error %s registry entry\n",
            IsRead ? "reading" : "writing"));
        return WSANO_RECOVERY;
    }

    // GUID ProviderId;
    if (IsRead) {
        carrier.len = sizeof(GUID);
        carrier.buf = (char*)& m_providerId;

        io_result = ReadRegistryEntry(
            EntryKey,                                  // EntryKey
            "ProviderId",                              // EntryName
            (PVOID) & (carrier),                       // Data
            sizeof(GUID),                              // MaxBytes
            REG_BINARY                                 // TypeFlag
            );
    } else {
        carrier.len = sizeof(GUID);
        carrier.buf = (char*) &m_providerId;
        io_result = WriteRegistryEntry(
            EntryKey,                                     // EntryKey
            "ProviderId",                                 // EntryName
            (PVOID) & (carrier),                          // Data
            REG_BINARY                                    // TypeFlag
            );
    }
    if (! io_result) {
        DEBUGF(
            DBG_ERR,
            ("error %s registry entry\n",
            IsRead ? "reading" : "writing"));
        return WSANO_RECOVERY;
    }


    // DWORD m_address_family;
    if (IsRead) {
        io_result = ReadRegistryEntry(
            EntryKey,                                     // EntryKey
            "AddressFamily",                              // EntryName
            (PVOID) & (m_address_family),                 // Data
            sizeof(DWORD),                                // MaxBytes
            REG_DWORD                                     // TypeFlag
            );
        if(!io_result)
        {
            //
            // since this key may not exist, treat an error
            // as an acceptable case and simply store the
            // default value.
            //
            m_address_family = -1;
            io_result = TRUE;
        }
    } else {
       //
       // only do this if a value has been set
       //
       if(m_address_family != -1)
       {
           io_result = WriteRegistryEntry(
                EntryKey,                                     // EntryKey
                "AddressFamily",                              // EntryName
                (PVOID) & (m_address_family),                 // Data
                REG_DWORD                                     // TypeFlag
                );
        }
        else
        {
           io_result = TRUE;
        }
    }
    if (! io_result) {
        DEBUGF(
            DBG_ERR,
            ("error %s registry entry\n",
            IsRead ? "reading" : "writing"));
        return WSANO_RECOVERY;
    }

    // DWORD m_namespace_id;
    if (IsRead) {
        io_result = ReadRegistryEntry(
            EntryKey,                                     // EntryKey
            "SupportedNameSpace",                         // EntryName
            (PVOID) & (m_namespace_id),                   // Data
            sizeof(DWORD),                                // MaxBytes
            REG_DWORD                                     // TypeFlag
            );
    } else {
       io_result = WriteRegistryEntry(
            EntryKey,                                     // EntryKey
            "SupportedNameSpace",                         // EntryName
            (PVOID) & (m_namespace_id),                   // Data
            REG_DWORD                                     // TypeFlag
            );
    }
    if (! io_result) {
        DEBUGF(
            DBG_ERR,
            ("error %s registry entry\n",
            IsRead ? "reading" : "writing"));
        return WSANO_RECOVERY;
    }

    // BOOLEAN m_enabled;
    if (IsRead) {
        io_result = ReadRegistryEntry(
            EntryKey,                                     // EntryKey
            "Enabled",                                    // EntryName
            &val,                                         // Data
            sizeof(DWORD),                                // MaxBytes
            REG_DWORD                                     // TypeFlag
            );
        if (io_result)
            m_enabled = (val!=0);
    } else {
       val = m_enabled ? 1 : 0;
       io_result = WriteRegistryEntry(
            EntryKey,                                     // EntryKey
            "Enabled",                                    // EntryName
            &val,                                         // Data
            REG_DWORD                                     // TypeFlag
            );
    }
    if (! io_result) {
        DEBUGF(
            DBG_ERR,
            ("error %s registry entry\n",
            IsRead ? "reading" : "writing"));
        return WSANO_RECOVERY;
    }

        // DWORD m_version;
    if (IsRead) {
        io_result = ReadRegistryEntry(
            EntryKey,                                     // EntryKey
            "Version",                                    // EntryName
            (PVOID) & (m_version),                        // Data
            sizeof(DWORD),                                // MaxBytes
            REG_DWORD                                     // TypeFlag
            );
    } else {
       io_result = WriteRegistryEntry(
            EntryKey,                                     // EntryKey
            "Version",                                    // EntryName
            (PVOID) & (m_version),                        // Data
            REG_DWORD                                     // TypeFlag
            );
    }
    if (! io_result) {
        DEBUGF(
            DBG_ERR,
            ("error %s registry entry\n",
            IsRead ? "reading" : "writing"));
        return WSANO_RECOVERY;
    }

    // BOOLEAN m_stores_service_class_info;
    if (IsRead) {
        io_result = ReadRegistryEntry(
            EntryKey,                                     // EntryKey
            "StoresServiceClassInfo",                     // EntryName
            &val,                                         // Data
            sizeof(DWORD),                                // MaxBytes
            REG_DWORD                                     // TypeFlag
            );
        if (io_result)
            m_stores_service_class_info = (val!=0);
    } else {
       val = m_stores_service_class_info ? 1 : 0;
       io_result = WriteRegistryEntry(
            EntryKey,                                     // EntryKey
            "StoresServiceClassInfo",                     // EntryName
            &val,                                         // Data
            REG_DWORD                                     // TypeFlag
            );
    }
    if (! io_result) {
        DEBUGF(
            DBG_ERR,
            ("error %s registry entry\n",
            IsRead ? "reading" : "writing"));
        return WSANO_RECOVERY;
    }

    return(ERROR_SUCCESS);
}  // IoRegistry

