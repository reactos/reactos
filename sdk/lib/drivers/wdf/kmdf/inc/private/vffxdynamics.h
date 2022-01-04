/*++

Module Name: VfFxDynamics.h

Abstract:
    Generated header for WDF API Verifier hooks

Environment:
    kernel mode only

    Warning: manual changes to this file will be lost.
--*/

#ifndef _VFFXDYNAMICS_H_
#define _VFFXDYNAMICS_H_


_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfChildListCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_CHILD_LIST_CONFIG Config,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES ChildListAttributes,
    _Out_
    WDFCHILDLIST* ChildList
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfChildListGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfChildListRetrievePdo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _Inout_
    PWDF_CHILD_RETRIEVE_INFO RetrieveInfo
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfChildListRetrieveAddressDescription)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _In_
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    _Inout_
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfChildListBeginScan)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfChildListEndScan)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfChildListBeginIteration)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _In_
    PWDF_CHILD_LIST_ITERATOR Iterator
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfChildListRetrieveNextDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _In_
    PWDF_CHILD_LIST_ITERATOR Iterator,
    _Out_
    WDFDEVICE* Device,
    _Inout_opt_
    PWDF_CHILD_RETRIEVE_INFO Info
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfChildListEndIteration)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _In_
    PWDF_CHILD_LIST_ITERATOR Iterator
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfChildListAddOrUpdateChildDescriptionAsPresent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _In_
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription,
    _In_opt_
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfChildListUpdateChildDescriptionAsMissing)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _In_
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfChildListUpdateAllChildDescriptionsAsPresent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfChildListRequestChildEject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _In_
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfCollectionCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES CollectionAttributes,
    _Out_
    WDFCOLLECTION* Collection
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
VFWDFEXPORT(WdfCollectionGetCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfCollectionAdd)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection,
    _In_
    WDFOBJECT Object
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfCollectionRemove)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection,
    _In_
    WDFOBJECT Item
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfCollectionRemoveItem)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
VFWDFEXPORT(WdfCollectionGetItem)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
VFWDFEXPORT(WdfCollectionGetFirstItem)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
VFWDFEXPORT(WdfCollectionGetLastItem)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfCommonBufferCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler,
    _In_
    _When_(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFCOMMONBUFFER* CommonBuffer
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfCommonBufferCreateWithConfig)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler,
    _In_
    _When_(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length,
    _In_
    PWDF_COMMON_BUFFER_CONFIG Config,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFCOMMONBUFFER* CommonBuffer
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PVOID
VFWDFEXPORT(WdfCommonBufferGetAlignedVirtualAddress)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOMMONBUFFER CommonBuffer
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PHYSICAL_ADDRESS
VFWDFEXPORT(WdfCommonBufferGetAlignedLogicalAddress)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOMMONBUFFER CommonBuffer
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
VFWDFEXPORT(WdfCommonBufferGetLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOMMONBUFFER CommonBuffer
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PWDFDEVICE_INIT
VFWDFEXPORT(WdfControlDeviceInitAllocate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver,
    _In_
    CONST UNICODE_STRING* SDDLString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfControlDeviceInitSetShutdownNotification)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PFN_WDF_DEVICE_SHUTDOWN_NOTIFICATION Notification,
    _In_
    UCHAR Flags
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfControlFinishInitializing)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PWDFCXDEVICE_INIT
VFWDFEXPORT(WdfCxDeviceInitAllocate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfCxDeviceInitAssignWdmIrpPreprocessCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFCXDEVICE_INIT CxDeviceInit,
    _In_
    PFN_WDFCXDEVICE_WDM_IRP_PREPROCESS EvtCxDeviceWdmIrpPreprocess,
    _In_
    UCHAR MajorFunction,
    _When_(NumMinorFunctions > 0, _In_reads_bytes_(NumMinorFunctions))
    _When_(NumMinorFunctions == 0, _In_opt_)
    PUCHAR MinorFunctions,
    _In_
    ULONG NumMinorFunctions
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfCxDeviceInitSetIoInCallerContextCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFCXDEVICE_INIT CxDeviceInit,
    _In_
    PFN_WDF_IO_IN_CALLER_CONTEXT EvtIoInCallerContext
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfCxDeviceInitSetRequestAttributes)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFCXDEVICE_INIT CxDeviceInit,
    _In_
    PWDF_OBJECT_ATTRIBUTES RequestAttributes
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfCxDeviceInitSetFileObjectConfig)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFCXDEVICE_INIT CxDeviceInit,
    _In_
    PWDFCX_FILEOBJECT_CONFIG CxFileObjectConfig,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES FileObjectAttributes
    );

WDFAPI
VOID
VFWDFEXPORT(WdfCxVerifierKeBugCheck)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    WDFOBJECT Object,
    _In_
    ULONG BugCheckCode,
    _In_
    ULONG_PTR BugCheckParameter1,
    _In_
    ULONG_PTR BugCheckParameter2,
    _In_
    ULONG_PTR BugCheckParameter3,
    _In_
    ULONG_PTR BugCheckParameter4
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceGetDeviceState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _Out_
    PWDF_DEVICE_STATE DeviceState
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceSetDeviceState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_STATE DeviceState
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfWdmDeviceGetWdfDeviceHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PDEVICE_OBJECT DeviceObject
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
VFWDFEXPORT(WdfDeviceWdmGetDeviceObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
VFWDFEXPORT(WdfDeviceWdmGetAttachedDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
VFWDFEXPORT(WdfDeviceWdmGetPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceWdmDispatchPreprocessedIrp)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PIRP Irp
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceWdmDispatchIrp)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PIRP Irp,
    _In_
    WDFCONTEXT DispatchContext
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceWdmDispatchIrpToIoQueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PIRP Irp,
    _In_
    WDFQUEUE Queue,
    _In_
    ULONG Flags
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceAddDependentUsageDeviceObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PDEVICE_OBJECT DependentDevice
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceRemoveDependentUsageDeviceObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PDEVICE_OBJECT DependentDevice
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceAddRemovalRelationsPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PDEVICE_OBJECT PhysicalDevice
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceRemoveRemovalRelationsPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PDEVICE_OBJECT PhysicalDevice
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceClearRemovalRelationsDevices)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDRIVER
VFWDFEXPORT(WdfDeviceGetDriver)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceRetrieveDeviceName)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDFSTRING String
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceAssignMofResourceName)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PCUNICODE_STRING MofResourceName
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFIOTARGET
VFWDFEXPORT(WdfDeviceGetIoTarget)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDF_DEVICE_PNP_STATE
VFWDFEXPORT(WdfDeviceGetDevicePnpState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDF_DEVICE_POWER_STATE
VFWDFEXPORT(WdfDeviceGetDevicePowerState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDF_DEVICE_POWER_POLICY_STATE
VFWDFEXPORT(WdfDeviceGetDevicePowerPolicyState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceAssignS0IdleSettings)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_POWER_POLICY_IDLE_SETTINGS Settings
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceAssignSxWakeSettings)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_POWER_POLICY_WAKE_SETTINGS Settings
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceOpenRegistryKey)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    ULONG DeviceInstanceKeyType,
    _In_
    ACCESS_MASK DesiredAccess,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    _Out_
    WDFKEY* Key
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceOpenDevicemapKey)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PCUNICODE_STRING KeyName,
    _In_
    ACCESS_MASK DesiredAccess,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    _Out_
    WDFKEY* Key
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceSetSpecialFileSupport)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDF_SPECIAL_FILE_TYPE FileType,
    _In_
    BOOLEAN FileTypeIsSupported
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceSetCharacteristics)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    ULONG DeviceCharacteristics
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
VFWDFEXPORT(WdfDeviceGetCharacteristics)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
VFWDFEXPORT(WdfDeviceGetAlignmentRequirement)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceSetAlignmentRequirement)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    ULONG AlignmentRequirement
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitFree)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetPnpPowerEventCallbacks)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_PNPPOWER_EVENT_CALLBACKS PnpPowerEventCallbacks
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetPowerPolicyEventCallbacks)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_POWER_POLICY_EVENT_CALLBACKS PowerPolicyEventCallbacks
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetPowerPolicyOwnership)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    BOOLEAN IsPowerPolicyOwner
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceInitRegisterPnpStateChangeCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    WDF_DEVICE_PNP_STATE PnpState,
    _In_
    PFN_WDF_DEVICE_PNP_STATE_CHANGE_NOTIFICATION EvtDevicePnpStateChange,
    _In_
    ULONG CallbackTypes
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceInitRegisterPowerStateChangeCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    WDF_DEVICE_POWER_STATE PowerState,
    _In_
    PFN_WDF_DEVICE_POWER_STATE_CHANGE_NOTIFICATION EvtDevicePowerStateChange,
    _In_
    ULONG CallbackTypes
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceInitRegisterPowerPolicyStateChangeCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    WDF_DEVICE_POWER_POLICY_STATE PowerPolicyState,
    _In_
    PFN_WDF_DEVICE_POWER_POLICY_STATE_CHANGE_NOTIFICATION EvtDevicePowerPolicyStateChange,
    _In_
    ULONG CallbackTypes
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetExclusive)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    BOOLEAN IsExclusive
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetIoType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    WDF_DEVICE_IO_TYPE IoType
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetPowerNotPageable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetPowerPageable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetPowerInrush)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetDeviceType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    DEVICE_TYPE DeviceType
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceInitAssignName)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_opt_
    PCUNICODE_STRING DeviceName
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceInitAssignSDDLString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_opt_
    PCUNICODE_STRING SDDLString
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetDeviceClass)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    CONST GUID* DeviceClassGuid
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetCharacteristics)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    ULONG DeviceCharacteristics,
    _In_
    BOOLEAN OrInValues
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetFileObjectConfig)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_FILEOBJECT_CONFIG FileObjectConfig,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES FileObjectAttributes
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetRequestAttributes)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_OBJECT_ATTRIBUTES RequestAttributes
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceInitAssignWdmIrpPreprocessCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PFN_WDFDEVICE_WDM_IRP_PREPROCESS EvtDeviceWdmIrpPreprocess,
    _In_
    UCHAR MajorFunction,
    _When_(NumMinorFunctions > 0, _In_reads_bytes_(NumMinorFunctions))
    _When_(NumMinorFunctions == 0, _In_opt_)
    PUCHAR MinorFunctions,
    _In_
    ULONG NumMinorFunctions
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetIoInCallerContextCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PFN_WDF_IO_IN_CALLER_CONTEXT EvtIoInCallerContext
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetRemoveLockOptions)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_REMOVE_LOCK_OPTIONS Options
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _Inout_
    PWDFDEVICE_INIT* DeviceInit,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES DeviceAttributes,
    _Out_
    WDFDEVICE* Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceSetStaticStopRemove)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    BOOLEAN Stoppable
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceCreateDeviceInterface)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    CONST GUID* InterfaceClassGUID,
    _In_opt_
    PCUNICODE_STRING ReferenceString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceSetDeviceInterfaceState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    CONST GUID* InterfaceClassGUID,
    _In_opt_
    PCUNICODE_STRING ReferenceString,
    _In_
    BOOLEAN IsInterfaceEnabled
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceRetrieveDeviceInterfaceString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    CONST GUID* InterfaceClassGUID,
    _In_opt_
    PCUNICODE_STRING ReferenceString,
    _In_
    WDFSTRING String
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceCreateSymbolicLink)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PCUNICODE_STRING SymbolicLinkName
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceQueryProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_
    ULONG BufferLength,
    _Out_writes_bytes_all_(BufferLength)
    PVOID PropertyBuffer,
    _Out_
    PULONG ResultLength
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceAllocAndQueryProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_
    WDFMEMORY* PropertyMemory
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceSetPnpCapabilities)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_PNP_CAPABILITIES PnpCapabilities
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceSetPowerCapabilities)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_POWER_CAPABILITIES PowerCapabilities
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceSetBusInformationForChildren)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PPNP_BUS_INFORMATION BusInformation
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceIndicateWakeStatus)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    NTSTATUS WaitWakeStatus
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceSetFailed)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDF_DEVICE_FAILED_ACTION FailedAction
    );

