// Copyright (c) 2004, Antony C. Roberts

// Use of this file is subject to the terms
// described in the LICENSE.TXT file that
// accompanies this file.
//
// Your use of this file indicates your
// acceptance of the terms described in
// LICENSE.TXT.
//
// http://www.freebt.net

#include "fbtusb.h"
#include "fbtpwr.h"
#include "fbtpnp.h"
#include "fbtdev.h"
#include "fbtrwr.h"
#include "fbtwmi.h"

#include "fbtusr.h"

#define MOFRESOURCENAME L"MofResourceName"

#define WMI_FREEBT_DRIVER_INFORMATION 0

DEFINE_GUID (FREEBT_WMI_STD_DATA_GUID, 0x871B1A60, 0xD3EA, 0x4f2f, 0x81, 0x7b, 0x46, 0x5e, 0x44, 0x86, 0x7b, 0xf5);

WMIGUIDREGINFO FreeBTWmiGuidList[1] =
{
	{
		&FREEBT_WMI_STD_DATA_GUID, 1, 0 // driver information

    }

};

NTSTATUS NTAPI FreeBT_WmiRegistration(IN OUT PDEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS ntStatus;

    PAGED_CODE();

    DeviceExtension->WmiLibInfo.GuidCount = sizeof (FreeBTWmiGuidList) / sizeof (WMIGUIDREGINFO);
    DeviceExtension->WmiLibInfo.GuidList           = FreeBTWmiGuidList;
    DeviceExtension->WmiLibInfo.QueryWmiRegInfo    = FreeBT_QueryWmiRegInfo;
    DeviceExtension->WmiLibInfo.QueryWmiDataBlock  = FreeBT_QueryWmiDataBlock;
    DeviceExtension->WmiLibInfo.SetWmiDataBlock    = FreeBT_SetWmiDataBlock;
    DeviceExtension->WmiLibInfo.SetWmiDataItem     = FreeBT_SetWmiDataItem;
    DeviceExtension->WmiLibInfo.ExecuteWmiMethod   = NULL;
    DeviceExtension->WmiLibInfo.WmiFunctionControl = NULL;

    // Register with WMI
    ntStatus = IoWMIRegistrationControl(DeviceExtension->FunctionalDeviceObject, WMIREG_ACTION_REGISTER);

    return ntStatus;

}

NTSTATUS NTAPI FreeBT_WmiDeRegistration(IN OUT PDEVICE_EXTENSION DeviceExtension)
{
    PAGED_CODE();
    return IoWMIRegistrationControl(DeviceExtension->FunctionalDeviceObject, WMIREG_ACTION_DEREGISTER);

}

NTSTATUS NTAPI FreeBT_DispatchSysCtrl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PDEVICE_EXTENSION       deviceExtension;
    SYSCTL_IRP_DISPOSITION  disposition;
    NTSTATUS                ntStatus;
    PIO_STACK_LOCATION      irpStack;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation (Irp);
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

	FreeBT_DbgPrint(3, ("FBTUSB: "));
    FreeBT_DbgPrint(3, (WMIMinorFunctionString(irpStack->MinorFunction)));
    if (Removed == deviceExtension->DeviceState)
	{
        ntStatus = STATUS_DELETE_PENDING;

        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return ntStatus;

    }

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchSysCtrl::"));
    FreeBT_IoIncrement(deviceExtension);

    ntStatus = WmiSystemControl(&deviceExtension->WmiLibInfo,
                                DeviceObject,
                                Irp,
                                &disposition);

    switch(disposition)
	{
        case IrpProcessed:
        {
            // This irp has been processed and may be completed or pending.
            break;

        }

        case IrpNotCompleted:
        {
            // This irp has not been completed, but has been fully processed.
            // we will complete it now
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;

        }

        case IrpForward:
        case IrpNotWmi:
        {
            // This irp is either not a WMI irp or is a WMI irp targeted
            // at a device lower in the stack.
            IoSkipCurrentIrpStackLocation (Irp);
            ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
            break;
        }

        default:
        {
            // We really should never get here, but if we do just forward....
            ASSERT(FALSE);
            IoSkipCurrentIrpStackLocation (Irp);
            ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
            break;

        }

    }

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchSysCtrl::"));
    FreeBT_IoDecrement(deviceExtension);

    return ntStatus;

}

