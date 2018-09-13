/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    wsautil.cpp

Abstract:

    This  module  contains utility functions for the winsock DLL implementation
    that did not seem to fit into the other module.

Author:

    Dirk Brandewie dirk@mink.intel.com

Notes:

    $Revision:   1.24  $

    $Modtime:   14 Feb 1996 10:32:32  $

Revision History:

    23-Aug-1995 dirk@mink.intel.com
        Moved includes into precomp.h

    07-31-1995 drewsxpa@ashland.intel.com
        Added Registry-manipulation functions

    07-18-1995 dirk@mink.intel.com
        Initial revision

--*/

#include "precomp.h"

//
// Global pointer to the appropriate prolog function. This either points
// to Prolog_v1 for WinSock 1.1 apps or Prolog_v2 for WinSock 2.x apps.
//

LPFN_PROLOG PrologPointer = &Prolog_v2;
HANDLE      gHeap = NULL;

INT
WINAPI
SlowPrologOvlp (
	OUT	LPWSATHREADID FAR *	ThreadId
	) 
{
    PDPROCESS	Process;
    PDTHREAD	Thread;
	INT			ErrorCode;
	
	ErrorCode = PROLOG(&Process,&Thread);
	if (ErrorCode==ERROR_SUCCESS) {
		*ThreadId = Thread->GetWahThreadID();
	}

	return (ErrorCode);
}

INT
WINAPI
SlowProlog (
	VOID
	) 
{
    PDPROCESS	Process;
    PDTHREAD	Thread;
	INT			ErrorCode;

	ErrorCode = PROLOG(&Process,&Thread);

	return (ErrorCode);
}


INT
WINAPI
Prolog_v2(
    OUT PDPROCESS FAR * Process,
    OUT PDTHREAD FAR * Thread
    )

/*++

Routine Description:

     This routine is the standard WinSock 1.1 prolog function used at all the
     winsock API entrypoints.  This function ensures that the process has
     called WSAStartup.

Arguments:

    Process   - Pointer to the DPROCESS object for the process calling the
                winsock API.

    Thread    - Pointer to the DTHREAD object for the calling thread

Returns:

    This function returns ERROR_SUCCESS if successful, otherwise 
    the specific WinSock error code

--*/

{   
    INT ErrorCode;
    if ((*Process = DPROCESS::GetCurrentDProcess()) !=NULL) {
        ErrorCode = DTHREAD::GetCurrentDThread(*Process, Thread);
    } //if
    else {
        ErrorCode = WSANOTINITIALISED;
    }
    return(ErrorCode);

}   // Prolog_v2


INT
WINAPI
Prolog_v1(
    OUT PDPROCESS FAR * Process,
    OUT PDTHREAD FAR * Thread
    )

/*++

Routine Description:

     This routine is the standard WinSock 1.1 prolog function used at all the
     winsock API entrypoints.  This function ensures that the process has
     called WSAStartup and that the current thread in the process does not have
     a WinSock call outstanding.

Arguments:

    Process   - Pointer to the DPROCESS object for the process calling the
                winsock API.

    Thread    - Pointer to the DTHREAD object for the calling thread


Returns:

    This function returns ERROR_SUCCESS if successful, otherwise 
    the specific WinSock error code

--*/

{
    INT ErrorCode;


    if ((*Process=DPROCESS::GetCurrentDProcess())!=NULL) {
        ErrorCode = DTHREAD::GetCurrentDThread(*Process, Thread);
        if (ErrorCode == ERROR_SUCCESS) {
            if( !(*Thread)->IsBlocking() ) {
                ;
            } else {
                ErrorCode = WSAEINPROGRESS;
            }
        } //if

    } //if
    else {
        ErrorCode = WSANOTINITIALISED;
    }
    return(ErrorCode);
}   // Prolog_v1


INT
WINAPI
Prolog_Detached(
    OUT PDPROCESS FAR * Process,
    OUT PDTHREAD FAR * Thread
    )

/*++

Routine Description:

    API prolog used after we've been detached from the process's address
    space. In theory, this should be totally unnecessary, but at least one
    popular DLL (MFC 4.x) calls WSACleanup() in its process detach handler,
    which may occur *after* our DLL is already detached. Grr...


Arguments:

    Process - Unused.

    Thread - Unused.


Returns:

    INT - Always WSASYSNOTREADY.

--*/

{

    Process;
    Thread;

    return WSASYSNOTREADY;

}   // Prolog_Detached


BOOL
WriteRegistryEntry(
    IN HKEY     EntryKey,
    IN LPCTSTR  EntryName,
    IN PVOID    Data,
    IN DWORD    TypeFlag
    )
