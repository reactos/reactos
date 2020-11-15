/*++

Module Name: VfWdfDynamics.cpp

Abstract:
    Generated implementation for WDF API Verifier hooks

Environment:
    kernel mode only

--*/


extern "C" {
#include <ntddk.h>
}
#include "vfpriv.hpp"


extern "C" {
extern WDFVERSION WdfVersion;
}

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(FX_ENHANCED_VERIFIER_SECTION_NAME,  \
    VFWDFEXPORT(WdfChildListCreate), \
    VFWDFEXPORT(WdfChildListGetDevice), \
    VFWDFEXPORT(WdfChildListRetrievePdo), \
    VFWDFEXPORT(WdfChildListRetrieveAddressDescription), \
    VFWDFEXPORT(WdfChildListBeginScan), \
    VFWDFEXPORT(WdfChildListEndScan), \
    VFWDFEXPORT(WdfChildListBeginIteration), \
    VFWDFEXPORT(WdfChildListRetrieveNextDevice), \
    VFWDFEXPORT(WdfChildListEndIteration), \
    VFWDFEXPORT(WdfChildListAddOrUpdateChildDescriptionAsPresent), \
    VFWDFEXPORT(WdfChildListUpdateChildDescriptionAsMissing), \
    VFWDFEXPORT(WdfChildListUpdateAllChildDescriptionsAsPresent), \
    VFWDFEXPORT(WdfChildListRequestChildEject), \
    VFWDFEXPORT(WdfCollectionCreate), \
    VFWDFEXPORT(WdfCollectionGetCount), \
    VFWDFEXPORT(WdfCollectionAdd), \
    VFWDFEXPORT(WdfCollectionRemove), \
    VFWDFEXPORT(WdfCollectionRemoveItem), \
    VFWDFEXPORT(WdfCollectionGetItem), \
    VFWDFEXPORT(WdfCollectionGetFirstItem), \
    VFWDFEXPORT(WdfCollectionGetLastItem), \
    VFWDFEXPORT(WdfCommonBufferCreate), \
    VFWDFEXPORT(WdfCommonBufferCreateWithConfig), \
    VFWDFEXPORT(WdfCommonBufferGetAlignedVirtualAddress), \
    VFWDFEXPORT(WdfCommonBufferGetAlignedLogicalAddress), \
    VFWDFEXPORT(WdfCommonBufferGetLength), \
    VFWDFEXPORT(WdfControlDeviceInitAllocate), \
    VFWDFEXPORT(WdfControlDeviceInitSetShutdownNotification), \
    VFWDFEXPORT(WdfControlFinishInitializing), \
    VFWDFEXPORT(WdfCxDeviceInitAllocate), \
    VFWDFEXPORT(WdfCxDeviceInitAssignWdmIrpPreprocessCallback), \
    VFWDFEXPORT(WdfCxDeviceInitSetIoInCallerContextCallback), \
    VFWDFEXPORT(WdfCxDeviceInitSetRequestAttributes), \
    VFWDFEXPORT(WdfCxDeviceInitSetFileObjectConfig), \
    VFWDFEXPORT(WdfCxVerifierKeBugCheck), \
    VFWDFEXPORT(WdfDeviceGetDeviceState), \
    VFWDFEXPORT(WdfDeviceSetDeviceState), \
    VFWDFEXPORT(WdfWdmDeviceGetWdfDeviceHandle), \
    VFWDFEXPORT(WdfDeviceWdmGetDeviceObject), \
    VFWDFEXPORT(WdfDeviceWdmGetAttachedDevice), \
    VFWDFEXPORT(WdfDeviceWdmGetPhysicalDevice), \
    VFWDFEXPORT(WdfDeviceWdmDispatchPreprocessedIrp), \
    VFWDFEXPORT(WdfDeviceWdmDispatchIrp), \
    VFWDFEXPORT(WdfDeviceWdmDispatchIrpToIoQueue), \
    VFWDFEXPORT(WdfDeviceAddDependentUsageDeviceObject), \
    VFWDFEXPORT(WdfDeviceRemoveDependentUsageDeviceObject), \
    VFWDFEXPORT(WdfDeviceAddRemovalRelationsPhysicalDevice), \
    VFWDFEXPORT(WdfDeviceRemoveRemovalRelationsPhysicalDevice), \
    VFWDFEXPORT(WdfDeviceClearRemovalRelationsDevices), \
    VFWDFEXPORT(WdfDeviceGetDriver), \
    VFWDFEXPORT(WdfDeviceRetrieveDeviceName), \
    VFWDFEXPORT(WdfDeviceAssignMofResourceName), \
    VFWDFEXPORT(WdfDeviceGetIoTarget), \
    VFWDFEXPORT(WdfDeviceGetDevicePnpState), \
    VFWDFEXPORT(WdfDeviceGetDevicePowerState), \
    VFWDFEXPORT(WdfDeviceGetDevicePowerPolicyState), \
    VFWDFEXPORT(WdfDeviceAssignS0IdleSettings), \
    VFWDFEXPORT(WdfDeviceAssignSxWakeSettings), \
    VFWDFEXPORT(WdfDeviceOpenRegistryKey), \
    VFWDFEXPORT(WdfDeviceOpenDevicemapKey), \
    VFWDFEXPORT(WdfDeviceSetSpecialFileSupport), \
    VFWDFEXPORT(WdfDeviceSetCharacteristics), \
    VFWDFEXPORT(WdfDeviceGetCharacteristics), \
    VFWDFEXPORT(WdfDeviceGetAlignmentRequirement), \
    VFWDFEXPORT(WdfDeviceSetAlignmentRequirement), \
    VFWDFEXPORT(WdfDeviceInitFree), \
    VFWDFEXPORT(WdfDeviceInitSetPnpPowerEventCallbacks), \
    VFWDFEXPORT(WdfDeviceInitSetPowerPolicyEventCallbacks), \
    VFWDFEXPORT(WdfDeviceInitSetPowerPolicyOwnership), \
    VFWDFEXPORT(WdfDeviceInitRegisterPnpStateChangeCallback), \
    VFWDFEXPORT(WdfDeviceInitRegisterPowerStateChangeCallback), \
    VFWDFEXPORT(WdfDeviceInitRegisterPowerPolicyStateChangeCallback), \
    VFWDFEXPORT(WdfDeviceInitSetExclusive), \
    VFWDFEXPORT(WdfDeviceInitSetIoType), \
    VFWDFEXPORT(WdfDeviceInitSetPowerNotPageable), \
    VFWDFEXPORT(WdfDeviceInitSetPowerPageable), \
    VFWDFEXPORT(WdfDeviceInitSetPowerInrush), \
    VFWDFEXPORT(WdfDeviceInitSetDeviceType), \
    VFWDFEXPORT(WdfDeviceInitAssignName), \
    VFWDFEXPORT(WdfDeviceInitAssignSDDLString), \
    VFWDFEXPORT(WdfDeviceInitSetDeviceClass), \
    VFWDFEXPORT(WdfDeviceInitSetCharacteristics), \
    VFWDFEXPORT(WdfDeviceInitSetFileObjectConfig), \
    VFWDFEXPORT(WdfDeviceInitSetRequestAttributes), \
    VFWDFEXPORT(WdfDeviceInitAssignWdmIrpPreprocessCallback), \
    VFWDFEXPORT(WdfDeviceInitSetIoInCallerContextCallback), \
    VFWDFEXPORT(WdfDeviceInitSetRemoveLockOptions), \
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
    VFWDFEXPORT(WdfDeviceSetBusInformationForChildren), \
    VFWDFEXPORT(WdfDeviceIndicateWakeStatus), \
    VFWDFEXPORT(WdfDeviceSetFailed), \
    VFWDFEXPORT(WdfDeviceStopIdleNoTrack), \
    VFWDFEXPORT(WdfDeviceResumeIdleNoTrack), \
    VFWDFEXPORT(WdfDeviceStopIdleActual), \
    VFWDFEXPORT(WdfDeviceResumeIdleActual), \
    VFWDFEXPORT(WdfDeviceGetFileObject), \
    VFWDFEXPORT(WdfDeviceEnqueueRequest), \
    VFWDFEXPORT(WdfDeviceGetDefaultQueue), \
    VFWDFEXPORT(WdfDeviceConfigureRequestDispatching), \
    VFWDFEXPORT(WdfDeviceConfigureWdmIrpDispatchCallback), \
    VFWDFEXPORT(WdfDeviceGetSystemPowerAction), \
    VFWDFEXPORT(WdfDeviceWdmAssignPowerFrameworkSettings), \
    VFWDFEXPORT(WdfDeviceInitSetReleaseHardwareOrderOnFailure), \
    VFWDFEXPORT(WdfDeviceInitSetIoTypeEx), \
    VFWDFEXPORT(WdfDeviceQueryPropertyEx), \
    VFWDFEXPORT(WdfDeviceAllocAndQueryPropertyEx), \
    VFWDFEXPORT(WdfDeviceAssignProperty), \
    VFWDFEXPORT(WdfDeviceGetSelfIoTarget), \
    VFWDFEXPORT(WdfDeviceInitAllowSelfIoTarget), \
    VFWDFEXPORT(WdfDmaEnablerCreate), \
    VFWDFEXPORT(WdfDmaEnablerConfigureSystemProfile), \
    VFWDFEXPORT(WdfDmaEnablerGetMaximumLength), \
    VFWDFEXPORT(WdfDmaEnablerGetMaximumScatterGatherElements), \
    VFWDFEXPORT(WdfDmaEnablerSetMaximumScatterGatherElements), \
    VFWDFEXPORT(WdfDmaEnablerGetFragmentLength), \
    VFWDFEXPORT(WdfDmaEnablerWdmGetDmaAdapter), \
    VFWDFEXPORT(WdfDmaTransactionCreate), \
    VFWDFEXPORT(WdfDmaTransactionInitialize), \
    VFWDFEXPORT(WdfDmaTransactionInitializeUsingOffset), \
    VFWDFEXPORT(WdfDmaTransactionInitializeUsingRequest), \
    VFWDFEXPORT(WdfDmaTransactionExecute), \
    VFWDFEXPORT(WdfDmaTransactionRelease), \
    VFWDFEXPORT(WdfDmaTransactionDmaCompleted), \
    VFWDFEXPORT(WdfDmaTransactionDmaCompletedWithLength), \
    VFWDFEXPORT(WdfDmaTransactionDmaCompletedFinal), \
    VFWDFEXPORT(WdfDmaTransactionGetBytesTransferred), \
    VFWDFEXPORT(WdfDmaTransactionSetMaximumLength), \
    VFWDFEXPORT(WdfDmaTransactionGetRequest), \
    VFWDFEXPORT(WdfDmaTransactionGetCurrentDmaTransferLength), \
    VFWDFEXPORT(WdfDmaTransactionGetDevice), \
    VFWDFEXPORT(WdfDmaTransactionGetTransferInfo), \
    VFWDFEXPORT(WdfDmaTransactionSetChannelConfigurationCallback), \
    VFWDFEXPORT(WdfDmaTransactionSetTransferCompleteCallback), \
    VFWDFEXPORT(WdfDmaTransactionSetImmediateExecution), \
    VFWDFEXPORT(WdfDmaTransactionAllocateResources), \
    VFWDFEXPORT(WdfDmaTransactionSetDeviceAddressOffset), \
    VFWDFEXPORT(WdfDmaTransactionFreeResources), \
    VFWDFEXPORT(WdfDmaTransactionCancel), \
    VFWDFEXPORT(WdfDmaTransactionWdmGetTransferContext), \
    VFWDFEXPORT(WdfDmaTransactionStopSystemTransfer), \
    VFWDFEXPORT(WdfDpcCreate), \
    VFWDFEXPORT(WdfDpcEnqueue), \
    VFWDFEXPORT(WdfDpcCancel), \
    VFWDFEXPORT(WdfDpcGetParentObject), \
    VFWDFEXPORT(WdfDpcWdmGetDpc), \
    VFWDFEXPORT(WdfDriverCreate), \
    VFWDFEXPORT(WdfDriverGetRegistryPath), \
    VFWDFEXPORT(WdfDriverWdmGetDriverObject), \
    VFWDFEXPORT(WdfDriverOpenParametersRegistryKey), \
    VFWDFEXPORT(WdfWdmDriverGetWdfDriverHandle), \
    VFWDFEXPORT(WdfDriverRegisterTraceInfo), \
    VFWDFEXPORT(WdfDriverRetrieveVersionString), \
    VFWDFEXPORT(WdfDriverIsVersionAvailable), \
    VFWDFEXPORT(WdfFdoInitWdmGetPhysicalDevice), \
    VFWDFEXPORT(WdfFdoInitOpenRegistryKey), \
    VFWDFEXPORT(WdfFdoInitQueryProperty), \
    VFWDFEXPORT(WdfFdoInitAllocAndQueryProperty), \
    VFWDFEXPORT(WdfFdoInitQueryPropertyEx), \
    VFWDFEXPORT(WdfFdoInitAllocAndQueryPropertyEx), \
    VFWDFEXPORT(WdfFdoInitSetEventCallbacks), \
    VFWDFEXPORT(WdfFdoInitSetFilter), \
    VFWDFEXPORT(WdfFdoInitSetDefaultChildListConfig), \
    VFWDFEXPORT(WdfFdoQueryForInterface), \
    VFWDFEXPORT(WdfFdoGetDefaultChildList), \
    VFWDFEXPORT(WdfFdoAddStaticChild), \
    VFWDFEXPORT(WdfFdoLockStaticChildListForIteration), \
    VFWDFEXPORT(WdfFdoRetrieveNextStaticChild), \
    VFWDFEXPORT(WdfFdoUnlockStaticChildListFromIteration), \
    VFWDFEXPORT(WdfFileObjectGetFileName), \
    VFWDFEXPORT(WdfFileObjectGetFlags), \
    VFWDFEXPORT(WdfFileObjectGetDevice), \
    VFWDFEXPORT(WdfFileObjectWdmGetFileObject), \
    VFWDFEXPORT(WdfInterruptCreate), \
    VFWDFEXPORT(WdfInterruptQueueDpcForIsr), \
    VFWDFEXPORT(WdfInterruptQueueWorkItemForIsr), \
    VFWDFEXPORT(WdfInterruptSynchronize), \
    VFWDFEXPORT(WdfInterruptAcquireLock), \
    VFWDFEXPORT(WdfInterruptReleaseLock), \
    VFWDFEXPORT(WdfInterruptEnable), \
    VFWDFEXPORT(WdfInterruptDisable), \
    VFWDFEXPORT(WdfInterruptWdmGetInterrupt), \
    VFWDFEXPORT(WdfInterruptGetInfo), \
    VFWDFEXPORT(WdfInterruptSetPolicy), \
    VFWDFEXPORT(WdfInterruptSetExtendedPolicy), \
    VFWDFEXPORT(WdfInterruptGetDevice), \
    VFWDFEXPORT(WdfInterruptTryToAcquireLock), \
    VFWDFEXPORT(WdfInterruptReportActive), \
    VFWDFEXPORT(WdfInterruptReportInactive), \
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
    VFWDFEXPORT(WdfIoQueueAssignForwardProgressPolicy), \
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
    VFWDFEXPORT(WdfIoTargetQueryTargetProperty), \
    VFWDFEXPORT(WdfIoTargetAllocAndQueryTargetProperty), \
    VFWDFEXPORT(WdfIoTargetQueryForInterface), \
    VFWDFEXPORT(WdfIoTargetWdmGetTargetDeviceObject), \
    VFWDFEXPORT(WdfIoTargetWdmGetTargetPhysicalDevice), \
    VFWDFEXPORT(WdfIoTargetWdmGetTargetFileObject), \
    VFWDFEXPORT(WdfIoTargetWdmGetTargetFileHandle), \
    VFWDFEXPORT(WdfIoTargetSendReadSynchronously), \
    VFWDFEXPORT(WdfIoTargetFormatRequestForRead), \
    VFWDFEXPORT(WdfIoTargetSendWriteSynchronously), \
    VFWDFEXPORT(WdfIoTargetFormatRequestForWrite), \
    VFWDFEXPORT(WdfIoTargetSendIoctlSynchronously), \
    VFWDFEXPORT(WdfIoTargetFormatRequestForIoctl), \
    VFWDFEXPORT(WdfIoTargetSendInternalIoctlSynchronously), \
    VFWDFEXPORT(WdfIoTargetFormatRequestForInternalIoctl), \
    VFWDFEXPORT(WdfIoTargetSendInternalIoctlOthersSynchronously), \
    VFWDFEXPORT(WdfIoTargetFormatRequestForInternalIoctlOthers), \
    VFWDFEXPORT(WdfIoTargetSelfAssignDefaultIoQueue), \
    VFWDFEXPORT(WdfMemoryCreate), \
    VFWDFEXPORT(WdfMemoryCreatePreallocated), \
    VFWDFEXPORT(WdfMemoryGetBuffer), \
    VFWDFEXPORT(WdfMemoryAssignBuffer), \
    VFWDFEXPORT(WdfMemoryCopyToBuffer), \
    VFWDFEXPORT(WdfMemoryCopyFromBuffer), \
    VFWDFEXPORT(WdfLookasideListCreate), \
    VFWDFEXPORT(WdfMemoryCreateFromLookaside), \
    VFWDFEXPORT(WdfDeviceMiniportCreate), \
    VFWDFEXPORT(WdfDriverMiniportUnload), \
    VFWDFEXPORT(WdfObjectGetTypedContextWorker), \
    VFWDFEXPORT(WdfObjectAllocateContext), \
    VFWDFEXPORT(WdfObjectContextGetObject), \
    VFWDFEXPORT(WdfObjectReferenceActual), \
    VFWDFEXPORT(WdfObjectDereferenceActual), \
    VFWDFEXPORT(WdfObjectCreate), \
    VFWDFEXPORT(WdfObjectDelete), \
    VFWDFEXPORT(WdfObjectQuery), \
    VFWDFEXPORT(WdfPdoInitAllocate), \
    VFWDFEXPORT(WdfPdoInitSetEventCallbacks), \
    VFWDFEXPORT(WdfPdoInitAssignDeviceID), \
    VFWDFEXPORT(WdfPdoInitAssignInstanceID), \
    VFWDFEXPORT(WdfPdoInitAddHardwareID), \
    VFWDFEXPORT(WdfPdoInitAddCompatibleID), \
    VFWDFEXPORT(WdfPdoInitAssignContainerID), \
    VFWDFEXPORT(WdfPdoInitAddDeviceText), \
    VFWDFEXPORT(WdfPdoInitSetDefaultLocale), \
    VFWDFEXPORT(WdfPdoInitAssignRawDevice), \
    VFWDFEXPORT(WdfPdoInitAllowForwardingRequestToParent), \
    VFWDFEXPORT(WdfPdoMarkMissing), \
    VFWDFEXPORT(WdfPdoRequestEject), \
    VFWDFEXPORT(WdfPdoGetParent), \
    VFWDFEXPORT(WdfPdoRetrieveIdentificationDescription), \
    VFWDFEXPORT(WdfPdoRetrieveAddressDescription), \
    VFWDFEXPORT(WdfPdoUpdateAddressDescription), \
    VFWDFEXPORT(WdfPdoAddEjectionRelationsPhysicalDevice), \
    VFWDFEXPORT(WdfPdoRemoveEjectionRelationsPhysicalDevice), \
    VFWDFEXPORT(WdfPdoClearEjectionRelationsDevices), \
    VFWDFEXPORT(WdfDeviceAddQueryInterface), \
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
    VFWDFEXPORT(WdfRequestCreateFromIrp), \
    VFWDFEXPORT(WdfRequestReuse), \
    VFWDFEXPORT(WdfRequestChangeTarget), \
    VFWDFEXPORT(WdfRequestFormatRequestUsingCurrentType), \
    VFWDFEXPORT(WdfRequestWdmFormatUsingStackLocation), \
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
    VFWDFEXPORT(WdfRequestCompleteWithPriorityBoost), \
    VFWDFEXPORT(WdfRequestCompleteWithInformation), \
    VFWDFEXPORT(WdfRequestGetParameters), \
    VFWDFEXPORT(WdfRequestRetrieveInputMemory), \
    VFWDFEXPORT(WdfRequestRetrieveOutputMemory), \
    VFWDFEXPORT(WdfRequestRetrieveInputBuffer), \
    VFWDFEXPORT(WdfRequestRetrieveOutputBuffer), \
    VFWDFEXPORT(WdfRequestRetrieveInputWdmMdl), \
    VFWDFEXPORT(WdfRequestRetrieveOutputWdmMdl), \
    VFWDFEXPORT(WdfRequestRetrieveUnsafeUserInputBuffer), \
    VFWDFEXPORT(WdfRequestRetrieveUnsafeUserOutputBuffer), \
    VFWDFEXPORT(WdfRequestSetInformation), \
    VFWDFEXPORT(WdfRequestGetInformation), \
    VFWDFEXPORT(WdfRequestGetFileObject), \
    VFWDFEXPORT(WdfRequestProbeAndLockUserBufferForRead), \
    VFWDFEXPORT(WdfRequestProbeAndLockUserBufferForWrite), \
    VFWDFEXPORT(WdfRequestGetRequestorMode), \
    VFWDFEXPORT(WdfRequestForwardToIoQueue), \
    VFWDFEXPORT(WdfRequestGetIoQueue), \
    VFWDFEXPORT(WdfRequestRequeue), \
    VFWDFEXPORT(WdfRequestStopAcknowledge), \
    VFWDFEXPORT(WdfRequestWdmGetIrp), \
    VFWDFEXPORT(WdfRequestIsReserved), \
    VFWDFEXPORT(WdfRequestForwardToParentDeviceIoQueue), \
    VFWDFEXPORT(WdfIoResourceRequirementsListSetSlotNumber), \
    VFWDFEXPORT(WdfIoResourceRequirementsListSetInterfaceType), \
    VFWDFEXPORT(WdfIoResourceRequirementsListAppendIoResList), \
    VFWDFEXPORT(WdfIoResourceRequirementsListInsertIoResList), \
    VFWDFEXPORT(WdfIoResourceRequirementsListGetCount), \
    VFWDFEXPORT(WdfIoResourceRequirementsListGetIoResList), \
    VFWDFEXPORT(WdfIoResourceRequirementsListRemove), \
    VFWDFEXPORT(WdfIoResourceRequirementsListRemoveByIoResList), \
    VFWDFEXPORT(WdfIoResourceListCreate), \
    VFWDFEXPORT(WdfIoResourceListAppendDescriptor), \
    VFWDFEXPORT(WdfIoResourceListInsertDescriptor), \
    VFWDFEXPORT(WdfIoResourceListUpdateDescriptor), \
    VFWDFEXPORT(WdfIoResourceListGetCount), \
    VFWDFEXPORT(WdfIoResourceListGetDescriptor), \
    VFWDFEXPORT(WdfIoResourceListRemove), \
    VFWDFEXPORT(WdfIoResourceListRemoveByDescriptor), \
    VFWDFEXPORT(WdfCmResourceListAppendDescriptor), \
    VFWDFEXPORT(WdfCmResourceListInsertDescriptor), \
    VFWDFEXPORT(WdfCmResourceListGetCount), \
    VFWDFEXPORT(WdfCmResourceListGetDescriptor), \
    VFWDFEXPORT(WdfCmResourceListRemove), \
    VFWDFEXPORT(WdfCmResourceListRemoveByDescriptor), \
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
    VFWDFEXPORT(WdfUsbTargetDeviceWdmGetConfigurationHandle), \
    VFWDFEXPORT(WdfUsbTargetDeviceRetrieveCurrentFrameNumber), \
    VFWDFEXPORT(WdfUsbTargetDeviceSendControlTransferSynchronously), \
    VFWDFEXPORT(WdfUsbTargetDeviceFormatRequestForControlTransfer), \
    VFWDFEXPORT(WdfUsbTargetDeviceIsConnectedSynchronous), \
    VFWDFEXPORT(WdfUsbTargetDeviceResetPortSynchronously), \
    VFWDFEXPORT(WdfUsbTargetDeviceCyclePortSynchronously), \
    VFWDFEXPORT(WdfUsbTargetDeviceFormatRequestForCyclePort), \
    VFWDFEXPORT(WdfUsbTargetDeviceSendUrbSynchronously), \
    VFWDFEXPORT(WdfUsbTargetDeviceFormatRequestForUrb), \
    VFWDFEXPORT(WdfUsbTargetDeviceQueryUsbCapability), \
    VFWDFEXPORT(WdfUsbTargetDeviceCreateUrb), \
    VFWDFEXPORT(WdfUsbTargetDeviceCreateIsochUrb), \
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
    VFWDFEXPORT(WdfUsbTargetPipeSendUrbSynchronously), \
    VFWDFEXPORT(WdfUsbTargetPipeFormatRequestForUrb), \
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
    VFWDFEXPORT(WdfUsbTargetPipeWdmGetPipeHandle), \
    VFWDFEXPORT(WdfVerifierDbgBreakPoint), \
    VFWDFEXPORT(WdfVerifierKeBugCheck), \
    VFWDFEXPORT(WdfGetTriageInfo), \
    VFWDFEXPORT(WdfWmiProviderCreate), \
    VFWDFEXPORT(WdfWmiProviderGetDevice), \
    VFWDFEXPORT(WdfWmiProviderIsEnabled), \
    VFWDFEXPORT(WdfWmiProviderGetTracingHandle), \
    VFWDFEXPORT(WdfWmiInstanceCreate), \
    VFWDFEXPORT(WdfWmiInstanceRegister), \
    VFWDFEXPORT(WdfWmiInstanceDeregister), \
    VFWDFEXPORT(WdfWmiInstanceGetDevice), \
    VFWDFEXPORT(WdfWmiInstanceGetProvider), \
    VFWDFEXPORT(WdfWmiInstanceFireEvent), \
    VFWDFEXPORT(WdfWorkItemCreate), \
    VFWDFEXPORT(WdfWorkItemEnqueue), \
    VFWDFEXPORT(WdfWorkItemGetParentObject), \
    VFWDFEXPORT(WdfWorkItemFlush), \
    )
