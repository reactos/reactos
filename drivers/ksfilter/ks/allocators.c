/* ===============================================================
    Allocator Functions
*/

#include <ntddk.h>
#include <debug.h>
#include <ks.h>

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsCreateAllocator(
    IN  HANDLE ConnectionHandle,
    IN  PKSALLOCATOR_FRAMING AllocatorFraming,
    OUT PHANDLE AllocatorHandle)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsCreateDefaultAllocator(
    IN  PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsValidateAllocatorCreateRequest(
    IN  PIRP Irp,
    OUT PKSALLOCATOR_FRAMING* AllocatorFraming)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsCreateDefaultAllocatorEx(
    IN  PIRP Irp,
    IN  PVOID InitializeContext OPTIONAL,
    IN  PFNKSDEFAULTALLOCATE DefaultAllocate OPTIONAL,
    IN  PFNKSDEFAULTFREE DefaultFree OPTIONAL,
    IN  PFNKSINITIALIZEALLOCATOR InitializeAllocator OPTIONAL,
    IN  PFNKSDELETEALLOCATOR DeleteAllocator OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsValidateAllocatorFramingEx(
    IN  PKSALLOCATOR_FRAMING_EX Framing,
    IN  ULONG BufferSize,
    IN  const KSALLOCATOR_FRAMING_EX* PinFraming)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}
