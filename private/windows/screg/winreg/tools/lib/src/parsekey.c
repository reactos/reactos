/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Parsekey.c

Abstract:

    This module contains the ParseKey function which is part of the
    Configuration Registry Tools (CRTools) library.

Author:

    David J. Gilman (davegi) 02-Jan-1992

Environment:

    Windows, Crt - User Mode

--*/

#include <stdlib.h>
#include <string.h>

#include "crtools.h"

//
// PREDEFINED_HANDLE_ENTRYs are used to map strings on the command line
// to real predefined handles.
//

typedef struct _PREDEFINED_HANDLE_ENTRY
    PREDEFINED_HANDLE_ENTRY,
    *PPREDEFINED_HANDLE_ENTRY;

struct _PREDEFINED_HANDLE_ENTRY {
    PKEY        PredefinedKey;
    PSTR        PredefinedHandleName;
};

PKEY
ParseKey(
    IN PSTR SubKeyName
    )

/*++

Routine Description:

    Attempts to parse the supplied Key (string) into a predefined handle
    and a sub key.  If succesful it allocates and returns a KEY structure.

    The form of the supplied Key should be:

       \\machine_name\<predefined_key_name>\sub-key

    where "\\machine_name\" is optional.

Arguments:

    SubKeyName - Supplies the string which contains the key to parse.

Return Value:

    PKEY - Returns a pointer to a KEY structure if the supplied SubKeyName
        was succesfully parsed.

--*/


{

    static PREDEFINED_HANDLE_ENTRY      PredefinedHandleTable[ ] = {
                                &KeyClassesRoot,  HKEY_CLASSES_ROOT_STRING,
                                &KeyCurrentUser,  HKEY_CURRENT_USER_STRING,
                                &KeyLocalMachine, HKEY_LOCAL_MACHINE_STRING,
                                &KeyUsers,        HKEY_USERS_STRING
                            };
    PSTR    Token;
    PSTR    Name;
    PSTR    StrPtr;
    PSTR    MachineNamePtr;
    TSTR    MachineName[ MAX_PATH ];
    WORD    i;
    PKEY    ParsedKey;


    ASSERT( ARGUMENT_PRESENT( SubKeyName ));

    //
    // See if the SubKeyName contains a \\machine name.
    //

    if(( SubKeyName[ 0 ] == '\\' ) && ( SubKeyName[ 1 ] == '\\' )) {

        //
        // Find the end of the machine name.
        //

        StrPtr = strchr( &SubKeyName[ 2 ], '\\' );

        //
        // If the SubKeyName only contained a machine name, its invalid.
        //

        if( *StrPtr == '\0' ) {

            ASSERT_MESSAGE( FALSE, "SubKeyName - " );
            return NULL;

        } else {

            //
            // Copy and NUL terminate the machine name and bump over the '\'
            // that seperates the machine name from predefined handle.
            //

            strncpy( MachineName, SubKeyName, StrPtr - SubKeyName );
            MachineName[ StrPtr - SubKeyName ] = '\0';
            StrPtr++;
            MachineNamePtr = MachineName;
        }

    } else {

        //
        // There is no machine name so parse the string from the beginning.
        //

        StrPtr = SubKeyName;
        MachineNamePtr = NULL;
    }

    //
    // Get the predefined handle from the string (this may be at the
    // beginning of the string or after the machine name).
    //

    Token = strtok( StrPtr, "\\\0" );

    //
    // For each predefined handle, search the table to determine which
    // handle is being referenced.
    //

    for(
        i = 0;
        i < sizeof( PredefinedHandleTable ) / sizeof( PREDEFINED_HANDLE_ENTRY );
        i++ ) {

        //
        // See if the predefined handle name has been mapped in
        // the environment.
        //

        Name = getenv( PredefinedHandleTable[ i ].PredefinedHandleName );

        //
        // If it hasn't been mapped, use the default.
        //

        if( Name == NULL ) {

            Name = PredefinedHandleTable[ i ].PredefinedHandleName;
        }

        //
        // If the Token matches one of the prefined handle names, allocate
        // a KEY structure.
        //

        if( _stricmp( Name, Token ) == 0 ) {

            ParsedKey = AllocateKey(
                            MachineNamePtr,
                            PredefinedHandleTable[ i ].PredefinedKey,
                            strtok( NULL, "\0" )
                            );

            if( ParsedKey != NULL ) {

                return ParsedKey;

            } else {

                ASSERT_MESSAGE( FALSE, "AllocateKey - " );
                return NULL;
            }
        }
    }

    //
    // The supplied Key could not be parsed. That is the first token
    // was not one of the predefined handle names.
    //

    return NULL;
}
