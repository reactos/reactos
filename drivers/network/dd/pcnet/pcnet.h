/*
 * ReactOS AMD PCNet Driver
 *
 * Copyright (C) 2003 Vizzini <vizzini@plasmic.com>
 * Copyright (C) 2004 Filip Navara <navaraf@reactos.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * REVISIONS:
 *     01-Sep-2003 vizzini - Created
 * NOTES:
 *     - this assumes a 32-bit machine, where sizeof(PVOID) = 32 and sizeof(USHORT) = 16
 *     - this assumes 32-bit physical addresses
 */

#ifndef _PCNET_PCH_
#define _PCNET_PCH_

#include <ndis.h>

#include "pci.h"
#include "pcnethw.h"

/* statistics struct */
typedef struct _ADAPTER_STATS
{
  ULONG XmtGoodFrames;
  ULONG XmtRetryErrors;
  ULONG XmtLossesOfCarrier;
  ULONG XmtCollisions;
  ULONG XmtLateCollisions;
  ULONG XmtExcessiveDeferrals;
  ULONG XmtBufferUnderflows;
  ULONG XmtBufferErrors;
  ULONG XmtOneRetry;
  ULONG XmtMoreThanOneRetry;
  ULONG RcvGoodFrames;
  ULONG RcvBufferErrors;
  ULONG RcvCrcErrors;
  ULONG RcvOverflowErrors;
  ULONG RcvFramingErrors;
} ADAPTER_STATS, *PADAPTER_STATS;

/* adapter struct */
typedef struct _ADAPTER
{
  NDIS_SPIN_LOCK Lock;

  NDIS_HANDLE MiniportAdapterHandle;
  ULONG Flags;
  ULONG InterruptVector;
  ULONG IoBaseAddress;
  ULONG_PTR PortOffset;
  NDIS_MINIPORT_INTERRUPT InterruptObject;
  NDIS_MEDIA_STATE MediaState;
  UINT MediaSpeed;
  BOOLEAN FullDuplex;
  NDIS_MINIPORT_TIMER MediaDetectionTimer;
  ULONG CurrentReceiveDescriptorIndex;
  ULONG CurrentPacketFilter;
  ULONG CurrentLookaheadSize;

  /* circular indexes to transmit descriptors */
  ULONG CurrentTransmitStartIndex;
  ULONG CurrentTransmitEndIndex;

  /* initialization block */
  ULONG InitializationBlockLength;
  PINITIALIZATION_BLOCK InitializationBlockVirt;
  PHYSICAL_ADDRESS InitializationBlockPhys;

  /* transmit descriptor ring */
  ULONG TransmitDescriptorRingLength;
  PTRANSMIT_DESCRIPTOR TransmitDescriptorRingVirt;
  PHYSICAL_ADDRESS TransmitDescriptorRingPhys;

  /* transmit buffers */
  ULONG TransmitBufferLength;
  PCHAR TransmitBufferPtrVirt;
  PHYSICAL_ADDRESS TransmitBufferPtrPhys;

  /* receive descriptor ring */
  ULONG ReceiveDescriptorRingLength;
  PRECEIVE_DESCRIPTOR ReceiveDescriptorRingVirt;
  PHYSICAL_ADDRESS ReceiveDescriptorRingPhys;

  /* receive buffers */
  ULONG ReceiveBufferLength;
  PCHAR ReceiveBufferPtrVirt;
  PHYSICAL_ADDRESS ReceiveBufferPtrPhys;

  /* buffer count */
  ULONG BufferCount;
  ULONG LogBufferCount;

  ADAPTER_STATS Statistics;
} ADAPTER, *PADAPTER;

/* forward declarations */
NDIS_STATUS
NTAPI
MiniportQueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded);

NDIS_STATUS
NTAPI
MiniportSetInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded);

NDIS_STATUS
NTAPI
MiSetMulticast(
    PADAPTER Adapter,
    UCHAR *Addresses,
    UINT AddressCount);

NDIS_MEDIA_STATE
NTAPI
MiGetMediaState(PADAPTER Adapter);

UINT
NTAPI
MiGetMediaSpeed(PADAPTER Adapter);

BOOLEAN
NTAPI
MiGetMediaDuplex(PADAPTER Adapter);

/* operational constants */
#define NUMBER_OF_BUFFERS     0x20
#define LOG_NUMBER_OF_BUFFERS 5         /* log2(NUMBER_OF_BUFFERS) */
#define BUFFER_SIZE           0x600
#define MAX_MULTICAST_ADDRESSES 32
#define MEDIA_DETECTION_INTERVAL 5000

/* flags */
#define RESET_IN_PROGRESS 0x1

/* Maximum number of interrupts handled per call to MiniportHandleInterrupt */
#define INTERRUPT_LIMIT 10

/* memory pool tag */
#define PCNET_TAG 'tNcP'

#endif /* _PCNET_PCH_ */
