/*++

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    zpodd.c

Abstract:

    Code for Zero Power ODD support.

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "ntddk.h"
#include "ntddstor.h"
#include "wdmguid.h"
#include "cdrom.h"
#include "mmc.h"
#include "ioctl.h"
#include "scratch.h"

#ifdef DEBUG_USE_WPP
#include "zpodd.tmh"
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
DeviceGetZPODDEnabledFromRegistry();

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceQueryD3ColdInterface(
    _In_    PCDROM_DEVICE_EXTENSION     DeviceExtension,
    _Out_   PD3COLD_SUPPORT_INTERFACE   D3ColdInterface
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceSendEnableIdlePowerIoctl(
    _In_    PCDROM_DEVICE_EXTENSION     DeviceExtension,
    _In_    BOOLEAN                     WakeCapable,
    _In_    BOOLEAN                     Enable,
    _In_    ULONG                       D3IdleTimeout
    );

#if ALLOC_PRAGMA

#pragma alloc_text(PAGE, DeviceInitializeZPODD)
#pragma alloc_text(PAGE, DeviceGetZPODDEnabledFromRegistry)
#pragma alloc_text(PAGE, DeviceQueryD3ColdInterface)
#pragma alloc_text(PAGE, DeviceSendEnableIdlePowerIoctl)
#pragma alloc_text(PAGE, DeviceReleaseZPODDResources)
#pragma alloc_text(PAGE, DeviceZPODDIsInHomePosition)
#pragma alloc_text(PAGE, DeviceMarkActive)

#endif

#pragma warning(push)
#pragma warning(disable:4152) // nonstandard extension, function/data pointer conversion in expression
#pragma warning(disable:26000) // read overflow reported because of pointer type conversion

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceInitializeZPODD(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension
    )
/*++

Routine Description:

    This routine initialize the contents of ZPODD structure.

Arguments:

    DeviceExtension - the device extension

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS                            status = STATUS_SUCCESS;
    NTSTATUS                            tempStatus = STATUS_SUCCESS;
    PZERO_POWER_ODD_INFO                zpoddInfo = NULL;
    PFEATURE_DATA_REMOVABLE_MEDIUM      removableMediumHeader = NULL;
    ULONG                               ZPODDEnabledInRegistry = 0;
    PD3COLD_SUPPORT_INTERFACE           d3ColdInterface = NULL;
    DEVICE_WAKE_DEPTH                   deepestWakeableDstate = DeviceWakeDepthNotWakeable;
    BOOLEAN                             inHomePosition = FALSE;

    PAGED_CODE();

    if (DeviceExtension->ZeroPowerODDInfo != NULL)
    {
        //
        // Already initialized.
        //

        goto Cleanup;
    }

    ZPODDEnabledInRegistry = DeviceGetZPODDEnabledFromRegistry();

    if (ZPODDEnabledInRegistry == 0)
    {
        //
        // User has explicitly disabled Zero Power ODD.
        //

        status = STATUS_NOT_SUPPORTED;

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER,
                    "DeviceInitializeZPODD: ZPODD not enabled due to registry settings.\n"
                    ));

        goto Cleanup;
    }

    zpoddInfo = ExAllocatePoolWithTag(NonPagedPoolNx,
                                      sizeof(ZERO_POWER_ODD_INFO),
                                      CDROM_TAG_ZERO_POWER_ODD);

    if (zpoddInfo == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto Cleanup;
    }

    RtlZeroMemory(zpoddInfo, sizeof (ZERO_POWER_ODD_INFO));

    //
    // Check the system for the following prerequisites:
    //
    // 1. SATA: Device Attention line
    // 2. SATA: Asynchronous Notification
    // 3. ODD:  LoChange / MediaRemoval
    // 4. ACPI: Wake capable
    //
    // Only drawer and slot loading types have well defined behaviors in the spec, so only these two
    // types are supported.
    //

    //
    // Check for DA & AN
    //

    if ((DeviceExtension->PowerDescriptor == NULL) ||
        (DeviceExtension->PowerDescriptor->DeviceAttentionSupported == FALSE) ||
        (DeviceExtension->PowerDescriptor->AsynchronousNotificationSupported == FALSE))
    {
        status = STATUS_NOT_SUPPORTED;

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER,
                    "DeviceInitializeZPODD: ZPODD not enabled due to SATA features not present.\n"
                    ));

        goto Cleanup;
    }

    //
    // Check for LoChange / MediaRemoval
    //

    removableMediumHeader = (PFEATURE_DATA_REMOVABLE_MEDIUM)
                            DeviceFindFeaturePage(DeviceExtension->DeviceAdditionalData.Mmc.CapabilitiesBuffer,
                                                  DeviceExtension->DeviceAdditionalData.Mmc.CapabilitiesBufferSize,
                                                  FeatureRemovableMedium);

    if ((removableMediumHeader == NULL) ||
        (!((removableMediumHeader->LoadingMechanism == LOADING_MECHANISM_TRAY) && (removableMediumHeader->Load == 0) &&     // Drawer ...
         (removableMediumHeader->DBML != FALSE)) &&                                                                         // requires LoChange/NotBusy
        !((removableMediumHeader->LoadingMechanism == LOADING_MECHANISM_CADDY) && (removableMediumHeader->Load == 0) &&     // Slot ...
         (DeviceExtension->MediaChangeDetectionInfo->Gesn.Supported != FALSE))))                                            // requires MediaRemoval
    {
        status = STATUS_NOT_SUPPORTED;

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER,
                    "DeviceInitializeZPODD: ZPODD not enabled due to ODD features not present.\n"
                    ));

        goto Cleanup;
    }

    zpoddInfo->LoadingMechanism = removableMediumHeader->LoadingMechanism;
    zpoddInfo->Load = removableMediumHeader->Load;

    //
    // Check for ACPI
    //

    status = DeviceQueryD3ColdInterface(DeviceExtension, &zpoddInfo->D3ColdInterface);

    if (!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER,
                    "DeviceInitializeZPODD: Query D3Cold support interface failed.\n"
                    ));

        goto Cleanup;
    }

    //
    // If the platform supports Zero Power ODD, the following conditions must be met:
    //
    // 1. The deepest wakeable D-state for the device is D3Cold;
    // 2. The platform supports D3Cold for the device.
    //

    d3ColdInterface = &zpoddInfo->D3ColdInterface;

    status = d3ColdInterface->GetIdleWakeInfo(d3ColdInterface->Context,
                                              PowerSystemWorking,
                                              &deepestWakeableDstate);

    if (!NT_SUCCESS(status))
    {
        goto Cleanup;
    }

    //
    // DeviceExtension->PowerDescriptor->D3ColdSupported is retrieved from lower layer.
    // It has more accurate supportive information than just uses d3ColdInterface->GetD3ColdCapability
    //
    if ((deepestWakeableDstate != DeviceWakeDepthD3cold) ||
        (DeviceExtension->PowerDescriptor->D3ColdSupported == FALSE))
    {
        status = STATUS_NOT_SUPPORTED;

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER,
                    "DeviceInitializeZPODD: ZPODD not enabled due to ACPI support not present.\n"
                    ));

        goto Cleanup;
    }

    //
    // The system meets all requirements. Go ahead and enable ZPODD.
    //

    //
    // Register with the runtime power framework.
    // Note that no un-registration is needed during tear-down.
    // D3Cold will be enabled (success case of following call) or disabled by port driver during processing Enable Idle Power IOCTL.
    //

    status = DeviceSendEnableIdlePowerIoctl(DeviceExtension, TRUE, TRUE, DELAY_TIME_TO_ENTER_ZERO_POWER_IN_MS);

    if (!NT_SUCCESS(status))
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER,
                    "DeviceInitializeZPODD: ZPODD not enabled due to runtime power framework.\n"
                    ));

        goto Cleanup;
    }

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER,
                "DeviceInitializeZPODD: ZPODD is enabled.\n"
                ));

    DeviceExtension->ZeroPowerODDInfo = zpoddInfo;

    //
    // If device is not in home position, then we should take an active reference here
    // to prevent it from being powered off.
    //

    inHomePosition = DeviceZPODDIsInHomePosition(DeviceExtension);

    if (inHomePosition == FALSE)
    {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_POWER,
                   "DeviceInitializeZPODD: not ready to power off, device marked as active\n"));

        DeviceMarkActive(DeviceExtension, TRUE, FALSE);
    }
    else
    {
        //
        // cache get configuration response.
        // failing is not critical, so we don't want to check for status here.
        //

        if (zpoddInfo->GetConfigurationBuffer == NULL)
        {
            tempStatus = DeviceGetConfigurationWithAlloc(DeviceExtension->Device,
                                                         &zpoddInfo->GetConfigurationBuffer,
                                                         &zpoddInfo->GetConfigurationBufferSize,
                                                         FeatureProfileList,
                                                         SCSI_GET_CONFIGURATION_REQUEST_TYPE_ALL);

            UNREFERENCED_PARAMETER(tempStatus); // Avoid PREFAST warning.
        }
    }

Cleanup:

    if (!NT_SUCCESS(status))
    {
        //
        // We register always even in non-ZPODD case, per request from storport.
        //

        tempStatus = DeviceSendEnableIdlePowerIoctl(DeviceExtension, FALSE, FALSE, DELAY_TIME_TO_ENTER_ZERO_POWER_IN_MS);

        if (NT_SUCCESS(tempStatus))
        {
            //
            // Mark the device active; this reference will never be released unless the system enters a
            // low power state.
            //

            DeviceMarkActive(DeviceExtension, TRUE, FALSE);
        }

        FREE_POOL(zpoddInfo);
    }

    //
    // If Zero Power ODD is not supported, we should not block the device init sequence.
    //

    return STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
DeviceGetZPODDEnabledFromRegistry()
/*++

Routine Description:

    Get the ZeroPowerODDEnabled value from registry, which dictates if Zero Power ODD
    should be enabled or not. If the value is not in registry, by default Zero
    Power ODD is enabled.

Arguments:

    None

Return Value:

    ULONG

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    WDFKEY      registryKey = NULL;
    ULONG       ZPODDEnabled = 0;

    DECLARE_CONST_UNICODE_STRING(registryValueName, L"ZeroPowerODDEnabled");

    PAGED_CODE();

    //
    // open the Parameters key under the service key.
    //

    status = WdfDriverOpenParametersRegistryKey(WdfGetDriver(),
                                                KEY_READ,
                                                WDF_NO_OBJECT_ATTRIBUTES,
                                                &registryKey);

    if (NT_SUCCESS(status))
    {
        status = WdfRegistryQueryULong(registryKey,
                                       &registryValueName,
                                       &ZPODDEnabled);

        WdfRegistryClose(registryKey);
    }

    if (!NT_SUCCESS(status))
    {
        //
        // By default, Zero Power ODD is enabled
        //

        ZPODDEnabled = 1;
    }

    return ZPODDEnabled;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceQueryD3ColdInterface(
    _In_    PCDROM_DEVICE_EXTENSION     DeviceExtension,
    _Out_   PD3COLD_SUPPORT_INTERFACE   D3ColdInterface
    )
/*++

Routine Description:

    Queries ACPI for the D3Cold support interface.

Arguments:

    DeviceExtension - the device extension
    D3ColdInterface - output buffer receiving the interface

Return Value:

    NTSTATUS

--*/
{
    PIRP                irp = NULL;
    KEVENT              event;
    NTSTATUS            status = STATUS_SUCCESS;
    PDEVICE_OBJECT      targetDevice = NULL;
    IO_STATUS_BLOCK     ioStatus = {0};
    PIO_STACK_LOCATION  irpStack = NULL;

    PAGED_CODE();

    RtlZeroMemory(D3ColdInterface, sizeof(D3COLD_SUPPORT_INTERFACE));

    //
    // Query D3COLD support interface synchronously
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    targetDevice = IoGetAttachedDeviceReference(DeviceExtension->DeviceObject);

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       targetDevice,
                                       NULL,
                                       0,
                                       0,
                                       &event,
                                       &ioStatus);

    if (irp == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto Cleanup;
    }

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    irpStack = IoGetNextIrpStackLocation(irp);
    irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpStack->Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_D3COLD_SUPPORT_INTERFACE;
    irpStack->Parameters.QueryInterface.Size = sizeof (D3COLD_SUPPORT_INTERFACE);
    irpStack->Parameters.QueryInterface.Version = D3COLD_SUPPORT_INTERFACE_VERSION;
    irpStack->Parameters.QueryInterface.Interface = (PINTERFACE) D3ColdInterface;
    irpStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    status = IoCallDriver(targetDevice, irp);

    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status))
    {
        goto Cleanup;
    }

    NT_ASSERT(D3ColdInterface->SetD3ColdSupport != NULL);
    NT_ASSERT(D3ColdInterface->GetIdleWakeInfo != NULL);
    NT_ASSERT(D3ColdInterface->GetD3ColdCapability != NULL);

