/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/driver.c
 * PURPOSE:         Driver Object Management
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Filip Navara (navaraf@reactos.org)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ERESOURCE IopDriverLoadResource;

LIST_ENTRY DriverReinitListHead;
KSPIN_LOCK DriverReinitListLock;
PLIST_ENTRY DriverReinitTailEntry;

PLIST_ENTRY DriverBootReinitTailEntry;
LIST_ENTRY DriverBootReinitListHead;
KSPIN_LOCK DriverBootReinitListLock;

UNICODE_STRING IopHardwareDatabaseKey =
   RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\HARDWARE\\DESCRIPTION\\SYSTEM");
static const WCHAR ServicesKeyName[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\";

POBJECT_TYPE IoDriverObjectType = NULL;

extern BOOLEAN ExpInTextModeSetup;
extern BOOLEAN PnpSystemInit;
extern BOOLEAN PnPBootDriversLoaded;
extern KEVENT PiEnumerationFinished;

USHORT IopGroupIndex;
PLIST_ENTRY IopGroupTable;

/* TYPES *********************************************************************/

// Parameters packet for Load/Unload work item's context
typedef struct _LOAD_UNLOAD_PARAMS
{
    NTSTATUS Status;
    PUNICODE_STRING RegistryPath;
    WORK_QUEUE_ITEM WorkItem;
    KEVENT Event;
    PDRIVER_OBJECT DriverObject;
    BOOLEAN SetEvent;
} LOAD_UNLOAD_PARAMS, *PLOAD_UNLOAD_PARAMS;

NTSTATUS
IopDoLoadUnloadDriver(
    _In_opt_ PUNICODE_STRING RegistryPath,
    _Inout_ PDRIVER_OBJECT *DriverObject);

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
IopInvalidDeviceRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_INVALID_DEVICE_REQUEST;
}

VOID
NTAPI
IopDeleteDriver(IN PVOID ObjectBody)
{
    PDRIVER_OBJECT DriverObject = ObjectBody;
    PIO_CLIENT_EXTENSION DriverExtension, NextDriverExtension;
    PAGED_CODE();

    DPRINT1("Deleting driver object '%wZ'\n", &DriverObject->DriverName);

    /* There must be no device objects remaining at this point */
    ASSERT(!DriverObject->DeviceObject);

    /* Get the extension and loop them */
    DriverExtension = IoGetDrvObjExtension(DriverObject)->ClientDriverExtension;
    while (DriverExtension)
    {
        /* Get the next one */
        NextDriverExtension = DriverExtension->NextExtension;
        ExFreePoolWithTag(DriverExtension, TAG_DRIVER_EXTENSION);

        /* Move on */
        DriverExtension = NextDriverExtension;
    }

    /* Check if the driver image is still loaded */
    if (DriverObject->DriverSection)
    {
        /* Unload it */
        MmUnloadSystemImage(DriverObject->DriverSection);
    }

    /* Check if it has a name */
    if (DriverObject->DriverName.Buffer)
    {
        /* Free it */
        ExFreePool(DriverObject->DriverName.Buffer);
    }

    /* Check if it has a service key name */
    if (DriverObject->DriverExtension->ServiceKeyName.Buffer)
    {
        /* Free it */
        ExFreePool(DriverObject->DriverExtension->ServiceKeyName.Buffer);
    }
}

