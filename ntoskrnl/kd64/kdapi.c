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

#ifdef KDBG
#include <kdbg/kdb.h>
#endif

#define NDEBUG
#include <debug.h>

VOID NTAPI PspDumpThreads(BOOLEAN SystemThreads);

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
KdpMoveMemory(
    _In_ PVOID Destination,
    _In_ PVOID Source,
    _In_ SIZE_T Length)
{
    PCHAR DestinationBytes, SourceBytes;

    /* Copy the buffers 1 byte at a time */
    DestinationBytes = Destination;
    SourceBytes = Source;
    while (Length--) *DestinationBytes++ = *SourceBytes++;
}

VOID
NTAPI
KdpZeroMemory(
    _In_ PVOID Destination,
    _In_ SIZE_T Length)
{
    PCHAR DestinationBytes;

    /* Zero the buffer 1 byte at a time */
    DestinationBytes = Destination;
    while (Length--) *DestinationBytes++ = 0;
}

NTSTATUS
NTAPI
KdpCopyMemoryChunks(
    _In_ ULONG64 Address,
    _In_ PVOID Buffer,
    _In_ ULONG TotalSize,
    _In_ ULONG ChunkSize,
    _In_ ULONG Flags,
    _Out_opt_ PULONG ActualSize)
{
    NTSTATUS Status;
    ULONG RemainingLength, CopyChunk;

    /* Check if we didn't get a chunk size or if it is too big */
    if (ChunkSize == 0)
    {
        /* Default to 4 byte chunks */
        ChunkSize = 4;
    }
    else if (ChunkSize > MMDBG_COPY_MAX_SIZE)
    {
        /* Normalize to maximum size */
        ChunkSize = MMDBG_COPY_MAX_SIZE;
    }

    /* Copy the whole range in aligned chunks */
    RemainingLength = TotalSize;
    CopyChunk = 1;
    while (RemainingLength > 0)
    {
        /*
         * Determine the best chunk size for this round.
         * The ideal size is aligned, isn't larger than the
         * the remaining length and respects the chunk limit.
         */
        while (((CopyChunk * 2) <= RemainingLength) &&
               (CopyChunk < ChunkSize) &&
               ((Address & ((CopyChunk * 2) - 1)) == 0))
        {
            /* Increase it */
            CopyChunk *= 2;
        }

        /*
         * The chunk size can be larger than the remaining size if this
         * isn't the first round, so check if we need to shrink it back.
         */
        while (CopyChunk > RemainingLength)
        {
            /* Shrink it */
            CopyChunk /= 2;
        }

        /* Do the copy */
        Status = MmDbgCopyMemory(Address, Buffer, CopyChunk, Flags);
        if (!NT_SUCCESS(Status))
        {
            /* Copy failed, break out */
            break;
        }

        /* Update pointers and length for the next run */
        Address = Address + CopyChunk;
        Buffer = (PVOID)((ULONG_PTR)Buffer + CopyChunk);
        RemainingLength = RemainingLength - CopyChunk;
    }

    /* We may have modified executable code, flush the instruction cache */
    KeSweepICache((PVOID)(ULONG_PTR)Address, TotalSize);

    /*
     * Return the size we managed to copy and return
     * success if we could copy the whole range.
     */
    if (ActualSize) *ActualSize = TotalSize - RemainingLength;
    return RemainingLength == 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
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
            /* Check if it's session space */
            if (MmIsSessionAddress((PVOID)(ULONG_PTR)Memory->Address))
            {
                /* It is */
                Memory->AddressSpace = DBGKD_QUERY_MEMORY_SESSION;
            }
            else
            {
                /* Not session space but some other kernel memory */
                Memory->AddressSpace = DBGKD_QUERY_MEMORY_KERNEL;
            }
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
    //PDBGKD_SEARCH_MEMORY SearchMemory = &State->u.SearchMemory;
    STRING Header;

    /* TODO */
    KdpDprintf("Memory Search support is unimplemented!\n");

    /* Send a failure packet */
    State->ReturnStatus = STATUS_UNSUCCESSFUL;
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
                 &KdpContext);
}

VOID
NTAPI
KdpFillMemory(IN PDBGKD_MANIPULATE_STATE64 State,
              IN PSTRING Data,
              IN PCONTEXT Context)
{
    //PDBGKD_FILL_MEMORY FillMemory = &State->u.FillMemory;
    STRING Header;

    /* TODO */
    KdpDprintf("Memory Fill support is unimplemented!\n");

    /* Send a failure packet */
    State->ReturnStatus = STATUS_UNSUCCESSFUL;
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
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
    //PDBGKD_BREAKPOINTEX = &State->u.BreakPointEx;
    STRING Header;

    /* TODO */
    KdpDprintf("Extended Breakpoint Write support is unimplemented!\n");

    /* Send a failure packet */
    State->ReturnStatus = STATUS_UNSUCCESSFUL;
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 Data,
                 &KdpContext);
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
KdpRestoreBreakPointEx(IN PDBGKD_MANIPULATE_STATE64 State,
                       IN PSTRING Data,
                       IN PCONTEXT Context)
{
    //PDBGKD_BREAKPOINTEX = &State->u.BreakPointEx;
    STRING Header;

    /* TODO */
    KdpDprintf("Extended Breakpoint Restore support is unimplemented!\n");

    /* Send a failure packet */
    State->ReturnStatus = STATUS_UNSUCCESSFUL;
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 Data,
                 &KdpContext);
}

