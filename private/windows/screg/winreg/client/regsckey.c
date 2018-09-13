/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regsckey.c

Abstract:

    This module contains the client side wrappers for the Win32 Registry
    APIs to set and get the SECURITY_DESCRIPTOR for a key.  That is:

        - RegGetKeySecurity
        - RegSetKeySecurity

Author:

    David J. Gilman (davegi) 18-Mar-1992

Notes:

    See the notes in server\regsckey.c.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"

NTSTATUS BaseRegGetUserAndMachineClass(
    PVOID          pKeySemantics,
    HKEY           hKey,
    REGSAM         samDesired,
    PHKEY          phkMachine,
    PHKEY          phkUser);


LONG
APIENTRY
RegGetKeySecurity (
    HKEY hKey,
    SECURITY_INFORMATION RequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    LPDWORD lpcbSecurityDescriptor
    )

/*++

Routine Description:

    Win32 RPC wrapper for getting a key's security descriptor.

--*/

{
    RPC_SECURITY_DESCRIPTOR     RpcSD;
    LONG                        Error;
    REGSAM                      DesiredAccess;
    BOOLEAN                     IsSpecialHandle = FALSE;

    HKEY                        hkMachineClass;
    HKEY                        hkUserClass;
    HKEY                        hkClassKey = NULL;

    BOOL                        fClassesRoot = FALSE;
    HKEY                        TempHandle = NULL;


#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    //
    // Limit the capabilities associated with HKEY_PERFORMANCE_DATA.
    //

    ConvertKey(&hKey);
    if( hKey == HKEY_PERFORMANCE_DATA ) {
        return ERROR_INVALID_HANDLE;
    }

    if (HKEY_CLASSES_ROOT == hKey) {
        fClassesRoot = TRUE;
    }

    if( IsPredefinedRegistryHandle( hKey ) &&
        ( ( RequestedInformation & SACL_SECURITY_INFORMATION ) != 0 )
      ) {
        //
        //  If SACL is to be retrieved, open a handle with special access
        //
        DesiredAccess = ACCESS_SYSTEM_SECURITY;
        if( ( RequestedInformation &
              ( DACL_SECURITY_INFORMATION |
                OWNER_SECURITY_INFORMATION |
                GROUP_SECURITY_INFORMATION
              ) ) != 0 ) {
            DesiredAccess |= READ_CONTROL;
        }

        Error = OpenPredefinedKeyForSpecialAccess( hKey,
                                                   DesiredAccess,
                                                   &hKey );

        if( Error != ERROR_SUCCESS ) {
            return( Error );
        }
        ASSERT( IsLocalHandle( hKey ) );
        IsSpecialHandle = TRUE;

    } else {
        hKey = MapPredefinedHandle( hKey, &TempHandle );
        IsSpecialHandle = FALSE;

        DesiredAccess = MAXIMUM_ALLOWED;
    }

    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    if (IsLocalHandle( hKey )) {

        NTSTATUS Status;

        if (IsSpecialClassesHandle( hKey ) || fClassesRoot) {

            Status = BaseRegGetUserAndMachineClass(
                NULL,
                hKey,
                DesiredAccess,
                &hkMachineClass,
                &hkUserClass);

            if( IsSpecialHandle ) {
                RegCloseKey( hKey );
            }

            if (!NT_SUCCESS(Status)) {
                Error = (error_status_t) RtlNtStatusToDosError(Status);
                goto ExitCleanup;
            }

            if (hkMachineClass && hkUserClass) {

                if (hkMachineClass != hKey) {
                    hkClassKey = hkMachineClass;
                } else {
                    hkClassKey = hkUserClass;
                }

                if (fClassesRoot) {
                    hKey = hkMachineClass;
                } else {
                    hKey = hkUserClass;
                }
            }
        }
    }

    //
    // Convert the supplied SECURITY_DESCRIPTOR to a RPCable version.
    //
    RpcSD.lpSecurityDescriptor    = pSecurityDescriptor;
    RpcSD.cbInSecurityDescriptor  = *lpcbSecurityDescriptor;
    RpcSD.cbOutSecurityDescriptor = 0;

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegGetKeySecurity(
                                hKey,
                                RequestedInformation,
                                &RpcSD
                                );
        if( IsSpecialHandle ) {
            RegCloseKey( hKey );
        }

    } else {

        Error = (LONG)BaseRegGetKeySecurity(
                                DereferenceRemoteHandle( hKey ),
                                RequestedInformation,
                                &RpcSD
                                );
    }

    //
    // Extract the size of the SECURITY_DESCRIPTOR from the RPCable version.
    //

    *lpcbSecurityDescriptor = RpcSD.cbInSecurityDescriptor;

    if (hkClassKey) {
        NtClose(hkClassKey);
    }

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;

}

