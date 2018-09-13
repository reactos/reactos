/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    stubs.c

Abstract:

    This module implements bug check and system shutdown code.

Author:

    Mark Lucovsky (markl) 30-Aug-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include <inbv.h>

//
//
//

extern KDDEBUGGER_DATA64 KdDebuggerDataBlock;

extern PVOID ExPoolCodeStart;
extern PVOID ExPoolCodeEnd;
extern PVOID MmPoolCodeStart;
extern PVOID MmPoolCodeEnd;
extern PVOID MmPteCodeStart;
extern PVOID MmPteCodeEnd;

#if defined (i386)
#define PROGRAM_COUNTER(_trapframe)   ((_trapframe)->Eip)
#elif defined (ALPHA)
#define PROGRAM_COUNTER(_trapframe)   ((_trapframe)->Fir)
#elif defined (_IA64_)
#define PROGRAM_COUNTER(_trapframe)   ((_trapframe)->StIIP)
#else
#error ("unknown processor type")
#endif

//
// Define forward referenced prototypes.
//

VOID
KiScanBugCheckCallbackList (
    VOID
    );

//
// Define bug count recursion counter and a context buffer.
//

ULONG KeBugCheckCount = 1;


VOID
KeBugCheck (
    IN ULONG BugCheckCode
    )

/*++

Routine Description:

    This function crashes the system in a controlled manner.

Arguments:

    BugCheckCode - Supplies the reason for the bug check.

Return Value:

    None.

--*/
{
    KeBugCheckEx(BugCheckCode,0,0,0,0);
}

ULONG_PTR KiBugCheckData[5];
PUNICODE_STRING KiBugCheckDriver;

BOOLEAN
KeGetBugMessageText(
    IN ULONG MessageId,
    IN PANSI_STRING ReturnedString OPTIONAL
    )
{
    ULONG   i;
    PUCHAR  s;
    PMESSAGE_RESOURCE_BLOCK MessageBlock;
    PUCHAR Buffer;
    BOOLEAN Result;

    Result = FALSE;
    try {
        if (KiBugCodeMessages != NULL) {
            MmMakeKernelResourceSectionWritable ();
            MessageBlock = &KiBugCodeMessages->Blocks[0];
            for (i = KiBugCodeMessages->NumberOfBlocks; i; i -= 1) {
                if (MessageId >= MessageBlock->LowId &&
                    MessageId <= MessageBlock->HighId) {

                    s = (PCHAR)KiBugCodeMessages + MessageBlock->OffsetToEntries;
                    for (i = MessageId - MessageBlock->LowId; i; i -= 1) {
                        s += ((PMESSAGE_RESOURCE_ENTRY)s)->Length;
                    }

                    Buffer = ((PMESSAGE_RESOURCE_ENTRY)s)->Text;

                    i = strlen(Buffer) - 1;
                    while (i > 0 && (Buffer[i] == '\n'  ||
                                     Buffer[i] == '\r'  ||
                                     Buffer[i] == 0
                                    )
                          ) {
                        if (!ARGUMENT_PRESENT( ReturnedString )) {
                            Buffer[i] = 0;
                        }
                        i -= 1;
                    }

                    if (!ARGUMENT_PRESENT( ReturnedString )) {
                        InbvDisplayString(Buffer);
                        }
                    else {
                        ReturnedString->Buffer = Buffer;
                        ReturnedString->Length = (USHORT)(i+1);
                        ReturnedString->MaximumLength = (USHORT)(i+1);
                    }
                    Result = TRUE;
                    break;
                }
                MessageBlock += 1;
            }
        }
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        ;
    }

    return Result;
}



PCHAR
KeBugCheckUnicodeToAnsi(
    IN PUNICODE_STRING UnicodeString,
    OUT PCHAR AnsiBuffer,
    IN ULONG MaxAnsiLength
    )
{
    PCHAR Dst;
    PWSTR Src;
    ULONG Length;

    Length = UnicodeString->Length / sizeof( WCHAR );
    if (Length >= MaxAnsiLength) {
        Length = MaxAnsiLength - 1;
        }
    Src = UnicodeString->Buffer;
    Dst = AnsiBuffer;
    while (Length--) {
        *Dst++ = (UCHAR)*Src++;
        }
    *Dst = '\0';
    return AnsiBuffer;
}

