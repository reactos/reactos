/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnpmgr.c
 * PURPOSE:         Initializes the PnP manager
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Copyright 2007 Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

PDEVICE_NODE IopRootDeviceNode;
KSPIN_LOCK IopDeviceTreeLock;
ERESOURCE PpRegistryDeviceResource;
KGUARDED_MUTEX PpDeviceReferenceTableLock;
RTL_AVL_TABLE PpDeviceReferenceTable;

extern ERESOURCE IopDriverLoadResource;
extern ULONG ExpInitializationPhase;
extern BOOLEAN PnpSystemInit;

/* DATA **********************************************************************/

PDRIVER_OBJECT IopRootDriverObject;
PIO_BUS_TYPE_GUID_LIST PnpBusTypeGuidList = NULL;
LIST_ENTRY IopDeviceRelationsRequestList;
WORK_QUEUE_ITEM IopDeviceRelationsWorkItem;
BOOLEAN IopDeviceRelationsRequestInProgress;
KSPIN_LOCK IopDeviceRelationsSpinLock;

typedef struct _INVALIDATE_DEVICE_RELATION_DATA
{
    LIST_ENTRY RequestListEntry;
    PDEVICE_OBJECT DeviceObject;
    DEVICE_RELATION_TYPE Type;
} INVALIDATE_DEVICE_RELATION_DATA, *PINVALIDATE_DEVICE_RELATION_DATA;

/* FUNCTIONS *****************************************************************/
NTSTATUS
NTAPI
IopCreateDeviceKeyPath(IN PCUNICODE_STRING RegistryPath,
                       IN ULONG CreateOptions,
                       OUT PHANDLE Handle);

VOID
IopCancelPrepareDeviceForRemoval(PDEVICE_OBJECT DeviceObject);

NTSTATUS
IopPrepareDeviceForRemoval(PDEVICE_OBJECT DeviceObject, BOOLEAN Force);

PDEVICE_OBJECT
IopGetDeviceObjectFromDeviceInstance(PUNICODE_STRING DeviceInstance);

PDEVICE_NODE
FASTCALL
IopGetDeviceNode(PDEVICE_OBJECT DeviceObject)
{
   return ((PEXTENDED_DEVOBJ_EXTENSION)DeviceObject->DeviceObjectExtension)->DeviceNode;
}

VOID
IopFixupDeviceId(PWCHAR String)
{
    SIZE_T Length = wcslen(String), i;

    for (i = 0; i < Length; i++)
    {
        if (String[i] == L'\\')
            String[i] = L'#';
    }
}

VOID
NTAPI
IopInstallCriticalDevice(PDEVICE_NODE DeviceNode)
{
    NTSTATUS Status;
    HANDLE CriticalDeviceKey, InstanceKey;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING CriticalDeviceKeyU = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\CriticalDeviceDatabase");
    UNICODE_STRING CompatibleIdU = RTL_CONSTANT_STRING(L"CompatibleIDs");
    UNICODE_STRING HardwareIdU = RTL_CONSTANT_STRING(L"HardwareID");
    UNICODE_STRING ServiceU = RTL_CONSTANT_STRING(L"Service");
    UNICODE_STRING ClassGuidU = RTL_CONSTANT_STRING(L"ClassGUID");
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo;
    ULONG HidLength = 0, CidLength = 0, BufferLength;
    PWCHAR IdBuffer, OriginalIdBuffer;

    /* Open the device instance key */
    Status = IopCreateDeviceKeyPath(&DeviceNode->InstancePath, REG_OPTION_NON_VOLATILE, &InstanceKey);
    if (Status != STATUS_SUCCESS)
        return;

    Status = ZwQueryValueKey(InstanceKey,
                             &HardwareIdU,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &HidLength);
    if (Status != STATUS_BUFFER_OVERFLOW && Status != STATUS_BUFFER_TOO_SMALL)
    {
        ZwClose(InstanceKey);
        return;
    }

    Status = ZwQueryValueKey(InstanceKey,
                             &CompatibleIdU,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &CidLength);
    if (Status != STATUS_BUFFER_OVERFLOW && Status != STATUS_BUFFER_TOO_SMALL)
    {
        CidLength = 0;
    }

    BufferLength = HidLength + CidLength;
    BufferLength -= (((CidLength != 0) ? 2 : 1) * FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data));

    /* Allocate a buffer to hold data from both */
    OriginalIdBuffer = IdBuffer = ExAllocatePool(PagedPool, BufferLength);
    if (!IdBuffer)
    {
        ZwClose(InstanceKey);
        return;
    }

    /* Compute the buffer size */
    if (HidLength > CidLength)
        BufferLength = HidLength;
    else
        BufferLength = CidLength;

    PartialInfo = ExAllocatePool(PagedPool, BufferLength);
    if (!PartialInfo)
    {
        ZwClose(InstanceKey);
        ExFreePool(OriginalIdBuffer);
        return;
    }

    Status = ZwQueryValueKey(InstanceKey,
                             &HardwareIdU,
                             KeyValuePartialInformation,
                             PartialInfo,
                             HidLength,
                             &HidLength);
    if (Status != STATUS_SUCCESS)
    {
        ExFreePool(PartialInfo);
        ExFreePool(OriginalIdBuffer);
        ZwClose(InstanceKey);
        return;
    }

    /* Copy in HID info first (without 2nd terminating NULL if CID is present) */
    HidLength = PartialInfo->DataLength - ((CidLength != 0) ? sizeof(WCHAR) : 0);
    RtlCopyMemory(IdBuffer, PartialInfo->Data, HidLength);

    if (CidLength != 0)
    {
        Status = ZwQueryValueKey(InstanceKey,
                                 &CompatibleIdU,
                                 KeyValuePartialInformation,
                                 PartialInfo,
                                 CidLength,
                                 &CidLength);
        if (Status != STATUS_SUCCESS)
        {
            ExFreePool(PartialInfo);
            ExFreePool(OriginalIdBuffer);
            ZwClose(InstanceKey);
            return;
        }

        /* Copy CID next */
        CidLength = PartialInfo->DataLength;
        RtlCopyMemory(((PUCHAR)IdBuffer) + HidLength, PartialInfo->Data, CidLength);
    }

    /* Free our temp buffer */
    ExFreePool(PartialInfo);

    InitializeObjectAttributes(&ObjectAttributes,
                               &CriticalDeviceKeyU,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&CriticalDeviceKey,
                       KEY_ENUMERATE_SUB_KEYS,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /* The critical device database doesn't exist because
         * we're probably in 1st stage setup, but it's ok */
        ExFreePool(OriginalIdBuffer);
        ZwClose(InstanceKey);
        return;
    }

    while (*IdBuffer)
    {
        USHORT StringLength = (USHORT)wcslen(IdBuffer) + 1, Index;

        IopFixupDeviceId(IdBuffer);

        /* Look through all subkeys for a match */
        for (Index = 0; TRUE; Index++)
        {
            ULONG NeededLength;
            PKEY_BASIC_INFORMATION BasicInfo;

            Status = ZwEnumerateKey(CriticalDeviceKey,
                                    Index,
                                    KeyBasicInformation,
                                    NULL,
                                    0,
                                    &NeededLength);
            if (Status == STATUS_NO_MORE_ENTRIES)
                break;
            else if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
            {
                UNICODE_STRING ChildIdNameU, RegKeyNameU;

                BasicInfo = ExAllocatePool(PagedPool, NeededLength);
                if (!BasicInfo)
                {
                    /* No memory */
                    ExFreePool(OriginalIdBuffer);
                    ZwClose(CriticalDeviceKey);
                    ZwClose(InstanceKey);
                    return;
                }

                Status = ZwEnumerateKey(CriticalDeviceKey,
                                        Index,
                                        KeyBasicInformation,
                                        BasicInfo,
                                        NeededLength,
                                        &NeededLength);
                if (Status != STATUS_SUCCESS)
                {
                    /* This shouldn't happen */
                    ExFreePool(BasicInfo);
                    continue;
                }

                ChildIdNameU.Buffer = IdBuffer;
                ChildIdNameU.MaximumLength = ChildIdNameU.Length = (StringLength - 1) * sizeof(WCHAR);
                RegKeyNameU.Buffer = BasicInfo->Name;
                RegKeyNameU.MaximumLength = RegKeyNameU.Length = (USHORT)BasicInfo->NameLength;

                if (RtlEqualUnicodeString(&ChildIdNameU, &RegKeyNameU, TRUE))
                {
                    HANDLE ChildKeyHandle;

                    InitializeObjectAttributes(&ObjectAttributes,
                                               &ChildIdNameU,
                                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                               CriticalDeviceKey,
                                               NULL);

                    Status = ZwOpenKey(&ChildKeyHandle,
                                       KEY_QUERY_VALUE,
                                       &ObjectAttributes);
                    if (Status != STATUS_SUCCESS)
                    {
                        ExFreePool(BasicInfo);
                        continue;
                    }

                    /* Check if there's already a driver installed */
                    Status = ZwQueryValueKey(InstanceKey,
                                             &ClassGuidU,
                                             KeyValuePartialInformation,
                                             NULL,
                                             0,
                                             &NeededLength);
                    if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
                    {
                        ExFreePool(BasicInfo);
                        continue;
                    }

                    Status = ZwQueryValueKey(ChildKeyHandle,
                                             &ClassGuidU,
                                             KeyValuePartialInformation,
                                             NULL,
                                             0,
                                             &NeededLength);
                    if (Status != STATUS_BUFFER_OVERFLOW && Status != STATUS_BUFFER_TOO_SMALL)
                    {
                        ExFreePool(BasicInfo);
                        continue;
                    }

                    PartialInfo = ExAllocatePool(PagedPool, NeededLength);
                    if (!PartialInfo)
                    {
                        ExFreePool(OriginalIdBuffer);
                        ExFreePool(BasicInfo);
                        ZwClose(InstanceKey);
                        ZwClose(ChildKeyHandle);
                        ZwClose(CriticalDeviceKey);
                        return;
                    }

                    /* Read ClassGUID entry in the CDDB */
                    Status = ZwQueryValueKey(ChildKeyHandle,
                                             &ClassGuidU,
                                             KeyValuePartialInformation,
                                             PartialInfo,
                                             NeededLength,
                                             &NeededLength);
                    if (Status != STATUS_SUCCESS)
                    {
                        ExFreePool(BasicInfo);
                        continue;
                    }

                    /* Write it to the ENUM key */
                    Status = ZwSetValueKey(InstanceKey,
                                           &ClassGuidU,
                                           0,
                                           REG_SZ,
                                           PartialInfo->Data,
                                           PartialInfo->DataLength);
                    if (Status != STATUS_SUCCESS)
                    {
                        ExFreePool(BasicInfo);
                        ExFreePool(PartialInfo);
                        ZwClose(ChildKeyHandle);
                        continue;
                    }

                    Status = ZwQueryValueKey(ChildKeyHandle,
                                             &ServiceU,
                                             KeyValuePartialInformation,
                                             NULL,
                                             0,
                                             &NeededLength);
                    if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
                    {
                        ExFreePool(PartialInfo);
                        PartialInfo = ExAllocatePool(PagedPool, NeededLength);
                        if (!PartialInfo)
                        {
                            ExFreePool(OriginalIdBuffer);
                            ExFreePool(BasicInfo);
                            ZwClose(InstanceKey);
                            ZwClose(ChildKeyHandle);
                            ZwClose(CriticalDeviceKey);
                            return;
                        }

                        /* Read the service entry from the CDDB */
                        Status = ZwQueryValueKey(ChildKeyHandle,
                                                 &ServiceU,
                                                 KeyValuePartialInformation,
                                                 PartialInfo,
                                                 NeededLength,
                                                 &NeededLength);
                        if (Status != STATUS_SUCCESS)
                        {
                            ExFreePool(BasicInfo);
                            ExFreePool(PartialInfo);
                            ZwClose(ChildKeyHandle);
                            continue;
                        }

                        /* Write it to the ENUM key */
                        Status = ZwSetValueKey(InstanceKey,
                                               &ServiceU,
                                               0,
                                               REG_SZ,
                                               PartialInfo->Data,
                                               PartialInfo->DataLength);
                        if (Status != STATUS_SUCCESS)
                        {
                            ExFreePool(BasicInfo);
                            ExFreePool(PartialInfo);
                            ZwClose(ChildKeyHandle);
                            continue;
                        }

                        DPRINT("Installed service '%S' for critical device '%wZ'\n", PartialInfo->Data, &ChildIdNameU);
                    }
                    else
                    {
                        DPRINT1("Installed NULL service for critical device '%wZ'\n", &ChildIdNameU);
                    }

                    ExFreePool(OriginalIdBuffer);
                    ExFreePool(PartialInfo);
                    ExFreePool(BasicInfo);
                    ZwClose(InstanceKey);
                    ZwClose(ChildKeyHandle);
                    ZwClose(CriticalDeviceKey);

                    /* That's it */
                    return;
                }

                ExFreePool(BasicInfo);
            }
            else
            {
                /* Umm, not sure what happened here */
                continue;
            }
        }

        /* Advance to the next ID */
        IdBuffer += StringLength;
    }

    ExFreePool(OriginalIdBuffer);
    ZwClose(InstanceKey);
    ZwClose(CriticalDeviceKey);
}

NTSTATUS
FASTCALL
IopInitializeDevice(PDEVICE_NODE DeviceNode,
                    PDRIVER_OBJECT DriverObject)
{
   PDEVICE_OBJECT Fdo;
   NTSTATUS Status;

   if (!DriverObject)
   {
      /* Special case for bus driven devices */
      DeviceNode->Flags |= DNF_ADDED;
      return STATUS_SUCCESS;
   }

   if (!DriverObject->DriverExtension->AddDevice)
   {
      DeviceNode->Flags |= DNF_LEGACY_DRIVER;
   }

   if (DeviceNode->Flags & DNF_LEGACY_DRIVER)
   {
      DeviceNode->Flags |= DNF_ADDED + DNF_STARTED;
      return STATUS_SUCCESS;
   }

   /* This is a Plug and Play driver */
   DPRINT("Plug and Play driver found\n");
   ASSERT(DeviceNode->PhysicalDeviceObject);

   DPRINT("Calling %wZ->AddDevice(%wZ)\n",
      &DriverObject->DriverName,
      &DeviceNode->InstancePath);
   Status = DriverObject->DriverExtension->AddDevice(
      DriverObject, DeviceNode->PhysicalDeviceObject);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("%wZ->AddDevice(%wZ) failed with status 0x%x\n",
              &DriverObject->DriverName,
              &DeviceNode->InstancePath,
              Status);
      IopDeviceNodeSetFlag(DeviceNode, DNF_DISABLED);
      DeviceNode->Problem = CM_PROB_FAILED_ADD;
      return Status;
   }

   Fdo = IoGetAttachedDeviceReference(DeviceNode->PhysicalDeviceObject);

   /* Check if we have a ACPI device (needed for power management) */
   if (Fdo->DeviceType == FILE_DEVICE_ACPI)
   {
      static BOOLEAN SystemPowerDeviceNodeCreated = FALSE;

      /* There can be only one system power device */
      if (!SystemPowerDeviceNodeCreated)
      {
         PopSystemPowerDeviceNode = DeviceNode;
         ObReferenceObject(PopSystemPowerDeviceNode->PhysicalDeviceObject);
         SystemPowerDeviceNodeCreated = TRUE;
      }
   }

   ObDereferenceObject(Fdo);

   IopDeviceNodeSetFlag(DeviceNode, DNF_ADDED);

   return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
IopSendEject(IN PDEVICE_OBJECT DeviceObject)
{
    IO_STACK_LOCATION Stack;
    PVOID Dummy;

    RtlZeroMemory(&Stack, sizeof(IO_STACK_LOCATION));
    Stack.MajorFunction = IRP_MJ_PNP;
    Stack.MinorFunction = IRP_MN_EJECT;

    return IopSynchronousCall(DeviceObject, &Stack, &Dummy);
}

static
VOID
NTAPI
IopSendSurpriseRemoval(IN PDEVICE_OBJECT DeviceObject)
{
    IO_STACK_LOCATION Stack;
    PVOID Dummy;

    RtlZeroMemory(&Stack, sizeof(IO_STACK_LOCATION));
    Stack.MajorFunction = IRP_MJ_PNP;
    Stack.MinorFunction = IRP_MN_SURPRISE_REMOVAL;

    /* Drivers should never fail a IRP_MN_SURPRISE_REMOVAL request */
    IopSynchronousCall(DeviceObject, &Stack, &Dummy);
}

static
NTSTATUS
NTAPI
IopQueryRemoveDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(DeviceObject);
    IO_STACK_LOCATION Stack;
    PVOID Dummy;
    NTSTATUS Status;

    ASSERT(DeviceNode);

    IopQueueTargetDeviceEvent(&GUID_DEVICE_REMOVE_PENDING,
                              &DeviceNode->InstancePath);

    RtlZeroMemory(&Stack, sizeof(IO_STACK_LOCATION));
    Stack.MajorFunction = IRP_MJ_PNP;
    Stack.MinorFunction = IRP_MN_QUERY_REMOVE_DEVICE;

    Status = IopSynchronousCall(DeviceObject, &Stack, &Dummy);

    IopNotifyPlugPlayNotification(DeviceObject,
                                  EventCategoryTargetDeviceChange,
                                  &GUID_TARGET_DEVICE_QUERY_REMOVE,
                                  NULL,
                                  NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Removal vetoed by %wZ\n", &DeviceNode->InstancePath);
        IopQueueTargetDeviceEvent(&GUID_DEVICE_REMOVAL_VETOED,
                                  &DeviceNode->InstancePath);
    }

    return Status;
}

static
NTSTATUS
NTAPI
IopQueryStopDevice(IN PDEVICE_OBJECT DeviceObject)
{
    IO_STACK_LOCATION Stack;
    PVOID Dummy;

    RtlZeroMemory(&Stack, sizeof(IO_STACK_LOCATION));
    Stack.MajorFunction = IRP_MJ_PNP;
    Stack.MinorFunction = IRP_MN_QUERY_STOP_DEVICE;

    return IopSynchronousCall(DeviceObject, &Stack, &Dummy);
}

static
VOID
NTAPI
IopSendRemoveDevice(IN PDEVICE_OBJECT DeviceObject)
{
    IO_STACK_LOCATION Stack;
    PVOID Dummy;
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(DeviceObject);

    /* Drop all our state for this device in case it isn't really going away */
    DeviceNode->Flags &= DNF_ENUMERATED | DNF_PROCESSED;

    RtlZeroMemory(&Stack, sizeof(IO_STACK_LOCATION));
    Stack.MajorFunction = IRP_MJ_PNP;
    Stack.MinorFunction = IRP_MN_REMOVE_DEVICE;

    /* Drivers should never fail a IRP_MN_REMOVE_DEVICE request */
    IopSynchronousCall(DeviceObject, &Stack, &Dummy);

    IopNotifyPlugPlayNotification(DeviceObject,
                                  EventCategoryTargetDeviceChange,
                                  &GUID_TARGET_DEVICE_REMOVE_COMPLETE,
                                  NULL,
                                  NULL);
    ObDereferenceObject(DeviceObject);
}

static
VOID
NTAPI
IopCancelRemoveDevice(IN PDEVICE_OBJECT DeviceObject)
{
    IO_STACK_LOCATION Stack;
    PVOID Dummy;

    RtlZeroMemory(&Stack, sizeof(IO_STACK_LOCATION));
    Stack.MajorFunction = IRP_MJ_PNP;
    Stack.MinorFunction = IRP_MN_CANCEL_REMOVE_DEVICE;

    /* Drivers should never fail a IRP_MN_CANCEL_REMOVE_DEVICE request */
    IopSynchronousCall(DeviceObject, &Stack, &Dummy);

    IopNotifyPlugPlayNotification(DeviceObject,
                                  EventCategoryTargetDeviceChange,
                                  &GUID_TARGET_DEVICE_REMOVE_CANCELLED,
                                  NULL,
                                  NULL);
}

