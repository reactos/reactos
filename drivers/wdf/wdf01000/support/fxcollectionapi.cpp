#include "wdf.h"


extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfCollectionCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES CollectionAttributes,
    __out
    WDFCOLLECTION *Collection
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
WDFEXPORT(WdfCollectionGetCount)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection
    )
{
    WDFNOTIMPLEMENTED();
    return 0;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfCollectionAdd)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection,
    __in
    WDFOBJECT Object
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfCollectionRemoveItem)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection,
    __in 
    ULONG Index
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfCollectionRemove)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection,
    __in
    WDFOBJECT Item
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
WDFEXPORT(WdfCollectionGetItem)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection,
    __in
    ULONG Index
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
WDFEXPORT(WdfCollectionGetFirstItem)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
WDFEXPORT(WdfCollectionGetLastItem)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCOLLECTION Collection
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

} // extern "C"
