/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    classwmi.c

Abstract:

    SCSI class driver routines

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "classp.h"

#include <wmistr.h>

NTSTATUS
NTAPI
ClassSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
ClassFindGuid(
    PGUIDREGINFO GuidList,
    ULONG GuidCount,
    LPGUID Guid,
    PULONG GuidIndex
    );

//
// This is the name for the MOF resource that must be part of all drivers that
// register via this interface.
#define MOFRESOURCENAME L"MofResourceName"

//
// What can be paged ???
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ClassSystemControl)
#pragma alloc_text(PAGE, ClassFindGuid)
#endif


/*++////////////////////////////////////////////////////////////////////////////

ClassFindGuid()

Routine Description:

    This routine will search the list of guids registered and return
    the index for the one that was registered.

Arguments:

    GuidList is the list of guids to search

    GuidCount is the count of guids in the list

    Guid is the guid being searched for

    *GuidIndex returns the index to the guid

Return Value:

    TRUE if guid is found else FALSE

--*/
BOOLEAN
ClassFindGuid(
    PGUIDREGINFO GuidList,
    ULONG GuidCount,
    LPGUID Guid,
    PULONG GuidIndex
    )
{
    ULONG i;

    PAGED_CODE();

    for (i = 0; i < GuidCount; i++)
    {
        if (IsEqualGUID(Guid, &GuidList[i].Guid))
        {
            *GuidIndex = i;
            return(TRUE);
        }
    }

    return(FALSE);
} // end ClassFindGuid()

