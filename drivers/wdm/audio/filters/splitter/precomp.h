#pragma once

#include <ntddk.h>
#include <portcls.h>
#include <ks.h>
#include <ksmedia.h>
#include <debug.h>

/* filter.c */
NTSTATUS
NTAPI
FilterProcess(
    IN PKSFILTER  Filter,
    IN PKSPROCESSPIN_INDEXENTRY  ProcessPinsIndex);

/* pin.c */
NTSTATUS
NTAPI
PinCreate(
    IN PKSPIN  Pin,
    IN PIRP  Irp);

NTSTATUS
NTAPI
PinClose(
    IN PKSPIN  Pin,
    IN PIRP  Irp);

VOID
NTAPI
PinReset(
    IN PKSPIN  Pin);

NTSTATUS
NTAPI
PinState(
    IN PKSPIN  Pin,
    IN KSSTATE  ToState,
    IN KSSTATE  FromState);

NTSTATUS
NTAPI
AudioPositionPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data);

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
    OUT PULONG DataSize);



typedef struct
{
    ULONG BytesAvailable;
    ULONG BytesProcessed;
}PIN_CONTEXT, *PPIN_CONTEXT;
