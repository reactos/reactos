#include "priv.h"

KSDDKAPI NTSTATUS NTAPI
KsCreatePin(
    IN  HANDLE FilterHandle,
    IN  PKSPIN_CONNECT Connect,
    IN  ACCESS_MASK DesiredAccess,
    OUT PHANDLE ConnectionHandle)
{
    UINT ConnectSize = sizeof(KSPIN_CONNECT);

    PKSDATAFORMAT_WAVEFORMATEX Format = (PKSDATAFORMAT_WAVEFORMATEX)(Connect + 1);
    if (Format->DataFormat.FormatSize == sizeof(KSDATAFORMAT) ||
        Format->DataFormat.FormatSize == sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX))
    {
        ConnectSize += Format->DataFormat.FormatSize;
    }

    return KspCreateObjectType(FilterHandle,
                               L"{146F1A80-4791-11D0-A5D6-28DB04C10000}",
                               (PVOID)Connect,
                               ConnectSize,
                               DesiredAccess,
                               ConnectionHandle);
}

KSDDKAPI NTSTATUS NTAPI
KsValidateConnectRequest(
    IN  PIRP Irp,
    IN  ULONG DescriptorsCount,
    IN  KSPIN_DESCRIPTOR* Descriptor,
    OUT PKSPIN_CONNECT* Connect)
{
    return STATUS_SUCCESS;
}

KSDDKAPI NTSTATUS NTAPI
KsPinPropertyHandler(
    IN  PIRP Irp,
    IN  PKSPROPERTY Property,
    IN  OUT PVOID Data,
    IN  ULONG DescriptorsCount,
    IN  const KSPIN_DESCRIPTOR* Descriptor)
{
    return STATUS_SUCCESS;
}

KSDDKAPI NTSTATUS NTAPI
KsPinDataIntersection(
    IN  PIRP Irp,
    IN  PKSPIN Pin,
    OUT PVOID Data,
    IN  ULONG DescriptorsCount,
    IN  const KSPIN_DESCRIPTOR* Descriptor,
    IN  PFNKSINTERSECTHANDLER IntersectHandler)
{
    return STATUS_SUCCESS;
}

/* Does this belong here? */

KSDDKAPI NTSTATUS NTAPI
KsHandleSizedListQuery(
    IN  PIRP Irp,
    IN  ULONG DataItemsCount,
    IN  ULONG DataItemSize,
    IN  const VOID* DataItems)
{
    return STATUS_SUCCESS;
}
