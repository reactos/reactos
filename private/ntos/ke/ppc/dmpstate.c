/*++

Copyright (c) 1993  IBM and Microsoft Corporation

Module Name:

    dmpstate.c

Abstract:

    This module implements  the architecture specific routine that dumps
    the machine state when a bug check occurs and no debugger is hooked
    to the system. It is assumed that it is called from bug check.

Author:

    Chuck Bauman 19-Sep-1993

Environment:

    Kernel mode.

Revision History:

    Based on Dave Cutler's MIPS implemenation

    Tom Wood (twood) 19-Aug-1994
    Update to use RtlVirtualUnwind even when there isn't a function table
    entry.  Add stack limit parameters to RtlVirtualUnwind.

    Changed KiLookupFunctionEntry to deal with the indirect entries.

--*/

#include "ki.h"

//
// Define forward referenced prototypes.
//

VOID
KiDisplayString (
    IN ULONG Column,
    IN ULONG Row,
    IN PCHAR Buffer
    );

PRUNTIME_FUNCTION
KiLookupFunctionEntry (
    IN ULONG ControlPc
    );

PVOID
KiPcToFileHeader(
    IN PVOID PcValue,
    OUT PVOID *BaseOfImage,
    OUT PLDR_DATA_TABLE_ENTRY *DataTableEntry
    );

//
// Define external data.
//

extern LIST_ENTRY PsLoadedModuleList;

VOID
KeDumpMachineState (
    IN PKPROCESSOR_STATE ProcessorState,
    IN PCHAR Buffer,
    IN PULONG BugCheckParameters,
    IN ULONG NumberOfParameters,
    IN PKE_BUGCHECK_UNICODE_TO_ANSI UnicodeToAnsiRoutine
    )

/*++

Routine Description:

    This function formats and displays the machine state at the time of the
    to bug check.

Arguments:

    ProcessorState - Supplies a pointer to a processor state record.

    Buffer - Supplies a pointer to a buffer to be used to output machine
        state information.

    BugCheckParameters - Supplies a pointer to an array of additional
        bug check information.

    NumberOfParameters - Suppiles the size of the bug check parameters
        array.

    UnicodeToAnsiRoutine - Supplies a pointer to a routine to convert Unicode strings
        to Ansi strings without touching paged translation tables.

Return Value:

    None.

--*/

