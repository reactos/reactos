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
#include <internal/debug.h>

/* GLOBALS ********************************************************************/

LIST_ENTRY DriverReinitListHead;
KSPIN_LOCK DriverReinitListLock;
PLIST_ENTRY DriverReinitTailEntry;

PLIST_ENTRY DriverBootReinitTailEntry;
LIST_ENTRY DriverBootReinitListHead;
KSPIN_LOCK DriverBootReinitListLock;

UNICODE_STRING IopHardwareDatabaseKey =
   RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\HARDWARE\\DESCRIPTION\\SYSTEM");

POBJECT_TYPE IoDriverObjectType = NULL;

extern BOOLEAN ExpInTextModeSetup;
extern BOOLEAN NoGuiBoot;

/* DECLARATIONS ***************************************************************/

NTSTATUS
LdrProcessModule(
    PVOID ModuleLoadBase,
    PUNICODE_STRING ModuleName,
    PLDR_DATA_TABLE_ENTRY *ModuleObject
);

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS STDCALL
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

    /* Get the extension and loop them */
    DriverExtension = IoGetDrvObjExtension(DriverObject)->
                      ClientDriverExtension;
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
        LdrpUnloadImage(DriverObject->DriverSection);
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

NTSTATUS FASTCALL
IopGetDriverObject(
   PDRIVER_OBJECT *DriverObject,
   PUNICODE_STRING ServiceName,
   BOOLEAN FileSystem)
{
   PDRIVER_OBJECT Object;
   WCHAR NameBuffer[MAX_PATH];
   UNICODE_STRING DriverName;
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;

   DPRINT("IopOpenDriverObject(%p '%wZ' %x)\n",
      DriverObject, ServiceName, FileSystem);

   *DriverObject = NULL;

   /* Create ModuleName string */
   if (ServiceName == NULL || ServiceName->Buffer == NULL)
      /* We don't know which DriverObject we have to open */
      return STATUS_INVALID_PARAMETER_2;

   DriverName.Buffer = NameBuffer;
   DriverName.Length = 0;
   DriverName.MaximumLength = sizeof(NameBuffer);

   if (FileSystem == TRUE)
      RtlAppendUnicodeToString(&DriverName, FILESYSTEM_ROOT_NAME);
   else
      RtlAppendUnicodeToString(&DriverName, DRIVER_ROOT_NAME);
   RtlAppendUnicodeStringToString(&DriverName, ServiceName);

   DPRINT("Driver name: '%wZ'\n", &DriverName);

   /* Initialize ObjectAttributes for driver object */
   InitializeObjectAttributes(
      &ObjectAttributes,
      &DriverName,
      OBJ_OPENIF | OBJ_KERNEL_HANDLE,
      NULL,
      NULL);

   /* Open driver object */
   Status = ObReferenceObjectByName(
      &DriverName,
      0, /* Attributes */
      NULL, /* PassedAccessState */
      0, /* DesiredAccess */
      IoDriverObjectType,
      KernelMode,
      NULL, /* ParseContext */
      (PVOID*)&Object);

   if (!NT_SUCCESS(Status))
      return Status;

   *DriverObject = Object;

   return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
IopCreateDriverObject(
   PDRIVER_OBJECT *DriverObject,
   PUNICODE_STRING ServiceName,
   ULONG CreateAttributes,
   BOOLEAN FileSystem,
   PVOID DriverImageStart,
   ULONG DriverImageSize)
{
   PDRIVER_OBJECT Object;
   WCHAR NameBuffer[MAX_PATH];
   UNICODE_STRING DriverName;
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;
   ULONG i;
   PWSTR Buffer = NULL;

   DPRINT("IopCreateDriverObject(%p '%wZ' %x %p %x)\n",
      DriverObject, ServiceName, FileSystem, DriverImageStart, DriverImageSize);

   *DriverObject = NULL;

   /* Create ModuleName string */
   if (ServiceName != NULL && ServiceName->Buffer != NULL)
   {
      if (FileSystem == TRUE)
         wcscpy(NameBuffer, FILESYSTEM_ROOT_NAME);
      else
         wcscpy(NameBuffer, DRIVER_ROOT_NAME);
      wcscat(NameBuffer, ServiceName->Buffer);

      RtlInitUnicodeString(&DriverName, NameBuffer);
      DPRINT("Driver name: '%wZ'\n", &DriverName);

      Buffer = (PWSTR)ExAllocatePool(PagedPool, DriverName.Length + sizeof(WCHAR));
      /* If we don't success, it is not a problem. Our driver
       * object will not have associated driver name... */
   }
   else
   {
      RtlInitUnicodeString(&DriverName, NULL);
   }

   /* Initialize ObjectAttributes for driver object */
   InitializeObjectAttributes(
      &ObjectAttributes,
      &DriverName,
      CreateAttributes | OBJ_PERMANENT,
      NULL,
      NULL);

   /* Create driver object */
   Status = ObCreateObject(
      KernelMode,
      IoDriverObjectType,
      &ObjectAttributes,
      KernelMode,
      NULL,
      sizeof(DRIVER_OBJECT),
      0,
      0,
      (PVOID*)&Object);

   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   /* Create driver extension */
   Object->DriverExtension = (PDRIVER_EXTENSION)
      ExAllocatePoolWithTag(
         NonPagedPool,
         sizeof(EXTENDED_DRIVER_EXTENSION),
         TAG_DRIVER_EXTENSION);

   if (Object->DriverExtension == NULL)
   {
      return STATUS_NO_MEMORY;
   }

   RtlZeroMemory(Object->DriverExtension, sizeof(EXTENDED_DRIVER_EXTENSION));

   Object->Type = IO_TYPE_DRIVER;

   for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
      Object->MajorFunction[i] = IopInvalidDeviceRequest;

   Object->HardwareDatabase = &IopHardwareDatabaseKey;

   Object->DriverStart = DriverImageStart;
   Object->DriverSize = DriverImageSize;
   if (Buffer)
   {
      if (!Object->DriverName.Buffer)
      {
         Object->DriverName.Buffer = Buffer;
         Object->DriverName.Length = DriverName.Length;
         Object->DriverName.MaximumLength = DriverName.Length + sizeof(WCHAR);
         RtlCopyMemory(Object->DriverName.Buffer, DriverName.Buffer, DriverName.Length);
         Object->DriverName.Buffer[Object->DriverName.Length / sizeof(WCHAR)] = L'\0';
      }
      else
         ExFreePool(Buffer);
   }


   Status = ObInsertObject(Object,
                           NULL,
                           FILE_ALL_ACCESS,
                           0,
                           NULL,
                           NULL);
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }  

   *DriverObject = Object;

   return STATUS_SUCCESS;
}

