/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/kdprint.c
 * PURPOSE:         KD64 Trap Handler Routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
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
    ULONG Length = Output->Length;

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

ULONG
NTAPI
KdpCommandString(IN ULONG Length,
                 IN LPSTR String,
                 IN KPROCESSOR_MODE PreviousMode,
                 IN PCONTEXT ContextRecord,
                 IN PKTRAP_FRAME TrapFrame,
                 IN PKEXCEPTION_FRAME ExceptionFrame)
{
    /* FIXME */
    return FALSE;
}

ULONG
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
    if ((PreviousMode != KernelMode) || (KdDebuggerNotPresent)) return 0;

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
    //KiRestoreProcessorControlState(&Prcb->ProcessorState);

    /* Exit the debugger and clear the CTRL-C state */
    KdExitDebugger(Entered);
    return 0;
}

ULONG
NTAPI
KdpPrompt(IN LPSTR InString,
          IN ULONG InStringLength,
          OUT LPSTR OutString,
          IN ULONG OutStringLength,
          IN KPROCESSOR_MODE PreviousMode,
          IN PKTRAP_FRAME TrapFrame,
          IN PKEXCEPTION_FRAME ExceptionFrame)
{
    /* FIXME */
    return FALSE;
}

ULONG
NTAPI
KdpPrint(IN ULONG ComponentId,
         IN ULONG ComponentMask,
         IN LPSTR String,
         IN ULONG Length,
         IN KPROCESSOR_MODE PreviousMode,
         IN PKTRAP_FRAME TrapFrame,
         IN PKEXCEPTION_FRAME ExceptionFrame,
         OUT PBOOLEAN Status)
{
    NTSTATUS ReturnValue;
    BOOLEAN Entered;
    ANSI_STRING AnsiString;

    /* Assume failure */
    *Status = FALSE;

    /* Validate the mask */
    if (ComponentMask <= 0x1F) ComponentMask = 1 << ComponentMask;
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
    AnsiString.Length = (USHORT)Length;

    /* Log the print */
    //KdLogDbgPrint(&AnsiString);

    /* Check for a debugger */
    if (KdDebuggerNotPresent)
    {
        /* Fail */
        *Status = TRUE;
        return (ULONG)STATUS_DEVICE_NOT_CONNECTED;
    }

    /* Enter the debugger */
    Entered = KdEnterDebugger(TrapFrame, ExceptionFrame);

    /* Print the string */
    if (KdpPrintString(&AnsiString))
    {
        /* User pressed CTRL-C, breakpoint on return */
        ReturnValue = STATUS_BREAKPOINT;
    }
    else
    {
        /* String was printed */
        ReturnValue = STATUS_SUCCESS;
    }

    /* Exit the debugger and return */
    KdExitDebugger(Entered);
    *Status = TRUE;
    return ReturnValue;
}

