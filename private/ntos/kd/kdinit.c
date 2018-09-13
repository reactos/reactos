/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    kdinit.c

Abstract:

    This module implements the initialization for the portable kernel debgger.

Author:

    David N. Cutler 27-July-1990

Revision History:

--*/

#include "kdp.h"

//
// Miscellaneous data from all over the kernel
//

#define BAUD_OPTION "BAUDRATE"
#define PORT_OPTION "DEBUGPORT"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEKD, KdInitSystem)
#endif

BOOLEAN
KdInitSystem(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL,
    BOOLEAN StopInDebugger
    )

/*++

Routine Description:

    This routine initializes the portable kernel debugger.

Arguments:

    LoaderBlock - Supplies a pointer to the LOADER_PARAMETER_BLOCK passed
        in from the OS Loader.

    StopInDebugger - Supplies a boolean value that determines whether a
        debug message and breakpoint are generated.

Return Value:

    None.

--*/

{

    ULONG Index;
    BOOLEAN Initialize;
    PCHAR Options;
    PCHAR BaudOption;
    PCHAR PortOption;

    //
    // If kernel debugger is already initialized, then return.
    //

    if (KdDebuggerEnabled != FALSE) {
        return TRUE;
    }

    KiDebugRoutine = KdpStub;

    //
    // Determine whether or not the debugger should be enabled.
    //
    // Note that if LoaderBlock == NULL, then KdInitSystem was called
    // from BugCheck code. For this case the debugger is always enabled
    // to report the bugcheck if possible.
    //

    if (LoaderBlock != NULL) {

        KdpNtosImageBase =  CONTAINING_RECORD(
                                    (LoaderBlock->LoadOrderListHead.Flink),
                                    LDR_DATA_TABLE_ENTRY,
                                    InLoadOrderLinks)->DllBase;

        //
        // Initialize the debugger data block list when called at startup time.
        //

        InitializeListHead(&KdpDebuggerDataListHead);

        //
        // Fill in and register the debugger's debugger data block.
        // Most fields are already initialized, some fields will not be
        // filled in until later.
        //

        KdDebuggerDataBlock.KernBase = (ULONG_PTR) KdpNtosImageBase;
        KdDebuggerDataBlock.KeUserCallbackDispatcher = (ULONG_PTR) KeUserCallbackDispatcher;

        KdRegisterDebuggerDataBlock(KDBG_TAG,
                                    &KdDebuggerDataBlock.Header,
                                    sizeof(KdDebuggerDataBlock));

        if (LoaderBlock->LoadOptions != NULL) {
            Options = LoaderBlock->LoadOptions;
            _strupr(Options);

            //
            // If any of the port option, baud option, or debug is specified,
            // then enable the debugger unless it is explictly disabled.
            //

            Initialize = TRUE;
            PortOption = strstr(Options, PORT_OPTION);
            BaudOption = strstr(Options, BAUD_OPTION);
            if ((PortOption == NULL) && (BaudOption == NULL)) {
                if (strstr(Options, "DEBUG") == NULL) {
                    Initialize = FALSE;
                }

            } else {
                if (PortOption) {
                    PortOption = strstr(PortOption, "COM");
                    if (PortOption) {
                        KdDebugParameters.CommunicationPort =
                                                     atol(PortOption + 3);
                    }
                }

                if (BaudOption) {
                    BaudOption += strlen(BAUD_OPTION);
                    while (*BaudOption == ' ') {
                        BaudOption++;
                    }

                    if (*BaudOption != '\0') {
                        KdDebugParameters.BaudRate = atol(BaudOption + 1);
                    }
                }
            }

            //
            // If the debugger is explicitly disabled, then set to NODEBUG.
            //

            if (strstr(Options, "NODEBUG")) {
                Initialize = FALSE;
                KdPitchDebugger = TRUE;
            }

            if (strstr(Options, "CRASHDEBUG")) {
                Initialize = FALSE;
                KdPitchDebugger = FALSE;
            }

        } else {

            //
            // If the load options are not specified, then set to NODEBUG.
            //

            KdPitchDebugger = TRUE;
            Initialize = FALSE;
        }

    } else {
        Initialize = TRUE;
    }

    if ((KdPortInitialize(&KdDebugParameters, LoaderBlock, Initialize) == FALSE) ||
        (Initialize == FALSE)) {
        return(TRUE);
    }

    //
    // Set address of kernel debugger trap routine.
    //

    KiDebugRoutine = KdpTrap;

    if (!KdpDebuggerStructuresInitialized) {

        KiDebugSwitchRoutine = KdpSwitchProcessor;
        KdpBreakpointInstruction = KDP_BREAKPOINT_VALUE;
        KdpOweBreakpoint = FALSE;

        //
        // Initialize the breakpoint table.
        //

        for (Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index += 1) {
            KdpBreakpointTable[Index].Flags = 0;
            KdpBreakpointTable[Index].Address = NULL;
            KdpBreakpointTable[Index].DirectoryTableBase = 0L;
        }

        //
        // Initialize TimeSlip
        //
        KeInitializeDpc(&KdpTimeSlipDpc, KdpTimeSlipDpcRoutine, NULL);
        KeInitializeTimer(&KdpTimeSlipTimer);
        ExInitializeWorkItem(&KdpTimeSlipWorkItem, KdpTimeSlipWork, NULL);

        KdpDebuggerStructuresInitialized = TRUE ;
    }

    //
    //  Initialize timer facility - HACKHACK
    //

    KeQueryPerformanceCounter(&KdPerformanceCounterRate);
    KdTimerStart.HighPart = 0L;
    KdTimerStart.LowPart = 0L;

    //
    // Initialize ID for NEXT packet to send and Expect ID of next incoming
    // packet.
    //

    KdpNextPacketIdToSend = INITIAL_PACKET_ID | SYNC_PACKET_ID;
    KdpPacketIdExpected = INITIAL_PACKET_ID;

    //
    // Mark debugger enabled.
    //
    KdPitchDebugger = FALSE;
    KdDebuggerEnabled = TRUE;
    SharedUserData->KdDebuggerEnabled = TRUE;

    //
    // If requested, stop in the kernel debugger.
    //

    if (StopInDebugger) {
        DbgBreakPoint();
    }

    return TRUE;
}


