/*++



Copyright (c) 1991  Microsoft Corporation

Module Name:

    Regkey.c

Abstract:

    This module contains the server side implementation for the Win32
    Registry APIs to open, create, flush and close keys.  That is:

        - BaseRegCloseKey
        - BaseRegCreateKey
        - BaseRegFlushKey
        - BaseRegOpenKey

Author:

    David J. Gilman (davegi) 15-Nov-1991

Notes:

    These notes apply to the Win32 Registry API implementation as a whole
    and not just to this module.

    On the client side, modules contain RPC wrappers for both the new
    Win32 and compatible Win 3.1 APIs.  The Win 3.1 wrappers generally
    supply default parameters before calling the Win32 wrappers.  In some
    cases they may need to call multiple Win32 wrappers in order to
    function correctly (e.g.  RegSetValue sometimes needs to call
    RegCreateKeyEx).  The Win32 wrappers are quite thin and usually do
    nothing more than map a predefined handle to a real handle and perform
    ANSI<->Unicode translations.  In some cases (e.g.  RegCreateKeyEx) the
    wrapper also converts some argument (e.g.  SECURITY_ATTRIBUTES) to an
    RPCable representation.  In both the Win 3.1 and Win32 cases ANSI and
    Unicode implementations are provided.

    On the server side, there is one entry point for each of the Win32
    APIs.  Each contains an identical interface with the client side
    wrapper with the exception that all string / count arguments are
    passed as a single counted Unicode string.  Pictorially, for an API
    named "F":

                RegWin31FA()          RegWin31FW()      (client side)

                    |                     |
                    |                     |
                    |                     |
                    |                     |
                    V                     V

                RegWin32FExA()        RegWin32FExW()

                    |                     |
                    ^                     ^
                    v                     v             (RPC)
                    |                     |
                    |                     |
                    +----> BaseRegF() <---+             (server side)


    This yields smaller code (as the string conversion is done only once
    per API) at the cost of slightly higher maintenance (i.e. Win 3.1
    default parameter replacement and Win32 string conversions must be
    manually kept in synch).

    Another option would be to have a calling sequence that looks like,

                RegWin31FA()          RegWin31FW()

                    |                     |
                    |                     |
                    |                     |
                    V                     V

                RegWin32FExA() -----> RegWin32FExW()

    and have the RegWin32FExW() API perform all of the actual work.  This
    method is generally less efficient.  It requires the RegWin32FExA()
    API to convert its ANSI string arguments to counted Unicode strings,
    extract the buffers to call the RegWin32FExW() API only to have it
    rebuild a counted Unicode string.  However in some cases (e.g.
    RegConnectRegistry) where a counted Unicode string was not needed in
    the Unicode API this method is used.

    Details of an API's functionality, arguments and return value can be
    found in the base implementations (e.g.  BaseRegF()).  All other
    function headers contain only minimal routine descriptions and no
    descriptions of their arguments or return value.

    The comment string "Win3.1ism" indicates special code for Win 3.1
    compatability.

    Throughout the implementation the following variable names are used
    and always refer to the same thing:

        Obja        - An OBJECT_ATTRIBUTES structure.
        Status      - A NTSTATUS value.
        Error       - A Win32 Registry error code (n.b. one of the error
                      values is ERROR_SUCCESS).

--*/

#include <rpc.h>
#include <string.h>
#include <wchar.h>
#include "regrpc.h"
#include "localreg.h"
#include "regclass.h"
#include "regecls.h"
#include "regsec.h"
#include <malloc.h>

#ifdef LOCAL
#include "tsappcmp.h"

#ifdef LEAK_TRACK
#include "regleak.h"
#endif // LEAK_TRACK

HKEY MapRestrictedKey(HKEY hKey, UNICODE_STRING *lpSubKey, UNICODE_STRING *NewSubKey);
extern HKEY HKEY_RestrictedSite;

#endif

NTSTATUS
BaseRegCreateMultipartKey(
    IN HKEY hkDestKey,
    IN PUNICODE_STRING pDestSubKey,
    IN PUNICODE_STRING lpClass OPTIONAL,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN PRPC_SECURITY_ATTRIBUTES pRpcSecurityAttributes OPTIONAL,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition OPTIONAL,
    ULONG             Attributes);


BOOL
InitializeRegCreateKey(
    )

/*++

Routine Description:

    This function was used to initialize a critical section that no longer
    exists. This critical section was used when a key name '\', and multiple
    multiple keys were to be created. The API used the wcstok defined in the
    kernel, which was not multi-threaded safe.

    This function now will always return TRUE. It will not be removed from the code
    to avoid change in the rpc interface.

Arguments:

    None.

Return Value:

    Returns TRUE always.

--*/

{
    return( TRUE );

}



BOOL
CleanupRegCreateKey(
    )

/*++

Routine Description:

    This function was used to clean up a critical section that no longer
    exists. This critical section was used when a key name '\', and multiple
    multiple keys were to be created. The API used the wcstok defined in the
    kernel, which was not multi-threaded safe.

    This function now will always return TRUE. It will not be removed from the code
    to avoid change in the rpc interface.



Arguments:

    None.

Return Value:

    Returns TRUE if the cleanup succeeds.

--*/

{
    return( TRUE );
}



error_status_t
BaseRegCloseKeyInternal(
    IN OUT PHKEY phKey
    )

