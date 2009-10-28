/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/kdapi.c
 * PURPOSE:         KD64 Public Routines and Internal Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
KdpCopyMemoryChunks(IN ULONG64 Address,
                    IN PVOID Buffer,
                    IN ULONG TotalSize,
                    IN ULONG ChunkSize,
                    IN ULONG Flags,
                    OUT PULONG ActualSize OPTIONAL)
{
    ULONG Length;
    NTSTATUS Status;

    /* Check if this is physical or virtual copy */
    if (Flags & MMDBG_COPY_PHYSICAL)
    {
        /* Fail physical memory read/write for now */
        if (Flags & MMDBG_COPY_WRITE)
        {
            KdpDprintf("KdpCopyMemoryChunks: Failing write for Physical Address 0x%I64x Length: %x\n",
                       Address,
                       TotalSize);
        }
        else
        {
            KdpDprintf("KdpCopyMemoryChunks: Failing read for Physical Address 0x%I64x Length: %x\n",
                       Address,
                       TotalSize);
        }

        /* Return an error */
        Length = 0;
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        /* Protect against NULL */
        if (!Address)
        {
            if (ActualSize) *ActualSize = 0;
            return STATUS_UNSUCCESSFUL;
        }

        /* Check if this is read or write */
        if (Flags & MMDBG_COPY_WRITE)
        {
            /* Do the write */
            RtlCopyMemory((PVOID)(ULONG_PTR)Address,
                          Buffer,
                          TotalSize);
        }
        else
        {
            /* Do the read */
            RtlCopyMemory(Buffer,
                          (PVOID)(ULONG_PTR)Address,
                          TotalSize);
        }

        /* Set size and status */
        Length = TotalSize;
        Status = STATUS_SUCCESS;
    }

    /* Return the actual length if requested */
    if (ActualSize) *ActualSize = Length;

    /* Return status */
    return Status;
}

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
        if ((PVOID)(ULONG_PTR)Memory->Address < MmHighestUserAddress)
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
KdpSearchMemory(IN PDBGKD_MANIPULATE_STATE64 State,
                IN PSTRING Data,
                IN PCONTEXT Context)
{
    /* FIXME: STUB */
    KdpDprintf("KdpSearchMemory called\n");
    while (TRUE);
}

VOID
NTAPI
KdpFillMemory(IN PDBGKD_MANIPULATE_STATE64 State,
              IN PSTRING Data,
              IN PCONTEXT Context)
{
    /* FIXME: STUB */
    KdpDprintf("KdpFillMemory called\n");
    while (TRUE);
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
        KdpAddBreakpoint((PVOID)(ULONG_PTR)Breakpoint->BreakPointAddress);
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

NTSTATUS
NTAPI
KdpWriteBreakPointEx(IN PDBGKD_MANIPULATE_STATE64 State,
                     IN PSTRING Data,
                     IN PCONTEXT Context)
{
    /* FIXME: STUB */
    KdpDprintf("KdpWriteBreakPointEx called\n");
    while (TRUE);
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
KdpRestoreBreakPointEx(IN PDBGKD_MANIPULATE_STATE64 State,
                       IN PSTRING Data,
                       IN PCONTEXT Context)
{
    /* FIXME: STUB */
    KdpDprintf("KdpRestoreBreakPointEx called\n");
    while (TRUE);
}

VOID
NTAPI
DumpTraceData(IN PSTRING TraceData)
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
KdpSetCommonState(IN ULONG NewState,
                  IN PCONTEXT Context,
                  IN PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange)
{
    USHORT InstructionCount;
    BOOLEAN HadBreakpoints;

    /* Setup common stuff available for all CPU architectures */
    WaitStateChange->NewState = NewState;
    WaitStateChange->ProcessorLevel = KeProcessorLevel;
    WaitStateChange->Processor = (USHORT)KeGetCurrentPrcb()->Number;
    WaitStateChange->NumberProcessors = (ULONG)KeNumberProcessors;
    WaitStateChange->Thread = (ULONG64)(LONG_PTR)KeGetCurrentThread();
    WaitStateChange->ProgramCounter = (ULONG64)(LONG_PTR)KeGetContextPc(Context);

    /* Zero out the entire Control Report */
    RtlZeroMemory(&WaitStateChange->AnyControlReport,
                  sizeof(DBGKD_ANY_CONTROL_REPORT));

    /* Now copy the instruction stream and set the count */
    RtlCopyMemory(&WaitStateChange->ControlReport.InstructionStream[0],
                  (PVOID)(ULONG_PTR)WaitStateChange->ProgramCounter,
                  DBGKD_MAXSTREAM);
    InstructionCount = DBGKD_MAXSTREAM;
    WaitStateChange->ControlReport.InstructionCount = InstructionCount;

    /* Clear all the breakpoints in this region */
    HadBreakpoints =
        KdpDeleteBreakpointRange((PVOID)(ULONG_PTR)WaitStateChange->ProgramCounter,
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
    PDBGKD_READ_MEMORY64 ReadMemory = &State->u.ReadMemory;
    STRING Header;
    ULONG Length = ReadMemory->TransferCount;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == 0);

    /* Validate length */
    if (Length > (PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64)))
    {
        /* Overflow, set it to maximum possible */
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64);
    }

    /* Do the read */
    State->ReturnStatus = KdpCopyMemoryChunks(ReadMemory->TargetBaseAddress,
                                              Data->Buffer,
                                              Length,
                                              0,
                                              MMDBG_COPY_UNSAFE,
                                              &Length);

    /* Return the actual length read */
    Data->Length = ReadMemory->ActualBytesRead = Length;

    /* Send the packet */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 Data,
                 &KdpContext);
}

