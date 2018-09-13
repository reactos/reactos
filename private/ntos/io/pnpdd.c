/*++

Copyright (c) 1991  Microsoft Corporation
All rights reserved

Module Name:

    pnpdd.c

Abstract:

    This module implements new Plug-And-Play driver entries and IRPs.

Author:

    Shie-Lin Tzong (shielint) June-16-1995

Environment:

    Kernel mode only.

Revision History:

*/

#include "iop.h"
#pragma hdrstop

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'ddpP')
#endif

//
// Internal definitions and references
//

typedef struct _DEVICE_LIST_CONTEXT {
    ULONG DeviceCount;
    BOOLEAN Reallocation;
    PDEVICE_OBJECT DeviceList[1];
} DEVICE_LIST_CONTEXT, *PDEVICE_LIST_CONTEXT;

BOOLEAN
IopAddDevicesToBootDriverWorker(
    IN HANDLE DeviceInstanceHandle,
    IN PUNICODE_STRING DeviceInstancePath,
    IN OUT PVOID Context
    );

NTSTATUS
IopProcessAddDevicesWorker (
   IN PDEVICE_NODE DeviceNode,
   IN PVOID Context
   );

NTSTATUS
IopProcessAssignResourcesWorker (
   IN PDEVICE_NODE DeviceNode,
   IN PVOID Context
   );

VOID
IopPnPCompleteRequest(
    IN OUT PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

NTSTATUS
IopGetDriverDeviceList (
   IN PDRIVER_OBJECT DriverObject,
   OUT PDEVICE_LIST_CONTEXT *DeviceList
   );

BOOLEAN
IopGetDriverDeviceListWorker(
    IN HANDLE DeviceInstanceHandle,
    IN PUNICODE_STRING DeviceInstancePath,
    IN OUT PVOID Context
    );

NTSTATUS
IopAssignResourcesToDevices (
    IN ULONG DeviceCount,
    IN PIOP_RESOURCE_REQUEST RequestTable,
    IN BOOLEAN BootConfigsOK
    );

BOOLEAN
IopIsFirmwareDisabled (
    IN PDEVICE_NODE DeviceNode
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, IopAddDevicesToBootDriver)
#pragma alloc_text(INIT, IopAddDevicesToBootDriverWorker)
//#pragma alloc_text(INIT, IopReportResourcesForAllChildren)
#pragma alloc_text(PAGE, IopReleaseDeviceResources)
#pragma alloc_text(PAGE, IopPnPAddDevice)
#pragma alloc_text(PAGE, IopPnPDispatch)
#pragma alloc_text(INIT, IopProcessAddDevices)
#pragma alloc_text(INIT, IopProcessAddDevicesWorker)
#pragma alloc_text(PAGE, IopProcessAssignResources)
#pragma alloc_text(PAGE, IopProcessAssignResourcesWorker)
#pragma alloc_text(PAGE, IopGetDriverDeviceList)
#pragma alloc_text(PAGE, IopGetDriverDeviceListWorker)
#pragma alloc_text(PAGE, IopNewDevice)
#pragma alloc_text(PAGE, IopStartDriverDevices)
#pragma alloc_text(PAGE, IopAssignResourcesToDevices)
#pragma alloc_text(PAGE, IopWriteAllocatedResourcesToRegistry)
#pragma alloc_text(PAGE, IopIsFirmwareDisabled)
#endif

NTSTATUS
IopAddDevicesToBootDriver (
   IN PDRIVER_OBJECT DriverObject
   )

/*++

Routine Description:

    This functions is used by Pnp manager to inform a boot device driver of
    all the devices it can possibly control.  This routine is for boot
    drivers only.

Parameters:

    DriverObject - Supplies a driver object to receive its boot devices.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS status;


    //
    // For each device instance in the driver's service/enum key, we will
    // invoke the driver's AddDevice routine and perform enumeration on
    // the device.
    // Note, we don't acquire registry lock before calling IopApplyFunction
    // routine.  We know this code is for boot driver initialization.  No
    // one else would access the registry Enum key at this time and most
    // important we need the registry lock in other down level routines.
    //

    status = IopApplyFunctionToServiceInstances(
                                NULL,
                                &DriverObject->DriverExtension->ServiceKeyName,
                                KEY_ALL_ACCESS,
                                TRUE,
                                IopAddDevicesToBootDriverWorker,
                                DriverObject,
                                NULL
                                );

    return status;
}

BOOLEAN
IopAddDevicesToBootDriverWorker(
    IN HANDLE DeviceInstanceHandle,
    IN PUNICODE_STRING DeviceInstancePath,
    IN OUT PVOID Context
    )

/*++

Routine Description:

    This routine is a callback function for IopApplyFunctionToServiceInstances.
    It is called for each device instance key referenced by a service instance
    value under the specified service's volatile Enum subkey. The purpose of this
    routine is to invoke the AddDevice() entry of a boot driver with the device
    object.

    Note this routine is also used for the devices controlled by a legacy driver.
    If the specified device instance is controlled by a legacy driver this routine
    sets the device node flags.

Arguments:

    DeviceInstanceHandle - Supplies a handle to the registry path (relative to
        HKLM\CCS\System\Enum) to this device instance.

    DeviceInstancePath - Supplies the registry path (relative to HKLM\CCS\System\Enum)
        to this device instance.

    Context - Supplies a pointer to a DRIVER_OBJECT structure.

Return Value:

    TRUE to continue the enumeration.
    FALSE to abort it.

--*/

{
    NTSTATUS status;
//  PDRIVER_OBJECT driverObject = (PDRIVER_OBJECT)Context;
    PDEVICE_OBJECT physicalDevice;
    PDEVICE_NODE deviceNode;
    ULONG length;
    BOOLEAN conflict;
    PCM_RESOURCE_LIST cmResource;

    ADD_CONTEXT addContext;


    //
    // Reference the physical device object associated with the device instance.
    //

    physicalDevice = IopDeviceObjectFromDeviceInstance(DeviceInstanceHandle,
                                                       DeviceInstancePath);
    if (!physicalDevice) {
        return TRUE;
    }

    deviceNode = (PDEVICE_NODE)physicalDevice->DeviceObjectExtension->DeviceNode;
    ASSERT(deviceNode && (deviceNode->PhysicalDeviceObject == physicalDevice));

    //
    // If the device has been added (or failed) skip it.
    //

    if (!OK_TO_ADD_DEVICE(deviceNode)) {
        goto exit;
    }

    //
    // If we know the device is a duplicate of another device which
    // has been enumerated at this point. we will skip this device.
    //

    if ((deviceNode->Flags & DNF_DUPLICATE) && (deviceNode->DuplicatePDO)) {
        goto exit;
    }

    //
    // Invoke driver's AddDevice Entry for the device.
    // Since the driver has already been loaded, we generate a fake
    // context to make sure that group order won't get in the way of
    // adding the drivers.

    addContext.GroupsToStart = 0xffff;
    addContext.GroupToStartNext = 0;
    addContext.DriverStartType = SERVICE_BOOT_START;

    IopCallDriverAddDevice(deviceNode, FALSE, &addContext);

exit:
    ObDereferenceObject(physicalDevice);
    return TRUE;
}

NTSTATUS
IopReleaseDeviceResources (
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN ReserveResources
    )

/*++

Routine Description:

    This routine releases the resources assigned to a device.

Arguments:

    DeviceObject - supplies a pointer to a device whose resources are to be released.

    ReserveResources - indicates whether we need to re-query and reserve BOOT config for DeviceObject.

Return Value:

    NTSTATUS code.


--*/
{
    BOOLEAN             conflict;
    NTSTATUS            status= STATUS_SUCCESS;
    PCM_RESOURCE_LIST   cmResource;
    ULONG               cmLength;

    PAGED_CODE();

    if (DeviceNode->ResourceList || (DeviceNode->Flags & DNF_BOOT_CONFIG_RESERVED)) {

        cmResource = NULL;
        cmLength = 0;

        //
        // If needed, re-query for BOOT configs. We need to do this BEFORE we
        // release the BOOT config (otherwise ROOT devices cannot report BOOT
        // config).
        //

        if (ReserveResources && !(DeviceNode->Flags & DNF_MADEUP)) {

            //
            // First query for new BOOT config (order important for ROOT devices).
            //

            status = IopQueryDeviceResources (  DeviceNode->PhysicalDeviceObject,
                                                QUERY_RESOURCE_LIST,
                                                &cmResource,
                                                &cmLength);

            if (!NT_SUCCESS(status)) {

                cmResource = NULL;
                cmLength = 0;

            }
        }

        //
        // Release resources for this device.
        //

        status = IopLegacyResourceAllocation(   ArbiterRequestUndefined,
                                                IoPnpDriverObject,
                                                DeviceNode->PhysicalDeviceObject,
                                                NULL,
                                                NULL);
        if (NT_SUCCESS(status)) {

            IopResourcesReleased = TRUE;        // Signal there are resources available.

            //
            // If needed, re-query and reserve current BOOT config for this device.
            // We always rereserve the boot config (ie DNF_MADEUP root enumerated
            // and IoReportDetected) devices in IopLegacyResourceAllocation.
            //

            if (ReserveResources && !(DeviceNode->Flags & DNF_MADEUP)) {

                UNICODE_STRING      unicodeName;
                HANDLE              logConfHandle;
                HANDLE              handle;

                ASSERT(DeviceNode->BootResources == NULL);

                status = IopDeviceObjectToDeviceInstance(DeviceNode->PhysicalDeviceObject, &handle, KEY_ALL_ACCESS);
                logConfHandle = NULL;
                if (NT_SUCCESS(status)) {


                    PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
                    status = IopCreateRegistryKeyEx( &logConfHandle,
                                                     handle,
                                                     &unicodeName,
                                                     KEY_ALL_ACCESS,
                                                     REG_OPTION_NON_VOLATILE,
                                                     NULL);
                    if (!NT_SUCCESS(status)) {

                        logConfHandle = NULL;

                    }
                }

                if (logConfHandle) {

                    PiWstrToUnicodeString(&unicodeName, REGSTR_VAL_BOOTCONFIG);
                    KeEnterCriticalRegion();
                    ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);
                    if (cmResource) {

                        ZwSetValueKey(  logConfHandle,
                                        &unicodeName,
                                        TITLE_INDEX_VALUE,
                                        REG_RESOURCE_LIST,
                                        cmResource,
                                        cmLength);
                    } else {

                        ZwDeleteValueKey(logConfHandle, &unicodeName);

                    }
                    ExReleaseResource(&PpRegistryDeviceResource);
                    KeLeaveCriticalRegion();

                    ZwClose(logConfHandle);
                }

                //
                // Reserve any remaining BOOT config.
                //

                if (cmResource) {

                    DeviceNode->Flags |= DNF_HAS_BOOT_CONFIG;
                    DeviceNode->BootResources = cmResource;

                    //
                    // This device consumes BOOT resources.  Reserve its boot resources
                    //

                    (*IopReserveResourcesRoutine)(  ArbiterRequestPnpEnumerated,
                                                    DeviceNode->PhysicalDeviceObject,
                                                    cmResource);
                }
            }
        }
    }

    return status;
}

