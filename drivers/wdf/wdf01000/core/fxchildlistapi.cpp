#include "wdf.h"


extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfChildListCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_CHILD_LIST_CONFIG Config,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES DeviceListAttributes,
    __out
    WDFCHILDLIST* DeviceList
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
WDFEXPORT(WdfChildListGetDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfChildListRetrieveAddressDescription)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __in
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    __inout
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfChildListBeginScan)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfChildListEndScan)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfChildListBeginIteration)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __in
    PWDF_CHILD_LIST_ITERATOR Iterator
    )
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfChildListRetrieveNextDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __in
    PWDF_CHILD_LIST_ITERATOR Iterator,
    __out
    WDFDEVICE* Device,
    __inout_opt
    PWDF_CHILD_RETRIEVE_INFO Info
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfChildListEndIteration)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __in
    PWDF_CHILD_LIST_ITERATOR Iterator
    )
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfChildListAddOrUpdateChildDescriptionAsPresent)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __in
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    __in_opt
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfChildListUpdateChildDescriptionAsMissing)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __in
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfChildListUpdateAllChildDescriptionsAsPresent)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList
    )
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
WDFEXPORT(WdfChildListRetrievePdo)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __inout
    PWDF_CHILD_RETRIEVE_INFO RetrieveInfo
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFEXPORT(WdfChildListRequestChildEject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFCHILDLIST DeviceList,
    __in
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    )
{
    WDFNOTIMPLEMENTED();
    return FALSE;
}



} // extern "C"
