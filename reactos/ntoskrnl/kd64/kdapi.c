/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/kdapi.c
 * PURPOSE:         KD64 Public Routines and Internal Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
KdpQueryMemory(IN PDBGKD_MANIPULATE_STATE64 State,
               IN PCONTEXT Context)
{
    PDBGKD_QUERY_MEMORY Memory = &State->u.QueryMemory;
    STRING Header;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Validate the address space */
    if (Memory->AddressSpace == DBGKD_QUERY_MEMORY_VIRTUAL)
    {
        /* Check if this is process memory */
        if ((PVOID)(LONG_PTR)Memory->Address < MmHighestUserAddress)
        {
            /* It is */
            Memory->AddressSpace = DBGKD_QUERY_MEMORY_PROCESS;
        }
        else
        {
            /* FIXME: Check if it's session space */
            Memory->AddressSpace = DBGKD_QUERY_MEMORY_KERNEL;
        }

        /* Set flags */
        Memory->Flags = DBGKD_QUERY_MEMORY_READ |
                        DBGKD_QUERY_MEMORY_WRITE |
                        DBGKD_QUERY_MEMORY_EXECUTE;
    }
    else
    {
        /* Invalid */
        Status = STATUS_INVALID_PARAMETER;
    }

    /* Return structure */
    State->ReturnStatus = Status;
    Memory->Reserved = 0;

    /* Build header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;

    /* Send the packet */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
                 &KdpContext);
}

VOID
NTAPI
KdpWriteBreakpoint(IN PDBGKD_MANIPULATE_STATE64 State,
                   IN PSTRING Data,
                   IN PCONTEXT Context)
{
    PDBGKD_WRITE_BREAKPOINT64 Breakpoint = &State->u.WriteBreakPoint;
    STRING Header;

    /* Build header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == 0);

    /* Create the breakpoint */
    Breakpoint->BreakPointHandle =
        KdpAddBreakpoint((PVOID)(LONG_PTR)Breakpoint->BreakPointAddress);
    if (!Breakpoint->BreakPointHandle)
    {
        /* We failed */
        State->ReturnStatus = STATUS_UNSUCCESSFUL;
    }
    else
    {
        /* Success! */
        State->ReturnStatus = STATUS_SUCCESS;
    }

    /* Send the packet */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
                 &KdpContext);
}

VOID
NTAPI
DumpTraceData(OUT PSTRING TraceData)
{
    /* Update the buffer */
    TraceDataBuffer[0] = TraceDataBufferPosition;

    /* Setup the trace data */
    TraceData->Length = TraceDataBufferPosition * sizeof(ULONG);
    TraceData->Buffer = (PCHAR)TraceDataBuffer;

    /* Reset the buffer location */
    TraceDataBufferPosition = 1;
}

VOID
NTAPI
KdpGetStateChange(IN PDBGKD_MANIPULATE_STATE64 State,
                  IN PCONTEXT Context)
{
    PKPRCB Prcb;
    ULONG i;

    /* Check for success */
    if (NT_SUCCESS(State->u.Continue2.ContinueStatus))
    {
        /* Check if we're tracing */
        if (State->u.Continue2.ControlSet.TraceFlag)
        {
            /* Enable TF */
            Context->EFlags |= EFLAGS_TF;
        }
        else
        {
            /* Remove it */
            Context->EFlags &= ~EFLAGS_TF;
        }

        /* Loop all processors */
        for (i = 0; i < KeNumberProcessors; i++)
        {
            /* Get the PRCB and update DR7 and DR6 */
            Prcb = KiProcessorBlock[i];
            Prcb->ProcessorState.SpecialRegisters.KernelDr7 =
                State->u.Continue2.ControlSet.Dr7;
            Prcb->ProcessorState.SpecialRegisters.KernelDr6 = 0;
        }

        /* Check if we have new symbol information */
        if (State->u.Continue2.ControlSet.CurrentSymbolStart != 1)
        {
            /* Update it */
            KdpCurrentSymbolStart =
                State->u.Continue2.ControlSet.CurrentSymbolStart;
            KdpCurrentSymbolEnd= State->u.Continue2.ControlSet.CurrentSymbolEnd;
        }
    }
}

