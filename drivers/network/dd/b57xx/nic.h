/*
 * PROJECT:     ReactOS Broadcom NetXtreme Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware driver definitions
 * COPYRIGHT:   Copyright 2021-2022 Scott Maday <coldasdryice1@gmail.com>
 */
#pragma once

#include <ndis.h>

#include "b57xxhw.h"

#define B57XX_TAG       '075b'
#define DRIVER_VERSION  1

#define PAYLOAD_SIZE_MINI       500
#define PAYLOAD_SIZE_STANDARD   1500
#define PAYLOAD_SIZE_JUMBO      9000

typedef struct _B57XX_ADAPTER
{
    /* NIC Memory */
    NDIS_HANDLE MiniportAdapterHandle;
    NDIS_HARDWARE_STATUS HardwareStatus;
    volatile PUCHAR IoBase;
    NDIS_PHYSICAL_ADDRESS IoAddress;
    ULONG IoLength;
    
    B57XX_DEVICE_ID DeviceID;
    BOOLEAN B5705Plus;
    BOOLEAN B5721Plus;
    BOOLEAN IsSerDes;
    
    struct _B57XX_PCI_STATE
    {
        USHORT VendorID;
        USHORT DeviceID;
        USHORT Command;
        UCHAR CacheLineSize;
        USHORT SubVendorID; 
        USHORT SubSystemID; 
        UCHAR RevisionID;
    } PciState;
    
    struct _B57XX_INTERRUPT
    {
        ULONG Vector;
        ULONG Level;
        BOOLEAN Shared;
        ULONG Flags;
        NDIS_MINIPORT_INTERRUPT Interrupt;
        BOOLEAN Registered;
        BOOLEAN Pending;
        NDIS_SPIN_LOCK Lock;
    } Interrupt;
    
    /* RX */
    B57XX_RECEIVE_PRODUCER_BLOCK MiniProducer;
    B57XX_RECEIVE_PRODUCER_BLOCK StandardProducer;
    B57XX_RECEIVE_PRODUCER_BLOCK JumboProducer;
    B57XX_RECEIVE_CONSUMER_BLOCK ReturnConsumer[1];
    
    /* TX */
    B57XX_SEND_BLOCK SendProducer[1];
    NDIS_SPIN_LOCK SendLock;
    
    /* Status */
    struct _B57XX_STATUS
    {
        PB57XX_STATUS_BLOCK pBlock;
        NDIS_PHYSICAL_ADDRESS HostAddress;
        ULONG LastTag;
    } Status;

    /* Statistics */
    struct _B57XX_STATISTICS
    {
        PB57XX_STATISTICS_BLOCK pBlock;
        NDIS_PHYSICAL_ADDRESS HostAddress;
        ULONG64 TransmitSuccesses;
        ULONG64 ReceiveSuccesses;
        ULONG64 ReceiveErrors;
        ULONG64 ReceiveBufferErrors;
    } Statistics;
    
    /* NIC Info */
    ULONG MaxFrameSize;
    MAC_ADDRESS MACAddresses[1];
    MAC_ADDRESS MulticastList[MAXIMUM_MULTICAST_ADDRESSES];
    ULONG MulticastListSize;
    
    ULONG PacketFilter;
    USHORT LinkStatus;
} B57XX_ADAPTER, *PB57XX_ADAPTER;

/* MINIPORT FUNCTIONS *****************************************************************************/

NDIS_STATUS
NTAPI
MiniportInitialize(_Out_ PNDIS_STATUS OpenErrorStatus,
                   _Out_ PUINT SelectedMediumIndex,
                   _In_  PNDIS_MEDIUM MediumArray,
                   _In_  UINT MediumArraySize,
                   _In_  NDIS_HANDLE MiniportAdapterHandle,
                   _In_  NDIS_HANDLE WrapperConfigurationContext);
             
NDIS_STATUS
NTAPI
MiniportReset(_Out_ PBOOLEAN AddressingReset,
              _In_  NDIS_HANDLE MiniportAdapterContext);

