/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    udbg.c

Abstract:

    Usermode test for debugger

Author:

    Mark Lucovsky (markl) 19-Jan-1990

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdbg.h>

HANDLE DebugPort;


NTSTATUS
ThreadThatExits (
    IN PVOID ThreadParameter
    )
{
    NtTerminateThread(NtCurrentThread(),(NTSTATUS) ThreadParameter );
}

ULONG
foo(PULONG l)
{
    //ULONG x;
    //x = *l;
    //return x + 1;

    return *l;

}

NTSTATUS
ThreadThatExcepts (
    IN PVOID ThreadParameter
    )
{
    foo((PULONG)0x00000001);
    NtTerminateThread(NtCurrentThread(),(NTSTATUS) ThreadParameter );
}



NTSTATUS
ThreadThatSpins (
    IN PVOID ThreadParameter
    )
{
    for(;;);
    NtTerminateThread(NtCurrentThread(),STATUS_SUCCESS);
}


UdbgTest1()
{
    NTSTATUS st;
    HANDLE ExitThread, SpinThread, DebugProcess;
    CLIENT_ID ExitClientId, SpinClientId;
    DBGKM_APIMSG m;
    PDBGKM_CREATE_THREAD CreateThreadArgs;
    PDBGKM_CREATE_PROCESS CreateProcessArgs;
    PDBGKM_EXIT_THREAD ExitThreadArgs;
    PDBGKM_EXIT_PROCESS ExitProcessArgs;
    ULONG Psp;

    DbgPrint("UdbgTest1: (1)...\n");

        //
        // Verify that a process can be created with a debug
        // port.
        //

        st = NtCreateProcess(
                &DebugProcess,
                PROCESS_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                FALSE,
                NULL,
                DebugPort,
                NULL
                );
        ASSERT(NT_SUCCESS(st));

        st = RtlCreateUserThread(
                DebugProcess,
                NULL,
                TRUE,
                0L,
                0L,
                0L,
                ThreadThatExits,
                (PVOID) STATUS_ABANDONED,
                &ExitThread,
                &ExitClientId
                );
        ASSERT(NT_SUCCESS(st));

        st = RtlCreateUserThread(
                DebugProcess,
                NULL,
                TRUE,
                0L,
                0L,
                0L,
                ThreadThatSpins,
                NULL,
                &SpinThread,
                &SpinClientId
                );
        ASSERT(NT_SUCCESS(st));

    DbgPrint("UdbgTest1: (2)...\n");

        //
        // Verify that CreateProcess Messages Arrive, and that
        // they are correct
        //

        st = NtResumeThread(SpinThread,NULL);
        ASSERT(NT_SUCCESS(st));

        st = NtReplyWaitReceivePort(
                DebugPort,
                NULL,
                NULL,
                (PPORT_MESSAGE)&m
                );
        ASSERT(NT_SUCCESS(st));
        ASSERT(m.ApiNumber == DbgKmCreateProcessApi);

        CreateThreadArgs = &m.u.CreateProcess.InitialThread;
        CreateProcessArgs = &m.u.CreateProcess;
        ASSERT( CreateThreadArgs->SubSystemKey == 0 && CreateThreadArgs->StartAddress == (PVOID)ThreadThatSpins );
        ASSERT( CreateProcessArgs->SubSystemKey == 0);

    DbgPrint("UdbgTest1: (3)...\n");

        //
        // Verify that other threads in the process are properly suspended
        //

        st = NtSuspendThread(ExitThread,&Psp);
        ASSERT(NT_SUCCESS(st) && Psp == 2);

        st = NtResumeThread(ExitThread,&Psp);
        ASSERT(NT_SUCCESS(st) && Psp == 3);

        st = NtReplyPort(DebugPort,(PPORT_MESSAGE)&m);
        ASSERT(NT_SUCCESS(st));


    DbgPrint("UdbgTest1: (4)...\n");

        //
        // Verify that CreateThread Messages Arrive, and that
        // they are correct
        //

        st = NtResumeThread(ExitThread,&Psp);
        ASSERT(NT_SUCCESS(st));

        st = NtReplyWaitReceivePort(
                DebugPort,
                NULL,
                NULL,
                (PPORT_MESSAGE)&m
                );
        ASSERT(NT_SUCCESS(st));
        ASSERT(m.ApiNumber == DbgKmCreateThreadApi);

        CreateThreadArgs = &m.u.CreateThread;
        ASSERT( CreateThreadArgs->SubSystemKey == 0 && CreateThreadArgs->StartAddress == (PVOID)ThreadThatExits );

        st = NtReplyPort(DebugPort,(PPORT_MESSAGE)&m);
        ASSERT(NT_SUCCESS(st));

    DbgPrint("UdbgTest1: (5)...\n");

        //
        // Verify that ExitThread Messages Arrive, and that
        // they are correct
        //

        st = NtReplyWaitReceivePort(
                DebugPort,
                NULL,
                NULL,
                (PPORT_MESSAGE)&m
                );
        ASSERT(NT_SUCCESS(st));
        ASSERT(m.ApiNumber == DbgKmExitThreadApi);

        ExitThreadArgs = &m.u.ExitThread;
        ASSERT( ExitThreadArgs->ExitStatus == STATUS_ABANDONED );

        st = NtReplyPort(DebugPort,(PPORT_MESSAGE)&m);
        ASSERT(NT_SUCCESS(st));

        st = NtWaitForSingleObject(ExitThread,FALSE,NULL);
        ASSERT(NT_SUCCESS(st));

    DbgPrint("UdbgTest1: (6)...\n");

        //
        // Verify that ExitThread Messages Arrive, and that
        // they are correct
        //

        st = NtTerminateProcess(DebugProcess,STATUS_REPARSE);
        ASSERT(NT_SUCCESS(st));

        st = NtReplyWaitReceivePort(
                DebugPort,
                NULL,
                NULL,
                (PPORT_MESSAGE)&m
                );
        ASSERT(NT_SUCCESS(st));
        ASSERT(m.ApiNumber == DbgKmExitThreadApi);

        ExitThreadArgs = &m.u.ExitThread;
        ASSERT( ExitThreadArgs->ExitStatus == STATUS_REPARSE );

        st = NtReplyPort(DebugPort,(PPORT_MESSAGE)&m);
        ASSERT(NT_SUCCESS(st));

    DbgPrint("UdbgTest1: (7)...\n");

        //
        // Verify that ExitProcess Messages Arrive, and that
        // they are correct
        //

        st = NtReplyWaitReceivePort(
                DebugPort,
                NULL,
                NULL,
                (PPORT_MESSAGE)&m
                );
        ASSERT(NT_SUCCESS(st));
        ASSERT(m.ApiNumber == DbgKmExitProcessApi);

        ExitProcessArgs = &m.u.ExitProcess;
        ASSERT( ExitProcessArgs->ExitStatus == STATUS_REPARSE );

        st = NtReplyPort(DebugPort,(PPORT_MESSAGE)&m);
        ASSERT(NT_SUCCESS(st));


        st = NtWaitForSingleObject(ExitThread,FALSE,NULL);
        ASSERT(NT_SUCCESS(st));

        st = NtWaitForSingleObject(DebugProcess,FALSE,NULL);
        ASSERT(NT_SUCCESS(st));

    NtClose(ExitThread);
    NtClose(SpinThread);
    NtClose(DebugProcess);

    DbgPrint("UdbgTest1: END OF TEST ***\n");

}

