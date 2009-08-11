/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/filters/splitter/pin.c
 * PURPOSE:         Pin Context Handling
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

NTSTATUS
NTAPI
PinCreate(
    IN PKSPIN  Pin,
    IN PIRP  Irp)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PinClose(
    IN PKSPIN  Pin,
    IN PIRP  Irp)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
PinReset(
    IN PKSPIN  Pin)
{
    UNIMPLEMENTED
}

NTSTATUS
NTAPI
PinState(
    IN PKSPIN  Pin,
    IN KSSTATE  ToState,
    IN KSSTATE  FromState)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
AudioPositionPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PinIntersectHandler(
    IN PVOID Context,
    IN PIRP Irp,
    IN PKSP_PIN Pin,
    IN PKSDATARANGE DataRange,
    IN PKSDATARANGE MatchingDataRange,
    IN ULONG DataBufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

