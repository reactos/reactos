/*
 * PROJECT:     ReactOS Broadcom NetXtreme Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Driver entrypoint and miscellaneous miniport functions
 * COPYRIGHT:   Copyright 2021-2022 Scott Maday <coldasdryice1@gmail.com>
 */

#include "nic.h"

#define NDEBUG
#include <debug.h>

ULONG DebugTraceLevel = MIN_TRACE;

static
NDIS_STATUS
InitAdapterResources(_In_ PB57XX_ADAPTER Adapter,
                     _In_ PNDIS_RESOURCE_LIST ResourceList)
{
    for (UINT n = 0; n < ResourceList->Count; n++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor = ResourceList->PartialDescriptors + n;

        switch (ResourceDescriptor->Type)
        {
            case CmResourceTypeInterrupt:
                ASSERT(Adapter->Interrupt.Vector == 0);
                ASSERT(Adapter->Interrupt.Level == 0);

                Adapter->Interrupt.Vector = ResourceDescriptor->u.Interrupt.Vector;
                Adapter->Interrupt.Level = ResourceDescriptor->u.Interrupt.Level;
                Adapter->Interrupt.Shared = (ResourceDescriptor->ShareDisposition ==
                                           CmResourceShareShared);
                Adapter->Interrupt.Flags = ResourceDescriptor->Flags;
                break;
            case CmResourceTypeMemory:
                ASSERT(Adapter->IoAddress.LowPart == 0);

                Adapter->IoAddress.QuadPart = ResourceDescriptor->u.Memory.Start.QuadPart;
                Adapter->IoLength = ResourceDescriptor->u.Memory.Length;
                break;

            default:
                break;
        }
    }

    if (Adapter->IoAddress.QuadPart == 0 || Adapter->Interrupt.Vector == 0)
    {
        NDIS_MinDbgPrint("Adapter didn't receive enough resources\n");
        return NDIS_STATUS_RESOURCES;
    }

    return NDIS_STATUS_SUCCESS;
}

static
NDIS_STATUS
InitAllocateSharedMemory(_In_  PB57XX_ADAPTER Adapter,
                         _In_  ULONG Length,
                         _Out_ PVOID* pVirtualAddress,
                         _Out_ PB57XX_PHYSICAL_ADDRESS pPhysicalAddress)
{
    NDIS_PHYSICAL_ADDRESS Tmp;
    
    NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle, Length, FALSE, pVirtualAddress, &Tmp);
    if (*pVirtualAddress == NULL)
    {
        return NDIS_STATUS_RESOURCES;
    }
    
    NdisZeroMemory(*pVirtualAddress, Length);
    
    B57XXConvertAddress(pPhysicalAddress, &Tmp);
    
    return NDIS_STATUS_SUCCESS;
}