_Must_inspect_result_
_When_(WaitForD0 == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(WaitForD0 != 0, _IRQL_requires_max_(PASSIVE_LEVEL))
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceStopIdleNoTrack)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    BOOLEAN WaitForD0
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceResumeIdleNoTrack)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_Must_inspect_result_
_When_(WaitForD0 == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(WaitForD0 != 0, _IRQL_requires_max_(PASSIVE_LEVEL))
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceStopIdleActual)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    BOOLEAN WaitForD0,
    _In_opt_
    PVOID Tag,
    _In_
    LONG Line,
    _In_z_
    PCHAR File
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceResumeIdleActual)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_opt_
    PVOID Tag,
    _In_
    LONG Line,
    _In_z_
    PCHAR File
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFFILEOBJECT
VFWDFEXPORT(WdfDeviceGetFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PFILE_OBJECT FileObject
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceEnqueueRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFQUEUE
VFWDFEXPORT(WdfDeviceGetDefaultQueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceConfigureRequestDispatching)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDFQUEUE Queue,
    _In_
    _Strict_type_match_
    WDF_REQUEST_TYPE RequestType
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceConfigureWdmIrpDispatchCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_opt_
    WDFDRIVER Driver,
    _In_
    UCHAR MajorFunction,
    _In_
    PFN_WDFDEVICE_WDM_IRP_DISPATCH EvtDeviceWdmIrpDisptach,
    _In_opt_
    WDFCONTEXT DriverContext
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
POWER_ACTION
VFWDFEXPORT(WdfDeviceGetSystemPowerAction)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceWdmAssignPowerFrameworkSettings)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_POWER_FRAMEWORK_SETTINGS PowerFrameworkSettings
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetReleaseHardwareOrderOnFailure)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    WDF_RELEASE_HARDWARE_ORDER_ON_FAILURE ReleaseHardwareOrderOnFailure
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetIoTypeEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_IO_TYPE_CONFIG IoTypeConfig
    );

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceQueryPropertyEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_PROPERTY_DATA DeviceProperty,
    _In_
    ULONG BufferLength,
    _Out_
    PVOID PropertyBuffer,
    _Out_
    PULONG RequiredSize,
    _Out_
    PDEVPROPTYPE Type
    );

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceAllocAndQueryPropertyEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_PROPERTY_DATA DeviceProperty,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_
    WDFMEMORY* PropertyMemory,
    _Out_
    PDEVPROPTYPE Type
    );

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceAssignProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_PROPERTY_DATA DeviceProperty,
    _In_
    DEVPROPTYPE Type,
    _In_
    ULONG Size,
    _In_opt_
    PVOID Data
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFIOTARGET
VFWDFEXPORT(WdfDeviceGetSelfIoTarget)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitAllowSelfIoTarget)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDmaEnablerCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DMA_ENABLER_CONFIG Config,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFDMAENABLER* DmaEnablerHandle
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDmaEnablerConfigureSystemProfile)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler,
    _In_
    PWDF_DMA_SYSTEM_PROFILE_CONFIG ProfileConfig,
    _In_
    WDF_DMA_DIRECTION ConfigDirection
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
VFWDFEXPORT(WdfDmaEnablerGetMaximumLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
VFWDFEXPORT(WdfDmaEnablerGetMaximumScatterGatherElements)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDmaEnablerSetMaximumScatterGatherElements)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler,
    _In_
    _When_(MaximumFragments == 0, __drv_reportError(MaximumFragments cannot be zero))
    size_t MaximumFragments
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
VFWDFEXPORT(WdfDmaEnablerGetFragmentLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler,
    _In_
    WDF_DMA_DIRECTION DmaDirection
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDMA_ADAPTER
VFWDFEXPORT(WdfDmaEnablerWdmGetDmaAdapter)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler,
    _In_
    WDF_DMA_DIRECTION DmaDirection
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDmaTransactionCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFDMATRANSACTION* DmaTransaction
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDmaTransactionInitialize)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_
    PFN_WDF_PROGRAM_DMA EvtProgramDmaFunction,
    _In_
    WDF_DMA_DIRECTION DmaDirection,
    _In_
    PMDL Mdl,
    _In_
    PVOID VirtualAddress,
    _In_
    _When_(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDmaTransactionInitializeUsingOffset)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_
    PFN_WDF_PROGRAM_DMA EvtProgramDmaFunction,
    _In_
    WDF_DMA_DIRECTION DmaDirection,
    _In_
    PMDL Mdl,
    _In_
    size_t Offset,
    _In_
    _When_(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDmaTransactionInitializeUsingRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_
    WDFREQUEST Request,
    _In_
    PFN_WDF_PROGRAM_DMA EvtProgramDmaFunction,
    _In_
    WDF_DMA_DIRECTION DmaDirection
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDmaTransactionExecute)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_opt_
    WDFCONTEXT Context
    );

_Success_(TRUE)
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDmaTransactionRelease)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfDmaTransactionDmaCompleted)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _Out_
    NTSTATUS* Status
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfDmaTransactionDmaCompletedWithLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_
    size_t TransferredLength,
    _Out_
    NTSTATUS* Status
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfDmaTransactionDmaCompletedFinal)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_
    size_t FinalTransferredLength,
    _Out_
    NTSTATUS* Status
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
VFWDFEXPORT(WdfDmaTransactionGetBytesTransferred)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDmaTransactionSetMaximumLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_
    size_t MaximumLength
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFREQUEST
VFWDFEXPORT(WdfDmaTransactionGetRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
VFWDFEXPORT(WdfDmaTransactionGetCurrentDmaTransferLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfDmaTransactionGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDmaTransactionGetTransferInfo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _Out_opt_
    ULONG* MapRegisterCount,
    _Out_opt_
    ULONG* ScatterGatherElementCount
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDmaTransactionSetChannelConfigurationCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_opt_
    PFN_WDF_DMA_TRANSACTION_CONFIGURE_DMA_CHANNEL ConfigureRoutine,
    _In_opt_
    PVOID ConfigureContext
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDmaTransactionSetTransferCompleteCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_opt_
    PFN_WDF_DMA_TRANSACTION_DMA_TRANSFER_COMPLETE DmaCompletionRoutine,
    _In_opt_
    PVOID DmaCompletionContext
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDmaTransactionSetImmediateExecution)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_
    BOOLEAN UseImmediateExecution
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDmaTransactionAllocateResources)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_
    WDF_DMA_DIRECTION DmaDirection,
    _In_
    ULONG RequiredMapRegisters,
    _In_
    PFN_WDF_RESERVE_DMA EvtReserveDmaFunction,
    _In_
    PVOID EvtReserveDmaContext
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDmaTransactionSetDeviceAddressOffset)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction,
    _In_
    ULONG Offset
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDmaTransactionFreeResources)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfDmaTransactionCancel)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PVOID
VFWDFEXPORT(WdfDmaTransactionWdmGetTransferContext)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDmaTransactionStopSystemTransfer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDpcCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDF_DPC_CONFIG Config,
    _In_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFDPC* Dpc
    );

_IRQL_requires_max_(HIGH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfDpcEnqueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDPC Dpc
    );

_When_(Wait == __true, _IRQL_requires_max_(PASSIVE_LEVEL))
_When_(Wait == __false, _IRQL_requires_max_(HIGH_LEVEL))
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfDpcCancel)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDPC Dpc,
    _In_
    BOOLEAN Wait
    );

_IRQL_requires_max_(HIGH_LEVEL)
WDFAPI
WDFOBJECT
VFWDFEXPORT(WdfDpcGetParentObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDPC Dpc
    );

_IRQL_requires_max_(HIGH_LEVEL)
WDFAPI
PKDPC
VFWDFEXPORT(WdfDpcWdmGetDpc)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDPC Dpc
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDriverCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PDRIVER_OBJECT DriverObject,
    _In_
    PCUNICODE_STRING RegistryPath,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES DriverAttributes,
    _In_
    PWDF_DRIVER_CONFIG DriverConfig,
    _Out_opt_
    WDFDRIVER* Driver
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PWSTR
VFWDFEXPORT(WdfDriverGetRegistryPath)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDRIVER_OBJECT
VFWDFEXPORT(WdfDriverWdmGetDriverObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDriverOpenParametersRegistryKey)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver,
    _In_
    ACCESS_MASK DesiredAccess,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    _Out_
    WDFKEY* Key
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDRIVER
VFWDFEXPORT(WdfWdmDriverGetWdfDriverHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PDRIVER_OBJECT DriverObject
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDriverRegisterTraceInfo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PDRIVER_OBJECT DriverObject,
    _In_
    PFN_WDF_TRACE_CALLBACK EvtTraceCallback,
    _In_
    PVOID ControlBlock
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDriverRetrieveVersionString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver,
    _In_
    WDFSTRING String
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfDriverIsVersionAvailable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver,
    _In_
    PWDF_DRIVER_VERSION_AVAILABLE_PARAMS VersionAvailableParams
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
VFWDFEXPORT(WdfFdoInitWdmGetPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfFdoInitOpenRegistryKey)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    ULONG DeviceInstanceKeyType,
    _In_
    ACCESS_MASK DesiredAccess,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    _Out_
    WDFKEY* Key
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfFdoInitQueryProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_
    ULONG BufferLength,
    _Out_writes_bytes_all_opt_(BufferLength)
    PVOID PropertyBuffer,
    _Out_
    PULONG ResultLength
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfFdoInitAllocAndQueryProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_
    WDFMEMORY* PropertyMemory
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfFdoInitQueryPropertyEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_DEVICE_PROPERTY_DATA DeviceProperty,
    _In_
    ULONG BufferLength,
    _Out_
    PVOID PropertyBuffer,
    _Out_
    PULONG ResultLength,
    _Out_
    PDEVPROPTYPE Type
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfFdoInitAllocAndQueryPropertyEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_DEVICE_PROPERTY_DATA DeviceProperty,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_
    WDFMEMORY* PropertyMemory,
    _Out_
    PDEVPROPTYPE Type
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfFdoInitSetEventCallbacks)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_FDO_EVENT_CALLBACKS FdoEventCallbacks
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfFdoInitSetFilter)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfFdoInitSetDefaultChildListConfig)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _Inout_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_CHILD_LIST_CONFIG Config,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES DefaultChildListAttributes
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfFdoQueryForInterface)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Fdo,
    _In_
    LPCGUID InterfaceType,
    _Out_
    PINTERFACE Interface,
    _In_
    USHORT Size,
    _In_
    USHORT Version,
    _In_opt_
    PVOID InterfaceSpecificData
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFCHILDLIST
VFWDFEXPORT(WdfFdoGetDefaultChildList)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Fdo
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfFdoAddStaticChild)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Fdo,
    _In_
    WDFDEVICE Child
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfFdoLockStaticChildListForIteration)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Fdo
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfFdoRetrieveNextStaticChild)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Fdo,
    _In_opt_
    WDFDEVICE PreviousChild,
    _In_
    ULONG Flags
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfFdoUnlockStaticChildListFromIteration)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Fdo
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PUNICODE_STRING
VFWDFEXPORT(WdfFileObjectGetFileName)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
VFWDFEXPORT(WdfFileObjectGetFlags)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfFileObjectGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PFILE_OBJECT
VFWDFEXPORT(WdfFileObjectWdmGetFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfInterruptCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_INTERRUPT_CONFIG Configuration,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFINTERRUPT* Interrupt
    );

