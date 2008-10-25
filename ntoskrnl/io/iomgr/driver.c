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

LIST_ENTRY DriverReinitListHead;
KSPIN_LOCK DriverReinitListLock;
PLIST_ENTRY DriverReinitTailEntry;

PLIST_ENTRY DriverBootReinitTailEntry;
LIST_ENTRY DriverBootReinitListHead;
KSPIN_LOCK DriverBootReinitListLock;

UNICODE_STRING IopHardwareDatabaseKey =
   RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\HARDWARE\\DESCRIPTION\\SYSTEM");

POBJECT_TYPE IoDriverObjectType = NULL;

#define TAG_RTLREGISTRY TAG('R', 'q', 'r', 'v')

extern BOOLEAN ExpInTextModeSetup;

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
        //LdrpUnloadImage(DriverObject->DriverSection);
    }

    /* Check if it has a name */
    if (DriverObject->DriverName.Buffer)
    {
        /* Free it */
        ExFreePool(DriverObject->DriverName.Buffer);
    }

#if 0 /* See a bit of hack in IopCreateDriver */
    /* Check if it has a service key name */
    if (DriverObject->DriverExtension->ServiceKeyName.Buffer)
    {
        /* Free it */
        ExFreePool(DriverObject->DriverExtension->ServiceKeyName.Buffer);
    }
