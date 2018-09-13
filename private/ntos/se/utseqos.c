/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    utseqos.c

Abstract:

    Security component user-mode test.

    Security quality of service test from user mode.

Author:

    Jim Kelly (JimK) 27-June-1990

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define _TST_USER_  // User mode test

#include "tsecomm.c"     // Common routines
#include "ctseqos.c"     // quality of service tests



BOOLEAN
Test()
{
    BOOLEAN Result = TRUE;


    Result = CtSeQos();


    return Result;
}

BOOLEAN
main()
{
    return Test();
}
