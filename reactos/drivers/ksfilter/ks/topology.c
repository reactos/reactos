#include <ntddk.h>
#include <debug.h>
#include <ks.h>

/* ===============================================================
    Topology Functions
*/

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsCreateTopologyNode(
    IN  HANDLE ParentHandle,
    IN  PKSNODE_CREATE NodeCreate,
    IN  ACCESS_MASK DesiredAccess,
    OUT PHANDLE NodeHandle)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
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
