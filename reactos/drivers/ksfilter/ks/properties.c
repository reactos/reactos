#include <ntddk.h>
#include <debug.h>
#include <ks.h>

/* ===============================================================
    Property Functions
*/

/*
    @unimplemented
*/
KSDDKAPI BOOLEAN NTAPI
KsFastPropertyHandler(
    IN  PFILE_OBJECT FileObject,
    IN  PKSPROPERTY UNALIGNED Property,
    IN  ULONG PropertyLength,
    IN  OUT PVOID UNALIGNED Data,
    IN  ULONG DataLength,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  ULONG PropertySetsCount,
    IN  const KSPROPERTY_SET* PropertySet)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsPropertyHandler(
    IN  PIRP Irp,
    IN  ULONG PropertySetsCount,
    IN  const KSPROPERTY_SET* PropertySet)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsPropertyHandlerWithAllocator(
    IN  PIRP Irp,
    IN  ULONG PropertySetsCount,
    IN  PKSPROPERTY_SET PropertySet,
    IN  PFNKSALLOCATOR Allocator OPTIONAL,
    IN  ULONG PropertyItemSize OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsUnserializeObjectPropertiesFromRegistry(
    IN  PFILE_OBJECT FileObject,
    IN  HANDLE ParentKey OPTIONAL,
    IN  PUNICODE_STRING RegistryPath OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}
