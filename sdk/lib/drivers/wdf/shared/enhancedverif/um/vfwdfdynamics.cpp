/*++

Module Name: VfWdfDynamics.cpp

Abstract:
    Generated implementation for WDF API Verifier hooks

Environment:
    user mode only

--*/

#include "fxmin.hpp"
#include "vfpriv.hpp"

extern "C" {
#include "FxDynamics.h"
}


extern "C" {
extern WDFVERSION WdfVersion;
}

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(FX_ENHANCED_VERIFIER_SECTION_NAME,  \
    VFWDFEXPORT(WdfCollectionCreate), \
    VFWDFEXPORT(WdfCollectionGetCount), \
    VFWDFEXPORT(WdfCollectionAdd), \
    VFWDFEXPORT(WdfCollectionRemove), \
    VFWDFEXPORT(WdfCollectionRemoveItem), \
    VFWDFEXPORT(WdfCollectionGetItem), \
    VFWDFEXPORT(WdfCollectionGetFirstItem), \
    VFWDFEXPORT(WdfCollectionGetLastItem), \
    VFWDFEXPORT(WdfCxDeviceInitAllocate), \
    VFWDFEXPORT(WdfCxDeviceInitSetRequestAttributes), \
    VFWDFEXPORT(WdfCxDeviceInitSetFileObjectConfig), \
    VFWDFEXPORT(WdfCxVerifierKeBugCheck), \
    VFWDFEXPORT(WdfDeviceGetDeviceState), \
    VFWDFEXPORT(WdfDeviceSetDeviceState), \
    VFWDFEXPORT(WdfDeviceWdmDispatchIrp), \
    VFWDFEXPORT(WdfDeviceWdmDispatchIrpToIoQueue), \
    VFWDFEXPORT(WdfDeviceGetDriver), \
    VFWDFEXPORT(WdfDeviceGetIoTarget), \
    VFWDFEXPORT(WdfDeviceAssignS0IdleSettings), \
    VFWDFEXPORT(WdfDeviceAssignSxWakeSettings), \
    VFWDFEXPORT(WdfDeviceOpenRegistryKey), \
    VFWDFEXPORT(WdfDeviceOpenDevicemapKey), \
    VFWDFEXPORT(WdfDeviceInitSetPnpPowerEventCallbacks), \
    VFWDFEXPORT(WdfDeviceInitSetPowerPolicyEventCallbacks), \
    VFWDFEXPORT(WdfDeviceInitSetPowerPolicyOwnership), \
    VFWDFEXPORT(WdfDeviceInitSetIoType), \
    VFWDFEXPORT(WdfDeviceInitSetFileObjectConfig), \
    VFWDFEXPORT(WdfDeviceInitSetRequestAttributes), \
    VFWDFEXPORT(WdfDeviceCreate), \
    VFWDFEXPORT(WdfDeviceSetStaticStopRemove), \
    VFWDFEXPORT(WdfDeviceCreateDeviceInterface), \
    VFWDFEXPORT(WdfDeviceSetDeviceInterfaceState), \
    VFWDFEXPORT(WdfDeviceRetrieveDeviceInterfaceString), \
    VFWDFEXPORT(WdfDeviceCreateSymbolicLink), \
    VFWDFEXPORT(WdfDeviceQueryProperty), \
    VFWDFEXPORT(WdfDeviceAllocAndQueryProperty), \
    VFWDFEXPORT(WdfDeviceSetPnpCapabilities), \
    VFWDFEXPORT(WdfDeviceSetPowerCapabilities), \
    VFWDFEXPORT(WdfDeviceSetFailed), \
    VFWDFEXPORT(WdfDeviceStopIdleNoTrack), \
    VFWDFEXPORT(WdfDeviceResumeIdleNoTrack), \
    VFWDFEXPORT(WdfDeviceStopIdleActual), \
    VFWDFEXPORT(WdfDeviceResumeIdleActual), \
    VFWDFEXPORT(WdfDeviceGetFileObject), \
    VFWDFEXPORT(WdfDeviceGetDefaultQueue), \
    VFWDFEXPORT(WdfDeviceConfigureRequestDispatching), \
    VFWDFEXPORT(WdfDeviceConfigureWdmIrpDispatchCallback), \
    VFWDFEXPORT(WdfDeviceGetSystemPowerAction), \
    VFWDFEXPORT(WdfDeviceInitSetReleaseHardwareOrderOnFailure), \
    VFWDFEXPORT(WdfDeviceInitSetIoTypeEx), \
    VFWDFEXPORT(WdfDevicePostEvent), \
    VFWDFEXPORT(WdfDeviceMapIoSpace), \
    VFWDFEXPORT(WdfDeviceUnmapIoSpace), \
    VFWDFEXPORT(WdfDeviceGetHardwareRegisterMappedAddress), \
    VFWDFEXPORT(WdfDeviceReadFromHardware), \
    VFWDFEXPORT(WdfDeviceWriteToHardware), \
    VFWDFEXPORT(WdfDeviceAssignInterfaceProperty), \
    VFWDFEXPORT(WdfDeviceAllocAndQueryInterfaceProperty), \
    VFWDFEXPORT(WdfDeviceQueryInterfaceProperty), \
    VFWDFEXPORT(WdfDeviceGetDeviceStackIoType), \
    VFWDFEXPORT(WdfDeviceQueryPropertyEx), \
    VFWDFEXPORT(WdfDeviceAllocAndQueryPropertyEx), \
    VFWDFEXPORT(WdfDeviceAssignProperty), \
    VFWDFEXPORT(WdfDeviceGetSelfIoTarget), \
    VFWDFEXPORT(WdfDeviceInitAllowSelfIoTarget), \
    VFWDFEXPORT(WdfDriverCreate), \
    VFWDFEXPORT(WdfDriverGetRegistryPath), \
    VFWDFEXPORT(WdfDriverOpenParametersRegistryKey), \
    VFWDFEXPORT(WdfDriverRetrieveVersionString), \
    VFWDFEXPORT(WdfDriverIsVersionAvailable), \
    VFWDFEXPORT(WdfFdoInitOpenRegistryKey), \
    VFWDFEXPORT(WdfFdoInitQueryProperty), \
    VFWDFEXPORT(WdfFdoInitAllocAndQueryProperty), \
    VFWDFEXPORT(WdfFdoInitQueryPropertyEx), \
    VFWDFEXPORT(WdfFdoInitAllocAndQueryPropertyEx), \
    VFWDFEXPORT(WdfFdoInitSetFilter), \
    VFWDFEXPORT(WdfFileObjectGetFileName), \
    VFWDFEXPORT(WdfFileObjectGetDevice), \
    VFWDFEXPORT(WdfFileObjectGetInitiatorProcessId), \
    VFWDFEXPORT(WdfFileObjectGetRelatedFileObject), \
    VFWDFEXPORT(WdfDeviceInitEnableHidInterface), \
    VFWDFEXPORT(WdfDeviceHidNotifyPresence), \
    VFWDFEXPORT(WdfInterruptCreate), \
    VFWDFEXPORT(WdfInterruptQueueDpcForIsr), \
    VFWDFEXPORT(WdfInterruptQueueWorkItemForIsr), \
    VFWDFEXPORT(WdfInterruptSynchronize), \
    VFWDFEXPORT(WdfInterruptAcquireLock), \
    VFWDFEXPORT(WdfInterruptReleaseLock), \
    VFWDFEXPORT(WdfInterruptEnable), \
    VFWDFEXPORT(WdfInterruptDisable), \
    VFWDFEXPORT(WdfInterruptGetInfo), \
    VFWDFEXPORT(WdfInterruptSetPolicy), \
    VFWDFEXPORT(WdfInterruptSetExtendedPolicy), \
    VFWDFEXPORT(WdfInterruptGetDevice), \
    VFWDFEXPORT(WdfInterruptTryToAcquireLock), \
    VFWDFEXPORT(WdfIoQueueCreate), \
    VFWDFEXPORT(WdfIoQueueGetState), \
    VFWDFEXPORT(WdfIoQueueStart), \
    VFWDFEXPORT(WdfIoQueueStop), \
    VFWDFEXPORT(WdfIoQueueStopSynchronously), \
    VFWDFEXPORT(WdfIoQueueGetDevice), \
    VFWDFEXPORT(WdfIoQueueRetrieveNextRequest), \
    VFWDFEXPORT(WdfIoQueueRetrieveRequestByFileObject), \
    VFWDFEXPORT(WdfIoQueueFindRequest), \
    VFWDFEXPORT(WdfIoQueueRetrieveFoundRequest), \
    VFWDFEXPORT(WdfIoQueueDrainSynchronously), \
    VFWDFEXPORT(WdfIoQueueDrain), \
    VFWDFEXPORT(WdfIoQueuePurgeSynchronously), \
    VFWDFEXPORT(WdfIoQueuePurge), \
    VFWDFEXPORT(WdfIoQueueReadyNotify), \
    VFWDFEXPORT(WdfIoQueueStopAndPurge), \
    VFWDFEXPORT(WdfIoQueueStopAndPurgeSynchronously), \
    VFWDFEXPORT(WdfIoTargetCreate), \
    VFWDFEXPORT(WdfIoTargetOpen), \
    VFWDFEXPORT(WdfIoTargetCloseForQueryRemove), \
    VFWDFEXPORT(WdfIoTargetClose), \
    VFWDFEXPORT(WdfIoTargetStart), \
    VFWDFEXPORT(WdfIoTargetStop), \
    VFWDFEXPORT(WdfIoTargetPurge), \
    VFWDFEXPORT(WdfIoTargetGetState), \
    VFWDFEXPORT(WdfIoTargetGetDevice), \
    VFWDFEXPORT(WdfIoTargetWdmGetTargetFileHandle), \
    VFWDFEXPORT(WdfIoTargetSendReadSynchronously), \
    VFWDFEXPORT(WdfIoTargetFormatRequestForRead), \
    VFWDFEXPORT(WdfIoTargetSendWriteSynchronously), \
    VFWDFEXPORT(WdfIoTargetFormatRequestForWrite), \
    VFWDFEXPORT(WdfIoTargetSendIoctlSynchronously), \
    VFWDFEXPORT(WdfIoTargetFormatRequestForIoctl), \
    VFWDFEXPORT(WdfIoTargetSelfAssignDefaultIoQueue), \
    VFWDFEXPORT(WdfMemoryCreate), \
    VFWDFEXPORT(WdfMemoryCreatePreallocated), \
    VFWDFEXPORT(WdfMemoryGetBuffer), \
    VFWDFEXPORT(WdfMemoryAssignBuffer), \
    VFWDFEXPORT(WdfMemoryCopyToBuffer), \
    VFWDFEXPORT(WdfMemoryCopyFromBuffer), \
    VFWDFEXPORT(WdfObjectGetTypedContextWorker), \
    VFWDFEXPORT(WdfObjectAllocateContext), \
    VFWDFEXPORT(WdfObjectContextGetObject), \
    VFWDFEXPORT(WdfObjectReferenceActual), \
    VFWDFEXPORT(WdfObjectDereferenceActual), \
    VFWDFEXPORT(WdfObjectCreate), \
    VFWDFEXPORT(WdfObjectDelete), \
    VFWDFEXPORT(WdfObjectQuery), \
    VFWDFEXPORT(WdfRegistryOpenKey), \
    VFWDFEXPORT(WdfRegistryCreateKey), \
    VFWDFEXPORT(WdfRegistryClose), \
    VFWDFEXPORT(WdfRegistryWdmGetHandle), \
    VFWDFEXPORT(WdfRegistryRemoveKey), \
    VFWDFEXPORT(WdfRegistryRemoveValue), \
    VFWDFEXPORT(WdfRegistryQueryValue), \
    VFWDFEXPORT(WdfRegistryQueryMemory), \
    VFWDFEXPORT(WdfRegistryQueryMultiString), \
    VFWDFEXPORT(WdfRegistryQueryUnicodeString), \
    VFWDFEXPORT(WdfRegistryQueryString), \
    VFWDFEXPORT(WdfRegistryQueryULong), \
    VFWDFEXPORT(WdfRegistryAssignValue), \
    VFWDFEXPORT(WdfRegistryAssignMemory), \
    VFWDFEXPORT(WdfRegistryAssignMultiString), \
    VFWDFEXPORT(WdfRegistryAssignUnicodeString), \
    VFWDFEXPORT(WdfRegistryAssignString), \
    VFWDFEXPORT(WdfRegistryAssignULong), \
    VFWDFEXPORT(WdfRequestCreate), \
    VFWDFEXPORT(WdfRequestReuse), \
    VFWDFEXPORT(WdfRequestChangeTarget), \
    VFWDFEXPORT(WdfRequestFormatRequestUsingCurrentType), \
    VFWDFEXPORT(WdfRequestSend), \
    VFWDFEXPORT(WdfRequestGetStatus), \
    VFWDFEXPORT(WdfRequestMarkCancelable), \
    VFWDFEXPORT(WdfRequestMarkCancelableEx), \
    VFWDFEXPORT(WdfRequestUnmarkCancelable), \
    VFWDFEXPORT(WdfRequestIsCanceled), \
    VFWDFEXPORT(WdfRequestCancelSentRequest), \
    VFWDFEXPORT(WdfRequestIsFrom32BitProcess), \
    VFWDFEXPORT(WdfRequestSetCompletionRoutine), \
    VFWDFEXPORT(WdfRequestGetCompletionParams), \
    VFWDFEXPORT(WdfRequestAllocateTimer), \
    VFWDFEXPORT(WdfRequestComplete), \
    VFWDFEXPORT(WdfRequestCompleteWithInformation), \
    VFWDFEXPORT(WdfRequestGetParameters), \
    VFWDFEXPORT(WdfRequestRetrieveInputMemory), \
    VFWDFEXPORT(WdfRequestRetrieveOutputMemory), \
    VFWDFEXPORT(WdfRequestRetrieveInputBuffer), \
    VFWDFEXPORT(WdfRequestRetrieveOutputBuffer), \
    VFWDFEXPORT(WdfRequestSetInformation), \
    VFWDFEXPORT(WdfRequestGetInformation), \
    VFWDFEXPORT(WdfRequestGetFileObject), \
    VFWDFEXPORT(WdfRequestGetRequestorMode), \
    VFWDFEXPORT(WdfRequestForwardToIoQueue), \
    VFWDFEXPORT(WdfRequestGetIoQueue), \
    VFWDFEXPORT(WdfRequestRequeue), \
    VFWDFEXPORT(WdfRequestStopAcknowledge), \
    VFWDFEXPORT(WdfRequestImpersonate), \
    VFWDFEXPORT(WdfRequestGetRequestorProcessId), \
    VFWDFEXPORT(WdfRequestIsFromUserModeDriver), \
    VFWDFEXPORT(WdfRequestSetUserModeDriverInitiatedIo), \
    VFWDFEXPORT(WdfRequestGetUserModeDriverInitiatedIo), \
    VFWDFEXPORT(WdfRequestSetActivityId), \
    VFWDFEXPORT(WdfRequestRetrieveActivityId), \
    VFWDFEXPORT(WdfRequestGetEffectiveIoType), \
    VFWDFEXPORT(WdfCmResourceListGetCount), \
    VFWDFEXPORT(WdfCmResourceListGetDescriptor), \
    VFWDFEXPORT(WdfStringCreate), \
    VFWDFEXPORT(WdfStringGetUnicodeString), \
    VFWDFEXPORT(WdfObjectAcquireLock), \
    VFWDFEXPORT(WdfObjectReleaseLock), \
    VFWDFEXPORT(WdfWaitLockCreate), \
    VFWDFEXPORT(WdfWaitLockAcquire), \
    VFWDFEXPORT(WdfWaitLockRelease), \
    VFWDFEXPORT(WdfSpinLockCreate), \
    VFWDFEXPORT(WdfSpinLockAcquire), \
    VFWDFEXPORT(WdfSpinLockRelease), \
    VFWDFEXPORT(WdfTimerCreate), \
    VFWDFEXPORT(WdfTimerStart), \
    VFWDFEXPORT(WdfTimerStop), \
    VFWDFEXPORT(WdfTimerGetParentObject), \
    VFWDFEXPORT(WdfUsbTargetDeviceCreate), \
    VFWDFEXPORT(WdfUsbTargetDeviceCreateWithParameters), \
    VFWDFEXPORT(WdfUsbTargetDeviceRetrieveInformation), \
    VFWDFEXPORT(WdfUsbTargetDeviceGetDeviceDescriptor), \
    VFWDFEXPORT(WdfUsbTargetDeviceRetrieveConfigDescriptor), \
    VFWDFEXPORT(WdfUsbTargetDeviceQueryString), \
    VFWDFEXPORT(WdfUsbTargetDeviceAllocAndQueryString), \
    VFWDFEXPORT(WdfUsbTargetDeviceFormatRequestForString), \
    VFWDFEXPORT(WdfUsbTargetDeviceGetNumInterfaces), \
    VFWDFEXPORT(WdfUsbTargetDeviceSelectConfig), \
    VFWDFEXPORT(WdfUsbTargetDeviceSendControlTransferSynchronously), \
    VFWDFEXPORT(WdfUsbTargetDeviceFormatRequestForControlTransfer), \
    VFWDFEXPORT(WdfUsbTargetDeviceResetPortSynchronously), \
    VFWDFEXPORT(WdfUsbTargetDeviceQueryUsbCapability), \
    VFWDFEXPORT(WdfUsbTargetPipeGetInformation), \
    VFWDFEXPORT(WdfUsbTargetPipeIsInEndpoint), \
    VFWDFEXPORT(WdfUsbTargetPipeIsOutEndpoint), \
    VFWDFEXPORT(WdfUsbTargetPipeGetType), \
    VFWDFEXPORT(WdfUsbTargetPipeSetNoMaximumPacketSizeCheck), \
    VFWDFEXPORT(WdfUsbTargetPipeWriteSynchronously), \
    VFWDFEXPORT(WdfUsbTargetPipeFormatRequestForWrite), \
    VFWDFEXPORT(WdfUsbTargetPipeReadSynchronously), \
    VFWDFEXPORT(WdfUsbTargetPipeFormatRequestForRead), \
    VFWDFEXPORT(WdfUsbTargetPipeConfigContinuousReader), \
    VFWDFEXPORT(WdfUsbTargetPipeAbortSynchronously), \
    VFWDFEXPORT(WdfUsbTargetPipeFormatRequestForAbort), \
    VFWDFEXPORT(WdfUsbTargetPipeResetSynchronously), \
    VFWDFEXPORT(WdfUsbTargetPipeFormatRequestForReset), \
    VFWDFEXPORT(WdfUsbInterfaceGetInterfaceNumber), \
    VFWDFEXPORT(WdfUsbInterfaceGetNumEndpoints), \
    VFWDFEXPORT(WdfUsbInterfaceGetDescriptor), \
    VFWDFEXPORT(WdfUsbInterfaceGetNumSettings), \
    VFWDFEXPORT(WdfUsbInterfaceSelectSetting), \
    VFWDFEXPORT(WdfUsbInterfaceGetEndpointInformation), \
    VFWDFEXPORT(WdfUsbTargetDeviceGetInterface), \
    VFWDFEXPORT(WdfUsbInterfaceGetConfiguredSettingIndex), \
    VFWDFEXPORT(WdfUsbInterfaceGetNumConfiguredPipes), \
    VFWDFEXPORT(WdfUsbInterfaceGetConfiguredPipe), \
    VFWDFEXPORT(WdfVerifierDbgBreakPoint), \
    VFWDFEXPORT(WdfVerifierKeBugCheck), \
    VFWDFEXPORT(WdfGetTriageInfo), \
    VFWDFEXPORT(WdfWorkItemCreate), \
    VFWDFEXPORT(WdfWorkItemEnqueue), \
    VFWDFEXPORT(WdfWorkItemGetParentObject), \
    VFWDFEXPORT(WdfWorkItemFlush), \
    )