/*++

Routine Description:

    This  procedure  writes  a  single named value into an opened registry key.
    The  value  may  be  any  type whose length can be determined from its type
    (e.g., scalar types, zero-terminated strings).

Arguments:

    EntryKey  - Supplies  the open entry key under which the new named value is
                to be written.

    EntryName - Supplies the name of the value to be written.

    Data      - Supplies  a  reference  to the location where the entry data is
                found,  or to a WSABUF describing the data location in the case
                of REG_BINARY data.

    TypeFlag  - Supplies  an identifier for the type of the data to be written.
                Supported   types  are  REG_BINARY,  REG_DWORD,  REG_EXPAND_SZ,
                REG_SZ.    Types   not   supported   are  REG_DWORD_BIG_ENDIAN,
                REG_DWORD_LITTLE_ENDIAN,   REG_LINK,   REG_MULTI_SZ,  REG_NONE,
                REG_RESOURCE_LIST.   Note  that  depending on the architecture,
                one   of   the   "big_endian"   or   "little_endian"  forms  of
                REG_DWORD_x_ENDIAN  is implicitly allowed, since it is equal to
                REG_DWORD.

Return Value:

    The function returns TRUE if successful, or FALSE if an error occurred.

Implementation note:

    There was no need identified for the REG_MULTI_SZ case, so support for this
    case  was  omitted  since  it was more difficult to derive the data length.
    There  is  no  reason  in  principle why this case cannot be added if it is
    really needed.
--*/
{
    DWORD  cbData;
    LONG   result;
    BOOL   ok_so_far;
    BYTE * data_buf;

    assert(
        (TypeFlag == REG_BINARY) ||
        (TypeFlag == REG_DWORD) ||
        (TypeFlag == REG_EXPAND_SZ) ||
        (TypeFlag == REG_SZ));


    ok_so_far = TRUE;

    switch (TypeFlag) {
        case REG_BINARY:
            cbData = (DWORD) (((LPWSABUF) Data)->len);
            data_buf = (BYTE *) (((LPWSABUF) Data)->buf);
            break;

        case REG_DWORD:
            cbData = sizeof(DWORD);
            data_buf = (BYTE *) Data;
            break;

        case REG_EXPAND_SZ:
            cbData = (DWORD) (lstrlen((char *) Data)+1);
            data_buf = (BYTE *) Data;
            break;

        case REG_SZ:
            cbData = (DWORD) (lstrlen((char *) Data)+1);
            data_buf = (BYTE *) Data;
            break;

        default:
            DEBUGF(
                DBG_ERR,
                ("Unsupported type flag specified (%lu)",
                TypeFlag));
            ok_so_far = FALSE;
            break;

    }  // switch (TypeFlag)

    if (ok_so_far) {
        result = RegSetValueEx(
            EntryKey,             // hkey
            (LPCTSTR) EntryName,  // lpszValueName
            0,                    // dwReserved
            TypeFlag,             // fdwType
            data_buf,             // lpbData
            cbData                // cbData
            );
        if (result != ERROR_SUCCESS) {
            DEBUGF(
                DBG_ERR,
                ("Could not set value '%s'\n",
                EntryName));
            ok_so_far = FALSE;
        } // if not success
    } // if ok_so_far

    return (ok_so_far);

}  // WriteRegistryEntry




BOOL
ReadRegistryEntry(
    IN  HKEY    EntryKey,
    IN  LPTSTR  EntryName,
    OUT PVOID   Data,
    IN  DWORD   MaxBytes,
    IN  DWORD   TypeFlag
    )
/*++

Routine Description:

    This procedure reads a single named value from an opened registry key.  The
    value  may  be any type whose length can be determined from its type (e.g.,
    scalar  types,  zero-terminated  strings).  The function checks the type of
    the newly read value to make sure it matches the expected type.

Arguments:

    EntryKey  - Supplies  the  open entry key from which the new named value is
                to be read.

    EntryName - Supplies the name of the value to be read.

    Data      - Supplies  a  reference  to the location where the entry data is
                placed.   Returns  the registry entry value.  In the case where
                the  TypeFlag  is  REG_BINARY,  this is a reference to a WSABUF
                describing the target data buffer.  The "len" field returns the
                length read (or required) from the registry.

    MaxData   - Supplies the size in bytes of the Data buffer supplied.

    TypeFlag  - Supplies  an  identifier  for  the type of the data to be read.
                Supported   types  are  REG_BINARY,  REG_DWORD,  REG_EXPAND_SZ,
                REG_SZ.    Types   not   supported   are  REG_DWORD_BIG_ENDIAN,
                REG_DWORD_LITTLE_ENDIAN,   REG_LINK,   REG_MULTI_SZ,  REG_NONE,
                REG_RESOURCE_LIST.   Note  that  depending on the architecture,
                one   of   the   "big_endian"   or   "little_endian"  forms  of
                REG_DWORD_x_ENDIAN  is implicitly allowed, since it is equal to
                REG_DWORD.

Return Value:

    The  function  returns  TRUE  if successful, or FALSE if an error occurred.
    Errors include unsupported types, non-matching types, and oversize data.

Implementation note:

    There was no need identified for the REG_MULTI_SZ case, so support for this
    case  was  omitted  since  it was more difficult to derive the data length.
    There  is  no  reason  in  principle why this case cannot be added if it is
    really needed.

    The  validity  checks  in this routine have been written as a linear series
    instead  of  in the "conditional-tunnelling" nested-if form.  The series of
    tests  is  long  enough that the nested-if form is far too complex to read.
    This  procedure  should  not  be sensitive to execution speed, so the extra
    tests and branches in the linear series form should not be a problem.
--*/
{
    DWORD  count_expected;
    LONG   result;
    DWORD  type_read;
    DWORD  entry_size;
    BOOL   need_exact_length;
    BOOL   ok_so_far;
    BYTE * data_buf;

    assert(
        (TypeFlag == REG_BINARY) ||
        (TypeFlag == REG_DWORD) ||
        (TypeFlag == REG_EXPAND_SZ) ||
        (TypeFlag == REG_SZ));

    ok_so_far = TRUE;

    switch (TypeFlag) {
        case REG_BINARY:
            count_expected = MaxBytes;
            // Special case: REG_BINARY length compared against maximum
            need_exact_length = FALSE;
            data_buf = (BYTE *) (((LPWSABUF) Data)->buf);
            break;

        case REG_DWORD:
            count_expected = sizeof(DWORD);
            need_exact_length = TRUE;
            data_buf = (BYTE *) Data;
            break;

        case REG_EXPAND_SZ:
            count_expected = MaxBytes;
            // Special case: strings length compared against maximum
            need_exact_length = FALSE;
            data_buf = (BYTE *) Data;
            break;

        case REG_SZ:
            count_expected = MaxBytes;
            // Special case: strings length compared against maximum
            need_exact_length = FALSE;
            data_buf = (BYTE *) Data;
            break;

        default:
            DEBUGF(
                DBG_ERR,
                ("Unsupported type flag specified (%lu)",
                TypeFlag));
            ok_so_far = FALSE;
            break;

    }  // switch (TypeFlag)


    // Read from registry
    if (ok_so_far) {
        entry_size = MaxBytes;
        result = RegQueryValueEx(
            EntryKey,            // hkey
            (LPTSTR) EntryName,  // lpszValueName
            0,                   // dwReserved
            & type_read,         // lpdwType
            data_buf,            // lpbData
            & entry_size         // lpcbData
            );
        if (result != ERROR_SUCCESS) {
            DEBUGF(
                DBG_WARN,
                ("Could not read value '%s'\n",
                EntryName));
            if (result == ERROR_MORE_DATA) {
                DEBUGF(
                    DBG_WARN,
                    ("Data buffer too small\n"));
            } // if ERROR_MORE_DATA
            ok_so_far = FALSE;
        } // if result != ERROR_SUCCESS
    } // if ok_so_far


    // Special case for REG_BINARY
    if (TypeFlag == REG_BINARY) {
        (((LPWSABUF) Data)->len) = (u_long) entry_size;
    }


    // check type
    if (ok_so_far) {
        if (type_read != TypeFlag) {
            DEBUGF(
                DBG_ERR,
                ("Type read (%lu) different from expected (%lu)\n",
                type_read,
                TypeFlag));
            ok_so_far = FALSE;
        } // if type_read != TypeFlag
    } // if ok_so_far


    // Check length
    if (ok_so_far) {
        if (need_exact_length) {
            if (count_expected != entry_size) {
                DEBUGF(
                    DBG_ERR,
                    ("Length read (%lu) different from expected (%lu)\n",
                    entry_size,
                    count_expected));
                ok_so_far = FALSE;
             } // if size mismatch
        } // if need_exact_length
    } // if ok_so_far

    return ok_so_far;

}  // ReadRegistryEntry




