/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    class.c

Abstract:

    SCSI class driver routines

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "classp.h"

#include <stddef.h>

#include <initguid.h>
#include <mountdev.h>

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(INIT, DriverEntry)
    #pragma alloc_text(PAGE, ClassAddDevice)
    #pragma alloc_text(PAGE, ClassClaimDevice)
    #pragma alloc_text(PAGE, ClassCreateDeviceObject)
    #pragma alloc_text(PAGE, ClassDispatchPnp)
    #pragma alloc_text(PAGE, ClassGetDescriptor)
    #pragma alloc_text(PAGE, ClassGetPdoId)
    #pragma alloc_text(PAGE, ClassInitialize)
    #pragma alloc_text(PAGE, ClassInitializeEx)
    #pragma alloc_text(PAGE, ClassInvalidateBusRelations)
    #pragma alloc_text(PAGE, ClassMarkChildMissing)
    #pragma alloc_text(PAGE, ClassMarkChildrenMissing)
    #pragma alloc_text(PAGE, ClassModeSense)
    #pragma alloc_text(PAGE, ClassPnpQueryFdoRelations)
    #pragma alloc_text(PAGE, ClassPnpStartDevice)
    #pragma alloc_text(PAGE, ClassQueryPnpCapabilities)
    #pragma alloc_text(PAGE, ClassQueryTimeOutRegistryValue)
    #pragma alloc_text(PAGE, ClassRemoveDevice)
    #pragma alloc_text(PAGE, ClassRetrieveDeviceRelations)
    #pragma alloc_text(PAGE, ClassUpdateInformationInRegistry)
    #pragma alloc_text(PAGE, ClassSendDeviceIoControlSynchronous)
    #pragma alloc_text(PAGE, ClassUnload)
    #pragma alloc_text(PAGE, ClasspAllocateReleaseRequest)
    #pragma alloc_text(PAGE, ClasspFreeReleaseRequest)
    #pragma alloc_text(PAGE, ClasspInitializeHotplugInfo)
    #pragma alloc_text(PAGE, ClasspRegisterMountedDeviceInterface)
    #pragma alloc_text(PAGE, ClasspScanForClassHacks)
    #pragma alloc_text(PAGE, ClasspScanForSpecialInRegistry)
#endif

ULONG ClassPnpAllowUnload = TRUE;


#define FirstDriveLetter 'C'
#define LastDriveLetter  'Z'



/*++////////////////////////////////////////////////////////////////////////////

DriverEntry()

Routine Description:

    Temporary entry point needed to initialize the class system dll.
    It doesn't do anything.

Arguments:

    DriverObject - Pointer to the driver object created by the system.

Return Value:

   STATUS_SUCCESS

--*/
NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    return STATUS_SUCCESS;
}

/*++////////////////////////////////////////////////////////////////////////////

ClassInitialize()

Routine Description:

    This routine is called by a class driver during its
    DriverEntry routine to initialize the driver.

Arguments:

    Argument1          - Driver Object.
    Argument2          - Registry Path.
    InitializationData - Device-specific driver's initialization data.

Return Value:

    A valid return code for a DriverEntry routine.

--*/
ULONG
NTAPI
ClassInitialize(
    IN  PVOID            Argument1,
    IN  PVOID            Argument2,
    IN  PCLASS_INIT_DATA InitializationData
    )
{
    PDRIVER_OBJECT  DriverObject = Argument1;
    PUNICODE_STRING RegistryPath = Argument2;

    PCLASS_DRIVER_EXTENSION driverExtension;

    NTSTATUS        status;

    PAGED_CODE();
    
    DebugPrint((3,"\n\nSCSI Class Driver\n"));

    ClasspInitializeDebugGlobals();

    //
    // Validate the length of this structure. This is effectively a
    // version check.
    //

    if (InitializationData->InitializationDataSize != sizeof(CLASS_INIT_DATA)) {

        //
        // This DebugPrint is to help third-party driver writers
        //

        DebugPrint((0,"ClassInitialize: Class driver wrong version\n"));
        return (ULONG) STATUS_REVISION_MISMATCH;
    }

    //
    // Check that each required entry is not NULL. Note that Shutdown, Flush and Error
    // are not required entry points.
    //

    if ((!InitializationData->FdoData.ClassDeviceControl) ||
        (!((InitializationData->FdoData.ClassReadWriteVerification) ||
           (InitializationData->ClassStartIo))) ||
        (!InitializationData->ClassAddDevice) ||
        (!InitializationData->FdoData.ClassStartDevice)) {

        //
        // This DebugPrint is to help third-party driver writers
        //

        DebugPrint((0,
            "ClassInitialize: Class device-specific driver missing required "
            "FDO entry\n"));

        return (ULONG) STATUS_REVISION_MISMATCH;
    }

    if ((InitializationData->ClassEnumerateDevice) &&
        ((!InitializationData->PdoData.ClassDeviceControl) ||
         (!InitializationData->PdoData.ClassStartDevice) ||
         (!((InitializationData->PdoData.ClassReadWriteVerification) ||
            (InitializationData->ClassStartIo))))) {

        //
        // This DebugPrint is to help third-party driver writers
        //

        DebugPrint((0, "ClassInitialize: Class device-specific missing "
                       "required PDO entry\n"));

        return (ULONG) STATUS_REVISION_MISMATCH;
    }

    if((InitializationData->FdoData.ClassStopDevice == NULL) ||
        ((InitializationData->ClassEnumerateDevice != NULL) &&
         (InitializationData->PdoData.ClassStopDevice == NULL))) {

        //
        // This DebugPrint is to help third-party driver writers
        //

        DebugPrint((0, "ClassInitialize: Class device-specific missing "
                       "required PDO entry\n"));
        ASSERT(FALSE);
        return (ULONG) STATUS_REVISION_MISMATCH;
    }

    //
    // Setup the default power handlers if the class driver didn't provide
    // any.
    //

    if(InitializationData->FdoData.ClassPowerDevice == NULL) {
        InitializationData->FdoData.ClassPowerDevice = ClassMinimalPowerHandler;
    }

    if((InitializationData->ClassEnumerateDevice != NULL) &&
       (InitializationData->PdoData.ClassPowerDevice == NULL)) {
        InitializationData->PdoData.ClassPowerDevice = ClassMinimalPowerHandler;
    }

    //
    // warn that unload is not supported
    //
    // ISSUE-2000/02/03-peterwie
    // We should think about making this a fatal error.
    //

    if(InitializationData->ClassUnload == NULL) {

        //
        // This DebugPrint is to help third-party driver writers
        //

        DebugPrint((0, "ClassInitialize: driver does not support unload %wZ\n",
                    RegistryPath));
    }

    //
    // Create an extension for the driver object
    //

    status = IoAllocateDriverObjectExtension(DriverObject,
                                             CLASS_DRIVER_EXTENSION_KEY,
                                             sizeof(CLASS_DRIVER_EXTENSION),
                                             (PVOID *)&driverExtension);

    if(NT_SUCCESS(status)) {

        //
        // Copy the registry path into the driver extension so we can use it later
        //

        driverExtension->RegistryPath.Length = RegistryPath->Length;
        driverExtension->RegistryPath.MaximumLength = RegistryPath->MaximumLength;

        driverExtension->RegistryPath.Buffer =
            ExAllocatePoolWithTag(PagedPool,
                                  RegistryPath->MaximumLength,
                                  '1CcS');

        if(driverExtension->RegistryPath.Buffer == NULL) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            return status;
        }

        RtlCopyUnicodeString(
            &(driverExtension->RegistryPath),
            RegistryPath);

        //
        // Copy the initialization data into the driver extension so we can reuse
        // it during our add device routine
        //

        RtlCopyMemory(
            &(driverExtension->InitData),
            InitializationData,
            sizeof(CLASS_INIT_DATA));

        driverExtension->DeviceCount = 0;

    } else if (status == STATUS_OBJECT_NAME_COLLISION) {

        //
        // The extension already exists - get a pointer to it
        //

        driverExtension = IoGetDriverObjectExtension(DriverObject,
                                                     CLASS_DRIVER_EXTENSION_KEY);

        ASSERT(driverExtension != NULL);

    } else {

        DebugPrint((1, "ClassInitialize: Class driver extension could not be "
                       "allocated %lx\n", status));
        return status;
    }

    //
    // Update driver object with entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = ClassCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ClassCreateClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = ClassReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = ClassReadWrite;
    DriverObject->MajorFunction[IRP_MJ_SCSI] = ClassInternalIoControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ClassDeviceControlDispatch;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = ClassShutdownFlush;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = ClassShutdownFlush;
    DriverObject->MajorFunction[IRP_MJ_PNP] = ClassDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = ClassDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = ClassSystemControl;

    if (InitializationData->ClassStartIo) {
        DriverObject->DriverStartIo = ClasspStartIo;
    }

    if ((InitializationData->ClassUnload) && (ClassPnpAllowUnload != FALSE)) {
        DriverObject->DriverUnload = ClassUnload;
    } else {
        DriverObject->DriverUnload = NULL;
    }

    DriverObject->DriverExtension->AddDevice = ClassAddDevice;

    DbgPrint("Driver is ready to go\n");
    status = STATUS_SUCCESS;
    return status;
} // end ClassInitialize()

/*++////////////////////////////////////////////////////////////////////////////

ClassInitializeEx()

Routine Description:

    This routine is allows the caller to do any extra initialization or
    setup that is not done in ClassInitialize. The operation is
    controlled by the GUID that is passed and the contents of the Data
    parameter is dependent upon the GUID.

    This is the list of supported operations:

    Guid - GUID_CLASSPNP_QUERY_REGINFOEX
    Data - A PCLASS_QUERY_WMI_REGINFO_EX callback function pointer

        Initialized classpnp to callback a PCLASS_QUERY_WMI_REGINFO_EX
        callback instead of a PCLASS_QUERY_WMI_REGINFO callback. The
        former callback allows the driver to specify the name of the
        mof resource.

Arguments:

    DriverObject
    Guid
    Data

Return Value:

    Status Code

--*/
ULONG
NTAPI
ClassInitializeEx(
    IN  PDRIVER_OBJECT   DriverObject,
    IN  LPGUID           Guid,
    IN  PVOID            Data
    )
{
    PCLASS_DRIVER_EXTENSION driverExtension;

    NTSTATUS        status;

    PAGED_CODE();

    driverExtension = IoGetDriverObjectExtension( DriverObject,
                                                  CLASS_DRIVER_EXTENSION_KEY
                                                  );
    if (IsEqualGUID(Guid, &ClassGuidQueryRegInfoEx))
    {
        PCLASS_QUERY_WMI_REGINFO_EX_LIST List;

        //
        // Indicate the device supports PCLASS_QUERY_REGINFO_EX
        // callback instead of PCLASS_QUERY_REGINFO callback.
        //
        List = (PCLASS_QUERY_WMI_REGINFO_EX_LIST)Data;

        if (List->Size == sizeof(CLASS_QUERY_WMI_REGINFO_EX_LIST))
        {
            driverExtension->ClassFdoQueryWmiRegInfoEx = List->ClassFdoQueryWmiRegInfoEx;
            driverExtension->ClassPdoQueryWmiRegInfoEx = List->ClassPdoQueryWmiRegInfoEx;
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_INVALID_PARAMETER;
        }
    } else {
        status = STATUS_NOT_SUPPORTED;
    }

    return(status);

} // end ClassInitializeEx()

/*++////////////////////////////////////////////////////////////////////////////

ClassUnload()

Routine Description:

    called when there are no more references to the driver.  this allows
    drivers to be updated without rebooting.

Arguments:

    DriverObject - a pointer to the driver object that is being unloaded

Status:

--*/
VOID
NTAPI
ClassUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PCLASS_DRIVER_EXTENSION driverExtension;
    //NTSTATUS status;

    PAGED_CODE();

    ASSERT( DriverObject->DeviceObject == NULL );

    driverExtension = IoGetDriverObjectExtension( DriverObject,
                                                  CLASS_DRIVER_EXTENSION_KEY
                                                  );

    ASSERT(driverExtension != NULL);
    ASSERT(driverExtension->RegistryPath.Buffer != NULL);
    ASSERT(driverExtension->InitData.ClassUnload != NULL);

    DebugPrint((1, "ClassUnload: driver unloading %wZ\n",
                &driverExtension->RegistryPath));

    //
    // attempt to process the driver's unload routine first.
    //

    driverExtension->InitData.ClassUnload(DriverObject);

    //
    // free own allocated resources and return
    //

    ExFreePool( driverExtension->RegistryPath.Buffer );
    driverExtension->RegistryPath.Buffer = NULL;
    driverExtension->RegistryPath.Length = 0;
    driverExtension->RegistryPath.MaximumLength = 0;

    return;
} // end ClassUnload()

/*++////////////////////////////////////////////////////////////////////////////

ClassAddDevice()

Routine Description:

    SCSI class driver add device routine.  This is called by pnp when a new
    physical device come into being.

    This routine will call out to the class driver to verify that it should
    own this device then will create and attach a device object and then hand
    it to the driver to initialize and create symbolic links

Arguments:

    DriverObject - a pointer to the driver object that this is being created for
    PhysicalDeviceObject - a pointer to the physical device object

Status: STATUS_NO_SUCH_DEVICE if the class driver did not want this device
    STATUS_SUCCESS if the creation and attachment was successful
    status of device creation and initialization

--*/
NTSTATUS
NTAPI
ClassAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
{
    PCLASS_DRIVER_EXTENSION driverExtension =
        IoGetDriverObjectExtension(DriverObject,
                                   CLASS_DRIVER_EXTENSION_KEY);

    NTSTATUS status;

    PAGED_CODE();

    DbgPrint("got a device\n");
    status = driverExtension->InitData.ClassAddDevice(DriverObject,
                                                      PhysicalDeviceObject);
    return status;
} // end ClassAddDevice()

/*++////////////////////////////////////////////////////////////////////////////

ClassDispatchPnp()

Routine Description:

    Storage class driver pnp routine.  This is called by the io system when
    a PNP request is sent to the device.

Arguments:

    DeviceObject - pointer to the device object

    Irp - pointer to the io request packet

Return Value:

    status

--*/
NTSTATUS
NTAPI
ClassDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    BOOLEAN isFdo = commonExtension->IsFdo;

    PCLASS_DRIVER_EXTENSION driverExtension;
    PCLASS_INIT_DATA initData;
    PCLASS_DEV_INFO devInfo;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    NTSTATUS status = Irp->IoStatus.Status;
    BOOLEAN completeRequest = TRUE;
    BOOLEAN lockReleased = FALSE;

    PAGED_CODE();

    //
    // Extract all the useful information out of the driver object
    // extension
    //

    driverExtension = IoGetDriverObjectExtension(DeviceObject->DriverObject,
                                                 CLASS_DRIVER_EXTENSION_KEY);
    if (driverExtension){

        initData = &(driverExtension->InitData);

        if(isFdo) {
            devInfo = &(initData->FdoData);
        } else {
            devInfo = &(initData->PdoData);
        }

        ClassAcquireRemoveLock(DeviceObject, Irp);

        DebugPrint((2, "ClassDispatchPnp (%p,%p): minor code %#x for %s %p\n",
                       DeviceObject, Irp,
                       irpStack->MinorFunction,
                       isFdo ? "fdo" : "pdo",
                       DeviceObject));
        DebugPrint((2, "ClassDispatchPnp (%p,%p): previous %#x, current %#x\n",
                       DeviceObject, Irp,
                       commonExtension->PreviousState,
                       commonExtension->CurrentState));

        switch(irpStack->MinorFunction) {

            case IRP_MN_START_DEVICE: {

                //
                // if this is sent to the FDO we should forward it down the
                // attachment chain before we start the FDO.
                //

                if (isFdo) {
                    status = ClassForwardIrpSynchronous(commonExtension, Irp);
                }
                else {
                    status = STATUS_SUCCESS;
                }

                if (NT_SUCCESS(status)){
                    status = Irp->IoStatus.Status = ClassPnpStartDevice(DeviceObject);
                }

                break;
            }


            case IRP_MN_QUERY_DEVICE_RELATIONS: {

                DEVICE_RELATION_TYPE type =
                    irpStack->Parameters.QueryDeviceRelations.Type;

                PDEVICE_RELATIONS deviceRelations = NULL;

                if(!isFdo) {

                    if(type == TargetDeviceRelation) {

                        //
                        // Device relations has one entry built in to it's size.
                        //

                        status = STATUS_INSUFFICIENT_RESOURCES;

                        deviceRelations = ExAllocatePoolWithTag(PagedPool,
                                                         sizeof(DEVICE_RELATIONS),
                                                         '2CcS');

                        if(deviceRelations != NULL) {

                            RtlZeroMemory(deviceRelations,
                                          sizeof(DEVICE_RELATIONS));

                            Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;

                            deviceRelations->Count = 1;
                            deviceRelations->Objects[0] = DeviceObject;
                            ObReferenceObject(deviceRelations->Objects[0]);

                            status = STATUS_SUCCESS;
                        }

                    } else {
                        //
                        // PDO's just complete enumeration requests without altering
                        // the status.
                        //

                        status = Irp->IoStatus.Status;
                    }

                    break;

                } else if (type == BusRelations) {

                    ASSERT(commonExtension->IsInitialized);

                    //
                    // Make sure we support enumeration
                    //

                    if(initData->ClassEnumerateDevice == NULL) {

                        //
                        // Just send the request down to the lower driver.  Perhaps
                        // It can enumerate children.
                        //

                    } else {

                        //
                        // Re-enumerate the device
                        //

                        status = ClassPnpQueryFdoRelations(DeviceObject, Irp);

                        if(!NT_SUCCESS(status)) {
                            completeRequest = TRUE;
                            break;
                        }
                    }
                }

                IoCopyCurrentIrpStackLocationToNext(Irp);
                ClassReleaseRemoveLock(DeviceObject, Irp);
                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                completeRequest = FALSE;

                break;
            }

            case IRP_MN_QUERY_ID: {

                BUS_QUERY_ID_TYPE idType = irpStack->Parameters.QueryId.IdType;
                UNICODE_STRING unicodeString;

                if(isFdo) {

                    //
                    // FDO's should just forward the query down to the lower
                    // device objects
                    //

                    IoCopyCurrentIrpStackLocationToNext(Irp);
                    ClassReleaseRemoveLock(DeviceObject, Irp);

                    status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                    completeRequest = FALSE;
                    break;
                }

                //
                // PDO's need to give an answer - this is easy for now
                //

                RtlInitUnicodeString(&unicodeString, NULL);

                status = ClassGetPdoId(DeviceObject,
                                       idType,
                                       &unicodeString);

                if(status == STATUS_NOT_IMPLEMENTED) {
                    //
                    // The driver doesn't implement this ID (whatever it is).
                    // Use the status out of the IRP so that we don't mangle a
                    // response from someone else.
                    //

                    status = Irp->IoStatus.Status;
                } else if(NT_SUCCESS(status)) {
                    Irp->IoStatus.Information = (ULONG_PTR) unicodeString.Buffer;
                } else {
                    Irp->IoStatus.Information = (ULONG_PTR) NULL;
                }

                break;
            }

            case IRP_MN_QUERY_STOP_DEVICE:
            case IRP_MN_QUERY_REMOVE_DEVICE: {

                DebugPrint((2, "ClassDispatchPnp (%p,%p): Processing QUERY_%s irp\n",
                            DeviceObject, Irp,
                            ((irpStack->MinorFunction == IRP_MN_QUERY_STOP_DEVICE) ?
                             "STOP" : "REMOVE")));

                //
                // If this device is in use for some reason (paging, etc...)
                // then we need to fail the request.
                //

                if(commonExtension->PagingPathCount != 0) {

                    DebugPrint((1, "ClassDispatchPnp (%p,%p): device is in paging "
                                "path and cannot be removed\n",
                                DeviceObject, Irp));
                    status = STATUS_DEVICE_BUSY;
                    break;
                }

                //
                // Check with the class driver to see if the query operation
                // can succeed.
                //

                if(irpStack->MinorFunction == IRP_MN_QUERY_STOP_DEVICE) {
                    status = devInfo->ClassStopDevice(DeviceObject,
                                                      irpStack->MinorFunction);
                } else {
                    status = devInfo->ClassRemoveDevice(DeviceObject,
                                                        irpStack->MinorFunction);
                }

                if(NT_SUCCESS(status)) {

                    //
                    // ASSERT that we never get two queries in a row, as
                    // this will severely mess up the state machine
                    //
                    ASSERT(commonExtension->CurrentState != irpStack->MinorFunction);
                    commonExtension->PreviousState = commonExtension->CurrentState;
                    commonExtension->CurrentState = irpStack->MinorFunction;

                    if(isFdo) {
                        DebugPrint((2, "ClassDispatchPnp (%p,%p): Forwarding QUERY_"
                                    "%s irp\n", DeviceObject, Irp,
                                    ((irpStack->MinorFunction == IRP_MN_QUERY_STOP_DEVICE) ?
                                     "STOP" : "REMOVE")));
                        status = ClassForwardIrpSynchronous(commonExtension, Irp);
                    }
                }
                DebugPrint((2, "ClassDispatchPnp (%p,%p): Final status == %x\n",
                            DeviceObject, Irp, status));

                break;
            }

            case IRP_MN_CANCEL_STOP_DEVICE:
            case IRP_MN_CANCEL_REMOVE_DEVICE: {

                //
                // Check with the class driver to see if the query or cancel
                // operation can succeed.
                //

                if(irpStack->MinorFunction == IRP_MN_CANCEL_STOP_DEVICE) {
                    status = devInfo->ClassStopDevice(DeviceObject,
                                                      irpStack->MinorFunction);
                    NT_ASSERTMSGW(L"ClassDispatchPnp !! CANCEL_STOP_DEVICE should "
                                  L"never be failed\n", NT_SUCCESS(status));
                } else {
                    status = devInfo->ClassRemoveDevice(DeviceObject,
                                                        irpStack->MinorFunction);
                    NT_ASSERTMSGW(L"ClassDispatchPnp !! CANCEL_REMOVE_DEVICE should "
                                  L"never be failed\n", NT_SUCCESS(status));
                }

                Irp->IoStatus.Status = status;

                //
                // We got a CANCEL - roll back to the previous state only
                // if the current state is the respective QUERY state.
                //

                if(((irpStack->MinorFunction == IRP_MN_CANCEL_STOP_DEVICE) &&
                    (commonExtension->CurrentState == IRP_MN_QUERY_STOP_DEVICE)
                    ) ||
                   ((irpStack->MinorFunction == IRP_MN_CANCEL_REMOVE_DEVICE) &&
                    (commonExtension->CurrentState == IRP_MN_QUERY_REMOVE_DEVICE)
                    )
                   ) {

                    commonExtension->CurrentState =
                        commonExtension->PreviousState;
                    commonExtension->PreviousState = 0xff;

                }

                if(isFdo) {
                    IoCopyCurrentIrpStackLocationToNext(Irp);
                    ClassReleaseRemoveLock(DeviceObject, Irp);
                    status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                    completeRequest = FALSE;
                } else {
                    status = STATUS_SUCCESS;
                }

                break;
            }

            case IRP_MN_STOP_DEVICE: {

                //
                // These all mean nothing to the class driver currently.  The
                // port driver will handle all queueing when necessary.
                //

                DebugPrint((2, "ClassDispatchPnp (%p,%p): got stop request for %s\n",
                            DeviceObject, Irp,
                            (isFdo ? "fdo" : "pdo")
                            ));

                ASSERT(commonExtension->PagingPathCount == 0);

                //
                // ISSUE-2000/02/03-peterwie
                // if we stop the timer here then it means no class driver can
                // do i/o in its ClassStopDevice routine.  This is because the
                // retry (among other things) is tied into the tick handler
                // and disabling retries could cause the class driver to deadlock.
                // Currently no class driver we're aware of issues i/o in its
                // Stop routine but this is a case we may want to defend ourself
                // against.
                //

                if (DeviceObject->Timer) {
                    IoStopTimer(DeviceObject);
                }

                status = devInfo->ClassStopDevice(DeviceObject, IRP_MN_STOP_DEVICE);

                NT_ASSERTMSGW(L"ClassDispatchPnp !! STOP_DEVICE should "
                              L"never be failed\n", NT_SUCCESS(status));

                if(isFdo) {
                    status = ClassForwardIrpSynchronous(commonExtension, Irp);
                }

                if(NT_SUCCESS(status)) {
                    commonExtension->CurrentState = irpStack->MinorFunction;
                    commonExtension->PreviousState = 0xff;
                }

                break;
            }

            case IRP_MN_REMOVE_DEVICE:
            case IRP_MN_SURPRISE_REMOVAL: {

                UCHAR removeType = irpStack->MinorFunction;

                if (commonExtension->PagingPathCount != 0) {
                    DBGTRACE(ClassDebugWarning, ("ClassDispatchPnp (%p,%p): paging device is getting removed!", DeviceObject, Irp));
                }

                //
                // Release the lock for this IRP before calling in.
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);
                lockReleased = TRUE;

                /*
                 *  If a timer was started on the device, stop it.
                 */
                if (DeviceObject->Timer) {
                    IoStopTimer(DeviceObject);
                }

                /*
                 *  "Fire-and-forget" the remove irp to the lower stack.
                 *  Don't touch the irp (or the irp stack!) after this.
                 */
                if (isFdo) {
                    IoCopyCurrentIrpStackLocationToNext(Irp);
                    status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                    ASSERT(NT_SUCCESS(status));
                    completeRequest = FALSE;
                }
                else {
                    status = STATUS_SUCCESS;
                }

                /*
                 *  Do our own cleanup and call the class driver's remove
                 *  cleanup routine.
                 *  For IRP_MN_REMOVE_DEVICE, this also deletes our device object,
                 *  so don't touch the extension after this.
                 */
                commonExtension->PreviousState = commonExtension->CurrentState;
                commonExtension->CurrentState = removeType;
                ClassRemoveDevice(DeviceObject, removeType);

                break;
            }

            case IRP_MN_DEVICE_USAGE_NOTIFICATION: {

                switch(irpStack->Parameters.UsageNotification.Type) {

                    case DeviceUsageTypePaging: {

                        BOOLEAN setPagable;

                        if((irpStack->Parameters.UsageNotification.InPath) &&
                           (commonExtension->CurrentState != IRP_MN_START_DEVICE)) {

                            //
                            // Device isn't started.  Don't allow adding a
                            // paging file, but allow a removal of one.
                            //

                            status = STATUS_DEVICE_NOT_READY;
                            break;
                        }

                        ASSERT(commonExtension->IsInitialized);

                        //
                        // need to synchronize this now...
                        //

                        KeEnterCriticalRegion();
                        status = KeWaitForSingleObject(&commonExtension->PathCountEvent,
                                                       Executive, KernelMode,
                                                       FALSE, NULL);
                        ASSERT(NT_SUCCESS(status));
                        status = STATUS_SUCCESS;

                        //
                        // If the volume is removable we should try to lock it in
                        // place or unlock it once per paging path count
                        //

                        if (commonExtension->IsFdo){
                            status = ClasspEjectionControl(
                                            DeviceObject,
                                            Irp,
                                            InternalMediaLock,
                                            (BOOLEAN)irpStack->Parameters.UsageNotification.InPath);
                        }

                        if (!NT_SUCCESS(status)){
                            KeSetEvent(&commonExtension->PathCountEvent, IO_NO_INCREMENT, FALSE);
                            KeLeaveCriticalRegion();
                            break;
                        }

                        //
                        // if removing last paging device, need to set DO_POWER_PAGABLE
                        // bit here, and possible re-set it below on failure.
                        //

                        setPagable = FALSE;

                        if (!irpStack->Parameters.UsageNotification.InPath &&
                            commonExtension->PagingPathCount == 1
                            ) {

                            //
                            // removing last paging file
                            // must have DO_POWER_PAGABLE bits set, but only
                            // if noone set the DO_POWER_INRUSH bit
                            //


                            if (TEST_FLAG(DeviceObject->Flags, DO_POWER_INRUSH)) {
                                DebugPrint((2, "ClassDispatchPnp (%p,%p): Last "
                                            "paging file removed, but "
                                            "DO_POWER_INRUSH was set, so NOT "
                                            "setting DO_POWER_PAGABLE\n",
                                            DeviceObject, Irp));
                            } else {
                                DebugPrint((2, "ClassDispatchPnp (%p,%p): Last "
                                            "paging file removed, "
                                            "setting DO_POWER_PAGABLE\n",
                                            DeviceObject, Irp));
                                SET_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);
                                setPagable = TRUE;
                            }

                        }

                        //
                        // forward the irp before finishing handling the
                        // special cases
                        //

                        status = ClassForwardIrpSynchronous(commonExtension, Irp);

                        //
                        // now deal with the failure and success cases.
                        // note that we are not allowed to fail the irp
                        // once it is sent to the lower drivers.
                        //

                        if (NT_SUCCESS(status)) {

                            IoAdjustPagingPathCount(
                                (PLONG)&commonExtension->PagingPathCount,
                                irpStack->Parameters.UsageNotification.InPath);

                            if (irpStack->Parameters.UsageNotification.InPath) {
                                if (commonExtension->PagingPathCount == 1) {
                                    DebugPrint((2, "ClassDispatchPnp (%p,%p): "
                                                "Clearing PAGABLE bit\n",
                                                DeviceObject, Irp));
                                    CLEAR_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);
                                }
                            }

                        } else {

                            //
                            // cleanup the changes done above
                            //

                            if (setPagable != FALSE) {
                                DebugPrint((2, "ClassDispatchPnp (%p,%p): Unsetting "
                                            "PAGABLE bit due to irp failure\n",
                                            DeviceObject, Irp));
                                CLEAR_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);
                                setPagable = FALSE;
                            }

                            //
                            // relock or unlock the media if needed.
                            //

                            if (commonExtension->IsFdo) {

                                ClasspEjectionControl(
                                        DeviceObject,
                                        Irp,
                                        InternalMediaLock,
                                        (BOOLEAN)!irpStack->Parameters.UsageNotification.InPath);
                            }
                        }

                        //
                        // set the event so the next one can occur.
                        //

                        KeSetEvent(&commonExtension->PathCountEvent,
                                   IO_NO_INCREMENT, FALSE);
                        KeLeaveCriticalRegion();
                        break;
                    }

                    case DeviceUsageTypeHibernation: {

                        IoAdjustPagingPathCount(
                            (PLONG)&commonExtension->HibernationPathCount,
                            irpStack->Parameters.UsageNotification.InPath
                            );
                        status = ClassForwardIrpSynchronous(commonExtension, Irp);
                        if (!NT_SUCCESS(status)) {
                            IoAdjustPagingPathCount(
                                (PLONG)&commonExtension->HibernationPathCount,
                                !irpStack->Parameters.UsageNotification.InPath
                                );
                        }

                        break;
                    }

                    case DeviceUsageTypeDumpFile: {
                        IoAdjustPagingPathCount(
                            (PLONG)&commonExtension->DumpPathCount,
                            irpStack->Parameters.UsageNotification.InPath
                            );
                        status = ClassForwardIrpSynchronous(commonExtension, Irp);
                        if (!NT_SUCCESS(status)) {
                            IoAdjustPagingPathCount(
                                (PLONG)&commonExtension->DumpPathCount,
                                !irpStack->Parameters.UsageNotification.InPath
                                );
                        }

                        break;
                    }

                    default: {
                        status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                }
                break;
            }

            case IRP_MN_QUERY_CAPABILITIES: {

                DebugPrint((2, "ClassDispatchPnp (%p,%p): QueryCapabilities\n",
                            DeviceObject, Irp));

                if(!isFdo) {

                    status = ClassQueryPnpCapabilities(
                                DeviceObject,
                                irpStack->Parameters.DeviceCapabilities.Capabilities
                                );

                    break;

                } else {

                    PDEVICE_CAPABILITIES deviceCapabilities;
                    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
                    PCLASS_PRIVATE_FDO_DATA fdoData;

                    fdoExtension = DeviceObject->DeviceExtension;
                    fdoData = fdoExtension->PrivateFdoData;
                    deviceCapabilities =
                        irpStack->Parameters.DeviceCapabilities.Capabilities;

                    //
                    // forward the irp before handling the special cases
                    //

                    status = ClassForwardIrpSynchronous(commonExtension, Irp);
                    if (!NT_SUCCESS(status)) {
                        break;
                    }

                    //
                    // we generally want to remove the device from the hotplug
                    // applet, which requires the SR-OK bit to be set.
                    // only when the user specifies that they are capable of
                    // safely removing things do we want to clear this bit
                    // (saved in WriteCacheEnableOverride)
                    //
                    // setting of this bit is done either above, or by the
                    // lower driver.
                    //
                    // note: may not be started, so check we have FDO data first.
                    //

                    if (fdoData &&
                        fdoData->HotplugInfo.WriteCacheEnableOverride) {
                        if (deviceCapabilities->SurpriseRemovalOK) {
                            DebugPrint((1, "Classpnp: Clearing SR-OK bit in "
                                        "device capabilities due to hotplug "
                                        "device or media\n"));
                        }
                        deviceCapabilities->SurpriseRemovalOK = FALSE;
                    }
                    break;

                } // end QUERY_CAPABILITIES for FDOs

                ASSERT(FALSE);
                break;


            } // end QUERY_CAPABILITIES

            default: {

                if (isFdo){
                    IoCopyCurrentIrpStackLocationToNext(Irp);

                    ClassReleaseRemoveLock(DeviceObject, Irp);
                    status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);

                    completeRequest = FALSE;
                }

                break;
            }
        }
    }
    else {
        ASSERT(driverExtension);
        status = STATUS_INTERNAL_ERROR;
    }

    if (completeRequest){
        Irp->IoStatus.Status = status;

        if (!lockReleased){
            ClassReleaseRemoveLock(DeviceObject, Irp);
        }

        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

        DBGTRACE(ClassDebugTrace, ("ClassDispatchPnp (%p,%p): leaving with previous %#x, current %#x.", DeviceObject, Irp, commonExtension->PreviousState, commonExtension->CurrentState));
    }
    else {
        /*
         *  The irp is already completed so don't touch it.
         *  This may be a remove so don't touch the device extension.
         */
        DBGTRACE(ClassDebugTrace, ("ClassDispatchPnp (%p,%p): leaving.", DeviceObject, Irp));
    }

    return status;
} // end ClassDispatchPnp()