VOID
NTAPI
MiniportHalt(_In_ NDIS_HANDLE MiniportAdapterContext);

NDIS_STATUS
NTAPI
MiniportSend(_In_ NDIS_HANDLE MiniportAdapterContext,
             _In_ PNDIS_PACKET Packet,
             _In_ UINT Flags);

NDIS_STATUS
NTAPI
MiniportSetInformation(_In_  NDIS_HANDLE MiniportAdapterContext,
                       _In_  NDIS_OID Oid,
                       _In_  PVOID InformationBuffer,
                       _In_  ULONG InformationBufferLength,
                       _Out_ PULONG BytesRead,
                       _Out_ PULONG BytesNeeded);

NDIS_STATUS
NTAPI
MiniportQueryInformation(_In_  NDIS_HANDLE MiniportAdapterContext,
                         _In_  NDIS_OID Oid,
                         _In_  PVOID InformationBuffer,
                         _In_  ULONG InformationBufferLength,
                         _Out_ PULONG BytesWritten,
                         _Out_ PULONG BytesNeeded);

VOID
NTAPI
MiniportDisableInterrupt(_In_ NDIS_HANDLE MiniportAdapterContext);

VOID
NTAPI
MiniportEnableInterrupt(_In_ NDIS_HANDLE MiniportAdapterContext);

VOID
NTAPI
MiniportISR(_Out_ PBOOLEAN InterruptRecognized,
            _Out_ PBOOLEAN QueueMiniportHandleInterrupt,
            _In_  NDIS_HANDLE MiniportAdapterContext);

VOID
NTAPI
MiniportHandleInterrupt(_In_ NDIS_HANDLE MiniportAdapterContext);

/* NIC FUNCTIONS **********************************************************************************/

#define NICISDEVICE(Adapter, ...)                                                                  \
    NICIsDevice(Adapter,                                                                           \
                sizeof((B57XX_DEVICE_ID[]){__VA_ARGS__})/sizeof(B57XX_DEVICE_ID),                  \
                (B57XX_DEVICE_ID[]){__VA_ARGS__})

BOOLEAN
NTAPI
NICIsDevice(_In_ PB57XX_ADAPTER Adapter,
            _In_ ULONG DeviceIDListLength,
            _In_reads_(DeviceIDListLength) const B57XX_DEVICE_ID DeviceIDList[]);

_Check_return_
NDIS_STATUS
NTAPI
NICConfigureAdapter(_In_ PB57XX_ADAPTER Adapter);

_Check_return_
NDIS_STATUS
NTAPI
NICPowerOn(_In_ PB57XX_ADAPTER Adapter);

