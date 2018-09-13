/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    utseacc.c

Abstract:

    Security component user-mode test.

    Test Object Security manipulation and accessibility from user mode.

Author:

    Jim Kelly (JimK) 13-Apr-1990

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define _TST_USER_      // User mode test


#include "tsevars.c"    // Common test variables

#include "ctseacc.c"    // Common accessibility test routines



BOOLEAN
Test()
{
    BOOLEAN Result = TRUE;

    DbgPrint("Se: Start User Mode Security Test...\n");

    Result = TSeAcc();

    DbgPrint("Se: End User Mode Security Test.\n");

    return Result;
}

NTSTATUS
__cdecl
main()
{
    Test();

    return STATUS_SUCCESS;
}