static
VOID
NTAPI
IopSendStopDevice(IN PDEVICE_OBJECT DeviceObject)
{
    IO_STACK_LOCATION Stack;
    PVOID Dummy;

    RtlZeroMemory(&Stack, sizeof(IO_STACK_LOCATION));
    Stack.MajorFunction = IRP_MJ_PNP;
    Stack.MinorFunction = IRP_MN_STOP_DEVICE;

    /* Drivers should never fail a IRP_MN_STOP_DEVICE request */
    IopSynchronousCall(DeviceObject, &Stack, &Dummy);
}

VOID
NTAPI
IopStartDevice2(IN PDEVICE_OBJECT DeviceObject)
{
    IO_STACK_LOCATION Stack;
    PDEVICE_NODE DeviceNode;
    NTSTATUS Status;
    PVOID Dummy;
    DEVICE_CAPABILITIES DeviceCapabilities;

    /* Get the device node */
    DeviceNode = IopGetDeviceNode(DeviceObject);

    ASSERT(!(DeviceNode->Flags & DNF_DISABLED));

    /* Build the I/O stack location */
    RtlZeroMemory(&Stack, sizeof(IO_STACK_LOCATION));
    Stack.MajorFunction = IRP_MJ_PNP;
    Stack.MinorFunction = IRP_MN_START_DEVICE;

    Stack.Parameters.StartDevice.AllocatedResources =
         DeviceNode->ResourceList;
    Stack.Parameters.StartDevice.AllocatedResourcesTranslated =
         DeviceNode->ResourceListTranslated;

    /* Do the call */
    Status = IopSynchronousCall(DeviceObject, &Stack, &Dummy);
    if (!NT_SUCCESS(Status))
    {
        /* Send an IRP_MN_REMOVE_DEVICE request */
        IopRemoveDevice(DeviceNode);

        /* Set the appropriate flag */
        DeviceNode->Flags |= DNF_START_FAILED;
        DeviceNode->Problem = CM_PROB_FAILED_START;

        DPRINT1("Warning: PnP Start failed (%wZ) [Status: 0x%x]\n", &DeviceNode->InstancePath, Status);
        return;
    }

    DPRINT("Sending IRP_MN_QUERY_CAPABILITIES to device stack (after start)\n");

    Status = IopQueryDeviceCapabilities(DeviceNode, &DeviceCapabilities);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopInitiatePnpIrp() failed (Status 0x%08lx)\n", Status);
    }

    /* Invalidate device state so IRP_MN_QUERY_PNP_DEVICE_STATE is sent */
    IoInvalidateDeviceState(DeviceObject);

    /* Otherwise, mark us as started */
    DeviceNode->Flags |= DNF_STARTED;
    DeviceNode->Flags &= ~DNF_STOPPED;

    /* We now need enumeration */
    DeviceNode->Flags |= DNF_NEED_ENUMERATION_ONLY;
}

NTSTATUS
NTAPI
IopStartAndEnumerateDevice(IN PDEVICE_NODE DeviceNode)
{
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    PAGED_CODE();

    /* Sanity check */
    ASSERT((DeviceNode->Flags & DNF_ADDED));
    ASSERT((DeviceNode->Flags & (DNF_RESOURCE_ASSIGNED |
                                 DNF_RESOURCE_REPORTED |
                                 DNF_NO_RESOURCE_REQUIRED)));

    /* Get the device object */
    DeviceObject = DeviceNode->PhysicalDeviceObject;

    /* Check if we're not started yet */
    if (!(DeviceNode->Flags & DNF_STARTED))
    {
        /* Start us */
        IopStartDevice2(DeviceObject);
    }

    /* Do we need to query IDs? This happens in the case of manual reporting */
#if 0
    if (DeviceNode->Flags & DNF_NEED_QUERY_IDS)
    {
        DPRINT1("Warning: Device node has DNF_NEED_QUERY_IDS\n");
        /* And that case shouldn't happen yet */
        ASSERT(FALSE);
    }
#endif

    /* Make sure we're started, and check if we need enumeration */
    if ((DeviceNode->Flags & DNF_STARTED) &&
        (DeviceNode->Flags & DNF_NEED_ENUMERATION_ONLY))
    {
        /* Enumerate us */
        IoSynchronousInvalidateDeviceRelations(DeviceObject, BusRelations);
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* Nothing to do */
        Status = STATUS_SUCCESS;
    }

    /* Return */
    return Status;
}

NTSTATUS
IopStopDevice(
   PDEVICE_NODE DeviceNode)
{
   NTSTATUS Status;

   DPRINT("Stopping device: %wZ\n", &DeviceNode->InstancePath);

   Status = IopQueryStopDevice(DeviceNode->PhysicalDeviceObject);
   if (NT_SUCCESS(Status))
   {
       IopSendStopDevice(DeviceNode->PhysicalDeviceObject);

       DeviceNode->Flags &= ~(DNF_STARTED | DNF_START_REQUEST_PENDING);
       DeviceNode->Flags |= DNF_STOPPED;

       return STATUS_SUCCESS;
   }

   return Status;
}

NTSTATUS
IopStartDevice(
   PDEVICE_NODE DeviceNode)
{
   NTSTATUS Status;
   HANDLE InstanceHandle = NULL, ControlHandle = NULL;
   UNICODE_STRING KeyName;
   OBJECT_ATTRIBUTES ObjectAttributes;

   if (DeviceNode->Flags & DNF_DISABLED)
       return STATUS_SUCCESS;

   Status = IopAssignDeviceResources(DeviceNode);
   if (!NT_SUCCESS(Status))
       goto ByeBye;

   /* New PnP ABI */
   IopStartAndEnumerateDevice(DeviceNode);

   /* FIX: Should be done in new device instance code */
   Status = IopCreateDeviceKeyPath(&DeviceNode->InstancePath, REG_OPTION_NON_VOLATILE, &InstanceHandle);
   if (!NT_SUCCESS(Status))
       goto ByeBye;

   /* FIX: Should be done in IoXxxPrepareDriverLoading */
   // {
   RtlInitUnicodeString(&KeyName, L"Control");
   InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                              InstanceHandle,
                              NULL);
   Status = ZwCreateKey(&ControlHandle,
                        KEY_SET_VALUE,
                        &ObjectAttributes,
                        0,
                        NULL,
                        REG_OPTION_VOLATILE,
                        NULL);
   if (!NT_SUCCESS(Status))
       goto ByeBye;

   RtlInitUnicodeString(&KeyName, L"ActiveService");
   Status = ZwSetValueKey(ControlHandle, &KeyName, 0, REG_SZ, DeviceNode->ServiceName.Buffer, DeviceNode->ServiceName.Length);
   // }

ByeBye:
   if (ControlHandle != NULL)
       ZwClose(ControlHandle);

   if (InstanceHandle != NULL)
       ZwClose(InstanceHandle);

   return Status;
}

NTSTATUS
NTAPI
IopQueryDeviceCapabilities(PDEVICE_NODE DeviceNode,
                           PDEVICE_CAPABILITIES DeviceCaps)
{
   IO_STATUS_BLOCK StatusBlock;
   IO_STACK_LOCATION Stack;
   NTSTATUS Status;
   HANDLE InstanceKey;
   UNICODE_STRING ValueName;

   /* Set up the Header */
   RtlZeroMemory(DeviceCaps, sizeof(DEVICE_CAPABILITIES));
   DeviceCaps->Size = sizeof(DEVICE_CAPABILITIES);
   DeviceCaps->Version = 1;
   DeviceCaps->Address = -1;
   DeviceCaps->UINumber = -1;

   /* Set up the Stack */
   RtlZeroMemory(&Stack, sizeof(IO_STACK_LOCATION));
   Stack.Parameters.DeviceCapabilities.Capabilities = DeviceCaps;

   /* Send the IRP */
   Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                              &StatusBlock,
                              IRP_MN_QUERY_CAPABILITIES,
                              &Stack);
   if (!NT_SUCCESS(Status))
   {
       if (Status != STATUS_NOT_SUPPORTED)
       {
          DPRINT1("IRP_MN_QUERY_CAPABILITIES failed with status 0x%lx\n", Status);
       }
       return Status;
   }

   DeviceNode->CapabilityFlags = *(PULONG)((ULONG_PTR)&DeviceCaps->Version + sizeof(DeviceCaps->Version));

   if (DeviceCaps->NoDisplayInUI)
       DeviceNode->UserFlags |= DNUF_DONT_SHOW_IN_UI;
   else
       DeviceNode->UserFlags &= ~DNUF_DONT_SHOW_IN_UI;

   Status = IopCreateDeviceKeyPath(&DeviceNode->InstancePath, REG_OPTION_NON_VOLATILE, &InstanceKey);
   if (NT_SUCCESS(Status))
   {
      /* Set 'Capabilities' value */
      RtlInitUnicodeString(&ValueName, L"Capabilities");
      Status = ZwSetValueKey(InstanceKey,
                             &ValueName,
                             0,
                             REG_DWORD,
                             &DeviceNode->CapabilityFlags,
                             sizeof(ULONG));

      /* Set 'UINumber' value */
      if (DeviceCaps->UINumber != MAXULONG)
      {
         RtlInitUnicodeString(&ValueName, L"UINumber");
         Status = ZwSetValueKey(InstanceKey,
                                &ValueName,
                                0,
                                REG_DWORD,
                                &DeviceCaps->UINumber,
                                sizeof(ULONG));
      }

      ZwClose(InstanceKey);
   }

   return Status;
}

static
VOID
NTAPI
IopDeviceRelationsWorker(
    _In_ PVOID Context)
{
    PLIST_ENTRY ListEntry;
    PINVALIDATE_DEVICE_RELATION_DATA Data;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IopDeviceRelationsSpinLock, &OldIrql);
    while (!IsListEmpty(&IopDeviceRelationsRequestList))
    {
        ListEntry = RemoveHeadList(&IopDeviceRelationsRequestList);
        KeReleaseSpinLock(&IopDeviceRelationsSpinLock, OldIrql);
        Data = CONTAINING_RECORD(ListEntry,
                                 INVALIDATE_DEVICE_RELATION_DATA,
                                 RequestListEntry);

        IoSynchronousInvalidateDeviceRelations(Data->DeviceObject,
                                               Data->Type);

        ObDereferenceObject(Data->DeviceObject);
        ExFreePool(Data);
        KeAcquireSpinLock(&IopDeviceRelationsSpinLock, &OldIrql);
    }
    IopDeviceRelationsRequestInProgress = FALSE;
    KeReleaseSpinLock(&IopDeviceRelationsSpinLock, OldIrql);
}

NTSTATUS
IopGetSystemPowerDeviceObject(PDEVICE_OBJECT *DeviceObject)
{
   KIRQL OldIrql;

   if (PopSystemPowerDeviceNode)
   {
      KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
      *DeviceObject = PopSystemPowerDeviceNode->PhysicalDeviceObject;
      KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

      return STATUS_SUCCESS;
   }

   return STATUS_UNSUCCESSFUL;
}

USHORT
NTAPI
IopGetBusTypeGuidIndex(LPGUID BusTypeGuid)
{
   USHORT i = 0, FoundIndex = 0xFFFF;
   ULONG NewSize;
   PVOID NewList;

   /* Acquire the lock */
   ExAcquireFastMutex(&PnpBusTypeGuidList->Lock);

   /* Loop all entries */
   while (i < PnpBusTypeGuidList->GuidCount)
   {
       /* Try to find a match */
       if (RtlCompareMemory(BusTypeGuid,
                            &PnpBusTypeGuidList->Guids[i],
                            sizeof(GUID)) == sizeof(GUID))
       {
           /* Found it */
           FoundIndex = i;
           goto Quickie;
       }
       i++;
   }

   /* Check if we have to grow the list */
   if (PnpBusTypeGuidList->GuidCount)
   {
       /* Calculate the new size */
       NewSize = sizeof(IO_BUS_TYPE_GUID_LIST) +
                (sizeof(GUID) * PnpBusTypeGuidList->GuidCount);

       /* Allocate the new copy */
       NewList = ExAllocatePool(PagedPool, NewSize);

       if (!NewList) {
       /* Fail */
       ExFreePool(PnpBusTypeGuidList);
       goto Quickie;
       }

       /* Now copy them, decrease the size too */
       NewSize -= sizeof(GUID);
       RtlCopyMemory(NewList, PnpBusTypeGuidList, NewSize);

       /* Free the old list */
       ExFreePool(PnpBusTypeGuidList);

       /* Use the new buffer */
       PnpBusTypeGuidList = NewList;
   }

   /* Copy the new GUID */
   RtlCopyMemory(&PnpBusTypeGuidList->Guids[PnpBusTypeGuidList->GuidCount],
                 BusTypeGuid,
                 sizeof(GUID));

   /* The new entry is the index */
   FoundIndex = (USHORT)PnpBusTypeGuidList->GuidCount;
   PnpBusTypeGuidList->GuidCount++;

Quickie:
   ExReleaseFastMutex(&PnpBusTypeGuidList->Lock);
   return FoundIndex;
}

/*
 * DESCRIPTION
 *     Creates a device node
 *
 * ARGUMENTS
 *   ParentNode           = Pointer to parent device node
 *   PhysicalDeviceObject = Pointer to PDO for device object. Pass NULL
 *                          to have the root device node create one
 *                          (eg. for legacy drivers)
 *   DeviceNode           = Pointer to storage for created device node
 *
 * RETURN VALUE
 *     Status
 */
