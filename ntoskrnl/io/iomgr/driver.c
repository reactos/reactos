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

POBJECT_TYPE IoDriverObjectType = NULL;

#define TAG_RTLREGISTRY 'vrqR'

extern BOOLEAN ExpInTextModeSetup;
extern BOOLEAN PnpSystemInit;

USHORT IopGroupIndex;
PLIST_ENTRY IopGroupTable;

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
FASTCALL
IopGetDriverObject(
    PDRIVER_OBJECT *DriverObject,
    PUNICODE_STRING ServiceName,
    BOOLEAN FileSystem)
{
    PDRIVER_OBJECT Object;
    WCHAR NameBuffer[MAX_PATH];
    UNICODE_STRING DriverName;
    NTSTATUS Status;

    DPRINT("IopGetDriverObject(%p '%wZ' %x)\n",
           DriverObject, ServiceName, FileSystem);

    ASSERT(ExIsResourceAcquiredExclusiveLite(&IopDriverLoadResource));
    *DriverObject = NULL;

    /* Create ModuleName string */
    if (ServiceName == NULL || ServiceName->Buffer == NULL)
        /* We don't know which DriverObject we have to open */
        return STATUS_INVALID_PARAMETER_2;

    DriverName.Buffer = NameBuffer;
    DriverName.Length = 0;
    DriverName.MaximumLength = sizeof(NameBuffer);

    if (FileSystem != FALSE)
        RtlAppendUnicodeToString(&DriverName, FILESYSTEM_ROOT_NAME);
    else
        RtlAppendUnicodeToString(&DriverName, DRIVER_ROOT_NAME);
    RtlAppendUnicodeStringToString(&DriverName, ServiceName);

    DPRINT("Driver name: '%wZ'\n", &DriverName);

    /* Open driver object */
    Status = ObReferenceObjectByName(&DriverName,
                                     OBJ_OPENIF | OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, /* Attributes */
                                     NULL, /* PassedAccessState */
                                     0, /* DesiredAccess */
                                     IoDriverObjectType,
                                     KernelMode,
                                     NULL, /* ParseContext */
                                     (PVOID*)&Object);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to reference driver object, status=0x%08x\n", Status);
        return Status;
    }

    *DriverObject = Object;

    DPRINT("Driver Object: %p\n", Object);

    return STATUS_SUCCESS;
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

/*
 * IopLoadServiceModule
 *
 * Load a module specified by registry settings for service.
 *
 * Parameters
 *    ServiceName
 *       Name of the service to load.
 *
 * Return Value
 *    Status
 */