/*++////////////////////////////////////////////////////////////////////////////

ClassPnpStartDevice()

Routine Description:

    Storage class driver routine for IRP_MN_START_DEVICE requests.
    This routine kicks off any device specific initialization

Arguments:

    DeviceObject - a pointer to the device object

    Irp - a pointer to the io request packet

Return Value:

    none

--*/
NTSTATUS NTAPI ClassPnpStartDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PCLASS_DRIVER_EXTENSION driverExtension;
    PCLASS_INIT_DATA initData;

    PCLASS_DEV_INFO devInfo;

    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    BOOLEAN isFdo = commonExtension->IsFdo;

    BOOLEAN isMountedDevice = TRUE;
    //UNICODE_STRING  interfaceName;

    BOOLEAN timerStarted;

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    driverExtension = IoGetDriverObjectExtension(DeviceObject->DriverObject,
                                                 CLASS_DRIVER_EXTENSION_KEY);

    initData = &(driverExtension->InitData);
    if(isFdo) {
        devInfo = &(initData->FdoData);
    } else {
        devInfo = &(initData->PdoData);
    }

    ASSERT(devInfo->ClassInitDevice != NULL);
    ASSERT(devInfo->ClassStartDevice != NULL);

    if (!commonExtension->IsInitialized){

        //
        // perform FDO/PDO specific initialization
        //

        if (isFdo){
            STORAGE_PROPERTY_ID propertyId;

            //
            // allocate a private extension for class data
            //

            if (fdoExtension->PrivateFdoData == NULL) {
                fdoExtension->PrivateFdoData =
                    ExAllocatePoolWithTag(NonPagedPool,
                                          sizeof(CLASS_PRIVATE_FDO_DATA),
                                          CLASS_TAG_PRIVATE_DATA
                                          );
            }

            if (fdoExtension->PrivateFdoData == NULL) {
                DebugPrint((0, "ClassPnpStartDevice: Cannot allocate for "
                            "private fdo data\n"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // initialize the struct's various fields.
            //

            RtlZeroMemory(fdoExtension->PrivateFdoData,
                          sizeof(CLASS_PRIVATE_FDO_DATA)
                          );
            KeInitializeTimer(&fdoExtension->PrivateFdoData->Retry.Timer);
            KeInitializeDpc(&fdoExtension->PrivateFdoData->Retry.Dpc,
                            ClasspRetryRequestDpc,
                            DeviceObject);
            KeInitializeSpinLock(&fdoExtension->PrivateFdoData->Retry.Lock);
            fdoExtension->PrivateFdoData->Retry.Granularity =
                KeQueryTimeIncrement();
            commonExtension->Reserved4 = (ULONG_PTR)(' GPH'); // debug aid

            //
            // NOTE: the old interface allowed the class driver to allocate
            // this.  this was unsafe for low-memory conditions. allocate one
            // unconditionally now, and modify our internal functions to use
            // our own exclusively as it is the only safe way to do this.
            //

            status = ClasspAllocateReleaseQueueIrp(fdoExtension);
            if (!NT_SUCCESS(status)) {
                DebugPrint((0, "ClassPnpStartDevice: Cannot allocate the "
                            "private release queue irp\n"));
                return status;
            }

            //
            // Call port driver to get adapter capabilities.
            //

            propertyId = StorageAdapterProperty;

            status = ClassGetDescriptor(
                        commonExtension->LowerDeviceObject,
                        &propertyId,
                        (PSTORAGE_DESCRIPTOR_HEADER *)&fdoExtension->AdapterDescriptor);

            if(!NT_SUCCESS(status)) {

                //
                // This DebugPrint is to help third-party driver writers
                //

                DebugPrint((0, "ClassPnpStartDevice: ClassGetDescriptor "
                               "[ADAPTER] failed %lx\n", status));
                return status;
            }

            //
            // Call port driver to get device descriptor.
            //

            propertyId = StorageDeviceProperty;

            status = ClassGetDescriptor(
                        commonExtension->LowerDeviceObject,
                        &propertyId,
                        (PSTORAGE_DESCRIPTOR_HEADER *)&fdoExtension->DeviceDescriptor);

            if(!NT_SUCCESS(status)) {

                //
                // This DebugPrint is to help third-party driver writers
                //

                DebugPrint((0, "ClassPnpStartDevice: ClassGetDescriptor "
                               "[DEVICE] failed %lx\n", status));
                return status;
            }

            ClasspScanForSpecialInRegistry(fdoExtension);
            ClassScanForSpecial(fdoExtension,
                                ClassBadItems,
                                ClasspScanForClassHacks);

            //
            // allow perf to be re-enabled after a given number of failed IOs
            // require this number to be at least CLASS_PERF_RESTORE_MINIMUM
            //

            {
                ULONG t = 0;
                ClassGetDeviceParameter(fdoExtension,
                                        CLASSP_REG_SUBKEY_NAME,
                                        CLASSP_REG_PERF_RESTORE_VALUE_NAME,
                                        &t);
                if (t >= CLASS_PERF_RESTORE_MINIMUM) {
                    fdoExtension->PrivateFdoData->Perf.ReEnableThreshold = t;
                }
            }


            //
            // compatibility comes first.  writable cd media will not
            // get a SYNCH_CACHE on power down.
            //

            if (fdoExtension->DeviceObject->DeviceType != FILE_DEVICE_DISK) {
                SET_FLAG(fdoExtension->PrivateFdoData->HackFlags,
                         FDO_HACK_NO_SYNC_CACHE);
            }

            //
            // initialize the hotplug information only after the ScanForSpecial
            // routines, as it relies upon the hack flags.
            //

            status = ClasspInitializeHotplugInfo(fdoExtension);

            if (!NT_SUCCESS(status)) {
                DebugPrint((1, "ClassPnpStartDevice: Could not initialize "
                            "hotplug information %lx\n", status));
                return status;
            }

            /*
             *  Allocate/initialize TRANSFER_PACKETs and related resources.
             */
            status = InitializeTransferPackets(DeviceObject);
        }

        //
        // ISSUE - drivers need to disable write caching on the media
        //         if hotplug and !useroverride.  perhaps we should
        //         allow registration of a callback to enable/disable
        //         write cache instead.
        //

        if (NT_SUCCESS(status)){
            status = devInfo->ClassInitDevice(DeviceObject);
        }

    }

    if (!NT_SUCCESS(status)){

        //
        // Just bail out - the remove that comes down will clean up the
        // initialized scraps.
        //

        return status;
    } else {
        commonExtension->IsInitialized = TRUE;

        if (commonExtension->IsFdo) {
            fdoExtension->PrivateFdoData->Perf.OriginalSrbFlags = fdoExtension->SrbFlags;
        }

    }

    //
    // If device requests autorun functionality or a once a second callback
    // then enable the once per second timer.
    //
    // NOTE: This assumes that ClassInitializeMediaChangeDetection is always
    //       called in the context of the ClassInitDevice callback. If called
    //       after then this check will have already been made and the
    //       once a second timer will not have been enabled.
    //
    if ((isFdo) &&
        ((initData->ClassTick != NULL) ||
         (fdoExtension->MediaChangeDetectionInfo != NULL) ||
         ((fdoExtension->FailurePredictionInfo != NULL) &&
          (fdoExtension->FailurePredictionInfo->Method != FailurePredictionNone))))
    {
        ClasspEnableTimer(DeviceObject);
        timerStarted = TRUE;
    } else {
        timerStarted = FALSE;
    }

    //
    // NOTE: the timer looks at commonExtension->CurrentState now
    //       to prevent Media Change Notification code from running
    //       until the device is started, but allows the device
    //       specific tick handler to run.  therefore it is imperative
    //       that commonExtension->CurrentState not be updated until
    //       the device specific startdevice handler has finished.
    //

    status = devInfo->ClassStartDevice(DeviceObject);

    if(NT_SUCCESS(status)) {
        commonExtension->CurrentState = IRP_MN_START_DEVICE;

        if((isFdo) && (initData->ClassEnumerateDevice != NULL)) {
            isMountedDevice = FALSE;
        }

        if((DeviceObject->DeviceType != FILE_DEVICE_DISK) &&
           (DeviceObject->DeviceType != FILE_DEVICE_CD_ROM)) {

            isMountedDevice = FALSE;
        }


        if(isMountedDevice) {
            ClasspRegisterMountedDeviceInterface(DeviceObject);
        }

        if((commonExtension->IsFdo) &&
           (devInfo->ClassWmiInfo.GuidRegInfo != NULL)) {

            IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_REGISTER);
        }
    } else {

        if (timerStarted) {
            ClasspDisableTimer(DeviceObject);
        }
    }

    return status;
}


/*++////////////////////////////////////////////////////////////////////////////

ClassReadWrite()

Routine Description:

    This is the system entry point for read and write requests. The
    device-specific handler is invoked to perform any validation necessary.

    If the device object is a PDO (partition object) then the request will
    simply be adjusted for Partition0 and issued to the lower device driver.

    IF the device object is an FDO (partition 0 object), the number of bytes
    in the request are checked against the maximum byte counts that the adapter
    supports and requests are broken up into
    smaller sizes if necessary.

Arguments:

    DeviceObject - a pointer to the device object for this request

    Irp - IO request

Return Value:

    NT Status

--*/
NTSTATUS NTAPI ClassReadWrite(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT      lowerDeviceObject = commonExtension->LowerDeviceObject;
    PIO_STACK_LOCATION  currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    //LARGE_INTEGER       startingOffset = currentIrpStack->Parameters.Read.ByteOffset;
    ULONG               transferByteCount = currentIrpStack->Parameters.Read.Length;
    ULONG               isRemoved;
    NTSTATUS            status;

    /*
     *  Grab the remove lock.  If we can't acquire it, bail out.
     */
    isRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);
    if (isRemoved) {
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        ClassReleaseRemoveLock(DeviceObject, Irp);
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
        status = STATUS_DEVICE_DOES_NOT_EXIST;
    }
    else if (TEST_FLAG(DeviceObject->Flags, DO_VERIFY_VOLUME) &&
             (currentIrpStack->MinorFunction != CLASSP_VOLUME_VERIFY_CHECKED) &&
             !TEST_FLAG(currentIrpStack->Flags, SL_OVERRIDE_VERIFY_VOLUME)){

        /*
         *  DO_VERIFY_VOLUME is set for the device object,
         *  but this request is not itself a verify request.
         *  So fail this request.
         */
        IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
        Irp->IoStatus.Status = STATUS_VERIFY_REQUIRED;
        Irp->IoStatus.Information = 0;
        ClassReleaseRemoveLock(DeviceObject, Irp);
        ClassCompleteRequest(DeviceObject, Irp, 0);
        status = STATUS_VERIFY_REQUIRED;
    }
    else {

        /*
         *  Since we've bypassed the verify-required tests we don't need to repeat
         *  them with this IRP - in particular we don't want to worry about
         *  hitting them at the partition 0 level if the request has come through
         *  a non-zero partition.
         */
        currentIrpStack->MinorFunction = CLASSP_VOLUME_VERIFY_CHECKED;

        /*
         *  Call the miniport driver's pre-pass filter to check if we
         *  should continue with this transfer.
         */
        ASSERT(commonExtension->DevInfo->ClassReadWriteVerification);
        status = commonExtension->DevInfo->ClassReadWriteVerification(DeviceObject, Irp);
        if (!NT_SUCCESS(status)){
            ASSERT(Irp->IoStatus.Status == status);
            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest (DeviceObject, Irp, IO_NO_INCREMENT);
        }
        else if (status == STATUS_PENDING){
            /*
             *  ClassReadWriteVerification queued this request.
             *  So don't touch the irp anymore.
             */
        }
        else {

            if (transferByteCount == 0) {
                /*
                 *  Several parts of the code turn 0 into 0xffffffff,
                 *  so don't process a zero-length request any further.
                 */
                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = 0;
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
                status = STATUS_SUCCESS;
            }
            else {
                /*
                 *  If the driver has its own StartIo routine, call it.
                 */
                if (commonExtension->DriverExtension->InitData.ClassStartIo) {
                    IoMarkIrpPending(Irp);
                    IoStartPacket(DeviceObject, Irp, NULL, NULL);
                    status = STATUS_PENDING;
                }
                else {
                    /*
                     *  The driver does not have its own StartIo routine.
                     *  So process this request ourselves.
                     */

                    /*
                     *  Add partition byte offset to make starting byte relative to
                     *  beginning of disk.
                     */
                    currentIrpStack->Parameters.Read.ByteOffset.QuadPart +=
                        commonExtension->StartingOffset.QuadPart;

                    if (commonExtension->IsFdo){

                        /*
                         *  Add in any skew for the disk manager software.
                         */
                        currentIrpStack->Parameters.Read.ByteOffset.QuadPart +=
                             commonExtension->PartitionZeroExtension->DMByteSkew;

                        /*
                         *  Perform the actual transfer(s) on the hardware
                         *  to service this request.
                         */
                        ServiceTransferRequest(DeviceObject, Irp);
                        status = STATUS_PENDING;
                    }
                    else {
                        /*
                         *  This is a child PDO enumerated for our FDO by e.g. disk.sys
                         *  and owned by e.g. partmgr.  Send it down to the next device
                         *  and the same irp will come back to us for the FDO.
                         */
                        IoCopyCurrentIrpStackLocationToNext(Irp);
                        ClassReleaseRemoveLock(DeviceObject, Irp);
                        status = IoCallDriver(lowerDeviceObject, Irp);
                    }
                }
            }
        }
    }

    return status;
}


/*++////////////////////////////////////////////////////////////////////////////

ClassReadDriveCapacity()

Routine Description:

    This routine sends a READ CAPACITY to the requested device, updates
    the geometry information in the device object and returns
    when it is complete.  This routine is synchronous.

    This routine must be called with the remove lock held or some other
    assurance that the Fdo will not be removed while processing.

Arguments:

    DeviceObject - Supplies a pointer to the device object that represents
        the device whose capacity is to be read.

Return Value:

    Status is returned.

--*/
NTSTATUS NTAPI ClassReadDriveCapacity(IN PDEVICE_OBJECT Fdo)
{
    READ_CAPACITY_DATA readCapacityBuffer = {0};
    NTSTATUS status;
    PMDL driveCapMdl;

    driveCapMdl = BuildDeviceInputMdl(&readCapacityBuffer, sizeof(READ_CAPACITY_DATA));
    if (driveCapMdl){

        TRANSFER_PACKET *pkt = DequeueFreeTransferPacket(Fdo, TRUE);
        if (pkt){
            PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
            KEVENT event;
            //NTSTATUS pktStatus;
            IRP pseudoIrp = {0};

            /*
             *  Our engine needs an "original irp" to write the status back to
             *  and to count down packets (one in this case).
             *  Just use a pretend irp for this.
             */
            pseudoIrp.Tail.Overlay.DriverContext[0] = LongToPtr(1);
            pseudoIrp.IoStatus.Status = STATUS_SUCCESS;
            pseudoIrp.IoStatus.Information = 0;
            pseudoIrp.MdlAddress = driveCapMdl;

            /*
             *  Set this up as a SYNCHRONOUS transfer, submit it,
             *  and wait for the packet to complete.  The result
             *  status will be written to the original irp.
             */
            KeInitializeEvent(&event, SynchronizationEvent, FALSE);
            SetupDriveCapacityTransferPacket(   pkt,
                                            &readCapacityBuffer,
                                            sizeof(READ_CAPACITY_DATA),
                                            &event,
                                            &pseudoIrp);
            SubmitTransferPacket(pkt);
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

            status = pseudoIrp.IoStatus.Status;

            /*
             *  If we got an UNDERRUN, retry exactly once.
             *  (The transfer_packet engine didn't retry because the result
             *   status was success).
             */
            if (NT_SUCCESS(status) &&
               (pseudoIrp.IoStatus.Information < sizeof(READ_CAPACITY_DATA))){
                DBGERR(("ClassReadDriveCapacity: read len (%xh) < %xh, retrying ...", (ULONG)pseudoIrp.IoStatus.Information, sizeof(READ_CAPACITY_DATA)));

                pkt = DequeueFreeTransferPacket(Fdo, TRUE);
                if (pkt){
                    pseudoIrp.Tail.Overlay.DriverContext[0] = LongToPtr(1);
                    pseudoIrp.IoStatus.Status = STATUS_SUCCESS;
                    pseudoIrp.IoStatus.Information = 0;
                    KeInitializeEvent(&event, SynchronizationEvent, FALSE);
                    SetupDriveCapacityTransferPacket(   pkt,
                                                    &readCapacityBuffer,
                                                    sizeof(READ_CAPACITY_DATA),
                                                    &event,
                                                    &pseudoIrp);
                    SubmitTransferPacket(pkt);
                    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                    status = pseudoIrp.IoStatus.Status;
                    if (pseudoIrp.IoStatus.Information < sizeof(READ_CAPACITY_DATA)){
                        status = STATUS_DEVICE_BUSY;
                    }
                }
                else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }


            if (NT_SUCCESS(status)){
                /*
                 *  The request succeeded.
                 *  Read out and store the drive information.
                 */
                ULONG cylinderSize;
                ULONG bytesPerSector;
                ULONG tmp;
                ULONG lastSector;

                /*
                 *  Read the bytesPerSector value,
                 *  which is big-endian in the returned buffer.
                 *  Default to the standard 512 bytes.
                 */
                tmp = readCapacityBuffer.BytesPerBlock;
                ((PFOUR_BYTE)&bytesPerSector)->Byte0 = ((PFOUR_BYTE)&tmp)->Byte3;
                ((PFOUR_BYTE)&bytesPerSector)->Byte1 = ((PFOUR_BYTE)&tmp)->Byte2;
                ((PFOUR_BYTE)&bytesPerSector)->Byte2 = ((PFOUR_BYTE)&tmp)->Byte1;
                ((PFOUR_BYTE)&bytesPerSector)->Byte3 = ((PFOUR_BYTE)&tmp)->Byte0;
                if (bytesPerSector == 0) {
                    bytesPerSector = 512;
                }
                else {
                    /*
                     *  Clear all but the highest set bit.
                     *  That will give us a bytesPerSector value that is a power of 2.
                     */
                    while (bytesPerSector & (bytesPerSector-1)) {
                        bytesPerSector &= bytesPerSector-1;
                    }
                }
                fdoExt->DiskGeometry.BytesPerSector = bytesPerSector;

                //
                // Copy last sector in reverse byte order.
                //

                tmp = readCapacityBuffer.LogicalBlockAddress;
                ((PFOUR_BYTE)&lastSector)->Byte0 = ((PFOUR_BYTE)&tmp)->Byte3;
                ((PFOUR_BYTE)&lastSector)->Byte1 = ((PFOUR_BYTE)&tmp)->Byte2;
                ((PFOUR_BYTE)&lastSector)->Byte2 = ((PFOUR_BYTE)&tmp)->Byte1;
                ((PFOUR_BYTE)&lastSector)->Byte3 = ((PFOUR_BYTE)&tmp)->Byte0;

                //
                // Calculate sector to byte shift.
                //

                WHICH_BIT(fdoExt->DiskGeometry.BytesPerSector, fdoExt->SectorShift);

                DebugPrint((2,"SCSI ClassReadDriveCapacity: Sector size is %d\n",
                    fdoExt->DiskGeometry.BytesPerSector));

                DebugPrint((2,"SCSI ClassReadDriveCapacity: Number of Sectors is %d\n",
                    lastSector + 1));

                if (fdoExt->DMActive){
                    DebugPrint((1, "SCSI ClassReadDriveCapacity: reducing number of sectors by %d\n",
                                fdoExt->DMSkew));
                    lastSector -= fdoExt->DMSkew;
                }

                /*
                 *  Check to see if we have a geometry we should be using already.
                 */
                cylinderSize = (fdoExt->DiskGeometry.TracksPerCylinder *
                                fdoExt->DiskGeometry.SectorsPerTrack);
                if (cylinderSize == 0){
                    DebugPrint((1, "ClassReadDriveCapacity: resetting H & S geometry "
                                   "values from %#x/%#x to %#x/%#x\n",
                                fdoExt->DiskGeometry.TracksPerCylinder,
                                fdoExt->DiskGeometry.SectorsPerTrack,
                                0xff,
                                0x3f));

                    fdoExt->DiskGeometry.TracksPerCylinder = 0xff;
                    fdoExt->DiskGeometry.SectorsPerTrack = 0x3f;


                    cylinderSize = (fdoExt->DiskGeometry.TracksPerCylinder *
                                    fdoExt->DiskGeometry.SectorsPerTrack);
                }

                //
                // Calculate number of cylinders.
                //

                fdoExt->DiskGeometry.Cylinders.QuadPart = (LONGLONG)((lastSector + 1)/cylinderSize);

                //
                // if there are zero cylinders, then the device lied AND it's
                // smaller than 0xff*0x3f (about 16k sectors, usually 8 meg)
                // this can fit into a single LONGLONG, so create another usable
                // geometry, even if it's unusual looking.  This allows small,
                // non-standard devices, such as Sony's Memory Stick, to show
                // up as having a partition.
                //

                if (fdoExt->DiskGeometry.Cylinders.QuadPart == (LONGLONG)0) {
                    fdoExt->DiskGeometry.SectorsPerTrack    = 1;
                    fdoExt->DiskGeometry.TracksPerCylinder  = 1;
                    fdoExt->DiskGeometry.Cylinders.QuadPart = lastSector;
                }


                //
                // Calculate media capacity in bytes.
                //

                fdoExt->CommonExtension.PartitionLength.QuadPart =
                    ((LONGLONG)(lastSector + 1)) << fdoExt->SectorShift;

                /*
                 *  Is this removable or fixed media
                 */
                if (TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA)){
                    fdoExt->DiskGeometry.MediaType = RemovableMedia;
                }
                else {
                    fdoExt->DiskGeometry.MediaType = FixedMedia;
                }
            }
            else {
                /*
                 *  The request failed.
                 */

                //
                // ISSUE - 2000/02/04 - henrygab - non-512-byte sector sizes and failed geometry update
                //    what happens when the disk's sector size is bigger than
                //    512 bytes and we hit this code path?  this is untested.
                //
                // If the read capacity fails, set the geometry to reasonable parameter
                // so things don't fail at unexpected places.  Zero the geometry
                // except for the bytes per sector and sector shift.
                //

                /*
                 *  This request can sometimes fail legitimately
                 *  (e.g. when a SCSI device is attached but turned off)
                 *  so this is not necessarily a device/driver bug.
                 */
                DBGTRACE(ClassDebugWarning, ("ClassReadDriveCapacity on Fdo %xh failed with status %xh.", Fdo, status));

                /*
                 *  Write in a default disk geometry which we HOPE is right (??).
                 *      BUGBUG !!
                 */
                RtlZeroMemory(&fdoExt->DiskGeometry, sizeof(DISK_GEOMETRY));
                fdoExt->DiskGeometry.BytesPerSector = 512;
                fdoExt->SectorShift = 9;
                fdoExt->CommonExtension.PartitionLength.QuadPart = (LONGLONG) 0;

                /*
                 *  Is this removable or fixed media
                 */
                if (TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA)){
                    fdoExt->DiskGeometry.MediaType = RemovableMedia;
                }
                else {
                    fdoExt->DiskGeometry.MediaType = FixedMedia;
                }
            }

        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        FreeDeviceInputMdl(driveCapMdl);
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


/*++////////////////////////////////////////////////////////////////////////////

ClassSendStartUnit()

Routine Description:

    Send command to SCSI unit to start or power up.
    Because this command is issued asynchronously, that is, without
    waiting on it to complete, the IMMEDIATE flag is not set. This
    means that the CDB will not return until the drive has powered up.
    This should keep subsequent requests from being submitted to the
    device before it has completely spun up.

    This routine is called from the InterpretSense routine, when a
    request sense returns data indicating that a drive must be
    powered up.

    This routine may also be called from a class driver's error handler,
    or anytime a non-critical start device should be sent to the device.

Arguments:

    Fdo - The functional device object for the stopped device.

Return Value:

    None.

--*/
VOID
NTAPI
ClassSendStartUnit(
    IN PDEVICE_OBJECT Fdo
    )
{
    PIO_STACK_LOCATION irpStack;
    PIRP irp;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PSCSI_REQUEST_BLOCK srb;
    PCOMPLETION_CONTEXT context;
    PCDB cdb;

    //
    // Allocate Srb from nonpaged pool.
    //

    context = ExAllocatePoolWithTag(NonPagedPool,
                             sizeof(COMPLETION_CONTEXT),
                             '6CcS');

    if(context == NULL) {

        //
        // ISSUE-2000/02/03-peterwie
        // This code path was inherited from the NT 4.0 class2.sys driver.
        // It needs to be changed to survive low-memory conditions.
        //

        KeBugCheck(SCSI_DISK_DRIVER_INTERNAL);
    }

    //
    // Save the device object in the context for use by the completion
    // routine.
    //

    context->DeviceObject = Fdo;
    srb = &context->Srb;

    //
    // Zero out srb.
    //

    RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Write length to SRB.
    //

    srb->Length = sizeof(SCSI_REQUEST_BLOCK);

    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

    //
    // Set timeout value large enough for drive to spin up.
    //

    srb->TimeOutValue = START_UNIT_TIMEOUT;

    //
    // Set the transfer length.
    //

    srb->SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER |
                    SRB_FLAGS_DISABLE_AUTOSENSE |
                    SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    //
    // Build the start unit CDB.
    //

    srb->CdbLength = 6;
    cdb = (PCDB)srb->Cdb;

    cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
    cdb->START_STOP.Start = 1;
    cdb->START_STOP.Immediate = 0;
    cdb->START_STOP.LogicalUnitNumber = srb->Lun;

    //
    // Build the asynchronous request to be sent to the port driver.
    // Since this routine is called from a DPC the IRP should always be
    // available.
    //

    irp = IoAllocateIrp(Fdo->StackSize, FALSE);

    if(irp == NULL) {

        //
        // ISSUE-2000/02/03-peterwie
        // This code path was inherited from the NT 4.0 class2.sys driver.
        // It needs to be changed to survive low-memory conditions.
        //

        KeBugCheck(SCSI_DISK_DRIVER_INTERNAL);

    }

    ClassAcquireRemoveLock(Fdo, irp);

    IoSetCompletionRoutine(irp,
                           ClassAsynchronousCompletion,
                           context,
                           TRUE,
                           TRUE,
                           TRUE);

    irpStack = IoGetNextIrpStackLocation(irp);
    irpStack->MajorFunction = IRP_MJ_SCSI;
    srb->OriginalRequest = irp;

    //
    // Store the SRB address in next stack for port driver.
    //

    irpStack->Parameters.Scsi.Srb = srb;

    //
    // Call the port driver with the IRP.
    //

    IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp);

    return;

} // end StartUnit()