NTSTATUS
IopCreateDeviceNode(PDEVICE_NODE ParentNode,
                    PDEVICE_OBJECT PhysicalDeviceObject,
                    PUNICODE_STRING ServiceName,
                    PDEVICE_NODE *DeviceNode)
{
   PDEVICE_NODE Node;
   NTSTATUS Status;
   KIRQL OldIrql;
   UNICODE_STRING FullServiceName;
   UNICODE_STRING LegacyPrefix = RTL_CONSTANT_STRING(L"LEGACY_");
   UNICODE_STRING UnknownDeviceName = RTL_CONSTANT_STRING(L"UNKNOWN");
   UNICODE_STRING KeyName, ClassName;
   PUNICODE_STRING ServiceName1;
   ULONG LegacyValue;
   UNICODE_STRING ClassGUID;
   HANDLE InstanceHandle;

   DPRINT("ParentNode 0x%p PhysicalDeviceObject 0x%p ServiceName %wZ\n",
      ParentNode, PhysicalDeviceObject, ServiceName);

   Node = ExAllocatePoolWithTag(NonPagedPool, sizeof(DEVICE_NODE), TAG_IO_DEVNODE);
   if (!Node)
   {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlZeroMemory(Node, sizeof(DEVICE_NODE));

   if (!ServiceName)
       ServiceName1 = &UnknownDeviceName;
   else
       ServiceName1 = ServiceName;

   if (!PhysicalDeviceObject)
   {
      FullServiceName.MaximumLength = LegacyPrefix.Length + ServiceName1->Length;
      FullServiceName.Length = 0;
      FullServiceName.Buffer = ExAllocatePool(PagedPool, FullServiceName.MaximumLength);
      if (!FullServiceName.Buffer)
      {
          ExFreePoolWithTag(Node, TAG_IO_DEVNODE);
          return STATUS_INSUFFICIENT_RESOURCES;
      }

      RtlAppendUnicodeStringToString(&FullServiceName, &LegacyPrefix);
      RtlAppendUnicodeStringToString(&FullServiceName, ServiceName1);

      Status = PnpRootCreateDevice(&FullServiceName, NULL, &PhysicalDeviceObject, &Node->InstancePath);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("PnpRootCreateDevice() failed with status 0x%08X\n", Status);
         ExFreePool(FullServiceName.Buffer);
         ExFreePoolWithTag(Node, TAG_IO_DEVNODE);
         return Status;
      }

      /* Create the device key for legacy drivers */
      Status = IopCreateDeviceKeyPath(&Node->InstancePath, REG_OPTION_VOLATILE, &InstanceHandle);
      if (!NT_SUCCESS(Status))
      {
          ExFreePool(FullServiceName.Buffer);
          ExFreePoolWithTag(Node, TAG_IO_DEVNODE);
          return Status;
      }

      Node->ServiceName.Buffer = ExAllocatePool(PagedPool, ServiceName1->Length);
      if (!Node->ServiceName.Buffer)
      {
          ZwClose(InstanceHandle);
          ExFreePool(FullServiceName.Buffer);
          ExFreePoolWithTag(Node, TAG_IO_DEVNODE);
          return Status;
      }

      Node->ServiceName.MaximumLength = ServiceName1->Length;
      Node->ServiceName.Length = 0;

      RtlAppendUnicodeStringToString(&Node->ServiceName, ServiceName1);

      if (ServiceName)
      {
          RtlInitUnicodeString(&KeyName, L"Service");
          Status = ZwSetValueKey(InstanceHandle, &KeyName, 0, REG_SZ, ServiceName->Buffer, ServiceName->Length);
      }

      if (NT_SUCCESS(Status))
      {
          RtlInitUnicodeString(&KeyName, L"Legacy");

          LegacyValue = 1;
          Status = ZwSetValueKey(InstanceHandle, &KeyName, 0, REG_DWORD, &LegacyValue, sizeof(LegacyValue));
          if (NT_SUCCESS(Status))
          {
              RtlInitUnicodeString(&KeyName, L"Class");

              RtlInitUnicodeString(&ClassName, L"LegacyDriver\0");
              Status = ZwSetValueKey(InstanceHandle, &KeyName, 0, REG_SZ, ClassName.Buffer, ClassName.Length + sizeof(UNICODE_NULL));
              if (NT_SUCCESS(Status))
              {
                  RtlInitUnicodeString(&KeyName, L"ClassGUID");

                  RtlInitUnicodeString(&ClassGUID, L"{8ECC055D-047F-11D1-A537-0000F8753ED1}\0");
                  Status = ZwSetValueKey(InstanceHandle, &KeyName, 0, REG_SZ, ClassGUID.Buffer, ClassGUID.Length + sizeof(UNICODE_NULL));
                  if (NT_SUCCESS(Status))
                  {
                      RtlInitUnicodeString(&KeyName, L"DeviceDesc");

                      Status = ZwSetValueKey(InstanceHandle, &KeyName, 0, REG_SZ, ServiceName1->Buffer, ServiceName1->Length + sizeof(UNICODE_NULL));
                  }
              }
          }
      }

      ZwClose(InstanceHandle);
      ExFreePool(FullServiceName.Buffer);

      if (!NT_SUCCESS(Status))
      {
          ExFreePool(Node->ServiceName.Buffer);
          ExFreePoolWithTag(Node, TAG_IO_DEVNODE);
          return Status;
      }

      IopDeviceNodeSetFlag(Node, DNF_LEGACY_DRIVER);
      IopDeviceNodeSetFlag(Node, DNF_PROCESSED);
      IopDeviceNodeSetFlag(Node, DNF_ADDED);
      IopDeviceNodeSetFlag(Node, DNF_STARTED);
   }

   Node->PhysicalDeviceObject = PhysicalDeviceObject;

   ((PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode = Node;

    if (ParentNode)
    {
        KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
        Node->Parent = ParentNode;
        Node->Sibling = NULL;
        if (ParentNode->LastChild == NULL)
        {
            ParentNode->Child = Node;
            ParentNode->LastChild = Node;
        }
        else
        {
            ParentNode->LastChild->Sibling = Node;
            ParentNode->LastChild = Node;
        }
        KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);
        Node->Level = ParentNode->Level + 1;
    }

    PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

   *DeviceNode = Node;

   return STATUS_SUCCESS;
}

NTSTATUS
IopFreeDeviceNode(PDEVICE_NODE DeviceNode)
{
   KIRQL OldIrql;
   PDEVICE_NODE PrevSibling = NULL;

   /* All children must be deleted before a parent is deleted */
   ASSERT(!DeviceNode->Child);
   ASSERT(DeviceNode->PhysicalDeviceObject);

   KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);

    /* Get previous sibling */
    if (DeviceNode->Parent && DeviceNode->Parent->Child != DeviceNode)
    {
        PrevSibling = DeviceNode->Parent->Child;
        while (PrevSibling->Sibling != DeviceNode)
            PrevSibling = PrevSibling->Sibling;
    }

    /* Unlink from parent if it exists */
    if (DeviceNode->Parent)
    {
        if (DeviceNode->Parent->LastChild == DeviceNode)
        {
            DeviceNode->Parent->LastChild = PrevSibling;
            if (PrevSibling)
                PrevSibling->Sibling = NULL;
        }
        if (DeviceNode->Parent->Child == DeviceNode)
            DeviceNode->Parent->Child = DeviceNode->Sibling;
    }

    /* Unlink from sibling list */
    if (PrevSibling)
        PrevSibling->Sibling = DeviceNode->Sibling;

   KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

   RtlFreeUnicodeString(&DeviceNode->InstancePath);

   RtlFreeUnicodeString(&DeviceNode->ServiceName);

   if (DeviceNode->ResourceList)
   {
      ExFreePool(DeviceNode->ResourceList);
   }

   if (DeviceNode->ResourceListTranslated)
   {
      ExFreePool(DeviceNode->ResourceListTranslated);
   }

   if (DeviceNode->ResourceRequirements)
   {
      ExFreePool(DeviceNode->ResourceRequirements);
   }

   if (DeviceNode->BootResources)
   {
      ExFreePool(DeviceNode->BootResources);
   }

   ((PEXTENDED_DEVOBJ_EXTENSION)DeviceNode->PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode = NULL;
   ExFreePoolWithTag(DeviceNode, TAG_IO_DEVNODE);

   return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IopSynchronousCall(IN PDEVICE_OBJECT DeviceObject,
                   IN PIO_STACK_LOCATION IoStackLocation,
                   OUT PVOID *Information)
{
    PIRP Irp;
    PIO_STACK_LOCATION IrpStack;
    IO_STATUS_BLOCK IoStatusBlock;
    KEVENT Event;
    NTSTATUS Status;
    PDEVICE_OBJECT TopDeviceObject;
    PAGED_CODE();

    /* Call the top of the device stack */
    TopDeviceObject = IoGetAttachedDeviceReference(DeviceObject);

    /* Allocate an IRP */
    Irp = IoAllocateIrp(TopDeviceObject->StackSize, FALSE);
    if (!Irp) return STATUS_INSUFFICIENT_RESOURCES;

    /* Initialize to failure */
    Irp->IoStatus.Status = IoStatusBlock.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = IoStatusBlock.Information = 0;

    /* Special case for IRP_MN_FILTER_RESOURCE_REQUIREMENTS */
    if (IoStackLocation->MinorFunction == IRP_MN_FILTER_RESOURCE_REQUIREMENTS)
    {
        /* Copy the resource requirements list into the IOSB */
        Irp->IoStatus.Information =
        IoStatusBlock.Information = (ULONG_PTR)IoStackLocation->Parameters.FilterResourceRequirements.IoResourceRequirementList;
    }

    /* Initialize the event */
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    /* Set them up */
    Irp->UserIosb = &IoStatusBlock;
    Irp->UserEvent = &Event;

    /* Queue the IRP */
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    IoQueueThreadIrp(Irp);

    /* Copy-in the stack */
    IrpStack = IoGetNextIrpStackLocation(Irp);
    *IrpStack = *IoStackLocation;

    /* Call the driver */
    Status = IoCallDriver(TopDeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for it */
        KeWaitForSingleObject(&Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatusBlock.Status;
    }

    /* Remove the reference */
    ObDereferenceObject(TopDeviceObject);

    /* Return the information */
    *Information = (PVOID)IoStatusBlock.Information;
    return Status;
}

NTSTATUS
NTAPI
IopInitiatePnpIrp(IN PDEVICE_OBJECT DeviceObject,
                  IN OUT PIO_STATUS_BLOCK IoStatusBlock,
                  IN UCHAR MinorFunction,
                  IN PIO_STACK_LOCATION Stack OPTIONAL)
{
    IO_STACK_LOCATION IoStackLocation;

    /* Fill out the stack information */
    RtlZeroMemory(&IoStackLocation, sizeof(IO_STACK_LOCATION));
    IoStackLocation.MajorFunction = IRP_MJ_PNP;
    IoStackLocation.MinorFunction = MinorFunction;
    if (Stack)
    {
        /* Copy the rest */
        RtlCopyMemory(&IoStackLocation.Parameters,
                      &Stack->Parameters,
                      sizeof(Stack->Parameters));
    }

    /* Do the PnP call */
    IoStatusBlock->Status = IopSynchronousCall(DeviceObject,
                                               &IoStackLocation,
                                               (PVOID)&IoStatusBlock->Information);
    return IoStatusBlock->Status;
}

NTSTATUS
IopTraverseDeviceTreeNode(PDEVICETREE_TRAVERSE_CONTEXT Context)
{
   PDEVICE_NODE ParentDeviceNode;
   PDEVICE_NODE ChildDeviceNode;
   NTSTATUS Status;

   /* Copy context data so we don't overwrite it in subsequent calls to this function */
   ParentDeviceNode = Context->DeviceNode;

   /* Call the action routine */
   Status = (Context->Action)(ParentDeviceNode, Context->Context);
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }

   /* Traversal of all children nodes */
   for (ChildDeviceNode = ParentDeviceNode->Child;
        ChildDeviceNode != NULL;
        ChildDeviceNode = ChildDeviceNode->Sibling)
   {
      /* Pass the current device node to the action routine */
      Context->DeviceNode = ChildDeviceNode;

      Status = IopTraverseDeviceTreeNode(Context);
      if (!NT_SUCCESS(Status))
      {
         return Status;
      }
   }

   return Status;
}


NTSTATUS
IopTraverseDeviceTree(PDEVICETREE_TRAVERSE_CONTEXT Context)
{
   NTSTATUS Status;

   DPRINT("Context 0x%p\n", Context);

   DPRINT("IopTraverseDeviceTree(DeviceNode 0x%p  FirstDeviceNode 0x%p  Action %p  Context 0x%p)\n",
      Context->DeviceNode, Context->FirstDeviceNode, Context->Action, Context->Context);

   /* Start from the specified device node */
   Context->DeviceNode = Context->FirstDeviceNode;

   /* Recursively traverse the device tree */
   Status = IopTraverseDeviceTreeNode(Context);
   if (Status == STATUS_UNSUCCESSFUL)
   {
      /* The action routine just wanted to terminate the traversal with status
      code STATUS_SUCCESS */
      Status = STATUS_SUCCESS;
   }

   return Status;
}


/*
 * IopCreateDeviceKeyPath
 *
 * Creates a registry key
 *
 * Parameters
 *    RegistryPath
 *        Name of the key to be created.
 *    Handle
 *        Handle to the newly created key
 *
 * Remarks
 *     This method can create nested trees, so parent of RegistryPath can
 *     be not existant, and will be created if needed.
 */
NTSTATUS
NTAPI
IopCreateDeviceKeyPath(IN PCUNICODE_STRING RegistryPath,
                       IN ULONG CreateOptions,
                       OUT PHANDLE Handle)
{
    UNICODE_STRING EnumU = RTL_CONSTANT_STRING(ENUM_ROOT);
    HANDLE hParent = NULL, hKey;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    PCWSTR Current, Last;
    USHORT Length;
    NTSTATUS Status;

    /* Assume failure */
    *Handle = NULL;

    /* Open root key for device instances */
    Status = IopOpenRegistryKeyEx(&hParent, NULL, &EnumU, KEY_CREATE_SUB_KEY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwOpenKey('%wZ') failed with status 0x%08lx\n", &EnumU, Status);
        return Status;
    }

    Current = KeyName.Buffer = RegistryPath->Buffer;
    Last = &RegistryPath->Buffer[RegistryPath->Length / sizeof(WCHAR)];

    /* Go up to the end of the string */
    while (Current <= Last)
    {
        if (Current != Last && *Current != L'\\')
        {
            /* Not the end of the string and not a separator */
            Current++;
            continue;
        }

        /* Prepare relative key name */
        Length = (USHORT)((ULONG_PTR)Current - (ULONG_PTR)KeyName.Buffer);
        KeyName.MaximumLength = KeyName.Length = Length;
        DPRINT("Create '%wZ'\n", &KeyName);

        /* Open key */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   hParent,
                                   NULL);
        Status = ZwCreateKey(&hKey,
                             Current == Last ? KEY_ALL_ACCESS : KEY_CREATE_SUB_KEY,
                             &ObjectAttributes,
                             0,
                             NULL,
                             CreateOptions,
                             NULL);

        /* Close parent key handle, we don't need it anymore */
        if (hParent)
            ZwClose(hParent);

        /* Key opening/creating failed? */
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ZwCreateKey('%wZ') failed with status 0x%08lx\n", &KeyName, Status);
            return Status;
        }

        /* Check if it is the end of the string */
        if (Current == Last)
        {
            /* Yes, return success */
            *Handle = hKey;
            return STATUS_SUCCESS;
        }

        /* Start with this new parent key */
        hParent = hKey;
        Current++;
        KeyName.Buffer = (PWSTR)Current;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
IopSetDeviceInstanceData(HANDLE InstanceKey,
                         PDEVICE_NODE DeviceNode)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING KeyName;
   HANDLE LogConfKey;
   ULONG ResCount;
   ULONG ResultLength;
   NTSTATUS Status;
   HANDLE ControlHandle;

   DPRINT("IopSetDeviceInstanceData() called\n");

   /* Create the 'LogConf' key */
   RtlInitUnicodeString(&KeyName, L"LogConf");
   InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                              InstanceKey,
                              NULL);
   Status = ZwCreateKey(&LogConfKey,
                        KEY_ALL_ACCESS,
                        &ObjectAttributes,
                        0,
                        NULL,
                        // FIXME? In r53694 it was silently turned from non-volatile into this,
                        // without any extra warning. Is this still needed??
                        REG_OPTION_VOLATILE,
                        NULL);
   if (NT_SUCCESS(Status))
   {
      /* Set 'BootConfig' value */
      if (DeviceNode->BootResources != NULL)
      {
         ResCount = DeviceNode->BootResources->Count;
         if (ResCount != 0)
         {
            RtlInitUnicodeString(&KeyName, L"BootConfig");
            Status = ZwSetValueKey(LogConfKey,
                                   &KeyName,
                                   0,
                                   REG_RESOURCE_LIST,
                                   DeviceNode->BootResources,
                                   PnpDetermineResourceListSize(DeviceNode->BootResources));
         }
      }

      /* Set 'BasicConfigVector' value */
      if (DeviceNode->ResourceRequirements != NULL &&
         DeviceNode->ResourceRequirements->ListSize != 0)
      {
         RtlInitUnicodeString(&KeyName, L"BasicConfigVector");
         Status = ZwSetValueKey(LogConfKey,
                                &KeyName,
                                0,
                                REG_RESOURCE_REQUIREMENTS_LIST,
                                DeviceNode->ResourceRequirements,
                                DeviceNode->ResourceRequirements->ListSize);
      }

      ZwClose(LogConfKey);
   }

   /* Set the 'ConfigFlags' value */
   RtlInitUnicodeString(&KeyName, L"ConfigFlags");
   Status = ZwQueryValueKey(InstanceKey,
                            &KeyName,
                            KeyValueBasicInformation,
                            NULL,
                            0,
                            &ResultLength);
  if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
  {
    /* Write the default value */
    ULONG DefaultConfigFlags = 0;
    Status = ZwSetValueKey(InstanceKey,
                           &KeyName,
                           0,
                           REG_DWORD,
                           &DefaultConfigFlags,
                           sizeof(DefaultConfigFlags));
  }

   /* Create the 'Control' key */
   RtlInitUnicodeString(&KeyName, L"Control");
   InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                              InstanceKey,
                              NULL);
   Status = ZwCreateKey(&ControlHandle,
                        KEY_ALL_ACCESS,
                        &ObjectAttributes,
                        0,
                        NULL,
                        REG_OPTION_VOLATILE,
                        NULL);
   if (NT_SUCCESS(Status))
       ZwClose(ControlHandle);

  DPRINT("IopSetDeviceInstanceData() done\n");

  return Status;
}

/*
 * IopGetParentIdPrefix
 *
 * Retrieve (or create) a string which identifies a device.
 *
 * Parameters
 *    DeviceNode
 *        Pointer to device node.
 *    ParentIdPrefix
 *        Pointer to the string where is returned the parent node identifier
 *
 * Remarks
 *     If the return code is STATUS_SUCCESS, the ParentIdPrefix string is
 *     valid and its Buffer field is NULL-terminated. The caller needs to
 *     to free the string with RtlFreeUnicodeString when it is no longer
 *     needed.
 */

NTSTATUS
IopGetParentIdPrefix(PDEVICE_NODE DeviceNode,
                     PUNICODE_STRING ParentIdPrefix)
{
    const UNICODE_STRING EnumKeyPath = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\");
    ULONG KeyNameBufferLength;
    PKEY_VALUE_PARTIAL_INFORMATION ParentIdPrefixInformation = NULL;
    UNICODE_STRING KeyName = {0, 0, NULL};
    UNICODE_STRING KeyValue;
    UNICODE_STRING ValueName;
    HANDLE hKey = NULL;
    ULONG crc32;
    NTSTATUS Status;

    /* HACK: As long as some devices have a NULL device
     * instance path, the following test is required :(
     */
    if (DeviceNode->Parent->InstancePath.Length == 0)
    {
        DPRINT1("Parent of %wZ has NULL Instance path, please report!\n",
                &DeviceNode->InstancePath);
        return STATUS_UNSUCCESSFUL;
    }

    /* 1. Try to retrieve ParentIdPrefix from registry */
    KeyNameBufferLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]) + MAX_PATH * sizeof(WCHAR);
    ParentIdPrefixInformation = ExAllocatePoolWithTag(PagedPool,
                                                      KeyNameBufferLength + sizeof(UNICODE_NULL),
                                                      TAG_IO);
    if (!ParentIdPrefixInformation)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeyName.Length = 0;
    KeyName.MaximumLength = EnumKeyPath.Length +
                            DeviceNode->Parent->InstancePath.Length +
                            sizeof(UNICODE_NULL);
    KeyName.Buffer = ExAllocatePoolWithTag(PagedPool,
                                           KeyName.MaximumLength,
                                           TAG_IO);
    if (!KeyName.Buffer)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    RtlCopyUnicodeString(&KeyName, &EnumKeyPath);
    RtlAppendUnicodeStringToString(&KeyName, &DeviceNode->Parent->InstancePath);

    Status = IopOpenRegistryKeyEx(&hKey, NULL, &KeyName, KEY_QUERY_VALUE | KEY_SET_VALUE);
    if (!NT_SUCCESS(Status))
    {
        goto cleanup;
    }
    RtlInitUnicodeString(&ValueName, L"ParentIdPrefix");
    Status = ZwQueryValueKey(hKey,
                             &ValueName,
                             KeyValuePartialInformation,
                             ParentIdPrefixInformation,
                             KeyNameBufferLength,
                             &KeyNameBufferLength);
    if (NT_SUCCESS(Status))
    {
        if (ParentIdPrefixInformation->Type != REG_SZ)
        {
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            KeyValue.MaximumLength = (USHORT)ParentIdPrefixInformation->DataLength;
            KeyValue.Length = KeyValue.MaximumLength - sizeof(UNICODE_NULL);
            KeyValue.Buffer = (PWSTR)ParentIdPrefixInformation->Data;
            ASSERT(KeyValue.Buffer[KeyValue.Length / sizeof(WCHAR)] == UNICODE_NULL);
        }
        goto cleanup;
    }
    if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        /* FIXME how do we get here and why is ParentIdPrefixInformation valid? */
        KeyValue.MaximumLength = (USHORT)ParentIdPrefixInformation->DataLength;
        KeyValue.Length = KeyValue.MaximumLength - sizeof(UNICODE_NULL);
        KeyValue.Buffer = (PWSTR)ParentIdPrefixInformation->Data;
        ASSERT(KeyValue.Buffer[KeyValue.Length / sizeof(WCHAR)] == UNICODE_NULL);
        goto cleanup;
    }

    /* 2. Create the ParentIdPrefix value */
    crc32 = RtlComputeCrc32(0,
                            (PUCHAR)DeviceNode->Parent->InstancePath.Buffer,
                            DeviceNode->Parent->InstancePath.Length);

    RtlStringCbPrintfW((PWSTR)ParentIdPrefixInformation,
                       KeyNameBufferLength,
                       L"%lx&%lx",
                       DeviceNode->Parent->Level,
                       crc32);
    RtlInitUnicodeString(&KeyValue, (PWSTR)ParentIdPrefixInformation);

    /* 3. Try to write the ParentIdPrefix to registry */
    Status = ZwSetValueKey(hKey,
                           &ValueName,
                           0,
                           REG_SZ,
                           KeyValue.Buffer,
                           ((ULONG)wcslen(KeyValue.Buffer) + 1) * sizeof(WCHAR));

cleanup:
    if (NT_SUCCESS(Status))
    {
       /* Duplicate the string to return it */
       Status = RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                                          &KeyValue,
                                          ParentIdPrefix);
    }
    ExFreePoolWithTag(ParentIdPrefixInformation, TAG_IO);
    RtlFreeUnicodeString(&KeyName);
    if (hKey != NULL)
    {
        ZwClose(hKey);
    }
    return Status;
}

NTSTATUS
IopQueryHardwareIds(PDEVICE_NODE DeviceNode,
                    HANDLE InstanceKey)
{
   IO_STACK_LOCATION Stack;
   IO_STATUS_BLOCK IoStatusBlock;
   PWSTR Ptr;
   UNICODE_STRING ValueName;
   NTSTATUS Status;
   ULONG Length, TotalLength;

   DPRINT("Sending IRP_MN_QUERY_ID.BusQueryHardwareIDs to device stack\n");

   RtlZeroMemory(&Stack, sizeof(Stack));
   Stack.Parameters.QueryId.IdType = BusQueryHardwareIDs;
   Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                              &IoStatusBlock,
                              IRP_MN_QUERY_ID,
                              &Stack);
   if (NT_SUCCESS(Status))
   {
      /*
       * FIXME: Check for valid characters, if there is invalid characters
       * then bugcheck.
       */
      TotalLength = 0;
      Ptr = (PWSTR)IoStatusBlock.Information;
      DPRINT("Hardware IDs:\n");
      while (*Ptr)
      {
         DPRINT("  %S\n", Ptr);
         Length = (ULONG)wcslen(Ptr) + 1;

         Ptr += Length;
         TotalLength += Length;
      }
      DPRINT("TotalLength: %hu\n", TotalLength);
      DPRINT("\n");

      RtlInitUnicodeString(&ValueName, L"HardwareID");
      Status = ZwSetValueKey(InstanceKey,
                 &ValueName,
                 0,
                 REG_MULTI_SZ,
                 (PVOID)IoStatusBlock.Information,
                 (TotalLength + 1) * sizeof(WCHAR));
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("ZwSetValueKey() failed (Status %lx)\n", Status);
      }
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %x)\n", Status);
   }

   return Status;
}

NTSTATUS
IopQueryCompatibleIds(PDEVICE_NODE DeviceNode,
                      HANDLE InstanceKey)
{
   IO_STACK_LOCATION Stack;
   IO_STATUS_BLOCK IoStatusBlock;
   PWSTR Ptr;
   UNICODE_STRING ValueName;
   NTSTATUS Status;
   ULONG Length, TotalLength;

   DPRINT("Sending IRP_MN_QUERY_ID.BusQueryCompatibleIDs to device stack\n");

   RtlZeroMemory(&Stack, sizeof(Stack));
   Stack.Parameters.QueryId.IdType = BusQueryCompatibleIDs;
   Status = IopInitiatePnpIrp(
      DeviceNode->PhysicalDeviceObject,
      &IoStatusBlock,
      IRP_MN_QUERY_ID,
      &Stack);
   if (NT_SUCCESS(Status) && IoStatusBlock.Information)
   {
      /*
      * FIXME: Check for valid characters, if there is invalid characters
      * then bugcheck.
      */
      TotalLength = 0;
      Ptr = (PWSTR)IoStatusBlock.Information;
      DPRINT("Compatible IDs:\n");
      while (*Ptr)
      {
         DPRINT("  %S\n", Ptr);
         Length = (ULONG)wcslen(Ptr) + 1;

         Ptr += Length;
         TotalLength += Length;
      }
      DPRINT("TotalLength: %hu\n", TotalLength);
      DPRINT("\n");

      RtlInitUnicodeString(&ValueName, L"CompatibleIDs");
      Status = ZwSetValueKey(InstanceKey,
                             &ValueName,
                             0,
                             REG_MULTI_SZ,
                             (PVOID)IoStatusBlock.Information,
                             (TotalLength + 1) * sizeof(WCHAR));
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("ZwSetValueKey() failed (Status %lx) or no Compatible ID returned\n", Status);
      }
   }
   else
   {
      DPRINT("IopInitiatePnpIrp() failed (Status %x)\n", Status);
   }

   return Status;
}

