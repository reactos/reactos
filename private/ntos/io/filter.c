/*++

Copyright (c) 1991  Microsoft Corporation
All rights reserved

Module Name:

    filter.c

Abstract:

    This module contains support functions for IoAssignResources to filter caller's
    IoRequirementResourceList.
    Some of the stuff here is copied from KenR's Hal code.

Author:

    Shie-Lin Tzong (shielint) Apr-7-1995

Environment:

    Kernel mode only.

Revision History:

*/

#include "iop.h"

#if _PNP_POWER_

typedef struct _NRParams {
    PIO_RESOURCE_DESCRIPTOR     InDesc;
    PIO_RESOURCE_DESCRIPTOR     OutDesc;
    PSUPPORTED_RANGE            CurrentPosition;
    LONGLONG                    Base;
    LONGLONG                    Limit;
    UCHAR                       DescOpt;
    BOOLEAN                     AnotherListPending;
} NRPARAMS, *PNRPARAMS;

PIO_RESOURCE_REQUIREMENTS_LIST
IopCmResourcesToIoResources (
    IN ULONG SlotNumber,
    IN PCM_RESOURCE_LIST CmResourceList
    );

NTSTATUS
IopAdjustResourceListRange (
    IN PSUPPORTED_RANGES                    SupportedRanges,
    IN PSUPPORTED_RANGE                     InterruptRange,
    IN PIO_RESOURCE_REQUIREMENTS_LIST       IoResourceList,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *ReturnedList
    );

NTSTATUS
IopGetNextSupportedRange (
    IN LONGLONG             MinimumAddress,
    IN LONGLONG             MaximumAddress,
    IN OUT PNRPARAMS        PNRParams,
    OUT PIO_RESOURCE_DESCRIPTOR *IoDescriptor
    );

ULONG
IopSortRanges (
    IN PSUPPORTED_RANGE     RangeList
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,IopAdjustResourceListRange)
#pragma alloc_text(PAGE,IopSortRanges)
#pragma alloc_text(PAGE,IopGetNextSupportedRange)
#pragma alloc_text(PAGE,IopAddDefaultResourceList)
#pragma alloc_text(PAGE,IopCmResourcesToIoResources)
#endif

PIO_RESOURCE_REQUIREMENTS_LIST
IopAddDefaultResourceList (
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoResourceList,
    IN PCM_RESOURCE_LIST CmResourceList
    )