NTSTATUS
IopGetDriverNames(
    _In_ HANDLE ServiceHandle,
    _Out_ PUNICODE_STRING DriverName,
    _Out_opt_ PUNICODE_STRING ServiceName)
{
    UNICODE_STRING driverName = {.Buffer = NULL}, serviceName;
    PKEY_VALUE_FULL_INFORMATION kvInfo;
    NTSTATUS status;

    PAGED_CODE();

    /* 1. Check the "ObjectName" field in the driver's registry key (it has priority) */
    status = IopGetRegistryValue(ServiceHandle, L"ObjectName", &kvInfo);
    if (NT_SUCCESS(status))
    {
        /* We've got the ObjectName, use it as the driver name */
        if (kvInfo->Type != REG_SZ || kvInfo->DataLength == 0)
        {
            ExFreePool(kvInfo);
            return STATUS_ILL_FORMED_SERVICE_ENTRY;
        }

        driverName.Length = kvInfo->DataLength - sizeof(UNICODE_NULL);
        driverName.MaximumLength = kvInfo->DataLength;
        driverName.Buffer = ExAllocatePoolWithTag(NonPagedPool, driverName.MaximumLength, TAG_IO);
        if (!driverName.Buffer)
        {
            ExFreePool(kvInfo);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlMoveMemory(driverName.Buffer,
                      (PVOID)((ULONG_PTR)kvInfo + kvInfo->DataOffset),
                      driverName.Length);
        driverName.Buffer[driverName.Length / sizeof(WCHAR)] = UNICODE_NULL;
        ExFreePool(kvInfo);
    }

    /* Check whether we need to get ServiceName as well, either to construct
     * the driver name (because we could not use "ObjectName"), or because
     * it is requested by the caller. */
    PKEY_BASIC_INFORMATION basicInfo = NULL;
    if (!NT_SUCCESS(status) || ServiceName != NULL)
    {
        /* Retrieve the necessary buffer size */
        ULONG infoLength;
        status = ZwQueryKey(ServiceHandle, KeyBasicInformation, NULL, 0, &infoLength);
        if (status != STATUS_BUFFER_TOO_SMALL)
        {
            status = (NT_SUCCESS(status) ? STATUS_UNSUCCESSFUL : status);
            goto Cleanup;
        }

        /* Allocate the buffer and retrieve the data */
        basicInfo = ExAllocatePoolWithTag(PagedPool, infoLength, TAG_IO);
        if (!basicInfo)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        status = ZwQueryKey(ServiceHandle, KeyBasicInformation, basicInfo, infoLength, &infoLength);
        if (!NT_SUCCESS(status))
        {
            goto Cleanup;
        }

        serviceName.Length = basicInfo->NameLength;
        serviceName.MaximumLength = basicInfo->NameLength;
        serviceName.Buffer = basicInfo->Name;
    }

    /* 2. There is no "ObjectName" - construct it ourselves. Depending on the driver type,
     * it will be either "\Driver\<ServiceName>" or "\FileSystem\<ServiceName>" */
    if (driverName.Buffer == NULL)
    {
        ASSERT(basicInfo); // Container for serviceName

        /* Retrieve the driver type */
        ULONG driverType;
        status = IopGetRegistryValue(ServiceHandle, L"Type", &kvInfo);
        if (!NT_SUCCESS(status))
        {
            goto Cleanup;
        }
        if (kvInfo->Type != REG_DWORD || kvInfo->DataLength != sizeof(ULONG))
        {
            ExFreePool(kvInfo);
            status = STATUS_ILL_FORMED_SERVICE_ENTRY;
            goto Cleanup;
        }

        RtlMoveMemory(&driverType,
                      (PVOID)((ULONG_PTR)kvInfo + kvInfo->DataOffset),
                      sizeof(ULONG));
        ExFreePool(kvInfo);

        /* Compute the necessary driver name string size */
        if (driverType == SERVICE_RECOGNIZER_DRIVER || driverType == SERVICE_FILE_SYSTEM_DRIVER)
            driverName.MaximumLength = sizeof(FILESYSTEM_ROOT_NAME);
        else
            driverName.MaximumLength = sizeof(DRIVER_ROOT_NAME);

        driverName.MaximumLength += serviceName.Length;
        driverName.Length = 0;

        /* Allocate and build it */
        driverName.Buffer = ExAllocatePoolWithTag(NonPagedPool, driverName.MaximumLength, TAG_IO);
        if (!driverName.Buffer)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        if (driverType == SERVICE_RECOGNIZER_DRIVER || driverType == SERVICE_FILE_SYSTEM_DRIVER)
            RtlAppendUnicodeToString(&driverName, FILESYSTEM_ROOT_NAME);
        else
            RtlAppendUnicodeToString(&driverName, DRIVER_ROOT_NAME);

        RtlAppendUnicodeStringToString(&driverName, &serviceName);
    }

    if (ServiceName != NULL)
    {
        ASSERT(basicInfo); // Container for serviceName

        /* Allocate a copy for the caller */
        PWCHAR buf = ExAllocatePoolWithTag(PagedPool, serviceName.Length, TAG_IO);
        if (!buf)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlMoveMemory(buf, serviceName.Buffer, serviceName.Length);
        ServiceName->MaximumLength = serviceName.Length;
        ServiceName->Length = serviceName.Length;
        ServiceName->Buffer = buf;
    }

    *DriverName = driverName;
    status = STATUS_SUCCESS;

Cleanup:
    if (basicInfo)
        ExFreePoolWithTag(basicInfo, TAG_IO);

    if (!NT_SUCCESS(status) && driverName.Buffer)
        ExFreePoolWithTag(driverName.Buffer, TAG_IO);

    return status;
}

/*
 * RETURNS
 *  TRUE if String2 contains String1 as a suffix.
 */
BOOLEAN
NTAPI
IopSuffixUnicodeString(
    IN PCUNICODE_STRING String1,
    IN PCUNICODE_STRING String2)
{
    PWCHAR pc1;
    PWCHAR pc2;
    ULONG Length;

    if (String2->Length < String1->Length)
        return FALSE;

    Length = String1->Length / 2;
    pc1 = String1->Buffer;
    pc2 = &String2->Buffer[String2->Length / sizeof(WCHAR) - Length];

    if (pc1 && pc2)
    {
        while (Length--)
        {
            if( *pc1++ != *pc2++ )
                return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * IopDisplayLoadingMessage
 *
 * Display 'Loading XXX...' message.
 */
VOID
FASTCALL
IopDisplayLoadingMessage(PUNICODE_STRING ServiceName)
{
    CHAR TextBuffer[256];
    UNICODE_STRING DotSys = RTL_CONSTANT_STRING(L".SYS");

    if (ExpInTextModeSetup) return;
    if (!KeLoaderBlock) return;
    RtlUpcaseUnicodeString(ServiceName, ServiceName, FALSE);
    snprintf(TextBuffer, sizeof(TextBuffer),
            "%s%sSystem32\\Drivers\\%wZ%s\r\n",
            KeLoaderBlock->ArcBootDeviceName,
            KeLoaderBlock->NtBootPathName,
            ServiceName,
            IopSuffixUnicodeString(&DotSys, ServiceName) ? "" : ".SYS");
    HalDisplayString(TextBuffer);
}

/*
 * IopNormalizeImagePath
 *
 * Normalize an image path to contain complete path.
 *
 * Parameters
 *    ImagePath
 *       The input path and on exit the result path. ImagePath.Buffer
 *       must be allocated by ExAllocatePool on input. Caller is responsible
 *       for freeing the buffer when it's no longer needed.
 *
 *    ServiceName
 *       Name of the service that ImagePath belongs to.
 *
 * Return Value
 *    Status
 *
 * Remarks
 *    The input image path isn't freed on error.
 */
NTSTATUS
FASTCALL
IopNormalizeImagePath(
    _Inout_ _When_(return>=0, _At_(ImagePath->Buffer, _Post_notnull_ __drv_allocatesMem(Mem)))
         PUNICODE_STRING ImagePath,
    _In_ PUNICODE_STRING ServiceName)
{
    UNICODE_STRING SystemRootString = RTL_CONSTANT_STRING(L"\\SystemRoot\\");
    UNICODE_STRING DriversPathString = RTL_CONSTANT_STRING(L"\\SystemRoot\\System32\\drivers\\");
    UNICODE_STRING DotSysString = RTL_CONSTANT_STRING(L".sys");
    UNICODE_STRING InputImagePath;

    DPRINT("Normalizing image path '%wZ' for service '%wZ'\n", ImagePath, ServiceName);

    InputImagePath = *ImagePath;
    if (InputImagePath.Length == 0)
    {
        ImagePath->Length = 0;
        ImagePath->MaximumLength = DriversPathString.Length +
                                   ServiceName->Length +
                                   DotSysString.Length +
                                   sizeof(UNICODE_NULL);
        ImagePath->Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                  ImagePath->MaximumLength,
                                                  TAG_IO);
        if (ImagePath->Buffer == NULL)
            return STATUS_NO_MEMORY;

        RtlCopyUnicodeString(ImagePath, &DriversPathString);
        RtlAppendUnicodeStringToString(ImagePath, ServiceName);
        RtlAppendUnicodeStringToString(ImagePath, &DotSysString);
    }
    else if (InputImagePath.Buffer[0] != L'\\')
    {
        ImagePath->Length = 0;
        ImagePath->MaximumLength = SystemRootString.Length +
                                   InputImagePath.Length +
                                   sizeof(UNICODE_NULL);
        ImagePath->Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                  ImagePath->MaximumLength,
                                                  TAG_IO);
        if (ImagePath->Buffer == NULL)
            return STATUS_NO_MEMORY;

        RtlCopyUnicodeString(ImagePath, &SystemRootString);
        RtlAppendUnicodeStringToString(ImagePath, &InputImagePath);

        /* Free caller's string */
        ExFreePoolWithTag(InputImagePath.Buffer, TAG_RTLREGISTRY);
    }

    DPRINT("Normalized image path is '%wZ' for service '%wZ'\n", ImagePath, ServiceName);

    return STATUS_SUCCESS;
}

/**
 * @brief      Initialize a loaded driver
 *
 * @param[in]  ModuleObject
 *     Module object representing the driver. It can be retrieved by IopLoadServiceModule.
 *     Freed on failure, so in a such case this should not be accessed anymore
 *
 * @param[in]  ServiceHandle
 *     Handle to a driver's CCS/Services/<ServiceName> key
 *
 * @param[out] DriverObject
 *     This contains the driver object if it was created (even with unsuccessfull result)
 *
 * @param[out] DriverEntryStatus
 *     This contains the status value returned by the driver's DriverEntry routine
 *     (will not be valid of the return value is not STATUS_SUCCESS or STATUS_FAILED_DRIVER_ENTRY)
 *
 * @return     Status of the operation
 */
NTSTATUS
IopInitializeDriverModule(
    _In_ PLDR_DATA_TABLE_ENTRY ModuleObject,
    _In_ HANDLE ServiceHandle,
    _Out_ PDRIVER_OBJECT *OutDriverObject,
    _Out_ NTSTATUS *DriverEntryStatus)
{
    UNICODE_STRING DriverName, RegistryPath, ServiceName;
    NTSTATUS Status;

    PAGED_CODE();

    Status = IopGetDriverNames(ServiceHandle, &DriverName, &ServiceName);
    if (!NT_SUCCESS(Status))
    {
        MmUnloadSystemImage(ModuleObject);
        return Status;
    }

    DPRINT("Driver name: '%wZ'\n", &DriverName);

    /*
     * Retrieve the driver's PE image NT header and perform some sanity checks.
     * NOTE: We suppose that since the driver has been successfully loaded,
     * its NT and optional headers are all valid and have expected sizes.
     */
    PIMAGE_NT_HEADERS NtHeaders = RtlImageNtHeader(ModuleObject->DllBase);
    ASSERT(NtHeaders);
    // NOTE: ModuleObject->SizeOfImage is actually (number of PTEs)*PAGE_SIZE.
    ASSERT(ModuleObject->SizeOfImage == ROUND_TO_PAGES(NtHeaders->OptionalHeader.SizeOfImage));
    ASSERT(ModuleObject->EntryPoint == RVA(ModuleObject->DllBase, NtHeaders->OptionalHeader.AddressOfEntryPoint));

    /* Obtain the registry path for the DriverInit routine */
    PKEY_NAME_INFORMATION nameInfo;
    ULONG infoLength;
    Status = ZwQueryKey(ServiceHandle, KeyNameInformation, NULL, 0, &infoLength);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        nameInfo = ExAllocatePoolWithTag(NonPagedPool, infoLength, TAG_IO);
        if (nameInfo)
        {
            Status = ZwQueryKey(ServiceHandle,
                                KeyNameInformation,
                                nameInfo,
                                infoLength,
                                &infoLength);
            if (NT_SUCCESS(Status))
            {
                RegistryPath.Length = nameInfo->NameLength;
                RegistryPath.MaximumLength = nameInfo->NameLength;
                RegistryPath.Buffer = nameInfo->Name;
            }
            else
            {
                ExFreePoolWithTag(nameInfo, TAG_IO);
            }
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        Status = NT_SUCCESS(Status) ? STATUS_UNSUCCESSFUL : Status;
    }

    if (!NT_SUCCESS(Status))
    {
        RtlFreeUnicodeString(&ServiceName);
        RtlFreeUnicodeString(&DriverName);
        MmUnloadSystemImage(ModuleObject);
        return Status;
    }

    /* Create the driver object */
    ULONG ObjectSize = sizeof(DRIVER_OBJECT) + sizeof(EXTENDED_DRIVER_EXTENSION);
    OBJECT_ATTRIBUTES objAttrs;
    PDRIVER_OBJECT driverObject;
    InitializeObjectAttributes(&objAttrs,
                               &DriverName,
                               OBJ_PERMANENT | OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ObCreateObject(KernelMode,
                            IoDriverObjectType,
                            &objAttrs,
                            KernelMode,
                            NULL,
                            ObjectSize,
                            0,
                            0,
                            (PVOID*)&driverObject);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(nameInfo, TAG_IO); // container for RegistryPath
        RtlFreeUnicodeString(&ServiceName);
        RtlFreeUnicodeString(&DriverName);
        MmUnloadSystemImage(ModuleObject);
        DPRINT1("Error while creating driver object \"%wZ\" status %x\n", &DriverName, Status);
        return Status;
    }

    DPRINT("Created driver object 0x%p for \"%wZ\"\n", driverObject, &DriverName);

    RtlZeroMemory(driverObject, ObjectSize);
    driverObject->Type = IO_TYPE_DRIVER;
    driverObject->Size = sizeof(DRIVER_OBJECT);

    /* Set the legacy flag if this is not a WDM driver */
    if (!(NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_WDM_DRIVER))
        driverObject->Flags |= DRVO_LEGACY_DRIVER;

    driverObject->DriverSection = ModuleObject;
    driverObject->DriverStart = ModuleObject->DllBase;
    driverObject->DriverSize = ModuleObject->SizeOfImage;
    driverObject->DriverInit = ModuleObject->EntryPoint;
    driverObject->HardwareDatabase = &IopHardwareDatabaseKey;
    driverObject->DriverExtension = (PDRIVER_EXTENSION)(driverObject + 1);
    driverObject->DriverExtension->DriverObject = driverObject;

    /* Loop all Major Functions */
    for (INT i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        /* Invalidate each function */
        driverObject->MajorFunction[i] = IopInvalidDeviceRequest;
    }

    /* Add the Object and get its handle */
    HANDLE hDriver;
    Status = ObInsertObject(driverObject, NULL, FILE_READ_DATA, 0, NULL, &hDriver);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(nameInfo, TAG_IO);
        RtlFreeUnicodeString(&ServiceName);
        RtlFreeUnicodeString(&DriverName);
        return Status;
    }

    /* Now reference it */
    Status = ObReferenceObjectByHandle(hDriver,
                                       0,
                                       IoDriverObjectType,
                                       KernelMode,
                                       (PVOID*)&driverObject,
                                       NULL);

    /* Close the extra handle */
    ZwClose(hDriver);

    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(nameInfo, TAG_IO); // container for RegistryPath
        RtlFreeUnicodeString(&ServiceName);
        RtlFreeUnicodeString(&DriverName);
        return Status;
    }

    /* Set up the service key name buffer */
    UNICODE_STRING serviceKeyName;
    serviceKeyName.Length = 0;
    // NULL-terminate for Windows compatibility
    serviceKeyName.MaximumLength = ServiceName.MaximumLength + sizeof(UNICODE_NULL);
    serviceKeyName.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                  serviceKeyName.MaximumLength,
                                                  TAG_IO);
    if (!serviceKeyName.Buffer)
    {
        ObMakeTemporaryObject(driverObject);
        ObDereferenceObject(driverObject);
        ExFreePoolWithTag(nameInfo, TAG_IO); // container for RegistryPath
        RtlFreeUnicodeString(&ServiceName);
        RtlFreeUnicodeString(&DriverName);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy the name and set it in the driver extension */
    RtlCopyUnicodeString(&serviceKeyName, &ServiceName);
    RtlFreeUnicodeString(&ServiceName);
    driverObject->DriverExtension->ServiceKeyName = serviceKeyName;

    /* Make a copy of the driver name to store in the driver object */
    UNICODE_STRING driverNamePaged;
    driverNamePaged.Length = 0;
    // NULL-terminate for Windows compatibility
    driverNamePaged.MaximumLength = DriverName.MaximumLength + sizeof(UNICODE_NULL);
    driverNamePaged.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                   driverNamePaged.MaximumLength,
                                                   TAG_IO);
    if (!driverNamePaged.Buffer)
    {
        ObMakeTemporaryObject(driverObject);
        ObDereferenceObject(driverObject);
        ExFreePoolWithTag(nameInfo, TAG_IO); // container for RegistryPath
        RtlFreeUnicodeString(&DriverName);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyUnicodeString(&driverNamePaged, &DriverName);
    driverObject->DriverName = driverNamePaged;

    /* Finally, call its init function */
    Status = driverObject->DriverInit(driverObject, &RegistryPath);
    *DriverEntryStatus = Status;
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("'%wZ' initialization failed, status (0x%08lx)\n", &DriverName, Status);
        // return a special status value in case of failure
        Status = STATUS_FAILED_DRIVER_ENTRY;
    }

    /* HACK: We're going to say if we don't have any DOs from DriverEntry, then we're not legacy.
     * Other parts of the I/O manager depend on this behavior */
    if (!driverObject->DeviceObject)
    {
        driverObject->Flags &= ~DRVO_LEGACY_DRIVER;
    }

    // Windows does this fixup - keep it for compatibility
    for (INT i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        /*
         * Make sure the driver didn't set any dispatch entry point to NULL!
         * Doing so is illegal; drivers shouldn't touch entry points they
         * do not implement.
         */

        /* Check if it did so anyway */
        if (!driverObject->MajorFunction[i])
        {
            /* Print a warning in the debug log */
            DPRINT1("Driver <%wZ> set DriverObject->MajorFunction[%lu] to NULL!\n",
                    &driverObject->DriverName, i);

            /* Fix it up */
            driverObject->MajorFunction[i] = IopInvalidDeviceRequest;
        }
    }

    // TODO: for legacy drivers, unload the driver if it didn't create any DO

    ExFreePoolWithTag(nameInfo, TAG_IO); // container for RegistryPath
    RtlFreeUnicodeString(&DriverName);

    if (!NT_SUCCESS(Status))
    {
        // if the driver entry has been failed, clear the object
        ObMakeTemporaryObject(driverObject);
        ObDereferenceObject(driverObject);
        return Status;
    }

    *OutDriverObject = driverObject;

    MmFreeDriverInitialization((PLDR_DATA_TABLE_ENTRY)driverObject->DriverSection);

    /* Set the driver as initialized */
    IopReadyDeviceObjects(driverObject);

    if (PnpSystemInit) IopReinitializeDrivers();

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MiResolveImageReferences(IN PVOID ImageBase,
                         IN PUNICODE_STRING ImageFileDirectory,
                         IN PUNICODE_STRING NamePrefix OPTIONAL,
                         OUT PCHAR *MissingApi,
                         OUT PWCHAR *MissingDriver,
                         OUT PLOAD_IMPORTS *LoadImports);