/*++

Routine Description:

    Closes a key handle.

Arguments:

    phKey - Supplies a handle to an open key to be closed.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    NTSTATUS Status;
#if defined(LEAK_TRACK) 
    BOOL fTrack;
#endif // defined(LEAK_TRACK) 

    //
    // Call out to Perflib if the HKEY is HKEY_PERFOMANCE_DATA.
    //

    if(( *phKey == HKEY_PERFORMANCE_DATA ) ||
       ( *phKey == HKEY_PERFORMANCE_TEXT ) ||
       ( *phKey == HKEY_PERFORMANCE_NLSTEXT )) {

        Status = PerfRegCloseKey( phKey );
        RPC_REVERT_TO_SELF();
        return (error_status_t)Status;
    }

    ASSERT( IsPredefinedRegistryHandle( *phKey ) == FALSE );

#ifdef LOCAL
    //
    // now we need to remove any state for registry key enumeration associated
    // with this key if it's a class registration parent
    //
    if (REG_CLASS_IS_SPECIAL_KEY(*phKey)) {

        // this may not succeed since someone could have already removed this key
        (void) EnumTableRemoveKey(
            &gClassesEnumTable,
            *phKey,
            ENUM_TABLE_REMOVEKEY_CRITERIA_ANYTHREAD);
    }

#if defined(LEAK_TRACK) 

    if (g_RegLeakTraceInfo.bEnableLeakTrack) {
        fTrack = RegLeakTableIsTrackedObject(&gLeakTable, *phKey);
    }

#endif // defined(LEAK_TRACK)

#endif // LOCAL


    Status = NtClose( *phKey );

    if( NT_SUCCESS( Status )) {

#ifdef LOCAL
#if defined(LEAK_TRACK)

        if (g_RegLeakTraceInfo.bEnableLeakTrack) {
            if (fTrack) {
                (void) UnTrackObject(*phKey);
            }
        }
        
#endif // defined(LEAK_TRACK)
#endif // LOCAL

        //
        // Set the handle to NULL so that RPC knows that it has been closed.
        //
        *phKey = NULL;

        return ERROR_SUCCESS;

    } else {

        return (error_status_t)RtlNtStatusToDosError( Status );
    }
}



error_status_t
BaseRegCloseKey(
    IN OUT PHKEY phKey
    )

/*++

Routine Description:

    Closes a key handle.

Arguments:

    phKey - Supplies a handle to an open key to be closed.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    error_status_t Error;

    RPC_IMPERSONATE_CLIENT(NULL);

    Error = BaseRegCloseKeyInternal(phKey);

    RPC_REVERT_TO_SELF();

    return Error;
}



error_status_t
BaseRegCreateKey(
    IN HKEY hKey,
    IN PUNICODE_STRING lpSubKey,
    IN PUNICODE_STRING lpClass OPTIONAL,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN PRPC_SECURITY_ATTRIBUTES pRpcSecurityAttributes OPTIONAL,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition OPTIONAL
    )

/*++

Routine Description:

    Create a new key, with the specified name, or open an already existing
    key.  RegCreateKeyExW is atomic, meaning that one can use it to create
    a key as a lock.  If a second caller creates the same key, the call
    will return a value that says whether the key already existed or not,
    and thus whether the caller "owns" the "lock" or not.  RegCreateKeyExW
    does NOT truncate an existing entry, so the lock entry may contain
    data.

Arguments:

    hKey - Supplies a handle to an open key.  The lpSubKey key path
        parameter is relative to this key handle.  Any of the predefined
        reserved handle values or a previously opened key handle may be used
        for hKey.

    lpSubKey - Supplies the downward key path to the key to create.
        lpSubKey is always relative to the key specified by hKey.
        This parameter may not be NULL.

    lpClass - Supplies the class (object type) of this key.  Ignored if
        the key already exists.  No class is associated with this key if
        this parameter is NULL.

    dwOptions - Supplies special options.  Only one is currently defined:

        REG_VOLATILE -  Specifies that this key should not be preserved
            across reboot.  The default is not volatile.  This is ignored
            if the key already exists.

        WARNING: All descendent keys of a volatile key are also volatile.

    samDesired - Supplies the requested security access mask.  This
        access mask describes the desired security access to the newly
        created key.

    lpSecurityAttributes - Supplies a pointer to a SECURITY_ATTRIBUTES
        structure for the newly created key. This parameter is ignored
        if NULL or not supported by the OS.

    phkResult - Returns an open handle to the newly created key.

    lpdwDisposition - Returns the disposition state, which can be one of:

            REG_CREATED_NEW_KEY - the key did not exist and was created.

            REG_OPENED_EXISTING_KEY - the key already existed, and was simply
                opened without being changed.

        This parameter is ignored if NULL.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

    If successful, RegCreateKeyEx creates the new key (or opens the key if
    it already exists), and returns an open handle to the newly created
    key in phkResult.  Newly created keys have no value; RegSetValue, or
    RegSetValueEx must be called to set values.  hKey must have been
    opened for KEY_CREATE_SUB_KEY access.

--*/