/*
 * IopDisplayLoadingMessage
 *
 * Display 'Loading XXX...' message.
 */

VOID 
FASTCALL
INIT_FUNCTION
IopDisplayLoadingMessage(PVOID ServiceName, 
                         BOOLEAN Unicode)
{
    CHAR TextBuffer[256];
    if (ExpInTextModeSetup || !NoGuiBoot) return;
    if (Unicode) 
    {
        sprintf(TextBuffer, "Loading %S...\n", (PWCHAR)ServiceName);
    }
    else
    {
        sprintf(TextBuffer, "Loading %s...\n", (PCHAR)ServiceName);
    }
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

NTSTATUS FASTCALL
IopNormalizeImagePath(
   IN OUT PUNICODE_STRING ImagePath,
   IN PUNICODE_STRING ServiceName)
{
   UNICODE_STRING InputImagePath;

   RtlCopyMemory(
      &InputImagePath,
      ImagePath,
      sizeof(UNICODE_STRING));

   if (InputImagePath.Length == 0)
   {
      ImagePath->Length = (33 * sizeof(WCHAR)) + ServiceName->Length;
      ImagePath->MaximumLength = ImagePath->Length + sizeof(UNICODE_NULL);
      ImagePath->Buffer = ExAllocatePool(NonPagedPool, ImagePath->MaximumLength);
      if (ImagePath->Buffer == NULL)
         return STATUS_NO_MEMORY;

      wcscpy(ImagePath->Buffer, L"\\SystemRoot\\system32\\drivers\\");
      wcscat(ImagePath->Buffer, ServiceName->Buffer);
      wcscat(ImagePath->Buffer, L".sys");
   } else
   if (InputImagePath.Buffer[0] != L'\\')
   {
      ImagePath->Length = (12 * sizeof(WCHAR)) + InputImagePath.Length;
      ImagePath->MaximumLength = ImagePath->Length + sizeof(UNICODE_NULL);
      ImagePath->Buffer = ExAllocatePool(NonPagedPool, ImagePath->MaximumLength);
      if (ImagePath->Buffer == NULL)
         return STATUS_NO_MEMORY;

      wcscpy(ImagePath->Buffer, L"\\SystemRoot\\");
      wcscat(ImagePath->Buffer, InputImagePath.Buffer);
      ExFreePool(InputImagePath.Buffer);
   }

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

NTSTATUS FASTCALL
IopLoadServiceModule(
   IN PUNICODE_STRING ServiceName,
   OUT PLDR_DATA_TABLE_ENTRY *ModuleObject)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[3];
   ULONG ServiceStart;
   UNICODE_STRING ServiceImagePath;
   NTSTATUS Status;

   DPRINT("IopLoadServiceModule(%wZ, 0x%p)\n", ServiceName, ModuleObject);

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

   Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
      ServiceName->Buffer, QueryTable, NULL, NULL);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("RtlQueryRegistryValues() failed (Status %x)\n", Status);
      return Status;
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
    * Load the module.
    */

   *ModuleObject = LdrGetModuleObject(&ServiceImagePath);

   if (*ModuleObject == NULL)
   {
      Status = STATUS_UNSUCCESSFUL;

      /*
       * Special case for boot modules that were loaded by boot loader.
       */

      if (ServiceStart == 0)
      {
         WCHAR SearchNameBuffer[256];
         UNICODE_STRING SearchName;
         PLIST_ENTRY ListHead, NextEntry;
         PLDR_DATA_TABLE_ENTRY LdrEntry;

         /*
          * FIXME:
          * Improve this searching algorithm by using the image name
          * stored in registry entry ImageName and use the whole path
          * (requires change in FreeLoader).
          */
         swprintf(SearchNameBuffer, L"%wZ.sys", ServiceName);
         RtlInitUnicodeString(&SearchName, SearchNameBuffer);

         /* Loop the boot modules */
         ListHead = &KeLoaderBlock->LoadOrderListHead;
         NextEntry = ListHead->Flink->Flink;
         while (ListHead != NextEntry)
         {
            /* Get the entry */
            LdrEntry = CONTAINING_RECORD(NextEntry,
                                         LDR_DATA_TABLE_ENTRY,
                                         InLoadOrderLinks);

            /* Compare names */
            if (RtlEqualUnicodeString(&LdrEntry->BaseDllName, &SearchName, TRUE))
            {
                /* Tell, that the module is already loaded */
                LdrEntry->Flags |= LDRP_ENTRY_INSERTED;

                Status = LdrProcessModule(LdrEntry->DllBase,
                                          &ServiceImagePath,
                                          ModuleObject);

                KDB_SYMBOLFILE_HOOK(&SearchName);
                break;
            }

            /* Go to the next driver */
            NextEntry = NextEntry->Flink;
         }

         if (!NT_SUCCESS(Status))
            /* Try to load it. It may just have been installed by PnP manager */
            Status = LdrLoadModule(&ServiceImagePath, ModuleObject);
      }

      /*
       * Case for rest of the drivers (except disabled)
       */

      else if (ServiceStart < 4)
      {
         DPRINT("Loading module\n");
         Status = LdrLoadModule(&ServiceImagePath, ModuleObject);
      }
   }
   else
   {
      DPRINT("Module already loaded\n");
      Status = STATUS_IMAGE_ALREADY_LOADED;
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

/*
 * IopInitializeDriverModule
 *
 * Initalize a loaded driver.
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

NTSTATUS FASTCALL
IopInitializeDriverModule(
   IN PDEVICE_NODE DeviceNode,
   IN PLDR_DATA_TABLE_ENTRY ModuleObject,
   IN PUNICODE_STRING ServiceName,
   IN BOOLEAN FileSystemDriver,
   OUT PDRIVER_OBJECT *DriverObject)
{
   const WCHAR ServicesKeyName[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\";
   UNICODE_STRING RegistryKey;
   PDRIVER_INITIALIZE DriverEntry;
   PDRIVER_OBJECT Driver;
   PDEVICE_OBJECT DeviceObject;
   NTSTATUS Status;

   DriverEntry = ModuleObject->EntryPoint;

   if (ServiceName != NULL && ServiceName->Length != 0)
   {
      RegistryKey.Length = 0;
      RegistryKey.MaximumLength = sizeof(ServicesKeyName) + ServiceName->Length;
      RegistryKey.Buffer = ExAllocatePool(PagedPool, RegistryKey.MaximumLength);
      if (RegistryKey.Buffer == NULL)
      {
         return STATUS_INSUFFICIENT_RESOURCES;
      }
      RtlAppendUnicodeToString(&RegistryKey, ServicesKeyName);
      RtlAppendUnicodeStringToString(&RegistryKey, ServiceName);
   }
   else
   {
      RtlInitUnicodeString(&RegistryKey, NULL);
   }

   Status = IopCreateDriverObject(
      &Driver,
      ServiceName,
      0,
      FileSystemDriver,
      ModuleObject->DllBase,
      ModuleObject->SizeOfImage);
   *DriverObject = Driver;
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopCreateDriverObject failed (Status %x)\n", Status);
      return Status;
   }

   DPRINT("RegistryKey: %wZ\n", &RegistryKey);
   DPRINT("Calling driver entrypoint at %08lx\n", DriverEntry);

   Status = DriverEntry(Driver, &RegistryKey);

   RtlFreeUnicodeString(&RegistryKey);

   if (!NT_SUCCESS(Status))
   {
      ObMakeTemporaryObject(Driver);
      ObDereferenceObject(Driver);
      return Status;
   }

   /* Set the driver as initialized */
   Driver->Flags |= DRVO_INITIALIZED;
   DeviceObject = Driver->DeviceObject;
   while (DeviceObject)
   {
       /* Set every device as initialized too */
       DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
       DeviceObject = DeviceObject->NextDevice;
   }

   IopReinitializeDrivers();

   return STATUS_SUCCESS;
}

/*
 * IopAttachFilterDriversCallback
 *
 * Internal routine used by IopAttachFilterDrivers.
 */

NTSTATUS STDCALL
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

   for (Filters = ValueData;
        ((ULONG_PTR)Filters - (ULONG_PTR)ValueData) < ValueLength &&
        *Filters != 0;
        Filters += (ServiceName.Length / sizeof(WCHAR)) + 1)
   {
      DPRINT("Filter Driver: %S (%wZ)\n", Filters, &DeviceNode->InstancePath);
      ServiceName.Buffer = Filters;
      ServiceName.MaximumLength =
      ServiceName.Length = wcslen(Filters) * sizeof(WCHAR);

      /* Load and initialize the filter driver */
      Status = IopLoadServiceModule(&ServiceName, &ModuleObject);
      if (Status != STATUS_IMAGE_ALREADY_LOADED)
      {
         if (!NT_SUCCESS(Status))
            continue;

         Status = IopInitializeDriverModule(DeviceNode, ModuleObject, &ServiceName,
                                            FALSE, &DriverObject);
         if (!NT_SUCCESS(Status))
            continue;
      }
      else
      {
         /* get existing DriverObject pointer */
         Status = IopGetDriverObject(
            &DriverObject,
            &ServiceName,
            FALSE);
         if (!NT_SUCCESS(Status))
            continue;
      }

      Status = IopInitializeDevice(DeviceNode, DriverObject);
      if (!NT_SUCCESS(Status))
         continue;
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

NTSTATUS FASTCALL
IopAttachFilterDrivers(
   PDEVICE_NODE DeviceNode,
   BOOLEAN Lower)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[2];
   PWCHAR KeyBuffer;
   UNICODE_STRING Class;
   WCHAR ClassBuffer[40];
   NTSTATUS Status;

   /*
    * First load the device filters
    */

   QueryTable[0].QueryRoutine = IopAttachFilterDriversCallback;
   if (Lower)
     QueryTable[0].Name = L"LowerFilters";
   else
     QueryTable[0].Name = L"UpperFilters";
   QueryTable[0].EntryContext = NULL;
   QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
   QueryTable[1].QueryRoutine = NULL;
   QueryTable[1].Name = NULL;

   KeyBuffer = ExAllocatePool(
      PagedPool,
      (49 * sizeof(WCHAR)) + DeviceNode->InstancePath.Length);
   wcscpy(KeyBuffer, L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\");
   wcscat(KeyBuffer, DeviceNode->InstancePath.Buffer);

   RtlQueryRegistryValues(
      RTL_REGISTRY_ABSOLUTE,
      KeyBuffer,
      QueryTable,
      DeviceNode,
      NULL);

   /*
    * Now get the class GUID
    */

   Class.Length = 0;
   Class.MaximumLength = 40 * sizeof(WCHAR);
   Class.Buffer = ClassBuffer;
   QueryTable[0].QueryRoutine = NULL;
   QueryTable[0].Name = L"ClassGUID";
   QueryTable[0].EntryContext = &Class;
   QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_DIRECT;

   Status = RtlQueryRegistryValues(
      RTL_REGISTRY_ABSOLUTE,
      KeyBuffer,
      QueryTable,
      DeviceNode,
      NULL);

   ExFreePool(KeyBuffer);

   /*
    * Load the class filter driver
    */

   if (NT_SUCCESS(Status))
   {
      QueryTable[0].QueryRoutine = IopAttachFilterDriversCallback;
      if (Lower)
         QueryTable[0].Name = L"LowerFilters";
      else
         QueryTable[0].Name = L"UpperFilters";
      QueryTable[0].EntryContext = NULL;
      QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;

      KeyBuffer = ExAllocatePool(PagedPool, (58 * sizeof(WCHAR)) + Class.Length);
      wcscpy(KeyBuffer, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class\\");
      wcscat(KeyBuffer, ClassBuffer);

      RtlQueryRegistryValues(
         RTL_REGISTRY_ABSOLUTE,
         KeyBuffer,
         QueryTable,
         DeviceNode,
         NULL);

      ExFreePool(KeyBuffer);
   }

   return STATUS_SUCCESS;
}

static VOID INIT_FUNCTION
MiFreeBootDriverMemory(PVOID StartAddress, ULONG Length)
{
   ULONG i;

   for (i = 0; i < PAGE_ROUND_UP(Length) / PAGE_SIZE; i++)
   {
      MmDeleteVirtualMapping(NULL, (char*)StartAddress + i * PAGE_SIZE, TRUE, NULL, NULL);
   }
}

/*
 * IopInitializeBuiltinDriver
 *
 * Initialize a driver that is already loaded in memory.
 */

NTSTATUS FASTCALL INIT_FUNCTION
IopInitializeBuiltinDriver(
   PDEVICE_NODE ModuleDeviceNode,
   PVOID ModuleLoadBase,
   PUNICODE_STRING ModuleName,
   ULONG ModuleLength)
{
   PLDR_DATA_TABLE_ENTRY ModuleObject;
   PDEVICE_NODE DeviceNode;
   PDRIVER_OBJECT DriverObject;
   NTSTATUS Status;
   PWCHAR FileNameWithoutPath;
   LPWSTR FileExtension;

   DPRINT("Initializing driver '%wZ' at %08lx, length 0x%08lx\n",
      ModuleName, ModuleLoadBase, ModuleLength);

   /*
    * Display 'Loading XXX...' message
    */
   IopDisplayLoadingMessage(ModuleName->Buffer, TRUE);

   /*
    * Determine the right device object
    */

   if (ModuleDeviceNode == NULL)
   {
      /* Use IopRootDeviceNode for now */
      Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &DeviceNode);
      if (!NT_SUCCESS(Status))
      {
         CPRINT("Driver '%wZ' load failed, status (%x)\n", ModuleName, Status);
         return(Status);
      }
   } else
   {
      DeviceNode = ModuleDeviceNode;
   }

   /*
    * Generate filename without path (not needed by freeldr)
    */

   FileNameWithoutPath = wcsrchr(ModuleName->Buffer, L'\\');
   if (FileNameWithoutPath == NULL)
   {
      FileNameWithoutPath = ModuleName->Buffer;
   }

   /*
    * Load the module
    */
   RtlCreateUnicodeString(&DeviceNode->ServiceName, FileNameWithoutPath);
   Status = LdrProcessModule(ModuleLoadBase, &DeviceNode->ServiceName,
      &ModuleObject);
   if (!NT_SUCCESS(Status))
   {
      if (ModuleDeviceNode == NULL)
         IopFreeDeviceNode(DeviceNode);
      CPRINT("Driver '%wZ' load failed, status (%x)\n", ModuleName, Status);
      return Status;
   }

   /* Load symbols */
   KDB_SYMBOLFILE_HOOK(ModuleName);

   /*
    * Strip the file extension from ServiceName
    */

   FileExtension = wcsrchr(DeviceNode->ServiceName.Buffer, '.');
   if (FileExtension != NULL)
   {
      DeviceNode->ServiceName.Length -= wcslen(FileExtension) * sizeof(WCHAR);
      FileExtension[0] = 0;
   }

   /*
    * Initialize the driver
    */

   Status = IopInitializeDriverModule(DeviceNode, ModuleObject,
      &DeviceNode->ServiceName, FALSE, &DriverObject);

   if (!NT_SUCCESS(Status))
   {
      if (ModuleDeviceNode == NULL)
         IopFreeDeviceNode(DeviceNode);
      CPRINT("Driver '%wZ' load failed, status (%x)\n", ModuleName, Status);
      return Status;
   }

   Status = IopInitializeDevice(DeviceNode, DriverObject);
   if (NT_SUCCESS(Status))
   {
      Status = IopStartDevice(DeviceNode);
   }

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
IopInitializeBootDrivers(VOID)
{
    PLIST_ENTRY ListHead, NextEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    UNICODE_STRING NtosSymName = RTL_CONSTANT_STRING(L"ntoskrnl.sym");
    PDEVICE_NODE DeviceNode;
    PDRIVER_OBJECT DriverObject;
    LDR_DATA_TABLE_ENTRY ModuleObject;
    NTSTATUS Status;

    /* Use IopRootDeviceNode for now */
    Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &DeviceNode);
    if (!NT_SUCCESS(Status)) return;

    /* Setup the module object for the RAW FS Driver */
    ModuleObject.DllBase = NULL;
    ModuleObject.SizeOfImage = 0;
    ModuleObject.EntryPoint = RawFsDriverEntry;

    /* Initialize it */
    Status = IopInitializeDriverModule(DeviceNode,
                                       &ModuleObject,
                                       &DeviceNode->ServiceName,
                                       TRUE,
                                       &DriverObject);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        IopFreeDeviceNode(DeviceNode);
        return;
    }

    /* Now initialize the associated device */
    Status = IopInitializeDevice(DeviceNode, DriverObject);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        IopFreeDeviceNode(DeviceNode);
        return;
    }

    /* Start it up */
    Status = IopStartDevice(DeviceNode);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        IopFreeDeviceNode(DeviceNode);
        return;
    }

    /* Hack for NTOSKRNL.SYM */
    KDB_SYMBOLFILE_HOOK(&NtosSymName);

    /* Loop the boot modules */
    ListHead = &KeLoaderBlock->LoadOrderListHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        /*
         * HACK: Make sure we're loading a driver
         * (we should be using BootDriverListHead!)
         */
        if (wcsstr(LdrEntry->BaseDllName.Buffer, L".sys"))
        {
            /* Make sure we didn't load this driver already */
            if (!(LdrEntry->Flags & LDRP_ENTRY_INSERTED))
            {
                /* Initialize it */
                IopInitializeBuiltinDriver(NULL,
                                           LdrEntry->DllBase,
                                           &LdrEntry->BaseDllName,
                                           LdrEntry->SizeOfImage);
            }
        }

        /* Go to the next driver */
        NextEntry = NextEntry->Flink;
    }

    /* Loop modules again */
    NextEntry = ListHead->Flink->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        /* Free memory */
        MiFreeBootDriverMemory(LdrEntry->DllBase, LdrEntry->SizeOfImage);

        /* Go to the next driver */
        NextEntry = NextEntry->Flink;
    }

    /* In old ROS, the loader list became empty after this point. Simulate. */
    InitializeListHead(&KeLoaderBlock->LoadOrderListHead);
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

