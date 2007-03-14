/*
    ReactOS Operating System

    Port Class API
    IMiniPortMidi Implementation

    by Andrew Greenwood

    REFERENCE:
        http://www.osronline.com/ddkx/stream/audmp-routines_64vn.htm
*/

#include <portcls.h>

NTSTATUS
IMiniport::GetDescription(
    OUT PPCFILTER_DESCRIPTOR* Description)
{
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
IMiniport::DataRangeIntersection(
    IN  ULONG PinId,
    IN  PKSDATARANGE DataRange,
    IN  PKSDATARANGE MatchingDataRange,
    IN  ULONG OutputBufferLength,
    OUT PVOID ResultantFormat OPTIONAL,
    OUT PULONG ResultantFormatLength)
{
    return STATUS_UNSUCCESSFUL;
}