WDFAPI
BOOLEAN
VFWDFEXPORT(WdfInterruptQueueDpcForIsr)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    );

WDFAPI
BOOLEAN
VFWDFEXPORT(WdfInterruptQueueWorkItemForIsr)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfInterruptSynchronize)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt,
    _In_
    PFN_WDF_INTERRUPT_SYNCHRONIZE Callback,
    _In_
    WDFCONTEXT Context
    );

_IRQL_requires_max_(DISPATCH_LEVEL + 1)
WDFAPI
VOID
VFWDFEXPORT(WdfInterruptAcquireLock)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_not_held_(_Curr_)
    _Acquires_lock_(_Curr_)
    WDFINTERRUPT Interrupt
    );

_IRQL_requires_max_(DISPATCH_LEVEL + 1)
WDFAPI
VOID
VFWDFEXPORT(WdfInterruptReleaseLock)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_held_(_Curr_)
    _Releases_lock_(_Curr_)
    WDFINTERRUPT Interrupt
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfInterruptEnable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfInterruptDisable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    );

_Must_inspect_result_
WDFAPI
PKINTERRUPT
VFWDFEXPORT(WdfInterruptWdmGetInterrupt)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfInterruptGetInfo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt,
    _Out_
    PWDF_INTERRUPT_INFO Info
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfInterruptSetPolicy)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt,
    _In_
    WDF_INTERRUPT_POLICY Policy,
    _In_
    WDF_INTERRUPT_PRIORITY Priority,
    _In_
    KAFFINITY TargetProcessorSet
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfInterruptSetExtendedPolicy)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt,
    _In_
    PWDF_INTERRUPT_EXTENDED_POLICY PolicyAndGroup
    );

WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfInterruptGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    );

_Must_inspect_result_
_Post_satisfies_(return == 1 || return == 0)
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfInterruptTryToAcquireLock)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_not_held_(_Curr_)
    _When_(return!=0, _Acquires_lock_(_Curr_))
    WDFINTERRUPT Interrupt
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfInterruptReportActive)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfInterruptReportInactive)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoQueueCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_IO_QUEUE_CONFIG Config,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES QueueAttributes,
    _Out_opt_
    WDFQUEUE* Queue
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDF_IO_QUEUE_STATE
VFWDFEXPORT(WdfIoQueueGetState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _Out_opt_
    PULONG QueueRequests,
    _Out_opt_
    PULONG DriverRequests
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoQueueStart)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoQueueStop)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _When_(Context != 0, _In_)
    _When_(Context == 0, _In_opt_)
    PFN_WDF_IO_QUEUE_STATE StopComplete,
    _When_(StopComplete != 0, _In_)
    _When_(StopComplete == 0, _In_opt_)
    WDFCONTEXT Context
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoQueueStopSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfIoQueueGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoQueueRetrieveNextRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _Out_
    WDFREQUEST* OutRequest
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoQueueRetrieveRequestByFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _In_
    WDFFILEOBJECT FileObject,
    _Out_
    WDFREQUEST* OutRequest
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoQueueFindRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _In_opt_
    WDFREQUEST FoundRequest,
    _In_opt_
    WDFFILEOBJECT FileObject,
    _Inout_opt_
    PWDF_REQUEST_PARAMETERS Parameters,
    _Out_
    WDFREQUEST* OutRequest
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoQueueRetrieveFoundRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _In_
    WDFREQUEST FoundRequest,
    _Out_
    WDFREQUEST* OutRequest
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoQueueDrainSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoQueueDrain)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _When_(Context != 0, _In_)
    _When_(Context == 0, _In_opt_)
    PFN_WDF_IO_QUEUE_STATE DrainComplete,
    _When_(DrainComplete != 0, _In_)
    _When_(DrainComplete == 0, _In_opt_)
    WDFCONTEXT Context
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoQueuePurgeSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoQueuePurge)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _When_(Context != 0, _In_)
    _When_(Context == 0, _In_opt_)
    PFN_WDF_IO_QUEUE_STATE PurgeComplete,
    _When_(PurgeComplete != 0, _In_)
    _When_(PurgeComplete == 0, _In_opt_)
    WDFCONTEXT Context
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoQueueReadyNotify)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _In_opt_
    PFN_WDF_IO_QUEUE_STATE QueueReady,
    _In_opt_
    WDFCONTEXT Context
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoQueueAssignForwardProgressPolicy)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _In_
    PWDF_IO_QUEUE_FORWARD_PROGRESS_POLICY ForwardProgressPolicy
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoQueueStopAndPurge)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue,
    _When_(Context != 0, _In_)
    _When_(Context == 0, _In_opt_)
    PFN_WDF_IO_QUEUE_STATE StopAndPurgeComplete,
    _When_(StopAndPurgeComplete != 0, _In_)
    _When_(StopAndPurgeComplete == 0, _In_opt_)
    WDFCONTEXT Context
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoQueueStopAndPurgeSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES IoTargetAttributes,
    _Out_
    WDFIOTARGET* IoTarget
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetOpen)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    PWDF_IO_TARGET_OPEN_PARAMS OpenParams
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoTargetCloseForQueryRemove)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoTargetClose)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetStart)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_When_(Action == 3, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(Action == 0 || Action == 1 || Action == 2, _IRQL_requires_max_(PASSIVE_LEVEL))
WDFAPI
VOID
VFWDFEXPORT(WdfIoTargetStop)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    _Strict_type_match_
    WDF_IO_TARGET_SENT_IO_ACTION Action
    );

_When_(Action == 2, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(Action == 0 || Action == 1, _IRQL_requires_max_(PASSIVE_LEVEL))
WDFAPI
VOID
VFWDFEXPORT(WdfIoTargetPurge)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    _Strict_type_match_
    WDF_IO_TARGET_PURGE_IO_ACTION Action
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDF_IO_TARGET_STATE
VFWDFEXPORT(WdfIoTargetGetState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfIoTargetGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetQueryTargetProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_
    ULONG BufferLength,
    _When_(BufferLength != 0, _Out_writes_bytes_to_opt_(BufferLength, *ResultLength))
    _When_(BufferLength == 0, _Out_opt_)
    PVOID PropertyBuffer,
    _Deref_out_range_(<=,BufferLength)
    PULONG ResultLength
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetAllocAndQueryTargetProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    DEVICE_REGISTRY_PROPERTY DeviceProperty,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_
    WDFMEMORY* PropertyMemory
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetQueryForInterface)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    LPCGUID InterfaceType,
    _Out_
    PINTERFACE Interface,
    _In_
    USHORT Size,
    _In_
    USHORT Version,
    _In_opt_
    PVOID InterfaceSpecificData
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
VFWDFEXPORT(WdfIoTargetWdmGetTargetDeviceObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
VFWDFEXPORT(WdfIoTargetWdmGetTargetPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PFILE_OBJECT
VFWDFEXPORT(WdfIoTargetWdmGetTargetFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
HANDLE
VFWDFEXPORT(WdfIoTargetWdmGetTargetFileHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetSendReadSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR OutputBuffer,
    _In_opt_
    PLONGLONG DeviceOffset,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _Out_opt_
    PULONG_PTR BytesRead
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetFormatRequestForRead)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    WDFREQUEST Request,
    _In_opt_
    WDFMEMORY OutputBuffer,
    _In_opt_
    PWDFMEMORY_OFFSET OutputBufferOffset,
    _In_opt_
    PLONGLONG DeviceOffset
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetSendWriteSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR InputBuffer,
    _In_opt_
    PLONGLONG DeviceOffset,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _Out_opt_
    PULONG_PTR BytesWritten
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetFormatRequestForWrite)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    WDFREQUEST Request,
    _In_opt_
    WDFMEMORY InputBuffer,
    _In_opt_
    PWDFMEMORY_OFFSET InputBufferOffset,
    _In_opt_
    PLONGLONG DeviceOffset
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetSendIoctlSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_opt_
    WDFREQUEST Request,
    _In_
    ULONG IoctlCode,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR InputBuffer,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR OutputBuffer,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _Out_opt_
    PULONG_PTR BytesReturned
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetFormatRequestForIoctl)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    WDFREQUEST Request,
    _In_
    ULONG IoctlCode,
    _In_opt_
    WDFMEMORY InputBuffer,
    _In_opt_
    PWDFMEMORY_OFFSET InputBufferOffset,
    _In_opt_
    WDFMEMORY OutputBuffer,
    _In_opt_
    PWDFMEMORY_OFFSET OutputBufferOffset
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetSendInternalIoctlSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_opt_
    WDFREQUEST Request,
    _In_
    ULONG IoctlCode,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR InputBuffer,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR OutputBuffer,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _Out_opt_
    PULONG_PTR BytesReturned
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetFormatRequestForInternalIoctl)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    WDFREQUEST Request,
    _In_
    ULONG IoctlCode,
    _In_opt_
    WDFMEMORY InputBuffer,
    _In_opt_
    PWDFMEMORY_OFFSET InputBufferOffset,
    _In_opt_
    WDFMEMORY OutputBuffer,
    _In_opt_
    PWDFMEMORY_OFFSET OutputBufferOffset
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetSendInternalIoctlOthersSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_opt_
    WDFREQUEST Request,
    _In_
    ULONG IoctlCode,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR OtherArg1,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR OtherArg2,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR OtherArg4,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _Out_opt_
    PULONG_PTR BytesReturned
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetFormatRequestForInternalIoctlOthers)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    WDFREQUEST Request,
    _In_
    ULONG IoctlCode,
    _In_opt_
    WDFMEMORY OtherArg1,
    _In_opt_
    PWDFMEMORY_OFFSET OtherArg1Offset,
    _In_opt_
    WDFMEMORY OtherArg2,
    _In_opt_
    PWDFMEMORY_OFFSET OtherArg2Offset,
    _In_opt_
    WDFMEMORY OtherArg4,
    _In_opt_
    PWDFMEMORY_OFFSET OtherArg4Offset
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetSelfAssignDefaultIoQueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    WDFQUEUE Queue
    );

_Must_inspect_result_
_When_(PoolType == 1 || PoolType == 257, _IRQL_requires_max_(APC_LEVEL))
_When_(PoolType == 0 || PoolType == 256, _IRQL_requires_max_(DISPATCH_LEVEL))
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfMemoryCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    ULONG PoolTag,
    _In_
    _When_(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    _Out_
    WDFMEMORY* Memory,
    _Outptr_opt_result_bytebuffer_(BufferSize)
    PVOID* Buffer
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfMemoryCreatePreallocated)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _In_ __drv_aliasesMem
    PVOID Buffer,
    _In_
    _When_(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    _Out_
    WDFMEMORY* Memory
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PVOID
VFWDFEXPORT(WdfMemoryGetBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFMEMORY Memory,
    _Out_opt_
    size_t* BufferSize
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfMemoryAssignBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFMEMORY Memory,
    _Pre_notnull_ _Pre_writable_byte_size_(BufferSize)
    PVOID Buffer,
    _In_
    _When_(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfMemoryCopyToBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFMEMORY SourceMemory,
    _In_
    size_t SourceOffset,
    _Out_writes_bytes_( NumBytesToCopyTo )
    PVOID Buffer,
    _In_
    _When_(NumBytesToCopyTo == 0, __drv_reportError(NumBytesToCopyTo cannot be zero))
    size_t NumBytesToCopyTo
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfMemoryCopyFromBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFMEMORY DestinationMemory,
    _In_
    size_t DestinationOffset,
    _In_
    PVOID Buffer,
    _In_
    _When_(NumBytesToCopyFrom == 0, __drv_reportError(NumBytesToCopyFrom cannot be zero))
    size_t NumBytesToCopyFrom
    );

_Must_inspect_result_
_When_(PoolType == 1 || PoolType == 257, _IRQL_requires_max_(APC_LEVEL))
_When_(PoolType == 0 || PoolType == 256, _IRQL_requires_max_(DISPATCH_LEVEL))
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfLookasideListCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES LookasideAttributes,
    _In_
    _When_(BufferSize == 0, __drv_reportError(BufferSize cannot be zero))
    size_t BufferSize,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES MemoryAttributes,
    _In_opt_
    ULONG PoolTag,
    _Out_
    WDFLOOKASIDE* Lookaside
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfMemoryCreateFromLookaside)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFLOOKASIDE Lookaside,
    _Out_
    WDFMEMORY* Memory
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceMiniportCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _In_
    PDEVICE_OBJECT DeviceObject,
    _In_opt_
    PDEVICE_OBJECT AttachedDeviceObject,
    _In_opt_
    PDEVICE_OBJECT Pdo,
    _Out_
    WDFDEVICE* Device
    );

