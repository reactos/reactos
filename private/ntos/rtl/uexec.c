/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    uexec.c

Abstract:

    Test program for the NT OS User Mode Runtime Library (URTL)

Author:

    Steve Wood (stevewo) 18-Aug-1989

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <string.h>

PVOID MyHeap = NULL;

NTSTATUS
main(
    IN ULONG argc,
    IN PCH argv[],
    IN PCH envp[],
    IN ULONG DebugParameter OPTIONAL
    )
{
    NTSTATUS Status;
    STRING ImagePathName;
    PCHAR PathVariable, *pp;
    CHAR ImageNameBuffer[ 128 ];
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    ULONG i, CountBytes, envc, Bogus;
    PSTRING DstString;
    PCH Src, Dst;
    PCH Parameters[ RTL_USER_PROC_PARAMS_DEBUGFLAG+2 ];

    Parameters[ RTL_USER_PROC_PARAMS_IMAGEFILE ] =
         "Full Path Specification of Image File goes here";

    Parameters[ RTL_USER_PROC_PARAMS_CMDLINE ] =
         "Complete Command Line goes here";

    Parameters[ RTL_USER_PROC_PARAMS_DEBUGFLAG ] =
         "Debugging String goes here";

    Parameters[ RTL_USER_PROC_PARAMS_DEBUGFLAG+1 ] = NULL;

    MyHeap = RtlProcessHeap();

#if DBG
    DbgPrint( "Entering UEXEC User Mode Test Program\n" );
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

#endif

    PathVariable = "\\SystemRoot";
    if (envp != NULL) {
        pp = envp;
        while (Src = *pp++) {
            if (!_strnicmp( Src, "PATH=", 5 )) {
                PathVariable = Src+5;
                break;
                }
            }
        }

    DbgPrint( "PATH=%s\n", PathVariable );

    ProcessParameters = (PRTL_USER_PROCESS_PARAMETERS)
                            RtlAllocateHeap( MyHeap, 0, 2048 );
    ProcessParameters->MaximumLength = 2048;

    argv[ argc ] = NULL;
    Status = RtlVectorsToProcessParameters(
                argv,
                envp,
                Parameters,
                ProcessParameters
                );

    ImagePathName.Buffer = ImageNameBuffer;
    ImagePathName.Length = 0;
    ImagePathName.MaximumLength = sizeof( ImageNameBuffer );
    if (RtlSearchPath( PathVariable, "uexec1.exe", NULL, &ImagePathName )) {
        Status = RtlCreateUserProcess( &ImagePathName,
                                       NULL,
                                       NULL,
                                       NULL,
                                       TRUE,
                                       NULL,
                                       NULL,
                                       ProcessParameters,
                                       &ProcessInformation,
                                       NULL
                                     );
        if (NT_SUCCESS( Status )) {
            Status = NtResumeThread( ProcessInformation.Thread, &Bogus );
            if (NT_SUCCESS( Status )) {
#if DBG
                DbgPrint( "UEXEC waiting for UEXEC1...\n" );
#endif
                Status = NtWaitForSingleObject( ProcessInformation.Process,
                                                TRUE,
                                                NULL
                                              );
                }
            }
        }
    else {
        DbgPrint( "UEXEC1.EXE not found in %s\n", PathVariable );
        Status = STATUS_UNSUCCESSFUL;
        }

#if DBG
    DbgPrint( "Leaving UEXEC User Mode Test Program\n" );
#endif

    return( Status );
}
