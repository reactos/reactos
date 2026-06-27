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

/* PRIVATE FUNCTIONS ********************************************************/

#if DBG && defined(_M_IX86)
#define KDP_RAW_COM1_BASE 0x3F8
#define KDP_RAW_COM1_LINE_STATUS 5
#define KDP_RAW_COM1_TRANSMIT_EMPTY 0x20

static
VOID
NTAPI
KdpRawCom1WriteByte(
    _In_ UCHAR Character)
{
    ULONG SpinCount = 100000;

    while (SpinCount-- != 0)
    {
        if (READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)(KDP_RAW_COM1_BASE +
                                                KDP_RAW_COM1_LINE_STATUS)) &
            KDP_RAW_COM1_TRANSMIT_EMPTY)
            break;
    }

    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)KDP_RAW_COM1_BASE, Character);
}

static
VOID
NTAPI
KdpRawCom1WriteString(
    _In_z_ const CHAR *String)
{
    while (*String != ANSI_NULL)
    {
        if (*String == '\n')
            KdpRawCom1WriteByte('\r');

        KdpRawCom1WriteByte(*String++);
    }
}

static
VOID
NTAPI
KdpRawCom1WriteAnsiString(
    _In_opt_ PSTRING String)
{
    USHORT Index;

    if ((String == NULL) || (String->Buffer == NULL))
        return;

    for (Index = 0; Index < String->Length; Index++)
    {
        CHAR Character = String->Buffer[Index];
        KdpRawCom1WriteByte((Character >= 0x20 && Character < 0x7f) ?
                            (UCHAR)Character : (UCHAR)'?');
    }
}

static
VOID
NTAPI
KdpRawCom1WriteHex(
    _In_ ULONG_PTR Value)
{
    ULONG Index;

    for (Index = 0; Index < 8; Index++)
    {
        ULONG Nibble = (Value >> (28 - Index * 4)) & 0xF;

        KdpRawCom1WriteByte((UCHAR)(Nibble < 10 ? ('0' + Nibble) :
                                                ('A' + Nibble - 10)));
    }
}

static
VOID
NTAPI
KdpRawCom1WriteField(
    _In_z_ const CHAR *Name,
    _In_ ULONG_PTR Value)
{
    KdpRawCom1WriteByte(' ');
    KdpRawCom1WriteString(Name);
    KdpRawCom1WriteByte('=');
    KdpRawCom1WriteHex(Value);
}

static
VOID
NTAPI
KdpRawCom1DumpSymbolStage(
    _In_ ULONG Stage,
    _In_opt_ PSTRING DllPath,
    _In_opt_ PKD_SYMBOLS_INFO SymbolInfo,
    _In_ ULONG_PTR Detail)
{
    KdpRawCom1WriteString("\nKdpSym");
    KdpRawCom1WriteField("stage", Stage);
    KdpRawCom1WriteField("cpu", KeGetCurrentProcessorNumber());
    KdpRawCom1WriteField("irql", KeGetCurrentIrql());
    KdpRawCom1WriteField("info", (ULONG_PTR)SymbolInfo);
    KdpRawCom1WriteField("detail", Detail);
    KdpRawCom1WriteString(" name=");
    KdpRawCom1WriteAnsiString(DllPath);
    KdpRawCom1WriteByte('\n');
}
#endif

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

#if DBG && defined(_M_IX86)
static
BOOLEAN
NTAPI
KdpTryAcquirePrintLock(
    _In_ PKSPIN_LOCK SpinLock,
    _Out_ PKIRQL OldIrql)
{
    KiI386BootTraceRecord(0xD401,
                          (ULONG_PTR)SpinLock,
                          *SpinLock,
                          0,
                          0,
                          0,
                          0);

    if (!KeTestSpinLock(SpinLock))
    {
        KiI386BootTraceRecord(0xD402,
                              (ULONG_PTR)SpinLock,
                              *SpinLock,
                              0,
                              0,
                              0,
                              0);
        return FALSE;
    }

    KeRaiseIrql(HIGH_LEVEL, OldIrql);

    if (KeTryToAcquireSpinLockAtDpcLevel(SpinLock))
    {
        KiI386BootTraceRecord(0xD403,
                              (ULONG_PTR)SpinLock,
                              *SpinLock,
                              *OldIrql,
                              0,
                              0,
                              0);
        return TRUE;
    }

    KiI386BootTraceRecord(0xD404,
                          (ULONG_PTR)SpinLock,
                          *SpinLock,
                          *OldIrql,
                          0,
                          0,
                          0);
    KeLowerIrql(*OldIrql);
    return FALSE;
}
#endif

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

