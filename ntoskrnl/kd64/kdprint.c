/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/kdprint.c
 * PURPOSE:         KD64 Trap Handler Routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define KD_PRINT_MAX_BYTES 512

/* FUNCTIONS *****************************************************************/

KIRQL
NTAPI
KdpAcquireLock(
    _In_ PKSPIN_LOCK SpinLock)
{
    KIRQL OldIrql;

    /* Acquire the spinlock without waiting at raised IRQL */
    while (TRUE)
    {
        /* Loop until the spinlock becomes available */
        while (!KeTestSpinLock(SpinLock));

        /* Spinlock is free, raise IRQL to high level */
        KeRaiseIrql(HIGH_LEVEL, &OldIrql);

        /* Try to get the spinlock */
        if (KeTryToAcquireSpinLockAtDpcLevel(SpinLock))
            break;

        /* Someone else got the spinlock, lower IRQL back */
        KeLowerIrql(OldIrql);
    }

    return OldIrql;
}

VOID
NTAPI
KdpReleaseLock(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ KIRQL OldIrql)
{
    /* Release the spinlock */
    KiReleaseSpinLock(SpinLock);
    // KeReleaseSpinLockFromDpcLevel(SpinLock);

    /* Restore the old IRQL */
    KeLowerIrql(OldIrql);
}

VOID
NTAPI
KdLogDbgPrint(
    _In_ PSTRING String)
{
    SIZE_T Length, Remaining;
    KIRQL OldIrql;

    /* If the string is empty, bail out */
    if (!String->Buffer || (String->Length == 0))
        return;

    /* If no log buffer available, bail out */
    if (!KdPrintCircularBuffer /*|| (KdPrintBufferSize == 0)*/)
        return;

    /* Acquire the log spinlock without waiting at raised IRQL */
    OldIrql = KdpAcquireLock(&KdpPrintSpinLock);

    Length = min(String->Length, KdPrintBufferSize);
    Remaining = KdPrintCircularBuffer + KdPrintBufferSize - KdPrintWritePointer;

    if (Length < Remaining)
    {
        KdpMoveMemory(KdPrintWritePointer, String->Buffer, Length);
        KdPrintWritePointer += Length;
    }
    else
    {
        KdpMoveMemory(KdPrintWritePointer, String->Buffer, Remaining);
        Length -= Remaining;
        if (Length > 0)
            KdpMoveMemory(KdPrintCircularBuffer, String->Buffer + Remaining, Length);

        KdPrintWritePointer = KdPrintCircularBuffer + Length;

        /* Got a rollover, update count (handle wrapping, must always be >= 1) */
        ++KdPrintRolloverCount;
        if (KdPrintRolloverCount == 0)
            ++KdPrintRolloverCount;
    }

    /* Release the spinlock */
    KdpReleaseLock(&KdpPrintSpinLock, OldIrql);
}

BOOLEAN
NTAPI
KdpPrintString(
    _In_ PSTRING Output)
{
    STRING Data, Header;
    DBGKD_DEBUG_IO DebugIo;
    USHORT Length;

    /* Copy the string */
    KdpMoveMemory(KdpMessageBuffer,
                  Output->Buffer,
                  Output->Length);

    /* Make sure we don't exceed the KD Packet size */
    Length = Output->Length;
    if ((sizeof(DBGKD_DEBUG_IO) + Length) > PACKET_MAX_SIZE)
    {
        /* Normalize length */
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_DEBUG_IO);
    }

    /* Build the packet header */
    DebugIo.ApiNumber = DbgKdPrintStringApi;
    DebugIo.ProcessorLevel = (USHORT)KeProcessorLevel;
    DebugIo.Processor = KeGetCurrentPrcb()->Number;
    DebugIo.u.PrintString.LengthOfString = Length;
    Header.Length = sizeof(DBGKD_DEBUG_IO);
    Header.Buffer = (PCHAR)&DebugIo;

    /* Build the data */
    Data.Length = Length;
    Data.Buffer = KdpMessageBuffer;

    /* Send the packet */
    KdSendPacket(PACKET_TYPE_KD_DEBUG_IO, &Header, &Data, &KdpContext);

    /* Check if the user pressed CTRL+C */
    return KdpPollBreakInWithPortLock();
}