VOID
NTAPI
KdpWriteVirtualMemory(IN PDBGKD_MANIPULATE_STATE64 State,
                      IN PSTRING Data,
                      IN PCONTEXT Context)
{
    PDBGKD_WRITE_MEMORY64 WriteMemory = &State->u.WriteMemory;
    STRING Header;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;

    /* Do the write */
    State->ReturnStatus = KdpCopyMemoryChunks(WriteMemory->TargetBaseAddress,
                                              Data->Buffer,
                                              Data->Length,
                                              0,
                                              MMDBG_COPY_UNSAFE |
                                              MMDBG_COPY_WRITE,
                                              &WriteMemory->ActualBytesWritten);

    /* Send the packet */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
                 &KdpContext);
}

VOID
NTAPI
KdpReadPhysicalmemory(IN PDBGKD_MANIPULATE_STATE64 State,
                      IN PSTRING Data,
                      IN PCONTEXT Context)
{
    PDBGKD_READ_MEMORY64 ReadMemory = &State->u.ReadMemory;
    STRING Header;
    ULONG Length = ReadMemory->TransferCount;
    ULONG Flags, CacheFlags;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == 0);

    /* Validate length */
    if (Length > (PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64)))
    {
        /* Overflow, set it to maximum possible */
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64);
    }

    /* Start with the default flags */
    Flags = MMDBG_COPY_UNSAFE | MMDBG_COPY_PHYSICAL;

    /* Get the caching flags and check if a type is specified */
    CacheFlags = ReadMemory->ActualBytesRead;
    if (CacheFlags == DBGKD_CACHING_CACHED)
    {
        /* Cached */
        Flags |= MMDBG_COPY_CACHED;
    }
    else if (CacheFlags == DBGKD_CACHING_UNCACHED)
    {
        /* Uncached */
        Flags |= MMDBG_COPY_UNCACHED;
    }
    else if (CacheFlags == DBGKD_CACHING_UNCACHED)
    {
        /* Write Combined */
        Flags |= DBGKD_CACHING_WRITE_COMBINED;
    }

    /* Do the read */
    State->ReturnStatus = KdpCopyMemoryChunks(ReadMemory->TargetBaseAddress,
                                              Data->Buffer,
                                              Length,
                                              0,
                                              Flags,
                                              &Length);

    /* Return the actual length read */
    Data->Length = ReadMemory->ActualBytesRead = Length;

    /* Send the packet */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 Data,
                 &KdpContext);
}

