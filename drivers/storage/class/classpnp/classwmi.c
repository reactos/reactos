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

#ifndef __REACTOS__
#include "stddef.h"
#include "ntddk.h"
#include "scsi.h"

#include "classpnp.h"

#include "mountdev.h"

#include <stdarg.h>
#endif

#include "classp.h"
#include <wmistr.h>
#include <wmidata.h>
// #include <classlog.h> __REACTOS__

#ifdef DEBUG_USE_WPP
#include "classwmi.tmh"
#endif

#define TIME_STRING_LENGTH      25

BOOLEAN
ClassFindGuid(
    PGUIDREGINFO GuidList,
    ULONG GuidCount,
    LPGUID Guid,
    PULONG GuidIndex
    );

NTSTATUS
ClassQueryInternalDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

PWCHAR
ConvertTickToDateTime(
    IN LARGE_INTEGER Tick,
    _Out_writes_(TIME_STRING_LENGTH) PWCHAR String
    );

BOOLEAN
ClassFindInternalGuid(
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
#pragma alloc_text(PAGE, ClassFindInternalGuid)
#endif

#ifdef __REACTOS__
#define MSStorageDriver_ClassErrorLogGuid {0xD5A9A51E, 0x03F9, 0x404d, {0x97, 0x22, 0x15, 0xF9, 0x0E, 0xB0, 0x70, 0x38}}
#endif

//
// Define WMI interface to all class drivers
//
GUIDREGINFO wmiClassGuids[] =
{
    {
        MSStorageDriver_ClassErrorLogGuid, 1, 0
    }
};

#define MSStorageDriver_ClassErrorLogGuid_Index     0
#define NUM_CLASS_WMI_GUIDS     (sizeof(wmiClassGuids) / sizeof(GUIDREGINFO))


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

ClassFindInternalGuid()

Routine Description:

    This routine will search the list of internal guids registered and return
    the index for the one that was registered.

Arguments:

    Guid is the guid being searched for

    *GuidIndex returns the index to the guid

Return Value:

    TRUE if guid is found else FALSE

--*/
BOOLEAN
ClassFindInternalGuid(
    LPGUID Guid,
    PULONG GuidIndex
    )
{
    ULONG i;

    PAGED_CODE();

    for (i = 0; i < NUM_CLASS_WMI_GUIDS; i++)
    {
        if (IsEqualGUID(Guid, &wmiClassGuids[i].Guid))
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
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
    ULONG guidIndex = (ULONG)-1;
    PCLASS_WMI_INFO classWmiInfo;
    BOOLEAN isInternalGuid = FALSE;

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
    // If the irp is not a WMI irp or it is not targetted at this device
    // or this device has not regstered with WMI then just forward it on.
    minorFunction = irpStack->MinorFunction;
    if ((minorFunction > IRP_MN_EXECUTE_METHOD) ||
        (irpStack->Parameters.WMI.ProviderId != (ULONG_PTR)DeviceObject) ||
        ((minorFunction != IRP_MN_REGINFO) &&
         (commonExtension->GuidCount == 0)))
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
        if (commonExtension->GuidRegInfo != NULL &&
            ClassFindGuid(commonExtension->GuidRegInfo,
                            commonExtension->GuidCount,
                            (LPGUID)irpStack->Parameters.WMI.DataPath,
                            &guidIndex))
        {
            isInternalGuid = FALSE;
            status = STATUS_SUCCESS;
        } else if (ClassFindInternalGuid((LPGUID)irpStack->Parameters.WMI.DataPath,
                            &guidIndex)) {
            isInternalGuid = TRUE;
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_WMI_GUID_NOT_FOUND;
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_WMI, "WMI GUID not found!"));
        }

        TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_WMI, "WMI Find Guid = %x, isInternalGuid = %x", status, isInternalGuid));
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
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_WMI, "WMI Instance not found!"));
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

            if ((classWmiInfo->GuidRegInfo != NULL) &&
                (classWmiInfo->ClassQueryWmiRegInfo != NULL) &&
                (ClassQueryWmiRegInfoEx == NULL))
            {
                status = classWmiInfo->ClassQueryWmiRegInfo(
                                                        DeviceObject,
                                                        &nameFlags,
                                                        &name);

                RtlInitUnicodeString(&mofName, MOFRESOURCENAME);

            } else if ((classWmiInfo->GuidRegInfo != NULL) && (ClassQueryWmiRegInfoEx != NULL)) {
                RtlInitUnicodeString(&mofName, L"");

                status = (*ClassQueryWmiRegInfoEx)(
                                                    DeviceObject,
                                                    &nameFlags,
                                                    &name,
                                                    &mofName);
            } else {
                RtlInitUnicodeString(&mofName, L"");
                nameFlags = WMIREG_FLAG_INSTANCE_PDO;
                status = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(status) &&
                (! (nameFlags &  WMIREG_FLAG_INSTANCE_PDO) &&
                (name.Buffer == NULL)))
            {
                //
                // if PDO flag not specified then an instance name must be
                status = STATUS_INVALID_DEVICE_REQUEST;
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_WMI, "Invalid Device Request!"));
            }

            if (NT_SUCCESS(status))
            {
                guidList = classWmiInfo->GuidRegInfo;
                guidCount = (classWmiInfo->GuidRegInfo == NULL ? 0 : classWmiInfo->GuidCount) + NUM_CLASS_WMI_GUIDS;

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

                bufferNeeded = registryPathOffset + regPath->Length;
                bufferNeeded += sizeof(USHORT);

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

                    for (i = 0; i < classWmiInfo->GuidCount; i++)
                    {
                        wmiRegGuid = &wmiRegInfo->WmiRegGuid[i];
                        wmiRegGuid->Guid = guidList[i].Guid;
                        wmiRegGuid->Flags = guidList[i].Flags | nameFlags;
                        wmiRegGuid->InstanceInfo = nameInfo;
                        wmiRegGuid->InstanceCount = 1;
                    }
                    for (i = 0; i < NUM_CLASS_WMI_GUIDS; i++)
                    {
                        wmiRegGuid = &wmiRegInfo->WmiRegGuid[i + classWmiInfo->GuidCount];
                        wmiRegGuid->Guid = wmiClassGuids[i].Guid;
                        wmiRegGuid->Flags = wmiClassGuids[i].Flags | nameFlags;
                        wmiRegGuid->InstanceInfo = nameInfo;
                        wmiRegGuid->InstanceCount = 1;
                    }

                    if ( nameFlags &  WMIREG_FLAG_INSTANCE_LIST)
                    {
                        bufferNeeded = nameOffset + sizeof(WCHAR);
                        bufferNeeded += name.Length;

                        if (bufferSize >= bufferNeeded){
                            stringPtr = (PWCHAR)((PUCHAR)buffer + nameOffset);
                            *stringPtr++ = name.Length;
                            RtlCopyMemory(stringPtr, name.Buffer, name.Length);
                        }
                        else {
                            NT_ASSERT(bufferSize >= bufferNeeded);
                            status = STATUS_INVALID_BUFFER_SIZE;
                        }
                    }

                    bufferNeeded = mofResourceOffset + sizeof(WCHAR);
                    bufferNeeded += mofName.Length;

                    if (bufferSize >= bufferNeeded){
                        stringPtr = (PWCHAR)((PUCHAR)buffer + mofResourceOffset);
                        *stringPtr++ = mofName.Length;
                        RtlCopyMemory(stringPtr, mofName.Buffer, mofName.Length);
                    }
                    else {
                        NT_ASSERT(bufferSize >= bufferNeeded);
                        status = STATUS_INVALID_BUFFER_SIZE;
                    }

                    bufferNeeded = registryPathOffset + sizeof(WCHAR);
                    bufferNeeded += regPath->Length;

                    if (bufferSize >= bufferNeeded){
                        stringPtr = (PWCHAR)((PUCHAR)buffer + registryPathOffset);
                        *stringPtr++ = regPath->Length;
                        RtlCopyMemory(stringPtr,
                                  regPath->Buffer,
                                  regPath->Length);
                    }
                    else {

                        NT_ASSERT(bufferSize >= bufferNeeded);
                        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_WMI, "Invalid Buffer Size!"));
                        status = STATUS_INVALID_BUFFER_SIZE;
                    }

                } else {
                    *((PULONG)buffer) = bufferNeeded;
                    retSize = sizeof(ULONG);
                }
            } else {
                retSize = 0;
            }

            FREE_POOL(name.Buffer);

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

            NT_ASSERT(guidIndex != (ULONG)-1);
            _Analysis_assume_(isInternalGuid);
            if (isInternalGuid)
            {
                status = ClassQueryInternalDataBlock(
                                                DeviceObject,
                                                Irp,
                                                guidIndex,
                                                bufferAvail,
                                                buffer + sizeof(WNODE_ALL_DATA));
            } else {
                status = classWmiInfo->ClassQueryWmiDataBlock(
                                                 DeviceObject,
                                                 Irp,
                                                 guidIndex,
                                                 bufferAvail,
                                                 buffer + sizeof(WNODE_ALL_DATA));
            }
            break;
        }

        case IRP_MN_QUERY_SINGLE_INSTANCE:
        {
            PWNODE_SINGLE_INSTANCE wnode;
            ULONG dataBlockOffset;

            wnode = (PWNODE_SINGLE_INSTANCE)buffer;

            dataBlockOffset = wnode->DataBlockOffset;

            NT_ASSERT(guidIndex != (ULONG)-1);
            _Analysis_assume_(isInternalGuid);
            if (isInternalGuid)
            {
                status = ClassQueryInternalDataBlock(
                                            DeviceObject,
                                            Irp,
                                            guidIndex,
                                            bufferSize - dataBlockOffset,
                                            (PUCHAR)wnode + dataBlockOffset);
            } else {
                status = classWmiInfo->ClassQueryWmiDataBlock(
                                              DeviceObject,
                                              Irp,
                                              guidIndex,
                                              bufferSize - dataBlockOffset,
                                              (PUCHAR)wnode + dataBlockOffset);
            }
            break;
        }

        case IRP_MN_CHANGE_SINGLE_INSTANCE:
        {
            PWNODE_SINGLE_INSTANCE wnode;

            wnode = (PWNODE_SINGLE_INSTANCE)buffer;
            _Analysis_assume_(isInternalGuid);
            if (isInternalGuid)
            {
                status = ClassWmiCompleteRequest(DeviceObject,
                                                Irp,
                                                STATUS_WMI_GUID_NOT_FOUND,
                                                0,
                                                IO_NO_INCREMENT);
            } else {

                NT_ASSERT(guidIndex != (ULONG)-1);

                status = classWmiInfo->ClassSetWmiDataBlock(
                                         DeviceObject,
                                         Irp,
                                         guidIndex,
                                         wnode->SizeDataBlock,
                                         (PUCHAR)wnode + wnode->DataBlockOffset);
            }

            break;
        }

        case IRP_MN_CHANGE_SINGLE_ITEM:
        {
            PWNODE_SINGLE_ITEM wnode;

            wnode = (PWNODE_SINGLE_ITEM)buffer;

            NT_ASSERT(guidIndex != (ULONG)-1);
            _Analysis_assume_(isInternalGuid);
            if (isInternalGuid)
            {
                status = ClassWmiCompleteRequest(DeviceObject,
                                                Irp,
                                                STATUS_WMI_GUID_NOT_FOUND,
                                                0,
                                                IO_NO_INCREMENT);
            } else {

                NT_ASSERT(guidIndex != (ULONG)-1);

                status = classWmiInfo->ClassSetWmiDataItem(
                                         DeviceObject,
                                         Irp,
                                         guidIndex,
                                         wnode->ItemId,
                                         wnode->SizeDataItem,
                                         (PUCHAR)wnode + wnode->DataBlockOffset);

            }

            break;
        }

        case IRP_MN_EXECUTE_METHOD:
        {
            PWNODE_METHOD_ITEM wnode;

            wnode = (PWNODE_METHOD_ITEM)buffer;
            _Analysis_assume_(isInternalGuid);
            if (isInternalGuid)
            {
                status = ClassWmiCompleteRequest(DeviceObject,
                                                Irp,
                                                STATUS_WMI_GUID_NOT_FOUND,
                                                0,
                                                IO_NO_INCREMENT);
            } else {

                NT_ASSERT(guidIndex != (ULONG)-1);

                status = classWmiInfo->ClassExecuteWmiMethod(
                                             DeviceObject,
                                             Irp,
                                             guidIndex,
                                             wnode->MethodId,
                                             wnode->SizeDataBlock,
                                             bufferSize - wnode->DataBlockOffset,
                                             buffer + wnode->DataBlockOffset);
            }

            break;
        }

        case IRP_MN_ENABLE_EVENTS:
        {
            _Analysis_assume_(isInternalGuid);
            if (isInternalGuid)
            {
                status = ClassWmiCompleteRequest(DeviceObject,
                                                Irp,
                                                STATUS_WMI_GUID_NOT_FOUND,
                                                0,
                                                IO_NO_INCREMENT);
            } else {

                NT_ASSERT(guidIndex != (ULONG)-1);

                status = classWmiInfo->ClassWmiFunctionControl(
                                                               DeviceObject,
                                                               Irp,
                                                               guidIndex,
                                                               EventGeneration,
                                                               TRUE);
            }
            break;
        }

        case IRP_MN_DISABLE_EVENTS:
        {
            _Analysis_assume_(isInternalGuid);
            if (isInternalGuid)
            {
                status = ClassWmiCompleteRequest(DeviceObject,
                                                Irp,
                                                STATUS_WMI_GUID_NOT_FOUND,
                                                0,
                                                IO_NO_INCREMENT);
            } else {

                NT_ASSERT(guidIndex != (ULONG)-1);

                status = classWmiInfo->ClassWmiFunctionControl(
                                                               DeviceObject,
                                                               Irp,
                                                               guidIndex,
                                                               EventGeneration,
                                                               FALSE);
            }
            break;
        }

        case IRP_MN_ENABLE_COLLECTION:
        {
            _Analysis_assume_(isInternalGuid);
            if (isInternalGuid)
            {
                status = ClassWmiCompleteRequest(DeviceObject,
                                                Irp,
                                                STATUS_WMI_GUID_NOT_FOUND,
                                                0,
                                                IO_NO_INCREMENT);
            } else {

                NT_ASSERT(guidIndex != (ULONG)-1);

                status = classWmiInfo->ClassWmiFunctionControl(
                                                             DeviceObject,
                                                             Irp,
                                                             guidIndex,
                                                             DataBlockCollection,
                                                             TRUE);
            }
            break;
        }

        case IRP_MN_DISABLE_COLLECTION:
        {
            _Analysis_assume_(isInternalGuid);
            if (isInternalGuid)
            {
                status = ClassWmiCompleteRequest(DeviceObject,
                                                Irp,
                                                STATUS_WMI_GUID_NOT_FOUND,
                                                0,
                                                IO_NO_INCREMENT);
            } else {

                NT_ASSERT(guidIndex != (ULONG)-1);

                status = classWmiInfo->ClassWmiFunctionControl(
                                                             DeviceObject,
                                                             Irp,
                                                             guidIndex,
                                                             DataBlockCollection,
                                                             FALSE);
            }

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


NTSTATUS
ClassQueryInternalDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine allows querying for the contents of an internal WMI
    data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    BufferAvail on has the maximum size available to write the data
        block.

    Buffer on return is filled with the returned data block


Return Value:

    status

--*/
{
    NTSTATUS status;
#ifndef __REACTOS__ // WMI in not a thing on ReactOS yet
    ULONG sizeNeeded = 0, i;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = DeviceObject->DeviceExtension;
    if (GuidIndex == MSStorageDriver_ClassErrorLogGuid_Index) {

        //
        // NOTE - ClassErrorLog is still using SCSI_REQUEST_BLOCK and will not be
        // updated to support extended SRB until classpnp is updated to send >16
        // byte CDBs. Extended SRBs will be translated to SCSI_REQUEST_BLOCK.
        //
        sizeNeeded = MSStorageDriver_ClassErrorLog_SIZE;
        if (BufferAvail >= sizeNeeded) {
            PMSStorageDriver_ClassErrorLog errorLog = (PMSStorageDriver_ClassErrorLog) Buffer;
            PMSStorageDriver_ClassErrorLogEntry logEntry;
            PMSStorageDriver_ScsiRequestBlock srbBlock;
            PMSStorageDriver_SenseData senseData;
            PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
            PCLASS_ERROR_LOG_DATA fdoLogEntry;
            PSCSI_REQUEST_BLOCK fdoSRBBlock;
            PSENSE_DATA fdoSenseData;
            errorLog->numEntries = NUM_ERROR_LOG_ENTRIES;
            for (i = 0; i < NUM_ERROR_LOG_ENTRIES; i++) {
                fdoLogEntry = &fdoData->ErrorLogs[i];
                fdoSRBBlock = &fdoLogEntry->Srb;
                fdoSenseData = &fdoLogEntry->SenseData;
                logEntry = &errorLog->logEntries[i];
                srbBlock = &logEntry->srb;
                senseData = &logEntry->senseData;
                logEntry->tickCount = fdoLogEntry->TickCount.QuadPart;
                logEntry->portNumber = fdoLogEntry->PortNumber;
                logEntry->errorPaging = (fdoLogEntry->ErrorPaging == 0 ? FALSE : TRUE);
                logEntry->errorRetried = (fdoLogEntry->ErrorRetried == 0 ? FALSE : TRUE);
                logEntry->errorUnhandled = (fdoLogEntry->ErrorUnhandled == 0 ? FALSE : TRUE);
                logEntry->errorReserved = fdoLogEntry->ErrorReserved;
                RtlMoveMemory(logEntry->reserved, fdoLogEntry->Reserved, sizeof(logEntry->reserved));
                ConvertTickToDateTime(fdoLogEntry->TickCount, logEntry->eventTime);

                srbBlock->length = fdoSRBBlock->Length;
                srbBlock->function = fdoSRBBlock->Function;
                srbBlock->srbStatus = fdoSRBBlock->SrbStatus;
                srbBlock->scsiStatus = fdoSRBBlock->ScsiStatus;
                srbBlock->pathID = fdoSRBBlock->PathId;
                srbBlock->targetID = fdoSRBBlock->TargetId;
                srbBlock->lun = fdoSRBBlock->Lun;
                srbBlock->queueTag = fdoSRBBlock->QueueTag;
                srbBlock->queueAction = fdoSRBBlock->QueueAction;
                srbBlock->cdbLength = fdoSRBBlock->CdbLength;
                srbBlock->senseInfoBufferLength = fdoSRBBlock->SenseInfoBufferLength;
                srbBlock->srbFlags = fdoSRBBlock->SrbFlags;
                srbBlock->dataTransferLength = fdoSRBBlock->DataTransferLength;
                srbBlock->timeOutValue = fdoSRBBlock->TimeOutValue;
                srbBlock->dataBuffer = (ULONGLONG) fdoSRBBlock->DataBuffer;
                srbBlock->senseInfoBuffer = (ULONGLONG) fdoSRBBlock->SenseInfoBuffer;
                srbBlock->nextSRB = (ULONGLONG) fdoSRBBlock->NextSrb;
                srbBlock->originalRequest = (ULONGLONG) fdoSRBBlock->OriginalRequest;
                srbBlock->srbExtension = (ULONGLONG) fdoSRBBlock->SrbExtension;
                srbBlock->internalStatus = fdoSRBBlock->InternalStatus;
#if defined(_WIN64)
                srbBlock->reserved = fdoSRBBlock->Reserved;
#else
                srbBlock->reserved = 0;
#endif
                RtlMoveMemory(srbBlock->cdb, fdoSRBBlock->Cdb, sizeof(srbBlock->cdb));

                //
                // Note: Sense data has been converted into Fixed format before it was
                //       put in the log.  Therefore, no conversion is needed here.
                //
                senseData->errorCode = fdoSenseData->ErrorCode;
                senseData->valid = (fdoSenseData->Valid == 0 ? FALSE : TRUE);
                senseData->segmentNumber = fdoSenseData->SegmentNumber;
                senseData->senseKey = fdoSenseData->SenseKey;
                senseData->reserved = (fdoSenseData->Reserved == 0 ? FALSE : TRUE);
                senseData->incorrectLength = (fdoSenseData->IncorrectLength == 0 ? FALSE : TRUE);
                senseData->endOfMedia = (fdoSenseData->EndOfMedia == 0 ? FALSE : TRUE);
                senseData->fileMark = (fdoSenseData->FileMark == 0 ? FALSE : TRUE);
                RtlMoveMemory(senseData->information, fdoSenseData->Information, sizeof(senseData->information));
                senseData->additionalSenseLength = fdoSenseData->AdditionalSenseLength;
                RtlMoveMemory(senseData->commandSpecificInformation, fdoSenseData->CommandSpecificInformation, sizeof(senseData->commandSpecificInformation));
                senseData->additionalSenseCode = fdoSenseData->AdditionalSenseCode;
                senseData->additionalSenseCodeQualifier = fdoSenseData->AdditionalSenseCodeQualifier;
                senseData->fieldReplaceableUnitCode = fdoSenseData->FieldReplaceableUnitCode;
                RtlMoveMemory(senseData->senseKeySpecific, fdoSenseData->SenseKeySpecific, sizeof(senseData->senseKeySpecific));
            }
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
    } else if (GuidIndex > 0 && GuidIndex < NUM_CLASS_WMI_GUIDS) {
        status = STATUS_WMI_INSTANCE_NOT_FOUND;
    } else {
        status = STATUS_WMI_GUID_NOT_FOUND;
    }
#else
    ULONG sizeNeeded = 0;
    status = STATUS_WMI_GUID_NOT_FOUND;
#endif
    status = ClassWmiCompleteRequest(DeviceObject,
                                    Irp,
                                    status,
                                    sizeNeeded,
                                    IO_NO_INCREMENT);
    return status;
}

PWCHAR
ConvertTickToDateTime(
    IN LARGE_INTEGER Tick,
    _Out_writes_(TIME_STRING_LENGTH) PWCHAR String
    )

/*++

Routine Description:

    This routine converts a tick count to a datetime (MOF) data type

Arguments:

    Tick - The tick count that needs to be converted
    String - The buffer to hold the time string, must be able to hold WCHAR[25]

Return Value:

    The time string

--*/

{
    LARGE_INTEGER nowTick, nowTime, time;
    ULONG maxInc = 0;
    TIME_FIELDS timeFields = {0};
    WCHAR outDateTime[TIME_STRING_LENGTH + 1];

    nowTick.QuadPart = 0;
    nowTime.QuadPart = 0;
    //
    // Translate the tick count to a system time
    //
    KeQueryTickCount(&nowTick);
    maxInc = KeQueryTimeIncrement();
    KeQuerySystemTime(&nowTime);
    time.QuadPart = nowTime.QuadPart - ((nowTick.QuadPart - Tick.QuadPart) * maxInc);

    RtlTimeToTimeFields(&time, &timeFields);

    //
    // The buffer String is of size MAX_PATH. Use that to specify the buffer size.
    //
    //yyyymmddhhmmss.mmmmmmsutc
    RtlStringCbPrintfW(outDateTime, sizeof(outDateTime), L"%04d%02d%02d%02d%02d%02d.%03d***+000", timeFields.Year, timeFields.Month, timeFields.Day, timeFields.Hour, timeFields.Minute, timeFields.Second, timeFields.Milliseconds);
    RtlMoveMemory(String, outDateTime, sizeof(WCHAR) * TIME_STRING_LENGTH);
    return  String;
}

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
SCSIPORT_API
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassWmiCompleteRequest(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _In_ NTSTATUS Status,
    _In_ ULONG BufferUsed,
    _In_ CCHAR PriorityBoost
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
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
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassWmiFireEvent(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ LPGUID Guid,
    _In_ ULONG InstanceIndex,
    _In_ ULONG EventDataSize,
    _In_reads_bytes_(EventDataSize) PVOID EventData
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

    event = ExAllocatePoolWithTag(NonPagedPoolNx, sizeNeeded, CLASS_TAG_WMI);
    if (event != NULL)
    {
        RtlZeroMemory(event, sizeNeeded);
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
            FREE_POOL(event);
        }
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return(status);
} // end ClassWmiFireEvent()
