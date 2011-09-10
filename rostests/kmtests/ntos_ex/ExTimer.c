/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Executive Regressions KM-Test
 * PROGRAMMER:      Aleksey Bragin <aleksey@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

static
VOID
NTAPI
TestTimerApcRoutine(IN PVOID TimerContext,
                    IN ULONG TimerLowValue,
                    IN LONG TimerHighValue)

{
    ULONG *ApcCount = (ULONG *)TimerContext;
    DPRINT("Timer Apc called!\n");
    (*ApcCount)++;
}

START_TEST(ExTimer)
{
    UNICODE_STRING TimerName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE TimerHandle;
    HANDLE HandleOpened;
    LARGE_INTEGER DueTime;
    BOOLEAN PreviousState, CurrentState;
    NTSTATUS Status;
    ULONG ApcCount;

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
}
