
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    utsertl.c

Abstract:

    Security component user-mode test.
    Test security RTL routines from user mode.

Author:

    Jim Kelly (JimK) 13-Apr-1990

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define _TST_USER_      // User mode test

#include "tsevars.c"    // Common test variables

#include "ctsertl.c"    // Common RTL test routines


BOOLEAN
turtl()
{
    BOOLEAN Result;

    DbgPrint("Se: Start User Mode RTL Test...\n");

    Result = TestSeRtl();

    if (!Result) {
        DbgPrint("Se: ** User Mode RTL Test Failed **\n");
    }
    DbgPrint("Se: End User Mode RTL Test.\n");
    return Result;
}

NTSTATUS
__cdecl
main()
{
    turtl();

    return STATUS_SUCCESS;
}