NTSTATUS
FASTCALL
IopLoadServiceModule(
    IN PUNICODE_STRING ServiceName,
    OUT PLDR_DATA_TABLE_ENTRY *ModuleObject)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    ULONG ServiceStart;
    UNICODE_STRING ServiceImagePath, CCSName;
    NTSTATUS Status;
    HANDLE CCSKey, ServiceKey;
    PVOID BaseAddress;

    ASSERT(ExIsResourceAcquiredExclusiveLite(&IopDriverLoadResource));
    ASSERT(ServiceName->Length);
    DPRINT("IopLoadServiceModule(%wZ, 0x%p)\n", ServiceName, ModuleObject);

    if (ExpInTextModeSetup)
    {
        /* We have no registry, but luckily we know where all the drivers are */
        DPRINT1("IopLoadServiceModule(%wZ, 0x%p) called in ExpInTextModeSetup mode...\n", ServiceName, ModuleObject);

        /* ServiceStart < 4 is all that matters */
        ServiceStart = 0;

        /* IopNormalizeImagePath will do all of the work for us if we give it an empty string */
        RtlInitEmptyUnicodeString(&ServiceImagePath, NULL, 0);
    }
    else
    {
        /* Open CurrentControlSet */
        RtlInitUnicodeString(&CCSName,
                             L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services");
        Status = IopOpenRegistryKeyEx(&CCSKey, NULL, &CCSName, KEY_READ);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopOpenRegistryKeyEx() failed for '%wZ' with status 0x%lx\n",
                    &CCSName, Status);
            return Status;
        }

        /* Open service key */
        Status = IopOpenRegistryKeyEx(&ServiceKey, CCSKey, ServiceName, KEY_READ);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopOpenRegistryKeyEx() failed for '%wZ' with status 0x%lx\n",
                    ServiceName, Status);
            ZwClose(CCSKey);
            return Status;
        }

        /*
         * Get information about the service.
         */
        RtlZeroMemory(QueryTable, sizeof(QueryTable));

        RtlInitUnicodeString(&ServiceImagePath, NULL);

        QueryTable[0].Name = L"Start";
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[0].EntryContext = &ServiceStart;

        QueryTable[1].Name = L"ImagePath";
        QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[1].EntryContext = &ServiceImagePath;

        Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        (PWSTR)ServiceKey,
                                        QueryTable,
                                        NULL,
                                        NULL);

        ZwClose(ServiceKey);
        ZwClose(CCSKey);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("RtlQueryRegistryValues() failed (Status %x)\n", Status);
            return Status;
        }
    }

    /*
     * Normalize the image path for all later processing.
     */
    Status = IopNormalizeImagePath(&ServiceImagePath, ServiceName);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopNormalizeImagePath() failed (Status %x)\n", Status);
        return Status;
    }

    /*
     * Case for disabled drivers
     */
    if (ServiceStart >= 4)
    {
        /* We can't load this */
        Status = STATUS_DRIVER_UNABLE_TO_LOAD;
    }
    else
    {
        DPRINT("Loading module from %wZ\n", &ServiceImagePath);
        Status = MmLoadSystemImage(&ServiceImagePath, NULL, NULL, 0, (PVOID)ModuleObject, &BaseAddress);
        if (NT_SUCCESS(Status))
        {
            IopDisplayLoadingMessage(ServiceName);
        }
    }

    ExFreePool(ServiceImagePath.Buffer);

    /*
     * Now check if the module was loaded successfully.
     */
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Module loading failed (Status %x)\n", Status);
    }

    DPRINT("Module loading (Status %x)\n", Status);

    return Status;
}

VOID
NTAPI
MmFreeDriverInitialization(IN PLDR_DATA_TABLE_ENTRY LdrEntry);

/*
 * IopInitializeDriverModule
 *
 * Initialize a loaded driver.
 *
 * Parameters
 *    DeviceNode
 *       Pointer to device node.
 *
 *    ModuleObject
 *       Module object representing the driver. It can be retrieve by
 *       IopLoadServiceModule.
 *
 *    ServiceName
 *       Name of the service (as in registry).
 *
 *    FileSystemDriver
 *       Set to TRUE for file system drivers.
 *
 *    DriverObject
 *       On successful return this contains the driver object representing
 *       the loaded driver.
 */
