/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    setup.c

Abstract:

    This module contains code for migrating WinSock 1.1 setup information to
    the new WinSock 2.0 structure.

    The basic strategy keeps a private copy of the Winsock 1.1 protocol data
    off to the side and compares this saved data with the current data when
    necessary. MigrateWinsockConfiguration() is invoked by NT Setup
    immediately after binding review, and at an appropriate point during the
    upgrade process.

    MigrateWinsockConfiguration() uses the following registry structure:

        HKEY_LOCAL_MACHINE
          System
            CurrentControlSet
              Services
                WinSock
                  Setup Migration
                    Setup Version = REG_DWORD {...}
                    Provider List = REG_MULTI_SZ "provider1 provider2..."
                    Known Static Providers = REG_MULTI_SZ "provider1 provider2..."
                    Providers
                      provider1
                        WinSock 1.1 Provider Data = REG_BINARY {...}
                        WinSock 2.0 Provider ID = REG_BINARY {...}
                      provider2
                        WinSock 1.1 Provider Data = REG_BINARY {...}
                        WinSock 2.0 Provider ID = REG_BINARY {...}
                      ...
                    Well Known Guids
                      provider1 = REG_BINARY {...}
                      provider2 = REG_BINARY {...}
                      ...

    Where:

        Setup Version - Contains a version number of the migration code.
            This exists for forward compatibility. If future versions of
            the migration code have a differing registry layout, they can
            key off this version number and optionally blow away the entire
            Setup Migration tree.

        Provider List - Contains a copy of the WinSock\Parameters\Transports
            value from a previous invocation of MigrateWinsockConfiguration().
            Keeping a copy of this data allows us to quickly determine which
            transports have been added or deleted.

        Known Static Providers - Some transports (such as NetBIOS) are LANA
            based, and therefore change their supported triples depending on
            installed hardware. To reduce registry space, and to also reduce
            thrashing of WinSock 1.1 providers and avoid calling
            WSHEnumProtocols() unnecessarily, this value keeps a list of
            providers that are known to not change their supported triples
            based on installed hardware. Examples of providers eligible for
            this list are Tcpip, NwlnkIpx, and NwlnkSpx. TP4 and XNS are
            probably eligible as well.

        Providers - This subkey contains a catalog of installed WinSock 1.1
            providers. Each installed provider has an individual subkey
            beneath this key.

        WinSock 1.1 Provider Data - Contains the raw binary data returned by
            the provider's WSHEnumProtocols() entrypoint. This data is stored
            in self-relative form (embedded pointers are mapped to structure
            offsets before writing the data to the registry). This value does
            not exist for known static providers.

        WinSock 2.0 Provider ID - Contains the provider GUID identifying the
            provider. This value has three possible sources:

                1. The provider's helper DLL, if the WSHGetProviderGuid()
                   entrypoint is supported.

                2. A hard-coded list of "well known" GUIDs for select
                   providers.

                3. Created on-the-fly.

        Well Known Guids - A catalog of well known provider GUIDs. We'd
            really like a given provider to always have the same GUID on all
            machines. For providers in which we have control over the helper
            DLL, we'll add the WSHGetProviderGuid() entrypoint. For known
            providers in which we do not have control over the helper DLL,
            we'll create a GUID for the provider and add it to this catalog.
            All other providers (basically, anything unknown) will have a
            GUID created on-the-fly and added to this catalog. These providers
            will not have the same GUID on all machines, but at least they
            will have the same GUID if removed & reinstalled on a particular
            machine.

Author:

    Keith Moore (keithmo)        31-Oct-1995

Revision History:

--*/


#define UNICODE
#define _UNICODE
#include "winsockp.h"
#include <rpc.h>
#include <nspapi.h>
#include <osdef.h>
#include <tchar.h>


//
// Private constants.
//

#define ALLOC_MEM(cb)           ALLOCATE_HEAP(cb)   // These macros isolate
#define FREE_MEM(p)             FREE_HEAP(p)        // this module from
#define DBG_ASSERT(exp)         WS_ASSERT(exp)      // WSOCK32.DLL, in case
#define DBG_PRINT               WS_PRINT            // this is moved to
#define IF_DEBUG_SETUP          IF_DEBUG( SETUP )   // a separate DLL.

#define WINSOCK_SETUP_VERSION   0x1009      // Update for major setup changes!
#define WINSOCK_SPI_VERSION     2

#define MAX_REGISTRY_NAME       256

#define DEFAULT_PROVIDER_PATH   TEXT("%SystemRoot%\\system32\\msafd.dll")

#define SERVICES_KEY            TEXT("System\\CurrentControlSet\\Services")
#define WINSOCK_SUBKEY          TEXT("WinSock")
#define MIGRATION_SUBKEY        TEXT("Setup Migration")
#define PROVIDERS_SUBKEY        TEXT("Providers")
#define PARAMETERS_SUBKEY       TEXT("Parameters")
#define SERVICE_PARAMS_SUBKEY   TEXT("Parameters\\WinSock")
#define WELL_KNOWN_GUIDS_SUBKEY TEXT("Well Known Guids")

#define SETUP_VERSION_VALUE     TEXT("Setup Version")
#define TRANSPORTS_VALUE        TEXT("Transports")
#define MAPPING_VALUE           TEXT("Mapping")
#define PROVIDER_LIST_VALUE     TEXT("Provider List")
#define KNOWN_STATIC_VALUE      TEXT("Known Static Providers")
#define WINSOCK_1_1_DATA_VALUE  TEXT("WinSock 1.1 Provider Data")
#define WINSOCK_2_0_ID_VALUE    TEXT("WinSock 2.0 Provider ID")
#define HELPER_DLL_NAME_VALUE   TEXT("HelperDllName")

#define COMMON_SERVICE_FLAGS    ( XP_CONNECTIONLESS         | \
                                  XP_GUARANTEED_DELIVERY    | \
                                  XP_GUARANTEED_ORDER       | \
                                  XP_MESSAGE_ORIENTED       | \
                                  XP_PSEUDO_STREAM          | \
                                  XP_GRACEFUL_CLOSE         | \
                                  XP_EXPEDITED_DATA         | \
                                  XP_CONNECT_DATA           | \
                                  XP_DISCONNECT_DATA        | \
                                  XP_SUPPORTS_BROADCAST     | \
                                  XP_SUPPORTS_MULTICAST     | \
                                  XP_BANDWIDTH_ALLOCATION )

#define FORCED_SERVICE_FLAGS    XP1_IFS_HANDLES


//
// The following macro makes invoking the user's callback a little
// prettier. The macro makes the following assumptions:
//
//      1. The callback parameter is named "Callback".
//
//      2. The context parameter is named "Context".
//
//      3. A label for premature exit exists and is named "exit".
//

#define INVOKE_CALLBACK(op,p)                               \
            if( Callback != NULL ) {                        \
                if( !(*Callback)(                           \
                        (op),                               \
                        (p),                                \
                        Context                             \
                        ) ) {                               \
                    goto exit;                              \
                }                                           \
            } else


//
// Private types.
//

typedef struct _NAME_GUID_PAIR {

    LPTSTR Name;
    GUID Guid;

} NAME_GUID_PAIR, *LPNAME_GUID_PAIR;


//
// Private globals.
//

HMODULE Winsock2DllHandle = NULL;
LPWSCDEINSTALLPROVIDER WSCDeinstallProviderProc = NULL;
LPWSCINSTALLPROVIDER WSCInstallProviderProc = NULL;

DWORD DefaultSetupVersion = WINSOCK_SETUP_VERSION;
TCHAR DefaultProviderList[] = TEXT("");
TCHAR DefaultKnownStaticProviders[] = TEXT("Tcpip\0")
                                      TEXT("NwlnkIpx\0")
                                      TEXT("NwlnkSpx\0")
                                      TEXT("AppleTalk\0")
                                      TEXT("IsoTp\0");

NAME_GUID_PAIR DefaultWellKnownGuids[] =
    {
        {
            TEXT("IsoTp"),
            { /* 89e4cbb0-b9c1-11cf-95c8-00805f48a192 */
                0x89e4cbb0,
                0xb9c1,
                0x11cf,
                {0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}
            }
        },

        {
            TEXT("McsXns"),
            { /* 89e4cbb1-b9c1-11cf-95c8-00805f48a192 */
                0x89e4cbb1,
                0xb9c1,
                0x11cf,
                {0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}
            }
        },

        {
            TEXT("AppleTalk"),
            { /* 2c3b17a0-c6df-11cf-95c8-00805f48a192 */
                0x2c3b17a0,
                0xc6df,
                0x11cf,
                {0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}
            }
        }
    };

#define NUM_DEFAULT_WELL_KNOWN_GUIDS \
            (sizeof(DefaultWellKnownGuids) / sizeof(DefaultWellKnownGuids[0]))


//
// Private prototypes.
//

DWORD
InitializeSetup(
    VOID
    );

VOID
TerminateSetup(
    VOID
    );

BOOL
IsStringInMultiSz(
    LPTSTR MultiSz,
    LPTSTR String
    );

DWORD
ReadDword(
    HKEY RootKey,
    LPTSTR ValueName,
    LPDWORD Value
    );

DWORD
ReadMultiSz(
    HKEY RootKey,
    LPTSTR ValueName,
    LPTSTR FAR * Value
    );

DWORD
ReadBinary(
    HKEY RootKey,
    LPTSTR ValueName,
    LPVOID FAR * Value,
    LPDWORD ValueLength
    );

DWORD
ReadString(
    HKEY RootKey,
    LPTSTR ValueName,
    LPTSTR FAR * Value
    );

DWORD
ReadGuid(
    HKEY RootKey,
    LPTSTR ValueName,
    LPGUID Value
    );

DWORD
WriteDword(
    HKEY RootKey,
    LPTSTR ValueName,
    DWORD Value
    );

DWORD
WriteMultiSz(
    HKEY RootKey,
    LPTSTR ValueName,
    LPTSTR Value
    );

DWORD
WriteBinary(
    HKEY RootKey,
    LPTSTR ValueName,
    LPVOID Value,
    DWORD ValueLength
    );

DWORD
WriteString(
    HKEY RootKey,
    LPTSTR ValueName,
    LPTSTR Value
    );