LONG
RegDeleteKeyRecursive(
    IN HKEY  hkey,
    IN LPCTSTR  lpszSubKey
    )
/*++

Routine Description:

    The RegDeleteKeyRecursive function deletes the specified key and all of its
    subkeys, recursively.

Arguments:

    hkey       - Supplies  a  currently  open  key  or  any  of  the  following
                 predefined reserved handle values:

                 HKEY_CLASSES_ROOT
                 HKEY_CURRENT_USER
                 HKEY_LOCAL_MACHINE
                 HKEY_USERS

                 The key specified by the lpszSubKey parameter must be a subkey
                 of the key identified by hkey.

    lpszSubKey - Supplies  a  reference  to a null-terminated string specifying
                 the name of the key to delete.  This parameter cannot be NULL.
                 The specified key may have subkeys.

Return Value:

    If  the  function  succeeds,  the  return  value  is ERROR_SUCCESS.  If the
    function fails, the return value is an operating system error value.

Implementation Notes:

    Open targetkey
    while find subkey
        RegDeleteKeyRecursive(... subkey)
    end while
    close targetkey
    delete targetkey
--*/
{
    LONG result;
    HKEY targetkey;
    LONG return_value;

    DEBUGF(
        DBG_TRACE,
        ("RegDeleteKeyRecursive (%lu), '%s'\n",
        (ULONG_PTR) hkey,
        lpszSubKey));

    result = RegOpenKeyEx(
        hkey,            // hkey
        lpszSubKey,      // lpszSubKey
        0,               // dwReserved
        KEY_ALL_ACCESS,  // samDesired
        & targetkey      // phkResult
        );
    if (result != ERROR_SUCCESS) {
        DEBUGF(
            DBG_WARN,
            ("Unable to open key '%s' to be deleted\n",
            lpszSubKey));
        return result;
    }

    {
        BOOL      deleting_subkeys;
        LPTSTR    subkey_name;
        DWORD     subkey_name_len;
        FILETIME  dont_care;

        return_value = ERROR_SUCCESS;
        deleting_subkeys = TRUE;
        subkey_name = (LPTSTR) new char[MAX_PATH];
        if (subkey_name == NULL) {
            return_value = ERROR_OUTOFMEMORY;
            deleting_subkeys = FALSE;
        }
        while (deleting_subkeys) {
            subkey_name_len = MAX_PATH;
            // Since  we  delete  a  subkey  each  time  through this loop, the
            // remaining  subkeys  effectively  get  renumbered.  Therefore the
            // subkey   index  we  "enumerate"  each  time  is  0  (instead  of
            // incrementing) to retrieve any remaining subkey.
            result = RegEnumKeyEx(
                targetkey,         // hkey
                0,                 // iSubkey
                subkey_name,       // lpszName
                & subkey_name_len, // lpcchName
                0,                 // lpdwReserved
                NULL,              // lpszClass
                NULL,              // lpcchClass
                & dont_care        // lpftLastWrite
                );
            switch (result) {
                case ERROR_SUCCESS:
                    result = RegDeleteKeyRecursive(
                        targetkey,   // hkey
                        subkey_name  // lpszSubKey
                        );
                    if (result != ERROR_SUCCESS) {
                        deleting_subkeys = FALSE;
                        return_value = result;
                    }
                    break;

                case ERROR_NO_MORE_ITEMS:
                    deleting_subkeys = FALSE;
                    break;

                default:
                    DEBUGF(
                        DBG_ERR,
                        ("Unable to enumerate subkeys\n"));
                    deleting_subkeys = FALSE;
                    return_value = result;
                    break;

            }  // switch (result)
        }  // while (deleting_subkeys)

        delete subkey_name;
    }

    result = RegCloseKey(
        targetkey  // hkey
        );
    if (result != ERROR_SUCCESS) {
        DEBUGF(
            DBG_ERR,
            ("Unable to close subkey '%s'\n",
            lpszSubKey));
        return_value = result;
    }

    result = RegDeleteKey(
        hkey,       // hkey
        lpszSubKey  // lpszSubKey
        );
    if (result != ERROR_SUCCESS) {
        DEBUGF(
            DBG_WARN,
            ("Unable to delete subkey '%s'\n",
            lpszSubKey));
        return_value = result;
    }

    return return_value;

}  // RegDeleteKeyRecursive