#endif   // #pragma alloc_text

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCOLLECTIONCREATE) WdfVersion.Functions.pfnWdfCollectionCreate)(DriverGlobals, CollectionAttributes, Collection);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
VFWDFEXPORT(WdfCollectionGetCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCOLLECTIONGETCOUNT) WdfVersion.Functions.pfnWdfCollectionGetCount)(DriverGlobals, Collection);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCOLLECTIONADD) WdfVersion.Functions.pfnWdfCollectionAdd)(DriverGlobals, Collection, Object);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFCOLLECTIONREMOVE) WdfVersion.Functions.pfnWdfCollectionRemove)(DriverGlobals, Collection, Item);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFCOLLECTIONREMOVEITEM) WdfVersion.Functions.pfnWdfCollectionRemoveItem)(DriverGlobals, Collection, Index);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCOLLECTIONGETITEM) WdfVersion.Functions.pfnWdfCollectionGetItem)(DriverGlobals, Collection, Index);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
VFWDFEXPORT(WdfCollectionGetFirstItem)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCOLLECTIONGETFIRSTITEM) WdfVersion.Functions.pfnWdfCollectionGetFirstItem)(DriverGlobals, Collection);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
VFWDFEXPORT(WdfCollectionGetLastItem)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOLLECTION Collection
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCOLLECTIONGETLASTITEM) WdfVersion.Functions.pfnWdfCollectionGetLastItem)(DriverGlobals, Collection);
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PWDFCXDEVICE_INIT
VFWDFEXPORT(WdfCxDeviceInitAllocate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCXDEVICEINITALLOCATE) WdfVersion.Functions.pfnWdfCxDeviceInitAllocate)(DriverGlobals, DeviceInit);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFCXDEVICEINITSETREQUESTATTRIBUTES) WdfVersion.Functions.pfnWdfCxDeviceInitSetRequestAttributes)(DriverGlobals, CxDeviceInit, RequestAttributes);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFCXDEVICEINITSETFILEOBJECTCONFIG) WdfVersion.Functions.pfnWdfCxDeviceInitSetFileObjectConfig)(DriverGlobals, CxDeviceInit, CxFileObjectConfig, FileObjectAttributes);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFCXVERIFIERKEBUGCHECK) WdfVersion.Functions.pfnWdfCxVerifierKeBugCheck)(DriverGlobals, Object, BugCheckCode, BugCheckParameter1, BugCheckParameter2, BugCheckParameter3, BugCheckParameter4);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEGETDEVICESTATE) WdfVersion.Functions.pfnWdfDeviceGetDeviceState)(DriverGlobals, Device, DeviceState);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICESETDEVICESTATE) WdfVersion.Functions.pfnWdfDeviceSetDeviceState)(DriverGlobals, Device, DeviceState);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEWDMDISPATCHIRP) WdfVersion.Functions.pfnWdfDeviceWdmDispatchIrp)(DriverGlobals, Device, Irp, DispatchContext);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEWDMDISPATCHIRPTOIOQUEUE) WdfVersion.Functions.pfnWdfDeviceWdmDispatchIrpToIoQueue)(DriverGlobals, Device, Irp, Queue, Flags);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDRIVER
VFWDFEXPORT(WdfDeviceGetDriver)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEGETDRIVER) WdfVersion.Functions.pfnWdfDeviceGetDriver)(DriverGlobals, Device);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFIOTARGET
VFWDFEXPORT(WdfDeviceGetIoTarget)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEGETIOTARGET) WdfVersion.Functions.pfnWdfDeviceGetIoTarget)(DriverGlobals, Device);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEASSIGNS0IDLESETTINGS) WdfVersion.Functions.pfnWdfDeviceAssignS0IdleSettings)(DriverGlobals, Device, Settings);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEASSIGNSXWAKESETTINGS) WdfVersion.Functions.pfnWdfDeviceAssignSxWakeSettings)(DriverGlobals, Device, Settings);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEOPENREGISTRYKEY) WdfVersion.Functions.pfnWdfDeviceOpenRegistryKey)(DriverGlobals, Device, DeviceInstanceKeyType, DesiredAccess, KeyAttributes, Key);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEOPENDEVICEMAPKEY) WdfVersion.Functions.pfnWdfDeviceOpenDevicemapKey)(DriverGlobals, Device, KeyName, DesiredAccess, KeyAttributes, Key);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETPNPPOWEREVENTCALLBACKS) WdfVersion.Functions.pfnWdfDeviceInitSetPnpPowerEventCallbacks)(DriverGlobals, DeviceInit, PnpPowerEventCallbacks);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETPOWERPOLICYEVENTCALLBACKS) WdfVersion.Functions.pfnWdfDeviceInitSetPowerPolicyEventCallbacks)(DriverGlobals, DeviceInit, PowerPolicyEventCallbacks);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETPOWERPOLICYOWNERSHIP) WdfVersion.Functions.pfnWdfDeviceInitSetPowerPolicyOwnership)(DriverGlobals, DeviceInit, IsPowerPolicyOwner);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETIOTYPE) WdfVersion.Functions.pfnWdfDeviceInitSetIoType)(DriverGlobals, DeviceInit, IoType);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETFILEOBJECTCONFIG) WdfVersion.Functions.pfnWdfDeviceInitSetFileObjectConfig)(DriverGlobals, DeviceInit, FileObjectConfig, FileObjectAttributes);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETREQUESTATTRIBUTES) WdfVersion.Functions.pfnWdfDeviceInitSetRequestAttributes)(DriverGlobals, DeviceInit, RequestAttributes);
}

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
    )
{
    VF_HOOK_PROCESS_INFO hookInfo;
    NTSTATUS status;

    PAGED_CODE_LOCKED();
    RtlZeroMemory(&hookInfo, sizeof(VF_HOOK_PROCESS_INFO));

    status = AddEventHooksWdfDeviceCreate(
        &hookInfo,
        DriverGlobals,
        DeviceInit,
        DeviceAttributes,
        Device);

    UNREFERENCED_PARAMETER(status);

    if (hookInfo.DonotCallKmdfLib) {
        return hookInfo.DdiCallStatus;
    }

    return ((PFN_WDFDEVICECREATE) WdfVersion.Functions.pfnWdfDeviceCreate)(DriverGlobals, DeviceInit, DeviceAttributes, Device);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICESETSTATICSTOPREMOVE) WdfVersion.Functions.pfnWdfDeviceSetStaticStopRemove)(DriverGlobals, Device, Stoppable);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICECREATEDEVICEINTERFACE) WdfVersion.Functions.pfnWdfDeviceCreateDeviceInterface)(DriverGlobals, Device, InterfaceClassGUID, ReferenceString);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICESETDEVICEINTERFACESTATE) WdfVersion.Functions.pfnWdfDeviceSetDeviceInterfaceState)(DriverGlobals, Device, InterfaceClassGUID, ReferenceString, IsInterfaceEnabled);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICERETRIEVEDEVICEINTERFACESTRING) WdfVersion.Functions.pfnWdfDeviceRetrieveDeviceInterfaceString)(DriverGlobals, Device, InterfaceClassGUID, ReferenceString, String);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICECREATESYMBOLICLINK) WdfVersion.Functions.pfnWdfDeviceCreateSymbolicLink)(DriverGlobals, Device, SymbolicLinkName);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEQUERYPROPERTY) WdfVersion.Functions.pfnWdfDeviceQueryProperty)(DriverGlobals, Device, DeviceProperty, BufferLength, PropertyBuffer, ResultLength);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEALLOCANDQUERYPROPERTY) WdfVersion.Functions.pfnWdfDeviceAllocAndQueryProperty)(DriverGlobals, Device, DeviceProperty, PoolType, PropertyMemoryAttributes, PropertyMemory);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICESETPNPCAPABILITIES) WdfVersion.Functions.pfnWdfDeviceSetPnpCapabilities)(DriverGlobals, Device, PnpCapabilities);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICESETPOWERCAPABILITIES) WdfVersion.Functions.pfnWdfDeviceSetPowerCapabilities)(DriverGlobals, Device, PowerCapabilities);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICESETFAILED) WdfVersion.Functions.pfnWdfDeviceSetFailed)(DriverGlobals, Device, FailedAction);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICESTOPIDLENOTRACK) WdfVersion.Functions.pfnWdfDeviceStopIdleNoTrack)(DriverGlobals, Device, WaitForD0);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceResumeIdleNoTrack)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICERESUMEIDLENOTRACK) WdfVersion.Functions.pfnWdfDeviceResumeIdleNoTrack)(DriverGlobals, Device);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICESTOPIDLEACTUAL) WdfVersion.Functions.pfnWdfDeviceStopIdleActual)(DriverGlobals, Device, WaitForD0, Tag, Line, File);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICERESUMEIDLEACTUAL) WdfVersion.Functions.pfnWdfDeviceResumeIdleActual)(DriverGlobals, Device, Tag, Line, File);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEGETFILEOBJECT) WdfVersion.Functions.pfnWdfDeviceGetFileObject)(DriverGlobals, Device, FileObject);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFQUEUE
