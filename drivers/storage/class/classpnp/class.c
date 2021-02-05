/*++

Copyright (C) Microsoft Corporation, 1991 - 2010

Module Name:

    class.c

Abstract:

    SCSI class driver routines

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#define CLASS_INIT_GUID 1
#define DEBUG_MAIN_SOURCE 1

#include "classp.h"
#include "debug.h"
#include <process.h>
#include <devpkey.h>
#include <ntiologc.h>


#ifdef DEBUG_USE_WPP
#include "class.tmh"
#endif

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
    #pragma alloc_text(PAGE, ClasspModeSense)
    #pragma alloc_text(PAGE, ClasspIsPortable)
    #pragma alloc_text(PAGE, ClassAcquireChildLock)
    #pragma alloc_text(PAGE, ClassDetermineTokenOperationCommandSupport)
    #pragma alloc_text(PAGE, ClassDeviceProcessOffloadRead)
    #pragma alloc_text(PAGE, ClassDeviceProcessOffloadWrite)
    #pragma alloc_text(PAGE, ClasspServicePopulateTokenTransferRequest)
    #pragma alloc_text(PAGE, ClasspServiceWriteUsingTokenTransferRequest)
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
    #pragma alloc_text(PAGE, ClassModeSenseEx)
#endif
#endif

#ifdef _MSC_VER
#pragma prefast(disable:28159, "There are certain cases when we have to bugcheck...")
#endif

IO_COMPLETION_ROUTINE ClassCheckVerifyComplete;


ULONG ClassPnpAllowUnload = TRUE;
ULONG ClassMaxInterleavePerCriticalIo = CLASS_MAX_INTERLEAVE_PER_CRITICAL_IO;
CONST LARGE_INTEGER Magic10000 = {{0xe219652c, 0xd1b71758}};
GUID StoragePredictFailureDPSGuid = WDI_STORAGE_PREDICT_FAILURE_DPS_GUID;

#define FirstDriveLetter 'C'
#define LastDriveLetter  'Z'

BOOLEAN UseQPCTime = FALSE;

//
// Keep track of whether security cookie is initialized or not. This is
// required by SDL.
//

BOOLEAN InitSecurityCookie = FALSE;

//
// List Identifier for offload data transfer operations
//
ULONG MaxTokenOperationListIdentifier = MAX_TOKEN_LIST_IDENTIFIERS;
volatile ULONG TokenOperationListIdentifier = (ULONG)-1;

//
// List of FDOs that have enabled idle power management.
//
LIST_ENTRY IdlePowerFDOList = {0};
KGUARDED_MUTEX IdlePowerFDOListMutex;

//
// Handle used to register for power setting notifications.
//
PVOID PowerSettingNotificationHandle;

//
// Handle used to register for screen state setting notifications.
//
PVOID ScreenStateNotificationHandle;

//
// Disk idle timeout in milliseconds.
// We default this to 0xFFFFFFFF as this is what the power manager considers
// "never" and ensures we do not set a disk idle timeout until the power
// manager calls us back with a different value.
//
ULONG DiskIdleTimeoutInMS = 0xFFFFFFFF;


NTSTATUS DllUnload(VOID)
{
    DbgPrintEx(DPFLTR_CLASSPNP_ID, DPFLTR_INFO_LEVEL, "classpnp.sys is now unloading\n");

    if (PowerSettingNotificationHandle) {
        PoUnregisterPowerSettingCallback(PowerSettingNotificationHandle);
        PowerSettingNotificationHandle = NULL;
    }

    if (ScreenStateNotificationHandle) {
        PoUnregisterPowerSettingCallback(ScreenStateNotificationHandle);
        ScreenStateNotificationHandle = NULL;
    }


    return STATUS_SUCCESS;
}


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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

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
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
ULONG
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassInitialize(
    _In_  PVOID            Argument1,
    _In_  PVOID            Argument2,
    _In_  PCLASS_INIT_DATA InitializationData
    )
{
    PDRIVER_OBJECT  DriverObject = Argument1;
    PUNICODE_STRING RegistryPath = Argument2;

    PCLASS_DRIVER_EXTENSION driverExtension;

    NTSTATUS        status;



    PAGED_CODE();

    //
    // Initialize the security cookie if needed.
    //
#ifndef __REACTOS__
    if (InitSecurityCookie == FALSE) {
        __security_init_cookie();
        InitSecurityCookie = TRUE;
    }
#endif


    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT, "\n\nSCSI Class Driver\n"));

    ClasspInitializeDebugGlobals();

    //
    // Validate the length of this structure. This is effectively a
    // version check.
    //

    if (InitializationData->InitializationDataSize != sizeof(CLASS_INIT_DATA)) {

        //
        // This DebugPrint is to help third-party driver writers
        //

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT, "ClassInitialize: Class driver wrong version\n"));
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

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,
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

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,  "ClassInitialize: Class device-specific missing "
                       "required PDO entry\n"));

        return (ULONG) STATUS_REVISION_MISMATCH;
    }

    if((InitializationData->FdoData.ClassStopDevice == NULL) ||
        ((InitializationData->ClassEnumerateDevice != NULL) &&
         (InitializationData->PdoData.ClassStopDevice == NULL))) {

        //
        // This DebugPrint is to help third-party driver writers
        //

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,  "ClassInitialize: Class device-specific missing "
                       "required PDO entry\n"));
        NT_ASSERT(FALSE);
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

        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,  "ClassInitialize: driver does not support unload %wZ\n",
                    RegistryPath));
    }

    //
    // Create an extension for the driver object
    //
#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer as data pointer for this use case
#endif
    status = IoAllocateDriverObjectExtension(DriverObject, CLASS_DRIVER_EXTENSION_KEY, sizeof(CLASS_DRIVER_EXTENSION), (PVOID *)&driverExtension);

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

        ClassInitializeDispatchTables(driverExtension);

    } else if (status == STATUS_OBJECT_NAME_COLLISION) {

        //
        // The extension already exists - get a pointer to it
        //
#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer as data pointer for this use case
#endif
        driverExtension = IoGetDriverObjectExtension(DriverObject, CLASS_DRIVER_EXTENSION_KEY);

        NT_ASSERT(driverExtension != NULL);

    } else {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,  "ClassInitialize: Class driver extension could not be "
                       "allocated %lx\n", status));
        return status;
    }


    //
    // Update driver object with entry points.
    //

#ifdef _MSC_VER
#pragma prefast(push)
#pragma prefast(disable:28175, "Accessing DRIVER_OBJECT fileds is OK here since this function " \
                               "is supposed to be invoked from DriverEntry only")
#endif
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_READ]            = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_WRITE]           = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_SCSI]            = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]        = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]   = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP]             = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER]           = ClassGlobalDispatch;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = ClassGlobalDispatch;

    if (InitializationData->ClassStartIo) {
        DriverObject->DriverStartIo = ClasspStartIo;
    }

    if ((InitializationData->ClassUnload) && (ClassPnpAllowUnload == TRUE)) {
        DriverObject->DriverUnload = ClassUnload;
    } else {
        DriverObject->DriverUnload = NULL;
    }

    DriverObject->DriverExtension->AddDevice = ClassAddDevice;
#ifdef _MSC_VER
#pragma prefast(pop)
#endif


    //
    // Register for event tracing
    //
    if (driverExtension->EtwHandle == 0) {
        status = EtwRegister(&StoragePredictFailureDPSGuid,
                             NULL,
                             NULL,
                             &driverExtension->EtwHandle);
        if (!NT_SUCCESS(status)) {
            driverExtension->EtwHandle = 0;
        }
        WPP_INIT_TRACING(DriverObject, RegistryPath);
    }


    //
    // Ensure these are only initialized once.
    //
    if (IdlePowerFDOList.Flink == NULL) {
        InitializeListHead(&IdlePowerFDOList);
        KeInitializeGuardedMutex(&IdlePowerFDOListMutex);
    }

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

    GUID_CLASSPNP_QUERY_REGINFOEX == CLASS_QUERY_WMI_REGINFO_EX_LIST

        Initialized classpnp to callback a PCLASS_QUERY_WMI_REGINFO_EX
        callback instead of a PCLASS_QUERY_WMI_REGINFO callback. The
        former callback allows the driver to specify the name of the
        mof resource.

    GUID_CLASSPNP_SENSEINFO2      == CLASS_INTERPRET_SENSE_INFO2

        Initialize classpnp to callback into class drive for interpretation
        of all sense info, and to indicate the count of "history" to keep
        for each packet.

    GUID_CLASSPNP_WORKING_SET     == CLASS_WORKING_SET

        Allow class driver to override the min and max working set transfer
        packet value used in classpnp.

    GUID_CLASSPNP_SRB_SUPPORT     == ULONG

        Allow class driver to provide supported SRB types.

Arguments:

    DriverObject
    Guid
    Data

Return Value:

    Status Code

--*/
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
ULONG
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassInitializeEx(
    _In_  PDRIVER_OBJECT   DriverObject,
    _In_  LPGUID           Guid,
    _In_  PVOID            Data
    )
{
    PCLASS_DRIVER_EXTENSION driverExtension;

    NTSTATUS        status;

    PAGED_CODE();

#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer as data pointer for this use case
#endif
    driverExtension = IoGetDriverObjectExtension( DriverObject, CLASS_DRIVER_EXTENSION_KEY );

    if (driverExtension == NULL)
    {
        NT_ASSERT(FALSE);
        return (ULONG)STATUS_UNSUCCESSFUL;
    }

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
    }
    else if (IsEqualGUID(Guid, &ClassGuidWorkingSet))
    {
        PCLASS_WORKING_SET infoOriginal = (PCLASS_WORKING_SET)Data;
        PCLASS_WORKING_SET info = NULL;

        // only try to allocate memory for cached copy if size is correct
        if (infoOriginal->Size != sizeof(CLASS_WORKING_SET))
        {
            // incorrect size -- client programming error
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            info = ExAllocatePoolWithTag(NonPagedPoolNx,
                                         sizeof(CLASS_WORKING_SET),
                                         CLASS_TAG_WORKING_SET
                                         );
            if (info == NULL)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                // cache the structure internally
                RtlCopyMemory(info, infoOriginal, sizeof(CLASS_WORKING_SET));
                status = STATUS_SUCCESS;
            }
        }
        // if we successfully cached a copy, validate all the data within
        if (NT_SUCCESS(status))
        {
            if (info->Size != sizeof(CLASS_WORKING_SET))
            {
                // incorrect size -- client programming error
                status = STATUS_INVALID_PARAMETER;
            }
            else if (info->XferPacketsWorkingSetMaximum > CLASS_WORKING_SET_MAXIMUM)
            {
                // too many requested in the working set
                status = STATUS_INVALID_PARAMETER;
            }
            else if (info->XferPacketsWorkingSetMinimum > CLASS_WORKING_SET_MAXIMUM)
            {
                // too many requested in the working set
                status = STATUS_INVALID_PARAMETER;
            }
            else if (driverExtension->InitData.FdoData.DeviceType != FILE_DEVICE_CD_ROM)
            {
                // classpnp developer wants to restrict this code path
                // for now to CDROM devices only.
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            else if (driverExtension->WorkingSet != NULL)
            {
                // not allowed to change it once it is set for a driver
                status = STATUS_INVALID_PARAMETER;
                NT_ASSERT(FALSE);
            }
        }
        // save results or cleanup
        if (NT_SUCCESS(status))
        {
            driverExtension->WorkingSet = info; info = NULL;
        }
        else
        {
            FREE_POOL( info );
        }
    }
    else if (IsEqualGUID(Guid, &ClassGuidSenseInfo2))
    {
        PCLASS_INTERPRET_SENSE_INFO2 infoOriginal = (PCLASS_INTERPRET_SENSE_INFO2)Data;
        PCLASS_INTERPRET_SENSE_INFO2 info = NULL;

        // only try to allocate memory for cached copy if size is correct
        if (infoOriginal->Size != sizeof(CLASS_INTERPRET_SENSE_INFO2))
        {
            // incorrect size -- client programming error
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            info = ExAllocatePoolWithTag(NonPagedPoolNx,
                                         sizeof(CLASS_INTERPRET_SENSE_INFO2),
                                         CLASS_TAG_SENSE2
                                         );
            if (info == NULL)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                // cache the structure internally
                RtlCopyMemory(info, infoOriginal, sizeof(CLASS_INTERPRET_SENSE_INFO2));
                status = STATUS_SUCCESS;
            }
        }

        // if we successfully cached a copy, validate all the data within
        if (NT_SUCCESS(status))
        {
            if (info->Size != sizeof(CLASS_INTERPRET_SENSE_INFO2))
            {
                // incorrect size -- client programming error
                status = STATUS_INVALID_PARAMETER;
            }
            else if (info->HistoryCount > CLASS_INTERPRET_SENSE_INFO2_MAXIMUM_HISTORY_COUNT)
            {
                // incorrect count -- client programming error
                status = STATUS_INVALID_PARAMETER;
            }
            else if (info->Compress == NULL)
            {
                // Compression of the history is required to be supported
                status = STATUS_INVALID_PARAMETER;
            }
            else if (info->HistoryCount == 0)
            {
                // History count cannot be zero
                status = STATUS_INVALID_PARAMETER;
            }
            else if (info->Interpret == NULL)
            {
                // Updated interpret sense info function is required
                status = STATUS_INVALID_PARAMETER;
            }
            else if (driverExtension->InitData.FdoData.DeviceType != FILE_DEVICE_CD_ROM)
            {
                // classpnp developer wants to restrict this code path
                // for now to CDROM devices only.
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            else if (driverExtension->InterpretSenseInfo != NULL)
            {
                // not allowed to change it once it is set for a driver
                status = STATUS_INVALID_PARAMETER;
                NT_ASSERT(FALSE);
            }
        }

        // save results or cleanup
        if (NT_SUCCESS(status))
        {
            driverExtension->InterpretSenseInfo = info; info = NULL;
        }
        else
        {
            FREE_POOL( info );
        }
    }
    else if (IsEqualGUID(Guid, &ClassGuidSrbSupport))
    {
        ULONG srbSupport = *((PULONG)Data);

        //
        // Validate that at least one of the supported bit flags is set. Assume
        // all class drivers support SCSI_REQUEST_BLOCK as a class driver that
        // supports only extended SRB is not feasible.
        //
        if ((srbSupport &
             (CLASS_SRB_SCSI_REQUEST_BLOCK | CLASS_SRB_STORAGE_REQUEST_BLOCK)) != 0) {
            driverExtension->SrbSupport = srbSupport;
            status = STATUS_SUCCESS;

            //
            // Catch cases of a class driver reporting only extended SRB support
            //
            if ((driverExtension->SrbSupport & CLASS_SRB_SCSI_REQUEST_BLOCK) == 0) {
                NT_ASSERT(FALSE);
            }
        } else {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        status = STATUS_NOT_SUPPORTED;
    }

    return status;

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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PCLASS_DRIVER_EXTENSION driverExtension;

    PAGED_CODE();

    NT_ASSERT( DriverObject->DeviceObject == NULL );

#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer as data pointer for this use case
#endif
    driverExtension = IoGetDriverObjectExtension( DriverObject, CLASS_DRIVER_EXTENSION_KEY );


    if (driverExtension == NULL)
    {
        NT_ASSERT(FALSE);
        return;
    }

    NT_ASSERT(driverExtension->RegistryPath.Buffer != NULL);
    NT_ASSERT(driverExtension->InitData.ClassUnload != NULL);

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,  "ClassUnload: driver unloading %wZ\n",
                &driverExtension->RegistryPath));

    //
    // attempt to process the driver's unload routine first.
    //

    driverExtension->InitData.ClassUnload(DriverObject);

    //
    // free own allocated resources and return
    //

    FREE_POOL( driverExtension->WorkingSet          );
    FREE_POOL( driverExtension->InterpretSenseInfo  );
    FREE_POOL( driverExtension->RegistryPath.Buffer );
    driverExtension->RegistryPath.Length = 0;
    driverExtension->RegistryPath.MaximumLength = 0;


    //
    // Unregister ETW
    //
    if (driverExtension->EtwHandle != 0) {
        EtwUnregister(driverExtension->EtwHandle);
        driverExtension->EtwHandle = 0;

        WPP_CLEANUP(DriverObject);
    }


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
#ifdef _MSC_VER
#pragma prefast(suppress:28152, "We expect the class driver to clear the DO_DEVICE_INITIALIZING flag in its AddDevice routine.")
#endif
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
{
#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer as data pointer for this use case
#endif
    PCLASS_DRIVER_EXTENSION driverExtension = IoGetDriverObjectExtension(DriverObject, CLASS_DRIVER_EXTENSION_KEY);

    NTSTATUS status;

    PAGED_CODE();


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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
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

#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer as data pointer for this use case
#endif
    driverExtension = IoGetDriverObjectExtension(DeviceObject->DriverObject, CLASS_DRIVER_EXTENSION_KEY);

    if (driverExtension){

        initData = &(driverExtension->InitData);

        if(isFdo) {
            devInfo = &(initData->FdoData);
        } else {
            devInfo = &(initData->PdoData);
        }

        ClassAcquireRemoveLock(DeviceObject, Irp);

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): minor code %#x for %s %p\n",
                       DeviceObject, Irp,
                       irpStack->MinorFunction,
                       isFdo ? "fdo" : "pdo",
                       DeviceObject));
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): previous %#x, current %#x\n",
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

                    NT_ASSERT(commonExtension->IsInitialized);

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


                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): Processing QUERY_%s irp\n",
                            DeviceObject, Irp,
                            ((irpStack->MinorFunction == IRP_MN_QUERY_STOP_DEVICE) ?
                             "STOP" : "REMOVE")));

                //
                // If this device is in use for some reason (paging, etc...)
                // then we need to fail the request.
                //

                if(commonExtension->PagingPathCount != 0) {

                    TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): device is in paging "
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
                    // this will severly mess up the state machine
                    //
                    NT_ASSERT(commonExtension->CurrentState != irpStack->MinorFunction);
                    commonExtension->PreviousState = commonExtension->CurrentState;
                    commonExtension->CurrentState = irpStack->MinorFunction;

                    if(isFdo) {
                        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): Forwarding QUERY_"
                                    "%s irp\n", DeviceObject, Irp,
                                    ((irpStack->MinorFunction == IRP_MN_QUERY_STOP_DEVICE) ?
                                     "STOP" : "REMOVE")));
                        status = ClassForwardIrpSynchronous(commonExtension, Irp);
                    }
                }


                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): Final status == %x\n",
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
                    NT_ASSERTMSG("ClassDispatchPnp !! CANCEL_STOP_DEVICE should never be failed\n", NT_SUCCESS(status));
                } else {
                    status = devInfo->ClassRemoveDevice(DeviceObject,
                                                        irpStack->MinorFunction);
                    NT_ASSERTMSG("ClassDispatchPnp !! CANCEL_REMOVE_DEVICE should never be failed\n", NT_SUCCESS(status));
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

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): got stop request for %s\n",
                            DeviceObject, Irp,
                            (isFdo ? "fdo" : "pdo")
                            ));

                NT_ASSERT(commonExtension->PagingPathCount == 0);

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

                ClasspDisableTimer((PFUNCTIONAL_DEVICE_EXTENSION)commonExtension);


                status = devInfo->ClassStopDevice(DeviceObject, IRP_MN_STOP_DEVICE);

                NT_ASSERTMSG("ClassDispatchPnp !! STOP_DEVICE should never be failed\n", NT_SUCCESS(status));

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

                //
                // Log a sytem event when non-removable disks are surprise-removed.
                //
                if (isFdo &&
                    (removeType == IRP_MN_SURPRISE_REMOVAL)) {

                    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
                    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;
                    BOOLEAN logSurpriseRemove = TRUE;
                    STORAGE_BUS_TYPE busType = fdoExtension->DeviceDescriptor->BusType;

                    //
                    // Don't log an event for VHDs
                    //
                    if (busType == BusTypeFileBackedVirtual) {
                        logSurpriseRemove = FALSE;

                    } else if (fdoData->HotplugInfo.MediaRemovable) {
                        logSurpriseRemove = FALSE;

                    } else if (fdoData->HotplugInfo.DeviceHotplug && ( busType == BusTypeUsb || busType == BusType1394)) {

                        /*
                        This device is reported as DeviceHotplug but since the busType is Usb or 1394, don't log an event
                        Note that some storage arrays may report DeviceHotplug and we would like to log an event in those cases
                        */

                        logSurpriseRemove = FALSE;
                    }

                    if (logSurpriseRemove) {

                        ClasspLogSystemEventWithDeviceNumber(DeviceObject, IO_WARNING_DISK_SURPRISE_REMOVED);
                    }

                }

                if (commonExtension->PagingPathCount != 0) {
                    TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_PNP, "ClassDispatchPnp (%p,%p): paging device is getting removed!", DeviceObject, Irp));
                }

                //
                // Release the lock for this IRP before calling in.
                //
                ClassReleaseRemoveLock(DeviceObject, Irp);
                lockReleased = TRUE;

                /*
                 *  Set IsRemoved before propagating the REMOVE down the stack.
                 *  This keeps class-initiated I/O (e.g. the MCN irp) from getting sent
                 *  after we propagate the remove.
                 */
                commonExtension->IsRemoved = REMOVE_PENDING;

                /*
                 *  If a timer was started on the device, stop it.
                 */
                 ClasspDisableTimer((PFUNCTIONAL_DEVICE_EXTENSION)commonExtension);

                /*
                 *  "Fire-and-forget" the remove irp to the lower stack.
                 *  Don't touch the irp (or the irp stack!) after this.
                 */
                if (isFdo) {


                    IoCopyCurrentIrpStackLocationToNext(Irp);
                    status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                    NT_ASSERT(NT_SUCCESS(status));
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

                DEVICE_USAGE_NOTIFICATION_TYPE type = irpStack->Parameters.UsageNotification.Type;
                BOOLEAN setPagable;


                switch(type) {

                    case DeviceUsageTypePaging: {

                        if ((irpStack->Parameters.UsageNotification.InPath) &&
                            (commonExtension->CurrentState != IRP_MN_START_DEVICE)) {

                            //
                            // Device isn't started.  Don't allow adding a
                            // paging file, but allow a removal of one.
                            //

                            status = STATUS_DEVICE_NOT_READY;
                            break;
                        }

                        NT_ASSERT(commonExtension->IsInitialized);

                        /*
                         *  Ensure that this user thread is not suspended while we are holding the PathCountEvent.
                         */
                        KeEnterCriticalRegion();

                        (VOID)KeWaitForSingleObject(&commonExtension->PathCountEvent,
                                                    Executive, KernelMode,
                                                    FALSE, NULL);
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

                        if ((!irpStack->Parameters.UsageNotification.InPath) &&
                            (commonExtension->PagingPathCount == 1)) {

                            //
                            // removing last paging file
                            // must have DO_POWER_PAGABLE bits set, but only
                            // if none set the DO_POWER_INRUSH bit and no other special files
                            //

                            if (TEST_FLAG(DeviceObject->Flags, DO_POWER_INRUSH)) {
                                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): Last "
                                            "paging file removed, but "
                                            "DO_POWER_INRUSH was set, so NOT "
                                            "setting DO_POWER_PAGABLE\n",
                                            DeviceObject, Irp));
                            } else if ((commonExtension->HibernationPathCount == 0) &&
                                       (commonExtension->DumpPathCount == 0)) {
                                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): Last "
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
                                (volatile LONG *)&commonExtension->PagingPathCount,
                                irpStack->Parameters.UsageNotification.InPath);

                            if (irpStack->Parameters.UsageNotification.InPath) {
                                if (commonExtension->PagingPathCount == 1) {
                                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): "
                                                "Clearing PAGABLE bit\n",
                                                DeviceObject, Irp));
                                    CLEAR_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);


                                }

                            }

                        } else {

                            //
                            // cleanup the changes done above
                            //

                            if (setPagable == TRUE) {
                                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): Unsetting "
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

                        //
                        // if removing last hiber device, need to set DO_POWER_PAGABLE
                        // bit here, and possible re-set it below on failure.
                        //

                        setPagable = FALSE;

                        if ((!irpStack->Parameters.UsageNotification.InPath) &&
                            (commonExtension->HibernationPathCount == 1)) {

                            //
                            // removing last hiber file
                            // must have DO_POWER_PAGABLE bits set, but only
                            // if none set the DO_POWER_INRUSH bit and no other special files
                            //

                            if (TEST_FLAG(DeviceObject->Flags, DO_POWER_INRUSH)) {
                                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): Last "
                                            "hiber file removed, but "
                                            "DO_POWER_INRUSH was set, so NOT "
                                            "setting DO_POWER_PAGABLE\n",
                                            DeviceObject, Irp));
                            } else if ((commonExtension->PagingPathCount == 0) &&
                                       (commonExtension->DumpPathCount == 0)) {
                                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): Last "
                                            "hiber file removed, "
                                            "setting DO_POWER_PAGABLE\n",
                                            DeviceObject, Irp));
                                SET_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);
                                setPagable = TRUE;
                            }

                        }

                        status = ClassForwardIrpSynchronous(commonExtension, Irp);
                        if (!NT_SUCCESS(status)) {

                            if (setPagable == TRUE) {
                                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): Unsetting "
                                            "PAGABLE bit due to irp failure\n",
                                            DeviceObject, Irp));
                                CLEAR_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);
                                setPagable = FALSE;
                            }

                        } else {

                            IoAdjustPagingPathCount(
                                (volatile LONG *)&commonExtension->HibernationPathCount,
                                irpStack->Parameters.UsageNotification.InPath
                                );

                            if ((irpStack->Parameters.UsageNotification.InPath) &&
                                (commonExtension->HibernationPathCount == 1)) {
                                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): "
                                            "Clearing PAGABLE bit\n",
                                            DeviceObject, Irp));
                                CLEAR_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);
                            }
                        }

                        break;
                    }

                    case DeviceUsageTypeDumpFile: {

                        //
                        // if removing last dump device, need to set DO_POWER_PAGABLE
                        // bit here, and possible re-set it below on failure.
                        //

                        setPagable = FALSE;

                        if ((!irpStack->Parameters.UsageNotification.InPath) &&
                            (commonExtension->DumpPathCount == 1)) {

                            //
                            // removing last dump file
                            // must have DO_POWER_PAGABLE bits set, but only
                            // if none set the DO_POWER_INRUSH bit and no other special files
                            //

                            if (TEST_FLAG(DeviceObject->Flags, DO_POWER_INRUSH)) {
                                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): Last "
                                            "dump file removed, but "
                                            "DO_POWER_INRUSH was set, so NOT "
                                            "setting DO_POWER_PAGABLE\n",
                                            DeviceObject, Irp));
                            } else if ((commonExtension->PagingPathCount == 0) &&
                                       (commonExtension->HibernationPathCount == 0)) {
                                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): Last "
                                            "dump file removed, "
                                            "setting DO_POWER_PAGABLE\n",
                                            DeviceObject, Irp));
                                SET_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);
                                setPagable = TRUE;
                            }

                        }

                        status = ClassForwardIrpSynchronous(commonExtension, Irp);
                        if (!NT_SUCCESS(status)) {

                            if (setPagable == TRUE) {
                                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): Unsetting "
                                            "PAGABLE bit due to irp failure\n",
                                            DeviceObject, Irp));
                                CLEAR_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);
                                setPagable = FALSE;
                            }

                        } else {

                            IoAdjustPagingPathCount(
                                (volatile LONG *)&commonExtension->DumpPathCount,
                                irpStack->Parameters.UsageNotification.InPath
                                );

                            if ((irpStack->Parameters.UsageNotification.InPath) &&
                                (commonExtension->DumpPathCount == 1)) {
                                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): "
                                            "Clearing PAGABLE bit\n",
                                            DeviceObject, Irp));
                                CLEAR_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);
                            }
                        }

                        break;
                    }

                    case DeviceUsageTypeBoot: {

                        if (isFdo) {
                            PCLASS_PRIVATE_FDO_DATA fdoData;

                            fdoData = ((PFUNCTIONAL_DEVICE_EXTENSION)(DeviceObject->DeviceExtension))->PrivateFdoData;


                            //
                            // If boot disk has removal policy as RemovalPolicyExpectSurpriseRemoval (e.g. disk is hotplug-able),
                            // change the removal policy to RemovalPolicyExpectOrderlyRemoval.
                            // This will cause the write cache of disk to be enabled on subsequent start Fdo (next boot).
                            //
                            if ((fdoData != NULL) &&
                                fdoData->HotplugInfo.DeviceHotplug) {

                                fdoData->HotplugInfo.DeviceHotplug = FALSE;
                                fdoData->HotplugInfo.MediaRemovable = FALSE;

                                ClassSetDeviceParameter((PFUNCTIONAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension,
                                                        CLASSP_REG_SUBKEY_NAME,
                                                        CLASSP_REG_REMOVAL_POLICY_VALUE_NAME,
                                                        RemovalPolicyExpectOrderlyRemoval);
                            }
                        }

                        status = ClassForwardIrpSynchronous(commonExtension, Irp);
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

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "ClassDispatchPnp (%p,%p): QueryCapabilities\n",
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
                            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,  "Classpnp: Clearing SR-OK bit in "
                                        "device capabilities due to hotplug "
                                        "device or media\n"));
                        }
                        deviceCapabilities->SurpriseRemovalOK = FALSE;
                    }
                    break;

                } // end QUERY_CAPABILITIES for FDOs

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
        NT_ASSERT(driverExtension);
        status = STATUS_INTERNAL_ERROR;
    }

    if (completeRequest){
        Irp->IoStatus.Status = status;

        if (!lockReleased){
            ClassReleaseRemoveLock(DeviceObject, Irp);
        }

        ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP, "ClassDispatchPnp (%p,%p): leaving with previous %#x, current %#x.", DeviceObject, Irp, commonExtension->PreviousState, commonExtension->CurrentState));
    }
    else {
        /*
         *  The irp is already completed so don't touch it.
         *  This may be a remove so don't touch the device extension.
         */
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP, "ClassDispatchPnp (%p,%p): leaving.", DeviceObject, Irp));
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
NTSTATUS ClassPnpStartDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PCLASS_DRIVER_EXTENSION driverExtension;
    PCLASS_INIT_DATA initData;

    PCLASS_DEV_INFO devInfo;

    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    BOOLEAN isFdo = commonExtension->IsFdo;

    BOOLEAN isMountedDevice = TRUE;
    BOOLEAN isPortable = FALSE;

    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_POWER_DESCRIPTOR powerDescriptor = NULL;


    PAGED_CODE();

#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer as data pointer for this use case
#endif
    driverExtension = IoGetDriverObjectExtension(DeviceObject->DriverObject, CLASS_DRIVER_EXTENSION_KEY);

    initData = &(driverExtension->InitData);
    if(isFdo) {
        devInfo = &(initData->FdoData);
    } else {
        devInfo = &(initData->PdoData);
    }

    NT_ASSERT(devInfo->ClassInitDevice != NULL);
    NT_ASSERT(devInfo->ClassStartDevice != NULL);

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
                fdoExtension->PrivateFdoData = ExAllocatePoolWithTag(NonPagedPoolNx,
                                                                     sizeof(CLASS_PRIVATE_FDO_DATA),
                                                                     CLASS_TAG_PRIVATE_DATA
                                                                     );
            }

            if (fdoExtension->PrivateFdoData == NULL) {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP,  "ClassPnpStartDevice: Cannot allocate for private fdo data\n"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlZeroMemory(fdoExtension->PrivateFdoData, sizeof(CLASS_PRIVATE_FDO_DATA));


#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
            //
            // Allocate a structure to hold more data than what we can put in FUNCTIONAL_DEVICE_EXTENSION.
            // This structure's memory is managed by classpnp, so it is more extensible.
            //
            if (fdoExtension->AdditionalFdoData == NULL) {
                fdoExtension->AdditionalFdoData = ExAllocatePoolWithTag(NonPagedPoolNx,
                                                                        sizeof(ADDITIONAL_FDO_DATA),
                                                                        CLASSPNP_POOL_TAG_ADDITIONAL_DATA
                                                                        );
            }

            if (fdoExtension->AdditionalFdoData == NULL) {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP, "ClassPnpStartDevice: Cannot allocate memory for the additional data structure.\n"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlZeroMemory(fdoExtension->AdditionalFdoData, sizeof(ADDITIONAL_FDO_DATA));
#endif

            status = ClasspInitializeTimer(fdoExtension);
            if (NT_SUCCESS(status) == FALSE) {
                FREE_POOL(fdoExtension->PrivateFdoData);
#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
                FREE_POOL(fdoExtension->AdditionalFdoData);
#endif
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP, "ClassPnpStartDevice: Failed to initialize tick timer\n"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // allocate LowerLayerSupport for class data
            //

            if (fdoExtension->FunctionSupportInfo == NULL) {
                fdoExtension->FunctionSupportInfo = (PCLASS_FUNCTION_SUPPORT_INFO)ExAllocatePoolWithTag(NonPagedPoolNx,
                                                                                                        sizeof(CLASS_FUNCTION_SUPPORT_INFO),
                                                                                                        '3BcS'
                                                                                                        );
            }

            if (fdoExtension->FunctionSupportInfo == NULL) {
                ClasspDeleteTimer(fdoExtension);
                FREE_POOL(fdoExtension->PrivateFdoData);
#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
                FREE_POOL(fdoExtension->AdditionalFdoData);
#endif
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP,  "ClassPnpStartDevice: Cannot allocate for FunctionSupportInfo\n"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // initialize the struct's various fields.
            //
            RtlZeroMemory(fdoExtension->FunctionSupportInfo, sizeof(CLASS_FUNCTION_SUPPORT_INFO));
            KeInitializeSpinLock(&fdoExtension->FunctionSupportInfo->SyncLock);

            //
            // intialize the CommandStatus to -1 indicates that no effort made yet to retrieve the info.
            // Possible values of CommandStatus (data type: NTSTATUS):
            //     -1:             It's not attempted yet to retrieve the information.
            //     success:        Command sent and succeeded, information cached in FdoExtension.
            //     failed/warning: Command is either not supported or failed by device or lower level driver.
            //                     The command should not be attempted again.
            //
            fdoExtension->FunctionSupportInfo->BlockLimitsData.CommandStatus = -1;
            fdoExtension->FunctionSupportInfo->DeviceCharacteristicsData.CommandStatus = -1;
            fdoExtension->FunctionSupportInfo->LBProvisioningData.CommandStatus = -1;
            fdoExtension->FunctionSupportInfo->ReadCapacity16Data.CommandStatus = -1;
            fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.CommandStatus = -1;


            KeInitializeTimer(&fdoExtension->PrivateFdoData->Retry.Timer);
            KeInitializeDpc(&fdoExtension->PrivateFdoData->Retry.Dpc,
                            ClasspRetryRequestDpc,
                            DeviceObject);
            KeInitializeSpinLock(&fdoExtension->PrivateFdoData->Retry.Lock);
            fdoExtension->PrivateFdoData->Retry.Granularity = KeQueryTimeIncrement();
            commonExtension->Reserved4 = (ULONG_PTR)(' GPH'); // debug aid
            InitializeListHead(&fdoExtension->PrivateFdoData->DeferredClientIrpList);

            KeInitializeSpinLock(&fdoExtension->PrivateFdoData->SpinLock);

            //
            // keep a pointer to the senseinfo2 stuff locally also (used in every read/write).
            //
            fdoExtension->PrivateFdoData->InterpretSenseInfo = driverExtension->InterpretSenseInfo;

            fdoExtension->PrivateFdoData->MaxNumberOfIoRetries = NUM_IO_RETRIES;

            //
            // Initialize release queue extended SRB
            //
            status = InitializeStorageRequestBlock(&(fdoExtension->PrivateFdoData->ReleaseQueueSrb.SrbEx),
                                                   STORAGE_ADDRESS_TYPE_BTL8,
                                                   sizeof(fdoExtension->PrivateFdoData->ReleaseQueueSrb.ReleaseQueueSrbBuffer),
                                                   0);
            if (!NT_SUCCESS(status)) {
                NT_ASSERT(FALSE);
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP,
                            "ClassPnpStartDevice: fail to initialize release queue extended SRB 0x%x\n", status));
                return status;
            }


            /*
             *  Anchor the FDO in our static list.
             *  Pnp is synchronized, so we shouldn't need any synchronization here.
             */
            InsertTailList(&AllFdosList, &fdoExtension->PrivateFdoData->AllFdosListEntry);

            //
            // NOTE: the old interface allowed the class driver to allocate
            // this.  this was unsafe for low-memory conditions. allocate one
            // unconditionally now, and modify our internal functions to use
            // our own exclusively as it is the only safe way to do this.
            //

            status = ClasspAllocateReleaseQueueIrp(fdoExtension);
            if (!NT_SUCCESS(status)) {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP,  "ClassPnpStartDevice: Cannot allocate the private release queue irp\n"));
                return status;
            }

            status = ClasspAllocatePowerProcessIrp(fdoExtension);
            if (!NT_SUCCESS(status)) {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP,  "ClassPnpStartDevice: Cannot allocate the power process irp\n"));
                return status;
            }

            //
            // Call port driver to get miniport properties for disk devices
            // It's ok for this call to fail
            //

            if ((DeviceObject->DeviceType == FILE_DEVICE_DISK) &&
                (!TEST_FLAG(DeviceObject->Characteristics, FILE_FLOPPY_DISKETTE))) {

                propertyId = StorageMiniportProperty;

                status = ClassGetDescriptor(fdoExtension->CommonExtension.LowerDeviceObject,
                                            &propertyId,
                                            (PVOID *)&fdoExtension->MiniportDescriptor);

                //
                // function ClassGetDescriptor returns succeed with buffer "fdoExtension->MiniportDescriptor" allocated.
                //
                if ( NT_SUCCESS(status) &&
                     (fdoExtension->MiniportDescriptor->Portdriver != StoragePortCodeSetStorport &&
                      fdoExtension->MiniportDescriptor->Portdriver != StoragePortCodeSetUSBport) ) {
                    //
                    // field "IoTimeoutValue" supported for either Storport or USBStor
                    //
                    fdoExtension->MiniportDescriptor->IoTimeoutValue = 0;
                }



            }

            //
            // Call port driver to get adapter capabilities.
            //

            propertyId = StorageAdapterProperty;

            status = ClassGetDescriptor(
                        commonExtension->LowerDeviceObject,
                        &propertyId,
                        (PVOID *)&fdoExtension->AdapterDescriptor);
            if (!NT_SUCCESS(status)) {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP,  "ClassPnpStartDevice: ClassGetDescriptor [ADAPTER] failed %lx\n", status));
                return status;
            }

            //
            // Call port driver to get device descriptor.
            //

            propertyId = StorageDeviceProperty;

            status = ClassGetDescriptor(
                        commonExtension->LowerDeviceObject,
                        &propertyId,
                        (PVOID *)&fdoExtension->DeviceDescriptor);
            if (NT_SUCCESS(status)){

                ClasspScanForSpecialInRegistry(fdoExtension);
                ClassScanForSpecial(fdoExtension, ClassBadItems, ClasspScanForClassHacks);

                //
                // allow perf to be re-enabled after a given number of failed IOs
                // require this number to be at least CLASS_PERF_RESTORE_MINIMUM
                //

                {
                    ULONG t = CLASS_PERF_RESTORE_MINIMUM;

                    ClassGetDeviceParameter(fdoExtension,
                                            CLASSP_REG_SUBKEY_NAME,
                                            CLASSP_REG_PERF_RESTORE_VALUE_NAME,
                                            &t);
                    if (t >= CLASS_PERF_RESTORE_MINIMUM) {
                        fdoExtension->PrivateFdoData->Perf.ReEnableThreshhold = t;
                    }
                }

                //
                // compatibility comes first.  writable cd media will not
                // get a SYNCH_CACHE on power down.
                //
                if (fdoExtension->DeviceObject->DeviceType != FILE_DEVICE_DISK) {
                    SET_FLAG(fdoExtension->PrivateFdoData->HackFlags, FDO_HACK_NO_SYNC_CACHE);
                }


                //
                // Test if the device is portable and updated the characteristics if so
                //
                status = ClasspIsPortable(fdoExtension,
                                          &isPortable);

                if (NT_SUCCESS(status) && (isPortable == TRUE)) {
                    DeviceObject->Characteristics |= FILE_PORTABLE_DEVICE;
                }

                //
                // initialize the hotplug information only after the ScanForSpecial
                // routines, as it relies upon the hack flags.
                //
                status = ClasspInitializeHotplugInfo(fdoExtension);
                if (NT_SUCCESS(status)){
                    /*
                     *  Allocate/initialize TRANSFER_PACKETs and related resources.
                     */
                    status = InitializeTransferPackets(DeviceObject);
                }
                else {
                    TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_PNP,  "ClassPnpStartDevice: Could not initialize hotplug information %lx\n", status));
                }
            }
            else {
                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP,  "ClassPnpStartDevice: ClassGetDescriptor [DEVICE] failed %lx\n", status));
                return status;
            }


            if (NT_SUCCESS(status)) {

                //
                // Retrieve info on whether async notification is supported by port drivers
                //
                propertyId = StorageDevicePowerProperty;

                status = ClassGetDescriptor(fdoExtension->CommonExtension.LowerDeviceObject,
                                            &propertyId,
                                            (PVOID *)&powerDescriptor);
                if (NT_SUCCESS(status) && (powerDescriptor != NULL)) {
                    fdoExtension->FunctionSupportInfo->AsynchronousNotificationSupported = powerDescriptor->AsynchronousNotificationSupported;
                    fdoExtension->FunctionSupportInfo->IdlePower.D3ColdSupported = powerDescriptor->D3ColdSupported;
                    fdoExtension->FunctionSupportInfo->IdlePower.NoVerifyDuringIdlePower = powerDescriptor->NoVerifyDuringIdlePower;
                    FREE_POOL(powerDescriptor);
                } else {
                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP,  "ClassPnpStartDevice: ClassGetDescriptor [DevicePower] failed %lx\n", status));

                    //
                    // Ignore error as device power property is optional
                    //
                    status = STATUS_SUCCESS;
                }
            }
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

        if (commonExtension->IsFdo) {
            fdoExtension->PrivateFdoData->Perf.OriginalSrbFlags = fdoExtension->SrbFlags;

            //
            // initialization for disk device
            //
            if ((DeviceObject->DeviceType == FILE_DEVICE_DISK) &&
                (!TEST_FLAG(DeviceObject->Characteristics, FILE_FLOPPY_DISKETTE))) {

                ULONG accessAlignmentNotSupported = 0;
                ULONG qerrOverrideMode = QERR_SET_ZERO_ODX_OR_TP_ONLY;
                ULONG legacyErrorHandling = FALSE;

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_PNP,
                           "ClassPnpStartDevice: Enabling idle timer for %p\n", DeviceObject));
                // Initialize idle timer for disk devices
                ClasspInitializeIdleTimer(fdoExtension);

                if (ClasspIsObsoletePortDriver(fdoExtension) == FALSE) {
                    // get INQUIRY VPD support information. It's safe to send command as everything is ready in ClassInitDevice().
                    ClasspGetInquiryVpdSupportInfo(fdoExtension);

                    // Query and cache away Logical Block Provisioning info in the FDO extension.
                    // The cached information will be used in responding to some IOCTLs
                    ClasspGetLBProvisioningInfo(fdoExtension);

                    //
                    // Query and cache away Block Device ROD Limits info in the FDO extension.
                    //
                    if (fdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockDeviceRODLimits) {
                        ClassDetermineTokenOperationCommandSupport(DeviceObject);
                    }

                    //
                    // See if the user has specified a particular QERR override
                    // mode. "Override" meaning setting QERR = 0 via Mode Select.
                    //  0 = Only when ODX or Thin Provisioning are supported (default)
                    //  1 = Always
                    //  2 = Never (or any value >= 2)
                    //
                    ClassGetDeviceParameter(fdoExtension,
                                            CLASSP_REG_SUBKEY_NAME,
                                            CLASSP_REG_QERR_OVERRIDE_MODE,
                                            &qerrOverrideMode);

                    //
                    // If this device is thinly provisioned or supports ODX, we
                    // may need to force QERR to zero.  The user may have also
                    // specified that we should always or never do this.
                    //
                    if (qerrOverrideMode == QERR_SET_ZERO_ALWAYS ||
                        (qerrOverrideMode == QERR_SET_ZERO_ODX_OR_TP_ONLY &&
                         (ClasspIsThinProvisioned(fdoExtension->FunctionSupportInfo) ||
                          NT_SUCCESS(ClasspValidateOffloadSupported(DeviceObject, NULL))))) {

                        ClasspZeroQERR(DeviceObject);
                    }

                } else {

                    //
                    // Since this device has been exposed by a legacy miniport (e.g. SCSIPort miniport)
                    // set its LB Provisioning command status to an error status that will be surfaced
                    // up to the caller of a TRIM/Unmap command.
                    //
                    fdoExtension->FunctionSupportInfo->LBProvisioningData.CommandStatus = STATUS_UNSUCCESSFUL;
                    fdoExtension->FunctionSupportInfo->BlockLimitsData.CommandStatus = STATUS_UNSUCCESSFUL;
                    fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.CommandStatus = STATUS_UNSUCCESSFUL;
                }

                // Get registry setting of failing the IOCTL for AccessAlignment Property.
                ClassGetDeviceParameter(fdoExtension,
                                        CLASSP_REG_SUBKEY_NAME,
                                        CLASSP_REG_ACCESS_ALIGNMENT_NOT_SUPPORTED,
                                        &accessAlignmentNotSupported);

                if (accessAlignmentNotSupported > 0) {
                    fdoExtension->FunctionSupportInfo->RegAccessAlignmentQueryNotSupported = TRUE;
                }

#if (NTDDI_VERSION >= NTDDI_WINBLUE)


                //
                // See if the user has specified legacy error handling.
                //
                ClassGetDeviceParameter(fdoExtension,
                                        CLASSP_REG_SUBKEY_NAME,
                                        CLASSP_REG_LEGACY_ERROR_HANDLING,
                                        &legacyErrorHandling);

                if (legacyErrorHandling) {
                    //
                    // Legacy error handling means that the maximum number of
                    // retries allowd for an IO request is 8 instead of 4.
                    //
                    fdoExtension->PrivateFdoData->MaxNumberOfIoRetries = LEGACY_NUM_IO_RETRIES;
                    fdoExtension->PrivateFdoData->LegacyErrorHandling = TRUE;
                }
#else
                UNREFERENCED_PARAMETER(legacyErrorHandling);
#endif


                //
                // Get the copy offload max target duration value.
                // This function will set the default value if one hasn't been
                // specified in the registry.
                //
                ClasspGetCopyOffloadMaxDuration(DeviceObject,
                                                REG_DISK_CLASS_CONTROL,
                                                &(fdoExtension->PrivateFdoData->CopyOffloadMaxTargetDuration));

            }

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
    }

    //
    // If device requests autorun functionality or a once a second callback
    // then enable the once per second timer. Exception is if media change
    // detection is desired but device supports async notification.
    //
    // NOTE: This assumes that ClassInitializeMediaChangeDetection is always
    //       called in the context of the ClassInitDevice callback. If called
    //       after then this check will have already been made and the
    //       once a second timer will not have been enabled.
    //
    if ((isFdo) &&
        ((initData->ClassTick != NULL) ||
         ((fdoExtension->MediaChangeDetectionInfo != NULL) &&
          (fdoExtension->FunctionSupportInfo != NULL) &&
          (fdoExtension->FunctionSupportInfo->AsynchronousNotificationSupported == FALSE)) ||
         ((fdoExtension->FailurePredictionInfo != NULL) &&
          (fdoExtension->FailurePredictionInfo->Method != FailurePredictionNone))))
    {
        ClasspEnableTimer(fdoExtension);

        //
        // In addition, we may change our polling behavior when the screen is
        // off so register for screen state notification if we haven't already
        // done so.
        //
        if (ScreenStateNotificationHandle == NULL) {
            PoRegisterPowerSettingCallback(DeviceObject,
                                            &GUID_CONSOLE_DISPLAY_STATE,
                                            &ClasspPowerSettingCallback,
                                            NULL,
                                            &ScreenStateNotificationHandle);
        }
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

    if (NT_SUCCESS(status)){
        commonExtension->CurrentState = IRP_MN_START_DEVICE;

        if((isFdo) && (initData->ClassEnumerateDevice != NULL)) {
            isMountedDevice = FALSE;
        }

        if (DeviceObject->DeviceType != FILE_DEVICE_CD_ROM) {

            isMountedDevice = FALSE;
        }

        //
        // Register for mounted device interface if this is a
        // sfloppy device.
        //
        if ((DeviceObject->DeviceType == FILE_DEVICE_DISK) &&
            (TEST_FLAG(DeviceObject->Characteristics, FILE_FLOPPY_DISKETTE))) {

            isMountedDevice = TRUE;
        }

        if(isMountedDevice) {
            ClasspRegisterMountedDeviceInterface(DeviceObject);
        }

        if(commonExtension->IsFdo) {
            IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_REGISTER);

            //
            // Tell Storport (Usbstor or SD) to enable idle power management for this
            // device, assuming the user hasn't turned it off in the registry.
            //
            if (fdoExtension->FunctionSupportInfo != NULL &&
                fdoExtension->FunctionSupportInfo->IdlePower.IdlePowerEnabled == FALSE &&
                fdoExtension->MiniportDescriptor != NULL &&
                (fdoExtension->MiniportDescriptor->Portdriver == StoragePortCodeSetStorport ||
                 fdoExtension->MiniportDescriptor->Portdriver == StoragePortCodeSetSDport ||
                 fdoExtension->MiniportDescriptor->Portdriver == StoragePortCodeSetUSBport)) {
                ULONG disableIdlePower= 0;
                ClassGetDeviceParameter(fdoExtension,
                                        CLASSP_REG_SUBKEY_NAME,
                                        CLASSP_REG_DISBALE_IDLE_POWER_NAME,
                                        &disableIdlePower);

                if (!disableIdlePower) {
                    ClasspEnableIdlePower(DeviceObject);
                }
            }
        }
    }
    else {
        ClasspDisableTimer(fdoExtension);
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

    IF the device object is an FDO (paritition 0 object), the number of bytes
    in the request are checked against the maximum byte counts that the adapter
    supports and requests are broken up into
    smaller sizes if necessary.

Arguments:

    DeviceObject - a pointer to the device object for this request

    Irp - IO request

Return Value:

    NT Status

--*/
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassReadWrite(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT      lowerDeviceObject = commonExtension->LowerDeviceObject;
    PIO_STACK_LOCATION  currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG               transferByteCount = currentIrpStack->Parameters.Read.Length;
    ULONG               isRemoved;
    NTSTATUS            status;

    /*
     *  Grab the remove lock.  If we can't acquire it, bail out.
     */
    isRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);
    if (isRemoved) {
        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        Irp->IoStatus.Information = 0;
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
        if (Irp->Tail.Overlay.Thread != NULL) {
            IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
        }
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
        NT_ASSERT(commonExtension->DevInfo->ClassReadWriteVerification);
        status = commonExtension->DevInfo->ClassReadWriteVerification(DeviceObject, Irp);
        // Code Analysis cannot analyze the code paths specific to clients.
        _Analysis_assume_(status != STATUS_PENDING);
        if (!NT_SUCCESS(status)){
            NT_ASSERT(Irp->IoStatus.Status == status);
            Irp->IoStatus.Information = 0;
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

                        PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
                        PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;

                        /*
                         *  Add in any skew for the disk manager software.
                         */
                        currentIrpStack->Parameters.Read.ByteOffset.QuadPart +=
                             commonExtension->PartitionZeroExtension->DMByteSkew;

                        //
                        // In case DEV_USE_16BYTE_CDB flag is not set, R/W request will be translated into READ/WRITE 10 SCSI command.
                        // These SCSI commands have 4 bytes in "Starting LBA" field.
                        // Requests cannot be represented in these SCSI commands should be failed.
                        //
                        if (!TEST_FLAG(fdoExtension->DeviceFlags, DEV_USE_16BYTE_CDB)) {
                            LARGE_INTEGER startingLba;

                            startingLba.QuadPart = currentIrpStack->Parameters.Read.ByteOffset.QuadPart >> fdoExtension->SectorShift;

                            if (startingLba.QuadPart > MAXULONG) {
                                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                                Irp->IoStatus.Information = 0;
                                ClassReleaseRemoveLock(DeviceObject, Irp);
                                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
                                status = STATUS_INVALID_PARAMETER;
                                goto Exit;
                            }
                        }

#if DBG
                        //
                        // Record the caller if:
                        // 1. the disk is currently off
                        // 2. the operation is a read, (likely resulting in disk spinnage)
                        // 3. the operation is marked WT (likely resulting in disk spinnage)
                        //
                        if((fdoExtension->DevicePowerState == PowerDeviceD3) && // disk is off
                            ((currentIrpStack->MajorFunction == IRP_MJ_READ) ||                       // It's a read.
                             (TEST_FLAG(currentIrpStack->Flags, SL_WRITE_THROUGH))) ) {               // they *really* want it to go to disk.

                            SnapDiskStartup();
                        }
#endif

                        /*
                         *  Perform the actual transfer(s) on the hardware
                         *  to service this request.
                         */
                        if (ClasspIsIdleRequestSupported(fdoData, Irp)) {
                            ClasspMarkIrpAsIdle(Irp, TRUE);
                            status = ClasspEnqueueIdleRequest(DeviceObject, Irp);
                        } else {
                            UCHAR uniqueAddr = 0;

                            //
                            // Since we're touching fdoData after servicing the transfer packet, this opens us up to
                            // a potential window, where the device may be removed between the time that
                            // ServiceTransferPacket completes and we've had a chance to access fdoData. In order
                            // to guard against this, we acquire the removelock an additional time here. This
                            // acquire is guaranteed to succeed otherwise we wouldn't be here (because of the
                            // outer acquire).
                            // The sequence of events we're guarding against with this remLock acquire is:
                            // 1. This UL IRP acquired the lock.
                            // 2. Device gets surprised removed, then gets IRP_MN_REMOVE_DEVICE; ClassRemoveDevice
                            //    waits for the above RemoveLock.
                            // 3. ServiceTransferRequest breaks the UL IRP into DL IRPs.
                            // 4. DL IRPs complete with STATUS_NO_SUCH_DEVICE and TransferPktComplete completes the UL
                            //    IRP with STATUS_NO_SUCH_DEVICE; releases the RemoveLock.
                            // 5. ClassRemoveDevice is now unblocked, continues running and frees resources (including
                            //    fdoData).
                            // 6. Finally ClassReadWrite gets to run again and accesses a freed fdoData when trying to
                            //    check/update idle-related fields.
                            //
                            ClassAcquireRemoveLock(DeviceObject, (PVOID)&uniqueAddr);

                            ClasspMarkIrpAsIdle(Irp, FALSE);
                            status = ServiceTransferRequest(DeviceObject, Irp, FALSE);
                            if (fdoData->IdlePrioritySupported == TRUE) {
                                fdoData->LastNonIdleIoTime = ClasspGetCurrentTime();
                            }

                            ClassReleaseRemoveLock(DeviceObject, (PVOID)&uniqueAddr);
                        }
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

Exit:
    return status;
}


VOID InterpretCapacityData(PDEVICE_OBJECT Fdo, PREAD_CAPACITY_DATA_EX ReadCapacityData)
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    ULONG cylinderSize;
    ULONG bytesPerSector;
    LARGE_INTEGER lastSector;
    LARGE_INTEGER largeInt;

    bytesPerSector = ClasspCalculateLogicalSectorSize(Fdo, ReadCapacityData->BytesPerBlock);

    fdoExt->DiskGeometry.BytesPerSector = bytesPerSector;
    WHICH_BIT(fdoExt->DiskGeometry.BytesPerSector, fdoExt->SectorShift);

    /*
     *  LogicalBlockAddress is the last sector of the logical drive, in big-endian.
     *  It tells us the size of the drive (#sectors is lastSector+1).
     */

    largeInt = ReadCapacityData->LogicalBlockAddress;
    REVERSE_BYTES_QUAD(&lastSector, &largeInt);

    if (fdoExt->DMActive){
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,  "ClassReadDriveCapacity: reducing number of sectors by %d\n", fdoExt->DMSkew));
        lastSector.QuadPart -= fdoExt->DMSkew;
    }

    /*
     *  Check to see if we have a geometry we should be using already.
     *  If not, we set part of the disk geometry to garbage values that will be filled in by the caller (e.g. disk.sys).
     *
     *  So the first call to ClassReadDriveCapacity always sets a meaningless geometry.
     *  TracksPerCylinder and SectorsPerTrack are kind of meaningless anyway wrt I/O,
     *  because I/O is always targeted to a logical sector number.
     *  All that really matters is BytesPerSector and the number of sectors.
     */
    cylinderSize = fdoExt->DiskGeometry.TracksPerCylinder * fdoExt->DiskGeometry.SectorsPerTrack;
    if (cylinderSize == 0){
        fdoExt->DiskGeometry.TracksPerCylinder = 0xff;
        fdoExt->DiskGeometry.SectorsPerTrack = 0x3f;
        cylinderSize = fdoExt->DiskGeometry.TracksPerCylinder * fdoExt->DiskGeometry.SectorsPerTrack;
    }

    /*
     *  Calculate number of cylinders.
     *  If there are zero cylinders, then the device lied AND it's
     *  smaller than 0xff*0x3f (about 16k sectors, usually 8 meg)
     *  this can fit into a single LONGLONG, so create another usable
     *  geometry, even if it's unusual looking.
     *  This allows small, non-standard devices, such as Sony's Memory Stick, to show up as having a partition.
     */
    fdoExt->DiskGeometry.Cylinders.QuadPart = (LONGLONG)((lastSector.QuadPart + 1)/cylinderSize);
    if (fdoExt->DiskGeometry.Cylinders.QuadPart == (LONGLONG)0) {
        fdoExt->DiskGeometry.SectorsPerTrack    = 1;
        fdoExt->DiskGeometry.TracksPerCylinder  = 1;
        fdoExt->DiskGeometry.Cylinders.QuadPart = lastSector.QuadPart + 1;
    }

    /*
     *  Calculate media capacity in bytes.
     *  For this purpose we treat the entire LUN as is if it is one partition.  Disk will deal with actual partitioning.
     */
    fdoExt->CommonExtension.PartitionLength.QuadPart =
        ((LONGLONG)(lastSector.QuadPart + 1)) << fdoExt->SectorShift;

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
_Must_inspect_result_
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassReadDriveCapacity(_In_ PDEVICE_OBJECT Fdo)
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    READ_CAPACITY_DATA_EX PTRALIGN readCapacityData = {0};
    PTRANSFER_PACKET pkt;
    NTSTATUS status;
    PMDL driveCapMdl = NULL;
    KEVENT event;
    IRP pseudoIrp = {0};
    ULONG readCapacityDataSize;
    BOOLEAN use16ByteCdb;
    BOOLEAN match = TRUE;

    use16ByteCdb = TEST_FLAG(fdoExt->DeviceFlags, DEV_USE_16BYTE_CDB);

RetryRequest:

    if (use16ByteCdb) {
        readCapacityDataSize = sizeof(READ_CAPACITY_DATA_EX);
    } else {
        readCapacityDataSize = sizeof(READ_CAPACITY_DATA);
    }

    if (driveCapMdl != NULL) {
        FreeDeviceInputMdl(driveCapMdl);
        driveCapMdl = NULL;
    }

    //
    // Allocate the MDL based on the Read Capacity command.
    //
    driveCapMdl = BuildDeviceInputMdl(&readCapacityData, readCapacityDataSize);
    if (driveCapMdl == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto SafeExit;
    }

    pkt = DequeueFreeTransferPacket(Fdo, TRUE);
    if (pkt == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto SafeExit;
    }

    //
    //  Our engine needs an "original irp" to write the status back to
    //  and to count down packets (one in this case).
    //  Just use a pretend irp for this.
    //

    pseudoIrp.Tail.Overlay.DriverContext[0] = LongToPtr(1);
    pseudoIrp.IoStatus.Status = STATUS_SUCCESS;
    pseudoIrp.IoStatus.Information = 0;
    pseudoIrp.MdlAddress = driveCapMdl;

    //
    //  Set this up as a SYNCHRONOUS transfer, submit it,
    //  and wait for the packet to complete.  The result
    //  status will be written to the original irp.
    //

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);
    SetupDriveCapacityTransferPacket(pkt,
                                     &readCapacityData,
                                     readCapacityDataSize,
                                     &event,
                                     &pseudoIrp,
                                     use16ByteCdb);
    SubmitTransferPacket(pkt);
    (VOID)KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    status = pseudoIrp.IoStatus.Status;

    //
    //  If we got an UNDERRUN, retry exactly once.
    //  (The transfer_packet engine didn't retry because the result
    //   status was success).
    //

    if (NT_SUCCESS(status) &&
       (pseudoIrp.IoStatus.Information < readCapacityDataSize)) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT, "ClassReadDriveCapacity: read len (%xh) < %xh, retrying ...",
                (ULONG)pseudoIrp.IoStatus.Information, readCapacityDataSize));

        pkt = DequeueFreeTransferPacket(Fdo, TRUE);
        if (pkt) {
            pseudoIrp.Tail.Overlay.DriverContext[0] = LongToPtr(1);
            pseudoIrp.IoStatus.Status = STATUS_SUCCESS;
            pseudoIrp.IoStatus.Information = 0;
            KeInitializeEvent(&event, SynchronizationEvent, FALSE);
            SetupDriveCapacityTransferPacket(pkt,
                                             &readCapacityData,
                                             readCapacityDataSize,
                                             &event,
                                             &pseudoIrp,
                                             use16ByteCdb);
            SubmitTransferPacket(pkt);
            (VOID)KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = pseudoIrp.IoStatus.Status;
            if (pseudoIrp.IoStatus.Information < readCapacityDataSize){
                status = STATUS_DEVICE_BUSY;
            }
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(status)) {
        //
        //  The request succeeded. Check for 8 byte LBA support.
        //

        if (use16ByteCdb == FALSE) {

            PREAD_CAPACITY_DATA readCapacity;

            //
            // Check whether the device supports 8 byte LBA. If the device supports
            // it then retry the request using 16 byte CDB.
            //

            readCapacity = (PREAD_CAPACITY_DATA) &readCapacityData;

            if (readCapacity->LogicalBlockAddress == 0xFFFFFFFF) {
                //
                // Device returned max size for last LBA. Need to send
                // 16 byte request to get the size.
                //
                use16ByteCdb = TRUE;
                goto RetryRequest;

            } else {
                //
                // Convert the 4 byte LBA (READ_CAPACITY_DATA) to 8 byte LBA (READ_CAPACITY_DATA_EX)
                // format for ease of use. This is the only format stored in the device extension.
                //

                RtlMoveMemory((PUCHAR)(&readCapacityData) + sizeof(ULONG), readCapacity, sizeof(READ_CAPACITY_DATA));
                RtlZeroMemory((PUCHAR)(&readCapacityData), sizeof(ULONG));

            }
        } else {
            //
            // Device completed 16 byte command successfully, it supports 8-byte LBA.
            //

            SET_FLAG(fdoExt->DeviceFlags, DEV_USE_16BYTE_CDB);
        }

        //
        // Read out and store the drive information.
        //

        InterpretCapacityData(Fdo, &readCapacityData);

        //
        // Before caching the new drive capacity, compare it with the
        // cached capacity for any change.
        //

        if (fdoData->IsCachedDriveCapDataValid == TRUE) {

            match = (BOOLEAN) RtlEqualMemory(&fdoData->LastKnownDriveCapacityData,
                                    &readCapacityData, sizeof(READ_CAPACITY_DATA_EX));
        }

        //
        // Store the readCapacityData in private FDO data.
        // This is so that runtime memory failures don't cause disk.sys to put
        // the paging disk in an error state. Also this is used in
        // IOCTL_STORAGE_READ_CAPACITY.
        //
        fdoData->LastKnownDriveCapacityData = readCapacityData;
        fdoData->IsCachedDriveCapDataValid = TRUE;

        if (match == FALSE) {
            if (commonExtension->CurrentState != IRP_MN_START_DEVICE)
            {
                //
                // This can happen if a disk reports Parameters Changed / Capacity Data Changed sense data.
                // NT_ASSERT(!"Drive capacity has changed while the device wasn't started!");
                //
            } else {
                //
                // state of (commonExtension->CurrentState == IRP_MN_START_DEVICE) indicates that the device has been started.
                // UpdateDiskPropertiesWorkItemActive is used as a flag to ensure we only have one work item updating the disk
                // properties at a time.
                //
                if (InterlockedCompareExchange((volatile LONG *)&fdoData->UpdateDiskPropertiesWorkItemActive, 1, 0) == 0)
                {
                    PIO_WORKITEM workItem;

                    workItem = IoAllocateWorkItem(Fdo);

                    if (workItem) {
                        //
                        // The disk capacity has changed, send notification to the disk driver.
                        // Start a work item to notify the disk class driver asynchronously.
                        //
                        IoQueueWorkItem(workItem, ClasspUpdateDiskProperties, DelayedWorkQueue, workItem);
                    } else {
                        InterlockedExchange((volatile LONG *)&fdoData->UpdateDiskPropertiesWorkItemActive, 0);
                    }
                }
            }
        }

    } else {
        //
        //  The request failed.
        //

        //
        // ISSUE - 2000/02/04 - henrygab - non-512-byte sector sizes and failed geometry update
        //    what happens when the disk's sector size is bigger than
        //    512 bytes and we hit this code path?  this is untested.
        //
        // If the read capacity fails, set the geometry to reasonable parameter
        // so things don't fail at unexpected places.  Zero the geometry
        // except for the bytes per sector and sector shift.
        //

        //
        //  This request can sometimes fail legitimately
        //  (e.g. when a SCSI device is attached but turned off)
        //  so this is not necessarily a device/driver bug.
        //

        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT, "ClassReadDriveCapacity on Fdo %p failed with status %ul.", Fdo, status));

        //
        //  Write in a default disk geometry which we HOPE is right (??).
        //

        RtlZeroMemory(&fdoExt->DiskGeometry, sizeof(DISK_GEOMETRY));
        fdoExt->DiskGeometry.BytesPerSector = 512;
        fdoExt->SectorShift = 9;
        fdoExt->CommonExtension.PartitionLength.QuadPart = (LONGLONG) 0;

        //
        //  Is this removable or fixed media
        //

        if (TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA)){
            fdoExt->DiskGeometry.MediaType = RemovableMedia;
        } else {
            fdoExt->DiskGeometry.MediaType = FixedMedia;
        }
    }

SafeExit:


    //
    // In case DEV_USE_16BYTE_CDB flag is not set, classpnp translates R/W request into READ/WRITE 10 SCSI command.
    // These SCSI commands have 2 bytes in "Transfer Blocks" field.
    // Make sure this max length (0xFFFF * sector size) is respected during request split.
    //
    if (!TEST_FLAG(fdoExt->DeviceFlags, DEV_USE_16BYTE_CDB)) {
        ULONG cdb10MaxBlocks = ((ULONG)USHORT_MAX) << fdoExt->SectorShift;

        fdoData->HwMaxXferLen = min(cdb10MaxBlocks, fdoData->HwMaxXferLen);
    }

    if (driveCapMdl != NULL) {
        FreeDeviceInputMdl(driveCapMdl);
    }

    //
    // If the request failed for some reason then invalidate the cached
    // capacity data for removable devices. So that we won't return
    // wrong capacity in IOCTL_STORAGE_READ_CAPACITY
    //

    if (!NT_SUCCESS(status) && (fdoExt->DiskGeometry.MediaType == RemovableMedia)) {
        fdoData->IsCachedDriveCapDataValid = FALSE;
    }

    //
    //  Don't let memory failures (either here or in the port driver) in the ReadDriveCapacity call
    //  put the paging disk in an error state such that paging fails.
    //  Return the last known drive capacity (which may possibly be slightly out of date, even on
    //  fixed media, e.g. for storage cabinets that can grow a logical disk).
    //
    if ((status == STATUS_INSUFFICIENT_RESOURCES) &&
        (fdoData->IsCachedDriveCapDataValid) &&
        (fdoExt->DiskGeometry.MediaType == FixedMedia)) {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT, "ClassReadDriveCapacity: defaulting to cached DriveCapacity data"));
        InterpretCapacityData(Fdo, &fdoData->LastKnownDriveCapacityData);
        status = STATUS_SUCCESS;
    }

    return status;
}


/*++////////////////////////////////////////////////////////////////////////////

ClassSendStartUnit()

Routine Description:

    Send command to SCSI unit to start or power up.
    Because this command is issued asynchronounsly, that is, without
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassSendStartUnit(
    _In_ PDEVICE_OBJECT Fdo
    )
{
    PIO_STACK_LOCATION irpStack;
    PIRP irp;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PSCSI_REQUEST_BLOCK srb;
    PCOMPLETION_CONTEXT context;
    PCDB cdb;
    NTSTATUS status;
    PSTORAGE_REQUEST_BLOCK srbEx;

    //
    // Allocate Srb from nonpaged pool.
    //

    context = ExAllocatePoolWithTag(NonPagedPoolNx,
                             sizeof(COMPLETION_CONTEXT),
                             '6CcS');

    if (context == NULL) {

        //
        // ISSUE-2000/02/03-peterwie
        // This code path was inheritted from the NT 4.0 class2.sys driver.
        // It needs to be changed to survive low-memory conditions.
        //

        KeBugCheck(SCSI_DISK_DRIVER_INTERNAL);
    }

    //
    // Save the device object in the context for use by the completion
    // routine.
    //

    context->DeviceObject = Fdo;

    srb = &context->Srb.Srb;
    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srbEx = &context->Srb.SrbEx;
        status = InitializeStorageRequestBlock(srbEx,
                                               STORAGE_ADDRESS_TYPE_BTL8,
                                               sizeof(context->Srb.SrbExBuffer),
                                               1,
                                               SrbExDataTypeScsiCdb16);
        if (!NT_SUCCESS(status)) {
            FREE_POOL(context);
            NT_ASSERT(FALSE);
            return;
        }

        srbEx->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;

    } else {

        //
        // Zero out srb.
        //

        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));

        //
        // Write length to SRB.
        //

        srb->Length = sizeof(SCSI_REQUEST_BLOCK);

        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    }

    //
    // Set timeout value large enough for drive to spin up.
    //

    SrbSetTimeOutValue(srb, START_UNIT_TIMEOUT);

    //
    // Set the transfer length.
    //

    SrbAssignSrbFlags(srb,
                      (SRB_FLAGS_NO_DATA_TRANSFER  |
                       SRB_FLAGS_DISABLE_AUTOSENSE |
                       SRB_FLAGS_DISABLE_SYNCH_TRANSFER));

    //
    // Build the start unit CDB.
    //

    SrbSetCdbLength(srb, 6);
    cdb = SrbGetCdb(srb);

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

    if (irp == NULL) {

        //
        // ISSUE-2000/02/03-peterwie
        // This code path was inheritted from the NT 4.0 class2.sys driver.
        // It needs to be changed to survive low-memory conditions.
        //

        KeBugCheck(SCSI_DISK_DRIVER_INTERNAL);

    }

    ClassAcquireRemoveLock(Fdo, irp);

    IoSetCompletionRoutine(irp,
                           (PIO_COMPLETION_ROUTINE)ClassAsynchronousCompletion,
                           context,
                           TRUE,
                           TRUE,
                           TRUE);

    irpStack = IoGetNextIrpStackLocation(irp);
    irpStack->MajorFunction = IRP_MJ_SCSI;
    SrbSetOriginalRequest(srb, irp);

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
    which was issused by the class driver completes.  Examples of such requests
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassAsynchronousCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    PCOMPLETION_CONTEXT context = Context;
    PSCSI_REQUEST_BLOCK srb;
    ULONG srbFunction;
    ULONG srbFlags;

    if (context == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (DeviceObject == NULL) {

        DeviceObject = context->DeviceObject;
    }

    srb = &context->Srb.Srb;

    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK) {
        srbFunction = ((PSTORAGE_REQUEST_BLOCK)srb)->SrbFunction;
        srbFlags = ((PSTORAGE_REQUEST_BLOCK)srb)->SrbFlags;
    } else {
        srbFunction = srb->Function;
        srbFlags = srb->SrbFlags;
    }

    //
    // If this is an execute srb, then check the return status and make sure.
    // the queue is not frozen.
    //

    if (srbFunction == SRB_FUNCTION_EXECUTE_SCSI) {

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

            NT_ASSERT(!TEST_FLAG(srbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));

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

    FREE_POOL(context);

    IoFreeIrp(Irp);

    //
    // Indicate the I/O system should stop processing the Irp completion.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

} // end ClassAsynchronousCompletion()


NTSTATUS
ServiceTransferRequest(
    PDEVICE_OBJECT Fdo,
    PIRP Irp,
    BOOLEAN PostToDpc
    )

/*++

Routine description:

    This routine processes Io requests, splitting them if they
    are larger than what the hardware can handle at a time. If
    there isn't enough memory available, the request is placed
    in a queue, to be processed at a later time

    If this is a high priority  paging request, all regular Io
    are throttled to provide Mm with better thoroughput

Arguments:

    Fdo - The functional device object processing the request
    Irp - The Io request to be processed
    PostToDpc - Flag that indicates that this IRP must be posted to a DPC

Return Value:

    STATUS_SUCCESS if successful, an error code otherwise

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension = Fdo->DeviceExtension;
    PSTORAGE_ADAPTER_DESCRIPTOR adapterDesc = commonExtension->PartitionZeroExtension->AdapterDescriptor;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    IO_PAGING_PRIORITY priority = (TEST_FLAG(Irp->Flags, IRP_PAGING_IO)) ? IoGetPagingIoPriority(Irp) : IoPagingPriorityInvalid;
    BOOLEAN deferClientIrp = FALSE;
    BOOLEAN driverUsesStartIO = (commonExtension->DriverExtension->InitData.ClassStartIo != NULL);
    KIRQL oldIrql;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION  currentIrpStack = IoGetCurrentIrpStackLocation(Irp);

    /*
     * Initialize IRP status for the master IRP to
     * - STATUS_FT_READ_FROM_COPY if it's a copy-specific read
     * - STATUS_SUCCESS otherwise.
     *
     * This is required. When Classpnp determines the status for the master IRP
     * when completing child IRPs, the call to IoSetMasterIrpStatus
     * will be functioning properly (See TransferPktComplete function)
     *
     * Note:
     * If the IRP is a copy-specific read, File System already initialized the IRP status
     * to be STATUS_FT_READ_FROM_COPY. However, this can be changed when the IRP arrives
     * at Classpnp. It's possible that other drivers in the stack may initialize the
     * IRP status field to other values before forwarding the IRP down the stack.
     * To be defensive, we initialize the IRP status to either STATUS_FT_READ_FROM_COPY
     * if it's a copy-specific read, or STATUS_SUCCESS otherwise.
     */
    if (currentIrpStack->MajorFunction == IRP_MJ_READ &&
        TEST_FLAG(currentIrpStack->Flags, SL_KEY_SPECIFIED) &&
        IsKeyReadCopyNumber(currentIrpStack->Parameters.Read.Key)) {
        Irp->IoStatus.Status = STATUS_FT_READ_FROM_COPY;
    } else {
        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    //
    // If this is a high priority request, hold off all other Io requests
    //

    if (priority == IoPagingPriorityHigh)
    {
        KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);

        if (fdoData->NumHighPriorityPagingIo == 0)
        {
            //
            // Entering throttle mode
            //

            KeQuerySystemTime(&fdoData->ThrottleStartTime);
        }

        fdoData->NumHighPriorityPagingIo++;
        fdoData->MaxInterleavedNormalIo += ClassMaxInterleavePerCriticalIo;

        KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);
    }
    else
    {
        if (fdoData->NumHighPriorityPagingIo != 0)
        {
            //
            // This request wasn't flagged as critical and atleast one critical request
            // is currently outstanding. Queue this request until all of those are done
            // but only if the interleave threshold has been reached
            //

            KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);

            if (fdoData->NumHighPriorityPagingIo != 0)
            {
                if (fdoData->MaxInterleavedNormalIo == 0)
                {
                    deferClientIrp = TRUE;
                }
                else
                {
                    fdoData->MaxInterleavedNormalIo--;
                }
            }

            KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);
        }
    }

    if (!deferClientIrp)
    {
        PIO_STACK_LOCATION currentSp = IoGetCurrentIrpStackLocation(Irp);
        ULONG entireXferLen = currentSp->Parameters.Read.Length;
        PUCHAR bufPtr = MmGetMdlVirtualAddress(Irp->MdlAddress);
        LARGE_INTEGER targetLocation = currentSp->Parameters.Read.ByteOffset;
        PTRANSFER_PACKET pkt;
        SINGLE_LIST_ENTRY pktList;
        PSINGLE_LIST_ENTRY slistEntry;
        ULONG hwMaxXferLen;
        ULONG numPackets;
        ULONG i;


        /*
         *  We precomputed fdoData->HwMaxXferLen using (MaximumPhysicalPages-1).
         *  If the buffer is page-aligned, that's one less page crossing so we can add the page back in.
         *  Note: adapters that return MaximumPhysicalPages=0x10 depend on this to
         *           transfer aligned 64K requests in one piece.
         *  Also note:  make sure adding PAGE_SIZE back in doesn't wrap to zero.
         */
        if (((ULONG_PTR)bufPtr & (PAGE_SIZE-1)) || (fdoData->HwMaxXferLen > 0xffffffff-PAGE_SIZE)){
            hwMaxXferLen = fdoData->HwMaxXferLen;
        }
        else {
            NT_ASSERT((PAGE_SIZE%fdoExt->DiskGeometry.BytesPerSector) == 0);
            hwMaxXferLen = min(fdoData->HwMaxXferLen+PAGE_SIZE, adapterDesc->MaximumTransferLength);
        }

        /*
         *  Compute the number of hw xfers we'll have to do.
         *  Calculate this without allowing for an overflow condition.
         */
        NT_ASSERT(hwMaxXferLen >= PAGE_SIZE);
        numPackets = entireXferLen/hwMaxXferLen;
        if (entireXferLen % hwMaxXferLen){
            numPackets++;
        }

        /*
         *  Use our 'simple' slist functions since we don't need interlocked.
         */
        SimpleInitSlistHdr(&pktList);

        if (driverUsesStartIO) {
            /*
             * special case: StartIO-based writing must stay serialized, so just
             * re-use one packet.
             */
            pkt = DequeueFreeTransferPacket(Fdo, TRUE);
            if (pkt) {
                SimplePushSlist(&pktList, (PSINGLE_LIST_ENTRY)&pkt->SlistEntry);
                i = 1;
            } else {
                i = 0;
            }
        } else {
            /*
             *  First get all the TRANSFER_PACKETs that we'll need at once.
             */
            for (i = 0; i < numPackets; i++){
                pkt = DequeueFreeTransferPacket(Fdo, TRUE);
                if (pkt){
                    SimplePushSlist(&pktList, (PSINGLE_LIST_ENTRY)&pkt->SlistEntry);
                }
                else {
                    break;
                }
            }
        }


        if ((i == numPackets) &&
            (!driverUsesStartIO)) {
            NTSTATUS pktStat;

            /*
             *  The IoStatus.Information field will be incremented to the
             *  transfer length as the pieces complete.
             */
            Irp->IoStatus.Information = 0;

            /*
             *  Store the number of transfer pieces inside the original IRP.
             *  It will be used to count down the pieces as they complete.
             */
            Irp->Tail.Overlay.DriverContext[0] = LongToPtr(numPackets);

            /*
             *  For the common 1-packet case, we want to allow for an optimization by BlkCache
             *  (and also potentially synchronous storage drivers) which may complete the
             *  downward request synchronously.
             *  In that synchronous completion case, we want to _not_ mark the original irp pending
             *  and thereby save on the top-level APC.
             *  It's critical to coordinate this with the completion routine so that we mark the original irp
             *  pending if-and-only-if we return STATUS_PENDING for it.
             */
            if (numPackets > 1){
                IoMarkIrpPending(Irp);
                status = STATUS_PENDING;
            }
            else {
                status = STATUS_SUCCESS;
            }

            /*
             *  Transmit the pieces of the transfer.
             */
            while (entireXferLen > 0){
                ULONG thisPieceLen = MIN(hwMaxXferLen, entireXferLen);

                /*
                 *  Set up a TRANSFER_PACKET for this piece and send it.
                 */
                slistEntry = SimplePopSlist(&pktList);
                NT_ASSERT(slistEntry);
                pkt = CONTAINING_RECORD(slistEntry, TRANSFER_PACKET, SlistEntry);
                SetupReadWriteTransferPacket(   pkt,
                                            bufPtr,
                                            thisPieceLen,
                                            targetLocation,
                                            Irp);

                //
                // If the IRP needs to be split, then we need to use a partial MDL.
                // This prevents problems if the same MDL is mapped multiple times.
                //
                if (numPackets > 1) {
                    pkt->UsePartialMdl = TRUE;
                }

                /*
                 *  When an IRP is completed, the completion routine checks to see if there
                 *  is a deferred IRP ready to sent down (assuming that there are no non-idle
                 *  requests waiting to be serviced). If such a deferred IRP is available, it
                 *  is sent down using this routine. However, if the lower driver completes
                 *  the request inline, there is a potential for multiple deferred IRPs being
                 *  sent down in the context of the same completion thread, thus exhausting
                 *  the call stack.
                 *  In order to prevent this from happening, we need to ensure that deferred
                 *  IRPs that are dequeued in the context of a request's completion routine
                 *  get posted to a DPC.
                 */
                if (PostToDpc) {

                    pkt->RetryIn100nsUnits = 0;
                    TransferPacketQueueRetryDpc(pkt);
                    status = STATUS_PENDING;

                } else {

                    pktStat = SubmitTransferPacket(pkt);

                    /*
                     *  If any of the packets completes with pending, we MUST return pending.
                     *  Also, if a packet completes with an error, return pending; this is because
                     *  in the completion routine we mark the original irp pending if the packet failed
                     *  (since we may retry, thereby switching threads).
                     */
                    if (pktStat != STATUS_SUCCESS){
                        status = STATUS_PENDING;
                    }
                }

                entireXferLen -= thisPieceLen;
                bufPtr += thisPieceLen;
                targetLocation.QuadPart += thisPieceLen;
            }
            NT_ASSERT(SimpleIsSlistEmpty(&pktList));
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
                NT_ASSERT(slistEntry);
                pkt = CONTAINING_RECORD(slistEntry, TRANSFER_PACKET, SlistEntry);
                EnqueueFreeTransferPacket(Fdo, pkt);
            }

            /*
             *  Get the single TRANSFER_PACKET that we'll be using.
             */
            slistEntry = SimplePopSlist(&pktList);
            NT_ASSERT(slistEntry);
            NT_ASSERT(SimpleIsSlistEmpty(&pktList));
            pkt = CONTAINING_RECORD(slistEntry, TRANSFER_PACKET, SlistEntry);

            if (!driverUsesStartIO) {
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_RW, "Insufficient packets available in ServiceTransferRequest - entering lowMemRetry with pkt=%p.", pkt));
            }

            /*
             *  Set the number of transfer packets (one)
             *  inside the original irp.
             */
            Irp->IoStatus.Information = 0;
            Irp->Tail.Overlay.DriverContext[0] = LongToPtr(1);
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
            status = STATUS_PENDING;
        }
        else {
            /*
             *  We were unable to get ANY TRANSFER_PACKETs.
             *  Defer this client irp until some TRANSFER_PACKETs free up.
             */
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_RW, "No packets available in ServiceTransferRequest - deferring transfer (Irp=%p)...", Irp));

            if (priority == IoPagingPriorityHigh)
            {
                KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);

                if (fdoData->MaxInterleavedNormalIo < ClassMaxInterleavePerCriticalIo)
                {
                    fdoData->MaxInterleavedNormalIo = 0;
                }
                else
                {
                    fdoData->MaxInterleavedNormalIo -= ClassMaxInterleavePerCriticalIo;
                }

                fdoData->NumHighPriorityPagingIo--;

                if (fdoData->NumHighPriorityPagingIo == 0)
                {
                    LARGE_INTEGER period;

                    //
                    // Exiting throttle mode
                    //

                    KeQuerySystemTime(&fdoData->ThrottleStopTime);

                    period.QuadPart = fdoData->ThrottleStopTime.QuadPart - fdoData->ThrottleStartTime.QuadPart;
                    fdoData->LongestThrottlePeriod.QuadPart = max(fdoData->LongestThrottlePeriod.QuadPart, period.QuadPart);
                }

                KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);
            }

            deferClientIrp = TRUE;
        }
    }
    _Analysis_assume_(deferClientIrp);
    if (deferClientIrp)
    {
        IoMarkIrpPending(Irp);
        EnqueueDeferredClientIrp(Fdo, Irp);
        status = STATUS_PENDING;
    }

    NT_ASSERT(status != STATUS_UNSUCCESSFUL);

    return status;
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
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
    ULONG srbFlags;
    ULONG srbFunction;

    NT_ASSERT(fdoExtension->CommonExtension.IsFdo);

    if (srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK) {
        srbFlags = ((PSTORAGE_REQUEST_BLOCK)srb)->SrbFlags;
        srbFunction = ((PSTORAGE_REQUEST_BLOCK)srb)->SrbFunction;
    } else {
        srbFlags = srb->SrbFlags;
        srbFunction = srb->Function;
    }

    #if DBG
        if (srbFunction == SRB_FUNCTION_FLUSH) {
            DBGLOGFLUSHINFO(fdoData, FALSE, FALSE, TRUE);
        }
    #endif

    //
    // Check SRB status for success of completing request.
    //

    if (SRB_STATUS(srb->SrbStatus) != SRB_STATUS_SUCCESS) {
        LONGLONG retryInterval;

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassIoComplete: IRP %p, SRB %p\n", Irp, srb));

        //
        // Release the queue if it is frozen.
        //

        if (srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN) {
            ClassReleaseQueue(Fdo);
        }
        retry = InterpretSenseInfoWithoutHistory(
            Fdo,
            Irp,
            srb,
            irpStack->MajorFunction,
            ((irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL) ?
             irpStack->Parameters.DeviceIoControl.IoControlCode :
             0),
            MAXIMUM_RETRIES -
                ((ULONG)(ULONG_PTR)irpStack->Parameters.Others.Argument4),
            &status,
            &retryInterval);

        //
        // For Persistent Reserve requests, make sure user gets back partial data.
        //

        if (irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL &&
            (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_PERSISTENT_RESERVE_IN ||
             irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_PERSISTENT_RESERVE_OUT) &&
            status == STATUS_DATA_OVERRUN) {

            status = STATUS_SUCCESS;
            retry = FALSE;
        }

        //
        // If the status is verified required and the this request
        // should bypass verify required then retry the request.
        //

        if (TEST_FLAG(irpStack->Flags, SL_OVERRIDE_VERIFY_VOLUME) &&
            status == STATUS_VERIFY_REQUIRED) {

            status = STATUS_IO_DEVICE_ERROR;
            retry = TRUE;
        }

#ifndef __REACTOS__
#pragma warning(suppress:4213) // okay to cast Arg4 as a ulong for this use case
        if (retry && ((ULONG)(ULONG_PTR)irpStack->Parameters.Others.Argument4)--) {
#else
        if (retry && (*(ULONG *)&irpStack->Parameters.Others.Argument4)--) {
#endif


            //
            // Retry request.
            //

            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,  "Retry request %p\n", Irp));

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
        NT_ASSERT(Irp->IoStatus.Information != 0);
        NT_ASSERT(irpStack->Parameters.Read.Length == Irp->IoStatus.Information);
    }

    //
    // remember if the caller wanted to skip calling IoStartNextPacket.
    // for legacy reasons, we cannot call IoStartNextPacket for IoDeviceControl
    // calls.  this setting only affects device objects with StartIo routines.
    //

    callStartNextPacket = !TEST_FLAG(srbFlags, SRB_FLAGS_DONT_START_NEXT_PACKET);
    if (irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL) {
        callStartNextPacket = FALSE;
    }

    //
    // Free MDL if allocated.
    //

    if (TEST_FLAG(srbFlags, SRB_CLASS_FLAGS_FREE_MDL)) {
        SrbClearSrbFlags(srb, SRB_CLASS_FLAGS_FREE_MDL);
        IoFreeMdl(Irp->MdlAddress);
        Irp->MdlAddress = NULL;
    }


    //
    // Free the srb
    //

    if (!TEST_FLAG(srbFlags, SRB_CLASS_FLAGS_PERSISTANT)) {

        if (PORT_ALLOCATED_SENSE(fdoExtension, srb)) {
            FREE_PORT_ALLOCATED_SENSE_BUFFER(fdoExtension, srb);
        }

        if (fdoExtension->CommonExtension.IsSrbLookasideListInitialized){
            ClassFreeOrReuseSrb(fdoExtension, srb);
        }
        else {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, "ClassIoComplete is freeing an SRB (possibly) on behalf of another driver."));
            FREE_POOL(srb);
        }

    } else {

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassIoComplete: Not Freeing srb @ %p because "
                    "SRB_CLASS_FLAGS_PERSISTANT set\n", srb));
        if (PORT_ALLOCATED_SENSE(fdoExtension, srb)) {
            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassIoComplete: Not Freeing sensebuffer @ %p "
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
    // If disk firmware update succeeded, log a system event.
    //

    if (NT_SUCCESS(status) &&
        (irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL) &&
        (irpStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_FIRMWARE_ACTIVATE)) {

        ClasspLogSystemEventWithDeviceNumber(Fdo, IO_WARNING_DISK_FIRMWARE_UPDATED);
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
            IoStartNextPacket(Fdo, TRUE); // Yes, some IO must now be cancellable.
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassSendSrbSynchronous(
    _In_ PDEVICE_OBJECT Fdo,
    _Inout_ PSCSI_REQUEST_BLOCK _Srb,
    _In_reads_bytes_opt_(BufferLength) PVOID BufferAddress,
    _In_ ULONG BufferLength,
    _In_ BOOLEAN WriteToDevice
    )
{

    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;
    IO_STATUS_BLOCK ioStatus = {0};
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    KEVENT event;
    PVOID senseInfoBuffer = NULL;
    ULONG senseInfoBufferLength = SENSE_BUFFER_SIZE_EX;
    ULONG retryCount = MAXIMUM_RETRIES;
    NTSTATUS status;
    BOOLEAN retry;
    PSTORAGE_REQUEST_BLOCK_HEADER Srb = (PSTORAGE_REQUEST_BLOCK_HEADER)_Srb;

    //
    // NOTE: This code is only pagable because we are not freezing
    //       the queue.  Allowing the queue to be frozen from a pagable
    //       routine could leave the queue frozen as we try to page in
    //       the code to unfreeze the queue.  The result would be a nice
    //       case of deadlock.  Therefore, since we are unfreezing the
    //       queue regardless of the result, just set the NO_FREEZE_QUEUE
    //       flag in the SRB.
    //

    NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    NT_ASSERT(fdoExtension->CommonExtension.IsFdo);

    if (Srb->Function != SRB_FUNCTION_STORAGE_REQUEST_BLOCK) {
        //
        // Write length to SRB.
        //

        Srb->Length = sizeof(SCSI_REQUEST_BLOCK);

        //
        // Set SCSI bus address.
        //

        Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    }

    //
    // The Srb->Function should have been set corresponding to SrbType.
    //

    NT_ASSERT( ((fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_SCSI_REQUEST_BLOCK) && (Srb->Function == SRB_FUNCTION_EXECUTE_SCSI)) ||
               ((fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) && (Srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)) );


    //
    // Sense buffer is in aligned nonpaged pool.
    //

#if defined(_ARM_) || defined(_ARM64_)

    //
    // ARM has specific alignment requirements, although this will not have a functional impact on x86 or amd64
    // based platforms. We are taking the conservative approach here.
    //
    senseInfoBufferLength = ALIGN_UP_BY(senseInfoBufferLength,KeGetRecommendedSharedDataAlignment());

#endif

    senseInfoBuffer = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                            senseInfoBufferLength,
                                            '7CcS');

    if (senseInfoBuffer == NULL) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,  "ClassSendSrbSynchronous: Can't allocate request sense "
                       "buffer\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }


    //
    // Enable auto request sense.
    //
    SrbSetSenseInfoBufferLength(Srb, SENSE_BUFFER_SIZE_EX);
    SrbSetSenseInfoBuffer(Srb, senseInfoBuffer);

    SrbSetDataBuffer(Srb, BufferAddress);


    //
    // Start retries here.
    //

retry:

    //
    // use fdoextension's flags by default.
    // do not move out of loop, as the flag may change due to errors
    // sending this command.
    //

    SrbAssignSrbFlags(Srb, fdoExtension->SrbFlags);

    if (BufferAddress != NULL) {
        if (WriteToDevice) {
            SrbSetSrbFlags(Srb, SRB_FLAGS_DATA_OUT);
        } else {
            SrbSetSrbFlags(Srb, SRB_FLAGS_DATA_IN);
        }
    }


    //
    // Initialize the QueueAction field.
    //

    SrbSetRequestAttribute(Srb, SRB_SIMPLE_TAG_REQUEST);

    //
    // Disable synchronous transfer for these requests.
    //
    SrbSetSrbFlags(Srb, SRB_FLAGS_DISABLE_SYNCH_TRANSFER | SRB_FLAGS_NO_QUEUE_FREEZE);

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

    if (irp == NULL) {
        FREE_POOL(senseInfoBuffer);
        SrbSetSenseInfoBuffer(Srb, NULL);
        SrbSetSenseInfoBufferLength(Srb, 0);
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,  "ClassSendSrbSynchronous: Can't allocate Irp\n"));
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
    irpStack->Parameters.Scsi.Srb = (PSCSI_REQUEST_BLOCK)Srb;

    IoSetCompletionRoutine(irp,
                           ClasspSendSynchronousCompletion,
                           Srb,
                           TRUE,
                           TRUE,
                           TRUE);

    irp->UserIosb = &ioStatus;
    irp->UserEvent = &event;

    if (BufferAddress) {
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
            FREE_POOL(senseInfoBuffer);
            SrbSetSenseInfoBuffer(Srb, NULL);
            SrbSetSenseInfoBufferLength(Srb, 0);
            IoFreeIrp( irp );
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,  "ClassSendSrbSynchronous: Can't allocate MDL\n"));
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

#ifdef _MSC_VER
        #pragma warning(suppress: 6320) // We want to handle any exception that MmProbeAndLockPages might throw
#endif
        } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
            status = _SEH2_GetExceptionCode();

            FREE_POOL(senseInfoBuffer);
            SrbSetSenseInfoBuffer(Srb, NULL);
            SrbSetSenseInfoBufferLength(Srb, 0);
            IoFreeMdl(irp->MdlAddress);
            IoFreeIrp(irp);

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,  "ClassSendSrbSynchronous: Exception %lx "
                           "locking buffer\n", status));
            _SEH2_YIELD(return status);
        } _SEH2_END;
    }

    //
    // Set the transfer length.
    //

    SrbSetDataTransferLength(Srb, BufferLength);

    //
    // Zero out status.
    //

    SrbSetScsiStatus(Srb, 0);
    Srb->SrbStatus = 0;
    SrbSetNextSrb(Srb, NULL);

    //
    // Set up IRP Address.
    //

    SrbSetOriginalRequest(Srb, irp);

    //
    // Call the port driver with the request and wait for it to complete.
    //

    status = IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp);

    if (status == STATUS_PENDING) {
        (VOID)KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

//    NT_ASSERT(SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_PENDING);
    NT_ASSERT(status != STATUS_PENDING);
    NT_ASSERT(!(Srb->SrbStatus & SRB_STATUS_QUEUE_FROZEN));

    //
    // Clear the IRP address in SRB as IRP has been freed at this time
    // and don't want to leave any references that may be accessed.
    //

    SrbSetOriginalRequest(Srb, NULL);

    //
    // Check that request completed without error.
    //

    if (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS) {

        LONGLONG retryInterval;

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL, "ClassSendSrbSynchronous - srb %ph failed (op=%s srbstat=%s(%xh), irpstat=%xh, sense=%s/%s/%s)", Srb,
                    DBGGETSCSIOPSTR(Srb),
                    DBGGETSRBSTATUSSTR(Srb), (ULONG)Srb->SrbStatus, status,
                    DBGGETSENSECODESTR(Srb),
                    DBGGETADSENSECODESTR(Srb),
                    DBGGETADSENSEQUALIFIERSTR(Srb)));

        //
        // assert that the queue is not frozen
        //

        NT_ASSERT(!TEST_FLAG(Srb->SrbStatus, SRB_STATUS_QUEUE_FROZEN));

        //
        // Update status and determine if request should be retried.
        //

        retry = InterpretSenseInfoWithoutHistory(Fdo,
                                                 NULL, // no valid irp exists
                                                 (PSCSI_REQUEST_BLOCK)Srb,
                                                 IRP_MJ_SCSI,
                                                 0,
                                                 MAXIMUM_RETRIES  - retryCount,
                                                 &status,
                                                 &retryInterval);

        if (retry) {

            BOOLEAN validSense = FALSE;
            UCHAR additionalSenseCode = 0;

            if (status == STATUS_DEVICE_NOT_READY) {

                validSense = ScsiGetSenseKeyAndCodes(senseInfoBuffer,
                                                     SrbGetSenseInfoBufferLength(Srb),
                                                     SCSI_SENSE_OPTIONS_FIXED_FORMAT_IF_UNKNOWN_FORMAT_INDICATED,
                                                     NULL,
                                                     &additionalSenseCode,
                                                     NULL);
            }

            if ((validSense && additionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY) ||
                (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_SELECTION_TIMEOUT)) {

                LARGE_INTEGER delay;

                //
                // Delay for at least 2 seconds.
                //

                if (retryInterval < 2*1000*1000*10) {
                    retryInterval = 2*1000*1000*10;
                }

                delay.QuadPart = -retryInterval;

                //
                // Stall for a while to let the device become ready
                //

                KeDelayExecutionThread(KernelMode, FALSE, &delay);

            }


            //
            // If retries are not exhausted then retry this operation.
            //

            if (retryCount--) {

                if (PORT_ALLOCATED_SENSE_EX(fdoExtension, Srb)) {
                    FREE_PORT_ALLOCATED_SENSE_BUFFER_EX(fdoExtension, Srb);
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

    if (PORT_ALLOCATED_SENSE_EX(fdoExtension, Srb)) {
        FREE_PORT_ALLOCATED_SENSE_BUFFER_EX(fdoExtension, Srb);
    }

    FREE_POOL(senseInfoBuffer);
    SrbSetSenseInfoBuffer(Srb, NULL);
    SrbSetSenseInfoBufferLength(Srb, 0);

    return status;
}


/*++////////////////////////////////////////////////////////////////////////////

ClassInterpretSenseInfo()

Routine Description:

    This routine interprets the data returned from the SCSI
    request sense. It determines the status to return in the
    IRP and whether this request can be retried.

Arguments:

    Fdo - Supplies the device object associated with this request.

    Srb - Supplies the scsi request block which failed.

    MajorFunctionCode - Supplies the function code to be used for logging.

    IoDeviceCode - Supplies the device code to be used for logging.

    RetryCount - Number of times that the request has been retried.

    Status - Returns the status for the request.

    RetryInterval - Number of seconds before the request should be retried.
                    Zero indicates the request should be immediately retried.

Return Value:

    BOOLEAN TRUE: Drivers should retry this request.
            FALSE: Drivers should not retry this request.

--*/
BOOLEAN
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassInterpretSenseInfo(
    _In_ PDEVICE_OBJECT Fdo,
    _In_ PSCSI_REQUEST_BLOCK _Srb,
    _In_ UCHAR MajorFunctionCode,
    _In_ ULONG IoDeviceCode,
    _In_ ULONG RetryCount,
    _Out_ NTSTATUS *Status,
    _Out_opt_ _Deref_out_range_(0,100) ULONG *RetryInterval
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;
    PSTORAGE_REQUEST_BLOCK_HEADER Srb = (PSTORAGE_REQUEST_BLOCK_HEADER)_Srb;
    PVOID senseBuffer = SrbGetSenseInfoBuffer(Srb);
    BOOLEAN retry = TRUE;
    BOOLEAN logError = FALSE;
    BOOLEAN unhandledError = FALSE;
    BOOLEAN incrementErrorCount = FALSE;

    //
    // NOTE: This flag must be used only for read/write requests that
    // fail with a unexpected retryable error.
    //
    BOOLEAN logRetryableError = TRUE;

    //
    // Indicates if we should log this error in our internal log.
    //
    BOOLEAN logErrorInternal = TRUE;

    ULONGLONG badSector = 0;
    ULONG uniqueId = 0;

    NTSTATUS logStatus;

    ULONGLONG readSector;
    ULONG index;

    ULONG retryInterval = 0;
    KIRQL oldIrql;
    PCDB cdb = SrbGetCdb(Srb);
    UCHAR cdbOpcode = 0;
    ULONG cdbLength = SrbGetCdbLength(Srb);

#if DBG
    BOOLEAN isReservationConflict = FALSE;
#endif

    if (cdb) {
        cdbOpcode = cdb->CDB6GENERIC.OperationCode;
    }

    *Status = STATUS_IO_DEVICE_ERROR;
    logStatus = -1;

    if (TEST_FLAG(SrbGetSrbFlags(Srb), SRB_CLASS_FLAGS_PAGING)) {

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

    NT_ASSERT(fdoExtension->CommonExtension.IsFdo);


    //
    // must handle the SRB_STATUS_INTERNAL_ERROR case first,
    // as it has  all the flags set.
    //

    if (SRB_STATUS(Srb->SrbStatus) == SRB_STATUS_INTERNAL_ERROR) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                    "ClassInterpretSenseInfo: Internal Error code is %x\n",
                    SrbGetSystemStatus(Srb)));

        retry = FALSE;
        *Status = SrbGetSystemStatus(Srb);

    } else if (SrbGetScsiStatus(Srb) == SCSISTAT_RESERVATION_CONFLICT) {

        //
        // Need to reserve STATUS_DEVICE_BUSY to convey reservation conflict
        // for read/write requests as there are upper level components that
        // have built-in assumptions that STATUS_DEVICE_BUSY implies reservation
        // conflict.
        //
        *Status = STATUS_DEVICE_BUSY;
        retry = FALSE;
        logError = FALSE;
#if DBG
        isReservationConflict = TRUE;
#endif

    } else {

        BOOLEAN validSense = FALSE;

        if ((Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID) && senseBuffer)  {

            UCHAR errorCode = 0;
            UCHAR senseKey = 0;
            UCHAR addlSenseCode = 0;
            UCHAR addlSenseCodeQual = 0;
            BOOLEAN isIncorrectLengthValid = FALSE;
            BOOLEAN incorrectLength = FALSE;
            BOOLEAN isInformationValid = FALSE;
            ULONGLONG information = 0;


            validSense = ScsiGetSenseKeyAndCodes(senseBuffer,
                                                 SrbGetSenseInfoBufferLength(Srb),
                                                 SCSI_SENSE_OPTIONS_NONE,
                                                 &senseKey,
                                                 &addlSenseCode,
                                                 &addlSenseCodeQual);

            if (!validSense && !IsSenseDataFormatValueValid(senseBuffer)) {

                NT_ASSERT(FALSE);

                validSense = ScsiGetFixedSenseKeyAndCodes(senseBuffer,
                                                          SrbGetSenseInfoBufferLength(Srb),
                                                          &senseKey,
                                                          &addlSenseCode,
                                                          &addlSenseCodeQual);
            }

            if (!validSense) {
                goto __ClassInterpretSenseInfo_ProcessingInvalidSenseBuffer;
            }

            errorCode = ScsiGetSenseErrorCode(senseBuffer);

            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: Error code is %x\n", errorCode));
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: Sense key is %x\n", senseKey));
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: Additional sense code is %x\n", addlSenseCode));
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: Additional sense code qualifier is %x\n", addlSenseCodeQual));

            if (IsDescriptorSenseDataFormat(senseBuffer)) {

                //
                // Sense data in Descriptor format
                //

                PVOID startBuffer = NULL;
                UCHAR startBufferLength = 0;


                if (ScsiGetSenseDescriptor(senseBuffer,
                                           SrbGetSenseInfoBufferLength(Srb),
                                           &startBuffer,
                                           &startBufferLength)) {
                    UCHAR outType;
                    PVOID outBuffer = NULL;
                    UCHAR outBufferLength = 0;
                    BOOLEAN foundBlockCommandType = FALSE;
                    BOOLEAN foundInformationType = FALSE;
                    UCHAR descriptorLength = 0;

                    UCHAR typeList[2] = {SCSI_SENSE_DESCRIPTOR_TYPE_INFORMATION,
                                         SCSI_SENSE_DESCRIPTOR_TYPE_BLOCK_COMMAND};

                    while ((!foundBlockCommandType || !foundInformationType) &&
                           ScsiGetNextSenseDescriptorByType(startBuffer,
                                                            startBufferLength,
                                                            typeList,
                                                            ARRAYSIZE(typeList),
                                                            &outType,
                                                            &outBuffer,
                                                            &outBufferLength)) {

                        descriptorLength = ScsiGetSenseDescriptorLength(outBuffer);

                        if (outBufferLength < descriptorLength) {

                            // Descriptor data is truncated.
                            // Complete searching descriptors. Exit the loop now.
                            break;
                        }

                        if (outType == SCSI_SENSE_DESCRIPTOR_TYPE_BLOCK_COMMAND) {

                            //
                            // Block Command type
                            //

                            if (!foundBlockCommandType) {

                                foundBlockCommandType = TRUE;

                                if (ScsiValidateBlockCommandSenseDescriptor(outBuffer, outBufferLength)) {
                                    incorrectLength = ((PSCSI_SENSE_DESCRIPTOR_BLOCK_COMMAND)outBuffer)->IncorrectLength;
                                    isIncorrectLengthValid = TRUE;
                                }
                            } else {

                                //
                                // A Block Command descriptor is already found earlier.
                                //
                                // T10 SPC specification only allows one descriptor for Block Command Descriptor type.
                                // Assert here to catch devices that violate this rule. Ignore this descriptor.
                                //
                                NT_ASSERT(FALSE);
                            }

                        } else if (outType == SCSI_SENSE_DESCRIPTOR_TYPE_INFORMATION) {

                            //
                            // Information type
                            //

                            if (!foundInformationType) {

                                foundInformationType = TRUE;

                                if (ScsiValidateInformationSenseDescriptor(outBuffer, outBufferLength)) {
                                    REVERSE_BYTES_QUAD(&information, &(((PSCSI_SENSE_DESCRIPTOR_INFORMATION)outBuffer)->Information));
                                    isInformationValid = TRUE;
                                }
                            } else {

                                //
                                // A Information descriptor is already found earlier.
                                //
                                // T10 SPC specification only allows one descriptor for Information Descriptor type.
                                // Assert here to catch devices that violate this rule. Ignore this descriptor.
                                //
                                NT_ASSERT(FALSE);
                            }

                        } else {

                            //
                            // ScsiGetNextDescriptorByType should only return a type that is specified by us.
                            //
                            NT_ASSERT(FALSE);
                            break;
                        }

                        //
                        // Advance to start address of next descriptor
                        //
                        startBuffer = (PUCHAR)outBuffer + descriptorLength;
                        startBufferLength = outBufferLength - descriptorLength;
                    }
                }
            } else {

                //
                // Sense data in Fixed format
                //

                incorrectLength = ((PFIXED_SENSE_DATA)(senseBuffer))->IncorrectLength;
                REVERSE_BYTES(&information, &(((PFIXED_SENSE_DATA)senseBuffer)->Information));
                isInformationValid = TRUE;
                isIncorrectLengthValid = TRUE;
            }


            switch (senseKey) {

                case SCSI_SENSE_NO_SENSE: {

                    //
                    // Check other indicators.
                    //

                    if (isIncorrectLengthValid && incorrectLength) {

                        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                    "Incorrect length detected.\n"));
                        *Status = STATUS_INVALID_BLOCK_LENGTH ;
                        retry   = FALSE;

                    } else {

                        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                    "No specific sense key\n"));
                        *Status = STATUS_IO_DEVICE_ERROR;
                        retry   = TRUE;
                    }

                    break;
                } // end SCSI_SENSE_NO_SENSE

                case SCSI_SENSE_RECOVERED_ERROR: {

                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                "Recovered error\n"));
                    *Status = STATUS_SUCCESS;
                    retry = FALSE;
                    logError = TRUE;
                    uniqueId = 258;

                    switch(addlSenseCode) {
                        case SCSI_ADSENSE_TRACK_ERROR:
                        case SCSI_ADSENSE_SEEK_ERROR: {
                            logStatus = IO_ERR_SEEK_ERROR;
                            break;
                        }

                        case SCSI_ADSENSE_REC_DATA_NOECC:
                        case SCSI_ADSENSE_REC_DATA_ECC: {
                            logStatus = IO_RECOVERED_VIA_ECC;
                            break;
                        }

                        case SCSI_ADSENSE_FAILURE_PREDICTION_THRESHOLD_EXCEEDED: {

                            UCHAR wmiEventData[sizeof(ULONG)+sizeof(UCHAR)] = {0};

                            *((PULONG)wmiEventData) = sizeof(UCHAR);
                            wmiEventData[sizeof(ULONG)] = addlSenseCodeQual;

                            //
                            // Don't log another eventlog if we have already logged once
                            // NOTE: this should have been interlocked, but the structure
                            //       was publicly defined to use a BOOLEAN (char).  Since
                            //       media only reports these errors once per X minutes,
                            //       the potential race condition is nearly non-existant.
                            //       the worst case is duplicate log entries, so ignore.
                            //

                            logError = FALSE;
                            if (fdoExtension->FailurePredicted == 0) {
                                logError = TRUE;
                            }
                            fdoExtension->FailureReason = addlSenseCodeQual;
                            logStatus = IO_WRN_FAILURE_PREDICTED;

                            ClassNotifyFailurePredicted(fdoExtension,
                                                        (PUCHAR)wmiEventData,
                                                        sizeof(wmiEventData),
                                                        FALSE,   // do not log error
                                                        4,          // unique error value
                                                        SrbGetPathId(Srb),
                                                        SrbGetTargetId(Srb),
                                                        SrbGetLun(Srb));

                            fdoExtension->FailurePredicted = TRUE;
                            break;
                        }

                        default: {
                            logStatus = IO_ERR_CONTROLLER_ERROR;
                            break;
                        }

                    } // end switch(addlSenseCode)

                    if (isIncorrectLengthValid && incorrectLength) {

                        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                    "Incorrect length detected.\n"));
                        *Status = STATUS_INVALID_BLOCK_LENGTH ;
                    }


                    break;
                } // end SCSI_SENSE_RECOVERED_ERROR

                case SCSI_SENSE_NOT_READY: {

                    TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                "Device not ready\n"));
                    *Status = STATUS_DEVICE_NOT_READY;

                    switch (addlSenseCode) {

                        case SCSI_ADSENSE_LUN_NOT_READY: {

                            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                        "Lun not ready\n"));

                            retryInterval = NOT_READY_RETRY_INTERVAL;

                            switch (addlSenseCodeQual) {

                                case SCSI_SENSEQ_BECOMING_READY: {
                                    DEVICE_EVENT_BECOMING_READY notReady = {0};

                                    logRetryableError = FALSE;
                                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                                "In process of becoming ready\n"));

                                    notReady.Version = 1;
                                    notReady.Reason = 1;
                                    notReady.Estimated100msToReady = retryInterval * 10;
                                    ClassSendNotification(fdoExtension,
                                                           &GUID_IO_DEVICE_BECOMING_READY,
                                                           sizeof(DEVICE_EVENT_BECOMING_READY),
                                                           &notReady);
                                    break;
                                }

                                case SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED: {
                                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                                "Manual intervention required\n"));
                                    *Status = STATUS_NO_MEDIA_IN_DEVICE;
                                    retry = FALSE;
                                    break;
                                }

                                case SCSI_SENSEQ_FORMAT_IN_PROGRESS: {
                                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                                "Format in progress\n"));
                                    retry = FALSE;
                                    break;
                                }

                                case SCSI_SENSEQ_OPERATION_IN_PROGRESS: {
                                    DEVICE_EVENT_BECOMING_READY notReady = {0};

                                    logRetryableError = FALSE;
                                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                                "Operation In Progress\n"));

                                    notReady.Version = 1;
                                    notReady.Reason = 2;
                                    notReady.Estimated100msToReady = retryInterval * 10;
                                    ClassSendNotification(fdoExtension,
                                                           &GUID_IO_DEVICE_BECOMING_READY,
                                                           sizeof(DEVICE_EVENT_BECOMING_READY),
                                                           &notReady);

                                    break;
                                }

                                case SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS: {
                                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                                "Long write in progress\n"));
                                    //
                                    // This has been seen as a transcient failure on some cdrom
                                    // drives. The cdrom class driver is going to override this
                                    // setting but has no way of dropping the retry interval
                                    //
                                    retry = FALSE;
                                    retryInterval = 1;
                                    break;
                                }

                                case SCSI_SENSEQ_SPACE_ALLOC_IN_PROGRESS: {
                                    logRetryableError = FALSE;
                                    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                                "The device (%p) is busy allocating space.\n",
                                                Fdo));

                                    //
                                    // This indicates that a thinly-provisioned device has hit
                                    // a temporary resource exhaustion and is busy allocating
                                    // more space.  We need to retry the request as the device
                                    // will eventually be able to service it.
                                    //
                                    *Status = STATUS_RETRY;
                                    retry = TRUE;

                                    break;
                                }

                                case SCSI_SENSEQ_CAUSE_NOT_REPORTABLE: {

                                    if (!TEST_FLAG(fdoExtension->ScanForSpecialFlags,
                                                   CLASS_SPECIAL_CAUSE_NOT_REPORTABLE_HACK)) {

                                        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
                                                    "ClassInterpretSenseInfo: "
                                                    "not ready, cause unknown\n"));
                                        /*
                                        Many non-WHQL certified drives (mostly CD-RW) return
                                        this when they have no media instead of the obvious
                                        choice of:

                                        SCSI_SENSE_NOT_READY/SCSI_ADSENSE_NO_MEDIA_IN_DEVICE

                                        These drives should not pass WHQL certification due
                                        to this discrepency.

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
                                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                                "Initializing command required\n"));
                                    retryInterval = 0; // go back to default
                                    logRetryableError = FALSE;

                                    //
                                    // This sense code/additional sense code
                                    // combination may indicate that the device
                                    // needs to be started.  Send an start unit if this
                                    // is a disk device.
                                    //
                                    if (TEST_FLAG(fdoExtension->DeviceFlags, DEV_SAFE_START_UNIT) &&
                                        !TEST_FLAG(SrbGetSrbFlags(Srb), SRB_CLASS_FLAGS_LOW_PRIORITY)){

                                        ClassSendStartUnit(Fdo);
                                    }
                                    break;
                                }

                            } // end switch (addlSenseCodeQual)
                            break;
                        }

                        case SCSI_ADSENSE_NO_MEDIA_IN_DEVICE: {
                            TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                        "No Media in device.\n"));
                            *Status = STATUS_NO_MEDIA_IN_DEVICE;
                            retry = FALSE;

                            //
                            // signal MCN that there isn't any media in the device
                            //
                            if (!TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA)) {
                                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                            "No Media in a non-removable device %p\n",
                                            Fdo));
                            }

                            if (addlSenseCodeQual == 0xCC){
                                /*
                                 *  The IMAPI filter returns this ASCQ value when it is burning CD-R media.
                                 *  We want to indicate that the media is not present to most applications;
                                 *  but RSM has to know that the media is still in the drive (i.e. the drive is not free).
                                 */
                                ClassSetMediaChangeState(fdoExtension, MediaUnavailable, FALSE);
                            }
                            else {
                                ClassSetMediaChangeState(fdoExtension, MediaNotPresent, FALSE);
                            }

                            break;
                        }
                    } // end switch (addlSenseCode)

                    break;
                } // end SCSI_SENSE_NOT_READY

                case SCSI_SENSE_MEDIUM_ERROR: {
                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                "Medium Error (bad block)\n"));
                    *Status = STATUS_DEVICE_DATA_ERROR;

                    retry = FALSE;
                    logError = TRUE;
                    uniqueId = 256;
                    logStatus = IO_ERR_BAD_BLOCK;

                    //
                    // Check if this error is due to unknown format
                    //
                    if (addlSenseCode == SCSI_ADSENSE_INVALID_MEDIA) {

                        switch (addlSenseCodeQual) {

                            case SCSI_SENSEQ_UNKNOWN_FORMAT: {

                                *Status = STATUS_UNRECOGNIZED_MEDIA;

                                //
                                // Log error only if this is a paging request
                                //
                                if (!TEST_FLAG(SrbGetSrbFlags(Srb), SRB_CLASS_FLAGS_PAGING)) {
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
                        } // end switch addlSenseCodeQual

                    } // end SCSI_ADSENSE_INVALID_MEDIA

                    break;

                } // end SCSI_SENSE_MEDIUM_ERROR

                case SCSI_SENSE_HARDWARE_ERROR: {
                    BOOLEAN logHardwareError = TRUE;

                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                "Hardware error\n"));

                    if (fdoData->LegacyErrorHandling == FALSE) {
                        //
                        // Hardware errors indicate something has seriously gone
                        // wrong with the device and retries are very unlikely to
                        // succeed so fail this request back immediately.
                        //
                        retry = FALSE;
                        *Status = STATUS_DEVICE_HARDWARE_ERROR;
                        logError = FALSE;

                    } else {
                        //
                        // Revert to legacy behavior.  That is, retry everything by default.
                        //
                        retry = TRUE;
                        *Status = STATUS_IO_DEVICE_ERROR;
                        logError = TRUE;
                        uniqueId = 257;
                        logStatus = IO_ERR_CONTROLLER_ERROR;
                        logHardwareError = FALSE;

                        //
                        // This indicates the possibility of a dropped FC packet.
                        //
                        if ((addlSenseCode == SCSI_ADSENSE_LOGICAL_UNIT_ERROR && addlSenseCodeQual == SCSI_SENSEQ_TIMEOUT_ON_LOGICAL_UNIT) ||
                            (addlSenseCode == SCSI_ADSENSE_DATA_TRANSFER_ERROR && addlSenseCodeQual == SCSI_SENSEQ_INITIATOR_RESPONSE_TIMEOUT)) {
                            //
                            // Fail requests that report this error back to the application.
                            //
                            retry = FALSE;

                            //
                            // Log a more descriptive error and avoid a second
                            // error message (IO_ERR_CONTROLLER_ERROR) being logged.
                            //
                            logHardwareError = TRUE;
                            logError = FALSE;
                        }
                    }

                    //
                    // If CRC error was returned, retry after a slight delay.
                    //
                    if (addlSenseCode == SCSI_ADSENSE_LUN_COMMUNICATION &&
                        addlSenseCodeQual == SCSI_SESNEQ_COMM_CRC_ERROR) {
                        retry = TRUE;
                        retryInterval = 1;
                        logHardwareError = FALSE;
                        logError = FALSE;
                    }

                    //
                    // Hardware errors warrant a more descriptive error.
                    // Specifically, we need to ensure this disk is easily
                    // identifiable.
                    //
                    if (logHardwareError) {
                        UCHAR senseInfoBufferLength = SrbGetSenseInfoBufferLength(Srb);
                        UCHAR senseBufferSize = 0;

                        if (ScsiGetTotalSenseByteCountIndicated(senseBuffer,
                                                                senseInfoBufferLength,
                                                                &senseBufferSize)) {

                            senseBufferSize = min(senseBufferSize, senseInfoBufferLength);

                        } else {
                            //
                            // it's smaller than required to read the total number of
                            // valid bytes, so just use the SenseInfoBufferLength field.
                            //
                            senseBufferSize = senseInfoBufferLength;
                        }

                        ClasspQueueLogIOEventWithContextWorker(Fdo,
                                                               senseBufferSize,
                                                               senseBuffer,
                                                               SRB_STATUS(Srb->SrbStatus),
                                                               SrbGetScsiStatus(Srb),
                                                               (ULONG)IO_ERROR_IO_HARDWARE_ERROR,
                                                               cdbLength,
                                                               cdb,
                                                               NULL);
                    }

                    break;
                } // end SCSI_SENSE_HARDWARE_ERROR

                case SCSI_SENSE_ILLEGAL_REQUEST: {

                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                "Illegal SCSI request\n"));
                    *Status = STATUS_INVALID_DEVICE_REQUEST;
                    retry = FALSE;

                    switch (addlSenseCode) {

                        case SCSI_ADSENSE_NO_SENSE: {

                            switch (addlSenseCodeQual) {

                                //
                                // 1. Duplicate List Identifier
                                //
                                case SCSI_SENSEQ_OPERATION_IS_IN_PROGRESS: {

                                    //
                                    // XCOPY, READ BUFFER and CHANGE ALIASES return this sense combination under
                                    // certain conditions. Since these commands aren't sent down natively by the
                                    // Windows OS, return the default error for them and only handle this sense
                                    // combination for offload data transfer commands.
                                    //
                                    if (ClasspIsOffloadDataTransferCommand(cdb)) {

                                        TracePrint((TRACE_LEVEL_ERROR,
                                                    TRACE_FLAG_IOCTL,
                                                    "ClassInterpretSenseInfo (%p): Duplicate List Identifier (command %x, parameter field offset 0x%016llx)\n",
                                                    Fdo,
                                                    cdbOpcode,
                                                    information));

                                        NT_ASSERTMSG("Duplicate list identifier specified", FALSE);

                                        //
                                        // The host should ensure that it uses unique list id for each TokenOperation request.
                                        //
                                        *Status = STATUS_OPERATION_IN_PROGRESS;
                                    }
                                    break;
                                }
                            }
                            break;
                        }

                        case SCSI_ADSENSE_LUN_COMMUNICATION: {

                            switch (addlSenseCodeQual) {

                                //
                                // 1. Source/Destination pairing can't communicate with each other or the copy manager.
                                //
                                case SCSI_SENSEQ_UNREACHABLE_TARGET: {

                                    TracePrint((TRACE_LEVEL_ERROR,
                                                TRACE_FLAG_IOCTL,
                                                "ClassInterpretSenseInfo (%p): Source-Destination LUNs can't communicate (command %x)\n",
                                                Fdo,
                                                cdbOpcode));

                                    *Status = STATUS_DEVICE_UNREACHABLE;
                                    break;
                                }
                            }
                            break;
                        }

                        case SCSI_ADSENSE_COPY_TARGET_DEVICE_ERROR: {

                            switch (addlSenseCodeQual) {

                                //
                                // 1. Sum of logical block fields in all block device range descriptors is greater than number
                                //    of logical blocks in the ROD minus block offset into ROD
                                //
                                case SCSI_SENSEQ_DATA_UNDERRUN: {

                                    TracePrint((TRACE_LEVEL_ERROR,
                                                TRACE_FLAG_IOCTL,
                                                "ClassInterpretSenseInfo (%p): Host specified a transfer length greater than what is represented by the token (considering the offset) [command %x]\n",
                                                Fdo,
                                                cdbOpcode));

                                    NT_ASSERTMSG("Host specified blocks to write beyond what is represented by the token", FALSE);

                                    *Status = STATUS_DATA_OVERRUN;
                                    break;
                                }
                            }
                            break;
                        }

                        //
                        // 1. Parameter data truncation (e.g. last descriptor was not fully specified)
                        //
                        case SCSI_ADSENSE_PARAMETER_LIST_LENGTH: {

                            TracePrint((TRACE_LEVEL_ERROR,
                                        TRACE_FLAG_IOCTL,
                                        "ClassInterpretSenseInfo (%p): Target truncated the block device range descriptors in the parameter list (command %x)\n",
                                        Fdo,
                                        cdbOpcode));

                            NT_ASSERTMSG("Parameter data truncation", FALSE);

                            *Status = STATUS_DATA_OVERRUN;
                            break;
                        }

                        case SCSI_ADSENSE_ILLEGAL_COMMAND: {
                            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                        "Illegal command\n"));
                            break;
                        }

                        case SCSI_ADSENSE_ILLEGAL_BLOCK: {

                            LARGE_INTEGER logicalBlockAddr;
                            LARGE_INTEGER lastLBA;
                            ULONG numTransferBlocks = 0;

                            logicalBlockAddr.QuadPart = 0;

                            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: Illegal block address\n"));

                            *Status = STATUS_NONEXISTENT_SECTOR;

                            if (Fdo->DeviceType == FILE_DEVICE_DISK) {

                                if (IS_SCSIOP_READWRITE(cdbOpcode) && cdb) {

                                    if (TEST_FLAG(fdoExtension->DeviceFlags, DEV_USE_16BYTE_CDB)) {
                                        REVERSE_BYTES_QUAD(&logicalBlockAddr, &cdb->CDB16.LogicalBlock);
                                        REVERSE_BYTES(&numTransferBlocks, &cdb->CDB16.TransferLength);
                                    } else {
                                        REVERSE_BYTES(&logicalBlockAddr.LowPart, &cdb->CDB10.LogicalBlockByte0);
                                        REVERSE_BYTES_SHORT((PUSHORT)&numTransferBlocks, &cdb->CDB10.TransferBlocksMsb);
                                    }

                                    REVERSE_BYTES_QUAD(&lastLBA, &fdoData->LastKnownDriveCapacityData.LogicalBlockAddress);

                                    if ((logicalBlockAddr.QuadPart > lastLBA.QuadPart) ||
                                        ((logicalBlockAddr.QuadPart + numTransferBlocks - 1) > lastLBA.QuadPart)) {

                                        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                                    "Request beyond boundary. Last LBA: 0x%I64X Read LBA: 0x%I64X Length: 0x%X\n",
                                                    (__int64) lastLBA.QuadPart, (__int64) logicalBlockAddr.QuadPart, numTransferBlocks));
                                    } else {
                                        //
                                        // Should only retry these if the request was
                                        // truly within our expected size.
                                        //
                                        // Fujitsu IDE drives have been observed to
                                        // return this error transiently for a legal LBA;
                                        // manual retry in the debugger then works, so
                                        // there is a good chance that a programmed retry
                                        // will also work.
                                        //

                                        retry = TRUE;
                                        retryInterval = 5;
                                    }
                                } else if (ClasspIsOffloadDataTransferCommand(cdb)) {

                                    //
                                    // 1. Number of logical blocks of block device range descriptor exceeds capacity of the medium
                                    //
                                    TracePrint((TRACE_LEVEL_ERROR,
                                                TRACE_FLAG_IOCTL,
                                                "ClassInterpretSenseInfo (%p): LBA out of range (command %x, parameter field offset 0x%016llx)\n",
                                                Fdo,
                                                cdbOpcode,
                                                information));

                                    NT_ASSERTMSG("Number of blocks specified exceeds LUN capacity", FALSE);
                                }
                            }
                            break;
                        }

                        //
                        //  1. Generic error - cause not reportable
                        //  2. Insufficient resources to create ROD
                        //  3. Insufficient resources to create Token
                        //  4. Max number of tokens exceeded
                        //  5. Remote Token creation not supported
                        //  6. Token expired
                        //  7. Token unknown
                        //  8. Unsupported Token type
                        //  9. Token corrupt
                        // 10. Token revoked
                        // 11. Token cancelled
                        // 12. Remote Token usage not supported
                        //
                        case SCSI_ADSENSE_INVALID_TOKEN: {

                            TracePrint((TRACE_LEVEL_ERROR,
                                        TRACE_FLAG_IOCTL,
                                        "ClassInterpretSenseInfo (%p): Invalid/Expired/Modified token specified (command %x, parameter field offset 0x%016llx)\n",
                                        Fdo,
                                        cdbOpcode,
                                        information));

                            *Status = STATUS_INVALID_TOKEN;
                            break;
                        }

                        case SCSI_ADSENSE_INVALID_CDB: {
                            if (ClasspIsOffloadDataTransferCommand(cdb)) {

                                //
                                // 1. Mismatched I_T nexus and list identifier
                                //
                                TracePrint((TRACE_LEVEL_ERROR,
                                            TRACE_FLAG_IOCTL,
                                            "ClassInterpretSenseInfo (%p): Incorrect I_T nexus likely used (command %x)\n",
                                            Fdo,
                                            cdbOpcode));

                                //
                                // The host should ensure that it sends TokenOperation and ReceiveTokenInformation for the same
                                // list Id using the same I_T nexus.
                                //
                                *Status = STATUS_INVALID_INITIATOR_TARGET_PATH;

                            } else {

                                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
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
                            }
                            break;
                        }

                        case SCSI_ADSENSE_INVALID_LUN: {
                            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                        "Invalid LUN\n"));
                            *Status = STATUS_NO_SUCH_DEVICE;
                            break;
                        }

                        case SCSI_ADSENSE_INVALID_FIELD_PARAMETER_LIST: {

                            switch (addlSenseCodeQual) {

                                //
                                // 1. Alignment violation (e.g. copy manager is unable to copy because destination offset is NOT aligned to LUN's granularity/alignment)
                                //
                                case SCSI_SENSEQ_INVALID_RELEASE_OF_PERSISTENT_RESERVATION: {

                                    TracePrint((TRACE_LEVEL_ERROR,
                                                TRACE_FLAG_IOCTL,
                                                "ClassInterpretSenseInfo (%p): Alignment violation for command %x.\n",
                                                Fdo,
                                                cdbOpcode));

                                    NT_ASSERTMSG("Specified offset is not aligned to LUN's granularity", FALSE);

                                    *Status = STATUS_INVALID_OFFSET_ALIGNMENT;
                                    break;
                                }

                                //
                                // 1. Number of block device range descriptors is greater than maximum range descriptors
                                //
                                case SCSI_SENSEQ_TOO_MANY_SEGMENT_DESCRIPTORS: {

                                    TracePrint((TRACE_LEVEL_ERROR,
                                                TRACE_FLAG_IOCTL,
                                                "ClassInterpretSenseInfo (%p): Too many descriptors in parameter list for command %x (parameter field offset 0x%016llx)\n",
                                                Fdo,
                                                cdbOpcode,
                                                information));

                                    NT_ASSERTMSG("Too many descriptors specified", FALSE);

                                    *Status = STATUS_TOO_MANY_SEGMENT_DESCRIPTORS;
                                    break;
                                }

                                default: {

                                    if (ClasspIsOffloadDataTransferCommand(cdb)) {

                                        //
                                        // 1. (Various) Invalid parameter length
                                        // 2. Requested inactivity timeout is greater than maximum inactivity timeout
                                        // 3. Same LBA is included in more than one block device range descriptor (overlapping LBAs)
                                        // 4. Total number of logical blocks of all block range descriptors is greater than the maximum transfer size
                                        // 5. Total number of logical blocks of all block range descriptors is greater than maximum token transfer size
                                        //    (e.g. WriteUsingToken descriptors specify a cumulative total block count that exceeds the PopulateToken that created the token)
                                        // 6. Block offset into ROD specified an offset that is greater than or equal to the number of logical blocks in the ROD
                                        // 7. Number of logical blocks in a block device range descriptor is greater than maximum transfer length in blocks
                                        //
                                        TracePrint((TRACE_LEVEL_ERROR,
                                                    TRACE_FLAG_IOCTL,
                                                    "ClassInterpretSenseInfo (%p): Illegal field in parameter list for command %x (parameter field offset 0x%016llx) [AddSense %x, AddSenseQ %x]\n",
                                                    Fdo,
                                                    cdbOpcode,
                                                    information,
                                                    addlSenseCode,
                                                    addlSenseCodeQual));

                                        NT_ASSERTMSG("Invalid field in parameter list", FALSE);

                                        *Status = STATUS_INVALID_FIELD_IN_PARAMETER_LIST;
                                    }

                                    break;
                                }
                            }
                            break;
                        }

                        case SCSI_ADSENSE_COPY_PROTECTION_FAILURE: {
                            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                        "Copy protection failure\n"));

                            *Status = STATUS_COPY_PROTECTION_FAILURE;

                            switch (addlSenseCodeQual) {
                                case SCSI_SENSEQ_AUTHENTICATION_FAILURE:
                                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                                                "ClassInterpretSenseInfo: "
                                                "Authentication failure\n"));
                                    *Status = STATUS_CSS_AUTHENTICATION_FAILURE;
                                    break;
                                case SCSI_SENSEQ_KEY_NOT_PRESENT:
                                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                                                "ClassInterpretSenseInfo: "
                                                "Key not present\n"));
                                    *Status = STATUS_CSS_KEY_NOT_PRESENT;
                                    break;
                                case SCSI_SENSEQ_KEY_NOT_ESTABLISHED:
                                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                                                "ClassInterpretSenseInfo: "
                                                "Key not established\n"));
                                    *Status = STATUS_CSS_KEY_NOT_ESTABLISHED;
                                    break;
                                case SCSI_SENSEQ_READ_OF_SCRAMBLED_SECTOR_WITHOUT_AUTHENTICATION:
                                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                                                "ClassInterpretSenseInfo: "
                                                "Read of scrambled sector w/o "
                                                "authentication\n"));
                                    *Status = STATUS_CSS_SCRAMBLED_SECTOR;
                                    break;
                                case SCSI_SENSEQ_MEDIA_CODE_MISMATCHED_TO_LOGICAL_UNIT:
                                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                                                "ClassInterpretSenseInfo: "
                                                "Media region does not logical unit "
                                                "region\n"));
                                    *Status = STATUS_CSS_REGION_MISMATCH;
                                    break;
                                case SCSI_SENSEQ_LOGICAL_UNIT_RESET_COUNT_ERROR:
                                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,
                                                "ClassInterpretSenseInfo: "
                                                "Region set error -- region may "
                                                "be permanent\n"));
                                    *Status = STATUS_CSS_RESETS_EXHAUSTED;
                                    break;
                            } // end switch of ASCQ for COPY_PROTECTION_FAILURE

                            break;
                        }

                        case SCSI_ADSENSE_MUSIC_AREA: {
                            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                        "Music area\n"));
                            break;
                        }

                        case SCSI_ADSENSE_DATA_AREA: {
                            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                        "Data area\n"));
                            break;
                        }

                        case SCSI_ADSENSE_VOLUME_OVERFLOW: {
                            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                        "Volume overflow\n"));
                            break;
                        }

                    } // end switch (addlSenseCode)

                    break;
                } // end SCSI_SENSE_ILLEGAL_REQUEST

                case SCSI_SENSE_UNIT_ATTENTION: {

                    ULONG count;

                    //
                    // A media change may have occured so increment the change
                    // count for the physical device
                    //

                    count = InterlockedIncrement((volatile LONG *)&fdoExtension->MediaChangeCount);
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_MCN,  "ClassInterpretSenseInfo: "
                                "Media change count for device %d incremented to %#lx\n",
                                fdoExtension->DeviceNumber, count));


                    switch (addlSenseCode) {
                        case SCSI_ADSENSE_MEDIUM_CHANGED: {
                            logRetryableError = FALSE;
                            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_MCN,  "ClassInterpretSenseInfo: "
                                        "Media changed\n"));

                            if (!TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA)) {
                                TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_MCN, "ClassInterpretSenseInfo: "
                                            "Media Changed on non-removable device %p\n",
                                            Fdo));
                            }
                            ClassSetMediaChangeState(fdoExtension, MediaPresent, FALSE);
                            break;
                        }

                        case SCSI_ADSENSE_BUS_RESET: {
                            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                        "Bus reset\n"));
                            break;
                        }

                        case SCSI_ADSENSE_PARAMETERS_CHANGED: {
                            logRetryableError = FALSE;
                            if (addlSenseCodeQual == SCSI_SENSEQ_CAPACITY_DATA_CHANGED) {
                                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                            "Device capacity changed (e.g. thinly provisioned LUN). Retry the request.\n"));

                                ClassQueueCapacityChangedEventWorker(Fdo);

                                //
                                // Retry with 1 second delay as ClassQueueCapacityChangedEventWorker may trigger a couple of commands sent to disk.
                                //
                                retryInterval = 1;
                                retry = TRUE;
                            }
                            break;
                        }

                        case SCSI_ADSENSE_LB_PROVISIONING: {

                            switch (addlSenseCodeQual) {

                                case SCSI_SENSEQ_SOFT_THRESHOLD_REACHED: {

                                    logRetryableError = FALSE;
                                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                            "Device (%p) has hit a soft threshold.\n",
                                            Fdo));

                                    //
                                    // This indicates that a resource provisioned or thinly
                                    // provisioned device has hit a soft threshold.  Queue a
                                    // worker thread to log a system event and then retry the
                                    // original request.
                                    //
                                    ClassQueueThresholdEventWorker(Fdo);
                                    break;
                                }
                                default: {
                                    retry = FALSE;
                                    break;
                                }

                            } // end  switch (addlSenseCodeQual)
                            break;
                        }

                        case SCSI_ADSENSE_OPERATING_CONDITIONS_CHANGED: {

                            if (addlSenseCodeQual == SCSI_SENSEQ_MICROCODE_CHANGED) {
                                //
                                // Device firmware has been changed. Retry the request.
                                //
                                logRetryableError = TRUE;
                                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                            "Device firmware has been changed.\n"));

                                retryInterval = 1;
                                retry = TRUE;
                            } else {
                                //
                                // Device information has changed, we need to rescan the
                                // bus for changed information such as the capacity.
                                //
                                logRetryableError = FALSE;
                                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                            "Device information changed. Invalidate the bus\n"));

                                if (addlSenseCodeQual == SCSI_SENSEQ_INQUIRY_DATA_CHANGED) {

                                    ClassQueueProvisioningTypeChangedEventWorker(Fdo);
                                }

                                if (addlSenseCodeQual == SCSI_SENSEQ_INQUIRY_DATA_CHANGED ||
                                    addlSenseCodeQual == SCSI_SENSEQ_OPERATING_DEFINITION_CHANGED) {

                                    //
                                    // Since either the LB provisioning type changed, or the block/slab size
                                    // changed, next time anyone trying to query the FunctionSupportInfo, we
                                    // will requery the device.
                                    //
                                    InterlockedIncrement((volatile LONG *)&fdoExtension->FunctionSupportInfo->ChangeRequestCount);
                                }

                                IoInvalidateDeviceRelations(fdoExtension->LowerPdo, BusRelations);
                                retryInterval = 5;
                            }
                            break;
                        }

                        case SCSI_ADSENSE_OPERATOR_REQUEST: {
                            switch (addlSenseCodeQual) {

                                case SCSI_SENSEQ_MEDIUM_REMOVAL: {
                                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                                "Ejection request received!\n"));
                                    ClassSendEjectionNotification(fdoExtension);
                                    break;
                                }

                                case SCSI_SENSEQ_WRITE_PROTECT_ENABLE: {
                                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                                "Operator selected write permit?! "
                                                "(unsupported!)\n"));
                                    break;
                                }

                                case SCSI_SENSEQ_WRITE_PROTECT_DISABLE: {
                                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                                "Operator selected write protect?! "
                                                "(unsupported!)\n"));
                                    break;
                                }
                            }
                        }

                        case SCSI_ADSENSE_FAILURE_PREDICTION_THRESHOLD_EXCEEDED: {

                            UCHAR wmiEventData[sizeof(ULONG)+sizeof(UCHAR)] = {0};

                            *((PULONG)wmiEventData) = sizeof(UCHAR);
                            wmiEventData[sizeof(ULONG)] = addlSenseCodeQual;

                            //
                            // Don't log another eventlog if we have already logged once
                            // NOTE: this should have been interlocked, but the structure
                            //       was publicly defined to use a BOOLEAN (char).  Since
                            //       media only reports these errors once per X minutes,
                            //       the potential race condition is nearly non-existant.
                            //       the worst case is duplicate log entries, so ignore.
                            //

                            logError = FALSE;
                            if (fdoExtension->FailurePredicted == 0) {
                                logError = TRUE;
                            }
                            fdoExtension->FailureReason = addlSenseCodeQual;
                            logStatus = IO_WRN_FAILURE_PREDICTED;

                            ClassNotifyFailurePredicted(fdoExtension,
                                                        (PUCHAR)wmiEventData,
                                                        sizeof(wmiEventData),
                                                        FALSE,   // do not log error
                                                        4,          // unique error value
                                                        SrbGetPathId(Srb),
                                                        SrbGetTargetId(Srb),
                                                        SrbGetLun(Srb));

                            fdoExtension->FailurePredicted = TRUE;

                            //
                            // Since this is a Unit Attention we need to make
                            // sure we retry this request.
                            //
                            retry = TRUE;

                            break;
                        }

                        default: {
                            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                        "Unit attention\n"));
                            break;
                        }

                    } // end  switch (addlSenseCode)

                    if (TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA))
                    {

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
                        else {
                            *Status = STATUS_IO_DEVICE_ERROR;
                        }
                    }
                    else
                    {
                        *Status = STATUS_IO_DEVICE_ERROR;
                    }

                    break;

                } // end SCSI_SENSE_UNIT_ATTENTION

                case SCSI_SENSE_DATA_PROTECT: {

                    retry = FALSE;

                    if (addlSenseCode == SCSI_ADSENSE_WRITE_PROTECT)
                    {
                        switch (addlSenseCodeQual) {

                            case SCSI_SENSEQ_SPACE_ALLOC_FAILED_WRITE_PROTECT: {

                                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                        "Device's (%p) resources are exhausted.\n",
                                        Fdo));

                                ClassQueueResourceExhaustionEventWorker(Fdo);

                                //
                                // This indicates that a thinly-provisioned device has
                                // hit a permanent resource exhaustion.  We need to
                                // return this status code so that patmgr can take the
                                // disk offline.
                                //
                                *Status = STATUS_DISK_RESOURCES_EXHAUSTED;
                                break;
                            }
                            default:
                            {
                                break;
                            }

                        } // end switch addlSenseCodeQual
                    }
                    else
                    {
                        if (IS_SCSIOP_WRITE(cdbOpcode)) {
                            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                        "Media write protected\n"));
                            *Status = STATUS_MEDIA_WRITE_PROTECTED;
                        } else {
                            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL, "ClassInterpretSenseInfo: "
                                        "Access denied\n"));
                            *Status = STATUS_ACCESS_DENIED;
                        }
                    }
                    break;
                } // end SCSI_SENSE_DATA_PROTECT

                case SCSI_SENSE_BLANK_CHECK: {
                    TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                "Media blank check\n"));
                    retry = FALSE;
                    *Status = STATUS_NO_DATA_DETECTED;
                    break;
                } // end SCSI_SENSE_BLANK_CHECK

                case SCSI_SENSE_COPY_ABORTED: {

                    switch (addlSenseCode) {

                        case SCSI_ADSENSE_COPY_TARGET_DEVICE_ERROR: {

                            switch (addlSenseCodeQual) {

                                //
                                // 1. Target truncated the data transfer.
                                //
                                case SCSI_SENSEQ_DATA_UNDERRUN: {

                                    TracePrint((TRACE_LEVEL_WARNING,
                                                TRACE_FLAG_IOCTL,
                                                "ClassInterpretSenseInfo (%p): Data transfer was truncated (command %x)\n",
                                                Fdo,
                                                cdbOpcode));

                                    *Status = STATUS_SUCCESS;
                                    retry = FALSE;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    break;
                }

                case SCSI_SENSE_ABORTED_COMMAND: {
                    if (ClasspIsOffloadDataTransferCommand(cdb)) {

                        switch (addlSenseCode) {

                            case SCSI_ADSENSE_COPY_TARGET_DEVICE_ERROR: {

                                switch (addlSenseCodeQual) {

                                    //
                                    // 1. Target truncated the data transfer.
                                    //
                                    case SCSI_SENSEQ_DATA_UNDERRUN: {

                                        TracePrint((TRACE_LEVEL_WARNING,
                                                    TRACE_FLAG_IOCTL,
                                                    "ClassInterpretSenseInfo (%p): Target has truncated the data transfer (command %x)\n",
                                                    Fdo,
                                                    cdbOpcode));

                                        *Status = STATUS_SUCCESS;
                                        retry = FALSE;
                                        break;
                                    }
                                }
                                break;
                            }

                            case SCSI_ADSENSE_RESOURCE_FAILURE: {

                                switch (addlSenseCodeQual) {

                                    //
                                    // 1. Copy manager wasn't able to finish the operation because of insuffient resources
                                    //    (e.g. microsnapshot failure on read, no space on write, etc.)
                                    //
                                    case SCSI_SENSEQ_INSUFFICIENT_RESOURCES: {

                                        TracePrint((TRACE_LEVEL_ERROR,
                                                    TRACE_FLAG_IOCTL,
                                                    "ClassInterpretSenseInfo (%p): Target has insufficient resources (command %x)\n",
                                                    Fdo,
                                                    cdb->CDB6GENERIC.OperationCode));

                                        *Status = STATUS_INSUFFICIENT_RESOURCES;
                                        retry = FALSE;
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                    } else {
                        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                    "Command aborted\n"));
                        *Status = STATUS_IO_DEVICE_ERROR;
                        retryInterval = 1;
                    }
                    break;
                } // end SCSI_SENSE_ABORTED_COMMAND

                default: {
                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                                "Unrecognized sense code\n"));
                    *Status = STATUS_IO_DEVICE_ERROR;
                    break;
                }

            } // end switch (senseKey)



            //
            // Try to determine bad sector information from sense data
            //

            if (((IS_SCSIOP_READWRITE(cdbOpcode))   ||
                (cdbOpcode == SCSIOP_VERIFY)        ||
                (cdbOpcode == SCSIOP_VERIFY16)) && cdb) {

                if (isInformationValid)
                {
                    readSector = 0;
                    badSector = information;

                    if (cdbOpcode == SCSIOP_READ16 || cdbOpcode == SCSIOP_WRITE16 || cdbOpcode == SCSIOP_VERIFY16) {
                        REVERSE_BYTES_QUAD(&readSector, &(cdb->AsByte[2]));
                    } else {
                        REVERSE_BYTES(&readSector, &(cdb->AsByte[2]));
                    }

                    if (cdbOpcode == SCSIOP_READ || cdbOpcode == SCSIOP_WRITE || cdbOpcode == SCSIOP_VERIFY) {
                        REVERSE_BYTES_SHORT(&index, &(cdb->CDB10.TransferBlocksMsb));
                    } else if (cdbOpcode == SCSIOP_READ6 ||  cdbOpcode == SCSIOP_WRITE6) {
                        index = cdb->CDB6READWRITE.TransferBlocks;
                    } else if(cdbOpcode == SCSIOP_READ12 || cdbOpcode == SCSIOP_WRITE12) {
                        REVERSE_BYTES(&index, &(cdb->CDB12.TransferLength));
                    } else {
                        REVERSE_BYTES(&index, &(cdb->CDB16.TransferLength));
                    }

                    //
                    // Make sure the bad sector is within the read sectors.
                    //

                    if (!(badSector >= readSector && badSector < readSector + index)) {
                        badSector = readSector;
                    }
                }
            }
        }

__ClassInterpretSenseInfo_ProcessingInvalidSenseBuffer:

        if (!validSense) {

            //
            // Request sense buffer not valid. No sense information
            // to pinpoint the error. Return general request fail.
            //

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClassInterpretSenseInfo: "
                        "Request sense info not valid. SrbStatus %2x\n",
                        SRB_STATUS(Srb->SrbStatus)));
            retry = TRUE;


            switch (SRB_STATUS(Srb->SrbStatus)) {
                case SRB_STATUS_ABORTED: {

                    //
                    // Update the error count for the device.
                    //

                    incrementErrorCount = TRUE;
                    *Status = STATUS_IO_TIMEOUT;
                    retryInterval = 1;
                    retry = TRUE;
                    break;
                }

                case SRB_STATUS_ERROR: {

                    *Status = STATUS_IO_DEVICE_ERROR;
                    if (SrbGetScsiStatus(Srb) == SCSISTAT_GOOD) {

                        //
                        // This is some strange return code.  Update the error
                        // count for the device.
                        //

                        incrementErrorCount = TRUE;

                    } else if (SrbGetScsiStatus(Srb) == SCSISTAT_BUSY) {

                        *Status = STATUS_DEVICE_NOT_READY;
                        logRetryableError = FALSE;
                    }

                    break;
                }

                case SRB_STATUS_INVALID_REQUEST: {
                    *Status = STATUS_INVALID_DEVICE_REQUEST;
                    retry = FALSE;
                    break;
                }

                case SRB_STATUS_INVALID_PATH_ID:
                case SRB_STATUS_NO_DEVICE:
                case SRB_STATUS_NO_HBA:
                case SRB_STATUS_INVALID_LUN:
                case SRB_STATUS_INVALID_TARGET_ID: {
                    *Status = STATUS_NO_SUCH_DEVICE;
                    retry = FALSE;
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

                case SRB_STATUS_TIMEOUT:
                case SRB_STATUS_COMMAND_TIMEOUT: {

                    //
                    // Update the error count for the device.
                    //
                    incrementErrorCount = TRUE;
                    *Status = STATUS_IO_TIMEOUT;
                    break;
                }

                case SRB_STATUS_PARITY_ERROR:
                case SRB_STATUS_UNEXPECTED_BUS_FREE:

                    //
                    // Update the error count for the device
                    // and fall through to below
                    //
                    incrementErrorCount = TRUE;

                case SRB_STATUS_BUS_RESET: {

                    *Status = STATUS_IO_DEVICE_ERROR;
                    logRetryableError = FALSE;
                    break;
                }

                case SRB_STATUS_DATA_OVERRUN: {

                    *Status = STATUS_DATA_OVERRUN;
                    retry = FALSE;

                    //
                    // For some commands, we allocate a buffer that may be
                    // larger than necessary.  In these cases, the SRB may be
                    // returned with SRB_STATUS_DATA_OVERRUN to indicate a
                    // buffer *underrun*.  However, the command was still
                    // successful so we ensure STATUS_SUCCESS is returned.
                    // We will also prevent these errors from causing noise in
                    // the error logs.
                    //
                    if ((cdbOpcode == SCSIOP_MODE_SENSE && SrbGetDataTransferLength(Srb) <= cdb->MODE_SENSE.AllocationLength) ||
                        (cdbOpcode == SCSIOP_INQUIRY && SrbGetDataTransferLength(Srb) <= cdb->CDB6INQUIRY.AllocationLength)) {
                        *Status = STATUS_SUCCESS;
                        logErrorInternal = FALSE;
                        logError = FALSE;
                    } else if (cdbOpcode == SCSIOP_MODE_SENSE10) {
                        USHORT allocationLength;
                        REVERSE_BYTES_SHORT(&(cdb->MODE_SENSE10.AllocationLength), &allocationLength);
                        if (SrbGetDataTransferLength(Srb) <= allocationLength) {
                            *Status = STATUS_SUCCESS;
                            logErrorInternal = FALSE;
                            logError = FALSE;
                        }
                    } else if (ClasspIsReceiveTokenInformation(cdb)) {
                        ULONG allocationLength;
                        REVERSE_BYTES(&(cdb->RECEIVE_TOKEN_INFORMATION.AllocationLength), &allocationLength);
                        if (SrbGetDataTransferLength(Srb) <= allocationLength) {
                            *Status = STATUS_SUCCESS;
                            logErrorInternal = FALSE;
                            logError = FALSE;
                        }
                    }

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
                        logRetryableError = FALSE;
                    }

                    break;
                }


                default: {
                    logError = TRUE;
                    logStatus = IO_ERR_CONTROLLER_ERROR;
                    uniqueId = 259;
                    *Status = STATUS_IO_DEVICE_ERROR;
                    unhandledError = TRUE;
                    logRetryableError = FALSE;
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

        SCSI_REQUEST_BLOCK tempSrb = {0};
        PSCSI_REQUEST_BLOCK srbPtr = (PSCSI_REQUEST_BLOCK)Srb;

        //
        // If class driver does not support extended SRB and this is
        // an extended SRB, convert to legacy SRB and pass to class
        // driver.
        //
        if ((Srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK) &&
            ((fdoExtension->CommonExtension.DriverExtension->SrbSupport &
              CLASS_SRB_STORAGE_REQUEST_BLOCK) == 0)) {
            ClasspConvertToScsiRequestBlock(&tempSrb, (PSTORAGE_REQUEST_BLOCK)Srb);
            srbPtr = &tempSrb;
        }

        fdoExtension->CommonExtension.DevInfo->ClassError(Fdo,
                                                          srbPtr,
                                                          Status,
                                                          &retry);
    }

    //
    // If the caller wants to know the suggested retry interval tell them.
    //

    if (ARGUMENT_PRESENT(RetryInterval)) {
        *RetryInterval = retryInterval;
    }

    //
    // The RESERVE(6) / RELEASE(6) commands are optional. So
    // if they aren't supported, try the 10-byte equivalents
    //

    cdb = SrbGetCdb(Srb);
    if (cdb) {
        cdbOpcode = cdb->CDB6GENERIC.OperationCode;
    }

    if ((cdbOpcode == SCSIOP_RESERVE_UNIT ||
         cdbOpcode == SCSIOP_RELEASE_UNIT) && cdb)
    {
        if (*Status == STATUS_INVALID_DEVICE_REQUEST)
        {
            SrbSetCdbLength(Srb, 10);
            cdb->CDB10.OperationCode = (cdb->CDB6GENERIC.OperationCode == SCSIOP_RESERVE_UNIT) ? SCSIOP_RESERVE_UNIT10 : SCSIOP_RELEASE_UNIT10;

            SET_FLAG(fdoExtension->PrivateFdoData->HackFlags, FDO_HACK_NO_RESERVE6);
            retry = TRUE;
        }
    }

#if DBG

    //
    // Ensure that for read/write requests, only return STATUS_DEVICE_BUSY if
    // reservation conflict.
    //
    if (IS_SCSIOP_READWRITE(cdbOpcode) && (*Status == STATUS_DEVICE_BUSY)) {
        NT_ASSERT(isReservationConflict == TRUE);
    }

#endif

    /*
     *  LOG the error:
     *      If logErrorInternal is set, log the error in our internal log.
     *      If logError is set, also log the error in the system log.
     */
    if (logErrorInternal || logError) {
        ULONG totalSize;
        ULONG senseBufferSize = 0;
        IO_ERROR_LOG_PACKET staticErrLogEntry = {0};
        CLASS_ERROR_LOG_DATA staticErrLogData = {0};
        SENSE_DATA convertedSenseBuffer = {0};
        UCHAR convertedSenseBufferLength = 0;
        BOOLEAN senseDataConverted = FALSE;

        //
        // Logic below assumes that IO_ERROR_LOG_PACKET + CLASS_ERROR_LOG_DATA
        // is less than ERROR_LOG_MAXIMUM_SIZE which is not true for extended SRB.
        // Given that classpnp currently does not use >16 byte CDB, we'll convert
        // an extended SRB to SCSI_REQUEST_BLOCK instead of changing this code.
        // More changes will need to be made when classpnp starts using >16 byte
        // CDBs.
        //

        //
        // Calculate the total size of the error log entry.
        // add to totalSize in the order that they are used.
        // the advantage to calculating all the sizes here is
        // that we don't have to do a bunch of extraneous checks
        // later on in this code path.
        //
        totalSize = sizeof(IO_ERROR_LOG_PACKET)  // required
                  + sizeof(CLASS_ERROR_LOG_DATA);// struct for ease

        //
        // also save any available extra sense data, up to the maximum errlog
        // packet size .  WMI should be used for real-time analysis.
        // the event log should only be used for post-mortem debugging.
        //
        if ((TEST_FLAG(Srb->SrbStatus, SRB_STATUS_AUTOSENSE_VALID)) && senseBuffer) {

            UCHAR validSenseBytes = 0;
            UCHAR senseInfoBufferLength = 0;

            senseInfoBufferLength = SrbGetSenseInfoBufferLength(Srb);

            //
            // If sense data is in Descriptor format, convert it to Fixed format
            // for the private log.
            //

            if (IsDescriptorSenseDataFormat(senseBuffer)) {

                convertedSenseBufferLength = sizeof(convertedSenseBuffer);

                senseDataConverted = ScsiConvertToFixedSenseFormat(senseBuffer,
                                                                   senseInfoBufferLength,
                                                                   (PVOID)&convertedSenseBuffer,
                                                                   convertedSenseBufferLength);
            }

            //
            // For System Log, copy the maximum amount of available sense data
            //

            if (ScsiGetTotalSenseByteCountIndicated(senseBuffer,
                                                    senseInfoBufferLength,
                                                    &validSenseBytes)) {

                //
                // If it is able to determine number of valid bytes,
                // copy the maximum amount of available
                // sense data that can be saved into the the errlog.
                //

                //
                // set to save the most sense buffer possible
                //

                senseBufferSize = max(validSenseBytes, sizeof(staticErrLogData.SenseData));
                senseBufferSize = min(senseBufferSize, senseInfoBufferLength);

            } else {
                //
                // it's smaller than required to read the total number of
                // valid bytes, so just use the SenseInfoBufferLength field.
                //
                senseBufferSize = senseInfoBufferLength;
            }

            /*
             *  Bump totalSize by the number of extra senseBuffer bytes
             *  (beyond the default sense buffer within CLASS_ERROR_LOG_DATA).
             *  Make sure to never allocate more than ERROR_LOG_MAXIMUM_SIZE.
             */
            if (senseBufferSize > sizeof(staticErrLogData.SenseData)){
                totalSize += senseBufferSize-sizeof(staticErrLogData.SenseData);
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
        // ISSUE: the test below should also check RetryCount to determine if we will actually retry,
        //            but there is no easy test because we'd have to consider the original retry count
        //            for the op; besides, InterpretTransferPacketError sometimes ignores the retry
        //            decision returned by this function.  So just ErrorRetried to be true in the majority case.
        //
        if (retry){
            staticErrLogEntry.FinalStatus = STATUS_SUCCESS;
            staticErrLogData.ErrorRetried = TRUE;
        } else {
            staticErrLogEntry.FinalStatus = *Status;
        }

        //
        // Don't log generic IO_WARNING_PAGING_FAILURE message if either the
        // I/O is retried, or it completed successfully.
        //
        if (logStatus == IO_WARNING_PAGING_FAILURE &&
            (retry || NT_SUCCESS(*Status)) ) {
            logError = FALSE;
        }

        if (TEST_FLAG(SrbGetSrbFlags(Srb), SRB_CLASS_FLAGS_PAGING)) {
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
         *  The dump data follows the IO_ERROR_LOG_PACKET
         */
        staticErrLogEntry.DumpDataSize = (USHORT)totalSize - sizeof(IO_ERROR_LOG_PACKET);

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
        if (Srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK) {
            ClasspConvertToScsiRequestBlock(&staticErrLogData.Srb, (PSTORAGE_REQUEST_BLOCK)Srb);
        } else {
            staticErrLogData.Srb = *(PSCSI_REQUEST_BLOCK)Srb;
        }

        /*
         *  For our private log, save just the default length of the SENSE_DATA.
         */

        if ((senseBufferSize != 0) && senseBuffer) {

            //
            // If sense buffer is in Fixed format, put it in the private log
            //
            // If sense buffer is in Descriptor format, put it in the private log if conversion to Fixed format
            // succeeded. Otherwise, do not put it in the private log.
            //
            // If sense buffer is in unknown format, the device or the driver probably does not populate
            // the first byte of sense data, we probably still want to log error in this case assuming
            // it's fixed format, so that its sense key, its additional sense code, and its additional sense code
            // qualifier would be shown in the debugger extension output. By doing so, it minimizes any potential
            // negative impacts to our ability to diagnose issue.
            //
            if (IsDescriptorSenseDataFormat(senseBuffer)) {
                if (senseDataConverted) {
                    RtlCopyMemory(&staticErrLogData.SenseData, &convertedSenseBuffer, min(convertedSenseBufferLength, sizeof(staticErrLogData.SenseData)));
                }
            } else {
                RtlCopyMemory(&staticErrLogData.SenseData, senseBuffer, min(senseBufferSize, sizeof(staticErrLogData.SenseData)));
            }
        }

        /*
         *  Save the error log in our context.
         *  We only save the default sense buffer length.
         */
        if (logErrorInternal) {
            KeAcquireSpinLock(&fdoData->SpinLock, &oldIrql);
            fdoData->ErrorLogs[fdoData->ErrorLogNextIndex] = staticErrLogData;
            fdoData->ErrorLogNextIndex++;
            fdoData->ErrorLogNextIndex %= NUM_ERROR_LOG_ENTRIES;
            KeReleaseSpinLock(&fdoData->SpinLock, oldIrql);
        }

        /*
         *  Log an event if an IO is being retried for reasons that may indicate
         *  a transient/permanent problem with the I_T_L nexus. But log the event
         *  only once per retried IO.
         */
        if (!IS_SCSIOP_READWRITE(cdbOpcode) ||
            !retry ||
            (RetryCount != 0)) {

            logRetryableError = FALSE;
        }

        if (logRetryableError) {

            logError = TRUE;
        }

        /*
         *  If logError is set, also save this log in the system's error log.
         *  But make sure we don't log TUR failures over and over
         *  (e.g. if an external drive was switched off and we're still sending TUR's to it every second).
         */

        if (logError)
        {
            //
            // We do not want to log certain system events repetitively
            //

            cdb = SrbGetCdb(Srb);
            if (cdb) {
                switch (cdb->CDB10.OperationCode)
                {
                    case SCSIOP_TEST_UNIT_READY:
                    {
                        if (fdoData->LoggedTURFailureSinceLastIO)
                        {
                            logError = FALSE;
                        }
                        else
                        {
                            fdoData->LoggedTURFailureSinceLastIO = TRUE;
                        }

                        break;
                    }

                    case SCSIOP_SYNCHRONIZE_CACHE:
                    {
                        if (fdoData->LoggedSYNCFailure)
                        {
                            logError = FALSE;
                        }
                        else
                        {
                            fdoData->LoggedSYNCFailure = TRUE;
                        }

                        break;
                    }
                }
            }
        }

        if (logError){

            if (logRetryableError) {

                NT_ASSERT(IS_SCSIOP_READWRITE(cdbOpcode));

                //
                // A large Disk TimeOutValue (like 60 seconds) results in giving a command a
                // large window to complete in. However, if the target returns a retryable error
                // just prior to the command timing out, and if multiple retries kick in, it may
                // take a significantly long time for the request to complete back to the
                // application, leading to a user perception of a hung system. So log an event
                // for retried IO so that an admin can help explain the reason for this behavior.
                //
                ClasspQueueLogIOEventWithContextWorker(Fdo,
                                                       senseBufferSize,
                                                       senseBuffer,
                                                       SRB_STATUS(Srb->SrbStatus),
                                                       SrbGetScsiStatus(Srb),
                                                       (ULONG)IO_WARNING_IO_OPERATION_RETRIED,
                                                       cdbLength,
                                                       cdb,
                                                       NULL);

            } else {

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
                    if ((senseBufferSize != 0) && senseBuffer) {
                        RtlCopyMemory(&errlogData->SenseData, senseBuffer, senseBufferSize);
                    }

                    /*
                     *  Write the error log packet to the system error logging thread.
                     *  It will be freed by the kernel.
                     */
                    IoWriteErrorLogEntry(errorLogEntry);
                }
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

    PageMode - Supplies the page or pages of mode sense data to be retrived.

Return Value:

    Length of the transferred data is returned.

--*/
ULONG
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassModeSense(
    _In_ PDEVICE_OBJECT Fdo,
    _In_reads_bytes_(Length) PCHAR ModeSenseBuffer,
    _In_ ULONG Length,
    _In_ UCHAR PageMode
    )
{
    PAGED_CODE();

    return ClasspModeSense(Fdo,
                           ModeSenseBuffer,
                           Length,
                           PageMode,
                           MODE_SENSE_CURRENT_VALUES);
}


ULONG
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassModeSenseEx(
    _In_ PDEVICE_OBJECT Fdo,
    _In_reads_bytes_(Length) PCHAR ModeSenseBuffer,
    _In_ ULONG Length,
    _In_ UCHAR PageMode,
    _In_ UCHAR PageControl
    )
{
    PAGED_CODE();
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
    return ClasspModeSense(Fdo,
                           ModeSenseBuffer,
                           Length,
                           PageMode,
                           PageControl);
#else
    UNREFERENCED_PARAMETER(Fdo);
    UNREFERENCED_PARAMETER(ModeSenseBuffer);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(PageMode);
    UNREFERENCED_PARAMETER(PageControl);
    return 0;
#endif
}

ULONG ClasspModeSense(
    _In_ PDEVICE_OBJECT Fdo,
    _In_reads_bytes_(Length) PCHAR ModeSenseBuffer,
    _In_ ULONG Length,
    _In_ UCHAR PageMode,
    _In_ UCHAR PageControl
    )
/*
Routine Description:

    This routine sends a mode sense command to a target ID and returns
    when it is complete.

Arguments:

    Fdo - Supplies the functional device object associated with this request.

    ModeSenseBuffer - Supplies a buffer to store the sense data.

    Length - Supplies the length in bytes of the mode sense buffer.

    PageMode - Supplies the page or pages of mode sense data to be retrived.

    PageControl - Supplies the page control value of the request, which is
        one of the following:
        MODE_SENSE_CURRENT_VALUES
        MODE_SENSE_CHANGEABLE_VALUES
        MODE_SENSE_DEFAULT_VAULES
        MODE_SENSE_SAVED_VALUES

Return Value:

    Length of the transferred data is returned.

--*/
{
    ULONG lengthTransferred = 0;
    PMDL senseBufferMdl;

    PAGED_CODE();

    senseBufferMdl = BuildDeviceInputMdl(ModeSenseBuffer, Length);
    if (senseBufferMdl){

        TRANSFER_PACKET *pkt = DequeueFreeTransferPacket(Fdo, TRUE);
        if (pkt){
            KEVENT event;
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
            NT_ASSERT(Length <= 0x0ff);
            KeInitializeEvent(&event, SynchronizationEvent, FALSE);
            SetupModeSenseTransferPacket(pkt, &event, ModeSenseBuffer, (UCHAR)Length, PageMode, 0, &pseudoIrp, PageControl);
            SubmitTransferPacket(pkt);
            (VOID)KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

            if (NT_SUCCESS(pseudoIrp.IoStatus.Status)){
                lengthTransferred = (ULONG)pseudoIrp.IoStatus.Information;
            }
            else {
                /*
                 *  This request can sometimes fail legitimately
                 *  (e.g. when a SCSI device is attached but turned off)
                 *  so this is not necessarily a device/driver bug.
                 */
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL, "ClasspModeSense on Fdo %ph failed with status %xh.", Fdo, pseudoIrp.IoStatus.Status));
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassFindModePage(
    _In_reads_bytes_(Length) PCHAR ModeSenseBuffer,
    _In_ ULONG Length,
    _In_ UCHAR PageMode,
    _In_ BOOLEAN Use6Byte
    )
{
    PUCHAR limit;
    ULONG  parameterHeaderLength;
    PVOID result = NULL;

    limit = (PUCHAR)ModeSenseBuffer + Length;
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
               RTL_SIZEOF_THROUGH_FIELD(MODE_DISCONNECT_PAGE, PageLength) < (PCHAR)limit) {

            if (((PMODE_DISCONNECT_PAGE) ModeSenseBuffer)->PageCode == PageMode) {

                /*
                 * found the mode page.  make sure it's safe to touch it all
                 * before returning the pointer to caller
                 */

                if (ModeSenseBuffer + ((PMODE_DISCONNECT_PAGE)ModeSenseBuffer)->PageLength > (PCHAR)limit) {
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


NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassModeSelect(
    _In_ PDEVICE_OBJECT Fdo,
    _In_reads_bytes_(Length) PCHAR ModeSelectBuffer,
    _In_ ULONG Length,
    _In_ BOOLEAN SavePages
    )
{
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
    return ClasspModeSelect(Fdo,
                            ModeSelectBuffer,
                            Length,
                            SavePages);
#else
    UNREFERENCED_PARAMETER(Fdo);
    UNREFERENCED_PARAMETER(ModeSelectBuffer);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(SavePages);
    return STATUS_NOT_SUPPORTED;
#endif
}

/*++
ClasspModeSelect()

Routine Description:

    This routine sends a mode select command to a target ID and returns
    when it is complete.

Arguments:

    Fdo - Supplies the functional device object associated with this request.

    ModeSelectBuffer - Supplies a buffer to the select data.

    Length - Supplies the length in bytes of the mode select buffer.

    SavePages - Specifies the value of the save pages (SP) bit in the mode
        select command.

Return Value:

    NTSTATUS code of the request.

--*/
NTSTATUS
ClasspModeSelect(
    _In_ PDEVICE_OBJECT Fdo,
    _In_reads_bytes_(Length) PCHAR ModeSelectBuffer,
    _In_ ULONG Length,
    _In_ BOOLEAN SavePages
    )
{

    PMDL senseBufferMdl;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    senseBufferMdl = BuildDeviceInputMdl(ModeSelectBuffer, Length);
    if (senseBufferMdl) {

        TRANSFER_PACKET *pkt = DequeueFreeTransferPacket(Fdo, TRUE);
        if (pkt){
            KEVENT event;
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
            NT_ASSERT(Length <= 0x0ff);
            KeInitializeEvent(&event, SynchronizationEvent, FALSE);
            SetupModeSelectTransferPacket(pkt, &event, ModeSelectBuffer, (UCHAR)Length, SavePages, &pseudoIrp);
            SubmitTransferPacket(pkt);
            (VOID)KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

            if (!NT_SUCCESS(pseudoIrp.IoStatus.Status)){
                /*
                 *  This request can sometimes fail legitimately
                 *  (e.g. when a SCSI device is attached but turned off)
                 *  so this is not necessarily a device/driver bug.
                 */
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_GENERAL, "ClassModeSelect on Fdo %ph failed with status %xh.", Fdo, pseudoIrp.IoStatus.Status));
            }

            status = pseudoIrp.IoStatus.Status;
        }

        FreeDeviceInputMdl(senseBufferMdl);
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

/*++////////////////////////////////////////////////////////////////////////////

ClassSendSrbAsynchronous()

Routine Description:

    This routine takes a partially built Srb and an Irp and sends it down to
    the port driver.

    This routine must be called with the remove lock held for the specified
    Irp.

Arguments:

    Fdo - Supplies the functional device object for the orginal request.

    Srb - Supplies a paritally build ScsiRequestBlock.  In particular, the
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
_Success_(return == STATUS_PENDING)
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassSendSrbAsynchronous(
    _In_ PDEVICE_OBJECT Fdo,
    _Inout_ __on_failure(__drv_freesMem(Mem)) __drv_aliasesMem PSCSI_REQUEST_BLOCK _Srb,
    _In_ PIRP Irp,
    _In_reads_bytes_opt_(BufferLength) __drv_aliasesMem PVOID BufferAddress,
    _In_ ULONG BufferLength,
    _In_ BOOLEAN WriteToDevice
    )
{

    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PIO_STACK_LOCATION irpStack;
    PSTORAGE_REQUEST_BLOCK_HEADER Srb = (PSTORAGE_REQUEST_BLOCK_HEADER)_Srb;

    ULONG savedFlags;

    if (Srb->Function != SRB_FUNCTION_STORAGE_REQUEST_BLOCK) {
        //
        // Write length to SRB.
        //

        Srb->Length = sizeof(SCSI_REQUEST_BLOCK);

        //
        // Set SCSI bus address.
        //

        Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    }

    //
    // This is a violation of the SCSI spec but it is required for
    // some targets.
    //

    // Srb->Cdb[1] |= deviceExtension->Lun << 5;

    //
    // Indicate auto request sense by specifying buffer and size.
    //

    SrbSetSenseInfoBuffer(Srb, fdoExtension->SenseData);
    SrbSetSenseInfoBufferLength(Srb, GET_FDO_EXTENSON_SENSE_DATA_LENGTH(fdoExtension));

    SrbSetDataBuffer(Srb, BufferAddress);

    //
    // Set the transfer length.
    //
    SrbSetDataTransferLength(Srb, BufferLength);

    //
    // Save the class driver specific flags away.
    //

    savedFlags = SrbGetSrbFlags(Srb) & SRB_FLAGS_CLASS_DRIVER_RESERVED;

    //
    // Allow the caller to specify that they do not wish
    // IoStartNextPacket() to be called in the completion routine.
    //

    SET_FLAG(savedFlags, (SrbGetSrbFlags(Srb) & SRB_FLAGS_DONT_START_NEXT_PACKET));

    //
    // If caller wants to this request to be tagged, save this fact.
    //

    if ( TEST_FLAG(SrbGetSrbFlags(Srb), SRB_FLAGS_QUEUE_ACTION_ENABLE) &&
         ( SRB_SIMPLE_TAG_REQUEST == SrbGetRequestAttribute(Srb) ||
           SRB_HEAD_OF_QUEUE_TAG_REQUEST == SrbGetRequestAttribute(Srb) ||
           SRB_ORDERED_QUEUE_TAG_REQUEST == SrbGetRequestAttribute(Srb) ) ) {

        SET_FLAG(savedFlags, SRB_FLAGS_QUEUE_ACTION_ENABLE);
        if (TEST_FLAG(SrbGetSrbFlags(Srb), SRB_FLAGS_NO_QUEUE_FREEZE)) {
            SET_FLAG(savedFlags, SRB_FLAGS_NO_QUEUE_FREEZE);
        }
    }

    if (BufferAddress != NULL) {

        //
        // Build Mdl if necessary.
        //

        if (Irp->MdlAddress == NULL) {

            PMDL mdl;

            mdl = IoAllocateMdl(BufferAddress,
                                BufferLength,
                                FALSE,
                                FALSE,
                                Irp);

            if ((mdl == NULL) || (Irp->MdlAddress == NULL)) {

                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;

                //
                // ClassIoComplete() would have free'd the srb
                //

                if (PORT_ALLOCATED_SENSE_EX(fdoExtension, Srb)) {
                    FREE_PORT_ALLOCATED_SENSE_BUFFER_EX(fdoExtension, Srb);
                }
                ClassFreeOrReuseSrb(fdoExtension, (PSCSI_REQUEST_BLOCK)Srb);
                ClassReleaseRemoveLock(Fdo, Irp);
                ClassCompleteRequest(Fdo, Irp, IO_NO_INCREMENT);

                return STATUS_INSUFFICIENT_RESOURCES;
            }

            SET_FLAG(savedFlags, SRB_CLASS_FLAGS_FREE_MDL);

            MmBuildMdlForNonPagedPool(Irp->MdlAddress);

        } else {

            //
            // Make sure the buffer requested matches the MDL.
            //

            NT_ASSERT(BufferAddress == MmGetMdlVirtualAddress(Irp->MdlAddress));
        }

        //
        // Set read flag.
        //

        SrbAssignSrbFlags(Srb, WriteToDevice ? SRB_FLAGS_DATA_OUT : SRB_FLAGS_DATA_IN);

    } else {

        //
        // Clear flags.
        //

        SrbAssignSrbFlags(Srb, SRB_FLAGS_NO_DATA_TRANSFER);
    }

    //
    // Restore saved flags.
    //

    SrbSetSrbFlags(Srb, savedFlags);

    //
    // Disable synchronous transfer for these requests.
    //

    SrbSetSrbFlags(Srb, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);

    //
    // Zero out status.
    //

    SrbSetScsiStatus(Srb, 0);
    Srb->SrbStatus = 0;

    SrbSetNextSrb(Srb, NULL);

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

    irpStack->Parameters.Scsi.Srb = (PSCSI_REQUEST_BLOCK)Srb;

    //
    // Set up Irp Address.
    //

    SrbSetOriginalRequest(Srb, Irp);

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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassDeviceControlDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{

    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    ULONG isRemoved;

    isRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);
    _Analysis_assume_(isRemoved);
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

    NT_ASSERT(commonExtension->DevInfo->ClassDeviceControl);

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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextStack = NULL;

    ULONG controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    PSCSI_REQUEST_BLOCK srb = NULL;
    PCDB cdb = NULL;

    NTSTATUS status;
    ULONG modifiedIoControlCode = 0;
    GUID activityId = {0};


    //
    // If this is a pass through I/O control, set the minor function code
    // and device address and pass it to the port driver.
    //

    if ( (controlCode == IOCTL_SCSI_PASS_THROUGH) ||
         (controlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT) ||
         (controlCode == IOCTL_SCSI_PASS_THROUGH_EX) ||
         (controlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT_EX) ) {



        //
        // Validiate the user buffer for SCSI pass through.
        // For pass through EX: as the handler will validate the size anyway,
        //                      do not apply the similar check and leave the work to the handler.
        //
        if ( (controlCode == IOCTL_SCSI_PASS_THROUGH) ||
             (controlCode == IOCTL_SCSI_PASS_THROUGH_DIRECT) ) {

        #if BUILD_WOW64_ENABLED && defined(_WIN64)

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
            RtlZeroMemory(uniqueId, sizeof(MOUNTDEV_UNIQUE_ID));
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

            NT_ASSERT(commonExtension->DeviceName.Buffer);

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(MOUNTDEV_NAME)) {

                status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
                break;
            }

            name = Irp->AssociatedIrp.SystemBuffer;
            RtlZeroMemory(name, sizeof(MOUNTDEV_NAME));
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
            WCHAR driveLetterNameBuffer[10] = {0};
            RTL_QUERY_REGISTRY_TABLE queryTable[2] = {0};
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
            driveLetterName.MaximumLength = sizeof(driveLetterNameBuffer);
            driveLetterName.Length = 0;

            queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED |
                                  RTL_QUERY_REGISTRY_DIRECT |
                                  RTL_QUERY_REGISTRY_TYPECHECK;
            queryTable[0].Name = valueName;
            queryTable[0].EntryContext = &driveLetterName;
            queryTable->DefaultType = (REG_SZ << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_NONE;

            status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                            L"\\Registry\\Machine\\System\\DISK",
                                            queryTable, NULL, NULL);

            if (!NT_SUCCESS(status)) {
                FREE_POOL(valueName);
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
                FREE_POOL(valueName);
                break;
            }

            suggestedName = Irp->AssociatedIrp.SystemBuffer;
            RtlZeroMemory(suggestedName, sizeof(MOUNTDEV_SUGGESTED_LINK_NAME));
            suggestedName->UseOnlyIfThereAreNoOtherLinks = TRUE;
            suggestedName->NameLength = 28;

            Irp->IoStatus.Information =
                    FIELD_OFFSET(MOUNTDEV_SUGGESTED_LINK_NAME, Name) + 28;

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                Irp->IoStatus.Information) {

                Irp->IoStatus.Information =
                        sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);
                status = STATUS_BUFFER_OVERFLOW;
                FREE_POOL(valueName);
                break;
            }

            RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                   L"\\Registry\\Machine\\System\\DISK",
                                   valueName);

            FREE_POOL(valueName);

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
        PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = (PFUNCTIONAL_DEVICE_EXTENSION)commonExtension;
        size_t sizeNeeded;

        //
        // Allocate a SCSI SRB for handling various IOCTLs.
        // NOTE - there is a case where an IOCTL is sent to classpnp before AdapterDescriptor
        // is initialized. In this case, default to legacy SRB.
        //
        if ((fdoExtension->AdapterDescriptor != NULL) &&
            (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK)) {
            sizeNeeded = CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE;
        } else {
            sizeNeeded = sizeof(SCSI_REQUEST_BLOCK);
        }

        srb = ExAllocatePoolWithTag(NonPagedPoolNx,
                             sizeNeeded +
                             (sizeof(ULONG_PTR) * 2),
                             '9CcS');

        if (srb == NULL) {

            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            ClassReleaseRemoveLock(DeviceObject, Irp);
            ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto SetStatusAndReturn;
        }

        if ((fdoExtension->AdapterDescriptor != NULL) &&
            (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK)) {
            status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)srb,
                                                   STORAGE_ADDRESS_TYPE_BTL8,
                                                   CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE,
                                                   1,
                                                   SrbExDataTypeScsiCdb16);
            if (NT_SUCCESS(status)) {
                ((PSTORAGE_REQUEST_BLOCK)srb)->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
                function = (PULONG_PTR)((PCHAR)srb + sizeNeeded);
            } else {
                //
                // Should not occur.
                //
                NT_ASSERT(FALSE);
                goto SetStatusAndReturn;
            }
        } else {
            RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));
            srb->Length = sizeof(SCSI_REQUEST_BLOCK);
            srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
            function = (PULONG_PTR) ((PSCSI_REQUEST_BLOCK) (srb + 1));
        }

        //
        // Save the function code and the device object in the memory after
        // the SRB.
        //

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

    TracePrint((TRACE_LEVEL_VERBOSE, TRACE_FLAG_GENERAL, "> ioctl %xh (%s)", modifiedIoControlCode, DBGGETIOCTLSTR(modifiedIoControlCode)));


    switch (modifiedIoControlCode) {

        case IOCTL_STORAGE_GET_HOTPLUG_INFO: {

            FREE_POOL(srb);

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(STORAGE_HOTPLUG_INFO)) {

                //
                // Indicate unsuccessful status and no data transferred.
                //

                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = sizeof(STORAGE_HOTPLUG_INFO);

                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
                status = STATUS_BUFFER_TOO_SMALL;

            } else if (!commonExtension->IsFdo) {


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

            FREE_POOL(srb);

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

            if (!commonExtension->IsFdo) {


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

                if (NT_SUCCESS(status))
                {
                    if (info->WriteCacheEnableOverride != fdoExtension->PrivateFdoData->HotplugInfo.WriteCacheEnableOverride)
                    {
                        fdoExtension->PrivateFdoData->HotplugInfo.WriteCacheEnableOverride = info->WriteCacheEnableOverride;

                        //
                        // Store the user-defined override in the registry
                        //

                        ClassSetDeviceParameter(fdoExtension,
                                                CLASSP_REG_SUBKEY_NAME,
                                                CLASSP_REG_WRITE_CACHE_VALUE_NAME,
                                                info->WriteCacheEnableOverride);
                    }

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

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DeviceIoControl: Check verify\n"));

            //
            // If a buffer for a media change count was provided, make sure it's
            // big enough to hold the result
            //

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength) {

                //
                // If the buffer is too small to hold the media change count
                // then return an error to the caller
                //

                if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(ULONG)) {

                    TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL, "DeviceIoControl: media count "
                                                                       "buffer too small\n"));

                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                    Irp->IoStatus.Information = sizeof(ULONG);

                    FREE_POOL(srb);

                    ClassReleaseRemoveLock(DeviceObject, Irp);
                    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                    status = STATUS_BUFFER_TOO_SMALL;
                    goto SetStatusAndReturn;
                }
            }

            if (!commonExtension->IsFdo) {


                //
                // If this is a PDO then we should just forward the request down
                //
                NT_ASSERT(!srb);

                IoCopyCurrentIrpStackLocationToNext(Irp);

                ClassReleaseRemoveLock(DeviceObject, Irp);

                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);

                goto SetStatusAndReturn;

            } else {

                fdoExtension = DeviceObject->DeviceExtension;

            }

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength) {

                //
                // The caller has provided a valid buffer.  Allocate an additional
                // irp and stick the CheckVerify completion routine on it.  We will
                // then send this down to the port driver instead of the irp the
                // caller sent in
                //

                TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "DeviceIoControl: Check verify wants "
                                                                       "media count\n"));

                //
                // Allocate a new irp to send the TestUnitReady to the port driver
                //

                irp2 = IoAllocateIrp((CCHAR) (DeviceObject->StackSize + 3), FALSE);

                if (irp2 == NULL) {
                    Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                    Irp->IoStatus.Information = 0;
                    FREE_POOL(srb);
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
                newStack->Flags = irpStack->Flags;


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

            SrbSetCdbLength(srb, 6);
            cdb = SrbGetCdb(srb);
            cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

            //
            // Set timeout value.
            //

            SrbSetTimeOutValue(srb, fdoExtension->TimeOutValue);

            //
            // If this was a CV2 then mark the request as low-priority so we don't
            // spin up the drive just to satisfy it.
            //

            if (controlCode == IOCTL_STORAGE_CHECK_VERIFY2) {
                SrbSetSrbFlags(srb, SRB_CLASS_FLAGS_LOW_PRIORITY);
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

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,  "DiskIoControl: ejection control\n"));

            FREE_POOL(srb);

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
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

            if (!commonExtension->IsFdo) {


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
                // NT_ASSERT(TEST_FLAG(DeviceObject->Characteristics, FILE_REMOVABLE_MEDIA));
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

            TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL,  "DiskIoControl: MCN control\n"));

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(PREVENT_MEDIA_REMOVAL)) {

                //
                // Indicate unsuccessful status and no data transferred.
                //

                Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;
                Irp->IoStatus.Information = 0;

                FREE_POOL(srb);

                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
                status = STATUS_INFO_LENGTH_MISMATCH;
                goto SetStatusAndReturn;
            }

            if (!commonExtension->IsFdo) {


                //
                // Just forward this down and return
                //

                FREE_POOL(srb);

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

            if (!commonExtension->IsFdo) {


                IoCopyCurrentIrpStackLocationToNext(Irp);

                ClassReleaseRemoveLock(DeviceObject, Irp);
                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                goto SetStatusAndReturn;

            } else {
                fdoExtension = DeviceObject->DeviceExtension;
            }

            if (TEST_FLAG(fdoExtension->PrivateFdoData->HackFlags, FDO_HACK_NO_RESERVE6))
            {
                SrbSetCdbLength(srb, 10);
                cdb = SrbGetCdb(srb);
                cdb->CDB10.OperationCode = (modifiedIoControlCode == IOCTL_STORAGE_RESERVE) ? SCSIOP_RESERVE_UNIT10 : SCSIOP_RELEASE_UNIT10;
            }
            else
            {
                SrbSetCdbLength(srb, 6);
                cdb = SrbGetCdb(srb);
                cdb->CDB6GENERIC.OperationCode = (modifiedIoControlCode == IOCTL_STORAGE_RESERVE) ? SCSIOP_RESERVE_UNIT : SCSIOP_RELEASE_UNIT;
            }

            //
            // Set timeout value.
            //

            SrbSetTimeOutValue(srb, fdoExtension->TimeOutValue);

            //
            // Send reserves as tagged requests.
            //

            if ( IOCTL_STORAGE_RESERVE == modifiedIoControlCode ) {
                SrbSetSrbFlags(srb, SRB_FLAGS_QUEUE_ACTION_ENABLE);
                SrbSetRequestAttribute(srb, SRB_SIMPLE_TAG_REQUEST);
            }

            status = ClassSendSrbAsynchronous(DeviceObject,
                                              srb,
                                              Irp,
                                              NULL,
                                              0,
                                              FALSE);

            break;
        }

        case IOCTL_STORAGE_PERSISTENT_RESERVE_IN:
        case IOCTL_STORAGE_PERSISTENT_RESERVE_OUT: {

            if (!commonExtension->IsFdo) {

                IoCopyCurrentIrpStackLocationToNext(Irp);

                ClassReleaseRemoveLock(DeviceObject, Irp);
                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                goto SetStatusAndReturn;
            }

            //
            // Process Persistent Reserve
            //

            status = ClasspPersistentReserve(DeviceObject, Irp, srb);

            break;

        }

        case IOCTL_STORAGE_EJECT_MEDIA:
        case IOCTL_STORAGE_LOAD_MEDIA:
        case IOCTL_STORAGE_LOAD_MEDIA2:{

            //
            // Eject media.
            //

            PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = NULL;

            if (!commonExtension->IsFdo) {


                IoCopyCurrentIrpStackLocationToNext(Irp);

                ClassReleaseRemoveLock(DeviceObject, Irp);

                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                goto SetStatusAndReturn;
            } else {
                fdoExtension = DeviceObject->DeviceExtension;
            }

            if (commonExtension->PagingPathCount != 0) {

                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,  "ClassDeviceControl: call to eject paging device - "
                                                                    "failure\n"));

                status = STATUS_FILES_OPEN;
                Irp->IoStatus.Status = status;

                Irp->IoStatus.Information = 0;

                FREE_POOL(srb);

                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
                goto SetStatusAndReturn;
            }

            //
            // Synchronize with ejection control and ejection cleanup code as
            // well as other eject/load requests.
            //

            KeEnterCriticalRegion();
            (VOID)KeWaitForSingleObject(&(fdoExtension->EjectSynchronizationEvent),
                                        UserRequest,
                                        KernelMode,
                                        FALSE,
                                        NULL);

            if (fdoExtension->ProtectedLockCount != 0) {

                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,  "ClassDeviceControl: call to eject protected locked "
                                                                    "device - failure\n"));

                status = STATUS_DEVICE_BUSY;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;

                FREE_POOL(srb);

                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

                KeSetEvent(&fdoExtension->EjectSynchronizationEvent,
                           IO_NO_INCREMENT,
                           FALSE);
                KeLeaveCriticalRegion();

                goto SetStatusAndReturn;
            }

            SrbSetCdbLength(srb, 6);
            cdb = SrbGetCdb(srb);

            cdb->START_STOP.OperationCode = SCSIOP_START_STOP_UNIT;
            cdb->START_STOP.LoadEject = 1;

            if (modifiedIoControlCode == IOCTL_STORAGE_EJECT_MEDIA) {
                cdb->START_STOP.Start = 0;
            } else {
                cdb->START_STOP.Start = 1;
            }

            //
            // Set timeout value.
            //

            SrbSetTimeOutValue(srb, fdoExtension->TimeOutValue);
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

            FREE_POOL(srb);

            if (commonExtension->IsFdo) {

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

            FREE_POOL(srb);

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength >=
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


        case IOCTL_STORAGE_READ_CAPACITY: {

            FREE_POOL(srb);

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(STORAGE_READ_CAPACITY)) {

                //
                // Indicate unsuccessful status and no data transferred.
                //

                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                Irp->IoStatus.Information = sizeof(STORAGE_READ_CAPACITY);

                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            if (!commonExtension->IsFdo) {


                //
                // Just forward this down and return
                //

                IoCopyCurrentIrpStackLocationToNext(Irp);

                ClassReleaseRemoveLock(DeviceObject, Irp);
                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
            }
            else {

                PFUNCTIONAL_DEVICE_EXTENSION fdoExt = DeviceObject->DeviceExtension;
                PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
                PSTORAGE_READ_CAPACITY readCapacity = Irp->AssociatedIrp.SystemBuffer;
                LARGE_INTEGER diskLength;

                status = ClassReadDriveCapacity(DeviceObject);
                if (NT_SUCCESS(status) && fdoData->IsCachedDriveCapDataValid) {

                    readCapacity->Version = sizeof(STORAGE_READ_CAPACITY);
                    readCapacity->Size = sizeof(STORAGE_READ_CAPACITY);

                    REVERSE_BYTES(&readCapacity->BlockLength,
                                        &fdoData->LastKnownDriveCapacityData.BytesPerBlock);
                    REVERSE_BYTES_QUAD(&readCapacity->NumberOfBlocks,
                                       &fdoData->LastKnownDriveCapacityData.LogicalBlockAddress);
                    readCapacity->NumberOfBlocks.QuadPart++;

                    readCapacity->DiskLength = fdoExt->CommonExtension.PartitionLength;

                    //
                    // Make sure the lengths are equal.
                    // Remove this after testing.
                    //
                    diskLength.QuadPart = readCapacity->NumberOfBlocks.QuadPart *
                                            readCapacity->BlockLength;

                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    Irp->IoStatus.Information = sizeof(STORAGE_READ_CAPACITY);

                } else {
                    //
                    // Read capacity request failed.
                    //
                    TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_IOCTL,  "ClassDeviceControl: ClassReadDriveCapacity failed: 0x%X IsCachedDriveCapDataValid: %d\n",
                                   status, fdoData->IsCachedDriveCapDataValid));
                    Irp->IoStatus.Status = status;
                    Irp->IoStatus.Information = 0;
                }
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
            }

            break;
        }

        case IOCTL_STORAGE_QUERY_PROPERTY: {

            PSTORAGE_PROPERTY_QUERY query = Irp->AssociatedIrp.SystemBuffer;

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(STORAGE_PROPERTY_QUERY)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
                Irp->IoStatus.Status = status;
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
                FREE_POOL(srb);
                break;
            }

            if (!commonExtension->IsFdo) {


                IoCopyCurrentIrpStackLocationToNext(Irp);

                ClassReleaseRemoveLock(DeviceObject, Irp);
                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                FREE_POOL(srb);
                break;
            }

            //
            // Determine PropertyId type and either call appropriate routine
            // or pass request to lower drivers.
            //

            switch ( query->PropertyId ) {

                case StorageDeviceUniqueIdProperty: {

                    status = ClasspDuidQueryProperty(DeviceObject, Irp);
                    break;
                }

                case StorageDeviceWriteCacheProperty: {

                    status = ClasspWriteCacheProperty(DeviceObject, Irp, srb);
                    break;
                }

                // these propertyId has been implemented in some port driver and filter drivers.
                // to keep the backwards compatibility, classpnp will send the request down if it's supported by lower layer.
                // otherwise, classpnp sends SCSI command and then interprets the result.
                case StorageAccessAlignmentProperty: {

                    status = ClasspAccessAlignmentProperty(DeviceObject, Irp, srb);
                    break;
                }

                case StorageDeviceSeekPenaltyProperty: {

                    status = ClasspDeviceSeekPenaltyProperty(DeviceObject, Irp, srb);
                    break;
                }

                case StorageDeviceTrimProperty: {

                    status = ClasspDeviceTrimProperty(DeviceObject, Irp, srb);
                    break;
                }

                case StorageDeviceLBProvisioningProperty: {

                    status = ClasspDeviceLBProvisioningProperty(DeviceObject, Irp, srb);
                    break;
                }

                case StorageDeviceCopyOffloadProperty: {

                    status = ClasspDeviceCopyOffloadProperty(DeviceObject, Irp, srb);
                    break;
                }

                case StorageDeviceMediumProductType: {

                    status = ClasspDeviceMediaTypeProperty(DeviceObject, Irp, srb);
                    break;
                }

                default: {

                    //
                    // Copy the Irp stack parameters to the next stack location.
                    //

                    IoCopyCurrentIrpStackLocationToNext(Irp);

                    ClassReleaseRemoveLock(DeviceObject, Irp);
                    status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                    break;
                }
            }   // end switch

            FREE_POOL(srb);
            break;
        }

        case IOCTL_STORAGE_CHECK_PRIORITY_HINT_SUPPORT: {

            FREE_POOL(srb);

            if (!commonExtension->IsFdo) {


                IoCopyCurrentIrpStackLocationToNext(Irp);

                ClassReleaseRemoveLock(DeviceObject, Irp);
                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                break;
            }

            //
            // Process priority hit request
            //

            status = ClasspPriorityHint(DeviceObject, Irp);
            break;
        }

        case IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES: {

            PDEVICE_MANAGE_DATA_SET_ATTRIBUTES dsmAttributes = Irp->AssociatedIrp.SystemBuffer;

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES)) {

                status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Status = status;
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
                FREE_POOL(srb);
                break;
            }

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                (sizeof(DEVICE_MANAGE_DATA_SET_ATTRIBUTES) + dsmAttributes->ParameterBlockLength + dsmAttributes->DataSetRangesLength)) {

                status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Status = status;
                ClassReleaseRemoveLock(DeviceObject, Irp);
                ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);
                FREE_POOL(srb);
                break;
            }

            if (!commonExtension->IsFdo) {


                IoCopyCurrentIrpStackLocationToNext(Irp);

                ClassReleaseRemoveLock(DeviceObject, Irp);
                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                FREE_POOL(srb);
                break;
            }

            switch(dsmAttributes->Action) {

                // only process Trim action in class layer if possible.
                case DeviceDsmAction_Trim: {
                    status = ClasspDeviceTrimProcess(DeviceObject, Irp, &activityId, srb);
                    break;
                }

                case DeviceDsmAction_OffloadRead: {
                    status = ClassDeviceProcessOffloadRead(DeviceObject, Irp, srb);
                    break;
                }

                case DeviceDsmAction_OffloadWrite: {
                    status = ClassDeviceProcessOffloadWrite(DeviceObject, Irp, srb);
                    break;
                }

                case DeviceDsmAction_Allocation: {
                    status = ClasspDeviceGetLBAStatus(DeviceObject, Irp, srb);
                    break;
                }


                default: {


                    //
                    // Copy the Irp stack parameters to the next stack location.
                    //

                    IoCopyCurrentIrpStackLocationToNext(Irp);

                    ClassReleaseRemoveLock(DeviceObject, Irp);
                    status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                    break;
                }
            }   // end switch

            FREE_POOL(srb);
            break;
        }

        case IOCTL_STORAGE_GET_LB_PROVISIONING_MAP_RESOURCES: {

            if (commonExtension->IsFdo) {

                status = ClassDeviceGetLBProvisioningResources(DeviceObject, Irp, srb);

            } else {

                IoCopyCurrentIrpStackLocationToNext(Irp);

                ClassReleaseRemoveLock(DeviceObject, Irp);
                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
            }

            FREE_POOL(srb);

            break;
        }

        case IOCTL_STORAGE_EVENT_NOTIFICATION: {

            FREE_POOL(srb);

            status = ClasspStorageEventNotification(DeviceObject, Irp);
            break;
        }

#if (NTDDI_VERSION >= NTDDI_WINTRHESHOLD)
        case IOCTL_STORAGE_FIRMWARE_GET_INFO: {
            FREE_POOL(srb);

            status = ClassDeviceHwFirmwareGetInfoProcess(DeviceObject, Irp);
            break;
        }

        case IOCTL_STORAGE_FIRMWARE_DOWNLOAD: {
            if (!commonExtension->IsFdo) {

                IoCopyCurrentIrpStackLocationToNext(Irp);

                ClassReleaseRemoveLock(DeviceObject, Irp);
                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                goto SetStatusAndReturn;
            }

            status = ClassDeviceHwFirmwareDownloadProcess(DeviceObject, Irp, srb);
            break;
        }

        case IOCTL_STORAGE_FIRMWARE_ACTIVATE: {
            if (!commonExtension->IsFdo) {

                IoCopyCurrentIrpStackLocationToNext(Irp);

                ClassReleaseRemoveLock(DeviceObject, Irp);
                status = IoCallDriver(commonExtension->LowerDeviceObject, Irp);
                goto SetStatusAndReturn;
            }

            status = ClassDeviceHwFirmwareActivateProcess(DeviceObject, Irp, srb);
            break;
        }
#endif


        default: {

            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL, "IoDeviceControl: Unsupported device IOCTL %x for %p\n",
                        controlCode, DeviceObject));


            //
            // Pass the device control to the next driver.
            //

            FREE_POOL(srb);

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

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "< ioctl %xh (%s): status %xh.", modifiedIoControlCode, DBGGETIOCTLSTR(modifiedIoControlCode), status));

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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassShutdownFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;

    ULONG isRemoved;

    isRemoved = ClassAcquireRemoveLock(DeviceObject, Irp);
    _Analysis_assume_(isRemoved);
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

ClasspIsPortable()

Routine Description:

    This routine is called during start device to determine whether the PDO
    for a stack reports itself as portable.

Arguments:

    FdoExtension - Pointer to FDO whose PDO we check for portability.

    IsPortable - Boolean pointer in which to store result.

Return Value:

    NT Status

--*/
NTSTATUS
ClasspIsPortable(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION   FdoExtension,
    _Out_ PBOOLEAN                      IsPortable
    )
{
    DEVPROP_BOOLEAN            isInternal = DEVPROP_FALSE;
    BOOLEAN                    isPortable = FALSE;
    ULONG                      size       = 0;
    NTSTATUS                   status     = STATUS_SUCCESS;
    DEVPROPTYPE                type       = DEVPROP_TYPE_EMPTY;

    PAGED_CODE();

    *IsPortable = FALSE;

    //
    // Check to see if the underlying device
    // object is in local machine container
    //

    status = IoGetDevicePropertyData(FdoExtension->LowerPdo,
                                     &DEVPKEY_Device_InLocalMachineContainer,
                                     0,
                                     0,
                                     sizeof(isInternal),
                                     &isInternal,
                                     &size,
                                     &type);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    NT_ASSERT(size == sizeof(isInternal));
    NT_ASSERT(type == DEVPROP_TYPE_BOOLEAN);

    //
    // Volume is hot-pluggable if the disk pdo
    // container id differs from that of root device
    //

    if (isInternal == DEVPROP_TRUE) {
        goto cleanup;
    }

    isPortable = TRUE;

    //
    // Examine the  bus type to  ensure
    // that this really is a fixed disk
    //

    if (FdoExtension->DeviceDescriptor->BusType == BusTypeFibre ||
        FdoExtension->DeviceDescriptor->BusType == BusTypeiScsi ||
        FdoExtension->DeviceDescriptor->BusType == BusTypeRAID) {

        isPortable = FALSE;
    }

    *IsPortable = isPortable;

cleanup:

    return status;
}

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

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Post_satisfies_(return <= 0)
SCSIPORT_API
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassCreateDeviceObject(
    _In_ PDRIVER_OBJECT     DriverObject,
    _In_z_ PCCHAR           ObjectNameBuffer,
    _In_ PDEVICE_OBJECT     LowerDevice,
    _In_ BOOLEAN            IsFdo,
    _Outptr_result_nullonfailure_
    _At_(*DeviceObject, __drv_allocatesMem(Mem) __drv_aliasesMem)
    PDEVICE_OBJECT          *DeviceObject
    )
{
    BOOLEAN        isPartitionable;
    STRING         ntNameString;
    UNICODE_STRING ntUnicodeString;
    NTSTATUS       status;
    PDEVICE_OBJECT deviceObject = NULL;

    ULONG          characteristics;
    SIZE_T         rundownSize = ExSizeOfRundownProtectionCacheAware();
    PCHAR          rundownAddr = NULL;
    ULONG          devExtSize;

#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer as data pointer for this use case
#endif
    PCLASS_DRIVER_EXTENSION driverExtension = IoGetDriverObjectExtension(DriverObject, CLASS_DRIVER_EXTENSION_KEY);

    PCLASS_DEV_INFO devInfo;

    PAGED_CODE();

    _Analysis_assume_(driverExtension != NULL);

    *DeviceObject = NULL;
    RtlInitUnicodeString(&ntUnicodeString, NULL);

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,  "ClassCreateFdo: Create device object\n"));

    NT_ASSERT(LowerDevice);

    //
    // Make sure that if we're making PDO's we have an enumeration routine
    //

    isPartitionable = (driverExtension->InitData.ClassEnumerateDevice != NULL);

    NT_ASSERT(IsFdo || isPartitionable);

    //
    // Grab the correct dev-info structure out of the init data
    //

    if (IsFdo) {
        devInfo = &(driverExtension->InitData.FdoData);
    } else {
        devInfo = &(driverExtension->InitData.PdoData);
    }

    characteristics = devInfo->DeviceCharacteristics;

    if (ARGUMENT_PRESENT(ObjectNameBuffer)) {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,  "ClassCreateFdo: Name is %s\n", ObjectNameBuffer));

        RtlInitString(&ntNameString, ObjectNameBuffer);

        status = RtlAnsiStringToUnicodeString(&ntUnicodeString, &ntNameString, TRUE);

        if (!NT_SUCCESS(status)) {

            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                        "ClassCreateFdo: Cannot convert string %s\n",
                        ObjectNameBuffer));

            ntUnicodeString.Buffer = NULL;
            return status;
        }
    } else {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,  "ClassCreateFdo: Object will be unnamed\n"));

        if (IsFdo == FALSE) {

            //
            // PDO's have to have some sort of name.
            //

            SET_FLAG(characteristics, FILE_AUTOGENERATED_DEVICE_NAME);
        }

        RtlInitUnicodeString(&ntUnicodeString, NULL);
    }

    devExtSize = devInfo->DeviceExtensionSize +
        (ULONG)sizeof(CLASS_PRIVATE_COMMON_DATA) + (ULONG)rundownSize;
    status = IoCreateDevice(DriverObject,
                            devExtSize,
                            &ntUnicodeString,
                            devInfo->DeviceType,
                            characteristics,
                            FALSE,
                            &deviceObject);

    if (!NT_SUCCESS(status)) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,  "ClassCreateFdo: Can not create device object %lx\n",
                    status));
        NT_ASSERT(deviceObject == NULL);

        //
        // buffer is not used any longer here.
        //

        if (ntUnicodeString.Buffer != NULL) {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,  "ClassCreateFdo: Freeing unicode name buffer\n"));
            FREE_POOL(ntUnicodeString.Buffer);
            RtlInitUnicodeString(&ntUnicodeString, NULL);
        }

    } else {

        PCOMMON_DEVICE_EXTENSION commonExtension = deviceObject->DeviceExtension;

        RtlZeroMemory(
            deviceObject->DeviceExtension,
            devExtSize);

        //
        // Setup version code
        //

        commonExtension->Version = 0x03;

        //
        // Setup the remove lock and event
        //

        commonExtension->IsRemoved = NO_REMOVE;

#if DBG

        commonExtension->RemoveLock = 0;

#endif

        KeInitializeEvent(&commonExtension->RemoveEvent,
                          SynchronizationEvent,
                          FALSE);


        ClasspInitializeRemoveTracking(deviceObject);

        //
        // Initialize the PrivateCommonData
        //

        commonExtension->PrivateCommonData = (PCLASS_PRIVATE_COMMON_DATA)
            ((PCHAR)deviceObject->DeviceExtension + devInfo->DeviceExtensionSize);
        rundownAddr = (PCHAR)commonExtension->PrivateCommonData + sizeof(CLASS_PRIVATE_COMMON_DATA);
        ExInitializeRundownProtectionCacheAware((PEX_RUNDOWN_REF_CACHE_AWARE)rundownAddr, rundownSize);
        commonExtension->PrivateCommonData->RemoveLockFailAcquire = 0;

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

        commonExtension->DispatchTable = driverExtension->DeviceMajorFunctionTable;

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

            //
            // The disk class driver creates only FDO. The partition number
            // for the FDO must be 0.
            //

            if ((isPartitionable == TRUE) ||
                (devInfo->DeviceType == FILE_DEVICE_DISK)) {

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
                    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,  "ClassCreateFdo: Freeing unicode name buffer\n"));
                    FREE_POOL(ntUnicodeString.Buffer);
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

        if (commonExtension->DriverExtension->InitData.ClassStartIo) {
            IoSetStartIoAttributes(deviceObject, TRUE, TRUE);
        }
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
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassClaimDevice(
    _In_ PDEVICE_OBJECT LowerDeviceObject,
    _In_ BOOLEAN Release
    )
{
    IO_STATUS_BLOCK    ioStatus;
    PIRP               irp;
    PIO_STACK_LOCATION irpStack;
    KEVENT             event;
    NTSTATUS           status;
    SCSI_REQUEST_BLOCK srb = {0};

    PAGED_CODE();

    //
    // WORK ITEM - MPIO related. Need to think about how to handle.
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
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,  "ClassClaimDevice: Can't allocate Irp\n"));
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

        (VOID)KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
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

    NT_ASSERT(srb.DataBuffer != NULL);
    NT_ASSERT(!TEST_FLAG(srb.SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));

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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
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
    _Analysis_assume_(isRemoved);
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
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassQueryTimeOutRegistryValue(
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
    //
    // Find the appropriate reg. key
    //

#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer as data pointer for this use case
#endif
    PCLASS_DRIVER_EXTENSION driverExtension = IoGetDriverObjectExtension(DeviceObject->DriverObject, CLASS_DRIVER_EXTENSION_KEY);

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

    parameters = ExAllocatePoolWithTag(NonPagedPoolNx,
                                sizeof(RTL_QUERY_REGISTRY_TABLE)*2,
                                '1BcS');

    if (!parameters) {
        return 0;
    }

    size = registryPath->MaximumLength + sizeof(WCHAR);
    path = ExAllocatePoolWithTag(NonPagedPoolNx, size, '2BcS');

    if (!path) {
        FREE_POOL(parameters);
        return 0;
    }

    RtlZeroMemory(path,size);
    RtlCopyMemory(path, registryPath->Buffer, size - sizeof(WCHAR));


    //
    // Check for the Timeout value.
    //

    RtlZeroMemory(parameters,
                  (sizeof(RTL_QUERY_REGISTRY_TABLE)*2));

    parameters[0].Flags         = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    parameters[0].Name          = L"TimeOutValue";
    parameters[0].EntryContext  = &timeOut;
    parameters[0].DefaultType   = (REG_DWORD << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_DWORD;
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

    FREE_POOL(parameters);
    FREE_POOL(path);

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT,
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassCheckVerifyComplete(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;

    PIRP originalIrp;

    UNREFERENCED_PARAMETER(Context);

    ASSERT_FDO(Fdo);

    originalIrp = irpStack->Parameters.Others.Argument1;

    //
    // Copy the media change count and status
    //

    *((PULONG) (originalIrp->AssociatedIrp.SystemBuffer)) =
        fdoExtension->MediaChangeCount;

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_IOCTL, "ClassCheckVerifyComplete - Media change count for"
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
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassGetDescriptor(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PSTORAGE_PROPERTY_ID PropertyId,
    _Outptr_ PVOID *Descriptor
    )
{
    STORAGE_PROPERTY_QUERY query = {0};
    IO_STATUS_BLOCK ioStatus;

    PSTORAGE_DESCRIPTOR_HEADER descriptor = NULL;
    ULONG length;

    PAGED_CODE();

    //
    // Set the passed-in descriptor pointer to NULL as default
    //

    *Descriptor = NULL;

    query.PropertyId = *PropertyId;
    query.QueryType = PropertyStandardQuery;

    //
    // On the first pass we just want to get the first few
    // bytes of the descriptor so we can read it's size
    //

    descriptor = (PVOID)&query;

    NT_ASSERT(sizeof(STORAGE_PROPERTY_QUERY) >= (sizeof(ULONG)*2));

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

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,  "ClassGetDescriptor: error %lx trying to "
                       "query properties #1\n", ioStatus.Status));
        return ioStatus.Status;
    }

    if (descriptor->Size == 0) {

        //
        // This DebugPrint is to help third-party driver writers
        //

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,  "ClassGetDescriptor: size returned was zero?! (status "
                    "%x\n", ioStatus.Status));
        return STATUS_UNSUCCESSFUL;

    }

    //
    // This time we know how much data there is so we can
    // allocate a buffer of the correct size
    //

    length = descriptor->Size;
    NT_ASSERT(length >= sizeof(STORAGE_PROPERTY_QUERY));
    length = max(length, sizeof(STORAGE_PROPERTY_QUERY));

    descriptor = ExAllocatePoolWithTag(NonPagedPoolNx, length, '4BcS');

    if(descriptor == NULL) {

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,  "ClassGetDescriptor: unable to memory for descriptor "
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

    RtlZeroMemory(descriptor, length);

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

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_INIT,  "ClassGetDescriptor: error %lx trying to "
                       "query properties #1\n", ioStatus.Status));
        FREE_POOL(descriptor);
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassSignalCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PKEVENT Event = (PKEVENT)Context;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    if (Context == NULL) {
        NT_ASSERT(Context != NULL);
        return STATUS_INVALID_PARAMETER;
    }

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
ClassPnpQueryFdoRelations(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer as data pointer for this use case
#endif
    PCLASS_DRIVER_EXTENSION driverExtension = IoGetDriverObjectExtension(Fdo->DriverObject, CLASS_DRIVER_EXTENSION_KEY);

    PAGED_CODE();

    _Analysis_assume_(driverExtension != NULL);

    //
    // If there's already an enumeration in progress then don't start another
    // one.
    //

    if(InterlockedIncrement((volatile LONG *)&(fdoExtension->EnumerationInterlock)) == 1) {
        driverExtension->InitData.ClassEnumerateDevice(Fdo);
    }

    Irp->IoStatus.Status = ClassRetrieveDeviceRelations(
                                Fdo,
                                BusRelations,
                                (PDEVICE_RELATIONS *) &Irp->IoStatus.Information);
    InterlockedDecrement((volatile LONG *)&(fdoExtension->EnumerationInterlock));

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
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassMarkChildrenMissing(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION Fdo
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
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassMarkChildMissing(
    _In_ PPHYSICAL_DEVICE_EXTENSION Child,
    _In_ BOOLEAN AcquireChildLock
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
        PCOMMON_DEVICE_EXTENSION commonExtension = Child->DeviceObject->DeviceExtension;
        commonExtension->IsRemoved = REMOVE_PENDING;
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
    PDEVICE_RELATIONS oldRelations = *DeviceRelations;

    NTSTATUS status;

    UNREFERENCED_PARAMETER(RelationType);

    PAGED_CODE();

    ClassAcquireChildLock(fdoExtension);

    nextChild = fdoExtension->CommonExtension.ChildList;

    //
    // Count the number of PDO's attached to this disk
    //

    while (nextChild != NULL) {
        PCOMMON_DEVICE_EXTENSION commonExtension;

        commonExtension = &(nextChild->CommonExtension);

        NT_ASSERTMSG("ClassPnp internal error: missing child on active list\n",
                  (nextChild->IsMissing == FALSE));

        nextChild = commonExtension->ChildList;

        count++;
    };

    //
    // If relations already exist in the QDR, adjust the current count
    // to include the previous list.
    //

    if (oldRelations) {
        count += oldRelations->Count;
    }

    relationsSize = (sizeof(DEVICE_RELATIONS) +
                     (count * sizeof(PDEVICE_OBJECT)));

    deviceRelations = ExAllocatePoolWithTag(PagedPool, relationsSize, '5BcS');

    if (deviceRelations == NULL) {

        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_ENUM,  "ClassRetrieveDeviceRelations: unable to allocate "
                       "%d bytes for device relations\n", relationsSize));

        ClassReleaseChildLock(fdoExtension);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(deviceRelations, relationsSize);

    if (oldRelations) {

        //
        // Copy the old relations to the new list and free the old list.
        //

        for (i = 0; i < oldRelations->Count; i++) {
            deviceRelations->Objects[i] = oldRelations->Objects[i];
        }

        FREE_POOL(oldRelations);
    }

    nextChild = fdoExtension->CommonExtension.ChildList;
    i = count;

    while (nextChild != NULL) {
        PCOMMON_DEVICE_EXTENSION commonExtension;

        commonExtension = &(nextChild->CommonExtension);

        NT_ASSERTMSG("ClassPnp internal error: missing child on active list\n",
                  (nextChild->IsMissing == FALSE));

    _Analysis_assume_(i >= 1);
    deviceRelations->Objects[--i] = nextChild->DeviceObject;

        status = ObReferenceObjectByPointer(
                    nextChild->DeviceObject,
                    0,
                    NULL,
                    KernelMode);
        if (!NT_SUCCESS(status)) {
            NT_ASSERT(!"Error referencing child device by pointer");
            TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_ENUM,
                        "ClassRetrieveDeviceRelations: Error referencing child "
                        "device %p by pointer\n", nextChild->DeviceObject));

        }
        nextChild->IsEnumerated = TRUE;
        nextChild = commonExtension->ChildList;
    }

    NT_ASSERTMSG("Child list has changed: ", i == 0);

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
ClassGetPdoId(
    IN PDEVICE_OBJECT Pdo,
    IN BUS_QUERY_ID_TYPE IdType,
    IN PUNICODE_STRING IdString
    )
{
#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer as data pointer for this use case
#endif
    PCLASS_DRIVER_EXTENSION driverExtension = IoGetDriverObjectExtension(Pdo->DriverObject, CLASS_DRIVER_EXTENSION_KEY);

    _Analysis_assume_(driverExtension != NULL);

    ASSERT_PDO(Pdo);
    NT_ASSERT(driverExtension->InitData.ClassQueryId);

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

    NT_ASSERT(DeviceObject);
    NT_ASSERT(Capabilities);

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
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassInvalidateBusRelations(
    _In_ PDEVICE_OBJECT Fdo
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer as data pointer for this use case
#endif
    PCLASS_DRIVER_EXTENSION driverExtension = IoGetDriverObjectExtension(Fdo->DriverObject, CLASS_DRIVER_EXTENSION_KEY);

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    _Analysis_assume_(driverExtension != NULL);

    ASSERT_FDO(Fdo);
    NT_ASSERT(driverExtension->InitData.ClassEnumerateDevice != NULL);

    if(InterlockedIncrement((volatile LONG *)&(fdoExtension->EnumerationInterlock)) == 1) {
        status = driverExtension->InitData.ClassEnumerateDevice(Fdo);
    }
    InterlockedDecrement((volatile LONG *)&(fdoExtension->EnumerationInterlock));

    if(!NT_SUCCESS(status)) {

        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_ENUM,  "ClassInvalidateBusRelations: EnumerateDevice routine "
                       "returned %lx\n", status));
    }

    IoInvalidateDeviceRelations(fdoExtension->LowerPdo, BusRelations);

    return;
} // end ClassInvalidateBusRelations()

/*++////////////////////////////////////////////////////////////////////////////

ClassRemoveDevice() ISSUE-2000/02/18-henrygab - why public?!

Routine Description:

    This routine is called to handle the "removal" of a device.  It will
    forward the request downwards if necesssary, call into the driver
    to release any necessary resources (memory, events, etc) and then
    will delete the device object.

Arguments:

    DeviceObject - a pointer to the device object being removed

    RemoveType - indicates what type of remove this is (regular or surprise).

Return Value:

    status

--*/
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassRemoveDevice(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR RemoveType
    )
{
#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer as data pointer for this use case
#endif
    PCLASS_DRIVER_EXTENSION driverExtension = IoGetDriverObjectExtension(DeviceObject->DriverObject, CLASS_DRIVER_EXTENSION_KEY);
    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT lowerDeviceObject = commonExtension->LowerDeviceObject;
    BOOLEAN proceedWithRemove = TRUE;
    NTSTATUS status;
    PEX_RUNDOWN_REF_CACHE_AWARE removeLockRundown = NULL;


    PAGED_CODE();

    _Analysis_assume_(driverExtension != NULL);

    /*
     *  Deregister from WMI.
     */
    if (commonExtension->IsFdo ||
        driverExtension->InitData.PdoData.ClassWmiInfo.GuidRegInfo) {
        status = IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_DEREGISTER);
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_WMI, "ClassRemoveDevice: IoWMIRegistrationControl(%p, WMI_ACTION_DEREGISTER) --> %lx", DeviceObject, status));
    }

    /*
     *  If we exposed a "shingle" (a named device interface openable by CreateFile)
     *  then delete it now.
     */
    if (commonExtension->MountedDeviceInterfaceName.Buffer){
        (VOID)IoSetDeviceInterfaceState(&commonExtension->MountedDeviceInterfaceName, FALSE);
        RtlFreeUnicodeString(&commonExtension->MountedDeviceInterfaceName);
        RtlInitUnicodeString(&commonExtension->MountedDeviceInterfaceName, NULL);
    }

    //
    // If this is a surprise removal we leave the device around - which means
    // we don't have to (or want to) drop the remove lock and wait for pending
    // requests to complete.
    //

    if (RemoveType == IRP_MN_REMOVE_DEVICE) {

        //
        // Release the lock we acquired when the device object was created.
        //

        ClassReleaseRemoveLock(DeviceObject, (PIRP) DeviceObject);

        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_PNP,  "ClasspRemoveDevice - Reference count is now %d\n",
                    commonExtension->RemoveLock));

        //
        // The RemoveLockRundown allows fast protection of the device object
        // structure that is torn down by a single thread. While in the rundown process (the call to
        // ExWaitForRundownProtectionReleaseCacheAware returns), the rundown object becomes
        // invalid and the subsequent calls to ExAcquireRundownProtectionCacheAware will return FALSE.
        // ExReInitializeRundownProtectionCacheAware needs to be called to re-initialize the
        // RemoveLockRundown protection.
        //

        removeLockRundown = (PEX_RUNDOWN_REF_CACHE_AWARE)
            ((PCHAR)commonExtension->PrivateCommonData +
             sizeof(CLASS_PRIVATE_COMMON_DATA));
        ExWaitForRundownProtectionReleaseCacheAware(removeLockRundown);

        KeSetEvent(&commonExtension->RemoveEvent,
                   IO_NO_INCREMENT,
                   FALSE);

        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_PNP,  "ClasspRemoveDevice - removing device %p\n", DeviceObject));

        if (commonExtension->IsFdo) {

            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_PNP,  "ClasspRemoveDevice - FDO %p has received a "
                           "remove request.\n", DeviceObject));

        } else {
            PPHYSICAL_DEVICE_EXTENSION pdoExtension = DeviceObject->DeviceExtension;

            if (pdoExtension->IsMissing) {
                /*
                 *  The child partition PDO is missing, so we are going to go ahead
                 *  and delete it for the remove.
                 */
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_PNP, "ClasspRemoveDevice - PDO %p is missing and will be removed", DeviceObject));
            } else {
                /*
                 *  We got a remove for a child partition PDO which is not actually missing.
                 *  So we will NOT actually delete it.
                 */
                TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_PNP, "ClasspRemoveDevice - PDO %p still exists and will be removed when it disappears", DeviceObject));

                ExReInitializeRundownProtectionCacheAware(removeLockRundown);

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


    if (proceedWithRemove) {

        /*
         *  Call the class driver's remove handler.
         *  All this is supposed to do is clean up its data and device interfaces.
         */
        NT_ASSERT(commonExtension->DevInfo->ClassRemoveDevice);
        status = commonExtension->DevInfo->ClassRemoveDevice(DeviceObject, RemoveType);
        NT_ASSERT(NT_SUCCESS(status));
        status = STATUS_SUCCESS;
        UNREFERENCED_PARAMETER(status); // disables prefast warning; defensive coding...

        if (commonExtension->IsFdo) {
            PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;

            ClasspDisableTimer(fdoExtension);

            if (RemoveType == IRP_MN_REMOVE_DEVICE) {

                PPHYSICAL_DEVICE_EXTENSION child;

                //
                // If this FDO is idle power managed, remove it from the
                // list of idle power managed FDOs.
                //
                if (fdoExtension->FunctionSupportInfo &&
                    fdoExtension->FunctionSupportInfo->IdlePower.IdlePowerEnabled) {
                    PIDLE_POWER_FDO_LIST_ENTRY fdoEntry;

                    KeAcquireGuardedMutex(&IdlePowerFDOListMutex);
                    fdoEntry = (PIDLE_POWER_FDO_LIST_ENTRY)IdlePowerFDOList.Flink;
                    while ((PLIST_ENTRY)fdoEntry != &IdlePowerFDOList) {
                        PIDLE_POWER_FDO_LIST_ENTRY nextEntry = (PIDLE_POWER_FDO_LIST_ENTRY)fdoEntry->ListEntry.Flink;
                        if (fdoEntry->Fdo == DeviceObject) {
                            RemoveEntryList(&(fdoEntry->ListEntry));
                            ExFreePool(fdoEntry);
                            break;
                        }
                        fdoEntry = nextEntry;
                    }
                    KeReleaseGuardedMutex(&IdlePowerFDOListMutex);
                }

                //
                // Cleanup the media detection resources now that the class driver
                // has stopped it's timer (if any) and we can be sure they won't
                // call us to do detection again.
                //

                ClassCleanupMediaChangeDetection(fdoExtension);

                //
                // Cleanup any Failure Prediction stuff
                //
                FREE_POOL(fdoExtension->FailurePredictionInfo);

                /*
                 *  Ordinarily all child PDOs will be removed by the time
                 *  that the parent gets the REMOVE_DEVICE.
                 *  However, if a child PDO has been created but has not
                 *  been announced in a QueryDeviceRelations, then it is
                 *  just a private data structure unknown to pnp, and we have
                 *  to delete it ourselves.
                 */
                ClassAcquireChildLock(fdoExtension);
                child = ClassRemoveChild(fdoExtension, NULL, FALSE);
                while (child) {
                    PCOMMON_DEVICE_EXTENSION childCommonExtension = child->DeviceObject->DeviceExtension;

                    //
                    // Yank the pdo.  This routine will unlink the device from the
                    // pdo list so NextPdo will point to the next one when it's
                    // complete.
                    //
                    child->IsMissing = TRUE;
                    childCommonExtension->IsRemoved = REMOVE_PENDING;
                    ClassRemoveDevice(child->DeviceObject, IRP_MN_REMOVE_DEVICE);
                    child = ClassRemoveChild(fdoExtension, NULL, FALSE);
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

            if (RemoveType == IRP_MN_REMOVE_DEVICE) {

                ClasspFreeReleaseRequest(DeviceObject);

                //
                // Free FDO-specific data structs
                //
                if (fdoExtension->PrivateFdoData) {
                    //
                    // Only remove the entry if the list has been initialized, or
                    // else we will access invalid memory.
                    //
                    PLIST_ENTRY allFdosListEntry = &fdoExtension->PrivateFdoData->AllFdosListEntry;
                    if (allFdosListEntry->Flink && allFdosListEntry->Blink) {
                        //
                        //  Remove the FDO from the static list.
                        //  Pnp is synchronized so this shouldn't need any synchronization.
                        //
                        RemoveEntryList(allFdosListEntry);
                    }
                    InitializeListHead(allFdosListEntry);

                    DestroyAllTransferPackets(DeviceObject);

                    //
                    // Delete the tick timer now.
                    //
                    ClasspDeleteTimer(fdoExtension);


                    FREE_POOL(fdoExtension->PrivateFdoData->PowerProcessIrp);
                    FREE_POOL(fdoExtension->PrivateFdoData->FreeTransferPacketsLists);
                    FREE_POOL(fdoExtension->PrivateFdoData);
                }

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
                FREE_POOL(fdoExtension->AdditionalFdoData);
#endif

                if (commonExtension->DeviceName.Buffer) {
                    FREE_POOL(commonExtension->DeviceName.Buffer);
                    RtlInitUnicodeString(&commonExtension->DeviceName, NULL);
                }

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
                if (fdoExtension->FunctionSupportInfo != NULL) {
                    FREE_POOL(fdoExtension->FunctionSupportInfo->HwFirmwareInfo);
                }
#endif
                FREE_POOL(fdoExtension->FunctionSupportInfo);

                FREE_POOL(fdoExtension->MiniportDescriptor);

                FREE_POOL(fdoExtension->AdapterDescriptor);

                FREE_POOL(fdoExtension->DeviceDescriptor);

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
            if (RemoveType == IRP_MN_REMOVE_DEVICE) {
                PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = commonExtension->PartitionZeroExtension;
                PPHYSICAL_DEVICE_EXTENSION pdoExtension = (PPHYSICAL_DEVICE_EXTENSION)commonExtension;

                //
                // See if this device is in the child list (if this was a suprise
                // removal it might be) and remove it.
                //
                ClassRemoveChild(fdoExtension, pdoExtension, TRUE);
            }
        }

        commonExtension->PartitionLength.QuadPart = 0;

        if (RemoveType == IRP_MN_REMOVE_DEVICE) {

            ClasspUninitializeRemoveTracking(DeviceObject);

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
__drv_aliasesMem
_IRQL_requires_max_(DISPATCH_LEVEL)
PCLASS_DRIVER_EXTENSION
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassGetDriverExtension(
    _In_ PDRIVER_OBJECT DriverObject
    )
{
#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer as data pointer for this use case
#endif
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
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

#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer to PIRP for this use case
#endif
        ClassAcquireRemoveLock(DeviceObject, (PIRP) ClasspStartIo);

        ClassReleaseRemoveLock(DeviceObject, Irp);
        ClassCompleteRequest(DeviceObject, Irp, IO_DISK_INCREMENT);
        IoStartNextPacket(DeviceObject, TRUE); // Some IO is cancellable

#ifdef _MSC_VER
#pragma warning(suppress:4054) // okay to type cast function pointer to PIRP for this use case
#endif
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
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassUpdateInformationInRegistry(
    _In_ PDEVICE_OBJECT   Fdo,
    _In_ PCHAR            DeviceName,
    _In_ ULONG            DeviceNumber,
    _In_reads_bytes_opt_(InquiryDataLength) PINQUIRYDATA InquiryData,
    _In_ ULONG            InquiryDataLength
    )
{
    NTSTATUS          status;
    SCSI_ADDRESS      scsiAddress = {0};
    OBJECT_ATTRIBUTES objectAttributes = {0};
    STRING            string;
    UNICODE_STRING    unicodeName = {0};
    UNICODE_STRING    unicodeRegistryPath = {0};
    UNICODE_STRING    unicodeData = {0};
    HANDLE            targetKey;
    IO_STATUS_BLOCK   ioStatus;
    UCHAR buffer[256] = {0};

    PAGED_CODE();

    NT_ASSERT(DeviceName);
    targetKey = NULL;

    _SEH2_TRY {

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
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                        "UpdateInformationInRegistry: Get Address failed %lx\n",
                        status));
            _SEH2_LEAVE;

        } else {

            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                        "GetAddress: Port %x, Path %x, Target %x, Lun %x\n",
                        scsiAddress.PortNumber,
                        scsiAddress.PathId,
                        scsiAddress.TargetId,
                        scsiAddress.Lun));

        }

        status = RtlStringCchPrintfA((NTSTRSAFE_PSTR)buffer,
                sizeof(buffer)-1,
                "\\Registry\\Machine\\Hardware\\DeviceMap\\Scsi\\Scsi Port %d\\Scsi Bus %d\\Target Id %d\\Logical Unit Id %d",
                scsiAddress.PortNumber,
                scsiAddress.PathId,
                scsiAddress.TargetId,
                scsiAddress.Lun);

        if (!NT_SUCCESS(status)) {
            _SEH2_LEAVE;
        }

        RtlInitString(&string, (PCSZ)buffer);

        status = RtlAnsiStringToUnicodeString(&unicodeRegistryPath,
                                              &string,
                                              TRUE);

        if (!NT_SUCCESS(status)) {
            _SEH2_LEAVE;
        }

        //
        // Open the registry key for the scsi information for this
        // scsibus, target, lun.
        //

        InitializeObjectAttributes(&objectAttributes,
                                   &unicodeRegistryPath,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);

        status = ZwOpenKey(&targetKey,
                           KEY_READ | KEY_WRITE,
                           &objectAttributes);

        if (!NT_SUCCESS(status)) {
            _SEH2_LEAVE;
        }

        //
        // Now construct and attempt to create the registry value
        // specifying the device name in the appropriate place in the
        // device map.
        //

        RtlInitUnicodeString(&unicodeName, L"DeviceName");

        status = RtlStringCchPrintfA((NTSTRSAFE_PSTR)buffer, sizeof(buffer)-1, "%s%d", DeviceName, DeviceNumber);
        if (!NT_SUCCESS(status)) {
            _SEH2_LEAVE;
        }

        RtlInitString(&string, (PCSZ)buffer);
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

        if (NT_SUCCESS(status) && InquiryDataLength) {

            NT_ASSERT(InquiryData);

            RtlInitUnicodeString(&unicodeName, L"InquiryData");
            status = ZwSetValueKey(targetKey,
                                   &unicodeName,
                                   0,
                                   REG_BINARY,
                                   InquiryData,
                                   InquiryDataLength);
        }

        // that's all, except to clean up.

    } _SEH2_FINALLY {

        if (!NT_SUCCESS(status)) {
            TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT,
                        "Failure to update information in registry: %08x\n",
                        status
                        ));
        }

        if (unicodeData.Buffer) {
            RtlFreeUnicodeString(&unicodeData);
        }
        if (unicodeRegistryPath.Buffer) {
            RtlFreeUnicodeString(&unicodeRegistryPath);
        }
        if (targetKey) {
            ZwClose(targetKey);
        }

    } _SEH2_END;

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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClasspSendSynchronousCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,  "ClasspSendSynchronousCompletion: %p %p %p\n",
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
ClasspRegisterMountedDeviceInterface(
    IN PDEVICE_OBJECT DeviceObject
    )
{

    PCOMMON_DEVICE_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    BOOLEAN isFdo = commonExtension->IsFdo;
    PDEVICE_OBJECT pdo;
    UNICODE_STRING interfaceName;

    NTSTATUS status;

    PAGED_CODE();

    if(isFdo) {

        PFUNCTIONAL_DEVICE_EXTENSION functionalExtension;

        functionalExtension =
            (PFUNCTIONAL_DEVICE_EXTENSION) commonExtension;
        pdo = functionalExtension->LowerPdo;
    } else {
        pdo = DeviceObject;
    }

#ifdef _MSC_VER
#pragma prefast(suppress:6014, "The allocated memory that interfaceName points to will be freed in ClassRemoveDevice().")
#endif
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassSendDeviceIoControlSynchronous(
    _In_ ULONG IoControlCode,
    _In_ PDEVICE_OBJECT TargetDeviceObject,
    _Inout_updates_opt_(_Inexpressible_(max(InputBufferLength, OutputBufferLength))) PVOID Buffer,
    _In_ ULONG InputBufferLength,
    _In_ ULONG OutputBufferLength,
    _In_ BOOLEAN InternalDeviceIoControl,
    _Out_ PIO_STATUS_BLOCK IoStatus
    )
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    ULONG method;

    PAGED_CODE();

    irp = NULL;
    method = IoControlCode & 3;

    #if DBG // Begin Argument Checking (nop in fre version)

    NT_ASSERT(ARGUMENT_PRESENT(IoStatus));

    if ((InputBufferLength != 0) || (OutputBufferLength != 0)) {
        NT_ASSERT(ARGUMENT_PRESENT(Buffer));
    }
    else {
        NT_ASSERT(!ARGUMENT_PRESENT(Buffer));
    }
    #endif

    //
    // Begin by allocating the IRP for this request.  Do not charge quota to
    // the current process for this IRP.
    //

    irp = IoAllocateIrp(TargetDeviceObject->StackSize, FALSE);
    if (!irp) {
        IoStatus->Information = 0;
        IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
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

    switch (method)
    {
        //
        // case 0
        //
        case METHOD_BUFFERED:
        {
            if ((InputBufferLength != 0) || (OutputBufferLength != 0))
            {
                irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                                                        max(InputBufferLength, OutputBufferLength),
                                                                        CLASS_TAG_DEVICE_CONTROL);
                if (irp->AssociatedIrp.SystemBuffer == NULL)
                {
                    IoFreeIrp(irp);

                    IoStatus->Information = 0;
                    IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
                    return;
                }

                if (InputBufferLength != 0)
                {
                    RtlCopyMemory(irp->AssociatedIrp.SystemBuffer, Buffer, InputBufferLength);
                }
            }

            irp->UserBuffer = Buffer;

            break;
        }

        //
        // case 1, case 2
        //
        case METHOD_IN_DIRECT:
        case METHOD_OUT_DIRECT:
        {
            if (InputBufferLength != 0)
            {
                irp->AssociatedIrp.SystemBuffer = Buffer;
            }

            if (OutputBufferLength != 0)
            {
                irp->MdlAddress = IoAllocateMdl(Buffer,
                                                OutputBufferLength,
                                                FALSE,
                                                FALSE,
                                                (PIRP) NULL);
                if (irp->MdlAddress == NULL)
                {
                    IoFreeIrp(irp);

                    IoStatus->Information = 0;
                    IoStatus->Status = STATUS_INSUFFICIENT_RESOURCES;
                    return;
                }

                _SEH2_TRY
                {
                    MmProbeAndLockPages(irp->MdlAddress,
                                        KernelMode,
                                        (method == METHOD_IN_DIRECT) ? IoReadAccess : IoWriteAccess);
                }
#ifdef _MSC_VER
                #pragma warning(suppress: 6320) // We want to handle any exception that MmProbeAndLockPages might throw
#endif
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    IoFreeMdl(irp->MdlAddress);
                    IoFreeIrp(irp);

                    IoStatus->Information = 0;
                    IoStatus->Status = _SEH2_GetExceptionCode();
                    _SEH2_YIELD(return);
                } _SEH2_END;
            }

            break;
        }

        //
        // case 3
        //
        case METHOD_NEITHER:
        {
            NT_ASSERT(!"ClassSendDeviceIoControlSynchronous does not support METHOD_NEITHER Ioctls");

            IoFreeIrp(irp);

            IoStatus->Information = 0;
            IoStatus->Status = STATUS_NOT_SUPPORTED;
            return;
        }
    }

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

            NT_ASSERT(irp->UserBuffer == Buffer);

            //
            // first copy the buffered result, if any
            // Note that there are no security implications in
            // not checking for success since only drivers can
            // call into this routine anyways...
            //

            if (OutputBufferLength != 0) {
#ifdef _MSC_VER
                #pragma warning(suppress: 6386) // Buffer's size is max(InputBufferLength, OutputBufferLength)
#endif
                RtlCopyMemory(Buffer, // irp->UserBuffer
                              irp->AssociatedIrp.SystemBuffer,
                              OutputBufferLength
                              );
            }

            //
            // then free the memory allocated to buffer the io
            //

            if ((InputBufferLength !=0) || (OutputBufferLength != 0)) {
                FREE_POOL(irp->AssociatedIrp.SystemBuffer);
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
                NT_ASSERT(irp->MdlAddress != NULL);
                MmUnlockPages(irp->MdlAddress);
                IoFreeMdl(irp->MdlAddress);
                irp->MdlAddress = (PMDL) NULL;
            }
            break;
        }

        case METHOD_NEITHER: {
            NT_ASSERT(!"Code is out of date");
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassForwardIrpSynchronous(
    _In_ PCOMMON_DEVICE_EXTENSION CommonExtension,
    _In_ PIRP Irp
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassSendIrpSynchronous(
    _In_ PDEVICE_OBJECT TargetDeviceObject,
    _In_ PIRP Irp
    )
{
    KEVENT event;
    NTSTATUS status;

    NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    NT_ASSERT(TargetDeviceObject != NULL);
    NT_ASSERT(Irp != NULL);
    NT_ASSERT(Irp->StackCount >= TargetDeviceObject->StackSize);

    //
    // ISSUE-2000/02/20-henrygab   What if APCs are disabled?
    //    May need to enter critical section before IoCallDriver()
    //    until the event is hit?
    //

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);
    IoSetCompletionRoutine(Irp, ClassSignalCompletion, &event,
                           TRUE, TRUE, TRUE);

    status = IoCallDriver(TargetDeviceObject, Irp);
    _Analysis_assume_(status!=STATUS_PENDING);
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

                    TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL,  "ClassSendIrpSynchronous: (%p) irp %p did not "
                                "complete within %x seconds\n",
                                TargetDeviceObject, Irp,
                                ClasspnpGlobals.SecondsToWaitForIrps
                                ));

                    if (ClasspnpGlobals.BreakOnLostIrps != 0) {
                        NT_ASSERT(!" - Irp failed to complete within 30 seconds - ");
                    }
                }


            } while (status==STATUS_TIMEOUT);
        #else
            (VOID)KeWaitForSingleObject(&event,
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassGetVpb(
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
#ifdef _MSC_VER
#pragma prefast(suppress:28175)
#endif
    return DeviceObject->Vpb;
} // end ClassGetVpb()

/*++

    ISSUE-2000/02/20-henrygab Not documented ClasspAllocateReleaseRequest

--*/
NTSTATUS
ClasspAllocateReleaseRequest(
    IN PDEVICE_OBJECT Fdo
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;

    PAGED_CODE();

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
ClasspFreeReleaseRequest(
    IN PDEVICE_OBJECT Fdo
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;

    PAGED_CODE();

    //KIRQL oldIrql;

    NT_ASSERT(fdoExtension->CommonExtension.IsRemoved != NO_REMOVE);

    //
    // free anything the driver allocated
    //

    if (fdoExtension->ReleaseQueueIrp) {
        if (fdoExtension->ReleaseQueueIrpFromPool) {
            FREE_POOL(fdoExtension->ReleaseQueueIrp);
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

        FREE_POOL(fdoExtension->PrivateFdoData->ReleaseQueueIrp);
        fdoExtension->PrivateFdoData->ReleaseQueueIrpAllocated = FALSE;
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassReleaseQueue(
    _In_ PDEVICE_OBJECT Fdo
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
ClasspAllocateReleaseQueueIrp(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
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

    NT_ASSERT(!(FdoExtension->ReleaseQueueInProgress));

    FdoExtension->PrivateFdoData->ReleaseQueueIrp =
        ExAllocatePoolWithTag(NonPagedPoolNx,
                              IoSizeOfIrp(lowerStackSize),
                              CLASS_TAG_RELEASE_QUEUE
                              );

    if (FdoExtension->PrivateFdoData->ReleaseQueueIrp == NULL) {
        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_PNP,  "ClassPnpStartDevice: Cannot allocate for "
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

ClasspAllocatePowerProcessIrp()

Routine Description:

    This routine allocates the power process irp.
    This routine should be called after PrivateFdoData is allocated.

Return Value:

    NTSTATUS value.

Notes:

    Should only be called from StartDevice()

--*/
NTSTATUS
ClasspAllocatePowerProcessIrp(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    UCHAR stackSize;

    NT_ASSERT(FdoExtension->PrivateFdoData != NULL);

    stackSize = FdoExtension->CommonExtension.LowerDeviceObject->StackSize + 1;

    FdoExtension->PrivateFdoData->PowerProcessIrp = ExAllocatePoolWithTag(NonPagedPoolNx,
                                                                          IoSizeOfIrp(stackSize),
                                                                          CLASS_TAG_POWER
                                                                          );

    if (FdoExtension->PrivateFdoData->PowerProcessIrp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    } else {

        IoInitializeIrp(FdoExtension->PrivateFdoData->PowerProcessIrp,
                        IoSizeOfIrp(stackSize),
                        stackSize);

        return STATUS_SUCCESS;
    }
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
ClasspReleaseQueue(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP ReleaseQueueIrp OPTIONAL
    )
{
    PIO_STACK_LOCATION irpStack;
    PIRP irp;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PDEVICE_OBJECT lowerDevice;
    PSTORAGE_REQUEST_BLOCK_HEADER srb;
    KIRQL currentIrql;
    ULONG function;

    lowerDevice = fdoExtension->CommonExtension.LowerDeviceObject;

    //
    // we raise irql seperately so we're not swapped out or suspended
    // while holding the release queue irp in this routine.  this lets
    // us release the spin lock before lowering irql.
    //

    KeRaiseIrql(DISPATCH_LEVEL, &currentIrql);

    KeAcquireSpinLockAtDpcLevel(&(fdoExtension->ReleaseQueueSpinLock));

    //
    // make sure that if they passed us an irp, it matches our allocated irp.
    //

    NT_ASSERT((ReleaseQueueIrp == NULL) ||
           (ReleaseQueueIrp == fdoExtension->PrivateFdoData->ReleaseQueueIrp));

    //
    // ASSERT that we've already allocated this. (should not occur)
    // try to allocate it anyways, then finally bugcheck if
    // there's still no memory...
    //

    NT_ASSERT(fdoExtension->PrivateFdoData->ReleaseQueueIrpAllocated);
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

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        srb = (PSTORAGE_REQUEST_BLOCK_HEADER)&(fdoExtension->PrivateFdoData->ReleaseQueueSrb.SrbEx);
    } else {
        srb = (PSTORAGE_REQUEST_BLOCK_HEADER)&(fdoExtension->ReleaseQueueSrb);
    }

    KeReleaseSpinLockFromDpcLevel(&(fdoExtension->ReleaseQueueSpinLock));

    NT_ASSERT(irp != NULL);

    irpStack = IoGetNextIrpStackLocation(irp);

    irpStack->MajorFunction = IRP_MJ_SCSI;

    SrbSetOriginalRequest(srb, irp);

    //
    // Store the SRB address in next stack for port driver.
    //

    irpStack->Parameters.Scsi.Srb = (PSCSI_REQUEST_BLOCK)srb;

    //
    // If this device is removable then flush the queue.  This will also
    // release it.
    //

    if (TEST_FLAG(Fdo->Characteristics, FILE_REMOVABLE_MEDIA)){
       function = SRB_FUNCTION_FLUSH_QUEUE;
    }
    else {
       function = SRB_FUNCTION_RELEASE_QUEUE;
    }

    if (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {
        ((PSTORAGE_REQUEST_BLOCK)srb)->SrbFunction = function;
    } else {
        srb->Function = (UCHAR)function;
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
    which was issused by the class driver completes.  Examples of such requests
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassReleaseQueueCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    KIRQL oldIrql;

    BOOLEAN releaseQueueNeeded;

    if (Context == NULL) {
        NT_ASSERT(Context != NULL);
        return STATUS_INVALID_PARAMETER;
    }

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
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassAcquireChildLock(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PAGED_CODE();

    if(FdoExtension->ChildLockOwner != KeGetCurrentThread()) {
        (VOID)KeWaitForSingleObject(&FdoExtension->ChildLock,
                                    Executive, KernelMode,
                                    FALSE, NULL);

        NT_ASSERT(FdoExtension->ChildLockOwner == NULL);
        NT_ASSERT(FdoExtension->ChildLockAcquisitionCount == 0);

        FdoExtension->ChildLockOwner = KeGetCurrentThread();
    } else {
        NT_ASSERT(FdoExtension->ChildLockAcquisitionCount != 0);
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassReleaseChildLock(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    NT_ASSERT(FdoExtension->ChildLockOwner == KeGetCurrentThread());
    NT_ASSERT(FdoExtension->ChildLockAcquisitionCount != 0);

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
ClassAddChild(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION Parent,
    _In_ PPHYSICAL_DEVICE_EXTENSION Child,
    _In_ BOOLEAN AcquireLock
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

                NT_ASSERT(testChild != Child);
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
                NT_ASSERT(previousChild != &Child->CommonExtension);

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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClasspRetryRequestDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID Arg1,
    IN PVOID Arg2
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PCOMMON_DEVICE_EXTENSION commonExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData;
    PCLASS_RETRY_INFO retryList;
    KIRQL irql;
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Arg1);
    UNREFERENCED_PARAMETER(Arg2);

    if (DeferredContext == NULL) {
        NT_ASSERT(DeferredContext != NULL);
        return;
    }

    commonExtension = DeviceObject->DeviceExtension;
    NT_ASSERT(commonExtension->IsFdo);
    fdoExtension = DeviceObject->DeviceExtension;
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
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, "ClassRetry:  -- %p\n", irp));
        retryList = retryList->Next;
        #if DBG
            irp->Tail.Overlay.DriverContext[0] = ULongToPtr(0xdddddddd); // invalidate data
            irp->Tail.Overlay.DriverContext[1] = ULongToPtr(0xdddddddd); // invalidate data
            irp->Tail.Overlay.DriverContext[2] = ULongToPtr(0xdddddddd); // invalidate data
            irp->Tail.Overlay.DriverContext[3] = ULongToPtr(0xdddddddd); // invalidate data
        #endif


        if (NO_REMOVE == InterlockedCompareExchange((volatile LONG *)&commonExtension->IsRemoved, REMOVE_PENDING, REMOVE_PENDING)) {

            IoCallDriver(commonExtension->LowerDeviceObject, irp);

        } else {

            PIO_STACK_LOCATION irpStack;

            //
            // Ensure that we don't skip a completion routine (equivalent of sending down a request
            // to a device after it has received a remove and it completes the request. We need to
            // mimic that behavior here).
            //
            IoSetNextIrpStackLocation(irp);

            irpStack = IoGetCurrentIrpStackLocation(irp);

            if (irpStack->MajorFunction == IRP_MJ_SCSI) {

                PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;

                if (srb) {
                    srb->SrbStatus = SRB_STATUS_NO_DEVICE;
                }
            }

            //
            // Ensure that no retries will take place. This takes care of requests that are either
            // not IRP_MJ_SCSI or for ones where the SRB was passed in to the completion routine
            // as a context as opposed to an argument on the IRP stack location.
            //
            irpStack->Parameters.Others.Argument4 = (PVOID)0;

            irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
            irp->IoStatus.Information = 0;
            IoCompleteRequest(irp,  IO_NO_INCREMENT);
        }

    }
    return;

} // end ClasspRetryRequestDpc()

/*++

    ISSUE-2000/02/20-henrygab Not documented ClassRetryRequest

--*/
VOID
ClassRetryRequest(
    IN PDEVICE_OBJECT SelfDeviceObject,
    IN PIRP           Irp,
    _In_ _In_range_(0,MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS) // 100 seconds; already an assert on this...
    IN LONGLONG       TimeDelta100ns // in 100ns units
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData;
    PCLASS_RETRY_INFO  retryInfo;
    LARGE_INTEGER      delta;
    KIRQL irql;

    //
    // this checks we aren't destroying irps
    //
    NT_ASSERT(sizeof(CLASS_RETRY_INFO) <= (4*sizeof(PVOID)));

    fdoExtension = SelfDeviceObject->DeviceExtension;

    if (!fdoExtension->CommonExtension.IsFdo) {

        //
        // this debug print/assertion should ALWAYS be investigated.
        // ClassRetryRequest can currently only be used by FDO's
        //

        TracePrint((TRACE_LEVEL_ERROR, TRACE_FLAG_GENERAL, "ClassRetryRequestEx: LOST IRP %p\n", Irp));
        NT_ASSERT(!"ClassRetryRequestEx Called From PDO? LOST IRP");
        return;

    }

    fdoData = fdoExtension->PrivateFdoData;

    if (TimeDelta100ns < 0) {
        NT_ASSERT(!"ClassRetryRequest - must use positive delay");
        TimeDelta100ns *= -1;
    }

    /*
     *  We are going to queue the irp and send it down in a timer DPC.
     *  This means that we may be causing the irp to complete on a different thread than the issuing thread.
     *  So mark the irp pending.
     */
    IoMarkIrpPending(Irp);

    //
    // prepare what we can out of the loop
    //

    retryInfo = (PCLASS_RETRY_INFO)(&Irp->Tail.Overlay.DriverContext[0]);
    RtlZeroMemory(retryInfo, sizeof(CLASS_RETRY_INFO));

    delta.QuadPart = (TimeDelta100ns / fdoData->Retry.Granularity);
    if (TimeDelta100ns % fdoData->Retry.Granularity) {
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

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, "ClassRetry: +++ %p\n", Irp));

        //
        // must be exactly one item on list
        //

        NT_ASSERT(fdoData->Retry.ListHead       != NULL);
        NT_ASSERT(fdoData->Retry.ListHead->Next == NULL);

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

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, "ClassRetry:  ++ %p\n", Irp));

        //
        // must be at least two items on list
        //

        NT_ASSERT(fdoData->Retry.ListHead       != NULL);
        NT_ASSERT(fdoData->Retry.ListHead->Next != NULL);

        fdoData->Retry.Tick.QuadPart  -= fdoData->Retry.Delta.QuadPart;
        fdoData->Retry.Tick.QuadPart  += delta.QuadPart;

        fdoData->Retry.Delta.QuadPart  = delta.QuadPart;

    } else {

        //
        // just inserting it on the list was enough
        //

        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL, "ClassRetry:  ++ %p\n", Irp));

    }


    KeReleaseSpinLock(&fdoData->Retry.Lock, irql);


} // end ClassRetryRequest()

/*++

    ISSUE-2000/02/20-henrygab Not documented ClasspRetryDpcTimer

--*/
VOID
ClasspRetryDpcTimer(
    IN PCLASS_PRIVATE_FDO_DATA FdoData
    )
{
    LARGE_INTEGER fire;

    NT_ASSERT(FdoData->Retry.Tick.QuadPart != (LONGLONG)0);
    NT_ASSERT(FdoData->Retry.ListHead      != NULL);  // never fire an empty list

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

    TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_GENERAL,
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
ClasspInitializeHotplugInfo(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PCLASS_PRIVATE_FDO_DATA fdoData = FdoExtension->PrivateFdoData;
    DEVICE_REMOVAL_POLICY deviceRemovalPolicy = 0;
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
    // Query the default removal policy from the kernel
    //

    status = IoGetDeviceProperty(FdoExtension->LowerPdo,
                                 DevicePropertyRemovalPolicy,
                                 sizeof(DEVICE_REMOVAL_POLICY),
                                 (PVOID)&deviceRemovalPolicy,
                                 &resultLength);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (resultLength != sizeof(DEVICE_REMOVAL_POLICY)) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Look into the registry to see if the user has chosen
    // to override the default setting for the removal policy.
    // User can override only if the default removal policy is
    // orderly or suprise removal.

    if ((deviceRemovalPolicy == RemovalPolicyExpectOrderlyRemoval) ||
        (deviceRemovalPolicy == RemovalPolicyExpectSurpriseRemoval)) {

        DEVICE_REMOVAL_POLICY userRemovalPolicy = 0;

        ClassGetDeviceParameter(FdoExtension,
                                CLASSP_REG_SUBKEY_NAME,
                                CLASSP_REG_REMOVAL_POLICY_VALUE_NAME,
                                (PULONG)&userRemovalPolicy);

        //
        // Validate the override value and use it only if it is an
        // allowed value.
        //
        if ((userRemovalPolicy == RemovalPolicyExpectOrderlyRemoval) ||
            (userRemovalPolicy == RemovalPolicyExpectSurpriseRemoval)) {
            deviceRemovalPolicy = userRemovalPolicy;
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
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
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
ClasspScanForSpecialInRegistry(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    HANDLE             deviceParameterHandle; // device instance key
    HANDLE             classParameterHandle; // classpnp subkey
    OBJECT_ATTRIBUTES  objectAttributes = {0};
    UNICODE_STRING     subkeyName;
    NTSTATUS           status;

    //
    // seeded in the ENUM tree by ClassInstaller
    //
    ULONG deviceHacks;
    RTL_QUERY_REGISTRY_TABLE queryTable[2] = {0}; // null terminated array

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
    // Setup the structure to read
    //

    queryTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    queryTable[0].Name          = CLASSP_REG_HACK_VALUE_NAME;
    queryTable[0].EntryContext  = &deviceHacks;
    queryTable[0].DefaultType   = (REG_DWORD << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT) | REG_DWORD;
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

/*++////////////////////////////////////////////////////////////////////////////

ClasspUpdateDiskProperties()

Routine Description:

    This routine will send IOCTL_DISK_UPDATE_PROPERTIES to top of stack - Partition Manager
    to invalidate the cached geometry.

Arguments:

    Fdo - The device object whose capacity needs to be verified.

Return Value:

    NTSTATUS

--*/

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClasspUpdateDiskProperties(
    IN PDEVICE_OBJECT Fdo,
    IN PVOID Context
    )
{
    PDEVICE_OBJECT topOfStack;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status = STATUS_SUCCESS;
    KEVENT event;
    PIRP irp = NULL;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExt->PrivateFdoData;
    PIO_WORKITEM WorkItem = (PIO_WORKITEM)Context;

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    topOfStack = IoGetAttachedDeviceReference(Fdo);

    //
    // Send down irp to update properties
    //

    irp = IoBuildDeviceIoControlRequest(
                    IOCTL_DISK_UPDATE_PROPERTIES,
                    topOfStack,
                    NULL,
                    0,
                    NULL,
                    0,
                    FALSE,
                    &event,
                    &ioStatus);

    if (irp != NULL) {


        status = IoCallDriver(topOfStack, irp);
        if (status == STATUS_PENDING) {
            (VOID)KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    InterlockedExchange((volatile LONG *)&fdoData->UpdateDiskPropertiesWorkItemActive, 0);

    if (!NT_SUCCESS(status)) {
        TracePrint((TRACE_LEVEL_WARNING, TRACE_FLAG_INIT, "ClasspUpdateDiskProperties: Disk property update for fdo %p failed with status 0x%X.\n", Fdo, status));
    } else {
        TracePrint((TRACE_LEVEL_INFORMATION, TRACE_FLAG_INIT, "ClasspUpdateDiskProperties: Drive capacity has changed for %p.\n", Fdo));
    }
    ObDereferenceObject(topOfStack);

    if (WorkItem != NULL) {
        IoFreeWorkItem(WorkItem);
    }

    return;
}

BOOLEAN
InterpretSenseInfoWithoutHistory(
    _In_ PDEVICE_OBJECT Fdo,
    _In_opt_ PIRP OriginalRequest,
    _In_ PSCSI_REQUEST_BLOCK Srb,
         UCHAR MajorFunctionCode,
         ULONG IoDeviceCode,
         ULONG PreviousRetryCount,
    _Out_ NTSTATUS * Status,
    _Out_opt_ _Deref_out_range_(0,MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS)
         LONGLONG * RetryIn100nsUnits
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;
    LONGLONG tmpRetry = 0;
    BOOLEAN retry = FALSE;

    if (fdoData->InterpretSenseInfo != NULL)
    {
        SCSI_REQUEST_BLOCK tempSrb = {0};
        PSCSI_REQUEST_BLOCK srbPtr = Srb;

        // SAL annotations and ClassInitializeEx() both validate this
        NT_ASSERT(fdoData->InterpretSenseInfo->Interpret != NULL);

        //
        // If class driver does not support extended SRB and this is
        // an extended SRB, convert to legacy SRB and pass to class
        // driver.
        //
        if ((Srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK) &&
            ((fdoExtension->CommonExtension.DriverExtension->SrbSupport &
              CLASS_SRB_STORAGE_REQUEST_BLOCK) == 0)) {
            ClasspConvertToScsiRequestBlock(&tempSrb, (PSTORAGE_REQUEST_BLOCK)Srb);
            srbPtr = &tempSrb;
        }

        retry = fdoData->InterpretSenseInfo->Interpret(Fdo,
                                                       OriginalRequest,
                                                       srbPtr,
                                                       MajorFunctionCode,
                                                       IoDeviceCode,
                                                       PreviousRetryCount,
                                                       NULL,
                                                       Status,
                                                       &tmpRetry);
    }
    else
    {
        ULONG seconds = 0;

        retry = ClassInterpretSenseInfo(Fdo,
                                        Srb,
                                        MajorFunctionCode,
                                        IoDeviceCode,
                                        PreviousRetryCount,
                                        Status,
                                        &seconds);
        tmpRetry = ((LONGLONG)seconds) * 1000 * 1000 * 10;
    }


    if (RetryIn100nsUnits != NULL)
    {
        *RetryIn100nsUnits = tmpRetry;
    }
    return retry;
}

VOID
ClasspGetInquiryVpdSupportInfo(
    _Inout_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    NTSTATUS                  status;
    PCOMMON_DEVICE_EXTENSION  commonExtension = (PCOMMON_DEVICE_EXTENSION)FdoExtension;
    SCSI_REQUEST_BLOCK        srb = {0};
    PCDB                      cdb;
    PVPD_SUPPORTED_PAGES_PAGE supportedPages = NULL;
    UCHAR                     bufferLength = VPD_MAX_BUFFER_SIZE;
    ULONG                     allocationBufferLength = bufferLength;
    UCHAR srbExBuffer[CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE] = {0};
    PSTORAGE_REQUEST_BLOCK_HEADER srbHeader;

#if defined(_ARM_) || defined(_ARM64_)
    //
    // ARM has specific alignment requirements, although this will not have a functional impact on x86 or amd64
    // based platforms. We are taking the conservative approach here.
    //
    allocationBufferLength = ALIGN_UP_BY(allocationBufferLength,KeGetRecommendedSharedDataAlignment());
    supportedPages = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned,
                                           allocationBufferLength,
                                           '3CcS'
                                           );

#else
    supportedPages = ExAllocatePoolWithTag(NonPagedPoolNx,
                                           bufferLength,
                                           '3CcS'
                                           );
#endif

    if (supportedPages == NULL) {
        // memory allocation failure.
        return;
    }

    RtlZeroMemory(supportedPages, allocationBufferLength);

    // prepare the Srb
    if (FdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK) {

#ifdef _MSC_VER
        #pragma prefast(suppress:26015, "InitializeStorageRequestBlock ensures buffer access is bounded")
#endif
        status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)srbExBuffer,
                                               STORAGE_ADDRESS_TYPE_BTL8,
                                               sizeof(srbExBuffer),
                                               1,
                                               SrbExDataTypeScsiCdb16);

        if (!NT_SUCCESS(status)) {
            //
            // Should not happen. Revert to legacy SRB.
            NT_ASSERT(FALSE);
            srb.Length = SCSI_REQUEST_BLOCK_SIZE;
            srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
            srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&srb;
        } else {
            ((PSTORAGE_REQUEST_BLOCK)srbExBuffer)->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;
            srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)srbExBuffer;
        }

    } else {
        srb.Length = SCSI_REQUEST_BLOCK_SIZE;
        srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
        srbHeader = (PSTORAGE_REQUEST_BLOCK_HEADER)&srb;
    }

    SrbSetTimeOutValue(srbHeader, FdoExtension->TimeOutValue);
    SrbSetRequestTag(srbHeader, SP_UNTAGGED);
    SrbSetRequestAttribute(srbHeader, SRB_SIMPLE_TAG_REQUEST);
    SrbAssignSrbFlags(srbHeader, FdoExtension->SrbFlags);
    SrbSetCdbLength(srbHeader, 6);

    cdb = SrbGetCdb(srbHeader);
    cdb->CDB6INQUIRY3.OperationCode = SCSIOP_INQUIRY;
    cdb->CDB6INQUIRY3.EnableVitalProductData = 1;       //EVPD bit
    cdb->CDB6INQUIRY3.PageCode = VPD_SUPPORTED_PAGES;
    cdb->CDB6INQUIRY3.AllocationLength = bufferLength;  //AllocationLength field in CDB6INQUIRY3 is only one byte.

    status = ClassSendSrbSynchronous(commonExtension->DeviceObject,
                                     (PSCSI_REQUEST_BLOCK)srbHeader,
                                     supportedPages,
                                     allocationBufferLength,
                                     FALSE);

    //
    // Handle the case where we get back STATUS_DATA_OVERRUN b/c the input
    // buffer was larger than necessary.
    //
    if (status == STATUS_DATA_OVERRUN && SrbGetDataTransferLength(srbHeader) < bufferLength)
    {
        status = STATUS_SUCCESS;
    }

    if ( NT_SUCCESS(status) &&
         (supportedPages->PageLength > 0) &&
         (supportedPages->SupportedPageList[0] > 0) ) {
        // ataport treats all INQUIRY command as standard INQUIRY command, thus fills invalid info
        // If VPD INQUIRY is supported, the first page reported (additional length field for standard INQUIRY data) should be '00'
        status = STATUS_NOT_SUPPORTED;
    }

    if (NT_SUCCESS(status)) {
        int i;

        for (i = 0; i < supportedPages->PageLength; i++) {
            if ( (i > 0) && (supportedPages->SupportedPageList[i] <= supportedPages->SupportedPageList[i - 1]) ) {
                // shall be in ascending order beginning with page code 00h.
                FdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockDeviceRODLimits = FALSE;
                FdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockLimits = FALSE;
                FdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockDeviceCharacteristics = FALSE;
                FdoExtension->FunctionSupportInfo->ValidInquiryPages.LBProvisioning = FALSE;


                break;
            }
            switch (supportedPages->SupportedPageList[i]) {
                case VPD_THIRD_PARTY_COPY:
                    FdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockDeviceRODLimits = TRUE;
                    break;

                case VPD_BLOCK_LIMITS:
                    FdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockLimits = TRUE;
                    break;

                case VPD_BLOCK_DEVICE_CHARACTERISTICS:
                    FdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockDeviceCharacteristics = TRUE;
                    break;

                case VPD_LOGICAL_BLOCK_PROVISIONING:
                    FdoExtension->FunctionSupportInfo->ValidInquiryPages.LBProvisioning = TRUE;
                    break;

            }
        }
    }

    ExFreePool(supportedPages);

    return;
}

//
// ClasspGetLBProvisioningInfo
//
//  Description: This function does the work to get Logical Block Provisioning
//      (LBP) info for a given LUN, including UNMAP support and parameters.  It
//      will attempt to get both the Logical Block Provisioning (0xB2) VPD page
//      and the Block Limits (0xB0) VPD page and cache the relevant values in
//      the given FDO extension.
//
//      After calling this function, you can use the ClasspIsThinProvisioned()
//      function to determine if the device is thinly provisioned and the
//      ClasspSupportsUnmap() function to determine if the device supports the
//      UNMAP command.
//
//  Arguments:
//      - FdoExtension: The FDO extension associated with the LUN for which Thin
//          Provisioning info is desired.  The Thin Provisioning info is stored
//          in the FunctionSupportInfo member of this FDO extension.
//
// Returns:
//      - STATUS_INVALID_PARAMETER if the given FDO extension has not been
//          allocated properly.
//      - STATUS_INSUFFICIENT_RESOURCES if this function was unable to allocate
//          an SRB used to get the necessary VPD pages.
//      - STATUS_SUCCESS in all other cases.  If any of the incidental functions
//          don't return STATUS_SUCCESS this function will just assume Thin
//          Provisioning is not enabled and return STATUS_SUCCESS.
//
NTSTATUS
ClasspGetLBProvisioningInfo(
    _Inout_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    PSCSI_REQUEST_BLOCK srb = NULL;
    ULONG               srbSize = 0;

    //
    // Make sure we actually have data structures to work with.
    //
    if (FdoExtension == NULL ||
        FdoExtension->FunctionSupportInfo == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Allocate an SRB for querying the device for LBP-related info if either
    // the Logical Block Provisioning (0xB2) or Block Limits (0xB0) VPD page
    // exists.
    //
    if ((FdoExtension->FunctionSupportInfo->ValidInquiryPages.LBProvisioning == TRUE) ||
        (FdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockLimits == TRUE)) {

        if ((FdoExtension->AdapterDescriptor != NULL) &&
            (FdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK)) {
            srbSize = CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE;
        } else {
            srbSize = sizeof(SCSI_REQUEST_BLOCK);
        }

        srb = ExAllocatePoolWithTag(NonPagedPoolNx, srbSize, '0DcS');

        if (srb == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // Get the Logical Block Provisioning VPD page (0xB2).  This function will
    // set some default values if the LBP VPD page is not supported.
    //
    ClasspDeviceGetLBProvisioningVPDPage(FdoExtension->DeviceObject, srb);


    //
    // Get the Block Limits VPD page (0xB0), which may override the default
    // UNMAP parameter values set above.
    //
    ClasspDeviceGetBlockLimitsVPDPage(FdoExtension,
                                      srb,
                                      srbSize,
                                      &FdoExtension->FunctionSupportInfo->BlockLimitsData);

    FREE_POOL(srb);

    return STATUS_SUCCESS;
}



_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClassDetermineTokenOperationCommandSupport(
    _In_ PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine determines if Token Operation and Receive ROD Token Information commands
    are supported by this device and updates the internal structures to reflect this.
    In addition, it also queries the registry to determine the maximum listIdentifier
    to use for TokenOperation commands.

    This function must be called at IRQL == PASSIVE_LEVEL.

Arguments:

    DeviceObject - The device object for which we want to determine command support.

Return Value:

    Nothing

--*/

{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_PNP,
                "ClassDetermineTokenOperationCommandSupport (%p): Entering function.\n",
                DeviceObject));

    //
    // Send down Inquiry for VPD_THIRD_PARTY_COPY_PAGE and cache away the device parameters
    // from WINDOWS_BLOCK_DEVICE_TOKEN_LIMITS_DESCRIPTOR.
    //
    status = ClasspGetBlockDeviceTokenLimitsInfo(DeviceObject);

    if (NT_SUCCESS(status)) {

        ULONG maxListIdentifier = MaxTokenOperationListIdentifier;

        //
        // Query the maximum list identifier to use for TokenOperation commands.
        //
        if (NT_SUCCESS(ClasspGetMaximumTokenListIdentifier(DeviceObject, REG_DISK_CLASS_CONTROL, &maxListIdentifier))) {
            if (maxListIdentifier >= MIN_TOKEN_LIST_IDENTIFIERS) {

                NT_ASSERT(maxListIdentifier <= MAX_TOKEN_LIST_IDENTIFIERS);
                MaxTokenOperationListIdentifier = maxListIdentifier;
            }
        }

    }

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_PNP,
                "ClassDetermineTokenOperationCommandSupport (%p): Exiting function with status %x.\n",
                DeviceObject,
                status));

    return status;
}


_IRQL_requires_same_
NTSTATUS
ClasspGetBlockDeviceTokenLimitsInfo(
    _Inout_ PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine does the work to get the Block Device Token Limits info for a given LUN.
    This is done by sending Inquiry for the Third Party Copy VPD page.

Arguments:

    DeviceObject - The FDO associated with the LUN for which Block Device Token
                       limits info is desired.

Return Value:

    STATUS_DEVICE_FEATURE_NOT_SUPPORTED if either the Inquiry fails or validations fail.
    STATUS_SUCCESS otherwise.

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;
    PSCSI_REQUEST_BLOCK srb = NULL;
    ULONG srbSize = 0;
    USHORT pageLength = 0;
    USHORT descriptorLength = 0;
    PVOID dataBuffer = NULL;
    UCHAR bufferLength = VPD_MAX_BUFFER_SIZE;
    ULONG allocationBufferLength = bufferLength;
    PCDB cdb;
    ULONG dataTransferLength = 0;
    PVPD_THIRD_PARTY_COPY_PAGE operatingParameters = NULL;
    PWINDOWS_BLOCK_DEVICE_TOKEN_LIMITS_DESCRIPTOR blockDeviceTokenLimits = NULL;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_PNP,
                "ClasspGetBlockDeviceTokenLimitsInfo (%p): Entering function.\n",
                DeviceObject));

    //
    // Allocate an SRB for querying the device for LBP-related info.
    //
    if ((fdoExtension->AdapterDescriptor != NULL) &&
        (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK)) {
        srbSize = CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE;
    } else {
        srbSize = sizeof(SCSI_REQUEST_BLOCK);
    }

    srb = ExAllocatePoolWithTag(NonPagedPoolNx, srbSize, CLASSPNP_POOL_TAG_SRB);

    if (!srb) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_PNP,
                    "ClasspGetBlockDeviceTokenLimitsInfo (%p): Couldn't allocate SRB.\n",
                    DeviceObject));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto __ClasspGetBlockDeviceTokenLimitsInfo_Exit;
    }

#if defined(_ARM_) || defined(_ARM64_)
    //
    // ARM has specific alignment requirements, although this will not have a functional impact on x86 or amd64
    // based platforms. We are taking the conservative approach here.
    //
    allocationBufferLength = ALIGN_UP_BY(allocationBufferLength, KeGetRecommendedSharedDataAlignment());
    dataBuffer = ExAllocatePoolWithTag(NonPagedPoolNxCacheAligned, allocationBufferLength, CLASSPNP_POOL_TAG_VPD);
#else
    dataBuffer = ExAllocatePoolWithTag(NonPagedPoolNx, bufferLength, CLASSPNP_POOL_TAG_VPD);
#endif

    if (!dataBuffer) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_PNP,
                    "ClasspGetBlockDeviceTokenLimitsInfo (%p): Couldn't allocate dataBuffer.\n",
                    DeviceObject));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto __ClasspGetBlockDeviceTokenLimitsInfo_Exit;
    }

    operatingParameters = (PVPD_THIRD_PARTY_COPY_PAGE)dataBuffer;

    RtlZeroMemory(dataBuffer, allocationBufferLength);

    if ((fdoExtension->AdapterDescriptor != NULL) &&
        (fdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK)) {
        status = InitializeStorageRequestBlock((PSTORAGE_REQUEST_BLOCK)srb,
                                               STORAGE_ADDRESS_TYPE_BTL8,
                                               CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE,
                                               1,
                                               SrbExDataTypeScsiCdb16);
        if (NT_SUCCESS(status)) {

            ((PSTORAGE_REQUEST_BLOCK)srb)->SrbFunction = SRB_FUNCTION_EXECUTE_SCSI;

        } else {

            //
            // Should not occur. Revert to legacy SRB.
            //
            NT_ASSERT(FALSE);

            TracePrint((TRACE_LEVEL_WARNING,
                        TRACE_FLAG_PNP,
                        "ClasspGetBlockDeviceTokenLimitsInfo (%p): Falling back to using SRB (instead of SRB_EX).\n",
                        DeviceObject));

            RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));
            srb->Length = sizeof(SCSI_REQUEST_BLOCK);
            srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
        }
    } else {

        RtlZeroMemory(srb, sizeof(SCSI_REQUEST_BLOCK));
        srb->Length = sizeof(SCSI_REQUEST_BLOCK);
        srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
    }

    SrbSetTimeOutValue(srb, fdoExtension->TimeOutValue);
    SrbSetRequestTag(srb, SP_UNTAGGED);
    SrbSetRequestAttribute(srb, SRB_SIMPLE_TAG_REQUEST);
    SrbAssignSrbFlags(srb, fdoExtension->SrbFlags);

    SrbSetCdbLength(srb, 6);

    cdb = SrbGetCdb(srb);
    cdb->CDB6INQUIRY3.OperationCode = SCSIOP_INQUIRY;
    cdb->CDB6INQUIRY3.EnableVitalProductData = 1;
    cdb->CDB6INQUIRY3.PageCode = VPD_THIRD_PARTY_COPY;
    cdb->CDB6INQUIRY3.AllocationLength = bufferLength;

    status = ClassSendSrbSynchronous(fdoExtension->DeviceObject,
                                     srb,
                                     dataBuffer,
                                     allocationBufferLength,
                                     FALSE);

    //
    // Handle the case where we get back STATUS_DATA_OVERRUN because the input
    // buffer was larger than necessary.
    //
    dataTransferLength = SrbGetDataTransferLength(srb);

    if (status == STATUS_DATA_OVERRUN && dataTransferLength < bufferLength) {

        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {

        REVERSE_BYTES_SHORT(&pageLength, &operatingParameters->PageLength);

    } else {
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_PNP,
                    "ClasspGetBlockDeviceTokenLimitsInfo (%p): Inquiry for TPC VPD failed with %x.\n",
                    DeviceObject,
                    status));
    }

    if ((NT_SUCCESS(status)) &&
        (pageLength >= sizeof(WINDOWS_BLOCK_DEVICE_TOKEN_LIMITS_DESCRIPTOR))) {

        USHORT descriptorType;

        blockDeviceTokenLimits = (PWINDOWS_BLOCK_DEVICE_TOKEN_LIMITS_DESCRIPTOR)operatingParameters->ThirdPartyCopyDescriptors;
        REVERSE_BYTES_SHORT(&descriptorType, &blockDeviceTokenLimits->DescriptorType);
        REVERSE_BYTES_SHORT(&descriptorLength, &blockDeviceTokenLimits->DescriptorLength);

        if ((descriptorType == BLOCK_DEVICE_TOKEN_LIMITS_DESCRIPTOR_TYPE_WINDOWS) &&
            (VPD_PAGE_HEADER_SIZE + descriptorLength == sizeof(WINDOWS_BLOCK_DEVICE_TOKEN_LIMITS_DESCRIPTOR))) {

            fdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockDeviceRODLimits = TRUE;

            REVERSE_BYTES_SHORT(&fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumRangeDescriptors, &blockDeviceTokenLimits->MaximumRangeDescriptors);
            REVERSE_BYTES(&fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumInactivityTimer, &blockDeviceTokenLimits->MaximumInactivityTimer);
            REVERSE_BYTES(&fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.DefaultInactivityTimer, &blockDeviceTokenLimits->DefaultInactivityTimer);
            REVERSE_BYTES_QUAD(&fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumTokenTransferSize, &blockDeviceTokenLimits->MaximumTokenTransferSize);
            REVERSE_BYTES_QUAD(&fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.OptimalTransferCount, &blockDeviceTokenLimits->OptimalTransferCount);

            TracePrint((TRACE_LEVEL_INFORMATION,
                        TRACE_FLAG_PNP,
                        "ClasspGetBlockDeviceTokenLimitsInfo (%p): %s %s (rev %s) reported following parameters: \
                        \n\t\t\tMaxRangeDescriptors: %u\n\t\t\tMaxIAT: %u\n\t\t\tDefaultIAT: %u \
                        \n\t\t\tMaxTokenTransferSize: %I64u\n\t\t\tOptimalTransferCount: %I64u\n\t\t\tOptimalTransferLengthGranularity: %u \
                        \n\t\t\tOptimalTransferLength: %u\n\t\t\tMaxTransferLength: %u\n",
                        DeviceObject,
                        (PCSZ)(((PUCHAR)fdoExtension->DeviceDescriptor) + fdoExtension->DeviceDescriptor->VendorIdOffset),
                        (PCSZ)(((PUCHAR)fdoExtension->DeviceDescriptor) + fdoExtension->DeviceDescriptor->ProductIdOffset),
                        (PCSZ)(((PUCHAR)fdoExtension->DeviceDescriptor) + fdoExtension->DeviceDescriptor->ProductRevisionOffset),
                        fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumRangeDescriptors,
                        fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumInactivityTimer,
                        fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.DefaultInactivityTimer,
                        fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumTokenTransferSize,
                        fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.OptimalTransferCount,
                        fdoExtension->FunctionSupportInfo->BlockLimitsData.OptimalTransferLengthGranularity,
                        fdoExtension->FunctionSupportInfo->BlockLimitsData.OptimalTransferLength,
                        fdoExtension->FunctionSupportInfo->BlockLimitsData.MaximumTransferLength));
        } else {

            TracePrint((TRACE_LEVEL_WARNING,
                        TRACE_FLAG_PNP,
                        "ClasspGetBlockDeviceTokenLimitsInfo (%p): ThirdPartyCopy VPD data doesn't have Windows OffloadDataTransfer descriptor.\n",
                        DeviceObject));

            fdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockDeviceRODLimits = FALSE;
            status = STATUS_DEVICE_FEATURE_NOT_SUPPORTED;
        }
    } else {

        TracePrint((TRACE_LEVEL_WARNING,
                    TRACE_FLAG_PNP,
                    "ClasspGetBlockDeviceTokenLimitsInfo (%p): TPC VPD data didn't return TPC descriptors of interest.\n",
                    DeviceObject));

        fdoExtension->FunctionSupportInfo->ValidInquiryPages.BlockDeviceRODLimits = FALSE;
        status = STATUS_DEVICE_FEATURE_NOT_SUPPORTED;
    }

__ClasspGetBlockDeviceTokenLimitsInfo_Exit:

    FREE_POOL(dataBuffer);
    FREE_POOL(srb);
    fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.CommandStatus = status;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_PNP,
                "ClasspGetBlockDeviceTokenLimitsInfo (%p): Exiting function with status %x.\n",
                DeviceObject,
                status));

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClassDeviceProcessOffloadRead(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine services IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES for CopyOffload
    Read. If the device supports copy offload, it performs the translation of the
    IOCTL into the appropriate SCSI commands to complete the operation.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request
    Irp - The IRP to be processed
    Srb - An SRB that can be optimally used to process this request

Return Value:

    NTSTATUS code

--*/

{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS status;
    PDEVICE_MANAGE_DATA_SET_ATTRIBUTES dsmAttributes;
    PDEVICE_DSM_OFFLOAD_READ_PARAMETERS offloadReadParameters;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;

    UNREFERENCED_PARAMETER(Srb);

    //
    // This function must be called at less than dispatch level.
    // Fail if IRQL >= DISPATCH_LEVEL.
    //
    PAGED_CODE();

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClassDeviceProcessOffloadRead (%p): Entering function. Irp %p\n",
                DeviceObject,
                Irp));


    irpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Validations
    //
    status = ClasspValidateOffloadSupported(DeviceObject, Irp);
    if (!NT_SUCCESS(status)) {
        goto __ClassDeviceProcessOffloadRead_CompleteAndExit;
    }

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {

        NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClassDeviceProcessOffloadRead (%p): Called at raised IRQL.\n",
                    DeviceObject));

        status = STATUS_INVALID_LEVEL;
        goto __ClassDeviceProcessOffloadRead_CompleteAndExit;
    }

    //
    // Ensure that this DSM IOCTL was generated in kernel
    //
    if (Irp->RequestorMode != KernelMode) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClassDeviceProcessOffloadRead (%p): Called from user mode.\n",
                    DeviceObject));

        status = STATUS_ACCESS_DENIED;
        goto __ClassDeviceProcessOffloadRead_CompleteAndExit;
    }

    status = ClasspValidateOffloadInputParameters(DeviceObject, Irp);
    if (!NT_SUCCESS(status)) {
        goto __ClassDeviceProcessOffloadRead_CompleteAndExit;
    }

    dsmAttributes = Irp->AssociatedIrp.SystemBuffer;

    //
    // Validate that we were passed in correct sized parameter block.
    //
    if (dsmAttributes->ParameterBlockLength < sizeof(DEVICE_DSM_OFFLOAD_READ_PARAMETERS)) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClassDeviceProcessOffloadRead (%p): Parameter block size (%u) too small. Required %u.\n",
                    DeviceObject,
                    dsmAttributes->ParameterBlockLength,
                    sizeof(DEVICE_DSM_OFFLOAD_READ_PARAMETERS)));

        status = STATUS_INVALID_PARAMETER;
        goto __ClassDeviceProcessOffloadRead_CompleteAndExit;
    }

    offloadReadParameters = Add2Ptr(dsmAttributes, dsmAttributes->ParameterBlockOffset);

    fdoExtension = DeviceObject->DeviceExtension;

    //
    // If the request TTL is greater than the max supported by this storage, the target will
    // end up failing this command, so might as well do the check up front.
    //
    if ((fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumInactivityTimer > 0) &&
        (offloadReadParameters->TimeToLive > fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumInactivityTimer)) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClassDeviceProcessOffloadRead (%p): Requested TTL (%u) greater than max supported (%u).\n",
                    DeviceObject,
                    offloadReadParameters->TimeToLive,
                    fdoExtension->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumInactivityTimer));

        status = STATUS_INVALID_PARAMETER;
        goto __ClassDeviceProcessOffloadRead_CompleteAndExit;
    }

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_OFFLOAD_READ_OUTPUT)) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClassDeviceProcessOffloadRead (%p): Output buffer size (%u) too small.\n",
                    DeviceObject,
                    irpStack->Parameters.DeviceIoControl.OutputBufferLength));

        status = STATUS_BUFFER_TOO_SMALL;
        goto __ClassDeviceProcessOffloadRead_CompleteAndExit;
    }



    status = ClasspServicePopulateTokenTransferRequest(DeviceObject, Irp);

    if (status == STATUS_PENDING) {
        goto __ClassDeviceProcessOffloadRead_Exit;
    }

__ClassDeviceProcessOffloadRead_CompleteAndExit:
    ClasspCompleteOffloadRequest(DeviceObject, Irp, status);
__ClassDeviceProcessOffloadRead_Exit:
    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClassDeviceProcessOffloadRead (%p): Exiting function Irp %p with status %x.\n",
                DeviceObject,
                Irp,
                status));

    return status;
}

VOID
ClasspCompleteOffloadRequest(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ NTSTATUS CompletionStatus)
{
    NTSTATUS status = CompletionStatus;


    Irp->IoStatus.Status = status;

    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_NO_INCREMENT);

    return;
}


_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClassDeviceProcessOffloadWrite(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine services IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES for CopyOffload
    Write. If the device supports copy offload, it performs the translation of the
    IOCTL into the appropriate SCSI commands to complete the operation.

    This function must be called at IRQL < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Supplies the device object associated with this request
    Irp - The IRP to be processed
    Srb - An SRB that can be optinally used to process this request

Return Value:

    NTSTATUS code

--*/

{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS status;
    PDEVICE_MANAGE_DATA_SET_ATTRIBUTES dsmAttributes;

    UNREFERENCED_PARAMETER(Srb);

    //
    // This function must be called at less than dispatch level.
    //
    PAGED_CODE();

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClassDeviceProcessOffloadWrite (%p): Entering function. Irp %p\n",
                DeviceObject,
                Irp));


    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Validations
    //
    status = ClasspValidateOffloadSupported(DeviceObject, Irp);
    if (!NT_SUCCESS(status)) {
        goto __ClassDeviceProcessOffloadWrite_CompleteAndExit;
    }

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {

        NT_ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClassDeviceProcessOffloadWrite (%p): Called at raised IRQL.\n",
                    DeviceObject));

        status = STATUS_INVALID_LEVEL;
        goto __ClassDeviceProcessOffloadWrite_CompleteAndExit;
    }

    //
    // Ensure that this DSM IOCTL was generated in kernel
    //
    if (Irp->RequestorMode != KernelMode) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClassDeviceProcessOffloadWrite (%p): Called from user mode.\n",
                    DeviceObject));

        status = STATUS_ACCESS_DENIED;
        goto __ClassDeviceProcessOffloadWrite_CompleteAndExit;
    }

    status = ClasspValidateOffloadInputParameters(DeviceObject, Irp);
    if (!NT_SUCCESS(status)) {
        goto __ClassDeviceProcessOffloadWrite_CompleteAndExit;
    }

    dsmAttributes = Irp->AssociatedIrp.SystemBuffer;

    //
    // Validate that we were passed in correct sized parameter block.
    //
    if (dsmAttributes->ParameterBlockLength < sizeof(DEVICE_DSM_OFFLOAD_WRITE_PARAMETERS)) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClassDeviceProcessOffloadWrite (%p): Parameter block size (%u) too small. Required %u.\n",
                    DeviceObject,
                    dsmAttributes->ParameterBlockLength,
                    sizeof(DEVICE_DSM_OFFLOAD_WRITE_PARAMETERS)));

        status = STATUS_INVALID_PARAMETER;
        goto __ClassDeviceProcessOffloadWrite_CompleteAndExit;
    }

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_OFFLOAD_WRITE_OUTPUT)) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClassDeviceProcessOffloadWrite (%p): Output buffer size (%u) too small.\n",
                    DeviceObject,
                    irpStack->Parameters.DeviceIoControl.OutputBufferLength));

        status = STATUS_BUFFER_TOO_SMALL;
        goto __ClassDeviceProcessOffloadWrite_CompleteAndExit;
    }



    status = ClasspServiceWriteUsingTokenTransferRequest(DeviceObject, Irp);

    if (status == STATUS_PENDING) {
        goto __ClassDeviceProcessOffloadWrite_Exit;
    }

__ClassDeviceProcessOffloadWrite_CompleteAndExit:
    ClasspCompleteOffloadRequest(DeviceObject, Irp, status);
__ClassDeviceProcessOffloadWrite_Exit:
    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClassDeviceProcessOffloadWrite (%p): Exiting function Irp %p with status %x\n",
                DeviceObject,
                Irp,
                status));

    return status;
}


_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClasspServicePopulateTokenTransferRequest(
    _In_ PDEVICE_OBJECT Fdo,
    _In_ PIRP Irp
    )

/*++

Routine description:

    This routine processes offload read requests by building the SRB
    for PopulateToken and its result (i.e. token).

Arguments:

    Fdo - The functional device object processing the request
    Irp - The Io request to be processed

Return Value:

    STATUS_SUCCESS if successful, an error code otherwise

--*/

{
    BOOLEAN allDataSetRangeFullyConverted;
    UINT32 allocationSize;
    ULONG blockDescrIndex;
    PBLOCK_DEVICE_RANGE_DESCRIPTOR blockDescrPointer;
    PVOID buffer;
    ULONG bufferLength;
    ULONG dataSetRangeIndex;
    PDEVICE_DATA_SET_RANGE dataSetRanges;
    ULONG dataSetRangesCount;
    PDEVICE_MANAGE_DATA_SET_ATTRIBUTES dsmAttributes;
    ULONGLONG entireXferLen;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt;
    ULONG i;
    ULONG lbaCount;
    ULONG listIdentifier;
    ULONG maxBlockDescrCount;
    ULONGLONG maxLbaCount;
    POFFLOAD_READ_CONTEXT offloadReadContext;
    PDEVICE_DSM_OFFLOAD_READ_PARAMETERS offloadReadParameters;
    PTRANSFER_PACKET pkt;
    USHORT populateTokenDataLength;
    USHORT populateTokenDescriptorsLength;
    PMDL populateTokenMdl;
    PIRP pseudoIrp;
    ULONG receiveTokenInformationBufferLength;
    NTSTATUS status;
    DEVICE_DATA_SET_RANGE tempDataSetRange;
    BOOLEAN tempDataSetRangeFullyConverted;
    PUCHAR token;
    ULONG tokenLength;
    ULONG tokenOperationBufferLength;
    ULONGLONG totalSectorCount;
    ULONGLONG totalSectorsToProcess;
    ULONG transferSize;

    PAGED_CODE();

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspServicePopulateTokenTransferRequest (%p): Entering function. Irp %p\n",
                Fdo,
                Irp));

    fdoExt = Fdo->DeviceExtension;
    status = STATUS_SUCCESS;
    dsmAttributes = Irp->AssociatedIrp.SystemBuffer;
    buffer = NULL;
    pkt = NULL;
    populateTokenMdl = NULL;
    offloadReadParameters = Add2Ptr(dsmAttributes, dsmAttributes->ParameterBlockOffset);
    dataSetRanges = Add2Ptr(dsmAttributes, dsmAttributes->DataSetRangesOffset);
    dataSetRangesCount = dsmAttributes->DataSetRangesLength / sizeof(DEVICE_DATA_SET_RANGE);
    totalSectorsToProcess = 0;
    token = NULL;
    tokenLength = 0;
    bufferLength = 0;
    offloadReadContext = NULL;

    NT_ASSERT(fdoExt->FunctionSupportInfo->ValidInquiryPages.BlockDeviceRODLimits &&
              NT_SUCCESS(fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.CommandStatus));


    for (i = 0, entireXferLen = 0; i < dataSetRangesCount; i++) {
        entireXferLen += dataSetRanges[i].LengthInBytes;
    }

    //
    // We need to truncate the read request based on the following hardware limitations:
    // 1. The size of the data buffer containing the TokenOperation command's parameters must
    //    not exceed the MaximumTransferLength (and max physical pages) of the underlying
    //    adapter.
    // 2. The number of descriptors specified in the TokenOperation command must not exceed
    //    the MaximumRangeDescriptors.
    // 3. The cumulative total of the number of transfer blocks in all the descriptors in
    //    the TokenOperation command must not exceed the MaximumTokenTransferSize.
    // 4. If the TTL has been set to the 0 (i.e. indicates the use of the default TLT),
    //    limit the cumulative total of the number of transfer blocks in all the descriptors
    //    in the TokenOperation command to the OptimalTransferCount.
    //    NOTE: only a TTL of 0 has an implicit indication that the initiator of the
    //    request would like the storage stack to be smart about the amount of data
    //    transfer.
    //
    // In addition to the above, we need to ensure that for each of the descriptors in the
    // TokenOperation command:
    // 1. The number of blocks specified is an exact multiple of the OptimalTransferLengthGranularity.
    // 2. The number of blocks specified is limited to the MaximumTransferLength. (We shall
    //    however, limit the number of blocks specified in each descriptor to be a maximum of
    //    OptimalTransferLength or MaximumTransferLength, whichever is lesser).
    //
    // Finally, we shall always send down the PopulateToken command using IMMED = 0 for this
    // release. This makes it simpler to handle multi-initiator scenarios since we won't need
    // to deal with the PopulateToken (with IMMED = 1) succeeding but ReceiveRODTokenInformation
    // failing due to the path/node failing, thus making it impossible to retrieve the token.
    //
    // The LBA ranges is in DEVICE_DATA_SET_RANGE format, it needs to be converted into
    // WINDOWS_RANGE_DESCRIPTOR Block Descriptors.
    //

    ClasspGetTokenOperationCommandBufferLength(Fdo,
                                               SERVICE_ACTION_POPULATE_TOKEN,
                                               &bufferLength,
                                               &tokenOperationBufferLength,
                                               &receiveTokenInformationBufferLength);

    allocationSize = sizeof(OFFLOAD_READ_CONTEXT) + bufferLength;

    offloadReadContext = ExAllocatePoolWithTag(
        NonPagedPoolNx,
        allocationSize,
        CLASSPNP_POOL_TAG_TOKEN_OPERATION);

    if (!offloadReadContext) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspServicePopulateTokenTransferRequest (%p): Failed to allocate buffer for PopulateToken operations.\n",
                    Fdo));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto __ClasspServicePopulateTokenTransferRequest_ErrorExit;
    }

    RtlZeroMemory(offloadReadContext, allocationSize);

    offloadReadContext->Fdo = Fdo;
    offloadReadContext->OffloadReadDsmIrp = Irp;

    //
    // The buffer for the commands is after the offloadReadContext.
    //
    buffer = (offloadReadContext + 1);

    //
    // No matter how large the offload read request, we'll be sending it down in one shot.
    // So we need only one transfer packet.
    //
    pkt = DequeueFreeTransferPacket(Fdo, TRUE);
    if (!pkt){

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspServicePopulateTokenTransferRequest (%p): Failed to retrieve transfer packet for TokenOperation (PopulateToken) operation.\n",
                    Fdo));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto __ClasspServicePopulateTokenTransferRequest_ErrorExit;
    }

    offloadReadContext->Pkt = pkt;

    ClasspGetTokenOperationDescriptorLimits(Fdo,
                                            SERVICE_ACTION_POPULATE_TOKEN,
                                            tokenOperationBufferLength,
                                            &maxBlockDescrCount,
                                            &maxLbaCount);

    //
    // We will limit the maximum data transfer in an offload read operation to:
    // - OTC if TTL was specified as 0
    // - MaximumTransferTransferSize if lesser than above, or if TTL was specified was non-zero
    //
    if ((offloadReadParameters->TimeToLive == 0) && (fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.OptimalTransferCount > 0)) {

        maxLbaCount = MIN(maxLbaCount, fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.OptimalTransferCount);
    }

    //
    // Since we do not want very fragmented files to end up causing the PopulateToken command to take
    // too long (and potentially timeout), we will limit the max number of descriptors sent down in a
    // command.
    //
    maxBlockDescrCount = MIN(maxBlockDescrCount, MAX_NUMBER_BLOCK_DEVICE_DESCRIPTORS);

    TracePrint((TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_IOCTL,
                "ClasspServicePopulateTokenTransferRequest (%p): Using MaxBlockDescrCount %u and MaxLbaCount %I64u.\n",
                Fdo,
                maxBlockDescrCount,
                maxLbaCount));

    allDataSetRangeFullyConverted = FALSE;
    tempDataSetRangeFullyConverted = TRUE;
    dataSetRangeIndex = (ULONG)-1;

    blockDescrPointer = (PBLOCK_DEVICE_RANGE_DESCRIPTOR)
        &((PPOPULATE_TOKEN_HEADER)buffer)->BlockDeviceRangeDescriptor[0];

    RtlZeroMemory(&tempDataSetRange, sizeof(tempDataSetRange));

    blockDescrIndex = 0;
    lbaCount = 0;

    //
    // Send PopulateToken command when the buffer is full or all input entries are converted.
    //
    while (!((blockDescrIndex == maxBlockDescrCount) ||   // buffer full or block descriptor count reached
             (lbaCount == maxLbaCount) ||                 // block LBA count reached
             (allDataSetRangeFullyConverted))) {          // all DataSetRanges have been converted

        //
        // If the previous entry conversion completed, continue the next one;
        // Otherwise, still process the left part of the un-completed entry.
        //
        if (tempDataSetRangeFullyConverted) {
            dataSetRangeIndex++;
            tempDataSetRange.StartingOffset = dataSetRanges[dataSetRangeIndex].StartingOffset;
            tempDataSetRange.LengthInBytes = dataSetRanges[dataSetRangeIndex].LengthInBytes;
        }

        totalSectorCount = 0;

        ClasspConvertDataSetRangeToBlockDescr(Fdo,
                                              blockDescrPointer,
                                              &blockDescrIndex,
                                              maxBlockDescrCount,
                                              &lbaCount,
                                              maxLbaCount,
                                              &tempDataSetRange,
                                              &totalSectorCount);

        tempDataSetRangeFullyConverted = (tempDataSetRange.LengthInBytes == 0) ? TRUE : FALSE;

        allDataSetRangeFullyConverted = tempDataSetRangeFullyConverted && ((dataSetRangeIndex + 1) == dataSetRangesCount);

        totalSectorsToProcess += totalSectorCount;
    }

    //
    // Calculate transfer size, including the header
    //
    transferSize = (blockDescrIndex * sizeof(BLOCK_DEVICE_RANGE_DESCRIPTOR)) + FIELD_OFFSET(POPULATE_TOKEN_HEADER, BlockDeviceRangeDescriptor);

    NT_ASSERT(transferSize <= MAX_TOKEN_OPERATION_PARAMETER_DATA_LENGTH);

    populateTokenDataLength = (USHORT)transferSize - RTL_SIZEOF_THROUGH_FIELD(POPULATE_TOKEN_HEADER, PopulateTokenDataLength);
    REVERSE_BYTES_SHORT(((PPOPULATE_TOKEN_HEADER)buffer)->PopulateTokenDataLength, &populateTokenDataLength);

    ((PPOPULATE_TOKEN_HEADER)buffer)->Immediate = 0;

    REVERSE_BYTES(((PPOPULATE_TOKEN_HEADER)buffer)->InactivityTimeout, &offloadReadParameters->TimeToLive);

    populateTokenDescriptorsLength = (USHORT)transferSize - FIELD_OFFSET(POPULATE_TOKEN_HEADER, BlockDeviceRangeDescriptor);
    REVERSE_BYTES_SHORT(((PPOPULATE_TOKEN_HEADER)buffer)->BlockDeviceRangeDescriptorListLength, &populateTokenDescriptorsLength);

    //
    // Reuse a single buffer for both TokenOperation and ReceiveTokenInformation. This has the one disadvantage
    // that we'll be marking the page(s) as IoWriteAccess even though we only need read access for token
    // operation command. However, the advantage is that we eliminate the possibility of any potential failures
    // when trying to allocate an MDL for the ReceiveTokenInformation later on.
    //
    bufferLength = max(transferSize, receiveTokenInformationBufferLength);

    populateTokenMdl = ClasspBuildDeviceMdl(buffer, bufferLength, FALSE);
    if (!populateTokenMdl) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspServicePopulateTokenTransferRequest (%p): Failed to allocate MDL for PopulateToken operations.\n",
                    Fdo));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto __ClasspServicePopulateTokenTransferRequest_ErrorExit;
    }

    offloadReadContext->PopulateTokenMdl = populateTokenMdl;

    pseudoIrp = &offloadReadContext->PseudoIrp;


    pseudoIrp->IoStatus.Status = STATUS_SUCCESS;
    pseudoIrp->IoStatus.Information = 0;
    pseudoIrp->Tail.Overlay.DriverContext[0] = LongToPtr(1);
    pseudoIrp->MdlAddress = populateTokenMdl;

    InterlockedCompareExchange((volatile LONG *)&TokenOperationListIdentifier, -1, MaxTokenOperationListIdentifier);
    listIdentifier = InterlockedIncrement((volatile LONG *)&TokenOperationListIdentifier);

    ClasspSetupPopulateTokenTransferPacket(
        offloadReadContext,
        pkt,
        transferSize,
        (PUCHAR)buffer,
        pseudoIrp,
        listIdentifier);

    TracePrint((TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_IOCTL,
                "ClasspServicePopulateTokenTransferRequest (%p): Generate token for %I64u bytes (versus %I64u) [via %u descriptors]. \
                \n\t\t\tDataLength: %u, DescriptorsLength: %u. Pkt %p (list id %x). Requested TTL: %u secs.\n",
                Fdo,
                totalSectorsToProcess * fdoExt->DiskGeometry.BytesPerSector,
                entireXferLen,
                blockDescrIndex,
                populateTokenDataLength,
                populateTokenDescriptorsLength,
                pkt,
                listIdentifier,
                offloadReadParameters->TimeToLive));

    //
    // Save (into the offloadReadContext) any remaining things that
    // ClasspPopulateTokenTransferPacketDone() will need.
    //

    offloadReadContext->ListIdentifier = listIdentifier;
    offloadReadContext->BufferLength = bufferLength;
    offloadReadContext->ReceiveTokenInformationBufferLength = receiveTokenInformationBufferLength;
    offloadReadContext->TotalSectorsToProcess = totalSectorsToProcess; // so far
    offloadReadContext->EntireXferLen = entireXferLen;

    NT_ASSERT(status == STATUS_SUCCESS); // so far.

    IoMarkIrpPending(Irp);
    SubmitTransferPacket(pkt);

    status = STATUS_PENDING;
    goto __ClasspServicePopulateTokenTransferRequest_Exit;

    //
    // Error cleanup label only - not used in success case:
    //

__ClasspServicePopulateTokenTransferRequest_ErrorExit:

    NT_ASSERT(status != STATUS_PENDING);

    if (offloadReadContext != NULL) {
        ClasspCleanupOffloadReadContext(offloadReadContext);
        offloadReadContext = NULL;
    }

__ClasspServicePopulateTokenTransferRequest_Exit:

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspServicePopulateTokenTransferRequest (%p): Exiting function (Irp %p) with status %x.\n",
                Fdo,
                Irp,
                status));

    return status;
}


VOID
ClasspPopulateTokenTransferPacketDone(
    _In_ PVOID Context
    )

/*++

Routine description:

    This routine continues an offload read operation on completion of the
    populate token transfer packet.

    This function is responsible for continuing or completing the offload read
    operation.

Arguments:

    Context - Pointer to the OFFLOAD_READ_CONTEXT for the offload read
        operation.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT fdo;
    ULONG listIdentifier;
    POFFLOAD_READ_CONTEXT offloadReadContext;
    PTRANSFER_PACKET pkt;
    PIRP pseudoIrp;
    NTSTATUS status;

    offloadReadContext = Context;
    pseudoIrp = &offloadReadContext->PseudoIrp;
    pkt = offloadReadContext->Pkt;
    fdo = offloadReadContext->Fdo;
    listIdentifier = offloadReadContext->ListIdentifier;

    offloadReadContext->Pkt = NULL;


    status = pseudoIrp->IoStatus.Status;
    NT_ASSERT(status != STATUS_PENDING);

    if (!NT_SUCCESS(status)) {
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspPopulateTokenTransferPacketDone (%p): Generate token for list Id %x failed with %x (Pkt %p).\n",
                    fdo,
                    listIdentifier,
                    status,
                    pkt));
        goto __ClasspPopulateTokenTransferPacketDone_ErrorExit;
    }

    //
    // If a token was successfully generated, it is now time to retrieve the token.
    // The called function is responsible for completing the offload read DSM IRP.
    //
    ClasspReceivePopulateTokenInformation(offloadReadContext);

    //
    // ClasspReceivePopulateTokenInformation() takes care of completing the IRP,
    // so this function is done regardless of success or failure in
    // ClasspReceivePopulateTokenInformation().
    //

    return;

    //
    // Error cleanup label only - not used in success case:
    //

__ClasspPopulateTokenTransferPacketDone_ErrorExit:

    NT_ASSERT(!NT_SUCCESS(status));

    //
    // ClasspCompleteOffloadRead also cleans up offloadReadContext.
    //

    ClasspCompleteOffloadRead(offloadReadContext, status);

    return;
}


VOID
ClasspCompleteOffloadRead(
    _In_ POFFLOAD_READ_CONTEXT OffloadReadContext,
    _In_ NTSTATUS CompletionStatus
    )

/*++

Routine description:

    This routine completes an offload read operation with given status, and
    cleans up the OFFLOAD_READ_CONTEXT.

Arguments:

    OffloadReadContext - Pointer to the OFFLOAD_READ_CONTEXT for the offload
        read operation.

    CompletionStatus - The completion status for the offload read operation.

Return Value:

    None.

--*/

{
    PDEVICE_MANAGE_DATA_SET_ATTRIBUTES dsmAttributes;
    ULONGLONG entireXferLen;
    PDEVICE_OBJECT fdo;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt;
    PIRP irp;
    NTSTATUS status;
    PUCHAR token;
    ULONGLONG totalSectorsProcessed;

    status = CompletionStatus;
    dsmAttributes = OffloadReadContext->OffloadReadDsmIrp->AssociatedIrp.SystemBuffer;
    totalSectorsProcessed = OffloadReadContext->TotalSectorsProcessed;
    fdoExt = OffloadReadContext->Fdo->DeviceExtension;
    entireXferLen = OffloadReadContext->EntireXferLen;
    token = OffloadReadContext->Token;
    irp = OffloadReadContext->OffloadReadDsmIrp;
    fdo = OffloadReadContext->Fdo;

    ((PSTORAGE_OFFLOAD_READ_OUTPUT)dsmAttributes)->OffloadReadFlags = 0;
    ((PSTORAGE_OFFLOAD_READ_OUTPUT)dsmAttributes)->Reserved = 0;

    if (NT_SUCCESS(status)) {
        ULONGLONG totalBytesProcessed = totalSectorsProcessed * fdoExt->DiskGeometry.BytesPerSector;

        TracePrint((totalBytesProcessed == entireXferLen ? TRACE_LEVEL_INFORMATION : TRACE_LEVEL_WARNING,
                    TRACE_FLAG_IOCTL,
                    "ClasspCompleteOffloadRead (%p): Successfully populated token with %I64u (out of %I64u) bytes (list Id %x).\n",
                    fdo,
                    totalBytesProcessed,
                    entireXferLen,
                    OffloadReadContext->ListIdentifier));

        if (totalBytesProcessed < entireXferLen) {
            SET_FLAG(((PSTORAGE_OFFLOAD_READ_OUTPUT)dsmAttributes)->OffloadReadFlags, STORAGE_OFFLOAD_READ_RANGE_TRUNCATED);
        }
        ((PSTORAGE_OFFLOAD_READ_OUTPUT)dsmAttributes)->LengthProtected = totalBytesProcessed;
        ((PSTORAGE_OFFLOAD_READ_OUTPUT)dsmAttributes)->TokenLength = STORAGE_OFFLOAD_MAX_TOKEN_LENGTH;
        RtlCopyMemory(&((PSTORAGE_OFFLOAD_READ_OUTPUT)dsmAttributes)->Token, token, STORAGE_OFFLOAD_MAX_TOKEN_LENGTH);
    } else {
        ((PSTORAGE_OFFLOAD_READ_OUTPUT)dsmAttributes)->LengthProtected = 0;
        ((PSTORAGE_OFFLOAD_READ_OUTPUT)dsmAttributes)->TokenLength = 0;
    }

    irp->IoStatus.Information = sizeof(STORAGE_OFFLOAD_READ_OUTPUT);

    ClasspCompleteOffloadRequest(fdo, irp, status);
    ClasspCleanupOffloadReadContext(OffloadReadContext);
    OffloadReadContext = NULL;

    return;
}


VOID
ClasspCleanupOffloadReadContext(
    _In_ __drv_freesMem(mem) POFFLOAD_READ_CONTEXT OffloadReadContext
    )

/*++

Routine description:

    This routine cleans up an OFFLOAD_READ_CONTEXT.

Arguments:

    OffloadReadContext - Pointer to the OFFLOAD_READ_CONTEXT for the offload
        read operation.

Return Value:

    None.

--*/

{
    PMDL populateTokenMdl;

    populateTokenMdl = OffloadReadContext->PopulateTokenMdl;

    NT_ASSERT(OffloadReadContext != NULL);

    if (populateTokenMdl) {
        ClasspFreeDeviceMdl(populateTokenMdl);
    }
    FREE_POOL(OffloadReadContext);

    return;
}


_IRQL_requires_same_
VOID
ClasspReceivePopulateTokenInformation(
    _In_ POFFLOAD_READ_CONTEXT OffloadReadContext
    )

/*++

Routine description:

    This routine retrieves the token after a PopulateToken command
    has been sent down.

    Can also be called repeatedly for a single offload op when a previous
    RECEIVE ROD TOKEN INFORMATION for that op indicated that the operation was
    not yet complete.

    This function is responsible for continuing or completing the offload read
    operation.

Arguments:

    OffloadReadContext - Pointer to the OFFLOAD_READ_CONTEXT for the offload
        read operation.

Return Value:

    None.

--*/

{
    PVOID buffer;
    ULONG bufferLength;
    ULONG cdbLength;
    PDEVICE_OBJECT fdo;
    PIRP irp;
    ULONG listIdentifier;
    PTRANSFER_PACKET pkt;
    PIRP pseudoIrp;
    ULONG receiveTokenInformationBufferLength;
    PSCSI_REQUEST_BLOCK srb;
    NTSTATUS status;
    ULONG tempSizeUlong;
    PULONGLONG totalSectorsProcessed;

    totalSectorsProcessed = &OffloadReadContext->TotalSectorsProcessed;
    buffer = OffloadReadContext + 1;
    bufferLength = OffloadReadContext->BufferLength;
    fdo = OffloadReadContext->Fdo;
    irp = OffloadReadContext->OffloadReadDsmIrp;
    receiveTokenInformationBufferLength = OffloadReadContext->ReceiveTokenInformationBufferLength;
    listIdentifier = OffloadReadContext->ListIdentifier;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspReceivePopulateTokenInformation (%p): Entering function. Irp %p\n",
                fdo,
                irp));

    srb = &OffloadReadContext->Srb;
    *totalSectorsProcessed = 0;

    pkt = DequeueFreeTransferPacket(fdo, TRUE);
    if (!pkt){

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspReceivePopulateTokenInformation (%p): Failed to retrieve transfer packet for ReceiveTokenInformation (PopulateToken) operation.\n",
                    fdo));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto __ClasspReceivePopulateTokenInformation_ErrorExit;
    }

    OffloadReadContext->Pkt = pkt;

    RtlZeroMemory(buffer, bufferLength);

    tempSizeUlong = receiveTokenInformationBufferLength - 4;
    REVERSE_BYTES(((PRECEIVE_TOKEN_INFORMATION_HEADER)buffer)->AvailableData, &tempSizeUlong);

    pseudoIrp = &OffloadReadContext->PseudoIrp;
    RtlZeroMemory(pseudoIrp, sizeof(IRP));


    pseudoIrp->IoStatus.Status = STATUS_SUCCESS;
    pseudoIrp->IoStatus.Information = 0;
    pseudoIrp->Tail.Overlay.DriverContext[0] = LongToPtr(1);
    pseudoIrp->MdlAddress = OffloadReadContext->PopulateTokenMdl;

    ClasspSetupReceivePopulateTokenInformationTransferPacket(
        OffloadReadContext,
        pkt,
        receiveTokenInformationBufferLength,
        (PUCHAR)buffer,
        pseudoIrp,
        listIdentifier);

    //
    // Cache away the CDB as it may be required for forwarded sense data
    // after this command completes.
    //
    RtlZeroMemory(srb, sizeof(*srb));
    cdbLength = SrbGetCdbLength(pkt->Srb);
    if (cdbLength <= 16) {
        RtlCopyMemory(&srb->Cdb, SrbGetCdb(pkt->Srb), cdbLength);
    }

    SubmitTransferPacket(pkt);

    return;

    //
    // Error cleanup label only - not used in success case:
    //

__ClasspReceivePopulateTokenInformation_ErrorExit:

    NT_ASSERT(!NT_SUCCESS(status));

    //
    // ClasspCompleteOffloadRead also cleans up offloadReadContext.
    //

    ClasspCompleteOffloadRead(OffloadReadContext, status);

    return;
}


VOID
ClasspReceivePopulateTokenInformationTransferPacketDone(
    _In_ PVOID Context
    )

/*++

Routine description:

    This routine continues an offload read operation on completion of the
    RECEIVE ROD TOKEN INFORMATION transfer packet.

    This routine is responsible for continuing or completing the offload read
    operation.

Arguments:

    Context - Pointer to the OFFLOAD_READ_CONTEXT for the offload read
        operation.

Return Value:

    None.

--*/

{
    ULONG availableData;
    PVOID buffer;
    UCHAR completionStatus;
    ULONG estimatedRetryInterval;
    PDEVICE_OBJECT fdo;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt;
    PIRP irp;
    ULONG listIdentifier;
    POFFLOAD_READ_CONTEXT offloadReadContext;
    BOOLEAN operationCompleted;
    UCHAR operationStatus;
    PIRP pseudoIrp;
    USHORT segmentsProcessed;
    PSENSE_DATA senseData;
    ULONG senseDataFieldLength;
    UCHAR senseDataLength;
    PSCSI_REQUEST_BLOCK srb;
    NTSTATUS status;
    PUCHAR token;
    PVOID tokenAscii;
    PBLOCK_DEVICE_TOKEN_DESCRIPTOR tokenDescriptor;
    ULONG tokenDescriptorLength;
    PRECEIVE_TOKEN_INFORMATION_HEADER tokenInformationResults;
    PRECEIVE_TOKEN_INFORMATION_RESPONSE_HEADER tokenInformationResultsResponse;
    ULONG tokenLength;
    ULONG tokenSize;
    PULONGLONG totalSectorsProcessed;
    ULONGLONG totalSectorsToProcess;
    ULONGLONG transferBlockCount;

    offloadReadContext = Context;
    fdo = offloadReadContext->Fdo;
    fdoExt = fdo->DeviceExtension;
    buffer = offloadReadContext + 1;
    listIdentifier = offloadReadContext->ListIdentifier;
    irp = offloadReadContext->OffloadReadDsmIrp;
    totalSectorsToProcess = offloadReadContext->TotalSectorsToProcess;
    totalSectorsProcessed = &offloadReadContext->TotalSectorsProcessed;
    srb = &offloadReadContext->Srb;
    tokenAscii = NULL;
    tokenSize = BLOCK_DEVICE_TOKEN_SIZE;
    tokenInformationResults = (PRECEIVE_TOKEN_INFORMATION_HEADER)buffer;
    senseData = (PSENSE_DATA)((PUCHAR)tokenInformationResults + FIELD_OFFSET(RECEIVE_TOKEN_INFORMATION_HEADER, SenseData));
    transferBlockCount = 0;
    tokenInformationResultsResponse = NULL;
    tokenDescriptor = NULL;
    operationCompleted = FALSE;
    tokenDescriptorLength = 0;
    tokenLength = 0;
    token = NULL;
    pseudoIrp = &offloadReadContext->PseudoIrp;

    status = pseudoIrp->IoStatus.Status;
    NT_ASSERT(status != STATUS_PENDING);

    //
    // The buffer we hand allows for the max sizes for all the fields whereas the returned
    // data may be lesser (e.g. sense data info will almost never be MAX_SENSE_BUFFER_SIZE, etc.
    // so handle underrun "error"
    //
    if (status == STATUS_DATA_OVERRUN) {

        status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(status)) {
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspReceivePopulateTokenInformationTransferPacketDone (%p): Token retrieval failed for list Id %x with %x.\n",
                    fdo,
                    listIdentifier,
                    status));
        goto __ClasspReceivePopulateTokenInformationTransferPacketDone_Exit;
    }

    REVERSE_BYTES(&availableData, &tokenInformationResults->AvailableData);

    NT_ASSERT(availableData <= FIELD_OFFSET(RECEIVE_TOKEN_INFORMATION_HEADER, SenseData) + MAX_SENSE_BUFFER_SIZE +
                               FIELD_OFFSET(RECEIVE_TOKEN_INFORMATION_RESPONSE_HEADER, TokenDescriptor) + BLOCK_DEVICE_TOKEN_SIZE);

    NT_ASSERT(tokenInformationResults->ResponseToServiceAction == SERVICE_ACTION_POPULATE_TOKEN);

    operationStatus = tokenInformationResults->OperationStatus;
    operationCompleted = ClasspIsTokenOperationComplete(operationStatus);
    NT_ASSERT(operationCompleted);

    REVERSE_BYTES(&estimatedRetryInterval, &tokenInformationResults->EstimatedStatusUpdateDelay);

    completionStatus = tokenInformationResults->CompletionStatus;

    NT_ASSERT(tokenInformationResults->TransferCountUnits == TRANSFER_COUNT_UNITS_NUMBER_BLOCKS);
    REVERSE_BYTES_QUAD(&transferBlockCount, &tokenInformationResults->TransferCount);

    REVERSE_BYTES_SHORT(&segmentsProcessed, &tokenInformationResults->SegmentsProcessed);
    NT_ASSERT(segmentsProcessed == 0);

    if (operationCompleted) {

        if (transferBlockCount > totalSectorsToProcess) {

            //
            // Buggy or hostile target.  Don't let it claim more was procesed
            // than was requested.  Since this is likely a bug and it's unknown
            // how much was actually transferred, assume no data was
            // transferred.
            //

            NT_ASSERT(transferBlockCount <= totalSectorsToProcess);
            transferBlockCount = 0;
        }

        if (operationStatus != OPERATION_COMPLETED_WITH_SUCCESS &&
            operationStatus != OPERATION_COMPLETED_WITH_RESIDUAL_DATA) {

            //
            // Assert on buggy response from target, but in any case, make sure not
            // to claim that any data was written.
            //

            NT_ASSERT(transferBlockCount == 0);
            transferBlockCount = 0;
        }

        //
        // Since the TokenOperation was sent down synchronously, the operation is complete as soon as the command returns.
        //

        senseDataFieldLength = tokenInformationResults->SenseDataFieldLength;
        senseDataLength = tokenInformationResults->SenseDataLength;
        NT_ASSERT(senseDataFieldLength >= senseDataLength);

        tokenInformationResultsResponse = (PRECEIVE_TOKEN_INFORMATION_RESPONSE_HEADER)((PUCHAR)tokenInformationResults +
                                                                                               FIELD_OFFSET(RECEIVE_TOKEN_INFORMATION_HEADER, SenseData) +
                                                                                               tokenInformationResults->SenseDataFieldLength);

        REVERSE_BYTES(&tokenDescriptorLength, &tokenInformationResultsResponse->TokenDescriptorsLength);

        if (tokenDescriptorLength > 0) {

            NT_ASSERT(tokenDescriptorLength == sizeof(BLOCK_DEVICE_TOKEN_DESCRIPTOR));

            if (tokenDescriptorLength != sizeof(BLOCK_DEVICE_TOKEN_DESCRIPTOR)) {

                TracePrint((TRACE_LEVEL_ERROR,
                            TRACE_FLAG_IOCTL,
                            "ClasspReceivePopulateTokenInformationTransferPacketDone (%p): Bad firmware, token descriptor length %u.\n",
                            fdo,
                            tokenDescriptorLength));

                NT_ASSERT((*totalSectorsProcessed) == 0);
                NT_ASSERT(tokenLength == 0);

            } else {

                USHORT restrictedId;

                tokenDescriptor = (PBLOCK_DEVICE_TOKEN_DESCRIPTOR)tokenInformationResultsResponse->TokenDescriptor;

                REVERSE_BYTES_SHORT(&restrictedId, &tokenDescriptor->TokenIdentifier);
                NT_ASSERT(restrictedId == 0);

                tokenLength = BLOCK_DEVICE_TOKEN_SIZE;
                token = tokenDescriptor->Token;

                *totalSectorsProcessed = transferBlockCount;

                if (transferBlockCount < totalSectorsToProcess) {

                    NT_ASSERT(operationStatus == OPERATION_COMPLETED_WITH_RESIDUAL_DATA ||
                              operationStatus == OPERATION_COMPLETED_WITH_ERROR ||
                              operationStatus == OPERATION_TERMINATED);

                    if (transferBlockCount == 0) {
                        //
                        // Treat the same as not getting a token.
                        //

                        tokenLength = 0;
                    }

                } else {

                    NT_ASSERT(operationStatus == OPERATION_COMPLETED_WITH_SUCCESS);
                    NT_ASSERT(transferBlockCount == totalSectorsToProcess);
                }

                    //
                    // Need to convert to ascii.
                    //
                    tokenAscii = ClasspBinaryToAscii((PUCHAR)token,
                                                     tokenSize,
                                                     &tokenSize);

                    TracePrint((transferBlockCount == totalSectorsToProcess ? TRACE_LEVEL_INFORMATION : TRACE_LEVEL_WARNING,
                                TRACE_FLAG_IOCTL,
                                "ClasspReceivePopulateTokenInformationTransferPacketDone (%p): %wsToken %s generated successfully for list Id %x for data size %I64u bytes.\n",
                                fdo,
                                transferBlockCount == totalSectorsToProcess ? L"" : L"Target truncated read. ",
                                (tokenAscii == NULL) ? "" : tokenAscii,
                                listIdentifier,
                                (*totalSectorsProcessed) * fdoExt->DiskGeometry.BytesPerSector));

                    FREE_POOL(tokenAscii);
            }
        } else {

            TracePrint((TRACE_LEVEL_ERROR,
                        TRACE_FLAG_IOCTL,
                        "ClasspReceivePopulateTokenInformationTransferPacketDone (%p): Target failed to generate a token for list Id %x for data size %I64u bytes (requested %I64u bytes).\n",
                        fdo,
                        listIdentifier,
                        transferBlockCount * fdoExt->DiskGeometry.BytesPerSector,
                        totalSectorsToProcess * fdoExt->DiskGeometry.BytesPerSector));

            *totalSectorsProcessed = 0;

            NT_ASSERT(operationStatus == OPERATION_COMPLETED_WITH_ERROR);
        }

        //
        // Operation that completes with success can have sense data (for target to pass on some extra info)
        // but we don't care about such sense info.
        // Operation that complete but not with success, may not have sense data associated, but may
        // have valid CompletionStatus.
        //
        // The "status" may be overriden by ClassInterpretSenseInfo().  Final
        // status is determined a bit later - this is just the default status
        // when ClassInterpretSenseInfo() doesn't get to run here.
        //
        status = STATUS_SUCCESS;
        if (operationStatus == OPERATION_COMPLETED_WITH_ERROR ||
            operationStatus == OPERATION_COMPLETED_WITH_RESIDUAL_DATA ||
            operationStatus == OPERATION_TERMINATED) {

            SrbSetScsiStatus((PSTORAGE_REQUEST_BLOCK_HEADER)srb, completionStatus);

            if (senseDataLength) {

                ULONG retryInterval;

                NT_ASSERT(senseDataLength <= sizeof(SENSE_DATA));

                srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

                srb->SrbStatus = SRB_STATUS_AUTOSENSE_VALID | SRB_STATUS_ERROR;
                SrbSetSenseInfoBuffer((PSTORAGE_REQUEST_BLOCK_HEADER)srb, senseData);
                SrbSetSenseInfoBufferLength((PSTORAGE_REQUEST_BLOCK_HEADER)srb, senseDataLength);

                ClassInterpretSenseInfo(fdo,
                                        srb,
                                        IRP_MJ_SCSI,
                                        0,
                                        0,
                                        &status,
                                        &retryInterval);

                TracePrint((TRACE_LEVEL_WARNING,
                            TRACE_FLAG_IOCTL,
                            "ClasspReceivePopulateTokenInformationTransferPacketDone (%p): Reason for truncation/failure: %x - for list Id %x for data size %I64u bytes.\n",
                            fdo,
                            status,
                            listIdentifier,
                            transferBlockCount * fdoExt->DiskGeometry.BytesPerSector));
            } else {

                TracePrint((TRACE_LEVEL_WARNING,
                            TRACE_FLAG_IOCTL,
                            "ClasspReceivePopulateTokenInformationTransferPacketDone (%p): No sense data available but reason for truncation/failure, possibly: %x - for list Id %x for data size %I64u bytes.\n",
                            fdo,
                            completionStatus,
                            listIdentifier,
                            transferBlockCount * fdoExt->DiskGeometry.BytesPerSector));
            }
        }

        if (tokenLength > 0) {

            offloadReadContext->Token = token;

            //
            // Even if target returned an error, from the OS upper layers' perspective,
            // it is a success (with truncation) if any data at all was read.
            //
            status = STATUS_SUCCESS;

        } else {

            if (NT_SUCCESS(status)) {
                //
                // Make sure status is a failing status, without throwing away an
                // already-failing status obtained from sense data.
                //
                status = STATUS_UNSUCCESSFUL;
            }
        }

        //
        // Done with the operation.
        //

        NT_ASSERT(status != STATUS_PENDING);
        goto __ClasspReceivePopulateTokenInformationTransferPacketDone_Exit;

    } else {

        status = STATUS_UNSUCCESSFUL;

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspReceivePopulateTokenInformationTransferPacketDone (%p): Token retrieval failed for list Id %x with %x.\n",
                    fdo,
                    listIdentifier,
                    status));

        NT_ASSERT(*totalSectorsProcessed == 0);
        goto __ClasspReceivePopulateTokenInformationTransferPacketDone_Exit;
    }

__ClasspReceivePopulateTokenInformationTransferPacketDone_Exit:

    if (status != STATUS_PENDING) {

        //
        // The "status" value can be success or failure at this point, as
        // appropriate.
        //

        ClasspCompleteOffloadRead(offloadReadContext, status);
    }

    //
    // Due to tracing a potentially freed pointer value "Irp", this trace could
    // be delayed beyond another offload op picking up the same pointer value
    // for its Irp.  This function exits after the operation is complete when
    // status != STATUS_PENDING.
    //

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspReceivePopulateTokenInformationTransferPacketDone (%p): Exiting function (Irp %p) with internal status %x.\n",
                fdo,
                irp,
                status));

    return;
}


_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClasspServiceWriteUsingTokenTransferRequest(
    _In_ PDEVICE_OBJECT Fdo,
    _In_ PIRP Irp
    )

/*++

Routine description:

    This routine processes offload write requests by building the SRB
    for WriteUsingToken.

Arguments:

    Fdo - The functional device object processing the request
    Irp - The Io request to be processed

Return Value:

    STATUS_SUCCESS if successful, an error code otherwise

--*/

{
    ULONG allocationSize;
    PVOID buffer;
    ULONG bufferLength;
    PDEVICE_DATA_SET_RANGE dataSetRanges;
    ULONG dataSetRangesCount;
    PDEVICE_MANAGE_DATA_SET_ATTRIBUTES dsmAttributes;
    ULONGLONG entireXferLen;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt;
    ULONG i;
    ULONGLONG logicalBlockOffset;
    ULONG maxBlockDescrCount;
    ULONGLONG maxLbaCount;
    POFFLOAD_WRITE_CONTEXT offloadWriteContext;
    PDEVICE_DSM_OFFLOAD_WRITE_PARAMETERS offloadWriteParameters;
    ULONG receiveTokenInformationBufferLength;
    NTSTATUS status;
    NTSTATUS tempStatus;
    BOOLEAN tokenInvalidated;
    ULONG tokenOperationBufferLength;
    PMDL writeUsingTokenMdl;

    PAGED_CODE();

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspServiceWriteUsingTokenTransferRequest (%p): Entering function. Irp %p.\n",
                Fdo,
                Irp));

    fdoExt = Fdo->DeviceExtension;
    status = STATUS_SUCCESS;
    tempStatus = STATUS_SUCCESS;
    dsmAttributes = Irp->AssociatedIrp.SystemBuffer;
    buffer = NULL;
    writeUsingTokenMdl = NULL;
    offloadWriteParameters = Add2Ptr(dsmAttributes, dsmAttributes->ParameterBlockOffset);
    dataSetRanges = Add2Ptr(dsmAttributes, dsmAttributes->DataSetRangesOffset);
    dataSetRangesCount = dsmAttributes->DataSetRangesLength / sizeof(DEVICE_DATA_SET_RANGE);
    logicalBlockOffset = offloadWriteParameters->TokenOffset / fdoExt->DiskGeometry.BytesPerSector;
    tokenInvalidated = FALSE;
    bufferLength = 0;


    NT_ASSERT(fdoExt->FunctionSupportInfo->ValidInquiryPages.BlockDeviceRODLimits &&
              NT_SUCCESS(fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.CommandStatus));

    for (i = 0, entireXferLen = 0; i < dataSetRangesCount; i++) {
        entireXferLen += dataSetRanges[i].LengthInBytes;
    }

    //
    // We need to split the write request based on the following hardware limitations:
    // 1. The size of the data buffer containing the TokenOperation command's parameters must
    //    not exceed the MaximumTransferLength (and max physical pages) of the underlying
    //    adapter.
    // 2. The number of descriptors specified in the TokenOperation command must not exceed
    //    the MaximumRangeDescriptors.
    // 3. The cumulative total of the number of transfer blocks in all the descriptors in
    //    the TokenOperation command must not exceed the MaximumTokenTransferSize.
    //
    // In addition to the above, we need to ensure that for each of the descriptors in the
    // TokenOperation command:
    // 1. The number of blocks specified is an exact multiple of the OptimalTransferLengthGranularity.
    // 2. The number of blocks specified is limited to the MaximumTransferLength. (We shall
    //    however, limit the number of blocks specified in each descriptor to be a maximum of
    //    OptimalTransferLength or MaximumTransferLength, whichever is lesser).
    //
    // Finally, we shall always send down the WriteUsingToken command using IMMED = 0 for this
    // release. This makes it simpler to handle multi-initiator scenarios since we won't need
    // to deal with the WriteUsingToken (with IMMED = 1) succeeding but ReceiveRODTokenInformation
    // failing due to the path/node failing, thus making it impossible to retrieve results of
    // the data transfer operation, or to cancel the data transfer via CopyOperationAbort.
    // Since a write data transfer of a large amount of data may take a long time when sent
    // down IMMED = 0, we shall limit the size to a maximum of 256MB even if it means truncating
    // the original requested size to this capped size. The application is expected to deal with
    // this truncation.
    // (NOTE: the cap of 256MB is chosen to match with the Copy Engine's chunk size).
    //
    // The LBA ranges is in DEVICE_DATA_SET_RANGE format, it needs to be converted into
    // WINDOWS_BLOCK_DEVICE_RANGE_DESCRIPTOR Block Descriptors.
    //

    ClasspGetTokenOperationCommandBufferLength(Fdo,
                                               SERVICE_ACTION_WRITE_USING_TOKEN,
                                               &bufferLength,
                                               &tokenOperationBufferLength,
                                               &receiveTokenInformationBufferLength);

    allocationSize = sizeof(OFFLOAD_WRITE_CONTEXT) + bufferLength;

    offloadWriteContext = ExAllocatePoolWithTag(
        NonPagedPoolNx,
        allocationSize,
        CLASSPNP_POOL_TAG_TOKEN_OPERATION);

    if (!offloadWriteContext) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspServiceWriteUsingTokenTransferRequest (%p): Failed to allocate buffer for WriteUsingToken operations.\n",
                    Fdo));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto __ClasspServiceWriteUsingTokenTransferRequest_ErrorExit;
    }

    //
    // Only zero the context portion here.  The buffer portion is zeroed for
    // each sub-request.
    //
    RtlZeroMemory(offloadWriteContext, sizeof(OFFLOAD_WRITE_CONTEXT));

    offloadWriteContext->Fdo = Fdo;
    offloadWriteContext->OffloadWriteDsmIrp = Irp;
    offloadWriteContext->OperationStartTime = KeQueryInterruptTime();

    //
    // The buffer for the commands is after the offloadWriteContext.
    //
    buffer = (offloadWriteContext + 1);

    //
    // Set up fields that allow iterating through whole request, by issuing sub-
    // requests which each do some of the writing.  Because of truncation by the
    // target, it's not known exactly how many bytes will be written by the
    // target in each sub-request (can be less than requested), so the
    // progress through the outer request must only commit the move through the
    // upper DSM ranges when a lower request is done and the number of written
    // sectors is known.
    //

    NT_ASSERT(offloadWriteContext->TotalSectorsProcessedSuccessfully == 0);
    offloadWriteContext->TotalRequestSizeSectors = entireXferLen / fdoExt->DiskGeometry.BytesPerSector;
    NT_ASSERT(offloadWriteContext->DataSetRangeIndex == 0);
    NT_ASSERT(offloadWriteContext->DataSetRangeByteOffset == 0);
    offloadWriteContext->DataSetRangesCount = dataSetRangesCount;

    offloadWriteContext->DsmAttributes = dsmAttributes;
    offloadWriteContext->OffloadWriteParameters = offloadWriteParameters;
    offloadWriteContext->DataSetRanges = dataSetRanges;
    offloadWriteContext->LogicalBlockOffset = logicalBlockOffset;

    ClasspGetTokenOperationDescriptorLimits(Fdo,
                                            SERVICE_ACTION_WRITE_USING_TOKEN,
                                            tokenOperationBufferLength,
                                            &maxBlockDescrCount,
                                            &maxLbaCount);

    if (fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.OptimalTransferCount && fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumTokenTransferSize) {

        NT_ASSERT(fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.OptimalTransferCount <= fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.MaximumTokenTransferSize);
    }

    //
    // We will limit the maximum data transfer in an offload write operation to:
    // - 64MB if OptimalTransferCount = 0
    // - OptimalTransferCount if < 256MB
    // - 256MB if OptimalTransferCount >= 256MB
    // - MaximumTokenTransferSize if lesser than above chosen size
    //
    if (0 == fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.OptimalTransferCount) {

        maxLbaCount = MIN(maxLbaCount, (DEFAULT_MAX_NUMBER_BYTES_PER_SYNC_WRITE_USING_TOKEN / (ULONGLONG)fdoExt->DiskGeometry.BytesPerSector));

    } else if (fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.OptimalTransferCount < (MAX_NUMBER_BYTES_PER_SYNC_WRITE_USING_TOKEN / (ULONGLONG)fdoExt->DiskGeometry.BytesPerSector)) {

        maxLbaCount = MIN(maxLbaCount, fdoExt->FunctionSupportInfo->BlockDeviceRODLimitsData.OptimalTransferCount);

    } else {

        maxLbaCount = MIN(maxLbaCount, (MAX_NUMBER_BYTES_PER_SYNC_WRITE_USING_TOKEN / (ULONGLONG)fdoExt->DiskGeometry.BytesPerSector));
    }

    //
    // Since we do not want very fragmented files to end up causing the WriteUsingToken command to take
    // too long (and potentially timeout), we will limit the max number of descriptors sent down in a
    // command.
    //
    maxBlockDescrCount = MIN(maxBlockDescrCount, MAX_NUMBER_BLOCK_DEVICE_DESCRIPTORS);

    TracePrint((TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_IOCTL,
                "ClasspServiceWriteUsingTokenTransferRequest (%p): Using MaxBlockDescrCount %u and MaxLbaCount %I64u.\n",
                Fdo,
                maxBlockDescrCount,
                maxLbaCount));

    offloadWriteContext->MaxBlockDescrCount = maxBlockDescrCount;
    offloadWriteContext->MaxLbaCount = maxLbaCount;

    //
    // Reuse a single buffer for both TokenOperation and ReceiveTokenInformation. This has the one disadvantage
    // that we'll be marking the page(s) as IoWriteAccess even though we only need read access for token
    // operation command and we may not even need to send down a ReceiveTokenInformation (in case of a successful
    // synchronous transfer). However, the advantage is that we eliminate the possibility of any potential
    // failures when trying to allocate an MDL for the ReceiveTokenInformation later on if we need to send it.
    //
    writeUsingTokenMdl = ClasspBuildDeviceMdl(buffer, bufferLength, FALSE);
    if (!writeUsingTokenMdl) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspServiceWriteUsingTokenTransferRequest (%p): Failed to allocate MDL for WriteUsingToken operations.\n",
                    Fdo));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto __ClasspServiceWriteUsingTokenTransferRequest_ErrorExit;
    }

    offloadWriteContext->WriteUsingTokenMdl = writeUsingTokenMdl;

    //
    // There are potentially two approaches that we can take:
    // 1. Determine how many transfer packets we need (in case we need to split the request), get
    //    them all up-front and then send down all the split WriteUsingToken commands in parallel.
    //    The benefit of this approach is that the performance will be improved in the success case.
    //    But error case handling becomes very complex to handle, since if one of the intermediate
    //    write fails, there is no way to cancel the remaining writes that were sent. Waiting for
    //    such requests to complete in geo-distributed source and target cases can be very time
    //    consuming. The complexity gets worse in the case that the target succeeds only a partial
    //    amount of data for one the of intermediate split commands.
    //                                       [OR]
    // 2. Until the entire data set range is processed, build the command for as much of the range as
    //    possible, send down a packet, and once it completes, repeat sequentially in a loop.
    //    The advantage of this approach is its simplistic nature. In the success case, it will
    //    be less performant as compared to the previous approach, but since the gain of offload
    //    copy is so significant compared to native buffered-copy, the tradeoff is acceptable.
    //    In the failure case the simplicity offers the following benefit - if any command fails,
    //    there is no further processing that is needed. And the cumulative total bytes that succeeded
    //    (until the failing split WriteUsingToken command) is easily tracked.
    //
    // Given the above, we're going with the second approach.
    //

    NT_ASSERT(status == STATUS_SUCCESS); // so far

    //
    // Save (into the offloadReadContext) any remaining things that
    // ClasspPopulateTokenTransferPacketDone() will need.
    //

    offloadWriteContext->BufferLength = bufferLength;
    offloadWriteContext->ReceiveTokenInformationBufferLength = receiveTokenInformationBufferLength;
    offloadWriteContext->EntireXferLen = entireXferLen;

    IoMarkIrpPending(Irp);
    ClasspContinueOffloadWrite(offloadWriteContext);

    status = STATUS_PENDING;
    goto __ClasspServiceWriteUsingTokenTransferRequest_Exit;

    //
    // Error label only - not used in success case:
    //

__ClasspServiceWriteUsingTokenTransferRequest_ErrorExit:

    NT_ASSERT(status != STATUS_PENDING);

    if (offloadWriteContext != NULL) {
        ClasspCleanupOffloadWriteContext(offloadWriteContext);
        offloadWriteContext = NULL;
    }

__ClasspServiceWriteUsingTokenTransferRequest_Exit:

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspServiceWriteUsingTokenTransferRequest (%p): Exiting function (Irp %p) with status %x.\n",
                Fdo,
                Irp,
                status));

    return status;
}


VOID
#ifdef _MSC_VER
#pragma warning(suppress: 28194) // This function will either alias or free OffloadWriteContext
#endif
ClasspContinueOffloadWrite(
    _In_ __drv_aliasesMem POFFLOAD_WRITE_CONTEXT OffloadWriteContext
    )

/*++

Routine description:

    This routine continues an offload write operation.  This routine expects the
    offload write operation to be set up and ready to start a WRITE USING TOKEN,
    but with no WRITE USING TOKEN currently in flight.

    This routine is responsible for continuing or completing the offload write
    operation.

Arguments:

    OffloadWriteContext - Pointer to the OFFLOAD_WRITE_CONTEXT for the offload
        write operation.

Return Value:

    None.

--*/

{
    BOOLEAN allDataSetRangeFullyConverted;
    ULONG blockDescrIndex;
    PBLOCK_DEVICE_RANGE_DESCRIPTOR blockDescrPointer;
    PVOID buffer;
    ULONG bufferLength;
    ULONGLONG dataSetRangeByteOffset;
    ULONG dataSetRangeIndex;
    PDEVICE_DATA_SET_RANGE dataSetRanges;
    ULONG dataSetRangesCount;
    ULONGLONG entireXferLen;
    PDEVICE_OBJECT fdo;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt;
    PIRP irp;
    ULONG lbaCount;
    ULONG listIdentifier;
    ULONGLONG logicalBlockOffset;
    ULONG maxBlockDescrCount;
    ULONGLONG maxLbaCount;
    PDEVICE_DSM_OFFLOAD_WRITE_PARAMETERS offloadWriteParameters;
    PTRANSFER_PACKET pkt;
    PIRP pseudoIrp;
    NTSTATUS status;
    DEVICE_DATA_SET_RANGE tempDataSetRange;
    BOOLEAN tempDataSetRangeFullyConverted;
    PVOID tokenAscii;
    ULONG tokenSize;
    ULONGLONG totalSectorCount;
    ULONGLONG totalSectorsProcessedSuccessfully;
    ULONGLONG totalSectorsToProcess;
    ULONG transferSize;
    USHORT writeUsingTokenDataLength;
    USHORT writeUsingTokenDescriptorsLength;
    PMDL writeUsingTokenMdl;

    tokenAscii = NULL;
    tokenSize = BLOCK_DEVICE_TOKEN_SIZE;
    tempDataSetRangeFullyConverted = FALSE;
    allDataSetRangeFullyConverted = FALSE;
    fdo = OffloadWriteContext->Fdo;
    fdoExt = fdo->DeviceExtension;
    irp = OffloadWriteContext->OffloadWriteDsmIrp;
    buffer = OffloadWriteContext + 1;
    bufferLength = OffloadWriteContext->BufferLength;
    dataSetRanges = OffloadWriteContext->DataSetRanges;
    offloadWriteParameters = OffloadWriteContext->OffloadWriteParameters;
    pseudoIrp = &OffloadWriteContext->PseudoIrp;
    writeUsingTokenMdl = OffloadWriteContext->WriteUsingTokenMdl;
    entireXferLen = OffloadWriteContext->EntireXferLen;

    RtlZeroMemory(buffer, bufferLength);

    pkt = DequeueFreeTransferPacket(fdo, TRUE);
    if (!pkt){

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspContinueOffloadWrite (%p): Failed to retrieve transfer packet for TokenOperation (WriteUsingToken) operation.\n",
                    fdo));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto __ClasspContinueOffloadWrite_ErrorExit;
    }

    OffloadWriteContext->Pkt = pkt;

    blockDescrPointer = (PBLOCK_DEVICE_RANGE_DESCRIPTOR)
        &((PWRITE_USING_TOKEN_HEADER)buffer)->BlockDeviceRangeDescriptor[0];

    blockDescrIndex = 0;
    lbaCount = 0;

    totalSectorsToProcess = 0;

    maxBlockDescrCount = OffloadWriteContext->MaxBlockDescrCount;
    maxLbaCount = OffloadWriteContext->MaxLbaCount;

    //
    // The OffloadWriteContext->DataSetRangeIndex, DataSetRangeByteOffset, and
    // TotalSectorsProcessedSuccessfully don't move forward until RRTI has
    // reported the actual amount written.
    //
    // For that reason, this function only updates
    // OffloadWriteContext->TotalSectorsProcessed, which tracks the number of
    // sectors requested to be written by the current WRITE USING TOKEN command
    // (not all will necessarily be written).
    //

    dataSetRangeIndex = OffloadWriteContext->DataSetRangeIndex;
    dataSetRangesCount = OffloadWriteContext->DataSetRangesCount;
    dataSetRangeByteOffset = OffloadWriteContext->DataSetRangeByteOffset;
    totalSectorsProcessedSuccessfully = OffloadWriteContext->TotalSectorsProcessedSuccessfully;

    //
    // Send WriteUsingToken commands when the buffer is full or all input entries are converted.
    //
    while (!((blockDescrIndex == maxBlockDescrCount) ||        // buffer full or block descriptor count reached
             (lbaCount == maxLbaCount) ||                      // block LBA count reached
             (allDataSetRangeFullyConverted))) {               // all DataSetRanges have been converted

        NT_ASSERT(dataSetRangeIndex < dataSetRangesCount);
        NT_ASSERT(dataSetRangeByteOffset < dataSetRanges[dataSetRangeIndex].LengthInBytes);

        tempDataSetRange.StartingOffset = dataSetRanges[dataSetRangeIndex].StartingOffset + dataSetRangeByteOffset;
        tempDataSetRange.LengthInBytes = dataSetRanges[dataSetRangeIndex].LengthInBytes - dataSetRangeByteOffset;

        totalSectorCount = 0;

        ClasspConvertDataSetRangeToBlockDescr(fdo,
                                              blockDescrPointer,
                                              &blockDescrIndex,
                                              maxBlockDescrCount,
                                              &lbaCount,
                                              maxLbaCount,
                                              &tempDataSetRange,
                                              &totalSectorCount);

        tempDataSetRangeFullyConverted = (tempDataSetRange.LengthInBytes == 0) ? TRUE : FALSE;

        allDataSetRangeFullyConverted = tempDataSetRangeFullyConverted && ((dataSetRangeIndex + 1) == dataSetRangesCount);

        if (tempDataSetRangeFullyConverted) {
            dataSetRangeIndex += 1;
            dataSetRangeByteOffset = 0;
            NT_ASSERT(dataSetRangeIndex <= dataSetRangesCount);
        } else {
            dataSetRangeByteOffset += totalSectorCount * fdoExt->DiskGeometry.BytesPerSector;
            NT_ASSERT(dataSetRangeByteOffset < dataSetRanges[dataSetRangeIndex].LengthInBytes);
        }

        totalSectorsToProcess += totalSectorCount;
    }

    //
    // Save the number of sectors being attempted in this WRITE USING TOKEN
    // command, so that a success return from the command will know how much
    // was written, without needing to issue a RECEIVE ROD TOKEN INFORMATION
    // command.
    //
    OffloadWriteContext->TotalSectorsToProcess = totalSectorsToProcess;
    OffloadWriteContext->TotalSectorsProcessed = 0;

    //
    // Calculate transfer size, including the header
    //
    transferSize = (blockDescrIndex * sizeof(BLOCK_DEVICE_RANGE_DESCRIPTOR)) + FIELD_OFFSET(WRITE_USING_TOKEN_HEADER, BlockDeviceRangeDescriptor);

    NT_ASSERT(transferSize <= MAX_TOKEN_OPERATION_PARAMETER_DATA_LENGTH);

    writeUsingTokenDataLength = (USHORT)transferSize - RTL_SIZEOF_THROUGH_FIELD(WRITE_USING_TOKEN_HEADER, WriteUsingTokenDataLength);
    REVERSE_BYTES_SHORT(((PWRITE_USING_TOKEN_HEADER)buffer)->WriteUsingTokenDataLength, &writeUsingTokenDataLength);

    ((PWRITE_USING_TOKEN_HEADER)buffer)->Immediate = 0;

    logicalBlockOffset = OffloadWriteContext->LogicalBlockOffset + totalSectorsProcessedSuccessfully;
    REVERSE_BYTES_QUAD(((PWRITE_USING_TOKEN_HEADER)buffer)->BlockOffsetIntoToken, &logicalBlockOffset);

    RtlCopyMemory(((PWRITE_USING_TOKEN_HEADER)buffer)->Token,
                  &offloadWriteParameters->Token,
                  BLOCK_DEVICE_TOKEN_SIZE);

    writeUsingTokenDescriptorsLength = (USHORT)transferSize - FIELD_OFFSET(WRITE_USING_TOKEN_HEADER, BlockDeviceRangeDescriptor);
    REVERSE_BYTES_SHORT(((PWRITE_USING_TOKEN_HEADER)buffer)->BlockDeviceRangeDescriptorListLength, &writeUsingTokenDescriptorsLength);

    RtlZeroMemory(pseudoIrp, sizeof(IRP));

    pseudoIrp->IoStatus.Status = STATUS_SUCCESS;
    pseudoIrp->IoStatus.Information = 0;
    pseudoIrp->Tail.Overlay.DriverContext[0] = LongToPtr(1);
    pseudoIrp->MdlAddress = writeUsingTokenMdl;

    InterlockedCompareExchange((volatile LONG *)&TokenOperationListIdentifier, -1, MaxTokenOperationListIdentifier);
    listIdentifier = InterlockedIncrement((volatile LONG *)&TokenOperationListIdentifier);

    ClasspSetupWriteUsingTokenTransferPacket(
        OffloadWriteContext,
        pkt,
        transferSize,
        (PUCHAR)buffer,
        pseudoIrp,
        listIdentifier);

    tokenAscii = ClasspBinaryToAscii((PUCHAR)(((PWRITE_USING_TOKEN_HEADER)buffer)->Token),
                                     tokenSize,
                                     &tokenSize);

    TracePrint((TRACE_LEVEL_INFORMATION,
                TRACE_FLAG_IOCTL,
                "ClasspContinueOffloadWrite (%p): Offloading write for %I64u bytes (versus %I64u) [via %u descriptors]. \
                \n\t\t\tDataLength: %u, DescriptorsLength: %u. Pkt %p (list id %x) [Token: %s]\n",
                fdo,
                totalSectorsToProcess * fdoExt->DiskGeometry.BytesPerSector,
                entireXferLen,
                blockDescrIndex,
                writeUsingTokenDataLength,
                writeUsingTokenDescriptorsLength,
                pkt,
                listIdentifier,
                (tokenAscii == NULL) ? "" : tokenAscii));

    FREE_POOL(tokenAscii);

    OffloadWriteContext->ListIdentifier = listIdentifier;

    SubmitTransferPacket(pkt);

    //
    // ClasspWriteUsingTokenTransferPacketDone() takes care of completing the
    // IRP, so this function is done.
    //

    return;

    //
    // Error cleaup label only - not used in success case:
    //

__ClasspContinueOffloadWrite_ErrorExit:

    NT_ASSERT(!NT_SUCCESS(status));

    //
    // ClasspCompleteOffloadWrite also cleans up offloadWriteContext.
    //

    ClasspCompleteOffloadWrite(OffloadWriteContext, status);

    return;
}


VOID
ClasspAdvanceOffloadWritePosition(
    _In_ POFFLOAD_WRITE_CONTEXT OffloadWriteContext,
    _In_ ULONGLONG SectorsToAdvance
    )

/*++

Routine description:

    After the target has responded to WRITE USING TOKEN with success, or RRTI
    with a specific TRANSFER COUNT, this routine is used to update the relative
    position within the overall offload write request.  This position includes
    the TotalSectorsProcessedSuccessfully, the DataSetRangeIndex, and the
    DataSetRangeByteOffset.

    The caller is responsible for continuing or completing the offload write
    operation (this routine doesn't do that).

Arguments:

    OffloadWriteContext - Pointer to the OFFLOAD_WRITE_CONTEXT for the offload
        write operation.

    SectorsToAdvance - The number of sectors which were just written
        successfully (not the total for the offload write operation overall,
        just the number done by the most recent WRITE USING TOKEN).

Return Value:

    None.

--*/

{
    ULONGLONG bytesToAdvance;
    ULONGLONG bytesToDo;
    PULONGLONG dataSetRangeByteOffset;
    PULONG dataSetRangeIndex;
    PDEVICE_DATA_SET_RANGE dataSetRanges;
    PDEVICE_OBJECT fdo;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt;
    PULONGLONG totalSectorsProcessedSuccessfully;

    fdo = OffloadWriteContext->Fdo;
    fdoExt = fdo->DeviceExtension;
    dataSetRanges = OffloadWriteContext->DataSetRanges;
    dataSetRangeByteOffset = &OffloadWriteContext->DataSetRangeByteOffset;
    dataSetRangeIndex = &OffloadWriteContext->DataSetRangeIndex;
    totalSectorsProcessedSuccessfully = &OffloadWriteContext->TotalSectorsProcessedSuccessfully;
    bytesToAdvance = SectorsToAdvance * fdoExt->DiskGeometry.BytesPerSector;

    (*totalSectorsProcessedSuccessfully) += SectorsToAdvance;
    NT_ASSERT((*totalSectorsProcessedSuccessfully) <= OffloadWriteContext->TotalRequestSizeSectors);

    while (bytesToAdvance != 0) {
        bytesToDo = dataSetRanges[*dataSetRangeIndex].LengthInBytes - *dataSetRangeByteOffset;
        if (bytesToDo > bytesToAdvance) {
            bytesToDo = bytesToAdvance;
        }
        (*dataSetRangeByteOffset) += bytesToDo;
        bytesToAdvance -= bytesToDo;
        if ((*dataSetRangeByteOffset) == dataSetRanges[*dataSetRangeIndex].LengthInBytes) {
            (*dataSetRangeIndex) += 1;
            (*dataSetRangeByteOffset) = 0;
        }
    }

    NT_ASSERT((*dataSetRangeIndex) <= OffloadWriteContext->DataSetRangesCount);

    return;
}


VOID
ClasspWriteUsingTokenTransferPacketDone(
    _In_ PVOID Context
    )

/*++

Routine description:

    This routine continues an offload write operation when the WRITE USING
    TOKEN transfer packet completes.

    This routine may be able to determine that all requested sectors were
    written if the WRITE USING TOKEN completed with success, or may need to
    issue a RECEIVE ROD TOKEN INFORMATION if the WRITE USING TOKEN indicated
    check condition.

    This routine is responsible for continuing or completing the offload write
    operation.

Arguments:

    Context - Pointer to the OFFLOAD_WRITE_CONTEXT for the offload write
        operation.

Return Value:

    None.

--*/

{
    ULONGLONG entireXferLen;
    PDEVICE_OBJECT fdo;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt;
    ULONG listIdentifier;
    POFFLOAD_WRITE_CONTEXT offloadWriteContext;
    PTRANSFER_PACKET pkt;
    PIRP pseudoIrp;
    NTSTATUS status;
    PBOOLEAN tokenInvalidated;
    ULONGLONG totalSectorsToProcess;

    offloadWriteContext = Context;
    pseudoIrp = &offloadWriteContext->PseudoIrp;
    fdo = offloadWriteContext->Fdo;
    fdoExt = fdo->DeviceExtension;
    listIdentifier = offloadWriteContext->ListIdentifier;
    totalSectorsToProcess = offloadWriteContext->TotalSectorsToProcess;
    entireXferLen = offloadWriteContext->EntireXferLen;
    tokenInvalidated = &offloadWriteContext->TokenInvalidated;
    pkt = offloadWriteContext->Pkt;

    offloadWriteContext->Pkt = NULL;


    status = pseudoIrp->IoStatus.Status;
    NT_ASSERT(status != STATUS_PENDING);

    //
    // If the request failed with any of the following errors, then it is meaningless to send
    // down a ReceiveTokenInformation (regardless of whether the transfer was requested as sync
    // or async), since the target has no saved information about the command:
    // - STATUS_INVALID_TOKEN
    // - STATUS_INVALID_PARAMETER
    //
    if (status == STATUS_INVALID_PARAMETER ||
        status == STATUS_INVALID_TOKEN) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspWriteUsingTokenTransferPacketDone (%p): Write failed with %x (list id %x).\n",
                    fdo,
                    status,
                    listIdentifier));

        //
        // If the token isn't valid any longer, we should let the upper layers know so that
        // they don't waste time retrying the write with the same token.
        //
        if (status == STATUS_INVALID_TOKEN) {

            *tokenInvalidated = TRUE;
        }

        NT_ASSERT(status != STATUS_PENDING && !NT_SUCCESS(status));
        goto __ClasspWriteUsingTokenTransferPacketDone_Exit;

    } else if ((NT_SUCCESS(status)) &&
               (pkt->Srb->SrbStatus == SRB_STATUS_SUCCESS || pkt->TransferCount != 0)) {

        //
        // If the TokenOperation command was sent to the target requesting synchronous data
        // transfer, a success indicates that the command is complete.
        // This could either be because of a successful completion of the entire transfer
        // or because of a partial transfer due to target truncation. If it is the latter,
        // and the information field of the sense data has returned the TransferCount, we
        // can avoid sending down an RRTI.
        //
        if (pkt->Srb->SrbStatus == SRB_STATUS_SUCCESS) {

            //
            // The entire transfer has completed successfully.
            //
            offloadWriteContext->TotalSectorsProcessed = totalSectorsToProcess;
            TracePrint((TRACE_LEVEL_INFORMATION,
                        TRACE_FLAG_IOCTL,
                        "ClasspWriteUsingTokenTransferPacketDone (%p): Successfully wrote using token %I64u (out of %I64u) bytes (list Id %x).\n",
                        fdo,
                        totalSectorsToProcess * fdoExt->DiskGeometry.BytesPerSector,
                        entireXferLen,
                        listIdentifier));
        } else {

            //
            // The target has returned how much data it transferred in the response to the
            // WUT command itself, allowing us to optimize by removing the necessaity for
            // sending down an RRTI to query the TransferCount.
            //
            NT_ASSERT(pkt->TransferCount);

            offloadWriteContext->TotalSectorsProcessed = totalSectorsToProcess = pkt->TransferCount;
            TracePrint((TRACE_LEVEL_INFORMATION,
                        TRACE_FLAG_IOCTL,
                        "ClasspWriteUsingTokenTransferPacketDone (%p): Target truncated write using token %I64u (out of %I64u) bytes (list Id %x).\n",
                        fdo,
                        totalSectorsToProcess * fdoExt->DiskGeometry.BytesPerSector,
                        entireXferLen,
                        listIdentifier));
        }

        ClasspAdvanceOffloadWritePosition(offloadWriteContext, totalSectorsToProcess);

        NT_ASSERT(status != STATUS_PENDING);

        //
        // ClasspReceiveWriteUsingTokenInformationDone() takes care of
        // completing the operation (eventually), so pending from point of view
        // of this function.
        //

        ClasspReceiveWriteUsingTokenInformationDone(offloadWriteContext, status);
        status = STATUS_PENDING;

        goto __ClasspWriteUsingTokenTransferPacketDone_Exit;

    } else {

        //
        // Since the TokenOperation was failed (or the target truncated the transfer but
        // didn't indicate the amount), we need to send down ReceiveTokenInformation.
        //
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspWriteUsingTokenTransferPacketDone (%p): Write failed with status %x, %x (list id %x).\n",
                    fdo,
                    status,
                    pkt->Srb->SrbStatus,
                    listIdentifier));

        ClasspReceiveWriteUsingTokenInformation(offloadWriteContext);

        status = STATUS_PENDING;
        goto __ClasspWriteUsingTokenTransferPacketDone_Exit;
    }

__ClasspWriteUsingTokenTransferPacketDone_Exit:

    if (status != STATUS_PENDING) {
        ClasspCompleteOffloadWrite(offloadWriteContext, status);
    }

    return;
}


VOID
ClasspReceiveWriteUsingTokenInformationDone(
    _In_ POFFLOAD_WRITE_CONTEXT OffloadWriteContext,
    _In_ NTSTATUS CompletionCausingStatus
    )

/*++

Routine description:

    This routine continues an offload write operation when a WRITE USING TOKEN
    and possible associated RECEIVE ROD TOKEN INFORMATION have both fully
    completed and the RRTI has indicated completion of the WUT.

    This routine checks to see if the total sectors written is already equal to
    the overall total requested sector count, and if so, completes the offload
    write operation.  If not, this routine continues the operation by issuing
    another WUT.

    This routine is responsible for continuing or completing the offload write
    operation.

Arguments:

    OffloadWriteContext - Pointer to the OFFLOAD_WRITE_CONTEXT for the offload
        write operation.

Return Value:

    None.

--*/

{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = OffloadWriteContext->Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;

    //
    // Time taken in 100ns units.
    //
    ULONGLONG durationIn100ns = (KeQueryInterruptTime() - OffloadWriteContext->OperationStartTime);
    ULONGLONG maxTargetDuration = fdoData->CopyOffloadMaxTargetDuration * 10ULL * 1000 * 1000;

    NT_ASSERT(
        OffloadWriteContext->TotalSectorsProcessedSuccessfully <=
        OffloadWriteContext->TotalRequestSizeSectors);


    if (OffloadWriteContext->TotalSectorsProcessedSuccessfully == OffloadWriteContext->TotalRequestSizeSectors) {

        ClasspCompleteOffloadWrite(OffloadWriteContext, CompletionCausingStatus);

        goto __ClasspReceiveWriteUsingTokenInformationDone_Exit;
    }

    //
    // Since we don't want a layered timeout mechanism (e.g. guest and parent OS in Hyper-V scenarios)
    // to cause a SCSI timeout for the higher layer token operations.
    //
    if (maxTargetDuration <= durationIn100ns) {

        TracePrint((TRACE_LEVEL_WARNING,
                    TRACE_FLAG_IOCTL,
                    "ClasspReceiveWriteUsingTokenInformationDone (%p): Truncating write (list id %x) because of max-duration-rule.\n",
                    OffloadWriteContext->Fdo,
                    OffloadWriteContext->ListIdentifier));

        //
        // We could technically pass in STATUS_IO_OPERATION_TIMEOUT, but ClasspCompleteOffloadWrite
        // won't end up doing anything (useful) with this status, since some bytes would already
        // have been transferred.
        //
        ClasspCompleteOffloadWrite(OffloadWriteContext, STATUS_UNSUCCESSFUL);

        goto __ClasspReceiveWriteUsingTokenInformationDone_Exit;
    }

    NT_ASSERT(
        OffloadWriteContext->TotalSectorsProcessedSuccessfully <
        OffloadWriteContext->TotalRequestSizeSectors);

    //
    // Keep going with the next sub-request.
    //

    ClasspContinueOffloadWrite(OffloadWriteContext);

__ClasspReceiveWriteUsingTokenInformationDone_Exit:

    return;
}


VOID
ClasspCompleteOffloadWrite(
    _In_ __drv_freesMem(Mem) POFFLOAD_WRITE_CONTEXT OffloadWriteContext,
    _In_ NTSTATUS CompletionCausingStatus
    )

/*++

Routine description:

    This routine is used to complete an offload write operation.

    The input CompletionCausingStatus doesn't necessarily drive the completion
    status of the offload write operation overall, if the offload write
    operation overall has previously written some sectors successfully.

    This routine completes the offload write operation.

Arguments:

    OffloadWriteContext - Pointer to the OFFLOAD_WRITE_CONTEXT for the offload
        write operation.

    CompletionCausingStatus - Status code indicating a reason that the offload
        write operation is completing.  For success this will be STATUS_SUCCESS,
        but for a failure, this status will indicate what failure occurred.
        This status doesn't directly propagate to the completion status of the
        overall offload write operation if this status is failure and the
        overall offload write operation has already previously written some
        sectors successfully.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT fdo;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt;
    PDEVICE_MANAGE_DATA_SET_ATTRIBUTES dsmAttributes;
    PULONGLONG totalSectorsProcessedSuccessfully;
    ULONGLONG entireXferLen;
    PIRP irp;
    PBOOLEAN tokenInvalidated;
    ULONG listIdentifier;
    ULONGLONG totalSectorsProcessed;
    NTSTATUS status;
    ULONGLONG totalBytesProcessed;

    fdo = OffloadWriteContext->Fdo;
    fdoExt = fdo->DeviceExtension;
    dsmAttributes = OffloadWriteContext->DsmAttributes;
    totalSectorsProcessedSuccessfully = &OffloadWriteContext->TotalSectorsProcessedSuccessfully;
    entireXferLen = OffloadWriteContext->EntireXferLen;
    irp = OffloadWriteContext->OffloadWriteDsmIrp;
    tokenInvalidated = &OffloadWriteContext->TokenInvalidated;
    listIdentifier = OffloadWriteContext->ListIdentifier;
    totalSectorsProcessed = OffloadWriteContext->TotalSectorsProcessed;
    status = CompletionCausingStatus;

    ((PSTORAGE_OFFLOAD_WRITE_OUTPUT)dsmAttributes)->OffloadWriteFlags = 0;
    ((PSTORAGE_OFFLOAD_WRITE_OUTPUT)dsmAttributes)->Reserved = 0;

    totalBytesProcessed = (*totalSectorsProcessedSuccessfully) * fdoExt->DiskGeometry.BytesPerSector;

    TracePrint((totalBytesProcessed == entireXferLen ? TRACE_LEVEL_INFORMATION : TRACE_LEVEL_WARNING,
                TRACE_FLAG_IOCTL,
                "ClasspCompleteOffloadWrite (%p): %ws wrote using token %I64u (out of %I64u) bytes (Irp %p).\n",
                fdo,
                NT_SUCCESS(status) ? L"Successful" : L"Failed",
                totalBytesProcessed,
                entireXferLen,
                irp));

    if (totalBytesProcessed > 0 && totalBytesProcessed < entireXferLen) {
        SET_FLAG(((PSTORAGE_OFFLOAD_WRITE_OUTPUT)dsmAttributes)->OffloadWriteFlags, STORAGE_OFFLOAD_WRITE_RANGE_TRUNCATED);
    }
    if (*tokenInvalidated) {
        SET_FLAG(((PSTORAGE_OFFLOAD_WRITE_OUTPUT)dsmAttributes)->OffloadWriteFlags, STORAGE_OFFLOAD_TOKEN_INVALID);
    }
    ((PSTORAGE_OFFLOAD_WRITE_OUTPUT)dsmAttributes)->LengthCopied = totalBytesProcessed;


    if (!NT_SUCCESS(status)) {

        TracePrint((TRACE_LEVEL_WARNING,
                    TRACE_FLAG_IOCTL,
                    "ClasspCompleteOffloadWrite (%p): TokenOperation for WriteUsingToken (list Id %u) completed with %x writing %I64u blocks (currentTotal %I64u blocks).\n",
                    fdo,
                    listIdentifier,
                    status,
                    totalSectorsProcessed,
                    *totalSectorsProcessedSuccessfully));

        //
        // Even if target returned an error, from the OS upper layers' perspective,
        // it is a success (with truncation) if any data at all was written.
        //
        if (*totalSectorsProcessedSuccessfully) {
            status = STATUS_SUCCESS;
        }
    }

    irp->IoStatus.Information = sizeof(STORAGE_OFFLOAD_WRITE_OUTPUT);

    ClasspCompleteOffloadRequest(fdo, irp, status);
    ClasspCleanupOffloadWriteContext(OffloadWriteContext);
    OffloadWriteContext = NULL;

    return;
}


VOID
ClasspCleanupOffloadWriteContext(
    _In_ __drv_freesMem(mem) POFFLOAD_WRITE_CONTEXT OffloadWriteContext
    )

/*++

Routine description:

    This routine cleans up an offload write context.

Arguments:

    OffloadWriteContext - Pointer to the OFFLOAD_WRITE_CONTEXT for the offload
        write operation.

Return Value:

    None.

--*/

{
    PMDL writeUsingTokenMdl = OffloadWriteContext->WriteUsingTokenMdl;

    if (writeUsingTokenMdl) {
        ClasspFreeDeviceMdl(writeUsingTokenMdl);
    }
    FREE_POOL(OffloadWriteContext);

    return;
}


_IRQL_requires_same_
VOID
ClasspReceiveWriteUsingTokenInformation(
    _In_ POFFLOAD_WRITE_CONTEXT OffloadWriteContext
    )

/*++

Routine description:

    This routine retrieves the token after a WriteUsingToken command
    has been sent down in case of an error or if there is a need to
    poll for the result.

    This routine is responsible for continuing or completing the offload write
    operation.

Arguments:

    OffloadWriteContext - Pointer to the OFFLOAD_WRITE_CONTEXT for the offload
        write operation.

Return Value:

    None.

--*/

{
    PVOID buffer;
    ULONG bufferLength;
    ULONG cdbLength;
    PDEVICE_OBJECT fdo;
    PIRP irp;
    ULONG listIdentifier;
    PTRANSFER_PACKET pkt;
    PIRP pseudoIrp;
    ULONG receiveTokenInformationBufferLength;
    PSCSI_REQUEST_BLOCK srb;
    NTSTATUS status;
    ULONG tempSizeUlong;
    PMDL writeUsingTokenMdl;

    fdo = OffloadWriteContext->Fdo;
    irp = OffloadWriteContext->OffloadWriteDsmIrp;
    pseudoIrp = &OffloadWriteContext->PseudoIrp;
    buffer = OffloadWriteContext + 1;
    bufferLength = OffloadWriteContext->BufferLength;
    receiveTokenInformationBufferLength = OffloadWriteContext->ReceiveTokenInformationBufferLength;
    writeUsingTokenMdl = OffloadWriteContext->WriteUsingTokenMdl;
    listIdentifier = OffloadWriteContext->ListIdentifier;
    srb = &OffloadWriteContext->Srb;
    status = STATUS_SUCCESS;

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspReceiveWriteUsingTokenInformation (%p): Entering function. Irp %p\n",
                fdo,
                irp));

    //
    // The WRITE USING TOKEN wasn't immediately fully successful, so that means
    // the only way to find out how many sectors were processed by the WRITE
    // USING TOKEN is to get a successful RECEIVE ROD TOKEN INFORMATION that
    // indicates the operation is complete.
    //

    NT_ASSERT(OffloadWriteContext->TotalSectorsProcessed == 0);

    pkt = DequeueFreeTransferPacket(fdo, TRUE);

    if (!pkt) {

        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspReceiveWriteUsingTokenInformation (%p): Failed to retrieve transfer packet for ReceiveTokenInformation (WriteUsingToken) operation.\n",
                    fdo));

        status = STATUS_INSUFFICIENT_RESOURCES;

        goto __ClasspReceiveWriteUsingTokenInformation_ErrorExit;
    }

    RtlZeroMemory(buffer, bufferLength);

    tempSizeUlong = receiveTokenInformationBufferLength - 4;
    REVERSE_BYTES(((PRECEIVE_TOKEN_INFORMATION_HEADER)buffer)->AvailableData, &tempSizeUlong);

    RtlZeroMemory(pseudoIrp, sizeof(IRP));

    pseudoIrp->IoStatus.Status = STATUS_SUCCESS;
    pseudoIrp->IoStatus.Information = 0;
    pseudoIrp->Tail.Overlay.DriverContext[0] = LongToPtr(1);
    pseudoIrp->MdlAddress = writeUsingTokenMdl;

    ClasspSetupReceiveWriteUsingTokenInformationTransferPacket(
        OffloadWriteContext,
        pkt,
        bufferLength,
        (PUCHAR)buffer,
        pseudoIrp,
        listIdentifier);

    //
    // Cache away the CDB as it may be required for forwarded sense data
    // after this command completes.
    //
    RtlZeroMemory(srb, sizeof(*srb));
    cdbLength = SrbGetCdbLength(pkt->Srb);
    if (cdbLength <= 16) {
        RtlCopyMemory(&srb->Cdb, SrbGetCdb(pkt->Srb), cdbLength);
    }

    SubmitTransferPacket(pkt);

    return;

    //
    // Error label only - not used by success cases:
    //

__ClasspReceiveWriteUsingTokenInformation_ErrorExit:

    NT_ASSERT(!NT_SUCCESS(status));

    //
    // ClasspCompleteOffloadWrite also cleans up OffloadWriteContext.
    //

    ClasspCompleteOffloadWrite(OffloadWriteContext, status);

    return;
}


VOID
ClasspReceiveWriteUsingTokenInformationTransferPacketDone(
    _In_ POFFLOAD_WRITE_CONTEXT OffloadWriteContext
    )

/*++

Routine description:

    This routine continues an offload write operation when a RECEIVE ROD TOKEN
    INFORMATION transfer packet is done.

    This routine may need to send another RRTI, or it may be able to indicate
    that this WUT is done via call to
    ClasspReceiveWriteUsingTokenInformationDone().

    This routine is responsible for continuing or completing the offload write
    operation.

Arguments:

    OffloadWriteContext - Pointer to the OFFLOAD_WRITE_CONTEXT for the offload
        write operation.

Return Value:

    None.

--*/

{
    ULONG availableData;
    PVOID buffer;
    UCHAR completionStatus;
    ULONG estimatedRetryInterval;
    PDEVICE_OBJECT fdo;
    PFUNCTIONAL_DEVICE_EXTENSION fdoExt;
    PIRP irp;
    ULONG listIdentifier;
    BOOLEAN operationCompleted;
    UCHAR operationStatus;
    PIRP pseudoIrp;
    USHORT segmentsProcessed;
    PSENSE_DATA senseData;
    ULONG senseDataFieldLength;
    UCHAR senseDataLength;
    PSCSI_REQUEST_BLOCK srb;
    NTSTATUS status;
    ULONG tokenDescriptorLength;
    PRECEIVE_TOKEN_INFORMATION_HEADER tokenInformationResults;
    PRECEIVE_TOKEN_INFORMATION_RESPONSE_HEADER tokenInformationResponsePadding;
    PBOOLEAN tokenInvalidated;
    PULONGLONG totalSectorsProcessed;
    ULONGLONG totalSectorsToProcess;
    ULONGLONG transferBlockCount;

    fdo = OffloadWriteContext->Fdo;
    fdoExt = fdo->DeviceExtension;
    listIdentifier = OffloadWriteContext->ListIdentifier;
    totalSectorsProcessed = &OffloadWriteContext->TotalSectorsProcessed;
    totalSectorsToProcess = OffloadWriteContext->TotalSectorsToProcess;
    irp = OffloadWriteContext->OffloadWriteDsmIrp;
    pseudoIrp = &OffloadWriteContext->PseudoIrp;
    tokenInvalidated = &OffloadWriteContext->TokenInvalidated;
    srb = &OffloadWriteContext->Srb;
    operationCompleted = FALSE;
    buffer = OffloadWriteContext + 1;
    tokenInformationResults = (PRECEIVE_TOKEN_INFORMATION_HEADER)buffer;
    senseData = (PSENSE_DATA)((PUCHAR)tokenInformationResults + FIELD_OFFSET(RECEIVE_TOKEN_INFORMATION_HEADER, SenseData));
    transferBlockCount = 0;
    tokenInformationResponsePadding = NULL;
    tokenDescriptorLength = 0;

    NT_ASSERT((*totalSectorsProcessed) == 0);

    OffloadWriteContext->Pkt = NULL;


    status = pseudoIrp->IoStatus.Status;
    NT_ASSERT(status != STATUS_PENDING);

    //
    // The buffer we hand allows for the max sizes for all the fields whereas the returned
    // data may be lesser (e.g. sense data info will almost never be MAX_SENSE_BUFFER_SIZE, etc.
    // so handle underrun "error"
    //
    if (status == STATUS_DATA_OVERRUN) {

        status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(status)) {
        TracePrint((TRACE_LEVEL_ERROR,
                    TRACE_FLAG_IOCTL,
                    "ClasspReceiveWriteUsingTokenInformationTransferPacketDone (%p): Failed with %x to retrieve write results for list Id %x for data size %I64u bytes.\n",
                    fdo,
                    status,
                    listIdentifier,
                    totalSectorsToProcess * fdoExt->DiskGeometry.BytesPerSector));

        NT_ASSERT((*totalSectorsProcessed) == 0);

        goto __ClasspReceiveWriteUsingTokenInformationTransferPacketDone_ErrorExit;
    }

    REVERSE_BYTES(&availableData, &tokenInformationResults->AvailableData);

    NT_ASSERT(availableData <= FIELD_OFFSET(RECEIVE_TOKEN_INFORMATION_HEADER, SenseData) + MAX_SENSE_BUFFER_SIZE);

    NT_ASSERT(tokenInformationResults->ResponseToServiceAction == SERVICE_ACTION_WRITE_USING_TOKEN);

    operationStatus = tokenInformationResults->OperationStatus;
    operationCompleted = ClasspIsTokenOperationComplete(operationStatus);
    NT_ASSERT(operationCompleted);

    REVERSE_BYTES(&estimatedRetryInterval, &tokenInformationResults->EstimatedStatusUpdateDelay);

    completionStatus = tokenInformationResults->CompletionStatus;

    senseDataFieldLength = tokenInformationResults->SenseDataFieldLength;
    senseDataLength = tokenInformationResults->SenseDataLength;
    NT_ASSERT(senseDataFieldLength >= senseDataLength);

    tokenInformationResponsePadding = (PRECEIVE_TOKEN_INFORMATION_RESPONSE_HEADER)((PUCHAR)tokenInformationResults +
                                                                                           FIELD_OFFSET(RECEIVE_TOKEN_INFORMATION_HEADER, SenseData) +
                                                                                           tokenInformationResults->SenseDataFieldLength);

    REVERSE_BYTES(&tokenDescriptorLength, &tokenInformationResponsePadding->TokenDescriptorsLength);
    NT_ASSERT(tokenDescriptorLength == 0);

    NT_ASSERT(tokenInformationResults->TransferCountUnits == TRANSFER_COUNT_UNITS_NUMBER_BLOCKS);
    REVERSE_BYTES_QUAD(&transferBlockCount, &tokenInformationResults->TransferCount);

    REVERSE_BYTES_SHORT(&segmentsProcessed, &tokenInformationResults->SegmentsProcessed);
    NT_ASSERT(segmentsProcessed == 0);

    if (operationCompleted) {

        if (transferBlockCount > totalSectorsToProcess) {

            //
            // Buggy or hostile target.  Don't let it claim more was procesed
            // than was requested.  Since this is likely a bug and it's unknown
            // how much was actually transferred, assume no data was
            // transferred.
            //

            NT_ASSERT(transferBlockCount <= totalSectorsToProcess);
            transferBlockCount = 0;
        }

        if (operationStatus != OPERATION_COMPLETED_WITH_SUCCESS &&
            operationStatus != OPERATION_COMPLETED_WITH_RESIDUAL_DATA) {

            //
            // Assert on buggy response from target, but in any case, make sure not
            // to claim that any data was written.
            //

            NT_ASSERT(transferBlockCount == 0);
            transferBlockCount = 0;
        }

        //
        // Since the TokenOperation was sent down synchronously but failed, the operation is complete as soon as the
        // ReceiveTokenInformation command returns.
        //

        NT_ASSERT((*totalSectorsProcessed) == 0);
        *totalSectorsProcessed = transferBlockCount;
        ClasspAdvanceOffloadWritePosition(OffloadWriteContext, transferBlockCount);

        if (transferBlockCount < totalSectorsToProcess) {

            NT_ASSERT(operationStatus == OPERATION_COMPLETED_WITH_RESIDUAL_DATA ||
                      operationStatus == OPERATION_COMPLETED_WITH_ERROR ||
                      operationStatus == OPERATION_TERMINATED);

        } else {

            NT_ASSERT(operationStatus == OPERATION_COMPLETED_WITH_SUCCESS);
        }

        TracePrint((transferBlockCount == totalSectorsToProcess ? TRACE_LEVEL_INFORMATION : TRACE_LEVEL_WARNING,
                    TRACE_FLAG_IOCTL,
                    "ClasspReceiveWriteUsingTokenInformationTransferPacketDone (%p): %wsSuccessfully wrote (for list Id %x) for data size %I64u bytes\n",
                    fdo,
                    transferBlockCount == totalSectorsToProcess ? L"" : L"Target truncated write. ",
                    listIdentifier,
                    (*totalSectorsProcessed) * fdoExt->DiskGeometry.BytesPerSector));

        //
        // Operation that completes with success can have sense data (for target to pass on some extra info)
        // but we don't care about such sense info.
        // Operation that complete but not with success, may not have sense data associated, but may
        // have valid CompletionStatus.
        //
        // The "status" may be overriden by ClassInterpretSenseInfo().  Final
        // status is determined a bit later - this is just the default status
        // when ClassInterpretSenseInfo() doesn't get to run here.
        //
        status = STATUS_SUCCESS;
        if (operationStatus == OPERATION_COMPLETED_WITH_ERROR ||
            operationStatus == OPERATION_COMPLETED_WITH_RESIDUAL_DATA ||
            operationStatus == OPERATION_TERMINATED) {

            SrbSetScsiStatus((PSTORAGE_REQUEST_BLOCK_HEADER)srb, completionStatus);

            if (senseDataLength) {

                ULONG retryInterval;

                NT_ASSERT(senseDataLength <= sizeof(SENSE_DATA));

                srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

                srb->SrbStatus = SRB_STATUS_AUTOSENSE_VALID | SRB_STATUS_ERROR;
                SrbSetSenseInfoBuffer((PSTORAGE_REQUEST_BLOCK_HEADER)srb, senseData);
                SrbSetSenseInfoBufferLength((PSTORAGE_REQUEST_BLOCK_HEADER)srb, senseDataLength);

                ClassInterpretSenseInfo(fdo,
                                        srb,
                                        IRP_MJ_SCSI,
                                        0,
                                        0,
                                        &status,
                                        &retryInterval);

                TracePrint((TRACE_LEVEL_WARNING,
                            TRACE_FLAG_IOCTL,
                            "ClasspReceiveWriteUsingTokenInformationTransferPacketDone (%p): Reason for truncation/failure: %x - for list Id %x for data size %I64u bytes.\n",
                            fdo,
                            status,
                            listIdentifier,
                            transferBlockCount * fdoExt->DiskGeometry.BytesPerSector));

                //
                // If the token isn't valid any longer, we should let the upper layers know so that
                // they don't waste time retrying the write with the same token.
                //
                if (status == STATUS_INVALID_TOKEN) {

                    *tokenInvalidated = TRUE;
                }
            } else {

                TracePrint((TRACE_LEVEL_WARNING,
                            TRACE_FLAG_IOCTL,
                            "ClasspReceiveWriteUsingTokenInformationTransferPacketDone (%p): No sense data available but reason for truncation/failure, possibly: %x - for list Id %x for data size %I64u bytes.\n",
                            fdo,
                            completionStatus,
                            listIdentifier,
                            transferBlockCount * fdoExt->DiskGeometry.BytesPerSector));
            }
        }

        //
        // Initialize status. Upper layer needs to know if command failed because it
        // timed out without doing any writing.  ClasspCompleteOffloadWrite() will
        // force status to success if any data was written, so it's this function's
        // job to set the status appropriately based on the outcome of this
        // WRITE USING TOKEN command, and then ClasspCompleteOffloadWrite()
        // can override with success if previos WRITE USING TOKEN commands
        // issued for the same upper request were able to write some data.
        //
        if (transferBlockCount != 0) {
            status = STATUS_SUCCESS;
        } else {

            if (NT_SUCCESS(status)) {
                //
                // Make sure status is a failing status, without throwing away an
                // already-failing status obtained from sense data.
                //
                status = STATUS_UNSUCCESSFUL;
            }
        }

        NT_ASSERT(status != STATUS_PENDING);

        if (!NT_SUCCESS(status)) {
            goto __ClasspReceiveWriteUsingTokenInformationTransferPacketDone_ErrorExit;
        }

        ClasspReceiveWriteUsingTokenInformationDone(OffloadWriteContext, status);
        status = STATUS_PENDING;
        goto __ClasspReceiveWriteUsingTokenInformationTransferPacketDone_Exit;

    } else {

        status = STATUS_UNSUCCESSFUL;

        goto __ClasspReceiveWriteUsingTokenInformationTransferPacketDone_ErrorExit;
    }

    //
    // Error label only - not used in success case:
    //

__ClasspReceiveWriteUsingTokenInformationTransferPacketDone_ErrorExit:

    NT_ASSERT(!NT_SUCCESS(status));

    ClasspCompleteOffloadWrite(OffloadWriteContext, status);

__ClasspReceiveWriteUsingTokenInformationTransferPacketDone_Exit:

    //
    // Due to tracing a potentially freed pointer value "Irp", this trace could
    // be delayed beyond another offload op picking up the same pointer value
    // for its Irp.
    //

    TracePrint((TRACE_LEVEL_VERBOSE,
                TRACE_FLAG_IOCTL,
                "ClasspReceiveWriteUsingTokenInformationTransferPacketDone (%p): Exiting function (Irp %p) with status %x.\n",
                fdo,
                irp,
                status));

    return;
}


NTSTATUS
ClasspRefreshFunctionSupportInfo(
    _Inout_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _In_ BOOLEAN ForceQuery
    )
/*
Routine Description:

    This function is to update various properties described in FDO extension's
    CLASS_FUNCTION_SUPPORT_INFO structure by requerying the device's VPD pages.
    Although this function is capable of updating any properties in the
    CLASS_FUNCTION_SUPPORT_INFO structure, it will initially support only
    a small number of proporties in the block limit data

Arguments:

    FdoExtension : FDO extension

    ForceQuery : TRUE if the caller wants to force a query of the device's
        VPD pages.  Otherwise, the function may use cached data.

Return Value:

    STATUS_SUCCESS or an error status

--*/
{
    NTSTATUS            status;
    PSCSI_REQUEST_BLOCK srb = NULL;
    ULONG               srbSize;
    CLASS_VPD_B0_DATA   blockLimitsDataNew;
    PCLASS_VPD_B0_DATA  blockLimitsDataOriginal;
    KLOCK_QUEUE_HANDLE  lockHandle;
    ULONG               generationCount;
    ULONG               changeRequestCount;

    //
    // ChangeRequestCount is incremented every time we get an unit attention with
    // SCSI_ADSENSE_OPERATING_CONDITIONS_CHANGED.  GenerationCount will be set to
    // ChangeRequestCount after CLASS_FUNCTION_SUPPORT_INFO is refreshed with the latest
    // VPD data.  i.e.  if both values are the same, data in
    // CLASS_FUNCTION_SUPPORT_INFO is current
    //

    generationCount = FdoExtension->FunctionSupportInfo->GenerationCount;
    changeRequestCount = FdoExtension->FunctionSupportInfo->ChangeRequestCount;
    if (!ForceQuery && generationCount == changeRequestCount) {
        return STATUS_SUCCESS;
    }

    //
    // Allocate an SRB for querying the device for LBP-related info if either
    // the Logical Block Provisioning (0xB2) or Block Limits (0xB0) VPD page
    // exists.
    //
    if ((FdoExtension->AdapterDescriptor != NULL) &&
        (FdoExtension->AdapterDescriptor->SrbType == SRB_TYPE_STORAGE_REQUEST_BLOCK)) {
        srbSize = CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE;
    } else {
        srbSize = sizeof(SCSI_REQUEST_BLOCK);
    }

    srb = ExAllocatePoolWithTag(NonPagedPoolNx, srbSize, '1DcS');
    if (srb == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ClasspDeviceGetBlockLimitsVPDPage(FdoExtension,
                                               srb,
                                               srbSize,
                                               &blockLimitsDataNew);

    if (NT_SUCCESS(status)) {

        KeAcquireInStackQueuedSpinLock(&FdoExtension->FunctionSupportInfo->SyncLock, &lockHandle);

        //
        // If the generationCount didn't change since we looked at it last time, it means
        // no one has tried to update the CLASS_FUNCTION_SUPPORT_INFO data; otherwise, someone
        // else has beat us to it.
        //
        if (generationCount == FdoExtension->FunctionSupportInfo->GenerationCount) {

            blockLimitsDataOriginal = &FdoExtension->FunctionSupportInfo->BlockLimitsData;
            if (blockLimitsDataOriginal->CommandStatus == -1) {
                //
                // CommandStatus == -1 means this is the first time we have
                // gotten the block limits data.
                //
                *blockLimitsDataOriginal = blockLimitsDataNew;
            } else {
                //
                // We only expect the Optimal Unmap Granularity (and alignment)
                // to change, so those are the only parameters we update.
                //
                blockLimitsDataOriginal->UGAVALID = blockLimitsDataNew.UGAVALID;
                blockLimitsDataOriginal->UnmapGranularityAlignment = blockLimitsDataNew.UnmapGranularityAlignment;
                blockLimitsDataOriginal->OptimalUnmapGranularity = blockLimitsDataNew.OptimalUnmapGranularity;
            }
            FdoExtension->FunctionSupportInfo->GenerationCount = changeRequestCount;
        }

        KeReleaseInStackQueuedSpinLock(&lockHandle);
    }

    FREE_POOL(srb);
    return status;
}

NTSTATUS
ClasspBlockLimitsDataSnapshot(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _In_ BOOLEAN ForceQuery,
    _Out_ PCLASS_VPD_B0_DATA BlockLimitsData,
    _Out_ PULONG GenerationCount
    )
/*
Routine Description:

    This function is to get a copy of the latest block limits data.

    When this function is called multiple times, GenerationCount can change (value always goes up)
    while BlockLimitsData stays the same.  In this case, the caller should assume BlockLimitsData
    has changed to different values and eventually changed back to the same state when the first
    call to this function was made.

Arguments:

    FdoExtension : FDO extension

    ForceQuery : TRUE if the caller wants to force a query of the device's
        VPD pages.  Otherwise, the function may use cached data.

    BlockLimitsData : pointer to memory that will receive the block limits data

    GenerationCount : generation count of the block limit data.

    DataIsOutdated: set to TRUE if the BlockLimitsData is old but this function fails to
    query the latest data from the device due to insufficient resources

Return Value:

    STATUS_SUCCESS or an error status

--*/
{
    NTSTATUS            status;
    KLOCK_QUEUE_HANDLE  lockHandle;

    status = ClasspRefreshFunctionSupportInfo(FdoExtension, ForceQuery);

    KeAcquireInStackQueuedSpinLock(&FdoExtension->FunctionSupportInfo->SyncLock, &lockHandle);
    *BlockLimitsData = FdoExtension->FunctionSupportInfo->BlockLimitsData;
    *GenerationCount = FdoExtension->FunctionSupportInfo->GenerationCount;
    KeReleaseInStackQueuedSpinLock(&lockHandle);

    return status;
}