/*++////////////////////////////////////////////////////////////////////////////

ClassSystemControl()

Routine Description:

    Dispatch routine for IRP_MJ_SYSTEM_CONTROL. This routine will process
    all wmi requests received, forwarding them if they are not for this
    driver or determining if the guid is valid and if so passing it to
    the driver specific function for handing wmi requests.

Arguments:

    DeviceObject - Supplies a pointer to the device object for this request.

    Irp - Supplies the Irp making the request.

Return Value:

    status

--*/
NTSTATUS
NTAPI
ClassSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PCLASS_DRIVER_EXTENSION driverExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG isRemoved;
    ULONG bufferSize;
    PUCHAR buffer;
    NTSTATUS status;
    UCHAR minorFunction;
    ULONG guidIndex;
    PCLASS_WMI_INFO classWmiInfo;

    PAGED_CODE();

    //
    // Make sure device has not been removed
    isRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);
    if(isRemoved)
    {
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        ClassReleaseRemoveLock(DeviceObject, Irp);
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // If the irp is not a WMI irp or it is not targeted at this device
    // or this device has not registered with WMI then just forward it on.
    minorFunction = irpStack->MinorFunction;
    if ((minorFunction > IRP_MN_EXECUTE_METHOD) ||
        (irpStack->Parameters.WMI.ProviderId != (ULONG_PTR)DeviceObject) ||
        ((minorFunction != IRP_MN_REGINFO) &&
         (commonExtension->GuidRegInfo == NULL)))
    {
        //
        // CONSIDER: Do I need to hang onto lock until IoCallDriver returns ?
        IoSkipCurrentIrpStackLocation(Irp);
        ClassReleaseRemoveLock(DeviceObject, Irp);
        return(IoCallDriver(commonExtension->LowerDeviceObject, Irp));
    }

    buffer = (PUCHAR)irpStack->Parameters.WMI.Buffer;
    bufferSize = irpStack->Parameters.WMI.BufferSize;

    if (minorFunction != IRP_MN_REGINFO)
    {
        //
        // For all requests other than query registration info we are passed
        // a guid. Determine if the guid is one that is supported by the
        // device.
        if (ClassFindGuid(commonExtension->GuidRegInfo,
                            commonExtension->GuidCount,
                            (LPGUID)irpStack->Parameters.WMI.DataPath,
                            &guidIndex))
        {
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_WMI_GUID_NOT_FOUND;
        }

        if (NT_SUCCESS(status) &&
            ((minorFunction == IRP_MN_QUERY_SINGLE_INSTANCE) ||
             (minorFunction == IRP_MN_CHANGE_SINGLE_INSTANCE) ||
             (minorFunction == IRP_MN_CHANGE_SINGLE_ITEM) ||
             (minorFunction == IRP_MN_EXECUTE_METHOD)))
        {
            if ( (((PWNODE_HEADER)buffer)->Flags) &
                                          WNODE_FLAG_STATIC_INSTANCE_NAMES)
            {
                if ( ((PWNODE_SINGLE_INSTANCE)buffer)->InstanceIndex != 0 )
                {
                    status = STATUS_WMI_INSTANCE_NOT_FOUND;
                }
            } else {
                status = STATUS_WMI_INSTANCE_NOT_FOUND;
            }
        }

        if (! NT_SUCCESS(status))
        {
            Irp->IoStatus.Status = status;
            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            return(status);
        }
    }

    driverExtension = commonExtension->DriverExtension;

    classWmiInfo = commonExtension->IsFdo ?
                           &driverExtension->InitData.FdoData.ClassWmiInfo :
                           &driverExtension->InitData.PdoData.ClassWmiInfo;
    switch(minorFunction)
    {
        case IRP_MN_REGINFO:
        {
            ULONG guidCount;
            PGUIDREGINFO guidList;
            PWMIREGINFOW wmiRegInfo;
            PWMIREGGUIDW wmiRegGuid;
            //PDEVICE_OBJECT pdo;
            PUNICODE_STRING regPath;
            PWCHAR stringPtr;
            ULONG retSize;
            ULONG registryPathOffset;
            ULONG mofResourceOffset;
            ULONG bufferNeeded;
            ULONG i;
            ULONG_PTR nameInfo;
            ULONG nameSize, nameOffset, nameFlags;
            UNICODE_STRING name, mofName;
            PCLASS_QUERY_WMI_REGINFO_EX ClassQueryWmiRegInfoEx;

            name.Buffer = NULL;
            name.Length = 0;
            name.MaximumLength = 0;
            nameFlags = 0;

            ClassQueryWmiRegInfoEx = commonExtension->IsFdo ?
                               driverExtension->ClassFdoQueryWmiRegInfoEx :
                               driverExtension->ClassPdoQueryWmiRegInfoEx;
        
            if (ClassQueryWmiRegInfoEx == NULL)
            {
                status = classWmiInfo->ClassQueryWmiRegInfo(
                                                        DeviceObject,
                                                        &nameFlags,
                                                        &name);
                
                RtlInitUnicodeString(&mofName, MOFRESOURCENAME);
            } else {
                RtlInitUnicodeString(&mofName, L"");
                status = (*ClassQueryWmiRegInfoEx)(
                                                    DeviceObject,
                                                    &nameFlags,
                                                    &name,
                                                    &mofName);
            }

            if (NT_SUCCESS(status) &&
                (! (nameFlags &  WMIREG_FLAG_INSTANCE_PDO) &&
                (name.Buffer == NULL)))
            {
                //
                // if PDO flag not specified then an instance name must be
                status = STATUS_INVALID_DEVICE_REQUEST;
            }

            if (NT_SUCCESS(status))
            {
                guidList = classWmiInfo->GuidRegInfo;
                guidCount = classWmiInfo->GuidCount;

                nameOffset = sizeof(WMIREGINFO) +
                                      guidCount * sizeof(WMIREGGUIDW);

                if (nameFlags & WMIREG_FLAG_INSTANCE_PDO)
                {
                    nameSize = 0;
                    nameInfo = commonExtension->IsFdo ?
                                   (ULONG_PTR)((PFUNCTIONAL_DEVICE_EXTENSION)commonExtension)->LowerPdo :
                                   (ULONG_PTR)DeviceObject;
                } else {
                    nameFlags |= WMIREG_FLAG_INSTANCE_LIST;
                    nameSize = name.Length + sizeof(USHORT);
                    nameInfo = nameOffset;
                }

                mofResourceOffset = nameOffset + nameSize;

                registryPathOffset = mofResourceOffset +
                                  mofName.Length + sizeof(USHORT);

                regPath = &driverExtension->RegistryPath;
                bufferNeeded = registryPathOffset +
                regPath->Length + sizeof(USHORT);

                if (bufferNeeded <= bufferSize)
                {
                    retSize = bufferNeeded;

                    commonExtension->GuidCount = guidCount;
                    commonExtension->GuidRegInfo = guidList;

                    wmiRegInfo = (PWMIREGINFO)buffer;
                    wmiRegInfo->BufferSize = bufferNeeded;
                    wmiRegInfo->NextWmiRegInfo = 0;
                    wmiRegInfo->MofResourceName = mofResourceOffset;
                    wmiRegInfo->RegistryPath = registryPathOffset;
                    wmiRegInfo->GuidCount = guidCount;

                    for (i = 0; i < guidCount; i++)
                    {
                        wmiRegGuid = &wmiRegInfo->WmiRegGuid[i];
                        wmiRegGuid->Guid = guidList[i].Guid;
                        wmiRegGuid->Flags = guidList[i].Flags | nameFlags;
                        wmiRegGuid->InstanceInfo = nameInfo;
                        wmiRegGuid->InstanceCount = 1;
                    }

                    if ( nameFlags &  WMIREG_FLAG_INSTANCE_LIST)
                    {
                        stringPtr = (PWCHAR)((PUCHAR)buffer + nameOffset);
                        *stringPtr++ = name.Length;
                        RtlCopyMemory(stringPtr,
                                  name.Buffer,
                                  name.Length);
                    }

                    stringPtr = (PWCHAR)((PUCHAR)buffer + mofResourceOffset);
                    *stringPtr++ = mofName.Length;
                    RtlCopyMemory(stringPtr,
                                  mofName.Buffer,
                                  mofName.Length);

                    stringPtr = (PWCHAR)((PUCHAR)buffer + registryPathOffset);
                    *stringPtr++ = regPath->Length;
                    RtlCopyMemory(stringPtr,
                              regPath->Buffer,
                              regPath->Length);
                } else {
                    *((PULONG)buffer) = bufferNeeded;
                    retSize = sizeof(ULONG);
                }
            } else {
                retSize = 0;
            }

            if (name.Buffer != NULL)
            {
                ExFreePool(name.Buffer);
            }

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = retSize;
            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            return(status);
        }

        case IRP_MN_QUERY_ALL_DATA:
        {
            PWNODE_ALL_DATA wnode;
            ULONG bufferAvail;

            wnode = (PWNODE_ALL_DATA)buffer;

            if (bufferSize < sizeof(WNODE_ALL_DATA))
            {
                bufferAvail = 0;
            } else {
                bufferAvail = bufferSize - sizeof(WNODE_ALL_DATA);
            }

            wnode->DataBlockOffset = sizeof(WNODE_ALL_DATA);

            status = classWmiInfo->ClassQueryWmiDataBlock(
                                             DeviceObject,
                                             Irp,
                                             guidIndex,
                                             bufferAvail,
                                             buffer + sizeof(WNODE_ALL_DATA));

            break;
        }

        case IRP_MN_QUERY_SINGLE_INSTANCE:
        {
            PWNODE_SINGLE_INSTANCE wnode;
            ULONG dataBlockOffset;

            wnode = (PWNODE_SINGLE_INSTANCE)buffer;

            dataBlockOffset = wnode->DataBlockOffset;

            status = classWmiInfo->ClassQueryWmiDataBlock(
                                          DeviceObject,
                                          Irp,
                                          guidIndex,
                                          bufferSize - dataBlockOffset,
                                          (PUCHAR)wnode + dataBlockOffset);

            break;
        }

        case IRP_MN_CHANGE_SINGLE_INSTANCE:
        {
            PWNODE_SINGLE_INSTANCE wnode;

            wnode = (PWNODE_SINGLE_INSTANCE)buffer;

            status = classWmiInfo->ClassSetWmiDataBlock(
                                     DeviceObject,
                                     Irp,
                                     guidIndex,
                                     wnode->SizeDataBlock,
                                     (PUCHAR)wnode + wnode->DataBlockOffset);

            break;
        }

        case IRP_MN_CHANGE_SINGLE_ITEM:
        {
            PWNODE_SINGLE_ITEM wnode;

            wnode = (PWNODE_SINGLE_ITEM)buffer;

            status = classWmiInfo->ClassSetWmiDataItem(
                                     DeviceObject,
                                     Irp,
                                     guidIndex,
                                     wnode->ItemId,
                                     wnode->SizeDataItem,
                                     (PUCHAR)wnode + wnode->DataBlockOffset);

            break;
        }

        case IRP_MN_EXECUTE_METHOD:
        {
            PWNODE_METHOD_ITEM wnode;

            wnode = (PWNODE_METHOD_ITEM)buffer;

            status = classWmiInfo->ClassExecuteWmiMethod(
                                         DeviceObject,
                                         Irp,
                                         guidIndex,
                                         wnode->MethodId,
                                         wnode->SizeDataBlock,
                                         bufferSize - wnode->DataBlockOffset,
                                         buffer + wnode->DataBlockOffset);


            break;
        }

        case IRP_MN_ENABLE_EVENTS:
        {
            status = classWmiInfo->ClassWmiFunctionControl(
                                                           DeviceObject,
                                                           Irp,
                                                           guidIndex,
                                                           EventGeneration,
                                                           TRUE);
            break;
        }

        case IRP_MN_DISABLE_EVENTS:
        {
            status = classWmiInfo->ClassWmiFunctionControl(
                                                           DeviceObject,
                                                           Irp,
                                                           guidIndex,
                                                           EventGeneration,
                                                           FALSE);
            break;
        }

        case IRP_MN_ENABLE_COLLECTION:
        {
            status = classWmiInfo->ClassWmiFunctionControl(
                                                         DeviceObject,
                                                         Irp,
                                                         guidIndex,
                                                         DataBlockCollection,
                                                         TRUE);
            break;
        }

        case IRP_MN_DISABLE_COLLECTION:
        {
            status = classWmiInfo->ClassWmiFunctionControl(
                                                         DeviceObject,
                                                         Irp,
                                                         guidIndex,
                                                         DataBlockCollection,
                                                         FALSE);
            break;
        }

        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

    }

    return(status);
} // end ClassSystemControl()

