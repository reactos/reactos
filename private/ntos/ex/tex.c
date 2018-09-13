/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tex.c

Abstract:

    Test program for the EX subcomponent of the NTOS project

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "exp.h"
//#include "zwapi.h"
#include <version.h>
#include <string.h>

#define DumpPool(x, y)

BOOLEAN
ExTest (
    VOID
    );

PTESTFCN TestFunction = ExTest;

#ifndef MIPS

USHORT TestEvent = 0;
USHORT TestHandle = 0;
USHORT TestInfo = 0;
USHORT TestLuid = 0;
USHORT TestMemory = 0;
USHORT TestParty = 0;
USHORT TestPool = 0;
USHORT TestResource = 0;
USHORT TestBitMap = 0;
USHORT TestSemaphore = 0;
USHORT TestTimer = 0;
USHORT TestZone = 0;
USHORT TestMutant = 0;
USHORT TestException = 0;

#else

USHORT TestEvent = 1;
USHORT TestHandle = 0;
USHORT TestInfo = 0;
USHORT TestLuid = 0;
USHORT TestMemory = 0;
USHORT TestParty = 0;
USHORT TestPool = 0;
USHORT TestResource = 0;
USHORT TestBitMap = 0;
USHORT TestSemaphore = 2;
USHORT TestTimer = 3;
USHORT TestZone = 0;
USHORT TestMutant = 4;
USHORT TestException = 0;

#endif // MIPS

BOOLEAN
DoEventTest(
    )
{
    ULONG DesiredAccess = EVENT_ALL_ACCESS;
    EVENT_BASIC_INFORMATION EventInformation;
    HANDLE Handle1;
    HANDLE Handle1c;
    HANDLE Handle2;
    HANDLE Handle2c;
    ULONG Length;
    UNICODE_STRING Name1;
    UNICODE_STRING Name2;
    OBJECT_ATTRIBUTES Object1Attributes;
    OBJECT_ATTRIBUTES Object2Attributes;
    LONG State;
    NTSTATUS Status;

    //
    // Announce start of event test.
    //

    DbgPrint(" ** Start of Event Test **\n");

    //
    // Initialize strings and fill in object attributes structures.
    //

    RtlInitUnicodeString(&Name1, L "\\Event1");
    RtlInitUnicodeString(&Name2, L "\\Event2");
    InitializeObjectAttributes(&Object1Attributes, &Name1, 0, NULL, NULL);
    InitializeObjectAttributes(&Object2Attributes, &Name2, 0, NULL, NULL);

    //
    // Create event 1.
    //

    Status = ZwCreateEvent(&Handle1c, DesiredAccess, &Object1Attributes,
                           NotificationEvent, TRUE);
    if (Status < 0) {
        DbgPrint(" Event test - create event 1 failed, status = %lx\n", Status);
    }

    //
    // Open event 1.
    //

    Status = ZwOpenEvent(&Handle1, DesiredAccess, &Object1Attributes);
    if (Status < 0) {
        DbgPrint(" Event test - open event 1 failed, status = %lx\n", Status);
    }

    //
    // Query event 1.
    //

    EventInformation.EventState = 0;
    Length = 0;
    Status = ZwQueryEvent(Handle1, EventBasicInformation,
                          (PVOID)&EventInformation, sizeof(EVENT_BASIC_INFORMATION),
                          &Length);
    if (Status < 0) {
        DbgPrint(" Event test - query event 1 failed, status = %lx\n", Status);
    }
    if (EventInformation.EventType != NotificationEvent) {
        DbgPrint(" Event test - query event 1 wrong event type\n");
    }
    if (EventInformation.EventState == 0) {
        DbgPrint(" Event test - query event 1 current state wrong\n");
    }
    if (Length != sizeof(EVENT_BASIC_INFORMATION)) {
        DbgPrint(" Event test - query event 1 return length wrong\n");
    }

    //
    // Pulse event 1.
    //

    State = 0;
    Status = ZwPulseEvent(Handle1, &State);
    if (Status < 0) {
        DbgPrint(" Event test - pulse event 1 failed, status = %lx\n", Status);
    }
    if (State == 0) {
        DbgPrint(" Event test - pulse event 1 previous state wrong\n");
    }

    //
    // Set event 1.
    //

    State = 1;
    Status = ZwSetEvent(Handle1, &State);
    if (Status < 0) {
        DbgPrint(" Event test - set event 1 failed, status = %lx\n", Status);
    }
    if (State == 1) {
        DbgPrint(" Event test - set event 1 previous state wrong\n");
    }

    //
    // Wait on event 1.
    //

    Status = ZwWaitForSingleObject(Handle1, FALSE, NULL);
    if (Status < 0) {
        DbgPrint(" Event test - wait event 1 failed\n");
    }

    //
    // Reset event 1.
    //

    State = 0;
    Status = ZwResetEvent(Handle1, &State);
    if (Status < 0) {
        DbgPrint(" Event test - reset event 1 failed, status = %lx\n", Status);
    }
    if (State == 0) {
        DbgPrint(" Event test - reset event 1 previous state wrong\n");
    }

    //
    // Create event 2.
    //

    Status = ZwCreateEvent(&Handle2c, DesiredAccess, &Object2Attributes,
                           NotificationEvent, FALSE);
    if (Status < 0) {
        DbgPrint(" Event test - create event 2 failed, status = %lx\n", Status);
    }

    //
    // Open event 2.
    //

    Status = ZwOpenEvent(&Handle2, DesiredAccess, &Object2Attributes);
    if (Status < 0) {
        DbgPrint(" Event test - open event 2 failed, status = %lx\n", Status);
    }

    //
    // Query event 2.
    //

    EventInformation.EventState = 1;
    Length = 0;
    Status = ZwQueryEvent(Handle2, EventBasicInformation,
                          (PVOID)&EventInformation, sizeof(EVENT_BASIC_INFORMATION),
                          &Length);
    if (Status < 0) {
        DbgPrint(" Event test - query event 2 failed, status = %lx\n", Status);
    }
    if (EventInformation.EventType != NotificationEvent) {
        DbgPrint(" Event test - query event 2 wrong event type\n");
    }
    if (EventInformation.EventState == 1) {
        DbgPrint(" Event test - query event 2 current state wrong\n");
    }
    if (Length != sizeof(EVENT_BASIC_INFORMATION)) {
        DbgPrint(" Event test - query event 2 return length wrong\n");
    }

    //
    // Pulse event 2.
    //

    State = 1;
    Status = ZwPulseEvent(Handle2, &State);
    if (Status < 0) {
        DbgPrint(" Event test - pulse event 2 failed, status = %lx\n", Status);
    }
    if (State == 1) {
        DbgPrint(" Event test - pulse event 2 previous state wrong\n");
    }

    //
    // Set event 2.
    //

    State = 1;
    Status = ZwSetEvent(Handle2, &State);
    if (Status < 0) {
        DbgPrint(" Event test - set event 2 failed, status = %lx\n", Status);
    }
    if (State == 1) {
        DbgPrint(" Event test - set event 2 previous state wrong\n");
    }

    //
    // Wait on event 2.
    //

    Status = ZwWaitForSingleObject(Handle2, FALSE, NULL);
    if (Status < 0) {
        DbgPrint(" Event test - wait event 2 failed\n");
    }

    //
    // Reset event 2.
    //

    State = 0;
    Status = ZwResetEvent(Handle2, &State);
    if (Status < 0) {
        DbgPrint(" Event test - reset event 2 failed, status = %lx\n", Status);
    }
    if (State == 0) {
        DbgPrint(" Event test - reset event 2 previous state wrong\n");
    }

    //
    // Close all handles.
    //

    Status = NtClose(Handle1);
    if (Status < 0) {
        DbgPrint(" Event test - event 1 close failed, status = %lx\n", Status);
    }
    Status = NtClose(Handle1c);
    if (Status < 0) {
        DbgPrint(" Event test - event 1c close failed, status = %lx\n", Status);
    }
    Status = NtClose(Handle2);
    if (Status < 0) {
        DbgPrint(" Event test - event 2 close failed, status = %lx\n", Status);
    }
    Status = NtClose(Handle2c);
    if (Status < 0) {
        DbgPrint(" Event test - event 2c close failed, status = %lx\n", Status);
    }

    //
    // Announce end of event test.
    //

    DbgPrint(" ** End of Event Test **\n");
    return TRUE;
}

BOOLEAN
DoExceptionTest(
    )

{
#ifndef  i386
    NTSTATUS Status;

    //
    // Announce start of system service exception test.
    //

    DbgPrint(" ** Start of System Service Exception Test **\n");

    //
    // Eventually this should have a test case for each system service that
    // has input of output arguments which are addressed by pointers. The
    // intent of this test is to make sure that each service correctly
    // handles access violations.
    //

    //
    // Query system time test.
    //

    Status = ZwQuerySystemTime((PLARGE_INTEGER)NULL);
    if (Status != STATUS_ACCESS_VIOLATION) {
        DbgPrint(" Exception test - NtQuerySystemTime failed, status = %lx\n", Status);
    }

    //
    // Set system time test.
    //

    Status = ZwSetSystemTime((PLARGE_INTEGER)NULL, (PLARGE_INTEGER)NULL);
    if (Status != STATUS_ACCESS_VIOLATION) {
        DbgPrint(" Exception test - NtSetSystemTime failed, status = %lx\n", Status);
    }

    //
    // Announce end of system service exception test.
    //

    DbgPrint(" ** End of System Service Exception Test **\n");
#else
    DbgPrint(" ** Skip System Service Exception Test for 386 **\n");
#endif  // i386
    return TRUE;
}