{
    OBJECT_ATTRIBUTES   Obja;
    ULONG               Attributes;
    NTSTATUS            Status;
#if DBG
    HANDLE              DebugKey = hKey;
#endif
    HKEY                hkDestKey;
    UNICODE_STRING      DestClassSubkey;
    PUNICODE_STRING     pDestSubkey;
    DWORD               dwDisposition;
    BOOL                fRetryOnAccessDenied;
    BOOL                fRetried;
    BOOL                fTrySingleCreate;
#if LOCAL
    SKeySemantics       keyinfo;
    BYTE                rgNameInfoBuf[REG_MAX_CLASSKEY_LEN];
    REGSAM              OriginalSam = samDesired;

    memset(&keyinfo, 0, sizeof(keyinfo));
#endif

    ASSERT( IsPredefinedRegistryHandle( hKey ) == FALSE );
    ASSERT( lpSubKey->Length > 0 );

    DestClassSubkey.Buffer = NULL;

    //
    // For class registrations, retry on access denied in machine hive --
    // if we do retry, this will be set to FALSE so we only retry once
    //
    fRetryOnAccessDenied = TRUE;
    fRetried = FALSE;

    //
    // First attempt should do create with a single ntcreatekey call
    // If that doesn't work, this gets set to false so we remember if we
    // have to retry for access denied in the machine hive
    //
    fTrySingleCreate = TRUE;

    hkDestKey = NULL;
    pDestSubkey = NULL;
    

    //
    // Quick check for a "restricted" handle
    //

    if ( REGSEC_CHECK_HANDLE( hKey ) )
    {
        if ( ! REGSEC_CHECK_PATH( hKey, lpSubKey ) )
        {
            return( ERROR_ACCESS_DENIED );
        }

        hKey = (HANDLE) REGSEC_CLEAR_HANDLE( hKey );
    }

    //
    // Check for malformed arguments from malicious clients
    //

    if ((lpSubKey->Length < sizeof(UNICODE_NULL)) ||
        (lpSubKey->Buffer == NULL) ||
        ((lpSubKey->Length % sizeof(WCHAR)) != 0) ||
        (lpSubKey->Buffer[lpSubKey->Length / sizeof(WCHAR) - 1] != L'\0')) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Impersonate the client.
    //

    RPC_IMPERSONATE_CLIENT( NULL );

    //
    //  Initialize the variable that will contain the handle to NULL
    //  to ensure that in case of error the API will not return a
    //  bogus handle. This is required otherwise RPC will get confused.
    //  Note that RPC should have already initialized it to 0.
    //
    *phkResult = NULL;

    //
    //  Subtract the NULLs from the Length of the provided strings.
    //  These were added on the client side so that the NULLs were
    //  transmitted by RPC.
    //
    lpSubKey->Length -= sizeof( UNICODE_NULL );

    if( lpSubKey->Buffer[0] == ( WCHAR )'\\' ) {
        //
        // Do not accept a key name that starts with '\', even though
        // the code below would handle it. This is to ensure that
        // RegCreateKeyEx and RegOpenKeyEx will behave in the same way
        // when they get a key name that starts with '\'.
        //
        Status = STATUS_OBJECT_PATH_INVALID;
        goto cleanup;
    }

    if ( lpClass->Length > 0 ) {
        lpClass->Length -= sizeof( UNICODE_NULL );
    }

    //
    // Determine the correct set of attributes.
    //

    Attributes = OBJ_CASE_INSENSITIVE;

    if( ARGUMENT_PRESENT( pRpcSecurityAttributes )) {

        if( pRpcSecurityAttributes->bInheritHandle ) {

            Attributes |= OBJ_INHERIT;
        }
    }

    if (dwOptions & REG_OPTION_OPEN_LINK) {
        Attributes |= OBJ_OPENLINK;
    } 

#ifdef LOCAL
    if (REG_CLASS_IS_SPECIAL_KEY(hKey)) {

        // 
        // Find more information 
        // about this key -- the most important piece of information
        // is whether it's a class registration key
        //
        keyinfo._pFullPath = (PKEY_NAME_INFORMATION) rgNameInfoBuf;
        keyinfo._cbFullPath = sizeof(rgNameInfoBuf);

        //
        // see if this is a class registration
        //
        Status = BaseRegGetKeySemantics(hKey, lpSubKey, &keyinfo);

        // if we can't determine what type of key this is, leave
        if (!NT_SUCCESS(Status)) {
            goto cleanup;
        }

        Status = BaseRegMapClassRegistrationKey(
            hKey,
            lpSubKey,
            &keyinfo,
            &DestClassSubkey,
            &fRetryOnAccessDenied,
            &hkDestKey,
            &pDestSubkey);

        if (!NT_SUCCESS(Status)) {
            goto cleanup;
        }

    } else
#endif // LOCAL
    {
#ifdef LOCAL
        memset(&keyinfo, 0, sizeof(keyinfo));
#endif // LOCAL

        hkDestKey = hKey;
        pDestSubkey = lpSubKey;
    }


    for (;;) {
        //
        // If this is a restricted process skip the initial create so that logic
        // below can handle remapping
        //

#ifdef LOCAL

        Status = STATUS_OBJECT_NAME_NOT_FOUND;

        if (!HKEY_RestrictedSite && fTrySingleCreate)
        {
#endif
            //
            // Try to create the specified key. This will work if there is only
            // one key being created or if the key already exists. If more than
            // one key needs to be created, this will fail and we will have to
            // do all the complicated stuff to create each intermediate key.
            //
            InitializeObjectAttributes(&Obja,
                                       pDestSubkey,
                                       Attributes,
                                       hkDestKey,
                                       ARGUMENT_PRESENT( pRpcSecurityAttributes )
                                       ? pRpcSecurityAttributes
                                       ->RpcSecurityDescriptor.lpSecurityDescriptor
                                       : NULL);
            Status = NtCreateKey(phkResult,
                                 samDesired,
                                 &Obja,
                                 0,
                                 lpClass,
                                 dwOptions,
                                 &dwDisposition);


#ifdef LOCAL
            if (gpfnTermsrvCreateRegEntry && NT_SUCCESS(Status) && (dwDisposition == REG_CREATED_NEW_KEY)) {
                //
                // Terminal Server application compatiblity
                // Store the newly created key in the Terminal Server registry tracking database
                //
                
                gpfnTermsrvCreateRegEntry(*phkResult,
                                          &Obja,
                                          0,
                                          lpClass,
                                          dwOptions);
            }

        }

#endif

#ifdef LOCAL
#ifdef CLASSES_RETRY_ON_ACCESS_DENIED
        if (fTrySingleCreate && (STATUS_ACCESS_DENIED == Status) && keyinfo._fCombinedClasses &&
            fRetryOnAccessDenied) {

            Status = BaseRegMapClassOnAccessDenied(
                &keyinfo,
                &hkDestKey,
                pDestSubkey,
                &fRetryOnAccessDenied);

            if (NT_SUCCESS(Status)) {
                fRetried = TRUE;
                continue;
            }

            // we failed for some reason -- exit
            break;
        }
#endif // CLASSES_RETRY_ON_ACCESS_DENIED
#endif // LOCAL

        fTrySingleCreate = FALSE;

        if (NT_SUCCESS(Status)) {

            if (lpdwDisposition) {
                *lpdwDisposition = dwDisposition;
            }

        } else {

            Status = BaseRegCreateMultipartKey(
                hkDestKey,
                pDestSubkey,
                lpClass,
                dwOptions,
                samDesired,
                pRpcSecurityAttributes,
                phkResult,
                lpdwDisposition,
                Attributes);
        }
    
#ifdef LOCAL
#ifdef CLASSES_RETRY_ON_ACCESS_DENIED
        if ((STATUS_ACCESS_DENIED == Status) && keyinfo._fCombinedClasses &&
            fRetryOnAccessDenied) {

            Status = BaseRegMapClassOnAccessDenied(
                &keyinfo,
                &hkDestKey,
                pDestSubkey,
                &fRetryOnAccessDenied);

            if (NT_SUCCESS(Status)) {
                fRetried = TRUE;
                continue;
            }

            break;
        }
#endif //  CLASSES_RETRY_ON_ACCESS_DENIED

        if (NT_SUCCESS(Status) && !HKEY_RestrictedSite) {
            if (keyinfo._fCombinedClasses) {
                // mark this key as part of hkcr
                *phkResult = REG_CLASS_SET_SPECIAL_KEY(*phkResult);
            }
        }

#endif // LOCAL      

        break;
    }

cleanup:

#ifdef CLASSES_RETRY_ON_ACCESS_DENIED
    //
    // Memory was allocated if we retried, so free it
    //
    if (fRetried && pDestSubkey->Buffer) {
        RtlFreeHeap(RtlProcessHeap(), 0, pDestSubkey->Buffer);
        pDestSubkey->Buffer = NULL;
    }
#endif // CLASSES_RETRY_ON_ACCESS_DENIED

    if (hkDestKey && (hkDestKey != hKey)) {
        NtClose(hkDestKey);
    }

#ifdef LOCAL
    if (DestClassSubkey.Buffer) {
        RegClassHeapFree(DestClassSubkey.Buffer);
    }

    BaseRegReleaseKeySemantics(&keyinfo);

#endif // LOCAL

    if (NT_SUCCESS(Status)) {
#ifdef LOCAL
#if defined(LEAK_TRACK)

        if (g_RegLeakTraceInfo.bEnableLeakTrack) {
            (void) TrackObject(*phkResult);
        }
        
#endif // defined(LEAK_TRACK)       
#endif LOCAL
        ASSERT( *phkResult != DebugKey );
    }

    RPC_REVERT_TO_SELF();
    return (error_status_t)RtlNtStatusToDosError( Status );

}