LONG
RegDeleteSubkeys(
    IN HKEY  hkey
    )
/*++

Routine Description:

    Deletes all the first level subkeys of the specified key

Arguments:

    hkey       - Supplies  a  currently  open  key  or  any  of  the  following
                 predefined reserved handle values:

                 HKEY_CLASSES_ROOT
                 HKEY_CURRENT_USER
                 HKEY_LOCAL_MACHINE
                 HKEY_USERS


Return Value:

    If  the  function  succeeds,  the  return  value  is ERROR_SUCCESS.  If the
    function fails, the return value is an operating system error value.

Implementation Notes:

--*/
{
    BOOL      deleting_subkeys = TRUE;
    LONG result;
    LONG return_value;
    LPTSTR    subkey_name;
    DWORD     subkey_name_len;
    FILETIME  dont_care;

    DEBUGF(
        DBG_TRACE,
        ("RegDeleteSubkeys (%lu)\n", (ULONG_PTR)hkey));

    subkey_name = (LPTSTR) new char[MAX_PATH];
    if (subkey_name == NULL) {
        return WSA_NOT_ENOUGH_MEMORY;
    }

    return_value = ERROR_SUCCESS;
    while (deleting_subkeys) {
        subkey_name_len = MAX_PATH;
        // Since  we  delete  a  subkey  each  time  through this loop, the
        // remaining  subkeys  effectively  get  renumbered.  Therefore the
        // subkey   index  we  "enumerate"  each  time  is  0  (instead  of
        // incrementing) to retrieve any remaining subkey.
        result = RegEnumKeyEx(
            hkey,               // hkey
            0,                 // iSubkey
            subkey_name,       // lpszName
            & subkey_name_len, // lpcchName
            0,                 // lpdwReserved
            NULL,              // lpszClass
            NULL,              // lpcchClass
            & dont_care        // lpftLastWrite
            );
        switch (result) {
            case ERROR_SUCCESS:
                result = RegDeleteKey(
                    hkey,        // hkey
                    subkey_name  // lpszSubKey
                    );
                if (result != ERROR_SUCCESS) {
                    deleting_subkeys = FALSE;
                    return_value = result;
                }
                break;

            case ERROR_NO_MORE_ITEMS:
                deleting_subkeys = FALSE;
                break;

            default:
                DEBUGF(
                    DBG_ERR,
                    ("Unable to enumerate subkeys\n"));
                deleting_subkeys = FALSE;
                return_value = result;
                break;

        }  // switch (result)
    }  // while (deleting_subkeys)

    delete subkey_name;

    return return_value;

}  // RegDeleteSubkeys