VOID
KiBugCheckDebugBreak (
    IN ULONG    BreakStatus
    )
{
    do {
        try {

            //
            // Issue a breakpoint
            //

            DbgBreakPointWithStatus (BreakStatus);

        } except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // Failure to issue breakpoint, halt the system
            //

            try {

                HalHaltSystem();

            } except(EXCEPTION_EXECUTE_HANDLER) {

                for (;;) {
                }
            }

            for (;;) {
            }
        }
    } while (BreakStatus != DBG_STATUS_BUGCHECK_FIRST);
}

PVOID
KiPcToFileHeader(
    IN PVOID PcValue,
    OUT PLDR_DATA_TABLE_ENTRY *DataTableEntry,
    IN LOGICAL DriversOnly,
    OUT PBOOLEAN InKernelOrHal
    )

/*++

Routine Description:

    This function returns the base of an image that contains the
    specified PcValue. An image contains the PcValue if the PcValue
    is within the ImageBase, and the ImageBase plus the size of the
    virtual image.

Arguments:

    PcValue - Supplies a PcValue.

    DataTableEntry - Supplies a pointer to a variable that receives the
        address of the data table entry that describes the image.

    DriversOnly - Supplies TRUE if the kernel and HAL should be skipped.

    InKernelOrHal - Set to TRUE if the PcValue is in the kernel or the HAL.
        This only has meaning if DriversOnly is FALSE.

Return Value:

    NULL - No image was found that contains the PcValue.

    NON-NULL - Returns the base address of the image that contains the
        PcValue.

--*/

{
    ULONG i;
    PLIST_ENTRY ModuleListHead;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Next;
    ULONG_PTR Bounds;
    PVOID ReturnBase, Base;

    //
    // If the module list has been initialized, then scan the list to
    // locate the appropriate entry.
    //

    if (KeLoaderBlock != NULL) {
        ModuleListHead = &KeLoaderBlock->LoadOrderListHead;

    } else {
        ModuleListHead = &PsLoadedModuleList;
    }

    *InKernelOrHal = FALSE;

    ReturnBase = NULL;
    Next = ModuleListHead->Flink;
    if (Next != NULL) {
        i = 0;
        while (Next != ModuleListHead) {
            if (MmDbgReadCheck(Next) == NULL) {
                return NULL;
            }
            i += 1;
            if ((i <= 2) && (DriversOnly == TRUE)) {
                Next = Next->Flink;
                continue;
            }

            Entry = CONTAINING_RECORD(Next,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);

            Next = Next->Flink;
            Base = Entry->DllBase;
            Bounds = (ULONG_PTR)Base + Entry->SizeOfImage;
            if ((ULONG_PTR)PcValue >= (ULONG_PTR)Base && (ULONG_PTR)PcValue < Bounds) {
                *DataTableEntry = Entry;
                ReturnBase = Base;
                if (i <= 2) {
                    *InKernelOrHal = TRUE;
                }
                break;
            }
        }
    }

    return ReturnBase;
}



VOID
KiDumpParameterImages(
    IN PCHAR Buffer,
    IN PULONG_PTR BugCheckParameters,
    IN ULONG NumberOfParameters,
    IN PKE_BUGCHECK_UNICODE_TO_ANSI UnicodeToAnsiRoutine
    )

/*++

Routine Description:

    This function formats and displays the image names of boogcheck parameters
    that happen to match an address in an image.

Arguments:

    Buffer - Supplies a pointer to a buffer to be used to output machine
        state information.

    BugCheckParameters - Supplies additional bugcheck information.

    NumberOfParameters - sizeof BugCheckParameters array.

    UnicodeToAnsiRoutine - Supplies a pointer to a routine to convert Unicode
        strings to Ansi strings without touching paged translation tables.

Return Value:

    None.

--*/