NTSTATUS
IopPnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine handles AddDevice for an madeup PDO device.

Arguments:

    DriverObject - Pointer to our pseudo driver object.

    DeviceObject - Pointer to the device object for which this requestapplies.

Return Value:

    NT status.

--*/
{
    PAGED_CODE();

#if DBG

    //
    // We should never get an AddDevice request.
    //

    DbgBreakPoint();

#endif

    return STATUS_SUCCESS;
}
//  PNPRES test

NTSTATUS
IopArbiterHandlerxx (
    IN PVOID Context,
    IN ARBITER_ACTION Action,
    IN OUT PARBITER_PARAMETERS Parameters
    )
{
    PLIST_ENTRY listHead, listEntry;
    PIO_RESOURCE_DESCRIPTOR ioDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDesc;
    PARBITER_LIST_ENTRY arbiterListEntry;

    if (Action == ArbiterActionQueryArbitrate) {
        return STATUS_SUCCESS;
    }
    if (Parameters == NULL) {
        return STATUS_SUCCESS;
    }
    listHead = Parameters->Parameters.TestAllocation.ArbitrationList;
    if (IsListEmpty(listHead)) {
        return STATUS_SUCCESS;
    }
    listEntry = listHead->Flink;
    while (listEntry != listHead) {
      arbiterListEntry = (PARBITER_LIST_ENTRY)listEntry;
      cmDesc = arbiterListEntry->Assignment;
      ioDesc = arbiterListEntry->Alternatives;
      if (cmDesc == NULL || ioDesc == NULL) {
          return STATUS_SUCCESS;
      }
      cmDesc->Type = ioDesc->Type;
      cmDesc->ShareDisposition = ioDesc->ShareDisposition;
      cmDesc->Flags = ioDesc->Flags;
      if (ioDesc->Type == CmResourceTypePort) {
          cmDesc->u.Port.Start = ioDesc->u.Port.MinimumAddress;
          cmDesc->u.Port.Length = ioDesc->u.Port.Length;
      } else if (ioDesc->Type == CmResourceTypeInterrupt) {
          cmDesc->u.Interrupt.Level = ioDesc->u.Interrupt.MinimumVector;
          cmDesc->u.Interrupt.Vector = ioDesc->u.Interrupt.MinimumVector;
          cmDesc->u.Interrupt.Affinity = (ULONG) -1;
      } else if (ioDesc->Type == CmResourceTypeMemory) {
          cmDesc->u.Memory.Start = ioDesc->u.Memory.MinimumAddress;
          cmDesc->u.Memory.Length = ioDesc->u.Memory.Length;
      } else if (ioDesc->Type == CmResourceTypeDma) {
          cmDesc->u.Dma.Channel = ioDesc->u.Dma.MinimumChannel;
          cmDesc->u.Dma.Port = 0;
          cmDesc->u.Dma.Reserved1 = 0;
      }
      listEntry = listEntry->Flink;
    }
    return STATUS_SUCCESS;
}
NTSTATUS
IopTranslatorHandlerCm (
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT DeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    )
{
    *Target = *Source;
#if 0
    if (Direction == TranslateChildToParent) {
        if (Target->Type == CmResourceTypePort) {
            Target->u.Port.Start.LowPart += 0x10000;
        } else if (Target->Type == CmResourceTypeMemory) {
            Target->u.Memory.Start.LowPart += 0x100000;
        }
    } else {
        if (Target->Type == CmResourceTypePort) {
            Target->u.Port.Start.LowPart -= 0x10000;
        } else if (Target->Type == CmResourceTypeMemory) {
            Target->u.Memory.Start.LowPart -= 0x100000;
        }
    }
#endif
    return STATUS_SUCCESS;
}
NTSTATUS
IopTranslatorHandlerIo (
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    )
{
    PIO_RESOURCE_DESCRIPTOR newDesc;

    newDesc = (PIO_RESOURCE_DESCRIPTOR) ExAllocatePool(PagedPool, sizeof(IO_RESOURCE_DESCRIPTOR));
    if (newDesc == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    *TargetCount = 1;
    *newDesc = *Source;
#if 0
    if (newDesc->Type == CmResourceTypePort) {
        newDesc->u.Port.MinimumAddress.LowPart += 0x10000;
        newDesc->u.Port.MaximumAddress.LowPart += 0x10000;
    } else if (newDesc->Type == CmResourceTypeMemory) {
        newDesc->u.Memory.MinimumAddress.LowPart += 0x100000;
        newDesc->u.Memory.MaximumAddress.LowPart += 0x100000;
    }
#endif
    *Target = newDesc;
    return STATUS_SUCCESS;
}

NTSTATUS
IopPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION      IrpSp;
    PPOWER_SEQUENCE         PowerSequence;
    NTSTATUS                Status;


    UNREFERENCED_PARAMETER( DeviceObject );

    IrpSp = IoGetCurrentIrpStackLocation (Irp);
    Status = Irp->IoStatus.Status;

    switch (IrpSp->MinorFunction) {
        case IRP_MN_WAIT_WAKE:
            Status = STATUS_NOT_SUPPORTED;
            break;

        case IRP_MN_POWER_SEQUENCE:
            PowerSequence = IrpSp->Parameters.PowerSequence.PowerSequence;
            PowerSequence->SequenceD1 = PoPowerSequence;
            PowerSequence->SequenceD2 = PoPowerSequence;
            PowerSequence->SequenceD3 = PoPowerSequence;
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_POWER:
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_SET_POWER:
            switch (IrpSp->Parameters.Power.Type) {
                case SystemPowerState:
                    Status = STATUS_SUCCESS;
                    break;

                case DevicePowerState:
                    //
                    // To be here the FDO must have passed the IRP on.
                    // We do not know how to turn the device off, but the
                    // FDO is prepaired for it work
                    //

                    Status = STATUS_SUCCESS;
                    break;

                default:
                    // Unkown power type
                    Status = STATUS_NOT_SUPPORTED;
                    break;
            }
            break;

        default:
            // Unkown power minor code
            Status = STATUS_NOT_SUPPORTED;
            break;
    }


    //
    // For lagecy devices that do not have drivers loaded, complete
    // power irps with success.
    //

    PoStartNextPowerIrp(Irp);
    if (Status != STATUS_NOT_SUPPORTED) {
       Irp->IoStatus.Status = Status;
    } else {
       Status = Irp->IoStatus.Status;
    }
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return Status;
}

