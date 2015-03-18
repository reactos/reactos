/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ps/win32.c
 * PURPOSE:         Process Manager: Win32K Initialization and Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

PKWIN32_PROCESS_CALLOUT PspW32ProcessCallout = NULL;
PKWIN32_THREAD_CALLOUT PspW32ThreadCallout = NULL;
PGDI_BATCHFLUSH_ROUTINE KeGdiFlushUserBatch = NULL;
extern PKWIN32_SESSION_CALLOUT ExpWindowStationObjectParse;
extern PKWIN32_SESSION_CALLOUT ExpWindowStationObjectDelete;
extern PKWIN32_SESSION_CALLOUT ExpWindowStationObjectOkToClose;
extern PKWIN32_SESSION_CALLOUT ExpDesktopObjectOkToClose;
extern PKWIN32_SESSION_CALLOUT ExpDesktopObjectDelete;
extern PKWIN32_SESSION_CALLOUT ExpDesktopObjectOpen;
extern PKWIN32_SESSION_CALLOUT ExpDesktopObjectClose;
extern PKWIN32_POWEREVENT_CALLOUT PopEventCallout;

/* PRIVATE FUNCTIONS *********************************************************/

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
    if (KeGetPreviousMode() == KernelMode) return STATUS_INVALID_PARAMETER;

    /* If no win32k, crashes later */
    ASSERT(PspW32ProcessCallout != NULL);

    /* Make sure win32k is here */
    if (!PspW32ProcessCallout) return STATUS_ACCESS_DENIED;

    /* Make sure it's not already win32 */
    if (Thread->Tcb.ServiceTable != KeServiceDescriptorTable)
    {
        /* We're already a win32 thread */
        return STATUS_ALREADY_WIN32;
    }

    /* Check if we don't already have a kernel-mode stack */
    if (!Thread->Tcb.LargeStack)
    {
        /* We don't create one */
        NewStack = (ULONG_PTR)MmCreateKernelStack(TRUE, 0);
        if (!NewStack)
        {
            /* Panic in user-mode */
            return STATUS_NO_MEMORY;
        }

        /* We're about to switch stacks. Enter a guarded region */
        KeEnterGuardedRegion();

        /* Switch stacks */
        OldStack = KeSwitchKernelStack((PVOID)NewStack,
                                       (PVOID)(NewStack - KERNEL_STACK_SIZE));

        /* Leave the guarded region */
        KeLeaveGuardedRegion();

        /* Delete the old stack */
        MmDeleteKernelStack(OldStack, FALSE);
    }

    /* Always do the process callout! */
    Status = PspW32ProcessCallout(Process, TRUE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Set the new service table */
    Thread->Tcb.ServiceTable = KeServiceDescriptorTableShadow;
    ASSERT(Thread->Tcb.Win32Thread == 0);

    /* Tell Win32k about our thread */
    Status = PspW32ThreadCallout(Thread, PsW32ThreadCalloutInitialize);
    if (!NT_SUCCESS(Status))
    {
        /* Revert our table */
        Thread->Tcb.ServiceTable = KeServiceDescriptorTable;
    }

    /* Return status */
    return Status;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
PsEstablishWin32Callouts(IN PWIN32_CALLOUTS_FPNS CalloutData)
{
    /* Setup the callback pointers */
    PspW32ProcessCallout = CalloutData->ProcessCallout;
    PspW32ThreadCallout = CalloutData->ThreadCallout;
    ExpWindowStationObjectParse = CalloutData->WindowStationParseProcedure;
    ExpWindowStationObjectDelete = CalloutData->WindowStationDeleteProcedure;
    ExpWindowStationObjectOkToClose = CalloutData->WindowStationOkToCloseProcedure;
    ExpDesktopObjectOkToClose = CalloutData->DesktopOkToCloseProcedure;
    ExpDesktopObjectDelete = CalloutData->DesktopDeleteProcedure;
    ExpDesktopObjectOpen = CalloutData->DesktopOpenProcedure;
    ExpDesktopObjectClose = CalloutData->DesktopCloseProcedure;
    PopEventCallout = CalloutData->PowerEventCallout;
    KeGdiFlushUserBatch = CalloutData->BatchFlushRoutine;
}

/* EOF */
