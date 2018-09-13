/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    exatom.c

Abstract:

    This file contains functions for manipulating global atom tables
    stored in kernel space.

Author:

    Steve Wood (stevewo) 13-Dec-1995

Revision History:

--*/

#include "exp.h"
#pragma hdrstop

//
//  Local Procedure prototype
//

PVOID
ExpGetGlobalAtomTable (
    );

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, NtAddAtom)
#pragma alloc_text(PAGE, NtFindAtom)
#pragma alloc_text(PAGE, NtDeleteAtom)
#pragma alloc_text(PAGE, NtQueryInformationAtom)
#pragma alloc_text(PAGE, ExpGetGlobalAtomTable)
#endif


NTSYSAPI
NTSTATUS
NTAPI
NtAddAtom (
    IN PWSTR AtomName,
    IN ULONG Length,
    OUT PRTL_ATOM Atom OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS Status;
    RTL_ATOM ReturnAtom;
    PVOID AtomTable = ExpGetGlobalAtomTable();
    KPROCESSOR_MODE PreviousMode;
    PWSTR CapturedAtomNameBuffer;
    ULONG AllocLength;

    PAGED_CODE();

    if (AtomTable == NULL) {

        return STATUS_ACCESS_DENIED;
    }

    if (Length > (RTL_ATOM_MAXIMUM_NAME_LENGTH * sizeof(WCHAR))) {

        return STATUS_INVALID_PARAMETER;
    }

    PreviousMode = KeGetPreviousMode();
    CapturedAtomNameBuffer = NULL;

    Status = STATUS_SUCCESS;

    if (PreviousMode != KernelMode) {

        try {

            if (ARGUMENT_PRESENT( AtomName )) {

                AllocLength = (Length + sizeof( UNICODE_NULL ))&~(sizeof (WCHAR)-1);
                ProbeForRead( AtomName, Length, sizeof( WCHAR ) );
                CapturedAtomNameBuffer = ExAllocatePoolWithTag( PagedPool, AllocLength, 'motA' );

                if (CapturedAtomNameBuffer == NULL) {

                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlMoveMemory( CapturedAtomNameBuffer, AtomName, Length );
                CapturedAtomNameBuffer[Length / sizeof (WCHAR)] = '\0';
            }

            if (ARGUMENT_PRESENT( Atom )) {

                ProbeForWriteUshort( Atom );
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();
        }

    } else {

        if (ARGUMENT_PRESENT( AtomName )) {

            CapturedAtomNameBuffer = AtomName;
        }
    }

    if (NT_SUCCESS( Status )) {

        Status = RtlAddAtomToAtomTable( AtomTable, CapturedAtomNameBuffer, &ReturnAtom );

        if (NT_SUCCESS( Status ) && ARGUMENT_PRESENT( Atom )) {

            try {

                *Atom = ReturnAtom;

            } except (EXCEPTION_EXECUTE_HANDLER) {

                Status = GetExceptionCode();
            }
        }
    }

    if ((CapturedAtomNameBuffer != NULL) && (CapturedAtomNameBuffer != AtomName)) {

        ExFreePool( CapturedAtomNameBuffer );
    }

    return Status;
}


NTSYSAPI
NTSTATUS
NTAPI
NtFindAtom (
    IN PWSTR AtomName,
    IN ULONG Length,
    OUT PRTL_ATOM Atom OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS Status;
    RTL_ATOM ReturnAtom;
    PVOID AtomTable = ExpGetGlobalAtomTable();
    KPROCESSOR_MODE PreviousMode;
    PWSTR CapturedAtomNameBuffer;
    ULONG AllocLength;

    PAGED_CODE();

    if (AtomTable == NULL) {

        return STATUS_ACCESS_DENIED;
    }

    if (Length > (RTL_ATOM_MAXIMUM_NAME_LENGTH * sizeof(WCHAR))) {

        return STATUS_INVALID_PARAMETER;
    }

    PreviousMode = KeGetPreviousMode();
    CapturedAtomNameBuffer = NULL;

    Status = STATUS_SUCCESS;

    if (PreviousMode != KernelMode) {

        try {

            if (ARGUMENT_PRESENT( AtomName )) {

                AllocLength = (Length + sizeof( UNICODE_NULL ))&~(sizeof (WCHAR)-1);
                ProbeForRead( AtomName, Length, sizeof( WCHAR ) );
                CapturedAtomNameBuffer = ExAllocatePoolWithTag( PagedPool, AllocLength, 'motA' );

                if (CapturedAtomNameBuffer == NULL) {

                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlMoveMemory( CapturedAtomNameBuffer, AtomName, Length );
                CapturedAtomNameBuffer[Length / sizeof (WCHAR)] = '\0';

            }

            if (ARGUMENT_PRESENT( Atom )) {

                ProbeForWriteUshort( Atom );
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();
        }

    } else {

        if (ARGUMENT_PRESENT( AtomName )) {

            CapturedAtomNameBuffer = AtomName;
        }
    }

    if (NT_SUCCESS( Status )) {

        Status = RtlLookupAtomInAtomTable( AtomTable, CapturedAtomNameBuffer, &ReturnAtom );

        if (NT_SUCCESS( Status ) && ARGUMENT_PRESENT( Atom )) {

            try {

                *Atom = ReturnAtom;

            } except (EXCEPTION_EXECUTE_HANDLER) {

                Status = GetExceptionCode();
            }
        }
    }

    if (CapturedAtomNameBuffer != NULL && CapturedAtomNameBuffer != AtomName) {

        ExFreePool( CapturedAtomNameBuffer );
    }

    return Status;
}


NTSYSAPI
NTSTATUS
NTAPI
NtDeleteAtom (
    IN RTL_ATOM Atom
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS Status;
    PVOID AtomTable = ExpGetGlobalAtomTable();

    PAGED_CODE();

    if (AtomTable == NULL) {

        return STATUS_ACCESS_DENIED;
    }

    Status = RtlDeleteAtomFromAtomTable( AtomTable, Atom );

    return Status;
}


NTSYSAPI
NTSTATUS
NTAPI
NtQueryInformationAtom(
    IN RTL_ATOM Atom,
    IN ATOM_INFORMATION_CLASS AtomInformationClass,
    OUT PVOID AtomInformation,
    IN ULONG AtomInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    ULONG RequiredLength;
    ULONG UsageCount;
    ULONG NameLength;
    ULONG AtomFlags;
    PATOM_BASIC_INFORMATION BasicInfo;
    PATOM_TABLE_INFORMATION TableInfo;
    PVOID AtomTable = ExpGetGlobalAtomTable();

    PAGED_CODE();

    if (AtomTable == NULL) {

        return STATUS_ACCESS_DENIED;
    }

    //
    //  Assume successful completion.
    //

    Status = STATUS_SUCCESS;

    try {

        //
        //  Get previous processor mode and probe output argument if necessary.
        //

        PreviousMode = KeGetPreviousMode();

        if (PreviousMode != KernelMode) {

            ProbeForWrite( AtomInformation,
                           AtomInformationLength,
                           sizeof( ULONG ));

            if (ARGUMENT_PRESENT( ReturnLength )) {

                ProbeForWriteUlong( ReturnLength );
            }
        }

        RequiredLength = 0;

        switch (AtomInformationClass) {

        case AtomBasicInformation:

            RequiredLength = FIELD_OFFSET( ATOM_BASIC_INFORMATION, Name );

            if (AtomInformationLength < RequiredLength) {

                return STATUS_INFO_LENGTH_MISMATCH;
            }

            BasicInfo = (PATOM_BASIC_INFORMATION)AtomInformation;
            UsageCount = 0;
            NameLength = AtomInformationLength - RequiredLength;
            BasicInfo->Name[ 0 ] = UNICODE_NULL;

            Status = RtlQueryAtomInAtomTable( AtomTable,
                                              Atom,
                                              &UsageCount,
                                              &AtomFlags,
                                              &BasicInfo->Name[0],
                                              &NameLength );

            if (NT_SUCCESS(Status)) {

                BasicInfo->UsageCount = (USHORT)UsageCount;
                BasicInfo->Flags = (USHORT)AtomFlags;
                BasicInfo->NameLength = (USHORT)NameLength;
                RequiredLength += NameLength + sizeof( UNICODE_NULL );
            }

            break;

        case AtomTableInformation:

            RequiredLength = FIELD_OFFSET( ATOM_TABLE_INFORMATION, Atoms );

            if (AtomInformationLength < RequiredLength) {

                return STATUS_INFO_LENGTH_MISMATCH;
            }

            TableInfo = (PATOM_TABLE_INFORMATION)AtomInformation;

            Status = RtlQueryAtomsInAtomTable( AtomTable,
                                               (AtomInformationLength - RequiredLength) / sizeof( RTL_ATOM ),
                                               &TableInfo->NumberOfAtoms,
                                               &TableInfo->Atoms[0] );

            if (NT_SUCCESS(Status)) {

                RequiredLength += TableInfo->NumberOfAtoms * sizeof( RTL_ATOM );
            }

            break;

        default:

            Status = STATUS_INVALID_INFO_CLASS;

            break;
        }

        if (ARGUMENT_PRESENT( ReturnLength )) {

            *ReturnLength = RequiredLength;
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();
    }

    return Status;
}


//
//  Local support routine
//

PKWIN32_GLOBALATOMTABLE_CALLOUT ExGlobalAtomTableCallout;

PVOID
ExpGetGlobalAtomTable (
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    if (ExGlobalAtomTableCallout != NULL) {

        return ((*ExGlobalAtomTableCallout)());

    } else {

#if DBG
        DbgPrint( "EX: ExpGetGlobalAtomTable is about to return NULL!\n" );
        DbgBreakPoint();
#endif
        return NULL;
    }
}