/*++////////////////////////////////////////////////////////////////////////////

ClassAsynchronousCompletion() ISSUE-2000/02/18-henrygab - why public?!

Routine Description:

    This routine is called when an asynchronous I/O request
    which was issued by the class driver completes.  Examples of such requests
    are release queue or START UNIT. This routine releases the queue if
    necessary.  It then frees the context and the IRP.

Arguments:

    DeviceObject - The device object for the logical unit; however since this
        is the top stack location the value is NULL.

    Irp - Supplies a pointer to the Irp to be processed.

    Context - Supplies the context to be used to process this request.

Return Value:

    None.

--*/
NTSTATUS
NTAPI
ClassAsynchronousCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    PCOMPLETION_CONTEXT context = Context;
    PSCSI_REQUEST_BLOCK srb;

    if(DeviceObject == NULL) {

        DeviceObject = context->DeviceObject;
    }

    srb = &context->Srb;

    //
    // If this is an execute srb, then check the return status and make sure.
    // the queue is not frozen.
    //

    if (srb->Function == SRB_FUNCTION_EXECUTE_SCSI) {

        //
        // Check for a frozen queue.
        //

        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {

            //
            // Unfreeze the queue getting the device object from the context.
            //

            ClassReleaseQueue(context->DeviceObject);
        }
    }

    { // free port-allocated sense buffer if we can detect

        if (((PCOMMON_DEVICE_EXTENSION)(DeviceObject->DeviceExtension))->IsFdo) {

            PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
            if (PORT_ALLOCATED_SENSE(fdoExtension, srb)) {
                FREE_PORT_ALLOCATED_SENSE_BUFFER(fdoExtension, srb);
            }

        } else {

            ASSERT(!TEST_FLAG(srb->SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));

        }
    }


    //
    // Free the context and the Irp.
    //

    if (Irp->MdlAddress != NULL) {
        MmUnlockPages(Irp->MdlAddress);
        IoFreeMdl(Irp->MdlAddress);

        Irp->MdlAddress = NULL;
    }

    ClassReleaseRemoveLock(DeviceObject, Irp);

    ExFreePool(context);
    IoFreeIrp(Irp);

    //
    // Indicate the I/O system should stop processing the Irp completion.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

} // end ClassAsynchronousCompletion()

VOID NTAPI ServiceTransferRequest(PDEVICE_OBJECT Fdo, PIRP Irp)
{
    //PCOMMON_DEVICE_EXTENSION commonExt = Fdo->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    //PSTORAGE_ADAPTER_DESCRIPTOR adapterDesc = commonExt->PartitionZeroExtension->AdapterDescriptor;
    PIO_STACK_LOCATION currentSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG entireXferLen = currentSp->Parameters.Read.Length;
    PUCHAR bufPtr = MmGetMdlVirtualAddress(Irp->MdlAddress);
    LARGE_INTEGER targetLocation = currentSp->Parameters.Read.ByteOffset;
    PTRANSFER_PACKET pkt;
    SLIST_ENTRY pktList;
    PSLIST_ENTRY slistEntry;
    ULONG numPackets;
    //KIRQL oldIrql;
    ULONG i;

    /*
     *  Compute the number of hw xfers we'll have to do.
     *  Calculate this without allowing for an overflow condition.
     */
    ASSERT(fdoData->HwMaxXferLen >= PAGE_SIZE);
    numPackets = entireXferLen/fdoData->HwMaxXferLen;
    if (entireXferLen % fdoData->HwMaxXferLen){
        numPackets++;
    }

    /*
     *  First get all the TRANSFER_PACKETs that we'll need at once.
     *  Use our 'simple' slist functions since we don't need interlocked.
     */
    SimpleInitSlistHdr(&pktList);
    for (i = 0; i < numPackets; i++){
        pkt = DequeueFreeTransferPacket(Fdo, TRUE);
        if (pkt){
            SimplePushSlist(&pktList, &pkt->SlistEntry);
        }
        else {
            break;
        }
    }

    if (i == numPackets){
        /*
         *  Initialize the original IRP's status to success.
         *  If any of the packets fail, they will set it to an error status.
         *  The IoStatus.Information field will be incremented to the
         *  transfer length as the pieces complete.
         */
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        /*
         *  Store the number of transfer pieces inside the original IRP.
         *  It will be used to count down the pieces as they complete.
         */
        Irp->Tail.Overlay.DriverContext[0] = LongToPtr(numPackets);

        /*
         *  We are proceeding with the transfer.
         *  Mark the client IRP pending since it may complete on a different thread.
         */
        IoMarkIrpPending(Irp);

        /*
         *  Transmit the pieces of the transfer.
         */
        while (entireXferLen > 0){
            ULONG thisPieceLen = MIN(fdoData->HwMaxXferLen, entireXferLen);

            /*
             *  Set up a TRANSFER_PACKET for this piece and send it.
             */
            slistEntry = SimplePopSlist(&pktList);
            ASSERT(slistEntry);
            pkt = CONTAINING_RECORD(slistEntry, TRANSFER_PACKET, SlistEntry);
            SetupReadWriteTransferPacket(   pkt,
                                        bufPtr,
                                        thisPieceLen,
                                        targetLocation,
                                        Irp);
            SubmitTransferPacket(pkt);

            entireXferLen -= thisPieceLen;
            bufPtr += thisPieceLen;
            targetLocation.QuadPart += thisPieceLen;
        }
        ASSERT(SimpleIsSlistEmpty(&pktList));
    }
    else if (i >= 1){
        /*
         *  We were unable to get all the TRANSFER_PACKETs we need,
         *  but we did get at least one.
         *  That means that we are in extreme low-memory stress.
         *  We'll try doing this transfer using a single packet.
         *  The port driver is certainly also in stress, so use one-page
         *  transfers.
         */

        /*
         *  Free all but one of the TRANSFER_PACKETs.
         */
        while (i-- > 1){
            slistEntry = SimplePopSlist(&pktList);
            ASSERT(slistEntry);
            pkt = CONTAINING_RECORD(slistEntry, TRANSFER_PACKET, SlistEntry);
            EnqueueFreeTransferPacket(Fdo, pkt);
        }

        /*
         *  Get the single TRANSFER_PACKET that we'll be using.
         */
        slistEntry = SimplePopSlist(&pktList);
        ASSERT(slistEntry);
        ASSERT(SimpleIsSlistEmpty(&pktList));
        pkt = CONTAINING_RECORD(slistEntry, TRANSFER_PACKET, SlistEntry);
        DBGWARN(("Insufficient packets available in ServiceTransferRequest - entering lowMemRetry with pkt=%xh.", pkt));

        /*
         *  Set default status and the number of transfer packets (one)
         *  inside the original irp.
         */
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        Irp->Tail.Overlay.DriverContext[0] = LongToPtr(1);

        /*
         *  Mark the client irp pending since it may complete on
         *  another thread.
         */
        IoMarkIrpPending(Irp);

        /*
         *  Set up the TRANSFER_PACKET for a lowMem transfer and launch.
         */
        SetupReadWriteTransferPacket(  pkt,
                                    bufPtr,
                                    entireXferLen,
                                    targetLocation,
                                    Irp);
        InitLowMemRetry(pkt, bufPtr, entireXferLen, targetLocation);
        StepLowMemRetry(pkt);
    }
    else {
        /*
         *  We were unable to get ANY TRANSFER_PACKETs.
         *  Defer this client irp until some TRANSFER_PACKETs free up.
         */
        DBGWARN(("No packets available in ServiceTransferRequest - deferring transfer (Irp=%xh)...", Irp));
        IoMarkIrpPending(Irp);
        EnqueueDeferredClientIrp(fdoData, Irp);
    }

}

/*++////////////////////////////////////////////////////////////////////////////

ClassIoComplete()

Routine Description:

    This routine executes when the port driver has completed a request.
    It looks at the SRB status in the completing SRB and if not success
    it checks for valid request sense buffer information. If valid, the
    info is used to update status with more precise message of type of
    error. This routine deallocates the SRB.

    This routine should only be placed on the stack location for a class
    driver FDO.

Arguments:

    Fdo - Supplies the device object which represents the logical
        unit.

    Irp - Supplies the Irp which has completed.

    Context - Supplies a pointer to the SRB.

Return Value:

    NT status

--*/
NTSTATUS
NTAPI
ClassIoComplete(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = Context;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;
    NTSTATUS status;
    BOOLEAN retry;
    BOOLEAN callStartNextPacket;

    ASSERT(fdoExtension->CommonExtension.IsFdo);

    //
    // Check SRB status for success of completing request.
    //

    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {
        ULONG retryInterval;

        DebugPrint((2, "ClassIoComplete: IRP %p, SRB %p\n", Irp, srb));

        //
        // Release the queue if it is frozen.
        //

        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            ClassReleaseQueue(Fdo);
        }

        retry = ClassInterpretSenseInfo(
                    Fdo,
                    srb,
                    irpStack->MajorFunction,
                    irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL ?
                     irpStack->Parameters.DeviceIoControl.IoControlCode :
                     0,
                    MAXIMUM_RETRIES -
                        ((ULONG)(ULONG_PTR)irpStack->Parameters.Others.Argument4),
                    &status,
                    &retryInterval);

        //
        // If the status is verified required and the this request
        // should bypass verify required then retry the request.
        //

        if (TEST_FLAG(irpStack->Flags, SL_OVERRIDE_VERIFY_VOLUME) &&
            status == STATUS_VERIFY_REQUIRED) {

            status = STATUS_IO_DEVICE_ERROR;
            retry = TRUE;
        }

        if (retry && ((*(PCHAR*)&irpStack->Parameters.Others.Argument4)--)) {

            //
            // Retry request.
            //

            DebugPrint((1, "Retry request %p\n", Irp));

            if (PORT_ALLOCATED_SENSE(fdoExtension, srb)) {
                FREE_PORT_ALLOCATED_SENSE_BUFFER(fdoExtension, srb);
            }

            RetryRequest(Fdo, Irp, srb, FALSE, retryInterval);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

    } else {

        //
        // Set status for successful request
        //
        fdoData->LoggedTURFailureSinceLastIO = FALSE;
        ClasspPerfIncrementSuccessfulIo(fdoExtension);
        status = STATUS_SUCCESS;
    } // end if (SRB_STATUS(srb->SrbStatus) == SRB_STATUS_SUCCESS)


    //
    // ensure we have returned some info, and it matches what the
    // original request wanted for PAGING operations only
    //

    if ((NT_SUCCESS(status)) && TEST_FLAG(Irp->Flags, IRP_PAGING_IO)) {
        ASSERT(Irp->IoStatus.Information != 0);
        ASSERT(irpStack->Parameters.Read.Length == Irp->IoStatus.Information);
    }

    //
    // remember if the caller wanted to skip calling IoStartNextPacket.
    // for legacy reasons, we cannot call IoStartNextPacket for IoDeviceControl
    // calls.  this setting only affects device objects with StartIo routines.
    //

    callStartNextPacket = !TEST_FLAG(srb->SrbFlags, SRB_FLAGS_DONT_START_NEXT_PACKET);
    if (irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
        callStartNextPacket = FALSE;
    }

    //
    // Free the srb
    //

    if(!TEST_FLAG(srb->SrbFlags, SRB_CLASS_FLAGS_PERSISTANT)) {

        if (PORT_ALLOCATED_SENSE(fdoExtension, srb)) {
            FREE_PORT_ALLOCATED_SENSE_BUFFER(fdoExtension, srb);
        }

        if (fdoExtension->CommonExtension.IsSrbLookasideListInitialized){
            ClassFreeOrReuseSrb(fdoExtension, srb);
        }
        else {
            DBGWARN(("ClassIoComplete is freeing an SRB (possibly) on behalf of another driver."));
            ExFreePool(srb);
        }

    } else {

        DebugPrint((2, "ClassIoComplete: Not Freeing srb @ %p because "
                    "SRB_CLASS_FLAGS_PERSISTANT set\n", srb));
        if (PORT_ALLOCATED_SENSE(fdoExtension, srb)) {
            DebugPrint((2, "ClassIoComplete: Not Freeing sensebuffer @ %p "
                        " because SRB_CLASS_FLAGS_PERSISTANT set\n",
                        srb->SenseInfoBuffer));
        }

    }

    //
    // Set status in completing IRP.
    //

    Irp->IoStatus.Status = status;

    //
    // Set the hard error if necessary.
    //

    if (!NT_SUCCESS(status) &&
        IoIsErrorUserInduced(status) &&
        (Irp->Tail.Overlay.Thread != NULL)
        ) {

        //
        // Store DeviceObject for filesystem, and clear
        // in IoStatus.Information field.
        //

        IoSetHardErrorOrVerifyDevice(Irp, Fdo);
        Irp->IoStatus.Information = 0;
    }

    //
    // If pending has be returned for this irp then mark the current stack as
    // pending.
    //

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    if (fdoExtension->CommonExtension.DriverExtension->InitData.ClassStartIo) {
        if (callStartNextPacket) {
            KIRQL oldIrql;
            KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
            IoStartNextPacket(Fdo, FALSE);
            KeLowerIrql(oldIrql);
        }
    }

    ClassReleaseRemoveLock(Fdo, Irp);

    return status;

} // end ClassIoComplete()