{

    PCONTEXT ContextRecord;
    ULONG ControlPc;
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    ULONG DisplayColumn;
    ULONG DisplayHeight;
    ULONG DisplayRow;
    ULONG DisplayWidth;
    UNICODE_STRING DllName;
    ULONG EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    PVOID ImageBase;
    ULONG Index;
    BOOLEAN InFunction;
    ULONG LastStack;
    PLIST_ENTRY ModuleListHead;
    PLIST_ENTRY NextEntry;
    ULONG NextPc;
    ULONG StackLimit;
    UCHAR AnsiBuffer[ 32 ];
    ULONG DateStamp;

    //
    // Call the HAL to force all external interrupts to be disabled
    // at the interrupt controller. PowerPC optimization does not
    // do this when raising to high level.
    //
    for (Index = 0; Index < MAXIMUM_VECTOR; Index++) {
       HalDisableSystemInterrupt(Index, HIGH_LEVEL);
    }

    //
    // Query display parameters.
    //

    HalQueryDisplayParameters(&DisplayWidth,
                              &DisplayHeight,
                              &DisplayColumn,
                              &DisplayRow);

    //
    // Display any addresses that fall within the range of any module in
    // the loaded module list.
    //

    for (Index = 0; Index < NumberOfParameters; Index += 1) {
        ImageBase = KiPcToFileHeader((PVOID)*BugCheckParameters,
                                     &ImageBase,
                                     &DataTableEntry);

        if (ImageBase != NULL) {
            sprintf(Buffer,
                    "*** %08lX has base at %08lX - %s\n",
                    *BugCheckParameters,
                    ImageBase,
                    (*UnicodeToAnsiRoutine)( &DataTableEntry->BaseDllName, AnsiBuffer, sizeof( AnsiBuffer )));

            HalDisplayString(Buffer);
        }

        BugCheckParameters += 1;
    }

    //
    // Virtually unwind to the caller of bug check.
    //

    ContextRecord = &ProcessorState->ContextFrame;
    LastStack = ContextRecord->Gpr1;
    ControlPc = ContextRecord->Lr - 4;
    NextPc = ControlPc;
    FunctionEntry = KiLookupFunctionEntry(ControlPc);
    if (FunctionEntry != NULL) {
        NextPc = RtlVirtualUnwind(ControlPc,
                                  FunctionEntry,
                                  ContextRecord,
                                  &InFunction,
                                  &EstablisherFrame,
                                  NULL,
                                  0,
                                  0xffffffff);
    }

    //
    // At this point the context record contains the machine state at the
    // call to bug check.
    //
    // Put out the machine state at the time of the bugcheck.
    //

    sprintf(Buffer,
            "\n Machine State at Call to Bug Check    IAR:%08lX MSR:%08lX\n",
            ContextRecord->Lr,
            ContextRecord->Msr);

    HalDisplayString(Buffer);

    //
    // Format and output the integer registers.
    //

    sprintf(Buffer,
            " R0:%8lX  R1:%8lX  R2:%8lX  R3:%8lX  R4:%8lX  R5:%8lX\n",
            ContextRecord->Gpr0,
            ContextRecord->Gpr1,
            ContextRecord->Gpr2,
            ContextRecord->Gpr3,
            ContextRecord->Gpr4,
            ContextRecord->Gpr5);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            " R6:%8lX  R7:%8lX  R8:%8lX  R9:%8lX R10:%8lX R11:%8lX\n",
            ContextRecord->Gpr6,
            ContextRecord->Gpr7,
            ContextRecord->Gpr8,
            ContextRecord->Gpr9,
            ContextRecord->Gpr10,
            ContextRecord->Gpr11);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "R12:%8lX R13:%8lX R14:%8lX R15:%8lX R16:%8lX R17:%8lX\n",
            ContextRecord->Gpr12,
            ContextRecord->Gpr13,
            ContextRecord->Gpr14,
            ContextRecord->Gpr15,
            ContextRecord->Gpr16,
            ContextRecord->Gpr17);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "R18:%8lX R19:%8lX R20:%8lX R21:%8lX R22:%8lX R23:%8lX\n",
            ContextRecord->Gpr18,
            ContextRecord->Gpr19,
            ContextRecord->Gpr20,
            ContextRecord->Gpr21,
            ContextRecord->Gpr22,
            ContextRecord->Gpr23);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "R24:%8lX R25:%8lX R26:%8lX R27:%8lX R28:%8lX R29:%8lX\n",
            ContextRecord->Gpr24,
            ContextRecord->Gpr25,
            ContextRecord->Gpr26,
            ContextRecord->Gpr27,
            ContextRecord->Gpr28,
            ContextRecord->Gpr29);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "R30:%8lX R31:%8lX  CR:%8lX CTR:%8lX XER:%8lX\n",
            ContextRecord->Gpr30,
            ContextRecord->Gpr31,
            ContextRecord->Cr,
            ContextRecord->Ctr,
            ContextRecord->Xer);

    HalDisplayString(Buffer);