NTSTATUS
IopCreateDeviceInstancePath(
    _In_ PDEVICE_NODE DeviceNode,
    _Out_ PUNICODE_STRING InstancePath)
{
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING DeviceId;
    UNICODE_STRING InstanceId;
    IO_STACK_LOCATION Stack;
    NTSTATUS Status;
    UNICODE_STRING ParentIdPrefix = { 0, 0, NULL };
    DEVICE_CAPABILITIES DeviceCapabilities;

    DPRINT("Sending IRP_MN_QUERY_ID.BusQueryDeviceID to device stack\n");

    Stack.Parameters.QueryId.IdType = BusQueryDeviceID;
    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_ID,
                               &Stack);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopInitiatePnpIrp(BusQueryDeviceID) failed (Status %x)\n", Status);
        return Status;
    }

    /* Save the device id string */
    RtlInitUnicodeString(&DeviceId, (PWSTR)IoStatusBlock.Information);

    /*
     * FIXME: Check for valid characters, if there is invalid characters
     * then bugcheck.
     */

    DPRINT("Sending IRP_MN_QUERY_CAPABILITIES to device stack (after enumeration)\n");

    Status = IopQueryDeviceCapabilities(DeviceNode, &DeviceCapabilities);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopQueryDeviceCapabilities() failed (Status 0x%08lx)\n", Status);
        RtlFreeUnicodeString(&DeviceId);
        return Status;
    }

    /* This bit is only check after enumeration */
    if (DeviceCapabilities.HardwareDisabled)
    {
        /* FIXME: Cleanup device */
        DeviceNode->Flags |= DNF_DISABLED;
        RtlFreeUnicodeString(&DeviceId);
        return STATUS_PLUGPLAY_NO_DEVICE;
    }
    else
    {
        DeviceNode->Flags &= ~DNF_DISABLED;
    }

    if (!DeviceCapabilities.UniqueID)
    {
        /* Device has not a unique ID. We need to prepend parent bus unique identifier */
        DPRINT("Instance ID is not unique\n");
        Status = IopGetParentIdPrefix(DeviceNode, &ParentIdPrefix);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopGetParentIdPrefix() failed (Status 0x%08lx)\n", Status);
            RtlFreeUnicodeString(&DeviceId);
            return Status;
        }
    }

    DPRINT("Sending IRP_MN_QUERY_ID.BusQueryInstanceID to device stack\n");

    Stack.Parameters.QueryId.IdType = BusQueryInstanceID;
    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_ID,
                               &Stack);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopInitiatePnpIrp(BusQueryInstanceID) failed (Status %lx)\n", Status);
        ASSERT(IoStatusBlock.Information == 0);
    }

    RtlInitUnicodeString(&InstanceId,
                         (PWSTR)IoStatusBlock.Information);

    InstancePath->Length = 0;
    InstancePath->MaximumLength = DeviceId.Length + sizeof(WCHAR) +
                                  ParentIdPrefix.Length +
                                  InstanceId.Length +
                                  sizeof(UNICODE_NULL);
    if (ParentIdPrefix.Length && InstanceId.Length)
    {
        InstancePath->MaximumLength += sizeof(WCHAR);
    }

    InstancePath->Buffer = ExAllocatePoolWithTag(PagedPool,
                                                 InstancePath->MaximumLength,
                                                 TAG_IO);
    if (!InstancePath->Buffer)
    {
        RtlFreeUnicodeString(&InstanceId);
        RtlFreeUnicodeString(&ParentIdPrefix);
        RtlFreeUnicodeString(&DeviceId);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Start with the device id */
    RtlCopyUnicodeString(InstancePath, &DeviceId);
    RtlAppendUnicodeToString(InstancePath, L"\\");

    /* Add information from parent bus device to InstancePath */
    RtlAppendUnicodeStringToString(InstancePath, &ParentIdPrefix);
    if (ParentIdPrefix.Length && InstanceId.Length)
    {
        RtlAppendUnicodeToString(InstancePath, L"&");
    }

    /* Finally, add the id returned by the driver stack */
    RtlAppendUnicodeStringToString(InstancePath, &InstanceId);

    /*
     * FIXME: Check for valid characters, if there is invalid characters
     * then bugcheck
     */

    RtlFreeUnicodeString(&InstanceId);
    RtlFreeUnicodeString(&DeviceId);
    RtlFreeUnicodeString(&ParentIdPrefix);

    return STATUS_SUCCESS;
}


/*
 * IopActionInterrogateDeviceStack
 *
 * Retrieve information for all (direct) child nodes of a parent node.
 *
 * Parameters
 *    DeviceNode
 *       Pointer to device node.
 *    Context
 *       Pointer to parent node to retrieve child node information for.
 *
 * Remarks
 *    Any errors that occur are logged instead so that all child services have a chance
 *    of being interrogated.
 */

NTSTATUS
IopActionInterrogateDeviceStack(PDEVICE_NODE DeviceNode,
                                PVOID Context)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PWSTR DeviceDescription;
    PWSTR LocationInformation;
    PDEVICE_NODE ParentDeviceNode;
    IO_STACK_LOCATION Stack;
    NTSTATUS Status;
    ULONG RequiredLength;
    LCID LocaleId;
    HANDLE InstanceKey = NULL;
    UNICODE_STRING ValueName;
    UNICODE_STRING InstancePathU;
    PDEVICE_OBJECT OldDeviceObject;

    DPRINT("IopActionInterrogateDeviceStack(%p, %p)\n", DeviceNode, Context);
    DPRINT("PDO 0x%p\n", DeviceNode->PhysicalDeviceObject);

    ParentDeviceNode = (PDEVICE_NODE)Context;

    /*
     * We are called for the parent too, but we don't need to do special
     * handling for this node
     */
    if (DeviceNode == ParentDeviceNode)
    {
        DPRINT("Success\n");
        return STATUS_SUCCESS;
    }

    /*
     * Make sure this device node is a direct child of the parent device node
     * that is given as an argument
     */
    if (DeviceNode->Parent != ParentDeviceNode)
    {
        DPRINT("Skipping 2+ level child\n");
        return STATUS_SUCCESS;
    }

    /* Skip processing if it was already completed before */
    if (DeviceNode->Flags & DNF_PROCESSED)
    {
        /* Nothing to do */
        return STATUS_SUCCESS;
    }

    /* Get Locale ID */
    Status = ZwQueryDefaultLocale(FALSE, &LocaleId);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwQueryDefaultLocale() failed with status 0x%lx\n", Status);
        return Status;
    }

    /*
     * FIXME: For critical errors, cleanup and disable device, but always
     * return STATUS_SUCCESS.
     */

    Status = IopCreateDeviceInstancePath(DeviceNode, &InstancePathU);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_PLUGPLAY_NO_DEVICE)
        {
            DPRINT1("IopCreateDeviceInstancePath() failed with status 0x%lx\n", Status);
        }

        /* We have to return success otherwise we abort the traverse operation */
        return STATUS_SUCCESS;
    }

    /* Verify that this is not a duplicate */
    OldDeviceObject = IopGetDeviceObjectFromDeviceInstance(&InstancePathU);
    if (OldDeviceObject != NULL)
    {
        PDEVICE_NODE OldDeviceNode = IopGetDeviceNode(OldDeviceObject);

        DPRINT1("Duplicate device instance '%wZ'\n", &InstancePathU);
        DPRINT1("Current instance parent: '%wZ'\n", &DeviceNode->Parent->InstancePath);
        DPRINT1("Old instance parent: '%wZ'\n", &OldDeviceNode->Parent->InstancePath);

        KeBugCheckEx(PNP_DETECTED_FATAL_ERROR,
                     0x01,
                     (ULONG_PTR)DeviceNode->PhysicalDeviceObject,
                     (ULONG_PTR)OldDeviceObject,
                     0);
    }

    DeviceNode->InstancePath = InstancePathU;

    DPRINT("InstancePath is %S\n", DeviceNode->InstancePath.Buffer);

    /*
     * Create registry key for the instance id, if it doesn't exist yet
     */
    Status = IopCreateDeviceKeyPath(&DeviceNode->InstancePath, REG_OPTION_NON_VOLATILE, &InstanceKey);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create the instance key! (Status %lx)\n", Status);

        /* We have to return success otherwise we abort the traverse operation */
        return STATUS_SUCCESS;
    }

    IopQueryHardwareIds(DeviceNode, InstanceKey);

    IopQueryCompatibleIds(DeviceNode, InstanceKey);

    DPRINT("Sending IRP_MN_QUERY_DEVICE_TEXT.DeviceTextDescription to device stack\n");

    Stack.Parameters.QueryDeviceText.DeviceTextType = DeviceTextDescription;
    Stack.Parameters.QueryDeviceText.LocaleId = LocaleId;
    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_DEVICE_TEXT,
                               &Stack);
    DeviceDescription = NT_SUCCESS(Status) ? (PWSTR)IoStatusBlock.Information
                                           : NULL;
    /* This key is mandatory, so even if the Irp fails, we still write it */
    RtlInitUnicodeString(&ValueName, L"DeviceDesc");
    if (ZwQueryValueKey(InstanceKey, &ValueName, KeyValueBasicInformation, NULL, 0, &RequiredLength) == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        if (DeviceDescription &&
            *DeviceDescription != UNICODE_NULL)
        {
            /* This key is overriden when a driver is installed. Don't write the
             * new description if another one already exists */
            Status = ZwSetValueKey(InstanceKey,
                                   &ValueName,
                                   0,
                                   REG_SZ,
                                   DeviceDescription,
                                   ((ULONG)wcslen(DeviceDescription) + 1) * sizeof(WCHAR));
        }
        else
        {
            UNICODE_STRING DeviceDesc = RTL_CONSTANT_STRING(L"Unknown device");
            DPRINT("Driver didn't return DeviceDesc (Status 0x%08lx), so place unknown device there\n", Status);

            Status = ZwSetValueKey(InstanceKey,
                                   &ValueName,
                                   0,
                                   REG_SZ,
                                   DeviceDesc.Buffer,
                                   DeviceDesc.MaximumLength);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("ZwSetValueKey() failed (Status 0x%lx)\n", Status);
            }

        }
    }

    if (DeviceDescription)
    {
        ExFreePoolWithTag(DeviceDescription, 0);
    }

    DPRINT("Sending IRP_MN_QUERY_DEVICE_TEXT.DeviceTextLocation to device stack\n");

    Stack.Parameters.QueryDeviceText.DeviceTextType = DeviceTextLocationInformation;
    Stack.Parameters.QueryDeviceText.LocaleId = LocaleId;
    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_DEVICE_TEXT,
                               &Stack);
    if (NT_SUCCESS(Status) && IoStatusBlock.Information)
    {
        LocationInformation = (PWSTR)IoStatusBlock.Information;
        DPRINT("LocationInformation: %S\n", LocationInformation);
        RtlInitUnicodeString(&ValueName, L"LocationInformation");
        Status = ZwSetValueKey(InstanceKey,
                               &ValueName,
                               0,
                               REG_SZ,
                               LocationInformation,
                               ((ULONG)wcslen(LocationInformation) + 1) * sizeof(WCHAR));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ZwSetValueKey() failed (Status %lx)\n", Status);
        }

        ExFreePoolWithTag(LocationInformation, 0);
    }
    else
    {
        DPRINT("IopInitiatePnpIrp() failed (Status %x) or IoStatusBlock.Information=NULL\n", Status);
    }

    DPRINT("Sending IRP_MN_QUERY_BUS_INFORMATION to device stack\n");

    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_BUS_INFORMATION,
                               NULL);
    if (NT_SUCCESS(Status) && IoStatusBlock.Information)
    {
        PPNP_BUS_INFORMATION BusInformation = (PPNP_BUS_INFORMATION)IoStatusBlock.Information;

        DeviceNode->ChildBusNumber = BusInformation->BusNumber;
        DeviceNode->ChildInterfaceType = BusInformation->LegacyBusType;
        DeviceNode->ChildBusTypeIndex = IopGetBusTypeGuidIndex(&BusInformation->BusTypeGuid);
        ExFreePoolWithTag(BusInformation, 0);
    }
    else
    {
        DPRINT("IopInitiatePnpIrp() failed (Status %x) or IoStatusBlock.Information=NULL\n", Status);

        DeviceNode->ChildBusNumber = 0xFFFFFFF0;
        DeviceNode->ChildInterfaceType = InterfaceTypeUndefined;
        DeviceNode->ChildBusTypeIndex = -1;
    }

    DPRINT("Sending IRP_MN_QUERY_RESOURCES to device stack\n");

    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_RESOURCES,
                               NULL);
    if (NT_SUCCESS(Status) && IoStatusBlock.Information)
    {
        DeviceNode->BootResources = (PCM_RESOURCE_LIST)IoStatusBlock.Information;
        IopDeviceNodeSetFlag(DeviceNode, DNF_HAS_BOOT_CONFIG);
    }
    else
    {
        DPRINT("IopInitiatePnpIrp() failed (Status %x) or IoStatusBlock.Information=NULL\n", Status);
        DeviceNode->BootResources = NULL;
    }

    DPRINT("Sending IRP_MN_QUERY_RESOURCE_REQUIREMENTS to device stack\n");

    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_RESOURCE_REQUIREMENTS,
                               NULL);
    if (NT_SUCCESS(Status))
    {
        DeviceNode->ResourceRequirements = (PIO_RESOURCE_REQUIREMENTS_LIST)IoStatusBlock.Information;
    }
    else
    {
        DPRINT("IopInitiatePnpIrp() failed (Status %08lx)\n", Status);
        DeviceNode->ResourceRequirements = NULL;
    }

    if (InstanceKey != NULL)
    {
        IopSetDeviceInstanceData(InstanceKey, DeviceNode);
    }

    ZwClose(InstanceKey);

    IopDeviceNodeSetFlag(DeviceNode, DNF_PROCESSED);

    if (!IopDeviceNodeHasFlag(DeviceNode, DNF_LEGACY_DRIVER))
    {
        /* Report the device to the user-mode pnp manager */
        IopQueueTargetDeviceEvent(&GUID_DEVICE_ENUMERATED,
                                  &DeviceNode->InstancePath);
    }

    return STATUS_SUCCESS;
}

static
VOID
IopHandleDeviceRemoval(
    IN PDEVICE_NODE DeviceNode,
    IN PDEVICE_RELATIONS DeviceRelations)
{
    PDEVICE_NODE Child = DeviceNode->Child, NextChild;
    ULONG i;
    BOOLEAN Found;

    if (DeviceNode == IopRootDeviceNode)
        return;

    while (Child != NULL)
    {
        NextChild = Child->Sibling;
        Found = FALSE;

        for (i = 0; DeviceRelations && i < DeviceRelations->Count; i++)
        {
            if (IopGetDeviceNode(DeviceRelations->Objects[i]) == Child)
            {
                Found = TRUE;
                break;
            }
        }

        if (!Found && !(Child->Flags & DNF_WILL_BE_REMOVED))
        {
            /* Send removal IRPs to all of its children */
            IopPrepareDeviceForRemoval(Child->PhysicalDeviceObject, TRUE);

            /* Send the surprise removal IRP */
            IopSendSurpriseRemoval(Child->PhysicalDeviceObject);

            /* Tell the user-mode PnP manager that a device was removed */
            IopQueueTargetDeviceEvent(&GUID_DEVICE_SURPRISE_REMOVAL,
                                      &Child->InstancePath);

            /* Send the remove device IRP */
            IopSendRemoveDevice(Child->PhysicalDeviceObject);
        }

        Child = NextChild;
    }
}

NTSTATUS
IopEnumerateDevice(
    IN PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(DeviceObject);
    DEVICETREE_TRAVERSE_CONTEXT Context;
    PDEVICE_RELATIONS DeviceRelations;
    PDEVICE_OBJECT ChildDeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;
    PDEVICE_NODE ChildDeviceNode;
    IO_STACK_LOCATION Stack;
    NTSTATUS Status;
    ULONG i;

    DPRINT("DeviceObject 0x%p\n", DeviceObject);

    if (DeviceNode->Flags & DNF_NEED_ENUMERATION_ONLY)
    {
        DeviceNode->Flags &= ~DNF_NEED_ENUMERATION_ONLY;

        DPRINT("Sending GUID_DEVICE_ARRIVAL\n");
        IopQueueTargetDeviceEvent(&GUID_DEVICE_ARRIVAL,
                                  &DeviceNode->InstancePath);
    }

    DPRINT("Sending IRP_MN_QUERY_DEVICE_RELATIONS to device stack\n");

    Stack.Parameters.QueryDeviceRelations.Type = BusRelations;

    Status = IopInitiatePnpIrp(
        DeviceObject,
        &IoStatusBlock,
        IRP_MN_QUERY_DEVICE_RELATIONS,
        &Stack);
    if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
    {
        DPRINT("IopInitiatePnpIrp() failed with status 0x%08lx\n", Status);
        return Status;
    }

    DeviceRelations = (PDEVICE_RELATIONS)IoStatusBlock.Information;

    /*
     * Send removal IRPs for devices that have disappeared
     * NOTE: This code handles the case where no relations are specified
     */
    IopHandleDeviceRemoval(DeviceNode, DeviceRelations);

    /* Now we bail if nothing was returned */
    if (!DeviceRelations)
    {
        /* We're all done */
        DPRINT("No PDOs\n");
        return STATUS_SUCCESS;
    }

    DPRINT("Got %u PDOs\n", DeviceRelations->Count);

    /*
     * Create device nodes for all discovered devices
     */
    for (i = 0; i < DeviceRelations->Count; i++)
    {
        ChildDeviceObject = DeviceRelations->Objects[i];
        ASSERT((ChildDeviceObject->Flags & DO_DEVICE_INITIALIZING) == 0);

        ChildDeviceNode = IopGetDeviceNode(ChildDeviceObject);
        if (!ChildDeviceNode)
        {
            /* One doesn't exist, create it */
            Status = IopCreateDeviceNode(
                DeviceNode,
                ChildDeviceObject,
                NULL,
                &ChildDeviceNode);
            if (NT_SUCCESS(Status))
            {
                /* Mark the node as enumerated */
                ChildDeviceNode->Flags |= DNF_ENUMERATED;

                /* Mark the DO as bus enumerated */
                ChildDeviceObject->Flags |= DO_BUS_ENUMERATED_DEVICE;
            }
            else
            {
                /* Ignore this DO */
                DPRINT1("IopCreateDeviceNode() failed with status 0x%08x. Skipping PDO %u\n", Status, i);
                ObDereferenceObject(ChildDeviceObject);
            }
        }
        else
        {
            /* Mark it as enumerated */
            ChildDeviceNode->Flags |= DNF_ENUMERATED;
            ObDereferenceObject(ChildDeviceObject);
        }
    }
    ExFreePool(DeviceRelations);

    /*
     * Retrieve information about all discovered children from the bus driver
     */
    IopInitDeviceTreeTraverseContext(
        &Context,
        DeviceNode,
        IopActionInterrogateDeviceStack,
        DeviceNode);

    Status = IopTraverseDeviceTree(&Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopTraverseDeviceTree() failed with status 0x%08lx\n", Status);
        return Status;
    }

    /*
     * Retrieve configuration from the registry for discovered children
     */
    IopInitDeviceTreeTraverseContext(
        &Context,
        DeviceNode,
        IopActionConfigureChildServices,
        DeviceNode);

    Status = IopTraverseDeviceTree(&Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopTraverseDeviceTree() failed with status 0x%08lx\n", Status);
        return Status;
    }

    /*
     * Initialize services for discovered children.
     */
    Status = IopInitializePnpServices(DeviceNode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopInitializePnpServices() failed with status 0x%08lx\n", Status);
        return Status;
    }

    DPRINT("IopEnumerateDevice() finished\n");
    return STATUS_SUCCESS;
}