{
    PUNICODE_STRING BugCheckDriver;
    PLIST_ENTRY ModuleListHead;
    PLIST_ENTRY Next;
    ULONG i;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PVOID ImageBase;
    UCHAR AnsiBuffer[ 32 ];
    ULONG DateStamp;
    PIMAGE_NT_HEADERS NtHeaders;
    BOOLEAN FirstPrint = TRUE;
    BOOLEAN InKernelOrHal;

    //
    // At this point the context record contains the machine state at the
    // call to bug check.
    //
    // Put out the system version and the title line with the PSR and FSR.
    //

    //
    // Check to see if any BugCheckParameters are valid code addresses.
    // If so, print them for the user.
    //

    for (i = 0; i < NumberOfParameters; i += 1) {
        ImageBase = KiPcToFileHeader((PVOID) BugCheckParameters[i],
                                     &DataTableEntry,
                                     FALSE,
                                     &InKernelOrHal);
        if (ImageBase == NULL) {
            BugCheckDriver = MmLocateUnloadedDriver ((PVOID)BugCheckParameters[i]);
            if (BugCheckDriver == NULL) {
                continue;
            }
            ImageBase = (PVOID)BugCheckParameters[i];
            DateStamp = 0;
            (*UnicodeToAnsiRoutine) (BugCheckDriver,
                                     AnsiBuffer,
                                     sizeof (AnsiBuffer));
        }
        else {
            if (MmDbgReadCheck(DataTableEntry->DllBase) != NULL) {

                NtHeaders = RtlImageNtHeader(DataTableEntry->DllBase);
                DateStamp = NtHeaders->FileHeader.TimeDateStamp;

            } else {
                DateStamp = 0;
            }
            (*UnicodeToAnsiRoutine)( &DataTableEntry->BaseDllName,
                                     AnsiBuffer,
                                     sizeof( AnsiBuffer ));
        }

        sprintf (Buffer, "%s** Address %p base at %p, DateStamp %08lx - %-12.12s\n",
            FirstPrint ? "\n*":"*",
                BugCheckParameters[i],
                ImageBase,
                DateStamp,
                AnsiBuffer);

        InbvDisplayString(Buffer);
        FirstPrint = FALSE;
    }

    return;
}

//
// MessageId: POWER_FAILURE_SIMULATE
//
// MessageText:
//
//  POWER_FAILURE_SIMULATE
//
#define POWER_FAILURE_SIMULATE           ((ULONG)0x000000E5L)

VOID
KeBugCheckEx (
    IN ULONG BugCheckCode,
    IN ULONG_PTR BugCheckParameter1,
    IN ULONG_PTR BugCheckParameter2,
    IN ULONG_PTR BugCheckParameter3,
    IN ULONG_PTR BugCheckParameter4
    )

/*++

Routine Description:

    This function crashes the system in a controlled manner.

Arguments:

    BugCheckCode - Supplies the reason for the bug check.

    BugCheckParameter1-4 - Supplies additional bug check information

Return Value:

    None.

--*/

