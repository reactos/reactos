/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Dispmsg.c

Abstract:

    This module contains the DisplayKey function which is part of the
    Configuration Registry Tools (CRTools) library.

Author:

    David J. Gilman (davegi) 08-Jan-1992

Environment:

    Windows, Crt - User Mode

--*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "crtools.h"

VOID
DisplayMessage(
    IN BOOL Terminate,
    IN PSTR Format,
    IN ...
    )

/*++

Routine Description:

    Displays a message on the standard error stream and optionally
    terminates the program.

Arguments:

    Terminate - Supplies a flag which if TRUE causes DisplayMessage to
        terminate the program.

    Format - Supplies a printf style format string.

    ... - Supplies optional arguments, one for each format specifier in
        Format.

Return Value:

    None.

--*/

{
    va_list marker;

    ASSERT( ARGUMENT_PRESENT( Format ));

    va_start( marker, Format );

    vfprintf( stderr, Format, marker );

    va_end( marker );

    if( Terminate ) {
        exit( -1 );
    }
}
