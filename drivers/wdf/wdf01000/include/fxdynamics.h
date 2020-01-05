#ifndef _FXDYNAMICS_H_
#define _FXDYNAMICS_H_

#include <ntddk.h>
#include "wdf.h"
#include "common/fxmacros.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef
WDFAPI
NTSTATUS
(*PFN_WDFUNIMPLEMENTED)();

WDFAPI
NTSTATUS
static
NotImplemented()
{
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "Not implemented");
    __debugbreak();
	return STATUS_UNSUCCESSFUL;
}

typedef struct _WDFFUNCTIONS {
    PFN_WDFUNIMPLEMENTED   pfnWdfChildListCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfChildListGetDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfChildListRetrievePdo;
	PFN_WDFUNIMPLEMENTED   pfnWdfChildListRetrieveAddressDescription;
	PFN_WDFUNIMPLEMENTED   pfnWdfChildListBeginScan;
	PFN_WDFUNIMPLEMENTED   pfnWdfChildListEndScan;
	PFN_WDFUNIMPLEMENTED   pfnWdfChildListBeginIteration;
	PFN_WDFUNIMPLEMENTED   pfnWdfChildListRetrieveNextDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfChildListEndIteration;
	PFN_WDFUNIMPLEMENTED   pfnWdfChildListAddOrUpdateChildDescriptionAsPresent;
	PFN_WDFUNIMPLEMENTED   pfnWdfChildListUpdateChildDescriptionAsMissing;
	PFN_WDFUNIMPLEMENTED   pfnWdfChildListUpdateAllChildDescriptionsAsPresent;
	PFN_WDFUNIMPLEMENTED   pfnWdfChildListRequestChildEject;
	PFN_WDFUNIMPLEMENTED   pfnWdfCollectionCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfCollectionGetCount;
	PFN_WDFUNIMPLEMENTED   pfnWdfCollectionAdd;
	PFN_WDFUNIMPLEMENTED   pfnWdfCollectionRemove;
	PFN_WDFUNIMPLEMENTED   pfnWdfCollectionRemoveItem;
	PFN_WDFUNIMPLEMENTED   pfnWdfCollectionGetItem;
	PFN_WDFUNIMPLEMENTED   pfnWdfCollectionGetFirstItem;
	PFN_WDFUNIMPLEMENTED   pfnWdfCollectionGetLastItem;
	PFN_WDFUNIMPLEMENTED   pfnWdfCommonBufferCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfCommonBufferGetAlignedVirtualAddress;
	PFN_WDFUNIMPLEMENTED   pfnWdfCommonBufferGetAlignedLogicalAddress;
	PFN_WDFUNIMPLEMENTED   pfnWdfCommonBufferGetLength;
	PFN_WDFUNIMPLEMENTED   pfnWdfControlDeviceInitAllocate;
	PFN_WDFUNIMPLEMENTED   pfnWdfControlDeviceInitSetShutdownNotification;
	PFN_WDFUNIMPLEMENTED   pfnWdfControlFinishInitializing;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceGetDeviceState;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceSetDeviceState;
	PFN_WDFUNIMPLEMENTED   pfnWdfWdmDeviceGetWdfDeviceHandle;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceWdmGetDeviceObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceWdmGetAttachedDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceWdmGetPhysicalDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceWdmDispatchPreprocessedIrp;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceAddDependentUsageDeviceObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceAddRemovalRelationsPhysicalDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceRemoveRemovalRelationsPhysicalDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceClearRemovalRelationsDevices;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceGetDriver;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceRetrieveDeviceName;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceAssignMofResourceName;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceGetIoTarget;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceGetDevicePnpState;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceGetDevicePowerState;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceGetDevicePowerPolicyState;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceAssignS0IdleSettings;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceAssignSxWakeSettings;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceOpenRegistryKey;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceSetSpecialFileSupport;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceSetCharacteristics;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceGetCharacteristics;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceGetAlignmentRequirement;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceSetAlignmentRequirement;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitFree;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitSetPnpPowerEventCallbacks;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitSetPowerPolicyEventCallbacks;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitSetPowerPolicyOwnership;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitRegisterPnpStateChangeCallback;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitRegisterPowerStateChangeCallback;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitRegisterPowerPolicyStateChangeCallback;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitSetIoType;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitSetExclusive;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitSetPowerNotPageable;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitSetPowerPageable;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitSetPowerInrush;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitSetDeviceType;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitAssignName;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitAssignSDDLString;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitSetDeviceClass;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitSetCharacteristics;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitSetFileObjectConfig;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitSetRequestAttributes;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitAssignWdmIrpPreprocessCallback;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitSetIoInCallerContextCallback;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceSetStaticStopRemove;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceCreateDeviceInterface;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceSetDeviceInterfaceState;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceRetrieveDeviceInterfaceString;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceCreateSymbolicLink;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceQueryProperty;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceAllocAndQueryProperty;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceSetPnpCapabilities;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceSetPowerCapabilities;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceSetBusInformationForChildren;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceIndicateWakeStatus;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceSetFailed;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceStopIdle;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceResumeIdle;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceGetFileObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceEnqueueRequest;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceGetDefaultQueue;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceConfigureRequestDispatching;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaEnablerCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaEnablerGetMaximumLength;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaEnablerGetMaximumScatterGatherElements;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaEnablerSetMaximumScatterGatherElements;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaTransactionCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaTransactionInitialize;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaTransactionInitializeUsingRequest;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaTransactionExecute;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaTransactionRelease;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaTransactionDmaCompleted;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaTransactionDmaCompletedWithLength;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaTransactionDmaCompletedFinal;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaTransactionGetBytesTransferred;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaTransactionSetMaximumLength;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaTransactionGetRequest;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaTransactionGetCurrentDmaTransferLength;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaTransactionGetDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfDpcCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfDpcEnqueue;
	PFN_WDFUNIMPLEMENTED   pfnWdfDpcCancel;
	PFN_WDFUNIMPLEMENTED   pfnWdfDpcGetParentObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfDpcWdmGetDpc;
	PFN_WDFDRIVERCREATE    pfnWdfDriverCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfDriverGetRegistryPath;
	PFN_WDFUNIMPLEMENTED   pfnWdfDriverWdmGetDriverObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfDriverOpenParametersRegistryKey;
	PFN_WDFUNIMPLEMENTED   pfnWdfWdmDriverGetWdfDriverHandle;
	PFN_WDFUNIMPLEMENTED   pfnWdfDriverRegisterTraceInfo;
	PFN_WDFUNIMPLEMENTED   pfnWdfDriverRetrieveVersionString;
	PFN_WDFUNIMPLEMENTED   pfnWdfDriverIsVersionAvailable;
	PFN_WDFUNIMPLEMENTED   pfnWdfFdoInitWdmGetPhysicalDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfFdoInitOpenRegistryKey;
	PFN_WDFUNIMPLEMENTED   pfnWdfFdoInitQueryProperty;
	PFN_WDFUNIMPLEMENTED   pfnWdfFdoInitAllocAndQueryProperty;
	PFN_WDFUNIMPLEMENTED   pfnWdfFdoInitSetEventCallbacks;
	PFN_WDFUNIMPLEMENTED   pfnWdfFdoInitSetFilter;
	PFN_WDFUNIMPLEMENTED   pfnWdfFdoInitSetDefaultChildListConfig;
	PFN_WDFUNIMPLEMENTED   pfnWdfFdoQueryForInterface;
	PFN_WDFUNIMPLEMENTED   pfnWdfFdoGetDefaultChildList;
	PFN_WDFUNIMPLEMENTED   pfnWdfFdoAddStaticChild;
	PFN_WDFUNIMPLEMENTED   pfnWdfFdoLockStaticChildListForIteration;
	PFN_WDFUNIMPLEMENTED   pfnWdfFdoRetrieveNextStaticChild;
	PFN_WDFUNIMPLEMENTED   pfnWdfFdoUnlockStaticChildListFromIteration;
	PFN_WDFUNIMPLEMENTED   pfnWdfFileObjectGetFileName;
	PFN_WDFUNIMPLEMENTED   pfnWdfFileObjectGetFlags;
	PFN_WDFUNIMPLEMENTED   pfnWdfFileObjectGetDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfFileObjectWdmGetFileObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfInterruptCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfInterruptQueueDpcForIsr;
	PFN_WDFUNIMPLEMENTED   pfnWdfInterruptSynchronize;
	PFN_WDFUNIMPLEMENTED   pfnWdfInterruptAcquireLock;
	PFN_WDFUNIMPLEMENTED   pfnWdfInterruptReleaseLock;
	PFN_WDFUNIMPLEMENTED   pfnWdfInterruptEnable;
	PFN_WDFUNIMPLEMENTED   pfnWdfInterruptDisable;
	PFN_WDFUNIMPLEMENTED   pfnWdfInterruptWdmGetInterrupt;
	PFN_WDFUNIMPLEMENTED   pfnWdfInterruptGetInfo;
	PFN_WDFUNIMPLEMENTED   pfnWdfInterruptSetPolicy;
	PFN_WDFUNIMPLEMENTED   pfnWdfInterruptGetDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueGetState;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueStart;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueStop;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueStopSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueGetDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueRetrieveNextRequest;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueRetrieveRequestByFileObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueFindRequest;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueRetrieveFoundRequest;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueDrainSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueDrain;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueuePurgeSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueuePurge;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueReadyNotify;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetOpen;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetCloseForQueryRemove;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetClose;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetStart;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetStop;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetGetState;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetGetDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetQueryTargetProperty;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetAllocAndQueryTargetProperty;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetQueryForInterface;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetWdmGetTargetDeviceObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetWdmGetTargetPhysicalDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetWdmGetTargetFileObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetWdmGetTargetFileHandle;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetSendReadSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetFormatRequestForRead;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetSendWriteSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetFormatRequestForWrite;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetSendIoctlSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetFormatRequestForIoctl;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetSendInternalIoctlSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetFormatRequestForInternalIoctl;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetSendInternalIoctlOthersSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoTargetFormatRequestForInternalIoctlOthers;
	PFN_WDFUNIMPLEMENTED   pfnWdfMemoryCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfMemoryCreatePreallocated;
	PFN_WDFUNIMPLEMENTED   pfnWdfMemoryGetBuffer;
	PFN_WDFUNIMPLEMENTED   pfnWdfMemoryAssignBuffer;
	PFN_WDFUNIMPLEMENTED   pfnWdfMemoryCopyToBuffer;
	PFN_WDFUNIMPLEMENTED   pfnWdfMemoryCopyFromBuffer;
	PFN_WDFUNIMPLEMENTED   pfnWdfLookasideListCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfMemoryCreateFromLookaside;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceMiniportCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfDriverMiniportUnload;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectGetTypedContextWorker;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectAllocateContext;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectContextGetObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectReferenceActual;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectDereferenceActual;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectDelete;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectQuery;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoInitAllocate;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoInitSetEventCallbacks;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoInitAssignDeviceID;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoInitAssignInstanceID;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoInitAddHardwareID;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoInitAddCompatibleID;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoInitAddDeviceText;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoInitSetDefaultLocale;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoInitAssignRawDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoMarkMissing;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoRequestEject;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoGetParent;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoRetrieveIdentificationDescription;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoRetrieveAddressDescription;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoUpdateAddressDescription;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoAddEjectionRelationsPhysicalDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoRemoveEjectionRelationsPhysicalDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoClearEjectionRelationsDevices;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceAddQueryInterface;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryOpenKey;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryCreateKey;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryClose;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryWdmGetHandle;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryRemoveKey;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryRemoveValue;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryQueryValue;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryQueryMemory;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryQueryMultiString;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryQueryUnicodeString;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryQueryString;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryQueryULong;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryAssignValue;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryAssignMemory;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryAssignMultiString;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryAssignUnicodeString;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryAssignString;
	PFN_WDFUNIMPLEMENTED   pfnWdfRegistryAssignULong;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestCreateFromIrp;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestReuse;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestChangeTarget;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestFormatRequestUsingCurrentType;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestWdmFormatUsingStackLocation;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestSend;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestGetStatus;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestMarkCancelable;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestUnmarkCancelable;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestIsCanceled;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestCancelSentRequest;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestIsFrom32BitProcess;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestSetCompletionRoutine;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestGetCompletionParams;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestAllocateTimer;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestComplete;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestCompleteWithPriorityBoost;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestCompleteWithInformation;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestGetParameters;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestRetrieveInputMemory;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestRetrieveOutputMemory;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestRetrieveInputBuffer;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestRetrieveOutputBuffer;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestRetrieveInputWdmMdl;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestRetrieveOutputWdmMdl;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestRetrieveUnsafeUserInputBuffer;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestRetrieveUnsafeUserOutputBuffer;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestSetInformation;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestGetInformation;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestGetFileObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestProbeAndLockUserBufferForRead;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestProbeAndLockUserBufferForWrite;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestGetRequestorMode;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestForwardToIoQueue;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestGetIoQueue;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestRequeue;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestStopAcknowledge;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestWdmGetIrp;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceRequirementsListSetSlotNumber;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceRequirementsListSetInterfaceType;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceRequirementsListAppendIoResList;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceRequirementsListInsertIoResList;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceRequirementsListGetCount;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceRequirementsListGetIoResList;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceRequirementsListRemove;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceRequirementsListRemoveByIoResList;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceListCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceListAppendDescriptor;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceListInsertDescriptor;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceListUpdateDescriptor;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceListGetCount;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceListGetDescriptor;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceListRemove;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoResourceListRemoveByDescriptor;
	PFN_WDFUNIMPLEMENTED   pfnWdfCmResourceListAppendDescriptor;
	PFN_WDFUNIMPLEMENTED   pfnWdfCmResourceListInsertDescriptor;
	PFN_WDFUNIMPLEMENTED   pfnWdfCmResourceListGetCount;
	PFN_WDFUNIMPLEMENTED   pfnWdfCmResourceListGetDescriptor;
	PFN_WDFUNIMPLEMENTED   pfnWdfCmResourceListRemove;
	PFN_WDFUNIMPLEMENTED   pfnWdfCmResourceListRemoveByDescriptor;
	PFN_WDFUNIMPLEMENTED   pfnWdfStringCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfStringGetUnicodeString;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectAcquireLock;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectReleaseLock;
	PFN_WDFUNIMPLEMENTED   pfnWdfWaitLockCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfWaitLockAcquire;
	PFN_WDFUNIMPLEMENTED   pfnWdfWaitLockRelease;
	PFN_WDFUNIMPLEMENTED   pfnWdfSpinLockCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfSpinLockAcquire;
	PFN_WDFUNIMPLEMENTED   pfnWdfSpinLockRelease;
	PFN_WDFUNIMPLEMENTED   pfnWdfTimerCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfTimerStart;
	PFN_WDFUNIMPLEMENTED   pfnWdfTimerStop;
	PFN_WDFUNIMPLEMENTED   pfnWdfTimerGetParentObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceRetrieveInformation;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceGetDeviceDescriptor;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceRetrieveConfigDescriptor;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceQueryString;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceAllocAndQueryString;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceFormatRequestForString;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceGetNumInterfaces;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceSelectConfig;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceWdmGetConfigurationHandle;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceRetrieveCurrentFrameNumber;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceSendControlTransferSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceFormatRequestForControlTransfer;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceIsConnectedSynchronous;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceResetPortSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceCyclePortSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceFormatRequestForCyclePort;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceSendUrbSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceFormatRequestForUrb;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeGetInformation;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeIsInEndpoint;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeIsOutEndpoint;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeGetType;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeSetNoMaximumPacketSizeCheck;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeWriteSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeFormatRequestForWrite;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeReadSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeFormatRequestForRead;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeConfigContinuousReader;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeAbortSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeFormatRequestForAbort;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeResetSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeFormatRequestForReset;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeSendUrbSynchronously;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeFormatRequestForUrb;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbInterfaceGetInterfaceNumber;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbInterfaceGetNumEndpoints;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbInterfaceGetDescriptor;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbInterfaceSelectSetting;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbInterfaceGetEndpointInformation;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetDeviceGetInterface;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbInterfaceGetConfiguredSettingIndex;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbInterfaceGetNumConfiguredPipes;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbInterfaceGetConfiguredPipe;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbTargetPipeWdmGetPipeHandle;
	PFN_WDFUNIMPLEMENTED   pfnWdfVerifierDbgBreakPoint;
	PFN_WDFUNIMPLEMENTED   pfnWdfVerifierKeBugCheck;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiProviderCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiProviderGetDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiProviderIsEnabled;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiProviderGetTracingHandle;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiInstanceCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiInstanceRegister;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiInstanceDeregister;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiInstanceGetDevice;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiInstanceGetProvider;
	PFN_WDFUNIMPLEMENTED   pfnWdfWmiInstanceFireEvent;
	PFN_WDFUNIMPLEMENTED   pfnWdfWorkItemCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfWorkItemEnqueue;
	PFN_WDFUNIMPLEMENTED   pfnWdfWorkItemGetParentObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfWorkItemFlush;
	PFN_WDFUNIMPLEMENTED   pfnWdfCommonBufferCreateWithConfig;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaEnablerGetFragmentLength;
	PFN_WDFUNIMPLEMENTED   pfnWdfDmaEnablerWdmGetDmaAdapter;
	PFN_WDFUNIMPLEMENTED   pfnWdfUsbInterfaceGetNumSettings;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceRemoveDependentUsageDeviceObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceGetSystemPowerAction;
	PFN_WDFUNIMPLEMENTED   pfnWdfInterruptSetExtendedPolicy;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueAssignForwardProgressPolicy;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoInitAssignContainerID;
	PFN_WDFUNIMPLEMENTED   pfnWdfPdoInitAllowForwardingRequestToParent;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestMarkCancelableEx;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestIsReserved;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestForwardToParentDeviceIoQueue;
} WDFFUNCTIONS, *PWDFFUNCTIONS;

typedef struct _WDFSTRUCTURES {
} WDFSTRUCTURES, *PWDFSTRUCTURES;

typedef struct _WDFVERSION {

    ULONG         Size;
    ULONG         FuncCount;
    WDFFUNCTIONS  Functions;
    ULONG         StructCount;
    WDFSTRUCTURES Structures;

} WDFVERSION, *PWDFVERSION;

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDriverCreate)(
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

extern WDFVERSION WdfVersion;

static WDFVERSION WdfVersion = {
		sizeof(WDFVERSION),
		sizeof(WDFFUNCTIONS)/sizeof(PVOID),
		{
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
            WDFEXPORT(WdfDriverCreate),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented
		}
	};

#ifdef __cplusplus
}
#endif

#endif //_FXDYNAMICS_H_