/*++

Routine Description:

    This functions builds an IO resource list by adjusting input IoResourceList to
    the ranges specified by CmResourceList and merges the new Io resource list to the
    input IoResourceList and returns the combined list.
    If IoResourceList has zero descriptor, this routine bulds an
    IO RESOURCE REQUIREMENTS LIST by simply converting the CmResourceList.
    It's caller's responsiblity to free the returned list.

Arguments:

    IoResourceList - The resource requirements list which needs to
                     be adjusted.

    CmResourceList - the cm resource list to specify the resource ranges desired.

Return Value:

    returns a IO_RESOURCE_REQUIREMENTS_LISTST if succeeds.  Otherwise a NULL value is
    returned.

--*/
{
    PIO_RESOURCE_REQUIREMENTS_LIST ioResReqList, combinedList = NULL;
    SUPPORTED_RANGES supportRanges;
    SUPPORTED_RANGE interruptRange;
    PSUPPORTED_RANGE range, tmpRange;
    PSUPPORTED_RANGE intRange = NULL, ioRange = NULL, memRange = NULL, dmaRange = NULL;
    ULONG size, i, j, addressSpace = 0;
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmPartDesc;
    NTSTATUS status;

    if (!IoResourceList) {

        //
        // If IoResourceList is NULL, return failure.
        //

        return NULL;
    } else if (IoResourceList->List[0].Count == 0) {

        //
        // an empty Io resource requirements list, simply return the converted
        // list.
        //

        ioResReqList = IopCmResourcesToIoResources(IoResourceList->SlotNumber,
                                                   CmResourceList);
        return ioResReqList;
    }

    //
    // Clip the IoResourceList against CmResourceList.
    // First, build the supported ranges structure to do the clipping.
    //

    RtlZeroMemory(&supportRanges, sizeof(supportRanges));
    supportRanges.Version = BUS_SUPPORTED_RANGE_VERSION;
    supportRanges.Sorted = FALSE;
    ioResReqList = NULL;

    cmFullDesc = &CmResourceList->List[0];
    for (i = 0; i < CmResourceList->Count; i++) {
        cmPartDesc = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
        for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {
            size = 0;
            range = NULL;
            switch (cmPartDesc->Type) {
            case CmResourceTypePort:
                 if (!ioRange) {
                     range = &supportRanges.IO;
                 } else {
                     range = (PSUPPORTED_RANGE)ExAllocatePool(PagedPool, sizeof(SUPPORTED_RANGE));
                     ioRange->Next = range;
                 }
                 range->SystemAddressSpace = 1;
                 range->SystemBase = 0;
                 range->Base = cmPartDesc->u.Port.Start.QuadPart;
                 range->Limit = cmPartDesc->u.Port.Start.QuadPart +
                                            cmPartDesc->u.Port.Length - 1;
                 range->Next = NULL;
                 ioRange = range;
                 supportRanges.NoIO++;
                 break;
            case CmResourceTypeInterrupt:
                 if (!intRange) {
                     range = &interruptRange;
                 } else {
                     range = (PSUPPORTED_RANGE)ExAllocatePool(PagedPool, sizeof(SUPPORTED_RANGE));
                     intRange->Next = range;
                 }
                 range->SystemAddressSpace = 0;
                 range->SystemBase = 0;
                 range->Base = cmPartDesc->u.Interrupt.Vector;
                 range->Limit = cmPartDesc->u.Interrupt.Vector;
                 range->Next = NULL;
                 intRange = range;
                 break;
            case CmResourceTypeMemory:
                 if (!memRange) {
                     range = &supportRanges.Memory;
                 } else {
                     range = (PSUPPORTED_RANGE)ExAllocatePool(PagedPool, sizeof(SUPPORTED_RANGE));
                     memRange->Next = range;
                 }
                 range->SystemAddressSpace = 0;
                 range->SystemBase = 0;
                 range->Base = cmPartDesc->u.Memory.Start.QuadPart;
                 range->Limit = cmPartDesc->u.Memory.Start.QuadPart +
                                            cmPartDesc->u.Memory.Length - 1;
                 range->Next = NULL;
                 memRange = range;
                 supportRanges.NoMemory++;
                 break;
            case CmResourceTypeDma:
                 if (!dmaRange) {
                     range = &supportRanges.Dma;
                 } else {
                     range = (PSUPPORTED_RANGE)ExAllocatePool(PagedPool, sizeof(SUPPORTED_RANGE));
                     dmaRange->Next = range;
                 }
                 range->SystemAddressSpace = 0;
                 range->SystemBase = 0;
                 range->Base = cmPartDesc->u.Dma.Channel;
                 range->Limit = cmPartDesc->u.Dma.Channel;
                 range->Next = NULL;
                 dmaRange = range;
                 supportRanges.NoDma++;
                 break;
            case CmResourceTypeDeviceSpecific:
                 size = cmPartDesc->u.DeviceSpecificData.DataSize;
                 break;
            }
            cmPartDesc++;
            cmPartDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmPartDesc + size);
        }
        cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmPartDesc;
    }

    //
    // Adjust the input resource requirements list ...
    //

    status = IopAdjustResourceListRange (
                 &supportRanges,
                 &interruptRange,
                 IoResourceList,
                 &ioResReqList
                 );

    if (!NT_SUCCESS(status)) {
        goto exit0;
    }

    //
    // Combine the clipped io resource requirements list with the original resource
    // list.
    //

    size = ioResReqList->ListSize + IoResourceList->ListSize -
           FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List);
    combinedList = (PIO_RESOURCE_REQUIREMENTS_LIST) ExAllocatePool(PagedPool, size);
    if (combinedList) {
        RtlMoveMemory(combinedList, ioResReqList, ioResReqList->ListSize);
        combinedList->ListSize = size;
        combinedList->AlternativeLists += IoResourceList->AlternativeLists;
        RtlMoveMemory((PUCHAR)combinedList + ioResReqList->ListSize,
                      (PUCHAR)IoResourceList->List,
                      IoResourceList->ListSize - FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List)
                      );
    }
    ExFreePool(ioResReqList);