VOID
NTAPI
KdpSetCommonState(IN ULONG NewState,
                  IN PCONTEXT Context,
                  OUT PDBGKD_WAIT_STATE_CHANGE64 WaitStateChange)
{
    USHORT InstructionCount;
    BOOLEAN HadBreakpoints;

    /* Setup common stuff available for all CPU architectures */
    WaitStateChange->NewState = NewState;
    WaitStateChange->ProcessorLevel = KeProcessorLevel;
    WaitStateChange->Processor = (USHORT)KeGetCurrentPrcb()->Number;
    WaitStateChange->NumberProcessors = (ULONG)KeNumberProcessors;
    WaitStateChange->Thread = (ULONG)(LONG_PTR)KeGetCurrentThread();
#if defined(_M_X86_)
    WaitStateChange->ProgramCounter = (ULONG)(LONG_PTR)Context->Eip;
#elif defined(_M_AMD64)
    WaitStateChange->ProgramCounter = (LONG_PTR)Context->Rip;
#else
#error Unknown platform
#endif

    /* Zero out the Control Report */
    RtlZeroMemory(&WaitStateChange->ControlReport,
                  sizeof(DBGKD_CONTROL_REPORT));

    /* Now copy the instruction stream and set the count */
    RtlCopyMemory(&WaitStateChange->ControlReport.InstructionStream[0],
                  (PVOID)(ULONG_PTR)WaitStateChange->ProgramCounter,
                  DBGKD_MAXSTREAM);
    InstructionCount = DBGKD_MAXSTREAM;
    WaitStateChange->ControlReport.InstructionCount = InstructionCount;

    /* Clear all the breakpoints in this region */
    HadBreakpoints =
        KdpDeleteBreakpointRange((PVOID)(LONG_PTR)WaitStateChange->ProgramCounter,
                                 (PVOID)((ULONG_PTR)WaitStateChange->ProgramCounter +
                                         WaitStateChange->ControlReport.InstructionCount - 1));
    if (HadBreakpoints)
    {
        /* Copy the instruction stream again, this time without breakpoints */
        RtlCopyMemory(&WaitStateChange->ControlReport.InstructionStream[0],
                      (PVOID)(ULONG_PTR)WaitStateChange->ProgramCounter,
                      WaitStateChange->ControlReport.InstructionCount);
    }
}

VOID
NTAPI
KdpSysGetVersion(IN PDBGKD_GET_VERSION64 Version)
{
    /* Copy the version block */
    RtlCopyMemory(Version, &KdVersionBlock, sizeof(DBGKD_GET_VERSION64));
}

VOID
NTAPI
KdpGetVersion(IN PDBGKD_MANIPULATE_STATE64 State)
{
    STRING Header;

    /* Fill out the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;

    /* Get the version block */
    KdpSysGetVersion(&State->u.GetVersion64);

    /* Fill out the state */
    State->ApiNumber = DbgKdGetVersionApi;
    State->ReturnStatus = STATUS_SUCCESS;

    /* Send the packet */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
                 &KdpContext);
}

VOID
NTAPI
KdpReadVirtualMemory(IN PDBGKD_MANIPULATE_STATE64 State,
                     IN PSTRING Data,
                     IN PCONTEXT Context)
{
    STRING Header;
    ULONG Length = State->u.ReadMemory.TransferCount;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Validate length */
    if (Length > (PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64)))
    {
        /* Overflow, set it to maximum possible */
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64);
    }

#if 0
    if (!MmIsAddressValid((PVOID)(ULONG_PTR)State->u.ReadMemory.TargetBaseAddress))
    {
        Ke386SetCr2(State->u.ReadMemory.TargetBaseAddress);
        while (TRUE);
    }
#endif

    if (!State->u.ReadMemory.TargetBaseAddress)
    {
        Length = 0;
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        RtlCopyMemory(Data->Buffer,
                      (PVOID)(ULONG_PTR)State->u.ReadMemory.TargetBaseAddress,
                      Length);
    }

    /* Fill out the header */
    Data->Length = Length;
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;

    /* Fill out the state */
    State->ReturnStatus = Status;
    State->u.ReadMemory.ActualBytesRead = Length;

    /* Send the packet */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 Data,
                 &KdpContext);
}

VOID
NTAPI
KdpReadControlSpace(IN PDBGKD_MANIPULATE_STATE64 State,
                    IN PSTRING Data,
                    IN PCONTEXT Context)
{
    PDBGKD_READ_MEMORY64 ReadMemory = &State->u.ReadMemory;
    STRING Header;
    ULONG Length, RealLength;
    PVOID ControlStart;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == 0);

    /* Check the length requested */
    Length = ReadMemory->TransferCount;
    if (Length > (PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64)))
    {
        /* Use maximum allowed */
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64);
    }

