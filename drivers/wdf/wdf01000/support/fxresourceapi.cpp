/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Resource api functions
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "wdf.h"



extern "C" {

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoResourceRequirementsListSetSlotNumber)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    ULONG SlotNumber
    )
/*++

Routine Description:
    Sets the slot number for a given resource requirements list

Arguments:
    RequirementsList - list to be modified

    SlotNumber - slot value to assign

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoResourceRequirementsListSetInterfaceType)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    __drv_strictTypeMatch(__drv_typeCond)    
    INTERFACE_TYPE InterfaceType
    )
/*++

Routine Description:
    Sets the InterfaceType for a given resource requirements list

Arguments:
    RequirementsList - list to be modified

    InterfaceType - interface type to assign

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoResourceRequirementsListInsertIoResList)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    WDFIORESLIST IoResList,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Inserts a resource list into a requirements list at a particular index.

Arguments:
    RequirementsList - list to be modified

    IoResList - resource list to add

    Index - zero based index to insert at

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoResourceRequirementsListAppendIoResList)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    WDFIORESLIST IoResList
    )
/*++

Routine Description:
   Appends a resource list to a resource requirements list

Arguments:
    RequirementsList - list to be modified

    IoResList - resource list to append

Return Value:
    NTSTATUS

  --*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}


__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
WDFEXPORT(WdfIoResourceRequirementsListGetCount)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList
    )
/*++

Routine Description:
    Returns the number of resource lists in the requirements list


Arguments:
    RequirementsList - requirements list whose count will be returned

Return Value:
    number of elements in the list

  --*/

{
    WDFNOTIMPLEMENTED();
    return 0;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFIORESLIST
WDFEXPORT(WdfIoResourceRequirementsListGetIoResList)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Retrieves a resource list from the requirements list at a given index.

Arguments:
    RequirementsList - list to retrieve the resource list from

    Index - zero based index from which to retrieve the list

Return Value:
    resource list handle or NULL

  --*/
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoResourceRequirementsListRemove)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Removes a resource list from the requirements list at a given index

Arguments:
    RequirementsList - list of resource requirements which will be modified

    Index - zero based index which indictes location in the list to find the
            resource list

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoResourceRequirementsListRemoveByIoResList)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    WDFIORESLIST IoResList
    )
/*++

Routine Description:
    Removes a resource list from the requirements list based on the resource list's
    handle

Arguments:
    RequirementsList - resource requirements list being modified

    IoResList - resource list to be removed

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoResourceListCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFIORESLIST* ResourceList
    )
/*++

Routine Description:
   Creates a resource list.

Arguments:
    RequirementsList - the resource requirements list that the resource list will
                       be associated with

    Attributes - generic object attributes for the new resource list

    ResourceList - pointer which will receive the new object handle

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoResourceListInsertDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Inserts a descriptor into a resource list at a particular index.

Arguments:
    ResourceList - list to be modified

    Descriptor - descriptor to insert

    Index - zero based index to insert at

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoResourceListAppendDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor
    )
/*++

Routine Description:
   Appends a descriptor to a resource list

Arguments:
    ResourceList - list to be modified

    Descriptor - item to be appended

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoResourceListUpdateDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Updates resource requirement in place in the list.

Arguments:
    ResourceList - list to be modified

    Descriptor - Pointer to descriptor whic contains the updated value

    Index - zero based location in the list to update

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
WDFEXPORT(WdfIoResourceListGetCount)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList
    )
/*++

Routine Description:
    Returns the number of descriptors in the resource list

Arguments:
    ResourceList - resource list whose count will be returned

Return Value:
    number of elements in the list

  --*/
{
    WDFNOTIMPLEMENTED();
    return 0;
}


__drv_maxIRQL(DISPATCH_LEVEL)
PIO_RESOURCE_DESCRIPTOR
WDFEXPORT(WdfIoResourceListGetDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Retrieves an io resource desciptor for a given index in the resource list

Arguments:
    ResourceList - list being looked up

    Index - zero based index into the list to find the value of

Return Value:
    pointer to an io resource descriptor upon success, NULL upon error

  --*/

{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoResourceListRemove)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Removes a descriptor in an io resource list

Arguments:
    ResourceList - resource list to modify

    Index - zero based index into the list in which to remove the descriptor

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoResourceListRemoveByDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor
    )
/*++

Routine Description:
    Removes a descriptor by value in a given io resource list.  Equality is
    determined by RtlCompareMemory.

Arguments:
    ResourceList - the io resource list to modify

    Descriptor - pointer to a descriptor to remove.

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfCmResourceListInsertDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Inserts a descriptor into a cm resource list at a particular index.

Arguments:
    ResourceList - list to be modified

    Descriptor - descriptor to insert

    Index - zero based index to insert at

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfCmResourceListAppendDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    )
/*++

Routine Description:
   Appends a descriptor to a cm resource list

Arguments:
    ResourceList - list to be modified

    Descriptor - item to be appended

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
WDFEXPORT(WdfCmResourceListGetCount)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List
    )
/*++

Routine Description:
    Returns the number of cm descriptors in the resource list

Arguments:
    ResourceList - resource list whose count will be returned

Return Value:
    number of elements in the list

  --*/
{
    WDFNOTIMPLEMENTED();
    return 0;
}


__drv_maxIRQL(DISPATCH_LEVEL)
PCM_PARTIAL_RESOURCE_DESCRIPTOR
WDFEXPORT(WdfCmResourceListGetDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Retrieves a cm resource desciptor for a given index in the resource list

Arguments:
    ResourceList - list being looked up

    Index - zero based index into the list to find the value of

Return Value:
    pointer to a cm resource descriptor upon success, NULL upon error

  --*/
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfCmResourceListRemove)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    ULONG Index
    )
/*++

Routine Description:
    Removes a descriptor in an cm resource list

Arguments:
    ResourceList - resource list to modify

    Index - zero based index into the list in which to remove the descriptor

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfCmResourceListRemoveByDescriptor)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    )
/*++

Routine Description:
    Removes a descriptor by value in a given cm resource list.  Equality is
    determined by RtlCompareMemory.

Arguments:
    ResourceList - the io resource list to modify

    Descriptor - pointer to a descriptor to remove.

Return Value:
    None

  --*/
{
    WDFNOTIMPLEMENTED();
}

} //extern "C"
