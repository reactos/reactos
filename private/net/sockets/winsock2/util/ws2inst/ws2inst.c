/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    ws2inst.c

Abstract:

    This is a temporary WinSock 2 transport & namespace service provider
    installation utility.

    The command line arguments for this utility are:

        ws2inst operation file.ini [options]

    Where operation is one of the following:

        install - Installs the service provider[s] specified in the
            .INI file.

        remove - Removes the service provider[s] specified in the .INI
            file.

    And options may be one or more of the following:

        -t - Install/remove transport provider only.

        -n - Install/remove namespace provider only.

        -b - Install/remove transport and namespace providers (default).

        -e - Ignore errors.

    The layout of the .INI file is:

        [WinSock 2 Transport Service Providers]
        ProviderCount = N
        Provider0 = Transport_Provider_0_Section_Name
        Provider1 = Transport_Provider_1_Section_Name
        Provider2 = Transport_Provider_2_Section_Name
                        .
                        .
                        .
        ProviderN-1 = Transport_Provider_N-1_Section_Name

        [Transport_Provider_X_Section_Name]
        ProviderName = Provider_Name
        ProviderPath = Path_To_Providers_Dll
        ProviderId = Provider_Guid
        ProtocolCount = M

        [Transport_Provider_X_Section_Name Protocol M]
        dwServiceFlags1 = x
        dwServiceFlags2 = x
        dwServiceFlags3 = x
        dwServiceFlags4 = x
        dwProviderFlags = x
        iVersion = x
        iAddressFamily = x
        iMaxSockAddr = x
        iMinSockAddr = x
        iSocketType = x
        iProtocol = x
        iProtocolMaxOffset = x
        iNetworkByteOrder = x
        iSecurityScheme = x
        dwMessageSize = x
        dwProviderReserved = x
        szProtocol = Protocol_Name

        [WinSock 2 Name Space Providers]
        ProviderCount = N
        Provider0 = Name_Space_Provider_0_Section_Name
        Provider1 = Name_Space_Provider_1_Section_Name
        Provider2 = Name_Space_Provider_2_Section_Name
                        .
                        .
                        .
        ProviderN-1 = Name_Space_Provider_N-1_Section_Name

        [Name_Space_Provider_X_Section_Name]
        ProviderName = Provider_Name
        ProviderPath = Path_To_Providers_DLL
        ProviderId = Provider_Guid
        NameSpaceId = Name_Space_ID

Author:

    Keith Moore (keithmo)       17-Jun-1996

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//

#define MAX_INIFILE_LINE        256
#define TRANSPORT_SECTION_NAME  TEXT("WinSock 2 Transport Service Providers")
#define NAMESPACE_SECTION_NAME  TEXT("WinSock 2 Name Space Providers")

#define OPTION_FLAG_TRANSPORT       0x00000001
#define OPTION_FLAG_NAMESPACE       0x00000002
#define OPTION_FLAG_BOTH            0x00000003  // transport and namespace
#define OPTION_FLAG_IGNORE_ERRORS   0x00000004


//
// Private types.
//

typedef
BOOL
(CALLBACK * LPFN_ENUM_SECTION_CALLBACK)(
    LPTSTR IniFile,
    LPTSTR ProviderSectionName,
    DWORD Context
    );


//
// Private prototypes.
//

VOID
Usage(
    VOID
    );

VOID
InstallFromIniFile(
    LPTSTR IniFile,
    DWORD Options
    );

VOID
RemoveFromIniFile(
    LPTSTR IniFile,
    DWORD Options
    );

VOID
EnumProviderSections(
    LPTSTR IniFile,
    LPTSTR SectionName,
    LPFN_ENUM_SECTION_CALLBACK Callback,
    DWORD Context
    );

BOOL
InstallTransportProviderCallback(
    LPTSTR IniFile,
    LPTSTR SectionName,
    DWORD Context
    );

BOOL
RemoveTransportProviderCallback(
    LPTSTR IniFile,
    LPTSTR SectionName,
    DWORD Context
    );

BOOL
InstallNameSpaceProviderCallback(
    LPTSTR IniFile,
    LPTSTR SectionName,
    DWORD Context
    );