#if defined (_M_AMD64)
    if ((ULONG)ReadMemory->TargetBaseAddress <= 2)
    {
        switch ((ULONG_PTR)ReadMemory->TargetBaseAddress)
        {
            case 1:
                ControlStart = &KiProcessorBlock[State->Processor];
                RealLength = sizeof(PVOID);
                break;

            case 2:
                ControlStart = &KiProcessorBlock[State->Processor]->
                                         ProcessorState.SpecialRegisters;
                RealLength = sizeof(KSPECIAL_REGISTERS);
                break;

            default:
                ControlStart = NULL;
                ASSERT(FALSE);
        }

        if (RealLength < Length) Length = RealLength;

        /* Copy the memory */
        RtlCopyMemory(Data->Buffer, ControlStart, Length);
        Data->Length = Length;

        /* Finish up */
        State->ReturnStatus = STATUS_SUCCESS;
        ReadMemory->ActualBytesRead = Data->Length;
    }
#else
    /* Make sure that this is a valid request */
    if (((ULONG)ReadMemory->TargetBaseAddress < sizeof(KPROCESSOR_STATE)) &&
        (State->Processor < KeNumberProcessors))
    {
        /* Get the actual length */
        RealLength = sizeof(KPROCESSOR_STATE) -
                     (ULONG_PTR)ReadMemory->TargetBaseAddress;
        if (RealLength < Length) Length = RealLength;

        /* Set the proper address */
        ControlStart = (PVOID)((ULONG_PTR)ReadMemory->TargetBaseAddress +
                               (ULONG_PTR)&KiProcessorBlock[State->Processor]->
                                           ProcessorState);

        /* Copy the memory */
        RtlCopyMemory(Data->Buffer, ControlStart, Length);
        Data->Length = Length;

        /* Finish up */
        State->ReturnStatus = STATUS_SUCCESS;
        ReadMemory->ActualBytesRead = Data->Length;
    }
#endif
    else
    {
        /* Invalid request */
        Data->Length = 0;
        State->ReturnStatus = STATUS_UNSUCCESSFUL;
        ReadMemory->ActualBytesRead = 0;
    }

    /* Send the reply */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 Data,
                 &KdpContext);
}

VOID
NTAPI
KdpWriteControlSpace(IN PDBGKD_MANIPULATE_STATE64 State,
                     IN PSTRING Data,
                     IN PCONTEXT Context)
{
    PDBGKD_WRITE_MEMORY64 WriteMemory = &State->u.WriteMemory;
    STRING Header;
    ULONG Length;
    PVOID ControlStart;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;

    /* Make sure that this is a valid request */
    Length = WriteMemory->TransferCount;
    if ((((ULONG)WriteMemory->TargetBaseAddress + Length) <=
          sizeof(KPROCESSOR_STATE)) &&
        (State->Processor < KeNumberProcessors))
    {
        /* Set the proper address */
        ControlStart = (PVOID)((ULONG_PTR)WriteMemory->TargetBaseAddress +
                               (ULONG_PTR)&KiProcessorBlock[State->Processor]->
                                           ProcessorState);

        /* Copy the memory */
        RtlCopyMemory(ControlStart, Data->Buffer, Data->Length);
        Length = Data->Length;

        /* Finish up */
        State->ReturnStatus = STATUS_SUCCESS;
        WriteMemory->ActualBytesWritten = Length;
    }
    else
    {
        /* Invalid request */
        Data->Length = 0;
        State->ReturnStatus = STATUS_UNSUCCESSFUL;
        WriteMemory->ActualBytesWritten = 0;
    }

    /* Send the reply */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 Data,
                 &KdpContext);
}

VOID
NTAPI
KdpRestoreBreakpoint(IN PDBGKD_MANIPULATE_STATE64 State,
                     IN PSTRING Data,
                     IN PCONTEXT Context)
{
    PDBGKD_RESTORE_BREAKPOINT RestoreBp = &State->u.RestoreBreakPoint;
    STRING Header;

    /* Fill out the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == 0);

    /* Get the version block */
    if (KdpDeleteBreakpoint(RestoreBp->BreakPointHandle))
    {
        /* We're all good */
        State->ReturnStatus = STATUS_SUCCESS;
    }
    else
    {
        /* We failed */
        State->ReturnStatus = STATUS_UNSUCCESSFUL;
    }

    /* Send the packet */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
                 &KdpContext);
}

