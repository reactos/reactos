/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pnpmemio.c

Abstract:

    Root IO Port and Memory arbiter

Author:

    Andy Thornton (andrewth) 04/17/97

Revision History:

--*/

#include "iop.h"
#pragma hdrstop

#define BUGFEST_HACKS

//
// Constants
//

#define MAX_ULONGLONG           ((ULONGLONG) -1)
#define MAX_ALIAS_PORT          0x0000FFFF

typedef struct _PORT_ARBITER_EXTENSION {

    PRTL_RANGE_LIST Aliases;
    PRTL_RANGE_LIST PossibleAliases;
    RTL_RANGE_LIST RangeLists[2];

} PORT_ARBITER_EXTENSION, *PPORT_ARBITER_EXTENSION;

//
// Prototypes
//

VOID
IopPortBacktrackAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     );

BOOLEAN
IopPortGetNextAlias(
    ULONG IoDescriptorFlags,
    ULONGLONG LastAlias,
    PULONGLONG NextAlias
    );

BOOLEAN
IopPortFindSuitableRange(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    );

BOOLEAN
IopMemFindSuitableRange(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    );


NTSTATUS
IopGenericUnpackRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Minimum,
    OUT PULONGLONG Maximum,
    OUT PULONG Length,
    OUT PULONG Alignment
    );

NTSTATUS
IopGenericPackResource(
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    IN ULONGLONG Start,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

LONG
IopGenericScoreRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    );

NTSTATUS
IopGenericUnpackResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    );

BOOLEAN
IopPortIsAliasedRangeAvailable(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

NTSTATUS
IopMemInitialize(
    VOID
    );

VOID
IopPortAddAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

NTSTATUS
IopTranslateBusAddress(
    IN PHYSICAL_ADDRESS SourceAddress,
    IN UCHAR SourceResourceType,
    OUT PPHYSICAL_ADDRESS TargetAddress,
    OUT PUCHAR TargetResourceType
    );


//
// Make everything pageable
//

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, IopPortInitialize)
#pragma alloc_text(PAGE, IopMemInitialize)
#pragma alloc_text(PAGE, IopGenericUnpackRequirement)
#pragma alloc_text(PAGE, IopGenericPackResource)
#pragma alloc_text(PAGE, IopGenericScoreRequirement)
#pragma alloc_text(PAGE, IopGenericUnpackResource)
#pragma alloc_text(PAGE, IopPortBacktrackAllocation)
#pragma alloc_text(PAGE, IopPortFindSuitableRange)
#pragma alloc_text(PAGE, IopMemFindSuitableRange)
#pragma alloc_text(PAGE, IopPortGetNextAlias)
#pragma alloc_text(PAGE, IopPortAddAllocation)
#pragma alloc_text(PAGE, IopPortIsAliasedRangeAvailable)
#pragma alloc_text(PAGE, IopTranslateBusAddress)
#endif // ALLOC_PRAGMA


#define ADDRESS_SPACE_MEMORY                0x0
#define ADDRESS_SPACE_PORT                  0x1
#define ADDRESS_SPACE_USER_MEMORY           0x2
#define ADDRESS_SPACE_USER_PORT             0x3
#define ADDRESS_SPACE_DENSE_MEMORY          0x4
#define ADDRESS_SPACE_USER_DENSE_MEMORY     0x6

NTSTATUS
IopTranslateBusAddress(
    IN PHYSICAL_ADDRESS SourceAddress,
    IN UCHAR SourceResourceType,
    OUT PPHYSICAL_ADDRESS TargetAddress,
    OUT PUCHAR TargetResourceType
    )
/*++

Routine Description:

    This routine translates addresses.

Parameters:

    SourceAddress - The address to translate

    ResourceType - The resource type (IO or Memory) we are translaing.  If the
        address space changes from IO->Memory this will be updated.

    TargetAddress - Pointer to where the target should be translated to.

Return Value:

    STATUS_SUCCESS or an error status

--*/