/*++////////////////////////////////////////////////////////////////////////////

ClassWmiCompleteRequest()

Routine Description:


    This routine will do the work of completing a WMI irp. Depending upon the
    the WMI request this routine will fixup the returned WNODE appropriately.

    NOTE: This routine assumes that the ClassRemoveLock is held and it will
          release it.

Arguments:

    DeviceObject - Supplies a pointer to the device object for this request.

    Irp - Supplies the Irp making the request.
    
    Status - Status to complete the irp with.  STATUS_BUFFER_TOO_SMALL is used
        to indicate that more buffer is required for the data requested.
    
    BufferUsed - number of bytes of actual data to return (not including WMI
        specific structures)
    
    PriorityBoost - priority boost to pass to ClassCompleteRequest

Return Value:

    status

--*/
SCSIPORTAPI
NTSTATUS
NTAPI
ClassWmiCompleteRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG BufferUsed,
    IN CCHAR PriorityBoost
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    //UCHAR MinorFunction;
    PUCHAR buffer;
    ULONG retSize;
    UCHAR minorFunction;

    minorFunction = irpStack->MinorFunction;
    buffer = (PUCHAR)irpStack->Parameters.WMI.Buffer;

    switch(minorFunction)
    {
        case IRP_MN_QUERY_ALL_DATA:
        {
            PWNODE_ALL_DATA wnode;
            PWNODE_TOO_SMALL wnodeTooSmall;
            ULONG bufferNeeded;

            wnode = (PWNODE_ALL_DATA)buffer;

            bufferNeeded = sizeof(WNODE_ALL_DATA) + BufferUsed;

            if (NT_SUCCESS(Status))
            {
                retSize = bufferNeeded;
                wnode->WnodeHeader.BufferSize = bufferNeeded;
                KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);
                wnode->WnodeHeader.Flags |= WNODE_FLAG_FIXED_INSTANCE_SIZE;
                wnode->FixedInstanceSize = BufferUsed;
                wnode->InstanceCount = 1;

            } else if (Status == STATUS_BUFFER_TOO_SMALL) {
                wnodeTooSmall = (PWNODE_TOO_SMALL)wnode;

                wnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
                wnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
                wnodeTooSmall->SizeNeeded = sizeof(WNODE_ALL_DATA) + BufferUsed;
                retSize = sizeof(WNODE_TOO_SMALL);
                Status = STATUS_SUCCESS;
            } else {
                retSize = 0;
            }
            break;
        }

        case IRP_MN_QUERY_SINGLE_INSTANCE:
        {
            PWNODE_SINGLE_INSTANCE wnode;
            PWNODE_TOO_SMALL wnodeTooSmall;
            ULONG bufferNeeded;

            wnode = (PWNODE_SINGLE_INSTANCE)buffer;

            bufferNeeded = wnode->DataBlockOffset + BufferUsed;

            if (NT_SUCCESS(Status))
            {
                retSize = bufferNeeded;
                wnode->WnodeHeader.BufferSize = bufferNeeded;
                KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);
                wnode->SizeDataBlock = BufferUsed;

            } else if (Status == STATUS_BUFFER_TOO_SMALL) {
                wnodeTooSmall = (PWNODE_TOO_SMALL)wnode;

                wnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
                wnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
                wnodeTooSmall->SizeNeeded = bufferNeeded;
                retSize = sizeof(WNODE_TOO_SMALL);
                Status = STATUS_SUCCESS;
            } else {
                retSize = 0;
            }
            break;
        }

        case IRP_MN_EXECUTE_METHOD:
        {
            PWNODE_METHOD_ITEM wnode;
            PWNODE_TOO_SMALL wnodeTooSmall;
            ULONG bufferNeeded;

            wnode = (PWNODE_METHOD_ITEM)buffer;

            bufferNeeded = wnode->DataBlockOffset + BufferUsed;

            if (NT_SUCCESS(Status))
            {
                retSize = bufferNeeded;
                wnode->WnodeHeader.BufferSize = bufferNeeded;
                KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);
                wnode->SizeDataBlock = BufferUsed;

            } else if (Status == STATUS_BUFFER_TOO_SMALL) {
                wnodeTooSmall = (PWNODE_TOO_SMALL)wnode;

                wnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
                wnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
                wnodeTooSmall->SizeNeeded = bufferNeeded;
                retSize = sizeof(WNODE_TOO_SMALL);
                Status = STATUS_SUCCESS;
            } else {
                retSize = 0;
            }
            break;
        }

        default:
        {
            //
            // All other requests don't return any data
            retSize = 0;
            break;
        }

    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = retSize;
    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, PriorityBoost);
    return(Status);
} // end ClassWmiCompleteRequest()