//
// Used for images already loaded (boot drivers)
//
CODE_SEG("INIT")
NTSTATUS
NTAPI
LdrProcessDriverModule(PLDR_DATA_TABLE_ENTRY LdrEntry,
                       PUNICODE_STRING FileName,
                       PLDR_DATA_TABLE_ENTRY *ModuleObject)
{
    NTSTATUS Status;
    UNICODE_STRING BaseName, BaseDirectory;
    PLOAD_IMPORTS LoadedImports = (PVOID)-2;
    PCHAR MissingApiName, Buffer;
    PWCHAR MissingDriverName;
    PVOID DriverBase = LdrEntry->DllBase;

    /* Allocate a buffer we'll use for names */
    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   MAXIMUM_FILENAME_LENGTH,
                                   TAG_LDR_WSTR);
    if (!Buffer)
    {
        /* Fail */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Check for a separator */
    if (FileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
    {
        PWCHAR p;
        ULONG BaseLength;

        /* Loop the path until we get to the base name */
        p = &FileName->Buffer[FileName->Length / sizeof(WCHAR)];
        while (*(p - 1) != OBJ_NAME_PATH_SEPARATOR) p--;

        /* Get the length */
        BaseLength = (ULONG)(&FileName->Buffer[FileName->Length / sizeof(WCHAR)] - p);
        BaseLength *= sizeof(WCHAR);

        /* Setup the string */
        BaseName.Length = (USHORT)BaseLength;
        BaseName.Buffer = p;
    }
    else
    {
        /* Otherwise, we already have a base name */
        BaseName.Length = FileName->Length;
        BaseName.Buffer = FileName->Buffer;
    }

    /* Setup the maximum length */
    BaseName.MaximumLength = BaseName.Length;

    /* Now compute the base directory */
    BaseDirectory = *FileName;
    BaseDirectory.Length -= BaseName.Length;
    BaseDirectory.MaximumLength = BaseDirectory.Length;

    /* Resolve imports */
    MissingApiName = Buffer;
    Status = MiResolveImageReferences(DriverBase,
                                      &BaseDirectory,
                                      NULL,
                                      &MissingApiName,
                                      &MissingDriverName,
                                      &LoadedImports);

    /* Free the temporary buffer */
    ExFreePoolWithTag(Buffer, TAG_LDR_WSTR);

    /* Check the result of the imports resolution */
    if (!NT_SUCCESS(Status)) return Status;

    /* Return */
    *ModuleObject = LdrEntry;
    return STATUS_SUCCESS;
}

PDEVICE_OBJECT
IopGetDeviceObjectFromDeviceInstance(PUNICODE_STRING DeviceInstance);

/*
 * IopInitializeBuiltinDriver
 *
 * Initialize a driver that is already loaded in memory.
 */
CODE_SEG("INIT")
static
BOOLEAN
IopInitializeBuiltinDriver(IN PLDR_DATA_TABLE_ENTRY BootLdrEntry)
{
    PDRIVER_OBJECT DriverObject;
    NTSTATUS Status;
    PWCHAR Buffer, FileNameWithoutPath;
    PWSTR FileExtension;
    PUNICODE_STRING ModuleName = &BootLdrEntry->BaseDllName;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PLIST_ENTRY NextEntry;
    UNICODE_STRING ServiceName;
    BOOLEAN Success;

    /*
     * Display 'Loading XXX...' message
     */
    IopDisplayLoadingMessage(ModuleName);
    InbvIndicateProgress();

    Buffer = ExAllocatePoolWithTag(PagedPool,
                                   ModuleName->Length + sizeof(UNICODE_NULL),
                                   TAG_IO);
    if (Buffer == NULL)
    {
        return FALSE;
    }

    RtlCopyMemory(Buffer, ModuleName->Buffer, ModuleName->Length);
    Buffer[ModuleName->Length / sizeof(WCHAR)] = UNICODE_NULL;

    /*
     * Generate filename without path (not needed by freeldr)
     */
    FileNameWithoutPath = wcsrchr(Buffer, L'\\');
    if (FileNameWithoutPath == NULL)
    {
        FileNameWithoutPath = Buffer;
    }
    else
    {
        FileNameWithoutPath++;
    }

    /*
     * Strip the file extension from ServiceName
     */
    Success = RtlCreateUnicodeString(&ServiceName, FileNameWithoutPath);
    ExFreePoolWithTag(Buffer, TAG_IO);
    if (!Success)
    {
        return FALSE;
    }

    FileExtension = wcsrchr(ServiceName.Buffer, L'.');
    if (FileExtension != NULL)
    {
        ServiceName.Length -= (USHORT)wcslen(FileExtension) * sizeof(WCHAR);
        FileExtension[0] = UNICODE_NULL;
    }

    UNICODE_STRING RegistryPath;

    // Make the registry path for the driver
    RegistryPath.Length = 0;
    RegistryPath.MaximumLength = sizeof(ServicesKeyName) + ServiceName.Length;
    RegistryPath.Buffer = ExAllocatePoolWithTag(PagedPool, RegistryPath.MaximumLength, TAG_IO);
    if (RegistryPath.Buffer == NULL)
    {
        return FALSE;
    }
    RtlAppendUnicodeToString(&RegistryPath, ServicesKeyName);
    RtlAppendUnicodeStringToString(&RegistryPath, &ServiceName);
    RtlFreeUnicodeString(&ServiceName);

    HANDLE serviceHandle;
    Status = IopOpenRegistryKeyEx(&serviceHandle, NULL, &RegistryPath, KEY_READ);
    RtlFreeUnicodeString(&RegistryPath);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    /* Lookup the new Ldr entry in PsLoadedModuleList */
    for (NextEntry = PsLoadedModuleList.Flink;
         NextEntry != &PsLoadedModuleList;
         NextEntry = NextEntry->Flink)
    {
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);
        if (RtlEqualUnicodeString(ModuleName, &LdrEntry->BaseDllName, TRUE))
        {
            break;
        }
    }
    ASSERT(NextEntry != &PsLoadedModuleList);

    /*
     * Initialize the driver
     */
    NTSTATUS driverEntryStatus;
    Status = IopInitializeDriverModule(LdrEntry,
                                       serviceHandle,
                                       &DriverObject,
                                       &driverEntryStatus);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Driver '%wZ' load failed, status (%x)\n", ModuleName, Status);
        return FALSE;
    }

    // The driver has been loaded, now check if where are any PDOs
    // for that driver, and queue AddDevice call for them.
    // The check is possible because HKLM/SYSTEM/CCS/Services/<ServiceName>/Enum directory
    // is populated upon a new device arrival based on a (critical) device database

    // Legacy drivers may add devices inside DriverEntry.
    // We're lazy and always assume that they are doing so
    BOOLEAN deviceAdded = !!(DriverObject->Flags & DRVO_LEGACY_DRIVER);

    HANDLE enumServiceHandle;
    UNICODE_STRING enumName = RTL_CONSTANT_STRING(L"Enum");

    Status = IopOpenRegistryKeyEx(&enumServiceHandle, serviceHandle, &enumName, KEY_READ);
    ZwClose(serviceHandle);

    if (NT_SUCCESS(Status))
    {
        ULONG instanceCount = 0;
        PKEY_VALUE_FULL_INFORMATION kvInfo;
        Status = IopGetRegistryValue(enumServiceHandle, L"Count", &kvInfo);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        if (kvInfo->Type != REG_DWORD || kvInfo->DataLength != sizeof(ULONG))
        {
            ExFreePool(kvInfo);
            goto Cleanup;
        }

        RtlMoveMemory(&instanceCount,
                      (PVOID)((ULONG_PTR)kvInfo + kvInfo->DataOffset),
                      sizeof(ULONG));
        ExFreePool(kvInfo);

        DPRINT("Processing %u instances for %wZ module\n", instanceCount, ModuleName);

        for (ULONG i = 0; i < instanceCount; i++)
        {
            WCHAR num[11];
            UNICODE_STRING instancePath;
            RtlStringCchPrintfW(num, sizeof(num), L"%u", i);

            Status = IopGetRegistryValue(enumServiceHandle, num, &kvInfo);
            if (!NT_SUCCESS(Status))
            {
                continue;
            }
            if (kvInfo->Type != REG_SZ || kvInfo->DataLength == 0)
            {
                ExFreePool(kvInfo);
                continue;
            }

            instancePath.Length = kvInfo->DataLength - sizeof(UNICODE_NULL);
            instancePath.MaximumLength = kvInfo->DataLength;
            instancePath.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                        instancePath.MaximumLength,
                                                        TAG_IO);
            if (instancePath.Buffer)
            {
                RtlMoveMemory(instancePath.Buffer,
                              (PVOID)((ULONG_PTR)kvInfo + kvInfo->DataOffset),
                              instancePath.Length);
                instancePath.Buffer[instancePath.Length / sizeof(WCHAR)] = UNICODE_NULL;

                PDEVICE_OBJECT pdo = IopGetDeviceObjectFromDeviceInstance(&instancePath);
                PiQueueDeviceAction(pdo, PiActionAddBootDevices, NULL, NULL);
                ObDereferenceObject(pdo);
                deviceAdded = TRUE;
            }

            ExFreePool(kvInfo);
        }

        ZwClose(enumServiceHandle);
    }