#endif   // #pragma alloc_text

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCHILDLISTCREATE) WdfVersion.Functions.pfnWdfChildListCreate)(DriverGlobals, Device, Config, ChildListAttributes, ChildList);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfChildListGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCHILDLISTGETDEVICE) WdfVersion.Functions.pfnWdfChildListGetDevice)(DriverGlobals, ChildList);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCHILDLISTRETRIEVEPDO) WdfVersion.Functions.pfnWdfChildListRetrievePdo)(DriverGlobals, ChildList, RetrieveInfo);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCHILDLISTRETRIEVEADDRESSDESCRIPTION) WdfVersion.Functions.pfnWdfChildListRetrieveAddressDescription)(DriverGlobals, ChildList, IdentificationDescription, AddressDescription);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfChildListBeginScan)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFCHILDLISTBEGINSCAN) WdfVersion.Functions.pfnWdfChildListBeginScan)(DriverGlobals, ChildList);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfChildListEndScan)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFCHILDLISTENDSCAN) WdfVersion.Functions.pfnWdfChildListEndScan)(DriverGlobals, ChildList);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFCHILDLISTBEGINITERATION) WdfVersion.Functions.pfnWdfChildListBeginIteration)(DriverGlobals, ChildList, Iterator);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCHILDLISTRETRIEVENEXTDEVICE) WdfVersion.Functions.pfnWdfChildListRetrieveNextDevice)(DriverGlobals, ChildList, Iterator, Device, Info);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFCHILDLISTENDITERATION) WdfVersion.Functions.pfnWdfChildListEndIteration)(DriverGlobals, ChildList, Iterator);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCHILDLISTADDORUPDATECHILDDESCRIPTIONASPRESENT) WdfVersion.Functions.pfnWdfChildListAddOrUpdateChildDescriptionAsPresent)(DriverGlobals, ChildList, IdentificationDescription, AddressDescription);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCHILDLISTUPDATECHILDDESCRIPTIONASMISSING) WdfVersion.Functions.pfnWdfChildListUpdateChildDescriptionAsMissing)(DriverGlobals, ChildList, IdentificationDescription);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfChildListUpdateAllChildDescriptionsAsPresent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFCHILDLISTUPDATEALLCHILDDESCRIPTIONSASPRESENT) WdfVersion.Functions.pfnWdfChildListUpdateAllChildDescriptionsAsPresent)(DriverGlobals, ChildList);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCHILDLISTREQUESTCHILDEJECT) WdfVersion.Functions.pfnWdfChildListRequestChildEject)(DriverGlobals, ChildList, IdentificationDescription);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCOMMONBUFFERCREATE) WdfVersion.Functions.pfnWdfCommonBufferCreate)(DriverGlobals, DmaEnabler, Length, Attributes, CommonBuffer);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCOMMONBUFFERCREATEWITHCONFIG) WdfVersion.Functions.pfnWdfCommonBufferCreateWithConfig)(DriverGlobals, DmaEnabler, Length, Config, Attributes, CommonBuffer);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PVOID
VFWDFEXPORT(WdfCommonBufferGetAlignedVirtualAddress)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOMMONBUFFER CommonBuffer
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCOMMONBUFFERGETALIGNEDVIRTUALADDRESS) WdfVersion.Functions.pfnWdfCommonBufferGetAlignedVirtualAddress)(DriverGlobals, CommonBuffer);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PHYSICAL_ADDRESS
VFWDFEXPORT(WdfCommonBufferGetAlignedLogicalAddress)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOMMONBUFFER CommonBuffer
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCOMMONBUFFERGETALIGNEDLOGICALADDRESS) WdfVersion.Functions.pfnWdfCommonBufferGetAlignedLogicalAddress)(DriverGlobals, CommonBuffer);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
VFWDFEXPORT(WdfCommonBufferGetLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCOMMONBUFFER CommonBuffer
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCOMMONBUFFERGETLENGTH) WdfVersion.Functions.pfnWdfCommonBufferGetLength)(DriverGlobals, CommonBuffer);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCONTROLDEVICEINITALLOCATE) WdfVersion.Functions.pfnWdfControlDeviceInitAllocate)(DriverGlobals, Driver, SDDLString);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFCONTROLDEVICEINITSETSHUTDOWNNOTIFICATION) WdfVersion.Functions.pfnWdfControlDeviceInitSetShutdownNotification)(DriverGlobals, DeviceInit, Notification, Flags);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfControlFinishInitializing)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFCONTROLFINISHINITIALIZING) WdfVersion.Functions.pfnWdfControlFinishInitializing)(DriverGlobals, Device);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCXDEVICEINITASSIGNWDMIRPPREPROCESSCALLBACK) WdfVersion.Functions.pfnWdfCxDeviceInitAssignWdmIrpPreprocessCallback)(DriverGlobals, CxDeviceInit, EvtCxDeviceWdmIrpPreprocess, MajorFunction, MinorFunctions, NumMinorFunctions);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFCXDEVICEINITSETIOINCALLERCONTEXTCALLBACK) WdfVersion.Functions.pfnWdfCxDeviceInitSetIoInCallerContextCallback)(DriverGlobals, CxDeviceInit, EvtIoInCallerContext);
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfWdmDeviceGetWdfDeviceHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PDEVICE_OBJECT DeviceObject
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFWDMDEVICEGETWDFDEVICEHANDLE) WdfVersion.Functions.pfnWdfWdmDeviceGetWdfDeviceHandle)(DriverGlobals, DeviceObject);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
VFWDFEXPORT(WdfDeviceWdmGetDeviceObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEWDMGETDEVICEOBJECT) WdfVersion.Functions.pfnWdfDeviceWdmGetDeviceObject)(DriverGlobals, Device);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
VFWDFEXPORT(WdfDeviceWdmGetAttachedDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEWDMGETATTACHEDDEVICE) WdfVersion.Functions.pfnWdfDeviceWdmGetAttachedDevice)(DriverGlobals, Device);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
VFWDFEXPORT(WdfDeviceWdmGetPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEWDMGETPHYSICALDEVICE) WdfVersion.Functions.pfnWdfDeviceWdmGetPhysicalDevice)(DriverGlobals, Device);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEWDMDISPATCHPREPROCESSEDIRP) WdfVersion.Functions.pfnWdfDeviceWdmDispatchPreprocessedIrp)(DriverGlobals, Device, Irp);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEADDDEPENDENTUSAGEDEVICEOBJECT) WdfVersion.Functions.pfnWdfDeviceAddDependentUsageDeviceObject)(DriverGlobals, Device, DependentDevice);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEREMOVEDEPENDENTUSAGEDEVICEOBJECT) WdfVersion.Functions.pfnWdfDeviceRemoveDependentUsageDeviceObject)(DriverGlobals, Device, DependentDevice);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEADDREMOVALRELATIONSPHYSICALDEVICE) WdfVersion.Functions.pfnWdfDeviceAddRemovalRelationsPhysicalDevice)(DriverGlobals, Device, PhysicalDevice);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEREMOVEREMOVALRELATIONSPHYSICALDEVICE) WdfVersion.Functions.pfnWdfDeviceRemoveRemovalRelationsPhysicalDevice)(DriverGlobals, Device, PhysicalDevice);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceClearRemovalRelationsDevices)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICECLEARREMOVALRELATIONSDEVICES) WdfVersion.Functions.pfnWdfDeviceClearRemovalRelationsDevices)(DriverGlobals, Device);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICERETRIEVEDEVICENAME) WdfVersion.Functions.pfnWdfDeviceRetrieveDeviceName)(DriverGlobals, Device, String);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEASSIGNMOFRESOURCENAME) WdfVersion.Functions.pfnWdfDeviceAssignMofResourceName)(DriverGlobals, Device, MofResourceName);
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDF_DEVICE_PNP_STATE
VFWDFEXPORT(WdfDeviceGetDevicePnpState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEGETDEVICEPNPSTATE) WdfVersion.Functions.pfnWdfDeviceGetDevicePnpState)(DriverGlobals, Device);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDF_DEVICE_POWER_STATE
VFWDFEXPORT(WdfDeviceGetDevicePowerState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEGETDEVICEPOWERSTATE) WdfVersion.Functions.pfnWdfDeviceGetDevicePowerState)(DriverGlobals, Device);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDF_DEVICE_POWER_POLICY_STATE
VFWDFEXPORT(WdfDeviceGetDevicePowerPolicyState)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEGETDEVICEPOWERPOLICYSTATE) WdfVersion.Functions.pfnWdfDeviceGetDevicePowerPolicyState)(DriverGlobals, Device);
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
VFWDFEXPORT(WdfDeviceSetSpecialFileSupport)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    WDF_SPECIAL_FILE_TYPE FileType,
    _In_
    BOOLEAN FileTypeIsSupported
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICESETSPECIALFILESUPPORT) WdfVersion.Functions.pfnWdfDeviceSetSpecialFileSupport)(DriverGlobals, Device, FileType, FileTypeIsSupported);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICESETCHARACTERISTICS) WdfVersion.Functions.pfnWdfDeviceSetCharacteristics)(DriverGlobals, Device, DeviceCharacteristics);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
VFWDFEXPORT(WdfDeviceGetCharacteristics)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEGETCHARACTERISTICS) WdfVersion.Functions.pfnWdfDeviceGetCharacteristics)(DriverGlobals, Device);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
VFWDFEXPORT(WdfDeviceGetAlignmentRequirement)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEGETALIGNMENTREQUIREMENT) WdfVersion.Functions.pfnWdfDeviceGetAlignmentRequirement)(DriverGlobals, Device);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICESETALIGNMENTREQUIREMENT) WdfVersion.Functions.pfnWdfDeviceSetAlignmentRequirement)(DriverGlobals, Device, AlignmentRequirement);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitFree)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITFREE) WdfVersion.Functions.pfnWdfDeviceInitFree)(DriverGlobals, DeviceInit);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEINITREGISTERPNPSTATECHANGECALLBACK) WdfVersion.Functions.pfnWdfDeviceInitRegisterPnpStateChangeCallback)(DriverGlobals, DeviceInit, PnpState, EvtDevicePnpStateChange, CallbackTypes);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEINITREGISTERPOWERSTATECHANGECALLBACK) WdfVersion.Functions.pfnWdfDeviceInitRegisterPowerStateChangeCallback)(DriverGlobals, DeviceInit, PowerState, EvtDevicePowerStateChange, CallbackTypes);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEINITREGISTERPOWERPOLICYSTATECHANGECALLBACK) WdfVersion.Functions.pfnWdfDeviceInitRegisterPowerPolicyStateChangeCallback)(DriverGlobals, DeviceInit, PowerPolicyState, EvtDevicePowerPolicyStateChange, CallbackTypes);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETEXCLUSIVE) WdfVersion.Functions.pfnWdfDeviceInitSetExclusive)(DriverGlobals, DeviceInit, IsExclusive);
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
VFWDFEXPORT(WdfDeviceInitSetPowerNotPageable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETPOWERNOTPAGEABLE) WdfVersion.Functions.pfnWdfDeviceInitSetPowerNotPageable)(DriverGlobals, DeviceInit);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetPowerPageable)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETPOWERPAGEABLE) WdfVersion.Functions.pfnWdfDeviceInitSetPowerPageable)(DriverGlobals, DeviceInit);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDeviceInitSetPowerInrush)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETPOWERINRUSH) WdfVersion.Functions.pfnWdfDeviceInitSetPowerInrush)(DriverGlobals, DeviceInit);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETDEVICETYPE) WdfVersion.Functions.pfnWdfDeviceInitSetDeviceType)(DriverGlobals, DeviceInit, DeviceType);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEINITASSIGNNAME) WdfVersion.Functions.pfnWdfDeviceInitAssignName)(DriverGlobals, DeviceInit, DeviceName);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEINITASSIGNSDDLSTRING) WdfVersion.Functions.pfnWdfDeviceInitAssignSDDLString)(DriverGlobals, DeviceInit, SDDLString);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETDEVICECLASS) WdfVersion.Functions.pfnWdfDeviceInitSetDeviceClass)(DriverGlobals, DeviceInit, DeviceClassGuid);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETCHARACTERISTICS) WdfVersion.Functions.pfnWdfDeviceInitSetCharacteristics)(DriverGlobals, DeviceInit, DeviceCharacteristics, OrInValues);
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
    )
{
    PAGED_CODE_LOCKED();
#pragma prefast(suppress: __WARNING_INVALID_PARAM_VALUE_3,"This is a verifier DDI hook routine. It just passes the caller parameters to original routine")
    return ((PFN_WDFDEVICEINITASSIGNWDMIRPPREPROCESSCALLBACK) WdfVersion.Functions.pfnWdfDeviceInitAssignWdmIrpPreprocessCallback)(DriverGlobals, DeviceInit, EvtDeviceWdmIrpPreprocess, MajorFunction, MinorFunctions, NumMinorFunctions);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETIOINCALLERCONTEXTCALLBACK) WdfVersion.Functions.pfnWdfDeviceInitSetIoInCallerContextCallback)(DriverGlobals, DeviceInit, EvtIoInCallerContext);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICEINITSETREMOVELOCKOPTIONS) WdfVersion.Functions.pfnWdfDeviceInitSetRemoveLockOptions)(DriverGlobals, DeviceInit, Options);
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
VFWDFEXPORT(WdfDeviceSetBusInformationForChildren)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PPNP_BUS_INFORMATION BusInformation
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDEVICESETBUSINFORMATIONFORCHILDREN) WdfVersion.Functions.pfnWdfDeviceSetBusInformationForChildren)(DriverGlobals, Device, BusInformation);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEINDICATEWAKESTATUS) WdfVersion.Functions.pfnWdfDeviceIndicateWakeStatus)(DriverGlobals, Device, WaitWakeStatus);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEENQUEUEREQUEST) WdfVersion.Functions.pfnWdfDeviceEnqueueRequest)(DriverGlobals, Device, Request);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEWDMASSIGNPOWERFRAMEWORKSETTINGS) WdfVersion.Functions.pfnWdfDeviceWdmAssignPowerFrameworkSettings)(DriverGlobals, Device, PowerFrameworkSettings);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMAENABLERCREATE) WdfVersion.Functions.pfnWdfDmaEnablerCreate)(DriverGlobals, Device, Config, Attributes, DmaEnablerHandle);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMAENABLERCONFIGURESYSTEMPROFILE) WdfVersion.Functions.pfnWdfDmaEnablerConfigureSystemProfile)(DriverGlobals, DmaEnabler, ProfileConfig, ConfigDirection);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
VFWDFEXPORT(WdfDmaEnablerGetMaximumLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMAENABLERGETMAXIMUMLENGTH) WdfVersion.Functions.pfnWdfDmaEnablerGetMaximumLength)(DriverGlobals, DmaEnabler);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
VFWDFEXPORT(WdfDmaEnablerGetMaximumScatterGatherElements)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMAENABLER DmaEnabler
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMAENABLERGETMAXIMUMSCATTERGATHERELEMENTS) WdfVersion.Functions.pfnWdfDmaEnablerGetMaximumScatterGatherElements)(DriverGlobals, DmaEnabler);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDMAENABLERSETMAXIMUMSCATTERGATHERELEMENTS) WdfVersion.Functions.pfnWdfDmaEnablerSetMaximumScatterGatherElements)(DriverGlobals, DmaEnabler, MaximumFragments);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMAENABLERGETFRAGMENTLENGTH) WdfVersion.Functions.pfnWdfDmaEnablerGetFragmentLength)(DriverGlobals, DmaEnabler, DmaDirection);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMAENABLERWDMGETDMAADAPTER) WdfVersion.Functions.pfnWdfDmaEnablerWdmGetDmaAdapter)(DriverGlobals, DmaEnabler, DmaDirection);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONCREATE) WdfVersion.Functions.pfnWdfDmaTransactionCreate)(DriverGlobals, DmaEnabler, Attributes, DmaTransaction);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONINITIALIZE) WdfVersion.Functions.pfnWdfDmaTransactionInitialize)(DriverGlobals, DmaTransaction, EvtProgramDmaFunction, DmaDirection, Mdl, VirtualAddress, Length);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONINITIALIZEUSINGOFFSET) WdfVersion.Functions.pfnWdfDmaTransactionInitializeUsingOffset)(DriverGlobals, DmaTransaction, EvtProgramDmaFunction, DmaDirection, Mdl, Offset, Length);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONINITIALIZEUSINGREQUEST) WdfVersion.Functions.pfnWdfDmaTransactionInitializeUsingRequest)(DriverGlobals, DmaTransaction, Request, EvtProgramDmaFunction, DmaDirection);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONEXECUTE) WdfVersion.Functions.pfnWdfDmaTransactionExecute)(DriverGlobals, DmaTransaction, Context);
}