NTSTATUS
BaseRegCreateMultipartKey(
    IN HKEY hkDestKey,
    IN PUNICODE_STRING pDestSubKey,
    IN PUNICODE_STRING lpClass OPTIONAL,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN PRPC_SECURITY_ATTRIBUTES pRpcSecurityAttributes OPTIONAL,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition OPTIONAL,
    ULONG             Attributes)
/*++

Routine Description:

    This function creates registry keys for which multiple path components
    are nonexistent.  It parses the key path and creates each intermediate 
    subkey.

Arguments:

    See BaseRegCreateKey.

Return Value:

    Returns STATUS_SUCCESS on success, other NTSTATUS if failed.

--*/
{
    LPWSTR            KeyBuffer;
    ULONG             NumberOfSubKeys;
    LPWSTR            p;
    ULONG             i;
    LPWSTR            Token;
    UNICODE_STRING    KeyName;
    HANDLE            TempHandle1;
    HANDLE            TempHandle2;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS          Status;
    DWORD             dwDisposition;
#ifdef LOCAL
    REGSAM            OriginalSam = samDesired;
#endif // LOCAL

    dwDisposition = REG_OPENED_EXISTING_KEY;
    TempHandle1 = NULL;

    //
    // Win3.1ism - Loop through each '\' separated component in the
    // supplied sub key and create a key for each component. This is
    // guaranteed to work at least once because lpSubKey was validated
    // on the client side.
    //


    //
    // Initialize the buffer to be tokenized.
    //

    KeyBuffer = pDestSubKey->Buffer;

    //
    //  Find out the number of subkeys to be created
    //
    NumberOfSubKeys = 1;
    p = KeyBuffer;
    while ( ( p = wcschr( p, ( WCHAR )'\\' ) ) != NULL ) {
        p++;
        NumberOfSubKeys++;
    }

    for( i = 0, Token = KeyBuffer; i < NumberOfSubKeys; i++ ) {

        ASSERT(Token != NULL);

        if( ( *Token == ( WCHAR )'\\' ) &&
            ( i != NumberOfSubKeys - 1 ) ) {
            //
            //  If the first character of the key name is '\', and the key
            //  is not the last to be created, then ignore this key name.
            //  This condition can happen if the key name contains
            //  consecutive '\'.
            //  This behavior is consistent with the one we had in the past
            //  when the API used wcstok() to get the key names.
            //  Note that if the key name is an empty string, we return a handle
            //  that is different than hKey, even though both point to the same
            //  key. This is by design.
            //
            Token++;
            continue;
        }

        //
        // Convert the token to a counted Unicode string.
        //
        KeyName.Buffer = Token;
        if (i == NumberOfSubKeys - 1) {
            KeyName.Length = wcslen(Token)*sizeof(WCHAR);
        } else {
            KeyName.Length = (USHORT)(wcschr(Token, ( WCHAR )'\\') - Token)*sizeof(WCHAR);
        }

        //
        // Remember the intermediate handle (NULL the first time through).
        //

        TempHandle2 = TempHandle1;

// local-only check for restricted

#ifdef LOCAL
        //
        // Try to open the restricted version of the key first
        //

        samDesired = OriginalSam;
        Status = STATUS_OBJECT_NAME_NOT_FOUND;

        if (HKEY_RestrictedSite)
        {
            UNICODE_STRING  NewSubKey;
            HKEY            MappedKey;
            
            MappedKey = MapRestrictedKey(hkDestKey,&KeyName,&NewSubKey);

            if (NULL != MappedKey)
            {
                NTSTATUS            Status2;
                OBJECT_ATTRIBUTES   ObjAttr;
                
                InitializeObjectAttributes(
                    &ObjAttr,
                    &NewSubKey,
                    Attributes,
                    MappedKey,
                    ARGUMENT_PRESENT( pRpcSecurityAttributes )
                    ? pRpcSecurityAttributes
                    ->RpcSecurityDescriptor.lpSecurityDescriptor
                    : NULL
                    );

                Status2 = NtCreateKey(
                    &TempHandle1,
                    ( i == NumberOfSubKeys - 1 )
                    ? samDesired
                    : MAXIMUM_ALLOWED,
                    &ObjAttr,
                    0,
                    lpClass,
                    dwOptions,
                    &dwDisposition
                    );

                if (NT_SUCCESS(Status) && lpdwDisposition) {
                    *lpdwDisposition = dwDisposition;
                }


                Status = (NT_SUCCESS(Status2)
                          || STATUS_OBJECT_NAME_NOT_FOUND == Status)
                    ? Status2
                    : Status;

                RtlFreeUnicodeString(&NewSubKey);
                NtClose(MappedKey);
            }
            else if (NULL != NewSubKey.Buffer)
            {
                //
                // The key is a class registration parent key (e.g. CLSID)
                // We need to always return the global version of these
                // keys to ensure global data under them is accessable.
                // Munge the samDesired to ensure that the key can be
                // opened.
                //
                samDesired = MAXIMUM_ALLOWED;
            }
        }

        if (!NT_SUCCESS(Status))
#endif // local-only check for restricted
        {
            //
            // Initialize the OBJECT_ATTRIBUTES structure, close the
            // intermediate key and create or open the key.
            //

            InitializeObjectAttributes(
                &Obja,
                &KeyName,
                Attributes,
                hkDestKey,
                ARGUMENT_PRESENT( pRpcSecurityAttributes )
                ? pRpcSecurityAttributes
                ->RpcSecurityDescriptor.lpSecurityDescriptor
                : NULL
                );
            
            Status = NtCreateKey(
                &TempHandle1,
                ( i == NumberOfSubKeys - 1 )? samDesired : MAXIMUM_ALLOWED,
                &Obja,
                0,
                lpClass,
                dwOptions,
                &dwDisposition
                );

            if (NT_SUCCESS(Status) && lpdwDisposition) {
                *lpdwDisposition = dwDisposition;
            }
            
#ifdef LOCAL          
            // This code is in Hydra 4. We have disabled this for NT 5
            // for now till we are sure that its needed to get some imporatant
            // app to work on Hydra 5. Otherwise this should be removed
            if (IsTerminalServer()) {

                // For Terminal Server only.
                // Some apps try to create/open the key with all of the access bits
                // turned on.  We'll mask off the ones they don't have access to by
                // default, (at least under HKEY_LOCAL_MACHINE\Software) and try to 
                // open the key again.
                if (Status == STATUS_ACCESS_DENIED) {
                    
                    Status = NtCreateKey(
                        &TempHandle1,
                        (samDesired & 
                         ~(WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK)),
                        &Obja,
                        0,
                        lpClass,
                        dwOptions,
                        &dwDisposition);
                
                    // Give app back the original error
                    if (!NT_SUCCESS(Status)) {
                        Status = STATUS_ACCESS_DENIED;
                    }

                    if (lpdwDisposition) {
                         *lpdwDisposition = dwDisposition;
                    }
                }
            }

            
            if (gpfnTermsrvCreateRegEntry && NT_SUCCESS(Status) && (dwDisposition == REG_CREATED_NEW_KEY)) {

                //
                // Terminal Server application compatiblity
                // Store the newly created key in the Terminal Server registry tracking database
                //
                gpfnTermsrvCreateRegEntry(TempHandle1,
                                          &Obja,
                                          0,
                                          lpClass,
                                          dwOptions);
            }
#endif
        }

        //
        // Initialize the next object directory (i.e. parent key) handle.
        //

        hkDestKey = TempHandle1;
        
        //
        // Close the intermediate key.
        // This fails the first time through the loop since the
        // handle is NULL.
        //

        if( TempHandle2 != NULL ) {
            NtClose( TempHandle2 );
        }
        
        //
        // If creating the key failed, map and return the error.
        //

        if( ! NT_SUCCESS( Status )) {
            return Status;
        }

        Token = wcschr( Token, ( WCHAR )'\\') + 1;

    }

    //
    // Only set the return value once we know we've
    // succeeded.
    //
    *phkResult = hkDestKey;

    return STATUS_SUCCESS;
}