VOID
NTAPI
KdpGetContext(IN PDBGKD_MANIPULATE_STATE64 State,
              IN PSTRING Data,
              IN PCONTEXT Context)
{
    STRING Header;
    PVOID ControlStart;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == 0);

    /* Make sure that this is a valid request */
    if (State->Processor < KeNumberProcessors)
    {
        /* Check if the request is for this CPU */
        if (State->Processor == KeGetCurrentPrcb()->Number)
        {
            /* We're just copying our own context */
            ControlStart = Context;
        }
        else
        {
            /* SMP not yet handled */
            ControlStart = NULL;
            while (TRUE);
        }

        /* Copy the memory */
        RtlCopyMemory(Data->Buffer, ControlStart, sizeof(CONTEXT));
        Data->Length = sizeof(CONTEXT);

        /* Finish up */
        State->ReturnStatus = STATUS_SUCCESS;
    }
    else
    {
        /* Invalid request */
        State->ReturnStatus = STATUS_UNSUCCESSFUL;
    }

    /* Send the reply */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 Data,
                 &KdpContext);
}

VOID
NTAPI
KdpSetContext(IN PDBGKD_MANIPULATE_STATE64 State,
              IN PSTRING Data,
              IN PCONTEXT Context)
{
    STRING Header;
    PVOID ControlStart;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == sizeof(CONTEXT));

    /* Make sure that this is a valid request */
    if (State->Processor < KeNumberProcessors)
    {
        /* Check if the request is for this CPU */
        if (State->Processor == KeGetCurrentPrcb()->Number)
        {
            /* We're just copying our own context */
            ControlStart = Context;
        }
        else
        {
            /* SMP not yet handled */
            ControlStart = NULL;
            while (TRUE);
        }

        /* Copy the memory */
        RtlCopyMemory(ControlStart, Data->Buffer, sizeof(CONTEXT));

        /* Finish up */
        State->ReturnStatus = STATUS_SUCCESS;
    }
    else
    {
        /* Invalid request */
        State->ReturnStatus = STATUS_UNSUCCESSFUL;
    }

    /* Send the reply */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 Data,
                 &KdpContext);
}

KCONTINUE_STATUS
NTAPI
KdpSendWaitContinue(IN ULONG PacketType,
                    IN PSTRING SendHeader,
                    IN PSTRING SendData OPTIONAL,
                    IN OUT PCONTEXT Context)
{
    STRING Data, Header;
    DBGKD_MANIPULATE_STATE64 ManipulateState;
    ULONG Length;
    KDSTATUS RecvCode;
FrLdrDbgPrint("Enter KdpSendWaitContinue\n");
    /* Setup the Manipulate State structure */
    Header.MaximumLength = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)&ManipulateState;
    Data.MaximumLength = sizeof(KdpMessageBuffer);
    Data.Buffer = KdpMessageBuffer;
    //KdpContextSent = FALSE;