static
NDIS_STATUS
FreeSharedMemory(_In_ PB57XX_ADAPTER Adapter,
                 _In_ ULONG Length,
                 _In_ PVOID VirtualAddress,
                 _In_ PB57XX_PHYSICAL_ADDRESS pPhysicalAddress)
{
    NDIS_PHYSICAL_ADDRESS Tmp;
    
    if (VirtualAddress == NULL)
    {
        return NDIS_STATUS_SOFT_ERRORS;
    }
    
    Tmp.HighPart = pPhysicalAddress->HighPart;
    Tmp.LowPart = pPhysicalAddress->LowPart;
    
    NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle, Length, FALSE, VirtualAddress, Tmp);
    
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
MiniportInitialize(_Out_ PNDIS_STATUS OpenErrorStatus,
                   _Out_ PUINT SelectedMediumIndex,
                   _In_  PNDIS_MEDIUM MediumArray,
                   _In_  UINT MediumArraySize,
                   _In_  NDIS_HANDLE MiniportAdapterHandle,
                   _In_  NDIS_HANDLE WrapperConfigurationContext)
{
    NDIS_STATUS Status;
    PB57XX_ADAPTER Adapter;
    PCI_COMMON_CONFIG PCIConfig;
    UINT i;
    PNDIS_RESOURCE_LIST ResourceList = NULL;
    UINT ResourceListSize = 0;

    /* Make sure the medium is supported */
    for (i = 0; i < MediumArraySize; i++)
    {
        if (MediumArray[i] == NdisMedium802_3)
        {
            *SelectedMediumIndex = i;
            break;
        }
    }
    if (i == MediumArraySize)
    {
        NDIS_MinDbgPrint("802.3 medium was not found in the medium array\n");
        return NDIS_STATUS_UNSUPPORTED_MEDIA;
    }
    
    /* Allocate our adapter context */
    Status = NdisAllocateMemoryWithTag((PVOID*)&Adapter, sizeof(B57XX_ADAPTER), B57XX_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_MinDbgPrint("Failed to allocate adapter context (0x%x)\n", Status);
        return NDIS_STATUS_RESOURCES;
    }
    
    NdisZeroMemory(Adapter, sizeof(*Adapter));
    Adapter->MiniportAdapterHandle = MiniportAdapterHandle;
    Adapter->HardwareStatus = NdisHardwareStatusNotReady;
    NdisAllocateSpinLock(&Adapter->Interrupt.Lock);
    NdisAllocateSpinLock(&Adapter->SendLock);
    NdisAllocateSpinLock(&Adapter->InfoLock);
    
    /* Notify NDIS of some characteristics of our NIC */
    NdisMSetAttributesEx(Adapter->MiniportAdapterHandle,
                         Adapter,
                         0,
                         NDIS_ATTRIBUTE_BUS_MASTER,
                         NdisInterfacePci);
    
    /* Get PCI configuration of our NIC */
    NdisReadPciSlotInformation(Adapter->MiniportAdapterHandle,
                               0,
                               FIELD_OFFSET(PCI_COMMON_CONFIG, VendorID),
                               &PCIConfig,
                               sizeof(PCIConfig));
    Adapter->PciState.VendorID = PCIConfig.VendorID;
    Adapter->PciState.DeviceID = PCIConfig.DeviceID;
    Adapter->PciState.Command = PCIConfig.Command;
    Adapter->PciState.CacheLineSize = PCIConfig.CacheLineSize;
    Adapter->PciState.RevisionID = PCIConfig.RevisionID;
    Adapter->PciState.SubVendorID = PCIConfig.u.type0.SubVendorID;
    Adapter->PciState.SubSystemID = PCIConfig.u.type0.SubSystemID;

    /* Get our resources for IRQ and IO base information */
    NdisMQueryAdapterResources(&Status,
                               WrapperConfigurationContext,
                               ResourceList,
                               &ResourceListSize);
    if (Status != NDIS_STATUS_RESOURCES)
    {
        NDIS_MinDbgPrint("Unexpected failure of NdisMQueryAdapterResources (0x%x)\n", Status);
        Status = NDIS_STATUS_FAILURE;
        goto Cleanup;
    }

    Status = NdisAllocateMemoryWithTag((PVOID*)&ResourceList, ResourceListSize, B57XX_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_MinDbgPrint("Failed to allocate resource list (0x%x)\n", Status);
        goto Cleanup;
    }
    
    NdisMQueryAdapterResources(&Status,
                               WrapperConfigurationContext,
                               ResourceList,
                               &ResourceListSize);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_MinDbgPrint("Unexpected failure of NdisMQueryAdapterResources (0x%x)\n", Status);
        goto Cleanup;
    }

    ASSERT(ResourceList->Version == 1);
    ASSERT(ResourceList->Revision == 1);
    
    /* Begin hardware initialization */
    Status = InitAdapterResources(Adapter, ResourceList);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        goto Cleanup;
    }

    NdisFreeMemory(ResourceList, ResourceListSize, 0);
    ResourceList = NULL;
    
    Status = NICConfigureAdapter(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        goto Cleanup;
    }
    
    /* Map physical IO address */
    Status = NdisMMapIoSpace((PVOID*)&Adapter->IoBase,
                             Adapter->MiniportAdapterHandle,
                             Adapter->IoAddress,
                             Adapter->IoLength);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_MinDbgPrint("Unable to map IO memory (0x%x)\n", Status);
        return Status;
    }
    
    /* Allocate the DMA resources */
    Status = NdisMInitializeScatterGatherDma(MiniportAdapterHandle,
                                             FALSE, 
                                             Adapter->MaxFrameSize);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_MinDbgPrint("Unable to configure DMA\n");
        Status = NDIS_STATUS_RESOURCES;
        goto Cleanup;
    }
    
    /* Allocate standard receive producer ring & buffer */
    Status = InitAllocateSharedMemory(Adapter,
                                      sizeof(B57XX_RECEIVE_BUFFER_DESCRIPTOR) * 
                                      Adapter->StandardProducer.Count,
                                      (PVOID*)&Adapter->StandardProducer.pRing,
                                      &Adapter->StandardProducer.RingControlBlock.HostRingAddress);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_MinDbgPrint("Unable to allocate standard receive consumer ring");
        goto Cleanup;
    }
    
    Status = InitAllocateSharedMemory(Adapter,
                                      Adapter->StandardProducer.FrameBufferLength *
                                      Adapter->StandardProducer.Count,
                                      (PVOID*)&Adapter->StandardProducer.HostBuffer,
                                      &Adapter->StandardProducer.pRing[0].HostAddress);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_MinDbgPrint("Unable to allocate host receive buffers");
        goto Cleanup;
    }
    
    for (i = 0; i < Adapter->StandardProducer.Count; i++)
    {
        if (i != 0)
        {
            NdisMoveMemory(&Adapter->StandardProducer.pRing[i].HostAddress,
                           &Adapter->StandardProducer.pRing[0].HostAddress,
                           sizeof(B57XX_PHYSICAL_ADDRESS));
            B57XXAccumulateAddress(&Adapter->StandardProducer.pRing[i].HostAddress,
                                   i * Adapter->StandardProducer.FrameBufferLength);
        }
        Adapter->StandardProducer.pRing[i].Index = i;
        Adapter->StandardProducer.pRing[i].Length = Adapter->StandardProducer.FrameBufferLength;
        Adapter->StandardProducer.pRing[i].Flags = B57XX_RBD_PACKET_END;
    }
    
    /* Allocate receive return consumer ring */
    Status = InitAllocateSharedMemory(Adapter,
                                      sizeof(B57XX_RECEIVE_BUFFER_DESCRIPTOR) * 
                                      Adapter->ReturnConsumer[0].Count,
                                      (PVOID*)&Adapter->ReturnConsumer[0].pRing,
                                      &Adapter->ReturnConsumer[0].RingControlBlock.HostRingAddress);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_MinDbgPrint("Unable to allocate receive return ring");
        goto Cleanup;
    }
    
    /* Allocate send producer ring & packet list */
    Status = InitAllocateSharedMemory(Adapter,
                                      sizeof(B57XX_SEND_BUFFER_DESCRIPTOR) * 
                                      Adapter->SendProducer[0].Count,
                                      (PVOID*)&Adapter->SendProducer[0].pRing,
                                      &Adapter->SendProducer[0].RingControlBlock.HostRingAddress);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_MinDbgPrint("Unable to allocate send ring");
        goto Cleanup;
    }
    
    Status = NdisAllocateMemoryWithTag((PVOID*)&Adapter->SendProducer[0].pPacketList,
                                       sizeof(PNDIS_PACKET) * Adapter->SendProducer[0].Count,
                                       B57XX_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_MinDbgPrint("Unable to allocate transmit packet pointer list");
        goto Cleanup;
    }
    NdisZeroMemory(Adapter->SendProducer[0].pPacketList,
                   sizeof(PNDIS_PACKET) * Adapter->SendProducer[0].Count);
    
    /* Allocate status block */
    NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle,
                              sizeof(B57XX_STATUS_BLOCK),
                              FALSE,
                              (PVOID*)&Adapter->Status.pBlock,
                              &Adapter->Status.HostAddress);
    if (Adapter->Status.pBlock == NULL)
    {
        NDIS_MinDbgPrint("Unable to allocate status block");
        goto Cleanup;
    }
    NdisZeroMemory(Adapter->Status.pBlock, sizeof(B57XX_STATUS_BLOCK));
    
    /* Allocate statistics block */
    if (!Adapter->B5705Plus)
    {
        NdisMAllocateSharedMemory(Adapter->MiniportAdapterHandle,
                                  sizeof(B57XX_STATISTICS_BLOCK),
                                  FALSE,
                                  (PVOID*)&Adapter->Statistics.pBlock,
                                  &Adapter->Statistics.HostAddress);
        if (Adapter->Statistics.pBlock == NULL)
        {
            NDIS_MinDbgPrint("Unable to allocate statistics block");
            goto Cleanup;
        }
        NdisZeroMemory(Adapter->Statistics.pBlock, sizeof(B57XX_STATISTICS_BLOCK));
    }
    
    /* Adapter setup */
    Status = NICPowerOn(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_MinDbgPrint("Unable to power on NIC (0x%x)\n", Status);
        goto Cleanup;
    }

    /* We're ready to handle interrupts now */
    Status = NdisMRegisterInterrupt(&Adapter->Interrupt.Interrupt,
                                    Adapter->MiniportAdapterHandle,
                                    Adapter->Interrupt.Vector,
                                    Adapter->Interrupt.Level,
                                    TRUE, // We always want ISR calls
                                    Adapter->Interrupt.Shared,
                                    (Adapter->Interrupt.Flags & CM_RESOURCE_INTERRUPT_LATCHED) ?
                                    NdisInterruptLatched : NdisInterruptLevelSensitive);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_MinDbgPrint("Unable to register interrupt (0x%x)\n", Status);
        goto Cleanup;
    }
    Adapter->Interrupt.Registered = TRUE;
    
    NICEnableInterrupts(Adapter);

    return NDIS_STATUS_SUCCESS;

