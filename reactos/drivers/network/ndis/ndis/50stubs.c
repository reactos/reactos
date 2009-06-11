/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/50stubs.c
 * PURPOSE:     NDIS 5.0 Stubs
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
NdisSetPacketStatus(
    IN PNDIS_PACKET  Packet,
    IN NDIS_STATUS  Status,
    IN NDIS_HANDLE  Handle,
    IN ULONG  Code)
{
    UNIMPLEMENTED
}

/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisQueryPendingIOCount(
    IN  PVOID  NdisBindingHandle,
    OUT PULONG  IoCount)
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}

/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisIMNotifyPnPEvent(
    IN  NDIS_HANDLE  MiniportHandle,
    IN  PNET_PNP_EVENT  NetPnPEvent)
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}

/*
 * @unimplemented
 */
PNDIS_PACKET_STACK
EXPORT
NdisIMGetCurrentPacketStack(
    IN PNDIS_PACKET  Packet,
    OUT BOOLEAN  *StacksRemaining)
{
    UNIMPLEMENTED

    *StacksRemaining = FALSE;

    return NULL;
}

/*
 * @unimplemented
 */
UCHAR
EXPORT
NdisGeneratePartialCancelId(VOID)
{
    UNIMPLEMENTED

    return 0;
}

/*
 * @unimplemented
 */
VOID
EXPORT
NdisFreeToBlockPool(
    IN PUCHAR  Block)
{
    UNIMPLEMENTED
}

/*
 * @unimplemented
 */
VOID
EXPORT
NdisDestroyBlockPool(
    IN NDIS_HANDLE  BlockPoolHandle)
{
    UNIMPLEMENTED
}

/*
 * @unimplemented
 */
NDIS_HANDLE
EXPORT
NdisCreateBlockPool(
    IN USHORT  BlockSize,
    IN USHORT  FreeBlockLinkOffset,
    IN ULONG  Tag,
    IN NDIS_BLOCK_INITIALIZER  InitFunction OPTIONAL)
{
    UNIMPLEMENTED

    return NULL;
}

/*
 * @unimplemented
 */
PUCHAR
EXPORT
NdisAllocateFromBlockPool(
    IN NDIS_HANDLE  BlockPoolHandle)
{
    UNIMPLEMENTED

    return NULL;
}

/*
 * @unimplemented
 */
PVOID
EXPORT
NdisGetRoutineAddress(
    IN PUNICODE_STRING  NdisRoutineName)
{
    UNIMPLEMENTED

    return NULL;
}

/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisQueryBindInstanceName(
    OUT PNDIS_STRING  pAdapterInstanceName,
    IN NDIS_HANDLE  BindingContext)
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}

/*
 * @unimplemented
 */
VOID
EXPORT
NdisSetPacketPoolProtocolId(
    IN NDIS_HANDLE  PacketPoolHandle,
    IN UINT  ProtocolId)
{
    UNIMPLEMENTED
}

/*
 * @unimplemented
 */
VOID
EXPORT
NdisCompleteQueryStatistics(
    IN  NDIS_HANDLE     NdisAdapterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  NDIS_STATUS     Status)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisIMDeInitializeDeviceInstance(
    IN  NDIS_HANDLE NdisMiniportHandle)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
#undef NdisIMInitializeDeviceInstance
NDIS_STATUS
EXPORT
NdisIMInitializeDeviceInstance(
    IN  NDIS_HANDLE     DriverHandle,
    IN  PNDIS_STRING    DeviceInstance)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisIMRegisterLayeredMiniport(
    IN  NDIS_HANDLE                     NdisWrapperHandle,
    IN  PNDIS_MINIPORT_CHARACTERISTICS  MiniportCharacteristics,
    IN  UINT                            CharacteristicsLength,
    OUT PNDIS_HANDLE                    DriverHandle)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
#undef NdisMWanIndicateReceive
VOID
EXPORT
NdisMWanIndicateReceive(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  NDIS_HANDLE     NdisLinkContext,
    IN  PUCHAR          PacketBuffer,
    IN  UINT            PacketSize)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
