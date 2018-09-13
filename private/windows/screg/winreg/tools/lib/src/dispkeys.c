/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Dispkeys.c

Abstract:

    This module contains the DisplayKeys function which is part of the
    Configuration Registry Tools (CRTools) library.

Author:

    David J. Gilman (davegi) 08-Jan-1992

Environment:

    Windows, Crt - User Mode

--*/

#include <stdlib.h>
#include "crtools.h"

VOID
DisplayKeys(
    IN PKEY Key,
    IN BOOL Values,
    IN BOOL Data,
    IN BOOL Recurse
    )

/*++

Routine Description:

    Display (on stdout) information about a Key, and optionally about
    its/there values, data and decendants.  Decendants are displayed using
    a deapth first traversal.

Arguments:

    Key - Supplies a pointer to a KEY structure which contains the
        HKEY and sub key name to display.

    Values - Supplies a flag which if TRUE causes all of the Key's values
        to be displayed.

    Data - Supplies a flag which if TRUE causes all of the Key's data
        to be displayed.

    Recurse - Supplies a flag which if TRUE causes all of the Key's sub-keys
        to be displayed.

Return Value:

    None.

--*/

{
    LONG        Error;
    PKEY        ChildKey;
    PSTR        ChildSubKeyName;
    DWORD       ChildSubKeyNameLength;
    DWORD       NumberOfSubKeys;

    ASSERT( ARGUMENT_PRESENT( Key ));

    //
    // Display the key.
    //

    DisplayKey( Key, Values, Data );

    //
    // If requested display all of the Key's children, their children etc.
    //

    if( Recurse ) {

        //
        // Allocate space for the largest sub-key name.
        //

        ChildSubKeyName = ( PSTR ) malloc( Key->MaxSubKeyNameLength + 1 );
        if( ! ChildSubKeyName ) {

            ASSERT_MESSAGE( FALSE, "ChildSubKeyName allocated - " );
            return;
        }

        //
        // For each immediate child key, retrieve its name, create a KEY
        // object and recursively call DisplayKeys.
        //

        for(
            NumberOfSubKeys = 0;
            NumberOfSubKeys < Key->NumberOfSubKeys;
            NumberOfSubKeys++ ) {

            ChildSubKeyNameLength = Key->MaxSubKeyNameLength + 1;

            //
            // Retrieve the child's name.
            //

            Error = RegEnumKey(
                        Key->KeyHandle,
                        NumberOfSubKeys,
                        ChildSubKeyName,
                        ChildSubKeyNameLength
                        );

            if( Error != ERROR_SUCCESS ) {

                ASSERT_MESSAGE( FALSE, "RegEnumKey suceeded - " );
                return;
            }

            //
            // Allocate a KEY object.
            //

            ChildKey = AllocateKey( NULL, Key, ChildSubKeyName );

            if( ! ChildKey ) {

                ASSERT_MESSAGE( FALSE, "AllocateKey suceeded - " );
                return;
            }

            DisplayKeys( ChildKey, Values, Data, Recurse );
            FreeKey( ChildKey );
        }

        //
        // Release the child name buffer.
        //

        free( ChildSubKeyName );
    }
}