{
    UCHAR Buffer[100];
    ULONG_PTR BugCheckParameters[4];
    CONTEXT ContextSave;
    ULONG PssMessage;
    PCHAR HardErrorCaption;
    PCHAR HardErrorMessage;
    KIRQL OldIrql;
    PKTRAP_FRAME TrapInformation;
    PVOID ExecutionAddress;
    PVOID ImageBase;
    PVOID VirtualAddress;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    UCHAR AnsiBuffer[32];
    PKTHREAD Thread;
    BOOLEAN InKernelOrHal;
    BOOLEAN DontCare;

#if !defined(NT_UP)

    ULONG TargetSet;

#endif
    BOOLEAN hardErrorCalled;

    //
    // Try to simulate a power failure for Cluster testing
    //

    if (BugCheckCode == POWER_FAILURE_SIMULATE) {
        KiScanBugCheckCallbackList();
        HalReturnToFirmware(HalRebootRoutine);
    }

    //
    // Capture the callers context as closely as possible into the debugger's
    // processor state area of the Prcb.
    //
    // N.B. There may be some prologue code that shuffles registers such that
    //      they get destroyed.
    //

#if defined(i386)
    KiSetHardwareTrigger();
#else
    KiHardwareTrigger = 1;
#endif

    RtlCaptureContext(&KeGetCurrentPrcb()->ProcessorState.ContextFrame);
    KiSaveProcessorControlState(&KeGetCurrentPrcb()->ProcessorState);

    //
    // This is necessary on machines where the virtual unwind that happens
    // during KeDumpMachineState() destroys the context record.
    //

    ContextSave = KeGetCurrentPrcb()->ProcessorState.ContextFrame;

    //
    // If we are called by hard error then we don't want to dump the
    // processor state on the machine.
    //
    // We know that we are called by hard error because the bug check
    // code will be FATAL_UNHANDLED_HARD_ERROR.  If this is so then the
    // error status passed to harderr is the first parameter, and a pointer
    // to the parameter array from hard error is passed as the second
    // argument.
    //
    // The third argument is the OemCaption to be printed.
    // The last argument is the OemMessage to be printed.
    //

    if (BugCheckCode == FATAL_UNHANDLED_HARD_ERROR) {

        PULONG_PTR parameterArray;

        hardErrorCalled = TRUE;

        HardErrorCaption = (PCHAR)BugCheckParameter3;
        HardErrorMessage = (PCHAR)BugCheckParameter4;
        parameterArray = (PULONG_PTR)BugCheckParameter2;
        BugCheckCode = (ULONG)BugCheckParameter1;
        BugCheckParameter1 = parameterArray[0];
        BugCheckParameter2 = parameterArray[1];
        BugCheckParameter3 = parameterArray[2];
        BugCheckParameter4 = parameterArray[3];

    } else {

        hardErrorCalled = FALSE;

        switch (BugCheckCode) {

            case IRQL_NOT_LESS_OR_EQUAL:

                ExecutionAddress = (PVOID)BugCheckParameter4;

                if (ExecutionAddress >= ExPoolCodeStart && ExecutionAddress < ExPoolCodeEnd) {
                    BugCheckCode = DRIVER_CORRUPTED_EXPOOL;
                }
                else if (ExecutionAddress >= MmPoolCodeStart && ExecutionAddress < MmPoolCodeEnd) {
                    BugCheckCode = DRIVER_CORRUPTED_MMPOOL;
                }
                else if (ExecutionAddress >= MmPteCodeStart && ExecutionAddress < MmPteCodeEnd) {
                    BugCheckCode = DRIVER_CORRUPTED_SYSPTES;
                }
                else {
                    ImageBase = KiPcToFileHeader (ExecutionAddress,
                                                  &DataTableEntry,
                                                  FALSE,
                                                  &InKernelOrHal);
                    if (InKernelOrHal == TRUE) {

                        //
                        // The kernel faulted at raised IRQL.  Quite often this
                        // is a driver that has unloaded without deleting its
                        // lookaside lists or other resources.  Or its resources
                        // are marked pagable and shouldn't be.  Detect both
                        // cases here and identify the offending driver
                        // whenever possible.
                        //

                        VirtualAddress = (PVOID)BugCheckParameter1;

                        ImageBase = KiPcToFileHeader (VirtualAddress,
                                                      &DataTableEntry,
                                                      TRUE,
                                                      &InKernelOrHal);

                        if (ImageBase != NULL) {
                            KiBugCheckDriver = &DataTableEntry->BaseDllName;
                            BugCheckCode = DRIVER_PORTION_MUST_BE_NONPAGED;
                        }
                        else {
                            KiBugCheckDriver = MmLocateUnloadedDriver (VirtualAddress);
                            if (KiBugCheckDriver != NULL) {
                                BugCheckCode = SYSTEM_SCAN_AT_RAISED_IRQL_CAUGHT_IMPROPER_DRIVER_UNLOAD;
                            }
                        }
                    }
                    else {
                        BugCheckCode = DRIVER_IRQL_NOT_LESS_OR_EQUAL;
                    }
                }
                break;

            case ATTEMPTED_WRITE_TO_READONLY_MEMORY:

                KiBugCheckDriver = NULL;
                TrapInformation = (PKTRAP_FRAME)BugCheckParameter3;

                //
                // Extract the execution address from the trap frame to
                // identify the component.
                //

                if (TrapInformation != NULL) {
                    ExecutionAddress = (PVOID)PROGRAM_COUNTER (TrapInformation);
                    ImageBase = KiPcToFileHeader (ExecutionAddress,
                                                  &DataTableEntry,
                                                  TRUE,
                                                  &InKernelOrHal);

                    if (ImageBase != NULL) {
                        KiBugCheckDriver = &DataTableEntry->BaseDllName;
                    }
                }

                break;

            case DRIVER_LEFT_LOCKED_PAGES_IN_PROCESS:

                ExecutionAddress = (PVOID)BugCheckParameter1;

                ImageBase = KiPcToFileHeader (ExecutionAddress,
                                              &DataTableEntry,
                                              TRUE,
                                              &InKernelOrHal);

                if (ImageBase != NULL) {
                    KiBugCheckDriver = &DataTableEntry->BaseDllName;
                }
                else {
                    KiBugCheckDriver = MmLocateUnloadedDriver (ExecutionAddress);
                }

                BugCheckParameter4 = (ULONG_PTR)KiBugCheckDriver;

                break;

            case DRIVER_USED_EXCESSIVE_PTES:

                DataTableEntry = (PLDR_DATA_TABLE_ENTRY)BugCheckParameter1;
                KiBugCheckDriver = &DataTableEntry->BaseDllName;

                break;

            case PAGE_FAULT_IN_NONPAGED_AREA:

                ExecutionAddress = NULL;
                KiBugCheckDriver = NULL;

                VirtualAddress = (PVOID)BugCheckParameter1;

                TrapInformation = (PKTRAP_FRAME)BugCheckParameter3;

                //
                // Extract the execution address from the trap frame to
                // identify the component.
                //

                if (TrapInformation != NULL) {

                    ExecutionAddress = (PVOID)PROGRAM_COUNTER (TrapInformation);
                    BugCheckParameter3 = (ULONG_PTR)ExecutionAddress;

                    KiPcToFileHeader (ExecutionAddress,
                                      &DataTableEntry,
                                      FALSE,
                                      &InKernelOrHal);

                    ImageBase = KiPcToFileHeader (ExecutionAddress,
                                                  &DataTableEntry,
                                                  TRUE,
                                                  &DontCare);

                    if (ImageBase != NULL) {
                        KiBugCheckDriver = &DataTableEntry->BaseDllName;
                    }
                }
                else {

                    //
                    // No trap frame, so no execution address either.
                    //

                    BugCheckParameter3 = 0;
                }

                Thread = KeGetCurrentThread();

                if ((VirtualAddress >= MmSpecialPoolStart) &&
                    (VirtualAddress < MmSpecialPoolEnd)) {

                    //
                    // Update the bugcheck number so the administrator gets
                    // useful feedback that enabling special pool has enabled
                    // the system to locate the corruptor.
                    //

                    if (MmIsSpecialPoolAddressFree (VirtualAddress) == TRUE) {
                        if (InKernelOrHal == TRUE) {
                            BugCheckCode = PAGE_FAULT_IN_FREED_SPECIAL_POOL;
                        }
                        else {
                            BugCheckCode = DRIVER_PAGE_FAULT_IN_FREED_SPECIAL_POOL;
                        }
                    }
                    else {
                        if (InKernelOrHal == TRUE) {
                            BugCheckCode = PAGE_FAULT_BEYOND_END_OF_ALLOCATION;
                        }
                        else {
                            BugCheckCode = DRIVER_PAGE_FAULT_BEYOND_END_OF_ALLOCATION;
                        }
                    }
                }
                else if ((ExecutionAddress == VirtualAddress) &&
                        (MmIsHydraAddress (VirtualAddress) == TRUE) &&
                        ((Thread->Teb == NULL) || (IS_SYSTEM_ADDRESS(Thread->Teb)))) {
                    //
                    // This is a driver reference to session space from a
                    // worker thread.  Since the system process has no session
                    // space this is illegal and the driver must be fixed.
                    //

                    BugCheckCode = TERMINAL_SERVER_DRIVER_MADE_INCORRECT_MEMORY_REFERENCE;
                }
                else {
                    KiBugCheckDriver = MmLocateUnloadedDriver (VirtualAddress);
                    if (KiBugCheckDriver != NULL) {
                        BugCheckCode = DRIVER_UNLOADED_WITHOUT_CANCELLING_PENDING_OPERATIONS;
                    }
                }

                break;

            default:
                break;
        }
    }

    if (KiBugCheckDriver != NULL) {
        KeBugCheckUnicodeToAnsi (KiBugCheckDriver,
                                 AnsiBuffer,
                                 sizeof (AnsiBuffer));
    }

    KiBugCheckData[0] = BugCheckCode;
    KiBugCheckData[1] = BugCheckParameter1;
    KiBugCheckData[2] = BugCheckParameter2;
    KiBugCheckData[3] = BugCheckParameter3;
    KiBugCheckData[4] = BugCheckParameter4;

    BugCheckParameters[0] = BugCheckParameter1;
    BugCheckParameters[1] = BugCheckParameter2;
    BugCheckParameters[2] = BugCheckParameter3;
    BugCheckParameters[3] = BugCheckParameter4;

    if (KdPitchDebugger == FALSE ) {
        KdDebuggerDataBlock.SavedContext = (ULONG_PTR) &ContextSave;
    }

    //
    // If the user manually crashed the machine, skips the DbgPrints and
    // go to the crashdump.
    // Trying to do DbgPrint causes us to reeeter the debugger which causes
    // some problems.
    //
    // Otherwise, if the debugger is enabled, print out the information and
    // stop.
    //

    if ((BugCheckCode != MANUALLY_INITIATED_CRASH) &&
        (KdDebuggerEnabled)) {

        DbgPrint("\n*** Fatal System Error: 0x%08lx\n"
                 "                       (0x%p,0x%p,0x%p,0x%p)\n\n",
                 BugCheckCode,
                 BugCheckParameter1,
                 BugCheckParameter2,
                 BugCheckParameter3,
                 BugCheckParameter4);

        //
        // If the debugger is not actually connected, or the user manually
        // crashed the machine by typing .crash in the debugger, proceed to
        // "blue screen" the system.
        //
        // The call to DbgPrint above will have set the state of
        // KdDebuggerNotPresent if the debugger has become disconnected
        // since the system was booted.
        //

        if (KdDebuggerNotPresent == FALSE) {

            if (KiBugCheckDriver != NULL) {
                DbgPrint("The %s driver may be at fault.\n", AnsiBuffer);
            }

            if (hardErrorCalled != FALSE) {
                if (HardErrorCaption) {
                    DbgPrint(HardErrorCaption);
                }
                if (HardErrorMessage) {
                    DbgPrint(HardErrorMessage);
                }
            }

            KiBugCheckDebugBreak (DBG_STATUS_BUGCHECK_FIRST);
        }
    }

    //
    // Freeze execution of the system by disabling interrupts and looping.
    //

    KiDisableInterrupts();
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    //
    // Don't attempt to display message more than once.
    //

    if (InterlockedDecrement (&KeBugCheckCount) == 0) {

#if !defined(NT_UP)

        //
        // Attempt to get the other processors frozen now, but don't wait
        // for them to freeze (in case someone is stuck).
        //

        TargetSet = KeActiveProcessors & ~KeGetCurrentPrcb()->SetMember;
        if (TargetSet != 0) {
            KiIpiSend((KAFFINITY) TargetSet, IPI_FREEZE);

            //
            // Give the other processors one second to flush their data caches.
            //
            // N.B. This cannot be synchronized since the reason for the bug
            //      may be one of the other processors failed.
            //

            KeStallExecutionProcessor(1000 * 1000);
        }

#endif

        //
        // Enable InbvDisplayString calls to make it through to bootvid driver.
        //

        if (InbvIsBootDriverInstalled()) {

            InbvAcquireDisplayOwnership();

            InbvResetDisplay();
            InbvSolidColorFill(0,0,639,479,4); // make the screen blue
            InbvSetTextColor(15);
            InbvInstallDisplayStringFilter((INBV_DISPLAY_STRING_FILTER)NULL);
            InbvEnableDisplayString(TRUE);     // enable display string
            InbvSetScrollRegion(0,0,639,479);  // set to use entire screen
        }

        if (!hardErrorCalled) {
            sprintf((char *)Buffer,
                    "\n*** STOP: 0x%08lX (0x%p,0x%p,0x%p,0x%p)\n",
                    BugCheckCode,
                    BugCheckParameter1,
                    BugCheckParameter2,
                    BugCheckParameter3,
                    BugCheckParameter4
                    );
            InbvDisplayString((char *)Buffer);

            KeGetBugMessageText(BugCheckCode, NULL);
            InbvDisplayString("\n");

            if (KiBugCheckDriver != NULL) {

                //
                // Output the driver name.
                //

                KeGetBugMessageText(BUGCODE_ID_DRIVER, NULL);
                InbvDisplayString(AnsiBuffer);
                InbvDisplayString("\n");
            }

        } else {
            if (HardErrorCaption) {
                InbvDisplayString(HardErrorCaption);
            }
            if (HardErrorMessage) {
                InbvDisplayString(HardErrorMessage);
            }
        }

        //
        // Process the bug check callback list.
        //

        KiScanBugCheckCallbackList();

        //
        // If the debugger is not enabled, then dump the machine state and
        // attempt to enable the debugger.
        //

        if (!hardErrorCalled) {

            KiDumpParameterImages(
                (char *)Buffer,
                BugCheckParameters,
                4,
                KeBugCheckUnicodeToAnsi);

        }

        if (KdDebuggerEnabled == FALSE && KdPitchDebugger == FALSE ) {
            KdInitSystem(NULL, FALSE);

        } else {
            InbvDisplayString("\n");
        }

        //
        // Write a crash dump and optionally reboot if the system has been
        // so configured.
        //

        KeGetCurrentPrcb()->ProcessorState.ContextFrame = ContextSave;

        if (!IoWriteCrashDump(BugCheckCode,
                              BugCheckParameter1,
                              BugCheckParameter2,
                              BugCheckParameter3,
                              BugCheckParameter4,
                              &ContextSave
                              )) {
            //
            // If no crashdump take, display the PSS message.
            //

            switch ( BugCheckCode ) {

                case IRQL_NOT_LESS_OR_EQUAL:
                case DRIVER_IRQL_NOT_LESS_OR_EQUAL:
                case DRIVER_CORRUPTED_EXPOOL:
                case DRIVER_CORRUPTED_MMPOOL:
                case DRIVER_PORTION_MUST_BE_NONPAGED:
                case SYSTEM_SCAN_AT_RAISED_IRQL_CAUGHT_IMPROPER_DRIVER_UNLOAD:
                    PssMessage = BUGCODE_PSS_MESSAGE_A;
                    break;

                case KMODE_EXCEPTION_NOT_HANDLED:
                    PssMessage = BUGCODE_PSS_MESSAGE_1E;
                    break;

                case FAT_FILE_SYSTEM:
                case NTFS_FILE_SYSTEM:
                    PssMessage = BUGCODE_PSS_MESSAGE_23;
                    break;

                case DATA_BUS_ERROR:
                    PssMessage = BUGCODE_PSS_MESSAGE_2E;
                    break;

                case NO_MORE_SYSTEM_PTES:
                    PssMessage = BUGCODE_PSS_MESSAGE_3F;
                    break;

                case INACCESSIBLE_BOOT_DEVICE:
                    PssMessage = BUGCODE_PSS_MESSAGE_7B;
                    break;

                case UNEXPECTED_KERNEL_MODE_TRAP:
                    PssMessage = BUGCODE_PSS_MESSAGE_7F;
                    break;

                case STATUS_SYSTEM_IMAGE_BAD_SIGNATURE:
                    PssMessage = BUGCODE_PSS_MESSAGE_SIGNATURE;
                    break;

                case ACPI_BIOS_ERROR:
                    PssMessage = BUGCODE_PSS_MESSAGE_A5;
                    break;

                case ACPI_BIOS_FATAL_ERROR:
                    PssMessage = ACPI_BIOS_FATAL_ERROR;
                    break;

                default:
                    PssMessage = BUGCODE_PSS_MESSAGE;
                    break;
                }

            KeGetBugMessageText(PssMessage, NULL);
        }
    }

    //
    // Attempt to enter the kernel debugger.
    //

    KiBugCheckDebugBreak (DBG_STATUS_BUGCHECK_SECOND);
    return;
}