BOOL
RemoveNameSpaceProviderCallback(
    LPTSTR IniFile,
    LPTSTR SectionName,
    DWORD Context
    );


//
// Public functions.
//


INT
__cdecl
_tmain(
    INT argc,
    LPTSTR argv[]
    )

/*++

Routine Description:

    Main program entrypoint.

Arguments:

    argc - Number of command line arguments.

    argv - Array of pointers to command line arguments.

Return Value:

    INT - Completion status.

--*/

{

    LPTSTR opCode;
    LPTSTR iniFile;
    LPTSTR arg;
    DWORD options;
    INT i;

    //
    // Interpret the command line arguments.
    //

    if( argc < 3 ) {

        Usage();
        return 1;

    }

    opCode = argv[1];
    iniFile = argv[2];
    options = 0;

    for( i = 3 ; i < argc ; i++ ) {

        arg = argv[i];

        if( *arg != TEXT('-') ) {

            Usage();
            return 1;

        }

        arg++;

        while( *arg != TEXT('\0') ) {

            switch( *arg++ ) {

            case TEXT('t') :
            case TEXT('T') :
                options |= OPTION_FLAG_TRANSPORT;
                break;

            case TEXT('n') :
            case TEXT('N') :
                options |= OPTION_FLAG_NAMESPACE;
                break;

            case TEXT('b') :
            case TEXT('B') :
                options |= OPTION_FLAG_BOTH;
                break;

            case TEXT('e') :
            case TEXT('E') :
                options |= OPTION_FLAG_IGNORE_ERRORS;
                break;

            default :
                Usage();
                return 1;

            }

        }

    }

    //
    // Default == install transports and namespaces.
    //

    if( ( options & ( OPTION_FLAG_BOTH ) ) == 0 ) {

        options |= OPTION_FLAG_BOTH;

    }

    if( _tcsicmp( opCode, TEXT("install") ) == 0 ) {

        InstallFromIniFile(
            iniFile,
            options
            );

    } else if( _tcsicmp( opCode, TEXT("remove") ) == 0 ) {

        RemoveFromIniFile(
            iniFile,
            options
            );

    } else {

        Usage();
        return 1;

    }

    return 0;

}   // main


//
// Private functions.
//


VOID
Usage(
    VOID
    )

/*++

Routine Description:

    Displays this utility's proper command line parameters.

Arguments:

    None.

Return Value:

    None.

--*/

{

    _ftprintf(
        stderr,
        TEXT("WS2INST 0.03 by Keith Moore %hs\n")
        TEXT("use: WS2INST Operation file.ini [Options]\n")
        TEXT("where Operation may be one of the following:\n")
        TEXT("    install - Installs service providers specified in .INI file\n")
        TEXT("    remove - Removes service providers specified in .INI file\n")
        TEXT("and Options may be one or more of the following:\n")
        TEXT("    -t - Install/remove transport providers only\n")
        TEXT("    -n - Install/remove namespace providers only\n")
        TEXT("    -b - Install/remove both (default)\n")
        TEXT("    -i - Ignore errors\n"),
        __DATE__
        );

}   // Usage


VOID
InstallFromIniFile(
    LPTSTR IniFile,
    DWORD Options
    )

/*++

Routine Description:

    Installs transports and/or namespace providers specified in the
    given .INI file.

Arguments:

    IniFile - The .INI file describing the providers to install.

    Options - Behaviour control options (OPTION_FLAG_*).

Return Value:

    None.

--*/

{

    //
    // Let the user know what we're up to.
    //

    _tprintf(
        TEXT("Installing from %s\n"),
        IniFile
        );

    //
    // Install transport providers if so requested.
    //

    if( Options & OPTION_FLAG_TRANSPORT ) {

        EnumProviderSections(
            IniFile,
            TRANSPORT_SECTION_NAME,
            InstallTransportProviderCallback,
            Options
            );

    }

    //
    // Install namespace providers if so requested.
    //

    if( Options & OPTION_FLAG_NAMESPACE ) {

        EnumProviderSections(
            IniFile,
            NAMESPACE_SECTION_NAME,
            InstallNameSpaceProviderCallback,
            Options
            );

    }

}   // InstallFromIniFile