/*++////////////////////////////////////////////////////////////////////////////

ClassSendSrbSynchronous()

Routine Description:

    This routine is called by SCSI device controls to complete an
    SRB and send it to the port driver synchronously (ie wait for
    completion). The CDB is already completed along with the SRB CDB
    size and request timeout value.

Arguments:

    Fdo - Supplies the functional device object which represents the target.

    Srb - Supplies a partially initialized SRB. The SRB cannot come from zone.

    BufferAddress - Supplies the address of the buffer.

    BufferLength - Supplies the length in bytes of the buffer.

    WriteToDevice - Indicates the data should be transfer to the device.

Return Value:

    NTSTATUS indicating the final results of the operation.

    If NT_SUCCESS(), then the amount of usable data is contained in the field
       Srb->DataTransferLength

--*/
NTSTATUS
NTAPI
ClassSendSrbSynchronous(
    PDEVICE_OBJECT Fdo,
    PSCSI_REQUEST_BLOCK Srb,
    PVOID BufferAddress,
    ULONG BufferLength,
    BOOLEAN WriteToDevice
    )
{

    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;
    IO_STATUS_BLOCK ioStatus;
    //ULONG controlType;
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    KEVENT event;
    PUCHAR senseInfoBuffer;
    ULONG retryCount = MAXIMUM_RETRIES;
    NTSTATUS status;
    BOOLEAN retry;

    //
    // NOTE: This code is only pageable because we are not freezing
    //       the queue.  Allowing the queue to be frozen from a pageable
    //       routine could leave the queue frozen as we try to page in
    //       the code to unfreeze the queue.  The result would be a nice
    //       case of deadlock.  Therefore, since we are unfreezing the
    //       queue regardless of the result, just set the NO_FREEZE_QUEUE
    //       flag in the SRB.
    //

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    ASSERT(fdoExtension->CommonExtension.IsFdo);

    //
    // Write length to SRB.
    //

    Srb->Length = sizeof(SCSI_REQUEST_BLOCK);

    //
    // Set SCSI bus address.
    //

    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

    //
    // Enable auto request sense.
    //

    Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

    //
    // Sense buffer is in aligned nonpaged pool.
    //
        //
    senseInfoBuffer = ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                     SENSE_BUFFER_SIZE,
                                     '7CcS');

    if (senseInfoBuffer == NULL) {

        DebugPrint((1, "ClassSendSrbSynchronous: Can't allocate request sense "
                       "buffer\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    Srb->SenseInfoBuffer = senseInfoBuffer;
    Srb->DataBuffer = BufferAddress;

    //
    // Start retries here.
    //

retry:

    //
    // use fdoextension's flags by default.
    // do not move out of loop, as the flag may change due to errors
    // sending this command.
    //

    Srb->SrbFlags = fdoExtension->SrbFlags;

    if(BufferAddress != NULL) {
        if(WriteToDevice) {
            SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DATA_OUT);
        } else {
            SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DATA_IN);
        }
    }

    //
    // Initialize the QueueAction field.
    //

    Srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;

    //
    // Disable synchronous transfer for these requests.
    // Disable freezing the queue, since all we do is unfreeze it anyways.
    //

    SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
    SET_FLAG(Srb->SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE);

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Build device I/O control request with METHOD_NEITHER data transfer.
    // We'll queue a completion routine to cleanup the MDL's and such ourself.
    //

    irp = IoAllocateIrp(
            (CCHAR) (fdoExtension->CommonExtension.LowerDeviceObject->StackSize + 1),
            FALSE);

    if(irp == NULL) {
        ExFreePool(senseInfoBuffer);
        DebugPrint((1, "ClassSendSrbSynchronous: Can't allocate Irp\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Get next stack location.
    //

    irpStack = IoGetNextIrpStackLocation(irp);

    //
    // Set up SRB for execute scsi request. Save SRB address in next stack
    // for the port driver.
    //

    irpStack->MajorFunction = IRP_MJ_SCSI;
    irpStack->Parameters.Scsi.Srb = Srb;

    IoSetCompletionRoutine(irp,
                           ClasspSendSynchronousCompletion,
                           Srb,
                           TRUE,
                           TRUE,
                           TRUE);

    irp->UserIosb = &ioStatus;
    irp->UserEvent = &event;

    if(BufferAddress) {
        //
        // Build an MDL for the data buffer and stick it into the irp.  The
        // completion routine will unlock the pages and free the MDL.
        //

        irp->MdlAddress = IoAllocateMdl( BufferAddress,
                                         BufferLength,
                                         FALSE,
                                         FALSE,
                                         irp );
        if (irp->MdlAddress == NULL) {
            ExFreePool(senseInfoBuffer);
            Srb->SenseInfoBuffer = NULL;
            IoFreeIrp( irp );
            DebugPrint((1, "ClassSendSrbSynchronous: Can't allocate MDL\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        _SEH2_TRY {

            //
            // the io manager unlocks these pages upon completion
            //

            MmProbeAndLockPages( irp->MdlAddress,
                                 KernelMode,
                                 (WriteToDevice ? IoReadAccess :
                                                  IoWriteAccess));

        } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            status = _SEH2_GetExceptionCode();

            ExFreePool(senseInfoBuffer);
            Srb->SenseInfoBuffer = NULL;
            IoFreeMdl(irp->MdlAddress);
            IoFreeIrp(irp);

            DebugPrint((1, "ClassSendSrbSynchronous: Exception %lx "
                           "locking buffer\n", status));
            _SEH2_YIELD(return status);
        } _SEH2_END;
    }

    //
    // Set the transfer length.
    //

    Srb->DataTransferLength = BufferLength;

    //
    // Zero out status.
    //

    Srb->ScsiStatus = Srb->SrbStatus = 0;
    Srb->NextSrb = 0;

    //
    // Set up IRP Address.
    //

    Srb->OriginalRequest = irp;

    //
    // Call the port driver with the request and wait for it to complete.
    //

    status = IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    //
    // Check that request completed without error.
    //

    if (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        ULONG retryInterval;

        DBGTRACE(ClassDebugWarning, ("ClassSendSrbSynchronous - srb %ph failed (op=%s srbstat=%s(%xh), irpstat=%xh, sense=%s/%s/%s)", Srb, DBGGETSCSIOPSTR(Srb), DBGGETSRBSTATUSSTR(Srb), (ULONG)Srb->SrbStatus, status, DBGGETSENSECODESTR(Srb), DBGGETADSENSECODESTR(Srb), DBGGETADSENSEQUALIFIERSTR(Srb)));

        //
        // assert that the queue is not frozen
        //

        ASSERT(!TEST_FLAG(Srb->SrbStatus, SRB_STATUS_QUEUE_FROZEN));

        //
        // Update status and determine if request should be retried.
        //

        retry = ClassInterpretSenseInfo(Fdo,
                                        Srb,
                                        IRP_MJ_SCSI,
                                        0,
                                        MAXIMUM_RETRIES  - retryCount,
                                        &status,
                                        &retryInterval);


        if (retry) {

            if ((status == STATUS_DEVICE_NOT_READY &&
                 ((PSENSE_DATA) senseInfoBuffer)->AdditionalSenseCode ==
                                SCSI_ADSENSE_LUN_NOT_READY) ||
                (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SELECTION_TIMEOUT)) {

                LARGE_INTEGER delay;

                //
                // Delay for at least 2 seconds.
                //

                if(retryInterval < 2) {
                    retryInterval = 2;
                }

                delay.QuadPart = (LONGLONG)( - 10 * 1000 * (LONGLONG)1000 * retryInterval);

                //
                // Stall for a while to let the device become ready
                //

                KeDelayExecutionThread(KernelMode, FALSE, &delay);

            }

            //
            // If retries are not exhausted then retry this operation.
            //

            if (retryCount--) {

                if (PORT_ALLOCATED_SENSE(fdoExtension, Srb)) {
                    FREE_PORT_ALLOCATED_SENSE_BUFFER(fdoExtension, Srb);
                }

                goto retry;
            }
        }

    } else {
        fdoData->LoggedTURFailureSinceLastIO = FALSE;
        status = STATUS_SUCCESS;
    }

    //
    // required even though we allocated our own, since the port driver may
    // have allocated one also
    //

    if (PORT_ALLOCATED_SENSE(fdoExtension, Srb)) {
        FREE_PORT_ALLOCATED_SENSE_BUFFER(fdoExtension, Srb);
    }

    Srb->SenseInfoBuffer = NULL;
    ExFreePool(senseInfoBuffer);

    return status;
}

/*++////////////////////////////////////////////////////////////////////////////

ClassInterpretSenseInfo()

Routine Description:

    This routine interprets the data returned from the SCSI
    request sense. It determines the status to return in the
    IRP and whether this request can be retried.

Arguments:

    DeviceObject - Supplies the device object associated with this request.

    Srb - Supplies the scsi request block which failed.

    MajorFunctionCode - Supplies the function code to be used for logging.

    IoDeviceCode - Supplies the device code to be used for logging.

    Status - Returns the status for the request.

Return Value:

    BOOLEAN TRUE: Drivers should retry this request.
            FALSE: Drivers should not retry this request.

--*/
BOOLEAN
NTAPI
ClassInterpretSenseInfo(
    IN PDEVICE_OBJECT Fdo,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN UCHAR MajorFunctionCode,
    IN ULONG IoDeviceCode,
    IN ULONG RetryCount,
    OUT NTSTATUS *Status,
    OUT OPTIONAL ULONG *RetryInterval
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;

    PSENSE_DATA       senseBuffer = Srb->SenseInfoBuffer;

    BOOLEAN           retry = TRUE;
    BOOLEAN           logError = FALSE;
    BOOLEAN           unhandledError = FALSE;
    BOOLEAN           incrementErrorCount = FALSE;

    ULONG             badSector = 0;
    ULONG             uniqueId = 0;

    NTSTATUS          logStatus;

    ULONG             readSector;
    ULONG             index;

    ULONG             retryInterval = 0;
    KIRQL oldIrql;


    logStatus = -1;

    if(TEST_FLAG(Srb->SrbFlags, SRB_CLASS_FLAGS_PAGING)) {

        //
        // Log anything remotely incorrect about paging i/o
        //

        logError = TRUE;
        uniqueId = 301;
        logStatus = IO_WARNING_PAGING_FAILURE;
    }

    //
    // Check that request sense buffer is valid.
    //

    ASSERT(fdoExtension->CommonExtension.IsFdo);


    //
    // must handle the SRB_STATUS_INTERNAL_ERROR case first,
    // as it has  all the flags set.
    //

    if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_INTERNAL_ERROR) {

        DebugPrint((ClassDebugSenseInfo,
                    "ClassInterpretSenseInfo: Internal Error code is %x\n",
                    Srb->InternalStatus));

        retry = FALSE;
        *Status = Srb->InternalStatus;

    } else if ((Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
        (Srb->SenseInfoBufferLength >=
            offsetof(SENSE_DATA, CommandSpecificInformation))) {

        //
        // Zero the additional sense code and additional sense code qualifier
        // if they were not returned by the device.
        //

        readSector = senseBuffer->AdditionalSenseLength +
            offsetof(SENSE_DATA, AdditionalSenseLength);

        if (readSector > Srb->SenseInfoBufferLength) {
            readSector = Srb->SenseInfoBufferLength;
        }

        if (readSector <= offsetof(SENSE_DATA, AdditionalSenseCode)) {
            senseBuffer->AdditionalSenseCode = 0;
        }

        if (readSector <= offsetof(SENSE_DATA, AdditionalSenseCodeQualifier)) {
            senseBuffer->AdditionalSenseCodeQualifier = 0;
        }

        DebugPrint((ClassDebugSenseInfo,
                    "ClassInterpretSenseInfo: Error code is %x\n",
                    senseBuffer->ErrorCode));
        DebugPrint((ClassDebugSenseInfo,
                    "ClassInterpretSenseInfo: Sense key is %x\n",
                    senseBuffer->SenseKey));
        DebugPrint((ClassDebugSenseInfo,
                    "ClassInterpretSenseInfo: Additional sense code is %x\n",
                    senseBuffer->AdditionalSenseCode));
        DebugPrint((ClassDebugSenseInfo,
                    "ClassInterpretSenseInfo: Additional sense code qualifier "
                    "is %x\n",
                    senseBuffer->AdditionalSenseCodeQualifier));


        switch (senseBuffer->SenseKey & 0xf) {

        case SCSI_SENSE_NOT_READY: {

            DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                        "Device not ready\n"));
            *Status = STATUS_DEVICE_NOT_READY;

            switch (senseBuffer->AdditionalSenseCode) {

            case SCSI_ADSENSE_LUN_NOT_READY: {

                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "Lun not ready\n"));

                switch (senseBuffer->AdditionalSenseCodeQualifier) {

                case SCSI_SENSEQ_OPERATION_IN_PROGRESS: {
                    DEVICE_EVENT_BECOMING_READY notReady;

                    DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                                "Operation In Progress\n"));
                    retryInterval = NOT_READY_RETRY_INTERVAL;

                    RtlZeroMemory(&notReady, sizeof(DEVICE_EVENT_BECOMING_READY));
                    notReady.Version = 1;
                    notReady.Reason = 2;
                    notReady.Estimated100msToReady = retryInterval * 10;
                    ClasspSendNotification(fdoExtension,
                                           &GUID_IO_DEVICE_BECOMING_READY,
                                           sizeof(DEVICE_EVENT_BECOMING_READY),
                                           &notReady);

                    break;
                }

                case SCSI_SENSEQ_BECOMING_READY: {
                    DEVICE_EVENT_BECOMING_READY notReady;

                    DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                                "In process of becoming ready\n"));
                    retryInterval = NOT_READY_RETRY_INTERVAL;

                    RtlZeroMemory(&notReady, sizeof(DEVICE_EVENT_BECOMING_READY));
                    notReady.Version = 1;
                    notReady.Reason = 1;
                    notReady.Estimated100msToReady = retryInterval * 10;
                    ClasspSendNotification(fdoExtension,
                                           &GUID_IO_DEVICE_BECOMING_READY,
                                           sizeof(DEVICE_EVENT_BECOMING_READY),
                                           &notReady);
                    break;
                }

                case SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS: {
                    DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                                "Long write in progress\n"));
                    retry = FALSE;
                    break;
                }

                case SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED: {
                    DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                                "Manual intervention required\n"));
                    *Status = STATUS_NO_MEDIA_IN_DEVICE;
                    retry = FALSE;
                    break;
                }

                case SCSI_SENSEQ_FORMAT_IN_PROGRESS: {
                    DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                                "Format in progress\n"));
                    retry = FALSE;
                    break;
                }

                case SCSI_SENSEQ_CAUSE_NOT_REPORTABLE: {

                    if(!TEST_FLAG(fdoExtension->ScanForSpecialFlags,
                                 CLASS_SPECIAL_CAUSE_NOT_REPORTABLE_HACK)) {

                        DebugPrint((ClassDebugSenseInfo,
                                    "ClassInterpretSenseInfo: "
                                    "not ready, cause unknown\n"));
                        /*
                        Many non-WHQL certified drives (mostly CD-RW) return
                        this when they have no media instead of the obvious
                        choice of:

                        SCSI_SENSE_NOT_READY/SCSI_ADSENSE_NO_MEDIA_IN_DEVICE

                        These drives should not pass WHQL certification due
                        to this discrepancy.

                        */
                        retry = FALSE;
                        break;

                    } else {

                        //
                        // Treat this as init command required and fall through.
                        //
                    }
                }

                case SCSI_SENSEQ_INIT_COMMAND_REQUIRED:
                default: {
                    DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                                "Initializing command required\n"));

                    //
                    // This sense code/additional sense code
                    // combination may indicate that the device
                    // needs to be started.  Send an start unit if this
                    // is a disk device.
                    //

                    if(TEST_FLAG(fdoExtension->DeviceFlags,
                                 DEV_SAFE_START_UNIT) &&
                        !TEST_FLAG(Srb->SrbFlags,
                                   SRB_CLASS_FLAGS_LOW_PRIORITY)) {
                        ClassSendStartUnit(Fdo);
                    }
                    break;
                }


                } // end switch (senseBuffer->AdditionalSenseCodeQualifier)
                break;
            }

            case SCSI_ADSENSE_NO_MEDIA_IN_DEVICE: {
                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "No Media in device.\n"));
                *Status = STATUS_NO_MEDIA_IN_DEVICE;
                retry = FALSE;

                //
                // signal MCN that there isn't any media in the device
                //
                if (!TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA)) {
                    DebugPrint((ClassDebugError, "ClassInterpretSenseInfo: "
                                "No Media in a non-removable device %p\n",
                                Fdo));
                }
                ClassSetMediaChangeState(fdoExtension, MediaNotPresent, FALSE);

                break;
            }
            } // end switch (senseBuffer->AdditionalSenseCode)

            break;
        } // end SCSI_SENSE_NOT_READY

        case SCSI_SENSE_DATA_PROTECT: {
            DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                        "Media write protected\n"));
            *Status = STATUS_MEDIA_WRITE_PROTECTED;
            retry = FALSE;
            break;
        } // end SCSI_SENSE_DATA_PROTECT

        case SCSI_SENSE_MEDIUM_ERROR: {
            DebugPrint((ClassDebugSenseInfo,"ClassInterpretSenseInfo: "
                        "Medium Error (bad block)\n"));
            *Status = STATUS_DEVICE_DATA_ERROR;

            retry = FALSE;
            logError = TRUE;
            uniqueId = 256;
            logStatus = IO_ERR_BAD_BLOCK;

            //
            // Check if this error is due to unknown format
            //
            if (senseBuffer->AdditionalSenseCode == SCSI_ADSENSE_INVALID_MEDIA){

                switch (senseBuffer->AdditionalSenseCodeQualifier) {

                case SCSI_SENSEQ_UNKNOWN_FORMAT: {

                    *Status = STATUS_UNRECOGNIZED_MEDIA;

                    //
                    // Log error only if this is a paging request
                    //
                    if(!TEST_FLAG(Srb->SrbFlags, SRB_CLASS_FLAGS_PAGING)) {
                        logError = FALSE;
                    }
                    break;
                }

                case SCSI_SENSEQ_CLEANING_CARTRIDGE_INSTALLED: {

                    *Status = STATUS_CLEANER_CARTRIDGE_INSTALLED;
                    logError = FALSE;
                    break;

                }
                default: {
                    break;
                }
                } // end switch AdditionalSenseCodeQualifier

            } // end SCSI_ADSENSE_INVALID_MEDIA

            break;

        } // end SCSI_SENSE_MEDIUM_ERROR

        case SCSI_SENSE_HARDWARE_ERROR: {
            DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                        "Hardware error\n"));
            *Status = STATUS_IO_DEVICE_ERROR;
            logError = TRUE;
            uniqueId = 257;
            logStatus = IO_ERR_CONTROLLER_ERROR;
            break;
        } // end SCSI_SENSE_HARDWARE_ERROR

        case SCSI_SENSE_ILLEGAL_REQUEST: {

            DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                        "Illegal SCSI request\n"));
            *Status = STATUS_INVALID_DEVICE_REQUEST;
            retry = FALSE;

            switch (senseBuffer->AdditionalSenseCode) {

            case SCSI_ADSENSE_ILLEGAL_COMMAND: {
                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "Illegal command\n"));
                break;
            }

            case SCSI_ADSENSE_ILLEGAL_BLOCK: {
                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "Illegal block address\n"));
                *Status = STATUS_NONEXISTENT_SECTOR;
                break;
            }

            case SCSI_ADSENSE_INVALID_LUN: {
                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "Invalid LUN\n"));
                *Status = STATUS_NO_SUCH_DEVICE;
                break;
            }

            case SCSI_ADSENSE_MUSIC_AREA: {
                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "Music area\n"));
                break;
            }

            case SCSI_ADSENSE_DATA_AREA: {
                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "Data area\n"));
                break;
            }

            case SCSI_ADSENSE_VOLUME_OVERFLOW: {
                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "Volume overflow\n"));
                break;
            }

            case SCSI_ADSENSE_COPY_PROTECTION_FAILURE: {
                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "Copy protection failure\n"));

                *Status = STATUS_COPY_PROTECTION_FAILURE;

                switch (senseBuffer->AdditionalSenseCodeQualifier) {
                    case SCSI_SENSEQ_AUTHENTICATION_FAILURE:
                        DebugPrint((ClassDebugSenseInfo,
                                    "ClassInterpretSenseInfo: "
                                    "Authentication failure\n"));
                        *Status = STATUS_CSS_AUTHENTICATION_FAILURE;
                        break;
                    case SCSI_SENSEQ_KEY_NOT_PRESENT:
                        DebugPrint((ClassDebugSenseInfo,
                                    "ClassInterpretSenseInfo: "
                                    "Key not present\n"));
                        *Status = STATUS_CSS_KEY_NOT_PRESENT;
                        break;
                    case SCSI_SENSEQ_KEY_NOT_ESTABLISHED:
                        DebugPrint((ClassDebugSenseInfo,
                                    "ClassInterpretSenseInfo: "
                                    "Key not established\n"));
                        *Status = STATUS_CSS_KEY_NOT_ESTABLISHED;
                        break;
                    case SCSI_SENSEQ_READ_OF_SCRAMBLED_SECTOR_WITHOUT_AUTHENTICATION:
                        DebugPrint((ClassDebugSenseInfo,
                                    "ClassInterpretSenseInfo: "
                                    "Read of scrambled sector w/o "
                                    "authentication\n"));
                        *Status = STATUS_CSS_SCRAMBLED_SECTOR;
                        break;
                    case SCSI_SENSEQ_MEDIA_CODE_MISMATCHED_TO_LOGICAL_UNIT:
                        DebugPrint((ClassDebugSenseInfo,
                                    "ClassInterpretSenseInfo: "
                                    "Media region does not logical unit "
                                    "region\n"));
                        *Status = STATUS_CSS_REGION_MISMATCH;
                        break;
                    case SCSI_SENSEQ_LOGICAL_UNIT_RESET_COUNT_ERROR:
                        DebugPrint((ClassDebugSenseInfo,
                                    "ClassInterpretSenseInfo: "
                                    "Region set error -- region may "
                                    "be permanent\n"));
                        *Status = STATUS_CSS_RESETS_EXHAUSTED;
                        break;
                } // end switch of ASCQ for COPY_PROTECTION_FAILURE

                break;
            }


            case SCSI_ADSENSE_INVALID_CDB: {
                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "Invalid CDB\n"));

                //
                // Note: the retry interval is not typically used.
                // it is set here only because a ClassErrorHandler
                // cannot set the retryInterval, and the error may
                // require a few commands to be sent to clear whatever
                // caused this condition (i.e. disk clears the write
                // cache, requiring at least two commands)
                //
                // hopefully, this shortcoming can be changed for
                // blackcomb.
                //

                retryInterval = 3;
                break;
            }

            } // end switch (senseBuffer->AdditionalSenseCode)

            break;
        } // end SCSI_SENSE_ILLEGAL_REQUEST

        case SCSI_SENSE_UNIT_ATTENTION: {

            //PVPB vpb;
            ULONG count;

            //
            // A media change may have occured so increment the change
            // count for the physical device
            //

            count = InterlockedIncrement((PLONG)&fdoExtension->MediaChangeCount);
            DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                        "Media change count for device %d incremented to %#lx\n",
                        fdoExtension->DeviceNumber, count));


            switch (senseBuffer->AdditionalSenseCode) {
            case SCSI_ADSENSE_MEDIUM_CHANGED: {
                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "Media changed\n"));

                if (!TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA)) {
                    DebugPrint((ClassDebugError, "ClassInterpretSenseInfo: "
                                "Media Changed on non-removable device %p\n",
                                Fdo));
                }
                ClassSetMediaChangeState(fdoExtension, MediaPresent, FALSE);
                break;
            }

            case SCSI_ADSENSE_BUS_RESET: {
                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "Bus reset\n"));
                break;
            }

            case SCSI_ADSENSE_OPERATOR_REQUEST: {
                switch (senseBuffer->AdditionalSenseCodeQualifier) {

                case SCSI_SENSEQ_MEDIUM_REMOVAL: {
                    DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                                "Ejection request received!\n"));
                    ClassSendEjectionNotification(fdoExtension);
                    break;
                }

                case SCSI_SENSEQ_WRITE_PROTECT_ENABLE: {
                    DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                                "Operator selected write permit?! "
                                "(unsupported!)\n"));
                    break;
                }

                case SCSI_SENSEQ_WRITE_PROTECT_DISABLE: {
                    DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                                "Operator selected write protect?! "
                                "(unsupported!)\n"));
                    break;
                }

                }
                break;
            }

            default: {
                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "Unit attention\n"));
                break;
            }

            } // end  switch (senseBuffer->AdditionalSenseCode)

            if (TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA))
            {
                //
                // TODO : Is the media lockable?
                //

                if ((ClassGetVpb(Fdo) != NULL) && (ClassGetVpb(Fdo)->Flags & VPB_MOUNTED))
                {
                    //
                    // Set bit to indicate that media may have changed
                    // and volume needs verification.
                    //

                    SET_FLAG(Fdo->Flags, DO_VERIFY_VOLUME);

                    *Status = STATUS_VERIFY_REQUIRED;
                    retry = FALSE;
                }
            }
            else
            {
                *Status = STATUS_IO_DEVICE_ERROR;
            }

            break;

        } // end SCSI_SENSE_UNIT_ATTENTION

        case SCSI_SENSE_ABORTED_COMMAND: {
            DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                        "Command aborted\n"));
            *Status = STATUS_IO_DEVICE_ERROR;
            retryInterval = 1;
            break;
        } // end SCSI_SENSE_ABORTED_COMMAND

        case SCSI_SENSE_BLANK_CHECK: {
            DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                        "Media blank check\n"));
            retry = FALSE;
            *Status = STATUS_NO_DATA_DETECTED;
            break;
        } // end SCSI_SENSE_BLANK_CHECK

        case SCSI_SENSE_RECOVERED_ERROR: {

            DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                        "Recovered error\n"));
            *Status = STATUS_SUCCESS;
            retry = FALSE;
            logError = TRUE;
            uniqueId = 258;

            switch(senseBuffer->AdditionalSenseCode) {
            case SCSI_ADSENSE_SEEK_ERROR:
            case SCSI_ADSENSE_TRACK_ERROR: {
                logStatus = IO_ERR_SEEK_ERROR;
                break;
            }

            case SCSI_ADSENSE_REC_DATA_NOECC:
            case SCSI_ADSENSE_REC_DATA_ECC: {
                logStatus = IO_RECOVERED_VIA_ECC;
                break;
            }

            case SCSI_ADSENSE_FAILURE_PREDICTION_THRESHOLD_EXCEEDED: {
                UCHAR wmiEventData[5];

                *((PULONG)wmiEventData) = sizeof(UCHAR);
                wmiEventData[sizeof(ULONG)] = senseBuffer->AdditionalSenseCodeQualifier;

                //
                // Don't log another eventlog if we have already logged once
                // NOTE: this should have been interlocked, but the structure
                //       was publicly defined to use a BOOLEAN (char).  Since
                //       media only reports these errors once per X minutes,
                //       the potential race condition is nearly non-existant.
                //       the worst case is duplicate log entries, so ignore.
                //

                if (fdoExtension->FailurePredicted == 0) {
                    logError = TRUE;
                }
                fdoExtension->FailurePredicted = TRUE;
                fdoExtension->FailureReason = senseBuffer->AdditionalSenseCodeQualifier;
                logStatus = IO_WRN_FAILURE_PREDICTED;

                ClassNotifyFailurePredicted(fdoExtension,
                                            (PUCHAR)&wmiEventData,
                                            sizeof(wmiEventData),
                                            0,
                                            4,
                                            Srb->PathId,
                                            Srb->TargetId,
                                            Srb->Lun);
                break;
            }

            default: {
                logStatus = IO_ERR_CONTROLLER_ERROR;
                break;
            }

            } // end switch(senseBuffer->AdditionalSenseCode)

            if (senseBuffer->IncorrectLength) {

                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "Incorrect length detected.\n"));
                *Status = STATUS_INVALID_BLOCK_LENGTH ;
            }

            break;
        } // end SCSI_SENSE_RECOVERED_ERROR

        case SCSI_SENSE_NO_SENSE: {

            //
            // Check other indicators.
            //

            if (senseBuffer->IncorrectLength) {

                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "Incorrect length detected.\n"));
                *Status = STATUS_INVALID_BLOCK_LENGTH ;
                retry   = FALSE;

            } else {

                DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                            "No specific sense key\n"));
                *Status = STATUS_IO_DEVICE_ERROR;
                retry   = TRUE;
            }

            break;
        } // end SCSI_SENSE_NO_SENSE

        default: {
            DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                        "Unrecognized sense code\n"));
            *Status = STATUS_IO_DEVICE_ERROR;
            break;
        }

        } // end switch (senseBuffer->SenseKey & 0xf)

        //
        // Try to determine the bad sector from the inquiry data.
        //

        if ((((PCDB)Srb->Cdb)->CDB10.OperationCode == SCSIOP_READ ||
            ((PCDB)Srb->Cdb)->CDB10.OperationCode == SCSIOP_VERIFY ||
            ((PCDB)Srb->Cdb)->CDB10.OperationCode == SCSIOP_WRITE)) {

            for (index = 0; index < 4; index++) {
                badSector = (badSector << 8) | senseBuffer->Information[index];
            }

            readSector = 0;
            for (index = 0; index < 4; index++) {
                readSector = (readSector << 8) | Srb->Cdb[index+2];
            }

            index = (((PCDB)Srb->Cdb)->CDB10.TransferBlocksMsb << 8) |
                ((PCDB)Srb->Cdb)->CDB10.TransferBlocksLsb;

            //
            // Make sure the bad sector is within the read sectors.
            //

            if (!(badSector >= readSector && badSector < readSector + index)) {
                badSector = readSector;
            }
        }

    } else {

        //
        // Request sense buffer not valid. No sense information
        // to pinpoint the error. Return general request fail.
        //

        DebugPrint((ClassDebugSenseInfo, "ClassInterpretSenseInfo: "
                    "Request sense info not valid. SrbStatus %2x\n",
                    SRB_STATUS(Srb->SrbStatus)));
        retry = TRUE;

        switch (SRB_STATUS(Srb->SrbStatus)) {
        case SRB_STATUS_INVALID_LUN:
        case SRB_STATUS_INVALID_TARGET_ID:
        case SRB_STATUS_NO_DEVICE:
        case SRB_STATUS_NO_HBA:
        case SRB_STATUS_INVALID_PATH_ID: {
            *Status = STATUS_NO_SUCH_DEVICE;
            retry = FALSE;
            break;
        }

        case SRB_STATUS_COMMAND_TIMEOUT:
        case SRB_STATUS_TIMEOUT: {

            //
            // Update the error count for the device.
            //

            incrementErrorCount = TRUE;
            *Status = STATUS_IO_TIMEOUT;
            break;
        }

        case SRB_STATUS_ABORTED: {

            //
            // Update the error count for the device.
            //

            incrementErrorCount = TRUE;
            *Status = STATUS_IO_TIMEOUT;
            retryInterval = 1;
            break;
        }


        case SRB_STATUS_SELECTION_TIMEOUT: {
            logError = TRUE;
            logStatus = IO_ERR_NOT_READY;
            uniqueId = 260;
            *Status = STATUS_DEVICE_NOT_CONNECTED;
            retry = FALSE;
            break;
        }

        case SRB_STATUS_DATA_OVERRUN: {
            *Status = STATUS_DATA_OVERRUN;
            retry = FALSE;
            break;
        }

        case SRB_STATUS_PHASE_SEQUENCE_FAILURE: {

            //
            // Update the error count for the device.
            //

            incrementErrorCount = TRUE;
            *Status = STATUS_IO_DEVICE_ERROR;

            //
            // If there was  phase sequence error then limit the number of
            // retries.
            //

            if (RetryCount > 1 ) {
                retry = FALSE;
            }

            break;
        }

        case SRB_STATUS_REQUEST_FLUSHED: {

            //
            // If the status needs verification bit is set.  Then set
            // the status to need verification and no retry; otherwise,
            // just retry the request.
            //

            if (TEST_FLAG(Fdo->Flags, DO_VERIFY_VOLUME)) {

                *Status = STATUS_VERIFY_REQUIRED;
                retry = FALSE;

            } else {
                *Status = STATUS_IO_DEVICE_ERROR;
            }

            break;
        }

        case SRB_STATUS_INVALID_REQUEST: {
            *Status = STATUS_INVALID_DEVICE_REQUEST;
            retry = FALSE;
            break;
        }

        case SRB_STATUS_UNEXPECTED_BUS_FREE:
        case SRB_STATUS_PARITY_ERROR:

            //
            // Update the error count for the device
            // and fall through to below
            //

            incrementErrorCount = TRUE;

        case SRB_STATUS_BUS_RESET: {
            *Status = STATUS_IO_DEVICE_ERROR;
            break;
        }

        case SRB_STATUS_ERROR: {

            *Status = STATUS_IO_DEVICE_ERROR;
            if (Srb->ScsiStatus == 0) {

                //
                // This is some strange return code.  Update the error
                // count for the device.
                //

                incrementErrorCount = TRUE;

            } if (Srb->ScsiStatus == SCSISTAT_BUSY) {

                *Status = STATUS_DEVICE_NOT_READY;

            } if (Srb->ScsiStatus == SCSISTAT_RESERVATION_CONFLICT) {

                *Status = STATUS_DEVICE_BUSY;
                retry = FALSE;
                logError = FALSE;

            }

            break;
        }

        default: {
            logError = TRUE;
            logStatus = IO_ERR_CONTROLLER_ERROR;
            uniqueId = 259;
            *Status = STATUS_IO_DEVICE_ERROR;
            unhandledError = TRUE;
            break;
        }

        }

        //
        // NTRAID #183546 - if we support GESN subtype NOT_READY events, and
        // we know from a previous poll when the device will be ready (ETA)
        // we should delay the retry more appropriately than just guessing.
        //
        /*
        if (fdoExtension->MediaChangeDetectionInfo &&
            fdoExtension->MediaChangeDetectionInfo->Gesn.Supported &&
            TEST_FLAG(fdoExtension->MediaChangeDetectionInfo->Gesn.EventMask,
                      NOTIFICATION_DEVICE_BUSY_CLASS_MASK)
            ) {
            // check if Gesn.ReadyTime if greater than current tick count
            // if so, delay that long (from 1 to 30 seconds max?)
            // else, leave the guess of time alone.
        }
        */

    }

    if (incrementErrorCount) {

        //
        // if any error count occurred, delay the retry of this io by
        // at least one second, if caller supports it.
        //

        if (retryInterval == 0) {
            retryInterval = 1;
        }
        ClasspPerfIncrementErrorCount(fdoExtension);
    }

    //
    // If there is a class specific error handler call it.
    //

    if (fdoExtension->CommonExtension.DevInfo->ClassError != NULL) {

        fdoExtension->CommonExtension.DevInfo->ClassError(Fdo,
                                                          Srb,
                                                          Status,
                                                          &retry);
    }

    //
    // If the caller wants to know the suggested retry interval tell them.
    //

    if(ARGUMENT_PRESENT(RetryInterval)) {
        *RetryInterval = retryInterval;
    }


    /*
     *  LOG the error:
     *      Always log the error in our internal log.
     *      If logError is set, also log the error in the system log.
     */
    {
        ULONG totalSize;
        ULONG senseBufferSize = 0;
        IO_ERROR_LOG_PACKET staticErrLogEntry = {0};
        CLASS_ERROR_LOG_DATA staticErrLogData = { { { 0 } } };

        //
        // Calculate the total size of the error log entry.
        // add to totalSize in the order that they are used.
        // the advantage to calculating all the sizes here is
        // that we don't have to do a bunch of extraneous checks
        // later on in this code path.
        //
        totalSize = sizeof(IO_ERROR_LOG_PACKET)  // required
                  - sizeof(ULONG)                // struct includes one ULONG
                  + sizeof(CLASS_ERROR_LOG_DATA);// struct for ease

        //
        // also save any available extra sense data, up to the maximum errlog
        // packet size .  WMI should be used for real-time analysis.
        // the event log should only be used for post-mortem debugging.
        //
        if (TEST_FLAG(Srb->SrbStatus, SRB_STATUS_AUTOSENSE_VALID)) {
            ULONG validSenseBytes;
            BOOLEAN validSense;

            //
            // make sure we can at least access the AdditionalSenseLength field
            //
            validSense = RTL_CONTAINS_FIELD(senseBuffer,
                                            Srb->SenseInfoBufferLength,
                                            AdditionalSenseLength);
            if (validSense) {

                //
                // if extra info exists, copy the maximum amount of available
                // sense data that is safe into the the errlog.
                //
                validSenseBytes = senseBuffer->AdditionalSenseLength
                                + offsetof(SENSE_DATA, AdditionalSenseLength);

                //
                // this is invalid because it causes overflow!
                // whoever sent this type of request would cause
                // a system crash.
                //
                ASSERT(validSenseBytes < MAX_ADDITIONAL_SENSE_BYTES);

                //
                // set to save the most sense buffer possible
                //
                senseBufferSize = max(validSenseBytes, sizeof(SENSE_DATA));
                senseBufferSize = min(senseBufferSize, Srb->SenseInfoBufferLength);
            } else {
                //
                // it's smaller than required to read the total number of
                // valid bytes, so just use the SenseInfoBufferLength field.
                //
                senseBufferSize = Srb->SenseInfoBufferLength;
            }

            /*
             *  Bump totalSize by the number of extra senseBuffer bytes
             *  (beyond the default sense buffer within CLASS_ERROR_LOG_DATA).
             *  Make sure to never allocate more than ERROR_LOG_MAXIMUM_SIZE.
             */
            if (senseBufferSize > sizeof(SENSE_DATA)){
                totalSize += senseBufferSize-sizeof(SENSE_DATA);
                if (totalSize > ERROR_LOG_MAXIMUM_SIZE){
                    senseBufferSize -= totalSize-ERROR_LOG_MAXIMUM_SIZE;
                    totalSize = ERROR_LOG_MAXIMUM_SIZE;
                }
            }
        }

        //
        // If we've used up all of our retry attempts, set the final status to
        // reflect the appropriate result.
        //
        if (retry && RetryCount < MAXIMUM_RETRIES) {
            staticErrLogEntry.FinalStatus = STATUS_SUCCESS;
            staticErrLogData.ErrorRetried = TRUE;
        } else {
            staticErrLogEntry.FinalStatus = *Status;
        }
        if (TEST_FLAG(Srb->SrbFlags, SRB_CLASS_FLAGS_PAGING)) {
            staticErrLogData.ErrorPaging = TRUE;
        }
        if (unhandledError) {
            staticErrLogData.ErrorUnhandled = TRUE;
        }

        //
        // Calculate the device offset if there is a geometry.
        //
        staticErrLogEntry.DeviceOffset.QuadPart = (LONGLONG)badSector;
        staticErrLogEntry.DeviceOffset.QuadPart *= (LONGLONG)fdoExtension->DiskGeometry.BytesPerSector;
        if (logStatus == -1){
            staticErrLogEntry.ErrorCode = STATUS_IO_DEVICE_ERROR;
        } else {
            staticErrLogEntry.ErrorCode = logStatus;
        }

        /*
         *  The dump data follows the IO_ERROR_LOG_PACKET,
         *  with the first ULONG of dump data inside the packet.
         */
        staticErrLogEntry.DumpDataSize = (USHORT)totalSize - sizeof(IO_ERROR_LOG_PACKET) + sizeof(ULONG);

        staticErrLogEntry.SequenceNumber = 0;
        staticErrLogEntry.MajorFunctionCode = MajorFunctionCode;
        staticErrLogEntry.IoControlCode = IoDeviceCode;
        staticErrLogEntry.RetryCount = (UCHAR) RetryCount;
        staticErrLogEntry.UniqueErrorValue = uniqueId;

        KeQueryTickCount(&staticErrLogData.TickCount);
        staticErrLogData.PortNumber = (ULONG)-1;

        /*
         *  Save the entire contents of the SRB.
         */
        staticErrLogData.Srb = *Srb;

        /*
         *  For our private log, save just the default length of the SENSE_DATA.
         */
        if (senseBufferSize != 0){
            RtlCopyMemory(&staticErrLogData.SenseData, senseBuffer, min(senseBufferSize, sizeof(SENSE_DATA)));
        }

        /*
         *  Save the error log in our context.
         *  We only save the default sense buffer length.
         */
        KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);
        fdoData->ErrorLogs[fdoData->ErrorLogNextIndex] = staticErrLogData;
        fdoData->ErrorLogNextIndex++;
        fdoData->ErrorLogNextIndex %= NUM_ERROR_LOG_ENTRIES;
        KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);

        /*
         *  If logError is set, also save this log in the system's error log.
         *  But make sure we don't log TUR failures over and over 
         *  (e.g. if an external drive was switched off and we're still sending TUR's to it every second).
         */
        if ((((PCDB)Srb->Cdb)->CDB10.OperationCode == SCSIOP_TEST_UNIT_READY) && logError){
            if (fdoData->LoggedTURFailureSinceLastIO){
                logError = FALSE;
            }
            else {
                fdoData->LoggedTURFailureSinceLastIO = TRUE;    
            }
        }
        if (logError){
            PIO_ERROR_LOG_PACKET errorLogEntry;
            PCLASS_ERROR_LOG_DATA errlogData;

            errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(Fdo, (UCHAR)totalSize);
            if (errorLogEntry){
                errlogData = (PCLASS_ERROR_LOG_DATA)errorLogEntry->DumpData;

                *errorLogEntry = staticErrLogEntry;
                *errlogData = staticErrLogData;

                /*
                 *  For the system log, copy as much of the sense buffer as possible.
                 */
                if (senseBufferSize != 0) {
                    RtlCopyMemory(&errlogData->SenseData, senseBuffer, senseBufferSize);
                }

                /*
                 *  Write the error log packet to the system error logging thread.
                 */
                IoWriteErrorLogEntry(errorLogEntry);
            }
        }
    }

    return retry;

} // end ClassInterpretSenseInfo()

/*++////////////////////////////////////////////////////////////////////////////

ClassModeSense()

Routine Description:

    This routine sends a mode sense command to a target ID and returns
    when it is complete.

Arguments:

    Fdo - Supplies the functional device object associated with this request.

    ModeSenseBuffer - Supplies a buffer to store the sense data.

    Length - Supplies the length in bytes of the mode sense buffer.

    PageMode - Supplies the page or pages of mode sense data to be retrieved.

Return Value:

    Length of the transferred data is returned.

--*/
ULONG NTAPI ClassModeSense(   IN PDEVICE_OBJECT Fdo,
                        IN PCHAR ModeSenseBuffer,
                        IN ULONG Length,
                        IN UCHAR PageMode)
{
    ULONG lengthTransferred = 0;
    PMDL senseBufferMdl;

    PAGED_CODE();

    senseBufferMdl = BuildDeviceInputMdl(ModeSenseBuffer, Length);
    if (senseBufferMdl){

        TRANSFER_PACKET *pkt = DequeueFreeTransferPacket(Fdo, TRUE);
        if (pkt){
            KEVENT event;
            //NTSTATUS pktStatus;
            IRP pseudoIrp = {0};

            /*
             *  Store the number of packets servicing the irp (one)
             *  inside the original IRP.  It will be used to counted down
             *  to zero when the packet completes.
             *  Initialize the original IRP's status to success.
             *  If the packet fails, we will set it to the error status.
             */
            pseudoIrp.Tail.Overlay.DriverContext[0] = LongToPtr(1);
            pseudoIrp.IoStatus.Status = STATUS_SUCCESS;
            pseudoIrp.IoStatus.Information = 0;
            pseudoIrp.MdlAddress = senseBufferMdl;

            /*
             *  Set this up as a SYNCHRONOUS transfer, submit it,
             *  and wait for the packet to complete.  The result
             *  status will be written to the original irp.
             */
            ASSERT(Length <= 0x0ff);
            KeInitializeEvent(&event, SynchronizationEvent, FALSE);
            SetupModeSenseTransferPacket(pkt, &event, ModeSenseBuffer, (UCHAR)Length, PageMode, &pseudoIrp);
            SubmitTransferPacket(pkt);
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

            if (NT_SUCCESS(pseudoIrp.IoStatus.Status)){
                lengthTransferred = (ULONG)pseudoIrp.IoStatus.Information;
            }
            else {
                /*
                 *  This request can sometimes fail legitimately
                 *  (e.g. when a SCSI device is attached but turned off)
                 *  so this is not necessarily a device/driver bug.
                 */
                DBGTRACE(ClassDebugWarning, ("ClassModeSense on Fdo %ph failed with status %xh.", Fdo, pseudoIrp.IoStatus.Status));
            }
        }

        FreeDeviceInputMdl(senseBufferMdl);
    }

    return lengthTransferred;
}

/*++////////////////////////////////////////////////////////////////////////////

ClassFindModePage()

Routine Description:

    This routine scans through the mode sense data and finds the requested
    mode sense page code.

Arguments:
    ModeSenseBuffer - Supplies a pointer to the mode sense data.

    Length - Indicates the length of valid data.

    PageMode - Supplies the page mode to be searched for.

    Use6Byte - Indicates whether 6 or 10 byte mode sense was used.

Return Value:

    A pointer to the the requested mode page.  If the mode page was not found
    then NULL is return.

--*/
PVOID
NTAPI
ClassFindModePage(
    IN PCHAR ModeSenseBuffer,
    IN ULONG Length,
    IN UCHAR PageMode,
    IN BOOLEAN Use6Byte
    )
{
    PCHAR limit;
    ULONG  parameterHeaderLength;
    PVOID result = NULL;

    limit = ModeSenseBuffer + Length;
    parameterHeaderLength = (Use6Byte) ? sizeof(MODE_PARAMETER_HEADER) : sizeof(MODE_PARAMETER_HEADER10);

    if (Length >= parameterHeaderLength) {

        PMODE_PARAMETER_HEADER10 modeParam10;
        ULONG blockDescriptorLength;

        /*
         *  Skip the mode select header and block descriptors.
         */
        if (Use6Byte){
            blockDescriptorLength = ((PMODE_PARAMETER_HEADER) ModeSenseBuffer)->BlockDescriptorLength;
        }
        else {
            modeParam10 = (PMODE_PARAMETER_HEADER10) ModeSenseBuffer;
            blockDescriptorLength = modeParam10->BlockDescriptorLength[1];
        }

        ModeSenseBuffer += parameterHeaderLength + blockDescriptorLength;

        //
        // ModeSenseBuffer now points at pages.  Walk the pages looking for the
        // requested page until the limit is reached.
        //

        while (ModeSenseBuffer +
               RTL_SIZEOF_THROUGH_FIELD(MODE_DISCONNECT_PAGE, PageLength) < limit) {

            if (((PMODE_DISCONNECT_PAGE) ModeSenseBuffer)->PageCode == PageMode) {

                /*
                 * found the mode page.  make sure it's safe to touch it all
                 * before returning the pointer to caller
                 */

                if (ModeSenseBuffer + ((PMODE_DISCONNECT_PAGE)ModeSenseBuffer)->PageLength > limit) {
                    /*
                     *  Return NULL since the page is not safe to access in full
                     */
                    result = NULL;
                }
                else {
                    result = ModeSenseBuffer;
                }
                break;
            }

            //
            // Advance to the next page which is 4-byte-aligned offset after this page.
            //
            ModeSenseBuffer +=
                ((PMODE_DISCONNECT_PAGE) ModeSenseBuffer)->PageLength +
                RTL_SIZEOF_THROUGH_FIELD(MODE_DISCONNECT_PAGE, PageLength);

        }
    }

    return result;
} // end ClassFindModePage()