NTSTATUS
IopPnPDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles all IRP_MJ_PNP IRPs for madeup PDO device.

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/
{
    PIOPNP_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PVOID information = NULL;
    ULONG length;
    PWCHAR id, wp;
    PDEVICE_NODE deviceNode;
    PARBITER_INTERFACE arbiterInterface;  // PNPRES test
    PTRANSLATOR_INTERFACE translatorInterface;  // PNPRES test

    PAGED_CODE();

    //
    // Get a pointer to our stack location and take appropriate action based
    // on the minor function.
    //

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    switch (irpSp->MinorFunction){

    case IRP_MN_DEVICE_USAGE_NOTIFICATION:
    case IRP_MN_START_DEVICE:

        //
        // If we get a start device request for a PDO, we simply
        // return success.
        //

        status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:

        //
        // As we fail all STOP's, this cancel is always successful, and we have
        // no work to do.
        //
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_STOP_DEVICE:

        //
        // We can not success the query stop.  We don't handle it.  because
        // we don't know how to stop a root enumerated device.
        //
        status = STATUS_UNSUCCESSFUL ;
        break;

    case IRP_MN_QUERY_RESOURCES:

        status = IopGetDeviceResourcesFromRegistry(
                         DeviceObject,
                         QUERY_RESOURCE_LIST,
                         REGISTRY_BOOT_CONFIG,
                         &information,
                         &length);
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
            status = STATUS_SUCCESS;
            information = NULL;
        }
        break;

    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:

        status = IopGetDeviceResourcesFromRegistry(
                         DeviceObject,
                         QUERY_RESOURCE_REQUIREMENTS,
                         REGISTRY_BASIC_CONFIGVECTOR,
                         &information,
                         &length);
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
            status = STATUS_SUCCESS;
            information = NULL;
        }
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_REMOVE_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:

        //
        // For root enumerated devices we let the device objects stays.
        // So, they will be marked as deleted but still show up in the tree.
        //

        //IoDeleteDevice(DeviceObject);
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:

        if (DeviceObject == IopRootDeviceNode->PhysicalDeviceObject &&
            irpSp->Parameters.QueryDeviceRelations.Type == BusRelations) {
            status = IopGetRootDevices((PDEVICE_RELATIONS *)&information);
        } else {
            if (irpSp->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation) {
                PDEVICE_RELATIONS deviceRelations;

                deviceRelations = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
                if (deviceRelations == NULL) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    deviceRelations->Count = 1;
                    deviceRelations->Objects[0] = DeviceObject;
                    ObReferenceObject(DeviceObject);
                    information = (PVOID)deviceRelations;
                    status = STATUS_SUCCESS;
                }
            } else {
                information = (PVOID)Irp->IoStatus.Information;
                status = Irp->IoStatus.Status;
            }
        }
        break;

    case IRP_MN_QUERY_INTERFACE:
        deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
        if (deviceNode == IopRootDeviceNode) {
            if ( IopCompareGuid((PVOID)irpSp->Parameters.QueryInterface.InterfaceType, (PVOID)&GUID_ARBITER_INTERFACE_STANDARD)) {
                status = STATUS_SUCCESS;
                arbiterInterface = (PARBITER_INTERFACE) irpSp->Parameters.QueryInterface.Interface;
                arbiterInterface->ArbiterHandler = ArbArbiterHandler;
                switch ((UCHAR)((ULONG_PTR)irpSp->Parameters.QueryInterface.InterfaceSpecificData)) {
                case CmResourceTypePort:
                    arbiterInterface->Context = (PVOID) &IopRootPortArbiter;
                    break;
                case CmResourceTypeMemory:
                    arbiterInterface->Context = (PVOID) &IopRootMemArbiter;
                    break;
                case CmResourceTypeInterrupt:
                    arbiterInterface->Context = (PVOID) &IopRootIrqArbiter;
                    break;
                case CmResourceTypeDma:
                    arbiterInterface->Context = (PVOID) &IopRootDmaArbiter;
                    break;
                case CmResourceTypeBusNumber:
                    arbiterInterface->Context = (PVOID) &IopRootBusNumberArbiter;
                    break;
                default:
                    status = STATUS_INVALID_PARAMETER;
                    break;
                }
            } else if ( IopCompareGuid((PVOID)irpSp->Parameters.QueryInterface.InterfaceType, (PVOID)&GUID_TRANSLATOR_INTERFACE_STANDARD)) {
                translatorInterface = (PTRANSLATOR_INTERFACE) irpSp->Parameters.QueryInterface.Interface;
                translatorInterface->TranslateResources = IopTranslatorHandlerCm;
                translatorInterface->TranslateResourceRequirements = IopTranslatorHandlerIo;
                status = STATUS_SUCCESS;
            }
            break;
        }

        status = Irp->IoStatus.Status;
        break;

    case IRP_MN_QUERY_CAPABILITIES:

        {
            ULONG i;
            PDEVICE_POWER_STATE state;
            PDEVICE_CAPABILITIES deviceCapabilities;

            deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

            deviceCapabilities = irpSp->Parameters.DeviceCapabilities.Capabilities;
            deviceCapabilities->Size = sizeof(DEVICE_CAPABILITIES);
            deviceCapabilities->Version = 1;

            deviceCapabilities->DeviceState[PowerSystemUnspecified]=PowerDeviceUnspecified;
            deviceCapabilities->DeviceState[PowerSystemWorking]=PowerDeviceD0;

            state = &deviceCapabilities->DeviceState[PowerSystemSleeping1];

            for (i = PowerSystemSleeping1; i < PowerSystemMaximum; i++) {

                //
                // Only supported state, currently, is off.
                //

                *state++ = PowerDeviceD3;
            }


            if(IopIsFirmwareDisabled(deviceNode)) {
                //
                // this device has been disabled by BIOS
                //
                deviceCapabilities->HardwareDisabled = TRUE;
            }

            status = STATUS_SUCCESS;
        }
        break;

    case IRP_MN_QUERY_ID:
        if (DeviceObject != IopRootDeviceNode->PhysicalDeviceObject &&
            (!NT_SUCCESS(Irp->IoStatus.Status) || !Irp->IoStatus.Information)) {

            deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
            switch (irpSp->Parameters.QueryId.IdType) {

            case BusQueryInstanceID:
            case BusQueryDeviceID:

                id = (PWCHAR)ExAllocatePool(PagedPool, deviceNode->InstancePath.Length);
                if (id) {
                    ULONG separatorCount = 0;

                    RtlZeroMemory(id, deviceNode->InstancePath.Length);
                    information = id;
                    status = STATUS_SUCCESS;
                    wp = deviceNode->InstancePath.Buffer;
                    if (irpSp->Parameters.QueryId.IdType == BusQueryDeviceID) {
                        while(*wp) {
                            if (*wp == OBJ_NAME_PATH_SEPARATOR) {
                                separatorCount++;
                                if (separatorCount == 2) {
                                    break;
                                }
                            }
                            *id = *wp;
                            id++;
                            wp++;
                        }
                    } else {
                        while(*wp) {
                            if (*wp == OBJ_NAME_PATH_SEPARATOR) {
                                separatorCount++;
                                if (separatorCount == 2) {
                                    wp++;
                                    break;
                                }
                            }
                            wp++;
                        }
                        while (*wp) {
                            *id = *wp;
                            id++;
                            wp++;
                        }
                    }
                } else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
                break;

            case BusQueryCompatibleIDs:

                if((Irp->IoStatus.Status != STATUS_NOT_SUPPORTED) ||
                   (deviceExtension == NULL))  {

                    //
                    // Upper driver has given some sort of reply or this device
                    // object wasn't allocated to handle these requests.
                    //

                    status = Irp->IoStatus.Status;
                    break;
                }

                if(deviceExtension->CompatibleIdListSize != 0) {

                    id = ExAllocatePool(PagedPool,
                                        deviceExtension->CompatibleIdListSize);

                    if(id == NULL) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    RtlCopyMemory(id,
                                  deviceExtension->CompatibleIdList,
                                  deviceExtension->CompatibleIdListSize);

                    information = id;
                    status = STATUS_SUCCESS;
                    break;
                }

            default:

                information = (PVOID)Irp->IoStatus.Information;
                status = Irp->IoStatus.Status;
            }
        } else {
            information = (PVOID)Irp->IoStatus.Information;
            status = Irp->IoStatus.Status;
        }

        break;
