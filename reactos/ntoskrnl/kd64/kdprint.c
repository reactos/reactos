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

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
KdpPrintString(IN PSTRING Output)
{
    STRING Data, Header;
    DBGKD_DEBUG_IO DebugIo;
    USHORT Length = Output->Length;

    /* Copy the string */
    RtlMoveMemory(KdpMessageBuffer, Output->Buffer, Length);

    /* Make sure we don't exceed the KD Packet size */
    if ((sizeof(DBGKD_DEBUG_IO) + Length) > PACKET_MAX_SIZE)
    {
        /* Normalize length */
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_DEBUG_IO);
    }

    /* Build the packet header */
    DebugIo.ApiNumber = DbgKdPrintStringApi;
    DebugIo.ProcessorLevel = KeProcessorLevel;
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
KdpPromptString(IN PSTRING PromptString,
                IN PSTRING ResponseString)
{
    STRING Data, Header;
    DBGKD_DEBUG_IO DebugIo;
    ULONG Length = PromptString->Length;
    KDSTATUS Status;

    /* Copy the string to the message buffer */
    RtlCopyMemory(KdpMessageBuffer,
                  PromptString->Buffer,
                  PromptString->Length);

    /* Make sure we don't exceed the KD Packet size */
    if ((sizeof(DBGKD_DEBUG_IO) + Length) > PACKET_MAX_SIZE)
    {
        /* Normalize length */
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_DEBUG_IO);
    }

    /* Build the packet header */
    DebugIo.ApiNumber = DbgKdGetStringApi;
    DebugIo.ProcessorLevel = KeProcessorLevel;
    DebugIo.Processor = KeGetCurrentPrcb()->Number;
    DebugIo.u.GetString.LengthOfPromptString = Length;
    DebugIo.u.GetString.LengthOfStringRead = ResponseString->MaximumLength;
    Header.Length = sizeof(DBGKD_DEBUG_IO);
    Header.Buffer = (PCHAR)&DebugIo;

    /* Build the data */
    Data.Length = PromptString->Length;
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

    /* Don't copy back a larger respone than there is room foor */
    Length = min(Length, ResponseString->MaximumLength);

    /* Copy back the string and return the length */
    RtlCopyMemory(ResponseString->Buffer, KdpMessageBuffer, Length);
    ResponseString->Length = Length;

    /* Success; we don't need to resend */
    return FALSE;
}

VOID
NTAPI
KdpCommandString(IN ULONG Length,
                 IN LPSTR String,
                 IN KPROCESSOR_MODE PreviousMode,
                 IN PCONTEXT ContextRecord,
                 IN PKTRAP_FRAME TrapFrame,
                 IN PKEXCEPTION_FRAME ExceptionFrame)
{
    /* FIXME */
    KdpDprintf("KdpCommandString called\n");
    while (TRUE);
}

VOID
NTAPI
KdpSymbol(IN PSTRING DllPath,
          IN PKD_SYMBOLS_INFO DllBase,
          IN BOOLEAN Unload,
          IN KPROCESSOR_MODE PreviousMode,
          IN PCONTEXT ContextRecord,
          IN PKTRAP_FRAME TrapFrame,
          IN PKEXCEPTION_FRAME ExceptionFrame)
{
    BOOLEAN Entered;
    PKPRCB Prcb = KeGetCurrentPrcb();
    ULONG Status;

    /* Check if we need to do anything */
    if ((PreviousMode != KernelMode) || (KdDebuggerNotPresent)) return;

    /* Enter the debugger */
    Entered = KdEnterDebugger(TrapFrame, ExceptionFrame);

    /* Save the CPU Control State and save the context */
    KiSaveProcessorControlState(&Prcb->ProcessorState);
    RtlCopyMemory(&Prcb->ProcessorState.ContextFrame,
                  ContextRecord,
                  sizeof(CONTEXT));

    /* Report the new state */
    Status = KdpReportLoadSymbolsStateChange(DllPath,
                                             DllBase,
                                             Unload,
                                             &Prcb->ProcessorState.
                                             ContextFrame);

    /* Now restore the processor state, manually again. */
    RtlCopyMemory(ContextRecord,
                  &Prcb->ProcessorState.ContextFrame,
                  sizeof(CONTEXT));
    KiRestoreProcessorControlState(&Prcb->ProcessorState);

    /* Exit the debugger and clear the CTRL-C state */
    KdExitDebugger(Entered);
}