/*++////////////////////////////////////////////////////////////////////////////

ClassSendSrbAsynchronous()

Routine Description:

    This routine takes a partially built Srb and an Irp and sends it down to
    the port driver.

    This routine must be called with the remove lock held for the specified
    Irp.

Arguments:

    Fdo - Supplies the functional device object for the original request.

    Srb - Supplies a partially built ScsiRequestBlock.  In particular, the
        CDB and the SRB timeout value must be filled in.  The SRB must not be
        allocated from zone.

    Irp - Supplies the requesting Irp.

    BufferAddress - Supplies a pointer to the buffer to be transfered.

    BufferLength - Supplies the length of data transfer.

    WriteToDevice - Indicates the data transfer will be from system memory to
        device.

Return Value:

    Returns STATUS_PENDING if the request is dispatched (since the
    completion routine may change the irp's status value we cannot simply
    return the value of the dispatch)

    or returns a status value to indicate why it failed.

--*/
NTSTATUS
NTAPI
ClassSendSrbAsynchronous(
    PDEVICE_OBJECT Fdo,
    PSCSI_REQUEST_BLOCK Srb,
    PIRP Irp,
    PVOID BufferAddress,
    ULONG BufferLength,
    BOOLEAN WriteToDevice
    )
{

    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PIO_STACK_LOCATION irpStack;

    ULONG savedFlags;

    //
    // Write length to SRB.
    //

    Srb->Length = sizeof(SCSI_REQUEST_BLOCK);

    //
    // Set SCSI bus address.
    //

    Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

    //
    // This is a violation of the SCSI spec but it is required for
    // some targets.
    //

    // Srb->Cdb[1] |= deviceExtension->Lun << 5;

    //
    // Indicate auto request sense by specifying buffer and size.
    //

    Srb->SenseInfoBuffer = fdoExtension->SenseData;
    Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
    Srb->DataBuffer = BufferAddress;

    //
    // Save the class driver specific flags away.
    //

    savedFlags = Srb->SrbFlags & SRB_FLAGS_CLASS_DRIVER_RESERVED;

    //
    // Allow the caller to specify that they do not wish
    // IoStartNextPacket() to be called in the completion routine.
    //

    SET_FLAG(savedFlags, (Srb->SrbFlags & SRB_FLAGS_DONT_START_NEXT_PACKET));

    if (BufferAddress != NULL) {

        //
        // Build Mdl if necessary.
        //

        if (Irp->MdlAddress == NULL) {

            if (IoAllocateMdl(BufferAddress,
                              BufferLength,
                              FALSE,
                              FALSE,
                              Irp) == NULL) {

                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                //
                // ClassIoComplete() would have free'd the srb
                //

                if (PORT_ALLOCATED_SENSE(fdoExtension, Srb)) {
                    FREE_PORT_ALLOCATED_SENSE_BUFFER(fdoExtension, Srb);
                }
                ClassFreeOrReuseSrb(fdoExtension, Srb);
                ClassReleaseRemoveLock(Fdo, Irp);
                ClassCompleteRequest(Fdo, Irp, IO_NO_INCREMENT);

                return STATUS_INSUFFICIENT_RESOURCES;
            }

            MmBuildMdlForNonPagedPool(Irp->MdlAddress);

        } else {

            //
            // Make sure the buffer requested matches the MDL.
            //

            ASSERT(BufferAddress == MmGetMdlVirtualAddress(Irp->MdlAddress));
        }

        //
        // Set read flag.
        //

        Srb->SrbFlags = WriteToDevice ? SRB_FLAGS_DATA_OUT : SRB_FLAGS_DATA_IN;

    } else {

        //
        // Clear flags.
        //

        Srb->SrbFlags = SRB_FLAGS_NO_DATA_TRANSFER;
    }

    //
    // Restore saved flags.
    //

    SET_FLAG(Srb->SrbFlags, savedFlags);

    //
    // Disable synchronous transfer for these requests.
    //

    SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);

    //
    // Set the transfer length.
    //

    Srb->DataTransferLength = BufferLength;

    //
    // Zero out status.
    //

    Srb->ScsiStatus = Srb->SrbStatus = 0;

    Srb->NextSrb = 0;

    //
    // Save a few parameters in the current stack location.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Save retry count in current Irp stack.
    //

    irpStack->Parameters.Others.Argument4 = (PVOID)MAXIMUM_RETRIES;

    //
    // Set up IoCompletion routine address.
    //

    IoSetCompletionRoutine(Irp, ClassIoComplete, Srb, TRUE, TRUE, TRUE);

    //
    // Get next stack location and
    // set major function code.
    //

    irpStack = IoGetNextIrpStackLocation(Irp);

    irpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Save SRB address in next stack for port driver.
    //

    irpStack->Parameters.Scsi.Srb = Srb;

    //
    // Set up Irp Address.
    //

    Srb->OriginalRequest = Irp;

    //
    // Call the port driver to process the request.
    //

    IoMarkIrpPending(Irp);

    IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, Irp);

    return STATUS_PENDING;

} // end ClassSendSrbAsynchronous()

/*++////////////////////////////////////////////////////////////////////////////

ClassDeviceControlDispatch()

Routine Description:

    The routine is the common class driver device control dispatch entry point.
    This routine is invokes the device-specific drivers DeviceControl routine,
    (which may call the Class driver's common DeviceControl routine).

Arguments:

    DeviceObject - Supplies a pointer to the device object for this request.

    Irp - Supplies the Irp making the request.

Return Value:

   Returns the status returned from the device-specific driver.

--*/
NTSTATUS
NTAPI
ClassDeviceControlDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{

    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    ULONG isRemoved;

    isRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);

    if(isRemoved) {

        ClassReleaseRemoveLock(DeviceObject, Irp);

        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // Call the class specific driver DeviceControl routine.
    // If it doesn't handle it, it will call back into ClassDeviceControl.
    //

    ASSERT(commonExtension->DevInfo->ClassDeviceControl);

    return commonExtension->DevInfo->ClassDeviceControl(DeviceObject,Irp);
} // end ClassDeviceControlDispatch()

/*++////////////////////////////////////////////////////////////////////////////

ClassDeviceControl()

Routine Description:

    The routine is the common class driver device control dispatch function.
    This routine is called by a class driver when it get an unrecognized
    device control request.  This routine will perform the correct action for
    common requests such as lock media.  If the device request is unknown it
    passed down to the next level.

    This routine must be called with the remove lock held for the specified
    irp.

Arguments:

    DeviceObject - Supplies a pointer to the device object for this request.

    Irp - Supplies the Irp making the request.

Return Value:

   Returns back a STATUS_PENDING or a completion status.

--*/
NTSTATUS
NTAPI
ClassDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextStack = NULL;

    ULONG controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    PSCSI_REQUEST_BLOCK srb = NULL;
    PCDB cdb = NULL;

    NTSTATUS status;
    ULONG modifiedIoControlCode;

    //
    // If this is a pass through I/O control, set the minor function code
    // and device address and pass it to the port driver.
    //

    if ((controlCode == IOCTL_SCSI_PASS_THROUGH) ||
        (controlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT)) {

        //PSCSI_PASS_THROUGH scsiPass;

        //
        // Validate the user buffer.
        //
        #if defined (_WIN64)

            if (IoIs32bitProcess(Irp)) {

                if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SCSI_PASS_THROUGH32)){

                    Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

                    ClassReleaseRemoveLock(DeviceObject, Irp);
                    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                    status = STATUS_INVALID_PARAMETER;
                    goto SetStatusAndReturn;
                }
            }
            else
        #endif
            {
                if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                    sizeof(SCSI_PASS_THROUGH)) {

                    Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

                    ClassReleaseRemoveLock(DeviceObject, Irp);
                    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                    status = STATUS_INVALID_PARAMETER;
                    goto SetStatusAndReturn;
                }
            }

        IoCopyCurrentIrpStackLocationToNext(Irp);

        nextStack = IoGetNextIrpStackLocation(Irp);
        nextStack->MinorFunction = 1;

        ClassReleaseRemoveLock(DeviceObject, Irp);

        status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
        goto SetStatusAndReturn;
    }

    Irp->IoStatus.Information = 0;

    switch (controlCode) {

        case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID: {

            PMOUNTDEV_UNIQUE_ID uniqueId;

            if (!commonExtension->MountedDeviceInterfaceName.Buffer) {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(MOUNTDEV_UNIQUE_ID)) {

                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
                break;
            }

            uniqueId = Irp->AssociatedIrp.SystemBuffer;
            uniqueId->UniqueIdLength =
                    commonExtension->MountedDeviceInterfaceName.Length;

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(USHORT) + uniqueId->UniqueIdLength) {

                status = STATUS_BUFFER_OVERFLOW;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
                break;
            }

            RtlCopyMemory(uniqueId->UniqueId,
                          commonExtension->MountedDeviceInterfaceName.Buffer,
                          uniqueId->UniqueIdLength);

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(USHORT) +
                                        uniqueId->UniqueIdLength;
            break;
        }

        case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME: {

            PMOUNTDEV_NAME name;

            ASSERT(commonExtension->DeviceName.Buffer);

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(MOUNTDEV_NAME)) {

                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
                break;
            }

            name = Irp->AssociatedIrp.SystemBuffer;
            name->NameLength = commonExtension->DeviceName.Length;

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(USHORT) + name->NameLength) {

                status = STATUS_BUFFER_OVERFLOW;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
                break;
            }

            RtlCopyMemory(name->Name, commonExtension->DeviceName.Buffer,
                          name->NameLength);

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(USHORT) + name->NameLength;
            break;
        }

        case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME: {

            PMOUNTDEV_SUGGESTED_LINK_NAME suggestedName;
            WCHAR driveLetterNameBuffer[10];
            RTL_QUERY_REGISTRY_TABLE queryTable[2];
            PWSTR valueName;
            UNICODE_STRING driveLetterName;

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(MOUNTDEV_SUGGESTED_LINK_NAME)) {

                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);
                break;
            }

            valueName = ExAllocatePoolWithTag(
                            PagedPool,
                            commonExtension->DeviceName.Length + sizeof(WCHAR),
                            '8CcS');

            if (!valueName) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            RtlCopyMemory(valueName, commonExtension->DeviceName.Buffer,
                          commonExtension->DeviceName.Length);
            valueName[commonExtension->DeviceName.Length/sizeof(WCHAR)] = 0;

            driveLetterName.Buffer = driveLetterNameBuffer;
            driveLetterName.MaximumLength = 20;
            driveLetterName.Length = 0;

            RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
            queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                                  RTL_QUERY_REGISTRY_DIRECT;
            queryTable[0].Name = valueName;
            queryTable[0].EntryContext = &driveLetterName;

            status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                            L"\\Registry\\Machine\\System\\DISK",
                                            queryTable, NULL, NULL);

            if (!NT_SUCCESS(status)) {
                ExFreePool(valueName);
                break;
            }

            if (driveLetterName.Length == 4 &&
                driveLetterName.Buffer[0] == '%' &&
                driveLetterName.Buffer[1] == ':') {

                driveLetterName.Buffer[0] = 0xFF;

            } else if (driveLetterName.Length != 4 ||
                driveLetterName.Buffer[0] < FirstDriveLetter ||
                driveLetterName.Buffer[0] > LastDriveLetter ||
                driveLetterName.Buffer[1] != ':') {

                status = STATUS_NOT_FOUND;
                ExFreePool(valueName);
                break;
            }

            suggestedName = Irp->AssociatedIrp.SystemBuffer;
            suggestedName->UseOnlyIfThereAreNoOtherLinks = TRUE;
            suggestedName->NameLength = 28;

            Irp->IoStatus.Information =
                    FIELD_OFFSET(MOUNTDEV_SUGGESTED_LINK_NAME, Name) + 28;

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                Irp->IoStatus.Information) {

                Irp->IoStatus.Information =
                        sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);
                status = STATUS_BUFFER_OVERFLOW;
                ExFreePool(valueName);
                break;
            }

            RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                   L"\\Registry\\Machine\\System\\DISK",
                                   valueName);

            ExFreePool(valueName);

            RtlCopyMemory(suggestedName->Name, L"\\DosDevices\\", 24);
            suggestedName->Name[12] = driveLetterName.Buffer[0];
            suggestedName->Name[13] = ':';

            //
            // NT_SUCCESS(status) based on RtlQueryRegistryValues
            //
            status = STATUS_SUCCESS;

            break;
        }

        default:
            status = STATUS_PENDING;
            break;
    }

    if (status != STATUS_PENDING) {
        ClassReleaseRemoveLock(DeviceObject, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    if (commonExtension->IsFdo){

        PULONG_PTR function;

        srb = ExAllocatePoolWithTag(NonPagedPool,
                             sizeof(SCSI_REQUEST_BLOCK) +
                             (sizeof(ULONG_PTR) * 2),
                             '9CcS');

        if (srb == NULL) {

            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto SetStatusAndReturn;
        }

        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

        cdb = (PCDB)srb->Cdb;

        //
        // Save the function code and the device object in the memory after
        // the SRB.
        //

        function = (PULONG_PTR) ((PSCSI_REQUEST_BLOCK) (srb + 1));
        *function = (ULONG_PTR) DeviceObject;
        function++;
        *function = (ULONG_PTR) controlCode;

    } else {
        srb = NULL;
    }

    //
    // Change the device type to storage for the switch statement, but only
    // if from a legacy device type
    //

    if (((controlCode & 0xffff0000) == (IOCTL_DISK_BASE  << 16)) ||
        ((controlCode & 0xffff0000) == (IOCTL_TAPE_BASE  << 16)) ||
        ((controlCode & 0xffff0000) == (IOCTL_CDROM_BASE << 16))
        ) {

        modifiedIoControlCode = (controlCode & ~0xffff0000);
        modifiedIoControlCode |= (IOCTL_STORAGE_BASE << 16);

    } else {

        modifiedIoControlCode = controlCode;

    }

    DBGTRACE(ClassDebugTrace, ("> ioctl %xh (%s)", modifiedIoControlCode, DBGGETIOCTLSTR(modifiedIoControlCode)));

    switch (modifiedIoControlCode) {

    case IOCTL_STORAGE_GET_HOTPLUG_INFO: {

        if (srb) {
            ExFreePool(srb);
            srb = NULL;
        }

        if(irpStack->Parameters.DeviceIoControl.OutputBufferLength <
           sizeof(STORAGE_HOTPLUG_INFO)) {

            //
            // Indicate unsuccessful status and no data transferred.
            //

            Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = sizeof(STORAGE_HOTPLUG_INFO);

            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            status = STATUS_BUFFER_TOO_SMALL;

        } else if(!commonExtension->IsFdo) {

            //
            // Just forward this down and return
            //

            IoCopyCurrentIrpStackLocationToNext(Irp);

            ClassReleaseRemoveLock(DeviceObject, Irp);
            status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);

        } else {

            PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
            PSTORAGE_HOTPLUG_INFO info;

            fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)commonExtension;
            info = Irp->AssociatedIrp.SystemBuffer;

            *info = fdoExtension->PrivateFdoData->HotplugInfo;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(STORAGE_HOTPLUG_INFO);
            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            status = STATUS_SUCCESS;

        }
        break;
    }

    case IOCTL_STORAGE_SET_HOTPLUG_INFO: {

        if (srb)
        {
            ExFreePool(srb);
            srb = NULL;
        }

        if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(STORAGE_HOTPLUG_INFO)) {

            //
            // Indicate unsuccessful status and no data transferred.
            //

            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;

            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            status = STATUS_INFO_LENGTH_MISMATCH;
            goto SetStatusAndReturn;

        }

        if(!commonExtension->IsFdo) {

            //
            // Just forward this down and return
            //

            IoCopyCurrentIrpStackLocationToNext(Irp);

            ClassReleaseRemoveLock(DeviceObject, Irp);
            status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);

        } else {

            PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)commonExtension;
            PSTORAGE_HOTPLUG_INFO info = Irp->AssociatedIrp.SystemBuffer;

            status = STATUS_SUCCESS;

            if (info->Size != fdoExtension->PrivateFdoData->HotplugInfo.Size)
            {
                status = STATUS_INVALID_PARAMETER_1;
            }

            if (info->MediaRemovable != fdoExtension->PrivateFdoData->HotplugInfo.MediaRemovable)
            {
                status = STATUS_INVALID_PARAMETER_2;
            }

            if (info->MediaHotplug != fdoExtension->PrivateFdoData->HotplugInfo.MediaHotplug)
            {
                status = STATUS_INVALID_PARAMETER_3;
            }

            if (info->WriteCacheEnableOverride != fdoExtension->PrivateFdoData->HotplugInfo.WriteCacheEnableOverride)
            {
                status = STATUS_INVALID_PARAMETER_5;
            }

            if (NT_SUCCESS(status))
            {
                fdoExtension->PrivateFdoData->HotplugInfo.DeviceHotplug = info->DeviceHotplug;

                //
                // Store the user-defined override in the registry
                //

                ClassSetDeviceParameter(fdoExtension,
                                        CLASSP_REG_SUBKEY_NAME,
                                        CLASSP_REG_REMOVAL_POLICY_VALUE_NAME,
                                        (info->DeviceHotplug) ? RemovalPolicyExpectSurpriseRemoval : RemovalPolicyExpectOrderlyRemoval);
            }

            Irp->IoStatus.Status = status;

            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
        }

        break;
    }

    case IOCTL_STORAGE_CHECK_VERIFY:
    case IOCTL_STORAGE_CHECK_VERIFY2: {

        PIRP irp2 = NULL;
        PIO_STACK_LOCATION newStack;

        PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = NULL;

        DebugPrint((1,"DeviceIoControl: Check verify\n"));

        //
        // If a buffer for a media change count was provided, make sure it's
        // big enough to hold the result
        //

        if(irpStack->Parameters.DeviceIoControl.OutputBufferLength) {

            //
            // If the buffer is too small to hold the media change count
            // then return an error to the caller
            //

            if(irpStack->Parameters.DeviceIoControl.OutputBufferLength <
               sizeof(ULONG)) {

                DebugPrint((3,"DeviceIoControl: media count "
                              "buffer too small\n"));

                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = sizeof(ULONG);

                if(srb != NULL) {
                    ExFreePool(srb);
                }

                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                status = STATUS_BUFFER_TOO_SMALL;
                goto SetStatusAndReturn;

            }
        }

        if(!commonExtension->IsFdo) {

            //
            // If this is a PDO then we should just forward the request down
            //
            ASSERT(!srb);

            IoCopyCurrentIrpStackLocationToNext(Irp);

            ClassReleaseRemoveLock(DeviceObject, Irp);

            status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);

            goto SetStatusAndReturn;

        } else {

            fdoExtension = DeviceObject->DeviceExtension;

        }

        if(irpStack->Parameters.DeviceIoControl.OutputBufferLength) {

            //
            // The caller has provided a valid buffer.  Allocate an additional
            // irp and stick the CheckVerify completion routine on it.  We will
            // then send this down to the port driver instead of the irp the
            // caller sent in
            //

            DebugPrint((2,"DeviceIoControl: Check verify wants "
                          "media count\n"));

            //
            // Allocate a new irp to send the TestUnitReady to the port driver
            //

            irp2 = IoAllocateIrp((CCHAR) (DeviceObject->StackSize + 3), FALSE);

            if(irp2 == NULL) {
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Information = 0;
                ASSERT(srb);
                ExFreePool(srb);
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto SetStatusAndReturn;

                break;
            }

            //
            // Make sure to acquire the lock for the new irp.
            //

            ClassAcquireRemoveLock(DeviceObject, irp2);

            irp2->Tail.Overlay.Thread = Irp->Tail.Overlay.Thread;
            IoSetNextIrpStackLocation(irp2);

            //
            // Set the top stack location and shove the master Irp into the
            // top location
            //

            newStack = IoGetCurrentIrpStackLocation(irp2);
            newStack->Parameters.Others.Argument1 = Irp;
            newStack->DeviceObject = DeviceObject;

            //
            // Stick the check verify completion routine onto the stack
            // and prepare the irp for the port driver
            //

            IoSetCompletionRoutine(irp2,
                                   ClassCheckVerifyComplete,
                                   NULL,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            IoSetNextIrpStackLocation(irp2);
            newStack = IoGetCurrentIrpStackLocation(irp2);
            newStack->DeviceObject = DeviceObject;
            newStack->MajorFunction = irpStack->MajorFunction;
            newStack->MinorFunction = irpStack->MinorFunction;

            //
            // Mark the master irp as pending - whether the lower level
            // driver completes it immediately or not this should allow it
            // to go all the way back up.
            //

            IoMarkIrpPending(Irp);

            Irp = irp2;

        }

        //
        // Test Unit Ready
        //

        srb->CdbLength = 6;
        cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

        //
        // Set timeout value.
        //

        srb->TimeOutValue = fdoExtension->TimeOutValue;

        //
        // If this was a CV2 then mark the request as low-priority so we don't
        // spin up the drive just to satisfy it.
        //

        if(controlCode == IOCTL_STORAGE_CHECK_VERIFY2) {
            SET_FLAG(srb->SrbFlags, SRB_CLASS_FLAGS_LOW_PRIORITY);
        }

        //
        // Since this routine will always hand the request to the
        // port driver if there isn't a data transfer to be done
        // we don't have to worry about completing the request here
        // on an error
        //

        //
        // This routine uses a completion routine so we don't want to release
        // the remove lock until then.
        //

        status = ClassSendSrbAsynchronous(DeviceObject,
                                          srb,
                                          Irp,
                                          NULL,
                                          0,
                                          FALSE);

        break;
    }

    case IOCTL_STORAGE_MEDIA_REMOVAL:
    case IOCTL_STORAGE_EJECTION_CONTROL: {

        PPREVENT_MEDIA_REMOVAL mediaRemoval = Irp->AssociatedIrp.SystemBuffer;

        DebugPrint((3, "DiskIoControl: ejection control\n"));

        if(srb) {
            ExFreePool(srb);
        }

        if(irpStack->Parameters.DeviceIoControl.InputBufferLength <
           sizeof(PREVENT_MEDIA_REMOVAL)) {

            //
            // Indicate unsuccessful status and no data transferred.
            //

            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;

            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            status = STATUS_INFO_LENGTH_MISMATCH;
            goto SetStatusAndReturn;
        }

        if(!commonExtension->IsFdo) {

            //
            // Just forward this down and return
            //

            IoCopyCurrentIrpStackLocationToNext(Irp);

            ClassReleaseRemoveLock(DeviceObject, Irp);
            status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
        }
        else {

            // i don't believe this assertion is valid.  this is a request
            // from user-mode, so they could request this for any device
            // they want?  also, we handle it properly.
            // ASSERT(TEST_FLAG(DeviceObject->Characteristics, FILE_REMOVABLE_MEDIA));
            status = ClasspEjectionControl(
                        DeviceObject,
                        Irp,
                        ((modifiedIoControlCode ==
                        IOCTL_STORAGE_EJECTION_CONTROL) ? SecureMediaLock :
                                                          SimpleMediaLock),
                        mediaRemoval->PreventMediaRemoval);

            Irp->IoStatus.Status = status;
            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
        }

        break;
    }

    case IOCTL_STORAGE_MCN_CONTROL: {

        DebugPrint((3, "DiskIoControl: MCN control\n"));

        if(irpStack->Parameters.DeviceIoControl.InputBufferLength <
           sizeof(PREVENT_MEDIA_REMOVAL)) {

            //
            // Indicate unsuccessful status and no data transferred.
            //

            Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
            Irp->IoStatus.Information = 0;

            if(srb) {
                ExFreePool(srb);
            }

            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            status = STATUS_INFO_LENGTH_MISMATCH;
            goto SetStatusAndReturn;
        }

        if(!commonExtension->IsFdo) {

            //
            // Just forward this down and return
            //

            if(srb) {
                ExFreePool(srb);
            }

            IoCopyCurrentIrpStackLocationToNext(Irp);

            ClassReleaseRemoveLock(DeviceObject, Irp);
            status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);

        } else {

            //
            // Call to the FDO - handle the ejection control.
            //

            status = ClasspMcnControl(DeviceObject->DeviceExtension,
                                      Irp,
                                      srb);
        }
        goto SetStatusAndReturn;
    }

    case IOCTL_STORAGE_RESERVE:
    case IOCTL_STORAGE_RELEASE: {

        //
        // Reserve logical unit.
        //

        PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = NULL;

        if(!commonExtension->IsFdo) {

            IoCopyCurrentIrpStackLocationToNext(Irp);

            ClassReleaseRemoveLock(DeviceObject, Irp);
            status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
            goto SetStatusAndReturn;
        } else {
            fdoExtension = DeviceObject->DeviceExtension;
        }

        srb->CdbLength = 6;

        if(modifiedIoControlCode == IOCTL_STORAGE_RESERVE) {
            cdb->CDB6GENERIC.OperationCode = SCSIOP_RESERVE_UNIT;
        } else {
            cdb->CDB6GENERIC.OperationCode = SCSIOP_RELEASE_UNIT;
        }

        //
        // Set timeout value.
        //

        srb->TimeOutValue = fdoExtension->TimeOutValue;

        status = ClassSendSrbAsynchronous(DeviceObject,
                                          srb,
                                          Irp,
                                          NULL,
                                          0,
                                          FALSE);

        break;
    }

    case IOCTL_STORAGE_EJECT_MEDIA:
    case IOCTL_STORAGE_LOAD_MEDIA:
    case IOCTL_STORAGE_LOAD_MEDIA2:{

        //
        // Eject media.
        //

        PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = NULL;

        if(!commonExtension->IsFdo) {

            IoCopyCurrentIrpStackLocationToNext(Irp);

            ClassReleaseRemoveLock(DeviceObject, Irp);

            status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
            goto SetStatusAndReturn;
        } else {
            fdoExtension = DeviceObject->DeviceExtension;
        }

        if(commonExtension->PagingPathCount != 0) {

            DebugPrint((1, "ClassDeviceControl: call to eject paging device - "
                           "failure\n"));

            status = STATUS_FILES_OPEN;
            Irp->IoStatus.Status = status;

            Irp->IoStatus.Information = 0;

            if(srb) {
                ExFreePool(srb);
            }

            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            goto SetStatusAndReturn;
        }

        //
        // Synchronize with ejection control and ejection cleanup code as
        // well as other eject/load requests.
        //

        KeEnterCriticalRegion();
        KeWaitForSingleObject(&(fdoExtension->EjectSynchronizationEvent),
                              UserRequest,
                              UserMode,
                              FALSE,
                              NULL);

        if(fdoExtension->ProtectedLockCount != 0) {

            DebugPrint((1, "ClassDeviceControl: call to eject protected locked "
                           "device - failure\n"));

            status = STATUS_DEVICE_BUSY;
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;

            if(srb) {
                ExFreePool(srb);
            }

            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

            KeSetEvent(&fdoExtension->EjectSynchronizationEvent,
                       IO_NO_INCREMENT,
                       FALSE);
            KeLeaveCriticalRegion();

            goto SetStatusAndReturn;
        }

        srb->CdbLength = 6;

        cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
        cdb->START_STOP.LoadEject = 1;

        if(modifiedIoControlCode == IOCTL_STORAGE_EJECT_MEDIA) {
            cdb->START_STOP.Start = 0;
        } else {
            cdb->START_STOP.Start = 1;
        }

        //
        // Set timeout value.
        //

        srb->TimeOutValue = fdoExtension->TimeOutValue;
        status = ClassSendSrbAsynchronous(DeviceObject,
                                              srb,
                                              Irp,
                                              NULL,
                                              0,
                                              FALSE);

        KeSetEvent(&fdoExtension->EjectSynchronizationEvent, IO_NO_INCREMENT, FALSE);
        KeLeaveCriticalRegion();

        break;
    }

    case IOCTL_STORAGE_FIND_NEW_DEVICES: {

        if(srb) {
            ExFreePool(srb);
        }

        if(commonExtension->IsFdo) {

            IoInvalidateDeviceRelations(
                ((PFUNCTIONAL_DEVICE_EXTENSION) commonExtension)->LowerPdo,
                BusRelations);

            status = STATUS_SUCCESS;
            Irp->IoStatus.Status = status;

            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
        }
        else {

            IoCopyCurrentIrpStackLocationToNext(Irp);

            ClassReleaseRemoveLock(DeviceObject, Irp);
            status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
        }
        break;
    }

    case IOCTL_STORAGE_GET_DEVICE_NUMBER: {

        if(srb) {
            ExFreePool(srb);
        }

        if(irpStack->Parameters.DeviceIoControl.OutputBufferLength >=
           sizeof(STORAGE_DEVICE_NUMBER)) {

            PSTORAGE_DEVICE_NUMBER deviceNumber =
                Irp->AssociatedIrp.SystemBuffer;
            PFUNCTIONAL_DEVICE_EXTENSION fdoExtension =
                commonExtension->PartitionZeroExtension;

            deviceNumber->DeviceType = fdoExtension->CommonExtension.DeviceObject->DeviceType;
            deviceNumber->DeviceNumber = fdoExtension->DeviceNumber;
            deviceNumber->PartitionNumber = commonExtension->PartitionNumber;

            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(STORAGE_DEVICE_NUMBER);

        } else {
            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = sizeof(STORAGE_DEVICE_NUMBER);
        }

        Irp->IoStatus.Status = status;
        ClassReleaseRemoveLock(DeviceObject, Irp);
        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

        break;
    }

    default: {

        DebugPrint((4, "IoDeviceControl: Unsupported device IOCTL %x for %p\n",
                    controlCode, DeviceObject));

        //
        // Pass the device control to the next driver.
        //

        if(srb) {
            ExFreePool(srb);
        }

        //
        // Copy the Irp stack parameters to the next stack location.
        //

        IoCopyCurrentIrpStackLocationToNext(Irp);

        ClassReleaseRemoveLock(DeviceObject, Irp);
        status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
        break;
    }

    } // end switch( ...

SetStatusAndReturn:

    DBGTRACE(ClassDebugTrace, ("< ioctl %xh (%s): status %xh.", modifiedIoControlCode, DBGGETIOCTLSTR(modifiedIoControlCode), status));

    return status;
} // end ClassDeviceControl()