BOOLEAN
NTAPI
KdpPromptString(
    _In_ PSTRING PromptString,
    _In_ PSTRING ResponseString)
{
    STRING Data, Header;
    DBGKD_DEBUG_IO DebugIo;
    ULONG Length;
    KDSTATUS Status;

    /* Copy the string to the message buffer */
    KdpMoveMemory(KdpMessageBuffer,
                  PromptString->Buffer,
                  PromptString->Length);

    /* Make sure we don't exceed the KD Packet size */
    Length = PromptString->Length;
    if ((sizeof(DBGKD_DEBUG_IO) + Length) > PACKET_MAX_SIZE)
    {
        /* Normalize length */
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_DEBUG_IO);
    }

    /* Build the packet header */
    DebugIo.ApiNumber = DbgKdGetStringApi;
    DebugIo.ProcessorLevel = (USHORT)KeProcessorLevel;
    DebugIo.Processor = KeGetCurrentPrcb()->Number;
    DebugIo.u.GetString.LengthOfPromptString = Length;
    DebugIo.u.GetString.LengthOfStringRead = ResponseString->MaximumLength;
    Header.Length = sizeof(DBGKD_DEBUG_IO);
    Header.Buffer = (PCHAR)&DebugIo;

    /* Build the data */
    Data.Length = Length;
    Data.Buffer = KdpMessageBuffer;

    /* Send the packet */
    KdSendPacket(PACKET_TYPE_KD_DEBUG_IO, &Header, &Data, &KdpContext);

    /* Set the maximum lengths for the receive */
    Header.MaximumLength = sizeof(DBGKD_DEBUG_IO);
    Data.MaximumLength = sizeof(KdpMessageBuffer);

    /* Enter receive loop */
    do
    {
        /* Get our reply */
        Status = KdReceivePacket(PACKET_TYPE_KD_DEBUG_IO,
                                 &Header,
                                 &Data,
                                 &Length,
                                 &KdpContext);

        /* Return TRUE if we need to resend */
        if (Status == KdPacketNeedsResend) return TRUE;

    /* Loop until we succeed */
    } while (Status != KdPacketReceived);

    /* Don't copy back a larger response than there is room for */
    Length = min(Length,
                 ResponseString->MaximumLength);

    /* Copy back the string and return the length */
    KdpMoveMemory(ResponseString->Buffer,
                  KdpMessageBuffer,
                  Length);
    ResponseString->Length = (USHORT)Length;

    /* Success; we don't need to resend */
    return FALSE;
}

VOID
NTAPI
KdpCommandString(IN PSTRING NameString,
                 IN PSTRING CommandString,
                 IN KPROCESSOR_MODE PreviousMode,
                 IN PCONTEXT ContextRecord,
                 IN PKTRAP_FRAME TrapFrame,
                 IN PKEXCEPTION_FRAME ExceptionFrame)
{
    BOOLEAN Enable;
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Check if we need to do anything */
    if ((PreviousMode != KernelMode) || (KdDebuggerNotPresent)) return;

    /* Enter the debugger */
    Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);

    /* Save the CPU Control State and save the context */
    KiSaveProcessorControlState(&Prcb->ProcessorState);
    KdpMoveMemory(&Prcb->ProcessorState.ContextFrame,
                  ContextRecord,
                  sizeof(CONTEXT));

    /* Send the command string to the debugger */
    KdpReportCommandStringStateChange(NameString,
                                      CommandString,
                                      &Prcb->ProcessorState.ContextFrame);

    /* Restore the processor state */
    KdpMoveMemory(ContextRecord,
                  &Prcb->ProcessorState.ContextFrame,
                  sizeof(CONTEXT));
    KiRestoreProcessorControlState(&Prcb->ProcessorState);

    /* Exit the debugger and return */
    KdExitDebugger(Enable);
}