/*
 * IopActionConfigureChildServices
 *
 * Retrieve configuration for all (direct) child nodes of a parent node.
 *
 * Parameters
 *    DeviceNode
 *       Pointer to device node.
 *    Context
 *       Pointer to parent node to retrieve child node configuration for.
 *
 * Remarks
 *    Any errors that occur are logged instead so that all child services have a chance of beeing
 *    configured.
 */

NTSTATUS
IopActionConfigureChildServices(PDEVICE_NODE DeviceNode,
                                PVOID Context)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[3];
   PDEVICE_NODE ParentDeviceNode;
   PUNICODE_STRING Service;
   UNICODE_STRING ClassGUID;
   NTSTATUS Status;
   DEVICE_CAPABILITIES DeviceCaps;

   DPRINT("IopActionConfigureChildServices(%p, %p)\n", DeviceNode, Context);

   ParentDeviceNode = (PDEVICE_NODE)Context;

   /*
    * We are called for the parent too, but we don't need to do special
    * handling for this node
    */
   if (DeviceNode == ParentDeviceNode)
   {
      DPRINT("Success\n");
      return STATUS_SUCCESS;
   }

   /*
    * Make sure this device node is a direct child of the parent device node
    * that is given as an argument
    */

   if (DeviceNode->Parent != ParentDeviceNode)
   {
      DPRINT("Skipping 2+ level child\n");
      return STATUS_SUCCESS;
   }

   if (!(DeviceNode->Flags & DNF_PROCESSED))
   {
       DPRINT1("Child not ready to be configured\n");
       return STATUS_SUCCESS;
   }

   if (!(DeviceNode->Flags & (DNF_DISABLED | DNF_STARTED | DNF_ADDED)))
   {
      WCHAR RegKeyBuffer[MAX_PATH];
      UNICODE_STRING RegKey;

      /* Install the service for this if it's in the CDDB */
      IopInstallCriticalDevice(DeviceNode);

      RegKey.Length = 0;
      RegKey.MaximumLength = sizeof(RegKeyBuffer);
      RegKey.Buffer = RegKeyBuffer;

      /*
       * Retrieve configuration from Enum key
       */

      Service = &DeviceNode->ServiceName;

      RtlZeroMemory(QueryTable, sizeof(QueryTable));
      RtlInitUnicodeString(Service, NULL);
      RtlInitUnicodeString(&ClassGUID, NULL);

      QueryTable[0].Name = L"Service";
      QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
      QueryTable[0].EntryContext = Service;

      QueryTable[1].Name = L"ClassGUID";
      QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
      QueryTable[1].EntryContext = &ClassGUID;
      QueryTable[1].DefaultType = REG_SZ;
      QueryTable[1].DefaultData = L"";
      QueryTable[1].DefaultLength = 0;

      RtlAppendUnicodeToString(&RegKey, L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\");
      RtlAppendUnicodeStringToString(&RegKey, &DeviceNode->InstancePath);

      Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
         RegKey.Buffer, QueryTable, NULL, NULL);

      if (!NT_SUCCESS(Status))
      {
         /* FIXME: Log the error */
         DPRINT("Could not retrieve configuration for device %wZ (Status 0x%08x)\n",
            &DeviceNode->InstancePath, Status);
         IopDeviceNodeSetFlag(DeviceNode, DNF_DISABLED);
         return STATUS_SUCCESS;
      }

      if (Service->Buffer == NULL)
      {
         if (NT_SUCCESS(IopQueryDeviceCapabilities(DeviceNode, &DeviceCaps)) &&
             DeviceCaps.RawDeviceOK)
         {
            DPRINT("%wZ is using parent bus driver (%wZ)\n", &DeviceNode->InstancePath, &ParentDeviceNode->ServiceName);

            DeviceNode->ServiceName.Length = 0;
            DeviceNode->ServiceName.MaximumLength = 0;
            DeviceNode->ServiceName.Buffer = NULL;
         }
         else if (ClassGUID.Length != 0)
         {
            /* Device has a ClassGUID value, but no Service value.
             * Suppose it is using the NULL driver, so state the
             * device is started */
            DPRINT("%wZ is using NULL driver\n", &DeviceNode->InstancePath);
            IopDeviceNodeSetFlag(DeviceNode, DNF_STARTED);
         }
         else
         {
            DeviceNode->Problem = CM_PROB_FAILED_INSTALL;
            IopDeviceNodeSetFlag(DeviceNode, DNF_DISABLED);
         }
         return STATUS_SUCCESS;
      }

      DPRINT("Got Service %S\n", Service->Buffer);
   }

   return STATUS_SUCCESS;
}

/*
 * IopActionInitChildServices
 *
 * Initialize the service for all (direct) child nodes of a parent node
 *
 * Parameters
 *    DeviceNode
 *       Pointer to device node.
 *    Context
 *       Pointer to parent node to initialize child node services for.
 *
 * Remarks
 *    If the driver image for a service is not loaded and initialized
 *    it is done here too. Any errors that occur are logged instead so
 *    that all child services have a chance of being initialized.
 */

NTSTATUS
IopActionInitChildServices(PDEVICE_NODE DeviceNode,
                           PVOID Context)
{
   PDEVICE_NODE ParentDeviceNode;
   NTSTATUS Status;
   BOOLEAN BootDrivers = !PnpSystemInit;

   DPRINT("IopActionInitChildServices(%p, %p)\n", DeviceNode, Context);

   ParentDeviceNode = Context;

   /*
    * We are called for the parent too, but we don't need to do special
    * handling for this node
    */
   if (DeviceNode == ParentDeviceNode)
   {
      DPRINT("Success\n");
      return STATUS_SUCCESS;
   }

   /*
    * We don't want to check for a direct child because
    * this function is called during boot to reinitialize
    * devices with drivers that couldn't load yet due to
    * stage 0 limitations (ie can't load from disk yet).
    */

   if (!(DeviceNode->Flags & DNF_PROCESSED))
   {
       DPRINT1("Child not ready to be added\n");
       return STATUS_SUCCESS;
   }

   if (IopDeviceNodeHasFlag(DeviceNode, DNF_STARTED) ||
       IopDeviceNodeHasFlag(DeviceNode, DNF_ADDED) ||
       IopDeviceNodeHasFlag(DeviceNode, DNF_DISABLED))
       return STATUS_SUCCESS;

   if (DeviceNode->ServiceName.Buffer == NULL)
   {
      /* We don't need to worry about loading the driver because we're
       * being driven in raw mode so our parent must be loaded to get here */
      Status = IopInitializeDevice(DeviceNode, NULL);
      if (NT_SUCCESS(Status))
      {
          Status = IopStartDevice(DeviceNode);
          if (!NT_SUCCESS(Status))
          {
              DPRINT1("IopStartDevice(%wZ) failed with status 0x%08x\n",
                      &DeviceNode->InstancePath, Status);
          }
      }
   }
   else
   {
      PLDR_DATA_TABLE_ENTRY ModuleObject;
      PDRIVER_OBJECT DriverObject;

      KeEnterCriticalRegion();
      ExAcquireResourceExclusiveLite(&IopDriverLoadResource, TRUE);
      /* Get existing DriverObject pointer (in case the driver has
         already been loaded and initialized) */
      Status = IopGetDriverObject(
          &DriverObject,
          &DeviceNode->ServiceName,
          FALSE);

      if (!NT_SUCCESS(Status))
      {
         /* Driver is not initialized, try to load it */
         Status = IopLoadServiceModule(&DeviceNode->ServiceName, &ModuleObject);

         if (NT_SUCCESS(Status) || Status == STATUS_IMAGE_ALREADY_LOADED)
         {
            /* Initialize the driver */
            Status = IopInitializeDriverModule(DeviceNode, ModuleObject,
                  &DeviceNode->ServiceName, FALSE, &DriverObject);
            if (!NT_SUCCESS(Status)) DeviceNode->Problem = CM_PROB_FAILED_DRIVER_ENTRY;
         }
         else if (Status == STATUS_DRIVER_UNABLE_TO_LOAD)
         {
            DPRINT1("Service '%wZ' is disabled\n", &DeviceNode->ServiceName);
            DeviceNode->Problem = CM_PROB_DISABLED_SERVICE;
         }
         else
         {
            DPRINT("IopLoadServiceModule(%wZ) failed with status 0x%08x\n",
                    &DeviceNode->ServiceName, Status);
            if (!BootDrivers) DeviceNode->Problem = CM_PROB_DRIVER_FAILED_LOAD;
         }
      }
      ExReleaseResourceLite(&IopDriverLoadResource);
      KeLeaveCriticalRegion();

      /* Driver is loaded and initialized at this point */
      if (NT_SUCCESS(Status))
      {
          /* Initialize the device, including all filters */
          Status = PipCallDriverAddDevice(DeviceNode, FALSE, DriverObject);

          /* Remove the extra reference */
          ObDereferenceObject(DriverObject);
      }
      else
      {
         /*
          * Don't disable when trying to load only boot drivers
          */
         if (!BootDrivers)
         {
            IopDeviceNodeSetFlag(DeviceNode, DNF_DISABLED);
         }
      }
   }

   return STATUS_SUCCESS;
}

/*
 * IopInitializePnpServices
 *
 * Initialize services for discovered children
 *
 * Parameters
 *    DeviceNode
 *       Top device node to start initializing services.
 *
 * Return Value
 *    Status
 */
NTSTATUS
IopInitializePnpServices(IN PDEVICE_NODE DeviceNode)
{
   DEVICETREE_TRAVERSE_CONTEXT Context;

   DPRINT("IopInitializePnpServices(%p)\n", DeviceNode);

   IopInitDeviceTreeTraverseContext(
      &Context,
      DeviceNode,
      IopActionInitChildServices,
      DeviceNode);

   return IopTraverseDeviceTree(&Context);
}

static NTSTATUS INIT_FUNCTION
IopEnumerateDetectedDevices(
   IN HANDLE hBaseKey,
   IN PUNICODE_STRING RelativePath OPTIONAL,
   IN HANDLE hRootKey,
   IN BOOLEAN EnumerateSubKeys,
   IN PCM_FULL_RESOURCE_DESCRIPTOR ParentBootResources,
   IN ULONG ParentBootResourcesLength)
{
   UNICODE_STRING IdentifierU = RTL_CONSTANT_STRING(L"Identifier");
   UNICODE_STRING HardwareIDU = RTL_CONSTANT_STRING(L"HardwareID");
   UNICODE_STRING ConfigurationDataU = RTL_CONSTANT_STRING(L"Configuration Data");
   UNICODE_STRING BootConfigU = RTL_CONSTANT_STRING(L"BootConfig");
   UNICODE_STRING LogConfU = RTL_CONSTANT_STRING(L"LogConf");
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE hDevicesKey = NULL;
   HANDLE hDeviceKey = NULL;
   HANDLE hLevel1Key, hLevel2Key = NULL, hLogConf;
   UNICODE_STRING Level2NameU;
   WCHAR Level2Name[5];
   ULONG IndexDevice = 0;
   ULONG IndexSubKey;
   PKEY_BASIC_INFORMATION pDeviceInformation = NULL;
   ULONG DeviceInfoLength = sizeof(KEY_BASIC_INFORMATION) + 50 * sizeof(WCHAR);
   PKEY_VALUE_PARTIAL_INFORMATION pValueInformation = NULL;
   ULONG ValueInfoLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 50 * sizeof(WCHAR);
   UNICODE_STRING DeviceName, ValueName;
   ULONG RequiredSize;
   PCM_FULL_RESOURCE_DESCRIPTOR BootResources = NULL;
   ULONG BootResourcesLength;
   NTSTATUS Status;

   const UNICODE_STRING IdentifierSerial = RTL_CONSTANT_STRING(L"SerialController");
   UNICODE_STRING HardwareIdSerial = RTL_CONSTANT_STRING(L"*PNP0501\0");
   static ULONG DeviceIndexSerial = 0;
   const UNICODE_STRING IdentifierKeyboard = RTL_CONSTANT_STRING(L"KeyboardController");
   UNICODE_STRING HardwareIdKeyboard = RTL_CONSTANT_STRING(L"*PNP0303\0");
   static ULONG DeviceIndexKeyboard = 0;
   const UNICODE_STRING IdentifierMouse = RTL_CONSTANT_STRING(L"PointerController");
   UNICODE_STRING HardwareIdMouse = RTL_CONSTANT_STRING(L"*PNP0F13\0");
   static ULONG DeviceIndexMouse = 0;
   const UNICODE_STRING IdentifierParallel = RTL_CONSTANT_STRING(L"ParallelController");
   UNICODE_STRING HardwareIdParallel = RTL_CONSTANT_STRING(L"*PNP0400\0");
   static ULONG DeviceIndexParallel = 0;
   const UNICODE_STRING IdentifierFloppy = RTL_CONSTANT_STRING(L"FloppyDiskPeripheral");
   UNICODE_STRING HardwareIdFloppy = RTL_CONSTANT_STRING(L"*PNP0700\0");
   static ULONG DeviceIndexFloppy = 0;
   UNICODE_STRING HardwareIdKey;
   PUNICODE_STRING pHardwareId;
   ULONG DeviceIndex = 0;
   PUCHAR CmResourceList;
   ULONG ListCount;

    if (RelativePath)
    {
        Status = IopOpenRegistryKeyEx(&hDevicesKey, hBaseKey, RelativePath, KEY_ENUMERATE_SUB_KEYS);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwOpenKey() failed with status 0x%08lx\n", Status);
            goto cleanup;
        }
    }
    else
        hDevicesKey = hBaseKey;

   pDeviceInformation = ExAllocatePool(PagedPool, DeviceInfoLength);
   if (!pDeviceInformation)
   {
      DPRINT("ExAllocatePool() failed\n");
      Status = STATUS_NO_MEMORY;
      goto cleanup;
   }

   pValueInformation = ExAllocatePool(PagedPool, ValueInfoLength);
   if (!pValueInformation)
   {
      DPRINT("ExAllocatePool() failed\n");
      Status = STATUS_NO_MEMORY;
      goto cleanup;
   }

   while (TRUE)
   {
      Status = ZwEnumerateKey(hDevicesKey, IndexDevice, KeyBasicInformation, pDeviceInformation, DeviceInfoLength, &RequiredSize);
      if (Status == STATUS_NO_MORE_ENTRIES)
         break;
      else if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
      {
         ExFreePool(pDeviceInformation);
         DeviceInfoLength = RequiredSize;
         pDeviceInformation = ExAllocatePool(PagedPool, DeviceInfoLength);
         if (!pDeviceInformation)
         {
            DPRINT("ExAllocatePool() failed\n");
            Status = STATUS_NO_MEMORY;
            goto cleanup;
         }
         Status = ZwEnumerateKey(hDevicesKey, IndexDevice, KeyBasicInformation, pDeviceInformation, DeviceInfoLength, &RequiredSize);
      }
      if (!NT_SUCCESS(Status))
      {
         DPRINT("ZwEnumerateKey() failed with status 0x%08lx\n", Status);
         goto cleanup;
      }
      IndexDevice++;

      /* Open device key */
      DeviceName.Length = DeviceName.MaximumLength = (USHORT)pDeviceInformation->NameLength;
      DeviceName.Buffer = pDeviceInformation->Name;

      Status = IopOpenRegistryKeyEx(&hDeviceKey, hDevicesKey, &DeviceName,
          KEY_QUERY_VALUE + (EnumerateSubKeys ? KEY_ENUMERATE_SUB_KEYS : 0));
      if (!NT_SUCCESS(Status))
      {
         DPRINT("ZwOpenKey() failed with status 0x%08lx\n", Status);
         goto cleanup;
      }

      /* Read boot resources, and add then to parent ones */
      Status = ZwQueryValueKey(hDeviceKey, &ConfigurationDataU, KeyValuePartialInformation, pValueInformation, ValueInfoLength, &RequiredSize);
      if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
      {
         ExFreePool(pValueInformation);
         ValueInfoLength = RequiredSize;
         pValueInformation = ExAllocatePool(PagedPool, ValueInfoLength);
         if (!pValueInformation)
         {
            DPRINT("ExAllocatePool() failed\n");
            ZwDeleteKey(hLevel2Key);
            Status = STATUS_NO_MEMORY;
            goto cleanup;
         }
         Status = ZwQueryValueKey(hDeviceKey, &ConfigurationDataU, KeyValuePartialInformation, pValueInformation, ValueInfoLength, &RequiredSize);
      }
      if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
      {
         BootResources = ParentBootResources;
         BootResourcesLength = ParentBootResourcesLength;
      }
      else if (!NT_SUCCESS(Status))
      {
         DPRINT("ZwQueryValueKey() failed with status 0x%08lx\n", Status);
         goto nextdevice;
      }
      else if (pValueInformation->Type != REG_FULL_RESOURCE_DESCRIPTOR)
      {
         DPRINT("Wrong registry type: got 0x%lx, expected 0x%lx\n", pValueInformation->Type, REG_FULL_RESOURCE_DESCRIPTOR);
         goto nextdevice;
      }
      else
      {
         static const ULONG Header = FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList.PartialDescriptors);

         /* Concatenate current resources and parent ones */
         if (ParentBootResourcesLength == 0)
            BootResourcesLength = pValueInformation->DataLength;
         else
            BootResourcesLength = ParentBootResourcesLength
            + pValueInformation->DataLength
               - Header;
         BootResources = ExAllocatePool(PagedPool, BootResourcesLength);
         if (!BootResources)
         {
            DPRINT("ExAllocatePool() failed\n");
            goto nextdevice;
         }
         if (ParentBootResourcesLength < sizeof(CM_FULL_RESOURCE_DESCRIPTOR))
         {
            RtlCopyMemory(BootResources, pValueInformation->Data, pValueInformation->DataLength);
         }
         else if (ParentBootResources->PartialResourceList.PartialDescriptors[ParentBootResources->PartialResourceList.Count - 1].Type == CmResourceTypeDeviceSpecific)
         {
            RtlCopyMemory(BootResources, pValueInformation->Data, pValueInformation->DataLength);
            RtlCopyMemory(
               (PVOID)((ULONG_PTR)BootResources + pValueInformation->DataLength),
               (PVOID)((ULONG_PTR)ParentBootResources + Header),
               ParentBootResourcesLength - Header);
            BootResources->PartialResourceList.Count += ParentBootResources->PartialResourceList.Count;
         }
         else
         {
            RtlCopyMemory(BootResources, pValueInformation->Data, Header);
            RtlCopyMemory(
               (PVOID)((ULONG_PTR)BootResources + Header),
               (PVOID)((ULONG_PTR)ParentBootResources + Header),
               ParentBootResourcesLength - Header);
            RtlCopyMemory(
               (PVOID)((ULONG_PTR)BootResources + ParentBootResourcesLength),
               pValueInformation->Data + Header,
               pValueInformation->DataLength - Header);
            BootResources->PartialResourceList.Count += ParentBootResources->PartialResourceList.Count;
         }
      }

      if (EnumerateSubKeys)
      {
         IndexSubKey = 0;
         while (TRUE)
         {
            Status = ZwEnumerateKey(hDeviceKey, IndexSubKey, KeyBasicInformation, pDeviceInformation, DeviceInfoLength, &RequiredSize);
            if (Status == STATUS_NO_MORE_ENTRIES)
               break;
            else if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
            {
               ExFreePool(pDeviceInformation);
               DeviceInfoLength = RequiredSize;
               pDeviceInformation = ExAllocatePool(PagedPool, DeviceInfoLength);
               if (!pDeviceInformation)
               {
                  DPRINT("ExAllocatePool() failed\n");
                  Status = STATUS_NO_MEMORY;
                  goto cleanup;
               }
               Status = ZwEnumerateKey(hDeviceKey, IndexSubKey, KeyBasicInformation, pDeviceInformation, DeviceInfoLength, &RequiredSize);
            }
            if (!NT_SUCCESS(Status))
            {
               DPRINT("ZwEnumerateKey() failed with status 0x%08lx\n", Status);
               goto cleanup;
            }
            IndexSubKey++;
            DeviceName.Length = DeviceName.MaximumLength = (USHORT)pDeviceInformation->NameLength;
            DeviceName.Buffer = pDeviceInformation->Name;

            Status = IopEnumerateDetectedDevices(
               hDeviceKey,
               &DeviceName,
               hRootKey,
               TRUE,
               BootResources,
               BootResourcesLength);
            if (!NT_SUCCESS(Status))
               goto cleanup;
         }
      }

      /* Read identifier */
      Status = ZwQueryValueKey(hDeviceKey, &IdentifierU, KeyValuePartialInformation, pValueInformation, ValueInfoLength, &RequiredSize);
      if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
      {
         ExFreePool(pValueInformation);
         ValueInfoLength = RequiredSize;
         pValueInformation = ExAllocatePool(PagedPool, ValueInfoLength);
         if (!pValueInformation)
         {
            DPRINT("ExAllocatePool() failed\n");
            Status = STATUS_NO_MEMORY;
            goto cleanup;
         }
         Status = ZwQueryValueKey(hDeviceKey, &IdentifierU, KeyValuePartialInformation, pValueInformation, ValueInfoLength, &RequiredSize);
      }
      if (!NT_SUCCESS(Status))
      {
         if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
         {
            DPRINT("ZwQueryValueKey() failed with status 0x%08lx\n", Status);
            goto nextdevice;
         }
         ValueName.Length = ValueName.MaximumLength = 0;
      }
      else if (pValueInformation->Type != REG_SZ)
      {
         DPRINT("Wrong registry type: got 0x%lx, expected 0x%lx\n", pValueInformation->Type, REG_SZ);
         goto nextdevice;
      }
      else
      {
         /* Assign hardware id to this device */
         ValueName.Length = ValueName.MaximumLength = (USHORT)pValueInformation->DataLength;
         ValueName.Buffer = (PWCHAR)pValueInformation->Data;
         if (ValueName.Length >= sizeof(WCHAR) && ValueName.Buffer[ValueName.Length / sizeof(WCHAR) - 1] == UNICODE_NULL)
            ValueName.Length -= sizeof(WCHAR);
      }

      if (RelativePath && RtlCompareUnicodeString(RelativePath, &IdentifierSerial, FALSE) == 0)
      {
         pHardwareId = &HardwareIdSerial;
         DeviceIndex = DeviceIndexSerial++;
      }
      else if (RelativePath && RtlCompareUnicodeString(RelativePath, &IdentifierKeyboard, FALSE) == 0)
      {
         pHardwareId = &HardwareIdKeyboard;
         DeviceIndex = DeviceIndexKeyboard++;
      }
      else if (RelativePath && RtlCompareUnicodeString(RelativePath, &IdentifierMouse, FALSE) == 0)
      {
         pHardwareId = &HardwareIdMouse;
         DeviceIndex = DeviceIndexMouse++;
      }
      else if (RelativePath && RtlCompareUnicodeString(RelativePath, &IdentifierParallel, FALSE) == 0)
      {
         pHardwareId = &HardwareIdParallel;
         DeviceIndex = DeviceIndexParallel++;
      }
      else if (RelativePath && RtlCompareUnicodeString(RelativePath, &IdentifierFloppy, FALSE) == 0)
      {
         pHardwareId = &HardwareIdFloppy;
         DeviceIndex = DeviceIndexFloppy++;
      }
      else
      {
         /* Unknown key path */
         DPRINT("Unknown key path '%wZ'\n", RelativePath);
         goto nextdevice;
      }

      /* Prepare hardware id key (hardware id value without final \0) */
      HardwareIdKey = *pHardwareId;
      HardwareIdKey.Length -= sizeof(UNICODE_NULL);

      /* Add the detected device to Root key */
      InitializeObjectAttributes(&ObjectAttributes, &HardwareIdKey, OBJ_KERNEL_HANDLE, hRootKey, NULL);
      Status = ZwCreateKey(
         &hLevel1Key,
         KEY_CREATE_SUB_KEY,
         &ObjectAttributes,
         0,
         NULL,
         REG_OPTION_NON_VOLATILE,
         NULL);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("ZwCreateKey() failed with status 0x%08lx\n", Status);
         goto nextdevice;
      }
      swprintf(Level2Name, L"%04lu", DeviceIndex);
      RtlInitUnicodeString(&Level2NameU, Level2Name);
      InitializeObjectAttributes(&ObjectAttributes, &Level2NameU, OBJ_KERNEL_HANDLE, hLevel1Key, NULL);
      Status = ZwCreateKey(
         &hLevel2Key,
         KEY_SET_VALUE | KEY_CREATE_SUB_KEY,
         &ObjectAttributes,
         0,
         NULL,
         REG_OPTION_NON_VOLATILE,
         NULL);
      ZwClose(hLevel1Key);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("ZwCreateKey() failed with status 0x%08lx\n", Status);
         goto nextdevice;
      }
      DPRINT("Found %wZ #%lu (%wZ)\n", &ValueName, DeviceIndex, &HardwareIdKey);
      Status = ZwSetValueKey(hLevel2Key, &HardwareIDU, 0, REG_MULTI_SZ, pHardwareId->Buffer, pHardwareId->MaximumLength);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("ZwSetValueKey() failed with status 0x%08lx\n", Status);
         ZwDeleteKey(hLevel2Key);
         goto nextdevice;
      }
      /* Create 'LogConf' subkey */
      InitializeObjectAttributes(&ObjectAttributes, &LogConfU, OBJ_KERNEL_HANDLE, hLevel2Key, NULL);
      Status = ZwCreateKey(
         &hLogConf,
         KEY_SET_VALUE,
         &ObjectAttributes,
         0,
         NULL,
         REG_OPTION_VOLATILE,
         NULL);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("ZwCreateKey() failed with status 0x%08lx\n", Status);
         ZwDeleteKey(hLevel2Key);
         goto nextdevice;
      }
      if (BootResourcesLength >= sizeof(CM_FULL_RESOURCE_DESCRIPTOR))
      {
         CmResourceList = ExAllocatePool(PagedPool, BootResourcesLength + sizeof(ULONG));
         if (!CmResourceList)
         {
            ZwClose(hLogConf);
            ZwDeleteKey(hLevel2Key);
            goto nextdevice;
         }

         /* Add the list count (1st member of CM_RESOURCE_LIST) */
         ListCount = 1;
         RtlCopyMemory(CmResourceList,
                       &ListCount,
                       sizeof(ULONG));

         /* Now add the actual list (2nd member of CM_RESOURCE_LIST) */
         RtlCopyMemory(CmResourceList + sizeof(ULONG),
                       BootResources,
                       BootResourcesLength);

         /* Save boot resources to 'LogConf\BootConfig' */
         Status = ZwSetValueKey(hLogConf, &BootConfigU, 0, REG_RESOURCE_LIST, CmResourceList, BootResourcesLength + sizeof(ULONG));
         if (!NT_SUCCESS(Status))
         {
            DPRINT("ZwSetValueKey() failed with status 0x%08lx\n", Status);
            ZwClose(hLogConf);
            ZwDeleteKey(hLevel2Key);
            goto nextdevice;
         }
      }
      ZwClose(hLogConf);