VOID
NTAPI
KdpWriteCustomBreakpoint(IN PDBGKD_MANIPULATE_STATE64 State,
                         IN PSTRING Data,
                         IN PCONTEXT Context)
{
    //PDBGKD_WRITE_CUSTOM_BREAKPOINT = &State->u.WriteCustomBreakpoint;
    STRING Header;

    /* Not supported */
    KdpDprintf("Custom Breakpoint Write is unimplemented\n");

    /* Send a failure packet */
    State->ReturnStatus = STATUS_UNSUCCESSFUL;
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
                 &KdpContext);
}

VOID
NTAPI
DumpTraceData(IN PSTRING TraceData)
{
    /* Update the buffer */
    TraceDataBuffer[0] = TraceDataBufferPosition;

    /* Setup the trace data */
    TraceData->Length = (USHORT)(TraceDataBufferPosition * sizeof(ULONG));
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
    ULONG InstructionCount;
    BOOLEAN HadBreakpoints;

    /* Setup common stuff available for all CPU architectures */
    WaitStateChange->NewState = NewState;
    WaitStateChange->ProcessorLevel = KeProcessorLevel;
    WaitStateChange->Processor = (USHORT)KeGetCurrentPrcb()->Number;
    WaitStateChange->NumberProcessors = (ULONG)KeNumberProcessors;
    WaitStateChange->Thread = (ULONG64)(LONG_PTR)KeGetCurrentThread();
    WaitStateChange->ProgramCounter = (ULONG64)(LONG_PTR)KeGetContextPc(Context);

    /* Zero out the entire Control Report */
    KdpZeroMemory(&WaitStateChange->AnyControlReport,
                  sizeof(DBGKD_ANY_CONTROL_REPORT));

    /* Now copy the instruction stream and set the count */
    KdpCopyMemoryChunks((ULONG_PTR)WaitStateChange->ProgramCounter,
                        &WaitStateChange->ControlReport.InstructionStream[0],
                        DBGKD_MAXSTREAM,
                        0,
                        MMDBG_COPY_UNSAFE,
                        &InstructionCount);
    WaitStateChange->ControlReport.InstructionCount = (USHORT)InstructionCount;

    /* Clear all the breakpoints in this region */
    HadBreakpoints =
        KdpDeleteBreakpointRange((PVOID)(ULONG_PTR)WaitStateChange->ProgramCounter,
                                 (PVOID)((ULONG_PTR)WaitStateChange->ProgramCounter +
                                         WaitStateChange->ControlReport.InstructionCount - 1));
    if (HadBreakpoints)
    {
        /* Copy the instruction stream again, this time without breakpoints */
        KdpCopyMemoryChunks((ULONG_PTR)WaitStateChange->ProgramCounter,
                            &WaitStateChange->ControlReport.InstructionStream[0],
                            InstructionCount,
                            0,
                            MMDBG_COPY_UNSAFE,
                            NULL);
    }
}

VOID
NTAPI
KdpSysGetVersion(
    _Out_ PDBGKD_GET_VERSION64 Version)
{
    /* Copy the version block */
    KdpMoveMemory(Version,
                  &KdVersionBlock,
                  sizeof(DBGKD_GET_VERSION64));
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
    ReadMemory->ActualBytesRead = Length;
    Data->Length = (USHORT)Length;

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
KdpReadPhysicalMemory(IN PDBGKD_MANIPULATE_STATE64 State,
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
    else if (CacheFlags == DBGKD_CACHING_WRITE_COMBINED)
    {
        /* Write Combined */
        Flags |= MMDBG_COPY_WRITE_COMBINED;
    }

    /* Do the read */
    State->ReturnStatus = KdpCopyMemoryChunks(ReadMemory->TargetBaseAddress,
                                              Data->Buffer,
                                              Length,
                                              0,
                                              Flags,
                                              &Length);

    /* Return the actual length read */
    ReadMemory->ActualBytesRead = Length;
    Data->Length = (USHORT)Length;

    /* Send the packet */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 Data,
                 &KdpContext);
}

