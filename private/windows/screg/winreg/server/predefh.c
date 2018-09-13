/*++


Copyright (c) 1991 Microsoft Corporation

Module Name:

    Predefh.c

Abstract:

    This module contains routines for opening the Win32 Registry API's
    predefined handles.

    A predefined handle is used as a root to an absolute or relative
    sub-tree in the real Nt Registry. An absolute predefined handle maps
    to a specific key within the Registry. A relative predefined handle
    maps to a key relative to some additional information such as the
    current user.

    Predefined handles are strictly part of the Win32 Registry API. The
    Nt Registry API knows nothing about them.

    A predefined handle can be used anywhere that a non-predefined handle
    (i.e. one returned from RegCreateKey(), RegOpenKey() or
    RegConnectRegistry()) can be used.

Author:

    David J. Gilman (davegi) 15-Nov-1991

--*/

#include <rpc.h>
#include "regrpc.h"
#include "localreg.h"
#include "regclass.h"
#include "ntconreg.h"
#include "regsec.h"
#ifdef LOCAL
#include "tsappcmp.h"
#endif

#if defined(LEAK_TRACK) 
#include "regleak.h"
#endif // LEAK_TRACK

//
// Determine the length of a Unicode string w/o the trailing NULL.
//

#define LENGTH( str )   ( sizeof( str ) - sizeof( UNICODE_NULL ))

//
// Nt Registry name space.
//

#define MACHINE         L"\\REGISTRY\\MACHINE"

#define USER            L"\\REGISTRY\\USER"

#define CLASSES         L"\\REGISTRY\\MACHINE\\SOFTWARE\\CLASSES"

#define CLASSES_SUFFIX  L"\\SOFTWARE\\CLASSES"

#define CURRENTCONFIG   L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\HARDWARE PROFILES\\CURRENT"



UNICODE_STRING          MachineStringKey = {
                            LENGTH( MACHINE ),
                            LENGTH( MACHINE ),
                            MACHINE
                            };

UNICODE_STRING          UserStringKey = {
                            LENGTH( USER ),
                            LENGTH( USER ),
                            USER
                        };

UNICODE_STRING          ClassesStringKey = {
                            LENGTH( CLASSES ),
                            LENGTH( CLASSES ),
                            CLASSES
                        };

UNICODE_STRING          CurrentConfigStringKey = {
                            LENGTH( CURRENTCONFIG ),
                            LENGTH( CURRENTCONFIG ),
                            CURRENTCONFIG
                        };


NTSTATUS
InitSecurityAcls(PSECURITY_DESCRIPTOR *SecurityDescriptor)
/*++

Routine Description:

    Gives GENERIC_ALL to admins and denies WRITE_OWNER | WRITE_DAC  from everyone

Arguments:


Return Value:


--*/
{
    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSID                        BuiltInAdministrators = NULL;
    PSID                        Everyone = NULL;
    NTSTATUS                    Status;
    ULONG                       AclSize;
    ACL                         *Acl;

    *SecurityDescriptor = NULL;

    Status = RtlAllocateAndInitializeSid(
              &WorldAuthority,
              1,
              SECURITY_WORLD_RID,
              0,0,0,0,0,0,0,
              &Everyone );
    if( !NT_SUCCESS(Status) ) {
        goto Exit;
    }

    Status = RtlAllocateAndInitializeSid(
              &NtAuthority,
              2,
              SECURITY_BUILTIN_DOMAIN_RID,
              DOMAIN_ALIAS_RID_ADMINS,
              0,0,0,0,0,0,
              &BuiltInAdministrators );

    if( !NT_SUCCESS(Status) ) {
        goto Exit;
    }



    AclSize = sizeof (ACL) +
        (2 * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (ULONG))) +
        GetLengthSid(BuiltInAdministrators) +
        GetLengthSid(Everyone);

    *SecurityDescriptor = (PSECURITY_DESCRIPTOR)RtlAllocateHeap( RtlProcessHeap(), 0, SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize);
    if (!*SecurityDescriptor) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    } 

    Acl = (ACL *)((BYTE *)(*SecurityDescriptor) + SECURITY_DESCRIPTOR_MIN_LENGTH);

    Status = RtlCreateAcl(  Acl,
                            AclSize,
                            ACL_REVISION);
    if( !NT_SUCCESS(Status) ) {
        goto Exit;
    }

    Status = RtlAddAccessAllowedAce(Acl,
                                    ACL_REVISION,
                                    (KEY_ALL_ACCESS | ~(WRITE_OWNER | WRITE_DAC)),
                                    Everyone);
    if( !NT_SUCCESS(Status) ) {
        goto Exit;
    }

    Status = RtlAddAccessAllowedAce(Acl,
                                    ACL_REVISION,
                                    GENERIC_ALL,
                                    BuiltInAdministrators);
    if( !NT_SUCCESS(Status) ) {
        goto Exit;
    }

    Status = RtlCreateSecurityDescriptor(
                *SecurityDescriptor,
                SECURITY_DESCRIPTOR_REVISION
                );
    if( !NT_SUCCESS(Status) ) {
        goto Exit;
    }

    Status = RtlSetDaclSecurityDescriptor(  *SecurityDescriptor,
                                            TRUE,
                                            Acl,
                                            FALSE);