VOID
NTAPI
KdpSymbol(IN PSTRING DllPath,
          IN PKD_SYMBOLS_INFO SymbolInfo,
          IN BOOLEAN Unload,
          IN KPROCESSOR_MODE PreviousMode,
          IN PCONTEXT ContextRecord,
          IN PKTRAP_FRAME TrapFrame,
          IN PKEXCEPTION_FRAME ExceptionFrame)
{
    BOOLEAN Enable;
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Check if we need to do anything */
    if ((PreviousMode != KernelMode) || (KdDebuggerNotPresent)) return;

    /* Enter the debugger */
    Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);

    /* Save the CPU Control State and save the context */
    KiSaveProcessorControlState(&Prcb->ProcessorState);
    KdpMoveMemory(&Prcb->ProcessorState.ContextFrame,
                  ContextRecord,
                  sizeof(CONTEXT));

    /* Report the new state */
    KdpReportLoadSymbolsStateChange(DllPath,
                                    SymbolInfo,
                                    Unload,
                                    &Prcb->ProcessorState.ContextFrame);

    /* Restore the processor state */
    KdpMoveMemory(ContextRecord,
                  &Prcb->ProcessorState.ContextFrame,
                  sizeof(CONTEXT));
    KiRestoreProcessorControlState(&Prcb->ProcessorState);

    /* Exit the debugger and return */
    KdExitDebugger(Enable);
}

USHORT
NTAPI
KdpPrompt(
    _In_reads_bytes_(PromptLength) PCHAR PromptString,
    _In_ USHORT PromptLength,
    _Out_writes_bytes_(MaximumResponseLength) PCHAR ResponseString,
    _In_ USHORT MaximumResponseLength,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ PKEXCEPTION_FRAME ExceptionFrame)
{
    STRING PromptBuffer, ResponseBuffer;
    BOOLEAN Enable, Resend;
    PCHAR SafeResponseString;
    CHAR CapturedPrompt[KD_PRINT_MAX_BYTES];
    CHAR SafeResponseBuffer[KD_PRINT_MAX_BYTES];

    /* Normalize the lengths */
    PromptLength = min(PromptLength,
                       sizeof(CapturedPrompt));
    MaximumResponseLength = min(MaximumResponseLength,
                                sizeof(SafeResponseBuffer));

    /* Check if we need to verify the string */
    if (PreviousMode != KernelMode)
    {
        /* Handle user-mode buffers safely */
        _SEH2_TRY
        {
            /* Probe and capture the prompt */
            ProbeForRead(PromptString, PromptLength, 1);
            KdpMoveMemory(CapturedPrompt, PromptString, PromptLength);
            PromptString = CapturedPrompt;

            /* Probe and make room for the response */
            ProbeForWrite(ResponseString, MaximumResponseLength, 1);
            SafeResponseString = SafeResponseBuffer;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Bad string pointer, bail out */
            _SEH2_YIELD(return 0);
        }
        _SEH2_END;
    }
    else
    {
        SafeResponseString = ResponseString;
    }

    /* Setup the prompt and response buffers */
    PromptBuffer.Buffer = PromptString;
    PromptBuffer.Length = PromptBuffer.MaximumLength = PromptLength;
    ResponseBuffer.Buffer = SafeResponseString;
    ResponseBuffer.Length = 0;
    ResponseBuffer.MaximumLength = MaximumResponseLength;

    /* Log the print */
    KdLogDbgPrint(&PromptBuffer);

    /* Enter the debugger */
    Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);

    /* Enter prompt loop */
    do
    {
        /* Send the prompt and receive the response */
        Resend = KdpPromptString(&PromptBuffer, &ResponseBuffer);

    /* Loop while we need to resend */
    } while (Resend);

    /* Exit the debugger */
    KdExitDebugger(Enable);

    /* Copy back the response if required */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Safely copy back the response to user mode */
            KdpMoveMemory(ResponseString,
                          ResponseBuffer.Buffer,
                          ResponseBuffer.Length);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* String became invalid after we exited, fail */
            _SEH2_YIELD(return 0);
        }
        _SEH2_END;
    }

    /* Return the number of characters received */
    return ResponseBuffer.Length;
}