LONG
APIENTRY
RegSetKeySecurity(
    HKEY hKey,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )

/*++

Routine Description:

    Win32 RPC wrapper for setting a key's security descriptor.

--*/

{
    RPC_SECURITY_DESCRIPTOR     RpcSD;
    LONG                        Error;
    BOOLEAN                     IsSpecialHandle;
    REGSAM                      DesiredAccess;

    HKEY                        hkMachineClass;
    HKEY                        hkUserClass;
    HKEY                        hkClassKey = NULL;

    BOOL                        fClassesRoot = FALSE;
    HKEY                        TempHandle = NULL;


#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    //
    // Limit the capabilities associated with HKEY_PERFORMANCE_DATA.
    //

    ConvertKey(&hKey);
    if( hKey == HKEY_PERFORMANCE_DATA ) {
        return ERROR_INVALID_HANDLE;
    }

    if (HKEY_CLASSES_ROOT == hKey) {
        fClassesRoot = TRUE;
    }

    if( IsPredefinedRegistryHandle( hKey ) &&
        ( ( SecurityInformation & SACL_SECURITY_INFORMATION ) != 0 )
      ) {
        //
        //  If the SACL is to be set, open a handle with
        //  special access
        //
        DesiredAccess = MAXIMUM_ALLOWED | ACCESS_SYSTEM_SECURITY;
        if( SecurityInformation & DACL_SECURITY_INFORMATION ) {
            DesiredAccess |= WRITE_DAC;
        } else if( SecurityInformation & OWNER_SECURITY_INFORMATION ) {
            DesiredAccess |= WRITE_OWNER;
        }

        Error = OpenPredefinedKeyForSpecialAccess( hKey,
                                                   DesiredAccess,
                                                   &hKey );
        if( Error != ERROR_SUCCESS ) {
            return( Error );
        }
        ASSERT( IsLocalHandle( hKey ) );
        IsSpecialHandle = TRUE;

    } else {
        hKey = MapPredefinedHandle( hKey, &TempHandle );
        IsSpecialHandle = FALSE;

        DesiredAccess = MAXIMUM_ALLOWED;
    }

    if( hKey == NULL ) {
        Error = ERROR_INVALID_HANDLE;
        goto ExitCleanup;
    }

    if (IsLocalHandle( hKey )) {

        NTSTATUS Status;

        if (IsSpecialClassesHandle( hKey ) || fClassesRoot) {

            Status = BaseRegGetUserAndMachineClass(
                NULL,
                hKey,
                DesiredAccess,
                &hkMachineClass,
                &hkUserClass);

            if( IsSpecialHandle ) {
                RegCloseKey( hKey );
            }

            if (!NT_SUCCESS(Status)) {
                Error = (error_status_t) RtlNtStatusToDosError(Status);
                goto ExitCleanup;
            }

            if (hkMachineClass && hkUserClass) {

                if (hkMachineClass != hKey) {
                    hkClassKey = hkMachineClass;
                } else {
                    hkClassKey = hkUserClass;
                }

                if (fClassesRoot) {
                    hKey = hkMachineClass;
                } else {
                    hKey = hkUserClass;
                }
            }
        }
    }

    //
    // Convert the supplied SECURITY_DESCRIPTOR to a RPCable version.
    //
    RpcSD.lpSecurityDescriptor = NULL;

    Error = MapSDToRpcSD(
        pSecurityDescriptor,
        &RpcSD
        );


    if( Error != ERROR_SUCCESS ) {
        if( IsSpecialHandle ) {
            RegCloseKey( hKey );
        }
        goto ExitCleanup;
    }

    if( IsLocalHandle( hKey )) {

        Error = (LONG)LocalBaseRegSetKeySecurity (
                            hKey,
                            SecurityInformation,
                            &RpcSD
                            );

    } else {

        Error = (LONG)BaseRegSetKeySecurity (
                            DereferenceRemoteHandle( hKey ),
                            SecurityInformation,
                            &RpcSD
                            );
    }

    //
    // Free the buffer allocated by MapSDToRpcSD.
    //

    RtlFreeHeap(
        RtlProcessHeap( ), 0,
        RpcSD.lpSecurityDescriptor
        );

    if( IsSpecialHandle ) {
        RegCloseKey( hKey );
    }

    if (hkClassKey) {
        NtClose(hkClassKey);
    }

ExitCleanup:
    CLOSE_LOCAL_HANDLE(TempHandle);
    return Error;
}
