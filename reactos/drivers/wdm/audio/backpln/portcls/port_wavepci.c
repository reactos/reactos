#include "private.h"

typedef struct
{
    IPortWavePciVtbl *lpVtbl;
    IPortClsVersion  *lpVtblPortClsVersion;
#if 0
    IUnregisterSubdevice *lpVtblUnregisterSubDevice;
#endif
    LONG ref;


}IPortWavePciImpl;


NTSTATUS
NTAPI
IPortWavePci_fnNewMasterDmaChannel(
    IN IPortWavePci * iface,
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
IPortWavePci_fnNotify(
    IN IPortWavePci * iface,
    IN  PSERVICEGROUP ServiceGroup)
{
}

NTSTATUS
NewPortWavePci(
    OUT PPORT* OutPort)
{
    return STATUS_UNSUCCESSFUL;
}
