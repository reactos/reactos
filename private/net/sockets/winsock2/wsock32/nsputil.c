/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    Nsputil.c

Abstract:

    This module contains support for the Name Space Provider utility APIs
    such as GetTypeByName().

Author:

    David Treadwell (davidtr)    22-Apr-1994

Revision History:

--*/

#ifdef CHICAGO
#undef UNICODE
#else
#define UNICODE
#define _UNICODE
#endif

#include "winsockp.h"
#include <nspapi.h>
#include <nspapip.h>
#include <svcguid.h>
#include <rpc.h>
#include <nspmisc.h>

//
// Keep an array of well-known services hard-coded.
//

struct _KNOWN_GUIDS {
    LPTSTR TypeName;
    GUID Guid;
} KnownGuids[] = {
    { TEXT( "hostname" ),                SVCID_HOSTNAME },
    { TEXT( "printqueue" ),              SVCID_PRINT_QUEUE },
    { TEXT( "fileserver" ),              SVCID_FILE_SERVER },
    { TEXT( "jobserver" ),               SVCID_JOB_SERVER },
    { TEXT( "gateway" ),                 SVCID_GATEWAY },
    { TEXT( "printserver" ),             SVCID_PRINT_SERVER },
    { TEXT( "archivequeue" ),            SVCID_ARCHIVE_QUEUE },
    { TEXT( "archiveserver" ),           SVCID_ARCHIVE_SERVER },
    { TEXT( "jobqueue" ),                SVCID_JOB_QUEUE },
    { TEXT( "administration" ),          SVCID_ADMINISTRATION },
    { TEXT( "snagateway" ),              SVCID_NAS_SNA_GATEWAY },
    { TEXT( "remotebridge" ),            SVCID_REMOTE_BRIDGE_SERVER },
    { TEXT( "timesyncserver" ),          SVCID_TIME_SYNCHRONIZATION_SERVER },
    { TEXT( "archiveserversap" ),        SVCID_ARCHIVE_SERVER_DYNAMIC_SAP },
    { TEXT( "advprintserver" ),          SVCID_ADVERTISING_PRINT_SERVER },
    { TEXT( "btrievevap" ),              SVCID_BTRIEVE_VAP },
    { TEXT( "directoryserver" ),         SVCID_DIRECTORY_SERVER },
    { TEXT( "netware386" ),              SVCID_NETWARE_386 },
    { TEXT( "hpprintserver" ),           SVCID_HP_PRINT_SERVER },
    { TEXT( "snaserver" ),               SVCID_SNA_SERVER },
    { TEXT( "saaserver" ),               SVCID_SAA_SERVER }
};

#define KNOWN_GUID_COUNT (sizeof(KnownGuids) / sizeof(struct _KNOWN_GUIDS))


