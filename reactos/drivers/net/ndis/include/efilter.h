/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/miniport.h
 * PURPOSE:     Definitions for Ethernet filter
 */

#ifndef __EFILTER_H
#define __EFILTER_H

BOOLEAN
STDCALL
EthCreateFilter(
    IN  UINT                MaximumMulticastAddresses,
    IN  PUCHAR              AdapterAddress,
    OUT PETH_FILTER         * Filter);

VOID
STDCALL
EthFilterDprIndicateReceive(
    IN	PETH_FILTER Filter,
    IN	NDIS_HANDLE MacReceiveContext,
    IN	PCHAR       Address,
    IN	PVOID       HeaderBuffer,
    IN	UINT        HeaderBufferSize,
    IN	PVOID       LookaheadBuffer,
    IN	UINT        LookaheadBufferSize,
    IN	UINT        PacketSize);

VOID
STDCALL
EthFilterDprIndicateReceiveComplete(
    IN  PETH_FILTER Filter);

#endif /* __EFILTER_H */

/* EOF */