#if 0

    //
    // I'd much rather see a longer stack trace and skip the floating
    // point stuff when the system crashes.  plj
    //

    //
    // Format and output the floating registers.
    //
    DumpFloat = (PULONG)(&ContextRecord->Fpr0);
    sprintf(Buffer,
            " F0- F3:%08lX%08lX %08lX%08lX %08lX%08lX %08lX%08lX\n",
            *(DumpFloat+1),
            *DumpFloat,
            *(DumpFloat+3),
            *(DumpFloat+2),
            *(DumpFloat+5),
            *(DumpFloat+4),
            *(DumpFloat+7),
            *(DumpFloat+6));

    HalDisplayString(Buffer);

    DumpFloat = (PULONG)(&ContextRecord->Fpr4);
    sprintf(Buffer,
            " F4- F7:%08lX%08lX %08lX%08lX %08lX%08lX %08lX%08lX\n",
            *(DumpFloat+1),
            *DumpFloat,
            *(DumpFloat+3),
            *(DumpFloat+2),
            *(DumpFloat+5),
            *(DumpFloat+4),
            *(DumpFloat+7),
            *(DumpFloat+6));

    HalDisplayString(Buffer);

    DumpFloat = (PULONG)(&ContextRecord->Fpr8);
    sprintf(Buffer,
            " F8-F11:%08lX%08lX %08lX%08lX %08lX%08lX %08lX%08lX\n",
            *(DumpFloat+1),
            *DumpFloat,
            *(DumpFloat+3),
            *(DumpFloat+2),
            *(DumpFloat+5),
            *(DumpFloat+4),
            *(DumpFloat+7),
            *(DumpFloat+6));

    HalDisplayString(Buffer);

    DumpFloat = (PULONG)(&ContextRecord->Fpr12);
    sprintf(Buffer,
            "F12-F15:%08lX%08lX %08lX%08lX %08lX%08lX %08lX%08lX\n",
            *(DumpFloat+1),
            *DumpFloat,
            *(DumpFloat+3),
            *(DumpFloat+2),
            *(DumpFloat+5),
            *(DumpFloat+4),
            *(DumpFloat+7),
            *(DumpFloat+6));

    HalDisplayString(Buffer);

    DumpFloat = (PULONG)(&ContextRecord->Fpr16);
    sprintf(Buffer,
            "F16-F19:%08lX%08lX %08lX%08lX %08lX%08lX %08lX%08lX\n",
            *(DumpFloat+1),
            *DumpFloat,
            *(DumpFloat+3),
            *(DumpFloat+2),
            *(DumpFloat+5),
            *(DumpFloat+4),
            *(DumpFloat+7),
            *(DumpFloat+6));

    HalDisplayString(Buffer);

    DumpFloat = (PULONG)(&ContextRecord->Fpr20);
    sprintf(Buffer,
            "F20-F23:%08lX%08lX %08lX%08lX %08lX%08lX %08lX%08lX\n",
            *(DumpFloat+1),
            *DumpFloat,
            *(DumpFloat+3),
            *(DumpFloat+2),
            *(DumpFloat+5),
            *(DumpFloat+4),
            *(DumpFloat+7),
            *(DumpFloat+6));

    HalDisplayString(Buffer);

    DumpFloat = (PULONG)(&ContextRecord->Fpr24);
    sprintf(Buffer,
            "F24-F27:%08lX%08lX %08lX%08lX %08lX%08lX %08lX%08lX\n",
            *(DumpFloat+1),
            *DumpFloat,
            *(DumpFloat+3),
            *(DumpFloat+2),
            *(DumpFloat+5),
            *(DumpFloat+4),
            *(DumpFloat+7),
            *(DumpFloat+6));

    HalDisplayString(Buffer);

    DumpFloat = (PULONG)(&ContextRecord->Fpr28);
    sprintf(Buffer,
            "F28-F31:%08lX%08lX %08lX%08lX %08lX%08lX %08lX%08lX\n",
            *(DumpFloat+1),
            *DumpFloat,
            *(DumpFloat+3),
            *(DumpFloat+2),
            *(DumpFloat+5),
            *(DumpFloat+4),
            *(DumpFloat+7),
            *(DumpFloat+6));

    HalDisplayString(Buffer);


    DumpFloat = (PULONG)(&ContextRecord->Fpscr);
    sprintf(Buffer,
            "  FPSCR:%08lX%08lX\n",
            *(DumpFloat+1),
            *DumpFloat);

    HalDisplayString(Buffer);

