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

Notes:

    Following is a sample display of a key:


----------------------------------

Key Name:        blech
Class Name:      foobar
Title Index:     12345678
Last Write Time: Wed 08-Jan-1992 11:19:43

Value 1
  Name:          bar
  Title Index:   12345678
  Type:          Double Word (2)
  Data:

00000000   42 55 49 4c 44 3a 20 43 - 6f 6d 70 69 6c 69 6e 67  BUILD: Compiling
00000010   20 64 3a 5c 6e 74 5c 70 - 72 69 76 61 74 65 5c 77   d:\nt\private\w
00000020   69 62 0a 0a 53 74 6f 70 - 2e 20 0a                 ib..Stop. .

Value 2
  Name:          foo
  Title Index:   12345678
  Type:          String (1)
  Data:

00000000   42 55 49 4c 44 3a 20 43 - 6f 6d 70 69 6c 69 6e 67  BUILD: Compiling
00000010   20 64 3a 5c 6e 74 5c 70 - 72 69 76 61 74 65 5c 77   d:\nt\private\w
00000020   69 62 0a 0a 53 74 6f 70 - 2e 20 0a                 ib..Stop. .

----------------------------------

    The first block ("Key Name:...11:19:43") is displayed by DisplayKey
    (dispkey.c).  The second block ("Value 1...Word (2)") is displayed by
    DisplayValues (disval.c) and the third block ("00000000...Stop. .")
    is displayed by DisplayData (dispdata.c).

--*/

#include <stdio.h>

#include "crtools.h"

VOID
DisplayKey(
    IN PKEY Key,
    IN BOOL Values,
    IN BOOL Data
    )

/*++

Routine Description:

    Display (on stdout) information about a Key and optionally about its
    values and data.

Arguments:

    Key - Supplies a pointer to a KEY structure which contains the
        HKEY and sub key name to display.

    Values - Supplies a flag which if TRUE causes all of the Key's values
        to be displayed.

    Data - Supplies a flag which if TRUE causes all of the Key's data
        to be displayed.

Return Value:

    None.

--*/

{
    ASSERT( ARGUMENT_PRESENT( Key ));

    DisplayKeyInformation( Key );

    if( Values ) {

        DisplayValues( Key, Data );
    }
}