SendPacket:
    /* Send the Packet */
    KdSendPacket(PacketType, SendHeader, SendData, &KdpContext);

    /* If the debugger isn't present anymore, just return success */
    if (KdDebuggerNotPresent) return ContinueSuccess;

    /* Main processing Loop */
    for (;;)
    {
        /* Receive Loop */
        do
        {
            /* Wait to get a reply to our packet */
            RecvCode = KdReceivePacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                                       &Header,
                                       &Data,
                                       &Length,
                                       &KdpContext);

            /* If we got a resend request, do it */
            if (RecvCode == KdPacketNeedsResend) goto SendPacket;
        } while (RecvCode == KdPacketTimedOut);

        /* Now check what API we got */
        switch (ManipulateState.ApiNumber)
        {
            case DbgKdReadVirtualMemoryApi:

                /* Read virtual memory */
                KdpReadVirtualMemory(&ManipulateState, &Data, Context);
                break;

            case DbgKdWriteVirtualMemoryApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdWriteVirtualMemoryApi);
                while (TRUE);
                break;

            case DbgKdGetContextApi:

                /* Get the current context */
                KdpGetContext(&ManipulateState, &Data, Context);
                break;

            case DbgKdSetContextApi:

                /* Set a new context */
                KdpSetContext(&ManipulateState, &Data, Context);
                break;

            case DbgKdWriteBreakPointApi:

                /* Write the breakpoint */
                KdpWriteBreakpoint(&ManipulateState, &Data, Context);
                break;

            case DbgKdRestoreBreakPointApi:

                /* FIXME: TODO */
                KdpRestoreBreakpoint(&ManipulateState, &Data, Context);
                break;

            case DbgKdContinueApi:

                /* Simply continue */
                return NT_SUCCESS(ManipulateState.u.Continue.ContinueStatus);

            case DbgKdReadControlSpaceApi:

                /* Read control space */
                KdpReadControlSpace(&ManipulateState, &Data, Context);
                break;

            case DbgKdWriteControlSpaceApi:

                /* FIXME: TODO */
                KdpWriteControlSpace(&ManipulateState, &Data, Context);
                break;

            case DbgKdReadIoSpaceApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdReadIoSpaceApi);
                while (TRUE);
                break;

            case DbgKdWriteIoSpaceApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdWriteIoSpaceApi);
                while (TRUE);
                break;

            case DbgKdRebootApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdRebootApi);
                while (TRUE);
                break;

            case DbgKdContinueApi2:

                /* Check if caller reports success */
                if (NT_SUCCESS(ManipulateState.u.Continue2.ContinueStatus))
                {
                    /* Update the state */
                    KdpGetStateChange(&ManipulateState, Context);
                    return ContinueSuccess;
                }
                else
                {
                    /* Return an error */
                    return ContinueError;
                }
                break;

            case DbgKdReadPhysicalMemoryApi:

                /* FIXME: TODO */
                goto fail;
                Ke386SetCr2(DbgKdReadPhysicalMemoryApi);
                while (TRUE);
                break;

            case DbgKdWritePhysicalMemoryApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdWritePhysicalMemoryApi);
                while (TRUE);
                break;

            case DbgKdQuerySpecialCallsApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdQuerySpecialCallsApi);
                while (TRUE);
                break;

            case DbgKdSetSpecialCallApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdSetSpecialCallApi);
                while (TRUE);
                break;

            case DbgKdClearSpecialCallsApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdClearSpecialCallsApi);
                while (TRUE);
                break;

            case DbgKdSetInternalBreakPointApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdSetInternalBreakPointApi);
                while (TRUE);
                break;

            case DbgKdGetInternalBreakPointApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdGetInternalBreakPointApi);
                while (TRUE);
                break;

            case DbgKdReadIoSpaceExtendedApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdReadIoSpaceExtendedApi);
                while (TRUE);
                break;

            case DbgKdWriteIoSpaceExtendedApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdWriteIoSpaceExtendedApi);
                while (TRUE);
                break;

            case DbgKdGetVersionApi:

                /* Get version data */
                KdpGetVersion(&ManipulateState);
                break;

            case DbgKdWriteBreakPointExApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdWriteBreakPointExApi);
                while (TRUE);
                break;

            case DbgKdRestoreBreakPointExApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdRestoreBreakPointExApi);
                while (TRUE);
                break;

            case DbgKdCauseBugCheckApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdCauseBugCheckApi);
                while (TRUE);
                break;

            case DbgKdSwitchProcessor:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdSwitchProcessor);
                while (TRUE);
                break;

            case DbgKdPageInApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdPageInApi);
                while (TRUE);
                break;

            case DbgKdReadMachineSpecificRegister:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdReadMachineSpecificRegister);
                while (TRUE);
                break;

            case DbgKdWriteMachineSpecificRegister:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdWriteMachineSpecificRegister);
                while (TRUE);
                break;

            case OldVlm1:

                /* FIXME: TODO */
                Ke386SetCr2(OldVlm1);
                while (TRUE);
                break;

            case OldVlm2:

                /* FIXME: TODO */
                Ke386SetCr2(OldVlm2);
                while (TRUE);
                break;

            case DbgKdSearchMemoryApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdSearchMemoryApi);
                while (TRUE);
                break;

            case DbgKdGetBusDataApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdGetBusDataApi);
                while (TRUE);
                break;

            case DbgKdSetBusDataApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdSetBusDataApi);
                while (TRUE);
                break;

            case DbgKdCheckLowMemoryApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdCheckLowMemoryApi);
                while (TRUE);
                break;

            case DbgKdClearAllInternalBreakpointsApi:

                /* Just clear the counter */
                KdpNumInternalBreakpoints = 0;
                break;

            case DbgKdFillMemoryApi:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdFillMemoryApi);
                while (TRUE);
                break;

            case DbgKdQueryMemoryApi:

                /* Query memory */
                KdpQueryMemory(&ManipulateState, Context);
                break;

            case DbgKdSwitchPartition:

                /* FIXME: TODO */
                Ke386SetCr2(DbgKdSwitchPartition);
                while (TRUE);
                break;

            /* Unsupported Message */
            default:

                /* Setup an empty message, with failure */
                while (TRUE);
