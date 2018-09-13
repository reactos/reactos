/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Freekey.c

Abstract:

    This module contains the FreeKey function which is part of the
    Configuration Registry Tools (CRTools) library.

Author:

    David J. Gilman (davegi) 02-Jan-1992

Environment:

    Windows, Crt - User Mode

--*/

#include <stdlib.h>

#include "crtools.h"

VOID
FreeKey(
    IN PKEY Key
    )

/*++

Routine Description:

    Frees all memory associated with the supplied Key.

Arguments:

    Key - Supplies a pointer to the KEY structure to be freed.

Return Value:

    None.

--*/

{
    LONG    Error;

    ASSERT_IS_KEY( Key );

    //
    // Don't free/closee predefined handles.
    //

    if(
            ( Key->KeyHandle == HKEY_CLASSES_ROOT )
        ||  ( Key->KeyHandle == HKEY_CURRENT_USER )
        ||  ( Key->KeyHandle == HKEY_LOCAL_MACHINE )
        ||  ( Key->KeyHandle == HKEY_USERS )) {

        return;

    }

    Error = RegCloseKey( Key->KeyHandle );
    ASSERT( Error == ERROR_SUCCESS );

    free( Key->ClassName );
    free( Key->SubKeyName );
    free( Key->SubKeyFullName );
    free( Key );
}