#if 0
    case IRP_MN_QUERY_BUS_INFORMATION:
        deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
        if (deviceNode == IopRootDeviceNode) {
            PPNP_BUS_INFORMATION busInfo;

            busInfo = (PPNP_BUS_INFORMATION) ExAllocatePool(PagedPool, sizeof(PNP_BUS_INFORMATION));
            if (busInfo) {
                busInfo->LegacyBusType = PnpDefaultInterfaceType;
                busInfo->BusNumber = 0;
                if (PnpDefaultInterfaceType == Isa) {
                    busInfo->BusTypeGuid = GUID_BUS_TYPE_ISAPNP; // BUGBUG
                } else if (PnpDefaultInterfaceType == Eisa) {
                    busInfo->BusTypeGuid = GUID_BUS_TYPE_EISA;
                } else { // Microchannel
                    busInfo->BusTypeGuid = GUID_BUS_TYPE_MCA;
                }
                information = busInfo;
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
                information = NULL;
            }
            break;
        }
#endif

        //
        // Otherwise, let it fall through the default path.
        //

    default:

        information = (PVOID)Irp->IoStatus.Information;
        status = Irp->IoStatus.Status;
        break;
    }

    //
    // Complete the Irp and return.
    //

    IopPnPCompleteRequest(Irp, status, (ULONG_PTR)information);
    return status;
}

VOID
IopPnPCompleteRequest(
    IN OUT PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )

/*++

Routine Description:

    This routine completes PnP irps for our pseudo driver.

Arguments:

    Irp - Supplies a pointer to the irp to be completed.

    Status - completion status.

    Information - completion information to be passed back.

Return Value:

    None.

--*/