fail:
                Data.Length = 0;
                ManipulateState.ReturnStatus = STATUS_UNSUCCESSFUL;

                /* Send it */
                KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                             &Header,
                             &Data,
                             &KdpContext);
                break;
        }
    }
}

BOOLEAN
NTAPI
KdpReportLoadSymbolsStateChange(IN PSTRING PathName,
                                IN PKD_SYMBOLS_INFO SymbolInfo,
                                IN BOOLEAN Unload,
                                IN OUT PCONTEXT Context)
{
    PSTRING ExtraData;
    STRING Data, Header;
    DBGKD_WAIT_STATE_CHANGE64 WaitStateChange;
    KCONTINUE_STATUS Status;

    /* Start wait loop */
    do
    {
        /* Build the architecture common parts of the message */
        KdpSetCommonState(DbgKdLoadSymbolsStateChange,
                          Context,
                          &WaitStateChange);

        /* Now finish creating the structure */
        KdpSetContextState(&WaitStateChange, Context);

        /* Fill out load data */
        WaitStateChange.u.LoadSymbols.UnloadSymbols = Unload;
        WaitStateChange.u.LoadSymbols.BaseOfDll = (ULONGLONG)(LONG_PTR)SymbolInfo->BaseOfDll;
        WaitStateChange.u.LoadSymbols.ProcessId = SymbolInfo->ProcessId;
        WaitStateChange.u.LoadSymbols.CheckSum = SymbolInfo->CheckSum;
        WaitStateChange.u.LoadSymbols.SizeOfImage = SymbolInfo->SizeOfImage;

        /* Check if we have a symbol name */
        if (PathName)
        {
            /* Setup the information */
            WaitStateChange.u.LoadSymbols.PathNameLength = PathName->Length;
            RtlCopyMemory(KdpPathBuffer, PathName->Buffer, PathName->Length);
            Data.Buffer = KdpPathBuffer;
            Data.Length = WaitStateChange.u.LoadSymbols.PathNameLength;
            ExtraData = &Data;
        }
        else
        {
            /* No name */
            WaitStateChange.u.LoadSymbols.PathNameLength = 0;
            ExtraData = NULL;
        }

        /* Setup the header */
        Header.Length = sizeof(DBGKD_WAIT_STATE_CHANGE64);
        Header.Buffer = (PCHAR)&WaitStateChange;

        /* Send the packet */
        Status = KdpSendWaitContinue(PACKET_TYPE_KD_STATE_CHANGE64,
                                     &Header,
                                     ExtraData,
                                     Context);
    } while(Status == ContinueProcessorReselected);

    /* Return status */
    return Status;
}

BOOLEAN
NTAPI
KdpReportExceptionStateChange(IN PEXCEPTION_RECORD ExceptionRecord,
                              IN OUT PCONTEXT Context,
                              IN BOOLEAN SecondChanceException)
{
    STRING Header, Data;
    DBGKD_WAIT_STATE_CHANGE64 WaitStateChange;
    BOOLEAN Status;
FrLdrDbgPrint("Enter KdpReportExceptionStateChange, Rip = 0x%p\n", (PVOID)Context->Rip);
    /* Start report loop */
    do
    {
        /* Build the architecture common parts of the message */
        KdpSetCommonState(DbgKdExceptionStateChange, Context, &WaitStateChange);

        /* Convert the exception record to 64-bits and set First Chance flag */
        ExceptionRecord32To64((PEXCEPTION_RECORD32)ExceptionRecord,
                              &WaitStateChange.u.Exception.ExceptionRecord);
        WaitStateChange.u.Exception.FirstChance = !SecondChanceException;

        /* Now finish creating the structure */
        KdpSetContextState(&WaitStateChange, Context);

        /* Setup the actual header to send to KD */
        Header.Length = sizeof(DBGKD_WAIT_STATE_CHANGE64) - sizeof(CONTEXT);
        Header.Buffer = (PCHAR)&WaitStateChange;

        /* Setup the trace data */
        DumpTraceData(&Data);
FrLdrDbgPrint("KdpReportExceptionStateChange 5\n");
        /* Send State Change packet and wait for a reply */
        Status = KdpSendWaitContinue(PACKET_TYPE_KD_STATE_CHANGE64,
                                     &Header,
                                     &Data,
                                     Context);
    } while (Status == KdPacketNeedsResend);
FrLdrDbgPrint("Leave KdpReportExceptionStateChange, Status = 0x%x\n", Status);
    /* Return */
    return Status;
}