INT
APIENTRY
GetTypeByName (
    IN     LPTSTR          lpServiceName,
    IN OUT LPGUID          lpServiceType
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    INT err;
    HKEY serviceTypesKey;
    HKEY serviceKey;
    TCHAR guidString[100];
    DWORD length;
    DWORD type;
    PSERVENT servent;
    PSTR ansiServiceName;
    INT i;

    //
    // Open the key that stores the name space provider info.
    //

    err = RegOpenKeyEx(
              HKEY_LOCAL_MACHINE,
              NSP_SERVICE_KEY_NAME,
              0,
              KEY_READ,
              &serviceTypesKey
              );

    if ( err == NO_ERROR ) {

        //
        // Open the key for this particular service.
        //

        err = RegOpenKeyEx(
                  serviceTypesKey,
                  lpServiceName,
                  0,
                  KEY_READ,
                  &serviceKey
                  );
        RegCloseKey( serviceTypesKey );

        //
        // If the key exists then we will read the GUID from the registry.
        //

        if ( err == NO_ERROR ) {

            //
            // Query the GUID value for the service.
            //

            length = sizeof(guidString);

            err = RegQueryValueEx(
                      serviceKey,
                      TEXT("GUID"),
                      NULL,
                      &type,
                      (PVOID)guidString,
                      &length
                      );
            RegCloseKey( serviceKey );
            if ( err != NO_ERROR ) {
                SetLastError( err );
                return -1;
            }

            //
            // Convert the Guid string to a proper Guid representation.
            // Before calling the conversion routine, we must strip the
            // leading and trailing braces from the string.
            //

            guidString[_tcslen(guidString) - 1] = L'\0';

            err = UuidFromString( guidString + 1, lpServiceType );
            if ( err != NO_ERROR ) {
                SetLastError( err );
                return -1;
            }

            return NO_ERROR;
        }
    }

    //
    // The key doesn't exist.  Check if it is a well-known TCP or UDP
    // service type.
    //

    ansiServiceName = GetAnsiName( lpServiceName );

    if ( ansiServiceName != NULL ) {

        servent = getservbyname( ansiServiceName, "tcp" );

        //
        // If getservbyname() worked, convert the port number into the
        // GUID corresponding to that port.
        //

        if ( servent != NULL ) {
            FREE_HEAP( ansiServiceName );
            SET_TCP_SVCID( lpServiceType, htons(servent->s_port) );
            return NO_ERROR;
        }

        //
        // Repeat the getservbyname() lookup for UDP.
        //

        servent = getservbyname( ansiServiceName, "udp" );
        FREE_HEAP( ansiServiceName );

        if ( servent != NULL ) {
            SET_UDP_SVCID( lpServiceType, htons(servent->s_port) );
            return NO_ERROR;
        }
    }

    //
    // This name isn't listed in the TCP/IP services file.  Check if it
    // is one of the well-known services hardcoded into this DLL.
    //

    for ( i = 0; i < KNOWN_GUID_COUNT; i++ ) {
        if ( _tcscmp( lpServiceName, KnownGuids[i].TypeName ) == 0 ) {
            RtlCopyMemory( lpServiceType, &KnownGuids[i].Guid, sizeof(GUID) );
            return NO_ERROR;
        }
    }

    SetLastError( ERROR_SERVICE_DOES_NOT_EXIST );

    return -1;

} // GetTypeByName

#if defined(UNICODE)


INT
APIENTRY
GetTypeByNameA (
    IN     LPSTR           lpServiceName,
    IN OUT LPGUID          lpServiceType
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;
    NTSTATUS status;
    INT err;

    //
    // Convert the service name to Unicode and call the Unicode version
    // of this routine.
    //

    RtlInitAnsiString( &ansiString, lpServiceName );
    status = RtlAnsiStringToUnicodeString( &unicodeString, &ansiString, TRUE );
    if ( !NT_SUCCESS(status) ) {
        SetLastError( RtlNtStatusToDosError( status ) );
        return -1;
    }

    err = GetTypeByNameW( unicodeString.Buffer, lpServiceType );

    RtlFreeUnicodeString( &unicodeString );

    return err;

} // GetTypeByNameA

#else // defined(UNICODE)

INT
APIENTRY
GetTypeByNameW (
    IN     LPWSTR          lpServiceName,
    IN OUT LPGUID          lpServiceType
    )
{

    SetLastError( ERROR_NOT_SUPPORTED );
    return -1;

} // GetTypeByNameW

#endif // defined(UNICODE)