static
NTSTATUS
NTAPI
KdpPrintFromUser(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_reads_bytes_(Length) PCHAR String,
    _In_ USHORT Length,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ PKEXCEPTION_FRAME ExceptionFrame,
    _Out_ PBOOLEAN Handled)
{
    CHAR CapturedString[KD_PRINT_MAX_BYTES];

    ASSERT(PreviousMode == UserMode);
    ASSERT(Length <= sizeof(CapturedString));

    /* Capture user-mode buffers */
    _SEH2_TRY
    {
        /* Probe and capture the string */
        ProbeForRead(String, Length, 1);
        KdpMoveMemory(CapturedString, String, Length);
        String = CapturedString;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Bad string pointer, bail out */
        _SEH2_YIELD(return STATUS_ACCESS_VIOLATION);
    }
    _SEH2_END;

    /* Now go through the kernel-mode code path */
    return KdpPrint(ComponentId,
                    Level,
                    String,
                    Length,
                    KernelMode,
                    TrapFrame,
                    ExceptionFrame,
                    Handled);
}

NTSTATUS
NTAPI
KdpPrint(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_reads_bytes_(Length) PCHAR String,
    _In_ USHORT Length,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ PKEXCEPTION_FRAME ExceptionFrame,
    _Out_ PBOOLEAN Handled)
{
    NTSTATUS Status;
    BOOLEAN Enable;
    STRING OutputString;

    if (NtQueryDebugFilterState(ComponentId, Level) == (NTSTATUS)FALSE)
    {
        /* Mask validation failed */
        *Handled = TRUE;
        return STATUS_SUCCESS;
    }

    /* Assume failure */
    *Handled = FALSE;

    /* Normalize the length */
    Length = min(Length, KD_PRINT_MAX_BYTES);

    /* Check if we need to verify the string */
    if (PreviousMode != KernelMode)
    {
        /* This case requires a 512 byte stack buffer.
         * We don't want to use that much stack in the kernel case, but we
         * can't use _alloca due to PSEH. So the buffer exists in this
         * helper function instead.
         */
        return KdpPrintFromUser(ComponentId,
                                Level,
                                String,
                                Length,
                                PreviousMode,
                                TrapFrame,
                                ExceptionFrame,
                                Handled);
    }

    /* Setup the output string */
    OutputString.Buffer = String;
    OutputString.Length = OutputString.MaximumLength = Length;

    /* Log the print */
    KdLogDbgPrint(&OutputString);

    /* Check for a debugger */
    if (KdDebuggerNotPresent)
    {
        /* Fail */
        *Handled = TRUE;
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    /* Enter the debugger */
    Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);

    /* Print the string */
    if (KdpPrintString(&OutputString))
    {
        /* User pressed CTRL-C, breakpoint on return */
        Status = STATUS_BREAKPOINT;
    }
    else
    {
        /* String was printed */
        Status = STATUS_SUCCESS;
    }

    /* Exit the debugger and return */
    KdExitDebugger(Enable);
    *Handled = TRUE;
    return Status;
}

VOID
__cdecl
KdpDprintf(
    _In_ PCHAR Format,
    ...)
{
    STRING String;
    USHORT Length;
    va_list ap;
    CHAR Buffer[512];

    /* Format the string */
    va_start(ap, Format);
    Length = (USHORT)_vsnprintf(Buffer,
                                sizeof(Buffer),
                                Format,
                                ap);
    va_end(ap);

    /* Set it up */
    String.Buffer = Buffer;
    String.Length = String.MaximumLength = Length;

    /* Send it to the debugger directly */
    KdpPrintString(&String);
}