exit0:

    //
    // Release the space allocated for supported ranges...
    //

    range = interruptRange.Next;
    while (range) {
        tmpRange = range;
        range = range->Next;
        ExFreePool(tmpRange);
    }
    range = supportRanges.IO.Next;
    while (range) {
        tmpRange = range;
        range = range->Next;
        ExFreePool(tmpRange);
    }
    range = supportRanges.Memory.Next;
    while (range) {
        tmpRange = range;
        range = range->Next;
        ExFreePool(tmpRange);
    }
    range = supportRanges.Dma.Next;
    while (range) {
        tmpRange = range;
        range = range->Next;
        ExFreePool(tmpRange);
    }
    return combinedList;
}

PIO_RESOURCE_REQUIREMENTS_LIST
IopCmResourcesToIoResources (
    IN ULONG SlotNumber,
    IN PCM_RESOURCE_LIST CmResourceList
    )

/*++

Routine Description:

    This routines converts the input CmResourceList to IO_RESOURCE_REQUIREMENTS_LIST.
    Note, the SlotNumber field of the returned list is not initialized.  Caller must
    fill in the correct value.

Arguments:

    SlotNumber - supplies the SlotNumber the resources refer to.

    CmResourceList - the cm resource list to convert.

Return Value:

    returns a IO_RESOURCE_REQUIREMENTS_LISTST if succeeds.  Otherwise a NULL value is
    returned.

--*/
{
    PIO_RESOURCE_REQUIREMENTS_LIST ioResReqList;
    ULONG count = 0, size, i, j;
    PCM_FULL_RESOURCE_DESCRIPTOR cmFullDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmPartDesc;
    PIO_RESOURCE_DESCRIPTOR ioDesc;

    //
    // First determine number of descriptors required.
    //

    cmFullDesc = &CmResourceList->List[0];
    for (i = 0; i < CmResourceList->Count; i++) {
        count += cmFullDesc->PartialResourceList.Count;
        cmPartDesc = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
        for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {
            size = 0;
            switch (cmPartDesc->Type) {
            case CmResourceTypeDeviceSpecific:
                 size = cmPartDesc->u.DeviceSpecificData.DataSize;
                 break;
            }
            cmPartDesc++;
            cmPartDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmPartDesc + size);
        }
        cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmPartDesc;
    }

    //
    // Allocate heap space for IO RESOURCE REQUIREMENTS LIST
    //

    ioResReqList = (PIO_RESOURCE_REQUIREMENTS_LIST)ExAllocatePool(
                       PagedPool,
                       sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
                           count * sizeof(IO_RESOURCE_DESCRIPTOR)
                       );
    if (!ioResReqList) {
        return NULL;
    }

    //
    // Parse the cm resource descriptor and build its corresponding IO resource descriptor
    //

    ioResReqList->InterfaceType = CmResourceList->List[0].InterfaceType;
    ioResReqList->BusNumber = CmResourceList->List[0].BusNumber;
    ioResReqList->SlotNumber = SlotNumber;
    ioResReqList->Reserved[0] = 0;
    ioResReqList->Reserved[1] = 0;
    ioResReqList->Reserved[2] = 0;
    ioResReqList->AlternativeLists = 1;
    ioResReqList->List[0].Version = 1;
    ioResReqList->List[0].Revision = 1;
    ioResReqList->List[0].Count = count;

    ioDesc = &ioResReqList->List[0].Descriptors[0];
    cmFullDesc = &CmResourceList->List[0];
    for (i = 0; i < CmResourceList->Count; i++) {
        cmPartDesc = &cmFullDesc->PartialResourceList.PartialDescriptors[0];
        for (j = 0; j < cmFullDesc->PartialResourceList.Count; j++) {
            ioDesc->Option = IO_RESOURCE_PREFERRED;
            ioDesc->Type = cmPartDesc->Type;
            ioDesc->ShareDisposition = cmPartDesc->ShareDisposition;
            ioDesc->Flags = cmPartDesc->Flags;
            ioDesc->Spare1 = 0;
            ioDesc->Spare2 = 0;

            size = 0;
            switch (cmPartDesc->Type) {
            case CmResourceTypePort:
                 ioDesc->u.Port.MinimumAddress = cmPartDesc->u.Port.Start;
                 ioDesc->u.Port.MaximumAddress.QuadPart = cmPartDesc->u.Port.Start.QuadPart +
                                                             cmPartDesc->u.Port.Length - 1;
                 ioDesc->u.Port.Alignment = 1;
                 ioDesc->u.Port.Length = cmPartDesc->u.Port.Length;
                 ioDesc++;
                 break;
            case CmResourceTypeInterrupt:
                 ioDesc->u.Interrupt.MinimumVector = cmPartDesc->u.Interrupt.Vector;
                 ioDesc->u.Interrupt.MaximumVector = cmPartDesc->u.Interrupt.Vector;
                 ioDesc++;
                 break;
            case CmResourceTypeMemory:
                 ioDesc->u.Memory.MinimumAddress = cmPartDesc->u.Memory.Start;
                 ioDesc->u.Memory.MaximumAddress.QuadPart = cmPartDesc->u.Memory.Start.QuadPart +
                                                               cmPartDesc->u.Memory.Length - 1;
                 ioDesc->u.Memory.Alignment = 1;
                 ioDesc->u.Memory.Length = cmPartDesc->u.Memory.Length;
                 ioDesc++;
                 break;
            case CmResourceTypeDma:
                 ioDesc->u.Dma.MinimumChannel = cmPartDesc->u.Dma.Channel;
                 ioDesc->u.Dma.MaximumChannel = cmPartDesc->u.Dma.Channel;
                 ioDesc++;
                 break;
            case CmResourceTypeDeviceSpecific:
                 size = cmPartDesc->u.DeviceSpecificData.DataSize;
                 break;
            }
            cmPartDesc++;
            cmPartDesc = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) ((PUCHAR)cmPartDesc + size);
        }
        cmFullDesc = (PCM_FULL_RESOURCE_DESCRIPTOR)cmPartDesc;
    }
    ioResReqList->ListSize = (ULONG)ioDesc - (ULONG)ioResReqList;
    return ioResReqList;
}