INT
APIENTRY
GetNameByType (
    IN     LPGUID          lpServiceType,
    IN OUT LPTSTR          lpServiceName,
    IN     DWORD           dwNameLength
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    INT err;
    DWORD i;
    BOOL ret;
    HKEY serviceTypesKey;
    DWORD keyIndex;
    TCHAR serviceName[255];
    DWORD nameLength;
    FILETIME lastWriteTime;
    GUID guid;
    PSERVENT servent;

    //
    // Open the key that stores the name space provider info.
    //

    err = RegOpenKeyEx(
              HKEY_LOCAL_MACHINE,
              NSP_SERVICE_KEY_NAME,
              0,
              KEY_READ,
              &serviceTypesKey
              );

    if ( err == NO_ERROR ) {

        //
        // Walk through the service keys, checking whether each one
        // corresponds to the Guid we're checking against.
        //

        keyIndex = 0;
        nameLength = sizeof(serviceName);

        while ( (err = RegEnumKeyEx(
                           serviceTypesKey,
                           keyIndex,
                           serviceName,
                           &nameLength,
                           NULL,
                           NULL,
                           NULL,
                           &lastWriteTime) == NO_ERROR ) ) {

            //
            // Get the Guid for this service type.
            //

            err = GetTypeByName( serviceName, &guid );

            if ( err == NO_ERROR ) {

                //
                // Check whether this Guid matches the one we're looking for.
                //

                if ( GuidEqual( lpServiceType, &guid ) ) {

                    //
                    // We have a match.  Check whether the user buffer is
                    // large enough to hold this service name.
                    //

                    RegCloseKey( serviceTypesKey );

                    nameLength = (_tcslen( serviceName ) + 1) * sizeof(TCHAR);

                    if ( dwNameLength < nameLength ) {
                        SetLastError( ERROR_INSUFFICIENT_BUFFER );
                        return -1;
                    }

                    //
                    // Copy the service name to the user buffer and indicate
                    // success to the caller.
                    //

                    memcpy( lpServiceName, serviceName, nameLength );
                    return NO_ERROR;
                }
            }

            //
            // Update locals for the next call to RegEnumKeyEx.
            //

            keyIndex++;
            nameLength = sizeof(serviceName);
        }

        RegCloseKey( serviceTypesKey );
    }

    //
    // The key doesn't exist.  Check if it is a well-known TCP or UDP
    // service type.
    //

    if ( IS_SVCID_TCP( lpServiceType ) ) {

        servent = getservbyport( htons(PORT_FROM_SVCID_TCP(lpServiceType)), "tcp" );
        if ( servent == NULL ) {
            SetLastError( ERROR_SERVICE_DOES_NOT_EXIST );
            return -1;
        }

        ret = WriteAnsiName( lpServiceName, dwNameLength, servent->s_name );
        if ( !ret ) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return -1;
        }

        return NO_ERROR;
    }

    if ( IS_SVCID_UDP( lpServiceType ) ) {

        servent = getservbyport( htons(PORT_FROM_SVCID_UDP(lpServiceType)), "udp" );
        if ( servent == NULL ) {
            SetLastError( ERROR_SERVICE_DOES_NOT_EXIST );
            return -1;
        }

        ret = WriteAnsiName( lpServiceName, dwNameLength, servent->s_name );
        if ( !ret ) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return -1;
        }

        return NO_ERROR;
    }

    //
    // This name isn't listed in the TCP/IP services file.  Check if it
    // is one of the well-known services hardcoded into this DLL.
    //

    for ( i = 0; i < KNOWN_GUID_COUNT; i++ ) {

        if ( GuidEqual( lpServiceType, &KnownGuids[i].Guid ) ) {

            if ( _tcslen( KnownGuids[i].TypeName ) + 1 > dwNameLength ) {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                return -1;
            }

            _tcscpy( lpServiceName, KnownGuids[i].TypeName );

            return NO_ERROR;
        }
    }

    //
    // We didn't find a match.  Fail.
    //

    SetLastError( ERROR_SERVICE_DOES_NOT_EXIST );

    return -1;

} // GetNameByType

#if defined(UNICODE)


INT
APIENTRY
GetNameByTypeA (
    IN     LPGUID          lpServiceType,
    IN OUT LPSTR           lpServiceName,
    IN     DWORD           dwNameLength
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;
    INT err;
    NTSTATUS status;
    PWCHAR buffer;

    //
    // Allocate space to hold the Unicode name buffer.
    //

    buffer = ALLOCATE_HEAP( dwNameLength * sizeof(WCHAR) );
    if ( buffer == NULL ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return -1;
    }

    //
    // Call the Unicode version of this routine to do the actual work.
    //

    err = GetNameByTypeW( lpServiceType, buffer, dwNameLength * sizeof(WCHAR) );
    if ( err != NO_ERROR ) {
        FREE_HEAP( buffer );
        return -1;
    }

    //
    // Convert the unicode string to ANSI and return.
    //

    RtlInitUnicodeString( &unicodeString, buffer );
    ansiString.Length = (WORD)dwNameLength;
    ansiString.MaximumLength = (WORD)dwNameLength;
    ansiString.Buffer = lpServiceName;

    status = RtlUnicodeStringToAnsiString( &ansiString, &unicodeString, FALSE );
    FREE_HEAP( buffer );

    if ( !NT_SUCCESS(status) ) {
        return -1;
    }

    return NO_ERROR;

} // GetNameByTypeA