Cleanup:
    /* Remove extra reference from IopInitializeDriverModule */
    ObDereferenceObject(DriverObject);

    return deviceAdded;
}

/*
 * IopInitializeBootDrivers
 *
 * Initialize boot drivers and free memory for boot files.
 *
 * Parameters
 *    None
 *
 * Return Value
 *    None
 */
CODE_SEG("INIT")
VOID
FASTCALL
IopInitializeBootDrivers(VOID)
{
    PLIST_ENTRY ListHead, NextEntry, NextEntry2;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    NTSTATUS Status;
    UNICODE_STRING DriverName;
    ULONG i, Index;
    PDRIVER_INFORMATION DriverInfo, DriverInfoTag;
    HANDLE KeyHandle;
    PBOOT_DRIVER_LIST_ENTRY BootEntry;
    DPRINT("IopInitializeBootDrivers()\n");

    /* Create the RAW FS built-in driver */
    RtlInitUnicodeString(&DriverName, L"\\FileSystem\\RAW");

    Status = IoCreateDriver(&DriverName, RawFsDriverEntry);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        return;
    }

    /* Get highest group order index */
    IopGroupIndex = PpInitGetGroupOrderIndex(NULL);
    if (IopGroupIndex == 0xFFFF) ASSERT(FALSE);

    /* Allocate the group table */
    IopGroupTable = ExAllocatePoolWithTag(PagedPool,
                                          IopGroupIndex * sizeof(LIST_ENTRY),
                                          TAG_IO);
    if (IopGroupTable == NULL) ASSERT(FALSE);

    /* Initialize the group table lists */
    for (i = 0; i < IopGroupIndex; i++) InitializeListHead(&IopGroupTable[i]);

    /* Loop the boot modules */
    ListHead = &KeLoaderBlock->LoadOrderListHead;
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        /* Check if the DLL needs to be initialized */
        if (LdrEntry->Flags & LDRP_DRIVER_DEPENDENT_DLL)
        {
            /* Call its entrypoint */
            MmCallDllInitialize(LdrEntry, NULL);
        }
    }

    /* Loop the boot drivers */
    ListHead = &KeLoaderBlock->BootDriverListHead;
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Get the entry */
        BootEntry = CONTAINING_RECORD(NextEntry,
                                      BOOT_DRIVER_LIST_ENTRY,
                                      Link);

        // FIXME: TODO: This LdrEntry is to be used in a special handling
        // for SETUPLDR (a similar procedure is done on Windows), where
        // the loader would, under certain conditions, be loaded in the
        // SETUPLDR-specific code block below...