Exit:
    if( Everyone ) {
        RtlFreeSid( Everyone );
    }
    
    if( BuiltInAdministrators ) {
        RtlFreeSid( BuiltInAdministrators );
    }

    return Status;
}


error_status_t
OpenClassesRoot(
    IN PREGISTRY_SERVER_NAME ServerName,
    IN REGSAM samDesired,
    OUT PRPC_HKEY phKey
    )

/*++

Routine Description:

    Attempts to open the the HKEY_CLASSES_ROOT predefined handle.

Arguments:

    ServerName - Not used.
    samDesired - This access mask describes the desired security access
                 for the key.
    phKey - Returns a handle to the key \REGISTRY\MACHINE\SOFTWARE\CLASSES.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    PSECURITY_DESCRIPTOR     SecurityDescriptor = NULL;
    OBJECT_ATTRIBUTES       Obja;
    NTSTATUS                Status;
    UNICODE_STRING          UsersHive;
    UNICODE_STRING          UsersMergedHive;
    error_status_t          Res;

    UNREFERENCED_PARAMETER( ServerName );

    //
    // Impersonate the client.
    //

    RPC_IMPERSONATE_CLIENT( NULL );

#ifdef LOCAL
    //
    // Multiuser CLASSES key so each user has their own key.  If opening
    // CLASSES in execute mode - open it under HKEY_CURRENT_USER else
    // just let it fall thru here and open the global one.
    //
    if (gpfnTermsrvOpenUserClasses) {
        Status = gpfnTermsrvOpenUserClasses(samDesired,phKey);
    } else {
        *phKey = NULL;
    }
    if (!(*phKey)) {
#endif // LOCAL


    //
    // Initialize the SECURITY_DESCRIPTOR.
    //
    Status = InitSecurityAcls(&SecurityDescriptor);

    ASSERT( NT_SUCCESS( Status ));
    if( ! NT_SUCCESS( Status )) {
        goto error_exit;
    }


#ifdef LOCAL

    if (gbCombinedClasses) {
        // first try for a per-user HKCR
        Res = OpenCombinedClassesRoot( samDesired, phKey );

        if ( NT_SUCCESS( Res ) ) {
            goto error_exit;
        }
    }
#endif

    //
    // Initialize the OBJECT_ATTRIBUTES structure so that it creates
    // (opens) the key "\REGISTRY\MACHINE\SOFTWARE\CLASSES" with a Security
    // Descriptor that allows everyone complete access.
    //

    InitializeObjectAttributes(
        &Obja,
        &ClassesStringKey,
        OBJ_CASE_INSENSITIVE,
        NULL,
        SecurityDescriptor
        );

    Status = NtCreateKey(
                phKey,
                samDesired, // MAXIMUM_ALLOWED,
                &Obja,
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                NULL
                );
#ifdef LOCAL
    }
#endif // LOCAL

#if DBG
        if( ! NT_SUCCESS( Status )) {
            DbgPrint(
                "Winreg Server: "
                "Creating HKEY_CLASSES_ROOT failed, status = 0x%x\n",
                Status
                );
        }
#endif

error_exit:
    
    if( SecurityDescriptor != NULL ) {
	RtlFreeHeap( RtlProcessHeap(), 0, SecurityDescriptor );
    }
    RPC_REVERT_TO_SELF();
    return (error_status_t)RtlNtStatusToDosError( Status );
}

error_status_t
OpenCurrentUser(
    IN PREGISTRY_SERVER_NAME ServerName,
    IN REGSAM samDesired,
    OUT PRPC_HKEY phKey
    )

/*++

Routine Description:

    Attempts to open the the HKEY_CURRENT_USER predefined handle.

Arguments:

    ServerName - Not used.
    samDesired - This access mask describes the desired security access
                 for the key.
    phKey - Returns a handle to the key \REGISTRY\USER\*.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    NTSTATUS            Status;

    UNREFERENCED_PARAMETER( ServerName );

    //
    // Impersonate the client.
    //

    RPC_IMPERSONATE_CLIENT( NULL );

    //
    // Open the registry key.
    //

    Status = RtlOpenCurrentUser( samDesired, /* MAXIMUM_ALLOWED, */ phKey );

    RPC_REVERT_TO_SELF();
    //
    // Map the returned status
    //

    return (error_status_t)RtlNtStatusToDosError( Status );
}