NTSTATUS NTAPI FreeBT_QueryWmiRegInfo(
    IN  PDEVICE_OBJECT  DeviceObject,
    OUT ULONG           *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT  *Pdo
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve the list of
    guids or data blocks that the driver wants to register with WMI. This
    routine may not pend or block. Driver should NOT call
    WmiCompleteRequest.

Arguments:

    DeviceObject is the device whose data block is being queried

    *RegFlags returns with a set of flags that describe the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver

    *MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned as NULL.

    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is returned in
        *RegFlags.

Return Value:

    status

--*/
{
    PDEVICE_EXTENSION deviceExtension;

    PAGED_CODE();

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_QueryWmiRegInfo: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    *RegFlags     = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &Globals.FreeBT_RegistryPath;
    *Pdo          = deviceExtension->PhysicalDeviceObject;
    RtlInitUnicodeString(MofResourceName, MOFRESOURCENAME);

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_QueryWmiRegInfo: Leaving\n"));

    return STATUS_SUCCESS;

}

NTSTATUS NTAPI FreeBT_QueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          InstanceCount,
    IN OUT PULONG     InstanceLengthArray,
    IN ULONG          OutBufferSize,
    OUT PUCHAR        Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call WmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.

    InstanceCount is the number of instances expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fulfill the request
        so the irp should be completed with the buffer needed.

    OutBufferSize has the maximum size available to write the data
        block.

    Buffer on return is filled with the returned data block


Return Value:

    status

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS          ntStatus;
    ULONG             size;
    WCHAR             modelName[] = L"Aishverya\0\0";
    USHORT            modelNameLen;

    PAGED_CODE();

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_QueryWmiDataBlock: Entered\n"));

    size = 0;
    modelNameLen = (wcslen(modelName) + 1) * sizeof(WCHAR);

    // Only ever registers 1 instance per guid
    ASSERT((InstanceIndex == 0) && (InstanceCount == 1));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    switch (GuidIndex)
	{
    case WMI_FREEBT_DRIVER_INFORMATION:
        size = sizeof(ULONG) + modelNameLen + sizeof(USHORT);
        if (OutBufferSize < size )
		{
            FreeBT_DbgPrint(3, ("FBTUSB: OutBuffer too small\n"));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;

        }

        * (PULONG) Buffer = DebugLevel;
        Buffer += sizeof(ULONG);

        // put length of string ahead of string
        *((PUSHORT)Buffer) = modelNameLen;
        Buffer = (PUCHAR)Buffer + sizeof(USHORT);
        RtlCopyBytes((PVOID)Buffer, (PVOID)modelName, modelNameLen);
        *InstanceLengthArray = size ;

        ntStatus = STATUS_SUCCESS;
        break;

    default:
        ntStatus = STATUS_WMI_GUID_NOT_FOUND;

    }

    ntStatus = WmiCompleteRequest(DeviceObject,
                                Irp,
                                ntStatus,
                                size,
                                IO_NO_INCREMENT);

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_QueryWmiDataBlock: Leaving\n"));

    return ntStatus;

}


NTSTATUS NTAPI FreeBT_SetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          DataItemId,
    IN ULONG          BufferSize,
    IN PUCHAR         Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set for the contents of
    a data block. When the driver has finished filling the data block it
    must call WmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.

    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    Buffer has the new values for the data item


Return Value:

    status

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS          ntStatus;
    ULONG             info;

    PAGED_CODE();

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_SetWmiDataItem: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    info = 0;

    switch(GuidIndex)
	{
    case WMI_FREEBT_DRIVER_INFORMATION:
        if(DataItemId == 1)
		{
            if(BufferSize == sizeof(ULONG))
			{
                DebugLevel = *((PULONG)Buffer);
                ntStatus = STATUS_SUCCESS;
                info = sizeof(ULONG);

            }

            else
			{
                ntStatus = STATUS_INFO_LENGTH_MISMATCH;

            }

        }

        else
		{
            ntStatus = STATUS_WMI_READ_ONLY;

        }

        break;

    default:
        ntStatus = STATUS_WMI_GUID_NOT_FOUND;

    }

    ntStatus = WmiCompleteRequest(DeviceObject,
                                Irp,
                                ntStatus,
                                info,
                                IO_NO_INCREMENT);

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_SetWmiDataItem: Leaving\n"));

    return ntStatus;

}

NTSTATUS NTAPI FreeBT_SetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          BufferSize,
    IN PUCHAR         Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set the contents of
    a data block. When the driver has finished filling the data block it
    must call WmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.

    BufferSize has the size of the data block passed

    Buffer has the new values for the data block

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS          ntStatus;
    ULONG             info;

    PAGED_CODE();

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_SetWmiDataBlock: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    info = 0;

    switch(GuidIndex)
	{
    case WMI_FREEBT_DRIVER_INFORMATION:
        if(BufferSize == sizeof(ULONG))
		{
            DebugLevel = *(PULONG) Buffer;
            ntStatus = STATUS_SUCCESS;
            info = sizeof(ULONG);

        }

        else
		{
            ntStatus = STATUS_INFO_LENGTH_MISMATCH;

        }

        break;

    default:
        ntStatus = STATUS_WMI_GUID_NOT_FOUND;

    }

    ntStatus = WmiCompleteRequest(DeviceObject,
                                Irp,
                                ntStatus,
                                info,
                                IO_NO_INCREMENT);

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_SetWmiDataBlock: Leaving\n"));

    return ntStatus;

}

PCHAR NTAPI WMIMinorFunctionString(UCHAR MinorFunction)
{
    switch (MinorFunction)
	{
        case IRP_MN_CHANGE_SINGLE_INSTANCE:
            return "IRP_MN_CHANGE_SINGLE_INSTANCE\n";

        case IRP_MN_CHANGE_SINGLE_ITEM:
            return "IRP_MN_CHANGE_SINGLE_ITEM\n";

        case IRP_MN_DISABLE_COLLECTION:
            return "IRP_MN_DISABLE_COLLECTION\n";

        case IRP_MN_DISABLE_EVENTS:
            return "IRP_MN_DISABLE_EVENTS\n";

        case IRP_MN_ENABLE_COLLECTION:
            return "IRP_MN_ENABLE_COLLECTION\n";

        case IRP_MN_ENABLE_EVENTS:
            return "IRP_MN_ENABLE_EVENTS\n";

        case IRP_MN_EXECUTE_METHOD:
            return "IRP_MN_EXECUTE_METHOD\n";

        case IRP_MN_QUERY_ALL_DATA:
            return "IRP_MN_QUERY_ALL_DATA\n";

        case IRP_MN_QUERY_SINGLE_INSTANCE:
            return "IRP_MN_QUERY_SINGLE_INSTANCE\n";

        case IRP_MN_REGINFO:
            return "IRP_MN_REGINFO\n";

        default:
            return "IRP_MN_?????\n";

    }

}