Cleanup:
    if (ResourceList != NULL)
    {
        NdisFreeMemory(ResourceList, ResourceListSize, 0);
    }

    MiniportHalt(Adapter);

    return Status;
}

NDIS_STATUS
NTAPI
MiniportReset(_Out_ PBOOLEAN AddressingReset,
              _In_ NDIS_HANDLE MiniportAdapterContext)
{
    NDIS_STATUS Status;
    PB57XX_ADAPTER Adapter = (PB57XX_ADAPTER)MiniportAdapterContext;
    
    NDIS_MinDbgPrint("B57XX Reset\n");

    *AddressingReset = TRUE;
    
    Status = NICSoftReset(Adapter);
    if (Status == NDIS_STATUS_SUCCESS)
    {
        NICEnableInterrupts(Adapter);
    }
    
    return Status;
}

VOID
NTAPI
MiniportHalt(_In_ NDIS_HANDLE MiniportAdapterContext)
{
    PB57XX_ADAPTER Adapter = (PB57XX_ADAPTER)MiniportAdapterContext;
    
    NDIS_MinDbgPrint("B57XX Halt\n");

    ASSERT(Adapter != NULL);
    
    Adapter->HardwareStatus = NdisHardwareStatusNotReady;
    
    /* Unregister interrupts */
    if (Adapter->Interrupt.Registered)
    {
        NICDisableInterrupts(Adapter);
        NdisMDeregisterInterrupt(&Adapter->Interrupt.Interrupt);
        Adapter->Interrupt.Registered = FALSE;
    }

    NICShutdown(Adapter);
    
    /* Free shared resources */
    FreeSharedMemory(Adapter,
                     Adapter->StandardProducer.FrameBufferLength * Adapter->StandardProducer.Count,
                     (PVOID)Adapter->StandardProducer.HostBuffer,
                     &Adapter->StandardProducer.pRing[0].HostAddress);
    
    FreeSharedMemory(Adapter,
                     sizeof(B57XX_RECEIVE_BUFFER_DESCRIPTOR) *  Adapter->StandardProducer.Count,
                     (PVOID)Adapter->StandardProducer.pRing,
                     &Adapter->StandardProducer.RingControlBlock.HostRingAddress);
    
    FreeSharedMemory(Adapter,
                     sizeof(B57XX_RECEIVE_BUFFER_DESCRIPTOR) * Adapter->ReturnConsumer[0].Count,
                     (PVOID)Adapter->ReturnConsumer[0].pRing,
                     &Adapter->ReturnConsumer[0].RingControlBlock.HostRingAddress);
    
    FreeSharedMemory(Adapter,
                     sizeof(B57XX_SEND_BUFFER_DESCRIPTOR) * Adapter->SendProducer[0].Count,
                     (PVOID)Adapter->SendProducer[0].pRing,
                     &Adapter->SendProducer[0].RingControlBlock.HostRingAddress);
    
    if (Adapter->SendProducer[0].pPacketList != NULL)
    {
        NdisFreeMemory(Adapter->SendProducer[0].pPacketList,
                       sizeof(PNDIS_PACKET) * Adapter->SendProducer[0].Count,
                       0);
    }
    
    if (Adapter->Status.pBlock != NULL)
    {
        NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle,
                              sizeof(B57XX_STATUS_BLOCK),
                              FALSE,
                              (PVOID)Adapter->Status.pBlock,
                              Adapter->Status.HostAddress);
    }
    
    if (Adapter->Statistics.pBlock != NULL)
    {
        NdisMFreeSharedMemory(Adapter->MiniportAdapterHandle,
                              sizeof(B57XX_STATISTICS_BLOCK),
                              FALSE,
                              (PVOID)Adapter->Statistics.pBlock,
                              Adapter->Statistics.HostAddress);
    }
    
    /* Free MMIO */
    if (Adapter->IoBase)
    {
        NdisMUnmapIoSpace(Adapter->MiniportAdapterHandle, Adapter->IoBase, Adapter->IoLength);
    }

    /* Destroy the adapter context */
    NdisFreeMemory(Adapter, sizeof(B57XX_ADAPTER), 0);
}

