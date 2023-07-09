/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Assert.c

Abstract:

    This module contains the CruAssert function which is part of the
    Configuration Registry Tools (CRTools) library.

Author:

    David J. Gilman (davegi) 02-Jan-1992

Environment:

    Windows, Crt - User Mode

--*/

#include <conio.h>
#include <stdio.h>

#include "crtools.h"

VOID
CrAssert(
    IN PSTR FailedAssertion,
    IN PSTR FileName,
    IN DWORD LineNumber,
    IN PSTR Message OPTIONAL
    )

/*++

Routine Description:

    Display (on stderr) a string representing the failed assertion. Then
    prompt the user for the appropriate action. An optional message can
    also be displayed.

Arguments:

    FailedAssertion - Supplies the string representing the failed
        assertion.

    FileName - Supplies the string containing the file name of the failed
        assertion.

    LineNumber - Supplies the line number within the file that contains
        the failed assertion.

    Message - Supplies an optional message to be displayed along with the
        failed assertion.

Return Value:

    None.

--*/


{
    int Response;

    while( 1 ) {

        fprintf( stderr,
            "\n*** Assertion failed: %s %s\n***"
            "   Source File: %s, line %ld\n\n",
              Message ? Message : "",
              FailedAssertion,
              FileName,
              LineNumber
            );

        fprintf( stderr,
            "Break, Ignore, Exit Process or Exit Thread (bipt)? "
            );

        Response = getche( );
        fprintf( stderr, "\n\n" );

        switch( Response ) {

            case 'B':
            case 'b':
                DebugBreak( );
                break;

            case 'I':
            case 'i':
                return;

            case 'P':
            case 'p':
                ExitProcess( -1 );
                break;

            case 'T':
            case 't':
                ExitThread( -1 );
                break;
        }
    }
}