VOID
NTAPI
KdpWritePhysicalMemory(IN PDBGKD_MANIPULATE_STATE64 State,
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
    else if (CacheFlags == DBGKD_CACHING_WRITE_COMBINED)
    {
        /* Write Combined */
        Flags |= MMDBG_COPY_WRITE_COMBINED;
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
    ReadMemory->ActualBytesRead = Length;
    Data->Length = (USHORT)Length;

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
    PCONTEXT TargetContext;

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
            TargetContext = Context;
        }
        else
        {
            /* Get the context from the PRCB array */
            TargetContext = &KiProcessorBlock[State->Processor]->
                            ProcessorState.ContextFrame;
        }

        /* Copy it over to the debugger */
        KdpMoveMemory(Data->Buffer,
                      TargetContext,
                      sizeof(CONTEXT));
        Data->Length = sizeof(CONTEXT);

        /* Let the debugger set the context now */
        KdpContextSent = TRUE;

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
    PCONTEXT TargetContext;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == sizeof(CONTEXT));

    /* Make sure that this is a valid request */
    if ((State->Processor < KeNumberProcessors) &&
        (KdpContextSent))
    {
        /* Check if the request is for this CPU */
        if (State->Processor == KeGetCurrentPrcb()->Number)
        {
            /* We're just copying our own context */
            TargetContext = Context;
        }
        else
        {
            /* Get the context from the PRCB array */
            TargetContext = &KiProcessorBlock[State->Processor]->
                            ProcessorState.ContextFrame;
        }

        /* Copy the new context to it */
        KdpMoveMemory(TargetContext,
                      Data->Buffer,
                      sizeof(CONTEXT));

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
KdpGetContextEx(IN PDBGKD_MANIPULATE_STATE64 State,
                IN PSTRING Data,
                IN PCONTEXT Context)
{
    STRING Header;
    PDBGKD_CONTEXT_EX ContextEx;
    PCONTEXT TargetContext;
    ASSERT(Data->Length == 0);

    /* Get our struct */
    ContextEx = &State->u.ContextEx;

    /* Set up the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;

    /* Make sure that this is a valid request */
    if ((State->Processor < KeNumberProcessors) &&
        (ContextEx->Offset + ContextEx->ByteCount) <= sizeof(CONTEXT))
    {
        /* Check if the request is for this CPU */
        if (State->Processor == KeGetCurrentPrcb()->Number)
        {
            /* We're just copying our own context */
            TargetContext = Context;
        }
        else
        {
            /* Get the context from the PRCB array */
            TargetContext = &KiProcessorBlock[State->Processor]->
                            ProcessorState.ContextFrame;
        }

        /* Copy what is requested */
        KdpMoveMemory(Data->Buffer,
                      (PVOID)((ULONG_PTR)TargetContext + ContextEx->Offset),
                      ContextEx->ByteCount);

        /* KD copies all */
        Data->Length = ContextEx->BytesCopied = ContextEx->ByteCount;

        /* Let the debugger set the context now */
        KdpContextSent = TRUE;

        /* Finish up */
        State->ReturnStatus = STATUS_SUCCESS;
    }
    else
    {
        /* Invalid request */
        ContextEx->BytesCopied = 0;
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
KdpSetContextEx(IN PDBGKD_MANIPULATE_STATE64 State,
                IN PSTRING Data,
                IN PCONTEXT Context)
{
    STRING Header;
    PDBGKD_CONTEXT_EX ContextEx;
    PCONTEXT TargetContext;

    /* Get our struct */
    ContextEx = &State->u.ContextEx;
    ASSERT(Data->Length == ContextEx->ByteCount);

    /* Set up the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;

    /* Make sure that this is a valid request */
    if ((State->Processor < KeNumberProcessors) &&
        ((ContextEx->Offset + ContextEx->ByteCount) <= sizeof(CONTEXT)) &&
        (KdpContextSent))
    {
        /* Check if the request is for this CPU */
        if (State->Processor == KeGetCurrentPrcb()->Number)
        {
            /* We're just copying our own context */
            TargetContext = Context;
        }
        else
        {
            /* Get the context from the PRCB array */
            TargetContext = &KiProcessorBlock[State->Processor]->
                            ProcessorState.ContextFrame;
        }

        /* Copy what is requested */
        KdpMoveMemory((PVOID)((ULONG_PTR)TargetContext + ContextEx->Offset),
                      Data->Buffer,
                      ContextEx->ByteCount);

        /* KD copies all */
        ContextEx->BytesCopied = ContextEx->ByteCount;

        /* Finish up */
        State->ReturnStatus = STATUS_SUCCESS;
    }
    else
    {
        /* Invalid request */
        ContextEx->BytesCopied = 0;
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
    ULARGE_INTEGER MsrValue;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == 0);

    /* Call the internal routine */
    State->ReturnStatus = KdpSysReadMsr(ReadMsr->Msr, &MsrValue.QuadPart);

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
    ULARGE_INTEGER MsrValue;

    /* Setup the header */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;
    ASSERT(Data->Length == 0);

    /* Call the internal routine */
    MsrValue.LowPart = WriteMsr->DataValueLow;
    MsrValue.HighPart = WriteMsr->DataValueHigh;
    State->ReturnStatus = KdpSysWriteMsr(WriteMsr->Msr, &MsrValue.QuadPart);

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
    GetBusData->Length = Length;
    Data->Length = (USHORT)Length;

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
                                             &SetBusData->Length);

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
    State->ReturnStatus = KdpSysWriteIoSpace(WriteIoExtended->InterfaceType,
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

VOID
NTAPI
KdpNotSupported(IN PDBGKD_MANIPULATE_STATE64 State)
{
    STRING Header;

    /* Set failure */
    State->ReturnStatus = STATUS_UNSUCCESSFUL;

    /* Setup the packet */
    Header.Length = sizeof(DBGKD_MANIPULATE_STATE64);
    Header.Buffer = (PCHAR)State;

    /* Send it */
    KdSendPacket(PACKET_TYPE_KD_STATE_MANIPULATE,
                 &Header,
                 NULL,
                 &KdpContext);
}

static
KCONTINUE_STATUS
KdpSwitchProcessor(
    _In_ USHORT ProcessorIndex)
{
    /* Make sure that the processor index is valid */
    if (ProcessorIndex >= KeNumberProcessors)
    {
        KdpDprintf("%u is not a valid processor number\n", ProcessorIndex);
        return ContinueProcessorReselected;
    }

    /* If the new processor is the current one, there is nothing to do */
    if (ProcessorIndex == KeGetCurrentProcessorNumber())
    {
        return ContinueProcessorReselected;
    }

    /* Call the architecture specific Ke routine */
    return KxSwitchKdProcessor(ProcessorIndex);
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

    /*
     * Reset the context state to ensure the debugger has received
     * the current context before it sets it.
     */
    KdpContextSent = FALSE;

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
                KdpReadPhysicalMemory(&ManipulateState, &Data, Context);
                break;

            case DbgKdWritePhysicalMemoryApi:

                /* Write  physical memory */
                KdpWritePhysicalMemory(&ManipulateState, &Data, Context);
                break;

            case DbgKdQuerySpecialCallsApi:
            case DbgKdSetSpecialCallApi:
            case DbgKdClearSpecialCallsApi:

                /* TODO */
                KdpDprintf("Special Call support is unimplemented!\n");
                KdpNotSupported(&ManipulateState);
                break;

            case DbgKdSetInternalBreakPointApi:
            case DbgKdGetInternalBreakPointApi:

                /* TODO */
                KdpDprintf("Internal Breakpoint support is unimplemented!\n");
                KdpNotSupported(&ManipulateState);
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

                /* Switch the processor and return */
                return KdpSwitchProcessor(ManipulateState.Processor);

            case DbgKdPageInApi:

                /* This API, introduced in NT4, has been obsoleted in NT5. It is
                 * replaced by ExpDebuggerPageIn support in ExpDebuggerWorker(). */
                KdpDprintf("DbgKdPageInApi is obsolete!\n");
                KdpNotSupported(&ManipulateState);
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

                /* TODO */
                KdpDprintf("Partition Switch support is unimplemented!\n");
                KdpNotSupported(&ManipulateState);
                break;

            case DbgKdWriteCustomBreakpointApi:

                /* Write the customized breakpoint */
                KdpWriteCustomBreakpoint(&ManipulateState, &Data, Context);
                break;

            case DbgKdGetContextExApi:

                /* Extended Context Get */
                KdpGetContextEx(&ManipulateState, &Data, Context);
                break;

            case DbgKdSetContextExApi:

                /* Extended Context Set */
                KdpSetContextEx(&ManipulateState, &Data, Context);
                break;

            /* Unsupported Messages */
            default:

                /* Send warning */
                KdpDprintf("Received Unrecognized API 0x%lx\n", ManipulateState.ApiNumber);

                /* Setup an empty message, with failure */
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

VOID
NTAPI
KdpReportLoadSymbolsStateChange(IN PSTRING PathName,
                                IN PKD_SYMBOLS_INFO SymbolInfo,
                                IN BOOLEAN Unload,
                                IN OUT PCONTEXT Context)
{
    PSTRING ExtraData;
    STRING Data, Header;
    DBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange;
    ULONG PathNameLength;
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

        /* Check if we have a path name */
        if (PathName)
        {
            /* Copy it to the path buffer */
            KdpCopyMemoryChunks((ULONG_PTR)PathName->Buffer,
                                KdpPathBuffer,
                                PathName->Length,
                                0,
                                MMDBG_COPY_UNSAFE,
                                &PathNameLength);

            /* Null terminate */
            KdpPathBuffer[PathNameLength++] = ANSI_NULL;

            /* Set the path length */
            WaitStateChange.u.LoadSymbols.PathNameLength = PathNameLength;

            /* Set up the data */
            Data.Buffer = KdpPathBuffer;
            Data.Length = (USHORT)PathNameLength;
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
}

VOID
NTAPI
KdpReportCommandStringStateChange(IN PSTRING NameString,
                                  IN PSTRING CommandString,
                                  IN OUT PCONTEXT Context)
{
    STRING Header, Data;
    DBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange;
    ULONG Length, ActualLength, TotalLength;
    KCONTINUE_STATUS Status;

    /* Start wait loop */
    do
    {
        /* Build the architecture common parts of the message */
        KdpSetCommonState(DbgKdCommandStringStateChange,
                          Context,
                          &WaitStateChange);

        /* Set the context */
        KdpSetContextState(&WaitStateChange, Context);

        /* Clear the command string structure */
        KdpZeroMemory(&WaitStateChange.u.CommandString,
                      sizeof(DBGKD_COMMAND_STRING));

        /* Normalize name string to max */
        Length = min(128 - 1, NameString->Length);

        /* Copy it to the message buffer */
        KdpCopyMemoryChunks((ULONG_PTR)NameString->Buffer,
                            KdpMessageBuffer,
                            Length,
                            0,
                            MMDBG_COPY_UNSAFE,
                            &ActualLength);

        /* Null terminate and calculate the total length */
        TotalLength = ActualLength;
        KdpMessageBuffer[TotalLength++] = ANSI_NULL;

        /* Check if the command string is too long */
        Length = CommandString->Length;
        if (Length > (PACKET_MAX_SIZE -
                      sizeof(DBGKD_ANY_WAIT_STATE_CHANGE) - TotalLength))
        {
            /* Use maximum possible size */
            Length = (PACKET_MAX_SIZE -
                      sizeof(DBGKD_ANY_WAIT_STATE_CHANGE) - TotalLength);
        }

        /* Copy it to the message buffer */
        KdpCopyMemoryChunks((ULONG_PTR)CommandString->Buffer,
                            KdpMessageBuffer + TotalLength,
                            Length,
                            0,
                            MMDBG_COPY_UNSAFE,
                            &ActualLength);

        /* Null terminate and calculate the total length */
        TotalLength += ActualLength;
        KdpMessageBuffer[TotalLength++] = ANSI_NULL;

        /* Now set up the header and the data */
        Header.Length = sizeof(DBGKD_ANY_WAIT_STATE_CHANGE);
        Header.Buffer = (PCHAR)&WaitStateChange;
        Data.Length = (USHORT)TotalLength;
        Data.Buffer = KdpMessageBuffer;

        /* Send State Change packet and wait for a reply */
        Status = KdpSendWaitContinue(PACKET_TYPE_KD_STATE_CHANGE64,
                                     &Header,
                                     &Data,
                                     Context);
    } while (Status == ContinueProcessorReselected);
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

#if !defined(_WIN64)

        /* Convert it and copy it over */
        ExceptionRecord32To64((PEXCEPTION_RECORD32)ExceptionRecord,
                              &WaitStateChange.u.Exception.ExceptionRecord);

#else

        /* Just copy it directly, no need to convert */
        KdpMoveMemory(&WaitStateChange.u.Exception.ExceptionRecord,
                      ExceptionRecord,
                      sizeof(EXCEPTION_RECORD));

#endif

        /* Set the First Chance flag */
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

KCONTINUE_STATUS
NTAPI
KdReportProcessorChange(
    VOID)
{
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();
    PCONTEXT ContextRecord = &CurrentPrcb->ProcessorState.ContextFrame;
    EXCEPTION_RECORD ExceptionRecord = {0};
    KCONTINUE_STATUS Status;

    /* Save the port data */
    KdSave(FALSE);

    ExceptionRecord.ExceptionAddress = (PVOID)KeGetContextPc(ContextRecord);
    ExceptionRecord.ExceptionCode = STATUS_WAKE_SYSTEM_DEBUGGER;

    /* Report the new state */
    Status = KdpReportExceptionStateChange(&ExceptionRecord,
                                           ContextRecord,
                                           FALSE);

    /* Restore the port data */
    KdRestore(FALSE);

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

BOOLEAN
NTAPI
KdEnterDebugger(IN PKTRAP_FRAME TrapFrame,
                IN PKEXCEPTION_FRAME ExceptionFrame)
{
    BOOLEAN Enable;

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

    /* Freeze all CPUs, raising also the IRQL to HIGH_LEVEL */
    Enable = KeFreezeExecution(TrapFrame, ExceptionFrame);

    /* Lock the port, save its state and set the debugger entered flag */
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

    /* Return if interrupts needs to be re-enabled */
    return Enable;
}

VOID
NTAPI
KdExitDebugger(IN BOOLEAN Enable)
{
    ULONG TimeSlip;

    /* Reset the debugger entered flag, restore the port state and unlock it */
    KdEnteredDebugger = FALSE;
    KdRestore(FALSE);
    if (KdpPortLocked) KdpPortUnlock();

    /* Unfreeze the CPUs, restoring also the IRQL */
    KeThawExecution(Enable);

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
        KeInsertQueueDpc(&KdpTimeSlipDpc, NULL, NULL); // FIXME: this can trigger context switches!
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
            KdpPortUnlock();
            KeLowerIrql(OldIrql);

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
            KdInitSystem(0, NULL);
            KdpRestoreAllBreakpoints();
        }
    }

    /* Check if we had locked the port before */
    if (NeedLock)
    {
        /* Yes, now unlock it */
        KdpPortUnlock();
        KeLowerIrql(OldIrql);
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
                KdpPortUnlock();
                KeLowerIrql(OldIrql);
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
            SharedUserData->KdDebuggerEnabled = FALSE;
        }
     }

    /* Increment the disable count */
    KdDisableCount++;

    /* Check if we had locked the port before */
    if (NeedLock)
    {
        /* Yes, now unlock it */
        KdpPortUnlock();
        KeLowerIrql(OldIrql);
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

/**
 * @brief
 * Perform various queries to the kernel debugger.
 *
 * @param[in]   Command
 * A SYSDBG_COMMAND value describing the kernel debugger command to perform.
 *
 * @param[in]   InputBuffer
 * Pointer to a user-provided input command-specific buffer, whose length
 * is given by InputBufferLength.
 *
 * @param[in]   InputBufferLength
 * The size (in bytes) of the buffer pointed by InputBuffer.
 *
 * @param[out]  OutputBuffer
 * Pointer to a user-provided command-specific output buffer, whose length
 * is given by OutputBufferLength.
 *
 * @param[in]   OutputBufferLength
 * The size (in bytes) of the buffer pointed by OutputBuffer.
 *
 * @param[out]  ReturnLength
 * Optional pointer to a ULONG variable that receives the actual length of
 * data written written in the output buffer. It is always zero, except for
 * the live dump commands where an actual non-zero length is returned.
 *
 * @param[in]   PreviousMode
 * The processor mode (KernelMode or UserMode) in which the command is being executed.
 *
 * @return
 * STATUS_SUCCESS in case of success, or a proper error code otherwise.
 *
 * @remarks
 * - This is a kernel-mode function, accessible only by kernel-mode drivers.
 *
 * @note
 * See: https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2004-2339
 *
 * @see NtSystemDebugControl()
 **/
NTSTATUS
NTAPI
KdSystemDebugControl(
    _In_ SYSDBG_COMMAND Command,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    NTSTATUS Status;
    ULONG Length = 0;

    /* Handle some internal commands */
    switch ((ULONG)Command)
    {
#if DBG
        case ' soR': /* ROS-INTERNAL */
        {
            switch ((ULONG_PTR)InputBuffer)
            {
                case 0x21: // DumpAllThreads:
                    PspDumpThreads(TRUE);
                    break;

                case 0x22: // DumpUserThreads:
                    PspDumpThreads(FALSE);
                    break;

                case 0x24: // KdSpare3:
                    MmDumpArmPfnDatabase(FALSE);
                    break;

                default:
                    break;
            }
            return STATUS_SUCCESS;
        }

#if defined(_M_IX86) && !defined(_WINKD_) // See ke/i386/traphdlr.c
        /* Register a debug callback */
        case 'CsoR':
        {
            switch (InputBufferLength)
            {
                case ID_Win32PreServiceHook:
                    KeWin32PreServiceHook = InputBuffer;
                    break;

                case ID_Win32PostServiceHook:
                    KeWin32PostServiceHook = InputBuffer;
                    break;

            }
            break;
        }
#endif

        /* Special case for stack frame dumps */
        case 'DsoR':
        {
            KeRosDumpStackFrames((PULONG_PTR)InputBuffer, InputBufferLength);
            break;
        }
#ifdef KDBG
        /* Register KDBG CLI callback */
        case 'RbdK':
        {
            return KdbRegisterCliCallback(InputBuffer, InputBufferLength);
        }
#endif // KDBG
#endif
        default:
            break;
    }

    switch (Command)
    {
        case SysDbgQueryVersion:
            if (OutputBufferLength != sizeof(DBGKD_GET_VERSION64))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                KdpSysGetVersion((PDBGKD_GET_VERSION64)OutputBuffer);
                Status = STATUS_SUCCESS;
            }
            break;

        case SysDbgReadVirtual:
        case SysDbgWriteVirtual:
            if (InputBufferLength != sizeof(SYSDBG_VIRTUAL))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                SYSDBG_VIRTUAL Request = *(PSYSDBG_VIRTUAL)InputBuffer;
                PVOID LockedBuffer;
                PMDL LockVariable;

                Status = ExLockUserBuffer(Request.Buffer,
                                          Request.Request,
                                          PreviousMode,
                                          Command == SysDbgReadVirtual ? IoWriteAccess : IoReadAccess,
                                          &LockedBuffer,
                                          &LockVariable);
                if (NT_SUCCESS(Status))
                {
                    Status = KdpCopyMemoryChunks((ULONG64)(ULONG_PTR)Request.Address,
                                                 Request.Buffer,
                                                 Request.Request,
                                                 0,
                                                 Command == SysDbgReadVirtual ? 0 : MMDBG_COPY_WRITE,
                                                 &Length);
                    ExUnlockUserBuffer(LockVariable);
                }
            }
            break;

        case SysDbgReadPhysical:
        case SysDbgWritePhysical:
            if (InputBufferLength != sizeof(SYSDBG_PHYSICAL))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                SYSDBG_PHYSICAL Request = *(PSYSDBG_PHYSICAL)InputBuffer;
                PVOID LockedBuffer;
                PMDL LockVariable;

                Status = ExLockUserBuffer(Request.Buffer,
                                          Request.Request,
                                          PreviousMode,
                                          Command == SysDbgReadVirtual ? IoWriteAccess : IoReadAccess,
                                          &LockedBuffer,
                                          &LockVariable);
                if (NT_SUCCESS(Status))
                {
                    Status = KdpCopyMemoryChunks(Request.Address.QuadPart,
                                                 Request.Buffer,
                                                 Request.Request,
                                                 0,
                                                 MMDBG_COPY_PHYSICAL | (Command == SysDbgReadVirtual ? 0 : MMDBG_COPY_WRITE),
                                                 &Length);
                    ExUnlockUserBuffer(LockVariable);
                }
            }
            break;

        case SysDbgReadControlSpace:
            if (InputBufferLength != sizeof(SYSDBG_CONTROL_SPACE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                SYSDBG_CONTROL_SPACE Request = *(PSYSDBG_CONTROL_SPACE)InputBuffer;
                PVOID LockedBuffer;
                PMDL LockVariable;

                Status = ExLockUserBuffer(Request.Buffer,
                                          Request.Request,
                                          PreviousMode,
                                          IoWriteAccess,
                                          &LockedBuffer,
                                          &LockVariable);
                if (NT_SUCCESS(Status))
                {
                    Status = KdpSysReadControlSpace(Request.Processor,
                                                    Request.Address,
                                                    LockedBuffer,
                                                    Request.Request,
                                                    &Length);
                    ExUnlockUserBuffer(LockVariable);
                }
            }
            break;

        case SysDbgWriteControlSpace:
            if (InputBufferLength != sizeof(SYSDBG_CONTROL_SPACE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                SYSDBG_CONTROL_SPACE Request = *(PSYSDBG_CONTROL_SPACE)InputBuffer;
                PVOID LockedBuffer;
                PMDL LockVariable;

                Status = ExLockUserBuffer(Request.Buffer,
                                          Request.Request,
                                          PreviousMode,
                                          IoReadAccess,
                                          &LockedBuffer,
                                          &LockVariable);
                if (NT_SUCCESS(Status))
                {
                    Status = KdpSysWriteControlSpace(Request.Processor,
                                                     Request.Address,
                                                     LockedBuffer,
                                                     Request.Request,
                                                     &Length);
                    ExUnlockUserBuffer(LockVariable);
                }
            }
            break;

        case SysDbgReadIoSpace:
            if (InputBufferLength != sizeof(SYSDBG_IO_SPACE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                SYSDBG_IO_SPACE Request = *(PSYSDBG_IO_SPACE)InputBuffer;
                PVOID LockedBuffer;
                PMDL LockVariable;

                Status = ExLockUserBuffer(Request.Buffer,
                                          Request.Request,
                                          PreviousMode,
                                          IoWriteAccess,
                                          &LockedBuffer,
                                          &LockVariable);
                if (NT_SUCCESS(Status))
                {
                    Status = KdpSysReadIoSpace(Request.InterfaceType,
                                               Request.BusNumber,
                                               Request.AddressSpace,
                                               Request.Address,
                                               LockedBuffer,
                                               Request.Request,
                                               &Length);
                    ExUnlockUserBuffer(LockVariable);
                }
            }
            break;

        case SysDbgWriteIoSpace:
            if (InputBufferLength != sizeof(SYSDBG_IO_SPACE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                SYSDBG_IO_SPACE Request = *(PSYSDBG_IO_SPACE)InputBuffer;
                PVOID LockedBuffer;
                PMDL LockVariable;

                Status = ExLockUserBuffer(Request.Buffer,
                                          Request.Request,
                                          PreviousMode,
                                          IoReadAccess,
                                          &LockedBuffer,
                                          &LockVariable);
                if (NT_SUCCESS(Status))
                {
                    Status = KdpSysWriteIoSpace(Request.InterfaceType,
                                                Request.BusNumber,
                                                Request.AddressSpace,
                                                Request.Address,
                                                LockedBuffer,
                                                Request.Request,
                                                &Length);
                    ExUnlockUserBuffer(LockVariable);
                }
            }
            break;

        case SysDbgReadMsr:
            if (InputBufferLength != sizeof(SYSDBG_MSR))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                PSYSDBG_MSR Request = (PSYSDBG_MSR)InputBuffer;
                Status = KdpSysReadMsr(Request->Address, &Request->Data);
            }
            break;

        case SysDbgWriteMsr:
            if (InputBufferLength != sizeof(SYSDBG_MSR))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                PSYSDBG_MSR Request = (PSYSDBG_MSR)InputBuffer;
                Status = KdpSysWriteMsr(Request->Address, &Request->Data);
            }
            break;

        case SysDbgReadBusData:
            if (InputBufferLength != sizeof(SYSDBG_BUS_DATA))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                SYSDBG_BUS_DATA Request = *(PSYSDBG_BUS_DATA)InputBuffer;
                PVOID LockedBuffer;
                PMDL LockVariable;

                Status = ExLockUserBuffer(Request.Buffer,
                                          Request.Request,
                                          PreviousMode,
                                          IoWriteAccess,
                                          &LockedBuffer,
                                          &LockVariable);
                if (NT_SUCCESS(Status))
                {
                    Status = KdpSysReadBusData(Request.BusDataType,
                                               Request.BusNumber,
                                               Request.SlotNumber,
                                               Request.Address,
                                               LockedBuffer,
                                               Request.Request,
                                               &Length);
                    ExUnlockUserBuffer(LockVariable);
                }
            }
            break;

        case SysDbgWriteBusData:
            if (InputBufferLength != sizeof(SYSDBG_BUS_DATA))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                SYSDBG_BUS_DATA Request = *(PSYSDBG_BUS_DATA)InputBuffer;
                PVOID LockedBuffer;
                PMDL LockVariable;

                Status = ExLockUserBuffer(Request.Buffer,
                                          Request.Request,
                                          PreviousMode,
                                          IoReadAccess,
                                          &LockedBuffer,
                                          &LockVariable);
                if (NT_SUCCESS(Status))
                {
                    Status = KdpSysWriteBusData(Request.BusDataType,
                                                Request.BusNumber,
                                                Request.SlotNumber,
                                                Request.Address,
                                                LockedBuffer,
                                                Request.Request,
                                                &Length);
                    ExUnlockUserBuffer(LockVariable);
                }
            }
            break;

        case SysDbgCheckLowMemory:
            Status = KdpSysCheckLowMemory(0);
            break;

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    if (ReturnLength)
        *ReturnLength = Length;

    return Status;
}

/*
 * @implemented
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
    /* Fail if there is no debugger */
    if (KdPitchDebugger)
    {
        /* No debugger, no options */
        return STATUS_DEBUGGER_INACTIVE;
    }

    /* Do we recognize this option? */
    if (Option != KD_OPTION_SET_BLOCK_ENABLE)
    {
        /* We don't, clear the output length and fail */
        if (OutBufferNeeded) *OutBufferNeeded = 0;
        return STATUS_INVALID_INFO_CLASS;
    }

    /* Verify parameters */
    if ((InBufferBytes != sizeof(BOOLEAN)) ||
        (OutBufferBytes != 0) ||
        (OutBuffer != NULL))
    {
        /* Invalid parameters for this option, fail */
        return STATUS_INVALID_PARAMETER;
    }

    /*
     * Check if the high bit is set, meaning we don't
     * allow the debugger to be enabled
     */
    if (KdBlockEnable & 0x80)
    {
        /* Fail regardless of what state the caller tried to set */
        return STATUS_ACCESS_VIOLATION;
    }

    /* Set the new block enable state */
    KdBlockEnable = *(PBOOLEAN)InBuffer;

    /* No output buffer required for this option */
    if (OutBufferNeeded) *OutBufferNeeded = 0;

    /* We are done */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KdPowerTransition(IN DEVICE_POWER_STATE NewState)
{
    /* Check what power state this is */
    if (NewState == PowerDeviceD0)
    {
        /* Wake up the debug port */
        KdD0Transition();
        return STATUS_SUCCESS;
    }
    else if ((NewState == PowerDeviceD1) ||
             (NewState == PowerDeviceD2) ||
             (NewState == PowerDeviceD3))
    {
        /* Power down the debug port */
        KdD3Transition();
        return STATUS_SUCCESS;
    }
    else
    {
        /* Invalid state! */
        return STATUS_INVALID_PARAMETER_1;
    }
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KdRefreshDebuggerNotPresent(VOID)
{
    BOOLEAN Enable, DebuggerNotPresent;

    /* Check if the debugger is completely disabled */
    if (KdPitchDebugger)
    {
        /* Don't try to refresh then, fail early */
        return TRUE;
    }

    /* Enter the debugger */
    Enable = KdEnterDebugger(NULL, NULL);

    /*
     * Attempt to send a string to the debugger
     * to refresh the connection state.
     */
    KdpDprintf("KDTARGET: Refreshing KD connection\n");

    /* Save the state while we are holding the lock */
    DebuggerNotPresent = KdDebuggerNotPresent;

    /* Exit the debugger and return the state */
    KdExitDebugger(Enable);
    return DebuggerNotPresent;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryDebugFilterState(
    _In_ ULONG ComponentId,
    _In_ ULONG Level)
{
    PULONG Mask;

    /* Check if the ID fits in the component table */
    if (ComponentId < KdComponentTableSize)
    {
        /* It does, so get the mask from there */
        Mask = KdComponentTable[ComponentId];
    }
    else if (ComponentId == MAXULONG)
    {
        /*
         * This is the internal ID used for DbgPrint messages without ID
         * and Level. Use the system-wide mask for those.
         */
        Mask = &Kd_WIN2000_Mask;
    }
    else
    {
#if (NTDDI_VERSION >= NTDDI_VISTA)
        /* Use the default component ID */
        Mask = &Kd_DEFAULT_Mask;
        // Level = DPFLTR_INFO_LEVEL; // Override the Level.
#else
        /* Invalid ID, fail */
        return STATUS_INVALID_PARAMETER_1;
#endif
    }

    /* Convert Level to bit field if required */
    if (Level < 32) Level = 1 << Level;
    Level &= ~DPFLTR_MASK;

    /* Determine if this Level is filtered out */
    if ((Kd_WIN2000_Mask & Level) || (*Mask & Level))
    {
        /* This mask will get through to the debugger */
        return (NTSTATUS)TRUE;
    }
    else
    {
        /* This mask is filtered out */
        return (NTSTATUS)FALSE;
    }
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtSetDebugFilterState(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_ BOOLEAN State)
{
    PULONG Mask;

    /* Modifying debug filters requires the debug privilege */
    if (!SeSinglePrivilegeCheck(SeDebugPrivilege, ExGetPreviousMode()))
    {
        /* Fail */
        return STATUS_ACCESS_DENIED;
    }

    /* Check if the ID fits in the component table */
    if (ComponentId < KdComponentTableSize)
    {
        /* It does, so get the mask from there */
        Mask = KdComponentTable[ComponentId];
    }
    else if (ComponentId == MAXULONG)
    {
        /*
         * This is the internal ID used for DbgPrint messages without ID
         * and Level. Use the system-wide mask for those.
         */
        Mask = &Kd_WIN2000_Mask;
    }
    else
    {
#if (NTDDI_VERSION >= NTDDI_VISTA)
        /* Use the default component ID */
        Mask = &Kd_DEFAULT_Mask;
#else
        /* Invalid ID, fail */
        return STATUS_INVALID_PARAMETER_1;
#endif
    }

    /* Convert Level to bit field if required */
    if (Level < 32) Level = 1 << Level;
    Level &= ~DPFLTR_MASK;

    /* Set or remove the Level */
    if (State)
        *Mask |= Level;
    else
        *Mask &= ~Level;

    return STATUS_SUCCESS;
}