VFWDFEXPORT(WdfDeviceGetDefaultQueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEGETDEFAULTQUEUE) WdfVersion.Functions.pfnWdfDeviceGetDefaultQueue)(DriverGlobals, Device);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICECONFIGUREREQUESTDISPATCHING) WdfVersion.Functions.pfnWdfDeviceConfigureRequestDispatching)(DriverGlobals, Device, Queue, RequestType);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICECONFIGUREWDMIRPDISPATCHCALLBACK) WdfVersion.Functions.pfnWdfDeviceConfigureWdmIrpDispatchCallback)(DriverGlobals, Device, Driver, MajorFunction, EvtDeviceWdmIrpDisptach, DriverContext);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
POWER_ACTION
VFWDFEXPORT(WdfDeviceGetSystemPowerAction)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEGETSYSTEMPOWERACTION) WdfVersion.Functions.pfnWdfDeviceGetSystemPowerAction)(DriverGlobals, Device);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETRELEASEHARDWAREORDERONFAILURE) WdfVersion.Functions.pfnWdfDeviceInitSetReleaseHardwareOrderOnFailure)(DriverGlobals, DeviceInit, ReleaseHardwareOrderOnFailure);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETIOTYPEEX) WdfVersion.Functions.pfnWdfDeviceInitSetIoTypeEx)(DriverGlobals, DeviceInit, IoTypeConfig);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDevicePostEvent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    REFGUID EventGuid,
    _In_
    WDF_EVENT_TYPE WdfEventType,
    _In_reads_bytes_(DataSizeCb)
    BYTE* Data,
    _In_
    ULONG DataSizeCb
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEPOSTEVENT) WdfVersion.Functions.pfnWdfDevicePostEvent)(DriverGlobals, Device, EventGuid, WdfEventType, Data, DataSizeCb);
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceMapIoSpace)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PHYSICAL_ADDRESS PhysicalAddress,
    _In_
    SIZE_T NumberOfBytes,
    _In_
    MEMORY_CACHING_TYPE CacheType,
    _Out_
    PVOID* PseudoBaseAddress
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEMAPIOSPACE) WdfVersion.Functions.pfnWdfDeviceMapIoSpace)(DriverGlobals, Device, PhysicalAddress, NumberOfBytes, CacheType, PseudoBaseAddress);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceUnmapIoSpace)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PVOID PseudoBaseAddress,
    _In_
    SIZE_T NumberOfBytes
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEUNMAPIOSPACE) WdfVersion.Functions.pfnWdfDeviceUnmapIoSpace)(DriverGlobals, Device, PseudoBaseAddress, NumberOfBytes);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PVOID
VFWDFEXPORT(WdfDeviceGetHardwareRegisterMappedAddress)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PVOID PseudoBaseAddress
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEGETHARDWAREREGISTERMAPPEDADDRESS) WdfVersion.Functions.pfnWdfDeviceGetHardwareRegisterMappedAddress)(DriverGlobals, Device, PseudoBaseAddress);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
SIZE_T
VFWDFEXPORT(WdfDeviceReadFromHardware)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDF_DEVICE_HWACCESS_TARGET_TYPE Type,
    _In_
    WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
    _In_
    PVOID TargetAddress,
    _Out_writes_all_opt_(Count)
    PVOID Buffer,
    _In_opt_
    ULONG Count
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEREADFROMHARDWARE) WdfVersion.Functions.pfnWdfDeviceReadFromHardware)(DriverGlobals, Device, Type, Size, TargetAddress, Buffer, Count);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceWriteToHardware)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDF_DEVICE_HWACCESS_TARGET_TYPE Type,
    _In_
    WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
    _In_
    PVOID TargetAddress,
    _In_
    SIZE_T Value,
    _In_reads_opt_(Count)
    PVOID Buffer,
    _In_opt_
    ULONG Count
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEWRITETOHARDWARE) WdfVersion.Functions.pfnWdfDeviceWriteToHardware)(DriverGlobals, Device, Type, Size, TargetAddress, Value, Buffer, Count);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceAssignInterfaceProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_INTERFACE_PROPERTY_DATA PropertyData,
    _In_
    DEVPROPTYPE Type,
    _In_
    ULONG BufferLength,
    _In_reads_bytes_opt_(BufferLength)
    PVOID PropertyBuffer
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEASSIGNINTERFACEPROPERTY) WdfVersion.Functions.pfnWdfDeviceAssignInterfaceProperty)(DriverGlobals, Device, PropertyData, Type, BufferLength, PropertyBuffer);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceAllocAndQueryInterfaceProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_INTERFACE_PROPERTY_DATA PropertyData,
    _In_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_
    WDFMEMORY* PropertyMemory,
    _Out_
    PDEVPROPTYPE Type
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEALLOCANDQUERYINTERFACEPROPERTY) WdfVersion.Functions.pfnWdfDeviceAllocAndQueryInterfaceProperty)(DriverGlobals, Device, PropertyData, PoolType, PropertyMemoryAttributes, PropertyMemory, Type);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceQueryInterfaceProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_INTERFACE_PROPERTY_DATA PropertyData,
    _In_
    ULONG BufferLength,
    _Out_writes_bytes_opt_(BufferLength)
    PVOID PropertyBuffer,
    _Out_
    PULONG ResultLength,
    _Out_
    PDEVPROPTYPE Type
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEQUERYINTERFACEPROPERTY) WdfVersion.Functions.pfnWdfDeviceQueryInterfaceProperty)(DriverGlobals, Device, PropertyData, BufferLength, PropertyBuffer, ResultLength, Type);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceGetDeviceStackIoType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _Out_
    WDF_DEVICE_IO_TYPE* ReadWriteIoType,
    _Out_
    WDF_DEVICE_IO_TYPE* IoControlIoType
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEGETDEVICESTACKIOTYPE) WdfVersion.Functions.pfnWdfDeviceGetDeviceStackIoType)(DriverGlobals, Device, ReadWriteIoType, IoControlIoType);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEQUERYPROPERTYEX) WdfVersion.Functions.pfnWdfDeviceQueryPropertyEx)(DriverGlobals, Device, DeviceProperty, BufferLength, PropertyBuffer, RequiredSize, Type);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEALLOCANDQUERYPROPERTYEX) WdfVersion.Functions.pfnWdfDeviceAllocAndQueryPropertyEx)(DriverGlobals, Device, DeviceProperty, PoolType, PropertyMemoryAttributes, PropertyMemory, Type);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEASSIGNPROPERTY) WdfVersion.Functions.pfnWdfDeviceAssignProperty)(DriverGlobals, Device, DeviceProperty, Type, Size, Data);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFIOTARGET
VFWDFEXPORT(WdfDeviceGetSelfIoTarget)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEGETSELFIOTARGET) WdfVersion.Functions.pfnWdfDeviceGetSelfIoTarget)(DriverGlobals, Device);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitAllowSelfIoTarget)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITALLOWSELFIOTARGET) WdfVersion.Functions.pfnWdfDeviceInitAllowSelfIoTarget)(DriverGlobals, DeviceInit);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDRIVERCREATE) WdfVersion.Functions.pfnWdfDriverCreate)(DriverGlobals, DriverObject, RegistryPath, DriverAttributes, DriverConfig, Driver);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PWSTR
VFWDFEXPORT(WdfDriverGetRegistryPath)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDRIVERGETREGISTRYPATH) WdfVersion.Functions.pfnWdfDriverGetRegistryPath)(DriverGlobals, Driver);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDRIVEROPENPARAMETERSREGISTRYKEY) WdfVersion.Functions.pfnWdfDriverOpenParametersRegistryKey)(DriverGlobals, Driver, DesiredAccess, KeyAttributes, Key);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDRIVERRETRIEVEVERSIONSTRING) WdfVersion.Functions.pfnWdfDriverRetrieveVersionString)(DriverGlobals, Driver, String);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDRIVERISVERSIONAVAILABLE) WdfVersion.Functions.pfnWdfDriverIsVersionAvailable)(DriverGlobals, Driver, VersionAvailableParams);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFDOINITOPENREGISTRYKEY) WdfVersion.Functions.pfnWdfFdoInitOpenRegistryKey)(DriverGlobals, DeviceInit, DeviceInstanceKeyType, DesiredAccess, KeyAttributes, Key);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFDOINITQUERYPROPERTY) WdfVersion.Functions.pfnWdfFdoInitQueryProperty)(DriverGlobals, DeviceInit, DeviceProperty, BufferLength, PropertyBuffer, ResultLength);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFDOINITALLOCANDQUERYPROPERTY) WdfVersion.Functions.pfnWdfFdoInitAllocAndQueryProperty)(DriverGlobals, DeviceInit, DeviceProperty, PoolType, PropertyMemoryAttributes, PropertyMemory);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFDOINITQUERYPROPERTYEX) WdfVersion.Functions.pfnWdfFdoInitQueryPropertyEx)(DriverGlobals, DeviceInit, DeviceProperty, BufferLength, PropertyBuffer, ResultLength, Type);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFDOINITALLOCANDQUERYPROPERTYEX) WdfVersion.Functions.pfnWdfFdoInitAllocAndQueryPropertyEx)(DriverGlobals, DeviceInit, DeviceProperty, PoolType, PropertyMemoryAttributes, PropertyMemory, Type);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfFdoInitSetFilter)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFFDOINITSETFILTER) WdfVersion.Functions.pfnWdfFdoInitSetFilter)(DriverGlobals, DeviceInit);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
PUNICODE_STRING
VFWDFEXPORT(WdfFileObjectGetFileName)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFILEOBJECTGETFILENAME) WdfVersion.Functions.pfnWdfFileObjectGetFileName)(DriverGlobals, FileObject);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfFileObjectGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFILEOBJECTGETDEVICE) WdfVersion.Functions.pfnWdfFileObjectGetDevice)(DriverGlobals, FileObject);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
ULONG
VFWDFEXPORT(WdfFileObjectGetInitiatorProcessId)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFILEOBJECTGETINITIATORPROCESSID) WdfVersion.Functions.pfnWdfFileObjectGetInitiatorProcessId)(DriverGlobals, FileObject);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
WDFFILEOBJECT
VFWDFEXPORT(WdfFileObjectGetRelatedFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFILEOBJECTGETRELATEDFILEOBJECT) WdfVersion.Functions.pfnWdfFileObjectGetRelatedFileObject)(DriverGlobals, FileObject);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitEnableHidInterface)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITENABLEHIDINTERFACE) WdfVersion.Functions.pfnWdfDeviceInitEnableHidInterface)(DriverGlobals, DeviceInit);
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDeviceHidNotifyPresence)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    BOOLEAN IsPresent
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEHIDNOTIFYPRESENCE) WdfVersion.Functions.pfnWdfDeviceHidNotifyPresence)(DriverGlobals, Device, IsPresent);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFINTERRUPTCREATE) WdfVersion.Functions.pfnWdfInterruptCreate)(DriverGlobals, Device, Configuration, Attributes, Interrupt);
}

