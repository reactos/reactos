/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    getproto.cpp

Abstract:

    This module handles the getprotobyX() functions.

    The following functions are exported by this module:

    getprotobyname()
    getprotobynumber()

Author:

    Keith Moore (keithmo)        18-Jun-1996

Revision History:

--*/


#include "precomp.h"
//#pragma hdrstop
//#include <stdio.h> Now in precomp.h


//
// Private contants.
//

#define DATABASE_PATH_REGISTRY_KEY \
            "System\\CurrentControlSet\\Services\\Tcpip\\Parameters"

#define DATABASE_PATH_REGISTRY_VALUE "DataBasePath"

#define PROTOCOL_DATABASE_FILENAME "protocol"


//
// Private prototypes.
//

FILE *
GetProtoOpenNetworkDatabase(
    CHAR * Name
    );

CHAR *
GetProtoPatternMatch(
    CHAR * Scan,
    CHAR * Match
    );

PPROTOENT
GetProtoGetNextEnt(
    FILE * DbFile,
    PGETPROTO_INFO ProtoInfo
    );


//
// Public functions.
//


struct protoent FAR *
WSAAPI
getprotobynumber(
    IN int number
    )
/*++
Routine Description:

    Get protocol information corresponding to a protocol number.

Arguments:

    number - Supplies a protocol number, in host byte order

Returns:

    If  no  error  occurs, getprotobynumber() returns a pointer to the protoent
    structure  described  above.   Otherwise  it  returns  a NULL pointer and a
    specific error r code is stored with SetErrorCode().
--*/
{

    PDPROCESS Process;
    PDTHREAD Thread;
    INT ErrorCode;
    PGETPROTO_INFO protoInfo;
    PPROTOENT pent;
    char ** ptr;
    FILE * dbFile;

    ErrorCode = PROLOG(&Process,
                 &Thread);
    if(ErrorCode != ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return(NULL);
    }

    //
    // Get the per-thread buffer.
    //

    protoInfo = Thread->GetProtoInfo();

    if( protoInfo == NULL ) {

        SetLastError( WSANO_DATA );
        return NULL;

    }

    //
    // Open the database file.
    //

    dbFile = GetProtoOpenNetworkDatabase( "protocol" );

    if( dbFile == NULL ) {

        SetLastError( WSANO_DATA );
        return NULL;

    }

    //
    // Scan it.
    //

    while( TRUE ) {

        pent = GetProtoGetNextEnt(
                   dbFile,
                   protoInfo
                   );

        if( pent == NULL ) {

            break;

        }

        if( (int)pent->p_proto == number ) {

            break;

        }

    }

    //
    // Close the database.
    //

    fclose( dbFile );

    if( pent == NULL ) {

        SetLastError( WSANO_DATA );

    }

    return pent;

}  // getprotobynumber


struct protoent FAR *
WSAAPI
getprotobyname(
    IN const char FAR * name
    )
/*++
Routine Description:

    Get protocol information corresponding to a protocol name.

Arguments:

    name - A pointer to a null terminated protocol name.

Returns:

    If  no  error  occurs,  getprotobyname()  returns a pointer to the protoent
    structure  described  above.   Otherwise  it  returns  a NULL pointer and a
    specific error code is stored with SetErrorCode().
--*/
{

    PDPROCESS Process;
    PDTHREAD Thread;
    INT ErrorCode;
    PGETPROTO_INFO protoInfo;
    PPROTOENT pent;
    char ** ptr;
    FILE * dbFile;

    ErrorCode = PROLOG(&Process,
                 &Thread);
    if(ErrorCode != ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return(NULL);
    }

    if ( !name ) // Bug fix for #112420
    {
        SetLastError(WSAEINVAL);
        return(NULL);
    }

    //
    // Get the per-thread buffer.
    //

    protoInfo = Thread->GetProtoInfo();

    if( protoInfo == NULL ) {

        SetLastError( WSANO_DATA );
        return NULL;

    }

    //
    // Open the database file.
    //

    dbFile = GetProtoOpenNetworkDatabase( "protocol" );

    if( dbFile == NULL ) {

        SetLastError( WSANO_DATA );
        return NULL;

    }

    //
    // Scan it.
    //

    while( TRUE ) {

        pent = GetProtoGetNextEnt(
                   dbFile,
                   protoInfo
                   );

        if( pent == NULL ) {

            break;

        }

        __try {
            if( _stricmp( pent->p_name, name ) == 0 ) {

                break;

            }

        }
        __except (WS2_EXCEPTION_FILTER()) {
            fclose (dbFile);
            SetLastError (WSAEFAULT);
            return NULL;
        }
    }

    //
    // Close the database.
    //

    fclose( dbFile );

    if( pent == NULL ) {

        SetLastError( WSANO_DATA );

    }

    return pent;

}  // getprotobyname


//
// Private functions.
//


FILE *
GetProtoOpenNetworkDatabase(
    CHAR * Name
    )

/*++

Routine Description:

    Opens a stream to the specified network database file.

Arguments:

    Name - The name of the database to open (i.e. "services" or
        "protocol").

Return Value:

    FILE * - Pointer to the open stream if successful, NULL if not.

--*/

