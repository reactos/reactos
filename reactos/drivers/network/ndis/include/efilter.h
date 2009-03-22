/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/miniport.h
 * PURPOSE:     Definitions for Ethernet filter
 */

#ifndef __EFILTER_H
#define __EFILTER_H

#define DECLARE_UNKNOWN_STRUCT(BaseName) \
  typedef struct _##BaseName BaseName, *P##BaseName;

#define DECLARE_UNKNOWN_PROTOTYPE(Name) \
  typedef VOID (*(Name))(VOID);

#define ETH_LENGTH_OF_ADDRESS 6

DECLARE_UNKNOWN_STRUCT(ETH_BINDING_INFO)

DECLARE_UNKNOWN_PROTOTYPE(ETH_ADDRESS_CHANGE)
DECLARE_UNKNOWN_PROTOTYPE(ETH_FILTER_CHANGE)
DECLARE_UNKNOWN_PROTOTYPE(ETH_DEFERRED_CLOSE)

typedef struct ETHI_FILTER {
  PNDIS_SPIN_LOCK  Lock;
  CHAR  (*MCastAddressBuf)[ETH_LENGTH_OF_ADDRESS];
  struct _NDIS_MINIPORT_BLOCK  *Miniport;
  UINT  CombinedPacketFilter;
  PETH_BINDING_INFO  OpenList;
  ETH_ADDRESS_CHANGE  AddressChangeAction;
  ETH_FILTER_CHANGE  FilterChangeAction;
  ETH_DEFERRED_CLOSE  CloseAction;
  UINT  MaxMulticastAddresses;
  UINT  NumAddresses;
  UCHAR AdapterAddress[ETH_LENGTH_OF_ADDRESS];
  UINT  OldCombinedPacketFilter;
  CHAR  (*OldMCastAddressBuf)[ETH_LENGTH_OF_ADDRESS];
  UINT  OldNumAddresses;
  PETH_BINDING_INFO  DirectedList;
  PETH_BINDING_INFO  BMList;
  PETH_BINDING_INFO  MCastSet;
#if defined(NDIS_WRAPPER)
  UINT  NumOpens;
  PVOID  BindListLock;
#endif
} ETHI_FILTER, *PETHI_FILTER;


BOOLEAN
NTAPI
EthCreateFilter(
    IN  UINT                MaximumMulticastAddresses,
    IN  PUCHAR              AdapterAddress,
    OUT PETH_FILTER         * Filter);

VOID
NTAPI
EthDeleteFilter(
    IN  PETH_FILTER Filter);

VOID
NTAPI
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
NTAPI
EthFilterDprIndicateReceiveComplete(
    IN  PETH_FILTER Filter);

#endif /* __EFILTER_H */

/* EOF */