NDIS_STATUS
NTAPI
MiniportSend(_In_ NDIS_HANDLE MiniportAdapterContext,
             _In_ PNDIS_PACKET Packet,
             _In_ UINT Flags)
{
    NDIS_STATUS Status;
    PB57XX_ADAPTER Adapter = (PB57XX_ADAPTER)MiniportAdapterContext;
    PSCATTER_GATHER_LIST SGList = NDIS_PER_PACKET_INFO_FROM_PACKET(Packet,
                                                                   ScatterGatherListPacketInfo);

    
    NdisAcquireSpinLock(&Adapter->SendLock);
    
    ASSERT(SGList != NULL);
    
    for (UINT i = 0; i < SGList->NumberOfElements; i++)
    {
        ASSERT(SGList->Elements[i].Length <= Adapter->MaxFrameSize);
        
        Adapter->SendProducer[0].pPacketList[Adapter->SendProducer[0].ProducerIndex] = Packet;

        Status = NICTransmitPacket(Adapter,
                                   SGList->Elements[i].Address,
                                   SGList->Elements[i].Length,
                                   i == SGList->NumberOfElements - 1 ? B57XX_SBD_PACKET_END : 0);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            Adapter->Statistics.TransmitErrors++;
        }
    }
    
    NdisReleaseSpinLock(&Adapter->SendLock);
    
    return NDIS_STATUS_PENDING;
}