nextdevice:
      if (BootResources && BootResources != ParentBootResources)
      {
         ExFreePool(BootResources);
         BootResources = NULL;
      }
      if (hLevel2Key)
      {
         ZwClose(hLevel2Key);
         hLevel2Key = NULL;
      }
      if (hDeviceKey)
      {
         ZwClose(hDeviceKey);
         hDeviceKey = NULL;
      }
   }

   Status = STATUS_SUCCESS;

cleanup:
   if (hDevicesKey && hDevicesKey != hBaseKey)
      ZwClose(hDevicesKey);
   if (hDeviceKey)
      ZwClose(hDeviceKey);
   if (pDeviceInformation)
      ExFreePool(pDeviceInformation);
   if (pValueInformation)
      ExFreePool(pValueInformation);
   return Status;
}

static BOOLEAN INIT_FUNCTION
IopIsFirmwareMapperDisabled(VOID)
{
   UNICODE_STRING KeyPathU = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CURRENTCONTROLSET\\Control\\Pnp");
   UNICODE_STRING KeyNameU = RTL_CONSTANT_STRING(L"DisableFirmwareMapper");
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE hPnpKey;
   PKEY_VALUE_PARTIAL_INFORMATION KeyInformation;
   ULONG DesiredLength, Length;
   ULONG KeyValue = 0;
   NTSTATUS Status;

   InitializeObjectAttributes(&ObjectAttributes, &KeyPathU, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
   Status = ZwOpenKey(&hPnpKey, KEY_QUERY_VALUE, &ObjectAttributes);
   if (NT_SUCCESS(Status))
   {
       Status = ZwQueryValueKey(hPnpKey,
                                &KeyNameU,
                                KeyValuePartialInformation,
                                NULL,
                                0,
                                &DesiredLength);
       if ((Status == STATUS_BUFFER_TOO_SMALL) ||
           (Status == STATUS_BUFFER_OVERFLOW))
       {
           Length = DesiredLength;
           KeyInformation = ExAllocatePool(PagedPool, Length);
           if (KeyInformation)
           {
               Status = ZwQueryValueKey(hPnpKey,
                                        &KeyNameU,
                                        KeyValuePartialInformation,
                                        KeyInformation,
                                        Length,
                                        &DesiredLength);
               if (NT_SUCCESS(Status) && KeyInformation->DataLength == sizeof(ULONG))
               {
                   KeyValue = (ULONG)(*KeyInformation->Data);
               }
               else
               {
                   DPRINT1("ZwQueryValueKey(%wZ%wZ) failed\n", &KeyPathU, &KeyNameU);
               }

               ExFreePool(KeyInformation);
           }
           else
           {
               DPRINT1("Failed to allocate memory for registry query\n");
           }
       }
       else
       {
           DPRINT1("ZwQueryValueKey(%wZ%wZ) failed with status 0x%08lx\n", &KeyPathU, &KeyNameU, Status);
       }

       ZwClose(hPnpKey);
   }
   else
   {
       DPRINT1("ZwOpenKey(%wZ) failed with status 0x%08lx\n", &KeyPathU, Status);
   }

   DPRINT("Firmware mapper is %s\n", KeyValue != 0 ? "disabled" : "enabled");

   return (KeyValue != 0) ? TRUE : FALSE;
}

NTSTATUS
NTAPI
INIT_FUNCTION
IopUpdateRootKey(VOID)
{
   UNICODE_STRING EnumU = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Enum");
   UNICODE_STRING RootPathU = RTL_CONSTANT_STRING(L"Root");
   UNICODE_STRING MultiKeyPathU = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter");
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE hEnum, hRoot;
   NTSTATUS Status;

   InitializeObjectAttributes(&ObjectAttributes, &EnumU, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
   Status = ZwCreateKey(&hEnum, KEY_CREATE_SUB_KEY, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("ZwCreateKey() failed with status 0x%08lx\n", Status);
      return Status;
   }

   InitializeObjectAttributes(&ObjectAttributes, &RootPathU, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, hEnum, NULL);
   Status = ZwCreateKey(&hRoot, KEY_CREATE_SUB_KEY, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
   ZwClose(hEnum);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("ZwOpenKey() failed with status 0x%08lx\n", Status);
      return Status;
   }

   if (!IopIsFirmwareMapperDisabled())
   {
        Status = IopOpenRegistryKeyEx(&hEnum, NULL, &MultiKeyPathU, KEY_ENUMERATE_SUB_KEYS);
        if (!NT_SUCCESS(Status))
        {
            /* Nothing to do, don't return with an error status */
            DPRINT("ZwOpenKey() failed with status 0x%08lx\n", Status);
            ZwClose(hRoot);
            return STATUS_SUCCESS;
        }
        Status = IopEnumerateDetectedDevices(
            hEnum,
            NULL,
            hRoot,
            TRUE,
            NULL,
            0);
        ZwClose(hEnum);
   }
   else
   {
        /* Enumeration is disabled */
        Status = STATUS_SUCCESS;
   }

   ZwClose(hRoot);

   return Status;
}

NTSTATUS
NTAPI
IopOpenRegistryKeyEx(PHANDLE KeyHandle,
                     HANDLE ParentKey,
                     PUNICODE_STRING Name,
                     ACCESS_MASK DesiredAccess)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    PAGED_CODE();

    *KeyHandle = NULL;

    InitializeObjectAttributes(&ObjectAttributes,
        Name,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        ParentKey,
        NULL);

    Status = ZwOpenKey(KeyHandle, DesiredAccess, &ObjectAttributes);

    return Status;
}

NTSTATUS
NTAPI
IopCreateRegistryKeyEx(OUT PHANDLE Handle,
                       IN HANDLE RootHandle OPTIONAL,
                       IN PUNICODE_STRING KeyName,
                       IN ACCESS_MASK DesiredAccess,
                       IN ULONG CreateOptions,
                       OUT PULONG Disposition OPTIONAL)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG KeyDisposition, RootHandleIndex = 0, i = 1, NestedCloseLevel = 0;
    USHORT Length;
    HANDLE HandleArray[2];
    BOOLEAN Recursing = TRUE;
    PWCHAR pp, p, p1;
    UNICODE_STRING KeyString;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* P1 is start, pp is end */
    p1 = KeyName->Buffer;
    pp = (PVOID)((ULONG_PTR)p1 + KeyName->Length);

    /* Create the target key */
    InitializeObjectAttributes(&ObjectAttributes,
                               KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               RootHandle,
                               NULL);
    Status = ZwCreateKey(&HandleArray[i],
                         DesiredAccess,
                         &ObjectAttributes,
                         0,
                         NULL,
                         CreateOptions,
                         &KeyDisposition);

    /* Now we check if this failed */
    if ((Status == STATUS_OBJECT_NAME_NOT_FOUND) && (RootHandle))
    {
        /* Target key failed, so we'll need to create its parent. Setup array */
        HandleArray[0] = NULL;
        HandleArray[1] = RootHandle;

        /* Keep recursing for each missing parent */
        while (Recursing)
        {
            /* And if we're deep enough, close the last handle */
            if (NestedCloseLevel > 1) ZwClose(HandleArray[RootHandleIndex]);

            /* We're setup to ping-pong between the two handle array entries */
            RootHandleIndex = i;
            i = (i + 1) & 1;

            /* Clear the one we're attempting to open now */
            HandleArray[i] = NULL;

            /* Process the parent key name */
            for (p = p1; ((p < pp) && (*p != OBJ_NAME_PATH_SEPARATOR)); p++);
            Length = (USHORT)(p - p1) * sizeof(WCHAR);

            /* Is there a parent name? */
            if (Length)
            {
                /* Build the unicode string for it */
                KeyString.Buffer = p1;
                KeyString.Length = KeyString.MaximumLength = Length;

                /* Now try opening the parent */
                InitializeObjectAttributes(&ObjectAttributes,
                                           &KeyString,
                                           OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                           HandleArray[RootHandleIndex],
                                           NULL);
                Status = ZwCreateKey(&HandleArray[i],
                                     DesiredAccess,
                                     &ObjectAttributes,
                                     0,
                                     NULL,
                                     CreateOptions,
                                     &KeyDisposition);
                if (NT_SUCCESS(Status))
                {
                    /* It worked, we have one more handle */
                    NestedCloseLevel++;
                }
                else
                {
                    /* Parent key creation failed, abandon loop */
                    Recursing = FALSE;
                    continue;
                }
            }
            else
            {
                /* We don't have a parent name, probably corrupted key name */
                Status = STATUS_INVALID_PARAMETER;
                Recursing = FALSE;
                continue;
            }

            /* Now see if there's more parents to create */
            p1 = p + 1;
            if ((p == pp) || (p1 == pp))
            {
                /* We're done, hopefully successfully, so stop */
                Recursing = FALSE;
            }
        }

        /* Outer loop check for handle nesting that requires closing the top handle */
        if (NestedCloseLevel > 1) ZwClose(HandleArray[RootHandleIndex]);
    }

    /* Check if we broke out of the loop due to success */
    if (NT_SUCCESS(Status))
    {
        /* Return the target handle (we closed all the parent ones) and disposition */
        *Handle = HandleArray[i];
        if (Disposition) *Disposition = KeyDisposition;
    }

    /* Return the success state */
    return Status;
}

NTSTATUS
NTAPI
IopGetRegistryValue(IN HANDLE Handle,
                    IN PWSTR ValueName,
                    OUT PKEY_VALUE_FULL_INFORMATION *Information)
{
    UNICODE_STRING ValueString;
    NTSTATUS Status;
    PKEY_VALUE_FULL_INFORMATION FullInformation;
    ULONG Size;
    PAGED_CODE();

    RtlInitUnicodeString(&ValueString, ValueName);

    Status = ZwQueryValueKey(Handle,
                             &ValueString,
                             KeyValueFullInformation,
                             NULL,
                             0,
                             &Size);
    if ((Status != STATUS_BUFFER_OVERFLOW) &&
        (Status != STATUS_BUFFER_TOO_SMALL))
    {
        return Status;
    }

    FullInformation = ExAllocatePool(NonPagedPool, Size);
    if (!FullInformation) return STATUS_INSUFFICIENT_RESOURCES;

    Status = ZwQueryValueKey(Handle,
                             &ValueString,
                             KeyValueFullInformation,
                             FullInformation,
                             Size,
                             &Size);
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(FullInformation);
        return Status;
    }

    *Information = FullInformation;
    return STATUS_SUCCESS;
}

RTL_GENERIC_COMPARE_RESULTS
NTAPI
PiCompareInstancePath(IN PRTL_AVL_TABLE Table,
                      IN PVOID FirstStruct,
                      IN PVOID SecondStruct)
{
    /* FIXME: TODO */
    ASSERT(FALSE);
    return 0;
}

//
//  The allocation function is called by the generic table package whenever
//  it needs to allocate memory for the table.
//

PVOID
NTAPI
PiAllocateGenericTableEntry(IN PRTL_AVL_TABLE Table,
                            IN CLONG ByteSize)
{
    /* FIXME: TODO */
    ASSERT(FALSE);
    return NULL;
}

VOID
NTAPI
PiFreeGenericTableEntry(IN PRTL_AVL_TABLE Table,
                        IN PVOID Buffer)
{
    /* FIXME: TODO */
    ASSERT(FALSE);
}

VOID
NTAPI
PpInitializeDeviceReferenceTable(VOID)
{
    /* Setup the guarded mutex and AVL table */
    KeInitializeGuardedMutex(&PpDeviceReferenceTableLock);
    RtlInitializeGenericTableAvl(
        &PpDeviceReferenceTable,
        (PRTL_AVL_COMPARE_ROUTINE)PiCompareInstancePath,
        (PRTL_AVL_ALLOCATE_ROUTINE)PiAllocateGenericTableEntry,
        (PRTL_AVL_FREE_ROUTINE)PiFreeGenericTableEntry,
        NULL);
}

BOOLEAN
NTAPI
PiInitPhase0(VOID)
{
    /* Initialize the resource when accessing device registry data */
    ExInitializeResourceLite(&PpRegistryDeviceResource);

    /* Setup the device reference AVL table */
    PpInitializeDeviceReferenceTable();
    return TRUE;
}

BOOLEAN
NTAPI
PpInitSystem(VOID)
{
    /* Check the initialization phase */
    switch (ExpInitializationPhase)
    {
    case 0:

        /* Do Phase 0 */
        return PiInitPhase0();

    case 1:

        /* Do Phase 1 */
        return TRUE;
        //return PiInitPhase1();

    default:

        /* Don't know any other phase! Bugcheck! */
        KeBugCheck(UNEXPECTED_INITIALIZATION_CALL);
        return FALSE;
    }
}

