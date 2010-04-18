/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/topoology.c
 * PURPOSE:         KS Allocator functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"


NTSTATUS
NTAPI
KspCreateObjectType(
    IN HANDLE ParentHandle,
    IN LPWSTR ObjectType,
    PVOID CreateParameters,
    UINT CreateParametersSize,
    IN  ACCESS_MASK DesiredAccess,
    OUT PHANDLE NodeHandle)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;

    /* calculate request length */
    Name.Length = 0;
    Name.MaximumLength = wcslen(ObjectType) * sizeof(WCHAR) + CreateParametersSize +  2 * sizeof(WCHAR);
    Name.MaximumLength += sizeof(WCHAR);
    /* acquire request buffer */
    Name.Buffer = ExAllocatePool(NonPagedPool, Name.MaximumLength);
    /* check for success */
    if (!Name.Buffer)
    {
        /* insufficient resources */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* build a request which looks like \{ObjectClass}\CreateParameters 
     * For pins the parent is the reference string used in registration
     * For clocks it is full path for pin\{ClockGuid}\ClockCreateParams
     */
    RtlAppendUnicodeToString(&Name, L"\\");
    RtlAppendUnicodeToString(&Name, ObjectType);
    RtlAppendUnicodeToString(&Name, L"\\");
    /* append create parameters */
    RtlMoveMemory(Name.Buffer + (Name.Length / sizeof(WCHAR)), CreateParameters, CreateParametersSize);
    Name.Length += CreateParametersSize;
    Name.Buffer[Name.Length / 2] = L'\0';

    InitializeObjectAttributes(&ObjectAttributes, &Name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE | OBJ_OPENIF, ParentHandle, NULL);
    /* create the instance */
    Status = IoCreateFile(NodeHandle,
                          DesiredAccess,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          0,
                          FILE_OPEN,
                          0,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          NULL,
                          IO_NO_PARAMETER_CHECKING | IO_FORCE_ACCESS_CHECK);

    /* free request buffer */
    ExFreePool(Name.Buffer);
    return Status;
}