/*++////////////////////////////////////////////////////////////////////////////

ClassShutdownFlush()

Routine Description:

    This routine is called for a shutdown and flush IRPs.  These are sent by the
    system before it actually shuts down or when the file system does a flush.
    If it exists, the device-specific driver's routine will be invoked. If there
    wasn't one specified, the Irp will be completed with an Invalid device request.

Arguments:

    DriverObject - Pointer to device object to being shutdown by system.

    Irp - IRP involved.

Return Value:

    NT Status

--*/
NTSTATUS
NTAPI
ClassShutdownFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    ULONG isRemoved;

    //NTSTATUS status;

    isRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);

    if(isRemoved) {

        ClassReleaseRemoveLock(DeviceObject, Irp);

        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;

        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    if (commonExtension->DevInfo->ClassShutdownFlush) {

        //
        // Call the device-specific driver's routine.
        //

        return commonExtension->DevInfo->ClassShutdownFlush(DeviceObject, Irp);
    }

    //
    // Device-specific driver doesn't support this.
    //

    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    return STATUS_INVALID_DEVICE_REQUEST;
} // end ClassShutdownFlush()

/*++////////////////////////////////////////////////////////////////////////////

ClassCreateDeviceObject()

Routine Description:

    This routine creates an object for the physical device specified and
    sets up the deviceExtension's function pointers for each entry point
    in the device-specific driver.

Arguments:

    DriverObject - Pointer to driver object created by system.

    ObjectNameBuffer - Dir. name of the object to create.

    LowerDeviceObject - Pointer to the lower device object

    IsFdo - should this be an fdo or a pdo

    DeviceObject - Pointer to the device object pointer we will return.

Return Value:

    NTSTATUS

--*/
NTSTATUS
NTAPI
ClassCreateDeviceObject(
    IN PDRIVER_OBJECT          DriverObject,
    IN PCCHAR                  ObjectNameBuffer,
    IN PDEVICE_OBJECT          LowerDevice,
    IN BOOLEAN                 IsFdo,
    IN OUT PDEVICE_OBJECT      *DeviceObject
    )
{
    BOOLEAN        isPartitionable;
    STRING         ntNameString;
    UNICODE_STRING ntUnicodeString;
    NTSTATUS       status;
    PDEVICE_OBJECT deviceObject = NULL;

    ULONG          characteristics;

    PCLASS_DRIVER_EXTENSION
        driverExtension = IoGetDriverObjectExtension(DriverObject,
                                                     CLASS_DRIVER_EXTENSION_KEY);

    PCLASS_DEV_INFO devInfo;

    PAGED_CODE();

    *DeviceObject = NULL;
    RtlInitUnicodeString(&ntUnicodeString, NULL);

    DebugPrint((2, "ClassCreateFdo: Create device object\n"));

    ASSERT(LowerDevice);

    //
    // Make sure that if we're making PDO's we have an enumeration routine
    //

    isPartitionable = (driverExtension->InitData.ClassEnumerateDevice != NULL);

    ASSERT(IsFdo || isPartitionable);

    //
    // Grab the correct dev-info structure out of the init data
    //

    if(IsFdo) {
        devInfo = &(driverExtension->InitData.FdoData);
    } else {
        devInfo = &(driverExtension->InitData.PdoData);
    }

    characteristics = devInfo->DeviceCharacteristics;

    if(ARGUMENT_PRESENT(ObjectNameBuffer)) {
        DebugPrint((2, "ClassCreateFdo: Name is %s\n", ObjectNameBuffer));

        RtlInitString(&ntNameString, ObjectNameBuffer);

        status = RtlAnsiStringToUnicodeString(&ntUnicodeString, &ntNameString, TRUE);

        if (!NT_SUCCESS(status)) {

            DebugPrint((1,
                        "ClassCreateFdo: Cannot convert string %s\n",
                        ObjectNameBuffer));

            ntUnicodeString.Buffer = NULL;
            return status;
        }
    } else {
        DebugPrint((2, "ClassCreateFdo: Object will be unnamed\n"));

        if(IsFdo == FALSE) {

            //
            // PDO's have to have some sort of name.
            //

            SET_FLAG(characteristics, FILE_AUTOGENERATED_DEVICE_NAME);
        }

        RtlInitUnicodeString(&ntUnicodeString, NULL);
    }

    status = IoCreateDevice(DriverObject,
                            devInfo->DeviceExtensionSize,
                            &ntUnicodeString,
                            devInfo->DeviceType,
                            devInfo->DeviceCharacteristics,
                            FALSE,
                            &deviceObject);

    if (!NT_SUCCESS(status)) {

        DebugPrint((1, "ClassCreateFdo: Can not create device object %lx\n",
                    status));
        ASSERT(deviceObject == NULL);

        //
        // buffer is not used any longer here.
        //

        if (ntUnicodeString.Buffer != NULL) {
            DebugPrint((1, "ClassCreateFdo: Freeing unicode name buffer\n"));
            ExFreePool(ntUnicodeString.Buffer);
            RtlInitUnicodeString(&ntUnicodeString, NULL);
        }

    } else {

        PCOMMON_DEVICE_EXTENSION commonExtension = deviceObject->DeviceExtension;

        RtlZeroMemory(
            deviceObject->DeviceExtension,
            devInfo->DeviceExtensionSize);

        //
        // Setup version code
        //

        commonExtension->Version = 0x03;

        //
        // Setup the remove lock and event
        //

        commonExtension->IsRemoved = NO_REMOVE;
        commonExtension->RemoveLock = 0;
        KeInitializeEvent(&commonExtension->RemoveEvent,
                          SynchronizationEvent,
                          FALSE);

        #if DBG
            KeInitializeSpinLock(&commonExtension->RemoveTrackingSpinlock);
            commonExtension->RemoveTrackingList = NULL;
        #else
            commonExtension->RemoveTrackingSpinlock = (ULONG_PTR) -1;
            commonExtension->RemoveTrackingList = (PVOID) -1;
        #endif

        //
        // Acquire the lock once.  This reference will be released when the
        // remove IRP has been received.
        //

        ClassAcquireRemoveLock(deviceObject, (PIRP) deviceObject);

        //
        // Store a pointer to the driver extension so we don't have to do
        // lookups to get it.
        //

        commonExtension->DriverExtension = driverExtension;

        //
        // Fill in entry points
        //

        commonExtension->DevInfo = devInfo;

        //
        // Initialize some of the common values in the structure
        //

        commonExtension->DeviceObject = deviceObject;

        commonExtension->LowerDeviceObject = NULL;

        if(IsFdo) {

            PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PVOID) commonExtension;

            commonExtension->PartitionZeroExtension = deviceObject->DeviceExtension;

            //
            // Set the initial device object flags.
            //

            SET_FLAG(deviceObject->Flags, DO_POWER_PAGABLE);

            //
            // Clear the PDO list
            //

            commonExtension->ChildList = NULL;

            commonExtension->DriverData =
                ((PFUNCTIONAL_DEVICE_EXTENSION) deviceObject->DeviceExtension + 1);

            if(isPartitionable) {

                commonExtension->PartitionNumber = 0;
            } else {
                commonExtension->PartitionNumber = (ULONG) (-1L);
            }

            fdoExtension->DevicePowerState = PowerDeviceD0;

            KeInitializeEvent(&fdoExtension->EjectSynchronizationEvent,
                              SynchronizationEvent,
                              TRUE);

            KeInitializeEvent(&fdoExtension->ChildLock,
                              SynchronizationEvent,
                              TRUE);

            status = ClasspAllocateReleaseRequest(deviceObject);

            if(!NT_SUCCESS(status)) {
                IoDeleteDevice(deviceObject);
                *DeviceObject = NULL;

                if (ntUnicodeString.Buffer != NULL) {
                    DebugPrint((1, "ClassCreateFdo: Freeing unicode name buffer\n"));
                    ExFreePool(ntUnicodeString.Buffer);
                    RtlInitUnicodeString(&ntUnicodeString, NULL);
                }

                return status;
            }

        } else {

            PPHYSICAL_DEVICE_EXTENSION pdoExtension =
                deviceObject->DeviceExtension;

            PFUNCTIONAL_DEVICE_EXTENSION p0Extension =
                LowerDevice->DeviceExtension;

            SET_FLAG(deviceObject->Flags, DO_POWER_PAGABLE);

            commonExtension->PartitionZeroExtension = p0Extension;

            //
            // Stick this onto the PDO list
            //

            ClassAddChild(p0Extension, pdoExtension, TRUE);

            commonExtension->DriverData = (PVOID) (pdoExtension + 1);

            //
            // Get the top of stack for the lower device - this allows
            // filters to get stuck in between the partitions and the
            // physical disk.
            //

            commonExtension->LowerDeviceObject =
                IoGetAttachedDeviceReference(LowerDevice);

            //
            // Pnp will keep a reference to the lower device object long
            // after this partition has been deleted.  Dereference now so
            // we don't have to deal with it later.
            //

            ObDereferenceObject(commonExtension->LowerDeviceObject);
        }

        KeInitializeEvent(&commonExtension->PathCountEvent, SynchronizationEvent, TRUE);

        commonExtension->IsFdo = IsFdo;

        commonExtension->DeviceName = ntUnicodeString;

        commonExtension->PreviousState = 0xff;

        InitializeDictionary(&(commonExtension->FileObjectDictionary));

        commonExtension->CurrentState = IRP_MN_STOP_DEVICE;
    }

    *DeviceObject = deviceObject;

    return status;
} // end ClassCreateDeviceObject()

/*++////////////////////////////////////////////////////////////////////////////

ClassClaimDevice()

Routine Description:

    This function claims a device in the port driver.  The port driver object
    is updated with the correct driver object if the device is successfully
    claimed.

Arguments:

    LowerDeviceObject - Supplies the base port device object.

    Release - Indicates the logical unit should be released rather than claimed.

Return Value:

    Returns a status indicating success or failure of the operation.

--*/
NTSTATUS
NTAPI
ClassClaimDevice(
    IN PDEVICE_OBJECT LowerDeviceObject,
    IN BOOLEAN Release
    )
{
    IO_STATUS_BLOCK    ioStatus;
    PIRP               irp;
    PIO_STACK_LOCATION irpStack;
    KEVENT             event;
    NTSTATUS           status;
    SCSI_REQUEST_BLOCK srb;

    PAGED_CODE();

    //
    // Clear the SRB fields.
    //

    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Write length to SRB.
    //

    srb.Length = sizeof(SCSI_REQUEST_BLOCK);

    srb.Function = Release ? SRB_FUNCTION_RELEASE_DEVICE :
        SRB_FUNCTION_CLAIM_DEVICE;

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion
    //

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // Build synchronous request with no transfer.
    //

    irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_EXECUTE_NONE,
                                        LowerDeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &event,
                                        &ioStatus);

    if (irp == NULL) {
        DebugPrint((1, "ClassClaimDevice: Can't allocate Irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpStack = IoGetNextIrpStackLocation(irp);

    //
    // Save SRB address in next stack for port driver.
    //

    irpStack->Parameters.Scsi.Srb = &srb;

    //
    // Set up IRP Address.
    //

    srb.OriginalRequest = irp;

    //
    // Call the port driver with the request and wait for it to complete.
    //

    status = IoCallDriver(LowerDeviceObject, irp);
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    //
    // If this is a release request, then just decrement the reference count
    // and return.  The status does not matter.
    //

    if (Release) {

        // ObDereferenceObject(LowerDeviceObject);
        return STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    ASSERT(srb.DataBuffer != NULL);
    ASSERT(!TEST_FLAG(srb.SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));

    return status;
} // end ClassClaimDevice()

/*++////////////////////////////////////////////////////////////////////////////

ClassInternalIoControl()

Routine Description:

    This routine passes internal device controls to the port driver.
    Internal device controls are used by higher level drivers both for ioctls
    and to pass through scsi requests.

    If the IoControlCode does not match any of the handled ioctls and is
    a valid system address then the request will be treated as an SRB and
    passed down to the lower driver.  If the IoControlCode is not a valid
    system address the ioctl will be failed.

    Callers must therefore be extremely cautious to pass correct, initialized
    values to this function.

Arguments:

    DeviceObject - Supplies a pointer to the device object for this request.

    Irp - Supplies the Irp making the request.

Return Value:

   Returns back a STATUS_PENDING or a completion status.

--*/
NTSTATUS
NTAPI
ClassInternalIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextStack = IoGetNextIrpStackLocation(Irp);

    ULONG isRemoved;

    PSCSI_REQUEST_BLOCK srb;

    isRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);

    if(isRemoved) {

        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;

        ClassReleaseRemoveLock(DeviceObject, Irp);

        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // Get a pointer to the SRB.
    //

    srb = irpStack->Parameters.Scsi.Srb;

    //
    // Set the parameters in the next stack location.
    //

    if(commonExtension->IsFdo) {
        nextStack->Parameters.Scsi.Srb = srb;
        nextStack->MajorFunction = IRP_MJ_SCSI;
        nextStack->MinorFunction = IRP_MN_SCSI_CLASS;

    } else {

        IoCopyCurrentIrpStackLocationToNext(Irp);
    }

    ClassReleaseRemoveLock(DeviceObject, Irp);

    return IoCallDriver(commonExtension->LowerDeviceObject, Irp);
} // end ClassInternalIoControl()

/*++////////////////////////////////////////////////////////////////////////////

ClassQueryTimeOutRegistryValue()

Routine Description:

    This routine determines whether a reg key for a user-specified timeout
    value exists.  This should be called at initialization time.

Arguments:

    DeviceObject - Pointer to the device object we are retrieving the timeout
                   value for

Return Value:

    None, but it sets a new default timeout for a class of devices.

--*/
ULONG
NTAPI
ClassQueryTimeOutRegistryValue(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    //
    // Find the appropriate reg. key
    //

    PCLASS_DRIVER_EXTENSION
        driverExtension = IoGetDriverObjectExtension(DeviceObject->DriverObject,
                                                     CLASS_DRIVER_EXTENSION_KEY);

    PUNICODE_STRING registryPath = &(driverExtension->RegistryPath);

    PRTL_QUERY_REGISTRY_TABLE parameters = NULL;
    PWSTR path;
    NTSTATUS status;
    LONG     timeOut = 0;
    ULONG    zero = 0;
    ULONG    size;

    PAGED_CODE();

    if (!registryPath) {
        return 0;
    }

    parameters = ExAllocatePoolWithTag(NonPagedPool,
                                sizeof(RTL_QUERY_REGISTRY_TABLE)*2,
                                '1BcS');

    if (!parameters) {
        return 0;
    }

    size = registryPath->MaximumLength + sizeof(WCHAR);
    path = ExAllocatePoolWithTag(NonPagedPool, size, '2BcS');

    if (!path) {
        ExFreePool(parameters);
        return 0;
    }

    RtlZeroMemory(path,size);
    RtlCopyMemory(path, registryPath->Buffer, size - sizeof(WCHAR));


    //
    // Check for the Timeout value.
    //

    RtlZeroMemory(parameters,
                  (sizeof(RTL_QUERY_REGISTRY_TABLE)*2));

    parameters[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    parameters[0].Name          = L"TimeOutValue";
    parameters[0].EntryContext  = &timeOut;
    parameters[0].DefaultType   = REG_DWORD;
    parameters[0].DefaultData   = &zero;
    parameters[0].DefaultLength = sizeof(ULONG);

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                    path,
                                    parameters,
                                    NULL,
                                    NULL);

    if (!(NT_SUCCESS(status))) {
        timeOut = 0;
    }

    ExFreePool(parameters);
    ExFreePool(path);

    DebugPrint((2,
                "ClassQueryTimeOutRegistryValue: Timeout value %d\n",
                timeOut));


    return timeOut;

} // end ClassQueryTimeOutRegistryValue()

/*++////////////////////////////////////////////////////////////////////////////

ClassCheckVerifyComplete() ISSUE-2000/02/18-henrygab - why public?!

Routine Description:

    This routine executes when the port driver has completed a check verify
    ioctl.  It will set the status of the master Irp, copy the media change
    count and complete the request.

Arguments:

    Fdo - Supplies the functional device object which represents the logical unit.

    Irp - Supplies the Irp which has completed.

    Context - NULL

Return Value:

    NT status

--*/
NTSTATUS
NTAPI
ClassCheckVerifyComplete(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;

    PIRP originalIrp;

    ASSERT_FDO(Fdo);

    originalIrp = irpStack->Parameters.Others.Argument1;

    //
    // Copy the media change count and status
    //

    *((PULONG) (originalIrp->AssociatedIrp.SystemBuffer)) =
        fdoExtension->MediaChangeCount;

    DebugPrint((2, "ClassCheckVerifyComplete - Media change count for"
                   "device %d is %lx - saved as %lx\n",
                fdoExtension->DeviceNumber,
                fdoExtension->MediaChangeCount,
                *((PULONG) originalIrp->AssociatedIrp.SystemBuffer)));

    originalIrp->IoStatus.Status = Irp->IoStatus.Status;
    originalIrp->IoStatus.Information = sizeof(ULONG);

    ClassReleaseRemoveLock(Fdo, originalIrp);
    ClassCompleteRequest(Fdo, originalIrp, IO_DISK_INCREMENT);

    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;

} // end ClassCheckVerifyComplete()

/*++////////////////////////////////////////////////////////////////////////////

ClassGetDescriptor()

Routine Description:

    This routine will perform a query for the specified property id and will
    allocate a non-paged buffer to store the data in.  It is the responsibility
    of the caller to ensure that this buffer is freed.

    This routine must be run at IRQL_PASSIVE_LEVEL

Arguments:

    DeviceObject - the device to query
    DeviceInfo - a location to store a pointer to the buffer we allocate

Return Value:

    status
    if status is unsuccessful *DeviceInfo will be set to NULL, else the
    buffer allocated on behalf of the caller.

--*/
NTSTATUS
NTAPI
ClassGetDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PSTORAGE_PROPERTY_ID PropertyId,
    OUT PSTORAGE_DESCRIPTOR_HEADER *Descriptor
    )
{
    STORAGE_PROPERTY_QUERY query;
    IO_STATUS_BLOCK ioStatus;

    PSTORAGE_DESCRIPTOR_HEADER descriptor = NULL;
    ULONG length;

    //UCHAR pass = 0;

    PAGED_CODE();

    //
    // Set the passed-in descriptor pointer to NULL as default
    //

    *Descriptor = NULL;


    RtlZeroMemory(&query, sizeof(STORAGE_PROPERTY_QUERY));
    query.PropertyId = *PropertyId;
    query.QueryType = PropertyStandardQuery;

    //
    // On the first pass we just want to get the first few
    // bytes of the descriptor so we can read it's size
    //

    descriptor = (PVOID)&query;

    ASSERT(sizeof(STORAGE_PROPERTY_QUERY) >= (sizeof(ULONG)*2));

    ClassSendDeviceIoControlSynchronous(
        IOCTL_STORAGE_QUERY_PROPERTY,
        DeviceObject,
        &query,
        sizeof(STORAGE_PROPERTY_QUERY),
        sizeof(ULONG) * 2,
        FALSE,
        &ioStatus
        );

    if(!NT_SUCCESS(ioStatus.Status)) {

        DebugPrint((1, "ClassGetDescriptor: error %lx trying to "
                       "query properties #1\n", ioStatus.Status));
        return ioStatus.Status;
    }

    if (descriptor->Size == 0) {

        //
        // This DebugPrint is to help third-party driver writers
        //

        DebugPrint((0, "ClassGetDescriptor: size returned was zero?! (status "
                    "%x\n", ioStatus.Status));
        return STATUS_UNSUCCESSFUL;

    }

    //
    // This time we know how much data there is so we can
    // allocate a buffer of the correct size
    //

    length = descriptor->Size;

    descriptor = ExAllocatePoolWithTag(NonPagedPool, length, '4BcS');

    if(descriptor == NULL) {

        DebugPrint((1, "ClassGetDescriptor: unable to memory for descriptor "
                    "(%d bytes)\n", length));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // setup the query again, as it was overwritten above
    //

    RtlZeroMemory(&query, sizeof(STORAGE_PROPERTY_QUERY));
    query.PropertyId = *PropertyId;
    query.QueryType = PropertyStandardQuery;

    //
    // copy the input to the new outputbuffer
    //

    RtlCopyMemory(descriptor,
                  &query,
                  sizeof(STORAGE_PROPERTY_QUERY)
                  );

    ClassSendDeviceIoControlSynchronous(
        IOCTL_STORAGE_QUERY_PROPERTY,
        DeviceObject,
        descriptor,
        sizeof(STORAGE_PROPERTY_QUERY),
        length,
        FALSE,
        &ioStatus
        );

    if(!NT_SUCCESS(ioStatus.Status)) {

        DebugPrint((1, "ClassGetDescriptor: error %lx trying to "
                       "query properties #1\n", ioStatus.Status));
        ExFreePool(descriptor);
        return ioStatus.Status;
    }

    //
    // return the memory we've allocated to the caller
    //

    *Descriptor = descriptor;
    return ioStatus.Status;
} // end ClassGetDescriptor()

/*++////////////////////////////////////////////////////////////////////////////

ClassSignalCompletion()

Routine Description:

    This completion routine will signal the event given as context and then
    return STATUS_MORE_PROCESSING_REQUIRED to stop event completion.  It is
    the responsibility of the routine waiting on the event to complete the
    request and free the event.

Arguments:

    DeviceObject - a pointer to the device object

    Irp - a pointer to the irp

    Event - a pointer to the event to signal

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/
NTSTATUS
NTAPI
ClassSignalCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PKEVENT Event = Context;

    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
} // end ClassSignalCompletion()

/*++////////////////////////////////////////////////////////////////////////////

ClassPnpQueryFdoRelations()

Routine Description:

    This routine will call the driver's enumeration routine to update the
    list of PDO's.  It will then build a response to the
    IRP_MN_QUERY_DEVICE_RELATIONS and place it into the information field in
    the irp.

Arguments:

    Fdo - a pointer to the functional device object we are enumerating

    Irp - a pointer to the enumeration request

Return Value:

    status

--*/
NTSTATUS
NTAPI
ClassPnpQueryFdoRelations(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCLASS_DRIVER_EXTENSION
        driverExtension = IoGetDriverObjectExtension(Fdo->DriverObject,
                                                     CLASS_DRIVER_EXTENSION_KEY);

    PAGED_CODE();

    //
    // If there's already an enumeration in progress then don't start another
    // one.
    //

    if(InterlockedIncrement((PLONG)&fdoExtension->EnumerationInterlock) == 1) {
        driverExtension->InitData.ClassEnumerateDevice(Fdo);
    }

    Irp->IoStatus.Information = (ULONG_PTR) NULL;

    Irp->IoStatus.Status = ClassRetrieveDeviceRelations(
                                Fdo,
                                BusRelations,
                                (PDEVICE_RELATIONS*)&Irp->IoStatus.Information);
    InterlockedDecrement((PLONG)&fdoExtension->EnumerationInterlock);

    return Irp->IoStatus.Status;
} // end ClassPnpQueryFdoRelations()

/*++////////////////////////////////////////////////////////////////////////////

ClassMarkChildrenMissing()

Routine Description:

    This routine will call ClassMarkChildMissing() for all children.
    It acquires the ChildLock before calling ClassMarkChildMissing().

Arguments:

    Fdo - the "bus's" device object, such as the disk FDO for non-removable
        disks with multiple partitions.

Return Value:

    None

--*/
VOID
NTAPI
ClassMarkChildrenMissing(
    IN PFUNCTIONAL_DEVICE_EXTENSION Fdo
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = &(Fdo->CommonExtension);
    PPHYSICAL_DEVICE_EXTENSION nextChild = commonExtension->ChildList;

    PAGED_CODE();

    ClassAcquireChildLock(Fdo);

    while (nextChild){
        PPHYSICAL_DEVICE_EXTENSION tmpChild;

        /*
         *  ClassMarkChildMissing will also dequeue the child extension.
         *  So get the next pointer before calling ClassMarkChildMissing.
         */
        tmpChild = nextChild;
        nextChild = tmpChild->CommonExtension.ChildList;
        ClassMarkChildMissing(tmpChild, FALSE);
    }
    ClassReleaseChildLock(Fdo);
    return;
} // end ClassMarkChildrenMissing()

/*++////////////////////////////////////////////////////////////////////////////

ClassMarkChildMissing()

Routine Description:

    This routine will make an active child "missing."  If the device has never
    been enumerated then it will be deleted on the spot.  If the device has
    not been enumerated then it will be marked as missing so that we can
    not report it in the next device enumeration.

Arguments:

    Child - the child device to be marked as missing.

    AcquireChildLock - TRUE if the child lock should be acquired before removing
                       the missing child.  FALSE if the child lock is already
                       acquired by this thread.

Return Value:

    returns whether or not the child device object has previously been reported
    to PNP.

--*/
BOOLEAN
NTAPI
ClassMarkChildMissing(
    IN PPHYSICAL_DEVICE_EXTENSION Child,
    IN BOOLEAN AcquireChildLock
    )
{
    BOOLEAN returnValue = Child->IsEnumerated;

    PAGED_CODE();
    ASSERT_PDO(Child->DeviceObject);

    Child->IsMissing = TRUE;

    //
    // Make sure this child is not in the active list.
    //

    ClassRemoveChild(Child->CommonExtension.PartitionZeroExtension,
                     Child,
                     AcquireChildLock);

    if(Child->IsEnumerated == FALSE) {
        ClassRemoveDevice(Child->DeviceObject, IRP_MN_REMOVE_DEVICE);
    }

    return returnValue;
} // end ClassMarkChildMissing()

/*++////////////////////////////////////////////////////////////////////////////

ClassRetrieveDeviceRelations()

Routine Description:

    This routine will allocate a buffer to hold the specified list of
    relations.  It will then fill in the list with referenced device pointers
    and will return the request.

Arguments:

    Fdo - pointer to the FDO being queried

    RelationType - what type of relations are being queried

    DeviceRelations - a location to store a pointer to the response

Return Value:

    status

--*/
NTSTATUS
NTAPI
ClassRetrieveDeviceRelations(
    IN PDEVICE_OBJECT Fdo,
    IN DEVICE_RELATION_TYPE RelationType,
    OUT PDEVICE_RELATIONS *DeviceRelations
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;

    ULONG count = 0;
    ULONG i;

    PPHYSICAL_DEVICE_EXTENSION nextChild;

    ULONG relationsSize;
    PDEVICE_RELATIONS deviceRelations = NULL;

    NTSTATUS status;

    PAGED_CODE();

    ClassAcquireChildLock(fdoExtension);

    nextChild = fdoExtension->CommonExtension.ChildList;

    //
    // Count the number of PDO's attached to this disk
    //

    while(nextChild != NULL) {
        PCOMMON_DEVICE_EXTENSION commonExtension;

        commonExtension = &(nextChild->CommonExtension);

        ASSERTMSG("ClassPnp internal error: missing child on active list\n",
                  (nextChild->IsMissing == FALSE));

        nextChild = commonExtension->ChildList;

        count++;
    };

    relationsSize = (sizeof(DEVICE_RELATIONS) +
                     (count * sizeof(PDEVICE_OBJECT)));

    deviceRelations = ExAllocatePoolWithTag(PagedPool, relationsSize, '5BcS');

    if(deviceRelations == NULL) {

        DebugPrint((1, "ClassRetrieveDeviceRelations: unable to allocate "
                       "%d bytes for device relations\n", relationsSize));

        ClassReleaseChildLock(fdoExtension);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(deviceRelations, relationsSize);

    nextChild = fdoExtension->CommonExtension.ChildList;
    i = count - 1;

    while(nextChild != NULL) {
        PCOMMON_DEVICE_EXTENSION commonExtension;

        commonExtension = &(nextChild->CommonExtension);

        ASSERTMSG("ClassPnp internal error: missing child on active list\n",
                  (nextChild->IsMissing == FALSE));

        deviceRelations->Objects[i--] = nextChild->DeviceObject;

        status = ObReferenceObjectByPointer(
                    nextChild->DeviceObject,
                    0,
                    NULL,
                    KernelMode);
        ASSERT(NT_SUCCESS(status));

        nextChild->IsEnumerated = TRUE;
        nextChild = commonExtension->ChildList;
    }

    ASSERTMSG("Child list has changed: ", i == -1);

    deviceRelations->Count = count;
    *DeviceRelations = deviceRelations;
    ClassReleaseChildLock(fdoExtension);
    return STATUS_SUCCESS;
} // end ClassRetrieveDeviceRelations()

/*++////////////////////////////////////////////////////////////////////////////

ClassGetPdoId()

Routine Description:

    This routine will call into the driver to retrieve a copy of one of it's
    id strings.

Arguments:

    Pdo - a pointer to the pdo being queried

    IdType - which type of id string is being queried

    IdString - an allocated unicode string structure which the driver
               can fill in.

Return Value:

    status

--*/
NTSTATUS
NTAPI
ClassGetPdoId(
    IN PDEVICE_OBJECT Pdo,
    IN BUS_QUERY_ID_TYPE IdType,
    IN PUNICODE_STRING IdString
    )
{
    PCLASS_DRIVER_EXTENSION
        driverExtension = IoGetDriverObjectExtension(Pdo->DriverObject,
                                                     CLASS_DRIVER_EXTENSION_KEY);

    ASSERT_PDO(Pdo);
    ASSERT(driverExtension->InitData.ClassQueryId);

    PAGED_CODE();

    return driverExtension->InitData.ClassQueryId( Pdo, IdType, IdString);
} // end ClassGetPdoId()

/*++////////////////////////////////////////////////////////////////////////////

ClassQueryPnpCapabilities()

Routine Description:

    This routine will call into the class driver to retrieve it's pnp
    capabilities.

Arguments:

    PhysicalDeviceObject - The physical device object to retrieve properties
                           for.

Return Value:

    status

--*/
NTSTATUS
NTAPI
ClassQueryPnpCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_CAPABILITIES Capabilities
    )
{
    PCLASS_DRIVER_EXTENSION driverExtension =
        ClassGetDriverExtension(DeviceObject->DriverObject);
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PCLASS_QUERY_PNP_CAPABILITIES queryRoutine = NULL;

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Capabilities);

    if(commonExtension->IsFdo) {
        queryRoutine = driverExtension->InitData.FdoData.ClassQueryPnpCapabilities;
    } else {
        queryRoutine = driverExtension->InitData.PdoData.ClassQueryPnpCapabilities;
    }

    if(queryRoutine) {
        return queryRoutine(DeviceObject,
                            Capabilities);
    } else {
        return STATUS_NOT_IMPLEMENTED;
    }
} // end ClassQueryPnpCapabilities()

/*++////////////////////////////////////////////////////////////////////////////

ClassInvalidateBusRelations()

Routine Description:

    This routine re-enumerates the devices on the "bus".  It will call into
    the driver's ClassEnumerate routine to update the device objects
    immediately.  It will then schedule a bus re-enumeration for pnp by calling
    IoInvalidateDeviceRelations.

Arguments:

    Fdo - a pointer to the functional device object for this bus

Return Value:

    none

--*/
VOID
NTAPI
ClassInvalidateBusRelations(
    IN PDEVICE_OBJECT Fdo
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCLASS_DRIVER_EXTENSION
        driverExtension = IoGetDriverObjectExtension(Fdo->DriverObject,
                                                     CLASS_DRIVER_EXTENSION_KEY);

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT_FDO(Fdo);
    ASSERT(driverExtension->InitData.ClassEnumerateDevice != NULL);

    if(InterlockedIncrement((PLONG)&fdoExtension->EnumerationInterlock) == 1) {
        status = driverExtension->InitData.ClassEnumerateDevice(Fdo);
    }
    InterlockedDecrement((PLONG)&fdoExtension->EnumerationInterlock);

    if(!NT_SUCCESS(status)) {

        DebugPrint((1, "ClassInvalidateBusRelations: EnumerateDevice routine "
                       "returned %lx\n", status));
    }

    IoInvalidateDeviceRelations(fdoExtension->LowerPdo, BusRelations);

    return;
} // end ClassInvalidateBusRelations()

/*++////////////////////////////////////////////////////////////////////////////

ClassRemoveDevice() ISSUE-2000/02/18-henrygab - why public?!

Routine Description:

    This routine is called to handle the "removal" of a device.  It will
    forward the request downwards if necessary, call into the driver
    to release any necessary resources (memory, events, etc) and then
    will delete the device object.

Arguments:

    DeviceObject - a pointer to the device object being removed

    RemoveType - indicates what type of remove this is (regular or surprise).

Return Value:

    status

--*/
NTSTATUS
NTAPI
ClassRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR RemoveType
    )
{
    PCLASS_DRIVER_EXTENSION
        driverExtension = IoGetDriverObjectExtension(DeviceObject->DriverObject,
                                                     CLASS_DRIVER_EXTENSION_KEY);
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT lowerDeviceObject = commonExtension->LowerDeviceObject;
    PCLASS_WMI_INFO classWmiInfo;
    BOOLEAN proceedWithRemove = TRUE;
    NTSTATUS status;

    PAGED_CODE();

    commonExtension->IsRemoved = REMOVE_PENDING;

    /*
     *  Deregister from WMI.
     */
    classWmiInfo = commonExtension->IsFdo ?
                            &driverExtension->InitData.FdoData.ClassWmiInfo :
                            &driverExtension->InitData.PdoData.ClassWmiInfo;
    if (classWmiInfo->GuidRegInfo){
        status = IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_DEREGISTER);
        DBGTRACE(ClassDebugInfo, ("ClassRemoveDevice: IoWMIRegistrationControl(%p, WMI_ACTION_DEREGISTER) --> %lx", DeviceObject, status));
    }

    /*
     *  If we exposed a "shingle" (a named device interface openable by CreateFile)
     *  then delete it now.
     */
    if (commonExtension->MountedDeviceInterfaceName.Buffer){
        IoSetDeviceInterfaceState(&commonExtension->MountedDeviceInterfaceName, FALSE);
        RtlFreeUnicodeString(&commonExtension->MountedDeviceInterfaceName);
        RtlInitUnicodeString(&commonExtension->MountedDeviceInterfaceName, NULL);
    }

    //
    // If this is a surprise removal we leave the device around - which means
    // we don't have to (or want to) drop the remove lock and wait for pending
    // requests to complete.
    //

    if (RemoveType == IRP_MN_REMOVE_DEVICE){

        //
        // Release the lock we acquired when the device object was created.
        //

        ClassReleaseRemoveLock(DeviceObject, (PIRP) DeviceObject);

        DebugPrint((1, "ClasspRemoveDevice - Reference count is now %d\n",
                    commonExtension->RemoveLock));

        KeWaitForSingleObject(&commonExtension->RemoveEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        DebugPrint((1, "ClasspRemoveDevice - removing device %p\n", DeviceObject));

        if(commonExtension->IsFdo) {

            DebugPrint((1, "ClasspRemoveDevice - FDO %p has received a "
                           "remove request.\n", DeviceObject));

        }
        else {
            PPHYSICAL_DEVICE_EXTENSION pdoExtension = DeviceObject->DeviceExtension;

            if (pdoExtension->IsMissing){
                /*
                 *  The child partition PDO is missing, so we are going to go ahead
                 *  and delete it for the remove.
                 */
                DBGTRACE(ClassDebugWarning, ("ClasspRemoveDevice - PDO %p is missing and will be removed", DeviceObject));
            }
            else {
                /*
                 *  We got a remove for a child partition PDO which is not actually missing.
                 *  So we will NOT actually delete it.
                 */
                DBGTRACE(ClassDebugWarning, ("ClasspRemoveDevice - PDO %p still exists and will be removed when it disappears", DeviceObject));

                //
                // Reacquire the remove lock for the next time this comes around.
                //

                ClassAcquireRemoveLock(DeviceObject, (PIRP) DeviceObject);

                //
                // the device wasn't missing so it's not really been removed.
                //

                commonExtension->IsRemoved = NO_REMOVE;

                IoInvalidateDeviceRelations(
                    commonExtension->PartitionZeroExtension->LowerPdo,
                    BusRelations);

                proceedWithRemove = FALSE;
            }
        }
    }


    if (proceedWithRemove){

        /*
         *  Call the class driver's remove handler.
         *  All this is supposed to do is clean up its data and device interfaces.
         */
        ASSERT(commonExtension->DevInfo->ClassRemoveDevice);
        status = commonExtension->DevInfo->ClassRemoveDevice(DeviceObject, RemoveType);
        ASSERT(NT_SUCCESS(status));
        status = STATUS_SUCCESS;

        if (commonExtension->IsFdo){
            //PDEVICE_OBJECT pdo;
            PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;

            ClasspDisableTimer(fdoExtension->DeviceObject);

            if (RemoveType == IRP_MN_REMOVE_DEVICE){

                PPHYSICAL_DEVICE_EXTENSION child;

                //
                // Cleanup the media detection resources now that the class driver
                // has stopped it's timer (if any) and we can be sure they won't
                // call us to do detection again.
                //

                ClassCleanupMediaChangeDetection(fdoExtension);

                //
                // Cleanup any Failure Prediction stuff
                //
                if (fdoExtension->FailurePredictionInfo) {
                    ExFreePool(fdoExtension->FailurePredictionInfo);
                    fdoExtension->FailurePredictionInfo = NULL;
                }

                /*
                 *  Ordinarily all child PDOs will be removed by the time
                 *  that the parent gets the REMOVE_DEVICE.
                 *  However, if a child PDO has been created but has not
                 *  been announced in a QueryDeviceRelations, then it is
                 *  just a private data structure unknown to pnp, and we have
                 *  to delete it ourselves.
                 */
                ClassAcquireChildLock(fdoExtension);
                while ((child = ClassRemoveChild(fdoExtension, NULL, FALSE))){

                    //
                    // Yank the pdo.  This routine will unlink the device from the
                    // pdo list so NextPdo will point to the next one when it's
                    // complete.
                    //
                    child->IsMissing = TRUE;
                    ClassRemoveDevice(child->DeviceObject, IRP_MN_REMOVE_DEVICE);
                }
                ClassReleaseChildLock(fdoExtension);
            }
            else if (RemoveType == IRP_MN_SURPRISE_REMOVAL){
                /*
                 *  This is a surprise-remove on the parent FDO.
                 *  We will mark the child PDOs as missing so that they
                 *  will actually get deleted when they get a REMOVE_DEVICE.
                 */
                ClassMarkChildrenMissing(fdoExtension);
            }

            ClasspFreeReleaseRequest(DeviceObject);

            if (RemoveType == IRP_MN_REMOVE_DEVICE){

                //
                // Free FDO-specific data structs
                //
                if (fdoExtension->PrivateFdoData){

                    DestroyAllTransferPackets(DeviceObject);

                    ExFreePool(fdoExtension->PrivateFdoData);
                    fdoExtension->PrivateFdoData = NULL;
                }

                if (commonExtension->DeviceName.Buffer) {
                    ExFreePool(commonExtension->DeviceName.Buffer);
                    RtlInitUnicodeString(&commonExtension->DeviceName, NULL);
                }

                if (fdoExtension->AdapterDescriptor) {
                    ExFreePool(fdoExtension->AdapterDescriptor);
                    fdoExtension->AdapterDescriptor = NULL;
                }

                if (fdoExtension->DeviceDescriptor) {
                    ExFreePool(fdoExtension->DeviceDescriptor);
                    fdoExtension->DeviceDescriptor = NULL;
                }

                //
                // Detach our device object from the stack - there's no reason
                // to hold off our cleanup any longer.
                //

                IoDetachDevice(lowerDeviceObject);
            }
        }
        else {
            /*
             *  This is a child partition PDO.
             *  We have already determined that it was previously marked
             *  as missing.  So if this is a REMOVE_DEVICE, we will actually
             *  delete it.
             */
            if (RemoveType == IRP_MN_REMOVE_DEVICE){
                PFUNCTIONAL_DEVICE_EXTENSION fdoExtension =
                    commonExtension->PartitionZeroExtension;
                PPHYSICAL_DEVICE_EXTENSION pdoExtension =
                    (PPHYSICAL_DEVICE_EXTENSION) commonExtension;

                //
                // See if this device is in the child list (if this was a surprise
                // removal it might be) and remove it.
                //

                ClassRemoveChild(fdoExtension, pdoExtension, TRUE);
            }
        }

        commonExtension->PartitionLength.QuadPart = 0;

        if (RemoveType == IRP_MN_REMOVE_DEVICE){
            IoDeleteDevice(DeviceObject);
        }
    }

    return STATUS_SUCCESS;
} // end ClassRemoveDevice()

