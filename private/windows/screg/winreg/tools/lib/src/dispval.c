/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Dispkey.c

Abstract:

    This module contains the DisplayKey function which is part of the
    Configuration Registry Tools (CRTools) library.

Author:

    David J. Gilman (davegi) 02-Jan-1992

Environment:

    Windows, Crt - User Mode

--*/

#include <stdio.h>
#include <stdlib.h>

#include "crtools.h"

//
// Ensure that ValueTypeStrings table is correct.
//

//
// Maximum number of registry types.
//

#define NUMBER_OF_REG_TYPES     ( 9 )

#if (  (REG_NONE             != ( 0 )) \
    || (REG_SZ               != ( 1 )) \
    || (REG_EXPAND_SZ        != ( 2 )) \
    || (REG_BINARY           != ( 3 )) \
    || (REG_DWORD            != ( 4 )) \
    || (REG_DWORD_BIG_ENDIAN != ( 5 )) \
    || (REG_LINK             != ( 6 )) \
    || (REG_MULTI_SZ         != ( 7 )) \
    || (REG_RESOURCE_LIST    != ( 8 )))



#error REG_* does not map ValueTypeStrings correctly.

#endif

VOID
DisplayValues(
    IN PKEY Key,
    IN BOOL Data
    )

/*++

Routine Description:

    Display (on stdout) information about a Key's values and optionally
    its data.

Arguments:

    KeyHandle - Supplies a HKEY for which meta information is to be
        displayed.

    Key - Supplies a pointer to a KEY structure where the meta information
        is stored.

    Data - Supplies a flag which if TRUE causes all of the Key's data
        to be displayed.

Return Value:

    BOOL

--*/

{
    static PSTR ValueTypeStrings[ ] =   {
                                            TEXT( "None" ),
                                            TEXT( "String" ),
                                            TEXT( "Binary" ),
                                            TEXT( "Double Word" ),
                                            TEXT( "Double Word (big endian)" ),
                                            TEXT( "Symbolic link" ),
                                            TEXT( "Multi-SZ" ),
                                            TEXT( "Resource list" ),
                                            TEXT( "Unknown" )
                                        };


    LONG        Error;
    DWORD       Index;

    PSTR        ValueName;
    DWORD       ValueNameLength;
    DWORD       ValueTitleIndex;
    DWORD       ValueType;
    PBYTE       ValueData;
    DWORD       ValueDataLength;

    ASSERT( ARGUMENT_PRESENT( Key ));

    // Attempt to allocate memory for the largest possible value name.
    //

    ValueName = ( PSTR ) malloc( Key->MaxValueNameLength + 1 );
    if( ValueName == NULL ) {

        ASSERT_MESSAGE( FALSE, "Value name memory allocation - " );
        return;
    }

    //
    // If data is requested, attempt to allocate memory for the largest amount
    // of data.
    //

    if( Data ) {

        ValueData = ( PBYTE ) malloc( Key->MaxValueDataLength + 1 );
        if( ValueData == NULL ) {

            ASSERT_MESSAGE( FALSE, "Value data memory allocation - " );
            return;
        }

    } else {

        ValueData = NULL;
    }

    //
    // For each value in the sub key, enumerate and display its
    // details.
    //

    for( Index = 0; Index < Key->NumberOfValues; Index++ ) {

        //
        // Can't use the Key->Max*Length variables as they will be
        // overwritten by the RegEnumValue API.
        //

        ValueNameLength = Key->MaxValueNameLength + 1;
        ValueDataLength = Key->MaxValueDataLength;

        //
        // Get the name, title index, type and data for the
        // current value.
        //

        Error = RegEnumValue(
                    Key->KeyHandle,
                    Index,
                    ValueName,
                    &ValueNameLength,
                    NULL,
                    &ValueType,
                    ValueData,
                    &ValueDataLength
                    );

        if( Error != ERROR_SUCCESS ) {
            ASSERT_MESSAGE( FALSE, "RegEnumValue - " );
            return;
        }

        //
        // Display the value information.
        //

        printf( "\n"
                "Value %d\n"
                "Name:              %.*s\n"
                // "Title Index:       %ld\n"
                "Type:              %s (%ld)\n"
                "Data Length:       %ld\n",
                Index + 1,
                ( ValueNameLength == 0 ) ? 80 : ValueNameLength,
                ( ValueNameLength == 0 ) ? "<NONE>" : ValueName,
                // ValueTitleIndex,
                ValueTypeStrings[( ValueType < NUMBER_OF_REG_TYPES )
                                ? ValueType
                                : NUMBER_OF_REG_TYPES ],
                ValueType,
                ValueDataLength
                );

        if( Data ) {

            ASSERT( ValueData != NULL );
            DisplayData( ValueData, ValueDataLength );
        }
    }

    //
    // Release the buffers.
    //

    free( ValueName );

    if( ValueData != NULL ) {

        free( ValueData );
    }
}
