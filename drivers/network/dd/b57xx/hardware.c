/*
 * PROJECT:     ReactOS Broadcom NetXtreme Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware specific functions
 * COPYRIGHT:   Copyright 2021 Scott Maday <coldasdryice1@gmail.com>
 */

#include "nic.h"

#include "debug.h"

/* NIC FUNCTIONS **************************************************************/

NDIS_STATUS
NTAPI
NICInitializeAdapterResources(IN PB57XX_ADAPTER Adapter,
                              IN PNDIS_RESOURCE_LIST ResourceList)
{
    for (UINT n = 0; n < ResourceList->Count; n++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor = ResourceList->PartialDescriptors + n;

        switch (ResourceDescriptor->Type)
        {
            case CmResourceTypePort:
                ASSERT(Adapter->IoPortAddress == 0);
                ASSERT(ResourceDescriptor->u.Port.Start.HighPart == 0);

                Adapter->IoPortAddress = ResourceDescriptor->u.Port.Start.LowPart;
                Adapter->IoPortLength = ResourceDescriptor->u.Port.Length;

                NDIS_DbgPrint(MID_TRACE, ("I/O port range is %p to %p\n",
                                        Adapter->IoPortAddress,
                                        Adapter->IoPortAddress + Adapter->IoPortLength));
                break;
            case CmResourceTypeInterrupt:
                ASSERT(Adapter->InterruptVector == 0);
                ASSERT(Adapter->InterruptLevel == 0);

                Adapter->InterruptVector = ResourceDescriptor->u.Interrupt.Vector;
                Adapter->InterruptLevel = ResourceDescriptor->u.Interrupt.Level;
                Adapter->InterruptShared = (ResourceDescriptor->ShareDisposition ==
                                        CmResourceShareShared);
                Adapter->InterruptFlags = ResourceDescriptor->Flags;

                NDIS_DbgPrint(MID_TRACE, ("IRQ vector is %d\n", Adapter->InterruptVector));
                break;
            case CmResourceTypeMemory:
                /* Internal registers and memories (including PHY) */
                if (ResourceDescriptor->u.Memory.Length ==  (128 * 1024))
                {
                    ASSERT(Adapter->IoAddress.LowPart == 0);
                    ASSERT(ResourceDescriptor->u.Port.Start.HighPart == 0);


                    Adapter->IoAddress.QuadPart = ResourceDescriptor->u.Memory.Start.QuadPart;
                    Adapter->IoLength = ResourceDescriptor->u.Memory.Length;
                    NDIS_DbgPrint(MID_TRACE, ("Memory range is %I64x to %I64x\n",
                                            Adapter->IoAddress.QuadPart,
                                            Adapter->IoAddress.QuadPart + Adapter->IoLength));
                }
                break;

            default:
                NDIS_MinDbgPrint("Unrecognized resource type: 0x%x\n", ResourceDescriptor->Type);
                break;
        }
    }

    if (Adapter->IoAddress.QuadPart == 0 ||
        Adapter->IoPortAddress == 0 ||
        Adapter->InterruptVector == 0)
    {
        NDIS_MinDbgPrint("Adapter didn't receive enough resources\n");
        return NDIS_STATUS_RESOURCES;
    }

    return NDIS_STATUS_SUCCESS;
}

BOOLEAN
NTAPI
NICRecognizeHardware(IN PB57XX_ADAPTER Adapter)
{
    NDIS_MinDbgPrint("NICRecognizeHardware\n");

    return FALSE;
}

