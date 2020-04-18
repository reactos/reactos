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

typedef struct _WDFFUNCTIONS {
    PFN_WDFCHILDLISTCREATE   pfnWdfChildListCreate;
	PFN_WDFCHILDLISTGETDEVICE   pfnWdfChildListGetDevice;
	PFN_WDFCHILDLISTRETRIEVEPDO   pfnWdfChildListRetrievePdo;
	PFN_WDFCHILDLISTRETRIEVEADDRESSDESCRIPTION   pfnWdfChildListRetrieveAddressDescription;
	PFN_WDFCHILDLISTBEGINSCAN   pfnWdfChildListBeginScan;
	PFN_WDFCHILDLISTENDSCAN   pfnWdfChildListEndScan;
	PFN_WDFCHILDLISTBEGINITERATION   pfnWdfChildListBeginIteration;
	PFN_WDFCHILDLISTRETRIEVENEXTDEVICE   pfnWdfChildListRetrieveNextDevice;
	PFN_WDFCHILDLISTENDITERATION   pfnWdfChildListEndIteration;
	PFN_WDFCHILDLISTADDORUPDATECHILDDESCRIPTIONASPRESENT   pfnWdfChildListAddOrUpdateChildDescriptionAsPresent;
	PFN_WDFCHILDLISTUPDATECHILDDESCRIPTIONASMISSING   pfnWdfChildListUpdateChildDescriptionAsMissing;
	PFN_WDFCHILDLISTUPDATEALLCHILDDESCRIPTIONSASPRESENT   pfnWdfChildListUpdateAllChildDescriptionsAsPresent;
	PFN_WDFCHILDLISTREQUESTCHILDEJECT   pfnWdfChildListRequestChildEject;
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
	PFN_WDFDEVICEINITSETPNPPOWEREVENTCALLBACKS   pfnWdfDeviceInitSetPnpPowerEventCallbacks;
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
	PFN_WDFDEVICEINITSETREQUESTATTRIBUTES   pfnWdfDeviceInitSetRequestAttributes;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitAssignWdmIrpPreprocessCallback;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceInitSetIoInCallerContextCallback;
	PFN_WDFDEVICECREATE   pfnWdfDeviceCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceSetStaticStopRemove;
	PFN_WDFDEVICECREATEDEVICEINTERFACE   pfnWdfDeviceCreateDeviceInterface;
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
	PFN_WDFDEVICEGETDEFAULTQUEUE   pfnWdfDeviceGetDefaultQueue;
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
	PFN_WDFDRIVERRETRIEVEVERSIONSTRING   pfnWdfDriverRetrieveVersionString;
	PFN_WDFDRIVERISVERSIONAVAILABLE   pfnWdfDriverIsVersionAvailable;
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
	PFN_WDFIOQUEUECREATE   pfnWdfIoQueueCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueGetState;
	PFN_WDFIOQUEUESTART   pfnWdfIoQueueStart;
	PFN_WDFUNIMPLEMENTED   pfnWdfIoQueueStop;
	PFN_WDFIOQUEUESTOPSYNCHRONOUSLY   pfnWdfIoQueueStopSynchronously;
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
	PFN_WDFMEMORYCOPYTOBUFFER   pfnWdfMemoryCopyToBuffer;
	PFN_WDFMEMORYCOPYFROMBUFFER   pfnWdfMemoryCopyFromBuffer;
	PFN_WDFUNIMPLEMENTED   pfnWdfLookasideListCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfMemoryCreateFromLookaside;
	PFN_WDFUNIMPLEMENTED   pfnWdfDeviceMiniportCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfDriverMiniportUnload;
	PFN_WDFOBJECTGETTYPEDCONTEXTWORKER   pfnWdfObjectGetTypedContextWorker;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectAllocateContext;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectContextGetObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectReferenceActual;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectDereferenceActual;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectCreate;
	PFN_WDFOBJECTDELETE   pfnWdfObjectDelete;
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
	PFN_WDFREQUESTMARKCANCELABLE   pfnWdfRequestMarkCancelable;
	PFN_WDFREQUESTUNMARKCANCELABLE   pfnWdfRequestUnmarkCancelable;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestIsCanceled;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestCancelSentRequest;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestIsFrom32BitProcess;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestSetCompletionRoutine;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestGetCompletionParams;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestAllocateTimer;
	PFN_WDFREQUESTCOMPLETE   pfnWdfRequestComplete;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestCompleteWithPriorityBoost;
	PFN_WDFREQUESTCOMPLETEWITHINFORMATION   pfnWdfRequestCompleteWithInformation;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestGetParameters;
	PFN_WDFREQUESTRETRIEVEINPUTMEMORY   pfnWdfRequestRetrieveInputMemory;
	PFN_WDFREQUESTRETRIEVEOUTPUTMEMORY   pfnWdfRequestRetrieveOutputMemory;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestRetrieveInputBuffer;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestRetrieveOutputBuffer;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestRetrieveInputWdmMdl;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestRetrieveOutputWdmMdl;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestRetrieveUnsafeUserInputBuffer;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestRetrieveUnsafeUserOutputBuffer;
	PFN_WDFREQUESTSETINFORMATION   pfnWdfRequestSetInformation;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestGetInformation;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestGetFileObject;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestProbeAndLockUserBufferForRead;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestProbeAndLockUserBufferForWrite;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestGetRequestorMode;
	PFN_WDFUNIMPLEMENTED   pfnWdfRequestForwardToIoQueue;
	PFN_WDFREQUESTGETIOQUEUE   pfnWdfRequestGetIoQueue;
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
	PFN_WDFSTRINGCREATE   pfnWdfStringCreate;
	PFN_WDFSTRINGGETUNICODESTRING   pfnWdfStringGetUnicodeString;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectAcquireLock;
	PFN_WDFUNIMPLEMENTED   pfnWdfObjectReleaseLock;
	PFN_WDFUNIMPLEMENTED   pfnWdfWaitLockCreate;
	PFN_WDFUNIMPLEMENTED   pfnWdfWaitLockAcquire;
	PFN_WDFUNIMPLEMENTED   pfnWdfWaitLockRelease;
	PFN_WDFSPINLOCKCREATE   pfnWdfSpinLockCreate;
	PFN_WDFSPINLOCKACQUIRE  pfnWdfSpinLockAcquire;
    PFN_WDFSPINLOCKRELEASE  pfnWdfSpinLockRelease;
	PFN_WDFTIMERCREATE   pfnWdfTimerCreate;
	PFN_WDFTIMERSTART   pfnWdfTimerStart;
	PFN_WDFTIMERSTOP   pfnWdfTimerStop;
	PFN_WDFTIMERGETPARENTOBJECT   pfnWdfTimerGetParentObject;
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
	PFN_WDFVERIFIERDBGBREAKPOINT   pfnWdfVerifierDbgBreakPoint;
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
	PFN_WDFREQUESTMARKCANCELABLEEX   pfnWdfRequestMarkCancelableEx;
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

// ----- WDFCHILDLIST ----- //

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfChildListCreate)(
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
WDFEXPORT(WdfChildListGetDevice)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    );

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
WDFEXPORT(WdfChildListRetrievePdo)(
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
WDFEXPORT(WdfChildListRetrieveAddressDescription)(
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
WDFEXPORT(WdfChildListBeginScan)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfChildListEndScan)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfChildListBeginIteration)(
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
WDFEXPORT(WdfChildListRetrieveNextDevice)(
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
WDFEXPORT(WdfChildListEndIteration)(
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
WDFEXPORT(WdfChildListAddOrUpdateChildDescriptionAsPresent)(
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
WDFEXPORT(WdfChildListUpdateChildDescriptionAsMissing)(
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
WDFEXPORT(WdfChildListUpdateAllChildDescriptionsAsPresent)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfChildListRequestChildEject)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFCHILDLIST ChildList,
    _In_
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    );

// ----- WDFCHILDLIST ----- //


// ----- WDFDRIVER ----- //
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

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDriverRetrieveVersionString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in
    WDFSTRING String
    );

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
BOOLEAN
WDFEXPORT(WdfDriverIsVersionAvailable)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in
    PWDF_DRIVER_VERSION_AVAILABLE_PARAMS VersionAvailableParams
    );

// ----- WDFDRIVER ----- //

// ----- WDFDEVICE -----//
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetPnpPowerEventCallbacks)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_PNPPOWER_EVENT_CALLBACKS PnpPowerEventCallbacks
    );

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __inout
    PWDFDEVICE_INIT* DeviceInit,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES DeviceAttributes,
    __out
    WDFDEVICE* Device
    );

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceCreateDeviceInterface)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    CONST GUID *InterfaceClassGUID,
    __in_opt
    PCUNICODE_STRING ReferenceString
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFQUEUE
WDFEXPORT(WdfDeviceGetDefaultQueue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetRequestAttributes)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_OBJECT_ATTRIBUTES RequestAttributes
    );