NTSTATUS
FASTCALL
IopInitializeDriverModule(
    IN PDEVICE_NODE DeviceNode,
    IN PLDR_DATA_TABLE_ENTRY ModuleObject,
    IN PUNICODE_STRING ServiceName,
    IN BOOLEAN FileSystemDriver,
    OUT PDRIVER_OBJECT *DriverObject)
{
    static const WCHAR ServicesKeyName[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\";
    UNICODE_STRING DriverName;
    UNICODE_STRING RegistryKey;
    PDRIVER_INITIALIZE DriverEntry;
    PDRIVER_OBJECT Driver;
    NTSTATUS Status;

    DriverEntry = ModuleObject->EntryPoint;

    if (ServiceName != NULL && ServiceName->Length != 0)
    {
        RegistryKey.Length = 0;
        RegistryKey.MaximumLength = sizeof(ServicesKeyName) + ServiceName->Length;
        RegistryKey.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                   RegistryKey.MaximumLength,
                                                   TAG_IO);
        if (RegistryKey.Buffer == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlAppendUnicodeToString(&RegistryKey, ServicesKeyName);
        RtlAppendUnicodeStringToString(&RegistryKey, ServiceName);
    }
    else
    {
        RtlInitEmptyUnicodeString(&RegistryKey, NULL, 0);
    }

    /* Create ModuleName string */
    if (ServiceName && ServiceName->Length > 0)
    {
        DriverName.Length = 0;
        DriverName.MaximumLength = sizeof(FILESYSTEM_ROOT_NAME) + ServiceName->Length;
        DriverName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                  DriverName.MaximumLength,
                                                  TAG_IO);
        if (DriverName.Buffer == NULL)
        {
            RtlFreeUnicodeString(&RegistryKey);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (FileSystemDriver != FALSE)
            RtlAppendUnicodeToString(&DriverName, FILESYSTEM_ROOT_NAME);
        else
            RtlAppendUnicodeToString(&DriverName, DRIVER_ROOT_NAME);
        RtlAppendUnicodeStringToString(&DriverName, ServiceName);

        DPRINT("Driver name: '%wZ'\n", &DriverName);
    }
    else
    {
        RtlInitEmptyUnicodeString(&DriverName, NULL, 0);
    }

    Status = IopCreateDriver(DriverName.Length > 0 ? &DriverName : NULL,
                             DriverEntry,
                             &RegistryKey,
                             ServiceName,
                             ModuleObject,
                             &Driver);
    RtlFreeUnicodeString(&RegistryKey);
    RtlFreeUnicodeString(&DriverName);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopCreateDriver() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    *DriverObject = Driver;

    MmFreeDriverInitialization((PLDR_DATA_TABLE_ENTRY)Driver->DriverSection);

    /* Set the driver as initialized */
    IopReadyDeviceObjects(Driver);

    if (PnpSystemInit) IopReinitializeDrivers();

    return STATUS_SUCCESS;
}

/*
 * IopAttachFilterDriversCallback
 *
 * Internal routine used by IopAttachFilterDrivers.
 */
NTSTATUS
NTAPI
IopAttachFilterDriversCallback(
    PWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength,
    PVOID Context,
    PVOID EntryContext)
{
    PDEVICE_NODE DeviceNode = Context;
    UNICODE_STRING ServiceName;
    PWCHAR Filters;
    PLDR_DATA_TABLE_ENTRY ModuleObject;
    PDRIVER_OBJECT DriverObject;
    NTSTATUS Status;

    /* No filter value present */
    if (ValueType == REG_NONE)
        return STATUS_SUCCESS;

    for (Filters = ValueData;
         ((ULONG_PTR)Filters - (ULONG_PTR)ValueData) < ValueLength &&
         *Filters != 0;
         Filters += (ServiceName.Length / sizeof(WCHAR)) + 1)
    {
        DPRINT("Filter Driver: %S (%wZ)\n", Filters, &DeviceNode->InstancePath);

        ServiceName.Buffer = Filters;
        ServiceName.MaximumLength =
        ServiceName.Length = (USHORT)wcslen(Filters) * sizeof(WCHAR);

        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite(&IopDriverLoadResource, TRUE);
        Status = IopGetDriverObject(&DriverObject,
                                    &ServiceName,
                                    FALSE);
        if (!NT_SUCCESS(Status))
        {
            /* Load and initialize the filter driver */
            Status = IopLoadServiceModule(&ServiceName, &ModuleObject);
            if (!NT_SUCCESS(Status))
            {
                ExReleaseResourceLite(&IopDriverLoadResource);
                KeLeaveCriticalRegion();
                return Status;
            }

            Status = IopInitializeDriverModule(DeviceNode,
                                               ModuleObject,
                                               &ServiceName,
                                               FALSE,
                                               &DriverObject);
            if (!NT_SUCCESS(Status))
            {
                ExReleaseResourceLite(&IopDriverLoadResource);
                KeLeaveCriticalRegion();
                return Status;
            }
        }

        ExReleaseResourceLite(&IopDriverLoadResource);
        KeLeaveCriticalRegion();

        Status = IopInitializeDevice(DeviceNode, DriverObject);

        /* Remove extra reference */
        ObDereferenceObject(DriverObject);

        if (!NT_SUCCESS(Status))
            return Status;
    }

    return STATUS_SUCCESS;
}

/*
 * IopAttachFilterDrivers
 *
 * Load filter drivers for specified device node.
 *
 * Parameters
 *    Lower
 *       Set to TRUE for loading lower level filters or FALSE for upper
 *       level filters.
 */
NTSTATUS
FASTCALL
IopAttachFilterDrivers(
    PDEVICE_NODE DeviceNode,
    HANDLE EnumSubKey,
    HANDLE ClassKey,
    BOOLEAN Lower)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2] = { { NULL, 0, NULL, NULL, 0, NULL, 0 }, };
    NTSTATUS Status;

    /*
     * First load the device filters
     */
    QueryTable[0].QueryRoutine = IopAttachFilterDriversCallback;
    if (Lower)
        QueryTable[0].Name = L"LowerFilters";
    else
        QueryTable[0].Name = L"UpperFilters";
    QueryTable[0].Flags = 0;
    QueryTable[0].DefaultType = REG_NONE;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    (PWSTR)EnumSubKey,
                                    QueryTable,
                                    DeviceNode,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to load device %s filters: %08X\n",
                Lower ? "lower" : "upper", Status);
        return Status;
    }

    QueryTable[0].QueryRoutine = IopAttachFilterDriversCallback;
    if (Lower)
        QueryTable[0].Name = L"LowerFilters";
    else
        QueryTable[0].Name = L"UpperFilters";
    QueryTable[0].EntryContext = NULL;
    QueryTable[0].Flags = 0;
    QueryTable[0].DefaultType = REG_NONE;

    if (ClassKey == NULL)
    {
        return STATUS_SUCCESS;
    }

    Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    (PWSTR)ClassKey,
                                    QueryTable,
                                    DeviceNode,
                                    NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to load class %s filters: %08X\n",
                Lower ? "lower" : "upper", Status);
        return Status;
    }

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
NTSTATUS
NTAPI
INIT_FUNCTION
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
    Buffer = ExAllocatePoolWithTag(NonPagedPool, MAX_PATH, TAG_LDR_WSTR);
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

