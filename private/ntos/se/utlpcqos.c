/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    utlpcqos.c

Abstract:

    Security component user-mode test.

    Security quality of service test for LPC from user mode.

    This test must be run from the SM> prompt in the debugger.

Author:

    Jim Kelly (JimK) 27-June-1990

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define _TST_USER_  // User mode test

#include "tsecomm.c"     // Common routines
#include "ctlpcqos.c"     // quality of service tests



BOOLEAN
Test()
{
    BOOLEAN Result = TRUE;


    Result = CtLpcQos();


    return Result;
}

BOOLEAN
main()
{
    return Test();
}