WDFAPI
VOID
VFWDFEXPORT(WdfDriverMiniportUnload)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver
    );

WDFAPI
PVOID
FASTCALL
VFWDFEXPORT(WdfObjectGetTypedContextWorker)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFOBJECT Handle,
    _In_
    PCWDF_OBJECT_CONTEXT_TYPE_INFO TypeInfo
    );

WDFAPI
NTSTATUS
VFWDFEXPORT(WdfObjectAllocateContext)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFOBJECT Handle,
    _In_
    PWDF_OBJECT_ATTRIBUTES ContextAttributes,
    _Outptr_opt_
    PVOID* Context
    );

WDFAPI
WDFOBJECT
FASTCALL
VFWDFEXPORT(WdfObjectContextGetObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PVOID ContextPointer
    );

WDFAPI
VOID
VFWDFEXPORT(WdfObjectReferenceActual)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFOBJECT Handle,
    _In_opt_
    PVOID Tag,
    _In_
    LONG Line,
    _In_z_
    PCHAR File
    );

WDFAPI
VOID
VFWDFEXPORT(WdfObjectDereferenceActual)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFOBJECT Handle,
    _In_opt_
    PVOID Tag,
    _In_
    LONG Line,
    _In_z_
    PCHAR File
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfObjectCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFOBJECT* Object
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfObjectDelete)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFOBJECT Object
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfObjectQuery)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFOBJECT Object,
    _In_
    CONST GUID* Guid,
    _In_
    ULONG QueryBufferLength,
    _Out_writes_bytes_(QueryBufferLength)
    PVOID QueryBuffer
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PWDFDEVICE_INIT
VFWDFEXPORT(WdfPdoInitAllocate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE ParentDevice
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfPdoInitSetEventCallbacks)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_PDO_EVENT_CALLBACKS DispatchTable
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfPdoInitAssignDeviceID)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PCUNICODE_STRING DeviceID
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfPdoInitAssignInstanceID)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PCUNICODE_STRING InstanceID
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfPdoInitAddHardwareID)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PCUNICODE_STRING HardwareID
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfPdoInitAddCompatibleID)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PCUNICODE_STRING CompatibleID
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfPdoInitAssignContainerID)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PCUNICODE_STRING ContainerID
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfPdoInitAddDeviceText)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PCUNICODE_STRING DeviceDescription,
    _In_
    PCUNICODE_STRING DeviceLocation,
    _In_
    LCID LocaleId
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfPdoInitSetDefaultLocale)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    LCID LocaleId
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfPdoInitAssignRawDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    CONST GUID* DeviceClassGuid
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfPdoInitAllowForwardingRequestToParent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfPdoMarkMissing)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfPdoRequestEject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfPdoGetParent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfPdoRetrieveIdentificationDescription)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _Inout_
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfPdoRetrieveAddressDescription)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _Inout_
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfPdoUpdateAddressDescription)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _Inout_
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfPdoAddEjectionRelationsPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PDEVICE_OBJECT PhysicalDevice
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfPdoRemoveEjectionRelationsPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PDEVICE_OBJECT PhysicalDevice
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfPdoClearEjectionRelationsDevices)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceAddQueryInterface)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_QUERY_INTERFACE_CONFIG InterfaceConfig
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryOpenKey)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    WDFKEY ParentKey,
    _In_
    PCUNICODE_STRING KeyName,
    _In_
    ACCESS_MASK DesiredAccess,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    _Out_
    WDFKEY* Key
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryCreateKey)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    WDFKEY ParentKey,
    _In_
    PCUNICODE_STRING KeyName,
    _In_
    ACCESS_MASK DesiredAccess,
    _In_
    ULONG CreateOptions,
    _Out_opt_
    PULONG CreateDisposition,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    _Out_
    WDFKEY* Key
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRegistryClose)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
HANDLE
VFWDFEXPORT(WdfRegistryWdmGetHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryRemoveKey)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryRemoveValue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryQueryValue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    ULONG ValueLength,
    _Out_writes_bytes_opt_( ValueLength)
    PVOID Value,
    _Out_opt_
    PULONG ValueLengthQueried,
    _Out_opt_
    PULONG ValueType
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryQueryMemory)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES MemoryAttributes,
    _Out_
    WDFMEMORY* Memory,
    _Out_opt_
    PULONG ValueType
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryQueryMultiString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES StringsAttributes,
    _In_
    WDFCOLLECTION Collection
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryQueryUnicodeString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _Out_opt_
    PUSHORT ValueByteLength,
    _Inout_opt_
    PUNICODE_STRING Value
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryQueryString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    WDFSTRING String
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryQueryULong)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _Out_
    PULONG Value
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryAssignValue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    ULONG ValueType,
    _In_
    ULONG ValueLength,
    _In_reads_( ValueLength)
    PVOID Value
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryAssignMemory)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    ULONG ValueType,
    _In_
    WDFMEMORY Memory,
    _In_opt_
    PWDFMEMORY_OFFSET MemoryOffsets
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryAssignMultiString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    WDFCOLLECTION StringsCollection
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryAssignUnicodeString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    PCUNICODE_STRING Value
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryAssignString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    WDFSTRING String
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryAssignULong)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key,
    _In_
    PCUNICODE_STRING ValueName,
    _In_
    ULONG Value
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES RequestAttributes,
    _In_opt_
    WDFIOTARGET IoTarget,
    _Out_
    WDFREQUEST* Request
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestCreateFromIrp)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES RequestAttributes,
    _In_
    PIRP Irp,
    _In_
    BOOLEAN RequestFreesIrp,
    _Out_
    WDFREQUEST* Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestReuse)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    PWDF_REQUEST_REUSE_PARAMS ReuseParams
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestChangeTarget)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    WDFIOTARGET IoTarget
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRequestFormatRequestUsingCurrentType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRequestWdmFormatUsingStackLocation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    PIO_STACK_LOCATION Stack
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_When_(Options->Flags & WDF_REQUEST_SEND_OPTION_SYNCHRONOUS == 0, _Must_inspect_result_)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfRequestSend)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    WDFIOTARGET Target,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS Options
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestGetStatus)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRequestMarkCancelable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    PFN_WDF_REQUEST_CANCEL EvtRequestCancel
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestMarkCancelableEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    PFN_WDF_REQUEST_CANCEL EvtRequestCancel
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestUnmarkCancelable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfRequestIsCanceled)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfRequestCancelSentRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfRequestIsFrom32BitProcess)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRequestSetCompletionRoutine)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_opt_
    PFN_WDF_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
    _In_opt_ __drv_aliasesMem
    WDFCONTEXT CompletionContext
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRequestGetCompletionParams)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _Out_
    PWDF_REQUEST_COMPLETION_PARAMS Params
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestAllocateTimer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRequestComplete)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    NTSTATUS Status
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRequestCompleteWithPriorityBoost)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    NTSTATUS Status,
    _In_
    CCHAR PriorityBoost
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRequestCompleteWithInformation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    NTSTATUS Status,
    _In_
    ULONG_PTR Information
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRequestGetParameters)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _Out_
    PWDF_REQUEST_PARAMETERS Parameters
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestRetrieveInputMemory)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _Out_
    WDFMEMORY* Memory
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestRetrieveOutputMemory)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _Out_
    WDFMEMORY* Memory
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestRetrieveInputBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    size_t MinimumRequiredLength,
    _Outptr_result_bytebuffer_(*Length)
    PVOID* Buffer,
    _Out_opt_
    size_t* Length
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestRetrieveOutputBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    size_t MinimumRequiredSize,
    _Outptr_result_bytebuffer_(*Length)
    PVOID* Buffer,
    _Out_opt_
    size_t* Length
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestRetrieveInputWdmMdl)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _Outptr_
    PMDL* Mdl
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestRetrieveOutputWdmMdl)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _Outptr_
    PMDL* Mdl
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestRetrieveUnsafeUserInputBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    size_t MinimumRequiredLength,
    _Outptr_result_bytebuffer_maybenull_(*Length)
    PVOID* InputBuffer,
    _Out_opt_
    size_t* Length
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestRetrieveUnsafeUserOutputBuffer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    size_t MinimumRequiredLength,
    _Outptr_result_bytebuffer_maybenull_(*Length)
    PVOID* OutputBuffer,
    _Out_opt_
    size_t* Length
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRequestSetInformation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    ULONG_PTR Information
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG_PTR
VFWDFEXPORT(WdfRequestGetInformation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFFILEOBJECT
VFWDFEXPORT(WdfRequestGetFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestProbeAndLockUserBufferForRead)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_reads_bytes_(Length)
    PVOID Buffer,
    _In_
    size_t Length,
    _Out_
    WDFMEMORY* MemoryObject
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestProbeAndLockUserBufferForWrite)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_reads_bytes_(Length)
    PVOID Buffer,
    _In_
    size_t Length,
    _Out_
    WDFMEMORY* MemoryObject
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
KPROCESSOR_MODE
VFWDFEXPORT(WdfRequestGetRequestorMode)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestForwardToIoQueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    WDFQUEUE DestinationQueue
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFQUEUE
VFWDFEXPORT(WdfRequestGetIoQueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestRequeue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRequestStopAcknowledge)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    BOOLEAN Requeue
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PIRP
VFWDFEXPORT(WdfRequestWdmGetIrp)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfRequestIsReserved)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestForwardToParentDeviceIoQueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    WDFQUEUE ParentDeviceQueue,
    _In_
    PWDF_REQUEST_FORWARD_OPTIONS ForwardOptions
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoResourceRequirementsListSetSlotNumber)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_
    ULONG SlotNumber
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoResourceRequirementsListSetInterfaceType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_
    _Strict_type_match_
    INTERFACE_TYPE InterfaceType
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoResourceRequirementsListAppendIoResList)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_
    WDFIORESLIST IoResList
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoResourceRequirementsListInsertIoResList)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_
    WDFIORESLIST IoResList,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
VFWDFEXPORT(WdfIoResourceRequirementsListGetCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFIORESLIST
VFWDFEXPORT(WdfIoResourceRequirementsListGetIoResList)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoResourceRequirementsListRemove)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoResourceRequirementsListRemoveByIoResList)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_
    WDFIORESLIST IoResList
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoResourceListCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFIORESLIST* ResourceList
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoResourceListAppendDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESLIST ResourceList,
    _In_
    PIO_RESOURCE_DESCRIPTOR Descriptor
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoResourceListInsertDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESLIST ResourceList,
    _In_
    PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoResourceListUpdateDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESLIST ResourceList,
    _In_
    PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