NTSTATUS STDCALL
IopUnloadDriver(PUNICODE_STRING DriverServiceName, BOOLEAN UnloadPnpDrivers)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[2];
   UNICODE_STRING ImagePath;
   UNICODE_STRING ServiceName;
   UNICODE_STRING ObjectName;
   PDRIVER_OBJECT DriverObject;
   PLDR_DATA_TABLE_ENTRY ModuleObject;
   NTSTATUS Status;
   LPWSTR Start;

   DPRINT("IopUnloadDriver('%wZ', %d)\n", DriverServiceName, UnloadPnpDrivers);

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

   ObjectName.Length = (wcslen(Start) + 8) * sizeof(WCHAR);
   ObjectName.MaximumLength = ObjectName.Length + sizeof(WCHAR);
   ObjectName.Buffer = ExAllocatePool(PagedPool, ObjectName.MaximumLength);
   wcscpy(ObjectName.Buffer, L"\\Driver\\");
   memcpy(ObjectName.Buffer + 8, Start, (ObjectName.Length - 8) * sizeof(WCHAR));
   ObjectName.Buffer[ObjectName.Length/sizeof(WCHAR)] = 0;

   /*
    * Find the driver object
    */

   Status = ObReferenceObjectByName(&ObjectName, 0, 0, 0, IoDriverObjectType,
      KernelMode, 0, (PVOID*)&DriverObject);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Can't locate driver object for %wZ\n", ObjectName);
      return Status;
   }

   /*
    * Free the buffer for driver object name
    */

   ExFreePool(ObjectName.Buffer);

   /*
    * Get path of service...
    */

   RtlZeroMemory(QueryTable, sizeof(QueryTable));

   RtlInitUnicodeString(&ImagePath, NULL);

   QueryTable[0].Name = L"ImagePath";
   QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
   QueryTable[0].EntryContext = &ImagePath;

   Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
       DriverServiceName->Buffer, QueryTable, NULL, NULL);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("RtlQueryRegistryValues() failed (Status %x)\n", Status);
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

   /*
    * ... and check if it's loaded
    */

   ModuleObject = LdrGetModuleObject(&ImagePath);
   if (ModuleObject == NULL)
   {
      return STATUS_UNSUCCESSFUL;
   }

   /*
    * Free the service path
    */

   ExFreePool(ImagePath.Buffer);

   /*
    * Unload the module and release the references to the device object
    */

   if (DriverObject->DriverUnload)
      (*DriverObject->DriverUnload)(DriverObject);
   ObDereferenceObject(DriverObject);
   ObDereferenceObject(DriverObject);
   LdrUnloadModule(ModuleObject);

   return STATUS_SUCCESS;
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
        /* Get the item*/
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
        /* Get the item*/
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

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoCreateDriver(IN PUNICODE_STRING DriverName OPTIONAL,
               IN PDRIVER_INITIALIZE InitializationFunction)
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
    ULONG i;

    /* First, create a unique name for the driver if we don't have one */
    if (!DriverName)
    {
        /* Create a random name and set up the string*/
        NameLength = swprintf(NameBuffer, L"\\Driver\\%08u", KeTickCount);
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
                               OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
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
    ServiceKeyName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                  LocalDriverName.Length +
                                                  sizeof(WCHAR),
                                                  TAG_IO);
    if (!ServiceKeyName.Buffer)
    {
        /* Fail */
        ObMakeTemporaryObject(DriverObject);
        ObDereferenceObject(DriverObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Fill out the key data and copy the buffer */
    ServiceKeyName.Length = LocalDriverName.Length;
    ServiceKeyName.MaximumLength = LocalDriverName.MaximumLength;
    RtlMoveMemory(ServiceKeyName.Buffer,
                  LocalDriverName.Buffer,
                  LocalDriverName.Length);

    /* Null-terminate it and set it */
    ServiceKeyName.Buffer[ServiceKeyName.Length / sizeof(WCHAR)] = UNICODE_NULL;
    DriverObject->DriverExtension->ServiceKeyName =  ServiceKeyName;

    /* Also store it in the Driver Object. This is a bit of a hack. */
    RtlMoveMemory(&DriverObject->DriverName,
                  &ServiceKeyName,
                  sizeof(UNICODE_STRING));

    /* Add the Object and get its handle */
    Status = ObInsertObject(DriverObject,
                            NULL,
                            FILE_READ_DATA,
                            0,
                            NULL,
                            &hDriver);
    if (!NT_SUCCESS(Status)) return Status;

    /* Now reference it */
    Status = ObReferenceObjectByHandle(hDriver,
                                       0,
                                       IoDriverObjectType,
                                       KernelMode,
                                       (PVOID*)&DriverObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ObMakeTemporaryObject(DriverObject);
        ObDereferenceObject(DriverObject);
        return Status;
    }

    /* Close the extra handle */
    ZwClose(hDriver);

    /* Finally, call its init function */
    Status = (*InitializationFunction)(DriverObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        /* If it didn't work, then kill the object */
        ObMakeTemporaryObject(DriverObject);
        ObDereferenceObject(DriverObject);
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
    /* Simply derefence the Object */
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
    KfLowerIrql(OldIrql);

    /* Check if insertion failed */
    if (!Inserted)
    {
        /* Free the entry and fail */
        ExFreePool(NewDriverExtension);
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
    KfLowerIrql(OldIrql);

    /* Return nothing or the extension */
    if (!DriverExtensions) return NULL;
    return DriverExtensions + 1;
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
NTSTATUS STDCALL
NtLoadDriver(IN PUNICODE_STRING DriverServiceName)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[3];
   UNICODE_STRING ImagePath;
   UNICODE_STRING ServiceName;
   UNICODE_STRING CapturedDriverServiceName = {0};
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status;
   ULONG Type;
   PDEVICE_NODE DeviceNode;
   PLDR_DATA_TABLE_ENTRY ModuleObject;
   PDRIVER_OBJECT DriverObject;
   WCHAR *cur;

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

   RtlInitUnicodeString(&ImagePath, NULL);

   /*
    * Get the service name from the registry key name.
    */
   ASSERT(CapturedDriverServiceName.Length >= sizeof(WCHAR));

   ServiceName = CapturedDriverServiceName;
   cur = CapturedDriverServiceName.Buffer + (CapturedDriverServiceName.Length / sizeof(WCHAR)) - 1;
   while (CapturedDriverServiceName.Buffer != cur)
   {
      if(*cur == L'\\')
      {
         ServiceName.Buffer = cur + 1;
         ServiceName.Length = CapturedDriverServiceName.Length -
                              (USHORT)((ULONG_PTR)ServiceName.Buffer -
                                       (ULONG_PTR)CapturedDriverServiceName.Buffer);
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
      CapturedDriverServiceName.Buffer, QueryTable, NULL, NULL);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
      ExFreePool(ImagePath.Buffer);
      goto ReleaseCapturedString;
   }

   /*
    * Normalize the image path for all later processing.
    */

   Status = IopNormalizeImagePath(&ImagePath, &ServiceName);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopNormalizeImagePath() failed (Status %x)\n", Status);
      goto ReleaseCapturedString;
   }

   DPRINT("FullImagePath: '%wZ'\n", &ImagePath);
   DPRINT("Type: %lx\n", Type);

   /*
    * See, if the driver module isn't already loaded
    */

   ModuleObject = LdrGetModuleObject(&ImagePath);
   if (ModuleObject != NULL)
   {
      DPRINT("Image already loaded\n");
      Status = STATUS_IMAGE_ALREADY_LOADED;
      goto ReleaseCapturedString;
   }

   /*
    * Create device node
    */

   /* Use IopRootDeviceNode for now */
   Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &DeviceNode);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopCreateDeviceNode() failed (Status %lx)\n", Status);
      goto ReleaseCapturedString;
   }

   /*
    * Load the driver module
    */

   Status = LdrLoadModule(&ImagePath, &ModuleObject);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("LdrLoadModule() failed (Status %lx)\n", Status);
      IopFreeDeviceNode(DeviceNode);
      goto ReleaseCapturedString;
   }

   /*
    * Set a service name for the device node
    */

   RtlCreateUnicodeString(&DeviceNode->ServiceName, ServiceName.Buffer);

   /*
    * Initialize the driver module
    */

   Status = IopInitializeDriverModule(
      DeviceNode,
      ModuleObject,
      &DeviceNode->ServiceName,
      (Type == 2 /* SERVICE_FILE_SYSTEM_DRIVER */ ||
       Type == 8 /* SERVICE_RECOGNIZER_DRIVER */),
      &DriverObject);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopInitializeDriver() failed (Status %lx)\n", Status);
      LdrUnloadModule(ModuleObject);
      IopFreeDeviceNode(DeviceNode);
      goto ReleaseCapturedString;
   }

   IopInitializeDevice(DeviceNode, DriverObject);
   Status = IopStartDevice(DeviceNode);

ReleaseCapturedString:
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

NTSTATUS STDCALL
NtUnloadDriver(IN PUNICODE_STRING DriverServiceName)
{
   return IopUnloadDriver(DriverServiceName, FALSE);
}

/* EOF */
