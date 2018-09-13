
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pnpbusno.c

Abstract:

    Root Bus Number arbiter

Author:

    Andy Thornton (andrewth) 04/17/97

Revision History:

--*/

#include "iop.h"
#pragma hdrstop

//
// Constants
//


#define MAX_ULONGLONG           ((ULONGLONG) -1)

//
// Prototypes
//

NTSTATUS
IopBusNumberInitialize(
    VOID
    );

NTSTATUS
IopBusNumberUnpackRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Minimum,
    OUT PULONGLONG Maximum,
    OUT PULONG Length,
    OUT PULONG Alignment
    );

NTSTATUS
IopBusNumberPackResource(
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    IN ULONGLONG Start,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

LONG
IopBusNumberScoreRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    );

NTSTATUS
IopBusNumberUnpackResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    );


//
// Make everything pageable
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, IopBusNumberInitialize)
#pragma alloc_text(PAGE, IopBusNumberUnpackRequirement)
#pragma alloc_text(PAGE, IopBusNumberPackResource)
#pragma alloc_text(PAGE, IopBusNumberScoreRequirement)
#pragma alloc_text(PAGE, IopBusNumberUnpackResource)

#endif // ALLOC_PRAGMA

//
// Implementation
//

NTSTATUS
IopBusNumberInitialize(
    VOID
    )

/*++

Routine Description:

    This routine initializes the arbiter

Parameters:

    None

Return Value:

    None

--*/

{
    NTSTATUS    status;

    IopRootBusNumberArbiter.UnpackRequirement = IopBusNumberUnpackRequirement;
    IopRootBusNumberArbiter.PackResource = IopBusNumberPackResource;
    IopRootBusNumberArbiter.UnpackResource = IopBusNumberUnpackResource;
    IopRootBusNumberArbiter.ScoreRequirement = IopBusNumberScoreRequirement;

    status = ArbInitializeArbiterInstance(&IopRootBusNumberArbiter,
                                          NULL,  // Indicates a root arbiter
                                          CmResourceTypeBusNumber,
                                          L"RootBusNumber",
                                          L"Root",
                                          NULL    // no translation of BusNumber
                                          );
    if (NT_SUCCESS(status)) {

        //
        // Add the invalid range 100 - ffffffff ffffffff
        //
        RtlAddRange( IopRootBusNumberArbiter.Allocation,
                     (ULONGLONG) 0x100,
                     (ULONGLONG) -1,
                     0, // UserFlags
                     0, // Flag
                     NULL,
                     NULL
                   );

    }

    return status;
}

//
// Arbiter callbacks
//

NTSTATUS
IopBusNumberUnpackRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Minimum,
    OUT PULONGLONG Maximum,
    OUT PULONG Length,
    OUT PULONG Alignment
    )

/*++

Routine Description:

    This routine unpacks an resource requirement descriptor.

Arguments:

    Descriptor - The descriptor describing the requirement to unpack.

    Minimum - Pointer to where the minimum acceptable start value should be
        unpacked to.

    Maximum - Pointer to where the maximum acceptable end value should be
        unpacked to.

    Length - Pointer to where the required length should be unpacked to.

    Minimum - Pointer to where the required alignment should be unpacked to.

Return Value:

    Returns the status of this operation.

--*/

{
    ASSERT(Descriptor);
    ASSERT(Descriptor->Type == CmResourceTypeBusNumber);

    ARB_PRINT(2,
                ("Unpacking BusNumber requirement %p => 0x%I64x-0x%I64x\n",
                Descriptor,
                (ULONGLONG) Descriptor->u.BusNumber.MinBusNumber,
                (ULONGLONG) Descriptor->u.BusNumber.MaxBusNumber
                ));

    *Minimum = (ULONGLONG) Descriptor->u.BusNumber.MinBusNumber;
    *Maximum = (ULONGLONG) Descriptor->u.BusNumber.MaxBusNumber;
    *Length = Descriptor->u.BusNumber.Length;
    *Alignment = 1;

    return STATUS_SUCCESS;

}

LONG
IopBusNumberScoreRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    )

/*++

Routine Description:

    This routine scores a requirement based on how flexible it is.  The least
    flexible devices are scored the least and so when the arbitration list is
    sorted we try to allocate their resources first.

Arguments:

    Descriptor - The descriptor describing the requirement to score.


Return Value:

    The score.

--*/

{
    LONG score;

    ASSERT(Descriptor);
    ASSERT(Descriptor->Type == CmResourceTypeBusNumber);

    score = (Descriptor->u.BusNumber.MaxBusNumber -
                Descriptor->u.BusNumber.MinBusNumber) /
                Descriptor->u.BusNumber.Length;

    ARB_PRINT(2,
                ("Scoring BusNumber resource %p => %i\n",
                Descriptor,
                score
                ));

    return score;
}

NTSTATUS
IopBusNumberPackResource(
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    IN ULONGLONG Start,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    )

/*++

Routine Description:

    This routine packs an resource descriptor.

Arguments:

    Requirement - The requirement from which this resource was chosen.

    Start - The start value of the resource.

    Descriptor - Pointer to the descriptor to pack into.

Return Value:

    Returns the status of this operation.

--*/

{
    ASSERT(Descriptor);
    ASSERT(Start < ((ULONG)-1));
    ASSERT(Requirement);
    ASSERT(Requirement->Type == CmResourceTypeBusNumber);

    ARB_PRINT(2,
                ("Packing BusNumber resource %p => 0x%I64x\n",
                Descriptor,
                Start
                ));

    Descriptor->Type = CmResourceTypeBusNumber;
    Descriptor->ShareDisposition = Requirement->ShareDisposition;
    Descriptor->Flags = Requirement->Flags;
    Descriptor->u.BusNumber.Start = (ULONG) Start;
    Descriptor->u.BusNumber.Length = Requirement->u.BusNumber.Length;

    return STATUS_SUCCESS;
}

NTSTATUS
IopBusNumberUnpackResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine unpacks an resource descriptor.

Arguments:

    Descriptor - The descriptor describing the resource to unpack.

    Start - Pointer to where the start value should be unpacked to.

    End - Pointer to where the end value should be unpacked to.

Return Value:

    Returns the status of this operation.

--*/

{
    ASSERT(Descriptor);
    ASSERT(Start);
    ASSERT(Length);
    ASSERT(Descriptor->Type == CmResourceTypeBusNumber);

    *Start = (ULONGLONG) Descriptor->u.BusNumber.Start;
    *Length = Descriptor->u.BusNumber.Length;

    ARB_PRINT(2,
                ("Unpacking BusNumber resource %p => 0x%I64x\n",
                Descriptor,
                *Start
                ));

    return STATUS_SUCCESS;

}