error_status_t
BaseRegFlushKey(
    IN HKEY hKey
    )

/*++

Routine Description:

    Flush changes to backing store.  Flush will not return until the data
    has been written to backing store.  It will flush all the attributes
    of a single key.  Closing a key without flushing it will NOT abort
    changes.

Arguments:

    hKey - Supplies a handle to the open key.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

    If successful, RegFlushKey will flush to backing store any changes
    made to the key.

Notes:

    RegFlushKey may also flush other data in the Registry, and therefore
    can be expensive, it should not be called gratuitously.

--*/

{
    if ((hKey == HKEY_PERFORMANCE_DATA) ||
        (hKey == HKEY_PERFORMANCE_TEXT) ||
        (hKey == HKEY_PERFORMANCE_NLSTEXT)) {
        return(ERROR_SUCCESS);
    }

    ASSERT( IsPredefinedRegistryHandle( hKey ) == FALSE );


    //
    // Call the Nt Api to flush the key, map the NTSTATUS code to a
    // Win32 Registry error code and return.
    //

    return (error_status_t)RtlNtStatusToDosError( NtFlushKey( hKey ));
}

error_status_t
BaseRegOpenKey(
    IN HKEY hKey,
    IN PUNICODE_STRING lpSubKey,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )

/*++

Routine Description:

    Open a key for access, returning a handle to the key.  If the key is
    not present, it is not created (see RegCreateKeyExW).

Arguments:

    hKey - Supplies a handle to an open key.  The lpSubKey pathname
        parameter is relative to this key handle.  Any of the predefined
        reserved handle values or a previously opened key handle may be used
        for hKey.  NULL is not permitted.

    lpSubKey - Supplies the downward key path to the key to open.
        lpSubKey is always relative to the key specified by hKey.

    dwOptions -- reserved.

    samDesired -- This access mask describes the desired security access
        for the key.

    phkResult -- Returns the handle to the newly opened key.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

    If successful, RegOpenKeyEx will return the handle to the newly opened
    key in phkResult.

--*/