NDIS_STATUS
NTAPI
NICAllocateIoResources(IN PB57XX_ADAPTER Adapter)
{
    NDIS_STATUS Status;
    //ULONG AllocationSize;

    NDIS_MinDbgPrint("NICAllocateIoResources\n");

    Status = NdisMRegisterIoPortRange((PVOID*)&Adapter->IoPort,
                                      Adapter->MiniportAdapterHandle,
                                      Adapter->IoPortAddress,
                                      Adapter->IoPortLength);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_MinDbgPrint("Unable to register IO port range (0x%x)\n", Status);
        return NDIS_STATUS_RESOURCES;
    }

    Status = NdisMMapIoSpace((PVOID*)&Adapter->IoBase,
                             Adapter->MiniportAdapterHandle,
                             Adapter->IoAddress,
                             Adapter->IoLength);

    /*NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle,
                              sizeof(EB57XXTRANSMIT_DESCRIPTOR) * NUM_TRANSMIT_DESCRIPTORS,
                              FALSE,
                              (PVOID*)&Adapter->TransmitDescriptors,
                              &Adapter->TransmitDescriptorsPa);
    if (Adapter->TransmitDescriptors == NULL)
    {
        NDIS_MinDbgPrint("Unable to allocate transmit descriptors\n"));
        return NDIS_STATUS_RESOURCES;
    }

    for (UINT n = 0; n < NUM_TRANSMIT_DESCRIPTORS; ++n)
    {
        PEB57XXTRANSMIT_DESCRIPTOR Descriptor = Adapter->TransmitDescriptors + n;
        Descriptor->Address = 0;
        Descriptor->Length = 0;
    }

    NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle,
                              sizeof(EB57XXRECEIVE_DESCRIPTOR) * NUM_RECEIVE_DESCRIPTORS,
                              FALSE,
                              (PVOID*)&Adapter->ReceiveDescriptors,
                              &Adapter->ReceiveDescriptorsPa);
    if (Adapter->ReceiveDescriptors == NULL)
    {
        NDIS_MinDbgPrint("Unable to allocate receive descriptors\n");
        return NDIS_STATUS_RESOURCES;
    }

    AllocationSize = RcvBufAllocationSize(Adapter->ReceiveBufferType);
    ASSERT(Adapter->ReceiveBufferEntrySize == 0 ||
           Adapter->ReceiveBufferEntrySize == AllocationSize);
    Adapter->ReceiveBufferEntrySize = AllocationSize;

    NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle,
                              Adapter->ReceiveBufferEntrySize * NUM_RECEIVE_DESCRIPTORS,
                              FALSE,
                              (PVOID*)&Adapter->ReceiveBuffer,
                              &Adapter->ReceiveBufferPa);

    if (Adapter->ReceiveBuffer == NULL)
    {
        NDIS_MinDbgPrint("Unable to allocate receive buffer\n");
        return NDIS_STATUS_RESOURCES;
    }

    for (UINT n = 0; n < NUM_RECEIVE_DESCRIPTORS; ++n)
    {
        PEB57XXRECEIVE_DESCRIPTOR Descriptor = Adapter->ReceiveDescriptors + n;

        RtlZeroMemory(Descriptor, sizeof(*Descriptor));
        Descriptor->Address = Adapter->ReceiveBufferPa.QuadPart + n *
                              Adapter->ReceiveBufferEntrySize;
    }

    return NDIS_STATUS_SUCCESS;*/
    
    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICRegisterInterrupts(IN PB57XX_ADAPTER Adapter)
{
    NDIS_STATUS Status;

    NDIS_MinDbgPrint("NICRegisterInterrupts\n");

    Status = NdisMRegisterInterrupt(&Adapter->Interrupt,
                                    Adapter->MiniportAdapterHandle,
                                    Adapter->InterruptVector,
                                    Adapter->InterruptLevel,
                                    TRUE, // We always want ISR calls
                                    Adapter->InterruptShared,
                                    (Adapter->InterruptFlags & CM_RESOURCE_INTERRUPT_LATCHED) ?
                                    NdisInterruptLatched : NdisInterruptLevelSensitive);

    if (Status == NDIS_STATUS_SUCCESS)
    {
        Adapter->InterruptRegistered = TRUE;
    }

    return Status;
}