LONG IopNumberDeviceNodes;

PDEVICE_NODE
NTAPI
PipAllocateDeviceNode(IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_NODE DeviceNode;
    PAGED_CODE();

    /* Allocate it */
    DeviceNode = ExAllocatePoolWithTag(NonPagedPool, sizeof(DEVICE_NODE), TAG_IO_DEVNODE);
    if (!DeviceNode) return DeviceNode;

    /* Statistics */
    InterlockedIncrement(&IopNumberDeviceNodes);

    /* Set it up */
    RtlZeroMemory(DeviceNode, sizeof(DEVICE_NODE));
    DeviceNode->InterfaceType = InterfaceTypeUndefined;
    DeviceNode->BusNumber = -1;
    DeviceNode->ChildInterfaceType = InterfaceTypeUndefined;
    DeviceNode->ChildBusNumber = -1;
    DeviceNode->ChildBusTypeIndex = -1;
//    KeInitializeEvent(&DeviceNode->EnumerationMutex, SynchronizationEvent, TRUE);
    InitializeListHead(&DeviceNode->DeviceArbiterList);
    InitializeListHead(&DeviceNode->DeviceTranslatorList);
    InitializeListHead(&DeviceNode->TargetDeviceNotify);
    InitializeListHead(&DeviceNode->DockInfo.ListEntry);
    InitializeListHead(&DeviceNode->PendedSetInterfaceState);

    /* Check if there is a PDO */
    if (PhysicalDeviceObject)
    {
        /* Link it and remove the init flag */
        DeviceNode->PhysicalDeviceObject = PhysicalDeviceObject;
        ((PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode = DeviceNode;
        PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    /* Return the node */
    return DeviceNode;
}

/* PUBLIC FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
PnpBusTypeGuidGet(IN USHORT Index,
                  IN LPGUID BusTypeGuid)
{
    NTSTATUS Status = STATUS_SUCCESS;

    /* Acquire the lock */
    ExAcquireFastMutex(&PnpBusTypeGuidList->Lock);

    /* Validate size */
    if (Index < PnpBusTypeGuidList->GuidCount)
    {
        /* Copy the data */
        RtlCopyMemory(BusTypeGuid, &PnpBusTypeGuidList->Guids[Index], sizeof(GUID));
    }
    else
    {
        /* Failure path */
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

    /* Release lock and return status */
    ExReleaseFastMutex(&PnpBusTypeGuidList->Lock);
    return Status;
}

NTSTATUS
NTAPI
PnpDeviceObjectToDeviceInstance(IN PDEVICE_OBJECT DeviceObject,
                                IN PHANDLE DeviceInstanceHandle,
                                IN ACCESS_MASK DesiredAccess)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    PDEVICE_NODE DeviceNode;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\ENUM");
    PAGED_CODE();

    /* Open the enum key */
    Status = IopOpenRegistryKeyEx(&KeyHandle,
                                  NULL,
                                  &KeyName,
                                  KEY_READ);
    if (!NT_SUCCESS(Status)) return Status;

    /* Make sure we have an instance path */
    DeviceNode = IopGetDeviceNode(DeviceObject);
    if ((DeviceNode) && (DeviceNode->InstancePath.Length))
    {
        /* Get the instance key */
        Status = IopOpenRegistryKeyEx(DeviceInstanceHandle,
                                      KeyHandle,
                                      &DeviceNode->InstancePath,
                                      DesiredAccess);
    }
    else
    {
        /* Fail */
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Close the handle and return status */
    ZwClose(KeyHandle);
    return Status;
}

ULONG
NTAPI
PnpDetermineResourceListSize(IN PCM_RESOURCE_LIST ResourceList)
{
    ULONG FinalSize, PartialSize, EntrySize, i, j;
    PCM_FULL_RESOURCE_DESCRIPTOR FullDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;

    /* If we don't have one, that's easy */
    if (!ResourceList) return 0;

    /* Start with the minimum size possible */
    FinalSize = FIELD_OFFSET(CM_RESOURCE_LIST, List);

    /* Loop each full descriptor */
    FullDescriptor = ResourceList->List;
    for (i = 0; i < ResourceList->Count; i++)
    {
        /* Start with the minimum size possible */
        PartialSize = FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR, PartialResourceList) +
        FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors);

        /* Loop each partial descriptor */
        PartialDescriptor = FullDescriptor->PartialResourceList.PartialDescriptors;
        for (j = 0; j < FullDescriptor->PartialResourceList.Count; j++)
        {
            /* Start with the minimum size possible */
            EntrySize = sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

            /* Check if there is extra data */
            if (PartialDescriptor->Type == CmResourceTypeDeviceSpecific)
            {
                /* Add that data */
                EntrySize += PartialDescriptor->u.DeviceSpecificData.DataSize;
            }

            /* The size of partial descriptors is bigger */
            PartialSize += EntrySize;

            /* Go to the next partial descriptor */
            PartialDescriptor = (PVOID)((ULONG_PTR)PartialDescriptor + EntrySize);
        }

        /* The size of full descriptors is bigger */
        FinalSize += PartialSize;

        /* Go to the next full descriptor */
        FullDescriptor = (PVOID)((ULONG_PTR)FullDescriptor + PartialSize);
    }

    /* Return the final size */
    return FinalSize;
}

NTSTATUS
NTAPI
PiGetDeviceRegistryProperty(IN PDEVICE_OBJECT DeviceObject,
                            IN ULONG ValueType,
                            IN PWSTR ValueName,
                            IN PWSTR KeyName,
                            OUT PVOID Buffer,
                            IN PULONG BufferLength)
{
    NTSTATUS Status;
    HANDLE KeyHandle, SubHandle;
    UNICODE_STRING KeyString;
    PKEY_VALUE_FULL_INFORMATION KeyValueInfo = NULL;
    ULONG Length;
    PAGED_CODE();

    /* Find the instance key */
    Status = PnpDeviceObjectToDeviceInstance(DeviceObject, &KeyHandle, KEY_READ);
    if (NT_SUCCESS(Status))
    {
        /* Check for name given by caller */
        if (KeyName)
        {
            /* Open this key */
            RtlInitUnicodeString(&KeyString, KeyName);
            Status = IopOpenRegistryKeyEx(&SubHandle,
                                          KeyHandle,
                                          &KeyString,
                                          KEY_READ);
            if (NT_SUCCESS(Status))
            {
                /* And use this handle instead */
                ZwClose(KeyHandle);
                KeyHandle = SubHandle;
            }
        }

        /* Check if sub-key handle succeeded (or no-op if no key name given) */
        if (NT_SUCCESS(Status))
        {
            /* Now get the size of the property */
            Status = IopGetRegistryValue(KeyHandle,
                                         ValueName,
                                         &KeyValueInfo);
        }

        /* Close the key */
        ZwClose(KeyHandle);
    }

    /* Fail if any of the registry operations failed */
    if (!NT_SUCCESS(Status)) return Status;

    /* Check how much data we have to copy */
    Length = KeyValueInfo->DataLength;
    if (*BufferLength >= Length)
    {
        /* Check for a match in the value type */
        if (KeyValueInfo->Type == ValueType)
        {
            /* Copy the data */
            RtlCopyMemory(Buffer,
                          (PVOID)((ULONG_PTR)KeyValueInfo +
                          KeyValueInfo->DataOffset),
                          Length);
        }
        else
        {
            /* Invalid registry property type, fail */
           Status = STATUS_INVALID_PARAMETER_2;
        }
    }
    else
    {
        /* Buffer is too small to hold data */
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    /* Return the required buffer length, free the buffer, and return status */
    *BufferLength = Length;
    ExFreePool(KeyValueInfo);
    return Status;
}

#define PIP_RETURN_DATA(x, y)   {ReturnLength = x; Data = y; Status = STATUS_SUCCESS; break;}
#define PIP_REGISTRY_DATA(x, y) {ValueName = x; ValueType = y; break;}
#define PIP_UNIMPLEMENTED()     {UNIMPLEMENTED_DBGBREAK(); break;}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoGetDeviceProperty(IN PDEVICE_OBJECT DeviceObject,
                    IN DEVICE_REGISTRY_PROPERTY DeviceProperty,
                    IN ULONG BufferLength,
                    OUT PVOID PropertyBuffer,
                    OUT PULONG ResultLength)
{
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(DeviceObject);
    DEVICE_CAPABILITIES DeviceCaps;
    ULONG ReturnLength = 0, Length = 0, ValueType;
    PWCHAR ValueName = NULL, EnumeratorNameEnd, DeviceInstanceName;
    PVOID Data = NULL;
    NTSTATUS Status = STATUS_BUFFER_TOO_SMALL;
    GUID BusTypeGuid;
    POBJECT_NAME_INFORMATION ObjectNameInfo = NULL;
    BOOLEAN NullTerminate = FALSE;

    DPRINT("IoGetDeviceProperty(0x%p %d)\n", DeviceObject, DeviceProperty);

    /* Assume failure */
    *ResultLength = 0;

    /* Only PDOs can call this */
    if (!DeviceNode) return STATUS_INVALID_DEVICE_REQUEST;

    /* Handle all properties */
    switch (DeviceProperty)
    {
        case DevicePropertyBusTypeGuid:

            /* Get the GUID from the internal cache */
            Status = PnpBusTypeGuidGet(DeviceNode->ChildBusTypeIndex, &BusTypeGuid);
            if (!NT_SUCCESS(Status)) return Status;

            /* This is the format of the returned data */
            PIP_RETURN_DATA(sizeof(GUID), &BusTypeGuid);

        case DevicePropertyLegacyBusType:

            /* Validate correct interface type */
            if (DeviceNode->ChildInterfaceType == InterfaceTypeUndefined)
                return STATUS_OBJECT_NAME_NOT_FOUND;

            /* This is the format of the returned data */
            PIP_RETURN_DATA(sizeof(INTERFACE_TYPE), &DeviceNode->ChildInterfaceType);

        case DevicePropertyBusNumber:

            /* Validate correct bus number */
            if ((DeviceNode->ChildBusNumber & 0x80000000) == 0x80000000)
                return STATUS_OBJECT_NAME_NOT_FOUND;

            /* This is the format of the returned data */
            PIP_RETURN_DATA(sizeof(ULONG), &DeviceNode->ChildBusNumber);

        case DevicePropertyEnumeratorName:

            /* Get the instance path */
            DeviceInstanceName = DeviceNode->InstancePath.Buffer;

            /* Sanity checks */
            ASSERT((BufferLength & 1) == 0);
            ASSERT(DeviceInstanceName != NULL);

            /* Get the name from the path */
            EnumeratorNameEnd = wcschr(DeviceInstanceName, OBJ_NAME_PATH_SEPARATOR);
            ASSERT(EnumeratorNameEnd);

            /* This string needs to be NULL-terminated */
            NullTerminate = TRUE;

            /* This is the format of the returned data */
            PIP_RETURN_DATA((ULONG)(EnumeratorNameEnd - DeviceInstanceName) * sizeof(WCHAR),
                            DeviceInstanceName);

        case DevicePropertyAddress:

            /* Query the device caps */
            Status = IopQueryDeviceCapabilities(DeviceNode, &DeviceCaps);
            if (!NT_SUCCESS(Status) || (DeviceCaps.Address == MAXULONG))
                return STATUS_OBJECT_NAME_NOT_FOUND;

            /* This is the format of the returned data */
            PIP_RETURN_DATA(sizeof(ULONG), &DeviceCaps.Address);

        case DevicePropertyBootConfigurationTranslated:

            /* Validate we have resources */
            if (!DeviceNode->BootResources)
//            if (!DeviceNode->BootResourcesTranslated) // FIXFIX: Need this field
            {
                /* No resources will still fake success, but with 0 bytes */
                *ResultLength = 0;
                return STATUS_SUCCESS;
            }

            /* This is the format of the returned data */
            PIP_RETURN_DATA(PnpDetermineResourceListSize(DeviceNode->BootResources), // FIXFIX: Should use BootResourcesTranslated
                            DeviceNode->BootResources); // FIXFIX: Should use BootResourcesTranslated

        case DevicePropertyPhysicalDeviceObjectName:

            /* Sanity check for Unicode-sized string */
            ASSERT((BufferLength & 1) == 0);

            /* Allocate name buffer */
            Length = BufferLength + sizeof(OBJECT_NAME_INFORMATION);
            ObjectNameInfo = ExAllocatePool(PagedPool, Length);
            if (!ObjectNameInfo) return STATUS_INSUFFICIENT_RESOURCES;

            /* Query the PDO name */
            Status = ObQueryNameString(DeviceObject,
                                       ObjectNameInfo,
                                       Length,
                                       ResultLength);
            if (Status == STATUS_INFO_LENGTH_MISMATCH)
            {
                /* It's up to the caller to try again */
                Status = STATUS_BUFFER_TOO_SMALL;
            }

            /* This string needs to be NULL-terminated */
            NullTerminate = TRUE;

            /* Return if successful */
            if (NT_SUCCESS(Status)) PIP_RETURN_DATA(ObjectNameInfo->Name.Length,
                                                    ObjectNameInfo->Name.Buffer);

            /* Let the caller know how big the name is */
            *ResultLength -= sizeof(OBJECT_NAME_INFORMATION);
            break;

        /* Handle the registry-based properties */
        case DevicePropertyUINumber:
            PIP_REGISTRY_DATA(REGSTR_VAL_UI_NUMBER, REG_DWORD);
        case DevicePropertyLocationInformation:
            PIP_REGISTRY_DATA(REGSTR_VAL_LOCATION_INFORMATION, REG_SZ);
        case DevicePropertyDeviceDescription:
            PIP_REGISTRY_DATA(REGSTR_VAL_DEVDESC, REG_SZ);
        case DevicePropertyHardwareID:
            PIP_REGISTRY_DATA(REGSTR_VAL_HARDWAREID, REG_MULTI_SZ);
        case DevicePropertyCompatibleIDs:
            PIP_REGISTRY_DATA(REGSTR_VAL_COMPATIBLEIDS, REG_MULTI_SZ);
        case DevicePropertyBootConfiguration:
            PIP_REGISTRY_DATA(REGSTR_VAL_BOOTCONFIG, REG_RESOURCE_LIST);
        case DevicePropertyClassName:
            PIP_REGISTRY_DATA(REGSTR_VAL_CLASS, REG_SZ);
        case DevicePropertyClassGuid:
            PIP_REGISTRY_DATA(REGSTR_VAL_CLASSGUID, REG_SZ);
        case DevicePropertyDriverKeyName:
            PIP_REGISTRY_DATA(REGSTR_VAL_DRIVER, REG_SZ);
        case DevicePropertyManufacturer:
            PIP_REGISTRY_DATA(REGSTR_VAL_MFG, REG_SZ);
        case DevicePropertyFriendlyName:
            PIP_REGISTRY_DATA(REGSTR_VAL_FRIENDLYNAME, REG_SZ);
        case DevicePropertyContainerID:
            //PIP_REGISTRY_DATA(REGSTR_VAL_CONTAINERID, REG_SZ); // Win7
            PIP_UNIMPLEMENTED();
        case DevicePropertyRemovalPolicy:
            PIP_UNIMPLEMENTED();
            break;
        case DevicePropertyInstallState:
            PIP_REGISTRY_DATA(REGSTR_VAL_CONFIGFLAGS, REG_DWORD);
            break;
        case DevicePropertyResourceRequirements:
            PIP_UNIMPLEMENTED();
        case DevicePropertyAllocatedResources:
            PIP_UNIMPLEMENTED();
        default:
            return STATUS_INVALID_PARAMETER_2;
    }

    /* Having a registry value name implies registry data */
    if (ValueName)
    {
        /* We know up-front how much data to expect */
        *ResultLength = BufferLength;

        /* Go get the data, use the LogConf subkey if necessary */
        Status = PiGetDeviceRegistryProperty(DeviceObject,
                                             ValueType,
                                             ValueName,
                                             (DeviceProperty ==
                                              DevicePropertyBootConfiguration) ?
                                             L"LogConf":  NULL,
                                             PropertyBuffer,
                                             ResultLength);
    }
    else if (NT_SUCCESS(Status))
    {
        /* We know up-front how much data to expect, check the caller's buffer */
        *ResultLength = ReturnLength + (NullTerminate ? sizeof(UNICODE_NULL) : 0);
        if (*ResultLength <= BufferLength)
        {
            /* Buffer is all good, copy the data */
            RtlCopyMemory(PropertyBuffer, Data, ReturnLength);

            /* Check if we need to NULL-terminate the string */
            if (NullTerminate)
            {
                /* Terminate the string */
                ((PWCHAR)PropertyBuffer)[ReturnLength / sizeof(WCHAR)] = UNICODE_NULL;
            }

            /* This is the success path */
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* Failure path */
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }

    /* Free any allocation we may have made, and return the status code */
    if (ObjectNameInfo) ExFreePool(ObjectNameInfo);
    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
IoInvalidateDeviceState(IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(PhysicalDeviceObject);
    IO_STACK_LOCATION Stack;
    ULONG PnPFlags;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    RtlZeroMemory(&Stack, sizeof(IO_STACK_LOCATION));
    Stack.MajorFunction = IRP_MJ_PNP;
    Stack.MinorFunction = IRP_MN_QUERY_PNP_DEVICE_STATE;

    Status = IopSynchronousCall(PhysicalDeviceObject, &Stack, (PVOID*)&PnPFlags);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_NOT_SUPPORTED)
        {
            DPRINT1("IRP_MN_QUERY_PNP_DEVICE_STATE failed with status 0x%lx\n", Status);
        }
        return;
    }

    if (PnPFlags & PNP_DEVICE_NOT_DISABLEABLE)
        DeviceNode->UserFlags |= DNUF_NOT_DISABLEABLE;
    else
        DeviceNode->UserFlags &= ~DNUF_NOT_DISABLEABLE;

    if (PnPFlags & PNP_DEVICE_DONT_DISPLAY_IN_UI)
        DeviceNode->UserFlags |= DNUF_DONT_SHOW_IN_UI;
    else
        DeviceNode->UserFlags &= ~DNUF_DONT_SHOW_IN_UI;

    if ((PnPFlags & PNP_DEVICE_REMOVED) ||
        ((PnPFlags & PNP_DEVICE_FAILED) && !(PnPFlags & PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED)))
    {
        /* Flag it if it's failed */
        if (PnPFlags & PNP_DEVICE_FAILED) DeviceNode->Problem = CM_PROB_FAILED_POST_START;

        /* Send removal IRPs to all of its children */
        IopPrepareDeviceForRemoval(PhysicalDeviceObject, TRUE);

        /* Send surprise removal */
        IopSendSurpriseRemoval(PhysicalDeviceObject);

        /* Tell the user-mode PnP manager that a device was removed */
        IopQueueTargetDeviceEvent(&GUID_DEVICE_SURPRISE_REMOVAL,
                                  &DeviceNode->InstancePath);

        IopSendRemoveDevice(PhysicalDeviceObject);
    }
    else if ((PnPFlags & PNP_DEVICE_FAILED) && (PnPFlags & PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED))
    {
        /* Stop for resource rebalance */
        Status = IopStopDevice(DeviceNode);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to stop device for rebalancing\n");

            /* Stop failed so don't rebalance */
            PnPFlags &= ~PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED;
        }
    }

    /* Resource rebalance */
    if (PnPFlags & PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED)
    {
        DPRINT("Sending IRP_MN_QUERY_RESOURCES to device stack\n");

        Status = IopInitiatePnpIrp(PhysicalDeviceObject,
                                   &IoStatusBlock,
                                   IRP_MN_QUERY_RESOURCES,
                                   NULL);
        if (NT_SUCCESS(Status) && IoStatusBlock.Information)
        {
            DeviceNode->BootResources =
            (PCM_RESOURCE_LIST)IoStatusBlock.Information;
            IopDeviceNodeSetFlag(DeviceNode, DNF_HAS_BOOT_CONFIG);
        }
        else
        {
            DPRINT("IopInitiatePnpIrp() failed (Status %x) or IoStatusBlock.Information=NULL\n", Status);
            DeviceNode->BootResources = NULL;
        }

        DPRINT("Sending IRP_MN_QUERY_RESOURCE_REQUIREMENTS to device stack\n");

        Status = IopInitiatePnpIrp(PhysicalDeviceObject,
                                   &IoStatusBlock,
                                   IRP_MN_QUERY_RESOURCE_REQUIREMENTS,
                                   NULL);
        if (NT_SUCCESS(Status))
        {
            DeviceNode->ResourceRequirements =
            (PIO_RESOURCE_REQUIREMENTS_LIST)IoStatusBlock.Information;
        }
        else
        {
            DPRINT("IopInitiatePnpIrp() failed (Status %08lx)\n", Status);
            DeviceNode->ResourceRequirements = NULL;
        }

        /* IRP_MN_FILTER_RESOURCE_REQUIREMENTS is called indirectly by IopStartDevice */
        if (IopStartDevice(DeviceNode) != STATUS_SUCCESS)
        {
            DPRINT1("Restart after resource rebalance failed\n");

            DeviceNode->Flags &= ~(DNF_STARTED | DNF_START_REQUEST_PENDING);
            DeviceNode->Flags |= DNF_START_FAILED;

            IopRemoveDevice(DeviceNode);
        }
    }
}