#if 0
        /* Get the driver loader entry */
        LdrEntry = BootEntry->LdrEntry;
#endif

        /* Allocate our internal accounting structure */
        DriverInfo = ExAllocatePoolWithTag(PagedPool,
                                           sizeof(DRIVER_INFORMATION),
                                           TAG_IO);
        if (DriverInfo)
        {
            /* Zero it and initialize it */
            RtlZeroMemory(DriverInfo, sizeof(DRIVER_INFORMATION));
            InitializeListHead(&DriverInfo->Link);
            DriverInfo->DataTableEntry = BootEntry;

            /* Open the registry key */
            Status = IopOpenRegistryKeyEx(&KeyHandle,
                                          NULL,
                                          &BootEntry->RegistryPath,
                                          KEY_READ);
            DPRINT("IopOpenRegistryKeyEx(%wZ) returned 0x%08lx\n", &BootEntry->RegistryPath, Status);
#if 0
            if (NT_SUCCESS(Status))
#else // Hack still needed...
            if ((NT_SUCCESS(Status)) || /* ReactOS HACK for SETUPLDR */
                ((KeLoaderBlock->SetupLdrBlock) && ((KeyHandle = (PVOID)1)))) // yes, it's an assignment!
#endif
            {
                /* Save the handle */
                DriverInfo->ServiceHandle = KeyHandle;

                /* Get the group oder index */
                Index = PpInitGetGroupOrderIndex(KeyHandle);

                /* Get the tag position */
                DriverInfo->TagPosition = PipGetDriverTagPriority(KeyHandle);

                /* Insert it into the list, at the right place */
                ASSERT(Index < IopGroupIndex);
                NextEntry2 = IopGroupTable[Index].Flink;
                while (NextEntry2 != &IopGroupTable[Index])
                {
                    /* Get the driver info */
                    DriverInfoTag = CONTAINING_RECORD(NextEntry2,
                                                      DRIVER_INFORMATION,
                                                      Link);

                    /* Check if we found the right tag position */
                    if (DriverInfoTag->TagPosition > DriverInfo->TagPosition)
                    {
                        /* We're done */
                        break;
                    }

                    /* Next entry */
                    NextEntry2 = NextEntry2->Flink;
                }

                /* Insert us right before the next entry */
                NextEntry2 = NextEntry2->Blink;
                InsertHeadList(NextEntry2, &DriverInfo->Link);
            }
        }
    }

    /* Loop each group index */
    for (i = 0; i < IopGroupIndex; i++)
    {
        /* Loop each group table */
        for (NextEntry = IopGroupTable[i].Flink;
             NextEntry != &IopGroupTable[i];
             NextEntry = NextEntry->Flink)
        {
            /* Get the entry */
            DriverInfo = CONTAINING_RECORD(NextEntry,
                                           DRIVER_INFORMATION,
                                           Link);

            /* Get the driver loader entry */
            LdrEntry = DriverInfo->DataTableEntry->LdrEntry;

            /* Initialize it */
            if (IopInitializeBuiltinDriver(LdrEntry))
            {
                // it does not make sense to enumerate the tree if there are no new devices added
                PiQueueDeviceAction(IopRootDeviceNode->PhysicalDeviceObject,
                                    PiActionEnumRootDevices,
                                    NULL,
                                    NULL);
            }
        }
    }

    /* HAL Root Bus is being initialized before loading the boot drivers so this may cause issues
     * when some devices are not being initialized with their drivers. This flag is used to delay
     * all actions with devices (except PnP root device) until boot drivers are loaded.
     * See PiQueueDeviceAction function
     */
    PnPBootDriversLoaded = TRUE;

    DbgPrint("BOOT DRIVERS LOADED\n");

    PiQueueDeviceAction(IopRootDeviceNode->PhysicalDeviceObject,
                        PiActionEnumDeviceTree,
                        NULL,
                        NULL);
}

CODE_SEG("INIT")
VOID
FASTCALL
IopInitializeSystemDrivers(VOID)
{
    PUNICODE_STRING *DriverList, *SavedList;

    PiPerformSyncDeviceAction(IopRootDeviceNode->PhysicalDeviceObject, PiActionEnumDeviceTree);

    /* No system drivers on the boot cd */
    if (KeLoaderBlock->SetupLdrBlock) return; // ExpInTextModeSetup

    /* Get the driver list */
    SavedList = DriverList = CmGetSystemDriverList();
    ASSERT(DriverList);

    /* Loop it */
    while (*DriverList)
    {
        /* Load the driver */
        ZwLoadDriver(*DriverList);

        /* Free the entry */
        RtlFreeUnicodeString(*DriverList);
        ExFreePool(*DriverList);

        /* Next entry */
        InbvIndicateProgress();
        DriverList++;
    }

    /* Free the list */
    ExFreePool(SavedList);

    PiQueueDeviceAction(IopRootDeviceNode->PhysicalDeviceObject,
                        PiActionEnumDeviceTree,
                        NULL,
                        NULL);
}

/*
 * IopUnloadDriver
 *
 * Unloads a device driver.
 *
 * Parameters
 *    DriverServiceName
 *       Name of the service to unload (registry key).
 *
 *    UnloadPnpDrivers
 *       Whether to unload Plug & Plug or only legacy drivers. If this
 *       parameter is set to FALSE, the routine will unload only legacy
 *       drivers.
 *
 * Return Value
 *    Status
 *
 * To do
 *    Guard the whole function by SEH.
 */

