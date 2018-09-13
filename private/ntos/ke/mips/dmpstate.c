/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dmpstate.c

Abstract:

    This module implements  the architecture specific routine that dumps
    the machine state when a bug check occurs and no debugger is hooked
    to the system. It is assumed that it is called from bug check.

Author:

    David N. Cutler (davec) 17-Jan-1992

Environment:

    Kernel mode.

Revision History:

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
    LastStack = (ULONG)ContextRecord->XIntSp;
    ControlPc = (ULONG)(ContextRecord->XIntRa - 4);
    NextPc = ControlPc;
    FunctionEntry = KiLookupFunctionEntry(ControlPc);
    if (FunctionEntry != NULL) {
        NextPc = RtlVirtualUnwind(ControlPc | 1,
                                  FunctionEntry,
                                  ContextRecord,
                                  &InFunction,
                                  &EstablisherFrame,
                                  NULL);
    }

    //
    // At this point the context record contains the machine state at the
    // call to bug check.
    //
    // Put out the machine state at the time of the bugcheck.
    //

    sprintf(Buffer,
            "\nMachine State at Call to Bug Check PC : %08lX PSR : %08lX\n\n",
            (ULONG)ContextRecord->XIntRa,
            ContextRecord->Psr);

    HalDisplayString(Buffer);

    //
    // Format and output the integer registers.
    //

    sprintf(Buffer,
            "AT :%8lX  V0 :%8lX  V1 :%8lX  A0 :%8lX\n",
            (ULONG)ContextRecord->XIntAt,
            (ULONG)ContextRecord->XIntV0,
            (ULONG)ContextRecord->XIntV1,
            (ULONG)ContextRecord->XIntA0);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "A1 :%8lX  A2 :%8lX  A3 :%8lX  T0 :%8lX\n",
            (ULONG)ContextRecord->XIntA1,
            (ULONG)ContextRecord->XIntA2,
            (ULONG)ContextRecord->XIntA3,
            (ULONG)ContextRecord->XIntT0);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "T1 :%8lX  T2 :%8lX  T3 :%8lX  T4 :%8lX\n",
            (ULONG)ContextRecord->XIntT1,
            (ULONG)ContextRecord->XIntT2,
            (ULONG)ContextRecord->XIntT3,
            (ULONG)ContextRecord->XIntT4);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "T5 :%8lX  T6 :%8lX  T7 :%8lX  T8 :%8lX\n",
            (ULONG)ContextRecord->XIntT5,
            (ULONG)ContextRecord->XIntT6,
            (ULONG)ContextRecord->XIntT7,
            (ULONG)ContextRecord->XIntT8);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "T9 :%8lX  S0 :%8lX  S1 :%8lX  S2 :%8lX\n",
            (ULONG)ContextRecord->XIntT9,
            (ULONG)ContextRecord->XIntS0,
            (ULONG)ContextRecord->XIntS1,
            (ULONG)ContextRecord->XIntS2);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "S3 :%8lX  S4 :%8lX  S5 :%8lX  S6 :%8lX\n",
            (ULONG)ContextRecord->XIntS3,
            (ULONG)ContextRecord->XIntS4,
            (ULONG)ContextRecord->XIntS5,
            (ULONG)ContextRecord->XIntS6);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "S7 :%8lX  S8 :%8lX  GP :%8lX  SP :%8lX\n",
            (ULONG)ContextRecord->XIntS7,
            (ULONG)ContextRecord->XIntS8,
            (ULONG)ContextRecord->XIntGp,
            (ULONG)ContextRecord->XIntSp);

     HalDisplayString(Buffer);

     sprintf(Buffer,
            "RA :%8lX  LO :%8lX  HI :%8lX  FSR:%8lX\n",
            (ULONG)ContextRecord->XIntRa,
            (ULONG)ContextRecord->XIntLo,
            (ULONG)ContextRecord->XIntHi,
            (ULONG)ContextRecord->Fsr);

    HalDisplayString(Buffer);

    //
    // Format and output the firswt four floating registers.
    //

    sprintf(Buffer,
            "F0 :%8lX  F1 :%8lX  F2 :%8lX  F3 :%8lX\n",
            ContextRecord->FltF0,
            ContextRecord->FltF1,
            ContextRecord->FltF2,
            ContextRecord->FltF3);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "F4 :%8lX  F5 :%8lX  F6 :%8lX  F7 :%8lX\n",
            ContextRecord->FltF4,
            ContextRecord->FltF5,
            ContextRecord->FltF6,
            ContextRecord->FltF7);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "F8 :%8lX  F9 :%8lX  F10:%8lX  F11:%8lX\n",
            ContextRecord->FltF8,
            ContextRecord->FltF9,
            ContextRecord->FltF10,
            ContextRecord->FltF11);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "F12:%8lX  F13:%8lX  F14:%8lX  F15:%8lX\n",
            ContextRecord->FltF12,
            ContextRecord->FltF13,
            ContextRecord->FltF14,
            ContextRecord->FltF15);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "F16:%8lX  F17:%8lX  F18:%8lX  F19:%8lX\n",
            ContextRecord->FltF16,
            ContextRecord->FltF17,
            ContextRecord->FltF18,
            ContextRecord->FltF19);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "F20:%8lX  F21:%8lX  F22:%8lX  F23:%8lX\n",
            ContextRecord->FltF20,
            ContextRecord->FltF21,
            ContextRecord->FltF22,
            ContextRecord->FltF23);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "F24:%8lX  F25:%8lX  F26:%8lX  F27:%8lX\n",
            ContextRecord->FltF24,
            ContextRecord->FltF25,
            ContextRecord->FltF26,
            ContextRecord->FltF27);

    HalDisplayString(Buffer);

    sprintf(Buffer,
            "F28:%8lX  F29:%8lX  F30:%8lX  F31:%8lX\n\n",
            ContextRecord->FltF28,
            ContextRecord->FltF29,
            ContextRecord->FltF30,
            ContextRecord->FltF31);

    HalDisplayString(Buffer);

    //
    // Output short stack back trace with base address.
    //

    DllName.Length = 0;
    DllName.Buffer = L"";
    if (FunctionEntry != NULL) {
        StackLimit = (ULONG)KeGetCurrentThread()->KernelStack;
        HalDisplayString("Callee-Sp Return-Ra  Dll Base - Name\n");
        for (Index = 0; Index < 8; Index += 1) {
            ImageBase = KiPcToFileHeader((PVOID)ControlPc,
                                         &ImageBase,
                                         &DataTableEntry);

            sprintf(Buffer,
                    " %08lX %08lX : %08lX - %s\n",
                    (ULONG)ContextRecord->XIntSp,
                    NextPc + 4,
                    ImageBase,
                    (*UnicodeToAnsiRoutine)( (ImageBase != NULL) ? &DataTableEntry->BaseDllName : &DllName,
                                             AnsiBuffer, sizeof( AnsiBuffer )));

            HalDisplayString(Buffer);
            if ((NextPc != ControlPc) || ((ULONG)ContextRecord->XIntSp != LastStack)) {
                ControlPc = NextPc;
                LastStack = (ULONG)ContextRecord->XIntSp;
                FunctionEntry = KiLookupFunctionEntry(ControlPc);
                if ((FunctionEntry != NULL) && (LastStack < StackLimit)) {
                    NextPc = RtlVirtualUnwind(ControlPc | 1,
                                              FunctionEntry,
                                              ContextRecord,
                                              &InFunction,
                                              &EstablisherFrame,
                                              NULL);
                } else {
                    NextPc = (ULONG)ContextRecord->XIntRa;
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
            "Processor Id %d.%d, Icache : %d, Dcache : %d\n",
            (PCR->ProcessorId >> 8) & 0xff,
            PCR->ProcessorId & 0xff,
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
                    // have discontiguous code segments described by separate
                    // function table entries. If the ending prologue address
                    // is not within the limits of the begining and ending
                    // address of the function able entry, then the prologue
                    // ending address is the address of a function table entry
                    // that accurately describes the ending prologue address.
                    //

                    if ((FunctionEntry->PrologEndAddress < FunctionEntry->BeginAddress) ||
                        (FunctionEntry->PrologEndAddress >= FunctionEntry->EndAddress)) {
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