/*++////////////////////////////////////////////////////////////////////////////

ClassGetDriverExtension()

Routine Description:

    This routine will return the classpnp's driver extension.

Arguments:

    DriverObject - the driver object for which to get classpnp's extension

Return Value:

    Either NULL if none, or a pointer to the driver extension

--*/
PCLASS_DRIVER_EXTENSION
NTAPI
ClassGetDriverExtension(
    IN PDRIVER_OBJECT DriverObject
    )
{
    return IoGetDriverObjectExtension(DriverObject, CLASS_DRIVER_EXTENSION_KEY);
} // end ClassGetDriverExtension()

/*++////////////////////////////////////////////////////////////////////////////

ClasspStartIo()

Routine Description:

    This routine wraps the class driver's start io routine.  If the device
    is being removed it will complete any requests with
    STATUS_DEVICE_DOES_NOT_EXIST and fire up the next packet.

Arguments:

Return Value:

    none

--*/
VOID
NTAPI
ClasspStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    //
    // We're already holding the remove lock so just check the variable and
    // see what's going on.
    //

    if(commonExtension->IsRemoved) {

        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;

        ClassAcquireRemoveLock(DeviceObject, (PIRP) ClasspStartIo);
        ClassReleaseRemoveLock(DeviceObject, Irp);
        ClassCompleteRequest(DeviceObject, Irp, IO_DISK_INCREMENT);
        IoStartNextPacket(DeviceObject, FALSE);
        ClassReleaseRemoveLock(DeviceObject, (PIRP) ClasspStartIo);
        return;
    }

    commonExtension->DriverExtension->InitData.ClassStartIo(
        DeviceObject,
        Irp);

    return;
} // ClasspStartIo()

/*++////////////////////////////////////////////////////////////////////////////

ClassUpdateInformationInRegistry()

Routine Description:

    This routine has knowledge about the layout of the device map information
    in the registry.  It will update this information to include a value
    entry specifying the dos device name that is assumed to get assigned
    to this NT device name.  For more information on this assigning of the
    dos device name look in the drive support routine in the hal that assigns
    all dos names.

    Since some versions of some device's firmware did not work and some
    vendors did not bother to follow the specification, the entire inquiry
    information must also be stored in the registry so than someone can
    figure out the firmware version.

Arguments:

    DeviceObject - A pointer to the device object for the tape device.

Return Value:

    None

--*/
VOID
NTAPI
ClassUpdateInformationInRegistry(
    IN PDEVICE_OBJECT     Fdo,
    IN PCHAR              DeviceName,
    IN ULONG              DeviceNumber,
    IN PINQUIRYDATA       InquiryData,
    IN ULONG              InquiryDataLength
    )
{
    NTSTATUS          status;
    SCSI_ADDRESS      scsiAddress;
    OBJECT_ATTRIBUTES objectAttributes;
    PSTR              buffer;
    STRING            string;
    UNICODE_STRING    unicodeName;
    UNICODE_STRING    unicodeRegistryPath;
    UNICODE_STRING    unicodeData;
    HANDLE            targetKey;
    IO_STATUS_BLOCK   ioStatus;


    PAGED_CODE();

    ASSERT(DeviceName);
    buffer = NULL;
    targetKey = NULL;
    RtlZeroMemory(&unicodeName,         sizeof(UNICODE_STRING));
    RtlZeroMemory(&unicodeData,         sizeof(UNICODE_STRING));
    RtlZeroMemory(&unicodeRegistryPath, sizeof(UNICODE_STRING));

    TRY {

        //
        // Issue GET_ADDRESS Ioctl to determine path, target, and lun information.
        //

        ClassSendDeviceIoControlSynchronous(
            IOCTL_SCSI_GET_ADDRESS,
            Fdo,
            &scsiAddress,
            0,
            sizeof(SCSI_ADDRESS),
            FALSE,
            &ioStatus
            );

        if (!NT_SUCCESS(ioStatus.Status)) {

            status = ioStatus.Status;
            DebugPrint((1,
                        "UpdateInformationInRegistry: Get Address failed %lx\n",
                        status));
            LEAVE;

        } else {

            DebugPrint((1,
                        "GetAddress: Port %x, Path %x, Target %x, Lun %x\n",
                        scsiAddress.PortNumber,
                        scsiAddress.PathId,
                        scsiAddress.TargetId,
                        scsiAddress.Lun));

        }

        //
        // Allocate a buffer for the reg. spooge.
        //

        buffer = ExAllocatePoolWithTag(PagedPool, 1024, '6BcS');

        if (buffer == NULL) {

            //
            // There is not return value for this.  Since this is done at
            // claim device time (currently only system initialization) getting
            // the registry information correct will be the least of the worries.
            //

            LEAVE;
        }

        sprintf(buffer,
                "\\Registry\\Machine\\Hardware\\DeviceMap\\Scsi\\Scsi Port %d\\Scsi Bus %d\\Target Id %d\\Logical Unit Id %d",
                scsiAddress.PortNumber,
                scsiAddress.PathId,
                scsiAddress.TargetId,
                scsiAddress.Lun);

        RtlInitString(&string, buffer);

        status = RtlAnsiStringToUnicodeString(&unicodeRegistryPath,
                                              &string,
                                              TRUE);

        if (!NT_SUCCESS(status)) {
            LEAVE;
        }

        //
        // Open the registry key for the scsi information for this
        // scsibus, target, lun.
        //

        InitializeObjectAttributes(&objectAttributes,
                                   &unicodeRegistryPath,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        status = ZwOpenKey(&targetKey,
                           KEY_READ | KEY_WRITE,
                           &objectAttributes);

        if (!NT_SUCCESS(status)) {
            LEAVE;
        }

        //
        // Now construct and attempt to create the registry value
        // specifying the device name in the appropriate place in the
        // device map.
        //

        RtlInitUnicodeString(&unicodeName, L"DeviceName");

        sprintf(buffer, "%s%lu", DeviceName, DeviceNumber);
        RtlInitString(&string, buffer);
        status = RtlAnsiStringToUnicodeString(&unicodeData,
                                              &string,
                                              TRUE);
        if (NT_SUCCESS(status)) {
            status = ZwSetValueKey(targetKey,
                                   &unicodeName,
                                   0,
                                   REG_SZ,
                                   unicodeData.Buffer,
                                   unicodeData.Length);
        }

        //
        // if they sent in data, update the registry
        //

        if (InquiryDataLength) {

            ASSERT(InquiryData);

            RtlInitUnicodeString(&unicodeName, L"InquiryData");
            status = ZwSetValueKey(targetKey,
                                   &unicodeName,
                                   0,
                                   REG_BINARY,
                                   InquiryData,
                                   InquiryDataLength);
        }

        // that's all, except to clean up.

    } FINALLY {

        if (unicodeData.Buffer) {
            RtlFreeUnicodeString(&unicodeData);
        }
        if (unicodeRegistryPath.Buffer) {
            RtlFreeUnicodeString(&unicodeRegistryPath);
        }
        if (targetKey) {
            ZwClose(targetKey);
        }
        if (buffer) {
            ExFreePool(buffer);
        }

    }

} // end ClassUpdateInformationInRegistry()

/*++////////////////////////////////////////////////////////////////////////////

ClasspSendSynchronousCompletion()

Routine Description:

    This completion routine will set the user event in the irp after
    freeing the irp and the associated MDL (if any).

Arguments:

    DeviceObject - the device object which requested the completion routine

    Irp - the irp being completed

    Context - unused

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/
NTSTATUS
NTAPI
ClasspSendSynchronousCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    DebugPrint((3, "ClasspSendSynchronousCompletion: %p %p %p\n",
                   DeviceObject, Irp, Context));
    //
    // First set the status and information fields in the io status block
    // provided by the caller.
    //

    *(Irp->UserIosb) = Irp->IoStatus;

    //
    // Unlock the pages for the data buffer.
    //

    if(Irp->MdlAddress) {
        MmUnlockPages(Irp->MdlAddress);
        IoFreeMdl(Irp->MdlAddress);
    }

    //
    // Signal the caller's event.
    //

    KeSetEvent(Irp->UserEvent, IO_NO_INCREMENT, FALSE);

    //
    // Free the MDL and the IRP.
    //

    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
} // end ClasspSendSynchronousCompletion()

/*++

    ISSUE-2000/02/20-henrygab Not documented ClasspRegisterMountedDeviceInterface

--*/
VOID
NTAPI
ClasspRegisterMountedDeviceInterface(
    IN PDEVICE_OBJECT DeviceObject
    )
{

    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    BOOLEAN isFdo = commonExtension->IsFdo;

    PDEVICE_OBJECT pdo;
    UNICODE_STRING interfaceName;

    NTSTATUS status;

    if(isFdo) {

        PFUNCTIONAL_DEVICE_EXTENSION functionalExtension;

        functionalExtension =
            (PFUNCTIONAL_DEVICE_EXTENSION) commonExtension;
        pdo = functionalExtension->LowerPdo;
    } else {
        pdo = DeviceObject;
    }

    status = IoRegisterDeviceInterface(
                pdo,
                &MOUNTDEV_MOUNTED_DEVICE_GUID,
                NULL,
                &interfaceName
                );

    if(NT_SUCCESS(status)) {

        //
        // Copy the interface name before setting the interface state - the
        // name is needed by the components we notify.
        //

        commonExtension->MountedDeviceInterfaceName = interfaceName;
        status = IoSetDeviceInterfaceState(&interfaceName, TRUE);

        if(!NT_SUCCESS(status)) {
            RtlFreeUnicodeString(&interfaceName);
        }
    }

    if(!NT_SUCCESS(status)) {
        RtlInitUnicodeString(&(commonExtension->MountedDeviceInterfaceName),
                             NULL);
    }
    return;
} // end ClasspRegisterMountedDeviceInterface()

/*++////////////////////////////////////////////////////////////////////////////

ClassSendDeviceIoControlSynchronous()

Routine Description:

    This routine is based upon IoBuildDeviceIoControlRequest().  It has been
    modified to reduce code and memory by not double-buffering the io, using
    the same buffer for both input and output, allocating and deallocating
    the mdl on behalf of the caller, and waiting for the io to complete.

    This routine also works around the rare cases in which APC's are disabled.
    Since IoBuildDeviceIoControl() used APC's to signal completion, this had
    led to a number of difficult-to-detect hangs, where the irp was completed,
    but the event passed to IoBuild..() was still being waited upon by the
    caller.

Arguments:

    IoControlCode - the IOCTL to send

    TargetDeviceObject - the device object that should handle the ioctl

    Buffer - the input and output buffer, or NULL if no input/output

    InputBufferLength - the number of bytes prepared for the IOCTL in Buffer

    OutputBufferLength - the number of bytes to be filled in upon success

    InternalDeviceIoControl - if TRUE, uses IRP_MJ_INTERNAL_DEVICE_CONTROL

    IoStatus - the status block that contains the results of the operation

Return Value:

--*/
VOID
NTAPI
ClassSendDeviceIoControlSynchronous(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    OUT PIO_STATUS_BLOCK IoStatus
    )
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    ULONG method;

    PAGED_CODE();

    irp = NULL;
    method = IoControlCode & 3;


    #if DBG // Begin Argument Checking (nop in fre version)

        ASSERT(ARGUMENT_PRESENT(IoStatus));

        if ((InputBufferLength != 0) || (OutputBufferLength != 0)) {
            ASSERT(ARGUMENT_PRESENT(Buffer));
        }
        else {
            ASSERT(!ARGUMENT_PRESENT(Buffer));
        }
    #endif

    //
    // Begin by allocating the IRP for this request.  Do not charge quota to
    // the current process for this IRP.
    //

    irp = IoAllocateIrp(TargetDeviceObject->StackSize, FALSE);
    if (!irp) {
        (*IoStatus).Information = 0;
        (*IoStatus).Status = STATUS_INSUFFICIENT_RESOURCES;
        return;
    }

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and the parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation(irp);

    //
    // Set the major function code based on the type of device I/O control
    // function the caller has specified.
    //

    if (InternalDeviceIoControl) {
        irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    } else {
        irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    }

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP for those parameters that are the same for all four methods.
    //

    irpSp->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    irpSp->Parameters.DeviceIoControl.IoControlCode = IoControlCode;

    //
    // Get the method bits from the I/O control code to determine how the
    // buffers are to be passed to the driver.
    //

    switch (method) {
        // case 0
        case METHOD_BUFFERED: {
            if ((InputBufferLength != 0) || (OutputBufferLength != 0)) {

                irp->AssociatedIrp.SystemBuffer =
                    ExAllocatePoolWithTag(NonPagedPoolCacheAligned,
                                          max(InputBufferLength, OutputBufferLength),
                                          CLASS_TAG_DEVICE_CONTROL
                                          );

                if (irp->AssociatedIrp.SystemBuffer == NULL) {
                    IoFreeIrp(irp);
                    (*IoStatus).Information = 0;
                    (*IoStatus).Status = STATUS_INSUFFICIENT_RESOURCES;
                    return;
                }

                if (InputBufferLength != 0) {
                    RtlCopyMemory(irp->AssociatedIrp.SystemBuffer,
                                  Buffer,
                                  InputBufferLength);
                }
            } // end of buffering

            irp->UserBuffer = Buffer;
            break;
        }

        // case 1, case 2
        case METHOD_IN_DIRECT:
        case METHOD_OUT_DIRECT: {


            if (InputBufferLength != 0) {
                irp->AssociatedIrp.SystemBuffer = Buffer;
            }

            if (OutputBufferLength != 0) {

                irp->MdlAddress = IoAllocateMdl(Buffer,
                                                OutputBufferLength,
                                                FALSE, FALSE,
                                                (PIRP) NULL);

                if (irp->MdlAddress == NULL) {
                    IoFreeIrp(irp);
                    (*IoStatus).Information = 0;
                    (*IoStatus).Status = STATUS_INSUFFICIENT_RESOURCES;
                    return;
                }

                if (method == METHOD_IN_DIRECT) {
                    MmProbeAndLockPages(irp->MdlAddress,
                                        KernelMode,
                                        IoReadAccess);
                } else if (method == METHOD_OUT_DIRECT) {
                    MmProbeAndLockPages(irp->MdlAddress,
                                        KernelMode,
                                        IoWriteAccess);
                } else {
                    ASSERT(!"If other methods reach here, code is out of date");
                }
            }
            break;
        }

        // case 3
        case METHOD_NEITHER: {

            ASSERT(!"This routine does not support METHOD_NEITHER ioctls");
            IoStatus->Information = 0;
            IoStatus->Status = STATUS_NOT_SUPPORTED;
            return;
            break;
        }
    } // end of switch(method)

    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // send the irp synchronously
    //

    ClassSendIrpSynchronous(TargetDeviceObject, irp);

    //
    // copy the iostatus block for the caller
    //

    *IoStatus = irp->IoStatus;

    //
    // free any allocated resources
    //

    switch (method) {
        case METHOD_BUFFERED: {

            ASSERT(irp->UserBuffer == Buffer);

            //
            // first copy the buffered result, if any
            // Note that there are no security implications in
            // not checking for success since only drivers can
            // call into this routine anyways...
            //

            if (OutputBufferLength != 0) {
                RtlCopyMemory(Buffer, // irp->UserBuffer
                              irp->AssociatedIrp.SystemBuffer,
                              OutputBufferLength
                              );
            }

            //
            // then free the memory allocated to buffer the io
            //

            if ((InputBufferLength !=0) || (OutputBufferLength != 0)) {
                ExFreePool(irp->AssociatedIrp.SystemBuffer);
                irp->AssociatedIrp.SystemBuffer = NULL;
            }
            break;
        }

        case METHOD_IN_DIRECT:
        case METHOD_OUT_DIRECT: {

            //
            // we alloc a mdl if there is an output buffer specified
            // free it here after unlocking the pages
            //

            if (OutputBufferLength != 0) {
                ASSERT(irp->MdlAddress != NULL);
                MmUnlockPages(irp->MdlAddress);
                IoFreeMdl(irp->MdlAddress);
                irp->MdlAddress = (PMDL) NULL;
            }
            break;
        }

        case METHOD_NEITHER: {
            ASSERT(!"Code is out of date");
            break;
        }
    }

    //
    // we always have allocated an irp.  free it here.
    //

    IoFreeIrp(irp);
    irp = (PIRP) NULL;

    //
    // return the io status block's status to the caller
    //

    return;
} // end ClassSendDeviceIoControlSynchronous()

/*++////////////////////////////////////////////////////////////////////////////

ClassForwardIrpSynchronous()

Routine Description:

    Forwards a given irp to the next lower device object.

Arguments:

    CommonExtension - the common class extension

    Irp - the request to forward down the stack

Return Value:

--*/
NTSTATUS
NTAPI
ClassForwardIrpSynchronous(
    IN PCOMMON_DEVICE_EXTENSION CommonExtension,
    IN PIRP Irp
    )
{
    IoCopyCurrentIrpStackLocationToNext(Irp);
    return ClassSendIrpSynchronous(CommonExtension->LowerDeviceObject, Irp);
} // end ClassForwardIrpSynchronous()

/*++////////////////////////////////////////////////////////////////////////////

ClassSendIrpSynchronous()

Routine Description:

    This routine sends the given irp to the given device object, and waits for
    it to complete.  On debug versions, will print out a debug message and
    optionally assert for "lost" irps based upon classpnp's globals

Arguments:

    TargetDeviceObject - the device object to handle this irp

    Irp - the request to be sent

Return Value:

--*/
NTSTATUS
NTAPI
ClassSendIrpSynchronous(
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PIRP Irp
    )
{
    KEVENT event;
    NTSTATUS status;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    ASSERT(TargetDeviceObject != NULL);
    ASSERT(Irp != NULL);
    ASSERT(Irp->StackCount >= TargetDeviceObject->StackSize);

    //
    // ISSUE-2000/02/20-henrygab   What if APCs are disabled?
    //    May need to enter critical section before IoCallDriver()
    //    until the event is hit?
    //

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);
    IoSetCompletionRoutine(Irp, ClassSignalCompletion, &event,
                           TRUE, TRUE, TRUE);

    status = IoCallDriver(TargetDeviceObject, Irp);

    if (status == STATUS_PENDING) {

        #if DBG
            LARGE_INTEGER timeout;

            timeout.QuadPart = (LONGLONG)(-1 * 10 * 1000 * (LONGLONG)1000 *
                                          ClasspnpGlobals.SecondsToWaitForIrps);

            do {
                status = KeWaitForSingleObject(&event,
                                               Executive,
                                               KernelMode,
                                               FALSE,
                                               &timeout);


                if (status == STATUS_TIMEOUT) {

                    //
                    // This DebugPrint should almost always be investigated by the
                    // party who sent the irp and/or the current owner of the irp.
                    // Synchronous Irps should not take this long (currently 30
                    // seconds) without good reason.  This points to a potentially
                    // serious problem in the underlying device stack.
                    //

                    DebugPrint((0, "ClassSendIrpSynchronous: (%p) irp %p did not "
                                "complete within %x seconds\n",
                                TargetDeviceObject, Irp,
                                ClasspnpGlobals.SecondsToWaitForIrps
                                ));

                    if (ClasspnpGlobals.BreakOnLostIrps != 0) {
                        ASSERT(!" - Irp failed to complete within 30 seconds - ");
                    }
                }


            } while (status==STATUS_TIMEOUT);
        #else
            KeWaitForSingleObject(&event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        #endif

        status = Irp->IoStatus.Status;
    }

    return status;
} // end ClassSendIrpSynchronous()

/*++////////////////////////////////////////////////////////////////////////////

ClassGetVpb()

Routine Description:

    This routine returns the current VPB (Volume Parameter Block) for the
    given device object.
    The Vpb field is only visible in the ntddk.h (not the wdm.h) definition
    of DEVICE_OBJECT; hence this exported function.

Arguments:

    DeviceObject - the device to get the VPB for

Return Value:

    the VPB, or NULL if none.

--*/
PVPB
NTAPI
ClassGetVpb(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    return DeviceObject->Vpb;
} // end ClassGetVpb()

/*++

    ISSUE-2000/02/20-henrygab Not documented ClasspAllocateReleaseRequest

--*/
NTSTATUS
NTAPI
ClasspAllocateReleaseRequest(
    IN PDEVICE_OBJECT Fdo
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    //PIO_STACK_LOCATION irpStack;

    KeInitializeSpinLock(&(fdoExtension->ReleaseQueueSpinLock));

    fdoExtension->ReleaseQueueNeeded = FALSE;
    fdoExtension->ReleaseQueueInProgress = FALSE;
    fdoExtension->ReleaseQueueIrpFromPool = FALSE;

    //
    // The class driver is responsible for allocating a properly sized irp,
    // or ClassReleaseQueue will attempt to do it on the first error.
    //

    fdoExtension->ReleaseQueueIrp = NULL;

    //
    // Write length to SRB.
    //

    fdoExtension->ReleaseQueueSrb.Length = sizeof(SCSI_REQUEST_BLOCK);

    return STATUS_SUCCESS;
} // end ClasspAllocateReleaseRequest()