WDFAPI
BOOLEAN
VFWDFEXPORT(WdfInterruptQueueDpcForIsr)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFINTERRUPTQUEUEDPCFORISR) WdfVersion.Functions.pfnWdfInterruptQueueDpcForIsr)(DriverGlobals, Interrupt);
}

WDFAPI
BOOLEAN
VFWDFEXPORT(WdfInterruptQueueWorkItemForIsr)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFINTERRUPTQUEUEWORKITEMFORISR) WdfVersion.Functions.pfnWdfInterruptQueueWorkItemForIsr)(DriverGlobals, Interrupt);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFINTERRUPTSYNCHRONIZE) WdfVersion.Functions.pfnWdfInterruptSynchronize)(DriverGlobals, Interrupt, Callback, Context);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFINTERRUPTACQUIRELOCK) WdfVersion.Functions.pfnWdfInterruptAcquireLock)(DriverGlobals, Interrupt);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFINTERRUPTRELEASELOCK) WdfVersion.Functions.pfnWdfInterruptReleaseLock)(DriverGlobals, Interrupt);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfInterruptEnable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFINTERRUPTENABLE) WdfVersion.Functions.pfnWdfInterruptEnable)(DriverGlobals, Interrupt);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfInterruptDisable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFINTERRUPTDISABLE) WdfVersion.Functions.pfnWdfInterruptDisable)(DriverGlobals, Interrupt);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFINTERRUPTGETINFO) WdfVersion.Functions.pfnWdfInterruptGetInfo)(DriverGlobals, Interrupt, Info);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFINTERRUPTSETPOLICY) WdfVersion.Functions.pfnWdfInterruptSetPolicy)(DriverGlobals, Interrupt, Policy, Priority, TargetProcessorSet);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFINTERRUPTSETEXTENDEDPOLICY) WdfVersion.Functions.pfnWdfInterruptSetExtendedPolicy)(DriverGlobals, Interrupt, PolicyAndGroup);
}

WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfInterruptGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFINTERRUPTGETDEVICE) WdfVersion.Functions.pfnWdfInterruptGetDevice)(DriverGlobals, Interrupt);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFINTERRUPTTRYTOACQUIRELOCK) WdfVersion.Functions.pfnWdfInterruptTryToAcquireLock)(DriverGlobals, Interrupt);
}

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
    )
{
    VF_HOOK_PROCESS_INFO hookInfo;
    NTSTATUS status;

    PAGED_CODE_LOCKED();
    RtlZeroMemory(&hookInfo, sizeof(VF_HOOK_PROCESS_INFO));

    status = AddEventHooksWdfIoQueueCreate(
        &hookInfo,
        DriverGlobals,
        Device,
        Config,
        QueueAttributes,
        Queue);

    UNREFERENCED_PARAMETER(status);

    if (hookInfo.DonotCallKmdfLib) {
        return hookInfo.DdiCallStatus;
    }

    return ((PFN_WDFIOQUEUECREATE) WdfVersion.Functions.pfnWdfIoQueueCreate)(DriverGlobals, Device, Config, QueueAttributes, Queue);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOQUEUEGETSTATE) WdfVersion.Functions.pfnWdfIoQueueGetState)(DriverGlobals, Queue, QueueRequests, DriverRequests);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoQueueStart)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIOQUEUESTART) WdfVersion.Functions.pfnWdfIoQueueStart)(DriverGlobals, Queue);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIOQUEUESTOP) WdfVersion.Functions.pfnWdfIoQueueStop)(DriverGlobals, Queue, StopComplete, Context);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoQueueStopSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIOQUEUESTOPSYNCHRONOUSLY) WdfVersion.Functions.pfnWdfIoQueueStopSynchronously)(DriverGlobals, Queue);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfIoQueueGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOQUEUEGETDEVICE) WdfVersion.Functions.pfnWdfIoQueueGetDevice)(DriverGlobals, Queue);
}

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
    )
{
    PAGED_CODE_LOCKED();
    NTSTATUS rtn = ((PFN_WDFIOQUEUERETRIEVENEXTREQUEST) WdfVersion.Functions.pfnWdfIoQueueRetrieveNextRequest)(DriverGlobals, Queue, OutRequest);
    if (rtn == STATUS_SUCCESS) {
        PerfIoStart(*OutRequest);
    }
    return rtn;
}

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
    )
{
    PAGED_CODE_LOCKED();
    NTSTATUS rtn = ((PFN_WDFIOQUEUERETRIEVEREQUESTBYFILEOBJECT) WdfVersion.Functions.pfnWdfIoQueueRetrieveRequestByFileObject)(DriverGlobals, Queue, FileObject, OutRequest);
    if (rtn == STATUS_SUCCESS) {
        PerfIoStart(*OutRequest);
    }
    return rtn;
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOQUEUEFINDREQUEST) WdfVersion.Functions.pfnWdfIoQueueFindRequest)(DriverGlobals, Queue, FoundRequest, FileObject, Parameters, OutRequest);
}

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
    )
{
    PAGED_CODE_LOCKED();
    NTSTATUS rtn = ((PFN_WDFIOQUEUERETRIEVEFOUNDREQUEST) WdfVersion.Functions.pfnWdfIoQueueRetrieveFoundRequest)(DriverGlobals, Queue, FoundRequest, OutRequest);
    if (rtn == STATUS_SUCCESS) {
        PerfIoStart(*OutRequest);
    }
    return rtn;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoQueueDrainSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIOQUEUEDRAINSYNCHRONOUSLY) WdfVersion.Functions.pfnWdfIoQueueDrainSynchronously)(DriverGlobals, Queue);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIOQUEUEDRAIN) WdfVersion.Functions.pfnWdfIoQueueDrain)(DriverGlobals, Queue, DrainComplete, Context);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoQueuePurgeSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIOQUEUEPURGESYNCHRONOUSLY) WdfVersion.Functions.pfnWdfIoQueuePurgeSynchronously)(DriverGlobals, Queue);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIOQUEUEPURGE) WdfVersion.Functions.pfnWdfIoQueuePurge)(DriverGlobals, Queue, PurgeComplete, Context);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOQUEUEREADYNOTIFY) WdfVersion.Functions.pfnWdfIoQueueReadyNotify)(DriverGlobals, Queue, QueueReady, Context);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIOQUEUESTOPANDPURGE) WdfVersion.Functions.pfnWdfIoQueueStopAndPurge)(DriverGlobals, Queue, StopAndPurgeComplete, Context);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoQueueStopAndPurgeSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFQUEUE Queue
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIOQUEUESTOPANDPURGESYNCHRONOUSLY) WdfVersion.Functions.pfnWdfIoQueueStopAndPurgeSynchronously)(DriverGlobals, Queue);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETCREATE) WdfVersion.Functions.pfnWdfIoTargetCreate)(DriverGlobals, Device, IoTargetAttributes, IoTarget);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETOPEN) WdfVersion.Functions.pfnWdfIoTargetOpen)(DriverGlobals, IoTarget, OpenParams);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoTargetCloseForQueryRemove)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIOTARGETCLOSEFORQUERYREMOVE) WdfVersion.Functions.pfnWdfIoTargetCloseForQueryRemove)(DriverGlobals, IoTarget);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfIoTargetClose)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIOTARGETCLOSE) WdfVersion.Functions.pfnWdfIoTargetClose)(DriverGlobals, IoTarget);
}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfIoTargetStart)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETSTART) WdfVersion.Functions.pfnWdfIoTargetStart)(DriverGlobals, IoTarget);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIOTARGETSTOP) WdfVersion.Functions.pfnWdfIoTargetStop)(DriverGlobals, IoTarget, Action);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIOTARGETPURGE) WdfVersion.Functions.pfnWdfIoTargetPurge)(DriverGlobals, IoTarget, Action);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDF_IO_TARGET_STATE
VFWDFEXPORT(WdfIoTargetGetState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETGETSTATE) WdfVersion.Functions.pfnWdfIoTargetGetState)(DriverGlobals, IoTarget);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfIoTargetGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETGETDEVICE) WdfVersion.Functions.pfnWdfIoTargetGetDevice)(DriverGlobals, IoTarget);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
HANDLE
VFWDFEXPORT(WdfIoTargetWdmGetTargetFileHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETWDMGETTARGETFILEHANDLE) WdfVersion.Functions.pfnWdfIoTargetWdmGetTargetFileHandle)(DriverGlobals, IoTarget);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETSENDREADSYNCHRONOUSLY) WdfVersion.Functions.pfnWdfIoTargetSendReadSynchronously)(DriverGlobals, IoTarget, Request, OutputBuffer, DeviceOffset, RequestOptions, BytesRead);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETFORMATREQUESTFORREAD) WdfVersion.Functions.pfnWdfIoTargetFormatRequestForRead)(DriverGlobals, IoTarget, Request, OutputBuffer, OutputBufferOffset, DeviceOffset);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETSENDWRITESYNCHRONOUSLY) WdfVersion.Functions.pfnWdfIoTargetSendWriteSynchronously)(DriverGlobals, IoTarget, Request, InputBuffer, DeviceOffset, RequestOptions, BytesWritten);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETFORMATREQUESTFORWRITE) WdfVersion.Functions.pfnWdfIoTargetFormatRequestForWrite)(DriverGlobals, IoTarget, Request, InputBuffer, InputBufferOffset, DeviceOffset);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETSENDIOCTLSYNCHRONOUSLY) WdfVersion.Functions.pfnWdfIoTargetSendIoctlSynchronously)(DriverGlobals, IoTarget, Request, IoctlCode, InputBuffer, OutputBuffer, RequestOptions, BytesReturned);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETFORMATREQUESTFORIOCTL) WdfVersion.Functions.pfnWdfIoTargetFormatRequestForIoctl)(DriverGlobals, IoTarget, Request, IoctlCode, InputBuffer, InputBufferOffset, OutputBuffer, OutputBufferOffset);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETSELFASSIGNDEFAULTIOQUEUE) WdfVersion.Functions.pfnWdfIoTargetSelfAssignDefaultIoQueue)(DriverGlobals, IoTarget, Queue);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFMEMORYCREATE) WdfVersion.Functions.pfnWdfMemoryCreate)(DriverGlobals, Attributes, PoolType, PoolTag, BufferSize, Memory, Buffer);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFMEMORYCREATEPREALLOCATED) WdfVersion.Functions.pfnWdfMemoryCreatePreallocated)(DriverGlobals, Attributes, Buffer, BufferSize, Memory);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFMEMORYGETBUFFER) WdfVersion.Functions.pfnWdfMemoryGetBuffer)(DriverGlobals, Memory, BufferSize);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFMEMORYASSIGNBUFFER) WdfVersion.Functions.pfnWdfMemoryAssignBuffer)(DriverGlobals, Memory, Buffer, BufferSize);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFMEMORYCOPYTOBUFFER) WdfVersion.Functions.pfnWdfMemoryCopyToBuffer)(DriverGlobals, SourceMemory, SourceOffset, Buffer, NumBytesToCopyTo);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFMEMORYCOPYFROMBUFFER) WdfVersion.Functions.pfnWdfMemoryCopyFromBuffer)(DriverGlobals, DestinationMemory, DestinationOffset, Buffer, NumBytesToCopyFrom);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFOBJECTGETTYPEDCONTEXTWORKER) WdfVersion.Functions.pfnWdfObjectGetTypedContextWorker)(DriverGlobals, Handle, TypeInfo);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFOBJECTALLOCATECONTEXT) WdfVersion.Functions.pfnWdfObjectAllocateContext)(DriverGlobals, Handle, ContextAttributes, Context);
}