VOID
NTAPI
KdpWritePhysicalmemory(IN PDBGKD_MANIPULATE_STATE64 State,
                       IN PSTRING Data,
                       IN PCONTEXT Context)
{
    PDBGKD_WRITE_MEMORY64 WriteMemory = &State->u.WriteMemory;
    STRING Header;
    ULONG Flags, CacheFlags;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;

    /* Start with the default flags */
    Flags = MMDBG_COPY_UNSAFE | MMDBG_COPY_WRITE | MMDBG_COPY_PHYSICAL;

    /* Get the caching flags and check if a type is specified */
    CacheFlags = WriteMemory->ActualBytesWritten;
    if (CacheFlags == DBGKD_CACHING_CACHED)
    {
        /* Cached */
        Flags |= MMDBG_COPY_CACHED;
    }
    else if (CacheFlags == DBGKD_CACHING_UNCACHED)
    {
        /* Uncached */
        Flags |= MMDBG_COPY_UNCACHED;
    }
    else if (CacheFlags == DBGKD_CACHING_UNCACHED)
    {
        /* Write Combined */
        Flags |= DBGKD_CACHING_WRITE_COMBINED;
    }

    /* Do the write */
    State->ReturnStatus = KdpCopyMemoryChunks(WriteMemory->TargetBaseAddress,
                                              Data->Buffer,
                                              Data->Length,
                                              0,
                                              Flags,
                                              &WriteMemory->ActualBytesWritten);

    /* Send the packet */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
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
    ULONG Length;

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

    /* Call the internal routine */
    State->ReturnStatus = KdpSysReadControlSpace(State->Processor,
                                                 ReadMemory->TargetBaseAddress,
                                                 Data->Buffer,
                                                 Length,
                                                 &Length);

    /* Return the actual length read */
    Data->Length = ReadMemory->ActualBytesRead = Length;

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

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;

    /* Call the internal routine */
    State->ReturnStatus = KdpSysWriteControlSpace(State->Processor,
                                                  WriteMemory->TargetBaseAddress,
                                                  Data->Buffer,
                                                  Data->Length,
                                                  &WriteMemory->ActualBytesWritten);

    /* Send the reply */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 Data,
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
            KdpDprintf("KdpGetContext: SMP UNHANDLED\n");
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
            KdpDprintf("KdpSetContext: SMP UNHANDLED\n");
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
                 NULL,
                 &KdpContext);
}

VOID
NTAPI
KdpCauseBugCheck(IN PDBGKD_MANIPULATE_STATE64 State)
{
    /* Crash with the special code */
    KeBugCheck(MANUALLY_INITIATED_CRASH);
}

VOID
NTAPI
KdpReadMachineSpecificRegister(IN PDBGKD_MANIPULATE_STATE64 State,
                               IN PSTRING Data,
                               IN PCONTEXT Context)
{
    STRING Header;
    PDBGKD_READ_WRITE_MSR ReadMsr = &State->u.ReadWriteMsr;
    LARGE_INTEGER MsrValue;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == 0);

    /* Call the internal routine */
    State->ReturnStatus = KdpSysReadMsr(ReadMsr->Msr,
                                        &MsrValue);

    /* Return the data */
    ReadMsr->DataValueLow = MsrValue.LowPart;
    ReadMsr->DataValueHigh = MsrValue.HighPart;

    /* Send the reply */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
                 &KdpContext);
}

VOID
NTAPI
KdpWriteMachineSpecificRegister(IN PDBGKD_MANIPULATE_STATE64 State,
                                IN PSTRING Data,
                                IN PCONTEXT Context)
{
    STRING Header;
    PDBGKD_READ_WRITE_MSR WriteMsr = &State->u.ReadWriteMsr;
    LARGE_INTEGER MsrValue;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == 0);

    /* Call the internal routine */
    MsrValue.LowPart = WriteMsr->DataValueLow;
    MsrValue.HighPart = WriteMsr->DataValueHigh;
    State->ReturnStatus = KdpSysWriteMsr(WriteMsr->Msr,
                                         &MsrValue);

    /* Send the reply */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
                 &KdpContext);
}

