/*
 * PROJECT:     ReactOS Intel PRO/1000 Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Intel PRO/1000 driver definitions
 * COPYRIGHT:   Copyright 2013 Cameron Gutman (cameron.gutman@reactos.org)
 *              Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
 */

#ifndef _E1000_PCH_
#define _E1000_PCH_

#include <ndis.h>

#include "e1000hw.h"

#define E1000_TAG '001e'

#define MAXIMUM_FRAME_SIZE 1522
#define RECEIVE_BUFFER_SIZE          2048

#define DRIVER_VERSION 1


typedef struct _E1000_ADAPTER
{
    NDIS_SPIN_LOCK Lock;
    NDIS_HANDLE AdapterHandle;
    USHORT VendorID;
    USHORT DeviceID;
    USHORT SubsystemID;
    USHORT SubsystemVendorID;

    UCHAR PermanentMacAddress[IEEE_802_ADDR_LENGTH];
    UCHAR CurrentMacAddress[IEEE_802_ADDR_LENGTH];

    ULONG LinkSpeedMbps;
    ULONG MediaState;
    ULONG InterruptPending;

} E1000_ADAPTER, *PE1000_ADAPTER;


BOOLEAN
NTAPI
NICRecognizeHardware(
    IN PE1000_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICInitializeAdapterResources(
    IN PE1000_ADAPTER Adapter,
    IN PNDIS_RESOURCE_LIST ResourceList);

NDIS_STATUS
NTAPI
NICAllocateResources(
    IN PE1000_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICRegisterInterrupts(
    IN PE1000_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICUnregisterInterrupts(
    IN PE1000_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICReleaseIoResources(
    IN PE1000_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICPowerOn(
    IN PE1000_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICSoftReset(
    IN PE1000_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICEnableTxRx(
    IN PE1000_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICDisableTxRx(
    IN PE1000_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICGetPermanentMacAddress(
    IN PE1000_ADAPTER Adapter,
    OUT PUCHAR MacAddress);

NDIS_STATUS
NTAPI
NICApplyPacketFilter(
    IN PE1000_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICApplyInterruptMask(
    IN PE1000_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICDisableInterrupts(
    IN PE1000_ADAPTER Adapter);

USHORT
NTAPI
NICInterruptRecognized(
    IN PE1000_ADAPTER Adapter,
    OUT PBOOLEAN InterruptRecognized);

VOID
NTAPI
NICAcknowledgeInterrupts(
    IN PE1000_ADAPTER Adapter);

VOID
NTAPI
NICUpdateLinkStatus(
    IN PE1000_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICTransmitPacket(
    IN PE1000_ADAPTER Adapter,
    IN UCHAR TxDesc,
    IN ULONG PhysicalAddress,
    IN ULONG Length);

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
MiniportQueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded);

VOID
NTAPI
MiniportISR(
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueMiniportHandleInterrupt,
    IN NDIS_HANDLE MiniportAdapterContext);

VOID
NTAPI
MiniportHandleInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext);

#endif /* _E1000_PCH_ */