BOOLEAN
DoMutantTest(
    )
{

    LONG Count;
    ULONG DesiredAccess = MUTANT_ALL_ACCESS;
    HANDLE Handle1;
    HANDLE Handle1c;
    HANDLE Handle2;
    HANDLE Handle2c;
    ULONG Length;
    STRING Name1;
    STRING Name2;
    OBJECT_ATTRIBUTES Object1Attributes;
    OBJECT_ATTRIBUTES Object2Attributes;
    MUTANT_BASIC_INFORMATION MutantInformation;
    NTSTATUS Status;

    //
    // Announce start of mutant test.
    //

    DbgPrint(" ** Start of Mutant Test **\n");

    //
    // Initialize strings and fill in object attributes structures.
    //

    RtlInitUnicodeString(&Name1, L"\\Mutant1");
    RtlInitUnicodeString(&Name2, L"\\Mutant2");
    InitializeObjectAttributes(&Object1Attributes,&Name1,0,NULL,NULL);
    InitializeObjectAttributes(&Object2Attributes,&Name2,0,NULL,NULL);

    //
    // Create mutant 1.
    //

    Status = ZwCreateMutant(&Handle1c, DesiredAccess, &Object1Attributes,
                            FALSE);
    if (Status < 0) {
        DbgPrint(" Mutant test - create mutant 1 failed, status = %lx\n",
                Status);
    }

    //
    // Open mutant 1.
    //

    Status = ZwOpenMutant(&Handle1, DesiredAccess, &Object1Attributes);
    if (Status < 0) {
        DbgPrint(" Mutant test - open mutant 1 failed, status = %lx\n",
                Status);
    }

    //
    // Query mutant 1.
    //

    MutantInformation.CurrentCount = 10;
    MutantInformation.AbandonedState = TRUE;
    Length = 0;
    Status = ZwQueryMutant(Handle1, MutantBasicInformation,
                           (PVOID)&MutantInformation,
                           sizeof(MUTANT_BASIC_INFORMATION), &Length);
    if (Status < 0) {
        DbgPrint(" Mutant test - query mutant 1 failed, status = %lx\n",
                Status);
    }
    if (MutantInformation.CurrentCount != 1) {
        DbgPrint(" Mutant test - query mutant 1 current count wrong\n");
    }
    if (MutantInformation.AbandonedState != FALSE) {
        DbgPrint(" Mutant test - query mutant 1 abandoned state wrong\n");
    }
    if (Length != sizeof(MUTANT_BASIC_INFORMATION)) {
        DbgPrint(" Mutant test - query mutant 1 return length wrong\n");
    }

    //
    // Acquire mutant 1.
    //

    Status = ZwWaitForSingleObject(Handle1, FALSE, NULL);
    if (Status < 0) {
        DbgPrint(" Mutant test - wait mutant 1 failed, status = %lx\n",
                Status);
    }

    //
    // Release mutant 1.
    //

    Count = 100;
    Status = ZwReleaseMutant(Handle1, &Count);
    if (Status < 0) {
        DbgPrint(" Mutant test - release mutant 1 failed, status = %lx\n",
                Status);
    }
    if (Count != 0) {
        DbgPrint(" Mutant test - release mutant 1 previous count wrong\n");
    }

    //
    // Create mutant 2.
    //

    Status = ZwCreateMutant(&Handle2c, DesiredAccess, &Object2Attributes,
                            FALSE);
    if (Status < 0) {
        DbgPrint(" Mutant test - create mutant 2 failed, status = %lx\n",
                Status);
    }

    //
    // Open mutant 2.
    //

    Status = ZwOpenMutant(&Handle2, DesiredAccess, &Object2Attributes);
    if (Status < 0) {
        DbgPrint(" Mutant test - open mutant 2 failed, status = %lx\n",
                Status);
    }

    //
    // Acquire mutant 2.
    //

    Status = ZwWaitForSingleObject(Handle2, FALSE, NULL);
    if (Status < 0) {
        DbgPrint(" Mutant test - wait mutant 2 failed, status = %lx\n",
                Status);
    }

    //
    // Query mutant 2.
    //

    MutantInformation.CurrentCount = 20;
    MutantInformation.AbandonedState = TRUE;
    Length = 0;
    Status = ZwQueryMutant(Handle2, MutantBasicInformation,
                          (PVOID)&MutantInformation,
                          sizeof(MUTANT_BASIC_INFORMATION), &Length);
    if (Status < 0) {
        DbgPrint(" Mutant test - query mutant 2 failed, status = %lx\n",
                Status);
    }
    if (MutantInformation.CurrentCount != 0) {
        DbgPrint(" Mutant test - query mutant 2 current count wrong\n");
    }
    if (MutantInformation.AbandonedState != FALSE) {
        DbgPrint(" Mutant test - query mutant 2 abandoned state wrong\n");
    }
    if (Length != sizeof(MUTANT_BASIC_INFORMATION)) {
        DbgPrint(" Mutant test - query mutant 2 return length wrong\n");
    }

    //
    // Acquire mutant 2.
    //

    Status = ZwWaitForSingleObject(Handle2, FALSE, NULL);
    if (Status < 0) {
        DbgPrint(" Mutant test - wait mutant 2 failed, status = %lx\n",
                Status);
    }

    //
    // Release mutant 2.
    //

    Count = 100;
    Status = ZwReleaseMutant(Handle2, &Count);
    if (Status < 0) {
        DbgPrint(" Mutant test - release mutant 2 failed, status = %lx\n",
                Status);
    }
    if (Count != - 1) {
        DbgPrint(" Mutant test - release mutant 2 previous count wrong\n");
    }

    //
    // Release mutant 2.
    //

    Count = 100;
    Status = ZwReleaseMutant(Handle2, &Count);
    if (Status < 0) {
        DbgPrint(" Mutant test - release mutant 2 failed, status = %lx\n",
                Status);
    }
    if (Count != 0) {
        DbgPrint(" Mutant test - release mutant 2 previous count wrong\n");
    }

    //
    // Close all handles.
    //

    Status = NtClose(Handle1);
    if (Status < 0) {
        DbgPrint(" Mutant test - mutant 1 close failed, status = %lx\n",
                Status);
    }
    Status = NtClose(Handle1c);
    if (Status < 0) {
        DbgPrint(" Mutant test - mutant 1c close failed, status = %lx\n",
                Status);
    }
    Status = NtClose(Handle2);
    if (Status < 0) {
        DbgPrint(" Mutant test - mutant 2 close failed, status = %lx\n",
                Status);
    }
    Status = NtClose(Handle2c);
    if (Status < 0) {
        DbgPrint(" Mutant test - mutant 2c close failed, status = %lx\n",
                Status);
    }

    //
    // Announce end of mutant test.
    //

    DbgPrint(" ** End of Mutant Test **\n");
    return TRUE;
}