NTSTATUS
NTAPI
DriverEntry(_In_ PDRIVER_OBJECT DriverObject,
            _In_ PUNICODE_STRING RegistryPath)
{
    NDIS_HANDLE WrapperHandle;
    NDIS_STATUS Status;
    NDIS_MINIPORT_CHARACTERISTICS Characteristics = {0};

    Characteristics.MajorNdisVersion = NDIS_MINIPORT_MAJOR_VERSION;
    Characteristics.MinorNdisVersion = NDIS_MINIPORT_MINOR_VERSION;
    Characteristics.CheckForHangHandler = NULL;
    Characteristics.DisableInterruptHandler = MiniportDisableInterrupt;
    Characteristics.EnableInterruptHandler = MiniportEnableInterrupt;
    Characteristics.HaltHandler = MiniportHalt;
    Characteristics.HandleInterruptHandler = MiniportHandleInterrupt;
    Characteristics.InitializeHandler = MiniportInitialize;
    Characteristics.ISRHandler = MiniportISR;
    Characteristics.QueryInformationHandler = MiniportQueryInformation;
    Characteristics.ReconfigureHandler = NULL;
    Characteristics.ResetHandler = MiniportReset;
    Characteristics.SendHandler = MiniportSend;
    Characteristics.SetInformationHandler = MiniportSetInformation;
    Characteristics.TransferDataHandler = NULL;
    /*Characteristics.ReturnPacketHandler = NULL;
    Characteristics.SendPacketsHandler = NULL;
    Characteristics.AllocateCompleteHandler = NULL;*/

    NdisMInitializeWrapper(&WrapperHandle, DriverObject, RegistryPath, NULL);
    if (!WrapperHandle)
    {
        return NDIS_STATUS_FAILURE;
    }

    Status = NdisMRegisterMiniport(WrapperHandle, &Characteristics, sizeof(Characteristics));
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NdisTerminateWrapper(WrapperHandle, 0);
        return Status;
    }

    return NDIS_STATUS_SUCCESS;
}
