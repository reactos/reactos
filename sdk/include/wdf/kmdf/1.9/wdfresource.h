/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    wdfresource.h

Abstract:

    This defines the DDIs for hardware resources

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _WDFRESOURCE_H_
#define _WDFRESOURCE_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)

#define WDF_INSERT_AT_END ((ULONG) -1)



//
// WDF Function: WdfIoResourceRequirementsListSetSlotNumber
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFIORESOURCEREQUIREMENTSLISTSETSLOTNUMBER)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    ULONG SlotNumber
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfIoResourceRequirementsListSetSlotNumber(
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    ULONG SlotNumber
    )
{
    ((PFN_WDFIORESOURCEREQUIREMENTSLISTSETSLOTNUMBER) WdfFunctions[WdfIoResourceRequirementsListSetSlotNumberTableIndex])(WdfDriverGlobals, RequirementsList, SlotNumber);
}

//
// WDF Function: WdfIoResourceRequirementsListSetInterfaceType
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFIORESOURCEREQUIREMENTSLISTSETINTERFACETYPE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    __drv_strictTypeMatch(__drv_typeCond)
    INTERFACE_TYPE InterfaceType
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfIoResourceRequirementsListSetInterfaceType(
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    __drv_strictTypeMatch(__drv_typeCond)
    INTERFACE_TYPE InterfaceType
    )
{
    ((PFN_WDFIORESOURCEREQUIREMENTSLISTSETINTERFACETYPE) WdfFunctions[WdfIoResourceRequirementsListSetInterfaceTypeTableIndex])(WdfDriverGlobals, RequirementsList, InterfaceType);
}

//
// WDF Function: WdfIoResourceRequirementsListAppendIoResList
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFIORESOURCEREQUIREMENTSLISTAPPENDIORESLIST)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    WDFIORESLIST IoResList
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfIoResourceRequirementsListAppendIoResList(
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    WDFIORESLIST IoResList
    )
{
    return ((PFN_WDFIORESOURCEREQUIREMENTSLISTAPPENDIORESLIST) WdfFunctions[WdfIoResourceRequirementsListAppendIoResListTableIndex])(WdfDriverGlobals, RequirementsList, IoResList);
}

//
// WDF Function: WdfIoResourceRequirementsListInsertIoResList
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFIORESOURCEREQUIREMENTSLISTINSERTIORESLIST)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    WDFIORESLIST IoResList,
    __in
    ULONG Index
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfIoResourceRequirementsListInsertIoResList(
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    WDFIORESLIST IoResList,
    __in
    ULONG Index
    )
{
    return ((PFN_WDFIORESOURCEREQUIREMENTSLISTINSERTIORESLIST) WdfFunctions[WdfIoResourceRequirementsListInsertIoResListTableIndex])(WdfDriverGlobals, RequirementsList, IoResList, Index);
}

//
// WDF Function: WdfIoResourceRequirementsListGetCount
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
ULONG
(*PFN_WDFIORESOURCEREQUIREMENTSLISTGETCOUNT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList
    );

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
FORCEINLINE
WdfIoResourceRequirementsListGetCount(
    __in
    WDFIORESREQLIST RequirementsList
    )
{
    return ((PFN_WDFIORESOURCEREQUIREMENTSLISTGETCOUNT) WdfFunctions[WdfIoResourceRequirementsListGetCountTableIndex])(WdfDriverGlobals, RequirementsList);
}

//
// WDF Function: WdfIoResourceRequirementsListGetIoResList
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFIORESLIST
(*PFN_WDFIORESOURCEREQUIREMENTSLISTGETIORESLIST)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    ULONG Index
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFIORESLIST
FORCEINLINE
WdfIoResourceRequirementsListGetIoResList(
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    ULONG Index
    )
{
    return ((PFN_WDFIORESOURCEREQUIREMENTSLISTGETIORESLIST) WdfFunctions[WdfIoResourceRequirementsListGetIoResListTableIndex])(WdfDriverGlobals, RequirementsList, Index);
}