BOOLEAN
DoSemaphoreTest(
    )
{

    LONG Count;
    ULONG DesiredAccess = SEMAPHORE_ALL_ACCESS;
    HANDLE Handle1;
    HANDLE Handle1c;
    HANDLE Handle2;
    HANDLE Handle2c;
    ULONG Length;
    STRING Name1;
    STRING Name2;
    OBJECT_ATTRIBUTES Object1Attributes;
    OBJECT_ATTRIBUTES Object2Attributes;
    SEMAPHORE_BASIC_INFORMATION SemaphoreInformation;
    NTSTATUS Status;

    //
    // Announce start of semaphore test.
    //

    DbgPrint(" ** Start of Semaphore Test **\n");

    //
    // Initialize strings and fill in object attributes structures.
    //

    RtlInitUnicodeString(&Name1, L"\\Semaphore1");
    RtlInitUnicodeString(&Name2, L"\\Semaphore2");
    InitializeObjectAttributes(&Object1Attributes,&Name1,0,NULL,NULL);
    InitializeObjectAttributes(&Object2Attributes,&Name2,0,NULL,NULL);

    //
    // Create semaphore 1.
    //

    Status = ZwCreateSemaphore(&Handle1c, DesiredAccess, &Object1Attributes,
                               0, 10);
    if (Status < 0) {
        DbgPrint(" Semaphore test - create semaphore 1 failed, status = %lx\n",
                Status);
    }

    //
    // Open semaphore 1.
    //

    Status = ZwOpenSemaphore(&Handle1, DesiredAccess, &Object1Attributes);
    if (Status < 0) {
        DbgPrint(" Semaphore test - open semaphore 1 failed, status = %lx\n",
                Status);
    }

    //
    // Query semaphore 1.
    //

    SemaphoreInformation.CurrentCount = 10;
    SemaphoreInformation.MaximumCount = 0;
    Length = 0;
    Status = ZwQuerySemaphore(Handle1, SemaphoreBasicInformation,
                          (PVOID)&SemaphoreInformation,
                          sizeof(SEMAPHORE_BASIC_INFORMATION), &Length);
    if (Status < 0) {
        DbgPrint(" Semaphore test - query semaphore 1 failed, status = %lx\n",
                Status);
    }
    if (SemaphoreInformation.CurrentCount != 0) {
        DbgPrint(" Semaphore test - query semaphore 1 current count wrong\n");
    }
    if (SemaphoreInformation.MaximumCount != 10) {
        DbgPrint(" Semaphore test - query semaphore 1 maximum count wrong\n");
    }
    if (Length != sizeof(SEMAPHORE_BASIC_INFORMATION)) {
        DbgPrint(" Semaphore test - query semaphore 1 return length wrong\n");
    }

    //
    // Release semaphore 1.
    //

    Count = 100;
    Status = ZwReleaseSemaphore(Handle1, 2, &Count);
    if (Status < 0) {
        DbgPrint(" Semaphore test - release semaphore 1 failed, status = %lx\n",
                Status);
    }
    if (Count != 0) {
        DbgPrint(" Semaphore test - release semaphore 1 previous count wrong\n");
    }

    //
    // Release semaphore 1.
    //

    Count = 100;
    Status = ZwReleaseSemaphore(Handle1, 5, &Count);
    if (Status < 0) {
        DbgPrint(" Semaphore test - release semaphore 1 failed, status = %lx\n",
                Status);
    }
    if (Count != 2) {
        DbgPrint(" Semaphore test - release semaphore 1 previous count wrong\n");
    }

    //
    // Create semaphore 2.
    //

    Status = ZwCreateSemaphore(&Handle2c, DesiredAccess, &Object2Attributes,
                               5, 20);
    if (Status < 0) {
        DbgPrint(" Semaphore test - create semaphore 2 failed, status = %lx\n",
                Status);
    }

    //
    // Open semaphore 2.
    //

    Status = ZwOpenSemaphore(&Handle2, DesiredAccess, &Object2Attributes);
    if (Status < 0) {
        DbgPrint(" Semaphore test - open semaphore 2 failed, status = %lx\n",
                Status);
    }

    //
    // Query semaphore 2.
    //

    SemaphoreInformation.CurrentCount = 20;
    SemaphoreInformation.MaximumCount = 5;
    Length = 0;
    Status = ZwQuerySemaphore(Handle2, SemaphoreBasicInformation,
                          (PVOID)&SemaphoreInformation,
                          sizeof(SEMAPHORE_BASIC_INFORMATION), &Length);
    if (Status < 0) {
        DbgPrint(" Semaphore test - query semaphore 2 failed, status = %lx\n",
                Status);
    }
    if (SemaphoreInformation.CurrentCount != 5) {
        DbgPrint(" Semaphore test - query semaphore 2 current count wrong\n");
    }
    if (SemaphoreInformation.MaximumCount != 20) {
        DbgPrint(" Semaphore test - query semaphore 2 maximum count wrong\n");
    }
    if (Length != sizeof(SEMAPHORE_BASIC_INFORMATION)) {
        DbgPrint(" Semaphore test - query semaphore 2 return length wrong\n");
    }

    //
    // Release semaphore 2.
    //

    Count = 100;
    Status = ZwReleaseSemaphore(Handle2, 3, &Count);
    if (Status < 0) {
        DbgPrint(" Semaphore test - release semaphore 2 failed, status = %lx\n",
                Status);
    }
    if (Count != 5) {
        DbgPrint(" Semaphore test - release semaphore 2 previous count wrong\n");
    }

    //
    // Release semaphore 2.
    //

    Count = 100;
    Status = ZwReleaseSemaphore(Handle2, 5, &Count);
    if (Status < 0) {
        DbgPrint(" Semaphore test - release semaphore 2 failed, status = %lx\n",
                Status);
    }
    if (Count != 8) {
        DbgPrint(" Semaphore test - release semaphore 2 previous count wrong\n");
    }

    //
    // Close all handles.
    //

    Status = NtClose(Handle1);
    if (Status < 0) {
        DbgPrint(" Semaphore test - semaphore 1 close failed, status = %lx\n",
                Status);
    }
    Status = NtClose(Handle1c);
    if (Status < 0) {
        DbgPrint(" Semaphore test - semaphore 1c close failed, status = %lx\n",
                Status);
    }
    Status = NtClose(Handle2);
    if (Status < 0) {
        DbgPrint(" Semaphore test - semaphore 2 close failed, status = %lx\n",
                Status);
    }
    Status = NtClose(Handle2c);
    if (Status < 0) {
        DbgPrint(" Semaphore test - semaphore 2c close failed, status = %lx\n",
                Status);
    }

    //
    // Announce end of semaphore test.
    //

    DbgPrint(" ** End of Semaphore Test **\n");
    return TRUE;
}

VOID
TimerApcRoutine (
    IN PVOID TimerContext,
    IN ULONG TimerLowValue,
    IN LONG TimerHighValue
    )

{

    *((PBOOLEAN)TimerContext) = TRUE;
    return;
}

BOOLEAN
DoTimerTest (
    )