HKEY
OpenWinSockRegistryRoot()
/*++

Routine Description:

    This  procedure opens the root of the WinSock2 portion of the registry.  It
    takes  care  of  creating  and initializing the root if necessary.  It also
    takes  care  of  comparing versions of the WinSock2 portion of the registry
    and updating the registry version if required.

    It   is   the  caller's  responsibility  to  call  CloseWinSockRegistryRoot
    eventually with the returned key.

Arguments:

    None

Return Value:

    The  function  returns the opened registry key if successful.  If it is not
    successful, it returns NULL.

Implementation Notes:

    The first version of this function has no previous versions of the registry
    to  be  compatible  with,  so it does not have to take care of updating any
    out-of-date  registry  information.   If  and  when  the  WinSock  spec  or
    implementation  is  updated  in a way that changes the registry information
    this procedure may have to be updated to update the registry.
--*/
{
    HKEY  root_key;
    LONG  lresult;
    DWORD  create_disp;

    DEBUGF(
        DBG_TRACE,
        ("OpenWinSockRegistryRoot\n"));

    //
    // We must first try to open the key before trying to create it.
    // RegCreateKeyEx() will fail with ERROR_ACCESS_DENIED if the current
    // user has insufficient privilege to create the target registry key,
    // even if that key already exists.
    //

    lresult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,             // hkey
        WINSOCK_REGISTRY_ROOT,          // lpszSubKey
        0,                              // dwReserved
        MAXIMUM_ALLOWED,                // samDesired
        & root_key                      // phkResult
        );

    if( lresult == ERROR_SUCCESS ) {
        create_disp = REG_OPENED_EXISTING_KEY;
    } else if( lresult == ERROR_FILE_NOT_FOUND ) {
        lresult = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,         // hkey
            WINSOCK_REGISTRY_ROOT,      // lpszSubKey
            0,                          // dwReserved
            NULL,                       // lpszClass
            REG_OPTION_NON_VOLATILE,    // fdwOptions
            KEY_ALL_ACCESS,             // samDesired
            NULL,                       // lpSecurityAttributes
            & root_key,                 // phkResult
            & create_disp               // lpdwDisposition
            );
    }
    if (lresult != ERROR_SUCCESS) {
        DEBUGF(
            DBG_ERR,
            ("Result (%lu) creating/opening registry root\n",
            lresult));
        return NULL;
    }

    TRY_START(guard_root_open) {
        BOOL   bresult;
        TCHAR  reg_version[] = WINSOCK_REGISTRY_VERSION_VALUE;
            // Initialization forces size to be the size desired.

        switch (create_disp) {
            case REG_CREATED_NEW_KEY:
                bresult = WriteRegistryEntry(
                    root_key,                               // EntryKey
                    WINSOCK_REGISTRY_VERSION_NAME,          // EntryName
                    (PVOID)WINSOCK_REGISTRY_VERSION_VALUE,  // Data
                    REG_SZ                                  // TypeFlag
                    );
                if (! bresult) {
                    DEBUGF(
                        DBG_ERR,
                        ("Writing version value to registry\n"));
                    TRY_THROW(guard_root_open);
                }
                break;

            case REG_OPENED_EXISTING_KEY:
                bresult = ReadRegistryEntry(
                    root_key,                               // EntryKey
                    WINSOCK_REGISTRY_VERSION_NAME,          // EntryName
                    (PVOID) reg_version,                    // Data
                    sizeof(reg_version),                    // MaxBytes
                    REG_SZ                                  // TypeFlag
                    );
                if (! bresult) {
                    DEBUGF(
                        DBG_ERR,
                        ("Reading version value from registry\n"));
                    TRY_THROW(guard_root_open);
                }
                if (lstrcmp(reg_version, WINSOCK_REGISTRY_VERSION_VALUE) != 0) {
                    DEBUGF(
                        DBG_ERR,
                        ("Expected registry version '%s', got '%s'\n",
                        WINSOCK_REGISTRY_VERSION_VALUE,
                        reg_version));
                    TRY_THROW(guard_root_open);
                }
                break;

            default:
                break;

        }  // switch (create_disp)

    } TRY_CATCH(guard_root_open) {
        CloseWinSockRegistryRoot(root_key);
        root_key = NULL;
    } TRY_END(guard_root_open);

    return root_key;

}  // OpenWinSockRegistryRoot




VOID
CloseWinSockRegistryRoot(
    HKEY  RootKey
    )
/*++

Routine Description:

    This  procedure  closes  the open registry key representing the root of the
    WinSock  portion  of the registry.  The function checks for and handles any
    errors that might occur.

Arguments:

    RootKey - Supplies  the  open  registry  key  representing  the root of the
              WinSock portion of the registry.

Return Value:

    None
--*/
{
    LONG lresult;

    DEBUGF(
        DBG_TRACE,
        ("Closing registry root\n"));

    lresult = RegCloseKey(
        RootKey  // hkey
        );
    if (lresult != ERROR_SUCCESS) {
        DEBUGF(
            DBG_ERR,
            ("Unexpected result (%lu) closing registry root\n",
            lresult));
    }

}  // CloseWinSockRegistryRoot





INT
MapUnicodeProtocolInfoToAnsi(
    IN  LPWSAPROTOCOL_INFOW UnicodeProtocolInfo,
    OUT LPWSAPROTOCOL_INFOA AnsiProtocolInfo
    )
/*++

Routine Description:

    This procedure maps a UNICODE WSAPROTOCOL_INFOW structure to the
    corresponding ANSI WSAPROTOCOL_INFOA structure. All scalar fields
    are copied over "as is" and any embedded strings are mapped.

Arguments:

    UnicodeProtocolInfo - Points to the source WSAPROTOCOL_INFOW structure.

    AnsiProtocolInfo - Points to the destination WSAPROTOCOL_INFOA structure.

Return Value:

    INT - ERROR_SUCCESS if successful, a Win32 status code otherwise.

--*/
{

    INT result;

    //
    // Sanity check.
    //

    assert( UnicodeProtocolInfo != NULL );
    assert( AnsiProtocolInfo != NULL );


    __try {
        //
        // Copy over the scalar values.
        //
        // Just to make things a bit easier, this code depends on the fact
        // that the szProtocol[] character array is the last field of the
        // WSAPROTOCOL_INFO structure.
        //

        CopyMemory(
            AnsiProtocolInfo,
            UnicodeProtocolInfo,
            sizeof(*UnicodeProtocolInfo) - sizeof(UnicodeProtocolInfo->szProtocol)
            );

        //
        // And now map the string from UNICODE to ANSI.
        //

        result = WideCharToMultiByte(
                     CP_ACP,                                    // CodePage (ANSI)
                     0,                                         // dwFlags
                     UnicodeProtocolInfo->szProtocol,           // lpWideCharStr
                     -1,                                        // cchWideChar
                     AnsiProtocolInfo->szProtocol,              // lpMultiByteStr
                     sizeof(AnsiProtocolInfo->szProtocol),      // cchMultiByte
                     NULL,                                      // lpDefaultChar
                     NULL                                       // lpUsedDefaultChar
                     );

        if( result == 0 ) {

            //
            // WideCharToMultiByte() failed.
            //

            return WSASYSCALLFAILURE;

        }

        //
        // Success!
        //

        return ERROR_SUCCESS;
    }
    __except (WS2_EXCEPTION_FILTER()) {
        return WSAEFAULT;
    }

}   // MapUnicodeProtocolInfoToAnsi





