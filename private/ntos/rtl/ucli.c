/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ucli.c

Abstract:

    Test program for the NT OS User Mode Runtime Library (URTL)

Author:

    Steve Wood (stevewo) 18-Aug-1989

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsm.h>

#include <string.h>

NTSTATUS
main(
    IN int argc,
    IN char *argv[],
    IN char *envp[],
    IN ULONG DebugParameter OPTIONAL
    )
{
    PCH InitialCommandLine = NULL;
    CHAR Buffer[ 256 ];

    if (argc-- > 1) {
        InitialCommandLine = Buffer;
        *Buffer = '\0';
        while (argc--) {
            strcat( Buffer, *++argv );
            strcat( Buffer, " " );
            }
        }

    RtlCommandLineInterpreter( "UCLI> ", envp, InitialCommandLine );

    return( STATUS_SUCCESS );
}
