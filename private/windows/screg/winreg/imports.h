/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Imports.h

Abstract:

    This file allows us to include standard system header files in the
    regrpc.idl file.  The regrpc.idl file imports a file called
    imports.idl.  This allows the regrpc.idl file to use the types defined
    in these header files.  It also causes the following line to be added
    in the MIDL generated header file:

    #include "imports.h"

    Thus these types are available to the RPC stub routines as well.

Author:

    David J. Gilman (davegi) 28-Jan-1992

--*/

#ifndef __IMPORTS_H__
#define __IMPORTS_H__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winregp.h>

typedef struct _RVALENT {       // Remote Value entry for RegQueryMultipleValues
        PUNICODE_STRING rv_valuename;
        DWORD   rv_valuelen;
        DWORD   rv_valueptr;
        DWORD   rv_type;
} RVALENT;

typedef RVALENT *PRVALENT;

//
// NT 3.1, 3.5 and 3.51 have no implementation of BaseRegGetVersion so this
// number is irrelevant to them.
//
// For a Win95 registry REMOTE_REGISTRY_VERSION should be == 4. Unfortunately,
// someone a bit confused by the concept of a pointer mis-implented the
// Win95 BaseRegGetVersion and it does not actually return a version number.
// So we detect Win95 by assuming anything that succeeds but returns
// a dwVersion outside the range 5-10.
//
// For an NT 4.0 registry, REMOTE_REGISTRY_VERSION==5.
//
// Win95 has the following bugs than NT 4.0 works around on the client side:
//  - BaseRegQueryInfoKey does not account for Unicode value names & data correctly
//  - BaseRegEnumValue returns value data length that is one WCHAR more than it
//    really should be for REG_SZ, REG_MULTI_SZ, and REG_EXPAND_SZ types.
//

#define WIN95_REMOTE_REGISTRY_VERSION 4
#define REMOTE_REGISTRY_VERSION 5

#define IsWin95Server(h,v) ((BaseRegGetVersion(h,&v)==ERROR_SUCCESS) &&  \
                            ((v < 5) || (v > 10)))

//
//  BOOL
//  IsPredefinedRegistryHandle(
//      IN RPC_HKEY     Handle
//      );
//

#define IsPredefinedRegistryHandle( h )                                     \
    ((  ( h == HKEY_CLASSES_ROOT        )                                   \
    ||  ( h == HKEY_CURRENT_USER        )                                   \
    ||  ( h == HKEY_LOCAL_MACHINE       )                                   \
    ||  ( h == HKEY_PERFORMANCE_DATA    )                                   \
    ||  ( h == HKEY_PERFORMANCE_TEXT    )                                   \
    ||  ( h == HKEY_PERFORMANCE_NLSTEXT )                                   \
    ||  ( h == HKEY_USERS               )                                   \
    ||  ( h == HKEY_CURRENT_CONFIG      )                                   \
    ||  ( h == HKEY_DYN_DATA            ))                                  \
    ?   TRUE                                                                \
    :   FALSE )

//
// RPC constants.
//

#define INTERFACE_NAME  L"winreg"
#define BIND_SECURITY   L"Security=Impersonation Dynamic False"

//
// External synchronization event.
//

#define PUBLIC_EVENT    "Microsoft.RPC_Registry_Server"

//
// Force the implementation of the API to be explicit (i.e wrt ANSI or
// UNICODE) about what other Registry APIs are called.
//

#undef RegCloseKey
#undef RegConnectRegistry
#undef RegCreateKey
#undef RegCreateKeyEx
#undef RegDeleteKey
#undef RegDeleteValue
#undef RegEnumKey
#undef RegEnumKeyEx
#undef RegEnumValue
#undef RegFlushKey
#undef RegGetKeySecurity
#undef RegNotifyChangeKeyValue
#undef RegOpenKey
#undef RegOpenKeyEx
#undef RegQueryInfoKey
#undef RegQueryValue
#undef RegQueryValueEx
#undef RegRestoreKey
#undef RegSaveKey
#undef RegSetKeySecurity
#undef RegSetValue
#undef RegSetValueEx

//
// Additional type for string arrays.
//

typedef CHAR    STR;

//
// Default values for Win 3.1 requested access.
//

#define WIN31_REGSAM                MAXIMUM_ALLOWED

#endif //__IMPORTS_H__