INT
MapAnsiProtocolInfoToUnicode(
    IN  LPWSAPROTOCOL_INFOA AnsiProtocolInfo,
    OUT LPWSAPROTOCOL_INFOW UnicodeProtocolInfo
    )
/*++

Routine Description:

    This procedure maps an ANSI WSAPROTOCOL_INFOA structure to the
    corresponding UNICODE WSAPROTOCOL_INFOW structure. All scalar fields
    are copied over "as is" and any embedded strings are mapped.

Arguments:

    AnsiProtocolInfo - Points to the source WSAPROTOCOL_INFOA structure.

    UnicodeProtocolInfo - Points to the destination WSAPROTOCOL_INFOW
        structure.

Return Value:

    INT - ERROR_SUCCESS if successful, a Win32 status code otherwise.

--*/
{

    INT result;

    //
    // Sanity check.
    //

    assert( AnsiProtocolInfo != NULL );
    assert( UnicodeProtocolInfo != NULL );

    __try {
        //
        // Copy over the scalar values.
        //
        // Just to make things a bit easier, this code depends on the fact
        // that the szProtocol[] character array is the last field of the
        // WSAPROTOCOL_INFO structure.
        //

        CopyMemory(
            UnicodeProtocolInfo,
            AnsiProtocolInfo,
            sizeof(*AnsiProtocolInfo) - sizeof(AnsiProtocolInfo->szProtocol)
            );

        //
        // And now map the string from ANSI to UNICODE.
        //

        result = MultiByteToWideChar(
                     CP_ACP,                                    // CodePage (ANSI)
                     0,                                         // dwFlags
                     AnsiProtocolInfo->szProtocol,              // lpMultiByteStr
                     -1,                                        // cchWideChar
                     UnicodeProtocolInfo->szProtocol,           // lpWideCharStr
                     sizeof(UnicodeProtocolInfo->szProtocol)    // cchMultiByte
                     );

        if( result == 0 ) {

            //
            // MultiByteToWideChar() failed.
            //

            return WSASYSCALLFAILURE;

        }

        //
        // Success!
        //

        return ERROR_SUCCESS;

    }
    __except (WS2_EXCEPTION_FILTER()) {
        return WSAEFAULT;
    }
}   // MapAnsiProtocolInfoToUnicode


VOID
ValidateCurrentCatalogName(
    HKEY RootKey,
    LPSTR ValueName,
    LPSTR ExpectedName
    )

/*++

Routine Description:

    This routine checks for consistency between the protocol or namespace
    catalog as stored in the registry and the catalog format expected by
    the current version of this DLL. There's no great magic here; this
    code assumes that the person updating the registry format will change
    the catalog to use a different catalog name (such as Protocol_Catalog9,
    Protocol_Catalog10, etc.). This assumption means we can validate the
    registry format by validating the *name* of the registry key used
    for this catalog.

    The following steps are performed:

        1.  Try to read 'ValueName' from the registry.

        2.  If it doesn't exist, cool. Just create the new value. This
            typically means we're updating a pre-release system that
            did not support this mechanism.

        3.  If it does, and its value matches 'ExpectedName', fine.

        4.  If it does, and its value doesn't match, then the catalog
            format has been updated, so blow away the old catalog, then
            write the updated value into the registry.

    Since this routine is called at setup/upgrade time, it should only
    fail if something truly horrible happens. In other words, it should
    be very 'fault tolerant'.

Arguments:

    RootKey - An open key to the WinSock configuration registry tree.

    ValueName - The name of the registry value that contains the name
        of the current catalog. This will typically be a value such as
        "Current_Protocol_Catalog" or "Current_NameSpace_Catalog".

    ExpectedName - The expected value stored in the 'ValueName' registry
        value. This will typically be a value such as "Protocol_Catalog9"
        or "NameSpace_Catalog5".

Return Value:

    None.

--*/

{

    BOOL result;
    LONG err;
    CHAR value[MAX_CATALOG_NAME_LENGTH];

    //
    // Try to read the name from the registry.
    //

    result = ReadRegistryEntry(
                 RootKey,
                 ValueName,
                 (PVOID)value,
                 sizeof(value),
                 REG_SZ
                 );

    if( result ) {

        if( lstrcmp( value, ExpectedName ) == 0 ) {

            //
            // No update in format. We're done.
            //

            return;

        }

        //
        // The values don't match, indicating an update in registry format.
        // So, blow away the old key.
        //

        err = RegDeleteKeyRecursive(
                  RootKey,
                  value
                  );

        if( err != NO_ERROR ) {

            //
            // Unfortunate, but nonfatal.
            //

            DEBUGF(
                DBG_ERR,
                ("Deleting key %s, continuing\n",
                value
                ));

        }

    }

    //
    // At this point, we either couldn't read the value from the registry
    // (probably indicating that we're upgrading a pre-release system
    // that was setup before we supported this particular feature) OR
    // the values don't match and we've just blown away the old catalog.
    // In either case we need to update the value in the registry before
    // returning.
    //

    result = WriteRegistryEntry(
                 RootKey,
                 ValueName,
                 ExpectedName,
                 REG_SZ
                 );

    if( !result ) {

        //
        // Also unfortunate, but nonfatal.
        //

        DEBUGF(
            DBG_ERR,
            ("Writing %s with value %s\n",
            ValueName,
            ExpectedName
            ));

    }

}   // ValidateCurrentCatalogName

