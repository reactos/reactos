/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ws2setup

Abstract:

    This module contains a simple setup application for NT WinSock 2.0.
    This application migrates the existing WinSock 1.1 information stored
    in the registry to WinSock 2.0.

Author:

    Keith Moore (keithmo)         9-Oct-1995

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//

#define MAX_REGISTRY_NAME       256

#define SERVICES_KEY            "System\\CurrentControlSet\\Services"

#define WINSOCK_SUBKEY          "WinSock"
#define MIGRATION_SUBKEY        "Setup Migration"
#define WINSOCK2_SUBKEY         "WinSock2"
#define WINSOCK2_PARAM_SUBKEY   "WinSock2\\Parameters"

#define MAX_CATALOG_NAME_LENGTH 32  // stolen from dll\include\wsautil.h


//
// Private prototypes.
//

BOOL
CALLBACK
MigrationCallback(
    WSA_SETUP_OPCODE Opcode,
    LPVOID Parameter,
    DWORD Context
    );

DWORD
CleanWinsock2Tree(
    VOID
    );

DWORD
OpenServicesRoot(
    PHKEY ServicesKey
    );

DWORD
OpenWinsockRoot(
    HKEY ServicesKey,
    PHKEY WinsockKey
    );

DWORD
OpenWinsock2ParametersRoot(
    HKEY ServicesKey,
    PHKEY Winsock2ParametersKey
    );

DWORD
RecursivelyDeleteRegistryTree(
    HKEY RootKey,
    LPSTR TargetKeyName
    );


//
// Public functions.
//

INT
__cdecl
main(
    INT argc,
    CHAR * argv[]
    )
{

    DWORD error;
    WSA_SETUP_DISPOSITION disposition;
    BOOL cleanFirst;
    HMODULE winsockDllHandle;
    LPFN_MIGRATE_WINSOCK_CONFIGURATION migrateWinsockConfiguration;

    //
    // Interpret the command line arguments.
    //

    cleanFirst = FALSE;

    if( argc != 1 ) {

        if( argc == 2 &&
            _stricmp( argv[1], "clean" ) == 0 ) {

            cleanFirst = TRUE;

        } else {

            printf( "Use: ws2setup [clean]\n" );
            return 1;

        }

    }

    //
    // If necessary, blow away the old WinSock 2.0 tree before continuing.
    // If this fails, tell the user but press on regardless.
    //

    if( cleanFirst ) {

        printf( "Cleaning WinSock 2.0 Protocol Catalog..." );

        error = CleanWinsock2Tree();

        if( error == NO_ERROR ) {

            printf( "done\n" );

        } else {

            printf( "error %lu\n", error );

        }

    }

    //
    // Load WSOCK32.DLL. Note that we must dynamically load it; otherwise,
    // it will cache registry data that we may want to delete.
    //

    winsockDllHandle = LoadLibrary( "WSOCK32.DLL" );

    if( winsockDllHandle == NULL ) {

        error = GetLastError();

        printf( "Cannot load WSOCK32.DLL, error %lu\n", error );
        return 1;

    }

    migrateWinsockConfiguration = (PVOID)GetProcAddress(
                                      winsockDllHandle,
                                      "MigrateWinsockConfiguration"
                                      );

    if( migrateWinsockConfiguration == NULL ) {

        error = GetLastError();

        printf( "Cannot find MigrateWinsockConfiguration(), error %lu\n", error );
        return 1;

    }

    //
    // Do it.
    //

    printf( "Updating configuration info...\n" );

    error = migrateWinsockConfiguration(
                &disposition,
                &MigrationCallback,
                0 );

    if( error != 0 ) {

        printf( "MigrateWinsockConfiguration() failed, error %lu\n", error );
        return 1;

    }

    switch( disposition ) {

    case WsaSetupNoChangesMade:
        printf( "No configuration changes necessary\n" );
        break;

    case WsaSetupChangesMadeRebootNotNecessary:
    case WsaSetupChangesMadeRebootRequired:
        printf( "Configuration changes made successfully\n" );
        break;

    default:
        printf(
            "MigrateWinsockConfiguration() returned invalid disposition %d\n",
            disposition
            );
        break;

    }

    return 0;

}   // main


