/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PnP manager device manipulation functions
 * COPYRIGHT:   Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              2007 Hervé Poussineau (hpoussin@reactos.org)
 *              2014-2017 Thomas Faber (thomas.faber@reactos.org)
 *              2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

/* Device tree is a resource shared among all system services: hal, kernel, drivers etc.
 * Thus all code which interacts with the tree needs to be synchronized.
 * Here it's done via a list of DEVICE_ACTION_REQUEST structures, which represents
 * the device action queue. It is being processed exclusively by the PipDeviceActionWorker.
 *
 * Operation queuing can be done with the PiQueueDeviceAction function or with
 * the PiPerfomSyncDeviceAction for synchronous operations.
 * All device manipulation like starting, removing, enumeration (see DEVICE_ACTION enum)
 * have to be done with the PiQueueDeviceAction in order to avoid race conditions.
 *
 * Note: there is one special operation here - PiActionEnumRootDevices. It is meant to be done
 * during initialization process (and be the first device tree operation executed) and
 * is always executed synchronously.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern ERESOURCE IopDriverLoadResource;
extern BOOLEAN PnpSystemInit;
extern PDEVICE_NODE IopRootDeviceNode;
extern BOOLEAN PnPBootDriversLoaded;

#define MAX_DEVICE_ID_LEN          200
#define MAX_SEPARATORS_INSTANCEID  0
#define MAX_SEPARATORS_DEVICEID    1

/* DATA **********************************************************************/

LIST_ENTRY IopDeviceActionRequestList;
WORK_QUEUE_ITEM IopDeviceActionWorkItem;
BOOLEAN IopDeviceActionInProgress;
KSPIN_LOCK IopDeviceActionLock;
KEVENT PiEnumerationFinished;

/* TYPES *********************************************************************/

typedef struct _DEVICE_ACTION_REQUEST
{
    LIST_ENTRY RequestListEntry;
    PDEVICE_OBJECT DeviceObject;
    PKEVENT CompletionEvent;
    NTSTATUS *CompletionStatus;
    DEVICE_ACTION Action;
} DEVICE_ACTION_REQUEST, *PDEVICE_ACTION_REQUEST;

/* FUNCTIONS *****************************************************************/

PDEVICE_OBJECT
IopGetDeviceObjectFromDeviceInstance(PUNICODE_STRING DeviceInstance);

NTSTATUS
IopGetParentIdPrefix(PDEVICE_NODE DeviceNode, PUNICODE_STRING ParentIdPrefix);

USHORT
NTAPI
IopGetBusTypeGuidIndex(LPGUID BusTypeGuid);

NTSTATUS
IopSetDeviceInstanceData(HANDLE InstanceKey, PDEVICE_NODE DeviceNode);

VOID
NTAPI
IopInstallCriticalDevice(PDEVICE_NODE DeviceNode);

static
VOID
IopCancelPrepareDeviceForRemoval(PDEVICE_OBJECT DeviceObject);

static
NTSTATUS
IopPrepareDeviceForRemoval(PDEVICE_OBJECT DeviceObject, BOOLEAN Force);