{

    BOOLEAN ApcHappened;
    BOOLEAN CurrentState;
    ULONG DesiredAccess = TIMER_ALL_ACCESS;
    LARGE_INTEGER DueTime;
    HANDLE Handle1;
    HANDLE Handle1c;
    HANDLE Handle2;
    HANDLE Handle2c;
    ULONG Length;
    STRING Name1;
    STRING Name2;
    OBJECT_ATTRIBUTES Object1Attributes;
    OBJECT_ATTRIBUTES Object2Attributes;
    BOOLEAN PreviousState;
    TIMER_BASIC_INFORMATION TimerInformation;
    NTSTATUS Status;

    //
    // Announce start of timer test.
    //

    DbgPrint(" ** Start of Timer Test **\n");

    //
    // Initialize strings and fill in object attributes structures.
    //

    RtlInitUnicodeString(&Name1, L"\\Timer1");
    RtlInitUnicodeString(&Name2, L"\\Timer2");
    InitializeObjectAttributes(&Object1Attributes,&Name1,0,NULL,NULL);
    InitializeObjectAttributes(&Object2Attributes,&Name2,0,NULL,NULL);

    //
    // Create timer 1.
    //

    Status = ZwCreateTimer(&Handle1c, DesiredAccess, &Object1Attributes);
    if (!NT_SUCCESS(Status)) {
        DbgPrint(" Timer test - create timer 1 failed, status = %lx\n",
                Status);
    }

    //
    // Open timer 1.
    //

    Status = ZwOpenTimer(&Handle1, DesiredAccess, &Object1Attributes);
    if (Status < 0) {
        DbgPrint(" Timer test - open timer 1 failed, status = %lx\n",
                Status);
    }

    //
    // Query timer 1.
    //

    TimerInformation.TimerState = TRUE;
    Length = 0;
    Status = ZwQueryTimer(Handle1, TimerBasicInformation,
                          (PVOID)&TimerInformation,
                          sizeof(TIMER_BASIC_INFORMATION), &Length);
    if (Status < 0) {
        DbgPrint(" Timer test - query timer 1 failed, status = %lx\n",
                Status);
    }
    if (TimerInformation.TimerState) {
        DbgPrint(" Timer test - query timer 1 state wrong\n");
    }
    if (Length != sizeof(TIMER_BASIC_INFORMATION)) {
        DbgPrint(" Timer test - query timer 1 return length wrong\n");
    }

    //
    // Set timer 1 and then cancel timer 1.
    //

    DueTime.LowPart = -100000;
    DueTime.HighPart = -1;
    PreviousState = TRUE;
    Status = ZwSetTimer(Handle1, &DueTime, NULL, NULL, &PreviousState);
    if (!NT_SUCCESS(Status)) {
        DbgPrint(" Timer test - set timer 1 failed, status = %lx\n",
                Status);
    }
    if (PreviousState) {
        DbgPrint(" Timer test - set timer 1 previous state wrong\n");
    }
    CurrentState = TRUE;
    Status = ZwCancelTimer(Handle1, &CurrentState);
    if (!NT_SUCCESS(Status)) {
        DbgPrint(" Timer test - cancel timer 1 failed, status = %lx\n",
                Status);
    }
    if (CurrentState) {
        DbgPrint(" Timer test - cancel timer 1 current state wrong\n");
    }

    //
    // Set timer 1, wait for timer to expire, and then cancel timer 1.
    //

    DueTime.LowPart = -5;
    DueTime.HighPart = -1;
    PreviousState = TRUE;
    Status = ZwSetTimer(Handle1, &DueTime, NULL, NULL, &PreviousState);
    if (!NT_SUCCESS(Status)) {
        DbgPrint(" Timer test - set timer 1 failed, status = %lx\n",
                Status);
    }
    if (PreviousState) {
        DbgPrint(" Timer test - set timer 1 previous state wrong\n");
    }
    Status = ZwWaitForSingleObject(Handle1, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        DbgPrint(" Timer test - wait timer 1 failed, status = %lx\n",
                Status);
    }
    CurrentState = FALSE;
    Status = ZwCancelTimer(Handle1, &CurrentState);
    if (!NT_SUCCESS(Status)) {
        DbgPrint(" Timer test - cancel timer 1 failed, status = %lx\n",
                Status);
    }
    if (!CurrentState) {
        DbgPrint(" Timer test - cancel timer 1 current state wrong\n");
    }

    //
    // Set timer 1 with APC, then cancel timer 1.
    //

    ApcHappened = FALSE;
    DueTime.LowPart = -100000;
    DueTime.HighPart = -1;
    PreviousState = FALSE;
    Status = ZwSetTimer(Handle1, &DueTime, TimerApcRoutine, &ApcHappened,
                        &PreviousState);
    if (!NT_SUCCESS(Status)) {
        DbgPrint(" Timer test - set timer 1 failed, status = %lx\n",
                Status);
    }
    if (!PreviousState) {
        DbgPrint(" Timer test - set timer 1 previous state wrong\n");
    }
    CurrentState = TRUE;
    Status = ZwCancelTimer(Handle1, &CurrentState);
    if (!NT_SUCCESS(Status)) {
        DbgPrint(" Timer test - cancel timer 1 failed, status = %lx\n",
                Status);
    }
    if (CurrentState) {
        DbgPrint(" Timer test - cancel timer 1 current state wrong\n");
    }
    if (ApcHappened) {
        DbgPrint(" Timer test - cancel timer 1 APC happened state wrong\n");
    }

    //
    // Set timer 1 with APC, set timer again with APC, wait for timer, then
    // cancel timer 1.
    //

    ApcHappened = FALSE;
    DueTime.LowPart = -100000;
    DueTime.HighPart = -1;
    PreviousState = TRUE;
    Status = ZwSetTimer(Handle1, &DueTime, TimerApcRoutine, &ApcHappened,
                        &PreviousState);
    if (!NT_SUCCESS(Status)) {
        DbgPrint(" Timer test - set timer 1 failed, status = %lx\n",
                Status);
    }
    if (PreviousState) {
        DbgPrint(" Timer test - set timer 1 previous state wrong\n");
    }
    DueTime.LowPart = -5;
    DueTime.HighPart = -1;
    PreviousState = TRUE;
    Status = ZwSetTimer(Handle1, &DueTime, TimerApcRoutine, &ApcHappened,
                        &PreviousState);
    if (!NT_SUCCESS(Status)) {
        DbgPrint(" Timer test - set timer 1 failed, status = %lx\n",
                Status);
    }
    if (PreviousState) {
        DbgPrint(" Timer test - set timer 1 previous state wrong\n");
    }
    Status = ZwWaitForSingleObject(Handle1, FALSE, NULL);
    if (!NT_SUCCESS(Status)) {
        DbgPrint(" Timer test - wait timer 1 failed, status = %lx\n",
                Status);
    }
    CurrentState = FALSE;
    Status = ZwCancelTimer(Handle1, &CurrentState);
    if (!NT_SUCCESS(Status)) {
        DbgPrint(" Timer test - cancel timer 1 failed, status = %lx\n",
                Status);
    }
    if (!CurrentState) {
        DbgPrint(" Timer test - cancel timer 1 current state wrong\n");
    }
    if (!ApcHappened) {
        DbgPrint(" Timer test - cancel timer 1 APC happened state wrong\n");
    }

    //
    // Create timer 2.
    //

    Status = ZwCreateTimer(&Handle2c, DesiredAccess, &Object2Attributes);
    if (Status < 0) {
        DbgPrint(" Timer test - create timer 2 failed, status = %lx\n",
                Status);
    }

    //
    // Open timer 2.
    //

    Status = ZwOpenTimer(&Handle2, DesiredAccess, &Object2Attributes);
    if (Status < 0) {
        DbgPrint(" Timer test - open timer 2 failed, status = %lx\n",
                Status);
    }

    //
    // Query timer 2.
    //

    TimerInformation.TimerState = TRUE;
    Length = 0;
    Status = ZwQueryTimer(Handle2, TimerBasicInformation,
                          (PVOID)&TimerInformation,
                          sizeof(TIMER_BASIC_INFORMATION), &Length);
    if (Status < 0) {
        DbgPrint(" Timer test - query timer 2 failed, status = %lx\n",
                Status);
    }
    if (TimerInformation.TimerState) {
        DbgPrint(" Timer test - query timer 2 state wrong\n");
    }
    if (Length != sizeof(TIMER_BASIC_INFORMATION)) {
        DbgPrint(" Timer test - query timer 2 return length wrong\n");
    }

    //
    // Close all handles.
    //

    Status = NtClose(Handle1);
    if (Status < 0) {
        DbgPrint(" Timer test - timer 1 close failed, status = %lx\n",
                Status);
    }
    Status = NtClose(Handle1c);
    if (Status < 0) {
        DbgPrint(" Timer test - timer 1c close failed, status = %lx\n",
                Status);
    }
    Status = NtClose(Handle2);
    if (Status < 0) {
        DbgPrint(" Timer test - timer 2 close failed, status = %lx\n",
                Status);
    }
    Status = NtClose(Handle2c);
    if (Status < 0) {
        DbgPrint(" Timer test - timer 2c close failed, status = %lx\n",
                Status);
    }

    //
    // Announce end of timer test.
    //

    DbgPrint(" ** End of Timer Test **\n");
    return TRUE;
}

BOOLEAN
TestDupHandle1(
    IN PVOID HandleTableEntry
    )
{
    DbgPrint( "Dupping %lx\n", HandleTableEntry );
    return( TRUE );
}

BOOLEAN
TestDupHandle4(
    IN PVOID HandleTableEntry
    )
{
    PULONG p = (PULONG)HandleTableEntry;
    ULONG i;

    if (!((*p>>4) % 4)) {
        return( FALSE );
        }

    DbgPrint( "Dupping " );
    for (i=0; i<4; i++) {
        DbgPrint( "  %lx", *p++ );
        }
    DbgPrint( "\n" );
    return( TRUE );
}

BOOLEAN
TestEnumHandle1(
    IN PVOID HandleTableEntry,
    IN PVOID EnumParameter
    )
{
    if (EnumParameter == HandleTableEntry) {
        return( TRUE );
        }
    else {
        return( FALSE );
        }
}

BOOLEAN
TestEnumHandle4(
    IN PVOID HandleTableEntry,
    IN PVOID EnumParameter
    )
{
    if (EnumParameter == (PVOID)*(PULONG)HandleTableEntry) {
        return( TRUE );
        }
    else {
        return( FALSE );
        }
}

#define HANDLE_TEST_SIZE    30