WDFAPI
WDFOBJECT
FASTCALL
VFWDFEXPORT(WdfObjectContextGetObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PVOID ContextPointer
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFOBJECTCONTEXTGETOBJECT) WdfVersion.Functions.pfnWdfObjectContextGetObject)(DriverGlobals, ContextPointer);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFOBJECTREFERENCEACTUAL) WdfVersion.Functions.pfnWdfObjectReferenceActual)(DriverGlobals, Handle, Tag, Line, File);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFOBJECTDEREFERENCEACTUAL) WdfVersion.Functions.pfnWdfObjectDereferenceActual)(DriverGlobals, Handle, Tag, Line, File);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFOBJECTCREATE) WdfVersion.Functions.pfnWdfObjectCreate)(DriverGlobals, Attributes, Object);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfObjectDelete)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFOBJECT Object
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFOBJECTDELETE) WdfVersion.Functions.pfnWdfObjectDelete)(DriverGlobals, Object);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFOBJECTQUERY) WdfVersion.Functions.pfnWdfObjectQuery)(DriverGlobals, Object, Guid, QueryBufferLength, QueryBuffer);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYOPENKEY) WdfVersion.Functions.pfnWdfRegistryOpenKey)(DriverGlobals, ParentKey, KeyName, DesiredAccess, KeyAttributes, Key);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYCREATEKEY) WdfVersion.Functions.pfnWdfRegistryCreateKey)(DriverGlobals, ParentKey, KeyName, DesiredAccess, CreateOptions, CreateDisposition, KeyAttributes, Key);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRegistryClose)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFREGISTRYCLOSE) WdfVersion.Functions.pfnWdfRegistryClose)(DriverGlobals, Key);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
HANDLE
VFWDFEXPORT(WdfRegistryWdmGetHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYWDMGETHANDLE) WdfVersion.Functions.pfnWdfRegistryWdmGetHandle)(DriverGlobals, Key);
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRegistryRemoveKey)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFKEY Key
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYREMOVEKEY) WdfVersion.Functions.pfnWdfRegistryRemoveKey)(DriverGlobals, Key);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYREMOVEVALUE) WdfVersion.Functions.pfnWdfRegistryRemoveValue)(DriverGlobals, Key, ValueName);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYQUERYVALUE) WdfVersion.Functions.pfnWdfRegistryQueryValue)(DriverGlobals, Key, ValueName, ValueLength, Value, ValueLengthQueried, ValueType);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYQUERYMEMORY) WdfVersion.Functions.pfnWdfRegistryQueryMemory)(DriverGlobals, Key, ValueName, PoolType, MemoryAttributes, Memory, ValueType);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYQUERYMULTISTRING) WdfVersion.Functions.pfnWdfRegistryQueryMultiString)(DriverGlobals, Key, ValueName, StringsAttributes, Collection);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYQUERYUNICODESTRING) WdfVersion.Functions.pfnWdfRegistryQueryUnicodeString)(DriverGlobals, Key, ValueName, ValueByteLength, Value);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYQUERYSTRING) WdfVersion.Functions.pfnWdfRegistryQueryString)(DriverGlobals, Key, ValueName, String);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYQUERYULONG) WdfVersion.Functions.pfnWdfRegistryQueryULong)(DriverGlobals, Key, ValueName, Value);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYASSIGNVALUE) WdfVersion.Functions.pfnWdfRegistryAssignValue)(DriverGlobals, Key, ValueName, ValueType, ValueLength, Value);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYASSIGNMEMORY) WdfVersion.Functions.pfnWdfRegistryAssignMemory)(DriverGlobals, Key, ValueName, ValueType, Memory, MemoryOffsets);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYASSIGNMULTISTRING) WdfVersion.Functions.pfnWdfRegistryAssignMultiString)(DriverGlobals, Key, ValueName, StringsCollection);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYASSIGNUNICODESTRING) WdfVersion.Functions.pfnWdfRegistryAssignUnicodeString)(DriverGlobals, Key, ValueName, Value);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYASSIGNSTRING) WdfVersion.Functions.pfnWdfRegistryAssignString)(DriverGlobals, Key, ValueName, String);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREGISTRYASSIGNULONG) WdfVersion.Functions.pfnWdfRegistryAssignULong)(DriverGlobals, Key, ValueName, Value);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTCREATE) WdfVersion.Functions.pfnWdfRequestCreate)(DriverGlobals, RequestAttributes, IoTarget, Request);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTREUSE) WdfVersion.Functions.pfnWdfRequestReuse)(DriverGlobals, Request, ReuseParams);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTCHANGETARGET) WdfVersion.Functions.pfnWdfRequestChangeTarget)(DriverGlobals, Request, IoTarget);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRequestFormatRequestUsingCurrentType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFREQUESTFORMATREQUESTUSINGCURRENTTYPE) WdfVersion.Functions.pfnWdfRequestFormatRequestUsingCurrentType)(DriverGlobals, Request);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTSEND) WdfVersion.Functions.pfnWdfRequestSend)(DriverGlobals, Request, Target, Options);
}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestGetStatus)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTGETSTATUS) WdfVersion.Functions.pfnWdfRequestGetStatus)(DriverGlobals, Request);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFREQUESTMARKCANCELABLE) WdfVersion.Functions.pfnWdfRequestMarkCancelable)(DriverGlobals, Request, EvtRequestCancel);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTMARKCANCELABLEEX) WdfVersion.Functions.pfnWdfRequestMarkCancelableEx)(DriverGlobals, Request, EvtRequestCancel);
}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestUnmarkCancelable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTUNMARKCANCELABLE) WdfVersion.Functions.pfnWdfRequestUnmarkCancelable)(DriverGlobals, Request);
}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfRequestIsCanceled)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTISCANCELED) WdfVersion.Functions.pfnWdfRequestIsCanceled)(DriverGlobals, Request);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfRequestCancelSentRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTCANCELSENTREQUEST) WdfVersion.Functions.pfnWdfRequestCancelSentRequest)(DriverGlobals, Request);
}

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfRequestIsFrom32BitProcess)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTISFROM32BITPROCESS) WdfVersion.Functions.pfnWdfRequestIsFrom32BitProcess)(DriverGlobals, Request);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFREQUESTSETCOMPLETIONROUTINE) WdfVersion.Functions.pfnWdfRequestSetCompletionRoutine)(DriverGlobals, Request, CompletionRoutine, CompletionContext);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFREQUESTGETCOMPLETIONPARAMS) WdfVersion.Functions.pfnWdfRequestGetCompletionParams)(DriverGlobals, Request, Params);
}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestAllocateTimer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTALLOCATETIMER) WdfVersion.Functions.pfnWdfRequestAllocateTimer)(DriverGlobals, Request);
}

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
    )
{
    PAGED_CODE_LOCKED();
    PerfIoComplete(Request);
    ((PFN_WDFREQUESTCOMPLETE) WdfVersion.Functions.pfnWdfRequestComplete)(DriverGlobals, Request, Status);
}

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
    )
{
    PAGED_CODE_LOCKED();
    PerfIoComplete(Request);
    ((PFN_WDFREQUESTCOMPLETEWITHINFORMATION) WdfVersion.Functions.pfnWdfRequestCompleteWithInformation)(DriverGlobals, Request, Status, Information);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFREQUESTGETPARAMETERS) WdfVersion.Functions.pfnWdfRequestGetParameters)(DriverGlobals, Request, Parameters);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTRETRIEVEINPUTMEMORY) WdfVersion.Functions.pfnWdfRequestRetrieveInputMemory)(DriverGlobals, Request, Memory);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTRETRIEVEOUTPUTMEMORY) WdfVersion.Functions.pfnWdfRequestRetrieveOutputMemory)(DriverGlobals, Request, Memory);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTRETRIEVEINPUTBUFFER) WdfVersion.Functions.pfnWdfRequestRetrieveInputBuffer)(DriverGlobals, Request, MinimumRequiredLength, Buffer, Length);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTRETRIEVEOUTPUTBUFFER) WdfVersion.Functions.pfnWdfRequestRetrieveOutputBuffer)(DriverGlobals, Request, MinimumRequiredSize, Buffer, Length);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFREQUESTSETINFORMATION) WdfVersion.Functions.pfnWdfRequestSetInformation)(DriverGlobals, Request, Information);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG_PTR