{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG sourceAddressSpace, targetAddressSpace;
    BOOLEAN translated;

    PAGED_CODE();

    //
    // Select the appropriate address space
    //

    if (SourceResourceType == CmResourceTypeMemory) {
        sourceAddressSpace = ADDRESS_SPACE_MEMORY;
    } else if (SourceResourceType == CmResourceTypePort) {
        sourceAddressSpace = ADDRESS_SPACE_PORT;
    } else {
        return STATUS_INVALID_PARAMETER;
    }

    ARB_PRINT(
        2,
        ("Translating %s address 0x%I64x => ",
        SourceResourceType == CmResourceTypeMemory ? "Memory" : "I/O",
        SourceAddress.QuadPart
       ));

    //
    // HACKHACK Ask the HAL to translate on ISA bus - if we can't then just
    // don't translate because this must be a PCI system so the root arbiters
    // don't do much (Yes it's a steaming hack but it'll work for beta 1)
    //

    targetAddressSpace = sourceAddressSpace;
    translated = HalTranslateBusAddress(
                     Isa,
                     0,
                     SourceAddress,
                     &targetAddressSpace,
                     TargetAddress
                     );

    if (!translated) {
        ARB_PRINT(2,("Translation failed!\n"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Update the resource type in the target if we have gone from Io to Memory
    //


    //
    // BUBBUG - update the length for IO -> Memory (Dense vs Sparse)
    // I think the answer is dense -> spares is multiply length by 32
    //

    if (targetAddressSpace == ADDRESS_SPACE_MEMORY
    ||  targetAddressSpace == ADDRESS_SPACE_USER_MEMORY
    ||  targetAddressSpace == ADDRESS_SPACE_DENSE_MEMORY
    ||  targetAddressSpace == ADDRESS_SPACE_USER_DENSE_MEMORY) {
        *TargetResourceType = CmResourceTypeMemory;
    } else if (targetAddressSpace == ADDRESS_SPACE_PORT
           ||  targetAddressSpace == ADDRESS_SPACE_USER_PORT) {
        *TargetResourceType = CmResourceTypePort;
    } else {
        ASSERT(0 && "Translation has returned an unknown address space");
    }

    ARB_PRINT(
        2,
        ("%s address 0x%I64x\n",
        *TargetResourceType == CmResourceTypeMemory ? "Memory" : "I/O",
        TargetAddress->QuadPart
        ));

    return STATUS_SUCCESS;

}


NTSTATUS
IopGenericTranslateOrdering(
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

    STATUS_SUCCESS



*/

{
    NTSTATUS status;
    UCHAR initialResourceType, minResourceType, maxResourceType;
    PAGED_CODE();


    *Target = *Source;

    if (Source->Type != CmResourceTypeMemory
    && Source->Type != CmResourceTypePort) {
        return STATUS_SUCCESS;
    }

    initialResourceType = Source->Type;

    //
    // Translate the minimum
    //

    status = IopTranslateBusAddress(Source->u.Generic.MinimumAddress,
                                    initialResourceType,
                                    &Target->u.Generic.MinimumAddress,
                                    &minResourceType
                                    );

    if (NT_SUCCESS(status)) {

        //
        // Translate the maximum iff we could translate the minimum
        //

        status = IopTranslateBusAddress(Source->u.Generic.MaximumAddress,
                                        initialResourceType,
                                        &Target->u.Generic.MaximumAddress,
                                        &maxResourceType
                                        );

    }

    //
    // If we couldn't translate both ends of the range then we want to skip this
    // range - set it's type to CmResourceTypeNull
    //

    if (!NT_SUCCESS(status)) {
        Target->Type = CmResourceTypeNull;
    } else {
        ASSERT(minResourceType == maxResourceType);
        Target->Type = minResourceType;
    }

    return STATUS_SUCCESS;

}

//
// Implementation
//

NTSTATUS
IopPortInitialize(
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
    PAGED_CODE();

    //
    // Fill in the non-default action handlers
    //

    IopRootPortArbiter.FindSuitableRange    = IopPortFindSuitableRange;
    IopRootPortArbiter.AddAllocation        = IopPortAddAllocation;
    IopRootPortArbiter.BacktrackAllocation  = IopPortBacktrackAllocation;

    IopRootPortArbiter.UnpackRequirement    = IopGenericUnpackRequirement;
    IopRootPortArbiter.PackResource         = IopGenericPackResource;
    IopRootPortArbiter.UnpackResource       = IopGenericUnpackResource;
    IopRootPortArbiter.ScoreRequirement     = IopGenericScoreRequirement;

    return ArbInitializeArbiterInstance(&IopRootPortArbiter,
                                        NULL,     // Indicates ROOT arbiter
                                        CmResourceTypePort,
                                        L"RootPort",
                                        L"Root",
                                        IopGenericTranslateOrdering
                                        );

}

NTSTATUS
IopMemInitialize(
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
    NTSTATUS status;

    PAGED_CODE();

    IopRootMemArbiter.UnpackRequirement = IopGenericUnpackRequirement;
    IopRootMemArbiter.PackResource      = IopGenericPackResource;
    IopRootMemArbiter.UnpackResource    = IopGenericUnpackResource;
    IopRootMemArbiter.ScoreRequirement  = IopGenericScoreRequirement;

    IopRootMemArbiter.FindSuitableRange    = IopMemFindSuitableRange;

    status = ArbInitializeArbiterInstance(&IopRootMemArbiter,
                                          NULL,     // Indicates ROOT arbiter
                                          CmResourceTypeMemory,
                                          L"RootMemory",
                                          L"Root",
                                          IopGenericTranslateOrdering
                                          );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Allocate the first page of physical memory as the firmware uses it and
    // doesn't report it as so Mm doesn't reuse it.
    //

    status = RtlAddRange(IopRootMemArbiter.Allocation,
                         0,
                         PAGE_SIZE,
                         0, // RangeAttributes
                         0, // Flags
                         NULL,
                         NULL
                         );
    return status;

}


//
// Arbiter callbacks
//

NTSTATUS
IopGenericUnpackRequirement(
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
    PAGED_CODE();
    ASSERT(Descriptor);
    ASSERT(Descriptor->Type == CmResourceTypePort
           || Descriptor->Type == CmResourceTypeMemory);


    *Minimum = (ULONGLONG) Descriptor->u.Generic.MinimumAddress.QuadPart;
    *Maximum = (ULONGLONG) Descriptor->u.Generic.MaximumAddress.QuadPart;
    *Length = Descriptor->u.Generic.Length;
    *Alignment = Descriptor->u.Generic.Alignment;

    //
    // Fix the broken hardware that reports 0 alignment
    //

    if (*Alignment == 0) {
        *Alignment = 1;
    }

    //
    // Fix broken INF's that report they support 24bit memory > 0xFFFFFF
    //

    if (Descriptor->Type == CmResourceTypeMemory
    && Descriptor->Flags & CM_RESOURCE_MEMORY_24
    && Descriptor->u.Memory.MaximumAddress.QuadPart > 0xFFFFFF) {
        *Maximum = 0xFFFFFF;
    }

    ARB_PRINT(2,
                ("Unpacking %s requirement %p => 0x%I64x-0x%I64x length 0x%x alignment 0x%x\n",
                Descriptor->Type == CmResourceTypePort ? "port" : "memory",
                Descriptor,
                *Minimum,
                *Maximum,
                *Length,
                *Alignment
                ));

    return STATUS_SUCCESS;

}

LONG
IopGenericScoreRequirement(
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
    ULONGLONG start, end;
    LONGLONG bigscore;
    ULONG alignment;

    PAGED_CODE();

#define MAX_SCORE MAXLONG

    ASSERT(Descriptor);
    ASSERT((Descriptor->Type == CmResourceTypePort) ||
           (Descriptor->Type == CmResourceTypeMemory));

    alignment = Descriptor->u.Generic.Alignment;

    //
    // Fix the broken hardware that reports 0 alignment
    // Since this is not a PCI device, set the alignment to 1.
    //
    //

    if (alignment == 0 &&
        ((Descriptor->Type == CmResourceTypePort) ||
         (Descriptor->Type == CmResourceTypeMemory))) {
        alignment = 1;
    }



    start = ALIGN_ADDRESS_UP(
                Descriptor->u.Generic.MinimumAddress.QuadPart,
                alignment
                );

    end = Descriptor->u.Generic.MaximumAddress.QuadPart;

    //
    // The score is the number of possible allocations that could be made
    // given the alignment and length constraints
    //

    bigscore = (((end - Descriptor->u.Generic.Length + 1) - start)
                    / alignment) + 1;

    score = (LONG)bigscore;
    if (bigscore < 0) {
        score = -1;
    } else if (bigscore > MAX_SCORE) {
        score = MAX_SCORE;
    }

    ARB_PRINT(2,
                ("Scoring port resource %p(0x%I64x-0x%I64x) => %i\n",
                Descriptor->Type == CmResourceTypePort ? "port" : "memory",
                Descriptor,
                Descriptor->u.Generic.MinimumAddress.QuadPart,
                end,
                score
                ));

    return score;
}

NTSTATUS
IopGenericPackResource(
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

    PAGED_CODE();
    ASSERT(Descriptor);
    ASSERT(Requirement);
    ASSERT(Requirement->Type == CmResourceTypePort
           || Requirement->Type == CmResourceTypeMemory);

    Descriptor->Type = Requirement->Type;
    Descriptor->Flags = Requirement->Flags;
    Descriptor->ShareDisposition = Requirement->ShareDisposition;
    Descriptor->u.Generic.Start.QuadPart = Start;
    Descriptor->u.Generic.Length = Requirement->u.Generic.Length;

    ARB_PRINT(2,
                ("Packing %s resource %p => 0x%I64x length 0x%x\n",
                Descriptor->Type == CmResourceTypePort ? "port" : "memory",
                Descriptor,
                Descriptor->u.Port.Start.QuadPart,
                Descriptor->u.Port.Length
                ));

    return STATUS_SUCCESS;
}

NTSTATUS
IopGenericUnpackResource(
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

    PAGED_CODE();
    ASSERT(Descriptor);
    ASSERT(Descriptor->Type == CmResourceTypePort
           || Descriptor->Type == CmResourceTypeMemory);

    *Start = Descriptor->u.Generic.Start.QuadPart;
    *Length = Descriptor->u.Generic.Length;

    ARB_PRINT(2,
                ("Unpacking %s resource %p => 0x%I64x Length 0x%x\n",
                Descriptor->Type == CmResourceTypePort ? "port" : "memory",
                Descriptor,
                *Start,
                *Length
                ));

    return STATUS_SUCCESS;

}
#if 0
NTSTATUS
IopPortRetestAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    )

/*++

Routine Description:

    This providesa port specific implementation of the RetestAllocation action
    which takes into account ISA aliases and adds them where appropriate.
    It walks the arbitration list and updates the possible allocation to reflect
    the allocation entries of the list.  For these entries to be valid
    TestAllocation must have been performed on this arbitration list.

Parameters:

    Arbiter - The arbiter instance data for the arbiter being called.

    ArbitrationList - A list of ARBITER_LIST_ENTRY entries which contain the
        requirements and associated devices.  TestAllocation for this arbiter
        should have been called on this list.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    PARBITER_LIST_ENTRY current;
    PIO_RESOURCE_DESCRIPTOR alternative;
    ULONGLONG start;
    ULONG length;

    PAGED_CODE();

    //
    // Copy the current allocation and reserved
    //

    ARB_PRINT(3, ("Retest: Copy current allocation\n"));
    status = RtlCopyRangeList(Arbiter->PossibleAllocation, Arbiter->Allocation);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Free all the resources currently allocated to all the devices we
    // are arbitrating for
    //

    FOR_ALL_IN_LIST(ARBITER_LIST_ENTRY, ArbitrationList, current) {

        ARB_PRINT(2, ("Retest: Delete 0x%08x's resources\n", current->PhysicalDeviceObject));

        status = RtlDeleteOwnersRanges(Arbiter->PossibleAllocation,
                                       (PVOID) current->PhysicalDeviceObject
                                       );

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }
    }

    //
    // Copy the previous allocation into the range list
    //

    FOR_ALL_IN_LIST(ARBITER_LIST_ENTRY, ArbitrationList, current) {

        ASSERT(current->Assignment);

        status = Arbiter->UnpackResource(current->Assignment,
                                         &start,
                                         &length
                                         );

        ASSERT(NT_SUCCESS(status));

        //
        // If we had a requirement for length 0 then that will be seen as
        // end == start - 1 here so don't attempt to add the range - it will
        // fail!
        //

        if (length != 0) {

            status = RtlAddRange(
                Arbiter->PossibleAllocation,
                start,
                start + length - 1,
                0,
                RTL_RANGE_LIST_ADD_IF_CONFLICT +
                    (current->Assignment->ShareDisposition == CmResourceShareShared ?
                        RTL_RANGE_LIST_ADD_SHARED : 0),
                NULL,
                current->PhysicalDeviceObject
                );

            ASSERT(NT_SUCCESS(status));

            //
            // Retireve the alternative from which the assignment was chosen from
            // then
            //

            alternative = current->SelectedAlternative;

            //
            // Add the aliases
            //

            if (alternative->Flags & CM_RESOURCE_PORT_10_BIT_DECODE
            || alternative->Flags & CM_RESOURCE_PORT_12_BIT_DECODE) {

                ULONGLONG alias = start;
                BOOLEAN shared = current->Assignment->ShareDisposition ==
                                     CmResourceShareShared;

                ARB_PRINT(3, ("Adding aliases\n"));

                while (IopPortGetNextAlias(alternative->Flags,
                                           alias,
                                           &alias)) {

                    status = RtlAddRange(
                                 Arbiter->PossibleAllocation,
                                 alias,
                                 alias + length - 1,
                                 ARBITER_RANGE_ALIAS,
                                 RTL_RANGE_LIST_ADD_IF_CONFLICT +
                                    (shared ? RTL_RANGE_LIST_SHARED_OK : 0),
                                 NULL,
                                 current->PhysicalDeviceObject
                                 );

                    //
                    // We have already checked if these ranges are available
                    // so we should not fail...
                    //

                    ASSERT(NT_SUCCESS(status));
                }
            }
        }
    }

    return status;

cleanup:

    RtlFreeRangeList(Arbiter->PossibleAllocation);
    return status;
}
#endif
VOID
IopPortBacktrackAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     )

/*++

Routine Description:

    This routine is called from AllocateEntry if the possible solution
    (State->Start - State->End) does not allow us to allocate resources to
    the rest of the devices being considered.  It deletes the ranges that were
    added to Arbiter->PossibleAllocation by AddAllocation including those
    associated with ISA aliases.

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    None.

--*/


{

    NTSTATUS status;
    ULONGLONG alias = State->Start;

    PAGED_CODE();

    //
    // Delete the aliases
    //

    ARB_PRINT(2, ("\t\tDeleting aliases\n"));

    while (IopPortGetNextAlias(State->CurrentAlternative->Flags,
                               alias,
                               &alias)) {

        status = RtlDeleteRange(
                     Arbiter->PossibleAllocation,
                     alias,
                     alias + State->CurrentAlternative->Length - 1,
                     State->Entry->PhysicalDeviceObject
                     );

        //
        // We should not fail...
        //

        ASSERT(NT_SUCCESS(status));
    }

    //
    // Now call the original function to delete the base range
    //

    ArbBacktrackAllocation(Arbiter, State);

}


BOOLEAN
IopPortFindSuitableRange(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    )
/*++

Routine Description:

    This routine is called from AllocateEntry once we have decided where we want
    to allocate from.  It tries to find a free range that matches the
    requirements in State while restricting its possible solutions to the range
    State->Start to State->CurrentMaximum.  On success State->Start and
    State->End represent this range.  Conflicts with ISA aliases are considered.

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    TRUE if we found a range, FALSE otherwise.

--*/
{
    NTSTATUS status;
    UCHAR userFlagsMask = 0;

    PAGED_CODE();

    //
    // If we are asking for zero ports then trivially succeed with the minimum
    // value
    //

    if (State->CurrentAlternative->Length == 0) {
        State->End = State->Start;
        return TRUE;
    }

    //
    // For legacy requests from IoAssignResources (directly or by way of
    // HalAssignSlotResources) or IoReportResourceUsage we consider preallocated
    // resources to be available for backward compatibility reasons.
    //
    // If we are allocating a devices boot config then we consider all other
    // boot configs to be available.
    //

    if (State->Entry->RequestSource == ArbiterRequestLegacyReported
        || State->Entry->RequestSource == ArbiterRequestLegacyAssigned
        || State->Entry->Flags & ARBITER_FLAG_BOOT_CONFIG) {

        userFlagsMask = ARBITER_RANGE_BOOT_ALLOCATED;
    }

    //
    // Try to satisfy the request
    //

    while (State->CurrentMinimum <= State->CurrentMaximum) {

        //
        // Select the first free alternative from the current alternative
        //

        status = RtlFindRange(
                     Arbiter->PossibleAllocation,
                     State->CurrentMinimum,
                     State->CurrentMaximum,
                     State->CurrentAlternative->Length,
                     State->CurrentAlternative->Alignment,
                     State->CurrentAlternative->Flags &
                        ARBITER_ALTERNATIVE_FLAG_SHARED ?
                            RTL_RANGE_LIST_SHARED_OK : 0,
                     userFlagsMask,
                     Arbiter->ConflictCallbackContext,
                     Arbiter->ConflictCallback,
                     &State->Start
                     );


        //
        // Did we find a range and if not can we override any conflict
        //
        if (NT_SUCCESS(status)
        || Arbiter->OverrideConflict(Arbiter, State)) {

            State->End = State->Start + State->CurrentAlternative->Length - 1;

            //
            // Check if the aliases are available
            //
            if (IopPortIsAliasedRangeAvailable(Arbiter, State)) {

                //
                // We found a suitable range so return
                //

                return TRUE;

            } else {

                //
                // This range's aliases arn't available so try the next range
                //

                State->Start += State->CurrentAlternative->Length;

                continue;
            }
        } else {

            //
            // We couldn't find a base range
            //

            break;
        }
    }

    return FALSE;
}



BOOLEAN
IopPortGetNextAlias(
    ULONG IoDescriptorFlags,
    ULONGLONG LastAlias,
    PULONGLONG NextAlias
    )
/*++

Routine Description:

    This routine calculates the next alias of an IO port up to MAX_ALIAS_PORT.

Arguments:

    IoDescriptorFlags - The flags from the requirement descriptor indicating the
        type of alias if any.

    LastAlias - The alias previous to this one.

    NextAlias - Point to where the next alias should be returned

Return Value:

    TRUE if we found an alias, FALSE otherwise.

--*/

{
    ULONGLONG next;

    PAGED_CODE();

    if (IoDescriptorFlags & CM_RESOURCE_PORT_10_BIT_DECODE) {
        next = LastAlias + (1 << 10);
    } else if (IoDescriptorFlags & CM_RESOURCE_PORT_12_BIT_DECODE) {
        next = LastAlias + (1 << 12);
    } else {

        // BUGBUG - should CM_RESOURCE_PORT_16_BIT_DECODE be set?
        //
        // There are no aliases
        //

        return FALSE;
    }

    //
    // Check that we are below the maximum aliased port
    //

    if (next > MAX_ALIAS_PORT) {
        return FALSE;
    } else {
        *NextAlias = next;
        return TRUE;
    }
}


VOID
IopPortAddAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    )

/*++

Routine Description:

    This routine is called from AllocateEntry once we have found a possible
    solution (State->Start - State->End).  It adds the ranges that will not be
    available if we commit to this solution to Arbiter->PossibleAllocation.

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    ULONGLONG alias;
    BOOLEAN isAlias;

    PAGED_CODE();

    ASSERT(Arbiter);
    ASSERT(State);

    status = RtlAddRange(Arbiter->PossibleAllocation,
                 State->Start,
                 State->End,
                 State->RangeAttributes,
                 RTL_RANGE_LIST_ADD_IF_CONFLICT +
                    (State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED
                        ? RTL_RANGE_LIST_ADD_SHARED : 0),
                 NULL,
                 State->Entry->PhysicalDeviceObject
                 );

    ASSERT(NT_SUCCESS(status));

    //
    // Add any aliases
    //

    alias = State->Start;
    ARB_PRINT(2, ("Adding aliases\n"));

    while (IopPortGetNextAlias(State->CurrentAlternative->Descriptor->Flags,
                             alias,
                             &alias)) {

        status = RtlAddRange(Arbiter->PossibleAllocation,
                     alias,
                     alias + State->CurrentAlternative->Length - 1,
                     (UCHAR) (State->RangeAttributes | ARBITER_RANGE_ALIAS),
                     RTL_RANGE_LIST_ADD_IF_CONFLICT +
                        (State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED
                            ? RTL_RANGE_LIST_ADD_SHARED : 0),
                     NULL,
                     State->Entry->PhysicalDeviceObject
                     );

        //
        // We have already checked if these ranges are available
        // so we should not fail...
        //

        ASSERT(NT_SUCCESS(status));
    }
}


BOOLEAN
IopPortIsAliasedRangeAvailable(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    )

/*++

Routine Description:

    This routine determines if the range (Start-(Length-1)) is available taking
    into account any aliases.

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    TRUE if the range is available, FALSE otherwise.

--*/

{
    NTSTATUS status;
    ULONGLONG alias = State->Start;
    BOOLEAN aliasAvailable;
    UCHAR userFlagsMask = 0;

    PAGED_CODE();

#if defined(BUGFEST_HACKS)
    //
    // For the purposes of the Bug^H^H^HPlugFest don't mind is aliases conflict
    // with any devices but still add them...
    //
    return TRUE;
#endif

    //
    // For legacy requests from IoAssignResources (directly or by way of
    // HalAssignSlotResources) or IoReportResourceUsage we consider preallocated
    // resources to be available for backward compatibility reasons.
    //
    if (State->Entry->RequestSource == ArbiterRequestLegacyReported
        || State->Entry->RequestSource == ArbiterRequestLegacyAssigned) {

        userFlagsMask |= ARBITER_RANGE_BOOT_ALLOCATED;
    }

    while (IopPortGetNextAlias(State->CurrentAlternative->Descriptor->Flags,
                             alias,
                             &alias)) {

        status = RtlIsRangeAvailable(
                     Arbiter->PossibleAllocation,
                     alias,
                     alias + State->CurrentAlternative->Length - 1,
                     State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED ?
                        RTL_RANGE_LIST_SHARED_OK : 0,
                     userFlagsMask,
                     Arbiter->ConflictCallbackContext,
                     Arbiter->ConflictCallback,
                     &aliasAvailable
                     );

        ASSERT(NT_SUCCESS(status));

        if (!aliasAvailable) {

            ARBITER_ALLOCATION_STATE tempState;

            //
            // Check if we allow this conflict by calling OverrideConflict -
            // we will need to falsify ourselves an allocation state first
            //
            // BUGBUG - this works but relies on knowing what OverrideConflict
            // looks at.  A better fix invloves storing the aliases in another
            // list but this it too much of a change for Win2k
            //

            RtlCopyMemory(&tempState, State, sizeof(ARBITER_ALLOCATION_STATE));

            tempState.CurrentMinimum = alias;
            tempState.CurrentMaximum = alias + State->CurrentAlternative->Length - 1;

            if (Arbiter->OverrideConflict(Arbiter, &tempState)) {
                //
                // We decided this conflict was ok so contine checking the rest
                // of the aliases
                //

                continue;

            }

            //
            // An alias isn't available - get another possibility
            //

            ARB_PRINT(2,
                        ("\t\tAlias 0x%x-0x%x not available\n",
                        alias,
                        alias + State->CurrentAlternative->Length - 1
                        ));

            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
IopMemFindSuitableRange(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    )
/*++

Routine Description:

    This routine is called from AllocateEntry once we have decided where we want
    to allocate from.  It tries to find a free range that matches the
    requirements in State while restricting its possible solutions to the range
    State->Start to State->CurrentMaximum.  On success State->Start and
    State->End represent this range.  Conflicts between boot configs are allowed

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    TRUE if we found a range, FALSE otherwise.

--*/
{
    //
    // If this was a boot config then consider other boot configs to be
    // available
    //

    if (State->Entry->Flags & ARBITER_FLAG_BOOT_CONFIG) {
        State->RangeAvailableAttributes |= ARBITER_RANGE_BOOT_ALLOCATED;
    }

    //
    // Do the default thing
    //

    return ArbFindSuitableRange(Arbiter, State);
}