USHORT
NTAPI
KdpPrompt(IN LPSTR PromptString,
          IN USHORT PromptLength,
          OUT LPSTR ResponseString,
          IN USHORT MaximumResponseLength,
          IN KPROCESSOR_MODE PreviousMode,
          IN PKTRAP_FRAME TrapFrame,
          IN PKEXCEPTION_FRAME ExceptionFrame)
{
    STRING PromptBuffer, ResponseBuffer;
    BOOLEAN Entered, Resend;

    /* Normalize the lengths */
    PromptLength = min(PromptLength, 512);
    MaximumResponseLength = min(MaximumResponseLength, 512);

    /* Check if we need to verify the string */
    if (PreviousMode != KernelMode)
    {
        /* FIXME: Handle user-mode */
    }

    /* Setup the prompt and response  buffers */
    PromptBuffer.Buffer = PromptString;
    PromptBuffer.Length = PromptLength;
    ResponseBuffer.Buffer = ResponseString;
    ResponseBuffer.Length = 0;
    ResponseBuffer.MaximumLength = MaximumResponseLength;

    /* Log the print */
    //KdLogDbgPrint(&PromptBuffer);

    /* Enter the debugger */
    Entered = KdEnterDebugger(TrapFrame, ExceptionFrame);

    /* Enter prompt loop */
    do
    {
        /* Send the prompt and receive the response */
        Resend = KdpPromptString(&PromptBuffer, &ResponseBuffer);

    /* Loop while we need to resend */
    } while (Resend);

    /* Exit the debugger */
    KdExitDebugger(Entered);

    /* Return the number of characters received */
    return ResponseBuffer.Length;
}

NTSTATUS
NTAPI
KdpPrint(IN ULONG ComponentId,
         IN ULONG ComponentMask,
         IN LPSTR String,
         IN USHORT Length,
         IN KPROCESSOR_MODE PreviousMode,
         IN PKTRAP_FRAME TrapFrame,
         IN PKEXCEPTION_FRAME ExceptionFrame,
         OUT PBOOLEAN Status)
{
    NTSTATUS ReturnStatus;
    BOOLEAN Entered;
    ANSI_STRING AnsiString;

    /* Assume failure */
    *Status = FALSE;

    /* Validate the mask */
    if (ComponentMask < 0x20) ComponentMask = 1 << ComponentMask;
    if (!(Kd_WIN2000_Mask & ComponentMask) ||
        ((ComponentId < KdComponentTableSize) &&
        !(*KdComponentTable[ComponentId] & ComponentMask)))
    {
        /* Mask validation failed */
        *Status = TRUE;
        return FALSE;
    }

    /* Normalize the length */
    Length = min(Length, 512);

    /* Check if we need to verify the buffer */
    if (PreviousMode != KernelMode)
    {
        /* FIXME: Support user-mode */
    }

    /* Setup the ANSI string */
    AnsiString.Buffer = String;
    AnsiString.Length = Length;

    /* Log the print */
    //KdLogDbgPrint(&AnsiString);

    /* Check for a debugger */
    if (KdDebuggerNotPresent)
    {
        /* Fail */
        *Status = TRUE;
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    /* Enter the debugger */
    Entered = KdEnterDebugger(TrapFrame, ExceptionFrame);

    /* Print the string */
    if (KdpPrintString(&AnsiString))
    {
        /* User pressed CTRL-C, breakpoint on return */
        ReturnStatus = STATUS_BREAKPOINT;
    }
    else
    {
        /* String was printed */
        ReturnStatus = STATUS_SUCCESS;
    }

    /* Exit the debugger and return */
    KdExitDebugger(Entered);
    *Status = TRUE;
    return ReturnStatus;
}

VOID
__cdecl
KdpDprintf(IN PCHAR Format,
           ...)
{
    STRING String;
    CHAR Buffer[100];
    USHORT Length;
    va_list ap;

    /* Format the string */
    va_start(ap, Format);
    Length = (USHORT)_vsnprintf(Buffer,
                                sizeof(Buffer),
                                Format,
                                ap);

    /* Set it up */
    String.Buffer = Buffer;
    String.Length = Length + 1;

    /* Send it to the debugger directly */
    KdpPrintString(&String);
    va_end(ap);
}