BOOLEAN
DoHandleTest( void )
{
    PVOID HandleTable1;
    PVOID HandleTable4;
    PVOID HandleTable1a;
    PVOID HandleTable4a;
    HANDLE HandlesForTable1[ HANDLE_TEST_SIZE ];
    HANDLE HandlesForTable4[ HANDLE_TEST_SIZE ];
    HANDLE h;
    PULONG HandleValue;
    BOOLEAN LockFlag;
    ULONG i, v[4];

    HandleTable1 = ExCreateHandleTable( (PEPROCESS)NULL, 0L, 0L, 0L, MUTEX_LEVEL_PS_CID_TABLE, FALSE );
    HandleTable4 = ExCreateHandleTable( (PEPROCESS)NULL, 16L, 8L, 2L, MUTEX_LEVEL_OB_TABLE, TRUE );

    ExDumpHandleTable( (PEPROCESS)NULL, HandleTable1, NULL );
    ExDumpHandleTable( (PEPROCESS)NULL, HandleTable4, NULL );

    for (i=0; i<HANDLE_TEST_SIZE; i++) {
        v[0] = (i+1) << 4;
        v[1] = (i+1) << 3;
        v[2] = (i+1) << 2;
        v[3] = (i+1) << 1;

        HandlesForTable1[ i ] = ExCreateHandle( HandleTable1, (PVOID)(v[0]) );
        DbgPrint( "HandleTable1: %lx => %lx\n", HandlesForTable1[ i ], v[0] );
        HandlesForTable4[ i ] = ExCreateHandle( HandleTable4, (PVOID)(&v[0]) );
        DbgPrint( "HandleTable4: %lx => %lx\n", HandlesForTable4[ i ], v[0] );
        }

    ExDumpHandleTable( HandleTable1, NULL, NULL );
    ExDumpHandleTable( HandleTable4, NULL, NULL );

    for (i=0; i<=HANDLE_TEST_SIZE; i++) {
        v[0] = (i+1) << 4;
        v[1] = (i+1) << 3;
        v[2] = (i+1) << 2;
        v[3] = (i+1) << 1;

        if (ExEnumHandleTable( HandleTable1, TestEnumHandle1, (PVOID)(v[0]), &h )) {
            DbgPrint( "HandleTable1: Found: %lx <= %lx\n", v[0], h );
            }
        else {
            DbgPrint( "HandleTable1: %lx not found\n", v[0] );
            }

        if (ExEnumHandleTable( HandleTable4, TestEnumHandle4, (PVOID)(v[0]), &h )) {
            DbgPrint( "HandleTable4: Found: %lx <= %lx\n", v[0], h );
            }
        else {
            DbgPrint( "HandleTable4: %lx not found\n", v[0] );
            }
        }

    for (i=0; i<HANDLE_TEST_SIZE; i++) {
        LockFlag = ExMapHandleToPointer( HandleTable1,
                                         HandlesForTable1[ i ],
                                         (PVOID)&HandleValue
                                       );

        DbgPrint( "HandleTable1: %lx => %lx\n",
                 HandlesForTable1[ i ], HandleValue
               );
        ExUnlockHandleTable( HandleTable1, LockFlag );

        LockFlag = ExMapHandleToPointer( HandleTable4,
                                         HandlesForTable4[ i ],
                                         (PVOID)&HandleValue
                                       );
        DbgPrint( "HandleTable4: %lx => %lx\n",
                 HandlesForTable4[ i ], *HandleValue
               );
        ExUnlockHandleTable( HandleTable4, LockFlag );
        }

    HandleTable1a = ExDupHandleTable( (PEPROCESS)NULL, HandleTable1, TestDupHandle1 );
    HandleTable4a = ExDupHandleTable( (PEPROCESS)NULL, HandleTable4, TestDupHandle4 );

    ExDumpHandleTable( HandleTable1a, NULL, NULL );
    ExDumpHandleTable( HandleTable4a, NULL, NULL );

    for (i=0; i<HANDLE_TEST_SIZE; i++) {
        ExDestroyHandle( HandleTable1, HandlesForTable1[ i ] );
        ExDestroyHandle( HandleTable4, HandlesForTable4[ i ] );
        }

    ExDumpHandleTable( HandleTable1, NULL, NULL );
    ExDumpHandleTable( HandleTable4, NULL, NULL );

    ExDestroyHandleTable( HandleTable1, NULL );
    ExDestroyHandleTable( HandleTable4, NULL );

    ExDestroyHandleTable( HandleTable1a, NULL );
    ExDestroyHandleTable( HandleTable4a, NULL );

    return( TRUE );
}

BOOLEAN
DoInfoTest( void )
{
    BOOLEAN Result = FALSE;
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_PROCESSOR_INFORMATION ProcessorInfo;
    ULONG ReturnedLength;

    DbgPrint(" ** Start of System Information Test **\n");
    Status = ZwQuerySystemInformation( SystemBasicInformation,
                                       (PVOID)&BasicInfo,
                                       sizeof( BasicInfo ),
                                       &ReturnedLength
                                     );
    if (NT_SUCCESS( Status )) {
        DbgPrint( "NtQuerySystemInformation returns:\n" );
        DbgPrint( "    Number of Processors: %ld\n",
                 BasicInfo.NumberOfProcessors
               );
        DbgPrint( "    OEM Machine Id: %lx\n",
                 BasicInfo.OemMachineId
               );
        DbgPrint( "    Timer Resolution: %ld microseconds\n",
                 BasicInfo.TimerResolutionInMicroSeconds
               );
        DbgPrint( "    Page Size: %ld   Allocation Granularity: %ld\n",
                 BasicInfo.PageSize,
                 BasicInfo.AllocationGranularity
               );
        DbgPrint( "    User Mode Address Range: 0x%08lx <-> 0x%08lx\n",
                 BasicInfo.MinimumUserModeAddress,
                 BasicInfo.MaximumUserModeAddress
               );
        }
    else {
        DbgPrint( "NtQuerySystemInformation failed.  Status == %X\n",
                 Status
               );
        }

    DbgPrint(" ** End of System Information Test **\n");
    return( Result );
}

BOOLEAN
DoLuidTest( void )
{
    BOOLEAN Result = TRUE;
    NTSTATUS Status;

    LUID FirstLuid;
    LUID SecondLuid;

    FirstLuid.LowPart = 0;
    FirstLuid.HighPart = 0;

    SecondLuid.LowPart = 0;
    SecondLuid.HighPart = 0;

    DbgPrint(" ** Start of Locally Unique ID Test **\n");



    Status = ZwAllocateLocallyUniqueId( &FirstLuid );

    if (!NT_SUCCESS( Status )) {
        DbgPrint( "First Luid Allocation Error.\n" );
        Result = FALSE;
    }

    if (LiLeqZero( FirstLuid )) {
        DbgPrint( "First Luid Allocation Failed - Bad Value.\n" );
        Result = FALSE;
    }



    if (Result) {

        Status = ZwAllocateLocallyUniqueId( &SecondLuid );

        if (!NT_SUCCESS( Status )) {
            DbgPrint( "Second Luid Allocation Error.\n" );
            Result = FALSE;
        }

        if (LiLeqZero( SecondLuid )) {
            DbgPrint( "Second Luid Allocation Failed - Bad Value.\n" );
            Result = FALSE;
        }

        if (LiLeq( FirstLuid, SecondLuid )) {
            DbgPrint( "Second Luid Allocation Failed - Not larger than first value.\n" );
            Result = FALSE;
        }

    }


    DbgPrint(" ** End of Locally Unique ID Test **\n");
    return( Result );
}

char MemoryTestBuffer1[ 128 ];
char TestString1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
char TestString2[] = "123456789012345678901234567890123456789012345678901234567890";
char MemoryTestBuffer2[ 128 ];

BOOLEAN
DoMemoryTest( void )
{
    LONG i,j,k;
    BOOLEAN Result;

    DbgPrint(" ** Start of Memory Test **\n");

    Result = TRUE;
    strcpy( MemoryTestBuffer1, TestString1 );
    for (i=15; i>=0; i--) {
        MemoryTestBuffer1[16] = 0xFF;
        RtlZeroMemory( &MemoryTestBuffer1[i], 16-i );
        if (strncmp( MemoryTestBuffer1, TestString1, i ) || MemoryTestBuffer1[i] || !MemoryTestBuffer1[16]) {
            DbgPrint( "*** failed *** - RtlZeroMemory( %s, %ld )\n",
                     MemoryTestBuffer1, 16-i );
            Result = FALSE;
            }
        }

    for (k = 0; k < 8; k++) {
        DbgPrint("k = %d, j = ",k);
        for (j = 0; j < 8; j++) {
            DbgPrint(" %d ",j);
            for (i=0; i<26; i++) {
                RtlZeroMemory( MemoryTestBuffer1, (ULONG)sizeof( MemoryTestBuffer1 ) );
                RtlMoveMemory( &MemoryTestBuffer1[j], &TestString2[k], i );
                if (strncmp( &MemoryTestBuffer1[j], &TestString2[k], i ) || MemoryTestBuffer1[j+i]) {
                    DbgPrint( "*** failed *** - RtlMoveMemory( %s, %s, %ld )\n",
                             &MemoryTestBuffer1[j], TestString2, i );
                    Result = FALSE;
                    }
                }
            }
        DbgPrint("\n");
        }

    for (k = 0; k < 8; k++) {
        DbgPrint("k = %d, j = ",k);
        for (j = 0; j < 8; j++) {
            DbgPrint(" %d ",j);
            for (i=0; i<26; i++) {
                RtlZeroMemory( MemoryTestBuffer2, (ULONG)sizeof( MemoryTestBuffer2 ) );
                RtlMoveMemory( &MemoryTestBuffer2[j], &TestString2[k], i );
                if (strncmp( &MemoryTestBuffer2[j], &TestString2[k], i ) || MemoryTestBuffer2[j+i]) {
                    DbgPrint( "*** failed *** - RtlMoveMemory( %s, %s, %ld )\n",
                             &MemoryTestBuffer2[j], TestString2, i );
                    Result = FALSE;
                    }
                }
            }
        DbgPrint("\n");
        }

    for (k = 0; k < 8; k++) {
        DbgPrint("k = %d, j = ",k);
        for (j = 0; j < 8; j++) {
            DbgPrint(" %d ",j);
            for (i=0; i<26; i++) {
                strcpy( MemoryTestBuffer1, TestString1 );
                RtlMoveMemory( &MemoryTestBuffer1[j], &MemoryTestBuffer1[k], i );
                if (strncmp( &MemoryTestBuffer1[j], &TestString1[k], i )) {
                    DbgPrint( "*** failed *** - RtlMoveMemory( %s, %s, %ld )\n",
                             &MemoryTestBuffer2[j], TestString2, i );
                    Result = FALSE;
                    }
                }
            }
        DbgPrint("\n");
        }

    DbgPrint(" ** End of Memory Test **\n");

    return( Result );
}

