/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/except.c
 * PURPOSE:         Platform independent exception handling
 * PROGRAMMERS:     ReactOS Portable Systems Group
 *                  Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiContinuePreviousModeUser(
    _In_ PCONTEXT Context,
    _Out_ PKEXCEPTION_FRAME ExceptionFrame,
    _Out_ PKTRAP_FRAME TrapFrame)
{
    CONTEXT LocalContext;

    /* We'll have to make a copy and probe it */
    ProbeForRead(Context, sizeof(CONTEXT), sizeof(ULONG));
    RtlCopyMemory(&LocalContext, Context, sizeof(CONTEXT));
    Context = &LocalContext;

    /* Convert the context into Exception/Trap Frames */
    KeContextToTrapFrame(&LocalContext,
                         ExceptionFrame,
                         TrapFrame,
                         LocalContext.ContextFlags,
                         UserMode);
}

NTSTATUS
NTAPI
KiContinue(IN PCONTEXT Context,
           IN PKEXCEPTION_FRAME ExceptionFrame,
           IN PKTRAP_FRAME TrapFrame)
{
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL OldIrql = APC_LEVEL;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();

    /* Raise to APC_LEVEL, only if needed */
    if (KeGetCurrentIrql() < APC_LEVEL) KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Set up SEH to validate the context */
    _SEH2_TRY
    {
        /* Check the previous mode */
        if (PreviousMode != KernelMode)
        {
            /* Validate from user-mode */
            KiContinuePreviousModeUser(Context,
                                       ExceptionFrame,
                                       TrapFrame);
        }
        else
        {
            /* Convert the context into Exception/Trap Frames */
            KeContextToTrapFrame(Context,
                                 ExceptionFrame,
                                 TrapFrame,
                                 Context->ContextFlags,
                                 KernelMode);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Save the exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Lower the IRQL if needed */
    if (OldIrql < APC_LEVEL) KeLowerIrql(OldIrql);

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
KiRaiseException(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT Context,
    _Out_ PKEXCEPTION_FRAME ExceptionFrame,
    _Out_ PKTRAP_FRAME TrapFrame,
    _In_ BOOLEAN SearchFrames)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    CONTEXT LocalContext;
    EXCEPTION_RECORD LocalExceptionRecord;
    ULONG ParameterCount, Size;

    /* Check if we need to probe */
    if (PreviousMode != KernelMode)
    {
        /* Set up SEH */
        _SEH2_TRY
        {
            /* Probe the context */
            ProbeForRead(Context, sizeof(CONTEXT), sizeof(ULONG));

            /* Probe the Exception Record */
            ProbeForRead(ExceptionRecord,
                         FIELD_OFFSET(EXCEPTION_RECORD, NumberParameters) +
                         sizeof(ULONG),
                         sizeof(ULONG));

            /* Validate the maximum parameters */
            if ((ParameterCount = ExceptionRecord->NumberParameters) >
                EXCEPTION_MAXIMUM_PARAMETERS)
            {
                /* Too large */
                _SEH2_YIELD(return STATUS_INVALID_PARAMETER);
            }

            /* Probe the entire parameters now*/
            Size = (sizeof(EXCEPTION_RECORD) -
                    ((EXCEPTION_MAXIMUM_PARAMETERS - ParameterCount) * sizeof(ULONG)));
            ProbeForRead(ExceptionRecord, Size, sizeof(ULONG));

            /* Now make copies in the stack */
            RtlCopyMemory(&LocalContext, Context, sizeof(CONTEXT));
            RtlCopyMemory(&LocalExceptionRecord, ExceptionRecord, Size);
            Context = &LocalContext;
            ExceptionRecord = &LocalExceptionRecord;

            /* Update the parameter count */
            ExceptionRecord->NumberParameters = ParameterCount;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Don't fail silently */
            DPRINT1("KiRaiseException: Failed to Probe\n");

            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Convert the context record */
    KeContextToTrapFrame(Context,
                         ExceptionFrame,
                         TrapFrame,
                         Context->ContextFlags,
                         PreviousMode);

    /* Dispatch the exception */
    ExceptionRecord->ExceptionCode &= ~KI_EXCEPTION_INTERNAL;
    KiDispatchException(ExceptionRecord,
                        ExceptionFrame,
                        TrapFrame,
                        PreviousMode,
                        SearchFrames);

    /* We are done */
    return STATUS_SUCCESS;
}

/* SYSTEM CALLS ***************************************************************/

NTSTATUS
NTAPI
NtRaiseException(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT Context,
    _In_ BOOLEAN FirstChance)
{
    NTSTATUS Status;
    PKTHREAD Thread;
    PKTRAP_FRAME TrapFrame;
#ifdef _M_IX86
    PKEXCEPTION_FRAME ExceptionFrame = NULL;
#else
    KEXCEPTION_FRAME LocalExceptionFrame;
    PKEXCEPTION_FRAME ExceptionFrame = &LocalExceptionFrame;
#endif

    /* Get trap frame and link previous one */
    Thread = KeGetCurrentThread();
    TrapFrame = Thread->TrapFrame;
    Thread->TrapFrame = KiGetLinkedTrapFrame(TrapFrame);

    /* Set exception list */
#ifdef _M_IX86
    KeGetPcr()->NtTib.ExceptionList = TrapFrame->ExceptionList;
#endif

    /* Raise the exception */
    Status = KiRaiseException(ExceptionRecord,
                              Context,
                              ExceptionFrame,
                              TrapFrame,
                              FirstChance);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KiRaiseException failed. Status = 0x%lx\n", Status);
        return Status;
    }

    /* It was handled, so exit restoring all state */
    KiExceptionExit(TrapFrame, ExceptionFrame);
}

NTSTATUS
NTAPI
NtContinue(
    _In_ PCONTEXT Context,
    _In_ BOOLEAN TestAlert)
{
    PKTHREAD Thread;
    NTSTATUS Status;
    PKTRAP_FRAME TrapFrame;
#ifdef _M_IX86
    PKEXCEPTION_FRAME ExceptionFrame = NULL;
#else
    KEXCEPTION_FRAME LocalExceptionFrame;
    PKEXCEPTION_FRAME ExceptionFrame = &LocalExceptionFrame;
#endif

    /* Get trap frame and link previous one*/
    Thread = KeGetCurrentThread();
    TrapFrame = Thread->TrapFrame;
    Thread->TrapFrame = KiGetLinkedTrapFrame(TrapFrame);

    /* Continue from this point on */
    Status = KiContinue(Context, ExceptionFrame, TrapFrame);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KiContinue failed. Status = 0x%lx\n", Status);
        return Status;
    }

    /* Check if alert was requested */
    if (TestAlert)
    {
        KeTestAlertThread(Thread->PreviousMode);
    }

    /* Exit to new context */
    KiExceptionExit(TrapFrame, ExceptionFrame);
}

/* EOF */