VOID
NTAPI
KdpGetBusData(IN PDBGKD_MANIPULATE_STATE64 State,
              IN PSTRING Data,
              IN PCONTEXT Context)
{
    STRING Header;
    PDBGKD_GET_SET_BUS_DATA GetBusData = &State->u.GetSetBusData;
    ULONG Length;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == 0);

    /* Check the length requested */
    Length = GetBusData->Length;
    if (Length > (PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64)))
    {
        /* Use maximum allowed */
        Length = PACKET_MAX_SIZE - sizeof(DBGKD_MANIPULATE_STATE64);
    }

    /* Call the internal routine */
    State->ReturnStatus = KdpSysReadBusData(GetBusData->BusDataType,
                                            GetBusData->BusNumber,
                                            GetBusData->SlotNumber,
                                            GetBusData->Offset,
                                            Data->Buffer,
                                            Length,
                                            &Length);

    /* Return the actual length read */
    Data->Length = GetBusData->Length = Length;

    /* Send the reply */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 Data,
                 &KdpContext);
}

VOID
NTAPI
KdpSetBusData(IN PDBGKD_MANIPULATE_STATE64 State,
              IN PSTRING Data,
              IN PCONTEXT Context)
{
    STRING Header;
    PDBGKD_GET_SET_BUS_DATA SetBusData = &State->u.GetSetBusData;
    ULONG Length;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;

    /* Call the internal routine */
    State->ReturnStatus = KdpSysWriteBusData(SetBusData->BusDataType,
                                             SetBusData->BusNumber,
                                             SetBusData->SlotNumber,
                                             SetBusData->Offset,
                                             Data->Buffer,
                                             SetBusData->Length,
                                             &Length);

    /* Return the actual length written */
    SetBusData->Length = Length;

    /* Send the reply */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
                 &KdpContext);
}

VOID
NTAPI
KdpReadIoSpace(IN PDBGKD_MANIPULATE_STATE64 State,
               IN PSTRING Data,
               IN PCONTEXT Context)
{
    STRING Header;
    PDBGKD_READ_WRITE_IO64 ReadIo = &State->u.ReadWriteIo;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == 0);

    /*
     * Clear the value so 1 or 2 byte reads
     * don't leave the higher bits unmodified 
     */
    ReadIo->DataValue = 0;

    /* Call the internal routine */
    State->ReturnStatus = KdpSysReadIoSpace(Isa,
                                            0,
                                            1,
                                            ReadIo->IoAddress,
                                            &ReadIo->DataValue,
                                            ReadIo->DataSize,
                                            &ReadIo->DataSize);

    /* Send the reply */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
                 &KdpContext);
}

VOID
NTAPI
KdpWriteIoSpace(IN PDBGKD_MANIPULATE_STATE64 State,
                IN PSTRING Data,
                IN PCONTEXT Context)
{
    STRING Header;
    PDBGKD_READ_WRITE_IO64 WriteIo = &State->u.ReadWriteIo;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == 0);

    /* Call the internal routine */
    State->ReturnStatus = KdpSysWriteIoSpace(Isa,
                                             0,
                                             1,
                                             WriteIo->IoAddress,
                                             &WriteIo->DataValue,
                                             WriteIo->DataSize,
                                             &WriteIo->DataSize);

    /* Send the reply */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
                 &KdpContext);
}

VOID
NTAPI
KdpReadIoSpaceExtended(IN PDBGKD_MANIPULATE_STATE64 State,
                       IN PSTRING Data,
                       IN PCONTEXT Context)
{
    STRING Header;
    PDBGKD_READ_WRITE_IO_EXTENDED64 ReadIoExtended = &State->u.
                                                      ReadWriteIoExtended;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == 0);

    /*
     * Clear the value so 1 or 2 byte reads
     * don't leave the higher bits unmodified 
     */
    ReadIoExtended->DataValue = 0;

    /* Call the internal routine */
    State->ReturnStatus = KdpSysReadIoSpace(ReadIoExtended->InterfaceType,
                                            ReadIoExtended->BusNumber,
                                            ReadIoExtended->AddressSpace,
                                            ReadIoExtended->IoAddress,
                                            &ReadIoExtended->DataValue,
                                            ReadIoExtended->DataSize,
                                            &ReadIoExtended->DataSize);

    /* Send the reply */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
                 &KdpContext);
}