// ----- WDFDEVICE -----//

// ----- WDFIO -----//
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfIoQueueStart)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue
    );

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfIoQueueCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_IO_QUEUE_CONFIG Config,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES QueueAttributes,
    __out_opt
    WDFQUEUE* Queue
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfIoQueueStopSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue
    );

// ----- WDFIO -----//

// ----- WDFTIMER -----//
_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfTimerCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDF_TIMER_CONFIG Config,
    __in
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFTIMER * Timer
    );

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFEXPORT(WdfTimerStart)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFTIMER Timer,
    __in
    LONGLONG DueTime
    );

__drv_when(Wait == __true, __drv_maxIRQL(PASSIVE_LEVEL))
__drv_when(Wait == __false, __drv_maxIRQL(DISPATCH_LEVEL))
BOOLEAN
WDFEXPORT(WdfTimerStop)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFTIMER Timer,
    __in
    BOOLEAN  Wait
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
WDFEXPORT(WdfTimerGetParentObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFTIMER Timer
    );

// ----- WDFTIMER -----//

// ----- WDFREQUEST -----//
__drv_maxIRQL(DISPATCH_LEVEL)
WDFQUEUE
WDFAPI
WDFEXPORT(WdfRequestGetIoQueue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfRequestCompleteWithInformation)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    NTSTATUS RequestStatus,
    __in
    ULONG_PTR Information
    );

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfRequestRetrieveOutputMemory)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __out
    WDFMEMORY *Memory
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfRequestComplete)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    NTSTATUS RequestStatus
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfRequestSetInformation)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    ULONG_PTR Information
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfRequestMarkCancelable)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFREQUEST Request,
   __in
   PFN_WDF_REQUEST_CANCEL  EvtRequestCancel
   );

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfRequestRetrieveInputMemory)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __out
    WDFMEMORY *Memory
    );

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfRequestUnmarkCancelable)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFREQUEST Request
   );