VOID
KeEnterKernelDebugger (
    VOID
    )

/*++

Routine Description:

    This function crashes the system in a controlled manner attempting
    to invoke the kernel debugger.

Arguments:

    None.

Return Value:

    None.

--*/

{

#if !defined(i386)
    KIRQL OldIrql;
#endif

    //
    // Freeze execution of the system by disabling interrupts and looping.
    //

    KiHardwareTrigger = 1;
    KiDisableInterrupts();
#if !defined(i386)
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
#endif
    if (InterlockedDecrement (&KeBugCheckCount) == 0) {
        if (KdDebuggerEnabled == FALSE) {
            if ( KdPitchDebugger == FALSE ) {
                KdInitSystem(NULL, FALSE);
            }
        }
    }

    KiBugCheckDebugBreak (DBG_STATUS_FATAL);
}

NTKERNELAPI
BOOLEAN
KeDeregisterBugCheckCallback (
    IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord
    )

/*++

Routine Description:

    This function deregisters a bug check callback record.

Arguments:

    CallbackRecord - Supplies a pointer to a bug check callback record.

Return Value:

    If the specified bug check callback record is successfully deregistered,
    then a value of TRUE is returned. Otherwise, a value of FALSE is returned.

--*/

{

    BOOLEAN Deregister;
    KIRQL OldIrql;

    //
    // Raise IRQL to HIGH_LEVEL and acquire the bug check callback list
    // spinlock.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    KiAcquireSpinLock(&KeBugCheckCallbackLock);

    //
    // If the specified callback record is currently registered, then
    // deregister the callback record.
    //

    Deregister = FALSE;
    if (CallbackRecord->State == BufferInserted) {
        CallbackRecord->State = BufferEmpty;
        RemoveEntryList(&CallbackRecord->Entry);
        Deregister = TRUE;
    }

    //
    // Release the bug check callback spinlock, lower IRQL to its previous
    // value, and return whether the callback record was successfully
    // deregistered.
    //

    KiReleaseSpinLock(&KeBugCheckCallbackLock);
    KeLowerIrql(OldIrql);
    return Deregister;
}