DWORD
WriteGuid(
    HKEY RootKey,
    LPTSTR ValueName,
    LPGUID Value
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
OpenSetupMigrationRoot(
    HKEY WinsockKey,
    PHKEY MigrationKey,
    PHKEY ProvidersKey,
    PHKEY WellKnownGuidsKey
    );

DWORD
ReadNewProviderList(
    HKEY WinsockKey,
    LPTSTR FAR * NewProviderList
    );

DWORD
ReadOldProviderList(
    HKEY MigrationKey,
    LPTSTR FAR * OldProviderList
    );

DWORD
ReadKnownStaticProviderList(
    HKEY MigrationKey,
    LPTSTR FAR * KnownStaticProviderList
    );

DWORD
ReadProviderId(
    HKEY ProvidersKey,
    LPTSTR ProviderName,
    LPGUID ProviderId
    );

DWORD
CreateDefaultSetupMigrationTree(
    HKEY WinsockKey,
    PHKEY MigrationKey,
    PHKEY ProvidersKey,
    PHKEY WellKnownGuidsKey
    );

DWORD
RecursivelyDeleteRegistryTree(
    HKEY RootKey,
    LPTSTR TargetKeyName
    );

DWORD
RemoveProviderByName(
    HKEY ProvidersKey,
    LPTSTR ProviderName
    );

DWORD
ReadProtocolDataFromRegistry(
    HKEY ProvidersKey,
    LPTSTR ProviderName,
    LPPROTOCOL_INFO FAR * RegistryInfo11,
    LPDWORD RegistryInfo11Length
    );

DWORD
ReadProtocolDataFromProvider(
    HKEY ServicesKey,
    LPTSTR ProviderName,
    LPTSTR ProviderDllPath,
    LPPROTOCOL_INFO FAR * ProtocolInfo11,
    LPDWORD ProtocolInfo11Length,
    LPDWORD ProtocolInfo11Entries
    );

DWORD
ReadProviderSupportedProtocolsFromRegistry(
    HKEY ParametersKey,
    LPDWORD FAR * ProtocolList
    );

VOID
MapProtocolInfoToSelfRelative(
    LPPROTOCOL_INFO ProtocolInfo11,
    DWORD ProtocolInfo11Entries
    );

VOID
MapProtocolInfoToAbsolute(
    LPPROTOCOL_INFO ProtocolInfo11,
    DWORD ProtocolInfo11Entries
    );

DWORD
BuildWinsock2ProtocolList(
    LPTSTR ProviderName,
    LPTSTR ProviderDllPath,
    LPPROTOCOL_INFO ProtocolInfo11,
    DWORD ProtocolInfo11Entries,
    LPWSAPROTOCOL_INFO FAR * ProtocolInfo2,
    LPDWORD ProtocolInfo2Entries
    );

VOID
BuildNewProtocolName(
    LPTSTR ProviderName,
    LPPROTOCOL_INFO ProtocolInfo11,
    LPTSTR NewProtocolName
    );

DWORD
InstallNewProvider(
    HKEY WellKnownGuidsKey,
    HKEY ProvidersKey,
    LPTSTR ProviderName,
    LPTSTR ProviderDllPath,
    BOOL IsKnownStaticProvider,
    LPPROTOCOL_INFO ProtocolInfo11,
    DWORD ProtocolInfo11Length,
    DWORD ProtocolInfo11Entries
    );

DWORD
CreateMigrationRegistryForProvider(
    HKEY ProvidersKey,
    LPTSTR ProviderName,
    BOOL IsKnownStaticProvider,
    LPPROTOCOL_INFO ProtocolInfo11,
    DWORD ProtocolInfo11Length,
    LPGUID ProviderId
    );

DWORD
CreateProtocolCatalogMutex(
    LPHANDLE Handle
    );

DWORD
AcquireProtocolCatalogMutex(
    HANDLE Handle
    );

DWORD
ReleaseProtocolCatalogMutex(
    HANDLE Handle
    );

DWORD
RemoveAllInstalledProviders(
    HKEY ProvidersKey
    );

DWORD
AppendStringToMultiSz(
    LPTSTR * MultiSz,
    LPTSTR String
    );

DWORD
SanitizeWinsock2ConfigForProvider(
    LPGUID ProviderId
    );

DWORD
DetermineGuidForProvider(
    HKEY WellKnownGuidsKey,
    LPTSTR ProviderName,
    LPTSTR ProviderDllPath,
    LPGUID ProviderId
    );


//
// Public functions.
//


DWORD
MigrateWinsockConfiguration(
    LPWSA_SETUP_DISPOSITION Disposition,
    LPFN_WSA_SETUP_CALLBACK Callback OPTIONAL,
    DWORD Context OPTIONAL
    )

/*++

Routine Description:

    This entrypoint is called by NT Setup whenever WinSock 1.1 setup
    changes might need to be migrated to WinSock 2.0. This entrypoint
    is typically called immediately after binding review, and at an
    appropriate point during system upgrade.

Arguments:

    Disposition - Points to a WSA_SETUP_DISPOSITION value that allows
        this routine to indicate to the caller the scope of any changes
        made. If this function is successful, then this enum will receive
        one of the following values:

            WsaSetupNoChangesMade - There were no changes made to the
                existing WinSock 2.0 configuration.

            WsaSetupChangesMadeRebootNotNecessary - There were changes
                made to the WinSock 2.0 configuration, and a reboot of
                the system is not necessary.

            WsaSetupChangesMadeRebootRequired - There were changes made
                to the WinSock 2.0 configuration, and a reboot of the
                system is required.

    Callback - An optional pointer to a callback function invoked at
        strategic points in the migration process. The callback function
        has the following prototype:

            BOOL
            CALLBACK
            WsaSetupCallback(
                WSA_SETUP_OPCODE Opcode,
                LPVOID Parameter,
                Context
                );

        Where:

            Opcode - Specifies the type of indication being made. This
                may be one of the following values:

                    WsaSetupInstallingProvider - A new provider is
                        being installed. Parameter points to the name
                        of the new provider.

                    WsaSetupRemovingProvider - An existing provider is
                        being removed. Parameter points to the name of
                        the existing provider.

                    WsaSetupValidatingProvider - An existing provider is
                        being validated to determine if it needs to be
                        updated. Parameter points to the name of the
                        existing provider.

                    WsaSetupUpdatingProvider - An existing provider is
                        being updated. Parameter points to the name of
                        the existing proivder.

            Parameter - A generic parameter whose value depends upon the
                current Opcode (see above).

            Context - The context value passed into the migration function.

        The callback should return TRUE if the migration process is to
        continue, FALSE if it should be immediately terminated.

    Context - An uninterpreted context value to be passed to the callback
        function.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    HKEY servicesKey;
    HKEY winsockKey;
    HKEY migrationKey;
    HKEY providersKey;
    HKEY wellKnownGuidsKey;
    LPTSTR oldProviders;
    LPTSTR newProviders;
    LPTSTR knownStaticProviders;
    LPTSTR providerName;
    LPTSTR updatedProviderList;
    INT result;
    LPPROTOCOL_INFO protocolInfo11;
    DWORD protocolInfo11Length;
    DWORD protocolInfo11Entries;
    LPPROTOCOL_INFO registryInfo11;
    DWORD registryInfo11Length;
    WSA_SETUP_DISPOSITION disposition;
    TCHAR providerDllPath[MAX_PATH];

    //
    // Sanity check.
    //

    DBG_ASSERT( Disposition != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    servicesKey = NULL;
    winsockKey = NULL;
    migrationKey = NULL;
    providersKey = NULL;
    wellKnownGuidsKey = NULL;
    oldProviders = NULL;
    newProviders = NULL;
    updatedProviderList = NULL;
    knownStaticProviders = NULL;
    protocolInfo11 = NULL;
    registryInfo11 = NULL;

    //
    // Assume no changes will be made.
    //

    disposition = WsaSetupNoChangesMade;

    //
    // Initialize things.
    //

    error = InitializeSetup();

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Open the necessary registry keys.
    //

    error = OpenServicesRoot(
                &servicesKey
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    error = OpenWinsockRoot(
                servicesKey,
                &winsockKey
                );

    if( error != NO_ERROR ) {

        if( error == ERROR_FILE_NOT_FOUND ) {

            error = NO_ERROR;

        }

        goto exit;

    }

    error = OpenSetupMigrationRoot(
                winsockKey,
                &migrationKey,
                &providersKey,
                &wellKnownGuidsKey
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Read the new & old provider lists.
    //

    error = ReadNewProviderList(
                winsockKey,
                &newProviders
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    error = ReadOldProviderList(
                migrationKey,
                &oldProviders
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    error = ReadKnownStaticProviderList(
                migrationKey,
                &knownStaticProviders
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Scan the old provider list, and remove any providers that are
    // not in the new provider list.
    //

    for( providerName = oldProviders ;
         *providerName != TEXT('\0') ;
         providerName += _tcslen( providerName ) + 1 ) {

        if( !IsStringInMultiSz( newProviders, providerName ) ) {

            INVOKE_CALLBACK(
                WsaSetupRemovingProvider,
                providerName
                );

            error = RemoveProviderByName(
                        providersKey,
                        providerName
                        );

            if( error != NO_ERROR ) {

                goto exit;

            }

            disposition = WsaSetupChangesMadeRebootNotNecessary;

        }

    }

    //
    // Scan the new provider list, and add any providers that are not
    // in the old provider list.
    //

    for( providerName = newProviders ;
         *providerName != TEXT('\0') ;
         providerName += _tcslen( providerName ) + 1 ) {

        if( !IsStringInMultiSz( oldProviders, providerName ) ) {

            INVOKE_CALLBACK(
                WsaSetupInstallingProvider,
                providerName
                );

            error = ReadProtocolDataFromProvider(
                        servicesKey,
                        providerName,
                        providerDllPath,
                        &protocolInfo11,
                        &protocolInfo11Length,
                        &protocolInfo11Entries
                        );

            if( error != NO_ERROR ) {

                goto exit;

            }

            if( protocolInfo11 == NULL ) {

                DBG_PRINT((
                    "%ls returned zero protocol entries!?!\n",
                    providerName
                    ));

                DBG_ASSERT( protocolInfo11Length == 0 );
                DBG_ASSERT( protocolInfo11Entries == 0 );

                continue;

            }

            error = InstallNewProvider(
                        wellKnownGuidsKey,
                        providersKey,
                        providerName,
                        providerDllPath,
                        IsStringInMultiSz( knownStaticProviders, providerName ),
                        protocolInfo11,
                        protocolInfo11Length,
                        protocolInfo11Entries
                        );

            if( error == NO_ERROR ) {

                error = AppendStringToMultiSz(
                            &updatedProviderList,
                            providerName
                            );

                if( error != NO_ERROR ) {

                    goto exit;

                }

            } else {

                DBG_PRINT((
                    "cannot install %ls, error %d, skipping\n",
                    providerName,
                    error
                    ));

                error = NO_ERROR;

            }

            FREE_MEM( protocolInfo11 );
            protocolInfo11 = NULL;

            disposition = WsaSetupChangesMadeRebootNotNecessary;

        }

    }

    //
    // Finally, scan for dynamic entries that need to be updated. We'll
    // determine if a provider needs to be updated by reading the protocol
    // data stored in the registry, and also retrieving the protocol data
    // directly from the provider. If these two blocks of data do not
    // EXACTLY match, then we'll remove the old provider & reinstall it
    // using the protocol data retrieved from the provider.
    //

    for( providerName = newProviders ;
         *providerName != TEXT('\0') ;
         providerName += _tcslen( providerName ) + 1 ) {

        if( IsStringInMultiSz( oldProviders, providerName ) ) {

            if( IsStringInMultiSz( knownStaticProviders, providerName ) ) {

                error = AppendStringToMultiSz(
                            &updatedProviderList,
                            providerName
                            );

                if( error != NO_ERROR ) {

                    goto exit;

                }

                continue;

            }

            INVOKE_CALLBACK(
                WsaSetupValidatingProvider,
                providerName
                );

            error = ReadProtocolDataFromRegistry(
                        providersKey,
                        providerName,
                        &registryInfo11,
                        &registryInfo11Length
                        );

            if( error == ERROR_FILE_NOT_FOUND ) {

                DBG_PRINT((
                    "no registry data for %ls?!?\n",
                    providerName
                    ));

                error = NO_ERROR;
                registryInfo11 = NULL;
                registryInfo11Length = 0;

            }

            if( error != NO_ERROR ) {

                DBG_PRINT((
                    "cannot read registry data for %ls, error %lu\n",
                    providerName,
                    error
                    ));

                goto exit;

            }

            error = ReadProtocolDataFromProvider(
                        servicesKey,
                        providerName,
                        providerDllPath,
                        &protocolInfo11,
                        &protocolInfo11Length,
                        &protocolInfo11Entries
                        );

            if( error != NO_ERROR ) {

                goto exit;

            }

            if( protocolInfo11 == NULL ) {

                if( registryInfo11 != NULL ) {

                    FREE_MEM( registryInfo11 );
                    registryInfo11 = NULL;

                }

                DBG_PRINT((
                    "%ls returned zero protocol entries!?!\n",
                    providerName
                    ));

                DBG_ASSERT( protocolInfo11Length == 0 );
                DBG_ASSERT( protocolInfo11Entries == 0 );

                continue;

            }

            MapProtocolInfoToSelfRelative(
                protocolInfo11,
                protocolInfo11Entries
                );

            if( registryInfo11Length == protocolInfo11Length &&
                RtlEqualMemory(
                    registryInfo11,
                    protocolInfo11,
                    protocolInfo11Length
                    ) ) {

                //
                // They match, so add the provider to the list.
                //

                error = AppendStringToMultiSz(
                            &updatedProviderList,
                            providerName
                            );

                if( error != NO_ERROR ) {

                    goto exit;

                }

            } else {

                //
                // They don't match, so remove the provider & reinstall
                // using the protocol information read from the provider.
                //

                INVOKE_CALLBACK(
                    WsaSetupUpdatingProvider,
                    providerName
                    );

                error = RemoveProviderByName(
                            providersKey,
                            providerName
                            );

                if( error != NO_ERROR ) {

                    goto exit;

                }

                MapProtocolInfoToAbsolute(
                    protocolInfo11,
                    protocolInfo11Entries
                    );

                error = InstallNewProvider(
                            wellKnownGuidsKey,
                            providersKey,
                            providerName,
                            providerDllPath,
                            FALSE,
                            protocolInfo11,
                            protocolInfo11Length,
                            protocolInfo11Entries
                            );

                if( error == NO_ERROR ) {

                    error = AppendStringToMultiSz(
                                &updatedProviderList,
                                providerName
                                );

                    if( error != NO_ERROR ) {

                        goto exit;

                    }

                } else {

                    DBG_PRINT((
                        "cannot install %ls, error %d, skipping\n",
                        providerName,
                        error
                        ));

                    error = NO_ERROR;

                }

                disposition = WsaSetupChangesMadeRebootNotNecessary;

            }

            if( protocolInfo11 != NULL ) {

                FREE_MEM( protocolInfo11 );
                protocolInfo11 = NULL;

            }

            if( registryInfo11 != NULL ) {

                FREE_MEM( registryInfo11 );
                registryInfo11 = NULL;

            }

        }

    }

    //
    // Update the provider list if necessary.
    //

    if( disposition != WsaSetupNoChangesMade ) {

        error = WriteMultiSz(
                    migrationKey,
                    PROVIDER_LIST_VALUE,
                    updatedProviderList
                    );

        if( error != NO_ERROR ) {

            goto exit;

        }

    }

exit:

    if( servicesKey != NULL ) {

        (VOID)RegCloseKey( servicesKey );

    }

    if( winsockKey != NULL ) {

        (VOID)RegCloseKey( winsockKey );

    }

    if( migrationKey != NULL ) {

        (VOID)RegCloseKey( migrationKey );

    }

    if( providersKey != NULL ) {

        (VOID)RegCloseKey( providersKey );

    }

    if( wellKnownGuidsKey != NULL ) {

        (VOID)RegCloseKey( wellKnownGuidsKey );

    }

    if( oldProviders != NULL ) {

        FREE_MEM( oldProviders );

    }

    if( newProviders != NULL ) {

        FREE_MEM( newProviders );

    }

    if( updatedProviderList != NULL ) {

        FREE_MEM( updatedProviderList );

    }

    if( knownStaticProviders != NULL ) {

        FREE_MEM( knownStaticProviders );

    }

    if( protocolInfo11 != NULL ) {

        FREE_MEM( protocolInfo11 );

    }

    if( registryInfo11 != NULL ) {

        FREE_MEM( registryInfo11 );

    }


    TerminateSetup();

    *Disposition = disposition;

    return error;

}   // MigrateWinsockConfiguration


//
// Private functions.
//

DWORD
InitializeSetup(
    VOID
    )

/*++

Routine Description:

    Performs any global initialization necessary for this module.

Arguments:

    None.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;

    //
    // Load the WinSock 2.0 DLL.
    //

    Winsock2DllHandle = LoadLibrary( TEXT("WS2_32.DLL") );

    if( Winsock2DllHandle == NULL ) {

        error = GetLastError();
        goto exit;

    }

    //
    // Find the entrypoints.
    //

    WSCDeinstallProviderProc = (PVOID)GetProcAddress(
                                          Winsock2DllHandle,
                                          "WSCDeinstallProvider"
                                          );

    if( WSCDeinstallProviderProc == NULL ) {

        error = GetLastError();
        goto exit;

    }

    WSCInstallProviderProc = (PVOID)GetProcAddress(
                                        Winsock2DllHandle,
                                        "WSCInstallProvider"
                                        );

    if( WSCInstallProviderProc == NULL ) {

        error = GetLastError();
        goto exit;

    }

    //
    // Success!
    //

    error = NO_ERROR;

exit:

    if( error != NO_ERROR ) {

        TerminateSetup();

    }

    return error;

}   // InitializeSetup


VOID
TerminateSetup(
    VOID
    )

/*++

Routine Description:

    Performs any global cleanup necessary for this module.

Arguments:

    None.

Return Value:

    None.

--*/

{

    if( Winsock2DllHandle != NULL ) {

        FreeLibrary( Winsock2DllHandle );
        Winsock2DllHandle = NULL;

    }

    WSCDeinstallProviderProc = NULL;
    WSCInstallProviderProc = NULL;

}   // TerminateSetup


BOOL
IsStringInMultiSz(
    LPTSTR MultiSz,
    LPTSTR String
    )

/*++

Routine Description:

    Searches a REG_MULTI_SZ value for the specified string.

Arguments:

    MultiSz - The REG_MULTI_SZ value to search.

    String - The string to search for.

Return Value:

    BOOL - TRUE if the string was found, FALSE otherwise.

--*/

{

    //
    // Sanity check.
    //

    DBG_ASSERT( MultiSz != NULL );
    DBG_ASSERT( String != NULL );
    DBG_ASSERT( *String != TEXT('\0') );

    //
    // Scan it.
    //

    while( *MultiSz != TEXT('\0') ) {

        if( _tcsicmp( MultiSz, String ) == 0 ) {

            return TRUE;

        }

        MultiSz += _tcslen( MultiSz ) + 1;

    }

    return FALSE;

}   // IsStringInMultiSz


DWORD
ReadDword(
    HKEY RootKey,
    LPTSTR ValueName,
    LPDWORD Value
    )

/*++

Routine Description:

    Reads a DWORD value from the registry.

Arguments:

    RootKey - The root key containing the value to read.

    ValueName - The name of the value to read.

    Value - Will receive the read DWORD if successful.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    DWORD valueType;
    DWORD valueLength;

    //
    // Sanity check.
    //

    DBG_ASSERT( RootKey != NULL );
    DBG_ASSERT( ValueName != NULL );
    DBG_ASSERT( Value != NULL );

    //
    // Read the data.
    //

    valueLength = sizeof(*Value);

    error = RegQueryValueEx(
                RootKey,
                ValueName,
                NULL,
                &valueType,
                (LPBYTE)Value,
                &valueLength
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // If the type is not REG_DWORD, then fail the request.
    //

    if( valueType != REG_DWORD ) {

        error = ERROR_INVALID_DATATYPE;
        goto exit;

    }

    //
    // Success!
    //

exit:

    return error;

}   // ReadDword


DWORD
ReadMultiSz(
    HKEY RootKey,
    LPTSTR ValueName,
    LPTSTR FAR * Value
    )

/*++

Routine Description:

    Reads a MULTI_SZ value from the registry.

Arguments:

    RootKey - The root key containing the value to read.

    ValueName - The name of the value to read.

    Value - Will receive a pointer to the read MULTI_SZ if successful.
        Note that it is the caller's responsibility to free this
        memory.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    LPTSTR valueBuffer;
    DWORD valueBufferLength;
    DWORD valueType;

    //
    // Sanity check.
    //

    DBG_ASSERT( RootKey != NULL );
    DBG_ASSERT( ValueName != NULL );
    DBG_ASSERT( Value != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    valueBuffer = NULL;
    valueBufferLength = 0;

    //
    // Determine the value length.
    //

    error = RegQueryValueEx(
                RootKey,
                ValueName,
                NULL,
                &valueType,
                (LPBYTE)valueBuffer,
                &valueBufferLength
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Allocate a buffer for the value.
    //

    valueBuffer = ALLOC_MEM( valueBufferLength );

    if( valueBuffer == NULL ) {

        error = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;

    }

    //
    // And now read the data.
    //

    error = RegQueryValueEx(
                RootKey,
                ValueName,
                NULL,
                &valueType,
                (LPBYTE)valueBuffer,
                &valueBufferLength
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // If the type is not REG_MULTI_SZ, then fail the request.
    //

    if( valueType != REG_MULTI_SZ ) {

        error = ERROR_INVALID_DATATYPE;
        goto exit;

    }

    //
    // Success!
    //

    *Value = valueBuffer;

exit:

    if( error != NO_ERROR && valueBuffer != NULL ) {

        FREE_MEM( valueBuffer );

    }

    return error;

}   // ReadMultiSz


DWORD
ReadString(
    HKEY RootKey,
    LPTSTR ValueName,
    LPTSTR FAR * Value
    )

/*++

Routine Description:

    Reads a string value from the registry.

Arguments:

    RootKey - The root key containing the value to read.

    ValueName - The name of the value to read.

    Value - Will receive a pointer to the read string if successful.
        Note that it is the caller's responsibility to free this
        memory.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    LPTSTR valueBuffer;
    DWORD valueBufferLength;
    DWORD valueType;

    //
    // Sanity check.
    //

    DBG_ASSERT( RootKey != NULL );
    DBG_ASSERT( ValueName != NULL );
    DBG_ASSERT( Value != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    valueBuffer = NULL;
    valueBufferLength = 0;

    //
    // Determine the value length.
    //

    error = RegQueryValueEx(
                RootKey,
                ValueName,
                NULL,
                &valueType,
                (LPBYTE)valueBuffer,
                &valueBufferLength
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Allocate a buffer for the value.
    //

    valueBuffer = ALLOC_MEM( valueBufferLength );

    if( valueBuffer == NULL ) {

        error = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;

    }

    //
    // And now read the data.
    //

    error = RegQueryValueEx(
                RootKey,
                ValueName,
                NULL,
                &valueType,
                (LPBYTE)valueBuffer,
                &valueBufferLength
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // If the type is not REG_SZ or REG_EXPAND_SZ, then fail the request.
    //

    if( valueType != REG_SZ && valueType != REG_EXPAND_SZ ) {

        error = ERROR_INVALID_DATATYPE;
        goto exit;

    }

    //
    // Success!
    //

    *Value = valueBuffer;

exit:

    if( error != NO_ERROR && valueBuffer != NULL ) {

        FREE_MEM( valueBuffer );

    }

    return error;

}   // ReadString


DWORD
ReadBinary(
    HKEY RootKey,
    LPTSTR ValueName,
    LPVOID FAR * Value,
    LPDWORD ValueLength
    )

/*++

Routine Description:

    Reads a raw binary value from the registry.

Arguments:

    RootKey - The root key containing the value to read.

    ValueName - The name of the value to read.

    Value - Will receive a pointer to the read binary data if successful.
        Note that it is the caller's responsibility to free this
        memory.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    LPVOID valueBuffer;
    DWORD valueBufferLength;
    DWORD valueType;

    //
    // Sanity check.
    //

    DBG_ASSERT( RootKey != NULL );
    DBG_ASSERT( ValueName != NULL );
    DBG_ASSERT( Value != NULL );
    DBG_ASSERT( ValueLength != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    valueBuffer = NULL;
    valueBufferLength = 0;

    //
    // Determine the value length.
    //

    error = RegQueryValueEx(
                RootKey,
                ValueName,
                NULL,
                &valueType,
                valueBuffer,
                &valueBufferLength
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Allocate a buffer for the value.
    //

    valueBuffer = ALLOC_MEM( valueBufferLength );

    if( valueBuffer == NULL ) {

        error = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;

    }

    //
    // And now read the data.
    //

    error = RegQueryValueEx(
                RootKey,
                ValueName,
                NULL,
                &valueType,
                valueBuffer,
                &valueBufferLength
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // If the type is not REG_BINARY, then fail the request.
    //

    if( valueType != REG_BINARY ) {

        error = ERROR_INVALID_DATATYPE;
        goto exit;

    }

    //
    // Success!
    //

    *Value = valueBuffer;
    *ValueLength = valueBufferLength;

exit:

    if( error != NO_ERROR && valueBuffer != NULL ) {

        FREE_MEM( valueBuffer );

    }

    return error;

}   // ReadBinary


DWORD
ReadGuid(
    HKEY RootKey,
    LPTSTR ValueName,
    LPGUID Value
    )

/*++

Routine Description:

    Reads a GUID value from the registry.

Arguments:

    RootKey - The root key containing the value to read.

    ValueName - The name of the value to read.

    Value - Will receive the read GUID if successful.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    DWORD valueType;
    DWORD valueLength;

    //
    // Sanity check.
    //

    DBG_ASSERT( RootKey != NULL );
    DBG_ASSERT( ValueName != NULL );
    DBG_ASSERT( Value != NULL );

    //
    // Read the data.
    //

    valueLength = sizeof(*Value);

    error = RegQueryValueEx(
                RootKey,
                ValueName,
                NULL,
                &valueType,
                (LPBYTE)Value,
                &valueLength
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // If the type is not REG_BINARY, then fail the request.
    //

    if( valueType != REG_BINARY ) {

        error = ERROR_INVALID_DATATYPE;
        goto exit;

    }

    //
    // Success!
    //

exit:

    return error;

}   // ReadGuid


DWORD
WriteDword(
    HKEY RootKey,
    LPTSTR ValueName,
    DWORD Value
    )

/*++

Routine Description:

    Writes a DWORD value to the registry.

Arguments:

    RootKey - The root key containing the value to write.

    ValueName - The name of the value to write.

    Value - The DWORD to write.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;

    //
    // Sanity check.
    //

    DBG_ASSERT( RootKey != NULL );
    DBG_ASSERT( ValueName != NULL );

    //
    // Write it.
    //

    error = RegSetValueEx(
                RootKey,
                ValueName,
                0,
                REG_DWORD,
                (LPBYTE)&Value,
                sizeof(Value)
                );

    return error;

}   // WriteDword


DWORD
WriteMultiSz(
    HKEY RootKey,
    LPTSTR ValueName,
    LPTSTR Value
    )

/*++

Routine Description:

    Writes a MULTI_SZ value to the registry.

Arguments:

    RootKey - The root key containing the value to write.

    ValueName - The name of the value to write.

    Value - Points to the MULTI_SZ to write.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    LPTSTR valueScan;
    DWORD scanLength;
    DWORD valueLength;

    //
    // Sanity check.
    //

    DBG_ASSERT( RootKey != NULL );
    DBG_ASSERT( ValueName != NULL );

    //
    // Compute the length of the MULTI_SZ, including the final
    // terminating '\0'.
    //

    valueLength = 0;

    if( Value == NULL ) {

        Value = TEXT("");

    } else {

        valueScan = Value;

        while( *valueScan != TEXT('\0') ) {

            scanLength = (DWORD)_tcslen( valueScan ) + 1;

            valueLength += scanLength;
            valueScan += scanLength;

        }

    }

    valueLength++;

    //
    // Write it.
    //

    error = RegSetValueEx(
                RootKey,
                ValueName,
                0,
                REG_MULTI_SZ,
                (LPBYTE)Value,
                valueLength * sizeof(TCHAR)
                );

    return error;

}   // WriteMultiSz


DWORD
WriteBinary(
    HKEY RootKey,
    LPTSTR ValueName,
    LPVOID Value,
    DWORD ValueLength
    )

/*++

Routine Description:

    Writes a raw binary value to the registry.

Arguments:

    RootKey - The root key containing the value to write.

    ValueName - The name of the value to write.

    Value - Points to the raw binary data to write.

    ValueLength - The length (in BYTEs) of the data to write.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;

    //
    // Sanity check.
    //

    DBG_ASSERT( RootKey != NULL );
    DBG_ASSERT( ValueName != NULL );
    DBG_ASSERT( Value != NULL );

    //
    // Write it.
    //

    error = RegSetValueEx(
                RootKey,
                ValueName,
                0,
                REG_BINARY,
                (LPBYTE)Value,
                ValueLength
                );

    return error;

}   // WriteBinary


DWORD
WriteString(
    HKEY RootKey,
    LPTSTR ValueName,
    LPTSTR Value
    )

/*++

Routine Description:

    Writes a string value to the registry.

Arguments:

    RootKey - The root key containing the value to write.

    ValueName - The name of the value to write.

    Value - Points to the string to write.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;

    //
    // Sanity check.
    //

    DBG_ASSERT( RootKey != NULL );
    DBG_ASSERT( ValueName != NULL );
    DBG_ASSERT( Value != NULL );

    //
    // Write it.
    //

    error = RegSetValueEx(
                RootKey,
                ValueName,
                0,
                REG_SZ,
                (LPBYTE)Value,
                (DWORD)_tcslen( Value ) + sizeof(TCHAR)
                );

    return error;

}   // WriteString


DWORD
WriteGuid(
    HKEY RootKey,
    LPTSTR ValueName,
    LPGUID Value
    )

/*++

Routine Description:

    Writes a GUID value to the registry.

Arguments:

    RootKey - The root key containing the value to write.

    ValueName - The name of the value to write.

    Value - The GUID to write.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;

    //
    // Sanity check.
    //

    DBG_ASSERT( RootKey != NULL );
    DBG_ASSERT( ValueName != NULL );

    //
    // Write it.
    //

    error = RegSetValueEx(
                RootKey,
                ValueName,
                0,
                REG_BINARY,
                (LPBYTE)Value,
                sizeof(*Value)
                );

    return error;

}   // WriteGuid


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
    // Sanity check.
    //

    DBG_ASSERT( ServicesKey != NULL );

    //
    // Open the key.
    //

    IF_DEBUG_SETUP {

        DBG_PRINT((
            "OpenServicesRoot(): opening %lx\\%s\n",
            HKEY_LOCAL_MACHINE,
            SERVICES_KEY
            ));

    }

    error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                SERVICES_KEY,
                0,
                KEY_ALL_ACCESS,
                ServicesKey
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    IF_DEBUG_SETUP {

        DBG_PRINT((
            "OpenServicesRoot(): handle %lx\n",
            *ServicesKey
            ));

    }

    //
    // Success!
    //

exit:

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
    // Sanity check.
    //

    DBG_ASSERT( ServicesKey != NULL );
    DBG_ASSERT( WinsockKey != NULL );

    //
    // Open the WinSock key.
    //

    IF_DEBUG_SETUP {

        DBG_PRINT((
            "OpenWinsockRoot(): opening %lx\\%s\n",
            ServicesKey,
            WINSOCK_SUBKEY
            ));

    }

    error = RegOpenKeyEx(
                ServicesKey,
                WINSOCK_SUBKEY,
                0,
                KEY_ALL_ACCESS,
                WinsockKey
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    IF_DEBUG_SETUP {

        DBG_PRINT((
            "OpenWinsockRoot(): handle %lx\n",
            *WinsockKey
            ));

    }

    //
    // Success!
    //

exit:

    return error;

}   // OpenWinsockRoot


DWORD
OpenSetupMigrationRoot(
    HKEY WinsockKey,
    PHKEY MigrationKey,
    PHKEY ProvidersKey,
    PHKEY WellKnownGuidsKey
    )

/*++

Routine Description:

    Opens the ...\WinSock\Setup Migration, ...\Setup Migration\Providers,
    and ...\Setup Migration\Well Known Guids registry keys. If the Setup
    Migration key is not present, it is created and initialized with default
    values. If it contains an invalid setup version number, the entire Setup
    Migration tree is deleted and recreated from scratch with default values.

Arguments:

    WinsockKey - The handle to the ...\WinSock key.

    MigrationKey - Will receive a handle to the Setup Migration key if
        successful.

    ProvidersKey - Will receive a handle to the Setup Migration\Providers
        key if successful.

    WellKnownGuidsKey - Will receive a handle to the Setup Migration\Well
        Known Guids key if successful.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    HKEY migrationKey;
    HKEY providersKey;
    HKEY wellKnownGuidsKey;
    DWORD setupVersion;

    //
    // Sanity check.
    //

    DBG_ASSERT( WinsockKey != NULL );
    DBG_ASSERT( MigrationKey != NULL );
    DBG_ASSERT( ProvidersKey != NULL );
    DBG_ASSERT( WellKnownGuidsKey != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    migrationKey = NULL;
    providersKey = NULL;
    wellKnownGuidsKey = NULL;

    //
    // Open the migration key.
    //

    IF_DEBUG_SETUP {

        DBG_PRINT((
            "OpenSetupMigrationRoot(): opening %lx\\%s\n",
            WinsockKey,
            MIGRATION_SUBKEY
            ));

    }

    error = RegOpenKeyEx(
                WinsockKey,
                MIGRATION_SUBKEY,
                0,
                KEY_ALL_ACCESS,
                &migrationKey
                );

    if( error != NO_ERROR && error != ERROR_FILE_NOT_FOUND ) {

        goto exit;

    }

    IF_DEBUG_SETUP {

        if( error == NO_ERROR ) {

            DBG_PRINT((
                "OpenSetupMigrationRoot(): handle %lx\n",
                migrationKey
                ));

        } else {

            DBG_PRINT((
                "OpenSetupMigrationRoot(): not found\n"
                ));

        }

    }

    //
    // Open the providers key.
    //

    if( error == NO_ERROR ) {

        IF_DEBUG_SETUP {

            DBG_PRINT((
                "OpenSetupMigrationRoot(): opening %lx\\%s\n",
                migrationKey,
                PROVIDERS_SUBKEY
                ));

        }

        error = RegOpenKeyEx(
                    migrationKey,
                    PROVIDERS_SUBKEY,
                    0,
                    KEY_ALL_ACCESS,
                    &providersKey
                    );

        if( error != NO_ERROR && error != ERROR_FILE_NOT_FOUND ) {

            goto exit;

        }

        IF_DEBUG_SETUP {

            if( error == NO_ERROR ) {

                DBG_PRINT((
                    "OpenSetupMigrationRoot(): handle %lx\n",
                    providersKey
                    ));

            } else {

                DBG_PRINT((
                    "OpenSetupMigrationRoot(): not found\n"
                    ));

            }

        }

    }

    //
    // Open the well known guids key.
    //

    if( error == NO_ERROR ) {

        IF_DEBUG_SETUP {

            DBG_PRINT((
                "OpenSetupMigrationRoot(): opening %lx\\%s\n",
                migrationKey,
                WELL_KNOWN_GUIDS_SUBKEY
                ));

        }

        error = RegOpenKeyEx(
                    migrationKey,
                    WELL_KNOWN_GUIDS_SUBKEY,
                    0,
                    KEY_ALL_ACCESS,
                    &wellKnownGuidsKey
                    );

        if( error != NO_ERROR && error != ERROR_FILE_NOT_FOUND ) {

            goto exit;

        }

        IF_DEBUG_SETUP {

            if( error == NO_ERROR ) {

                DBG_PRINT((
                    "OpenSetupMigrationRoot(): handle %lx\n",
                    wellKnownGuidsKey
                    ));

            } else {

                DBG_PRINT((
                    "OpenSetupMigrationRoot(): not found\n"
                    ));

            }

        }

    }

    //
    // If we managed to open the migration key, then try to read the
    // version number. If we can read it, and it matches our version
    // number, then all is well. Otherwise (either we can't read it or
    // it doesn't match) then close the migration key, blow away the
    // current migration registry tree, and start from scratch.
    //

    if( error == NO_ERROR ) {

        error = ReadDword(
                    migrationKey,
                    SETUP_VERSION_VALUE,
                    &setupVersion
                    );

        if( error == NO_ERROR ) {

            IF_DEBUG_SETUP {

                DBG_PRINT((
                    "OpenSetupMigrationRoot(): setup version %lu\n",
                    setupVersion
                    ));

            }

            if( setupVersion == WINSOCK_SETUP_VERSION ) {

                //
                // Good news.
                //

                *MigrationKey = migrationKey;
                *ProvidersKey = providersKey;
                *WellKnownGuidsKey = wellKnownGuidsKey;
                goto exit;

            }

        }

    }

    //
    // We'll only make it here if either a) the migration key could not
    // be opened, b) the migration key was opened, but the providers key
    // could not be opened, c) the migration key was opened, but the well
    // known guids key could not be opened, d) the migration key was opened,
    // but the setup version number could not be read, or e) the version
    // number was read but did not match our version number. We'll close the
    // registry keys if we managed to open then, blow away the existing
    // migration registry tree, and create a new one from scratch.
    //

    IF_DEBUG_SETUP {

        DBG_PRINT((
            "OpenSetupMigrationRoot(): starting over from scratch\n"
            ));

    }

    if( migrationKey != NULL ) {

        (VOID)RegCloseKey( migrationKey );
        migrationKey = NULL;

    }

    if( providersKey != NULL ) {

        //
        // Remove all installed providers.
        //

        error = RemoveAllInstalledProviders(
                    providersKey
                    );

        if( error != NO_ERROR ) {

            goto exit;

        }

        (VOID)RegCloseKey( providersKey );
        providersKey = NULL;

    }

    if( wellKnownGuidsKey != NULL ) {

        (VOID)RegCloseKey( wellKnownGuidsKey );
        wellKnownGuidsKey = NULL;

    }

    error = RecursivelyDeleteRegistryTree(
                WinsockKey,
                MIGRATION_SUBKEY
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    error = CreateDefaultSetupMigrationTree(
                WinsockKey,
                &migrationKey,
                &providersKey,
                &wellKnownGuidsKey
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Success!
    //

    *MigrationKey = migrationKey;
    *ProvidersKey = providersKey;
    *WellKnownGuidsKey = wellKnownGuidsKey;

exit:

    if( error != NO_ERROR ) {

        if( migrationKey != NULL ) {

            (VOID)RegCloseKey( migrationKey );

        }

        if( providersKey != NULL ) {

            (VOID)RegCloseKey( providersKey );

        }

        if( wellKnownGuidsKey != NULL ) {

            (VOID)RegCloseKey( wellKnownGuidsKey );

        }

        *MigrationKey = NULL;
        *ProvidersKey = NULL;
        *WellKnownGuidsKey = NULL;

    }

    return error;

}   // OpenSetupMigrationRoot


DWORD
ReadNewProviderList(
    HKEY WinsockKey,
    LPTSTR FAR * NewProviderList
    )

/*++

Routine Description:

    Reads the list of current WinSock 1.1 providers (helper DLLs).

Arguments:

    WinsockKey - A handle to the ...\Services\WinSock registry key.

    NewProviderList - Will receive a pointer to the MULTI_SZ for the
        provider list. Note that it is the caller's responsibility to
        free this memory.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    HKEY parametersKey;

    //
    // Sanity check.
    //

    DBG_ASSERT( WinsockKey != NULL );
    DBG_ASSERT( NewProviderList != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    parametersKey = NULL;

    //
    // Open the Parameters key.
    //

    IF_DEBUG_SETUP {

        DBG_PRINT((
            "ReadNewProviderList(): opening %lx\\%s\n",
            WinsockKey,
            PARAMETERS_SUBKEY
            ));

    }

    error = RegOpenKeyEx(
                WinsockKey,
                PARAMETERS_SUBKEY,
                0,
                KEY_ALL_ACCESS,
                &parametersKey
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    IF_DEBUG_SETUP {

        DBG_PRINT((
            "ReadNewProviderList(): handle %lx\n",
            parametersKey
            ));

    }

    //
    // Read the new provider list.
    //

    IF_DEBUG_SETUP {

        DBG_PRINT((
            "ReadNewProviderList(): reading %lx\\%s\n",
            parametersKey,
            TRANSPORTS_VALUE
            ));

    }

    error = ReadMultiSz(
                parametersKey,
                TRANSPORTS_VALUE,
                NewProviderList
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Success!
    //

exit:

    if( parametersKey != NULL ) {

        (VOID)RegCloseKey( parametersKey );

    }

    return error;

}   // ReadNewProviderList


DWORD
ReadOldProviderList(
    HKEY MigrationKey,
    LPTSTR FAR * OldProviderList
    )

/*++

Routine Description:

    Reads the list of currently migrated providers.

Arguments:

    MigrationKey - A handle to the ...\WinSock\Setup Migration registry
        key.

    OldProviderList - Will receive a pointer to the MULTI_SZ for the
        currently migrated providers. Note that it is the caller's
        responsibility to free this memory.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;

    //
    // Sanity check.
    //

    DBG_ASSERT( MigrationKey != NULL );
    DBG_ASSERT( OldProviderList != NULL );

    //
    // Read the old provider list.
    //

    error = ReadMultiSz(
                MigrationKey,
                PROVIDER_LIST_VALUE,
                OldProviderList
                );

    return error;

}   // ReadOldProviderList


DWORD
ReadKnownStaticProviderList(
    HKEY MigrationKey,
    LPTSTR FAR * KnownStaticProviderList
    )

/*++

Routine Description:

    Reads the list of known static provider names.

Arguments:

    MigrationKey - A handle to the ...\WinSock\Setup Migration registry
        key.

    KnownStaticProviderList - Will receive a pointer to the MULTI_SZ for the
        known static providers. Note that it is the caller's responsibility
        to free this memory.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;

    //
    // Sanity check.
    //

    DBG_ASSERT( MigrationKey != NULL );
    DBG_ASSERT( KnownStaticProviderList != NULL );

    //
    // Read the known static provider list.
    //

    error = ReadMultiSz(
                MigrationKey,
                KNOWN_STATIC_VALUE,
                KnownStaticProviderList
                );

    return error;

}   // ReadKnownStaticProviderList


DWORD
ReadProviderId(
    HKEY ProvidersKey,
    LPTSTR ProviderName,
    LPGUID ProviderId
    )

/*++

Routine Description:

    Reads a migrated provider's WinSock 2.0 provider ID from the registry.

Arguments:

    ProvidersKey - A handle to the ...\Setup Migration\Providers registry
        key.

    ProviderName - The name of the provider whose ID is to be queried.

    ProviderId - Will receive the provider's ID value.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    HKEY providerKey;

    //
    // Sanity check.
    //

    DBG_ASSERT( ProvidersKey != NULL );
    DBG_ASSERT( ProviderName != NULL );
    DBG_ASSERT( ProviderId != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    providerKey = NULL;

    //
    // Open the provider
    //

    error = RegOpenKeyEx(
                ProvidersKey,
                ProviderName,
                0,
                KEY_ALL_ACCESS,
                &providerKey
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Read the provider ID.
    //

    error = ReadGuid(
                providerKey,
                WINSOCK_2_0_ID_VALUE,
                ProviderId
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Success!
    //

exit:

    if( providerKey != NULL ) {

        (VOID)RegCloseKey( providerKey );

    }

    return error;

}   // ReadProviderId


DWORD
CreateDefaultSetupMigrationTree(
    HKEY WinsockKey,
    PHKEY MigrationKey,
    PHKEY ProvidersKey,
    PHKEY WellKnownGuidsKey
    )

/*++

Routine Description:

    Creates the default ...\WinSock\Setup Migration registry tree, using
    default values.

Arguments:

    WinsockKey - A handle to the ...\Services\WinSock registry key.

    MigratonKey - Will receive a handle to the ...\WinSock\Setup Migration
        registry key if successful.

    ProvidersKey - Will receive a handle to the ...\Setup Migration\Providers
        registry key if successful.

    WellKnownGuidsKey - Will receive a handle to the Setup Migration\Well
        Known Guids key if successful.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    DWORD disposition;
    DWORD i;
    HKEY migrationKey;
    HKEY providersKey;
    HKEY wellKnownGuidsKey;

    //
    // Sanity check.
    //

    DBG_ASSERT( WinsockKey != NULL );
    DBG_ASSERT( MigrationKey != NULL );
    DBG_ASSERT( ProvidersKey != NULL );
    DBG_ASSERT( WellKnownGuidsKey != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    migrationKey = NULL;
    providersKey = NULL;
    wellKnownGuidsKey = NULL;

    //
    // Create the Setup Migration key.
    //

    error = RegCreateKeyEx(
                WinsockKey,
                MIGRATION_SUBKEY,
                0,
                TEXT(""),
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &migrationKey,
                &disposition
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    DBG_ASSERT( disposition == REG_CREATED_NEW_KEY );

    //
    // Create the default values.
    //

    error = WriteDword(
                migrationKey,
                SETUP_VERSION_VALUE,
                DefaultSetupVersion
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    error = WriteMultiSz(
                migrationKey,
                PROVIDER_LIST_VALUE,
                DefaultProviderList
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    error = WriteMultiSz(
                migrationKey,
                KNOWN_STATIC_VALUE,
                DefaultKnownStaticProviders
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Create the Providers key.
    //

    error = RegCreateKeyEx(
                migrationKey,
                PROVIDERS_SUBKEY,
                0,
                TEXT(""),
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &providersKey,
                &disposition
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    DBG_ASSERT( disposition == REG_CREATED_NEW_KEY );

    //
    // Create the Well Known Guids key.
    //

    error = RegCreateKeyEx(
                migrationKey,
                WELL_KNOWN_GUIDS_SUBKEY,
                0,
                TEXT(""),
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &wellKnownGuidsKey,
                &disposition
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    DBG_ASSERT( disposition == REG_CREATED_NEW_KEY );

    //
    // Create the default values.
    //

    for( i = 0 ; i < NUM_DEFAULT_WELL_KNOWN_GUIDS ; i++ ) {

        error = WriteGuid(
                    wellKnownGuidsKey,
                    DefaultWellKnownGuids[i].Name,
                    &DefaultWellKnownGuids[i].Guid
                    );

        if( error != NO_ERROR ) {

            goto exit;

        }

    }

    //
    // Success!
    //

    *MigrationKey = migrationKey;
    *ProvidersKey = providersKey;
    *WellKnownGuidsKey = wellKnownGuidsKey;

exit:

    if( error != NO_ERROR ) {

        if( migrationKey != NULL ) {

            (VOID)RegCloseKey( migrationKey );

        }

        if( providersKey != NULL ) {

            (VOID)RegCloseKey( providersKey );

        }

        if( wellKnownGuidsKey != NULL ) {

            (VOID)RegCloseKey( wellKnownGuidsKey );

        }

        *MigrationKey = NULL;
        *ProvidersKey = NULL;
        *WellKnownGuidsKey = NULL;

    }

    return error;

}   // CreateDefaultSetupMigrationTree


DWORD
RecursivelyDeleteRegistryTree(
    HKEY RootKey,
    LPTSTR TargetKeyName
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
    TCHAR subkeyName[MAX_REGISTRY_NAME];

    //
    // Sanity check.
    //

    DBG_ASSERT( RootKey != NULL );
    DBG_ASSERT( TargetKeyName != NULL );

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


DWORD
RemoveProviderByName(
    HKEY ProvidersKey,
    LPTSTR ProviderName
    )

/*++

Routine Description:

    Removes the specified provider by deinstalling it from the WinSock 2.0
    configuration database and deleting its migration registry tree.

Arguments:

    ProvidersKey - A handle to the ...\Setup Migration\Providers registry
        key.

    ProviderName - The name of the provider to remove.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    INT result;
    GUID providerId;

    //
    // Sanity check.
    //

    DBG_ASSERT( ProvidersKey != NULL );
    DBG_ASSERT( ProviderName != NULL );

    //
    // Get the provider's WinSock 2.0 ID code.
    //

    error = ReadProviderId(
                ProvidersKey,
                ProviderName,
                &providerId
                );

    if( error == NO_ERROR ) {

        //
        // Uninstall it from WinSock 2.0.
        //

        result = (WSCDeinstallProviderProc)(
                     &providerId,
                     (LPINT)&error
                     );

        if( result == SOCKET_ERROR ) {

            if( error == WSAEFAULT ) {

                //
                // WSCDeinstallProvider returns WSAEFAULT if it could
                // not find the provider. We'll just ignore this and
                // press on regardless.
                //

                error = NO_ERROR;

            } else {

                DBG_ASSERT( error != NO_ERROR );
                goto exit;

            }

        }

    }

    //
    // Nuke its registry tree.
    //

    error = RecursivelyDeleteRegistryTree(
                ProvidersKey,
                ProviderName
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Success!
    //

exit:

    return error;

}   // RemoveProviderByName


DWORD
ReadProtocolDataFromRegistry(
    HKEY ProvidersKey,
    LPTSTR ProviderName,
    LPPROTOCOL_INFO FAR * RegistryInfo11,
    LPDWORD RegistryInfo11Length
    )

/*++

Routine Description:

    Reads the WinSock 1.1 protocol data stored in the provider's migration
    registry tree.

Arguments:

    ProvidersKey - A handle to the ...\Setup Migration\Providers registry
        key.

    ProviderName - The name of the provider to remove.

    RegistryInfo11 - Will receive a pointer to the provider's protocol
        information as read from the registry. Note that it is the caller's
        responsibility to free this memory.

    RegistryInfo11Length - Will receive the length (in BYTEs) of the
        provider's protocol information.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    HKEY providerKey;
    LPVOID registryData;
    DWORD registryDataLength;

    //
    // Sanity check.
    //

    DBG_ASSERT( ProvidersKey != NULL );
    DBG_ASSERT( ProviderName != NULL );
    DBG_ASSERT( RegistryInfo11 != NULL );
    DBG_ASSERT( RegistryInfo11Length != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    providerKey = NULL;
    registryData = NULL;

    //
    // Open the provider
    //

    error = RegOpenKeyEx(
                ProvidersKey,
                ProviderName,
                0,
                KEY_ALL_ACCESS,
                &providerKey
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Read the provider data.
    //

    error = ReadBinary(
                providerKey,
                WINSOCK_1_1_DATA_VALUE,
                &registryData,
                &registryDataLength
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Success!
    //

    *RegistryInfo11 = registryData;
    *RegistryInfo11Length = registryDataLength;

exit:

    if( providerKey != NULL ) {

        (VOID)RegCloseKey( providerKey );

    }

    if( error != NO_ERROR ) {

        if( registryData != NULL ) {

            FREE_MEM( registryData );

        }

        *RegistryInfo11 = NULL;
        *RegistryInfo11Length = 0;

    }

    return error;

}   // ReadProtocolDataFromRegistry


DWORD
ReadProtocolDataFromProvider(
    HKEY ServicesKey,
    LPTSTR ProviderName,
    LPTSTR ProviderDllPath,
    LPPROTOCOL_INFO FAR * ProtocolInfo11,
    LPDWORD ProtocolInfo11Length,
    LPDWORD ProtocolInfo11Entries
    )

/*++

Routine Description:

    Reads the WinSock 1.1 protocol data stored from the provider's helper
    DLL. This is done by loading the helper DLL and invoking it's
    WSHEnumProtocols() entrypoint.

Arguments:

    ServicesKey - A handle to the ...\CurrentControlSet\Services registry
        key.

    ProviderName - The name of the provider to remove.

    ProviderDllPath - Will receive the fully expanded path to the provider's
        helper DLL. This is assumed to point to an array of TCHARs at least
        MAX_PATH in length.

    ProtocolInfo11 - Will receive a pointer to the provider's protocol
        information as returned by WSHEnumProtocols(). Note that it is the
        caller's responsibility to free this memory.

    ProtocolInfo11Length - Will receive the length (in BYTEs) of the
        provider's protocol information.

    ProtocolInfo11Entries - Will receive the number of entries in the
        provider's protocol information.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

Notes:

    In general, this routine is overly tolerant of errors regarding
    registry layout. This keeps the migration process running, even if
    there is garbage in the registry.

--*/

{

    DWORD error;
    HKEY serviceKey;
    HKEY parametersKey;
    LPTSTR helperDllPath;
    DWORD expandedLength;
    HMODULE helperDllHandle;
    PWSH_ENUM_PROTOCOLS enumProtocols;
    INT numEntries;
    LPPROTOCOL_INFO protocolInfo11;
    DWORD protocolInfo11Length;
    LPDWORD protocolList;
    INT i;
    LPWSTR unicodeProviderName;
#if !defined(UNICODE)
    WCHAR unicodeProviderNameBuffer[MAX_PATH];
#endif

    //
    // Sanity check.
    //

    DBG_ASSERT( ServicesKey != NULL );
    DBG_ASSERT( ProviderName != NULL );
    DBG_ASSERT( ProviderDllPath != NULL );
    DBG_ASSERT( ProtocolInfo11 != NULL );
    DBG_ASSERT( ProtocolInfo11Length != NULL );
    DBG_ASSERT( ProtocolInfo11Entries != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    serviceKey = NULL;
    parametersKey = NULL;
    protocolList = NULL;
    helperDllPath = NULL;
    helperDllHandle = NULL;
    numEntries = 0;

    *ProtocolInfo11 = NULL;
    *ProtocolInfo11Length = 0;
    *ProtocolInfo11Entries = 0;

#if defined(UNICODE)
    unicodeProviderName = ProviderName;
#else
    //
    // Map the provider name to UNICODE so we can pass it down.
    //

    wsprintfW(
        unicodeProviderNameBuffer,
        L"%hs",
        ProviderName
        );

    unicodeProviderName = unicodeProviderNameBuffer;
#endif

    //
    // Open the service key.
    //

    error = RegOpenKeyEx(
                ServicesKey,
                ProviderName,
                0,
                KEY_ALL_ACCESS,
                &serviceKey
                );

    if( error != NO_ERROR ) {

        error = NO_ERROR;   // Press on regardless.
        goto exit;

    }

    //
    // Open the parameters key.
    //

    error = RegOpenKeyEx(
                serviceKey,
                SERVICE_PARAMS_SUBKEY,
                0,
                KEY_ALL_ACCESS,
                &parametersKey
                );

    if( error != NO_ERROR ) {

        error = NO_ERROR;   // Press on regardless.
        goto exit;

    }

    //
    // Read the supported protocols.
    //
    // HACK: Skip for NetBIOS, as its helper DLL will fail if any
    // protocols are passed in.
    //

    if( _tcsicmp( ProviderName, TEXT("NetBIOS") ) != 0 ) {

        error = ReadProviderSupportedProtocolsFromRegistry(
                    parametersKey,
                    &protocolList
                    );

        if( error != NO_ERROR ) {

            error = NO_ERROR;   // Press on regardless.
            goto exit;

        }

    }

    //
    // Read the Helper DLL path.
    //

    error = ReadString(
                parametersKey,
                HELPER_DLL_NAME_VALUE,
                &helperDllPath
                );

    if( error != NO_ERROR ) {

        error = NO_ERROR;   // Press on regardless.
        goto exit;

    }

    //
    // Expand any embedded environment strings.
    //

    expandedLength = ExpandEnvironmentStrings(
                         helperDllPath,
                         ProviderDllPath,
                         MAX_PATH
                         );

    if( expandedLength > MAX_PATH ) {

        error = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;

    }

    if( expandedLength == 0 ) {

        error = GetLastError();
        goto exit;

    }

    //
    // Load the DLL and find the entrypoint.
    //

    helperDllHandle = LoadLibrary( ProviderDllPath );

    if( helperDllHandle == NULL ) {

        DBG_ASSERT( error == NO_ERROR );
        goto exit;

    }

    enumProtocols = (PVOID)GetProcAddress(
                               helperDllHandle,
                               "WSHEnumProtocols"
                               );

    if( enumProtocols == NULL ) {

        DBG_ASSERT( error == NO_ERROR );
        goto exit;

    }

    //
    // Determine the required buffer size.
    //

    protocolInfo11 = NULL;
    protocolInfo11Length = 0;

    try {

        numEntries = enumProtocols(
                         protocolList,
                         (LPTSTR)unicodeProviderName,
                         protocolInfo11,
                         &protocolInfo11Length
                         );

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        numEntries = 0;

        DBG_PRINT((
            "%s!WSHEnumProtocols raised exception %08lX, skipping\n",
            helperDllPath,
            GetExceptionCode()
            ));

    }

    if( numEntries == 0 ) {

        DBG_ASSERT( error == NO_ERROR );
        goto exit;

    }

    DBG_ASSERT( numEntries == -1 );
    DBG_ASSERT( protocolInfo11Length > 0 );

    //
    // Allocate a buffer.
    //

    protocolInfo11 = ALLOC_MEM( protocolInfo11Length );

    if( protocolInfo11 == NULL ) {

        error = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;

    }

    RtlZeroMemory(
        protocolInfo11,
        protocolInfo11Length
        );

    //
    // And now really read the data.
    //

    try {

        numEntries = enumProtocols(
                         protocolList,
                         (LPTSTR)unicodeProviderName,
                         protocolInfo11,
                         &protocolInfo11Length
                         );

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        numEntries = 0;

        DBG_PRINT((
            "%s!WSHEnumProtocols raised exception %08lX, skipping\n",
            helperDllPath,
            GetExceptionCode()
            ));

    }

    if( numEntries == -1 ) {

        error = ERROR_GEN_FAILURE;
        goto exit;

    }

    if( numEntries == 0 ) {

        DBG_ASSERT( error == NO_ERROR );
        DBG_ASSERT( protocolInfo11 != NULL );

        FREE_MEM( protocolInfo11 );
        protocolInfo11 = NULL;

        goto exit;

    }

#if !defined(UNICODE)
    //
    // Map the UNICODE strings to ANSI.
    //

    for( i = 0 ; i < numEntries ; i++ ) {

        wsprintfA(
            protocolInfo11[i].lpProtocol,
            "%ls",
            protocolInfo11[i].lpProtocol
            );

    }
#endif

    //
    // Success!
    //

    *ProtocolInfo11 = protocolInfo11;
    *ProtocolInfo11Length = protocolInfo11Length;
    *ProtocolInfo11Entries = (DWORD)numEntries;

exit:

    if( serviceKey != NULL ) {

        (VOID)RegCloseKey( serviceKey );

    }

    if( parametersKey != NULL ) {

        (VOID)RegCloseKey( parametersKey );

    }

    if( protocolList != NULL ) {

        FREE_MEM( protocolList );

    }

    if( helperDllPath != NULL ) {

        FREE_MEM( helperDllPath );

    }

    if( helperDllHandle != NULL ) {

        FreeLibrary( helperDllHandle );

    }

    return error;

}   // ReadProtocolDataFromProvider


DWORD
ReadProviderSupportedProtocolsFromRegistry(
    HKEY ParametersKey,
    LPDWORD FAR * ProtocolList
    )

/*++

Routine Description:

    Reads a list of supported protocol values from the WinSock 1.1 mapping
    data stored in the registry.

Arguments:

    ParametersKey - A handle to the ...\{provider}\Parameters\WinSock
        registry key.

    ProtocolList - Will receive a pointer to an array of DWORD values,
        one for each supported protocol. Note that this list will be
        NULL if there are no non-zero protocols in the provider's
        mapping data. This is OK, as some providers (such as NetBIOS)
        are stored this way.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    PWINSOCK_MAPPING mapping;
    DWORD mappingLength;
    DWORD mappingRows;
    LPDWORD protocolList;
    DWORD protocolCount;
    DWORD currentProtocol;
    DWORD i, j;

    //
    // Sanity check.
    //

    DBG_ASSERT( ParametersKey != NULL );
    DBG_ASSERT( ProtocolList != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    mapping = NULL;

    //
    // Read the mapping data.
    //

    error = ReadBinary(
                ParametersKey,
                MAPPING_VALUE,
                &mapping,
                &mappingLength
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    mappingRows = mapping->Rows;

    if( mappingLength < sizeof(*mapping) ||
        mappingRows == 0 ||
        mapping->Columns != 3 ) {

        error = ERROR_GEN_FAILURE;
        goto exit;

    }

    //
    // Build the list of unique supported protocols. We'll do this in
    // place on top of the raw mapping data. This is a little wasteful
    // in terms of space, but the memory is only allocated temporarily,
    // and it avoids an additional allocation.
    //

    protocolList = (LPDWORD)mapping;
    protocolCount = 0;

    for( i = 0 ; i < mappingRows ; i++ ) {

        currentProtocol = mapping->Mapping[i].Protocol;

        if( currentProtocol == 0 ) {

            continue;

        }

        for( j = 0 ; j < protocolCount ; j++ ) {

            if( protocolList[j] == currentProtocol ) {

                break;
            }

        }

        if( j >= protocolCount ) {

            protocolList[protocolCount++] = currentProtocol;

        }

    }

    protocolList[protocolCount] = 0;

    //
    // If there were no non-zero providers in the list, then just
    // return NULL. This is OK.
    //

    if( protocolCount == 0 ) {

        FREE_MEM( protocolList );
        protocolList = NULL;

    }

    //
    // Success!
    //

    *ProtocolList = protocolList;

exit:

    if( error != NO_ERROR && mapping != NULL ) {

        FREE_MEM( mapping );

    }

    return error;

}   // ReadProviderSupportedProtocolsFromRegistry

VOID
MapProtocolInfoToSelfRelative(
    LPPROTOCOL_INFO ProtocolInfo11,
    DWORD ProtocolInfo11Entries
    )

/*++

Routine Description:

    Maps the embedded pointers in the WinSock 1.1 protocol information
    from absolute form to self-relative form.

Arguments:

    ProtocolInfo11 - A pointer to the WinSock 1.1 protocol information.

    ProtocolInfo11Entries - The number of entries in the protocol
        information.

Return Value:

    None.

--*/

{

    LPPROTOCOL_INFO protocolInfo11Start;

    //
    // Sanity check.
    //

    DBG_ASSERT( ProtocolInfo11 != NULL );
    DBG_ASSERT( ProtocolInfo11Entries > 0 );

    //
    // Do it.
    //

    protocolInfo11Start = ProtocolInfo11;

    while( ProtocolInfo11Entries-- > 0 ) {

        DBG_ASSERT( (ULONG_PTR)ProtocolInfo11->lpProtocol > (ULONG_PTR)protocolInfo11Start );

        ProtocolInfo11->lpProtocol =
            (LPVOID)( (ULONG_PTR)ProtocolInfo11->lpProtocol -
                (ULONG_PTR)protocolInfo11Start );

        ProtocolInfo11++;

    }

}   // MapProtocolInfoToSelfRelative

VOID
MapProtocolInfoToAbsolute(
    LPPROTOCOL_INFO ProtocolInfo11,
    DWORD ProtocolInfo11Entries
    )

/*++

Routine Description:

    Maps the embedded pointers in the WinSock 1.1 protocol information
    from self-relative form to absolute form.

Arguments:

    ProtocolInfo11 - A pointer to the WinSock 1.1 protocol information.

    ProtocolInfo11Entries - The number of entries in the protocol
        information.

Return Value:

    None.

--*/

{

    LPPROTOCOL_INFO protocolInfo11Start;

    //
    // Sanity check.
    //

    DBG_ASSERT( ProtocolInfo11 != NULL );
    DBG_ASSERT( ProtocolInfo11Entries > 0 );

    //
    // Do it.
    //

    protocolInfo11Start = ProtocolInfo11;

    while( ProtocolInfo11Entries-- > 0 ) {

        ProtocolInfo11->lpProtocol =
            (LPVOID)( (ULONG_PTR)ProtocolInfo11->lpProtocol +
                (ULONG_PTR)protocolInfo11Start );

        ProtocolInfo11++;

    }

}   // MapProtocolInfoToAbsolute

DWORD
BuildWinsock2ProtocolList(
    LPTSTR ProviderName,
    LPTSTR ProviderDllPath,
    LPPROTOCOL_INFO ProtocolInfo11,
    DWORD ProtocolInfo11Entries,
    LPWSAPROTOCOL_INFO FAR * ProtocolInfo2,
    LPDWORD ProtocolInfo2Entries
    )

/*++

Routine Description:

    Scans WinSock 1.1 protocol information and creates corresponding
    WinSock 2.0 protocol information.

Arguments:

    ProviderName - The name of the current provider.

    ProviderDllPath - The fully expanded path to the provider's helper DLL.

    ProtocolInfo11 - A pointer to the WinSock 1.1 protocol information.

    ProtocolInfo11Entries - The number of entries in the protocol
        information.

    ProtocolInfo2 - Will receive a pointer to the WinSock 2.0 protocol
        information if successful. Note that it is the caller's
        responsibility to free this memory.

    ProtocolInfo2Entries - Will receive the number of entries in the
        WinSock 2.0 protocol information. Note that this may be greater
        than ProtocolInfo11Entries.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    LPWSAPROTOCOL_INFO protocolInfo2;
    LPWSAPROTOCOL_INFO tmpProtocolInfo2;
    DWORD protocolInfo2Entries;
    DWORD protocolInfo2Length;
    DWORD i, j;
    INT addressFamily;
    INT socketType;
    INT protocol;
    HMODULE helperDllHandle;
    PWSH_GET_WSAPROTOCOL_INFO getWSAProtocolInfo;

    //
    // Sanity check.
    //

    DBG_ASSERT( ProviderName != NULL );
    DBG_ASSERT( ProviderDllPath != NULL );
    DBG_ASSERT( ProtocolInfo11 != NULL );
    DBG_ASSERT( ProtocolInfo11Entries > 0 );
    DBG_ASSERT( ProtocolInfo2 != NULL );
    DBG_ASSERT( ProtocolInfo2Entries != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    error = NO_ERROR;
    protocolInfo2 = NULL;
    helperDllHandle = NULL;

    //
    // Determine if this helper DLL supports the new WSHGetWSAProtocolInfo
    // entrypoint. If so, we'll just use it to build the WinSock 2.0
    // protocol info. If not, we'll build it ourselves using the WinSock 1.1
    // data.
    //

    helperDllHandle = LoadLibrary( ProviderDllPath );

    if( helperDllHandle != NULL ) {

        getWSAProtocolInfo = (PVOID)GetProcAddress(
                                        helperDllHandle,
                                        "WSHGetWSAProtocolInfo"
                                        );

        if( getWSAProtocolInfo != NULL ) {

            IF_DEBUG_SETUP {

                DBG_PRINT((
                    "Calling %ls!WSHGetWSAProtocolInfo\n",
                    ProviderDllPath
                    ));

            }

            //
            // The new entrypoint is exported by the helper DLL. Call
            // it to get the WSAPROTOCOL_INFO supported by this provider.
            //

            tmpProtocolInfo2 = NULL;
            protocolInfo2Entries = 0;

            try {

                error = (DWORD)getWSAProtocolInfo(
                                   ProviderName,
                                   &tmpProtocolInfo2,
                                   &protocolInfo2Entries
                                   );

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                error = GetExceptionCode();

            }

            if( error == NO_ERROR &&
                tmpProtocolInfo2 != NULL &&
                protocolInfo2Entries > 0 ) {

                //
                // We got the data. Allocate a new buffer for the data
                // and make a copy of it. If the allocation fails, we're
                // screwed.
                //

                protocolInfo2Length =
                    protocolInfo2Entries * sizeof(WSAPROTOCOL_INFO);

                protocolInfo2 = ALLOC_MEM( protocolInfo2Length );

                if( protocolInfo2 == NULL ) {

                    error = ERROR_NOT_ENOUGH_MEMORY;
                    goto exit;

                }

                //
                // Protect ourselves just in case the helper DLL returns
                // really stupid data.
                //

                try {

                    RtlCopyMemory(
                        protocolInfo2,
                        tmpProtocolInfo2,
                        protocolInfo2Length
                        );

                    error = NO_ERROR;

                } except( EXCEPTION_EXECUTE_HANDLER ) {

                    error = GetExceptionCode();

                }

                if( error == NO_ERROR ) {

                    IF_DEBUG_SETUP {

                        DBG_PRINT((
                            "BuildWinsock2ProtocolList returning %lu entries from %ls\n",
                            protocolInfo2Entries,
                            ProviderDllPath
                            ));

                    }

                    *ProtocolInfo2 = protocolInfo2;
                    *ProtocolInfo2Entries = protocolInfo2Entries;

                    goto exit;

                }

            }

        }

        //
        // If we made it this far, then either a) we couldn't load the
        // helper DLL, b) the helper DLL doesn't export the new entrypoint,
        // c) the entrypoint failed to return the required info, or d) the
        // entrypoint returned bogus data causing us to throw an exception
        // when we tried to copy it. In any case, just proceed and construct
        // the new data ourselves.
        //
        // Note that the common cleanup code at the end of this routine
        // is responsible for freeing the helper DLL if was successfully
        // loaded.
        //

    }

    //
    // Determine the required size of the WinSock 2.0 protocol
    // info buffer. We know we'll need at least as many entries
    // as in the 1.1 list, maybe more (for pseudo-stream protocols).
    //

    protocolInfo2Entries = ProtocolInfo11Entries;

    //
    // Determine the number of WinSock 2.0 entries required.
    //

    for( i = 0 ; i < ProtocolInfo11Entries ; i++ ) {

        if( ProtocolInfo11[i].dwServiceFlags & XP_PSEUDO_STREAM ) {

            protocolInfo2Entries++;

        }

    }

    //
    // If this is for TCP/IP, then add another entry for RAW sockets.
    //

    if( _tcsicmp( ProviderName, TEXT("TcpIp") ) == 0 ) {

        protocolInfo2Entries++;

    }

    //
    // Create the buffer.
    //

    protocolInfo2Length = protocolInfo2Entries * sizeof(WSAPROTOCOL_INFO);

    protocolInfo2 = ALLOC_MEM( protocolInfo2Length );

    if( protocolInfo2 == NULL ) {

        error = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;

    }

    RtlZeroMemory(
        protocolInfo2,
        protocolInfo2Length
        );

    //
    // Now map the 1.1 entries to 2.0.
    //

    for( i = 0, j = 0 ; i < ProtocolInfo11Entries ; i++, j++ ) {

        DBG_ASSERT( j < protocolInfo2Entries );

        addressFamily = ProtocolInfo11[i].iAddressFamily;
        socketType = ProtocolInfo11[i].iSocketType;
        protocol = ProtocolInfo11[i].iProtocol;

        protocolInfo2[j].iVersion           = WINSOCK_SPI_VERSION;
        protocolInfo2[j].iAddressFamily     = addressFamily;
        protocolInfo2[j].iMaxSockAddr       =
            ProtocolInfo11[i].iMaxSockAddr;
        protocolInfo2[j].iMinSockAddr       =
            ProtocolInfo11[i].iMinSockAddr;
        protocolInfo2[j].iSocketType        = socketType;
        protocolInfo2[j].iProtocol          = protocol;
        protocolInfo2[j].iProtocolMaxOffset = 0;
        protocolInfo2[j].iNetworkByteOrder  = BIGENDIAN;
        protocolInfo2[j].iSecurityScheme    = SECURITY_PROTOCOL_NONE;
        protocolInfo2[j].dwMessageSize      =
            ProtocolInfo11[i].dwMessageSize;
        protocolInfo2[j].dwProviderFlags    = 0;
        protocolInfo2[j].dwServiceFlags1    =
            ( ProtocolInfo11[i].dwServiceFlags & COMMON_SERVICE_FLAGS ) |
            FORCED_SERVICE_FLAGS;
        protocolInfo2[j].dwServiceFlags2    = 0;
        protocolInfo2[j].dwServiceFlags3    = 0;
        protocolInfo2[j].dwServiceFlags4    = 0;
        protocolInfo2[j].ProtocolChain.ChainLen = BASE_PROTOCOL;

        if( addressFamily == AF_INET ) {

            protocolInfo2[j].dwProviderFlags |= PFL_MATCHES_PROTOCOL_ZERO;

        }
        else
        if( addressFamily == AF_IPX &&
            socketType == SOCK_DGRAM ) {

            protocolInfo2[j].iProtocolMaxOffset = 255;

        }
        else
        if( addressFamily == AF_APPLETALK &&
            socketType == SOCK_DGRAM ) {

            protocolInfo2[j].iProtocolMaxOffset = 255;

        }
        else
        if( addressFamily == AF_NETBIOS &&
            ( protocol == 0 ||
              protocol == 0x80000000 ) ) {

            protocolInfo2[j].iProtocol = 0x80000000;
            protocolInfo2[j].dwProviderFlags |= PFL_MATCHES_PROTOCOL_ZERO;

        }
        else
        if( addressFamily == AF_ISO ) {

            protocolInfo2[j].dwServiceFlags1 |=
                XP1_CONNECT_DATA |
                XP1_DISCONNECT_DATA;

            protocolInfo2[j].dwProviderFlags |= PFL_MATCHES_PROTOCOL_ZERO;

        }

        //
        // The dwProviderReserved field MUST be zero for WSPDuplicateSocket()
        // to function properly.
        //

        DBG_ASSERT( protocolInfo2[j].dwProviderReserved == 0 );

        if( ProtocolInfo11[i].dwServiceFlags & XP_PSEUDO_STREAM ) {

            protocolInfo2[j].dwProviderFlags |=
                PFL_MULTIPLE_PROTO_ENTRIES |
                PFL_RECOMMENDED_PROTO_ENTRY;

            BuildNewProtocolName(
                ProviderName,
                &ProtocolInfo11[i],
                protocolInfo2[j].szProtocol
                );

            j++;

            protocolInfo2[j] = protocolInfo2[j-1];
            protocolInfo2[j].dwProviderFlags &= ~PFL_RECOMMENDED_PROTO_ENTRY;
            protocolInfo2[j].iSocketType    = SOCK_STREAM;
            protocolInfo2[j].dwMessageSize  = 0;

            BuildNewProtocolName(
                ProviderName,
                &ProtocolInfo11[i],
                protocolInfo2[j].szProtocol
                );

            _tcscat( protocolInfo2[j].szProtocol, TEXT(" [Pseudo Stream]") );

        } else {

            BuildNewProtocolName(
                ProviderName,
                &ProtocolInfo11[i],
                protocolInfo2[j].szProtocol
                );

        }

    }

    //
    // Hand build the RAW IP entry if necessary.
    //

    if( _tcsicmp( ProviderName, TEXT("TcpIp") ) == 0 ) {

        protocolInfo2[j].iVersion           = WINSOCK_SPI_VERSION;
        protocolInfo2[j].iAddressFamily     = AF_INET;
        protocolInfo2[j].iMaxSockAddr       = sizeof(SOCKADDR_IN);
        protocolInfo2[j].iMinSockAddr       = sizeof(SOCKADDR_IN);
        protocolInfo2[j].iSocketType        = SOCK_RAW;
        protocolInfo2[j].iProtocol          = IPPROTO_IP;
        protocolInfo2[j].iProtocolMaxOffset = 255;
        protocolInfo2[j].iNetworkByteOrder  = BIGENDIAN;
        protocolInfo2[j].iSecurityScheme    = SECURITY_PROTOCOL_NONE;
        protocolInfo2[j].dwMessageSize      = 65467;
        protocolInfo2[j].dwProviderFlags    =
            PFL_HIDDEN |
            PFL_MATCHES_PROTOCOL_ZERO;
        protocolInfo2[j].dwServiceFlags1    =
            FORCED_SERVICE_FLAGS |
            XP1_CONNECTIONLESS |
            XP1_MESSAGE_ORIENTED |
            XP1_SUPPORT_BROADCAST |
            XP1_SUPPORT_MULTIPOINT;
        protocolInfo2[j].dwServiceFlags2    = 0;
        protocolInfo2[j].dwServiceFlags3    = 0;
        protocolInfo2[j].dwServiceFlags4    = 0;
        protocolInfo2[j].ProtocolChain.ChainLen = BASE_PROTOCOL;

        wsprintf(
            protocolInfo2[j].szProtocol,
            TEXT("MSAFD %s [%s]"),
            ProviderName,
            TEXT("RAW/IP")
            );

        //
        // The dwProviderReserved field MUST be zero for WSPDuplicateSocket()
        // to function properly.
        //

        DBG_ASSERT( protocolInfo2[j].dwProviderReserved == 0 );

    }

    //
    // Success!
    //

    *ProtocolInfo2 = protocolInfo2;
    *ProtocolInfo2Entries = protocolInfo2Entries;

exit:

    if( error != NO_ERROR && protocolInfo2 != NULL ) {

        FREE_MEM( protocolInfo2 );
        *ProtocolInfo2 = NULL;

    }

    if( helperDllHandle != NULL ) {

        FreeLibrary( helperDllHandle );

    }

    return error;

}   // BuildWinsock2ProtocolList

VOID
BuildNewProtocolName(
    LPTSTR ProviderName,
    LPPROTOCOL_INFO ProtocolInfo11,
    LPTSTR NewProtocolName
    )

/*++

Routine Description:

    Constructs a protocol name for the given protocol, as supported by the
    given provider.

Arguments:

    ProviderName - The name of the current provider.

    ProtocolInfo11 - A pointer to the WinSock 1.1 protocol information
        describing a single protocol.

    NewProtocolName - Will receive the name for the new protocol.

Return Value:

    None.

--*/

{

    WCHAR socketType[16];
    INT lanaNumber;

    //
    // Sanity check.
    //

    DBG_ASSERT( ProviderName != NULL );
    DBG_ASSERT( ProtocolInfo11 != NULL );
    DBG_ASSERT( NewProtocolName != NULL );

    //
    // Build the name.
    //

    if( _tcsnicmp( ProtocolInfo11->lpProtocol, TEXT("\\device\\"), 8 ) == 0 ) {

        //
        // The old protocol name was of the form \Device\FooBar, so
        // we'll need to construct a new name that contains the LANA
        // number and socket type so that the names remain unique.
        //

        lanaNumber = ProtocolInfo11->iProtocol;

        if( lanaNumber == 0x80000000 ) {

            lanaNumber = 0;

        } else if( lanaNumber < 0 ) {

            lanaNumber = -lanaNumber;

        }

        switch( ProtocolInfo11->iSocketType ) {

        case SOCK_STREAM :
            _tcscpy( socketType, TEXT("STREAM") );
            break;

        case SOCK_DGRAM :
            _tcscpy( socketType, TEXT("DATAGRAM") );
            break;

        case SOCK_RAW :
            _tcscpy( socketType, TEXT("RAW") );
            break;

        case SOCK_RDM :
            _tcscpy( socketType, TEXT("RDM") );
            break;

        case SOCK_SEQPACKET :
            _tcscpy( socketType, TEXT("SEQPACKET") );
            break;

        default :
            wsprintf(
                socketType,
                TEXT("%d"),
                ProtocolInfo11->iSocketType
                );
            break;

        }

        wsprintf(
            NewProtocolName,
            TEXT("MSAFD %s [%s] %s %d"),
            ProviderName,
            ProtocolInfo11->lpProtocol,
            socketType,
            lanaNumber
            );

    } else {

        wsprintf(
            NewProtocolName,
            TEXT("MSAFD %s [%s]"),
            ProviderName,
            ProtocolInfo11->lpProtocol
            );

    }

}   // BuildNewProtocolName

DWORD
InstallNewProvider(
    HKEY WellKnownGuidsKey,
    HKEY ProvidersKey,
    LPTSTR ProviderName,
    LPTSTR ProviderDllPath,
    BOOL IsKnownStaticProvider,
    LPPROTOCOL_INFO ProtocolInfo11,
    DWORD ProtocolInfo11Length,
    DWORD ProtocolInfo11Entries
    )

/*++

Routine Description:

    Takes the WinSock 1.1 protocol information, maps it to WinSock 2.0
    protocol information, installs the provider by invoking the
    WSCInstallProvider() entrypoint, and creates the provider's subtree
    of the ...\Setup Migration\Providers registry tree.

Arguments:

    WellKnownGuidsKey - A handle to the ...\Setup Migration\Well Known
        Guids registry key.

    ProvidersKey - A handle to the ...\Setup Migration\Providers registry
        tree.

    ProviderName - The name of the provider being installed.

    ProviderDllPath - The fully expanded path to the provider's helper DLL.

    IsKnownStaticProvider - This will be TRUE if we know this is a static
        provider that doesn't change its supported triples at random.
        If this is TRUE, then we don't bother writing the WinSock 1.1
        protocol information to the registry.

    ProtocolInfo11 - A pointer to the provider's WinSock 1.1 protocol
        information.

    ProtocolInfo11Length - The length (in BYTEs) of the WinSock 1.1
        protocol information.

    ProtocolInfo11Entries - The number of entries in the WinSock 1.1
        protocol information.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    DWORD error2;
    LPWSAPROTOCOL_INFO protocolInfo2;
    DWORD protocolInfo2Entries;
    INT result;
    INT dummy;
    GUID providerId;
    BOOL providerInstalled;

    //
    // Sanity check.
    //

    DBG_ASSERT( WellKnownGuidsKey != NULL );
    DBG_ASSERT( ProvidersKey != NULL );
    DBG_ASSERT( ProviderName != NULL );
    DBG_ASSERT( ProviderDllPath != NULL );
    DBG_ASSERT( ProtocolInfo11 != NULL );
    DBG_ASSERT( ProtocolInfo11Length > 0 );
    DBG_ASSERT( ProtocolInfo11Entries > 0 );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    protocolInfo2 = NULL;
    providerInstalled = FALSE;

    //
    // Map the old (WinSock 1.1 RNR) protocol info to the new & improved
    // (WinSock 2.0) protocol info.
    //

    error = BuildWinsock2ProtocolList(
                ProviderName,
                ProviderDllPath,
                ProtocolInfo11,
                ProtocolInfo11Entries,
                &protocolInfo2,
                &protocolInfo2Entries
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Determine/create/whatever a GUID for this provider.
    //

    error = DetermineGuidForProvider(
                WellKnownGuidsKey,
                ProviderName,
                ProviderDllPath,
                &providerId
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Install it.
    //

    result = (WSCInstallProviderProc)(
                 &providerId,
                 DEFAULT_PROVIDER_PATH,
                 protocolInfo2,
                 protocolInfo2Entries,
                 (LPINT)&error
                 );

    if( result == SOCKET_ERROR ) {

        //
        // Bummer, could not install the provider. This could possibly
        // be due to bogus registry information. So, we'll try to clean
        // the configuration for this provider, then reattempt the install.
        //

        error2 = SanitizeWinsock2ConfigForProvider( &providerId );

        if( error2 == NO_ERROR ) {

            DBG_PRINT((
                "error %d, retrying\n",
                error
                ));

            result = (WSCInstallProviderProc)(
                         &providerId,
                         DEFAULT_PROVIDER_PATH,
                         protocolInfo2,
                         protocolInfo2Entries,
                         (LPINT)&error
                         );

            if( result == SOCKET_ERROR ) {

                DBG_ASSERT( error != NO_ERROR );
                goto exit;

            }

        }

        //
        // If we made it this far, then the we successfully santizied
        // the configuration & installed the provider on the second
        // attempt.
        //

        error = NO_ERROR;

    }

    providerInstalled = TRUE;

    //
    // Create the registry info.
    //

    MapProtocolInfoToSelfRelative(
        ProtocolInfo11,
        ProtocolInfo11Entries
        );

    error = CreateMigrationRegistryForProvider(
                ProvidersKey,
                ProviderName,
                IsKnownStaticProvider,
                ProtocolInfo11,
                ProtocolInfo11Length,
                &providerId
                );

    MapProtocolInfoToAbsolute(
        ProtocolInfo11,
        ProtocolInfo11Entries
        );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Success!
    //

exit:

    if( protocolInfo2 != NULL ) {

        FREE_MEM( protocolInfo2 );

    }

    if( error != NO_ERROR && providerInstalled ) {

        (VOID)(WSCDeinstallProviderProc)(
                  &providerId,
                  &dummy
                  );

    }

    return error;

}   // InstallNewProvider

DWORD
CreateMigrationRegistryForProvider(
    HKEY ProvidersKey,
    LPTSTR ProviderName,
    BOOL IsKnownStaticProvider,
    LPPROTOCOL_INFO ProtocolInfo11,
    DWORD ProtocolInfo11Length,
    LPGUID ProviderId
    )

/*++

Routine Description:

    Creates the ...\WinSock\Setup Migration\Providers\{provider} registry
    tree for the specified provider.

Arguments:

    ProvidersKey - A handle to the ...\Setup Migration\Providers registry
        key.

    ProviderName - The name of the provider being installed.

    IsKnownStaticProvider - This will be TRUE if we know this is a static
        provider that doesn't change its supported triples at random.
        If this is TRUE, then we don't bother writing the WinSock 1.1
        protocol information to the registry.

    ProtocolInfo11 - A pointer to the provider's WinSock 1.1 protocol
        information.

    ProtocolInfo11Length - The length (in BYTEs) of the WinSock 1.1
        protocol information.

    ProviderId - The provider's GUID.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    HKEY providerKey;
    DWORD disposition;

    //
    // Sanity check.
    //

    DBG_ASSERT( ProvidersKey != NULL );
    DBG_ASSERT( ProviderName != NULL );
    DBG_ASSERT( ProtocolInfo11 != NULL );
    DBG_ASSERT( ProtocolInfo11Length > 0 );
    DBG_ASSERT( ProviderId != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //

    providerKey = NULL;

    //
    // Create the provider's key.
    //

    error = RegCreateKeyEx(
                ProvidersKey,
                ProviderName,
                0,
                TEXT(""),
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS,
                NULL,
                &providerKey,
                &disposition
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Create the values.
    //

    if( !IsKnownStaticProvider ) {

        error = WriteBinary(
                    providerKey,
                    WINSOCK_1_1_DATA_VALUE,
                    ProtocolInfo11,
                    ProtocolInfo11Length
                    );

        if( error != NO_ERROR ) {

            goto exit;

        }

    }

    error = WriteGuid(
                providerKey,
                WINSOCK_2_0_ID_VALUE,
                ProviderId
                );

    if( error != NO_ERROR ) {

        goto exit;

    }

    //
    // Success!
    //

exit:

    if( providerKey != NULL ) {

        (VOID)RegCloseKey( providerKey );

    }

    if( error != NO_ERROR ) {

        (VOID)RecursivelyDeleteRegistryTree(
                  ProvidersKey,
                  ProviderName
                  );

    }

    return error;

}   // CreateMigrationRegistryForProvider

#if 0
DWORD
CreateProtocolCatalogMutex(
    LPHANDLE Handle
    )

/*++

Routine Description:

    Creates the named, shared mutex that protects the protocol catalog
    in the system registry.

Arguments:

    Handle - Will receive a handle to the protocol catalog mutex if
        successful.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    PSECURITY_DESCRIPTOR securityDescriptor;
    SECURITY_ATTRIBUTES securityAttributes;
    BOOL result;
    BYTE securityDescriptorBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH];

    //
    // Sanity check.
    //

    DBG_ASSERT( Handle != NULL );

    //
    // Initialize the security descriptor.
    //

    securityDescriptor = (PSECURITY_DESCRIPTOR)securityDescriptorBuffer;

    result = InitializeSecurityDescriptor(
                 securityDescriptor,
                 SECURITY_DESCRIPTOR_REVISION
                 );

    if( !result ) {

        return GetLastError();

    }

    //
    // Add a NULL DACL to the security descriptor.
    //

    result = SetSecurityDescriptorDacl(
                 securityDescriptor,        // psd
                 TRUE,                      // fDaclPresent
                 NULL,                      // pAcl
                 FALSE                      // pDaclDefaulted
                 );

    if( !result ) {

        return GetLastError();

    }

    //
    // Create the mutex.
    //

    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.lpSecurityDescriptor = securityDescriptor;
    securityAttributes.bInheritHandle = FALSE;

    *Handle = CreateMutexA(
                  &securityAttributes,      // lpMutexAttributes
                  FALSE,                    // bInitialOwner
                  CATALOG_MUTEX_NAME        // lpName
                  );

    if( *Handle == NULL ) {

        return GetLastError();

    }

    //
    // Success!
    //

    return NO_ERROR;

}   // CreateProtocolCatalogMutex

DWORD
AcquireProtocolCatalogMutex(
    HANDLE Handle
    )

/*++

Routine Description:

    Acquires ownership of the mutex protecting the protocol catalog.

Arguments:

    Handle - A handle to the protocol catalog mutex.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD result;

    //
    // Sanity check.
    //

    DBG_ASSERT( Handle != NULL );

    //
    // Wait for ownership of the mutex.
    //

    result = WaitForSingleObject(
                 Handle,
                 INFINITE
                 );

    if( result == WAIT_FAILED ){

        return GetLastError();

    }

    //
    // Success!
    //

    return NO_ERROR;

}   // AcquireProtocolCatalogMutex

DWORD
ReleaseProtocolCatalogMutex(
    HANDLE Handle
    )

/*++

Routine Description:

    Relinquishes ownership of the mutex protecting the protocol catalog.

Arguments:

    Handle - A handle to the protocol catalog mutex.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    BOOL result;

    //
    // Sanity check.
    //

    DBG_ASSERT( Handle != NULL );

    //
    // Relinquish ownership of the mutex.
    //

    result = ReleaseMutex( Handle );

    if( !result ) {

        return GetLastError();

    }

    //
    // Success!
    //

    return NO_ERROR;

}   // ReleaseProtocolCatalogMutex
#endif // if 0

DWORD
RemoveAllInstalledProviders(
    HKEY ProvidersKey
    )

/*++

Routine Description:

    Removes all installed providers. This is used to cleanup everything
    after a setup version update has been detected.

Arguments:

    ProvidersKey - A handle to the ...\Setup Migration\Providers registry
        key.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    FILETIME lastWriteTime;
    DWORD subkeyIndex;
    DWORD subkeyNameLength;
    WCHAR subkeyName[MAX_REGISTRY_NAME];

    //
    // Sanity check.
    //

    DBG_ASSERT( ProvidersKey != NULL );

    //
    // Enumerate & remove the providers.
    //

    subkeyIndex = 0;

    for( ; ; ) {

        subkeyNameLength = sizeof(subkeyName);

        error = RegEnumKeyEx(
                    ProvidersKey,
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

        error = RemoveProviderByName(
                    ProvidersKey,
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
    // Success!
    //

    error = NO_ERROR;

exit:

    return error;

}   // RemoveAllInstalledProviders

DWORD
AppendStringToMultiSz(
    LPTSTR * MultiSz,
    LPTSTR String
    )

/*++

Routine Description:

    Appends a string to a MULTI_SZ. If this is the first attempt, then
    the MULTI_SZ is created.

Arguments:

    MultiSz - Points to a pointer to the MULTI_SZ. MutliSz may never be
        NULL, but *MultiSz may be NULL if this routine is to create a
        new MULTI_SZ.

    String - The string to append.
        key.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    LPTSTR scan;
    LPTSTR newMultiSz;
    DWORD stringLength;

    //
    // Sanity check.
    //

    DBG_ASSERT( MultiSz != NULL );
    DBG_ASSERT( String != NULL );

    //
    // Calculate the length of the new string.
    //

    stringLength = _tcslen( String ) + 1;

    //
    // If this is the first append, then create a new MULTI_SZ.
    // Otherwise, calculate the new MULTI_SZ length, allocate another
    // buffer, and copy the old string into the new buffer.
    //

    if( *MultiSz == NULL ) {

        newMultiSz = ALLOC_MEM( ( stringLength + 1 ) * sizeof(TCHAR) );

        scan = newMultiSz;

    } else {

        for( scan = *MultiSz ;
             *scan != TEXT('\0') ;
             scan += _tcslen( scan ) + 1 ) {

            // Empty.

        }

        newMultiSz = ALLOC_MEM((ULONG)
                        ( ( scan - *MultiSz ) + stringLength + 1 ) *
                            sizeof(TCHAR) );

        if( newMultiSz != NULL ) {

            RtlCopyMemory(
                newMultiSz,
                *MultiSz,
                ( scan - *MultiSz ) * sizeof(TCHAR)
                );

            scan = newMultiSz + ( scan - *MultiSz );

        }

    }

    //
    // Bail if the allocation failed.
    //

    if( newMultiSz == NULL ) {

        return ERROR_NOT_ENOUGH_MEMORY;

    }

    //
    // Free the old MULTI_SZ.
    //

    if( *MultiSz != NULL ) {

        FREE_MEM( *MultiSz );

    }

    //
    // Append the new string onto the new MULTI_SZ.
    //

    _tcscpy( scan, String );
    scan[stringLength] = TEXT('\0');

    //
    // Success!
    //

    *MultiSz = newMultiSz;

    return NO_ERROR;

}   // AppendStringToMultiSz

DWORD
SanitizeWinsock2ConfigForProvider(
    LPGUID ProviderId
    )

/*++

Routine Description:

    Attempts to "sanitize" the Winsock 2 configuration for a specific
    provider after an installation attempt for that provider fails
    unexpectedly.

Arguments:

    ProviderId - Identifies the provider.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    INT result;
    INT error;

    //
    // Since we're using GUIDs instead of those goofy WS2_32.DLL-generated
    // DWORDs to identify providers, the only thing we need to do to
    // sanitize the configuration is attempt to remove the provider. If that
    // fails, we're probably screwed, but we'll press on regardless.
    //

    result = (WSCDeinstallProviderProc)(
                 ProviderId,
                 &error
                 );

    if( result == SOCKET_ERROR ) {

        if( error == WSAEFAULT ) {

            //
            // This just means that the provider didn't really
            // exist, so we won't even whine about this one.
            //

        } else {

            DBG_PRINT((
                "Cannot sanitize, error %d, ignoring\n",
                error
                ));

        }

        error = NO_ERROR;

    }
    else
        error = NO_ERROR;

    //
    // Press on regardless.
    //

    DBG_ASSERT( error == NO_ERROR );

    return error;

}   // SanitizeWinsock2ConfigForProvider


DWORD
DetermineGuidForProvider(
    HKEY WellKnownGuidsKey,
    LPTSTR ProviderName,
    LPTSTR ProviderDllPath,
    LPGUID ProviderId
    )

/*++

Routine Description:

    Determines the appropriate GUID for the specified provider. If the
    provider's helper DLL supports the WSHGetProviderGuid() entrypoint,
    we'll call it to get the GUID. Otherwise, we'll look it up in the
    Well Known Guids catalog. If that fails, we'll create one from
    scratch and add it to the catalog.

Arguments:

    WellKnownGuidsKey - A handle to the ...\Setup Migration\Well Known
        Guids registry key.

    ProviderName - The name of the provider being installed.

    ProviderDllPath - The fully expanded path to the provider's helper DLL.

    ProviderId - Will receive the provider's GUID if successful.

Return Value:

    DWORD - A Win32 status code, 0 if successful, !0 otherwise.

--*/

{

    DWORD error;
    HMODULE helperDllHandle;
    PWSH_GET_PROVIDER_GUID getProviderGuid;

    //
    // Sanity check.
    //

    DBG_ASSERT( WellKnownGuidsKey != NULL );
    DBG_ASSERT( ProviderName != NULL );
    DBG_ASSERT( ProviderDllPath != NULL );
    DBG_ASSERT( ProviderId != NULL );

    //
    // Load the DLL and find the entrypoint.
    //

    helperDllHandle = LoadLibrary( ProviderDllPath );

    if( helperDllHandle != NULL ) {

        getProviderGuid = (PVOID)GetProcAddress(
                                     helperDllHandle,
                                     "WSHGetProviderGuid"
                                     );

        if( getProviderGuid != NULL ) {

            //
            // Protect ourselves in case the helper DLL does something
            // stupid.
            //

            try {

                //
                // Call it to get the GUID.
                //

                error = (DWORD)getProviderGuid(
                                   ProviderName,
                                   ProviderId
                                   );

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                error = GetExceptionCode();

            }

            if( error == NO_ERROR ) {

                //
                // Success!
                //

                FreeLibrary( helperDllHandle );
                return NO_ERROR;

            }

        }

        FreeLibrary( helperDllHandle );

    }

    DBG_PRINT((
        "DetermineGuidForProvider: cannot get GUID from %ls for %ls\n",
        ProviderDllPath,
        ProviderName
        ));

    //
    // If we made it this far, then either the helper DLL doesn't support
    // the WSHGetProviderGuid() entrypoint, or it threw an exception when
    // we called it. In any case, see if the provider has an entry in the
    // well known GUIDs catalog.
    //

    error = ReadGuid(
                WellKnownGuidsKey,
                ProviderName,
                ProviderId
                );

    if( error == NO_ERROR ) {

        //
        // Success!
        //

        return NO_ERROR;

    }

    //
    // Bummer. We'll need to create a GUID from scratch and add it to the
    // catalog.
    //

    error = UuidCreate(
                ProviderId
                );

    if( error == NO_ERROR ) {

        error = WriteGuid(
                    WellKnownGuidsKey,
                    ProviderName,
                    ProviderId
                    );

    } else {

        //
        // Total bummer, UuidCreate() failed.
        //

        DBG_PRINT((
            "DetermineGuidForProvider: UuidCreate() failed, error %d\n",
            error
            ));

    }

    return error;

}   // DetermineGuidForProvider

