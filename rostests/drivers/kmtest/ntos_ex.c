/*
 * NTOSKRNL Executive Regressions KM-Test
 * ReactOS Kernel Mode Regression Testing framework
 *
 * Copyright 2006 Aleksey Bragin <aleksey@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* INCLUDES *******************************************************************/

#include <ddk/ntddk.h>
#include <ntifs.h>
#include <ndk/ntndk.h>
#include "kmtest.h"

//#define NDEBUG
#include "debug.h"

/* PRIVATE FUNCTIONS ***********************************************************/

VOID
NTAPI
TestTimerApcRoutine(IN PVOID TimerContext,
                    IN ULONG TimerLowValue,
                    IN LONG TimerHighValue)

{
    DPRINT("Timer Apc called!\n");
    ULONG *ApcCount = (ULONG *)TimerContext;
    (*ApcCount)++;
}


VOID
ExTimerTest()
{
    UNICODE_STRING TimerName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE TimerHandle;
    HANDLE HandleOpened;
    LARGE_INTEGER DueTime;
    BOOLEAN PreviousState, CurrentState;
    NTSTATUS Status;
    ULONG ApcCount;

    StartTest();

    // Create the timer
    RtlInitUnicodeString(&TimerName, L"\\TestTimer");
    InitializeObjectAttributes(&ObjectAttributes, &TimerName, 0, NULL, NULL);
    Status = ZwCreateTimer(&TimerHandle, TIMER_ALL_ACCESS,
        &ObjectAttributes, NotificationTimer);
    ok(Status == STATUS_SUCCESS, "ZwCreateTimer failed with Status=0x%08lX", Status);

    // Open the timer
    Status = ZwOpenTimer(&HandleOpened, TIMER_ALL_ACCESS, &ObjectAttributes);
    ok(Status == STATUS_SUCCESS, "ZwOpenTimer failed with Status=0x%08lX", Status);

    // Set the timer, to some rather high value so it doesn't expire
    DPRINT("Set timer 1\n");
    DueTime.LowPart = -10000;
    DueTime.HighPart = -10;
    PreviousState = TRUE;
    Status = ZwSetTimer(HandleOpened, &DueTime, NULL, NULL, FALSE, 0L, &PreviousState);
    ok(Status == STATUS_SUCCESS, "ZwSetTimer failed with Status=0x%08lX", Status);
    ok(PreviousState == FALSE, "Incorrect PreviousState returned when setting the timer");

    // Cancel the timer
    CurrentState = TRUE;
    Status = ZwCancelTimer(HandleOpened, &CurrentState);
    ok(Status == STATUS_SUCCESS, "ZwCancelTimer failed with Status=0x%08lX", Status);
    ok(CurrentState == FALSE, "Incorrect CurrentState returned when canceling the timer");

    // Set the timer to some small value, because we'll wait for it to expire
    DPRINT("Set timer 2\n");
    DueTime.LowPart = -100;
    DueTime.HighPart = -1;
    PreviousState = TRUE;
    Status = ZwSetTimer(HandleOpened, &DueTime, NULL, NULL, FALSE, 0L, &PreviousState);
    ok(Status == STATUS_SUCCESS, "ZwSetTimer failed with Status=0x%08lX", Status);
    ok(PreviousState == FALSE, "Incorrect PreviousState returned when setting the timer");

    // Wait until it expires
    DPRINT("Wait till timer expires\n");
    Status = ZwWaitForSingleObject(HandleOpened, FALSE, NULL);
    ok(Status == STATUS_SUCCESS, "ZwWaitForSingleObject failed with Status=0x%08lX", Status);

    // And cancel it
    DPRINT("Cancel it\n");
    CurrentState = FALSE;
    Status = ZwCancelTimer(HandleOpened, &CurrentState);
    ok(Status == STATUS_SUCCESS, "ZwCancelTimer failed with Status=0x%08lX", Status);
    ok(CurrentState == TRUE, "Incorrect CurrentState returned when setting the timer");

    // Test it with APC: Set, Cancel, check if APC has been called
    DPRINT("Set timer with Apc (3)\n");
    ApcCount = 0;
    DueTime.LowPart = -10000;
    DueTime.HighPart = -10;
    PreviousState = FALSE;
    Status = ZwSetTimer(HandleOpened, &DueTime,
        (PTIMER_APC_ROUTINE)TestTimerApcRoutine, &ApcCount, FALSE,
        0L, &PreviousState);

    ok(Status == STATUS_SUCCESS, "ZwSetTimer failed with Status=0x%08lX", Status);
    ok(PreviousState == TRUE, "Incorrect PreviousState returned when setting the timer");

    DPRINT("Cancel it\n");
    CurrentState = TRUE;
    Status = ZwCancelTimer(HandleOpened, &CurrentState);
    ok(Status == STATUS_SUCCESS, "ZwCancelTimer failed with Status=0x%08lX", Status);
    ok(CurrentState == FALSE, "Incorrect CurrentState returned when cancelling the timer");
    ok(ApcCount == 0, "Incorrect count of TimerApcRoutine calls: %ld, should be 0\n", ApcCount);

    // Test setting the timer two times in a row, APC routine must not be called
    DPRINT("Set timer with Apc (4)\n");
    ApcCount = 0;
    DueTime.LowPart = -10000;
    DueTime.HighPart = -10;
    PreviousState = TRUE;
    Status = ZwSetTimer(HandleOpened, &DueTime,
        (PTIMER_APC_ROUTINE)TestTimerApcRoutine, &ApcCount, FALSE,
        0L, &PreviousState);
    ok(Status == STATUS_SUCCESS, "ZwSetTimer failed with Status=0x%08lX", Status);
    ok(PreviousState == FALSE, "Incorrect PreviousState returned when setting the timer");

    // Set small due time, since we have to wait for timer to finish
    DPRINT("Set timer with Apc (5)\n");
    DueTime.LowPart = -10;
    DueTime.HighPart = -1;
    PreviousState = TRUE;
    Status = ZwSetTimer(HandleOpened, &DueTime,
        (PTIMER_APC_ROUTINE)TestTimerApcRoutine, &ApcCount, FALSE,
        0L, &PreviousState);
    ok(Status == STATUS_SUCCESS, "ZwSetTimer failed with Status=0x%08lX", Status);
    ok(PreviousState == FALSE, "Incorrect PreviousState returned when setting the timer");

    // Now wait till it's finished, and then check APC call
    DPRINT("Wait for it\n");
    Status = ZwWaitForSingleObject(HandleOpened, FALSE, NULL);
    ok(Status == STATUS_SUCCESS, "ZwWaitForSingleObject failed with Status=0x%08lX", Status);

    CurrentState = FALSE;
    Status = ZwCancelTimer(HandleOpened, &CurrentState);
    ok(Status == STATUS_SUCCESS, "ZwCancelTimer failed with Status=0x%08lX", Status);
    ok(CurrentState == TRUE, "Incorrect CurrentState returned when cancelling the timer");
    ok(ApcCount == 1, "Incorrect count of TimerApcRoutine calls: %ld, should be 1\n", ApcCount);

    // Cleanup...
    Status = ZwClose(HandleOpened);
    ok(Status == STATUS_SUCCESS, "ZwClose failed with Status=0x%08lX", Status);

    Status = ZwClose(TimerHandle);
    ok(Status == STATUS_SUCCESS, "ZwClose failed with Status=0x%08lX", Status);

    FinishTest("NTOSKRNL Executive Timer");
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
FASTCALL
NtoskrnlExecutiveTests()
{
    ExTimerTest();
}