NTSTATUS NTAPI
IopUnloadDriver(PUNICODE_STRING DriverServiceName, BOOLEAN UnloadPnpDrivers)
{
    UNICODE_STRING Backslash = RTL_CONSTANT_STRING(L"\\");
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    UNICODE_STRING ImagePath;
    UNICODE_STRING ServiceName;
    UNICODE_STRING ObjectName;
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT DeviceObject;
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;
    NTSTATUS Status;
    USHORT LastBackslash;
    BOOLEAN SafeToUnload = TRUE;
    KPROCESSOR_MODE PreviousMode;
    UNICODE_STRING CapturedServiceName;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    /* Need the appropriate priviliege */
    if (!SeSinglePrivilegeCheck(SeLoadDriverPrivilege, PreviousMode))
    {
        DPRINT1("No unload privilege!\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Capture the service name */
    Status = ProbeAndCaptureUnicodeString(&CapturedServiceName,
                                          PreviousMode,
                                          DriverServiceName);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    DPRINT("IopUnloadDriver('%wZ', %u)\n", &CapturedServiceName, UnloadPnpDrivers);

    /* We need a service name */
    if (CapturedServiceName.Length == 0 || CapturedServiceName.Buffer == NULL)
    {
        ReleaseCapturedUnicodeString(&CapturedServiceName, PreviousMode);
        return STATUS_INVALID_PARAMETER;
    }

    /*
     * Get the service name from the registry key name
     */
    Status = RtlFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
                                        &CapturedServiceName,
                                        &Backslash,
                                        &LastBackslash);
    if (NT_SUCCESS(Status))
    {
        NT_ASSERT(CapturedServiceName.Length >= LastBackslash + sizeof(WCHAR));
        ServiceName.Buffer = &CapturedServiceName.Buffer[LastBackslash / sizeof(WCHAR) + 1];
        ServiceName.Length = CapturedServiceName.Length - LastBackslash - sizeof(WCHAR);
        ServiceName.MaximumLength = CapturedServiceName.MaximumLength - LastBackslash - sizeof(WCHAR);
    }
    else
    {
        ServiceName = CapturedServiceName;
    }

    /*
     * Construct the driver object name
     */
    Status = RtlUShortAdd(sizeof(DRIVER_ROOT_NAME),
                          ServiceName.Length,
                          &ObjectName.MaximumLength);
    if (!NT_SUCCESS(Status))
    {
        ReleaseCapturedUnicodeString(&CapturedServiceName, PreviousMode);
        return Status;
    }
    ObjectName.Length = 0;
    ObjectName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                              ObjectName.MaximumLength,
                                              TAG_IO);
    if (!ObjectName.Buffer)
    {
        ReleaseCapturedUnicodeString(&CapturedServiceName, PreviousMode);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    NT_VERIFY(NT_SUCCESS(RtlAppendUnicodeToString(&ObjectName, DRIVER_ROOT_NAME)));
    NT_VERIFY(NT_SUCCESS(RtlAppendUnicodeStringToString(&ObjectName, &ServiceName)));

    /*
     * Find the driver object
     */
    Status = ObReferenceObjectByName(&ObjectName,
                                     0,
                                     0,
                                     0,
                                     IoDriverObjectType,
                                     KernelMode,
                                     0,
                                     (PVOID*)&DriverObject);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Can't locate driver object for %wZ\n", &ObjectName);
        ExFreePoolWithTag(ObjectName.Buffer, TAG_IO);
        ReleaseCapturedUnicodeString(&CapturedServiceName, PreviousMode);
        return Status;
    }

    /* Free the buffer for driver object name */
    ExFreePoolWithTag(ObjectName.Buffer, TAG_IO);

    /* Check that driver is not already unloading */
    if (DriverObject->Flags & DRVO_UNLOAD_INVOKED)
    {
        DPRINT1("Driver deletion pending\n");
        ObDereferenceObject(DriverObject);
        ReleaseCapturedUnicodeString(&CapturedServiceName, PreviousMode);
        return STATUS_DELETE_PENDING;
    }

    /*
     * Get path of service...
     */
    RtlZeroMemory(QueryTable, sizeof(QueryTable));

    RtlInitUnicodeString(&ImagePath, NULL);

    QueryTable[0].Name = L"ImagePath";
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].EntryContext = &ImagePath;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    CapturedServiceName.Buffer,
                                    QueryTable,
                                    NULL,
                                    NULL);

    /* We no longer need service name */
    ReleaseCapturedUnicodeString(&CapturedServiceName, PreviousMode);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlQueryRegistryValues() failed (Status %x)\n", Status);
        ObDereferenceObject(DriverObject);
        return Status;
    }

    /*
     * Normalize the image path for all later processing.
     */
    Status = IopNormalizeImagePath(&ImagePath, &ServiceName);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopNormalizeImagePath() failed (Status %x)\n", Status);
        ObDereferenceObject(DriverObject);
        return Status;
    }

    /* Free the service path */
    ExFreePool(ImagePath.Buffer);

    /*
     * Unload the module and release the references to the device object
     */

    /* Call the load/unload routine, depending on current process */
    if (DriverObject->DriverUnload && DriverObject->DriverSection &&
        (UnloadPnpDrivers || (DriverObject->Flags & DRVO_LEGACY_DRIVER)))
    {
        /* Loop through each device object of the driver
           and set DOE_UNLOAD_PENDING flag */
        DeviceObject = DriverObject->DeviceObject;
        while (DeviceObject)
        {
            /* Set the unload pending flag for the device */
            DeviceExtension = IoGetDevObjExtension(DeviceObject);
            DeviceExtension->ExtensionFlags |= DOE_UNLOAD_PENDING;

            /* Make sure there are no attached devices or no reference counts */
            if ((DeviceObject->ReferenceCount) || (DeviceObject->AttachedDevice))
            {
                /* Not safe to unload */
                DPRINT1("Drivers device object is referenced or has attached devices\n");

                SafeToUnload = FALSE;
            }

            DeviceObject = DeviceObject->NextDevice;
        }

        /* If not safe to unload, then return success */
        if (!SafeToUnload)
        {
            ObDereferenceObject(DriverObject);
            return STATUS_SUCCESS;
        }

        DPRINT1("Unloading driver '%wZ' (manual)\n", &DriverObject->DriverName);

        /* Set the unload invoked flag and call the unload routine */
        DriverObject->Flags |= DRVO_UNLOAD_INVOKED;
        Status = IopDoLoadUnloadDriver(NULL, &DriverObject);
        ASSERT(Status == STATUS_SUCCESS);

        /* Mark the driver object temporary, so it could be deleted later */
        ObMakeTemporaryObject(DriverObject);

        /* Dereference it 2 times */
        ObDereferenceObject(DriverObject);
        ObDereferenceObject(DriverObject);

        return Status;
    }
    else
    {
        DPRINT1("No DriverUnload function! '%wZ' will not be unloaded!\n", &DriverObject->DriverName);

        /* Dereference one time (refd inside this function) */
        ObDereferenceObject(DriverObject);

        /* Return unloading failure */
        return STATUS_INVALID_DEVICE_REQUEST;
    }
}

VOID
NTAPI
IopReinitializeDrivers(VOID)
{
    PDRIVER_REINIT_ITEM ReinitItem;
    PLIST_ENTRY Entry;

    /* Get the first entry and start looping */
    Entry = ExInterlockedRemoveHeadList(&DriverReinitListHead,
                                        &DriverReinitListLock);
    while (Entry)
    {
        /* Get the item */
        ReinitItem = CONTAINING_RECORD(Entry, DRIVER_REINIT_ITEM, ItemEntry);

        /* Increment reinitialization counter */
        ReinitItem->DriverObject->DriverExtension->Count++;

        /* Remove the device object flag */
        ReinitItem->DriverObject->Flags &= ~DRVO_REINIT_REGISTERED;

        /* Call the routine */
        ReinitItem->ReinitRoutine(ReinitItem->DriverObject,
                                  ReinitItem->Context,
                                  ReinitItem->DriverObject->
                                  DriverExtension->Count);

        /* Free the entry */
        ExFreePool(Entry);

        /* Move to the next one */
        Entry = ExInterlockedRemoveHeadList(&DriverReinitListHead,
                                            &DriverReinitListLock);
    }
}

