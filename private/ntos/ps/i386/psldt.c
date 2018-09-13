/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    psldt.c

Abstract:

    This module contains code for the process and thread ldt support

Author:

    Dave Hastings (daveh) 20 May 1991

Notes:

    We return STATUS_SUCCESS for exceptions resulting from accessing the
    process info structures in user space.  This is by design (so Markl
    tells me).  By the time we reach these functions all of the parameters
    in user space have been probed, so if we get an exception it indicates
    that the user is altering portions of his address space that have been
    passed to the system.

    The paged pool consumed by the Ldt is returned to the system at process
    deletion time.  The process deletion handler calls PspDeleteLdt.  We
    do not keep a reference to the process once the ldt is created.

    We capture the user mode parameters into local parameters to prevent
    the possibility that the user will change them after we validate them.

Revision History:

--*/

#include "psp.h"

//
// Internal constants
//

#define DESCRIPTOR_GRAN     0x00800000
#define DESCRIPTOR_NP       0x00008000
#define DESCRIPTOR_SYSTEM   0x00001000
#define DESCRIPTOR_CONFORM  0x00001C00
#define DESCRIPTOR_DPL      0x00006000
#define DESCRIPTOR_TYPEDPL  0x00007F00


KMUTEX LdtMutex;

//
// Internal subroutines
//

PLDT_ENTRY
PspCreateLdt(
    IN PLDT_ENTRY Ldt,
    IN ULONG Offset,
    IN ULONG Size,
    IN ULONG AllocationSize
    );

BOOLEAN
PspIsDescriptorValid(
    IN PLDT_ENTRY Descriptor
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, PspLdtInitialize)
#pragma alloc_text(PAGE, NtSetLdtEntries)
#pragma alloc_text(PAGE, PspDeleteLdt)
#pragma alloc_text(PAGE, PspQueryLdtInformation)
#pragma alloc_text(PAGE, PspSetLdtSize)
#pragma alloc_text(PAGE, PspSetLdtInformation)
#pragma alloc_text(PAGE, PspCreateLdt)
#pragma alloc_text(PAGE, PspIsDescriptorValid)
#pragma alloc_text(PAGE, PspQueryDescriptorThread)
#endif


NTSTATUS
PspLdtInitialize(
    )

/*++

Routine Description:

    This routine initializes the Ldt support for the x86

Arguments:

    None

Return Value:

    TBS
--*/
{
    KeInitializeMutex( &LdtMutex, MUTEX_LEVEL_PS_LDT );
    return STATUS_SUCCESS;
}


NTSTATUS
PspQueryLdtInformation(
    IN PEPROCESS Process,
    OUT PPROCESS_LDT_INFORMATION LdtInformation,
    IN ULONG LdtInformationLength,
    OUT PULONG ReturnLength
    )
