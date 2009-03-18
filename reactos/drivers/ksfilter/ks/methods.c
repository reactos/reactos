#include "priv.h"

/* ===============================================================
    Method Functions
*/

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsMethodHandler(
    IN  PIRP Irp,
    IN  ULONG MethodSetsCount,
    IN  PKSMETHOD_SET MethodSet)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsMethodHandlerWithAllocator(
    IN  PIRP Irp,
    IN  ULONG MethodSetsCount,
    IN  PKSMETHOD_SET MethodSet,
    IN  PFNKSALLOCATOR Allocator OPTIONAL,
    IN  ULONG MethodItemSize OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI BOOLEAN NTAPI
KsFastMethodHandler(
    IN  PFILE_OBJECT FileObject,
    IN  PKSMETHOD UNALIGNED Method,
    IN  ULONG MethodLength,
    IN  OUT PVOID UNALIGNED Data,
    IN  ULONG DataLength,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  ULONG MethodSetsCount,
    IN  const KSMETHOD_SET* MethodSet)
{
    UNIMPLEMENTED;
    return FALSE;
}