_Success_(TRUE)
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfDmaTransactionRelease)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONRELEASE) WdfVersion.Functions.pfnWdfDmaTransactionRelease)(DriverGlobals, DmaTransaction);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONDMACOMPLETED) WdfVersion.Functions.pfnWdfDmaTransactionDmaCompleted)(DriverGlobals, DmaTransaction, Status);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONDMACOMPLETEDWITHLENGTH) WdfVersion.Functions.pfnWdfDmaTransactionDmaCompletedWithLength)(DriverGlobals, DmaTransaction, TransferredLength, Status);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONDMACOMPLETEDFINAL) WdfVersion.Functions.pfnWdfDmaTransactionDmaCompletedFinal)(DriverGlobals, DmaTransaction, FinalTransferredLength, Status);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
VFWDFEXPORT(WdfDmaTransactionGetBytesTransferred)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONGETBYTESTRANSFERRED) WdfVersion.Functions.pfnWdfDmaTransactionGetBytesTransferred)(DriverGlobals, DmaTransaction);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDMATRANSACTIONSETMAXIMUMLENGTH) WdfVersion.Functions.pfnWdfDmaTransactionSetMaximumLength)(DriverGlobals, DmaTransaction, MaximumLength);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFREQUEST
VFWDFEXPORT(WdfDmaTransactionGetRequest)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONGETREQUEST) WdfVersion.Functions.pfnWdfDmaTransactionGetRequest)(DriverGlobals, DmaTransaction);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
size_t
VFWDFEXPORT(WdfDmaTransactionGetCurrentDmaTransferLength)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONGETCURRENTDMATRANSFERLENGTH) WdfVersion.Functions.pfnWdfDmaTransactionGetCurrentDmaTransferLength)(DriverGlobals, DmaTransaction);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfDmaTransactionGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONGETDEVICE) WdfVersion.Functions.pfnWdfDmaTransactionGetDevice)(DriverGlobals, DmaTransaction);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDMATRANSACTIONGETTRANSFERINFO) WdfVersion.Functions.pfnWdfDmaTransactionGetTransferInfo)(DriverGlobals, DmaTransaction, MapRegisterCount, ScatterGatherElementCount);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDMATRANSACTIONSETCHANNELCONFIGURATIONCALLBACK) WdfVersion.Functions.pfnWdfDmaTransactionSetChannelConfigurationCallback)(DriverGlobals, DmaTransaction, ConfigureRoutine, ConfigureContext);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDMATRANSACTIONSETTRANSFERCOMPLETECALLBACK) WdfVersion.Functions.pfnWdfDmaTransactionSetTransferCompleteCallback)(DriverGlobals, DmaTransaction, DmaCompletionRoutine, DmaCompletionContext);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDMATRANSACTIONSETIMMEDIATEEXECUTION) WdfVersion.Functions.pfnWdfDmaTransactionSetImmediateExecution)(DriverGlobals, DmaTransaction, UseImmediateExecution);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONALLOCATERESOURCES) WdfVersion.Functions.pfnWdfDmaTransactionAllocateResources)(DriverGlobals, DmaTransaction, DmaDirection, RequiredMapRegisters, EvtReserveDmaFunction, EvtReserveDmaContext);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDMATRANSACTIONSETDEVICEADDRESSOFFSET) WdfVersion.Functions.pfnWdfDmaTransactionSetDeviceAddressOffset)(DriverGlobals, DmaTransaction, Offset);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDmaTransactionFreeResources)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDMATRANSACTIONFREERESOURCES) WdfVersion.Functions.pfnWdfDmaTransactionFreeResources)(DriverGlobals, DmaTransaction);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfDmaTransactionCancel)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONCANCEL) WdfVersion.Functions.pfnWdfDmaTransactionCancel)(DriverGlobals, DmaTransaction);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PVOID
VFWDFEXPORT(WdfDmaTransactionWdmGetTransferContext)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDMATRANSACTIONWDMGETTRANSFERCONTEXT) WdfVersion.Functions.pfnWdfDmaTransactionWdmGetTransferContext)(DriverGlobals, DmaTransaction);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfDmaTransactionStopSystemTransfer)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDMATRANSACTION DmaTransaction
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDMATRANSACTIONSTOPSYSTEMTRANSFER) WdfVersion.Functions.pfnWdfDmaTransactionStopSystemTransfer)(DriverGlobals, DmaTransaction);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDPCCREATE) WdfVersion.Functions.pfnWdfDpcCreate)(DriverGlobals, Config, Attributes, Dpc);
}