VOID
NTAPI
IopReinitializeBootDrivers(VOID)
{
    PDRIVER_REINIT_ITEM ReinitItem;
    PLIST_ENTRY Entry;

    /* Get the first entry and start looping */
    Entry = ExInterlockedRemoveHeadList(&DriverBootReinitListHead,
                                        &DriverBootReinitListLock);
    while (Entry)
    {
        /* Get the item */
        ReinitItem = CONTAINING_RECORD(Entry, DRIVER_REINIT_ITEM, ItemEntry);

        /* Increment reinitialization counter */
        ReinitItem->DriverObject->DriverExtension->Count++;

        /* Remove the device object flag */
        ReinitItem->DriverObject->Flags &= ~DRVO_BOOTREINIT_REGISTERED;

        /* Call the routine */
        ReinitItem->ReinitRoutine(ReinitItem->DriverObject,
                                  ReinitItem->Context,
                                  ReinitItem->DriverObject->
                                  DriverExtension->Count);

        /* Free the entry */
        ExFreePool(Entry);

        /* Move to the next one */
        Entry = ExInterlockedRemoveHeadList(&DriverBootReinitListHead,
                                            &DriverBootReinitListLock);
    }

    /* Wait for all device actions being finished*/
    KeWaitForSingleObject(&PiEnumerationFinished, Executive, KernelMode, FALSE, NULL);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoCreateDriver(
    _In_opt_ PUNICODE_STRING DriverName,
    _In_ PDRIVER_INITIALIZE InitializationFunction)
{
    WCHAR NameBuffer[100];
    USHORT NameLength;
    UNICODE_STRING LocalDriverName;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG ObjectSize;
    PDRIVER_OBJECT DriverObject;
    UNICODE_STRING ServiceKeyName;
    HANDLE hDriver;
    ULONG i, RetryCount = 0;

try_again:
    /* First, create a unique name for the driver if we don't have one */
    if (!DriverName)
    {
        /* Create a random name and set up the string */
        NameLength = (USHORT)swprintf(NameBuffer,
                                      DRIVER_ROOT_NAME L"%08u",
                                      KeTickCount.LowPart);
        LocalDriverName.Length = NameLength * sizeof(WCHAR);
        LocalDriverName.MaximumLength = LocalDriverName.Length + sizeof(UNICODE_NULL);
        LocalDriverName.Buffer = NameBuffer;
    }
    else
    {
        /* So we can avoid another code path, use a local var */
        LocalDriverName = *DriverName;
    }

    /* Initialize the Attributes */
    ObjectSize = sizeof(DRIVER_OBJECT) + sizeof(EXTENDED_DRIVER_EXTENSION);
    InitializeObjectAttributes(&ObjectAttributes,
                               &LocalDriverName,
                               OBJ_PERMANENT | OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Create the Object */
    Status = ObCreateObject(KernelMode,
                            IoDriverObjectType,
                            &ObjectAttributes,
                            KernelMode,
                            NULL,
                            ObjectSize,
                            0,
                            0,
                            (PVOID*)&DriverObject);
    if (!NT_SUCCESS(Status)) return Status;

    DPRINT("IopCreateDriver(): created DO %p\n", DriverObject);

    /* Set up the Object */
    RtlZeroMemory(DriverObject, ObjectSize);
    DriverObject->Type = IO_TYPE_DRIVER;
    DriverObject->Size = sizeof(DRIVER_OBJECT);
    DriverObject->Flags = DRVO_BUILTIN_DRIVER;
    DriverObject->DriverExtension = (PDRIVER_EXTENSION)(DriverObject + 1);
    DriverObject->DriverExtension->DriverObject = DriverObject;
    DriverObject->DriverInit = InitializationFunction;
    /* Loop all Major Functions */
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        /* Invalidate each function */
        DriverObject->MajorFunction[i] = IopInvalidDeviceRequest;
    }

    /* Set up the service key name buffer */
    ServiceKeyName.MaximumLength = LocalDriverName.Length + sizeof(UNICODE_NULL);
    ServiceKeyName.Buffer = ExAllocatePoolWithTag(PagedPool, LocalDriverName.MaximumLength, TAG_IO);
    if (!ServiceKeyName.Buffer)
    {
        /* Fail */
        ObMakeTemporaryObject(DriverObject);
        ObDereferenceObject(DriverObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* For builtin drivers, the ServiceKeyName is equal to DriverName */
    RtlCopyUnicodeString(&ServiceKeyName, &LocalDriverName);
    ServiceKeyName.Buffer[ServiceKeyName.Length / sizeof(WCHAR)] = UNICODE_NULL;
    DriverObject->DriverExtension->ServiceKeyName = ServiceKeyName;

    /* Make a copy of the driver name to store in the driver object */
    DriverObject->DriverName.MaximumLength = LocalDriverName.Length;
    DriverObject->DriverName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                            DriverObject->DriverName.MaximumLength,
                                                            TAG_IO);
    if (!DriverObject->DriverName.Buffer)
    {
        /* Fail */
        ObMakeTemporaryObject(DriverObject);
        ObDereferenceObject(DriverObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyUnicodeString(&DriverObject->DriverName, &LocalDriverName);

    /* Add the Object and get its handle */
    Status = ObInsertObject(DriverObject,
                            NULL,
                            FILE_READ_DATA,
                            0,
                            NULL,
                            &hDriver);

    /* Eliminate small possibility when this function is called more than
       once in a row, and KeTickCount doesn't get enough time to change */
    if (!DriverName && (Status == STATUS_OBJECT_NAME_COLLISION) && (RetryCount < 100))
    {
        RetryCount++;
        goto try_again;
    }

    if (!NT_SUCCESS(Status)) return Status;

    /* Now reference it */
    Status = ObReferenceObjectByHandle(hDriver,
                                       0,
                                       IoDriverObjectType,
                                       KernelMode,
                                       (PVOID*)&DriverObject,
                                       NULL);

    /* Close the extra handle */
    ZwClose(hDriver);

    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ObMakeTemporaryObject(DriverObject);
        ObDereferenceObject(DriverObject);
        return Status;
    }

    /* Finally, call its init function */
    DPRINT("Calling driver entrypoint at %p\n", InitializationFunction);
    Status = InitializationFunction(DriverObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        /* If it didn't work, then kill the object */
        DPRINT1("'%wZ' initialization failed, status (0x%08lx)\n", DriverName, Status);
        ObMakeTemporaryObject(DriverObject);
        ObDereferenceObject(DriverObject);
        return Status;
    }

    // Windows does this fixup - keep it for compatibility
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        /*
         * Make sure the driver didn't set any dispatch entry point to NULL!
         * Doing so is illegal; drivers shouldn't touch entry points they
         * do not implement.
         */

        /* Check if it did so anyway */
        if (!DriverObject->MajorFunction[i])
        {
            /* Print a warning in the debug log */
            DPRINT1("Driver <%wZ> set DriverObject->MajorFunction[%lu] to NULL!\n",
                    &DriverObject->DriverName, i);

            /* Fix it up */
            DriverObject->MajorFunction[i] = IopInvalidDeviceRequest;
        }
    }

    /* Return the Status */
    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
IoDeleteDriver(IN PDRIVER_OBJECT DriverObject)
{
    /* Simply dereference the Object */
    ObDereferenceObject(DriverObject);
}

/*
 * @implemented
 */
VOID
NTAPI
IoRegisterBootDriverReinitialization(IN PDRIVER_OBJECT DriverObject,
                                     IN PDRIVER_REINITIALIZE ReinitRoutine,
                                     IN PVOID Context)
{
    PDRIVER_REINIT_ITEM ReinitItem;

    /* Allocate the entry */
    ReinitItem = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(DRIVER_REINIT_ITEM),
                                       TAG_REINIT);
    if (!ReinitItem) return;

    /* Fill it out */
    ReinitItem->DriverObject = DriverObject;
    ReinitItem->ReinitRoutine = ReinitRoutine;
    ReinitItem->Context = Context;

    /* Set the Driver Object flag and insert the entry into the list */
    DriverObject->Flags |= DRVO_BOOTREINIT_REGISTERED;
    ExInterlockedInsertTailList(&DriverBootReinitListHead,
                                &ReinitItem->ItemEntry,
                                &DriverBootReinitListLock);
}

/*
 * @implemented
 */
VOID
NTAPI
IoRegisterDriverReinitialization(IN PDRIVER_OBJECT DriverObject,
                                 IN PDRIVER_REINITIALIZE ReinitRoutine,
                                 IN PVOID Context)
{
    PDRIVER_REINIT_ITEM ReinitItem;

    /* Allocate the entry */
    ReinitItem = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(DRIVER_REINIT_ITEM),
                                       TAG_REINIT);
    if (!ReinitItem) return;

    /* Fill it out */
    ReinitItem->DriverObject = DriverObject;
    ReinitItem->ReinitRoutine = ReinitRoutine;
    ReinitItem->Context = Context;

    /* Set the Driver Object flag and insert the entry into the list */
    DriverObject->Flags |= DRVO_REINIT_REGISTERED;
    ExInterlockedInsertTailList(&DriverReinitListHead,
                                &ReinitItem->ItemEntry,
                                &DriverReinitListLock);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoAllocateDriverObjectExtension(IN PDRIVER_OBJECT DriverObject,
                                IN PVOID ClientIdentificationAddress,
                                IN ULONG DriverObjectExtensionSize,
                                OUT PVOID *DriverObjectExtension)
{
    KIRQL OldIrql;
    PIO_CLIENT_EXTENSION DriverExtensions, NewDriverExtension;
    BOOLEAN Inserted = FALSE;

    /* Assume failure */
    *DriverObjectExtension = NULL;

    /* Allocate the extension */
    NewDriverExtension = ExAllocatePoolWithTag(NonPagedPool,
                                               sizeof(IO_CLIENT_EXTENSION) +
                                               DriverObjectExtensionSize,
                                               TAG_DRIVER_EXTENSION);
    if (!NewDriverExtension) return STATUS_INSUFFICIENT_RESOURCES;

    /* Clear the extension for teh caller */
    RtlZeroMemory(NewDriverExtension,
                  sizeof(IO_CLIENT_EXTENSION) + DriverObjectExtensionSize);

    /* Acqure lock */
    OldIrql = KeRaiseIrqlToDpcLevel();

    /* Fill out the extension */
    NewDriverExtension->ClientIdentificationAddress = ClientIdentificationAddress;

    /* Loop the current extensions */
    DriverExtensions = IoGetDrvObjExtension(DriverObject)->
                       ClientDriverExtension;
    while (DriverExtensions)
    {
        /* Check if the identifier matches */
        if (DriverExtensions->ClientIdentificationAddress ==
            ClientIdentificationAddress)
        {
            /* We have a collision, break out */
            break;
        }

        /* Go to the next one */
        DriverExtensions = DriverExtensions->NextExtension;
    }

    /* Check if we didn't collide */
    if (!DriverExtensions)
    {
        /* Link this one in */
        NewDriverExtension->NextExtension =
            IoGetDrvObjExtension(DriverObject)->ClientDriverExtension;
        IoGetDrvObjExtension(DriverObject)->ClientDriverExtension =
            NewDriverExtension;
        Inserted = TRUE;
    }

    /* Release the lock */
    KeLowerIrql(OldIrql);

    /* Check if insertion failed */
    if (!Inserted)
    {
        /* Free the entry and fail */
        ExFreePoolWithTag(NewDriverExtension, TAG_DRIVER_EXTENSION);
        return STATUS_OBJECT_NAME_COLLISION;
    }

    /* Otherwise, return the pointer */
    *DriverObjectExtension = NewDriverExtension + 1;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
PVOID
NTAPI
IoGetDriverObjectExtension(IN PDRIVER_OBJECT DriverObject,
                           IN PVOID ClientIdentificationAddress)
{
    KIRQL OldIrql;
    PIO_CLIENT_EXTENSION DriverExtensions;

    /* Acquire lock */
    OldIrql = KeRaiseIrqlToDpcLevel();

    /* Loop the list until we find the right one */
    DriverExtensions = IoGetDrvObjExtension(DriverObject)->ClientDriverExtension;
    while (DriverExtensions)
    {
        /* Check for a match */
        if (DriverExtensions->ClientIdentificationAddress ==
            ClientIdentificationAddress)
        {
            /* Break out */
            break;
        }

        /* Keep looping */
        DriverExtensions = DriverExtensions->NextExtension;
    }

    /* Release lock */
    KeLowerIrql(OldIrql);

    /* Return nothing or the extension */
    if (!DriverExtensions) return NULL;
    return DriverExtensions + 1;
}

NTSTATUS
IopLoadDriver(
    _In_ HANDLE ServiceHandle,
    _Out_ PDRIVER_OBJECT *DriverObject)
{
    UNICODE_STRING ImagePath;
    NTSTATUS Status;
    PLDR_DATA_TABLE_ENTRY ModuleObject;
    PVOID BaseAddress;

    PKEY_VALUE_FULL_INFORMATION kvInfo;
    Status = IopGetRegistryValue(ServiceHandle, L"ImagePath", &kvInfo);
    if (NT_SUCCESS(Status))
    {
        if (kvInfo->Type != REG_EXPAND_SZ || kvInfo->DataLength == 0)
        {
            ExFreePool(kvInfo);
            return STATUS_ILL_FORMED_SERVICE_ENTRY;
        }

        ImagePath.Length = kvInfo->DataLength - sizeof(UNICODE_NULL);
        ImagePath.MaximumLength = kvInfo->DataLength;
        ImagePath.Buffer = ExAllocatePoolWithTag(PagedPool, ImagePath.MaximumLength, TAG_RTLREGISTRY);
        if (!ImagePath.Buffer)
        {
            ExFreePool(kvInfo);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlMoveMemory(ImagePath.Buffer,
                      (PVOID)((ULONG_PTR)kvInfo + kvInfo->DataOffset),
                      ImagePath.Length);
        ImagePath.Buffer[ImagePath.Length / sizeof(WCHAR)] = UNICODE_NULL;
        ExFreePool(kvInfo);
    }
    else
    {
        return Status;
    }

    /*
     * Normalize the image path for all later processing.
     */
    Status = IopNormalizeImagePath(&ImagePath, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopNormalizeImagePath() failed (Status %x)\n", Status);
        return Status;
    }

    DPRINT("FullImagePath: '%wZ'\n", &ImagePath);

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&IopDriverLoadResource, TRUE);

    /*
     * Load the driver module
     */
    DPRINT("Loading module from %wZ\n", &ImagePath);
    Status = MmLoadSystemImage(&ImagePath, NULL, NULL, 0, (PVOID)&ModuleObject, &BaseAddress);
    RtlFreeUnicodeString(&ImagePath);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("MmLoadSystemImage() failed (Status %lx)\n", Status);
        ExReleaseResourceLite(&IopDriverLoadResource);
        KeLeaveCriticalRegion();
        return Status;
    }

    // Display the loading message
    ULONG infoLength;
    Status = ZwQueryKey(ServiceHandle, KeyBasicInformation, NULL, 0, &infoLength);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        PKEY_BASIC_INFORMATION servName = ExAllocatePoolWithTag(PagedPool, infoLength, TAG_IO);
        if (servName)
        {
            Status = ZwQueryKey(ServiceHandle,
                                KeyBasicInformation,
                                servName,
                                infoLength,
                                &infoLength);
            if (NT_SUCCESS(Status))
            {
                UNICODE_STRING serviceName = {
                    .Length = servName->NameLength,
                    .MaximumLength = servName->NameLength,
                    .Buffer = servName->Name
                };

                IopDisplayLoadingMessage(&serviceName);
            }
            ExFreePoolWithTag(servName, TAG_IO);
        }
    }

    NTSTATUS driverEntryStatus;
    Status = IopInitializeDriverModule(ModuleObject,
                                       ServiceHandle,
                                       DriverObject,
                                       &driverEntryStatus);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopInitializeDriverModule() failed (Status %lx)\n", Status);
    }

    ExReleaseResourceLite(&IopDriverLoadResource);
    KeLeaveCriticalRegion();

    return Status;
}