BOOLEAN
DoPartyTest( void )
{
    BOOLEAN Result = TRUE;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;

    DbgPrint(" ** Start of Party By Number Test **\n");

    NtPartyByNumber( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 );
    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );
    Status = ZwCreateEvent( &Handle,
                            EVENT_ALL_ACCESS,
                            &ObjectAttributes, NotificationEvent ,TRUE);
    NtPartyByNumber( PARTY_DUMP_OBJECT_BY_HANDLE, Handle, NULL );
    ZwClose( Handle );
    NtPartyByNumber( PARTY_DUMP_OBJECT_BY_HANDLE, Handle, NULL );

    DbgPrint(" ** End of Party By Number Test **\n");
    return( Result );
}

BOOLEAN
DoPoolTest( void )
{
    PVOID p,p0,p1,p2,p3;

    p = ExAllocatePool(NonPagedPool,4000L);
    DumpPool("After 4000 byte Allocation",NonPagedPool);
    p = ExAllocatePool(NonPagedPool,2000L);
    DumpPool("After 2000 byte Allocation",NonPagedPool);
    p = ExAllocatePool(NonPagedPool,2000L);
    DumpPool("After 2000 byte Allocation",NonPagedPool);

    p0 = ExAllocatePool(NonPagedPool,24L);
    DumpPool("After 24 byte Allocation p0",NonPagedPool);
    p1 = ExAllocatePool(NonPagedPool,24L);
    DumpPool("After 24 byte Allocation p1",NonPagedPool);
    p2 = ExAllocatePool(NonPagedPool,24L);
    DumpPool("After 24 byte Allocation p2",NonPagedPool);
    p3 = ExAllocatePool(NonPagedPool,24L);
    DumpPool("After 24 byte Allocation p3",NonPagedPool);

    ExFreePool(p1);
    DumpPool("After 24 byte Deallocation p1",NonPagedPool);
    ExFreePool(p3);
    DumpPool("After 24 byte Deallocation p3",NonPagedPool);
    ExFreePool(p2);
    DumpPool("After 24 byte Deallocation p2",NonPagedPool);
    ExFreePool(p0);
    DumpPool("After 24 byte Deallocation p0",NonPagedPool);

    p0 = ExAllocatePool(NonPagedPool,120L);
    DumpPool("After 120 byte Allocation p0",NonPagedPool);
    p1 = ExAllocatePool(NonPagedPool,24L);
    DumpPool("After 24 byte Allocation p1",NonPagedPool);
    ExFreePool(p1);
    DumpPool("After 24 byte Deallocation p1",NonPagedPool);
    ExFreePool(p0);
    DumpPool("After 120 byte Deallocation p0",NonPagedPool);

    return( TRUE );
}

BOOLEAN
DoZoneTest( void )
{
    PULONG p1,p2;
    PZONE_HEADER z;
    NTSTATUS st;
    PVOID b1, b2, b3, b4, b5;

    z = ExAllocatePool(NonPagedPool,(ULONG)sizeof(ZONE_HEADER));
    p1 = ExAllocatePool(NonPagedPool,2048L);
    p2 = ExAllocatePool(NonPagedPool,1024L);
    st = ExInitializeZone(z,512L,p1,2048L);
    ExDumpZone(z);

    b1 = ExAllocateFromZone(z);
    DbgPrint("b1 = 0x%lx\n",b1);
    ExDumpZone(z);

    b2 = ExAllocateFromZone(z);
    DbgPrint("b2 = 0x%lx\n",b2);
    ExDumpZone(z);

    b3 = ExAllocateFromZone(z);
    DbgPrint("b3 = 0x%lx\n",b3);
    ExDumpZone(z);

    b4 = ExAllocateFromZone(z);
    DbgPrint("b4 = 0x%lx\n",b4);
    ExDumpZone(z);

    b5 = ExAllocateFromZone(z);
    DbgPrint("b5 = 0x%lx\n",b5);
    ExDumpZone(z);

    ExFreeToZone(z,b4);
    ExDumpZone(z);

    ExFreeToZone(z,b3);
    ExDumpZone(z);

    ExFreeToZone(z,b2);
    ExDumpZone(z);

    ExFreeToZone(z,b1);
    ExDumpZone(z);

    st = ExExtendZone(z,p2,1024L);
    ExDumpZone(z);

    return( TRUE );
}

ERESOURCE Resource;
ULONG ResourceCount;
KSEMAPHORE ResourceSemaphore;
PVOID ExDumpResource( IN PERESOURCE Resource );

VOID
Reader (
    IN PVOID StartContext
    )
{
    LARGE_INTEGER Time;

    //KeSetPriorityThread( &PsGetCurrentThread()->Tcb, 2 );

    DbgPrint("Starting Reader %lx...\n", StartContext);

    Time.LowPart = -(1+(ULONG)StartContext);
    Time.HighPart = -1;

    while (TRUE) {

        (VOID)ExAcquireResourceShared(&Resource,TRUE);

        DbgPrint("%lx with shared access\n", StartContext);

        if (ResourceCount >= 10) {
            ExReleaseResource(&Resource);
            break;
        }

        KeDelayExecutionThread ( KernelMode, FALSE, &Time);

        ExReleaseResource(&Resource);

        DbgPrint("%lx released shared access\n", StartContext);

        KeDelayExecutionThread ( KernelMode, FALSE, &Time);
    }

    DbgPrint("Reader %lx exiting\n", StartContext);

    KeReleaseSemaphore(&ResourceSemaphore, 0, 1, FALSE);
}

VOID
Writer (
    IN PVOID StartContext
    )
{
    LARGE_INTEGER Time;

    //KeSetPriorityThread( &PsGetCurrentThread()->Tcb, 3 );

    DbgPrint("Starting Writer %lx...\n", StartContext);

    Time.LowPart = -(1+(ULONG)StartContext);
    Time.HighPart = -1;

    while (TRUE) {

        (VOID)ExAcquireResourceExclusive(&Resource,TRUE);

        DbgPrint("%lx with Exclusive access\n", StartContext);

        ResourceCount += 1;
        if (ResourceCount >= 10) {
            ExReleaseResource(&Resource);
            break;
        }

        KeDelayExecutionThread ( KernelMode, FALSE, &Time);

        ExReleaseResource(&Resource);

        DbgPrint("%lx released Exclusive access\n", StartContext);

        KeDelayExecutionThread ( KernelMode, FALSE, &Time);
    }

    DbgPrint("Writer %lx exiting\n", StartContext);

    KeReleaseSemaphore(&ResourceSemaphore, 0, 1, FALSE);
}

VOID
ReaderTurnedWriter (
    IN PVOID StartContext
    )
{
    LARGE_INTEGER Time;

    //KeSetPriorityThread( &PsGetCurrentThread()->Tcb, 4 );

    DbgPrint("Starting Reader turned Writer %lx\n", StartContext);

    Time.LowPart = -(1+(ULONG)StartContext);
    Time.HighPart = -1;

    while (TRUE) {

        (VOID)ExAcquireResourceShared(&Resource,TRUE);

        DbgPrint("%lx with shared access\n", StartContext);

        if (ResourceCount >= 10) {
            ExReleaseResource(&Resource);
            break;
        }

        KeDelayExecutionThread ( KernelMode, FALSE, &Time);

        ExConvertSharedToExclusive(&Resource);

        DbgPrint("%lx Shared turned Exclusive access\n", StartContext);

        ResourceCount += 1;
        if (ResourceCount >= 10) {
            ExReleaseResource(&Resource);
            break;
        }

        KeDelayExecutionThread ( KernelMode, FALSE, &Time);

        ExConvertExclusiveToShared(&Resource);

        DbgPrint("%lx Exclusive turned Shared access\n", StartContext);

        if (ResourceCount >= 10) {
            ExReleaseResource(&Resource);
            break;
        }

        ExReleaseResource(&Resource);

        DbgPrint("%lx release Shared access\n", StartContext);

        KeDelayExecutionThread ( KernelMode, FALSE, &Time);
    }

    DbgPrint("Reader turned Writer %lx exiting\n", StartContext);

    KeReleaseSemaphore(&ResourceSemaphore, 0, 1, FALSE);
}

