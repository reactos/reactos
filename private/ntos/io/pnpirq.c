/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pnpirq.c

Abstract:

    Root IRQ arbiter

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
IopIrqInitialize(
    VOID
    );

NTSTATUS
IopIrqUnpackRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Minimum,
    OUT PULONGLONG Maximum,
    OUT PULONG Length,
    OUT PULONG Alignment
    );

NTSTATUS
IopIrqPackResource(
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    IN ULONGLONG Start,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

LONG
IopIrqScoreRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    );

NTSTATUS
IopIrqUnpackResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    );

NTSTATUS
IopIrqTranslateOrdering(
    OUT PIO_RESOURCE_DESCRIPTOR Target,
    IN PIO_RESOURCE_DESCRIPTOR Source
    );

BOOLEAN
IopIrqFindSuitableRange(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    );


//
// Make everything pageable
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, IopIrqInitialize)
#pragma alloc_text(PAGE, IopIrqUnpackRequirement)
#pragma alloc_text(PAGE, IopIrqPackResource)
#pragma alloc_text(PAGE, IopIrqScoreRequirement)
#pragma alloc_text(PAGE, IopIrqUnpackResource)
#pragma alloc_text(PAGE, IopIrqTranslateOrdering)
#pragma alloc_text(PAGE, IopIrqFindSuitableRange)
#endif // ALLOC_PRAGMA

//
// Implementation
//
#if !defined(NO_LEGACY_DRIVERS)
NTSTATUS
IopIrqTranslateOrdering(
    OUT PIO_RESOURCE_DESCRIPTOR Target,
    IN PIO_RESOURCE_DESCRIPTOR Source
    )

/*

Routine Description:

    This routine is called during arbiter initialization to translate the
    orderings.

Parameters:

    Target - Place to put the translated descriptor

    Source - Descriptor to translate

Return Value:

    Status code

*/

{

    KIRQL level;
    KAFFINITY affinity;

    PAGED_CODE();

    //
    // Copy the source to the target
    //

    *Target = *Source;

    if (Source->Type != CmResourceTypeInterrupt) {
        return STATUS_SUCCESS;
    }

    //
    // Translate the vector
    //


    ARB_PRINT(
        2,
        ("Translating Vector 0x%x-0x%x =>",
        Source->u.Interrupt.MinimumVector,
        Source->u.Interrupt.MaximumVector
        ));

    Target->u.Interrupt.MinimumVector =
        HalGetInterruptVector(Isa,
                              0,
                              Source->u.Interrupt.MinimumVector,
                              Source->u.Interrupt.MinimumVector,
                              &level,
                              &affinity
                              );

    if (affinity == 0) {
        ARB_PRINT(2,("Translation failed\n"));
        *Target = *Source;
        return STATUS_SUCCESS;
    }

    Target->u.Interrupt.MaximumVector =
        HalGetInterruptVector(Isa,
                              0,
                              Source->u.Interrupt.MaximumVector,
                              Source->u.Interrupt.MaximumVector,
                              &level,
                              &affinity
                              );

    if (affinity == 0) {
        ARB_PRINT(2,("Translation failed\n"));
        *Target = *Source;
        return STATUS_SUCCESS;
    }

    ARB_PRINT(
        2,
        ("0x%x-0x%x\n",
        Target->u.Interrupt.MinimumVector,
        Target->u.Interrupt.MaximumVector
        ));


    return STATUS_SUCCESS;
}
#endif // NO_LEGACY_DRIVERS

NTSTATUS
IopIrqInitialize(
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

    IopRootIrqArbiter.UnpackRequirement = IopIrqUnpackRequirement;
    IopRootIrqArbiter.PackResource      = IopIrqPackResource;
    IopRootIrqArbiter.UnpackResource    = IopIrqUnpackResource;
    IopRootIrqArbiter.ScoreRequirement  = IopIrqScoreRequirement;

    return ArbInitializeArbiterInstance(&IopRootIrqArbiter,
                                        NULL,     // Indicates ROOT arbiter
                                        CmResourceTypeInterrupt,
                                        L"RootIRQ",
                                        L"Root",
#if defined(NO_LEGACY_DRIVERS)
                                        NULL
#else
                                        IopIrqTranslateOrdering
#endif // NO_LEGACY_DRIVERS
                                        );
}

//
// Arbiter callbacks
//

NTSTATUS
IopIrqUnpackRequirement(
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
    ASSERT(Descriptor->Type == CmResourceTypeInterrupt);

    ARB_PRINT(2,
                ("Unpacking IRQ requirement %p => 0x%I64x-0x%I64x\n",
                Descriptor,
                (ULONGLONG) Descriptor->u.Interrupt.MinimumVector,
                (ULONGLONG) Descriptor->u.Interrupt.MaximumVector
                ));

    *Minimum = (ULONGLONG) Descriptor->u.Interrupt.MinimumVector;
    *Maximum = (ULONGLONG) Descriptor->u.Interrupt.MaximumVector;
    *Length = 1;
    *Alignment = 1;

    return STATUS_SUCCESS;

}

LONG
IopIrqScoreRequirement(
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
    ASSERT(Descriptor->Type == CmResourceTypeInterrupt);

    score = Descriptor->u.Interrupt.MaximumVector -
        Descriptor->u.Interrupt.MinimumVector + 1;

    ARB_PRINT(2,
                ("Scoring IRQ resource %p => %i\n",
                Descriptor,
                score
                ));

    return score;
}

NTSTATUS
IopIrqPackResource(
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
    ASSERT(Requirement->Type == CmResourceTypeInterrupt);

    ARB_PRINT(2,
                ("Packing IRQ resource %p => 0x%I64x\n",
                Descriptor,
                Start
                ));

    Descriptor->Type = CmResourceTypeInterrupt;
    Descriptor->Flags = Requirement->Flags; // BUGBUG - is this correct?
    Descriptor->ShareDisposition = Requirement->ShareDisposition;
    Descriptor->u.Interrupt.Vector = (ULONG) Start;
    Descriptor->u.Interrupt.Level = (ULONG) Start;
    Descriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

    return STATUS_SUCCESS;
}

NTSTATUS
IopIrqUnpackResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    )

/*++

Routine Description:

    This routine unpacks an resource descriptor.

Arguments:

    Descriptor - The descriptor describing the requirement to unpack.

    Start - Pointer to where the start value should be unpacked to.

    End - Pointer to where the end value should be unpacked to.

Return Value:

    Returns the status of this operation.

--*/


{

    ASSERT(Descriptor);
    ASSERT(Descriptor->Type == CmResourceTypeInterrupt);

    *Start = Descriptor->u.Interrupt.Vector;
    *Length = 1;

    ARB_PRINT(2,
                ("Unpacking IRQ resource %p => 0x%I64x\n",
                Descriptor,
                *Start
                ));

    return STATUS_SUCCESS;

}
