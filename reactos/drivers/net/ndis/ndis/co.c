/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/co.c
 * PURPOSE:     Services for connection-oriented NDIS drivers
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>


NDIS_STATUS
STDCALL
NdisCoCreateVc(
    IN  NDIS_HANDLE         NdisBindingHandle,
    IN  NDIS_HANDLE         NdisAfHandle  OPTIONAL,
    IN  NDIS_HANDLE         ProtocolVcContext,
    IN  OUT PNDIS_HANDLE    NdisVcHandle)
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


NDIS_STATUS
STDCALL
NdisCoDeleteVc(
    IN  NDIS_HANDLE NdisVcHandle)
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


NDIS_STATUS
STDCALL
NdisCoRequest(
    IN      NDIS_HANDLE     NdisBindingHandle,
    IN      NDIS_HANDLE     NdisAfHandle    OPTIONAL,
    IN      NDIS_HANDLE     NdisVcHandle    OPTIONAL,
    IN      NDIS_HANDLE     NdisPartyHandle OPTIONAL,
    IN OUT  PNDIS_REQUEST   NdisRequest)
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


VOID
STDCALL
NdisCoRequestComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     NdisAfHandle,
    IN  NDIS_HANDLE     NdisVcHandle    OPTIONAL,
    IN  NDIS_HANDLE     NdisPartyHandle OPTIONAL,
    IN  PNDIS_REQUEST   NdisRequest)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


VOID
STDCALL
NdisCoSendPackets(
    IN  NDIS_HANDLE     NdisVcHandle,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}



VOID
STDCALL
NdisMCoActivateVcComplete(
    IN  NDIS_STATUS         Status,
    IN  NDIS_HANDLE         NdisVcHandle,
    IN  PCO_CALL_PARAMETERS CallParameters)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


VOID
STDCALL
NdisMCoDeactivateVcComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE NdisVcHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


VOID
STDCALL
NdisMCoIndicateReceivePacket(
    IN  NDIS_HANDLE     NdisVcHandle,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


VOID
STDCALL
NdisMCoIndicateStatus(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_HANDLE NdisVcHandle    OPTIONAL,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer    OPTIONAL,
    IN  ULONG       StatusBufferSize)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


VOID
STDCALL
NdisMCoReceiveComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


VOID
STDCALL
NdisMCoRequestComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_REQUEST   Request)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


VOID 
STDCALL
NdisMCoSendComplete(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     NdisVcHandle,
    IN  PNDIS_PACKET    Packet)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/* EOF */