BOOLEAN
DoResourceTest( void )
{
    HANDLE Handles[32];
    ULONG i;

    DbgPrint("Start DoResourceTest...\n");

    ExInitializeResource(&Resource);
    ResourceCount = 0;

    KeInitializeSemaphore(&ResourceSemaphore, 0, MAXLONG);

    for (i = 0; i < 4; i += 1) {

        if (!NT_SUCCESS(PsCreateSystemThread(&Handles[i],
                                          0,
                                          NULL,
                                          0,
                                          NULL,
                                          Reader,
                                          (PVOID)i))) {

            DbgPrint("Create system thread error %8lx\n", i);
        }

    }

    for (i = 4; i < 6; i += 1) {

        if (!NT_SUCCESS(PsCreateSystemThread(&Handles[i],
                                          0,
                                          NULL,
                                          0,
                                          NULL,
                                          Writer,
                                          (PVOID)i))) {

            DbgPrint("Create system thread error %8lx\n", i);
        }

    }

    for (i = 6; i < 8; i += 1) {

        if (!NT_SUCCESS(PsCreateSystemThread(&Handles[i],
                                          0,
                                          NULL,
                                          0,
                                          NULL,
                                          ReaderTurnedWriter,
                                          (PVOID)i))) {

            DbgPrint("Create system thread error %8lx\n", i);
        }

    }

    DbgPrint("DoResourceTest wait for everyone to complete...\n");

    for (i = 0; i < 8; i += 1) {

        KeWaitForSingleObject( &ResourceSemaphore,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

    }

    DbgPrint("DoResourceTest Done\n");

    return( TRUE );
}

BOOLEAN
DoBitMapTest( void )
{
    ULONG Size;
    PRTL_BITMAP BitMap;

    DbgPrint("Start DoBitMapTest...\n");

    //
    //  First create a new bitmap
    //

    Size = sizeof(RTL_BITMAP) + (((2048*8 + 31) / 32) * 4);
    BitMap = (PRTL_BITMAP)(ExAllocatePool( NonPagedPool, Size ));
    RtlInitializeBitMap( BitMap, (PULONG)(BitMap+1), 2048*8 );

    //
    //  >>>> Test setting bits
    //

    //
    //  Now clear all bits
    //

    RtlClearAllBits( BitMap );

    //
    //  Now set some bit patterns, and test them
    //

    RtlSetBits( BitMap,   0,  1 );
    RtlSetBits( BitMap,  63,  1 );
    RtlSetBits( BitMap,  65, 30 );
    RtlSetBits( BitMap, 127,  2 );
    RtlSetBits( BitMap, 191, 34 );

    if ((BitMap->Buffer[0] != 0x00000001) ||
        (BitMap->Buffer[1] != 0x80000000) ||
        (BitMap->Buffer[2] != 0x7ffffffe) ||
        (BitMap->Buffer[3] != 0x80000000) ||
        (BitMap->Buffer[4] != 0x00000001) ||
        (BitMap->Buffer[5] != 0x80000000) ||
        (BitMap->Buffer[6] != 0xffffffff) ||
        (BitMap->Buffer[7] != 0x00000001)) {

        DbgPrint("RtlSetBits Error\n");
        return FALSE;
    }

    //
    //  Now test some RtlFindClearBitsAndSet
    //

    RtlSetAllBits( BitMap );

    RtlClearBits( BitMap, 0 +  10*32,  1 );
    RtlClearBits( BitMap, 5 +  11*32,  1 );
    RtlClearBits( BitMap, 7 +  12*32,  1 );

    RtlClearBits( BitMap, 0 +  13*32,  9 );
    RtlClearBits( BitMap, 4 +  14*32,  9 );
    RtlClearBits( BitMap, 7 +  15*32,  9 );

    RtlClearBits( BitMap, 0 +  16*32, 10 );
    RtlClearBits( BitMap, 4 +  17*32, 10 );
    RtlClearBits( BitMap, 6 +  18*32, 10 );
    RtlClearBits( BitMap, 7 +  19*32, 10 );

    RtlClearBits( BitMap, 0 + 110*32, 14 );
    RtlClearBits( BitMap, 1 + 111*32, 14 );
    RtlClearBits( BitMap, 2 + 112*32, 14 );

    RtlClearBits( BitMap, 0 + 113*32, 15 );
    RtlClearBits( BitMap, 1 + 114*32, 15 );
    RtlClearBits( BitMap, 2 + 115*32, 15 );

//    {
//        ULONG i;
//        for (i = 0; i < 16; i++) {
//            DbgPrint("%2d: %08lx\n", i, BitMap->Buffer[i]);
//        }
//    }

    if (RtlFindClearBitsAndSet( BitMap, 15, 0) != 0 + 113*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  0 + 113*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 15, 0) != 1 + 114*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  1 + 114*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 15, 0) != 2 + 115*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  2 + 115*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet( BitMap, 14, 0) != 0 + 110*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  0 + 110*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 14, 0) != 1 + 111*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  1 + 111*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 14, 0) != 2 + 112*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  2 + 112*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet( BitMap, 10, 0) != 0 + 16*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  0 + 16*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 10, 0) != 4 + 17*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  4 + 17*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 10, 0) != 6 + 18*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  6 + 18*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 10, 0) != 7 + 19*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  7 + 19*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet( BitMap, 9, 0) != 0 + 13*32) {
        DbgPrint("RtlFindClearBitsAndSet Error 0 + 13*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 9, 0) != 4 + 14*32) {
        DbgPrint("RtlFindClearBitsAndSet Error 4 + 14*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 9, 0) != 7 + 15*32) {
        DbgPrint("RtlFindClearBitsAndSet Error 7 + 15*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet( BitMap, 1, 0) != 0 + 10*32) {
        DbgPrint("RtlFindClearBitsAndSet Error 0 + 10*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 1, 0) != 5 + 11*32) {
        DbgPrint("RtlFindClearBitsAndSet Error 5 + 11*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 1, 0) != 7 + 12*32) {
        DbgPrint("RtlFindClearBitsAndSet Error 7 + 12*32\n");
        return FALSE;
    }

    //
    //  Now test some RtlFindClearBitsAndSet
    //

    RtlSetAllBits( BitMap );

    RtlClearBits( BitMap, 0 +  0*32,  1 );
    RtlClearBits( BitMap, 5 +  1*32,  1 );
    RtlClearBits( BitMap, 7 +  2*32,  1 );

    RtlClearBits( BitMap, 0 +  3*32,  9 );
    RtlClearBits( BitMap, 4 +  4*32,  9 );
    RtlClearBits( BitMap, 7 +  5*32,  9 );

    RtlClearBits( BitMap, 0 +  6*32, 10 );
    RtlClearBits( BitMap, 4 +  7*32, 10 );
    RtlClearBits( BitMap, 6 +  8*32, 10 );
    RtlClearBits( BitMap, 7 +  9*32, 10 );

    RtlClearBits( BitMap, 0 + 10*32, 14 );
    RtlClearBits( BitMap, 1 + 11*32, 14 );
    RtlClearBits( BitMap, 2 + 12*32, 14 );

    RtlClearBits( BitMap, 0 + 13*32, 15 );
    RtlClearBits( BitMap, 1 + 14*32, 15 );
    RtlClearBits( BitMap, 2 + 15*32, 15 );

//    {
//        ULONG i;
//        for (i = 0; i < 16; i++) {
//            DbgPrint("%2d: %08lx\n", i, BitMap->Buffer[i]);
//        }
//    }

    if (RtlFindClearBitsAndSet( BitMap, 15, 0) != 0 + 13*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  0 + 13*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 15, 0) != 1 + 14*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  1 + 14*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 15, 0) != 2 + 15*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  2 + 15*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet( BitMap, 14, 0) != 0 + 10*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  0 + 10*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 14, 0) != 1 + 11*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  1 + 11*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 14, 0) != 2 + 12*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  2 + 12*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet( BitMap, 10, 0) != 0 + 6*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  0 + 6*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 10, 0) != 4 + 7*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  4 + 7*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 10, 0) != 6 + 8*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  6 + 8*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 10, 0) != 7 + 9*32) {
        DbgPrint("RtlFindClearBitsAndSet Error  7 + 9*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet( BitMap, 9, 0) != 0 + 3*32) {
        DbgPrint("RtlFindClearBitsAndSet Error 0 + 3*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 9, 0) != 4 + 4*32) {
        DbgPrint("RtlFindClearBitsAndSet Error 4 + 4*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 9, 0) != 7 + 5*32) {
        DbgPrint("RtlFindClearBitsAndSet Error 7 + 5*32\n");
        return FALSE;
    }

    if (RtlFindClearBitsAndSet( BitMap, 1, 0) != 0 + 0*32) {
        DbgPrint("RtlFindClearBitsAndSet Error 0 + 0*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 1, 0) != 5 + 1*32) {
        DbgPrint("RtlFindClearBitsAndSet Error 5 + 1*32\n");
        return FALSE;
    }
    if (RtlFindClearBitsAndSet( BitMap, 1, 0) != 7 + 2*32) {
        DbgPrint("RtlFindClearBitsAndSet Error 7 + 2*32\n");
        return FALSE;
    }

    //
    //  >>>> Test clearing bits
    //

    //
    //  Now clear all bits
    //

    RtlSetAllBits( BitMap );

    //
    //  Now set some bit patterns, and test them
    //

    RtlClearBits( BitMap,   0,  1 );
    RtlClearBits( BitMap,  63,  1 );
    RtlClearBits( BitMap,  65, 30 );
    RtlClearBits( BitMap, 127,  2 );
    RtlClearBits( BitMap, 191, 34 );

    if ((BitMap->Buffer[0] != ~0x00000001) ||
        (BitMap->Buffer[1] != ~0x80000000) ||
        (BitMap->Buffer[2] != ~0x7ffffffe) ||
        (BitMap->Buffer[3] != ~0x80000000) ||
        (BitMap->Buffer[4] != ~0x00000001) ||
        (BitMap->Buffer[5] != ~0x80000000) ||
        (BitMap->Buffer[6] != ~0xffffffff) ||
        (BitMap->Buffer[7] != ~0x00000001)) {

        DbgPrint("RtlClearBits Error\n");
        return FALSE;
    }

    //
    //  Now test some RtlFindSetBitsAndClear
    //

    RtlClearAllBits( BitMap );

    RtlSetBits( BitMap, 0 +  0*32,  1 );
    RtlSetBits( BitMap, 5 +  1*32,  1 );
    RtlSetBits( BitMap, 7 +  2*32,  1 );

    RtlSetBits( BitMap, 0 +  3*32,  9 );
    RtlSetBits( BitMap, 4 +  4*32,  9 );
    RtlSetBits( BitMap, 7 +  5*32,  9 );

    RtlSetBits( BitMap, 0 +  6*32, 10 );
    RtlSetBits( BitMap, 4 +  7*32, 10 );
    RtlSetBits( BitMap, 6 +  8*32, 10 );
    RtlSetBits( BitMap, 7 +  9*32, 10 );

    RtlSetBits( BitMap, 0 + 10*32, 14 );
    RtlSetBits( BitMap, 1 + 11*32, 14 );
    RtlSetBits( BitMap, 2 + 12*32, 14 );

    RtlSetBits( BitMap, 0 + 13*32, 15 );
    RtlSetBits( BitMap, 1 + 14*32, 15 );
    RtlSetBits( BitMap, 2 + 15*32, 15 );

    {
        ULONG i;
        for (i = 0; i < 16; i++) {
            DbgPrint("%2d: %08lx\n", i, BitMap->Buffer[i]);
        }
    }

    if (RtlFindSetBitsAndClear( BitMap, 15, 0) != 0 + 13*32) {
        DbgPrint("RtlFindSetBitsAndClear Error  0 + 13*32\n");
        return FALSE;
    }
    if (RtlFindSetBitsAndClear( BitMap, 15, 0) != 1 + 14*32) {
        DbgPrint("RtlFindSetBitsAndClear Error  1 + 14*32\n");
        return FALSE;
    }
    if (RtlFindSetBitsAndClear( BitMap, 15, 0) != 2 + 15*32) {
        DbgPrint("RtlFindSetBitsAndClear Error  2 + 15*32\n");
        return FALSE;
    }

    if (RtlFindSetBitsAndClear( BitMap, 14, 0) != 0 + 10*32) {
        DbgPrint("RtlFindSetBitsAndClear Error  0 + 10*32\n");
        return FALSE;
    }
    if (RtlFindSetBitsAndClear( BitMap, 14, 0) != 1 + 11*32) {
        DbgPrint("RtlFindSetBitsAndClear Error  1 + 11*32\n");
        return FALSE;
    }
    if (RtlFindSetBitsAndClear( BitMap, 14, 0) != 2 + 12*32) {
        DbgPrint("RtlFindSetBitsAndClear Error  2 + 12*32\n");
        return FALSE;
    }

    if (RtlFindSetBitsAndClear( BitMap, 10, 0) != 0 + 6*32) {
        DbgPrint("RtlFindSetBitsAndClear Error  0 + 6*32\n");
        return FALSE;
    }
    if (RtlFindSetBitsAndClear( BitMap, 10, 0) != 4 + 7*32) {
        DbgPrint("RtlFindSetBitsAndClear Error  4 + 7*32\n");
        return FALSE;
    }
    if (RtlFindSetBitsAndClear( BitMap, 10, 0) != 6 + 8*32) {
        DbgPrint("RtlFindSetBitsAndClear Error  6 + 8*32\n");
        return FALSE;
    }
    if (RtlFindSetBitsAndClear( BitMap, 10, 0) != 7 + 9*32) {
        DbgPrint("RtlFindSetBitsAndClear Error  7 + 9*32\n");
        return FALSE;
    }

    if (RtlFindSetBitsAndClear( BitMap, 9, 0) != 0 + 3*32) {
        DbgPrint("RtlFindSetBitsAndClear Error 0 + 3*32\n");
        return FALSE;
    }
    if (RtlFindSetBitsAndClear( BitMap, 9, 0) != 4 + 4*32) {
        DbgPrint("RtlFindSetBitsAndClear Error 4 + 4*32\n");
        return FALSE;
    }
    if (RtlFindSetBitsAndClear( BitMap, 9, 0) != 7 + 5*32) {
        DbgPrint("RtlFindSetBitsAndClear Error 7 + 5*32\n");
        return FALSE;
    }

    if (RtlFindSetBitsAndClear( BitMap, 1, 0) != 0 + 0*32) {
        DbgPrint("RtlFindSetBitsAndClear Error 0 + 0*32\n");
        return FALSE;
    }
    if (RtlFindSetBitsAndClear( BitMap, 1, 0) != 5 + 1*32) {
        DbgPrint("RtlFindSetBitsAndClear Error 5 + 1*32\n");
        return FALSE;
    }
    if (RtlFindSetBitsAndClear( BitMap, 1, 0) != 7 + 2*32) {
        DbgPrint("RtlFindSetBitsAndClear Error 7 + 2*32\n");
        return FALSE;
    }

    DbgPrint("DoBitMapTest Done.\n");

    return TRUE;
}

BOOLEAN
ExTest (
    VOID
    )

{

    USHORT i;

    DbgPrint( "In extest\n" );
    for (i=1; i<16; i++) {
        if (i == TestEvent)
            DoEventTest();
        else
        if (i == TestHandle)
            DoHandleTest();
        else
        if (i == TestInfo)
            DoInfoTest();
        else
        if (i == TestLuid) {
            DoLuidTest();
            }
        else
        if (i == TestMemory) {
            DoMemoryTest();
            }
        else
        if (i == TestParty)
            DoPartyTest();
        else
        if (i == TestPool)
            DoPoolTest();
        else
        if (i == TestResource)
            DoResourceTest();
        else
        if (i == TestBitMap)
            DoBitMapTest();
        else
        if (i == TestSemaphore)
            DoSemaphoreTest();
        else
        if (i == TestTimer)
            DoTimerTest();
        else
        if (i == TestZone)
            DoZoneTest();
        else
        if (i == TestMutant)
            DoMutantTest();
        else
        if (i == TestException)
            DoExceptionTest();
        }

    TestFunction = NULL;    // Invoke the CLI
    return TRUE;
}
#ifndef MIPS

int
_CDECL
main(
    int argc,
    char *argv[]
    )
{
#ifdef SIMULATOR
    char c, *s;
    USHORT i;

    i = 1;
    if (argc > 1 ) {
        while (--argc) {
            s = *++argv;
            while ((c = *s++) != '\0') {
                switch (c) {
                case 'B':
                case 'b':
                    TestBitMap = i++;
                    break;

                case 'C':
                case 'c':
                    TestException = i++;
                    break;

                case 'E':
                case 'e':
                    TestEvent = i++;
                    break;

                case 'H':
                case 'h':
                    TestHandle = i++;
                    break;

                case 'I':
                case 'i':
                    TestInfo = i++;
                    break;

                case 'L':
                case 'l':
                    TestLuid = i++;
                    break;

                case 'M':
                case 'm':
                    TestMemory = i++;
                    break;

                case 'P':
                case 'p':
                    TestPool = i++;
                    break;

                case 'R':
                case 'r':
                    TestResource = i++;
                    break;

                case 'S':
                case 's':
                    TestSemaphore = i++;
                    break;

                case 'T':
                case 't':
                    TestTimer = i++;
                    break;

                case 'X':
                case 'x':
                    TestMutant = i++;
                    break;

                case 'Z':
                case 'z':
                    TestZone = i++;
                    break;

                default:
                    DbgPrint( "tex: invalid test code - '%s'", *argv );
                    break;
                }
            }
        }
    } else {
        if (!strcmp( "DAVEC", szVerUser )) {
            TestEvent = 1;
            TestSemaphore = 2;
            TestTimer = 3;
            TestMutant = 4;
            TestException = 5;
        }
        else
        if (!strcmp( "MARKL", szVerUser )) {
            TestPool = 1;
            TestZone = 2;
        }
        else
        if (!strcmp( "STEVEWO", szVerUser )) {
            TestInfo = 1;
            TestParty = 2;
            TestMemory = 3;
            TestHandle = 4;
        }
        else
        if (!strcmp( "GARYKI", szVerUser )) {
            TestResource = 1;
            TestMemory = 2;
            TestBitMap = 3;
        }
        else
        if (!strcmp( "JIMK", szVerUser )) {
            TestLuid = 1;
        }
        else {
            DbgPrint( "*** Warning *** - %s is an unauthorized user of tex\n",
                     szVerUser
                   );
        }
    }
#else
    TestEvent = 1;
    TestSemaphore = 2;
    TestTimer = 3;
    TestMutant = 4;
    TestException = 5;
#endif // SIMULATOR

    TestFunction = extest;
    KiSystemStartup();
    return 0;
}
#endif // MIPS

void
oops()
{
    ExTimerRundown();
}
