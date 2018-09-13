/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    utaccess.c

Abstract:

    Security component user-mode test.

Author:

    Robert Reichel (RobertRe) 14-Dec-90

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define _TST_USER_  // User mode test


#include "tsevars.c"    // Common test variables

#include "ctaccess.c"     // Common accessibility test routines



BOOLEAN
Test()
{
    BOOLEAN Result = TRUE;

    DbgPrint("Se: Start User Mode Access Test...\n");

    Result = CTAccess();

    DbgPrint("Se: End User Mode Access Test.\n");

    return Result;
}

BOOLEAN
main()
{
    return Test();
}