VOID
RemoveFromIniFile(
    LPTSTR IniFile,
    DWORD Options
    )

/*++

Routine Description:

    Removes transports and/or namespace providers specified in the
    given .INI file.

Arguments:

    IniFile - The .INI file describing the providers to remove.

    Options - Behaviour control options (OPTION_FLAG_*).

Return Value:

    None.

--*/

{

    //
    // Let the user know what we're up to.
    //

    _tprintf(
        TEXT("Removing from %s\n"),
        IniFile
        );

    //
    // Remove transport providers if so requested.
    //

    if( Options & OPTION_FLAG_TRANSPORT ) {

        EnumProviderSections(
            IniFile,
            TRANSPORT_SECTION_NAME,
            RemoveTransportProviderCallback,
            Options
            );

    }

    //
    // Remove namespace providers if so requested.
    //

    if( Options & OPTION_FLAG_NAMESPACE ) {

        EnumProviderSections(
            IniFile,
            NAMESPACE_SECTION_NAME,
            RemoveNameSpaceProviderCallback,
            Options
            );

    }

}   // RemoveFromIniFile


VOID
EnumProviderSections(
    LPTSTR IniFile,
    LPTSTR SectionName,
    LPFN_ENUM_SECTION_CALLBACK Callback,
    DWORD Context
    )

/*++

Routine Description:

    Enumerates the provider sections in the specified .INI file. The
    sections must be in the following format:

        [section_name]
        ProviderCount=N
        Provider0=provider_0_name
        Provider1=provider_1_name
        Provider2=provider_2_name
                .
                .
                .
        ProviderN-1=provider_N-1_name

Arguments:

    IniFile - The .INI file containing the sections to enumerate.

    SectionName - The name of the section to enumerate.

    Callback - Pointer to a callback routine. The callback is invoked
        once for each section. The prototype for the callback is:

            BOOL
            CALLBACK
            EnumSectionProc(
                LPTSTR IniFile,
                LPTSTR ProviderSectionName,
                DWORD Context
                );

        Where:

            IniFile - The .INI filename passed into EnumProviderSections().

            ProviderSectionName - The name of the current section.

            Context - The context value passed into EnumProviderSections().

        If the callback routine returns FALSE, then the enumeration is
        aborted. If the callback routine returns TRUE, then the enumeration
        is continued.

    Context - An uninterpreted context value passed to the callback function.

Return Value:

    None.

--*/

{

    TCHAR providerSectionName[MAX_INIFILE_LINE];
    TCHAR keyName[MAX_INIFILE_LINE];
    DWORD length;
    UINT providerCount;
    UINT i;
    BOOL result;

    //
    // Get the provider count.
    //

    providerCount = GetPrivateProfileInt(
                        SectionName,
                        TEXT("ProviderCount"),
                        0,
                        IniFile
                        );

    if( providerCount == 0 ) {

        return;

    }

    //
    // Do that enumeration thang.
    //

    for( i = 0 ; i < providerCount ; i++ ) {

        wsprintf(
            keyName,
            TEXT("Provider%u"),
            i
            );

        length = GetPrivateProfileString(
                     SectionName,
                     keyName,
                     TEXT(""),
                     providerSectionName,
                     sizeof(providerSectionName) / sizeof(providerSectionName[0]),
                     IniFile
                     );

        if( length > 0 ) {

            result = (Callback)(
                         IniFile,
                         providerSectionName,
                         Context
                         );

            if( !result ) {

                break;

            }

        }

    }

}   // EnumProviderSections


BOOL
InstallTransportProviderCallback(
    LPTSTR IniFile,
    LPTSTR SectionName,
    DWORD Context
    )

/*++

Routine Description:

    Callback routine for EnumProviderSections() that installs the
    transport service provider described by the given .INI file section.

Arguments:

    IniFile - The name of the .INI file describing the transport provider.

    SectionName - The name of the .INI file section for this provider.

    Context - Actually contains behaviour control options (OPTION_FLAG_*).

Return Value:

    BOOL - TRUE if successful, FALSE otherwise.

--*/

