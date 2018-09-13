/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Localreg.h

Abstract:

    This file is used to make small changes to the Winreg Base entry
    points so that they compile as local or remote functions.

Author:

    David J. Gilman (davegi) 26-Aug-1992

Notes:

    The mapping from BaseRegNotifyChangeKeyValue to
    LocalBaseRegNotifyChangeKeyValue is missing because in the
    local case the call to NJt is made by the client.


--*/

#if defined( LOCAL )

//
// Change the 'server' enrty point names for the local functions.
//

//
// Base functions.
//

#define BaseRegDeleteKey                LocalBaseRegDeleteKey
#define BaseRegDeleteValue              LocalBaseRegDeleteValue
#define BaseRegEnumKey                  LocalBaseRegEnumKey
#define BaseRegEnumValue                LocalBaseRegEnumValue
#define BaseRegCloseKey                 LocalBaseRegCloseKey
#define BaseRegCreateKey                LocalBaseRegCreateKey
#define BaseRegFlushKey                 LocalBaseRegFlushKey
#define BaseRegOpenKey                  LocalBaseRegOpenKey
#define BaseRegLoadKey                  LocalBaseRegLoadKey
#define BaseRegUnLoadKey                LocalBaseRegUnLoadKey
#define BaseRegReplaceKey               LocalBaseRegReplaceKey
#define BaseRegQueryInfoKey             LocalBaseRegQueryInfoKey
#define BaseRegQueryValue               LocalBaseRegQueryValue
#define BaseRegGetKeySecurity           LocalBaseRegGetKeySecurity
#define BaseRegSetKeySecurity           LocalBaseRegSetKeySecurity
#define BaseRegRestoreKey               LocalBaseRegRestoreKey
#define BaseRegSaveKey                  LocalBaseRegSaveKey
#define BaseRegSetValue                 LocalBaseRegSetValue
#define BaseRegQueryMultipleValues      LocalBaseRegQueryMultipleValues
#define BaseRegGetVersion               LocalBaseRegGetVersion


//
// Predefined handle functions.
//

#define OpenClassesRoot                 LocalOpenClassesRoot
#define OpenCurrentUser                 LocalOpenCurrentUser
#define OpenLocalMachine                LocalOpenLocalMachine
#define OpenUsers                       LocalOpenUsers
#define OpenPerformanceData             LocalOpenPerformanceData
#define OpenPerformanceText             LocalOpenPerformanceText
#define OpenPerformanceNlsText          LocalOpenPerformanceNlsText
#define OpenCurrentConfig               LocalOpenCurrentConfig
#define OpenDynData                     LocalOpenDynData


//
// Initialization and cleanup functions.
//

#define InitializeRegCreateKey          LocalInitializeRegCreateKey
#define CleanupRegCreateKey             LocalCleanupRegCreateKey


//
// No RPC Impersonation needed in the local case.
//

#define RPC_IMPERSONATE_CLIENT( Handle ) 
#define RPC_REVERT_TO_SELF()

#define REGSEC_CHECK_HANDLE( Handle )           ( 0 )
#define REGSEC_FLAG_HANDLE( Handle, Flag )      ( Handle )
#define REGSEC_TEST_HANDLE( Handle, Flag )      ( Handle )
#define REGSEC_CLEAR_HANDLE( Handle )           ( Handle )
#define REGSEC_CHECK_REMOTE( Key )              ( 1 )
#define REGSEC_CHECK_PATH( Key, Path )          ( 1 )

#else


//
// Impersonate the client.
//

#define RPC_IMPERSONATE_CLIENT( Handle )                        \
    {                                                           \
        RPC_STATUS _rpcstatus = RpcImpersonateClient( NULL );   \
        if (_rpcstatus != ERROR_SUCCESS) {                      \
            return(_rpcstatus);                                 \
        }                                                       \
    }

#define RPC_REVERT_TO_SELF() RpcRevertToSelf()

#define CHECK_MACHINE_PATHS     0x00000001
#define CHECK_USER_PATHS        0x00000002

#define REGSEC_CHECK_HANDLE( Handle )   ((LONG)(ULONG_PTR) (Handle) & (CHECK_MACHINE_PATHS | CHECK_USER_PATHS))
#define REGSEC_FLAG_HANDLE( Handle, Flag )    ((LONG)(ULONG_PTR) (Handle) | Flag )
#define REGSEC_TEST_HANDLE( Handle, Flag )  ((LONG)(ULONG_PTR) (Handle) & Flag )
#define REGSEC_CLEAR_HANDLE( Handle )   ((LONG)(ULONG_PTR) (Handle) & ~(CHECK_MACHINE_PATHS | CHECK_USER_PATHS))
#define REGSEC_CHECK_REMOTE( Key )              ( RegSecCheckRemoteAccess( Key ) )
#define REGSEC_CHECK_PATH( Key, Path )          ( RegSecCheckPath( Key, Path ) )


#endif // LOCAL

NTSTATUS RelinkMachineKey( 
   LPWSTR lpSubDirName, 
   PUNICODE_STRING lpSubKey,
   HKEY  hKey );