/*
 * IopInitializeBuiltinDriver
 *
 * Initialize a driver that is already loaded in memory.
 */
NTSTATUS
NTAPI
INIT_FUNCTION
IopInitializeBuiltinDriver(IN PLDR_DATA_TABLE_ENTRY BootLdrEntry)
{
    PDEVICE_NODE DeviceNode;
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
        return STATUS_INSUFFICIENT_RESOURCES;
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
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    FileExtension = wcsrchr(ServiceName.Buffer, L'.');
    if (FileExtension != NULL)
    {
        ServiceName.Length -= (USHORT)wcslen(FileExtension) * sizeof(WCHAR);
        FileExtension[0] = UNICODE_NULL;
    }

    /*
     * Determine the right device object
     */
    /* Use IopRootDeviceNode for now */
    Status = IopCreateDeviceNode(IopRootDeviceNode,
                                 NULL,
                                 &ServiceName,
                                 &DeviceNode);
    RtlFreeUnicodeString(&ServiceName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Driver '%wZ' load failed, status (%x)\n", ModuleName, Status);
        return Status;
    }

    /* Lookup the new Ldr entry in PsLoadedModuleList */
    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList)
    {
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);
        if (RtlEqualUnicodeString(ModuleName, &LdrEntry->BaseDllName, TRUE))
        {
            break;
        }

        NextEntry = NextEntry->Flink;
    }
    ASSERT(NextEntry != &PsLoadedModuleList);

    /*
     * Initialize the driver
     */
    Status = IopInitializeDriverModule(DeviceNode,
                                       LdrEntry,
                                       &DeviceNode->ServiceName,
                                       FALSE,
                                       &DriverObject);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = IopInitializeDevice(DeviceNode, DriverObject);
    if (NT_SUCCESS(Status))
    {
        Status = IopStartDevice(DeviceNode);
    }

    /* Remove extra reference from IopInitializeDriverModule */
    ObDereferenceObject(DriverObject);

    return Status;
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
VOID
FASTCALL
INIT_FUNCTION
IopInitializeBootDrivers(VOID)
{
    PLIST_ENTRY ListHead, NextEntry, NextEntry2;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PDEVICE_NODE DeviceNode;
    PDRIVER_OBJECT DriverObject;
    LDR_DATA_TABLE_ENTRY ModuleObject;
    NTSTATUS Status;
    UNICODE_STRING DriverName;
    ULONG i, Index;
    PDRIVER_INFORMATION DriverInfo, DriverInfoTag;
    HANDLE KeyHandle;
    PBOOT_DRIVER_LIST_ENTRY BootEntry;
    DPRINT("IopInitializeBootDrivers()\n");

    /* Use IopRootDeviceNode for now */
    Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, NULL, &DeviceNode);
    if (!NT_SUCCESS(Status)) return;

    /* Setup the module object for the RAW FS Driver */
    ModuleObject.DllBase = NULL;
    ModuleObject.SizeOfImage = 0;
    ModuleObject.EntryPoint = RawFsDriverEntry;
    RtlInitUnicodeString(&DriverName, L"RAW");

    /* Initialize it */
    Status = IopInitializeDriverModule(DeviceNode,
                                       &ModuleObject,
                                       &DriverName,
                                       TRUE,
                                       &DriverObject);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        return;
    }

    /* Now initialize the associated device */
    Status = IopInitializeDevice(DeviceNode, DriverObject);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ObDereferenceObject(DriverObject);
        return;
    }

    /* Start it up */
    Status = IopStartDevice(DeviceNode);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ObDereferenceObject(DriverObject);
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
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
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

        /* Go to the next driver */
        NextEntry = NextEntry->Flink;
    }

    /* Loop the boot drivers */
    ListHead = &KeLoaderBlock->BootDriverListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the entry */
        BootEntry = CONTAINING_RECORD(NextEntry,
                                      BOOT_DRIVER_LIST_ENTRY,
                                      Link);

        /* Get the driver loader entry */
        LdrEntry = BootEntry->LdrEntry;

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
            DPRINT1("IopOpenRegistryKeyEx(%wZ) returned 0x%08lx\n", &BootEntry->RegistryPath, Status);
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

        /* Go to the next driver */
        NextEntry = NextEntry->Flink;
    }

    /* Loop each group index */
    for (i = 0; i < IopGroupIndex; i++)
    {
        /* Loop each group table */
        NextEntry = IopGroupTable[i].Flink;
        while (NextEntry != &IopGroupTable[i])
        {
            /* Get the entry */
            DriverInfo = CONTAINING_RECORD(NextEntry,
                                           DRIVER_INFORMATION,
                                           Link);

            /* Get the driver loader entry */
            LdrEntry = DriverInfo->DataTableEntry->LdrEntry;

            /* Initialize it */
            IopInitializeBuiltinDriver(LdrEntry);

            /* Next entry */
            NextEntry = NextEntry->Flink;
        }
    }

    /* In old ROS, the loader list became empty after this point. Simulate. */
    InitializeListHead(&KeLoaderBlock->LoadOrderListHead);
}

