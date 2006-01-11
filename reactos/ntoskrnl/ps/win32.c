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

static PW32_PROCESS_CALLBACK PspWin32ProcessCallback = NULL;
static PW32_THREAD_CALLBACK PspWin32ThreadCallback = NULL;

extern OB_OPEN_METHOD ExpWindowStationObjectOpen;
extern OB_PARSE_METHOD ExpWindowStationObjectParse;
extern OB_DELETE_METHOD ExpWindowStationObjectDelete;
extern OB_FIND_METHOD ExpWindowStationObjectFind;
extern OB_CREATE_METHOD ExpDesktopObjectCreate;
extern OB_DELETE_METHOD ExpDesktopObjectDelete;

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
PsEstablishWin32Callouts(PW32_CALLOUT_DATA CalloutData)
{
    PspWin32ProcessCallback = CalloutData->W32ProcessCallout;
    PspWin32ThreadCallback = CalloutData->W32ThreadCallout;
    ExpWindowStationObjectOpen = CalloutData->WinStaCreate;
    ExpWindowStationObjectParse = CalloutData->WinStaParse;
    ExpWindowStationObjectDelete = CalloutData->WinStaDelete;
    ExpWindowStationObjectFind = CalloutData->WinStaFind;
    ExpDesktopObjectCreate = CalloutData->DesktopCreate;
    ExpDesktopObjectDelete = CalloutData->DesktopDelete;
}

NTSTATUS
NTAPI
PsConvertToGuiThread(VOID)
{
    PVOID NewStack, OldStack;
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
    if (!PspWin32ProcessCallback)
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
        NewStack = MmCreateKernelStack(TRUE);
        if (!NewStack)
        {
            /* Panic in user-mode */
            NtCurrentTeb()->LastErrorValue = ERROR_NOT_ENOUGH_MEMORY;
            return STATUS_NO_MEMORY;
        }

        /* We're about to switch stacks. Enter a critical region */
        KeEnterCriticalRegion();

        /* Switch stacks */
        OldStack = KeSwitchKernelStack((PVOID)((ULONG_PTR)NewStack + 0x3000),
                                       NewStack);

        /* Leave the critical region */
        KeLeaveCriticalRegion();

        /* Delete the old stack */
        //MmDeleteKernelStack(OldStack, FALSE);
    }

    /* This check is bizare. Check out win32k later */
    if (!Process->Win32Process)
    {
        /* Now tell win32k about us */
        Status = PspWin32ProcessCallback(Process, TRUE);
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
    Status = PspWin32ThreadCallback(Thread, TRUE);
    if (!NT_SUCCESS(Status))
    {
        /* Revert our table */
        DPRINT1("Danger: Win32k wasn't happy about us!\n");
        Thread->Tcb.ServiceTable = KeServiceDescriptorTable;
    }

    /* Return status */
    return Status;
}

VOID
NTAPI
PsTerminateWin32Process (PEPROCESS Process)
{
  if (Process->Win32Process == NULL)
    return;

  if (PspWin32ProcessCallback != NULL)
    {
      PspWin32ProcessCallback (Process, FALSE);
    }

  /* don't delete the W32PROCESS structure at this point, wait until the
     EPROCESS structure is being freed */
}


VOID
NTAPI
PsTerminateWin32Thread (PETHREAD Thread)
{
  if (Thread->Tcb.Win32Thread != NULL)
  {
    if (PspWin32ThreadCallback != NULL)
    {
      PspWin32ThreadCallback (Thread, FALSE);
    }

    /* don't delete the W32THREAD structure at this point, wait until the
       ETHREAD structure is being freed */
  }
}

NTSTATUS
STDCALL
NtW32Call(IN ULONG RoutineIndex,
          IN PVOID Argument,
          IN ULONG ArgumentLength,
          OUT PVOID* Result OPTIONAL,
          OUT PULONG ResultLength OPTIONAL)
{
    NTSTATUS CallbackStatus;

    DPRINT("NtW32Call(RoutineIndex %d, Argument %X, ArgumentLength %d)\n",
            RoutineIndex, Argument, ArgumentLength);

    /* FIXME: SEH!!! */

    /* Call kernel function */
    CallbackStatus = KeUserModeCallback(RoutineIndex,
                                        Argument,
                                        ArgumentLength,
                                        Result,
                                        ResultLength);

    /* Return the result */
    return(CallbackStatus);
}

/* EOF */