// ----- WDFREQUEST -----//

// ----- WDFMEMORY -----//
_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfMemoryCopyToBuffer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFMEMORY SourceMemory,
    __in
    size_t SourceOffset,
    __out_bcount( NumBytesToCopyTo)
    PVOID Buffer,
    __in
    __drv_when(NumBytesToCopyTo == 0, __drv_reportError(NumBytesToCopyTo cannot be zero))
    size_t NumBytesToCopyTo
    );

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfMemoryCopyFromBuffer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFMEMORY DestinationMemory,
    __in
    size_t DestinationOffset,
    __in
    PVOID Buffer,
    __in
    __drv_when(NumBytesToCopyFrom == 0, __drv_reportError(NumBytesToCopyFrom cannot be zero))
    size_t NumBytesToCopyFrom
    );

// ----- WDFMEMORY -----//

// ----- WDFSTRING -----//
_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfStringCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PCUNICODE_STRING UnicodeString,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES StringAttributes,
    __out
    WDFSTRING* String
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfStringGetUnicodeString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFSTRING String,
    __out
    PUNICODE_STRING UnicodeString
    );

// ----- WDFSTRING -----//

// ----- WDFOBJECT -----//
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI    
VOID
WDFEXPORT(WdfObjectDelete)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFOBJECT Object
    );

_IRQL_requires_max_(DISPATCH_LEVEL+1)
WDFAPI
PVOID
FASTCALL
WDFEXPORT(WdfObjectGetTypedContextWorker)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFOBJECT Handle,
    _In_
    PCWDF_OBJECT_CONTEXT_TYPE_INFO TypeInfo
    );

