/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/40gone.c
 * PURPOSE:     Obsoleted functions in NDIS 4.0
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>


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
NDIS_STATUS
EXPORT
EthChangeFilterAddresses(
    IN  PETH_FILTER     Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  UINT            AddressCount,
    IN  CHAR            Addresses [] [ETH_LENGTH_OF_ADDRESS],
    IN  BOOLEAN         Set)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
BOOLEAN
EXPORT
EthCreateFilter(
    IN  UINT                MaximumMulticastAddresses,
    IN  ETH_ADDRESS_CHANGE  AddressChangeAction,
    IN  ETH_FILTER_CHANGE   FilterChangeAction,
    IN  ETH_DEFERRED_CLOSE  CloseAction,
    IN  PUCHAR              AdapterAddress,
    IN  PNDIS_SPIN_LOCK     Lock,
    OUT PETH_FILTER         * Filter)
{
    UNIMPLEMENTED

	return FALSE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
EthDeleteFilter(
    IN  PETH_FILTER Filter)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
EthDeleteFilterOpenAdapter(
    IN  PETH_FILTER	Filter,
    IN  NDIS_HANDLE	NdisFilterHandle,
    IN  PNDIS_REQUEST	NdisRequest)
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
EthFilterAdjust(
    IN  PETH_FILTER     Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  UINT            FilterClasses,
    IN  BOOLEAN         Set)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
EthFilterIndicateReceive(
    IN	PETH_FILTER Filter,
    IN	NDIS_HANDLE MacReceiveContext,
    IN	PCHAR       Address,
    IN	PVOID       HeaderBuffer,
    IN	UINT        HeaderBufferSize,
    IN	PVOID       LookaheadBuffer,
    IN	UINT        LookaheadBufferSize,
    IN	UINT        PacketSize)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
EthFilterIndicateReceiveComplete(
    IN  PETH_FILTER Filter)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
EthFilterDprIndicateReceive(
    IN	PETH_FILTER Filter,
    IN	NDIS_HANDLE MacReceiveContext,
    IN	PCHAR       Address,
    IN	PVOID       HeaderBuffer,
    IN	UINT        HeaderBufferSize,
    IN	PVOID       LookaheadBuffer,
    IN	UINT        LookaheadBufferSize,
    IN	UINT        PacketSize)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
EthFilterDprIndicateReceiveComplete(
    IN  PETH_FILTER Filter)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
BOOLEAN
EXPORT
EthNoteFilterOpenAdapter(
    IN  PETH_FILTER     Filter,
    IN  NDIS_HANDLE     MacBindingHandle,
    IN  NDIS_HANDLE     NdisBindingContext,
    OUT PNDIS_HANDLE    NdisFilterHandle)
{
    UNIMPLEMENTED

	return FALSE;
}


/*
 * @unimplemented
 */
UINT
EXPORT
EthNumberOfOpenFilterAddresses(
    IN  PETH_FILTER Filter,
    IN  NDIS_HANDLE NdisFilterHandle)
{
    UNIMPLEMENTED

	return 0;
}


/*
 * @unimplemented
 */
VOID
EXPORT
EthQueryGlobalFilterAddresses (
    OUT PNDIS_STATUS    Status,
    IN  PETH_FILTER     Filter,
    IN  UINT            SizeOfArray,
    OUT PUINT           NumberOfAddresses,
    IN  OUT	CHAR        AddressArray [] [ETH_LENGTH_OF_ADDRESS])
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
EthQueryOpenFilterAddresses(
    OUT	    PNDIS_STATUS    Status,
    IN	    PETH_FILTER     Filter,
    IN	    NDIS_HANDLE     NdisFilterHandle,
    IN	    UINT            SizeOfArray,
    OUT	    PUINT           NumberOfAddresses,
    IN OUT  CHAR            AddressArray [] [ETH_LENGTH_OF_ADDRESS])
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
BOOLEAN
EXPORT
EthShouldAddressLoopBack(
    IN  PETH_FILTER Filter,
    IN  CHAR        Address [ETH_LENGTH_OF_ADDRESS])
{
    UNIMPLEMENTED

	return FALSE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
FddiChangeFilterLongAddresses(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  UINT            AddressCount,
    IN  CHAR            Addresses [] [FDDI_LENGTH_OF_LONG_ADDRESS],
    IN  BOOLEAN         Set)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
FddiChangeFilterShortAddresses(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  UINT            AddressCount,
    IN  CHAR            Addresses [] [FDDI_LENGTH_OF_SHORT_ADDRESS],
    IN  BOOLEAN         Set)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
BOOLEAN
EXPORT
FddiCreateFilter(
    IN  UINT                MaximumMulticastLongAddresses,
    IN  UINT                MaximumMulticastShortAddresses,
    IN  FDDI_ADDRESS_CHANGE AddressChangeAction,
    IN  FDDI_FILTER_CHANGE  FilterChangeAction,
    IN  FDDI_DEFERRED_CLOSE CloseAction,
    IN  PUCHAR              AdapterLongAddress,
    IN  PUCHAR              AdapterShortAddress,
    IN  PNDIS_SPIN_LOCK     Lock,
    OUT PFDDI_FILTER        * Filter)
{
    UNIMPLEMENTED

	return FALSE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
FddiDeleteFilter(
    IN  PFDDI_FILTER    Filter)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
FddiDeleteFilterOpenAdapter(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest)
{
    UNIMPLEMENTED

	return NDIS_STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
FddiFilterAdjust(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  UINT            FilterClasses,
    IN  BOOLEAN         Set)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
FddiFilterIndicateReceive(
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
FddiFilterIndicateReceiveComplete(
    IN  PFDDI_FILTER    Filter)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
BOOLEAN
EXPORT
FddiNoteFilterOpenAdapter(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     MacBindingHandle,
    IN  NDIS_HANDLE     NdisBindingContext,
    OUT PNDIS_HANDLE    NdisFilterHandle)
{
    UNIMPLEMENTED

	return FALSE;
}


/*
 * @unimplemented
 */
UINT
EXPORT
FddiNumberOfOpenFilterLongAddresses(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     NdisFilterHandle)
{
    UNIMPLEMENTED

	return 0;
}


/*
 * @unimplemented
 */
UINT
EXPORT
FddiNumberOfOpenFilterShortAddresses(
    IN  PFDDI_FILTER    Filter,
    IN  NDIS_HANDLE     NdisFilterHandle)
{
    UNIMPLEMENTED

	return 0;
}


/*
 * @unimplemented
 */
VOID
EXPORT
FddiQueryGlobalFilterLongAddresses(
    OUT     PNDIS_STATUS    Status,
    IN      PFDDI_FILTER    Filter,
    IN      UINT            SizeOfArray,
    OUT     PUINT           NumberOfAddresses,
    IN OUT  CHAR            AddressArray [] [FDDI_LENGTH_OF_LONG_ADDRESS])
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
FddiQueryGlobalFilterShortAddresses(
    OUT     PNDIS_STATUS    Status,
    IN      PFDDI_FILTER    Filter,
    IN      UINT            SizeOfArray,
    OUT     PUINT           NumberOfAddresses,
    IN OUT  CHAR            AddressArray [] [FDDI_LENGTH_OF_SHORT_ADDRESS])
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
FddiQueryOpenFilterLongAddresses(
    OUT     PNDIS_STATUS    Status,
    IN      PFDDI_FILTER    Filter,
    IN      NDIS_HANDLE     NdisFilterHandle,
    IN      UINT            SizeOfArray,
    OUT     PUINT           NumberOfAddresses,
    IN OUT  CHAR            AddressArray [] [FDDI_LENGTH_OF_LONG_ADDRESS])
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
FddiQueryOpenFilterShortAddresses(
    OUT     PNDIS_STATUS    Status,
    IN      PFDDI_FILTER    Filter,
    IN      NDIS_HANDLE     NdisFilterHandle,
    IN      UINT            SizeOfArray,
    OUT     PUINT           NumberOfAddresses,
    IN OUT  CHAR            AddressArray [] [FDDI_LENGTH_OF_SHORT_ADDRESS])
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
BOOLEAN
EXPORT
FddiShouldAddressLoopBack(
    IN  PFDDI_FILTER    Filter,
    IN  CHAR            Address [],
    IN  UINT            LengthOfAddress)
{
    UNIMPLEMENTED

	return FALSE;
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
NDIS_STATUS
EXPORT
TrChangeFunctionalAddress(
    IN  PTR_FILTER      Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  CHAR            FunctionalAddressArray [TR_LENGTH_OF_FUNCTIONAL],
    IN  BOOLEAN         Set)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
TrChangeGroupAddress(
    IN  PTR_FILTER      Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  CHAR            GroupAddressArray [TR_LENGTH_OF_FUNCTIONAL],
    IN  BOOLEAN         Set)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
BOOLEAN
EXPORT
TrCreateFilter(
    IN  TR_ADDRESS_CHANGE   AddressChangeAction,
    IN  TR_GROUP_CHANGE     GroupChangeAction,
    IN  TR_FILTER_CHANGE    FilterChangeAction,
    IN  TR_DEFERRED_CLOSE   CloseAction,
    IN  PUCHAR              AdapterAddress,
    IN  PNDIS_SPIN_LOCK     Lock,
    OUT PTR_FILTER          * Filter)
{
    UNIMPLEMENTED

	return FALSE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
TrDeleteFilter(
    IN  PTR_FILTER  Filter)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
TrDeleteFilterOpenAdapter (
    IN  PTR_FILTER      Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest)
{
    UNIMPLEMENTED

	return NDIS_STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
TrFilterAdjust(
    IN  PTR_FILTER      Filter,
    IN  NDIS_HANDLE     NdisFilterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  UINT            FilterClasses,
    IN  BOOLEAN         Set)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
TrFilterIndicateReceive(
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
TrFilterIndicateReceiveComplete(
    IN  PTR_FILTER  Filter)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
BOOLEAN
EXPORT
TrNoteFilterOpenAdapter(
    IN  PTR_FILTER      Filter,
    IN  NDIS_HANDLE     MacBindingHandle,
    IN  NDIS_HANDLE     NdisBindingContext,
    OUT PNDIS_HANDLE    NdisFilterHandle)
{
    UNIMPLEMENTED

	return FALSE;
}


/*
 * @unimplemented
 */
BOOLEAN
EXPORT
TrShouldAddressLoopBack(
    IN  PTR_FILTER  Filter,
    IN  CHAR        DestinationAddress [TR_LENGTH_OF_ADDRESS],
    IN  CHAR        SourceAddress [TR_LENGTH_OF_ADDRESS])
{
    UNIMPLEMENTED

	return FALSE;
}

/* EOF */
