/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ttseacc.c

Abstract:

    Security component kernel-mode test.

    Test Object Security manipulation and accessibility from kernel mode.

Author:

    Jim Kelly (JimK) 13-Apr-1990

Revision History:

--*/



#define _TST_KERNEL_    //Kernel mode test

#include <stdio.h>

#include "sep.h"

#include <zwapi.h>

#include "tsevars.c"    // Common test variables

#include "ctseacc.c"    // Common accessibility test routines



BOOLEAN
Test()
{
    BOOLEAN Result = TRUE;

    DbgPrint("Se: Start Kernel Mode Security Test...\n");

    Result = TSeAcc();

    DbgPrint("Se: End Kernel Mode Security Test.\n");

    return Result;
}

int
main(
    int argc,
    char *argv[]
    )
{
    VOID KiSystemStartup();

    TestFunction = Test;
    KiSystemStartup();
    return( 0 );
}