//
// Private functions.
//

BOOL
CALLBACK
MigrationCallback(
    WSA_SETUP_OPCODE Opcode,
    LPVOID Parameter,
    DWORD Context
    )
{

    switch( Opcode ) {

    case WsaSetupInstallingProvider :
        printf( "    Installing %ls\n", Parameter );
        break;

    case WsaSetupRemovingProvider :
        printf( "    Removing %ls\n", Parameter );
        break;

    case WsaSetupValidatingProvider :
        printf( "    Validating %ls\n", Parameter );
        break;

    case WsaSetupUpdatingProvider :
        printf( "    Updating %ls\n", Parameter );
        break;

    default :
        printf( "MigrationCallback(): unknown opcode %d\n", Opcode );
        break;
    }

    return TRUE;

}   // MigrationCallback


DWORD
CleanWinsock2Tree(
    VOID
    )
{

    DWORD error;
    HKEY servicesKey;
    HKEY winsockKey;
    HKEY winsock2ParametersKey;
    DWORD valueType;
    DWORD valueLength;
    CHAR currentProtocolCatalog[MAX_CATALOG_NAME_LENGTH];

    //
    // Setup locals so we know how to cleanup on exit.
    //

    servicesKey = NULL;
    winsockKey = NULL;
    winsock2ParametersKey = NULL;

    //
    // Open the services registry key.
    //

    error = OpenServicesRoot(
                &servicesKey
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Open the winsock registry key.
    //

    error = OpenWinsockRoot(
                servicesKey,
                &winsockKey
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Open the winsock2 parameters registry key.
    //

    error = OpenWinsock2ParametersRoot(
                servicesKey,
                &winsock2ParametersKey
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Determine the current protocol catalog key name.
    //

    valueLength = sizeof(currentProtocolCatalog);

    error = RegQueryValueEx(
                winsock2ParametersKey,
                WINSOCK_CURRENT_PROTOCOL_CATALOG_NAME,
                NULL,
                &valueType,
                currentProtocolCatalog,
                &valueLength
                );

    if( error == NO_ERROR ) {

        //
        // Delete the WinSock 2.0 protocol catalog key.
        //

        error = RecursivelyDeleteRegistryTree(
                    winsock2ParametersKey,
                    currentProtocolCatalog
                    );

        if( error != NO_ERROR ) {

            goto exit;

        }

    } else {

        //
        // This system's registry does not have the "current catalog"
        // value, so try all catalog names used prior to the introduction
        // of this value.
        //
        // Note that, since we're not taking the trouble to find out
        // exactly which registry key is being used for the protocol
        // catalog, we'll ignore any errors generated by these operations.
        //

        (VOID)RecursivelyDeleteRegistryTree(
                  winsock2ParametersKey,
                  "Protocol_Catalog"
                  );

        (VOID)RecursivelyDeleteRegistryTree(
                  winsock2ParametersKey,
                  "Protocol_Catalog0"
                  );

        (VOID)RecursivelyDeleteRegistryTree(
                  winsock2ParametersKey,
                  "Protocol_Catalog4"
                  );

        (VOID)RecursivelyDeleteRegistryTree(
                  winsock2ParametersKey,
                  "Protocol_Catalog5"
                  );

        error = NO_ERROR;

    }

    //
    // Delete the setup migration key.
    //

    error = RecursivelyDeleteRegistryTree(
                winsockKey,
                MIGRATION_SUBKEY
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Success!
    //

exit:

    if( servicesKey != NULL ) {

        (VOID)RegCloseKey( servicesKey );

    }

    if( winsockKey != NULL ) {

        (VOID)RegCloseKey( winsockKey );

    }

    if( winsock2ParametersKey != NULL ) {

        (VOID)RegCloseKey( winsock2ParametersKey );

    }

    return error;

}   // CleanWinsock2Tree


DWORD
OpenServicesRoot(
    PHKEY ServicesKey
    )

/*++

Routine Description:

    Opens the HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    registry key.

Arguments:

    ServicesKey - Will receive a handle to the registry key if successful.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;

    //
    // Open the key.
    //

    error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                SERVICES_KEY,
                0,
                KEY_ALL_ACCESS,
                ServicesKey
                );

    return error;

}   // OpenServicesRoot


DWORD
OpenWinsockRoot(
    HKEY ServicesKey,
    PHKEY WinsockKey
    )

/*++

Routine Description:

    Opens the HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\WinSock
    registry key.

Arguments:

    ServicesKey - The handle to the ...\Services key.

    WinsockKey - Will receive a handle to the WinSock key if successful.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;

    //
    // Open the WinSock key.
    //

    error = RegOpenKeyEx(
                ServicesKey,
                WINSOCK_SUBKEY,
                0,
                KEY_ALL_ACCESS,
                WinsockKey
                );

    return error;

}   // OpenWinsockRoot


DWORD
OpenWinsock2ParametersRoot(
    HKEY ServicesKey,
    PHKEY Winsock2ParametersKey
    )

/*++

Routine Description:

    Opens the HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\...
    ...\WinSock2\Parameters registry key.

Arguments:

    ServicesKey - The handle to the ...\Services key.

    Winsock2ParametersKey - Will receive a handle to the WinSock2
        parameters key if successful.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;

    //
    // Open the WinSock2 parameters key.
    //

    error = RegOpenKeyEx(
                ServicesKey,
                WINSOCK2_PARAM_SUBKEY,
                0,
                KEY_ALL_ACCESS,
                Winsock2ParametersKey
                );

    return error;

}   // OpenWinsock2ParametersRoot


DWORD
RecursivelyDeleteRegistryTree(
    HKEY RootKey,
    LPSTR TargetKeyName
    )

/*++

Routine Description:

    Deletes the specified registry key and all subkeys.

Arguments:

    RootKey - A handle to the registry key containing the key to delete.

    TargetKeyName - The name of the key to delete.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    HKEY targetKey;
    FILETIME lastWriteTime;
    DWORD subkeyIndex;
    DWORD subkeyNameLength;
    CHAR subkeyName[MAX_REGISTRY_NAME];

    //
    // Setup locals so we know how to cleanup on exit.
    //

    targetKey = NULL;

    //
    // Open the target key.
    //

    error = RegOpenKeyEx(
                RootKey,
                TargetKeyName,
                0,
                KEY_ALL_ACCESS,
                &targetKey
                );

    if( error == ERROR_FILE_NOT_FOUND ) {

        error = NO_ERROR;
        goto exit;

    } else if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Enumerate & recursively delete the subkeys.
    //

    subkeyIndex = 0;

    for( ; ; ) {

        subkeyNameLength = sizeof(subkeyName);

        error = RegEnumKeyEx(
                    targetKey,
                    subkeyIndex,
                    subkeyName,
                    &subkeyNameLength,
                    NULL,
                    NULL,
                    NULL,
                    &lastWriteTime
                    );

        if( error != NO_ERROR ) {

            break;

        }

        error = RecursivelyDeleteRegistryTree(
                    targetKey,
                    subkeyName
                    );

        if( error != NO_ERROR ) {

            break;

        }

        //
        // Note that since we just totally blew away the current subkey
        // in the enumeration, we do not want to increment the subkey
        // index.
        //

    }

    if( error != ERROR_NO_MORE_ITEMS ) {

        goto exit;

    }

    //
    // Close the target key.
    //

    error = RegCloseKey( targetKey );

    targetKey = NULL;

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Delete the target key.
    //

    error = RegDeleteKey(
                RootKey,
                TargetKeyName
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Success!
    //

exit:

    if( targetKey != NULL ) {

        (VOID)RegCloseKey( targetKey );

    }

    return error;

}   // RecursivelyDeleteRegistryTree

