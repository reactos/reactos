/*
    ReactOS Operating System

    Port Class API
    IMiniPortMidi Implementation

    by Andrew Greenwood

    REFERENCE:
        http://www.osronline.com/ddkx/stream/audmp-routines_1fsj.htm
*/
#include "private.h"
#include <portcls.h>

NTSTATUS
IMiniportMidi::Init(
    IN  PUNKNOWN UnknownAdapter,
    IN  PRESOURCELIST ResourceList,
    IN  PPORTMIDI Port,
    OUT PSERVICEGROUP* ServiceGroup)
{
    /* http://www.osronline.com/ddkx/stream/audmp-routines_6jsj.htm */

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
IMiniportMidi::NewStream(
    OUT PMINIPORTMIDISTREAM Stream,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  ULONG Pin,
    IN  BOOLEAN Capture,
    IN  PKSDATAFORMAT DataFormat,
    OUT PSERVICEGROUP* ServiceGroup)
{
    return STATUS_UNSUCCESSFUL;
}

void
IMiniportMidi::Service(void)
{
}
