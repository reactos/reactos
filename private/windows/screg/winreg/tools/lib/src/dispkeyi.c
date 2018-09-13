/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Dispkeyi.c

Abstract:

    This module contains the DisplayKeyInformation function which is part
    of the Configuration Registry Tools (CRTools) library.

Author:

    David J. Gilman (davegi) 02-Jan-1992

Environment:

    Windows, Crt - User Mode

--*/

#include <stdio.h>
#include <stdlib.h>

#include "crtools.h"

VOID
DisplayKeyInformation(
    IN PKEY Key
    )

/*++

Routine Description:

    Display (on stdout) meta information about a Key.

Arguments:

    Key - Supplies a pointer to a KEY for which information is to be
        displayed.
Return Value:

    None.

--*/

{
    ASSERT( ARGUMENT_PRESENT( Key ));

    printf( "\n"
            "Key Name:          %s\n"
            "Class Name:        %s\n"
            // "Title Index:       %ld\n"
            "Last Write Time:   %s\n",
            Key->SubKeyFullName,
            ( Key->ClassName == NULL ) ? "<NONE>" : Key->ClassName,
            // Key->TitleIndex,
            FormatFileTime( &Key->LastWriteTime, NULL )
            );
}
