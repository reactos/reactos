/*
 * PROJECT:     ReactOS KDBG Kernel Debugger
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel Debugger Initialization
 * COPYRIGHT:   Copyright 2020 Hervé Poussineau <hpoussin@reactos.org>
******              Copyright 2021 Jérôme Gardou <jerome.gardou@reactos.org> ****
 *              Copyright 2023 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES ******************************************************************/

#if 0

#define NOEXTAPI
#include <ntifs.h>
#include <halfuncs.h>
#include <stdio.h>
#include <arc/arc.h>
#include <windbgkd.h>
#include <kddll.h>

#else

#include <ntoskrnl.h>

#endif

/* GLOBALS *******************************************************************/

KD_DISPATCH_TABLE KdbDispatchTable; // HACK

static ULONG KdbgNextApiNumber = DbgKdContinueApi;
static CONTEXT KdbgContext;
static EXCEPTION_RECORD64 KdbgExceptionRecord;
static BOOLEAN KdbgFirstChanceException;
static NTSTATUS KdbgContinueStatus = STATUS_SUCCESS;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
KdDebuggerInitialize0(
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock)
#undef KdDebuggerInitialize0
#define pKdDebuggerInitialize0 KdDebuggerInitialize0
{
    NTSTATUS Status;
    PCHAR CommandLine;

    /* Call KdTerm */
    Status = pKdDebuggerInitialize0(LoaderBlock);
    if (!NT_SUCCESS(Status))
        return Status;

    if (LoaderBlock)
    {
        /* Check if we have a command line */
        CommandLine = LoaderBlock->LoadOptions;
        if (CommandLine)
        {
            /* Upcase it */
            _strupr(CommandLine);

            /* Get the KDBG Settings */
            KdbpGetCommandLineSettings(CommandLine);
        }
    }

    return KdbInitialize(&KdbDispatchTable, 0);
}

NTSTATUS
NTAPI
KdDebuggerInitialize1(
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock)
#undef KdDebuggerInitialize1
#define pKdDebuggerInitialize1 KdDebuggerInitialize1
{
    NTSTATUS Status;

    /* Call KdTerm */
    Status = pKdDebuggerInitialize1(LoaderBlock);
    if (!NT_SUCCESS(Status))
        return Status;

    NtGlobalFlag |= FLG_STOP_ON_EXCEPTION;

    return KdbInitialize(&KdbDispatchTable, 1);
}

NTSTATUS
NTAPI
KdD0Transition(VOID)
#undef KdD0Transition
#define pKdD0Transition KdD0Transition
{
    /* Call KdTerm */
    return pKdD0Transition();
}

NTSTATUS
NTAPI
KdD3Transition(VOID)
#undef KdD3Transition
#define pKdD3Transition KdD3Transition
{
    /* Call KdTerm */
    return pKdD3Transition();
}

NTSTATUS
NTAPI
KdSave(
    _In_ BOOLEAN SleepTransition)
#undef KdSave
#define pKdSave KdSave
{
    /* Call KdTerm */
    return pKdSave(SleepTransition);
}

NTSTATUS
NTAPI
KdRestore(
    _In_ BOOLEAN SleepTransition)
#undef KdRestore
#define pKdRestore KdRestore
{
    /* Call KdTerm */
    return pKdRestore(SleepTransition);
}

VOID
NTAPI
KdSendPacket(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_opt_ PSTRING MessageData,
    _Inout_ PKD_CONTEXT Context)