/*
    @implemented
*/
KSDDKAPI NTSTATUS NTAPI
KsCreateTopologyNode(
    IN  HANDLE ParentHandle,
    IN  PKSNODE_CREATE NodeCreate,
    IN  ACCESS_MASK DesiredAccess,
    OUT PHANDLE NodeHandle)
{
    return KspCreateObjectType(ParentHandle,
                               KSSTRING_TopologyNode,
                               (PVOID)NodeCreate,
                               sizeof(KSNODE_CREATE),
                               DesiredAccess,
                               NodeHandle);
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsValidateTopologyNodeCreateRequest(
    IN  PIRP Irp,
    IN  PKSTOPOLOGY Topology,
    OUT PKSNODE_CREATE* OutNodeCreate)
{
    PKSNODE_CREATE NodeCreate;
    ULONG Size;
    NTSTATUS Status;

    /* did the caller miss the topology */
    if (!Topology)
        return STATUS_INVALID_PARAMETER;

    /* set create param  size */
    Size = sizeof(KSNODE_CREATE);

    /* fetch create parameters */
    Status = KspCopyCreateRequest(Irp,
                                  KSSTRING_TopologyNode,
                                  &Size,
                                  (PVOID*)&NodeCreate);

    if (!NT_SUCCESS(Status))
        return Status;

    if (NodeCreate->CreateFlags != 0 || (NodeCreate->Node >= Topology->TopologyNodesCount && NodeCreate->Node != MAXULONG))
    {
        /* invalid node create */
        FreeItem(NodeCreate);
        return STATUS_UNSUCCESSFUL;
    }

    /* store result */
    *OutNodeCreate = NodeCreate;

    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsTopologyPropertyHandler(
    IN  PIRP Irp,
    IN  PKSPROPERTY Property,
    IN  OUT PVOID Data,
    IN  const KSTOPOLOGY* Topology)
{
    UNICODE_STRING LocalMachine = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\MediaCategories\\");
    UNICODE_STRING Name = RTL_CONSTANT_STRING(L"Name");
    UNICODE_STRING GuidString;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    KSP_NODE * Node;
    PIO_STACK_LOCATION IoStack;
    ULONG Size;
    NTSTATUS Status;
    HANDLE hKey;
    PKEY_VALUE_PARTIAL_INFORMATION KeyInfo;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KsTopologyPropertyHandler Irp %p Property %p Data %p Topology %p OutputLength %lu PropertyId %lu\n", Irp, Property, Data, Topology, IoStack->Parameters.DeviceIoControl.OutputBufferLength, Property->Id);

    if (Property->Flags != KSPROPERTY_TYPE_GET)
    {
        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        Irp->IoStatus.Information = 0;
        return STATUS_NOT_IMPLEMENTED;
    }

    switch(Property->Id)
    {
        case KSPROPERTY_TOPOLOGY_CATEGORIES:
            return KsHandleSizedListQuery(Irp, Topology->CategoriesCount, sizeof(GUID), Topology->Categories);

        case KSPROPERTY_TOPOLOGY_NODES:
            return KsHandleSizedListQuery(Irp, Topology->TopologyNodesCount, sizeof(GUID), Topology->TopologyNodes);

        case KSPROPERTY_TOPOLOGY_CONNECTIONS:
            return KsHandleSizedListQuery(Irp, Topology->TopologyConnectionsCount, sizeof(KSTOPOLOGY_CONNECTION), Topology->TopologyConnections);

        case KSPROPERTY_TOPOLOGY_NAME:
            Node = (KSP_NODE*)Property;

            if (Node->NodeId >= Topology->TopologyNodesCount)
            {
                Irp->IoStatus.Information = 0;
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            Status = RtlStringFromGUID(&Topology->TopologyNodesNames[Node->NodeId], &GuidString);
            if (!NT_SUCCESS(Status))
            {
                Irp->IoStatus.Information = 0;
                break;
            }

            KeyName.Length = 0;
            KeyName.MaximumLength = LocalMachine.Length + GuidString.Length + sizeof(WCHAR);
            KeyName.Buffer = ExAllocatePool(PagedPool, KeyName.MaximumLength);
            if (!KeyName.Buffer)
            {
                Irp->IoStatus.Information = 0;
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            RtlAppendUnicodeStringToString(&KeyName, &LocalMachine);
            RtlAppendUnicodeStringToString(&KeyName, &GuidString);


            InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
            Status = ZwOpenKey(&hKey, GENERIC_READ, &ObjectAttributes);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("ZwOpenKey() failed with status 0x%08lx\n", Status);
                ExFreePool(KeyName.Buffer);
                Irp->IoStatus.Information = 0;
                break;
            }
            ExFreePool(KeyName.Buffer);
            Status = ZwQueryValueKey(hKey, &Name, KeyValuePartialInformation, NULL, 0, &Size);
            if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL)
            {
                ZwClose(hKey);
                Irp->IoStatus.Information = 0;
                break;
            }

            KeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION) ExAllocatePool(NonPagedPool, Size);
            if (!KeyInfo)
            {
                Status = STATUS_NO_MEMORY;
                break;
            }

            Status = ZwQueryValueKey(hKey, &Name, KeyValuePartialInformation, (PVOID)KeyInfo, Size, &Size);
            if (!NT_SUCCESS(Status))
            {
                ExFreePool(KeyInfo);
                ZwClose(hKey);
                Irp->IoStatus.Information = 0;
                break;
            }

            ZwClose(hKey);
            if (KeyInfo->DataLength + sizeof(WCHAR) > IoStack->Parameters.DeviceIoControl.OutputBufferLength)
            {
                Irp->IoStatus.Information = KeyInfo->DataLength + sizeof(WCHAR);
                Status = STATUS_MORE_ENTRIES;
                ExFreePool(KeyInfo);
                break;
            }

            RtlMoveMemory(Irp->UserBuffer, &KeyInfo->Data, KeyInfo->DataLength);
            ((LPWSTR)Irp->UserBuffer)[KeyInfo->DataLength / sizeof(WCHAR)] = L'\0';
             Irp->IoStatus.Information = KeyInfo->DataLength + sizeof(WCHAR);
            ExFreePool(KeyInfo);
            break;
        default:
             Irp->IoStatus.Information = 0;
           Status = STATUS_NOT_IMPLEMENTED;
    }


    return Status;
}