NDIS_STATUS
NTAPI
NICUnregisterInterrupts(IN PB57XX_ADAPTER Adapter)
{
    NDIS_MinDbgPrint("NICUnregisterInterrupts\n");

    if (Adapter->InterruptRegistered)
    {
        NdisMDeregisterInterrupt(&Adapter->Interrupt);
        Adapter->InterruptRegistered = FALSE;
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
NICReleaseIoResources(IN PB57XX_ADAPTER Adapter)
{
    NDIS_MinDbgPrint("NICReleaseIoResources\n");

    /*if (Adapter->ReceiveDescriptors != NULL)
    {
        // Disassociate our shared buffer before freeing it to avoid NIC-induced memory corruption
        if (Adapter->IoBase)
        {
            B57XXWriteUlong(Adapter, B57XX_REG_RDH, 0);
            B57XXWriteUlong(Adapter, B57XX_REG_RDT, 0);
        }

        NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle,
                              sizeof(B57XX_RECEIVE_DESCRIPTOR) * NUM_RECEIVE_DESCRIPTORS,
                              FALSE,
                              Adapter->ReceiveDescriptors,
                              Adapter->ReceiveDescriptorsPa);

        Adapter->ReceiveDescriptors = NULL;
    }

    if (Adapter->ReceiveBuffer != NULL)
    {
        NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle,
                              Adapter->ReceiveBufferEntrySize * NUM_RECEIVE_DESCRIPTORS,
                              FALSE,
                              Adapter->ReceiveBuffer,
                              Adapter->ReceiveBufferPa);

        Adapter->ReceiveBuffer = NULL;
        Adapter->ReceiveBufferEntrySize = 0;
    }


    if (Adapter->TransmitDescriptors != NULL)
    {
        // Disassociate our shared buffer before freeing it to avoid NIC-induced memory corruption
        if (Adapter->IoBase)
        {
            B57XXWriteUlong(Adapter, B57XX_REG_TDH, 0);
            B57XXWriteUlong(Adapter, B57XX_REG_TDT, 0);
        }

        NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle,
                              sizeof(B57XX_TRANSMIT_DESCRIPTOR) * NUM_TRANSMIT_DESCRIPTORS,
                              FALSE,
                              Adapter->TransmitDescriptors,
                              Adapter->TransmitDescriptorsPa);

        Adapter->TransmitDescriptors = NULL;
    }*/

    if (Adapter->IoPort)
    {
        NdisMDeregisterIoPortRange(Adapter->MiniportAdapterHandle,
                                   Adapter->IoPortAddress,
                                   Adapter->IoPortLength,
                                   Adapter->IoPort);
    }

    if (Adapter->IoBase)
    {
        NdisMUnmapIoSpace(Adapter->MiniportAdapterHandle, Adapter->IoBase, Adapter->IoLength);
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
NICPowerOn(IN PB57XX_ADAPTER Adapter)
{
    NDIS_MinDbgPrint("NICPowerOn\n");

    NDIS_STATUS Status = NICSoftReset(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        return Status;
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
NICSoftReset(IN PB57XX_ADAPTER Adapter)
{
    //ULONG Value;
    NDIS_MinDbgPrint("NICSoftReset\n");

    NICDisableInterrupts(Adapter);
    // TODO Write and read to device registers

    for (ULONG ResetAttempts = 0; ResetAttempts < MAX_RESET_ATTEMPTS; ResetAttempts++)
    {
        NdisStallExecution(1);
        /*B57XXReadUlong(Adapter, ???, &Value);
        if (!(Value & B57XX_CTRL_RST))
        {
            NDIS_MinDbgPrint("Device is back (%u)\n", ResetAttempts);

            NICDisableInterrupts(Adapter);
            // Clear out interrupts (the register is cleared upon read)
            // TODO Write and read to device registers

            return NDIS_STATUS_SUCCESS;
        }*/
    }

    NDIS_MinDbgPrint("Unable to recover device");
    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICEnableTxRx(IN PB57XX_ADAPTER Adapter)
{
    NDIS_MinDbgPrint("NICEnableTxRx\n");

    // TODO Write and read to device registers

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICDisableTxRx(IN PB57XX_ADAPTER Adapter)
{
    NDIS_MinDbgPrint("NICDisableTxRx\n");

    // TODO Write and read to device registers

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICGetPermanentMacAddress(IN PB57XX_ADAPTER Adapter,
                          OUT PUCHAR MacAddress)
{
    USHORT AddressWord;

    NDIS_MinDbgPrint("NICGetPermanentMacAddress\n");

    /* Should we read from RAL/RAH first? */
    for (UINT n = 0; n < (IEEE_802_ADDR_LENGTH / 2); n++)
    {
        if (!B57XXReadEeprom(Adapter, (UCHAR)n, &AddressWord))
        {
            return NDIS_STATUS_FAILURE;
        }
        Adapter->PermanentMacAddress[n * 2 + 0] = AddressWord & 0xff;
        Adapter->PermanentMacAddress[n * 2 + 1] = (AddressWord >> 8) & 0xff;
    }

    NDIS_MinDbgPrint("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                     Adapter->PermanentMacAddress[0],
                     Adapter->PermanentMacAddress[1],
                     Adapter->PermanentMacAddress[2],
                     Adapter->PermanentMacAddress[3],
                     Adapter->PermanentMacAddress[4],
                     Adapter->PermanentMacAddress[5]);
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
NICUpdateMulticastList(IN PB57XX_ADAPTER Adapter)
{
    NDIS_MinDbgPrint("NICUpdateMulticastList\n");

    for (UINT n = 0; n < MAXIMUM_MULTICAST_ADDRESSES; ++n)
    {
        /*ULONG Ral = *(ULONG*)Adapter->MulticastList[n].MacAddress;
        ULONG Rah = *(USHORT*)&Adapter->MulticastList[n].MacAddress[4];

        if (Rah || Ral)
        {
            Rah |= B57XX_RAH_AV;

            B57XXWriteUlong(Adapter, B57XX_REG_RAL + (8*n), Ral);
            B57XXWriteUlong(Adapter, B57XX_REG_RAH + (8*n), Rah);
        }
        else
        {
            B57XXWriteUlong(Adapter, B57XX_REG_RAH + (8*n), 0);
            B57XXWriteUlong(Adapter, B57XX_REG_RAL + (8*n), 0);
        }*/
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
NICApplyPacketFilter(IN PB57XX_ADAPTER Adapter)
{
    //ULONG FilterMask;
    NDIS_MinDbgPrint("NICApplyPacketFilter\n");

    /*B57XXReadUlong(Adapter, B57XX_REG_RCTL, &FilterMask);

    FilterMask &= ~B57XX_RCTL_FILTER_BITS;
    FilterMask |= PacketFilterToMask(Adapter->PacketFilter);
    B57XXWriteUlong(Adapter, B57XX_REG_RCTL, FilterMask);

    return NDIS_STATUS_SUCCESS;*/
    
    return NDIS_STATUS_FAILURE;
}

VOID
NTAPI
NICUpdateLinkStatus(IN PB57XX_ADAPTER Adapter)
{
    /*ULONG DeviceStatus;
    SIZE_T SpeedIndex;
    static ULONG SpeedValues[] = {10, 100, 1000, 1000};*/

    NDIS_MinDbgPrint("NICUpdateLinkStatus\n");

    /*B57XXReadUlong(Adapter, B57XX_REG_STATUS, &DeviceStatus);
    Adapter->MediaState = (DeviceStatus & B57XX_STATUS_LU) ?
                           NdisMediaStateConnected :
                           NdisMediaStateDisconnected;
    SpeedIndex = (DeviceStatus & B57XX_STATUS_SPEEDMASK) >> B57XX_STATUS_SPEEDSHIFT;
    Adapter->LinkSpeedMbps = SpeedValues[SpeedIndex];*/
}

/* B57XX FUNCTIONS ************************************************************/

BOOLEAN
B57XXReadEeprom(IN PB57XX_ADAPTER Adapter,
                IN UCHAR Address,
                USHORT *Result)
{
    //ULONG Value;

    for (UINT n = 0; n < MAX_EEPROM_READ_ATTEMPTS; n++)
    {
        NdisStallExecution(5);
        
        // TODO
        // B57XXReadEeprom(Adapter, ???, &Value);

        /*if (Value & B57XX_EERD_DONE)
        {
            break;
        }*/
    }
    /*if (!(Value & B57XX_EERD_DONE))
    {
        NDIS_MinDbgPrint("EEPROM Read incomplete\n");
        return FALSE;
    }*/
    //*Result = (USHORT)(Value >> B57XX_EERD_DATA_SHIFT);
    return TRUE;
}