BOOLEAN
KdRegisterDebuggerDataBlock(
    IN ULONG Tag,
    IN PDBGKD_DEBUG_DATA_HEADER DataHeader,
    IN ULONG Size
    )
/*++

Routine Description:

    This routine is called by a component or driver to register a
    debugger data block.  The data block is made accessible to the
    kernel debugger, thus providing a reliable method of exposing
    random data to debugger extensions.

Arguments:

    Tag - Supplies a unique 4 byte tag which is used to identify the
            data block.

    DataHeader - Supplies the address of the debugger data block header.
            The OwnerTag field must contain a unique value, and the Size
            field must contain the size of the data block, including the
            header.  If this block is already present, or there is
            already a block with the same value for OwnerTag, this one
            will not be inserted.  If Size is incorrect, this code will
            not notice, but the usermode side of the debugger might not
            function correctly.

    Size - Supplies the size of the data block, including the header.

Return Value:

    TRUE if the block was added to the list, FALSE if not.

--*/
{
    KIRQL OldIrql;
    PLIST_ENTRY List;
    PDBGKD_DEBUG_DATA_HEADER Header;

    KeAcquireSpinLock(&KdpDataSpinLock, &OldIrql);

    //
    // Look for a record with the same tag or address
    //

    List = KdpDebuggerDataListHead.Flink;

    while (List != &KdpDebuggerDataListHead) {

        Header = CONTAINING_RECORD(List, DBGKD_DEBUG_DATA_HEADER, List);

        List = List->Flink;

        if ((Header == DataHeader) || (Header->OwnerTag == Tag)) {
            KeReleaseSpinLock(&KdpDataSpinLock, OldIrql);
            return FALSE;
        }
    }

    //
    // It wasn't already there, so add it.
    //

    DataHeader->OwnerTag = Tag;
    DataHeader->Size = Size;

    InsertTailList(&KdpDebuggerDataListHead, &DataHeader->List);

    KeReleaseSpinLock(&KdpDataSpinLock, OldIrql);

    return TRUE;
}


