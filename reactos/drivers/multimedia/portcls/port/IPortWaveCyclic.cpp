/*
    ReactOS Operating System
    Port Class API / IPort Implementation

    by Andrew Greenwood
*/

#include <portcls.h>

NTSTATUS
IPortWaveCyclic::NewMasterDmaChannel(
    OUT PDMACHANNEL* DmaChannel,
    IN  PUNKNOWN OuterUnknown,
    IN  PRESOURCELIST ResourceList OPTIONAL,
    IN  ULONG MaximumLength,
    IN  BOOL Dma32BitAddresses,
    IN  BOOL Dma64BitAddresses,
    IN  DMA_WIDTH DmaWidth,
    IN  DMA_SPEED DmaSpeed)
{
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
IPortWaveCyclic::NewSlaveDmaChannel(
    OUT PDMACHANNELSLAVE* DmaChannel,
    IN  PUNKNOWN OuterUnknown,
    IN  PRESOURCELIST ResourceList OPTIONAL,
    IN  ULONG DmaIndex,
    IN  ULONG MaximumLength,
    IN  BOOL DemandMode,
    IN  DMA_SPEED DmaSpeed)
{
    return STATUS_UNSUCCESSFUL;
}

VOID
IPortWaveCyclic::Notify(
    IN  PSERVICEGROUP ServiceGroup)
{
}