VFWDFEXPORT(WdfIoResourceListGetCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESLIST ResourceList
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PIO_RESOURCE_DESCRIPTOR
VFWDFEXPORT(WdfIoResourceListGetDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESLIST ResourceList,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoResourceListRemove)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESLIST ResourceList,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoResourceListRemoveByDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESLIST ResourceList,
    _In_
    PIO_RESOURCE_DESCRIPTOR Descriptor
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfCmResourceListAppendDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCMRESLIST List,
    _In_
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfCmResourceListInsertDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCMRESLIST List,
    _In_
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
VFWDFEXPORT(WdfCmResourceListGetCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCMRESLIST List
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PCM_PARTIAL_RESOURCE_DESCRIPTOR
VFWDFEXPORT(WdfCmResourceListGetDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCMRESLIST List,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfCmResourceListRemove)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCMRESLIST List,
    _In_
    ULONG Index
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfCmResourceListRemoveByDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCMRESLIST List,
    _In_
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfStringCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PCUNICODE_STRING UnicodeString,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES StringAttributes,
    _Out_
    WDFSTRING* String
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfStringGetUnicodeString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFSTRING String,
    _Out_
    PUNICODE_STRING UnicodeString
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfObjectAcquireLock)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_not_held_(_Curr_)
    _Acquires_lock_(_Curr_)
    WDFOBJECT Object
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfObjectReleaseLock)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_held_(_Curr_)
    _Releases_lock_(_Curr_)
    WDFOBJECT Object
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfWaitLockCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES LockAttributes,
    _Out_
    WDFWAITLOCK* Lock
    );

_When_(Timeout == NULL, _IRQL_requires_max_(PASSIVE_LEVEL))
_When_(Timeout != NULL && *Timeout == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(Timeout != NULL && *Timeout != 0, _IRQL_requires_max_(PASSIVE_LEVEL))
_Always_(_When_(Timeout == NULL, _Acquires_lock_(Lock)))
_When_(Timeout != NULL && return == STATUS_SUCCESS, _Acquires_lock_(Lock))
_When_(Timeout != NULL, _Must_inspect_result_)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfWaitLockAcquire)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_not_held_(_Curr_)
    WDFWAITLOCK Lock,
    _In_opt_
    PLONGLONG Timeout
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfWaitLockRelease)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_held_(_Curr_)
    _Releases_lock_(_Curr_)
    WDFWAITLOCK Lock
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfSpinLockCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES SpinLockAttributes,
    _Out_
    WDFSPINLOCK* SpinLock
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_raises_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfSpinLockAcquire)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_not_held_(_Curr_)
    _Acquires_lock_(_Curr_)
    _IRQL_saves_
    WDFSPINLOCK SpinLock
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfSpinLockRelease)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_held_(_Curr_)
    _Releases_lock_(_Curr_)
    _IRQL_restores_
    WDFSPINLOCK SpinLock
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfTimerCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDF_TIMER_CONFIG Config,
    _In_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFTIMER* Timer
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfTimerStart)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFTIMER Timer,
    _In_
    LONGLONG DueTime
    );

_When_(Wait == __true, _IRQL_requires_max_(PASSIVE_LEVEL))
_When_(Wait == __false, _IRQL_requires_max_(DISPATCH_LEVEL))
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfTimerStop)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFTIMER Timer,
    _In_
    BOOLEAN Wait
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
VFWDFEXPORT(WdfTimerGetParentObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFTIMER Timer
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFUSBDEVICE* UsbDevice
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceCreateWithParameters)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_USB_DEVICE_CREATE_CONFIG Config,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFUSBDEVICE* UsbDevice
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceRetrieveInformation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _Out_
    PWDF_USB_DEVICE_INFORMATION Information
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfUsbTargetDeviceGetDeviceDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _Out_
    PUSB_DEVICE_DESCRIPTOR UsbDeviceDescriptor
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceRetrieveConfigDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _Out_writes_bytes_to_opt_(*ConfigDescriptorLength,*ConfigDescriptorLength)
    PVOID ConfigDescriptor,
    _Inout_
    PUSHORT ConfigDescriptorLength
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceQueryString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _Out_writes_opt_(*NumCharacters)
    PUSHORT String,
    _Inout_
    PUSHORT NumCharacters,
    _In_
    UCHAR StringIndex,
    _In_opt_
    USHORT LangID
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceAllocAndQueryString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES StringMemoryAttributes,
    _Out_
    WDFMEMORY* StringMemory,
    _Out_opt_
    PUSHORT NumCharacters,
    _In_
    UCHAR StringIndex,
    _In_opt_
    USHORT LangID
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceFormatRequestForString)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_
    WDFREQUEST Request,
    _In_
    WDFMEMORY Memory,
    _In_opt_
    PWDFMEMORY_OFFSET Offset,
    _In_
    UCHAR StringIndex,
    _In_opt_
    USHORT LangID
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
UCHAR
VFWDFEXPORT(WdfUsbTargetDeviceGetNumInterfaces)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceSelectConfig)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PipeAttributes,
    _Inout_
    PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
USBD_CONFIGURATION_HANDLE
VFWDFEXPORT(WdfUsbTargetDeviceWdmGetConfigurationHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceRetrieveCurrentFrameNumber)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _Out_
    PULONG CurrentFrameNumber
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceSendControlTransferSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _In_
    PWDF_USB_CONTROL_SETUP_PACKET SetupPacket,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    _Out_opt_
    PULONG BytesTransferred
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceFormatRequestForControlTransfer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_
    WDFREQUEST Request,
    _In_
    PWDF_USB_CONTROL_SETUP_PACKET SetupPacket,
    _In_opt_
    WDFMEMORY TransferMemory,
    _In_opt_
    PWDFMEMORY_OFFSET TransferOffset
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceIsConnectedSynchronous)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceResetPortSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceCyclePortSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceFormatRequestForCyclePort)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceSendUrbSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _In_reads_(_Inexpressible_("union bug in SAL"))
    PURB Urb
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceFormatRequestForUrb)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_
    WDFREQUEST Request,
    _In_
    WDFMEMORY UrbMemory,
    _In_opt_
    PWDFMEMORY_OFFSET UrbMemoryOffset
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceQueryUsbCapability)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_
    CONST GUID* CapabilityType,
    _In_
    ULONG CapabilityBufferLength,
    _When_(CapabilityBufferLength == 0, _Out_opt_)
    _When_(CapabilityBufferLength != 0 && ResultLength == NULL, _Out_writes_bytes_(CapabilityBufferLength))
    _When_(CapabilityBufferLength != 0 && ResultLength != NULL, _Out_writes_bytes_to_opt_(CapabilityBufferLength, *ResultLength))
    PVOID CapabilityBuffer,
    _Out_opt_
    _When_(ResultLength != NULL,_Deref_out_range_(<=,CapabilityBufferLength))
    PULONG ResultLength
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceCreateUrb)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFMEMORY* UrbMemory,
    _Outptr_opt_result_bytebuffer_(sizeof(URB))
    PURB* Urb
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceCreateIsochUrb)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _In_
    ULONG NumberOfIsochPackets,
    _Out_
    WDFMEMORY* UrbMemory,
    _Outptr_opt_result_bytebuffer_(GET_ISO_URB_SIZE(NumberOfIsochPackets))
    PURB* Urb
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfUsbTargetPipeGetInformation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _Out_
    PWDF_USB_PIPE_INFORMATION PipeInformation
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfUsbTargetPipeIsInEndpoint)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfUsbTargetPipeIsOutEndpoint)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDF_USB_PIPE_TYPE
VFWDFEXPORT(WdfUsbTargetPipeGetType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfUsbTargetPipeSetNoMaximumPacketSizeCheck)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetPipeWriteSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    _Out_opt_
    PULONG BytesWritten
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetPipeFormatRequestForWrite)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_
    WDFREQUEST Request,
    _In_opt_
    WDFMEMORY WriteMemory,
    _In_opt_
    PWDFMEMORY_OFFSET WriteOffset
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetPipeReadSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _In_opt_
    PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    _Out_opt_
    PULONG BytesRead
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetPipeFormatRequestForRead)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_
    WDFREQUEST Request,
    _In_opt_
    WDFMEMORY ReadMemory,
    _In_opt_
    PWDFMEMORY_OFFSET ReadOffset
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetPipeConfigContinuousReader)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_
    PWDF_USB_CONTINUOUS_READER_CONFIG Config
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetPipeAbortSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetPipeFormatRequestForAbort)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetPipeResetSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetPipeFormatRequestForReset)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_
    WDFREQUEST Request
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetPipeSendUrbSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe,
    _In_opt_
    WDFREQUEST Request,
    _In_opt_
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    _In_reads_(_Inexpressible_("union bug in SAL"))
    PURB Urb
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetPipeFormatRequestForUrb)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE PIPE,
    _In_
    WDFREQUEST Request,
    _In_
    WDFMEMORY UrbMemory,
    _In_opt_
    PWDFMEMORY_OFFSET UrbMemoryOffset
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BYTE
VFWDFEXPORT(WdfUsbInterfaceGetInterfaceNumber)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BYTE
VFWDFEXPORT(WdfUsbInterfaceGetNumEndpoints)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface,
    _In_
    UCHAR SettingIndex
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfUsbInterfaceGetDescriptor)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface,
    _In_
    UCHAR SettingIndex,
    _Out_
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BYTE
VFWDFEXPORT(WdfUsbInterfaceGetNumSettings)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbInterfaceSelectSetting)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PipesAttributes,
    _In_
    PWDF_USB_INTERFACE_SELECT_SETTING_PARAMS Params
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfUsbInterfaceGetEndpointInformation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface,
    _In_
    UCHAR SettingIndex,
    _In_
    UCHAR EndpointIndex,
    _Out_
    PWDF_USB_PIPE_INFORMATION EndpointInfo
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFUSBINTERFACE
VFWDFEXPORT(WdfUsbTargetDeviceGetInterface)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice,
    _In_
    UCHAR InterfaceIndex
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BYTE
VFWDFEXPORT(WdfUsbInterfaceGetConfiguredSettingIndex)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE Interface
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BYTE
VFWDFEXPORT(WdfUsbInterfaceGetNumConfiguredPipes)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFUSBPIPE
VFWDFEXPORT(WdfUsbInterfaceGetConfiguredPipe)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface,
    _In_
    UCHAR PipeIndex,
    _Out_opt_
    PWDF_USB_PIPE_INFORMATION PipeInfo
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
USBD_PIPE_HANDLE
VFWDFEXPORT(WdfUsbTargetPipeWdmGetPipeHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE UsbPipe
    );

WDFAPI
VOID
VFWDFEXPORT(WdfVerifierDbgBreakPoint)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals
    );

WDFAPI
VOID
VFWDFEXPORT(WdfVerifierKeBugCheck)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    ULONG BugCheckCode,
    _In_
    ULONG_PTR BugCheckParameter1,
    _In_
    ULONG_PTR BugCheckParameter2,
    _In_
    ULONG_PTR BugCheckParameter3,
    _In_
    ULONG_PTR BugCheckParameter4
    );

