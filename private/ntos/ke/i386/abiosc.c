/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    abiosc.c

Abstract:

    This module implements ABIOS support C routines for i386 NT.

Author:

    Shie-Lin Tzong (shielint) 20-May-1991

Environment:

    Boot loader privileged, FLAT mode.


Revision History:

--*/

#include "ki.h"
#pragma hdrstop
#include "abios.h"

extern PKCOMMON_DATA_AREA KiCommonDataArea;
extern BOOLEAN KiAbiosPresent;

//
// The reason of having these variables defined in here is to isolate
// ABIOS from current system.
//

//
// KiNumberFreeSelectors defines the number of available selectors for
// ABIOS specific drivers.  This number should be the same accross all
// the processors.
//

static USHORT KiNumberFreeSelectors = 0;

//
// KiFreeGdtListHead points to the head of free GDT list on the processor 0.
//

static PKFREE_GDT_ENTRY KiFreeGdtListHead = 0L;

//
// Logica Id Table to control the ownership of logical Id.
//

PKLID_TABLE_ENTRY KiLogicalIdTable;

//
// KiAbiosGdt[] defines the Starting address of GDT for each processor.
//

ULONG KiAbiosGdt[MAXIMUM_PROCESSORS];

//
// SpinLock for accessing GDTs
//

KSPIN_LOCK KiAbiosGdtLock;

//
// Spinlock for accessing Logical Id Table
//

KSPIN_LOCK KiAbiosLidTableLock;

//
// KiStack16GdtEntry defines the address of the gdt entry for 16 bit stack.
//

ULONG KiStack16GdtEntry;

VOID
KiInitializeAbiosGdtEntry (
    OUT PKGDTENTRY GdtEntry,
    IN ULONG Base,
    IN ULONG Limit,
    IN USHORT Type
    )

/*++

Routine Description:

    This function initializes a GDT entry for abios specific code.  Base,
    Limit, and Type (code, data) are set according to parameters.  All other
    fields of the entry are set to match standard system values.

    N.B. The BIG and GRANULARITY are always set to 0.

Arguments:

    GdtEntry - GDT descriptor to be filled in.

    Base - Linear address of the first byte mapped by the selector.

    Limit - Size of the selector in BYTE.

    Type - Code or Data.  All code selectors are marked readable,
            all data selectors are marked writeable.

Return Value:

    Pointer to the GDT entry.

--*/

{
    GdtEntry->LimitLow = (USHORT)(Limit & 0xffff);
    GdtEntry->BaseLow = (USHORT)(Base & 0xffff);
    GdtEntry->HighWord.Bytes.BaseMid = (UCHAR)((Base & 0xff0000) >> 16);
    GdtEntry->HighWord.Bits.Type = Type;
    GdtEntry->HighWord.Bits.Dpl = 0;
    GdtEntry->HighWord.Bits.Pres = 1;
    GdtEntry->HighWord.Bits.LimitHi = (Limit & 0xf0000) >> 16;
    GdtEntry->HighWord.Bits.Sys = 0;
    GdtEntry->HighWord.Bits.Reserved_0 = 0;
    GdtEntry->HighWord.Bits.Default_Big = 0;
    GdtEntry->HighWord.Bits.Granularity = 0;
    GdtEntry->HighWord.Bytes.BaseHi = (UCHAR)((Base & 0xff000000) >> 24);
}

ULONG
KiI386SelectorBase (
    IN USHORT Selector
    )

/*++

Routine Description:

    This function returns the base address of the specified GDT selector.

Arguments:

    Selector - Supplies the desired selector.

Return Value:

    SelectorBase - Return the base address of the specified selector;
                   (return -1L if invalid selector)


--*/