// ----- WDFOBJECT -----//

// ----- WDFVERIFIER -----//
VOID
WDFEXPORT(WdfVerifierDbgBreakPoint)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals
    );

// ----- WDFVERIFIER -----//

// ----- WDFSYNC ----- //
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfSpinLockCreate)(
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
WDFEXPORT(WdfSpinLockAcquire)(
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
WDFEXPORT(WdfSpinLockRelease)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    _Requires_lock_held_(_Curr_)
    _Releases_lock_(_Curr_)
    _IRQL_restores_
    WDFSPINLOCK SpinLock
    );

// ----- WDFSYNC ----- //

// ----- WDFREQUEST ----- //
_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestMarkCancelableEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    PFN_WDF_REQUEST_CANCEL EvtRequestCancel
    );

// ----- WDFREQUEST ----- //

extern WDFVERSION WdfVersion;

WDFAPI
NTSTATUS
static
NotImplemented()
{
	DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "Not implemented. WDFFUNCTIONS Table: 0x%x\r\n", &WdfVersion);
    __debugbreak();
	return STATUS_UNSUCCESSFUL;
}

static WDFVERSION WdfVersion = {
		sizeof(WDFVERSION),
		sizeof(WDFFUNCTIONS)/sizeof(PVOID),
		{
			WDFEXPORT(WdfChildListCreate),
			WDFEXPORT(WdfChildListGetDevice),
			WDFEXPORT(WdfChildListRetrievePdo),
			WDFEXPORT(WdfChildListRetrieveAddressDescription),
			WDFEXPORT(WdfChildListBeginScan),
			WDFEXPORT(WdfChildListEndScan),
			WDFEXPORT(WdfChildListBeginIteration),
			WDFEXPORT(WdfChildListRetrieveNextDevice),
			WDFEXPORT(WdfChildListEndIteration),
			WDFEXPORT(WdfChildListAddOrUpdateChildDescriptionAsPresent),
			WDFEXPORT(WdfChildListUpdateChildDescriptionAsMissing),
			WDFEXPORT(WdfChildListUpdateAllChildDescriptionsAsPresent),
			WDFEXPORT(WdfChildListRequestChildEject),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfDeviceInitSetPnpPowerEventCallbacks),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfDeviceInitSetRequestAttributes),
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfDeviceCreate),
			NotImplemented,
			WDFEXPORT(WdfDeviceCreateDeviceInterface),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfDeviceGetDefaultQueue),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
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
			WDFEXPORT(WdfDriverRetrieveVersionString),
			WDFEXPORT(WdfDriverIsVersionAvailable),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfIoQueueCreate),
			NotImplemented,
			WDFEXPORT(WdfIoQueueStart),
			NotImplemented,
			WDFEXPORT(WdfIoQueueStopSynchronously),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfMemoryCopyToBuffer),
			WDFEXPORT(WdfMemoryCopyFromBuffer),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfObjectGetTypedContextWorker),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfObjectDelete),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfRequestMarkCancelable),
			WDFEXPORT(WdfRequestUnmarkCancelable),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfRequestComplete),
			NotImplemented,
			WDFEXPORT(WdfRequestCompleteWithInformation),
			NotImplemented,
			WDFEXPORT(WdfRequestRetrieveInputMemory),
			WDFEXPORT(WdfRequestRetrieveOutputMemory),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfRequestSetInformation),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfRequestGetIoQueue),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfStringCreate),
			WDFEXPORT(WdfStringGetUnicodeString),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfSpinLockCreate),
			WDFEXPORT(WdfSpinLockAcquire),
			WDFEXPORT(WdfSpinLockRelease),
			WDFEXPORT(WdfTimerCreate),
			WDFEXPORT(WdfTimerStart),
			WDFEXPORT(WdfTimerStop),
			WDFEXPORT(WdfTimerGetParentObject),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfVerifierDbgBreakPoint),
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			NotImplemented,
			WDFEXPORT(WdfRequestMarkCancelableEx),
			NotImplemented,
			NotImplemented
		}
	};

#ifdef __cplusplus
}
#endif

#endif //_FXDYNAMICS_H_
