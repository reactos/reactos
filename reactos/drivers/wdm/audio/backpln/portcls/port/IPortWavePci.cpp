/*
    ReactOS Operating System
    Port Class API / IPort Implementation

    by Andrew Greenwood
*/

#include <portcls.h>

NTSTATUS
IPortWavePci::NewMasterDmaChannel(
    OUT PDMACHANNEL* DmaChannel,
    IN  PUNKNOWN OuterUnknown,
    IN  POOL_TYPE PoolType,
    IN  PRESOURCELIST ResourceList OPTIONAL,
    IN  BOOL ScatterGather,
    IN  BOOL Dma32BitAddresses,
    IN  BOOL Dma64BitAddresses,
    IN  DMA_WIDTH DmaWidth,
    IN  DMA_SPEED DmaSpeed,
    IN  ULONG MaximumLength,
    IN  ULONG DmaPort)
{
    return STATUS_UNSUCCESSFUL;
}

VOID
IPortWavePci::Notify(
    IN  PSERVICEGROUP ServiceGroup)
{
}