VOID
FASTCALL
INIT_FUNCTION
IopInitializeSystemDrivers(VOID)
{
    PUNICODE_STRING *DriverList, *SavedList;

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
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    UNICODE_STRING ImagePath;
    UNICODE_STRING ServiceName;
    UNICODE_STRING ObjectName;
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT DeviceObject;
    PEXTENDED_DEVOBJ_EXTENSION DeviceExtension;
    NTSTATUS Status;
    PWSTR Start;
    BOOLEAN SafeToUnload = TRUE;

    DPRINT("IopUnloadDriver('%wZ', %u)\n", DriverServiceName, UnloadPnpDrivers);

    PAGED_CODE();

    /*
     * Get the service name from the registry key name
     */
    Start = wcsrchr(DriverServiceName->Buffer, L'\\');
    if (Start == NULL)
        Start = DriverServiceName->Buffer;
    else
        Start++;

    RtlInitUnicodeString(&ServiceName, Start);

    /*
     * Construct the driver object name
     */
    ObjectName.Length = ((USHORT)wcslen(Start) + 8) * sizeof(WCHAR);
    ObjectName.MaximumLength = ObjectName.Length + sizeof(WCHAR);
    ObjectName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                              ObjectName.MaximumLength,
                                              TAG_IO);
    if (!ObjectName.Buffer) return STATUS_INSUFFICIENT_RESOURCES;
    wcscpy(ObjectName.Buffer, DRIVER_ROOT_NAME);
    memcpy(ObjectName.Buffer + 8, Start, ObjectName.Length - 8 * sizeof(WCHAR));
    ObjectName.Buffer[ObjectName.Length/sizeof(WCHAR)] = UNICODE_NULL;

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
        return Status;
    }

    /* Free the buffer for driver object name */
    ExFreePoolWithTag(ObjectName.Buffer, TAG_IO);

    /* Check that driver is not already unloading */
    if (DriverObject->Flags & DRVO_UNLOAD_INVOKED)
    {
        DPRINT1("Driver deletion pending\n");
        ObDereferenceObject(DriverObject);
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
                                    DriverServiceName->Buffer,
                                    QueryTable,
                                    NULL,
                                    NULL);

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
        Status = IopLoadUnloadDriver(NULL, &DriverObject);
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
}