WDFAPI
PVOID
VFWDFEXPORT(WdfGetTriageInfo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfWmiProviderCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_WMI_PROVIDER_CONFIG WmiProviderConfig,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES ProviderAttributes,
    _Out_
    WDFWMIPROVIDER* WmiProvider
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfWmiProviderGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWMIPROVIDER WmiProvider
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfWmiProviderIsEnabled)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWMIPROVIDER WmiProvider,
    _In_
    WDF_WMI_PROVIDER_CONTROL ProviderControl
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONGLONG
VFWDFEXPORT(WdfWmiProviderGetTracingHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWMIPROVIDER WmiProvider
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfWmiInstanceCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_WMI_INSTANCE_CONFIG InstanceConfig,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES InstanceAttributes,
    _Out_opt_
    WDFWMIINSTANCE* Instance
    );

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfWmiInstanceRegister)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWMIINSTANCE WmiInstance
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfWmiInstanceDeregister)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWMIINSTANCE WmiInstance
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfWmiInstanceGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWMIINSTANCE WmiInstance
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFWMIPROVIDER
VFWDFEXPORT(WdfWmiInstanceGetProvider)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWMIINSTANCE WmiInstance
    );

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfWmiInstanceFireEvent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWMIINSTANCE WmiInstance,
    _In_opt_
    ULONG EventDataSize,
    _In_reads_bytes_opt_(EventDataSize)
    PVOID EventData
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfWorkItemCreate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDF_WORKITEM_CONFIG Config,
    _In_
    PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_
    WDFWORKITEM* WorkItem
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfWorkItemEnqueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWORKITEM WorkItem
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
VFWDFEXPORT(WdfWorkItemGetParentObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWORKITEM WorkItem
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfWorkItemFlush)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWORKITEM WorkItem
    );


#ifdef VF_FX_DYNAMICS_GENERATE_TABLE