#else

INT
APIENTRY
GetNameByTypeW (
    IN     LPGUID          lpServiceType,
    IN OUT LPWSTR          lpServiceName,
    IN     DWORD           dwNameLength
    )
{

    SetLastError( ERROR_NOT_SUPPORTED );
    return -1;

} // GetNameByTypeW

#endif // defined(UNICODE)


LPSTR
GetAnsiName (
    IN LPTSTR Name
    )
{

#if defined(UNICODE)

    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;
    NTSTATUS status;

    RtlInitUnicodeString( &unicodeString, Name );

    ansiString.MaximumLength = unicodeString.MaximumLength + 1;
    ansiString.Buffer = ALLOCATE_HEAP( ansiString.MaximumLength );
    if ( ansiString.Buffer == NULL ) {
        return NULL;
    }

    status = RtlUnicodeStringToAnsiString( &ansiString, &unicodeString, FALSE );
    if ( !NT_SUCCESS(status) ) {
        FREE_HEAP( ansiString.Buffer );
        return NULL;
    }

    return ansiString.Buffer;

#else // defined(UNICODE)

    PSTR newName;

    newName = ALLOCATE_HEAP( strlen( Name ) + 1 );
    if ( newName == NULL ) {
        return NULL;
    }

    _tcscpy( newName, Name );

    return newName;

#endif

} // GetAnsiName


LPSTR
GetOemName (
    IN LPWSTR Name
    )
{
    PWCHAR pszTemp;
    PCHAR  pszBuffer;
    DWORD  dwCount;
    DWORD  dwLen;

    dwCount = LCMapStringW( LOCALE_SYSTEM_DEFAULT,
                            LCMAP_UPPERCASE,
                            Name,
                            -1,
                            NULL,
                            0 );

    dwLen = (dwCount + 1) * sizeof(WCHAR);

    pszTemp = ALLOCATE_HEAP(dwLen);

    if ( pszTemp == NULL )
    {
        return NULL;
    }

    if(!LCMapStringW( LOCALE_SYSTEM_DEFAULT,
                      LCMAP_UPPERCASE,
                      Name,
                      -1,
                      pszTemp,
                      dwCount ))
    {
        FREE_HEAP( pszTemp );
        return NULL;
    }

    dwLen = WideCharToMultiByte( CP_OEMCP,
                                 0,
                                 pszTemp,
                                 -1,
                                 NULL,
                                 0,
                                 0,
                                 NULL );

    pszBuffer = ALLOCATE_HEAP( dwLen );

    if ( pszBuffer == NULL )
    {
        FREE_HEAP( pszTemp );
        return NULL;
    }

    if( !WideCharToMultiByte( CP_OEMCP,
                              0,
                              pszTemp,
                              -1,
                              pszBuffer,
                              dwLen,
                              0,
                              NULL ) )
    {
        FREE_HEAP( pszBuffer );
        FREE_HEAP( pszTemp );
        return NULL;
    }

    FREE_HEAP( pszTemp );

    return(pszBuffer);
} // GetOemName



BOOL
WriteAnsiName (
    IN PTSTR Name,
    IN DWORD NameLength,
    IN PSTR AnsiName
    )
{

#if defined(UNICODE)

    UNICODE_STRING unicodeString;
    ANSI_STRING ansiString;
    NTSTATUS status;

    RtlInitAnsiString( &ansiString, AnsiName );

    unicodeString.MaximumLength = (SHORT)NameLength;
    unicodeString.Buffer = Name;

    status = RtlAnsiStringToUnicodeString( &unicodeString, &ansiString, FALSE );
    if ( !NT_SUCCESS(status) ) {
        return FALSE;
    }

    return TRUE;

#else

    if ( strlen( AnsiName ) + 1 > NameLength ) {
        return FALSE;
    }

    strcpy( Name, AnsiName );

    return TRUE;

#endif

} // WriteAnsiName