_IRQL_requires_max_(HIGH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfDpcEnqueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDPC Dpc
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDPCENQUEUE) WdfVersion.Functions.pfnWdfDpcEnqueue)(DriverGlobals, Dpc);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDPCCANCEL) WdfVersion.Functions.pfnWdfDpcCancel)(DriverGlobals, Dpc, Wait);
}

_IRQL_requires_max_(HIGH_LEVEL)
WDFAPI
WDFOBJECT
VFWDFEXPORT(WdfDpcGetParentObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDPC Dpc
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDPCGETPARENTOBJECT) WdfVersion.Functions.pfnWdfDpcGetParentObject)(DriverGlobals, Dpc);
}

_IRQL_requires_max_(HIGH_LEVEL)
WDFAPI
PKDPC
VFWDFEXPORT(WdfDpcWdmGetDpc)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDPC Dpc
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDPCWDMGETDPC) WdfVersion.Functions.pfnWdfDpcWdmGetDpc)(DriverGlobals, Dpc);
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDRIVER_OBJECT
VFWDFEXPORT(WdfDriverWdmGetDriverObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDRIVERWDMGETDRIVEROBJECT) WdfVersion.Functions.pfnWdfDriverWdmGetDriverObject)(DriverGlobals, Driver);
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDRIVER
VFWDFEXPORT(WdfWdmDriverGetWdfDriverHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PDRIVER_OBJECT DriverObject
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFWDMDRIVERGETWDFDRIVERHANDLE) WdfVersion.Functions.pfnWdfWdmDriverGetWdfDriverHandle)(DriverGlobals, DriverObject);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDRIVERREGISTERTRACEINFO) WdfVersion.Functions.pfnWdfDriverRegisterTraceInfo)(DriverGlobals, DriverObject, EvtTraceCallback, ControlBlock);
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
VFWDFEXPORT(WdfFdoInitWdmGetPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFDOINITWDMGETPHYSICALDEVICE) WdfVersion.Functions.pfnWdfFdoInitWdmGetPhysicalDevice)(DriverGlobals, DeviceInit);
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
VFWDFEXPORT(WdfFdoInitSetEventCallbacks)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_FDO_EVENT_CALLBACKS FdoEventCallbacks
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFFDOINITSETEVENTCALLBACKS) WdfVersion.Functions.pfnWdfFdoInitSetEventCallbacks)(DriverGlobals, DeviceInit, FdoEventCallbacks);
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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFFDOINITSETDEFAULTCHILDLISTCONFIG) WdfVersion.Functions.pfnWdfFdoInitSetDefaultChildListConfig)(DriverGlobals, DeviceInit, Config, DefaultChildListAttributes);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFDOQUERYFORINTERFACE) WdfVersion.Functions.pfnWdfFdoQueryForInterface)(DriverGlobals, Fdo, InterfaceType, Interface, Size, Version, InterfaceSpecificData);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFCHILDLIST
VFWDFEXPORT(WdfFdoGetDefaultChildList)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Fdo
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFDOGETDEFAULTCHILDLIST) WdfVersion.Functions.pfnWdfFdoGetDefaultChildList)(DriverGlobals, Fdo);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFDOADDSTATICCHILD) WdfVersion.Functions.pfnWdfFdoAddStaticChild)(DriverGlobals, Fdo, Child);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfFdoLockStaticChildListForIteration)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Fdo
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFFDOLOCKSTATICCHILDLISTFORITERATION) WdfVersion.Functions.pfnWdfFdoLockStaticChildListForIteration)(DriverGlobals, Fdo);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFDORETRIEVENEXTSTATICCHILD) WdfVersion.Functions.pfnWdfFdoRetrieveNextStaticChild)(DriverGlobals, Fdo, PreviousChild, Flags);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfFdoUnlockStaticChildListFromIteration)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Fdo
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFFDOUNLOCKSTATICCHILDLISTFROMITERATION) WdfVersion.Functions.pfnWdfFdoUnlockStaticChildListFromIteration)(DriverGlobals, Fdo);
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
ULONG
VFWDFEXPORT(WdfFileObjectGetFlags)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFILEOBJECTGETFLAGS) WdfVersion.Functions.pfnWdfFileObjectGetFlags)(DriverGlobals, FileObject);
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PFILE_OBJECT
VFWDFEXPORT(WdfFileObjectWdmGetFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFFILEOBJECT FileObject
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFFILEOBJECTWDMGETFILEOBJECT) WdfVersion.Functions.pfnWdfFileObjectWdmGetFileObject)(DriverGlobals, FileObject);
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