VOID
KdDeregisterDebuggerDataBlock(
    IN PDBGKD_DEBUG_DATA_HEADER DataHeader
    )
/*++

Routine Description:

    This routine is called to deregister a data block previously
    registered with KdRegisterDebuggerDataBlock.  If the block is
    found in the list, it is removed.

Arguments:

    DataHeader - Supplies the address of the data block which is
                to be removed from the list.

Return Value:

    None

--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY List;
    PDBGKD_DEBUG_DATA_HEADER Header;

    KeAcquireSpinLock(&KdpDataSpinLock, &OldIrql);

    //
    // Make sure the data block is on our list before removing it.
    //

    List = KdpDebuggerDataListHead.Flink;

    while (List != &KdpDebuggerDataListHead) {

        Header = CONTAINING_RECORD(List, DBGKD_DEBUG_DATA_HEADER, List);
        List = List->Flink;

        if (DataHeader == Header) {
            RemoveEntryList(&DataHeader->List);
            break;
        }
    }

    KeReleaseSpinLock(&KdpDataSpinLock, OldIrql);
}


VOID
KdLogDbgPrint(
    IN PSTRING String
    )
{
    KIRQL OldIrql;
    ULONG Length;
    ULONG LengthCopied;

    for (; ;) {
        if (KeTestSpinLock (&KdpPrintSpinLock)) {
            KeRaiseIrql (HIGH_LEVEL, &OldIrql);
            if (KiTryToAcquireSpinLock(&KdpPrintSpinLock)) {
                break;          // got the lock
            }
            KeLowerIrql(OldIrql);
        }
    }

    if (KdPrintCircularBuffer) {
        Length = String->Length;
        //
        // truncate ridiculous strings
        //
        if (Length > KDPRINTBUFFERSIZE) {
            Length = KDPRINTBUFFERSIZE;
        }

        if (KdPrintWritePointer + Length <= KdPrintCircularBuffer+KDPRINTBUFFERSIZE) {
            LengthCopied = KdpMoveMemory(KdPrintWritePointer, String->Buffer, Length);
            KdPrintWritePointer += LengthCopied;
            if (KdPrintWritePointer >= KdPrintCircularBuffer+KDPRINTBUFFERSIZE) {
                KdPrintWritePointer = KdPrintCircularBuffer;
                KdPrintRolloverCount++;
            }
        } else {
            ULONG First = (ULONG)(KdPrintCircularBuffer + KDPRINTBUFFERSIZE - KdPrintWritePointer);
            LengthCopied = KdpMoveMemory(KdPrintWritePointer,
                                         String->Buffer,
                                         First);
            if (LengthCopied == First) {
                LengthCopied += KdpMoveMemory(KdPrintCircularBuffer,
                                              String->Buffer + First,
                                              Length - First);
            }
            if (LengthCopied > First) {
                KdPrintWritePointer = KdPrintCircularBuffer + LengthCopied - First;
                KdPrintRolloverCount++;
            } else {
                KdPrintWritePointer += LengthCopied;
                if (KdPrintWritePointer >= KdPrintCircularBuffer+KDPRINTBUFFERSIZE) {
                    KdPrintWritePointer = KdPrintCircularBuffer;
                    KdPrintRolloverCount++;
                }
            }
        }
    }

    KiReleaseSpinLock(&KdpPrintSpinLock);
    KeLowerIrql(OldIrql);
}