{

    CHAR path[MAX_PATH];
    CHAR unexpanded[MAX_PATH];
    CHAR * suffix;
    OSVERSIONINFO version;
    LONG err;
    HKEY key;
    DWORD type;
    DWORD length;

    //
    // Determine the directory for the database file.
    //
    // Under Win95, the database files live under the Windows directory
    // (i.e. C:\WINDOWS).
    //
    // Under WinNT, the path to the database files is configurable in
    // the registry, but the default is in the Drivers\Etc directory
    // (i.e. C:\WINDOWS\SYSTEM32\DRIVERS\ETC).
    //

    version.dwOSVersionInfoSize = sizeof(version);

    if( !GetVersionEx( &version ) ) {

        return NULL;

    }

    suffix = "";

    if( version.dwPlatformId == VER_PLATFORM_WIN32_NT ) {

        //
        // We're running under NT, so try to get the path from the
        // registry.
        //

        err = RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE,
                  DATABASE_PATH_REGISTRY_KEY,
                  0,
                  KEY_READ,
                  &key
                  );

        if( err == NO_ERROR ) {

            length = sizeof(unexpanded);

            err = RegQueryValueEx(
                      key,
                      DATABASE_PATH_REGISTRY_VALUE,
                      NULL,
                      &type,
                      (LPBYTE)unexpanded,
                      &length
                      );

            RegCloseKey( key );

        }

        if( err == NO_ERROR ) {

            if( ExpandEnvironmentStrings(
                    unexpanded,
                    path,
                    sizeof(path)
                    ) == 0 ) {

                err = WSASYSCALLFAILURE;

            }

        }

        if( err != NO_ERROR ) {

            //
            // Couldn't get it from the registry, just use the default.
            //

            if( GetSystemDirectory(
                    path,
                    sizeof(path)
                    ) == 0 ) {

                return NULL;

            }

            suffix = "DRIVERS\\ETC\\";

        }

    } else {

        //
        // We're running under Win95, so just get the Windows directory.
        //

        if( GetWindowsDirectory(
                path,
                sizeof(path)
                ) == 0 ) {

            return NULL;

        }

    }

    //
    // Ensure the path has a trailing backslash, then tack on any suffix
    // needed, then tack on the filename.
    //

    if( path[strlen( path ) - 1] != '\\' ) {

        strcat( path, "\\" );

    }

    strcat( path, suffix );
    strcat( path, Name );

    //
    // Open the file, return the result.
    //

    return fopen( path, "rt" );

}   // GetProtoOpenNetworkDatabase


CHAR *
GetProtoPatternMatch(
    CHAR * Scan,
    CHAR * Match
    )

/*++

Routine Description:

    Finds the first character in Scan that matches any character in Match.

Arguments:

    Scan - The string to scan.

    Match - The list of characters to match against.

Return Value:

    CHAR * - Pointer to the first occurrance in Scan if successful,
        NULL if not.

--*/

{

    CHAR ch;

    while( ( ch = *Scan ) != '\0' ) {

        if( strchr( Match, ch ) != NULL ) {

            return Scan;

        }

        Scan++;

    }

    return NULL;

}   // GetProtoPatternMatch



PPROTOENT
GetProtoGetNextEnt(
    FILE * DbFile,
    PGETPROTO_INFO ProtoInfo
    )
{
    CHAR * ptr;
    CHAR * token;
    CHAR ** aliases;
    PPROTOENT result = NULL;

    while( TRUE ) {

        //
        // Get the next line, bail if EOF.
        //

        ptr = fgets(
                  ProtoInfo->TextLine,
                  MAX_PROTO_TEXT_LINE,
                  DbFile
                  );

        if( ptr == NULL ) {

            break;

        }

        //
        // Skip comments.
        //

        if( *ptr == '#' ) {

            continue;

        }

        token = GetProtoPatternMatch ( ptr, "#\n" );

        if( token == NULL ) {

            continue;

        }

        *token = '\0';

        //
        // Start building the entry.
        //

        ProtoInfo->Proto.p_name = ptr;

        token = GetProtoPatternMatch( ptr, " \t" );

        if( token == NULL ) {

            continue;

        }

        *token++ = '\0';

        while( *token == ' ' || *token == '\t' ) {

            token++;

        }

        ptr = GetProtoPatternMatch( token, " \t" );

        if( ptr != NULL ) {

            *ptr++ = '\0';

        }

        ProtoInfo->Proto.p_proto = (short)atoi( token );

        //
        // Build the alias list.
        //

        ProtoInfo->Proto.p_aliases = ProtoInfo->Aliases;
        aliases = ProtoInfo->Proto.p_aliases;

        if( ptr != NULL ) {

            token = ptr;

            while( token && *token ) {

                if( *token == ' ' || *token == '\t' ) {

                    token++;
                    continue;

                }

                if( aliases < &ProtoInfo->Proto.p_aliases[MAX_PROTO_ALIASES - 1] ) {

                    *aliases++ = token;

                }

                token = GetProtoPatternMatch( token, " \t" );

                if( token != NULL ) {

                    *token++ = '\0';

                }

            }

        }

        *aliases = NULL;
        result = &ProtoInfo->Proto;
        break;

    }

    return result;

}   // GetProtoGetNextEnt

