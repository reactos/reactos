/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/40gone.c
 * PURPOSE:     Obsoleted functions in NDIS 4.0
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "ndissys.h"


/*
 * @unimplemented
 */
VOID
EXPORT
ArcFilterDprIndicateReceive(
    IN  PARC_FILTER Filter,
    IN  PUCHAR      pRawHeader,
    IN  PUCHAR      pData,
    IN  UINT        Length)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
ArcFilterDprIndicateReceiveComplete(
    IN  PARC_FILTER Filter)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
FddiFilterDprIndicateReceive(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     MacReceiveContext,
    IN  PCHAR           Address,
    IN  UINT            AddressLength,
    IN  PVOID           HeaderBuffer,
    IN  UINT            HeaderBufferSize,
    IN  PVOID           LookaheadBuffer,
    IN  UINT            LookaheadBufferSize,
    IN  UINT            PacketSize)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
FddiFilterDprIndicateReceiveComplete(
    IN  PFDDI_FILTER    Filter)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisAllocateDmaChannel(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_HANDLE            NdisDmaHandle,
    IN  NDIS_HANDLE             NdisAdapterHandle,
    IN  PNDIS_DMA_DESCRIPTION   DmaDescription,
    IN  ULONG                   MaximumLength)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisAllocateSharedMemory(
    IN  NDIS_HANDLE             NdisAdapterHandle,
    IN  ULONG                   Length,
    IN  BOOLEAN                 Cached,
    OUT PVOID                   *VirtualAddress,
    OUT PNDIS_PHYSICAL_ADDRESS  PhysicalAddress)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCompleteBufferPhysicalMapping(
    IN  NDIS_HANDLE     NdisAdapterHandle,
    IN  PNDIS_BUFFER    Buffer,
    IN  ULONG           PhysicalMapRegister)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCompleteRequest(
    IN  NDIS_HANDLE     NdisBindingContext,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  NDIS_STATUS     Status)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCompleteReset(
    IN  NDIS_HANDLE NdisBindingContext,
    IN  NDIS_STATUS Status)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCompleteSend(
    IN  NDIS_HANDLE     NdisBindingContext,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCompleteTransferData(
    IN  NDIS_HANDLE     NdisBindingContext,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status,
    IN  UINT            BytesTransferred)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisIndicateReceive(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     NdisBindingContext,
    IN  NDIS_HANDLE     MacReceiveContext,
    IN  PVOID           HeaderBuffer,
    IN  UINT            HeaderBufferSize,
    IN  PVOID           LookaheadBuffer,
    IN  UINT            LookaheadBufferSize,
    IN  UINT            PacketSize)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisIndicateReceiveComplete(
    IN  NDIS_HANDLE NdisBindingContext)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisIndicateStatus(
    IN  NDIS_HANDLE NdisBindingContext,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisIndicateStatusComplete(
    IN  NDIS_HANDLE NdisBindingContext)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisPciAssignResources(
    IN  NDIS_HANDLE         NdisMacHandle,
    IN  NDIS_HANDLE         NdisWrapperHandle,
    IN  NDIS_HANDLE         WrapperConfigurationContext,
    IN  ULONG               SlotNumber,
    OUT PNDIS_RESOURCE_LIST *AssignedResources)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisReadBindingInformation (
    OUT PNDIS_STATUS    Status,
    OUT PNDIS_STRING    * Binding,
    IN  NDIS_HANDLE     ConfigurationHandle)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
ULONG
EXPORT
NdisReadDmaCounter(
    IN  NDIS_HANDLE NdisDmaHandle)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisStartBufferPhysicalMapping(
    IN  NDIS_HANDLE                 NdisAdapterHandle,
    IN  PNDIS_BUFFER                Buffer,
    IN  ULONG                       PhysicalMapRegister,
    IN  BOOLEAN                     WriteToDevice,
    OUT PNDIS_PHYSICAL_ADDRESS_UNIT PhysicalAddressArray,
    OUT PUINT                       ArraySize)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
BOOLEAN
EXPORT
NdisSynchronizeWithInterrupt(
    IN  PNDIS_INTERRUPT Interrupt,
    IN  PVOID           SynchronizeFunction,
    IN  PVOID           SynchronizeContext)
{
    UNIMPLEMENTED

    return FALSE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisUnmapIoSpace(
    IN  NDIS_HANDLE NdisAdapterHandle,
    IN  PVOID       VirtualAddress,
    IN  UINT        Length)
{
    UNIMPLEMENTED
}
/*
 * @unimplemented
 */
VOID
EXPORT
TrFilterDprIndicateReceive(
    IN  PTR_FILTER  Filter,
    IN  NDIS_HANDLE MacReceiveContext,
    IN  PVOID       HeaderBuffer,
    IN  UINT        HeaderBufferSize,
    IN  PVOID       LookaheadBuffer,
    IN  UINT        LookaheadBufferSize,
    IN  UINT        PacketSize)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
TrFilterDprIndicateReceiveComplete(
    IN  PTR_FILTER  Filter)
{
    UNIMPLEMENTED
}

/* EOF */
