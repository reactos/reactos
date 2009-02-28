#include "priv.h"

/* ===============================================================
    Topology Functions
*/

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

    Name.Length = (wcslen(ObjectType) + 1) * sizeof(WCHAR) + CreateParametersSize;
    Name.MaximumLength += sizeof(WCHAR);
    Name.Buffer = ExAllocatePool(NonPagedPool, Name.MaximumLength);

    if (!Name.Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    wcscpy(Name.Buffer, ObjectType);
    Name.Buffer[wcslen(ObjectType)] = '\\';

    RtlMoveMemory(Name.Buffer + wcslen(ObjectType) +1, CreateParameters, CreateParametersSize);

    Name.Buffer[Name.Length / 2] = L'\0';
    InitializeObjectAttributes(&ObjectAttributes, &Name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE | OBJ_OPENIF, ParentHandle, NULL);


    Status = IoCreateFile(NodeHandle,
                          DesiredAccess,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          0,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          NULL,
                          IO_NO_PARAMETER_CHECKING | IO_FORCE_ACCESS_CHECK);

    // HACK HACK HACK HACK
    //ExFreePool(Name.Buffer);
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
                               L"{0621061A-EE75-11D0-B915-00A0C9223196}",
                               (PVOID)NodeCreate,
                               sizeof(KSNODE_CREATE),
                               DesiredAccess,
                               NodeHandle);
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsValidateTopologyNodeCreateRequest(
    IN  PIRP Irp,
    IN  PKSTOPOLOGY Topology,
    OUT PKSNODE_CREATE* NodeCreate)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
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
    KSMULTIPLE_ITEM * Item;
    KSP_NODE * Node;
    PIO_STACK_LOCATION IoStack;
    ULONG Size;
    NTSTATUS Status;
    HANDLE hKey;
    PKEY_VALUE_PARTIAL_INFORMATION KeyInfo;

    if (Property->Flags != KSPROPERTY_TYPE_GET)
    {
        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        Irp->IoStatus.Information = 0;
        return STATUS_NOT_IMPLEMENTED;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch(Property->Id)
    {
        case KSPROPERTY_TOPOLOGY_CATEGORIES:
            Size = sizeof(KSMULTIPLE_ITEM) + Topology->CategoriesCount * sizeof(GUID);
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Item = (KSMULTIPLE_ITEM*)Irp->UserBuffer;
            Item->Size = Size;
            Item->Count = Topology->CategoriesCount;

            RtlMoveMemory((PVOID)(Item + 1), (PVOID)Topology->Categories, Topology->CategoriesCount * sizeof(GUID));
            Irp->IoStatus.Information = Size;
            Status = STATUS_SUCCESS;
            break;

        case KSPROPERTY_TOPOLOGY_NODES:
            Size = sizeof(KSMULTIPLE_ITEM) + Topology->TopologyNodesCount * sizeof(GUID);
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Item = (KSMULTIPLE_ITEM*)Irp->UserBuffer;
            Item->Size = Size;
            Item->Count = Topology->TopologyNodesCount;

            RtlMoveMemory((PVOID)(Item + 1), (PVOID)Topology->TopologyNodes, Topology->TopologyNodesCount * sizeof(GUID));
            Irp->IoStatus.Information = Size;
            Status = STATUS_SUCCESS;
            break;

        case KSPROPERTY_TOPOLOGY_CONNECTIONS:
            Size = sizeof(KSMULTIPLE_ITEM) + Topology->TopologyConnectionsCount  * sizeof(GUID);
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
            {
                Irp->IoStatus.Information = Size;
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Item = (KSMULTIPLE_ITEM*)Irp->UserBuffer;
            Item->Size = Size;
            Item->Count = Topology->TopologyConnectionsCount;

            RtlMoveMemory((PVOID)(Item + 1), (PVOID)Topology->TopologyConnections, Topology->TopologyConnectionsCount * sizeof(GUID));
            Irp->IoStatus.Information = Size;
            Status = STATUS_SUCCESS;
            break;

        case KSPROPERTY_TOPOLOGY_NAME:
            Node = (KSP_NODE*)Property;

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
                Status = STATUS_BUFFER_TOO_SMALL;
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