UdbgTest2()
{
    NTSTATUS st;
    HANDLE ExceptionThread, DebugProcess;
    DBGKM_APIMSG m;
    PDBGKM_CREATE_THREAD CreateThreadArgs;
    PDBGKM_CREATE_PROCESS CreateProcessArgs;
    PDBGKM_EXIT_THREAD ExitThreadArgs;
    PDBGKM_EXIT_PROCESS ExitProcessArgs;
    PDBGKM_EXCEPTION ExceptionArgs;
    ULONG Psp;

    DbgPrint("UdbgTest2: (1)...\n");

        //
        // Verify that a process can be created with a debug
        // port.
        //

        st = NtCreateProcess(
                &DebugProcess,
                PROCESS_ALL_ACCESS,
                NULL,
                NtCurrentProcess(),
                FALSE,
                NULL,
                DebugPort,
                NULL
                );
        ASSERT(NT_SUCCESS(st));

        st = RtlCreateUserThread(
                DebugProcess,
                NULL,
                TRUE,
                0L,
                0L,
                0L,
                ThreadThatExcepts,
                (PVOID) STATUS_ABANDONED,
                &ExceptionThread,
                NULL
                );
        ASSERT(NT_SUCCESS(st));

    DbgPrint("UdbgTest2: (2)...\n");

        //
        // Verify that CreateThread Messages Arrive, and that
        // they are correct
        //

        st = NtResumeThread(ExceptionThread,NULL);
        ASSERT(NT_SUCCESS(st));

        st = NtReplyWaitReceivePort(
                DebugPort,
                NULL,
                NULL,
                (PPORT_MESSAGE)&m
                );
        ASSERT(NT_SUCCESS(st));
        ASSERT(m.ApiNumber == DbgKmCreateProcessApi);

        CreateThreadArgs = &m.u.CreateProcess.InitialThread;
        CreateProcessArgs = &m.u.CreateProcess;
        ASSERT( CreateThreadArgs->SubSystemKey == 0 && CreateThreadArgs->StartAddress == (PVOID)ThreadThatExcepts );
        ASSERT( CreateProcessArgs->SubSystemKey == 0);

        st = NtReplyPort(DebugPort,(PPORT_MESSAGE)&m);
        ASSERT(NT_SUCCESS(st));

    DbgPrint("UdbgTest2: (3)...\n");

        //
        // Verify that First Chance Exception Messages Arrive, and that
        // they are correct
        //

        st = NtReplyWaitReceivePort(
                DebugPort,
                NULL,
                NULL,
                (PPORT_MESSAGE)&m
                );
        ASSERT(NT_SUCCESS(st));
        ASSERT(m.ApiNumber == DbgKmExceptionApi);

        ExceptionArgs = &m.u.Exception;
        ASSERT( ExceptionArgs->FirstChance == TRUE );

        m.ReturnedStatus = DBG_EXCEPTION_NOT_HANDLED;

        st = NtReplyPort(DebugPort,(PPORT_MESSAGE)&m);
        ASSERT(NT_SUCCESS(st));

    DbgPrint("UdbgTest2: (4)...\n");

        //
        // Verify that First Chance Exception Messages Arrive, and that
        // they are correct
        //

        st = NtReplyWaitReceivePort(
                DebugPort,
                NULL,
                NULL,
                (PPORT_MESSAGE)&m
                );
        ASSERT(NT_SUCCESS(st));
        ASSERT(m.ApiNumber == DbgKmExceptionApi);

        ExceptionArgs = &m.u.Exception;
        ASSERT( ExceptionArgs->FirstChance == FALSE );

        m.ReturnedStatus = DBG_EXCEPTION_HANDLED;
skip4:
        st = NtTerminateProcess(DebugProcess,STATUS_REPARSE);
        ASSERT(NT_SUCCESS(st));

        st = NtReplyPort(DebugPort,(PPORT_MESSAGE)&m);
        ASSERT(NT_SUCCESS(st));

        st = NtReplyWaitReceivePort(
                DebugPort,
                NULL,
                NULL,
                (PPORT_MESSAGE)&m
                );
        ASSERT(NT_SUCCESS(st));
        ASSERT(m.ApiNumber == DbgKmExitThreadApi);

        ExitThreadArgs = &m.u.ExitThread;
        ASSERT( ExitThreadArgs->ExitStatus == STATUS_REPARSE );

        st = NtReplyPort(DebugPort,(PPORT_MESSAGE)&m);
        ASSERT(NT_SUCCESS(st));

    DbgPrint("UdbgTest2: (5)...\n");

        //
        // Verify that ExitProcess Messages Arrive, and that
        // they are correct
        //

        st = NtReplyWaitReceivePort(
                DebugPort,
                NULL,
                NULL,
                (PPORT_MESSAGE)&m
                );
        ASSERT(NT_SUCCESS(st));
        ASSERT(m.ApiNumber == DbgKmExitProcessApi);

        ExitProcessArgs = &m.u.ExitProcess;
        ASSERT( ExitProcessArgs->ExitStatus == STATUS_REPARSE );

        st = NtReplyPort(DebugPort,(PPORT_MESSAGE)&m);
        ASSERT(NT_SUCCESS(st));


        st = NtWaitForSingleObject(ExceptionThread,FALSE,NULL);
        ASSERT(NT_SUCCESS(st));

        st = NtWaitForSingleObject(DebugProcess,FALSE,NULL);
        ASSERT(NT_SUCCESS(st));

    NtClose(ExceptionThread);
    NtClose(DebugProcess);

    DbgPrint("UdbgTest2: END OF TEST ***\n");
}

main()
{
    NTSTATUS st;
    OBJECT_ATTRIBUTES Obja;

    InitializeObjectAttributes(&Obja, NULL, 0, NULL, NULL);

    st = NtCreatePort(
            &DebugPort,
            &Obja,
            0L,
            256,
            256 * 16
            );
    ASSERT(NT_SUCCESS(st));

    UdbgTest2();
    UdbgTest1();

}