VOID
NTAPI
KdpWriteIoSpaceExtended(IN PDBGKD_MANIPULATE_STATE64 State,
                        IN PSTRING Data,
                        IN PCONTEXT Context)
{
    STRING Header;
    PDBGKD_READ_WRITE_IO_EXTENDED64 WriteIoExtended = &State->u.
                                                       ReadWriteIoExtended;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == 0);

    /* Call the internal routine */
    State->ReturnStatus = KdpSysReadIoSpace(WriteIoExtended->InterfaceType,
                                            WriteIoExtended->BusNumber,
                                            WriteIoExtended->AddressSpace,
                                            WriteIoExtended->IoAddress,
                                            &WriteIoExtended->DataValue,
                                            WriteIoExtended->DataSize,
                                            &WriteIoExtended->DataSize);

    /* Send the reply */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
                 &KdpContext);
}

VOID
NTAPI
KdpCheckLowMemory(IN PDBGKD_MANIPULATE_STATE64 State)
{
    STRING Header;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;

    /* Call the internal routine */
    State->ReturnStatus = KdpSysCheckLowMemory(MMDBG_COPY_UNSAFE);

    /* Send the reply */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
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

                /* Write virtual memory */
                KdpWriteVirtualMemory(&ManipulateState, &Data, Context);
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

                /* Restore the breakpoint */
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

                /* Write control space */
                KdpWriteControlSpace(&ManipulateState, &Data, Context);
                break;

            case DbgKdReadIoSpaceApi:

                /* Read I/O Space */
                KdpReadIoSpace(&ManipulateState, &Data, Context);
                break;

            case DbgKdWriteIoSpaceApi:

                /* Write I/O Space */
                KdpWriteIoSpace(&ManipulateState, &Data, Context);
                break;

            case DbgKdRebootApi:

                /* Reboot the system */
                HalReturnToFirmware(HalRebootRoutine);
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

            case DbgKdReadPhysicalMemoryApi:

                /* Read  physical memory */
                KdpReadPhysicalmemory(&ManipulateState, &Data, Context);
                break;

            case DbgKdWritePhysicalMemoryApi:

                /* Write  physical memory */
                KdpWritePhysicalmemory(&ManipulateState, &Data, Context);
                break;

            case DbgKdQuerySpecialCallsApi:

                /* FIXME: TODO */
                KdpDprintf("DbgKdQuerySpecialCallsApi called\n");
                while (TRUE);
                break;

            case DbgKdSetSpecialCallApi:

                /* FIXME: TODO */
                KdpDprintf("DbgKdSetSpecialCallApi called\n");
                while (TRUE);
                break;

            case DbgKdClearSpecialCallsApi:

                /* FIXME: TODO */
                KdpDprintf("DbgKdClearSpecialCallsApi called\n");
                while (TRUE);
                break;

            case DbgKdSetInternalBreakPointApi:

                /* FIXME: TODO */
                KdpDprintf("DbgKdSetInternalBreakPointApi called\n");
                while (TRUE);
                break;

            case DbgKdGetInternalBreakPointApi:

                /* FIXME: TODO */
                KdpDprintf("DbgKdGetInternalBreakPointApi called\n");
                while (TRUE);
                break;

            case DbgKdReadIoSpaceExtendedApi:

                /* Read I/O Space */
                KdpReadIoSpaceExtended(&ManipulateState, &Data, Context);
                break;

            case DbgKdWriteIoSpaceExtendedApi:

                /* Write I/O Space */
                KdpWriteIoSpaceExtended(&ManipulateState, &Data, Context);
                break;

            case DbgKdGetVersionApi:

                /* Get version data */
                KdpGetVersion(&ManipulateState);
                break;

            case DbgKdWriteBreakPointExApi:

                /* Write the breakpoint and check if it failed */
                if (!NT_SUCCESS(KdpWriteBreakPointEx(&ManipulateState,
                                                     &Data,
                                                     Context)))
                {
                    /* Return an error */
                    return ContinueError;
                }
                break;

            case DbgKdRestoreBreakPointExApi:

                /* Restore the breakpoint */
                KdpRestoreBreakPointEx(&ManipulateState, &Data, Context);
                break;

            case DbgKdCauseBugCheckApi:

                /* Crash the system */
                KdpCauseBugCheck(&ManipulateState);
                break;

            case DbgKdSwitchProcessor:

                /* FIXME: TODO */
                KdpDprintf("DbgKdSwitchProcessor called\n");
                while (TRUE);
                break;

            case DbgKdPageInApi:

                /* FIXME: TODO */
                KdpDprintf("DbgKdPageInApi called\n");
                while (TRUE);
                break;

            case DbgKdReadMachineSpecificRegister:

                /* Read from the specified MSR */
                KdpReadMachineSpecificRegister(&ManipulateState, &Data, Context);
                break;

            case DbgKdWriteMachineSpecificRegister:

                /* Write to the specified MSR */
                KdpWriteMachineSpecificRegister(&ManipulateState, &Data, Context);
                break;

            case DbgKdSearchMemoryApi:

                /* Search memory */
                KdpSearchMemory(&ManipulateState, &Data, Context);
                break;

            case DbgKdGetBusDataApi:

                /* Read from the bus */
                KdpGetBusData(&ManipulateState, &Data, Context);
                break;

            case DbgKdSetBusDataApi:

                /* Write to the bus */
                KdpSetBusData(&ManipulateState, &Data, Context);
                break;

            case DbgKdCheckLowMemoryApi:

                /* Check for memory corruption in the lower 4 GB */
                KdpCheckLowMemory(&ManipulateState);
                break;

            case DbgKdClearAllInternalBreakpointsApi:

                /* Just clear the counter */
                KdpNumInternalBreakpoints = 0;
                break;

            case DbgKdFillMemoryApi:

                /* Fill memory */
                KdpFillMemory(&ManipulateState, &Data, Context);
                break;

            case DbgKdQueryMemoryApi:

                /* Query memory */
                KdpQueryMemory(&ManipulateState, Context);
                break;

            case DbgKdSwitchPartition:

                /* FIXME: TODO */
                KdpDprintf("DbgKdSwitchPartition called\n");
                while (TRUE);
                break;

            /* Unsupported Message */
            default:

                /* Setup an empty message, with failure */
                KdpDprintf("Received Unhandled API %lx\n", ManipulateState.ApiNumber);
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
    DBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange;
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
        WaitStateChange.u.LoadSymbols.BaseOfDll = (ULONG64)(LONG_PTR)SymbolInfo->BaseOfDll;
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
        Header.Length = sizeof(DBGKD_ANY_WAIT_STATE_CHANGE);
        Header.Buffer = (PCHAR)&WaitStateChange;

        /* Send the packet */
        Status = KdpSendWaitContinue(PACKET_TYPE_KD_STATE_CHANGE64,
                                     &Header,
                                     ExtraData,
                                     Context);
    } while (Status == ContinueProcessorReselected);

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
    DBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange;
    KCONTINUE_STATUS Status;

    /* Start report loop */
    do
    {
        /* Build the architecture common parts of the message */
        KdpSetCommonState(DbgKdExceptionStateChange, Context, &WaitStateChange);

        /* Copy the Exception Record and set First Chance flag */
#if !defined(_WIN64)
        ExceptionRecord32To64((PEXCEPTION_RECORD32)ExceptionRecord,
                              &WaitStateChange.u.Exception.ExceptionRecord);
#else
        RtlCopyMemory(&WaitStateChange.u.Exception.ExceptionRecord,
                      ExceptionRecord,
                      sizeof(EXCEPTION_RECORD));
#endif
        WaitStateChange.u.Exception.FirstChance = !SecondChanceException;

        /* Now finish creating the structure */
        KdpSetContextState(&WaitStateChange, Context);

        /* Setup the actual header to send to KD */
        Header.Length = sizeof(DBGKD_ANY_WAIT_STATE_CHANGE);
        Header.Buffer = (PCHAR)&WaitStateChange;

        /* Setup the trace data */
        DumpTraceData(&Data);

        /* Send State Change packet and wait for a reply */
        Status = KdpSendWaitContinue(PACKET_TYPE_KD_STATE_CHANGE64,
                                     &Header,
                                     &Data,
                                     Context);
    } while (Status == ContinueProcessorReselected);

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
    if (!KeGetTrapFrameInterruptState(TrapFrame))
    {
        /* Nothing to return */
        return Null;
    }

    /* Otherwise, do the call */
    return KeQueryPerformanceCounter(NULL);
}