/*++////////////////////////////////////////////////////////////////////////////

ClassWmiFireEvent()

Routine Description:

    This routine will fire a WMI event using the data buffer passed. This
    routine may be called at or below DPC level

Arguments:

    DeviceObject - Supplies a pointer to the device object for this event

    Guid is pointer to the GUID that represents the event

    InstanceIndex is the index of the instance of the event

    EventDataSize is the number of bytes of data that is being fired with
       with the event

    EventData is the data that is fired with the events. This may be NULL
        if there is no data associated with the event


Return Value:

    status

--*/
NTSTATUS
NTAPI
ClassWmiFireEvent(
    IN PDEVICE_OBJECT DeviceObject,
    IN LPGUID Guid,
    IN ULONG InstanceIndex,
    IN ULONG EventDataSize,
    IN PVOID EventData
    )
{

    ULONG sizeNeeded;
    PWNODE_SINGLE_INSTANCE event;
    NTSTATUS status;

    if (EventData == NULL)
    {
        EventDataSize = 0;
    }

    sizeNeeded = sizeof(WNODE_SINGLE_INSTANCE) + EventDataSize;

    event = ExAllocatePoolWithTag(NonPagedPool, sizeNeeded, CLASS_TAG_WMI);
    if (event != NULL)
    {
        event->WnodeHeader.Guid = *Guid;
        event->WnodeHeader.ProviderId = IoWMIDeviceObjectToProviderId(DeviceObject);
        event->WnodeHeader.BufferSize = sizeNeeded;
        event->WnodeHeader.Flags =  WNODE_FLAG_SINGLE_INSTANCE |
                                    WNODE_FLAG_EVENT_ITEM |
                                    WNODE_FLAG_STATIC_INSTANCE_NAMES;
        KeQuerySystemTime(&event->WnodeHeader.TimeStamp);

        event->InstanceIndex = InstanceIndex;
        event->SizeDataBlock = EventDataSize;
        event->DataBlockOffset = sizeof(WNODE_SINGLE_INSTANCE);
        if (EventData != NULL)
        {
            RtlCopyMemory( &event->VariableData, EventData, EventDataSize);
        }

        status = IoWMIWriteEvent(event);
        if (! NT_SUCCESS(status))
        {
            ExFreePool(event);
        }
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return(status);
} // end ClassWmiFireEvent()