NTSTATUS
NTAPI
IopCreateDriver(IN PUNICODE_STRING DriverName OPTIONAL,
                IN PDRIVER_INITIALIZE InitializationFunction,
                IN PUNICODE_STRING RegistryPath,
                IN PCUNICODE_STRING ServiceName,
                PLDR_DATA_TABLE_ENTRY ModuleObject,
                OUT PDRIVER_OBJECT *pDriverObject)
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
    DriverObject->Flags = DRVO_LEGACY_DRIVER;
    DriverObject->DriverExtension = (PDRIVER_EXTENSION)(DriverObject + 1);
    DriverObject->DriverExtension->DriverObject = DriverObject;
    DriverObject->DriverInit = InitializationFunction;
    DriverObject->DriverSection = ModuleObject;
    /* Loop all Major Functions */
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        /* Invalidate each function */
        DriverObject->MajorFunction[i] = IopInvalidDeviceRequest;
    }

    /* Set up the service key name buffer */
    ServiceKeyName.MaximumLength = ServiceName->Length + sizeof(UNICODE_NULL);
    ServiceKeyName.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                  ServiceKeyName.MaximumLength,
                                                  TAG_IO);
    if (!ServiceKeyName.Buffer)
    {
        /* Fail */
        ObMakeTemporaryObject(DriverObject);
        ObDereferenceObject(DriverObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy the name and set it in the driver extension */
    RtlCopyUnicodeString(&ServiceKeyName,
                         ServiceName);
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

    RtlCopyUnicodeString(&DriverObject->DriverName,
                         &LocalDriverName);

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

    DriverObject->HardwareDatabase = &IopHardwareDatabaseKey;
    DriverObject->DriverStart = ModuleObject ? ModuleObject->DllBase : 0;
    DriverObject->DriverSize = ModuleObject ? ModuleObject->SizeOfImage : 0;

    /* Finally, call its init function */
    DPRINT("RegistryKey: %wZ\n", RegistryPath);
    DPRINT("Calling driver entrypoint at %p\n", InitializationFunction);
    Status = (*InitializationFunction)(DriverObject, RegistryPath);
    if (!NT_SUCCESS(Status))
    {
        /* If it didn't work, then kill the object */
        DPRINT1("'%wZ' initialization failed, status (0x%08lx)\n", DriverName, Status);
        DriverObject->DriverSection = NULL;
        ObMakeTemporaryObject(DriverObject);
        ObDereferenceObject(DriverObject);
        return Status;
    }
    else
    {
        /* Returns to caller the object */
        *pDriverObject = DriverObject;
    }

    /* We're going to say if we don't have any DOs from DriverEntry, then we're not legacy.
     * Other parts of the I/O manager depend on this behavior */
    if (!DriverObject->DeviceObject) DriverObject->Flags &= ~DRVO_LEGACY_DRIVER;

    /* Loop all Major Functions */
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

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoCreateDriver(IN PUNICODE_STRING DriverName OPTIONAL,
               IN PDRIVER_INITIALIZE InitializationFunction)
{
    PDRIVER_OBJECT DriverObject;
    return IopCreateDriver(DriverName, InitializationFunction, NULL, DriverName, NULL, &DriverObject);
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

VOID
NTAPI
IopLoadUnloadDriverWorker(
    _Inout_ PVOID Parameter)
{
    PLOAD_UNLOAD_PARAMS LoadParams = Parameter;

    ASSERT(PsGetCurrentProcess() == PsInitialSystemProcess);
    LoadParams->Status = IopLoadUnloadDriver(LoadParams->RegistryPath,
                                             &LoadParams->DriverObject);
    KeSetEvent(&LoadParams->Event, 0, FALSE);
}

NTSTATUS
NTAPI
IopLoadUnloadDriver(
    _In_opt_ PCUNICODE_STRING RegistryPath,
    _Inout_ PDRIVER_OBJECT *DriverObject)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    UNICODE_STRING ImagePath;
    UNICODE_STRING ServiceName;
    NTSTATUS Status;
    ULONG Type;
    PDEVICE_NODE DeviceNode;
    PLDR_DATA_TABLE_ENTRY ModuleObject;
    PVOID BaseAddress;
    WCHAR *cur;

    /* Load/Unload must be called from system process */
    if (PsGetCurrentProcess() != PsInitialSystemProcess)
    {
        LOAD_UNLOAD_PARAMS LoadParams;

        /* Prepare parameters block */
        LoadParams.RegistryPath = RegistryPath;
        LoadParams.DriverObject = *DriverObject;
        KeInitializeEvent(&LoadParams.Event, NotificationEvent, FALSE);

        /* Initialize and queue a work item */
        ExInitializeWorkItem(&LoadParams.WorkItem,
                             IopLoadUnloadDriverWorker,
                             &LoadParams);
        ExQueueWorkItem(&LoadParams.WorkItem, DelayedWorkQueue);

        /* And wait till it completes */
        KeWaitForSingleObject(&LoadParams.Event,
                              UserRequest,
                              KernelMode,
                              FALSE,
                              NULL);
        return LoadParams.Status;
    }

    /* Check if it's an unload request */
    if (*DriverObject)
    {
        (*DriverObject)->DriverUnload(*DriverObject);
        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&ImagePath, NULL);

    /*
     * Get the service name from the registry key name.
     */
    ASSERT(RegistryPath->Length >= sizeof(WCHAR));

    ServiceName = *RegistryPath;
    cur = RegistryPath->Buffer + RegistryPath->Length / sizeof(WCHAR) - 1;
    while (RegistryPath->Buffer != cur)
    {
        if (*cur == L'\\')
        {
            ServiceName.Buffer = cur + 1;
            ServiceName.Length = RegistryPath->Length -
                                 (USHORT)((ULONG_PTR)ServiceName.Buffer -
                                          (ULONG_PTR)RegistryPath->Buffer);
            break;
        }
        cur--;
    }

    /*
     * Get service type.
     */
    RtlZeroMemory(&QueryTable, sizeof(QueryTable));

    RtlInitUnicodeString(&ImagePath, NULL);

    QueryTable[0].Name = L"Type";
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].EntryContext = &Type;

    QueryTable[1].Name = L"ImagePath";
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[1].EntryContext = &ImagePath;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    RegistryPath->Buffer,
                                    QueryTable, NULL, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
        if (ImagePath.Buffer) ExFreePool(ImagePath.Buffer);
        return Status;
    }

    /*
     * Normalize the image path for all later processing.
     */
    Status = IopNormalizeImagePath(&ImagePath, &ServiceName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopNormalizeImagePath() failed (Status %x)\n", Status);
        return Status;
    }

    DPRINT("FullImagePath: '%wZ'\n", &ImagePath);
    DPRINT("Type: %lx\n", Type);

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&IopDriverLoadResource, TRUE);
    /*
     * Get existing DriverObject pointer (in case the driver
     * has already been loaded and initialized).
     */
    Status = IopGetDriverObject(DriverObject,
                                &ServiceName,
                                (Type == SERVICE_FILE_SYSTEM_DRIVER ||
                                 Type == SERVICE_RECOGNIZER_DRIVER));

    if (!NT_SUCCESS(Status))
    {
        /*
         * Load the driver module
         */
        DPRINT("Loading module from %wZ\n", &ImagePath);
        Status = MmLoadSystemImage(&ImagePath, NULL, NULL, 0, (PVOID)&ModuleObject, &BaseAddress);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("MmLoadSystemImage() failed (Status %lx)\n", Status);
            ExReleaseResourceLite(&IopDriverLoadResource);
            KeLeaveCriticalRegion();
            return Status;
        }

        /*
         * Initialize the driver module if it's loaded for the first time
         */
        Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &ServiceName, &DeviceNode);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopCreateDeviceNode() failed (Status %lx)\n", Status);
            ExReleaseResourceLite(&IopDriverLoadResource);
            KeLeaveCriticalRegion();
            MmUnloadSystemImage(ModuleObject);
            return Status;
        }

        IopDisplayLoadingMessage(&DeviceNode->ServiceName);

        Status = IopInitializeDriverModule(DeviceNode,
                                           ModuleObject,
                                           &DeviceNode->ServiceName,
                                           (Type == SERVICE_FILE_SYSTEM_DRIVER ||
                                            Type == SERVICE_RECOGNIZER_DRIVER),
                                           DriverObject);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopInitializeDriverModule() failed (Status %lx)\n", Status);
            ExReleaseResourceLite(&IopDriverLoadResource);
            KeLeaveCriticalRegion();
            MmUnloadSystemImage(ModuleObject);
            return Status;
        }

        ExReleaseResourceLite(&IopDriverLoadResource);
        KeLeaveCriticalRegion();

        /* Initialize and start device */
        IopInitializeDevice(DeviceNode, *DriverObject);
        Status = IopStartDevice(DeviceNode);
    }
    else
    {
        ExReleaseResourceLite(&IopDriverLoadResource);
        KeLeaveCriticalRegion();

        DPRINT("DriverObject already exist in ObjectManager\n");
        Status = STATUS_IMAGE_ALREADY_LOADED;

        /* IopGetDriverObject references the DriverObject, so dereference it */
        ObDereferenceObject(*DriverObject);
    }

    return Status;
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
    UNICODE_STRING CapturedDriverServiceName = { 0, 0, NULL };
    KPROCESSOR_MODE PreviousMode;
    PDRIVER_OBJECT DriverObject;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    /*
     * Check security privileges
     */

    /* FIXME: Uncomment when privileges will be correctly implemented. */
#if 0
    if (!SeSinglePrivilegeCheck(SeLoadDriverPrivilege, PreviousMode))
    {
        DPRINT("Privilege not held\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }
#endif

    Status = ProbeAndCaptureUnicodeString(&CapturedDriverServiceName,
                                          PreviousMode,
                                          DriverServiceName);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    DPRINT("NtLoadDriver('%wZ')\n", &CapturedDriverServiceName);

    /* Load driver and call its entry point */
    DriverObject = NULL;
    Status = IopLoadUnloadDriver(&CapturedDriverServiceName, &DriverObject);

    ReleaseCapturedUnicodeString(&CapturedDriverServiceName,
                                 PreviousMode);

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