/*++

Routine Description:

    This function performs the work for the Ldt portion of the query
    process information function.  It copies the contents of the Ldt
    for the specified process into the users buffer, up to the length
    of the buffer.

Arguments:

    Process -- Supplies a pointer to the process to return LDT info for
    LdtInformation -- Supplies a pointer to the buffer
    ReturnLength -- Returns the number of bytes put into the buffer

Return Value:

    TBS
--*/
{
    ULONG CopyLength, CopyEnd;
    NTSTATUS Status;
    ULONG HeaderLength;
    ULONG Length, Start;
    LONG MutexStatus;
    PLDTINFORMATION ProcessLdtInfo;
    BOOLEAN ReturnNow = FALSE;

    PAGED_CODE();

    //
    // Verify the parameters
    //

    if ( LdtInformationLength < (ULONG)sizeof(PROCESS_LDT_INFORMATION) ) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    //
    // This portion of the parameters may be in user space
    //
    try {
        //
        // Capture parameters
        //
        Length = LdtInformation->Length;
        Start = LdtInformation->Start;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        ReturnNow = TRUE;
    }

    if (ReturnNow) {
        return STATUS_SUCCESS;
    }

    //
    // The buffer containing the Ldt entries must be in the information
    // structure.  We subtract one Ldt entry, because the structure is
    // declared to contain one.
    //
    if (LdtInformationLength - sizeof(PROCESS_LDT_INFORMATION) + sizeof(LDT_ENTRY) < Length) {

        return STATUS_INFO_LENGTH_MISMATCH;
    }

    // An Ldt entry is a processor structure, and must be 8 bytes long
    ASSERT((sizeof(LDT_ENTRY) == 8));

    //
    // The length of the structure must be an even number of Ldt entries
    //
    if (Length % sizeof(LDT_ENTRY)) {
        return STATUS_INVALID_LDT_SIZE;
    }

    //
    // The information to get from the Ldt must start on an Ldt entry
    // boundary.
    //
    if (Start % sizeof(LDT_ENTRY)) {
        return STATUS_INVALID_LDT_OFFSET;
    }

    //
    // Acquire the Ldt mutex
    //

    Status = KeWaitForSingleObject(
                &LdtMutex,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return Status;
    }

    ProcessLdtInfo = Process->LdtInformation;

    //
    // If the process has an Ldt
    //
    if (( ProcessLdtInfo) && (ProcessLdtInfo->Size )) {

        ASSERT ((ProcessLdtInfo->Ldt));

        //
        // Set the end of the copy to be the smaller of:
        //  the end of the information the user requested or
        //  the end of the information that is actually there
        //

        if (ProcessLdtInfo->Size < Start) {
           CopyEnd = Start;
        } else if (ProcessLdtInfo->Size - Start  > Length) {
            CopyEnd = Length + Start;
        } else {
            CopyEnd = ProcessLdtInfo->Size;
        }

        CopyLength = CopyEnd - Start;

        try {

            //
            // Set the length field to the actual length of the Ldt
            //
            LdtInformation->Length = ProcessLdtInfo->Size;

            //
            // Copy the contents of the Ldt into the user's buffer
            //

            if (CopyLength) {
                RtlMoveMemory(
                    &(LdtInformation->LdtEntries),
                    (PCHAR)ProcessLdtInfo->Ldt + Start,
                    CopyLength
                    );
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            ReturnNow = TRUE;
            MutexStatus = KeReleaseMutex( &LdtMutex, FALSE );
            ASSERT(( MutexStatus == 0 ));
        }


        if (ReturnNow) {
            return STATUS_SUCCESS;
        }

    } else {

        //
        // There is no Ldt
        //

        CopyLength = 0;
        try {
            LdtInformation->Length = 0;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            ReturnNow = TRUE;
            MutexStatus = KeReleaseMutex( &LdtMutex, FALSE );
            ASSERT(( MutexStatus == 0 ));
        }

        if (ReturnNow) {
            return STATUS_SUCCESS;
        }
    }

    //
    // Set the length of the information returned
    //
    if ( ARGUMENT_PRESENT(ReturnLength) ) {
        try {
            HeaderLength = (PCHAR)(&(LdtInformation->LdtEntries)) -
                (PCHAR)(&(LdtInformation->Start));
            *ReturnLength = CopyLength + HeaderLength;
        } except(EXCEPTION_EXECUTE_HANDLER){
            // We don't do anything here because we want to return success
        }
    }

    MutexStatus = KeReleaseMutex( &LdtMutex, FALSE );
    ASSERT(( MutexStatus == 0 ));
    return STATUS_SUCCESS;
}


NTSTATUS
PspSetLdtSize(
    IN PEPROCESS Process,
    IN PPROCESS_LDT_SIZE LdtSize,
    IN ULONG LdtSizeLength
    )

/*++

Routine Description:

    This routine changes the LDT size.  It will shrink the LDT, but not
    grow it.  If the LDT shrinks by 1 or more pages from its current allocation,
    the LDT will be reallocated for the new smaller size.  If the allocated
    size of the LDT changes, the quota charge for the Ldt will be reduced.

Arguments:

    Process -- Supplies a pointer to the process whose Ldt is to be sized
    LdtSize -- Supplies a pointer to the size information

Return Value:

    TBS
--*/
{
    ULONG OldSize = 0;
    LONG MutexState;
    ULONG Length;
    PLDT_ENTRY OldLdt = NULL;
    NTSTATUS Status;
    PLDTINFORMATION ProcessLdtInfo;
    BOOLEAN ReturnNow = FALSE;
    PLDT_ENTRY Ldt;

    PAGED_CODE();

    //
    // Verify the parameters
    //
    if ( LdtSizeLength != (ULONG)sizeof( PROCESS_LDT_SIZE ) ){
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    //
    // The following parameters may be in user space
    //
    try {
        //
        // Capture the new Ldt length
        //
        Length = LdtSize->Length;

    } except(EXCEPTION_EXECUTE_HANDLER){
        ReturnNow = TRUE;
    }

    if (ReturnNow) {
        return STATUS_SUCCESS;
    }

    ASSERT((sizeof(LDT_ENTRY) == 8));

    //
    // The Ldt must always be an integral number of LDT_ENTRIES
    //
    if (Length % sizeof(LDT_ENTRY)) {
        return STATUS_INVALID_LDT_SIZE;
    }

    //
    // Acquire the Ldt Mutex
    //

    Status = KeWaitForSingleObject(
                &LdtMutex,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
    if ( !(NT_SUCCESS(Status)) ) {
        return Status;
    }

    //
    // If there isn't an Ldt we can't set the size of the LDT
    //
    ProcessLdtInfo = Process->LdtInformation;
    if ((ProcessLdtInfo == NULL) || (ProcessLdtInfo->Size == 0)) {
        MutexState = KeReleaseMutex( &LdtMutex, FALSE );
        ASSERT((MutexState == 0));
        return STATUS_NO_LDT;
    }

    //
    // This function cannot be used to grow the Ldt
    //
    if (Length > ProcessLdtInfo->Size) {
        MutexState = KeReleaseMutex( &LdtMutex, FALSE );
        ASSERT((MutexState == 0));
        return STATUS_INVALID_LDT_SIZE;
    }

    //
    // Later, we will set ProcessLdtInfo->Ldt = Ldt.  We may set the value
    // of Ldt in the if statement below, but there is one case where we
    // don't
    //
    Ldt = ProcessLdtInfo->Ldt;

    //
    // Adjust the size of the LDT
    //

    ProcessLdtInfo->Size = Length;

    //
    // Free some of the Ldt memory if conditions allow
    //

    if ( Length == 0 ) {

        OldSize = ProcessLdtInfo->AllocatedSize;
        OldLdt = ProcessLdtInfo->Ldt;

        ProcessLdtInfo->AllocatedSize = 0;
        Ldt = NULL;

    } else if ( (ProcessLdtInfo->AllocatedSize - ProcessLdtInfo->Size) >=
        PAGE_SIZE
    ) {

        OldSize = ProcessLdtInfo->AllocatedSize;
        OldLdt = ProcessLdtInfo->Ldt;

        //
        // Calculate new Ldt size (lowest integer number of pages
        // large enough)
        //

        ProcessLdtInfo->AllocatedSize = (ProcessLdtInfo->Size + PAGE_SIZE - 1)
            & ~(PAGE_SIZE - 1);

        //
        // Reallocate and copy the Ldt
        //
        Ldt = PspCreateLdt(
            ProcessLdtInfo->Ldt,
            0,
            ProcessLdtInfo->Size,
            ProcessLdtInfo->AllocatedSize
            );

        if ( Ldt == NULL ) {
            // We cannot reduce the allocation, but we can reduce the
            // Ldt selector limit (done using Ke386SetLdtProcess)
            Ldt = OldLdt;
            ProcessLdtInfo->AllocatedSize = OldSize;
            OldLdt = NULL;
        }
    }

    ProcessLdtInfo->Ldt = Ldt;

    //
    // Change the limit on the Process Ldt
    //

    Ke386SetLdtProcess(
        &(Process->Pcb),
        ProcessLdtInfo->Ldt,
        ProcessLdtInfo->Size
        );

    //
    // If we resized the Ldt, free to old one and reduce the quota charge
    //

    if (OldLdt) {
        ExFreePool( OldLdt );

        PsReturnPoolQuota(
            Process,
            PagedPool,
            OldSize - ProcessLdtInfo->AllocatedSize
            );
    }

    MutexState = KeReleaseMutex( &LdtMutex, FALSE );
    ASSERT((MutexState == 0));
    return STATUS_SUCCESS;
}


NTSTATUS
PspSetLdtInformation(
    IN PEPROCESS Process,
    IN PPROCESS_LDT_INFORMATION LdtInformation,
    IN ULONG LdtInformationLength
    )

/*++

Routine Description:

    This function alters the ldt for a specified process.  It can alter
    portions of the LDT, or the whole LDT.  If an Ldt is created or
    grown, the specified process will be charged the quota for the LDT.
    Each descriptor that is set will be verified.

Arguments:

    Process -- Supplies a pointer to the process whose Ldt is to be modified
    LdtInformation -- Supplies a pointer to the information about the Ldt
        modifications
    LdtInformationLength -- Supplies the length of the LdtInformation
        structure.
Return Value:

    TBS
--*/
{
    NTSTATUS Status;
    PLDT_ENTRY OldLdt = NULL;
    ULONG OldSize = 0;
    ULONG AllocatedSize;
    ULONG Size;
    ULONG MutexState;
    ULONG LdtOffset;
    PLDT_ENTRY CurrentDescriptor;
    PPROCESS_LDT_INFORMATION LdtInfo;
    PLDTINFORMATION ProcessLdtInfo;
    PLDT_ENTRY Ldt;

    PAGED_CODE();

    if ( LdtInformationLength < (ULONG)sizeof( PROCESS_LDT_INFORMATION)) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    Status = STATUS_SUCCESS;
    //
    // alocate a local buffer to capture the ldt information to
    //
    try {
        LdtInfo = ExAllocatePool(
            PagedPool,
            LdtInformationLength
            );
        if (LdtInfo) {
            //
            // Copy the information the user is supplying
            //
            RtlMoveMemory(
                LdtInfo,
                LdtInformation,
                LdtInformationLength
                );
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        if (LdtInfo) {
            ExFreePool(LdtInfo);
        }
        Status = GetExceptionCode();
    }

    //
    // If the capture didn't succeed
    //
    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_ACCESS_VIOLATION) {
            return STATUS_SUCCESS;
        } else {
            return Status;
        }
    }

    if (LdtInfo == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Verify that the Start and Length are plausible
    //
    if (LdtInfo->Start & 0xFFFF0000) {
        return STATUS_INVALID_LDT_OFFSET;
    }

    if (LdtInfo->Length & 0xFFFF0000) {
        return STATUS_INVALID_LDT_SIZE;
    }

    //
    // Insure that the buffer it large enough to contain the specified number
    // of selectors.
    //
    if (LdtInformationLength - sizeof(PROCESS_LDT_INFORMATION) + sizeof(LDT_ENTRY) < LdtInfo->Length) {
        ExFreePool(LdtInfo);
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    //
    // The info to set must be an integral number of selectors
    //
    if (LdtInfo->Length % sizeof(LDT_ENTRY)) {
        ExFreePool(LdtInfo);
        return STATUS_INVALID_LDT_SIZE;
    }

    //
    // The beginning of the info must be on a selector boundary
    //
    if (LdtInfo->Start % sizeof(LDT_ENTRY)) {
        ExFreePool(LdtInfo);
        return STATUS_INVALID_LDT_OFFSET;
    }

    //
    // Verify all of the descriptors.
    //

    for (CurrentDescriptor = LdtInfo->LdtEntries;

        (PCHAR)CurrentDescriptor < (PCHAR)LdtInfo->LdtEntries +
        LdtInfo->Length;

        CurrentDescriptor++
    ) {
        if (!PspIsDescriptorValid( CurrentDescriptor )) {
            ExFreePool(LdtInfo);
            return STATUS_INVALID_LDT_DESCRIPTOR;
        }
    }

    //
    // Acquire the Ldt Mutex
    //

    Status = KeWaitForSingleObject(
                &LdtMutex,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
    if ( !(NT_SUCCESS(Status)) ) {
        return Status;
    }

    ProcessLdtInfo = Process->LdtInformation;

    //
    // If the process doen't have an Ldt information structure, allocate
    //  one and attach it to the process
    //
    if ( ProcessLdtInfo == NULL ) {
        ProcessLdtInfo = ExAllocatePool(
                            PagedPool,
                            sizeof(LDTINFORMATION)
                            );
        if ( ProcessLdtInfo == NULL ) {
            goto SetInfoCleanup;
        }
        Process->LdtInformation = ProcessLdtInfo;
        RtlZeroMemory( ProcessLdtInfo, sizeof(LDTINFORMATION) );
    }

    //
    // If we are supposed to remove the LDT
    //
    if ( LdtInfo->Length == 0 )  {

        //
        // Remove the process' Ldt
        //

        if ( ProcessLdtInfo->Ldt ) {
            OldSize = ProcessLdtInfo->AllocatedSize;
            OldLdt = ProcessLdtInfo->Ldt;

            ProcessLdtInfo->AllocatedSize = 0;
            ProcessLdtInfo->Size = 0;
            ProcessLdtInfo->Ldt = NULL;

            Ke386SetLdtProcess(
                &(Process->Pcb),
                NULL,
                0
                );

            PsReturnPoolQuota( Process, PagedPool, OldSize );
        }


    } else if ( ProcessLdtInfo->Ldt == NULL ) {

        //
        // Create a new Ldt for the process
        //

        //
        // Allocate an integral number of pages for the LDT.
        //

        ASSERT(((PAGE_SIZE % 2) == 0));

        AllocatedSize = (LdtInfo->Start + LdtInfo->Length + PAGE_SIZE - 1) &
            ~(PAGE_SIZE - 1);

        Size = LdtInfo->Start + LdtInfo->Length;

        Ldt = PspCreateLdt(
            LdtInfo->LdtEntries,
            LdtInfo->Start,
            Size,
            AllocatedSize
            );

        if ( Ldt == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto SetInfoCleanup;
        }

        try {
            PsChargePoolQuota(
                Process,
                PagedPool,
                AllocatedSize
                );
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
        }

        if (!NT_SUCCESS(Status)) {
            goto SetInfoCleanup;
        }

        ProcessLdtInfo->Ldt = Ldt;
        ProcessLdtInfo->Size = Size;
        ProcessLdtInfo->AllocatedSize = AllocatedSize;
        Ke386SetLdtProcess(
            &(Process->Pcb),
            ProcessLdtInfo->Ldt,
            ProcessLdtInfo->Size
            );


    } else if ( (LdtInfo->Length + LdtInfo->Start) > ProcessLdtInfo->Size ) {

        //
        // Grow the process' Ldt
        //

        if ( (LdtInfo->Length + LdtInfo->Start) >
            ProcessLdtInfo->AllocatedSize
        ) {

            //
            // Current Ldt allocation is not large enough, so create a
            // new larger Ldt
            //

            OldSize = ProcessLdtInfo->AllocatedSize;

            Size = LdtInfo->Start + LdtInfo->Length;
            AllocatedSize = (Size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

            Ldt = PspCreateLdt(
                ProcessLdtInfo->Ldt,
                0,
                OldSize,
                AllocatedSize
                );

            if ( Ldt == NULL ) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto SetInfoCleanup;
            }

            try {

                PsChargePoolQuota(
                    Process,
                    PagedPool,
                    ProcessLdtInfo->AllocatedSize - OldSize
                    );

            } except(EXCEPTION_EXECUTE_HANDLER) {
                Status = GetExceptionCode();
            }

            if (!NT_SUCCESS(Status)) {
                goto SetInfoCleanup;
            }

            //
            // Swap Ldt information
            //
            OldLdt = ProcessLdtInfo->Ldt;
            ProcessLdtInfo->Ldt = Ldt;
            ProcessLdtInfo->Size = Size;
            ProcessLdtInfo->AllocatedSize = AllocatedSize;

            //
            // Put new selectors into the new ldt
            //
            RtlMoveMemory(
                (PCHAR)(ProcessLdtInfo->Ldt) + LdtInfo->Start,
                LdtInfo->LdtEntries,
                LdtInfo->Length
                );

            Ke386SetLdtProcess(
                &(Process->Pcb),
                ProcessLdtInfo->Ldt,
                ProcessLdtInfo->Size
                );


        } else {

            //
            // Current Ldt allocation is large enough
            //

            ProcessLdtInfo->Size = LdtInfo->Length + LdtInfo->Start;

            Ke386SetLdtProcess(
                &(Process->Pcb),
                ProcessLdtInfo->Ldt,
                ProcessLdtInfo->Size
                );

            //
            // Change the selectors in the table
            //
            for (LdtOffset = LdtInfo->Start,
                CurrentDescriptor = LdtInfo->LdtEntries;

                LdtOffset < LdtInfo->Start + LdtInfo->Length;

                LdtOffset += sizeof(LDT_ENTRY),
                CurrentDescriptor++
            ) {

                Ke386SetDescriptorProcess(
                    &(Process->Pcb),
                    LdtOffset,
                    *CurrentDescriptor
                    );
            }
        }
    } else {

        //
        // Simply changing some selectors
        //

        for (LdtOffset = LdtInfo->Start,
            CurrentDescriptor = LdtInfo->LdtEntries;

            LdtOffset < LdtInfo->Start +  LdtInfo->Length;

            LdtOffset += sizeof(LDT_ENTRY),
            CurrentDescriptor++
        ) {

            Ke386SetDescriptorProcess(
                &(Process->Pcb),
                LdtOffset,
                *CurrentDescriptor
                );
        }
        Status = STATUS_SUCCESS;
    }


SetInfoCleanup:

    MutexState = KeReleaseMutex( &LdtMutex, FALSE );
    ASSERT(( MutexState == 0 ));

    if (OldLdt != NULL) {
        ExFreePool(OldLdt);
    }

    if (LdtInfo != NULL) {
        ExFreePool(LdtInfo);
    }

    return Status;
}

PLDT_ENTRY
PspCreateLdt(
    IN PLDT_ENTRY Ldt,
    IN ULONG Offset,
    IN ULONG Size,
    IN ULONG AllocationSize
    )

/*++

Routine Description:

    This routine allocates space in paged pool for an LDT, and copies the
    specified selectors into it.  IT DOES NOT VALIDATE THE SELECTORS.
    Selector validation must be done before calling this routine.  IT
    DOES NOT CHARGE THE QUOTA FOR THE LDT.

Arguments:

    Ldt -- Supplies a pointer to the descriptors to be put into the Ldt.
    Offset -- Supplies the offset in the LDT to copy the descriptors to.
    Size -- Supplies the actualsize of the new Ldt
    AllocationSize -- Supplies the size to allocate

Return Value:

    Pointer to the new Ldt
--*/
{
    PLDT_ENTRY NewLdt;

    PAGED_CODE();

    ASSERT(( AllocationSize >= Size ));
    ASSERT(( (Size % sizeof(LDT_ENTRY)) == 0 ));

    NewLdt = ExAllocatePool( PagedPool, AllocationSize );
    if (NewLdt == NULL) {
        return NewLdt;
    }


    RtlZeroMemory( NewLdt, AllocationSize );
    RtlMoveMemory( (PCHAR)NewLdt + Offset, Ldt, Size - Offset );

    return NewLdt;
}



BOOLEAN
PspIsDescriptorValid(
    IN PLDT_ENTRY Descriptor
    )

/*++

Routine Description:

    This function determines if the supplied descriptor is valid to put
    into a process Ldt.  For the descriptor to be valid it must have the
    following characteristics:

    Base < MM_HIGHEST_USER_ADDRESS
    Base + Limit < MM_HIGHEST_USER_ADDRESS
    Type must be
        ReadWrite, ReadOnly, ExecuteRead, ExecuteOnly, or Invalid
        big or small
        normal or grow down
        Not a system descriptor (system bit is 1 == application)
            This rules out all gates, etc
        Not conforming
    DPL must be 3

Arguments:

    Descriptor -- Supplies a pointer to the descriptor to check

Return Value:

    True if the descriptor is valid (note: valid to put into an LDT.  This
        includes Invalid descriptors)
    False if not
--*/

{
    ULONG Base;
    ULONG Limit;

    PAGED_CODE();

    //
    // if descriptor is an invalid descriptor
    //

    if ( (Descriptor->HighWord.Bits.Type == 0) &&
        (Descriptor->HighWord.Bits.Dpl == 0) ) {

        return TRUE;
    }

    Base = Descriptor->BaseLow | (Descriptor->HighWord.Bytes.BaseMid << 16) |
        (Descriptor->HighWord.Bytes.BaseHi << 24);

    Limit = Descriptor->LimitLow | (Descriptor->HighWord.Bits.LimitHi << 16);

    //
    // Only have to check for present selectors
    //
    if (Descriptor->HighWord.Bits.Pres) {
        ULONG ActualLimit;

        if ( (PVOID)Base > MM_HIGHEST_USER_ADDRESS ) {
            return FALSE;
        }

        ActualLimit = (Limit << (Descriptor->HighWord.Bits.Granularity *
            12)) + 0xFFF * Descriptor->HighWord.Bits.Granularity;

        if ( (Base > (Base + ActualLimit))
             || ((PVOID)(Base + ActualLimit) > MM_HIGHEST_USER_ADDRESS)
        ) {

            return FALSE;
        }

    }

    //
    // if descriptor is a system descriptor (which includes gates)
    // if bit 4 of the Type field is 0, then it's a system descriptor,
    // and we don't like it.
    //

    if (  ! (Descriptor->HighWord.Bits.Type & 0x10)) {
        return FALSE;
    }

    //
    // if descriptor is conforming code
    //

    if ( ((Descriptor->HighWord.Bits.Type & 0x18) == 0x18) &&
        (Descriptor->HighWord.Bits.Type & 0x4)) {

        return FALSE;
    }

    //
    // if Dpl is not 3
    //

    if ( Descriptor->HighWord.Bits.Dpl != 3 ) {
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
PspQueryDescriptorThread (
    PETHREAD Thread,
    PVOID ThreadInformation,
    ULONG ThreadInformationLength,
    PULONG ReturnLength
    )
/*++

Routine Description:

    This function retrieves a descriptor table entry for the specified thread.
    This entry may be in either the Gdt or the Ldt, as specfied by the
    supplied selector

Arguments:

    Thread -- Supplies a pointer to the thread.
    ThreadInformation -- Supplies information on the descriptor.
    ThreadInformationLength -- Supplies the length of the information.
    ReturnLength -- Returns the number of bytes returned.

Return Value:

    TBS
--*/
{
    DESCRIPTOR_TABLE_ENTRY DescriptorEntry;
    PEPROCESS Process;
    BOOLEAN ReturnNow = FALSE;
    LONG MutexState;
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT( sizeof(KGDTENTRY) == sizeof(LDT_ENTRY) );

    //
    // Verify parameters
    //

    if ( ThreadInformationLength != sizeof(DESCRIPTOR_TABLE_ENTRY) ) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    try {
        DescriptorEntry = *(PDESCRIPTOR_TABLE_ENTRY)ThreadInformation;
    } except(EXCEPTION_EXECUTE_HANDLER){
        ReturnNow = TRUE;
    }

    if (ReturnNow) {
        return STATUS_SUCCESS;
    }

    Status = STATUS_SUCCESS;

    //
    // If its a Gdt entry, let the kernel find it for us
    //

    if ( !(DescriptorEntry.Selector & SELECTOR_TABLE_INDEX) ) {

        if ( (DescriptorEntry.Selector & 0xFFFFFFF8) >= KGDT_NUMBER *
            sizeof(KGDTENTRY) ) {

            return STATUS_ACCESS_VIOLATION;
        }

        try {
            Ke386GetGdtEntryThread( &(Thread->Tcb),
                DescriptorEntry.Selector & 0xFFFFFFF8,
                (PKGDTENTRY)
                    &(((PDESCRIPTOR_TABLE_ENTRY)ThreadInformation)->Descriptor)
                );
            if ( ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = sizeof(LDT_ENTRY);
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            // We want to return STATUS_SUCCESS (see module notes), so
            // do nothing and fall out of if.
        }
    } else {

        //
        // it's an Ldt entry, so copy it from the ldt
        //

        Process = THREAD_TO_PROCESS(Thread);

        //
        // Acquire the Ldt Mutex
        //

        Status = KeWaitForSingleObject(
                    &LdtMutex,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL
                    );
        if ( !(NT_SUCCESS(Status)) ) {
            return Status;
        }

        if ( Process->LdtInformation == NULL ) {

            // If there is no Ldt
            Status = STATUS_NO_LDT;

        } else if ( (DescriptorEntry.Selector & 0xFFFFFFF8) >=
            ((PLDTINFORMATION)(Process->LdtInformation))->Size ) {

            // Else If the selector is outside the table
            Status = STATUS_ACCESS_VIOLATION;

        } else try {

            // Else return the contents of the descriptor
            RtlMoveMemory(
                &(((PDESCRIPTOR_TABLE_ENTRY)ThreadInformation)->Descriptor),
                (PCHAR)(((PLDTINFORMATION)(Process->LdtInformation))->Ldt) +
                    (DescriptorEntry.Selector & 0xFFFFFFF8),
                sizeof(LDT_ENTRY)
                );
            if (ARGUMENT_PRESENT(ReturnLength)) {
                *ReturnLength = sizeof(LDT_ENTRY);
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            // We want to return STATUS_SUCCESS (see module notes), so
            // do nothing and fall out of if.
        }

        MutexState = KeReleaseMutex( &LdtMutex, FALSE );
        ASSERT(( MutexState == 0 ));
    }

    return Status;
}

VOID
PspDeleteLdt(
    IN PEPROCESS Process
    )
/*++

Routine Description:

    This routine frees the paged pool associated with a process' Ldt, if
    it has one.

Arguments:

    Process -- Supplies a pointer to the process

Return Value:

    None
--*/
{
    PLDTINFORMATION LdtInformation;

    PAGED_CODE();

    LdtInformation = Process->LdtInformation;
    if ( LdtInformation != NULL ) {
        if ( LdtInformation->Ldt != NULL ) {
            ExFreePool( LdtInformation->Ldt );
        }
        ExFreePool( LdtInformation );
    }
}

NTSTATUS
NtSetLdtEntries(
    IN ULONG Selector0,
    IN ULONG Entry0Low,
    IN ULONG Entry0Hi,
    IN ULONG Selector1,
    IN ULONG Entry1Low,
    IN ULONG Entry1Hi
    )
/*++

Routine Description:

    This routine sets up to two selectors in the current process's LDT.
    The LDT will be grown as necessary.  A selector value of 0 indicates
    that the specified selector was not passed (allowing the setting of
    a single selector).

Arguments:

    Selector0 -- Supplies the number of the first descriptor to set
    Entry0Low -- Supplies the low 32 bits of the descriptor
    Entry0Hi -- Supplies the high 32 bits of the descriptor
    Selector1 -- Supplies the number of the first descriptor to set
    Entry1Low -- Supplies the low 32 bits of the descriptor
    Entry1Hi -- Supplies the high 32 bits of the descriptor

Return Value:

    TBS
--*/

{
    ULONG Base, Limit, LdtSize, AllocatedSize;
    NTSTATUS Status;
    PEPROCESS Process;
    LDT_ENTRY Descriptor;
    PLDT_ENTRY Ldt, OldLdt;
    PLDTINFORMATION ProcessLdtInformation;
    LONG MutexState;

    PAGED_CODE();

    //
    // Verify the selectors.  We do not allow selectors that point into
    // Kernel space, system selectors, or conforming code selectors
    //

    //
    // Verify the selectors
    //
    if ((Selector0 & 0xFFFF0000) || (Selector1 & 0xFFFF0000)) {
        return STATUS_INVALID_LDT_DESCRIPTOR;
    }

    // Change the selector values to indexes into the LDT

    Selector0 = Selector0 & ~(RPL_MASK | SELECTOR_TABLE_INDEX);
    Selector1 = Selector1 & ~(RPL_MASK | SELECTOR_TABLE_INDEX);


    //
    // Verify descriptor 0
    //

    if (Selector0) {

        // Form the base and the limit
        Base = ((Entry0Low & 0xFFFF0000) >> 16) + ((Entry0Hi & 0xFF) << 16)
            + (Entry0Hi & 0xFF000000);

        Limit = (Entry0Low & 0xFFFF) + (Entry0Hi & 0xF0000);

        // N.B.  the interpretation of the limit depends on the G bit
        // in the descriptor

        if (Entry0Hi & DESCRIPTOR_GRAN) {
            Limit = (Limit << 12) | 0xFFF;
        }

        //
        // Base and limit don't matter for NP descriptors, so only check
        // for present descriptors
        //
        if (Entry0Hi & DESCRIPTOR_NP) {

            // Check descriptor base and limit
            if (((PVOID)Base > MM_HIGHEST_USER_ADDRESS)
                || ((PVOID)Base > (PVOID) (Base + Limit))
                || ((PVOID)(Base + Limit) > MM_HIGHEST_USER_ADDRESS))
            {
                return STATUS_INVALID_LDT_DESCRIPTOR;
            }

        }
        //
        // If type and DPL are 0, this is an invalid descriptor, otherwise,
        // we have to check the type (invalid from the standpoint of a
        // descriptor created to generate a GP fault)
        //

        if (Entry0Hi & DESCRIPTOR_TYPEDPL) {

            // No system descriptors (system descriptors have system bit = 0)
            if (!(Entry0Hi & DESCRIPTOR_SYSTEM)) {
                return STATUS_INVALID_LDT_DESCRIPTOR;
            }

            // No conforming code
            if ((Entry0Hi & DESCRIPTOR_CONFORM) == DESCRIPTOR_CONFORM) {
                return STATUS_INVALID_LDT_DESCRIPTOR;
            }

            // Dpl must be 3
            if ((Entry0Hi & DESCRIPTOR_DPL) != DESCRIPTOR_DPL) {
                return STATUS_INVALID_LDT_DESCRIPTOR;
            }
        }
    }

    //
    // Verify descriptor 1
    //

    if (Selector1) {

        // Form the base and the limit
        Base = ((Entry1Low & 0xFFFF0000) >> 16) + ((Entry1Hi & 0xFF) << 16)
            + (Entry1Hi & 0xFF000000);

        Limit = (Entry1Low & 0xFFFF) + (Entry1Hi & 0xF0000);

        // N.B.  the interpretation of the limit depends on the G bit
        // in the descriptor

        if (Entry1Hi & DESCRIPTOR_GRAN) {
            Limit = (Limit << 12) | 0xFFF;
        }

        //
        // Base and limit don't matter for NP descriptor, so only check
        // for present descriptors
        //
        if (Entry1Hi & DESCRIPTOR_NP) {

            // Check descriptor base and limit
            if (((PVOID)Base > MM_HIGHEST_USER_ADDRESS)
                || ((PVOID)Base > (PVOID) (Base + Limit))
                || ((PVOID)(Base + Limit) > MM_HIGHEST_USER_ADDRESS))
            {
                return STATUS_INVALID_LDT_DESCRIPTOR;
            }
        }
        //
        // If type and DPL are 0, this is an invalid descriptor, otherwise,
        // we have to check the type (invalid from the standpoint of a
        // descriptor created to generate a GP fault)
        //

        if (Entry1Hi & DESCRIPTOR_TYPEDPL) {

            // No system descriptors (system descriptors have system bit = 0)
            if (!(Entry1Hi & DESCRIPTOR_SYSTEM)) {
                return STATUS_INVALID_LDT_DESCRIPTOR;
            }

            // No conforming code
            if ((Entry1Hi & DESCRIPTOR_CONFORM) == DESCRIPTOR_CONFORM) {
                return STATUS_INVALID_LDT_DESCRIPTOR;
            }

            // Dpl must be 3
            if ((Entry1Hi & DESCRIPTOR_DPL) != DESCRIPTOR_DPL) {
                return STATUS_INVALID_LDT_DESCRIPTOR;
            }
        }
    }

    //
    // Acquire the LDT mutex.
    //

    Status = KeWaitForSingleObject(
                &LdtMutex,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return Status;
    }

    //
    // Figure out how large the LDT needs to be
    //

    if (Selector0 > Selector1) {
        LdtSize = Selector0 + sizeof(LDT_ENTRY);
    } else {
        LdtSize = Selector1 + sizeof(LDT_ENTRY);
    }

    Process = PsGetCurrentProcess();
    ProcessLdtInformation = Process->LdtInformation;

    //
    // Most of the time, the process will already have an LDT, and it
    // will be large enough.  for this, we just set the descriptors and
    // return
    //

    if (ProcessLdtInformation) {

        //
        // If the LDT descriptor does not have to be modified.
        //
        if (ProcessLdtInformation->Size >= LdtSize) {
            if (Selector0) {

                *((PULONG)(&Descriptor)) = Entry0Low;
                *(((PULONG)(&Descriptor)) + 1) = Entry0Hi;

                Ke386SetDescriptorProcess(
                    &(Process->Pcb),
                    Selector0,
                    Descriptor
                    );
            }

            if (Selector1) {

                *((PULONG)(&Descriptor)) = Entry1Low;
                *(((PULONG)(&Descriptor)) + 1) = Entry1Hi;

                Ke386SetDescriptorProcess(
                    &(Process->Pcb),
                    Selector1,
                    Descriptor
                    );
            }

            MutexState = KeReleaseMutex( &LdtMutex, FALSE );
            ASSERT(( MutexState == 0 ));
            return STATUS_SUCCESS;

        //
        // Else if the Ldt will fit in the memory currently allocated
        //
        } else if (ProcessLdtInformation->AllocatedSize >= LdtSize) {

            //
            // First remove the LDT.  This will allow us to edit the memory.
            // We will then put the LDT back.  Since we have to change the
            // limit anyway, it would take two calls to the kernel ldt
            // management minimum to set the descriptors.  Each of those calls
            // would stall all of the processors in an MP system.  If we
            // didn't remove the ldt first, and we were setting two descriptors,
            // we would have to call the LDT management 3 times (once per
            // descriptor, and once to change the limit of the LDT).
            //

            Ke386SetLdtProcess(
                &(Process->Pcb),
                NULL,
                0L
                );

            //
            // Set the Descriptors in the LDT
            //
            if (Selector0) {
                *((PULONG)(&(ProcessLdtInformation->Ldt[Selector0/sizeof(LDT_ENTRY)]))) = Entry0Low;
                *((PULONG)(&(ProcessLdtInformation->Ldt[Selector0/sizeof(LDT_ENTRY)])) + 1) =
                    Entry0Hi;
            }

            if (Selector1) {
                *((PULONG)(&(ProcessLdtInformation->Ldt[Selector1/sizeof(LDT_ENTRY)]))) = Entry1Low;
                *((PULONG)(&(ProcessLdtInformation->Ldt[Selector1/sizeof(LDT_ENTRY)])) + 1) =
                    Entry1Hi;
            }

            //
            // Set the LDT for the process
            //

            ProcessLdtInformation->Size = LdtSize;

            Ke386SetLdtProcess(
                &(Process->Pcb),
                ProcessLdtInformation->Ldt,
                ProcessLdtInformation->Size
                );

            MutexState = KeReleaseMutex( &LdtMutex, FALSE );
            ASSERT(( MutexState == 0 ));
            return STATUS_SUCCESS;
        //
        // Otherwise, we have to grow the LDT allocation
        //
        }
    }

    //
    // If the process does not yet have an LDT information structure,
    // allocate and attach one.
    //

    OldLdt = NULL;

    if (!Process->LdtInformation) {
        ProcessLdtInformation = ExAllocatePool(
            PagedPool,
            sizeof(LDTINFORMATION)
            );
        if (ProcessLdtInformation == NULL) {
            goto SetLdtEntriesCleanup;
        }
        Process->LdtInformation = ProcessLdtInformation;
        ProcessLdtInformation->Size = 0L;
        ProcessLdtInformation->AllocatedSize = 0L;
        ProcessLdtInformation->Ldt = NULL;
    }

    //
    // Now, we either need to create or grow an LDT, so allocate some
    // memory, and copy as necessary
    //

    AllocatedSize = (LdtSize + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    Ldt = ExAllocatePool(
        PagedPool,
        AllocatedSize
        );

    if (Ldt) {
        RtlZeroMemory(
            Ldt,
            AllocatedSize
            );
    } else {
        goto SetLdtEntriesCleanup;
    }


    if (ProcessLdtInformation->Ldt) {
        //
        // copy the contents of the old ldt
        //
        RtlMoveMemory(
            Ldt,
            ProcessLdtInformation->Ldt,
            ProcessLdtInformation->Size
            );

        try {
            PsChargePoolQuota(
                Process,
                PagedPool,
                AllocatedSize - ProcessLdtInformation->AllocatedSize
                );
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
            ExFreePool(Ldt);
            Ldt = NULL;
        }

        if (Ldt == NULL) {
            goto SetLdtEntriesCleanup;
        }

    } else {
        try {
            PsChargePoolQuota(
                Process,
                PagedPool,
                AllocatedSize
                );
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
            ExFreePool(Ldt);
            Ldt = NULL;
        }

        if (Ldt == NULL) {
            goto SetLdtEntriesCleanup;
        }
    }

    OldLdt = ProcessLdtInformation->Ldt;
    ProcessLdtInformation->Size = LdtSize;
    ProcessLdtInformation->AllocatedSize = AllocatedSize;
    ProcessLdtInformation->Ldt = Ldt;

    //
    // Set the descriptors in the LDT
    //

    if (Selector0) {
        *((PULONG)(&(ProcessLdtInformation->Ldt[Selector0/sizeof(LDT_ENTRY)]))) = Entry0Low;
        *((PULONG)(&(ProcessLdtInformation->Ldt[Selector0/sizeof(LDT_ENTRY)])) + 1) =
            Entry0Hi;
    }

    if (Selector1) {
        *((PULONG)(&(ProcessLdtInformation->Ldt[Selector1/sizeof(LDT_ENTRY)]))) = Entry1Low;
        *((PULONG)(&(ProcessLdtInformation->Ldt[Selector1/sizeof(LDT_ENTRY)])) + 1) =
            Entry1Hi;
    }

    //
    // Set the LDT for the process
    //

    Ke386SetLdtProcess(
        &(Process->Pcb),
        ProcessLdtInformation->Ldt,
        ProcessLdtInformation->Size
        );

    //
    // Cleanup and exit
    //

    Status = STATUS_SUCCESS;

SetLdtEntriesCleanup:

    if (OldLdt) {
        ExFreePool(OldLdt);
    }

    MutexState = KeReleaseMutex( &LdtMutex, FALSE );
    ASSERT(( MutexState == 0 ));
    return Status;

}
