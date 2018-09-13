/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ttsertl.c

Abstract:

    Kernel mode test of security rtl routines.



Author:

    Jim Kelly       (JimK)     23-Mar-1990

Environment:

    Test of security.

Revision History:

--*/

#ifndef _SERTL_DEBUG_
#define _SERTL_DEBUG_
#endif

#define _TST_KERNEL_ //Kernel mode test

#include <stdio.h>

#include "sep.h"

#include <zwapi.h>

#include "tsevars.c"    // Common test variables

#include "ctsertl.c"    // Common RTL test routines


BOOLEAN SeRtlTest();

int
main(
    int argc,
    char *argv[]
    )
{
    VOID KiSystemStartup();

    TestFunction = SeRtlTest;
    KiSystemStartup();
    return( 0 );
}


BOOLEAN
SeRtlTest()
{

    BOOLEAN Result;

    DbgPrint("Se: Start Kernel Mode RTL Test...\n");

    Result = TestSeRtl();

    if (!Result) {
        DbgPrint("Se: ** Kernel Mode RTL Test Failed **\n");
    }
    DbgPrint("Se: End Kernel Mode RTL Test.\n");
    return Result;
}