_Must_inspect_result_
WDFAPI
PKINTERRUPT
VFWDFEXPORT(WdfInterruptWdmGetInterrupt)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFINTERRUPTWDMGETINTERRUPT) WdfVersion.Functions.pfnWdfInterruptWdmGetInterrupt)(DriverGlobals, Interrupt);
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfInterruptReportActive)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFINTERRUPTREPORTACTIVE) WdfVersion.Functions.pfnWdfInterruptReportActive)(DriverGlobals, Interrupt);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfInterruptReportInactive)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFINTERRUPT Interrupt
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFINTERRUPTREPORTINACTIVE) WdfVersion.Functions.pfnWdfInterruptReportInactive)(DriverGlobals, Interrupt);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOQUEUEASSIGNFORWARDPROGRESSPOLICY) WdfVersion.Functions.pfnWdfIoQueueAssignForwardProgressPolicy)(DriverGlobals, Queue, ForwardProgressPolicy);
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
    )
{
    PAGED_CODE_LOCKED();
#pragma prefast(suppress: __WARNING_HIGH_PRIORITY_OVERFLOW_POSTCONDITION, "This is a verifier DDI hook routine and all it does is call original routine.")
    return ((PFN_WDFIOTARGETQUERYTARGETPROPERTY) WdfVersion.Functions.pfnWdfIoTargetQueryTargetProperty)(DriverGlobals, IoTarget, DeviceProperty, BufferLength, PropertyBuffer, ResultLength);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETALLOCANDQUERYTARGETPROPERTY) WdfVersion.Functions.pfnWdfIoTargetAllocAndQueryTargetProperty)(DriverGlobals, IoTarget, DeviceProperty, PoolType, PropertyMemoryAttributes, PropertyMemory);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETQUERYFORINTERFACE) WdfVersion.Functions.pfnWdfIoTargetQueryForInterface)(DriverGlobals, IoTarget, InterfaceType, Interface, Size, Version, InterfaceSpecificData);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