NTSTATUS
NTAPI
KdpAllowDisable(VOID)
{
    /* Check if we are on MP */
    if (KeNumberProcessors > 1)
    {
        /* TODO */
        KdpDprintf("KdpAllowDisable: SMP UNHANDLED\n");
        while (TRUE);
    }

    /* Allow disable */
    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
KdEnterDebugger(IN PKTRAP_FRAME TrapFrame,
                IN PKEXCEPTION_FRAME ExceptionFrame)
{
    BOOLEAN Entered;

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
        KdpDprintf("FreezeLock was jammed!  Backup SpinLock was used!\n");
    }

    /* Check processor state */
    if (KiFreezeFlag & 2)
    {
        /* Print out errror */
        KdpDprintf("Some processors not frozen in debugger!\n");
    }

    /* Make sure we acquired the port */
    if (!KdpPortLocked) KdpDprintf("Port lock was not acquired!\n");

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
KdEnableDebuggerWithLock(IN BOOLEAN NeedLock)
{
    KIRQL OldIrql;

#if defined(__GNUC__)
    /* Make gcc happy */
    OldIrql = PASSIVE_LEVEL;
#endif

    /* Check if enabling the debugger is blocked */
    if (KdBlockEnable)
    {
        /* It is, fail the enable */
        return STATUS_ACCESS_DENIED;
    }

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

            /* Fail: We're already enabled */
            return STATUS_INVALID_PARAMETER;
        }
        else
        {
            /*
             * This can only happen if we are called from a bugcheck
             * and were never initialized, so initialize the debugger now.
             */
            KdInitSystem(0, NULL);

            /* Return success since we initialized */
            return STATUS_SUCCESS;
        }
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

NTSTATUS
NTAPI
KdDisableDebuggerWithLock(IN BOOLEAN NeedLock)
{
    KIRQL OldIrql;
    NTSTATUS Status;

#if defined(__GNUC__)
    /* Make gcc happy */
    OldIrql = PASSIVE_LEVEL;
#endif

    /*
     * If enabling the debugger is blocked
     * then there is nothing to disable (duh)
     */
    if (KdBlockEnable)
    {
        /* Fail */
        return STATUS_ACCESS_DENIED;
    }

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
        /* Check if the debugger was never actually initialized */
        if (!(KdDebuggerEnabled) && !(KdPitchDebugger))
        {
            /* It wasn't, so don't re-enable it later */
            KdPreviouslyEnabled = FALSE;
        }
        else
        {
            /* It was, so we will re-enable it later */
            KdPreviouslyEnabled = TRUE;
        }

        /* Check if we were called from the exported API and are enabled */
        if ((NeedLock) && (KdPreviouslyEnabled))
        {
            /* Check if it is safe to disable the debugger */
            Status = KdpAllowDisable();
            if (!NT_SUCCESS(Status))
            {
                /* Release the lock and fail */
                KeLowerIrql(OldIrql);
                KdpPortUnlock();
                return Status;
            }
        }

        /* Only disable the debugger if it is enabled */
        if (KdDebuggerEnabled)
        {
            /*
             * Disable the debugger; suspend breakpoints
             * and reset the debug stub
             */
            KdpSuspendAllBreakPoints();
            KiDebugRoutine = KdpStub;

            /* We are disabled now */
            KdDebuggerEnabled = FALSE;
#undef KdDebuggerEnabled
            SharedUserData->KdDebuggerEnabled = FALSE;
#define KdDebuggerEnabled _KdDebuggerEnabled
        }
     }

    /* Increment the disable count */
    KdDisableCount++;

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
    return KdEnableDebuggerWithLock(TRUE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KdDisableDebugger(VOID)
{
    /* Use the internal routine */
    return KdDisableDebuggerWithLock(TRUE);
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
BOOLEAN
NTAPI
KdRefreshDebuggerNotPresent(VOID)
{
    /* HACK */
    return KdDebuggerNotPresent;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtQueryDebugFilterState(IN ULONG ComponentId,
                        IN ULONG Level)
{
    /* HACK */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtSetDebugFilterState(IN ULONG ComponentId,
                      IN ULONG Level,
                      IN BOOLEAN State)
{
    /* HACK */
    return STATUS_SUCCESS;
}