static
BOOLEAN
IopValidateID(
    _In_ PWCHAR Id,
    _In_ BUS_QUERY_ID_TYPE QueryType)
{
    PWCHAR PtrChar;
    PWCHAR StringEnd;
    WCHAR Char;
    ULONG SeparatorsCount = 0;
    PWCHAR PtrPrevChar = NULL;
    ULONG MaxSeparators;
    BOOLEAN IsMultiSz;

    PAGED_CODE();

    switch (QueryType)
    {
        case BusQueryDeviceID:
            MaxSeparators = MAX_SEPARATORS_DEVICEID;
            IsMultiSz = FALSE;
            break;
        case BusQueryInstanceID:
            MaxSeparators = MAX_SEPARATORS_INSTANCEID;
            IsMultiSz = FALSE;
            break;

        case BusQueryHardwareIDs:
        case BusQueryCompatibleIDs:
            MaxSeparators = MAX_SEPARATORS_DEVICEID;
            IsMultiSz = TRUE;
            break;

        default:
            DPRINT1("IopValidateID: Not handled QueryType - %x\n", QueryType);
            return FALSE;
    }

    StringEnd = Id + MAX_DEVICE_ID_LEN;

    for (PtrChar = Id; PtrChar < StringEnd; PtrChar++)
    {
        Char = *PtrChar;

        if (Char == UNICODE_NULL)
        {
            if (!IsMultiSz || (PtrPrevChar && PtrChar == PtrPrevChar + 1))
            {
                if (MaxSeparators == SeparatorsCount || IsMultiSz)
                {
                    return TRUE;
                }

                DPRINT1("IopValidateID: SeparatorsCount - %lu, MaxSeparators - %lu\n",
                        SeparatorsCount, MaxSeparators);
                goto ErrorExit;
            }

            StringEnd = PtrChar + MAX_DEVICE_ID_LEN + 1;
            PtrPrevChar = PtrChar;
            SeparatorsCount = 0;
        }
        else if (Char < ' ' || Char > 0x7F || Char == ',')
        {
            DPRINT1("IopValidateID: Invalid character - %04X\n", Char);
            goto ErrorExit;
        }
        else if (Char == ' ')
        {
            *PtrChar = '_';
        }
        else if (Char == '\\')
        {
            SeparatorsCount++;

            if (SeparatorsCount > MaxSeparators)
            {
                DPRINT1("IopValidateID: SeparatorsCount - %lu, MaxSeparators - %lu\n",
                        SeparatorsCount, MaxSeparators);
                goto ErrorExit;
            }
        }
    }

    DPRINT1("IopValidateID: Not terminated ID\n");

ErrorExit:
    // FIXME logging
    return FALSE;
}