/**
 * @name IoOpenDeviceRegistryKey
 *
 * Open a registry key unique for a specified driver or device instance.
 *
 * @param DeviceObject   Device to get the registry key for.
 * @param DevInstKeyType Type of the key to return.
 * @param DesiredAccess  Access mask (eg. KEY_READ | KEY_WRITE).
 * @param DevInstRegKey  Handle to the opened registry key on
 *                       successful return.
 *
 * @return Status.
 *
 * @implemented
 */
NTSTATUS
NTAPI
IoOpenDeviceRegistryKey(IN PDEVICE_OBJECT DeviceObject,
                        IN ULONG DevInstKeyType,
                        IN ACCESS_MASK DesiredAccess,
                        OUT PHANDLE DevInstRegKey)
{
   static WCHAR RootKeyName[] =
      L"\\Registry\\Machine\\System\\CurrentControlSet\\";
   static WCHAR ProfileKeyName[] =
      L"Hardware Profiles\\Current\\System\\CurrentControlSet\\";
   static WCHAR ClassKeyName[] = L"Control\\Class\\";
   static WCHAR EnumKeyName[] = L"Enum\\";
   static WCHAR DeviceParametersKeyName[] = L"Device Parameters";
   ULONG KeyNameLength;
   PWSTR KeyNameBuffer;
   UNICODE_STRING KeyName;
   ULONG DriverKeyLength;
   OBJECT_ATTRIBUTES ObjectAttributes;
   PDEVICE_NODE DeviceNode = NULL;
   NTSTATUS Status;

   DPRINT("IoOpenDeviceRegistryKey() called\n");

   if ((DevInstKeyType & (PLUGPLAY_REGKEY_DEVICE | PLUGPLAY_REGKEY_DRIVER)) == 0)
   {
       DPRINT1("IoOpenDeviceRegistryKey(): got wrong params, exiting... \n");
       return STATUS_INVALID_PARAMETER;
   }

   if (!IopIsValidPhysicalDeviceObject(DeviceObject))
       return STATUS_INVALID_DEVICE_REQUEST;
   DeviceNode = IopGetDeviceNode(DeviceObject);

   /*
    * Calculate the length of the base key name. This is the full
    * name for driver key or the name excluding "Device Parameters"
    * subkey for device key.
    */

   KeyNameLength = sizeof(RootKeyName);
   if (DevInstKeyType & PLUGPLAY_REGKEY_CURRENT_HWPROFILE)
      KeyNameLength += sizeof(ProfileKeyName) - sizeof(UNICODE_NULL);
   if (DevInstKeyType & PLUGPLAY_REGKEY_DRIVER)
   {
      KeyNameLength += sizeof(ClassKeyName) - sizeof(UNICODE_NULL);
      Status = IoGetDeviceProperty(DeviceObject, DevicePropertyDriverKeyName,
                                   0, NULL, &DriverKeyLength);
      if (Status != STATUS_BUFFER_TOO_SMALL)
         return Status;
      KeyNameLength += DriverKeyLength;
   }
   else
   {
      KeyNameLength += sizeof(EnumKeyName) - sizeof(UNICODE_NULL) +
                       DeviceNode->InstancePath.Length;
   }

   /*
    * Now allocate the buffer for the key name...
    */

   KeyNameBuffer = ExAllocatePool(PagedPool, KeyNameLength);
   if (KeyNameBuffer == NULL)
      return STATUS_INSUFFICIENT_RESOURCES;

   KeyName.Length = 0;
   KeyName.MaximumLength = (USHORT)KeyNameLength;
   KeyName.Buffer = KeyNameBuffer;

   /*
    * ...and build the key name.
    */

   KeyName.Length += sizeof(RootKeyName) - sizeof(UNICODE_NULL);
   RtlCopyMemory(KeyNameBuffer, RootKeyName, KeyName.Length);

   if (DevInstKeyType & PLUGPLAY_REGKEY_CURRENT_HWPROFILE)
      RtlAppendUnicodeToString(&KeyName, ProfileKeyName);

   if (DevInstKeyType & PLUGPLAY_REGKEY_DRIVER)
   {
      RtlAppendUnicodeToString(&KeyName, ClassKeyName);
      Status = IoGetDeviceProperty(DeviceObject, DevicePropertyDriverKeyName,
                                   DriverKeyLength, KeyNameBuffer +
                                   (KeyName.Length / sizeof(WCHAR)),
                                   &DriverKeyLength);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Call to IoGetDeviceProperty() failed with Status 0x%08lx\n", Status);
         ExFreePool(KeyNameBuffer);
         return Status;
      }
      KeyName.Length += (USHORT)DriverKeyLength - sizeof(UNICODE_NULL);
   }
   else
   {
      RtlAppendUnicodeToString(&KeyName, EnumKeyName);
      Status = RtlAppendUnicodeStringToString(&KeyName, &DeviceNode->InstancePath);
      if (DeviceNode->InstancePath.Length == 0)
      {
         ExFreePool(KeyNameBuffer);
         return Status;
      }
   }

   /*
    * Open the base key.
    */
   Status = IopOpenRegistryKeyEx(DevInstRegKey, NULL, &KeyName, DesiredAccess);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("IoOpenDeviceRegistryKey(%wZ): Base key doesn't exist, exiting... (Status 0x%08lx)\n", &KeyName, Status);
      ExFreePool(KeyNameBuffer);
      return Status;
   }
   ExFreePool(KeyNameBuffer);

   /*
    * For driver key we're done now.
    */

   if (DevInstKeyType & PLUGPLAY_REGKEY_DRIVER)
      return Status;

   /*
    * Let's go further. For device key we must open "Device Parameters"
    * subkey and create it if it doesn't exist yet.
    */

   RtlInitUnicodeString(&KeyName, DeviceParametersKeyName);
   InitializeObjectAttributes(&ObjectAttributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                              *DevInstRegKey,
                              NULL);
   Status = ZwCreateKey(DevInstRegKey,
                        DesiredAccess,
                        &ObjectAttributes,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        NULL);
   ZwClose(ObjectAttributes.RootDirectory);

   return Status;
}

static
NTSTATUS
IopQueryRemoveChildDevices(PDEVICE_NODE ParentDeviceNode, BOOLEAN Force)
{
    PDEVICE_NODE ChildDeviceNode, NextDeviceNode, FailedRemoveDevice;
    NTSTATUS Status;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
    ChildDeviceNode = ParentDeviceNode->Child;
    while (ChildDeviceNode != NULL)
    {
        NextDeviceNode = ChildDeviceNode->Sibling;
        KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

        Status = IopPrepareDeviceForRemoval(ChildDeviceNode->PhysicalDeviceObject, Force);
        if (!NT_SUCCESS(Status))
        {
            FailedRemoveDevice = ChildDeviceNode;
            goto cleanup;
        }

        KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
        ChildDeviceNode = NextDeviceNode;
    }
    KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

    return STATUS_SUCCESS;

cleanup:
    KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
    ChildDeviceNode = ParentDeviceNode->Child;
    while (ChildDeviceNode != NULL)
    {
        NextDeviceNode = ChildDeviceNode->Sibling;
        KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

        IopCancelPrepareDeviceForRemoval(ChildDeviceNode->PhysicalDeviceObject);

        /* IRP_MN_CANCEL_REMOVE_DEVICE is also sent to the device
         * that failed the IRP_MN_QUERY_REMOVE_DEVICE request */
        if (ChildDeviceNode == FailedRemoveDevice)
            return Status;

        ChildDeviceNode = NextDeviceNode;

        KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
    }
    KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

    return Status;
}

static
VOID
IopSendRemoveChildDevices(PDEVICE_NODE ParentDeviceNode)
{
    PDEVICE_NODE ChildDeviceNode, NextDeviceNode;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
    ChildDeviceNode = ParentDeviceNode->Child;
    while (ChildDeviceNode != NULL)
    {
        NextDeviceNode = ChildDeviceNode->Sibling;
        KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

        IopSendRemoveDevice(ChildDeviceNode->PhysicalDeviceObject);

        ChildDeviceNode = NextDeviceNode;

        KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
    }
    KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);
}

static
VOID
IopCancelRemoveChildDevices(PDEVICE_NODE ParentDeviceNode)
{
    PDEVICE_NODE ChildDeviceNode, NextDeviceNode;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
    ChildDeviceNode = ParentDeviceNode->Child;
    while (ChildDeviceNode != NULL)
    {
        NextDeviceNode = ChildDeviceNode->Sibling;
        KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

        IopCancelPrepareDeviceForRemoval(ChildDeviceNode->PhysicalDeviceObject);

        ChildDeviceNode = NextDeviceNode;

        KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
    }
    KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);
}

static
NTSTATUS
IopQueryRemoveDeviceRelations(PDEVICE_RELATIONS DeviceRelations, BOOLEAN Force)
{
    /* This function DOES NOT dereference the device objects on SUCCESS
     * but it DOES dereference device objects on FAILURE */

    ULONG i, j;
    NTSTATUS Status;

    for (i = 0; i < DeviceRelations->Count; i++)
    {
        Status = IopPrepareDeviceForRemoval(DeviceRelations->Objects[i], Force);
        if (!NT_SUCCESS(Status))
        {
            j = i;
            goto cleanup;
        }
    }

    return STATUS_SUCCESS;

cleanup:
    /* IRP_MN_CANCEL_REMOVE_DEVICE is also sent to the device
     * that failed the IRP_MN_QUERY_REMOVE_DEVICE request */
    for (i = 0; i <= j; i++)
    {
        IopCancelPrepareDeviceForRemoval(DeviceRelations->Objects[i]);
        ObDereferenceObject(DeviceRelations->Objects[i]);
        DeviceRelations->Objects[i] = NULL;
    }
    for (; i < DeviceRelations->Count; i++)
    {
        ObDereferenceObject(DeviceRelations->Objects[i]);
        DeviceRelations->Objects[i] = NULL;
    }
    ExFreePool(DeviceRelations);

    return Status;
}

static
VOID
IopSendRemoveDeviceRelations(PDEVICE_RELATIONS DeviceRelations)
{
    /* This function DOES dereference the device objects in all cases */

    ULONG i;

    for (i = 0; i < DeviceRelations->Count; i++)
    {
        IopSendRemoveDevice(DeviceRelations->Objects[i]);
        DeviceRelations->Objects[i] = NULL;
    }

    ExFreePool(DeviceRelations);
}

static
VOID
IopCancelRemoveDeviceRelations(PDEVICE_RELATIONS DeviceRelations)
{
    /* This function DOES dereference the device objects in all cases */

    ULONG i;

    for (i = 0; i < DeviceRelations->Count; i++)
    {
        IopCancelPrepareDeviceForRemoval(DeviceRelations->Objects[i]);
        ObDereferenceObject(DeviceRelations->Objects[i]);
        DeviceRelations->Objects[i] = NULL;
    }

    ExFreePool(DeviceRelations);
}

VOID
IopCancelPrepareDeviceForRemoval(PDEVICE_OBJECT DeviceObject)
{
    IO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatusBlock;
    PDEVICE_RELATIONS DeviceRelations;
    NTSTATUS Status;

    IopCancelRemoveDevice(DeviceObject);

    Stack.Parameters.QueryDeviceRelations.Type = RemovalRelations;

    Status = IopInitiatePnpIrp(DeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_DEVICE_RELATIONS,
                               &Stack);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopInitiatePnpIrp() failed with status 0x%08lx\n", Status);
        DeviceRelations = NULL;
    }
    else
    {
        DeviceRelations = (PDEVICE_RELATIONS)IoStatusBlock.Information;
    }

    if (DeviceRelations)
        IopCancelRemoveDeviceRelations(DeviceRelations);
}

NTSTATUS
IopPrepareDeviceForRemoval(IN PDEVICE_OBJECT DeviceObject, BOOLEAN Force)
{
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(DeviceObject);
    IO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatusBlock;
    PDEVICE_RELATIONS DeviceRelations;
    NTSTATUS Status;

    if ((DeviceNode->UserFlags & DNUF_NOT_DISABLEABLE) && !Force)
    {
        DPRINT1("Removal not allowed for %wZ\n", &DeviceNode->InstancePath);
        return STATUS_UNSUCCESSFUL;
    }

    if (!Force && IopQueryRemoveDevice(DeviceObject) != STATUS_SUCCESS)
    {
        DPRINT1("Removal vetoed by failing the query remove request\n");

        IopCancelRemoveDevice(DeviceObject);

        return STATUS_UNSUCCESSFUL;
    }

    Stack.Parameters.QueryDeviceRelations.Type = RemovalRelations;

    Status = IopInitiatePnpIrp(DeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_DEVICE_RELATIONS,
                               &Stack);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopInitiatePnpIrp() failed with status 0x%08lx\n", Status);
        DeviceRelations = NULL;
    }
    else
    {
        DeviceRelations = (PDEVICE_RELATIONS)IoStatusBlock.Information;
    }

    if (DeviceRelations)
    {
        Status = IopQueryRemoveDeviceRelations(DeviceRelations, Force);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    Status = IopQueryRemoveChildDevices(DeviceNode, Force);
    if (!NT_SUCCESS(Status))
    {
        if (DeviceRelations)
            IopCancelRemoveDeviceRelations(DeviceRelations);
        return Status;
    }

    if (DeviceRelations)
        IopSendRemoveDeviceRelations(DeviceRelations);
    IopSendRemoveChildDevices(DeviceNode);

    return STATUS_SUCCESS;
}

NTSTATUS
IopRemoveDevice(PDEVICE_NODE DeviceNode)
{
    NTSTATUS Status;

    DPRINT("Removing device: %wZ\n", &DeviceNode->InstancePath);

    Status = IopPrepareDeviceForRemoval(DeviceNode->PhysicalDeviceObject, FALSE);
    if (NT_SUCCESS(Status))
    {
        IopSendRemoveDevice(DeviceNode->PhysicalDeviceObject);
        IopQueueTargetDeviceEvent(&GUID_DEVICE_SAFE_REMOVAL,
                                  &DeviceNode->InstancePath);
        return STATUS_SUCCESS;
    }

    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
IoRequestDeviceEject(IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(PhysicalDeviceObject);
    PDEVICE_RELATIONS DeviceRelations;
    IO_STATUS_BLOCK IoStatusBlock;
    IO_STACK_LOCATION Stack;
    DEVICE_CAPABILITIES Capabilities;
    NTSTATUS Status;

    IopQueueTargetDeviceEvent(&GUID_DEVICE_KERNEL_INITIATED_EJECT,
                              &DeviceNode->InstancePath);

    if (IopQueryDeviceCapabilities(DeviceNode, &Capabilities) != STATUS_SUCCESS)
    {
        goto cleanup;
    }

    Stack.Parameters.QueryDeviceRelations.Type = EjectionRelations;

    Status = IopInitiatePnpIrp(PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_DEVICE_RELATIONS,
                               &Stack);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopInitiatePnpIrp() failed with status 0x%08lx\n", Status);
        DeviceRelations = NULL;
    }
    else
    {
        DeviceRelations = (PDEVICE_RELATIONS)IoStatusBlock.Information;
    }

    if (DeviceRelations)
    {
        Status = IopQueryRemoveDeviceRelations(DeviceRelations, FALSE);
        if (!NT_SUCCESS(Status))
            goto cleanup;
    }

    Status = IopQueryRemoveChildDevices(DeviceNode, FALSE);
    if (!NT_SUCCESS(Status))
    {
        if (DeviceRelations)
            IopCancelRemoveDeviceRelations(DeviceRelations);
        goto cleanup;
    }

    if (IopPrepareDeviceForRemoval(PhysicalDeviceObject, FALSE) != STATUS_SUCCESS)
    {
        if (DeviceRelations)
            IopCancelRemoveDeviceRelations(DeviceRelations);
        IopCancelRemoveChildDevices(DeviceNode);
        goto cleanup;
    }

    if (DeviceRelations)
        IopSendRemoveDeviceRelations(DeviceRelations);
    IopSendRemoveChildDevices(DeviceNode);

    DeviceNode->Problem = CM_PROB_HELD_FOR_EJECT;
    if (Capabilities.EjectSupported)
    {
        if (IopSendEject(PhysicalDeviceObject) != STATUS_SUCCESS)
        {
            goto cleanup;
        }
    }
    else
    {
        DeviceNode->Flags |= DNF_DISABLED;
    }

    IopQueueTargetDeviceEvent(&GUID_DEVICE_EJECT,
                              &DeviceNode->InstancePath);

    return;

cleanup:
    IopQueueTargetDeviceEvent(&GUID_DEVICE_EJECT_VETOED,
                              &DeviceNode->InstancePath);
}

/*
 * @implemented
 */
VOID
NTAPI
IoInvalidateDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN DEVICE_RELATION_TYPE Type)
{
    PINVALIDATE_DEVICE_RELATION_DATA Data;
    KIRQL OldIrql;

    Data = ExAllocatePool(NonPagedPool, sizeof(INVALIDATE_DEVICE_RELATION_DATA));
    if (!Data)
        return;

    ObReferenceObject(DeviceObject);
    Data->DeviceObject = DeviceObject;
    Data->Type = Type;

    KeAcquireSpinLock(&IopDeviceRelationsSpinLock, &OldIrql);
    InsertTailList(&IopDeviceRelationsRequestList, &Data->RequestListEntry);
    if (IopDeviceRelationsRequestInProgress)
    {
        KeReleaseSpinLock(&IopDeviceRelationsSpinLock, OldIrql);
        return;
    }
    IopDeviceRelationsRequestInProgress = TRUE;
    KeReleaseSpinLock(&IopDeviceRelationsSpinLock, OldIrql);

    ExInitializeWorkItem(&IopDeviceRelationsWorkItem,
                         IopDeviceRelationsWorker,
                         NULL);
    ExQueueWorkItem(&IopDeviceRelationsWorkItem,
                    DelayedWorkQueue);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoSynchronousInvalidateDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN DEVICE_RELATION_TYPE Type)
{
    PAGED_CODE();

    switch (Type)
    {
        case BusRelations:
            /* Enumerate the device */
            return IopEnumerateDevice(DeviceObject);
        case PowerRelations:
             /* Not handled yet */
             return STATUS_NOT_IMPLEMENTED;
        case TargetDeviceRelation:
            /* Nothing to do */
            return STATUS_SUCCESS;
        default:
            /* Ejection relations are not supported */
            return STATUS_NOT_SUPPORTED;
    }
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IoTranslateBusAddress(IN INTERFACE_TYPE InterfaceType,
                      IN ULONG BusNumber,
                      IN PHYSICAL_ADDRESS BusAddress,
                      IN OUT PULONG AddressSpace,
                      OUT PPHYSICAL_ADDRESS TranslatedAddress)
{
    /* FIXME: Notify the resource arbiter */

    return HalTranslateBusAddress(InterfaceType,
                                  BusNumber,
                                  BusAddress,
                                  AddressSpace,
                                  TranslatedAddress);
}
