
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pnpirq.c

Abstract:

    Root DMA arbiter

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
IopDmaInitialize(
    VOID
    );

NTSTATUS
IopDmaUnpackRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Minimum,
    OUT PULONGLONG Maximum,
    OUT PULONG Length,
    OUT PULONG Alignment
    );

NTSTATUS
IopDmaPackResource(
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    IN ULONGLONG Start,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

LONG
IopDmaScoreRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    );

NTSTATUS
IopDmaUnpackResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    );


BOOLEAN
IopDmaOverrideConflict(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

//
// Make everything pageable
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, IopDmaInitialize)
#pragma alloc_text(PAGE, IopDmaUnpackRequirement)
#pragma alloc_text(PAGE, IopDmaPackResource)
#pragma alloc_text(PAGE, IopDmaScoreRequirement)
#pragma alloc_text(PAGE, IopDmaUnpackResource)
#pragma alloc_text(PAGE, IopDmaOverrideConflict)
#endif // ALLOC_PRAGMA

//
// Implementation
//

NTSTATUS
IopDmaInitialize(
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
    
    IopRootDmaArbiter.UnpackRequirement = IopDmaUnpackRequirement;
    IopRootDmaArbiter.PackResource = IopDmaPackResource;
    IopRootDmaArbiter.UnpackResource = IopDmaUnpackResource;
    IopRootDmaArbiter.ScoreRequirement = IopDmaScoreRequirement;
    IopRootDmaArbiter.OverrideConflict = IopDmaOverrideConflict;

    return ArbInitializeArbiterInstance(&IopRootDmaArbiter,
                                        NULL,
                                        CmResourceTypeDma,
                                        L"RootDMA",
                                        L"Root",
                                        NULL    // no translation of DMA
                                       );
}

//
// Arbiter callbacks
//

NTSTATUS
IopDmaUnpackRequirement(
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
    ASSERT(Descriptor->Type == CmResourceTypeDma);

    ARB_PRINT(2,
                ("Unpacking DMA requirement %p => 0x%I64x-0x%I64x\n",
                Descriptor,
                (ULONGLONG) Descriptor->u.Dma.MinimumChannel,
                (ULONGLONG) Descriptor->u.Dma.MaximumChannel
                ));

    *Minimum = (ULONGLONG) Descriptor->u.Dma.MinimumChannel;
    *Maximum = (ULONGLONG) Descriptor->u.Dma.MaximumChannel;
    *Length = 1;
    *Alignment = 1;

    return STATUS_SUCCESS;

}

LONG
IopDmaScoreRequirement(
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
    ASSERT(Descriptor->Type == CmResourceTypeDma);
    
    score = Descriptor->u.Dma.MaximumChannel - Descriptor->u.Dma.MinimumChannel;
    
    ARB_PRINT(2,
                ("Scoring DMA resource %p => %i\n",
                Descriptor,
                score
                ));

    return score;
}

NTSTATUS
IopDmaPackResource(
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
    ASSERT(Requirement->Type == CmResourceTypeDma);

    ARB_PRINT(2,
                ("Packing DMA resource %p => 0x%I64x\n",
                Descriptor,
                Start
                ));
    
    Descriptor->Type = CmResourceTypeDma;
    Descriptor->ShareDisposition = Requirement->ShareDisposition;
    Descriptor->Flags = Requirement->Flags; // BUGBUG - is this correct?
    Descriptor->u.Dma.Channel = (ULONG) Start;
    Descriptor->u.Dma.Port = 0;

    return STATUS_SUCCESS;
}

NTSTATUS
IopDmaUnpackResource(
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
    
    Length - Pointer to where the length value should be unpacked to.
    
Return Value:

    Returns the status of this operation.

--*/

{

    *Start = Descriptor->u.Dma.Channel;
    *Length = 1;
    
    ARB_PRINT(2,
                ("Unpacking DMA resource %p => 0x%I64x\n",
                Descriptor,
                *Start
                ));
    
    return STATUS_SUCCESS;

}


BOOLEAN
IopDmaOverrideConflict(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    )

/*++

Routine Description:

    Just say no.

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    TRUE if the conflict is allowable, false otherwise

--*/

{
    PAGED_CODE();
    
    return FALSE;
}