{
    OBJECT_ATTRIBUTES   Obja;
    NTSTATUS            Status = STATUS_OBJECT_NAME_NOT_FOUND;
    error_status_t      ret = ERROR_SUCCESS;
#ifdef LOCAL
    HKEY                MappedKey = NULL;
#endif

    UNREFERENCED_PARAMETER( dwOptions );

    ASSERT( IsPredefinedRegistryHandle( hKey ) == FALSE );

    //
    // Need to NULL this out param for compat with NT4, even though SDK
    // does not define this out param on api failure -- bad apps were written
    // which rely on this.  Used to get NULLed by call to NtOpenKey, but since
    // we don't always call that now, we need to do this here in user mode.  Also
    // need an exception wrapper since NtOpenKey would simply return an error if
    // the pointer were invalid, whereas in user mode we access violate if we simply
    // assign -- yet another fix needed for app compatibility as some apps on NT 4
    // were actually passing in a bad pointer and ignoring the error returned
    // by the api as part of their normal operation.
    //

    __try {

        *phkResult = NULL;

    } __except ( EXCEPTION_EXECUTE_HANDLER ) {

        Status = GetExceptionCode();

#if DBG
        DbgPrint( "WINREG Error: Exception %x in BaseRegOpenKey\n",
                  Status );
#endif
        ret = RtlNtStatusToDosError( Status );
    }

    //
    // This will only be true if there was an exception above --
    // return the exception code as an error
    //

    if (ERROR_SUCCESS != ret) {
        return ret;
    }

    //
    // Quick check for a "restricted" handle
    //

    if ( REGSEC_CHECK_HANDLE( hKey ) )
    {
        if ( ! REGSEC_CHECK_PATH( hKey, lpSubKey ) )
        {
            return( ERROR_ACCESS_DENIED );
        }

        hKey = (HANDLE) REGSEC_CLEAR_HANDLE( hKey );
    }

    //
    // Impersonate the client.
    //

    RPC_IMPERSONATE_CLIENT( NULL );

    //
    //  Subtract the NULLs from the Length of the provided string.
    //  This was added on the client side so that the NULL was
    //  transmited by RPC.
    //
    lpSubKey->Length -= sizeof( UNICODE_NULL );

    //
    // If this is a restricted site, first try to open the per-site version
    // of this key
    //

// local-only check for restricted
#ifdef LOCAL
    if (HKEY_RestrictedSite)
    {
        OBJECT_ATTRIBUTES   attributes;
        UNICODE_STRING      NewSubKey;

        MappedKey = MapRestrictedKey(hKey, lpSubKey, &NewSubKey);

        if (NULL != MappedKey)
        {
            InitializeObjectAttributes(
                &Obja,
                &NewSubKey,
                dwOptions & REG_OPTION_OPEN_LINK
                            ? (OBJ_OPENLINK | OBJ_CASE_INSENSITIVE)
                            : OBJ_CASE_INSENSITIVE,
                MappedKey,
                NULL
                );

            Status = NtOpenKey(
                        phkResult,
                        samDesired,
                        &Obja);

            RtlFreeUnicodeString(&NewSubKey);
            NtClose(MappedKey);

            if (NT_SUCCESS(Status))
            {
                RPC_REVERT_TO_SELF();
                return (error_status_t)RtlNtStatusToDosError( Status );
            }
        }
        else if (NULL != NewSubKey.Buffer)
        {
            //
            // The key is a class registration parent key (e.g. CLSID)
            // We need to always return the global version of these
            // keys to ensure global data under them is accessable.
            // Munge the samDesired to ensure that the key can be
            // opened.
            //
            samDesired = MAXIMUM_ALLOWED;
        }
    }
#endif

    //
    // Initialize the OBJECT_ATTRIBUTES structure and open the key.
    //

    InitializeObjectAttributes(
        &Obja,
        lpSubKey,
        dwOptions & REG_OPTION_OPEN_LINK ? (OBJ_OPENLINK | OBJ_CASE_INSENSITIVE)
        : OBJ_CASE_INSENSITIVE,
        hKey,
        NULL
        );

#ifdef LOCAL
    if (REG_CLASS_IS_SPECIAL_KEY(hKey)) {

        Status = BaseRegOpenClassKey(
            hKey,
            lpSubKey,
            dwOptions,
            samDesired,
            phkResult);

    } else 
#endif // LOCAL
    {
        //
        // Obja was initialized above 
        //

        Status = NtOpenKey(
            phkResult,
            samDesired,
            &Obja);
    }

    RPC_REVERT_TO_SELF();

    ret = (error_status_t)RtlNtStatusToDosError( Status );

    //
    // Restricted processes don't have access to most of the registry
    // so if we get here with an access denied, and this is a mappable key,
    // then try to create a per-site version of the key
    //

// local-only check for restricted
#ifdef LOCAL
    if (NULL != MappedKey && STATUS_ACCESS_DENIED == Status)
    {
        UNICODE_STRING  Class = {0, 0, 0};

        lpSubKey->Length += sizeof(UNICODE_NULL);

        ret = BaseRegCreateKey(
                        hKey,
                        lpSubKey,
                        &Class,
                        0,
                        MAXIMUM_ALLOWED,
                        NULL,
                        phkResult,
                        NULL);
    }


    if ((!REG_CLASS_IS_SPECIAL_KEY(hKey)) && !NT_SUCCESS(Status) && gpfnTermsrvOpenRegEntry) {

        //
        // Obja was initialized above
        //

        if (gpfnTermsrvOpenRegEntry(phkResult,
                                    samDesired,
                                    &Obja)) {
            Status = STATUS_SUCCESS;
            ret = (error_status_t)RtlNtStatusToDosError( Status );
        }
    }
#if defined(LEAK_TRACK)

    if (g_RegLeakTraceInfo.bEnableLeakTrack) {
        if (ERROR_SUCCESS == ret) {
            (void) TrackObject(*phkResult);
        }
    }
    
#endif (LEAK_TRACK)
#endif // local-only check for restricted

    //
    // Map the NTSTATUS code to a Win32 Registry error code and return.
    //

    return ret;
}