VFWDFEXPORT(WdfIoTargetWdmGetTargetDeviceObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETWDMGETTARGETDEVICEOBJECT) WdfVersion.Functions.pfnWdfIoTargetWdmGetTargetDeviceObject)(DriverGlobals, IoTarget);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PDEVICE_OBJECT
VFWDFEXPORT(WdfIoTargetWdmGetTargetPhysicalDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETWDMGETTARGETPHYSICALDEVICE) WdfVersion.Functions.pfnWdfIoTargetWdmGetTargetPhysicalDevice)(DriverGlobals, IoTarget);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PFILE_OBJECT
VFWDFEXPORT(WdfIoTargetWdmGetTargetFileObject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETWDMGETTARGETFILEOBJECT) WdfVersion.Functions.pfnWdfIoTargetWdmGetTargetFileObject)(DriverGlobals, IoTarget);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETSENDINTERNALIOCTLSYNCHRONOUSLY) WdfVersion.Functions.pfnWdfIoTargetSendInternalIoctlSynchronously)(DriverGlobals, IoTarget, Request, IoctlCode, InputBuffer, OutputBuffer, RequestOptions, BytesReturned);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETFORMATREQUESTFORINTERNALIOCTL) WdfVersion.Functions.pfnWdfIoTargetFormatRequestForInternalIoctl)(DriverGlobals, IoTarget, Request, IoctlCode, InputBuffer, InputBufferOffset, OutputBuffer, OutputBufferOffset);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETSENDINTERNALIOCTLOTHERSSYNCHRONOUSLY) WdfVersion.Functions.pfnWdfIoTargetSendInternalIoctlOthersSynchronously)(DriverGlobals, IoTarget, Request, IoctlCode, OtherArg1, OtherArg2, OtherArg4, RequestOptions, BytesReturned);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIOTARGETFORMATREQUESTFORINTERNALIOCTLOTHERS) WdfVersion.Functions.pfnWdfIoTargetFormatRequestForInternalIoctlOthers)(DriverGlobals, IoTarget, Request, IoctlCode, OtherArg1, OtherArg1Offset, OtherArg2, OtherArg2Offset, OtherArg4, OtherArg4Offset);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFLOOKASIDELISTCREATE) WdfVersion.Functions.pfnWdfLookasideListCreate)(DriverGlobals, LookasideAttributes, BufferSize, PoolType, MemoryAttributes, PoolTag, Lookaside);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFMEMORYCREATEFROMLOOKASIDE) WdfVersion.Functions.pfnWdfMemoryCreateFromLookaside)(DriverGlobals, Lookaside, Memory);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEMINIPORTCREATE) WdfVersion.Functions.pfnWdfDeviceMiniportCreate)(DriverGlobals, Driver, Attributes, DeviceObject, AttachedDeviceObject, Pdo, Device);
}

