/*++

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    mmc.h

Abstract:

    Functions for MMC area.

Author:

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#ifndef __MMC_H__
#define __MMC_H__

_IRQL_requires_max_(APC_LEVEL)
VOID
DeviceDeallocateMmcResources(
    _In_ WDFDEVICE Device
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceAllocateMmcResources(
    _In_ WDFDEVICE Device
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceUpdateMmcCapabilities(
    _In_ WDFDEVICE Device
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceGetConfigurationWithAlloc(
    _In_ WDFDEVICE                  Device,
    _Outptr_result_bytebuffer_all_(*BytesReturned) 
    PGET_CONFIGURATION_HEADER *     Buffer,
    _Out_ PULONG                    BytesReturned,
    FEATURE_NUMBER const            StartingFeature,
    ULONG const                     RequestedType
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceGetConfiguration(
    _In_  WDFDEVICE                 Device,
    _Out_writes_bytes_to_(BufferSize, *ValidBytes)
          PGET_CONFIGURATION_HEADER Buffer,
    _In_  ULONG const               BufferSize,
    _Out_ PULONG                    ValidBytes,
    _In_  FEATURE_NUMBER const      StartingFeature,
    _In_  ULONG const               RequestedType
    );

_IRQL_requires_max_(APC_LEVEL)
VOID
DeviceUpdateMmcWriteCapability(
    _In_reads_bytes_(BufferSize) 
        PGET_CONFIGURATION_HEADER   Buffer,
    ULONG const                     BufferSize,
    BOOLEAN const                   CurrentOnly, // TRUE == can drive write now, FALSE == can drive ever write
    _Out_ PBOOLEAN                  Writable,
    _Out_ PFEATURE_NUMBER           ValidationSchema,
    _Out_ PULONG                    BlockingFactor
    );

_IRQL_requires_max_(APC_LEVEL)
PVOID
MmcDataFindFeaturePage(
    _In_reads_bytes_(Length) 
       PGET_CONFIGURATION_HEADER    FeatureBuffer,
    ULONG const                     Length,
    FEATURE_NUMBER const            Feature
    );

_IRQL_requires_max_(APC_LEVEL)
VOID
MmcDataFindProfileInProfiles(
     _In_   FEATURE_DATA_PROFILE_LIST const* ProfileHeader,
     _In_   FEATURE_PROFILE_TYPE const       ProfileToFind,
     _In_   BOOLEAN const                    CurrentOnly,
     _Out_  PBOOLEAN                         Found
     );

_IRQL_requires_max_(APC_LEVEL)
_Ret_range_(-1,MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS)
LONGLONG
DeviceRetryTimeGuessBasedOnProfile(
    FEATURE_PROFILE_TYPE const Profile
    );

_IRQL_requires_max_(APC_LEVEL)
_Ret_range_(-1,MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS)
LONGLONG
DeviceRetryTimeDetectionBasedOnModePage2A(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(APC_LEVEL)
_Ret_range_(-1,MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS)
LONGLONG
DeviceRetryTimeDetectionBasedOnGetPerformance(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ BOOLEAN                 UseLegacyNominalPerformance
    );

#endif // __MMC_H__