INT
AcquireExclusiveCatalogAccess (
	IN	HKEY	CatalogKey,
	IN	DWORD	ExpectedSerialNum,
    OUT PHKEY   AccessKey
    )
/*++
Routine Description:

    This procedure acquires registry lock using volatile registry key.
    This ensures that only one application at a time can modify
    registry catalog.

Arguments:
	CatalogKey	-	Supplies catalog key to lock

	ExpectedSerialNum -  Supplies catalog serial number that caller
					expects to see in the registry. It validates
					that catalog has not changed since it was last read
                    by the client

	AccessKey	-	Returns handle to the registry key that is used
					for synchronization (to be passed back in
                    ReleaseExclusiveCatalogAccess)

Return Value:

    If  the  function  is  successful, it returns ERROR_SUCCESS.  Otherwise, it
    returns an appropriate WinSock error code:
		WSATRY_AGAIN	- catalog serial number in the registry does not
							match the one supplied
		WSAEACCESS		- caller does not have write access to the catalog portion
							of the registry
		WSASYSCALLFAILURE - one of the registry operation failed

--*/
{
    LONG        lresult;
	INT			return_code;
    BOOL        bresult;
    DWORD       serial_num, disposition;
    HKEY        access_key;
    TCHAR       serial_num_buffer[32];


	// Initialize return value
	*AccessKey = NULL;

	// Read current serial number
    bresult = ReadRegistryEntry (
                    CatalogKey,             // EntryKey
                    SERIAL_NUMBER_NAME,     // EntryName
                    (PVOID) &serial_num,    // Data
                    sizeof (DWORD),         // MaxBytes
                    REG_DWORD               // TypeFlag
                    ); 
    if (!bresult) {
        DEBUGF (DBG_ERR, ("Reading catalog serial number value.\n"));
        return WSASYSCALLFAILURE;
    }

	// Check if it what caller was expecting
    if (ExpectedSerialNum!=serial_num) {
        DEBUGF (DBG_ERR,
            ("Catalog serial number changed since we read it, %ld->%ld.\n",
            ExpectedSerialNum, serial_num));
        return WSATRY_AGAIN;
    }

	// Create synchronization key
    _stprintf (serial_num_buffer, TEXT("%08.8lX"), serial_num);
    lresult = RegCreateKeyEx (
                    CatalogKey,              // hKey
                    serial_num_buffer,      // lpSubKey
                    0,                      // dwReserved
                    NULL,                   // lpszClass
                    REG_OPTION_VOLATILE,    // fdwOptions
                    KEY_ALL_ACCESS,         // samDesired
                    NULL,                   // lpSecurityAttributes
                    &access_key,            // phkResult
                    &disposition            // lpdwDisposition
                    );
    if (lresult != ERROR_SUCCESS) {
        DEBUGF (DBG_ERR, ("Creating access key '%s', err: %ld.\n",
            serial_num_buffer));
        if (lresult == ERROR_ACCESS_DENIED)
            return WSAEACCES;
        else
            return WSASYSCALLFAILURE;
    }

    if (disposition==REG_CREATED_NEW_KEY) {
		// We created the key, so caller can have the registry to itself
		*AccessKey = access_key;
        return ERROR_SUCCESS;
    }
    else {
		// The key was there already, someone must be writing to the
		// registry and thus current callers representation of it
		// becomes invalid.
        RegCloseKey (access_key);
        DEBUGF (DBG_WARN, 
            ("Trying to lock accessed catalog, serial num: %ld.\n",
            serial_num));
        return WSATRY_AGAIN;
    }

} // AcquireExclusiveRegistryAccess

VOID
ReleaseExclusiveCatalogAccess (
	IN	HKEY	CatalogKey,
	IN  DWORD	CurrentSerialNum,
    IN  HKEY    access_key
    )
/*++
Routine Description:

    This procedure releases registry lock acquired using
    AcuireExclusiveCatalogAccess.

Arguments:
	CatalogKey	-	Supplies catalog key to lock

	CurrentSerialNum -  Supplies catalog serial number which was in
					effect when catalog was locked.

	AccessKey	-	Supplise handle to the registry key that was used
					for synchronization.

Return Value:

    None
--*/
{
    LONG        lresult;
    BOOL        bresult;
    TCHAR       serial_num_buffer[32];

	// Save and increment catalog serial number
    _stprintf (serial_num_buffer, TEXT("%08.8lX"), CurrentSerialNum);
	
	CurrentSerialNum += 1;

    // Store new catalog serial number
    bresult = WriteRegistryEntry (
                    CatalogKey,					// EntryKey
                    SERIAL_NUMBER_NAME,			// EntryName
                    (PVOID)&CurrentSerialNum,	// Data
                    REG_DWORD					// TypeFlag
                    );
    if (!bresult) {
        DEBUGF (DBG_ERR,
            ("Writing serial number value %ld.\n", CurrentSerialNum));
        assert (FALSE);
        //
        // Nothing we can do, writer has done its job anyway
        //
    }

    lresult = RegDeleteKey (CatalogKey, serial_num_buffer);
    if (lresult != ERROR_SUCCESS) {
        DEBUGF (DBG_ERR,
            ("Deleting serial access key '%s', err: %ld.\n",
			serial_num_buffer, lresult));
        //
        // Unfortunate but not fatal (just leaves it in the regstry);
        //
    }

    lresult = RegCloseKey (access_key);
    if (lresult != ERROR_SUCCESS) {
        DEBUGF (DBG_ERR,
            ("Closing serial access key '%s', err: %ld.\n", 
			serial_num_buffer, lresult));
        //
        // Unfortunate but not fatal (does not deallocate memory
        // and possibly leaves it in the regstry);
        //
    }

} //ReleaseExclusiveRegistryAccess


