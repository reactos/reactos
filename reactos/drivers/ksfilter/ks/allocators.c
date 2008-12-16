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
    @implemented
*/
KSDDKAPI NTSTATUS NTAPI
KsCreateDefaultAllocator(
    IN  PIRP Irp)
{
    return KsCreateDefaultAllocatorEx(Irp, NULL, NULL, NULL, NULL, NULL);
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
    NTSTATUS Status;
    PKSALLOCATOR_FRAMING AllocatorFraming;

   Status = KsValidateAllocatorCreateRequest(Irp, &AllocatorFraming);
   if (!NT_SUCCESS(Status))
       return STATUS_INVALID_PARAMETER;


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
