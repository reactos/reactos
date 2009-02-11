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

    InitializeObjectAttributes(&ObjectAttributes, &Name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, ParentHandle, NULL);


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
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsTopologyPropertyHandler(
    IN  PIRP Irp,
    IN  PKSPROPERTY Property,
    IN  OUT PVOID Data,
    IN  const KSTOPOLOGY* Topology)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}