NTSTATUS
IopAdjustResourceListRange (
    IN PSUPPORTED_RANGES                    SupportedRanges,
    IN PSUPPORTED_RANGE                     InterruptRange,
    IN PIO_RESOURCE_REQUIREMENTS_LIST       IoResourceList,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *ReturnedList
    )
/*++

Routine Description:

    This functions takes an IO_RESOURCE_REQUIREMENT_LIST and
    adjusts it such that all ranges in the list fit in the
    ranges specified by SupportedRanges & InterruptRange.

    This function is used by some HALs to clip the possible
    settings to be contained on what the particular bus supports
    in reponse to a HalAdjustResourceList call.

Arguments:

    SupportedRanges - Valid IO, Memory, Prefetch Memory, and DMA ranges.
    InterruptRange  - Valid InterruptRanges

    pResourceList   - The resource requirements list which needs to
                      be adjusted to only contain the ranges as
                      described by SupportedRanges & InterruptRange.

Return Value:

    STATUS_SUCCESS or an appropiate error return.

--*/
{
    PIO_RESOURCE_REQUIREMENTS_LIST  InCompleteList, OutCompleteList;
    PIO_RESOURCE_LIST               InResourceList, OutResourceList;
    PIO_RESOURCE_DESCRIPTOR         HeadOutDesc, SetDesc;
    NRPARAMS                        Pos;
    ULONG                           len, alt, cnt, i;
    ULONG                           icnt, ListCount;
    NTSTATUS                        status;

    //
    // Sanity check
    //

    if (!SupportedRanges  ||  SupportedRanges->Version != BUS_SUPPORTED_RANGE_VERSION) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // If SupportedRanges aren't sorted, sort them and get the
    // number of ranges for each type
    //

    if (!SupportedRanges->Sorted) {
        SupportedRanges->NoIO = IopSortRanges (&SupportedRanges->IO);
        SupportedRanges->NoMemory = IopSortRanges (&SupportedRanges->Memory);
        SupportedRanges->NoPrefetchMemory = IopSortRanges (&SupportedRanges->PrefetchMemory);
        SupportedRanges->NoDma = IopSortRanges (&SupportedRanges->Dma);
        SupportedRanges->Sorted = TRUE;
    }

    icnt = IopSortRanges (InterruptRange);

    InCompleteList = IoResourceList;
    len = InCompleteList->ListSize;

    //
    // Scan input list - verify revision #'s, and increase len varible
    // by amount output list may increase.
    //

    i = 0;
    InResourceList = InCompleteList->List;
    for (alt=0; alt < InCompleteList->AlternativeLists; alt++) {
        if (InResourceList->Version != 1 || InResourceList->Revision < 1) {
            return STATUS_INVALID_PARAMETER;
        }

        Pos.InDesc  = InResourceList->Descriptors;
        for (cnt = InResourceList->Count; cnt; cnt--) {
            switch (Pos.InDesc->Type) {
                case CmResourceTypeInterrupt: i += icnt;                   break;
                case CmResourceTypePort:      i += SupportedRanges->NoIO;  break;
                case CmResourceTypeDma:       i += SupportedRanges->NoDma; break;

                case CmResourceTypeMemory:
                    i += SupportedRanges->NoMemory;
                    if (Pos.InDesc->Flags & CM_RESOURCE_MEMORY_PREFETCHABLE) {
                        i += SupportedRanges->NoPrefetchMemory;
                    }
                    break;

                default:
                    return STATUS_INVALID_PARAMETER;
            }

            // take one off for the original which is already accounted for in 'len'
            i -= 1;

            // Next descriptor
            Pos.InDesc++;
        }

        // Next Resource List
        InResourceList  = (PIO_RESOURCE_LIST) Pos.InDesc;
    }
    len += i * sizeof (IO_RESOURCE_DESCRIPTOR);

    //
    // Allocate output list
    //

    OutCompleteList = (PIO_RESOURCE_REQUIREMENTS_LIST)
                            ExAllocatePool (PagedPool, len);

    if (!OutCompleteList) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory (OutCompleteList, len);

    //
    // Walk each ResourceList and build output structure
    //

    InResourceList   = InCompleteList->List;
    *OutCompleteList = *InCompleteList;
    OutResourceList  = OutCompleteList->List;
    ListCount = InCompleteList->AlternativeLists;

    for (alt=0; alt < InCompleteList->AlternativeLists; alt++) {
        OutResourceList->Version  = 1;
        OutResourceList->Revision = 1;

        Pos.InDesc  = InResourceList->Descriptors;
        Pos.OutDesc = OutResourceList->Descriptors;
        HeadOutDesc = Pos.OutDesc;

        for (cnt = InResourceList->Count; cnt; cnt--) {

            //
            // Limit desctiptor to be with the buses supported ranges
            //

            Pos.DescOpt = Pos.InDesc->Option | IO_RESOURCE_PREFERRED;
            Pos.AnotherListPending = FALSE;

            switch (Pos.InDesc->Type) {
                case CmResourceTypePort:

                    //
                    // Get supported IO ranges
                    //

                    Pos.CurrentPosition = &SupportedRanges->IO;
                    do {
                        status = IopGetNextSupportedRange (
                                    Pos.InDesc->u.Port.MinimumAddress.QuadPart,
                                    Pos.InDesc->u.Port.MaximumAddress.QuadPart,
                                    &Pos,
                                    &SetDesc
                                    );
                        if (NT_SUCCESS(status)) {
                            if (SetDesc) {
                                SetDesc->u.Port.MinimumAddress.QuadPart = Pos.Base;
                                SetDesc->u.Port.MaximumAddress.QuadPart = Pos.Limit;
                            }
                        } else {
                            goto skipCurrentList;
                        }
                    } while (SetDesc) ;
                    break;

                case CmResourceTypeInterrupt:
                    //
                    // Get supported Interrupt ranges
                    //

                    Pos.CurrentPosition = InterruptRange;
                    do {
                        status = IopGetNextSupportedRange (
                                    Pos.InDesc->u.Interrupt.MinimumVector,
                                    Pos.InDesc->u.Interrupt.MaximumVector,
                                    &Pos,
                                    &SetDesc
                                    );

                        if (NT_SUCCESS(status)) {
                            if (SetDesc) {
                                SetDesc->u.Interrupt.MinimumVector = (ULONG) Pos.Base;
                                SetDesc->u.Interrupt.MaximumVector = (ULONG) Pos.Limit;
                            }
                        } else {
                            goto skipCurrentList;
                        }
                    } while (SetDesc) ;
                    break;

                case CmResourceTypeMemory:
                    //
                    // Get supported memory ranges
                    //

                    if (Pos.InDesc->Flags & CM_RESOURCE_MEMORY_PREFETCHABLE) {

                        //
                        // This is a Prefetchable range.
                        // First add in any supported prefetchable ranges, then
                        // add in any regualer supported ranges
                        //

                        Pos.AnotherListPending = TRUE;
                        Pos.CurrentPosition = &SupportedRanges->PrefetchMemory;

                        do {
                            status = IopGetNextSupportedRange (
                                        Pos.InDesc->u.Memory.MinimumAddress.QuadPart,
                                        Pos.InDesc->u.Memory.MaximumAddress.QuadPart,
                                        &Pos,
                                        &SetDesc
                                        );

                            if (NT_SUCCESS(status)) {
                                if (SetDesc) {
                                    SetDesc->u.Memory.MinimumAddress.QuadPart = Pos.Base;
                                    SetDesc->u.Memory.MaximumAddress.QuadPart = Pos.Limit;
                                    SetDesc->Option |= IO_RESOURCE_PREFERRED;
                                }
                            } else {
                                goto skipCurrentList;
                            }
                        } while (SetDesc) ;

                        Pos.AnotherListPending = FALSE;
                    }

                    //
                    // Add in supported bus memory ranges
                    //

                    Pos.CurrentPosition = &SupportedRanges->Memory;
                    do {
                        status = IopGetNextSupportedRange (
                                        Pos.InDesc->u.Memory.MinimumAddress.QuadPart,
                                        Pos.InDesc->u.Memory.MaximumAddress.QuadPart,
                                        &Pos,
                                        &SetDesc
                                        );
                        if (NT_SUCCESS(status)) {
                            if (SetDesc) {
                                SetDesc->u.Memory.MinimumAddress.QuadPart = Pos.Base;
                                SetDesc->u.Memory.MaximumAddress.QuadPart = Pos.Limit;
                            }
                        } else {
                            goto skipCurrentList;
                        }
                    } while (SetDesc);
                    break;

                case CmResourceTypeDma:
                    //
                    // Get supported DMA ranges
                    //

                    Pos.CurrentPosition = &SupportedRanges->Dma;
                    do {
                        status = IopGetNextSupportedRange (
                                    Pos.InDesc->u.Dma.MinimumChannel,
                                    Pos.InDesc->u.Dma.MaximumChannel,
                                    &Pos,
                                    &SetDesc
                                    );

                        if (NT_SUCCESS(status)) {
                            if (SetDesc) {
                                SetDesc->u.Dma.MinimumChannel = (ULONG) Pos.Base;
                                SetDesc->u.Dma.MaximumChannel = (ULONG) Pos.Limit;
                            }
                         } else {
                             goto skipCurrentList;
                         }
                    } while (SetDesc) ;
                    break;

#if DBG
                default:
                    DbgPrint ("HalAdjustResourceList: Unkown resource type\n");
                    break;
#endif
            }

            //
            // Next descriptor
            //

            Pos.InDesc++;
        }

        OutResourceList->Count = Pos.OutDesc - HeadOutDesc;

        //
        // Next Resource List
        //

        InResourceList  = (PIO_RESOURCE_LIST) Pos.InDesc;
        OutResourceList = (PIO_RESOURCE_LIST) Pos.OutDesc;
        continue;

skipCurrentList:
        InResourceList = (PIO_RESOURCE_LIST) (InResourceList->Descriptors + InResourceList->Count);
        ListCount--;
    }

    //
    // Return output list
    //

    if (ListCount == 0) {
        ExFreePool(OutCompleteList);
        return STATUS_UNSUCCESSFUL;
    } else {
        OutCompleteList->ListSize = (ULONG) ((PUCHAR) OutResourceList - (PUCHAR) OutCompleteList);
        OutCompleteList->AlternativeLists = ListCount;
        *ReturnedList = OutCompleteList;
        return STATUS_SUCCESS;
    }
}