{

    TCHAR providerName[MAX_INIFILE_LINE];
    TCHAR providerPath[MAX_INIFILE_LINE];
    TCHAR providerIdString[MAX_INIFILE_LINE];
    TCHAR protocolSectionName[MAX_INIFILE_LINE];
    UINT protocolCount;
    UINT i;
    DWORD length;
    LPWSAPROTOCOL_INFO protocolInfo;
    INT result;
    INT error;
    GUID providerId;
    RPC_STATUS status;
    BOOL ignoreErrors;

    //
    // Let the user know what we're up to.
    //

    _tprintf(
        TEXT("Installing %s\n"),
        SectionName
        );

    //
    // Determine if we should ignore errors. If so, then this routine
    // will always return TRUE.
    //

    ignoreErrors = ( ( Context & OPTION_FLAG_IGNORE_ERRORS ) != 0 );

    //
    // Read the fixed information.
    //

    length = GetPrivateProfileString(
                 SectionName,
                 TEXT("ProviderName"),
                 TEXT(""),
                 providerName,
                 sizeof(providerName) / sizeof(providerName[0]),
                 IniFile
                 );

    if( length == 0 ) {

        _tprintf(
            TEXT("ERROR: missing ProviderName key\n")
            );

        return ignoreErrors;

    }

    length = GetPrivateProfileString(
                 SectionName,
                 TEXT("ProviderPath"),
                 TEXT(""),
                 providerPath,
                 sizeof(providerPath) / sizeof(providerPath[0]),
                 IniFile
                 );

    if( length == 0 ) {

        _tprintf(
            TEXT("ERROR: missing ProviderPath key\n")
            );

        return ignoreErrors;

    }

    protocolCount = GetPrivateProfileInt(
                        SectionName,
                        TEXT("ProtocolCount"),
                        0,
                        IniFile
                        );

    if( protocolCount == 0 ) {

        _tprintf(
            TEXT("ERROR: missing ProtocolCount key\n")
            );

        return ignoreErrors;

    }

    length = GetPrivateProfileString(
                 SectionName,
                 TEXT("ProviderId"),
                 TEXT(""),
                 providerIdString,
                 sizeof(providerIdString) / sizeof(providerIdString[0]),
                 IniFile
                 );

    if( length == 0 ) {

        _tprintf(
            TEXT("ERROR: missing ProviderId key\n")
            );

        return ignoreErrors;

    }

    //
    // Build the GUID.
    //

    status = UuidFromString(
                 providerIdString,
                 &providerId
                 );

    if( status != RPC_S_OK ) {

        _tprintf(
            TEXT("ERROR: invalid ProviderId %s\n"),
            providerIdString
            );

        return ignoreErrors;

    }

    //
    // Allocate the space for the protocol info structures.
    //

    protocolInfo = malloc( protocolCount * sizeof(*protocolInfo) );

    if( protocolInfo == NULL ) {

        _tprintf(
            TEXT("ERROR: out of memory\n")
            );

        return ignoreErrors;

    }

    //
    // Enumerate the protocols.
    //

    for( i = 0 ; i < protocolCount ; i++ ) {

        //
        // Build the section name for this protocol.
        //

        wsprintf(
            protocolSectionName,
            TEXT("%s Protocol %u"),
            SectionName,
            i
            );

        //
        // Read the individual protocol info.
        //

        protocolInfo[i].dwServiceFlags1 = (DWORD)GetPrivateProfileInt(
                                              protocolSectionName,
                                              TEXT("dwServiceFlags1"),
                                              0,
                                              IniFile
                                              );

        protocolInfo[i].dwServiceFlags2 = (DWORD)GetPrivateProfileInt(
                                              protocolSectionName,
                                              TEXT("dwServiceFlags2"),
                                              0,
                                              IniFile
                                              );

        protocolInfo[i].dwServiceFlags3 = (DWORD)GetPrivateProfileInt(
                                              protocolSectionName,
                                              TEXT("dwServiceFlags3"),
                                              0,
                                              IniFile
                                              );

        protocolInfo[i].dwServiceFlags4 = (DWORD)GetPrivateProfileInt(
                                              protocolSectionName,
                                              TEXT("dwServiceFlags4"),
                                              0,
                                              IniFile
                                              );

        protocolInfo[i].dwProviderFlags = (DWORD)GetPrivateProfileInt(
                                              protocolSectionName,
                                              TEXT("dwProviderFlags"),
                                              0,
                                              IniFile
                                              );

        protocolInfo[i].iVersion = (DWORD)GetPrivateProfileInt(
                                       protocolSectionName,
                                       TEXT("iVersion"),
                                       0,
                                       IniFile
                                       );

        protocolInfo[i].iAddressFamily = (DWORD)GetPrivateProfileInt(
                                              protocolSectionName,
                                              TEXT("iAddressFamily"),
                                              0,
                                              IniFile
                                              );

        protocolInfo[i].iMaxSockAddr = (DWORD)GetPrivateProfileInt(
                                           protocolSectionName,
                                           TEXT("iMaxSockAddr"),
                                           0,
                                           IniFile
                                           );

        protocolInfo[i].iMinSockAddr = (DWORD)GetPrivateProfileInt(
                                           protocolSectionName,
                                           TEXT("iMinSockAddr"),
                                           0,
                                           IniFile
                                           );

        protocolInfo[i].iSocketType = (DWORD)GetPrivateProfileInt(
                                          protocolSectionName,
                                          TEXT("iSocketType"),
                                          0,
                                          IniFile
                                          );

        protocolInfo[i].iProtocol = (DWORD)GetPrivateProfileInt(
                                        protocolSectionName,
                                        TEXT("iProtocol"),
                                        0,
                                        IniFile
                                        );

        protocolInfo[i].iProtocolMaxOffset = (DWORD)GetPrivateProfileInt(
                                                 protocolSectionName,
                                                 TEXT("iProtocolMaxOffset"),
                                                 0,
                                                 IniFile
                                                 );

        protocolInfo[i].iNetworkByteOrder = (DWORD)GetPrivateProfileInt(
                                                protocolSectionName,
                                                TEXT("iNetworkByteOrder"),
                                                0,
                                                IniFile
                                                );

        protocolInfo[i].iSecurityScheme = (DWORD)GetPrivateProfileInt(
                                              protocolSectionName,
                                              TEXT("iSecurityScheme"),
                                              0,
                                              IniFile
                                              );

        protocolInfo[i].dwMessageSize = (DWORD)GetPrivateProfileInt(
                                            protocolSectionName,
                                            TEXT("dwMessageSize"),
                                            0,
                                            IniFile
                                            );

        protocolInfo[i].dwProviderReserved = (DWORD)GetPrivateProfileInt(
                                                 protocolSectionName,
                                                 TEXT("dwProviderReserved"),
                                                 0,
                                                 IniFile
                                                 );

        length = GetPrivateProfileString(
                     protocolSectionName,
                     TEXT("szProtocol"),
                     TEXT(""),
                     protocolInfo[i].szProtocol,
                     sizeof(protocolInfo[i].szProtocol) / sizeof(protocolInfo[i].szProtocol[0]),
                     IniFile
                     );

        if( length == 0 ) {

            _tprintf(
                TEXT("ERROR: missing szProtocol key\n")
                );

            free( protocolInfo );
            return ignoreErrors;

        }

    }

    //
    // OK, we've got the protocol data, now just ask WS2_32.DLL to
    // install 'em.
    //

    result = WSCInstallProvider(
                 &providerId,
                 providerPath,
                 protocolInfo,
                 (DWORD)protocolCount,
                 &error
                 );

    free( protocolInfo );

    if( result == SOCKET_ERROR ) {

        _tprintf(
            TEXT("Cannot install %s, error %d\n"),
            providerName,
            error
            );

        return ignoreErrors;

    }

    return TRUE;

}   // InstallTransportProviderCallback