VOID
NTAPI
KdpTimeSlipDpcRoutine(IN PKDPC Dpc,
                      IN PVOID DeferredContext,
                      IN PVOID SystemArgument1,
                      IN PVOID SystemArgument2)
{
    LONG OldSlip, NewSlip, PendingSlip;

    /* Get the current pending slip */
    PendingSlip = KdpTimeSlipPending;
    do
    {
        /* Save the old value and either disable or enable it now. */
        OldSlip = PendingSlip;
        NewSlip = OldSlip > 1 ? 1 : 0;

        /* Try to change the value */
    } while (InterlockedCompareExchange(&KdpTimeSlipPending,
                                        NewSlip,
                                        OldSlip) != OldSlip);

    /* If the New Slip value is 1, then do the Time Slipping */
    if (NewSlip) ExQueueWorkItem(&KdpTimeSlipWorkItem, DelayedWorkQueue);
}

VOID
NTAPI
KdpTimeSlipWork(IN PVOID Context)
{
    KIRQL OldIrql;
    LARGE_INTEGER DueTime;

    /* Update the System time from the CMOS */
    ExAcquireTimeRefreshLock(FALSE);
    ExUpdateSystemTimeFromCmos(FALSE, 0);
    ExReleaseTimeRefreshLock();

    /* Check if we have a registered Time Slip Event and signal it */
    KeAcquireSpinLock(&KdpTimeSlipEventLock, &OldIrql);
    if (KdpTimeSlipEvent) KeSetEvent(KdpTimeSlipEvent, 0, FALSE);
    KeReleaseSpinLock(&KdpTimeSlipEventLock, OldIrql);

    /* Delay the DPC until it runs next time */
    DueTime.QuadPart = -1800000000;
    KeSetTimer(&KdpTimeSlipTimer, DueTime, &KdpTimeSlipDpc);
}

BOOLEAN
NTAPI
KdpSwitchProcessor(IN PEXCEPTION_RECORD ExceptionRecord,
                   IN OUT PCONTEXT ContextRecord,
                   IN BOOLEAN SecondChanceException)
{
    BOOLEAN Status;

    /* Save the port data */
    KdSave(FALSE);

    /* Report a state change */
    Status = KdpReportExceptionStateChange(ExceptionRecord,
                                           ContextRecord,
                                           SecondChanceException);

    /* Restore the port data and return */
    KdRestore(FALSE);
    return Status;
}

LARGE_INTEGER
NTAPI
KdpQueryPerformanceCounter(IN PKTRAP_FRAME TrapFrame)
{
    LARGE_INTEGER Null = {{0}};

    /* Check if interrupts were disabled */
    if (!(TrapFrame->EFlags & EFLAGS_INTERRUPT_MASK))
    {
        /* Nothing to return */
        return Null;
    }

    /* Otherwise, do the call */
    return KeQueryPerformanceCounter(NULL);
}

BOOLEAN
NTAPI
KdEnterDebugger(IN PKTRAP_FRAME TrapFrame,
                IN PKEXCEPTION_FRAME ExceptionFrame)
{
    BOOLEAN Entered;
FrLdrDbgPrint("KdEnterDebugger!\n");
    /* Check if we have a trap frame */
    if (TrapFrame)
    {
        /* Calculate the time difference for the enter */
        KdTimerStop = KdpQueryPerformanceCounter(TrapFrame);
        KdTimerDifference.QuadPart = KdTimerStop.QuadPart -
                                     KdTimerStart.QuadPart;
    }
    else
    {
        /* No trap frame, so can't calculate */
        KdTimerStop.QuadPart = 0;
    }

    /* Save the current IRQL */
    KeGetCurrentPrcb()->DebuggerSavedIRQL = KeGetCurrentIrql();

    /* Freeze all CPUs */
    Entered = KeFreezeExecution(TrapFrame, ExceptionFrame);

    /* Lock the port, save the state and set debugger entered */
    KdpPortLocked = KeTryToAcquireSpinLockAtDpcLevel(&KdpDebuggerLock);
    KdSave(FALSE);
    KdEnteredDebugger = TRUE;

    /* Check freeze flag */
    if (KiFreezeFlag & 1)
    {
        /* Print out errror */
        DbgPrint("FreezeLock was jammed!  Backup SpinLock was used!\n");
    }

    /* Check processor state */
    if (KiFreezeFlag & 2)
    {
        /* Print out errror */
        DbgPrint("Some processors not frozen in debugger!\n");
    }

    /* Make sure we acquired the port */
    if (!KdpPortLocked) DbgPrint("Port lock was not acquired!\n");
FrLdrDbgPrint("KdEnterDebugger returns %d\n", Entered);
    /* Return enter state */
    return Entered;
}