#define STAKWALK 4
#else
#define STAKWALK 8
#endif

    //
    // Output short stack back trace with base address.
    //

    DllName.Length = 0;
    DllName.Buffer = L"";
    if (FunctionEntry != NULL) {
        StackLimit = (ULONG)KeGetCurrentThread()->KernelStack;
        HalDisplayString("Callee-Sp Return-Ra  Dll Base - Name\n");
        for (Index = 0; Index < STAKWALK; Index += 1) {
            ImageBase = KiPcToFileHeader((PVOID)ControlPc,
                                         &ImageBase,
                                         &DataTableEntry);

            sprintf(Buffer,
                    " %08lX %08lX : %08lX - %s\n",
                    ContextRecord->Gpr1,
                    NextPc + 4,
                    ImageBase,
                    (*UnicodeToAnsiRoutine)( (ImageBase != NULL) ? &DataTableEntry->BaseDllName : &DllName,
                                             AnsiBuffer, sizeof( AnsiBuffer )));

            HalDisplayString(Buffer);

            if ((NextPc != ControlPc) || (ContextRecord->Gpr1 != LastStack)) {
                ControlPc = NextPc;
                LastStack = ContextRecord->Gpr1;
                FunctionEntry = KiLookupFunctionEntry(ControlPc);
                if ((FunctionEntry != NULL) && (LastStack < StackLimit)) {
                    NextPc = RtlVirtualUnwind(ControlPc,
                                              FunctionEntry,
                                              ContextRecord,
                                              &InFunction,
                                              &EstablisherFrame,
                                              NULL,
                                              0,
                                              0xffffffff);
                } else {
                    NextPc = ContextRecord->Lr;
                }

            } else {
                break;
            }
        }
    }

    //
    // Output the build number and other useful information.
    //

    sprintf(Buffer,
           "\nIRQL : %d, DPC Active : %s, SYSVER 0x%08x\n",
           KeGetCurrentIrql(),
           KeIsExecutingDpc() ? "TRUE" : "FALSE",
           NtBuildNumber);

    HalDisplayString(Buffer);

    //
    // Output the processor id and the primary cache sizes.
    //

    sprintf(Buffer,
            "Processor Id: %d.%d, Icache: %d, Dcache: %d",
            PCR->ProcessorVersion,
            PCR->ProcessorRevision,
            PCR->FirstLevelIcacheSize,
            PCR->FirstLevelDcacheSize);

    HalDisplayString(Buffer);

    //
    // If the display width is greater than 80 + 24 (the size of a DLL
    // name and base address), then display all the modules loaded in
    // the system.
    //

    HalQueryDisplayParameters(&DisplayWidth,
                              &DisplayHeight,
                              &DisplayColumn,
                              &DisplayRow);

    if (DisplayWidth > (80 + 24)) {
        HalDisplayString("\n");
        if (KeLoaderBlock != NULL) {
            ModuleListHead = &KeLoaderBlock->LoadOrderListHead;

        } else {
            ModuleListHead = &PsLoadedModuleList;
        }

        //
        // Output display headers.
        //

        Index = 1;
        KiDisplayString(80, Index, "Dll Base DateStmp - Name");
        NextEntry = ModuleListHead->Flink;
        if (NextEntry != NULL) {

            //
            // Scan the list of loaded modules and display their base
            // address and name.
            //

            while (NextEntry != ModuleListHead) {
                Index += 1;
                DataTableEntry = CONTAINING_RECORD(NextEntry,
                                                   LDR_DATA_TABLE_ENTRY,
                                                   InLoadOrderLinks);

                if (MmDbgReadCheck(DataTableEntry->DllBase) != NULL) {
                    PIMAGE_NT_HEADERS NtHeaders;

                    NtHeaders = RtlImageNtHeader(DataTableEntry->DllBase);
                    DateStamp = NtHeaders->FileHeader.TimeDateStamp;

                } else {
                    DateStamp = 0;
                }
                sprintf(Buffer,
                        "%08lX %08lx - %s",
                        DataTableEntry->DllBase,
                        DateStamp,
                        (*UnicodeToAnsiRoutine)( &DataTableEntry->BaseDllName, AnsiBuffer, sizeof( AnsiBuffer )));

                KiDisplayString(80, Index, Buffer);
                NextEntry = NextEntry->Flink;
                if (Index > DisplayHeight) {
                    break;
                }
            }
        }
    }

    //
    // Reset the current display position.
    //

    HalSetDisplayParameters(DisplayColumn, DisplayRow);

    //
    // The system has crashed, if we are running without the Kernel
    // debugger attached, attach it now.
    //

    KdInitSystem(NULL, FALSE);

    return;
}

VOID
KiDisplayString (
    IN ULONG Column,
    IN ULONG Row,
    IN PCHAR Buffer
    )

/*++

Routine Description:

    This function display a string starting at the specified column and row
    position on the screen.

Arguments:

    Column - Supplies the starting column of where the string is displayed.

    Row - Supplies the starting row of where the string is displayed.

    Bufer - Supplies a pointer to the string that is displayed.

Return Value:

    None.

--*/

{

    //
    // Position the cursor and display the string.
    //

    HalSetDisplayParameters(Column, Row);
    HalDisplayString(Buffer);
    return;
}

PRUNTIME_FUNCTION
KiLookupFunctionEntry (
    IN ULONG ControlPc
    )

/*++

Routine Description:

    This function searches the currently active function tables for an entry
    that corresponds to the specified PC value.

Arguments:

    ControlPc - Supplies the address of an instruction within the specified
        function.

Return Value:

    If there is no entry in the function table for the specified PC, then
    NULL is returned. Otherwise, the address of the function table entry
    that corresponds to the specified PC is returned.

--*/