//
// BaseRegGetVersion - new for Chicago to determine what version a registry
//                                              key is connected to.
//

error_status_t
BaseRegGetVersion(
    IN HKEY hKey,
    OUT LPDWORD lpdwVersion
    )
/*++

Routine Description:

    New for Win95, allows a caller to determine what version a registry
    key is connected to.

Arguments:

    hKey - Supplies a handle to an open key.

    lpdwVersion - Returns the registry version.

Return Value:

    Returns ERROR_SUCCESS (0) for success;

    If successful, BaseRegGetVersion returns the registry version in lpdwVersion

--*/
{
    if (lpdwVersion != NULL) {
        *lpdwVersion = REMOTE_REGISTRY_VERSION;
        return(ERROR_SUCCESS);
    }
    //
    // ERROR_NOACCESS is kind of a weird thing to return,
    // but we want to return something different in the
    // NULL case because that is how we tell whether we
    // are talking to a Win95 machine. Win95's implementation
    // of BaseRegGetVersion does not actually fill in the
    // version. It just returns ERROR_SUCCESS or
    // ERROR_INVALID_PARAMETER.
    //
    return(ERROR_NOACCESS);
}

// local-only check for restricted
#ifdef LOCAL

HKEY
MapRestrictedKey(HKEY hKey, UNICODE_STRING *lpSubKey, UNICODE_STRING *NewSubKey)