#undef NdisMWanIndicateReceiveComplete
VOID
EXPORT
NdisMWanIndicateReceiveComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
#undef NdisMWanSendComplete
VOID
EXPORT
NdisMWanSendComplete(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  PNDIS_WAN_PACKET    Packet,
    IN  NDIS_STATUS         Status)
{
    UNIMPLEMENTED
}

/*
 * @unimplemented
 */
VOID
EXPORT
NdisRegisterTdiCallBack(
    IN  TDI_REGISTER_CALLBACK   RegisterCallback,
    IN  TDI_PNP_HANDLER         PnPHandler)
{
    UNIMPLEMENTED
}

/*
 * @unimplemented
 */
VOID
EXPORT
NdisDeregisterTdiCallBack(VOID)
{
    UNIMPLEMENTED
}

/*
 * @unimplemented
 */
VOID
EXPORT
NdisSetProtocolFilter(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  RECEIVE_HANDLER         ReceiveHandler,
    IN  RECEIVE_PACKET_HANDLER  ReceivePacketHandler,
    IN  NDIS_MEDIUM             Medium,
    IN  UINT                    Offset,
    IN  UINT                    Size,
    IN  PUCHAR                  Pattern)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisConvertStringToAtmAddress(
    OUT PNDIS_STATUS    Status,
    IN  PNDIS_STRING    String,
    OUT PATM_ADDRESS    AtmAddress)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
PNDIS_PACKET
EXPORT
NdisGetReceivedPacket(
    IN  PNDIS_HANDLE    NdisBindingHandle,
    IN  PNDIS_HANDLE    MacContext)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NULL;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMPromoteMiniport(
    IN  NDIS_HANDLE MiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMRemoveMiniport(
    IN  NDIS_HANDLE MiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMSetMiniportSecondary(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_HANDLE PrimaryMiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisIMAssociateMiniport(
    IN  NDIS_HANDLE DriverHandle,
    IN  NDIS_HANDLE ProtocolHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisIMCancelInitializeDeviceInstance(
    IN  NDIS_HANDLE     DriverHandle,
    IN  PNDIS_STRING    DeviceInstance)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisIMCopySendCompletePerPacketInfo(
    IN  PNDIS_PACKET    DstPacket,
    IN  PNDIS_PACKET    SrcPacket)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisIMCopySendPerPacketInfo(
    IN  PNDIS_PACKET    DstPacket,
    IN  PNDIS_PACKET    SrcPacket)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisIMDeregisterLayeredMiniport(
    IN  NDIS_HANDLE DriverHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_HANDLE
EXPORT
NdisIMGetBindingContext(
    IN  NDIS_HANDLE NdisBindingHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return (NDIS_HANDLE)NULL;
}


/*
 * @unimplemented
 */
NDIS_HANDLE
EXPORT
NdisIMGetDeviceContext(
    IN  NDIS_HANDLE MiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return (NDIS_HANDLE)NULL;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisIMInitializeDeviceInstanceEx(
    IN  NDIS_HANDLE     DriverHandle,
    IN  PNDIS_STRING    DriverInstance,
    IN  NDIS_HANDLE     DeviceContext   OPTIONAL)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisReturnPackets(
    IN  PNDIS_PACKET    *PacketsToReturn,
    IN  UINT            NumberOfPackets)
/*
 * FUNCTION: Releases ownership of one or more packets
 * ARGUMENTS:
 *     PacketsToReturn = Pointer to an array of pointers to packet descriptors
 *     NumberOfPackets = Number of pointers in descriptor pointer array
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
UINT
EXPORT
NdisPacketPoolUsage(
    IN  NDIS_HANDLE PoolHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMDeregisterIoPortRange(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  UINT        InitialPort,
    IN  UINT        NumberOfPorts,
    IN  PVOID       PortOffset)
/*
 * FUNCTION: Releases a register mapping to I/O ports
 * ARGUMENTS:
 *     MiniportAdapterHandle = Specifies handle input to MiniportInitialize
 *     InitialPort           = Bus-relative base port address of a range to be mapped
 *     NumberOfPorts         = Specifies number of ports to be mapped
 *     PortOffset            = Pointer to mapped base port address
 */
{
   UNIMPLEMENTED
}
