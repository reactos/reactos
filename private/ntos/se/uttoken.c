/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    uttoken.c

Abstract:

    Security component user-mode test.

    Token Object test from user mode.

Author:

    Jim Kelly (JimK) 27-June-1990

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define _TST_USER_  // User mode test


#include "tsevars.c"    // Common test variables

#include "cttoken.c"     // Common accessibility test routines



BOOLEAN
Test()
{

    BOOLEAN Result = TRUE;

    DbgPrint("Se: Start User Mode Token Object Test...\n");

    Result = CTToken();

    DbgPrint("Se: End User Mode Token Object Test.\n");

    return Result;
}


BOOLEAN
__cdecl
main()
{
    return Test();
}