/*++

Routine Description:

    If the given key is in a restricted area of the registry that apps need
    to be able to write to, return a new key pointing at
    HKCU\RestrictedSites\<site sid> that allows the site access.

Arguments:

    hKey        - The handle of the key to map
    lpSubKey    - The key that is to be opened/created/deleted
    NewSubKey   - The new subkey corresponding the the returned mapped key

Return Value:

    If the [hKey] is in an interesting area return a new key pointing at the
    RestrictedSites area

    If not, or if an error occurs, return NULL.

--*/
{
    NTSTATUS            status;
    SKeySemantics       semantics;
    BYTE                KeyNameBuffer[1024];    // Arbitrary large size
    UNICODE_STRING      KeyName;
    UNICODE_STRING     *pKeyName;
    OBJECT_ATTRIBUTES   attributes;
    USHORT              OldLength = 0;
    UNICODE_STRING      NullString = {0, 0, 0};
    WCHAR              *LastElement;
    
    //
    // Check to see if this is a mappable key
    //

    semantics._cbFullPath = sizeof(KeyNameBuffer);
    semantics._pFullPath = (PKEY_NAME_INFORMATION) KeyNameBuffer;

    status = BaseRegGetKeySemantics(hKey, lpSubKey, &semantics);

    //
    // If there are no class registration keys involved then it's not mappable
    //
    if (!NT_SUCCESS(status)
        || (!semantics._fClassRegParent && !semantics._fClassRegistration ) )

    {
        NewSubKey->Buffer = NULL;
        return NULL;
    }

    //
    // Class reg parent keys (e.g. HKCR, CLSID, Interface, etc) can't be
    // mapped because doing so would prevent further access to global data
    // under them.  Put a magic cookie in the buffer to indicate this status.
    //
    if (semantics._fClassRegParent)
    {
        NewSubKey->Buffer = (PWCHAR) 1;
        return NULL;
    }

    //
    // At this point the registry key were trying to map is in the form:
    //
    //      <prefix>[\<special>][\<suffix>]\<lastelement>
    //
    // <prefix> is generally HKCR.
    // <special> is CLSID, AppID, etc.
    // <suffix> is whatever is left not including the last element.
    // <lastelement> is the last element
    //
    // The goal is to return a mapped version of \<special>\<suffix> and
    // to set NewSubKey to <lastelement>
    //

    KeyName.Length = (USHORT) semantics._pFullPath->NameLength;
    KeyName.MaximumLength = (USHORT) semantics._pFullPath->NameLength;
    KeyName.Buffer = semantics._pFullPath->Name;

    pKeyName = &KeyName;

    //
    // Adjust the prefix length to include the trailing backslash (if any) and
    // trim off the prefix so that we can replace it with HKEY_RestrictedSite
    //

    semantics._cbPrefixLen = min(semantics._cbPrefixLen + sizeof(L'\\'),
                                 pKeyName->Length);

    pKeyName->Length        -= semantics._cbPrefixLen;
    pKeyName->MaximumLength -= semantics._cbPrefixLen;
    pKeyName->Buffer        += semantics._cbPrefixLen / sizeof(WCHAR);

    //
    // The caller can (justly) assume that <prefix>\<special> already exists,
    // so temporarily trim off <suffix> and create <special> in the
    // restricted area.
    //

    OldLength = pKeyName->Length;
    pKeyName->Length = semantics._cbSpecialKey;

    // _cbSpecialKey includes a backslash unless <special> doesn't exist

    if (semantics._cbSpecialKey > 0)
        pKeyName->Length -= sizeof(L'\\');

    InitializeObjectAttributes(
            &attributes,
            pKeyName,
            OBJ_CASE_INSENSITIVE,
            HKEY_RestrictedSite,
            NULL);

    // BUGBUG: Multipart special keys

    status = NtCreateKey(&hKey, MAXIMUM_ALLOWED, &attributes, 0,NULL,0,NULL);

    if (!NT_SUCCESS(status))
        return NULL;

    //
    // We've successfully mapped <prefix>\<special> to <HKRS>\<special>.  Now
    // Trim off the last element in <suffix> and point NewSubKey at it - this
    // element will be opened/created/whatever by the caller.
    //

    pKeyName->Buffer        += semantics._cbSpecialKey / sizeof(WCHAR);
    pKeyName->MaximumLength -= semantics._cbSpecialKey;
    pKeyName->Length         = OldLength - pKeyName->Length;

    if (semantics._cbSpecialKey > 0)
        pKeyName->Length -= sizeof(L'\\');

    ASSERT(pKeyName->Length < (MAXUSHORT - 10) && "Length is suspicious...");

    LastElement = pKeyName->Buffer + pKeyName->Length / 2;
    do
    {
        --LastElement;
        if (LastElement < pKeyName->Buffer)
            break;
    }
    while (L'\\' != *LastElement);
    ++LastElement;

    NewSubKey->MaximumLength = pKeyName->Length
                                - (USHORT)((BYTE *) LastElement
                                            - (BYTE *) pKeyName->Buffer);
    NewSubKey->Buffer        = RtlAllocateHeap(
                                        RtlProcessHeap(),
                                        0,
                                        NewSubKey->MaximumLength);
    if (NULL == NewSubKey->Buffer)
    {
        NtClose(hKey);
        return NULL;
    }

    NewSubKey->Length        = NewSubKey->MaximumLength;
    CopyMemory(NewSubKey->Buffer, LastElement, NewSubKey->Length);

    //
    // All thats left to do is open <suffix> if necessary
    //

    pKeyName->Length -= NewSubKey->Length;

    if (0 != pKeyName->Length)
    {
        HKEY BaseKey = hKey;

        pKeyName->Length -= sizeof(L'\\');

        InitializeObjectAttributes(
                &attributes,
                pKeyName,
                OBJ_CASE_INSENSITIVE,
                BaseKey,
                NULL);

        if (!NT_SUCCESS(NtOpenKey(&hKey, MAXIMUM_ALLOWED, &attributes)))
        {
            hKey = NULL;
            RtlFreeHeap(RtlProcessHeap(), 0, NewSubKey->Buffer);
            NewSubKey->Buffer = NULL;
        }

        NtClose(BaseKey);
    }

    return hKey;
}

#endif // local-only check for restricted