VFWDFEXPORT(WdfRequestGetInformation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTGETINFORMATION) WdfVersion.Functions.pfnWdfRequestGetInformation)(DriverGlobals, Request);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFFILEOBJECT
VFWDFEXPORT(WdfRequestGetFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTGETFILEOBJECT) WdfVersion.Functions.pfnWdfRequestGetFileObject)(DriverGlobals, Request);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
KPROCESSOR_MODE
VFWDFEXPORT(WdfRequestGetRequestorMode)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTGETREQUESTORMODE) WdfVersion.Functions.pfnWdfRequestGetRequestorMode)(DriverGlobals, Request);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTFORWARDTOIOQUEUE) WdfVersion.Functions.pfnWdfRequestForwardToIoQueue)(DriverGlobals, Request, DestinationQueue);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFQUEUE
VFWDFEXPORT(WdfRequestGetIoQueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTGETIOQUEUE) WdfVersion.Functions.pfnWdfRequestGetIoQueue)(DriverGlobals, Request);
}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestRequeue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTREQUEUE) WdfVersion.Functions.pfnWdfRequestRequeue)(DriverGlobals, Request);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFREQUESTSTOPACKNOWLEDGE) WdfVersion.Functions.pfnWdfRequestStopAcknowledge)(DriverGlobals, Request, Requeue);
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestImpersonate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    _In_
    PFN_WDF_REQUEST_IMPERSONATE EvtRequestImpersonate,
    _In_opt_
    PVOID Context
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTIMPERSONATE) WdfVersion.Functions.pfnWdfRequestImpersonate)(DriverGlobals, Request, ImpersonationLevel, EvtRequestImpersonate, Context);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
ULONG
VFWDFEXPORT(WdfRequestGetRequestorProcessId)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTGETREQUESTORPROCESSID) WdfVersion.Functions.pfnWdfRequestGetRequestorProcessId)(DriverGlobals, Request);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfRequestIsFromUserModeDriver)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTISFROMUSERMODEDRIVER) WdfVersion.Functions.pfnWdfRequestIsFromUserModeDriver)(DriverGlobals, Request);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRequestSetUserModeDriverInitiatedIo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    BOOLEAN IsUserModeDriverInitiated
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFREQUESTSETUSERMODEDRIVERINITIATEDIO) WdfVersion.Functions.pfnWdfRequestSetUserModeDriverInitiatedIo)(DriverGlobals, Request, IsUserModeDriverInitiated);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfRequestGetUserModeDriverInitiatedIo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTGETUSERMODEDRIVERINITIATEDIO) WdfVersion.Functions.pfnWdfRequestGetUserModeDriverInitiatedIo)(DriverGlobals, Request);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfRequestSetActivityId)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    LPGUID ActivityId
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFREQUESTSETACTIVITYID) WdfVersion.Functions.pfnWdfRequestSetActivityId)(DriverGlobals, Request, ActivityId);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfRequestRetrieveActivityId)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _Out_
    LPGUID ActivityId
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTRETRIEVEACTIVITYID) WdfVersion.Functions.pfnWdfRequestRetrieveActivityId)(DriverGlobals, Request, ActivityId);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
WDF_DEVICE_IO_TYPE
VFWDFEXPORT(WdfRequestGetEffectiveIoType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTGETEFFECTIVEIOTYPE) WdfVersion.Functions.pfnWdfRequestGetEffectiveIoType)(DriverGlobals, Request);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
VFWDFEXPORT(WdfCmResourceListGetCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCMRESLIST List
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCMRESOURCELISTGETCOUNT) WdfVersion.Functions.pfnWdfCmResourceListGetCount)(DriverGlobals, List);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCMRESOURCELISTGETDESCRIPTOR) WdfVersion.Functions.pfnWdfCmResourceListGetDescriptor)(DriverGlobals, List, Index);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFSTRINGCREATE) WdfVersion.Functions.pfnWdfStringCreate)(DriverGlobals, UnicodeString, StringAttributes, String);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFSTRINGGETUNICODESTRING) WdfVersion.Functions.pfnWdfStringGetUnicodeString)(DriverGlobals, String, UnicodeString);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFOBJECTACQUIRELOCK) WdfVersion.Functions.pfnWdfObjectAcquireLock)(DriverGlobals, Object);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFOBJECTRELEASELOCK) WdfVersion.Functions.pfnWdfObjectReleaseLock)(DriverGlobals, Object);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFWAITLOCKCREATE) WdfVersion.Functions.pfnWdfWaitLockCreate)(DriverGlobals, LockAttributes, Lock);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFWAITLOCKACQUIRE) WdfVersion.Functions.pfnWdfWaitLockAcquire)(DriverGlobals, Lock, Timeout);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFWAITLOCKRELEASE) WdfVersion.Functions.pfnWdfWaitLockRelease)(DriverGlobals, Lock);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFSPINLOCKCREATE) WdfVersion.Functions.pfnWdfSpinLockCreate)(DriverGlobals, SpinLockAttributes, SpinLock);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFSPINLOCKACQUIRE) WdfVersion.Functions.pfnWdfSpinLockAcquire)(DriverGlobals, SpinLock);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFSPINLOCKRELEASE) WdfVersion.Functions.pfnWdfSpinLockRelease)(DriverGlobals, SpinLock);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFTIMERCREATE) WdfVersion.Functions.pfnWdfTimerCreate)(DriverGlobals, Config, Attributes, Timer);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFTIMERSTART) WdfVersion.Functions.pfnWdfTimerStart)(DriverGlobals, Timer, DueTime);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFTIMERSTOP) WdfVersion.Functions.pfnWdfTimerStop)(DriverGlobals, Timer, Wait);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
VFWDFEXPORT(WdfTimerGetParentObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFTIMER Timer
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFTIMERGETPARENTOBJECT) WdfVersion.Functions.pfnWdfTimerGetParentObject)(DriverGlobals, Timer);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICECREATE) WdfVersion.Functions.pfnWdfUsbTargetDeviceCreate)(DriverGlobals, Device, Attributes, UsbDevice);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICECREATEWITHPARAMETERS) WdfVersion.Functions.pfnWdfUsbTargetDeviceCreateWithParameters)(DriverGlobals, Device, Config, Attributes, UsbDevice);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICERETRIEVEINFORMATION) WdfVersion.Functions.pfnWdfUsbTargetDeviceRetrieveInformation)(DriverGlobals, UsbDevice, Information);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFUSBTARGETDEVICEGETDEVICEDESCRIPTOR) WdfVersion.Functions.pfnWdfUsbTargetDeviceGetDeviceDescriptor)(DriverGlobals, UsbDevice, UsbDeviceDescriptor);
}

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
    )
{
    PAGED_CODE_LOCKED();
#pragma prefast(suppress: __WARNING_HIGH_PRIORITY_OVERFLOW_POSTCONDITION, "This is a verifier DDI hook routine and all it does is call original routine.")
    return ((PFN_WDFUSBTARGETDEVICERETRIEVECONFIGDESCRIPTOR) WdfVersion.Functions.pfnWdfUsbTargetDeviceRetrieveConfigDescriptor)(DriverGlobals, UsbDevice, ConfigDescriptor, ConfigDescriptorLength);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICEQUERYSTRING) WdfVersion.Functions.pfnWdfUsbTargetDeviceQueryString)(DriverGlobals, UsbDevice, Request, RequestOptions, String, NumCharacters, StringIndex, LangID);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICEALLOCANDQUERYSTRING) WdfVersion.Functions.pfnWdfUsbTargetDeviceAllocAndQueryString)(DriverGlobals, UsbDevice, StringMemoryAttributes, StringMemory, NumCharacters, StringIndex, LangID);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICEFORMATREQUESTFORSTRING) WdfVersion.Functions.pfnWdfUsbTargetDeviceFormatRequestForString)(DriverGlobals, UsbDevice, Request, Memory, Offset, StringIndex, LangID);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