{
    KIRQL oldIrql;

    //
    // Complete the IRP.  First update the status...
    //

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;

    //
    // ... and complete it.
    //

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

USHORT
IopProcessAddDevices (
   IN PDEVICE_NODE DeviceNode,
   IN USHORT StartOrder,
   IN ULONG  DriverStartType
   )

/*++

Routine Description:

    This functions is used by Pnp manager to load driver and perform AddDevice for
    all the devices under DeviceNode subtree.  This routine is used at boot time
    to process devices which were not processed during boot driver initialization.

    Note, caller must acquire the enumeration mutex of the DeviceNode before calling
    this routine.

Parameters:

    DeviceNode - Specifies the device node whose subtree is to be checked for StartDevice.

    StartOrder - The group orders to start.

Return Value:

    TRUE - if any new device is successfully added to its driver.

--*/
{
    PDEVICE_NODE deviceNode, nextDeviceNode;
    USHORT returnValue = NO_MORE_GROUP;
    ADD_CONTEXT context;

    context.GroupsToStart = StartOrder;
    context.GroupToStartNext = NO_MORE_GROUP;
    context.DriverStartType = DriverStartType;

    //
    // Traverse the device node subtree to perform AddDevice call.
    //

    if (DeviceNode != IopRootDeviceNode) {
        IopAcquireEnumerationLock(DeviceNode);
    }

    deviceNode = DeviceNode->Child;
    while (deviceNode) {

        //
        // We need to remember the 'next' device node.  If the deviceNode is a madeup device
        // it may be deleted while processing the IopProcessAddDeviceWorker.
        //

        nextDeviceNode = deviceNode->Sibling;
        IopProcessAddDevicesWorker(deviceNode, &context);
        deviceNode = nextDeviceNode;
    }
    if (DeviceNode != IopRootDeviceNode) {
        IopReleaseEnumerationLock(DeviceNode);
    }
    return context.GroupToStartNext;
}

NTSTATUS
IopProcessAddDevicesWorker (
   IN PDEVICE_NODE DeviceNode,
   IN PVOID Context
   )

/*++

Routine Description:

    This function checks if the driver for the DeviceNode is present and loads
    the driver if necessary.

Arguments:

    DeviceNode - Supplies a pointer to the device node which is to be added.

    Context - Supplies a pointer to a BOOLEAN value to indicate if the specified device
              is successfully added to its driver.

Return Value:

    NTSTATUS code.

--*/

{
    PADD_CONTEXT addContext = (PADD_CONTEXT)Context;


    if (DeviceNode->Flags & DNF_ADDED) {

        //
        // If the device has been added, move to its children.
        //

        //
        // Acquire enumeration mutex to make sure its children won't change by
        // someone else.  Note, the current device node is protected by its parent's
        // Enumeration mutex and it won't disappear either.
        //

        IopAcquireEnumerationLock(DeviceNode);

        //
        // Recursively mark all of our children deleted.
        //

        IopForAllChildDeviceNodes(DeviceNode, IopProcessAddDevicesWorker, Context);

        //
        // Release the enumeration mutex of the device node.
        //

        IopReleaseEnumerationLock(DeviceNode);

    } else if (OK_TO_ADD_DEVICE(DeviceNode)) {

        //
        // If this device is not added (not because of failure), load its controlling
        // driver and invoke the driver's AddDevice entry.
        //

        IopCallDriverAddDevice(DeviceNode, TRUE, addContext);
    }
    return STATUS_SUCCESS;
}

BOOLEAN
IopProcessAssignResources (
   IN PDEVICE_NODE DeviceNode,
   IN BOOLEAN Reallocation,
   IN BOOLEAN BootConfigsOK
   )

/*++

Routine Description:

    This functions is used by Pnp manager to allocate resources for the devices
    which have been successfully added to their drivers and waiting to be started.

    Note, the caller must acquire the enumeration mutex before calling this routine.

Parameters:

    DeviceNode - specifies the device node whose subtree is to be checked for AssignRes.

    Reallocation - specifies if we are assigning resources for DNF_INSUFFICIENT_RESOURCES
                   devices.

    BootConfigsOK  - Indicates that it is OK to assign BOOT configs.

Return Value:

    TRUE - if more devices need resources.  This means we did not assign resources for all
    the devices.  caller must call this routine again.

--*/
{
    PDEVICE_NODE deviceNode;
    PDEVICE_LIST_CONTEXT context;
    BOOLEAN again = FALSE;
    ULONG count, i;
    PIOP_RESOURCE_REQUEST requestTable;

    PAGED_CODE();

    //
    // Allocate and init memory for resource context
    //

    context = (PDEVICE_LIST_CONTEXT) ExAllocatePool(
                                    PagedPool,
                                    sizeof(DEVICE_LIST_CONTEXT) +
                                    sizeof(PDEVICE_OBJECT) * IopNumberDeviceNodes
                                    );
    if (!context) {
        return again;
    }
    context->DeviceCount = 0;
    context->Reallocation = Reallocation;

    //
    // Parse the device node subtree to determine which devices need resources
    //

    IopAcquireEnumerationLock(DeviceNode);
    deviceNode = DeviceNode->Child;

    while (deviceNode) {
        IopProcessAssignResourcesWorker(deviceNode, context);
        deviceNode = deviceNode->Sibling;
    }
    IopReleaseEnumerationLock(DeviceNode);
    count = context->DeviceCount;
    if (count == 0) {
        ExFreePool(context);
        return again;
    }

    //
    // Need to assign resources to devices.  Build the resource request table and call
    // resource assignment routine.
    //

    requestTable = (PIOP_RESOURCE_REQUEST) ExAllocatePool(
                                    PagedPool,
                                    sizeof(IOP_RESOURCE_REQUEST) * count
                                    );
    if (requestTable) {

        for (i = 0; i < count; i++) {
            requestTable[i].Priority = 0;
            requestTable[i].PhysicalDevice = context->DeviceList[i];
        }

        //
        // Assign resources
        //

        ExAcquireResourceShared(&IopDeviceTreeLock, TRUE);
        IopAssignResourcesToDevices(count, requestTable, BootConfigsOK);

        //
        // Check the results
        //

        for (i = 0; i < count; i++) {

            deviceNode = (PDEVICE_NODE)
                          requestTable[i].PhysicalDevice->DeviceObjectExtension->DeviceNode;
            if (NT_SUCCESS(requestTable[i].Status)) {
                if (requestTable[i].ResourceAssignment) {
                    if (!(deviceNode->Flags & DNF_RESOURCE_REPORTED)) {
                        deviceNode->Flags |= DNF_RESOURCE_ASSIGNED;
                    }
                    //deviceNode->Flags &= ~DNF_INSUFFICIENT_RESOURCES;
                    deviceNode->ResourceList = requestTable[i].ResourceAssignment;
                    deviceNode->ResourceListTranslated = requestTable[i].TranslatedResourceAssignment;
                } else {
                    deviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED;
                }
            } else if (requestTable[i].Status == STATUS_RETRY) {
                again = TRUE;
            } else if (requestTable[i].Status == STATUS_DEVICE_CONFIGURATION_ERROR) {
                IopSetDevNodeProblem(deviceNode, CM_PROB_NO_SOFTCONFIG);
            } else if (requestTable[i].Status == STATUS_PNP_BAD_MPS_TABLE) {
                IopSetDevNodeProblem(deviceNode, CM_PROB_BIOS_TABLE);
            } else if (requestTable[i].Status == STATUS_PNP_TRANSLATION_FAILED) {
                IopSetDevNodeProblem(deviceNode, CM_PROB_TRANSLATION_FAILED);
            } else if (requestTable[i].Status == STATUS_PNP_IRQ_TRANSLATION_FAILED) {
                IopSetDevNodeProblem(deviceNode, CM_PROB_IRQ_TRANSLATION_FAILED);
            } else {
                IopSetDevNodeProblem(deviceNode, CM_PROB_NORMAL_CONFLICT);
            }

            //
            // IopProcessAssignReourcesWork marks the device nodes as DNF_ASSIGNING_RESOURCES
            // We need to clear it to indicate the assigment is done.
            //

            deviceNode->Flags &= ~DNF_ASSIGNING_RESOURCES;
        }
        ExReleaseResource(&IopDeviceTreeLock);
        ExFreePool(requestTable);
    }
    ExFreePool(context);
    return again;
}

NTSTATUS
IopProcessAssignResourcesWorker (
   IN PDEVICE_NODE DeviceNode,
   IN PVOID Context
   )

/*++

Routine Description:

    This functions searches the DeviceNode subtree to locate all the device objects
    which have been successfully added to their drivers and waiting for resources to
    be started.

Parameters:

    DeviceNode - specifies the device node whose subtree is to be checked for AssignRes.

    Context - specifies a pointer to a structure to pass resource assignment information.

Return Value:

    TRUE.

--*/
{
    PDEVICE_LIST_CONTEXT resourceContext = (PDEVICE_LIST_CONTEXT) Context;

    PAGED_CODE();

    //
    // If the device node/object has not been add, skip it.
    //

    if (!(DeviceNode->Flags & DNF_ADDED)) {
        return STATUS_SUCCESS;
    }

    if (resourceContext->Reallocation &&
        (   IopIsDevNodeProblem(DeviceNode, CM_PROB_NORMAL_CONFLICT) ||
            IopIsDevNodeProblem(DeviceNode, CM_PROB_TRANSLATION_FAILED) ||
            IopIsDevNodeProblem(DeviceNode, CM_PROB_IRQ_TRANSLATION_FAILED))) {
        IopClearDevNodeProblem(DeviceNode);
    }

    //
    // If the device object has not been started and has no resources yet.
    // Append it to our list.
    //

    if (    !(DeviceNode->Flags & DNF_START_PHASE) &&
            !(DeviceNode->Flags & DNF_ASSIGN_RESOURCE_PHASE)) {

           resourceContext->DeviceList[resourceContext->DeviceCount] =
                              DeviceNode->PhysicalDeviceObject;
           DeviceNode->Flags |= DNF_ASSIGNING_RESOURCES;
           resourceContext->DeviceCount++;

    } else {

        //
        // Acquire enumeration mutex to make sure its children won't change by
        // someone else.  Note, the current device node is protected by its parent's
        // Enumeration mutex and it won't disappear either.
        //

        IopAcquireEnumerationLock(DeviceNode);

        //
        // Recursively mark all of our children deleted.
        //

        IopForAllChildDeviceNodes(DeviceNode, IopProcessAssignResourcesWorker, Context);

        //
        // Release the enumeration mutex of the device node.
        //

        IopReleaseEnumerationLock(DeviceNode);
    }
    return STATUS_SUCCESS;
}

NTSTATUS
IopGetDriverDeviceList (
   IN PDRIVER_OBJECT DriverObject,
   OUT PDEVICE_LIST_CONTEXT *DeviceList
   )

/*++

Routine Description:

    This functions is used by Pnp manager to inform a device driver of
    all the devices it can possibly control.  See IopStartDriverDevices().

Parameters:

    DriverObject - Supplies a driver object to receive its boot devices.

    DeviceList - Specifies a pointer to a variable to receive the list of device
                 objects controlled by the driver.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS status;
    HANDLE enumHandle;
    ULONG count = 0;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PDEVICE_LIST_CONTEXT deviceList;

    PAGED_CODE();

    *DeviceList = NULL;
    KeEnterCriticalRegion();
    ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

    //
    // Open System\CurrentControlSet\Services
    //

    status = IopOpenServiceEnumKeys (
                 &DriverObject->DriverExtension->ServiceKeyName,
                 KEY_READ,
                 NULL,
                 &enumHandle,
                 FALSE
                 );
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    //
    // Read value of Count if present.  If it is not present, a value of
    // zero will be returned.
    //

    status = IopGetRegistryValue(enumHandle, REGSTR_VALUE_COUNT, &keyValueInformation);
    ZwClose(enumHandle);
    if (NT_SUCCESS(status)) {
        if (keyValueInformation->DataLength != 0) {
            count = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
        }
        ExFreePool(keyValueInformation);
    }

    if (count == 0) {
        goto exit;
    }

    deviceList = (PDEVICE_LIST_CONTEXT) ExAllocatePool(
                                PagedPool,
                                sizeof(DEVICE_LIST_CONTEXT) +
                                    sizeof(PDEVICE_OBJECT) * count * 2
                                );

    if (!deviceList) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    deviceList->DeviceCount = 0;

    //
    // For each device instance in the driver's service/enum key, we will
    // invoke the our worker routine to collect its corresponding device object.
    //

    status = IopApplyFunctionToServiceInstances(
                                NULL,
                                &DriverObject->DriverExtension->ServiceKeyName,
                                KEY_ALL_ACCESS,
                                TRUE,
                                IopGetDriverDeviceListWorker,
                                deviceList,
                                NULL
                                );
    *DeviceList = deviceList;
exit:
    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();
    return status;
}

BOOLEAN
IopGetDriverDeviceListWorker(
    IN HANDLE DeviceInstanceHandle,
    IN PUNICODE_STRING DeviceInstancePath,
    IN OUT PVOID Context
    )

/*++

Routine Description:

    This routine is a callback function for IopApplyFunctionToServiceInstances.
    It is called for each device instance key referenced by a service instance
    value under the specified service's volatile Enum subkey. The purpose of this
    routine is to generate a device list controlled by the same driver.

Arguments:

    DeviceInstanceHandle - Supplies a handle to the registry path (relative to
        HKLM\CCS\System\Enum) to this device instance.

    DeviceInstancePath - Supplies the registry path (relative to HKLM\CCS\System\Enum)
        to this device instance.

    Context - Supplies a pointer to a PDEVICE_LIST_CONTEXT structure.

Return Value:

    TRUE to continue the enumeration.
    FALSE to abort it.

--*/

{
    PDEVICE_LIST_CONTEXT deviceList = (PDEVICE_LIST_CONTEXT)Context;
    PDEVICE_OBJECT physicalDevice;
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    //
    // Reference the physical device object associated with the device instance.
    //

    physicalDevice = IopDeviceObjectFromDeviceInstance(DeviceInstanceHandle,
                                                       DeviceInstancePath);
    if (!physicalDevice) {
        return TRUE;
    }

    deviceNode = (PDEVICE_NODE)physicalDevice->DeviceObjectExtension->DeviceNode;

    //
    // Make sure the device instance is not disabled.
    //

    if (IopDoesDevNodeHaveProblem(deviceNode)) {
        goto exit;
    } else if (!IopIsDeviceInstanceEnabled(DeviceInstanceHandle, DeviceInstancePath, TRUE)) {
        goto exit;
    }

    //
    // If we know the device is a duplicate of another enumerated device and it
    // has been enumerated at this point. we will skip this device.
    //

    if ((deviceNode->Flags & DNF_DUPLICATE) && (deviceNode->DuplicatePDO)) {

        goto exit;
    }

    deviceList->DeviceList[deviceList->DeviceCount] = physicalDevice;
    deviceList->DeviceCount++;
    return TRUE;
exit:
    ObDereferenceObject(physicalDevice);
    return TRUE;
}

VOID
IopNewDevice(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine handles user-mode initiated starts of devices.

Parameters:

    DeviceObject - PDO.

ReturnValue:

    None.

--*/

{
    PDEVICE_NODE deviceNode, parent;
    IOP_RESOURCE_REQUEST requestTable;
    NTSTATUS status;
    BOOLEAN newDevice;
    START_CONTEXT startContext;
    ADD_CONTEXT addContext;

    PAGED_CODE();

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

    //
    // Make sure the device is not started
    //

    if (deviceNode->Flags & DNF_ADDED) {
        goto exit;
    }

    //
    // Need enumeration on IoReportDetectedDevice
    //

    if (!(deviceNode->Flags & DNF_NEED_QUERY_IDS)) {
        //
        // Invoke the driver's AddDevice() entry and enumerate the device.
        //

        addContext.GroupsToStart = 0xffff;
        addContext.GroupToStartNext = 0xffff;
        addContext.DriverStartType = SERVICE_DEMAND_START;

        status = IopCallDriverAddDevice(deviceNode, TRUE, &addContext);
        if (!NT_SUCCESS(status) || (deviceNode->Flags & DNF_LEGACY_DRIVER)) {
            goto exit;
        } else if (deviceNode->Flags & DNF_NEED_QUERY_IDS) {

            //
            // The driver may perform IoReportDetectedDevice
            //

            goto enumerate;
        }

        //
        // Assign resource to the device
        //

        requestTable.PhysicalDevice = DeviceObject;
        requestTable.Priority = 0;

        IopAssignResourcesToDevices(1, &requestTable, TRUE);

        if (NT_SUCCESS(requestTable.Status)) {
            if (requestTable.ResourceAssignment) {
                if (!(deviceNode->Flags & DNF_RESOURCE_REPORTED)) {
                    deviceNode->Flags |= DNF_RESOURCE_ASSIGNED;
                }
                // deviceNode->Flags &= ~DNF_INSUFFICIENT_RESOURCES;
                deviceNode->ResourceList = requestTable.ResourceAssignment;
                deviceNode->ResourceListTranslated = requestTable.TranslatedResourceAssignment;
            } else {
                deviceNode->Flags |= DNF_NO_RESOURCE_REQUIRED;
            }
        } else if (requestTable.Status == STATUS_DEVICE_CONFIGURATION_ERROR) {
            IopSetDevNodeProblem(deviceNode, CM_PROB_NO_SOFTCONFIG);
            goto exit;
        } else if (requestTable.Status == STATUS_PNP_BAD_MPS_TABLE) {
            IopSetDevNodeProblem(deviceNode, CM_PROB_BIOS_TABLE);
            goto exit;
        } else if (requestTable.Status == STATUS_PNP_TRANSLATION_FAILED) {
            IopSetDevNodeProblem(deviceNode, CM_PROB_TRANSLATION_FAILED);
            goto exit;
        } else if (requestTable.Status == STATUS_PNP_IRQ_TRANSLATION_FAILED) {
            IopSetDevNodeProblem(deviceNode, CM_PROB_IRQ_TRANSLATION_FAILED);
            goto exit;
        } else {
            IopSetDevNodeProblem(deviceNode, CM_PROB_NORMAL_CONFLICT);
            goto exit;
        }
    }

enumerate:

    //
    // Start and enumerate the device
    //

    startContext.LoadDriver = TRUE;
    startContext.NewDevice = FALSE;
    startContext.AddContext.GroupsToStart = NO_MORE_GROUP;
    startContext.AddContext.GroupToStartNext = NO_MORE_GROUP;
    startContext.AddContext.DriverStartType = SERVICE_DEMAND_START;

    IopStartAndEnumerateDevice(deviceNode, &startContext);
    newDevice = startContext.NewDevice;
    while (newDevice) {

        startContext.NewDevice = FALSE;

        //
        // Process the whole device tree to assign resources to those devices who
        // have been successfully added to their drivers.
        //

        newDevice = IopProcessAssignResources(deviceNode, FALSE, TRUE);

        //
        // Process the whole device tree to start those devices who have been allocated
        // resources and waiting to be started.
        // Note, the IopProcessStartDevices routine may enumerate new devices.
        //

        IopProcessStartDevices(deviceNode, &startContext);
        newDevice |= startContext.NewDevice;

    }

exit:
    ;
}

NTSTATUS
IopStartDriverDevices(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine is used by IopLoadDriver to add/start the devices controlled by
    the newly loaded driver.

Arguments:

    DriverObject - specifies the driver object which is being started thru
                   IopLoadDriver after the boot drivers and system drivers init
                   phase.

Return Value:

    NTSTATUS code.

--*/

{
    ULONG count, i;
    PDEVICE_LIST_CONTEXT deviceList;
    PDEVICE_NODE deviceNode;
    NTSTATUS status = STATUS_SUCCESS;
    PDRIVER_ADD_DEVICE addDeviceRoutine;

    PAGED_CODE();

    if (!PnPInitialized) {

        //
        // If this function is called at PnP Init time, we don't start the devices
        // for the specified driver.  The device add/start is handled by pnp based
        // on the device enumeration.
        //

        return status;
    }

    addDeviceRoutine = DriverObject->DriverExtension->AddDevice;
    if (addDeviceRoutine == NULL) {
        ASSERT(addDeviceRoutine);
        return status;
    }

    IopGetDriverDeviceList(DriverObject, &deviceList);
    count = deviceList ? deviceList->DeviceCount : 0;
    if (count == 0) {

        if (deviceList != NULL) {
            ExFreePool(deviceList);
        }

        return STATUS_PLUGPLAY_NO_DEVICE;
    }

    //
    // For each device int the list queue an IopNewDeviceEvent if the device is reported by driver
    //

    for (i = 0; i < count; i++) {

        deviceNode = (PDEVICE_NODE)deviceList->DeviceList[i]->DeviceObjectExtension->DeviceNode;
        if (!deviceNode || !(deviceNode->Flags & DNF_NEED_QUERY_IDS) ) {
            ObDereferenceObject(deviceList->DeviceList[i]);
            continue;
        }
        IopRequestDeviceAction(deviceList->DeviceList[i], StartDevice, NULL, NULL);
    }

    ExFreePool(deviceList);
    return status;
}

//
// The following routines should be removed once the real
// Resource Assign code is done.
//

NTSTATUS
IopAssignResourcesToDevices (
    IN ULONG DeviceCount,
    IN OUT PIOP_RESOURCE_REQUEST RequestTable,
    IN BOOLEAN BootConfigsOK
    )
/*++

Routine Description:

    This routine takes an input array of IOP_RESOURCE_REQUEST structures, and
    allocates resource for the physical device object specified in
    the structure.   The allocated resources are automatically recorded
    in the registry.

Arguments:

    DeviceCount - Supplies the number of device objects whom we need to
                  allocate resource to.  That is the number of entries
                  in the RequestTable.

    RequestTable - Supplies an array of IOP_RESOURCE_REQUEST structures which
                   contains the Physical device object to allocate resource to.
                   Upon entry, the ResourceAssignment pointer is NULL and on
                   return the allocated resource is returned via the this pointer.

    BootConfigsOK - Allow assignment of BOOT configs.

Return Value:

    The status returned is the final completion status of the operation.

    NOTE:
    If NTSTATUS_SUCCESS is returned, the resource allocation for *all* the devices
    specified is succeeded.  Otherwise, one or more are failed and caller must
    examine the ResourceAssignment pointer in each IOP_RESOURCE_REQUEST structure to
    determine which devices failed and which succeeded.

--*/
{
    NTSTATUS status;
    ULONG i;

    PAGED_CODE();

    ASSERT(DeviceCount != 0);

    for (i = 0; i < DeviceCount; i++) {

        //
        // Initialize table entry.
        //

        RequestTable[i].ResourceAssignment = NULL;
        RequestTable[i].Status = 0;
        RequestTable[i].Flags = 0;
        RequestTable[i].AllocationType = ArbiterRequestPnpEnumerated;
        if (((PDEVICE_NODE)(RequestTable[i].PhysicalDevice->DeviceObjectExtension->DeviceNode))->Flags & DNF_MADEUP) {

            ULONG           reportedDevice = 0;
            HANDLE          hInstance;

            status = IopDeviceObjectToDeviceInstance(RequestTable[i].PhysicalDevice, &hInstance, KEY_READ);
            if (NT_SUCCESS(status)) {

                ULONG           resultSize = 0;
                UCHAR           buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
                UNICODE_STRING  unicodeString;

                PiWstrToUnicodeString(&unicodeString, REGSTR_VALUE_DEVICE_REPORTED);
                status = ZwQueryValueKey(   hInstance,
                                            &unicodeString,
                                            KeyValuePartialInformation,
                                            (PVOID)buffer,
                                            sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG),
                                            &resultSize);
                if (NT_SUCCESS(status)) {

                    reportedDevice = *(PULONG)(((PKEY_VALUE_PARTIAL_INFORMATION)buffer)->Data);

                }

                ZwClose(hInstance);
            }

            //
            // Change the AllocationType for reported devices.
            //

            if (reportedDevice) {

                RequestTable[i].AllocationType = ArbiterRequestLegacyReported;

            }

        }
        RequestTable[i].ResourceRequirements = NULL;
    }

    //
    // Allocate memory to build a IOP_ASSIGN table to call IopAllocateResources()
    //

    status = IopAllocateResources(&DeviceCount, &RequestTable, FALSE, BootConfigsOK);
    return status;
}

NTSTATUS
IopWriteAllocatedResourcesToRegistry (
    PDEVICE_NODE DeviceNode,
    PCM_RESOURCE_LIST CmResourceList,
    ULONG Length
    )

/*++

Routine Description:

    This routine writes allocated resources for a device to its control key of device
    instance path key.

Arguments:

    DeviceNode - Supplies a pointer to the device node structure of the device.

    CmResourceList - Supplies a pointer to the device's allocated CM resource list.

    Length - Supplies the length of the CmResourceList.

Return Value:

    The status returned is the final completion status of the operation.

--*/
{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject = DeviceNode->PhysicalDeviceObject;
    HANDLE handle, handlex;
    UNICODE_STRING unicodeName;

    KeEnterCriticalRegion();
    ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

    status = IopDeviceObjectToDeviceInstance(
                                    deviceObject,
                                    &handlex,
                                    KEY_ALL_ACCESS);
    if (NT_SUCCESS(status)) {

        //
        // Open the LogConfig key of the device instance.
        //

        PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
        status = IopCreateRegistryKeyEx( &handle,
                                         handlex,
                                         &unicodeName,
                                         KEY_ALL_ACCESS,
                                         REG_OPTION_VOLATILE,
                                         NULL
                                         );
        ZwClose(handlex);
        if (NT_SUCCESS(status)) {

            RtlInitUnicodeString(&unicodeName, REGSTR_VALUE_ALLOC_CONFIG);
            if (CmResourceList) {
                status = ZwSetValueKey(
                              handle,
                              &unicodeName,
                              TITLE_INDEX_VALUE,
                              REG_RESOURCE_LIST,
                              CmResourceList,
                              Length
                              );
            } else {
                status = ZwDeleteValueKey(handle, &unicodeName);
            }
            ZwClose(handle);
        }
    }
    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();
    return status;
}

BOOLEAN
IopIsFirmwareDisabled (
    IN PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This routine determines if the devicenode has been disabled by firmware.

Arguments:

    DeviceNode - Supplies a pointer to the device node structure of the device.

Return Value:

    TRUE if disabled, otherwise FALSE

--*/
{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject = DeviceNode->PhysicalDeviceObject;
    HANDLE handle, handlex;
    UNICODE_STRING unicodeName;
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION value = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
    ULONG buflen;
    BOOLEAN FirmwareDisabled = FALSE;

    KeEnterCriticalRegion();
    ExAcquireResourceShared(&PpRegistryDeviceResource, TRUE);

    status = IopDeviceObjectToDeviceInstance(
                                    deviceObject,
                                    &handlex,
                                    KEY_ALL_ACCESS);
    if (NT_SUCCESS(status)) {

        //
        // Open the LogConfig key of the device instance.
        //

        PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
        status = IopCreateRegistryKeyEx( &handle,
                                         handlex,
                                         &unicodeName,
                                         KEY_ALL_ACCESS,
                                         REG_OPTION_VOLATILE,
                                         NULL
                                         );
        ZwClose(handlex);
        if (NT_SUCCESS(status)) {

            RtlInitUnicodeString(&unicodeName, REGSTR_VAL_FIRMWAREDISABLED);
            value = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
            buflen = sizeof(buffer);
            status = ZwQueryValueKey(handle,
                                     &unicodeName,
                                     KeyValuePartialInformation,
                                     value,
                                     sizeof(buffer),
                                     &buflen
                                     );

            ZwClose(handle);

            //
            // We don't need to check the buffer was big enough because it starts
            // off that way and doesn't get any smaller!
            //

            if (NT_SUCCESS(status)
                && value->Type == REG_DWORD
                && value->DataLength == sizeof(ULONG)
                && (*(PULONG)(value->Data))!=0) {

                //
                // firmware disabled
                //
                FirmwareDisabled = TRUE;
            }
        }
    }
    ExReleaseResource(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();
    return FirmwareDisabled;
}