NTKERNELAPI
BOOLEAN
KeRegisterBugCheckCallback (
    IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
    IN PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PUCHAR Component
    )

/*++

Routine Description:

    This function registers a bug check callback record. If the system
    crashes, then the specified function will be called during bug check
    processing so it may dump additional state in the specified bug check
    buffer.

    N.B. Bug check callback routines are called in reverse order of
         registration, i.e., in LIFO order.

Arguments:

    CallbackRecord - Supplies a pointer to a callback record.

    CallbackRoutine - Supplies a pointer to the callback routine.

    Buffer - Supplies a pointer to the bug check buffer.

    Length - Supplies the length of the bug check buffer in bytes.

    Component - Supplies a pointer to a zero terminated component
        identifier.

Return Value:

    If the specified bug check callback record is successfully registered,
    then a value of TRUE is returned. Otherwise, a value of FALSE is returned.

--*/

{

    BOOLEAN Inserted;
    KIRQL OldIrql;

    //
    // Raise IRQL to HIGH_LEVEL and acquire the bug check callback list
    // spinlock.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    KiAcquireSpinLock(&KeBugCheckCallbackLock);

    //
    // If the specified callback record is currently not registered, then
    // register the callback record.
    //

    Inserted = FALSE;
    if (CallbackRecord->State == BufferEmpty) {
        CallbackRecord->CallbackRoutine = CallbackRoutine;
        CallbackRecord->Buffer = Buffer;
        CallbackRecord->Length = Length;
        CallbackRecord->Component = Component;
        CallbackRecord->Checksum =
            ((ULONG_PTR)CallbackRoutine + (ULONG_PTR)Buffer + Length + (ULONG_PTR)Component);

        CallbackRecord->State = BufferInserted;
        InsertHeadList(&KeBugCheckCallbackListHead, &CallbackRecord->Entry);
        Inserted = TRUE;
    }

    //
    // Release the bug check callback spinlock, lower IRQL to its previous
    // value, and return whether the callback record was successfully
    // registered.
    //

    KiReleaseSpinLock(&KeBugCheckCallbackLock);
    KeLowerIrql(OldIrql);
    return Inserted;
}