Cleanup:

    ObDereferenceObject(targetDevice);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceSendEnableIdlePowerIoctl(
    _In_    PCDROM_DEVICE_EXTENSION     DeviceExtension,
    _In_    BOOLEAN                     WakeCapable,
    _In_    BOOLEAN                     Enable,
    _In_    ULONG                       D3IdleTimeout
    )
/*++

Routine Description:

    Enables idle power support.

Arguments:

    DeviceExtension - the device extension
    WakeCapable - whether the device is wake capable
    Enable - enable / disable idle power management

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    STORAGE_IDLE_POWER  idlePower = {0};
    IO_STATUS_BLOCK     ioStatus = {0};
    PIRP                irp = NULL;
    KEVENT              event;

    PAGED_CODE();

    idlePower.Version = 1;
    idlePower.Size = sizeof (STORAGE_IDLE_POWER);
    idlePower.WakeCapableHint = WakeCapable;
    idlePower.D3ColdSupported = Enable;
    idlePower.D3IdleTimeout = D3IdleTimeout;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_ENABLE_IDLE_POWER,
                                        DeviceExtension->LowerPdo,
                                        &idlePower,
                                        sizeof(STORAGE_IDLE_POWER),
                                        NULL,
                                        0,
                                        FALSE,
                                        &event,
                                        &ioStatus);

    if (irp == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        //
        // Send the synchronous request to port driver.
        //

        status = IoCallDriver(DeviceExtension->LowerPdo, irp);

        if (status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

            status = ioStatus.Status;
        }
    }

    TracePrint((TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_POWER,
                "DeviceSendEnableIdlePowerIoctl: Port driver returned status (%x) for FDO (%p)\n"
                "\tD3ColdSupported: %u\n"
                "\tD3IdleTimeout: %u (ms)",
                status,
                DeviceExtension->DeviceObject,
                Enable,
                DELAY_TIME_TO_ENTER_ZERO_POWER_IN_MS));

    return status;
}

_IRQL_requires_max_(APC_LEVEL)
VOID
DeviceReleaseZPODDResources(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine will cleanup any resources allocated for ZPODD.

Arguments:

    DeviceExtension - the device context

Return Value:
    None.

--*/
{
    PZERO_POWER_ODD_INFO zpoddInfo = DeviceExtension->ZeroPowerODDInfo;

    PAGED_CODE()

    if (zpoddInfo != NULL)
    {
        FREE_POOL(zpoddInfo->GetConfigurationBuffer);
        FREE_POOL(zpoddInfo);
    }

    return;
}