static
VOID
NTAPI
IopLoadUnloadDriverWorker(
    _Inout_ PVOID Parameter)
{
    PLOAD_UNLOAD_PARAMS LoadParams = Parameter;

    ASSERT(PsGetCurrentProcess() == PsInitialSystemProcess);

    if (LoadParams->DriverObject)
    {
        // unload request
        LoadParams->DriverObject->DriverUnload(LoadParams->DriverObject);
        LoadParams->Status = STATUS_SUCCESS;
    }
    else
    {
        // load request
        HANDLE serviceHandle;
        NTSTATUS status;
        status = IopOpenRegistryKeyEx(&serviceHandle, NULL, LoadParams->RegistryPath, KEY_READ);
        if (!NT_SUCCESS(status))
        {
            LoadParams->Status = status;
        }
        else
        {
            LoadParams->Status = IopLoadDriver(serviceHandle, &LoadParams->DriverObject);
            ZwClose(serviceHandle);
        }
    }

    if (LoadParams->SetEvent)
    {
        KeSetEvent(&LoadParams->Event, 0, FALSE);
    }
}

/**
 * @brief      Process load and unload driver operations. This is mostly for NtLoadDriver
 *             and NtUnloadDriver, because their code should run inside PsInitialSystemProcess
 *
 * @param[in]  RegistryPath  The registry path
 * @param      DriverObject  The driver object
 *
 * @return     Status of the operation
 */
NTSTATUS
IopDoLoadUnloadDriver(
    _In_opt_ PUNICODE_STRING RegistryPath,
    _Inout_ PDRIVER_OBJECT *DriverObject)
{
    LOAD_UNLOAD_PARAMS LoadParams;

    /* Prepare parameters block */
    LoadParams.RegistryPath = RegistryPath;
    LoadParams.DriverObject = *DriverObject;

    if (PsGetCurrentProcess() != PsInitialSystemProcess)
    {
        LoadParams.SetEvent = TRUE;
        KeInitializeEvent(&LoadParams.Event, NotificationEvent, FALSE);

        /* Initialize and queue a work item */
        ExInitializeWorkItem(&LoadParams.WorkItem, IopLoadUnloadDriverWorker, &LoadParams);
        ExQueueWorkItem(&LoadParams.WorkItem, DelayedWorkQueue);

        /* And wait till it completes */
        KeWaitForSingleObject(&LoadParams.Event, UserRequest, KernelMode, FALSE, NULL);
    }
    else
    {
        /* If we're already in a system process, call it right here */
        LoadParams.SetEvent = FALSE;
        IopLoadUnloadDriverWorker(&LoadParams);
    }

    return LoadParams.Status;
}

/*
 * NtLoadDriver
 *
 * Loads a device driver.
 *
 * Parameters
 *    DriverServiceName
 *       Name of the service to load (registry key).
 *
 * Return Value
 *    Status
 *
 * Status
 *    implemented
 */
NTSTATUS NTAPI
NtLoadDriver(IN PUNICODE_STRING DriverServiceName)
{
    UNICODE_STRING CapturedServiceName = { 0, 0, NULL };
    KPROCESSOR_MODE PreviousMode;
    PDRIVER_OBJECT DriverObject;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    /* Need the appropriate priviliege */
    if (!SeSinglePrivilegeCheck(SeLoadDriverPrivilege, PreviousMode))
    {
        DPRINT1("No load privilege!\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Capture the service name */
    Status = ProbeAndCaptureUnicodeString(&CapturedServiceName,
                                          PreviousMode,
                                          DriverServiceName);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    DPRINT("NtLoadDriver('%wZ')\n", &CapturedServiceName);

    /* We need a service name */
    if (CapturedServiceName.Length == 0 || CapturedServiceName.Buffer == NULL)
    {
        ReleaseCapturedUnicodeString(&CapturedServiceName, PreviousMode);
        return STATUS_INVALID_PARAMETER;
    }

    /* Load driver and call its entry point */
    DriverObject = NULL;
    Status = IopDoLoadUnloadDriver(&CapturedServiceName, &DriverObject);

    ReleaseCapturedUnicodeString(&CapturedServiceName, PreviousMode);
    return Status;
}

/*
 * NtUnloadDriver
 *
 * Unloads a legacy device driver.
 *
 * Parameters
 *    DriverServiceName
 *       Name of the service to unload (registry key).
 *
 * Return Value
 *    Status
 *
 * Status
 *    implemented
 */

NTSTATUS NTAPI
NtUnloadDriver(IN PUNICODE_STRING DriverServiceName)
{
    return IopUnloadDriver(DriverServiceName, FALSE);
}

/* EOF */