//
// WDF Function: WdfIoResourceRequirementsListRemove
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFIORESOURCEREQUIREMENTSLISTREMOVE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    ULONG Index
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfIoResourceRequirementsListRemove(
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    ULONG Index
    )
{
    ((PFN_WDFIORESOURCEREQUIREMENTSLISTREMOVE) WdfFunctions[WdfIoResourceRequirementsListRemoveTableIndex])(WdfDriverGlobals, RequirementsList, Index);
}

//
// WDF Function: WdfIoResourceRequirementsListRemoveByIoResList
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFIORESOURCEREQUIREMENTSLISTREMOVEBYIORESLIST)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    WDFIORESLIST IoResList
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfIoResourceRequirementsListRemoveByIoResList(
    __in
    WDFIORESREQLIST RequirementsList,
    __in
    WDFIORESLIST IoResList
    )
{
    ((PFN_WDFIORESOURCEREQUIREMENTSLISTREMOVEBYIORESLIST) WdfFunctions[WdfIoResourceRequirementsListRemoveByIoResListTableIndex])(WdfDriverGlobals, RequirementsList, IoResList);
}

//
// WDF Function: WdfIoResourceListCreate
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFIORESOURCELISTCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESREQLIST RequirementsList,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFIORESLIST* ResourceList
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfIoResourceListCreate(
    __in
    WDFIORESREQLIST RequirementsList,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFIORESLIST* ResourceList
    )
{
    return ((PFN_WDFIORESOURCELISTCREATE) WdfFunctions[WdfIoResourceListCreateTableIndex])(WdfDriverGlobals, RequirementsList, Attributes, ResourceList);
}

//
// WDF Function: WdfIoResourceListAppendDescriptor
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFIORESOURCELISTAPPENDDESCRIPTOR)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfIoResourceListAppendDescriptor(
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor
    )
{
    return ((PFN_WDFIORESOURCELISTAPPENDDESCRIPTOR) WdfFunctions[WdfIoResourceListAppendDescriptorTableIndex])(WdfDriverGlobals, ResourceList, Descriptor);
}

//
// WDF Function: WdfIoResourceListInsertDescriptor
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFIORESOURCELISTINSERTDESCRIPTOR)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor,
    __in
    ULONG Index
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfIoResourceListInsertDescriptor(
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor,
    __in
    ULONG Index
    )
{
    return ((PFN_WDFIORESOURCELISTINSERTDESCRIPTOR) WdfFunctions[WdfIoResourceListInsertDescriptorTableIndex])(WdfDriverGlobals, ResourceList, Descriptor, Index);
}

//
// WDF Function: WdfIoResourceListUpdateDescriptor
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFIORESOURCELISTUPDATEDESCRIPTOR)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor,
    __in
    ULONG Index
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfIoResourceListUpdateDescriptor(
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor,
    __in
    ULONG Index
    )
{
    ((PFN_WDFIORESOURCELISTUPDATEDESCRIPTOR) WdfFunctions[WdfIoResourceListUpdateDescriptorTableIndex])(WdfDriverGlobals, ResourceList, Descriptor, Index);
}

//
// WDF Function: WdfIoResourceListGetCount
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
ULONG
(*PFN_WDFIORESOURCELISTGETCOUNT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList
    );

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
FORCEINLINE
WdfIoResourceListGetCount(
    __in
    WDFIORESLIST ResourceList
    )
{
    return ((PFN_WDFIORESOURCELISTGETCOUNT) WdfFunctions[WdfIoResourceListGetCountTableIndex])(WdfDriverGlobals, ResourceList);
}

//
// WDF Function: WdfIoResourceListGetDescriptor
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
PIO_RESOURCE_DESCRIPTOR
(*PFN_WDFIORESOURCELISTGETDESCRIPTOR)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    ULONG Index
    );

__drv_maxIRQL(DISPATCH_LEVEL)
PIO_RESOURCE_DESCRIPTOR
FORCEINLINE
WdfIoResourceListGetDescriptor(
    __in
    WDFIORESLIST ResourceList,
    __in
    ULONG Index
    )
{
    return ((PFN_WDFIORESOURCELISTGETDESCRIPTOR) WdfFunctions[WdfIoResourceListGetDescriptorTableIndex])(WdfDriverGlobals, ResourceList, Index);
}