error_status_t
OpenLocalMachine(
    IN PREGISTRY_SERVER_NAME ServerName,
    IN REGSAM samDesired,
    OUT PRPC_HKEY phKey
    )

/*++

Routine Description:

    Attempt to open the the HKEY_LOCAL_MACHINE predefined handle.

Arguments:

    ServerName - Not used.
    samDesired - This access mask describes the desired security access
                 for the key.
    phKey - Returns a handle to the key \REGISTRY\MACHINE.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    OBJECT_ATTRIBUTES   Obja;
    NTSTATUS            Status;

    UNREFERENCED_PARAMETER( ServerName );

    //
    // Impersonate the client.
    //

    RPC_IMPERSONATE_CLIENT( NULL );

    InitializeObjectAttributes(
        &Obja,
        &MachineStringKey,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(
                phKey,
                samDesired, // MAXIMUM_ALLOWED,
                &Obja
                );
#if DBG
        if( ! NT_SUCCESS( Status )) {
            DbgPrint(
                "Winreg Server: "
                "Opening HKEY_LOCAL_MACHINE failed, status = 0x%x\n",
                Status
                );
        }
#endif

    if ( NT_SUCCESS( Status ) )
    {
        if (! REGSEC_CHECK_REMOTE( phKey ) )
        {
            *phKey = (HANDLE) REGSEC_FLAG_HANDLE( *phKey, CHECK_MACHINE_PATHS );
        }
    }

    RPC_REVERT_TO_SELF();

    return (error_status_t)RtlNtStatusToDosError( Status );
}

error_status_t
OpenUsers(
    IN PREGISTRY_SERVER_NAME ServerName,
    IN REGSAM samDesired,
    OUT PRPC_HKEY phKey
    )

/*++

Routine Description:

    Attempts to open the the HKEY_USERS predefined handle.

Arguments:

    ServerName - Not used.
    samDesired - This access mask describes the desired security access
                 for the key.
    phKey - Returns a handle to the key \REGISTRY\USER.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    OBJECT_ATTRIBUTES   Obja;
    NTSTATUS            Status;

    UNREFERENCED_PARAMETER( ServerName );

    //
    // Impersonate the client.
    //

    RPC_IMPERSONATE_CLIENT( NULL );

    InitializeObjectAttributes(
        &Obja,
        &UserStringKey,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(
                phKey,
                samDesired, // MAXIMUM_ALLOWED,
                &Obja
                );
#if DBG
        if( ! NT_SUCCESS( Status )) {
            DbgPrint(
                "Winreg Server: "
                "Opening HKEY_USERS failed, status = 0x%x\n",
                Status
                );
        }
#endif

    if ( NT_SUCCESS( Status ) )
    {
        if (! REGSEC_CHECK_REMOTE( phKey ) )
        {
            *phKey = (HANDLE) REGSEC_FLAG_HANDLE( *phKey, CHECK_USER_PATHS );
        }
    }

    RPC_REVERT_TO_SELF();

    return (error_status_t)RtlNtStatusToDosError( Status );
}

error_status_t
OpenCurrentConfig(
    IN PREGISTRY_SERVER_NAME ServerName,
    IN REGSAM samDesired,
    OUT PRPC_HKEY phKey
    )

/*++

Routine Description:

    Attempts to open the the HKEY_CURRENT_CONFIG predefined handle.

Arguments:

    ServerName - Not used.
    samDesired - This access mask describes the desired security access
                 for the key.
    phKey - Returns a handle to the key \REGISTRY\MACHINE\SYSTEM\CURRENTCONTROLSET\HARDWARE PROFILES\CURRENT

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    OBJECT_ATTRIBUTES   Obja;
    NTSTATUS            Status;

    UNREFERENCED_PARAMETER( ServerName );

    //
    // Impersonate the client.
    //

    RPC_IMPERSONATE_CLIENT( NULL );

    InitializeObjectAttributes(
        &Obja,
        &CurrentConfigStringKey,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(
                phKey,
                samDesired, // MAXIMUM_ALLOWED,
                &Obja
                );
#if DBG
        if( ! NT_SUCCESS( Status )) {
            DbgPrint(
                "Winreg Server: "
                "Opening HKEY_CURRENT_CONFIG failed, status = 0x%x\n",
                Status
                );
        }
#endif
    RPC_REVERT_TO_SELF();

    return (error_status_t)RtlNtStatusToDosError( Status );
}
error_status_t
OpenPerformanceData(
    IN PREGISTRY_SERVER_NAME ServerName,
    IN REGSAM samDesired,
    OUT PRPC_HKEY phKey
    )

/*++

Routine Description:

    Attempts to open the the HKEY_PERFORMANCE_DATA predefined handle.

Arguments:

    ServerName - Not used.
    samDesired - Not used.
    phKey - Returns a the predefined handle HKEY_PERFORMANCE_DATA.

Return Value:

    Returns ERROR_SUCCESS (0) for success;
    or a DOS (not NT) error-code for failure.

--*/