BOOL
RemoveTransportProviderCallback(
    LPTSTR IniFile,
    LPTSTR SectionName,
    DWORD Context
    )

/*++

Routine Description:

    Callback routine for EnumProviderSections() that removes the
    transport service provider described by the given .INI file section.

Arguments:

    IniFile - The name of the .INI file describing the transport provider.

    SectionName - The name of the .INI file section for this provider.

    Context - Actually contains behaviour control options (OPTION_FLAG_*).

Return Value:

    BOOL - TRUE if successful, FALSE otherwise.

--*/

{


    TCHAR providerName[MAX_INIFILE_LINE];
    TCHAR providerIdString[MAX_INIFILE_LINE];
    DWORD length;
    INT result;
    INT error;
    GUID providerId;
    RPC_STATUS status;
    BOOL ignoreErrors;

    //
    // Let the user know what we're up to.
    //

    _tprintf(
        TEXT("Removing %s\n"),
        SectionName
        );

    //
    // Determine if we should ignore errors. If so, then this routine
    // will always return TRUE.
    //

    ignoreErrors = ( ( Context & OPTION_FLAG_IGNORE_ERRORS ) != 0 );

    //
    // Read the provider name & ID.
    //

    length = GetPrivateProfileString(
                 SectionName,
                 TEXT("ProviderName"),
                 TEXT(""),
                 providerName,
                 sizeof(providerName) / sizeof(providerName[0]),
                 IniFile
                 );

    if( length == 0 ) {

        _tprintf(
            TEXT("ERROR: missing ProviderName key\n")
            );

        return ignoreErrors;

    }

    length = GetPrivateProfileString(
                     SectionName,
                     TEXT("ProviderId"),
                     TEXT(""),
                     providerIdString,
                     sizeof(providerIdString) / sizeof(providerIdString[0]),
                     IniFile
                     );

    if( length == 0 ) {

        _tprintf(
            TEXT("ERROR: missing ProviderId key\n")
            );

        return ignoreErrors;

    }

    //
    // Build the GUID.
    //

    status = UuidFromString(
                 providerIdString,
                 &providerId
                 );

    if( status != RPC_S_OK ) {

        _tprintf(
            TEXT("ERROR: invalid ProviderId %s\n"),
            providerIdString
            );

        return ignoreErrors;

    }

    //
    // Remove it.
    //

    result = WSCDeinstallProvider(
                 &providerId,
                 &error
                 );

    if( result == SOCKET_ERROR ) {

        _tprintf(
            TEXT("Cannot remove %s, error %d\n"),
            providerName,
            error
            );

        return ignoreErrors;

    }

    return TRUE;

}   // RemoveTransportProviderCallback