UCHAR
VFWDFEXPORT(WdfUsbTargetDeviceGetNumInterfaces)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICEGETNUMINTERFACES) WdfVersion.Functions.pfnWdfUsbTargetDeviceGetNumInterfaces)(DriverGlobals, UsbDevice);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICESELECTCONFIG) WdfVersion.Functions.pfnWdfUsbTargetDeviceSelectConfig)(DriverGlobals, UsbDevice, PipeAttributes, Params);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICESENDCONTROLTRANSFERSYNCHRONOUSLY) WdfVersion.Functions.pfnWdfUsbTargetDeviceSendControlTransferSynchronously)(DriverGlobals, UsbDevice, Request, RequestOptions, SetupPacket, MemoryDescriptor, BytesTransferred);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICEFORMATREQUESTFORCONTROLTRANSFER) WdfVersion.Functions.pfnWdfUsbTargetDeviceFormatRequestForControlTransfer)(DriverGlobals, UsbDevice, Request, SetupPacket, TransferMemory, TransferOffset);
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfUsbTargetDeviceResetPortSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICERESETPORTSYNCHRONOUSLY) WdfVersion.Functions.pfnWdfUsbTargetDeviceResetPortSynchronously)(DriverGlobals, UsbDevice);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICEQUERYUSBCAPABILITY) WdfVersion.Functions.pfnWdfUsbTargetDeviceQueryUsbCapability)(DriverGlobals, UsbDevice, CapabilityType, CapabilityBufferLength, CapabilityBuffer, ResultLength);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFUSBTARGETPIPEGETINFORMATION) WdfVersion.Functions.pfnWdfUsbTargetPipeGetInformation)(DriverGlobals, Pipe, PipeInformation);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfUsbTargetPipeIsInEndpoint)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETPIPEISINENDPOINT) WdfVersion.Functions.pfnWdfUsbTargetPipeIsInEndpoint)(DriverGlobals, Pipe);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfUsbTargetPipeIsOutEndpoint)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETPIPEISOUTENDPOINT) WdfVersion.Functions.pfnWdfUsbTargetPipeIsOutEndpoint)(DriverGlobals, Pipe);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDF_USB_PIPE_TYPE
VFWDFEXPORT(WdfUsbTargetPipeGetType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETPIPEGETTYPE) WdfVersion.Functions.pfnWdfUsbTargetPipeGetType)(DriverGlobals, Pipe);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfUsbTargetPipeSetNoMaximumPacketSizeCheck)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE Pipe
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFUSBTARGETPIPESETNOMAXIMUMPACKETSIZECHECK) WdfVersion.Functions.pfnWdfUsbTargetPipeSetNoMaximumPacketSizeCheck)(DriverGlobals, Pipe);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETPIPEWRITESYNCHRONOUSLY) WdfVersion.Functions.pfnWdfUsbTargetPipeWriteSynchronously)(DriverGlobals, Pipe, Request, RequestOptions, MemoryDescriptor, BytesWritten);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETPIPEFORMATREQUESTFORWRITE) WdfVersion.Functions.pfnWdfUsbTargetPipeFormatRequestForWrite)(DriverGlobals, Pipe, Request, WriteMemory, WriteOffset);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETPIPEREADSYNCHRONOUSLY) WdfVersion.Functions.pfnWdfUsbTargetPipeReadSynchronously)(DriverGlobals, Pipe, Request, RequestOptions, MemoryDescriptor, BytesRead);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETPIPEFORMATREQUESTFORREAD) WdfVersion.Functions.pfnWdfUsbTargetPipeFormatRequestForRead)(DriverGlobals, Pipe, Request, ReadMemory, ReadOffset);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETPIPECONFIGCONTINUOUSREADER) WdfVersion.Functions.pfnWdfUsbTargetPipeConfigContinuousReader)(DriverGlobals, Pipe, Config);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETPIPEABORTSYNCHRONOUSLY) WdfVersion.Functions.pfnWdfUsbTargetPipeAbortSynchronously)(DriverGlobals, Pipe, Request, RequestOptions);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETPIPEFORMATREQUESTFORABORT) WdfVersion.Functions.pfnWdfUsbTargetPipeFormatRequestForAbort)(DriverGlobals, Pipe, Request);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETPIPERESETSYNCHRONOUSLY) WdfVersion.Functions.pfnWdfUsbTargetPipeResetSynchronously)(DriverGlobals, Pipe, Request, RequestOptions);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETPIPEFORMATREQUESTFORRESET) WdfVersion.Functions.pfnWdfUsbTargetPipeFormatRequestForReset)(DriverGlobals, Pipe, Request);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BYTE
VFWDFEXPORT(WdfUsbInterfaceGetInterfaceNumber)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBINTERFACEGETINTERFACENUMBER) WdfVersion.Functions.pfnWdfUsbInterfaceGetInterfaceNumber)(DriverGlobals, UsbInterface);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBINTERFACEGETNUMENDPOINTS) WdfVersion.Functions.pfnWdfUsbInterfaceGetNumEndpoints)(DriverGlobals, UsbInterface, SettingIndex);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFUSBINTERFACEGETDESCRIPTOR) WdfVersion.Functions.pfnWdfUsbInterfaceGetDescriptor)(DriverGlobals, UsbInterface, SettingIndex, InterfaceDescriptor);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BYTE
VFWDFEXPORT(WdfUsbInterfaceGetNumSettings)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBINTERFACEGETNUMSETTINGS) WdfVersion.Functions.pfnWdfUsbInterfaceGetNumSettings)(DriverGlobals, UsbInterface);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBINTERFACESELECTSETTING) WdfVersion.Functions.pfnWdfUsbInterfaceSelectSetting)(DriverGlobals, UsbInterface, PipesAttributes, Params);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFUSBINTERFACEGETENDPOINTINFORMATION) WdfVersion.Functions.pfnWdfUsbInterfaceGetEndpointInformation)(DriverGlobals, UsbInterface, SettingIndex, EndpointIndex, EndpointInfo);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICEGETINTERFACE) WdfVersion.Functions.pfnWdfUsbTargetDeviceGetInterface)(DriverGlobals, UsbDevice, InterfaceIndex);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BYTE
VFWDFEXPORT(WdfUsbInterfaceGetConfiguredSettingIndex)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE Interface
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBINTERFACEGETCONFIGUREDSETTINGINDEX) WdfVersion.Functions.pfnWdfUsbInterfaceGetConfiguredSettingIndex)(DriverGlobals, Interface);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BYTE
VFWDFEXPORT(WdfUsbInterfaceGetNumConfiguredPipes)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBINTERFACE UsbInterface
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBINTERFACEGETNUMCONFIGUREDPIPES) WdfVersion.Functions.pfnWdfUsbInterfaceGetNumConfiguredPipes)(DriverGlobals, UsbInterface);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBINTERFACEGETCONFIGUREDPIPE) WdfVersion.Functions.pfnWdfUsbInterfaceGetConfiguredPipe)(DriverGlobals, UsbInterface, PipeIndex, PipeInfo);
}

WDFAPI
VOID
VFWDFEXPORT(WdfVerifierDbgBreakPoint)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFVERIFIERDBGBREAKPOINT) WdfVersion.Functions.pfnWdfVerifierDbgBreakPoint)(DriverGlobals);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFVERIFIERKEBUGCHECK) WdfVersion.Functions.pfnWdfVerifierKeBugCheck)(DriverGlobals, BugCheckCode, BugCheckParameter1, BugCheckParameter2, BugCheckParameter3, BugCheckParameter4);
}

WDFAPI
PVOID
VFWDFEXPORT(WdfGetTriageInfo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFGETTRIAGEINFO) WdfVersion.Functions.pfnWdfGetTriageInfo)(DriverGlobals);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFWORKITEMCREATE) WdfVersion.Functions.pfnWdfWorkItemCreate)(DriverGlobals, Config, Attributes, WorkItem);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfWorkItemEnqueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWORKITEM WorkItem
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFWORKITEMENQUEUE) WdfVersion.Functions.pfnWdfWorkItemEnqueue)(DriverGlobals, WorkItem);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
VFWDFEXPORT(WdfWorkItemGetParentObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWORKITEM WorkItem
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFWORKITEMGETPARENTOBJECT) WdfVersion.Functions.pfnWdfWorkItemGetParentObject)(DriverGlobals, WorkItem);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfWorkItemFlush)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWORKITEM WorkItem
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFWORKITEMFLUSH) WdfVersion.Functions.pfnWdfWorkItemFlush)(DriverGlobals, WorkItem);
}