#endif
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
   NTSTATUS Status;

   DPRINT("IopGetDriverObject(%p '%wZ' %x)\n",
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

   /* Open driver object */
   Status = ObReferenceObjectByName(
      &DriverName,
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
    PCHAR Extra = ".sys";

    if (ExpInTextModeSetup) return;
    if (Unicode)
    {
        if (wcsstr(_wcsupr(ServiceName), L".SYS")) Extra = "";
        sprintf(TextBuffer,
                "%s%s%s\\%S%s\n",
                KeLoaderBlock->ArcBootDeviceName,
                KeLoaderBlock->NtBootPathName,
                "System32\\Drivers",
                (PWCHAR)ServiceName,
                Extra);
    }
    else
    {
        if (strstr(_strupr(ServiceName), ".SYS")) Extra = "";
        sprintf(TextBuffer,
                "%s%s%s\\%s%s\n",
                KeLoaderBlock->ArcBootDeviceName,
                KeLoaderBlock->NtBootPathName,
                "System32\\Drivers",
                (PCHAR)ServiceName,
                Extra);
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
      ImagePath->Length = 0;
      ImagePath->MaximumLength =
          (33 * sizeof(WCHAR)) + ServiceName->Length + sizeof(UNICODE_NULL);
      ImagePath->Buffer = ExAllocatePool(NonPagedPool, ImagePath->MaximumLength);
      if (ImagePath->Buffer == NULL)
         return STATUS_NO_MEMORY;

      RtlAppendUnicodeToString(ImagePath, L"\\SystemRoot\\system32\\drivers\\");
      RtlAppendUnicodeStringToString(ImagePath, ServiceName);
      RtlAppendUnicodeToString(ImagePath, L".sys");
   } else
   if (InputImagePath.Buffer[0] != L'\\')
   {
      ImagePath->Length = 0;
      ImagePath->MaximumLength =
          12 * sizeof(WCHAR) + InputImagePath.Length + sizeof(UNICODE_NULL);
      ImagePath->Buffer = ExAllocatePool(NonPagedPool, ImagePath->MaximumLength);
      if (ImagePath->Buffer == NULL)
         return STATUS_NO_MEMORY;

      RtlAppendUnicodeToString(ImagePath, L"\\SystemRoot\\");
      RtlAppendUnicodeStringToString(ImagePath, &InputImagePath);

      /* Free caller's string */
      ExFreePoolWithTag(InputImagePath.Buffer, TAG_RTLREGISTRY);
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
   UNICODE_STRING ServiceImagePath, CCSName;
   NTSTATUS Status;
   HANDLE CCSKey, ServiceKey;
   PVOID BaseAddress;

   DPRINT("IopLoadServiceModule(%wZ, 0x%p)\n", ServiceName, ModuleObject);

   /* FIXME: This check may be removed once the bug is fixed */
   if (ServiceName->Buffer == NULL)
   {
       DPRINT1("If you see this, please report to Fireball or hpoussin!\n");
      return STATUS_UNSUCCESSFUL;
   }

   /* Open CurrentControlSet */
   RtlInitUnicodeString(&CCSName,
                        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services");
   Status = IopOpenRegistryKeyEx(&CCSKey, NULL, &CCSName, KEY_READ);
   if (!NT_SUCCESS(Status))
   {
       DPRINT1("ZwOpenKey() failed with Status %08X\n", Status);
       return Status;
   }

   /* Open service key */
   Status = IopOpenRegistryKeyEx(&ServiceKey, CCSKey, ServiceName, KEY_READ);
   if (!NT_SUCCESS(Status))
   {
       DPRINT1("ZwOpenKey() failed with Status %08X\n", Status);
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
      (PWSTR)ServiceKey, QueryTable, NULL, NULL);

   ZwClose(ServiceKey);
   ZwClose(CCSKey);

   if (!NT_SUCCESS(Status))
   {
      DPRINT1("RtlQueryRegistryValues() failed (Status %x)\n", Status);
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
       * Case for disabled drivers
       */

  if (ServiceStart >= 4)
  {
     /* FIXME: Check if it is the right status code */
     Status = STATUS_PLUGPLAY_NO_DEVICE;
  }
  else
  {
     DPRINT("Loading module\n");
     Status = MmLoadSystemImage(&ServiceImagePath, NULL, NULL, 0, (PVOID)ModuleObject, &BaseAddress);
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
   WCHAR NameBuffer[MAX_PATH];
   UNICODE_STRING DriverName;
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

   /* Create ModuleName string */
   if (ServiceName && ServiceName->Length > 0)
   {
      if (FileSystemDriver == TRUE)
         wcscpy(NameBuffer, FILESYSTEM_ROOT_NAME);
      else
         wcscpy(NameBuffer, DRIVER_ROOT_NAME);

      RtlInitUnicodeString(&DriverName, NameBuffer);
      DriverName.MaximumLength = sizeof(NameBuffer);

      RtlAppendUnicodeStringToString(&DriverName, ServiceName);

      DPRINT("Driver name: '%wZ'\n", &DriverName);
   }
   else
      DriverName.Length = 0;

   Status = IopCreateDriver(
       DriverName.Length > 0 ? &DriverName : NULL,
       DriverEntry,
       &RegistryKey,
       ModuleObject->DllBase,
       ModuleObject->SizeOfImage,
       &Driver);
   RtlFreeUnicodeString(&RegistryKey);

   *DriverObject = Driver;
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopCreateDriver() failed (Status 0x%08lx)\n", Status);
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
         {
            DPRINT1("IopGetDriverObject() returned status 0x%08x!\n", Status);
            continue;
         }
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
   RTL_QUERY_REGISTRY_TABLE QueryTable[2] = {{0}};
   UNICODE_STRING Class;
   WCHAR ClassBuffer[40];
   UNICODE_STRING EnumRoot = RTL_CONSTANT_STRING(ENUM_ROOT);
   HANDLE EnumRootKey, SubKey;
   NTSTATUS Status;

   /* Open enumeration root key */
   Status = IopOpenRegistryKeyEx(&EnumRootKey, NULL,
       &EnumRoot, KEY_READ);
   if (!NT_SUCCESS(Status))
   {
       DPRINT1("ZwOpenKey() failed with Status %08X\n", Status);
       return Status;
   }

   /* Open subkey */
   Status = IopOpenRegistryKeyEx(&SubKey, EnumRootKey,
       &DeviceNode->InstancePath, KEY_READ);
   if (!NT_SUCCESS(Status))
   {
       DPRINT1("ZwOpenKey() failed with Status %08X\n", Status);
       ZwClose(EnumRootKey);
       return Status;
   }

   /*
    * First load the device filters
    */
   QueryTable[0].QueryRoutine = IopAttachFilterDriversCallback;
   if (Lower)
     QueryTable[0].Name = L"LowerFilters";
   else
     QueryTable[0].Name = L"UpperFilters";
   QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;

   RtlQueryRegistryValues(
      RTL_REGISTRY_HANDLE,
      (PWSTR)SubKey,
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
      RTL_REGISTRY_HANDLE,
      (PWSTR)SubKey,
      QueryTable,
      DeviceNode,
      NULL);

   /* Close handles */
   ZwClose(SubKey);
   ZwClose(EnumRootKey);

   /*
    * Load the class filter driver
    */
   if (NT_SUCCESS(Status))
   {
       UNICODE_STRING ControlClass = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class");

       Status = IopOpenRegistryKeyEx(&EnumRootKey, NULL,
           &ControlClass, KEY_READ);
       if (!NT_SUCCESS(Status))
       {
           DPRINT1("ZwOpenKey() failed with Status %08X\n", Status);
           return Status;
       }

       /* Open subkey */
       Status = IopOpenRegistryKeyEx(&SubKey, EnumRootKey,
           &Class, KEY_READ);
       if (!NT_SUCCESS(Status))
       {
           DPRINT1("ZwOpenKey() failed with Status %08X\n", Status);
           ZwClose(EnumRootKey);
           return Status;
       }

      QueryTable[0].QueryRoutine = IopAttachFilterDriversCallback;
      if (Lower)
         QueryTable[0].Name = L"LowerFilters";
      else
         QueryTable[0].Name = L"UpperFilters";
      QueryTable[0].EntryContext = NULL;
      QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;

      RtlQueryRegistryValues(
         RTL_REGISTRY_HANDLE,
         (PWSTR)SubKey,
         QueryTable,
         DeviceNode,
         NULL);

      /* Clean up */
      ZwClose(SubKey);
      ZwClose(EnumRootKey);
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

extern KSPIN_LOCK PsLoadedModuleSpinLock;

//
// Used for images already loaded (boot drivers)
//
NTSTATUS
NTAPI
LdrProcessDriverModule(PLDR_DATA_TABLE_ENTRY LdrEntry,
                       PUNICODE_STRING FileName,
                       PLDR_DATA_TABLE_ENTRY *ModuleObject)
{
    NTSTATUS Status;
    PLDR_DATA_TABLE_ENTRY NewEntry;
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

    NewEntry = LdrEntry;

    /* Resolve imports */
    MissingApiName = Buffer;
    Status = MiResolveImageReferences(DriverBase,
                                      &BaseDirectory,
                                      NULL,
                                      &MissingApiName,
                                      &MissingDriverName,
                                      &LoadedImports);
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
IopInitializeBuiltinDriver(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PDEVICE_NODE DeviceNode;
    PDRIVER_OBJECT DriverObject;
    NTSTATUS Status;
    PWCHAR FileNameWithoutPath;
    LPWSTR FileExtension;
    PUNICODE_STRING ModuleName = &LdrEntry->BaseDllName;
    UNICODE_STRING ServiceName;

   /*
    * Display 'Loading XXX...' message
    */
   IopDisplayLoadingMessage(ModuleName->Buffer, TRUE);

   /*
    * Generate filename without path (not needed by freeldr)
    */
   FileNameWithoutPath = wcsrchr(ModuleName->Buffer, L'\\');
   if (FileNameWithoutPath == NULL)
   {
      FileNameWithoutPath = ModuleName->Buffer;
   }
   else
   {
      FileNameWithoutPath++;
   }

   /*
    * Strip the file extension from ServiceName
    */
   RtlCreateUnicodeString(&ServiceName, FileNameWithoutPath);
   FileExtension = wcsrchr(ServiceName.Buffer, '.');
   if (FileExtension != NULL)
   {
      ServiceName.Length -= wcslen(FileExtension) * sizeof(WCHAR);
      FileExtension[0] = 0;
   }

   /*
    * Determine the right device object
    */
   /* Use IopRootDeviceNode for now */
   Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &ServiceName, &DeviceNode);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Driver '%wZ' load failed, status (%x)\n", ModuleName, Status);
      return(Status);
   }
   DeviceNode->ServiceName = ServiceName;

   /*
    * Initialize the driver
    */
   Status = IopInitializeDriverModule(DeviceNode, LdrEntry,
      &DeviceNode->ServiceName, FALSE, &DriverObject);

   if (!NT_SUCCESS(Status))
   {
      IopFreeDeviceNode(DeviceNode);
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
    PDEVICE_NODE DeviceNode;
    PDRIVER_OBJECT DriverObject;
    LDR_DATA_TABLE_ENTRY ModuleObject;
    NTSTATUS Status;
    UNICODE_STRING DriverName;

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
        if (wcsstr(_wcsupr(LdrEntry->BaseDllName.Buffer), L".SYS"))
        {
            /* Make sure we didn't load this driver already */
            if (!(LdrEntry->Flags & LDRP_ENTRY_INSERTED))
            {
                DPRINT("Initializing bootdriver %wZ\n", &LdrEntry->BaseDllName);
                /* Initialize it */
                IopInitializeBuiltinDriver(LdrEntry);
            }
        }

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
   LOAD_UNLOAD_PARAMS LoadParams;
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
   memcpy(ObjectName.Buffer + 8, Start, ObjectName.Length - 8 * sizeof(WCHAR));
   ObjectName.Buffer[ObjectName.Length/sizeof(WCHAR)] = 0;

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

   /*
    * Free the buffer for driver object name
    */
   ExFreePool(ObjectName.Buffer);

   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Can't locate driver object for %wZ\n", &ObjectName);
      return Status;
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
       DriverServiceName->Buffer, QueryTable, NULL, NULL);

   if (!NT_SUCCESS(Status))
   {
      DPRINT1("RtlQueryRegistryValues() failed (Status %x)\n", Status);
      return Status;
   }

   /*
    * Normalize the image path for all later processing.
    */

   Status = IopNormalizeImagePath(&ImagePath, &ServiceName);

   if (!NT_SUCCESS(Status))
   {
      DPRINT1("IopNormalizeImagePath() failed (Status %x)\n", Status);
      return Status;
   }

   /*
    * Free the service path
    */

   ExFreePool(ImagePath.Buffer);

   /*
    * Unload the module and release the references to the device object
    */

    /* Call the load/unload routine, depending on current process */
   if (DriverObject->DriverUnload && DriverObject->DriverSection)
   {
      if (PsGetCurrentProcess() == PsInitialSystemProcess)
      {
         /* Just call right away */
         (*DriverObject->DriverUnload)(DriverObject);
      }
      else
      {
         /* Load/Unload must be called from system process */

         /* Prepare parameters block */
         LoadParams.DriverObject = DriverObject;
         KeInitializeEvent(&LoadParams.Event, NotificationEvent, FALSE);

         ExInitializeWorkItem(&LoadParams.WorkItem,
             (PWORKER_THREAD_ROUTINE)IopLoadUnloadDriver,
             (PVOID)&LoadParams);

         /* Queue it */
         ExQueueWorkItem(&LoadParams.WorkItem, DelayedWorkQueue);

         /* And wait when it completes */
         KeWaitForSingleObject(&LoadParams.Event, UserRequest, KernelMode,
             FALSE, NULL);
      }

      /* Mark the driver object temporary, so it could be deleted later */
      ObMakeTemporaryObject(DriverObject);

      /* Dereference it 2 times */
      ObDereferenceObject(DriverObject);
      ObDereferenceObject(DriverObject);

      /* Unload the driver */
      MmUnloadSystemImage(DriverObject->DriverSection);

      return STATUS_SUCCESS;
   }
   else
   {
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

NTSTATUS
NTAPI
IopCreateDriver(IN PUNICODE_STRING DriverName OPTIONAL,
                IN PDRIVER_INITIALIZE InitializationFunction,
                IN PUNICODE_STRING RegistryPath,
                IN PVOID DllBase,
                IN ULONG SizeOfImage,
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
        /* Create a random name and set up the string*/
        NameLength = (USHORT)swprintf(NameBuffer,
                                      L"\\Driver\\%08u",
                                      KeTickCount);
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
    RtlCopyMemory(ServiceKeyName.Buffer,
                  LocalDriverName.Buffer,
                  LocalDriverName.Length);

    /* Null-terminate it and set it */
    ServiceKeyName.Buffer[ServiceKeyName.Length / sizeof(WCHAR)] = UNICODE_NULL;
    DriverObject->DriverExtension->ServiceKeyName = ServiceKeyName;

    /* Also store it in the Driver Object. This is a bit of a hack. */
    RtlCopyMemory(&DriverObject->DriverName,
                  &ServiceKeyName,
                  sizeof(UNICODE_STRING));

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
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ObMakeTemporaryObject(DriverObject);
        ObDereferenceObject(DriverObject);
        return Status;
    }

    /* Close the extra handle */
    ZwClose(hDriver);

    DriverObject->HardwareDatabase = &IopHardwareDatabaseKey;
    DriverObject->DriverStart = DllBase;
    DriverObject->DriverSize = SizeOfImage;

    /* Finally, call its init function */
    DPRINT("RegistryKey: %wZ\n", RegistryPath);
    DPRINT("Calling driver entrypoint at %p\n", InitializationFunction);
    Status = (*InitializationFunction)(DriverObject, RegistryPath);
    if (!NT_SUCCESS(Status))
    {
        /* If it didn't work, then kill the object */
        DPRINT1("'%wZ' initialization failed, status (0x%08lx)\n", DriverName, Status);
        ObMakeTemporaryObject(DriverObject);
        ObDereferenceObject(DriverObject);
    }
    else
    {
        /* Returns to caller the object */
        *pDriverObject = DriverObject;
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
   return IopCreateDriver(DriverName, InitializationFunction, NULL, 0, 0, &DriverObject);
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
    KeLowerIrql(OldIrql);

    /* Return nothing or the extension */
    if (!DriverExtensions) return NULL;
    return DriverExtensions + 1;
}

VOID NTAPI
IopLoadUnloadDriver(PLOAD_UNLOAD_PARAMS LoadParams)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[3];
   UNICODE_STRING ImagePath;
   UNICODE_STRING ServiceName;
   NTSTATUS Status;
   ULONG Type;
   PDEVICE_NODE DeviceNode;
   PDRIVER_OBJECT DriverObject;
   PLDR_DATA_TABLE_ENTRY ModuleObject;
   PVOID BaseAddress;
   WCHAR *cur;

   /* Check if it's an unload request */
   if (LoadParams->DriverObject)
   {
       (*LoadParams->DriverObject->DriverUnload)(LoadParams->DriverObject);

       /* Return success and signal the event */
       LoadParams->Status = STATUS_SUCCESS;
      (VOID)KeSetEvent(&LoadParams->Event, 0, FALSE);
       return;
   }

   RtlInitUnicodeString(&ImagePath, NULL);

   /*
    * Get the service name from the registry key name.
    */
   ASSERT(LoadParams->ServiceName->Length >= sizeof(WCHAR));

   ServiceName = *LoadParams->ServiceName;
   cur = LoadParams->ServiceName->Buffer +
       (LoadParams->ServiceName->Length / sizeof(WCHAR)) - 1;
   while (LoadParams->ServiceName->Buffer != cur)
   {
      if(*cur == L'\\')
      {
         ServiceName.Buffer = cur + 1;
         ServiceName.Length = LoadParams->ServiceName->Length -
                              (USHORT)((ULONG_PTR)ServiceName.Buffer -
                                       (ULONG_PTR)LoadParams->ServiceName->Buffer);
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
      LoadParams->ServiceName->Buffer, QueryTable, NULL, NULL);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
      ExFreePool(ImagePath.Buffer);
      LoadParams->Status = Status;
      (VOID)KeSetEvent(&LoadParams->Event, 0, FALSE);
      return;
   }

   /*
    * Normalize the image path for all later processing.
    */

   Status = IopNormalizeImagePath(&ImagePath, &ServiceName);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopNormalizeImagePath() failed (Status %x)\n", Status);
      LoadParams->Status = Status;
      (VOID)KeSetEvent(&LoadParams->Event, 0, FALSE);
      return;
   }

   DPRINT("FullImagePath: '%wZ'\n", &ImagePath);
   DPRINT("Type: %lx\n", Type);

   /*
    * Create device node
    */

   /* Use IopRootDeviceNode for now */
   Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &ServiceName, &DeviceNode);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopCreateDeviceNode() failed (Status %lx)\n", Status);
      LoadParams->Status = Status;
      (VOID)KeSetEvent(&LoadParams->Event, 0, FALSE);
      return;
   }

   /* Get existing DriverObject pointer (in case the driver has
      already been loaded and initialized) */
   Status = IopGetDriverObject(
       &DriverObject,
       &ServiceName,
       (Type == 2 /* SERVICE_FILE_SYSTEM_DRIVER */ ||
        Type == 8 /* SERVICE_RECOGNIZER_DRIVER */));

   if (!NT_SUCCESS(Status))
   {
       /*
        * Load the driver module
        */

       Status = MmLoadSystemImage(&ImagePath, NULL, NULL, 0, (PVOID)&ModuleObject, &BaseAddress);
       if (!NT_SUCCESS(Status) && Status != STATUS_IMAGE_ALREADY_LOADED)
       {
           DPRINT("MmLoadSystemImage() failed (Status %lx)\n", Status);
           IopFreeDeviceNode(DeviceNode);
           LoadParams->Status = Status;
           (VOID)KeSetEvent(&LoadParams->Event, 0, FALSE);
           return;
       }

       /*
        * Set a service name for the device node
        */

       RtlCreateUnicodeString(&DeviceNode->ServiceName, ServiceName.Buffer);

       /*
        * Initialize the driver module if it's loaded for the first time
        */
       if (Status != STATUS_IMAGE_ALREADY_LOADED)
       {
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
               MmUnloadSystemImage(ModuleObject);
               IopFreeDeviceNode(DeviceNode);
               LoadParams->Status = Status;
               (VOID)KeSetEvent(&LoadParams->Event, 0, FALSE);
               return;
           }
       }

       /* Store its DriverSection, so that it could be unloaded */
       DriverObject->DriverSection = ModuleObject;
   }

   IopInitializeDevice(DeviceNode, DriverObject);
   LoadParams->Status = IopStartDevice(DeviceNode);
   (VOID)KeSetEvent(&LoadParams->Event, 0, FALSE);
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
    UNICODE_STRING CapturedDriverServiceName = {0};
    KPROCESSOR_MODE PreviousMode;
    LOAD_UNLOAD_PARAMS LoadParams;
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

    LoadParams.ServiceName = &CapturedDriverServiceName;
    LoadParams.DriverObject = NULL;
    KeInitializeEvent(&LoadParams.Event, NotificationEvent, FALSE);

    /* Call the load/unload routine, depending on current process */
    if (PsGetCurrentProcess() == PsInitialSystemProcess)
    {
        /* Just call right away */
        IopLoadUnloadDriver(&LoadParams);
    }
    else
    {
        /* Load/Unload must be called from system process */
        ExInitializeWorkItem(&LoadParams.WorkItem,
                             (PWORKER_THREAD_ROUTINE)IopLoadUnloadDriver,
                             (PVOID)&LoadParams);

        /* Queue it */
        ExQueueWorkItem(&LoadParams.WorkItem, DelayedWorkQueue);

        /* And wait when it completes */
        KeWaitForSingleObject(&LoadParams.Event, UserRequest, KernelMode,
            FALSE, NULL);
    }

    ReleaseCapturedUnicodeString(&CapturedDriverServiceName,
                                 PreviousMode);

    return LoadParams.Status;
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
