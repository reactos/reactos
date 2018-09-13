/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    urtl.c

Abstract:

    Usermode test program for rtl

Author:

    Mark Lucovsky (markl) 22-Aug-1989

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

PVOID MyHeap = NULL;

DumpIt(
    IN PRTL_USER_PROCESS_PARAMETERS ArgBase
    )
{
    ULONG Base;
    PSTRING Vector;
    PCH *ParmVector;
    ULONG i;

    (VOID) RtlNormalizeProcessParameters( ArgBase );
    (VOID) RtlDeNormalizeProcessParameters( ArgBase );

    Base = (ULONG) ArgBase;

    DbgPrint("DumpIt: ArgBase       %lx\n",ArgBase);
    DbgPrint("DumpIt: MaximumLength %lx\n",ArgBase->MaximumLength);
    DbgPrint("DumpIt: Length        %lx\n",ArgBase->Length);
    DbgPrint("DumpIt: ArgumentCount %lx\n",ArgBase->ArgumentCount);
    DbgPrint("DumpIt: Arguments     %lx\n",ArgBase->Arguments    );
    DbgPrint("DumpIt: VariableCount %lx\n",ArgBase->VariableCount);
    DbgPrint("DumpIt: Variables     %lx\n",ArgBase->Variables    );
    DbgPrint("DumpIt: ParameterCount%lx\n",ArgBase->ParameterCount);
    DbgPrint("DumpIt: Parameters    %lx\n",ArgBase->Parameters    );

    if ( ArgBase->ArgumentCount ) {
        Vector = (PSTRING)((PCH)ArgBase->Arguments + Base);
        i = ArgBase->ArgumentCount;
        while(i--){
            DbgPrint("DumpIt: Argument %s\n",Vector->Buffer + Base);
            Vector++;
        }
    }

    if ( ArgBase->VariableCount ) {
        Vector = (PSTRING)((PCH)ArgBase->Variables + Base);
        i = ArgBase->VariableCount;
        while(i--){
            DbgPrint("DumpIt: Variable %s\n",Vector->Buffer + Base);
            Vector++;
        }
    }

    if ( ArgBase->ParameterCount ) {
        ParmVector = (PCH *)((PCH)ArgBase->Parameters + Base);
        i = ArgBase->ParameterCount;
        while(i--) {
            DbgPrint("DumpIt: Parameter %s\n",*ParmVector + Base);
            ParmVector++;
        }
    }
}

BOOLEAN
VectorTest(
    IN PCH Arguments[],
    IN PCH Variables[],
    IN PCH Parameters[]
    )
{

    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    NTSTATUS st;

    DbgPrint("VectorTest:++\n");

    ProcessParameters = RtlAllocateHeap(MyHeap, 0, 2048);
    ProcessParameters->MaximumLength = 2048;

    st = RtlVectorsToProcessParameters(
            Arguments,
            Variables,
            Parameters,
            ProcessParameters
            );

    DumpIt(ProcessParameters);

    DbgPrint("VectorTest:--\n");

    return TRUE;
}

NTSTATUS
main(
    IN ULONG argc,
    IN PCH argv[],
    IN PCH envp[],
    IN ULONG DebugParameter OPTIONAL
    )

{
    ULONG i;
    char c, *s;
    PCH *Arguments;
    PCH *Variables;
    PCH Parameters[ RTL_USER_PROC_PARAMS_DEBUGFLAG+2 ];

    ULONG TestVector = 0;

    Arguments = argv;
    Variables = envp;
    Parameters[ RTL_USER_PROC_PARAMS_IMAGEFILE ] =
         "Full Path Specification of Image File goes here";

    Parameters[ RTL_USER_PROC_PARAMS_CMDLINE ] =
         "Complete Command Line goes here";

    Parameters[ RTL_USER_PROC_PARAMS_DEBUGFLAG ] =
         "Debugging String goes here";

    Parameters[ RTL_USER_PROC_PARAMS_DEBUGFLAG+1 ] = NULL;

    MyHeap = RtlProcessHeap();


#if DBG
    DbgPrint( "Entering URTL User Mode Test Program\n" );
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
    i = 1;
    if (argc > 1 ) {
        while (--argc) {
            s = *++argv;
            while ((c = *s++) != '\0') {
                switch (c) {

                case 'V':
                case 'v':
                    TestVector = i++;
                    break;
                default:
                    DbgPrint( "urtl: invalid test code - '%s'", *argv );
                    break;
                }
            }
        }
    }

    if ( TestVector ) {
        VectorTest(Arguments,Variables,Parameters);
    }

    return( STATUS_SUCCESS );
}
