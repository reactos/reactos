/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tmp.c

Abstract:

    Temporary (unnecessary) DLL entry point routine.


    The entry in this file is a bit of a hack.  The code isn't
    needed, but our linker doesn't know how to build a dll with data only.
    When MikeOl gets this fixed, we can remove this obligatory source
    file.


Author:

    Jim Kelly 24-Mar-1992

Revision History:

--*/


#include <nt.h>

BOOLEAN
MsAuditTDummyEntry( VOID )

/*++

Routine Description:

    This routine gets called when this DLL is loaded by the loader.
    It does nothing and wouldn't be needed if the linker worked
    correctly.

Arguments:

    None.


Return Value:

    None.

--*/
{

    return TRUE;
}