{
    PKGDTENTRY GdtEntry;


    GdtEntry = (PKGDTENTRY)(KiAbiosGetGdt() + Selector);
    if (GdtEntry->HighWord.Bits.Pres) {
        return ((ULONG)GdtEntry->BaseLow |
                (ULONG)GdtEntry->HighWord.Bytes.BaseMid << 16 |
                (ULONG)GdtEntry->HighWord.Bytes.BaseHi << 24);
    } else {
        return (ULONG)(-1L);
    }
}

NTSTATUS
KeI386GetLid(
    IN USHORT DeviceId,
    IN USHORT RelativeLid,
    IN BOOLEAN SharedLid,
    IN PDRIVER_OBJECT DriverObject,
    OUT PUSHORT LogicalId
    )

/*++

Routine Description:

    This function searches Device Blocks and Common Data Area for the
    Logical Id matching the specified Device Id.

    N.B. (WARNING shielint) To speed the search, this routine ASSUMES that
    the LIDs with the same Device ID always appear consecutively in the
    Common Data Area.  IBM ABIOS doc does not explicitly specify this.
    But from the way ABIOS initializes Device Block and Function Transfer
    Table, I think the assumption is true.

Arguments:

    DeviceId - Desired Device Id.

    RelativeLid - Specifies the Nth logical Id for this device Id.  A value
                  of 0 indicates the first available Lid.

    SharedLid - A boolean value indicates if it is a shared or exclusively
                owned logical Id.

    DriverObject - Supplies a 32-bit flat pointer of the requesting device
                driver's driver object.  The DriverObject is used to establish
                the ownership of the desired LID.

    LogicalId - A pointer to a variable which will receive the Lid.

Return Value:

    STATUS_SUCCESS - If the requested LID is available.

    STATUS_ABIOS_NOT_PRESENT - If there is no ABIOS support in the system.

    STATUS_ABIOS_LID_NOT_EXIST - If the specified LID does not exist.

    STATUS_ABIOS_LID_ALREADY_OWNED - If the caller requests an exclusively
                                     owned LID.

--*/

{
    PKDB_FTT_SECTION CdaPointer;
    PKDEVICE_BLOCK DeviceBlock;
    USHORT Lid, RelativeLidCount = 1;
    ULONG Owner;
    USHORT Increment;
    KIRQL OldIrql;
    NTSTATUS Status;

    if (!KiAbiosPresent) {
        return STATUS_ABIOS_NOT_PRESENT;
    }

    if (SharedLid) {
        Owner = LID_NO_SPECIFIC_OWNER;
        Increment = 1;
    } else {
        Owner = (ULONG)DriverObject;
        Increment = 0;
    }

    //
    // If the Logical Id Table hasn't been created yet, create it now.
    //
    if (KiLogicalIdTable==NULL) {
        KiLogicalIdTable = ExAllocatePoolWithTag(NonPagedPool,
                                          NUMBER_LID_TABLE_ENTRIES *
                                          sizeof(KLID_TABLE_ENTRY),
                                          '  eK');
        if (KiLogicalIdTable == NULL) {
            return(STATUS_NO_MEMORY);
        }
        RtlZeroMemory(KiLogicalIdTable, NUMBER_LID_TABLE_ENTRIES*sizeof(KLID_TABLE_ENTRY));
    }

    //
    // For each Lid defined in Common Data Area, we check if it has non
    // empty device block and function transfer table.  If yes, we proceed
    // to check the device id.  Otherwise, we skip the Lid.
    //

    CdaPointer = (PKDB_FTT_SECTION)KiCommonDataArea + 2;
    Status = STATUS_ABIOS_LID_NOT_EXIST;

    ExAcquireSpinLock(&KiAbiosLidTableLock, &OldIrql);

    for (Lid = 2; Lid < KiCommonDataArea->NumberLids; Lid++) {
        if (CdaPointer->DeviceBlock.Selector != 0 &&
            CdaPointer->FunctionTransferTable.Selector != 0) {

            DeviceBlock = (PKDEVICE_BLOCK)(KiI386SelectorBase(
                                               CdaPointer->DeviceBlock.Selector)
                                           + (CdaPointer->DeviceBlock.Offset));
            if (DeviceBlock->DeviceId == DeviceId) {
                if (RelativeLid == RelativeLidCount || RelativeLid == 0) {
                    if (KiLogicalIdTable[Lid].Owner == 0L) {
                        KiLogicalIdTable[Lid].Owner = Owner;
                        KiLogicalIdTable[Lid].OwnerCount += Increment;
                        *LogicalId = Lid;
                        Status = STATUS_SUCCESS;
                    } else if (KiLogicalIdTable[Lid].Owner == LID_NO_SPECIFIC_OWNER) {
                        if (SharedLid) {
                            *LogicalId = Lid;
                            KiLogicalIdTable[Lid].OwnerCount += Increment;
                            Status = STATUS_SUCCESS;
                        } else {
                            Status = STATUS_ABIOS_LID_ALREADY_OWNED;
                        }
                    } else if (KiLogicalIdTable[Lid].Owner == (ULONG)DriverObject) {
                        *LogicalId = Lid;
                        Status = STATUS_SUCCESS;
                    } else if (RelativeLid != 0) {
                        Status = STATUS_ABIOS_LID_ALREADY_OWNED;
                    }
                    break;
                } else {
                    RelativeLidCount++;
                }
            }
        }
        CdaPointer++;
    }

    ExReleaseSpinLock(&KiAbiosLidTableLock, OldIrql);
    return Status;
}