/*++

    ISSUE-2000/02/20-henrygab Not documented ClasspFreeReleaseRequest

--*/
VOID
NTAPI
ClasspFreeReleaseRequest(
    IN PDEVICE_OBJECT Fdo
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    //KIRQL oldIrql;

    ASSERT(fdoExtension->CommonExtension.IsRemoved != NO_REMOVE);

    //
    // free anything the driver allocated
    //

    if (fdoExtension->ReleaseQueueIrp) {
        if (fdoExtension->ReleaseQueueIrpFromPool) {
            ExFreePool(fdoExtension->ReleaseQueueIrp);
        } else {
            IoFreeIrp(fdoExtension->ReleaseQueueIrp);
        }
        fdoExtension->ReleaseQueueIrp = NULL;
    }

    //
    // free anything that we allocated
    //

    if ((fdoExtension->PrivateFdoData) &&
        (fdoExtension->PrivateFdoData->ReleaseQueueIrpAllocated)) {

        ExFreePool(fdoExtension->PrivateFdoData->ReleaseQueueIrp);
        fdoExtension->PrivateFdoData->ReleaseQueueIrpAllocated = FALSE;
        fdoExtension->PrivateFdoData->ReleaseQueueIrp = NULL;
    }

    return;
} // end ClasspFreeReleaseRequest()

/*++////////////////////////////////////////////////////////////////////////////

ClassReleaseQueue()

Routine Description:

    This routine issues an internal device control command
    to the port driver to release a frozen queue. The call
    is issued asynchronously as ClassReleaseQueue will be invoked
    from the IO completion DPC (and will have no context to
    wait for a synchronous call to complete).

    This routine must be called with the remove lock held.

Arguments:

    Fdo - The functional device object for the device with the frozen queue.

Return Value:

    None.

--*/
VOID
NTAPI
ClassReleaseQueue(
    IN PDEVICE_OBJECT Fdo
    )
{
    ClasspReleaseQueue(Fdo, NULL);
    return;
} // end ClassReleaseQueue()

/*++////////////////////////////////////////////////////////////////////////////

ClasspAllocateReleaseQueueIrp()

Routine Description:

    This routine allocates the release queue irp held in classpnp's private
    extension.  This was added to allow no-memory conditions to be more
    survivable.

Return Value:

    NT_SUCCESS value.

Notes:

    Does not grab the spinlock.  Should only be called from StartDevice()
    routine.  May be called elsewhere for poorly-behaved drivers that cause
    the queue to lockup before the device is started.  This should *never*
    occur, since it's illegal to send a request to a non-started PDO.  This
    condition is checked for in ClasspReleaseQueue().

--*/
NTSTATUS
NTAPI
ClasspAllocateReleaseQueueIrp(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    //KIRQL oldIrql;
    UCHAR lowerStackSize;

    //
    // do an initial check w/o the spinlock
    //

    if (FdoExtension->PrivateFdoData->ReleaseQueueIrpAllocated) {
        return STATUS_SUCCESS;
    }


    lowerStackSize = FdoExtension->CommonExtension.LowerDeviceObject->StackSize;

    //
    // don't allocate one if one is in progress!  this means whoever called
    // this routine didn't check if one was in progress.
    //

    ASSERT(!(FdoExtension->ReleaseQueueInProgress));

    FdoExtension->PrivateFdoData->ReleaseQueueIrp =
        ExAllocatePoolWithTag(NonPagedPool,
                              IoSizeOfIrp(lowerStackSize),
                              CLASS_TAG_RELEASE_QUEUE
                              );

    if (FdoExtension->PrivateFdoData->ReleaseQueueIrp == NULL) {
        DebugPrint((0, "ClassPnpStartDevice: Cannot allocate for "
                    "release queue irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    IoInitializeIrp(FdoExtension->PrivateFdoData->ReleaseQueueIrp,
                    IoSizeOfIrp(lowerStackSize),
                    lowerStackSize);
    FdoExtension->PrivateFdoData->ReleaseQueueIrpAllocated = TRUE;

    return STATUS_SUCCESS;
}

/*++////////////////////////////////////////////////////////////////////////////

ClasspReleaseQueue()

Routine Description:

    This routine issues an internal device control command
    to the port driver to release a frozen queue. The call
    is issued asynchronously as ClassReleaseQueue will be invoked
    from the IO completion DPC (and will have no context to
    wait for a synchronous call to complete).

    This routine must be called with the remove lock held.

Arguments:

    Fdo - The functional device object for the device with the frozen queue.

    ReleaseQueueIrp - If this irp is supplied then the test to determine whether
                      a release queue request is in progress will be ignored.
                      The irp provided must be the IRP originally allocated
                      for release queue requests (so this parameter can only
                      really be provided by the release queue completion
                      routine.)

Return Value:

    None.

--*/
VOID
NTAPI
ClasspReleaseQueue(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP ReleaseQueueIrp OPTIONAL
    )
{
    PIO_STACK_LOCATION irpStack;
    PIRP irp;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PDEVICE_OBJECT lowerDevice;
    PSCSI_REQUEST_BLOCK srb;
    KIRQL currentIrql;

    lowerDevice = fdoExtension->CommonExtension.LowerDeviceObject;

    //
    // we raise irql separately so we're not swapped out or suspended
    // while holding the release queue irp in this routine.  this lets
    // us release the spin lock before lowering irql.
    //

    KeRaiseIrql(DISPATCH_LEVEL, &currentIrql);

    KeAcquireSpinLockAtDpcLevel(&(fdoExtension->ReleaseQueueSpinLock));

    //
    // make sure that if they passed us an irp, it matches our allocated irp.
    //

    ASSERT((ReleaseQueueIrp == NULL) ||
           (ReleaseQueueIrp == fdoExtension->PrivateFdoData->ReleaseQueueIrp));

    //
    // ASSERT that we've already allocated this. (should not occur)
    // try to allocate it anyways, then finally bugcheck if
    // there's still no memory...
    //

    ASSERT(fdoExtension->PrivateFdoData->ReleaseQueueIrpAllocated);
    if (!fdoExtension->PrivateFdoData->ReleaseQueueIrpAllocated) {
        ClasspAllocateReleaseQueueIrp(fdoExtension);
    }
    if (!fdoExtension->PrivateFdoData->ReleaseQueueIrpAllocated) {
        KeBugCheckEx(SCSI_DISK_DRIVER_INTERNAL, 0x12, (ULONG_PTR)Fdo, 0x0, 0x0);
    }

    if ((fdoExtension->ReleaseQueueInProgress) && (ReleaseQueueIrp == NULL)) {

        //
        // Someone is already using the irp - just set the flag to indicate that
        // we need to release the queue again.
        //

        fdoExtension->ReleaseQueueNeeded = TRUE;
        KeReleaseSpinLockFromDpcLevel(&(fdoExtension->ReleaseQueueSpinLock));
        KeLowerIrql(currentIrql);
        return;

    }

    //
    // Mark that there is a release queue in progress and drop the spinlock.
    //

    fdoExtension->ReleaseQueueInProgress = TRUE;
    if (ReleaseQueueIrp) {
        irp = ReleaseQueueIrp;
    } else {
        irp = fdoExtension->PrivateFdoData->ReleaseQueueIrp;
    }
    srb = &(fdoExtension->ReleaseQueueSrb);

    KeReleaseSpinLockFromDpcLevel(&(fdoExtension->ReleaseQueueSpinLock));

    ASSERT(irp != NULL);

    irpStack = IoGetNextIrpStackLocation(irp);

    irpStack->MajorFunction = IRP_MJ_SCSI;

    srb->OriginalRequest = irp;

    //
    // Store the SRB address in next stack for port driver.
    //

    irpStack->Parameters.Scsi.Srb = srb;

    //
    // If this device is removable then flush the queue.  This will also
    // release it.
    //

    if (TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA)){
       srb->Function = SRB_FUNCTION_FLUSH_QUEUE;
    }
    else {
       srb->Function = SRB_FUNCTION_RELEASE_QUEUE;
    }

    ClassAcquireRemoveLock(Fdo, irp);

    IoSetCompletionRoutine(irp,
                           ClassReleaseQueueCompletion,
                           Fdo,
                           TRUE,
                           TRUE,
                           TRUE);

    IoCallDriver(lowerDevice, irp);

    KeLowerIrql(currentIrql);

    return;

} // end ClassReleaseQueue()

/*++////////////////////////////////////////////////////////////////////////////

ClassReleaseQueueCompletion()

Routine Description:

    This routine is called when an asynchronous I/O request
    which was issued by the class driver completes.  Examples of such requests
    are release queue or START UNIT. This routine releases the queue if
    necessary.  It then frees the context and the IRP.

Arguments:

    DeviceObject - The device object for the logical unit; however since this
        is the top stack location the value is NULL.

    Irp - Supplies a pointer to the Irp to be processed.

    Context - Supplies the context to be used to process this request.

Return Value:

    None.

--*/
NTSTATUS
NTAPI
ClassReleaseQueueCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    KIRQL oldIrql;

    BOOLEAN releaseQueueNeeded;

    DeviceObject = Context;

    fdoExtension = DeviceObject->DeviceExtension;

    ClassReleaseRemoveLock(DeviceObject, Irp);

    //
    // Grab the spinlock and clear the release queue in progress flag so others
    // can run.  Save (and clear) the state of the release queue needed flag
    // so that we can issue a new release queue outside the spinlock.
    //

    KeAcquireSpinLock(&(fdoExtension->ReleaseQueueSpinLock), &oldIrql);

    releaseQueueNeeded = fdoExtension->ReleaseQueueNeeded;

    fdoExtension->ReleaseQueueNeeded = FALSE;
    fdoExtension->ReleaseQueueInProgress = FALSE;

    KeReleaseSpinLock(&(fdoExtension->ReleaseQueueSpinLock), oldIrql);

    //
    // If we need a release queue then issue one now.  Another processor may
    // have already started one in which case we'll try to issue this one after
    // it is done - but we should never recurse more than one deep.
    //

    if(releaseQueueNeeded) {
        ClasspReleaseQueue(DeviceObject, Irp);
    }

    //
    // Indicate the I/O system should stop processing the Irp completion.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

} // ClassAsynchronousCompletion()

/*++////////////////////////////////////////////////////////////////////////////

ClassAcquireChildLock()

Routine Description:

    This routine acquires the lock protecting children PDOs.  It may be
    acquired recursively by the same thread, but must be release by the
    thread once for each acquisition.

Arguments:

    FdoExtension - the device whose child list is protected.

Return Value:

    None

--*/
VOID
NTAPI
ClassAcquireChildLock(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PAGED_CODE();

    if(FdoExtension->ChildLockOwner != KeGetCurrentThread()) {
        KeWaitForSingleObject(&FdoExtension->ChildLock,
                              Executive, KernelMode,
                              FALSE, NULL);

        ASSERT(FdoExtension->ChildLockOwner == NULL);
        ASSERT(FdoExtension->ChildLockAcquisitionCount == 0);

        FdoExtension->ChildLockOwner = KeGetCurrentThread();
    } else {
        ASSERT(FdoExtension->ChildLockAcquisitionCount != 0);
    }

    FdoExtension->ChildLockAcquisitionCount++;
    return;
}

/*++////////////////////////////////////////////////////////////////////////////

ClassReleaseChildLock() ISSUE-2000/02/18-henrygab - not documented

Routine Description:

    This routine releases the lock protecting children PDOs.  It must be
    called once for each time ClassAcquireChildLock was called.

Arguments:

    FdoExtension - the device whose child list is protected

Return Value:

    None.

--*/
VOID
NTAPI
ClassReleaseChildLock(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    ASSERT(FdoExtension->ChildLockOwner == KeGetCurrentThread());
    ASSERT(FdoExtension->ChildLockAcquisitionCount != 0);

    FdoExtension->ChildLockAcquisitionCount -= 1;

    if(FdoExtension->ChildLockAcquisitionCount == 0) {
        FdoExtension->ChildLockOwner = NULL;
        KeSetEvent(&FdoExtension->ChildLock, IO_NO_INCREMENT, FALSE);
    }

    return;
} // end ClassReleaseChildLock(

/*++////////////////////////////////////////////////////////////////////////////

ClassAddChild()

Routine Description:

    This routine will insert a new child into the head of the child list.

Arguments:

    Parent - the child's parent (contains the head of the list)
    Child - the child to be inserted.
    AcquireLock - whether the child lock should be acquired (TRUE) or whether
                  it's already been acquired by or on behalf of the caller
                  (FALSE).

Return Value:

    None.

--*/
VOID
NTAPI
ClassAddChild(
    IN PFUNCTIONAL_DEVICE_EXTENSION Parent,
    IN PPHYSICAL_DEVICE_EXTENSION Child,
    IN BOOLEAN AcquireLock
    )
{
    if(AcquireLock) {
        ClassAcquireChildLock(Parent);
    }

    #if DBG
        //
        // Make sure this child's not already in the list.
        //
        {
            PPHYSICAL_DEVICE_EXTENSION testChild;

            for (testChild = Parent->CommonExtension.ChildList;
                 testChild != NULL;
                 testChild = testChild->CommonExtension.ChildList) {

                ASSERT(testChild != Child);
            }
        }
    #endif

    Child->CommonExtension.ChildList = Parent->CommonExtension.ChildList;
    Parent->CommonExtension.ChildList = Child;

    if(AcquireLock) {
        ClassReleaseChildLock(Parent);
    }
    return;
} // end ClassAddChild()

/*++////////////////////////////////////////////////////////////////////////////

ClassRemoveChild()

Routine Description:

    This routine will remove a child from the child list.

Arguments:

    Parent - the parent to be removed from.

    Child - the child to be removed or NULL if the first child should be
            removed.

    AcquireLock - whether the child lock should be acquired (TRUE) or whether
                  it's already been acquired by or on behalf of the caller
                  (FALSE).

Return Value:

    A pointer to the child which was removed or NULL if no such child could
    be found in the list (or if Child was NULL but the list is empty).

--*/
PPHYSICAL_DEVICE_EXTENSION
NTAPI
ClassRemoveChild(
    IN PFUNCTIONAL_DEVICE_EXTENSION Parent,
    IN PPHYSICAL_DEVICE_EXTENSION Child,
    IN BOOLEAN AcquireLock
    )
{
    if(AcquireLock) {
        ClassAcquireChildLock(Parent);
    }

    TRY {
        PCOMMON_DEVICE_EXTENSION previousChild = &Parent->CommonExtension;

        //
        // If the list is empty then bail out now.
        //

        if(Parent->CommonExtension.ChildList == NULL) {
            Child = NULL;
            LEAVE;
        }

        //
        // If the caller specified a child then find the child object before
        // it.  If none was specified then the FDO is the child object before
        // the one we want to remove.
        //

        if(Child != NULL) {

            //
            // Scan through the child list to find the entry which points to
            // this one.
            //

            do {
                ASSERT(previousChild != &Child->CommonExtension);

                if(previousChild->ChildList == Child) {
                    break;
                }

                previousChild = &previousChild->ChildList->CommonExtension;
            } while(previousChild != NULL);

            if(previousChild == NULL) {
                Child = NULL;
                LEAVE;
            }
        }

        //
        // Save the next child away then unlink it from the list.
        //

        Child = previousChild->ChildList;
        previousChild->ChildList = Child->CommonExtension.ChildList;
        Child->CommonExtension.ChildList = NULL;

    } FINALLY {
        if(AcquireLock) {
            ClassReleaseChildLock(Parent);
        }
    }
    return Child;
} // end ClassRemoveChild()

/*++

    ISSUE-2000/02/20-henrygab Not documented ClasspRetryRequestDpc

--*/
VOID
NTAPI
ClasspRetryRequestDpc(
    IN PKDPC Dpc,
    IN PVOID Context,
    IN PVOID Arg1,
    IN PVOID Arg2
    )
{
    PDEVICE_OBJECT deviceObject = Context;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData;
    PCLASS_RETRY_INFO retryList;
    KIRQL irql;


    commonExtension = deviceObject->DeviceExtension;
    ASSERT(commonExtension->IsFdo);
    fdoExtension = deviceObject->DeviceExtension;
    fdoData = fdoExtension->PrivateFdoData;


    KeAcquireSpinLock(&fdoData->Retry.Lock, &irql);
    {
        LARGE_INTEGER now;
        KeQueryTickCount(&now);

        //
        // if CurrentTick is less than now
        //      fire another DPC
        // else
        //      retry entire list
        // endif
        //

        if (now.QuadPart < fdoData->Retry.Tick.QuadPart) {

            ClasspRetryDpcTimer(fdoData);
            retryList = NULL;

        } else {

            retryList = fdoData->Retry.ListHead;
            fdoData->Retry.ListHead = NULL;
            fdoData->Retry.Delta.QuadPart = (LONGLONG)0;
            fdoData->Retry.Tick.QuadPart  = (LONGLONG)0;

        }
    }
    KeReleaseSpinLock(&fdoData->Retry.Lock, irql);

    while (retryList != NULL) {

        PIRP irp;

        irp = CONTAINING_RECORD(retryList, IRP, Tail.Overlay.DriverContext[0]);
        DebugPrint((ClassDebugDelayedRetry, "ClassRetry:  -- %p\n", irp));
        retryList = retryList->Next;
        #if DBG
            irp->Tail.Overlay.DriverContext[0] = ULongToPtr(0xdddddddd); // invalidate data
            irp->Tail.Overlay.DriverContext[1] = ULongToPtr(0xdddddddd); // invalidate data
            irp->Tail.Overlay.DriverContext[2] = ULongToPtr(0xdddddddd); // invalidate data
            irp->Tail.Overlay.DriverContext[3] = ULongToPtr(0xdddddddd); // invalidate data
        #endif

        IoCallDriver(commonExtension->LowerDeviceObject, irp);

    }
    return;

} // end ClasspRetryRequestDpc()

/*++

    ISSUE-2000/02/20-henrygab Not documented ClassRetryRequest

--*/
VOID
NTAPI
ClassRetryRequest(
    IN PDEVICE_OBJECT SelfDeviceObject,
    IN PIRP           Irp,
    IN LARGE_INTEGER  TimeDelta100ns // in 100ns units
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData;
    PCLASS_RETRY_INFO  retryInfo;
    //PCLASS_RETRY_INFO *previousNext;
    LARGE_INTEGER      delta;
    KIRQL irql;

    //
    // this checks we aren't destroying irps
    //
    ASSERT(sizeof(CLASS_RETRY_INFO) <= (4*sizeof(PVOID)));

    fdoExtension = SelfDeviceObject->DeviceExtension;
    fdoData = fdoExtension->PrivateFdoData;

    if (!fdoExtension->CommonExtension.IsFdo) {

        //
        // this debug print/assertion should ALWAYS be investigated.
        // ClassRetryRequest can currently only be used by FDO's
        //

        DebugPrint((ClassDebugError, "ClassRetryRequestEx: LOST IRP %p\n", Irp));
        ASSERT(!"ClassRetryRequestEx Called From PDO? LOST IRP");
        return;

    }

    if (TimeDelta100ns.QuadPart < 0) {
        ASSERT(!"ClassRetryRequest - must use positive delay");
        TimeDelta100ns.QuadPart *= -1;
    }

    //
    // prepare what we can out of the loop
    //

    retryInfo = (PCLASS_RETRY_INFO)(&Irp->Tail.Overlay.DriverContext[0]);
    RtlZeroMemory(retryInfo, sizeof(CLASS_RETRY_INFO));

    delta.QuadPart = (TimeDelta100ns.QuadPart / fdoData->Retry.Granularity);
    if (TimeDelta100ns.QuadPart % fdoData->Retry.Granularity) {
        delta.QuadPart ++; // round up to next tick
    }
    if (delta.QuadPart == (LONGLONG)0) {
        delta.QuadPart = MINIMUM_RETRY_UNITS;
    }

    //
    // now determine if we should fire another DPC or not
    //

    KeAcquireSpinLock(&fdoData->Retry.Lock, &irql);

    //
    // always add request to the list
    //

    retryInfo->Next = fdoData->Retry.ListHead;
    fdoData->Retry.ListHead = retryInfo;

    if (fdoData->Retry.Delta.QuadPart == (LONGLONG)0) {

        DebugPrint((ClassDebugDelayedRetry, "ClassRetry: +++ %p\n", Irp));

        //
        // must be exactly one item on list
        //

        ASSERT(fdoData->Retry.ListHead       != NULL);
        ASSERT(fdoData->Retry.ListHead->Next == NULL);

        //
        // if currentDelta is zero, always fire a DPC
        //

        KeQueryTickCount(&fdoData->Retry.Tick);
        fdoData->Retry.Tick.QuadPart  += delta.QuadPart;
        fdoData->Retry.Delta.QuadPart  = delta.QuadPart;
        ClasspRetryDpcTimer(fdoData);

    } else if (delta.QuadPart > fdoData->Retry.Delta.QuadPart) {

        //
        // if delta is greater than the list's current delta,
        // increase the DPC handling time by difference
        // and update the delta to new larger value
        // allow the DPC to re-fire itself if needed
        //

        DebugPrint((ClassDebugDelayedRetry, "ClassRetry:  ++ %p\n", Irp));

        //
        // must be at least two items on list
        //

        ASSERT(fdoData->Retry.ListHead       != NULL);
        ASSERT(fdoData->Retry.ListHead->Next != NULL);

        fdoData->Retry.Tick.QuadPart  -= fdoData->Retry.Delta.QuadPart;
        fdoData->Retry.Tick.QuadPart  += delta.QuadPart;

        fdoData->Retry.Delta.QuadPart  = delta.QuadPart;

    } else {

        //
        // just inserting it on the list was enough
        //

        DebugPrint((ClassDebugDelayedRetry, "ClassRetry:  ++ %p\n", Irp));

    }


    KeReleaseSpinLock(&fdoData->Retry.Lock, irql);


} // end ClassRetryRequest()

/*++

    ISSUE-2000/02/20-henrygab Not documented ClasspRetryDpcTimer

--*/
VOID
NTAPI
ClasspRetryDpcTimer(
    IN PCLASS_PRIVATE_FDO_DATA FdoData
    )
{
    LARGE_INTEGER fire;

    ASSERT(FdoData->Retry.Tick.QuadPart != (LONGLONG)0);
    ASSERT(FdoData->Retry.ListHead      != NULL);  // never fire an empty list

    //
    // fire == (CurrentTick - now) * (100ns per tick)
    //
    // NOTE: Overflow is nearly impossible and is ignored here
    //

    KeQueryTickCount(&fire);
    fire.QuadPart =  FdoData->Retry.Tick.QuadPart - fire.QuadPart;
    fire.QuadPart *= FdoData->Retry.Granularity;

    //
    // fire is now multiples of 100ns until should fire the timer.
    // if timer should already have expired, or would fire too quickly,
    // fire it in some arbitrary number of ticks to prevent infinitely
    // recursing.
    //

    if (fire.QuadPart < MINIMUM_RETRY_UNITS) {
        fire.QuadPart = MINIMUM_RETRY_UNITS;
    }

    DebugPrint((ClassDebugDelayedRetry,
                "ClassRetry: ======= %I64x ticks\n",
                fire.QuadPart));

    //
    // must use negative to specify relative time to fire
    //

    fire.QuadPart = fire.QuadPart * ((LONGLONG)-1);

    //
    // set the timer, since this is the first addition
    //

    KeSetTimerEx(&FdoData->Retry.Timer, fire, 0, &FdoData->Retry.Dpc);

    return;
} // end ClasspRetryDpcTimer()

NTSTATUS
NTAPI
ClasspInitializeHotplugInfo(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PCLASS_PRIVATE_FDO_DATA fdoData = FdoExtension->PrivateFdoData;
    DEVICE_REMOVAL_POLICY deviceRemovalPolicy;
    NTSTATUS status;
    ULONG resultLength = 0;
    ULONG writeCacheOverride;

    PAGED_CODE();

    //
    // start with some default settings
    //
    RtlZeroMemory(&(fdoData->HotplugInfo), sizeof(STORAGE_HOTPLUG_INFO));

    //
    // set the size (aka version)
    //

    fdoData->HotplugInfo.Size = sizeof(STORAGE_HOTPLUG_INFO);

    //
    // set if the device has removable media
    //

    if (FdoExtension->DeviceDescriptor->RemovableMedia) {
        fdoData->HotplugInfo.MediaRemovable = TRUE;
    } else {
        fdoData->HotplugInfo.MediaRemovable = FALSE;
    }

    //
    // this refers to devices which, for reasons not yet understood,
    // do not fail PREVENT_MEDIA_REMOVAL requests even though they
    // have no way to lock the media into the drive.  this allows
    // the filesystems to turn off delayed-write caching for these
    // devices as well.
    //

    if (TEST_FLAG(FdoExtension->PrivateFdoData->HackFlags,
                  FDO_HACK_CANNOT_LOCK_MEDIA)) {
        fdoData->HotplugInfo.MediaHotplug = TRUE;
    } else {
        fdoData->HotplugInfo.MediaHotplug = FALSE;
    }


    //
    // Look into the registry to  see if the user has  chosen
    // to override the default setting for the removal policy
    //

    RtlZeroMemory(&deviceRemovalPolicy, sizeof(DEVICE_REMOVAL_POLICY));

    ClassGetDeviceParameter(FdoExtension,
                            CLASSP_REG_SUBKEY_NAME,
                            CLASSP_REG_REMOVAL_POLICY_VALUE_NAME,
                            (PULONG)&deviceRemovalPolicy);

    if (deviceRemovalPolicy == 0)
    {
        //
        // Query the default removal policy from the kernel
        //

        status = IoGetDeviceProperty(FdoExtension->LowerPdo,
                                     DevicePropertyRemovalPolicy,
                                     sizeof(DEVICE_REMOVAL_POLICY),
                                     (PVOID)&deviceRemovalPolicy,
                                     &resultLength);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        if (resultLength != sizeof(DEVICE_REMOVAL_POLICY))
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    // use this info to set the DeviceHotplug setting
    // don't rely on DeviceCapabilities, since it can't properly
    // determine device relations, etc.  let the kernel figure this
    // stuff out instead.
    //

    if (deviceRemovalPolicy == RemovalPolicyExpectSurpriseRemoval) {
        fdoData->HotplugInfo.DeviceHotplug = TRUE;
    } else {
        fdoData->HotplugInfo.DeviceHotplug = FALSE;
    }

    //
    // this refers to the *filesystem* caching, but has to be included
    // here since it's a per-device setting.  this may change to be
    // stored by the system in the future.
    //

    writeCacheOverride = FALSE;
    ClassGetDeviceParameter(FdoExtension,
                            CLASSP_REG_SUBKEY_NAME,
                            CLASSP_REG_WRITE_CACHE_VALUE_NAME,
                            &writeCacheOverride);

    if (writeCacheOverride) {
        fdoData->HotplugInfo.WriteCacheEnableOverride = TRUE;
    } else {
        fdoData->HotplugInfo.WriteCacheEnableOverride = FALSE;
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
ClasspScanForClassHacks(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN ULONG_PTR Data
    )
{
    PAGED_CODE();

    //
    // remove invalid flags and save
    //

    CLEAR_FLAG(Data, FDO_HACK_INVALID_FLAGS);
    SET_FLAG(FdoExtension->PrivateFdoData->HackFlags, Data);
    return;
}

VOID
NTAPI
ClasspScanForSpecialInRegistry(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    HANDLE             deviceParameterHandle; // device instance key
    HANDLE             classParameterHandle; // classpnp subkey
    OBJECT_ATTRIBUTES  objectAttributes;
    UNICODE_STRING     subkeyName;
    NTSTATUS           status;

    //
    // seeded in the ENUM tree by ClassInstaller
    //
    ULONG deviceHacks;
    RTL_QUERY_REGISTRY_TABLE queryTable[2]; // null terminated array

    PAGED_CODE();

    deviceParameterHandle = NULL;
    classParameterHandle = NULL;
    deviceHacks = 0;

    status = IoOpenDeviceRegistryKey(FdoExtension->LowerPdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_WRITE,
                                     &deviceParameterHandle
                                     );

    if (!NT_SUCCESS(status)) {
        goto cleanupScanForSpecial;
    }

    RtlInitUnicodeString(&subkeyName, CLASSP_REG_SUBKEY_NAME);
    InitializeObjectAttributes(&objectAttributes,
                               &subkeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               deviceParameterHandle,
                               NULL
                               );

    status = ZwOpenKey( &classParameterHandle,
                        KEY_READ,
                        &objectAttributes
                        );

    if (!NT_SUCCESS(status)) {
        goto cleanupScanForSpecial;
    }

    //
    // Zero out the memory
    //

    RtlZeroMemory(&queryTable[0], 2*sizeof(RTL_QUERY_REGISTRY_TABLE));

    //
    // Setup the structure to read
    //

    queryTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[0].Name          = CLASSP_REG_HACK_VALUE_NAME;
    queryTable[0].EntryContext  = &deviceHacks;
    queryTable[0].DefaultType   = REG_DWORD;
    queryTable[0].DefaultData   = &deviceHacks;
    queryTable[0].DefaultLength = 0;

    //
    // read values
    //

    status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    (PWSTR)classParameterHandle,
                                    &queryTable[0],
                                    NULL,
                                    NULL
                                    );
    if (!NT_SUCCESS(status)) {
        goto cleanupScanForSpecial;
    }

    //
    // remove unknown values and save...
    //

    KdPrintEx((DPFLTR_CLASSPNP_ID, DPFLTR_ERROR_LEVEL,
               "Classpnp => ScanForSpecial: HackFlags %#08x\n",
               deviceHacks));

    CLEAR_FLAG(deviceHacks, FDO_HACK_INVALID_FLAGS);
    SET_FLAG(FdoExtension->PrivateFdoData->HackFlags, deviceHacks);


cleanupScanForSpecial:

    if (deviceParameterHandle) {
        ZwClose(deviceParameterHandle);
    }

    if (classParameterHandle) {
        ZwClose(classParameterHandle);
    }

    //
    // we should modify the system hive to include another key for us to grab
    // settings from.  in this case:  Classpnp\HackFlags
    //
    // the use of a DWORD value for the HackFlags allows 32 hacks w/o
    // significant use of the registry, and also reduces OEM exposure.
    //
    // definition of bit flags:
    //   0x00000001 -- Device succeeds PREVENT_MEDIUM_REMOVAL, but
    //                 cannot actually prevent removal.
    //   0x00000002 -- Device hard-hangs or times out for GESN requests.
    //   0xfffffffc -- Currently reserved, may be used later.
    //

    return;
}