NTSTATUS
DeviceZPODDGetPowerupReason(
    _In_    PCDROM_DEVICE_EXTENSION DeviceExtension,
    _Out_   PSTORAGE_IDLE_POWERUP_REASON PowerupReason
    )
/*++

Routine Description:

    This routine queries the port driver for what caused the power up.

Arguments:

    DeviceExtension - device extension.
    PowerupReason - what caused the power up.

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PIRP                    irp = NULL;
    IO_STATUS_BLOCK         ioStatus = {0};
    KEVENT                  event;

    RtlZeroMemory(PowerupReason, sizeof (STORAGE_IDLE_POWERUP_REASON));

    PowerupReason->Size = sizeof (STORAGE_IDLE_POWERUP_REASON);
    PowerupReason->Version = STORAGE_IDLE_POWERUP_REASON_VERSION_V1;

    //
    // Setup a synchronous irp.
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_IDLE_POWERUP_REASON,
                                        DeviceExtension->LowerPdo,
                                        PowerupReason,
                                        sizeof (STORAGE_IDLE_POWERUP_REASON),
                                        PowerupReason,
                                        sizeof (STORAGE_IDLE_POWERUP_REASON),
                                        FALSE,
                                        &event,
                                        &ioStatus);

    if (irp == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        //
        // Send the synchronous request to port driver.
        //

        status = IoCallDriver(DeviceExtension->LowerPdo, irp);

        if (status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

            status = ioStatus.Status;
        }
    }

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN
DeviceZPODDIsInHomePosition(
    _In_    PCDROM_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Checks to see if the device is ready to be powered off.
    Requirements are: 1. tray closed 2. no media present.

Arguments:

    DeviceExtension - device extension.

Return Value:
    BOOLEAN

--*/
{
    NTSTATUS             status = STATUS_SUCCESS;
    PZERO_POWER_ODD_INFO zpoddInfo = DeviceExtension->ZeroPowerODDInfo;
    SCSI_REQUEST_BLOCK   srb = {0};
    PCDB                 cdb = (PCDB) srb.Cdb;
    BOOLEAN              inHomePosition = FALSE;

    PAGED_CODE();

    if (zpoddInfo != NULL)
    {
        //
        // Clear sense data.
        //

        zpoddInfo->SenseKey = 0;
        zpoddInfo->AdditionalSenseCode = 0;
        zpoddInfo->AdditionalSenseCodeQualifier = 0;

        //
        // Send a Test Unit Ready to check media & tray status.
        //

        RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

        srb.CdbLength = 6;
        cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

        srb.TimeOutValue = CDROM_TEST_UNIT_READY_TIMEOUT;

        status = DeviceSendSrbSynchronously(DeviceExtension->Device,
                                            &srb,
                                            NULL,
                                            0,
                                            FALSE,
                                            NULL);

#ifdef __REACTOS__
        if (!NT_SUCCESS(status))
        {
            return FALSE;
        }
#endif

        //
        // At this time, sense data, if available, is already copied into zpoddInfo.
        //
        // We don't check for status because we expect it to fail in case there is no media in device.
        //
        // Should enter Zero Power state if:
        //
        // Drawer:  02/3A/01
        // Slot:    02/3A/xx
        //

        if (((zpoddInfo->LoadingMechanism == LOADING_MECHANISM_TRAY) && (zpoddInfo->Load == 0) && // Drawer
             (zpoddInfo->SenseKey                       == SCSI_SENSE_NOT_READY) &&
             (zpoddInfo->AdditionalSenseCode            == SCSI_ADSENSE_NO_MEDIA_IN_DEVICE) &&
             (zpoddInfo->AdditionalSenseCodeQualifier   == 0x01)) ||
            ((zpoddInfo->LoadingMechanism == LOADING_MECHANISM_CADDY) && (zpoddInfo->Load == 0) && // Slot
             (zpoddInfo->SenseKey                       == SCSI_SENSE_NOT_READY) &&
             (zpoddInfo->AdditionalSenseCode            == SCSI_ADSENSE_NO_MEDIA_IN_DEVICE)))
        {
            inHomePosition = TRUE;
        }
    }

    return inHomePosition;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
DeviceMarkActive(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ BOOLEAN                 IsActive,
    _In_ BOOLEAN                 SetIdleTimeout
    )
/*++

Routine Description:

    This routine will mark the device as active / idle.

Arguments:

    DeviceExtension - the device context
    IsActive - if the device should be marked as active

Return Value:
    None.

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PZERO_POWER_ODD_INFO    zpoddInfo = DeviceExtension->ZeroPowerODDInfo;

    PAGED_CODE()

    if (DeviceExtension->IsActive != IsActive)
    {
        if ((IsActive == FALSE) && (zpoddInfo != NULL))
        {
            // cache get configuration response.
            // failing is not critical, so we don't want to check for status here.
            if (zpoddInfo->GetConfigurationBuffer == NULL)
            {
                (VOID)DeviceGetConfigurationWithAlloc(DeviceExtension->Device,
                                                      &zpoddInfo->GetConfigurationBuffer,
                                                      &zpoddInfo->GetConfigurationBufferSize,
                                                      FeatureProfileList,
                                                      SCSI_GET_CONFIGURATION_REQUEST_TYPE_ALL);
            }
        }

        if (SetIdleTimeout)
        {
            status = DeviceSendEnableIdlePowerIoctl(DeviceExtension,
                                                    FALSE,
                                                    FALSE,
                                                    IsActive ? DELAY_TIME_TO_ENTER_ZERO_POWER_IN_MS : DELAY_TIME_TO_ENTER_AOAC_IDLE_POWER_IN_MS);
        }

        if (NT_SUCCESS(status))
        {
            DeviceSendIoctlAsynchronously(DeviceExtension,
                                          IsActive ? IOCTL_STORAGE_POWER_ACTIVE : IOCTL_STORAGE_POWER_IDLE,
                                          DeviceExtension->LowerPdo);
        }

        DeviceExtension->IsActive = IsActive;
    }
}

#pragma warning(pop) // un-sets any local warning changes