NTSTATUS
IopGetNextSupportedRange (
    IN LONGLONG MinimumAddress,
    IN LONGLONG MaximumAddress,
    IN OUT PNRPARAMS Pos,
    OUT PIO_RESOURCE_DESCRIPTOR *IoDescriptor
    )
/*++

Routine Description:

    Support function for IopAdjustResourceListRange.
    Returns the next supported range in the area passed in.

Arguments:

    MinimumAddress
    MaximumAddress  - Min & Max address of a range which needs
                      to be clipped to match that of the supported
                      ranges of the current bus.

    Pos             - describes the current postion

    IoDescriptor    - returns the adjusted resource descriptor
                      NULL is no more returned ranges

Return Value:

    Otherwise, the IO_RESOURCE_DESCRIPTOR which needs to be set
    with the matching range returned in Pos.

--*/
{
    LONGLONG Base, Limit;

    //
    // Find next range which is supported
    //

    Base  = MinimumAddress;
    Limit = MaximumAddress;

    while (Pos->CurrentPosition) {
        Pos->Base  = Base;
        Pos->Limit = Limit;

        //
        // Clip to current range
        //

        if (Pos->Base < Pos->CurrentPosition->Base) {
            Pos->Base = Pos->CurrentPosition->Base;
        }

        if (Pos->Limit > Pos->CurrentPosition->Limit) {
            Pos->Limit = Pos->CurrentPosition->Limit;
        }

        //
        // set position to next range
        //

        Pos->CurrentPosition = Pos->CurrentPosition->Next;

        //
        // If valid range, return it
        //

        if (Pos->Base <= Pos->Limit) {
            *Pos->OutDesc = *Pos->InDesc;
            Pos->OutDesc->Option = Pos->DescOpt;

            //
            // next descriptor (if any) is an alternative
            // to the descriptor being returned now
            //

            Pos->OutDesc += 1;
            Pos->DescOpt |= IO_RESOURCE_ALTERNATIVE;
            *IoDescriptor = Pos->OutDesc - 1;
            return STATUS_SUCCESS;
        }
    }

    //
    // There's no overlapping range.  If this descriptor is
    // not an alternative and this descriptor is not going to
    // be processed by another range list, then return
    // failure.
    //

    if (!(Pos->DescOpt & IO_RESOURCE_ALTERNATIVE) &&
        Pos->AnotherListPending == FALSE) {

        //
        // return a bogus descriptor
        //

        Pos->Base  = MinimumAddress;
        Pos->Limit = Pos->Base - 1;
        if (Pos->Base == 0) {       // if wrapped, fix it
            Pos->Base  = 1;
            Pos->Limit = 0;
        }

        *Pos->OutDesc = *Pos->InDesc;
        Pos->OutDesc->Option = Pos->DescOpt;

        Pos->OutDesc += 1;
        Pos->DescOpt |= IO_RESOURCE_ALTERNATIVE;
        *IoDescriptor = Pos->OutDesc - 1;
        return STATUS_SUCCESS;
    }

    //
    // No range found (or no more ranges)
    //

    *IoDescriptor = NULL;
    return STATUS_SUCCESS;
}

ULONG
IopSortRanges (
    IN PSUPPORTED_RANGE RangeList
    )
/*++

Routine Description:

    Support function for IopAdjustResourceListRange.
    Sorts a supported range list into decending order.

Arguments:

    pRange  - List to sort

Return Value:

--*/
{
    ULONG               cnt;
    LONGLONG            hldBase, hldLimit;
    PSUPPORTED_RANGE    Range1, Range2;

    //
    // Sort it
    //

    for (Range1 = RangeList; Range1; Range1 = Range1->Next) {
        for (Range2 = Range1->Next; Range2; Range2 = Range2->Next) {

            if (Range2->Base > Range1->Base) {
                hldBase  = Range1->Base;
                hldLimit = Range1->Limit;

                Range1->Base  = Range2->Base;
                Range1->Limit = Range2->Limit;

                Range2->Base  = hldBase;
                Range2->Limit = hldLimit;
            }
        }
    }

    //
    // Count the number of ranges
    //

    cnt = 0;
    for (Range1 = RangeList; Range1; Range1 = Range1->Next) {
        cnt += 1;
    }

    return cnt;
}
#endif // _PNP_POWER_