_Check_return_
NDIS_STATUS
NTAPI
NICSoftReset(_In_ PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICSetupPHY(_In_ PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICUpdateLinkStatus(_In_ PB57XX_ADAPTER Adapter);

VOID
NTAPI
NICSetupFlowControl(_In_ PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICShutdown(_In_ PB57XX_ADAPTER Adapter);

VOID
NTAPI
NICUpdateMACAddresses(_In_ PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICUpdateMulticastList(_In_ PB57XX_ADAPTER Adapter);

NDIS_STATUS
NTAPI
NICApplyPacketFilter(_In_ PB57XX_ADAPTER Adapter);

_Check_return_
NDIS_STATUS
NTAPI
NICTransmitPacket(_In_ PB57XX_ADAPTER Adapter,
                  _In_ PHYSICAL_ADDRESS PhysicalAddress,
                  _In_ ULONG Length,
                  _In_ USHORT Flags);

VOID
NTAPI
NICEnableInterrupts(_In_ PB57XX_ADAPTER Adapter);

VOID
NTAPI
NICDisableInterrupts(_In_ PB57XX_ADAPTER Adapter);

VOID
NTAPI
NICInterruptAcknowledge(_In_ PB57XX_ADAPTER Adapter);

BOOLEAN
NTAPI
NICInterruptCheckAvailability(_In_ PB57XX_ADAPTER Adapter);

VOID
NTAPI
NICInterruptSignalComplete(_In_ PB57XX_ADAPTER Adapter);

VOID
NTAPI
NICReceiveSignalComplete(_In_ PB57XX_ADAPTER Adapter);

VOID
NTAPI
NICQueryStatisticCounter(_In_  PB57XX_ADAPTER Adapter,
                         _In_  NDIS_OID Oid,
                         _Out_ PULONG64 pValue64);

NDIS_STATUS
NTAPI
NICFillPowerManagementCapabilities(_In_  PB57XX_ADAPTER Adapter,
                                   _Out_ PNDIS_PNP_CAPABILITIES Capabilities);

VOID
NTAPI
NICOutputDebugInfo(_In_ PB57XX_ADAPTER Adapter);

/* B57XX FUNCTIONS ********************************************************************************/

FORCEINLINE
ULONG
B57XXReadPciConfigULong(_In_  PB57XX_ADAPTER Adapter,
                        _In_  ULONG Offset,
                        _Out_ PULONG pValue)
{
    return NdisReadPciSlotInformation(Adapter->MiniportAdapterHandle,
                                      0,
                                      Offset,
                                      (PVOID)pValue,
                                      sizeof(ULONG));
}

FORCEINLINE
ULONG
B57XXWritePciConfigULong(_In_ PB57XX_ADAPTER Adapter,
                         _In_ ULONG Offset,
                         _In_ ULONG Value)
{
    return NdisWritePciSlotInformation(Adapter->MiniportAdapterHandle,
                                       0,
                                       Offset,
                                       (PVOID)&Value,
                                       sizeof(ULONG));
}

FORCEINLINE
VOID
B57XXEnableUShort(_In_ PB57XX_ADAPTER Adapter,
                  _In_ ULONG Address,
                  _Inout_ PULONG pValue,
                  _In_ ULONG Mask)
{
    NdisReadRegisterUshort((PUSHORT)(Adapter->IoBase + Address), pValue);
    *pValue |= Mask;
    NdisWriteRegisterUshort((PUSHORT)(Adapter->IoBase + Address), *pValue);
}

FORCEINLINE
VOID
B57XXReadULong(_In_  PB57XX_ADAPTER Adapter,
               _In_  ULONG Address,
               _Out_ PULONG pValue)
{
    NdisReadRegisterUlong((PULONG)(Adapter->IoBase + Address), pValue);
}

FORCEINLINE
VOID
B57XXWriteULong(_In_ PB57XX_ADAPTER Adapter,
                _In_ ULONG Address,
                _In_ ULONG Value)
{
    NdisWriteRegisterUlong((PULONG)(Adapter->IoBase + Address), Value);
}

FORCEINLINE
VOID
B57XXEnableULong(_In_ PB57XX_ADAPTER Adapter,
                 _In_ ULONG Address,
                 _Inout_ PULONG pValue,
                 _In_ ULONG Mask)
{
    B57XXReadULong(Adapter, Address, pValue);
    *pValue |= Mask;
    B57XXWriteULong(Adapter, Address, *pValue);
}

FORCEINLINE
VOID
B57XXReadRegister(_In_  PB57XX_ADAPTER Adapter,
                  _In_  ULONG Address,
                  _Out_ PULONG pValue)
{
    B57XXWritePciConfigULong(Adapter, B57XX_REG_REG_BASE, Address);
    B57XXReadPciConfigULong(Adapter, B57XX_REG_REG_DATA, pValue);
}

FORCEINLINE
VOID
B57XXWriteRegister(_In_ PB57XX_ADAPTER Adapter,
                   _In_ ULONG Address,
                   _In_ ULONG Value)
{
    B57XXWritePciConfigULong(Adapter, B57XX_REG_REG_BASE, Address);
    B57XXWritePciConfigULong(Adapter, B57XX_REG_REG_DATA, Value);
    
    B57XXWriteULong(Adapter, Address, Value);
}

FORCEINLINE
VOID
B57XXEnableRegister(_In_ PB57XX_ADAPTER Adapter,
                    _In_ ULONG Address,
                    _Inout_ PULONG pValue,
                    _In_ ULONG Mask)
{
    B57XXWritePciConfigULong(Adapter, B57XX_REG_REG_BASE, Address);
    B57XXReadPciConfigULong(Adapter, B57XX_REG_REG_DATA, pValue);
    *pValue |= Mask;
    B57XXWritePciConfigULong(Adapter, B57XX_REG_REG_DATA, *pValue);
}

FORCEINLINE
VOID
B57XXDisableRegister(_In_ PB57XX_ADAPTER Adapter,
                     _In_ ULONG Address,
                     _Inout_ PULONG pValue,
                     _In_ ULONG Mask)
{
    B57XXWritePciConfigULong(Adapter, B57XX_REG_REG_BASE, Address);
    B57XXReadPciConfigULong(Adapter, B57XX_REG_REG_DATA, pValue);
    *pValue &= ~Mask;
    B57XXWritePciConfigULong(Adapter, B57XX_REG_REG_DATA, *pValue);
}

FORCEINLINE
VOID
B57XXWriteMailbox(_In_ PB57XX_ADAPTER Adapter,
                  _In_ ULONG Address,
                  _In_ ULONG Value)
{
    B57XXWriteULong(Adapter, Address + 4, Value);
}

FORCEINLINE
VOID
B57XXReadMemoryULong(_In_ PB57XX_ADAPTER Adapter,
                     _In_ ULONG Address,
                     _Out_ PULONG pValue)
{
    B57XXWritePciConfigULong(Adapter, B57XX_REG_MEM_BASE, Address);
    B57XXReadPciConfigULong(Adapter, B57XX_REG_MEM_DATA, pValue);
}

FORCEINLINE
VOID
B57XXWriteMemoryULong(_In_ PB57XX_ADAPTER Adapter,
                      _In_ ULONG Address,
                      _In_ ULONG Value)
{
    B57XXWritePciConfigULong(Adapter, B57XX_REG_MEM_BASE, Address);
    B57XXWritePciConfigULong(Adapter, B57XX_REG_MEM_DATA, Value);
}

FORCEINLINE
VOID
B57XXConvertAddress(_Out_ PB57XX_PHYSICAL_ADDRESS pDestination,
                    _In_  PNDIS_PHYSICAL_ADDRESS pAddress)
{
    pDestination->HighPart = pAddress->HighPart;
    pDestination->LowPart = pAddress->LowPart;
}

VOID
B57XXWriteMemoryRCB(_In_ PB57XX_ADAPTER Adapter,
                    _In_ ULONG Address,
                    _In_ volatile PB57XX_RING_CONTROL_BLOCK pRCB);

NDIS_STATUS
B57XXReadPHY(_In_  PB57XX_ADAPTER Adapter,
             _In_  USHORT PHYRegOffset,
             _Out_ PUSHORT pValue);

NDIS_STATUS
B57XXWritePHY(_In_ PB57XX_ADAPTER Adapter,
              _In_ USHORT PHYRegOffset,
              _In_ USHORT Value);

VOID
B57XXReadAndClearPHYInterrupts(_In_  PB57XX_ADAPTER Adapter,
                               _Out_ PUSHORT Interrupts);

VOID
B57XXClearMACAttentions(_In_ PB57XX_ADAPTER Adapter);

VOID
B57XXReadStatistic(_In_  PB57XX_ADAPTER Adapter,
                   _In_  ULONG BlockOffset,
                   _In_  ULONG RegOffset,
                   _In_  BOOLEAN Is64Bit,
                   _Out_ PULONG64 pValue64);

VOID
B57XXAccumulateAddress(_Out_ PB57XX_PHYSICAL_ADDRESS pDestination,
                       _In_  LONG Offset);

ULONG
B57XXComputeInverseCrc32(_In_ PUCHAR pBuffer,
                         _In_ ULONG BufferSize);

/* EOF */
