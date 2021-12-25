/*
 * PROJECT:     ReactOS Broadcom NetXtreme Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware driver definitions
 * COPYRIGHT:   Copyright 2021 Scott Maday <coldasdryice1@gmail.com>
 */
#pragma once

#include <ndis.h>

#include "b57xxhw.h"

#define B57XX_TAG '075b'

#define MAXIMUM_FRAME_SIZE 1522
#define RECEIVE_BUFFER_SIZE 2048

#define DRIVER_VERSION 1

#define DEFAULT_INTERRUPT_MASK  0

typedef struct _B57XX_ADAPTER
{
    /* NIC Memory */
    NDIS_HANDLE MiniportAdapterHandle;
    volatile PUCHAR IoBase;
    NDIS_PHYSICAL_ADDRESS IoAddress;
    ULONG IoLength;

    UCHAR PermanentMacAddress[IEEE_802_ADDR_LENGTH];
    
    struct {
        UCHAR MacAddress[IEEE_802_ADDR_LENGTH];
    } MulticastList[MAXIMUM_MULTICAST_ADDRESSES];
    ULONG MulticastListSize;

    ULONG LinkSpeedMbps;
    ULONG MediaState;
    ULONG PacketFilter;

    /* Io Port */
    ULONG IoPortAddress;
    ULONG IoPortLength;
    volatile PUCHAR IoPort;

    /* Interrupt */
    ULONG InterruptVector;
    ULONG InterruptLevel;
    BOOLEAN InterruptShared;
    ULONG InterruptFlags;

    NDIS_MINIPORT_INTERRUPT Interrupt;
    BOOLEAN InterruptRegistered;

    LONG InterruptMask;
} B57XX_ADAPTER, *PB57XX_ADAPTER;


/* MINIPORT FUNCTIONS *********************************************************/

VOID
NTAPI
MiniportHalt(IN NDIS_HANDLE MiniportAdapterContext);

NDIS_STATUS
NTAPI
MiniportInitialize(OUT PNDIS_STATUS OpenErrorStatus,
                   OUT PUINT SelectedMediumIndex,
                   IN PNDIS_MEDIUM MediumArray,
                   IN UINT MediumArraySize,
                   IN NDIS_HANDLE MiniportAdapterHandle,
                   IN NDIS_HANDLE WrapperConfigurationContext);

NDIS_STATUS
NTAPI
MiniportSetInformation(IN NDIS_HANDLE MiniportAdapterContext,
                       IN NDIS_OID Oid,
                       IN PVOID InformationBuffer,
                       IN ULONG InformationBufferLength,
                       OUT PULONG BytesRead,
                       OUT PULONG BytesNeeded);

NDIS_STATUS
NTAPI
MiniportQueryInformation(IN NDIS_HANDLE MiniportAdapterContext,
                         IN NDIS_OID Oid,
                         IN PVOID InformationBuffer,
                         IN ULONG InformationBufferLength,
                         OUT PULONG BytesWritten,
                         OUT PULONG BytesNeeded);

NDIS_STATUS
NTAPI
MiniportSetInformation(IN NDIS_HANDLE MiniportAdapterContext,
                       IN NDIS_OID Oid,
                       IN PVOID InformationBuffer,
                       IN ULONG InformationBufferLength,
                       OUT PULONG BytesRead,
                       OUT PULONG BytesNeeded);


VOID
NTAPI
MiniportISR(OUT PBOOLEAN InterruptRecognized,
            OUT PBOOLEAN QueueMiniportHandleInterrupt,
            IN NDIS_HANDLE MiniportAdapterContext);
VOID
NTAPI
MiniportHandleInterrupt(IN NDIS_HANDLE MiniportAdapterContext);

NDIS_STATUS
NTAPI
MiniportSend(IN NDIS_HANDLE MiniportAdapterContext,
             IN PNDIS_PACKET Packet,
             IN UINT Flags);


/* NIC FUNCTIONS **************************************************************/

BOOLEAN
NTAPI
NICRecognizeHardware(IN PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICInitializeAdapterResources(IN PB57XX_ADAPTER Adapter,
                              IN PNDIS_RESOURCE_LIST ResourceList);

NDIS_STATUS
NTAPI
NICAllocateIoResources(IN PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICRegisterInterrupts(IN PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICUnregisterInterrupts(IN PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICReleaseIoResources(IN PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICPowerOn(IN PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICSoftReset(IN PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICEnableTxRx(IN PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICDisableTxRx(IN PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICGetPermanentMacAddress(IN PB57XX_ADAPTER Adapter,
                          OUT PUCHAR MacAddress);

NDIS_STATUS
NTAPI
NICUpdateMulticastList(IN PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICApplyPacketFilter(IN PB57XX_ADAPTER Adapter);

VOID
NTAPI
NICUpdateLinkStatus(IN PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICTransmitPacket(IN PB57XX_ADAPTER Adapter,
                  IN PHYSICAL_ADDRESS PhysicalAddress,
                  IN ULONG Length);

/* B57XX FUNCTIONS ************************************************************/

BOOLEAN
B57XXReadEeprom(IN PB57XX_ADAPTER Adapter,
                IN UCHAR Address,
                USHORT *Result);

FORCEINLINE
VOID
B57XXReadUlong(IN PB57XX_ADAPTER Adapter,
               IN ULONG Address,
               OUT PULONG Value)
{
    NdisReadRegisterUlong((PULONG)(Adapter->IoBase + Address), Value);
}

FORCEINLINE
VOID
B57XXWriteUlong(IN PB57XX_ADAPTER Adapter,
                IN ULONG Address,
                IN ULONG Value)
{
    NdisWriteRegisterUlong((PULONG)(Adapter->IoBase + Address), Value);
}

FORCEINLINE
VOID
B57XXWriteIoUlong(IN PB57XX_ADAPTER Adapter,
                  IN ULONG Address,
                  IN ULONG Value)
{
    //volatile ULONG Dummy;

    NdisRawWritePortUlong((PULONG)(Adapter->IoPort), Address);
    //NdisReadRegisterUlong(Adapter->IoBase + B57XX_REG_STATUS, &Dummy); TODO
    NdisRawWritePortUlong((PULONG)(Adapter->IoPort + 4), Value);
}

FORCEINLINE
VOID
NICApplyInterruptMask(IN PB57XX_ADAPTER Adapter)
{
    //B57XXWriteUlong(Adapter, B57XX_REG_IMS, Adapter->InterruptMask);
}

FORCEINLINE
VOID
NICDisableInterrupts(IN PB57XX_ADAPTER Adapter)
{
    //B57XXWriteUlong(Adapter, B57XX_REG_IMC, ~0);
}