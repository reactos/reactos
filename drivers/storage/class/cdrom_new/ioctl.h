/*++

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    ioctl.h

Abstract:

    Functions to handle IOCTLs.

Author:

Environment:

    kernel mode only 

Notes:


Revision History:

--*/

#ifndef __IOCTL_H__
#define __IOCTL_H__

BOOLEAN
RequestDispatchProcessDirectly(
    _In_ WDFDEVICE              Device, 
    _In_ WDFREQUEST             Request, 
    _In_ WDF_REQUEST_PARAMETERS RequestParameters
    );

BOOLEAN
RequestDispatchToSequentialQueue(
    _In_ WDFDEVICE              Device, 
    _In_ WDFREQUEST             Request, 
    _In_ WDF_REQUEST_PARAMETERS RequestParameters
    );

BOOLEAN
RequestDispatchSyncWithSequentialQueue(
    _In_ WDFDEVICE              Device, 
    _In_ WDFREQUEST             Request, 
    _In_ WDF_REQUEST_PARAMETERS RequestParameters
    );

BOOLEAN
RequestDispatchSpecialIoctls(
    _In_ WDFDEVICE              Device, 
    _In_ WDFREQUEST             Request, 
    _In_ WDF_REQUEST_PARAMETERS RequestParameters
    );

BOOLEAN
RequestDispatchUnknownRequests(
    _In_ WDFDEVICE              Device, 
    _In_ WDFREQUEST             Request, 
    _In_ WDF_REQUEST_PARAMETERS RequestParameters
    );

//
//  I/O Request Handlers
//

// Handlers that are called directly in dispatch routine.

NTSTATUS
RequestHandleGetInquiryData(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestHandleGetMediaTypeEx(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestHandleMountQueryUniqueId(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestHandleMountQueryDeviceName(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestHandleMountQuerySuggestedLinkName(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestHandleGetDeviceNumber(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestHandleGetHotPlugInfo(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestHandleSetHotPlugInfo(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleEventNotification(
    _In_      PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_opt_  WDFREQUEST               Request, 
    _In_opt_  PWDF_REQUEST_PARAMETERS  RequestParameters,
    _Out_     size_t *                 DataLength
    );

// Handlers that are called in RequestProcessSerializedIoctl in a work item.

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleGetDvdRegion(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestHandleQueryPropertyRetrieveCachedData(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );
                            
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleReadTOC(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleReadTocEx(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );
                            
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleGetConfiguration(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RequestHandleGetDriveGeometry(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleDiskVerify(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleCheckVerify(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleFakePartitionInfo(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RequestHandleEjectionControl(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RequestHandleLoadEjectMedia(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleMcnControl(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RequestHandleReserveRelease(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RequestHandlePersistentReserve(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

#if (NTDDI_VERSION >= NTDDI_WIN8)
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleAreVolumesReady(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleVolumeOnline(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceHandleRawRead(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandlePlayAudioMsf(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleReadQChannel(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandlePauseAudio(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleResumeAudio(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleSeekAudioMsf(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleStopAudio(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleGetSetVolume(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleReadDvdStructure(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleDvdEndSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleDvdStartSessionReadKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleDvdSendKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleSetReadAhead(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleSetSpeed(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsReadMediaKeyBlock(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsStartSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsEndSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsSendCertificate(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsGetCertificate(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsGetChallengeKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleSendChallengeKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleReadVolumeId(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsReadSerialNumber(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsReadMediaId(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsReadBindingNonce(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceHandleAacsGenerateBindingNonce(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleEnableStreaming(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleSendOpcInformation(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleGetPerformance(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleMcnSyncFakeIoctl(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _Out_ size_t *                 DataLength
    );

// Handlers that will be called by Sync process.

// RequestHandleUnknownIoctl could be called at DISPATCH_LEVEL.
NTSTATUS
RequestHandleUnknownIoctl(
    _In_ WDFDEVICE    Device,
    _In_ WDFREQUEST   Request
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleExclusiveAccessQueryLockState(
    _In_ WDFDEVICE    Device,
    _In_ WDFREQUEST   Request
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleExclusiveAccessLockDevice(
    _In_ WDFDEVICE    Device,
    _In_ WDFREQUEST   Request
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleExclusiveAccessUnlockDevice(
    _In_ WDFDEVICE    Device,
    _In_ WDFREQUEST   Request
    );

NTSTATUS
RequestHandleQueryPropertyDeviceUniqueId(
    _In_ WDFDEVICE    Device, 
    _In_ WDFREQUEST   Request
    );

NTSTATUS
RequestHandleQueryPropertyWriteCache(
    _In_ WDFDEVICE    Device, 
    _In_ WDFREQUEST   Request
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestHandleScsiPassThrough(
    _In_ WDFDEVICE    Device, 
    _In_ WDFREQUEST   Request
    );

// Read/write handler called possibly at DISPATCH_LEVEL.

NTSTATUS
RequestHandleReadWrite(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters
    );

//
//  I/O Request Validation helpers
//

NTSTATUS
RequestValidateRawRead(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateReadTocEx(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateReadToc(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateGetLastSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateReadQChannel(
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateDvdReadStructure(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateDvdStartSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateDvdSendKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateDvdReadKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateGetConfiguration(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateSetSpeed(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateAacsReadMediaKeyBlock(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateAacsStartSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateAacsSendCertificate(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateAacsGetCertificate(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateAacsGetChallengeKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateAacsSendChallengeKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateAacsReadVolumeId(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateAacsReadSerialNumber(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateAacsReadMediaId(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateAacsBindingNonce(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateExclusiveAccess(
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateEnableStreaming(
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateSendOpcInformation(
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateGetPerformance(
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

BOOLEAN
RequestIsRealtimeStreaming(
    _In_  WDFREQUEST               Request,
    _In_  BOOLEAN                  IsReadRequest
    );

NTSTATUS
RequestValidateReadWrite(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters
    );

NTSTATUS
RequestValidatePersistentReserve(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateDvdEndSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );

NTSTATUS
RequestValidateAacsEndSession(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension, 
    _In_  WDFREQUEST               Request, 
    _In_  WDF_REQUEST_PARAMETERS   RequestParameters,
    _Out_ size_t *                 DataLength
    );


//
// completion routines
//




#endif // __IOCTL_H__