static
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
    BOOLEAN IsValidID;

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

    IsValidID = IopValidateID((PWCHAR)IoStatusBlock.Information, BusQueryDeviceID);

    if (!IsValidID)
    {
        DPRINT1("Invalid DeviceID. DeviceNode - %p\n", DeviceNode);
    }

    /* Save the device id string */
    RtlInitUnicodeString(&DeviceId, (PWSTR)IoStatusBlock.Information);

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

    if (IoStatusBlock.Information)
    {
        IsValidID = IopValidateID((PWCHAR)IoStatusBlock.Information, BusQueryInstanceID);

        if (!IsValidID)
        {
            DPRINT1("Invalid InstanceID. DeviceNode - %p\n", DeviceNode);
        }
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

    /* Map device capabilities to capability flags */
    DeviceNode->CapabilityFlags = 0;
    if (DeviceCaps->LockSupported)
        DeviceNode->CapabilityFlags |= 0x00000001;    // CM_DEVCAP_LOCKSUPPORTED

    if (DeviceCaps->EjectSupported)
        DeviceNode->CapabilityFlags |= 0x00000002;    // CM_DEVCAP_EJECTSUPPORTED

    if (DeviceCaps->Removable)
        DeviceNode->CapabilityFlags |= 0x00000004;    // CM_DEVCAP_REMOVABLE

    if (DeviceCaps->DockDevice)
        DeviceNode->CapabilityFlags |= 0x00000008;    // CM_DEVCAP_DOCKDEVICE

    if (DeviceCaps->UniqueID)
        DeviceNode->CapabilityFlags |= 0x00000010;    // CM_DEVCAP_UNIQUEID

    if (DeviceCaps->SilentInstall)
        DeviceNode->CapabilityFlags |= 0x00000020;    // CM_DEVCAP_SILENTINSTALL

    if (DeviceCaps->RawDeviceOK)
        DeviceNode->CapabilityFlags |= 0x00000040;    // CM_DEVCAP_RAWDEVICEOK

    if (DeviceCaps->SurpriseRemovalOK)
        DeviceNode->CapabilityFlags |= 0x00000080;    // CM_DEVCAP_SURPRISEREMOVALOK

    if (DeviceCaps->HardwareDisabled)
        DeviceNode->CapabilityFlags |= 0x00000100;    // CM_DEVCAP_HARDWAREDISABLED

    if (DeviceCaps->NonDynamic)
        DeviceNode->CapabilityFlags |= 0x00000200;    // CM_DEVCAP_NONDYNAMIC

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
    BOOLEAN IsValidID;

    DPRINT("Sending IRP_MN_QUERY_ID.BusQueryHardwareIDs to device stack\n");

    RtlZeroMemory(&Stack, sizeof(Stack));
    Stack.Parameters.QueryId.IdType = BusQueryHardwareIDs;
    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_ID,
                               &Stack);
    if (NT_SUCCESS(Status))
    {
        IsValidID = IopValidateID((PWCHAR)IoStatusBlock.Information, BusQueryHardwareIDs);

        if (!IsValidID)
        {
            DPRINT1("Invalid HardwareIDs. DeviceNode - %p\n", DeviceNode);
        }

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

static
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
    BOOLEAN IsValidID;

    DPRINT("Sending IRP_MN_QUERY_ID.BusQueryCompatibleIDs to device stack\n");

    RtlZeroMemory(&Stack, sizeof(Stack));
    Stack.Parameters.QueryId.IdType = BusQueryCompatibleIDs;
    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_ID,
                               &Stack);
    if (NT_SUCCESS(Status) && IoStatusBlock.Information)
    {
        IsValidID = IopValidateID((PWCHAR)IoStatusBlock.Information, BusQueryCompatibleIDs);

        if (!IsValidID)
        {
            DPRINT1("Invalid CompatibleIDs. DeviceNode - %p\n", DeviceNode);
        }

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
        UNICODE_STRING RegKey;

        /* Install the service for this if it's in the CDDB */
        IopInstallCriticalDevice(DeviceNode);

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

        RegKey.Length = 0;
        RegKey.MaximumLength = sizeof(ENUM_ROOT) + sizeof(WCHAR) + DeviceNode->InstancePath.Length;
        RegKey.Buffer = ExAllocatePoolWithTag(PagedPool,
                                              RegKey.MaximumLength,
                                              TAG_IO);
        if (RegKey.Buffer == NULL)
        {
            IopDeviceNodeSetFlag(DeviceNode, DNF_DISABLED);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlAppendUnicodeToString(&RegKey, ENUM_ROOT);
        RtlAppendUnicodeToString(&RegKey, L"\\");
        RtlAppendUnicodeStringToString(&RegKey, &DeviceNode->InstancePath);

        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
            RegKey.Buffer, QueryTable, NULL, NULL);
        ExFreePoolWithTag(RegKey.Buffer, TAG_IO);

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
                RtlInitEmptyUnicodeString(&DeviceNode->ServiceName, NULL, 0);
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
                if (!NT_SUCCESS(Status))
                    DeviceNode->Problem = CM_PROB_FAILED_DRIVER_ENTRY;
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
                if (!BootDrivers)
                    DeviceNode->Problem = CM_PROB_DRIVER_FAILED_LOAD;
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

static
NTSTATUS
IopSetServiceEnumData(PDEVICE_NODE DeviceNode)
{
    UNICODE_STRING ServicesKeyPath = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    UNICODE_STRING ServiceKeyName;
    UNICODE_STRING EnumKeyName;
    UNICODE_STRING ValueName;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    HANDLE ServiceKey = NULL, ServiceEnumKey = NULL;
    ULONG Disposition;
    ULONG Count = 0, NextInstance = 0;
    WCHAR ValueBuffer[6];
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("IopSetServiceEnumData(%p)\n", DeviceNode);
    DPRINT("Instance: %wZ\n", &DeviceNode->InstancePath);
    DPRINT("Service: %wZ\n", &DeviceNode->ServiceName);

    if (DeviceNode->ServiceName.Buffer == NULL)
    {
        DPRINT1("No service!\n");
        return STATUS_SUCCESS;
    }

    ServiceKeyName.MaximumLength = ServicesKeyPath.Length + DeviceNode->ServiceName.Length + sizeof(UNICODE_NULL);
    ServiceKeyName.Length = 0;
    ServiceKeyName.Buffer = ExAllocatePool(PagedPool, ServiceKeyName.MaximumLength);
    if (ServiceKeyName.Buffer == NULL)
    {
        DPRINT1("No ServiceKeyName.Buffer!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlAppendUnicodeStringToString(&ServiceKeyName, &ServicesKeyPath);
    RtlAppendUnicodeStringToString(&ServiceKeyName, &DeviceNode->ServiceName);

    DPRINT("ServiceKeyName: %wZ\n", &ServiceKeyName);

    Status = IopOpenRegistryKeyEx(&ServiceKey, NULL, &ServiceKeyName, KEY_CREATE_SUB_KEY);
    if (!NT_SUCCESS(Status))
    {
        goto done;
    }

    RtlInitUnicodeString(&EnumKeyName, L"Enum");
    Status = IopCreateRegistryKeyEx(&ServiceEnumKey,
                                    ServiceKey,
                                    &EnumKeyName,
                                    KEY_SET_VALUE,
                                    REG_OPTION_VOLATILE,
                                    &Disposition);
    if (NT_SUCCESS(Status))
    {
        if (Disposition == REG_OPENED_EXISTING_KEY)
        {
            /* Read the NextInstance value */
            Status = IopGetRegistryValue(ServiceEnumKey,
                                         L"Count",
                                         &KeyValueInformation);
            if (!NT_SUCCESS(Status))
                goto done;

            if ((KeyValueInformation->Type == REG_DWORD) &&
                (KeyValueInformation->DataLength))
            {
                /* Read it */
                Count = *(PULONG)((ULONG_PTR)KeyValueInformation +
                                  KeyValueInformation->DataOffset);
            }

            ExFreePool(KeyValueInformation);
            KeyValueInformation = NULL;

            /* Read the NextInstance value */
            Status = IopGetRegistryValue(ServiceEnumKey,
                                         L"NextInstance",
                                         &KeyValueInformation);
            if (!NT_SUCCESS(Status))
                goto done;

            if ((KeyValueInformation->Type == REG_DWORD) &&
                (KeyValueInformation->DataLength))
            {
                NextInstance = *(PULONG)((ULONG_PTR)KeyValueInformation +
                                         KeyValueInformation->DataOffset);
            }

            ExFreePool(KeyValueInformation);
            KeyValueInformation = NULL;
        }

        /* Set the instance path */
        swprintf(ValueBuffer, L"%lu", NextInstance);
        RtlInitUnicodeString(&ValueName, ValueBuffer);
        Status = ZwSetValueKey(ServiceEnumKey,
                               &ValueName,
                               0,
                               REG_SZ,
                               DeviceNode->InstancePath.Buffer,
                               DeviceNode->InstancePath.MaximumLength);
        if (!NT_SUCCESS(Status))
            goto done;

        /* Increment Count and NextInstance */
        Count++;
        NextInstance++;

        /* Set the new Count value */
        RtlInitUnicodeString(&ValueName, L"Count");
        Status = ZwSetValueKey(ServiceEnumKey,
                               &ValueName,
                               0,
                               REG_DWORD,
                               &Count,
                               sizeof(Count));
        if (!NT_SUCCESS(Status))
            goto done;

        /* Set the new NextInstance value */
        RtlInitUnicodeString(&ValueName, L"NextInstance");
        Status = ZwSetValueKey(ServiceEnumKey,
                               &ValueName,
                               0,
                               REG_DWORD,
                               &NextInstance,
                               sizeof(NextInstance));
    }

done:
    if (ServiceEnumKey != NULL)
        ZwClose(ServiceEnumKey);

    if (ServiceKey != NULL)
        ZwClose(ServiceKey);

    ExFreePool(ServiceKeyName.Buffer);

    return Status;
}

static
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

static
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

    IopSetServiceEnumData(DeviceNode);

    /* Make sure we're started, and check if we need enumeration */
    if ((DeviceNode->Flags & DNF_STARTED) &&
        (DeviceNode->Flags & DNF_NEED_ENUMERATION_ONLY))
    {
        /* Enumerate us */
        IoInvalidateDeviceRelations(DeviceObject, BusRelations);
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
IopStartDevice(
    PDEVICE_NODE DeviceNode)
{
    NTSTATUS Status;
    HANDLE InstanceHandle = NULL, ControlHandle = NULL;
    UNICODE_STRING KeyName, ValueString;
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
    ValueString = DeviceNode->ServiceName;
    if (!ValueString.Buffer)
        RtlInitUnicodeString(&ValueString, L"");
    Status = ZwSetValueKey(ControlHandle, &KeyName, 0, REG_SZ, ValueString.Buffer, ValueString.Length + sizeof(UNICODE_NULL));
    // }

ByeBye:
    if (ControlHandle != NULL)
        ZwClose(ControlHandle);

    if (InstanceHandle != NULL)
        ZwClose(InstanceHandle);

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

/* PUBLIC FUNCTIONS **********************************************************/

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

static
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
IoInvalidateDeviceState(IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(PhysicalDeviceObject);
    IO_STACK_LOCATION Stack;
    ULONG_PTR PnPFlags;
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

static
NTSTATUS
PipEnumerateDevice(
    _In_ PDEVICE_NODE DeviceNode)
{
    DEVICETREE_TRAVERSE_CONTEXT Context;
    PDEVICE_RELATIONS DeviceRelations;
    PDEVICE_OBJECT ChildDeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;
    PDEVICE_NODE ChildDeviceNode;
    IO_STACK_LOCATION Stack;
    NTSTATUS Status;
    ULONG i;

    if (DeviceNode->Flags & DNF_NEED_ENUMERATION_ONLY)
    {
        DeviceNode->Flags &= ~DNF_NEED_ENUMERATION_ONLY;

        DPRINT("Sending GUID_DEVICE_ARRIVAL %wZ\n", &DeviceNode->InstancePath);
        IopQueueTargetDeviceEvent(&GUID_DEVICE_ARRIVAL,
                                  &DeviceNode->InstancePath);
    }

    DPRINT("Sending IRP_MN_QUERY_DEVICE_RELATIONS to device stack\n");

    Stack.Parameters.QueryDeviceRelations.Type = BusRelations;

    Status = IopInitiatePnpIrp(
        DeviceNode->PhysicalDeviceObject,
        &IoStatusBlock,
        IRP_MN_QUERY_DEVICE_RELATIONS,
        &Stack);
    if (!NT_SUCCESS(Status))
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

static
NTSTATUS
PipResetDevice(
    _In_ PDEVICE_NODE DeviceNode)
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(DeviceNode->Flags & DNF_ENUMERATED);
    ASSERT(DeviceNode->Flags & DNF_PROCESSED);

    /* Check if there's already a driver loaded for this device */
    if (DeviceNode->Flags & DNF_ADDED)
    {
        /* FIXME: our drivers do not handle device removal well enough */
#if 0
        /* Remove the device node */
        Status = IopRemoveDevice(DeviceNode);
        if (NT_SUCCESS(Status))
        {
            /* Invalidate device relations for the parent to reenumerate the device */
            DPRINT1("A new driver will be loaded for '%wZ' (FDO above removed)\n", &DeviceNode->InstancePath);
            Status = IoInvalidateDeviceRelations(DeviceNode->Parent->PhysicalDeviceObject, BusRelations);
        }
        else
#endif
        {
            /* A driver has already been loaded for this device */
            DPRINT("A reboot is required for the current driver for '%wZ' to be replaced\n", &DeviceNode->InstancePath);
            DeviceNode->Problem = CM_PROB_NEED_RESTART;
        }
    }
    else
    {
        /* FIXME: What if the device really is disabled? */
        DeviceNode->Flags &= ~DNF_DISABLED;
        DeviceNode->Problem = 0;

        /* Load service data from the registry */
        Status = IopActionConfigureChildServices(DeviceNode, DeviceNode->Parent);

        if (NT_SUCCESS(Status))
        {
            /* Start the service and begin PnP initialization of the device again */
            DPRINT("A new driver will be loaded for '%wZ' (no FDO above)\n", &DeviceNode->InstancePath);
            Status = IopActionInitChildServices(DeviceNode, DeviceNode->Parent);
        }
    }

    return Status;
}

#ifdef DBG
static
PCSTR
ActionToStr(
    _In_ DEVICE_ACTION Action)
{
    switch (Action)
    {
        case PiActionEnumDeviceTree:
            return "PiActionEnumDeviceTree";
        case PiActionEnumRootDevices:
            return "PiActionEnumRootDevices";
        case PiActionResetDevice:
            return "PiActionResetDevice";
        default:
            return "(request unknown)";
    }
}
#endif

static
VOID
NTAPI
PipDeviceActionWorker(
    _In_opt_ PVOID Context)
{
    PLIST_ENTRY ListEntry;
    PDEVICE_ACTION_REQUEST Request;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IopDeviceActionLock, &OldIrql);
    while (!IsListEmpty(&IopDeviceActionRequestList))
    {
        ListEntry = RemoveHeadList(&IopDeviceActionRequestList);
        KeReleaseSpinLock(&IopDeviceActionLock, OldIrql);
        Request = CONTAINING_RECORD(ListEntry, DEVICE_ACTION_REQUEST, RequestListEntry);

        ASSERT(Request->DeviceObject);

        PDEVICE_NODE deviceNode = IopGetDeviceNode(Request->DeviceObject);
        ASSERT(deviceNode);

        NTSTATUS status = STATUS_SUCCESS;

        DPRINT("Processing PnP request %p: DeviceObject - %p, Action - %s\n",
               Request, Request->DeviceObject, ActionToStr(Request->Action));

        switch (Request->Action)
        {
            case PiActionEnumRootDevices:
            case PiActionEnumDeviceTree:
                status = PipEnumerateDevice(deviceNode);
                break;

            case PiActionResetDevice:
                status = PipResetDevice(deviceNode);
                break;

            default:
                DPRINT1("Unimplemented device action %u\n", Request->Action);
                status = STATUS_NOT_IMPLEMENTED;
                break;
        }

        if (Request->CompletionStatus)
        {
            *Request->CompletionStatus = status;
        }

        if (Request->CompletionEvent)
        {
            KeSetEvent(Request->CompletionEvent, IO_NO_INCREMENT, FALSE);
        }

        DPRINT("Finished processing PnP request %p\n", Request);
        ObDereferenceObject(Request->DeviceObject);
        ExFreePoolWithTag(Request, TAG_IO);
        KeAcquireSpinLock(&IopDeviceActionLock, &OldIrql);
    }
    IopDeviceActionInProgress = FALSE;
    KeSetEvent(&PiEnumerationFinished, IO_NO_INCREMENT, FALSE);
    KeReleaseSpinLock(&IopDeviceActionLock, OldIrql);
}

/**
 * @brief      Queue a device operation to a worker thread.
 *
 * @param[in]  DeviceObject      The device object
 * @param[in]  Action            The action
 * @param[in]  CompletionEvent   The completion event object (optional)
 * @param[out] CompletionStatus  Status returned be the action will be written here
 */

VOID
PiQueueDeviceAction(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ DEVICE_ACTION Action,
    _In_opt_ PKEVENT CompletionEvent,
    _Out_opt_ NTSTATUS *CompletionStatus)
{
    PDEVICE_ACTION_REQUEST Request;
    KIRQL OldIrql;

    Request = ExAllocatePoolWithTag(NonPagedPoolMustSucceed, sizeof(*Request), TAG_IO);

    DPRINT("PiQueueDeviceAction: DeviceObject - %p, Request - %p, Action - %s\n",
        DeviceObject, Request, ActionToStr(Action));

    ObReferenceObject(DeviceObject);

    Request->DeviceObject = DeviceObject;
    Request->Action = Action;
    Request->CompletionEvent = CompletionEvent;
    Request->CompletionStatus = CompletionStatus;

    KeAcquireSpinLock(&IopDeviceActionLock, &OldIrql);
    InsertTailList(&IopDeviceActionRequestList, &Request->RequestListEntry);

    if (Action == PiActionEnumRootDevices)
    {
        ASSERT(!IopDeviceActionInProgress);

        IopDeviceActionInProgress = TRUE;
        KeClearEvent(&PiEnumerationFinished);
        KeReleaseSpinLock(&IopDeviceActionLock, OldIrql);

        PipDeviceActionWorker(NULL);
        return;
    }

    if (IopDeviceActionInProgress || !PnPBootDriversLoaded)
    {
        KeReleaseSpinLock(&IopDeviceActionLock, OldIrql);
        return;
    }
    IopDeviceActionInProgress = TRUE;
    KeClearEvent(&PiEnumerationFinished);
    KeReleaseSpinLock(&IopDeviceActionLock, OldIrql);

    ExInitializeWorkItem(&IopDeviceActionWorkItem, PipDeviceActionWorker, NULL);
    ExQueueWorkItem(&IopDeviceActionWorkItem, DelayedWorkQueue);
}

/**
 * @brief      Perfom a device operation synchronously via PiQueueDeviceAction
 *
 * @param[in]  DeviceObject  The device object
 * @param[in]  Action        The action
 *
 * @return     Status of the operation
 */

NTSTATUS
PiPerformSyncDeviceAction(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ DEVICE_ACTION Action)
{
    KEVENT opFinished;
    NTSTATUS status;

    KeInitializeEvent(&opFinished, SynchronizationEvent, FALSE);
    PiQueueDeviceAction(DeviceObject, Action, &opFinished, &status);
    KeWaitForSingleObject(&opFinished, Executive, KernelMode, FALSE, NULL);

    return status;
}