#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xD410,
                          (ULONG_PTR)String,
                          String->Length,
                          (ULONG_PTR)String->Buffer,
                          KdPrintBufferSize,
                          (ULONG_PTR)KdPrintWritePointer,
                          (ULONG_PTR)KdPrintCircularBuffer);
    if (!KdpTryAcquirePrintLock(&KdpPrintSpinLock, &OldIrql))
    {
        KiI386BootTraceRecord(0xD411,
                              (ULONG_PTR)&KdpPrintSpinLock,
                              KdpPrintSpinLock,
                              String->Length,
                              (ULONG_PTR)String->Buffer,
                              0,
                              0);
        return;
    }
#else
    /* Acquire the log spinlock without waiting at raised IRQL */
    OldIrql = KdpAcquireLock(&KdpPrintSpinLock);
#endif

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
#if DBG && defined(_M_IX86)
    KiI386BootTraceRecord(0xD412,
                          (ULONG_PTR)&KdpPrintSpinLock,
                          KdpPrintSpinLock,
                          String->Length,
                          (ULONG_PTR)String->Buffer,
                          0,
                          0);
#endif
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

#if DBG && defined(_M_IX86)
    KdpRawCom1DumpSymbolStage(0xD301,
                              DllPath,
                              SymbolInfo,
                              (ULONG_PTR)PreviousMode);
#endif

    /* Check if we need to do anything */
    if ((PreviousMode != KernelMode) || (KdDebuggerNotPresent))
    {
#if DBG && defined(_M_IX86)
        KdpRawCom1DumpSymbolStage(0xD302,
                                  DllPath,
                                  SymbolInfo,
                                  KdDebuggerNotPresent);
#endif
        return;
    }

    /* Enter the debugger */
#if DBG && defined(_M_IX86)
    KdpRawCom1DumpSymbolStage(0xD303,
                              DllPath,
                              SymbolInfo,
                              (ULONG_PTR)TrapFrame);
#endif
    Enable = KdEnterDebugger(TrapFrame, ExceptionFrame);
#if DBG && defined(_M_IX86)
    KdpRawCom1DumpSymbolStage(0xD304,
                              DllPath,
                              SymbolInfo,
                              Enable);
#endif

    /* Save the CPU Control State and save the context */
    KiSaveProcessorControlState(&Prcb->ProcessorState);
#if DBG && defined(_M_IX86)
    KdpRawCom1DumpSymbolStage(0xD305,
                              DllPath,
                              SymbolInfo,
                              (ULONG_PTR)&Prcb->ProcessorState);
#endif
    KdpMoveMemory(&Prcb->ProcessorState.ContextFrame,
                  ContextRecord,
                  sizeof(CONTEXT));

    /* Report the new state */
#if DBG && defined(_M_IX86)
    KdpRawCom1DumpSymbolStage(0xD306,
                              DllPath,
                              SymbolInfo,
                              Unload);
#endif
    KdpReportLoadSymbolsStateChange(DllPath,
                                    SymbolInfo,
                                    Unload,
                                    &Prcb->ProcessorState.ContextFrame);
#if DBG && defined(_M_IX86)
    KdpRawCom1DumpSymbolStage(0xD307,
                              DllPath,
                              SymbolInfo,
                              Unload);
#endif

    /* Restore the processor state */
    KdpMoveMemory(ContextRecord,
                  &Prcb->ProcessorState.ContextFrame,
                  sizeof(CONTEXT));
    KiRestoreProcessorControlState(&Prcb->ProcessorState);
#if DBG && defined(_M_IX86)
    KdpRawCom1DumpSymbolStage(0xD308,
                              DllPath,
                              SymbolInfo,
                              (ULONG_PTR)ContextRecord);
#endif

    /* Exit the debugger and return */
    KdExitDebugger(Enable);
#if DBG && defined(_M_IX86)
    KdpRawCom1DumpSymbolStage(0xD309,
                              DllPath,
                              SymbolInfo,
                              Enable);
#endif
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
    _In_ PCSTR Format,
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