NTSTATUS
KeI386ReleaseLid(
    IN USHORT LogicalId,
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This function releases a logical Id.  This routine is called at ABIOS
    device driver destallation or termination.

Arguments:

    LogicalId - Logical Id to be released.

    DriverObject - Supplies a 32-bit flat pointer of the requesting device
                driver's driver object.  The DriverObject is used to check
                the ownership of the specified LID.

Return Value:

    STATUS_SUCCESS - If the requested LID is released.

    STATUS_ABIOS_NOT_PRESENT - If there is no ABIOS support in the system.

    STATUS_ABIOS_NOT_LID_OWNER - If the caller does not own the LID.

--*/

{
    KIRQL OldIrql;
    NTSTATUS Status;

    if (!KiAbiosPresent) {
        return STATUS_ABIOS_NOT_PRESENT;
    }

    ExAcquireSpinLock(&KiAbiosLidTableLock, &OldIrql);

    if (KiLogicalIdTable[LogicalId].Owner == (ULONG)DriverObject) {
        KiLogicalIdTable[LogicalId].Owner = 0L;
        Status = STATUS_SUCCESS;
    } else if (KiLogicalIdTable[LogicalId].Owner == LID_NO_SPECIFIC_OWNER) {
        KiLogicalIdTable[LogicalId].OwnerCount--;
        if (KiLogicalIdTable[LogicalId].OwnerCount == 0L) {
            KiLogicalIdTable[LogicalId].Owner = 0L;
        }
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_ABIOS_NOT_LID_OWNER;
    }

    ExReleaseSpinLock(&KiAbiosLidTableLock, OldIrql);

    return Status;
}

NTSTATUS
KeI386AbiosCall(
    IN USHORT LogicalId,
    IN PDRIVER_OBJECT DriverObject,
    IN PUCHAR RequestBlock,
    IN USHORT EntryPoint
    )

/*++

Routine Description:

    This function calls an ABIOS service routine on behave of device driver
    using Operating System Transfer Convension.

Arguments:

    LogicalId - Logical Id for the call.

    DriverObject - Supplies a 32-bit flat pointer of the requesting device
                driver's driver object.  The DriverObject is used to verify
                the ownership of the desired LID.

    RequestBlock - A 16:16 (selector:offset) pointer to the request block.

    EntryPoint - Specifies which ABIOS entry point:

                 0 - Start Routine
                 1 - Interrupt Routine
                 2 - Timeout Routine

Return Value:

    STATUS_SUCCESS - If no error.

    STATUS_ABIOS_NOT_PRESENT - If there is no ABIOS support in the system.

    STATUS_ABIOS_INVALID_COMMAND - if the specified entry point is not supported.

    STATUS_ABIOS_INVALID_LID - If the Lid specified is invalid.

    STATUS_ABIOS_NOT_LID_OWNER - If the caller does not own this Lid.

    (Note that the request specific ABIOS returned code is in RequestBlock.)

--*/

{

    KABIOS_POINTER FuncTransferTable;
    KABIOS_POINTER DeviceBlock;
    KABIOS_POINTER AbiosFunction;
    PKFUNCTION_TRANSFER_TABLE FttPointer;

    if (!KiAbiosPresent) {
        return STATUS_ABIOS_NOT_PRESENT;
    }

    if (LogicalId >= KiCommonDataArea->NumberLids) {
        return STATUS_ABIOS_INVALID_LID;
    } else if (KiLogicalIdTable[LogicalId].Owner != (ULONG)DriverObject &&
               KiLogicalIdTable[LogicalId].Owner != LID_NO_SPECIFIC_OWNER) {
        return STATUS_ABIOS_NOT_LID_OWNER;
    } else if (EntryPoint > 2) {
        return STATUS_ABIOS_INVALID_COMMAND;
    }

    FuncTransferTable = ((PKDB_FTT_SECTION)KiCommonDataArea + LogicalId)->
                                               FunctionTransferTable;
    DeviceBlock = ((PKDB_FTT_SECTION)KiCommonDataArea + LogicalId)->DeviceBlock;
    FttPointer = (PKFUNCTION_TRANSFER_TABLE)(KiI386SelectorBase(FuncTransferTable.Selector) +
                                             (ULONG)FuncTransferTable.Offset);
    AbiosFunction = FttPointer->CommonRoutine[EntryPoint];
    KiI386CallAbios(AbiosFunction,
                    DeviceBlock,
                    FuncTransferTable,
                    *(PKABIOS_POINTER)&RequestBlock
                    );

    return STATUS_SUCCESS;
}

NTSTATUS
KeI386AllocateGdtSelectors(
    OUT PUSHORT SelectorArray,
    IN USHORT NumberOfSelectors
    )

/*++

Routine Description:

    This function allocates a set of GDT selectors for a device driver to use.
    Usually this allocation is performed at device driver initialization time
    to reserve the selectors for later use.

Arguments:

    SelectorArray - Supplies a pointer to an array of USHORT to be filled
                    in with the GDT selectors allocated.

    NumberOfSelectors - Specifies the number of selectors to be allocated.

Return Value:

    STATUS_SUCCESS - If the requested selectors are allocated.

    STATUS_ABIOS_SELECTOR_NOT_AVAILABLE - if systen can not allocate the number
                               of selectors requested.

--*/

{
    PKFREE_GDT_ENTRY GdtEntry;
    KIRQL OldIrql;

    if (KiNumberFreeSelectors >= NumberOfSelectors) {
        ExAcquireSpinLock(&KiAbiosGdtLock, &OldIrql);

        //
        // The Free Gdt link list is maintained on Processor 0's GDT ONLY.
        // Because the 'selector' is an offset to the beginning of GDT and
        // it should be the same accross all the processors.
        //

        KiNumberFreeSelectors -= NumberOfSelectors;
        GdtEntry = KiFreeGdtListHead;
        while (NumberOfSelectors != 0) {
            *SelectorArray++ = (USHORT)((ULONG)GdtEntry - KiAbiosGdt[0]);
            GdtEntry = GdtEntry->Flink;
            NumberOfSelectors--;
        }
        KiFreeGdtListHead = GdtEntry;
        ExReleaseSpinLock(&KiAbiosGdtLock, OldIrql);
        return STATUS_SUCCESS;
    } else {
        return STATUS_ABIOS_SELECTOR_NOT_AVAILABLE;
    }
}

NTSTATUS
KeI386ReleaseGdtSelectors(
    OUT PUSHORT SelectorArray,
    IN USHORT NumberOfSelectors
    )

/*++

Routine Description:

    This function releases a set of GDT selectors for a device driver.
    Usually this function is called at device driver termination or
    deinstallation time.

Arguments:

    SelectorArray - Supplies a pointer to an array of USHORT selectors
                    to be freed.

    NumberOfSelectors - Specifies the number of selectors to be released.

Return Value:

    STATUS_SUCCESS - If the requested LID is released.

--*/
{
    PKFREE_GDT_ENTRY GdtEntry;
    KIRQL OldIrql;
    ULONG Gdt;

    ExAcquireSpinLock(&KiAbiosGdtLock, &OldIrql);

    //
    // The Free Gdt link list is maintained on Processor 0's GDT ONLY.
    // Because the 'selector' is an offset to the beginning of GDT and
    // it should be the same accross all the processors.
    //

    KiNumberFreeSelectors += NumberOfSelectors;
    Gdt = KiAbiosGdt[0];
    while (NumberOfSelectors != 0) {
        GdtEntry = (PKFREE_GDT_ENTRY)(Gdt + *SelectorArray++);
        GdtEntry->Flink = KiFreeGdtListHead;
        KiFreeGdtListHead = GdtEntry;
        NumberOfSelectors--;
    }
    ExReleaseSpinLock(&KiAbiosGdtLock, OldIrql);
    return STATUS_SUCCESS;
}

NTSTATUS
KeI386FlatToGdtSelector(
    IN ULONG SelectorBase,
    IN USHORT Length,
    IN USHORT Selector
    )

/*++

Routine Description:

    This function converts a 32-bit flat address to a GDT selector-offset
    pair.  The segment set up is always 16-bit ring 0 data segment.

Arguments:

    SelectorBase - Supplies 32 bit flat address to be set as the base address
                   of the desired selector.

    Length - Supplies the Length of the segment.  The Length is a 16 bit value
             and zero means 64KB.

    Selector - Supplies the selector to be set up.

Return Value:

    STATUS_SUCCESS - If the requested LID is released.

    STATUS_ABIOS_NOT_PRESENT - If there is no ABIOS support in the system.

    STATUS_ABIOS_INVALID_SELECTOR - If the selector supplied is invalid.


--*/

{
    PKGDTENTRY GdtEntry, GdtEntry1;
    KIRQL OldIrql;
    ULONG i;

    if (!KiAbiosPresent) {
        return STATUS_ABIOS_NOT_PRESENT;
    }
    if (Selector < RESERVED_GDT_ENTRIES * sizeof(KGDTENTRY)) {
        return STATUS_ABIOS_INVALID_SELECTOR;
    } else {
        ExAcquireSpinLock(&KiAbiosGdtLock, &OldIrql);
        GdtEntry = (PKGDTENTRY)(KiAbiosGdt[0] + Selector);
        GdtEntry->LimitLow = (USHORT)(Length - 1);
        GdtEntry->BaseLow = LOWWORD(SelectorBase);
        GdtEntry->HighWord.Bytes.BaseMid = LOWBYTE(HIGHWORD(SelectorBase));
        GdtEntry->HighWord.Bytes.BaseHi = HIGHBYTE(HIGHWORD(SelectorBase));
        GdtEntry->HighWord.Bits.Pres = 1;
        GdtEntry->HighWord.Bits.Type = TYPE_DATA;
        GdtEntry->HighWord.Bits.Dpl = DPL_SYSTEM;
        for (i = 1; i < (ULONG)KeNumberProcessors; i++) {
            GdtEntry1 = (PKGDTENTRY)(KiAbiosGdt[i] + Selector);
            *GdtEntry1 = *GdtEntry;
        }
        ExReleaseSpinLock(&KiAbiosGdtLock, OldIrql);
        return STATUS_SUCCESS;
    }
}

VOID
Ki386InitializeGdtFreeList (
    PKFREE_GDT_ENTRY EndOfGdt
    )

/*++

Routine Description:

    This function initializes gdt free list by linking all the unused gdt
    entries to a free list.

Arguments:

    EndOfGdt - Supplies the ending address of desired GDT.

Return Value:

    None.

--*/
{
    PKFREE_GDT_ENTRY GdtEntry;

    GdtEntry = EndOfGdt - 1;
    KiFreeGdtListHead = (PKFREE_GDT_ENTRY)0;
    while (GdtEntry != (PKFREE_GDT_ENTRY)KiAbiosGetGdt() +
                        RESERVED_GDT_ENTRIES - 1) {
        if (GdtEntry->Present == 0) {
            GdtEntry->Flink = KiFreeGdtListHead;
            KiFreeGdtListHead = GdtEntry;
            KiNumberFreeSelectors++;
        }
        GdtEntry--;
    }
}

VOID
KiInitializeAbios (
    IN UCHAR Processor
    )

/*++

Routine Description:

    This function initializes gdt free list and sets up selector for
    KiI386AbiosCall (16-bit code).

Arguments:

    Processor - the processor who performs the initialization.

Return Value:

    None.

--*/

{

    ULONG GdtLength;
    PKGDTENTRY AliasGdtSelectorEntry;
    PKFREE_GDT_ENTRY EndOfGdt;

    //
    // First check if abios is recognized by osloader.
    //

    KiCommonDataArea = KeLoaderBlock->u.I386.CommonDataArea;

    //
    // NOTE For now we want to disable ABIOS support on MP.
    //

    if (KiCommonDataArea == NULL || Processor != 0) {
        KiAbiosPresent = FALSE;
    } else {
        KiAbiosPresent = TRUE;
    }

    //
    // Initialize the spinlocks for accessing GDTs and Lid Table.
    //

    KeInitializeSpinLock( &KiAbiosGdtLock );
    KeInitializeSpinLock( &KiAbiosLidTableLock );

    //
    // Determine the starting and ending addresses of GDT.
    //

    KiAbiosGdt[Processor] = KiAbiosGetGdt();

    AliasGdtSelectorEntry = (PKGDTENTRY)(KiAbiosGetGdt() + KGDT_GDT_ALIAS);
    GdtLength = 1 + (ULONG)(AliasGdtSelectorEntry->LimitLow) +
                (ULONG)(AliasGdtSelectorEntry->HighWord.Bits.LimitHi << 16);
    EndOfGdt = (PKFREE_GDT_ENTRY)(KiAbiosGetGdt() + GdtLength);

    //
    // Prepare selector for 16 bit stack segment
    //

    KiStack16GdtEntry = KiAbiosGetGdt() + KGDT_STACK16;

    KiInitializeAbiosGdtEntry(
                (PKGDTENTRY)KiStack16GdtEntry,
                0L,
                0xffff,
                TYPE_DATA
                );

    //
    // Establish the addressability of Common Data Area selector.
    //

    KiInitializeAbiosGdtEntry(
                (PKGDTENTRY)(KiAbiosGetGdt() + KGDT_CDA16),
                (ULONG)KiCommonDataArea,
                0xffff,
                TYPE_DATA
                );

    //
    // Set up 16-bit code selector for KiI386AbiosCall
    //

    KiInitializeAbiosGdtEntry(
                (PKGDTENTRY)(KiAbiosGetGdt() + KGDT_CODE16),
                (ULONG)&KiI386CallAbios,
                (ULONG)&KiEndOfCode16 - (ULONG)&KiI386CallAbios - 1,
                0x18                   // TYPE_CODE
                );

    //
    // Link all the unused GDT entries to our GDT free list.
    //

    if (Processor == 0) {
        Ki386InitializeGdtFreeList(EndOfGdt);
    }
}