BOOL
InstallNameSpaceProviderCallback(
    LPTSTR IniFile,
    LPTSTR SectionName,
    DWORD Context
    )

/*++

Routine Description:

    Callback routine for EnumProviderSections() that installs the
    namespace service provider described by the given .INI file section.

Arguments:

    IniFile - The name of the .INI file describing the namespace provider.

    SectionName - The name of the .INI file section for this provider.

    Context - Actually contains behaviour control options (OPTION_FLAG_*).

Return Value:

    BOOL - TRUE if successful, FALSE otherwise.

--*/

{

    TCHAR providerName[MAX_INIFILE_LINE];
    TCHAR providerPath[MAX_INIFILE_LINE];
    TCHAR providerIdString[MAX_INIFILE_LINE];
    UINT i;
    DWORD length;
    INT result;
    INT error;
    GUID providerId;
    DWORD nameSpaceId;
    RPC_STATUS status;
    BOOL ignoreErrors;

    //
    // Let the user know what we're up to.
    //

    _tprintf(
        TEXT("Installing %s\n"),
        SectionName
        );

    //
    // Determine if we should ignore errors. If so, then this routine
    // will always return TRUE.
    //

    ignoreErrors = ( ( Context & OPTION_FLAG_IGNORE_ERRORS ) != 0 );

    //
    // Read the fixed information.
    //

    length = GetPrivateProfileString(
                 SectionName,
                 TEXT("ProviderName"),
                 TEXT(""),
                 providerName,
                 sizeof(providerName) / sizeof(providerName[0]),
                 IniFile
                 );

    if( length == 0 ) {

        _tprintf(
            TEXT("ERROR: missing ProviderName key\n")
            );

        return ignoreErrors;

    }

    length = GetPrivateProfileString(
                 SectionName,
                 TEXT("ProviderPath"),
                 TEXT(""),
                 providerPath,
                 sizeof(providerPath) / sizeof(providerPath[0]),
                 IniFile
                 );

    if( length == 0 ) {

        _tprintf(
            TEXT("ERROR: missing ProviderPath key\n")
            );

        return ignoreErrors;

    }

    length = GetPrivateProfileString(
                 SectionName,
                 TEXT("ProviderId"),
                 TEXT(""),
                 providerIdString,
                 sizeof(providerIdString) / sizeof(providerIdString[0]),
                 IniFile
                 );

    if( length == 0 ) {

        _tprintf(
            TEXT("ERROR: missing ProviderId key\n")
            );

        return ignoreErrors;

    }

    //
    // Build the GUID.
    //

    status = UuidFromString(
                 providerIdString,
                 &providerId
                 );

    if( status != RPC_S_OK ) {

        _tprintf(
            TEXT("ERROR: invalid ProviderId %s\n"),
            providerIdString
            );

        return ignoreErrors;

    }

    nameSpaceId = GetPrivateProfileInt(
                      SectionName,
                      TEXT("NameSpaceId"),
                      0,
                      IniFile
                      );

    if( nameSpaceId == 0 ) {

        _tprintf(
            TEXT("ERROR: missing NameSpaceId key\n")
            );

        return ignoreErrors;

    }

    //
    // Install it.
    //

    result = WSCInstallNameSpace(
                 providerName,
                 providerPath,
                 nameSpaceId,
                 2,
                 &providerId
                 );

    if( result == SOCKET_ERROR ) {

        error = GetLastError();

        _tprintf(
            TEXT("Cannot install %s, error %d\n"),
            providerName,
            error
            );

        return ignoreErrors;

    }

    return TRUE;

}   // InstallNameSpaceProviderCallback