VOID
KiScanBugCheckCallbackList (
    VOID
    )

/*++

Routine Description:

    This function scans the bug check callback list and calls each bug
    check callback routine so it can dump component specific information
    that may identify the cause of the bug check.

    N.B. The scan of the bug check callback list is performed VERY
        carefully. Bug check callback routines are called at HIGH_LEVEL
        and may not acquire ANY resources.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PKBUGCHECK_CALLBACK_RECORD CallbackRecord;
    ULONG_PTR Checksum;
    ULONG Index;
    PLIST_ENTRY LastEntry;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    PUCHAR Source;

    //
    // If the bug check callback listhead is not initialized, then the
    // bug check has occured before the system has gotten far enough
    // in the initialization code to enable anyone to register a callback.
    //

    ListHead = &KeBugCheckCallbackListHead;
    if ((ListHead->Flink != NULL) && (ListHead->Blink != NULL)) {

        //
        // Scan the bug check callback list.
        //

        LastEntry = ListHead;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead) {

            //
            // The next entry address must be aligned properly, the
            // callback record must be readable, and the callback record
            // must have back link to the last entry.
            //

            if (((ULONG_PTR)NextEntry & (sizeof(ULONG_PTR) - 1)) != 0) {
                return;

            } else {
                CallbackRecord = CONTAINING_RECORD(NextEntry,
                                                   KBUGCHECK_CALLBACK_RECORD,
                                                   Entry);

                Source = (PUCHAR)CallbackRecord;
                for (Index = 0; Index < sizeof(KBUGCHECK_CALLBACK_RECORD); Index += 1) {
                    if (MmDbgReadCheck((PVOID)Source) == NULL) {
                        return;
                    }

                    Source += 1;
                }

                if (CallbackRecord->Entry.Blink != LastEntry) {
                    return;
                }

                //
                // If the callback record has a state of inserted and the
                // computed checksum matches the callback record checksum,
                // then call the specified bug check callback routine.
                //

                Checksum = (ULONG_PTR)CallbackRecord->CallbackRoutine;
                Checksum += (ULONG_PTR)CallbackRecord->Buffer;
                Checksum += CallbackRecord->Length;
                Checksum += (ULONG_PTR)CallbackRecord->Component;
                if ((CallbackRecord->State == BufferInserted) &&
                    (CallbackRecord->Checksum == Checksum)) {

                    //
                    // Call the specified bug check callback routine and
                    // handle any exceptions that occur.
                    //

                    CallbackRecord->State = BufferStarted;
                    try {
                        (CallbackRecord->CallbackRoutine)(CallbackRecord->Buffer,
                                                          CallbackRecord->Length);

                        CallbackRecord->State = BufferFinished;

                    } except(EXCEPTION_EXECUTE_HANDLER) {
                        CallbackRecord->State = BufferIncomplete;
                    }
                }
            }

            LastEntry = NextEntry;
            NextEntry = NextEntry->Flink;
        }
    }

    return;
}