//
// WDF Function: WdfIoResourceListRemove
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFIORESOURCELISTREMOVE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    ULONG Index
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfIoResourceListRemove(
    __in
    WDFIORESLIST ResourceList,
    __in
    ULONG Index
    )
{
    ((PFN_WDFIORESOURCELISTREMOVE) WdfFunctions[WdfIoResourceListRemoveTableIndex])(WdfDriverGlobals, ResourceList, Index);
}

//
// WDF Function: WdfIoResourceListRemoveByDescriptor
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFIORESOURCELISTREMOVEBYDESCRIPTOR)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfIoResourceListRemoveByDescriptor(
    __in
    WDFIORESLIST ResourceList,
    __in
    PIO_RESOURCE_DESCRIPTOR Descriptor
    )
{
    ((PFN_WDFIORESOURCELISTREMOVEBYDESCRIPTOR) WdfFunctions[WdfIoResourceListRemoveByDescriptorTableIndex])(WdfDriverGlobals, ResourceList, Descriptor);
}

//
// WDF Function: WdfCmResourceListAppendDescriptor
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFCMRESOURCELISTAPPENDDESCRIPTOR)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfCmResourceListAppendDescriptor(
    __in
    WDFCMRESLIST List,
    __in
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    )
{
    return ((PFN_WDFCMRESOURCELISTAPPENDDESCRIPTOR) WdfFunctions[WdfCmResourceListAppendDescriptorTableIndex])(WdfDriverGlobals, List, Descriptor);
}

//
// WDF Function: WdfCmResourceListInsertDescriptor
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFCMRESOURCELISTINSERTDESCRIPTOR)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    __in
    ULONG Index
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfCmResourceListInsertDescriptor(
    __in
    WDFCMRESLIST List,
    __in
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    __in
    ULONG Index
    )
{
    return ((PFN_WDFCMRESOURCELISTINSERTDESCRIPTOR) WdfFunctions[WdfCmResourceListInsertDescriptorTableIndex])(WdfDriverGlobals, List, Descriptor, Index);
}

//
// WDF Function: WdfCmResourceListGetCount
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
ULONG
(*PFN_WDFCMRESOURCELISTGETCOUNT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List
    );

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
FORCEINLINE
WdfCmResourceListGetCount(
    __in
    WDFCMRESLIST List
    )
{
    return ((PFN_WDFCMRESOURCELISTGETCOUNT) WdfFunctions[WdfCmResourceListGetCountTableIndex])(WdfDriverGlobals, List);
}

//
// WDF Function: WdfCmResourceListGetDescriptor
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
PCM_PARTIAL_RESOURCE_DESCRIPTOR
(*PFN_WDFCMRESOURCELISTGETDESCRIPTOR)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    ULONG Index
    );

__drv_maxIRQL(DISPATCH_LEVEL)
PCM_PARTIAL_RESOURCE_DESCRIPTOR
FORCEINLINE
WdfCmResourceListGetDescriptor(
    __in
    WDFCMRESLIST List,
    __in
    ULONG Index
    )
{
    return ((PFN_WDFCMRESOURCELISTGETDESCRIPTOR) WdfFunctions[WdfCmResourceListGetDescriptorTableIndex])(WdfDriverGlobals, List, Index);
}

//
// WDF Function: WdfCmResourceListRemove
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFCMRESOURCELISTREMOVE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    ULONG Index
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfCmResourceListRemove(
    __in
    WDFCMRESLIST List,
    __in
    ULONG Index
    )
{
    ((PFN_WDFCMRESOURCELISTREMOVE) WdfFunctions[WdfCmResourceListRemoveTableIndex])(WdfDriverGlobals, List, Index);
}

//
// WDF Function: WdfCmResourceListRemoveByDescriptor
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFCMRESOURCELISTREMOVEBYDESCRIPTOR)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCMRESLIST List,
    __in
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfCmResourceListRemoveByDescriptor(
    __in
    WDFCMRESLIST List,
    __in
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    )
{
    ((PFN_WDFCMRESOURCELISTREMOVEBYDESCRIPTOR) WdfFunctions[WdfCmResourceListRemoveByDescriptorTableIndex])(WdfDriverGlobals, List, Descriptor);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFRESOURCE_H_