BOOL
RemoveNameSpaceProviderCallback(
    LPTSTR IniFile,
    LPTSTR SectionName,
    DWORD Context
    )

/*++

Routine Description:

    Callback routine for EnumProviderSections() that removes the
    namespace service provider described by the given .INI file section.

Arguments:

    IniFile - The name of the .INI file describing the namespace provider.

    SectionName - The name of the .INI file section for this provider.

    Context - Actually contains behaviour control options (OPTION_FLAG_*).

Return Value:

    BOOL - TRUE if successful, FALSE otherwise.

--*/

{


    TCHAR providerName[MAX_INIFILE_LINE];
    TCHAR providerIdString[MAX_INIFILE_LINE];
    DWORD length;
    INT result;
    INT error;
    GUID providerId;
    RPC_STATUS status;
    BOOL ignoreErrors;

    //
    // Let the user know what we're up to.
    //

    _tprintf(
        TEXT("Removing %s\n"),
        SectionName
        );

    //
    // Determine if we should ignore errors. If so, then this routine
    // will always return TRUE.
    //

    ignoreErrors = ( ( Context & OPTION_FLAG_IGNORE_ERRORS ) != 0 );

    //
    // Read the provider name & ID.
    //

    length = GetPrivateProfileString(
                 SectionName,
                 TEXT("ProviderName"),
                 TEXT(""),
                 providerName,
                 sizeof(providerName) / sizeof(providerName[0]),
                 IniFile
                 );

    if( length == 0 ) {

        _tprintf(
            TEXT("ERROR: missing ProviderName key\n")
            );

        return ignoreErrors;

    }

    length = GetPrivateProfileString(
                     SectionName,
                     TEXT("ProviderId"),
                     TEXT(""),
                     providerIdString,
                     sizeof(providerIdString) / sizeof(providerIdString[0]),
                     IniFile
                     );

    if( length == 0 ) {

        _tprintf(
            TEXT("ERROR: missing ProviderId key\n")
            );

        return ignoreErrors;

    }

    //
    // Build the GUID.
    //

    status = UuidFromString(
                 providerIdString,
                 &providerId
                 );

    if( status != RPC_S_OK ) {

        _tprintf(
            TEXT("ERROR: invalid ProviderId %s\n"),
            providerIdString
            );

        return ignoreErrors;

    }

    //
    // Remove it.
    //

    result = WSCUnInstallNameSpace(
                 &providerId
                 );

    if( result == SOCKET_ERROR ) {

        error = GetLastError();

        _tprintf(
            TEXT("Cannot remove %s, error %d\n"),
            providerName,
            error
            );

        return ignoreErrors;

    }

    return TRUE;

}   // RemoveNameSpaceProviderCallback

