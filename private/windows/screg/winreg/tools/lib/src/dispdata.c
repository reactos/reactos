/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Dispdata.c

Abstract:

    This module contains the DisplayData function which is part of the
    Configuration Registry Tools (CRTools) library.

Author:

    David J. Gilman (davegi) 02-Jan-1992

Environment:

    Windows, Crt - User Mode

--*/

#include <ctype.h>
#include <stdio.h>

#include "crtools.h"

VOID
DisplayData(
    IN PBYTE ValueData,
    IN DWORD ValueDataLength
    )

/*++

Routine Description:

    Display (on stdout) the supplied data in hex and ascii formats, in
    16 byte chunks.

Arguments:

    ValueData - Supplies a pointer to the data to display.

    ValueDataLength - Supplies the number of bytes of data to display.

Return Value:

    None.

--*/

{
    DWORD       DataIndex;
    DWORD       DataIndex2;
    WORD        SeperatorChars;

    ASSERT( ARGUMENT_PRESENT( ValueData ));

    //
    // DataIndex2 tracks multiples of 16.
    //

    DataIndex2 = 0;

    //
    // Display label.
    //

    printf( "Data:\n\n" );

    //
    // Display rows of 16 bytes of data.
    //

    for(
        DataIndex = 0;
        DataIndex < ( ValueDataLength >> 4 );
        DataIndex++,
        DataIndex2 = DataIndex << 4 ) {

        printf( "%08x   "
                "%02x %02x %02x %02x %02x %02x %02x %02x - "
                "%02x %02x %02x %02x %02x %02x %02x %02x  "
                "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
                DataIndex2,
                ValueData[ DataIndex2 + 0  ],
                ValueData[ DataIndex2 + 1  ],
                ValueData[ DataIndex2 + 2  ],
                ValueData[ DataIndex2 + 3  ],
                ValueData[ DataIndex2 + 4  ],
                ValueData[ DataIndex2 + 5  ],
                ValueData[ DataIndex2 + 6  ],
                ValueData[ DataIndex2 + 7  ],
                ValueData[ DataIndex2 + 8  ],
                ValueData[ DataIndex2 + 9  ],
                ValueData[ DataIndex2 + 10 ],
                ValueData[ DataIndex2 + 11 ],
                ValueData[ DataIndex2 + 12 ],
                ValueData[ DataIndex2 + 13 ],
                ValueData[ DataIndex2 + 14 ],
                ValueData[ DataIndex2 + 15 ],
                isprint( ValueData[ DataIndex2 + 0  ] )
                    ? ValueData[ DataIndex2 + 0  ]  : '.',
                isprint( ValueData[ DataIndex2 + 1  ] )
                    ? ValueData[ DataIndex2 + 1  ]  : '.',
                isprint( ValueData[ DataIndex2 + 2  ] )
                    ? ValueData[ DataIndex2 + 2  ]  : '.',
                isprint( ValueData[ DataIndex2 + 3  ] )
                    ? ValueData[ DataIndex2 + 3  ]  : '.',
                isprint( ValueData[ DataIndex2 + 4  ] )
                    ? ValueData[ DataIndex2 + 4  ]  : '.',
                isprint( ValueData[ DataIndex2 + 5  ] )
                    ? ValueData[ DataIndex2 + 5  ]  : '.',
                isprint( ValueData[ DataIndex2 + 6  ] )
                    ? ValueData[ DataIndex2 + 6  ]  : '.',
                isprint( ValueData[ DataIndex2 + 7  ] )
                    ? ValueData[ DataIndex2 + 7  ]  : '.',
                isprint( ValueData[ DataIndex2 + 8  ] )
                    ? ValueData[ DataIndex2 + 8  ]  : '.',
                isprint( ValueData[ DataIndex2 + 9  ] )
                    ? ValueData[ DataIndex2 + 9  ]  : '.',
                isprint( ValueData[ DataIndex2 + 10 ] )
                    ? ValueData[ DataIndex2 + 10 ]  : '.',
                isprint( ValueData[ DataIndex2 + 11 ] )
                    ? ValueData[ DataIndex2 + 11 ]  : '.',
                isprint( ValueData[ DataIndex2 + 12 ] )
                    ? ValueData[ DataIndex2 + 12 ]  : '.',
                isprint( ValueData[ DataIndex2 + 13 ] )
                    ? ValueData[ DataIndex2 + 13 ]  : '.',
                isprint( ValueData[ DataIndex2 + 14 ] )
                    ? ValueData[ DataIndex2 + 14 ]  : '.',
                isprint( ValueData[ DataIndex2 + 15 ] )
                    ? ValueData[ DataIndex2 + 15 ]  : '.'
                );
    }

    //
    // If the ValueDataLength is not an even multiple of 16
    // then there is one additonal line of data to display.
    //

    if( ValueDataLength % 16 != 0 ) {

        //
        // No seperator characters displayed so far.
        //

        SeperatorChars = 0;

        printf( "%08x   ", DataIndex << 4 );

        //
        // Display the remaining data, one byte at a time in hex.
        //

        for(
            DataIndex = DataIndex2;
            DataIndex < ValueDataLength;
            DataIndex++ ) {

            printf( "%02x ", ValueData[ DataIndex ] );

            //
            // If eight data values have been displayed, print
            // the seperator.
            //

            if( DataIndex % 8 == 7 ) {

                printf( "- " );

                //
                // Remember that two seperator characters were
                // displayed.
                //

                SeperatorChars = 2;
            }
        }

        //
        // Fill with blanks to the printable characters position.
        // That is position 63 less 8 spaces for the 'address',
        // 3 blanks, 3 spaces for each value displayed, possibly
        // two for the seperator plus two blanks at the end.
        //

        printf( "%*c",
                64
                - ( 8 + 3
                + (( DataIndex % 16 ) * 3 )
                + SeperatorChars
                + 2 ), ' ' );

        //
        // Display the remaining data, one byte at a time as
        // printable characters.
        //

        for(
            DataIndex = DataIndex2;
            DataIndex < ValueDataLength;
            DataIndex++ ) {

            printf( "%c",
                isprint( ValueData[ DataIndex ] )
                    ? ValueData[ DataIndex ] : '.'
                );

        }
        printf( "\n" );
    }
}