{

    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PRUNTIME_FUNCTION FunctionEntry;
    PRUNTIME_FUNCTION FunctionTable;
    ULONG SizeOfExceptionTable;
    LONG High;
    PVOID ImageBase;
    LONG Low;
    LONG Middle;

    //
    // Search for the image that includes the specified PC value.
    //

    ImageBase = KiPcToFileHeader((PVOID)ControlPc,
                                 &ImageBase,
                                 &DataTableEntry);

    //
    // If an image is found that includes the specified PC, then locate the
    // function table for the image.
    //

    if (ImageBase != NULL) {
        FunctionTable = (PRUNTIME_FUNCTION)RtlImageDirectoryEntryToData(
                         ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                         &SizeOfExceptionTable);

        //
        // If a function table is located, then search the function table
        // for a function table entry for the specified PC.
        //

        if (FunctionTable != NULL) {

            //
            // Initialize search indicies.
            //

            Low = 0;
            High = (SizeOfExceptionTable / sizeof(RUNTIME_FUNCTION)) - 1;

            //
            // Perform binary search on the function table for a function table
            // entry that subsumes the specified PC.
            //

            while (High >= Low) {

                //
                // Compute next probe index and test entry. If the specified PC
                // is greater than of equal to the beginning address and less
                // than the ending address of the function table entry, then
                // return the address of the function table entry. Otherwise,
                // continue the search.
                //

                Middle = (Low + High) >> 1;
                FunctionEntry = &FunctionTable[Middle];
                if (ControlPc < FunctionEntry->BeginAddress) {
                    High = Middle - 1;

                } else if (ControlPc >= FunctionEntry->EndAddress) {
                    Low = Middle + 1;

                } else {

                    //
                    // The capability exists for more than one function entry
                    // to map to the same function. This permits a function to
                    // have (within reason) discontiguous code segment(s). If
                    // PrologEndAddress is out of range, it is re-interpreted
                    // as a pointer to the primary function table entry for
                    // that function.  The out of range test takes into account
                    // the redundant encoding of millicode and glue code.
                    //

                    if (((FunctionEntry->PrologEndAddress < FunctionEntry->BeginAddress) ||
                         (FunctionEntry->PrologEndAddress >= FunctionEntry->EndAddress)) &&
                         (FunctionEntry->PrologEndAddress & 3) == 0) {
                        FunctionEntry = (PRUNTIME_FUNCTION)FunctionEntry->PrologEndAddress;
                    }

                    return FunctionEntry;
                }
            }
        }
    }

    //
    // A function table entry for the specified PC was not found.
    //

    return NULL;
}

PVOID
KiPcToFileHeader(
    IN PVOID PcValue,
    OUT PVOID *BaseOfImage,
    OUT PLDR_DATA_TABLE_ENTRY *DataTableEntry
    )

/*++

Routine Description:

    This function returns the base of an image that contains the
    specified PcValue. An image contains the PcValue if the PcValue
    is within the ImageBase, and the ImageBase plus the size of the
    virtual image.

Arguments:

    PcValue - Supplies a PcValue.

    BaseOfImage - Returns the base address for the image containing the
        PcValue. This value must be added to any relative addresses in
        the headers to locate portions of the image.

    DataTableEntry - Suppies a pointer to a variable that receives the
        address of the data table entry that describes the image.

Return Value:

    NULL - No image was found that contains the PcValue.

    NON-NULL - Returns the base address of the image that contain the
        PcValue.

--*/

{

    PLIST_ENTRY ModuleListHead;
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY Next;
    ULONG Bounds;
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

    ReturnBase = NULL;
    Next = ModuleListHead->Flink;
    if (Next != NULL) {
        while (Next != ModuleListHead) {
            Entry = CONTAINING_RECORD(Next,
                                      LDR_DATA_TABLE_ENTRY,
                                      InLoadOrderLinks);

            Next = Next->Flink;
            Base = Entry->DllBase;
            Bounds = (ULONG)Base + Entry->SizeOfImage;
            if ((ULONG)PcValue >= (ULONG)Base && (ULONG)PcValue < Bounds) {
                *DataTableEntry = Entry;
                ReturnBase = Base;
                break;
            }
        }
    }

    *BaseOfImage = ReturnBase;
    return ReturnBase;
}