WDFAPI
VOID
VFWDFEXPORT(WdfDriverMiniportUnload)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDRIVER Driver
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFDRIVERMINIPORTUNLOAD) WdfVersion.Functions.pfnWdfDriverMiniportUnload)(DriverGlobals, Driver);
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
PWDFDEVICE_INIT
VFWDFEXPORT(WdfPdoInitAllocate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE ParentDevice
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFPDOINITALLOCATE) WdfVersion.Functions.pfnWdfPdoInitAllocate)(DriverGlobals, ParentDevice);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFPDOINITSETEVENTCALLBACKS) WdfVersion.Functions.pfnWdfPdoInitSetEventCallbacks)(DriverGlobals, DeviceInit, DispatchTable);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFPDOINITASSIGNDEVICEID) WdfVersion.Functions.pfnWdfPdoInitAssignDeviceID)(DriverGlobals, DeviceInit, DeviceID);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFPDOINITASSIGNINSTANCEID) WdfVersion.Functions.pfnWdfPdoInitAssignInstanceID)(DriverGlobals, DeviceInit, InstanceID);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFPDOINITADDHARDWAREID) WdfVersion.Functions.pfnWdfPdoInitAddHardwareID)(DriverGlobals, DeviceInit, HardwareID);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFPDOINITADDCOMPATIBLEID) WdfVersion.Functions.pfnWdfPdoInitAddCompatibleID)(DriverGlobals, DeviceInit, CompatibleID);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFPDOINITASSIGNCONTAINERID) WdfVersion.Functions.pfnWdfPdoInitAssignContainerID)(DriverGlobals, DeviceInit, ContainerID);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFPDOINITADDDEVICETEXT) WdfVersion.Functions.pfnWdfPdoInitAddDeviceText)(DriverGlobals, DeviceInit, DeviceDescription, DeviceLocation, LocaleId);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFPDOINITSETDEFAULTLOCALE) WdfVersion.Functions.pfnWdfPdoInitSetDefaultLocale)(DriverGlobals, DeviceInit, LocaleId);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFPDOINITASSIGNRAWDEVICE) WdfVersion.Functions.pfnWdfPdoInitAssignRawDevice)(DriverGlobals, DeviceInit, DeviceClassGuid);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfPdoInitAllowForwardingRequestToParent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFPDOINITALLOWFORWARDINGREQUESTTOPARENT) WdfVersion.Functions.pfnWdfPdoInitAllowForwardingRequestToParent)(DriverGlobals, DeviceInit);
}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfPdoMarkMissing)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFPDOMARKMISSING) WdfVersion.Functions.pfnWdfPdoMarkMissing)(DriverGlobals, Device);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfPdoRequestEject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFPDOREQUESTEJECT) WdfVersion.Functions.pfnWdfPdoRequestEject)(DriverGlobals, Device);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfPdoGetParent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFPDOGETPARENT) WdfVersion.Functions.pfnWdfPdoGetParent)(DriverGlobals, Device);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFPDORETRIEVEIDENTIFICATIONDESCRIPTION) WdfVersion.Functions.pfnWdfPdoRetrieveIdentificationDescription)(DriverGlobals, Device, IdentificationDescription);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFPDORETRIEVEADDRESSDESCRIPTION) WdfVersion.Functions.pfnWdfPdoRetrieveAddressDescription)(DriverGlobals, Device, AddressDescription);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFPDOUPDATEADDRESSDESCRIPTION) WdfVersion.Functions.pfnWdfPdoUpdateAddressDescription)(DriverGlobals, Device, AddressDescription);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFPDOADDEJECTIONRELATIONSPHYSICALDEVICE) WdfVersion.Functions.pfnWdfPdoAddEjectionRelationsPhysicalDevice)(DriverGlobals, Device, PhysicalDevice);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFPDOREMOVEEJECTIONRELATIONSPHYSICALDEVICE) WdfVersion.Functions.pfnWdfPdoRemoveEjectionRelationsPhysicalDevice)(DriverGlobals, Device, PhysicalDevice);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfPdoClearEjectionRelationsDevices)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFPDOCLEAREJECTIONRELATIONSDEVICES) WdfVersion.Functions.pfnWdfPdoClearEjectionRelationsDevices)(DriverGlobals, Device);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFDEVICEADDQUERYINTERFACE) WdfVersion.Functions.pfnWdfDeviceAddQueryInterface)(DriverGlobals, Device, InterfaceConfig);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTCREATEFROMIRP) WdfVersion.Functions.pfnWdfRequestCreateFromIrp)(DriverGlobals, RequestAttributes, Irp, RequestFreesIrp, Request);
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
WDFAPI
VOID
VFWDFEXPORT(WdfRequestWdmFormatUsingStackLocation)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    PIO_STACK_LOCATION Stack
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFREQUESTWDMFORMATUSINGSTACKLOCATION) WdfVersion.Functions.pfnWdfRequestWdmFormatUsingStackLocation)(DriverGlobals, Request, Stack);
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
VFWDFEXPORT(WdfRequestCompleteWithPriorityBoost)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    NTSTATUS Status,
    _In_
    CCHAR PriorityBoost
    )
{
    PAGED_CODE_LOCKED();
    PerfIoComplete(Request);
    ((PFN_WDFREQUESTCOMPLETEWITHPRIORITYBOOST) WdfVersion.Functions.pfnWdfRequestCompleteWithPriorityBoost)(DriverGlobals, Request, Status, PriorityBoost);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTRETRIEVEINPUTWDMMDL) WdfVersion.Functions.pfnWdfRequestRetrieveInputWdmMdl)(DriverGlobals, Request, Mdl);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTRETRIEVEOUTPUTWDMMDL) WdfVersion.Functions.pfnWdfRequestRetrieveOutputWdmMdl)(DriverGlobals, Request, Mdl);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTRETRIEVEUNSAFEUSERINPUTBUFFER) WdfVersion.Functions.pfnWdfRequestRetrieveUnsafeUserInputBuffer)(DriverGlobals, Request, MinimumRequiredLength, InputBuffer, Length);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTRETRIEVEUNSAFEUSEROUTPUTBUFFER) WdfVersion.Functions.pfnWdfRequestRetrieveUnsafeUserOutputBuffer)(DriverGlobals, Request, MinimumRequiredLength, OutputBuffer, Length);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTPROBEANDLOCKUSERBUFFERFORREAD) WdfVersion.Functions.pfnWdfRequestProbeAndLockUserBufferForRead)(DriverGlobals, Request, Buffer, Length, MemoryObject);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTPROBEANDLOCKUSERBUFFERFORWRITE) WdfVersion.Functions.pfnWdfRequestProbeAndLockUserBufferForWrite)(DriverGlobals, Request, Buffer, Length, MemoryObject);
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
PIRP
VFWDFEXPORT(WdfRequestWdmGetIrp)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTWDMGETIRP) WdfVersion.Functions.pfnWdfRequestWdmGetIrp)(DriverGlobals, Request);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
VFWDFEXPORT(WdfRequestIsReserved)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTISRESERVED) WdfVersion.Functions.pfnWdfRequestIsReserved)(DriverGlobals, Request);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFREQUESTFORWARDTOPARENTDEVICEIOQUEUE) WdfVersion.Functions.pfnWdfRequestForwardToParentDeviceIoQueue)(DriverGlobals, Request, ParentDeviceQueue, ForwardOptions);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIORESOURCEREQUIREMENTSLISTSETSLOTNUMBER) WdfVersion.Functions.pfnWdfIoResourceRequirementsListSetSlotNumber)(DriverGlobals, RequirementsList, SlotNumber);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIORESOURCEREQUIREMENTSLISTSETINTERFACETYPE) WdfVersion.Functions.pfnWdfIoResourceRequirementsListSetInterfaceType)(DriverGlobals, RequirementsList, InterfaceType);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIORESOURCEREQUIREMENTSLISTAPPENDIORESLIST) WdfVersion.Functions.pfnWdfIoResourceRequirementsListAppendIoResList)(DriverGlobals, RequirementsList, IoResList);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIORESOURCEREQUIREMENTSLISTINSERTIORESLIST) WdfVersion.Functions.pfnWdfIoResourceRequirementsListInsertIoResList)(DriverGlobals, RequirementsList, IoResList, Index);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
VFWDFEXPORT(WdfIoResourceRequirementsListGetCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESREQLIST RequirementsList
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIORESOURCEREQUIREMENTSLISTGETCOUNT) WdfVersion.Functions.pfnWdfIoResourceRequirementsListGetCount)(DriverGlobals, RequirementsList);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIORESOURCEREQUIREMENTSLISTGETIORESLIST) WdfVersion.Functions.pfnWdfIoResourceRequirementsListGetIoResList)(DriverGlobals, RequirementsList, Index);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIORESOURCEREQUIREMENTSLISTREMOVE) WdfVersion.Functions.pfnWdfIoResourceRequirementsListRemove)(DriverGlobals, RequirementsList, Index);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIORESOURCEREQUIREMENTSLISTREMOVEBYIORESLIST) WdfVersion.Functions.pfnWdfIoResourceRequirementsListRemoveByIoResList)(DriverGlobals, RequirementsList, IoResList);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIORESOURCELISTCREATE) WdfVersion.Functions.pfnWdfIoResourceListCreate)(DriverGlobals, RequirementsList, Attributes, ResourceList);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIORESOURCELISTAPPENDDESCRIPTOR) WdfVersion.Functions.pfnWdfIoResourceListAppendDescriptor)(DriverGlobals, ResourceList, Descriptor);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIORESOURCELISTINSERTDESCRIPTOR) WdfVersion.Functions.pfnWdfIoResourceListInsertDescriptor)(DriverGlobals, ResourceList, Descriptor, Index);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIORESOURCELISTUPDATEDESCRIPTOR) WdfVersion.Functions.pfnWdfIoResourceListUpdateDescriptor)(DriverGlobals, ResourceList, Descriptor, Index);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONG
VFWDFEXPORT(WdfIoResourceListGetCount)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIORESLIST ResourceList
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIORESOURCELISTGETCOUNT) WdfVersion.Functions.pfnWdfIoResourceListGetCount)(DriverGlobals, ResourceList);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFIORESOURCELISTGETDESCRIPTOR) WdfVersion.Functions.pfnWdfIoResourceListGetDescriptor)(DriverGlobals, ResourceList, Index);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIORESOURCELISTREMOVE) WdfVersion.Functions.pfnWdfIoResourceListRemove)(DriverGlobals, ResourceList, Index);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFIORESOURCELISTREMOVEBYDESCRIPTOR) WdfVersion.Functions.pfnWdfIoResourceListRemoveByDescriptor)(DriverGlobals, ResourceList, Descriptor);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCMRESOURCELISTAPPENDDESCRIPTOR) WdfVersion.Functions.pfnWdfCmResourceListAppendDescriptor)(DriverGlobals, List, Descriptor);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFCMRESOURCELISTINSERTDESCRIPTOR) WdfVersion.Functions.pfnWdfCmResourceListInsertDescriptor)(DriverGlobals, List, Descriptor, Index);
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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFCMRESOURCELISTREMOVE) WdfVersion.Functions.pfnWdfCmResourceListRemove)(DriverGlobals, List, Index);
}

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
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFCMRESOURCELISTREMOVEBYDESCRIPTOR) WdfVersion.Functions.pfnWdfCmResourceListRemoveByDescriptor)(DriverGlobals, List, Descriptor);
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
USBD_CONFIGURATION_HANDLE
VFWDFEXPORT(WdfUsbTargetDeviceWdmGetConfigurationHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICEWDMGETCONFIGURATIONHANDLE) WdfVersion.Functions.pfnWdfUsbTargetDeviceWdmGetConfigurationHandle)(DriverGlobals, UsbDevice);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICERETRIEVECURRENTFRAMENUMBER) WdfVersion.Functions.pfnWdfUsbTargetDeviceRetrieveCurrentFrameNumber)(DriverGlobals, UsbDevice, CurrentFrameNumber);
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
VFWDFEXPORT(WdfUsbTargetDeviceIsConnectedSynchronous)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICEISCONNECTEDSYNCHRONOUS) WdfVersion.Functions.pfnWdfUsbTargetDeviceIsConnectedSynchronous)(DriverGlobals, UsbDevice);
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
VFWDFEXPORT(WdfUsbTargetDeviceCyclePortSynchronously)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBDEVICE UsbDevice
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICECYCLEPORTSYNCHRONOUSLY) WdfVersion.Functions.pfnWdfUsbTargetDeviceCyclePortSynchronously)(DriverGlobals, UsbDevice);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICEFORMATREQUESTFORCYCLEPORT) WdfVersion.Functions.pfnWdfUsbTargetDeviceFormatRequestForCyclePort)(DriverGlobals, UsbDevice, Request);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICESENDURBSYNCHRONOUSLY) WdfVersion.Functions.pfnWdfUsbTargetDeviceSendUrbSynchronously)(DriverGlobals, UsbDevice, Request, RequestOptions, Urb);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICEFORMATREQUESTFORURB) WdfVersion.Functions.pfnWdfUsbTargetDeviceFormatRequestForUrb)(DriverGlobals, UsbDevice, Request, UrbMemory, UrbMemoryOffset);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICECREATEURB) WdfVersion.Functions.pfnWdfUsbTargetDeviceCreateUrb)(DriverGlobals, UsbDevice, Attributes, UrbMemory, Urb);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETDEVICECREATEISOCHURB) WdfVersion.Functions.pfnWdfUsbTargetDeviceCreateIsochUrb)(DriverGlobals, UsbDevice, Attributes, NumberOfIsochPackets, UrbMemory, Urb);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETPIPESENDURBSYNCHRONOUSLY) WdfVersion.Functions.pfnWdfUsbTargetPipeSendUrbSynchronously)(DriverGlobals, Pipe, Request, RequestOptions, Urb);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETPIPEFORMATREQUESTFORURB) WdfVersion.Functions.pfnWdfUsbTargetPipeFormatRequestForUrb)(DriverGlobals, PIPE, Request, UrbMemory, UrbMemoryOffset);
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

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
USBD_PIPE_HANDLE
VFWDFEXPORT(WdfUsbTargetPipeWdmGetPipeHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFUSBPIPE UsbPipe
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFUSBTARGETPIPEWDMGETPIPEHANDLE) WdfVersion.Functions.pfnWdfUsbTargetPipeWdmGetPipeHandle)(DriverGlobals, UsbPipe);
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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFWMIPROVIDERCREATE) WdfVersion.Functions.pfnWdfWmiProviderCreate)(DriverGlobals, Device, WmiProviderConfig, ProviderAttributes, WmiProvider);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfWmiProviderGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWMIPROVIDER WmiProvider
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFWMIPROVIDERGETDEVICE) WdfVersion.Functions.pfnWdfWmiProviderGetDevice)(DriverGlobals, WmiProvider);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFWMIPROVIDERISENABLED) WdfVersion.Functions.pfnWdfWmiProviderIsEnabled)(DriverGlobals, WmiProvider, ProviderControl);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
ULONGLONG
VFWDFEXPORT(WdfWmiProviderGetTracingHandle)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWMIPROVIDER WmiProvider
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFWMIPROVIDERGETTRACINGHANDLE) WdfVersion.Functions.pfnWdfWmiProviderGetTracingHandle)(DriverGlobals, WmiProvider);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFWMIINSTANCECREATE) WdfVersion.Functions.pfnWdfWmiInstanceCreate)(DriverGlobals, Device, InstanceConfig, InstanceAttributes, Instance);
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
VFWDFEXPORT(WdfWmiInstanceRegister)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWMIINSTANCE WmiInstance
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFWMIINSTANCEREGISTER) WdfVersion.Functions.pfnWdfWmiInstanceRegister)(DriverGlobals, WmiInstance);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
VFWDFEXPORT(WdfWmiInstanceDeregister)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWMIINSTANCE WmiInstance
    )
{
    PAGED_CODE_LOCKED();
    ((PFN_WDFWMIINSTANCEDEREGISTER) WdfVersion.Functions.pfnWdfWmiInstanceDeregister)(DriverGlobals, WmiInstance);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
VFWDFEXPORT(WdfWmiInstanceGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWMIINSTANCE WmiInstance
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFWMIINSTANCEGETDEVICE) WdfVersion.Functions.pfnWdfWmiInstanceGetDevice)(DriverGlobals, WmiInstance);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFWMIPROVIDER
VFWDFEXPORT(WdfWmiInstanceGetProvider)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFWMIINSTANCE WmiInstance
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFWMIINSTANCEGETPROVIDER) WdfVersion.Functions.pfnWdfWmiInstanceGetProvider)(DriverGlobals, WmiInstance);
}

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
    )
{
    PAGED_CODE_LOCKED();
    return ((PFN_WDFWMIINSTANCEFIREEVENT) WdfVersion.Functions.pfnWdfWmiInstanceFireEvent)(DriverGlobals, WmiInstance, EventDataSize, EventData);
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


