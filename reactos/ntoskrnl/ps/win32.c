/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/win32.c
 * PURPOSE:         win32k support
 *
 * PROGRAMMERS:     Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

PKWIN32_PROCESS_CALLOUT PspW32ProcessCallout = NULL;
PKWIN32_THREAD_CALLOUT PspW32ThreadCallout = NULL;
extern PKWIN32_PARSEMETHOD_CALLOUT ExpWindowStationObjectParse;
extern PKWIN32_DELETEMETHOD_CALLOUT ExpWindowStationObjectDelete;
extern PKWIN32_DELETEMETHOD_CALLOUT ExpDesktopObjectDelete;

#ifndef ALEX_CB_REWRITE
typedef struct _NTW32CALL_SAVED_STATE
{
  ULONG_PTR SavedStackLimit;
  PVOID SavedStackBase;
  PVOID SavedInitialStack;
  PVOID CallerResult;
  PULONG CallerResultLength;
  PNTSTATUS CallbackStatus;
  PKTRAP_FRAME SavedTrapFrame;
  PVOID SavedCallbackStack;
  PVOID SavedExceptionStack;
} NTW32CALL_SAVED_STATE, *PNTW32CALL_SAVED_STATE;
#endif

PVOID
STDCALL
KeSwitchKernelStack(
    IN PVOID StackBase,
    IN PVOID StackLimit
);

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
VOID 
STDCALL
PsEstablishWin32Callouts(PWIN32_CALLOUTS_FPNS CalloutData)
{
    PspW32ProcessCallout = CalloutData->ProcessCallout;
    PspW32ThreadCallout = CalloutData->ThreadCallout;
    ExpWindowStationObjectParse = CalloutData->WindowStationParseProcedure;
    ExpWindowStationObjectDelete = CalloutData->WindowStationDeleteProcedure;
    ExpDesktopObjectDelete = CalloutData->DesktopDeleteProcedure;
}

NTSTATUS
NTAPI
PsConvertToGuiThread(VOID)
{
    ULONG_PTR NewStack;
    PVOID OldStack;
    PETHREAD Thread = PsGetCurrentThread();
    PEPROCESS Process = PsGetCurrentProcess();
    NTSTATUS Status;
    PAGED_CODE();

    /* Validate the previous mode */
    if (KeGetPreviousMode() == KernelMode)
    {
        DPRINT1("Danger: win32k call being made in kernel-mode?!\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure win32k is here */
    if (!PspW32ProcessCallout)
    {
        DPRINT1("Danger: Win32K call attempted but Win32k not ready!\n");
        return STATUS_ACCESS_DENIED;
    }

    /* Make sure it's not already win32 */
    if (Thread->Tcb.ServiceTable != KeServiceDescriptorTable)
    {
        DPRINT1("Danger: Thread is already a win32 thread. Limit bypassed?\n");
        return STATUS_ALREADY_WIN32;
    }

    /* Check if we don't already have a kernel-mode stack */
    if (!Thread->Tcb.LargeStack)
    {
        /* We don't create one */
        NewStack = (ULONG_PTR)MmCreateKernelStack(TRUE) + KERNEL_LARGE_STACK_SIZE;
        if (!NewStack)
        {
            /* Panic in user-mode */
            NtCurrentTeb()->LastErrorValue = ERROR_NOT_ENOUGH_MEMORY;
            return STATUS_NO_MEMORY;
        }

        /* We're about to switch stacks. Enter a critical region */
        KeEnterCriticalRegion();

        /* Switch stacks */
        OldStack = KeSwitchKernelStack((PVOID)NewStack,
                                       (PVOID)(NewStack - KERNEL_STACK_SIZE));

        /* Leave the critical region */
        KeLeaveCriticalRegion();

        /* Delete the old stack */
        MmDeleteKernelStack(OldStack, FALSE);
    }

    /* This check is bizare. Check out win32k later */
    if (!Process->Win32Process)
    {
        /* Now tell win32k about us */
        Status = PspW32ProcessCallout(Process, TRUE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Danger: Win32k wasn't happy about us!\n");
            return Status;
        }
    }

    /* Set the new service table */
    Thread->Tcb.ServiceTable = KeServiceDescriptorTableShadow;
    ASSERT(Thread->Tcb.Win32Thread == 0);

    /* Tell Win32k about our thread */
    Status = PspW32ThreadCallout(Thread, PsW32ThreadCalloutInitialize);
    if (!NT_SUCCESS(Status))
    {
        /* Revert our table */
        DPRINT1("Danger: Win32k wasn't happy about us!\n");
        Thread->Tcb.ServiceTable = KeServiceDescriptorTable;
    }

    /* Return status */
    return Status;
}

NTSTATUS
STDCALL
NtW32Call(IN ULONG RoutineIndex,
          IN PVOID Argument,
          IN ULONG ArgumentLength,
          OUT PVOID* Result,
          OUT PULONG ResultLength)
{
    PVOID RetResult;
    ULONG RetResultLength;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("NtW32Call(RoutineIndex %d, Argument %p, ArgumentLength %d)\n",
            RoutineIndex, Argument, ArgumentLength);

    /* must not be called as KernelMode! */
    ASSERT(KeGetPreviousMode() != KernelMode);

    _SEH_TRY
    {
        ProbeForWritePointer(Result);
        ProbeForWriteUlong(ResultLength);
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if (NT_SUCCESS(Status))
    {
        /* Call kernel function */
        Status = KeUserModeCallback(RoutineIndex,
                                    Argument,
                                    ArgumentLength,
                                    &RetResult,
                                    &RetResultLength);

        if (NT_SUCCESS(Status))
        {
            _SEH_TRY
            {
                *Result = RetResult;
                *ResultLength = RetResultLength;
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
    }

    /* Return the result */
    return Status;
}

/* EOF */