#undef KdSendPacket
#define pKdSendPacket KdSendPacket
{
    if (PacketType == PACKET_TYPE_KD_DEBUG_IO)
    {
        ULONG ApiNumber = ((PDBGKD_DEBUG_IO)MessageHeader->Buffer)->ApiNumber;

        /* Validate API call */
        if (MessageHeader->Length != sizeof(DBGKD_DEBUG_IO))
            return;
        if ((ApiNumber != DbgKdPrintStringApi) &&
            (ApiNumber != DbgKdGetStringApi))
        {
            return;
        }
        if (!MessageData)
            return;

        /* IO packet: call KdTerm */
        pKdSendPacket(PacketType, MessageHeader, MessageData, Context);

        /* NOTE: MessageData->Length should be equal to
         * DebugIo.u.PrintString.LengthOfString, or to
         * DebugIo.u.GetString.LengthOfPromptString */

        /* Call also our debug print routine */
        KdbDebugPrint(MessageData->Buffer, MessageData->Length);
        return;
    }
    else
    /* Debugger-only packets */
    if (PacketType == PACKET_TYPE_KD_STATE_CHANGE64)
    {
        PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange = (PDBGKD_ANY_WAIT_STATE_CHANGE)MessageHeader->Buffer;
        if (WaitStateChange->NewState == DbgKdLoadSymbolsStateChange)
        {
            /* Load or unload symbols */
            PLDR_DATA_TABLE_ENTRY LdrEntry;
            if (KdbpSymFindModule((PVOID)(ULONG_PTR)WaitStateChange->u.LoadSymbols.BaseOfDll, -1, &LdrEntry))
            {
                KdbSymProcessSymbols(LdrEntry, !WaitStateChange->u.LoadSymbols.UnloadSymbols);
            }
            return;
        }
        else if (WaitStateChange->NewState == DbgKdExceptionStateChange)
        {
            KdbgNextApiNumber = DbgKdGetContextApi;
            KdbgExceptionRecord = WaitStateChange->u.Exception.ExceptionRecord;
            KdbgFirstChanceException = WaitStateChange->u.Exception.FirstChance;
            return;
        }
    }
    else if (PacketType == PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        PDBGKD_MANIPULATE_STATE64 ManipulateState = (PDBGKD_MANIPULATE_STATE64)MessageHeader->Buffer;
        if (ManipulateState->ApiNumber == DbgKdGetContextApi)
        {
            KD_CONTINUE_TYPE Result;

// #ifdef KDBG
            /* Check if this is an assertion failure */
            if (KdbgExceptionRecord.ExceptionCode == STATUS_ASSERTION_FAILURE)
            {
                /* Bump EIP to the instruction following the int 2C */
                KeSetContextPc(&KdbgContext, KeGetContextPc(&KdbgContext) + 2);
            }

            Result = KdbEnterDebuggerException(&KdbgExceptionRecord,
                                               KdbgContext.SegCs & 1,
                                               &KdbgContext,
                                               KdbgFirstChanceException);
// #else
            // /* We'll manually dump the stack for the user... */
            // KeRosDumpStackFrames(NULL, 0);
            // Result = kdHandleException;
// #endif
            if (Result != kdHandleException)
                KdbgContinueStatus = STATUS_SUCCESS;
            else
                KdbgContinueStatus = STATUS_UNSUCCESSFUL;
            KdbgNextApiNumber = DbgKdSetContextApi;
            return;
        }
        else if (ManipulateState->ApiNumber == DbgKdSetContextApi)
        {
            KdbgNextApiNumber = DbgKdContinueApi;
            return;
        }
    }
    return;
}

KDSTATUS
NTAPI
KdReceivePacket(
    _In_ ULONG PacketType,
    _Out_ PSTRING MessageHeader,
    _Out_ PSTRING MessageData,
    _Out_ PULONG DataLength,
    _Inout_ PKD_CONTEXT Context)
#undef KdReceivePacket
#define pKdReceivePacket KdReceivePacket
{
    if (PacketType == PACKET_TYPE_KD_DEBUG_IO)
    {
        extern STRING KdbPromptString;
        STRING NewLine = RTL_CONSTANT_STRING("\n");
        KDSTATUS Status;
        PDBGKD_DEBUG_IO DebugIo;

        DebugIo = (PDBGKD_DEBUG_IO)MessageHeader->Buffer;

        /* Validate API call */
        if (MessageHeader->MaximumLength != sizeof(DBGKD_DEBUG_IO))
            return KdPacketNeedsResend;
        if (DebugIo->ApiNumber != DbgKdGetStringApi)
            return KdPacketNeedsResend;

        /* The prompt string has been printed by KdSendPacket; go to
         * new line and print the kdb prompt -- for SYSREG2 support. */
        KdpPrintString(&NewLine);
        KdpPrintString(&KdbPromptString); // Alternatively, use "Input> "

        /* IO packet: call KdTerm */
        Status = pKdReceivePacket(PacketType, MessageHeader, MessageData, DataLength, Context);
        if (Status == KdPacketReceived)
        {
            // ASSERT(*DataLength == MessageData->Length);
            /* Mirror the answer to our debug print routine */
            KdbDebugPrint(MessageData->Buffer, MessageData->Length);
        }

        return Status;
    }
    else
    /* Debugger-only packets */
    if (PacketType == PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        PDBGKD_MANIPULATE_STATE64 ManipulateState = (PDBGKD_MANIPULATE_STATE64)MessageHeader->Buffer;
        RtlZeroMemory(MessageHeader->Buffer, MessageHeader->MaximumLength);
        if (KdbgNextApiNumber == DbgKdGetContextApi)
        {
            ManipulateState->ApiNumber = DbgKdGetContextApi;
            MessageData->Length = 0;
            MessageData->Buffer = (PCHAR)&KdbgContext;
            return KdPacketReceived;
        }
        else if (KdbgNextApiNumber == DbgKdSetContextApi)
        {
            ManipulateState->ApiNumber = DbgKdSetContextApi;
            MessageData->Length = sizeof(KdbgContext);
            MessageData->Buffer = (PCHAR)&KdbgContext;
            return KdPacketReceived;
        }
        else if (KdbgNextApiNumber != DbgKdContinueApi)
        {
            UNIMPLEMENTED;
        }
        ManipulateState->ApiNumber = DbgKdContinueApi;
        ManipulateState->u.Continue.ContinueStatus = KdbgContinueStatus;

        /* Prepare for next time */
        KdbgNextApiNumber = DbgKdContinueApi;
        KdbgContinueStatus = STATUS_SUCCESS;

        return KdPacketReceived;
    }

    UNIMPLEMENTED;
    return KdPacketTimedOut;
}

/* EOF */
