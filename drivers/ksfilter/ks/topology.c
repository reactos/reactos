/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/topoology.c
 * PURPOSE:         KS Allocator functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

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
    Name.MaximumLength = (USHORT)(wcslen(ObjectType) * sizeof(WCHAR) + CreateParametersSize +  1 * sizeof(WCHAR));
    Name.MaximumLength += sizeof(WCHAR);
    /* acquire request buffer */
    Name.Buffer = AllocateItem(NonPagedPool, Name.MaximumLength);
    /* check for success */
    if (!Name.Buffer)
    {
        /* insufficient resources */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* build a request which looks like {ObjectClass}\CreateParameters
     * For pins the parent is the reference string used in registration
     * For clocks it is full path for pin\{ClockGuid}\ClockCreateParams
     */
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
    FreeItem(Name.Buffer);
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
    KSP_NODE * Node;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    PKEY_VALUE_PARTIAL_INFORMATION KeyInfo;
    LPGUID Guid;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KsTopologyPropertyHandler Irp %p Property %p Data %p Topology %p OutputLength %lu PropertyId %lu\n", Irp, Property, Data, Topology, IoStack->Parameters.DeviceIoControl.OutputBufferLength, Property->Id);

    if ((Property->Flags & (KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET)) != KSPROPERTY_TYPE_GET)
    {
        UNIMPLEMENTED;
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

            /* check for invalid node id */
            if (Node->NodeId >= Topology->TopologyNodesCount)
            {
                /* invalid node id */
                Irp->IoStatus.Information = 0;
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* check if there is a name supplied */
            if (!IsEqualGUIDAligned(&Topology->TopologyNodesNames[Node->NodeId], &GUID_NULL))
            {
                /* node name has been supplied */
                Guid = (LPGUID)&Topology->TopologyNodesNames[Node->NodeId];
            }
            else
            {
                /* fallback to topology node type */
                Guid = (LPGUID)&Topology->TopologyNodes[Node->NodeId];
            }

            /* read topology node name */
            Status = KspReadMediaCategory(Guid, &KeyInfo);
            if (!NT_SUCCESS(Status))
            {
                Irp->IoStatus.Information = 0;
                break;
            }

            /* store result size */
            Irp->IoStatus.Information = KeyInfo->DataLength + sizeof(WCHAR);

            /* check for buffer overflow */
            if (KeyInfo->DataLength + sizeof(WCHAR) > IoStack->Parameters.DeviceIoControl.OutputBufferLength)
            {
                /* buffer too small */
                Status = STATUS_BUFFER_OVERFLOW;
                FreeItem(KeyInfo);
                break;
            }

            /* copy result buffer */
            RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer, &KeyInfo->Data, KeyInfo->DataLength);

            /* zero terminate it */
            ((LPWSTR)Irp->AssociatedIrp.SystemBuffer)[KeyInfo->DataLength / sizeof(WCHAR)] = L'\0';

            /* free key info */
            FreeItem(KeyInfo);

            break;
        default:
             Irp->IoStatus.Information = 0;
           Status = STATUS_NOT_FOUND;
    }

    Irp->IoStatus.Status = Status;
    return Status;
}