#define MAX_WRITER_WAIT_TIME    (3*60*1000)
INT
SynchronizeSharedCatalogAccess (
	IN	HKEY	CatalogKey,
	IN	HANDLE	ChangeEvent,
	OUT	LPDWORD	CurrentSerialNum
	)
/*++
Routine Description:

    This procedure synchronizes reades access to the registry
    catalog against possible writers.  It waits for any writers
    that are accessing the catalog at the time of the call
    and establishes event notification mechanism for any registry
    catalog modification afterwards

Arguments:
	CatalogKey	-	Supplies catalog key to synchronize with

    ChangeEvent -   Supplies event to signal when registry catalog 
                    is changed.

    CurrentSerialNumber - Returns current catalog serial number

Return Value:
    If  the  function  is  successful, it returns ERROR_SUCCESS.  Otherwise, it
    returns an appropriate WinSock error code:

    
--*/
{
	LONG	lresult;
	INT		return_value;
	BOOL	bresult;
	DWORD	serial_num;
    TCHAR   serial_num_buffer[32];
    HKEY    access_key;

    do {
        //
        // Register for notification of key creation/deletion
        // (The writer creates and keeps access key while it
        // modifies the catalog)
        //

        lresult = RegNotifyChangeKeyValue (
                    CatalogKey,                 // hKey
                    FALSE,                      // bWatchSubtree
                    REG_NOTIFY_CHANGE_NAME,     // dwNotifyFilter,
                    ChangeEvent,                // hEvent
                    TRUE                        // fAsynchronous
                    );
        if (lresult != ERROR_SUCCESS) {
            DEBUGF (DBG_ERR,
                ("Registering for registry key change notification, err: %ld.\n",
                    lresult));
            return_value = WSASYSCALLFAILURE;
            break;
        }

        // Read current catalog serial number, which is also
        // the name of the writer access key
        bresult = ReadRegistryEntry (
                        CatalogKey,             // EntryKey
                        SERIAL_NUMBER_NAME,     // EntryName
                        (PVOID) &serial_num,    // Data
                        sizeof (DWORD),         // MaxBytes
                        REG_DWORD               // TypeFlag
                        ); 
        if (!bresult) {
            DEBUGF (DBG_ERR, ("Reading '%s' value.\n", SERIAL_NUMBER_NAME));
            return_value = WSASYSCALLFAILURE;
            break;
        }

        // Try to open writer access key.
        _stprintf (serial_num_buffer, TEXT("%08.8lX"), serial_num);
        lresult = RegOpenKeyEx(
                        CatalogKey,             // hkey
                        serial_num_buffer,      // lpszSubKey
                        0,                      // dwReserved
                        MAXIMUM_ALLOWED,        // samDesired
                        & access_key            // phkResult
                        );
        if ((lresult == ERROR_FILE_NOT_FOUND)
                || (lresult == ERROR_KEY_DELETED)) {
			// Key was not found or is being deleted,
			// we can access the catalog
            return_value = ERROR_SUCCESS;
			*CurrentSerialNum = serial_num;
            break;
		}
        else if (lresult != ERROR_SUCCESS) {
			// Some other failure
            DEBUGF (DBG_ERR,
                ("Opening access key '%s', err: %ld.\n", 
                serial_num_buffer, lresult));
			return_value = WSASYSCALLFAILURE;
			break;
		}

        // Success, writer is active, close the key,
        // wait till it gets removed, and start over again
        lresult = RegCloseKey (access_key);
        if (lresult!=ERROR_SUCCESS) {
            DEBUGF (DBG_ERR,
                ("Closing access key '%ls', err: %ld.\n", 
                serial_num_buffer, lresult));
            // Non-fatal.
        }
        // Set the error code in case we fail the wait.
        return_value = WSANO_RECOVERY;
        // Limit the wait time in case writer crashed or
        // failed to remove the key.
	}
	while (WaitForSingleObject (ChangeEvent, MAX_WRITER_WAIT_TIME)==WAIT_OBJECT_0);

	return return_value;
}

BOOL
HasCatalogChanged (
	IN	HANDLE	ChangeEvent
	)
/*++
Routine Description:

    This procedure checks if registry catalog has changes since the
    caller has last synchronized with it
Arguments:
    ChangeEvent -   Event used for catalog syncrhonization.

Return Value:
    TRUE    - catalog has changed, FALSE otherwise
    
--*/
{
	DWORD	wait_result;

    // Simply check the event state
	wait_result = WaitForSingleObject (ChangeEvent, 0);
	if (wait_result==WAIT_OBJECT_0)
		return TRUE;
	if (wait_result==WAIT_TIMEOUT)
		return FALSE;

	DEBUGF (DBG_ERR, ("Waiting for registry change event, rc=%ld, err=%ld.\n",
				wait_result, GetLastError ()));
	assert (FALSE);
	return FALSE;
}



extern "C" {

VOID
WEP( VOID )
{
    // empty
}   // WEP

}   // extern "C"

void * __cdecl operator new(size_t sz) {
    return HeapAlloc (gHeap, 0, sz);
}

void __cdecl operator delete(void *p) {
    HeapFree (gHeap, 0, p);
}