{
    IO_STATUS_BLOCK IoStatusBlock;
    RTL_RELATIVE_NAME RelativeName;
    UNICODE_STRING DeviceNameU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING DeviceName;
    NTSTATUS status;

    if ( 0 ) {
        DBG_UNREFERENCED_PARAMETER(ServerName);
        DBG_UNREFERENCED_PARAMETER(samDesired);
    }

    //
    // Impersonate the client.
    //

    RPC_IMPERSONATE_CLIENT( NULL );

    if ( ! REGSEC_CHECK_REMOTE( phKey ) )
    {
        RPC_REVERT_TO_SELF();
        return( ERROR_ACCESS_DENIED );
    }

    // check if we are in the middle of Lodctr/unlodctr.
    // if so, don't open the performance data stuff.
    {
    HANDLE  hFileMapping = NULL;
    WCHAR MapFileName[] = L"Perflib Busy";
    DWORD             *lpData;
    BOOL    bBusy = FALSE;

    hFileMapping = OpenFileMappingW (FILE_MAP_READ, TRUE, (LPCWSTR)MapFileName);
    if (hFileMapping) {
        // someone is running lodctr perhaps find out by reading the first DWORD
        lpData = MapViewOfFile (hFileMapping,
                FILE_MAP_READ, 0L, 0L, 0L);
        if (lpData) {
             // successfully mapped so read it
             // 1 = busy, 0 = not
             bBusy = (BOOL)(*lpData);
             UnmapViewOfFile (lpData);
        }
        CloseHandle (hFileMapping);
        if (bBusy) {
            *phKey = (RPC_HKEY) HKEY_PERFORMANCE_DATA;
            RPC_REVERT_TO_SELF();
            return ERROR_SUCCESS;
        }
    } else {
        // no lodctr so continue
    }
    }
    status  = PerfOpenKey();

    RPC_REVERT_TO_SELF();

    *phKey = (RPC_HKEY) HKEY_PERFORMANCE_DATA;
    return ERROR_SUCCESS;

    RPC_REVERT_TO_SELF();
    return status;
}

error_status_t
OpenPerformanceText(
    IN PREGISTRY_SERVER_NAME ServerName,
    IN REGSAM samDesired,
    OUT PRPC_HKEY phKey
    )

/*++

Routine Description:

    Attempts to open the the HKEY_PERFORMANCE_TEXT predefined handle.

Arguments:

    ServerName - Not used.
    samDesired - Not used.
    phKey - Returns the predefined handle HKEY_PERFORMANCE_TEXT.

Return Value:

    Returns ERROR_SUCCESS (0) for success;
    or a DOS (not NT) error-code for failure.

--*/

{
    error_status_t Status = ERROR_SUCCESS;

// No need to call OpenPerformanceData for getting text (HWC 4/1994)
//    Status = OpenPerformanceData(ServerName, samDesired, phKey);
//    if (Status==ERROR_SUCCESS) {
        *phKey = HKEY_PERFORMANCE_TEXT;
//    }
    return(Status);
}

error_status_t
OpenPerformanceNlsText(
    IN PREGISTRY_SERVER_NAME ServerName,
    IN REGSAM samDesired,
    OUT PRPC_HKEY phKey
    )

/*++

Routine Description:

    Attempts to open the the HKEY_PERFORMANCE_TEXT predefined handle.

Arguments:

    ServerName - Not used.
    samDesired - Not used.
    phKey - Returns the predefined handle HKEY_PERFORMANCE_NLSTEXT.

Return Value:

    Returns ERROR_SUCCESS (0) for success;
    or a DOS (not NT) error-code for failure.

--*/

{
    error_status_t Status = ERROR_SUCCESS;

// No need to call OpenPerformanceData for getting text (HWC 4/1994)
//    Status = OpenPerformanceData(ServerName, samDesired, phKey);
//    if (Status==ERROR_SUCCESS) {
        *phKey = HKEY_PERFORMANCE_NLSTEXT;
//    }
    return(Status);
}


error_status_t
OpenDynData(
    IN PREGISTRY_SERVER_NAME ServerName,
    IN REGSAM samDesired,
    OUT PRPC_HKEY phKey
    )
/*++

Routine Description:

    Attempts to open the the HKEY_DYN_DATA predefined handle.

    There is currently no HKEY_DYN_DATA on NT, thus this
    function always returns ERROR_CALL_NOT_IMPLEMENTED.

Arguments:

    ServerName - Not used.
    samDesired - This access mask describes the desired security access
                 for the key.
    phKey - Returns a handle to the key HKEY_DYN_DATA

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    return((error_status_t)ERROR_CALL_NOT_IMPLEMENTED);
}

