/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    WdfCollection.h

Abstract:

    This is the interface to the collection object

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _WDFCOLLECTION_H_
#define _WDFCOLLECTION_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)



//
// WDF Function: WdfCollectionCreate
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFCOLLECTIONCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES CollectionAttributes,
    __out
    WDFCOLLECTION* Collection
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfCollectionCreate(
    __in_opt
    PWDF_OBJECT_ATTRIBUTES CollectionAttributes,
    __out
    WDFCOLLECTION* Collection
    )
{
    return ((PFN_WDFCOLLECTIONCREATE) WdfFunctions[WdfCollectionCreateTableIndex])(WdfDriverGlobals, CollectionAttributes, Collection);
}

//
// WDF Function: WdfCollectionGetCount
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
ULONG
(*PFN_WDFCOLLECTIONGETCOUNT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection
    );

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
FORCEINLINE
WdfCollectionGetCount(
    __in
    WDFCOLLECTION Collection
    )
{
    return ((PFN_WDFCOLLECTIONGETCOUNT) WdfFunctions[WdfCollectionGetCountTableIndex])(WdfDriverGlobals, Collection);
}

//
// WDF Function: WdfCollectionAdd
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFCOLLECTIONADD)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection,
    __in
    WDFOBJECT Object
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfCollectionAdd(
    __in
    WDFCOLLECTION Collection,
    __in
    WDFOBJECT Object
    )
{
    return ((PFN_WDFCOLLECTIONADD) WdfFunctions[WdfCollectionAddTableIndex])(WdfDriverGlobals, Collection, Object);
}

//
// WDF Function: WdfCollectionRemove
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFCOLLECTIONREMOVE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection,
    __in
    WDFOBJECT Item
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfCollectionRemove(
    __in
    WDFCOLLECTION Collection,
    __in
    WDFOBJECT Item
    )
{
    ((PFN_WDFCOLLECTIONREMOVE) WdfFunctions[WdfCollectionRemoveTableIndex])(WdfDriverGlobals, Collection, Item);
}

//
// WDF Function: WdfCollectionRemoveItem
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFCOLLECTIONREMOVEITEM)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection,
    __in
    ULONG Index
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfCollectionRemoveItem(
    __in
    WDFCOLLECTION Collection,
    __in
    ULONG Index
    )
{
    ((PFN_WDFCOLLECTIONREMOVEITEM) WdfFunctions[WdfCollectionRemoveItemTableIndex])(WdfDriverGlobals, Collection, Index);
}

//
// WDF Function: WdfCollectionGetItem
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
(*PFN_WDFCOLLECTIONGETITEM)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection,
    __in
    ULONG Index
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
FORCEINLINE
WdfCollectionGetItem(
    __in
    WDFCOLLECTION Collection,
    __in
    ULONG Index
    )
{
    return ((PFN_WDFCOLLECTIONGETITEM) WdfFunctions[WdfCollectionGetItemTableIndex])(WdfDriverGlobals, Collection, Index);
}

//
// WDF Function: WdfCollectionGetFirstItem
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
(*PFN_WDFCOLLECTIONGETFIRSTITEM)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
FORCEINLINE
WdfCollectionGetFirstItem(
    __in
    WDFCOLLECTION Collection
    )
{
    return ((PFN_WDFCOLLECTIONGETFIRSTITEM) WdfFunctions[WdfCollectionGetFirstItemTableIndex])(WdfDriverGlobals, Collection);
}

//
// WDF Function: WdfCollectionGetLastItem
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
(*PFN_WDFCOLLECTIONGETLASTITEM)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
FORCEINLINE
WdfCollectionGetLastItem(
    __in
    WDFCOLLECTION Collection
    )
{
    return ((PFN_WDFCOLLECTIONGETLASTITEM) WdfFunctions[WdfCollectionGetLastItemTableIndex])(WdfDriverGlobals, Collection);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFCOLLECTION_H_