WDFVERSION VfWdfVersion = {
    sizeof(WDFVERSION),
    sizeof(WDFFUNCTIONS)/sizeof(PVOID),
    {
        VFWDFEXPORT(WdfChildListCreate),
        VFWDFEXPORT(WdfChildListGetDevice),
        VFWDFEXPORT(WdfChildListRetrievePdo),
        VFWDFEXPORT(WdfChildListRetrieveAddressDescription),
        VFWDFEXPORT(WdfChildListBeginScan),
        VFWDFEXPORT(WdfChildListEndScan),
        VFWDFEXPORT(WdfChildListBeginIteration),
        VFWDFEXPORT(WdfChildListRetrieveNextDevice),
        VFWDFEXPORT(WdfChildListEndIteration),
        VFWDFEXPORT(WdfChildListAddOrUpdateChildDescriptionAsPresent),
        VFWDFEXPORT(WdfChildListUpdateChildDescriptionAsMissing),
        VFWDFEXPORT(WdfChildListUpdateAllChildDescriptionsAsPresent),
        VFWDFEXPORT(WdfChildListRequestChildEject),
        VFWDFEXPORT(WdfCollectionCreate),
        VFWDFEXPORT(WdfCollectionGetCount),
        VFWDFEXPORT(WdfCollectionAdd),
        VFWDFEXPORT(WdfCollectionRemove),
        VFWDFEXPORT(WdfCollectionRemoveItem),
        VFWDFEXPORT(WdfCollectionGetItem),
        VFWDFEXPORT(WdfCollectionGetFirstItem),
        VFWDFEXPORT(WdfCollectionGetLastItem),
        VFWDFEXPORT(WdfCommonBufferCreate),
        VFWDFEXPORT(WdfCommonBufferGetAlignedVirtualAddress),
        VFWDFEXPORT(WdfCommonBufferGetAlignedLogicalAddress),
        VFWDFEXPORT(WdfCommonBufferGetLength),
        VFWDFEXPORT(WdfControlDeviceInitAllocate),
        VFWDFEXPORT(WdfControlDeviceInitSetShutdownNotification),
        VFWDFEXPORT(WdfControlFinishInitializing),
        VFWDFEXPORT(WdfDeviceGetDeviceState),
        VFWDFEXPORT(WdfDeviceSetDeviceState),
        VFWDFEXPORT(WdfWdmDeviceGetWdfDeviceHandle),
        VFWDFEXPORT(WdfDeviceWdmGetDeviceObject),
        VFWDFEXPORT(WdfDeviceWdmGetAttachedDevice),
        VFWDFEXPORT(WdfDeviceWdmGetPhysicalDevice),
        VFWDFEXPORT(WdfDeviceWdmDispatchPreprocessedIrp),
        VFWDFEXPORT(WdfDeviceAddDependentUsageDeviceObject),
        VFWDFEXPORT(WdfDeviceAddRemovalRelationsPhysicalDevice),
        VFWDFEXPORT(WdfDeviceRemoveRemovalRelationsPhysicalDevice),
        VFWDFEXPORT(WdfDeviceClearRemovalRelationsDevices),
        VFWDFEXPORT(WdfDeviceGetDriver),
        VFWDFEXPORT(WdfDeviceRetrieveDeviceName),
        VFWDFEXPORT(WdfDeviceAssignMofResourceName),
        VFWDFEXPORT(WdfDeviceGetIoTarget),
        VFWDFEXPORT(WdfDeviceGetDevicePnpState),
        VFWDFEXPORT(WdfDeviceGetDevicePowerState),
        VFWDFEXPORT(WdfDeviceGetDevicePowerPolicyState),
        VFWDFEXPORT(WdfDeviceAssignS0IdleSettings),
        VFWDFEXPORT(WdfDeviceAssignSxWakeSettings),
        VFWDFEXPORT(WdfDeviceOpenRegistryKey),
        VFWDFEXPORT(WdfDeviceSetSpecialFileSupport),
        VFWDFEXPORT(WdfDeviceSetCharacteristics),
        VFWDFEXPORT(WdfDeviceGetCharacteristics),
        VFWDFEXPORT(WdfDeviceGetAlignmentRequirement),
        VFWDFEXPORT(WdfDeviceSetAlignmentRequirement),
        VFWDFEXPORT(WdfDeviceInitFree),
        VFWDFEXPORT(WdfDeviceInitSetPnpPowerEventCallbacks),
        VFWDFEXPORT(WdfDeviceInitSetPowerPolicyEventCallbacks),
        VFWDFEXPORT(WdfDeviceInitSetPowerPolicyOwnership),
        VFWDFEXPORT(WdfDeviceInitRegisterPnpStateChangeCallback),
        VFWDFEXPORT(WdfDeviceInitRegisterPowerStateChangeCallback),
        VFWDFEXPORT(WdfDeviceInitRegisterPowerPolicyStateChangeCallback),
        VFWDFEXPORT(WdfDeviceInitSetIoType),
        VFWDFEXPORT(WdfDeviceInitSetExclusive),
        VFWDFEXPORT(WdfDeviceInitSetPowerNotPageable),
        VFWDFEXPORT(WdfDeviceInitSetPowerPageable),
        VFWDFEXPORT(WdfDeviceInitSetPowerInrush),
        VFWDFEXPORT(WdfDeviceInitSetDeviceType),
        VFWDFEXPORT(WdfDeviceInitAssignName),
        VFWDFEXPORT(WdfDeviceInitAssignSDDLString),
        VFWDFEXPORT(WdfDeviceInitSetDeviceClass),
        VFWDFEXPORT(WdfDeviceInitSetCharacteristics),
        VFWDFEXPORT(WdfDeviceInitSetFileObjectConfig),
        VFWDFEXPORT(WdfDeviceInitSetRequestAttributes),
        VFWDFEXPORT(WdfDeviceInitAssignWdmIrpPreprocessCallback),
        VFWDFEXPORT(WdfDeviceInitSetIoInCallerContextCallback),
        VFWDFEXPORT(WdfDeviceCreate),
        VFWDFEXPORT(WdfDeviceSetStaticStopRemove),
        VFWDFEXPORT(WdfDeviceCreateDeviceInterface),
        VFWDFEXPORT(WdfDeviceSetDeviceInterfaceState),
        VFWDFEXPORT(WdfDeviceRetrieveDeviceInterfaceString),
        VFWDFEXPORT(WdfDeviceCreateSymbolicLink),
        VFWDFEXPORT(WdfDeviceQueryProperty),
        VFWDFEXPORT(WdfDeviceAllocAndQueryProperty),
        VFWDFEXPORT(WdfDeviceSetPnpCapabilities),
        VFWDFEXPORT(WdfDeviceSetPowerCapabilities),
        VFWDFEXPORT(WdfDeviceSetBusInformationForChildren),
        VFWDFEXPORT(WdfDeviceIndicateWakeStatus),
        VFWDFEXPORT(WdfDeviceSetFailed),
        VFWDFEXPORT(WdfDeviceStopIdleNoTrack),
        VFWDFEXPORT(WdfDeviceResumeIdleNoTrack),
        VFWDFEXPORT(WdfDeviceGetFileObject),
        VFWDFEXPORT(WdfDeviceEnqueueRequest),
        VFWDFEXPORT(WdfDeviceGetDefaultQueue),
        VFWDFEXPORT(WdfDeviceConfigureRequestDispatching),
        VFWDFEXPORT(WdfDmaEnablerCreate),
        VFWDFEXPORT(WdfDmaEnablerGetMaximumLength),
        VFWDFEXPORT(WdfDmaEnablerGetMaximumScatterGatherElements),
        VFWDFEXPORT(WdfDmaEnablerSetMaximumScatterGatherElements),
        VFWDFEXPORT(WdfDmaTransactionCreate),
        VFWDFEXPORT(WdfDmaTransactionInitialize),
        VFWDFEXPORT(WdfDmaTransactionInitializeUsingRequest),
        VFWDFEXPORT(WdfDmaTransactionExecute),
        VFWDFEXPORT(WdfDmaTransactionRelease),
        VFWDFEXPORT(WdfDmaTransactionDmaCompleted),
        VFWDFEXPORT(WdfDmaTransactionDmaCompletedWithLength),
        VFWDFEXPORT(WdfDmaTransactionDmaCompletedFinal),
        VFWDFEXPORT(WdfDmaTransactionGetBytesTransferred),
        VFWDFEXPORT(WdfDmaTransactionSetMaximumLength),
        VFWDFEXPORT(WdfDmaTransactionGetRequest),
        VFWDFEXPORT(WdfDmaTransactionGetCurrentDmaTransferLength),
        VFWDFEXPORT(WdfDmaTransactionGetDevice),
        VFWDFEXPORT(WdfDpcCreate),
        VFWDFEXPORT(WdfDpcEnqueue),
        VFWDFEXPORT(WdfDpcCancel),
        VFWDFEXPORT(WdfDpcGetParentObject),
        VFWDFEXPORT(WdfDpcWdmGetDpc),
        VFWDFEXPORT(WdfDriverCreate),
        VFWDFEXPORT(WdfDriverGetRegistryPath),
        VFWDFEXPORT(WdfDriverWdmGetDriverObject),
        VFWDFEXPORT(WdfDriverOpenParametersRegistryKey),
        VFWDFEXPORT(WdfWdmDriverGetWdfDriverHandle),
        VFWDFEXPORT(WdfDriverRegisterTraceInfo),
        VFWDFEXPORT(WdfDriverRetrieveVersionString),
        VFWDFEXPORT(WdfDriverIsVersionAvailable),
        VFWDFEXPORT(WdfFdoInitWdmGetPhysicalDevice),
        VFWDFEXPORT(WdfFdoInitOpenRegistryKey),
        VFWDFEXPORT(WdfFdoInitQueryProperty),
        VFWDFEXPORT(WdfFdoInitAllocAndQueryProperty),
        VFWDFEXPORT(WdfFdoInitSetEventCallbacks),
        VFWDFEXPORT(WdfFdoInitSetFilter),
        VFWDFEXPORT(WdfFdoInitSetDefaultChildListConfig),
        VFWDFEXPORT(WdfFdoQueryForInterface),
        VFWDFEXPORT(WdfFdoGetDefaultChildList),
        VFWDFEXPORT(WdfFdoAddStaticChild),
        VFWDFEXPORT(WdfFdoLockStaticChildListForIteration),
        VFWDFEXPORT(WdfFdoRetrieveNextStaticChild),
        VFWDFEXPORT(WdfFdoUnlockStaticChildListFromIteration),
        VFWDFEXPORT(WdfFileObjectGetFileName),
        VFWDFEXPORT(WdfFileObjectGetFlags),
        VFWDFEXPORT(WdfFileObjectGetDevice),
        VFWDFEXPORT(WdfFileObjectWdmGetFileObject),
        VFWDFEXPORT(WdfInterruptCreate),
        VFWDFEXPORT(WdfInterruptQueueDpcForIsr),
        VFWDFEXPORT(WdfInterruptSynchronize),
        VFWDFEXPORT(WdfInterruptAcquireLock),
        VFWDFEXPORT(WdfInterruptReleaseLock),
        VFWDFEXPORT(WdfInterruptEnable),
        VFWDFEXPORT(WdfInterruptDisable),
        VFWDFEXPORT(WdfInterruptWdmGetInterrupt),
        VFWDFEXPORT(WdfInterruptGetInfo),
        VFWDFEXPORT(WdfInterruptSetPolicy),
        VFWDFEXPORT(WdfInterruptGetDevice),
        VFWDFEXPORT(WdfIoQueueCreate),
        VFWDFEXPORT(WdfIoQueueGetState),
        VFWDFEXPORT(WdfIoQueueStart),
        VFWDFEXPORT(WdfIoQueueStop),
        VFWDFEXPORT(WdfIoQueueStopSynchronously),
        VFWDFEXPORT(WdfIoQueueGetDevice),
        VFWDFEXPORT(WdfIoQueueRetrieveNextRequest),
        VFWDFEXPORT(WdfIoQueueRetrieveRequestByFileObject),
        VFWDFEXPORT(WdfIoQueueFindRequest),
        VFWDFEXPORT(WdfIoQueueRetrieveFoundRequest),
        VFWDFEXPORT(WdfIoQueueDrainSynchronously),
        VFWDFEXPORT(WdfIoQueueDrain),
        VFWDFEXPORT(WdfIoQueuePurgeSynchronously),
        VFWDFEXPORT(WdfIoQueuePurge),
        VFWDFEXPORT(WdfIoQueueReadyNotify),
        VFWDFEXPORT(WdfIoTargetCreate),
        VFWDFEXPORT(WdfIoTargetOpen),
        VFWDFEXPORT(WdfIoTargetCloseForQueryRemove),
        VFWDFEXPORT(WdfIoTargetClose),
        VFWDFEXPORT(WdfIoTargetStart),
        VFWDFEXPORT(WdfIoTargetStop),
        VFWDFEXPORT(WdfIoTargetGetState),
        VFWDFEXPORT(WdfIoTargetGetDevice),
        VFWDFEXPORT(WdfIoTargetQueryTargetProperty),
        VFWDFEXPORT(WdfIoTargetAllocAndQueryTargetProperty),
        VFWDFEXPORT(WdfIoTargetQueryForInterface),
        VFWDFEXPORT(WdfIoTargetWdmGetTargetDeviceObject),
        VFWDFEXPORT(WdfIoTargetWdmGetTargetPhysicalDevice),
        VFWDFEXPORT(WdfIoTargetWdmGetTargetFileObject),
        VFWDFEXPORT(WdfIoTargetWdmGetTargetFileHandle),
        VFWDFEXPORT(WdfIoTargetSendReadSynchronously),
        VFWDFEXPORT(WdfIoTargetFormatRequestForRead),
        VFWDFEXPORT(WdfIoTargetSendWriteSynchronously),
        VFWDFEXPORT(WdfIoTargetFormatRequestForWrite),
        VFWDFEXPORT(WdfIoTargetSendIoctlSynchronously),
        VFWDFEXPORT(WdfIoTargetFormatRequestForIoctl),
        VFWDFEXPORT(WdfIoTargetSendInternalIoctlSynchronously),
        VFWDFEXPORT(WdfIoTargetFormatRequestForInternalIoctl),
        VFWDFEXPORT(WdfIoTargetSendInternalIoctlOthersSynchronously),
        VFWDFEXPORT(WdfIoTargetFormatRequestForInternalIoctlOthers),
        VFWDFEXPORT(WdfMemoryCreate),
        VFWDFEXPORT(WdfMemoryCreatePreallocated),
        VFWDFEXPORT(WdfMemoryGetBuffer),
        VFWDFEXPORT(WdfMemoryAssignBuffer),
        VFWDFEXPORT(WdfMemoryCopyToBuffer),
        VFWDFEXPORT(WdfMemoryCopyFromBuffer),
        VFWDFEXPORT(WdfLookasideListCreate),
        VFWDFEXPORT(WdfMemoryCreateFromLookaside),
        VFWDFEXPORT(WdfDeviceMiniportCreate),
        VFWDFEXPORT(WdfDriverMiniportUnload),
        VFWDFEXPORT(WdfObjectGetTypedContextWorker),
        VFWDFEXPORT(WdfObjectAllocateContext),
        VFWDFEXPORT(WdfObjectContextGetObject),
        VFWDFEXPORT(WdfObjectReferenceActual),
        VFWDFEXPORT(WdfObjectDereferenceActual),
        VFWDFEXPORT(WdfObjectCreate),
        VFWDFEXPORT(WdfObjectDelete),
        VFWDFEXPORT(WdfObjectQuery),
        VFWDFEXPORT(WdfPdoInitAllocate),
        VFWDFEXPORT(WdfPdoInitSetEventCallbacks),
        VFWDFEXPORT(WdfPdoInitAssignDeviceID),
        VFWDFEXPORT(WdfPdoInitAssignInstanceID),
        VFWDFEXPORT(WdfPdoInitAddHardwareID),
        VFWDFEXPORT(WdfPdoInitAddCompatibleID),
        VFWDFEXPORT(WdfPdoInitAddDeviceText),
        VFWDFEXPORT(WdfPdoInitSetDefaultLocale),
        VFWDFEXPORT(WdfPdoInitAssignRawDevice),
        VFWDFEXPORT(WdfPdoMarkMissing),
        VFWDFEXPORT(WdfPdoRequestEject),
        VFWDFEXPORT(WdfPdoGetParent),
        VFWDFEXPORT(WdfPdoRetrieveIdentificationDescription),
        VFWDFEXPORT(WdfPdoRetrieveAddressDescription),
        VFWDFEXPORT(WdfPdoUpdateAddressDescription),
        VFWDFEXPORT(WdfPdoAddEjectionRelationsPhysicalDevice),
        VFWDFEXPORT(WdfPdoRemoveEjectionRelationsPhysicalDevice),
        VFWDFEXPORT(WdfPdoClearEjectionRelationsDevices),
        VFWDFEXPORT(WdfDeviceAddQueryInterface),
        VFWDFEXPORT(WdfRegistryOpenKey),
        VFWDFEXPORT(WdfRegistryCreateKey),
        VFWDFEXPORT(WdfRegistryClose),
        VFWDFEXPORT(WdfRegistryWdmGetHandle),
        VFWDFEXPORT(WdfRegistryRemoveKey),
        VFWDFEXPORT(WdfRegistryRemoveValue),
        VFWDFEXPORT(WdfRegistryQueryValue),
        VFWDFEXPORT(WdfRegistryQueryMemory),
        VFWDFEXPORT(WdfRegistryQueryMultiString),
        VFWDFEXPORT(WdfRegistryQueryUnicodeString),
        VFWDFEXPORT(WdfRegistryQueryString),
        VFWDFEXPORT(WdfRegistryQueryULong),
        VFWDFEXPORT(WdfRegistryAssignValue),
        VFWDFEXPORT(WdfRegistryAssignMemory),
        VFWDFEXPORT(WdfRegistryAssignMultiString),
        VFWDFEXPORT(WdfRegistryAssignUnicodeString),
        VFWDFEXPORT(WdfRegistryAssignString),
        VFWDFEXPORT(WdfRegistryAssignULong),
        VFWDFEXPORT(WdfRequestCreate),
        VFWDFEXPORT(WdfRequestCreateFromIrp),
        VFWDFEXPORT(WdfRequestReuse),
        VFWDFEXPORT(WdfRequestChangeTarget),
        VFWDFEXPORT(WdfRequestFormatRequestUsingCurrentType),
        VFWDFEXPORT(WdfRequestWdmFormatUsingStackLocation),
        VFWDFEXPORT(WdfRequestSend),
        VFWDFEXPORT(WdfRequestGetStatus),
        VFWDFEXPORT(WdfRequestMarkCancelable),
        VFWDFEXPORT(WdfRequestUnmarkCancelable),
        VFWDFEXPORT(WdfRequestIsCanceled),
        VFWDFEXPORT(WdfRequestCancelSentRequest),
        VFWDFEXPORT(WdfRequestIsFrom32BitProcess),
        VFWDFEXPORT(WdfRequestSetCompletionRoutine),
        VFWDFEXPORT(WdfRequestGetCompletionParams),
        VFWDFEXPORT(WdfRequestAllocateTimer),
        VFWDFEXPORT(WdfRequestComplete),
        VFWDFEXPORT(WdfRequestCompleteWithPriorityBoost),
        VFWDFEXPORT(WdfRequestCompleteWithInformation),
        VFWDFEXPORT(WdfRequestGetParameters),
        VFWDFEXPORT(WdfRequestRetrieveInputMemory),
        VFWDFEXPORT(WdfRequestRetrieveOutputMemory),
        VFWDFEXPORT(WdfRequestRetrieveInputBuffer),
        VFWDFEXPORT(WdfRequestRetrieveOutputBuffer),
        VFWDFEXPORT(WdfRequestRetrieveInputWdmMdl),
        VFWDFEXPORT(WdfRequestRetrieveOutputWdmMdl),
        VFWDFEXPORT(WdfRequestRetrieveUnsafeUserInputBuffer),
        VFWDFEXPORT(WdfRequestRetrieveUnsafeUserOutputBuffer),
        VFWDFEXPORT(WdfRequestSetInformation),
        VFWDFEXPORT(WdfRequestGetInformation),
        VFWDFEXPORT(WdfRequestGetFileObject),
        VFWDFEXPORT(WdfRequestProbeAndLockUserBufferForRead),
        VFWDFEXPORT(WdfRequestProbeAndLockUserBufferForWrite),
        VFWDFEXPORT(WdfRequestGetRequestorMode),
        VFWDFEXPORT(WdfRequestForwardToIoQueue),
        VFWDFEXPORT(WdfRequestGetIoQueue),
        VFWDFEXPORT(WdfRequestRequeue),
        VFWDFEXPORT(WdfRequestStopAcknowledge),
        VFWDFEXPORT(WdfRequestWdmGetIrp),
        VFWDFEXPORT(WdfIoResourceRequirementsListSetSlotNumber),
        VFWDFEXPORT(WdfIoResourceRequirementsListSetInterfaceType),
        VFWDFEXPORT(WdfIoResourceRequirementsListAppendIoResList),
        VFWDFEXPORT(WdfIoResourceRequirementsListInsertIoResList),
        VFWDFEXPORT(WdfIoResourceRequirementsListGetCount),
        VFWDFEXPORT(WdfIoResourceRequirementsListGetIoResList),
        VFWDFEXPORT(WdfIoResourceRequirementsListRemove),
        VFWDFEXPORT(WdfIoResourceRequirementsListRemoveByIoResList),
        VFWDFEXPORT(WdfIoResourceListCreate),
        VFWDFEXPORT(WdfIoResourceListAppendDescriptor),
        VFWDFEXPORT(WdfIoResourceListInsertDescriptor),
        VFWDFEXPORT(WdfIoResourceListUpdateDescriptor),
        VFWDFEXPORT(WdfIoResourceListGetCount),
        VFWDFEXPORT(WdfIoResourceListGetDescriptor),
        VFWDFEXPORT(WdfIoResourceListRemove),
        VFWDFEXPORT(WdfIoResourceListRemoveByDescriptor),
        VFWDFEXPORT(WdfCmResourceListAppendDescriptor),
        VFWDFEXPORT(WdfCmResourceListInsertDescriptor),
        VFWDFEXPORT(WdfCmResourceListGetCount),
        VFWDFEXPORT(WdfCmResourceListGetDescriptor),
        VFWDFEXPORT(WdfCmResourceListRemove),
        VFWDFEXPORT(WdfCmResourceListRemoveByDescriptor),
        VFWDFEXPORT(WdfStringCreate),
        VFWDFEXPORT(WdfStringGetUnicodeString),
        VFWDFEXPORT(WdfObjectAcquireLock),
        VFWDFEXPORT(WdfObjectReleaseLock),
        VFWDFEXPORT(WdfWaitLockCreate),
        VFWDFEXPORT(WdfWaitLockAcquire),
        VFWDFEXPORT(WdfWaitLockRelease),
        VFWDFEXPORT(WdfSpinLockCreate),
        VFWDFEXPORT(WdfSpinLockAcquire),
        VFWDFEXPORT(WdfSpinLockRelease),
        VFWDFEXPORT(WdfTimerCreate),
        VFWDFEXPORT(WdfTimerStart),
        VFWDFEXPORT(WdfTimerStop),
        VFWDFEXPORT(WdfTimerGetParentObject),
        VFWDFEXPORT(WdfUsbTargetDeviceCreate),
        VFWDFEXPORT(WdfUsbTargetDeviceRetrieveInformation),
        VFWDFEXPORT(WdfUsbTargetDeviceGetDeviceDescriptor),
        VFWDFEXPORT(WdfUsbTargetDeviceRetrieveConfigDescriptor),
        VFWDFEXPORT(WdfUsbTargetDeviceQueryString),
        VFWDFEXPORT(WdfUsbTargetDeviceAllocAndQueryString),
        VFWDFEXPORT(WdfUsbTargetDeviceFormatRequestForString),
        VFWDFEXPORT(WdfUsbTargetDeviceGetNumInterfaces),
        VFWDFEXPORT(WdfUsbTargetDeviceSelectConfig),
        VFWDFEXPORT(WdfUsbTargetDeviceWdmGetConfigurationHandle),
        VFWDFEXPORT(WdfUsbTargetDeviceRetrieveCurrentFrameNumber),
        VFWDFEXPORT(WdfUsbTargetDeviceSendControlTransferSynchronously),
        VFWDFEXPORT(WdfUsbTargetDeviceFormatRequestForControlTransfer),
        VFWDFEXPORT(WdfUsbTargetDeviceIsConnectedSynchronous),
        VFWDFEXPORT(WdfUsbTargetDeviceResetPortSynchronously),
        VFWDFEXPORT(WdfUsbTargetDeviceCyclePortSynchronously),
        VFWDFEXPORT(WdfUsbTargetDeviceFormatRequestForCyclePort),
        VFWDFEXPORT(WdfUsbTargetDeviceSendUrbSynchronously),
        VFWDFEXPORT(WdfUsbTargetDeviceFormatRequestForUrb),
        VFWDFEXPORT(WdfUsbTargetPipeGetInformation),
        VFWDFEXPORT(WdfUsbTargetPipeIsInEndpoint),
        VFWDFEXPORT(WdfUsbTargetPipeIsOutEndpoint),
        VFWDFEXPORT(WdfUsbTargetPipeGetType),
        VFWDFEXPORT(WdfUsbTargetPipeSetNoMaximumPacketSizeCheck),
        VFWDFEXPORT(WdfUsbTargetPipeWriteSynchronously),
        VFWDFEXPORT(WdfUsbTargetPipeFormatRequestForWrite),
        VFWDFEXPORT(WdfUsbTargetPipeReadSynchronously),
        VFWDFEXPORT(WdfUsbTargetPipeFormatRequestForRead),
        VFWDFEXPORT(WdfUsbTargetPipeConfigContinuousReader),
        VFWDFEXPORT(WdfUsbTargetPipeAbortSynchronously),
        VFWDFEXPORT(WdfUsbTargetPipeFormatRequestForAbort),
        VFWDFEXPORT(WdfUsbTargetPipeResetSynchronously),
        VFWDFEXPORT(WdfUsbTargetPipeFormatRequestForReset),
        VFWDFEXPORT(WdfUsbTargetPipeSendUrbSynchronously),
        VFWDFEXPORT(WdfUsbTargetPipeFormatRequestForUrb),
        VFWDFEXPORT(WdfUsbInterfaceGetInterfaceNumber),
        VFWDFEXPORT(WdfUsbInterfaceGetNumEndpoints),
        VFWDFEXPORT(WdfUsbInterfaceGetDescriptor),
        VFWDFEXPORT(WdfUsbInterfaceSelectSetting),
        VFWDFEXPORT(WdfUsbInterfaceGetEndpointInformation),
        VFWDFEXPORT(WdfUsbTargetDeviceGetInterface),
        VFWDFEXPORT(WdfUsbInterfaceGetConfiguredSettingIndex),
        VFWDFEXPORT(WdfUsbInterfaceGetNumConfiguredPipes),
        VFWDFEXPORT(WdfUsbInterfaceGetConfiguredPipe),
        VFWDFEXPORT(WdfUsbTargetPipeWdmGetPipeHandle),
        VFWDFEXPORT(WdfVerifierDbgBreakPoint),
        VFWDFEXPORT(WdfVerifierKeBugCheck),
        VFWDFEXPORT(WdfWmiProviderCreate),
        VFWDFEXPORT(WdfWmiProviderGetDevice),
        VFWDFEXPORT(WdfWmiProviderIsEnabled),
        VFWDFEXPORT(WdfWmiProviderGetTracingHandle),
        VFWDFEXPORT(WdfWmiInstanceCreate),
        VFWDFEXPORT(WdfWmiInstanceRegister),
        VFWDFEXPORT(WdfWmiInstanceDeregister),
        VFWDFEXPORT(WdfWmiInstanceGetDevice),
        VFWDFEXPORT(WdfWmiInstanceGetProvider),
        VFWDFEXPORT(WdfWmiInstanceFireEvent),
        VFWDFEXPORT(WdfWorkItemCreate),
        VFWDFEXPORT(WdfWorkItemEnqueue),
        VFWDFEXPORT(WdfWorkItemGetParentObject),
        VFWDFEXPORT(WdfWorkItemFlush),
        VFWDFEXPORT(WdfCommonBufferCreateWithConfig),
        VFWDFEXPORT(WdfDmaEnablerGetFragmentLength),
        VFWDFEXPORT(WdfDmaEnablerWdmGetDmaAdapter),
        VFWDFEXPORT(WdfUsbInterfaceGetNumSettings),
        VFWDFEXPORT(WdfDeviceRemoveDependentUsageDeviceObject),
        VFWDFEXPORT(WdfDeviceGetSystemPowerAction),
        VFWDFEXPORT(WdfInterruptSetExtendedPolicy),
        VFWDFEXPORT(WdfIoQueueAssignForwardProgressPolicy),
        VFWDFEXPORT(WdfPdoInitAssignContainerID),
        VFWDFEXPORT(WdfPdoInitAllowForwardingRequestToParent),
        VFWDFEXPORT(WdfRequestMarkCancelableEx),
        VFWDFEXPORT(WdfRequestIsReserved),
        VFWDFEXPORT(WdfRequestForwardToParentDeviceIoQueue),
        VFWDFEXPORT(WdfCxDeviceInitAllocate),
        VFWDFEXPORT(WdfCxDeviceInitAssignWdmIrpPreprocessCallback),
        VFWDFEXPORT(WdfCxDeviceInitSetIoInCallerContextCallback),
        VFWDFEXPORT(WdfCxDeviceInitSetRequestAttributes),
        VFWDFEXPORT(WdfCxDeviceInitSetFileObjectConfig),
        VFWDFEXPORT(WdfDeviceWdmDispatchIrp),
        VFWDFEXPORT(WdfDeviceWdmDispatchIrpToIoQueue),
        VFWDFEXPORT(WdfDeviceInitSetRemoveLockOptions),
        VFWDFEXPORT(WdfDeviceConfigureWdmIrpDispatchCallback),
        VFWDFEXPORT(WdfDmaEnablerConfigureSystemProfile),
        VFWDFEXPORT(WdfDmaTransactionInitializeUsingOffset),
        VFWDFEXPORT(WdfDmaTransactionGetTransferInfo),
        VFWDFEXPORT(WdfDmaTransactionSetChannelConfigurationCallback),
        VFWDFEXPORT(WdfDmaTransactionSetTransferCompleteCallback),
        VFWDFEXPORT(WdfDmaTransactionSetImmediateExecution),
        VFWDFEXPORT(WdfDmaTransactionAllocateResources),
        VFWDFEXPORT(WdfDmaTransactionSetDeviceAddressOffset),
        VFWDFEXPORT(WdfDmaTransactionFreeResources),
        VFWDFEXPORT(WdfDmaTransactionCancel),
        VFWDFEXPORT(WdfDmaTransactionWdmGetTransferContext),
        VFWDFEXPORT(WdfInterruptQueueWorkItemForIsr),
        VFWDFEXPORT(WdfInterruptTryToAcquireLock),
        VFWDFEXPORT(WdfIoQueueStopAndPurge),
        VFWDFEXPORT(WdfIoQueueStopAndPurgeSynchronously),
        VFWDFEXPORT(WdfIoTargetPurge),
        VFWDFEXPORT(WdfUsbTargetDeviceCreateWithParameters),
        VFWDFEXPORT(WdfUsbTargetDeviceQueryUsbCapability),
        VFWDFEXPORT(WdfUsbTargetDeviceCreateUrb),
        VFWDFEXPORT(WdfUsbTargetDeviceCreateIsochUrb),
        VFWDFEXPORT(WdfDeviceWdmAssignPowerFrameworkSettings),
        VFWDFEXPORT(WdfDmaTransactionStopSystemTransfer),
        VFWDFEXPORT(WdfCxVerifierKeBugCheck),
        VFWDFEXPORT(WdfInterruptReportActive),
        VFWDFEXPORT(WdfInterruptReportInactive),
        VFWDFEXPORT(WdfDeviceInitSetReleaseHardwareOrderOnFailure),
        VFWDFEXPORT(WdfGetTriageInfo),
        VFWDFEXPORT(WdfDeviceInitSetIoTypeEx),
        VFWDFEXPORT(WdfDeviceQueryPropertyEx),
        VFWDFEXPORT(WdfDeviceAllocAndQueryPropertyEx),
        VFWDFEXPORT(WdfDeviceAssignProperty),
        VFWDFEXPORT(WdfFdoInitQueryPropertyEx),
        VFWDFEXPORT(WdfFdoInitAllocAndQueryPropertyEx),
        VFWDFEXPORT(WdfDeviceStopIdleActual),
        VFWDFEXPORT(WdfDeviceResumeIdleActual),
        VFWDFEXPORT(WdfDeviceGetSelfIoTarget),
        VFWDFEXPORT(WdfDeviceInitAllowSelfIoTarget),
        VFWDFEXPORT(WdfIoTargetSelfAssignDefaultIoQueue),
        VFWDFEXPORT(WdfDeviceOpenDevicemapKey),
    }
};

#endif // VF_FX_DYNAMICS_GENERATE_TABLE

#endif // _VFFXDYNAMICS_H_