VOID
NTAPI
KdExitDebugger(IN BOOLEAN Entered)
{
    ULONG TimeSlip;

    /* Restore the state and unlock the port */
    KdRestore(FALSE);
    if (KdpPortLocked) KdpPortUnlock();

    /* Unfreeze the CPUs */
    KeThawExecution(Entered);

    /* Compare time with the one from KdEnterDebugger */
    if (!KdTimerStop.QuadPart)
    {
        /* We didn't get a trap frame earlier in so never got the time */
        KdTimerStart = KdTimerStop;
    }
    else
    {
        /* Query the timer */
        KdTimerStart = KeQueryPerformanceCounter(NULL);
    }

    /* Check if a Time Slip was on queue */
    TimeSlip = InterlockedIncrement(&KdpTimeSlipPending);
    if (TimeSlip == 1)
    {
        /* Queue a DPC for the time slip */
        InterlockedIncrement(&KdpTimeSlipPending);
        KeInsertQueueDpc(&KdpTimeSlipDpc, NULL, NULL);
    }
}

NTSTATUS
NTAPI
KdEnableDebuggerWithLock(BOOLEAN NeedLock)
{
    KIRQL OldIrql = PASSIVE_LEVEL;

    /* Check if we need to acquire the lock */
    if (NeedLock)
    {
        /* Lock the port */
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        KdpPortLock();
    }

    /* Check if we're not disabled */
    if (!KdDisableCount)
    {
        /* Check if we had locked the port before */
        if (NeedLock)
        {
            /* Do the unlock */
            KeLowerIrql(OldIrql);
            KdpPortUnlock();
        }

        /* Fail: We're already enabled */
        return STATUS_INVALID_PARAMETER;
    }

    /* Decrease the disable count */
    if (!(--KdDisableCount))
    {
        /* We're now enabled again! Were we enabled before, too? */
        if (KdPreviouslyEnabled)
        {
            /* Reinitialize the Debugger */
            KdInitSystem(0, NULL) ;
            KdpRestoreAllBreakpoints();
        }
    }

    /* Check if we had locked the port before */
    if (NeedLock)
    {
        /* Yes, now unlock it */
        KeLowerIrql(OldIrql);
        KdpPortUnlock();
    }

    /* We're done */
    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
KdEnableDebugger(VOID)
{
    /* Use the internal routine */
    while (TRUE);
    return KdEnableDebuggerWithLock(TRUE);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KdSystemDebugControl(IN SYSDBG_COMMAND Command,
                     IN PVOID InputBuffer,
                     IN ULONG InputBufferLength,
                     OUT PVOID OutputBuffer,
                     IN ULONG OutputBufferLength,
                     IN OUT PULONG ReturnLength,
                     IN KPROCESSOR_MODE PreviousMode)
{
    /* HACK */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KdChangeOption(IN KD_OPTION Option,
               IN ULONG InBufferBytes OPTIONAL,
               IN PVOID InBuffer,
               IN ULONG OutBufferBytes OPTIONAL,
               OUT PVOID OutBuffer,
               OUT PULONG OutBufferNeeded OPTIONAL)
{
    /* HACK */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KdPowerTransition(IN DEVICE_POWER_STATE NewState)
{
    /* HACK */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KdDisableDebugger(VOID)
{
    /* HACK */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
KdRefreshDebuggerNotPresent(VOID)
{
    /* HACK */
    return KdDebuggerNotPresent;
}

NTSTATUS
NTAPI
NtQueryDebugFilterState(ULONG ComponentId,
                        ULONG Level)
{
    /* HACK */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NtSetDebugFilterState(ULONG ComponentId,
                      ULONG Level,
                      BOOLEAN State)
{
    /* HACK */
    return STATUS_SUCCESS;
}

