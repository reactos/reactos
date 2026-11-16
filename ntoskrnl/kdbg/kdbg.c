/*
 * PROJECT:     ReactOS KDBG Kernel Debugger
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel Debugger Initialization
 * COPYRIGHT:   Copyright 2020-2021 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2021 Jérôme Gardou <jerome.gardou@reactos.org>
 *              Copyright 2023 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Hervé Poussineau <hpoussin@reactos.org>
 *              Jérôme Gardou <jerome.gardou@reactos.org>
 *              Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 *              Ryan Wilson <ryan@verosnt.com>
 *
 * PreviousMode for KDB enter conditions used SegCs & 1. App serial
 * output/ntdll-apps/serial-App-20261116T092124Z.log: user AV in
 * firefox/xul.dll entered KDB as last-chance and waited on Press q
 * with -serial file (no keyboard). Page-fault last is already kmode,
 * so SegCs RPL looked like kernel; when the faulting PC is still below
 * MmHighestUserAddress, treat PreviousMode as UserMode.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "kdb.h"

/* GLOBALS *******************************************************************/

static ULONG KdbgNextApiNumber = DbgKdContinueApi;
static CONTEXT KdbgContext;
static EXCEPTION_RECORD64 KdbgExceptionRecord;
static BOOLEAN KdbgFirstChanceException;
static NTSTATUS KdbgContinueStatus = STATUS_SUCCESS;

/* FUNCTIONS *****************************************************************/

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
        /* Call KdTerm */
        pKdSendPacket(PacketType, MessageHeader, MessageData, Context);
        return;
    }

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

            /* Check if this is an assertion failure */
            if (KdbgExceptionRecord.ExceptionCode == STATUS_ASSERTION_FAILURE)
            {
                /* Bump EIP to the instruction following the int 2C */
                KeSetContextPc(&KdbgContext, KeGetContextPc(&KdbgContext) + 2);
            }

            {
                KPROCESSOR_MODE PreviousMode;

                /*
                 * SegCs & MODE_MASK is the usual PreviousMode guess. When that
                 * bit is clear but the faulting PC is still a user address
                 * (serial-App-20261116T092124Z.log User AV in xul.dll), keep
                 * PreviousMode as UserMode so last-chance kmode does not enter
                 * KDB and block on Press q with -serial file.
                 */
                PreviousMode = (KdbgContext.SegCs & MODE_MASK) ? UserMode : KernelMode;
                if (PreviousMode == KernelMode &&
                    (ULONG_PTR)KeGetContextPc(&KdbgContext) <= (ULONG_PTR)MmHighestUserAddress)
                {
                    PreviousMode = UserMode;
                }

                Result = KdbEnterDebuggerException(&KdbgExceptionRecord,
                                                   PreviousMode,
                                                   &KdbgContext,
                                                   KdbgFirstChanceException);
            }
#if 0
            /* Manually dump the stack for the user */
            KeRosDumpStackFrames(NULL, 0);
            Result = kdHandleException;
#endif
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

    KdbPrintf("%s: PacketType %d is UNIMPLEMENTED\n", __FUNCTION__, PacketType);
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
    if (PacketType == PACKET_TYPE_KD_POLL_BREAKIN)
    {
        // FIXME TODO: Implement break-in for the debugger
        // and return KdPacketReceived when handled properly.
        return KdPacketTimedOut;
    }

    if (PacketType == PACKET_TYPE_KD_DEBUG_IO)
    {
        /* Call KdTerm */
        return pKdReceivePacket(PacketType,
                                MessageHeader,
                                MessageData,
                                DataLength,
                                Context);
    }

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
            KdbPrintf("%s:%d is UNIMPLEMENTED\n", __FUNCTION__, __LINE__);
        }
        ManipulateState->ApiNumber = DbgKdContinueApi;
        ManipulateState->u.Continue.ContinueStatus = KdbgContinueStatus;

        /* Prepare for next time */
        KdbgNextApiNumber = DbgKdContinueApi;
        KdbgContinueStatus = STATUS_SUCCESS;

        return KdPacketReceived;
    }

    KdbPrintf("%s: PacketType %d is UNIMPLEMENTED\n", __FUNCTION__, PacketType);
    return KdPacketTimedOut;
}

/* EOF */
