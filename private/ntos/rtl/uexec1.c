/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tUEXEC1.c

Abstract:

    Sub-Test program for the NT OS User Mode Runtime Library (URTL)

Author:

    Steve Wood (stevewo) 18-Aug-1989

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

NTSTATUS
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    ULONG i;

    DbgPrint( "Entering UEXEC1 User Mode Test Program\n" );
    DbgPrint( "argc = %ld\n", argc );
    for (i=0; i<=argc; i++) {
        DbgPrint( "argv[ %ld ]: %s\n",
                  i,
                  argv[ i ] ? argv[ i ] : "<NULL>"
                );
        }
    DbgPrint( "\n" );
    for (i=0; envp[i]; i++) {
        DbgPrint( "envp[ %ld ]: %s\n", i, envp[ i ] );
        }

    DbgPrint( "Leaving UEXEC1 User Mode Test Program\n" );

    return( STATUS_SUCCESS );
}
